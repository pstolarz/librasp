/*
   Copyright (c) 2015,2016,2020 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* nRF24 RX example on the CHANNEL.
   nRF's polling via IRQ pin may be enabled by specifying GPIO_IRQ.
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "librasp/gpio.h"
#include "librasp/devices/nrf_hal.h"

#define CHANNEL     20

/* GPIO pin where the nRF IRQ pin is connected, undef for no IRQ polling */
/* #define GPIO_IRQ    22 */

#define SPI_CS      0
#define GPIO_CE     17

/* msecs */
#define RESTART_DELAY   5000

#define chip_enable()  gpio_set_value(&gpio_h, GPIO_CE, 1)
#define chip_disable() gpio_set_value(&gpio_h, GPIO_CE, 0)

#define NRF_EXEC(cmd) (cmd); if (errno==ECOMM) goto finish;
#define LR_EXEC(cmd) (cmd); if (cmd!=LREC_SUCCESS) goto finish;

static bool_t rx_finish = FALSE;

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

#ifdef GPIO_IRQ
    /* unmask RX irq and mask the remaining */
    NRF_EXEC(hal_nrf_set_irq_mode(HAL_NRF_RX_DR, true));
    hal_nrf_set_irq_mode(HAL_NRF_TX_DS, false);
    hal_nrf_set_irq_mode(HAL_NRF_MAX_RT, false);
#endif

    ret=TRUE;
finish:
    return ret;
}

int main(void)
{
    struct {
        unsigned int spi : 1;
        unsigned int gpio_h : 1;
        unsigned int pwr_up : 1;
        unsigned int sysfs_exp : 1;
    } init = {};

    spi_hndl_t spi_h;
    gpio_hndl_t gpio_h;

    int cnt;
    uint8_t irq_flg, rx[NRF_MAX_PL], addr[5];

    memset(rx, 0, sizeof(rx));

    LR_EXEC(gpio_init(&gpio_h, gpio_drv_io));
    init.gpio_h=TRUE;

    /* initialize GPIOs */
    gpio_direction_output(&gpio_h, GPIO_CE, 0);
#ifdef GPIO_IRQ
    gpio_set_driver(&gpio_h, gpio_drv_sysfs);
    LR_EXEC(gpio_sysfs_export(&gpio_h, GPIO_IRQ));
    init.sysfs_exp=TRUE;
    LR_EXEC(gpio_direction_input(&gpio_h, GPIO_IRQ));
    LR_EXEC(gpio_set_event(&gpio_h, GPIO_IRQ, GPIO_EVENT_BOTH));
    gpio_set_driver(&gpio_h, gpio_drv_io);
#endif

    LR_EXEC(spi_init( &spi_h, 0, SPI_CS, SPI_MODE_0,
        FALSE, 8, SPI_USE_DEF, SPI_USE_DEF, SPI_USE_DEF));
    init.spi=TRUE;

    signal(SIGINT, term_handler);
    signal(SIGTERM, term_handler);

    errno = 0;
    hal_nrf_set_spi_hndl(&spi_h);

    hal_nrf_get_address(HAL_NRF_PIPE0, addr);
    printf("P0 addr: %02x:%02x:%02x:%02x:%02x\n",
        addr[0], addr[1], addr[2], addr[3], addr[4]);

    printf("Tuned up, waiting for messages...\n");

restart:
    if (init.pwr_up) {
        chip_disable();
        hal_nrf_set_power_mode(HAL_NRF_PWR_DOWN);
        if (!errno) init.pwr_up=FALSE;
        usleep(1000);
    }
    hal_nrf_set_power_mode(HAL_NRF_PWR_UP);
    if (!errno) init.pwr_up=TRUE;
    usleep(1500);

    if (!init.pwr_up || !init_rx()) goto finish;

    chip_enable();
    usleep(150);

    /* RX loop */
    for (cnt=0; !rx_finish;)
    {
#ifdef GPIO_IRQ
        /* poll GPIO_IRQ */
        if (gpio_sysfs_poll(&gpio_h, GPIO_IRQ, RESTART_DELAY)==LREC_TIMEOUT)
            cnt+=RESTART_DELAY;
#endif
        /* check irq type */
        irq_flg = hal_nrf_get_clear_irq_flags();
        if (irq_flg & (1U<<HAL_NRF_RX_DR))
        {
            /* read RX FIFO of received messages */
            while (!hal_nrf_rx_fifo_empty())
            {
                hal_nrf_read_rx_payload(rx);
                if (rx[0]==0xAB) {
                    printf("Received: \"%s\"\n", &rx[1]);
                }
                cnt = 0;
            }
        }

        /* for unknown reason, from time to time RX seems to be locking in and
           stops to detect incoming traffic until restarting the transceiver is
           performed */
        if (cnt >= RESTART_DELAY) {
            cnt = 0;
            /* printf("No RX activity detected, restarting transceiver\n"); */
            goto restart;
        }

#ifndef GPIO_IRQ
        /* check irq flags every 1 msec for no polling case*/
        usleep(1000);
        cnt++;
#endif
    }

finish:
    if (errno==ECOMM) printf("SPI communication error\n");
    if (init.pwr_up) {
        chip_disable();
        hal_nrf_set_power_mode(HAL_NRF_PWR_DOWN);
    }
    if (init.spi) spi_free(&spi_h);
#ifdef GPIO_IRQ
    if (init.sysfs_exp) gpio_sysfs_unexport(&gpio_h, GPIO_IRQ);
#endif
    if (init.gpio_h) gpio_free(&gpio_h);

    return 0;
}
