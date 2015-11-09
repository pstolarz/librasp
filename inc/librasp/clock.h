/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __LR_CLOCK_H__
#define __LR_CLOCK_H__

#include "librasp/common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _clock_driver_t
{
    clock_drv_io=0,
    clock_drv_sys       /* if configured (CONFIG_CLOCK_SYS_DRIVER) */
} clock_driver_t;

typedef struct _clock_hndl_t
{
    clock_driver_t drv;

    /* BCM driver related */
    struct {
        volatile void *p_stc_io;
    } bcm;
} clock_hndl_t;

/* Free clock handle. Need not to be called for the SYS driver. */
void clock_free(clock_hndl_t *p_hndl);

/* Init clock handle. If configured, the function always successes for the SYS
   driver.

   NOTE: The initiated handle may be freely shared between all system clock
   operations basing on the same driver.
 */
lr_errc_t clock_init(clock_hndl_t *p_hndl, clock_driver_t drv);

/* Get 32/64 bit clock tick counters; the 64-bit version is slightly slower than
   the 32-bit counterpart and should be avoided for time critical operations.
   The functions always success for the BCM driver. */
lr_errc_t clock_get_ticks32(clock_hndl_t *p_hndl, uint32_t *p_ticks);
lr_errc_t clock_get_ticks64(clock_hndl_t *p_hndl, uint64_t *p_ticks);

/* Sleep at least 'usec'. The function always successes for the BCM driver. */
lr_errc_t clock_usleep(clock_hndl_t *p_hndl, uint32_t usec);

/* Sleep at least 'usec'. The function is BCM driver specific and always
   successes if called for this driver. */
lr_errc_t clock_bcm_usleep(clock_hndl_t *p_hndl, uint32_t usec, uint32_t thrshd);

#ifdef __cplusplus
}
#endif

#endif /* __LR_CLOCK_H__ */
