#include "main.h"
#include "stm32f1xx_it.h"
extern DMA_HandleTypeDef hdma_tim2_ch1,hdma_tim2_ch2_ch4,hdma_tim2_ch3;
extern DMA_HandleTypeDef hdma_tim3_ch1_trig,hdma_tim3_ch3;
void NMI_Handler(void) {while(1) {}}
void HardFault_Handler(void) {while(1) {}}
void MemManage_Handler(void) {while(1) {}}
void BusFault_Handler(void) {while(1) {}}
void UsageFault_Handler(void) {while(1) {}}
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}
void SysTick_Handler(void) {HAL_IncTick();}
void DMA1_Channel1_IRQHandler(void) {HAL_DMA_IRQHandler(&hdma_tim2_ch3);}
void DMA1_Channel2_IRQHandler(void) {HAL_DMA_IRQHandler(&hdma_tim3_ch3);}
void DMA1_Channel5_IRQHandler(void) {HAL_DMA_IRQHandler(&hdma_tim2_ch1);}
void DMA1_Channel6_IRQHandler(void) {HAL_DMA_IRQHandler(&hdma_tim3_ch1_trig);}
void DMA1_Channel7_IRQHandler(void) {HAL_DMA_IRQHandler(&hdma_tim2_ch2_ch4);}
