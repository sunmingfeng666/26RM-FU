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

const uint8_t RED_VAL[3]   = {255, 0, 0};
const uint8_t GREEN_VAL[3] = {0, 255, 0};
const uint8_t BLUE_VAL[3]  = {0, 0, 255};

#define STRIP1_LED(local_index) ((uint16_t)(STRIP1_START_INDEX + (local_index)))

static const uint16_t g_target_pattern_leds[] = {
    STRIP1_LED(0),  STRIP1_LED(1),  STRIP1_LED(2),
    STRIP1_LED(10), STRIP1_LED(11), STRIP1_LED(12), STRIP1_LED(13), STRIP1_LED(14),
    STRIP1_LED(22), STRIP1_LED(23), STRIP1_LED(24), STRIP1_LED(25), STRIP1_LED(26),
    STRIP1_LED(34), STRIP1_LED(35), STRIP1_LED(36),
    STRIP1_LED(37), STRIP1_LED(38),
    STRIP1_LED(46), STRIP1_LED(47), STRIP1_LED(48),
    STRIP1_LED(48), STRIP1_LED(49), STRIP1_LED(50), STRIP1_LED(51), STRIP1_LED(52),
    STRIP1_LED(53), STRIP1_LED(54), STRIP1_LED(55), STRIP1_LED(56), STRIP1_LED(57),
    STRIP1_LED(58), STRIP1_LED(59), STRIP1_LED(60), STRIP1_LED(61), STRIP1_LED(62),
    STRIP1_LED(63), STRIP1_LED(64), STRIP1_LED(65), STRIP1_LED(66), STRIP1_LED(67),
    STRIP1_LED(68), STRIP1_LED(69), STRIP1_LED(70), STRIP1_LED(71), STRIP1_LED(72),
    STRIP1_LED(73), STRIP1_LED(74), STRIP1_LED(75), STRIP1_LED(76), STRIP1_LED(77),
    STRIP1_LED(78), STRIP1_LED(79), STRIP1_LED(80), STRIP1_LED(81), STRIP1_LED(82),
    STRIP1_LED(83), STRIP1_LED(84), STRIP1_LED(85), STRIP1_LED(86), STRIP1_LED(87),
    STRIP1_LED(88), STRIP1_LED(89),
    STRIP1_LED(90), STRIP1_LED(91),
    STRIP1_LED(101), STRIP1_LED(102),
    STRIP1_LED(112), STRIP1_LED(113),
    STRIP1_LED(123), STRIP1_LED(124),
    STRIP1_LED(134), STRIP1_LED(135),
    STRIP1_LED(144), STRIP1_LED(145),
    STRIP1_LED(154), STRIP1_LED(155),
    STRIP1_LED(164), STRIP1_LED(165),
    STRIP1_LED(174), STRIP1_LED(175), STRIP1_LED(176), STRIP1_LED(177), STRIP1_LED(178),
    STRIP1_LED(179), STRIP1_LED(180), STRIP1_LED(181), STRIP1_LED(182), STRIP1_LED(183),
    STRIP1_LED(184), STRIP1_LED(185), STRIP1_LED(186), STRIP1_LED(187), STRIP1_LED(188),
    STRIP1_LED(189), STRIP1_LED(190), STRIP1_LED(191), STRIP1_LED(192), STRIP1_LED(193),
    STRIP1_LED(194), STRIP1_LED(195), STRIP1_LED(196), STRIP1_LED(197), STRIP1_LED(198),
    STRIP1_LED(199), STRIP1_LED(200), STRIP1_LED(201), STRIP1_LED(202), STRIP1_LED(203),
    STRIP1_LED(204), STRIP1_LED(205), STRIP1_LED(206), STRIP1_LED(207), STRIP1_LED(208),
    STRIP1_LED(209), STRIP1_LED(210),
    STRIP1_LED(215), STRIP1_LED(220), STRIP1_LED(225),
    STRIP1_LED(230), STRIP1_LED(235), STRIP1_LED(240), STRIP1_LED(245),
    STRIP1_LED(262), STRIP1_LED(263), STRIP1_LED(264), STRIP1_LED(265), STRIP1_LED(266),
    STRIP1_LED(267), STRIP1_LED(268), STRIP1_LED(269), STRIP1_LED(270)
};

static void DrawTargetPattern(void)
{
    WS2812_SetAll(0, 0, 0);
    for (uint16_t i = 0; i < (sizeof(g_target_pattern_leds) / sizeof(g_target_pattern_leds[0])); ++i) {
        WS2812_SetPixel(g_target_pattern_leds[i], LED_R, LED_G, LED_B);
    }
}


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

    for (int i = 0; i < 271; i++) {
        if (Ring1_Active) {
            WS2812_SetPixel(i, r, g, b);
        } else {
            WS2812_SetPixel(i, 0, 0, 0); 
        }
    }

	for (int i = 271; i < 357; i++) {
        if (Ring2_Active) {
            WS2812_SetPixel(i, r, g, b);
        } else {
            WS2812_SetPixel(i, 0, 0, 0); 
        }
    }


    for (int i = 357; i < 449; i++) {
        if (Ring3_Active) {
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
    DrawTargetPattern();
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

void ArmorLED_ScanNext(void)
{
}

void ArmorLED_Init(void)
{
    WS2812_Init();
    ArmorLED_ShowOff();
}
