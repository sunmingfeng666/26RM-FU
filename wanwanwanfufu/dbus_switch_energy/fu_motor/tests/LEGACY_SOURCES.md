# 旧程序测试基准

`legacy_big` 原样复制自 `E:/BaiduSyncdisk/老/dafu_finishi/true_dafu/linlkin/MDK-ARM`。
`legacy_small` 原样复制自 `E:/BaiduSyncdisk/老/dafu_finishi/true_wan_xiaofu/dafu/MDK-ARM`。

2026-10-19 复制了两份 `tim_task.c/.h`、`all_init.h`、`my_can.h`、`vofa.h`，以及大符的 `all_init.c`（两种模式沿用相同 PID 实现）。原文件不修改；主机编译时仅用宏重命名符号以便同时链接两份代码。

这些文件只用于主机回归测试，不进入 STM32 电机固件的编译清单。`SHA256.json` 记录快照来源及内容哈希。
