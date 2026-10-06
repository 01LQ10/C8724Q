/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724q_basic.h
 * @brief     driver c8724q basic example header file
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

#ifndef DRIVER_C8724Q_BASIC_H
#define DRIVER_C8724Q_BASIC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup c8724q_example_driver c8724q example driver function
 * @brief    c8724q example driver modules
 * @{
 */

#define C8724Q_BASIC_DEFAULT_SEG11_MODE             C8724Q_SEG_PIN_LED_OUTPUT /**< use SEG11 as LED output */
#define C8724Q_BASIC_DEFAULT_SEG12_MODE             C8724Q_SEG_PIN_LED_OUTPUT /**< use SEG12 as LED output */
#define C8724Q_BASIC_DEFAULT_GCC                    0U /**< use minimum SEG output current */
#define C8724Q_BASIC_DEFAULT_SCAN                   C8724Q_SCAN_8_ROWS /**< enable all eight GRID outputs */
#define C8724Q_BASIC_DEFAULT_OUTPUT                 C8724Q_BOOL_TRUE /**< enable SEG outputs */

/**
 * @brief      initialize the basic example
 * @return     status code
 *             - 0 success
 *             - 1 initialization or configuration failed
 * @note       platform interface functions must be implemented before use
 */
uint8_t c8724q_basic_init(void);

/**
 * @brief      write and display a complete 8 by 12 PWM frame
 * @param[in]  *data pointer to 96 PWM values in GRID-major order
 * @param[in]  len number of PWM values, must be 96
 * @return     status code
 *             - 0 success
 *             - 1 write or update failed
 * @note       each PWM value ranges from 0 through 255
 */
uint8_t c8724q_basic_display(const uint8_t *data, uint16_t len);

/**
 * @brief      deinitialize the basic example
 * @return     status code
 *             - 0 success
 *             - 1 deinitialization failed
 * @note       clears the LEDs before sleep
 */
uint8_t c8724q_basic_deinit(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
