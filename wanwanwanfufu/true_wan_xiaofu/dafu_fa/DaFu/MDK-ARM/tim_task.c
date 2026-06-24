#include "tim_task.h"
#include "can_task.h"   

volatile uint8_t armor_active = 0;// 0=未激活, 1=已激活, 2=已命中
volatile uint8_t current_target = 0;//当前命中是1-5哪一个
volatile uint8_t hit_flag = 0;         // 是否命中
volatile uint8_t timeout_flag = 0;     // 是否超时
volatile uint32_t tim2_ms = 0;         // 计时
volatile uint8_t round_state = 0;      // 0=选目标, 1=等待结果
volatile uint8_t pressed[5] = {0,0,0,0,0};
static volatile  uint8_t key_lock = 0; 
static uint32_t rand_seed = 1;
static volatile uint8_t wait_key_release = 0;

//初始化
void TIM_Task_Init(void)
{
    current_target = 0;
    hit_flag = 0;
    timeout_flag = 0;
    tim2_ms = 0;
    round_state = 0;
    key_lock = 0;
		wait_key_release = 0;
		rand_seed = HAL_GetTick() + 123;
	
    for (uint8_t i = 0; i < 5; i++)
    {
        pressed[i] = 0;
    }
}

//随机选择板子
//uint8_t Random_Target_5(void)
//{
//    return (HAL_GetTick() % 5) + 1;   // 返回1~5
//}

//随机选板子
uint32_t Simple_Rand(void)
{
    rand_seed = rand_seed * 1103515245 + 12345;
    return rand_seed;
}

//控制板子亮
void Turn_On_Target(uint8_t target)
{
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

//控制板子灭
void Turn_Off_Target(uint8_t target)
{
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

//随机选一个未按下的目标
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

//随机选一个除当前以外的目标
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

    tim2_ms++;


if (round_state == 0)
{
    current_target = Select_From_Unpressed();

    if (current_target == 0)
    {
        return;
    }

    Turn_On_Target(current_target);

    tim2_ms = 0;
    hit_flag = 0;
    timeout_flag=0;
    round_state = 1;
		return;
		
}

    /* 状态1：等待命中或超时 */
    if (round_state == 1)
    {
        /*if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == GPIO_PIN_RESET||
		HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_RESET||
			HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET ||
        HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_RESET ||
        HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) == GPIO_PIN_RESET)
        {
            if (key_lock == 0)
            {
                key_lock = 1;
                hit_flag = 1;
            }
        }
        else
        {
            key_lock = 0;
        }*/
			

        if (tim2_ms >= 2500)
        {
            timeout_flag = 1;
        }

        if (hit_flag == 1 || timeout_flag == 1)
        {
            Turn_Off_Target(current_target);

            if (hit_flag == 1)
            {
                pressed[current_target - 1] = 1;
            }

            tim2_ms = 0;
						timeout_flag = 0;
						round_state = 0;
        }
    }
}
