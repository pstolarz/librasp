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

#include <errno.h>
#include <string.h>
#include "librasp/devices/nrf_hal.h"

#define BIT(n) (1U<<(n))
#define SET_BIT(v, n) ((v)|BIT(n))
#define CLR_BIT(v, n) ((v)&~BIT(n))

static spi_hndl_t spi_hndl = {-1};

#define SET_SPI_ERR(cmd) \
    (errno = ((spi_hndl.fd!=-1) && (cmd)==LREC_SUCCESS ? 0 : ECOMM))

#define CHK_SPI_ERR() if (errno==ECOMM) goto finish;

static uint8_t hal_nrf_write_reg(uint8_t reg, uint8_t value);
static uint16_t hal_nrf_read_multibyte_reg(uint8_t reg, uint8_t *pbuf);
static void hal_nrf_write_multibyte_reg(
    uint8_t reg, const uint8_t *pbuf, uint8_t length);

/* Exported functions go below
 */

bool hal_nrf_set_spi_hndl(spi_hndl_t *p_hndl)
{
    if (p_hndl && p_hndl->fd!=-1 && p_hndl->mode==SPI_MODE_0 &&
        !p_hndl->lsb_first && p_hndl->bits_per_word==8)
    {
        /* deep copy of the handle */
        spi_hndl = *p_hndl;
        return true;
    } else
        return false;
}

void hal_nrf_set_operation_mode(hal_nrf_operation_mode_t op_mode)
{
    uint8_t config = hal_nrf_read_reg(CONFIG);
    CHK_SPI_ERR();

    if(op_mode == HAL_NRF_PRX) {
        config = (uint8_t)SET_BIT(config, PRIM_RX);
    } else {
        config = (uint8_t)CLR_BIT(config, PRIM_RX);
    }
    hal_nrf_write_reg(CONFIG, config);

finish:
    return;
}

hal_nrf_operation_mode_t hal_nrf_get_operation_mode(void)
{
    uint8_t config = hal_nrf_read_reg(CONFIG);
    return ((config & (uint8_t)BIT(PRIM_RX)) ? HAL_NRF_PRX : HAL_NRF_PTX);
}

void hal_nrf_set_power_mode(hal_nrf_pwr_mode_t pwr_mode)
{
    uint8_t config = hal_nrf_read_reg(CONFIG);
    CHK_SPI_ERR();

    if(pwr_mode == HAL_NRF_PWR_UP) {
        config = (uint8_t)SET_BIT(config, PWR_UP);
    } else {
        config = (uint8_t)CLR_BIT(config, PWR_UP);
    }
    hal_nrf_write_reg(CONFIG, config);

finish:
    return;
}

hal_nrf_pwr_mode_t hal_nrf_get_power_mode(void)
{
    uint8_t config = hal_nrf_read_reg(CONFIG);
    return ((config & (uint8_t)BIT(PWR_UP)) ? HAL_NRF_PWR_UP : HAL_NRF_PWR_DOWN);
}

void hal_nrf_set_crc_mode(hal_nrf_crc_mode_t crc_mode)
{
    uint8_t config = hal_nrf_read_reg(CONFIG);
    CHK_SPI_ERR();

    switch (crc_mode)
    {
    case HAL_NRF_CRC_OFF:
        config = (uint8_t)CLR_BIT(config, EN_CRC);
        break;
    case HAL_NRF_CRC_8BIT:
        config = (uint8_t)SET_BIT(config, EN_CRC);
        config = (uint8_t)CLR_BIT(config, CRCO);
        break;
    case HAL_NRF_CRC_16BIT:
        config = (uint8_t)SET_BIT(config, EN_CRC);
        config = (uint8_t)SET_BIT(config, CRCO);
        break;
    default:
        break;
    }
    hal_nrf_write_reg(CONFIG, config);

finish:
    return;
}

hal_nrf_crc_mode_t hal_nrf_get_crc_mode(void)
{
    uint8_t config = hal_nrf_read_reg(CONFIG);

    if ((config & (uint8_t)BIT(EN_CRC))) {
        return ((config & (uint8_t)BIT(CRCO)) ?
            HAL_NRF_CRC_16BIT : HAL_NRF_CRC_8BIT);
    } else
        return HAL_NRF_CRC_OFF;
}

void hal_nrf_set_irq_mode(hal_nrf_irq_source_t int_source, bool irq_state)
{
    uint8_t config = hal_nrf_read_reg(CONFIG);
    CHK_SPI_ERR();

    switch (int_source)
    {
    case HAL_NRF_MAX_RT:
        if (irq_state) {
            config = (uint8_t)CLR_BIT(config, MASK_MAX_RT);
        } else {
            config = (uint8_t)SET_BIT(config, MASK_MAX_RT);
        }
        break;
    case HAL_NRF_TX_DS:
        if (irq_state) {
            config = (uint8_t)CLR_BIT(config, MASK_TX_DS);
        } else {
            config = (uint8_t)SET_BIT(config, MASK_TX_DS);
        }
        break;
    case HAL_NRF_RX_DR:
        if (irq_state) {
            config = (uint8_t)CLR_BIT(config, MASK_RX_DR);
        } else {
            config = (uint8_t)SET_BIT(config, MASK_RX_DR);
        }
        break;
    }
    hal_nrf_write_reg(CONFIG, config);

finish:
    return;
}

bool hal_nrf_get_irq_mode(hal_nrf_irq_source_t int_source)
{
    bool retval = false;
    uint8_t config = hal_nrf_read_reg(CONFIG);

    switch (int_source)
    {
    case HAL_NRF_MAX_RT:
        retval = !(config & (uint8_t)BIT(MASK_MAX_RT));
    case HAL_NRF_TX_DS:
        retval = !(config & (uint8_t)BIT(MASK_TX_DS));
    case HAL_NRF_RX_DR:
        retval = !(config & (uint8_t)BIT(MASK_RX_DR));
    }
    return retval;
}

uint8_t hal_nrf_get_clear_irq_flags(void)
{
    return hal_nrf_write_reg(STATUS,
        (uint8_t)(BIT(MAX_RT)|BIT(TX_DS)|BIT(RX_DR))) &
        (uint8_t)(BIT(MAX_RT)|BIT(TX_DS)|BIT(RX_DR));
}

uint8_t hal_nrf_clear_irq_flags_get_status(void)
{
    /* When RFIRQ is cleared (when calling write_reg), pipe information
      is unreliable (read again with read_reg) */
    uint8_t retval = hal_nrf_write_reg(STATUS,
        (uint8_t)(BIT(MAX_RT)|BIT(TX_DS)|BIT(RX_DR))) &
        (uint8_t)(BIT(MAX_RT)|BIT(TX_DS)|BIT(RX_DR));
    CHK_SPI_ERR();

    retval |= hal_nrf_read_reg(STATUS) &
        (uint8_t)~(BIT(MAX_RT)|BIT(TX_DS)|BIT(RX_DR));
finish:
    return retval;
}

void hal_nrf_clear_irq_flag(hal_nrf_irq_source_t int_source)
{
    hal_nrf_write_reg(STATUS, (uint8_t)BIT(int_source));
}

uint8_t hal_nrf_get_irq_flags(void)
{
    return hal_nrf_get_status() & (uint8_t)(BIT(MAX_RT)|BIT(TX_DS)|BIT(RX_DR));
}

void hal_nrf_open_pipe(hal_nrf_address_t pipe_num, bool auto_ack)
{
    uint8_t en_rxaddr, en_aa;

    en_rxaddr = hal_nrf_read_reg(EN_RXADDR);
    CHK_SPI_ERR();
    en_aa = hal_nrf_read_reg(EN_AA);
    CHK_SPI_ERR();

    switch(pipe_num)
    {
    case HAL_NRF_PIPE0:
    case HAL_NRF_PIPE1:
    case HAL_NRF_PIPE2:
    case HAL_NRF_PIPE3:
    case HAL_NRF_PIPE4:
    case HAL_NRF_PIPE5:
        en_rxaddr = (uint8_t)SET_BIT(en_rxaddr, (int)pipe_num);

        if(auto_ack) {
            en_aa = (uint8_t)SET_BIT(en_aa, (int)pipe_num);
        }
        else {
            en_aa = (uint8_t)CLR_BIT(en_aa, (int)pipe_num);
        }
        break;

    case HAL_NRF_ALL:
        en_rxaddr = (uint8_t)(~(BIT(6)|BIT(7)));

        if(auto_ack) {
            en_aa = (uint8_t)(~(BIT(6)|BIT(7)));
        }
        else {
            en_aa = 0;
        }
        break;

    case HAL_NRF_TX:
    default:
        goto finish;
    }

    hal_nrf_write_reg(EN_RXADDR, en_rxaddr);
    CHK_SPI_ERR();
    hal_nrf_write_reg(EN_AA, en_aa);
finish:
    return;
}

void hal_nrf_close_pipe(hal_nrf_address_t pipe_num)
{
    uint8_t en_rxaddr, en_aa;

    en_rxaddr = hal_nrf_read_reg(EN_RXADDR);
    CHK_SPI_ERR();
    en_aa = hal_nrf_read_reg(EN_AA);
    CHK_SPI_ERR();

    switch(pipe_num)
    {
    case HAL_NRF_PIPE0:
    case HAL_NRF_PIPE1:
    case HAL_NRF_PIPE2:
    case HAL_NRF_PIPE3:
    case HAL_NRF_PIPE4:
    case HAL_NRF_PIPE5:
        en_rxaddr = (uint8_t)CLR_BIT(en_rxaddr, (int)pipe_num);
        en_aa = (uint8_t)CLR_BIT(en_aa, (int)pipe_num);
        break;

    case HAL_NRF_ALL:
        en_rxaddr = 0;
        en_aa = 0;
        break;

    case HAL_NRF_TX:
    default:
        goto finish;
    }

    hal_nrf_write_reg(EN_RXADDR, en_rxaddr);
    CHK_SPI_ERR();
    hal_nrf_write_reg(EN_AA, en_aa);
finish:
    return;
}

void hal_nrf_config_rx_pipe(hal_nrf_address_t pipe_num,
    const uint8_t *addr, bool auto_ack, uint8_t pload_width)
{
    hal_nrf_open_pipe(pipe_num, auto_ack);
    CHK_SPI_ERR();

    if (addr) {
        hal_nrf_set_address(pipe_num, addr);
        CHK_SPI_ERR();
    }

    hal_nrf_set_rx_payload_width((uint8_t)pipe_num, pload_width);
finish:
    return;
}

void hal_nrf_config_tx(const uint8_t *addr,
    hal_nrf_output_power_t power, uint8_t retr, uint16_t delay)
{
    hal_nrf_set_output_power(power);
    CHK_SPI_ERR();

    hal_nrf_set_auto_retr(retr, delay);
    CHK_SPI_ERR();

    if (addr) {
        hal_nrf_set_address(HAL_NRF_TX, addr);
        CHK_SPI_ERR();
        hal_nrf_set_address(HAL_NRF_PIPE0, addr);
        CHK_SPI_ERR();
    }

    /* open RX pipe 0 for receiving ack */
    hal_nrf_open_pipe(HAL_NRF_PIPE0, true);
finish:
    return;
}

uint8_t hal_nrf_get_pipe_status(uint8_t pipe_num)
{
    uint8_t en_rxaddr, en_aa;
    uint8_t en_rx_r=0, en_aa_r=0;

    en_rxaddr = hal_nrf_read_reg(EN_RXADDR);
    CHK_SPI_ERR();
    en_aa = hal_nrf_read_reg(EN_AA);

    if (pipe_num<=5) {
        en_rx_r = (en_rxaddr & (uint8_t)BIT((int)pipe_num)) !=0;
        en_aa_r = (en_aa & (uint8_t)BIT((int)pipe_num)) !=0;
    }
finish:
    return (uint8_t)(en_aa_r << 1) + en_rx_r;
}

void hal_nrf_set_address(const hal_nrf_address_t address, const uint8_t *addr)
{
    switch(address)
    {
    case HAL_NRF_TX:
    case HAL_NRF_PIPE0:
    case HAL_NRF_PIPE1:
        hal_nrf_write_multibyte_reg(W_REGISTER+RX_ADDR_P0+
            (uint8_t)address, addr, hal_nrf_get_address_width());
      break;
    case HAL_NRF_PIPE2:
    case HAL_NRF_PIPE3:
    case HAL_NRF_PIPE4:
    case HAL_NRF_PIPE5:
        hal_nrf_write_reg(RX_ADDR_P0 + (uint8_t)address, *addr);
        break;

    case HAL_NRF_ALL:
    default:
        break;
    }
}

uint8_t hal_nrf_get_address(uint8_t address, uint8_t *addr)
{
    switch (address)
    {
    case HAL_NRF_PIPE0:
    case HAL_NRF_PIPE1:
    case HAL_NRF_TX:
        return (uint8_t)hal_nrf_read_multibyte_reg(address, addr);
    default:
        *addr = hal_nrf_read_reg(RX_ADDR_P0 + address);
        return 1;
    }
}

void hal_nrf_set_auto_retr(uint8_t retr, uint16_t delay)
{
    uint8_t setup_retr =
        (uint8_t)((((delay>>8) & 0x0f) << 4) | (retr & 0x0f));
    hal_nrf_write_reg(SETUP_RETR, setup_retr);
}

uint8_t hal_nrf_get_auto_retr_ctr(void)
{
    return (hal_nrf_read_reg(SETUP_RETR) & 0x0f);
}

uint16_t hal_nrf_get_auto_retr_delay(void)
{
    return (uint16_t)((((hal_nrf_read_reg(SETUP_RETR)>>4) & 0x0f)+1)*250);
}

uint8_t hal_nrf_get_auto_retr_status(void)
{
    return hal_nrf_read_reg(OBSERVE_TX);
}

uint8_t hal_nrf_get_packet_lost_ctr(void)
{
    return ((hal_nrf_read_reg(OBSERVE_TX) &
        (uint8_t)(BIT(7)|BIT(6)|BIT(5)|BIT(4))) >> 4);
}

uint8_t hal_nrf_get_transmit_attempts(void)
{
    return (hal_nrf_read_reg(OBSERVE_TX) &
        (uint8_t)(BIT(3)|BIT(2)|BIT(1)|BIT(0)));
}

void hal_nrf_set_address_width(hal_nrf_address_width_t address_width)
{
    uint8_t setup_aw = (uint8_t)(((int)address_width-2) & 0x03);
    hal_nrf_write_reg(SETUP_AW, setup_aw);
}

uint8_t hal_nrf_get_address_width(void)
{
    return (uint8_t)(hal_nrf_read_reg(SETUP_AW)+2);
}

void hal_nrf_set_rx_payload_width(uint8_t pipe_num, uint8_t pload_width)
{
    hal_nrf_write_reg(RX_PW_P0+pipe_num, pload_width);
}

uint8_t hal_nrf_get_rx_payload_width(uint8_t pipe_num)
{
    return hal_nrf_read_reg(RX_PW_P0+pipe_num);
}

void hal_nrf_set_rf_channel(uint8_t channel)
{
    uint8_t rf_ch = (uint8_t)(channel & 0x7f);
    hal_nrf_write_reg(RF_CH, rf_ch);
}

uint8_t hal_nrf_get_rf_channel(void)
{
    return (hal_nrf_read_reg(RF_CH) & 0x7f);
}

void hal_nrf_set_output_power(hal_nrf_output_power_t power)
{
    uint8_t rf_setup = hal_nrf_read_reg(RF_SETUP);

    rf_setup &= (uint8_t)(~(uint8_t)(BIT(RF_PWR0)|BIT(RF_PWR1)));
    rf_setup |= (uint8_t)(((int)power & 0x03)<<RF_PWR0);
    hal_nrf_write_reg(RF_SETUP, rf_setup);
}

hal_nrf_output_power_t hal_nrf_get_output_power(void)
{
    return (hal_nrf_output_power_t)
        ((hal_nrf_read_reg(RF_SETUP)>>RF_PWR0) & 0x03);
}

void hal_nrf_set_datarate(hal_nrf_datarate_t datarate)
{
    uint8_t rf_setup = hal_nrf_read_reg(RF_SETUP);
    CHK_SPI_ERR();

    switch (datarate)
    {
    case HAL_NRF_250KBPS:
        rf_setup = (uint8_t)SET_BIT(rf_setup, RF_DR_LOW);
        rf_setup = (uint8_t)CLR_BIT(rf_setup, RF_DR_HIGH);
        break;
    case HAL_NRF_1MBPS:
        rf_setup = (uint8_t)CLR_BIT(rf_setup, RF_DR_LOW);
        rf_setup = (uint8_t)CLR_BIT(rf_setup, RF_DR_HIGH);
        break;
    case HAL_NRF_2MBPS:
    default:
        rf_setup = (uint8_t)CLR_BIT(rf_setup, RF_DR_LOW);
        rf_setup = (uint8_t)SET_BIT(rf_setup, RF_DR_HIGH);
        break;
    }
    hal_nrf_write_reg(RF_SETUP, rf_setup);
finish:
    return;
}

hal_nrf_datarate_t hal_nrf_get_datarate(void)
{
    uint8_t rf_setup = hal_nrf_read_reg(RF_SETUP);
    return (hal_nrf_datarate_t)
        (((rf_setup & (uint8_t)BIT(RF_DR_LOW))<<1) |
         (rf_setup & (uint8_t)BIT(RF_DR_HIGH)));
}

bool hal_nrf_rx_fifo_empty(void)
{
    return (bool)((hal_nrf_read_reg(FIFO_STATUS) >> RX_EMPTY) & 0x01);
}

bool hal_nrf_rx_fifo_full(void)
{
    return (bool)((hal_nrf_read_reg(FIFO_STATUS) >> RX_FULL) & 0x01);
}

bool hal_nrf_tx_fifo_empty(void)
{
    return (bool)((hal_nrf_read_reg(FIFO_STATUS) >> TX_EMPTY) & 0x01);
}

bool hal_nrf_tx_fifo_full(void)
{
    return (bool)((hal_nrf_read_reg(FIFO_STATUS) >> TX_FIFO_FULL) & 0x01);
}

uint8_t hal_nrf_get_rx_fifo_status(void)
{
    return (hal_nrf_read_reg(FIFO_STATUS) &
        (uint8_t)(BIT(RX_FULL)|BIT(RX_EMPTY)));
}

uint8_t hal_nrf_get_tx_fifo_status(void)
{
    return ((hal_nrf_read_reg(FIFO_STATUS) &
        (uint8_t)(BIT(TX_FIFO_FULL)|BIT(TX_EMPTY))) >> 4);
}

bool hal_nrf_get_reuse_tx_status(void)
{
    return (bool)((hal_nrf_read_reg(FIFO_STATUS) &
        (uint8_t)BIT(TX_REUSE)) >> TX_REUSE);
}

uint8_t hal_nrf_get_fifo_status(void)
{
    return hal_nrf_read_reg(FIFO_STATUS);
}

bool hal_nrf_get_carrier_detect(void)
{
    return (bool)(hal_nrf_read_reg(CD) & 0x01);
}

void hal_nrf_activate_features(void) {}

void hal_nrf_setup_dynamic_payload(uint8_t setup)
{
    uint8_t dynpd = setup & (uint8_t)(~(BIT(6)|BIT(7)));
    hal_nrf_write_reg(DYNPD, dynpd);
}

void hal_nrf_enable_dynamic_payload(bool enable)
{
    uint8_t feature = hal_nrf_read_reg(FEATURE);
    CHK_SPI_ERR();

    if (enable) {
        feature = (uint8_t)SET_BIT(feature, EN_DPL);
    } else {
        feature = (uint8_t)CLR_BIT(feature, EN_DPL);
    }
    hal_nrf_write_reg(FEATURE, feature);
finish:
    return;
}

bool hal_nrf_is_dynamic_payload_enabled(void)
{
    return ((hal_nrf_read_reg(FEATURE) & (uint8_t)BIT(EN_DPL)) != 0);
}

void hal_nrf_enable_ack_payload(bool enable)
{
    uint8_t feature = hal_nrf_read_reg(FEATURE);
    CHK_SPI_ERR();

    if (enable) {
        feature = (uint8_t)SET_BIT(feature, EN_ACK_PAY);
    } else {
        feature = (uint8_t)CLR_BIT(feature, EN_ACK_PAY);
    }
    hal_nrf_write_reg(FEATURE, feature);
finish:
    return;
}

bool hal_nrf_is_ack_payload_enabled(void)
{
    return ((hal_nrf_read_reg(FEATURE) & (uint8_t)BIT(EN_ACK_PAY)) != 0);
}

void hal_nrf_enable_dynamic_ack(bool enable)
{
    uint8_t feature = hal_nrf_read_reg(FEATURE);
    CHK_SPI_ERR();

    if (enable) {
        feature = (uint8_t)SET_BIT(feature, EN_DYN_ACK);
    } else {
        feature = (uint8_t)CLR_BIT(feature, EN_DYN_ACK);
    }
    hal_nrf_write_reg(FEATURE, feature);
finish:
    return;
}

bool hal_nrf_is_dynamic_ack_enabled(void)
{
    return ((hal_nrf_read_reg(FEATURE) & (uint8_t)BIT(EN_DYN_ACK)) != 0);
}

void hal_nrf_write_tx_payload(const uint8_t *tx_pload, uint8_t length)
{
    hal_nrf_write_multibyte_reg(W_TX_PAYLOAD, tx_pload, length);
}

void hal_nrf_write_tx_payload_noack(const uint8_t *tx_pload, uint8_t length)
{
    hal_nrf_write_multibyte_reg(W_TX_PAYLOAD_NOACK, tx_pload, length);
}

void hal_nrf_write_ack_payload(
    uint8_t pipe, const uint8_t *tx_pload, uint8_t length)
{
    hal_nrf_write_multibyte_reg(W_ACK_PAYLOAD | pipe, tx_pload, length);
}

uint8_t hal_nrf_read_rx_payload_width(void)
{
    return hal_nrf_read_reg(R_RX_PL_WID);
}

uint16_t hal_nrf_read_rx_payload(uint8_t *rx_pload)
{
    return hal_nrf_read_multibyte_reg((uint8_t)HAL_NRF_RX_PLOAD, rx_pload);
}

uint8_t hal_nrf_get_rx_data_source(void)
{
    /* read STATUS.RX_P_NO content */
    return ((hal_nrf_get_status() & (uint8_t)(BIT(3)|BIT(2)|BIT(1))) >> 1);
}

void hal_nrf_reuse_tx(void)
{
    uint8_t tx = REUSE_TX_PL;
    SET_SPI_ERR(spi_transmit(&spi_hndl, &tx, NULL, 1));
}

void hal_nrf_flush_rx(void)
{
    uint8_t tx = FLUSH_RX;
    SET_SPI_ERR(spi_transmit(&spi_hndl, &tx, NULL, 1));
}

void hal_nrf_flush_tx(void)
{
    uint8_t tx = FLUSH_TX;
    SET_SPI_ERR(spi_transmit(&spi_hndl, &tx, NULL, 1));
}

uint8_t hal_nrf_nop(void)
{
    uint8_t tx=NOP, rx;
    SET_SPI_ERR(spi_transmit(&spi_hndl, &tx, &rx, 1));
    return rx;
}

void hal_nrf_set_pll_mode(bool pll_lock)
{
    uint8_t rf_setup = hal_nrf_read_reg(RF_SETUP);
    CHK_SPI_ERR();

    if (pll_lock) {
        rf_setup = (uint8_t)SET_BIT(rf_setup, PLL_LOCK);
    } else {
        rf_setup = (uint8_t)CLR_BIT(rf_setup, PLL_LOCK);
    }
    hal_nrf_write_reg(RF_SETUP, rf_setup);
finish:
    return;
}

bool hal_nrf_get_pll_mode(void)
{
    return ((hal_nrf_read_reg(RF_SETUP) & (uint8_t)BIT(PLL_LOCK)) != 0);
}

void hal_nrf_enable_continious_wave(bool enable)
{
    uint8_t rf_setup = hal_nrf_read_reg(RF_SETUP);
    CHK_SPI_ERR();

    if (enable) {
        rf_setup = (uint8_t)SET_BIT(rf_setup, CONT_WAVE);
    } else {
        rf_setup = (uint8_t)CLR_BIT(rf_setup, CONT_WAVE);
    }
    hal_nrf_write_reg(RF_SETUP, rf_setup);
finish:
    return;
}

bool hal_nrf_is_continious_wave_enabled(void)
{
    return ((hal_nrf_read_reg(RF_SETUP) & (uint8_t)BIT(CONT_WAVE)) != 0);
}

uint8_t hal_nrf_read_reg(uint8_t reg)
{
    uint8_t tx[2], rx[2];

    tx[0] = reg;
    tx[1] = 0;
    SET_SPI_ERR(spi_transmit(&spi_hndl, tx, rx, 2));
    return rx[1];
}

/* Static functions go below
 */

/**
 * Basis function write_reg.
 * Use this function to write a new value to a radio register.
 * @param reg Register to write
 * @param value New value to write
 * @return Status register
 */
static uint8_t hal_nrf_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t tx[2], rx[2];

    tx[0] = W_REGISTER+reg;
    tx[1] = value;
    SET_SPI_ERR(spi_transmit(&spi_hndl, tx, rx, 2));
    return rx[0];
}

/**
 * Basis function, read_multibyte register.
 * Use this function to read multiple bytes from a multibyte radio-register.
 * @param reg Multibyte register to read from
 * @param *pbuf Pointer to buffer in which to store read bytes to
 * @return pipe# of received data (MSB), if operation used by
 * hal_nrf_read_rx_pload
 * @return length of read data (LSB), either for hal_nrf_read_rx_pload or
 * for hal_nrf_get_address.
 */
static uint16_t hal_nrf_read_multibyte_reg(uint8_t reg, uint8_t *pbuf)
{
    uint8_t length;
    uint8_t tx[NRF_MAX_PL+1], rx[NRF_MAX_PL+1];

    switch(reg)
    {
    case HAL_NRF_PIPE0:
    case HAL_NRF_PIPE1:
    case HAL_NRF_TX:
        length = hal_nrf_get_address_width();
        CHK_SPI_ERR();
        tx[0] = RX_ADDR_P0+reg;
        break;

    case HAL_NRF_RX_PLOAD:
        reg = hal_nrf_get_rx_data_source();
        CHK_SPI_ERR();

        if (reg < 7) {
            length = hal_nrf_read_rx_payload_width();
            CHK_SPI_ERR();
            tx[0] = R_RX_PAYLOAD;
        }
        else length = 0;
        break;

    default:
        length = 0;
        break;
    }

    if (length > 0) {
        memset(&tx[1], 0, length);
        SET_SPI_ERR(spi_transmit(&spi_hndl, tx, rx, length+1));
        memcpy(pbuf, &rx[1], length);
    }
finish:
    return (((uint16_t)reg << 8) | length);
}

/**
 * Basis function, write_multibyte register.
 * Use this function to write multiple bytes to a multiple radio register.
 * @param reg Register to write
 * @param *pbuf pointer to buffer in which data to write is
 * @param length # of bytes to write
 */
static void hal_nrf_write_multibyte_reg(
    uint8_t reg, const uint8_t *pbuf, uint8_t length)
{
    uint8_t tx[NRF_MAX_PL+1];

    tx[0] = reg;
    memcpy(&tx[1], pbuf, length);
    SET_SPI_ERR(spi_transmit(&spi_hndl, tx, NULL, length+1));
}
