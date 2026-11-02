#include "slave_lamp_test.h"
#include "ws2812.h"

volatile SlaveLampTestDebug g_slave_lamp_test_debug={.enabled=1,.red=32,.green=0,.blue=0};
static uint32_t last_refresh_ms;

static void show_test_frame(void)
{
    /* All six original outputs, steady low-brightness red; never send an OFF frame. */
    WS2812_SetAll(32U,0U,0U);
    WS2812_SendAll();
    last_refresh_ms=HAL_GetTick();
    g_slave_lamp_test_debug.frame_count++;
    g_slave_lamp_test_debug.last_frame_ms=last_refresh_ms;
}

HAL_StatusTypeDef SlaveLampTest_Init(void)
{
    /* Keep transceiver TXD recessive while CAN remains uninitialized. */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,GPIO_PIN_SET);
    GPIO_InitTypeDef gpio={0};
    gpio.Pin=GPIO_PIN_12;gpio.Mode=GPIO_MODE_OUTPUT_PP;gpio.Pull=GPIO_NOPULL;gpio.Speed=GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA,&gpio);
    WS2812_Init();
    show_test_frame();
    return HAL_OK;
}

void SlaveLampTest_Update(void)
{
    /* Refresh unchanged data so LED power may come up after MCU power. */
    if((uint32_t)(HAL_GetTick()-last_refresh_ms)>=500U) show_test_frame();
}
