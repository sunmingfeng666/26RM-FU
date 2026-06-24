#include "led_rings.h"

#define LED_R 255
#define LED_G 0
#define LED_B 0

typedef enum {
    COLOR_RED = 0,
    //COLOR_GREEN,
    COLOR_BLUE,
    //���������������ж���ɫѭ������
    COLOR_MAX_COUNT 
} TargetColor_t;

static ArmorLedState_t g_armor_led_state = ARMOR_LED_OFF;
static TargetColor_t  CurrentColor = COLOR_RED;
static uint8_t Ring1_Active = 1;
static uint8_t Ring2_Active = 1;
static uint8_t Ring3_Active = 1;
static uint8_t Ring4_Active = 1;

const uint8_t RED_VAL[3]   = {255, 0, 0};
const uint8_t GREEN_VAL[3] = {0, 255, 0};
const uint8_t BLUE_VAL[3]  = {0, 0, 255};


static void Update_LEDs(void) {
    uint8_t r, g, b;

    switch (CurrentColor) {
        case COLOR_RED:
            r = RED_VAL[0]; g = RED_VAL[1]; b = RED_VAL[2];
            break;
//        case COLOR_GREEN:
//            r = GREEN_VAL[0]; g = GREEN_VAL[1]; b = GREEN_VAL[2];
//            break;
        case COLOR_BLUE:
            r = BLUE_VAL[0]; g = BLUE_VAL[1]; b = BLUE_VAL[2];
            break;
        default:
            r = 255; g = 0; b = 0;
            break;
    }

    for (int i = 0; i < STRIP1_LED_COUNT; i++) {
        if (Ring1_Active) {
            WS2812_SetPixel(i, r, g, b);
        } else {
            WS2812_SetPixel(i, 0, 0, 0); 
        }
    }

	for (int i = STRIP1_LED_COUNT; i < STRIP1_LED_COUNT + STRIP2_LED_COUNT; i++) {
        if (Ring2_Active) {
            WS2812_SetPixel(i, r, g, b);
        } else {
            WS2812_SetPixel(i, 0, 0, 0); 
        }
    }


    for (int i = STRIP1_LED_COUNT + STRIP2_LED_COUNT;
         i < STRIP1_LED_COUNT + STRIP2_LED_COUNT + STRIP3_LED_COUNT;
         i++) {
        if (Ring3_Active) {
            WS2812_SetPixel(i, r, g, b);
        } else {
            WS2812_SetPixel(i, 0, 0, 0); 
        }
    }

    for (int i = STRIP1_LED_COUNT + STRIP2_LED_COUNT + STRIP3_LED_COUNT;
         i < TOTAL_LED_COUNT;
         i++) {
        if (Ring4_Active) {
            WS2812_SetPixel(i, r, g, b);
        } else {
            WS2812_SetPixel(i, 0, 0, 0);
        }
    }

    WS2812_SendAll();
}

void Rings_Init(void) {
    WS2812_Init();
    Update_LEDs(); 
}

uint8_t Is_Button_Pressed0(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET) { 
        HAL_Delay(20); 
        if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET) {
            //while(HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET); 
            return 1;
        }
    }
    return 0;
}

uint8_t Is_Button_Pressed1(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET) { 
        HAL_Delay(20); 
        if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET) {
            while(HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET); 
            return 1;
        }
    }
    return 0;
}
uint8_t needUpdate = 0;
void Rings_HandleButtons(void) {
    

    if (Is_Button_Pressed1(BTN_COLOR_PORT, BTN_COLOR_PIN)) {
        CurrentColor++;

        if (CurrentColor >= COLOR_MAX_COUNT) {
            CurrentColor = COLOR_RED;
        }
        needUpdate = 1;
    }

    if (Is_Button_Pressed0(BTN_RING1_PORT, BTN_RING1_PIN)) {
        Ring1_Active = 0;
        needUpdate = 1;
    }

    if (Is_Button_Pressed0(BTN_RING2_PORT, BTN_RING2_PIN)) {
        Ring2_Active = 0;
        needUpdate = 1;
    }

    if (Is_Button_Pressed0(BTN_RING3_PORT, BTN_RING3_PIN)) {
        Ring3_Active = 0;
        needUpdate = 1;
    }

    if (Is_Button_Pressed1(BTN_RESET_PORT, BTN_RESET_PIN)) {
        Ring1_Active = 1;
        Ring2_Active = 1;
        Ring3_Active = 1;
        Ring4_Active = 1;
        needUpdate = 1;
    }

    if (needUpdate) {
        Update_LEDs();
    }
}

void ArmorLED_ShowOff(void)
{
    WS2812_SetAll(0, 0, 0);
    WS2812_SendAll();
    g_armor_led_state = ARMOR_LED_OFF;
}



void ArmorLED_ShowOnAll(void)
{
    WS2812_SetAll(LED_R, LED_G, LED_B);
    WS2812_SendAll();
    g_armor_led_state = ARMOR_LED_ON_ALL;
}

void ArmorLED_SetState(ArmorLedState_t state)
{
    switch (state)
    {
        case ARMOR_LED_OFF:
            ArmorLED_ShowOff();
            break;

        case ARMOR_LED_ON_ALL:
            ArmorLED_ShowOnAll();
            break;

        default:
            break;
    }
}

void ArmorLED_Init(void)
{
    WS2812_Init();
    ArmorLED_ShowOff();
}
