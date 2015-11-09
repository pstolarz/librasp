/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "librasp/bcm_platform.h"
#include "librasp/gpio.h"

#define	BCM_GPIO_MAP_LEN    PAGE_SZ

/* exported; see header for details */
void gpio_free(gpio_hndl_t *p_hndl)
{
    if (p_hndl->drv==gpio_drv_io) {
        if (p_hndl->bcm.p_gpio_io) {
            munmap((void*)p_hndl->bcm.p_gpio_io, BCM_GPIO_MAP_LEN);
            p_hndl->bcm.p_gpio_io = NULL;
        }
    }
}

/* exported; see header for details */
lr_errc_t gpio_init(gpio_hndl_t *p_hndl, gpio_driver_t drv)
{
    lr_errc_t ret=LREC_SUCCESS;

    memset(p_hndl, 0, sizeof(*p_hndl));
    p_hndl->drv = (gpio_driver_t)-1;

    switch (drv)
    {
    case gpio_drv_io:
      {
        uint32_t io_base = get_bcm_io_base();

        p_hndl->drv = gpio_drv_io;
        if (io_base) {
            if (!(p_hndl->bcm.p_gpio_io=
                    io_mmap(io_base+GPIO_BASE_RA, BCM_GPIO_MAP_LEN)))
                ret=LREC_MMAP_ERR;
        } else {
            err_printf("[%s] BCM platform not detected\n", __func__);
            ret=LREC_PLAT_ERR;
        }
        break;
      }

    case gpio_drv_sysfs:
        ret=LREC_NOT_SUPP;
        break;

    default:
        ret=LREC_INV_ARG;
        break;
    }

    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_bcm_get_func(
    gpio_hndl_t *p_hndl, unsigned int gpio, gpio_bcm_func_t *p_func)
{
    lr_errc_t ret=LREC_SUCCESS;

    gpio %= GPIO_NUM;
    if (p_hndl->drv==gpio_drv_io) {
        volatile uint32_t *p_gpfsel = IO_REG32_PTR(
            p_hndl->bcm.p_gpio_io, GPFSEL0+sizeof(uint32_t)*(gpio/10));
        *p_func = (gpio_bcm_func_t)((*p_gpfsel>>(3*(gpio%10)))&7);
    } else {
        ret=LREC_NOT_SUPP;
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_bcm_set_func(
    gpio_hndl_t *p_hndl, unsigned int gpio, gpio_bcm_func_t func)
{
    lr_errc_t ret=LREC_SUCCESS;

    gpio %= GPIO_NUM;
    if (p_hndl->drv==gpio_drv_io) {
        unsigned int shl = 3*(gpio%10);
        volatile uint32_t *p_gpfsel = IO_REG32_PTR(
            p_hndl->bcm.p_gpio_io, GPFSEL0+sizeof(uint32_t)*(gpio/10));
        *p_gpfsel = (volatile uint32_t)SET_BITFLD(
            *p_gpfsel, ((uint32_t)func&7)<<shl, (uint32_t)7<<shl);
    } else {
        ret=LREC_NOT_SUPP;
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_get_value(
    gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int *p_val)
{
    lr_errc_t ret=LREC_SUCCESS;

    gpio %= GPIO_NUM;
    if (p_hndl->drv==gpio_drv_io) {
        volatile uint32_t *p_gplev = IO_REG32_PTR(
            p_hndl->bcm.p_gpio_io, GPLEV0+sizeof(uint32_t)*(gpio>>5));
        *p_val = (unsigned int)((*p_gplev>>(gpio&0x1f))&1);
    } else {
        ret=LREC_NOT_SUPP;
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_set_value(gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int val)
{
    lr_errc_t ret=LREC_SUCCESS;

    gpio %= GPIO_NUM;
    if (p_hndl->drv==gpio_drv_io) {
        volatile uint32_t *p_gpsetclr = IO_REG32_PTR(p_hndl->bcm.p_gpio_io,
            (val&1 ? GPSET0 : GPCLR0)+sizeof(uint32_t)*(gpio>>5));
        *p_gpsetclr = (uint32_t)1<<(gpio&0x1f);
    } else {
        ret=LREC_NOT_SUPP;
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_direction_input(gpio_hndl_t *p_hndl, unsigned int gpio)
{
    lr_errc_t ret=LREC_SUCCESS;

    if (p_hndl->drv==gpio_drv_io) {
        gpio_bcm_set_func(p_hndl, gpio, gpio_bcm_in);
    } else {
        ret=LREC_NOT_SUPP;
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_direction_output(
    gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int val)
{
    lr_errc_t ret=LREC_SUCCESS;

    if (p_hndl->drv==gpio_drv_io) {
        gpio_bcm_set_func(p_hndl, gpio, gpio_bcm_out);
        gpio_set_value(p_hndl, gpio, val);
    } else {
        ret=LREC_NOT_SUPP;
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_bcm_set_pull_config(
    gpio_hndl_t *p_hndl, unsigned int gpio, gpio_pull_t pull)
{
    lr_errc_t ret=LREC_SUCCESS;

    gpio %= GPIO_NUM;
    if (p_hndl->drv==gpio_drv_io)
    {
        volatile uint32_t *p_gppud = IO_REG32_PTR(p_hndl->bcm.p_gpio_io, GPPUD);
        volatile uint32_t *p_gppudclk = IO_REG32_PTR(
            p_hndl->bcm.p_gpio_io, GPPUDCLK0+sizeof(uint32_t)*(gpio>>5));
        uint32_t gpio_bit = (uint32_t)1<<(gpio&0x1f);

        /* set the required control signal */
        *p_gppud = (volatile uint32_t)pull%3;
        WAIT_CYCLES(150);
        /* clock the control signal into the GPIO pad */
        *p_gppudclk = gpio_bit;
        WAIT_CYCLES(150);
        /* remove the control signal and the clock */
        *p_gppud = 0;
        *p_gppudclk = 0;
    } else {
        ret=LREC_NOT_SUPP;
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_set_event(
    gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int event)
{
    lr_errc_t ret=LREC_SUCCESS;

    gpio %= GPIO_NUM;
    if (p_hndl->drv==gpio_drv_io) {
#ifdef CONFIG_BCM_GPIO_EVENTS
        volatile uint32_t *p_reg;
        uint32_t gpio_bit = (uint32_t)1<<(gpio&0x1f);

# define __SET_P_REG(f,r) \
    p_reg = IO_REG32_PTR(p_hndl->bcm.p_gpio_io, (r)+sizeof(uint32_t)*(gpio>>5)); \
    if (event&(f)) *p_reg|=gpio_bit; else *p_reg&=~gpio_bit;

        __SET_P_REG(GPIO_EVENT_RAISING, GPREN0);
        __SET_P_REG(GPIO_EVENT_FALLING, GPFEN0);
        __SET_P_REG(GPIO_EVENT_BCM_HIGH, GPHEN0);
        __SET_P_REG(GPIO_EVENT_BCM_LOW, GPLEN0);
        __SET_P_REG(GPIO_EVENT_BCM_ARAISING, GPAREN0);
        __SET_P_REG(GPIO_EVENT_BCM_AFALLING, GPAFEN0);
# undef __SET_P_REG
#else
        ret=LREC_NOT_SUPP;
#endif
    } else {
        ret=LREC_NOT_SUPP;
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_get_event_stat(
    gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int *p_stat)
{
    lr_errc_t ret=LREC_SUCCESS;

    gpio %= GPIO_NUM;
    if (p_hndl->drv==gpio_drv_io) {
#ifdef CONFIG_BCM_GPIO_EVENTS
        volatile uint32_t *p_gpeds = IO_REG32_PTR(
            p_hndl->bcm.p_gpio_io, GPEDS0+sizeof(uint32_t)*(gpio>>5));
        *p_stat = (unsigned int)((*p_gpeds>>(gpio&0x1f))&1);
        /* clear status of the detected event */
        if (*p_stat) *p_gpeds = (uint32_t)1<<(gpio&0x1f);
#else
        ret=LREC_NOT_SUPP;
#endif
    } else {
        ret=LREC_NOT_SUPP;
    }
    return ret;
}
