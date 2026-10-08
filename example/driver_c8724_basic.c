/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724_basic.c
 * @brief     driver c8724 basic example source file
 * @version   1.2.0
 * @author    LQ
 * @date      2026-10-05
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author  <th>Description
 * <tr><td>2026/10/05  <td>1.0.0    <td>LQ      <td>first upload
 * <tr><td>2026/10/05  <td>1.1.0    <td>LQ      <td>config flow moved to field macros and one set_config
 * <tr><td>2026/10/08  <td>1.2.0    <td>LQ      <td>applied the proven configuration preset (RCS = 0)
 * </table>
 */

#include "driver_c8724_basic.h"
#include "driver_c8724.h"
#include "driver_c8724_interface.h"

static c8724_handle_t gs_handle;

/**
 * @brief      initialize the basic example
 * @return     status code
 *             - 0 success
 *             - 1 initialization or configuration failed
 * @note       platform interface functions must be implemented before use
 */
uint8_t c8724_basic_init(void)
{
    uint8_t config[4];
    uint8_t res;

    /* link functions */
    DRIVER_C8724_LINK_INIT(&gs_handle, c8724_handle_t);
    DRIVER_C8724_LINK_SPI_INIT(&gs_handle, c8724_interface_spi_init);
    DRIVER_C8724_LINK_SPI_DEINIT(&gs_handle, c8724_interface_spi_deinit);
    DRIVER_C8724_LINK_SPI_WRITE_CMD(&gs_handle, c8724_interface_spi_write_cmd);
    DRIVER_C8724_LINK_DEBUG_PRINT(&gs_handle, c8724_interface_debug_print);

    /* initialize the chip */
    res = c8724_init(&gs_handle);
    if (res != 0U)
    {
        c8724_interface_debug_print("c8724: init failed.\n");
        return 1U;
    }

    if (res != 0U)
    {
        c8724_interface_debug_print("c8724: configure failed.\n");
        (void)c8724_deinit(&gs_handle);
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
uint8_t c8724_basic_display(const uint8_t *data, uint16_t len)
{
    uint8_t res;

    /* write the next frame */
    res = c8724_write_display(&gs_handle, data, len);
    if (res != 0U)
    {
        c8724_interface_debug_print("c8724: write display failed.\n");
        (void)c8724_deinit(&gs_handle);
        return 1U;
    }
    /* switch to the next frame */
    res = c8724_display_update(&gs_handle);
    if (res != 0U)
    {
        c8724_interface_debug_print("c8724: display update failed.\n");
        (void)c8724_deinit(&gs_handle);
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
uint8_t c8724_basic_deinit(void)
{
    uint8_t res;

    /* deinitialize the chip */
    res = c8724_deinit(&gs_handle);
    if (res != 0U)
    {
        c8724_interface_debug_print("c8724: deinit failed.\n");
        return 1U;
    }

    return 0U;
}
