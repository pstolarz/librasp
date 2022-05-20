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

   The therms are probed one by one. If SET_DSTH_RES is defined the temperature
   resolution may be set for each of the sensors.
 */

#include <stdio.h>
#include "librasp/devices/ds_therm.h"

#define MAX_MASTERS     32
#define MAX_SLAVES      128

/* define if want to set the temp. resolution for every DS sensor */
//#define SET_DSTH_RES    DSTH_RES_12BIT

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
                printf("w1 master [0x%08x]\n", p_masts->ids[i]);

                if (search_w1_slaves(&w1_h,
                    p_masts->ids[i], p_slavs, NULL)!=LREC_SUCCESS) continue;
                for (j=0; j<p_slavs->sz; j++)
                {
                    int temp;
                    const char *dsth_name = dsth_name(p_slavs->ids[j]);

                    if (!dsth_name) continue;
                    printf(" %s [0x%016llx]; T:",
                        dsth_name, (long long unsigned)p_slavs->ids[j]);

#ifdef SET_DSTH_RES
                    dsth_set_res(&w1_h, p_slavs->ids[j], SET_DSTH_RES);
#endif
                    if (dsth_probe(&w1_h, p_slavs->ids[j], &temp)==LREC_SUCCESS)
                    {
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
