/*
   Copyright (c) 2015,2016 Piotr Stolarz
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
#include "librasp/bcm_platform.h"

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

    /* I/O driver related */
    struct {
        volatile void *p_stc_io;
    } io;
} clock_hndl_t;

/* Initialize clock handle and set a given driver as active for the handle.

   NOTE: The initiated handle may be freely shared between all system clock
   operations.
 */
lr_errc_t clock_init(clock_hndl_t *p_hndl, clock_driver_t drv);

/* Activate clock driver for (already initialized) handle. Any subsequent clock
   API calls will be performed in a context of this driver.

   This function always success for SYS driver (if configured of course).
   For I/O driver may fail for the first-time call on I/O mapping error
   (LREC_MMAP_ERR). Once successes it will always success for the subsequent
   I/O driver activations.
 */
lr_errc_t clock_set_driver(clock_hndl_t *p_hndl, clock_driver_t drv);

/* Free clock handle
 */
void clock_free(clock_hndl_t *p_hndl);

/* Get 32/64 bit clock tick counters; the 64-bit version is slightly slower than
   the 32-bit counterpart and should be avoided for time critical operations.

   Operation performed by the functions and its result depends on the active
   driver already set:
   - For the I/O driver the functions always success,
   - If configured the function may fail for the SYS driver if the underlying
     system function fails.
 */
lr_errc_t clock_get_ticks32(clock_hndl_t *p_hndl, uint32_t *p_ticks);
lr_errc_t clock_get_ticks64(clock_hndl_t *p_hndl, uint64_t *p_ticks);

/* Sleep at least 'usec'.

   Operation performed by the function and its result depends on the active
   driver already set:
   - For the I/O driver the functions always success,
   - If configured the function may fail for the SYS driver if the underlying
     system function fails.
 */
lr_errc_t clock_usleep(clock_hndl_t *p_hndl, uint32_t usec);

/* Sleep at least 'usec'.

   NOTE: The function requires initialized I/O driver, which means
   clock_set_driver() must be previously called for this driver (but the driver
   need not to be active at the moment of call) otherwise LREC_NOINIT is
   returned.
 */
lr_errc_t clock_bcm_usleep(clock_hndl_t *p_hndl, uint32_t usec, uint32_t thrshd);

#ifdef __cplusplus
}
#endif

#endif /* __LR_CLOCK_H__ */
