# slave6：独立上电全灯常亮测试

2026-11-01 新增，用于承接先前 slave1 的临时常亮测试。slave1 已恢复原正常 CAN/击打控制版。本轮只修改源码、构建与主机侧配置核验，**未烧录任何板卡**。

## 上电行为

- 适用于 STM32F103C8T6、8 MHz 外部晶振；PLL ×9，系统 72 MHz。
- 六路共 865 颗灯珠全部低亮度红色常亮，`RGB=(32,0,0)`，约每 500 ms 重发同一幅数据。
- 包括箭头 DIN3 的全部 280 颗灯，不使用正常模式的图案/流水绘制。
- 不初始化 CAN、不读取击打按键；CAN 或按键不会改变显示。PA12 保持高电平，让 CAN 收发器 TXD 保持隐性输出。
- 灯带正常供电，并与 F1 共地。重发用于兼容灯带晚于 F1 上电的情况。

这是**独立测试工程名称**，没有新增第六个 CAN 比赛节点。原 `ENERGY_SLAVE_COUNT=5` 和五板协议不变，屏幕/C 板没有新增第六靶位。

| 灯带 | 引脚 | 灯珠数 |
|---|---|---:|
| DIN0 | PA1 / TIM2_CH2 | 271 |
| DIN1 | PA6 / TIM3_CH1 | 92 |
| DIN2 | PB0 / TIM3_CH3 | 92 |
| DIN3：箭头/流水 | PA0 / TIM2_CH1 | 280 |
| DIN4 | PA2 / TIM2_CH3 | 60 |
| DIN5 | PA3 / TIM2_CH4 | 70 |

## CLion 构建与下载

在所在分支的仓库根目录打开统一工程，Reload CMake 后选择：

- CMake 配置：`STM32-slave6-Debug`。
- 编译目标：`EnergyFirmware_slave6_Debug`。
- 下载配置：`Flash slave6-Debug`。
- 输出：`build/slave6-Debug/EnergyFirmware.elf/.hex/.bin`。

Release 对应 `STM32-slave6-Release` 和 `Flash slave6-Release`，默认未启用。

```powershell
.\tools\build-stm32.ps1 -Board slave6 -Configuration Debug
# 仅检查固件/板型匹配，不访问 USB、复位或烧录：
.\tools\flash\flash.ps1 -BuildDir build/slave6-Debug -ExpectedBoard slave6 -ExpectedConfiguration Debug -PlanOnly
```

`fu_clion` 工作树也已加入同样的目标。不要将 slave6 的构建目录配到 slave1 的下载入口，入口会拒绝该身份组合。

## 观察变量

- `g_slave_lamp_test_debug.enabled`：固定为 1，表示这份程序是常亮测试版；修改此快照不控制程序。
- `g_slave_lamp_test_debug.frame_count`：完整调用灯带发送函数的次数，运行时周期增长，不代表物理灯珠已确认点亮。
- `g_slave_lamp_test_debug.last_frame_ms`：最近一次发送函数结束时的毫秒时基。
- `LED_Data[0].R`、`LED_Data[455].R`、`LED_Data[734].R`、`LED_Data[795].R`：测试时均为 32，G/B 为 0。455～734 是整条 DIN3。

本工程没有 CAN 接收状态变量，不能通过 CAN 的 active 判断灯板测试效果。

## 软件核验

两分支的 Debug、Release 均构建通过。Debug Flash 13,108 B、RAM 18,296 B；Release Flash 7,476 B、RAM 18,224 B。两分支同配置的 bin 一致。

slave1 恢复后，两分支正常 bin 分别与临时常亮改动前逐字节一致；slave2～slave5 的时钟、CAN、定时器及灯效源文件指纹未变，slave2 保留已配置的 12 MHz。

主机配置核验通过：新增 CMake Preset、CLion Profile、Flash 配置及脚本参数支持；`-PlanOnly` 接受 slave6，错误地以 slave1 选择 slave6 构建则提前拒绝。构建日志和核验清单位于 `build/slave6-lamp-test-20261101/`。实机常亮效果待用户自行烧录后确认。

本工程保留独立 Core/APP/BSP，仅复用 `../fu_slave1/Drivers` 的 ST HAL/CMSIS。没有复用 slave1 的业务程序，也未新增 CubeMX 工程；本目标的时钟、初始化和中断以本目录源码为准。
