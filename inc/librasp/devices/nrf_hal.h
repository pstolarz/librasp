/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic
 * Semiconductor ASA.Terms and conditions of usage are described in detail
 * in NORDIC SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRENTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 */
/*
   Copyright (c) 2015,2016 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/**
 * This is basically nRFgo SDK's NRF HAL API ported to work with librasp library.
 *
 * The differences are:
 * 1. Addition of hal_nrf_set_spi_hndl() function, which need to be called at
 *    first (before any other NRF HALL API call) to set the SPI communication
 *    channel with the nRF24L01+ transceiver.
 * 2. Since the original NRF HAL API assumed successful SPI communication with
 *    the transceiver, this implementation uses "errno approach" to inform an
 *    API caller about possible problems with the SPI communication. A caller
 *    should check the errno value against ECOMM after any API call to detect
 *    SPI communication issues.
 * 3. Added new functions, mostly getters for already existing setters.
 */

/**
 * This is the nRF24L01+ transceiver used in several Nordic Semiconductor
 * devices. The transceiver is set up and controlled via an internal SPI
 * interface on the chip. The HAL for the radio transceiver hides this SPI
 * interface from the programmer.
 *
 * The nRF24LE1 uses the same 2.4GHz GFSK RF transceiver with embedded protocol
 * engine (Enhanced ShockBurst&tm;) that is found in the nRF24L01+ single chip
 * RF Transceiver.
 *
 * The RF Transceiver module is configured and operated through the RF
 * transceiver map. This register map is accessed by the MCU through a dedicated
 * on-chip Serial Peripheral interface (SPI) and is available in all power modes
 * of the RF Transceiver module. The register map contains all configuration
 * registers in the RF Transceiver and is accessible in all operation modes of
 * the transceiver. The radio transceiver HAL hides this register map and the
 * usage of the internal SPI.
 *
 * This HAL module contains setup functions for configurating the radio;
 * operation functions for controlling the radio when active and for sending
 * and receiving data; and test functions for setting the radio in test modes.
 */

#ifndef __LR_DEVS_NRF_HAL_H__
#define __LR_DEVS_NRF_HAL_H__

#include <stdint.h>
#include <stdbool.h>
#include "librasp/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* max payload size */
#define NRF_MAX_PL         32

/* nRF24L01 register details
 */

/* nRF24L01 Instruction Definitions */
#define R_REGISTER         0x00U  /* Register read (added to register id) */
#define W_REGISTER         0x20U  /* Register write (added to register id) */
#define R_RX_PAYLOAD       0x61U  /* Read RX payload */
#define W_TX_PAYLOAD       0xA0U  /* Write TX payload */
#define FLUSH_TX           0xE1U  /* Flush TX register */
#define FLUSH_RX           0xE2U  /* Flush RX register */
#define REUSE_TX_PL        0xE3U  /* Reuse TX payload */
#define ACTIVATE           0x50U  /* Activate features (obsolete) */
#define R_RX_PL_WID        0x60U  /* Read top RX FIFO payload width */
#define W_ACK_PAYLOAD      0xA8U  /* Write ACK payload */
#define W_TX_PAYLOAD_NOACK 0xB0U  /* Write TX payload (no ACK req.) */
#define NOP                0xFFU  /* No Operation,
                                     used for reading status register */

/* nRF24L01 Register Definitions */
#define CONFIG        0x00U  /* nRF24L01 config register */
#define EN_AA         0x01U  /* nRF24L01 enable Auto-Acknowledge register */
#define EN_RXADDR     0x02U  /* nRF24L01 enable RX addresses register */
#define SETUP_AW      0x03U  /* nRF24L01 setup of address width register */
#define SETUP_RETR    0x04U  /* nRF24L01 setup of automatic retransmission reg */
#define RF_CH         0x05U  /* nRF24L01 RF channel register */
#define RF_SETUP      0x06U  /* nRF24L01 RF setup register */
#define STATUS        0x07U  /* nRF24L01 status register */
#define OBSERVE_TX    0x08U  /* nRF24L01 transmit observe register */
#define CD            0x09U  /* nRF24L01 carrier detect register */
#define RX_ADDR_P0    0x0AU  /* nRF24L01 receive address data pipe0 */
#define RX_ADDR_P1    0x0BU  /* nRF24L01 receive address data pipe1 */
#define RX_ADDR_P2    0x0CU  /* nRF24L01 receive address data pipe2 */
#define RX_ADDR_P3    0x0DU  /* nRF24L01 receive address data pipe3 */
#define RX_ADDR_P4    0x0EU  /* nRF24L01 receive address data pipe4 */
#define RX_ADDR_P5    0x0FU  /* nRF24L01 receive address data pipe5 */
#define TX_ADDR       0x10U  /* nRF24L01 transmit address */
#define RX_PW_P0      0x11U  /* nRF24L01 # of bytes in RX payload for pipe0 */
#define RX_PW_P1      0x12U  /* nRF24L01 # of bytes in RX payload for pipe1 */
#define RX_PW_P2      0x13U  /* nRF24L01 # of bytes in RX payload for pipe2 */
#define RX_PW_P3      0x14U  /* nRF24L01 # of bytes in RX payload for pipe3 */
#define RX_PW_P4      0x15U  /* nRF24L01 # of bytes in RX payload for pipe4 */
#define RX_PW_P5      0x16U  /* nRF24L01 # of bytes in RX payload for pipe5 */
#define FIFO_STATUS   0x17U  /* nRF24L01 FIFO status register */
#define DYNPD         0x1CU  /* nRF24L01 Dynamic payload setup */
#define FEATURE       0x1DU  /* nRF24L01 Exclusive feature setup */

/* CONFIG register bit definitions */
#define MASK_RX_DR    6     /* CONFIG register bit 6 */
#define MASK_TX_DS    5     /* CONFIG register bit 5 */
#define MASK_MAX_RT   4     /* CONFIG register bit 4 */
#define EN_CRC        3     /* CONFIG register bit 3 */
#define CRCO          2     /* CONFIG register bit 2 */
#define PWR_UP        1     /* CONFIG register bit 1 */
#define PRIM_RX       0     /* CONFIG register bit 0 */

/* RF_SETUP register bit definitions */
#define CONT_WAVE     7     /* RF_SETUP register bit 7 */
#define RF_DR_LOW     5     /* RF_SETUP register bit 5 */
#define PLL_LOCK      4     /* RF_SETUP register bit 4 */
#define RF_DR_HIGH    3     /* RF_SETUP register bit 3 */
#define RF_PWR1       2     /* RF_SETUP register bit 2 */
#define RF_PWR0       1     /* RF_SETUP register bit 1 */
#define LNA_HCURR     0     /* RF_SETUP register bit 0 */

/* STATUS register bit definitions */
#define RX_DR         6     /* STATUS register bit 6 */
#define TX_DS         5     /* STATUS register bit 5 */
#define MAX_RT        4     /* STATUS register bit 4 */
#define TX_FULL       0     /* STATUS register bit 0 */

/* FIFO_STATUS register bit definitions */
#define TX_REUSE      6     /* FIFO_STATUS register bit 6 */
#define TX_FIFO_FULL  5     /* FIFO_STATUS register bit 5 */
#define TX_EMPTY      4     /* FIFO_STATUS register bit 4 */
#define RX_FULL       1     /* FIFO_STATUS register bit 1 */
#define RX_EMPTY      0     /* FIFO_STATUS register bit 0 */

/* FEATURE register bit definitions */
#define EN_DPL        2     /* FEATURE register bit 2 */
#define EN_ACK_PAY    1     /* FEATURE register bit 1 */
#define EN_DYN_ACK    0     /* FEATURE register bit 0 */

/* nRF24L01 related definitions,
   Interrupt definitions,
   Operation mode definitions
 */

/**
 * An enum describing the radio's irq sources.
 */
typedef enum {
    HAL_NRF_MAX_RT = 4,     /* Max retries interrupt */
    HAL_NRF_TX_DS,          /* TX data sent interrupt */
    HAL_NRF_RX_DR           /* RX data received interrupt */
} hal_nrf_irq_source_t;

/* Operation mode definitions */

/**
 * An enum describing the radio's power mode.
 */
typedef enum {
    HAL_NRF_PTX,            /* Primary TX operation */
    HAL_NRF_PRX             /* Primary RX operation */
} hal_nrf_operation_mode_t;

/**
 * An enum describing the radio's power mode.
 */
typedef enum {
    HAL_NRF_PWR_DOWN,       /* Device power-down */
    HAL_NRF_PWR_UP          /* Device power-up */
} hal_nrf_pwr_mode_t;

/**
 * An enum describing the radio's output power mode's.
 */
typedef enum {
    HAL_NRF_18DBM,          /* Output power set to -18dBm */
    HAL_NRF_12DBM,          /* Output power set to -12dBm */
    HAL_NRF_6DBM,           /* Output power set to -6dBm  */
    HAL_NRF_0DBM            /* Output power set to 0dBm   */
} hal_nrf_output_power_t;

/**
 * An enum describing the radio's on-air data-rate.
 */
typedef enum {
    HAL_NRF_1MBPS,          /* Data-rate set to 1 Mbps  */
    HAL_NRF_2MBPS,          /* Data-rate set to 2 Mbps  */
    HAL_NRF_250KBPS         /* Data-rate set to 250 kbps*/
} hal_nrf_datarate_t;

/**
 * An enum describing the radio's CRC mode.
 */
typedef enum {
    HAL_NRF_CRC_OFF,    /* CRC check disabled */
    HAL_NRF_CRC_8BIT,   /* CRC check set to 8-bit */
    HAL_NRF_CRC_16BIT   /* CRC check set to 16-bit */
} hal_nrf_crc_mode_t;

/**
 * An enum describing the read/write payload command.
 */
typedef enum {
    HAL_NRF_TX_PLOAD = 7,   /* TX payload definition */
    HAL_NRF_RX_PLOAD,       /* RX payload definition */
    HAL_NRF_ACK_PLOAD
} hal_nrf_pload_command_t;

#if 0
/**
 * Structure containing the radio's address map.
 *
 * Pipe0 contains 5 unique address bytes, while pipe[1..5] share the 4 MSB
 * bytes, set in pipe1. Remember that the LSB byte for all pipes have to be
 * unique!
 */
/* nRF24L01 Address struct */
typedef struct {
    uint8_t p0[5];            /* Pipe0 address, 5 bytes */
    uint8_t p1[5];            /* Pipe1 address, 5 bytes,
                                 4 MSB bytes shared for pipe 1 to 5 */
    uint8_t p2[1];            /* Pipe2 address, 1 byte */
    uint8_t p3[1];            /* Pipe3 address, 1 byte */
    uint8_t p4[1];            /* Pipe3 address, 1 byte */
    uint8_t p5[1];            /* Pipe3 address, 1 byte */
    uint8_t tx[5];            /* TX address, 5 byte */
} hal_nrf_l01_addr_map;
#endif

/**
 * An enum describing the nRF24L01 pipe addresses and TX address.
 */
typedef enum {
    HAL_NRF_PIPE0 = 0,          /* Select pipe0 */
    HAL_NRF_PIPE1,              /* Select pipe1 */
    HAL_NRF_PIPE2,              /* Select pipe2 */
    HAL_NRF_PIPE3,              /* Select pipe3 */
    HAL_NRF_PIPE4,              /* Select pipe4 */
    HAL_NRF_PIPE5,              /* Select pipe5 */
    HAL_NRF_TX,                 /* Refer to TX address*/
    HAL_NRF_ALL = 0xFF          /* Close or open all pipes*/
} hal_nrf_address_t;

/**
 * An enum describing the radio's address width.
 */
typedef enum {
    HAL_NRF_AW_3BYTES = 3,      /* Set address width to 3 bytes */
    HAL_NRF_AW_4BYTES,          /* Set address width to 4 bytes */
    HAL_NRF_AW_5BYTES           /* Set address width to 5 bytes */
} hal_nrf_address_width_t;

/**
 * Transceiver context.
 */
typedef struct {
    uint8_t config;
    uint8_t en_aa;
    uint8_t en_rxaddr;
    uint8_t setup_aw;
    uint8_t setup_retr;
    uint8_t rf_ch;
    uint8_t rf_setup;
    /* STATUS, OBSERVE_TX, CD omitted */
    uint8_t rx_addr_p0[5];
    uint8_t rx_addr_p1[5];
    uint8_t rx_addr_p2;
    uint8_t rx_addr_p3;
    uint8_t rx_addr_p4;
    uint8_t rx_addr_p5;
    uint8_t tx_addr[5];
    uint8_t rx_pw_p0;
    uint8_t rx_pw_p1;
    uint8_t rx_pw_p2;
    uint8_t rx_pw_p3;
    uint8_t rx_pw_p4;
    uint8_t rx_pw_p5;
    /* FIFO_STATUS omitted */
    uint8_t dynpd;
    uint8_t feature;
} hal_nrf_ctx_t;

/*
 * Setup functions prototypes
 */

/**
 * Initialize NRF HAL module with SPI handle used for communication with
 * the transceiver. The function MUST be called before any subsequent NRF HAL
 * API calls. The SPI handle must be initialized in MODE0 (CPOL=0, CPHA=0) with
 * MSB transmitted first and 8-bit length word otherwise the error will
 * be reported (function will return FALSE).
 *
 * NOTE: For a single transceiver (the most common case) connected to a board
 * the function is called once with a SPI handle initiated for communication
 * with the transceiver. If more than one transceiver is connected to a board
 * and there is a need to handle them via the NRF HAL API from the same process,
 * a caller need to set proper SPI handle (associated with the transceiver)
 * before API calls devoted for a specific transceiver. In this case any thread
 * synchronization issues should be resolved by the caller.
 */
bool hal_nrf_set_spi_hndl(spi_hndl_t *p_hndl);

/**
 * Set radio's operation mode.
 *
 * Use this function to enter PTX (primary TX) or PRX (primary RX).
 *
 * @param op_mode Operation mode.
 */
void hal_nrf_set_operation_mode(hal_nrf_operation_mode_t op_mode);

/**
 * Get radio's operation mode.
 */
hal_nrf_operation_mode_t hal_nrf_get_operation_mode(void);

/**
 * For the obsolete nRF24L01 it is necessary to issue an activate command before
 * the features enabled by the FEATURE register can be used. For nRF24L01+ these
 * features are by default enabled.
 */
void hal_nrf_activate_features(void);

/**
 * Enables the dynamic packet length.
 *
 * @param enable Whether enable or disable dynamic packet length.
 */
void hal_nrf_enable_dynamic_payload(bool enable);

/**
 * Check if dynamic packet length feature is enabled.
 */
bool hal_nrf_is_dynamic_payload_enabled(void);

/**
 * Enables the ACK payload feature.
 *
 * @param enable Whether to enable or disable ACK payload.
 */
void hal_nrf_enable_ack_payload(bool enable);

/**
 * Check if ACK payload feature is enabled.
 */
bool hal_nrf_is_ack_payload_enabled(void);

/**
 * Enables the dynamic ACK feature.
 *
 * @param enable Whether to enable or disable dynamic ACK.
 */
void hal_nrf_enable_dynamic_ack(bool enable);

/**
 * Check if dynamic ACK feature is enabled.
 */
bool hal_nrf_is_dynamic_ack_enabled(void);

/**
 * Function for enabling dynamic payload size.
 *
 * The input parameter is a byte where the bit values tells weather the pipe
 * should use dynamic payload size. For example if bit 0 is set then pipe 0
 * will accept dynamic payload size.
 *
 * @param setup Byte value telling for which pipes to enable dynamic payload
 * size.
 */
void hal_nrf_setup_dynamic_payload(uint8_t setup);

/**
 * Write ACK payload.
 *
 * Writes the payload that will be transmitted with the ACK on a given pipe.
 *
 * @param pipe Pipe that transmits the payload.
 * @param tx_pload Pointer to the payload data.
 * @param length Size of the data to transmit.
 */
void hal_nrf_write_ack_payload(
    uint8_t pipe, const uint8_t *tx_pload, uint8_t length);

/**
 * Set radio's RF channel.
 *
 * Use this function to select which RF channel to use.
 *
 * @param channel RF channel.
 */
void hal_nrf_set_rf_channel(uint8_t channel);

/**
 * Get radio's RF channel.
 */
uint8_t hal_nrf_get_rf_channel(void);

/**
 * Set radio's TX output power.
 *
 * Use this function set the radio's TX output power.
 *
 * @param power Radio's TX output power.
 */
void hal_nrf_set_output_power(hal_nrf_output_power_t power);

/**
 * Get radio's TX output power.
 */
hal_nrf_output_power_t hal_nrf_get_output_power(void);

/**
 * Set radio's on-air data-rate.
 *
 * Use this function to select radio's on-air data-rate.
 *
 * @param datarate On-air data-rate
 */
void hal_nrf_set_datarate(hal_nrf_datarate_t datarate);

/**
 * Get radio's on-air data-rate.
 */
hal_nrf_datarate_t hal_nrf_get_datarate(void);

/**
 * Set CRC mode used by the radio.
 *
 * Use this function to set CRC mode: CRC disabled, 1 or 2 bytes.
 *
 * @param crc_mode CRC mode to use.
 */
void hal_nrf_set_crc_mode(hal_nrf_crc_mode_t crc_mode);

/**
 * Get CRC mode used by the radio.
 */
hal_nrf_crc_mode_t hal_nrf_get_crc_mode(void);

/**
 * Set auto retransmission parameters.
 *
 * Use this function to set auto retransmission parameters.
 *
 * @param retr Number of retransmits, 0 means retransmit OFF.
 * @param delay Retransmit delay in usec (in range 250-4000 with step 250).
 */
void hal_nrf_set_auto_retr(uint8_t retr, uint16_t delay);

/**
 * Get auto retransmission number of retransmits.
 */
uint8_t hal_nrf_get_auto_retr_ctr(void);

/**
 * Get auto retransmission delay (usec).
 */
uint16_t hal_nrf_get_auto_retr_delay(void);

/**
 * Set payload width for selected pipe.
 *
 * Use this function to set the number of bytes expected on a selected pipe.
 *
 * @param pipe_num Pipe number to set payload width for.
 * @param pload_width number of bytes expected.
 */
void hal_nrf_set_rx_payload_width(uint8_t pipe_num, uint8_t pload_width);

/**
 * Get RX payload width for selected pipe.
 *
 * Use this function to get the expected payload width for selected pipe number.
 *
 * @param pipe_num Pipe number to get payload width for.
 * @return Payload_Width in bytes.
 */
uint8_t hal_nrf_get_rx_payload_width(uint8_t pipe_num);

/**
 * Open radio pipe and enable/disable auto acknowledge.
 *
 * Use this function to open one or all pipes, with or without auto acknowledge.
 *
 * @param pipe_num Radio pipe to open.
 * @param auto_ack Auto_Ack ON/OFF.
 */
void hal_nrf_open_pipe(hal_nrf_address_t pipe_num, bool auto_ack);

/**
 * Close radio pipe.
 *
 * Use this function to close one pipe or all pipes.
 *
 * @param pipe_num Pipe# number to close.
 */
void hal_nrf_close_pipe(hal_nrf_address_t pipe_num);

/**
 * Get pipe status.
 *
 * Use this function to check status for a selected pipe.
 *
 * @param  pipe_num Pipe number to check status for.
 * @return Pipe_Status.
 * @retval 0x00 Pipe is closed, autoack disabled,
 * @retval 0x01 Pipe is open, autoack disabled,
 * @retval 0x03 Pipe is open, autoack enabled.
 */
uint8_t hal_nrf_get_pipe_status(uint8_t pipe_num);

/**
 * Set radio's address width.
 *
 * Use this function to define the radio's address width, refers to both
 * RX and TX.
 *
 * @param address_width Address with in bytes.
 */
void hal_nrf_set_address_width(hal_nrf_address_width_t address_width);

/**
 * Gets the radio's address width.
 *
 * @return Address width
 */
uint8_t hal_nrf_get_address_width(void);

/**
 * Set radio's RX address and TX address.
 *
 * Use this function to set a RX address, or to set the TX address. Beware of
 * the difference for single and multibyte address registers.
 *
 * @param address Which address to set.
 * @param *addr Buffer from which the address is stored in.
 */
void hal_nrf_set_address(const hal_nrf_address_t address, const uint8_t *addr);

/**
 * Get address for selected pipe.
 *
 * Use this function to get address for selected pipe.
 *
 * @param address Which address to get, Pipe- or TX-address.
 * @param *addr buffer in which address bytes are written. For pipes containing
 * only LSB byte of address, this byte is returned in the *addr buffer.
 * @return Numbers of bytes copied to addr
 */
uint8_t hal_nrf_get_address(uint8_t address, uint8_t *addr);

/**
 * Configures pipe for transmission by:
 * 1. Enabling a pipe,
 * 2. Setting its address,
 * 3. Setting RX payload length.
 *
 * @param pipe_num Pipe to open.
 * @param addr Pipe address, if 0: don't change the address.
 * @param auto_ack Auto_Ack ON/OFF.
 * @param pload_width RX payload length. Set to max payload length (NRF_MAX_PL)
 * in case if dynamic feature is enabled.
 */
void hal_nrf_config_rx_pipe(hal_nrf_address_t pipe_num,
    const uint8_t *addr, bool auto_ack, uint8_t pload_width);

/**
 * Configure TX transmission by:
 * 1. Setting receiver address.
 * 2. Setting TX output power.
 * 3. Open pipe 0 for receiving ACKs packets.
 * 4. Setting auto-retry parameters.
 *
 * @param addr Receiver address, if 0: don't change the address.
 * @param power TX output power.
 * @param retr Number of retransmits, 0 means retransmit OFF.
 * @param delay Retransmit delay in usec (in range 250-4000 with step 250).
 */
void hal_nrf_config_tx(const uint8_t *addr,
    hal_nrf_output_power_t power, uint8_t retr, uint16_t delay);

/**
 * Enable or disable interrupt for the radio.
 *
 * Use this function to enable or disable one of the interrupt sources for the
 * radio. This function only changes state for selected int_source, the rest of
 * the interrupt sources are left unchanged.
 *
 * @param int_source Radio interrupt Source.
 * @param irq_state Enable or Disable.
 */
void hal_nrf_set_irq_mode(hal_nrf_irq_source_t int_source, bool irq_state);

/**
 * Get interrupt mode (enabled or disabled) for a given int_source.
 */
bool hal_nrf_get_irq_mode(hal_nrf_irq_source_t int_source);

/**
 * Read all interrupt flags.
 *
 * Use this function to get the interrupt flags. This function is similar to
 * hal_nrf_get_clear_irq_flags() with the exception that it does NOT clear the
 * irq_flags.
 *
 * @return Interrupt_flags.
 * @retval 0x10 Max Retransmit interrupt,
 * @retval 0x20 TX Data sent interrupt,
 * @retval 0x40 RX Data received interrupt.
 */
uint8_t hal_nrf_get_irq_flags(void);

/**
 * Read then clears all interrupt flags.
 *
 * Use this function to get the interrupt flags and clear them in the same
 * operation. Reduces radio interface activity and speed optimized.
 *
 * @return Interrupt_flags.
 * @retval 0x10 Max Retransmit interrupt,
 * @retval 0x20 TX Data sent interrupt,
 * @retval 0x40 RX Data received interrupt.
 */
uint8_t hal_nrf_get_clear_irq_flags(void);

uint8_t hal_nrf_clear_irq_flags_get_status(void);

/**
 * Clear one selected interrupt flag.
 *
 * Use this function to clear one specific interrupt flag. Other interrupt
 * flags are left unchanged.
 *
 * @param int_source Interrupt source of which flag to clear.
 */
void hal_nrf_clear_irq_flag(hal_nrf_irq_source_t int_source);

/**
 * Set radio's power mode.
 *
 * Use this function to power_up or power_down radio.
 *
 * @param pwr_mode POWER_UP or POWER_DOWN.
 */
void hal_nrf_set_power_mode(hal_nrf_pwr_mode_t pwr_mode);

/**
 * Get radio's operation mode.
 */
hal_nrf_pwr_mode_t hal_nrf_get_power_mode(void);

/*
 * Status functions prototypes
 */

/**
 * Get radio's TX FIFO status.
 *
 * Use this function to get the radio's TX FIFO status.
 *
 * @return TX FIFO status.
 * @retval 0x00 TX FIFO NOT empty, but NOT full,
 * @retval 0x01 FIFO empty,
 * @retval 0x02 FIFO full.
 */
uint8_t hal_nrf_get_tx_fifo_status(void);

/**
 * Check for TX FIFO empty.
 *
 * Use this function to check if TX FIFO is empty.
 *
 * @return TX FIFO empty bit,
 * @retval FALSE TX FIFO NOT empty,
 * @retval TRUE TX FIFO empty.
 *
 */
bool hal_nrf_tx_fifo_empty(void);

/**
 * Check for TX FIFO full.
 *
 * Use this function to check if TX FIFO is full.
 *
 * @return TX FIFO full bit.
 * @retval FALSE TX FIFO NOT full,
 * @retval TRUE TX FIFO full.
 *
 */
bool hal_nrf_tx_fifo_full(void);

/**
 * Get radio's RX FIFO status.
 *
 * Use this function to get the radio's TX FIFO status.
 *
 * @return RX FIFO status.
 * @retval 0x00 RX FIFO NOT empty, but NOT full,
 * @retval 0x01 RX FIFO empty,
 * @retval 0x02 RX FIFO full.
 *
 */
uint8_t hal_nrf_get_rx_fifo_status(void);

/**
 * Check for RX FIFO empty.
 *
 * Use this function to check if RX FIFO is empty. Reads STATUS register to
 * check this, not FIFO_STATUS
 *
 * @return RX FIFO empty bit.
 * @retval FALSE RX FIFO NOT empty,
 * @retval TRUE RX FIFO empty.
 *
 */
bool hal_nrf_rx_fifo_empty(void);

/**
 * Check for RX FIFO full.
 *
 * Use this function to check if RX FIFO is full.
 *
 * @return RX FIFO full bit.
 * @retval FALSE RX FIFO NOT full,
 * @retval TRUE RX FIFO full.
 */
bool hal_nrf_rx_fifo_full(void);

/**
 * Get FIFO_STATUS register.
 */
uint8_t hal_nrf_get_fifo_status(void);

/**
 * Get radio's transmit status register (OBSERVE_TX).
 *
 * @return Transmit status register.
 * @retval UpperNibble Packets lost counter.
 * @retval LowerNibble Retransmit attempts counter.
 */
uint8_t hal_nrf_get_auto_retr_status(void);

/**
 * Get radio's transmit attempts counter.
 *
 * @return Retransmit attempts counter.
 */
uint8_t hal_nrf_get_transmit_attempts(void);

/**
 * Get packets lost counter.
 *
 * @return Packets lost counter.
 */
uint8_t hal_nrf_get_packet_lost_ctr(void);

/**
 * Get the carrier detect flag.
 *
 * Use this function to get the carrier detect flag, used to detect stationary
 * disturbance on selected RF channel.
 *
 * @return Carrier Detect.
 * @retval FALSE Carrier NOT Detected,
 * @retval TRUE Carrier Detected.
 */
bool hal_nrf_get_carrier_detect(void);

/**
 * RPD used for nRF24l01+
 */
#define hal_nrf_get_rcv_pwr_detector() hal_nrf_get_carrier_detect()

/*
 * Data operation functions prototypes
 */

/**
 * Get RX data source.
 *
 * Use this function to read which RX pipe data was received on for current top
 * level FIFO data packet.
 *
 * @return pipe number of current packet present.
 */
uint8_t hal_nrf_get_rx_data_source(void);

/**
 * Reads the received payload width.
 *
 * @return Received payload width.
 */
uint8_t hal_nrf_read_rx_payload_width(void);

/**
 * Read RX payload.
 *
 * Use this function to read top level payload available in the RX FIFO.
 *
 * @param *rx_pload pointer to buffer in which RX payload are stored.
 * @return pipe number (MSB byte) and packet length (LSB byte).
 */
uint16_t hal_nrf_read_rx_payload(uint8_t *rx_pload);

/**
 * Write TX payload to radio.
 *
 * Use this function to write a packet of TX payload into the radio.
 * length is a number of bytes, which are stored in *tx_pload.
 *
 * @param *tx_pload pointer to buffer in which TX payload are present.
 * @param length number of bytes to write.
 */
void hal_nrf_write_tx_payload(const uint8_t *tx_pload, uint8_t length);

/**
 * Write TX payload which do not require ACK.
 *
 * When transmitting the ACK is not required nor sent from the receiver.
 * The payload will always be assumed as "sent". Use this function to write
 * a packet of TX payload into the radio. length is a number of bytes, which
 * are stored in *tx_pload.
 *
 * @param *tx_pload pointer to buffer in which TX payload are present.
 * @param length number of bytes to write.
 */
void hal_nrf_write_tx_payload_noack(const uint8_t *tx_pload, uint8_t length);

/**
 * Get status of reuse TX function.
 *
 * Use this function to check if reuse TX payload is activated.
 *
 * @return Reuse TX payload mode.
 * @retval FALSE Not activated,
 * @retval TRUE Activated.
 */
bool hal_nrf_get_reuse_tx_status(void);

/**
 * Reuse TX payload.
 *
 * Use this function to set that the radio is using the last transmitted payload
 * for the next packet as well.
 */
void hal_nrf_reuse_tx(void);

/**
 * Flush RX FIFO.
 *
 * Use this function to flush the radio's RX FIFO.
 */
void hal_nrf_flush_rx(void);

/**
 * Flush TX FIFO.
 *
 * Use this function to flush the radio's TX FIFO.
 */
void hal_nrf_flush_tx(void);

/**
 * No Operation command.
 *
 * Use this function to receive the radio's status register.
 *
 * @return Status register.
 */
uint8_t hal_nrf_nop(void);

#define hal_nrf_get_status() hal_nrf_nop()

/*
 * Test functions prototypes
 */

/**
 * Set radio's PLL mode.
 *
 * Use this function to either LOCK or UNLOCK the radio's PLL.
 *
 * @param pll_lock PLL locked, TRUE or FALSE.
 */
void hal_nrf_set_pll_mode(bool pll_lock);

/**
 * Get radio's PLL mode.
 */
bool hal_nrf_get_pll_mode(void);

/**
 * Enables continuous carrier transmit.
 *
 * Use this function to enable or disable continuous carrier transmission.
 *
 * @param enable Enable continuous carrier.
 */
void hal_nrf_enable_continious_wave(bool enable);

/**
 * Check if continuous carrier transmit is enabled.
 */
bool hal_nrf_is_continious_wave_enabled(void);

/*
 * Auxiliary functions prototypes
 */

/**
 * Save transceiver context.
 *
 * @param *p_ctx Pointer to a struct where the context will be written.
 */
void hal_nrf_save_ctx(hal_nrf_ctx_t *p_ctx);

/**
 * Read transmitter register content.
 *
 * @param reg Register to read.
 * @return Register contents.
 */
uint8_t hal_nrf_read_reg(uint8_t reg);

#ifdef __cplusplus
}
#endif

#endif /* __LR_DEVS_NRF_HAL_H__ */
