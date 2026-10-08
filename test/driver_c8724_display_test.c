/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724_display_test.c
 * @brief     driver c8724 display test source file
 * @version   1.2.0
 * @author    LQ
 * @date      2026-10-05
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author  <th>Description
 * <tr><td>2026/10/05  <td>1.0.0    <td>LQ      <td>first upload
 * <tr><td>2026/10/05  <td>1.1.0    <td>LQ      <td>config flow moved to field macros, frame buffer test added
 * <tr><td>2026/10/08  <td>1.2.0    <td>LQ      <td>switched to the proven configuration preset (RCS = 0)
 * </table>
 */

#include "driver_c8724_display_test.h"
#include "driver_c8724_interface.h"

static c8724_handle_t gs_handle; /**< c8724 test handle */

void effect_full_blink(uint32_t on_ms, uint32_t off_ms, uint32_t times);
void effect_running_light(uint32_t step_ms, uint32_t rounds);
void effect_breath(uint32_t step_ms, uint32_t rounds);
void effect_scan_column(uint32_t step_ms, uint32_t rounds);
void effect_addr_diagonal(uint32_t step_ms, uint32_t rounds);
void effect_addr_max_entries(uint32_t step_ms, uint32_t rounds);

#define delay_ms(ms) gs_handle.delay_ms(ms)

/**
 * @addtogroup c8724_test_driver
 * @{
 */

/**
 * @brief      test the frame buffer, sequential write, addressed write and update
 * @param[in]  address instruction chip select address
 * @param[in]  times number of display pattern iterations
 * @return     status code
 *             - 0 success
 *             - 1 test failed
 * @note       requires a physical C8724 and platform SPI interface implementation
 */
uint8_t c8724_display_test(c8724_address_t address, uint32_t times)
{
    c8724_info_t info;
    uint8_t config[4];
    uint8_t pixel_address[4] = {0x00U, 0x01U, 0x10U, 0x12U};
    uint8_t pixel_data[4] = {0xFFU, 0xFFU, 0xFFU, 0x20U};
    uint8_t res;
    uint8_t grid;
    uint8_t segment;
    uint32_t j;

    if (times == 0U)
    {
        gs_handle.debug_print("c8724: display test count is zero.\n");
        return 1U;
    }
    if ((uint8_t)address > (uint8_t)C8724_ADDRESS_3)
    {
        gs_handle.debug_print("c8724: display test address is invalid.\n");
        return 1U;
    }

    /* link functions */
    DRIVER_C8724_LINK_INIT(&gs_handle, c8724_handle_t);
    DRIVER_C8724_LINK_SPI_INIT(&gs_handle, c8724_interface_spi_init);
    DRIVER_C8724_LINK_SPI_DEINIT(&gs_handle, c8724_interface_spi_deinit);
    DRIVER_C8724_LINK_SPI_WRITE_CMD(&gs_handle, c8724_interface_spi_write_cmd);
    DRIVER_C8724_LINK_DELAY_MS(&gs_handle, c8724_interface_delay_ms);
    DRIVER_C8724_LINK_DEBUG_PRINT(&gs_handle, c8724_interface_debug_print);

    /* print chip information */
    res = c8724_info(&info);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: get chip information failed.\n");
        return 1U;
    }
    gs_handle.debug_print("c8724: chip is %s.\n", info.chip_name);
    gs_handle.debug_print("c8724: manufacturer is %s.\n", info.manufacturer_name);
    gs_handle.debug_print("c8724: interface is %s.\n", info.interface);
    gs_handle.debug_print("c8724: supply voltage is %0.1fV to %0.1fV.\n",
                                (double)info.supply_voltage_min_v, (double)info.supply_voltage_max_v);
    gs_handle.debug_print("c8724: max SEG current is %0.1fmA.\n", (double)info.max_current_ma);

    /* initialize the chip */
    res = c8724_init(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: display test init failed.\n");
        return 1U;
    }

    /* ===================== 基本信息 ===================== */
    c8724_set_address(&gs_handle, address);
    c8724_set_broadcast(&gs_handle, C8724_BOOL_FALSE); /* 单芯片用地址寻址，关闭广播 */

    /* ===================== 寄存器 1：SEG 引脚与电流 ===================== */
    c8724_SetSeg11PinMode(&gs_handle, C8724_SEG_PIN_LED_OUTPUT); /* SEG11 作恒流输出 */
    c8724_SetSeg12PinMode(&gs_handle, C8724_SEG_PIN_LED_OUTPUT); /* SEG12 作恒流输出 */
    c8724_SetGccMa(&gs_handle, 30);                              /* 30 mA；省电可降到 15 mA */

    /* ===================== 寄存器 2：时钟、扫描、更新 ===================== */
    c8724_SetClockSource(&gs_handle, C8724_CLOCK_SOURCE_INTERNAL); /* 用内部 RCLK，省电 */
    c8724_SetScan(&gs_handle, C8724_SCAN_8_ROWS);                  /* 8 行全开 */
    c8724_SetTestMode(&gs_handle, C8724_BOOL_FALSE);               /* 正常模式 */
    c8724_SetUpdateMode(&gs_handle, C8724_UPDATE_MODE_SCAN_END);   /* 扫描结束更新，画面稳 */

    /* ===================== 寄存器 3：保护、时钟、输出 ===================== */
    c8724_SetOtp125(&gs_handle, C8724_BOOL_TRUE);                   /* 打开 125°C 过温保护 */
    c8724_SetResistorSelect(&gs_handle, C8724_RES_SELECT_INTERNAL); /* RCS=0，内部电阻（必选） */
    c8724_SetInternalClock(&gs_handle, C8724_INTERNAL_CLOCK_4_MHZ); /* 4 MHz；省电可换 2 MHz */
    c8724_SetSegOutput(&gs_handle, C8724_BOOL_TRUE);                /* 打开 SEG 输出 */
    c8724_SetGhostRemoval(&gs_handle, C8724_GHOST_REMOVAL_WEAK);    /* 弱消影，省电 */
    c8724_SetLineBlanking(&gs_handle, C8724_LINE_BLANKING_8T);      /* 8T，折中 */

    /* ===================== 寄存器 4：休眠、复位、保护 ===================== */
    c8724_SetSleepEnable(&gs_handle, C8724_BOOL_FALSE);       /* 手动休眠不使能 */
    c8724_SetAutoSleep(&gs_handle, C8724_BOOL_TRUE);          /* 全 0 数据自动休眠，省电 */
    c8724_SetGlobalResetEnable(&gs_handle, C8724_BOOL_FALSE); /* 不用 */
    c8724_SetSoftResetEnable(&gs_handle, C8724_BOOL_FALSE);   /* 不用 */
    c8724_SetClockDouble(&gs_handle, C8724_BOOL_FALSE);       /* 关闭 2 倍频，省电 */
    c8724_SetOtp150(&gs_handle, C8724_BOOL_TRUE);             /* 打开 150°C 过温保护 */
    c8724_SetPowerDownReset(&gs_handle, C8724_BOOL_TRUE);     /* 掉电复位使能，稳定 */

    /* ===================== 一次性提交所有配置到芯片 ===================== */
    if (c8724_set_config(&gs_handle) != 0U)
    {
        /* 配置下发失败，芯片可能处于半包状态，建议重新初始化 */
    }

    /* 全屏闪烁 3 次，亮 300 ms，灭 300 ms */
    gs_handle.debug_print("c8724: effect start - full blink.\n");
    effect_full_blink(300U, 300U, 3U);
    gs_handle.debug_print("c8724: effect done  - full blink.\n");
    delay_ms(500U);

    /* 流水灯环绕 3 圈，每步 80 ms */
    gs_handle.debug_print("c8724: effect start - running light.\n");
    effect_running_light(80U, 3U);
    gs_handle.debug_print("c8724: effect done  - running light.\n");
    delay_ms(500U);

    /* 呼吸灯 2 个完整周期，每步 3 ms */
    gs_handle.debug_print("c8724: effect start - breath.\n");
    effect_breath(3U, 2U);
    gs_handle.debug_print("c8724: effect done  - breath.\n");
    delay_ms(500U);

    /* 横向扫描 3 个来回，每步 60 ms */
    gs_handle.debug_print("c8724: effect start - scan column.\n");
    effect_scan_column(60U, 3U);
    gs_handle.debug_print("c8724: effect done  - scan column.\n");

    /* 收尾清屏 */
    gs_handle.debug_print("c8724: effect start - clear.\n");
    (void)c8724_display_clear(&gs_handle);
    gs_handle.debug_print("c8724: effect done  - clear.\n");

    /* 单点按地址写：对角线 */
    gs_handle.debug_print("c8724: effect start - addr diagonal.\n");
    effect_addr_diagonal(120U, 2U);
    gs_handle.debug_print("c8724: effect done  - addr diagonal.\n");
    delay_ms(500U);

    /* 批量按地址写：72 条目上限 */
    gs_handle.debug_print("c8724: effect start - addr max entries.\n");
    effect_addr_max_entries(200U, 2U);
    gs_handle.debug_print("c8724: effect done  - addr max entries.\n");

    /* 收尾清屏 */
    gs_handle.debug_print("c8724: effect start - clear.\n");
    (void)c8724_display_clear(&gs_handle);
    gs_handle.debug_print("c8724: effect done  - clear.\n");

    /* clear the matrix and close the bus */
    res = c8724_deinit(&gs_handle);
    if (res != 0U)
    {
        gs_handle.debug_print("c8724: display test cleanup failed.\n");
        return 1U;
    }

    return 0U;
}

/* ==================================================================
 * 效果 1：全屏闪烁
 * 全部 96 颗 LED 同时全亮 / 全灭，用于最基础的亮灭验证
 * ================================================================== */
void effect_full_blink(uint32_t on_ms, uint32_t off_ms, uint32_t times)
{
    uint32_t i;

    for (i = 0U; i < times; i++)
    {
        (void)c8724_display_fill(&gs_handle, 0xFFU);
        (void)c8724_display_flush(&gs_handle);
        delay_ms(on_ms);

        (void)c8724_display_fill(&gs_handle, 0x00U);
        (void)c8724_display_flush(&gs_handle);
        delay_ms(off_ms);
    }
}

/* ==================================================================
 * 效果 2：循环流水灯（单点沿点阵外圈环绕）
 * 路线：顶行 左→右，右列 上→下，底行 右→左，左列 下→上
 * ================================================================== */
void effect_running_light(uint32_t step_ms, uint32_t rounds)
{
    uint8_t grid;
    uint8_t seg;
    uint32_t r;

    for (r = 0U; r < rounds; r++)
    {
        /* 顶行 左 -> 右 */
        for (seg = 0U; seg < C8724_DISPLAY_WIDTH; seg++)
        {
            (void)c8724_display_fill(&gs_handle, 0x00U);
            (void)c8724_display_set_pixel(&gs_handle, 0U, seg, 0xFFU);
            (void)c8724_display_flush(&gs_handle);
            delay_ms(step_ms);
        }
        /* 右列 上 -> 下（跳过右上角） */
        for (grid = 1U; grid < C8724_DISPLAY_HEIGHT; grid++)
        {
            (void)c8724_display_fill(&gs_handle, 0x00U);
            (void)c8724_display_set_pixel(&gs_handle, grid,
                                          (uint8_t)(C8724_DISPLAY_WIDTH - 1U), 0xFFU);
            (void)c8724_display_flush(&gs_handle);
            delay_ms(step_ms);
        }
        /* 底行 右 -> 左（跳过右下角） */
        for (seg = (uint8_t)(C8724_DISPLAY_WIDTH - 1U); seg > 0U; seg--)
        {
            (void)c8724_display_fill(&gs_handle, 0x00U);
            (void)c8724_display_set_pixel(&gs_handle,
                                          (uint8_t)(C8724_DISPLAY_HEIGHT - 1U),
                                          (uint8_t)(seg - 1U), 0xFFU);
            (void)c8724_display_flush(&gs_handle);
            delay_ms(step_ms);
        }
        /* 左列 下 -> 上（跳过左下角和左上角） */
        for (grid = (uint8_t)(C8724_DISPLAY_HEIGHT - 1U); grid > 1U; grid--)
        {
            (void)c8724_display_fill(&gs_handle, 0x00U);
            (void)c8724_display_set_pixel(&gs_handle, (uint8_t)(grid - 1U), 0U, 0xFFU);
            (void)c8724_display_flush(&gs_handle);
            delay_ms(step_ms);
        }
    }
}

/* ==================================================================
 * 效果 3：呼吸灯（全屏 PWM 同步渐亮渐灭）
 * step_ms 建议 2 ~ 4 ms，一次完整呼吸约 1 ~ 2 秒
 * ================================================================== */
void effect_breath(uint32_t step_ms, uint32_t rounds)
{
    int32_t v;
    uint32_t r;

    for (r = 0U; r < rounds; r++)
    {
        /* 渐亮 0 -> 255 */
        for (v = 0; v <= 255; v++)
        {
            (void)c8724_display_fill(&gs_handle, (uint8_t)v);
            (void)c8724_display_flush(&gs_handle);
            delay_ms(step_ms);
        }
        /* 渐灭 255 -> 0 */
        for (v = 255; v >= 0; v--)
        {
            (void)c8724_display_fill(&gs_handle, (uint8_t)v);
            (void)c8724_display_flush(&gs_handle);
            delay_ms(step_ms);
        }
    }
}

/* ==================================================================
 * 效果 4：横向扫描（竖条从左向右扫过，再反向扫回）
 * 每次只点亮一列 8 颗 LED，视觉上是一条竖线在移动
 * ================================================================== */
void effect_scan_column(uint32_t step_ms, uint32_t rounds)
{
    uint8_t grid;
    uint8_t seg;
    uint32_t r;

    for (r = 0U; r < rounds; r++)
    {
        /* 左 -> 右 */
        for (seg = 0U; seg < C8724_DISPLAY_WIDTH; seg++)
        {
            (void)c8724_display_fill(&gs_handle, 0x00U);
            for (grid = 0U; grid < C8724_DISPLAY_HEIGHT; grid++)
            {
                (void)c8724_display_set_pixel(&gs_handle, grid, seg, 0xFFU);
            }
            (void)c8724_display_flush(&gs_handle);
            delay_ms(step_ms);
        }
        /* 右 -> 左 */
        for (seg = (uint8_t)(C8724_DISPLAY_WIDTH - 1U); seg > 0U; seg--)
        {
            (void)c8724_display_fill(&gs_handle, 0x00U);
            for (grid = 0U; grid < C8724_DISPLAY_HEIGHT; grid++)
            {
                (void)c8724_display_set_pixel(&gs_handle, grid, (uint8_t)(seg - 1U), 0xFFU);
            }
            (void)c8724_display_flush(&gs_handle);
            delay_ms(step_ms);
        }
    }
}

/* 地址编码：(GRID 索引 << 4) | SEG 索引 */
#define C8724_ADDR(grid, seg) ((uint8_t)(((uint8_t)(grid) << 4U) | (uint8_t)(seg)))

/* ==================================================================
 * 效果 1：单点按地址写入
 * 每次只写一个地址，沿对角线逐点点亮，验证最小粒度写
 * ================================================================== */
void effect_addr_diagonal(uint32_t step_ms, uint32_t rounds)
{
    uint8_t address[1];
    uint8_t data[1];
    uint8_t clr_addr[C8724_DISPLAY_DATA_MAX];
    uint8_t clr_data[C8724_DISPLAY_DATA_MAX];
    uint8_t i;
    uint8_t grid;
    uint8_t seg;
    uint16_t n;
    uint32_t r;

    for (r = 0U; r < rounds; r++)
    {
        /* 全屏清 0：把所有 96 个地址一次打包写入 */
        n = 0U;
        for (grid = 0U; grid < C8724_DISPLAY_HEIGHT; grid++)
        {
            for (seg = 0U; seg < C8724_DISPLAY_WIDTH; seg++)
            {
                clr_addr[n] = C8724_ADDR(grid, seg);
                clr_data[n] = 0x00U;
                n++;
            }
        }
        (void)c8724_write_display_address(&gs_handle, clr_addr, clr_data, n);
        (void)c8724_display_update(&gs_handle);

        /* 逐点写对角线：GRID i / SEG i */
        for (i = 0U; i < C8724_DISPLAY_HEIGHT; i++)
        {
            address[0] = C8724_ADDR(i, i);
            data[0] = 0xFFU;
            (void)c8724_write_display_address(&gs_handle, address, data, 1U);
            (void)c8724_display_update(&gs_handle);
            delay_ms(step_ms);
        }
        delay_ms(step_ms * 4U);
    }
}

/* ==================================================================
 * 效果 2：批量按地址写入（上限 72 条目）
 * 一次写满 C8724_ADDRESS_DATA_MAX 条目，验证满包发送与边界
 * ================================================================== */
void effect_addr_max_entries(uint32_t step_ms, uint32_t rounds)
{
    uint8_t address[C8724_ADDRESS_DATA_MAX];
    uint8_t data[C8724_ADDRESS_DATA_MAX];
    uint8_t grid;
    uint8_t seg;
    uint16_t n;
    uint32_t r;

    for (r = 0U; r < rounds; r++)
    {
        /* 填充前 72 个地址（GRID0..GRID5 整行 + GRID6 前 6 个） */
        n = 0U;
        for (grid = 0U; (grid < C8724_DISPLAY_HEIGHT) && (n < C8724_ADDRESS_DATA_MAX); grid++)
        {
            for (seg = 0U; (seg < C8724_DISPLAY_WIDTH) && (n < C8724_ADDRESS_DATA_MAX); seg++)
            {
                address[n] = C8724_ADDR(grid, seg);
                data[n] = 0xFFU;
                n++;
            }
        }
        (void)c8724_write_display_address(&gs_handle, address, data, n);
        (void)c8724_display_update(&gs_handle);
        delay_ms(step_ms);

        /* 清掉这 72 个地址 */
        for (n = 0U; n < C8724_ADDRESS_DATA_MAX; n++)
        {
            data[n] = 0x00U;
        }
        (void)c8724_write_display_address(&gs_handle, address, data, C8724_ADDRESS_DATA_MAX);
        (void)c8724_display_update(&gs_handle);
        delay_ms(step_ms);
    }
}

/** @} */
