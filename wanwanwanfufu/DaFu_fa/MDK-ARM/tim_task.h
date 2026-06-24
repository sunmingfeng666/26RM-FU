#ifndef __TIM_TASK_H
#define __TIM_TASK_H

#include "main.h"

extern volatile uint8_t armor_active;
extern volatile uint8_t hit_flag;
extern volatile uint32_t tim2_ms;

void TIM_Task_Init(void);
void TIM_Task_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif