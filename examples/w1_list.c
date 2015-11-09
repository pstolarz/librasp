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
#include "librasp/w1.h"

#define MAX_MASTERS     32
#define MAX_SLAVES      128

int main(int argc, char **argv)
{
    w1_hndl_t w1_h;

    set_librasp_log_level(LRLOG_DEBUG);

    if (w1_init(&w1_h)==LREC_SUCCESS)
    {
        size_t n_masts, n_slavs, i, j;
        uint8_t masts_buf[get_w1_masters_bufsz(MAX_MASTERS)];
        uint8_t slavs_buf[get_w1_slaves_bufsz(MAX_SLAVES)];
        w1_masters_t *p_masts = (w1_masters_t*)masts_buf;
        w1_slaves_t *p_slavs = (w1_slaves_t*)slavs_buf;

        p_masts->max_sz = MAX_MASTERS;
        p_slavs->max_sz = MAX_SLAVES;

        if (list_w1_masters(&w1_h, p_masts, &n_masts)==LREC_SUCCESS)
        {
            printf("Total number of masters: %d\n", n_masts);
            for (i=0; i<p_masts->sz; i++)
            {
                printf(" 0x%08x\n", p_masts->ids[i]);

                if (search_w1_slaves(&w1_h,
                    p_masts->ids[i], p_slavs, &n_slavs)!=LREC_SUCCESS) continue;

                printf("  Total number of slaves: %d\n", n_slavs);
                for (j=0; j<p_slavs->sz; j++)
                    printf("   0x%016llx\n", p_slavs->ids[j]);
            }
        }

        w1_free(&w1_h);
    }

    return 0;
}
