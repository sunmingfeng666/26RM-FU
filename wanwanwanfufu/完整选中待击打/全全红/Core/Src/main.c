/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "dma.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "WS2812.h"
#include "led_rings.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/**
 * @brief GPIO
 */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint8_t key_lock = 0;
static volatile uint8_t pending_hit_key = 0;
volatile uint8_t g_debug_current_hit_key = 0;
volatile uint8_t g_debug_last_hit_key = 0;
volatile uint8_t g_debug_current_hit_port = 0;
volatile uint16_t g_debug_current_hit_pin = 0;
volatile uint8_t g_debug_last_hit_port = 0;
volatile uint16_t g_debug_last_hit_pin = 0;
volatile uint32_t g_debug_hit_event_count = 0;

#define FIRST_THREE_LED_COUNT (STRIP1_LED_COUNT + STRIP2_LED_COUNT + STRIP3_LED_COUNT)
#define DIN3_START_INDEX      FIRST_THREE_LED_COUNT
#define DIN3_END_INDEX        (DIN3_START_INDEX + STRIP4_LED_COUNT - 1u)
#define DIN4_START_INDEX      (DIN3_END_INDEX + 1u)
#define DIN4_END_INDEX        (DIN4_START_INDEX + STRIP5_LED_COUNT - 1u)
#define DIN5_START_INDEX      (DIN4_END_INDEX + 1u)
#define DIN5_END_INDEX        (DIN5_START_INDEX + STRIP6_LED_COUNT - 1u)
#define FLOW_DELAY_MS         150u
#define BLINK_DELAY_MS        250u
#define BOARD_LED_COUNT       70u
#define BOARD_COUNT           4u
#define ARROW_GROUP_COUNT     7u
#define ARROW_GROUP_WIDTH     3u
#define ARROW_PHASE_COUNT     10u
#define HIT_KEY_PB10          1u
#define HIT_KEY_PB11          2u
#define HIT_KEY_PB12          3u
#define HIT_KEY_PB13          4u
#define HIT_KEY_PB14          5u
#define HIT_KEY_PA3           6u
#define HIT_KEY_PA4           7u
#define HIT_KEY_PB6           8u
#define HIT_KEY_PB7           9u
#define HIT_KEY_PB15          10u

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
  uint8_t key_id;
} ButtonInput_t;

static const ButtonInput_t kHitButtons[] = {
  {GPIOB, GPIO_PIN_10, HIT_KEY_PB10},
  {GPIOB, GPIO_PIN_11, HIT_KEY_PB11},
  {GPIOB, GPIO_PIN_12, HIT_KEY_PB12},
  {GPIOB, GPIO_PIN_13, HIT_KEY_PB13},
  {GPIOB, GPIO_PIN_14, HIT_KEY_PB14},
  {GPIOA, GPIO_PIN_3, HIT_KEY_PA3},
  {GPIOA, GPIO_PIN_4, HIT_KEY_PA4},
  {GPIOB, GPIO_PIN_6, HIT_KEY_PB6},
  {GPIOB, GPIO_PIN_7, HIT_KEY_PB7},
  {GPIOB, GPIO_PIN_15, HIT_KEY_PB15},
};

static const uint16_t kSelectedPatternLeds[] = {
  0, 1, 2,
  10, 11, 12, 13, 14,
  22, 23, 24, 25, 26,
  34, 35, 36,
  37, 38,
  46, 47, 48,
  48, 49, 50, 51, 52,
  53, 54, 55, 56, 57,
  58, 59, 60, 61, 62,
  63, 64, 65, 66, 67,
  68, 69, 70, 71, 72,
  73, 74, 75, 76, 77,
  78, 79, 80, 81, 82,
  83, 84, 85, 86, 87,
  88, 89,
  90, 91,
  101, 102,
  112, 113,
  123, 124,
  134, 135,
  144, 145,
  154, 155,
  164, 165,
  174, 175, 176, 177, 178,
  179, 180, 181, 182, 183,
  184, 185, 186, 187, 188,
  189, 190, 191, 192, 193,
  194, 195, 196, 197, 198,
  199, 200, 201, 202, 203,
  204, 205, 206, 207, 208,
  209, 210,
  215, 220, 225,
  230, 235, 240, 245,
  262, 263, 264, 265, 266,
  267, 268, 269, 270
};

static const uint16_t kArrowGroupZoneStarts[ARROW_GROUP_COUNT] = {
  0u, 10u, 20u, 30u, 40u, 50u, 60u
};

static const uint16_t kArrowGroupLocalStarts[ARROW_GROUP_COUNT] = {
  3u, 2u, 1u, 0u, 1u, 2u, 3u
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static uint8_t GetDebugPortCode(GPIO_TypeDef *port);
static void UpdateCurrentHitDebugState(const ButtonInput_t *button);
static void RecordHitDebugEvent(const ButtonInput_t *button);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void SetRangeColor(uint16_t start_index, uint16_t end_index,
                          uint8_t r, uint8_t g, uint8_t b)
{
  if (start_index >= TOTAL_LED_COUNT)
  {
    return;
  }

  if (end_index >= TOTAL_LED_COUNT)
  {
    end_index = TOTAL_LED_COUNT - 1;
  }

  for (uint16_t i = start_index; i <= end_index; ++i)
  {
    WS2812_SetPixel(i, r, g, b);
  }
}

static void DrawSelectedPattern(void)
{
  for (uint16_t i = 0;
       i < (sizeof(kSelectedPatternLeds) / sizeof(kSelectedPatternLeds[0]));
       ++i)
  {
    WS2812_SetPixel(kSelectedPatternLeds[i], 255, 0, 0);
  }
}

static void DrawPendingPattern(void)
{
  SetRangeColor(250u, 261u, 255, 0, 0);
  SetRangeColor(STRIP1_LED_COUNT, FIRST_THREE_LED_COUNT - 1u, 255, 0, 0);
}

static void DrawFlowOnDin3(uint16_t phase_offset)
{
  for (uint16_t board_index = 0; board_index < BOARD_COUNT; ++board_index)
  {
    uint16_t board_start = (uint16_t)(board_index * BOARD_LED_COUNT);

    for (uint16_t group_index = 0; group_index < ARROW_GROUP_COUNT; ++group_index)
    {
      uint16_t zone_start =
        (uint16_t)(board_start + kArrowGroupZoneStarts[group_index]);

      for (uint16_t pixel_offset = 0; pixel_offset < ARROW_GROUP_WIDTH; ++pixel_offset)
      {
        uint16_t local_index =
          (uint16_t)((kArrowGroupLocalStarts[group_index] + phase_offset + pixel_offset) %
                     ARROW_PHASE_COUNT);
        uint16_t local_pixel = (uint16_t)(zone_start + local_index);

        if (local_pixel < STRIP4_LED_COUNT)
        {
          WS2812_SetPixel((uint16_t)(DIN3_START_INDEX + local_pixel), 255, 0, 0);
        }
      }
    }
  }
}

static void KeepDin5AlwaysOn(void)
{
  SetRangeColor(DIN5_START_INDEX, DIN5_END_INDEX, 255, 0, 0);
}

static void RenderPowerOnFrame(uint16_t phase_offset)
{
  WS2812_SetAll(0, 0, 0);
  DrawSelectedPattern();
  DrawFlowOnDin3(phase_offset);
  KeepDin5AlwaysOn();
  WS2812_SendAll();
}

static void RenderHitFrame(uint8_t din4_on)
{
  WS2812_SetAll(0, 0, 0);
  DrawPendingPattern();

  if (din4_on)
  {
    SetRangeColor(DIN3_START_INDEX, DIN3_END_INDEX, 255, 0, 0);
  }

  SetRangeColor(DIN4_START_INDEX, DIN4_END_INDEX, 255, 0, 0);
  KeepDin5AlwaysOn();

  WS2812_SendAll();
}

static void RenderFlowHitFrame(uint16_t din0_start, uint16_t din0_end,
                               uint16_t phase_offset)
{
  (void)phase_offset;
  WS2812_SetAll(0, 0, 0);
  SetRangeColor(din0_start, din0_end, 255, 0, 0);
  SetRangeColor(STRIP1_LED_COUNT, FIRST_THREE_LED_COUNT - 1u, 255, 0, 0);
  SetRangeColor(DIN3_START_INDEX, DIN3_END_INDEX, 255, 0, 0);
  SetRangeColor(DIN4_START_INDEX, DIN4_END_INDEX, 255, 0, 0);
  KeepDin5AlwaysOn();
  WS2812_SendAll();
}

static uint8_t ReadPressedHitButtonRaw(void)
{
  for (uint8_t i = 0; i < (sizeof(kHitButtons) / sizeof(kHitButtons[0])); ++i)
  {
    if (HAL_GPIO_ReadPin(kHitButtons[i].port, kHitButtons[i].pin) == GPIO_PIN_RESET)
    {
      if (g_debug_current_hit_key != kHitButtons[i].key_id)
      {
        RecordHitDebugEvent(&kHitButtons[i]);
      }

      UpdateCurrentHitDebugState(&kHitButtons[i]);
      return kHitButtons[i].key_id;
    }
  }

  UpdateCurrentHitDebugState(NULL);
  return 0;
}

static uint8_t GetPressedHitButton(void)
{
  uint8_t pressed_key = pending_hit_key;

  if (pressed_key != 0u)
  {
    pending_hit_key = 0;
  }

  return pressed_key;
}

void KeyScan_TimerTick(void)
{
  if (pending_hit_key == 0u)
  {
    pending_hit_key = ReadPressedHitButtonRaw();
  }
}

static void PlayHitEffect(void)
{
  RenderHitFrame(1);
}

static uint8_t GetDebugPortCode(GPIO_TypeDef *port)
{
  if (port == GPIOA)
  {
    return 1u;
  }

  if (port == GPIOB)
  {
    return 2u;
  }

  return 0u;
}

static void UpdateCurrentHitDebugState(const ButtonInput_t *button)
{
  if (button == NULL)
  {
    g_debug_current_hit_key = 0u;
    g_debug_current_hit_port = 0u;
    g_debug_current_hit_pin = 0u;
    return;
  }

  g_debug_current_hit_key = button->key_id;
  g_debug_current_hit_port = GetDebugPortCode(button->port);
  g_debug_current_hit_pin = button->pin;
}

static void RecordHitDebugEvent(const ButtonInput_t *button)
{
  if (button == NULL)
  {
    return;
  }

  g_debug_last_hit_key = button->key_id;
  g_debug_last_hit_port = GetDebugPortCode(button->port);
  g_debug_last_hit_pin = button->pin;
  g_debug_hit_event_count++;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM2_Init();
  /* MX_CAN_Init(); */
  MX_TIM3_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  uint16_t phase_offset = 0;
  uint32_t last_flow_tick = HAL_GetTick();
  uint8_t hit_done = 0;
  uint8_t flow_hit_mode = 0;
  uint16_t flow_hit_din0_start = 0;
  uint16_t flow_hit_din0_end = 0;

  if (HAL_TIM_Base_Start_IT(&htim4) != HAL_OK)
  {
    Error_Handler();
  }

	WS2812_Init();
  RenderPowerOnFrame(phase_offset);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint8_t pressed_key = GetPressedHitButton();
    uint8_t button_pressed = (pressed_key != 0u);

    if (!hit_done && button_pressed)
    {
      if (key_lock == 0)
      {
        key_lock = 1;
        hit_done = 1;
        if (pressed_key == HIT_KEY_PB10)
        {
          flow_hit_mode = 1;
          flow_hit_din0_start = 1u;
          flow_hit_din0_end = 47u;
          RenderFlowHitFrame(flow_hit_din0_start, flow_hit_din0_end, phase_offset);
        }
        else if (pressed_key == HIT_KEY_PB11)
        {
          flow_hit_mode = 1;
          flow_hit_din0_start = 48u;
          flow_hit_din0_end = 89u;
          RenderFlowHitFrame(flow_hit_din0_start, flow_hit_din0_end, phase_offset);
        }
        else if (pressed_key == HIT_KEY_PB12)
        {
          flow_hit_mode = 1;
          flow_hit_din0_start = 90u;
          flow_hit_din0_end = 133u;
          RenderFlowHitFrame(flow_hit_din0_start, flow_hit_din0_end, phase_offset);
        }
        else if (pressed_key == HIT_KEY_PB13)
        {
          flow_hit_mode = 1;
          flow_hit_din0_start = 134u;
          flow_hit_din0_end = 173u;
          RenderFlowHitFrame(flow_hit_din0_start, flow_hit_din0_end, phase_offset);
        }
        else if (pressed_key == HIT_KEY_PB14)
        {
          flow_hit_mode = 1;
          flow_hit_din0_start = 174u;
          flow_hit_din0_end = 209u;
          RenderFlowHitFrame(flow_hit_din0_start, flow_hit_din0_end, phase_offset);
        }
        else if (pressed_key == HIT_KEY_PB15)
        {
          flow_hit_mode = 1;
          flow_hit_din0_start = 210u;
          flow_hit_din0_end = 229u;
          RenderFlowHitFrame(flow_hit_din0_start, flow_hit_din0_end, phase_offset);
        }
        else if (pressed_key == HIT_KEY_PA4)
        {
          flow_hit_mode = 1;
          flow_hit_din0_start = 230u;
          flow_hit_din0_end = 249u;
          RenderFlowHitFrame(flow_hit_din0_start, flow_hit_din0_end, phase_offset);
        }
        else if (pressed_key == HIT_KEY_PB6)
        {
          flow_hit_mode = 1;
          flow_hit_din0_start = 250u;
          flow_hit_din0_end = 261u;
          RenderFlowHitFrame(flow_hit_din0_start, flow_hit_din0_end, phase_offset);
        }
        else if (pressed_key == HIT_KEY_PB7)
        {
          flow_hit_mode = 1;
          flow_hit_din0_start = 262u;
          flow_hit_din0_end = 270u;
          RenderFlowHitFrame(flow_hit_din0_start, flow_hit_din0_end, phase_offset);
        }
        else
        {
          flow_hit_mode = 0;
          PlayHitEffect();
        }
      }
    }

    if (!button_pressed)
    {
      key_lock = 0;
    }

    if ((flow_hit_mode || !hit_done) && ((HAL_GetTick() - last_flow_tick) >= FLOW_DELAY_MS))
    {
      last_flow_tick = HAL_GetTick();
      phase_offset = (uint16_t)((phase_offset + ARROW_PHASE_COUNT - 1u) % ARROW_PHASE_COUNT);
      if (flow_hit_mode)
      {
        RenderFlowHitFrame(flow_hit_din0_start, flow_hit_din0_end, phase_offset);
      }
      else
      {
        RenderPowerOnFrame(phase_offset);
      }
    }

    HAL_Delay(1);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM4)
  {
    KeyScan_TimerTick();
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
