#ifndef FU_MOTOR_CONTROL_H
#define FU_MOTOR_CONTROL_H
#include <stdint.h>

/* Standalone F103 motor board; no dependency on the lamp/remote protocol. */
enum { MOTOR_STOP=0, MOTOR_BIG=1, MOTOR_SMALL=2 };
enum { MOTOR_REASON_BOOT=0, MOTOR_REASON_KEY, MOTOR_REASON_FEEDBACK, MOTOR_REASON_CAN,
       MOTOR_REASON_CONFIG, MOTOR_REASON_TEMPERATURE, MOTOR_REASON_OVERSPEED, MOTOR_REASON_DEBUG_TIMEOUT,
       MOTOR_REASON_REMOTE_TIMEOUT,MOTOR_REASON_REMOTE_STOP,MOTOR_REASON_REMOTE_START };
enum { MOTOR_KEY_BIG=1, MOTOR_KEY_SMALL=2, MOTOR_KEY_BOTH=3 };
#define MOTOR_CONTROL_PERIOD_MS 1u
#define MOTOR_KEY_DEBOUNCE_MS 20u
#define MOTOR_FEEDBACK_TIMEOUT_MS 100u
#define MOTOR_CURRENT_LIMIT 12000.0f /* Hard cap; C620 protocol full scale is 16384 (20 A torque current). */
#define MOTOR_INTEGRAL_LIMIT 4000.0f
#define MOTOR_SMALL_RPM 570.0f
#define MOTOR_SMALL_RAMP_RPM 5.0f
#define MOTOR_REDUCTION_RATIO 57.0f
#define MOTOR_TEMPERATURE_STOP_C 70u
#define MOTOR_OVERSPEED_RPM 2200

/* Latched on a NEW mode start. Editing settings alone cannot start the motor. */
typedef struct {
    float kp,ki;
    float inertia_raw_per_rpm_s,observer_gain,observer_bandwidth,current_limit;
    float speed_ff_raw_per_rpm,bias_raw;
} MotorModeTuning;
typedef struct { MotorModeTuning big,small; } MotorTuning;
extern volatile MotorTuning g_motor_tuning;

typedef struct {
    uint8_t mode, reason, feedback_online, keys_raw, keys_stable, keys_armed;
    uint8_t feedback_seen;
    int16_t measured_rpm, command_current;
    uint32_t feedback_ms, key_change_ms, random_seed, transitions;
    uint32_t control_steps;
    float target_rpm, output_rad_s, phase_s, big_a, big_w, big_b;
    float kp, ki, kd, integral, previous_error, pid_output;
    int16_t measured_current;
    uint8_t temperature;
    float current_limit,inertia,observer_gain,observer_bandwidth;
    float filtered_rpm,observer_rpm,observer_current,load_estimate;
    float target_accel_rpm_s,feedforward;
    uint32_t saturated_steps;
    uint32_t phase_elapsed_ms;
    float speed_ff_gain,bias_ff;
} MotorControl;

void MotorControl_Init(MotorControl *c,uint32_t now);
void MotorControl_Feedback(MotorControl *c,int16_t rpm,uint32_t received_ms);
void MotorControl_Stop(MotorControl *c,uint8_t reason);
/* Debug requests share the physical-key start checks; caller supplies a bounded lease. */
void MotorControl_RequestMode(MotorControl *c,uint8_t mode,uint32_t now,uint8_t fault);
/* Call every 1 ms in main, keys bits are 1 when pressed (GPIO low). */
void MotorControl_Poll(MotorControl *c,uint8_t keys,uint32_t now,uint8_t can_fault);
/* Ki settings retain the per-10-ms convention; integration scales with elapsed time. */
void MotorControl_Step(MotorControl *c);
void MotorControl_StepElapsed(MotorControl *c,uint32_t elapsed_ms);
#endif
