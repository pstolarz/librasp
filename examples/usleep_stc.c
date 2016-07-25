/*
   Copyright (c) 2015,2016 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <sched.h>
#include <stdio.h>
#include <sys/resource.h>
#include "librasp/clock.h"

/* Accuracy check for STC's usleep() implementation.
 */

int main(int argc, char **argv)
{
    uint32_t start, stop;
    struct sched_param sparam;
    int reg_sched, reg_prio;
    clock_hndl_t clk_h;

    if (clock_init(&clk_h, clock_drv_io)!=LREC_SUCCESS) goto finish;

#define __USLEEP_TEST(u) \
    clock_get_ticks32(&clk_h, &start); \
    clock_usleep(&clk_h, (u)); \
    clock_get_ticks32(&clk_h, &stop); \
    printf("  BCM usleep(%d) -> start:0x%08X, stop:0x%08X, delta:%d\n", \
        (u), start, stop, stop-start); \

    reg_sched = sched_getscheduler(0);
    reg_prio = getpriority(PRIO_PROCESS, 0);

    printf("Regular scheduler(%d); prio:%d\n", reg_sched, reg_prio);
    __USLEEP_TEST(1);     __USLEEP_TEST(5);     __USLEEP_TEST(10);
    __USLEEP_TEST(50);    __USLEEP_TEST(100);   __USLEEP_TEST(200);
    __USLEEP_TEST(500);   __USLEEP_TEST(1000);  __USLEEP_TEST(10000);
    __USLEEP_TEST(100000);

    sparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (!sched_setscheduler(0, SCHED_FIFO, &sparam))
    {
        printf("FIFO scheduler(%d); prio:%d:\n", SCHED_FIFO,
            sparam.sched_priority);
        __USLEEP_TEST(1);     __USLEEP_TEST(5);    __USLEEP_TEST(10);
        __USLEEP_TEST(50);    __USLEEP_TEST(100);  __USLEEP_TEST(200);
        __USLEEP_TEST(500);   __USLEEP_TEST(1000); __USLEEP_TEST(10000);
        __USLEEP_TEST(100000);
    }

    sparam.sched_priority = sched_get_priority_max(SCHED_RR);
    if (!sched_setscheduler(0, SCHED_RR, &sparam))
    {
        printf("RR scheduler(%d); prio:%d:\n", SCHED_RR, sparam.sched_priority);
        __USLEEP_TEST(1);     __USLEEP_TEST(5);    __USLEEP_TEST(10);
        __USLEEP_TEST(50);    __USLEEP_TEST(100);  __USLEEP_TEST(200);
        __USLEEP_TEST(500);   __USLEEP_TEST(1000); __USLEEP_TEST(10000);
        __USLEEP_TEST(100000);
    }

    if (reg_sched>=0) {
        sparam.sched_priority = reg_prio;
        sched_setscheduler(0, reg_sched, &sparam);
    }

    clock_free(&clk_h);

#undef __USLEEP_TEST

finish:
    return 0;
}
