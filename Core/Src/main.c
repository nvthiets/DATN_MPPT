/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "st7789.h"
#include "mppt_ui.h"
#include "mppt_measure.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* BENCH ONLY: 12 V input, NO battery or other source at Vout.
 * VCC driver is NOT measured; ADS checks are NOT short-circuit protection.
 * Keep the user's calibrated mppt_measure files unchanged.
 */
#define BUCK_POWER_WAIT_MS       2000U
#define BUCK_VIN_STABLE_MS        500U
#define BUCK_RELAY_WAIT_MS        200U
#define BUCK_RELAY_STABLE_MS      500U
#define BUCK_RELAY_TIMEOUT_MS    2000U
#define BUCK_ADS_TIMEOUT_MS      5000U
#define BUCK_DATA_MAX_AGE_MS      150U
#define BUCK_RAMP_STEP_MS          20U
#define BUCK_VIN_START_MIN_V     11.5f
#define BUCK_VIN_START_MAX_V     12.5f
#define BUCK_VIN_RUN_MIN_V       11.0f
#define BUCK_VIN_RUN_MAX_V       13.0f
#define BUCK_VIN_DRIFT_MAX_V      0.3f
#define BUCK_VOUT_BEFORE_ON_V     0.2f
#define BUCK_VOUT_MAX_V           2.0f
#define BUCK_CURRENT_LIMIT_A     0.3f
/* Fixed zero offsets measured with NO fan/load, SD LOW, on this board at 12 V.
 * Do not learn/adapt these values while the charger is operating.
 * mppt_measure.c still reports current relative to MPPT_CURRENT_ZERO_V;
 * this main applies the matching correction to control AND display.
 */
#define BUCK_ACS_ZERO_OFF_V       2.499f
#define BUCK_ACS_ZERO_ON_V        2.419f
#define BUCK_PWM_HZ             50000U
#define BUCK_INITIAL_CCR           72U /* 5% of 1440 */
#define BUCK_TARGET_CCR           144U /* 10%, NOT MPPT */
#define BUCK_CCR_STEP              14U
#if MPPT_ADS_CURRENT_CH != 2U || MPPT_ADS_VIN_CH != 1U || MPPT_ADS_VOUT_CH != 0U
#error "Check mppt_measure.h: A0=Vout, A1=Vin, A2=ACS712 required."
#endif
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */
typedef enum {
    BUCK_WAIT_POWER = 0, BUCK_RELAY_SETTLE,
    BUCK_RAMP, BUCK_RUNNING, BUCK_FAULT
} BuckState;
static BuckState buck_state = BUCK_WAIT_POWER;
static const char *buck_message = "WAIT POWER | OFF";
static uint32_t boot_ms, state_ms, stable_ms, ramp_ms, last_ads_errors;
static uint16_t buck_ccr;
static uint8_t vin_stable, ads_seen;
static float vin_reference;
static uint32_t relay_sample_ms, relay_good_ms;
static uint8_t relay_good;
static float relay_vin_reference;
static uint8_t relay_commanded_on;
/* Fault evidence is captured before measurements recover after shutdown. */
static MPPT_Measurement fault_measure;
static uint8_t fault_measure_valid;
static BuckState fault_stage;
static uint16_t fault_ccr;
static uint32_t fault_ms;
static char fault_message[40];
static MPPT_UI_Data screen;
static uint32_t last_screen_ms;
/* Debugger Watch: measure->adc_V[], vin_V, vout_V, ipv_A, error_count. */
const MPPT_Measurement *measure;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static float Buck_CurrentForRelay(float library_current, uint8_t relay_on)
{
    float zero = relay_on ? BUCK_ACS_ZERO_ON_V : BUCK_ACS_ZERO_OFF_V;
    /* Preserve measured load current; shift the fixed calibration origin only.
     * No clamp of negative values and no automatic zero-current assumption.
     */
    return library_current + MPPT_CURRENT_SIGN *
        (MPPT_CURRENT_ZERO_V - zero) / MPPT_CURRENT_V_PER_A;
}
static float Buck_CurrentA(void)
{
    return Buck_CurrentForRelay(measure->ipv_A, relay_commanded_on);
}
/* SD first, then timer OFF / PA8 GPIO LOW, then relay OFF.
 * Also usable before TIM1 initialization, from Error_Handler.
 */
static void Power_Off(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_WritePin(BUCK_SD_GPIO_Port,
                     BUCK_SD_Pin, GPIO_PIN_RESET);
    if ((RCC->APB2ENR & RCC_APB2ENR_TIM1EN) != 0U) {
        TIM1->BDTR &= ~TIM_BDTR_MOE;
        TIM1->CCER &= ~TIM_CCER_CC1E;
        TIM1->CR1 &= ~TIM_CR1_CEN;
        TIM1->CCR1 = 0U;
    }
    HAL_GPIO_WritePin(BUCK_IN_GPIO_Port,
                     BUCK_IN_Pin, GPIO_PIN_RESET);
    gpio.Pin = BUCK_IN_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUCK_IN_GPIO_Port, &gpio);
    HAL_GPIO_WritePin(BCCU_GPIO_Port,
                     BCCU_Pin, GPIO_PIN_RESET);
    relay_commanded_on = 0U;
}
static uint8_t Buck_DataFresh(uint32_t now);
static void Buck_Fault(const char *reason)
{
    uint8_t relay_at_fault = relay_commanded_on;
    /* Shutdown has priority over diagnostic formatting/copies. */
    Power_Off();
    fault_stage = buck_state;
    fault_ccr = buck_ccr;
    fault_ms = HAL_GetTick();
    fault_measure_valid = Buck_DataFresh(fault_ms);
    if (measure != 0) {
        fault_measure = *measure;
        fault_measure.ipv_A = Buck_CurrentForRelay(measure->ipv_A, relay_at_fault);
        fault_measure.ppv_W = fault_measure.vin_V * fault_measure.ipv_A;
    }
    buck_ccr = 0U;
    buck_message = reason;
    buck_state = BUCK_FAULT; /* Latched until MCU reset/power cycle. */
}
static uint8_t Buck_DataFresh(uint32_t now)
{
    return (uint8_t)(measure != 0 && measure->valid &&
        (uint32_t)(now - measure->updated_ms) <= BUCK_DATA_MAX_AGE_MS);
}
static float Buck_RawVin(void)
{
    return measure->adc_V[MPPT_ADS_VIN_CH] * MPPT_VIN_RATIO;
}
static float Buck_RawVout(void)
{
    return measure->adc_V[MPPT_ADS_VOUT_CH] * MPPT_VOUT_RATIO;
}
static uint8_t Buck_CurrentReady(void)
{
    float zero = relay_commanded_on ? BUCK_ACS_ZERO_ON_V : BUCK_ACS_ZERO_OFF_V;
    float raw_current = MPPT_CURRENT_SIGN *
        (measure->adc_V[MPPT_ADS_CURRENT_CH] - zero) /
        MPPT_CURRENT_V_PER_A;
    float current = Buck_CurrentA();
    /* No auto-zero and no larger current limit. Both raw and filtered
     * measurements must qualify before allowing any PWM.
     */
    return (uint8_t)(raw_current >= -BUCK_CURRENT_LIMIT_A &&
        raw_current <= BUCK_CURRENT_LIMIT_A &&
        current >= -BUCK_CURRENT_LIMIT_A &&
        current <= BUCK_CURRENT_LIMIT_A);
}
static uint8_t Buck_VinStartOK(void)
{
    float v = Buck_RawVin();
    return (uint8_t)(v >= BUCK_VIN_START_MIN_V && v <= BUCK_VIN_START_MAX_V &&
        measure->vin_V >= BUCK_VIN_START_MIN_V && measure->vin_V <= BUCK_VIN_START_MAX_V);
}
static uint8_t Buck_CheckLimits(void)
{
    float vin = Buck_RawVin(), vout = Buck_RawVout(), current = Buck_CurrentA();
    /* Positive range checks reject NaN too; V channels are unfiltered. */
    if (!(vin >= BUCK_VIN_RUN_MIN_V && vin <= BUCK_VIN_RUN_MAX_V)) {
        Buck_Fault("FAULT VIN | OFF"); return 0U;
    }
    if (!(vout >= -0.1f && vout <= BUCK_VOUT_MAX_V)) {
        Buck_Fault("FAULT VOUT | OFF"); return 0U;
    }
    /* Filtered PV current: slow supervisory check, NOT a fast current trip. */
    if (!(current >= -BUCK_CURRENT_LIMIT_A && current <= BUCK_CURRENT_LIMIT_A)) {
        Buck_Fault(buck_state == BUCK_RELAY_SETTLE ? "IPV RELAY | OFF" :
                   buck_state == BUCK_RAMP ? "IPV RAMP | OFF" :
                   "IPV RUN | OFF");
        return 0U;
    }
    return 1U;
}
static void Buck_StartPWM(void)
{
    GPIO_InitTypeDef gpio = {0};
    /* Relay settled; SD stays LOW while timer is prepared at zero compare. */
    HAL_GPIO_WritePin(BUCK_SD_GPIO_Port, BUCK_SD_Pin, GPIO_PIN_RESET);
    __HAL_TIM_DISABLE(&htim1);
    __HAL_TIM_MOE_DISABLE(&htim1);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
    __HAL_TIM_SET_COUNTER(&htim1, 0U);
    htim1.Instance->EGR = TIM_EGR_UG;
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
    gpio.Pin = BUCK_IN_Pin;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BUCK_IN_GPIO_Port, &gpio);
    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) {
        Buck_Fault("FAULT TIMER | OFF"); return;
    }
    /* CCR=0: IN LOW. Enabling SD turns Q3 ON to charge bootstrap.
     * Only for a discharged output with NO battery/external source.
     * No TFT/ADS calls during this bounded 2 ms interval.
     */
    HAL_GPIO_WritePin(BUCK_SD_GPIO_Port, BUCK_SD_Pin, GPIO_PIN_SET);
    HAL_Delay(2U);
    buck_ccr = BUCK_INITIAL_CCR;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, buck_ccr);
    ramp_ms = HAL_GetTick();
    buck_state = BUCK_RAMP;
    buck_message = "RAMP | BUCK ON";
}
static void Buck_Task(void)
{
    uint32_t now = HAL_GetTick();
    float vin, delta;
    uint8_t fresh;
    if (buck_state == BUCK_FAULT) return;
    fresh = Buck_DataFresh(now);
    if (buck_state != BUCK_WAIT_POWER) {
        if (!fresh || measure->error_count != last_ads_errors) {
            Buck_Fault("FAULT ADS | OFF"); return;
        }
        /* Relay settling has its own bounded qualification below.
         * Never enable SD/PWM while waiting for its current reading.
         * Run-time current protection is unchanged.
         */
        if ((buck_state == BUCK_RAMP || buck_state == BUCK_RUNNING) &&
            !Buck_CheckLimits()) return;
    }
    switch (buck_state) {
    case BUCK_WAIT_POWER:
        if (fresh) ads_seen = 1U;
        if (!ads_seen && (uint32_t)(now - boot_ms) >= BUCK_ADS_TIMEOUT_MS) {
            Buck_Fault("FAULT ADS | OFF"); return;
        }
        if (ads_seen && !fresh) { Buck_Fault("FAULT ADS | OFF"); return; }
        if (!fresh) { vin_stable = 0U; break; }
        if (measure->error_count != last_ads_errors) {
            Buck_Fault("FAULT ADS | OFF"); return;
        }
        /* An unloaded output capacitor can retain or acquire a small voltage
         * during reset.  Keep every power command OFF and wait for it to fall
         * below the pre-start threshold.  A voltage above the 2 V bench limit
         * is treated as an external/live output and latches a fault.
         */
        if (!(Buck_RawVout() >= -0.1f && Buck_RawVout() <= BUCK_VOUT_MAX_V)) {
            Buck_Fault("OUTPUT LIVE | OFF"); return;
        }
        if (Buck_RawVout() > BUCK_VOUT_BEFORE_ON_V) {
            vin_stable = 0U;
            buck_message = "WAIT VOUT | OFF";
            break;
        }
        if (!Buck_CurrentReady()) {
            /* Relay is still OFF, so this can be ACS712 startup settling.
             * Stay safely OFF and restart the qualification interval.
             * Once the relay closes, Buck_CheckLimits() still latches a fault.
             */
            vin_stable = 0U;
            buck_message = "WAIT CURRENT | OFF";
            break;
        }
        if (!Buck_VinStartOK()) {
            vin_stable = 0U;
            buck_message = "WAIT VIN | OFF";
            break;
        }
        vin = Buck_RawVin();
        delta = vin - vin_reference;
        if (!vin_stable || delta > BUCK_VIN_DRIFT_MAX_V || delta < -BUCK_VIN_DRIFT_MAX_V) {
            vin_reference = vin;
            stable_ms = now;
            vin_stable = 1U;
        }
        buck_message = "WAIT STABLE | OFF";
        if ((uint32_t)(now - boot_ms) >= BUCK_POWER_WAIT_MS &&
            (uint32_t)(now - stable_ms) >= BUCK_VIN_STABLE_MS) {
            HAL_GPIO_WritePin(BCCU_GPIO_Port, BCCU_Pin, GPIO_PIN_SET);
            relay_commanded_on = 1U;
            state_ms = HAL_GetTick();
            relay_sample_ms = measure->updated_ms;
            relay_good = 0U;
            buck_state = BUCK_RELAY_SETTLE;
            buck_message = "RELAY WAIT | OFF";
        }
        break;
    case BUCK_RELAY_SETTLE:
        /* PA8 stays GPIO LOW, TIM1 stopped and SD LOW throughout this state.
         * The relay edge can capacitively lift the unloaded output for a short
         * time.  OUTPUT LIVE was already checked before the relay was closed,
         * so use the normal 2 V bench limit here instead of the 0.2 V
         * pre-start limit.
         * Current outside the test window blocks startup and may only wait
         * up to BUCK_RELAY_TIMEOUT_MS, never indefinitely with relay ON.
         */
        if (!Buck_VinStartOK()) { Buck_Fault("FAULT VIN | OFF"); return; }
        if (!(Buck_RawVout() >= -0.1f && Buck_RawVout() <= BUCK_VOUT_MAX_V)) {
            Buck_Fault("FAULT VOUT | OFF"); return;
        }
        if ((uint32_t)(now - state_ms) >= BUCK_RELAY_TIMEOUT_MS) {
            Buck_Fault(Buck_CurrentReady() ? "RELAY UNSTABLE | OFF" :
                                           "IPV RELAY | OFF");
            return;
        }
        if ((uint32_t)(now - state_ms) < BUCK_RELAY_WAIT_MS) break;
        /* Only qualify newly published scans, not repeated reads of one scan. */
        if (measure->updated_ms == relay_sample_ms) break;
        relay_sample_ms = measure->updated_ms;
        if (!Buck_CurrentReady()) {
            relay_good = 0U;
            buck_message = "RELAY I WAIT | OFF";
            break;
        }
        vin = Buck_RawVin();
        delta = vin - relay_vin_reference;
        if (!relay_good || delta > BUCK_VIN_DRIFT_MAX_V ||
                           delta < -BUCK_VIN_DRIFT_MAX_V) {
            relay_vin_reference = vin;
            relay_good_ms = now;
            relay_good = 1U;
        }
        buck_message = "RELAY QUALIFY | OFF";
        if ((uint32_t)(now - relay_good_ms) >= BUCK_RELAY_STABLE_MS) {
            Buck_StartPWM();
        }
        break;
    case BUCK_RAMP:
        if ((uint32_t)(now - ramp_ms) >= BUCK_RAMP_STEP_MS) {
            ramp_ms = now;
            buck_ccr = (uint16_t)(buck_ccr + BUCK_CCR_STEP);
            if (buck_ccr >= BUCK_TARGET_CCR) {
                buck_ccr = BUCK_TARGET_CCR;
                buck_state = BUCK_RUNNING;
                buck_message = "TEST 10% | BUCK ON";
            }
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, buck_ccr);
        }
        break;
    case BUCK_RUNNING: break;
    default: Buck_Fault("FAULT STATE | OFF"); break;
    }
}
static int32_t milli(float v)
{
    return (int32_t)(v * 1000.0f + (v >= 0.0f ? 0.5f : -0.5f));
}
static void UpdateScreen(void)
{
    const MPPT_Measurement *display_measure = measure;
    uint8_t display_valid = Buck_DataFresh(HAL_GetTick());
    if (buck_state == BUCK_FAULT && fault_measure_valid) {
        display_measure = &fault_measure;
        display_valid = 1U;
    }
    screen.total_energy_Wh = MPPT_UI_U32_NA;
    screen.switching_frequency_Hz =
        (buck_state == BUCK_RAMP || buck_state == BUCK_RUNNING) ? BUCK_PWM_HZ : MPPT_UI_U32_NA;
    screen.battery_current_mA = MPPT_UI_I32_NA;
    screen.battery_power_mW = MPPT_UI_I32_NA;
    screen.ip_address = "--";
    screen.wifi_connected = screen.demo = 0;
    screen.load_on = (uint8_t)(buck_state == BUCK_RAMP || buck_state == BUCK_RUNNING);
    if (display_valid) {
        screen.pv_voltage_mV = (uint32_t)milli(display_measure->vin_V);
        screen.battery_voltage_mV = (uint32_t)milli(display_measure->vout_V);
        screen.pv_current_mA = milli(display_measure == &fault_measure ?
            fault_measure.ipv_A : Buck_CurrentA());

				if ((screen.pv_current_mA > -30) &&
						(screen.pv_current_mA < 30))
				{
						screen.pv_current_mA = 0;
				}

				screen.pv_power_mW =
						(int32_t)(((int64_t)screen.pv_voltage_mV * screen.pv_current_mA) / 1000);
    } else {
        screen.pv_voltage_mV = screen.battery_voltage_mV = MPPT_UI_U32_NA;
        screen.pv_current_mA = screen.pv_power_mW = MPPT_UI_I32_NA;
    }
    screen.date_time = buck_message;
    if (buck_state == BUCK_FAULT && fault_measure_valid) {
        /* Alternate stage label and actual current sample used by the trip.
         * No float printf support needed on Keil. Frozen panels are TRIP data.
         */
        if (((HAL_GetTick() / 2000U) % 3U) == 1U) {
            (void)snprintf(fault_message, sizeof(fault_message),
                "TRIP I:%ldmA | OFF", (long)milli(fault_measure.ipv_A));
            screen.date_time = fault_message;
        } else if (((HAL_GetTick() / 2000U) % 3U) == 2U) {
            (void)snprintf(fault_message, sizeof(fault_message),
                "TRIP A2:%ldmV", (long)milli(fault_measure.adc_V[MPPT_ADS_CURRENT_CH]));
            screen.date_time = fault_message;
        }
    }
    MPPT_UI_SetWarning((uint8_t)(buck_state == BUCK_FAULT));
    MPPT_UI_Draw(&screen);
}



/* Step 1: initialize only the shutdown controls, never enable PWM.
 * Called before HSE startup; repeated writes do not change the pin mode.
 * Kept in USER CODE so CubeMX regeneration preserves this setup. */
static void Power_SafeInit(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    Power_Off();
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pin = GPIO_PIN_11 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_8;
    HAL_GPIO_Init(GPIOA, &gpio);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
Power_SafeInit();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
	/* USER CODE BEGIN 2 */
    Power_Off();
    /* Verify the rewired board and timer assumptions before allowing startup. */
    if (BUCK_IN_GPIO_Port != GPIOA || BUCK_IN_Pin != GPIO_PIN_8 ||
        BUCK_SD_GPIO_Port != GPIOB || BUCK_SD_Pin != GPIO_PIN_11 ||
        BCCU_GPIO_Port != GPIOB || BCCU_Pin != GPIO_PIN_10 ||
        HAL_RCC_GetPCLK2Freq() != 72000000U ||
        htim1.Instance->PSC != 0U || htim1.Instance->ARR != 1439U) {
        Error_Handler();
    }
    ST7789_Init();
    MPPT_UI_Init();
    MPPT_Measure_Init(&hi2c1);
    measure = MPPT_Measure_Get();
    last_ads_errors = measure->error_count;
    boot_ms = HAL_GetTick(); /* Full wait starts AFTER display initialization. */
    buck_state = BUCK_WAIT_POWER;
    buck_message = "WAIT POWER | OFF";
    UpdateScreen();
    last_screen_ms = HAL_GetTick();
	/* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Buck_Task();
    MPPT_Measure_Task();
    Buck_Task();
    if ((uint32_t)(HAL_GetTick() - last_screen_ms) >= 200U) {
        last_screen_ms = HAL_GetTick();
        /* Do not let screen drawing extend the staged startup/ramp. */
        if (buck_state != BUCK_RELAY_SETTLE && buck_state != BUCK_RAMP) {
            UpdateScreen();
        }
    }
    if (buck_state != BUCK_RELAY_SETTLE && buck_state != BUCK_RAMP) {
        MPPT_UI_Tick();
    }
    Buck_Task(); /* Check freshness again after blocking display work. */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1439;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, TFT_DC_Pin|TFT_RST_Pin|TFT_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, BCCU_Pin|BUCK_SD_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : TFT_DC_Pin TFT_RST_Pin TFT_CS_Pin */
  GPIO_InitStruct.Pin = TFT_DC_Pin|TFT_RST_Pin|TFT_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : BCCU_Pin BUCK_SD_Pin */
  GPIO_InitStruct.Pin = BCCU_Pin|BUCK_SD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
Power_Off();
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */



