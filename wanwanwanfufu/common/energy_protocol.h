#ifndef ENERGY_PROTOCOL_H
#define ENERGY_PROTOCOL_H

#include <stdint.h>

typedef enum
{
    ENERGY_MODE_STOP = 0U,
    ENERGY_MODE_BIG = 1U,
    ENERGY_MODE_SMALL = 2U
} EnergyMode_e;

#define ENERGY_SLAVE_COUNT       5U
#define ENERGY_CAN_ALL_OFF_ID    0x2FFU

#define ENERGY_CAN_ID_OFFSET(index) (((uint32_t)(index) - 1U) * 0x10U)
#define ENERGY_CAN_ON_ID(index)      (0x201U + ENERGY_CAN_ID_OFFSET(index))
#define ENERGY_CAN_OFF_ID(index)     (0x202U + ENERGY_CAN_ID_OFFSET(index))
#define ENERGY_CAN_HIT_ID(index)     (0x301U + ENERGY_CAN_ID_OFFSET(index))

#define ENERGY_MODE_IS_VALID(mode) \
    (((mode) == ENERGY_MODE_BIG) || ((mode) == ENERGY_MODE_SMALL))

#endif
