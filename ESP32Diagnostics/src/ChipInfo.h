#pragma once
#include "Utility.h"

struct ChipData {
    char     model[16];
    uint32_t revision;
    uint8_t  cores;
    uint32_t cpuFreqMHz;
    uint32_t xtalFreqMHz;
    uint64_t efuseId;          // Unique chip ID from eFuse
    uint8_t  mac[6];           // Base MAC address
    uint32_t featureBits;      // esp_chip_info_t.features raw bits
    bool     hasWiFi;
    bool     hasBT;
    bool     hasBLE;
    bool     has802154;
    bool     hasEmbFlash;
    bool     hasEmbPSRAM;
    bool     hasTempSensor;
    char     resetReason[32];
};

class ChipInfo
{
public:
    static void collect(ChipData& d);
    static void print  (DiagFormatter& fmt);
private:
    static const char* resetReasonStr(int reason);
};
