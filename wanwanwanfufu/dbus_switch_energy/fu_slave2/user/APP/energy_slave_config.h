#ifndef ENERGY_SLAVE_CONFIG_H
#define ENERGY_SLAVE_CONFIG_H

#include <stdint.h>

/* 五块F1使用同一套源码，通过编译宏选择板号；当前默认先验证1号从机。 */
#ifndef ENERGY_SLAVE_INDEX
#define ENERGY_SLAVE_INDEX 1U
#endif

#if (ENERGY_SLAVE_INDEX < 1U) || (ENERGY_SLAVE_INDEX > 5U)
#error "ENERGY_SLAVE_INDEX must be in range 1..5"
#endif

#define ENERGY_FLOW_PERIOD_MS  150U
#define ENERGY_KEY_DEBOUNCE_MS  10U

#endif
