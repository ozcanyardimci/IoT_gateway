#include "mqtt_tls_config.h"

namespace {
// 4000 bytes each - matches NVS's own per-string limit (see
// device_config.h), so any PEM DeviceConfig could possibly have stored
// fits here too. static (not stack-local to apply()): see this file's
// header comment on why these must outlive the function call.
char g_ca_cert[4000];
char g_client_cert[4000];
char g_client_key[4000];
} // namespace

bool MqttTlsConfig::apply(DeviceConfig &config, NetworkClientSecure &client) {
    if (!config.hasTlsMaterial()) return false;

    if (!config.getCaCert(g_ca_cert, sizeof(g_ca_cert))) return false;
    if (!config.getClientCert(g_client_cert, sizeof(g_client_cert))) return false;
    if (!config.getClientKey(g_client_key, sizeof(g_client_key))) return false;

    client.setCACert(g_ca_cert);
    client.setCertificate(g_client_cert);
    client.setPrivateKey(g_client_key);
    return true;
}
