#include <unity.h>
extern "C" {
#include "modbus_crc.h"
}

void setUp(void) {}
void tearDown(void) {}

// Reference values cross-checked against Python's crcmod library
// (crcmod.predefined.mkCrcFun('modbus')), not hand-derived.

void test_read_holding_registers(void) {
    uint8_t msg[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
    TEST_ASSERT_EQUAL_HEX16(0xCDC5, modbus_crc16(msg, sizeof(msg)));
}

void test_short_two_byte_message(void) {
    uint8_t msg[] = {0x02, 0x07};
    TEST_ASSERT_EQUAL_HEX16(0x1241, modbus_crc16(msg, sizeof(msg)));
}

void test_read_input_registers(void) {
    uint8_t msg[] = {0x11, 0x03, 0x00, 0x6B, 0x00, 0x03};
    TEST_ASSERT_EQUAL_HEX16(0x8776, modbus_crc16(msg, sizeof(msg)));
}

void test_write_single_coil(void) {
    uint8_t msg[] = {0x01, 0x05, 0x00, 0xAC, 0xFF, 0x00};
    TEST_ASSERT_EQUAL_HEX16(0x1B4C, modbus_crc16(msg, sizeof(msg)));
}

void test_empty_buffer_is_init_value(void) {
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, modbus_crc16(nullptr, 0));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_read_holding_registers);
    RUN_TEST(test_short_two_byte_message);
    RUN_TEST(test_read_input_registers);
    RUN_TEST(test_write_single_coil);
    RUN_TEST(test_empty_buffer_is_init_value);
    return UNITY_END();
}
