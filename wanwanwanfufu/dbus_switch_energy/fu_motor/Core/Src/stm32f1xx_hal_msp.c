#include "main.h"
void HAL_MspInit(void) {
    __HAL_RCC_AFIO_CLK_ENABLE(); __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG(); /* PA13/PA14 SWD remains available. */
}
void HAL_CAN_MspInit(CAN_HandleTypeDef *can) {
    if(can->Instance!=CAN1) return;
    __HAL_RCC_CAN1_CLK_ENABLE(); __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio={0};
    gpio.Pin=GPIO_PIN_11; gpio.Mode=GPIO_MODE_INPUT; gpio.Pull=GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA,&gpio);
    gpio.Pin=GPIO_PIN_12; gpio.Mode=GPIO_MODE_AF_PP; gpio.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA,&gpio);
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn,1,0); HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn,1,0); HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
}
void HAL_CAN_MspDeInit(CAN_HandleTypeDef *can) {
    if(can->Instance!=CAN1) return;
    __HAL_RCC_CAN1_CLK_DISABLE(); HAL_GPIO_DeInit(GPIOA,GPIO_PIN_11|GPIO_PIN_12);
    HAL_NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn); HAL_NVIC_DisableIRQ(CAN1_SCE_IRQn);
}
