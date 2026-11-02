# slave7：独立全灯与九键检测测试

这是在 slave6 全灯测试基础上新增的独立工程，适用于 **STM32F103C8T6、8 MHz 外部晶振、72 MHz 系统时钟**。不初始化 CAN，不需要 C 板，不增加比赛节点。slave1～slave5 的正常业务和 slave6 常亮版不变。

## 灯光与按键

- 上电六路共 865 颗灯全部低亮红色 `RGB=(32,0,0)`，包括 DIN3 箭头整路。
- 九键均为上拉输入、**低电平有效**，按下时接 GND，沿用原灯板接线。
- 按键稳定 10 ms 后，对应 DIN0 环区变为绿色 `RGB=(0,32,0)`。短按绿色至少保留到本次按下后的 300 ms；长按持续绿色，松开后恢复红色。其他灯珠保持红色全亮。
- 九键独立消抖，多键可以同时被检测并变绿；长按只增加一次按下计数，稳定松开后再次按下才再次计数。
- SysTick 每 1 ms 扫描一次，灯带 DMA 发送和主循环等待期间仍扫描；中断中不发送灯带。小于消抖窗口的脉冲不算有效击打。
- PA12 保持高电平，让未初始化的 CAN 收发器 TXD 保持隐性输出。接上 CAN 也不改变测试状态。
- 约每 500 ms 重发当前画面，兼容灯带稍晚于 F1 上电。

环区索引沿用正常程序，暂不用于确认实物丝印的环数；请逐键核对实际接线。

| 程序键号 | 输入引脚 | 掩码 | DIN0 灯珠索引（从0开始） |
|---|---|---|---|
| 1 | PB10 | `0x001` | 1～47 |
| 2 | PB11 | `0x002` | 48～89 |
| 3 | PB12 | `0x004` | 90～133 |
| 4 | PB13 | `0x008` | 134～173 |
| 5 | PB14 | `0x010` | 174～209 |
| 6 | PB15 | `0x020` | 210～229 |
| 7 | PA4 | `0x040` | 230～249 |
| 8 | PB6 | `0x080` | 250～261 |
| 9 | PB7 | `0x100` | 262～270 |

## CLion 构建和下载

两个分支都在原统一工程中加入以下入口。Reload CMake 后选择：

- Profile：`STM32-slave7-Debug`。
- 编译目标：`EnergyFirmware_slave7_Debug`。
- 下载配置：**`Flash slave7-Debug`**，不要把 ARM ELF 当电脑程序直接运行。
- 固件：`build/slave7-Debug/EnergyFirmware.elf`，同目录生成 `.hex`、`.bin`。
- Release 对应 `STM32-slave7-Release` / `Flash slave7-Release`，默认未启用。

```powershell
.\tools\build-stm32.ps1 -Board slave7 -Configuration Debug
# 只检查板型/配置，不扫描USB、不访问硬件：
.\tools\flash\flash.ps1 -BuildDir build/slave7-Debug -ExpectedBoard slave7 -ExpectedConfiguration Debug -PlanOnly
# 无硬件的按键行为测试（需要主机gcc）：
python tools/test-slave7.py
```

## Debug Watch 变量

所有字段都在 `g_slave_key_test_debug` 中，均为观察快照，不是控制入口。复位后计数清零。

| 字段 | 含义 |
|---|---|
| `raw_mask` | 当前原始低电平输入；bit0～bit8 对应键1～9，便于检查接线和抖动 |
| `stable_mask` | 消抖后仍按住的键；松开后为0，多键为位或 |
| `last_key` | 最近一次有效按下的程序键号1～9；上电未按时为0，松开后保留；同一毫秒多键按下时保留较大键号 |
| `press_count` / `release_count` | 所有键累计有效按下/松开次数，长按不连续累加 |
| `key_press_count[0..8]` | 九个键各自的按下次数，数组下标0对应键1 |
| `key_release_count[0..8]` | 九个键各自的松开次数 |
| `last_press_ms` | 最近一次有效按下的毫秒时间 |
| `scan_count` | 1ms 扫描次数，CPU运行时增长 |
| `shown_mask` | 最近发送的绿色环区掩码，短按松开后可保留到300ms |
| `frame_count` / `last_frame_ms` | 已调用灯带发送函数的次数/最近发送结束时间，不代表灯珠物理反馈 |

例如按 PB10：`raw_mask` 先变 `0x001`，稳定后 `stable_mask=0x001`、`last_key=1`，`key_press_count[0]` 加1。松开稳定后 `stable_mask=0`，松开计数加1。无需查看或修改 `g_can_slave_debug.active`。

## 软件验证（2026-11-01）

两个分支的 Debug、Release 均构建通过，同配置 bin 逐字节相同。Debug Flash 14,100 B、RAM 18,472 B；Release Flash 8,116 B、RAM 18,392 B（RAM 包含链接脚本预留堆栈）。

主机测试编译真实按键模块，覆盖九键接线映射、865颗灯珠画面、输入抖动/毛刺、长按与重按、九键同时按下、上电按住、灯带发送期间短按、300ms提示、毫秒回绕及定期刷新。两个分支均通过。现有 slave1～slave6 源文件与改动前指纹一致；原共享 Profile 保留，生成器在隔离目录生成18组入口；下载 `-PlanOnly` 身份检查通过。

本轮只交付源码、构建及主机测试结果，**未烧录任何板卡**。实际按键、灯色及环区位置需用户自行下载后确认。完整核验记录见仓库 `build/slave7-key-test-20261101/`。

与 slave6 一样，本工程独立维护 Core/APP/BSP，仅复用 `../fu_slave1/Drivers` 中的 ST HAL/CMSIS，不复用 slave1 业务程序，没有单独 CubeMX `.ioc`。
