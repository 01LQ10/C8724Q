/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724q.h
 * @brief     driver c8724q header file
 * @version   1.1.0
 * @author    LQ
 * @date      2026-10-05
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author  <th>Description
 * <tr><td>2026/10/05  <td>1.0.0    <td>LQ      <td>first upload
 * <tr><td>2026/10/05  <td>1.1.0    <td>LQ      <td>config API reduced to set/get plus field macros, frame buffer added
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
#define C8724Q_DRIVER_VERSION                        1100U /**< driver version 1.1.0 */

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
    uint8_t config[4]; /**< configuration registers mirroring the chip */
    uint8_t frame[C8724Q_DISPLAY_DATA_MAX]; /**< pending display frame in GRID-major order */
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
 * @note       sends a broadcast global reset after SPI initialization and loads
 *             the datasheet reset defaults into the shadow configuration
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
 * @note       clears the active frame, enables sleep in the configuration,
 *             sends the sleep instruction, then deinitializes SPI
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
 * @brief      send the four configuration registers to the chip
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  *config pointer to four configuration bytes, edit a copy from
 *             c8724q_get_config with the C8724Q_SET_xxx macros
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 config is null or contains reserved bits
 * @note       transmits all four registers and their checksum in one packet and
 *             updates the shadow configuration on success; on SPI failure the
 *             shadow configuration is left untouched
 */
uint8_t c8724q_set_config(c8724q_handle_t *handle, const uint8_t config[4]);

/**
 * @brief      copy the current configuration registers
 * @param[in]  *handle pointer to a c8724q handle
 * @param[out] *config pointer to a four-byte output buffer
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 config is null
 * @note       returns the shadow configuration; the chip has no register
 *             readback, and because every c8724q_set_config transmits at once,
 *             the shadow equals the last successfully applied value
 */
uint8_t c8724q_get_config(c8724q_handle_t *handle, uint8_t config[4]);

/**
 * @brief      configuration field read-modify-write macro
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  index register index, 0 through 3
 * @param[in]  mask field bit mask
 * @param[in]  shift field bit shift
 * @param[in]  value encoded field value, masked to the field width
 * @note       builds C8724Q_SET_xxx macros; cfg is evaluated more than once
 */
#define C8724Q_CFG_FIELD_SET(cfg, index, mask, shift, value)          \
    do {                                                              \
        (cfg)[(index)] = (uint8_t)(((cfg)[(index)] &                  \
                                    (uint8_t)(~(uint8_t)(mask))) |    \
                                   (((uint8_t)(value) << (shift)) &   \
                                    (uint8_t)(mask)));                \
    } while (0)

/**
 * @brief      configuration field extraction macro
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  index register index, 0 through 3
 * @param[in]  mask field bit mask
 * @param[in]  shift field bit shift
 * @param[in]  type output type of the extraction
 * @note       builds C8724Q_GET_xxx macros; cfg is evaluated more than once
 */
#define C8724Q_CFG_FIELD_GET(cfg, index, mask, shift, type)           \
    ((type)((((cfg)[(index)]) & (uint8_t)(mask)) >> (shift)))

/**
 * @brief      set and get the SEG12 pin function in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_seg_pin_mode_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_SEG12_MODE(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 0, C8724Q_REG1_SEG12_CS_MASK, 7, value)
#define C8724Q_GET_SEG12_MODE(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 0, C8724Q_REG1_SEG12_CS_MASK, 7, c8724q_seg_pin_mode_t)

/**
 * @brief      set and get the SEG11 pin function in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_seg_pin_mode_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_SEG11_MODE(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 0, C8724Q_REG1_SEG11_CS_MASK, 6, value)
#define C8724Q_GET_SEG11_MODE(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 0, C8724Q_REG1_SEG11_CS_MASK, 6, c8724q_seg_pin_mode_t)

/**
 * @brief      set and get the global current gain in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value GCC value from 0 through 63
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_GLOBAL_CURRENT_GAIN(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 0, C8724Q_REG1_GCC_MASK, 0, value)
#define C8724Q_GET_GLOBAL_CURRENT_GAIN(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 0, C8724Q_REG1_GCC_MASK, 0, uint8_t)

/**
 * @brief      set and get the clock source in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_clock_source_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_CLOCK_SOURCE(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 1, C8724Q_REG2_CKS_MASK, 7, value)
#define C8724Q_GET_CLOCK_SOURCE(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 1, C8724Q_REG2_CKS_MASK, 7, c8724q_clock_source_t)

/**
 * @brief      set and get the active GRID scan count in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_scan_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_SCAN(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 1, C8724Q_REG2_SCAN_MASK, 2, value)
#define C8724Q_GET_SCAN(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 1, C8724Q_REG2_SCAN_MASK, 2, c8724q_scan_t)

/**
 * @brief      set and get the test mode in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_bool_t value
 * @note       normal display operation requires test mode disabled
 */
#define C8724Q_SET_TEST_MODE(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 1, C8724Q_REG2_WM_MASK, 1, value)
#define C8724Q_GET_TEST_MODE(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 1, C8724Q_REG2_WM_MASK, 1, c8724q_bool_t)

/**
 * @brief      set and get the display update mode in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_update_mode_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_UPDATE_MODE(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 1, C8724Q_REG2_VSYN_M_MASK, 0, value)
#define C8724Q_GET_UPDATE_MODE(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 1, C8724Q_REG2_VSYN_M_MASK, 0, c8724q_update_mode_t)

/**
 * @brief      set and get the OTP1 125 degree protection in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value C8724Q_BOOL_TRUE enables the protection, any other value disables it
 * @note       the register bit is inverted: 0 enables and 1 disables the protection
 */
#define C8724Q_SET_OTP1_ENABLE(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 2, C8724Q_REG3_OTP1_MASK, 7, \
                         (((value) == C8724Q_BOOL_TRUE) ? 0U : 1U))
#define C8724Q_GET_OTP1_ENABLE(cfg) \
    ((c8724q_bool_t)(((((cfg)[2]) & C8724Q_REG3_OTP1_MASK) == 0U) ? \
                     C8724Q_BOOL_TRUE : C8724Q_BOOL_FALSE))

/**
 * @brief      set and get the current setting resistor selection in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value C8724Q_BOOL_TRUE selects the external resistor, C8724Q_BOOL_FALSE the internal one
 * @note       the datasheet only documents the internal resistor setting (RCS = 0)
 */
#define C8724Q_SET_RCS_EXTERNAL(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 2, C8724Q_REG3_RCS_MASK, 6, value)
#define C8724Q_GET_RCS_EXTERNAL(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 2, C8724Q_REG3_RCS_MASK, 6, c8724q_bool_t)

/**
 * @brief      set and get the internal clock frequency in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_internal_clock_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_INTERNAL_CLOCK(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 2, C8724Q_REG3_RCKS_MASK, 4, value)
#define C8724Q_GET_INTERNAL_CLOCK(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 2, C8724Q_REG3_RCKS_MASK, 4, c8724q_internal_clock_t)

/**
 * @brief      set and get the SEG output enable in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_bool_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_OUTPUT_ENABLE(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 2, C8724Q_REG3_OE_MASK, 3, value)
#define C8724Q_GET_OUTPUT_ENABLE(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 2, C8724Q_REG3_OE_MASK, 3, c8724q_bool_t)

/**
 * @brief      set and get the ghost removal strength in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_ghost_removal_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_GHOST_REMOVAL(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 2, C8724Q_REG3_DGH_MASK, 2, value)
#define C8724Q_GET_GHOST_REMOVAL(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 2, C8724Q_REG3_DGH_MASK, 2, c8724q_ghost_removal_t)

/**
 * @brief      set and get the line blanking time in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_line_blanking_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_LINE_BLANKING(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 2, C8724Q_REG3_TLS_MASK, 0, value)
#define C8724Q_GET_LINE_BLANKING(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 2, C8724Q_REG3_TLS_MASK, 0, c8724q_line_blanking_t)

/**
 * @brief      set and get the sleep mode enable in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_bool_t value
 * @note       the sleep instruction requires this field enabled
 */
#define C8724Q_SET_SLEEP_ENABLE(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 3, C8724Q_REG4_SLEEP_EN_MASK, 6, value)
#define C8724Q_GET_SLEEP_ENABLE(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 3, C8724Q_REG4_SLEEP_EN_MASK, 6, c8724q_bool_t)

/**
 * @brief      set and get the automatic sleep enable in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_bool_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_AUTO_SLEEP_ENABLE(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 3, C8724Q_REG4_SLEEP_AT_MASK, 5, value)
#define C8724Q_GET_AUTO_SLEEP_ENABLE(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 3, C8724Q_REG4_SLEEP_AT_MASK, 5, c8724q_bool_t)

/**
 * @brief      set and get the global reset enable in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_bool_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_GLOBAL_RESET_ENABLE(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 3, C8724Q_REG4_GRST_MASK, 4, value)
#define C8724Q_GET_GLOBAL_RESET_ENABLE(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 3, C8724Q_REG4_GRST_MASK, 4, c8724q_bool_t)

/**
 * @brief      set and get the soft reset enable in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_bool_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_SOFT_RESET_ENABLE(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 3, C8724Q_REG4_SRST_MASK, 3, value)
#define C8724Q_GET_SOFT_RESET_ENABLE(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 3, C8724Q_REG4_SRST_MASK, 3, c8724q_bool_t)

/**
 * @brief      set and get the scan clock doubling in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_bool_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_SCAN_CLOCK_DOUBLE(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 3, C8724Q_REG4_CLK2X_MASK, 2, value)
#define C8724Q_GET_SCAN_CLOCK_DOUBLE(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 3, C8724Q_REG4_CLK2X_MASK, 2, c8724q_bool_t)

/**
 * @brief      set and get the OTP2 150 degree protection in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value C8724Q_BOOL_TRUE enables the protection, any other value disables it
 * @note       the register bit is inverted: 0 enables and 1 disables the protection
 */
#define C8724Q_SET_OTP2_ENABLE(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 3, C8724Q_REG4_OTP2_MASK, 1, \
                         (((value) == C8724Q_BOOL_TRUE) ? 0U : 1U))
#define C8724Q_GET_OTP2_ENABLE(cfg) \
    ((c8724q_bool_t)(((((cfg)[3]) & C8724Q_REG4_OTP2_MASK) == 0U) ? \
                     C8724Q_BOOL_TRUE : C8724Q_BOOL_FALSE))

/**
 * @brief      set and get the power-down reset enable in a configuration array
 * @param[in]  cfg uint8_t[4] configuration array
 * @param[in]  value c8724q_bool_t value
 * @note       no range check; call c8724q_set_config to transmit
 */
#define C8724Q_SET_POWER_DOWN_RESET(cfg, value) \
    C8724Q_CFG_FIELD_SET(cfg, 3, C8724Q_REG4_PDR_MASK, 0, value)
#define C8724Q_GET_POWER_DOWN_RESET(cfg) \
    C8724Q_CFG_FIELD_GET(cfg, 3, C8724Q_REG4_PDR_MASK, 0, c8724q_bool_t)

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
 * @brief      set one PWM value in the frame buffer
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  grid GRID index, 0 through 7
 * @param[in]  segment SEG index, 0 through 11
 * @param[in]  pwm PWM value from 0 through 255
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 grid or segment is out of range
 * @note       modifies the frame buffer only; call c8724q_display_flush to show it
 */
uint8_t c8724q_display_set_pixel(c8724q_handle_t *handle, uint8_t grid, uint8_t segment, uint8_t pwm);

/**
 * @brief      get one PWM value from the frame buffer
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  grid GRID index, 0 through 7
 * @param[in]  segment SEG index, 0 through 11
 * @param[out] *pwm pointer to the PWM value output
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 grid, segment or pointer is invalid
 * @note       none
 */
uint8_t c8724q_display_get_pixel(c8724q_handle_t *handle, uint8_t grid, uint8_t segment, uint8_t *pwm);

/**
 * @brief      fill the whole frame buffer with one PWM value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  pwm PWM value from 0 through 255
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       modifies the frame buffer only; call c8724q_display_flush to show it
 */
uint8_t c8724q_display_fill(c8724q_handle_t *handle, uint8_t pwm);

/**
 * @brief      send the frame buffer to the chip and switch to it
 * @param[in]  *handle pointer to a c8724q handle
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       sends the rows enabled by SCAN in one sequential write and then
 *             issues the display update instruction
 */
uint8_t c8724q_display_flush(c8724q_handle_t *handle);

/**
 * @brief      clear the frame buffer and the active display frame
 * @param[in]  *handle pointer to a c8724q handle
 * @return     status code
 *             - 0 success
 *             - 1 SPI operation failed
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       fills the frame buffer with zero PWM data and flushes it
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
 * @note       resets the configuration registers, disables outputs and reloads
 *             the datasheet reset defaults into the shadow configuration
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
