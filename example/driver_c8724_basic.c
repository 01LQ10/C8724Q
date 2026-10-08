/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724_basic.c
 * @brief     driver c8724 basic example source file
 * @version   1.3.0
 * @author    LQ
 * @date      2026-10-05
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author  <th>Description
 * <tr><td>2026/10/05  <td>1.0.0    <td>LQ      <td>first upload
 * <tr><td>2026/10/05  <td>1.1.0    <td>LQ      <td>config flow moved to field macros and one set_config
 * <tr><td>2026/10/08  <td>1.2.0    <td>LQ      <td>applied the proven configuration preset (RCS = 0)
 * <tr><td>2026/10/08  <td>1.3.0    <td>LQ      <td>screen control API added (set pixel / refresh / fill / clear / screen off / sleep), five-stage init flow
 * </table>
 */

#include "driver_c8724_basic.h"
#include "driver_c8724.h"
#include "driver_c8724_interface.h"

static c8724_handle_t gs_handle;

/**
 * @brief      初始化 basic 示例
 * @return     状态码
 *             - 0 成功
 *             - 1 初始化或配置失败
 * @note       五段式流程的 ①②③ 段：LINK 绑定 → info 打印 → init + 默认配置编排；
 *             使用前必须先实现平台 interface 函数
 */
uint8_t c8724_basic_init(void)
{
    c8724_info_t info;
    uint8_t res;

    /* ① LINK 绑定：注入平台接口函数 */
    DRIVER_C8724_LINK_INIT(&gs_handle, c8724_handle_t);
    DRIVER_C8724_LINK_SPI_INIT(&gs_handle, c8724_interface_spi_init);
    DRIVER_C8724_LINK_SPI_DEINIT(&gs_handle, c8724_interface_spi_deinit);
    DRIVER_C8724_LINK_SPI_WRITE_CMD(&gs_handle, c8724_interface_spi_write_cmd);
    DRIVER_C8724_LINK_DELAY_MS(&gs_handle, c8724_interface_delay_ms);
    DRIVER_C8724_LINK_DEBUG_PRINT(&gs_handle, c8724_interface_debug_print);

    /* ② 打印芯片静态信息 */
    res = c8724_info(&info);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: get chip information failed.\n");
        return 1U;
    }
    gs_handle.debug_print("c8724: chip is %s.\n", info.chip_name);
    gs_handle.debug_print("c8724: manufacturer is %s.\n", info.manufacturer_name);
    gs_handle.debug_print("c8724: interface is %s.\n", info.interface);
    gs_handle.debug_print("c8724: driver version is %d.%d.\n",
                                info.driver_version / 1000U, (info.driver_version % 1000U) / 100U);
    gs_handle.debug_print("c8724: supply voltage is %0.1fV to %0.1fV.\n",
                                (double)info.supply_voltage_min_v, (double)info.supply_voltage_max_v);
    gs_handle.debug_print("c8724: max SEG current is %0.1fmA.\n", (double)info.max_current_ma);
    gs_handle.debug_print("c8724: temperature is %0.1fC to %0.1fC.\n",
                                (double)info.temperature_min, (double)info.temperature_max);

    /* ③ 初始化芯片并编排默认配置 */
    gs_handle.debug_print("c8724: init chip.\n");
    res = c8724_init(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: init failed.\n");
        return 1U;
    }

    gs_handle.debug_print("c8724: set default configuration.\n");
    /* 寄存器 1：SEG 引脚与电流 */
    c8724_SetSeg11PinMode(&gs_handle, C8724_BASIC_DEFAULT_SEG11_MODE);
    c8724_SetSeg12PinMode(&gs_handle, C8724_BASIC_DEFAULT_SEG12_MODE);
    c8724_SetGcc(&gs_handle, C8724_BASIC_DEFAULT_GCC);
    /* 寄存器 2：时钟、扫描、更新 */
    c8724_SetClockSource(&gs_handle, C8724_BASIC_DEFAULT_CLOCK_SOURCE);
    c8724_SetScan(&gs_handle, C8724_BASIC_DEFAULT_SCAN);
    c8724_SetTestMode(&gs_handle, C8724_BOOL_FALSE);                 /* 正常显示模式，禁止置 1 */
    c8724_SetUpdateMode(&gs_handle, C8724_BASIC_DEFAULT_UPDATE_MODE);
    /* 寄存器 3：保护、时钟、输出 */
    c8724_SetOtp125(&gs_handle, C8724_BOOL_TRUE);                    /* 打开 125°C 过温保护 */
    c8724_SetResistorSelect(&gs_handle, C8724_RES_SELECT_INTERNAL);  /* RCS=0 内部电阻，必选否则黑屏 */
    c8724_SetInternalClock(&gs_handle, C8724_BASIC_DEFAULT_CLOCK);
    c8724_SetSegOutput(&gs_handle, C8724_BASIC_DEFAULT_OUTPUT);
    c8724_SetGhostRemoval(&gs_handle, C8724_BASIC_DEFAULT_GHOST);
    c8724_SetLineBlanking(&gs_handle, C8724_BASIC_DEFAULT_BLANKING);
    /* 寄存器 4：休眠、复位、保护 */
    c8724_SetSleepEnable(&gs_handle, C8724_BOOL_FALSE);              /* 手动休眠由 basic_sleep 按需打开 */
    c8724_SetAutoSleep(&gs_handle, C8724_BASIC_DEFAULT_AUTO_SLEEP);
    c8724_SetGlobalResetEnable(&gs_handle, C8724_BOOL_FALSE);        /* 正常运行不使用 */
    c8724_SetSoftResetEnable(&gs_handle, C8724_BOOL_FALSE);          /* 正常运行不使用 */
    c8724_SetClockDouble(&gs_handle, C8724_BASIC_DEFAULT_CLK2X);
    c8724_SetOtp150(&gs_handle, C8724_BOOL_TRUE);                    /* 打开 150°C 过温保护 */
    c8724_SetPowerDownReset(&gs_handle, C8724_BOOL_TRUE);            /* 掉电复位使能，上电更稳定 */

    /* 一次性提交：指令校验 + 4 个寄存器 + 寄存器校验，一包下发 */
    res = c8724_set_config(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: set config failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }

    return 0U;
}

/**
 * @brief      把外部缓冲区的一帧直接推上屏幕（顺序写 + 显示更新）
 * @param[in]  *data 指向 PWM 数据缓冲区（GRID 主序：GRID1 SEG1-12、GRID2 SEG1-12 …）
 * @param[in]  len   PWM 字节数，必须等于 12 ×（SCAN + 1），默认配置下为 96
 * @return     状态码
 *             - 0 成功
 *             - 1 写入或更新失败
 * @note       每个 PWM 取值 0~255。本接口直接写芯片 SRAM，不经过内部帧缓冲；
 *             若之后要与 set_pixel / refresh 混用，请先用 fill 或 clear 同步缓冲
 */
uint8_t c8724_basic_display(const uint8_t *data, uint16_t len)
{
    uint8_t res;

    /* 写入下一帧 */
    res = c8724_write_display(&gs_handle, data, len);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: write display failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }
    /* 切换显示帧 */
    res = c8724_display_update(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: display update failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }

    return 0U;
}

/**
 * @brief      修改帧缓冲中一个像素的 PWM 值（不刷新屏幕）
 * @param[in]  grid    GRID 索引，0~7
 * @param[in]  segment SEG 索引，0~11
 * @param[in]  pwm     PWM 值，0~255
 * @return     状态码
 *             - 0 成功
 *             - 1 坐标无效或驱动调用失败
 * @note       仅改内部帧缓冲，调用 c8724_basic_refresh 后才会显示
 */
uint8_t c8724_basic_set_pixel(uint8_t grid, uint8_t segment, uint8_t pwm)
{
    uint8_t res;

    /* 参数错误先拦截，不触发回滚（未动过 SPI，芯片状态未变） */
    if ((grid >= C8724_DISPLAY_HEIGHT) || (segment >= C8724_DISPLAY_WIDTH))
    {
        gs_handle.debug_print("c8724: pixel coordinate is invalid.\n");
        return 1U;
    }
    res = c8724_display_set_pixel(&gs_handle, grid, segment, pwm);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: set pixel failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }

    return 0U;
}

/**
 * @brief      屏幕刷新：把内部帧缓冲发送给芯片并切换显示
 * @return     状态码
 *             - 0 成功
 *             - 1 写入或更新失败
 * @note       与 set_pixel 配合完成“先改后刷”的绘图流程
 */
uint8_t c8724_basic_refresh(void)
{
    uint8_t res;

    res = c8724_display_flush(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: refresh failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }

    return 0U;
}

/**
 * @brief      屏幕填充：用同一 PWM 值填满整屏并立即显示
 * @param[in]  pwm PWM 值，0~255（0xFF 全亮，0x00 全灭）
 * @return     状态码
 *             - 0 成功
 *             - 1 填充或刷新失败
 * @note       等价于“填充帧缓冲 + 刷新”，一步到位
 */
uint8_t c8724_basic_fill(uint8_t pwm)
{
    uint8_t res;

    res = c8724_display_fill(&gs_handle, pwm);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: fill failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }
    res = c8724_display_flush(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: refresh failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }

    return 0U;
}

/**
 * @brief      清屏：帧缓冲和屏幕同时清零
 * @return     状态码
 *             - 0 成功
 *             - 1 清屏失败
 * @note       芯片 SRAM 与内部帧缓冲都会被清空
 */
uint8_t c8724_basic_clear(void)
{
    uint8_t res;

    res = c8724_display_clear(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: clear failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }

    return 0U;
}

/**
 * @brief      息屏：关闭 SEG 恒流输出（OE = 0）
 * @return     状态码
 *             - 0 成功
 *             - 1 配置下发失败
 * @note       只关输出，扫描与 SRAM 数据保留，c8724_basic_screen_on 后原画面
 *             直接恢复、无需重新刷新；本接口不省电，真正低功耗请用
 *             c8724_basic_sleep
 */
uint8_t c8724_basic_screen_off(void)
{
    uint8_t res;

    gs_handle.debug_print("c8724: screen off.\n");
    c8724_SetSegOutput(&gs_handle, C8724_BOOL_FALSE);
    res = c8724_set_config(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: set config failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }

    return 0U;
}

/**
 * @brief      息屏恢复：重新打开 SEG 恒流输出（OE = 1）
 * @return     状态码
 *             - 0 成功
 *             - 1 配置下发失败
 * @note       与 c8724_basic_screen_off 配对使用
 */
uint8_t c8724_basic_screen_on(void)
{
    uint8_t res;

    gs_handle.debug_print("c8724: screen on.\n");
    c8724_SetSegOutput(&gs_handle, C8724_BOOL_TRUE);
    res = c8724_set_config(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: set config failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }

    return 0U;
}

/**
 * @brief      低功耗模式：清屏 → 使能休眠 → 发送休眠指令
 * @return     状态码
 *             - 0 成功
 *             - 1 清屏、配置或休眠指令失败
 * @note       手册要求休眠前必须先送入全 0 数据帧，因此本函数会先清屏；
 *             休眠期间屏幕为黑屏，唤醒后需重新填充内容。休眠电流典型 300 µA
 */
uint8_t c8724_basic_sleep(void)
{
    uint8_t res;

    gs_handle.debug_print("c8724: enter sleep mode.\n");

    /* 手册规定：休眠指令前必须先送入全 0 数据帧 */
    res = c8724_display_clear(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: clear failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }
    /* 打开休眠使能，否则休眠指令被驱动拒绝（返回 4） */
    c8724_SetSleepEnable(&gs_handle, C8724_BOOL_TRUE);
    res = c8724_set_config(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: set config failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }
    res = c8724_sleep(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: sleep failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }

    return 0U;
}

/**
 * @brief      退出低功耗模式：发送唤醒指令 → 关闭休眠使能
 * @return     状态码
 *             - 0 成功
 *             - 1 唤醒或配置下发失败
 * @note       唤醒后屏幕保持休眠前的全 0 黑屏状态；手册规定睡眠中写入非 0
 *             数据也会硬件自动唤醒，建议统一走本接口恢复
 */
uint8_t c8724_basic_wakeup(void)
{
    uint8_t res;

    gs_handle.debug_print("c8724: wake up.\n");
    res = c8724_wake(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: wake failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }
    /* 关闭休眠使能，回到常规工作配置 */
    c8724_SetSleepEnable(&gs_handle, C8724_BOOL_FALSE);
    res = c8724_set_config(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: set config failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }

    return 0U;
}

/**
 * @brief      反初始化 basic 示例
 * @return     状态码
 *             - 0 成功
 *             - 1 反初始化失败
 * @note       驱动内部会先清屏，再使能休眠并反初始化 SPI（五段式的 ⑤ 段）
 */
uint8_t c8724_basic_deinit(void)
{
    uint8_t res;

    /* 反初始化芯片 */
    res = c8724_deinit(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: deinit failed.\n");
        return 1U;
    }

    return 0U;
}
