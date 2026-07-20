#include "energy_master.h"
#include "battery.h"
#include "buzzer.h"
#include "can_master.h"
#include "dbus.h"
#include "adc.h"
#include "can.h"
#include "tim.h"
#include "usart.h"

#define BIG_FIRST_HIT_TIMEOUT_MS  2500U
#define BIG_SECOND_HIT_TIMEOUT_MS 1000U
#define BIG_ROUNDS_PER_GAME          5U
#define SMALL_HIT_TIMEOUT_MS      2500U
#define SMALL_RESTART_DELAY_MS    2000U
#define ALL_SLAVES_HIT_MASK         0x1FU

volatile EnergyMasterDebug_t g_energy_master_debug = {
    .mode = ENERGY_MODE_STOP,
    .state = ENERGY_MASTER_STOPPED
};

static volatile uint32_t s_uptime_ms = 0U;
static EnergyMode_e s_applied_mode = ENERGY_MODE_STOP;
static uint32_t s_state_start_ms = 0U;
static uint32_t s_rand_seed = 1U;
static uint8_t s_target1 = 0U;
static uint8_t s_target2 = 0U;
static uint8_t s_remaining_target = 0U;
static uint8_t s_big_round = 0U;
static uint8_t s_small_hit_mask = 0U;
static uint8_t s_low_voltage_latched = 0U;

static uint32_t EnergyMaster_Random(void)
{
    s_rand_seed = s_rand_seed * 1103515245U + 12345U;
    return s_rand_seed;
}

static uint8_t EnergyMaster_TargetBit(uint8_t target)
{
    return (uint8_t)(1U << (target - 1U));
}

static void EnergyMaster_UpdateDebug(void)
{
    g_energy_master_debug.uptime_ms = s_uptime_ms;
    g_energy_master_debug.dbus_online = Dbus_IsOnline();
    g_energy_master_debug.dbus_right_switch = Dbus_GetRightSwitch();
    g_energy_master_debug.mode = s_applied_mode;
    g_energy_master_debug.target1 = s_target1;
    g_energy_master_debug.target2 = s_target2;
    g_energy_master_debug.big_round_in_game = s_big_round;
    g_energy_master_debug.small_hit_mask = s_small_hit_mask;
    g_energy_master_debug.battery_raw = Battery_GetRaw();
    g_energy_master_debug.battery_mv = Battery_GetMillivolt();
    g_energy_master_debug.battery_low_count = Battery_GetLowConfirmCount();
    g_energy_master_debug.low_voltage = Battery_IsLowVoltage();
    g_energy_master_debug.can_rx_count = g_can_master_debug.rx_count;
    g_energy_master_debug.can_rx_error_count = g_can_master_debug.rx_error_count;
    g_energy_master_debug.can_tx_count = g_can_master_debug.tx_count;
    g_energy_master_debug.can_tx_error_count = g_can_master_debug.tx_error_count;
}

static void EnergyMaster_ResetRoundState(void)
{
    s_target1 = 0U;
    s_target2 = 0U;
    s_remaining_target = 0U;
    s_big_round = 0U;
    s_small_hit_mask = 0U;
    s_state_start_ms = s_uptime_ms;
    CanMaster_ClearHitMask();
}

static EnergyMode_e EnergyMaster_GetRequestedMode(void)
{
    if (!Dbus_IsOnline())
    {
        return ENERGY_MODE_STOP;
    }

    if (Dbus_GetRightSwitch() == DBUS_SWITCH_UP)
    {
        return ENERGY_MODE_BIG;
    }
    if (Dbus_GetRightSwitch() == DBUS_SWITCH_DOWN)
    {
        return ENERGY_MODE_SMALL;
    }
    return ENERGY_MODE_STOP;
}

static void EnergyMaster_ApplyMode(EnergyMode_e mode)
{
    (void)CanMaster_SendAllOff();
    Buzzer_CancelHitBeeps();
    EnergyMaster_ResetRoundState();
    s_applied_mode = mode;

    if (mode == ENERGY_MODE_BIG)
    {
        g_energy_master_debug.state = ENERGY_MASTER_BIG_PICK_TWO;
    }
    else if (mode == ENERGY_MODE_SMALL)
    {
        g_energy_master_debug.state = ENERGY_MASTER_SMALL_PICK_ONE;
    }
    else
    {
        g_energy_master_debug.state = ENERGY_MASTER_STOPPED;
    }
}

static void EnergyMaster_PickTwoTargets(void)
{
    s_target1 = (uint8_t)((EnergyMaster_Random() % ENERGY_SLAVE_COUNT) + 1U);
    do
    {
        s_target2 = (uint8_t)((EnergyMaster_Random() % ENERGY_SLAVE_COUNT) + 1U);
    } while (s_target2 == s_target1);
}

static uint8_t EnergyMaster_PickSmallTarget(void)
{
    uint8_t candidates[ENERGY_SLAVE_COUNT];
    uint8_t count = 0U;

    for (uint8_t target = 1U; target <= ENERGY_SLAVE_COUNT; ++target)
    {
        if ((s_small_hit_mask & EnergyMaster_TargetBit(target)) == 0U)
        {
            candidates[count++] = target;
        }
    }

    if (count == 0U)
    {
        return 0U;
    }
    return candidates[EnergyMaster_Random() % count];
}

static void EnergyMaster_EndBigRound(void)
{
    s_target1 = 0U;
    s_target2 = 0U;
    s_remaining_target = 0U;
    CanMaster_ClearHitMask();

    if (s_big_round >= BIG_ROUNDS_PER_GAME)
    {
        s_big_round = 0U;
        g_energy_master_debug.big_completed_games++;
    }
    g_energy_master_debug.state = ENERGY_MASTER_BIG_PICK_TWO;
}

static void EnergyMaster_RunBig(void)
{
    uint8_t hit_mask;
    uint8_t target_mask;
    uint8_t valid_hits;

    switch (g_energy_master_debug.state)
    {
        case ENERGY_MASTER_BIG_PICK_TWO:
            EnergyMaster_PickTwoTargets();
            CanMaster_ClearHitMask();
            (void)CanMaster_SendTargetOn(s_target1, ENERGY_MODE_BIG);
            (void)CanMaster_SendTargetOn(s_target2, ENERGY_MODE_BIG);
            s_big_round++;
            s_state_start_ms = s_uptime_ms;
            g_energy_master_debug.state = ENERGY_MASTER_BIG_WAIT_FIRST;
            break;

        case ENERGY_MASTER_BIG_WAIT_FIRST:
            hit_mask = CanMaster_TakeHitMask();
            target_mask = (uint8_t)(EnergyMaster_TargetBit(s_target1) |
                                    EnergyMaster_TargetBit(s_target2));
            valid_hits = (uint8_t)(hit_mask & target_mask);

            if (valid_hits != 0U)
            {
                if ((valid_hits & EnergyMaster_TargetBit(s_target1)) != 0U)
                {
                    (void)CanMaster_SendTargetOff(s_target1);
                    Buzzer_QueueHitBeep();
                }
                if ((valid_hits & EnergyMaster_TargetBit(s_target2)) != 0U)
                {
                    (void)CanMaster_SendTargetOff(s_target2);
                    Buzzer_QueueHitBeep();
                }

                if (valid_hits == target_mask)
                {
                    EnergyMaster_EndBigRound();
                }
                else
                {
                    s_remaining_target =
                        ((valid_hits & EnergyMaster_TargetBit(s_target1)) != 0U) ?
                        s_target2 : s_target1;
                    s_state_start_ms = s_uptime_ms;
                    g_energy_master_debug.state = ENERGY_MASTER_BIG_WAIT_SECOND;
                }
            }
            else if ((s_uptime_ms - s_state_start_ms) >= BIG_FIRST_HIT_TIMEOUT_MS)
            {
                (void)CanMaster_SendTargetOff(s_target1);
                (void)CanMaster_SendTargetOff(s_target2);
                EnergyMaster_EndBigRound();
            }
            break;

        case ENERGY_MASTER_BIG_WAIT_SECOND:
            hit_mask = CanMaster_TakeHitMask();
            if ((hit_mask & EnergyMaster_TargetBit(s_remaining_target)) != 0U)
            {
                (void)CanMaster_SendTargetOff(s_remaining_target);
                Buzzer_QueueHitBeep();
                EnergyMaster_EndBigRound();
            }
            else if ((s_uptime_ms - s_state_start_ms) >= BIG_SECOND_HIT_TIMEOUT_MS)
            {
                (void)CanMaster_SendTargetOff(s_remaining_target);
                EnergyMaster_EndBigRound();
            }
            break;

        default:
            g_energy_master_debug.state = ENERGY_MASTER_BIG_PICK_TWO;
            break;
    }
}

static void EnergyMaster_RunSmall(void)
{
    uint8_t hit_mask;

    switch (g_energy_master_debug.state)
    {
        case ENERGY_MASTER_SMALL_PICK_ONE:
            if (s_small_hit_mask == ALL_SLAVES_HIT_MASK)
            {
                (void)CanMaster_SendAllOff();
                s_small_hit_mask = 0U;
                s_target1 = 0U;
                s_state_start_ms = s_uptime_ms;
                g_energy_master_debug.state = ENERGY_MASTER_SMALL_ROUND_DELAY;
                break;
            }

            s_target1 = EnergyMaster_PickSmallTarget();
            s_target2 = 0U;
            if (s_target1 != 0U)
            {
                CanMaster_ClearHitMask();
                (void)CanMaster_SendTargetOn(s_target1, ENERGY_MODE_SMALL);
                s_state_start_ms = s_uptime_ms;
                g_energy_master_debug.state = ENERGY_MASTER_SMALL_WAIT_HIT;
            }
            break;

        case ENERGY_MASTER_SMALL_WAIT_HIT:
            hit_mask = CanMaster_TakeHitMask();
            if ((hit_mask & EnergyMaster_TargetBit(s_target1)) != 0U)
            {
                s_small_hit_mask |= EnergyMaster_TargetBit(s_target1);
                s_target1 = 0U;
                g_energy_master_debug.state = ENERGY_MASTER_SMALL_PICK_ONE;
            }
            else if ((s_uptime_ms - s_state_start_ms) >= SMALL_HIT_TIMEOUT_MS)
            {
                (void)CanMaster_SendTargetOff(s_target1);
                s_target1 = 0U;
                g_energy_master_debug.state = ENERGY_MASTER_SMALL_PICK_ONE;
            }
            break;

        case ENERGY_MASTER_SMALL_ROUND_DELAY:
            if ((s_uptime_ms - s_state_start_ms) >= SMALL_RESTART_DELAY_MS)
            {
                g_energy_master_debug.state = ENERGY_MASTER_SMALL_PICK_ONE;
            }
            break;

        default:
            g_energy_master_debug.state = ENERGY_MASTER_SMALL_PICK_ONE;
            break;
    }
}

HAL_StatusTypeDef EnergyMaster_Init(void)
{
    HAL_StatusTypeDef result;

    result = Battery_Init(&hadc3);
    if (result != HAL_OK)
    {
        return result;
    }

    result = Buzzer_Init(&htim4, TIM_CHANNEL_3);
    if (result != HAL_OK)
    {
        return result;
    }

    result = CanMaster_Init(&hcan2);
    if (result != HAL_OK)
    {
        return result;
    }

    result = Dbus_Init(&huart3);
    if (result != HAL_OK)
    {
        return result;
    }

    result = HAL_TIM_Base_Start_IT(&htim2);
    if (result != HAL_OK)
    {
        return result;
    }

    s_rand_seed = ((uint32_t)Battery_GetRaw() << 16U) ^ HAL_GetTick() ^ 0x6D2B79F5U;
    s_low_voltage_latched = Battery_IsLowVoltage();
    (void)CanMaster_SendAllOff();
    EnergyMaster_UpdateDebug();
    return HAL_OK;
}

void EnergyMaster_Update(void)
{
    EnergyMode_e requested_mode;
    uint32_t hal_now_ms = HAL_GetTick();

    Dbus_Poll(hal_now_ms);
    Battery_Update(hal_now_ms);

    if (Battery_IsLowVoltage() != s_low_voltage_latched)
    {
        s_low_voltage_latched = Battery_IsLowVoltage();
        if (s_low_voltage_latched != 0U)
        {
            (void)CanMaster_SendAllOff();
            EnergyMaster_ResetRoundState();
            Buzzer_SetLowVoltageAlarm(1U);
            g_energy_master_debug.state = ENERGY_MASTER_LOW_VOLTAGE;
        }
        else
        {
            Buzzer_SetLowVoltageAlarm(0U);
            (void)CanMaster_SendAllOff();
            EnergyMaster_ResetRoundState();
            s_applied_mode = ENERGY_MODE_STOP;
            g_energy_master_debug.state = ENERGY_MASTER_STOPPED;
        }
    }

    if (s_low_voltage_latched != 0U)
    {
        Buzzer_Update(hal_now_ms);
        EnergyMaster_UpdateDebug();
        return;
    }

    requested_mode = EnergyMaster_GetRequestedMode();
    if (requested_mode != s_applied_mode)
    {
        EnergyMaster_ApplyMode(requested_mode);
    }

    if (s_applied_mode == ENERGY_MODE_BIG)
    {
        EnergyMaster_RunBig();
    }
    else if (s_applied_mode == ENERGY_MODE_SMALL)
    {
        EnergyMaster_RunSmall();
    }

    Buzzer_Update(hal_now_ms);
    EnergyMaster_UpdateDebug();
}

void EnergyMaster_1msTick(void)
{
    s_uptime_ms++;
}
