#pragma once
#include "Utility.h"
#include <esp_partition.h>

static constexpr uint8_t PARTITION_MAX = 16;

struct PartitionEntry {
    char     label[17];
    uint32_t offset;
    uint32_t size;
    uint8_t  type;
    uint8_t  subtype;
    bool     encrypted;
    bool     isRunning;   // True for the currently executing app partition
    bool     isBoot;      // True for the OTA boot target partition
};

struct PartitionData {
    PartitionEntry entries[PARTITION_MAX];
    uint8_t        count;
    uint32_t       totalPartitionedBytes;
};

class PartitionInfo
{
public:
    static void collect(PartitionData& d);
    static void print  (DiagFormatter& fmt);
private:
    static const char* typeName   (uint8_t type, uint8_t subtype);
    static const char* subtypeName(uint8_t type, uint8_t subtype);
};
