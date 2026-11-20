#ifndef MPPT_UI_H
#define MPPT_UI_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Fixed-point values: no printf float support needed.
 * 1234 Wh = 1.234 kWh; 18600 mV = 18.6 V; 6450 mA = 6.45 A;
 * 120000 mW = 120.0 W; 20000 Hz = 20.0 kHz.
 * Battery current/power may be negative. Choose the sign convention in
 * your measurement code; the UI prints the sign without inferring charge mode.
 * String pointers must be valid during Draw; NULL is allowed.
 * date_time: "04/11/2026 12:30:00"; ip_address: "192.168.1.105". */
/* Unmeasured/invalid values render as -- rather than an invented zero. */
#define MPPT_UI_U32_NA UINT32_MAX
#define MPPT_UI_I32_NA INT32_MIN

typedef struct {
    uint32_t total_energy_Wh;
    uint32_t pv_voltage_mV;
    int32_t pv_current_mA;
    int32_t pv_power_mW;
    uint32_t battery_voltage_mV;
    int32_t battery_current_mA;
    int32_t battery_power_mW;
    uint32_t switching_frequency_Hz;
    uint8_t wifi_connected;
    uint8_t load_on;
    uint8_t demo;
    const char *ip_address;
    const char *date_time;
} MPPT_UI_Data;

/* All functions run in normal thread/main context, not an ISR.
 * Use with the provided STM32 HAL ST7789 driver, fonts and seven_segment.c.
 * Call ST7789_Init() first. Init sets landscape rotation 1 and draws the frame.
 * Init is optional before the first Draw: Draw initializes the frame if needed.
 * Call Init again if another screen overwrites this UI. */
void MPPT_UI_Init(void);
/* Redraws only changed fields. Same data twice produces no SPI traffic
 * when status animations are idle. Draw also services pending effects.
 * Font rendering is blocking; this is not a real-time control-loop task. */
void MPPT_UI_Draw(const MPPT_UI_Data *data);
/* Static sample layout, with a visible DEMO label, no hardware measurements. */
void MPPT_UI_DrawDemo(void);
/* Optional one-second splash example: ShowSplash(1000), after ST7789_Init().
 * This function uses HAL_Delay; call only at startup before timed control work. */
void MPPT_UI_ShowSplash(uint32_t hold_ms);
/* Call often in while(1) to finish status highlights and animate ALARM.
 * No HAL_Delay, no full-screen redraw. Painting still uses blocking SPI.
 * Uses HAL_GetTick; SysTick must be running. */
void MPPT_UI_Tick(void);
/* Manual application alarm flag, independent of the data structure.
 * No guessed voltage/current thresholds and no load-control action.
 * ALARM alternates red/yellow every 500ms; 0 clears it. Main context only. */
void MPPT_UI_SetWarning(uint8_t active);
#ifdef __cplusplus
}
#endif
#endif
