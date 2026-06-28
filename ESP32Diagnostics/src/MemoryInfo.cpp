#include "MemoryInfo.h"
#include <esp_heap_caps.h>

void MemoryInfo::collect(MemoryData& d)
{
    // Internal heap (MALLOC_CAP_INTERNAL covers DRAM used for the main heap)
    d.heapTotal    = heap_caps_get_total_size   (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    d.heapFree     = heap_caps_get_free_size    (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    d.heapMinFree  = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    d.heapMaxBlock = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    d.heapUsed     = (d.heapTotal > d.heapFree) ? (d.heapTotal - d.heapFree) : 0;

    // DMA-capable memory (subset of internal)
    d.dmaTotal = heap_caps_get_total_size (MALLOC_CAP_DMA);
    d.dmaFree  = heap_caps_get_free_size  (MALLOC_CAP_DMA);

    // IRAM (instruction RAM, also usable as fast data SRAM)
    d.iramTotal = heap_caps_get_total_size(MALLOC_CAP_IRAM_8BIT);
    d.iramFree  = heap_caps_get_free_size (MALLOC_CAP_IRAM_8BIT);

    // PSRAM
    d.psramPresent  = (ESP.getPsramSize() > 0);
    d.psramTotal    = ESP.getPsramSize();
    d.psramFree     = ESP.getFreePsram();
    d.psramMinFree  = ESP.getMinFreePsram();
    d.psramMaxBlock = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

    // Sketch storage
    d.sketchSize      = ESP.getSketchSize();
    d.sketchFreeSpace = ESP.getFreeSketchSpace();
}

void MemoryInfo::print(DiagFormatter& fmt)
{
    MemoryData d;
    collect(d);

    fmt.sectionHeader("MEMORY INFORMATION");

    fmt.println("  [Internal Heap]");
    fmt.kvBytes("Total",                d.heapTotal);
    fmt.kvBytes("Used",                 d.heapUsed);
    fmt.kvBytes("Free",                 d.heapFree);
    fmt.kvBytes("Min Free (watermark)", d.heapMinFree);
    fmt.kvBytes("Max Alloc Block",      d.heapMaxBlock);

    {
        // Fragmentation: 100% = free but unusable in one block
        uint8_t frag = 0;
        if (d.heapFree > 0)
            frag = (uint8_t)(100 - (100UL * d.heapMaxBlock / d.heapFree));
        char buf[24];
        snprintf(buf, sizeof(buf), "%u%%", frag);
        fmt.kv("Fragmentation", buf);
    }

    fmt.blankLine();
    fmt.println("  [DMA-Capable DRAM]");
    fmt.kvBytes("Total", d.dmaTotal);
    fmt.kvBytes("Free",  d.dmaFree);

    fmt.blankLine();
    fmt.println("  [IRAM (Instruction RAM)]");
    if (d.iramTotal > 0)
    {
        fmt.kvBytes("Total", d.iramTotal);
        fmt.kvBytes("Free",  d.iramFree);
    }
    else
    {
        fmt.kv("IRAM heap", "not exposed to allocator");
    }

    fmt.blankLine();
    fmt.println("  [External PSRAM]");
    if (d.psramPresent)
    {
        fmt.kvBytes("Total",           d.psramTotal);
        fmt.kvBytes("Free",            d.psramFree);
        fmt.kvBytes("Min Free",        d.psramMinFree);
        fmt.kvBytes("Max Alloc Block", d.psramMaxBlock);
    }
    else
    {
        fmt.kv("PSRAM", "not detected");
    }

    fmt.blankLine();
    fmt.println("  [Sketch / OTA]");
    fmt.kvBytes("Sketch size",   d.sketchSize);
    fmt.kvBytes("OTA free space", d.sketchFreeSpace);
}
