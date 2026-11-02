#include "motor_uart.h"
#include <string.h>
volatile MotorUartDebug g_motor_uart_debug;
static uint8_t rx_byte,rx[128],tx[ML_FRAME_SIZE];
static uint32_t stamps[128],last_byte_ms,last_report_ms,boot_ms;
static volatile uint16_t head,tail;
static volatile uint8_t rx_fault;
static MlParser parser;
HAL_StatusTypeDef MotorUart_Init(void) {
    head=tail=0;rx_fault=0;memset(&parser,0,sizeof(parser));boot_ms=HAL_GetTick();
    return HAL_UART_Receive_IT(&huart_motor,&rx_byte,1);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *port) {
    if(port!=&huart_motor) return;
    uint16_t next=(uint16_t)((head+1)&127u);g_motor_uart_debug.rx_bytes++;
    if(next==tail) {g_motor_uart_debug.overflow++;rx_fault=1;}
    else {rx[head]=rx_byte;stamps[head]=HAL_GetTick();__DMB();head=next;}
    if(HAL_UART_Receive_IT(port,&rx_byte,1)!=HAL_OK) rx_fault=1;
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *port) {
    if(port!=&huart_motor) return;
    g_motor_uart_debug.errors++;rx_fault=1;
    (void)HAL_UART_AbortReceive(port);(void)HAL_UART_Receive_IT(port,&rx_byte,1);
}
void MotorUart_Poll(MotorRemote *r,MotorControl *c,uint32_t now,uint8_t fault) {
    /* Receiver reboot/radio loss is covered by screen-originated two-way leases. */
    if(rx_fault || (uint32_t)(now-boot_ms)<ML_TIMEOUT_MS) {
        uint32_t mask=__get_PRIMASK();__disable_irq();tail=head;rx_fault=0;if(!mask) __enable_irq();
        parser.used=0;
        if(r->session) MotorRemote_Invalidate(r,c,MOTOR_REASON_REMOTE_TIMEOUT);
        if(huart_motor.RxState==HAL_UART_STATE_READY) (void)HAL_UART_Receive_IT(&huart_motor,&rx_byte,1);
        return;
    }
    for(unsigned budget=128;budget && tail!=head;--budget) {
        uint16_t pos=tail;uint8_t byte=rx[pos];uint32_t stamp=stamps[pos];__DMB();tail=(uint16_t)((pos+1)&127u);
        if((uint32_t)(now-stamp)>=ML_PERIOD_MS) {parser.used=0;g_motor_uart_debug.stale++;continue;}
        if((uint32_t)(stamp-last_byte_ms)>50) parser.used=0;
        last_byte_ms=stamp;MlMessage m;
        if(ml_parse(&parser,byte,&m)) {
            g_motor_uart_debug.rx_frames++;g_motor_uart_debug.last_rx_ms=stamp;
            MotorRemote_Receive(r,c,&m,stamp,fault);
        }
    }
}
void MotorUart_Report(MotorRemote *r,const MotorControl *c,uint32_t now) {
    if(huart_motor.gState!=HAL_UART_STATE_READY || (!r->dirty && (uint32_t)(now-last_report_ms)<ML_PERIOD_MS)) return;
    MlMessage m;MotorRemote_Status(r,c,now,&m);ml_encode(&m,tx);
    /* The static buffer remains untouched until the interrupt transfer completes. */
    if(HAL_UART_Transmit_IT(&huart_motor,tx,sizeof(tx))==HAL_OK) {
        last_report_ms=now;g_motor_uart_debug.tx_frames++;
    } else {r->dirty=1;g_motor_uart_debug.errors++;}
}
