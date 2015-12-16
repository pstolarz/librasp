/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* nRF24 RX example on the CHANNEL.
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "librasp/gpio.h"
#include "librasp/devices/nrf_hal.h"

#define CHANNEL     20

#define SPI_CS      0
#define GPIO_CE     17

#define chip_enable()  gpio_set_value(&gpio_h, GPIO_CE, 1)
#define chip_disable() gpio_set_value(&gpio_h, GPIO_CE, 0)

#define NRF_EXEC(cmd) (cmd); if (errno==ECOMM) goto finish;

bool_t rx_finish = FALSE;

/* signal handler */
static void term_handler(int signal)
{
    switch (signal)
    {
    case SIGINT:
    case SIGTERM:
        rx_finish = TRUE;
        break;
    }
}

static bool_t init_rx()
{
    bool_t ret=FALSE;

    hal_nrf_flush_rx();
    hal_nrf_flush_tx();

    /* clear all extra features for the purpose of this example */
    hal_nrf_enable_dynamic_payload(false);
    hal_nrf_enable_ack_payload(false);
    hal_nrf_enable_dynamic_ack(false);

    NRF_EXEC(hal_nrf_set_rf_channel(CHANNEL));
    NRF_EXEC(hal_nrf_set_datarate(HAL_NRF_1MBPS));
    NRF_EXEC(hal_nrf_set_crc_mode(HAL_NRF_CRC_16BIT));

    /* RX setup: use default pipe 0 address */
    NRF_EXEC(hal_nrf_set_operation_mode(HAL_NRF_PRX));

    hal_nrf_close_pipe(HAL_NRF_ALL);
    NRF_EXEC(hal_nrf_config_rx_pipe(HAL_NRF_PIPE0, NULL, true, NRF_MAX_PL));

    /* TX output power for auto ack */
    hal_nrf_set_output_power(HAL_NRF_0DBM);

    ret=TRUE;
finish:
    return ret;
}

int main(void)
{
    bool_t spi_ini=FALSE, gpio_ini=FALSE, pwr_up=FALSE;
    spi_hndl_t spi_h;
    gpio_hndl_t gpio_h;

    int i;
    uint8_t irq_flg, rx[NRF_MAX_PL], addr[5];

    memset(rx, 0, sizeof(rx));

    if (gpio_init(&gpio_h, gpio_drv_io)!=LREC_SUCCESS) goto finish;
    else gpio_ini=TRUE;

    gpio_direction_output(&gpio_h, GPIO_CE, 0);

    if (spi_init(&spi_h, 0, SPI_CS, SPI_MODE_0, FALSE, 8,
        SPI_USE_DEF, SPI_USE_DEF, SPI_USE_DEF)!=LREC_SUCCESS) goto finish;
    else spi_ini=TRUE;

    signal(SIGINT, term_handler);
    signal(SIGTERM, term_handler);

    errno = 0;
    hal_nrf_set_spi_hndl(&spi_h);

    hal_nrf_get_address(HAL_NRF_PIPE0, addr);
    printf("P0 addr: %02x:%02x:%02x:%02x:%02x\n",
        addr[0], addr[1], addr[2], addr[3], addr[4]);

    printf("Tuned up, waiting for messages...\n");

restart:
    if (pwr_up) {
        chip_disable();
        hal_nrf_set_power_mode(HAL_NRF_PWR_DOWN);
        if (!errno) pwr_up=FALSE;
        usleep(1000);
    }
    hal_nrf_set_power_mode(HAL_NRF_PWR_UP);
    if (!errno) pwr_up=TRUE;
    usleep(1500);

    if (!pwr_up || !init_rx()) goto finish;

    chip_enable();
    usleep(150);

    /* RX loop */
    for (i=0; !rx_finish; i++)
    {
        irq_flg = hal_nrf_get_clear_irq_flags();
        if(irq_flg & (1U<<HAL_NRF_RX_DR))
        {
            /* read RX FIFO of received messages */
            while(!hal_nrf_rx_fifo_empty())
            {
                hal_nrf_read_rx_payload(rx);
                if (rx[0]==0xAB) {
                    printf("Received: \"%s\"\n", &rx[1]);
                }
                hal_nrf_flush_rx();
                i = 0;
            }
        }

        /* for unknown reason, from time to time, RX seems to be locking in and
           stops to detect incoming traffic until restarting the transceiver */
        if (i >= 5000) {
            i = 0;
            goto restart;
        }

        usleep(1000);
    }

finish:
    if (errno==ECOMM) printf("SPI communication error\n");
    if (pwr_up) {
        chip_disable();
        hal_nrf_set_power_mode(HAL_NRF_PWR_DOWN);
    }
    if (spi_ini) spi_free(&spi_h);
    if (gpio_ini) gpio_free(&gpio_h);

    return 0;
}
