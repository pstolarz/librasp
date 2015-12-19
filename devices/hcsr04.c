/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include "common.h"
#include "librasp/devices/hcsr04.h"

/* threshold for breaking wait for the response loop (usec) */
#define BREAK_THRSHD    1000U

#define CM_PULSE_USEC   58U


/* exported; see header for details */
lr_errc_t hcsr_probe(gpio_hndl_t *p_gpio_h, clock_hndl_t *p_clk_h,
    unsigned int trig_gpio, unsigned int echo_gpio, unsigned int *p_dist_cm)
{
    lr_errc_t ret=LREC_SUCCESS;

    unsigned int state, resp_cnt;
    bool_t set_high=FALSE, sched_set=FALSE;

    uint32_t start;

    int sched, prio;
    struct sched_param sparam;

    EXECLK_RG(clock_get_ticks32(p_clk_h, &start));

    sched=sched_getscheduler(0);
    errno=0; prio=getpriority(PRIO_PROCESS, 0);

    /* Enter timing critical part
     */
    if (sched>=0 && (prio!=-1 || !errno)) {
        sparam.sched_priority = sched_get_priority_max(SCHED_RR);
        if (sched_setscheduler(0, SCHED_RR, &sparam)!=-1) sched_set=TRUE;
    }

    /* trigger the sensor */

    /* 100us high */
    EXEC_RG(gpio_set_value(p_gpio_h, trig_gpio, 1)); set_high=TRUE;
    EXECLK_RG(clock_usleep(p_clk_h, 100));

    /* 30us low */
    EXEC_RG(gpio_set_value(p_gpio_h, trig_gpio, 0)); set_high=FALSE;

    /* retrieve the response loop */
    for (state=0, resp_cnt=0; state<2;)
    {
        uint32_t tick;
        unsigned int echo;

        EXEC_RG(gpio_get_value(p_gpio_h, echo_gpio, &echo));
        EXECLK_RG(clock_get_ticks32(p_clk_h, &tick));

        switch(state)
        {
        case 0:     /* request sent, wait for echo */
            if (echo) {
                state++;
                resp_cnt++;
            } else {
                /* no input changed; check the break threshold */
                if (tick-start>=BREAK_THRSHD) goto break_readloop;
            }
            break;

        case 1:     /* count echo response */
            if (echo)
                resp_cnt++;
            else
                state++;    /* finishes the loop */
            break;
        }
        EXECLK_RG(clock_usleep(p_clk_h, CM_PULSE_USEC));
    }
break_readloop:

    /* Exit timing critical part
     */
    if (sched_set) {
        sparam.sched_priority = prio;
        if (sched_setscheduler(0, sched, &sparam)!=-1) sched_set=FALSE;
    }

    if (!resp_cnt) {
        err_printf("[%s] No sensor response\n", __func__);
        ret = LREC_NO_RESP;
        goto finish;
    }

    dbg_printf("Sensor response time: %d usec [%d cm]\n",
        resp_cnt*CM_PULSE_USEC, resp_cnt);

    *p_dist_cm = resp_cnt;

finish:
    if (sched_set) {
        sparam.sched_priority = prio;
        if (sched_setscheduler(0, sched, &sparam)==-1)
        {
            warn_printf("[%s] Can't restore original scheduler of the process; "
                "sched_setscheduler() error: %d; %s\n",
                __func__, errno, strerror(errno));
        }
    }
    if (set_high) {
        gpio_set_value(p_gpio_h, trig_gpio, 0);
    }
    return ret;
}
