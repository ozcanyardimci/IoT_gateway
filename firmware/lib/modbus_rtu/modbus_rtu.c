#include "modbus_rtu.h"
#include "modbus_crc.h"
#include <string.h>

int modbus_rtu_wrap(uint8_t *out, size_t out_size, uint8_t slave_addr,
                     const uint8_t *pdu, size_t pdu_len) {
    if (out == NULL || pdu == NULL) return MODBUS_ERR_INVALID_ARG;
    size_t needed = 1 + pdu_len + 2;
    if (out_size < needed) return MODBUS_ERR_BUFFER_TOO_SMALL;
    out[0] = slave_addr;
    memcpy(&out[1], pdu, pdu_len);
    uint16_t crc = modbus_crc16(out, 1 + pdu_len);
    out[1 + pdu_len]     = (uint8_t)(crc & 0xFF); // low byte first on the wire
    out[1 + pdu_len + 1] = (uint8_t)(crc >> 8);
    return (int)needed;
}

int modbus_rtu_unwrap(const uint8_t *frame, size_t frame_len, uint8_t expected_addr,
                       uint8_t *pdu_out, size_t pdu_out_size) {
    if (frame == NULL || pdu_out == NULL) return MODBUS_ERR_INVALID_ARG;
    if (frame_len < 4) return MODBUS_ERR_RESPONSE_TOO_SHORT; // addr + fc + crc(2)
    size_t pdu_len = frame_len - 3;
    uint16_t received_crc = (uint16_t)frame[frame_len - 2] | ((uint16_t)frame[frame_len - 1] << 8);
    uint16_t computed_crc = modbus_crc16(frame, frame_len - 2);
    if (received_crc != computed_crc) return MODBUS_ERR_CRC_MISMATCH;
    if (frame[0] != expected_addr) return MODBUS_ERR_ADDRESS_MISMATCH;
    if (pdu_out_size < pdu_len) return MODBUS_ERR_BUFFER_TOO_SMALL;
    memcpy(pdu_out, &frame[1], pdu_len);
    return (int)pdu_len;
}
