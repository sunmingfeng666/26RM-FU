#ifndef SLAVE_LAMP_TEST_H
#define SLAVE_LAMP_TEST_H
#include "stm32f1xx_hal.h"
#include <stdint.h>
/* Read-only diagnostic snapshot; editing these fields does not control output. */
typedef struct {
    uint8_t enabled,red,green,blue;
    uint32_t frame_count,last_frame_ms;
} SlaveLampTestDebug;
extern volatile SlaveLampTestDebug g_slave_lamp_test_debug;
HAL_StatusTypeDef SlaveLampTest_Init(void);
void SlaveLampTest_Update(void);
#endif
