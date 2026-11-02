#ifndef SLAVE_KEY_TEST_H
#define SLAVE_KEY_TEST_H

#include "main.h"

#define SLAVE_KEY_TEST_KEY_COUNT 9U
#define SLAVE_KEY_TEST_DEBOUNCE_MS 10U
#define SLAVE_KEY_TEST_HIGHLIGHT_MS 300U

/* 调试快照：bit0～bit8 对应按键1～9，计数只在消抖后的边沿更新。 */
typedef struct
{
    uint32_t enabled;
    uint32_t raw_mask;
    uint32_t stable_mask;
    uint32_t last_key;
    uint32_t last_press_ms;
    uint32_t scan_count;
    uint32_t press_count;
    uint32_t release_count;
    uint32_t key_press_count[SLAVE_KEY_TEST_KEY_COUNT];
    uint32_t key_release_count[SLAVE_KEY_TEST_KEY_COUNT];
    uint32_t shown_mask;
    uint32_t frame_count;
    uint32_t last_frame_ms;
} SlaveKeyTestDebug;

extern volatile SlaveKeyTestDebug g_slave_key_test_debug;

/* 主循环初始化：六路低亮红色；启动按键扫描，不初始化 CAN。 */
HAL_StatusTypeDef SlaveKeyTest_Init(void);
/* 仅从主循环调用：按键对应环区显示绿色至少300ms，随后恢复红色。 */
void SlaveKeyTest_Update(void);
/* 仅从1ms SysTick调用：独立扫描九键，禁止在中断中发送灯带数据。 */
void SlaveKeyTest_1msTick(void);

#endif
