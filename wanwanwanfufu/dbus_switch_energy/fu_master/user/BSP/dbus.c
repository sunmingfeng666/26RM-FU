#include "dbus.h"

#define DBUS_FRAME_LENGTH        18U
#define DBUS_FRAME_GAP_MS         3U
#define DBUS_OFFLINE_TIMEOUT_MS 100U
#define DBUS_CHANNEL_MIN        364
#define DBUS_CHANNEL_MAX       1684

volatile DbusDebug_t g_dbus_debug = {0};

static UART_HandleTypeDef *s_dbus_uart = NULL;
static uint8_t s_rx_byte = 0U;
static uint8_t s_frame[DBUS_FRAME_LENGTH] = {0};
static volatile uint8_t s_frame_index = 0U;
static volatile uint32_t s_last_byte_ms = 0U;

static uint8_t Dbus_IsSwitchValid(uint8_t value)
{
    return (value == DBUS_SWITCH_UP) ||
           (value == DBUS_SWITCH_DOWN) ||
           (value == DBUS_SWITCH_MIDDLE);
}

static uint8_t Dbus_DecodeFrame(const uint8_t *frame)
{
    int16_t channel[4];
    uint8_t right_switch;
    uint8_t left_switch;

    channel[0] = (int16_t)((frame[0] | (frame[1] << 8U)) & 0x07FFU);
    channel[1] = (int16_t)(((frame[1] >> 3U) | (frame[2] << 5U)) & 0x07FFU);
    channel[2] = (int16_t)(((frame[2] >> 6U) |
                            (frame[3] << 2U) |
                            (frame[4] << 10U)) & 0x07FFU);
    channel[3] = (int16_t)(((frame[4] >> 1U) | (frame[5] << 7U)) & 0x07FFU);

    right_switch = (uint8_t)(((frame[5] >> 4U) & 0x0CU) >> 2U);
    left_switch = (uint8_t)((frame[5] >> 4U) & 0x03U);

    for (uint8_t i = 0U; i < 4U; ++i)
    {
        if ((channel[i] < DBUS_CHANNEL_MIN) || (channel[i] > DBUS_CHANNEL_MAX))
        {
            return 0U;
        }
    }

    if (!Dbus_IsSwitchValid(right_switch) || !Dbus_IsSwitchValid(left_switch))
    {
        return 0U;
    }

    for (uint8_t i = 0U; i < 4U; ++i)
    {
        g_dbus_debug.channel[i] = (int16_t)(channel[i] - 1024);
    }
    g_dbus_debug.right_switch = right_switch;
    g_dbus_debug.left_switch = left_switch;
    g_dbus_debug.last_frame_ms = HAL_GetTick();
    g_dbus_debug.valid_frame_count++;
    g_dbus_debug.online = 1U;
    return 1U;
}

static void Dbus_RestartReceive(void)
{
    if (s_dbus_uart != NULL)
    {
        (void)HAL_UART_Receive_IT(s_dbus_uart, &s_rx_byte, 1U);
    }
}

HAL_StatusTypeDef Dbus_Init(UART_HandleTypeDef *huart)
{
    if (huart == NULL)
    {
        return HAL_ERROR;
    }

    s_dbus_uart = huart;
    s_frame_index = 0U;
    s_last_byte_ms = HAL_GetTick();
    g_dbus_debug.online = 0U;
    g_dbus_debug.right_switch = DBUS_SWITCH_MIDDLE;
    g_dbus_debug.left_switch = DBUS_SWITCH_MIDDLE;
    return HAL_UART_Receive_IT(s_dbus_uart, &s_rx_byte, 1U);
}

void Dbus_Poll(uint32_t now_ms)
{
    if ((g_dbus_debug.valid_frame_count == 0U) ||
        ((now_ms - g_dbus_debug.last_frame_ms) > DBUS_OFFLINE_TIMEOUT_MS))
    {
        g_dbus_debug.online = 0U;
    }
}

uint8_t Dbus_IsOnline(void)
{
    return g_dbus_debug.online;
}

uint8_t Dbus_GetRightSwitch(void)
{
    return g_dbus_debug.right_switch;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint32_t now_ms;

    if ((s_dbus_uart == NULL) || (huart->Instance != s_dbus_uart->Instance))
    {
        return;
    }

    now_ms = HAL_GetTick();
    if ((now_ms - s_last_byte_ms) >= DBUS_FRAME_GAP_MS)
    {
        s_frame_index = 0U;
    }
    s_last_byte_ms = now_ms;

    s_frame[s_frame_index++] = s_rx_byte;
    if (s_frame_index >= DBUS_FRAME_LENGTH)
    {
        if (!Dbus_DecodeFrame(s_frame))
        {
            g_dbus_debug.invalid_frame_count++;
        }
        s_frame_index = 0U;
    }

    Dbus_RestartReceive();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if ((s_dbus_uart == NULL) || (huart->Instance != s_dbus_uart->Instance))
    {
        return;
    }

    g_dbus_debug.uart_error_count++;
    g_dbus_debug.online = 0U;
    s_frame_index = 0U;
    Dbus_RestartReceive();
}
