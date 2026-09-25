#include <unity.h>
extern "C" {
#include "modbus_pdu.h"
#include "modbus_rtu.h"
}

void setUp(void) {}
void tearDown(void) {}

void test_wrap_matches_known_crc_vector(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A};
    uint8_t frame[16];
    int n = modbus_rtu_wrap(frame, sizeof(frame), 0x01, pdu, sizeof(pdu));
    uint8_t expect[] = {0x01,0x03,0x00,0x00,0x00,0x0A,0xC5,0xCD};
    TEST_ASSERT_EQUAL_INT(8, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect, frame, 8);
}

void test_unwrap_round_trip(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A};
    uint8_t frame[16];
    int n = modbus_rtu_wrap(frame, sizeof(frame), 0x01, pdu, sizeof(pdu));
    uint8_t pdu_out[16];
    int un = modbus_rtu_unwrap(frame, n, 0x01, pdu_out, sizeof(pdu_out));
    TEST_ASSERT_EQUAL_INT((int)sizeof(pdu), un);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(pdu, pdu_out, sizeof(pdu));
}

void test_unwrap_rejects_bad_crc(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A};
    uint8_t frame[16];
    int n = modbus_rtu_wrap(frame, sizeof(frame), 0x01, pdu, sizeof(pdu));
    frame[n - 1] ^= 0xFF;
    uint8_t pdu_out[16];
    int un = modbus_rtu_unwrap(frame, n, 0x01, pdu_out, sizeof(pdu_out));
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_CRC_MISMATCH, un);
}

void test_unwrap_rejects_wrong_address(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A};
    uint8_t frame[16];
    int n = modbus_rtu_wrap(frame, sizeof(frame), 0x01, pdu, sizeof(pdu));
    uint8_t pdu_out[16];
    int un = modbus_rtu_unwrap(frame, n, 0x02, pdu_out, sizeof(pdu_out));
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_ADDRESS_MISMATCH, un);
}

// T2: edge-case coverage added 2026-09-24 - wrap()/unwrap() had no tests at
// all for their own argument-validation branches (null pointers, output
// buffer too small) or unwrap()'s "frame too short to even contain a CRC"
// branch. Only the happy path and the two checks past the length gate
// (CRC, address) had coverage before this.

void test_wrap_rejects_null_out(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A};
    int n = modbus_rtu_wrap(NULL, 16, 0x01, pdu, sizeof(pdu));
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_INVALID_ARG, n);
}

void test_wrap_rejects_null_pdu(void) {
    uint8_t frame[16];
    int n = modbus_rtu_wrap(frame, sizeof(frame), 0x01, NULL, 5);
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_INVALID_ARG, n);
}

void test_wrap_rejects_buffer_too_small(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A}; // needs 1+5+2 = 8 bytes
    uint8_t frame[7];
    int n = modbus_rtu_wrap(frame, sizeof(frame), 0x01, pdu, sizeof(pdu));
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_BUFFER_TOO_SMALL, n);
}

void test_unwrap_rejects_too_short_frame(void) {
    // Needs at least 4 bytes (addr + fc + crc lo/hi) before the CRC check
    // even runs - 3 bytes must fail closed on length alone.
    uint8_t frame[3] = {0x01, 0x03, 0x00};
    uint8_t pdu_out[16];
    int un = modbus_rtu_unwrap(frame, sizeof(frame), 0x01, pdu_out, sizeof(pdu_out));
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_RESPONSE_TOO_SHORT, un);
}

void test_unwrap_rejects_null_frame(void) {
    uint8_t pdu_out[16];
    int un = modbus_rtu_unwrap(NULL, 8, 0x01, pdu_out, sizeof(pdu_out));
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_INVALID_ARG, un);
}

void test_unwrap_rejects_null_pdu_out(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A};
    uint8_t frame[16];
    int n = modbus_rtu_wrap(frame, sizeof(frame), 0x01, pdu, sizeof(pdu));
    int un = modbus_rtu_unwrap(frame, n, 0x01, NULL, 16);
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_INVALID_ARG, un);
}

void test_unwrap_rejects_buffer_too_small(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A}; // 5-byte PDU
    uint8_t frame[16];
    int n = modbus_rtu_wrap(frame, sizeof(frame), 0x01, pdu, sizeof(pdu));
    uint8_t pdu_out[4]; // one byte too small for the 5-byte PDU
    int un = modbus_rtu_unwrap(frame, n, 0x01, pdu_out, sizeof(pdu_out));
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_BUFFER_TOO_SMALL, un);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_wrap_matches_known_crc_vector);
    RUN_TEST(test_unwrap_round_trip);
    RUN_TEST(test_unwrap_rejects_bad_crc);
    RUN_TEST(test_unwrap_rejects_wrong_address);
    RUN_TEST(test_wrap_rejects_null_out);
    RUN_TEST(test_wrap_rejects_null_pdu);
    RUN_TEST(test_wrap_rejects_buffer_too_small);
    RUN_TEST(test_unwrap_rejects_too_short_frame);
    RUN_TEST(test_unwrap_rejects_null_frame);
    RUN_TEST(test_unwrap_rejects_null_pdu_out);
    RUN_TEST(test_unwrap_rejects_buffer_too_small);
    return UNITY_END();
}
