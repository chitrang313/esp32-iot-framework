#include "PartitionInfo.h"
#include <esp_ota_ops.h>
#include <string.h>

void PartitionInfo::collect(PartitionData& d)
{
    memset(&d, 0, sizeof(d));

    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* boot    = esp_ota_get_boot_partition();

    esp_partition_iterator_t it = esp_partition_find(
        ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);

    while (it != NULL && d.count < PARTITION_MAX)
    {
        const esp_partition_t* p = esp_partition_get(it);
        if (p)
        {
            PartitionEntry& e = d.entries[d.count];
            strncpy(e.label, p->label, sizeof(e.label) - 1);
            e.offset    = p->address;
            e.size      = p->size;
            e.type      = p->type;
            e.subtype   = p->subtype;
            e.encrypted = p->encrypted;
            e.isRunning = (p == running);
            e.isBoot    = (p == boot);
            d.totalPartitionedBytes += p->size;
            ++d.count;
        }
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);
}

void PartitionInfo::print(DiagFormatter& fmt)
{
    PartitionData d;
    collect(d);

    fmt.sectionHeader("PARTITION TABLE");

    // Header row
    fmt.println("  Label            Type      Subtype         Offset       Size      Flags");
    fmt.println("  ---------------  --------  --------------  -----------  --------  -------");

    for (uint8_t i = 0; i < d.count; ++i)
    {
        const PartitionEntry& e = d.entries[i];

        char flags[12] = "";
        if (e.isRunning) strncat(flags, "*RUN", sizeof(flags) - strlen(flags) - 1);
        if (e.isBoot)    strncat(flags, " BOOT", sizeof(flags) - strlen(flags) - 1);
        if (e.encrypted) strncat(flags, " ENC", sizeof(flags) - strlen(flags) - 1);

        char line[DIAG_LINE_WIDTH + 4];
        snprintf(line, sizeof(line),
                 "  %-15s  %-8s  %-14s  0x%08lX   %6lu KB  %s",
                 e.label,
                 typeName(e.type, e.subtype),
                 subtypeName(e.type, e.subtype),
                 (unsigned long)e.offset,
                 (unsigned long)(e.size / 1024UL),
                 flags);
        fmt.println(line);
    }

    fmt.blankLine();
    char totBuf[40];
    snprintf(totBuf, sizeof(totBuf), "%lu KB in %u partitions",
             (unsigned long)(d.totalPartitionedBytes / 1024UL), d.count);
    fmt.kv("Total partitioned", totBuf);
}

const char* PartitionInfo::typeName(uint8_t type, uint8_t /*subtype*/)
{
    switch (type)
    {
        case ESP_PARTITION_TYPE_APP:  return "app";
        case ESP_PARTITION_TYPE_DATA: return "data";
        default:
        {
            static char buf[8];
            snprintf(buf, sizeof(buf), "0x%02X", type);
            return buf;
        }
    }
}

const char* PartitionInfo::subtypeName(uint8_t type, uint8_t subtype)
{
    if (type == ESP_PARTITION_TYPE_APP)
    {
        if (subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY) return "factory";
        if (subtype == ESP_PARTITION_SUBTYPE_APP_TEST)    return "test";
        if (subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_0 &&
            subtype <= ESP_PARTITION_SUBTYPE_APP_OTA_MAX)
        {
            static char buf[8];
            snprintf(buf, sizeof(buf), "ota_%u",
                     subtype - ESP_PARTITION_SUBTYPE_APP_OTA_0);
            return buf;
        }
    }
    if (type == ESP_PARTITION_TYPE_DATA)
    {
        switch (subtype)
        {
            case ESP_PARTITION_SUBTYPE_DATA_OTA:      return "ota";
            case ESP_PARTITION_SUBTYPE_DATA_PHY:      return "phy";
            case ESP_PARTITION_SUBTYPE_DATA_NVS:      return "nvs";
            case ESP_PARTITION_SUBTYPE_DATA_COREDUMP: return "coredump";
            case ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS: return "nvs_keys";
            case ESP_PARTITION_SUBTYPE_DATA_EFUSE_EM: return "efuse_em";
            case ESP_PARTITION_SUBTYPE_DATA_SPIFFS:   return "spiffs";
            case ESP_PARTITION_SUBTYPE_DATA_FAT:      return "fat";
#if defined(ESP_PARTITION_SUBTYPE_DATA_LFS)
            case ESP_PARTITION_SUBTYPE_DATA_LFS:      return "littlefs";
#endif
            default: break;
        }
    }
    static char buf[8];
    snprintf(buf, sizeof(buf), "0x%02X", subtype);
    return buf;
}
