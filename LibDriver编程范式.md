# LibDriver 嵌入式驱动编程范式代码规范（文件 A）

> **适用对象**：希望使用 AI 进行模块化、解耦嵌入式驱动开发的工程师与 AI 编码助手。
> **规范来源**：GitHub `libdriver` 组织（作者 Shifeng Li，LibDriver 开源组织创始人）的 **187 个**嵌入式芯片/传感器驱动仓库。本文档基于其中 **14 个代表仓库的深度源码分析**（sht30、bme280、aht10、max31855、w25qxx、ds18b20、ws2812b、ch9121、pmsx003、ssd1306、st7789、nrf24l01、ds3231、pca9685），并在族内抽查仓库（aht20、bmp280、max6675、dht11、as608、ssd1309、mfrc522）中验证了范式一致性；全部 187 个仓库的接口类型、驱动对象、功能分类见文末**附录：187 仓库全量清单**（逐仓拉取代码验证，非推断）。
> **本文档定位**：规则条文（“规范是什么、为什么、怎么量化检查”）。可直接套用的代码骨架见配套文档《LibDriver 抽象层骨架代码模板》（文件 B）。

---

## 1. 核心理念：驱动与芯片外设分离

LibDriver 全库 187 个驱动遵循同一设计哲学，可概括为三条：

1. **芯片逻辑与硬件平台彻底分离**：驱动层（`src/`）只包含芯片数据手册上的协议逻辑（寄存器、命令、时序、补偿算法），**不出现任何 HAL / 外设寄存器 / RTOS API**。硬件差异被收敛到一层极薄的“接口抽象层”（`interface/`）。
2. **接口注入而非继承**：驱动不依赖抽象基类，而是在 `handle` 结构体中**用函数指针槽位**注入硬件操作（I2C/SPI/UART 读写、延时、打印、回调）。移植 = 实现一组接口函数 + 用 LINK 宏绑定，驱动本体零改动。
3. **全功能覆盖而非最小实现**：每个驱动提供 `info / init / deinit / set_xxx / get_xxx / 读系列 / set_reg / get_reg` 全量 API，寄存器配置逐条暴露为 `set_xxx`，不隐藏能力。

**可量化验收**：
- `src/` 下所有文件 `grep -iE "HAL_|stm32|__disable_irq"` 结果为空（时序类驱动除外，见 §11.3）；
- 同一驱动在 STM32（MDK/EW 工程）、树莓派 Linux 用户态（Makefile/CMake + libgpiod）两个平台，`src/` 文件完全一致，仅 `project/` 与 `interface` 平台实现不同。

---

## 2. 目录与文件组织规范

### 2.1 仓库结构（七层 + 文档标配）

```
<chip>/
├── src/                    # 芯片驱动层：driver_<chip>.c / driver_<chip>.h（+ _font.h 显示类）
├── interface/              # 接口抽象层：driver_<chip>_interface.h + driver_<chip>_interface_template.c
├── example/                # 示例：driver_<chip>_basic.c/.h、driver_<chip>_advance.*、driver_<chip>_shot.*（按功能分文件）
├── test/                   # 测试：driver_<chip>_read_test.c、driver_<chip>_register_test.c、driver_<chip>_alert_test.c（按测试类型分文件）
├── project/                # 平台工程：raspberrypi4b/（Makefile + CMakeLists.txt）、stm32f407/（MDK/ + EW/ + hal/ + interface/ + usr/）
├── datasheet/              # 芯片数据手册 PDF（目录名必须拼写为 datasheet/）
├── doc/                    # Doxygen 生成文档 + mainpage
├── misra/                  # MISRA / Polyspace 静态分析报告
├── Doxyfile                # Doxygen 配置
├── LICENSE                 # MIT
├── CHANGELOG.md / README.md（中英日韩德等多语言）/ SECURITY.md / CONTRIBUTING.md / CODE_OF_CONDUCT.md
```

### 2.2 分层职责与依赖规则（硬约束）

| 层 | 允许依赖 | 禁止 |
|----|----------|------|
| `src/` | 自身 `.h`、`<stdint.h>/<string.h>/<stdio.h>` | 任何 HAL、平台头、`interface/` 函数 |
| `interface/` | `driver_<chip>.h` | 芯片逻辑、业务函数 |
| `example/` + `test/` | `driver_<chip>.h` + `driver_<chip>_interface.h` | 平台细节（可含平台实现以演示） |
| `project/` | 全部上层 + HAL/OS | —— |

**文件命名**：一律 `driver_<chip>[_module].c/.h`，`<chip>` 为芯片型号**全小写**（`sht30`、`w25qxx`、`pmsx003`）；同一芯片的功能拆分以 `_module` 后缀区分（如 `driver_sht30_alert.c`），**禁止**按平台拆分文件。

---

## 3. 命名规范

### 3.1 总则

所有标识符以 `<chip>_` 小写前缀开头；宏以 `<CHIP>_` 大写前缀开头；见名知义、全库统一。

### 3.2 各类标识符规则表

| 类别 | 格式 | 示例（来自真实仓库） |
|------|------|----------------------|
| 公共函数 | `<chip>_<动词>[_<对象>]` | `sht30_init`、`sht30_single_read`、`sht30_set_repeatability` |
| 私有函数 | `static ... a_<chip>_<动词>`（`a_` 前缀表示文件内私有） | `a_sht30_write`、`a_w25qxx_page_program`、`a_bme280_iic_spi_read` |
| 接口函数 | `<chip>_interface_<总线>_<操作>` | `sht30_interface_iic_write_address16`、`max31855_interface_spi_read_cmd` |
| handle 结构体 | `typedef struct <chip>_handle_s { ... } <chip>_handle_t;` | `sht30_handle_t` |
| info 结构体 | `typedef struct <chip>_info_s { ... } <chip>_info_t;` | `bme280_info_t` |
| 枚举类型 | `<chip>_<语义>_t` | `sht30_repeatability_t`、`ssd1306_interface_t` |
| 枚举值 | `<CHIP>_<语义>_<值>` | `SHT30_RATE_1HZ`、`SHT30_BOOL_TRUE`、`W25QXX_TYPE_W25Q64` |
| 命令/寄存器宏 | `<CHIP>_COMMAND_<名字>` 或 `<CHIP>_REG_<名字>` | `SHT30_COMMAND_SOFT_RESET`、`W25QXX_CMD_PAGE_PROGRAM` |
| 芯片信息宏 | `CHIP_NAME` / `MANUFACTURER_NAME` / `SUPPLY_VOLTAGE_MIN` / `MAX_CURRENT` / `TEMPERATURE_MIN` / `DRIVER_VERSION` | 全库统一 |
| 超时宏 | `<CHIP>_<动作>_TIMEOUT_MS`（`#ifndef` 包裹，允许外部覆盖） | `W25QXX_READ_TIMEOUT_MS` |
| 链接宏 | `DRIVER_<CHIP>_LINK_<槽位>` | `DRIVER_SHT30_LINK_IIC_INIT` |
| 静态全局句柄 | `static <chip>_handle_t gs_handle;`（`gs_` = global static） | `sht30_basic.c` 中一致 |

### 3.3 一致性红线

- 同一仓库内 `iic`（I2C 的缩写）拼写统一为 `iic`，不混用 `i2c`；
- 接口函数名必须与 handle 槽位名、LINK 宏名三方一致（`iic_write_address16` ↔ `LINK_IIC_WRITE_ADDRESS16` ↔ `handle->iic_write_address16`）；
- 不缩写语义不明的单词；`*` 放在变量名侧（`uint8_t *buf`）。

---

## 4. 头文件规范

### 4.1 文件头模板（每个 .c/.h 必带，verbatim 风格）

```c
/**
 * Copyright (c) 2015 - present LibDriver All rights reserved
 * The MIT License (MIT)
 * ...（MIT 许可全文）...
 *
 * @file      driver_<chip>.h
 * @brief     driver <chip> header file
 * @version   2.0.0
 * @author    Shifeng Li
 * @date      2021-03-05
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2021/03/05  <td>2.0      <td>Shifeng Li  <td>format the code
 * <tr><td>2020/11/03  <td>1.0      <td>Shifeng Li  <td>first upload
 * </table>
 */
```

要求：许可头 + Doxygen 元信息 + **history 表**（每次改版追加一行，可追溯）；`@version` 用 `X.Y.Z`。

### 4.2 结构顺序（固定）

1. MIT 许可头 + Doxygen 块
2. `#ifndef DRIVER_<CHIP>_H` / `#define DRIVER_<CHIP>_H` 包含保护（接口层为 `DRIVER_<CHIP>_INTERFACE_H`）
3. `#include <stdio.h> <stdint.h> <string.h>`（只引入需要的标准头）
4. `extern "C" { ... }` 包裹全部内容（C++ 兼容）
5. `@defgroup <chip>_driver` / `@addtogroup <chip>_base_driver` 分组
6. 枚举定义（bool → 地址 → 模式/配置 → 状态位域）
7. `handle` 结构体 → `info` 结构体
8. `LINK` 宏组（`@defgroup <chip>_link_driver`）
9. 公共函数声明（按功能分组，`@defgroup <chip>_base_driver` 等）

### 4.3 枚举写法

```c
/**
 * @brief sht30 status enumeration definition
 */
typedef enum
{
    SHT30_STATUS_ALERT_PENDING_STATUS = (1 << 15),   /**< alert pending status */
    SHT30_STATUS_HEATER_ON            = (1 << 13),   /**< heater on */
    ...
} sht30_status_t;
```

要点：每个枚举值**行尾 `/**< 中文/英文说明 */`**；位域枚举用 `(1 << n)`；地址枚举直接给 I2C 7 位地址左移 1 位的写地址值（`(0x44 << 1)`）。

### 4.4 bool 枚举（全库统一）

```c
typedef enum
{
    <CHIP>_BOOL_FALSE = 0x00,   /**< disable function */
    <CHIP>_BOOL_TRUE  = 0x01,   /**< enable function */
} <chip>_bool_t;
```

注意：部分仓库（ssd1306、bme280、aht10）不定义 `_bool_t` 而用语义枚举，两种情况均合规，但**同一仓库内不可混用**。

### 4.5 handle 与 info 结构体

**handle**（驱动核心，承载所有运行时状态）：
```c
typedef struct <chip>_handle_s
{
    uint8_t (*iic_init)(void);                                          /**< point to an iic_init function address */
    uint8_t (*iic_deinit)(void);                                        /**< point to an iic_deinit function address */
    uint8_t (*iic_write_address16)(uint8_t addr, uint16_t reg, uint8_t *buf, uint16_t len);
    uint8_t (*iic_read_address16)(uint8_t addr, uint16_t reg, uint8_t *buf, uint16_t len);
    void (*receive_callback)(uint16_t type);                            /**< point to a receive_callback function address */
    void (*delay_ms)(uint32_t ms);
    void (*debug_print)(const char *const fmt, ...);
    uint8_t iic_addr;       /**< iic device address */
    uint8_t repeatability;  /**< repeatability value */
    uint8_t inited;         /**< inited flag */
} <chip>_handle_t;
```

规则：
- **函数指针槽位在前、数据字段在后**；每个槽位行尾注释；
- `inited` 标志**必有**（0=未初始化，1=已初始化）；
- 总线不同则槽位不同：SPI 为 `spi_init/spi_deinit/spi_read_cmd...`，UART 为 `uart_read/write/flush...`；多条总线/受控引脚（RST/DC/CE/OE/SET）**按需拼装**槽位（mfrc522 甚至有 iic+spi+uart 三组）；
- 需要跨调用状态时追加字段：UART 类加组包 `buf[]` 与 `status/packet_size`；nrf24l01 加 `finished` 三态标志；w25qxx 加 `buf[262]`/`buf_4k[4097]` 缓冲与 `type` 型号。

**info**（静态芯片信息，全库统一 9 字段）：
```c
typedef struct <chip>_info_s
{
    char chip_name[32];         /**< chip name */
    char manufacturer_name[32]; /**< manufacturer name */
    char interface[8];          /**< chip interface name（如 "IIC"/"SPI"/"UART"） */
    float supply_voltage_min_v; /**< chip min supply voltage */
    float supply_voltage_max_v; /**< chip max supply voltage */
    float max_current_ma;       /**< chip max current */
    float temperature_min;      /**< chip min operating temperature */
    float temperature_max;      /**< chip max operating temperature */
    uint32_t driver_version;    /**< driver version */
} <chip>_info_t;
```

### 4.6 LINK 宏组（接口注入的语法糖）

```c
#define DRIVER_<CHIP>_LINK_INIT(handle, structure)   memset(handle, 0, sizeof(structure))
#define DRIVER_<CHIP>_LINK_IIC_INIT(handle, fuc)     (handle)->iic_init = fuc
#define DRIVER_<CHIP>_LINK_IIC_DEINIT(handle, fuc)   (handle)->iic_deinit = fuc
#define DRIVER_<CHIP>_LINK_IIC_READ_ADDRESS16(handle, fuc)  (handle)->iic_read_address16 = fuc
/* ...每个槽位一条，槽位名与 handle 字段一致 ... */
```

`LINK_INIT` 用 `memset(0)` 清零整块 handle（保证未绑定槽位为 NULL，可被 init 校验发现）。

---

## 5. 接口抽象层规范

### 5.1 本质

接口层是**唯一允许出现平台代码**的移植点：一个 `.h`（声明）+ 一个 `_template.c`（桩实现，函数体 `return 0`/空体，保留完整 Doxygen 注释）。用户移植时复制 template 为平台实现（如 `driver_<chip>_interface.c`），填函数体即可。

### 5.2 接口函数集（按总线族，全部为 `<chip>_interface_<总线>_<操作>`）

| 总线族 | 函数集 | 说明 |
|--------|--------|------|
| I2C（8 位 reg） | `iic_init / iic_deinit / iic_write(addr, reg8, buf, len) / iic_read(addr, reg8, buf, len)` | 寄存器地址 8 位 |
| I2C（16 位 reg） | `iic_init / iic_deinit / iic_write_address16(addr, reg16, buf, len) / iic_read_address16(...)`（可加 `iic_scl_read_address16`） | sht30 类 |
| I2C（命令式） | `iic_init / iic_deinit / iic_write_cmd(addr, buf, len) / iic_read_cmd(addr, buf, len)` | aht10 类，无寄存器地址 |
| SPI（只读） | `spi_init / spi_deinit / spi_read_cmd(buf, len) / delay_ms / debug_print` | max31855/max6675，仅 5 个函数 |
| SPI（全功能） | `spi_qspi_write_read(instruction, 各线宽, address, address_len, alternate, dummy, in_buf, in_len, out_buf, out_len, data_line)` | w25qxx 类；SPI 是其特例（线宽=1）；另加 `delay_us` |
| UART | `uart_init / uart_deinit / uart_read(buf, len)→uint16_t / uart_write / uart_flush / delay_ms / debug_print` | `uart_read` 返回**实际读到的字节数**而非状态码；无回调，主机轮询 |
| 单总线位拨 | `bus_init / bus_deinit / bus_read(单bit) / bus_write(单bit) / delay_us / delay_ms / enable_irq / disable_irq / debug_print` | ds18b20/dht11；时序事务须关中断 |
| 受控 GPIO | `<xxx>_gpio_init / <xxx>_gpio_deinit / <xxx>_gpio_write(...)`（xxx = reset/cmd_data/cs/ce/oe/set...） | 按芯片需要追加 |
| 通用 | `<chip>_interface_delay_ms(uint32_t ms)`、`<chip>_interface_debug_print(const char *const fmt, ...)` | 全库必有 |
| 回调 | `<chip>_interface_receive_callback(...)`（参数随事件变：单参 type / 四参带数据） | 按需；UART 族与显示族无 |

### 5.3 接口返回约定

- 除 `uart_read`（返回字节数）、`delay_*`/`debug_print`/回调（void）外，接口函数一律 `uint8_t`：**0=成功，1=失败**；
- 接口层**不做参数校验**（addr/reg 合法性由驱动层保证）；
- 桩实现必须能编译通过（`return 0`），使“未移植”时驱动仍可构建。

### 5.4 多总线选择（运行时分发）

双总线芯片（bme280 I2C+SPI、ssd1306 I2C+SPI、mfrc522 三总线）：
- handle 增加选择字段（`uint8_t iic_spi;`）与 `typedef enum { <CHIP>_INTERFACE_IIC = 0, <CHIP>_INTERFACE_SPI = 1 } <chip>_interface_t;`
- 内部用 `static a_<chip>_xxx` 函数按 `if (iic_spi == IIC) ... else ...` **运行时分支**派发（如 `a_bme280_iic_spi_read`）；SPI 路径 8 位寄存器高位 bit7 由驱动置/清（读 `|0x80`、写 `&0x7F`）；
- example 只绑定所选模式的槽位。

### 5.5 跨平台移植实证

- 待完善，暂不考虑

---

## 6. 错误处理规范

### 6.1 返回码统一语义（全库 187 仓库一致）

| 码 | 语义 | 处理要求 |
|----|------|----------|
| `0` | 成功 | —— |
| `1` | 操作失败（总线/芯片动作失败） | 返回前必须 `debug_print("<chip>: <动作> failed.\n")` |
| `2` | 参数无效（`handle == NULL`） | **不打印**，直接返回；所有公共函数第一条语句 |
| `3` | 未初始化（`inited != 1`）或函数指针为 NULL | 指针 NULL 时**先** `debug_print("<chip>: xxx is null.\n")` 再返回；`inited` 不打印 |
| `4+` | 芯片特定错误（数据未就绪/ID 错误/超时/故障/校验失败/校准失败…） | 每个函数 `@return` 逐条列出；同芯片内语义一致 |

扩展惯例：`4=数据未就绪/ID 错误/故障/超时`，`5=校验/校准/帧错`，`6=读校准失败`；pmsx003 用 `4=frame error, 5=data error`；w25qxx init 细分到 `8`。**扩展码**从 4 起，数值语义在头文件 Doxygen 中逐条声明。

### 6.2 校验顺序（每个公共函数固定三段）

```c
uint8_t <chip>_xxx(<chip>_handle_t *handle, ...)
{
    if (handle == NULL)          /* ① 句柄校验 */    return 2;
    if (handle->inited != 1)     /* ② 初始化校验 */   return 3;
    /* ③ 业务参数校验（越界/指针为空），失败 return 4/5... */
    ...
}
```

### 6.3 错误日志格式

- 统一英文消息，行尾英文短注释（驱动源码注释全部为英文，实测 14 个代表仓库 0 个文件含中文，见 §13.1）：`handle->debug_print("<chip>: write command failed.\n"); /* write command failed */`
- 消息模板：`"<chip>: <动作> failed.\n"`、`"<chip>: <对象> is null.\n"`、`"<chip>: <动作> timeout.\n"`；
- **禁止直接 `printf`**——必须经 handle 注入的 `debug_print`（便于用户重定向/裁剪）。

### 6.4 失败回滚（事务性）

init 内任何一步失败：先 `debug_print`，再**回滚已初始化的资源**（调用对应接口 deinit，`(void)` 忽略返回值），最后 `return 1`。deinit 失败也记录并返回，但**保证执行到清 inited**（w25qxx 漏清 inited 是全库唯一反例，见 §18）。

---

## 7. 生命周期规范：init / deinit

### 7.1 init 状态机（全库统一）

```
① handle == NULL                    → return 2
② 逐个校验函数指针槽位 == NULL      → debug_print("<chip>: xxx is null.\n") + return 3
③ 调总线 init（iic_init/spi_init/uart_init...）  → 失败 debug_print + return 1
④ 芯片上电/软复位序列（软复位命令 → delay → 可选读 ID 自检 / 清状态位 / 读校准参数）
    任一步失败                       → (void)对应总线 deinit() 回滚 → return 1
⑤ handle->inited = 1               → return 0
```

### 7.2 deinit 状态机

```
① handle == NULL                    → return 2
② handle->inited != 1               → return 3
③ 芯片掉电/退出低功耗命令（先关芯片）
④ 总线 deinit()（后关总线）          → 失败 debug_print + return 1
⑤ handle->inited = 0                → return 0（必须执行到）
```

### 7.3 软件/硬件初始化分离（关键规则）

- **驱动 init 只做硬件 bring-up**（总线使能 + 芯片上电序列），**不包含完整寄存器配置**；
- 寄存器配置序列由**示例层逐条调用公开 `set_xxx` 编排**（如 st7789 的 MADCTL/COLMOD/伽马、ssd1306 的对比度/电荷泵/多路复用），每步失败 `(void)<chip>_deinit()` 回滚；
- 结论：公开 API 必须覆盖数据手册全部可配置寄存器，一个寄存器一个 `set_xxx`，而非 init 内写命令表数组。

---

## 8. 功能 API 设计规范

### 8.1 分组与命名

| 组 | 函数 | 备注 |
|----|------|------|
| base（必选） | `info / set_addr_pin / get_addr_pin / init / deinit` | 任何芯片 |
| read（按芯片） | `single_read` / `start_continuous_read` / `continuous_read` / `stop_continuous_read` | 传感器；显示类为 `display_on/off` + 绘图 |
| 配置（成对） | `set_<item>` / `get_<item>` | 可配置项全部成对；读配置用出参 `*value` |
| 事件 | `irq_handler(handle)` | 用户在中断里调用；读状态寄存器→清标志→按位分派回调 |
| 扩展 | 告警/高级功能（`set_alert_limit` 等）、`_convert_to_register` / `_convert_to_data` 换算对 | 按数据手册 |
| register（兜底） | `set_reg(handle, reg, buf, len)` / `get_reg(handle, reg, buf, len)` | 命令式芯片改为 `(handle, buf, len)`；显示类改为 `write_cmd/write_data` |

### 8.2 读函数输出约定

- raw 值与物理量**同时输出**（`uint16_t *raw` + `float *temperature` 等），换算在驱动内完成并 clamp 量程；
- 无寄存器地址的总线（1-Wire/UART/命令式 I2C）用语义化 API 替代 `set_reg/get_reg`：如 ds18b20 的 `scratchpad_set/get_resolution`、pmsx003 的 `read_temperature_humidity`。

### 8.3 位域操作

统一“读-改-写”：`a_<chip>_read(reg) → data |= (1<<bit) / &= ~(1<<bit) → a_<chip>_write(reg)`。

---

## 9. 时序与等待规范

1. **等待两种范式**：
   - 简单芯片：固定延时 + 单次状态检查（aht10：读后 `delay_ms(85)` 再查 busy 位）；
   - 复杂芯片：**轮询状态位 + 超时宏**（bme280 forced 模式轮询 ctrl_meas 归零、ds18b20 轮询转换位、w25qxx 轮询 WIP）。节拍 10ms（`timeout = XXX_TIMEOUT_MS / 10; while(timeout--){...delay_ms(10);}`），无 RTOS 依赖。
2. **超时宏**：`#define <CHIP>_XXX_TIMEOUT_MS ...` 用 `#ifndef` 包裹，允许用户编译期覆盖；超时返回 `4`。
3. **位拨时序**（1-Wire/单总线）：
   - 接口提供 `delay_us` 与 `enable_irq/disable_irq`；
   - 每个时序事务（复位/读位/写位）**整段关中断**，错误路径也必须先恢复中断再返回；
   - 时序参数直接以 `delay_us` 逐拍拼接（复位 750µs、写 1=2µs高+60µs低等），无状态机。
4. **SPI 合成时序**（ws2812b）：10MHz SPI 位流逼近纳秒时序（`one_code=0xFFF8`/`zero_code=0xE000`），**不需要** `delay_us` 与关中断；每灯 48 字节、复位帧 512 字节。
5. **构建要求**：位拨/合成时序驱动 Release 必须 `-O3 -DNDEBUG`，`-O0` 会破坏时序（见 §18）。

---

## 10. 缓冲与内存规范

- **小显存**：驱动内置完整本地显存（ssd1306 `gram[128][8]`），`gram_*` 改本地缓冲后须显式 `gram_update()` 刷屏；
- **大屏/大块**：驱动不内置大静态数组，**缓冲由调用方提供**（ws2812b `write(..., *temp, temp_len)`、st7789 4KB 分块写），驱动校验 `temp_len` 下限（如 `48*len+512`）后使用，越界返回错误码；
- UART 组包缓冲放 handle（`buf[128/384]` 级），属跨调用状态，归驱动所有。

---

## 11. 构建系统规范

### 11.1 Linux 用户态（project/raspberrypi4b/）

- Makefile 模板见文件 B §9.1：`-Wall -Wextra -O3 -DNDEBUG`、`libgpiod` 经 pkg-config 链接、一次产出 client 可执行文件 + 动态/静态库，可选 CMakeLists；
- 依赖注入：Linux 侧 I2C 可用 `/dev/i2c-N` ioctl、SPI 用 spidev（`cs_change=0` 自动释放片选）、GPIO 用 libgpiod。

### 11.2 单片机（project/stm32f407/）

- Keil MDK + IAR EW **双工程**，目录含 `cmsis/ hal/ interface/ usr/ driver/`；
- 驱动源码以 `src/*.c` 形式加入工程，不修改任何驱动文件。

### 11.3 平台差异红线

时序类驱动（ds18b20/dht11/ws2812b）的 `src/` 允许出现 `disable_irq` 概念（经接口抽象），但**平台具体实现（`__disable_irq()` 等）只能在 interface 实现或 project 中**；`src/` 中 `grep -E "HAL_|__disable_irq"` 应为空。

---

## 12. 文档规范

1. **Doxygen**：根目录 `Doxyfile`，`doc/` 为输出；函数分组用 `@defgroup`/`@addtogroup`（`<chip>_driver` → `<chip>_base_driver`/`<chip>_link_driver`/`<chip>_interface_driver`/`<chip>_extend_driver`/`<chip>_example_driver`/`<chip>_test_driver`）。
2. **README**：中英日韩德等多语言；含芯片特性、接线图、API 索引、Linux/STM32 构建与运行步骤。
3. **MISRA**：`misra/` 放 Polyspace 等静态分析报告；代码风格以 MISRA-C 为基线（`a_` 私有前缀、`(void)` 显式丢弃返回值、`if/else` 成对、行尾注释等均可追溯）。
4. **版本管理**：`CHANGELOG.md` + 头内 history 表同步维护；`@version` 与 `DRIVER_VERSION` 宏对应（如 2.0.0 ↔ 2000）。

---

## 13. 注释规范

> 本章基于 14 个代表仓库 `src/ interface/ example/ test/` 全部源码的实测归纳（sht30、bme280、aht10、max31855、w25qxx、ds18b20、ws2812b、ch9121、pmsx003、ssd1306、st7789、nrf24l01、ds3231、pca9685）。

### 13.1 注释语言与总体风格

- **驱动源码注释全部为英文**。实测 14 个代表仓库的 `src/ *.c/*.h`、`interface/*.h`、`example/*.c`、`test/*.c` 中**0 个文件含中文字符**；中文只出现在 README、CHANGELOG 等多语言文档中。
- 风格：短句、首字母小写（`iic init`）、祈使/名词短语、无句尾句号；代码与注释同行尾时做**列对齐**。
- **量化验收**：`grep -rP '[\x{4e00}-\x{9fff}]' src/ interface/ example/ test/` 返回空（新驱动同样适用）；抽查 `grep -cE ';\s*/\*' src/driver_<chip>.c` 大于 0（有行尾注释）。

### 13.2 文件头注释（每个 .c/.h 必带）

真实模板（verbatim，来自 `sht30/src/driver_sht30.h` 开头）：

```c
/**
 * Copyright (c) 2015 - present LibDriver All rights reserved
 *
 * The MIT License (MIT)
 * ...（MIT 许可全文）...
 *
 * @file      driver_sht30.h
 * @brief     driver sht30 header file
 * @version   2.0.0
 * @author    Shifeng Li
 * @date      2021-03-05
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2021/03/05  <td>2.0      <td>Shifeng Li  <td>format the code
 * <tr><td>2020/11/03  <td>1.0      <td>Shifeng Li  <td>first upload
 * </table>
 */
```

规则：
1. 许可块在前（Copyright + MIT 全文），Doxygen 元信息在后；
2. `@file` 必须与实际文件名一致；`@brief` 措辞固定为 `driver <chip> header/source file`（头文件）或 `driver <chip> <module> source file`（模块文件）；
3. `@version` 用 `X.Y.Z`；`@author`、`@date` 必填；
4. **history 表必带**：每次改版追加一行（日期/版本/作者/描述，`<tr><td>` HTML 表格），首个版本写 `first upload`。
- **量化验收**：每个文件头 `grep -c "@file"` = 1 且与文件名一致；`grep -c "first upload"` ≥ 1；history 表 `<tr><td>` 行数与 CHANGELOG 版本数一致。

### 13.3 函数注释（Doxygen 四件套）

每个函数（含 static 私有函数）独立 Doxygen 块，真实模板（来自 `sht30/interface/driver_sht30_interface.h`）：

```c
/**
 * @brief      interface iic bus read with 16 bits register address
 * @param[in]  addr iic device write address
 * @param[in]  reg iic register address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len length of the data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
uint8_t sht30_interface_iic_read_address16(uint8_t addr, uint16_t reg, uint8_t *buf, uint16_t len);
```

措辞习惯：
1. `@brief`：小写短语，格式 `[interface/basic/...] <chip> <动作> <对象>`（如 `interface iic bus init`、`basic example init`、`read test`）；无参无返回值时写 `@brief  interface iic bus init`（两个空格对齐）；
2. `@param[in]` / `@param[out]`：格式 `<名字> <描述>`，指针参数写 `*buf pointer to a data buffer`（`*` 在参数名前）；
3. `@return`：**逐条列出全部返回码**，格式 `- 0 success` / `- 1 read failed` / `- 2 handle is invalid` / `- 3 ...`，缩进对齐，扩展错误码同样逐条列（见 §6.1）；
4. `@note`：无特别说明写 `none`；
5. **不使用 `@verbatim`**：实测驱动源码（14 仓库）中 `@verbatim` 零出现，仅随附的 STM32 HAL 使用；驱动内如需展示多行内容用普通描述文本。
- **量化验收**：`grep -c "@brief" src/driver_<chip>.h` = 公共函数声明数；每个 `@return` 后必有 `- 0 success` 行；`grep -rn "@verbatim" src/ example/ test/` 返回空。

### 13.4 结构体 / 枚举 / 宏注释

- **结构体**：`typedef struct <chip>_xxx_s` 前加 `@brief` 块；**每个字段行尾 `/**< 描述 */`**，函数指针字段措辞固定为 `point to an <函数名> function address`（handle 结构体证据见 §4.5）。
- **枚举**：类型前 `@brief` 块；**每个枚举值行尾 `/**< 描述 */`**，位域枚举注释写明位含义（如 `/**< alert pending status */`）；地址枚举注明引脚连接（`/**< ADDR pin connected to GND */`）。
- **宏**：全部行尾 `/**< 描述 */`。命令宏证据（`w25qxx/src/driver_w25qxx.c`）：

```c
#define W25QXX_COMMAND_WRITE_ENABLE                0x06    /**< write enable */
#define W25QXX_COMMAND_READ_STATUS_REG1            0x05    /**< read status register-1 */
```

- **量化验收**：`grep -c '/\*\*<' src/driver_<chip>.h` ≥ 字段数 + 枚举值数 + 宏数之和（可人工抽样 10 处核对）；宏定义行 100% 带 `/**<` 注释。

### 13.5 行内注释与步骤注释

- **行尾注释**：关键语句后加英文短短语，同代码块列对齐。真实证据（`st7789/src/driver_st7789.c`）：

```c
    res = handle->cmd_data_gpio_write(cmd);        /* write gpio */
    if (res != 0)
    {
        return 1;                                  /* return error */
    }
    ...
    return 0;                                      /* success return 0 */
```

- **步骤注释**：功能块前一行 `/* <英文短语> */` 说明意图（如 `/* set command */`、`/* wait 10 ms */`、`/* check handle */`、`/* flag finish */`、`/* return error */`）。
- **常用短语表**（全库高频）：`check handle` / `return error` / `success return 0` / `flag finish` / `flag close` / `iic write` / `iic read` / `set command` / `delay 2ms` / `link functions` / `set addr pin`。
- **量化验收**：每个公共函数体至少 1 处步骤或行尾注释；错误返回语句行 100% 带 `/* return error */` 类注释。

### 13.6 分组注释（替代整行分隔线）

- 代码分节用 **Doxygen 分组**而非整行分隔线：`@defgroup <chip>_driver`（总组）→ `@addtogroup <chip>_base_driver` / `<chip>_link_driver` / `<chip>_interface_driver` / `<chip>_extend_driver` / `<chip>_example_driver` / `<chip>_test_driver`，每组以 `@}` 闭合；
- **实测证据**：`/*----- 整行分隔线 */` 与 `@verbatim` 一样只存在于随附 STM32 HAL（`project/stm32f407/hal/`）中，驱动源码全部用 Doxygen 分组 + 空行分隔；
- **量化验收**：`grep -c "@defgroup\|@addtogroup" src/driver_<chip>.h` ≥ 2（至少总组 + base 组）；`.c` 中整行 `/*---*/` 分隔线为 0。

### 13.7 命令宏 / 寄存器 / 私有函数注释约定

- **命令与寄存器宏**：集中在 `.c` 顶部（不进公共头），`#define <CHIP>_COMMAND_<名字>  0xXXXXU  /**< <描述> */`（sht30 的 `SHT30_COMMAND_SOFT_RESET 0x30A2U`、w25qxx 的 `W25QXX_COMMAND_*` 均如此）；芯片信息宏（`CHIP_NAME` 等）同样行尾注释；
- **超时宏**：`#ifndef <CHIP>_XXX_TIMEOUT_MS ... #endif` 包裹并注明默认超时用途；
- **私有函数**：`static` + `a_<chip>_` 前缀，**与公共函数同等 Doxygen 待遇**（完整 `@brief/@param/@return/@note`），证据见 §8.1 与文件 B §3.2；
- **量化验收**：`.c` 顶部宏区 `grep -c 'U\s*/\*\*<'` ≥ 命令宏总数；`a_<chip>_` 前缀函数 `grep -c "@brief"` 一一对应。

---

## 14. 示例与测试代码编写规范

> 本章证据来自 `example/`、`test/` 目录真实源码（sht30、bme280、w25qxx、ds18b20、ssd1306、st7789 等），及 `project/raspberrypi4b/Makefile`。

### 14.1 示例文件组织

- 命名：`driver_<chip>_<场景>.c/.h`，场景划分惯例：`basic`（常规使用流程）、`shot`（单次读取）、`advance`（高级功能）；如 `driver_sht30_basic.c`、`driver_sht30_shot.c`、`driver_sht30_alert.c`；
- 每个 `.c` **必须**有同名 `.h`；头文件包含保护 `DRIVER_<CHIP>_<场景>_H`，`@defgroup <chip>_example_driver` 分组；
- 示例头文件定义**默认配置宏**（证据 `sht30/example/driver_sht30_basic.h`）：

```c
#define SHT30_BASIC_DEFAULT_RATE                 SHT30_RATE_10HZ              /**< rate 100Hz */
#define SHT30_BASIC_DEFAULT_REPEATABILITY        SHT30_REPEATABILITY_HIGH     /**< set high */
#define SHT30_BASIC_DEFAULT_HEATER               SHT30_BOOL_FALSE             /**< disable heater */
```

- 示例 API 为 `uint8_t <chip>_basic_init(...)` / `<chip>_basic_read(...)` / `<chip>_basic_deinit(void)` 三件套，均带 Doxygen（返回 0 成功 / 1 失败）；
- **量化验收**：`ls example/ | grep -c '\.c$'` = `grep -c '\.h$'`（成对）；每个示例 .h 含 `_DEFAULT_` 配置宏。

### 14.2 示例执行流程（五段式，全库统一）

```
① LINK 绑定：static <chip>_handle_t gs_handle; + DRIVER_<CHIP>_LINK_INIT + 逐槽位 LINK_XXX 绑定 interface 函数
② info 打印：<chip>_info(&info) → debug_print 逐行打印 chip_name/manufacturer/interface/版本/电压/电流/温宽
③ init + 配置编排：<chip>_init → 逐条 set_xxx（每步失败 debug_print + (void)deinit 回滚 + return 1）
④ 主循环：<chip>_read / continuous_read 读取并 debug_print 打印物理量，固定延时
⑤ deinit：stop_continuous_read → <chip>_deinit
```

真实证据（`sht30/example/driver_sht30_basic.c`）：

```c
    /* link functions */
    DRIVER_SHT30_LINK_INIT(&gs_handle, sht30_handle_t);
    DRIVER_SHT30_LINK_IIC_INIT(&gs_handle, sht30_interface_iic_init);
    ...
    /* sht30 info */
    res = sht30_info(&info);
    if (res != 0) { ... return 1; }
    /* print chip information */
    sht30_interface_debug_print("sht30: chip is %s.\n", info.chip_name);
    sht30_interface_debug_print("sht30: driver version is %d.%d.\n",
                                info.driver_version / 1000, (info.driver_version % 1000) / 100);
    ...
    /* set default repeatability */
    res = sht30_set_repeatability(&gs_handle, SHT30_BASIC_DEFAULT_REPEATABILITY);
    if (res != 0)
    {
        sht30_interface_debug_print("sht30: set repeatability failed.\n");
        (void)sht30_deinit(&gs_handle);
        return 1;
    }
    ...
    for (i = 0; i < times; i++)
    {
        res = sht30_continuous_read(&gs_handle, &temperature_raw, &temperature_s, &humidity_raw, &humidity_s);
        if (res != 0) { debug_print(...); (void)sht30_deinit(&gs_handle); return 1; }
        sht30_interface_debug_print("sht30: temperature is %0.2fC.\n", temperature_s);
        sht30_interface_debug_print("sht30: humidity is %0.2f%%.\n", humidity_s);
        /* wait 2500 ms */
        sht30_interface_delay_ms(2500);
    }
```

- **量化验收**：示例 init 函数中 `grep -c "LINK_INIT"` = 1；info 打印字段 ≥ 5 项；配置步骤每步失败均 `(void)<chip>_deinit` 回滚。

### 14.3 返回值检查与调试输出约定（示例与测试通用）

- 失败模式固定四连：`if (res != 0) { <chip>_interface_debug_print("<chip>: <动作> failed.\n"); (void)<chip>_deinit(&gs_handle); return 1; }`；
- 每步动作前先打印进度：`sht30_interface_debug_print("sht30: set rate 0.5Hz.\n");`；
- 物理量打印格式：温度 `%0.2fC`、湿度 `%0.2f%%`、电压 `%0.1fV`、电流 `%0.2fmA`、版本 `%d.%d`（`driver_version/1000` 与 `(version%1000)/100`）；
- 输出经 `debug_print`，不直接 `printf`（与 §6.3 一致）；
- **量化验收**：示例/测试中 `printf` 出现次数 = 0；每个失败分支含 `return 1` 且失败前有 `debug_print`。

### 14.4 测试文件组织

- 命名：`driver_<chip>_<测试类型>_test.c/.h`，按测试类型分文件：`read_test`（数据读取）、`register_test`（寄存器读写）、`alert_test`（告警/事件）；显示屏类收敛为单个 `display_test`；
- 头文件：包含保护 `DRIVER_<CHIP>_<类型>_TEST_H`，`@addtogroup <chip>_test_driver` 分组（证据 `sht30/test/driver_sht30_read_test.h`）；
- 测试函数签名惯例：`uint8_t <chip>_read_test(<chip>_address_t addr_pin, uint32_t times)`（参数 = 可配置项 + 循环次数）；
- 与 example **同构**：独立 `static <chip>_handle_t gs_handle;`、独立 LINK 绑定、真机调用接口层（**禁止 mock 桩直接返回**）；
- **量化验收**：`ls test/ | grep -c '\.c$'` = `.h` 数；每个测试函数带完整 Doxygen 且返回 0/1。

### 14.5 测试断言与自检模式

- **断言即"失败即返回"**：无独立 assert 宏，统一 `res != 0 → debug_print 失败原因 → (void)deinit 回滚 → return 1`；
- 覆盖范式（证据 `sht30/test/driver_sht30_read_test.c`）：
  1. info 自检：`<chip>_info` 失败即退出，成功则打印全部信息字段；
  2. 配置组合覆盖：同一测试内切换多组配置（`REPEATABILITY_LOW`+`RATE_0P5HZ` → `RATE_1HZ` …），每组 start → 循环 `times` 次读 → stop，**每步失败即回滚返回**；
  3. register 测试：对全部可读写寄存器做写→读回比对（`register_test` 专属）；
- 主循环 `for (i = 0; i < times; i++)` 内读取 + 打印物理量，配合 `delay_ms` 节拍；
- **量化验收**：每个测试文件含 ≥1 个 `for` 循环（times 次）与 ≥2 个失败回滚分支；register_test 含写后读回比对（`set_reg` + `get_reg` 配对）。

### 14.6 测试与构建的关系

- 树莓派 `project/raspberrypi4b/Makefile` 把 `example/` 与 `test/` 一并编入 `client` 可执行文件（真机入口），并产出动态/静态库（证据）：

```makefile
CFLAGS  += -I ../../example/ \
           -I ../../test/
SRC     := $(wildcard ../../example/*.c) \
           $(wildcard ../../test/*.c) \
           ...
all: client lib<chip>.so lib<chip>.a
```

- 同一 Makefile 用 `-O3 -DNDEBUG`；`client` 运行即执行示例主流程与测试（真机/开发板），`lib<chip>.so/.a` 供应用链接；
- 测试无独立 assert 框架、无 CI 集成，靠"失败即非零退出 + 日志"人工判读——与库的嵌入式定位一致；
- **量化验收**：Makefile 中 `grep -c "example\|test"` ≥ 2（含 `-I` 与 `wildcard` 各一）；`-O3` 与 `-DNDEBUG` 均出现。

---

## 15. 版本兼容与多型号覆盖

- **一个驱动覆盖一个系列**：AT24CXX/M24CXX/GT24CXX（EEPROM 容量族）、W25QXX（12 个型号 0xEF10~0xEF21）、SHT4X/STS4X 等通过**运行时枚举**（`<chip>_type_t` + `set_type()`）而非编译期宏区分型号；
- 容量/地址宽度差异（如 W25Q256 切 4 字节地址、>128Mb 写 0xC5 扩展地址寄存器）在驱动内部按 `type` 分支；
- 新增型号 = 增加枚举值 + JEDEC ID 表项，驱动框架不动。

---

## 16. 异步事件与回调规范

- 需要异步的芯片（sht30 告警、nrf24l01 收发完成、RFID 卡检测）在 handle 提供 `receive_callback` 槽位 + 公共 `irq_handler`；
- `irq_handler`：用户外部中断调用 → 读状态寄存器 → 写回清标志 → 按位分派（如 nrf24l01：TX_DS→finished=1、MAX_RT→flush+finished=2、RX_DR→读 payload 并回调）；
- 回调签名随事件内容分化（单参 type / 四参含数据缓冲），按芯片声明；
- 纯轮询芯片（UART 族、显示族）**不提供**回调，不强行统一。

---

## 17. AI 使用规范（调用 AI 生成/修改驱动时的提示词要点）

1. 告诉 AI：**“按 LibDriver 范式生成 `<chip>` 驱动”**，并附上文件 B 对应总线模板 + 附录中同族仓库名（如“参考 max31855 的 interface 结构”）；
2. 强约束清单（写进提示词）：
   - 三层分离：`src/` 禁止 HAL；接口桩 `return 0`；
   - handle 函数指针槽位 + LINK 宏 + `inited`；
   - 返回码 0/1/2/3 + 扩展码；校验顺序三段式；失败必 `debug_print` + 回滚；
   - 私有函数 `a_<chip>_` 前缀；命令宏在 `.c` 顶部；文件头 MIT + Doxygen + history；
   - 超时宏 `#ifndef` 可覆盖；`-O3 -DNDEBUG`；
   - 寄存器配置在 example 层逐条 `set_xxx`，init 只做 bring-up；
3. 验收（AI 自检或人工检查）：`grep -iE "HAL_|stm32" src/` 为空；`inited` 在 init/deinit 成对置位；接口函数与 handle 槽位/LINK 宏三方同名。

---

## 18. 反例与陷阱清单（全库扫描所得）

| 反例 | 位置 | 规范要求 |
|------|------|----------|
| `w25qxx_deinit()` 漏执行 `handle->inited = 0` | repos/w25qxx | deinit 收尾必须清 inited（sht30/max31855/max6675 均正确，此为个案） |
| 目录拼写 `datsheet/`（应为 `datasheet/`） | repos/max31855、nrf24l01 | 目录名全库统一拼写 |
| 位拨时序在 `-O0` 下运行导致时序漂移 | 时序类 | 必须 `-O3 -DNDEBUG` 构建 |
| 在 init 内写超长寄存器配置序列 | 违反 §7.3 | 配置序列移到 example 层逐条 `set_xxx` |
| 编译期宏开关区分型号/容量 | 违反 §15 | 用运行时枚举 + 内部分支 |
| 错误日志直接 `printf` | 违反 §6.3 | 必须经 `debug_print` |

---

## 附录：187 仓库全量清单

> 清单由逐仓拉取 `src/driver_<name>.h` 并 grep 接口函数与芯片信息宏生成（187/187 可验证），分类按芯片功能判定。占位内容将在盘点完成后填充。

> **采集与验证方法**：全部 187 个仓库逐一拉取 `src/driver_<name>.h`（本地克隆 19 个 + raw.githubusercontent.com 168 个，0 失败），接口类型通过 grep 头文件中 `<chip>_interface_*` 函数指针声明判定（如 `iic_init`→iic、`spi_init`→spi、`uart_init`→uart、`bus_read`→one_wire、`*_gpio_*`→gpio 位拨、`contactless_*`→rfid、`smi_*`→smi、`adc_*`→adc），驱动对象取 `.c` 中 `CHIP_NAME` 宏，分类按芯片功能判定。一个仓库可同时支持多接口（如 ssd1306 = iic+spi、mfrc522 = iic+spi+uart+gpio）。

### 附录统计（187 个仓库）

| 分类 | 数量 | 分类 | 数量 |
|------|------|------|------|
| 环境传感（温湿度/气压/气体/光照/CO₂/PM/水质） | 82 | 显示屏与 LED | 18 |
| 运动与姿态（IMU/加速度/磁力/角度/光流） | 14 | UART 通信（指纹） | 1 |
| ADC/DAC/电源计量/模拟前端 | 17 | NFC/RFID/IC 卡 | 7 |
| 存储器（EEPROM/Flash/FRAM/字库） | 9 | 无线射频与网络（2.4G/LoRa/以太网/FM/雷击） | 11 |
| RTC 与时钟 | 5 | 电机/执行器/PWM/数字电位器 | 4 |
| 单总线/GPIO（1-Wire/按键/红外/超声/触摸） | 9 | 音频（录放/编解码/语音合成） | 9 |
| 待定（摄像头） | 1 | **合计** | **187** |

**接口类型分布**（按出现次数，一仓可多接口）：iic ×113、spi ×54、gpio ×39（含位拨串行/受控引脚）、uart ×21、one_wire ×6、rfid ×5（contactless 前端）、smi ×1（以太网 MII）、adc ×1（NTC 外部 ADC）。

### 全量清单（按仓库名字母序）

| 仓库名 | 分类 | 接口 | 驱动对象 | 主要功能 |
|--------|------|------|----------|----------|
| `ad7705` | ADC/DAC/电源计量 | spi | Analog Devices AD7705 | ADC 采集 |
| `ad9833` | ADC/DAC/电源计量 | spi | Analog Devices AD9833 | DDS 波形发生 |
| `adg728` | ADC/DAC/电源计量 | iic | Analog Devices ADG728 | 模拟开关矩阵/多路复用 |
| `ads1110` | ADC/DAC/电源计量 | iic | Texas Instruments ADS1110 | ADC 采集 |
| `ads1115` | ADC/DAC/电源计量 | iic | Texas Instruments ADS1115 | ADC 采集 |
| `ads1118` | ADC/DAC/电源计量 | spi | Texas Instruments ADS1118 | ADC 采集 |
| `adxl345` | 运动与姿态 | iic,spi | Analog Devices ADXL345 | 加速度计 |
| `adxl362` | 运动与姿态 | spi | Analog Devices ADXL362 | 加速度计 |
| `adxl375` | 运动与姿态 | iic,spi | Analog Devices ADXL375 | 加速度计 |
| `afs01` | 环境传感 | iic | ASAIR AFS01 | 气体流量 |
| `ags02ma` | 环境传感 | iic | ASAIR AGS02MA | TVOC |
| `ags10` | 环境传感 | iic | ASAIR AGS10 | TVOC |
| `ags10et` | 环境传感 | iic | ASAIR AGS10ET | 乙醇气体 |
| `aht10` | 环境传感 | iic | ASAIR AHT10 | 湿度、温度 |
| `aht20` | 环境传感 | iic | ASAIR AHT20 | 湿度、温度 |
| `aht21` | 环境传感 | iic | ASAIR AHT21 | 湿度、温度 |
| `aht25` | 环境传感 | iic | ASAIR AHT25 | 湿度、温度 |
| `aht30` | 环境传感 | iic | ASAIR AHT30 | 湿度、温度 |
| `aht40` | 环境传感 | iic | ASAIR AHT40 | 湿度 |
| `am2320` | 环境传感 | iic,one_wire | ASAIR AM2320 | 湿度、温度 |
| `amg8833` | 环境传感 | iic | Panasonic AMG8833 | 红外热成像（8×8） |
| `aox4000` | 环境传感 | uart | ASAIR AOX4000 | 溶解氧 |
| `apa102c` | 显示屏与LED | spi | Shiji Lighting APA102C | LED 灯带驱动 |
| `apds9960` | 环境传感 | iic | Broadcom APDS9960 | RGB 颜色、手势、接近检测 |
| `apm2000` | 环境传感 | iic,uart | ASAIR APM2000 | PM2.5 |
| `as3935` | 无线射频与网络 | iic,spi | AMS AS3935 | 雷击检测 |
| `as5600` | 运动与姿态 | iic | AMS AS5600 | 磁旋转编码器 |
| `as608` | UART通信 | uart | Synochip AS608 | 指纹识别 |
| `at24cxx` | 存储器 | iic | Microchip AT24CXX | EEPROM 存储 |
| `ba111` | 环境传感 | uart | AtomBit BA111 | TDS 水质 |
| `ba121` | 环境传感 | uart | AtomBit BA121 | 电导率 |
| `ba234` | 环境传感 | uart | AtomBit BA234 | TDS/电导率/盐度 |
| `bh1750fvi` | 环境传感 | iic | ROHM BH1750FVI | 光照强度 |
| `bme280` | 环境传感 | iic,spi | Bosch BME280 | 湿度、温度、气压 |
| `bme680` | 环境传感 | iic,spi | Bosch BME680 | 湿度、温度、气压、气体 |
| `bme688` | 环境传感 | iic,spi | Bosch BME688 | 湿度、温度、气压、气体 |
| `bme690` | 环境传感 | iic,spi | Bosch BME690 | 湿度、温度、气压、气体 |
| `bmm150` | 运动与姿态 | iic,spi | Bosch BMM150 | 磁力计 |
| `bmp180` | 环境传感 | iic | Bosch BMP180 | 气压 |
| `bmp280` | 环境传感 | iic,spi | Bosch BMP280 | 气压 |
| `bmp384` | 环境传感 | iic,spi | Bosch BMP384 | 气压 |
| `bmp388` | 环境传感 | iic,spi | Bosch BMP388 | 气压 |
| `bmp390` | 环境传感 | iic,spi | Bosch BMP390 | 气压 |
| `bpc` | 单总线/GPIO | gpio | China BPC | 长波授时码接收（68.5kHz） |
| `button` | 单总线/GPIO | gpio | General BUTTON | 按键输入 |
| `ccs811` | 环境传感 | iic | AMS CCS811 | CO₂、TVOC |
| `ch9120` | 无线射频与网络 | uart,gpio | WCH CH9120 | UART 转以太网 |
| `ch9121` | 无线射频与网络 | uart,gpio | WCH CH9121 | UART 转以太网 |
| `ch9121x` | 无线射频与网络 | uart,gpio | WCH CH9121X | UART 转以太网 |
| `cs100` | 单总线/GPIO | gpio | angoSense CS100 | 超声波测距 |
| `cs1237` | ADC/DAC/电源计量 | gpio | CHIPSEA CS1237 | 24 位 ADC 采集 |
| `cs1238` | ADC/DAC/电源计量 | gpio | CHIPSEA CS1238 | 24 位 ADC 采集 |
| `dht11` | 单总线/GPIO | one_wire | ASAIR DHT11 | 湿度、温度 |
| `dht20` | 环境传感 | iic | ASAIR DHT20 | 湿度、温度 |
| `ds1302` | RTC与时钟 | gpio | Maxim Integrated DS1302 | 实时时钟 |
| `ds1307` | RTC与时钟 | iic | Maxim Integrated DS1307 | 实时时钟 |
| `ds18b20` | 单总线/GPIO | one_wire | Maxim Integrated DS18B20 | 温度 |
| `ds2431` | 存储器 | one_wire | Maxim Integrated DS2431 | 1-Wire EEPROM |
| `ds3231` | RTC与时钟 | iic | Maxim Integrated DS3231 | 实时时钟 |
| `em4095` | NFC/RFID/IC卡 | gpio | EM Microelectronic EM4095 | RFID/NFC 读写 |
| `em4100` | NFC/RFID/IC卡 | rfid | EM Microelectronic EM4100 | RFID/NFC 读写 |
| `ens160` | 环境传感 | iic,spi | ScioSense ENS160 | CO₂、TVOC、空气质量 |
| `fm11rfxx` | NFC/RFID/IC卡 | rfid | FMSH FM11RFXX | RFID/NFC 读写 |
| `fm24clxx` | 存储器 | iic | Cypress FM24CLXX | 铁电存储(FRAM) |
| `gp2y1051au0f` | 环境传感 | uart | SHARP GP2Y1051AU0F | 粉尘浓度检测 |
| `gt24cxx` | 存储器 | iic | Giantec GT24CXX | EEPROM 存储 |
| `gt30l32s4w` | 存储器 | spi | Genitop GT30L32S4W | 字库存储 |
| `hcsr04` | 单总线/GPIO | gpio | JieShenna HCSR04 | 超声波测距 |
| `hdc1080` | 环境传感 | iic | Texas Instruments HDC1080 | 湿度、温度 |
| `hdc2080` | 环境传感 | iic | Texas Instruments HDC2080 | 湿度、温度 |
| `hdc302x` | 环境传感 | iic | Texas Instruments HDC302X | 湿度、温度 |
| `hlw8032` | ADC/DAC/电源计量 | uart | HLW TECHNOLOGY HLW8032 | 功率/电流监测、电能计量 |
| `hmc5883l` | 运动与姿态 | iic | Honeywell HMC5883L | 磁力计 |
| `ht162x` | 显示屏与LED | gpio | Holtek HT162X | LCD 段码/并口屏 |
| `htu21d` | 环境传感 | iic | TE HTU21D | 湿度、温度 |
| `htu31d` | 环境传感 | iic | TE HTU31D | 湿度、温度 |
| `hx711` | ADC/DAC/电源计量 | one_wire | Aviaic HX711 | 24 位 ADC（称重） |
| `ina219` | ADC/DAC/电源计量 | iic | Texas Instruments INA219 | 功率/电流监测 |
| `ina226` | ADC/DAC/电源计量 | iic | Texas Instruments INA226 | 功率/电流监测 |
| `ir_remote` | 单总线/GPIO | gpio | NEC IR REMOTE | 红外遥控接收（NEC 协议） |
| `isd17xx` | 音频 | spi | Nuvoton ISD17XX | 语音录放 |
| `jed1xx` | 环境传感 | iic | JXCT JED1XX | JXCT 传感器系列 |
| `l3gd20h` | 运动与姿态 | iic,spi | STMicroelectronics L3GD20H | 陀螺仪 |
| `lan8720` | 无线射频与网络 | smi,gpio | Microchip LAN8720 | 以太网 PHY |
| `ld3320` | 音频 | spi,gpio | IC Route LD3320 | 语音识别 |
| `llcc68` | 无线射频与网络 | spi,gpio | Semtech LLCC68 | LoRa 无线收发 |
| `lm75b` | 环境传感 | iic | NXP LM75B | 温度 |
| `m24cxx` | 存储器 | iic | STMicroelectronics M24CXX | EEPROM 存储 |
| `mag3110` | 运动与姿态 | iic | NXP MAG3110 | 磁力计 |
| `max30102` | 环境传感 | iic | Maxim Integrated MAX30102 | 心率/血氧(SpO₂)监测 |
| `max30105` | 环境传感 | iic | Maxim Integrated MAX30105 | 多波长光学传感 |
| `max30205` | 环境传感 | iic | Maxim Integrated MAX30205 | 体温 |
| `max31850` | 环境传感 | one_wire | Maxim Integrated MAX31850 | 热电偶温度（1-Wire） |
| `max31855` | 环境传感 | spi | Maxim Integrated MAX31855 | 热电偶温度（SPI） |
| `max31856` | 环境传感 | spi | Maxim Integrated MAX31856 | 热电偶温度（SPI） |
| `max31865` | 环境传感 | spi | Maxim Integrated MAX31865 | RTD 温度（SPI） |
| `max44009` | 环境传感 | iic | Analog Devices MAX44009 | 光照强度 |
| `max6675` | 环境传感 | spi | Maxim Integrated MAX6675 | 热电偶温度（SPI） |
| `max7219` | 显示屏与LED | spi | Maxim Integrated MAX7219 | 数码管/点阵 LED 驱动 |
| `mb85rcxx` | 存储器 | iic | RAMXEED MB85RCXX | 铁电存储(FRAM) |
| `mb85rsxx` | 存储器 | spi | RAMXEED MB85RSXX | 铁电存储(FRAM) |
| `mcp3421` | ADC/DAC/电源计量 | iic | Microchip MCP3421 | ADC 采集 |
| `mcp4725` | ADC/DAC/电源计量 | iic | Microchip MCP4725 | DAC 输出 |
| `mcp9600` | 环境传感 | iic | Microchip MCP9600 | 热电偶温度（I2C） |
| `mcp9808` | 环境传感 | iic | Microchip MCP9808 | 温度 |
| `mfrc522` | NFC/RFID/IC卡 | iic,spi,uart,gpio | NXP MFRC522 | RFID/NFC 读写 |
| `mifare_classic` | NFC/RFID/IC卡 | rfid | NXP MIFARE Classic EV1 | RFID/NFC 读写 |
| `mifare_ultralight` | NFC/RFID/IC卡 | rfid | NXP Ultralight | RFID/NFC 读写 |
| `mlx90614` | 环境传感 | iic | Melexis MLX90614 | 红外测温 |
| `mma7660fc` | 运动与姿态 | iic | NXP MMA7660FC | 加速度计 |
| `mpu6050` | 运动与姿态 | iic | TDK MPU6050 | 加速度、陀螺仪（IMU） |
| `mpu6500` | 运动与姿态 | iic,spi | TDK MPU6500 | 加速度、陀螺仪（IMU） |
| `mpu9250` | 运动与姿态 | iic,spi | TDK MPU9250 | 加速度、陀螺仪、磁力计（IMU） |
| `ms5611` | 环境传感 | iic,spi | TE MS5611 | 气压 |
| `ms5837` | 环境传感 | iic | TE MS5837 | 气压 |
| `multi_button` | 单总线/GPIO | gpio | General MULTI_BUTTON | 按键/矩阵按键 |
| `nrf24l01` | 无线射频与网络 | spi | Nordic nRF24L01 | 2.4G 无线收发 |
| `nrf905` | 无线射频与网络 | spi,gpio | Nordic nRF905 | Sub-1G 无线收发 |
| `ntag21x` | NFC/RFID/IC卡 | rfid | NXP NTAG213/215/216 | RFID/NFC 读写 |
| `ntc` | 环境传感 | adc | General NTC | 热敏电阻温度（外部 ADC） |
| `opt300x` | 环境传感 | iic | Texas Instruments OPT300X | 光照强度 |
| `ov2640` | 待定（摄像头） | iic | OmniVision OV2640 | 摄像头图像采集（DVP/SCCB） |
| `pca9548a` | ADC/DAC/电源计量 | iic,gpio | NXP PCA9548A | I2C 开关/多路复用 |
| `pca9685` | 显示屏与LED | iic,gpio | NXP PCA9685 | PWM LED 控制器 |
| `pcf8551a` | 显示屏与LED | iic | NXP PCF8551A | LCD 段码驱动 |
| `pcf8563` | RTC与时钟 | iic | NXP PCF8563 | 实时时钟 |
| `pcf8574` | 电机/执行器/PWM/电位器 | iic | NXP PCF8574 | IO 扩展 |
| `pcf8575` | 电机/执行器/PWM/电位器 | iic | Texas Instruments PCF8575 | IO 扩展 |
| `pcf8591` | ADC/DAC/电源计量 | iic | NXP PCF8591 | ADC 采集、DAC 输出 |
| `pms3003` | 环境传感 | uart,gpio | PLANTOWER PMS3003 | PM2.5/PM1.0/PM10 |
| `pmsx003` | 环境传感 | uart,gpio | PLANTOWER PMSX003 | PM2.5/PM1.0/PM10 |
| `pmw3901mb` | 运动与姿态 | spi,gpio | PixArt Imaging PMW3901MB | 光流 |
| `qmc5883l` | 运动与姿态 | iic | QST QMC5883L | 磁力计 |
| `rx8025t` | RTC与时钟 | iic | EPSON RX8025T | 实时时钟 |
| `scd30` | 环境传感 | iic,uart | Sensirion SCD30 | CO₂、温度、湿度 |
| `scd4x` | 环境传感 | iic | Sensirion SCD4X | CO₂、温度、湿度 |
| `sen5x` | 环境传感 | iic | Sensirion SEN5X | PM2.5 环境节点 |
| `sfa30` | 环境传感 | iic,uart | Sensirion SFA30 | 甲醛 |
| `sgp30` | 环境传感 | iic | Sensirion SGP30 | CO₂、TVOC |
| `sgp40` | 环境传感 | iic | Sensirion SGP40 | VOC |
| `sgp41` | 环境传感 | iic | Sensirion SGP41 | VOC、NOx |
| `sht2x` | 环境传感 | iic | Sensirion SHT2X | 湿度、温度 |
| `sht30` | 环境传感 | iic | Sensirion SHT30 | 湿度、温度 |
| `sht31` | 环境传感 | iic | Sensirion SHT31 | 湿度、温度 |
| `sht35` | 环境传感 | iic | Sensirion SHT35 | 湿度、温度 |
| `sht4x` | 环境传感 | iic | Sensirion SHT4X | 湿度、温度 |
| `sht85` | 环境传感 | iic | Sensirion SHT85 | 湿度、温度 |
| `shtc3` | 环境传感 | iic | Sensirion SHTC3 | 湿度、温度 |
| `si7021` | 环境传感 | iic | Silicon Labs SI7021 | 湿度、温度 |
| `sps30` | 环境传感 | iic,uart | Sensirion SPS30 | PM2.5 |
| `ssd1306` | 显示屏与LED | iic,spi,gpio | Solomon Systech SSD1306 | OLED 显示 |
| `ssd1309` | 显示屏与LED | iic,spi,gpio | Solomon Systech SSD1309 | OLED 显示 |
| `ssd1315` | 显示屏与LED | iic,spi,gpio | Solomon Systech SSD1315 | OLED 显示 |
| `ssd1351` | 显示屏与LED | spi,gpio | Solomon Systech SSD1351 | OLED 显示 |
| `ssd1681` | 显示屏与LED | spi,gpio | Solomon Systech SSD1681 | 电子纸显示 |
| `st7789` | 显示屏与LED | spi,gpio | Sitronix ST7789 | TFT 显示 |
| `st7920` | 显示屏与LED | gpio | Sitronix ST7920 | LCD 段码/并口屏 |
| `stcc4` | 环境传感 | iic | Sensirion STCC4 | 环境传感器 |
| `sts21` | 环境传感 | iic | Sensirion STS21 | 温度 |
| `sts3x` | 环境传感 | iic | Sensirion STS3X | 温度 |
| `sts4x` | 环境传感 | iic | Sensirion STS4X | 温度 |
| `stts22h` | 环境传感 | iic | STMicroelectronics STTS22H | 温度 |
| `sx1262` | 无线射频与网络 | spi,gpio | Semtech SX1262 | LoRa 无线收发 |
| `sx1268` | 无线射频与网络 | spi,gpio | Semtech SX1268 | LoRa 无线收发 |
| `syn6288` | 音频 | uart | YuToneWorld SYN6288 | 语音合成(TTS) |
| `syn6288e` | 音频 | uart | YuToneWorld SYN6288E | 语音合成(TTS) |
| `syn6658` | 音频 | spi,uart | Voicetx SYN6658 | 语音合成(TTS) |
| `syn6988` | 音频 | spi,uart | Voicetx SYN6988 | 语音合成(TTS) |
| `tcs34725` | 环境传感 | iic | AMS TCS34725 | 光照、RGB 颜色 |
| `tea5767` | 无线射频与网络 | iic | NXP TEA5767 | FM 收音 |
| `tlc5615` | ADC/DAC/电源计量 | spi | Texas Instruments TLC5615 | DAC 输出 |
| `tm1621x` | 显示屏与LED | gpio | Titan Micro TM1621X | LCD 段码/并口屏 |
| `tm1622` | 显示屏与LED | gpio | Titan Micro TM1622 | LCD 段码/并口屏 |
| `tm1637` | 显示屏与LED | iic | Titan Micro TM1637 | 数码管驱动、按键扫描 |
| `tm1638` | 显示屏与LED | spi | Titan Micro TM1638 | 数码管驱动、按键扫描 |
| `tm1640` | 显示屏与LED | gpio | Titan Micro TM1640 | 数码管驱动 |
| `tpl0501` | 电机/执行器/PWM/电位器 | spi | Texas Instruments TPL0501 | 数字电位器 |
| `tsl2561` | 环境传感 | iic | AMS TSL2561 | 光照强度 |
| `ttp229` | 单总线/GPIO | iic,spi | Tontek TTP229 | 触摸按键 |
| `uvis25` | 环境传感 | iic,spi | STMicroelectronics UVIS25 | 紫外线强度 |
| `veml7700` | 环境传感 | iic | VISHAY VEML7700 | 光照强度 |
| `vs1053b` | 音频 | spi,gpio | VLSI VS1053B | 音频解码播放 |
| `w25qxx` | 存储器 | spi | Winbond W25QXX | Flash 存储 |
| `wm8978` | 音频 | iic | Cirrus Logic WM8978 | 音频编解码 |
| `ws2812b` | 显示屏与LED | spi | Worldsemi WS2812B | LED 灯带驱动（SPI 合成时序） |
| `wt588e02b` | 音频 | gpio | Waytronic WT588E02B | 语音播放 |
| `x9cxx` | 电机/执行器/PWM/电位器 | gpio | Renesas X9CXX | 数字电位器 |

> 注：`ov2640`（数字摄像头，DVP/SCCB）在 12 类中无对应类别，暂列待定；`bpc/button/ir_remote` 仅用 GPIO 输入+中断（`timestamp_read`），`em4100/fm11rfxx/mifare_*/ntag21x` 使用 contactless 无线前端抽象（无物理总线），`lan8720` 使用 MII 管理接口（smi），`ntc` 使用外部 ADC，`ds1302/x9cxx/st7920/tm1621x/tm1622/tm1640/ht162x/wt588e02b/cs1237/cs1238/em4095` 使用位拨 GPIO 模拟串行时序——均为代码实测，非推断。
