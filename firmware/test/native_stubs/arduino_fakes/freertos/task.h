#pragma once
#include <stdint.h>

// Native-test stub matching just the subset of FreeRTOS's real task.h
// surface that firmware/lib/modbus_master/modbus_master.cpp calls.
// Definitions (function bodies) live in test_modbus_master.cpp, kept
// separate from these declarations so both modbus_master.cpp and
// test_modbus_master.cpp - two different translation units - see the
// exact same types/signatures without redefinition conflicts.
#ifdef __cplusplus
extern "C" {
#endif

typedef void* TaskHandle_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;

#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
#define pdFAIL 0
#define pdMS_TO_TICKS(x) (x)

BaseType_t xTaskCreatePinnedToCore(void (*pxTaskCode)(void *), const char *pcName,
                                    uint32_t ulStackDepth, void *pvParameters,
                                    UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask,
                                    BaseType_t xCoreID);
void vTaskDelay(TickType_t xTicksToDelay);
void vTaskDelete(TaskHandle_t xTaskToDelete);

#ifdef __cplusplus
}
#endif
