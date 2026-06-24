#include "tim_task.h"
#include "can_task.h"   

#define BATTERY_ADC_CHANNEL           8U
#define BATTERY_ADC_SAMPLE_PERIOD_MS 50U
#define BATTERY_ADC_AVG_SAMPLES      8U
#define BATTERY_STOP_MV              22200U
#define BATTERY_LOW_CONFIRM_COUNT    200U

volatile uint8_t armor_active = 0;// 0=???, 1=???, 2=???
volatile uint8_t current_target = 0;//?????1-5???
volatile uint8_t hit_flag = 0;         // ????
volatile uint8_t timeout_flag = 0;     // ????
volatile uint32_t tim2_ms = 0;         // ??
volatile uint8_t round_state = 0;      // 0=???, 1=????
volatile uint8_t pressed[5] = {0,0,0,0,0};
volatile uint16_t dbg_bat_raw = 0;
volatile uint32_t dbg_bat_mv = 0;
volatile uint8_t dbg_low_voltage = 0;
volatile uint16_t dbg_low_voltage_count = 0;
static volatile  uint8_t key_lock = 0; 
static uint32_t rand_seed = 1;
static volatile uint8_t wait_key_release = 0;
static volatile uint8_t restart_round_pending = 0;
static uint8_t battery_sample_tick = 0;
static uint8_t battery_low_voltage = 0;
static uint16_t battery_low_voltage_count = 0;

#define ROUND_RESTART_DELAY_MS 2000u

static void Force_Turn_Off_All_Targets(void)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        sdata.Data[i] = 0U;
    }

    RUI_F_CAN_SEDN_UNION(&hcan2, 0x2FF, sdata.Data);
}

static void Battery_Adc_Init(void)
{
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_ADC3_CLK_ENABLE();

    GPIOF->MODER |= (3U << (10U * 2U));
    GPIOF->PUPDR &= ~(3U << (10U * 2U));

    ADC->CCR &= ~ADC_CCR_ADCPRE;
    ADC->CCR |= ADC_CCR_ADCPRE_0;

    ADC3->CR1 = 0U;
    ADC3->CR2 = 0U;
    ADC3->SMPR1 = 0U;
    ADC3->SMPR2 = 0U;
    ADC3->SQR1 = 0U;
    ADC3->SQR2 = 0U;
    ADC3->SQR3 = BATTERY_ADC_CHANNEL;
    ADC3->SMPR2 |= (7U << (BATTERY_ADC_CHANNEL * 3U));
    ADC3->CR2 |= ADC_CR2_ADON;
}

static uint16_t Battery_Adc_ReadRaw(void)
{
    uint32_t timeout = 100000U;

    ADC3->SR = 0U;
    ADC3->CR2 |= ADC_CR2_SWSTART;

    while ((ADC3->SR & ADC_SR_EOC) == 0U)
    {
        if (timeout-- == 0U)
        {
            return 0U;
        }
    }

    return (uint16_t)ADC3->DR;
}

static uint16_t Battery_Adc_ReadAverageRaw(void)
{
    uint32_t sum = 0U;

    for (uint8_t i = 0; i < BATTERY_ADC_AVG_SAMPLES; i++)
    {
        sum += Battery_Adc_ReadRaw();
    }

    return ((uint16_t)(sum / BATTERY_ADC_AVG_SAMPLES));
}

static uint32_t Battery_Adc_RawToBatteryMv(uint16_t raw)
{
    return (((((uint32_t)raw * 33300U) + 2047U) / 4095U)+800);
}

static void Update_Battery_Protection(void)
{
    if (++battery_sample_tick < BATTERY_ADC_SAMPLE_PERIOD_MS)
    {
        return;
    }

    battery_sample_tick = 0;
    dbg_bat_raw = Battery_Adc_ReadAverageRaw();
    dbg_bat_mv = Battery_Adc_RawToBatteryMv(dbg_bat_raw);
    
    if (battery_low_voltage != 0U)
    {
        if (dbg_bat_mv > BATTERY_STOP_MV)
        {
            battery_low_voltage = 0U;
            battery_low_voltage_count = 0U;
        }
    }
    else
    {
        if (dbg_bat_mv < BATTERY_STOP_MV)
        {
            if (battery_low_voltage_count < BATTERY_LOW_CONFIRM_COUNT)
            {
                battery_low_voltage_count++;
            }

            if (battery_low_voltage_count >= BATTERY_LOW_CONFIRM_COUNT)
            {
                Force_Turn_Off_All_Targets();
                battery_low_voltage = 1U;
            }
        }
        else
        {
            battery_low_voltage_count = 0U;
        }
    }
    
    dbg_low_voltage = battery_low_voltage;
    dbg_low_voltage_count = battery_low_voltage_count;
}

static void Reset_Pressed_State(void)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        pressed[i] = 0;
    }
}

static uint8_t Are_All_Slaves_Activated(void)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        if (pressed[i] == 0)
        {
            return 0;
        }
    }

    return 1;
}

//???
void TIM_Task_Init(void)
{
    current_target = 0;
    hit_flag = 0;
    timeout_flag = 0;
    tim2_ms = 0;
    round_state = 0;
    key_lock = 0;
		wait_key_release = 0;
		restart_round_pending = 0;
		rand_seed = HAL_GetTick() + 123;
    battery_sample_tick = 0;
    battery_low_voltage_count = 0U;
	
    Battery_Adc_Init();
    dbg_bat_raw = Battery_Adc_ReadAverageRaw();
    dbg_bat_mv = Battery_Adc_RawToBatteryMv(dbg_bat_raw);
    battery_low_voltage = 0U;
    dbg_low_voltage = battery_low_voltage;
    dbg_low_voltage_count = battery_low_voltage_count;

    Reset_Pressed_State();
}

//??????
//uint8_t Random_Target_5(void)
//{
//    return (HAL_GetTick() % 5) + 1;   // ??1~5
//}

//?????
uint32_t Simple_Rand(void)
{
    rand_seed = rand_seed * 1103515245 + 12345;
    return rand_seed;
}

//?????
void Turn_On_Target(uint8_t target)
{
    if (battery_low_voltage != 0U)
    {
        return;
    }

    switch (target)
    {
        case 1:
            RUI_F_CAN_SEDN_UNION(&hcan2, 0x201, sdata.Data);
            break;
        case 2:
            RUI_F_CAN_SEDN_UNION(&hcan2, 0x211, sdata.Data);
            break;
        case 3:
            RUI_F_CAN_SEDN_UNION(&hcan2, 0x221, sdata.Data);
            break;
        case 4:
            RUI_F_CAN_SEDN_UNION(&hcan2, 0x231, sdata.Data);
            break;
        case 5:
            RUI_F_CAN_SEDN_UNION(&hcan2, 0x241, sdata.Data);
            break;
        default:
            break;
    }
}

//?????
void Turn_Off_Target(uint8_t target)
{
    if (battery_low_voltage != 0U)
    {
        return;
    }

    switch (target)
    {
        case 1:
            RUI_F_CAN_SEDN_UNION(&hcan2, 0x202, sdata.Data);
            break;
        case 2:
            RUI_F_CAN_SEDN_UNION(&hcan2, 0x212, sdata.Data);
            break;
        case 3:
            RUI_F_CAN_SEDN_UNION(&hcan2, 0x222, sdata.Data);
            break;
        case 4:
            RUI_F_CAN_SEDN_UNION(&hcan2, 0x232, sdata.Data);
            break;
        case 5:
            RUI_F_CAN_SEDN_UNION(&hcan2, 0x242, sdata.Data);
            break;
        default:
            break;
    }
}

void Turn_Off_All_Targets(void)
{
    if (battery_low_voltage != 0U)
    {
        return;
    }

    RUI_F_CAN_SEDN_UNION(&hcan2, 0x2FF, sdata.Data);
}

//???????????
uint8_t Select_From_Unpressed(void)
{
    uint8_t candidates[5];
    uint8_t count = 0;

    for (uint8_t i = 0; i < 5; i++)
    {
        if (pressed[i] == 0)
        {
            candidates[count++] = i + 1;
        }
    }

    if (count == 0) return 0;

    
		return candidates[Simple_Rand() % count];
}

//?????????????
uint8_t Select_From_Others(uint8_t current)
{
    uint8_t candidates[5];
    uint8_t count = 0;

    for (uint8_t i = 1; i <= 5; i++)
    {
        if (i != current)
        {
            candidates[count++] = i;
        }
    }

    if (count == 0) return 0;

    return candidates[HAL_GetTick() % count];
}

void TIM_Task_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2) return;

    Update_Battery_Protection();

    if (battery_low_voltage != 0U)
    {
        return;
    }

    tim2_ms++;


if (round_state == 0)
{
    if (restart_round_pending == 1)
    {
        if (tim2_ms < ROUND_RESTART_DELAY_MS)
        {
            return;
        }

        restart_round_pending = 0;
        tim2_ms = 0;
    }

    if (Are_All_Slaves_Activated())
    {
        Turn_Off_All_Targets();
        Reset_Pressed_State();
        restart_round_pending = 1;
        tim2_ms = 0;
        return;
    }

    current_target = Select_From_Unpressed();

    if (current_target == 0)
    {
        return;
    }

    Turn_On_Target(current_target);

    tim2_ms = 0;
    hit_flag = 0;
    timeout_flag = 0;
    key_lock = 1;              // ???
    wait_key_release = 1;      // ????
    round_state = 1;

    return;
		
}

    /* ??1:??????? */
    if (round_state == 1)
    {
        

        if (tim2_ms >= 2500)
        {
            timeout_flag = 1;
        }

        if (hit_flag == 1)
        {
            pressed[current_target - 1] = 1;
            current_target = 0;
            tim2_ms = 0;
            hit_flag = 0;
            timeout_flag = 0;
            round_state = 0;
        }
        else if (timeout_flag == 1)
        {
            Turn_Off_Target(current_target);
            current_target = 0;
            tim2_ms = 0;
            timeout_flag = 0;
            round_state = 0;
        }
    }
}
