# 独立 F1 旋转电机板

2026-10-31：新增 ESP32-C3 经 ESP-NOW / UART 联动，替代先前未接入的 HC-05；PB10/PB11 实体按键、原速度、PID 和补偿参数保留。新联动程序本轮先完成代码，尚未烧录或实物验收，详见仓库根目录 `docs/MOTOR_ESP_NOW_20261031.md`。

此固件烧录到专门控制旋转电机的 STM32F103C8T6（64 KB Flash、20 KB RAM、外部 8 MHz 晶振）。通过 PB10/PB11 两个本地按键选择大小符，C 板和五块灯板使用各自的固件。

## 接线与操作

| 引脚/接口 | 用途 |
|---|---|
| PB10 → 按键 → GND | 大符正弦变速 |
| PB11 → 按键 → GND | 小符恒速 |
| PA11 | CAN RX，连接 CAN 收发器 RXD |
| PA12 | CAN TX，连接 CAN 收发器 TXD |
| CANH / CANL / GND | 经收发器连接 M3508 的 C620 电调，1 Mbps，电机 ID=1 |
| PA13 / PA14 / GND | SWD 下载与调试 |
| PA10 / USART1_RX | 接 ESP32-C3 GPIO21/TX |
| PA9 / USART1_TX | 接 ESP32-C3 GPIO20/RX，115200 8N1 |

两个输入使用内部上拉，按下为低电平，消抖 20 ms。上电停止，即使上电时按住按键也不会启动，先松开再按。PB10/PB11 单击选择模式并持续运行；按同一模式按钮保持当前过程；运行中可通过另一按钮切换模式。两键同时按下并稳定 20 ms 后停止；停止后必须先全部松开，再按单键才能重新运行。

**电机 CAN 总线与灯板 CAN 总线分开接线。** 电机反馈 ID `0x201` 与灯板点亮 ID 冲突。电机板发送 `0x200`，第 1 路为电流指令，第 2～4 路固定为 0；只接收标准数据帧 `0x201`、DLC=8 的电机反馈。

## 当前固化参数（2026-10-19）

用户要求先保存当前参数，继续调参暂止于此。参数初始化位于 `user/APP/motor_control.c` 的 `g_motor_tuning`；烧录后断电重启使用这些值。Watch 中临时编辑只保留在 RAM，新模式启动时锁存生效。

| 项目 | 大符 | 小符 |
|---|---|---|
| 控制更新 | 1 ms，按实际经过的毫秒数更新目标与积分 | 同左 |
| 速度目标 | 输出轴 `a*sin(w*t)+b` rad/s | 电机侧 570 RPM，折算机关约 10 RPM |
| 速度参数 | `a=0.780～1.045`，`w=1.884～2.000`，`b=2.090-a` | 500 RPM/s 斜坡，约 1.14 s 达到目标 |
| 总减速比 | 约 57（电机约 19 × 皮带约 3） | 同左 |
| Kp / Ki / Kd | 24 / 0.005 / 0 | 24 / 0.005 / 0 |
| 运行电流上限 | ±10000 raw | ±8000 raw |
| 加速度前馈系数 | 5 raw/(RPM/s) | 0.6 raw/(RPM/s) |
| 负载观测补偿增益 | 0（关闭） | 1（开启） |
| 观测器带宽 | 6 rad/s | 6 rad/s |
| 速度变化前馈 / 固定偏置 | 0 / 0 | 0 / 0 |
| 积分限幅 | ±4000 raw | ±4000 raw |

Ki 参数仍使用每 10 ms 的离散标度，实际每次计算乘以 `elapsed_ms/10`。C620 的协议满量程为 ±16384 raw（±20 A 转矩电流）；这里的总软件硬上限为 ±12000，运行时使用表中的模式上限。加入条件积分防饱和、三点速度中值滤波、负载观测和加速度前馈；原速度目标规则与方向保留。

大符新一轮重新生成正弦参数。相同模式再次按键不重启；模式切换清理积分、观测器和前馈输出。三点中值滤波约增加 1–2 个反馈周期延迟，原始速度及跳变三帧仍保留在 `can` 调试字段，持续超速仍触发保护。

这是一份阶段性参数保存：小符已有改善，大符尚未完成最终效果验收。此前触发超速的高出力/高增益试验组合未选为启动默认值。详细证据见仓库 `docs/MOTOR_PARAMETERS_20261019.md`。

## 反馈与停止

必须已有 100 ms 内的新鲜电机反馈才能接受启动按键；没有反馈时按键请求不会排队等待。运行时反馈超过 100 ms、CAN bus-off 或发送失败会进入停止，清理 PID 并输出零电流。反馈恢复后保持停止，需重新按键。模式变化/停止时取消旧的待发 CAN 报文，停止期间每 1 ms 继续发送零电流。温度达到 70℃、中值速度绝对值超过 2200 RPM、配置无效也会停止。STOP 为零电流自由滑行，不是位置保持；不均匀负载可能回摆。

此板保留按键，并可通过专用 ESP32-C3 接收屏幕命令。屏幕管理的运行受 500 ms 新鲜双向心跳保护，屏幕/C 板停止、低压或失联会停止该运行；通信恢复后等待新的手动开始。有效本地按键可接管电机，本地运行不依赖屏幕，不受 C 板低压/断链自动停止控制；通信不可用时用本板双键停止。UART/启动身份初始化失败只禁用远程控制，本地按键仍可使用。此前仅有速度调试，新增联动、实体按键、机械停止距离和长期温升仍需实物验收。

## CLion 构建与下载

在所在分支的仓库根目录打开 CLion 工程：

- CMake 配置：`STM32-motor-Debug`（Release 需在 CMake 设置中启用）。
- 目标：`EnergyFirmware_motor_Debug`。
- 下载配置：`Flash motor-Debug`，目标芯片 `STM32F103C8`。
- 输出：`build/motor-Debug/EnergyFirmware.elf/.hex/.bin`。

```powershell
cmake --preset motor-Debug
cmake --build --preset motor-Debug
# 或：
.\tools\build-stm32.ps1 -Board motor -Configuration Debug -Ninja 'D:\CLion 2026.1.3\bin\ninja\win\x64\ninja.exe'
# 仅核验所选固件，不扫描 USB 或下载：
.\tools\flash\flash.ps1 -BuildDir build/motor-Debug -ExpectedBoard motor -ExpectedConfiguration Debug -PlanOnly
```

`fu_motor` 复用已有 `fu_slave1/Drivers` 中的 ST HAL/CMSIS，编译清单不含灯板应用源码。外设初始化维护在本目录 `Core/Src`。未新增 CubeMX 工程，端口/时钟以这里的源文件为准。

## 调试与验证

`g_motor_debug` 是只读快照，修改它不会命令电机。常看字段：

- `control.mode`：0 停止、1 大符、2 小符。
- `control.reason`：0 上电、1 按键/调试停止、2 反馈失联、3 CAN 故障、4 配置/时序异常、5 温度、6 超速、7 调试心跳超时。
- 新增原因：8 远程租约/链路超时，9 屏幕停止，10 屏幕启动。
- `remote_ready`：UART/启动身份模块是否初始化成功；这不代表无线/F1 双向链路已在线。
- `remote.owner`：0 无、1 按键、2 屏幕、3 调试；`remote` 中还有会话、命令/心跳序号与接受/拒绝/去重计数。
- `g_motor_uart_debug`：串口收发、错误、过期与溢出计数。
- `control.keys_raw/keys_stable/keys_armed`：按键位掩码与是否已松开等待新按键。
- `control.target_rpm/measured_rpm/command_current`：电机侧目标/反馈 rpm、发送电流 raw。
- `control.big_a/big_w/big_b/phase_s/integral`：正弦及 PID 状态。
- `can.last_rx_ms/rx_count/tx_count/tx_error_count/error_count`：通信状态。

主机验证：`python tools/test-motor.py`，当前通过 32,099 项检查。旧源码用于速度目标回归；PID 已调整，不再宣称 PID 输出与旧版相同。测试还覆盖按键、CAN、UART 回调/队列、远程失联与按键接管、远程初始化失败后按键可用、超速停机、防积分饱和、补偿清零、非法参数、调试心跳与迟到请求、三点滤波、变步长目标计时，以及控制周期超限时取消旧的待发转矩帧。

`g_motor_request` 是独立的一次性调试入口：先写 `command`（1 大符、2 小符、3 停止）与 `heartbeat`，最后递增 `sequence`。调试启动后需要每 500 ms 内更新心跳，否则停机；旧序号不能重新启动。正常实体按键运行不依赖电脑心跳。修改只读快照 `g_motor_debug` 不产生动作。

软件与实物证据分别见 `docs/MOTOR_VALIDATION.md`、`docs/MOTOR_PARAMETERS_20261019.md`，历史调试详见 `docs/MOTOR_LIVE_DIAGNOSIS_20261019.md`。
