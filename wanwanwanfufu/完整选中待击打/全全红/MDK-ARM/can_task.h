#ifndef CAN_TASK_H
#define CAN_TASK_H

#include "can.h"
#include "string.h"


union COMMUNICATION_UNION_Typdef // ??¨´?????????????
{
    struct __packed
    {
        //???????
        int16_t VX;
        int16_t VY;
        int16_t VW;
		int8_t  VZ;
		int8_t  VR;
        uint8_t DBUS_STATUS;
    } DataNeaten;
    // ???????????
    uint8_t  Data[9];
};

void CAN_Filter_Init(void);
//???can????
void RUI_F_CAN_SEDN(CAN_HandleTypeDef* _hcan , int16_t stdid , int16_t num1 , int16_t num2 , int16_t num3 , int16_t num4);

//?????³²??
void RUI_F_CAN_SEDN_UNION(CAN_HandleTypeDef* _hcan , int16_t stdid , uint8_t* Data);
extern volatile uint8_t a;

#endif
