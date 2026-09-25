#include <unity.h>
extern "C" {
#include "modbus_pdu.h"
#include "modbus_tcp.h"
}

void setUp(void) {}
void tearDown(void) {}

void test_wrap_matches_expected_mbap(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A};
    uint8_t frame[16];
    int n = modbus_tcp_wrap(frame, sizeof(frame), 0x1234, 0x01, pdu, sizeof(pdu));
    uint8_t expect[] = {0x12,0x34, 0x00,0x00, 0x00,0x06, 0x01, 0x03,0x00,0x00,0x00,0x0A};
    TEST_ASSERT_EQUAL_INT(12, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect, frame, 12);
}

void test_unwrap_round_trip(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A};
    uint8_t frame[16];
    int n = modbus_tcp_wrap(frame, sizeof(frame), 0x1234, 0x01, pdu, sizeof(pdu));
    uint8_t pdu_out[16];
    int un = modbus_tcp_unwrap(frame, n, 0x1234, pdu_out, sizeof(pdu_out));
    TEST_ASSERT_EQUAL_INT((int)sizeof(pdu), un);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(pdu, pdu_out, sizeof(pdu));
}

void test_unwrap_rejects_wrong_transaction_id(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A};
    uint8_t frame[16];
    int n = modbus_tcp_wrap(frame, sizeof(frame), 0x1234, 0x01, pdu, sizeof(pdu));
    uint8_t pdu_out[16];
    int un = modbus_tcp_unwrap(frame, n, 0x9999, pdu_out, sizeof(pdu_out));
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_TRANSACTION_MISMATCH, un);
}

// T2: edge-case coverage added 2026-09-24 - same gap as test_modbus_rtu.cpp:
// wrap()/unwrap()'s own argument-validation and length-gate branches had no
// direct tests before this, only the happy path plus the transaction-id
// mismatch check that runs after the length gate.

void test_wrap_rejects_null_out(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A};
    int n = modbus_tcp_wrap(NULL, 16, 0x1234, 0x01, pdu, sizeof(pdu));
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_INVALID_ARG, n);
}

void test_wrap_rejects_null_pdu(void) {
    uint8_t frame[16];
    int n = modbus_tcp_wrap(frame, sizeof(frame), 0x1234, 0x01, NULL, 5);
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_INVALID_ARG, n);
}

void test_wrap_rejects_buffer_too_small(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A}; // needs 7+5 = 12 bytes
    uint8_t frame[11];
    int n = modbus_tcp_wrap(frame, sizeof(frame), 0x1234, 0x01, pdu, sizeof(pdu));
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_BUFFER_TOO_SMALL, n);
}

void test_unwrap_rejects_too_short_frame(void) {
    // Needs at least 8 bytes (7-byte MBAP header + >=1 PDU byte) before the
    // transaction/protocol/length checks even run.
    uint8_t frame[7] = {0x12,0x34, 0x00,0x00, 0x00,0x01, 0x01};
    uint8_t pdu_out[16];
    int un = modbus_tcp_unwrap(frame, sizeof(frame), 0x1234, pdu_out, sizeof(pdu_out));
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_RESPONSE_TOO_SHORT, un);
}

void test_unwrap_rejects_null_frame(void) {
    uint8_t pdu_out[16];
    int un = modbus_tcp_unwrap(NULL, 12, 0x1234, pdu_out, sizeof(pdu_out));
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_INVALID_ARG, un);
}

void test_unwrap_rejects_null_pdu_out(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A};
    uint8_t frame[16];
    int n = modbus_tcp_wrap(frame, sizeof(frame), 0x1234, 0x01, pdu, sizeof(pdu));
    int un = modbus_tcp_unwrap(frame, n, 0x1234, NULL, 16);
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_INVALID_ARG, un);
}

void test_unwrap_rejects_buffer_too_small(void) {
    uint8_t pdu[] = {0x03, 0x00, 0x00, 0x00, 0x0A}; // 5-byte PDU
    uint8_t frame[16];
    int n = modbus_tcp_wrap(frame, sizeof(frame), 0x1234, 0x01, pdu, sizeof(pdu));
    uint8_t pdu_out[4]; // one byte too small for the 5-byte PDU
    int un = modbus_tcp_unwrap(frame, n, 0x1234, pdu_out, sizeof(pdu_out));
    TEST_ASSERT_EQUAL_INT(MODBUS_ERR_BUFFER_TOO_SMALL, un);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_wrap_matches_expected_mbap);
    RUN_TEST(test_unwrap_round_trip);
    RUN_TEST(test_unwrap_rejects_wrong_transaction_id);
    RUN_TEST(test_wrap_rejects_null_out);
    RUN_TEST(test_wrap_rejects_null_pdu);
    RUN_TEST(test_wrap_rejects_buffer_too_small);
    RUN_TEST(test_unwrap_rejects_too_short_frame);
    RUN_TEST(test_unwrap_rejects_null_frame);
    RUN_TEST(test_unwrap_rejects_null_pdu_out);
    RUN_TEST(test_unwrap_rejects_buffer_too_small);
    return UNITY_END();
}
