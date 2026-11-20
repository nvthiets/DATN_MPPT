#include "mppt_measure.h"
#include <string.h>

#define ADS_ADDRESS ((uint16_t)(MPPT_ADS_ADDR_7BIT << 1))
#define IO_TIMEOUT_MS 5U
#define CONVERSION_WAIT_MS 9U
#define CONVERSION_TIMEOUT_MS 30U
#define RETRY_MS 250U
#define STALE_MS 500U
static I2C_HandleTypeDef *bus;
static MPPT_Measurement result;
static int16_t pending_raw[3];
static uint8_t phase, channel, filter_ready;
static uint32_t started, retry_started;

static HAL_StatusTypeDef read_register(uint8_t reg, uint16_t *v)
{
    uint8_t b[2];
    HAL_StatusTypeDef s = HAL_I2C_Mem_Read(bus, ADS_ADDRESS, reg,
        I2C_MEMADD_SIZE_8BIT, b, 2, IO_TIMEOUT_MS);
    if (s == HAL_OK) *v = ((uint16_t)b[0] << 8) | b[1];
    return s;
}
static HAL_StatusTypeDef start_conversion(uint8_t ch)
{
    /* ADS1115 (not ADS1015): single-ended MUX, +/-6.144V PGA,
       single-shot, 128 SPS (DR=100), comparator disabled. */
    uint16_t config = (uint16_t)(0x8000U | ((4U + ch) << 12) |
                                 0x0100U | 0x0080U | 0x0003U);
    uint8_t b[2] = {(uint8_t)(config >> 8), (uint8_t)config};
    return HAL_I2C_Mem_Write(bus, ADS_ADDRESS, 1, I2C_MEMADD_SIZE_8BIT,
                              b, 2, IO_TIMEOUT_MS);
}
static void fail(void)
{
    result.valid = 0;
    ++result.error_count;
    filter_ready = 0;
    phase = 2;
    channel = 0;
    retry_started = HAL_GetTick();
}
static void publish(void)
{
    float v[3], vin, vout, current;
    unsigned i;
    for (i = 0; i < 3; ++i) {
        result.raw[i] = pending_raw[i];
        v[i] = (float)pending_raw[i] * (6.144f / 32768.0f);
        result.adc_V[i] = v[i];
    }
    vin = v[MPPT_ADS_VIN_CH] * MPPT_VIN_RATIO;
    vout = v[MPPT_ADS_VOUT_CH] * MPPT_VOUT_RATIO;
    if (vin < 0.0f) vin = 0.0f;
    if (vout < 0.0f) vout = 0.0f;
    /* Current1 has an RC filter, NOT a divide-by-two network. */
    current = MPPT_CURRENT_SIGN * (v[MPPT_ADS_CURRENT_CH] -
                                  MPPT_CURRENT_ZERO_V) / MPPT_CURRENT_V_PER_A;
    if (!filter_ready) {
        result.vin_V = vin; result.vout_V = vout; result.ipv_A = current;
        filter_ready = 1;
    } else {
        result.vin_V += MPPT_FILTER_ALPHA * (vin - result.vin_V);
        result.vout_V += MPPT_FILTER_ALPHA * (vout - result.vout_V);
        result.ipv_A += MPPT_FILTER_ALPHA * (current - result.ipv_A);
    }
    result.ppv_W = result.vin_V * result.ipv_A;
    result.updated_ms = HAL_GetTick();
    result.valid = 1;
}
void MPPT_Measure_Init(I2C_HandleTypeDef *handle)
{
    bus = handle;
    memset(&result, 0, sizeof(result));
    phase = channel = filter_ready = 0;
}
const MPPT_Measurement *MPPT_Measure_Get(void) { return &result; }
void MPPT_Measure_Task(void)
{
    uint32_t now = HAL_GetTick();
    uint16_t config, raw;
    if (!bus) return;
    if (result.valid && (uint32_t)(now - result.updated_ms) > STALE_MS) {
        fail(); return;
    }
    if (phase == 2) {
        if ((uint32_t)(now - retry_started) < RETRY_MS) return;
        phase = 0;
    }
    if (phase == 0) {
        if (start_conversion(channel) != HAL_OK) { fail(); return; }
        started = HAL_GetTick();
        phase = 1;
        return;
    }
    if ((uint32_t)(now - started) < CONVERSION_WAIT_MS) return;
    if (read_register(1, &config) != HAL_OK) { fail(); return; }
    if (!(config & 0x8000U)) {
        if ((uint32_t)(HAL_GetTick() - started) > CONVERSION_TIMEOUT_MS) fail();
        return;
    }
    if (read_register(0, &raw) != HAL_OK) { fail(); return; }
    /* Reject ADC clipping; negative near-zero noise is allowed. */
    if ((int16_t)raw >= 32760 || (int16_t)raw <= -32760) { fail(); return; }
    pending_raw[channel] = (int16_t)raw;
    ++channel;
    if (channel >= 3) { publish(); channel = 0; }
    phase = 0;
}
