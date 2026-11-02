#include "motor_link.h"
#include <string.h>
static void put32(uint8_t *p,uint32_t v) {for(unsigned i=0;i<4;i++) p[i]=(uint8_t)(v>>(8*i));}
static uint32_t get32(const uint8_t *p) {return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint16_t crc16(const uint8_t *p,size_t n) {
    uint16_t c=0xffff;
    while(n--) {c^=(uint16_t)*p++<<8;for(unsigned i=0;i<8;i++) c=(uint16_t)((c&0x8000)?(c<<1)^0x1021:c<<1);}
    return c;
}
int ml_newer(uint32_t a,uint32_t b) {return a!=b && (uint32_t)(a-b)<0x80000000u;}
size_t ml_encode(const MlMessage *m,uint8_t out[ML_FRAME_SIZE]) {
    if(m->type<ML_HELLO || m->type>ML_STATUS || m->mode>2) return 0;
    memset(out,0,ML_FRAME_SIZE);out[0]=0xa7;out[1]=0x7a;out[2]=1;out[3]=ML_FRAME_SIZE;
    out[4]=m->type;out[5]=m->mode;out[6]=m->owner;out[7]=m->result;out[8]=m->reason;out[9]=m->online;
    put32(out+10,m->boot);put32(out+14,m->epoch);put32(out+18,m->session);
    put32(out+22,m->seq);put32(out+26,m->command);put32(out+30,m->heartbeat);
    out[34]=(uint8_t)m->rpm;out[35]=(uint8_t)((uint16_t)m->rpm>>8);
    uint16_t crc=crc16(out+2,36);out[38]=(uint8_t)crc;out[39]=(uint8_t)(crc>>8);return ML_FRAME_SIZE;
}
int ml_parse(MlParser *p,uint8_t byte,MlMessage *m) {
    p->bytes[p->used++]=byte;
    while(p->used>=2) {
        uint8_t *b=p->bytes;
        if(b[0]!=0xa7 || b[1]!=0x7a) goto discard;
        if(p->used>=4 && (b[2]!=1 || b[3]!=ML_FRAME_SIZE)) {p->errors++;goto discard;}
        if(p->used<ML_FRAME_SIZE) return 0;
        if(b[4]<ML_HELLO || b[4]>ML_STATUS || b[5]>2 || b[6]>ML_DEBUG || b[7]>ML_KEYS || b[9]>1 || b[36] || b[37] ||
           crc16(b+2,36)!=(uint16_t)(b[38]|((uint16_t)b[39]<<8))) {p->errors++;goto discard;}
        memset(m,0,sizeof(*m));m->type=b[4];m->mode=b[5];m->owner=b[6];m->result=b[7];m->reason=b[8];m->online=b[9];
        m->boot=get32(b+10);m->epoch=get32(b+14);m->session=get32(b+18);
        m->seq=get32(b+22);m->command=get32(b+26);m->heartbeat=get32(b+30);
        m->rpm=(int16_t)(b[34]|((uint16_t)b[35]<<8));p->used=0;return 1;
discard:
        memmove(b,b+1,--p->used);
    }
    return 0;
}
