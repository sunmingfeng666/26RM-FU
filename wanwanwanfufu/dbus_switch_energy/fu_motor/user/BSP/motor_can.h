#ifndef FU_MOTOR_CAN_H
#define FU_MOTOR_CAN_H
#include "stm32f1xx_hal.h"
typedef struct {
    uint8_t received, temperature;
    uint16_t encoder;
    int16_t speed_rpm, measured_current;
    uint32_t last_rx_ms, rx_count, invalid_rx_count, tx_count, tx_error_count, error_count;
    int16_t speed_median_rpm,spike_oldest,spike_previous,spike_latest;
    uint32_t speed_spike_count,spike_ms;
} MotorCanFeedback;
extern CAN_HandleTypeDef hcan;
HAL_StatusTypeDef MotorCan_Init(void);
void MotorCan_Snapshot(MotorCanFeedback *out);
uint8_t MotorCan_TakeFault(void);
void MotorCan_CancelPending(void);
HAL_StatusTypeDef MotorCan_Send(int16_t current);
#endif
