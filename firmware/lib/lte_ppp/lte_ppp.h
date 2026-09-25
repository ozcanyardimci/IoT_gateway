#pragma once
#include <stdint.h>

// PPP data-link wrapper for the Quectel EG915U-EU LTE modem -
// arduino-esp32's built-in PPP library (PPP.h, wrapping ESP-IDF's
// esp_modem component), confirmed 2026-09-24 directly against
// arduino-esp32's own PPP.h and NetworkInterface.h source fetched from
// GitHub (not assumed). Fills the gap this project's main.cpp header
// comment used to flag plainly: LTE was previously wired for power-on
// and AT+CREG/AT+CSQ status polling only ("does NOT include PDP
// context activation...not treated as a connectivity fallback") - this
// module is the actual PPP/PDP bridge that was missing, letting LTE
// serve as a real third backhaul tier.
//
// Deliberately does NOT manage PWRKEY/RESET, even though
// PPPClass::setResetPin() exists (confirmed present in the real
// PPP.h). This project's LteModem (lib/lte_modem) already owns those
// two GPIOs, verified against the real schematic. Having two
// independent pieces of code drive the same reset line risks an
// unwanted reset mid-session (e.g. if some internal PPP recovery path
// pulsed it on its own schedule) and isn't needed - the caller
// sequences LteModem::begin()+powerOn() BEFORE this class's begin(),
// the same "power the peripheral, then talk to it" ordering any other
// UART peripheral on this board would need.
//
// UART: this modem's serial lines are this project's UART0 peripheral,
// remapped onto GPIO17 (TXD)/GPIO18 (RXD) - see lte_modem.h and
// pin_map.h. PPPClass::begin()'s uart_num parameter DEFAULTS TO 1 -
// passing that default here would silently collide with Rs485Serial,
// which owns UART1 (lib/rs485_serial/rs485_serial.h: HardwareSerial
// serial_{1}) - this module's begin() passes 0 explicitly. (UART2 is
// Rs232Serial's - lib/rs232_serial/rs232_serial.h: HardwareSerial
// serial_{2} - listed here only so the "why not 1 or 2" reasoning is
// checkable at a glance; this module doesn't touch UART2.) This board
// has no hardware flow control wired for the LTE UART (lte_modem.h's
// pin list has only TXD/RXD/PWRKEY/RESET, no RTS/CTS), so setPins()
// below passes ESP_MODEM_FLOW_CONTROL_NONE (PPPClass::setPins()'s own
// default).
//
// Model: PPP_MODEM_GENERIC. The Quectel EG915U isn't one of PPPClass's
// named profiles - ppp_modem_model_t (PPP.h, read directly) only lists
// GENERIC/SIM7600/SIM7070/SIM7000/BG96/SIM800, plus a CUSTOM slot
// gated behind CONFIG_ESP_MODEM_ADD_CUSTOM_MODULE, which this project
// hasn't set up. GENERIC targets esp_modem's standard 3GPP AT command
// set, which EG915U's own AT command manual (already sourced for
// lte_modem's earlier AT+CSQ/AT+CREG work) follows for the basic PDP-
// context/registration flow this module needs. UNTESTED against real
// hardware - flagged plainly rather than presented as verified end to
// end, the same honesty standard this project's other "not yet bench-
// tested" notes use (e.g. ethernet_link.h's own schematic-tracing
// caveat).
//
// Connection sequence matches Espressif's own official example
// (arduino-esp32 libraries/PPP/examples/PPP_Basic/PPP_Basic.ino,
// fetched and read directly): begin() starts the modem in command
// mode -> caller polls isAttached() (network registration) with its
// own bounded wait -> switchToDataMode() (CMUX - mixed command+data,
// what that example uses, not pure ESP_MODEM_MODE_DATA) -> caller
// polls isConnected() (actual IP-layer connectivity) with its own
// bounded wait. Deliberately NOT done as one internal blocking call:
// PPPClass::begin() itself already blocks briefly establishing AT sync
// with the modem, but the attach/connect waits that follow can each
// legitimately take many seconds against a real tower - this project's
// existing connectNetwork() (main.cpp) already owns that "bounded
// wait, feed the watchdog each iteration" pattern for WifiLink/
// EthernetLink, and LTE uses the exact same shape there rather than
// hiding its own un-watchdog-fed delay loop inside this class.
//
// isConnected()/isAttached() read PPPClass's own state directly
// (NetworkInterface::connected(), confirmed a real public base-class
// method, not PPP-specific) - no separate event handler needed here,
// unlike EthernetLink's manual Network.onEvent()+static-flag approach.
class LtePpp {
public:
    // Sets UART pins/APN/[SIM PIN] and starts the modem in command
    // mode. Returns false immediately if apn is null/empty, or if
    // PPPClass::begin() itself fails (e.g. no AT sync with the modem -
    // check LteModem::begin()/powerOn() ran first). sim_pin may be
    // null/empty if the SIM has no PIN lock.
    //
    // PRECONDITION (not enforced here): LteModem::begin()+powerOn()
    // must already have completed - see this class's header comment.
    bool begin(const char *apn, const char *sim_pin);

    // True once network registration has completed (esp_modem's own
    // attached() check).
    bool isAttached() const;

    // Switches from command mode to CMUX (mixed command+data) mode -
    // call once isAttached() is true, before polling isConnected().
    bool switchToDataMode();

    // True once the PPP interface has actual IP-layer connectivity -
    // this is the "is LTE usable as a backhaul right now" check.
    bool isConnected() const;

    // Signal strength via PPPClass::RSSI() - a direct passthrough, NOT
    // independently re-derived the way this project's old (now
    // deleted) lte_at_parse AT+CSQ->dBm conversion was. esp_modem's
    // RSSI() is documented, in the esp_modem component's own headers,
    // as already returning a dBm value rather than the raw AT+CSQ 0-31
    // scale - that documentation claim was NOT independently re-
    // verified against esp_modem's C source the way the old AT+CSQ
    // mapping was (that depth of verification wasn't repeated for this
    // passthrough); treat this value as approximate until checked
    // against real hardware.
    int16_t signalQualityDbm() const;
};
