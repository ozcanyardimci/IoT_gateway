#pragma once
#include "task.h" // BaseType_t / TickType_t, same as the real port's semphr.h

// Native-test stub matching just the subset of FreeRTOS's real
// semphr.h surface that modbus_master.cpp calls. See task.h in this
// same directory for why declarations and definitions are split
// across files here.
#ifdef __cplusplus
extern "C" {
#endif

typedef void* SemaphoreHandle_t;

SemaphoreHandle_t xSemaphoreCreateMutex(void);
BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait);
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);

#ifdef __cplusplus
}
#endif
