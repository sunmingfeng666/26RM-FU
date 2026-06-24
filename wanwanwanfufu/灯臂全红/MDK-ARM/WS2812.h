#ifndef __WS2812_H
#define __WS2812_H

#include "main.h"
#include "tim.h"

extern TIM_HandleTypeDef htim2;

// 3条灯带颗数，按实际接线为 PA1 / PA6 / PB0
#define STRIP1_LED_COUNT  271
#define STRIP2_LED_COUNT  92
#define STRIP3_LED_COUNT  92
#define TOTAL_LED_COUNT   (STRIP1_LED_COUNT + STRIP2_LED_COUNT + STRIP3_LED_COUNT)

// 缓冲区按最大条(301颗)分配
#define WS2812_BUFFER_SIZE (STRIP1_LED_COUNT * 24 + 50)
#define WS2812_PWM_LOW  29
#define WS2812_PWM_HIGH 57

typedef struct {
    uint8_t R;
    uint8_t G;
    uint8_t B;
} WS2812_Color_t;

void WS2812_Init(void);
void WS2812_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b);
void WS2812_SendAll(void);

#endif
