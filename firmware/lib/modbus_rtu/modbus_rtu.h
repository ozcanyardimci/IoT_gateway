#pragma once
#include <stdint.h>
#include <stddef.h>
#include "modbus_pdu.h"

#ifdef __cplusplus
extern "C" {
#endif

// Wraps a PDU (from modbus_pdu_build_*) into a full RTU frame:
// [slave_addr][pdu...][crc_lo][crc_hi]. Returns bytes written, or a negative
// modbus_status_t.
int modbus_rtu_wrap(uint8_t *out, size_t out_size, uint8_t slave_addr,
                     const uint8_t *pdu, size_t pdu_len);

// Validates a received RTU frame (CRC, then slave address) and extracts the
// PDU into pdu_out. Returns the PDU length, or a negative modbus_status_t.
// An exception response is NOT detected here - it's still a structurally
// valid frame; call modbus_pdu_parse_* on the extracted PDU to find out.
int modbus_rtu_unwrap(const uint8_t *frame, size_t frame_len, uint8_t expected_addr,
                       uint8_t *pdu_out, size_t pdu_out_size);

#ifdef __cplusplus
}
#endif
