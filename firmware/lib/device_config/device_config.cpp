#include "device_config.h"
#include <Preferences.h>

namespace {
const char *NS = "devcfg";

// Defensive helper: always leaves out[] a valid, null-terminated C
// string even if the key is missing or Preferences::getString()'s
// behavior on a missing key is ever ambiguous - out[0] is cleared
// before the real read, so a "not found" case can never leave out[]
// holding garbage.
bool readString(const char *key, char *out, size_t out_size) {
    if (out == nullptr || out_size == 0) return false;
    out[0] = '\0';
    Preferences prefs;
    if (!prefs.begin(NS, true)) return false;
    bool present = prefs.isKey(key);
    if (present) {
        size_t needed = prefs.getBytesLength(key);
        if (needed > out_size) {
            prefs.end();
            return false;
        }
        prefs.getString(key, out, out_size);
    }
    prefs.end();
    return present;
}
} // namespace

bool DeviceConfig::setWifi(const char *ssid, const char *password) {
    if (ssid == nullptr || password == nullptr) return false;
    Preferences prefs;
    if (!prefs.begin(NS, false)) return false;
    prefs.putString("wifi_ssid", ssid);
    prefs.putString("wifi_pass", password);
    prefs.end();
    return true;
}

bool DeviceConfig::getWifiSsid(char *out, size_t out_size) const {
    return readString("wifi_ssid", out, out_size);
}

bool DeviceConfig::getWifiPassword(char *out, size_t out_size) const {
    return readString("wifi_pass", out, out_size);
}

bool DeviceConfig::hasWifiCredentials() const {
    Preferences prefs;
    if (!prefs.begin(NS, true)) return false;
    bool has = prefs.isKey("wifi_ssid") && prefs.isKey("wifi_pass");
    prefs.end();
    return has;
}

bool DeviceConfig::setMqttBroker(const char *host, uint16_t port, bool use_tls) {
    if (host == nullptr) return false;
    Preferences prefs;
    if (!prefs.begin(NS, false)) return false;
    prefs.putString("mqtt_host", host);
    prefs.putUShort("mqtt_port", port);
    prefs.putBool("mqtt_tls", use_tls);
    prefs.end();
    return true;
}

bool DeviceConfig::getMqttHost(char *out, size_t out_size) const {
    return readString("mqtt_host", out, out_size);
}

uint16_t DeviceConfig::getMqttPort() const {
    Preferences prefs;
    if (!prefs.begin(NS, true)) return 0;
    uint16_t port = prefs.getUShort("mqtt_port", 0);
    prefs.end();
    return port;
}

bool DeviceConfig::getMqttUseTls() const {
    Preferences prefs;
    if (!prefs.begin(NS, true)) return false;
    bool use_tls = prefs.getBool("mqtt_tls", false);
    prefs.end();
    return use_tls;
}

bool DeviceConfig::setDeviceId(const char *id) {
    if (id == nullptr) return false;
    Preferences prefs;
    if (!prefs.begin(NS, false)) return false;
    prefs.putString("device_id", id);
    prefs.end();
    return true;
}

bool DeviceConfig::getDeviceId(char *out, size_t out_size) const {
    return readString("device_id", out, out_size);
}

bool DeviceConfig::setTlsMaterial(const char *ca_cert, const char *client_cert,
                                   const char *client_key) {
    if (ca_cert == nullptr || client_cert == nullptr || client_key == nullptr) return false;
    Preferences prefs;
    if (!prefs.begin(NS, false)) return false;
    prefs.putString("ca_cert", ca_cert);
    prefs.putString("cl_cert", client_cert);
    prefs.putString("cl_key", client_key);
    prefs.end();
    return true;
}

bool DeviceConfig::getCaCert(char *out, size_t out_size) const {
    return readString("ca_cert", out, out_size);
}

bool DeviceConfig::getClientCert(char *out, size_t out_size) const {
    return readString("cl_cert", out, out_size);
}

bool DeviceConfig::getClientKey(char *out, size_t out_size) const {
    return readString("cl_key", out, out_size);
}

bool DeviceConfig::hasTlsMaterial() const {
    Preferences prefs;
    if (!prefs.begin(NS, true)) return false;
    bool has = prefs.isKey("ca_cert") && prefs.isKey("cl_cert") && prefs.isKey("cl_key");
    prefs.end();
    return has;
}

bool DeviceConfig::setWireguard(const char *private_key, const char *peer_public_key,
                                 const char *peer_preshared_key, const char *local_address,
                                 const char *local_netmask, const char *peer_endpoint,
                                 uint16_t peer_port, uint16_t persistent_keepalive_sec) {
    // private_key/peer_public_key/local_address/peer_endpoint are the
    // required fields (see hasWireguardConfig()'s doc comment) -
    // peer_preshared_key may legitimately be nullptr/empty (optional in
    // the WireGuard protocol itself), local_netmask defaults to
    // 255.255.255.255 the same way WireguardLink's underlying library
    // does if left unset elsewhere, so it's not required here either.
    if (private_key == nullptr || peer_public_key == nullptr ||
        local_address == nullptr || peer_endpoint == nullptr) {
        return false;
    }
    Preferences prefs;
    if (!prefs.begin(NS, false)) return false;
    prefs.putString("wg_privkey", private_key);
    prefs.putString("wg_pubkey", peer_public_key);
    prefs.putString("wg_psk", peer_preshared_key != nullptr ? peer_preshared_key : "");
    prefs.putString("wg_addr", local_address);
    prefs.putString("wg_netmask", local_netmask != nullptr ? local_netmask : "");
    prefs.putString("wg_endpoint", peer_endpoint);
    prefs.putUShort("wg_port", peer_port);
    prefs.putUShort("wg_keepal", persistent_keepalive_sec);
    prefs.end();
    return true;
}

bool DeviceConfig::getWireguardPrivateKey(char *out, size_t out_size) const {
    return readString("wg_privkey", out, out_size);
}

bool DeviceConfig::getWireguardPeerPublicKey(char *out, size_t out_size) const {
    return readString("wg_pubkey", out, out_size);
}

bool DeviceConfig::getWireguardPeerPresharedKey(char *out, size_t out_size) const {
    return readString("wg_psk", out, out_size);
}

bool DeviceConfig::getWireguardLocalAddress(char *out, size_t out_size) const {
    return readString("wg_addr", out, out_size);
}

bool DeviceConfig::getWireguardLocalNetmask(char *out, size_t out_size) const {
    return readString("wg_netmask", out, out_size);
}

bool DeviceConfig::getWireguardPeerEndpoint(char *out, size_t out_size) const {
    return readString("wg_endpoint", out, out_size);
}

uint16_t DeviceConfig::getWireguardPeerPort() const {
    Preferences prefs;
    if (!prefs.begin(NS, true)) return 0;
    uint16_t port = prefs.getUShort("wg_port", 0);
    prefs.end();
    return port;
}

uint16_t DeviceConfig::getWireguardPersistentKeepalive() const {
    Preferences prefs;
    if (!prefs.begin(NS, true)) return 0;
    uint16_t keepalive = prefs.getUShort("wg_keepal", 0);
    prefs.end();
    return keepalive;
}

bool DeviceConfig::hasWireguardConfig() const {
    Preferences prefs;
    if (!prefs.begin(NS, true)) return false;
    bool has = prefs.isKey("wg_privkey") && prefs.isKey("wg_pubkey") &&
               prefs.isKey("wg_addr") && prefs.isKey("wg_endpoint");
    prefs.end();
    return has;
}

bool DeviceConfig::setOtaCaCert(const char *ca_cert) {
    if (ca_cert == nullptr) return false;
    Preferences prefs;
    if (!prefs.begin(NS, false)) return false;
    prefs.putString("ota_ca", ca_cert);
    prefs.end();
    return true;
}

bool DeviceConfig::getOtaCaCert(char *out, size_t out_size) const {
    return readString("ota_ca", out, out_size);
}

bool DeviceConfig::hasOtaCaCert() const {
    Preferences prefs;
    if (!prefs.begin(NS, true)) return false;
    bool has = prefs.isKey("ota_ca");
    prefs.end();
    return has;
}

bool DeviceConfig::setOtaHost(const char *host) {
    if (host == nullptr) return false;
    Preferences prefs;
    if (!prefs.begin(NS, false)) return false;
    prefs.putString("ota_host", host);
    prefs.end();
    return true;
}

bool DeviceConfig::getOtaHost(char *out, size_t out_size) const {
    return readString("ota_host", out, out_size);
}

bool DeviceConfig::hasOtaHost() const {
    Preferences prefs;
    if (!prefs.begin(NS, true)) return false;
    bool has = prefs.isKey("ota_host");
    prefs.end();
    return has;
}

bool DeviceConfig::setLteApn(const char *apn) {
    if (apn == nullptr) return false;
    Preferences prefs;
    if (!prefs.begin(NS, false)) return false;
    prefs.putString("lte_apn", apn);
    prefs.end();
    return true;
}

bool DeviceConfig::getLteApn(char *out, size_t out_size) const {
    return readString("lte_apn", out, out_size);
}

bool DeviceConfig::hasLteApn() const {
    Preferences prefs;
    if (!prefs.begin(NS, true)) return false;
    bool has = prefs.isKey("lte_apn");
    prefs.end();
    return has;
}

bool DeviceConfig::setLteSimPin(const char *pin) {
    if (pin == nullptr) return false;
    Preferences prefs;
    if (!prefs.begin(NS, false)) return false;
    prefs.putString("lte_pin", pin);
    prefs.end();
    return true;
}

bool DeviceConfig::getLteSimPin(char *out, size_t out_size) const {
    return readString("lte_pin", out, out_size);
}

bool DeviceConfig::setFirmwareVersion(uint32_t version) {
    Preferences prefs;
    if (!prefs.begin(NS, false)) return false;
    prefs.putUInt("fw_ver", version);
    prefs.end();
    return true;
}

uint32_t DeviceConfig::getFirmwareVersion() const {
    Preferences prefs;
    if (!prefs.begin(NS, true)) return 0;
    uint32_t ver = prefs.getUInt("fw_ver", 0);
    prefs.end();
    return ver;
}

void DeviceConfig::clearAll() {
    Preferences prefs;
    if (!prefs.begin(NS, false)) return;
    prefs.clear();
    prefs.end();
}
