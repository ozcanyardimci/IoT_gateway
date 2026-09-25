#pragma once
#include "device_config.h"
#include <NetworkClientSecure.h>

// Loads TLS material from DeviceConfig into a NetworkClientSecure so it
// can be handed to MqttClientWrapper's constructor (which takes a
// Client& - NetworkClientSecure -> NetworkClient -> ESPLwIPClient ->
// Client satisfies that, confirmed directly against arduino-esp32's own
// inheritance chain, not assumed).
//
// Class name note: this wraps NetworkClientSecure, NOT the older
// WiFiClientSecure name used in a lot of ESP32/Arduino TLS tutorials.
// arduino-esp32 core 3.x renamed WiFiClientSecure -> NetworkClientSecure
// (checked directly: WiFiClientSecure/src/WiFiClientSecure.h exists on
// the 2.0.x tag but 404s on every 3.x tag and on master; the current
// class lives at libraries/NetworkClientSecure/src/NetworkClientSecure.h
// instead - no compatibility alias/typedef was found for the old name).
// This project's other core-3.x-specific code (ethernet_link's
// Network.onEvent()) already assumes the 3.x core, so this follows the
// same assumption - if this project's pinned "stable" pioarduino
// platform build ever resolves to a 2.x core instead, swap this for
// WiFiClientSecure (same method names/shape otherwise).
class MqttTlsConfig {
public:
    // Loads CA cert / client cert / client key from config into client.
    // Returns false (leaving client's TLS material untouched) if
    // DeviceConfig has no TLS material stored, or a PEM read comes back
    // empty. The PEM text is held in static buffers owned by this
    // function, not copied by NetworkClientSecure itself (confirmed
    // against its actual implementation: setCACert()/setCertificate()/
    // setPrivateKey() just store the pointer you pass them - they do
    // NOT strdup/copy it) - so those buffers must, and do, outlive the
    // call. Only call apply() once per boot per client; a second call
    // overwrites the same static buffers, which is fine as long as
    // nothing is still relying on the previous client's TLS state.
    bool apply(DeviceConfig &config, NetworkClientSecure &client);
};
