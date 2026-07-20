#include "led_ring.h"
#include "ws2812.h"

#define FIRST_THREE_LED_COUNT (STRIP1_LED_COUNT + STRIP2_LED_COUNT + STRIP3_LED_COUNT)
#define DIN3_START_INDEX      FIRST_THREE_LED_COUNT
#define DIN3_END_INDEX        (DIN3_START_INDEX + STRIP4_LED_COUNT - 1U)
#define DIN4_START_INDEX      (DIN3_END_INDEX + 1U)
#define DIN4_END_INDEX        (DIN4_START_INDEX + STRIP5_LED_COUNT - 1U)
#define DIN5_START_INDEX      (DIN4_END_INDEX + 1U)
#define DIN5_END_INDEX        (DIN5_START_INDEX + STRIP6_LED_COUNT - 1U)
#define BOARD_LED_COUNT       70U
#define BOARD_COUNT           4U
#define ARROW_GROUP_COUNT     7U
#define ARROW_GROUP_WIDTH     3U
#define ARROW_PHASE_COUNT     10U

static const uint16_t kSelectedPatternLeds[] = {
    0, 1, 2,
    10, 11, 12, 13, 14,
    22, 23, 24, 25, 26,
    34, 35, 36,
    37, 38,
    46, 47, 48,
    48, 49, 50, 51, 52,
    53, 54, 55, 56, 57,
    58, 59, 60, 61, 62,
    63, 64, 65, 66, 67,
    68, 69, 70, 71, 72,
    73, 74, 75, 76, 77,
    78, 79, 80, 81, 82,
    83, 84, 85, 86, 87,
    88, 89,
    90, 91,
    101, 102,
    112, 113,
    123, 124,
    134, 135,
    144, 145,
    154, 155,
    164, 165,
    174, 175, 176, 177, 178,
    179, 180, 181, 182, 183,
    184, 185, 186, 187, 188,
    189, 190, 191, 192, 193,
    194, 195, 196, 197, 198,
    199, 200, 201, 202, 203,
    204, 205, 206, 207, 208,
    209, 210,
    215, 220, 225,
    230, 235, 240, 245,
    262, 263, 264, 265, 266,
    267, 268, 269, 270
};

static const uint16_t kArrowGroupZoneStarts[ARROW_GROUP_COUNT] = {
    0U, 10U, 20U, 30U, 40U, 50U, 60U
};

static const uint16_t kArrowGroupLocalStarts[ARROW_GROUP_COUNT] = {
    3U, 2U, 1U, 0U, 1U, 2U, 3U
};

static const uint16_t kHitStart[9] = {
    1U, 48U, 90U, 134U, 174U, 210U, 230U, 250U, 262U
};

static const uint16_t kHitEnd[9] = {
    47U, 89U, 133U, 173U, 209U, 229U, 249U, 261U, 270U
};

static void LedRing_SetRange(uint16_t start_index, uint16_t end_index,
                             uint8_t red, uint8_t green, uint8_t blue)
{
    if (start_index >= TOTAL_LED_COUNT)
    {
        return;
    }
    if (end_index >= TOTAL_LED_COUNT)
    {
        end_index = TOTAL_LED_COUNT - 1U;
    }

    for (uint16_t i = start_index; i <= end_index; ++i)
    {
        WS2812_SetPixel(i, red, green, blue);
    }
}

static void LedRing_KeepDin5On(void)
{
    LedRing_SetRange(DIN5_START_INDEX, DIN5_END_INDEX, 255U, 0U, 0U);
}

static void LedRing_DrawSelectedPattern(void)
{
    for (uint16_t i = 0U;
         i < (sizeof(kSelectedPatternLeds) / sizeof(kSelectedPatternLeds[0]));
         ++i)
    {
        WS2812_SetPixel(kSelectedPatternLeds[i], 255U, 0U, 0U);
    }
}

static void LedRing_DrawFlow(uint16_t phase_offset)
{
    for (uint16_t board_index = 0U; board_index < BOARD_COUNT; ++board_index)
    {
        uint16_t board_start = (uint16_t)(board_index * BOARD_LED_COUNT);

        for (uint16_t group_index = 0U; group_index < ARROW_GROUP_COUNT; ++group_index)
        {
            uint16_t zone_start =
                (uint16_t)(board_start + kArrowGroupZoneStarts[group_index]);

            for (uint16_t pixel_offset = 0U;
                 pixel_offset < ARROW_GROUP_WIDTH;
                 ++pixel_offset)
            {
                uint16_t local_index =
                    (uint16_t)((kArrowGroupLocalStarts[group_index] +
                                phase_offset + pixel_offset) % ARROW_PHASE_COUNT);
                uint16_t local_pixel = (uint16_t)(zone_start + local_index);
                if (local_pixel < STRIP4_LED_COUNT)
                {
                    WS2812_SetPixel((uint16_t)(DIN3_START_INDEX + local_pixel),
                                    255U, 0U, 0U);
                }
            }
        }
    }
}

void LedRing_Init(void)
{
    WS2812_Init();
    LedRing_ShowIdle();
}

void LedRing_ShowIdle(void)
{
    WS2812_SetAll(0U, 0U, 0U);
    LedRing_KeepDin5On();
    WS2812_SendAll();
}

void LedRing_ShowOff(void)
{
    WS2812_SetAll(0U, 0U, 0U);
    WS2812_SendAll();
}

void LedRing_ShowActive(uint16_t phase_offset)
{
    WS2812_SetAll(0U, 0U, 0U);
    LedRing_DrawSelectedPattern();
    LedRing_DrawFlow(phase_offset);
    LedRing_KeepDin5On();
    WS2812_SendAll();
}

void LedRing_ShowHit(uint8_t key_index)
{
    WS2812_SetAll(0U, 0U, 0U);

    if ((key_index >= 1U) && (key_index <= 9U))
    {
        LedRing_SetRange(kHitStart[key_index - 1U],
                         kHitEnd[key_index - 1U],
                         255U, 0U, 0U);
    }

    LedRing_SetRange(DIN3_START_INDEX, DIN3_END_INDEX, 255U, 0U, 0U);
    LedRing_SetRange(DIN4_START_INDEX, DIN4_END_INDEX, 255U, 0U, 0U);
    LedRing_KeepDin5On();
    WS2812_SendAll();
}
