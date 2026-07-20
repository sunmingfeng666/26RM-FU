#include "buzzer.h"

#define BUZZER_HIT_ON_MS  100U
#define BUZZER_HIT_GAP_MS  50U
#define BUZZER_MAX_QUEUE     8U

volatile BuzzerDebug_t g_buzzer_debug = {0};

static TIM_HandleTypeDef *s_timer = NULL;
static uint32_t s_channel = 0U;
static uint32_t s_deadline_ms = 0U;
static uint8_t s_hit_pulse_on = 0U;
static uint8_t s_hit_gap = 0U;

static HAL_StatusTypeDef Buzzer_SetOutput(uint8_t enabled)
{
    HAL_StatusTypeDef result = HAL_OK;

    if (s_timer == NULL)
    {
        return HAL_ERROR;
    }

    if (enabled != 0U)
    {
        __HAL_TIM_SET_COMPARE(s_timer, s_channel, (s_timer->Init.Period + 1U) / 2U);
        if (g_buzzer_debug.output_on == 0U)
        {
            result = HAL_TIM_PWM_Start(s_timer, s_channel);
        }
    }
    else
    {
        if (g_buzzer_debug.output_on != 0U)
        {
            result = HAL_TIM_PWM_Stop(s_timer, s_channel);
        }
        __HAL_TIM_SET_COMPARE(s_timer, s_channel, 0U);
    }

    if (result == HAL_OK)
    {
        g_buzzer_debug.output_on = (enabled != 0U) ? 1U : 0U;
    }
    return result;
}

HAL_StatusTypeDef Buzzer_Init(TIM_HandleTypeDef *htim, uint32_t channel)
{
    if (htim == NULL)
    {
        return HAL_ERROR;
    }

    s_timer = htim;
    s_channel = channel;
    g_buzzer_debug.output_on = 0U;
    g_buzzer_debug.low_voltage_alarm = 0U;
    g_buzzer_debug.queued_hit_beeps = 0U;
    __HAL_TIM_SET_COMPARE(s_timer, s_channel, 0U);
    return HAL_OK;
}

void Buzzer_SetLowVoltageAlarm(uint8_t enabled)
{
    g_buzzer_debug.low_voltage_alarm = (enabled != 0U) ? 1U : 0U;
    g_buzzer_debug.queued_hit_beeps = 0U;
    s_hit_pulse_on = 0U;
    s_hit_gap = 0U;
    (void)Buzzer_SetOutput(enabled);
}

void Buzzer_QueueHitBeep(void)
{
    if (g_buzzer_debug.low_voltage_alarm != 0U)
    {
        return;
    }
    if (g_buzzer_debug.queued_hit_beeps < BUZZER_MAX_QUEUE)
    {
        g_buzzer_debug.queued_hit_beeps++;
    }
}

void Buzzer_CancelHitBeeps(void)
{
    g_buzzer_debug.queued_hit_beeps = 0U;
    s_hit_pulse_on = 0U;
    s_hit_gap = 0U;
    if (g_buzzer_debug.low_voltage_alarm == 0U)
    {
        (void)Buzzer_SetOutput(0U);
    }
}

void Buzzer_Update(uint32_t now_ms)
{
    if (g_buzzer_debug.low_voltage_alarm != 0U)
    {
        if (g_buzzer_debug.output_on == 0U)
        {
            (void)Buzzer_SetOutput(1U);
        }
        return;
    }

    if (s_hit_pulse_on != 0U)
    {
        if ((int32_t)(now_ms - s_deadline_ms) >= 0)
        {
            (void)Buzzer_SetOutput(0U);
            s_hit_pulse_on = 0U;
            s_hit_gap = (g_buzzer_debug.queued_hit_beeps > 0U) ? 1U : 0U;
            s_deadline_ms = now_ms + BUZZER_HIT_GAP_MS;
        }
        return;
    }

    if (s_hit_gap != 0U)
    {
        if ((int32_t)(now_ms - s_deadline_ms) < 0)
        {
            return;
        }
        s_hit_gap = 0U;
    }

    if (g_buzzer_debug.queued_hit_beeps > 0U)
    {
        g_buzzer_debug.queued_hit_beeps--;
        g_buzzer_debug.hit_beep_count++;
        (void)Buzzer_SetOutput(1U);
        s_hit_pulse_on = 1U;
        s_deadline_ms = now_ms + BUZZER_HIT_ON_MS;
    }
}
