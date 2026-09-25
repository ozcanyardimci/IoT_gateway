#include "ethernet_link.h"
#include <ETH.h>
#include <SPI.h>
#include "pin_map.h"

static volatile bool s_eth_got_ip = false;

static void onEthEvent(arduino_event_id_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_GOT_IP:
            s_eth_got_ip = true;
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
        case ARDUINO_EVENT_ETH_STOP:
            s_eth_got_ip = false;
            break;
        default:
            break;
    }
}

bool EthernetLink::begin() {
    // Network.onEvent() is the arduino-esp32 core 3.x unified event API
    // (matches the current Espressif example this was written against).
    // If your installed core is still on the 2.x line, this call won't
    // compile - swap it for WiFi.onEvent(onEthEvent) instead; everything
    // else here is unaffected.
    Network.onEvent(onEthEvent);
    SPI.begin(PIN_ETH_SCLK, PIN_ETH_MISO, PIN_ETH_MOSI);
    return ETH.begin(ETH_PHY_W5500, 1, PIN_ETH_CS, PIN_ETH_INT, -1, SPI);
}

bool EthernetLink::isConnected() const {
    return s_eth_got_ip;
}
