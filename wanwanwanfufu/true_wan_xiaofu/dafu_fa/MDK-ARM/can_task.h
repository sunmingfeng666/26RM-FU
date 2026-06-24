#ifndef CAN_TASK_H
#define CAN_TASK_H

#include "can.h"
#include "string.h"
#include "main.h"

typedef union
{
    struct __packed
    {
        int16_t q0;
        int16_t q1;
        int16_t q2;
        int16_t q3;
    } DataNeaten;

    uint8_t Data[8];
} CAN_SendData_t;

extern CAN_SendData_t sdata;

void CAN_Filter_Init(void);
void RUI_F_CAN_SEDN(CAN_HandleTypeDef* _hcan , int16_t stdid , int16_t num1 , int16_t num2 , int16_t num3 , int16_t num4);
void RUI_F_CAN_SEDN_UNION(CAN_HandleTypeDef* _hcan , int16_t stdid , uint8_t* Data);

#endif