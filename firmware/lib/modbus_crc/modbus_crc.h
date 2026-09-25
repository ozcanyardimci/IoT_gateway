#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Modbus RTU CRC16 (poly 0xA001, init 0xFFFF, no xorout).
// Returns the 16-bit CRC register value; low byte is transmitted first on
// the wire, high byte second (i.e. append (crc & 0xFF) then (crc >> 8)).
uint16_t modbus_crc16(const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif
