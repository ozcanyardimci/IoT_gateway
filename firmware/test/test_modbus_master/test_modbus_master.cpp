#include <unity.h>
#include "modbus_master.h"
#include "HardwareSerial.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

HardwareSerial Serial1(1);

// FreeRTOS/Arduino stub DEFINITIONS only - the type/macro declarations
// and function prototypes these bodies satisfy now live in this
// directory's freertos/task.h and freertos/semphr.h (added so
// modbus_master.cpp, a SEPARATE translation unit that #includes those
// same paths directly, sees identical declarations instead of only
// what used to be private to this file - the previous version of this
// file defined these inline here, which satisfied this file's own
// compile but left modbus_master.cpp with a literal missing-header
// compile error since nothing at those include paths existed for it
// to find).
#ifdef __cplusplus
extern "C" {
#endif

SemaphoreHandle_t xSemaphoreCreateMutex(void) { return (SemaphoreHandle_t)1; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait) { (void)xSemaphore; (void)xTicksToWait; return pdTRUE; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore) { (void)xSemaphore; return pdTRUE; }
void vTaskDelay(TickType_t xTicksToDelay) { (void)xTicksToDelay; }
void vTaskDelete(TaskHandle_t xTaskToDelete) { (void)xTaskToDelete; }
BaseType_t xTaskCreatePinnedToCore(void (*pxTaskCode)(void*), const char* pcName, uint32_t ulStackDepth, void* pvParameters, UBaseType_t uxPriority, TaskHandle_t* pxCreatedTask, BaseType_t xCoreID) {
    (void)pxTaskCode; (void)pcName; (void)ulStackDepth; (void)pvParameters; (void)uxPriority; (void)pxCreatedTask; (void)xCoreID;
    return pdPASS;
}

// Arduino stubs - declared in this directory's Arduino.h (included by
// modbus_master.cpp via <Arduino.h>), defined here.
unsigned long millis() { return 0; }
unsigned long micros() { return 0; }
void delay(uint32_t ms) { (void)ms; }

#ifdef __cplusplus
}
#endif


void setUp(void) {
    Serial1.fake_reset();
}
void tearDown(void) {}

// Reference values computed independently from the Modbus_over_serial_line
// spec's own formula (3.5 * 11 bits / baud), not copied from the
// implementation - see modbus_master.h's header comment for the source.
void test_9600_baud_matches_spec_formula(void) {
    // 3.5 * 11 * 1,000,000 / 9600 = 4010.41666... -> 4010us (integer truncation)
    TEST_ASSERT_EQUAL_UINT32(4010, modbusRtuInterFrameGapUs(9600));
}
void test_19200_baud_still_uses_formula_not_fixed_value(void) {
    // Spec's fixed-value exception is for baud RATES GREATER THAN 19200 -
    // 19200 itself still uses the character-time formula.
    // 3.5 * 11 * 1,000,000 / 19200 = 2005.2083... -> 2005us
    TEST_ASSERT_EQUAL_UINT32(2005, modbusRtuInterFrameGapUs(19200));
}
void test_above_19200_uses_fixed_1750us(void) {
    TEST_ASSERT_EQUAL_UINT32(1750, modbusRtuInterFrameGapUs(19201));
}
void test_115200_baud_uses_fixed_1750us(void) {
    TEST_ASSERT_EQUAL_UINT32(1750, modbusRtuInterFrameGapUs(115200));
}
void test_zero_baud_does_not_divide_by_zero(void) {
    TEST_ASSERT_EQUAL_UINT32(1750, modbusRtuInterFrameGapUs(0));
}
void test_low_baud_gives_a_longer_gap(void) {
    // 2400 baud (a real, if slow, RS485 field-device rate): gap should be
    // noticeably longer than at 9600 - sanity check on the formula's
    // direction, not just its exact output.
    TEST_ASSERT_TRUE(modbusRtuInterFrameGapUs(2400) > modbusRtuInterFrameGapUs(9600));
}

void test_modbus_target_add_success(void) {
    Rs485Serial bus;
    ModbusMasterTask task;
    TEST_ASSERT_TRUE(task.begin(bus, 9600));
    
    ModbusMasterTarget target;
    target.slave_addr = 1;
    target.start_addr = 0x01;
    target.quantity = 2;
    target.poll_interval_ms = 1000;
    
    TEST_ASSERT_EQUAL_INT(0, task.addTarget(target));
}

void test_modbus_target_add_fails_after_start(void) {
    Rs485Serial bus;
    ModbusMasterTask task;
    TEST_ASSERT_TRUE(task.begin(bus, 9600));
    
    // We can add a target before start
    ModbusMasterTarget target;
    target.slave_addr = 1;
    target.start_addr = 0x01;
    target.quantity = 2;
    target.poll_interval_ms = 1000;
    TEST_ASSERT_EQUAL_INT(0, task.addTarget(target));

    // Call start (using dummy watchdog and core)
    TEST_ASSERT_TRUE(task.start(nullptr, 1, 1));
    
    // Attempting to add a target after start should be rejected (-1)
    TEST_ASSERT_EQUAL_INT(-1, task.addTarget(target));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_9600_baud_matches_spec_formula);
    RUN_TEST(test_19200_baud_still_uses_formula_not_fixed_value);
    RUN_TEST(test_above_19200_uses_fixed_1750us);
    RUN_TEST(test_115200_baud_uses_fixed_1750us);
    RUN_TEST(test_zero_baud_does_not_divide_by_zero);
    RUN_TEST(test_low_baud_gives_a_longer_gap);
    RUN_TEST(test_modbus_target_add_success);
    RUN_TEST(test_modbus_target_add_fails_after_start);
    return UNITY_END();
}
