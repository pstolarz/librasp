/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __LR_DEVS_HCSR_H__
#define __LR_DEVS_HCSR_H__

#include "librasp/gpio.h"
#include "librasp/clock.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Probe HC SR04 distance sensor. The distance in cm is written under 'p_dist_cm'.

   GPIO pre-call state:
       TRIG [out]: low,
       ECHO [in]: pull configuration depends on requirements.
   GPIO post-call state:
       As for pre-call.
 */
lr_errc_t hcsr_probe(gpio_hndl_t *p_gpio_h, clock_hndl_t *p_clk_h,
    unsigned int trig_gpio, unsigned int echo_gpio, unsigned int *p_dist_cm);

#ifdef __cplusplus
}
#endif

#endif /* __LR_DEVS_HCSR_H__ */
