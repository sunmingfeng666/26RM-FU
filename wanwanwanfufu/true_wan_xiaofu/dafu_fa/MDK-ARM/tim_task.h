#ifndef __TIM_TASK_H
#define __TIM_TASK_H

#include "main.h"

extern volatile uint8_t current_target;
extern volatile uint8_t hit_flag;
extern volatile uint8_t timeout_flag;
extern volatile uint32_t tim2_ms;
extern volatile uint8_t round_state;
extern volatile uint8_t pressed[5];

void TIM_Task_Init(void);
void TIM_Task_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif