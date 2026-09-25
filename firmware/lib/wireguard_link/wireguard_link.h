#pragma once

// Wraps esphome/wireguard (https://github.com/esphome-libs/wireguard) -
// a real, existing WireGuard library, formerly published as
// droscy/esp_wireguard (same repo and API, renamed - see its own
// README) - NOT hand-rolled crypto. This is deliberate: WireGuard's
// handshake and transport both depend on correctly-implemented
// Curve25519/ChaCha20-Poly1305/BLAKE2s, and a home-written version of
// any of that would be a real security risk, not just a bug. Use the
// real library.
//
// Earlier revisions of this comment assumed this was an ESP-IDF-only
// component needing manual vendoring under a project-level
// `components/` directory plus a `framework = arduino, espidf` combo
// build - that assumption was wrong. Checked directly against the
// library's own library.json: it declares real PlatformIO support for
// `framework = arduino` (build.includeDir/srcDir - PlatformIO's own
// library builder, not the idf_component_register() path in its
// CMakeLists.txt, which only runs under ESP-IDF's own build system,
// i.e. `framework = espidf`). platformio.ini's lib_deps now pulls it
// directly. libsodium is deliberately NOT pinned separately there -
// wireguard's own library.json already declares a libsodium dependency,
// which PlatformIO resolves automatically; pinning it a second time via
// an explicit git URL caused PlatformIO to install two separate copies
// of it, which collided during the SCons build (both copies' port/
// source folder mapped to the same build directory). This is the real,
// confirmed integration shape, not an unverified guess. What's still
// genuinely unverified is
// an actual `pio run` of this env - only the source and its
// PlatformIO manifest were inspected, not a real build (network access
// to run pio itself is blocked in this environment - see CLAUDE.md).
// Budget real time for a first build/link pass when you get to it in
// VS Code, same as any other module - but no build-integration
// redesign is expected to be needed here.
#include <esp_wireguard.h>

class WireguardLink {
public:
    // All fields point to strings owned by the caller (must outlive this
    // object) - config storage/lifetime is the caller's responsibility
    // (see F3's "secure credential storage" item; keys should not live in
    // firmware source).
    struct Config {
        const char *private_key;   // base64, from `wg genkey`
        const char *public_key;    // peer's base64 public key
        const char *preshared_key; // optional, nullptr if unused
        const char *local_address; // this device's tunnel IP
        const char *local_netmask;
        const char *endpoint;      // peer hostname/IP
        uint16_t endpoint_port;    // default WireGuard port is 51820
        uint16_t persistent_keepalive_sec; // 0 = disabled
    };

    bool begin(const Config &config);
    bool connect();
    bool isPeerUp() const;
    void disconnect();

private:
    wireguard_config_t wg_config_{};
    wireguard_ctx_t wg_ctx_{};
    bool initialized_ = false;
};
