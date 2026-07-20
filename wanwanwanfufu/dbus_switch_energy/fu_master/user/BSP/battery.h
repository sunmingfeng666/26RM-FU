#ifndef FU_MASTER_BATTERY_H
#define FU_MASTER_BATTERY_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef struct
{
    volatile uint16_t raw;
    volatile uint32_t millivolt;
    volatile uint16_t low_confirm_count;
    volatile uint8_t low_voltage;
    volatile uint32_t sample_count;
    volatile uint32_t adc_error_count;
} BatteryDebug_t;

extern volatile BatteryDebug_t g_battery_debug;

HAL_StatusTypeDef Battery_Init(ADC_HandleTypeDef *hadc);
void Battery_Update(uint32_t now_ms);
uint16_t Battery_GetRaw(void);
uint32_t Battery_GetMillivolt(void);
uint16_t Battery_GetLowConfirmCount(void);
uint8_t Battery_IsLowVoltage(void);

#endif
