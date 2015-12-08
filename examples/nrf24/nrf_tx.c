/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* nRF24 TX example on the CHANNEL.
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "librasp/gpio.h"
#include "librasp/devices/nrf_hal.h"

#define CHANNEL     20

#define SPI_CS      1
#define GPIO_CE     27

#define chip_enable()  gpio_set_value(&gpio_h, GPIO_CE, 1)
#define chip_disable() gpio_set_value(&gpio_h, GPIO_CE, 0)

#define NRF_EXEC(cmd) (cmd); if (errno==ECOMM) goto finish;

bool_t tx_finish = FALSE;

/* signal handler */
static void term_handler(int signal)
{
    switch (signal)
    {
    case SIGINT:
    case SIGTERM:
        tx_finish = TRUE;
        break;
    }
}

int main(void)
{
    bool_t spi_ini=FALSE, gpio_ini=FALSE;
    spi_hndl_t spi_h;
    gpio_hndl_t gpio_h;

    int cnt, i;
    uint8_t irq_flg, tx[NRF_MAX_PL], addr[5];

    memset(tx, 0, sizeof(tx));

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

    /* TX setup (use default TX address)
     */
    NRF_EXEC(hal_nrf_set_operation_mode(HAL_NRF_PTX));

    /* max power with no retransmissions */
    hal_nrf_set_output_power(HAL_NRF_0DBM);
    hal_nrf_set_auto_retr(0, 0);

    hal_nrf_get_address(HAL_NRF_PIPE0, addr);
    printf("P0 addr: %02x:%02x:%02x:%02x:%02x\n",
        addr[0], addr[1], addr[2], addr[3], addr[4]);

    hal_nrf_get_address(HAL_NRF_TX, addr);
    printf("TX addr: %02x:%02x:%02x:%02x:%02x\n",
        addr[0], addr[1], addr[2], addr[3], addr[4]);

    /* transition loop
     */
    for (cnt=0 ;!tx_finish; cnt++)
    {
        tx[0] = 0xAB;   /* magic */
        sprintf((char *)&tx[1], "msg no. %d", cnt);
        hal_nrf_write_tx_payload(tx, sizeof(tx));

        /* pulse CE for transmission */
        chip_enable();
        usleep(100);
        chip_disable();

        /* wait 2ms for transmission status */
        hal_nrf_get_clear_irq_flags();
        for (i=0; i<20; i++)
        {
            irq_flg = hal_nrf_get_clear_irq_flags();
            if (irq_flg & ((1U<<HAL_NRF_TX_DS)|(1U<<HAL_NRF_MAX_RT))) break;
            usleep(100);
        }

        if ((irq_flg & (1U<<HAL_NRF_MAX_RT)) || i>=20) {
            hal_nrf_flush_tx();
            printf("TX timeout\n");
        } else {
            printf("Sent message no. %d\n", cnt);
        }

        sleep(1);
    }

    hal_nrf_set_power_mode(HAL_NRF_PWR_DOWN);

finish:
    if (errno==ECOMM) printf("SPI communication error\n");
    if (spi_ini) spi_free(&spi_h);
    if (gpio_ini) gpio_free(&gpio_h);

    return 0;
}
