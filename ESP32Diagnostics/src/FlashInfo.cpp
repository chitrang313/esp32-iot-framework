#include "FlashInfo.h"
#include <esp_flash.h>
#include <string.h>

void FlashInfo::collect(FlashData& d)
{
    memset(&d, 0, sizeof(d));

    // Size from IDF 5.x non-deprecated API (NULL = main/default flash chip)
    esp_flash_get_size(NULL, &d.chipSizeBytes);

    d.speedHz = ESP.getFlashChipSpeed();
    d.mode    = (uint8_t)ESP.getFlashChipMode();

    // JEDEC ID: upper byte = manufacturer, middle = type, lower = capacity
    esp_flash_read_id(NULL, &d.jedecId);
    d.mfgId    = (uint8_t)((d.jedecId >> 16) & 0xFF);
    d.memType  = (uint8_t)((d.jedecId >>  8) & 0xFF);
    d.capacity = (uint8_t)( d.jedecId        & 0xFF);

    strncpy(d.manufacturer, manufacturerName(d.mfgId), sizeof(d.manufacturer) - 1);
    strncpy(d.modeStr,      modeString(d.mode),        sizeof(d.modeStr) - 1);
}

void FlashInfo::print(DiagFormatter& fmt)
{
    FlashData d;
    collect(d);

    fmt.sectionHeader("FLASH INFORMATION");
    fmt.kvBytes("Chip Size",    d.chipSizeBytes);
    fmt.kv("Speed",             d.speedHz / 1000000UL, "MHz");
    fmt.kv("Mode",              d.modeStr);
    fmt.kvHex32("JEDEC ID",     d.jedecId);
    fmt.kv("Manufacturer",      d.manufacturer);

    char typeBuf[8];
    snprintf(typeBuf, sizeof(typeBuf), "0x%02X", d.memType);
    fmt.kv("Memory Type",       typeBuf);

    char capBuf[16];
    snprintf(capBuf, sizeof(capBuf), "0x%02X (%lu MB)",
             d.capacity,
             (unsigned long)(1UL << (d.capacity > 17 ? d.capacity - 17 : 0)));
    fmt.kv("Capacity Code",     capBuf);
}

const char* FlashInfo::manufacturerName(uint8_t mfg)
{
    switch (mfg)
    {
        case 0xEF: return "Winbond";
        case 0xC8: return "GigaDevice";
        case 0x20: return "Micron / ST";
        case 0x9D: return "ISSI";
        case 0x85: return "Puya";
        case 0x68: return "Boya Micro";
        case 0xA1: return "Fudan Micro";
        case 0x5E: return "ZB Semiconductor";
        case 0x1C: return "EON";
        case 0xC2: return "Macronix";
        case 0xBF: return "Microchip (SST)";
        case 0x01: return "Spansion / Cypress";
        case 0x04: return "Fujitsu";
        default:
        {
            static char buf[16];
            snprintf(buf, sizeof(buf), "Unknown (0x%02X)", mfg);
            return buf;
        }
    }
}

const char* FlashInfo::modeString(uint8_t mode)
{
    switch (mode)
    {
        case 0:    return "QIO";
        case 1:    return "QOUT";
        case 2:    return "DIO";
        case 3:    return "DOUT";
        case 4:    return "FAST_READ";
        case 5:    return "SLOW_READ";
        default:   return "Unknown";
    }
}
