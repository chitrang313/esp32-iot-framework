#pragma once
#include "Utility.h"
#include <soc/soc_caps.h>
#include <stdint.h>

struct NetworkData {
    // WiFi (guarded by SOC_WIFI_SUPPORTED)
    bool    wifiSupported;
    bool    wifiEnabled;
    uint8_t staMac[6];
    uint8_t apMac[6];
    char    country[4];
    int8_t  maxTxPowerdBm;   // Raw IDF unit (0.25 dBm per unit)

    // Bluetooth
    bool btSupported;
    bool btEnabled;
    uint8_t btMac[6];

    // IEEE 802.15.4 (Zigbee / Thread)
    bool ieee154Supported;
};

class NetworkInfo
{
public:
    static void collect(NetworkData& d);
    static void print  (DiagFormatter& fmt);
};
