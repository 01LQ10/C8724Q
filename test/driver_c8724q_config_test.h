/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724q_config_test.h
 * @brief     driver c8724q configuration test header file
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

#ifndef DRIVER_C8724Q_CONFIG_TEST_H
#define DRIVER_C8724Q_CONFIG_TEST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup c8724q_test_driver
 * @{
 */

/**
 * @brief      test configuration macros, shadow update and SPI packet generation
 * @return     status code
 *             - 0 success
 *             - 1 test failed
 * @note       uses an internal mock SPI transport and does not require hardware
 */
uint8_t c8724q_config_test(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
