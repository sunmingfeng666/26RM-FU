#ifndef __MY_CAN_H
#define __MY_CAN_H

#include "main.h"
#include "can.h"


typedef struct
{
int16_t rawEncode;     // 编码器返回的原始值
  int16_t lastRawEncode; // 上一次的编码器原始值
  int32_t round;         // 圈数
  int32_t conEncode;     // 处理后的连续的编码器值
  int32_t lastConEncode; // 上一次的处理后的连续编码器值

  int16_t rawSpeed;     // 返回的转速
  int16_t lastRawSpeed; // 上一次返回的转速

  int16_t Current; // 转矩电流

  int8_t temp; // 温度
}M3508_Motor_t ;

extern M3508_Motor_t motor1;

void CAN_Filter_Init(CAN_HandleTypeDef *hcan);
void can_calc(CAN_HandleTypeDef*hcan,uint32_t ID);

void M3508_Send_Current(CAN_HandleTypeDef *hcan,
                        int16_t iq1,
                        int16_t iq2,
                        int16_t iq3,
                        int16_t iq4);
                        

#endif