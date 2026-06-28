#pragma once
#include "Utility.h"

struct FlashData {
    uint32_t chipSizeBytes;
    uint32_t speedHz;
    uint8_t  mode;           // FlashMode_t raw value
    uint32_t jedecId;        // Full 3-byte JEDEC ID
    uint8_t  mfgId;          // Manufacturer byte
    uint8_t  memType;        // Memory type byte
    uint8_t  capacity;       // Capacity byte (2^n bytes)
    char     manufacturer[24];
    char     modeStr[12];
};

class FlashInfo
{
public:
    static void collect(FlashData& d);
    static void print  (DiagFormatter& fmt);
private:
    static const char* manufacturerName(uint8_t mfg);
    static const char* modeString      (uint8_t mode);
};
