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
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>

#include "common.h"
#include "librasp/clock.h"

#define BCM_STC_MAP_LEN         PAGE_SZ
#define BCM_DEF_USLEEP_THRSHD   400U

/* exported; see header for details */
lr_errc_t clock_init(clock_hndl_t *p_hndl, clock_driver_t drv)
{
    memset(p_hndl, 0, sizeof(*p_hndl));
    p_hndl->drv = (clock_driver_t)-1;
    return clock_set_driver(p_hndl, drv);
}

/* exported; see header for details */
lr_errc_t clock_set_driver(clock_hndl_t *p_hndl, clock_driver_t drv)
{
    lr_errc_t ret=LREC_SUCCESS;

    switch (drv)
    {
    case clock_drv_io:
      {
        if (!p_hndl->io.p_stc_io)
        {
            uint32_t io_base = get_bcm_io_base();
            if (io_base) {
                if (!(p_hndl->io.p_stc_io=
                        io_mmap(io_base+ST_BASE_RA, BCM_STC_MAP_LEN)))
                    ret=LREC_MMAP_ERR;
            } else {
                err_printf("[%s] BCM platform not detected\n", __func__);
                ret=LREC_PLAT_ERR;
            }
        } else {
            /* already initialized I/O */
        }
        break;
      }

    case clock_drv_sys:
#ifdef CONFIG_CLOCK_SYS_DRIVER
        /* no initialization needed in this case */
#else
        ret=LREC_NOT_SUPP;
#endif
        break;

    default:
        ret=LREC_INV_ARG;
        break;
    }

    if (ret==LREC_SUCCESS) p_hndl->drv = drv;
    return ret;
}

/* exported; see header for details */
void clock_free(clock_hndl_t *p_hndl)
{
    if (p_hndl->drv!=(clock_driver_t)-1)
    {
        /* free I/O driver resources */
        if (p_hndl->io.p_stc_io) {
            munmap((void*)p_hndl->io.p_stc_io, BCM_STC_MAP_LEN);
            p_hndl->io.p_stc_io = NULL;
        }

        /* mark the handle as closed */
        p_hndl->drv = (clock_driver_t)-1;
    }
}

#define get_bcm_clock_ticks_lo(h) \
    ((uint32_t)*IO_REG32_PTR((h)->io.p_stc_io, ST_CLO))
#define get_bcm_clock_ticks_hi(h) \
    ((uint32_t)*IO_REG32_PTR((h)->io.p_stc_io, ST_CHI))

/* exported; see header for details */
lr_errc_t clock_get_ticks32(clock_hndl_t *p_hndl, uint32_t *p_ticks)
{
    lr_errc_t ret=LREC_SUCCESS;

    if (p_hndl->drv==clock_drv_io) {
        *p_ticks = get_bcm_clock_ticks_lo(p_hndl);
    } else {
#ifdef CONFIG_CLOCK_SYS_DRIVER
        struct timespec tp;
        if (!clock_gettime(CLOCK_MONOTONIC, &tp)) {
            *p_ticks = tp.tv_nsec;
        } else {
            err_printf("[%s] clock_gettime() error %d; %s\n",
                __func__, errno, strerror(errno));
            ret=LREC_CLK_ERR;
        }
#else
        ret=LREC_NOT_SUPP;
#endif
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t clock_get_ticks64(clock_hndl_t *p_hndl, uint64_t *p_ticks)
{
    lr_errc_t ret=LREC_SUCCESS;

    if (p_hndl->drv==clock_drv_io) {
        register uint32_t chi2;
        register uint32_t chi = get_bcm_clock_ticks_hi(p_hndl);
        register uint32_t clo = get_bcm_clock_ticks_lo(p_hndl);

        if ((chi2=get_bcm_clock_ticks_hi(p_hndl)) > chi) {
            /* ST_CLO reg overflow */
            clo = get_bcm_clock_ticks_lo(p_hndl);
            chi = chi2;
        }
        *p_ticks = ((uint64_t)chi<<32)|clo;
    } else {
#ifdef CONFIG_CLOCK_SYS_DRIVER
        struct timespec tp;
        if (!clock_gettime(CLOCK_MONOTONIC, &tp)) {
            *p_ticks = (uint64_t)tp.tv_sec*1000000000LL + tp.tv_nsec;
        } else {
            err_printf("[%s] clock_gettime() error %d; %s\n",
                __func__, errno, strerror(errno));
            ret=LREC_CLK_ERR;
        }
#else
        ret=LREC_NOT_SUPP;
#endif
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t clock_bcm_usleep(clock_hndl_t *p_hndl, uint32_t usec, uint32_t thrshd)
{
    lr_errc_t ret=LREC_SUCCESS;

    if (p_hndl->drv==clock_drv_io) {
        uint32_t ticks, stop, time;

        ticks = get_bcm_clock_ticks_lo(p_hndl);
        stop = ticks+usec;

        for (time=0; time<usec; )
        {
            uint32_t delta;

            if (stop-ticks >= thrshd) {
                /* don't waste CPU time for long sleeps */
                usleep((usec-time)>>1);
            }

            delta = get_bcm_clock_ticks_lo(p_hndl)-ticks;
            if (delta) {
                time = (time+delta<time ? (uint32_t)-1 : time+delta);
                ticks += delta;
            }
        }
    } else {
        ret=LREC_NOT_SUPP;
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t clock_usleep(clock_hndl_t *p_hndl, uint32_t usec)
{
    lr_errc_t ret=LREC_SUCCESS;

    if (p_hndl->drv==clock_drv_io) {
        clock_bcm_usleep(p_hndl, usec, BCM_DEF_USLEEP_THRSHD);
    } else {
#ifdef CONFIG_CLOCK_SYS_DRIVER
        if (usleep(usec)) {
            err_printf("[%s] usleep() error %d; %s\n",
                __func__, errno, strerror(errno));
            ret = LREC_CLK_ERR;
        }
#else
        ret=LREC_NOT_SUPP;
#endif
    }
    return ret;
}
