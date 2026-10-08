/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724.h
 * @brief     driver c8724 header file
 * @version   1.4.0
 * @author    LQ
 * @date      2026-10-05
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author  <th>Description
 * <tr><td>2026/10/05  <td>1.0.0    <td>LQ      <td>first upload
 * <tr><td>2026/10/05  <td>1.1.0    <td>LQ      <td>config API reduced to set/get plus field macros, frame buffer added
 * <tr><td>2026/10/08  <td>1.2.0    <td>LQ      <td>instruction byte fixed (broadcast B6, chip select B5:B4), C8724_CONFIG_PROVEN preset added
 * <tr><td>2026/10/08  <td>1.3.0    <td>LQ      <td>set_config reduced to handle-only (shadow edited via Set* helpers), RCS/GCC docs corrected, SEG11/12 data columns masked, packets batched
 * <tr><td>2026/10/08  <td>1.4.0    <td>LQ      <td>module renamed to c8724, function naming unified, comments normalized
 * </table>
 */

#ifndef DRIVER_C8724_H
#define DRIVER_C8724_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup c8724_driver c8724 driver function
 * @brief    c8724 driver modules
 * @{
 */

#define C8724_DISPLAY_WIDTH                          12U /**< number of SEG channels */
#define C8724_DISPLAY_HEIGHT                         8U  /**< number of GRID channels */
#define C8724_DISPLAY_DATA_MAX                       96U /**< maximum sequential PWM data bytes */
#define C8724_ADDRESS_DATA_MAX                       72U /**< maximum addressed PWM data entries */
#define C8724_DRIVER_VERSION                         1400U /**< driver version 1.4.0 */

/**
 * ============================================================================
 * C8724 设置寄存器位定义表
 * ============================================================================
 *
 * 设置寄存器 1（默认值 0x3F）
 * | Bit  | 名称       | 默认值 | 说明                                             |
 * |------|------------|--------|--------------------------------------------------|
 * | B7   | SEG12_CS   | 0      | 0: SEG12 作 CS1 输入；1: SEG12 恒流输出          |
 * | B6   | SEG11_CS   | 0      | 0: SEG11 作 CS0 输入；1: SEG11 恒流输出          |
 * | B5:0 | GCC        | 111111 | 全局电流增益，ISG = 1000 × (1 / (17 + GCC)) mA   |
 *
 * > ⚠ GCC 电流公式与手册自相矛盾：公式给出随 GCC 递减（63→12.5、0→58.8），
 * > 同页示例却写 63→40mA、0→8.5mA（递增），电气表在 @GCC=63 给出 8.5/20/40mA
 * > （min/typ/max）。极性未经实测验证——实证配置 GCC=63 可正常点亮，
 * > 请勿按公式推算绝对电流。
 *
 * 设置寄存器 2（默认值 0x00）
 * | Bit  | 名称       | 默认值 | 说明                                             |
 * |------|------------|--------|--------------------------------------------------|
 * | B7   | CKS        | 0      | 0: 内部时钟 RCLK；1: 外部时钟 CLK                |
 * | B6   | 保留       | 0      | --                                               |
 * | B5   | 保留       | 0      | --                                               |
 * | B4:2 | SCAN       | 000    | 行扫描设置，M = SCAN + 1，N = 12 × M             |
 * | B1   | WM         | 0      | 测试模式使能，正常显示不可置 1                   |
 * | B0   | VSYN_M     | 0      | 0: 强制更新；1: 芯片自动更新                     |
 *
 * 设置寄存器 3（默认值 0xF0）
 * | Bit  | 名称       | 默认值 | 说明                                             |
 * |------|------------|--------|--------------------------------------------------|
 * | B7   | OTP1       | 1      | 0: 过温保护 1 打开（125°C）；1: 关闭              |
 * | B6   | RCS        | 1      | 0: 内部设置电阻（手册要求固定为 0）；1: 外部电阻——本 24 脚封装无 ISET 脚，恒流基准不建立、必黑屏 |
 * | B5:4 | RCKS       | 11     | 00: 1MHz；01: 2MHz；10: 4MHz；11: 8MHz           |
 * | B3   | OE         | 0      | 0: SEG 输出关闭；1: SEG 输出打开                 |
 * | B2   | DGH        | 0      | 0: 消除鬼隐弱；1: 消除鬼隐强                     |
 * | B1:0 | TLS        | 00     | 00: 4T；01: 8T；10: 12T；11: 16T                 |
 *
 * 设置寄存器 4（默认值 0x02）
 * | Bit  | 名称       | 默认值 | 说明                                             |
 * |------|------------|--------|--------------------------------------------------|
 * | B7   | 保留       | 0      | --                                               |
 * | B6   | SLEEP_EN   | 0      | 休眠模式使能，0: 不使能；1: 使能                 |
 * | B5   | SLEEP_AT   | 0      | 自动休眠使能，0: 不使能；1: 使能                 |
 * | B4   | GRST       | 0      | 全局复位使能，0: 不使能；1: 使能                 |
 * | B3   | SRST       | 0      | 软复位使能，0: 不使能；1: 使能                   |
 * | B2   | CLK2X      | 0      | 扫描时钟 2 倍频使能，0: 不使能；1: 使能          |
 * | B1   | OTP2       | 1      | 0: 过温保护 2 打开（150°C）；1: 关闭              |
 * | B0   | PDR        | 0      | 掉电复位功能，0: 不使能；1: 使能                 |
 * ============================================================================
 */

/**
 * @brief      顺序写入方向（SRAM 地址自动递增）
 *
 * @details    数组标号映射：
 *                     SEG1     SEG2     SEG3     ...   SEG11    SEG12
 *            GRID1    [0]      [1]      [2]      ...   [10]     [11]
 *            GRID2    [12]     [13]     [14]     ...   [22]     [23]
 *            GRID3    [24]     [25]     [26]     ...   [34]     [35]
 *            GRID4    [36]     [37]     [38]     ...   [46]     [47]
 *            GRID5    [48]     [49]     [50]     ...   [58]     [59]
 *            GRID6    [60]     [61]     [62]     ...   [70]     [71]
 *            GRID7    [72]     [73]     [74]     ...   [82]     [83]
 *            GRID8    [84]     [85]     [86]     ...   [94]     [95]
 *
 *             写入方向：
 *            GRID1: [0] → [1] → ... → [11]
 *                      ↓
 *            GRID2: [12] → [13] → ... → [23]
 *                      ↓
 *            GRID3: [24] → [25] → ... → [35]
 *                      ↓
 *                    ...
 *                      ↓
 *            GRID8: [84] → [85] → ... → [95]
 *
 *             索引公式：index = GRID_index * 12 + SEG_index
 *                       GRID_index = 0~7  对应 GRID1~GRID8
 *                       SEG_index  = 0~11 对应 SEG1~SEG12
 *
 *             M = SCAN + 1，len = 12 * M
 *             只发送前 12 * M 个元素，即 [0] ~ [12*M - 1]
 */

/* ================= 设置寄存器 1 ================= */
#define C8724_REG1_SEG12_CS_POS       7
#define C8724_REG1_SEG12_CS_MASK      (0x01U << C8724_REG1_SEG12_CS_POS) /**< SEG12 功能位掩码 */
#define C8724_REG1_SEG11_CS_POS       6
#define C8724_REG1_SEG11_CS_MASK      (0x01U << C8724_REG1_SEG11_CS_POS) /**< SEG11 功能位掩码 */
#define C8724_REG1_GCC_POS            0
#define C8724_REG1_GCC_MASK           (0x3FU << C8724_REG1_GCC_POS)      /**< 全局电流增益位掩码 */

/* ================= 设置寄存器 2 ================= */
#define C8724_REG2_CKS_POS            7
#define C8724_REG2_CKS_MASK           (0x01U << C8724_REG2_CKS_POS)      /**< 时钟源选择位掩码 */
#define C8724_REG2_SCAN_POS           2
#define C8724_REG2_SCAN_MASK          (0x07U << C8724_REG2_SCAN_POS)     /**< 有效 GRID 数量位掩码 */
#define C8724_REG2_WM_POS             1
#define C8724_REG2_WM_MASK            (0x01U << C8724_REG2_WM_POS)       /**< 测试模式位掩码 */
#define C8724_REG2_VSYN_M_POS         0
#define C8724_REG2_VSYN_M_MASK        (0x01U << C8724_REG2_VSYN_M_POS)   /**< 显示更新模式位掩码 */

/* ================= 设置寄存器 3 ================= */
#define C8724_REG3_OTP1_POS           7
#define C8724_REG3_OTP1_MASK          (0x01U << C8724_REG3_OTP1_POS)     /**< 125 度过温保护位掩码 */
#define C8724_REG3_RCS_POS            6
#define C8724_REG3_RCS_MASK           (0x01U << C8724_REG3_RCS_POS)      /**< 电阻选择位掩码 */
#define C8724_REG3_RCKS_POS           4
#define C8724_REG3_RCKS_MASK          (0x03U << C8724_REG3_RCKS_POS)     /**< 内部时钟频率位掩码 */
#define C8724_REG3_OE_POS             3
#define C8724_REG3_OE_MASK            (0x01U << C8724_REG3_OE_POS)       /**< SEG 输出使能位掩码 */
#define C8724_REG3_DGH_POS            2
#define C8724_REG3_DGH_MASK           (0x01U << C8724_REG3_DGH_POS)      /**< 消影强度位掩码 */
#define C8724_REG3_TLS_POS            0
#define C8724_REG3_TLS_MASK           (0x03U << C8724_REG3_TLS_POS)      /**< 换行时间位掩码 */

/* ================= 设置寄存器 4 ================= */
#define C8724_REG4_SLEEP_EN_POS       6
#define C8724_REG4_SLEEP_EN_MASK      (0x01U << C8724_REG4_SLEEP_EN_POS) /**< 休眠模式使能位掩码 */
#define C8724_REG4_SLEEP_AT_POS       5
#define C8724_REG4_SLEEP_AT_MASK      (0x01U << C8724_REG4_SLEEP_AT_POS) /**< 自动休眠使能位掩码 */
#define C8724_REG4_GRST_POS           4
#define C8724_REG4_GRST_MASK          (0x01U << C8724_REG4_GRST_POS)     /**< 全局复位使能位掩码 */
#define C8724_REG4_SRST_POS           3
#define C8724_REG4_SRST_MASK          (0x01U << C8724_REG4_SRST_POS)     /**< 软复位使能位掩码 */
#define C8724_REG4_CLK2X_POS          2
#define C8724_REG4_CLK2X_MASK         (0x01U << C8724_REG4_CLK2X_POS)    /**< 扫描时钟 2 倍频位掩码 */
#define C8724_REG4_OTP2_POS           1
#define C8724_REG4_OTP2_MASK          (0x01U << C8724_REG4_OTP2_POS)     /**< 150 度过温保护位掩码 */
#define C8724_REG4_PDR_POS            0
#define C8724_REG4_PDR_MASK           (0x01U << C8724_REG4_PDR_POS)      /**< 掉电复位位掩码 */

/**
 * @brief c8724 boolean enumeration definition
 */
typedef enum
{
    C8724_BOOL_FALSE = 0x00, /**< disable function */
    C8724_BOOL_TRUE  = 0x01  /**< enable function */
} c8724_bool_t;

/**
 * @brief c8724 instruction address enumeration definition
 */
typedef enum
{
    C8724_ADDRESS_0 = 0x00, /**< chip select address 00 */
    C8724_ADDRESS_1 = 0x01, /**< chip select address 01 */
    C8724_ADDRESS_2 = 0x02, /**< chip select address 10 */
    C8724_ADDRESS_3 = 0x03  /**< chip select address 11 */
} c8724_address_t;

/**
 * @brief c8724 instruction command code enumeration definition
 */
typedef enum
{
    C8724_COMMAND_CONFIG               = 0x01U, /**< configuration data write instruction */
    C8724_COMMAND_DISPLAY_SEQUENTIAL   = 0x02U, /**< sequential display data write instruction */
    C8724_COMMAND_DISPLAY_ADDRESS      = 0x03U, /**< addressed display data write instruction */
    C8724_COMMAND_DISPLAY_UPDATE       = 0x04U, /**< display data update instruction */
    C8724_COMMAND_SOFT_RESET           = 0x08U, /**< soft reset instruction */
    C8724_COMMAND_SLEEP                = 0x0EU, /**< sleep instruction */
    C8724_COMMAND_WAKE                 = 0x0DU, /**< wake instruction */
    C8724_COMMAND_GLOBAL_RESET         = 0x0BU  /**< global reset instruction */
} c8724_command_t;

/**
 * @brief c8724 SEG pin function enumeration definition
 */
typedef enum
{
    C8724_SEG_PIN_CHIP_SELECT = 0x00, /**< SEG pin used as chip select input */
    C8724_SEG_PIN_LED_OUTPUT  = 0x01  /**< SEG pin used as LED output */
} c8724_seg_pin_mode_t;

/**
 * @brief [reg2] c8724 clock source enumeration definition
 */
typedef enum
{
    C8724_CLOCK_SOURCE_INTERNAL = 0x00, /**< use internal RCLK */
    C8724_CLOCK_SOURCE_EXTERNAL = 0x01  /**< use external CLK */
} c8724_clock_source_t;

/**
 * @brief [reg2] c8724 scan row enumeration definition
 */
typedef enum
{
    C8724_SCAN_1_ROW  = 0x00, /**< enable GRID1 */
    C8724_SCAN_2_ROWS = 0x01, /**< enable GRID1 through GRID2 */
    C8724_SCAN_3_ROWS = 0x02, /**< enable GRID1 through GRID3 */
    C8724_SCAN_4_ROWS = 0x03, /**< enable GRID1 through GRID4 */
    C8724_SCAN_5_ROWS = 0x04, /**< enable GRID1 through GRID5 */
    C8724_SCAN_6_ROWS = 0x05, /**< enable GRID1 through GRID6 */
    C8724_SCAN_7_ROWS = 0x06, /**< enable GRID1 through GRID7 */
    C8724_SCAN_8_ROWS = 0x07  /**< enable GRID1 through GRID8 */
} c8724_scan_t;


typedef enum
{
    C8724_RES_SELECT_INTERNAL = 0,
} c8724_ResistorSelect_t;


/**
 * @brief [reg2] c8724 display update mode enumeration definition
 */
typedef enum
{
    C8724_UPDATE_MODE_FORCE    = 0x00, /**< update on the next CLK */
    C8724_UPDATE_MODE_SCAN_END = 0x01  /**< update after a complete scan */
} c8724_update_mode_t;

/**
 * @brief [reg3] c8724 internal clock frequency enumeration definition
 */
typedef enum
{
    C8724_INTERNAL_CLOCK_1_MHZ = 0x00, /**< 1 MHz */
    C8724_INTERNAL_CLOCK_2_MHZ = 0x01, /**< 2 MHz */
    C8724_INTERNAL_CLOCK_4_MHZ = 0x02, /**< 4 MHz */
    C8724_INTERNAL_CLOCK_8_MHZ = 0x03  /**< 8 MHz */
} c8724_internal_clock_t;

/**
 * @brief c8724 ghost removal strength enumeration definition
 */
typedef enum
{
    C8724_GHOST_REMOVAL_WEAK   = 0x00, /**< weak ghost removal */
    C8724_GHOST_REMOVAL_STRONG = 0x01  /**< strong ghost removal */
} c8724_ghost_removal_t;

/**
 * @brief c8724 line blanking time enumeration definition
 */
typedef enum
{
    C8724_LINE_BLANKING_4T  = 0x00, /**< 4 PWM clock periods */
    C8724_LINE_BLANKING_8T  = 0x01, /**< 8 PWM clock periods */
    C8724_LINE_BLANKING_12T = 0x02, /**< 12 PWM clock periods */
    C8724_LINE_BLANKING_16T = 0x03  /**< 16 PWM clock periods */
} c8724_line_blanking_t;

/**
 * @brief c8724 static information structure
 */
typedef struct c8724_info_s
{
    char     chip_name[32];          /**< chip name */
    char     manufacturer_name[32];  /**< manufacturer name */
    char     interface[8];           /**< chip interface name */
    float    supply_voltage_min_v;   /**< chip minimum supply voltage */
    float    supply_voltage_max_v;   /**< chip maximum supply voltage */
    float    max_current_ma;         /**< maximum SEG output current */
    float    temperature_min;        /**< minimum operating temperature */
    float    temperature_max;        /**< maximum operating temperature */
    uint32_t driver_version;         /**< driver version */
} c8724_info_t;

/**
 * @brief c8724 handle structure
 */
typedef struct c8724_handle_s
{
    uint8_t (*spi_init)(void);                                  /**< point to a spi_init function address */
    uint8_t (*spi_deinit)(void);                                /**< point to a spi_deinit function address */
    uint8_t (*spi_write_cmd)(const uint8_t *buf, uint16_t len); /**< point to a spi_write_cmd function address */
    void    (*delay_ms)(uint32_t ms);                           /**< point to a delay_ms function address */
    void    (*debug_print)(const char *const fmt, ...);         /**< point to a debug_print function address */
    uint8_t config[4];                                          /**< configuration registers mirroring the chip */
    uint8_t frame[C8724_DISPLAY_DATA_MAX];                      /**< pending display frame in GRID-major order */
    uint8_t address;                                            /**< instruction chip select address */
    uint8_t broadcast;                                          /**< broadcast instruction selection */
    uint8_t inited;                                             /**< initialized flag */
} c8724_handle_t;

/**
 * @defgroup c8724_link_driver c8724 link driver function
 * @brief    c8724 link driver modules
 * @{
 */

#define DRIVER_C8724_LINK_INIT(handle, structure)          memset((handle), 0, sizeof(structure))
#define DRIVER_C8724_LINK_SPI_INIT(handle, f)              ((handle)->spi_init = (f))
#define DRIVER_C8724_LINK_SPI_DEINIT(handle, f)            ((handle)->spi_deinit = (f))
#define DRIVER_C8724_LINK_SPI_WRITE_CMD(handle, f)         ((handle)->spi_write_cmd = (f))
#define DRIVER_C8724_LINK_DELAY_MS(handle, f)              ((handle)->delay_ms = (f))
#define DRIVER_C8724_LINK_DEBUG_PRINT(handle, f)           ((handle)->debug_print = (f))

/** @} */

/**
 * @defgroup c8724_base_driver c8724 base driver function
 * @brief    c8724 base driver modules
 * @{
 */

/**
 * @brief      获取芯片信息
 * @param[out] info 指向 c8724_info_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 2 info 为空
 * @note       无
 */
uint8_t c8724_info(c8724_info_t *info);

/**
 * @brief      初始化芯片
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 为空
 *             - 3 回调函数为空或芯片已初始化
 * @note       SPI 初始化后发送广播全局复位，并把手册上电默认值载入影子寄存器
 */
uint8_t c8724_init(c8724_handle_t *handle);

/**
 * @brief      反初始化芯片
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 * @note       清空当前帧缓冲，使能休眠，发送休眠指令，最后反初始化 SPI
 */
uint8_t c8724_deinit(c8724_handle_t *handle);

/**
 * @brief      设置指令片选地址
 * @param[in]  handle  指向 c8724_handle_t 结构体的指针
 * @param[in]  address 片选地址，0~3
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 *             - 4 address 无效
 * @note       这是指令地址，不是 MCU 的 GPIO
 */
uint8_t c8724_set_address(c8724_handle_t *handle, c8724_address_t address);

/**
 * @brief      获取指令片选地址
 * @param[in]  handle  指向 c8724_handle_t 结构体的指针
 * @param[out] address 指向片选地址的指针
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 *             - 4 address 无效
 * @note       返回软件选择的指令地址
 */
uint8_t c8724_get_address(c8724_handle_t *handle, c8724_address_t *address);

/**
 * @brief      设置广播指令选择
 * @param[in]  handle  指向 c8724_handle_t 结构体的指针
 * @param[in]  enable  广播使能值
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 *             - 4 enable 无效
 * @note       广播指令不受芯片地址限制，所有芯片均响应
 */
uint8_t c8724_set_broadcast(c8724_handle_t *handle, c8724_bool_t enable);

/**
 * @brief      获取广播指令选择
 * @param[in]  handle  指向 c8724_handle_t 结构体的指针
 * @param[out] enable  指向广播使能值的指针
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 *             - 4 enable 无效
 * @note       返回软件选择的广播模式
 */
uint8_t c8724_get_broadcast(c8724_handle_t *handle, c8724_bool_t *enable);

/** @} */

/**
 * @defgroup c8724_config_driver c8724 configuration driver function
 * @brief    c8724 configuration driver modules
 * @{
 */

/**
 * @brief      发送 4 个配置寄存器到芯片
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 *             - 4 handle->config 含保留位
 * @note       修改直接在 handle->config（影子副本）上进行，可用 C8724_Set*
 *             系列内联函数；整套影子值先校验保留位，再作为一包发送
 *             （指令校验 + 4 个寄存器 + 寄存器校验）。SPI 失败时芯片可能已
 *             锁存了半包，而 handle->config 仍保持修改值，调用者应重试或
 *             重新初始化
 */
uint8_t c8724_set_config(c8724_handle_t *handle);

/** @} */

/**
 * @defgroup c8724_display_driver c8724 display driver function
 * @brief    c8724 display driver modules
 * @{
 */

/**
 * @brief      写入顺序显示 PWM 数据
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @param[in]  data   指向 PWM 数据缓冲区的指针
 * @param[in]  len    PWM 字节数
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 *             - 4 data 为空或长度与 SCAN 不匹配
 * @note       数据顺序为 GRID1 SEG1-12、GRID2 SEG1-12，依此类推；当 REG1
 *             把 SEG11/SEG12 配置为片选输入时，对应的数据列在发送前被
 *             强制为 0（手册规定），校验和按实际发送的字节计算。数据包按
 *             头部 / 数据 / 校验 分事务发送——字节间空隙无害（CF 参考实现
 *             已验证）。若 SPI 在包中途失败，芯片可能停留在接收模式，
 *             字节流在下一次 c8724_init / 全局复位前保持失步
 */
uint8_t c8724_write_display(c8724_handle_t *handle, const uint8_t *data, uint16_t len);

/**
 * @brief      写入按地址显示 PWM 数据
 * @param[in]  handle  指向 c8724_handle_t 结构体的指针
 * @param[in]  address 指向 SRAM 地址的指针
 * @param[in]  data    指向 PWM 数据缓冲区的指针
 * @param[in]  len     地址与数据条目数
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 *             - 4 指针、长度或 SRAM 地址无效
 * @note       最多 72 条目；地址 = (GRID 索引 << 4) | SEG 索引。整个数据包
 *             （头部 + 数据对 + 0xFF 结束符 + 校验）在一个缓冲区中组装后
 *             单次发送；若 SPI 在包中途失败，芯片可能停留在接收模式，字节流
 *             在下一次 c8724_init / 全局复位前保持失步
 */
uint8_t c8724_write_display_address(c8724_handle_t *handle, const uint8_t *address,
                                    const uint8_t *data, uint16_t len);

/**
 * @brief      用待发送 SRAM 帧更新显示
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 * @note       无
 */
uint8_t c8724_display_update(c8724_handle_t *handle);

/**
 * @brief      设置帧缓冲中一个像素的 PWM 值
 * @param[in]  handle  指向 c8724_handle_t 结构体的指针
 * @param[in]  grid    GRID 索引，0~7
 * @param[in]  segment SEG 索引，0~11
 * @param[in]  pwm     PWM 值，0~255
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 *             - 4 grid 或 segment 越界
 * @note       仅修改帧缓冲；调用 c8724_display_flush 后才会显示
 */
uint8_t c8724_display_set_pixel(c8724_handle_t *handle, uint8_t grid, uint8_t segment, uint8_t pwm);

/**
 * @brief      从帧缓冲读取一个像素的 PWM 值
 * @param[in]  handle  指向 c8724_handle_t 结构体的指针
 * @param[in]  grid    GRID 索引，0~7
 * @param[in]  segment SEG 索引，0~11
 * @param[out] pwm     指向 PWM 输出值的指针
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 *             - 4 grid、segment 或指针无效
 * @note       无
 */
uint8_t c8724_display_get_pixel(c8724_handle_t *handle, uint8_t grid, uint8_t segment, uint8_t *pwm);

/**
 * @brief      用同一 PWM 值填满整个帧缓冲
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @param[in]  pwm    PWM 值，0~255
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 * @note       仅修改帧缓冲；调用 c8724_display_flush 后才会显示
 */
uint8_t c8724_display_fill(c8724_handle_t *handle, uint8_t pwm);

/**
 * @brief      把帧缓冲发送给芯片并切换到该帧
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 * @note       用一次顺序写入发送 SCAN 使能的行，然后发送显示更新指令
 */
uint8_t c8724_display_flush(c8724_handle_t *handle);

/**
 * @brief      清空帧缓冲和当前显示帧
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 * @note       把帧缓冲填为 0 并刷新
 */
uint8_t c8724_display_clear(c8724_handle_t *handle);

/**
 * @brief      发送软复位指令
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 * @note       不复位配置寄存器
 */
uint8_t c8724_soft_reset(c8724_handle_t *handle);

/**
 * @brief      发送全局复位指令
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 * @note       复位配置寄存器、关闭输出，并把手册默认值载入影子寄存器
 */
uint8_t c8724_global_reset(c8724_handle_t *handle);

/**
 * @brief      发送休眠指令
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 *             - 4 配置寄存器 4 中休眠模式未使能
 * @note       调用者必须在休眠前写入全 0 帧
 */
uint8_t c8724_sleep(c8724_handle_t *handle);

/**
 * @brief      发送唤醒指令
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 为空
 *             - 3 芯片未初始化
 * @note       无
 */
uint8_t c8724_wake(c8724_handle_t *handle);

/** @} */

/* ================= 设置寄存器 1 ================= */

/**
 * @brief 设置 SEG12 引脚功能模式
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param mode   引脚模式：C8724_SEG_PIN_CHIP_SELECT 或 C8724_SEG_PIN_LED_OUTPUT
 */
static inline void c8724_SetSeg12PinMode(c8724_handle_t *handle, c8724_seg_pin_mode_t mode)
{
    if (mode)
        handle->config[0] |=  C8724_REG1_SEG12_CS_MASK;
    else
        handle->config[0] &= ~C8724_REG1_SEG12_CS_MASK;
}

/**
 * @brief 获取 SEG12 引脚功能模式
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return 当前引脚模式：C8724_SEG_PIN_CHIP_SELECT 或 C8724_SEG_PIN_LED_OUTPUT
 */
static inline c8724_seg_pin_mode_t c8724_GetSeg12PinMode(const c8724_handle_t *handle)
{
    return (handle->config[0] & C8724_REG1_SEG12_CS_MASK)
           ? C8724_SEG_PIN_LED_OUTPUT : C8724_SEG_PIN_CHIP_SELECT;
}

/**
 * @brief 设置 SEG11 引脚功能模式
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param mode   引脚模式：C8724_SEG_PIN_CHIP_SELECT 或 C8724_SEG_PIN_LED_OUTPUT
 */
static inline void c8724_SetSeg11PinMode(c8724_handle_t *handle, c8724_seg_pin_mode_t mode)
{
    if (mode)
        handle->config[0] |=  C8724_REG1_SEG11_CS_MASK;
    else
        handle->config[0] &= ~C8724_REG1_SEG11_CS_MASK;
}

/**
 * @brief 获取 SEG11 引脚功能模式
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return 当前引脚模式：C8724_SEG_PIN_CHIP_SELECT 或 C8724_SEG_PIN_LED_OUTPUT
 */
static inline c8724_seg_pin_mode_t c8724_GetSeg11PinMode(const c8724_handle_t *handle)
{
    return (handle->config[0] & C8724_REG1_SEG11_CS_MASK)
           ? C8724_SEG_PIN_LED_OUTPUT : C8724_SEG_PIN_CHIP_SELECT;
}

/**
 * @brief 设置全局电流增益 GCC
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param gcc    增益值，范围 0~63
 * @note  手册公式 ISG = (17 + GCC)/2 mA 与其自身示例/电气表矛盾
 *        （详见寄存器 1 说明的警告），电流极性未经实测验证；
 *        GCC=63（默认、实证配置值）已确认可正常点亮
 */
static inline void c8724_SetGcc(c8724_handle_t *handle, uint8_t gcc)
{
    handle->config[0] &= (uint8_t)~C8724_REG1_GCC_MASK;
    handle->config[0] |= (uint8_t)((gcc << C8724_REG1_GCC_POS) & C8724_REG1_GCC_MASK);
}

/**
 * @brief       以毫安为单位设置全局电流增益 GCC
 * @param[in]   handle 指向 c8724_handle_t 结构体的指针
 * @param[in]   ma     目标恒流输出电流，单位 mA，有效范围 9 ~ 40
 * @note        输入超出 [9, 40] 时钳位到边界；由于参数类型为 uint8_t，
 *              最小只能表示 9 mA（GCC=1），无法表示手册最小值
 *              8.5 mA（GCC=0）。如需 8.5 mA 请直接调用
 *              c8724_SetGcc(handle, 0)
 */
static inline void c8724_SetGccMa(c8724_handle_t *handle, uint8_t ma)
{
    const uint8_t ma_min = 9U;   /* 对应 GCC = 1 */
    const uint8_t ma_max = 40U;  /* 对应 GCC = 63 */

    if (ma < ma_min)
    {
        ma = ma_min;
    }
    else if (ma > ma_max)
    {
        ma = ma_max;
    }

    /* GCC = 2 × ma - 17，由 ISG[mA] = (GCC + 17) / 2 反解 */
    c8724_SetGcc(handle, (uint8_t)(2U * ma - 17U));
}

/**
 * @brief 获取全局电流增益 GCC
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return 当前增益值，范围 0~63
 */
static inline uint8_t c8724_GetGcc(const c8724_handle_t *handle)
{
    return (uint8_t)((handle->config[0] & C8724_REG1_GCC_MASK) >> C8724_REG1_GCC_POS);
}

/* ================= 设置寄存器 2 ================= */

/**
 * @brief 设置时钟源
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param source 时钟源：C8724_CLOCK_SOURCE_INTERNAL 或 C8724_CLOCK_SOURCE_EXTERNAL
 */
static inline void c8724_SetClockSource(c8724_handle_t *handle, c8724_clock_source_t source)
{
    if (source)
        handle->config[1] |=  C8724_REG2_CKS_MASK;
    else
        handle->config[1] &= ~C8724_REG2_CKS_MASK;
}

/**
 * @brief 获取时钟源
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return 当前时钟源：C8724_CLOCK_SOURCE_INTERNAL 或 C8724_CLOCK_SOURCE_EXTERNAL
 */
static inline c8724_clock_source_t c8724_GetClockSource(const c8724_handle_t *handle)
{
    return (handle->config[1] & C8724_REG2_CKS_MASK)
           ? C8724_CLOCK_SOURCE_EXTERNAL : C8724_CLOCK_SOURCE_INTERNAL;
}

/**
 * @brief 设置行扫描数
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param scan   扫描行数枚举，C8724_SCAN_1_ROW ~ C8724_SCAN_8_ROWS
 */
static inline void c8724_SetScan(c8724_handle_t *handle, c8724_scan_t scan)
{
    handle->config[1] &= (uint8_t)~C8724_REG2_SCAN_MASK;
    handle->config[1] |= (uint8_t)((scan << C8724_REG2_SCAN_POS) & C8724_REG2_SCAN_MASK);
}

/**
 * @brief 获取行扫描数
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return 当前扫描行数枚举
 */
static inline c8724_scan_t c8724_GetScan(const c8724_handle_t *handle)
{
    return (c8724_scan_t)((handle->config[1] & C8724_REG2_SCAN_MASK) >> C8724_REG2_SCAN_POS);
}

/**
 * @brief 设置测试模式
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param enable C8724_BOOL_TRUE 使能，C8724_BOOL_FALSE 关闭
 * @note  正常显示时不可置为 TRUE
 */
static inline void c8724_SetTestMode(c8724_handle_t *handle, c8724_bool_t enable)
{
    if (enable)
        handle->config[1] |=  C8724_REG2_WM_MASK;
    else
        handle->config[1] &= ~C8724_REG2_WM_MASK;
}

/**
 * @brief 获取测试模式状态
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return C8724_BOOL_TRUE 表示测试模式开启
 */
static inline c8724_bool_t c8724_GetTestMode(const c8724_handle_t *handle)
{
    return (handle->config[1] & C8724_REG2_WM_MASK)
           ? C8724_BOOL_TRUE : C8724_BOOL_FALSE;
}

/**
 * @brief 设置显示数据更新模式
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param mode   更新模式：C8724_UPDATE_MODE_FORCE 或 C8724_UPDATE_MODE_SCAN_END
 */
static inline void c8724_SetUpdateMode(c8724_handle_t *handle, c8724_update_mode_t mode)
{
    if (mode)
        handle->config[1] |=  C8724_REG2_VSYN_M_MASK;
    else
        handle->config[1] &= ~C8724_REG2_VSYN_M_MASK;
}

/**
 * @brief 获取显示数据更新模式
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return 当前更新模式：C8724_UPDATE_MODE_FORCE 或 C8724_UPDATE_MODE_SCAN_END
 */
static inline c8724_update_mode_t c8724_GetUpdateMode(const c8724_handle_t *handle)
{
    return (handle->config[1] & C8724_REG2_VSYN_M_MASK)
           ? C8724_UPDATE_MODE_SCAN_END : C8724_UPDATE_MODE_FORCE;
}

/* ================= 设置寄存器 3 ================= */

/**
 * @brief 设置 125°C 过温保护功能
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param enable C8724_BOOL_FALSE 关闭保护，C8724_BOOL_TRUE 打开保护
 */
static inline void c8724_SetOtp125(c8724_handle_t *handle, c8724_bool_t enable)
{
    if (enable)
        handle->config[2] &= ~C8724_REG3_OTP1_MASK;
    else
        handle->config[2] |=  C8724_REG3_OTP1_MASK;
}

/**
 * @brief 获取 125°C 过温保护功能状态
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return C8724_BOOL_TRUE 表示保护关闭，C8724_BOOL_FALSE 表示保护打开
 */
static inline c8724_bool_t c8724_GetOtp125(const c8724_handle_t *handle)
{
    return (handle->config[2] & C8724_REG3_OTP1_MASK)
           ? C8724_BOOL_FALSE : C8724_BOOL_TRUE;
}

/**
 * @brief 设置电流设置电阻选择 RCS
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param select C8724_BOOL_TRUE 置 RCS=1（外部电流设置电阻——本 24 脚封装无
 *               ISET 脚，恒流基准不建立、必黑屏，禁止使用）；
 *               C8724_BOOL_FALSE 清 RCS=0（内部电阻，手册要求固定为 0，
 *               正常配置必选此项）
 * @warning 复位默认 RCS=1 属危险位：从默认值组装配置时必须显式清 0，否则黑屏
 */
static inline void c8724_SetResistorSelect(c8724_handle_t *handle, c8724_ResistorSelect_t select)
{
    (void) select;
    handle->config[2] &= ~C8724_REG3_RCS_MASK;
}

/**
 * @brief 获取电流设置电阻选择状态
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return C8724_BOOL_TRUE 表示 RCS=1（外部电阻模式，本封装不可用，
 *         正常配置下应恒为 C8724_BOOL_FALSE）
 */
static inline c8724_bool_t c8724_GetResistorSel(const c8724_handle_t *handle)
{
    return (handle->config[2] & C8724_REG3_RCS_MASK)
           ? C8724_BOOL_TRUE : C8724_BOOL_FALSE;
}

/**
 * @brief 设置内部时钟频率
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param clock  内部时钟频率枚举：1/2/4/8 MHz
 */
static inline void c8724_SetInternalClock(c8724_handle_t *handle, c8724_internal_clock_t clock)
{
    handle->config[2] &= (uint8_t)~C8724_REG3_RCKS_MASK;
    handle->config[2] |= (uint8_t)((clock << C8724_REG3_RCKS_POS) & C8724_REG3_RCKS_MASK);
}

/**
 * @brief 获取内部时钟频率
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return 当前内部时钟频率枚举
 */
static inline c8724_internal_clock_t c8724_GetInternalClock(const c8724_handle_t *handle)
{
    return (c8724_internal_clock_t)((handle->config[2] & C8724_REG3_RCKS_MASK) >> C8724_REG3_RCKS_POS);
}

/**
 * @brief 设置 SEG 输出使能
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param enable C8724_BOOL_TRUE 打开输出，C8724_BOOL_FALSE 关闭输出
 */
static inline void c8724_SetSegOutput(c8724_handle_t *handle, c8724_bool_t enable)
{
    if (enable)
        handle->config[2] |=  C8724_REG3_OE_MASK;
    else
        handle->config[2] &= ~C8724_REG3_OE_MASK;
}

/**
 * @brief 获取 SEG 输出使能状态
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return C8724_BOOL_TRUE 表示输出已打开
 */
static inline c8724_bool_t c8724_GetSegOutput(const c8724_handle_t *handle)
{
    return (handle->config[2] & C8724_REG3_OE_MASK)
           ? C8724_BOOL_TRUE : C8724_BOOL_FALSE;
}

/**
 * @brief 设置消影强度
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param strength 消影强度：C8724_GHOST_REMOVAL_WEAK 或 C8724_GHOST_REMOVAL_STRONG
 */
static inline void c8724_SetGhostRemoval(c8724_handle_t *handle, c8724_ghost_removal_t strength)
{
    if (strength)
        handle->config[2] |=  C8724_REG3_DGH_MASK;
    else
        handle->config[2] &= ~C8724_REG3_DGH_MASK;
}

/**
 * @brief 获取消影强度
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return 当前消影强度：C8724_GHOST_REMOVAL_WEAK 或 C8724_GHOST_REMOVAL_STRONG
 */
static inline c8724_ghost_removal_t c8724_GetGhostRemoval(const c8724_handle_t *handle)
{
    return (handle->config[2] & C8724_REG3_DGH_MASK)
           ? C8724_GHOST_REMOVAL_STRONG : C8724_GHOST_REMOVAL_WEAK;
}

/**
 * @brief 设置换行时间 TLS
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param time   换行时间枚举：4T/8T/12T/16T
 */
static inline void c8724_SetLineBlanking(c8724_handle_t *handle, c8724_line_blanking_t time)
{
    handle->config[2] &= (uint8_t)~C8724_REG3_TLS_MASK;
    handle->config[2] |= (uint8_t)((time << C8724_REG3_TLS_POS) & C8724_REG3_TLS_MASK);
}

/**
 * @brief 获取换行时间 TLS
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return 当前换行时间枚举
 */
static inline c8724_line_blanking_t c8724_GetLineBlanking(const c8724_handle_t *handle)
{
    return (c8724_line_blanking_t)((handle->config[2] & C8724_REG3_TLS_MASK) >> C8724_REG3_TLS_POS);
}

/* ================= 设置寄存器 4 ================= */

/**
 * @brief 设置休眠模式使能
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param enable C8724_BOOL_TRUE 使能，C8724_BOOL_FALSE 不使能
 */
static inline void c8724_SetSleepEnable(c8724_handle_t *handle, c8724_bool_t enable)
{
    if (enable)
        handle->config[3] |=  C8724_REG4_SLEEP_EN_MASK;
    else
        handle->config[3] &= ~C8724_REG4_SLEEP_EN_MASK;
}

/**
 * @brief 获取休眠模式使能状态
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return C8724_BOOL_TRUE 表示已使能休眠模式
 */
static inline c8724_bool_t c8724_GetSleepEnable(const c8724_handle_t *handle)
{
    return (handle->config[3] & C8724_REG4_SLEEP_EN_MASK)
           ? C8724_BOOL_TRUE : C8724_BOOL_FALSE;
}

/**
 * @brief 设置自动休眠模式使能
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param enable C8724_BOOL_TRUE 使能，C8724_BOOL_FALSE 不使能
 * @note  使能后收到全 0 数据自动进入休眠，写入非 0 数据自动唤醒
 */
static inline void c8724_SetAutoSleep(c8724_handle_t *handle, c8724_bool_t enable)
{
    if (enable)
        handle->config[3] |=  C8724_REG4_SLEEP_AT_MASK;
    else
        handle->config[3] &= ~C8724_REG4_SLEEP_AT_MASK;
}

/**
 * @brief 获取自动休眠模式使能状态
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return C8724_BOOL_TRUE 表示已使能自动休眠
 */
static inline c8724_bool_t c8724_GetAutoSleep(const c8724_handle_t *handle)
{
    return (handle->config[3] & C8724_REG4_SLEEP_AT_MASK)
           ? C8724_BOOL_TRUE : C8724_BOOL_FALSE;
}

/**
 * @brief 设置全局复位使能
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param enable C8724_BOOL_TRUE 使能，C8724_BOOL_FALSE 不使能
 */
static inline void c8724_SetGlobalResetEnable(c8724_handle_t *handle, c8724_bool_t enable)
{
    if (enable)
        handle->config[3] |=  C8724_REG4_GRST_MASK;
    else
        handle->config[3] &= ~C8724_REG4_GRST_MASK;
}

/**
 * @brief 获取全局复位使能状态
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return C8724_BOOL_TRUE 表示已使能全局复位
 */
static inline c8724_bool_t c8724_GetGlobalResetEnable(const c8724_handle_t *handle)
{
    return (handle->config[3] & C8724_REG4_GRST_MASK)
           ? C8724_BOOL_TRUE : C8724_BOOL_FALSE;
}

/**
 * @brief 设置软复位使能
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param enable C8724_BOOL_TRUE 使能，C8724_BOOL_FALSE 不使能
 */
static inline void c8724_SetSoftResetEnable(c8724_handle_t *handle, c8724_bool_t enable)
{
    if (enable)
        handle->config[3] |=  C8724_REG4_SRST_MASK;
    else
        handle->config[3] &= ~C8724_REG4_SRST_MASK;
}

/**
 * @brief 获取软复位使能状态
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return C8724_BOOL_TRUE 表示已使能软复位
 */
static inline c8724_bool_t c8724_GetSoftResetEnable(const c8724_handle_t *handle)
{
    return (handle->config[3] & C8724_REG4_SRST_MASK)
           ? C8724_BOOL_TRUE : C8724_BOOL_FALSE;
}

/**
 * @brief 设置扫描时钟 2 倍频使能
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param enable C8724_BOOL_TRUE 使能，C8724_BOOL_FALSE 不使能
 */
static inline void c8724_SetClockDouble(c8724_handle_t *handle, c8724_bool_t enable)
{
    if (enable)
        handle->config[3] |=  C8724_REG4_CLK2X_MASK;
    else
        handle->config[3] &= ~C8724_REG4_CLK2X_MASK;
}

/**
 * @brief 获取扫描时钟 2 倍频使能状态
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return C8724_BOOL_TRUE 表示已使能 2 倍频
 */
static inline c8724_bool_t c8724_GetClockDouble(const c8724_handle_t *handle)
{
    return (handle->config[3] & C8724_REG4_CLK2X_MASK)
           ? C8724_BOOL_TRUE : C8724_BOOL_FALSE;
}

/**
 * @brief 设置 150°C 过温保护功能
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param enable C8724_BOOL_FALSE 打开保护，C8724_BOOL_TRUE 关闭保护
 */
static inline void c8724_SetOtp150(c8724_handle_t *handle, c8724_bool_t enable)
{
    if (enable)
        handle->config[3] &= ~C8724_REG4_OTP2_MASK;
    else
        handle->config[3] |=  C8724_REG4_OTP2_MASK;
}

/**
 * @brief 获取 150°C 过温保护功能状态
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return C8724_BOOL_FALSE 表示保护关闭，C8724_BOOL_TRUE 表示保护打开
 */
static inline c8724_bool_t c8724_GetOtp150(const c8724_handle_t *handle)
{
    return (handle->config[3] & C8724_REG4_OTP2_MASK)
           ? C8724_BOOL_FALSE : C8724_BOOL_TRUE;
}

/**
 * @brief 设置掉电复位功能
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @param enable C8724_BOOL_TRUE 使能，C8724_BOOL_FALSE 不使能
 */
static inline void c8724_SetPowerDownReset(c8724_handle_t *handle, c8724_bool_t enable)
{
    if (enable)
        handle->config[3] |=  C8724_REG4_PDR_MASK;
    else
        handle->config[3] &= ~C8724_REG4_PDR_MASK;
}

/**
 * @brief 获取掉电复位功能状态
 * @param handle 指向 c8724_handle_t 结构体的指针
 * @return C8724_BOOL_TRUE 表示已使能掉电复位
 */
static inline c8724_bool_t c8724_GetPowerDownReset(const c8724_handle_t *handle)
{
    return (handle->config[3] & C8724_REG4_PDR_MASK)
           ? C8724_BOOL_TRUE : C8724_BOOL_FALSE;
}

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif