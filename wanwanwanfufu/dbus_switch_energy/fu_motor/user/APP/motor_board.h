#ifndef FU_MOTOR_BOARD_H
#define FU_MOTOR_BOARD_H
#include "motor_control.h"
#include "motor_can.h"
#include "motor_remote.h"
typedef struct {
    MotorControl control;
    MotorCanFeedback can;
    uint32_t uptime_ms;
    uint32_t debug_sequence,debug_lease_active;
    MotorRemote remote;
    uint32_t remote_ready;
} MotorBoardDebug;
extern volatile MotorBoardDebug g_motor_debug;
/* Write command/heartbeat first, sequence LAST. 1=big, 2=small, 3=STOP.
   A debug-started run requires a changing heartbeat within every 500 ms.
   Physical PB10/PB11 operation is independent of this commissioning lease. */
typedef struct { uint32_t command,heartbeat,sequence; } MotorDebugRequest;
extern volatile MotorDebugRequest g_motor_request;
HAL_StatusTypeDef MotorBoard_Init(void);
void MotorBoard_Update(void);
#endif
