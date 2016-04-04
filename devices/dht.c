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
#include "librasp/devices/dht.h"

/* threshold for breaking reading DHT response loop (usec) */
#define BREAK_THRSHD    500U

#define DHT_DTA_BITS    40U


/* dht_probe() with flag indicating if the probe is retried or not.
 */
static lr_errc_t __dht_probe(
    gpio_hndl_t *p_gpio_h, clock_hndl_t *p_clk_h, unsigned int gpio,
    dht_model_t model, unsigned int *p_rh, int *p_temp, bool_t retried)
{
    lr_errc_t ret=LREC_SUCCESS;

    size_t i, j;
    unsigned int state;
    bool_t set_low=FALSE, sched_set=FALSE;

    int sched, prio;
    struct sched_param sparam;

    uint8_t data[DHT_DTA_BITS/8], crc;
    uint32_t start, siglens[DHT_DTA_BITS+1];

    memset(data, 0, sizeof(data));

    sched=sched_getscheduler(0);
    errno=0; prio=getpriority(PRIO_PROCESS, 0);

    /* Enter timing critical part
     */
    if (sched>=0 && (prio!=-1 || !errno)) {
        sparam.sched_priority = sched_get_priority_max(SCHED_RR);
        if (sched_setscheduler(0, SCHED_RR, &sparam)!=-1) sched_set=TRUE;
    }

    /* initialize DHT sensor communication */

    /* 20ms down */
    EXEC_RG(gpio_opndrn_set_value(p_gpio_h, gpio, 0)); set_low=TRUE;
    EXECLK_RG(clock_usleep(p_clk_h, 20000));

    /* 30us high */
    EXEC_RG(gpio_opndrn_set_value(p_gpio_h, gpio, 1)); set_low=FALSE;
    EXECLK_RG(clock_usleep(p_clk_h, 30));

    EXECLK_RG(clock_get_ticks32(p_clk_h, &start));

    /* retrieve the response loop */
    for (i=0, state=3; i<ARRAY_SZ(siglens);)
    {
        uint32_t tick;
        unsigned int in;

        EXEC_RG(gpio_get_value(p_gpio_h, gpio, &in));
        EXECLK_RG(clock_get_ticks32(p_clk_h, &tick));

        switch (state)
        {
        case 0: /* down state */
        case 1: /* high state (data encoding) */
            if (state^in) {
                if (state) siglens[i++] = tick-start;
                start = tick;
                state ^= 1;
                continue;
            }
            break;

        /* initial state: wait for down the wire */
        case 3:
            if (!in) {
                start = tick;
                state = 0;
                continue;
            }
            break;
        }

        /* no input changed; check the break threshold */
        if (tick-start>=BREAK_THRSHD) break;
    }

    /* Exit timing critical part
     */
    if (sched_set) {
        sparam.sched_priority = prio;
        if (sched_setscheduler(0, sched, &sparam)!=-1) sched_set=FALSE;
    }

    if (!i) {
        char *log = "[%s] No sensor response\n";
        if (!retried) err_printf(log, __func__);
        else warn_printf(log, __func__);

        ret = LREC_NO_RESP;
        goto finish;
    }

    if (get_librasp_log_level()<=LRLOG_DEBUG) {
        int len;
        char prnt_buf[100+20*ARRAY_SZ(siglens)];

        len=sprintf(prnt_buf, "DHT sensor response signal timings:\n");
        for (j=0; j<i; j++)
            len+=sprintf(prnt_buf+len, "  %d: %d\n", (int)j, siglens[j]);
        dbg_printf(prnt_buf);
    }

    j=0;    /* start of the data signals index */
    if (i<DHT_DTA_BITS) {
        char *log = "[%s] Data corrupted\n";
        if (!retried) err_printf(log, __func__);
        else warn_printf(log, __func__);

        ret = LREC_DTA_CRPT;
        goto finish;
    }
    if (i>DHT_DTA_BITS) j++;            /* skip initial signal */

    for (i=0; i<DHT_DTA_BITS; i++, j++) {
        /* 50us is the 0/1 threshold for the DHT's PWM encoding */
        if (siglens[j]>50) data[i>>3] |= 1<<(7-(i&7));
    }

    if (model==dht11) {
        dbg_printf("Decoded data (DHT11): %d[int-RH], %d[dec-RH], %d[int-T], "
            "%d[dec-T], %d[crc]\n", data[0], data[1], data[2], data[3], data[4]);
    } else {
        /* DHT22 */
        dbg_printf("Decoded data (DHT22): %d[hi-RH], %d[lo-RH], %d[hi-T], "
            "%d[lo-T], %d[crc]\n", data[0], data[1], data[2], data[3], data[4]);
    }

    for (i=0, crc=0; i<ARRAY_SZ(data)-1; i++) crc+=data[i];
    if (crc!=data[ARRAY_SZ(data)-1]) {
        char *log = "[%s] CRC doesn't match\n";
        if (!retried) err_printf(log, __func__);
        else warn_printf(log, __func__);

        ret = LREC_DTA_CRPT;
        goto finish;
    }

    if (model==dht11) {
        *p_rh = 10*(unsigned int)data[0] + (unsigned int)data[1];
        *p_temp = 10*(int)(int8_t)data[2] + (int)(int8_t)data[3];
    } else {
        /* DHT22 */
        *p_rh = ((unsigned int)data[0]<<8) | data[1];
        *p_temp = ((int)(int8_t)data[2]<<8) | data[3];
    }

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
    if (set_low) {
        gpio_opndrn_set_value(p_gpio_h, gpio, 1);
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t dht_probe(gpio_hndl_t *p_gpio_h, clock_hndl_t *p_clk_h,
    unsigned int gpio, dht_model_t model, unsigned int *p_rh, int *p_temp)
{
    return __dht_probe(p_gpio_h, p_clk_h, gpio, model, p_rh, p_temp, FALSE);
}

/* exported; see header for details */
lr_errc_t dht_probe_retried(gpio_hndl_t *p_gpio_h, clock_hndl_t *p_clk_h,
    unsigned int gpio, dht_model_t model, unsigned int n_retries,
    unsigned int *p_rh, int *p_temp)
{
    lr_errc_t ret=LREC_SUCCESS;
    unsigned int n_tries=0, cd_no_resp=10;

    /* probe retry loop */
    do {
        n_tries++;

        ret = __dht_probe(p_gpio_h, p_clk_h, gpio, model, p_rh, p_temp, TRUE);

        switch (ret)
        {
        default:
        case LREC_SUCCESS:
            goto finish;

        case LREC_NO_RESP:
            if (cd_no_resp && !--cd_no_resp) {
                err_printf("[%s] No sensor response\n", __func__);
                ret = LREC_NO_DEV;
                goto finish;
            }
            break;

        case LREC_DTA_CRPT:
            cd_no_resp=0;
            break;
        }
    } while (n_tries-1 < n_retries);

    err_printf("[%s] Exceeded number of sensor probe errors\n", __func__);
    ret = LREC_DEV_ERR;

finish:
    return ret;
}
