#include "can_task.h"
#include "led_rings.h"
#include <string.h>

volatile uint8_t a = 0;
CAN_FilterTypeDef CAN_FilterInitStrt;
/************************************************************????????**************************************************************
 * 	@author:			//???
 *	@performance:	    //CAN ID????
 *	@parameter:		    //
 *	@time:				//22-11-23 20:39
 *	@ReadMe:			//?????????????'??h??
 ************************************************************????????**************************************************************/
void CAN_Filter_Init(void)
{
    CAN_FilterInitStrt.SlaveStartFilterBank = 14;
    CAN_FilterInitStrt.FilterBank           = 14;
    CAN_FilterInitStrt.FilterActivation     = ENABLE;
    CAN_FilterInitStrt.FilterMode           = CAN_FILTERMODE_IDMASK;
    CAN_FilterInitStrt.FilterScale          = CAN_FILTERSCALE_32BIT;
    CAN_FilterInitStrt.FilterIdHigh         = 0x0000;
    CAN_FilterInitStrt.FilterIdLow          = 0x0000;
    CAN_FilterInitStrt.FilterMaskIdHigh     = 0x0000;
    CAN_FilterInitStrt.FilterMaskIdLow      = 0x0000;
    CAN_FilterInitStrt.FilterBank           = 0;
    CAN_FilterInitStrt.FilterFIFOAssignment = CAN_RX_FIFO0;

    HAL_CAN_ConfigFilter(&hcan , &CAN_FilterInitStrt);
    HAL_CAN_Start(&hcan);
    HAL_CAN_ActivateNotification(&hcan , CAN_IT_RX_FIFO0_MSG_PENDING);

    CAN_FilterInitStrt.SlaveStartFilterBank = 14;
    CAN_FilterInitStrt.FilterBank           = 14;

//    HAL_CAN_ConfigFilter(&hcan2 , &CAN_FilterInitStrt);
//    HAL_CAN_Start(&hcan2);
//    HAL_CAN_ActivateNotification(&hcan2 , CAN_IT_RX_FIFO0_MSG_PENDING);

    return;
}
/**
 * @brief CAN???????
 *
 * @param _hcan CANx
 * @param stdid ?????
 * @param num1
 * @param num2
 * @param num3
 * @param num4
 * @return void
 */
void CAN_send(CAN_HandleTypeDef *_hcan, int16_t stdid, int32_t num1, int16_t num2, int16_t num3, int16_t num4)
{
    CAN_TxHeaderTypeDef tx;
    uint8_t Data[8];
    uint32_t mailbox = 0;
    tx.DLC           = 0x08;
    tx.IDE           = CAN_ID_STD;
    tx.RTR           = CAN_RTR_DATA;
    tx.StdId         = stdid;
    tx.ExtId         = 0x000;
    Data[0]          = ((num1) >> 8);
    Data[1]          = (num1);
    Data[2]          = ((num2) >> 8);
    Data[3]          = (num2);
    Data[4]          = ((num3) >> 8);
    Data[5]          = (num3);
    Data[6]          = ((num4) >> 8);
    Data[7]          = (num4);

    HAL_CAN_AddTxMessage(&hcan, &tx, Data, &mailbox);
}

/************************************************************????????**************************************************************
 * 	@author:			//???
 *	@performance:	    //??????can????
 *	@parameter:		    //
 *	@time:				//23-05-06 19:07
 *	@ReadMe:			//
 ************************************************************????????**************************************************************/
void RUI_F_CAN_SEDN_UNION(CAN_HandleTypeDef* _hcan , int16_t stdid , uint8_t* Data)
{
    CAN_TxHeaderTypeDef TXMessage;

    uint32_t send_mail_box;

    TXMessage.DLC   = 0x08;
    TXMessage.IDE   = CAN_ID_STD;
    TXMessage.RTR   = CAN_RTR_DATA;
    TXMessage.StdId = stdid;


    if ((_hcan) == &hcan)
    {
        HAL_CAN_AddTxMessage(&hcan , &TXMessage , Data , &send_mail_box);

    }
//    else if ((_hcan) == &hcan2)
//    {
//        HAL_CAN_AddTxMessage(&hcan2 , &TXMessage , Data , &send_mail_box);
//    }
}





/************************************************************????????**************************************************************
 * 	@author:			//???
 *	@performance:	    //CAN???????
 *	@parameter:		    //
 *	@time:				//22-11-23 20:42
 *	@ReadMe:			//
 *  @LastUpDataTime:    //2023-04-20 02:52    bestrui
 *  @UpData??           //???�??????
 *  @LastUpDataTime:    //2023-05-06 20:23    bestrui
 *  @UpData??           //??????????
 ************************************************************????????**************************************************************/
uint8_t CAN1RXmessage[8];
CAN_RxHeaderTypeDef rx1;

void CAN_Calc(CAN_HandleTypeDef *hcan, uint32_t ID);

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{		
		memset(CAN1RXmessage, 0, sizeof(CAN1RXmessage));
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx1, CAN1RXmessage);
    CAN_Calc(hcan, rx1.StdId);
}

void CAN_Calc(CAN_HandleTypeDef *hcan, uint32_t ID)
{
    if (hcan->Instance == CAN1)
    {
			//a=0x55;
        switch (ID)
        {
            case 0x231:
                a = 1;
                break;

            case 0x232:
            case 0x2FF:
                a = 0;
                break;

            default:
                break;
        }
    }
}
