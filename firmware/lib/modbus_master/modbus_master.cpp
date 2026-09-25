#include "modbus_master.h"
#include "modbus_rtu.h"
#include "system_watchdog.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// Max RTU frame this class exchanges: slave_addr(1) + fc(1) + byte_count(1)
// + up to MODBUS_MASTER_MAX_REGISTERS*2 data bytes + crc(2). Sized from
// MODBUS_MASTER_MAX_REGISTERS, not the protocol's full 125-register/256-byte
// ceiling - this class never asks for more than MODBUS_MASTER_MAX_REGISTERS
// per target (addTarget() bounds-checks that), so a response can't come
// back bigger than this either.
#define MODBUS_MASTER_MAX_FRAME (5 + MODBUS_MASTER_MAX_REGISTERS * 2)

bool ModbusMasterTask::begin(Rs485Serial &bus, unsigned long baud,
                              unsigned long response_timeout_ms) {
    bus_ = &bus;
    baud_ = baud;
    response_timeout_ms_ = response_timeout_ms;
    gap_us_ = modbusRtuInterFrameGapUs(baud);
    bus_->begin(baud);

    SemaphoreHandle_t sem = xSemaphoreCreateMutex();
    if (sem == nullptr) return false;
    mutex_ = sem;

    for (int i = 0; i < MODBUS_MASTER_MAX_TARGETS; i++) {
        readings_[i].valid = false;
        readings_[i].last_poll_ok = false;
        readings_[i].count = 0;
    }
    return true;
}

int ModbusMasterTask::addTarget(const ModbusMasterTarget &target) {
    // BUG FIX (2026-09-24): this used to check `mutex_ != nullptr`, but
    // begin() (which MUST run before addTarget() per this class's own
    // documented usage - start() itself requires bus_/mutex_ already
    // set from begin()) creates the mutex too, so that check rejected
    // every single addTarget() call, including the legitimate
    // before-start case - confirmed by a native compile+run of
    // test_modbus_master, which failed test_modbus_target_add_success
    // (a pre-start call, expected to succeed) until this was fixed.
    // started_ is the actual signal this check needs.
    if (started_) return -1; // already started
    if (target_count_ >= MODBUS_MASTER_MAX_TARGETS) return -1;
    if (target.quantity == 0 || target.quantity > MODBUS_MASTER_MAX_REGISTERS) return -1;

    int index = target_count_;
    targets_[index] = target;
    last_polled_millis_[index] = 0;
    target_count_++;
    return index;
}

bool ModbusMasterTask::start(SystemWatchdog *watchdog, int core_id, int priority) {
    if (bus_ == nullptr || mutex_ == nullptr) return false; // begin() not called
    watchdog_ = watchdog;

    BaseType_t ok = xTaskCreatePinnedToCore(
        &ModbusMasterTask::taskEntry,
        "modbus_master",
        4096, // stack size in BYTES, not words - confirmed against
              // ESP-IDF's own xTaskCreatePinnedToCore doc comment
              // (idf_additions.h: "differs from vanilla FreeRTOS",
              // which does use words). Modbus PDU building/parsing +
              // a MODBUS_MASTER_MAX_FRAME-sized local buffer are the
              // biggest users, well within 4KB - revisit if
              // MODBUS_MASTER_MAX_REGISTERS is raised a lot
        this,
        (UBaseType_t)priority,
        nullptr,
        (BaseType_t)core_id);
    if (ok == pdPASS) {
        started_ = true;
    }
    return ok == pdPASS;
}

void ModbusMasterTask::taskEntry(void *arg) {
    static_cast<ModbusMasterTask *>(arg)->runLoop();
    // runLoop() never returns in normal operation; if it ever does,
    // don't fall off the end of a FreeRTOS task function (undefined
    // behavior) - delete self instead.
    vTaskDelete(nullptr);
}

void ModbusMasterTask::runLoop() {
    if (watchdog_ != nullptr) {
        watchdog_->addCurrentTask();
    }

    for (;;) {
        unsigned long now = millis();
        if (target_count_ == 0) {
            // Nothing configured yet - idle rather than busy-loop.
            // Still feeds the watchdog every pass so an empty target
            // list can't itself look like a hang.
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            for (int i = 0; i < target_count_; i++) {
                unsigned long due = last_polled_millis_[i] + targets_[i].poll_interval_ms;
                // now - due, not now >= due: correct across millis()
                // rollover the same way this project's other backoff/
                // timing code already reasons about it (e.g.
                // mqtt_client_wrapper's reconnect backoff).
                if (last_polled_millis_[i] == 0 || (long)(now - due) >= 0) {
                    pollOneTarget(i);
                    last_polled_millis_[i] = millis();
                }
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        if (watchdog_ != nullptr) {
            watchdog_->feed();
        }
    }
}

bool ModbusMasterTask::pollOneTarget(int index) {
    const ModbusMasterTarget &t = targets_[index];

    uint8_t pdu[8];
    int pdu_len = modbus_pdu_build_read_holding_registers(pdu, sizeof(pdu), t.start_addr, t.quantity);

    bool ok = false;
    ModbusMasterReading result;
    result.valid = true; // this attempt counts, whatever the outcome
    result.last_poll_millis = millis();
    result.count = 0;

    if (pdu_len > 0) {
        uint8_t frame[MODBUS_MASTER_MAX_FRAME];
        int frame_len = modbus_rtu_wrap(frame, sizeof(frame), t.slave_addr, pdu, (size_t)pdu_len);

        if (frame_len > 0) {
            // Drain anything stray left over from a previous exchange
            // (e.g. a late byte after the last poll's own timeout) so
            // it can't be mistaken for the start of this response.
            while (bus_->port().available()) {
                bus_->port().read();
            }

            bus_->port().write(frame, (size_t)frame_len);
            bus_->port().flush(); // block until TX actually clears the
                                   // UART - safe here, this task owns
                                   // the bus and has nothing else to do
                                   // until the response arrives anyway

            uint8_t response[MODBUS_MASTER_MAX_FRAME];
            size_t response_len = 0;
            unsigned long wait_start = millis();
            unsigned long last_byte_us = 0;
            bool got_any_byte = false;

            for (;;) {
                if (bus_->port().available()) {
                    if (response_len < sizeof(response)) {
                        response[response_len++] = (uint8_t)bus_->port().read();
                    } else {
                        bus_->port().read(); // discard overflow, still
                                              // track the gap below so
                                              // an oversized/garbled
                                              // response doesn't wedge
                                              // this loop
                    }
                    last_byte_us = micros();
                    got_any_byte = true;
                } else if (!got_any_byte) {
                    if (millis() - wait_start >= response_timeout_ms_) {
                        break; // no response at all within the budget
                    }
                } else {
                    if ((unsigned long)(micros() - last_byte_us) >= gap_us_) {
                        break; // t3.5 silence -> frame complete
                    }
                    if (millis() - wait_start >= response_timeout_ms_) {
                        break; // best-effort cap even if the gap is
                               // never cleanly seen (e.g. line noise)
                    }
                }
            }

            if (response_len > 0) {
                uint8_t out_pdu[MODBUS_MASTER_MAX_FRAME];
                int out_pdu_len = modbus_rtu_unwrap(response, response_len, t.slave_addr,
                                                      out_pdu, sizeof(out_pdu));
                if (out_pdu_len > 0) {
                    modbus_registers_t regs;
                    uint8_t exception_code = 0;
                    int parse_status = modbus_pdu_parse_read_registers_response(
                        out_pdu, (size_t)out_pdu_len, MODBUS_FC_READ_HOLDING_REGISTERS,
                        &regs, &exception_code);
                    if (parse_status == MODBUS_OK) {
                        uint16_t n = regs.count;
                        if (n > MODBUS_MASTER_MAX_REGISTERS) n = MODBUS_MASTER_MAX_REGISTERS;
                        for (uint16_t i = 0; i < n; i++) {
                            result.values[i] = regs.values[i];
                        }
                        result.count = n;
                        ok = true;
                    }
                }
            }
        }
    }

    result.last_poll_ok = ok;

    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(mutex_);
    if (xSemaphoreTake(sem, pdMS_TO_TICKS(50)) == pdTRUE) {
        readings_[index] = result;
        xSemaphoreGive(sem);
    }
    // A missed mutex (another reader holding it >50ms) just means this
    // one poll's result isn't published - the next cycle tries again.
    // Not treated as a fatal error.

    return ok;
}

bool ModbusMasterTask::getReading(int index, ModbusMasterReading *out) const {
    if (index < 0 || index >= target_count_ || out == nullptr) return false;

    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(mutex_);
    if (xSemaphoreTake(sem, pdMS_TO_TICKS(50)) != pdTRUE) return false;
    *out = readings_[index];
    xSemaphoreGive(sem);
    return true;
}
