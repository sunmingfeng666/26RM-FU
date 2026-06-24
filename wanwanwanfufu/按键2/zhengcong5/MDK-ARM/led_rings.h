#ifndef __LED_RINGS_H
#define __LED_RINGS_H

#include "main.h"
#include "ws2812.h"

#define BTN_COLOR_PORT   GPIOB
#define BTN_COLOR_PIN    GPIO_PIN_10

#define BTN_RING1_PORT   GPIOB
#define BTN_RING1_PIN    GPIO_PIN_12

#define BTN_RING2_PORT   GPIOB
#define BTN_RING2_PIN    GPIO_PIN_13

#define BTN_RING3_PORT   GPIOB
#define BTN_RING3_PIN    GPIO_PIN_14

#define BTN_RESET_PORT   GPIOB
#define BTN_RESET_PIN    GPIO_PIN_11

#define BTN_RING4_PORT   GPIOB
#define BTN_RING4_PIN    GPIO_PIN_15

#define BTN_RING5_PORT   GPIOA
#define BTN_RING5_PIN    GPIO_PIN_4

#define BTN_RING6_PORT   GPIOB
#define BTN_RING6_PIN    GPIO_PIN_6

#define BTN_RING7_PORT   GPIOB
#define BTN_RING7_PIN    GPIO_PIN_7

typedef enum
{
    ARMOR_LED_OFF = 0,
    ARMOR_LED_ON_ALL
} ArmorLedState_t;

void Rings_Init(void);
void Rings_HandleButtons(void);

void ArmorLED_Init(void);
void ArmorLED_ShowOff(void);
void ArmorLED_ShowOnAll(void);
void ArmorLED_ShowSingle(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void ArmorLED_ShowRange(uint16_t start_index, uint16_t end_index, uint8_t r, uint8_t g, uint8_t b);
void ArmorLED_ClearRange(uint16_t start_index, uint16_t end_index);
void ArmorLED_ShowIndices(const uint16_t *indices, uint16_t count, uint8_t r, uint8_t g, uint8_t b);
void jihuo(void);
void daijida(void);
void ArmorLED_SetState(ArmorLedState_t state);
void ArmorLED_ScanNext(void);

#endif
