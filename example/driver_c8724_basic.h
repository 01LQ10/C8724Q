/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724_basic.h
 * @brief     driver c8724 basic example header file
 * @version   1.3.0
 * @author    LQ
 * @date      2026-10-05
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author  <th>Description
 * <tr><td>2026/10/05  <td>1.0.0    <td>LQ      <td>first upload
 * <tr><td>2026/10/05  <td>1.1.0    <td>LQ      <td>config flow moved to field macros and one set_config
 * <tr><td>2026/10/08  <td>1.2.0    <td>LQ      <td>default GCC raised to the proven value 63
 * <tr><td>2026/10/08  <td>1.3.0    <td>LQ      <td>screen control API added (set pixel / refresh / fill / clear / screen off / sleep), five-stage init flow
 * </table>
 */

#ifndef DRIVER_C8724_BASIC_H
#define DRIVER_C8724_BASIC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup c8724_example_driver c8724 example driver function
 * @brief    c8724 example driver modules
 * @{
 */

/* ===================== 默认配置宏 =====================
 * 与实证配置 FF 1D 3D 05 对齐；省电场景可自行调低 GCC 或时钟频率。
 * 宏在调用点展开，使用前需先包含 driver_c8724.h。
 */
#define C8724_BASIC_DEFAULT_SEG11_MODE             C8724_SEG_PIN_LED_OUTPUT   /**< SEG11 作 LED 恒流输出 */
#define C8724_BASIC_DEFAULT_SEG12_MODE             C8724_SEG_PIN_LED_OUTPUT   /**< SEG12 作 LED 恒流输出 */
#define C8724_BASIC_DEFAULT_GCC                    63U                        /**< 实证电流增益（最大档，已验证可正常点亮） */
#define C8724_BASIC_DEFAULT_CLOCK_SOURCE           C8724_CLOCK_SOURCE_INTERNAL /**< 内部 RCLK，无需外部时钟 */
#define C8724_BASIC_DEFAULT_SCAN                   C8724_SCAN_8_ROWS          /**< 8 行全开（顺序写 N = 96 字节） */
#define C8724_BASIC_DEFAULT_UPDATE_MODE            C8724_UPDATE_MODE_SCAN_END /**< 扫描结束后更新，画面稳定 */
#define C8724_BASIC_DEFAULT_CLOCK                  C8724_INTERNAL_CLOCK_8_MHZ /**< 内部时钟 8 MHz（省电可降 2/4 MHz） */
#define C8724_BASIC_DEFAULT_OUTPUT                 C8724_BOOL_TRUE            /**< 打开 SEG 恒流输出 */
#define C8724_BASIC_DEFAULT_GHOST                  C8724_GHOST_REMOVAL_STRONG /**< 强消影 */
#define C8724_BASIC_DEFAULT_BLANKING               C8724_LINE_BLANKING_8T     /**< 换行时间 8T */
#define C8724_BASIC_DEFAULT_AUTO_SLEEP             C8724_BOOL_FALSE           /**< 自动休眠关闭，由 basic_sleep 主动管理 */
#define C8724_BASIC_DEFAULT_CLK2X                  C8724_BOOL_TRUE            /**< 扫描时钟 2 倍频（实证配置值） */

/**
 * @brief      初始化 basic 示例
 * @return     状态码
 *             - 0 成功
 *             - 1 初始化或配置失败
 * @note       五段式流程的 ①②③ 段：LINK 绑定 → info 打印 → init + 默认配置编排；
 *             使用前必须先实现平台 interface 函数
 */
uint8_t c8724_basic_init(void);

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
uint8_t c8724_basic_display(const uint8_t *data, uint16_t len);

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
uint8_t c8724_basic_set_pixel(uint8_t grid, uint8_t segment, uint8_t pwm);

/**
 * @brief      屏幕刷新：把内部帧缓冲发送给芯片并切换显示
 * @return     状态码
 *             - 0 成功
 *             - 1 写入或更新失败
 * @note       与 set_pixel 配合完成“先改后刷”的绘图流程
 */
uint8_t c8724_basic_refresh(void);

/**
 * @brief      屏幕填充：用同一 PWM 值填满整屏并立即显示
 * @param[in]  pwm PWM 值，0~255（0xFF 全亮，0x00 全灭）
 * @return     状态码
 *             - 0 成功
 *             - 1 填充或刷新失败
 * @note       等价于“填充帧缓冲 + 刷新”，一步到位
 */
uint8_t c8724_basic_fill(uint8_t pwm);

/**
 * @brief      清屏：帧缓冲和屏幕同时清零
 * @return     状态码
 *             - 0 成功
 *             - 1 清屏失败
 * @note       芯片 SRAM 与内部帧缓冲都会被清空
 */
uint8_t c8724_basic_clear(void);

/**
 * @brief      息屏：关闭 SEG 恒流输出（OE = 0）
 * @return     状态码
 *             - 0 成功
 *             - 1 配置下发失败
 * @note       只关输出，扫描与 SRAM 数据保留，c8724_basic_screen_on 后原画面
 *             直接恢复、无需重新刷新；本接口不省电，真正低功耗请用
 *             c8724_basic_sleep
 */
uint8_t c8724_basic_screen_off(void);

/**
 * @brief      息屏恢复：重新打开 SEG 恒流输出（OE = 1）
 * @return     状态码
 *             - 0 成功
 *             - 1 配置下发失败
 * @note       与 c8724_basic_screen_off 配对使用
 */
uint8_t c8724_basic_screen_on(void);

/**
 * @brief      低功耗模式：清屏 → 使能休眠 → 发送休眠指令
 * @return     状态码
 *             - 0 成功
 *             - 1 清屏、配置或休眠指令失败
 * @note       手册要求休眠前必须先送入全 0 数据帧，因此本函数会先清屏；
 *             休眠期间屏幕为黑屏，唤醒后需重新填充内容。休眠电流典型 300 µA
 */
uint8_t c8724_basic_sleep(void);

/**
 * @brief      退出低功耗模式：发送唤醒指令 → 关闭休眠使能
 * @return     状态码
 *             - 0 成功
 *             - 1 唤醒或配置下发失败
 * @note       唤醒后屏幕保持休眠前的全 0 黑屏状态；手册规定睡眠中写入非 0
 *             数据也会硬件自动唤醒，建议统一走本接口恢复
 */
uint8_t c8724_basic_wakeup(void);

/**
 * @brief      反初始化 basic 示例
 * @return     状态码
 *             - 0 成功
 *             - 1 反初始化失败
 * @note       驱动内部会先清屏，再使能休眠并反初始化 SPI（五段式的 ⑤ 段）
 */
uint8_t c8724_basic_deinit(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
