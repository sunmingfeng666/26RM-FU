#include "tim_task.h"
#include "my_can.h"
#include "can.h"
#include "vofa.h"
#include "usart.h"
#include <math.h>

/* 大能量机关正弦速度控制，先使用保守 PI 参数，D 先关闭避免速度反馈噪声放大 */
float kp_test = 1.20f;
float ki_test = 0.0030f;
float kd_test = 0.0f;

#define OUTPUT_SPEED_A_MIN             0.780f
#define OUTPUT_SPEED_A_MAX             1.045f
#define OUTPUT_SPEED_OMEGA_MIN_RAD_S   1.884f
#define OUTPUT_SPEED_OMEGA_MAX_RAD_S   2.000f
#define OUTPUT_SPEED_B_SUM_RAD_S       2.090f
#define TOTAL_REDUCTION_RATIO          57.0f
#define PI_F                           3.1415926f

/* 速度环PID */
pid_typedef motor1_speed_pid;

/* 目标速度，单位 rpm（电机侧） */
float motor1_target_speed = 0.0f;
float a_param = OUTPUT_SPEED_A_MIN;
float w_param = OUTPUT_SPEED_OMEGA_MIN_RAD_S;
float b_param = OUTPUT_SPEED_B_SUM_RAD_S - OUTPUT_SPEED_A_MIN;
float t_sec = 0.0f;

static uint32_t sine_seed = 0x13572468UL;

/* PID输出电流 */
float motor1_pid_out = 0.0f;

static float rand_range_f(float min_val, float max_val)
{
    float ratio;

    sine_seed = sine_seed * 1664525UL + 1013904223UL + HAL_GetTick();
    ratio = (float)(sine_seed & 0xFFFFUL) / 65535.0f;

    return min_val + (max_val - min_val) * ratio;
}

void TIM_Task_ResetSineParams(void)
{
    a_param = rand_range_f(OUTPUT_SPEED_A_MIN, OUTPUT_SPEED_A_MAX);
    w_param = rand_range_f(OUTPUT_SPEED_OMEGA_MIN_RAD_S, OUTPUT_SPEED_OMEGA_MAX_RAD_S);
    b_param = OUTPUT_SPEED_B_SUM_RAD_S - a_param;
    t_sec = 0.0f;
}

//pid初始化
void TIM_Task_Init(void)
{
    pid_init(&motor1_speed_pid,
         kp_test,
         ki_test,
         kd_test,
         pid_max_out,
         pid_max_iout);

    TIM_Task_ResetSineParams();
}

//正弦速度执行函数
void TIM_Task_10ms(void)
{
  float spd_rad_s;
  float target_rpm_out;

  motor1_speed_pid.kp = kp_test;
  motor1_speed_pid.ki = ki_test;
  motor1_speed_pid.kd = kd_test;

  t_sec += 0.01f;
  spd_rad_s = a_param * sinf(w_param * t_sec) + b_param;

  target_rpm_out = spd_rad_s * 60.0f / (2.0f * PI_F);
  motor1_target_speed = target_rpm_out * TOTAL_REDUCTION_RATIO;

  motor1_pid_out = pid_calc(&motor1_speed_pid,
                            motor1_target_speed,
                            motor1.rawSpeed,
                            0);

  if (motor1_pid_out > 32768.0f)  motor1_pid_out = 32768.0f;
  if (motor1_pid_out < -32768.0f) motor1_pid_out = -32768.0f;

  M3508_Send_Current(&hcan, (int16_t)motor1_pid_out, 0, 0, 0);
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
