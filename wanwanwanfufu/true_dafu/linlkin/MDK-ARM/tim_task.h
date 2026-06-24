#ifndef __TIM_TASK_H
#define __TIM_TASK_H

#include "main.h"
#include "tim.h"
#include "all_init.h"

extern float motor1_target_speed;
extern float motor1_pid_out;

void TIM_Task_Init(void);
void TIM_Task_10ms(void);
void TIM_Task_ResetSineParams(void);
void pid_init(pid_typedef*pid,float kp,float ki,float kd,float max_out,float max_iout);
float pid_calc(pid_typedef*pid,float set,float fdb,int max_angle);
void VOFA_Task_100ms();
void TIM_Task_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif
