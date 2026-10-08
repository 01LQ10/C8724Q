/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724.c
 * @brief     driver c8724 source file
 * @version   1.4.0
 * @author    LQ
 * @date      2026-10-05
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author  <th>Description
 * <tr><td>2026/10/05  <td>1.0.0    <td>LQ      <td>first upload
 * <tr><td>2026/10/05  <td>1.1.0    <td>LQ      <td>config API reduced to set/get plus field macros, frame buffer added
 * <tr><td>2026/10/08  <td>1.2.0    <td>LQ      <td>instruction byte fixed: broadcast at B6, chip select at B5:B4 (was B7/B4:B3, collided with type bits)
 * <tr><td>2026/10/08  <td>1.3.0    <td>LQ      <td>set_config takes only the handle, dead deinit backup removed, SEG11/12 masked when CS inputs, packets batched, command macros deduplicated
 * <tr><td>2026/10/08  <td>1.4.0    <td>LQ      <td>module renamed to c8724, function naming unified, comments normalized
 * </table>
 */

// 参考 https://blog.csdn.net/huyixiangbaba/article/details/143070779

#include "driver_c8724.h"

#define C8724_PACKET_HEADER_0                      0x5AU /**< instruction packet header byte 0 */
#define C8724_PACKET_HEADER_1                      0xFFU /**< instruction packet header byte 1 */

/* 指令码复用头文件中的 c8724_command_t 枚举，避免同名双轨定义 */

#define C8724_ADDRESS_BROADCAST_BIT                0x40U /**< broadcast selection bit (instruction B6) */
#define C8724_ADDRESS_SHIFT                        4U    /**< chip select address shift (instruction B5:B4) */
#define C8724_CONFIG_REG2_RESERVED_MASK            0x60U /**< reserved bits in configuration register 2 */
#define C8724_CONFIG_REG4_RESERVED_MASK            0x80U /**< reserved bit in configuration register 4 */

#define C8724_DISPLAY_ADDRESS_END                  0xFFU /**< addressed display data terminator */

/**
 * @brief      配置寄存器 1~4 的手册复位默认值
 * @note       芯片上电或全局复位后回到这些值
 */
static const uint8_t gs_c8724_config_default[4] = { 0x3FU, 0x00U, 0xF0U, 0x02U };

/**
 * @brief      发送无载荷指令包
 * @param[in]  handle  指向 c8724_handle_t 结构体的指针
 * @param[in]  command 指令码
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 写失败
 * @note       指令校验包含两个包头字节和指令码字节
 */
static uint8_t a_c8724_send_command(c8724_handle_t *handle, uint8_t command)
{
    uint8_t packet[4];
    uint8_t instruction;

    instruction = (uint8_t)(command | ((handle->address & 0x03U) << C8724_ADDRESS_SHIFT));
    if (handle->broadcast != 0U)
    {
        instruction |= C8724_ADDRESS_BROADCAST_BIT;
    }
    packet[0] = C8724_PACKET_HEADER_0;
    packet[1] = C8724_PACKET_HEADER_1;
    packet[2] = instruction;
    packet[3] = (uint8_t)(packet[0] + packet[1] + packet[2]);
    if (handle->spi_write_cmd(packet, 4U) != 0U)
    {
        handle->debug_print("c8724: write instruction failed.\n");
        return 1U;
    }

    return 0U;
}

/**
 * @brief      发送 4 个配置字节及其校验
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 写失败
 * @note       4 个字节取自 handle->config
 */
static uint8_t a_c8724_write_config(c8724_handle_t *handle)
{
    uint8_t packet[9];
    uint8_t instruction;

    instruction = (uint8_t)(C8724_COMMAND_CONFIG |
                            ((handle->address & 0x03U) << C8724_ADDRESS_SHIFT));
    if (handle->broadcast != 0U)
    {
        instruction |= C8724_ADDRESS_BROADCAST_BIT;
    }
    packet[0] = C8724_PACKET_HEADER_0;
    packet[1] = C8724_PACKET_HEADER_1;
    packet[2] = instruction;
    packet[3] = (uint8_t)(packet[0] + packet[1] + packet[2]);
    packet[4] = handle->config[0];
    packet[5] = handle->config[1];
    packet[6] = handle->config[2];
    packet[7] = handle->config[3];
    packet[8] = (uint8_t)(handle->config[0] + handle->config[1]
                        + handle->config[2] + handle->config[3]);
    if (handle->spi_write_cmd(packet, 9U) != 0U)
    {
        handle->debug_print("c8724: write configuration failed.\n");
        return 1U;
    }

    return 0U;
}

/**
 * @brief      校验配置保留位
 * @param[in]  config 指向 4 个配置字节的指针
 * @return     状态码
 *             - 0 配置有效
 *             - 1 保留位被置位
 * @note       保留位保持手册规定的值
 */
static uint8_t a_c8724_config_valid(const uint8_t config[4])
{
    if (((config[1] & C8724_CONFIG_REG2_RESERVED_MASK) != 0U) ||
        ((config[3] & C8724_CONFIG_REG4_RESERVED_MASK) != 0U))
    {
        return 1U;
    }

    return 0U;
}

/**
 * @brief      从配置寄存器 2 推导使能的 GRID 行数
 * @param[in]  reg2 配置寄存器 2 的值
 * @return     使能的 GRID 行数
 * @note       SCAN 编码为使能的 GRID 输出数减一
 */
static uint8_t a_c8724_get_scan_rows(uint8_t reg2)
{
    return (uint8_t)(((reg2 & C8724_REG2_SCAN_MASK) >> C8724_REG2_SCAN_POS) + 1U);
}

/**
 * @brief      检查句柄是否已初始化
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 已初始化
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 * @note       无
 */
static uint8_t a_c8724_check_handle(c8724_handle_t *handle)
{
    if (handle == NULL)
    {
        return 2U;
    }
    if (handle->inited != 1U)
    {
        return 3U;
    }

    return 0U;
}

/**
 * @brief      获取芯片信息
 * @param[out] info 指向 c8724_info_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 2 info 无效
 * @note       无
 */
uint8_t c8724_info(c8724_info_t *info)
{
    if (info == NULL)
    {
        return 2U;
    }
    (void)memset(info, 0, sizeof(c8724_info_t));
    (void)memcpy(info->chip_name, "C8724", sizeof("C8724"));
    (void)memcpy(info->manufacturer_name, "Chipfountain", sizeof("Chipfountain"));
    (void)memcpy(info->interface, "SPI", sizeof("SPI"));
    info->supply_voltage_min_v = 2.9f;
    info->supply_voltage_max_v = 5.5f;
    info->max_current_ma = 40.0f;
    info->temperature_min = -40.0f;
    info->temperature_max = 85.0f;
    info->driver_version = C8724_DRIVER_VERSION;

    return 0U;
}

/**
 * @brief      初始化芯片
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 无效
 *             - 3 所需函数为空或芯片已初始化
 * @note       SPI 初始化后发送广播全局复位，并把手册复位默认值载入影子寄存器
 */
uint8_t c8724_init(c8724_handle_t *handle)
{
    uint8_t res;

    if (handle == NULL)
    {
        return 2U;
    }
    if (handle->inited == 1U)
    {
        return 3U;
    }
    if (handle->debug_print == NULL)
    {
        return 3U;
    }
    if (handle->spi_init == NULL)
    {
        handle->debug_print("c8724: spi_init is null.\n");
        return 3U;
    }
    if (handle->spi_deinit == NULL)
    {
        handle->debug_print("c8724: spi_deinit is null.\n");
        return 3U;
    }
    if (handle->spi_write_cmd == NULL)
    {
        handle->debug_print("c8724: spi_write_cmd is null.\n");
        return 3U;
    }

    if (handle->spi_init() != 0U)
    {
        handle->debug_print("c8724: spi init failed.\n");
        return 1U;
    }
    handle->address = C8724_ADDRESS_0;
    handle->broadcast = C8724_BOOL_TRUE;
    (void)memcpy(handle->config, gs_c8724_config_default, sizeof(handle->config));
    (void)memset(handle->frame, 0, sizeof(handle->frame));
    res = a_c8724_send_command(handle, C8724_COMMAND_GLOBAL_RESET);
    if (res != 0U)
    {
        (void)handle->spi_deinit();
        return 1U;
    }
    handle->broadcast = C8724_BOOL_FALSE;
    handle->inited = 1U;

    return 0U;
}

/**
 * @brief      反初始化芯片
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 * @note       清空当前帧，在配置中使能休眠，发送休眠指令，最后反初始化 SPI
 */
uint8_t c8724_deinit(c8724_handle_t *handle)
{
    uint8_t res;
    uint8_t status;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    status = c8724_display_clear(handle);
    if (status != 0U)
    {
        handle->debug_print("c8724: clear display during deinit failed.\n");
        res = 1U;
    }
    else
    {
        c8724_SetSleepEnable(handle, C8724_BOOL_TRUE);
        status = c8724_set_config(handle);
        if (status != 0U)
        {
            handle->debug_print("c8724: enable sleep during deinit failed.\n");
            res = 1U;
        }
        else
        {
            status = a_c8724_send_command(handle, C8724_COMMAND_SLEEP);
            if (status != 0U)
            {
                handle->debug_print("c8724: sleep during deinit failed.\n");
                res = 1U;
            }
        }
    }
    if (handle->spi_deinit() != 0U)
    {
        handle->debug_print("c8724: spi deinit failed.\n");
        res = 1U;
    }
    handle->inited = 0U;

    return res;
}

/**
 * @brief      设置指令片选地址
 * @param[in]  handle  指向 c8724_handle_t 结构体的指针
 * @param[in]  address 片选地址，0~3
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 *             - 4 address 无效
 * @note       这是指令地址，不是 MCU 的 GPIO
 */
uint8_t c8724_set_address(c8724_handle_t *handle, c8724_address_t address)
{
    uint8_t res;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if ((uint8_t)address > 3U)
    {
        return 4U;
    }
    handle->address = (uint8_t)address;

    return 0U;
}

/**
 * @brief      获取指令片选地址
 * @param[in]  handle  指向 c8724_handle_t 结构体的指针
 * @param[out] address 指向片选地址的指针
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 *             - 4 address 无效
 * @note       返回软件选择的指令地址
 */
uint8_t c8724_get_address(c8724_handle_t *handle, c8724_address_t *address)
{
    uint8_t res;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if (address == NULL)
    {
        return 4U;
    }
    *address = (c8724_address_t)handle->address;

    return 0U;
}

/**
 * @brief      设置广播指令选择
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @param[in]  enable 广播使能值
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 *             - 4 enable 无效
 * @note       广播指令不受芯片地址限制
 */
uint8_t c8724_set_broadcast(c8724_handle_t *handle, c8724_bool_t enable)
{
    uint8_t res;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if ((enable != C8724_BOOL_FALSE) && (enable != C8724_BOOL_TRUE))
    {
        return 4U;
    }
    handle->broadcast = (uint8_t)enable;

    return 0U;
}

/**
 * @brief      获取广播指令选择
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @param[out] enable 指向广播使能值的指针
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 *             - 4 enable 无效
 * @note       返回软件选择的广播模式
 */
uint8_t c8724_get_broadcast(c8724_handle_t *handle, c8724_bool_t *enable)
{
    uint8_t res;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if (enable == NULL)
    {
        return 4U;
    }
    *enable = (c8724_bool_t)handle->broadcast;

    return 0U;
}

/**
 * @brief      发送 4 个配置寄存器到芯片
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 *             - 4 handle->config 含保留位
 * @note       4 个字节取自 handle->config，先校验保留位，再带两个校验和
 *             作为一包发送
 */
uint8_t c8724_set_config(c8724_handle_t *handle)
{
    uint8_t res;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if (a_c8724_config_valid(handle->config) != 0U)
    {
        return 4U;
    }
    res = a_c8724_write_config(handle);
    if (res != 0U)
    {
        return res;
    }

    return 0U;
}

/**
 * @brief      写入顺序显示 PWM 数据
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @param[in]  data   指向 PWM 数据缓冲区的指针
 * @param[in]  len    PWM 字节数
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 *             - 4 data 为空或长度与 SCAN 不匹配
 * @note       数据顺序为 GRID1 SEG1-12、GRID2 SEG1-12，依此类推；当 REG1
 *             把 SEG11/SEG12 配置为片选输入时，对应的数据列在发送前被
 *             强制为 0（手册规定），校验和按实际发送的字节计算。数据包按
 *             头部 / 数据 / 校验 分事务发送——字节间空隙无害（CF 参考实现
 *             已验证）。若 SPI 在包中途失败，芯片可能停留在接收模式，
 *             字节流在下一次 c8724_init / 全局复位前保持失步
 */
uint8_t c8724_write_display(c8724_handle_t *handle, const uint8_t *data, uint16_t len)
{
    uint8_t masked[C8724_DISPLAY_DATA_MAX];
    const uint8_t *send_buf;
    uint8_t packet_header[4];
    uint8_t checksum;
    uint8_t instruction;
    uint8_t res;
    uint16_t i;
    uint16_t expected_len;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    expected_len = (uint16_t)(C8724_DISPLAY_WIDTH * a_c8724_get_scan_rows(handle->config[1]));
    if ((data == NULL) || (len != expected_len))
    {
        return 4U;
    }
    instruction = (uint8_t)(C8724_COMMAND_DISPLAY_SEQUENTIAL |
                            ((handle->address & 0x03U) << C8724_ADDRESS_SHIFT));
    if (handle->broadcast != 0U)
    {
        instruction |= C8724_ADDRESS_BROADCAST_BIT;
    }
    packet_header[0] = C8724_PACKET_HEADER_0;
    packet_header[1] = C8724_PACKET_HEADER_1;
    packet_header[2] = instruction;
    packet_header[3] = (uint8_t)(packet_header[0] + packet_header[1] + packet_header[2]);

    /* 手册：SEG11/SEG12 被配置为片选输入时其数据固定写 0；
       仅在命中该条件时启用副本，不改动调用方缓冲区 */
    send_buf = data;
    if (((handle->config[0] & C8724_REG1_SEG11_CS_MASK) == 0U) ||
        ((handle->config[0] & C8724_REG1_SEG12_CS_MASK) == 0U))
    {
        (void)memcpy(masked, data, len);
        for (i = 0U; i < len; i = (uint16_t)(i + C8724_DISPLAY_WIDTH))
        {
            if ((handle->config[0] & C8724_REG1_SEG11_CS_MASK) == 0U)
            {
                masked[i + 10U] = 0U;
            }
            if ((handle->config[0] & C8724_REG1_SEG12_CS_MASK) == 0U)
            {
                masked[i + 11U] = 0U;
            }
        }
        send_buf = masked;
    }

    /* 表头 4 字节合并为一次发送（原 3+1 拆分） */
    if (handle->spi_write_cmd(packet_header, 4U) != 0U)
    {
        handle->debug_print("c8724: write display header failed.\n");
        return 1U;
    }
    if (handle->spi_write_cmd(send_buf, len) != 0U)
    {
        handle->debug_print("c8724: write display data failed.\n");
        return 1U;
    }
    checksum = 0U;
    for (i = 0U; i < len; i++)
    {
        checksum = (uint8_t)(checksum + send_buf[i]);
    }
    if (handle->spi_write_cmd(&checksum, 1U) != 0U)
    {
        handle->debug_print("c8724: write display checksum failed.\n");
        return 1U;
    }

    return 0U;
}

/**
 * @brief      写入按地址显示 PWM 数据
 * @param[in]  handle  指向 c8724_handle_t 结构体的指针
 * @param[in]  address 指向 SRAM 地址的指针
 * @param[in]  data    指向 PWM 数据缓冲区的指针
 * @param[in]  len     地址与数据条目数
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 *             - 4 指针、长度或 SRAM 地址无效
 * @note       最多 72 条目；地址 = (GRID 索引 << 4) | SEG 索引。整个数据包
 *             （头部 + 数据对 + 0xFF 结束符 + 校验）在一个缓冲区中组装后
 *             单次发送；若 SPI 在包中途失败，芯片可能停留在接收模式，字节流
 *             在下一次 c8724_init / 全局复位前保持失步
 */
uint8_t c8724_write_display_address(c8724_handle_t *handle, const uint8_t *address,
                                    const uint8_t *data, uint16_t len)
{
    uint8_t packet[4U + (2U * C8724_ADDRESS_DATA_MAX) + 2U];
    uint8_t checksum;
    uint8_t instruction;
    uint8_t rows;
    uint8_t grid;
    uint8_t segment;
    uint8_t res;
    uint16_t i;
    uint16_t total;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    rows = a_c8724_get_scan_rows(handle->config[1]);
    if ((address == NULL) || (data == NULL) || (len == 0U) || (len > C8724_ADDRESS_DATA_MAX))
    {
        return 4U;
    }
    for (i = 0U; i < len; i++)
    {
        grid = (uint8_t)(address[i] >> 4U);
        segment = (uint8_t)(address[i] & 0x0FU);
        if ((grid >= rows) || (segment >= C8724_DISPLAY_WIDTH))
        {
            return 4U;
        }
    }
    instruction = (uint8_t)(C8724_COMMAND_DISPLAY_ADDRESS |
                            ((handle->address & 0x03U) << C8724_ADDRESS_SHIFT));
    if (handle->broadcast != 0U)
    {
        instruction |= C8724_ADDRESS_BROADCAST_BIT;
    }
    /* 整帧一次组包：表头 + 地址/数据对 + FF 终止符 + 包尾校验，单次发送 */
    packet[0] = C8724_PACKET_HEADER_0;
    packet[1] = C8724_PACKET_HEADER_1;
    packet[2] = instruction;
    packet[3] = (uint8_t)(packet[0] + packet[1] + packet[2]);
    checksum = 0U;
    total = 4U;
    for (i = 0U; i < len; i++)
    {
        packet[total] = address[i];
        checksum = (uint8_t)(checksum + address[i]);
        total++;
        packet[total] = data[i];
        checksum = (uint8_t)(checksum + data[i]);
        total++;
    }
    packet[total] = C8724_DISPLAY_ADDRESS_END;
    checksum = (uint8_t)(checksum + C8724_DISPLAY_ADDRESS_END);
    total++;
    packet[total] = checksum;
    total++;
    if (handle->spi_write_cmd(packet, total) != 0U)
    {
        handle->debug_print("c8724: write addressed display failed.\n");
        return 1U;
    }

    return 0U;
}

/**
 * @brief      用待发送 SRAM 帧更新显示
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 * @note       无
 */
uint8_t c8724_display_update(c8724_handle_t *handle)
{
    uint8_t res;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }

    return a_c8724_send_command(handle, C8724_COMMAND_DISPLAY_UPDATE);
}

/**
 * @brief      设置帧缓冲中一个像素的 PWM 值
 * @param[in]  handle  指向 c8724_handle_t 结构体的指针
 * @param[in]  grid    GRID 索引，0~7
 * @param[in]  segment SEG 索引，0~11
 * @param[in]  pwm     PWM 值，0~255
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 *             - 4 grid 或 segment 越界
 * @note       仅修改帧缓冲；调用 c8724_display_flush 后才会显示
 */
uint8_t c8724_display_set_pixel(c8724_handle_t *handle, uint8_t grid, uint8_t segment, uint8_t pwm)
{
    uint8_t res;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if ((grid >= C8724_DISPLAY_HEIGHT) || (segment >= C8724_DISPLAY_WIDTH))
    {
        return 4U;
    }
    handle->frame[(uint16_t)((uint16_t)grid * C8724_DISPLAY_WIDTH + segment)] = pwm;

    return 0U;
}

/**
 * @brief      从帧缓冲读取一个像素的 PWM 值
 * @param[in]  handle  指向 c8724_handle_t 结构体的指针
 * @param[in]  grid    GRID 索引，0~7
 * @param[in]  segment SEG 索引，0~11
 * @param[out] pwm     指向 PWM 输出值的指针
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 *             - 4 grid、segment 或指针无效
 * @note       无
 */
uint8_t c8724_display_get_pixel(c8724_handle_t *handle, uint8_t grid, uint8_t segment, uint8_t *pwm)
{
    uint8_t res;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if ((grid >= C8724_DISPLAY_HEIGHT) || (segment >= C8724_DISPLAY_WIDTH) || (pwm == NULL))
    {
        return 4U;
    }
    *pwm = handle->frame[(uint16_t)((uint16_t)grid * C8724_DISPLAY_WIDTH + segment)];

    return 0U;
}

/**
 * @brief      用同一 PWM 值填满整个帧缓冲
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @param[in]  pwm    PWM 值，0~255
 * @return     状态码
 *             - 0 成功
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 * @note       仅修改帧缓冲；调用 c8724_display_flush 后才会显示
 */
uint8_t c8724_display_fill(c8724_handle_t *handle, uint8_t pwm)
{
    uint8_t res;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    (void)memset(handle->frame, pwm, sizeof(handle->frame));

    return 0U;
}

/**
 * @brief      把帧缓冲发送给芯片并切换到该帧
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 * @note       用一次顺序写入发送 SCAN 使能的行，然后发送显示更新指令
 */
uint8_t c8724_display_flush(c8724_handle_t *handle)
{
    uint8_t res;
    uint16_t len;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    len = (uint16_t)(C8724_DISPLAY_WIDTH * a_c8724_get_scan_rows(handle->config[1]));
    res = c8724_write_display(handle, handle->frame, len);
    if (res != 0U)
    {
        return res;
    }

    return c8724_display_update(handle);
}

/**
 * @brief      清空帧缓冲和当前显示帧
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 * @note       把帧缓冲填为 0 并刷新
 */
uint8_t c8724_display_clear(c8724_handle_t *handle)
{
    uint8_t res;

    res = c8724_display_fill(handle, 0U);
    if (res != 0U)
    {
        return res;
    }

    return c8724_display_flush(handle);
}

/**
 * @brief      发送软复位指令
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 * @note       不复位配置寄存器
 */
uint8_t c8724_soft_reset(c8724_handle_t *handle)
{
    uint8_t res;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }

    return a_c8724_send_command(handle, C8724_COMMAND_SOFT_RESET);
}

/**
 * @brief      发送全局复位指令
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 * @note       复位配置寄存器、关闭输出，并把手册复位默认值载入影子寄存器
 */
uint8_t c8724_global_reset(c8724_handle_t *handle)
{
    uint8_t res;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    res = a_c8724_send_command(handle, C8724_COMMAND_GLOBAL_RESET);
    if (res == 0U)
    {
        (void)memcpy(handle->config, gs_c8724_config_default, sizeof(handle->config));
        (void)memset(handle->frame, 0, sizeof(handle->frame));
    }

    return res;
}

/**
 * @brief      发送休眠指令
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 *             - 4 配置寄存器 4 中休眠模式未使能
 * @note       调用者必须在休眠前写入全 0 帧
 */
uint8_t c8724_sleep(c8724_handle_t *handle)
{
    uint8_t res;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if ((handle->config[3] & C8724_REG4_SLEEP_EN_MASK) == 0U)
    {
        return 4U;
    }

    return a_c8724_send_command(handle, C8724_COMMAND_SLEEP);
}

/**
 * @brief      发送唤醒指令
 * @param[in]  handle 指向 c8724_handle_t 结构体的指针
 * @return     状态码
 *             - 0 成功
 *             - 1 SPI 操作失败
 *             - 2 handle 无效
 *             - 3 芯片未初始化
 * @note       无
 */
uint8_t c8724_wake(c8724_handle_t *handle)
{
    uint8_t res;

    res = a_c8724_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }

    return a_c8724_send_command(handle, C8724_COMMAND_WAKE);
}