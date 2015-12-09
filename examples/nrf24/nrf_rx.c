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

int main(void)
{
    bool_t spi_ini=FALSE, gpio_ini=FALSE;
    spi_hndl_t spi_h;
    gpio_hndl_t gpio_h;

    int lc, msgc;
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

restart:
    hal_nrf_set_power_mode(HAL_NRF_PWR_UP);
    usleep(1500);

    /* init part
     */
    hal_nrf_flush_rx();
    hal_nrf_flush_tx();

    /* clear all extra features for the purpose of this example */
    hal_nrf_enable_dynamic_payload(false);
    hal_nrf_enable_ack_payload(false);
    hal_nrf_enable_dynamic_ack(false);

    NRF_EXEC(hal_nrf_set_rf_channel(CHANNEL));
    NRF_EXEC(hal_nrf_set_datarate(HAL_NRF_1MBPS));
    NRF_EXEC(hal_nrf_set_crc_mode(HAL_NRF_CRC_16BIT));

    /* RX setup
     */
    NRF_EXEC(hal_nrf_set_operation_mode(HAL_NRF_PRX));

    hal_nrf_close_pipe(HAL_NRF_ALL);
    /* use default pipe 0 address */
    NRF_EXEC(hal_nrf_cconfig_rx_pipe(HAL_NRF_PIPE0, true, NULL, NRF_MAX_PL));

    hal_nrf_get_address(HAL_NRF_PIPE0, addr);
    printf("P0 addr: %02x:%02x:%02x:%02x:%02x\n",
        addr[0], addr[1], addr[2], addr[3], addr[4]);

    printf("Tuned up, waiting for messages...\n");

    chip_enable();
    usleep(150);

    /* waiting for messages
     */
    for (msgc=lc=0; !rx_finish; lc++)
    {
        irq_flg = hal_nrf_get_clear_irq_flags();
        if(irq_flg & (1U<<HAL_NRF_RX_DR))
        {
            /* read RX FIFO of received messages */
            while(!hal_nrf_rx_fifo_empty())
            {
                hal_nrf_read_rx_payload(rx);
                if (rx[0]==0xAB) {
                    msgc = lc;
                    printf("Received: \"%s\"\n", &rx[1]);
                }
                hal_nrf_flush_rx();
            }
        }

        /* for unknown reason, from time to time, RX seems to be locking in and
           stops to detect incoming traffic until restarting the transceiver */
        if (lc-msgc >= 5000) {
            msgc = lc;
            printf("No RX traffic detected, restarting the transceiver\n");
            chip_disable();
            goto restart;
        }

        usleep(1000);
    }

    chip_disable();

    hal_nrf_set_power_mode(HAL_NRF_PWR_DOWN);

finish:
    if (errno==ECOMM) printf("SPI communication error\n");
    if (spi_ini) spi_free(&spi_h);
    if (gpio_ini) gpio_free(&gpio_h);

    return 0;
}
