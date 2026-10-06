/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724q_interface_template.c
 * @brief     driver c8724q interface template source file
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

#include "driver_c8724q_interface.h"

/**
 * @brief      interface SPI bus initialization
 * @return     status code
 *             - 0 success
 *             - 1 initialization failed
 * @note       replace this stub with platform SPI initialization
 */
uint8_t c8724q_interface_spi_init(void)
{
    return 0U;
}

/**
 * @brief      interface SPI bus deinitialization
 * @return     status code
 *             - 0 success
 *             - 1 deinitialization failed
 * @note       replace this stub with platform SPI deinitialization
 */
uint8_t c8724q_interface_spi_deinit(void)
{
    return 0U;
}

/**
 * @brief      interface SPI byte stream write
 * @param[in]  *buf pointer to the byte buffer
 * @param[in]  len number of bytes to write
 * @return     status code
 *             - 0 success
 *             - 1 write failed
 * @note       replace this stub with a synchronous write; no hardware CS; max clock 20 MHz
 */
uint8_t c8724q_interface_spi_write_cmd(const uint8_t *buf, uint16_t len)
{
    (void)buf;
    (void)len;

    return 0U;
}

/**
 * @brief      interface debug print
 * @param[in]  fmt format string
 * @param[in]  ... variable arguments
 * @return     none
 * @note       replace this stub with the platform debug output
 */
void c8724q_interface_debug_print(const char *const fmt, ...)
{
    (void)fmt;
}
