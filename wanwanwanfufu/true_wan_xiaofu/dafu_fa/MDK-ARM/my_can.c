#include "my_can.h"

//CAN接收数据缓存
uint8_t can1rxmessage[8]={0};
//CAN接收头结构体
CAN_RxHeaderTypeDef rx1;

M3508_Motor_t motor1;


void can_calc(CAN_HandleTypeDef*hcan,uint32_t ID);

//接收回调函数
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
   HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx1, can1rxmessage);
    can_calc(hcan, rx1.StdId);
}

/* 解析3508反馈数据 */
void can_calc(CAN_HandleTypeDef *hcan, uint32_t ID)
{
    (void)hcan;

    if (ID == 0x201)
{
    motor1.lastRawEncode = motor1.rawEncode;
    motor1.lastRawSpeed  = motor1.rawSpeed;
    motor1.lastConEncode = motor1.conEncode;

    motor1.rawEncode = (int16_t)((can1rxmessage[0] << 8) | can1rxmessage[1]);
    motor1.rawSpeed  = (int16_t)((can1rxmessage[2] << 8) | can1rxmessage[3]);

    motor1.Current = 0;
    motor1.temp    = 0;

    if ((motor1.rawEncode - motor1.lastRawEncode) > 4096)
    {
        motor1.round--;
    }
    else if ((motor1.rawEncode - motor1.lastRawEncode) < -4096)
    {
        motor1.round++;
    }

    motor1.conEncode = motor1.rawEncode + motor1.round * 8192;
}
}


//滤波器初始化
void CAN_Filter_Init(CAN_HandleTypeDef *hcan)
{
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank           = 0;
    sFilterConfig.FilterMode           = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale          = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh         = 0x0000;
    sFilterConfig.FilterIdLow          = 0x0000;
    sFilterConfig.FilterMaskIdHigh     = 0x0000;
    sFilterConfig.FilterMaskIdLow      = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation     = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(hcan, &sFilterConfig) != HAL_OK) {
        /* Filter configuration Error */
        Error_Handler();
    }
    /* Start the CAN peripheral */
    if (HAL_CAN_Start(hcan) != HAL_OK) {
        /* Start Error */
        Error_Handler();
    }
    /* Activate CAN RX notification */
    if (HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        /* Notification Error */
        Error_Handler();
    }
    /* Activate CAN TX notification */
    if (HAL_CAN_ActivateNotification(hcan, CAN_IT_TX_MAILBOX_EMPTY) != HAL_OK) {
        /* Notification Error */
        Error_Handler();
    }
}

//给3508发送控制
void M3508_Send_Current(CAN_HandleTypeDef *hcan,
                        int16_t iq1,
                        int16_t iq2,
                        int16_t iq3,
                        int16_t iq4)
{
    CAN_TxHeaderTypeDef tx_header;
    uint32_t tx_mailbox;
    uint8_t tx_data[8];

    tx_header.StdId = 0x200;
    tx_header.ExtId = 0x00;
    tx_header.IDE   = CAN_ID_STD;
    tx_header.RTR   = CAN_RTR_DATA;
    tx_header.DLC   = 8;
    tx_header.TransmitGlobalTime = DISABLE;

    tx_data[0] = (uint8_t)(iq1 >> 8);
    tx_data[1] = (uint8_t)(iq1);
    tx_data[2] = (uint8_t)(iq2 >> 8);
    tx_data[3] = (uint8_t)(iq2);
    tx_data[4] = (uint8_t)(iq3 >> 8);
    tx_data[5] = (uint8_t)(iq3);
    tx_data[6] = (uint8_t)(iq4 >> 8);
    tx_data[7] = (uint8_t)(iq4);

    HAL_CAN_AddTxMessage(hcan, &tx_header, tx_data, &tx_mailbox);
}

