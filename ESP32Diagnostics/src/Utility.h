#pragma once
#include <Arduino.h>
#include <stdint.h>

static constexpr uint8_t DIAG_LINE_WIDTH = 72;
static constexpr uint8_t DIAG_KEY_WIDTH  = 30;

// Formatting helper shared by all diagnostic modules.
// Wraps a Print& so output can be Serial, Telnet, SD file, etc.
class DiagFormatter
{
public:
    explicit DiagFormatter(Print& out) : m_out(out) {}

    void reportHeader(const char* title);
    void reportFooter();
    void sectionHeader(const char* title);
    void separator();
    void blankLine();

    // Key-value overloads
    void kv(const char* key, const char* value);
    void kv(const char* key, uint32_t value, const char* unit = "");
    void kv(const char* key, uint64_t value, const char* unit = "");
    void kv(const char* key, int32_t  value, const char* unit = "");
    void kv(const char* key, float    value, uint8_t decimals = 2,
                                              const char* unit = "");
    void kv(const char* key, bool     value,
            const char* trueStr  = "Yes",
            const char* falseStr = "No");
    void kvHex32(const char* key, uint32_t value);
    void kvHex64(const char* key, uint64_t value);
    void kvMAC  (const char* key, const uint8_t mac[6]);
    void kvBytes(const char* key, uint32_t bytes);

    // Raw line output
    void println(const char* str);
    void printf (const char* fmt, ...);

    Print& stream() { return m_out; }

private:
    Print& m_out;
    void   printKey(const char* key);
};
