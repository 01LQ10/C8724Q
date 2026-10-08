# C8724 LED Matrix Driver

此驱动依据 `datasheet/C8724P.pdf`（C8724 Datasheet V1，2025-03-10）实现，目录及 API 命名使用 `c8724`。

## 总线要求

- 使用 SPI 主机输出 MOSI→DIN、SCK→CLK；芯片在 CLK 上升沿采样，数据高位先行。
- SPI 时钟不得超过 20 MHz。datasheet 未规定 CLK 空闲电平或完整 SPI mode，平台配置应确保 DIN 在上升沿前后满足至少 5 ns 建立/保持时间。
- 芯片没有独立片选输入。禁止由 SPI 外设切换一个连接到芯片的硬件 CS；数据帧由 `0x5A, 0xFF` 帧头和校验码定界。
- SPI 写回调必须同步完成，平台应保证连续写调用之间不会产生额外的 DIN/CLK 活动。具体 SPI 写函数应按本平台实现，不可直接使用接口模板桩。

## 功能

- `src/driver_c8724.c`：命令帧、指令校验、配置校验、显示数据按顺序/地址写入及更新。
- `interface/driver_c8724_interface.h`：SPI 初始化、SPI 字节流写和调试打印接口。
- `interface/driver_c8724_interface_template.c`：可编译桩实现；返回成功仅为编译占位，不会实际访问硬件。
- `example/driver_c8724_basic.c`：8×12、GRID-major 缓冲示例。初始化先套用实证配置 `C8724_CONFIG_PROVEN`（否则复位默认的 RCS=1 会让恒流基准不建立、全黑），默认 SEG 输出电流档为 GCC=63（实证档；数据手册给出的电流数值表与公式仍互相矛盾，最终档位需按 LED 额定值确认）。
- `test/driver_c8724_display_test.c`：真机显示测试，覆盖配置宏、帧缓冲绘制与 flush、长度校验、按地址写入和帧更新；配置同样从 `C8724_CONFIG_PROVEN` 起步，金标四个字节为 `FF 1D 3D 05`。
- `test/driver_c8724_config_test.c`：mock SPI 测试，同时是**字节金标回归**：断言广播复位 `5A FF 4B A4`、配置帧 `5A FF 01 5A FF 1D 3D 05 5E`、顺序写头 `5A FF 02` + 96 字节数据 + 校验、更新帧 `5A FF 04 5D`，以及保留位拒绝、写入失败时影子保持与重试。无需硬件、可在主机直接运行（见文末命令）。

## 配置 API

配置以四字节数组为单位传输，流程固定为三步：

1. `c8724_get_config(handle, config)`：把句柄内的当前配置拷贝到外部数组；
2. 用 `C8724_SET_xxx(config, value)` 系列宏在外部数组上修改字段，`C8724_GET_xxx(config)` 读回字段；宏对目标字节做**读-改-写**（先清字段位再填新值），不触碰其他字段，也不做值域检查——合法值域由调用方保证（各枚举见 `driver_c8724.h`）；
3. `c8724_set_config(handle, config)`：把四个寄存器与数据校验和作为**单个事务**发给芯片，成功后同步更新句柄内的影子配置。

芯片没有配置寄存器读回指令，`c8724_get_config()` 返回的是驱动维护的影子值；由于每次 `c8724_set_config()` 都真实下发，影子值即最近一次成功应用的配置。SPI 写失败时返回错误码 1，影子保持不变，修复总线后重发即可。

字段宏覆盖 SEG11/12 引脚功能、GCC、时钟源、SCAN、测试模式、更新模式、OTP1/2、RCS、RCKS、SEG 输出使能、鬼影消除、TLS、休眠/自动休眠、全局/软复位使能、CLK2X 与 PDR。极性说明：`C8724_SET_OTP1_ENABLE(cfg, C8724_BOOL_TRUE)` 表示打开 125°C 降流保护（寄存器位写 0），`C8724_SET_RCS_EXTERNAL()` 为 TRUE 时选择外部电流设置电阻——**本 24 脚封装没有 ISET 脚，TRUE 会让恒流基准不建立、实测全黑（2026-10-08），必须传 FALSE**；数据手册也仅明确 RCS=0 内部电阻的行为。推荐直接用 `C8724_CONFIG_PROVEN(cfg)` 载入实证配置（四个字节 `FF 1D 3D 05`，与点灯成功的 CF_* 参照实现逐字节一致），个别字段再用 SET 宏覆盖，但不要把 RCS 改回 TRUE。寄存器 2 bit 6:5 与寄存器 4 bit 7 为保留位，`c8724_set_config()` 拒绝置位并返回错误码 4。

## 帧缓冲 API

驱动在句柄内维护 8×12（GRID-major）PWM 帧缓冲：`c8724_display_set_pixel / get_pixel / fill` 只修改缓冲，`c8724_display_flush` 按 SCAN 行数把缓冲经顺序写发送并自动调用 `c8724_display_update` 切帧，`c8724_display_clear` 等价于 fill(0) + flush。需要局部刷新或精确控制切帧时序时，仍可直接使用 `c8724_write_display`、`c8724_write_display_address` 与 `c8724_display_update`。

顺序显示数据长度必须与 SCAN 配置的 GRID 数量相符，顺序为 GRID1 的 SEG1～SEG12，然后 GRID2，依次类推。按地址写入使用 SRAM 地址 `(GRID 索引 << 4) | SEG 索引`，索引从 0 开始，每次最多 72 项。两种显示写入均须随后调用 `c8724_display_update` 才切换至新帧。

## 移植

复制并实现 `interface/driver_c8724_interface_template.c` 中的接口函数，再通过 `DRIVER_C8724_LINK_*` 宏绑定到 `c8724_handle_t`。SPI 写函数不得假定有芯片片选；`spi_init` 应使用符合硬件时序的 SPI 设置。

## 无硬件字节金标回归（1.2.0 起）

`c8724_config_test()` 用 mock SPI 抓取驱动实际发出的每个事务并逐字节断言，无需硬件，主机可直接运行：

```sh
cc -std=c99 -Wall -Wextra -I BSP/C8724/src -I BSP/C8724/test \
   BSP/C8724/src/driver_c8724.c BSP/C8724/test/driver_c8724_config_test.c \
   .tmp_config_test_host.c -o .tmp_config_test_host.exe && ./.tmp_config_test_host.exe
```

输出 `=> 0 (PASS)` 即驱动帧与实证参考逐字节一致；改了指令码/配置相关代码后先跑它，再上板。

---

指令数据包（广播参数，片选参数，指令码）
包头[0x5AFF] + 指令数据码[B6=广播 | B5:B4=片选地址 | B3:B0=指令码] + 指令校验[包头+指令数据码]
（1.2.0 修正：旧实现把广播放 B7、片选放 B4:B3 —— 抄自数据手册表头，与正文注释及实证实现不符，
且 B4:B3 会与 0x08/0x0B/0x0D/0x0E 等带 bit3 的指令码冲突；现为广播 0x40、片选移位 4）

设置数据包（广播参数，片选参数，配置寄存器[4]）
指令数据包（广播参数，片选参数，配置数据写入）+ 配置寄存器x4 + 校验[sum of 寄存器x4]


显示数据包()
指令数据包（广播参数，片选参数，显示数据按顺序写入）+ 显示数据[N] + 校验
指令数据包（广播参数，片选参数，显示数据按地址写入）+ 地址数据[N] + FF + 校验


c8724
seg[4]
cmd-pkt[4]
