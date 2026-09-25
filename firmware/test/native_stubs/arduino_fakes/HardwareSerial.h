#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>

// Fake HardwareSerial for modbus_master tests.
#define SERIAL_8N1 0x800001c

class HardwareSerial {
public:
    HardwareSerial(int uart_nr) : uart_nr_(uart_nr) {}

    // Rs485Serial::begin() calls this 4-arg overload on real hardware
    // (config byte, then rx/tx pins) - faked here as a no-op recorder
    // since this test never inspects UART configuration, only TX/RX
    // byte content via the fake_* instrumentation below.
    void begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin) {
        (void)config;
        begin_baud_ = baud;
        begin_rx_pin_ = rxPin;
        begin_tx_pin_ = txPin;
        begin_called_ = true;
    }

    // Methods called by ModbusMasterTask via Rs485Serial
    int available() {
        return rx_buffer_.size() - rx_pos_;
    }

    int read() {
        if (rx_pos_ >= rx_buffer_.size()) return -1;
        return rx_buffer_[rx_pos_++];
    }

    size_t write(const uint8_t *buffer, size_t size) {
        for (size_t i = 0; i < size; i++) {
            tx_buffer_.push_back(buffer[i]);
        }
        return size;
    }

    size_t write(uint8_t c) {
        tx_buffer_.push_back(c);
        return 1;
    }

    void flush() {
        // No-op for the fake, just marks that it was called.
        flush_called_ = true;
    }

    // --- Fake instrumentation for tests ---
    void fake_setRxBytes(const uint8_t *data, size_t len) {
        rx_buffer_.assign(data, data + len);
        rx_pos_ = 0;
    }

    void fake_clearRx() {
        rx_buffer_.clear();
        rx_pos_ = 0;
    }

    std::vector<uint8_t> fake_getTxBytes() const {
        return tx_buffer_;
    }

    void fake_clearTx() {
        tx_buffer_.clear();
    }

    bool fake_wasFlushCalled() const {
        return flush_called_;
    }

    void fake_reset() {
        fake_clearRx();
        fake_clearTx();
        flush_called_ = false;
    }

private:
    int uart_nr_;
    unsigned long begin_baud_ = 0;
    int8_t begin_rx_pin_ = -1;
    int8_t begin_tx_pin_ = -1;
    bool begin_called_ = false;
    std::vector<uint8_t> rx_buffer_;
    size_t rx_pos_ = 0;
    std::vector<uint8_t> tx_buffer_;
    bool flush_called_ = false;
};

// Global instance 1, matching the real ESP32 Arduino core
extern HardwareSerial Serial1;
