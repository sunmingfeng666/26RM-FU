#include "main.h"
#include "gpio.h"
#include "dma.h"
#include "tim.h"
#include "slave_key_test.h"

/* This standalone 8 MHz board keeps the original 72 MHz LED timing. */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc={0};RCC_ClkInitTypeDef clocks={0};
    osc.OscillatorType=RCC_OSCILLATORTYPE_HSE;osc.HSEState=RCC_HSE_ON;
    osc.HSEPredivValue=RCC_HSE_PREDIV_DIV1;osc.HSIState=RCC_HSI_ON;
    osc.PLL.PLLState=RCC_PLL_ON;osc.PLL.PLLSource=RCC_PLLSOURCE_HSE;osc.PLL.PLLMUL=RCC_PLL_MUL9;
    if(HAL_RCC_OscConfig(&osc)!=HAL_OK) Error_Handler();
    clocks.ClockType=RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource=RCC_SYSCLKSOURCE_PLLCLK;clocks.AHBCLKDivider=RCC_SYSCLK_DIV1;
    clocks.APB1CLKDivider=RCC_HCLK_DIV2;clocks.APB2CLKDivider=RCC_HCLK_DIV1;
    if(HAL_RCC_ClockConfig(&clocks,FLASH_LATENCY_2)!=HAL_OK) Error_Handler();
}

int main(void)
{
    HAL_Init();SystemClock_Config();MX_GPIO_Init();MX_DMA_Init();MX_TIM2_Init();MX_TIM3_Init();
    if(SlaveKeyTest_Init()!=HAL_OK) Error_Handler();
    while(1) {SlaveKeyTest_Update();HAL_Delay(1);}
}
void Error_Handler(void) {__disable_irq();while(1) {}}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file,uint32_t line) {(void)file;(void)line;Error_Handler();}
#endif
