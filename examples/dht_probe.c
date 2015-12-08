/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* Command line utility to probe DHT 11/22 temperature sensor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librasp/bcm_platform.h"
#include "librasp/devices/dht.h"

/* configuration */
struct
{
    dht_model_t model;
    unsigned int gpio;
    gpio_pull_t pull;
    unsigned int n_retries;
    bool_t verbose;
} config =
{
    /* default configuration */
    .model = (dht_model_t)-1,
    .gpio = (unsigned int)-1,
    .pull = (gpio_pull_t)-1,
    .n_retries = (unsigned int)-1,
    .verbose = FALSE
};

/* get the configuration */
static bool_t get_config(int argc, char **argv)
{
    struct {
        unsigned int gpio : 1;
        unsigned int model : 1;
        unsigned int pull : 1;
        unsigned int verbose : 1;
        unsigned int n_retries : 1;
    } cfg_provided = {};

    int i;
    bool_t ret = FALSE;

    if (argc<=1) goto finish;
    for (i=1; i<argc; i++)
    {
        char *prm = argv[i];
        if (prm[0]=='-')
        {
            size_t prm_len = strlen(prm);

            if (prm_len<=1) goto finish;
            switch (prm[1])
            {
            case 'b':   /* verbose */
                if (cfg_provided.verbose || prm_len>2) goto finish;
                config.verbose = TRUE;
                cfg_provided.verbose++;
                break;

            case 'r':   /* num of retires */
                if (cfg_provided.n_retries || (prm_len<=2 && i+1>=argc))
                    goto finish;
                config.n_retries = (unsigned int)strtoul(
                    (prm_len>2 ? &prm[2] : argv[++i]), NULL, 0);
                cfg_provided.n_retries++;
                break;

            case 'p':   /* pull config */
                if (cfg_provided.pull || prm_len!=3) goto finish;
                if (prm[2]=='u') config.pull=gpio_pull_up;
                else
                if (prm[2]=='o') config.pull=gpio_pull_off;
                else goto finish;
                cfg_provided.pull++;
                break;

            case 'm':   /* model */
              {
                unsigned long model_int;

                if (cfg_provided.model || (prm_len<=2 && i+1>=argc))
                    goto finish;
                model_int = strtoul(
                    (prm_len>2 ? &prm[2] : argv[++i]), NULL, 0);
                if (model_int==11) config.model = dht11;
                else
                if (model_int==22) config.model = dht22;
                else goto finish;

                cfg_provided.model++;
                break;
              }

            default:
                goto finish;
            }
        } else {
            /* gpio pin */
            config.gpio = (unsigned int)strtoul(prm, NULL, 0);
            if ((config.gpio>=GPIO_NUM) || cfg_provided.gpio) goto finish;
            cfg_provided.gpio++;
        }
    }

    /* GPIO & the DHT model must be provided */
    if (cfg_provided.gpio && cfg_provided.model) ret=TRUE;

finish:
    if (!ret)
    {
        /* usage info */
        printf(
"DHT sensor test utility. Usage:\n"
"%s [-b] [-r retries] [-pu|-po] {-m 11|22} gpio\n"
"  -b: verbose output (including librasp debugging logging)\n"
"  -r: number of communication retries before fatal error (default: infinite)\n"
"  -p{u|o}: Set pull up|off GPIO config; if not specified: don't change (default)\n"
"  -m: DHT sensor model: 11 - DHT11 or 22 - DHT22\n"
"  gpio: GPIO number for communication with DHT sensor\n", argv[0]);
    }
    return ret;
}

int main(int argc, char **argv)
{
    int ret=1;  /* error */
    clock_hndl_t clk_h;
    gpio_hndl_t gpio_h;
    bool_t clk_inited=FALSE, gpio_inited=FALSE;

    int temp;
    unsigned int rh;

    if (!get_config(argc, argv) ||
        !(gpio_inited=(gpio_init(&gpio_h, gpio_drv_io)==LREC_SUCCESS)) ||
        !(clk_inited=(clock_init(&clk_h, clock_drv_io)==LREC_SUCCESS)))
            goto finish;

    set_librasp_log_level(config.verbose ? LRLOG_DEBUG : LRLOG_OFF);

    if (config.verbose) {
        printf("CONFIG: gpio:%u, model:%s, verbose:%u, retries:%u, pull:%s\n",
            config.gpio, (config.model==dht11 ? "DHT11" : "DHT22"),
            config.verbose, config.n_retries, (config.pull==gpio_pull_up ?
                "up" : (config.pull==gpio_pull_off ? "off" : "n/a")));
    }

    if (config.pull!=(gpio_pull_t)-1) {
        gpio_bcm_set_pull_config(&gpio_h, config.gpio, config.pull);
    }

    switch (dht_probe_retried(&gpio_h, &clk_h,
        config.gpio, config.model, config.n_retries, &rh, &temp))
    {
    case LREC_SUCCESS:
        printf("T:%d.%d RH:%d.%d\n",
            temp/10, (temp%10)*(temp<0 ? -1 : 1), rh/10, rh%10);
        /* success */
        ret=0;
        break;
    case LREC_NO_DEV:
        printf("INFO: No sensor connected\n");
        break;
    case LREC_DEV_ERR:
        printf("FATAL: Exceeded number of DHT sensor communication errors\n");
        break;
    default: break;
    }

finish:
    if (clk_inited) clock_free(&clk_h);
    if (gpio_inited) gpio_free(&gpio_h);

    return ret;
}
