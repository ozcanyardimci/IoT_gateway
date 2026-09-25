#include "wireguard_link.h"

bool WireguardLink::begin(const Config &config) {
    wg_config_ = ESP_WIREGUARD_CONFIG_DEFAULT();
    wg_config_.private_key = config.private_key;
    wg_config_.public_key = config.public_key;
    wg_config_.preshared_key = config.preshared_key;
    wg_config_.address = config.local_address;
    wg_config_.netmask = config.local_netmask;
    wg_config_.endpoint = config.endpoint;
    wg_config_.port = config.endpoint_port ? config.endpoint_port : 51820;
    wg_config_.persistent_keepalive = config.persistent_keepalive_sec;

    esp_err_t err = esp_wireguard_init(&wg_config_, &wg_ctx_);
    initialized_ = (err == ESP_OK);
    return initialized_;
}

bool WireguardLink::connect() {
    if (!initialized_) return false;
    return esp_wireguard_connect(&wg_ctx_) == ESP_OK;
}

bool WireguardLink::isPeerUp() const {
    if (!initialized_) return false;
    return esp_wireguard_peer_is_up(&wg_ctx_) == ESP_OK;
}

void WireguardLink::disconnect() {
    if (!initialized_) return;
    esp_wireguard_disconnect(&wg_ctx_);
}
