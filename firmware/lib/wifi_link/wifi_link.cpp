#include "wifi_link.h"
#include <WiFi.h>

void WifiLink::begin(const char *ssid, const char *password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
}

bool WifiLink::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

void WifiLink::disconnect() {
    WiFi.disconnect(true);
}
