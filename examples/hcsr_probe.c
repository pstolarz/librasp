/*
   Copyright (c) 2015,2016 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <stdio.h>
#include <unistd.h>
#include "librasp/devices/hcsr04.h"

/* HC SR04 distance sensor probe. Connect the sensor pins according to GPIO_TRIG
   and GPIO_ECHO.
 */

#define GPIO_TRIG   4
#define GPIO_ECHO   14

int main(int argc, char **argv, char **envp)
{
    clock_hndl_t clk_h;
    gpio_hndl_t gpio_h;
    unsigned int dist_cm;

    bool_t clk_inited=FALSE, gpio_inited=FALSE;

    if (!(gpio_inited=(gpio_init(&gpio_h, gpio_drv_io)==LREC_SUCCESS)) ||
        !(clk_inited=(clock_init(&clk_h, clock_drv_io)==LREC_SUCCESS)))
        goto finish;

    gpio_direction_output(&gpio_h, GPIO_TRIG, 0);

    gpio_direction_input(&gpio_h, GPIO_ECHO);
    gpio_bcm_set_pull_config(&gpio_h, GPIO_ECHO, gpio_bcm_pull_off);

    for (;;) {
        if (hcsr_probe(&gpio_h, &clk_h,
            GPIO_TRIG, GPIO_ECHO, &dist_cm)!=LREC_SUCCESS) break;

        printf("Distance: %d cm\n", dist_cm);
        usleep(200*1000);
    }

finish:
    if (clk_inited) clock_free(&clk_h);
    if (gpio_inited) gpio_free(&gpio_h);
    return 0;
}
