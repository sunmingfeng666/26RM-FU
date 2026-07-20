#ifndef FU_MASTER_ENERGY_MASTER_H
#define FU_MASTER_ENERGY_MASTER_H

#include "stm32f4xx_hal.h"
#include "energy_protocol.h"
#include <stdint.h>

typedef enum
{
    ENERGY_MASTER_STOPPED = 0U,
    ENERGY_MASTER_BIG_PICK_TWO,
    ENERGY_MASTER_BIG_WAIT_FIRST,
    ENERGY_MASTER_BIG_WAIT_SECOND,
    ENERGY_MASTER_SMALL_PICK_ONE,
    ENERGY_MASTER_SMALL_WAIT_HIT,
    ENERGY_MASTER_SMALL_ROUND_DELAY,
    ENERGY_MASTER_LOW_VOLTAGE
} EnergyMasterState_e;

typedef struct
{
    volatile uint32_t uptime_ms;
    volatile uint8_t dbus_online;
    volatile uint8_t dbus_right_switch;
    volatile EnergyMode_e mode;
    volatile EnergyMasterState_e state;
    volatile uint8_t target1;
    volatile uint8_t target2;
    volatile uint8_t big_round_in_game;
    volatile uint32_t big_completed_games;
    volatile uint8_t small_hit_mask;
    volatile uint16_t battery_raw;
    volatile uint32_t battery_mv;
    volatile uint16_t battery_low_count;
    volatile uint8_t low_voltage;
    volatile uint32_t can_rx_count;
    volatile uint32_t can_rx_error_count;
    volatile uint32_t can_tx_count;
    volatile uint32_t can_tx_error_count;
} EnergyMasterDebug_t;

extern volatile EnergyMasterDebug_t g_energy_master_debug;

HAL_StatusTypeDef EnergyMaster_Init(void);
void EnergyMaster_Update(void);
void EnergyMaster_1msTick(void);

#endif
