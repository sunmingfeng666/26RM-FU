#include "motor_board.h"
#include "motor_uart.h"
#include "legacy_big/my_can.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

void Big_TIM_Task_Init(void); void Big_TIM_Task_10ms(void);
void Small_TIM_Task_Init(void); void Small_TIM_Task_10ms(void);
extern float Big_motor1_target_speed,Big_motor1_pid_out,Small_motor1_target_speed,Small_motor1_pid_out;
extern float a_param,w_param,b_param;
M3508_Motor_t motor1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart_motor;
static uint8_t *uart_rx;
static uint8_t uart_hw_fail,uart_arm_fail;
static MlMessage last_uart_status;
HAL_StatusTypeDef MotorUart_HardwareInit(uint32_t *boot) {*boot=12345;return uart_hw_fail?HAL_ERROR:HAL_OK;}
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *p,uint8_t *byte,uint16_t n) {
    (void)p;assert(n==1);uart_rx=byte;if(uart_arm_fail) {uart_arm_fail=0;return HAL_ERROR;}return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *p) {(void)p;return HAL_OK;}
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *p,uint8_t *data,uint16_t n) {
    (void)p;assert(n==ML_FRAME_SIZE);MlParser parser={0};
    for(unsigned i=0;i<n;i++) (void)ml_parse(&parser,data[i],&last_uart_status);
    return HAL_OK;
}
CAN_TypeDef test_can_registers;
static uint32_t now,checks,aborts,sent;
static uint8_t pressed,rx_pending,tx_fail;
static uint8_t rx_data[8],tx_data[8];
static CAN_RxHeaderTypeDef rx_header;
static CAN_FilterTypeDef filter;
static CAN_TxHeaderTypeDef tx_header;
static int16_t legacy_current;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1; } } while(0)
uint32_t HAL_GetTick(void) { return now; }
int HAL_GPIO_ReadPin(int port,uint16_t pin) {
    assert(port==GPIOB);return (pressed&(pin==GPIO_PIN_10?1:2))?GPIO_PIN_RESET:GPIO_PIN_SET;
}
HAL_StatusTypeDef HAL_CAN_Init(CAN_HandleTypeDef *c) { (void)c;return HAL_OK; }
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *c,CAN_FilterTypeDef *f) { (void)c;filter=*f;return HAL_OK; }
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *c) { (void)c;return HAL_OK; }
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *c,uint32_t bits) { (void)c;(void)bits;return HAL_OK; }
HAL_StatusTypeDef HAL_CAN_AbortTxRequest(CAN_HandleTypeDef *c,uint32_t bits) { (void)c;assert(bits==7);aborts++;return HAL_OK; }
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *c,CAN_TxHeaderTypeDef *h,uint8_t *data,uint32_t *mailbox) {
    (void)c;*mailbox=1;
    if(tx_fail) {tx_fail=0;return HAL_ERROR;}
    tx_header=*h;memcpy(tx_data,data,8);sent++;return HAL_OK;
}
uint32_t HAL_CAN_GetRxFifoFillLevel(CAN_HandleTypeDef *c,uint32_t fifo) { (void)c;(void)fifo;return rx_pending; }
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *c,uint32_t fifo,CAN_RxHeaderTypeDef *h,uint8_t *data) {
    (void)c;(void)fifo;*h=rx_header;memcpy(data,rx_data,8);rx_pending=0;return HAL_OK;
}
void M3508_Send_Current(CAN_HandleTypeDef *c,int16_t a,int16_t b,int16_t d,int16_t e) {
    (void)c;assert(b==0&&d==0&&e==0);legacy_current=a;
}
void VOFA_justfloat(float a,float b,float c,float d,float e,float f,float g,float h,float j,float k) {
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)j;(void)k;
}
static void receive(uint32_t id,uint32_t dlc,int16_t rpm) {
    rx_header=(CAN_RxHeaderTypeDef){.StdId=id,.IDE=CAN_ID_STD,.RTR=CAN_RTR_DATA,.DLC=dlc};
    memset(rx_data,0,8);uint16_t bits=(uint16_t)rpm;
    rx_data[0]=0x12;rx_data[1]=0x34;rx_data[2]=(uint8_t)(bits>>8);rx_data[3]=(uint8_t)bits;
    rx_data[4]=0xfe;rx_data[5]=0x0c;rx_data[6]=42;rx_pending=1;
    HAL_CAN_RxFifo0MsgPendingCallback(&hcan);
}
static void advance(unsigned ms,uint8_t keys,int feed) {
    pressed=keys;
    while(ms--) { now++;if(feed && now%10==0) receive(0x201,8,100);MotorBoard_Update(); }
}
static int16_t last_current(void) { return (int16_t)((uint16_t)tx_data[0]<<8|tx_data[1]); }
static void core_key(MotorControl *c,uint8_t key,uint32_t t) {
    MotorControl_Feedback(c,0,t);MotorControl_Poll(c,key,t,0);
    MotorControl_Feedback(c,0,t+20);MotorControl_Poll(c,key,t+20,0);
}
static int compare_legacy(void) {
    MotorControl c;now=50;
    MotorControl_Init(&c,0);core_key(&c,0,0);core_key(&c,MOTOR_KEY_BIG,30);
    Big_TIM_Task_Init();
    CHECK(c.big_a==a_param && c.big_w==w_param && c.big_b==b_param);
    CHECK(c.big_a>=0.780f && c.big_a<=1.045f && c.big_w>=1.884f && c.big_w<=2.000f);
    for(unsigned i=0;i<10000;i++) {
        now+=10;motor1.rawSpeed=(int16_t)((i*37)%3000)-900;
        MotorControl_Feedback(&c,motor1.rawSpeed,now);MotorControl_Poll(&c,0,now,0);
        MotorControl_StepElapsed(&c,10);Big_TIM_Task_10ms();
        CHECK(fabsf(c.target_rpm-Big_motor1_target_speed)<6.0f); /* Old float phase accumulates rounding; new elapsed-ms phase does not. */
        CHECK(isfinite(c.pid_output) && fabsf(c.pid_output)<=c.current_limit);
        CHECK(fabsf(c.integral)<=MOTOR_INTEGRAL_LIMIT);
    }
    MotorControl_Init(&c,0);core_key(&c,0,0);core_key(&c,MOTOR_KEY_SMALL,30);Small_TIM_Task_Init();now=50;
    for(unsigned i=0;i<1000;i++) {
        now+=10;motor1.rawSpeed=(int16_t)((i*19)%2000)-500;
        MotorControl_Feedback(&c,motor1.rawSpeed,now);MotorControl_Poll(&c,0,now,0);
        MotorControl_StepElapsed(&c,10);Small_TIM_Task_10ms();
        CHECK(c.target_rpm==Small_motor1_target_speed);
        CHECK(isfinite(c.pid_output) && fabsf(c.pid_output)<=c.current_limit);
        if(i==113) CHECK(c.target_rpm==570.0f);
    }
    return 0;
}
static int core_cases(void) {
    MotorControl c;MotorControl_Init(&c,0);
    core_key(&c,MOTOR_KEY_BIG,0);CHECK(c.mode==MOTOR_STOP); // Held during boot.
    core_key(&c,0,30);core_key(&c,MOTOR_KEY_SMALL,60);CHECK(c.mode==MOTOR_SMALL);
    MotorControl_StepElapsed(&c,10);CHECK(c.target_rpm==5.0f);
    uint32_t transitions=c.transitions;
    core_key(&c,MOTOR_KEY_SMALL,90);CHECK(c.transitions==transitions);
    core_key(&c,0,120);core_key(&c,MOTOR_KEY_SMALL,150);CHECK(c.transitions==transitions && c.target_rpm==5);
    core_key(&c,0,180);core_key(&c,MOTOR_KEY_BIG,210);CHECK(c.mode==MOTOR_BIG && c.target_rpm==0 && c.integral==0);
    core_key(&c,MOTOR_KEY_BOTH,240);CHECK(c.mode==MOTOR_STOP && c.command_current==0);
    core_key(&c,MOTOR_KEY_BIG,270);CHECK(c.mode==MOTOR_STOP); // Partial chord release.
    core_key(&c,0,300);core_key(&c,MOTOR_KEY_BIG,330);CHECK(c.mode==MOTOR_BIG);
    MotorControl_Poll(&c,0,449,0);CHECK(c.mode==MOTOR_BIG);
    MotorControl_Poll(&c,0,450,0);CHECK(c.mode==MOTOR_STOP && c.reason==MOTOR_REASON_FEEDBACK);
    MotorControl_Feedback(&c,0,451);MotorControl_Poll(&c,0,451,0);CHECK(c.mode==MOTOR_STOP && c.feedback_online);
    MotorControl_Poll(&c,0,475,0);CHECK(c.keys_armed);
    core_key(&c,MOTOR_KEY_SMALL,480);CHECK(c.mode==MOTOR_SMALL);
    MotorControl_Poll(&c,0,501,1);CHECK(c.mode==MOTOR_STOP && c.reason==MOTOR_REASON_CAN);
    MotorControl_Init(&c,0xffffffd0u);core_key(&c,0,0xffffffd0u);core_key(&c,1,0xfffffff0u);
    CHECK(c.mode==MOTOR_BIG);MotorControl_Poll(&c,1,103,0);CHECK(c.mode==MOTOR_BIG);
    MotorControl_Poll(&c,1,104,0);CHECK(c.mode==MOTOR_STOP);
    return 0;
}
static int board_cases(void) {
    now=0;CHECK(MotorBoard_Init()==HAL_OK);CHECK(last_current()==0);
    CHECK(hcan.Init.Prescaler==2 && hcan.Init.TimeSeg1==13 && hcan.Init.TimeSeg2==4);
    CHECK(filter.FilterIdHigh==(0x201u<<5) && filter.FilterMaskIdLow==6);
    advance(30,0,0);advance(30,1,0);CHECK(g_motor_debug.control.mode==MOTOR_STOP);
    advance(30,0,1);advance(30,2,1);CHECK(g_motor_debug.control.mode==MOTOR_SMALL);
    advance(1140,0,1);CHECK(g_motor_debug.control.target_rpm==570.0f);
    CHECK(tx_header.StdId==0x200 && tx_header.DLC==8 && tx_header.IDE==CAN_ID_STD);
    for(unsigned i=2;i<8;i++) CHECK(tx_data[i]==0);
    MotorCanFeedback f;receive(0x201,8,-1234);MotorCan_Snapshot(&f);
    CHECK(f.speed_rpm==-1234 && f.encoder==0x1234 && f.measured_current==-500 && f.temperature==42);
    uint32_t count=f.rx_count;
    receive(0x202,8,123);receive(0x201,7,123);MotorCan_Snapshot(&f);
    CHECK(f.rx_count==count && f.invalid_rx_count==2 && f.speed_rpm==-1234);
    CHECK(MotorCan_Send(-16000)==HAL_OK && last_current()==-(int16_t)MOTOR_CURRENT_LIMIT);
    CHECK(MotorCan_Send(16000)==HAL_OK && last_current()==(int16_t)MOTOR_CURRENT_LIMIT);
    advance(30,0,1);uint32_t before=aborts;
    advance(30,3,1);CHECK(g_motor_debug.control.mode==MOTOR_STOP && last_current()==0 && aborts>before);
    advance(40,1,1);CHECK(g_motor_debug.control.mode==MOTOR_STOP);
    advance(30,0,1);
    for(unsigned i=0;i<5;i++) { advance(4,1,1);advance(4,0,1); }
    CHECK(g_motor_debug.control.mode==MOTOR_STOP);advance(30,1,1);CHECK(g_motor_debug.control.mode==MOTOR_BIG);
    advance(110,0,0);CHECK(g_motor_debug.control.mode==MOTOR_STOP && last_current()==0);
    advance(40,0,1);CHECK(g_motor_debug.control.mode==MOTOR_STOP);
    advance(30,2,1);CHECK(g_motor_debug.control.mode==MOTOR_SMALL);
    test_can_registers.ESR=CAN_ESR_BOFF;HAL_CAN_ErrorCallback(&hcan);advance(1,0,1);
    CHECK(g_motor_debug.control.mode==MOTOR_STOP && last_current()==0);
    test_can_registers.ESR=0;advance(30,0,1);CHECK(g_motor_debug.control.mode==MOTOR_STOP);
    advance(30,1,1);CHECK(g_motor_debug.control.mode==MOTOR_BIG);
    tx_fail=1;advance(10,0,1);CHECK(g_motor_debug.control.mode==MOTOR_STOP && last_current()==0);
    g_motor_debug.control.mode=MOTOR_BIG;g_motor_debug.control.command_current=5000;
    advance(10,0,1);CHECK(g_motor_debug.control.mode==MOTOR_STOP && last_current()==0);
    advance(40,0,1);
    g_motor_request.command=MOTOR_SMALL;g_motor_request.heartbeat=1;g_motor_request.sequence=1;
    advance(1,0,1);CHECK(g_motor_debug.control.mode==MOTOR_SMALL && g_motor_debug.debug_lease_active);
    advance(490,0,1);CHECK(g_motor_debug.control.mode==MOTOR_SMALL);
    advance(10,0,1);CHECK(g_motor_debug.control.mode==MOTOR_STOP && g_motor_debug.control.reason==MOTOR_REASON_DEBUG_TIMEOUT);
    g_motor_request.heartbeat=2;advance(30,0,1);CHECK(g_motor_debug.control.mode==MOTOR_STOP);
    g_motor_request.sequence=2;advance(1,0,1);CHECK(g_motor_debug.control.mode==MOTOR_SMALL);
    g_motor_request.command=3;g_motor_request.sequence=3;advance(1,0,1);
    CHECK(g_motor_debug.control.mode==MOTOR_STOP && last_current()==0);
    g_motor_request.command=MOTOR_SMALL;g_motor_request.sequence=2;advance(2,0,1);
    CHECK(g_motor_debug.control.mode==MOTOR_STOP); /* Delayed START older than STOP is ignored. */
    g_motor_request.command=MOTOR_BIG;g_motor_request.sequence=4;advance(1,3,1);
    CHECK(g_motor_debug.control.mode==MOTOR_STOP);
    receive(0x201,8,100);receive(0x201,8,100);receive(0x201,8,3000);MotorCan_Snapshot(&f);
    CHECK(f.speed_rpm==3000 && f.speed_median_rpm==100 && f.speed_spike_count>0);
    receive(0x201,8,3000);MotorCan_Snapshot(&f);CHECK(f.speed_median_rpm==3000);
    now+=101;receive(0x201,8,50);MotorCan_Snapshot(&f);CHECK(f.speed_median_rpm==50);
    CHECK(sent>100);return 0;
}
static int tuned_cases(void) {
    MotorTuning saved=g_motor_tuning;
    MotorControl c;MotorControl_Init(&c,0);core_key(&c,0,0);core_key(&c,MOTOR_KEY_SMALL,30);
    CHECK(c.current_limit==g_motor_tuning.small.current_limit && c.current_limit>5000);
    c.target_rpm=570;c.measured_rpm= -2000;c.filtered_rpm= -2000;c.integral=0;
    MotorControl_Step(&c);CHECK(c.command_current==(int16_t)c.current_limit && c.integral==0);
    c.integral=3000;c.measured_rpm=650;c.filtered_rpm=650;
    MotorControl_Step(&c);CHECK(c.integral<3000);
    MotorControl_Stop(&c,MOTOR_REASON_KEY);
    CHECK(c.feedforward==0 && c.load_estimate==0 && c.integral==0);
    g_motor_tuning.small.observer_gain=1;g_motor_tuning.small.inertia_raw_per_rpm_s=5;
    core_key(&c,0,100);core_key(&c,MOTOR_KEY_SMALL,130);
    c.target_rpm=570;c.filtered_rpm=570;c.observer_rpm=570;c.measured_rpm=570;c.measured_current=2200;
    for(int i=0;i<2000;i++) MotorControl_Step(&c);
    CHECK(fabsf(c.load_estimate-2200)<1 && fabsf(c.feedforward-2200)<1);
    MotorControl_Stop(&c,MOTOR_REASON_KEY);g_motor_tuning.small.current_limit=NAN;
    core_key(&c,0,200);core_key(&c,MOTOR_KEY_SMALL,230);
    CHECK(c.mode==MOTOR_STOP && c.reason==MOTOR_REASON_CONFIG);
    g_motor_tuning.small.current_limit=MOTOR_CURRENT_LIMIT+1;
    core_key(&c,0,260);core_key(&c,MOTOR_KEY_SMALL,290);
    CHECK(c.mode==MOTOR_STOP && c.reason==MOTOR_REASON_CONFIG);
    g_motor_tuning=saved;c.temperature=70;
    core_key(&c,0,300);core_key(&c,MOTOR_KEY_SMALL,330);
    CHECK(c.mode==MOTOR_STOP && c.reason==MOTOR_REASON_TEMPERATURE);
    c.temperature=25;core_key(&c,0,400);core_key(&c,MOTOR_KEY_SMALL,430);
    MotorControl_Feedback(&c,2300,460);MotorControl_Poll(&c,0,460,0);
    CHECK(c.mode==MOTOR_STOP && c.reason==MOTOR_REASON_OVERSPEED);
    MotorControl_Init(&c,0);core_key(&c,0,0);core_key(&c,MOTOR_KEY_BIG,30);
    for(unsigned i=0;i<1000;i++) {
        MotorControl_Feedback(&c,500,now);MotorControl_StepElapsed(&c,(i%2)?2:1);
    }
    CHECK(c.phase_elapsed_ms==1500 && fabsf(c.phase_s-1.5f)<0.00001f);
    float analytic=(c.big_a*sinf(c.big_w*1.5f)+c.big_b)*60/(2*3.1415926f)*57;
    CHECK(fabsf(c.target_rpm-analytic)<0.001f);
    MotorControl_StepElapsed(&c,21);CHECK(c.mode==MOTOR_STOP && c.command_current==0);
    return 0;
}
static void uart_message(const MlMessage *m) {
    uint8_t bytes[ML_FRAME_SIZE];assert(ml_encode(m,bytes)==ML_FRAME_SIZE);
    for(unsigned i=0;i<ML_FRAME_SIZE;i++) {*uart_rx=bytes[i];HAL_UART_RxCpltCallback(&huart_motor);}
}
static void uart_start(void) {
    advance(600,0,1);
    MlMessage m=last_uart_status;m.type=ML_HELLO;m.session=0x123456;
    uart_message(&m);advance(1,0,1);
    m.type=ML_HEARTBEAT;m.seq=1;m.command=last_uart_status.seq;
    uart_message(&m);advance(100,0,1);
    m.type=ML_SET;m.seq=1;m.mode=MOTOR_BIG;uart_message(&m);advance(1,0,1);
}
static int uart_cases(void) {
    pressed=0;CHECK(MotorBoard_Init()==HAL_OK);uart_start();
    CHECK(g_motor_debug.control.mode==MOTOR_BIG && g_motor_debug.remote.owner==ML_REMOTE);
    MlMessage old=last_uart_status;old.type=ML_SET;old.seq=1;old.mode=MOTOR_BIG;
    uint32_t transitions=g_motor_debug.control.transitions;
    uart_message(&old);advance(1,0,1);CHECK(g_motor_debug.control.transitions==transitions);
    advance(501,0,1);CHECK(g_motor_debug.control.mode==0 && last_current()==0);
    uart_message(&old);advance(1,0,1);CHECK(g_motor_debug.control.mode==0);
    uart_start();CHECK(g_motor_debug.control.mode==MOTOR_BIG);
    advance(30,MOTOR_KEY_SMALL,1);CHECK(g_motor_debug.control.mode==MOTOR_SMALL && g_motor_debug.remote.owner==ML_LOCAL);
    advance(800,0,1);CHECK(g_motor_debug.control.mode==MOTOR_SMALL);
    advance(30,MOTOR_KEY_BOTH,1);CHECK(g_motor_debug.control.mode==0);
    advance(30,0,1);uart_start();CHECK(g_motor_debug.control.mode==MOTOR_BIG);
    HAL_UART_ErrorCallback(&huart_motor);advance(1,0,1);CHECK(g_motor_debug.control.mode==0 && last_current()==0);
    uart_start();CHECK(g_motor_debug.control.mode==MOTOR_BIG);
    old=last_uart_status;old.type=ML_SET;old.mode=MOTOR_BIG;old.seq=2;
    for(unsigned i=0;i<4;i++) uart_message(&old);
    advance(1,0,1);CHECK(g_motor_debug.control.mode==0 && g_motor_uart_debug.overflow>0);
    advance(600,0,1);old=last_uart_status;old.type=ML_HELLO;old.session=42;
    uart_message(&old);now+=101;MotorBoard_Update();CHECK(g_motor_debug.remote.session==0 && g_motor_uart_debug.stale>0);
    advance(30,0,1);advance(30,MOTOR_KEY_BIG,1);CHECK(g_motor_debug.control.mode==MOTOR_BIG);
    uint32_t previous_aborts=aborts;now+=25;receive(0x201,8,100);MotorBoard_Update();
    CHECK(g_motor_debug.control.mode==0 && g_motor_debug.control.reason==MOTOR_REASON_CONFIG && last_current()==0 && aborts>previous_aborts);
    for(unsigned fault=0;fault<2;fault++) {
        uart_hw_fail=fault==0;uart_arm_fail=fault==1;
        CHECK(MotorBoard_Init()==HAL_OK);advance(40,0,1);
        CHECK(!g_motor_debug.remote_ready && g_motor_debug.control.mode==0 && last_current()==0);
        advance(30,MOTOR_KEY_BIG,1);CHECK(g_motor_debug.control.mode==MOTOR_BIG);
        advance(30,MOTOR_KEY_BOTH,1);CHECK(g_motor_debug.control.mode==0);
    }
    uart_hw_fail=uart_arm_fail=0;
    return 0;
}
int main(void) {
    if(compare_legacy() || core_cases() || tuned_cases() || board_cases() || uart_cases()) return 1;
    printf("PASS: %lu checks; legacy speed targets, tuned control, buttons, CAN, real UART callbacks/queue, screen heartbeat timeout, stale/overflow/error, lease and fault recovery\n",(unsigned long)checks);
    return 0;
}
