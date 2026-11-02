#ifndef KEY_TEST_HOST_WS2812_H
#define KEY_TEST_HOST_WS2812_H
#include "main.h"
#define TOTAL_LED_COUNT 865U
typedef struct { uint8_t R, G, B; } WS2812_Color_t;
void WS2812_Init(void);
void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b);
void WS2812_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void WS2812_SendAll(void);
#endif
