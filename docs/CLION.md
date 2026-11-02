# 统一 STM32 / CLion 工程

在 CLion 打开仓库根目录的 `CMakeLists.txt`。主控和五块灯板属于同一个工程，通过共享 CMake Profile 选择固件，不再分别打开六个目录。命令行仍使用同名 CMake Preset（不带 `STM32-` 前缀）。

| 配置 | 芯片 | 源码目录 | 下载目标 |
|---|---|---|---|
| master-Debug / master-Release | STM32F407IGH6 | `wanwanwanfufu/dbus_switch_energy/fu_master` | stm32f4x |
| slave1-Debug / slave1-Release | STM32F103C8T6 | `.../fu_slave1` | stm32f1x |
| slave2-Debug / slave2-Release | STM32F103C8T6 | `.../fu_slave2` | stm32f1x |
| slave3-Debug / slave3-Release | STM32F103C8T6 | `.../fu_slave3` | stm32f1x |
| slave4-Debug / slave4-Release | STM32F103C8T6 | `.../fu_slave4` | stm32f1x |
| slave5-Debug / slave5-Release | STM32F103C8T6 | `.../fu_slave5` | stm32f1x |
| slave6-Debug / slave6-Release | STM32F103C8T6 | `.../fu_slave6`，独立上电常亮测试，不参加 CAN 比赛 | stm32f1x |
| slave7-Debug / slave7-Release | STM32F103C8T6 | `.../fu_slave7`，独立全灯与九键检测测试 | stm32f1x |
| motor-Debug / motor-Release | STM32F103C8T6 | `.../fu_motor`，独立 PB10/PB11 电机板 | stm32f1x |

`ACTIVE_BOARD` 固定 MCU 和板型。不同板必须使用不同构建目录；在旧目录强行切换 MCU 会报错。五个灯板目录本身的编号定义保持原样。独立电机板使用 `motor`，操作说明见 [fu_motor/README.md](../wanwanwanfufu/dbus_switch_energy/fu_motor/README.md)。

## 在 CLion 中编译与下载

1. 配置 ARM GNU 工具链，让 `arm-none-eabi-gcc`、`arm-none-eabi-g++`、`arm-none-eabi-objcopy` 在 PATH 中；使用 Ninja。当前验证工具链为 ARM GNU 14.2.rel1、CMake 4.4、Ninja 1.13。
2. `.idea/cmake.xml` 已提供默认启用的九个 Debug 共享配置，名称为 `STM32-master-Debug`、`STM32-slave1-Debug` ～ `STM32-slave7-Debug`、`STM32-motor-Debug`。Release 配置默认禁用，需要时在 Settings → Build, Execution, Deployment → CMake 勾选。CLion 导入 Preset 默认禁用，因此不能仅依赖 `CMakePresets.json`；参见 [CLion 官方共享配置说明](https://www.jetbrains.com/help/clion/cmake-profile.html#share-profiles)。
3. 选择例如 `STM32-slave2-Debug`，对应可执行目标 `EnergyFirmware_slave2_Debug`。每个板型/构建类型使用唯一目标，CLion 因而只匹配支持这个目标的 Profile。对应文件仍位于 `build/slave2-Debug/EnergyFirmware.elf`，同目录有 `.bin`、`.hex`、`.map` 和 `firmware.json`。`EnergyFirmware` 保留为命令行聚合构建目标。
4. Run/Debug Configurations 中选择共享的 `Flash master-Debug`。运行会先编译该配置，再调用 PowerShell 自动识别探针、下载、校验、复位运行。其他板选择各自的 `Flash slaveN-Debug`。Release 也有独立配置。
5. 实际下载由用户点击 Run 发起。普通 Build 不连接探针、不烧录。此次开发只验证了主机侧流程，没有运行实物下载。

下载配置沿用 Horizon 的 `$CMakeCurrentGenerationDir$` 宏。`flash.ps1` 校验构建目录内的板型、构建类型与 Run 配置一致，禁止把主控构建目录误用到灯板下载入口。灯板 1～5 的 MCU 相同，探针序列号只定位物理探针，用户需确保探针接在对应编号灯板上。

如果看到 `Build/profile identity mismatch`，不要改 `firmware.json` 或删除防错检查。旧的通用 `Debug` / `cmake-build-debug` 默认属于 master。重新加载 CMake（新共享配置没有出现时关闭并重新打开这个工程），选择 `STM32-slave2-Debug` 与 `Flash slave2-Debug`；其他板同理。修复后 slave2 的下载路径应为 `build/slave2-Debug`，不再是 master 的 `cmake-build-debug`。

自动识别支持 CMSIS-DAP/DAPLink、ST-Link、J-Link。原生 SEGGER 驱动使用 `JLink.exe`，其余支持的驱动使用 OpenOCD。保留 USB 描述符中的序列号大小写；连接多只探针时拒绝任意挑选。不会终止已有 OpenOCD/J-Link 会话。

部分 ST-Link 的 USB 序列号描述符无法读取，Windows 只给出类似 `5&3207D635&0&10` 的端口实例标识。脚本不会将其冒充序列号：它用所选 OpenOCD 自带的 libusb 枚举总线/端口链，再与 Windows 的 VID/PID 和物理端口链唯一匹配，生成 `adapter usb location`。相关枚举不打开调试接口、不连接或复位 MCU；端口无法唯一匹配、多探针未明确选择时仍拒绝。每次运行重新识别，不硬编码 `2-10` 之类端口号。正常可读序列号仍优先使用；DAPLink 描述符读取失败仍拒绝。依据：[OpenOCD USB 位置选择](https://www.openocd.org/doc/html/Debug-Adapter-Configuration.html)、[libusb 设备枚举 API](https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html)。

需要固定探针时，将 `tools/flash/flash.local.example.json` 复制为 `flash.local.json`，在对应板项中填写 `mode` 和 `serial`。此文件已忽略，不提交本机配置。OpenOCD 默认从 PATH 查找；也可填写 `openOcdRoot`。同时支持常规 `share/openocd/scripts` 与 xPack 的 `openocd/scripts` 目录结构。

下载前冻结固件并记录 SHA-256、探针身份和工具退出码，日志在 `tools/flash/logs`。OpenOCD 校验后才复位，SEGGER 使用 `LoadFile` 自带校验及 `ExitOnError`。失败不自动重试。

Windows 中文目录兼容：编程工具以冻结目录为工作目录，只接收 ASCII 相对文件名 `flash.cfg` / `flash.jlink` 与 `firmware.elf` / `firmware.hex`；中文绝对路径只保留在 PowerShell 元数据中。无需重命名工程或移动源码。OpenOCD 在访问实际调试接口前先用内置 `dummy` 适配器和 `testee` 模拟目标执行 [`test_image`](https://www.openocd.org/doc/html/General-Commands.html#Image-loading-commands)，强制按 ELF/IHEX 解析冻结镜像。文件不存在、格式损坏或路径不可读时提前失败，不再先暂停实际 MCU。该预检日志中的 `verified ... bytes` 表示主机镜像解析，不是目标 Flash 校验。

`-DryRun` 也走同一套冻结、相对路径和镜像预检，随后退出，便于复现真实文件加载路径。工具进程通过 Windows 工作目录启动，记录其实际 ExitCode，不依赖 PowerShell 5.1 管道中的 `$LASTEXITCODE`。

## 命令行

在仓库根目录运行：

```powershell
cmake --preset master-Debug
cmake --build --preset master-Debug

# 本机未将 Ninja 加入 PATH 时，可传 CLion 随附 ninja.exe 的路径。
.\tools\build-stm32.ps1 -Board all -Configuration Debug -Ninja 'C:\path\to\ninja.exe'

# 只检查构建目录和板型，不扫描 USB、不连接 MCU。
.\tools\flash\flash.ps1 -BuildDir build/master-Debug -ExpectedBoard master -PlanOnly

# 主机侧探针筛选、脚本生成与 OpenOCD 解析测试，不连接硬件。
.\tools\flash\tests\test-probes.ps1
.\tools\flash\tests\test-probes.ps1 -TargetCfg target/stm32f1x.cfg -Device STM32F103C8

# 真正调用 OpenOCD 镜像解析器，覆盖中文/空格/括号路径及坏镜像；不连接硬件。
.\tools\flash\tests\test-image-paths.ps1 -Image build/slave2-Debug/EnergyFirmware.elf
```

`-DryRun` 会扫描 USB、冻结镜像、做主机镜像解析并生成下载命令，但不连接实际目标；`-PlanOnly` 连 USB 都不扫描。实际下载入口不带这两个选项。

`tools/generate_profiles.py` 可重新生成 18 个 Preset、18 个共享 CMake Profile 和 18 个 `.run` 配置。现有源码仍位于各自 CubeMX 目录，电机板另有 `fu_motor`，独立常亮测试另有 `fu_slave6`，带九键检测的测试另有 `fu_slave7`；正常五块灯板业务和 CAN 协议保持原样。主控的公共头文件路径由 `../common` 修正为实际的 `../../common`。

## 验证范围

2026-11-01 追加 slave7：独立全灯与九键检测目标已在两分支完成 Debug/Release 构建、按键行为主机测试和下载入口身份预检；slave1～slave6 源文件指纹不变，未烧录。使用 `STM32-slave7-Debug` / `Flash slave7-Debug`，接线和观察变量见 [slave7 测试说明](../wanwanwanfufu/dbus_switch_energy/fu_slave7/README.md)。

2026-11-01 追加：slave1 已恢复正常 CAN 控制，slave6 独立测试目标的两分支 Debug/Release 均构建通过；配置和下载身份的主机预检通过，未代用户烧录。slave6 不属于原五板 CAN 控制协议，详见 [常亮测试说明](../wanwanwanfufu/dbus_switch_energy/fu_slave6/README.md)。以下为此前统一入口的历史验证记录。

六块板的 Debug 配置已完成干净构建，F407/F103 两种下载配置各通过 58 项主机检查，包含无序列号 ST-Link 的端口绑定与多设备拒绝。Release 配置已生成，未做本次验收构建。修复配置后尚未代替用户在 CLion GUI 中点击实际下载。未烧录、未复位任何实物板。

中文路径修复另通过 20 项镜像检查：实际 ELF/HEX 在中文目录中的解析、SHA-256 不变、固定相对文件名、J-Link 路径生成、缺失/损坏 ELF 提前拒绝、真实非零退出码。已在 PowerShell 5.1 下通过 slave2 原下载入口的 `-DryRun`，解析固件 23,072 字节。本轮未实际烧录；用户提供的旧失败日志显示 MCU 已因调试请求暂停，但固件文件打开失败。

这份统一入口可独立用于 `fu_clion` 分支，无需 ESP32 协议、屏幕源码或 USART6 新功能。分支内容由各分支自己的业务源码决定。
