#include "BuildInfo.h"
#include <esp_idf_version.h>
#include <string.h>
#include <stdio.h>

void BuildInfo::collect(BuildData& d)
{
    memset(&d, 0, sizeof(d));

    strncpy(d.buildDate, __DATE__, sizeof(d.buildDate) - 1);
    strncpy(d.buildTime, __TIME__, sizeof(d.buildTime) - 1);
    strncpy(d.idfVersion, esp_get_idf_version(), sizeof(d.idfVersion) - 1);
    strncpy(d.gccVersion, __VERSION__, sizeof(d.gccVersion) - 1);

    // ARDUINO is a hex-encoded version: 0x10815 = 1.8.21
    // Arduino ESP32 Core encodes as major*10000 + minor*100 + patch
#if defined(ARDUINO)
    uint32_t av = (uint32_t)ARDUINO;
    snprintf(d.arduinoVersion, sizeof(d.arduinoVersion),
             "%lu.%lu.%lu",
             (unsigned long)((av / 10000UL) % 100UL),
             (unsigned long)((av / 100UL)   % 100UL),
             (unsigned long)( av             % 100UL));
#else
    strncpy(d.arduinoVersion, "N/A", sizeof(d.arduinoVersion) - 1);
#endif

#if defined(CONFIG_IDF_TARGET)
    strncpy(d.targetChip, CONFIG_IDF_TARGET, sizeof(d.targetChip) - 1);
#else
    strncpy(d.targetChip, "unknown", sizeof(d.targetChip) - 1);
#endif
}

void BuildInfo::print(DiagFormatter& fmt)
{
    BuildData d;
    collect(d);

    fmt.sectionHeader("BUILD INFORMATION");
    fmt.kv("Build Date",       d.buildDate);
    fmt.kv("Build Time",       d.buildTime);
    fmt.kv("IDF Version",      d.idfVersion);
    fmt.kv("GCC Version",      d.gccVersion);
    fmt.kv("Arduino Core",     d.arduinoVersion);
    fmt.kv("Target Chip",      d.targetChip);
}
