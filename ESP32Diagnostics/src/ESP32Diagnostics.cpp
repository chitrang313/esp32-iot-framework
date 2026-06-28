#include "ESP32Diagnostics.h"
#include <string.h>
#include <stdio.h>

ESP32Diagnostics::ESP32Diagnostics()
    : m_output     (nullptr)
    , m_initialized(false)
{}

void ESP32Diagnostics::begin(Print& output)
{
    m_output      = &output;
    m_initialized = true;
}

void ESP32Diagnostics::printAll()
{
    printSections(DIAG_ALL);
}

void ESP32Diagnostics::printSections(uint16_t mask)
{
    if (!m_initialized) return;

    // Stack-allocate formatter — avoids any heap use.
    DiagFormatter fmt(*m_output);

    char title[72];
    snprintf(title, sizeof(title), "ESP32 DIAGNOSTICS REPORT  [%s]",
             ESP.getChipModel());
    fmt.reportHeader(title);

    if (mask & DIAG_BUILD)      BuildInfo::print(fmt);
    if (mask & DIAG_CHIP)       ChipInfo::print(fmt);
    if (mask & DIAG_FLASH)      FlashInfo::print(fmt);
    if (mask & DIAG_PARTITIONS) PartitionInfo::print(fmt);
    if (mask & DIAG_MEMORY)     MemoryInfo::print(fmt);
    if (mask & DIAG_EFUSE)      EfuseInfo::print(fmt);
    if (mask & DIAG_NETWORK)    NetworkInfo::print(fmt);
    if (mask & DIAG_FREERTOS)   FreeRTOSInfo::print(fmt);
    if (mask & DIAG_GPIO)       GPIOInfo::print(fmt);

    fmt.reportFooter();
}

// Individual section printers — each creates its own formatter on the stack.
#define SECTION_PRINT(ModuleClass) \
    do { if (!m_initialized) return; \
         DiagFormatter fmt(*m_output); \
         ModuleClass::print(fmt); } while (0)

void ESP32Diagnostics::printChip()       { SECTION_PRINT(ChipInfo); }
void ESP32Diagnostics::printMemory()     { SECTION_PRINT(MemoryInfo); }
void ESP32Diagnostics::printFlash()      { SECTION_PRINT(FlashInfo); }
void ESP32Diagnostics::printPartitions() { SECTION_PRINT(PartitionInfo); }
void ESP32Diagnostics::printEfuse()      { SECTION_PRINT(EfuseInfo); }
void ESP32Diagnostics::printGPIO()       { SECTION_PRINT(GPIOInfo); }
void ESP32Diagnostics::printNetwork()    { SECTION_PRINT(NetworkInfo); }
void ESP32Diagnostics::printFreeRTOS()   { SECTION_PRINT(FreeRTOSInfo); }
void ESP32Diagnostics::printBuild()      { SECTION_PRINT(BuildInfo); }

#undef SECTION_PRINT
