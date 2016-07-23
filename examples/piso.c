/*
   Copyright (c) 2015,2016 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* Read PISO shift register example.

   This code was tested on 74165 TI shift reg. with SH/LD, CLK and DATA pins
   connected to GPIOs as defined by GPIO_XXX defs. CLK-INH is not used and must
   be pulled to the ground.
 */

#include <stdio.h>
#include "librasp/devices/shr_piso.h"

/* GPIOs connected to the SHR */
#define GPIO_SH_LD      21
#define GPIO_CLK        20
#define GPIO_DATA       26

/* number of parallel inputs to be read */
#define INPUTS          10

int main(void)
{
    gpio_hndl_t gpio_h;

    if (gpio_init(&gpio_h, gpio_drv_io)==LREC_SUCCESS)
    {
        uint8_t dta[RNDUP8(INPUTS)/8];
        char hex[2*sizeof(dta)+4];

        shr_piso_gpio_init(&gpio_h, GPIO_SH_LD, GPIO_CLK, GPIO_DATA);
        gpio_bcm_set_pull_config(&gpio_h, GPIO_DATA, gpio_bcm_pull_down);

        shr_piso_read(&gpio_h, GPIO_SH_LD, GPIO_CLK, GPIO_DATA, INPUTS, dta);

        bts2hex(dta, sizeof(dta), hex);
        printf("data: %s\n", hex);

        /* protect the out pins */
        gpio_direction_input(&gpio_h, GPIO_SH_LD);
        gpio_direction_input(&gpio_h, GPIO_CLK);
    }
    return 0;
}
