#include "can_task.h"
#include "tim_task.h"

CAN_FilterTypeDef CAN_FilterInitStrt;

CAN_SendData_t sdata;
/************************************************************万能分隔符**************************************************************
 * 	@author:			//小瑞
 *	@performance:	    //CAN ID过滤
 *	@parameter:		    //
 *	@time:				//22-11-23 20:39
 *	@ReadMe:			//放在主函数里初始化一次
 ************************************************************万能分隔符**************************************************************/
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
    CAN_FilterInitStrt.FilterBank           = 14;
    CAN_FilterInitStrt.FilterFIFOAssignment = CAN_RX_FIFO0;

    HAL_CAN_ConfigFilter(&hcan2 , &CAN_FilterInitStrt);
    HAL_CAN_Start(&hcan2);
    HAL_CAN_ActivateNotification(&hcan2 , CAN_IT_RX_FIFO0_MSG_PENDING);

    CAN_FilterInitStrt.SlaveStartFilterBank = 14;
    CAN_FilterInitStrt.FilterBank           = 14;

//    HAL_CAN_ConfigFilter(&hcan2 , &CAN_FilterInitStrt);
//    HAL_CAN_Start(&hcan2);
//    HAL_CAN_ActivateNotification(&hcan2 , CAN_IT_RX_FIFO0_MSG_PENDING);

    return;
}
/************************************************************万能分隔符**************************************************************
 * 	@author:			//小瑞
 *	@performance:	    //CAN发送函数
 *	@parameter:		    //@CAN1 or CAN2 @stdid:ID @num1:要发送的int16数据
 *	@time:				//22-11-23 20:41
 *	@ReadMe:			//
 ************************************************************万能分隔符**************************************************************/
void RUI_F_CAN_SEDN(CAN_HandleTypeDef* _hcan , int16_t stdid , int16_t num1 , int16_t num2 , int16_t num3 , int16_t num4)
{
    CAN_TxHeaderTypeDef TXMessage;
    uint8_t Data[8];
    uint32_t send_mail_box;

    TXMessage.DLC   = 0x08;
    TXMessage.IDE   = CAN_ID_STD;
    TXMessage.RTR   = CAN_RTR_DATA;
    TXMessage.StdId = stdid;

    Data[0] = ((num1) >> 8);
    Data[1] = (num1);
    Data[2] = ((num2) >> 8);
    Data[3] = (num2);
    Data[4] = ((num3) >> 8);
    Data[5] = (num3);
    Data[6] = ((num4) >> 8);
    Data[7] = (num4);

    if ((_hcan) == &hcan2)
    {
        HAL_CAN_AddTxMessage(&hcan2 , &TXMessage , Data , &send_mail_box);

    }
//    else if ((_hcan) == &hcan2)
//    {
//        HAL_CAN_AddTxMessage(&hcan2 , &TXMessage , Data , &send_mail_box);
//    }
}
/************************************************************万能分隔符**************************************************************
 * 	@author:			//小瑞
 *	@performance:	    //共用体can发送
 *	@parameter:		    //
 *	@time:				//23-05-06 19:07
 *	@ReadMe:			//
 ************************************************************万能分隔符**************************************************************/
void RUI_F_CAN_SEDN_UNION(CAN_HandleTypeDef* _hcan , int16_t stdid , uint8_t* Data)
{
    CAN_TxHeaderTypeDef TXMessage;

    uint32_t send_mail_box;

    TXMessage.DLC   = 0x08;
    TXMessage.IDE   = CAN_ID_STD;
    TXMessage.RTR   = CAN_RTR_DATA;
    TXMessage.StdId = stdid;


    if ((_hcan) == &hcan2)
    {
        HAL_CAN_AddTxMessage(&hcan2 , &TXMessage , Data , &send_mail_box);

    }
//    else if ((_hcan) == &hcan2)
//    {
//        HAL_CAN_AddTxMessage(&hcan2 , &TXMessage , Data , &send_mail_box);
//    }
}





/************************************************************万能分隔符**************************************************************
 * 	@author:			//小瑞
 *	@performance:	    //CAN接收函数
 *	@parameter:		    //
 *	@time:				//22-11-23 20:42
 *	@ReadMe:			//
 *  @LastUpDataTime:    //2023-04-20 02:52    bestrui
 *  @UpData：           //更新成共用体
 *  @LastUpDataTime:    //2023-05-06 20:23    bestrui
 *  @UpData：           //更新判断逻辑
 ************************************************************万能分隔符**************************************************************/
CAN_RxHeaderTypeDef can_rx;
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan)
{
   
    uint8_t rx_data[8];

    HAL_CAN_GetRxMessage(hcan , CAN_RX_FIFO0 , &can_rx , rx_data);
    
	if (hcan == &hcan2)		
	{
		//头部CAN2
		switch (can_rx.StdId)
		{
			case 0x301:   // 1?????
                pressed[0] = 1;
                if (current_target == 1) hit_flag = 1;
                break;

            case 0x311:   // 2?????
                pressed[1] = 1;
                if (current_target == 2) hit_flag = 1;
                break;

            case 0x321:   // 3?????
                pressed[2] = 1;
                if (current_target == 3) hit_flag = 1;
                break;

            case 0x331:   // 4?????
                pressed[3] = 1;
                if (current_target == 4) hit_flag = 1;
                break;

            case 0x341:   // 5?????
                pressed[4] = 1;
                if (current_target == 5) hit_flag = 1;
                break;

            default:
                break;
			
		}
			
	}
	
}

