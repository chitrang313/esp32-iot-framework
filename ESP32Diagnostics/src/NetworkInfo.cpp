#include "NetworkInfo.h"
#include <esp_mac.h>
#include <string.h>

#if SOC_WIFI_SUPPORTED
  #include <WiFi.h>
  #include <esp_wifi.h>
#endif

#if SOC_BT_SUPPORTED
  #include <esp_bt.h>
#endif

void NetworkInfo::collect(NetworkData& d)
{
    memset(&d, 0, sizeof(d));

    // Read base MAC once; all interface MACs are derived from it.
    uint8_t baseMac[6];
    esp_efuse_mac_get_default(baseMac);

#if SOC_WIFI_SUPPORTED
    d.wifiSupported = true;
    d.wifiEnabled   = (WiFi.getMode() != WIFI_OFF);

    // STA uses base MAC; AP uses base MAC + 1 (per esp-idf convention).
    memcpy(d.staMac, baseMac, 6);
    memcpy(d.apMac,  baseMac, 6);
    d.apMac[5] += 1;

    if (d.wifiEnabled)
    {
        wifi_country_t country = {};
        if (esp_wifi_get_country(&country) == ESP_OK)
            strncpy(d.country, country.cc, sizeof(d.country) - 1);

        int8_t power = 0;
        if (esp_wifi_get_max_tx_power(&power) == ESP_OK)
            d.maxTxPowerdBm = power;
    }
    else
    {
        strncpy(d.country, "N/A", sizeof(d.country) - 1);
    }
#else
    d.wifiSupported = false;
    (void)baseMac;  // Suppress unused-variable warning on no-WiFi chips
#endif

#if SOC_BT_SUPPORTED
    d.btSupported = true;
    // BT MAC = base MAC + 2 (per esp-idf convention)
    memcpy(d.btMac, baseMac, 6);
    d.btMac[5] += 2;
    // Diagnostics doesn't start BT; report it as not actively enabled.
    d.btEnabled = false;
#else
    d.btSupported = false;
#endif

#if SOC_IEEE802154_SUPPORTED
    d.ieee154Supported = true;
#else
    d.ieee154Supported = false;
#endif
}

void NetworkInfo::print(DiagFormatter& fmt)
{
    NetworkData d;
    collect(d);

    fmt.sectionHeader("NETWORK INTERFACES");

    fmt.println("  [WiFi]");
    fmt.kv("Supported", d.wifiSupported);
    if (d.wifiSupported)
    {
        fmt.kv("Enabled",      d.wifiEnabled);
        fmt.kvMAC("STA MAC",   d.staMac);
        fmt.kvMAC("AP MAC",    d.apMac);
        fmt.kv("Country",      d.country[0] ? d.country : "N/A");
        if (d.wifiEnabled && d.maxTxPowerdBm != 0)
        {
            float txPow = d.maxTxPowerdBm / 4.0f;  // IDF unit = 0.25 dBm
            fmt.kv("Max TX Power", txPow, 2, "dBm");
        }
    }

    fmt.blankLine();
    fmt.println("  [Bluetooth]");
    fmt.kv("Supported", d.btSupported);
    if (d.btSupported)
        fmt.kvMAC("BT MAC", d.btMac);

    fmt.blankLine();
    fmt.println("  [IEEE 802.15.4 (Zigbee / Thread)]");
    fmt.kv("Supported", d.ieee154Supported);
}
