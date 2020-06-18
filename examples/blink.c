/*
   Copyright (c) 2015,2016 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* GPIO input/output test (I/O version).

   Connect switch and LED to the GPIO_IN and GPIO_OUT pins respectively. Press
   the switch and observe LED blinking.
 */

#include <unistd.h>
#include "librasp/gpio.h"

#define GPIO_IN     4
#define GPIO_OUT    27

#define STATES      6


int main(int argc, char **argv)
{
    gpio_hndl_t gpio_h;
    unsigned int i, state, led, div, prev_input;

    state=0; led=0; div=1; prev_input=1;

    if (gpio_init(&gpio_h, gpio_drv_gpio)!=LREC_SUCCESS) goto finish;

    gpio_direction_output(&gpio_h, GPIO_OUT, 0);

    gpio_direction_input(&gpio_h, GPIO_IN);
    gpio_bcm_set_pull_config(&gpio_h, GPIO_IN, gpio_bcm_pull_up);

    for (i=0; ;i=(i+1)%1000)
    {
        unsigned int input;

        if (state < STATES-1) {
            if (!(i%(500/div))) gpio_set_value(&gpio_h, GPIO_OUT, led=!led);
        } else {
            i--;
        }

        usleep(1000);

        gpio_get_value(&gpio_h, GPIO_IN, &input);

        if (!input && prev_input!=input) {
            state = (state+1) % STATES;
            div = (state ? div<<1 : 1);
        }
        prev_input = input;
    }

    /* protect the out pin  */
    gpio_direction_input(&gpio_h, GPIO_OUT);

    gpio_free(&gpio_h);

finish:
    return 0;
}
