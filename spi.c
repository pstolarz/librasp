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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "common.h"
#include "librasp/spi.h"

/* exported; see header for details */
lr_errc_t spi_init(spi_hndl_t *p_hndl, int dev_no, int cs_no,
    int mode, bool_t lsb_first, int bits_per_word, unsigned speed_hz,
    unsigned delay_us, bool_t cs_change)
{
    lr_errc_t ret = LREC_SUCCESS;
    char dev_name[32];

    memset(p_hndl, 0, sizeof(*p_hndl));
    p_hndl->fd = -1;

    /* set default values */
    if (dev_no==SPI_USE_DEF) dev_no = 0;
    if (cs_no==SPI_USE_DEF) cs_no = 0;
    if (mode==SPI_USE_DEF) mode = SPI_MODE_0;
    if (lsb_first==(bool_t)SPI_USE_DEF) lsb_first = FALSE;
    if (bits_per_word==SPI_USE_DEF) bits_per_word = 8;
    if (speed_hz==(unsigned)SPI_USE_DEF) speed_hz = 1000000;    /* 1MHz */
    if (delay_us==(unsigned)SPI_USE_DEF) p_hndl->delay_us = 0;
    if (cs_change==(bool_t)SPI_USE_DEF) p_hndl->cs_change = FALSE;

    sprintf(dev_name, "/dev/spidev%d.%d", dev_no, cs_no);
    p_hndl->fd = open(dev_name, O_RDWR);

    if (p_hndl->fd == -1) {
        err_printf("open() SPI error: %s\n", strerror(errno));
        ret = LREC_OPEN_ERR;
        goto finish;
    }

    EXEC_RG(spi_set_mode(p_hndl, mode));
    EXEC_RG(spi_set_lsb(p_hndl, lsb_first));
    EXEC_RG(spi_set_bits_per_word(p_hndl, bits_per_word));
    EXEC_RG(spi_set_speed(p_hndl, speed_hz));

finish:
    if (ret!=LREC_SUCCESS) spi_free(p_hndl);
    return ret;
}

/* exported; see header for details */
void spi_free(spi_hndl_t *p_hndl)
{
    if (p_hndl->fd!=-1) close(p_hndl->fd);
    p_hndl->fd = -1;
}

/* exported; see header for details */
lr_errc_t spi_set_mode(spi_hndl_t *p_hndl, int mode)
{
    lr_errc_t ret = LREC_SUCCESS;
    uint8_t u8 = (uint8_t)mode;

    p_hndl->mode = mode;

    if(ioctl(p_hndl->fd, SPI_IOC_WR_MODE, &u8)==-1 ||
       ioctl(p_hndl->fd, SPI_IOC_RD_MODE, &u8)==-1)
    {
        err_printf("ioctl() SPI MODE error: %s\n", strerror(errno));
        ret = LREC_IOCTL_ERR;
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t spi_set_lsb(spi_hndl_t *p_hndl, bool_t lsb_first)
{
    lr_errc_t ret = LREC_SUCCESS;
    uint8_t u8 = (uint8_t)lsb_first;

    p_hndl->lsb_first = lsb_first;

    if(ioctl(p_hndl->fd, SPI_IOC_WR_LSB_FIRST, &u8)==-1 ||
       ioctl(p_hndl->fd, SPI_IOC_RD_LSB_FIRST, &u8)==-1)
    {
        err_printf("ioctl() SPI LSB_FIRST error: %s\n", strerror(errno));
        ret = LREC_IOCTL_ERR;
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t spi_set_bits_per_word(spi_hndl_t *p_hndl, int bits_per_word)
{
    lr_errc_t ret = LREC_SUCCESS;
    uint8_t u8 = (uint8_t)bits_per_word;

    p_hndl->bits_per_word = bits_per_word;

    if(ioctl(p_hndl->fd, SPI_IOC_WR_BITS_PER_WORD, &u8)==-1 ||
       ioctl(p_hndl->fd, SPI_IOC_RD_BITS_PER_WORD, &u8)==-1)
    {
        err_printf("ioctl() SPI BITS_PER_WORD error: %s\n", strerror(errno));
        ret = LREC_IOCTL_ERR;
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t spi_set_speed(spi_hndl_t *p_hndl, unsigned speed_hz)
{
    lr_errc_t ret = LREC_SUCCESS;
    uint32_t u32 = (uint32_t)speed_hz;

    p_hndl->speed_hz = speed_hz;

    if(ioctl(p_hndl->fd, SPI_IOC_WR_MAX_SPEED_HZ, &u32)==-1 ||
       ioctl(p_hndl->fd, SPI_IOC_RD_MAX_SPEED_HZ, &u32)==-1)
    {
        err_printf("ioctl() SPI MAX_SPEED_HZ error: %s\n", strerror(errno));
        ret = LREC_IOCTL_ERR;
    }
    return ret;
}

/* exported; see header for details */
lr_errc_t spi_transmit(spi_hndl_t *p_hndl, uint8_t *tx, uint8_t *rx, size_t len)
{
    lr_errc_t ret = LREC_SUCCESS;
    struct spi_ioc_transfer tr;

    memset(&tr, 0, sizeof(tr));

    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    tr.len = len;

    tr.speed_hz = p_hndl->speed_hz;
    tr.delay_usecs = p_hndl->delay_us;
    tr.bits_per_word = p_hndl->bits_per_word;
    tr.cs_change = p_hndl->cs_change;

    if (ioctl(p_hndl->fd, SPI_IOC_MESSAGE(1), &tr)==-1) {
        err_printf("ioctl() SPI transfer error: %s \n", strerror(errno));
        ret = LREC_IOCTL_ERR;
    }
    return ret;
}
