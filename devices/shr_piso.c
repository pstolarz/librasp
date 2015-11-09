/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include "common.h"
#include "librasp/devices/shr_piso.h"

/* shift register cycle time (expressed in main processor's cycles) */
#define SHR_CYCLE   100

/* exported; see header for details */
lr_errc_t shr_piso_read(gpio_hndl_t *p_gpio_h, unsigned int sh_ld_gpio,
    unsigned int clk_gpio, unsigned int data_gpio, unsigned int n_dta_bits,
    uint8_t *p_dta)
{
    lr_errc_t ret=LREC_SUCCESS;
    unsigned int i=0, bt;
    bool_t clk_low=FALSE;

    if (!n_dta_bits) goto finish;

    /* load the register with parallel input */
    EXEC_RG(gpio_set_value(p_gpio_h, sh_ld_gpio, 0));
    WAIT_CYCLES(SHR_CYCLE);
    EXEC_RG(gpio_set_value(p_gpio_h, sh_ld_gpio, 1));

    /* prepare the clock */
    EXEC_RG(gpio_set_value(p_gpio_h, clk_gpio, 0)); clk_low=TRUE;
    WAIT_CYCLES(SHR_CYCLE);

    /* shift the loaded input */
    for (;;) {
        EXEC_RG(gpio_get_value(p_gpio_h, data_gpio, &bt));

        if (!i) *p_dta=0;
        *p_dta |= (uint8_t)((bt&1)<<i);
        if (++i>>3) { p_dta++; i&=7; }

        EXEC_RG(gpio_set_value(p_gpio_h, clk_gpio, 1)); clk_low=FALSE;
        WAIT_CYCLES(SHR_CYCLE);

        if (!--n_dta_bits) break;

        EXEC_RG(gpio_set_value(p_gpio_h, clk_gpio, 0)); clk_low=TRUE;
        WAIT_CYCLES(SHR_CYCLE);
    }

finish:
    if (clk_low) gpio_set_value(p_gpio_h, clk_gpio, 1);
    return ret;
}

/* exported; see header for details */
lr_errc_t shr_piso_gpio_init(gpio_hndl_t *p_gpio_h,
    unsigned int sh_ld_gpio, unsigned int clk_gpio, unsigned int data_gpio)
{
    lr_errc_t ret=LREC_SUCCESS;

    EXEC_RG(gpio_direction_output(p_gpio_h, sh_ld_gpio, 1));
    EXEC_RG(gpio_direction_output(p_gpio_h, clk_gpio, 1));
    EXEC_RG(gpio_direction_input(p_gpio_h, data_gpio));
finish:
    return ret;
}
