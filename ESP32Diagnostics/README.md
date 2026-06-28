# ESP32 Professional Diagnostics Utility

Structured hardware and firmware report for the ESP32 family.  
Compatible with **Arduino ESP32 Core 3.x (ESP-IDF 5.x)**.  
Supports: ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6, ESP32-H2.

---

## Features

| Section | What is reported |
|---|---|
| Build Info | Build date/time, IDF version, GCC version, Arduino core version, target chip |
| Chip | Model, revision, cores, CPU/XTAL frequency, chip ID, base MAC, feature flags, reset reason |
| Flash | Size, speed, mode (QIO/DIO/etc.), JEDEC manufacturer ID |
| Partitions | Full partition table with type, subtype, offset, size, active/boot/encrypted flags |
| Memory | Internal heap (total/used/free/watermark/fragmentation), DMA, IRAM, PSRAM |
| eFuse | Base MAC, flash encryption status, secure boot status, per-block empty/written status |
| Network | WiFi STA/AP MAC, country, TX power; Bluetooth MAC; 802.15.4 presence |
| FreeRTOS | Scheduler state, tick rate, uptime, task count, heap, task list |
| GPIO | Per-pin: input/output capability, RTC, ADC unit/channel, DAC channel, touch channel, flags |

---

## Usage

```cpp
#include <ESP32Diagnostics.h>

ESP32Diagnostics diag;

void setup() {
    Serial.begin(115200);
    diag.begin(Serial);
    diag.printAll();            // Full report
}
```

### Selective sections

```cpp
// Only memory and FreeRTOS
diag.printSections(DIAG_MEMORY | DIAG_FREERTOS);

// Single section
diag.printChip();
diag.printMemory();
diag.printFlash();
diag.printPartitions();
diag.printEfuse();
diag.printGPIO();
diag.printNetwork();
diag.printFreeRTOS();
diag.printBuild();
```

### Custom output stream

```cpp
// Output to any Print-compatible stream (Telnet, SD file, etc.)
WiFiClient client;
diag.begin(client);
diag.printAll();
```

---

## Section bitmask constants

| Constant | Hex |
|---|---|
| `DIAG_CHIP` | `0x001` |
| `DIAG_MEMORY` | `0x002` |
| `DIAG_FLASH` | `0x004` |
| `DIAG_PARTITIONS` | `0x008` |
| `DIAG_EFUSE` | `0x010` |
| `DIAG_GPIO` | `0x020` |
| `DIAG_NETWORK` | `0x040` |
| `DIAG_FREERTOS` | `0x080` |
| `DIAG_BUILD` | `0x100` |
| `DIAG_ALL` | `0xFFFF` |

---

## Project structure

```
ESP32Diagnostics/
├── library.properties
├── README.md
├── example/
│   └── Diagnostics.ino     — Full report + on-demand button trigger
└── src/
    ├── ESP32Diagnostics.h  — Main class, section bitmask constants
    ├── ESP32Diagnostics.cpp
    ├── Utility.h           — DiagFormatter (Print& wrapper)
    ├── Utility.cpp
    ├── ChipInfo.h/.cpp     — Chip model, revision, features, reset reason
    ├── MemoryInfo.h/.cpp   — Heap, DMA, IRAM, PSRAM
    ├── FlashInfo.h/.cpp    — Flash size, speed, mode, JEDEC ID
    ├── PartitionInfo.h/.cpp— Full partition table
    ├── EfuseInfo.h/.cpp    — eFuse blocks, flash encryption, secure boot
    ├── GPIOInfo.h/.cpp     — Per-pin capability table
    ├── NetworkInfo.h/.cpp  — WiFi/BT MACs, 802.15.4
    ├── FreeRTOSInfo.h/.cpp — Scheduler, task count, task list
    └── BuildInfo.h/.cpp    — Build metadata
```

---

## Sample output (ESP32-S3, 16 MB flash)

```
========================================================================
               ESP32 DIAGNOSTICS REPORT  [ESP32-S3]
========================================================================

------------------------------------------------------------------------
  BUILD INFORMATION
------------------------------------------------------------------------
  Build Date                     : Jun 28 2026
  Build Time                     : 14:23:01
  IDF Version                    : v5.1.2
  GCC Version                    : 12.2.0
  Arduino Core                   : 3.3.10
  Target Chip                    : esp32s3

------------------------------------------------------------------------
  CHIP INFORMATION
------------------------------------------------------------------------
  Model                          : ESP32-S3
  Revision                       : 0
  CPU Cores                      : 2
  CPU Frequency                  : 240 MHz
  XTAL Frequency                 : 40 MHz
  Chip ID (eFuse)                : 0x6921C4B4A8DC1234
  Base MAC                       : 68:B6:B3:2A:11:44
  WiFi                           : Yes
  Bluetooth Classic              : No
  Bluetooth LE                   : Yes
  IEEE 802.15.4                  : No
  Embedded Flash                 : No
  Embedded PSRAM                 : No
  Temp. Sensor                   : Yes
  Reset Reason                   : Power-on
...
```

---

## Notes

- **No dynamic allocation**: `DiagFormatter` uses placement new into a static buffer; all module data structs are stack-allocated.
- **GPIO task list**: `vTaskList()` requires `configUSE_TRACE_FACILITY=1` and `configUSE_STATS_FORMATTING_FUNCTIONS=1` (both enabled by default in Arduino ESP32 Core 3.x).
- **WiFi MAC**: Reads from eFuse base MAC + standard offset; does not require WiFi to be started.
- **Flash encryption / secure boot**: Flash encryption detected at runtime. Secure boot detected at compile time via `CONFIG_SECURE_BOOT*` macros (a build-time property, not run-time).
