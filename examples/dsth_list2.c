/*
   Copyright (c) 2015,2016,2022 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* List and probe all Dallas family sensors connected via 1-wire to the platform.

   This example demonstrates improved version of cumulative temperature read by
   sending single command to all therms to probe them, next reading the results
   one by one.
 */

#include <stdio.h>
#include <unistd.h>
#include "librasp/devices/ds_therm.h"

#define MAX_MASTERS     32
#define MAX_SLAVES      128

/* the example assumes common resolution for all probed sensors */
#define DSTH_RES        DSTH_RES_12BIT

/* Define if sensors are powered parasitically.

   NOTE1: Do not mix sensors of various powering mode (external vs parasite) on
   the same wire for this example (this is not the case for 'dsth_list' example).
   NOTE2: The library need to be configured with CONFIG_WRITE_PULLUP
   to support parasite powering.
 */
//#define PARASITE_POWER


int main(int argc, char **argv)
{
    w1_hndl_t w1_h;

#ifdef DEBUG
    set_librasp_log_level(LRLOG_DEBUG);
#endif

    if (w1_init(&w1_h)==LREC_SUCCESS)
    {
        size_t i, j;
        uint8_t masts_buf[get_w1_masters_bufsz(MAX_MASTERS)];
        uint8_t slavs_buf[get_w1_slaves_bufsz(MAX_SLAVES)];
        w1_masters_t *p_masts = (w1_masters_t*)masts_buf;
        w1_slaves_t *p_slavs = (w1_slaves_t*)slavs_buf;

        p_masts->max_sz = MAX_MASTERS;
        p_slavs->max_sz = MAX_SLAVES;

        if (list_w1_masters(&w1_h, p_masts, NULL)==LREC_SUCCESS)
        {
            for (i=0; i<p_masts->sz; i++)
            {
                unsigned int conv_time = dsth_get_conv_time(DSTH_RES);

                printf("w1 master [0x%08x]\n", p_masts->ids[i]);

#ifdef PARASITE_POWER
                if (dsth_convert_t_with_pullup_all(
                    &w1_h, p_masts->ids[i], conv_time)!=LREC_SUCCESS)
                    continue;
#else
                if (dsth_convert_t_all(&w1_h, p_masts->ids[i])!=LREC_SUCCESS)
                    continue;

                usleep(conv_time*1000);
#endif

                if (search_w1_slaves(&w1_h,
                    p_masts->ids[i], p_slavs, NULL)!=LREC_SUCCESS) continue;

                /* read the results loop */
                for (j=0; j<p_slavs->sz; j++)
                {
                    dsth_scratchpad_t scpd;
                    const char *dsth_name = dsth_name(p_slavs->ids[j]);

                    if (!dsth_name) continue;
                    printf(" %s [0x%016llx]; T:",
                        dsth_name, (long long unsigned)p_slavs->ids[j]);

                    if (dsth_read_scratchpad(
                        &w1_h, p_slavs->ids[j], &scpd)==LREC_SUCCESS)
                    {
                        int temp =
                            dsth_get_temp_scratchpad(p_slavs->ids[j], &scpd);
                        int temp_abs = (temp>=0 ? temp : -temp);

                        printf("%s%d.%03d\n",
                            (temp<0 ? "-" : ""), temp_abs/1000, temp_abs%1000);
                    } else
                        printf("???\n");
                }
            }
        }
        w1_free(&w1_h);
    }
    return 0;
}
