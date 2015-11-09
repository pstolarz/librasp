/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <stdio.h>
#include "librasp/devices/shr_piso.h"

/* GPIOs connected to the SHR */
#define SH_LD_GPIO      21
#define CLK_GPIO        20
#define DATA_GPIO       26

/* number of parallel inputs to be read */
#define INPUTS          10

int main(void)
{
    gpio_hndl_t gpio_h;

    if (gpio_init(&gpio_h, gpio_drv_io)==LREC_SUCCESS)
    {
        uint8_t dta[RNDUP8(INPUTS)/8];
        char hex[2*sizeof(dta)+4];

        shr_piso_gpio_init(&gpio_h, SH_LD_GPIO, CLK_GPIO, DATA_GPIO);
        gpio_bcm_set_pull_config(&gpio_h, DATA_GPIO, gpio_pull_down);

        shr_piso_read(&gpio_h, SH_LD_GPIO, CLK_GPIO, DATA_GPIO, INPUTS, dta);

        bts2hex(dta, sizeof(dta), hex);
        printf("data: %s\n", hex);

        /* protect the out pins */
        gpio_direction_input(&gpio_h, SH_LD_GPIO);
        gpio_direction_input(&gpio_h, CLK_GPIO);
    }
    return 0;
}
