#pragma once
#include "Utility.h"
#include <stdint.h>

// Maximum eFuse blocks supported across all ESP32 family members.
// ESP32 classic has 4 blocks; S2/S3/C3/C6/H2 have 11 blocks.
static constexpr uint8_t EFUSE_BLOCK_MAX_DIAG = 11;

struct EfuseData {
    uint8_t  mac[6];
    bool     flashEncEnabled;
    bool     secureBootEnabled;
    bool     blockIsEmpty[EFUSE_BLOCK_MAX_DIAG];
    uint8_t  numBlocks;
    uint32_t keySummary;    // Raw features/security bits (chip-specific)
};

class EfuseInfo
{
public:
    static void collect(EfuseData& d);
    static void print  (DiagFormatter& fmt);
};
