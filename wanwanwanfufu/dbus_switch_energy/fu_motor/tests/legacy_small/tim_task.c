#include "tim_task.h"
#include "my_can.h"
#include "can.h"
#include "vofa.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

float kp_test = 2.3f;
float ki_test = 0.005f;
float kd_test = 0.015f;

#define MOTOR1_TARGET_SPEED_FINAL_RPM 570.0f
#define MOTOR1_TARGET_SPEED_RAMP_STEP_RPM 5.0f

extern CAN_HandleTypeDef hcan;

/* 速度环PID */
pid_typedef motor1_speed_pid;

/* 当前目标速度，单位rpm，按斜坡逐步逼近最终目标 */
float motor1_target_speed = 0.0f;

/* PID输出电流 */
float motor1_pid_out = 0.0f;

uint32_t pid_log_time_ms = 0;
char pid_log_buf[128];

//pid初始化
void TIM_Task_Init(void)
{
    pid_init(&motor1_speed_pid,
         kp_test,
         ki_test,
         kd_test,
         pid_max_out,
         pid_max_iout);
}

void TIM_Task_10ms(void)
{
	motor1_speed_pid.kp = kp_test;
motor1_speed_pid.ki = ki_test;
motor1_speed_pid.kd = kd_test;

    if (motor1_target_speed < MOTOR1_TARGET_SPEED_FINAL_RPM)
    {
        motor1_target_speed += MOTOR1_TARGET_SPEED_RAMP_STEP_RPM;
        if (motor1_target_speed > MOTOR1_TARGET_SPEED_FINAL_RPM)
        {
            motor1_target_speed = MOTOR1_TARGET_SPEED_FINAL_RPM;
        }
    }
    else if (motor1_target_speed > MOTOR1_TARGET_SPEED_FINAL_RPM)
    {
        motor1_target_speed -= MOTOR1_TARGET_SPEED_RAMP_STEP_RPM;
        if (motor1_target_speed < MOTOR1_TARGET_SPEED_FINAL_RPM)
        {
            motor1_target_speed = MOTOR1_TARGET_SPEED_FINAL_RPM;
        }
    }
	
    motor1_pid_out = pid_calc(&motor1_speed_pid,
                              motor1_target_speed,
                              motor1.rawSpeed,
                              0);

    if (motor1_pid_out > 32768.0f)  motor1_pid_out = 32768.0f;
    if (motor1_pid_out < -32768.0f) motor1_pid_out = -32768.0f;

    M3508_Send_Current(&hcan, (int16_t)motor1_pid_out, 0, 0, 0);
	//M3508_Send_Current(&hcan, 1000, 0, 0, 0);
	
}


void VOFA_Task_100ms(void)
{
    if (huart2.gState == HAL_UART_STATE_READY)
    {
        VOFA_justfloat(motor1_target_speed,
                       motor1.rawSpeed,
                       motor1_pid_out,
                       0,0,0,0,0,0,0);
    }
}

//void VOFA_Task_100ms(void)
//{
//    if (huart2.gState == HAL_UART_STATE_READY)
//    {
//        pid_log_time_ms += 100;

//        int len = snprintf(pid_log_buf, sizeof(pid_log_buf),
//                           "%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\r\n",
//                           pid_log_time_ms,
//                           (float)motor1_target_speed,
//                           (float)motor1.rawSpeed,
//                           (float)motor1_pid_out,
//                           (float)motor1_speed_pid.err[0],
//                           (float)motor1_speed_pid.p_out,
//                           (float)motor1_speed_pid.i_out,
//                           (float)motor1_speed_pid.d_out);

//        HAL_UART_Transmit(&huart2, (uint8_t *)pid_log_buf, len, 100);
//    }
//}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    //定时器2任务
    if (htim->Instance == TIM2)
     {
        
       TIM_Task_10ms();
    }
		 
		//定时器3任务
		if (htim->Instance == TIM3)
    {
        VOFA_Task_100ms();
    }

}
