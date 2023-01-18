/*
   Copyright (c) 2015,2016,2020,2023 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* Transmit a carrier on the CHANNEL. The transmission may be observed via
   'nrf_scan' example.
 */

#include <assert.h>
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

static bool_t trans_finish = FALSE;

/* signal handler */
static void term_handler(int signal)
{
    switch (signal)
    {
    case SIGINT:
    case SIGTERM:
        trans_finish = TRUE;
        break;
    }
}

int main(void)
{
    bool_t spi_ini=FALSE, gpio_ini=FALSE;

    spi_hndl_t spi_h;
    gpio_hndl_t gpio_h;

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

    NRF_EXEC(hal_nrf_set_operation_mode(HAL_NRF_PTX));

    NRF_EXEC(hal_nrf_enable_continuous_wave(true));
    NRF_EXEC(hal_nrf_set_pll_mode(true));
    NRF_EXEC(hal_nrf_set_output_power(HAL_NRF_0DBM));
    NRF_EXEC(hal_nrf_set_rf_channel(CHANNEL));

    assert(hal_nrf_is_continuous_wave_enabled() &&
        hal_nrf_get_pll_mode() &&
        (hal_nrf_get_output_power()==HAL_NRF_0DBM) &&
        (hal_nrf_get_rf_channel()==CHANNEL));

    printf("Transmitting RF carrier on channel %d...\n", CHANNEL);

    chip_enable();
    while (!trans_finish) usleep(100000);
    chip_disable();

    printf("RF carrier transmission stopped!\n");

    hal_nrf_enable_continuous_wave(false);
    hal_nrf_set_pll_mode(false);

finish:
    if (errno==ECOMM) printf("SPI communication error\n");
    if (spi_ini) spi_free(&spi_h);
    if (gpio_ini) gpio_free(&gpio_h);

    return 0;
}
