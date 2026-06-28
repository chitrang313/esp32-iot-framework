#include "GPIOInfo.h"
#include <driver/rtc_io.h>
#include <soc/soc_caps.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Chip-specific pin tables
// Each table terminated by 0xFF sentinel.
// ---------------------------------------------------------------------------

#if defined(CONFIG_IDF_TARGET_ESP32)

// ADC1: GPIO36..39,32..35
static const uint8_t s_adc1[] = {36,37,38,39,32,33,34,35, 0xFF};
// ADC2: GPIO4,0,2,15,13,12,14,27,25,26
static const uint8_t s_adc2[] = {4,0,2,15,13,12,14,27,25,26, 0xFF};
// DAC: 25=CH1, 26=CH2
static const uint8_t s_dac[]  = {25,26, 0xFF};
// Touch: T0..T9
static const uint8_t s_touch[]= {4,0,2,15,13,12,14,27,32,33, 0xFF};
// Strapping pins
static const uint8_t s_strap[]= {0,2,5,12,15, 0xFF};
// SPI flash / PSRAM pins (avoid modifying)
static const uint8_t s_flash[]= {6,7,8,9,10,11,16,17, 0xFF};

#elif defined(CONFIG_IDF_TARGET_ESP32S2)

static const uint8_t s_adc1[] = {1,2,3,4,5,6,7,8,9,10, 0xFF};
static const uint8_t s_adc2[] = {11,12,13,14,15,16,17,18,19,20, 0xFF};
static const uint8_t s_dac[]  = {17,18, 0xFF};   // DAC_1=17, DAC_2=18
static const uint8_t s_touch[]= {1,2,3,4,5,6,7,8,9,10,11,12,13,14, 0xFF};
static const uint8_t s_strap[]= {0,45,46, 0xFF};
static const uint8_t s_flash[]= {26,27,28,29,30,31,32, 0xFF};

#elif defined(CONFIG_IDF_TARGET_ESP32S3)

static const uint8_t s_adc1[] = {1,2,3,4,5,6,7,8,9,10, 0xFF};
static const uint8_t s_adc2[] = {11,12,13,14,15,16,17,18,19,20, 0xFF};
static const uint8_t s_dac[]  = {0xFF};  // No DAC on S3
static const uint8_t s_touch[]= {1,2,3,4,5,6,7,8,9,10,11,12,13,14, 0xFF};
static const uint8_t s_strap[]= {0,3,45,46, 0xFF};
static const uint8_t s_flash[]= {27,28,29,30,31,32,33,34,35,36,37, 0xFF};

#elif defined(CONFIG_IDF_TARGET_ESP32C3)

static const uint8_t s_adc1[] = {0,1,2,3,4, 0xFF};
static const uint8_t s_adc2[] = {5, 0xFF};
static const uint8_t s_dac[]  = {0xFF};
static const uint8_t s_touch[]= {0xFF};  // No touch on C3
static const uint8_t s_strap[]= {2,8,9, 0xFF};
static const uint8_t s_flash[]= {11,12,13,14,15,16,17, 0xFF};

#elif defined(CONFIG_IDF_TARGET_ESP32C6)

static const uint8_t s_adc1[] = {0,1,2,3,4,5,6, 0xFF};
static const uint8_t s_adc2[] = {0xFF};
static const uint8_t s_dac[]  = {0xFF};
static const uint8_t s_touch[]= {0xFF};
static const uint8_t s_strap[]= {8,9, 0xFF};
static const uint8_t s_flash[]= {24,25,26,27,28,29,30, 0xFF};

#elif defined(CONFIG_IDF_TARGET_ESP32H2)

static const uint8_t s_adc1[] = {0,1,2,3,4, 0xFF};
static const uint8_t s_adc2[] = {0xFF};
static const uint8_t s_dac[]  = {0xFF};
static const uint8_t s_touch[]= {0xFF};
static const uint8_t s_strap[]= {8,9, 0xFF};
static const uint8_t s_flash[]= {15,16,17,18,19,20,21,22, 0xFF};

#else  // Unknown target — empty tables
static const uint8_t s_adc1[] = {0xFF};
static const uint8_t s_adc2[] = {0xFF};
static const uint8_t s_dac[]  = {0xFF};
static const uint8_t s_touch[]= {0xFF};
static const uint8_t s_strap[]= {0xFF};
static const uint8_t s_flash[]= {0xFF};
#endif

// ---------------------------------------------------------------------------
// Lookup helpers
// ---------------------------------------------------------------------------

static int8_t indexInTable(const uint8_t* tbl, uint8_t gpio)
{
    for (int8_t i = 0; tbl[i] != 0xFF; ++i)
        if (tbl[i] == gpio) return i;
    return -1;
}

int8_t GPIOInfo::getAdcUnit(uint8_t gpio)
{
    if (indexInTable(s_adc1, gpio) >= 0) return 1;
    if (indexInTable(s_adc2, gpio) >= 0) return 2;
    return -1;
}

int8_t GPIOInfo::getAdcChannel(uint8_t gpio)
{
    int8_t ch = indexInTable(s_adc1, gpio);
    if (ch >= 0) return ch;
    ch = indexInTable(s_adc2, gpio);
    return ch;
}

int8_t GPIOInfo::getDacChannel(uint8_t gpio)
{
    return indexInTable(s_dac, gpio);  // 0=CH1, 1=CH2; -1=none
}

int8_t GPIOInfo::getTouchChan(uint8_t gpio)
{
    return indexInTable(s_touch, gpio);
}

bool GPIOInfo::isStrapping(uint8_t gpio)
{
    return indexInTable(s_strap, gpio) >= 0;
}

bool GPIOInfo::isFlashPin(uint8_t gpio)
{
    return indexInTable(s_flash, gpio) >= 0;
}

// ---------------------------------------------------------------------------
// collect / print
// ---------------------------------------------------------------------------

void GPIOInfo::collect(GpioData& d)
{
    memset(&d, 0, sizeof(d));

    const int gpioMax = (int)SOC_GPIO_PIN_COUNT;
    const int limit   = (gpioMax < GPIO_DIAG_MAX) ? gpioMax : GPIO_DIAG_MAX;

    for (int g = 0; g < limit; ++g)
    {
        if (!GPIO_IS_VALID_GPIO(g)) continue;

        GpioEntry& e    = d.entries[d.count];
        e.num           = (uint8_t)g;
        e.validInput    = true;
        e.validOutput   = GPIO_IS_VALID_OUTPUT_GPIO(g) != 0;
        e.hasRtc        = rtc_gpio_is_valid_gpio((gpio_num_t)g);
        e.isStrapping   = isStrapping((uint8_t)g);
        e.isFlashPin    = isFlashPin((uint8_t)g);
        e.adcUnit       = getAdcUnit((uint8_t)g);
        e.adcChannel    = getAdcChannel((uint8_t)g);
        e.dacChannel    = getDacChannel((uint8_t)g);
        e.touchChannel  = getTouchChan((uint8_t)g);

        if (e.validInput)  ++d.inputCount;
        if (e.validOutput) ++d.outputCount;
        if (e.hasRtc)      ++d.rtcCount;
        if (e.adcUnit > 0) ++d.adcCount;
        if (e.dacChannel >= 0) ++d.dacCount;
        if (e.touchChannel >= 0) ++d.touchCount;

        ++d.count;
    }
}

void GPIOInfo::print(DiagFormatter& fmt)
{
    GpioData d;
    collect(d);

    fmt.sectionHeader("GPIO INFORMATION");

    {
        char buf[8];
        snprintf(buf, sizeof(buf), "%u", d.count);
        fmt.kv("GPIO pins (valid input)", buf);
    }
    fmt.kv("Output-capable",  (uint32_t)d.outputCount);
    fmt.kv("RTC-capable",     (uint32_t)d.rtcCount);
    fmt.kv("ADC channels",    (uint32_t)d.adcCount);
    fmt.kv("DAC channels",    (uint32_t)d.dacCount);
    fmt.kv("Touch channels",  (uint32_t)d.touchCount);

    fmt.blankLine();
    fmt.println("  GPIO  IN  OUT  RTC  ADC      DAC  Touch  Flags");
    fmt.println("  ----  --  ---  ---  -------  ---  -----  --------");

    for (uint8_t i = 0; i < d.count; ++i)
    {
        const GpioEntry& e = d.entries[i];

        char adcBuf[8] = "-      ";
        if (e.adcUnit > 0)
            snprintf(adcBuf, sizeof(adcBuf), "A%d:C%-2d", e.adcUnit, e.adcChannel);

        char dacBuf[4] = "-  ";
        if (e.dacChannel >= 0)
            snprintf(dacBuf, sizeof(dacBuf), "D%d ", e.dacChannel + 1);

        char tBuf[4] = "-  ";
        if (e.touchChannel >= 0)
            snprintf(tBuf, sizeof(tBuf), "T%-2d", e.touchChannel);

        char flags[16] = "";
        if (e.isStrapping) strncat(flags, "STRAP ",  sizeof(flags) - strlen(flags) - 1);
        if (e.isFlashPin)  strncat(flags, "FLASH",   sizeof(flags) - strlen(flags) - 1);

        char line[DIAG_LINE_WIDTH + 4];
        snprintf(line, sizeof(line),
                 "  %4u  %-2s  %-3s  %-3s  %-7s  %-3s  %-5s  %s",
                 e.num,
                 e.validInput  ? "Y" : "-",
                 e.validOutput ? "Y" : "-",
                 e.hasRtc      ? "Y" : "-",
                 adcBuf,
                 dacBuf,
                 tBuf,
                 flags[0] ? flags : "-");
        fmt.println(line);
    }
}
