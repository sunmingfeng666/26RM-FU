#ifndef ENERGY_SLAVE_H
#define ENERGY_SLAVE_H

#include "stm32f1xx_hal.h"
#include "energy_protocol.h"
#include <stdint.h>

typedef struct
{
    volatile uint8_t slave_index;
    volatile EnergyMode_e mode;
    volatile uint8_t can_active;
    volatile uint8_t hit_done;
    volatile uint8_t last_hit_key;
    volatile uint32_t mode_change_count;
    volatile uint32_t hit_count;
} EnergySlaveDebug_t;

extern volatile EnergySlaveDebug_t g_energy_slave_debug;

HAL_StatusTypeDef EnergySlave_Init(void);
void EnergySlave_Update(void);
void EnergySlave_1msTick(void);

#endif
