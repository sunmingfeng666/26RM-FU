# 能量机关灯板控制工程

本仓库保存能量机关相关的 STM32 控制程序，主要包含两类工程：

- `DaFu_fa`：基于 STM32F407 的主控工程，通过 CAN2 向各灯板下发点亮/熄灭指令。
- `WS2812_F1` 系列目录：基于 STM32F103C8T6 的灯板工程，通过 TIM + DMA 驱动 WS2812 灯带，并通过 CAN 接收主控指令或上报击打反馈。

工程由 STM32CubeMX 生成，使用 Keil MDK-ARM 打开和编译。

## 目录结构

```text
.
├── DaFu_fa/              # STM32F407 主控 CAN 工程
├── true_dafu/            # 大符相关整合/调试版本
├── true_wan_xiaofu/      # 小符/完整版本相关工程
├── 全全红/               # WS2812 全红效果工程
├── 全红/                 # WS2812 全红效果工程
├── 完整选中待击打/       # 选中 + 待击打灯效工程
├── 已选中/               # 已选中状态灯效工程
├── 待击打/               # 待击打状态灯效工程
├── 按键/                 # 按键测试/联调工程
├── 按键2/                # 按键测试/联调工程
├── 灯臂全红/             # 灯臂全红效果工程
├── 灯臂流水/             # 灯臂流水效果工程
└── 引脚.txt              # 按键与灯带信号引脚记录
```

每个 STM32 工程通常包含：

- `*.ioc`：STM32CubeMX 配置文件
- `Core/`：CubeMX 生成的 HAL 初始化代码
- `Drivers/`：CMSIS 与 HAL 驱动
- `MDK-ARM/*.uvprojx`：Keil 工程文件
- `MDK-ARM/*.c, *.h`：用户任务、CAN、WS2812 等业务代码

## 硬件平台

### 主控工程 `DaFu_fa`

- MCU：STM32F407IGHx
- 工具链：MDK-ARM V5.32
- 主要外设：
  - CAN2，波特率 1 Mbps
  - USB OTG FS
  - SWD 调试接口
- CAN2 引脚：
  - `PB5`：CAN2_RX
  - `PB6`：CAN2_TX

### 灯板工程 `WS2812_F1`

- MCU：STM32F103C8Tx
- 主要外设：
  - CAN
  - TIM2 / TIM3 PWM
  - DMA1
  - GPIO 按键输入
- WS2812 通过定时器 PWM + DMA 发送 GRB 数据。

## 引脚记录

按键引脚：

| 引脚 | 功能 |
| --- | --- |
| PB10 | key1 |
| PB11 | key2 |
| PB12 | key3 |
| PB13 | key4 |
| PB14 | key5 |
| PB15 | key6 |
| PA4 | key7 |
| PB6 | key8 |
| PB7 | key9 |

灯带/信号引脚：

| 引脚 | 功能 |
| --- | --- |
| PA1 | din0 |
| PA6 | din1 |
| PB0 | din2 |
| PA0 | din3 |
| PA2 | din4 |
| PA3 | R |

> 具体引脚功能以各目录下的 `*.ioc` 配置和实际硬件接线为准。

## CAN 通信

主控通过 CAN 向灯板发送 8 字节标准帧。当前代码中使用的灯板控制 ID 规则如下：

| 目标 | 点亮 ID | 熄灭 ID |
| --- | --- | --- |
| 1 | `0x201` | `0x202` |
| 2 | `0x211` | `0x212` |
| 3 | `0x221` | `0x222` |
| 4 | `0x231` | `0x232` |
| 5 | `0x241` | `0x242` |

灯板击打反馈 ID：

| ID | 说明 |
| --- | --- |
| `0x341` | 检测到按键/击打后向主控发送反馈 |

主控侧相关代码：

- `DaFu_fa/MDK-ARM/can_task.c`
- `DaFu_fa/MDK-ARM/tim_task.c`

灯板侧相关代码：

- `*/MDK-ARM/can_task.c`
- `*/MDK-ARM/WS2812.c`
- `*/MDK-ARM/led_rings.c`

## 主要功能

- CAN 过滤器初始化与标准帧收发
- 主控随机选择未击打目标
- 超时或击打后关闭当前目标灯板
- STM32F103 使用 TIM + DMA 驱动多路 WS2812 灯带
- 灯板按键检测与击打反馈
- 多个灯效/状态工程版本，便于单独烧录测试

## 开发环境

建议使用以下环境：

- STM32CubeMX
- Keil MDK-ARM 5.x
- STM32Cube FW_F4 V1.28.3
- STM32Cube FW_F1 对应版本
- ST-Link 或其他支持 SWD 的下载器

## 编译与烧录

1. 用 STM32CubeMX 打开对应目录下的 `*.ioc`，确认芯片型号、时钟、CAN、TIM、DMA、GPIO 配置。
2. 用 Keil 打开对应目录下的 `MDK-ARM/*.uvprojx`。
3. 编译工程，确认无错误。
4. 连接 ST-Link，通过 SWD 将程序烧录到对应控制板。
5. 主控与灯板接入同一 CAN 总线，确认 CAN 收发器供电、CANH/CANL 和终端电阻正确。

常用工程入口：

| 用途 | Keil 工程 |
| --- | --- |
| 主控 CAN 控制 | `DaFu_fa/MDK-ARM/DaFu.uvprojx` |
| 单灯板/灯效测试 | `全红/MDK-ARM/WS2812_F1.uvprojx` |
| 灯臂全红 | `灯臂全红/MDK-ARM/WS2812_F1.uvprojx` |
| 灯臂流水 | `灯臂流水/MDK-ARM/WS2812_F1.uvprojx` |

## 调试注意事项

- CAN 波特率需要主控和所有灯板保持一致。
- 若灯板不响应，先检查 CAN ID 是否与当前烧录工程匹配。
- WS2812 对时序敏感，修改 TIM 频率、ARR、CCR 或 DMA 配置后需要重新用示波器确认波形。
- 按键默认按下为低电平，硬件上需要确认上拉/下拉配置。
- 仓库中包含部分 `build/`、`.o`、`.hex`、`.axf` 等编译产物；正式开源前建议整理 `.gitignore`，只保留源码、工程文件和必要说明。
- 部分旧文件注释可能存在编码乱码，不影响编译，但建议后续统一为 UTF-8。

## 许可

本仓库包含 STMicroelectronics 生成的 HAL/CMSIS 代码及用户代码。ST 官方代码遵循其随包许可证；用户代码如需开源发布，建议补充明确的 LICENSE 文件。
