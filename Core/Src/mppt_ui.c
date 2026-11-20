#include "mppt_ui.h"
#include "st7789.h"
#include "seven_segment.h"
#include <stdio.h>
#include <string.h>

#define M_BG     0x0863
#define M_PANEL  0x1106
#define M_TEXT   0xEF7E
#define M_MUTED  0x9DB8
#define M_LINE   0x326B
#define M_ACCENT 0x7F59
#define M_OFF    0xFBAA
#define M_PV       0xFDC8
#define M_BAT      0x6F31
#define M_PV_BG    0x18A1
#define M_BAT_BG   0x08C3
#define M_LOAD_BG  0x1245
#define M_FLASH_BG 0x1B4C
#define M_RED      0xF986
#define M_YELLOW   0xFFE0
#define STATUS_FLASH_MS 450U
#define WARNING_PHASE_MS 500U

static uint8_t frame_ready, cache_valid;
static MPPT_UI_Data previous;
static char previous_ip[20], previous_time[20];
static uint8_t wifi_flash, load_flash;
static uint32_t wifi_flash_started, load_flash_started;
static uint8_t warning_active, warning_phase, warning_dirty;
static uint32_t warning_started;

static void erase(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t bg)
{
    ST7789_DrawFilledRectangle(x, y, w, h, bg);
}

static void text(uint16_t x, uint16_t y, const char *s, uint16_t fg, uint16_t bg)
{
    ST7789_WriteString(x, y, s, Font_7x10, fg, bg);
}

static void value_right(uint16_t x, uint16_t y, uint16_t w, const char *s,
                        uint16_t bg, uint16_t fg)
{
    const FontDef *font = &Font_11x18;
    size_t n = strlen(s);
    erase(x, y, w, 23, bg);
    if (n * font->width > w) font = &Font_7x10;
    if (n * font->width > w) { s = "OVR"; n = 3; }
    ST7789_WriteString(x + w - (uint16_t)n * font->width,
                       y + (23U - font->height) / 2U, s, *font, fg, bg);
}

/* Label and value share one baseline, separated by only five pixels.
 * Clear the whole row so shorter values cannot leave old characters behind. */
static void metric_row(uint16_t panel_x, uint16_t y, char label, const char *s,
                       uint16_t bg, uint16_t label_color, uint16_t value_color)
{
    const FontDef *font = &Font_11x18;
    char prefix[3] = {label, ':', '\0'};
    size_t n = strlen(s);
    uint16_t x = panel_x + 10U;
    erase(panel_x + 8U, y, 135, 23, bg);
    if ((n + 2U) * font->width + 5U > 131U) font = &Font_7x10;
    if ((n + 2U) * font->width + 5U > 131U) s = "OVR";
    y += (23U - font->height) / 2U;
    ST7789_WriteString(x, y, prefix, *font, label_color, bg);
    ST7789_WriteString(x + 2U * font->width + 5U, y, s, *font, value_color, bg);
}

static void format_voltage(char *out, size_t n, uint32_t mv)
{
    (void)snprintf(out, n, "%lu.%lu V", (unsigned long)(mv / 1000U),
                   (unsigned long)((mv % 1000U) / 100U));
}

static void format_signed(char *out, size_t n, int32_t v, uint8_t decimals,
                          const char *unit)
{
    /* Unsigned subtraction safely handles INT32_MIN too. */
    uint32_t magnitude = v < 0 ? 0U - (uint32_t)v : (uint32_t)v;
    if (decimals == 2U)
        (void)snprintf(out, n, "%s%lu.%02lu %s", v < 0 ? "-" : "",
                       (unsigned long)(magnitude / 1000U),
                       (unsigned long)((magnitude % 1000U) / 10U), unit);
    else
        (void)snprintf(out, n, "%s%lu.%lu %s", v < 0 ? "-" : "",
                       (unsigned long)(magnitude / 1000U),
                       (unsigned long)((magnitude % 1000U) / 100U), unit);
}

static void draw_wifi(uint8_t online, uint8_t highlight)
{
    uint16_t bg = highlight ? M_FLASH_BG : M_BG;
    uint16_t fg = online ? M_BAT : M_MUTED;
    erase(6, 3, 116, 15, bg);
    ST7789_DrawFilledCircle(11, 10, 3, fg);
    text(20, 5, online ? "WiFi: ONLINE" : "WiFi: OFFLINE", fg, bg);
}

static void draw_load(uint8_t on, uint8_t highlight)
{
    uint16_t bg = highlight ? M_FLASH_BG : (on ? M_LOAD_BG : M_PANEL);
    uint16_t fg = on ? M_BAT : M_MUTED;
    erase(215, 74, 98, 21, bg);
    ST7789_DrawRoundRect(215, 74, 98, 21, 4, fg);
    text(236, 79, on ? "BUCK: ON" : "BUCK:OFF", fg, bg);
}

void MPPT_UI_Tick(void)
{
    uint32_t now;
    if (!frame_ready || !cache_valid) return;
    if (!wifi_flash && !load_flash && !warning_active && !warning_dirty) return;
    now = HAL_GetTick();
    if (wifi_flash && (uint32_t)(now - wifi_flash_started) >= STATUS_FLASH_MS) {
        wifi_flash = 0;
        draw_wifi(previous.wifi_connected, 0);
    }
    if (load_flash && (uint32_t)(now - load_flash_started) >= STATUS_FLASH_MS) {
        load_flash = 0;
        draw_load(previous.load_on, 0);
    }
    if (warning_active) {
        uint8_t phase = (uint8_t)(((uint32_t)(now - warning_started) /
                                   WARNING_PHASE_MS) & 1U);
        if (warning_dirty || phase != warning_phase) {
            erase(124, 3, 48, 15, M_BG);
            text(130, 5, "ALARM", phase ? M_YELLOW : M_RED, M_BG);
            warning_phase = phase;
            warning_dirty = 0;
        }
    } else if (warning_dirty) {
        erase(124, 3, 48, 15, M_BG);
        warning_dirty = 0;
    }
}

void MPPT_UI_SetWarning(uint8_t active)
{
    active = active ? 1U : 0U;
    if (active != warning_active) {
        warning_active = active;
        warning_dirty = 1;
        warning_phase = 0;
        warning_started = HAL_GetTick();
        MPPT_UI_Tick();
    }
}

void MPPT_UI_ShowSplash(uint32_t hold_ms)
{
    /* Startup decoration, called once before the control task starts. */
    frame_ready = cache_valid = 0;
    wifi_flash = load_flash = 0;
    ST7789_SetRotation(1);
    ST7789_Fill_Color(M_BG);
    ST7789_DrawFilledCircle(160, 41, 12, M_PV);
    ST7789_DrawCircle(160, 41, 17, M_PV);
    ST7789_WriteString(80, 78, "MPPT SOLAR", Font_16x26, M_ACCENT, M_BG);
    text(83, 115, "SOLAR CHARGE CONTROLLER", M_MUTED, M_BG);
    ST7789_DrawLine(54, 143, 265, 143, M_LINE);
    erase(54, 143, 106, 2, M_PV);
    erase(160, 143, 106, 2, M_BAT);
    text(121, 163, "STARTING...", M_TEXT, M_BG);
    HAL_Delay(hold_ms);
}

static void draw_energy(uint32_t wh)
{
    char s[16];
    uint16_t width = 0;
    const unsigned char *p;
    erase(0, 25, 204, 75, M_BG);
    text(8, 29, "TOTAL ENERGY", M_MUTED, M_BG);
    if (wh == MPPT_UI_U32_NA) {
        ST7789_WriteString(12, 58, "-- kWh", Font_16x26, M_MUTED, M_BG);
        return;
    }
    (void)snprintf(s, sizeof(s), "%lu.%03lu", (unsigned long)(wh / 1000U),
                   (unsigned long)(wh % 1000U));
    p = (const unsigned char *)s;
    while (*p) {
        width += SevenSegment28x48.glyph[*p - SevenSegment28x48.first].xAdvance;
        ++p;
    }
    if (width <= 194U) {
        uint16_t x = width <= 145U ? 10U : (uint16_t)(202U - width);
        ST7789_WriteGFXString((int16_t)x, 91, s, &SevenSegment28x48, M_ACCENT);
        if (width <= 145U) text(162, 82, "kWh", M_MUTED, M_BG);
        else text(170, 29, "kWh", M_MUTED, M_BG);
    } else {
        /* Larger lifetime totals use a smaller font so no digits disappear. */
        value_right(8, 53, 186, s, M_BG, M_ACCENT);
        text(170, 82, "kWh", M_MUTED, M_BG);
    }
}

/* 25x18 solar icon: a two-row panel plus a short mounting stand. */
static void draw_solar_icon(uint16_t x, uint16_t y)
{
    ST7789_DrawRectangle(x, y, x + 24U, y + 12U, M_PV);
    ST7789_DrawRectangle(x + 1U, y + 1U, x + 23U, y + 11U, M_PV);
    ST7789_DrawLine(x + 8U, y + 2U, x + 8U, y + 10U, M_PV);
    ST7789_DrawLine(x + 16U, y + 2U, x + 16U, y + 10U, M_PV);
    ST7789_DrawLine(x + 2U, y + 6U, x + 22U, y + 6U, M_PV);
    erase(x + 11U, y + 13U, 3, 4, M_PV);
    erase(x + 6U, y + 17U, 13, 1, M_PV);
}

/* 28x15 battery icon, with +/- rather than an invented charge percentage. */
static void draw_battery_icon(uint16_t x, uint16_t y)
{
    ST7789_DrawRectangle(x, y, x + 24U, y + 14U, M_BAT);
    ST7789_DrawRectangle(x + 1U, y + 1U, x + 23U, y + 13U, M_BAT);
    erase(x + 25U, y + 4U, 3, 7, M_BAT);
    erase(x + 5U, y + 7U, 7, 2, M_BAT);
    erase(x + 8U, y + 4U, 2, 8, M_BAT);
    erase(x + 16U, y + 7U, 5, 2, M_BAT);
}

void MPPT_UI_Init(void)
{
    ST7789_SetRotation(1);
    ST7789_Fill_Color(M_BG);
    ST7789_DrawLine(7, 22, 312, 22, M_LINE);
    ST7789_DrawLine(204, 29, 204, 97, M_LINE);
    text(216, 32, "PWM FREQ.", M_MUTED, M_BG);
    ST7789_DrawLine(7, 102, 312, 102, M_LINE);
    erase(6, 108, 149, 103, M_PV_BG);
    erase(6, 108, 3, 103, M_PV);
    erase(6, 108, 149, 2, M_PV);
    erase(165, 108, 149, 103, M_BAT_BG);
    erase(165, 108, 3, 103, M_BAT);
    erase(165, 108, 149, 2, M_BAT);
    draw_solar_icon(16, 113);
    draw_battery_icon(175, 114);
    ST7789_WriteString(50, 115, "SOLAR PV", Font_11x18, M_PV, M_PV_BG);
    ST7789_WriteString(212, 115, "BATTERY", Font_11x18, M_BAT, M_BAT_BG);
    ST7789_DrawLine(14, 135, 147, 135, M_PV);
    ST7789_DrawLine(173, 135, 306, 135, M_BAT);
    ST7789_DrawLine(7, 216, 312, 216, M_LINE);
    frame_ready = 1;
    cache_valid = 0;
    wifi_flash = load_flash = 0;
    warning_dirty = 1;
}

void MPPT_UI_Draw(const MPPT_UI_Data *d)
{
    char s[40], ip[20], time[20];
    uint8_t all;
    if (!d) return;
    if (!frame_ready || ST7789_WIDTH != 320U || ST7789_HEIGHT != 240U)
        MPPT_UI_Init();
    all = (uint8_t)!cache_valid;

    if (all || d->wifi_connected != previous.wifi_connected) {
        wifi_flash = (uint8_t)!all;
        wifi_flash_started = HAL_GetTick();
        draw_wifi(d->wifi_connected, wifi_flash);
    }
    (void)snprintf(ip, sizeof(ip), "IP: %.15s", d->ip_address ? d->ip_address : "--");
    if (all || strcmp(ip, previous_ip) != 0) {
        size_t len = strlen(ip);
        erase(174, 3, 140, 15, M_BG);
        text((uint16_t)(313U - len * 7U), 5, ip, M_MUTED, M_BG);
        (void)snprintf(previous_ip, sizeof(previous_ip), "%s", ip);
    }
    if (all || d->total_energy_Wh != previous.total_energy_Wh)
        draw_energy(d->total_energy_Wh);
    if (all || d->switching_frequency_Hz != previous.switching_frequency_Hz) {
        uint32_t hz = d->switching_frequency_Hz;
        if (hz == MPPT_UI_U32_NA)
            (void)snprintf(s, sizeof(s), "OFF");
        else if (hz >= 1000U)
            (void)snprintf(s, sizeof(s), "%lu.%lukHz", (unsigned long)(hz / 1000U),
                           (unsigned long)((hz % 1000U) / 100U));
        else
            (void)snprintf(s, sizeof(s), "%luHz", (unsigned long)hz);
        value_right(215, 47, 97, s, M_BG, M_TEXT);
    }
    if (all || d->load_on != previous.load_on) {
        load_flash = (uint8_t)!all;
        load_flash_started = HAL_GetTick();
        draw_load(d->load_on, load_flash);
    }
    if (all || d->pv_voltage_mV != previous.pv_voltage_mV) {
        if (d->pv_voltage_mV == MPPT_UI_U32_NA) (void)snprintf(s, sizeof(s), "-- V");
        else format_voltage(s, sizeof(s), d->pv_voltage_mV);
        metric_row(6, 139, 'U', s, M_PV_BG, M_PV, M_TEXT);
    }
    if (all || d->pv_current_mA != previous.pv_current_mA) {
        if (d->pv_current_mA == MPPT_UI_I32_NA) (void)snprintf(s, sizeof(s), "-- A");
        else format_signed(s, sizeof(s), d->pv_current_mA, 2, "A");
        metric_row(6, 163, 'I', s, M_PV_BG, M_PV, M_TEXT);
    }
    if (all || d->pv_power_mW != previous.pv_power_mW) {
        if (d->pv_power_mW == MPPT_UI_I32_NA) (void)snprintf(s, sizeof(s), "-- W");
        else format_signed(s, sizeof(s), d->pv_power_mW, 1, "W");
        metric_row(6, 187, 'P', s, M_PV_BG, M_PV, M_PV);
    }
    if (all || d->battery_voltage_mV != previous.battery_voltage_mV) {
        if (d->battery_voltage_mV == MPPT_UI_U32_NA) (void)snprintf(s, sizeof(s), "-- V");
        else format_voltage(s, sizeof(s), d->battery_voltage_mV);
        metric_row(165, 139, 'U', s, M_BAT_BG, M_BAT, M_TEXT);
    }
    if (all || d->battery_current_mA != previous.battery_current_mA) {
        if (d->battery_current_mA == MPPT_UI_I32_NA) (void)snprintf(s, sizeof(s), "-- A");
        else format_signed(s, sizeof(s), d->battery_current_mA, 2, "A");
        metric_row(165, 163, 'I', s, M_BAT_BG, M_BAT, M_TEXT);
    }
    if (all || d->battery_power_mW != previous.battery_power_mW) {
        if (d->battery_power_mW == MPPT_UI_I32_NA) (void)snprintf(s, sizeof(s), "-- W");
        else format_signed(s, sizeof(s), d->battery_power_mW, 1, "W");
        metric_row(165, 187, 'P', s, M_BAT_BG, M_BAT, M_BAT);
    }
    (void)snprintf(time, sizeof(time), "%.19s", d->date_time ? d->date_time : "--/--/---- --:--:--");
    if (all || strcmp(time, previous_time) != 0) {
        erase(6, 218, 211, 21, M_BG);
        ST7789_WriteString(7, 219, time, Font_11x18, M_TEXT, M_BG);
        (void)snprintf(previous_time, sizeof(previous_time), "%s", time);
    }
    if (all || d->demo != previous.demo) {
        erase(277, 220, 37, 16, M_BG);
        if (d->demo) text(282, 224, "DEMO", M_MUTED, M_BG);
    }
    previous = *d;
    cache_valid = 1;
    MPPT_UI_Tick();
}

void MPPT_UI_DrawDemo(void)
{
    static const MPPT_UI_Data sample = {
        1234,                    /* 1.234 kWh */
        18600, 6450, 120000,      /* PV: 18.6V, 6.45A, 120.0W */
        14200, 7900, 112200,      /* Battery: 14.2V, 7.90A, 112.2W */
        20000,                   /* Sample switching frequency: 20.0kHz */
        1, 1, 1,                 /* Sample WiFi online, load on, demo */
        "192.168.1.105", "04/11/2026 12:30:00"
    };
    MPPT_UI_Draw(&sample);
}
