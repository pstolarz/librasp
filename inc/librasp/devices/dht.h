/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __LR_DEVS_DHT_H__
#define __LR_DEVS_DHT_H__

#include "librasp/gpio.h"
#include "librasp/clock.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _dht_model_t {
    dht11=0,
    dht22
} dht_model_t;

/* Probe DHT sensor of a given model for temperature/humidity (written into
   'p_rh' and 'p_temp' respectively). Humidity is an unsigned 0-100 integer
   (denoted in percentages); temperature - signed integer denoted in Celsius.
   Both temperature and humidity values are scaled by 10 multiplier. GPIO and
   clock handlers are required to perform the probe.

   GPIO pre-call state:
       Data pin [in/out]: set high as an open-drain emulated output. In fact
       the high state is recommended (to limit the shorted circuit current)
       rather than required since the dht_probe() will initially reset the sensor
       by long shorting the data wire to the ground.
   GPIO post-call state:
       Data pin: set high as an open-drain emulated output.
 */
lr_errc_t dht_probe(gpio_hndl_t *p_gpio_h, clock_hndl_t *p_clk_h,
    unsigned int gpio, dht_model_t model, unsigned int *p_rh, int *p_temp);

/* Due to the significant number of erroneous probes (insufficient timings which
   can be obtained on the userland level), the function enables to specify a
   number of probe retries until an error will be reported.
 */
lr_errc_t dht_probe_retried(gpio_hndl_t *p_gpio_h, clock_hndl_t *p_clk_h,
    unsigned int gpio, dht_model_t model, unsigned int n_retries,
    unsigned int *p_rh, int *p_temp);

#ifdef __cplusplus
}
#endif

#endif /* __LR_DEVS_DHT_H__ */
