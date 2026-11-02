#include "motor_can.h"
#include "motor_control.h"
CAN_HandleTypeDef hcan;
static volatile MotorCanFeedback feedback;
static volatile uint8_t bus_fault;
static int16_t previous_speed,oldest_speed;
static uint8_t speed_samples;
static int16_t median3(int16_t a,int16_t b,int16_t c) {
    if(a>b) { int16_t t=a;a=b;b=t; }
    if(b>c) { int16_t t=b;b=c;c=t; }
    return a>b?a:b;
}

HAL_StatusTypeDef MotorCan_Init(void) {
    feedback=(MotorCanFeedback){0}; bus_fault=0; speed_samples=0;
    hcan.Instance=CAN1;
    /* APB1=36 MHz / (2 * (1+13+4)) = 1 Mbps, as in the old motor project. */
    hcan.Init.Prescaler=2; hcan.Init.Mode=CAN_MODE_NORMAL;
    hcan.Init.SyncJumpWidth=CAN_SJW_1TQ;
    hcan.Init.TimeSeg1=CAN_BS1_13TQ; hcan.Init.TimeSeg2=CAN_BS2_4TQ;
    hcan.Init.TimeTriggeredMode=DISABLE; hcan.Init.AutoBusOff=ENABLE;
    hcan.Init.AutoWakeUp=DISABLE; hcan.Init.AutoRetransmission=DISABLE;
    hcan.Init.ReceiveFifoLocked=DISABLE; hcan.Init.TransmitFifoPriority=ENABLE;
    if(HAL_CAN_Init(&hcan)!=HAL_OK) return HAL_ERROR;
    CAN_FilterTypeDef filter={0};
    filter.FilterBank=0; filter.FilterMode=CAN_FILTERMODE_IDMASK;
    filter.FilterScale=CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh=0x201u<<5; filter.FilterMaskIdHigh=0x7ffu<<5;
    filter.FilterMaskIdLow=0x0006; /* Match standard DATA frames (IDE=RTR=0). */
    filter.FilterFIFOAssignment=CAN_RX_FIFO0; filter.FilterActivation=ENABLE;
    filter.SlaveStartFilterBank=14;
    if(HAL_CAN_ConfigFilter(&hcan,&filter)!=HAL_OK || HAL_CAN_Start(&hcan)!=HAL_OK) return HAL_ERROR;
    return HAL_CAN_ActivateNotification(&hcan,CAN_IT_RX_FIFO0_MSG_PENDING|CAN_IT_BUSOFF|CAN_IT_ERROR);
}
void MotorCan_Snapshot(MotorCanFeedback *out) {
    uint32_t mask=__get_PRIMASK(); __disable_irq();
    *out=feedback;
    if(!mask) __enable_irq();
}
uint8_t MotorCan_TakeFault(void) {
    uint32_t mask=__get_PRIMASK(); __disable_irq();
    uint8_t fault=bus_fault || ((hcan.Instance->ESR&CAN_ESR_BOFF)!=0);
    bus_fault=0;
    if(!mask) __enable_irq();
    return fault;
}
void MotorCan_CancelPending(void) {
    (void)HAL_CAN_AbortTxRequest(&hcan,CAN_TX_MAILBOX0|CAN_TX_MAILBOX1|CAN_TX_MAILBOX2);
}
HAL_StatusTypeDef MotorCan_Send(int16_t current) {
    if(current>(int16_t)MOTOR_CURRENT_LIMIT) current=(int16_t)MOTOR_CURRENT_LIMIT;
    if(current< -(int16_t)MOTOR_CURRENT_LIMIT) current= -(int16_t)MOTOR_CURRENT_LIMIT;
    CAN_TxHeaderTypeDef header={0}; uint32_t mailbox;
    uint16_t encoded=(uint16_t)current;
    uint8_t data[8]={(uint8_t)(encoded>>8),(uint8_t)encoded,0,0,0,0,0,0};
    header.StdId=0x200; header.IDE=CAN_ID_STD; header.RTR=CAN_RTR_DATA; header.DLC=8;
    HAL_StatusTypeDef result=HAL_CAN_AddTxMessage(&hcan,&header,data,&mailbox);
    if(result==HAL_OK) ++feedback.tx_count; else ++feedback.tx_error_count;
    return result;
}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *can) {
    if(can->Instance!=CAN1) return;
    /* Bound ISR work; hardware FIFO contains at most three frames. */
    for(unsigned i=0;i<3 && HAL_CAN_GetRxFifoFillLevel(can,CAN_RX_FIFO0);i++) {
        CAN_RxHeaderTypeDef header; uint8_t data[8];
        if(HAL_CAN_GetRxMessage(can,CAN_RX_FIFO0,&header,data)!=HAL_OK) { ++feedback.invalid_rx_count; break; }
        if(header.IDE!=CAN_ID_STD || header.RTR!=CAN_RTR_DATA || header.StdId!=0x201 || header.DLC!=8) {
            ++feedback.invalid_rx_count; continue;
        }
        uint32_t now=HAL_GetTick();
        if(feedback.received && (uint32_t)(now-feedback.last_rx_ms)>=MOTOR_FEEDBACK_TIMEOUT_MS) speed_samples=0;
        feedback.encoder=(uint16_t)((uint16_t)data[0]<<8)|data[1];
        int16_t raw=(int16_t)((uint16_t)data[2]<<8|data[3]);
        feedback.speed_rpm=raw;
        if(speed_samples<2) { feedback.speed_median_rpm=raw; ++speed_samples; }
        else feedback.speed_median_rpm=median3(oldest_speed,previous_speed,raw);
        int32_t difference=(int32_t)raw-feedback.speed_median_rpm;
        if(difference>300 || difference< -300) {
            ++feedback.speed_spike_count; feedback.spike_ms=now;
            feedback.spike_oldest=oldest_speed; feedback.spike_previous=previous_speed; feedback.spike_latest=raw;
        }
        oldest_speed=previous_speed; previous_speed=raw;
        feedback.measured_current=(int16_t)((uint16_t)data[4]<<8|data[5]);
        feedback.temperature=data[6]; feedback.last_rx_ms=now;
        ++feedback.rx_count; feedback.received=1;
    }
}
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *can) {
    if(can->Instance!=CAN1) return;
    ++feedback.error_count;
    if(can->Instance->ESR&CAN_ESR_BOFF) bus_fault=1;
}
