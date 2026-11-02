#include "slave_key_test.h"
#include "ws2812.h"

#define TEST_BRIGHTNESS 32U
#define REFRESH_PERIOD_MS 500U

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    uint16_t first_led;
    uint16_t last_led;
} TestKey;

/* 沿用正常灯板的九个低电平按键、DIN0环区索引；其余五路始终全亮。 */
static const TestKey kKeys[SLAVE_KEY_TEST_KEY_COUNT] = {
    {GPIOB, GPIO_PIN_10,   1U,  47U},
    {GPIOB, GPIO_PIN_11,  48U,  89U},
    {GPIOB, GPIO_PIN_12,  90U, 133U},
    {GPIOB, GPIO_PIN_13, 134U, 173U},
    {GPIOB, GPIO_PIN_14, 174U, 209U},
    {GPIOB, GPIO_PIN_15, 210U, 229U},
    {GPIOA, GPIO_PIN_4,  230U, 249U},
    {GPIOB, GPIO_PIN_6,  250U, 261U},
    {GPIOB, GPIO_PIN_7,  262U, 270U}
};

volatile SlaveKeyTestDebug g_slave_key_test_debug;
static volatile uint8_t s_ready;
static uint8_t s_candidate[SLAVE_KEY_TEST_KEY_COUNT];
static uint8_t s_debounce_ms[SLAVE_KEY_TEST_KEY_COUNT];
static volatile uint32_t s_last_press_ms[SLAVE_KEY_TEST_KEY_COUNT];
static volatile uint32_t s_seen_press_mask;
static uint32_t s_last_refresh_ms;
static uint32_t s_last_shown_mask;

void SlaveKeyTest_1msTick(void)
{
    if (s_ready == 0U)
    {
        return;
    }

    uint32_t raw_mask = 0U;
    uint32_t now_ms = HAL_GetTick();
    for (uint32_t i = 0U; i < SLAVE_KEY_TEST_KEY_COUNT; ++i)
    {
        uint32_t bit = 1UL << i;
        uint8_t pressed = (HAL_GPIO_ReadPin(kKeys[i].port, kKeys[i].pin) == GPIO_PIN_RESET);
        if (pressed != 0U)
        {
            raw_mask |= bit;
        }

        if (pressed != s_candidate[i])
        {
            s_candidate[i] = pressed;
            s_debounce_ms[i] = 0U;
        }
        else if (s_debounce_ms[i] < SLAVE_KEY_TEST_DEBOUNCE_MS)
        {
            ++s_debounce_ms[i];
            if ((s_debounce_ms[i] == SLAVE_KEY_TEST_DEBOUNCE_MS) &&
                (pressed != ((g_slave_key_test_debug.stable_mask & bit) != 0U)))
            {
                if (pressed != 0U)
                {
                    g_slave_key_test_debug.stable_mask |= bit;
                    g_slave_key_test_debug.last_key = i + 1U;
                    g_slave_key_test_debug.last_press_ms = now_ms;
                    ++g_slave_key_test_debug.press_count;
                    ++g_slave_key_test_debug.key_press_count[i];
                    s_last_press_ms[i] = now_ms;
                    s_seen_press_mask |= bit;
                }
                else
                {
                    g_slave_key_test_debug.stable_mask &= ~bit;
                    ++g_slave_key_test_debug.release_count;
                    ++g_slave_key_test_debug.key_release_count[i];
                }
            }
        }
    }
    g_slave_key_test_debug.raw_mask = raw_mask;
    ++g_slave_key_test_debug.scan_count;
}

void SlaveKeyTest_Update(void)
{
    uint32_t last_press_ms[SLAVE_KEY_TEST_KEY_COUNT];
    /* 短临界区只取一致快照；整个DMA发送期间继续运行SysTick按键扫描。 */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint32_t now_ms = HAL_GetTick();
    uint32_t shown_mask = g_slave_key_test_debug.stable_mask;
    uint32_t seen_mask = s_seen_press_mask;
    for (uint32_t i = 0U; i < SLAVE_KEY_TEST_KEY_COUNT; ++i)
    {
        last_press_ms[i] = s_last_press_ms[i];
    }
    __set_PRIMASK(primask);

    for (uint32_t i = 0U; i < SLAVE_KEY_TEST_KEY_COUNT; ++i)
    {
        uint32_t bit = 1UL << i;
        if (((seen_mask & bit) != 0U) &&
            ((uint32_t)(now_ms - last_press_ms[i]) < SLAVE_KEY_TEST_HIGHLIGHT_MS))
        {
            shown_mask |= bit;
        }
    }

    if ((shown_mask != s_last_shown_mask) ||
        ((uint32_t)(now_ms - s_last_refresh_ms) >= REFRESH_PERIOD_MS))
    {
        WS2812_SetAll(TEST_BRIGHTNESS, 0U, 0U);
        for (uint32_t key = 0U; key < SLAVE_KEY_TEST_KEY_COUNT; ++key)
        {
            if ((shown_mask & (1UL << key)) != 0U)
            {
                for (uint16_t led = kKeys[key].first_led; led <= kKeys[key].last_led; ++led)
                {
                    WS2812_SetPixel(led, 0U, TEST_BRIGHTNESS, 0U);
                }
            }
        }
        WS2812_SendAll();
        s_last_refresh_ms = HAL_GetTick();
        s_last_shown_mask = shown_mask;
        g_slave_key_test_debug.shown_mask = shown_mask;
        g_slave_key_test_debug.last_frame_ms = s_last_refresh_ms;
        ++g_slave_key_test_debug.frame_count;
    }
}

HAL_StatusTypeDef SlaveKeyTest_Init(void)
{
    s_ready = 0U;
    g_slave_key_test_debug = (SlaveKeyTestDebug){.enabled = 1U};
    s_seen_press_mask = 0U;
    for (uint32_t i = 0U; i < SLAVE_KEY_TEST_KEY_COUNT; ++i)
    {
        s_candidate[i] = 0U;
        s_debounce_ms[i] = 0U;
        s_last_press_ms[i] = 0U;
    }
    s_last_refresh_ms = HAL_GetTick();
    s_last_shown_mask = UINT32_MAX;

    /* CAN未初始化，PA12保持隐性，避免测试固件占用总线。 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);

    WS2812_Init();
    s_ready = 1U;
    SlaveKeyTest_Update();
    return HAL_OK;
}
