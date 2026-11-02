#ifndef KEY_TEST_HOST_MAIN_H
#define KEY_TEST_HOST_MAIN_H
#include <stdint.h>
typedef struct { uint32_t id; } GPIO_TypeDef;
extern GPIO_TypeDef host_gpio_a, host_gpio_b;
#define GPIOA (&host_gpio_a)
#define GPIOB (&host_gpio_b)
#define GPIO_PIN_4 (1U << 4)
#define GPIO_PIN_6 (1U << 6)
#define GPIO_PIN_7 (1U << 7)
#define GPIO_PIN_10 (1U << 10)
#define GPIO_PIN_11 (1U << 11)
#define GPIO_PIN_12 (1U << 12)
#define GPIO_PIN_13 (1U << 13)
#define GPIO_PIN_14 (1U << 14)
#define GPIO_PIN_15 (1U << 15)
#define GPIO_MODE_OUTPUT_PP 1U
#define GPIO_NOPULL 0U
#define GPIO_SPEED_FREQ_LOW 0U
#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 } GPIO_PinState;
typedef enum { HAL_OK = 0, HAL_ERROR = 1 } HAL_StatusTypeDef;
typedef struct { uint32_t Pin, Mode, Pull, Speed; } GPIO_InitTypeDef;
uint32_t HAL_GetTick(void);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *gpio);
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __set_PRIMASK(uint32_t mask);
#endif
