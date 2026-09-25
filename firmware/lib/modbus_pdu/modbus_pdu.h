#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Shared status codes across the whole modbus_* module family
// (modbus_pdu / modbus_rtu / modbus_tcp).
typedef enum {
    MODBUS_OK = 0,
    MODBUS_ERR_BUFFER_TOO_SMALL      = -1,
    MODBUS_ERR_INVALID_ARG           = -2,
    MODBUS_ERR_RESPONSE_TOO_SHORT    = -3,
    MODBUS_ERR_FUNCTION_MISMATCH     = -4,
    MODBUS_ERR_EXCEPTION             = -5,  // slave returned an exception response
    MODBUS_ERR_BYTE_COUNT_MISMATCH   = -6,
    MODBUS_ERR_CRC_MISMATCH          = -7,  // RTU only
    MODBUS_ERR_ADDRESS_MISMATCH      = -8,  // RTU only
    MODBUS_ERR_PROTOCOL_ID_MISMATCH  = -9,  // TCP only
    MODBUS_ERR_LENGTH_MISMATCH       = -10, // TCP only
    MODBUS_ERR_TRANSACTION_MISMATCH  = -11, // TCP only
} modbus_status_t;

// This gateway's role is Modbus master: it builds requests and parses
// responses from downstream field devices. Slave-side (responding to
// requests) is not implemented - out of scope, see docs/roadmap.md.
#define MODBUS_FC_READ_COILS               0x01
#define MODBUS_FC_READ_DISCRETE_INPUTS     0x02
#define MODBUS_FC_READ_HOLDING_REGISTERS   0x03
#define MODBUS_FC_READ_INPUT_REGISTERS     0x04
#define MODBUS_FC_WRITE_SINGLE_COIL        0x05
#define MODBUS_FC_WRITE_SINGLE_REGISTER    0x06
#define MODBUS_FC_WRITE_MULTIPLE_REGISTERS 0x10

// Builds a PDU (function code + data - no slave address, no CRC, no MBAP
// header; those are added by modbus_rtu.h / modbus_tcp.h) into `out`.
// Returns bytes written, or a negative modbus_status_t on error.
int modbus_pdu_build_read_coils(uint8_t *out, size_t out_size, uint16_t start_addr, uint16_t quantity);
int modbus_pdu_build_read_discrete_inputs(uint8_t *out, size_t out_size, uint16_t start_addr, uint16_t quantity);
int modbus_pdu_build_read_holding_registers(uint8_t *out, size_t out_size, uint16_t start_addr, uint16_t quantity);
int modbus_pdu_build_read_input_registers(uint8_t *out, size_t out_size, uint16_t start_addr, uint16_t quantity);
int modbus_pdu_build_write_single_coil(uint8_t *out, size_t out_size, uint16_t addr, int value_on);
int modbus_pdu_build_write_single_register(uint8_t *out, size_t out_size, uint16_t addr, uint16_t value);
int modbus_pdu_build_write_multiple_registers(uint8_t *out, size_t out_size, uint16_t start_addr,
                                               const uint16_t *values, uint16_t count);

// Max registers in a single read-holding/input-registers response, per the
// Modbus Application Protocol spec's 125-register request limit.
#define MODBUS_MAX_REGISTERS 125

typedef struct {
    uint16_t values[MODBUS_MAX_REGISTERS];
    uint16_t count;
} modbus_registers_t;

// Parses a read-holding/input-registers response PDU. `expected_fc` must be
// the function code that was requested. On MODBUS_ERR_EXCEPTION,
// *exception_code holds the slave's exception code.
int modbus_pdu_parse_read_registers_response(const uint8_t *pdu, size_t pdu_len,
                                              uint8_t expected_fc,
                                              modbus_registers_t *out,
                                              uint8_t *exception_code);

// Packed-bit buffer for read-coils/read-discrete-inputs responses. Sized for
// this project's own I/O counts plus headroom for downstream field devices,
// not the full 2000-bit spec maximum - resize if a larger device is added.
#define MODBUS_MAX_BIT_BYTES 32

typedef struct {
    uint8_t bits[MODBUS_MAX_BIT_BYTES]; // packed, LSB of bits[0] = first coil/input
    uint16_t byte_count;                // caller must track true requested bit
                                         // quantity separately; the PDU does not
                                         // carry it, only the padded byte count
} modbus_bits_t;

int modbus_pdu_parse_read_bits_response(const uint8_t *pdu, size_t pdu_len,
                                         uint8_t expected_fc,
                                         modbus_bits_t *out,
                                         uint8_t *exception_code);

// Validates a write-response echo (single coil, single register, or
// multiple-registers ack). Does not compare echoed values against what was
// sent - callers that need that should compare the raw PDU bytes themselves.
int modbus_pdu_parse_write_response(const uint8_t *pdu, size_t pdu_len,
                                     uint8_t expected_fc,
                                     uint8_t *exception_code);

#ifdef __cplusplus
}
#endif
