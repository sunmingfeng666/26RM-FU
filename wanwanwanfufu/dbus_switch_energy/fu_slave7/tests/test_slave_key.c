#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "slave_key_test.h"
#include "ws2812.h"

GPIO_TypeDef host_gpio_a, host_gpio_b;
static uint32_t now_ms, input_mask, irq_mask, frames;
static uint8_t inject_during_send;
static WS2812_Color_t pixels[TOTAL_LED_COUNT];

uint32_t HAL_GetTick(void) { return now_ms; }
uint32_t __get_PRIMASK(void) { return irq_mask; }
void __disable_irq(void) { irq_mask = 1U; }
void __set_PRIMASK(uint32_t mask) { irq_mask = mask; }

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    static const uint16_t pins[9] = {
        GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14,
        GPIO_PIN_15, GPIO_PIN_4, GPIO_PIN_6, GPIO_PIN_7
    };
    for (uint32_t i = 0U; i < 9U; ++i)
    {
        if ((port == ((i == 6U) ? GPIOA : GPIOB)) && (pin == pins[i]))
        {
            return ((input_mask & (1U << i)) != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET;
        }
    }
    assert(0 && "Unexpected GPIO input");
    return GPIO_PIN_SET;
}
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    assert(port == GPIOA && pin == GPIO_PIN_12 && state == GPIO_PIN_SET);
}
void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *gpio)
{
    assert(port == GPIOA && gpio->Pin == GPIO_PIN_12 && gpio->Mode == GPIO_MODE_OUTPUT_PP);
}
void WS2812_Init(void) { memset(pixels, 0, sizeof(pixels)); }
void WS2812_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    assert(index < TOTAL_LED_COUNT);
    pixels[index] = (WS2812_Color_t){r, g, b};
}
void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint16_t i = 0U; i < TOTAL_LED_COUNT; ++i) WS2812_SetPixel(i, r, g, b);
}
static void tick(uint32_t count)
{
    for (uint32_t i = 0U; i < count; ++i)
    {
        ++now_ms;
        assert(irq_mask == 0U);
        SlaveKeyTest_1msTick();
    }
}
void WS2812_SendAll(void)
{
    assert(irq_mask == 0U);
    ++frames;
    /* 在约60ms灯带发送窗口内按下/松开按键9，不给主循环任何扫描机会。 */
    if (inject_during_send != 0U)
    {
        inject_during_send = 0U;
        input_mask = 1U << 8;
        tick(15U);
        input_mask = 0U;
        tick(45U);
    }
    else tick(60U);
}
static void reset(uint32_t start_ms, uint32_t held_mask)
{
    now_ms = start_ms;
    input_mask = held_mask;
    irq_mask = frames = inject_during_send = 0U;
    assert(SlaveKeyTest_Init() == HAL_OK);
}
static void check_colors(uint32_t green_mask)
{
    const uint16_t first[9] = {1,48,90,134,174,210,230,250,262};
    const uint16_t last[9] = {47,89,133,173,209,229,249,261,270};
    for (uint16_t led = 0U; led < TOTAL_LED_COUNT; ++led)
    {
        uint8_t green = 0U;
        for (uint32_t key = 0U; key < 9U; ++key)
            if ((green_mask & (1U << key)) && led >= first[key] && led <= last[key]) green = 1U;
        assert(pixels[led].R == (green ? 0U : 32U));
        assert(pixels[led].G == (green ? 32U : 0U));
        assert(pixels[led].B == 0U);
    }
}
int main(void)
{
    /* 上电不凭空生成按键，六路所有灯珠红色。 */
    reset(0U, 0U);
    assert(g_slave_key_test_debug.press_count == 0U && g_slave_key_test_debug.stable_mask == 0U);
    check_colors(0U);

    /* 抖动和小于消抖窗口的毛刺不计数。 */
    for (uint32_t i = 0U; i < 30U; ++i) { input_mask = i & 1U; tick(1U); }
    input_mask = 0U; tick(20U);
    input_mask = 1U; tick(5U); input_mask = 0U; tick(20U);
    assert(g_slave_key_test_debug.press_count == 0U);

    /* 九键逐一验证映射，长按只计一次，松开计数后重按能够再计数。 */
    for (uint32_t key = 0U; key < 9U; ++key)
    {
        reset(0U, 0U);
        input_mask = 1U << key; tick(11U);
        assert(g_slave_key_test_debug.raw_mask == input_mask);
        assert(g_slave_key_test_debug.stable_mask == input_mask);
        assert(g_slave_key_test_debug.last_key == key + 1U);
        assert(g_slave_key_test_debug.press_count == 1U);
        assert(g_slave_key_test_debug.key_press_count[key] == 1U);
        SlaveKeyTest_Update(); check_colors(input_mask);
        tick(1000U); SlaveKeyTest_Update();
        assert(g_slave_key_test_debug.press_count == 1U);
        check_colors(input_mask);
        input_mask = 0U; tick(11U);
        assert(g_slave_key_test_debug.stable_mask == 0U);
        assert(g_slave_key_test_debug.release_count == 1U);
        assert(g_slave_key_test_debug.key_release_count[key] == 1U);
        SlaveKeyTest_Update(); check_colors(0U);
        input_mask = 1U << key; tick(11U);
        assert(g_slave_key_test_debug.press_count == 2U);
    }

    /* 九个按键同时按下，各自计数和显示；上电已按住也能识别。 */
    reset(0U, 0x1FFU);
    assert(g_slave_key_test_debug.stable_mask == 0x1FFU);
    assert(g_slave_key_test_debug.press_count == 9U);
    SlaveKeyTest_Update(); check_colors(0x1FFU);
    input_mask = 0U; tick(11U);
    assert(g_slave_key_test_debug.release_count == 9U);

    /* 发送期间短按仍被记录，松开后绿色保持至300ms；箭头仍全红。 */
    reset(0U, 0U);
    inject_during_send = 1U;
    tick(500U); SlaveKeyTest_Update();
    assert(g_slave_key_test_debug.press_count == 1U);
    assert(g_slave_key_test_debug.release_count == 1U);
    assert(g_slave_key_test_debug.last_key == 9U);
    assert(g_slave_key_test_debug.stable_mask == 0U);
    SlaveKeyTest_Update(); check_colors(1U << 8);
    now_ms = g_slave_key_test_debug.last_press_ms + 299U;
    SlaveKeyTest_Update(); check_colors(1U << 8);
    tick(1U); SlaveKeyTest_Update(); check_colors(0U);

    /* 毫秒回绕后消抖、显示保持时间和定期刷新仍有效。 */
    reset(UINT32_MAX - 80U, 0U);
    input_mask = 4U; tick(11U); input_mask = 0U; tick(11U);
    SlaveKeyTest_Update(); check_colors(4U);
    tick(300U); SlaveKeyTest_Update(); check_colors(0U);
    uint32_t before = frames;
    tick(500U); SlaveKeyTest_Update(); assert(frames == before + 1U);
    assert(g_slave_key_test_debug.scan_count > 0U);
    puts("PASS: GPIO mapping, all 865 pixels, debounce, nine independent keys, long hold/repress, boot held, send-time pulse, 300ms feedback, wraparound, refresh");
    return 0;
}
