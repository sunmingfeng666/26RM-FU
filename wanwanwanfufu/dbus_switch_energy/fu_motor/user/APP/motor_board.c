#include "motor_board.h"
#include "motor_uart.h"
static MotorControl control;
static MotorRemote remote;
static uint8_t remote_ready;
static uint32_t last_step_ms;
static uint32_t debug_sequence,debug_heartbeat,debug_last_ms;
static uint8_t debug_lease_active;
volatile MotorBoardDebug g_motor_debug;
volatile MotorDebugRequest g_motor_request;
HAL_StatusTypeDef MotorBoard_Init(void) {
    MotorControl_Init(&control,HAL_GetTick()); last_step_ms=HAL_GetTick();
    debug_sequence=g_motor_request.sequence; debug_lease_active=0;
    HAL_StatusTypeDef result=MotorCan_Init();
    if(result!=HAL_OK) return result;
    MotorCan_CancelPending();(void)MotorCan_Send(0);
    uint32_t boot=0;
    remote_ready=0;result=MotorUart_HardwareInit(&boot);
    MotorRemote_Init(&remote,boot);
    if(result==HAL_OK) result=MotorUart_Init();
    if(result==HAL_OK) remote_ready=1;else g_motor_uart_debug.errors++;
    // UART/entropy failure disables remote control, while local keys remain usable.
    return HAL_OK;
}
void MotorBoard_Update(void) {
    MotorCanFeedback received;
    MotorCan_Snapshot(&received);
    uint32_t now=HAL_GetTick();
    if(received.received) {
        MotorControl_Feedback(&control,received.speed_median_rpm,received.last_rx_ms);
        control.measured_current=received.measured_current; control.temperature=received.temperature;
    }
    uint8_t keys=0;
    if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_10)==GPIO_PIN_RESET) keys|=MOTOR_KEY_BIG;
    if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_11)==GPIO_PIN_RESET) keys|=MOTOR_KEY_SMALL;
    uint32_t before=control.transitions;
    uint8_t armed=control.keys_armed;
    uint8_t fault=MotorCan_TakeFault();
    MotorControl_Poll(&control,keys,now,fault);
    if((armed && !control.keys_armed) || (control.keys_stable==MOTOR_KEY_BOTH && keys==MOTOR_KEY_BOTH)) {
        MotorRemote_Local(&remote,&control,ML_LOCAL);debug_lease_active=0;
    }
    uint32_t request_sequence=g_motor_request.sequence;
    uint32_t request_command=g_motor_request.command;
    if((int32_t)(request_sequence-debug_sequence)>0 && request_sequence==g_motor_request.sequence) {
        debug_sequence=request_sequence;
        if(request_command==3) {
            MotorControl_Stop(&control,MOTOR_REASON_KEY); debug_lease_active=0;
            MotorRemote_Local(&remote,&control,ML_DEBUG);
        } else if((request_command==MOTOR_BIG || request_command==MOTOR_SMALL) && keys==0) {
            MotorControl_RequestMode(&control,(uint8_t)request_command,now,fault);
            debug_lease_active=control.mode!=MOTOR_STOP;
            debug_last_ms=now; debug_heartbeat=g_motor_request.heartbeat;
            MotorRemote_Local(&remote,&control,ML_DEBUG);
        }
    }
    MotorRemote_Poll(&remote,&control,now);
    if(remote_ready) MotorUart_Poll(&remote,&control,now,fault);
    if(remote.owner==ML_REMOTE || control.mode==MOTOR_STOP) debug_lease_active=0;
    if(debug_lease_active) {
        uint32_t heartbeat=g_motor_request.heartbeat;
        if(heartbeat!=debug_heartbeat) { debug_heartbeat=heartbeat; debug_last_ms=now; }
        if((uint32_t)(now-debug_last_ms)>=500) {
            MotorControl_Stop(&control,MOTOR_REASON_DEBUG_TIMEOUT); debug_lease_active=0;
        }
    }
    if(before!=control.transitions) {
        /* Do not leave an old nonzero frame queued across STOP or mode changes. */
        MotorCan_CancelPending(); (void)MotorCan_Send(0); last_step_ms=now;
    }
    if((uint32_t)(now-last_step_ms)>=MOTOR_CONTROL_PERIOD_MS) {
        uint32_t elapsed_ms=(uint32_t)(now-last_step_ms);
        last_step_ms=now;
        uint32_t step_transitions=control.transitions;
        MotorControl_StepElapsed(&control,elapsed_ms);
        /* A control timing fault must also cancel older queued torque frames. */
        if(step_transitions!=control.transitions) MotorCan_CancelPending();
        if(MotorCan_Send(control.command_current)!=HAL_OK) {
            MotorControl_Stop(&control,MOTOR_REASON_CAN);
            MotorCan_CancelPending(); (void)MotorCan_Send(0);
        }
    }
    /* Debug snapshot is read-only; changing it in Watch never commands motion. */
    g_motor_debug.control=control; g_motor_debug.can=received; g_motor_debug.uptime_ms=now;
    g_motor_debug.debug_sequence=debug_sequence; g_motor_debug.debug_lease_active=debug_lease_active;
    g_motor_debug.remote=remote;g_motor_debug.remote_ready=remote_ready;
    if(remote_ready) MotorUart_Report(&remote,&control,now);
}
