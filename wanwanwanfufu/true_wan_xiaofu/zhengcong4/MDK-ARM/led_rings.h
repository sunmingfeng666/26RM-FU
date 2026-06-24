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
void ArmorLED_SetState(ArmorLedState_t state);

#endif