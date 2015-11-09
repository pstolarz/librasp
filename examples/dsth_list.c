/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <stdio.h>
#include "librasp/devices/ds_therm.h"

#define MAX_MASTERS     32
#define MAX_SLAVES      128

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
                    printf(" %s [0x%016llx]; T:", dsth_name, p_slavs->ids[j]);

                    /*
                    dsth_set_res(&w1_h, p_slavs->ids[j], DSTH_RES_12BIT);
                     */

                    if (dsth_probe(&w1_h, p_slavs->ids[j], &temp)==LREC_SUCCESS)
                    {
                        printf("%d.%d\n",
                            temp/1000, (temp%1000)*(temp<0 ? -1 : 1));
                    } else
                        printf("???\n");
                }
            }
        }
        w1_free(&w1_h);
    }
    return 0;
}
