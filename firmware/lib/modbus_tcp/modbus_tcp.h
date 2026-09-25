#pragma once
#include <stdint.h>
#include <stddef.h>
#include "modbus_pdu.h"

#ifdef __cplusplus
extern "C" {
#endif

// Wraps a PDU into a full Modbus TCP frame (MBAP header + PDU):
// [transaction_id:2][protocol_id:2 = 0x0000][length:2][unit_id:1][pdu...]
// `length` = unit_id (1 byte) + pdu_len, per the Modbus TCP spec. Returns
// bytes written, or a negative modbus_status_t.
int modbus_tcp_wrap(uint8_t *out, size_t out_size, uint16_t transaction_id,
                     uint8_t unit_id, const uint8_t *pdu, size_t pdu_len);

// Validates a received MBAP header - transaction id echo, protocol id == 0,
// length field matches the actual frame - and extracts the PDU. Returns the
// PDU length, or a negative modbus_status_t.
int modbus_tcp_unwrap(const uint8_t *frame, size_t frame_len, uint16_t expected_transaction_id,
                       uint8_t *pdu_out, size_t pdu_out_size);

#ifdef __cplusplus
}
#endif
