#include "battery.h"

#define BATTERY_SAMPLE_PERIOD_MS  50U
#define BATTERY_AVERAGE_SAMPLES    8U
#define BATTERY_STOP_MV        22200U
#define BATTERY_LOW_CONFIRM_COUNT 200U
#define BATTERY_CALIBRATION_MV    800U

volatile BatteryDebug_t g_battery_debug = {0};

static ADC_HandleTypeDef *s_adc = NULL;
static uint32_t s_last_sample_ms = 0U;

static HAL_StatusTypeDef Battery_ReadAverage(uint16_t *average)
{
    uint32_t sum = 0U;

    if ((s_adc == NULL) || (average == NULL))
    {
        return HAL_ERROR;
    }

    for (uint8_t i = 0U; i < BATTERY_AVERAGE_SAMPLES; ++i)
    {
        if (HAL_ADC_Start(s_adc) != HAL_OK)
        {
            g_battery_debug.adc_error_count++;
            return HAL_ERROR;
        }
        if (HAL_ADC_PollForConversion(s_adc, 2U) != HAL_OK)
        {
            (void)HAL_ADC_Stop(s_adc);
            g_battery_debug.adc_error_count++;
            return HAL_ERROR;
        }
        sum += HAL_ADC_GetValue(s_adc);
        (void)HAL_ADC_Stop(s_adc);
    }

    *average = (uint16_t)(sum / BATTERY_AVERAGE_SAMPLES);
    return HAL_OK;
}

static uint32_t Battery_RawToMillivolt(uint16_t raw)
{
    return ((((uint32_t)raw * 33300U) + 2047U) / 4095U) + BATTERY_CALIBRATION_MV;
}

static void Battery_ApplySample(uint16_t raw)
{
    g_battery_debug.raw = raw;
    g_battery_debug.millivolt = Battery_RawToMillivolt(raw);
    g_battery_debug.sample_count++;

    if (g_battery_debug.low_voltage != 0U)
    {
        if (g_battery_debug.millivolt > BATTERY_STOP_MV)
        {
            g_battery_debug.low_voltage = 0U;
            g_battery_debug.low_confirm_count = 0U;
        }
    }
    else if (g_battery_debug.millivolt < BATTERY_STOP_MV)
    {
        if (g_battery_debug.low_confirm_count < BATTERY_LOW_CONFIRM_COUNT)
        {
            g_battery_debug.low_confirm_count++;
        }
        if (g_battery_debug.low_confirm_count >= BATTERY_LOW_CONFIRM_COUNT)
        {
            g_battery_debug.low_voltage = 1U;
        }
    }
    else
    {
        g_battery_debug.low_confirm_count = 0U;
    }
}

HAL_StatusTypeDef Battery_Init(ADC_HandleTypeDef *hadc)
{
    uint16_t raw;

    if (hadc == NULL)
    {
        return HAL_ERROR;
    }

    s_adc = hadc;
    s_last_sample_ms = HAL_GetTick();
    g_battery_debug.low_voltage = 0U;
    g_battery_debug.low_confirm_count = 0U;

    if (Battery_ReadAverage(&raw) != HAL_OK)
    {
        return HAL_ERROR;
    }

    g_battery_debug.raw = raw;
    g_battery_debug.millivolt = Battery_RawToMillivolt(raw);
    g_battery_debug.sample_count++;
    return HAL_OK;
}

void Battery_Update(uint32_t now_ms)
{
    uint16_t raw;

    if ((now_ms - s_last_sample_ms) < BATTERY_SAMPLE_PERIOD_MS)
    {
        return;
    }
    s_last_sample_ms = now_ms;

    if (Battery_ReadAverage(&raw) == HAL_OK)
    {
        Battery_ApplySample(raw);
    }
}

uint16_t Battery_GetRaw(void)
{
    return g_battery_debug.raw;
}

uint32_t Battery_GetMillivolt(void)
{
    return g_battery_debug.millivolt;
}

uint16_t Battery_GetLowConfirmCount(void)
{
    return g_battery_debug.low_confirm_count;
}

uint8_t Battery_IsLowVoltage(void)
{
    return g_battery_debug.low_voltage;
}
