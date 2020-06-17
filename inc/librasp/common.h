/*
   Copyright (c) 2015,2016,2020 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __LR_COMMON_H__
#define __LR_COMMON_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TRUE
#define TRUE    (1==1)
#endif

#ifndef FALSE
#define FALSE   (!TRUE)
#endif

#ifndef NULL
#define NULL    0
#endif

typedef int bool_t;

/* library error codes */
typedef enum _lr_errc_t
{
    LREC_SUCCESS = 0,

    LREC_INV_ARG,       /* invalid argument */
    LREC_NOMEM,         /* no memory error */
    LREC_NOT_SUPP,      /* not supported */
    LREC_NO_SPACE,      /* no space (full) error */
    LREC_EMPTY,         /* empty error */
    LREC_TIMEOUT,       /* operation timed out */
    LREC_NO_RESP,       /* no expected response received */
    LREC_COMM_ERR,      /* communication error (e.g. via sockets) */
    LREC_PROTO_ERR,     /* protocol error */
    LREC_BAD_MSG,       /* bad/unexpected message */
    LREC_MSG_SIZE,      /* message size problem */
    LREC_MMAP_ERR,      /* memory map error */
    LREC_PLAT_ERR,      /* system platform error */
    LREC_DEV_ERR,       /* device error */
    LREC_NO_DEV,        /* no device found */
    LREC_DTA_CRPT,      /* data corrupted/CRC error */
    LREC_CLK_ERR,       /* system clock related error */
    LREC_NOINIT,        /* not initialized */
    LREC_OPEN_ERR,      /* file descriptor open error */
    LREC_READ_ERR,      /* read() error */
    LREC_WRITE_ERR,     /* write() error */
    LREC_IOCTL_ERR,     /* ioctl() error */
    LREC_POLL_ERR,      /* poll() error */
    LREC_SCHED_ERR      /* scheduler related error */
} lr_errc_t;

typedef enum _lr_loglev_t
{
    LRLOG_DEBUG=0,
    LRLOG_INFO,
    LRLOG_WARN,
    LRLOG_ERR,

    /* suppress logging threshold */
    LRLOG_OFF
} lr_loglev_t;

typedef enum _lr_logdst_t
{
    LRLOGTO_STDOUT=0,
    LRLOGTO_SYSLOG
} lr_logdst_t;

void set_librasp_log_dest(lr_logdst_t dest);
lr_logdst_t get_librasp_log_dest(void);

void set_librasp_log_level(lr_loglev_t lev);
lr_loglev_t get_librasp_log_level(void);

typedef enum _platform_t
{
    bcm_2708=0,
    bcm_2709,
    bcm_2710,
    bcm_2711
} platform_t;

/* Platform type detection. Returns -1 in case the platform can't be recognized
 */
platform_t platform_detect();

/* Stuff below constitutes utilities loosely associated with the main
   purpose of the library, but which may be useful for a library's user.
   This part was moved to public part of the common.h because of it's
   potential usefulness, although it has more common to do with closed
   part of the header (e.g. no special "naming decoration").
 */

#define IO_REG8_PTR(b, r)  ((volatile uint8_t*)(b)+(r))
#define IO_REG16_PTR(b, r) (volatile uint16_t*)((volatile uint8_t*)(b)+(r))
#define IO_REG32_PTR(b, r) (volatile uint32_t*)((volatile uint8_t*)(b)+(r))

/* Get BCM SoC I/O peripherals base address. Returns 0 in case the platform
   (and therefore I/O base) can't be recognized
 */
uint32_t get_bcm_io_base();

/* Map I/O into the virtual space; return NULL in case of error */
volatile void *io_mmap(uint32_t io_base, uint32_t len);

/* Bytes 'in' to hex conversion (written into 'out') */
void bts2hex(const uint8_t *p_in, size_t in_len, char *outstr);

/* Wait at least specified amount of CPU cycles */
#define WAIT_CYCLES(c) { register volatile int r; for (r=(c)/2; r; r--); }

#define SET_BITFLD(v, b, m) (((v)&~(m))|(b))

#define ARRAY_SZ(a) (sizeof((a))/sizeof((a)[0]))

#define RNDUP(x, d) ((d)*(((x)+((d)-1))/(d)))
/* special cases */
#define RNDUP2(x)   ((((x)+1)>>1)<<1)
#define RNDUP4(x)   ((((x)+3)>>2)<<2)
#define RNDUP8(x)   ((((x)+7)>>3)<<3)

#define MIN(a, b)   ((a)<(b) ? (a) : (b))
#define MAX(a, b)   ((a)>(b) ? (a) : (b))

typedef struct _sched_rt
{
    int sched;      /* original scheduler */
    int prio;       /* original scheduler's priority */
} sched_rt_t;

/* Change current process scheduler to the real-time one with the maximum
   possible priority. 'p_sched_h' is a pointer to a handle which will be
   filled with data needed to restore the original scheduler.

   This function should be used for timing critical section of code. On
   exit of such section sched_restore() shall be used to restore the original
   scheduler and its priority.
 */
lr_errc_t sched_rt_raise_max(sched_rt_t *p_sched_h);

/* Restore the original scheduler and its priority changed previously by
   sched_rt_raise_max().

   NOTE: The function may be safety called with the same handle multiple times
   or even for a handle returned by failed sched_rt_raise_max().
 */
lr_errc_t sched_restore(sched_rt_t *p_sched_h);

#ifdef __cplusplus
}
#endif

#endif /* __LR_COMMON_H__ */
