#pragma once
#include <stdint.h>
#include <stddef.h>

// Persisted device configuration (NVS via Preferences) - WiFi station
// credentials, MQTT broker target, device identity, and (if TLS is
// used) certificate/key PEM blobs. Written by commissioning_portal,
// read by whatever composes WifiLink/MqttClientWrapper/mqtt_tls_config
// at startup.
//
// Security note, stated plainly rather than glossed over: NVS on its
// own is NOT secure storage - it's a plain flash partition, readable by
// anyone who can dump the flash (JTAG, or desoldering the chip). The
// values here are protected only to the extent ESP32-S3's flash
// encryption is enabled (see docs/secure-boot.md - deliberately not
// enabled yet, that's Ozcan's call on timing - irreversible eFuse
// burn). Until that's on, treat anything stored here as "obscured, not
// secured", not something to rely on against a physically-present
// attacker. Separately: NVS string values are capped at 4000 bytes
// including the null terminator (ESP-IDF's own documented limit) -
// plenty for a typical single-cert PEM, worth checking if a long
// certificate chain is ever used.
//
// Every accessor opens and closes its own Preferences handle rather
// than this class holding one open - no persistent NVS lock to manage,
// nothing to forget to release.
class DeviceConfig {
public:
    // WiFi station credentials.
    bool setWifi(const char *ssid, const char *password);
    bool getWifiSsid(char *out, size_t out_size) const;
    bool getWifiPassword(char *out, size_t out_size) const;
    bool hasWifiCredentials() const;

    // MQTT broker target.
    bool setMqttBroker(const char *host, uint16_t port, bool use_tls);
    bool getMqttHost(char *out, size_t out_size) const;
    uint16_t getMqttPort() const;
    bool getMqttUseTls() const;

    // Device identity - MQTT client ID / topic prefix.
    bool setDeviceId(const char *id);
    bool getDeviceId(char *out, size_t out_size) const;

    // TLS material - only meaningful if getMqttUseTls() is true. Stored
    // and returned as PEM text, null-terminated, as-is - no parsing or
    // validation here (see mqtt_tls_config, which hands these to
    // NetworkClientSecure).
    bool setTlsMaterial(const char *ca_cert, const char *client_cert, const char *client_key);
    bool getCaCert(char *out, size_t out_size) const;
    bool getClientCert(char *out, size_t out_size) const;
    bool getClientKey(char *out, size_t out_size) const;
    bool hasTlsMaterial() const;

    // WireGuard tunnel configuration - added 2026-09-24, following
    // research into how this project's own reference device (see
    // CLAUDE.md's non-negotiable naming policy) and ESPHome's own
    // WireGuard component (built on the same underlying library this
    // project uses, esphome-libs/wireguard) both treat these values:
    // ESPHome's docs say plainly "recommended to use secrets at least
    // for private and pre-shared keys" - i.e. keep them out of a
    // committed/shared file. This project's analogous move is storing
    // them the exact same way TlsMaterial already is below: NVS string
    // fields, with the same honest caveat that already applies there
    // (see this class's own header comment - NVS alone is "obscured,
    // not secured" until flash encryption is on). Field names/types
    // mirror wireguard_link.h's WireguardLink::Config exactly, so
    // whoever wires this into main.cpp isn't translating between two
    // different naming schemes.
    bool setWireguard(const char *private_key, const char *peer_public_key,
                       const char *peer_preshared_key, const char *local_address,
                       const char *local_netmask, const char *peer_endpoint,
                       uint16_t peer_port, uint16_t persistent_keepalive_sec);
    bool getWireguardPrivateKey(char *out, size_t out_size) const;
    bool getWireguardPeerPublicKey(char *out, size_t out_size) const;
    // Preshared key is optional (WireguardLink::Config allows nullptr) -
    // callers should treat an empty string the same as "not set", not
    // as a literal empty preshared key.
    bool getWireguardPeerPresharedKey(char *out, size_t out_size) const;
    bool getWireguardLocalAddress(char *out, size_t out_size) const;
    bool getWireguardLocalNetmask(char *out, size_t out_size) const;
    bool getWireguardPeerEndpoint(char *out, size_t out_size) const;
    uint16_t getWireguardPeerPort() const;
    uint16_t getWireguardPersistentKeepalive() const;
    // True once the required fields (private key, peer public key,
    // local address, peer endpoint) have all been set - preshared key
    // and keepalive are optional, so their absence doesn't count
    // against this.
    bool hasWireguardConfig() const;

    // OTA download server's CA certificate - added 2026-09-24 alongside
    // ota_session.h/ota_trigger.h. Deliberately separate from
    // getCaCert() above (which is documented as MQTT-broker-specific)
    // rather than reused for OTA too - a real deployment may put its
    // firmware download host behind a different CA than its MQTT
    // broker, and this project has generally preferred an explicit
    // field over an assumption that two hosts share trust (same
    // reasoning as WireGuard's preshared key being its own field
    // rather than folded into an existing one). This CA cert governs
    // TRANSPORT trust for the download only - it has no bearing on
    // whether a downloaded image is genuine; that's image_verify.h's
    // job, using a key that lives in firmware source, not here (see
    // ota_signing_key.h for why that one is deliberately NOT
    // NVS/commissioning-configurable like this one is).
    bool setOtaCaCert(const char *ca_cert);
    bool getOtaCaCert(char *out, size_t out_size) const;
    bool hasOtaCaCert() const;

    // The EXPECTED OTA download host - pre-configured, the same way
    // MQTT's broker host and WireGuard's peer endpoint already are,
    // and added to net_allowlist at boot alongside them (main.cpp).
    // This exists because an OTA-trigger command's own "url" field
    // (ota_trigger.h) is untrusted network input - if ota_session.h
    // allowlist-checked THAT host, whoever can publish to the MQTT
    // command topic could point this device at literally any HTTPS
    // host just by naming it in the trigger payload, making the
    // allowlist decorative. Requiring the host to be pre-approved here
    // (known only to whoever can commission the device or already has
    // MQTT-publish access to this device's own topics) keeps
    // net_allowlist's actual guarantee - see its own header comment -
    // intact for OTA the same way it already is for MQTT/WireGuard.
    bool setOtaHost(const char *host);
    bool getOtaHost(char *out, size_t out_size) const;
    bool hasOtaHost() const;

    // LTE PPP backhaul (lib/lte_ppp) config - added 2026-09-24
    // alongside that module. APN is the only strictly required field
    // (mirrors PPPClass::setApn() being the one call PPP.h itself
    // marks "Required for connecting to internet" - setPin() is
    // explicitly "only if the SIM card is protected by PIN"). SIM PIN
    // follows the exact same optional-field precedent WireGuard's
    // preshared key already set in this class: stored as a plain
    // string, absence/empty string both mean "not set, don't call
    // PPP.setPin()" - not a distinct hasLteSimPin() accessor, same
    // reasoning as getWireguardPeerPresharedKey()'s doc comment above.
    bool setLteApn(const char *apn);
    bool getLteApn(char *out, size_t out_size) const;
    bool hasLteApn() const;
    bool setLteSimPin(const char *pin);
    bool getLteSimPin(char *out, size_t out_size) const;

    // Firmware version for OTA downgrade protection.
    bool setFirmwareVersion(uint32_t version);
    uint32_t getFirmwareVersion() const;

    // Wipes every key this class owns (its whole NVS namespace) - e.g.
    // for a factory-reset commissioning flow.
    void clearAll();
};
