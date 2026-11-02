#include "motor_remote.h"
#include <string.h>
static int online(const MotorRemote *r,uint32_t now) {
    return r->session && r->have_heartbeat && (uint32_t)(now-r->last_ms)<ML_TIMEOUT_MS &&
        (uint32_t)(now-r->echo_ms)<ML_TIMEOUT_MS;
}
void MotorRemote_Init(MotorRemote *r,uint32_t boot) {
    memset(r,0,sizeof(*r));r->boot=boot?boot:1;r->epoch=1;r->dirty=1;
}
void MotorRemote_Invalidate(MotorRemote *r,MotorControl *c,uint8_t reason) {
    if(r->owner==ML_REMOTE) MotorControl_Stop(c,reason);
    if(r->session || r->have_heartbeat) {if(!++r->epoch) ++r->epoch;}
    r->session=0;r->command=0;r->heartbeat=0;r->echo_seq=0;r->have_heartbeat=0;r->result=ML_OK;r->dirty=1;
    if(c->mode==MOTOR_STOP) r->owner=ML_NONE;
}
void MotorRemote_Local(MotorRemote *r,MotorControl *c,uint8_t owner) {
    /* A fresh local press revokes the previous remote command, even in the same mode. */
    r->owner=owner;MotorRemote_Invalidate(r,c,MOTOR_REASON_REMOTE_TIMEOUT);
    r->owner=c->mode==MOTOR_STOP?ML_NONE:owner;
}
void MotorRemote_Poll(MotorRemote *r,MotorControl *c,uint32_t now) {
    if(r->session && ((uint32_t)(now-r->last_ms)>=ML_TIMEOUT_MS ||
       (r->have_heartbeat && (uint32_t)(now-r->echo_ms)>=ML_TIMEOUT_MS)))
        MotorRemote_Invalidate(r,c,MOTOR_REASON_REMOTE_TIMEOUT);
    if(c->mode==MOTOR_STOP && r->owner==ML_REMOTE) MotorRemote_Invalidate(r,c,c->reason);
    if(c->mode==MOTOR_STOP) r->owner=ML_NONE;
}
void MotorRemote_Receive(MotorRemote *r,MotorControl *c,const MlMessage *m,uint32_t now,uint8_t fault) {
    MotorRemote_Poll(r,c,now);
    if(m->type==ML_STATUS || m->boot!=r->boot || m->epoch!=r->epoch || !m->session) {r->rejected++;return;}
    if(m->type==ML_HELLO) {
        if(r->session && r->session!=m->session) {MotorRemote_Invalidate(r,c,MOTOR_REASON_REMOTE_TIMEOUT);return;}
        if(!r->session) {r->session=m->session;r->last_ms=now;r->dirty=1;}
        return; /* Retransmitted HELLO is never a heartbeat. */
    }
    if(m->session!=r->session) {r->rejected++;return;}
    if(m->type==ML_HEARTBEAT) {
        if(!ml_newer(m->seq,r->heartbeat) || !ml_newer(m->command,r->echo_seq) || ml_newer(m->command,r->status_seq)) return;
        r->heartbeat=m->seq;r->echo_seq=m->command;r->last_ms=now;r->echo_ms=now;r->have_heartbeat=1;return;
    }
    if(m->type!=ML_SET) return;
    if(m->seq==r->command) {r->duplicates++;r->dirty=1;return;}
    if(!ml_newer(m->seq,r->command)) {r->rejected++;return;}
    r->command=m->seq;r->result=ML_OK;r->dirty=1;
    if(m->mode==MOTOR_STOP) {
        MotorControl_Stop(c,MOTOR_REASON_REMOTE_STOP);r->owner=ML_NONE;
    } else if(!online(r,now)) r->result=ML_OFFLINE;
    else if(c->keys_raw || c->keys_stable || !c->keys_armed) r->result=ML_KEYS;
    else {
        /* One new START restarts its phase. Duplicate frames never repeat this. */
        MotorControl_Stop(c,MOTOR_REASON_REMOTE_STOP);
        MotorControl_RequestMode(c,m->mode,now,fault);
        if(c->mode!=m->mode) r->result=ML_FAULT;
        else {r->owner=ML_REMOTE;c->reason=MOTOR_REASON_REMOTE_START;}
    }
    if(r->result==ML_OK) r->accepted++;else r->rejected++;
}
void MotorRemote_Status(MotorRemote *r,const MotorControl *c,uint32_t now,MlMessage *out) {
    memset(out,0,sizeof(*out));out->type=ML_STATUS;out->boot=r->boot;out->epoch=r->epoch;out->session=r->session;
    out->seq=++r->status_seq;out->command=r->command;out->heartbeat=r->heartbeat;
    out->mode=c->mode;out->owner=r->owner;out->reason=c->reason;out->result=r->result;
    out->online=(uint8_t)online(r,now);out->rpm=c->measured_rpm;r->dirty=0;
}
