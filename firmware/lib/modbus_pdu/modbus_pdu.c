#include "modbus_pdu.h"
#include <string.h>

#define MODBUS_MAX_READ_BITS_QTY   2000
#define MODBUS_MAX_READ_REGS_QTY   125
#define MODBUS_MAX_WRITE_REGS_QTY  123

static int build_read_request(uint8_t *out, size_t out_size, uint8_t fc,
                               uint16_t start_addr, uint16_t quantity,
                               uint16_t max_quantity) {
    (void)start_addr;
    if (out == NULL) return MODBUS_ERR_INVALID_ARG;
    if (quantity < 1 || quantity > max_quantity) return MODBUS_ERR_INVALID_ARG;
    if (out_size < 5) return MODBUS_ERR_BUFFER_TOO_SMALL;
    out[0] = fc;
    out[1] = (uint8_t)(start_addr >> 8);
    out[2] = (uint8_t)(start_addr & 0xFF);
    out[3] = (uint8_t)(quantity >> 8);
    out[4] = (uint8_t)(quantity & 0xFF);
    return 5;
}

int modbus_pdu_build_read_coils(uint8_t *out, size_t out_size, uint16_t start_addr, uint16_t quantity) {
    return build_read_request(out, out_size, MODBUS_FC_READ_COILS, start_addr, quantity, MODBUS_MAX_READ_BITS_QTY);
}

int modbus_pdu_build_read_discrete_inputs(uint8_t *out, size_t out_size, uint16_t start_addr, uint16_t quantity) {
    return build_read_request(out, out_size, MODBUS_FC_READ_DISCRETE_INPUTS, start_addr, quantity, MODBUS_MAX_READ_BITS_QTY);
}

int modbus_pdu_build_read_holding_registers(uint8_t *out, size_t out_size, uint16_t start_addr, uint16_t quantity) {
    return build_read_request(out, out_size, MODBUS_FC_READ_HOLDING_REGISTERS, start_addr, quantity, MODBUS_MAX_READ_REGS_QTY);
}

int modbus_pdu_build_read_input_registers(uint8_t *out, size_t out_size, uint16_t start_addr, uint16_t quantity) {
    return build_read_request(out, out_size, MODBUS_FC_READ_INPUT_REGISTERS, start_addr, quantity, MODBUS_MAX_READ_REGS_QTY);
}

int modbus_pdu_build_write_single_coil(uint8_t *out, size_t out_size, uint16_t addr, int value_on) {
    if (out == NULL) return MODBUS_ERR_INVALID_ARG;
    if (out_size < 5) return MODBUS_ERR_BUFFER_TOO_SMALL;
    uint16_t val = value_on ? 0xFF00 : 0x0000;
    out[0] = MODBUS_FC_WRITE_SINGLE_COIL;
    out[1] = (uint8_t)(addr >> 8);
    out[2] = (uint8_t)(addr & 0xFF);
    out[3] = (uint8_t)(val >> 8);
    out[4] = (uint8_t)(val & 0xFF);
    return 5;
}

int modbus_pdu_build_write_single_register(uint8_t *out, size_t out_size, uint16_t addr, uint16_t value) {
    if (out == NULL) return MODBUS_ERR_INVALID_ARG;
    if (out_size < 5) return MODBUS_ERR_BUFFER_TOO_SMALL;
    out[0] = MODBUS_FC_WRITE_SINGLE_REGISTER;
    out[1] = (uint8_t)(addr >> 8);
    out[2] = (uint8_t)(addr & 0xFF);
    out[3] = (uint8_t)(value >> 8);
    out[4] = (uint8_t)(value & 0xFF);
    return 5;
}

int modbus_pdu_build_write_multiple_registers(uint8_t *out, size_t out_size, uint16_t start_addr,
                                               const uint16_t *values, uint16_t count) {
    if (out == NULL || values == NULL) return MODBUS_ERR_INVALID_ARG;
    if (count < 1 || count > MODBUS_MAX_WRITE_REGS_QTY) return MODBUS_ERR_INVALID_ARG;
    size_t needed = 6 + (size_t)count * 2;
    if (out_size < needed) return MODBUS_ERR_BUFFER_TOO_SMALL;
    out[0] = MODBUS_FC_WRITE_MULTIPLE_REGISTERS;
    out[1] = (uint8_t)(start_addr >> 8);
    out[2] = (uint8_t)(start_addr & 0xFF);
    out[3] = (uint8_t)(count >> 8);
    out[4] = (uint8_t)(count & 0xFF);
    out[5] = (uint8_t)(count * 2);
    for (uint16_t i = 0; i < count; i++) {
        out[6 + (size_t)i * 2]     = (uint8_t)(values[i] >> 8);
        out[6 + (size_t)i * 2 + 1] = (uint8_t)(values[i] & 0xFF);
    }
    return (int)needed;
}

int modbus_pdu_parse_read_registers_response(const uint8_t *pdu, size_t pdu_len,
                                              uint8_t expected_fc,
                                              modbus_registers_t *out,
                                              uint8_t *exception_code) {
    if (pdu == NULL || out == NULL || pdu_len < 2) return MODBUS_ERR_RESPONSE_TOO_SHORT;
    if (pdu[0] == (uint8_t)(expected_fc | 0x80)) {
        if (exception_code) *exception_code = pdu[1];
        return MODBUS_ERR_EXCEPTION;
    }
    if (pdu[0] != expected_fc) return MODBUS_ERR_FUNCTION_MISMATCH;
    uint8_t byte_count = pdu[1];
    if (byte_count % 2 != 0) return MODBUS_ERR_BYTE_COUNT_MISMATCH;
    if (pdu_len < (size_t)(2 + byte_count)) return MODBUS_ERR_RESPONSE_TOO_SHORT;
    uint16_t reg_count = (uint16_t)(byte_count / 2);
    if (reg_count > MODBUS_MAX_REGISTERS) return MODBUS_ERR_BYTE_COUNT_MISMATCH;
    for (uint16_t i = 0; i < reg_count; i++) {
        out->values[i] = ((uint16_t)pdu[2 + (size_t)i * 2] << 8) | pdu[2 + (size_t)i * 2 + 1];
    }
    out->count = reg_count;
    return MODBUS_OK;
}

int modbus_pdu_parse_read_bits_response(const uint8_t *pdu, size_t pdu_len,
                                         uint8_t expected_fc,
                                         modbus_bits_t *out,
                                         uint8_t *exception_code) {
    if (pdu == NULL || out == NULL || pdu_len < 2) return MODBUS_ERR_RESPONSE_TOO_SHORT;
    if (pdu[0] == (uint8_t)(expected_fc | 0x80)) {
        if (exception_code) *exception_code = pdu[1];
        return MODBUS_ERR_EXCEPTION;
    }
    if (pdu[0] != expected_fc) return MODBUS_ERR_FUNCTION_MISMATCH;
    uint8_t byte_count = pdu[1];
    if (pdu_len < (size_t)(2 + byte_count)) return MODBUS_ERR_RESPONSE_TOO_SHORT;
    if (byte_count > MODBUS_MAX_BIT_BYTES) return MODBUS_ERR_BYTE_COUNT_MISMATCH;
    memcpy(out->bits, &pdu[2], byte_count);
    out->byte_count = byte_count;
    return MODBUS_OK;
}

int modbus_pdu_parse_write_response(const uint8_t *pdu, size_t pdu_len,
                                     uint8_t expected_fc,
                                     uint8_t *exception_code) {
    if (pdu == NULL || pdu_len < 2) return MODBUS_ERR_RESPONSE_TOO_SHORT;
    if (pdu[0] == (uint8_t)(expected_fc | 0x80)) {
        if (exception_code) *exception_code = pdu[1];
        return MODBUS_ERR_EXCEPTION;
    }
    if (pdu[0] != expected_fc) return MODBUS_ERR_FUNCTION_MISMATCH;
    if (pdu_len != 5) return MODBUS_ERR_RESPONSE_TOO_SHORT;
    return MODBUS_OK;
}
