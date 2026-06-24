#ifndef CAN_TASK_H
#define CAN_TASK_H

#include "can.h"
#include "string.h"


union COMMUNICATION_UNION_Typdef // 使用共用体整合数据
{
    struct __packed
    {
        //遥控数据
        int16_t VX;
        int16_t VY;
        int16_t VW;
		int8_t  VZ;
		int8_t  VR;
        uint8_t DBUS_STATUS;
    } DataNeaten;
    // 接收到的数组
    uint8_t  Data[9];
};

void CAN_Filter_Init(void);
//电机can发送
void RUI_F_CAN_SEDN(CAN_HandleTypeDef* _hcan , int16_t stdid , int16_t num1 , int16_t num2 , int16_t num3 , int16_t num4);

//共用体发送
void RUI_F_CAN_SEDN_UNION(CAN_HandleTypeDef* _hcan , int16_t stdid , uint8_t* Data);

#endif
