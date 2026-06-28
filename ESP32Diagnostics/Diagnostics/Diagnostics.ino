// ESP32 Professional Diagnostics — full report example.
// Prints a complete hardware/firmware report once at startup.
// Baud rate: 115200.  Open Serial Monitor to view.
//
// Optional: Hold GPIO0 LOW at boot to repeat report on demand.

#include <ESP32Diagnostics.h>

ESP32Diagnostics diag;

static constexpr uint8_t TRIGGER_PIN    = 0;     // Strapping pin (boot button on most boards)
static constexpr uint32_t DEBOUNCE_MS   = 200;
static uint32_t s_lastTrigger           = 0;

void setup()
{
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {}  // Wait for USB CDC on native-USB boards

    // Run full report once at startup
    diag.begin(Serial);
    diag.printAll();

    // Example: only memory + FreeRTOS on demand
    // diag.printSections(DIAG_MEMORY | DIAG_FREERTOS);

    pinMode(TRIGGER_PIN, INPUT_PULLUP);
}

void loop()
{
    // Re-run on button press (GPIO0 / BOOT button)
    if (digitalRead(TRIGGER_PIN) == LOW)
    {
        const uint32_t now = millis();
        if ((int32_t)(now - s_lastTrigger) > DEBOUNCE_MS)
        {
            s_lastTrigger = now;
            diag.printAll();
        }
    }
}
