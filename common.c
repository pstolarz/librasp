/*
   Copyright (c) 2015,2016 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include "common.h"
#include "librasp/bcm_platform.h"

#define DEV_MEM     "/dev/mem"

/* logging destination */
static lr_logdst_t log_dest = LRLOGTO_STDOUT;

/* logging level */
static lr_loglev_t log_lev = LRLOG_ERR;

/* exported; see header for details */
void set_librasp_log_dest(lr_logdst_t dest) {
    log_dest = (dest==LRLOGTO_SYSLOG ? dest : LRLOGTO_STDOUT);
}
lr_logdst_t get_librasp_log_dest(void) { return log_dest; }

/* exported; see header for details */
void set_librasp_log_level(lr_loglev_t lev) { log_lev=lev; }
lr_loglev_t get_librasp_log_level(void) { return log_lev; }

/* exported; see header for details */
platform_t platform_detect()
{
    platform_t plat=(platform_t)-1;
    FILE *f = fopen("/proc/cpuinfo", "r");

    if (f) {
        char line[64];
        while (fgets(line, sizeof(line), f)) {
            if ((line[0]=='H' || line[0]=='h') &&
                !strncmp("ardware", &line[1], 7))
            {
                if (strstr(line, "BCM2708")) plat=bcm_2708;
                else
                if (strstr(line, "BCM2709")) plat=bcm_2709;
                break;
            }
        }
        fclose(f);
    }
    return plat;
}

/* exported; see header for details */
uint32_t get_bcm_io_base()
{
    uint32_t ret=0;

    switch (platform_detect())
    {
    case bcm_2708:
        ret=BCM2708_PERI_BASE;
        break;
    case bcm_2709:
        ret=BCM2709_PERI_BASE;
        break;
    }
    return ret;
}

/* exported; see header for details */
void bts2hex(const uint8_t *p_in, size_t in_len, char *outstr)
{
    static char HEX_DIGS[] = "0123456789abcdef";
    size_t i,j;

    for (i=j=0; i<in_len; i++) {
        outstr[j++] = HEX_DIGS[p_in[i]>>4];
        outstr[j++] = HEX_DIGS[p_in[i]&0x0f];
    }
    outstr[j]=0;
}

/* Log to the standard output */
static void vstdlog(
    lr_loglev_t lev, const char *format, va_list args)
{
    static const char *MONTH[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    const char *prefix=NULL;
    FILE *stream=stdout;
    time_t systm=0;
    struct tm *loctm;

    switch (lev)
    {
    case LRLOG_DEBUG:
        prefix = "DEBUG";
        stream = stdout;
        break;
    case LRLOG_INFO:
        prefix = "INFO";
        stream = stdout;
        break;
    case LRLOG_WARN:
        prefix = "WARN";
        stream = stderr;
        break;
    case LRLOG_ERR:
        prefix = "ERROR";
        stream = stderr;
        break;
    case LRLOG_OFF:           /* must not be used */
        return;
    }

    time(&systm);
    loctm = localtime(&systm);
    if (loctm) fprintf(stream, "%s %02d %02d:%02d:%02d ", MONTH[loctm->tm_mon],
        loctm->tm_mday, loctm->tm_hour, loctm->tm_min, loctm->tm_sec);

    if (prefix) fprintf(stream, "%s: ", prefix);
    vfprintf(stream, format, args);
    fflush(stream);
}

/* Common logging routine */
static void vprintf_log(
    lr_loglev_t lev, const char *format, va_list args)
{
    if (log_dest==LRLOGTO_SYSLOG) {
        int priority;
        switch (lev)
        {
        default:
        case LRLOG_DEBUG:
            priority = LOG_DEBUG;
            break;
        case LRLOG_INFO:
            priority = LOG_INFO;
            break;
        case LRLOG_WARN:
            priority = LOG_WARNING;
            break;
        case LRLOG_ERR:
            priority = LOG_ERR;
            break;
        }
        vsyslog(priority, format, args);
    } else {
        vstdlog(lev, format, args);
    }
}

#define MSG_PRINT_METHOD(name, level) \
    void name(const char *format, ...) { \
        if (level>=log_lev) { \
            va_list args; \
            va_start(args, format); \
            vprintf_log(level, format, args); \
            va_end(args); \
        } \
    }

MSG_PRINT_METHOD(dbg_printf, LRLOG_DEBUG)
MSG_PRINT_METHOD(info_printf, LRLOG_INFO)
MSG_PRINT_METHOD(warn_printf, LRLOG_WARN)
MSG_PRINT_METHOD(err_printf, LRLOG_ERR)

/* exported; see header for details */
volatile void *io_mmap(uint32_t io_base, uint32_t len)
{
    volatile void *ret = NULL;
    int phmem = -1;

    if ((phmem = open(DEV_MEM, O_RDWR|O_SYNC))<0) {
        err_printf("[%s] Open " DEV_MEM " error: %d; %s\n",
            __func__, errno, strerror(errno)) ;
    } else
    {
        ret = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, phmem, io_base);
        if (ret==MAP_FAILED) {
            err_printf("[%s] mmap() failed: %d; %s\n",
                __func__, errno, strerror(errno));
            ret=NULL;
        }
        close(phmem);
    }
    return ret;
}

#define RT_SCHED    SCHED_RR

/* exported; see header for details */
lr_errc_t sched_rt_raise_max(sched_rt_t *p_sched_h)
{
    lr_errc_t ret = LREC_SCHED_ERR;
    struct sched_param param;

    p_sched_h->sched = sched_getscheduler(0);

    errno=0;
    p_sched_h->prio = getpriority(PRIO_PROCESS, 0);

    if (p_sched_h->sched>=0 && (p_sched_h->prio!=-1 || !errno)) {
        param.sched_priority = sched_get_priority_max(RT_SCHED);
        if (sched_setscheduler(0, RT_SCHED, &param)!=-1)
            ret = LREC_SUCCESS;
    }

    if (ret!=LREC_SUCCESS) {
        err_printf("[%s] Can't set real-time scheduler; error: %d; %s\n",
            __func__, errno, strerror(errno));

        /* mark as unable to restore */
        p_sched_h->sched = -1;
    }

    return ret;
}

/* exported; see header for details */
lr_errc_t sched_restore(sched_rt_t *p_sched_h)
{
    lr_errc_t ret = LREC_SUCCESS;
    struct sched_param param;

    if (p_sched_h->sched >= 0) {
        param.sched_priority = p_sched_h->prio;
        if (sched_setscheduler(0, p_sched_h->sched, &param)!=-1)
        {
            err_printf("[%s] Can't restore original scheduler of the process; "
                "sched_setscheduler() error: %d; %s\n",
                __func__, errno, strerror(errno));

            ret = LREC_SCHED_ERR;
        } else
            /* mark as restored */
            p_sched_h->sched = -1;
    }

    return ret;
}
