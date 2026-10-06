/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724q_display_test.c
 * @brief     driver c8724q display test source file
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

#include "driver_c8724q_display_test.h"
#include "driver_c8724q_interface.h"

static c8724q_handle_t gs_handle; /**< c8724q test handle */

/**
 * @addtogroup c8724q_test_driver
 * @{
 */

/**
 * @brief      test display data writing and frame update
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
    uint8_t readback[4];
    uint8_t applied[4];
    uint8_t frame[C8724Q_DISPLAY_DATA_MAX];
    uint8_t pixel_address[4] = { 0x00U, 0x0BU, 0x70U, 0x7BU };
    uint8_t pixel_data[4] = { 0x20U, 0x00U, 0x00U, 0x20U };
    uint8_t res;
    uint16_t i;
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
    res = c8724q_get_config(&gs_handle, readback);
    if ((res != 0U) || (readback[0] != 0x3FU) || (readback[1] != 0x00U) ||
        (readback[2] != 0xF0U) || (readback[3] != 0x02U))
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

    /* stage the LED matrix configuration at minimum SEG current */
    res = c8724q_set_seg11_mode(&gs_handle, C8724Q_SEG_PIN_LED_OUTPUT);
    if (res == 0U)
    {
        res = c8724q_set_seg12_mode(&gs_handle, C8724Q_SEG_PIN_LED_OUTPUT);
    }
    if (res == 0U)
    {
        res = c8724q_set_global_current_gain(&gs_handle, 0U);
    }
    if (res == 0U)
    {
        res = c8724q_set_scan(&gs_handle, C8724Q_SCAN_8_ROWS);
    }
    if (res == 0U)
    {
        res = c8724q_set_output_enable(&gs_handle, C8724Q_BOOL_TRUE);
    }
    if (res == 0U)
    {
        res = c8724q_get_config(&gs_handle, readback);
    }
    if ((res != 0U) || (readback[0] != 0xC0U) || (readback[1] != 0x1CU) ||
        (readback[2] != 0xF8U) || (readback[3] != 0x02U))
    {
        c8724q_interface_debug_print("c8724q: stage or verify configuration failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }
    res = c8724q_get_applied_config(&gs_handle, applied);
    if ((res != 0U) || (applied[0] != 0x3FU) || (applied[1] != 0x00U) ||
        (applied[2] != 0xF0U) || (applied[3] != 0x02U))
    {
        c8724q_interface_debug_print("c8724q: verify unapplied configuration failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }
    res = c8724q_apply_config(&gs_handle);
    if (res == 0U)
    {
        res = c8724q_get_applied_config(&gs_handle, applied);
    }
    if ((res != 0U) || (applied[0] != 0xC0U) || (applied[1] != 0x1CU) ||
        (applied[2] != 0xF8U) || (applied[3] != 0x02U))
    {
        c8724q_interface_debug_print("c8724q: apply configuration failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }
    res = c8724q_set_scan(&gs_handle, (c8724q_scan_t)8U);
    if (res != 4U)
    {
        c8724q_interface_debug_print("c8724q: invalid scan value check failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }
    res = c8724q_set_config_reg2(&gs_handle, 0x20U);
    if (res != 4U)
    {
        c8724q_interface_debug_print("c8724q: reserved configuration bit check failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }

    /* check sequential display input length validation */
    res = c8724q_write_display(&gs_handle, frame, (uint16_t)(C8724Q_DISPLAY_DATA_MAX - 1U));
    if (res != 4U)
    {
        c8724q_interface_debug_print("c8724q: sequential display length check failed.\n");
        (void)c8724q_deinit(&gs_handle);
        return 1U;
    }

    for (j = 0U; j < times; j++)
    {
        /* create a low-brightness GRID-major pattern */
        for (i = 0U; i < C8724Q_DISPLAY_DATA_MAX; i++)
        {
            frame[i] = (uint8_t)((i + j) & 0x1FU);
        }
        res = c8724q_write_display(&gs_handle, frame, C8724Q_DISPLAY_DATA_MAX);
        if (res == 0U)
        {
            res = c8724q_display_update(&gs_handle);
        }
        if (res != 0U)
        {
            c8724q_interface_debug_print("c8724q: sequential display test failed.\n");
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
