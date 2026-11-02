#ifndef FU_MOTOR_UART_H
#define FU_MOTOR_UART_H
#include "motor_remote.h"
#include "stm32f1xx_hal.h"
extern UART_HandleTypeDef huart_motor;
typedef struct {uint32_t rx_bytes,rx_frames,tx_frames,errors,overflow,stale,last_rx_ms;} MotorUartDebug;
extern volatile MotorUartDebug g_motor_uart_debug;
HAL_StatusTypeDef MotorUart_HardwareInit(uint32_t *boot);
HAL_StatusTypeDef MotorUart_Init(void);
void MotorUart_Poll(MotorRemote *r,MotorControl *c,uint32_t now,uint8_t fault);
void MotorUart_Report(MotorRemote *r,const MotorControl *c,uint32_t now);
#endif
