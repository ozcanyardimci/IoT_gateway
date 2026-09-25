#include "commissioning_portal.h"
#include <WiFi.h>
#include <WebServer.h>

const char *CommissioningPortal::AP_SSID = "IoT-Gateway-Setup";
const char *CommissioningPortal::AP_PASSWORD = "setup1234"; // >=8 chars - WPA2 minimum

namespace {
WebServer g_server(80);

const char *FORM_HTML =
    "<!DOCTYPE html><html><head>"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>Gateway setup</title></head><body>"
    "<h1>Gateway setup</h1>"
    "<form method=\"POST\" action=\"/save\">"
    "<label>WiFi SSID<br><input name=\"ssid\" required></label><br><br>"
    "<label>WiFi password<br><input name=\"pass\" type=\"password\"></label><br><br>"
    "<label>MQTT broker host<br><input name=\"mqtt_host\" required></label><br><br>"
    "<label>MQTT broker port (1883 plain, 8883 typical for TLS)<br>"
    "<input name=\"mqtt_port\" value=\"1883\"></label><br><br>"
    "<label><input type=\"checkbox\" name=\"mqtt_tls\" value=\"1\"> Use TLS</label><br><br>"
    "<label>Device ID<br><input name=\"device_id\" required></label><br><br>"
    "<label>CA certificate PEM (only if TLS)<br>"
    "<textarea name=\"ca_cert\" rows=\"4\" cols=\"40\"></textarea></label><br><br>"
    "<label>Client certificate PEM (only if TLS)<br>"
    "<textarea name=\"client_cert\" rows=\"4\" cols=\"40\"></textarea></label><br><br>"
    "<label>Client private key PEM (only if TLS)<br>"
    "<textarea name=\"client_key\" rows=\"4\" cols=\"40\"></textarea></label><br><br>"
    "<label>OTA download server hostname (required before this device will "
    "accept an OTA trigger - see device_config.h)<br>"
    "<input name=\"ota_host\" placeholder=\"fw.example.com\"></label><br><br>"
    "<label>OTA download server CA certificate PEM (optional - only needed if "
    "the firmware download host uses a private CA rather than a public one)<br>"
    "<textarea name=\"ota_ca_cert\" rows=\"4\" cols=\"40\"></textarea></label><br><br>"
    "<label><input type=\"checkbox\" name=\"wg_enable\" value=\"1\"> Enable WireGuard</label><br><br>"
    "<label>WireGuard private key (this device's, only if enabled)<br>"
    "<input name=\"wg_privkey\"></label><br><br>"
    "<label>WireGuard peer public key (only if enabled)<br>"
    "<input name=\"wg_pubkey\"></label><br><br>"
    "<label>WireGuard peer preshared key (optional)<br>"
    "<input name=\"wg_psk\"></label><br><br>"
    "<label>WireGuard local tunnel address (only if enabled)<br>"
    "<input name=\"wg_addr\" placeholder=\"10.0.0.2\"></label><br><br>"
    "<label>WireGuard local tunnel netmask<br>"
    "<input name=\"wg_netmask\" value=\"255.255.255.0\"></label><br><br>"
    "<label>WireGuard peer endpoint host (only if enabled)<br>"
    "<input name=\"wg_endpoint\"></label><br><br>"
    "<label>WireGuard peer port<br><input name=\"wg_port\" value=\"51820\"></label><br><br>"
    "<label>WireGuard keepalive seconds (0 = disabled)<br>"
    "<input name=\"wg_keepalive\" value=\"25\"></label><br><br>"
    "<label>LTE APN (leave blank to disable LTE as a backhaul - see "
    "device_config.h)<br>"
    "<input name=\"lte_apn\" placeholder=\"internet\"></label><br><br>"
    "<label>LTE SIM PIN (optional - only if the SIM card is PIN-locked)<br>"
    "<input name=\"lte_pin\" type=\"password\"></label><br><br>"
    "<button type=\"submit\">Save and reboot</button>"
    "</form></body></html>";

const char *SAVED_HTML =
    "<!DOCTYPE html><html><body><h1>Saved.</h1>"
    "<p>Rebooting into normal operation - reconnect to your usual network.</p>"
    "</body></html>";
} // namespace

bool CommissioningPortal::begin(DeviceConfig &config) {
    config_ = &config;
    saved_ = false;

    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
        return false;
    }

    g_server.on("/", HTTP_GET, []() {
        g_server.send(200, "text/html", FORM_HTML);
    });

    g_server.on("/save", HTTP_POST, [this]() {
        if (!g_server.hasArg("ssid") || !g_server.hasArg("mqtt_host") ||
            !g_server.hasArg("device_id")) {
            g_server.send(400, "text/plain", "Missing required field(s)");
            return;
        }

        String ssid = g_server.arg("ssid");
        String pass = g_server.hasArg("pass") ? g_server.arg("pass") : String("");
        config_->setWifi(ssid.c_str(), pass.c_str());

        String host = g_server.arg("mqtt_host");
        uint16_t port = (uint16_t)g_server.arg("mqtt_port").toInt();
        if (port == 0) port = 1883;
        bool use_tls = g_server.hasArg("mqtt_tls");
        config_->setMqttBroker(host.c_str(), port, use_tls);

        String device_id = g_server.arg("device_id");
        config_->setDeviceId(device_id.c_str());

        if (use_tls) {
            String ca = g_server.hasArg("ca_cert") ? g_server.arg("ca_cert") : String("");
            String cc = g_server.hasArg("client_cert") ? g_server.arg("client_cert") : String("");
            String ck = g_server.hasArg("client_key") ? g_server.arg("client_key") : String("");
            config_->setTlsMaterial(ca.c_str(), cc.c_str(), ck.c_str());
        }

        // Unlike the MQTT TLS material above, this isn't gated behind a
        // checkbox/required-field validation here - it's a single
        // optional form field. But it's NOT optional at the point an
        // OTA actually runs: NetworkClientSecure needs an explicit CA
        // (device_config.h's getOtaCaCert() own header explains why
        // this is a separate field from ca_cert above, not reused) -
        // ota_session.h fails closed with nothing to verify the
        // download server's TLS certificate against if this was never
        // set, same "fail closed on missing security material" pattern
        // as image_verify.h's OTA_SIGNING_PUBLIC_KEY_PEM placeholder. A
        // device that will never receive an OTA trigger simply doesn't
        // need this filled in yet.
        if (g_server.hasArg("ota_host") && g_server.arg("ota_host").length() > 0) {
            config_->setOtaHost(g_server.arg("ota_host").c_str());
        }
        if (g_server.hasArg("ota_ca_cert") && g_server.arg("ota_ca_cert").length() > 0) {
            config_->setOtaCaCert(g_server.arg("ota_ca_cert").c_str());
        }

        // LTE APN/SIM PIN follow the exact same "single optional field,
        // no checkbox gate" pattern as ota_host above - an APN present in
        // DeviceConfig is itself what main.cpp's connectNetwork() checks
        // (hasLteApn()) to decide whether to attempt LTE at all, so there's
        // no separate "enable" checkbox needed the way WireGuard below has
        // one.
        if (g_server.hasArg("lte_apn") && g_server.arg("lte_apn").length() > 0) {
            config_->setLteApn(g_server.arg("lte_apn").c_str());
        }
        if (g_server.hasArg("lte_pin") && g_server.arg("lte_pin").length() > 0) {
            config_->setLteSimPin(g_server.arg("lte_pin").c_str());
        }

        // WireGuard fields are only required (by DeviceConfig::setWireguard's
        // own validation - see device_config.cpp) when wg_enable is checked;
        // an unchecked box just means those fields aren't written, same
        // pattern as the TLS section above. Not re-validating field-by-field
        // here - if a required one was left blank with the box checked,
        // setWireguard() returning false is caught and reported rather than
        // silently accepted as "configured".
        if (g_server.hasArg("wg_enable")) {
            String privkey = g_server.hasArg("wg_privkey") ? g_server.arg("wg_privkey") : String("");
            String pubkey = g_server.hasArg("wg_pubkey") ? g_server.arg("wg_pubkey") : String("");
            String psk = g_server.hasArg("wg_psk") ? g_server.arg("wg_psk") : String("");
            String addr = g_server.hasArg("wg_addr") ? g_server.arg("wg_addr") : String("");
            String netmask = g_server.hasArg("wg_netmask") ? g_server.arg("wg_netmask") : String("");
            String endpoint = g_server.hasArg("wg_endpoint") ? g_server.arg("wg_endpoint") : String("");
            uint16_t wg_port = (uint16_t)(g_server.hasArg("wg_port") ? g_server.arg("wg_port").toInt() : 51820);
            uint16_t wg_keepalive = (uint16_t)(g_server.hasArg("wg_keepalive") ? g_server.arg("wg_keepalive").toInt() : 0);

            bool wg_ok = config_->setWireguard(privkey.c_str(), pubkey.c_str(),
                                                psk.length() ? psk.c_str() : nullptr,
                                                addr.c_str(), netmask.c_str(),
                                                endpoint.c_str(), wg_port, wg_keepalive);
            if (!wg_ok) {
                g_server.send(400, "text/plain",
                               "WireGuard enabled but a required field (private key, "
                               "peer public key, local address, or peer endpoint) was left blank");
                return;
            }
        }

        saved_ = true;
        g_server.send(200, "text/html", SAVED_HTML);
    });

    g_server.begin();
    return true;
}

void CommissioningPortal::handleClient() {
    g_server.handleClient();
}

void CommissioningPortal::stop() {
    g_server.stop();
    WiFi.softAPdisconnect(true);
}
