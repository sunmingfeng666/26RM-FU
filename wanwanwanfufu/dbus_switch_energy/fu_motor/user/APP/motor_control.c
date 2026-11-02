#include "motor_control.h"
#include <math.h>
#include <string.h>

#define MOTOR_PI 3.1415926f
volatile MotorTuning g_motor_tuning={
    /* Saved 2026-10-19: retained settings, not the rejected high-output trials. */
    .big={
        .kp=24.0f, .ki=0.005f,
        .inertia_raw_per_rpm_s=5.0f,
        .observer_gain=0.0f, .observer_bandwidth=6.0f,
        .current_limit=10000.0f,
        .speed_ff_raw_per_rpm=0.0f, .bias_raw=0.0f
    },
    .small={
        .kp=24.0f, .ki=0.005f,
        .inertia_raw_per_rpm_s=0.6f,
        .observer_gain=1.0f, .observer_bandwidth=6.0f,
        .current_limit=8000.0f,
        .speed_ff_raw_per_rpm=0.0f, .bias_raw=0.0f
    }
};
static float limit(float value,float maximum) {
    if(value>maximum) return maximum;
    if(value< -maximum) return -maximum;
    return value;
}
static void reset_loop(MotorControl *c) {
    c->target_rpm=0.0f; c->output_rad_s=0.0f; c->phase_s=0.0f;
    c->integral=0.0f; c->previous_error=0.0f;
    c->pid_output=0.0f; c->command_current=0;
    c->filtered_rpm=(float)c->measured_rpm;
    c->observer_rpm=(float)c->measured_rpm; c->observer_current=0.0f;
    c->load_estimate=0.0f; c->target_accel_rpm_s=0.0f; c->feedforward=0.0f;
    c->saturated_steps=0;
    c->phase_elapsed_ms=0;
}
void MotorControl_Init(MotorControl *c,uint32_t now) {
    memset(c,0,sizeof(*c)); c->reason=MOTOR_REASON_BOOT;
    c->key_change_ms=now; c->random_seed=0x13572468u;
    c->big_a=0.780f; c->big_w=1.884f; c->big_b=2.090f-c->big_a;
}
void MotorControl_Feedback(MotorControl *c,int16_t rpm,uint32_t received_ms) {
    c->measured_rpm=rpm; c->feedback_ms=received_ms; c->feedback_seen=1;
}
void MotorControl_Stop(MotorControl *c,uint8_t reason) {
    if(c->mode!=MOTOR_STOP) ++c->transitions;
    c->mode=MOTOR_STOP; c->reason=reason; reset_loop(c);
}
static float random_range(MotorControl *c,float minimum,float maximum,uint32_t now) {
    /* Same generator and ranges as the old true_dafu TIM_Task_ResetSineParams. */
    c->random_seed=c->random_seed*1664525u+1013904223u+now;
    float ratio=(float)(c->random_seed&0xffffu)/65535.0f;
    return minimum+(maximum-minimum)*ratio;
}
static void select_mode(MotorControl *c,uint8_t mode,uint32_t now,uint8_t fault) {
    if(fault) { MotorControl_Stop(c,MOTOR_REASON_CAN); return; }
    if(!c->feedback_online) { MotorControl_Stop(c,MOTOR_REASON_FEEDBACK); return; }
    if(c->temperature>=MOTOR_TEMPERATURE_STOP_C) { MotorControl_Stop(c,MOTOR_REASON_TEMPERATURE); return; }
    if(c->measured_rpm>MOTOR_OVERSPEED_RPM || c->measured_rpm< -MOTOR_OVERSPEED_RPM) {
        MotorControl_Stop(c,MOTOR_REASON_OVERSPEED); return;
    }
    if(c->mode==mode) return; /* Holding or reselecting a mode never resets its phase. */
    MotorModeTuning t=mode==MOTOR_BIG?g_motor_tuning.big:g_motor_tuning.small;
    if(!isfinite(t.kp)||!isfinite(t.ki)||!isfinite(t.speed_ff_raw_per_rpm)||!isfinite(t.bias_raw)||
       !isfinite(t.inertia_raw_per_rpm_s)||!isfinite(t.observer_gain)||!isfinite(t.observer_bandwidth)||!isfinite(t.current_limit)||
       t.kp<1||t.kp>80||t.ki<0||t.ki>2||
       t.speed_ff_raw_per_rpm<0||t.speed_ff_raw_per_rpm>20||fabsf(t.bias_raw)>3000||
       t.inertia_raw_per_rpm_s<0||t.inertia_raw_per_rpm_s>15||
       t.observer_gain<0||t.observer_gain>1||t.observer_bandwidth<0.5f||t.observer_bandwidth>20||
       t.current_limit<500||t.current_limit>MOTOR_CURRENT_LIMIT) {
        MotorControl_Stop(c,MOTOR_REASON_CONFIG); return;
    }
    reset_loop(c); c->mode=mode; c->reason=MOTOR_REASON_KEY; ++c->transitions;
    c->current_limit=t.current_limit; c->inertia=t.inertia_raw_per_rpm_s;
    c->observer_gain=t.observer_gain; c->observer_bandwidth=t.observer_bandwidth; c->kd=0;
    c->kp=t.kp; c->ki=t.ki; c->speed_ff_gain=t.speed_ff_raw_per_rpm; c->bias_ff=t.bias_raw;
    if(mode==MOTOR_BIG) {
        c->big_a=random_range(c,0.780f,1.045f,now);
        c->big_w=random_range(c,1.884f,2.000f,now);
        c->big_b=2.090f-c->big_a;
    }
}
void MotorControl_RequestMode(MotorControl *c,uint8_t mode,uint32_t now,uint8_t fault) {
    if(mode==MOTOR_STOP) MotorControl_Stop(c,MOTOR_REASON_KEY);
    else if(mode==MOTOR_BIG || mode==MOTOR_SMALL) select_mode(c,mode,now,fault);
}
void MotorControl_Poll(MotorControl *c,uint8_t keys,uint32_t now,uint8_t can_fault) {
    c->feedback_online=c->feedback_seen && (uint32_t)(now-c->feedback_ms)<MOTOR_FEEDBACK_TIMEOUT_MS;
    if(can_fault) MotorControl_Stop(c,MOTOR_REASON_CAN);
    else if(c->mode!=MOTOR_STOP && !c->feedback_online) MotorControl_Stop(c,MOTOR_REASON_FEEDBACK);
    else if(c->temperature>=MOTOR_TEMPERATURE_STOP_C) MotorControl_Stop(c,MOTOR_REASON_TEMPERATURE);
    else if(c->measured_rpm>MOTOR_OVERSPEED_RPM || c->measured_rpm< -MOTOR_OVERSPEED_RPM)
        MotorControl_Stop(c,MOTOR_REASON_OVERSPEED);
    keys&=MOTOR_KEY_BOTH;
    if(keys!=c->keys_raw) { c->keys_raw=keys; c->key_change_ms=now; }
    if((uint32_t)(now-c->key_change_ms)<MOTOR_KEY_DEBOUNCE_MS) return;
    c->keys_stable=keys;
    if(keys==0) { c->keys_armed=1; return; }
    if(keys==MOTOR_KEY_BOTH) {
        MotorControl_Stop(c,MOTOR_REASON_KEY); c->keys_armed=0; return;
    }
    if(c->keys_armed) {
        c->keys_armed=0;
        select_mode(c,keys==MOTOR_KEY_BIG?MOTOR_BIG:MOTOR_SMALL,now,can_fault);
    }
}
void MotorControl_Step(MotorControl *c) {
    MotorControl_StepElapsed(c,MOTOR_CONTROL_PERIOD_MS);
}
void MotorControl_StepElapsed(MotorControl *c,uint32_t elapsed_ms) {
    ++c->control_steps;
    if(c->mode==MOTOR_STOP) { c->command_current=0; return; }
    if(!c->feedback_online) { MotorControl_Stop(c,MOTOR_REASON_FEEDBACK); return; }
    if(elapsed_ms==0 || elapsed_ms>20) { MotorControl_Stop(c,MOTOR_REASON_CONFIG); return; }
    float dt=(float)elapsed_ms*0.001f;
    if(c->mode==MOTOR_BIG) {
        c->phase_elapsed_ms+=elapsed_ms;
        c->phase_s=(float)c->phase_elapsed_ms*0.001f;
        c->output_rad_s=c->big_a*sinf(c->big_w*c->phase_s)+c->big_b;
        float output_rpm=c->output_rad_s*60.0f/(2.0f*MOTOR_PI);
        c->target_rpm=output_rpm*MOTOR_REDUCTION_RATIO;
        c->target_accel_rpm_s=c->big_a*c->big_w*cosf(c->big_w*c->phase_s)*60.0f/(2.0f*MOTOR_PI)*MOTOR_REDUCTION_RATIO;
    } else {
        float before=c->target_rpm;
        c->target_rpm+=MOTOR_SMALL_RAMP_RPM*(float)elapsed_ms/10.0f;
        if(c->target_rpm>MOTOR_SMALL_RPM) c->target_rpm=MOTOR_SMALL_RPM;
        c->output_rad_s=(c->target_rpm/MOTOR_REDUCTION_RATIO)*(2.0f*MOTOR_PI)/60.0f;
        c->target_accel_rpm_s=(c->target_rpm-before)/dt;
    }
    /* Q(s)*(actual_current - J*speed_derivative): both terms use the SAME low-pass.
       This estimates changing load without an assumed mechanical angle or power-on phase. */
    float alpha=(dt*c->observer_bandwidth)/(1.0f+dt*c->observer_bandwidth);
    float previous_observer_rpm=c->observer_rpm;
    c->observer_rpm+=alpha*((float)c->measured_rpm-c->observer_rpm);
    c->observer_current+=alpha*((float)c->measured_current-c->observer_current);
    c->load_estimate=limit(c->observer_current-c->inertia*(c->observer_rpm-previous_observer_rpm)/dt,6000.0f);
    c->filtered_rpm+=(dt/(0.003f+dt))*((float)c->measured_rpm-c->filtered_rpm);
    c->feedforward=c->inertia*c->target_accel_rpm_s+c->observer_gain*c->load_estimate;
    /* Dynamic feedforward is centred on the sine's mean speed; bias covers its DC load.
       This avoids treating periodic loading as a large constant positive torque. */
    float mean_rpm=c->mode==MOTOR_BIG?c->big_b*60.0f/(2.0f*MOTOR_PI)*MOTOR_REDUCTION_RATIO:MOTOR_SMALL_RPM;
    c->feedforward+=c->speed_ff_gain*(c->target_rpm-mean_rpm)+c->bias_ff;
    float error=c->target_rpm-(float)c->measured_rpm;
    float candidate_i=limit(c->integral+c->ki*(float)elapsed_ms/10.0f*error,MOTOR_INTEGRAL_LIMIT);
    float proposed=c->kp*error+candidate_i+c->feedforward;
    /* Conditional integration: unwind freely, but do not accumulate into saturation. */
    if(!((proposed>c->current_limit && error>0)||(proposed< -c->current_limit && error<0)))
        c->integral=candidate_i;
    float requested=c->kp*error+c->integral+c->feedforward;
    c->pid_output=limit(requested,c->current_limit);
    if(requested!=c->pid_output) ++c->saturated_steps;
    c->previous_error=error;
    c->command_current=(int16_t)c->pid_output;
}
