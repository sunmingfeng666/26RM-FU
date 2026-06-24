#ifndef __ALL_INIT_H
#define __ALL_INIT_H

#include "main.h"

#define pid_max_out  5000.0f
#define pid_max_iout 3000.0f

//pid结构体
typedef struct
{
    float kp;
    float ki;
    float kd;

    float max_out;
    float max_iout;

    float set;
    float fdb;
    float err[3];

    float p_out;
    float i_out;
    float d_out;

    float output;

} pid_typedef;

//电机结构体
typedef struct
{
    float angle;
    float speed;
    float angle_set;
    float speed_set;
    float give_current;


} motor_t;

//pid初始化函数

void All_Init(void);

#endif // !_pid_cmd_H

