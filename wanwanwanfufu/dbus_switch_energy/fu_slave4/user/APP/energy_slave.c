#include "energy_slave.h"
#include "energy_slave_config.h"
#include "led_ring.h"
#include "can_slave.h"
#include "can.h"
#include "tim.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} EnergyButton_t;

volatile EnergySlaveDebug_t g_energy_slave_debug = {
    .slave_index = ENERGY_SLAVE_INDEX,
    .mode = ENERGY_MODE_SMALL
};

/* 新PCB共九个低电平有效击打按键，PA3已经作为第六路灯带输出。 */
static const EnergyButton_t kHitButtons[9] = {
    {GPIOB, GPIO_PIN_10},
    {GPIOB, GPIO_PIN_11},
    {GPIOB, GPIO_PIN_12},
    {GPIOB, GPIO_PIN_13},
    {GPIOB, GPIO_PIN_14},
    {GPIOB, GPIO_PIN_15},
    {GPIOA, GPIO_PIN_4},
    {GPIOB, GPIO_PIN_6},
    {GPIOB, GPIO_PIN_7}
};

static uint8_t s_key_candidate = 0U;
static volatile uint8_t s_stable_key = 0U;
static uint8_t s_key_stable_ms = 0U;
static uint8_t s_key_lock = 0U;
static uint32_t s_last_can_command_count = 0U;
static uint8_t s_hit_done = 0U;
static uint16_t s_phase_offset = 0U;
static uint32_t s_last_flow_ms = 0U;

static uint8_t EnergySlave_ReadRawKey(void)
{
    for (uint8_t i = 0U; i < 9U; ++i)
    {
        if (HAL_GPIO_ReadPin(kHitButtons[i].port, kHitButtons[i].pin) == GPIO_PIN_RESET)
        {
            return (uint8_t)(i + 1U);
        }
    }
    return 0U;
}

HAL_StatusTypeDef EnergySlave_Init(void)
{
    HAL_StatusTypeDef result;

    LedRing_Init();

    result = CanSlave_Init(&hcan);
    if (result != HAL_OK)
    {
        return result;
    }

    result = HAL_TIM_Base_Start_IT(&htim4);
    if (result != HAL_OK)
    {
        return result;
    }

    s_last_flow_ms = HAL_GetTick();
    return HAL_OK;
}

void EnergySlave_Update(void)
{
    uint32_t now_ms = HAL_GetTick();
    uint32_t can_command_count;
    CanSlaveCommand_e can_command;
    uint8_t can_active;
    uint8_t hit_key;

    can_active = CanSlave_IsActive();
    can_command_count = CanSlave_GetControlCommandCount();

    if (can_command_count != s_last_can_command_count)
    {
        uint8_t was_hit = s_hit_done;

        s_last_can_command_count = can_command_count;
        can_command = CanSlave_GetLastControlCommand();

        if (can_command == CAN_SLAVE_COMMAND_ON)
        {
            EnergyMode_e received_mode = CanSlave_GetMode();
            if (received_mode != g_energy_slave_debug.mode)
            {
                g_energy_slave_debug.mode = received_mode;
                g_energy_slave_debug.mode_change_count++;
            }

            s_hit_done = 0U;
            s_phase_offset = 0U;
            s_last_flow_ms = now_ms;
            LedRing_ShowActive(s_phase_offset);
        }
        else if (can_command == CAN_SLAVE_COMMAND_ALL_OFF)
        {
            s_hit_done = 0U;
            LedRing_ShowIdle();
        }
        else if (can_command == CAN_SLAVE_COMMAND_LOCAL_OFF)
        {
            s_hit_done = 0U;

            /* 大福命中后的关闭指令继续保持六路全黑；超时关闭则恢复待机灯。 */
            if ((g_energy_slave_debug.mode == ENERGY_MODE_BIG) && (was_hit != 0U))
            {
                LedRing_ShowOff();
            }
            else if ((g_energy_slave_debug.mode == ENERGY_MODE_SMALL) && (was_hit != 0U))
            {
                /* 小福命中灯效只允许全局0x2FF清除。 */
                s_hit_done = 1U;
            }
            else
            {
                LedRing_ShowIdle();
            }
        }
    }

    hit_key = s_stable_key;
    if (hit_key == 0U)
    {
        s_key_lock = 0U;
    }

    if (can_active && !s_hit_done && !s_key_lock && (hit_key != 0U))
    {
        s_key_lock = 1U;
        s_hit_done = 1U;
        g_energy_slave_debug.last_hit_key = hit_key;
        g_energy_slave_debug.hit_count++;
        (void)CanSlave_SendHitFeedback();

        if (g_energy_slave_debug.mode == ENERGY_MODE_SMALL)
        {
            LedRing_ShowHit(hit_key);
        }
        else
        {
            LedRing_ShowOff();
        }
    }

    if (can_active && !s_hit_done &&
        ((now_ms - s_last_flow_ms) >= ENERGY_FLOW_PERIOD_MS))
    {
        s_last_flow_ms = now_ms;
        s_phase_offset = (uint16_t)((s_phase_offset + 9U) % 10U);
        LedRing_ShowActive(s_phase_offset);
    }

    g_energy_slave_debug.can_active = can_active;
    g_energy_slave_debug.hit_done = s_hit_done;
}

void EnergySlave_1msTick(void)
{
    uint8_t raw_key = EnergySlave_ReadRawKey();

    if (raw_key != s_key_candidate)
    {
        s_key_candidate = raw_key;
        s_key_stable_ms = 0U;
        return;
    }

    if (s_key_stable_ms < ENERGY_KEY_DEBOUNCE_MS)
    {
        s_key_stable_ms++;
    }

    if ((s_key_stable_ms >= ENERGY_KEY_DEBOUNCE_MS) &&
        (s_stable_key != s_key_candidate))
    {
        s_stable_key = s_key_candidate;
    }
}
