/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724q_config_test.c
 * @brief     driver c8724q configuration test source file
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

#include "driver_c8724q_config_test.h"
#include "driver_c8724q.h"

static uint8_t gs_packet[16]; /**< captured SPI packet */
static uint16_t gs_packet_len; /**< captured packet length */
static uint32_t gs_write_count; /**< SPI write callback count */
static uint8_t gs_write_fail; /**< mock SPI write failure flag */

/**
 * @brief      initialize mock SPI transport
 * @return     status code
 *             - 0 success
 * @note       none
 */
static uint8_t a_c8724q_config_test_spi_init(void)
{
    return 0U;
}

/**
 * @brief      deinitialize mock SPI transport
 * @return     status code
 *             - 0 success
 * @note       none
 */
static uint8_t a_c8724q_config_test_spi_deinit(void)
{
    return 0U;
}

/**
 * @brief      capture a mock SPI transaction
 * @param[in]  *buf pointer to data buffer
 * @param[in]  len number of data bytes
 * @return     status code
 *             - 0 success
 *             - 1 mock transaction failed
 * @note       captures one callback as one SPI transaction
 */
static uint8_t a_c8724q_config_test_spi_write(const uint8_t *buf, uint16_t len)
{
    gs_write_count++;
    if (gs_write_fail != 0U)
    {
        return 1U;
    }
    if ((buf == NULL) || (len == 0U))
    {
        return 1U;
    }
    if (len <= sizeof(gs_packet))
    {
        (void)memcpy(gs_packet, buf, len);
        gs_packet_len = len;
    }
    else
    {
        gs_packet_len = 0U;
    }

    return 0U;
}

/**
 * @brief      discard mock debug output
 * @param[in]  fmt format string
 * @param[in]  ... variable arguments
 * @return     none
 * @note       none
 */
static void a_c8724q_config_test_debug_print(const char *const fmt, ...)
{
    (void)fmt;
}

/**
 * @brief      check staged configuration values and packet encoding
 * @return     status code
 *             - 0 success
 *             - 1 test failed
 * @note       runs without physical hardware
 */
uint8_t c8724q_config_test(void)
{
    c8724q_handle_t handle;
    c8724q_seg_pin_mode_t seg_mode;
    c8724q_clock_source_t clock_source;
    c8724q_scan_t scan;
    c8724q_protection_t protection;
    c8724q_internal_clock_t internal_clock;
    c8724q_line_blanking_t line_blanking;
    c8724q_update_mode_t update_mode;
    c8724q_bool_t bool_value;
    uint8_t pending[4];
    uint8_t applied[4];
    uint8_t res;
    uint32_t write_count;

    (void)memset(&handle, 0, sizeof(handle));
    (void)memset(gs_packet, 0, sizeof(gs_packet));
    gs_packet_len = 0U;
    gs_write_count = 0U;
    gs_write_fail = 0U;
    handle.spi_init = a_c8724q_config_test_spi_init;
    handle.spi_deinit = a_c8724q_config_test_spi_deinit;
    handle.spi_write_cmd = a_c8724q_config_test_spi_write;
    handle.debug_print = a_c8724q_config_test_debug_print;

    /* initialize chip and verify datasheet reset defaults */
    res = c8724q_init(&handle);
    if ((res != 0U) || (gs_packet_len != 4U) || (gs_packet[0] != 0x5AU) ||
        (gs_packet[1] != 0xFFU) || (gs_packet[2] != 0x8BU) || (gs_packet[3] != 0xE4U))
    {
        return 1U;
    }

    /* stage all documented configuration fields without bus activity */
    write_count = gs_write_count;
    res = c8724q_set_seg11_mode(&handle, C8724Q_SEG_PIN_LED_OUTPUT);
    if (res == 0U)
    {
        res = c8724q_set_seg12_mode(&handle, C8724Q_SEG_PIN_LED_OUTPUT);
    }
    if (res == 0U)
    {
        res = c8724q_set_global_current_gain(&handle, 0U);
    }
    if (res == 0U)
    {
        res = c8724q_set_clock_source(&handle, C8724Q_CLOCK_SOURCE_INTERNAL);
    }
    if (res == 0U)
    {
        res = c8724q_set_scan(&handle, C8724Q_SCAN_8_ROWS);
    }
    if (res == 0U)
    {
        res = c8724q_set_test_mode(&handle, C8724Q_BOOL_FALSE);
    }
    if (res == 0U)
    {
        res = c8724q_set_update_mode(&handle, C8724Q_UPDATE_MODE_FORCE);
    }
    if (res == 0U)
    {
        res = c8724q_set_otp1_protection(&handle, C8724Q_PROTECTION_DISABLED);
    }
    if (res == 0U)
    {
        res = c8724q_set_rcs_bit(&handle, 1U);
    }
    if (res == 0U)
    {
        res = c8724q_set_internal_clock(&handle, C8724Q_INTERNAL_CLOCK_8_MHZ);
    }
    if (res == 0U)
    {
        res = c8724q_set_output_enable(&handle, C8724Q_BOOL_TRUE);
    }
    if (res == 0U)
    {
        res = c8724q_set_ghost_removal(&handle, C8724Q_GHOST_REMOVAL_WEAK);
    }
    if (res == 0U)
    {
        res = c8724q_set_line_blanking(&handle, C8724Q_LINE_BLANKING_4T);
    }
    if (res == 0U)
    {
        res = c8724q_set_sleep_enable(&handle, C8724Q_BOOL_FALSE);
    }
    if (res == 0U)
    {
        res = c8724q_set_auto_sleep_enable(&handle, C8724Q_BOOL_FALSE);
    }
    if (res == 0U)
    {
        res = c8724q_set_global_reset_enable(&handle, C8724Q_BOOL_FALSE);
    }
    if (res == 0U)
    {
        res = c8724q_set_soft_reset_enable(&handle, C8724Q_BOOL_FALSE);
    }
    if (res == 0U)
    {
        res = c8724q_set_scan_clock_double_enable(&handle, C8724Q_BOOL_FALSE);
    }
    if (res == 0U)
    {
        res = c8724q_set_otp2_protection(&handle, C8724Q_PROTECTION_DISABLED);
    }
    if (res == 0U)
    {
        res = c8724q_set_power_down_reset_enable(&handle, C8724Q_BOOL_FALSE);
    }
    if ((res != 0U) || (gs_write_count != write_count))
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }

    /* verify typed getters report the pending field values */
    if ((c8724q_get_seg11_mode(&handle, &seg_mode) != 0U) ||
        (seg_mode != C8724Q_SEG_PIN_LED_OUTPUT) ||
        (c8724q_get_clock_source(&handle, &clock_source) != 0U) ||
        (clock_source != C8724Q_CLOCK_SOURCE_INTERNAL) ||
        (c8724q_get_scan(&handle, &scan) != 0U) || (scan != C8724Q_SCAN_8_ROWS) ||
        (c8724q_get_otp1_protection(&handle, &protection) != 0U) ||
        (protection != C8724Q_PROTECTION_DISABLED) ||
        (c8724q_get_internal_clock(&handle, &internal_clock) != 0U) ||
        (internal_clock != C8724Q_INTERNAL_CLOCK_8_MHZ) ||
        (c8724q_get_line_blanking(&handle, &line_blanking) != 0U) ||
        (line_blanking != C8724Q_LINE_BLANKING_4T) ||
        (c8724q_get_update_mode(&handle, &update_mode) != 0U) ||
        (update_mode != C8724Q_UPDATE_MODE_FORCE) ||
        (c8724q_get_output_enable(&handle, &bool_value) != 0U) ||
        (bool_value != C8724Q_BOOL_TRUE))
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }

    /* compare staged and applied snapshots before commit */
    if ((c8724q_get_config(&handle, pending) != 0U) ||
        (c8724q_get_applied_config(&handle, applied) != 0U) ||
        (pending[0] != 0xC0U) || (pending[1] != 0x1CU) ||
        (pending[2] != 0xF8U) || (pending[3] != 0x02U) ||
        (applied[0] != 0x3FU) || (applied[1] != 0x00U) ||
        (applied[2] != 0xF0U) || (applied[3] != 0x02U))
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }

    /* apply all four bytes in one checksummed SPI transaction */
    res = c8724q_apply_config(&handle);
    if (res != 0U)
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }
    {
        const uint8_t expected[9] = { 0x5AU, 0xFFU, 0x01U, 0x5AU,
                                       0xC0U, 0x1CU, 0xF8U, 0x02U, 0xD6U };
        if ((gs_packet_len != sizeof(expected)) ||
            (memcmp(gs_packet, expected, sizeof(expected)) != 0) ||
            (handle.config_dirty != 0U))
        {
            (void)c8724q_deinit(&handle);
            return 1U;
        }
    }

    /* reject invalid enum, reserved register bits, and out-of-range raw fields */
    write_count = gs_write_count;
    if ((c8724q_set_scan(&handle, (c8724q_scan_t)8U) != 4U) ||
        (c8724q_set_rcs_bit(&handle, 2U) != 4U) ||
        (c8724q_set_config_reg2(&handle, 0x20U) != 4U) ||
        (c8724q_set_config_reg4(&handle, 0x80U) != 4U) ||
        (gs_write_count != write_count))
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }

    /* retain staged data and applied snapshot after a failed apply, then retry */
    if (c8724q_set_global_current_gain(&handle, 5U) != 0U)
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }
    gs_write_fail = 1U;
    res = c8724q_apply_config(&handle);
    gs_write_fail = 0U;
    if ((res != 1U) || (c8724q_get_config(&handle, pending) != 0U) ||
        (c8724q_get_applied_config(&handle, applied) != 0U) ||
        (pending[0] != 0xC5U) || (applied[0] != 0xC0U) || (handle.config_dirty == 0U))
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }
    if (c8724q_apply_config(&handle) != 0U)
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }
    {
        const uint8_t expected[9] = { 0x5AU, 0xFFU, 0x01U, 0x5AU,
                                       0xC5U, 0x1CU, 0xF8U, 0x02U, 0xDBU };
        if ((gs_packet_len != sizeof(expected)) ||
            (memcmp(gs_packet, expected, sizeof(expected)) != 0) ||
            (handle.config_dirty != 0U))
        {
            (void)c8724q_deinit(&handle);
            return 1U;
        }
    }

    /* finish the mock session */
    res = c8724q_deinit(&handle);
    if (res != 0U)
    {
        return 1U;
    }

    return 0U;
}

/** @} */
