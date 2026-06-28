#include "EfuseInfo.h"
#include <esp_efuse.h>
#include <esp_mac.h>
#include <esp_flash_encrypt.h>
#include <string.h>

// Secure boot status detection without deprecated APIs.
// In IDF 5.x, CONFIG_SECURE_BOOT is set at build time; if not set,
// the device was not compiled with secure boot — report accordingly.
static bool detectSecureBoot()
{
#if defined(CONFIG_SECURE_BOOT) || defined(CONFIG_SECURE_BOOT_ENABLED) \
 || defined(CONFIG_SECURE_BOOT_V2_ENABLED)
    return true;
#else
    return false;
#endif
}

void EfuseInfo::collect(EfuseData& d)
{
    memset(&d, 0, sizeof(d));

    esp_efuse_mac_get_default(d.mac);
    d.flashEncEnabled  = esp_flash_encryption_enabled();
    d.secureBootEnabled = detectSecureBoot();

    // Number of eFuse blocks is chip-specific.
#if defined(CONFIG_IDF_TARGET_ESP32)
    d.numBlocks = 4;
#else
    d.numBlocks = 11;
#endif

    for (uint8_t i = 0; i < d.numBlocks && i < EFUSE_BLOCK_MAX_DIAG; ++i)
    {
        d.blockIsEmpty[i] = esp_efuse_block_is_empty((esp_efuse_block_t)i);
    }
}

void EfuseInfo::print(DiagFormatter& fmt)
{
    EfuseData d;
    collect(d);

    fmt.sectionHeader("EFUSE INFORMATION");
    fmt.kvMAC("Base MAC (eFuse)",   d.mac);
    fmt.kv("Flash Encryption",      d.flashEncEnabled,
           "Enabled", "Disabled");
    fmt.kv("Secure Boot",           d.secureBootEnabled,
           "Enabled (compile-time)", "Disabled");

    fmt.blankLine();
    fmt.println("  [eFuse Block Status]");
    fmt.println("  Block  Status");
    fmt.println("  -----  ----------");

    for (uint8_t i = 0; i < d.numBlocks; ++i)
    {
        char line[32];
        snprintf(line, sizeof(line), "  BLK%-2u  %s",
                 i, d.blockIsEmpty[i] ? "Empty" : "Written");
        fmt.println(line);
    }
}
