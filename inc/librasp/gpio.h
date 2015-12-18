/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __LR_GPIO_H__
#define __LR_GPIO_H__

#include "librasp/common.h"
#include "librasp/bcm_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _gpio_driver_t
{
    gpio_drv_io=0,
    gpio_drv_sysfs
} gpio_driver_t;

typedef struct _gpio_hndl_t
{
    /* active driver */
    gpio_driver_t drv;

    /* I/O driver */
    struct {
        volatile void *p_gpio_io;
    } io;

    /* SYSFS driver */
    struct {
        int valfds[GPIO_NUM];  /* GPIO values handles */
    } sysfs;
} gpio_hndl_t;

/* Initialize GPIO handle and set a given driver as active for the handle.
   NOTE: The initiated handle may be freely shared between all GPIO operations.
 */
lr_errc_t gpio_init(gpio_hndl_t *p_hndl, gpio_driver_t drv);

/* Activate GPIO driver for (already initialized) handle. Any subsequent GPIO API
   calls will be performed in a context of this driver.

   This function always success for SYSFS driver. For I/O driver may for the
   first-time call on I/O mapping error (LREC_MMAP_ERR). Once successes it will
   always success for the subsequent I/O driver activations.
 */
lr_errc_t gpio_set_driver(gpio_hndl_t *p_hndl, gpio_driver_t drv);

/* Free the GPIO handle.

   NOTE: The function doesn't unexport exported GPIOs for the SYSFS driver. It
   need to be done via gpio_sysfs_unexport() function.
 */
void gpio_free(gpio_hndl_t *p_hndl);

/* Set GPIO as an input/output with initial value 'val' for the output.
   The functions always success for the I/O driver. For SYSFS driver the function
   may fail if the requested GPIO has not been exported or due to sysfs access
   error.
 */
lr_errc_t gpio_direction_input(gpio_hndl_t *p_hndl, unsigned int gpio);
lr_errc_t gpio_direction_output(
    gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int val);

/* Get/set GPIO value (0|1). The functions always success for the I/O driver.
   For SYSFS driver the function may fail if the requested GPIO has not been set
   as input or output (LREC_NOINIT) or due to sysfs access error.

   NOTE: For BCM chip the GPIO high/low output values are represented by active
   3.3V OR shorting to the ground of a GPIO pin. Open-drain/source outputs may
   be emulated by proper pull up/down resistor configuration (connection) and
   gpio_opndrn_set_value(), gpio_opnsrc_set_value() macros usage.
 */
lr_errc_t gpio_get_value(
    gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int *p_val);
lr_errc_t gpio_set_value(
    gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int val);

/* Emulate open-drain output value (0|1) setting. In the high state the GPIO pin
   is configured as input and may be read. NOTE: Open drain emulation may be
   performed only for inputs with pull-up resistor connected (by configuration
   or physically).
 */
#define gpio_opndrn_set_value(hndl, gpio, val) \
    ((val)&1 ? gpio_direction_input((hndl), (gpio)) : \
    gpio_direction_output((hndl), (gpio), 0))

/* Emulate open-source output value (0|1) setting. In the low state the GPIO pin
   is configured as input and may be read. NOTE: Open source emulation may be
   performed only for inputs with pull-down resistor connected (by configuration
   or physically).
 */
#define gpio_opnsrc_set_value(hndl, gpio, val) \
    (!((val)&1) ? gpio_direction_input((hndl), (gpio)) : \
    gpio_direction_output((hndl), (gpio), 1))

#define GPIO_EVENT_NONE             0x0000
#define GPIO_EVENT_RAISING          0x0001
#define GPIO_EVENT_FALLING          0x0002
#define GPIO_EVENT_BOTH             (GPIO_EVENT_RAISING|GPIO_EVENT_FALLING)
/* BCM driver only */
#define GPIO_EVENT_BCM_HIGH         0x0004
#define GPIO_EVENT_BCM_LOW          0x0008
#define GPIO_EVENT_BCM_ARAISING     0x0010
#define GPIO_EVENT_BCM_AFALLING     0x0020

/* Set event detection provided as a bit field in the 'event' argument (OR'ed
   GPIO_EVENT_XXX values). If the bit field is 0 event detection is turned off.
   If configured the function always successes for the I/O driver. For SYSFS
   driver the function may fail if the requested GPIO has not been exported or
   due to sysfs access error.
 */
lr_errc_t gpio_set_event(
    gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int event);

/* Get BCM's event status (1: event detected, 0: event not detected) for a given
   GPIO. The functions must be called for I/O driver and always success in this
   case.
 */
lr_errc_t gpio_bcm_get_event_stat(
    gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int *p_stat);

/* BCM's GPIO functions specification. */
typedef enum _gpio_bcm_func_t
{
    gpio_bcm_in=0,  /* 0b000 */
    gpio_bcm_out,   /* 0b001 */
    gpio_bcm_alt5,  /* 0b010 */
    gpio_bcm_alt4,  /* 0b011 */
    gpio_bcm_alt0,  /* 0b100 */
    gpio_bcm_alt1,  /* 0b101 */
    gpio_bcm_alt2,  /* 0b110 */
    gpio_bcm_alt3,  /* 0b111 */
} gpio_bcm_func_t;

/* Get/set BCM's function assigned to a GPIO. The functions must be called for
   I/O driver and always success in this case.
 */
lr_errc_t gpio_bcm_get_func(
    gpio_hndl_t *p_hndl, unsigned int gpio, gpio_bcm_func_t *p_func);
lr_errc_t gpio_bcm_set_func(
    gpio_hndl_t *p_hndl, unsigned int gpio, gpio_bcm_func_t func);

/* BCM's GPIO input pull resistor configuration */
typedef enum _gpio_pull_t
{
    gpio_pull_off=0,  /* 0b00 */
    gpio_pull_down,   /* 0b01 */
    gpio_pull_up      /* 0b10 */
} gpio_pull_t;

/* Set pull resistor configuration for BCM's GPIO. The functions must be called
   for I/O driver and always success in this case.
 */
lr_errc_t gpio_bcm_set_pull_config(
    gpio_hndl_t *p_hndl, unsigned int gpio, gpio_pull_t pull);

/* These functions exports/unexports a GPIO for the SYSFS driver. Any GPIO MUST
   be exported via gpio_sysfs_export() before its usage with SYSFS driver.
   If the GPIO is no more needed the app should call gpio_sysfs_unexport() to
   return the GPIO sysfs resource to the system.

   The functions must be called for SYSFS driver.
 */
lr_errc_t gpio_sysfs_export(gpio_hndl_t *p_hndl, unsigned int gpio);
lr_errc_t gpio_sysfs_unexport(gpio_hndl_t *p_hndl, unsigned int gpio);

/* Poll for an event for 'gpio'. The event must be already set by function
   gpio_set_event(). The polling timeouts by 'timeout' milliseconds (infinite
   time if <0). If the event is detected LREC_SUCCESS is returned, LREC_TIMEOUT
   means timeout, other error informs about some other problem (e.g poll()
   error). The functions must be called for SYSFS driver.

   NOTE: It seems like BCM platform doesn't recognize between GPIO_EVENT_RAISING
   and GPIO_EVENT_FALLING events treating them the same. The only distinguished
   are: GPIO_EVENT_NONE and GPIO_EVENT_BOTH.
 */
lr_errc_t gpio_sysfs_poll(gpio_hndl_t *p_hndl, unsigned int gpio, int timeout);

#ifdef __cplusplus
}
#endif

#endif /* __LR_GPIO_H__ */
