#include "ChipInfo.h"
#include <esp_chip_info.h>
#include <esp_system.h>
#include <esp_mac.h>
#include <soc/soc_caps.h>
#include <string.h>

// Chip feature bits (from esp_chip_info.h, IDF 5.x)
static constexpr uint32_t FEAT_EMB_FLASH  = (1 << 0);
static constexpr uint32_t FEAT_WIFI_BGN   = (1 << 1);
static constexpr uint32_t FEAT_BLE        = (1 << 4);
static constexpr uint32_t FEAT_BT         = (1 << 5);
static constexpr uint32_t FEAT_IEEE802154 = (1 << 6);
static constexpr uint32_t FEAT_EMB_PSRAM  = (1 << 7);

void ChipInfo::collect(ChipData& d)
{
    memset(&d, 0, sizeof(d));

    esp_chip_info_t ci;
    esp_chip_info(&ci);

    strncpy(d.model, ESP.getChipModel(), sizeof(d.model) - 1);
    d.revision    = ESP.getChipRevision();
    d.cores       = ci.cores;
    d.cpuFreqMHz  = ESP.getCpuFreqMHz();
    d.featureBits = ci.features;
    d.hasWiFi     = (ci.features & FEAT_WIFI_BGN)   != 0;
    d.hasBT       = (ci.features & FEAT_BT)         != 0;
    d.hasBLE      = (ci.features & FEAT_BLE)        != 0;
    d.has802154   = (ci.features & FEAT_IEEE802154)  != 0;
    d.hasEmbFlash = (ci.features & FEAT_EMB_FLASH)  != 0;
    d.hasEmbPSRAM = (ci.features & FEAT_EMB_PSRAM)  != 0;

#if SOC_TEMP_SENSOR_SUPPORTED
    d.hasTempSensor = true;
#else
    d.hasTempSensor = false;
#endif

    // Crystal frequency: accessible via rtc_clk_xtal_freq_get() in IDF 5.x
    // Falls back to common 40 MHz default if unavailable.
#if defined(CONFIG_IDF_TARGET_ESP32H2)
    d.xtalFreqMHz = 32;
#elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
    d.xtalFreqMHz = 40;
#else
    d.xtalFreqMHz = 40;
#endif

    // Unique chip ID from eFuse (chip-specific MAC + factory data)
    d.efuseId = ESP.getEfuseMac();

    // Base MAC address
    esp_efuse_mac_get_default(d.mac);

    // Reset reason string
    strncpy(d.resetReason, resetReasonStr((int)esp_reset_reason()),
            sizeof(d.resetReason) - 1);
}

void ChipInfo::print(DiagFormatter& fmt)
{
    ChipData d;
    collect(d);

    fmt.sectionHeader("CHIP INFORMATION");
    fmt.kv("Model",           d.model);
    fmt.kv("Revision",        d.revision);
    fmt.kv("CPU Cores",       (uint32_t)d.cores);
    fmt.kv("CPU Frequency",   d.cpuFreqMHz, "MHz");
    fmt.kv("XTAL Frequency",  d.xtalFreqMHz, "MHz");
    fmt.kvHex64("Chip ID (eFuse)",  d.efuseId);
    fmt.kvMAC("Base MAC",      d.mac);
    fmt.kv("WiFi",            d.hasWiFi);
    fmt.kv("Bluetooth Classic", d.hasBT);
    fmt.kv("Bluetooth LE",    d.hasBLE);
    fmt.kv("IEEE 802.15.4",   d.has802154);
    fmt.kv("Embedded Flash",  d.hasEmbFlash);
    fmt.kv("Embedded PSRAM",  d.hasEmbPSRAM);
    fmt.kv("Temp. Sensor",    d.hasTempSensor);
    fmt.kv("Reset Reason",    d.resetReason);
}

const char* ChipInfo::resetReasonStr(int r)
{
    switch (r)
    {
        case 1:  return "Power-on";
        case 2:  return "External pin";
        case 3:  return "Software (esp_restart)";
        case 4:  return "Panic / exception";
        case 5:  return "Interrupt watchdog";
        case 6:  return "Task watchdog";
        case 7:  return "Other watchdog";
        case 8:  return "Deep sleep wakeup";
        case 9:  return "Brownout";
        case 10: return "SDIO reset";
        case 11: return "USB reset";
        case 12: return "JTAG reset";
        case 13: return "eFuse error";
        case 14: return "Power glitch";
        case 15: return "CPU lockup";
        default: return "Unknown";
    }
}
