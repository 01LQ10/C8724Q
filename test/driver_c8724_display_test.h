/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724_display_test.h
 * @brief     driver c8724 display test header file
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

#ifndef DRIVER_C8724_DISPLAY_TEST_H
#define DRIVER_C8724_DISPLAY_TEST_H

#include "driver_c8724.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup c8724_test_driver
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
uint8_t c8724_display_test(c8724_address_t address, uint32_t times);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
