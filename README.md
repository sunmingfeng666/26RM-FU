# 能量机关：统一 CLion 工程（fu_clion）

- [slave6 独立上电全灯常亮测试](wanwanwanfufu/dbus_switch_energy/fu_slave6/README.md)
- [slave7 独立按键与全灯测试](wanwanwanfufu/dbus_switch_energy/fu_slave7/README.md)

新增专用 F1 旋转电机板：选择 `STM32-motor-Debug` / `Flash motor-Debug`，PB10 大符、PB11 小符、双键停止；[接线和旧速度参数](wanwanwanfufu/dbus_switch_energy/fu_motor/README.md)。

在 CLion 打开本目录的 `CMakeLists.txt`，通过 `STM32-master-Debug`、`STM32-slave1-Debug` ～ `STM32-slave5-Debug` 共享配置选择当前板。可执行目标带板型和构建类型，例如 `EnergyFirmware_slave2_Debug`，输出文件统一命名为 `EnergyFirmware.elf`。

若配置列表仍只有旧的 `Debug`，重新加载 CMake；共享配置没有出现时关闭并重新打开本工程。下载 2 号灯板请选择 `STM32-slave2-Debug` 和 `Flash slave2-Debug`。旧 `cmake-build-debug` 是 master 的构建目录，不用于下载灯板。

运行配置 `Flash <板名>-Debug` 会先编译对应固件，然后自动选择单个探针，下载、校验、复位运行。多探针需指定序列号。每块板也提供 Release 配置。

[完整构建、下载和探针配置说明](docs/CLION.md)

该分支新增独立 `fu_motor` 工程并接入统一编译/下载配置。原有主控和灯板的 `.c`、`.h`、`.ioc`、DBUS 控制逻辑保持原样；ESP32 遥控、USART6、屏幕和新分支环数功能仍在遥控分支内。

六个 Debug 配置已完成干净构建。主控 Flash 26,864 B、RAM 2,120 B；五块灯板各 Flash 23,072 B、RAM 18,528 B（含链接脚本预留堆栈）。下载流程仅做主机测试，未执行任何板卡烧录或复位。修改保留在本分支的独立工作区，未提交或推送。
