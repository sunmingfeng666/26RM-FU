#ifndef CAN_SLAVE_H
#define CAN_SLAVE_H

#include "stm32f1xx_hal.h"
#include "energy_protocol.h"
#include <stdint.h>

typedef enum
{
    CAN_SLAVE_COMMAND_NONE = 0U,
    CAN_SLAVE_COMMAND_ON,
    CAN_SLAVE_COMMAND_LOCAL_OFF,
    CAN_SLAVE_COMMAND_ALL_OFF
} CanSlaveCommand_e;

typedef struct
{
    volatile uint8_t active;
    volatile EnergyMode_e mode;
    volatile CanSlaveCommand_e last_control_command;
    volatile uint32_t control_command_count;
    volatile uint32_t last_rx_id;
    volatile uint32_t rx_count;
    volatile uint32_t rx_error_count;
    volatile uint32_t hit_tx_count;
    volatile uint32_t hit_tx_error_count;
    volatile HAL_StatusTypeDef last_hit_tx_result;
} CanSlaveDebug_t;

extern volatile CanSlaveDebug_t g_can_slave_debug;

HAL_StatusTypeDef CanSlave_Init(CAN_HandleTypeDef *hcan);
uint8_t CanSlave_IsActive(void);
EnergyMode_e CanSlave_GetMode(void);
CanSlaveCommand_e CanSlave_GetLastControlCommand(void);
uint32_t CanSlave_GetControlCommandCount(void);
HAL_StatusTypeDef CanSlave_SendHitFeedback(void);

#endif
