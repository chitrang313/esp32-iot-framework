#include "FreeRTOSInfo.h"
#include <freertos/portable.h>
#include <string.h>

void FreeRTOSInfo::collect(FreeRTOSData& d)
{
    memset(&d, 0, sizeof(d));

    d.tickRateHz      = (uint32_t)configTICK_RATE_HZ;
    d.tickCount       = (uint32_t)xTaskGetTickCount();
    d.uptimeMs        = d.tickCount * (1000UL / (uint32_t)configTICK_RATE_HZ);
    d.taskCount       = (uint32_t)uxTaskGetNumberOfTasks();
    d.schedulerState  = xTaskGetSchedulerState();
    d.freeHeapFreeRTOS = (uint32_t)xPortGetFreeHeapSize();
    d.minEverFreeHeap  = (uint32_t)xPortGetMinimumEverFreeHeapSize();

    // Stack watermark of the calling task (Diagnostics/loop task).
    // Units: stack words (4 bytes each on Xtensa/RISC-V).
    d.currentTaskStack = (uint32_t)uxTaskGetStackHighWaterMark(NULL);

#if (configUSE_TRACE_FACILITY == 1) && (configUSE_STATS_FORMATTING_FUNCTIONS == 1)
    vTaskList(d.taskList);
    d.taskListAvailable = true;
#else
    strncpy(d.taskList,
            "(vTaskList not available: configUSE_TRACE_FACILITY=0)",
            sizeof(d.taskList) - 1);
    d.taskListAvailable = false;
#endif
}

void FreeRTOSInfo::print(DiagFormatter& fmt)
{
    FreeRTOSData d;
    collect(d);

    fmt.sectionHeader("FREERTOS INFORMATION");

    fmt.kv("Scheduler State", schedulerStateName(d.schedulerState));
    fmt.kv("Tick Rate",       d.tickRateHz,        "Hz");
    fmt.kv("Tick Count",      d.tickCount);
    fmt.kv("Uptime",          d.uptimeMs / 1000UL,  "s");
    fmt.kv("Task Count",      d.taskCount);

    fmt.blankLine();
    fmt.println("  [FreeRTOS Heap]");
    fmt.kvBytes("Free (FreeRTOS view)", d.freeHeapFreeRTOS);
    fmt.kvBytes("Min Ever Free",        d.minEverFreeHeap);

    fmt.blankLine();
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%lu words (%lu bytes)",
                 (unsigned long)d.currentTaskStack,
                 (unsigned long)(d.currentTaskStack * sizeof(StackType_t)));
        fmt.kv("Calling task stack free", buf);
    }

    if (d.taskListAvailable)
    {
        fmt.blankLine();
        fmt.println("  [Task List]");
        fmt.println("  Name            State  Pri  Stack   Num");
        fmt.println("  --------------  -----  ---  ------  ---");
        // vTaskList output uses \t separators; print raw so user can read it
        fmt.println(d.taskList);
    }
}

const char* FreeRTOSInfo::schedulerStateName(BaseType_t state)
{
    switch (state)
    {
        case taskSCHEDULER_NOT_STARTED: return "Not started";
        case taskSCHEDULER_RUNNING:     return "Running";
        case taskSCHEDULER_SUSPENDED:   return "Suspended";
        default:                        return "Unknown";
    }
}
