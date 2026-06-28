#pragma once
#include "Utility.h"

struct MemoryData {
    // Internal DRAM heap
    uint32_t heapTotal;
    uint32_t heapFree;
    uint32_t heapMinFree;     // Low-water mark since boot
    uint32_t heapMaxBlock;    // Largest single allocatable block
    uint32_t heapUsed;

    // DMA-capable DRAM
    uint32_t dmaTotal;
    uint32_t dmaFree;

    // IRAM
    uint32_t iramTotal;
    uint32_t iramFree;

    // PSRAM (external SPI RAM)
    bool     psramPresent;
    uint32_t psramTotal;
    uint32_t psramFree;
    uint32_t psramMinFree;
    uint32_t psramMaxBlock;

    // Sketch
    uint32_t sketchSize;
    uint32_t sketchFreeSpace;
};

class MemoryInfo
{
public:
    static void collect(MemoryData& d);
    static void print  (DiagFormatter& fmt);
};
