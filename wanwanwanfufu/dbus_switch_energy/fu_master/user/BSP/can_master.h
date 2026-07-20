#ifndef FU_MASTER_CAN_MASTER_H
#define FU_MASTER_CAN_MASTER_H

#include "stm32f4xx_hal.h"
#include "energy_protocol.h"
#include <stdint.h>

typedef struct
{
    volatile uint32_t last_rx_id;
    volatile uint32_t rx_count;
    volatile uint32_t rx_error_count;
    volatile uint32_t tx_count;
    volatile uint32_t tx_error_count;
    volatile HAL_StatusTypeDef last_tx_result;
} CanMasterDebug_t;

extern volatile CanMasterDebug_t g_can_master_debug;

HAL_StatusTypeDef CanMaster_Init(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef CanMaster_SendTargetOn(uint8_t target, EnergyMode_e mode);
HAL_StatusTypeDef CanMaster_SendTargetOff(uint8_t target);
HAL_StatusTypeDef CanMaster_SendAllOff(void);
uint8_t CanMaster_TakeHitMask(void);
void CanMaster_ClearHitMask(void);

#endif
