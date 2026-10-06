/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724q.h
 * @brief     driver c8724q header file
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

#ifndef DRIVER_C8724Q_H
#define DRIVER_C8724Q_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup c8724q_driver c8724q driver function
 * @brief    c8724q driver modules
 * @{
 */

#define C8724Q_DISPLAY_WIDTH                         12U /**< number of SEG channels */
#define C8724Q_DISPLAY_HEIGHT                        8U  /**< number of GRID channels */
#define C8724Q_DISPLAY_DATA_MAX                      96U /**< maximum sequential PWM data bytes */
#define C8724Q_ADDRESS_DATA_MAX                      72U /**< maximum addressed PWM data entries */
#define C8724Q_DRIVER_VERSION                        1000U /**< driver version 1.0.0 */

#define C8724Q_REG1_SEG12_CS_MASK                    0x80U /**< SEG12 function bit mask */
#define C8724Q_REG1_SEG11_CS_MASK                    0x40U /**< SEG11 function bit mask */
#define C8724Q_REG1_GCC_MASK                         0x3FU /**< global current gain bit mask */
#define C8724Q_REG2_CKS_MASK                         0x80U /**< clock source bit mask */
#define C8724Q_REG2_SCAN_MASK                        0x1CU /**< active GRID count bit mask */
#define C8724Q_REG2_WM_MASK                          0x02U /**< test mode bit mask */
#define C8724Q_REG2_VSYN_M_MASK                      0x01U /**< display update mode bit mask */
#define C8724Q_REG3_OTP1_MASK                        0x80U /**< 125 degree protection bit mask */
#define C8724Q_REG3_RCS_MASK                         0x40U /**< resistor selection bit mask */
#define C8724Q_REG3_RCKS_MASK                        0x30U /**< internal clock frequency bit mask */
#define C8724Q_REG3_OE_MASK                          0x08U /**< SEG output enable bit mask */
#define C8724Q_REG3_DGH_MASK                         0x04U /**< ghost removal bit mask */
#define C8724Q_REG3_TLS_MASK                         0x03U /**< line blanking time bit mask */
#define C8724Q_REG4_SLEEP_EN_MASK                    0x40U /**< sleep mode enable bit mask */
#define C8724Q_REG4_SLEEP_AT_MASK                    0x20U /**< automatic sleep bit mask */
#define C8724Q_REG4_GRST_MASK                        0x10U /**< global reset enable bit mask */
#define C8724Q_REG4_SRST_MASK                        0x08U /**< soft reset enable bit mask */
#define C8724Q_REG4_CLK2X_MASK                       0x04U /**< scan clock doubling bit mask */
#define C8724Q_REG4_OTP2_MASK                        0x02U /**< 150 degree protection bit mask */
#define C8724Q_REG4_PDR_MASK                         0x01U /**< power-down reset bit mask */

/**
 * @brief c8724q boolean enumeration definition
 */
typedef enum
{
    C8724Q_BOOL_FALSE = 0x00, /**< disable function */
    C8724Q_BOOL_TRUE  = 0x01  /**< enable function */
} c8724q_bool_t;

/**
 * @brief c8724q instruction address enumeration definition
 */
typedef enum
{
    C8724Q_ADDRESS_0 = 0x00, /**< chip select address 00 */
    C8724Q_ADDRESS_1 = 0x01, /**< chip select address 01 */
    C8724Q_ADDRESS_2 = 0x02, /**< chip select address 10 */
    C8724Q_ADDRESS_3 = 0x03  /**< chip select address 11 */
} c8724q_address_t;

/**
 * @brief c8724q scan row enumeration definition
 */
typedef enum
{
    C8724Q_SCAN_1_ROW = 0x00, /**< enable GRID1 */
    C8724Q_SCAN_2_ROWS = 0x01, /**< enable GRID1 through GRID2 */
    C8724Q_SCAN_3_ROWS = 0x02, /**< enable GRID1 through GRID3 */
    C8724Q_SCAN_4_ROWS = 0x03, /**< enable GRID1 through GRID4 */
    C8724Q_SCAN_5_ROWS = 0x04, /**< enable GRID1 through GRID5 */
    C8724Q_SCAN_6_ROWS = 0x05, /**< enable GRID1 through GRID6 */
    C8724Q_SCAN_7_ROWS = 0x06, /**< enable GRID1 through GRID7 */
    C8724Q_SCAN_8_ROWS = 0x07  /**< enable GRID1 through GRID8 */
} c8724q_scan_t;

/**
 * @brief c8724q SEG pin function enumeration definition
 */
typedef enum
{
    C8724Q_SEG_PIN_CHIP_SELECT = 0x00, /**< SEG pin used as chip select input */
    C8724Q_SEG_PIN_LED_OUTPUT = 0x01 /**< SEG pin used as LED output */
} c8724q_seg_pin_mode_t;

/**
 * @brief c8724q clock source enumeration definition
 */
typedef enum
{
    C8724Q_CLOCK_SOURCE_INTERNAL = 0x00, /**< use internal RCLK */
    C8724Q_CLOCK_SOURCE_EXTERNAL = 0x01 /**< use external CLK */
} c8724q_clock_source_t;

/**
 * @brief c8724q display update mode enumeration definition
 */
typedef enum
{
    C8724Q_UPDATE_MODE_FORCE = 0x00, /**< update on the next CLK */
    C8724Q_UPDATE_MODE_SCAN_END = 0x01 /**< update after a complete scan */
} c8724q_update_mode_t;

/**
 * @brief c8724q overtemperature protection enumeration definition
 */
typedef enum
{
    C8724Q_PROTECTION_ENABLED = 0x00, /**< enable the corresponding protection */
    C8724Q_PROTECTION_DISABLED = 0x01 /**< disable the corresponding protection */
} c8724q_protection_t;

/**
 * @brief c8724q internal clock frequency enumeration definition
 */
typedef enum
{
    C8724Q_INTERNAL_CLOCK_1_MHZ = 0x00, /**< 1 MHz */
    C8724Q_INTERNAL_CLOCK_2_MHZ = 0x01, /**< 2 MHz */
    C8724Q_INTERNAL_CLOCK_4_MHZ = 0x02, /**< 4 MHz */
    C8724Q_INTERNAL_CLOCK_8_MHZ = 0x03 /**< 8 MHz */
} c8724q_internal_clock_t;

/**
 * @brief c8724q ghost removal strength enumeration definition
 */
typedef enum
{
    C8724Q_GHOST_REMOVAL_WEAK = 0x00, /**< weak ghost removal */
    C8724Q_GHOST_REMOVAL_STRONG = 0x01 /**< strong ghost removal */
} c8724q_ghost_removal_t;

/**
 * @brief c8724q line blanking time enumeration definition
 */
typedef enum
{
    C8724Q_LINE_BLANKING_4T = 0x00, /**< 4 PWM clock periods */
    C8724Q_LINE_BLANKING_8T = 0x01, /**< 8 PWM clock periods */
    C8724Q_LINE_BLANKING_12T = 0x02, /**< 12 PWM clock periods */
    C8724Q_LINE_BLANKING_16T = 0x03 /**< 16 PWM clock periods */
} c8724q_line_blanking_t;

/**
 * @brief c8724q static information structure
 */
typedef struct c8724q_info_s
{
    char chip_name[32]; /**< chip name */
    char manufacturer_name[32]; /**< manufacturer name */
    char interface[8]; /**< chip interface name */
    float supply_voltage_min_v; /**< chip minimum supply voltage */
    float supply_voltage_max_v; /**< chip maximum supply voltage */
    float max_current_ma; /**< maximum SEG output current */
    float temperature_min; /**< minimum operating temperature */
    float temperature_max; /**< maximum operating temperature */
    uint32_t driver_version; /**< driver version */
} c8724q_info_t;

/**
 * @brief c8724q handle structure
 */
typedef struct c8724q_handle_s
{
    uint8_t (*spi_init)(void); /**< point to a spi_init function address */
    uint8_t (*spi_deinit)(void); /**< point to a spi_deinit function address */
    uint8_t (*spi_write_cmd)(const uint8_t *buf, uint16_t len); /**< point to a spi_write_cmd function address */
    void (*debug_print)(const char *const fmt, ...); /**< point to a debug_print function address */
    uint8_t config[4]; /**< pending configuration registers 1 through 4 */
    uint8_t applied_config[4]; /**< last successfully applied configuration registers */
    uint8_t config_dirty; /**< pending configuration differs from applied configuration */
    uint8_t address; /**< instruction chip select address */
    uint8_t broadcast; /**< broadcast instruction selection */
    uint8_t inited; /**< initialized flag */
} c8724q_handle_t;

/**
 * @defgroup c8724q_link_driver c8724q link driver function
 * @brief    c8724q link driver modules
 * @{
 */

#define DRIVER_C8724Q_LINK_INIT(handle, structure)                 memset((handle), 0, sizeof(structure))
#define DRIVER_C8724Q_LINK_SPI_INIT(handle, f)                      ((handle)->spi_init = (f))
#define DRIVER_C8724Q_LINK_SPI_DEINIT(handle, f)                    ((handle)->spi_deinit = (f))
#define DRIVER_C8724Q_LINK_SPI_WRITE_CMD(handle, f)                 ((handle)->spi_write_cmd = (f))
#define DRIVER_C8724Q_LINK_DEBUG_PRINT(handle, f)                   ((handle)->debug_print = (f))

/** @} */

/**
 * @defgroup c8724q_base_driver c8724q base driver function
 * @brief    c8724q base driver modules
 * @{
 */

/**
 * @brief      get chip information
 * @param[out] *info pointer to a c8724q info structure
 * @return     status code
 *             - 0 success
 *             - 2 info is invalid
 * @note       none
 */
uint8_t c8724q_info(c8724q_info_t *info);

/**
 * @brief      initialize the chip
 * @param[in]  *handle pointer to a c8724q handle
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 required function is null or chip is already initialized
 * @note       sends a broadcast global reset after SPI initialization
 */
uint8_t c8724q_init(c8724q_handle_t *handle);

/**
 * @brief      deinitialize the chip
 * @param[in]  *handle pointer to a c8724q handle
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       clears the active frame, sends sleep, then deinitializes SPI
 */
uint8_t c8724q_deinit(c8724q_handle_t *handle);

/**
 * @brief      set the instruction chip select address
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  address chip select address, 0 through 3
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 address is invalid
 * @note       this is the instruction address, not an MCU GPIO
 */
uint8_t c8724q_set_address(c8724q_handle_t *handle, c8724q_address_t address);

/**
 * @brief      get the instruction chip select address
 * @param[in]  *handle pointer to a c8724q handle
 * @param[out] *address pointer to the chip select address
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 address is invalid
 * @note       returns the software-selected instruction address
 */
uint8_t c8724q_get_address(c8724q_handle_t *handle, c8724q_address_t *address);

/**
 * @brief      set broadcast instruction selection
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  enable broadcast enable value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 enable is invalid
 * @note       broadcast instructions are accepted regardless of chip address
 */
uint8_t c8724q_set_broadcast(c8724q_handle_t *handle, c8724q_bool_t enable);

/**
 * @brief      get broadcast instruction selection
 * @param[in]  *handle pointer to a c8724q handle
 * @param[out] *enable pointer to the broadcast enable value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 enable is invalid
 * @note       returns the software-selected broadcast mode
 */
uint8_t c8724q_get_broadcast(c8724q_handle_t *handle, c8724q_bool_t *enable);

/** @} */

/**
 * @defgroup c8724q_config_driver c8724q configuration driver function
 * @brief    c8724q configuration driver modules
 * @{
 */

/**
 * @brief      stage all four configuration registers
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  *config pointer to four configuration bytes
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 config is invalid or contains reserved bits
 * @note       stages the bytes only; call c8724q_apply_config to transmit them
 */
uint8_t c8724q_set_config(c8724q_handle_t *handle, const uint8_t config[4]);

/**
 * @brief      get the four pending configuration register values
 * @param[in]  *handle pointer to a c8724q handle
 * @param[out] *config pointer to a four-byte output buffer
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 config is null
 * @note       returns staged values, not hardware readback
 */
uint8_t c8724q_get_config(c8724q_handle_t *handle, uint8_t config[4]);

/**
 * @brief      apply pending configuration registers
 * @param[in]  *handle pointer to a c8724q handle
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 pending configuration is invalid
 * @note       sends all four configuration bytes and their checksum
 */
uint8_t c8724q_apply_config(c8724q_handle_t *handle);

/**
 * @brief      get the last successfully applied configuration
 * @param[in]  *handle pointer to a c8724q handle
 * @param[out] *config pointer to a four-byte output buffer
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 config is null
 * @note       returns the software snapshot of the last successful apply
 */
uint8_t c8724q_get_applied_config(c8724q_handle_t *handle, uint8_t config[4]);

/**
 * @brief      set configuration register 1 raw value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value configuration register 1 value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       low-level staging API; use feature setters where possible
 */
uint8_t c8724q_set_config_reg1(c8724q_handle_t *handle, uint8_t value);

/**
 * @brief      get configuration register 1 shadow value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[out] *value pointer to the output value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 value is null
 * @note       returns the pending software value
 */
uint8_t c8724q_get_config_reg1(c8724q_handle_t *handle, uint8_t *value);

/**
 * @brief      set configuration register 2 raw value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value configuration register 2 value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 reserved bits are set
 * @note       low-level staging API; bits 6 and 5 must be zero
 */
uint8_t c8724q_set_config_reg2(c8724q_handle_t *handle, uint8_t value);

/**
 * @brief      get configuration register 2 shadow value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[out] *value pointer to the output value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 value is null
 * @note       returns the pending software value
 */
uint8_t c8724q_get_config_reg2(c8724q_handle_t *handle, uint8_t *value);

/**
 * @brief      set configuration register 3 raw value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value configuration register 3 value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       low-level staging API; RCS=0 is documented, RCS=1 behavior is unspecified
 */
uint8_t c8724q_set_config_reg3(c8724q_handle_t *handle, uint8_t value);

/**
 * @brief      get configuration register 3 shadow value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[out] *value pointer to the output value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 value is null
 * @note       returns the pending software value
 */
uint8_t c8724q_get_config_reg3(c8724q_handle_t *handle, uint8_t *value);

/**
 * @brief      set configuration register 4 raw value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value configuration register 4 value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 reserved bit is set
 * @note       low-level staging API; bit 7 must be zero
 */
uint8_t c8724q_set_config_reg4(c8724q_handle_t *handle, uint8_t value);

/**
 * @brief      get configuration register 4 shadow value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[out] *value pointer to the output value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 value is null
 * @note       returns the pending software value
 */
uint8_t c8724q_get_config_reg4(c8724q_handle_t *handle, uint8_t *value);

/**
 * @brief      stage and get SEG11 pin function
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value SEG11 pin function
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_seg11_mode(c8724q_handle_t *handle, c8724q_seg_pin_mode_t value);
uint8_t c8724q_get_seg11_mode(c8724q_handle_t *handle, c8724q_seg_pin_mode_t *value);

/**
 * @brief      stage and get SEG12 pin function
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value SEG12 pin function
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_seg12_mode(c8724q_handle_t *handle, c8724q_seg_pin_mode_t value);
uint8_t c8724q_get_seg12_mode(c8724q_handle_t *handle, c8724q_seg_pin_mode_t *value);

/**
 * @brief      stage and get global current gain
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value GCC value from 0 through 63
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 out of range
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_global_current_gain(c8724q_handle_t *handle, uint8_t value);
uint8_t c8724q_get_global_current_gain(c8724q_handle_t *handle, uint8_t *value);

/**
 * @brief      stage and get clock source
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value clock source selection
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_clock_source(c8724q_handle_t *handle, c8724q_clock_source_t value);
uint8_t c8724q_get_clock_source(c8724q_handle_t *handle, c8724q_clock_source_t *value);

/**
 * @brief      stage and get active GRID scan count
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value scan row selection
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_scan(c8724q_handle_t *handle, c8724q_scan_t value);
uint8_t c8724q_get_scan(c8724q_handle_t *handle, c8724q_scan_t *value);

/**
 * @brief      stage and get test mode
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value test mode enable
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       normal display operation requires test mode disabled
 */
uint8_t c8724q_set_test_mode(c8724q_handle_t *handle, c8724q_bool_t value);
uint8_t c8724q_get_test_mode(c8724q_handle_t *handle, c8724q_bool_t *value);

/**
 * @brief      stage and get display update mode
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value display update mode
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_update_mode(c8724q_handle_t *handle, c8724q_update_mode_t value);
uint8_t c8724q_get_update_mode(c8724q_handle_t *handle, c8724q_update_mode_t *value);

/**
 * @brief      stage and get OTP1 protection state
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value OTP1 protection state
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       register bit value 0 enables protection and 1 disables it
 */
uint8_t c8724q_set_otp1_protection(c8724q_handle_t *handle, c8724q_protection_t value);
uint8_t c8724q_get_otp1_protection(c8724q_handle_t *handle, c8724q_protection_t *value);

/**
 * @brief      stage and get the raw RCS bit
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value raw RCS bit, 0 or 1
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 out of range
 * @note       datasheet documents RCS=0; behavior for RCS=1 is unspecified
 */
uint8_t c8724q_set_rcs_bit(c8724q_handle_t *handle, uint8_t value);
uint8_t c8724q_get_rcs_bit(c8724q_handle_t *handle, uint8_t *value);

/**
 * @brief      stage and get internal clock frequency
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value internal clock frequency
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_internal_clock(c8724q_handle_t *handle, c8724q_internal_clock_t value);
uint8_t c8724q_get_internal_clock(c8724q_handle_t *handle, c8724q_internal_clock_t *value);

/**
 * @brief      stage and get SEG output enable
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value SEG output enable
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_output_enable(c8724q_handle_t *handle, c8724q_bool_t value);
uint8_t c8724q_get_output_enable(c8724q_handle_t *handle, c8724q_bool_t *value);

/**
 * @brief      stage and get ghost removal strength
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value ghost removal strength
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_ghost_removal(c8724q_handle_t *handle, c8724q_ghost_removal_t value);
uint8_t c8724q_get_ghost_removal(c8724q_handle_t *handle, c8724q_ghost_removal_t *value);

/**
 * @brief      stage and get line blanking time
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value line blanking time
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_line_blanking(c8724q_handle_t *handle, c8724q_line_blanking_t value);
uint8_t c8724q_get_line_blanking(c8724q_handle_t *handle, c8724q_line_blanking_t *value);

/**
 * @brief      stage and get sleep mode enable
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value sleep mode enable
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_sleep_enable(c8724q_handle_t *handle, c8724q_bool_t value);
uint8_t c8724q_get_sleep_enable(c8724q_handle_t *handle, c8724q_bool_t *value);

/**
 * @brief      stage and get automatic sleep enable
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value automatic sleep enable
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_auto_sleep_enable(c8724q_handle_t *handle, c8724q_bool_t value);
uint8_t c8724q_get_auto_sleep_enable(c8724q_handle_t *handle, c8724q_bool_t *value);

/**
 * @brief      stage and get global reset enable
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value global reset enable
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_global_reset_enable(c8724q_handle_t *handle, c8724q_bool_t value);
uint8_t c8724q_get_global_reset_enable(c8724q_handle_t *handle, c8724q_bool_t *value);

/**
 * @brief      stage and get soft reset enable
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value soft reset enable
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_soft_reset_enable(c8724q_handle_t *handle, c8724q_bool_t value);
uint8_t c8724q_get_soft_reset_enable(c8724q_handle_t *handle, c8724q_bool_t *value);

/**
 * @brief      stage and get scan clock doubling
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value scan clock doubling enable
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_scan_clock_double_enable(c8724q_handle_t *handle, c8724q_bool_t value);
uint8_t c8724q_get_scan_clock_double_enable(c8724q_handle_t *handle, c8724q_bool_t *value);

/**
 * @brief      stage and get OTP2 protection state
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value OTP2 protection state
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       register bit value 0 enables protection and 1 disables it
 */
uint8_t c8724q_set_otp2_protection(c8724q_handle_t *handle, c8724q_protection_t value);
uint8_t c8724q_get_otp2_protection(c8724q_handle_t *handle, c8724q_protection_t *value);

/**
 * @brief      stage and get power-down reset enable
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value power-down reset enable
 * @return     setter: 0 success, 2 invalid handle, 3 not initialized, 4 invalid value
 * @note       getter returns pending state; call c8724q_apply_config to commit
 */
uint8_t c8724q_set_power_down_reset_enable(c8724q_handle_t *handle, c8724q_bool_t value);
uint8_t c8724q_get_power_down_reset_enable(c8724q_handle_t *handle, c8724q_bool_t *value);

/** @} */

/**
 * @defgroup c8724q_display_driver c8724q display driver function
 * @brief    c8724q display driver modules
 * @{
 */

/**
 * @brief      write sequential display PWM data
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  *data pointer to the PWM data buffer
 * @param[in]  len number of PWM bytes
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 data is null or length does not match SCAN
 * @note       data order is GRID1 SEG1-12, GRID2 SEG1-12, and so on
 */
uint8_t c8724q_write_display(c8724q_handle_t *handle, const uint8_t *data, uint16_t len);

/**
 * @brief      write addressed display PWM data
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  *address pointer to SRAM addresses
 * @param[in]  *data pointer to the PWM data buffer
 * @param[in]  len number of address and data entries
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 pointers, length or SRAM address are invalid
 * @note       maximum 72 entries; address is (GRID index << 4) | SEG index
 */
uint8_t c8724q_write_display_address(c8724q_handle_t *handle, const uint8_t *address,
                                     const uint8_t *data, uint16_t len);

/**
 * @brief      update the display from the pending SRAM frame
 * @param[in]  *handle pointer to a c8724q handle
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       none
 */
uint8_t c8724q_display_update(c8724q_handle_t *handle);

/**
 * @brief      clear the active display frame
 * @param[in]  *handle pointer to a c8724q handle
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       writes zero PWM data and issues a display update
 */
uint8_t c8724q_display_clear(c8724q_handle_t *handle);

/**
 * @brief      send a soft reset instruction
 * @param[in]  *handle pointer to a c8724q handle
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       does not reset the configuration registers
 */
uint8_t c8724q_soft_reset(c8724q_handle_t *handle);

/**
 * @brief      send a global reset instruction
 * @param[in]  *handle pointer to a c8724q handle
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       resets the configuration registers and disables outputs
 */
uint8_t c8724q_global_reset(c8724q_handle_t *handle);

/**
 * @brief      send a sleep instruction
 * @param[in]  *handle pointer to a c8724q handle
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 sleep mode is disabled in configuration register 4
 * @note       caller must write an all-zero frame before sleep
 */
uint8_t c8724q_sleep(c8724q_handle_t *handle);

/**
 * @brief      send a wake instruction
 * @param[in]  *handle pointer to a c8724q handle
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       none
 */
uint8_t c8724q_wake(c8724q_handle_t *handle);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif
