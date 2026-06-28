#include "Utility.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

static void printRepeat(Print& out, char c, uint8_t n)
{
    for (uint8_t i = 0; i < n; ++i) out.print(c);
}

static void centerText(Print& out, const char* text, uint8_t width)
{
    const uint8_t len    = (uint8_t)strlen(text);
    const uint8_t pad    = (len < width) ? (width - len) / 2 : 0;
    const uint8_t rpad   = (len < width) ? (width - len - pad) : 0;
    printRepeat(out, ' ', pad);
    out.print(text);
    printRepeat(out, ' ', rpad);
}

// ---------------------------------------------------------------------------
// Headers / footers
// ---------------------------------------------------------------------------

void DiagFormatter::reportHeader(const char* title)
{
    blankLine();
    printRepeat(m_out, '=', DIAG_LINE_WIDTH);
    m_out.println();
    centerText(m_out, title, DIAG_LINE_WIDTH);
    m_out.println();
    printRepeat(m_out, '=', DIAG_LINE_WIDTH);
    m_out.println();
}

void DiagFormatter::reportFooter()
{
    printRepeat(m_out, '=', DIAG_LINE_WIDTH);
    m_out.println();
    blankLine();
}

void DiagFormatter::sectionHeader(const char* title)
{
    blankLine();
    printRepeat(m_out, '-', DIAG_LINE_WIDTH);
    m_out.println();

    char buf[DIAG_LINE_WIDTH + 1];
    snprintf(buf, sizeof(buf), "  %s", title);
    m_out.println(buf);

    printRepeat(m_out, '-', DIAG_LINE_WIDTH);
    m_out.println();
}

void DiagFormatter::separator()
{
    printRepeat(m_out, '-', DIAG_LINE_WIDTH);
    m_out.println();
}

void DiagFormatter::blankLine()
{
    m_out.println();
}

// ---------------------------------------------------------------------------
// Key-value printing
// ---------------------------------------------------------------------------

void DiagFormatter::printKey(const char* key)
{
    char buf[DIAG_KEY_WIDTH + 6];
    snprintf(buf, sizeof(buf), "  %-*s : ", DIAG_KEY_WIDTH, key);
    m_out.print(buf);
}

void DiagFormatter::kv(const char* key, const char* value)
{
    printKey(key);
    m_out.println(value ? value : "(null)");
}

void DiagFormatter::kv(const char* key, uint32_t value, const char* unit)
{
    printKey(key);
    char buf[32];
    if (unit && unit[0])
        snprintf(buf, sizeof(buf), "%lu %s", (unsigned long)value, unit);
    else
        snprintf(buf, sizeof(buf), "%lu", (unsigned long)value);
    m_out.println(buf);
}

void DiagFormatter::kv(const char* key, uint64_t value, const char* unit)
{
    printKey(key);
    // Split into high/low for snprintf on 32-bit targets.
    char buf[48];
    if (value < 0xFFFFFFFFULL)
    {
        snprintf(buf, sizeof(buf), "%lu%s%s",
                 (unsigned long)value,
                 (unit && unit[0]) ? " " : "",
                 (unit && unit[0]) ? unit : "");
    }
    else
    {
        uint32_t hi = (uint32_t)(value >> 32);
        uint32_t lo = (uint32_t)(value & 0xFFFFFFFFULL);
        snprintf(buf, sizeof(buf), "0x%08lX%08lX", (unsigned long)hi, (unsigned long)lo);
    }
    m_out.println(buf);
}

void DiagFormatter::kv(const char* key, int32_t value, const char* unit)
{
    printKey(key);
    char buf[32];
    if (unit && unit[0])
        snprintf(buf, sizeof(buf), "%ld %s", (long)value, unit);
    else
        snprintf(buf, sizeof(buf), "%ld", (long)value);
    m_out.println(buf);
}

void DiagFormatter::kv(const char* key, float value, uint8_t decimals, const char* unit)
{
    printKey(key);
    char buf[32];
    // dtostrf is available on Arduino without libc printf float support.
    char num[24];
    dtostrf(value, 1, decimals, num);
    if (unit && unit[0])
        snprintf(buf, sizeof(buf), "%s %s", num, unit);
    else
        snprintf(buf, sizeof(buf), "%s", num);
    m_out.println(buf);
}

void DiagFormatter::kv(const char* key, bool value,
                        const char* trueStr, const char* falseStr)
{
    printKey(key);
    m_out.println(value ? trueStr : falseStr);
}

void DiagFormatter::kvHex32(const char* key, uint32_t value)
{
    printKey(key);
    char buf[12];
    snprintf(buf, sizeof(buf), "0x%08lX", (unsigned long)value);
    m_out.println(buf);
}

void DiagFormatter::kvHex64(const char* key, uint64_t value)
{
    printKey(key);
    char buf[20];
    uint32_t hi = (uint32_t)(value >> 32);
    uint32_t lo = (uint32_t)(value & 0xFFFFFFFFULL);
    snprintf(buf, sizeof(buf), "0x%08lX%08lX", (unsigned long)hi, (unsigned long)lo);
    m_out.println(buf);
}

void DiagFormatter::kvMAC(const char* key, const uint8_t mac[6])
{
    printKey(key);
    char buf[20];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    m_out.println(buf);
}

void DiagFormatter::kvBytes(const char* key, uint32_t bytes)
{
    printKey(key);
    char buf[32];
    if (bytes >= (1024UL * 1024UL))
        snprintf(buf, sizeof(buf), "%lu MB (%lu bytes)",
                 (unsigned long)(bytes / (1024UL * 1024UL)), (unsigned long)bytes);
    else if (bytes >= 1024UL)
        snprintf(buf, sizeof(buf), "%lu KB (%lu bytes)",
                 (unsigned long)(bytes / 1024UL), (unsigned long)bytes);
    else
        snprintf(buf, sizeof(buf), "%lu bytes", (unsigned long)bytes);
    m_out.println(buf);
}

// ---------------------------------------------------------------------------
// Raw output
// ---------------------------------------------------------------------------

void DiagFormatter::println(const char* str)
{
    m_out.println(str ? str : "");
}

void DiagFormatter::printf(const char* fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    m_out.print(buf);
}
