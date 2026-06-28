#pragma once
#include "Utility.h"

struct BuildData {
    char buildDate[16];      // __DATE__
    char buildTime[12];      // __TIME__
    char idfVersion[32];     // IDF version string
    char gccVersion[32];     // __VERSION__
    char arduinoVersion[16]; // Decoded from ARDUINO macro
    char targetChip[24];     // CONFIG_IDF_TARGET
};

class BuildInfo
{
public:
    static void collect(BuildData& d);
    static void print  (DiagFormatter& fmt);
};
