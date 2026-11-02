#include "main.h"
#include "motor_can.h"
#include "motor_uart.h"
void NMI_Handler(void) { while(1) {} }
void HardFault_Handler(void) { while(1) {} }
void MemManage_Handler(void) { while(1) {} }
void BusFault_Handler(void) { while(1) {} }
void UsageFault_Handler(void) { while(1) {} }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}
void SysTick_Handler(void) { HAL_IncTick(); }
void USB_LP_CAN1_RX0_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan); }
void CAN1_SCE_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan); }
void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&huart_motor); }
