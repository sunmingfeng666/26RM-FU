#ifndef FU_MASTER_BUZZER_H
#define FU_MASTER_BUZZER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef struct
{
    volatile uint8_t output_on;
    volatile uint8_t low_voltage_alarm;
    volatile uint8_t queued_hit_beeps;
    volatile uint32_t hit_beep_count;
} BuzzerDebug_t;

extern volatile BuzzerDebug_t g_buzzer_debug;

HAL_StatusTypeDef Buzzer_Init(TIM_HandleTypeDef *htim, uint32_t channel);
void Buzzer_SetLowVoltageAlarm(uint8_t enabled);
void Buzzer_QueueHitBeep(void);
void Buzzer_CancelHitBeeps(void);
void Buzzer_Update(uint32_t now_ms);

#endif
