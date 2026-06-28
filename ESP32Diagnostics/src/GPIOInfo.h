#pragma once
#include "Utility.h"
#include <driver/gpio.h>
#include <stdint.h>

static constexpr uint8_t GPIO_DIAG_MAX = 50;  // Covers all current ESP32 family

struct GpioEntry {
    uint8_t  num;
    bool     validInput;
    bool     validOutput;
    bool     hasRtc;
    bool     isStrapping;
    bool     isFlashPin;
    int8_t   adcUnit;      // 1 or 2; -1 = no ADC
    int8_t   adcChannel;   // ADC channel index; -1 = no ADC
    int8_t   dacChannel;   // DAC channel index; -1 = no DAC
    int8_t   touchChannel; // Touch channel index; -1 = no touch
};

struct GpioData {
    GpioEntry entries[GPIO_DIAG_MAX];
    uint8_t   count;        // Total GPIOs enumerated
    uint8_t   inputCount;
    uint8_t   outputCount;
    uint8_t   rtcCount;
    uint8_t   adcCount;
    uint8_t   dacCount;
    uint8_t   touchCount;
};

class GPIOInfo
{
public:
    static void collect(GpioData& d);
    static void print  (DiagFormatter& fmt);
private:
    static int8_t getAdcUnit   (uint8_t gpio);
    static int8_t getAdcChannel(uint8_t gpio);
    static int8_t getDacChannel(uint8_t gpio);
    static int8_t getTouchChan (uint8_t gpio);
    static bool   isStrapping  (uint8_t gpio);
    static bool   isFlashPin   (uint8_t gpio);
};
