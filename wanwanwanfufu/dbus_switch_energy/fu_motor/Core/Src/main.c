#include "main.h"
#include "motor_board.h"

static void SystemClock_Config(void) {
    RCC_OscInitTypeDef osc={0}; RCC_ClkInitTypeDef clocks={0};
    osc.OscillatorType=RCC_OSCILLATORTYPE_HSE;
    osc.HSEState=RCC_HSE_ON; osc.HSEPredivValue=RCC_HSE_PREDIV_DIV1;
    osc.HSIState=RCC_HSI_ON; osc.PLL.PLLState=RCC_PLL_ON;
    osc.PLL.PLLSource=RCC_PLLSOURCE_HSE; osc.PLL.PLLMUL=RCC_PLL_MUL9;
    if(HAL_RCC_OscConfig(&osc)!=HAL_OK) Error_Handler();
    clocks.ClockType=RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource=RCC_SYSCLKSOURCE_PLLCLK; clocks.AHBCLKDivider=RCC_SYSCLK_DIV1;
    clocks.APB1CLKDivider=RCC_HCLK_DIV2; clocks.APB2CLKDivider=RCC_HCLK_DIV1;
    if(HAL_RCC_ClockConfig(&clocks,FLASH_LATENCY_2)!=HAL_OK) Error_Handler();
}
static void Keys_Init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef gpio={0};
    gpio.Pin=GPIO_PIN_10|GPIO_PIN_11; gpio.Mode=GPIO_MODE_INPUT; gpio.Pull=GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB,&gpio);
}
int main(void) {
    HAL_Init(); SystemClock_Config(); Keys_Init();
    if(MotorBoard_Init()!=HAL_OK) Error_Handler();
    /* Keep SWD accessible on F103; run once per SysTick without HAL_Delay's extra tick. */
    uint32_t loop_ms=HAL_GetTick();
    while(1) {
        uint32_t now=HAL_GetTick();
        if(now!=loop_ms) { loop_ms=now; MotorBoard_Update(); }
    }
}
void Error_Handler(void) { __disable_irq(); while(1) {} }
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file,uint32_t line) { (void)file;(void)line;Error_Handler(); }
#endif
