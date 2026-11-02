#ifndef ENERGY_MOTOR_LINK_H
#define ENERGY_MOTOR_LINK_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Independent of the lamp CAN/ESP-NOW protocol. Explicit little-endian codec. */
#define ML_FRAME_SIZE 40u
#define ML_PERIOD_MS 100u
#define ML_TIMEOUT_MS 500u
enum { ML_HELLO=1, ML_HEARTBEAT, ML_SET, ML_STATUS };
enum { ML_NONE=0, ML_LOCAL, ML_REMOTE, ML_DEBUG };
enum { ML_OK=0, ML_SESSION, ML_OFFLINE, ML_OLD, ML_FAULT, ML_KEYS };
typedef struct {
    uint8_t type,mode,owner,result,reason,online;
    uint32_t boot,epoch,session,seq,command,heartbeat;
    int16_t rpm;
} MlMessage;
typedef struct { uint8_t bytes[ML_FRAME_SIZE],used; uint32_t errors; } MlParser;
int ml_newer(uint32_t a,uint32_t b);
size_t ml_encode(const MlMessage *m,uint8_t out[ML_FRAME_SIZE]);
int ml_parse(MlParser *p,uint8_t byte,MlMessage *m);
#ifdef __cplusplus
}
#endif
#endif
