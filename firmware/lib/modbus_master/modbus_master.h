#pragma once
#include <stdint.h>
#include <stddef.h>
#include "rs485_serial.h"
#include "modbus_pdu.h"

// F4's Modbus master polling engine (docs/roadmap.md Firmware roadmap,
// F4 "task scheduler...tying every module above together") - runs on
// its OWN dedicated FreeRTOS task (see start() below), not in the main
// loop(). Why: Modbus RTU frame boundaries are detected by a silent
// inter-byte gap (see modbusRtuInterFrameGapUs() below, ~4ms at this
// bus's default 9600 baud - Modbus_over_serial_line_V1_02, section
// 2.5.1.1) - if that timing gets delayed by something else running in
// the same call stack (an MQTT TLS handshake via mqtt_tls_config, an OTA
// flash write via ota_update - both genuinely block for hundreds of ms
// to a few seconds, confirmed against their real implementations, not
// assumed), frame boundaries can't be told apart reliably anymore. A
// separate task means FreeRTOS's own preemptive scheduler protects this
// timing regardless of what the main loop is doing, instead of hoping
// nothing else in a single cooperative loop ever blocks too long.
//
// Transport scope, deliberately narrow for this first pass: RS485 RTU
// only (Rs485Serial - the bus this project's docs describe downstream
// field devices connecting to). Modbus TCP master (polling IP-based
// slaves) is NOT included here - it would need its own
// WiFiClient/EthernetClient-based transport that doesn't exist yet in
// this codebase, and is a real separate piece of work, not an oversight.
// modbus_tcp.h's framing functions are unused by this class for that
// reason; a future ModbusMasterTcp class alongside this one is the
// natural extension point when that's needed.
//
// Function-code scope, also deliberately narrow: read holding registers
// only (MODBUS_FC_READ_HOLDING_REGISTERS) - the most common polling
// case for field devices (meters, sensors, PLC-exposed registers).
// Coils/discrete-inputs/writes are real modbus_pdu.h capabilities this
// class doesn't use yet, not capabilities that don't exist.
//
// Target list starts EMPTY. Nothing in this project's docs defines what
// downstream field devices this gateway actually polls (no register
// map, no device count, no slave addresses anywhere in
// docs/subsystems/) - inventing one here would be a guess, not an
// engineering decision. addTarget() is the real, working mechanism;
// what goes into it is a decision for whoever configures this gateway
// against its actual field devices, not something to hardcode.

// Pure, hardware-independent - gcc-testable the same way as
// ota_update.h's otaProgressPercent (see
// firmware/test/test_modbus_master). Modbus_over_serial_line_V1_02
// section 2.5.1.1: t3.5 (inter-frame silence, one character = 11 bits -
// 1 start + 8 data + 1 parity/stop + 1 stop) for baud <= 19200; a fixed
// 1750us for baud > 19200 (the character-time formula would otherwise
// give an unrealistically short gap at high baud rates - that's a
// stated exception in the spec, not this project's own guess).
inline unsigned long modbusRtuInterFrameGapUs(unsigned long baud) {
    if (baud == 0) return 1750; // avoid div-by-zero; fall back to the
                                 // high-baud fixed value, the safer
                                 // (larger relative to char time) of the
                                 // two constants this function returns
    if (baud > 19200) return 1750;
    // 3.5 chars * 11 bits/char * 1,000,000 us/s / baud, integer math
    // ordered to keep intermediate values in range for unsigned long.
    return (3500UL * 11UL * 1000UL) / baud;
}

#define MODBUS_MASTER_MAX_TARGETS 8
#define MODBUS_MASTER_MAX_REGISTERS 32 // per target - smaller than the
                                        // protocol's 125-register max
                                        // (modbus_pdu.h), plenty for a
                                        // typical field-device polling
                                        // block; raise if a real target
                                        // needs more

struct ModbusMasterTarget {
    uint8_t slave_addr;
    uint16_t start_addr;
    uint16_t quantity;         // 1..MODBUS_MASTER_MAX_REGISTERS
    unsigned long poll_interval_ms;
};

struct ModbusMasterReading {
    bool valid;                // true once at least one poll has completed
                                // (success or failure) - false only
                                // before the very first attempt
    bool last_poll_ok;
    unsigned long last_poll_millis;
    uint16_t values[MODBUS_MASTER_MAX_REGISTERS];
    uint16_t count;
};

class ModbusMasterTask {
public:
    // response_timeout_ms: how long to wait for a slave to start
    // responding at all before giving up on that poll (does not need to
    // be baud-derived like the inter-frame gap - this covers the
    // slave's own processing time, which varies by device).
    bool begin(Rs485Serial &bus, unsigned long baud = 9600,
               unsigned long response_timeout_ms = 500);

    // Bounds-checked like this project's other table-based modules
    // (net_allowlist.h's add(), etc.). Returns the added target's index,
    // or -1 if the table is full or quantity is out of range.
    int addTarget(const ModbusMasterTarget &target);

    int targetCount() const { return target_count_; }

    // Starts the dedicated FreeRTOS task that polls every configured
    // target in round-robin order, each at its own poll_interval_ms.
    // Safe to call with zero targets configured (the task simply idles,
    // feeding the watchdog each cycle) - starting the mechanism doesn't
    // require targets to already exist.
    //
    // watchdog may be nullptr (no watchdog registration - e.g. for
    // gcc/host testing setups that never reach this call anyway, since
    // it's hardware-only). core_id follows this project's F4 core-
    // pinning choice (see docs/roadmap.md's F4 entry): 0, opposite
    // Arduino's default loopTask core 1, so this task's scheduling
    // doesn't contend with the WiFi/BT stack's own internal core-0
    // tasks. Note this is a secondary refinement, not the thing that
    // actually protects Modbus timing - being a SEPARATE task is what
    // does that (FreeRTOS preempts by priority/time-slice regardless of
    // core); core placement just avoids extra contention on top of that.
    // priority is a plain int here (not FreeRTOS's UBaseType_t) so this
    // header doesn't need freertos/FreeRTOS.h just to declare start() -
    // cast to UBaseType_t happens in the .cpp, which already needs the
    // real FreeRTOS headers for xTaskCreatePinnedToCore itself.
    bool start(class SystemWatchdog *watchdog = nullptr, int core_id = 0,
               int priority = 2);

    // Mutex-protected copy-out for the main loop to read the latest
    // reading for a target (e.g. to fold into an MQTT publish or a
    // future rule-engine extension). Returns false (out left untouched)
    // if index is out of range.
    bool getReading(int index, ModbusMasterReading *out) const;

private:
    static void taskEntry(void *arg);
    void runLoop();
    bool pollOneTarget(int index);

    Rs485Serial *bus_ = nullptr;
    unsigned long baud_ = 9600;
    unsigned long response_timeout_ms_ = 500;
    unsigned long gap_us_ = 0;

    ModbusMasterTarget targets_[MODBUS_MASTER_MAX_TARGETS];
    ModbusMasterReading readings_[MODBUS_MASTER_MAX_TARGETS];
    unsigned long last_polled_millis_[MODBUS_MASTER_MAX_TARGETS] = {};
    int target_count_ = 0;
    bool started_ = false; // set true only by a successful start() -
                            // addTarget() must reject based on THIS,
                            // not on mutex_, which begin() (called
                            // BEFORE addTarget() in this class's own
                            // documented usage) already sets

    void *mutex_ = nullptr; // SemaphoreHandle_t, opaque here so this
                             // header doesn't need to pull in
                             // freertos/semphr.h for callers that only
                             // touch the public API
    class SystemWatchdog *watchdog_ = nullptr;
};
