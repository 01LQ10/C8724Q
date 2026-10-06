/**
 * Copyright (c) 2026 LQ
 * SPDX-License-Identifier: MIT
 *
 * @file      driver_c8724q.c
 * @brief     driver c8724q source file
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

#include "driver_c8724q.h"

#define C8724Q_PACKET_HEADER_0                      0x5AU /**< instruction packet header byte 0 */
#define C8724Q_PACKET_HEADER_1                      0xFFU /**< instruction packet header byte 1 */
#define C8724Q_COMMAND_CONFIG                       0x01U /**< configuration data write instruction */
#define C8724Q_COMMAND_DISPLAY_SEQUENTIAL           0x02U /**< sequential display data write instruction */
#define C8724Q_COMMAND_DISPLAY_ADDRESS              0x03U /**< addressed display data write instruction */
#define C8724Q_COMMAND_DISPLAY_UPDATE               0x04U /**< display data update instruction */
#define C8724Q_COMMAND_SOFT_RESET                   0x08U /**< soft reset instruction */
#define C8724Q_COMMAND_SLEEP                       0x0EU /**< sleep instruction */
#define C8724Q_COMMAND_WAKE                        0x0DU /**< wake instruction */
#define C8724Q_COMMAND_GLOBAL_RESET                 0x0BU /**< global reset instruction */
#define C8724Q_ADDRESS_BROADCAST_BIT                0x80U /**< broadcast selection bit */
#define C8724Q_ADDRESS_SHIFT                        3U   /**< chip select address shift */
#define C8724Q_CONFIG_REG2_RESERVED_MASK            0x60U /**< reserved bits in configuration register 2 */
#define C8724Q_CONFIG_REG4_RESERVED_MASK            0x80U /**< reserved bit in configuration register 4 */
#define C8724Q_DISPLAY_ADDRESS_END                  0xFFU /**< addressed display data terminator */

/**
 * @brief      send an instruction packet without payload
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  command instruction code
 * @return     status code
 *             - 0 success
 *             - 1 SPI write failed
 * @note       instruction checksum includes both header bytes and command byte
 */
static uint8_t a_c8724q_send_command(c8724q_handle_t *handle, uint8_t command)
{
    uint8_t packet[4];
    uint8_t instruction;

    instruction = (uint8_t)(command | ((handle->address & 0x03U) << C8724Q_ADDRESS_SHIFT));
    if (handle->broadcast != 0U)
    {
        instruction |= C8724Q_ADDRESS_BROADCAST_BIT;
    }
    packet[0] = C8724Q_PACKET_HEADER_0;
    packet[1] = C8724Q_PACKET_HEADER_1;
    packet[2] = instruction;
    packet[3] = (uint8_t)(packet[0] + packet[1] + packet[2]);
    if (handle->spi_write_cmd(packet, 4U) != 0U)
    {
        handle->debug_print("c8724q: write instruction failed.\n");
        return 1U;
    }

    return 0U;
}

/**
 * @brief      send four configuration bytes and their checksum
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  *config pointer to four configuration bytes
 * @return     status code
 *             - 0 success
 *             - 1 SPI write failed
 * @note       none
 */
static uint8_t a_c8724q_write_config(c8724q_handle_t *handle, const uint8_t config[4])
{
    uint8_t packet[9];
    uint8_t instruction;

    instruction = (uint8_t)(C8724Q_COMMAND_CONFIG |
                            ((handle->address & 0x03U) << C8724Q_ADDRESS_SHIFT));
    if (handle->broadcast != 0U)
    {
        instruction |= C8724Q_ADDRESS_BROADCAST_BIT;
    }
    packet[0] = C8724Q_PACKET_HEADER_0;
    packet[1] = C8724Q_PACKET_HEADER_1;
    packet[2] = instruction;
    packet[3] = (uint8_t)(packet[0] + packet[1] + packet[2]);
    packet[4] = config[0];
    packet[5] = config[1];
    packet[6] = config[2];
    packet[7] = config[3];
    packet[8] = (uint8_t)(config[0] + config[1] + config[2] + config[3]);
    if (handle->spi_write_cmd(packet, 9U) != 0U)
    {
        handle->debug_print("c8724q: write configuration failed.\n");
        return 1U;
    }

    return 0U;
}

/**
 * @brief      validate configuration reserved bits
 * @param[in]  *config pointer to four configuration bytes
 * @return     status code
 *             - 0 configuration is valid
 *             - 1 reserved bits are set
 * @note       reserved bits are kept at the values specified by the datasheet
 */
static uint8_t a_c8724q_config_valid(const uint8_t config[4])
{
    if (((config[1] & C8724Q_CONFIG_REG2_RESERVED_MASK) != 0U) ||
        ((config[3] & C8724Q_CONFIG_REG4_RESERVED_MASK) != 0U))
    {
        return 1U;
    }

    return 0U;
}

/**
 * @brief      derive enabled GRID row count from configuration register 2
 * @param[in]  reg2 configuration register 2 value
 * @return     enabled GRID row count
 * @note       SCAN encoding is the number of enabled GRID outputs minus one
 */
static uint8_t a_c8724q_get_scan_rows(uint8_t reg2)
{
    return (uint8_t)(((reg2 & C8724Q_REG2_SCAN_MASK) >> 2U) + 1U);
}

static uint8_t a_c8724q_check_handle(c8724q_handle_t *handle);

/**
 * @brief      stage one configuration field
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  reg configuration register index
 * @param[in]  mask configuration field mask
 * @param[in]  shift configuration field bit shift
 * @param[in]  value encoded field value
 * @param[in]  max maximum valid encoded value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 value is invalid
 * @note       no SPI transaction is performed
 */
static uint8_t a_c8724q_set_config_field(c8724q_handle_t *handle, uint8_t reg,
                                         uint8_t mask, uint8_t shift, uint8_t value, uint8_t max)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if ((reg >= 4U) || (value > max))
    {
        return 4U;
    }
    handle->config[reg] = (uint8_t)((handle->config[reg] & (uint8_t)(~mask)) |
                                    (uint8_t)((value << shift) & mask));
    handle->config_dirty = (uint8_t)(memcmp(handle->config, handle->applied_config, 4U) != 0);

    return 0U;
}

/**
 * @brief      read one pending configuration field
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  reg configuration register index
 * @param[in]  mask configuration field mask
 * @param[in]  shift configuration field bit shift
 * @param[out] *value pointer to the encoded field value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 register or output pointer is invalid
 * @note       reads the staged software value, not the chip
 */
static uint8_t a_c8724q_get_config_field(c8724q_handle_t *handle, uint8_t reg,
                                         uint8_t mask, uint8_t shift, uint8_t *value)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if ((reg >= 4U) || (value == NULL))
    {
        return 4U;
    }
    *value = (uint8_t)((handle->config[reg] & mask) >> shift);

    return 0U;
}

/**
 * @brief      check initialized handle
 * @param[in]  *handle pointer to a c8724q handle
 * @return     status code
 *             - 0 initialized
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       none
 */
static uint8_t a_c8724q_check_handle(c8724q_handle_t *handle)
{
    if (handle == NULL)
    {
        return 2U;
    }
    if (handle->inited != 1U)
    {
        return 3U;
    }

    return 0U;
}

/**
 * @brief      get chip information
 * @param[out] *info pointer to a c8724q info structure
 * @return     status code
 *             - 0 success
 *             - 2 info is invalid
 * @note       none
 */
uint8_t c8724q_info(c8724q_info_t *info)
{
    if (info == NULL)
    {
        return 2U;
    }
    (void)memset(info, 0, sizeof(c8724q_info_t));
    (void)memcpy(info->chip_name, "C8724", sizeof("C8724"));
    (void)memcpy(info->manufacturer_name, "Chipfountain", sizeof("Chipfountain"));
    (void)memcpy(info->interface, "SPI", sizeof("SPI"));
    info->supply_voltage_min_v = 2.9f;
    info->supply_voltage_max_v = 5.5f;
    info->max_current_ma = 40.0f;
    info->temperature_min = -40.0f;
    info->temperature_max = 85.0f;
    info->driver_version = C8724Q_DRIVER_VERSION;

    return 0U;
}

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
uint8_t c8724q_init(c8724q_handle_t *handle)
{
    uint8_t res;

    if (handle == NULL)
    {
        return 2U;
    }
    if (handle->inited == 1U)
    {
        return 3U;
    }
    if (handle->debug_print == NULL)
    {
        return 3U;
    }
    if (handle->spi_init == NULL)
    {
        handle->debug_print("c8724q: spi_init is null.\n");
        return 3U;
    }
    if (handle->spi_deinit == NULL)
    {
        handle->debug_print("c8724q: spi_deinit is null.\n");
        return 3U;
    }
    if (handle->spi_write_cmd == NULL)
    {
        handle->debug_print("c8724q: spi_write_cmd is null.\n");
        return 3U;
    }

    if (handle->spi_init() != 0U)
    {
        handle->debug_print("c8724q: spi init failed.\n");
        return 1U;
    }
    handle->address = C8724Q_ADDRESS_0;
    handle->broadcast = C8724Q_BOOL_TRUE;
    handle->config[0] = 0x3FU;
    handle->config[1] = 0x00U;
    handle->config[2] = 0xF0U;
    handle->config[3] = 0x02U;
    res = a_c8724q_send_command(handle, C8724Q_COMMAND_GLOBAL_RESET);
    if (res != 0U)
    {
        (void)handle->spi_deinit();
        return 1U;
    }
    (void)memcpy(handle->applied_config, handle->config, 4U);
    handle->config_dirty = 0U;
    handle->broadcast = C8724Q_BOOL_FALSE;
    handle->inited = 1U;

    return 0U;
}

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
uint8_t c8724q_deinit(c8724q_handle_t *handle)
{
    uint8_t config[4];
    uint8_t res;
    uint8_t status;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    status = c8724q_display_clear(handle);
    if (status != 0U)
    {
        handle->debug_print("c8724q: clear display during deinit failed.\n");
        res = 1U;
    }
    else
    {
        (void)memcpy(config, handle->config, sizeof(config));
        config[3] |= C8724Q_REG4_SLEEP_EN_MASK;
        status = c8724q_set_config(handle, config);
        if (status != 0U)
        {
            handle->debug_print("c8724q: enable sleep during deinit failed.\n");
            res = 1U;
        }
        else
        {
            status = c8724q_apply_config(handle);
            if (status == 0U)
            {
                status = a_c8724q_send_command(handle, C8724Q_COMMAND_SLEEP);
            }
            if (status != 0U)
            {
                handle->debug_print("c8724q: sleep during deinit failed.\n");
                res = 1U;
            }
        }
    }
    if (handle->spi_deinit() != 0U)
    {
        handle->debug_print("c8724q: spi deinit failed.\n");
        res = 1U;
    }
    handle->inited = 0U;

    return res;
}

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
uint8_t c8724q_set_address(c8724q_handle_t *handle, c8724q_address_t address)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if ((uint8_t)address > 3U)
    {
        return 4U;
    }
    handle->address = (uint8_t)address;

    return 0U;
}

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
uint8_t c8724q_get_address(c8724q_handle_t *handle, c8724q_address_t *address)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if (address == NULL)
    {
        return 4U;
    }
    *address = (c8724q_address_t)handle->address;

    return 0U;
}

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
uint8_t c8724q_set_broadcast(c8724q_handle_t *handle, c8724q_bool_t enable)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if ((enable != C8724Q_BOOL_FALSE) && (enable != C8724Q_BOOL_TRUE))
    {
        return 4U;
    }
    handle->broadcast = (uint8_t)enable;

    return 0U;
}

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
uint8_t c8724q_get_broadcast(c8724q_handle_t *handle, c8724q_bool_t *enable)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if (enable == NULL)
    {
        return 4U;
    }
    *enable = (c8724q_bool_t)handle->broadcast;

    return 0U;
}

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
uint8_t c8724q_set_config(c8724q_handle_t *handle, const uint8_t config[4])
{
    uint8_t staging[4];
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if (config == NULL)
    {
        return 4U;
    }
    (void)memcpy(staging, config, sizeof(staging));
    if (a_c8724q_config_valid(staging) != 0U)
    {
        return 4U;
    }
    (void)memcpy(handle->config, staging, sizeof(staging));
    handle->config_dirty = (uint8_t)(memcmp(handle->config, handle->applied_config, 4U) != 0);

    return 0U;
}

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
uint8_t c8724q_get_config(c8724q_handle_t *handle, uint8_t config[4])
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if (config == NULL)
    {
        return 4U;
    }
    (void)memcpy(config, handle->config, 4U);

    return 0U;
}

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
uint8_t c8724q_apply_config(c8724q_handle_t *handle)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if (a_c8724q_config_valid(handle->config) != 0U)
    {
        return 4U;
    }
    res = a_c8724q_write_config(handle, handle->config);
    if (res != 0U)
    {
        return res;
    }
    (void)memcpy(handle->applied_config, handle->config, 4U);
    handle->config_dirty = 0U;

    return 0U;
}

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
uint8_t c8724q_get_applied_config(c8724q_handle_t *handle, uint8_t config[4])
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if (config == NULL)
    {
        return 4U;
    }
    (void)memcpy(config, handle->applied_config, 4U);

    return 0U;
}

/**
 * @brief      stage configuration register 1 raw value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value configuration register 1 value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       use feature setters where possible
 */
uint8_t c8724q_set_config_reg1(c8724q_handle_t *handle, uint8_t value)
{
    uint8_t config[4];
    uint8_t res;

    res = c8724q_get_config(handle, config);
    if (res != 0U)
    {
        return res;
    }
    config[0] = value;

    return c8724q_set_config(handle, config);
}

/**
 * @brief      get pending configuration register 1 value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[out] *value pointer to the output value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 value is null
 * @note       returns the pending software value
 */
uint8_t c8724q_get_config_reg1(c8724q_handle_t *handle, uint8_t *value)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if (value == NULL)
    {
        return 4U;
    }
    *value = handle->config[0];

    return 0U;
}

/**
 * @brief      stage configuration register 2 raw value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value configuration register 2 value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 reserved bits are set
 * @note       bits 6 and 5 are reserved and must be zero
 */
uint8_t c8724q_set_config_reg2(c8724q_handle_t *handle, uint8_t value)
{
    uint8_t config[4];
    uint8_t res;

    res = c8724q_get_config(handle, config);
    if (res != 0U)
    {
        return res;
    }
    if ((value & C8724Q_CONFIG_REG2_RESERVED_MASK) != 0U)
    {
        return 4U;
    }
    config[1] = value;

    return c8724q_set_config(handle, config);
}

/**
 * @brief      get pending configuration register 2 value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[out] *value pointer to the output value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 value is null
 * @note       returns the pending software value
 */
uint8_t c8724q_get_config_reg2(c8724q_handle_t *handle, uint8_t *value)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if (value == NULL)
    {
        return 4U;
    }
    *value = handle->config[1];

    return 0U;
}

/**
 * @brief      stage configuration register 3 raw value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value configuration register 3 value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 * @note       RCS=0 is documented; RCS=1 behavior is unspecified
 */
uint8_t c8724q_set_config_reg3(c8724q_handle_t *handle, uint8_t value)
{
    uint8_t config[4];
    uint8_t res;

    res = c8724q_get_config(handle, config);
    if (res != 0U)
    {
        return res;
    }
    config[2] = value;

    return c8724q_set_config(handle, config);
}

/**
 * @brief      get pending configuration register 3 value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[out] *value pointer to the output value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 value is null
 * @note       returns the pending software value
 */
uint8_t c8724q_get_config_reg3(c8724q_handle_t *handle, uint8_t *value)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if (value == NULL)
    {
        return 4U;
    }
    *value = handle->config[2];

    return 0U;
}

/**
 * @brief      stage configuration register 4 raw value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[in]  value configuration register 4 value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 reserved bit is set
 * @note       bit 7 is reserved and must be zero
 */
uint8_t c8724q_set_config_reg4(c8724q_handle_t *handle, uint8_t value)
{
    uint8_t config[4];
    uint8_t res;

    res = c8724q_get_config(handle, config);
    if (res != 0U)
    {
        return res;
    }
    if ((value & C8724Q_CONFIG_REG4_RESERVED_MASK) != 0U)
    {
        return 4U;
    }
    config[3] = value;

    return c8724q_set_config(handle, config);
}

/**
 * @brief      get pending configuration register 4 value
 * @param[in]  *handle pointer to a c8724q handle
 * @param[out] *value pointer to the output value
 * @return     status code
 *             - 0 success
 *             - 2 handle is invalid
 *             - 3 chip is not initialized
 *             - 4 value is null
 * @note       returns the pending software value
 */
uint8_t c8724q_get_config_reg4(c8724q_handle_t *handle, uint8_t *value)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if (value == NULL)
    {
        return 4U;
    }
    *value = handle->config[3];

    return 0U;
}

#define C8724Q_DEFINE_CONFIG_FEATURE(name, type, reg, mask, shift, maximum) \
uint8_t c8724q_set_##name(c8724q_handle_t *handle, type value) \
{ \
    return a_c8724q_set_config_field(handle, (reg), (mask), (shift), (uint8_t)value, (maximum)); \
} \
uint8_t c8724q_get_##name(c8724q_handle_t *handle, type *value) \
{ \
    uint8_t raw; \
    uint8_t res; \
    if (value == NULL) \
    { \
        res = a_c8724q_check_handle(handle); \
        return (res == 0U) ? 4U : res; \
    } \
    res = a_c8724q_get_config_field(handle, (reg), (mask), (shift), &raw); \
    if (res == 0U) \
    { \
        *value = (type)raw; \
    } \
    return res; \
}

C8724Q_DEFINE_CONFIG_FEATURE(seg11_mode, c8724q_seg_pin_mode_t, 0U,
                             C8724Q_REG1_SEG11_CS_MASK, 6U, C8724Q_SEG_PIN_LED_OUTPUT)
C8724Q_DEFINE_CONFIG_FEATURE(seg12_mode, c8724q_seg_pin_mode_t, 0U,
                             C8724Q_REG1_SEG12_CS_MASK, 7U, C8724Q_SEG_PIN_LED_OUTPUT)
C8724Q_DEFINE_CONFIG_FEATURE(global_current_gain, uint8_t, 0U,
                             C8724Q_REG1_GCC_MASK, 0U, C8724Q_REG1_GCC_MASK)
C8724Q_DEFINE_CONFIG_FEATURE(clock_source, c8724q_clock_source_t, 1U,
                             C8724Q_REG2_CKS_MASK, 7U, C8724Q_CLOCK_SOURCE_EXTERNAL)
C8724Q_DEFINE_CONFIG_FEATURE(scan, c8724q_scan_t, 1U,
                             C8724Q_REG2_SCAN_MASK, 2U, C8724Q_SCAN_8_ROWS)
C8724Q_DEFINE_CONFIG_FEATURE(test_mode, c8724q_bool_t, 1U,
                             C8724Q_REG2_WM_MASK, 1U, C8724Q_BOOL_TRUE)
C8724Q_DEFINE_CONFIG_FEATURE(update_mode, c8724q_update_mode_t, 1U,
                             C8724Q_REG2_VSYN_M_MASK, 0U, C8724Q_UPDATE_MODE_SCAN_END)
C8724Q_DEFINE_CONFIG_FEATURE(otp1_protection, c8724q_protection_t, 2U,
                             C8724Q_REG3_OTP1_MASK, 7U, C8724Q_PROTECTION_DISABLED)
C8724Q_DEFINE_CONFIG_FEATURE(rcs_bit, uint8_t, 2U, C8724Q_REG3_RCS_MASK, 6U, 1U)
C8724Q_DEFINE_CONFIG_FEATURE(internal_clock, c8724q_internal_clock_t, 2U,
                             C8724Q_REG3_RCKS_MASK, 4U, C8724Q_INTERNAL_CLOCK_8_MHZ)
C8724Q_DEFINE_CONFIG_FEATURE(output_enable, c8724q_bool_t, 2U,
                             C8724Q_REG3_OE_MASK, 3U, C8724Q_BOOL_TRUE)
C8724Q_DEFINE_CONFIG_FEATURE(ghost_removal, c8724q_ghost_removal_t, 2U,
                             C8724Q_REG3_DGH_MASK, 2U, C8724Q_GHOST_REMOVAL_STRONG)
C8724Q_DEFINE_CONFIG_FEATURE(line_blanking, c8724q_line_blanking_t, 2U,
                             C8724Q_REG3_TLS_MASK, 0U, C8724Q_LINE_BLANKING_16T)
C8724Q_DEFINE_CONFIG_FEATURE(sleep_enable, c8724q_bool_t, 3U,
                             C8724Q_REG4_SLEEP_EN_MASK, 6U, C8724Q_BOOL_TRUE)
C8724Q_DEFINE_CONFIG_FEATURE(auto_sleep_enable, c8724q_bool_t, 3U,
                             C8724Q_REG4_SLEEP_AT_MASK, 5U, C8724Q_BOOL_TRUE)
C8724Q_DEFINE_CONFIG_FEATURE(global_reset_enable, c8724q_bool_t, 3U,
                             C8724Q_REG4_GRST_MASK, 4U, C8724Q_BOOL_TRUE)
C8724Q_DEFINE_CONFIG_FEATURE(soft_reset_enable, c8724q_bool_t, 3U,
                             C8724Q_REG4_SRST_MASK, 3U, C8724Q_BOOL_TRUE)
C8724Q_DEFINE_CONFIG_FEATURE(scan_clock_double_enable, c8724q_bool_t, 3U,
                             C8724Q_REG4_CLK2X_MASK, 2U, C8724Q_BOOL_TRUE)
C8724Q_DEFINE_CONFIG_FEATURE(otp2_protection, c8724q_protection_t, 3U,
                             C8724Q_REG4_OTP2_MASK, 1U, C8724Q_PROTECTION_DISABLED)
C8724Q_DEFINE_CONFIG_FEATURE(power_down_reset_enable, c8724q_bool_t, 3U,
                             C8724Q_REG4_PDR_MASK, 0U, C8724Q_BOOL_TRUE)

#undef C8724Q_DEFINE_CONFIG_FEATURE

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
uint8_t c8724q_write_display(c8724q_handle_t *handle, const uint8_t *data, uint16_t len)
{
    uint8_t packet_header[4];
    uint8_t checksum;
    uint8_t instruction;
    uint8_t res;
    uint16_t i;
    uint16_t expected_len;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    expected_len = (uint16_t)(C8724Q_DISPLAY_WIDTH * a_c8724q_get_scan_rows(handle->applied_config[1]));
    if ((data == NULL) || (len != expected_len))
    {
        return 4U;
    }
    instruction = (uint8_t)(C8724Q_COMMAND_DISPLAY_SEQUENTIAL |
                            ((handle->address & 0x03U) << C8724Q_ADDRESS_SHIFT));
    if (handle->broadcast != 0U)
    {
        instruction |= C8724Q_ADDRESS_BROADCAST_BIT;
    }
    packet_header[0] = C8724Q_PACKET_HEADER_0;
    packet_header[1] = C8724Q_PACKET_HEADER_1;
    packet_header[2] = instruction;
    packet_header[3] = (uint8_t)(packet_header[0] + packet_header[1] + packet_header[2]);
    if (handle->spi_write_cmd(packet_header, 3U) != 0U)
    {
        handle->debug_print("c8724q: write display header failed.\n");
        return 1U;
    }
    if (handle->spi_write_cmd(&packet_header[3], 1U) != 0U)
    {
        handle->debug_print("c8724q: write display instruction checksum failed.\n");
        return 1U;
    }
    if (handle->spi_write_cmd(data, len) != 0U)
    {
        handle->debug_print("c8724q: write display data failed.\n");
        return 1U;
    }
    checksum = 0U;
    for (i = 0U; i < len; i++)
    {
        checksum = (uint8_t)(checksum + data[i]);
    }
    if (handle->spi_write_cmd(&checksum, 1U) != 0U)
    {
        handle->debug_print("c8724q: write display checksum failed.\n");
        return 1U;
    }

    return 0U;
}

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
                                     const uint8_t *data, uint16_t len)
{
    uint8_t packet_header[4];
    uint8_t pair[2];
    uint8_t checksum;
    uint8_t instruction;
    uint8_t rows;
    uint8_t grid;
    uint8_t segment;
    uint8_t res;
    uint16_t i;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    rows = a_c8724q_get_scan_rows(handle->applied_config[1]);
    if ((address == NULL) || (data == NULL) || (len == 0U) || (len > C8724Q_ADDRESS_DATA_MAX))
    {
        return 4U;
    }
    for (i = 0U; i < len; i++)
    {
        grid = (uint8_t)(address[i] >> 4U);
        segment = (uint8_t)(address[i] & 0x0FU);
        if ((grid >= rows) || (segment >= C8724Q_DISPLAY_WIDTH))
        {
            return 4U;
        }
    }
    instruction = (uint8_t)(C8724Q_COMMAND_DISPLAY_ADDRESS |
                            ((handle->address & 0x03U) << C8724Q_ADDRESS_SHIFT));
    if (handle->broadcast != 0U)
    {
        instruction |= C8724Q_ADDRESS_BROADCAST_BIT;
    }
    packet_header[0] = C8724Q_PACKET_HEADER_0;
    packet_header[1] = C8724Q_PACKET_HEADER_1;
    packet_header[2] = instruction;
    packet_header[3] = (uint8_t)(packet_header[0] + packet_header[1] + packet_header[2]);
    if (handle->spi_write_cmd(packet_header, 4U) != 0U)
    {
        handle->debug_print("c8724q: write addressed display header failed.\n");
        return 1U;
    }
    checksum = 0U;
    for (i = 0U; i < len; i++)
    {
        pair[0] = address[i];
        pair[1] = data[i];
        checksum = (uint8_t)(checksum + pair[0] + pair[1]);
        if (handle->spi_write_cmd(pair, 2U) != 0U)
        {
            handle->debug_print("c8724q: write addressed display data failed.\n");
            return 1U;
        }
    }
    pair[0] = C8724Q_DISPLAY_ADDRESS_END;
    checksum = (uint8_t)(checksum + C8724Q_DISPLAY_ADDRESS_END);
    pair[1] = checksum;
    if (handle->spi_write_cmd(pair, 2U) != 0U)
    {
        handle->debug_print("c8724q: write addressed display checksum failed.\n");
        return 1U;
    }

    return 0U;
}

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
uint8_t c8724q_display_update(c8724q_handle_t *handle)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }

    return a_c8724q_send_command(handle, C8724Q_COMMAND_DISPLAY_UPDATE);
}

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
uint8_t c8724q_display_clear(c8724q_handle_t *handle)
{
    uint8_t data[C8724Q_DISPLAY_DATA_MAX];
    uint8_t res;
    uint16_t len;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    (void)memset(data, 0, sizeof(data));
    len = (uint16_t)(C8724Q_DISPLAY_WIDTH * a_c8724q_get_scan_rows(handle->applied_config[1]));
    res = c8724q_write_display(handle, data, len);
    if (res != 0U)
    {
        return res;
    }

    return c8724q_display_update(handle);
}

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
uint8_t c8724q_soft_reset(c8724q_handle_t *handle)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }

    return a_c8724q_send_command(handle, C8724Q_COMMAND_SOFT_RESET);
}

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
uint8_t c8724q_global_reset(c8724q_handle_t *handle)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    res = a_c8724q_send_command(handle, C8724Q_COMMAND_GLOBAL_RESET);
    if (res == 0U)
    {
        handle->config[0] = 0x3FU;
        handle->config[1] = 0x00U;
        handle->config[2] = 0xF0U;
        handle->config[3] = 0x02U;
        (void)memcpy(handle->applied_config, handle->config, 4U);
        handle->config_dirty = 0U;
    }

    return res;
}

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
uint8_t c8724q_sleep(c8724q_handle_t *handle)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }
    if ((handle->applied_config[3] & C8724Q_REG4_SLEEP_EN_MASK) == 0U)
    {
        return 4U;
    }

    return a_c8724q_send_command(handle, C8724Q_COMMAND_SLEEP);
}

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
uint8_t c8724q_wake(c8724q_handle_t *handle)
{
    uint8_t res;

    res = a_c8724q_check_handle(handle);
    if (res != 0U)
    {
        return res;
    }

    return a_c8724q_send_command(handle, C8724Q_COMMAND_WAKE);
}
