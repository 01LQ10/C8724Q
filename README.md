# C8724Q LED Matrix Driver

此驱动依据 `datasheet/C8724QP.pdf`（C8724 Datasheet V1，2025-03-10）实现，目录及 API 命名使用 `c8724q`。

## 总线要求

- 使用 SPI 主机输出 MOSI→DIN、SCK→CLK；芯片在 CLK 上升沿采样，数据高位先行。
- SPI 时钟不得超过 20 MHz。datasheet 未规定 CLK 空闲电平或完整 SPI mode，平台配置应确保 DIN 在上升沿前后满足至少 5 ns 建立/保持时间。
- 芯片没有独立片选输入。禁止由 SPI 外设切换一个连接到芯片的硬件 CS；数据帧由 `0x5A, 0xFF` 帧头和校验码定界。
- SPI 写回调必须同步完成，平台应保证连续写调用之间不会产生额外的 DIN/CLK 活动。具体 SPI 写函数应按本平台实现，不可直接使用接口模板桩。

## 功能

- `src/driver_c8724q.c`：命令帧、指令校验、配置校验、显示数据按顺序/地址写入及更新。
- `interface/driver_c8724q_interface.h`：SPI 初始化、SPI 字节流写和调试打印接口。
- `interface/driver_c8724q_interface_template.c`：可编译桩实现；返回成功仅为编译占位，不会实际访问硬件。
- `example/driver_c8724q_basic.c`：8×12、GRID-major 缓冲示例。默认 SEG 输出电流档为 GCC=0（数据手册给出的最小档位），硬件设计及 LED 额定值仍需由使用者确认。
- `test/driver_c8724q_display_test.c`：真机显示测试，覆盖配置宏、帧缓冲绘制与 flush、长度校验、按地址写入和帧更新。
- `test/driver_c8724q_config_test.c`：mock SPI 配置测试，校验字段宏不改影子不动总线、完整配置包及校验和、保留位拒绝、写入失败时影子保持与重试。

## 配置 API

配置以四字节数组为单位传输，流程固定为三步：

1. `c8724q_get_config(handle, config)`：把句柄内的当前配置拷贝到外部数组；
2. 用 `C8724Q_SET_xxx(config, value)` 系列宏在外部数组上修改字段，`C8724Q_GET_xxx(config)` 读回字段；宏对目标字节做**读-改-写**（先清字段位再填新值），不触碰其他字段，也不做值域检查——合法值域由调用方保证（各枚举见 `driver_c8724q.h`）；
3. `c8724q_set_config(handle, config)`：把四个寄存器与数据校验和作为**单个事务**发给芯片，成功后同步更新句柄内的影子配置。

芯片没有配置寄存器读回指令，`c8724q_get_config()` 返回的是驱动维护的影子值；由于每次 `c8724q_set_config()` 都真实下发，影子值即最近一次成功应用的配置。SPI 写失败时返回错误码 1，影子保持不变，修复总线后重发即可。

字段宏覆盖 SEG11/12 引脚功能、GCC、时钟源、SCAN、测试模式、更新模式、OTP1/2、RCS、RCKS、SEG 输出使能、鬼影消除、TLS、休眠/自动休眠、全局/软复位使能、CLK2X 与 PDR。极性说明：`C8724Q_SET_OTP1_ENABLE(cfg, C8724Q_BOOL_TRUE)` 表示打开 125°C 降流保护（寄存器位写 0），`C8724Q_SET_RCS_EXTERNAL()` 为 TRUE 时选择外部电流设置电阻（数据手册仅明确 RCS=0 内部电阻的行为）。寄存器 2 bit 6:5 与寄存器 4 bit 7 为保留位，`c8724q_set_config()` 拒绝置位并返回错误码 4。

## 帧缓冲 API

驱动在句柄内维护 8×12（GRID-major）PWM 帧缓冲：`c8724q_display_set_pixel / get_pixel / fill` 只修改缓冲，`c8724q_display_flush` 按 SCAN 行数把缓冲经顺序写发送并自动调用 `c8724q_display_update` 切帧，`c8724q_display_clear` 等价于 fill(0) + flush。需要局部刷新或精确控制切帧时序时，仍可直接使用 `c8724q_write_display`、`c8724q_write_display_address` 与 `c8724q_display_update`。

顺序显示数据长度必须与 SCAN 配置的 GRID 数量相符，顺序为 GRID1 的 SEG1～SEG12，然后 GRID2，依次类推。按地址写入使用 SRAM 地址 `(GRID 索引 << 4) | SEG 索引`，索引从 0 开始，每次最多 72 项。两种显示写入均须随后调用 `c8724q_display_update` 才切换至新帧。

## 移植

复制并实现 `interface/driver_c8724q_interface_template.c` 中的接口函数，再通过 `DRIVER_C8724Q_LINK_*` 宏绑定到 `c8724q_handle_t`。SPI 写函数不得假定有芯片片选；`spi_init` 应使用符合硬件时序的 SPI 设置。


指令数据包（广播参数，片选参数，指令码）
包头[0x5AFF] + 指令数据码[0|广播|片选|指令码] + 指令校验[包头+指令数据码]

设置数据包（广播参数，片选参数，配置寄存器[4]）
指令数据包（广播参数，片选参数，配置数据写入）+ 配置寄存器x4 + 校验[sum of 寄存器x4]


显示数据包()
指令数据包（广播参数，片选参数，显示数据按顺序写入）+ 显示数据[N] + 校验
指令数据包（广播参数，片选参数，显示数据按地址写入）+ 地址数据[N] + FF + 校验


c8724q
seg[4]
cmd-pkt[4]
