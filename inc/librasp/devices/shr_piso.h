/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __LR_DEVS_DS_THERM_H__
#define __LR_DEVS_DS_THERM_H__

#include "librasp/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Read PISO shift register input bits (in number of 'n_dta_bits') and write the
   result under 'p_dta' in the following order: 1st input: 0-bit of p_dta[0];
   2nd input: 1-bit of p_dta[0], ..., 8th input: 0bit of p_dta[1], etc. GPIOs
   pins for SH/~LD, CLK and the retrieved data pin (shift register output: Q or
   ~Q) are provided as arguments for the function.

   GPIO pre-call state:
       SH/~LD, CLK [out]: high,
       data [in]: pull configuration depends on requirements.
   GPIO post-call state:
       As for pre-call.

   NOTE: PISO's CLK-INH is not used and must be pulled to the ground.
 */
lr_errc_t shr_piso_read(gpio_hndl_t *p_gpio_h, unsigned int sh_ld_gpio,
    unsigned int clk_gpio, unsigned int data_gpio, unsigned int n_dta_bits,
    uint8_t *p_dta);

/* Initializes GPIO in/out pins for shr_piso_read() call.
   NOTE: The function doesn't set pull resistor configuration.
 */
lr_errc_t shr_piso_gpio_init(gpio_hndl_t *p_gpio_h,
    unsigned int sh_ld_gpio, unsigned int clk_gpio, unsigned int data_gpio);

#ifdef __cplusplus
}
#endif

#endif  /* __LR_DEVS_DS_THERM_H__ */
