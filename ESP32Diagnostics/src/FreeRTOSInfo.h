#pragma once
#include "Utility.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdint.h>

static constexpr uint16_t FREERTOS_TASKLIST_BUF = 2048;

struct FreeRTOSData {
    uint32_t tickRateHz;
    uint32_t tickCount;
    uint32_t uptimeMs;
    uint32_t taskCount;
    BaseType_t schedulerState;    // taskSCHEDULER_NOT_STARTED / RUNNING / SUSPENDED
    uint32_t freeHeapFreeRTOS;    // xPortGetFreeHeapSize()
    uint32_t minEverFreeHeap;     // xPortGetMinimumEverFreeHeapSize()
    uint32_t currentTaskStack;    // uxTaskGetStackHighWaterMark(NULL) — words remaining
    char     taskList[FREERTOS_TASKLIST_BUF];  // vTaskList output if available
    bool     taskListAvailable;
};

class FreeRTOSInfo
{
public:
    static void collect(FreeRTOSData& d);
    static void print  (DiagFormatter& fmt);
private:
    static const char* schedulerStateName(BaseType_t state);
};
