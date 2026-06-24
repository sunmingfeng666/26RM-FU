#ifndef __TIM_TASK_H
#define __TIM_TASK_H

#include "main.h"

extern volatile uint32_t tim2_ms;
extern volatile uint8_t hit_board_id;
extern volatile uint32_t group_count;
void TIM_Task_Init(void);
void TIM_Task_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif
