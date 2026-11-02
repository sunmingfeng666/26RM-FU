#ifndef MOTOR_TEST_HAL_H
#define MOTOR_TEST_HAL_H
#include <stdint.h>
#include <stddef.h>
typedef int HAL_StatusTypeDef;
typedef struct { uint32_t ESR; } CAN_TypeDef;
extern CAN_TypeDef test_can_registers;
#define CAN1 (&test_can_registers)
typedef struct {
    uint32_t Prescaler,Mode,SyncJumpWidth,TimeSeg1,TimeSeg2,TimeTriggeredMode;
    uint32_t AutoBusOff,AutoWakeUp,AutoRetransmission,ReceiveFifoLocked,TransmitFifoPriority;
} CAN_InitTypeDef;
typedef struct { CAN_TypeDef *Instance; CAN_InitTypeDef Init; } CAN_HandleTypeDef;
typedef struct {
    uint32_t FilterBank,FilterMode,FilterScale,FilterIdHigh,FilterIdLow;
    uint32_t FilterMaskIdHigh,FilterMaskIdLow,FilterFIFOAssignment,FilterActivation,SlaveStartFilterBank;
} CAN_FilterTypeDef;
typedef struct { uint32_t StdId,ExtId,IDE,RTR,DLC,TransmitGlobalTime; } CAN_TxHeaderTypeDef;
typedef CAN_TxHeaderTypeDef CAN_RxHeaderTypeDef;
typedef struct { int Instance; } TIM_HandleTypeDef;
typedef struct { int gState,RxState; } UART_HandleTypeDef;
#define TIM2 2
#define TIM3 3
#define HAL_UART_STATE_READY 0
#define HAL_OK 0
#define HAL_ERROR 1
#define DISABLE 0
#define ENABLE 1
#define CAN_MODE_NORMAL 0
#define CAN_SJW_1TQ 0
#define CAN_BS1_13TQ 13
#define CAN_BS2_4TQ 4
#define CAN_FILTERMODE_IDMASK 0
#define CAN_FILTERSCALE_32BIT 1
#define CAN_RX_FIFO0 0
#define CAN_IT_RX_FIFO0_MSG_PENDING 1u
#define CAN_IT_BUSOFF 2u
#define CAN_IT_ERROR 4u
#define CAN_TX_MAILBOX0 1u
#define CAN_TX_MAILBOX1 2u
#define CAN_TX_MAILBOX2 4u
#define CAN_ID_STD 0
#define CAN_ID_EXT 4
#define CAN_RTR_DATA 0
#define CAN_RTR_REMOTE 2
#define CAN_ESR_BOFF 4u
#define GPIOB 1
#define GPIOA 2
#define GPIO_PIN_8 (1u<<8)
#define GPIO_PIN_10 (1u<<10)
#define GPIO_PIN_11 (1u<<11)
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
uint32_t HAL_GetTick(void);
int HAL_GPIO_ReadPin(int port,uint16_t pin);
static inline uint32_t __get_PRIMASK(void) { return 0; }
static inline void __disable_irq(void) {}
static inline void __enable_irq(void) {}
static inline void __DMB(void) {}
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *,uint8_t *,uint16_t);
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *);
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *,uint8_t *,uint16_t);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *);
HAL_StatusTypeDef HAL_CAN_Init(CAN_HandleTypeDef *);
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *,CAN_FilterTypeDef *);
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *);
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *,uint32_t);
HAL_StatusTypeDef HAL_CAN_AbortTxRequest(CAN_HandleTypeDef *,uint32_t);
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *,CAN_TxHeaderTypeDef *,uint8_t *,uint32_t *);
uint32_t HAL_CAN_GetRxFifoFillLevel(CAN_HandleTypeDef *,uint32_t);
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *,uint32_t,CAN_RxHeaderTypeDef *,uint8_t *);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *);
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *);
#endif
