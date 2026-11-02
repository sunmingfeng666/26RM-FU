#include "motor_uart.h"
UART_HandleTypeDef huart_motor;
HAL_StatusTypeDef MotorUart_HardwareInit(uint32_t *boot) {
    __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_USART1_CLK_ENABLE();
    GPIO_InitTypeDef gpio={0};
    gpio.Pin=GPIO_PIN_9;gpio.Mode=GPIO_MODE_AF_PP;gpio.Speed=GPIO_SPEED_FREQ_HIGH;HAL_GPIO_Init(GPIOA,&gpio);
    gpio.Pin=GPIO_PIN_10;gpio.Mode=GPIO_MODE_INPUT;gpio.Pull=GPIO_PULLUP;HAL_GPIO_Init(GPIOA,&gpio);
    huart_motor.Instance=USART1;huart_motor.Init.BaudRate=115200;
    huart_motor.Init.WordLength=UART_WORDLENGTH_8B;huart_motor.Init.StopBits=UART_STOPBITS_1;
    huart_motor.Init.Parity=UART_PARITY_NONE;huart_motor.Init.Mode=UART_MODE_TX_RX;
    huart_motor.Init.HwFlowCtl=UART_HWCONTROL_NONE;huart_motor.Init.OverSampling=UART_OVERSAMPLING_16;
    if(HAL_UART_Init(&huart_motor)!=HAL_OK) return HAL_ERROR;
    HAL_NVIC_SetPriority(USART1_IRQn,2,0);HAL_NVIC_EnableIRQ(USART1_IRQn);
    /* F103 has no RNG. Mix internal Vref ADC noise and sampling jitter for boot identity.
       This is a non-cryptographic restart discriminator, not an authentication key. */
    __HAL_RCC_ADC1_CLK_ENABLE();__HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV6);
    ADC_HandleTypeDef adc={0};adc.Instance=ADC1;adc.Init.DataAlign=ADC_DATAALIGN_RIGHT;
    adc.Init.ScanConvMode=ADC_SCAN_DISABLE;adc.Init.ContinuousConvMode=DISABLE;
    adc.Init.DiscontinuousConvMode=DISABLE;adc.Init.ExternalTrigConv=ADC_SOFTWARE_START;adc.Init.NbrOfConversion=1;
    if(HAL_ADC_Init(&adc)!=HAL_OK || HAL_ADCEx_Calibration_Start(&adc)!=HAL_OK) return HAL_ERROR;
    ADC_ChannelConfTypeDef channel={0};channel.Channel=ADC_CHANNEL_VREFINT;channel.Rank=ADC_REGULAR_RANK_1;
    channel.SamplingTime=ADC_SAMPLETIME_239CYCLES_5;
    if(HAL_ADC_ConfigChannel(&adc,&channel)!=HAL_OK) return HAL_ERROR;
    uint32_t seed=HAL_GetUIDw0()^HAL_GetUIDw1()^HAL_GetUIDw2();
    for(unsigned i=0;i<64;i++) {
        if(HAL_ADC_Start(&adc)!=HAL_OK || HAL_ADC_PollForConversion(&adc,2)!=HAL_OK) return HAL_ERROR;
        seed^=HAL_ADC_GetValue(&adc)+(SysTick->VAL<<12)+i;
        seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
        (void)HAL_ADC_Stop(&adc);
    }
    (void)HAL_ADC_DeInit(&adc);__HAL_RCC_ADC1_CLK_DISABLE();*boot=seed?seed:1;
    return HAL_OK;
}
