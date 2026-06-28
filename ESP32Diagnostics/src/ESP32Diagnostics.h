#pragma once

// ESP32 Professional Diagnostics Utility
// Compatible with: Arduino ESP32 Core 3.x (ESP-IDF 5.x)
// Chips: ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6, ESP32-H2
//
// Usage:
//   #include <ESP32Diagnostics.h>
//   ESP32Diagnostics diag;
//   void setup() {
//       Serial.begin(115200);
//       diag.begin(Serial);
//       diag.printAll();
//   }

#include "Utility.h"
#include "ChipInfo.h"
#include "MemoryInfo.h"
#include "FlashInfo.h"
#include "PartitionInfo.h"
#include "EfuseInfo.h"
#include "GPIOInfo.h"
#include "NetworkInfo.h"
#include "FreeRTOSInfo.h"
#include "BuildInfo.h"

// Section bitmask for selective printing
static constexpr uint16_t DIAG_CHIP       = (1 << 0);
static constexpr uint16_t DIAG_MEMORY     = (1 << 1);
static constexpr uint16_t DIAG_FLASH      = (1 << 2);
static constexpr uint16_t DIAG_PARTITIONS = (1 << 3);
static constexpr uint16_t DIAG_EFUSE      = (1 << 4);
static constexpr uint16_t DIAG_GPIO       = (1 << 5);
static constexpr uint16_t DIAG_NETWORK    = (1 << 6);
static constexpr uint16_t DIAG_FREERTOS   = (1 << 7);
static constexpr uint16_t DIAG_BUILD      = (1 << 8);
static constexpr uint16_t DIAG_ALL        = 0xFFFF;

class ESP32Diagnostics
{
public:
    ESP32Diagnostics();

    // Associate with an output stream.  Must be called before any print method.
    void begin(Print& output);

    // Print all sections in logical order.
    void printAll();

    // Print a specific subset of sections via bitmask.
    // Example: diag.printSections(DIAG_CHIP | DIAG_MEMORY);
    void printSections(uint16_t mask);

    // Individual section printers
    void printChip       ();
    void printMemory     ();
    void printFlash      ();
    void printPartitions ();
    void printEfuse      ();
    void printGPIO       ();
    void printNetwork    ();
    void printFreeRTOS   ();
    void printBuild      ();

private:
    Print* m_output;
    bool   m_initialized;
};
