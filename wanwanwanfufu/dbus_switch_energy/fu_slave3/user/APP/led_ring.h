#ifndef FU_WAN_CLION_LED_RING_H
#define FU_WAN_CLION_LED_RING_H

#include <stdint.h>

void LedRing_Init(void);
void LedRing_ShowIdle(void);
void LedRing_ShowOff(void);
void LedRing_ShowActive(uint16_t phase_offset);
void LedRing_ShowHit(uint8_t key_index);

#endif
