# ESP32-C3 代替 HC-05，联动旋转电机 F1

本轮按用户要求先完成代码与软件验证，实物稍后接线。**本轮未烧录新固件、未启动电机。** 旧 HC-05 代码仍留在忽略目录备份中，没有引入蓝牙库。

## 控制链路

```text
                    ┌─ ESP-NOW ─ 原 XIAO ESP32-S3 ─ UART ─ C 板 ─ CAN ─ 五块灯板
NiceMCU 屏幕 ESP32 ─┤
                    └─ ESP-NOW ─ 新 ESP32-C3 ─ UART ─ 独立电机 F1 ─ CAN ─ C620/M3508
```

这块新 C3 是专用电机接收端，不替换原 C 板接收端。沿用用户确认的“灯板与电机一起联动”：两条链路都就绪才允许开始；先由 C 板 ACK + STATUS 确认，再启动电机。不是同步硬件触发，会有通信延迟。

本轮从 USB 实读新设备：COM45，ESP32-C3 QFN32 rev0.4，内置 4 MB Flash，Wi-Fi STA MAC `9C:96:D5:B1:65:EC`。屏幕此前实读 MAC 为 `70:4B:CA:2F:02:50`，原灯板接收端配置仍为 `7C:E8:B1:A1:47:08`。

## 接线

先断电接线，两个板各自正常供电，只连串口和地：

| ESP32-C3 | 独立电机 F1 | 用途 |
|---|---|---|
| GPIO21 / TX | PA10 / USART1_RX | C3 发，F1 收 |
| GPIO20 / RX | PA9 / USART1_TX | F1 发，C3 收 |
| GND | GND | 共地 |

串口 **115200、8N1、3.3 V 逻辑**。不连接电机的 24 V 电源到 C3；没有 HC-05 的 EN/KEY/STATE 接线，F1 PA8 不需要连接。C3 的 GPIO18/19 留给原生 USB。GPIO20/21 功能可见 [Espressif C3 引脚表](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c3/esp32-c3-devkitm-1/user_guide.html)。

F1 原有 PB10/PB11 两键、PA11/PA12 电机 CAN 和 PA13/PA14 SWD 保留。**电机 CAN 仍与灯板 CAN 分开**，两套总线的 0x201 含义不同。

## 操作

- 上电停止，屏幕默认待选小符。选模式不立即转动，按“开始/重新开始”才联动。
- 新的屏幕 START 重新开始电机曲线；相同编号的重试不重复清理相位。开始最多重试 3 次，间隔 100 ms；未确认则停止，不能靠“发送成功”判定执行成功。
- PB10：大符正弦变速；PB11：小符恒速；双键：停止。按键上拉低有效、20 ms 消抖，上电按住不启动；同模式再次按键保持相位。原速度、PID、电流上限、补偿参数均未改变。
- 有效本地按键操作接管电机，不会被周期心跳改回。按键不改变 C 板的模式；本地运行不依赖屏幕或 C 板低压状态。双键始终可以停止。
- 在双向链路就绪时，屏幕新的 START/STOP 可重新接管。失联时不能用屏幕保证停止本地按键运行，应使用 F1 双键。
- 对由屏幕管理的运行，屏幕停止、C 板停止/低压/失联、电机端拒绝/故障/失联都会请求停止联动；两板恢复通信后等待新的手动开始。
- 停机是零电流自由滑行，不是位置保持。原 CAN 100 ms 超时、温度与超速保护继续生效。

## 通信与重启

固定信道 6、固定 MAC 单播、无路由器。屏幕的灯板/电机接收队列分开，共用一次 Wi-Fi 和 ESP-NOW 初始化及回调，互不覆盖。回调只复制有界数据，超过 100 ms 的排队包丢弃。

F1 UART 使用独立 40 字节协议，显式小端字段与 CRC16，含 F1 启动身份、会话代次、屏幕会话、命令/心跳/状态序号、模式、来源、RPM。原灯板 CAN 和 C 板 UART 协议保持不变。

无线外层为 16/56 字节的 beacon/data 帧，另外携带 **C3 启动标识**、序号、长度及 CRC16；C3 只转发属于本次启动的包，重启前的 START 不会恢复补发。ESP-NOW 发送队列接收成功仅表示 SDK 接收，不代表 F1 或电机物理执行。

屏幕心跳/状态周期 100 ms，F1 采用 500 ms 新鲜双向租约；重复心跳、重复状态回声和 C3 自身 beacon 不续 F1 租约。C3 不生成遥控心跳，不缓存 START 等待恢复。F1 上电前 500 ms 丢弃串口输入并等待握手；F103 的启动身份使用内部 ADC/时序扰动，仅作重启区分，不是密码学认证。

## 配对与构建

下列 ESP32 构建命令在主工作区 `feature/esp32-screen-remote` 执行；`fu_clion` 仅执行电机 F1 构建命令，不包含 ESP32 源码或 ESP32 构建脚本。

已将实读 MAC 写入 Git 忽略的 `remote/common/MotorNow/src/motor_pairing.local.h`。屏幕与 C3 共用该文件；新设备需根据真实 Wi-Fi MAC 修改，模板为同目录 `motor_pairing.local.example.h`。未配置/自身 MAC 不匹配则禁用电机联动，不搜索同名设备。

```powershell
# 新屏幕联动程序（默认环境）
.\tools\build-esp32.ps1 -Target screen -ScreenEnvironment nicemcu_motor_now
# 新 ESP32-C3 电机接收端
.\tools\build-esp32.ps1 -Target motor_receiver
# 电机 F1（在当前分支或 fu_clion 各自根目录执行）
.\tools\build-stm32.ps1 -Board motor -Configuration Debug
# 保留的原纯灯板屏幕配置
.\tools\build-esp32.ps1 -Target screen -ScreenEnvironment nicemcu
```

| 需更新的固件 | 输出 |
|---|---|
| 屏幕 ESP32 | `remote/screen/.pio/build/nicemcu_motor_now/firmware.bin/.elf` |
| 电机接收 C3 | `remote/motor_receiver/.pio/build/motor_esp32c3/firmware.bin/.elf` |
| 电机 F1 | `build/motor-Debug/EnergyFirmware.elf/.hex/.bin`；CLion 仍选 motor-Debug |

接线后需更新以上三处。C 板、原 XIAO ESP32-S3、五块灯板无需因本功能更新。**本次屏幕仍为原 1.25 MiB 应用分区，不使用原 HC-05 版 3 MiB 分区。**

ESP32-C3 首次下载完整镜像时，其 bootloader 地址为 **0x0000**；普通 ESP32 屏幕为 **0x1000**；各自 partitions 为 0x8000、boot_app0 为 0xe000、app 为 0x10000。镜像必须来自对应项目，不能互换。后续烧录前仍需核对设备身份和现有分区。

F1 串口/协议及测试同步到 `fu_clion`，ESP32 源码不复制到该分支。F1 调试时使用与实际板上固件匹配的 ELF：`g_motor_debug.remote` 可查看会话/来源/接受拒绝/去重，`g_motor_uart_debug` 可查看收发、错误、溢出和过期字节；`g_motor_request` 仍是需单独提交序号的一次性调试入口。

## UI 和验收

保留 596b900 的布局、配色、字号、导航与按钮位置，仅在控制页及设备页原有空白显示电机来源、实际电机侧 RPM、等待确认、接收端离线或 F1 链路未就绪。没有轴角度测量，不绘制假旋转姿态。原 UI 版本的实体触摸命中不由本次离屏渲染证明。

软件验证和固件摘要见 [最终软件验证记录](MOTOR_ESP_NOW_FINAL_SOFTWARE_20261031.md)，当前预览见 [motor-now-ui](E:/BaiduSyncdisk/dafu_finishi/26能量机关/remote/docs/motor-now-ui/README.md)。实际接线、三处固件下载、两条无线链路、按键/触摸、故障注入和实际转速仍待后续实物联调。
