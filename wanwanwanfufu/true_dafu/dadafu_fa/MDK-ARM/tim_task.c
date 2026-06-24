#include "tim_task.h"
#include "can_task.h"

#define BATTERY_ADC_CHANNEL           8U
#define BATTERY_ADC_SAMPLE_PERIOD_MS 50U
#define BATTERY_ADC_AVG_SAMPLES      8U
#define BATTERY_STOP_MV              22000U
#define BATTERY_RESUME_MV            23000U

volatile uint8_t dbg_state = 0;
volatile uint8_t dbg_target1 = 0;
volatile uint8_t dbg_target2 = 0;
volatile uint8_t dbg_first_hit_target = 0;
volatile uint8_t dbg_remain_target = 0;
volatile uint16_t dbg_bat_raw = 0;
volatile uint32_t dbg_bat_mv = 0;
volatile uint8_t dbg_low_voltage = 0;

volatile uint32_t tim2_ms = 0;

//命中板号
volatile uint8_t hit_board_id = 0;//0 表示当前没有新的命中事

//组数计数：
volatile uint32_t group_count = 0;
static uint8_t pressed[5] = {0, 0, 0, 0, 0};

/* 本轮随机选中的两个目标板号 */
static uint8_t target1 = 0;
static uint8_t target2 = 0;

/* 第一次被命中的目标板号 */
static uint8_t first_hit_target = 0;

/* 第一次命中后剩余继续点亮的目标板号 */
static uint8_t remain_target = 0;

//随机数
static uint32_t rand_seed = 1;
static uint8_t battery_sample_tick = 0;
static uint8_t battery_low_voltage = 0;
static uint32_t Simple_Rand(void);

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

    return (uint16_t)(sum / BATTERY_ADC_AVG_SAMPLES);
}

static uint32_t Battery_Adc_RawToBatteryMv(uint16_t raw)
{
    /* 手册给出的分压关系: Vbat -> 200k -> ADC_BAT -> 22k -> GND */
    return (((uint32_t)raw * 33300U) + 2047U) / 4095U;
}

static void Reset_Pressed_State(void)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        pressed[i] = 0;
    }
}

static void Mark_Target_Pressed(uint8_t target)
{
    if ((target >= 1) && (target <= 5))
    {
        pressed[target - 1] = 1;
    }
}

static uint8_t Are_All_Targets_Pressed(void)
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

static uint8_t Count_Unpressed_Targets(void)
{
    uint8_t count = 0;

    for (uint8_t i = 0; i < 5; i++)
    {
        if (pressed[i] == 0)
        {
            count++;
        }
    }

    return count;
}

static uint8_t Pick_Target_From_Unpressed(void)
{
    uint8_t candidates[5];
    uint8_t count = 0;

    for (uint8_t i = 0; i < 5; i++)
    {
        if (pressed[i] == 0)
        {
            candidates[count++] = (uint8_t)(i + 1);
        }
    }

    if (count == 0)
    {
        return 0;
    }

    return candidates[Simple_Rand() % count];
}

static uint8_t Pick_Target_From_Others(uint8_t exclude)
{
    uint8_t candidates[5];
    uint8_t count = 0;

    for (uint8_t i = 1; i <= 5; i++)
    {
        if (i != exclude)
        {
            candidates[count++] = i;
        }
    }

    if (count == 0)
    {
        return 0;
    }

    return candidates[Simple_Rand() % count];
}


typedef enum
{
    GAME_PICK_TWO = 0,
    GAME_WAIT_FIRST_HIT,
    GAME_WAIT_SECOND_HIT
} GameState_t;

/* 当前状态机状态 */
static GameState_t game_state = GAME_PICK_TWO;


//函数初始化
void TIM_Task_Init(void)
{
    //清零计时与命中信息 
    tim2_ms      = 0;
    hit_board_id = 0;
    group_count  = 0;

    //清零本轮目标信息 
    target1          = 0;
    target2          = 0;
    first_hit_target = 0;
    remain_target    = 0;

    //状态机回到初始状态
    game_state = GAME_PICK_TWO;
    Reset_Pressed_State();

  //随机选择
    rand_seed = HAL_GetTick() + 123;

    Battery_Adc_Init();
    battery_sample_tick = 0;
    dbg_bat_raw = Battery_Adc_ReadAverageRaw();
    dbg_bat_mv = Battery_Adc_RawToBatteryMv(dbg_bat_raw);
    battery_low_voltage = (dbg_bat_mv < BATTERY_STOP_MV) ? 1U : 0U;
    dbg_low_voltage = battery_low_voltage;
}


//随机选择板子
static uint32_t Simple_Rand(void)
{
    rand_seed = rand_seed * 1103515245 + 12345;
    return rand_seed;
}



//从 1~5 号目标中随机选出两个不同的板号
//a - 返回第一个目标编号
//b - 返回第二个目标编号
static void Pick_Two_Random_Targets(uint8_t *a, uint8_t *b)
{
    uint8_t unpressed_count = Count_Unpressed_Targets();

    if (unpressed_count == 0)
    {
        Reset_Pressed_State();
        unpressed_count = Count_Unpressed_Targets();
    }

    *a = Pick_Target_From_Unpressed();

    if (unpressed_count >= 2)
    {
        do
        {
            *b = Pick_Target_From_Unpressed();
        } while ((*b == 0) || (*b == *a));
    }
    else
    {
        *b = Pick_Target_From_Others(*a);
    }
}

//发送ID给从机
void Turn_Off_Target(uint8_t target)
{
    if (battery_low_voltage != 0U)
    {
        return;
    }

    memset(sdata.Data, 0, 8);

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

//整个阶段没有打到，关闭选择的两个灯
static void Turn_Off_Two_Targets(uint8_t a, uint8_t b)
{
    Turn_Off_Target(a);

    if (b != a)
    {
        Turn_Off_Target(b);
    }
}


//点亮指定编号板子
void Turn_On_Target(uint8_t target)
{
    if (battery_low_voltage != 0U)
    {
        return;
    }

    memset(sdata.Data, 0, 8);

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

static void Update_Battery_Protection(void)
{
    if (++battery_sample_tick < BATTERY_ADC_SAMPLE_PERIOD_MS)
    {
        return;
    }

    battery_sample_tick = 0;
    dbg_bat_raw = Battery_Adc_ReadAverageRaw();
    dbg_bat_mv = Battery_Adc_RawToBatteryMv(dbg_bat_raw);

    if ((battery_low_voltage == 0U) && (dbg_bat_mv < BATTERY_STOP_MV))
    {
        battery_low_voltage = 1U;
        dbg_low_voltage = 1U;
    }
    else if ((battery_low_voltage != 0U) && (dbg_bat_mv > BATTERY_RESUME_MV))
    {
        battery_low_voltage = 0U;
        dbg_low_voltage = 0U;
    }
}

void TIM_Task_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

    if (htim->Instance != TIM2)
    {
        return;
    }


    Update_Battery_Protection();

    if (battery_low_voltage != 0U)
    {
        dbg_state = (uint8_t)game_state;
        dbg_target1 = target1;
        dbg_target2 = target2;
        dbg_first_hit_target = first_hit_target;
        dbg_remain_target = remain_target;
        return;
    }

    tim2_ms++;
		
		dbg_state = (uint8_t)game_state;
		dbg_target1 = target1;
		dbg_target2 = target2;
		dbg_first_hit_target = first_hit_target;
		dbg_remain_target = remain_target;
		
    switch (game_state)
    {

			//随机选两块并点亮，开启新的一组
        case GAME_PICK_TWO:
        {
            if (Are_All_Targets_Pressed())
            {
                Reset_Pressed_State();
            }

            Pick_Two_Random_Targets(&target1, &target2);

            Turn_On_Target(target1);
            Turn_On_Target(target2);

            group_count++;

            //清楚残留数据
            hit_board_id     = 0;
            first_hit_target = 0;
            remain_target    = 0;

           //命中后，等待
            tim2_ms = 0;
            game_state = GAME_WAIT_FIRST_HIT;
        }
        break;


				//等待第一次命中
        case GAME_WAIT_FIRST_HIT:
        {
            
            if ((hit_board_id == target1) || (hit_board_id == target2))
            {
                //记录第一次命中的目标 
                first_hit_target = hit_board_id;
                Mark_Target_Pressed(first_hit_target);

                // 计算剩余还亮着的目标 
                remain_target = (hit_board_id == target1) ? target2 : target1;

                if (Are_All_Targets_Pressed())
                {
                    Turn_Off_Two_Targets(target1, target2);

                    hit_board_id = 0;
                    first_hit_target = 0;
                    remain_target = 0;
                    tim2_ms = 0;
                    game_state = GAME_PICK_TWO;
                    break;
                }

                //命中后立刻关闭被击中的板子
                Turn_Off_Target(first_hit_target);

                
                hit_board_id = 0;
                tim2_ms = 0;
                game_state = GAME_WAIT_SECOND_HIT;
            }
            else if (tim2_ms >= 2500)
            {
               //都没打到熄灭
                Turn_Off_Two_Targets(target1, target2);

                hit_board_id = 0;
                tim2_ms = 0;
                game_state = GAME_PICK_TWO;
            }
        }
        break;


				//等待第二次命中
        case GAME_WAIT_SECOND_HIT:
        {
            if (hit_board_id == remain_target)
            {
                /* 第二次命中成功，关闭剩余目标并结束本组 */
                Mark_Target_Pressed(remain_target);
                Turn_Off_Target(remain_target);

                hit_board_id = 0;
                first_hit_target = 0;
                remain_target = 0;
                tim2_ms = 0;
                game_state = GAME_PICK_TWO;
            }
            else if (tim2_ms >= 1000)
            {
                /* 1 秒内未击中剩余目标，则强制关闭并结束本组 */
                Turn_Off_Target(remain_target);

                hit_board_id = 0;
                first_hit_target = 0;
                remain_target = 0;
                tim2_ms = 0;
                game_state = GAME_PICK_TWO;
            }
        }
        break;

				//异常保护
        default:
        {
            game_state = GAME_PICK_TWO;
            hit_board_id = 0;
            tim2_ms = 0;
        }
        break;
    }
}
