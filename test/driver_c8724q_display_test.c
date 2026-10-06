/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724q_display_test.c
 * @brief     driver c8724q display test source file
 * @version   1.1.0
 * @author    LQ
 * @date      2026-10-05
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author  <th>Description
 * <tr><td>2026/10/05  <td>1.0.0    <td>LQ      <td>first upload
 * <tr><td>2026/10/05  <td>1.1.0    <td>LQ      <td>config flow moved to field macros, frame buffer test added
 * </table>
 */

#include "driver_c8724q_display_test.h"
#include "driver_c8724q_interface.h"

static c8724q_handle_t gs_handle; /**< c8724q test handle */

/**
 * @addtogroup c8724q_test_driver
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
uint8_t c8724q_display_test(c8724q_address_t address, uint32_t times)
{
    c8724q_info_t info;
    uint8_t config[4];
    uint8_t pixel_address[4] = { 0x00U, 0x0BU, 0x70U, 0x7BU };
    uint8_t pixel_data[4] = { 0x20U, 0x00U, 0x00U, 0x20U };
    uint8_t pwm;
    uint8_t res;
    uint8_t grid;
    uint8_t segment;
    uint32_t j;

    if (times == 0U)
    {
        c8724q_interface_debug_print("c8724q: display test count is zero.\n");
        return 1U;
    }
    if ((uint8_t)address > (uint8_t)C8724Q_ADDRESS_3)
    {
        c8724q_interface_debug_print("c8724q: display test address is invalid.\n");
        return 1U;
    }

    /* link functions */
    DRIVER_C8724Q_LINK_INIT(&gs_handle, c8724q_handle_t);
    DRIVER_C8724Q_LINK_SPI_INIT(&gs_handle, c8724q_interface_spi_init);
    DRIVER_C8724Q_LINK_SPI_DEINIT(&gs_handle, c8724q_interface_spi_deinit);
    DRIVER_C8724Q_LINK_SPI_WRITE_CMD(&gs_handle, c8724q_interface_spi_write_cmd);
    DRIVER_C8724Q_LINK_DEBUG_PRINT(&gs_handle, c8724q_interface_debug_print);

    /* print chip information */
    res = c8724q_info(&info);
    if (res != 0U)
    {
        c8724q_interface_debug_print("c8724q: get chip information failed.\n");
        return 1U;
    }
    c8724q_interface_debug_print("c8724q: chip is %s.\n", info.chip_name);
    c8724q_interface_debug_print("c8724q: manufacturer is %s.\n", info.manufacturer_name);
    c8724q_interface_debug_print("c8724q: interface is %s.\n", info.interface);
    c8724q_interface_debug_print("c8724q: supply voltage is %0.1fV to %0.1fV.\n",
                                 (double)info.supply_voltage_min_v, (double)info.supply_voltage_max_v);
    c8724q_interface_debug_print("c8724q: max SEG current is %0.1fmA.\n", (double)info.max_current_ma);

    /* initialize the chip */
    res = c8724q_init(&gs_handle);
    if (res != 0U)
    {
        c8724q_interface_debug_print("c8724q: display test init failed.\n");
        return 1U;
    }
    if ((c8724q_get_config(&gs_handle, config) != 0U) ||
        (config[0] != 0x3FU) || (config[1] != 0x00U) ||
        (config[2] != 0xF0U) || (config[3] != 0x02U))
    {
        c8724q_interface_debug_print("c8724q: verify reset configuration failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }
    res = c8724q_set_address(&gs_handle, address);
    if (res == 0U)
    {
        res = c8724q_set_broadcast(&gs_handle, C8724Q_BOOL_FALSE);
    }
    if (res != 0U)
    {
        c8724q_interface_debug_print("c8724q: select chip address failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }

    /* configure the LED matrix through field macros and one set_config call */
    C8724Q_SET_SEG11_MODE(config, C8724Q_SEG_PIN_LED_OUTPUT);
    C8724Q_SET_SEG12_MODE(config, C8724Q_SEG_PIN_LED_OUTPUT);
    C8724Q_SET_GLOBAL_CURRENT_GAIN(config, 0U);
    C8724Q_SET_SCAN(config, C8724Q_SCAN_8_ROWS);
    C8724Q_SET_OUTPUT_ENABLE(config, C8724Q_BOOL_TRUE);
    res = c8724q_set_config(&gs_handle, config);
    if ((res == 0U) &&
        ((c8724q_get_config(&gs_handle, config) != 0U) ||
         (config[0] != 0xC0U) || (config[1] != 0x1CU) ||
         (config[2] != 0xF8U) || (config[3] != 0x02U)))
    {
        res = 1U;
    }
    if (res != 0U)
    {
        c8724q_interface_debug_print("c8724q: configure failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }

    /* reserved bits must still be rejected by set_config */
    config[1] = 0x20U;
    if (c8724q_set_config(&gs_handle, config) != 4U)
    {
        c8724q_interface_debug_print("c8724q: reserved configuration bit check failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }

    /* out-of-range pixels are rejected by the frame buffer */
    if ((c8724q_display_set_pixel(&gs_handle, C8724Q_DISPLAY_HEIGHT, 0U, 0U) != 4U) ||
        (c8724q_display_set_pixel(&gs_handle, 0U, C8724Q_DISPLAY_WIDTH, 0U) != 4U))
    {
        c8724q_interface_debug_print("c8724q: frame buffer range check failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }

    /* check sequential display input length validation */
    if (c8724q_write_display(&gs_handle, config, (uint16_t)(C8724Q_DISPLAY_DATA_MAX - 1U)) != 4U)
    {
        c8724q_interface_debug_print("c8724q: sequential display length check failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }

    for (j = 0U; j < times; j++)
    {
        /* fill the frame buffer with a low-brightness GRID-major gradient */
        res = c8724q_display_fill(&gs_handle, (uint8_t)(j & 0x1FU));
        for (grid = 0U; (grid < C8724Q_DISPLAY_HEIGHT) && (res == 0U); grid++)
        {
            for (segment = 0U; (segment < C8724Q_DISPLAY_WIDTH) && (res == 0U); segment++)
            {
                res = c8724q_display_set_pixel(&gs_handle, grid, segment,
                                               (uint8_t)(((grid * C8724Q_DISPLAY_WIDTH) +
                                                          segment + j) & 0x1FU));
            }
        }
        if (res == 0U)
        {
            res = c8724q_display_get_pixel(&gs_handle, 1U, 1U, &pwm);
            if ((res == 0U) && (pwm != (uint8_t)((C8724Q_DISPLAY_WIDTH + 1U + j) & 0x1FU)))
            {
                res = 1U;
            }
        }
        if (res == 0U)
        {
            res = c8724q_display_flush(&gs_handle);
        }
        if (res != 0U)
        {
            c8724q_interface_debug_print("c8724q: frame buffer test failed.\n");
            (void)c8724q_deinit(&gs_handle);
            return 1U;
        }

        /* update four corner pixels by SRAM address */
        res = c8724q_write_display_address(&gs_handle, pixel_address, pixel_data, 4U);
        if (res == 0U)
        {
            res = c8724q_display_update(&gs_handle);
        }
        if (res != 0U)
        {
            c8724q_interface_debug_print("c8724q: addressed display test failed.\n");
            (void)c8724q_deinit(&gs_handle);
            return 1U;
        }
        c8724q_interface_debug_print("c8724q: display test iteration %lu passed.\n",
                                     (unsigned long)(j + 1U));
    }

    /* clear the matrix and close the bus */
    res = c8724q_deinit(&gs_handle);
    if (res != 0U)
    {
        c8724q_interface_debug_print("c8724q: display test cleanup failed.\n");
        return 1U;
    }

    return 0U;
}

/** @} */
