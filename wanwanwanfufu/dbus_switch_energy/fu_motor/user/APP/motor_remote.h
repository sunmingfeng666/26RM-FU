#ifndef FU_MOTOR_REMOTE_H
#define FU_MOTOR_REMOTE_H
#include "motor_control.h"
#include "motor_link.h"
typedef struct {
    uint32_t boot,epoch,session,command,heartbeat,status_seq,echo_seq,last_ms,echo_ms;
    uint32_t accepted,rejected,duplicates;
    uint8_t owner,result,have_heartbeat,dirty;
} MotorRemote;
void MotorRemote_Init(MotorRemote *r,uint32_t boot);
void MotorRemote_Invalidate(MotorRemote *r,MotorControl *c,uint8_t reason);
void MotorRemote_Local(MotorRemote *r,MotorControl *c,uint8_t owner);
void MotorRemote_Poll(MotorRemote *r,MotorControl *c,uint32_t now);
void MotorRemote_Receive(MotorRemote *r,MotorControl *c,const MlMessage *m,uint32_t now,uint8_t fault);
void MotorRemote_Status(MotorRemote *r,const MotorControl *c,uint32_t now,MlMessage *out);
#endif
