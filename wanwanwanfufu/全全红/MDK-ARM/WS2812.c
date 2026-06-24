#include "ws2812.h"
#include <string.h>

// 全部LED的颜色数据
WS2812_Color_t LED_Data[TOTAL_LED_COUNT];

// 共用发送缓冲区，按最长灯带分配
uint16_t PWM_Buffer[WS2812_BUFFER_SIZE];

volatile uint8_t isSending = 0;

// 灯带配置表（参考powerrune24的strip数组思路）
typedef struct {
    uint16_t          count;   // 该条灯带颗数
    uint16_t          start;   // 在LED_Data中的起始索引
    TIM_HandleTypeDef *htim;   // 对应定时器
    uint32_t          channel; // 对应通道
} WS2812_Strip_t;

static WS2812_Strip_t strips[WS2812_STRIP_COUNT] = {
    {STRIP1_LED_COUNT, 0,                                        &htim2, TIM_CHANNEL_2},
    {STRIP2_LED_COUNT, STRIP1_LED_COUNT,                         &htim3, TIM_CHANNEL_1},
    {STRIP3_LED_COUNT, STRIP1_LED_COUNT + STRIP2_LED_COUNT,      &htim3, TIM_CHANNEL_3},
    {STRIP4_LED_COUNT, STRIP1_LED_COUNT + STRIP2_LED_COUNT + STRIP3_LED_COUNT, &htim2, TIM_CHANNEL_1},
    {STRIP5_LED_COUNT, STRIP1_LED_COUNT + STRIP2_LED_COUNT + STRIP3_LED_COUNT + STRIP4_LED_COUNT, &htim2, TIM_CHANNEL_4},
};

// 将指定灯带的颜色数据编码进PWM缓冲区
static void WS2812_FillBuffer(uint8_t strip_index)
{
    const WS2812_Strip_t *s = &strips[strip_index];
    uint32_t indx = 0;

    for (uint16_t i = 0; i < s->count; i++) {
        uint16_t led_idx = s->start + i;
        // WS2812B协议顺序：GRB
        uint32_t color = ((uint32_t)LED_Data[led_idx].G << 16) |
                         ((uint32_t)LED_Data[led_idx].R << 8)  |
                          LED_Data[led_idx].B;

        for (int j = 23; j >= 0; j--) {
            PWM_Buffer[indx++] = (color & (1u << j)) ? WS2812_PWM_HIGH : WS2812_PWM_LOW;
        }
    }
    // Reset信号：剩余位清零
    memset(&PWM_Buffer[indx], 0, 50 * sizeof(uint16_t));
}

void WS2812_Init(void)
{
    memset(LED_Data,   0, sizeof(LED_Data));
    memset(PWM_Buffer, 0, sizeof(PWM_Buffer));
}

void WS2812_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= TOTAL_LED_COUNT) return;
    LED_Data[index].R = r;
    LED_Data[index].G = g;
    LED_Data[index].B = b;
}

void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint16_t i = 0; i < TOTAL_LED_COUNT; i++) {
        WS2812_SetPixel(i, r, g, b);
    }
}

// 顺序发送4条灯带，每条发完再发下一条
void WS2812_SendAll(void)
{
    for (uint8_t i = 0; i < WS2812_STRIP_COUNT; i++) {
        WS2812_FillBuffer(i);
        HAL_TIM_PWM_Start_DMA(strips[i].htim, strips[i].channel,
                              (uint32_t *)PWM_Buffer,
                              strips[i].count * 24 + 50);
        HAL_Delay((strips[i].count * 30) / 1000 + 6);
        HAL_TIM_PWM_Stop_DMA(strips[i].htim, strips[i].channel);
    }
}

volatile uint8_t callback_called = 0;

// DMA发送完成回调
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
            HAL_TIM_PWM_Stop_DMA(&htim2, TIM_CHANNEL_1);
            isSending = 0;
        } else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
            HAL_TIM_PWM_Stop_DMA(&htim2, TIM_CHANNEL_2);
            isSending = 0;
        } else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4) {
            HAL_TIM_PWM_Stop_DMA(&htim2, TIM_CHANNEL_4);
            isSending = 0;
        }
    } else if (htim->Instance == TIM3) {
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
            HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);
            isSending = 0;
        } else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
            HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_2);
            isSending = 0;
        } else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
            HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_3);
            isSending = 0;
        }
    }
}
