#include <unity.h>
#include <cstring>
extern "C" {
#include "modbus_pdu.h"
}

void setUp(void) {}
void tearDown(void) {}

// Reference PDUs cross-checked against an independent from-spec Python
// re-implementation, not shared code with this library - see project chat.

void test_read_holding_registers(void) {
    uint8_t buf[16];
    int n = modbus_pdu_build_read_holding_registers(buf, sizeof(buf), 0x0000, 10);
    uint8_t expect[] = {0x03,0x00,0x00,0x00,0x0A};
    TEST_ASSERT_EQUAL_INT(5, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect, buf, 5);
}

void test_write_single_coil_on(void) {
    uint8_t buf[16];
    int n = modbus_pdu_build_write_single_coil(buf, sizeof(buf), 0x00AC, 1);
    uint8_t expect[] = {0x05,0x00,0xAC,0xFF,0x00};
    TEST_ASSERT_EQUAL_INT(5, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect, buf, 5);
}

void test_build_read_coils(void) {
    uint8_t buf[16];
    int n = modbus_pdu_build_read_coils(buf, sizeof(buf), 0x0013, 0x0025);
    uint8_t expect[] = {0x01,0x00,0x13,0x00,0x25};
    TEST_ASSERT_EQUAL_INT(5, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect, buf, 5);
}

void test_build_read_discrete_inputs(void) {
    uint8_t buf[16];
    int n = modbus_pdu_build_read_discrete_inputs(buf, sizeof(buf), 0x00C4, 0x0016);
    uint8_t expect[] = {0x02,0x00,0xC4,0x00,0x16};
    TEST_ASSERT_EQUAL_INT(5, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect, buf, 5);
}

void test_build_read_input_registers(void) {
    uint8_t buf[16];
    int n = modbus_pdu_build_read_input_registers(buf, sizeof(buf), 0x0008, 0x0001);
    uint8_t expect[] = {0x04,0x00,0x08,0x00,0x01};
    TEST_ASSERT_EQUAL_INT(5, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect, buf, 5);
}

void test_build_write_single_register(void) {
    uint8_t buf[16];
    int n = modbus_pdu_build_write_single_register(buf, sizeof(buf), 0x0001, 0x0003);
    uint8_t expect[] = {0x06,0x00,0x01,0x00,0x03};
    TEST_ASSERT_EQUAL_INT(5, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect, buf, 5);
}

void test_write_multiple_registers(void) {
    uint8_t buf[16];
    uint16_t vals[] = {10, 20};
    int n = modbus_pdu_build_write_multiple_registers(buf, sizeof(buf), 1, vals, 2);
    uint8_t expect[] = {0x10,0x00,0x01,0x00,0x02,0x04,0x00,0x0A,0x00,0x14};
    TEST_ASSERT_EQUAL_INT(10, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect, buf, 10);
}

void test_rejects_zero_quantity(void) {
    uint8_t buf[16];
    int n = modbus_pdu_build_read_holding_registers(buf, sizeof(buf), 0, 0);
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_INVALID_ARG, n);
}

void test_parse_read_registers_response(void) {
    uint8_t resp[] = {0x03, 0x04, 0x00, 0x0A, 0x00, 0x14};
    modbus_registers_t regs;
    uint8_t exc = 0;
    int rc = modbus_pdu_parse_read_registers_response(resp, sizeof(resp), 0x03, &regs, &exc);
    TEST_ASSERT_EQUAL_INT(MODBUS_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(2, regs.count);
    TEST_ASSERT_EQUAL_UINT16(10, regs.values[0]);
    TEST_ASSERT_EQUAL_UINT16(20, regs.values[1]);
}

void test_parse_exception_response(void) {
    uint8_t resp[] = {0x83, 0x02};
    modbus_registers_t regs;
    uint8_t exc = 0;
    int rc = modbus_pdu_parse_read_registers_response(resp, sizeof(resp), 0x03, &regs, &exc);
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_EXCEPTION, rc);
    TEST_ASSERT_EQUAL_UINT8(0x02, exc);
}

void test_parse_write_response_truncated(void) {
    uint8_t resp[] = {0x05, 0x00, 0xAC, 0xFF}; // 4 bytes instead of 5
    uint8_t exc = 0;
    int rc = modbus_pdu_parse_write_response(resp, sizeof(resp), 0x05, &exc);
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_RESPONSE_TOO_SHORT, rc);
}

void test_parse_write_response_valid(void) {
    uint8_t resp[] = {0x05, 0x00, 0xAC, 0xFF, 0x00}; // 5 bytes
    uint8_t exc = 0;
    int rc = modbus_pdu_parse_write_response(resp, sizeof(resp), 0x05, &exc);
    TEST_ASSERT_EQUAL_INT(MODBUS_OK, rc);
}

void test_parse_read_bits_response_valid(void) {
    uint8_t resp[] = {0x01, 0x03, 0xCD, 0x6B, 0x05};
    modbus_bits_t bits;
    uint8_t exc = 0;
    int rc = modbus_pdu_parse_read_bits_response(resp, sizeof(resp), 0x01, &bits, &exc);
    TEST_ASSERT_EQUAL_INT(MODBUS_OK, rc);
    TEST_ASSERT_EQUAL_UINT16(3, bits.byte_count);
    TEST_ASSERT_EQUAL_UINT8(0xCD, bits.bits[0]);
    TEST_ASSERT_EQUAL_UINT8(0x6B, bits.bits[1]);
    TEST_ASSERT_EQUAL_UINT8(0x05, bits.bits[2]);
}

void test_parse_read_bits_response_truncated(void) {
    uint8_t resp[] = {0x01, 0x03, 0xCD, 0x6B}; // Missing the last data byte (size is 4 instead of 5)
    modbus_bits_t bits;
    uint8_t exc = 0;
    int rc = modbus_pdu_parse_read_bits_response(resp, sizeof(resp), 0x01, &bits, &exc);
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_RESPONSE_TOO_SHORT, rc);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_read_holding_registers);
    RUN_TEST(test_write_single_coil_on);
    RUN_TEST(test_build_read_coils);
    RUN_TEST(test_build_read_discrete_inputs);
    RUN_TEST(test_build_read_input_registers);
    RUN_TEST(test_build_write_single_register);
    RUN_TEST(test_write_multiple_registers);
    RUN_TEST(test_rejects_zero_quantity);
    RUN_TEST(test_parse_read_registers_response);
    RUN_TEST(test_parse_exception_response);
    RUN_TEST(test_parse_write_response_truncated);
    RUN_TEST(test_parse_write_response_valid);
    RUN_TEST(test_parse_read_bits_response_valid);
    RUN_TEST(test_parse_read_bits_response_truncated);
    return UNITY_END();
}
