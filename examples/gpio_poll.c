/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* GPIO sysfs polling.
 */

#include <stdio.h>
#include <unistd.h>
#include "librasp/gpio.h"

#define GPIO_IN         22
#define POLL_TIMEOUT    10000

#define EXEC_G(c) if ((c)!=LREC_SUCCESS) goto finish;

int main(int argc, char **argv)
{
    bool_t h_init=FALSE, exprt=FALSE;
    gpio_hndl_t gpio_h;

    /* set pull-up resistor (it's possible via I/O driver only) */
    EXEC_G(gpio_init(&gpio_h, gpio_drv_io));
    gpio_bcm_set_pull_config(&gpio_h, GPIO_IN, gpio_pull_up);
    h_init = TRUE;

    /* switch to SYSFS driver */
    gpio_set_driver(&gpio_h, gpio_drv_sysfs);

    /* export the GPIO is required by sysfs */
    EXEC_G(gpio_sysfs_export(&gpio_h, GPIO_IN));
    exprt = TRUE;

    EXEC_G(gpio_direction_input(&gpio_h, GPIO_IN));

    /* config the event & poll */
    EXEC_G(gpio_set_event(&gpio_h, GPIO_IN, GPIO_EVENT_BOTH));

    printf("Waiting %d seconds for an event on GPIO%d\n",
        POLL_TIMEOUT/1000, GPIO_IN);

    switch(gpio_sysfs_poll(&gpio_h, GPIO_IN, POLL_TIMEOUT))
    {
    case LREC_SUCCESS:
        printf("Event detected\n");
        break;
    case LREC_TIMEOUT:
        printf("Timeout\n");
        break;
    default:
        printf("gpio_sysfs_poll() finished with error\n");
        break;
    }

finish:
    if (exprt) gpio_sysfs_unexport(&gpio_h, GPIO_IN);
    if (h_init) gpio_free(&gpio_h);
    return 0;
}
