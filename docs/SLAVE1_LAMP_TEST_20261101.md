# slave1 临时上电常亮测试版（8 MHz）

**已迁移：** 按后续要求，slave1 已恢复原正常 CAN 控制程序，常亮测试改为独立 `slave6` 目标。现在的 `build/slave1-Debug/EnergyFirmware.elf` 是正常版；如需常亮测试，应选择 `STM32-slave6-Debug` 和 `Flash slave6-Debug`，参见 [slave6 说明](../wanwanwanfufu/dbus_switch_energy/fu_slave6/README.md)。下文保留为前一阶段的历史记录；原临时测试镜像仍冻结在 `build/slave1-lamp-test-20261101/firmware-8mhz/`。

按用户 2026-11-01 的要求，slave1 临时改为独立灯板测试，用户自行烧录。本轮未访问探针、未复位板子、未烧录固件。

## 测试行为

- 适用硬件：用户确认的 **8 MHz 晶振正常板**；原 ×9 倍频、72 MHz 系统时钟和灯带时序保持不变。slave2 的 12 MHz 配置不受影响。
- 上电进入专用测试路径，只初始化 GPIO、DMA、TIM2/TIM3，然后六路原灯带全部显示红色 `RGB=(32,0,0)`。这是低亮度常亮，不是比赛图案或流水灯。
- 约每 500 ms 重发同一幅画面；刷新不会发送熄灯帧，灯带晚于 F1 上电也能在后续刷新时显示。
- 不初始化 CAN、击打按键定时器和调试 UART，不需要 C 板在线；按键和 CAN 指令不会改变测试画面。PA12/CAN_TX 保持高电平，使收发器处于隐性输出状态。
- 灯带仍需正常供电并与 F1 共地，不能用 MCU 已运行来推断灯带电源已到位。

原六路映射和灯珠数保持：

| 灯带 | 信号引脚 | 灯珠数 |
|---|---|---:|
| DIN0 | PA1 / TIM2_CH2 | 271 |
| DIN1 | PA6 / TIM3_CH1 | 92 |
| DIN2 | PB0 / TIM3_CH3 | 92 |
| DIN3（箭头/流水） | PA0 / TIM2_CH1 | 280 |
| DIN4 | PA2 / TIM2_CH3 | 60 |
| DIN5 | PA3 / TIM2_CH4 | 70 |

箭头一路的整体数据范围为 `LED_Data[455]` 至 `LED_Data[734]`。本测试版已将这 280 颗全部设为红色 32，不调用正常模式的箭头流水绘制函数。

## 固件与观察变量

两个分支均构建通过，测试 bin 相同，长度 18,192 B，SHA256：`ced5a391078549e9e36b20b01fde34986eca7bb80c55f982610b19b76a61b8f2`。

CLion 仍选 `STM32-slave1-Debug` / `EnergyFirmware_slave1_Debug`。当前输出位于 `.worktrees/fu_clion/build/slave1-Debug/EnergyFirmware.elf/.hex/.bin`。独立冻结副本位于 `build/slave1-lamp-test-20261101/firmware-8mhz/`，名字明确标为测试版。

可用 Watch：

- `g_slave_lamp_test_debug.enabled`：测试版为 1，修改它不切换运行模式。
- `g_slave_lamp_test_debug.frame_count`：灯带发送函数完成调用的次数，周期增长；不代表物理灯珠已确认点亮。
- `g_slave_lamp_test_debug.last_frame_ms`：最近一轮发送函数结束时的本板毫秒时基。
- `LED_Data[0].R`、`LED_Data[795].R`：测试时都为 32，所有灯珠 G/B 为 0。

测试模式中 CAN 的 active/rx_count 不用于判断点亮，正常机关控制暂不参与。

## 恢复正常控制

在当前分支的 `fu_slave1/user/APP/energy_slave_config.h` 中，把 `ENERGY_SLAVE_BOOT_LAMP_TEST` 从 **1 改为 0**，重新构建并下载即可恢复原 CAN/击打逻辑。两个分支都有各自的该文件，已同步测试入口；没有把另一个分支的 CAN 协议代码互相覆盖。

软件验证：两分支测试版 Debug 构建通过；测试 ELF 不包含 `CanSlave_Init`、`MX_CAN_Init`、`MX_TIM4_Init` 或击打读取函数。用独立构建目录关闭开关后，两分支的正常模式都能构建，生成 bin 分别与修改前正常固件逐字节完全一致。原源码与固件备份、构建日志、核验 JSON 位于 `build/slave1-lamp-test-20261101/`。真实上电显示效果待用户烧录后确认。
