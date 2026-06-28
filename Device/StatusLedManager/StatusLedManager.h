#pragma once

// WS2812B / NeoPixel driver for a single RGB status LED on ESP32.
// All animation is non-blocking; call loop() from the Arduino loop().
// Application code should use setState() and let this class decide color,
// animation, and timing.  Direct RGB access is available but discouraged.

#include <Adafruit_NeoPixel.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// Enumerations
// Placed in the global namespace so callers write LedColor::Red, not
// StatusLedManager::LedColor::Red.  The enum class keyword prevents name
// collisions with identifiers from other modules.
// ---------------------------------------------------------------------------

enum class LedColor : uint8_t
{
    Off,
    Red,
    Green,
    Blue,
    White,
    Yellow,
    Orange,
    Purple,
    Cyan,
    Pink,
    Custom  // application supplied raw RGB via setColor(r, g, b)
};

enum class LedAnimation : uint8_t
{
    None,       // LED off, no cycling
    Solid,      // constant color at current brightness
    Blink,      // on/off at _animationSpeed interval
    SlowBlink,  // on/off at SLOW_BLINK_MS (default 1000 ms)
    FastBlink,  // on/off at FAST_BLINK_MS (default 150 ms)
    Breathe,    // smooth sinusoidal fade in/out, repeating
    Pulse,      // single breathe cycle then stops
    Flash,      // N quick on/off bursts then stops
    Rainbow     // continuous hue rotation
};

enum class StatusLedState : uint8_t
{
    Off,          // LED dark, no animation
    Booting,      // Blue Breathe
    Idle,         // LED dark (same as Off, but semantically "ready")
    Success,      // Green Solid
    Busy,         // Blue Breathe
    Warning,      // Orange Blink
    Error,        // Red FastBlink
    Pairing,      // Yellow Blink
    Updating,     // Purple Breathe
    FactoryReset, // White FastBlink
    Custom        // application controls color and animation directly
};

// ---------------------------------------------------------------------------
// StatusLedManager
// ---------------------------------------------------------------------------

class StatusLedManager
{
public:

    // gpioPin     - ESP32 GPIO connected to the WS2812B DIN line
    // pixelType   - Adafruit pixel order + frequency flags; default suits
    //               most WS2812B modules (NEO_GRB + NEO_KHZ800)
    explicit StatusLedManager(uint8_t gpioPin,
                              neoPixelType pixelType = NEO_GRB + NEO_KHZ800);

    // Must be called once from Arduino setup() before any other method.
    void begin();

    // Must be called from every Arduino loop() iteration.
    // Ticks the animation engine and pushes pixels to hardware only when
    // the output has actually changed.
    void loop();

    // -----------------------------------------------------------------------
    // Power control
    // -----------------------------------------------------------------------

    void on();                    // equivalent to setEnabled(true)
    void off();                   // equivalent to setEnabled(false)

    void setEnabled(bool enabled);
    bool isEnabled() const;

    // -----------------------------------------------------------------------
    // Brightness  (0 = off, 255 = maximum; default 50)
    // Applied on top of per-animation brightness scaling.
    // Takes effect on the next hardware update.
    // -----------------------------------------------------------------------

    void    setBrightness(uint8_t brightness);
    uint8_t getBrightness() const;

    // -----------------------------------------------------------------------
    // Color
    // Prefer the enum overload; raw RGB is intended for truly custom colors.
    // -----------------------------------------------------------------------

    void     setColor(LedColor color);
    void     setColor(uint8_t r, uint8_t g, uint8_t b);
    LedColor getColor() const;

    // -----------------------------------------------------------------------
    // Animation
    // -----------------------------------------------------------------------

    void         setAnimation(LedAnimation animation);
    LedAnimation getAnimation() const;

    // Override the speed for the currently-running animation.
    // For Blink variants: full toggle period in ms.
    // For Breathe/Pulse:  full cycle period in ms.
    // For Rainbow:        hue step interval in ms.
    void setAnimationSpeed(uint16_t ms);

    void stopAnimation();   // immediately darkens LED and sets animation to None

    // Convenience wrappers - equivalent to setAnimation() with a preset speed.
    void breathe();
    void pulse();           // one-shot breathe cycle, then stops
    void blink();
    void blink(uint16_t intervalMs);
    void flash(uint8_t count);
    void rainbow();

    // -----------------------------------------------------------------------
    // Device-state abstraction
    // Preferred API: the application describes intent; this class picks color
    // and animation according to the mapping table in StatusLedManager.cpp.
    // -----------------------------------------------------------------------

    void           setState(StatusLedState state);
    StatusLedState getState() const;

    // Push a temporary state without losing the active state.
    // Example: WiFi connected (Success) -> button press -> overrideState(Pairing)
    //          -> clearOverride() restores Success.
    void overrideState(StatusLedState state);
    void clearOverride();

private:

    // -----------------------------------------------------------------------
    // Internal types
    // -----------------------------------------------------------------------

    // Compact RGB representation used throughout the private layer.
    // The application always works with LedColor; raw RGB stays here.
    struct RgbColor
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    struct BlinkState
    {
        bool isOnPhase; // true = LED showing color, false = LED dark
    };

    struct BreatheState
    {
        uint8_t level;    // 0-255 brightness scale applied to _currentRgb
        bool    rising;   // true = level increasing, false = level decreasing
        bool    isPulse;  // true = stop after one complete cycle (level back to 0)
    };

    struct FlashState
    {
        uint8_t remainingFlashes;
        bool    isOnPhase;
    };

    struct RainbowState
    {
        uint8_t hue; // 0-255; wraps naturally
    };

    // All per-animation run-time state in one place.
    // Only the fields relevant to the active animation are valid at any time.
    struct AnimationState
    {
        BlinkState   blink;
        BreatheState breathe;
        FlashState   flash;
        RainbowState rainbow;
    };

    // -----------------------------------------------------------------------
    // Compile-time constants
    // -----------------------------------------------------------------------

    // Phase 1: one LED.  The architecture supports more; expand NUM_LEDS and
    // update the hardware write helpers when adding multi-LED support.
    static constexpr uint8_t NUM_LEDS = 1;

    static constexpr uint8_t  DEFAULT_BRIGHTNESS  = 50;

    // Blink toggle periods (full on+off cycle).
    static constexpr uint16_t BLINK_DEFAULT_MS    = 500;
    static constexpr uint16_t SLOW_BLINK_MS       = 1000;
    static constexpr uint16_t FAST_BLINK_MS       = 150;

    // Breathe: full rise+fall cycle period.
    static constexpr uint16_t BREATHE_DEFAULT_MS  = 3000;
    static constexpr uint16_t PULSE_DEFAULT_MS    = 800;

    // Rainbow: ms between each hue step.
    static constexpr uint16_t RAINBOW_STEP_MS     = 20;

    // Flash: on and off durations per burst.
    static constexpr uint16_t FLASH_ON_MS         = 80;
    static constexpr uint16_t FLASH_OFF_MS        = 120;

    // Breathe uses 512 discrete steps per full cycle (256 up + 256 down).
    static constexpr uint16_t BREATHE_STEPS       = 512;

    // -----------------------------------------------------------------------
    // Hardware
    // -----------------------------------------------------------------------

    Adafruit_NeoPixel _strip;

    // -----------------------------------------------------------------------
    // LED state
    // -----------------------------------------------------------------------

    bool     _enabled;
    uint8_t  _brightness;

    LedColor _color;        // named color currently selected
    RgbColor _currentRgb;   // resolved RGB for _color (or custom values)

    LedAnimation _animation;
    uint16_t     _animationSpeed; // ms; meaning depends on animation type

    // -----------------------------------------------------------------------
    // Device-state bookkeeping
    // -----------------------------------------------------------------------

    StatusLedState _currentState;
    StatusLedState _previousState; // restored by clearOverride()
    bool           _hasOverride;

    // -----------------------------------------------------------------------
    // Animation engine
    // -----------------------------------------------------------------------

    AnimationState _animState;
    uint32_t       _lastAnimationTick; // millis() timestamp of last step

    // true when hardware pixel data is stale and strip.show() is needed.
    bool _hardwareDirty;

    // -----------------------------------------------------------------------
    // Private methods - grouped by responsibility
    // -----------------------------------------------------------------------

    // State machine helpers
    void applyStateMapping(StatusLedState state);
    void enterAnimation(LedAnimation animation, uint16_t speedMs);
    void resetAnimationState();

    // Animation tick dispatch (called every loop when enabled)
    void tickAnimation(uint32_t nowMs);
    void tickBlink(uint32_t nowMs, uint16_t halfPeriodMs);
    void tickBreathe(uint32_t nowMs);
    void tickFlash(uint32_t nowMs);
    void tickRainbow(uint32_t nowMs);

    // Hardware output helpers (only these may call _strip methods)
    void applyToHardware();
    void writePixelColor(uint8_t r, uint8_t g, uint8_t b);
    void writePixelOff();

    // Pure utility functions
    static RgbColor resolveNamedColor(LedColor color);
    static RgbColor hueToRgb(uint8_t hue);
    static uint8_t  scaleBrightness(uint8_t value, uint8_t scale);
};
