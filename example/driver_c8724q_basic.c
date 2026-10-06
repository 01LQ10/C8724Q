/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724q_basic.c
 * @brief     driver c8724q basic example source file
 * @version   1.0.0
 * @author    LQ
 * @date      2026-10-05
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author  <th>Description
 * <tr><td>2026/10/05  <td>1.0.0    <td>LQ      <td>first upload
 * </table>
 */

#include "driver_c8724q_basic.h"
#include "driver_c8724q.h"
#include "driver_c8724q_interface.h"

static c8724q_handle_t gs_handle;

/**
 * @brief      initialize the basic example
 * @return     status code
 *             - 0 success
 *             - 1 initialization or configuration failed
 * @note       platform interface functions must be implemented before use
 */
uint8_t c8724q_basic_init(void)
{
    uint8_t res;

    /* link functions */
    DRIVER_C8724Q_LINK_INIT(&gs_handle, c8724q_handle_t);
    DRIVER_C8724Q_LINK_SPI_INIT(&gs_handle, c8724q_interface_spi_init);
    DRIVER_C8724Q_LINK_SPI_DEINIT(&gs_handle, c8724q_interface_spi_deinit);
    DRIVER_C8724Q_LINK_SPI_WRITE_CMD(&gs_handle, c8724q_interface_spi_write_cmd);
    DRIVER_C8724Q_LINK_DEBUG_PRINT(&gs_handle, c8724q_interface_debug_print);

    /* initialize the chip */
    res = c8724q_init(&gs_handle);
    if (res != 0U)
    {
        c8724q_interface_debug_print("c8724q: init failed.\n");
        return 1U;
    }

    /* stage a low-current 8 by 12 display configuration */
    res = c8724q_set_seg11_mode(&gs_handle, C8724Q_BASIC_DEFAULT_SEG11_MODE);
    if (res == 0U)
    {
        res = c8724q_set_seg12_mode(&gs_handle, C8724Q_BASIC_DEFAULT_SEG12_MODE);
    }
    if (res == 0U)
    {
        res = c8724q_set_global_current_gain(&gs_handle, C8724Q_BASIC_DEFAULT_GCC);
    }
    if (res == 0U)
    {
        res = c8724q_set_scan(&gs_handle, C8724Q_BASIC_DEFAULT_SCAN);
    }
    if (res == 0U)
    {
        res = c8724q_set_output_enable(&gs_handle, C8724Q_BASIC_DEFAULT_OUTPUT);
    }
    if (res == 0U)
    {
        res = c8724q_apply_config(&gs_handle);
    }
    if (res != 0U)
    {
        c8724q_interface_debug_print("c8724q: stage or apply configuration failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }

    return 0U;
}

/**
 * @brief      write and display a complete 8 by 12 PWM frame
 * @param[in]  *data pointer to 96 PWM values in GRID-major order
 * @param[in]  len number of PWM values, must be 96
 * @return     status code
 *             - 0 success
 *             - 1 write or update failed
 * @note       each PWM value ranges from 0 through 255
 */
uint8_t c8724q_basic_display(const uint8_t *data, uint16_t len)
{
    uint8_t res;

    /* write the next frame */
    res = c8724q_write_display(&gs_handle, data, len);
    if (res != 0U)
    {
        c8724q_interface_debug_print("c8724q: write display failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }
    /* switch to the next frame */
    res = c8724q_display_update(&gs_handle);
    if (res != 0U)
    {
        c8724q_interface_debug_print("c8724q: display update failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }

    return 0U;
}

/**
 * @brief      deinitialize the basic example
 * @return     status code
 *             - 0 success
 *             - 1 deinitialization failed
 * @note       clears the LEDs before sleep
 */
uint8_t c8724q_basic_deinit(void)
{
    uint8_t res;

    /* deinitialize the chip */
    res = c8724q_deinit(&gs_handle);
    if (res != 0U)
    {
        c8724q_interface_debug_print("c8724q: deinit failed.\n");
        return 1U;
    }

    return 0U;
}
