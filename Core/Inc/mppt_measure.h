#ifndef MPPT_MEASURE_H
#define MPPT_MEASURE_H
#include "main.h"
/* Configuration follows the supplied schematic: A0=current, A1=Vin, A2=Vout. */
#define MPPT_ADS_ADDR_7BIT       0x48U
#define MPPT_ADS_CURRENT_CH     2U
#define MPPT_ADS_VIN_CH         1U
#define MPPT_ADS_VOUT_CH        0U
#define MPPT_VIN_RATIO          7.8f
#define MPPT_VOUT_RATIO         7.8f
/* Initial values from the ACS712 test project. Calibrate before using for control.
 * Sensitivity: 0.185 V/A only if the fitted sensor is the 5A variant.
 * Zero voltage must be measured with NO current through the sensor.
 * No automatic zero calibration is performed at startup. */
#define MPPT_CURRENT_ZERO_V     2.498f
//2,500 + 0,19 x 0,185 ~ 2,535V
//Khi do dong PV ~0A

#define MPPT_CURRENT_V_PER_A    0.185f
#define MPPT_CURRENT_SIGN       1.0f
#define MPPT_FILTER_ALPHA       0.2f

typedef struct {
    uint8_t valid;
    int16_t raw[3]; /* Same index as ADS channel A0/A1/A2. */
    float adc_V[3]; /* Unfiltered voltages at ADS inputs, for calibration. */
    float vin_V, vout_V, ipv_A, ppv_W;
    uint32_t updated_ms;
    uint32_t error_count;
} MPPT_Measurement;
void MPPT_Measure_Init(I2C_HandleTypeDef *bus);
/* Main context only. Conversion waiting is scheduled with HAL_GetTick;
 * each I2C transfer is blocking with a bounded 5ms HAL timeout. */
void MPPT_Measure_Task(void);
const MPPT_Measurement *MPPT_Measure_Get(void);
#endif
