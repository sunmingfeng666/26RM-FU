#ifndef FU_MASTER_DBUS_H
#define FU_MASTER_DBUS_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define DBUS_SWITCH_UP     1U
#define DBUS_SWITCH_DOWN   2U
#define DBUS_SWITCH_MIDDLE 3U

typedef struct
{
    volatile int16_t channel[4];
    volatile uint8_t right_switch;
    volatile uint8_t left_switch;
    volatile uint8_t online;
    volatile uint32_t last_frame_ms;
    volatile uint32_t valid_frame_count;
    volatile uint32_t invalid_frame_count;
    volatile uint32_t uart_error_count;
} DbusDebug_t;

extern volatile DbusDebug_t g_dbus_debug;

HAL_StatusTypeDef Dbus_Init(UART_HandleTypeDef *huart);
void Dbus_Poll(uint32_t now_ms);
uint8_t Dbus_IsOnline(void);
uint8_t Dbus_GetRightSwitch(void);

#endif
