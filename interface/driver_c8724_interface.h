/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724_interface.h
 * @brief     driver c8724 interface header file
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

#ifndef DRIVER_C8724_INTERFACE_H
#define DRIVER_C8724_INTERFACE_H

#include "driver_c8724.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup c8724_interface_driver c8724 interface driver function
 * @brief    c8724 interface driver modules
 * @{
 */

/**
 * @brief      interface SPI bus initialization
 * @return     status code
 *             - 0 success
 *             - 1 initialization failed
 * @note       configure MSB-first SPI at no more than 20 MHz; no CS pin is used
 */
uint8_t c8724_interface_spi_init(void);

/**
 * @brief      interface SPI bus deinitialization
 * @return     status code
 *             - 0 success
 *             - 1 deinitialization failed
 * @note       none
 */
uint8_t c8724_interface_spi_deinit(void);

/**
 * @brief      interface SPI byte stream write
 * @param[in]  *buf pointer to the byte buffer
 * @param[in]  len number of bytes to write
 * @return     status code
 *             - 0 success
 *             - 1 write failed
 * @note       synchronous write; no hardware chip select; DIN samples CLK rising edges
 */
uint8_t c8724_interface_spi_write_cmd(const uint8_t *buf, uint16_t len);

/**
 * @brief     interface delay in milliseconds
 * @param[in] ms delay time
 * @return    none
 * @note      none
 */
void c8724_interface_delay_ms(uint32_t ms);

/**
 * @brief      interface debug print
 * @param[in]  fmt format string
 * @param[in]  ... variable arguments
 * @return     none
 * @note       none
 */
void c8724_interface_debug_print(const char *const fmt, ...);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
