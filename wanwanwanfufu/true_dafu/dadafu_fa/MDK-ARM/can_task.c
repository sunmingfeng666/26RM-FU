#include "can_task.h"
#include "tim_task.h"


CAN_FilterTypeDef CAN_FilterInitStrt;


CAN_SendData_t sdata;
/************************************************************���ָܷ��**************************************************************
 * 	@author:			//С��
 *	@performance:	    //CAN ID����
 *	@parameter:		    //
 *	@time:				//22-11-23 20:39
 *	@ReadMe:			//�������������ʼ��һ��
 ************************************************************���ָܷ��**************************************************************/

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
/************************************************************���ָܷ��**************************************************************
 * 	@author:			//С��
 *	@performance:	    //CAN���ͺ���
 *	@parameter:		    //@CAN1 or CAN2 @stdid:ID @num1:Ҫ���͵�int16����
 *	@time:				//22-11-23 20:41
 *	@ReadMe:			//
 ************************************************************���ָܷ��**************************************************************/

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
/************************************************************���ָܷ��**************************************************************
 * 	@author:			//С��
 *	@performance:	    //������can����
 *	@parameter:		    //
 *	@time:				//23-05-06 19:07
 *	@ReadMe:			//
 ************************************************************���ָܷ��**************************************************************/

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





/************************************************************���ָܷ��**************************************************************
 * 	@author:			//С��
 *	@performance:	    //CAN���պ���
 *	@parameter:		    //
 *	@time:				//22-11-23 20:42
 *	@ReadMe:			//
 *  @LastUpDataTime:    //2023-04-20 02:52    bestrui
 *  @UpData��           //���³ɹ�����
 *  @LastUpDataTime:    //2023-05-06 20:23    bestrui
 *  @UpData��           //�����ж��߼�
 ************************************************************���ָܷ��**************************************************************/

CAN_RxHeaderTypeDef can_rx;

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan)
{
   
    uint8_t rx_data[8];

    HAL_CAN_GetRxMessage(hcan , CAN_RX_FIFO0 , &can_rx , rx_data);
    
	if (hcan == &hcan2)		
	{
		/* 0x301/0x311/.../0x341 分别映射到 1~5 号靶板。 */
		switch (can_rx.StdId)
		{
			case 0x301:
                    hit_board_id = 1;
                         break;

            case 0x311:
                    hit_board_id = 2;
                        break;

            case 0x321:
                    hit_board_id = 3;
                        break;

            case 0x331:
                    hit_board_id = 4;
                        break;

            case 0x341:
                    hit_board_id = 5;
                        break;

            default:
                break;
			
		}
			
	}
	
}

