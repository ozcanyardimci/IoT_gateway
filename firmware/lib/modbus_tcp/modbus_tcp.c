#include "modbus_tcp.h"
#include <string.h>

#define MODBUS_TCP_PROTOCOL_ID 0x0000

int modbus_tcp_wrap(uint8_t *out, size_t out_size, uint16_t transaction_id,
                     uint8_t unit_id, const uint8_t *pdu, size_t pdu_len) {
    if (out == NULL || pdu == NULL) return MODBUS_ERR_INVALID_ARG;
    size_t needed = 7 + pdu_len;
    if (out_size < needed) return MODBUS_ERR_BUFFER_TOO_SMALL;
    uint16_t length_field = (uint16_t)(1 + pdu_len); // unit_id + pdu
    out[0] = (uint8_t)(transaction_id >> 8);
    out[1] = (uint8_t)(transaction_id & 0xFF);
    out[2] = (uint8_t)(MODBUS_TCP_PROTOCOL_ID >> 8);
    out[3] = (uint8_t)(MODBUS_TCP_PROTOCOL_ID & 0xFF);
    out[4] = (uint8_t)(length_field >> 8);
    out[5] = (uint8_t)(length_field & 0xFF);
    out[6] = unit_id;
    memcpy(&out[7], pdu, pdu_len);
    return (int)needed;
}

int modbus_tcp_unwrap(const uint8_t *frame, size_t frame_len, uint16_t expected_transaction_id,
                       uint8_t *pdu_out, size_t pdu_out_size) {
    if (frame == NULL || pdu_out == NULL) return MODBUS_ERR_INVALID_ARG;
    if (frame_len < 8) return MODBUS_ERR_RESPONSE_TOO_SHORT; // 7-byte header + >=1 PDU byte
    uint16_t transaction_id = ((uint16_t)frame[0] << 8) | frame[1];
    uint16_t protocol_id    = ((uint16_t)frame[2] << 8) | frame[3];
    uint16_t length_field   = ((uint16_t)frame[4] << 8) | frame[5];
    if (transaction_id != expected_transaction_id) return MODBUS_ERR_TRANSACTION_MISMATCH;
    if (protocol_id != MODBUS_TCP_PROTOCOL_ID) return MODBUS_ERR_PROTOCOL_ID_MISMATCH;
    if ((size_t)length_field != frame_len - 6) return MODBUS_ERR_LENGTH_MISMATCH;
    size_t pdu_len = frame_len - 7;
    if (pdu_out_size < pdu_len) return MODBUS_ERR_BUFFER_TOO_SMALL;
    memcpy(pdu_out, &frame[7], pdu_len);
    return (int)pdu_len;
}
