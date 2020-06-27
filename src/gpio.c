/*
   Copyright (c) 2015,2016,2020 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "common.h"
#include "librasp/gpio.h"

#define	BCM_GPIO_MAP_LEN    PAGE_SZ

/* exported; see header for details */
lr_errc_t gpio_init(gpio_hndl_t *p_hndl, gpio_driver_t drv)
{
    int i;

    p_hndl->drv = (gpio_driver_t)-1;
    p_hndl->io.p_gpio_io = NULL;
    for (i=0 ; i<ARRAY_SZ(p_hndl->sysfs.valfds); i++)
        p_hndl->sysfs.valfds[i]=-1;

    return gpio_set_driver(p_hndl, drv);
}

/* exported; see header for details */
lr_errc_t gpio_set_driver(gpio_hndl_t *p_hndl, gpio_driver_t drv)
{
    lr_errc_t ret=LREC_SUCCESS;

    switch (drv)
    {
    default:
        drv = gpio_drv_io;
        /* fall-through */

    case gpio_drv_io:
    case gpio_drv_gpio:
      {
        if (!p_hndl->io.p_gpio_io)
        {
            uint32_t io_base = get_bcm_io_base();
            if (io_base) {
                if (!(p_hndl->io.p_gpio_io = io_mmap(
                    (drv == gpio_drv_io ? DEV_MEM_IO : DEV_MEM_GPIO),
                    io_base+GPIO_BASE_RA, BCM_GPIO_MAP_LEN)))
                {
                    ret=LREC_MMAP_ERR;
                }
            } else {
                err_printf("[%s] BCM platform not detected\n", __func__);
                ret=LREC_PLAT_ERR;
            }
        } else {
            /* already initialized I/O */
        }
        break;
      }

    case gpio_drv_sysfs:
        /* no initialization needed in this case */
        break;
    }

    if (ret==LREC_SUCCESS) p_hndl->drv = drv;
    return ret;
}

/* exported; see header for details */
void gpio_free(gpio_hndl_t *p_hndl)
{
    int i;

    if (p_hndl->drv!=(gpio_driver_t)-1)
    {
        /* free I/O driver resources */
        if (p_hndl->io.p_gpio_io) {
            munmap((void*)p_hndl->io.p_gpio_io, BCM_GPIO_MAP_LEN);
            p_hndl->io.p_gpio_io = NULL;
        }

        /* free SYSFS driver resources */
        for (i=0 ; i<ARRAY_SZ(p_hndl->sysfs.valfds); i++) {
            if (p_hndl->sysfs.valfds[i] != -1) {
                close(p_hndl->sysfs.valfds[i]);
                p_hndl->sysfs.valfds[i] = -1;
            }
        }

        /* mark the handle as closed */
        p_hndl->drv = (gpio_driver_t)-1;
    }
}

#define CHK_GPIO_NUM(n) \
    if ((n)<0 || (n)>=GPIO_NUM) { ret=LREC_INV_ARG; goto finish; }

/* exported; see header for details */
lr_errc_t gpio_bcm_get_func(
    gpio_hndl_t *p_hndl, unsigned int gpio, gpio_bcm_func_t *p_func)
{
    lr_errc_t ret=LREC_SUCCESS;

    CHK_GPIO_NUM(gpio);

    if (p_hndl->io.p_gpio_io) {
        volatile uint32_t *p_gpfsel = IO_REG32_PTR(
            p_hndl->io.p_gpio_io, GPFSEL0+sizeof(uint32_t)*(gpio/10));
        *p_func = (gpio_bcm_func_t)((*p_gpfsel>>(3*(gpio%10)))&7);
    } else
        ret=LREC_NOINIT;

finish:
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_bcm_set_func(
    gpio_hndl_t *p_hndl, unsigned int gpio, gpio_bcm_func_t func)
{
    lr_errc_t ret=LREC_SUCCESS;

    CHK_GPIO_NUM(gpio);

    if (p_hndl->io.p_gpio_io) {
        unsigned int shl = 3*(gpio%10);
        volatile uint32_t *p_gpfsel = IO_REG32_PTR(
            p_hndl->io.p_gpio_io, GPFSEL0+sizeof(uint32_t)*(gpio/10));
        *p_gpfsel = (volatile uint32_t)SET_BITFLD(
            *p_gpfsel, ((uint32_t)func&7)<<shl, (uint32_t)7<<shl);
    } else
        ret=LREC_NOINIT;

finish:
    return ret;
}

/* Set GPIO direction on sysfs. If GPIO value sysfs handle is not yet obtained -
   do it.
 */
static lr_errc_t
    sysfs_set_direction(gpio_hndl_t *p_hndl, unsigned int gpio, bool_t as_out)
{
    char buf[40];
    const char *dir_str;

    int valfd, dirfd=-1;
    lr_errc_t ret=LREC_SUCCESS;

    CHK_GPIO_NUM(gpio);
    valfd = p_hndl->sysfs.valfds[gpio];

    /* open sysfs GPIO file if not yet done */
    if (valfd == -1)
    {
        sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
        if ((valfd = p_hndl->sysfs.valfds[gpio] = open(buf, O_RDWR)) == -1)
        {
            err_printf("[%s] sysfs gpio-value open error: %d; %s\n"
                "Haven't forget to export the gpio?\n",
                __func__, errno, strerror(errno));
            ret=LREC_OPEN_ERR;
            goto finish;
        }
    }

    sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
    if ((dirfd = open(buf, O_WRONLY)) == -1)
    {
        err_printf("[%s] sysfs gpio-direction open error: %d; %s\n",
            __func__, errno, strerror(errno));
        ret=LREC_OPEN_ERR;
        goto finish;
    }

    dir_str = (as_out ? "out" : "in");
    if (write(dirfd, dir_str, strlen(dir_str)) == -1) {
        err_printf("[%s] sysfs gpio-direction write error: %d; %s\n",
            __func__, errno, strerror(errno));
        ret=LREC_WRITE_ERR;
    }

finish:
    if (dirfd != -1) close(dirfd);
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_direction_input(gpio_hndl_t *p_hndl, unsigned int gpio)
{
    lr_errc_t ret;

    if (p_hndl->drv==gpio_drv_io || p_hndl->drv==gpio_drv_gpio) {
        ret = gpio_bcm_set_func(p_hndl, gpio, gpio_bcm_in);
    } else {
        ret = sysfs_set_direction(p_hndl, gpio, FALSE);
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_direction_output(
    gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int val)
{
    lr_errc_t ret;

    if (p_hndl->drv==gpio_drv_io || p_hndl->drv==gpio_drv_gpio) {
        /* set value at first to avoid output blink */
        if ((ret=gpio_set_value(p_hndl, gpio, val))==LREC_SUCCESS)
            ret = gpio_bcm_set_func(p_hndl, gpio, gpio_bcm_out);
    } else {
        /* sysfs prevents setting a GPIO value before
           declaring its direction as an output */
        if ((ret=sysfs_set_direction(p_hndl, gpio, TRUE))==LREC_SUCCESS)
            ret = gpio_set_value(p_hndl, gpio, val);
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_get_value(
    gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int *p_val)
{
    lr_errc_t ret=LREC_SUCCESS;

    CHK_GPIO_NUM(gpio);

    if (p_hndl->drv==gpio_drv_io || p_hndl->drv==gpio_drv_gpio) {
        volatile uint32_t *p_gplev = IO_REG32_PTR(
            p_hndl->io.p_gpio_io, GPLEV0+sizeof(uint32_t)*(gpio>>5));
        *p_val = (unsigned int)((*p_gplev>>(gpio&0x1f))&1);
    } else
    {
        char c;
        int valfd = p_hndl->sysfs.valfds[gpio];

        if (valfd != -1) {
            if (lseek(valfd, 0, SEEK_SET)!=-1 && read(valfd, &c, 1) != -1)
            {
                *p_val = (unsigned int)!(c=='0');
            } else {
                err_printf("[%s] sysfs gpio-value read error: %d; %s\n",
                    __func__, errno, strerror(errno));
                ret=LREC_READ_ERR;
            }
        } else
            ret=LREC_NOINIT;
    }
finish:
    return ret;
}

/* exported; see header for details */
lr_errc_t
    gpio_set_value(gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int val)
{
    lr_errc_t ret=LREC_SUCCESS;

    CHK_GPIO_NUM(gpio);

    if (p_hndl->drv==gpio_drv_io || p_hndl->drv==gpio_drv_gpio) {
        volatile uint32_t *p_gpsetclr = IO_REG32_PTR(p_hndl->io.p_gpio_io,
            (val ? GPSET0 : GPCLR0)+sizeof(uint32_t)*(gpio>>5));
        *p_gpsetclr = (uint32_t)1<<(gpio&0x1f);
    } else
    {
        char c = (val ? '1' : '0');
        int valfd = p_hndl->sysfs.valfds[gpio];

        if (valfd != -1) {
            if (write(valfd, &c, 1) == -1)
            {
                err_printf("[%s] sysfs gpio-value write error: %d; %s\n",
                    __func__, errno, strerror(errno));
                ret=LREC_WRITE_ERR;
            }
        } else
            ret=LREC_NOINIT;
    }
finish:
    return ret;
}

/* Set GPIO event on sysfs.
 */
static lr_errc_t
    sysfs_set_event(gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int event)
{
    char buf[40];
    const char *ev_str;

    int evfd=-1;
    lr_errc_t ret=LREC_SUCCESS;

    CHK_GPIO_NUM(gpio);

    sprintf(buf, "/sys/class/gpio/gpio%d/edge", gpio);
    if ((evfd = open(buf, O_WRONLY)) == -1)
    {
        err_printf("[%s] sysfs gpio-edge open error: %d; %s\n"
            "Haven't forget to export the gpio?\n",
            __func__, errno, strerror(errno));
        ret=LREC_OPEN_ERR;
        goto finish;
    }

    if (event==GPIO_EVENT_NONE) {
        ev_str = "none";
    } else
    if ((event&GPIO_EVENT_RAISING)!=0 && (event&GPIO_EVENT_FALLING)!=0) {
        ev_str = "both";
    } else
    if ((event&GPIO_EVENT_RAISING)!=0) {
        ev_str = "rising";
    } else
    if ((event&GPIO_EVENT_FALLING)!=0) {
        ev_str = "falling";
    } else {
        ret=LREC_INV_ARG;
        goto finish;
    }

    if (write(evfd, ev_str, strlen(ev_str)) == -1) {
        err_printf("[%s] sysfs gpio-edge write error: %d; %s\n",
            __func__, errno, strerror(errno));
        ret=LREC_WRITE_ERR;
    }

finish:
    if (evfd != -1) close(evfd);
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_set_event(
    gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int event)
{
    lr_errc_t ret=LREC_SUCCESS;

    CHK_GPIO_NUM(gpio);

    if (p_hndl->drv==gpio_drv_io || p_hndl->drv==gpio_drv_gpio) {
#ifdef CONFIG_BCM_GPIO_EVENTS
        volatile uint32_t *p_reg;
        uint32_t gpio_bit = (uint32_t)1<<(gpio&0x1f);

# define __SET_P_REG(f,r) \
    p_reg = IO_REG32_PTR(p_hndl->io.p_gpio_io, (r)+sizeof(uint32_t)*(gpio>>5)); \
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
        ret = sysfs_set_event(p_hndl, gpio, event);
    }
finish:
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_bcm_get_event_stat(
    gpio_hndl_t *p_hndl, unsigned int gpio, unsigned int *p_stat)
{
    lr_errc_t ret=LREC_SUCCESS;

    CHK_GPIO_NUM(gpio);

    if (p_hndl->io.p_gpio_io)
    {
#ifdef CONFIG_BCM_GPIO_EVENTS
        volatile uint32_t *p_gpeds = IO_REG32_PTR(
            p_hndl->io.p_gpio_io, GPEDS0+sizeof(uint32_t)*(gpio>>5));
        *p_stat = (unsigned int)((*p_gpeds>>(gpio&0x1f))&1);
        /* clear status of the detected event */
        if (*p_stat) *p_gpeds = (uint32_t)1<<(gpio&0x1f);
#else
        ret=LREC_NOT_SUPP;
#endif
    } else {
        ret=LREC_NOINIT;
    }
finish:
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_bcm_set_pull_config(
    gpio_hndl_t *p_hndl, unsigned int gpio, gpio_bcm_pull_t pull)
{
    lr_errc_t ret=LREC_SUCCESS;

    CHK_GPIO_NUM(gpio);

    if (p_hndl->io.p_gpio_io)
    {
        volatile uint32_t *p_gppud = IO_REG32_PTR(p_hndl->io.p_gpio_io, GPPUD);
        volatile uint32_t *p_gppudclk = IO_REG32_PTR(
            p_hndl->io.p_gpio_io, GPPUDCLK0+sizeof(uint32_t)*(gpio>>5));
        uint32_t gpio_bit = (uint32_t)1<<(gpio&0x1f);

        /* set the required control signal */
        *p_gppud = (uint32_t)pull%3;
        WAIT_CYCLES(150);
        /* clock the control signal into the GPIO pad */
        *p_gppudclk = gpio_bit;
        WAIT_CYCLES(150);
        /* remove the control signal and the clock */
        *p_gppud = 0;
        *p_gppudclk = 0;
    } else {
        ret=LREC_NOINIT;
    }
finish:
    return ret;
}

/* Export/unexport GPIO on sysfs.
 */
static lr_errc_t
    sysfs_set_export(gpio_hndl_t *p_hndl, unsigned int gpio, bool_t export)
{
    int expfd=-1;
    lr_errc_t ret=LREC_SUCCESS;

    char buf[32];
    const char *expstr;

    CHK_GPIO_NUM(gpio);
    expstr = (export ? "export" : "unexport");

    sprintf(buf, "/sys/class/gpio/%s", expstr);
    if ((expfd = open(buf, O_WRONLY)) == -1)
    {
        err_printf("[%s] sysfs gpio-%s open error: %d; %s\n",
            __func__, expstr, errno, strerror(errno));
        ret=LREC_OPEN_ERR;
        goto finish;
    }

    if (write(expfd, buf, sprintf(buf, "%d", gpio))==-1 &&
        !((export && errno==EBUSY) || /* exporting already exported GPIO */
         (!export && errno==EINVAL))) /* unexporting already unexported GPIO */
    {
        err_printf("[%s] sysfs gpio-%s write error: %d; %s\n",
            __func__, expstr, errno, strerror(errno));
        ret=LREC_WRITE_ERR;
    }

finish:
    if (expfd != -1) close(expfd);
    return ret;
}

/* exported; see header for details */
lr_errc_t gpio_sysfs_export(gpio_hndl_t *p_hndl, unsigned int gpio)
{
    return sysfs_set_export(p_hndl, gpio, TRUE);
}

/* exported; see header for details */
lr_errc_t gpio_sysfs_unexport(gpio_hndl_t *p_hndl, unsigned int gpio)
{
    return sysfs_set_export(p_hndl, gpio, FALSE);
}

/* exported; see header for details */
lr_errc_t gpio_sysfs_poll(gpio_hndl_t *p_hndl, unsigned int gpio, int timeout)
{
    int plres, tmp;
    struct pollfd fds;
    lr_errc_t ret=LREC_SUCCESS;

    CHK_GPIO_NUM(gpio);

    fds.fd = p_hndl->sysfs.valfds[gpio];
    fds.events = POLLPRI;
    fds.revents = 0;

    if (fds.fd == -1) {
        ret=LREC_NOINIT;
        goto finish;
    }

    /* purge sysfs handle from pending data */
    lseek(fds.fd, 0, SEEK_SET);
    while (read(fds.fd, &tmp, sizeof(tmp))>0);

    plres = poll(&fds, 1, timeout);
    if (plres<0) {
        err_printf("[%s] poll() error: %d; %s\n",
            __func__, errno, strerror(errno));
        ret=LREC_POLL_ERR;
    } else
    if (!plres) {
        ret=LREC_TIMEOUT;
    }

finish:
    return ret;
}
