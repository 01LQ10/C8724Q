/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724q_config_test.c
 * @brief     driver c8724q configuration test source file
 * @version   1.1.0
 * @author    LQ
 * @date      2026-10-05
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author  <th>Description
 * <tr><td>2026/10/05  <td>1.0.0    <td>LQ      <td>first upload
 * <tr><td>2026/10/05  <td>1.1.0    <td>LQ      <td>config flow moved to field macros and one set_config
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
 * @brief      check configuration macros, shadow update and packet encoding
 * @return     status code
 *             - 0 success
 *             - 1 test failed
 * @note       runs without physical hardware
 */
uint8_t c8724q_config_test(void)
{
    c8724q_handle_t handle;
    uint8_t config[4];
    uint8_t shadow[4];
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

    /* initialize chip and verify the broadcast global reset packet */
    res = c8724q_init(&handle);
    if ((res != 0U) || (gs_packet_len != 4U) || (gs_packet[0] != 0x5AU) ||
        (gs_packet[1] != 0xFFU) || (gs_packet[2] != 0x8BU) || (gs_packet[3] != 0xE4U))
    {
        return 1U;
    }

    /* the reset configuration must match the datasheet defaults */
    if ((c8724q_get_config(&handle, config) != 0U) ||
        (config[0] != 0x3FU) || (config[1] != 0x00U) ||
        (config[2] != 0xF0U) || (config[3] != 0x02U))
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }

    /* field macros edit a local copy without any bus activity */
    write_count = gs_write_count;
    C8724Q_SET_SEG11_MODE(config, C8724Q_SEG_PIN_LED_OUTPUT);
    C8724Q_SET_SEG12_MODE(config, C8724Q_SEG_PIN_LED_OUTPUT);
    C8724Q_SET_GLOBAL_CURRENT_GAIN(config, 0U);
    C8724Q_SET_CLOCK_SOURCE(config, C8724Q_CLOCK_SOURCE_INTERNAL);
    C8724Q_SET_SCAN(config, C8724Q_SCAN_8_ROWS);
    C8724Q_SET_TEST_MODE(config, C8724Q_BOOL_FALSE);
    C8724Q_SET_UPDATE_MODE(config, C8724Q_UPDATE_MODE_FORCE);
    C8724Q_SET_OTP1_ENABLE(config, C8724Q_BOOL_FALSE);
    C8724Q_SET_RCS_EXTERNAL(config, C8724Q_BOOL_TRUE);
    C8724Q_SET_INTERNAL_CLOCK(config, C8724Q_INTERNAL_CLOCK_8_MHZ);
    C8724Q_SET_OUTPUT_ENABLE(config, C8724Q_BOOL_TRUE);
    C8724Q_SET_GHOST_REMOVAL(config, C8724Q_GHOST_REMOVAL_WEAK);
    C8724Q_SET_LINE_BLANKING(config, C8724Q_LINE_BLANKING_4T);
    C8724Q_SET_SLEEP_ENABLE(config, C8724Q_BOOL_FALSE);
    C8724Q_SET_AUTO_SLEEP_ENABLE(config, C8724Q_BOOL_FALSE);
    C8724Q_SET_GLOBAL_RESET_ENABLE(config, C8724Q_BOOL_FALSE);
    C8724Q_SET_SOFT_RESET_ENABLE(config, C8724Q_BOOL_FALSE);
    C8724Q_SET_SCAN_CLOCK_DOUBLE(config, C8724Q_BOOL_FALSE);
    C8724Q_SET_OTP2_ENABLE(config, C8724Q_BOOL_FALSE);
    C8724Q_SET_POWER_DOWN_RESET(config, C8724Q_BOOL_FALSE);
    if ((config[0] != 0xC0U) || (config[1] != 0x1CU) ||
        (config[2] != 0xF8U) || (config[3] != 0x02U) ||
        (gs_write_count != write_count))
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }

    /* typed getters must read every field back from the same array */
    if ((C8724Q_GET_SEG11_MODE(config) != C8724Q_SEG_PIN_LED_OUTPUT) ||
        (C8724Q_GET_SEG12_MODE(config) != C8724Q_SEG_PIN_LED_OUTPUT) ||
        (C8724Q_GET_GLOBAL_CURRENT_GAIN(config) != 0U) ||
        (C8724Q_GET_CLOCK_SOURCE(config) != C8724Q_CLOCK_SOURCE_INTERNAL) ||
        (C8724Q_GET_SCAN(config) != C8724Q_SCAN_8_ROWS) ||
        (C8724Q_GET_TEST_MODE(config) != C8724Q_BOOL_FALSE) ||
        (C8724Q_GET_UPDATE_MODE(config) != C8724Q_UPDATE_MODE_FORCE) ||
        (C8724Q_GET_OTP1_ENABLE(config) != C8724Q_BOOL_FALSE) ||
        (C8724Q_GET_RCS_EXTERNAL(config) != C8724Q_BOOL_TRUE) ||
        (C8724Q_GET_INTERNAL_CLOCK(config) != C8724Q_INTERNAL_CLOCK_8_MHZ) ||
        (C8724Q_GET_OUTPUT_ENABLE(config) != C8724Q_BOOL_TRUE) ||
        (C8724Q_GET_GHOST_REMOVAL(config) != C8724Q_GHOST_REMOVAL_WEAK) ||
        (C8724Q_GET_LINE_BLANKING(config) != C8724Q_LINE_BLANKING_4T) ||
        (C8724Q_GET_SLEEP_ENABLE(config) != C8724Q_BOOL_FALSE) ||
        (C8724Q_GET_AUTO_SLEEP_ENABLE(config) != C8724Q_BOOL_FALSE) ||
        (C8724Q_GET_GLOBAL_RESET_ENABLE(config) != C8724Q_BOOL_FALSE) ||
        (C8724Q_GET_SOFT_RESET_ENABLE(config) != C8724Q_BOOL_FALSE) ||
        (C8724Q_GET_SCAN_CLOCK_DOUBLE(config) != C8724Q_BOOL_FALSE) ||
        (C8724Q_GET_OTP2_ENABLE(config) != C8724Q_BOOL_FALSE) ||
        (C8724Q_GET_POWER_DOWN_RESET(config) != C8724Q_BOOL_FALSE))
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }

    /* one set_config sends the whole packet and refreshes the shadow */
    res = c8724q_set_config(&handle, config);
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
            (c8724q_get_config(&handle, shadow) != 0U) ||
            (memcmp(shadow, &expected[4], 4U) != 0))
        {
            (void)c8724q_deinit(&handle);
            return 1U;
        }
    }

    /* reserved bits are rejected and the shadow stays untouched */
    config[1] = 0x20U;
    res = c8724q_set_config(&handle, config);
    if ((res != 4U) || (c8724q_get_config(&handle, shadow) != 0U) ||
        (shadow[1] != 0x1CU))
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }
    config[1] = 0x1CU;
    config[3] = 0x82U;
    res = c8724q_set_config(&handle, config);
    if ((res != 4U) || (c8724q_get_config(&handle, shadow) != 0U) ||
        (shadow[3] != 0x02U))
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }
    config[3] = 0x02U;

    /* a failed SPI write keeps the previous shadow, a retry succeeds */
    C8724Q_SET_GLOBAL_CURRENT_GAIN(config, 5U);
    gs_write_fail = 1U;
    res = c8724q_set_config(&handle, config);
    gs_write_fail = 0U;
    if ((res != 1U) || (c8724q_get_config(&handle, shadow) != 0U) ||
        (shadow[0] != 0xC0U))
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }
    if (c8724q_set_config(&handle, config) != 0U)
    {
        (void)c8724q_deinit(&handle);
        return 1U;
    }
    {
        const uint8_t expected[9] = { 0x5AU, 0xFFU, 0x01U, 0x5AU,
                                       0xC5U, 0x1CU, 0xF8U, 0x02U, 0xDBU };
        if ((gs_packet_len != sizeof(expected)) ||
            (memcmp(gs_packet, expected, sizeof(expected)) != 0) ||
            (c8724q_get_config(&handle, shadow) != 0U) ||
            (memcmp(shadow, &expected[4], 4U) != 0))
        {
            (void)c8724q_deinit(&handle);
            return 1U;
        }
    }

    /* finish the mock session: clear, sleep enable, sleep and bus close */
    res = c8724q_deinit(&handle);
    if (res != 0U)
    {
        return 1U;
    }

    return 0U;
}

/** @} */
