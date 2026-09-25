#include <Arduino.h>
#include <Wire.h>
#include <NetworkClientSecure.h>
#include <WiFiClient.h>
#include <string.h> // memcpy - used by the OTA MQTT message callback
                     // below; not otherwise pulled in explicitly
                     // anywhere in this file before now

#include "pin_map.h"

// --- Local I/O (F2) ---
#include "di_driver.h"
#include "relay_driver.h"
#include "ads1115_adc.h"
#include "mcp4725_dac.h"
#include "status_leds.h"

// --- Connectivity (F2) ---
#include "wifi_link.h"
#include "ethernet_link.h"
#include "lte_modem.h"
#include "lte_ppp.h"
#include "rs485_serial.h"

// --- Protocol/state (F1) ---
#include "io_state.h"
#include "mqtt_payload.h"

// --- Industrial layer (F3) ---
#include "device_config.h"
#include "commissioning_portal.h"
#include "mqtt_tls_config.h"
#include "mqtt_client_wrapper.h"
#include "rule_engine.h"
#include "event_log.h"
#include "ntp_time.h"
#include "ota_update.h"
#include "power_safety.h"
#include "reset_reason.h"
#include "net_allowlist.h"

// --- System integration (F4) ---
#include "system_watchdog.h"
#include "modbus_master.h"
#include "wireguard_link.h"
#include "ota_trigger.h"
#include "ota_session.h"

// ============================================================================
// F4: system integration. Ties every module above together into one boot
// sequence + one running system, on the architecture agreed for this
// project (docs/roadmap.md's Firmware roadmap, F4 entry):
//
//   - Modbus master polling -> its OWN FreeRTOS task (ModbusMasterTask),
//     because its frame timing can't tolerate being stalled by a slow TLS
//     handshake or OTA flash write happening elsewhere.
//   - Everything else -> this file's loop(), cooperative/poll()-driven,
//     the same pattern most of these modules were already written in.
//   - SystemWatchdog is fed from both - either task hanging reboots the
//     device (docs/roadmap.md's F4 entry has the full reasoning).
//
// WireGuard (wireguard_link.h) is now wired: DeviceConfig
// (device_config.h) has a real NVS-backed storage path
// (setWireguard()/getWireguard*()/hasWireguardConfig(), added
// 2026-09-24) and commissioning_portal's form collects the fields.
// It's started in setup() AFTER NTP sync, not just after the network
// comes up - ESPHome's own WireGuard component documents the tunnel
// as requiring a synchronized system clock, and this project follows
// that rather than guessing it doesn't matter. If NTP sync fails,
// WireGuard is deliberately skipped this boot (logged, not silently
// retried in loop() yet) rather than started against a clock that may
// be wrong.
//
// OTA's update-trigger flow is now wired too: this device subscribes
// to "<device_id>/cmd/ota" and, on a message that parses as a valid
// ota_trigger.h command, downloads/verifies/installs it via
// ota_session.h's runOtaSession() - see that file's own header comment
// for the full flow (allowlist gate on a PRE-CONFIGURED OTA host, not
// the trigger's own untrusted url field; HTTPS download; SHA-256 +
// ECDSA-P256 signature check against a key compiled into firmware
// (image_verify.h/ota_signing_key.h) BEFORE the new image is ever
// marked bootable). Parsing happens inside the MQTT callback itself
// (fast, pure, no I/O - ota_trigger_parse() never blocks); the actual
// download+flash-write is deferred to loop() via g_ota_pending, since
// that part can legitimately take tens of seconds and has no business
// running inside a PubSubClient callback. ota_update.h's own boot-time
// rollback confirmation (safety-critical, independent of how an update
// gets triggered) was already wired before this and is unchanged.
//
// LTE is now a real backhaul tier, not just status monitoring:
// lib/lte_ppp (arduino-esp32's built-in PPP library, wrapping ESP-IDF's
// esp_modem component) gives the modem an actual PPP/PDP bridge, the
// missing piece this comment used to flag here. lib/lte_modem was
// trimmed down to power/reset GPIO control only (its old AT+CSQ/
// AT+CREG-polling transport moved out - see that header's own note);
// lte_at_parse.h/.c was deleted as dead code once nothing called
// sendAt() anymore (confirmed via repo-wide grep before deleting - no
// other dependents). See lib/lte_ppp/lte_ppp.h for the full mechanism
// (model choice, UART-conflict avoidance with Rs485Serial, the
// deliberate choice to leave PWRKEY/RESET owned solely by lte_modem)
// and docs/roadmap.md's F4 entry for what was researched/verified
// before writing any of it.
//
// Network priority: Ethernet, then LTE, then WiFi as a last resort.
// NOT this file's own assumption - Ozcan was asked explicitly (two
// separate questions, 2026-09-24) once LTE actually existed to
// reconsider the order against, rather than this being decided
// unilaterally:
//   1. Order: chose Ethernet -> LTE -> WiFi over keeping WiFi second.
//      The real reference device's documented pattern is wired-primary
//      with automatic CELLULAR failover, not WiFi, so LTE moved ahead
//      to match that. WiFi stays as a tier (not dropped entirely, which
//      would've matched the reference device even more closely) because
//      it's genuinely useful for this project's own bench/dev work
//      without Ethernet or a live SIM - a need the commercial reference
//      device's architecture doesn't have to account for.
//   2. Timing: chose to KEEP connectNetwork() boot-time-only rather than
//      add live failover monitoring now. connectNetwork() still runs
//      once, in setup() - there's no "a backhaul dropped mid-operation,
//      switch automatically" monitoring in loop() for ANY of the three
//      tiers (not a gap LTE introduces - Ethernet/WiFi already had none).
//      The real reference device's "automatic" cellular failover implies
//      live monitoring; this build's boot-time-only fallback is a real,
//      explicitly-scoped fidelity gap against that, not a hidden one -
//      live monitoring was deliberately left as a separate, distinctly-
//      larger follow-up (periodic health checks, switch-on-drop,
//      fail-back-on-recovery logic), not folded into this pass.
// ============================================================================

// ---- Module instances (file-scope, single instance each - matches
// this project's existing main.cpp precedent) ----

DeviceConfig g_config;
CommissioningPortal g_portal;

DigitalInputs g_di;
RelayOutputs g_relays;
AdsAnalogInput g_ai;
McpAnalogOutput g_ao; // L9 (2026-09-24): initialized and begin()-checked below, but
                       // nothing currently calls set()/write() on it - no consumer
                       // wired up yet. Left initialized rather than removed since the
                       // hardware channel exists on the board; flagging so this doesn't
                       // read as "finished and in use" until something actually drives it.
StatusLeds g_leds;
io_state_t g_io_state;

WifiLink g_wifi;
EthernetLink g_eth;
LteModem g_lte;
LtePpp g_lte_ppp;

NetworkClientSecure g_mqtt_tls_client;
NetworkClientSecure g_ota_https_client; // OTA downloads get their OWN
    // NetworkClientSecure, deliberately separate from g_mqtt_tls_client
    // above - that one's underlying TCP connection is MQTT's own
    // persistent session; running an unrelated HTTPS GET through it
    // (ota_session.h) would disrupt it, not just borrow it
WiFiClient g_mqtt_plain_client;
MqttTlsConfig g_mqtt_tls_config;
MqttClientWrapper *g_mqtt = nullptr; // constructed in setup() once the
                                      // transport (TLS vs plain) and
                                      // broker/client-id strings from
                                      // g_config are known

NtpTime g_ntp;
EventLog g_log;
PowerSafety g_power_safety;
NetAllowlist g_allowlist;

rule_engine_t g_rules;
OtaUpdate g_ota;

Rs485Serial g_rs485;
ModbusMasterTask g_modbus;
SystemWatchdog g_watchdog;
WireguardLink g_wireguard;

char g_device_id[64] = "gateway";
char g_mqtt_host[128] = "";
uint16_t g_mqtt_port = 1883;
bool g_mqtt_use_tls = false;

// WireGuard config, loaded from DeviceConfig in setup() (before
// connectNetwork(), so g_wg_peer_endpoint is ready in time for the
// allowlist population below) but not actually started until after
// NTP sync succeeds - see this file's header comment for why.
bool g_wg_configured = false;
char g_wg_private_key[64] = "";
char g_wg_peer_public_key[64] = "";
char g_wg_peer_preshared_key[64] = "";
char g_wg_local_address[32] = "";
char g_wg_local_netmask[32] = "";
char g_wg_peer_endpoint[128] = "";
uint16_t g_wg_peer_port = 0;
uint16_t g_wg_keepalive = 0;

// OTA trigger staging - set (synchronously, from inside the MQTT
// message callback) once a "<device_id>/cmd/ota" message parses as a
// valid command; cleared and acted on from loop() (see this file's
// header comment for why the actual download is deferred there rather
// than run inside the callback).
OtaTriggerCommand g_pending_ota_cmd;
bool g_ota_pending = false;
char g_ota_raw_payload[800]; // must hold up to
                              // MqttClientWrapper::MQTT_RX_BUFFER_SIZE
                              // (700) bytes plus a null terminator,
                              // with headroom

unsigned long g_last_publish_ms = 0;
const unsigned long PUBLISH_INTERVAL_MS = 5000;

// BUG FIX (2026-09-24, L8): StatusLed::HEARTBEAT existed in
// status_leds.h's enum from the start but nothing in loop() ever
// called g_leds.set(StatusLed::HEARTBEAT, ...) - the LED (or its
// software slot, on hardware where it's not physically wired) just
// sat unused. A simple 1Hz toggle (500ms on, 500ms off) is the
// standard "firmware is alive and loop() hasn't hung" indicator this
// LED exists for, same idiom as g_last_publish_ms/PUBLISH_INTERVAL_MS
// just above.
unsigned long g_last_heartbeat_toggle_ms = 0;
bool g_heartbeat_led_state = false;
const unsigned long HEARTBEAT_BLINK_INTERVAL_MS = 500;

// ---- Helpers ----

static void connectNetwork() {
    // Ethernet -> LTE -> WiFi. Reordered 2026-09-24 (Ozcan's call,
    // asked explicitly rather than decided unilaterally - see
    // docs/roadmap.md's F4 entry) once LTE existed to reconsider
    // against: the real reference device's documented pattern is
    // wired-primary with automatic CELLULAR failover, not WiFi, so LTE
    // moved ahead of WiFi to match that priority. WiFi stays as a
    // last-resort tier rather than being dropped entirely - useful for
    // bench/dev work without Ethernet or a live SIM, which the
    // reference device's own architecture doesn't need to account for
    // but this project's development workflow does.
    //
    // Ethernet: short bounded wait, not indefinite - a device with no
    // cable connected should fall through to the next tier rather than
    // hang here forever.
    g_eth.begin();
    unsigned long start = millis();
    while (!g_eth.isConnected() && (millis() - start) < 5000) {
        delay(50);
        g_watchdog.feed();
    }
    if (g_eth.isConnected()) {
        g_log.info(0, "net", "ethernet up");
        return;
    }

    // LTE - second tier, only attempted if an APN is configured
    // (DeviceConfig::hasLteApn(), commissioning_portal's "lte_apn"
    // field - an unconfigured APN means this device has no SIM/LTE
    // hardware fitted, or the installer simply hasn't set it up yet;
    // either way, attempting PPP.begin() against that is pointless).
    // PRECONDITION: LteModem::begin()+powerOn() must already have run
    // (setup() does this immediately before calling connectNetwork(),
    // not after - see this file's header comment on why that ordering
    // matters). Same bounded-wait-plus-watchdog-feed shape as Ethernet
    // above, split into two waits (attach, then data mode) because
    // lte_ppp.h's connection sequence has two distinct stages to wait
    // through - see that header for why.
    if (g_config.hasLteApn()) {
        char apn[64] = "";
        char sim_pin[16] = "";
        g_config.getLteApn(apn, sizeof(apn));
        g_config.getLteSimPin(sim_pin, sizeof(sim_pin));

        if (!g_lte_ppp.begin(apn, sim_pin[0] != '\0' ? sim_pin : nullptr)) {
            g_log.warn(0, "net", "lte ppp begin failed");
        } else {
            start = millis();
            while (!g_lte_ppp.isAttached() && (millis() - start) < 20000) {
                delay(50);
                g_watchdog.feed();
            }
            if (!g_lte_ppp.isAttached()) {
                g_log.warn(0, "net", "lte network registration timed out");
            } else if (!g_lte_ppp.switchToDataMode()) {
                g_log.warn(0, "net", "lte switch to data mode failed");
            } else {
                start = millis();
                while (!g_lte_ppp.isConnected() && (millis() - start) < 20000) {
                    delay(50);
                    g_watchdog.feed();
                }
                if (g_lte_ppp.isConnected()) {
                    g_log.info(0, "net", "lte up");
                    return;
                }
                g_log.warn(0, "net", "lte data connection timed out");
            }
        }
    } else {
        g_log.warn(0, "net", "no lte apn configured - skipping to wifi");
    }

    // WiFi - third/last-resort tier.
    if (g_config.hasWifiCredentials()) {
        char ssid[64], pass[64];
        g_config.getWifiSsid(ssid, sizeof(ssid));
        g_config.getWifiPassword(pass, sizeof(pass));
        g_wifi.begin(ssid, pass);
        start = millis();
        while (!g_wifi.isConnected() && (millis() - start) < 15000) {
            delay(50);
            g_watchdog.feed();
        }
        if (g_wifi.isConnected()) {
            g_log.info(0, "net", "wifi up");
            return;
        }
        g_log.warn(0, "net", "wifi connect timed out");
    } else {
        g_log.warn(0, "net", "no ethernet, lte, or wifi backhaul available");
    }
}

static bool networkIsUp() {
    return g_eth.isConnected() || g_wifi.isConnected() || g_lte_ppp.isConnected();
}

static void publishState() {
    if (g_mqtt == nullptr || !g_mqtt->isConnected()) return;

    mqtt_gateway_state_t mqtt_state;
    io_state_to_mqtt_state(&g_io_state, g_ntp.isSynced() ? g_ntp.nowUnix() : 0, &mqtt_state);

    char payload[256];
    int len = mqtt_payload_build_json(&mqtt_state, payload, sizeof(payload));
    if (len <= 0) return;

    char topic[96];
    snprintf(topic, sizeof(topic), "%s/state", g_device_id);
    g_mqtt->publish(topic, payload);
}

static void readLocalInputs() {
    bool di[DigitalInputs::CHANNEL_COUNT];
    g_di.readAll(di);
    for (int i = 0; i < DigitalInputs::CHANNEL_COUNT; i++) {
        io_state_set_di(&g_io_state, i, di[i] ? 1 : 0);
    }

    for (int i = 0; i < AdsAnalogInput::CHANNEL_COUNT; i++) {
        int16_t raw;
        if (g_ai.readChannel(i, &raw)) {
            io_state_set_ai(&g_io_state, i, (uint16_t)raw);
        }
    }
}

static void applyRelayCommands(const int commands[4], const int applied[4]) {
    for (int i = 0; i < 4; i++) {
        if (applied[i]) {
            g_relays.setChannel(i, commands[i] != 0);
            io_state_set_relay(&g_io_state, i, commands[i] != 0 ? 1 : 0);
        }
    }
}

// ---- Arduino entry points ----

void setup() {
    Serial.begin(115200);

    // (Boot-time rollback confirmation moved to after network connect)

    ResetCause cause = getResetCause();
    uint8_t brownout_streak = g_power_safety.begin(cause);
    unsigned long startup_delay = g_power_safety.recommendedStartupDelayMs();
    if (startup_delay > 0) {
        delay(startup_delay);
    }

    if (!g_log.begin()) { Serial.println("EventLog::begin failed"); }
    g_log.info(0, "boot", resetCauseToString(cause));
    if (brownout_streak > 0) {
        char msg[48];
        snprintf(msg, sizeof(msg), "brownout streak %u, delayed %lums",
                 brownout_streak, startup_delay);
        g_log.warn(0, "boot", msg);
    }

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    if (!g_leds.begin()) { g_log.warn(0, "boot", "StatusLeds::begin failed"); }
    g_di.begin(); // DigitalInputs::begin() only configures GPIO pinModes - nothing to fail, no bool to check
    g_relays.begin(); // de-energizes all relays - the safe startup state; RelayOutputs::begin() only configures GPIO - nothing to fail
    if (!g_ai.begin()) { g_log.warn(0, "boot", "AdsAnalogInput::begin failed"); }
    if (!g_ao.begin()) { g_log.warn(0, "boot", "McpAnalogOutput::begin failed"); }
    io_state_init(&g_io_state);

    // Watchdog starts here, deliberately before anything below that can
    // legitimately run for a while (commissioning's wait-for-form loop,
    // connectNetwork()'s wait-for-link loops) - starting it any later
    // would make those loops' feed() calls dead code, feeding a
    // watchdog that isn't running yet.
    if (!g_watchdog.begin(15000)) {
        g_log.error(0, "boot", "SystemWatchdog::begin failed");
    } // 15s: comfortably longer than a TLS
                              // handshake or a single OTA Update.write()
                              // chunk, this project's two longest known
                              // single blocking calls (see
                              // modbus_master.h's header comment) -
                              // sized with real headroom above those,
                              // not average-case loop timing
    g_watchdog.addCurrentTask(); // registers THIS task (Arduino's
                                  // loopTask, since setup()/loop() both
                                  // run in it) - ModbusMasterTask
                                  // registers itself separately inside
                                  // its own runLoop(), see
                                  // modbus_master.cpp

    // Commissioning: no stored WiFi credentials -> assume this is a
    // first-boot/field-commissioning device (commissioning_portal.h's
    // own documented usage pattern) and stop here until it's
    // configured, rather than falling through with an empty config.
    // Feeds the watchdog every pass - this loop can legitimately run
    // for a long time waiting on a human to fill out the web form (a
    // device sitting uncommissioned on a shelf), which shouldn't itself
    // look like a hang.
    if (!g_config.hasWifiCredentials()) {
        g_log.info(0, "boot", "no wifi credentials - starting commissioning portal");
        
        if (g_ota.isPendingVerification()) {
            g_ota.confirmValid();
            g_log.info(0, "ota", "boot confirmed valid in commissioning mode");
        }
        
        g_portal.begin(g_config);
        while (!g_portal.configSaved()) {
            g_portal.handleClient();
            g_watchdog.feed();
            delay(10);
        }
        g_portal.stop();
        ESP.restart();
        return; // unreachable, but explicit rather than relying on
                // ESP.restart() never returning
    }

    g_config.getDeviceId(g_device_id, sizeof(g_device_id));
    g_config.getMqttHost(g_mqtt_host, sizeof(g_mqtt_host));
    g_mqtt_port = g_config.getMqttPort();
    g_mqtt_use_tls = g_config.getMqttUseTls();

    g_wg_configured = g_config.hasWireguardConfig();
    if (g_wg_configured) {
        g_config.getWireguardPrivateKey(g_wg_private_key, sizeof(g_wg_private_key));
        g_config.getWireguardPeerPublicKey(g_wg_peer_public_key, sizeof(g_wg_peer_public_key));
        bool has_psk = g_config.getWireguardPeerPresharedKey(g_wg_peer_preshared_key,
                                                               sizeof(g_wg_peer_preshared_key)) &&
                       g_wg_peer_preshared_key[0] != '\0';
        if (!has_psk) g_wg_peer_preshared_key[0] = '\0';
        g_config.getWireguardLocalAddress(g_wg_local_address, sizeof(g_wg_local_address));
        g_config.getWireguardLocalNetmask(g_wg_local_netmask, sizeof(g_wg_local_netmask));
        g_config.getWireguardPeerEndpoint(g_wg_peer_endpoint, sizeof(g_wg_peer_endpoint));
        g_wg_peer_port = g_config.getWireguardPeerPort();
        g_wg_keepalive = g_config.getWireguardPersistentKeepalive();
    }

    // NetAllowlist: populated from the same config it's about to gate -
    // see net_allowlist.h's own header for what this does and doesn't
    // protect against. Real integration of the intended call pattern
    // ("consult before dialing out"), not a claim that this defends
    // against a compromised DeviceConfig specifically.
    char ota_host[NetAllowlist::MAX_HOST_LEN] = ""; // local, not
        // global - unlike g_device_id/g_mqtt_host above, nothing past
        // this setup() block needs it again (runOtaSession() re-reads
        // it live from g_config via allowlist matching, not from a
        // cached copy)
    bool has_ota_host = g_config.hasOtaHost() && g_config.getOtaHost(ota_host, sizeof(ota_host));

    g_allowlist.clear();
    if (g_mqtt_host[0] != '\0') g_allowlist.add(g_mqtt_host);
    g_allowlist.add("pool.ntp.org");
    if (has_ota_host && ota_host[0] != '\0') {
        // See device_config.h's setOtaHost() for why this is a
        // separate, pre-configured field rather than trusting
        // whatever host an OTA-trigger command's own "url" names.
        g_allowlist.add(ota_host);
    }
    if (g_wg_configured && g_wg_peer_endpoint[0] != '\0') {
        if (!g_allowlist.add(g_wg_peer_endpoint)) {
            // Hostname too long for NetAllowlist::MAX_HOST_LEN, or the
            // list is already full - isAllowed() below then fails
            // closed (net_allowlist.h's documented default) and
            // WireGuard simply won't start. Logged once connectNetwork()
            // has brought the network up and g_log has somewhere useful
            // to send this, not here.
        }
    }

    // LTE power-on MUST happen before connectNetwork() - lte_ppp's
    // begin() (called from inside connectNetwork(), if an APN is
    // configured) needs the modem already powered up and past its
    // boot-time settle before it starts talking AT commands to it
    // (see lte_ppp.h's header comment). Harmless to power the modem
    // on even when no APN is configured/PPP is never attempted - it
    // just idles.
    if (!g_lte.begin()) {
        g_log.error(0, "boot", "LteModem::begin failed");
    }
    g_lte.powerOn(); // LteModem::powerOn() is a fixed-duration GPIO pulse with no feedback path - nothing to fail, no bool to check

    connectNetwork();
    
    if (networkIsUp() && g_ota.isPendingVerification()) {
        g_ota.confirmValid();
        g_log.info(0, "ota", "boot confirmed valid after network connect");
    }

    if (networkIsUp() && g_allowlist.isAllowed("pool.ntp.org")) {
        g_ntp.begin("pool.ntp.org");
        g_ntp.waitForSync(10000);
    }

    // WireGuard: started only after the NTP block above, and only if
    // that sync actually succeeded - ESPHome's WireGuard component
    // documents the tunnel as requiring a synchronized system clock
    // (see this file's header comment). Skipping silently on missing
    // config (not configured at all) vs. logging a warning on a
    // present-but-blocked config (allowlist/no network/no sync) so a
    // real misconfiguration is visible without spamming devices that
    // simply don't use WireGuard.
    if (g_wg_configured) {
        if (networkIsUp() && g_ntp.isSynced() && g_allowlist.isAllowed(g_wg_peer_endpoint)) {
            WireguardLink::Config wg_config;
            wg_config.private_key = g_wg_private_key;
            wg_config.public_key = g_wg_peer_public_key;
            wg_config.preshared_key = g_wg_peer_preshared_key[0] != '\0' ? g_wg_peer_preshared_key : nullptr;
            wg_config.local_address = g_wg_local_address;
            wg_config.local_netmask = g_wg_local_netmask[0] != '\0' ? g_wg_local_netmask : "255.255.255.255";
            wg_config.endpoint = g_wg_peer_endpoint;
            wg_config.endpoint_port = g_wg_peer_port != 0 ? g_wg_peer_port : 51820;
            wg_config.persistent_keepalive_sec = g_wg_keepalive;

            if (g_wireguard.begin(wg_config) && g_wireguard.connect()) {
                g_log.info(0, "wireguard", "tunnel starting");
            } else {
                g_log.error(0, "wireguard", "begin/connect failed");
            }
        } else if (!g_ntp.isSynced()) {
            g_log.warn(0, "wireguard", "NTP not synced - not starting tunnel this boot");
        } else if (!g_allowlist.isAllowed(g_wg_peer_endpoint)) {
            g_log.warn(0, "wireguard", "peer endpoint not in allowlist - not connecting");
        } else {
            g_log.warn(0, "wireguard", "network down - not starting tunnel");
        }
    }

    if (networkIsUp() && g_mqtt_host[0] != '\0' && g_allowlist.isAllowed(g_mqtt_host)) {
        Client *net_client;
        if (g_mqtt_use_tls) {
            g_mqtt_tls_config.apply(g_config, g_mqtt_tls_client);
            net_client = &g_mqtt_tls_client;
        } else {
            net_client = &g_mqtt_plain_client;
        }
        g_mqtt = new MqttClientWrapper(*net_client, g_mqtt_host, g_mqtt_port, g_device_id);
        g_mqtt->begin();

        char ota_cmd_topic[96];
        snprintf(ota_cmd_topic, sizeof(ota_cmd_topic), "%s/cmd/ota", g_device_id);
        g_mqtt->subscribe(ota_cmd_topic);
        g_mqtt->onMessage([](const char *topic, const uint8_t *payload, unsigned int length) {
            (void)topic; // this device subscribes to exactly one topic right now
            if (length >= sizeof(g_ota_raw_payload)) {
                g_log.warn(0, "ota", "command payload too large - ignored");
                return;
            }
            memcpy(g_ota_raw_payload, payload, length);
            g_ota_raw_payload[length] = '\0';

            if (ota_trigger_parse(g_ota_raw_payload, &g_pending_ota_cmd)) {
                g_ota_pending = true;
                g_log.info(0, "ota", "trigger command received and parsed");
            } else {
                g_log.warn(0, "ota", "trigger command failed to parse - ignored");
            }
        });
    } else if (g_mqtt_host[0] != '\0') {
        g_log.warn(0, "mqtt", "broker host not in allowlist or network down - not connecting");
    }

    rule_engine_init(&g_rules);
    // No rules loaded yet - same "real mechanism, no invented content"
    // approach as ModbusMasterTask's empty target list. Rules are a
    // per-deployment decision (which DI/AI conditions should drive
    // which relay), not something to guess here.

    // ModbusMasterTask::begin() owns Rs485Serial::begin() internally
    // (see modbus_master.cpp) - not called again here.
    if (g_modbus.begin(g_rs485, 9600)) {
        if (!g_modbus.start(&g_watchdog)) {
            g_log.error(0, "modbus", "ModbusMasterTask::start failed");
        }
    } else {
        g_log.error(0, "modbus", "ModbusMasterTask::begin failed");
    }
    // No targets added - see modbus_master.h's own header for why.
}

void loop() {
    readLocalInputs();

    int relay_commands[4];
    int relay_applied[4];
    rule_engine_evaluate(&g_rules, &g_io_state, relay_commands, relay_applied);
    applyRelayCommands(relay_commands, relay_applied);

    if (g_mqtt != nullptr) {
        g_mqtt->loop();
    }

    unsigned long now = millis();
    if ((long)(now - (g_last_publish_ms + PUBLISH_INTERVAL_MS)) >= 0) {
        publishState();
        g_last_publish_ms = now;
    }

    // No periodic LTE poll()/reconnect-recovery loop here, by design -
    // matches this project's existing fidelity level for Ethernet/WiFi
    // too: connectNetwork() runs once, at boot, in setup(); none of the
    // three backhauls gets live reconnect monitoring from loop() yet.
    // PPP.connected() (unlike the old AT+CREG polling this replaced) is
    // a direct read of PPPClass's own already-event-backed state, not
    // an AT round-trip, so no interval throttle is needed here either -
    // matches how WIFI/ETH below are read every loop() pass.

    g_leds.set(StatusLed::WIFI, g_wifi.isConnected());
    g_leds.set(StatusLed::ETH, g_eth.isConnected());
    g_leds.set(StatusLed::LTE, g_lte_ppp.isConnected());
    g_leds.set(StatusLed::PWR_OK, true);
    g_leds.set(StatusLed::FAULT, g_mqtt_host[0] != '\0' && (g_mqtt == nullptr || !g_mqtt->isConnected()));

    // BUG FIX (2026-09-24, L8): see g_last_heartbeat_toggle_ms's
    // declaration above - this is the actual toggle, same
    // now-vs-last-timestamp idiom as the publishState() throttle a
    // few lines up, just with a much shorter interval.
    if ((long)(now - (g_last_heartbeat_toggle_ms + HEARTBEAT_BLINK_INTERVAL_MS)) >= 0) {
        g_heartbeat_led_state = !g_heartbeat_led_state;
        g_leds.set(StatusLed::HEARTBEAT, g_heartbeat_led_state);
        g_last_heartbeat_toggle_ms = now;
    }

    if (g_ota_pending) {
        g_ota_pending = false;
        // Real HTTPS download + flash write - can legitimately take
        // tens of seconds. Feeds the watchdog itself throughout (see
        // ota_session.h's own header comment) - nothing extra needed
        // here for that, same as connectNetwork()'s and the
        // commissioning portal's own long-running loops elsewhere in
        // this file.
        bool ota_ok = runOtaSession(g_pending_ota_cmd, g_config, g_ota_https_client,
                                     g_allowlist, g_watchdog, g_log);
        if (ota_ok) {
            g_log.info(0, "ota", "update committed - rebooting to apply");
            delay(200); // let the log line actually get written before reboot
            ESP.restart();
            return; // unreachable, but explicit rather than relying on
                    // ESP.restart() never returning - same style as
                    // the commissioning-portal path above
        }
        // Failure: runOtaSession() already logged why. Current
        // firmware just keeps running - no crash/reboot needed, the
        // inactive OTA slot simply gets overwritten by the next
        // attempt.
    }

    g_watchdog.feed();
    delay(10);
}
