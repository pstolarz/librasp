/*
   Copyright (c) 2015,2016 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */
#ifndef __LR_SPI_H__
#define __LR_SPI_H__

#include <linux/spi/spidev.h>
#include "librasp/common.h"

#ifdef __cplusplus
{
#endif

#define SPI_USE_DEF     -1

typedef struct _spi_hndl_t
{
    /* opened for a given SPI master device and slave */
    int fd;

    int mode;
    bool_t lsb_first;
    int bits_per_word;
    unsigned speed_hz;
    unsigned delay_us;
    bool_t cs_change;
} spi_hndl_t;

/* Initialize SPI handle and write it under 'p_hndl'.
   SPI_USE_DEF may be used for all params to use default values as follows:
       dev no:      0,
       CS no:       0,
       mode:        0,
       LSB first:   FALSE,
       bits/word:   8,
       speed:       1MHz,
       delay:       0 us,
       CS change:   FALSE.
 */
lr_errc_t spi_init(spi_hndl_t *p_hndl, int dev_no, int cs_no,
    int mode, bool_t lsb_first, int bits_per_word, unsigned speed_hz,
    unsigned delay_us, bool_t cs_change);

/* Free SPI handle. */
void spi_free(spi_hndl_t *p_hndl);

/* Set SPI mode for the SPI handle.

   NOTE: SPI related ioctl(2) is used to set the parameter in the system
   (therefore an error may occur).
 */
lr_errc_t spi_set_mode(spi_hndl_t *p_hndl, int mode);

/* Set LSB goes first specification for the SPI handle.

   NOTE: SPI related ioctl(2) is used to set the parameter in the system
   (therefore an error may occur).
 */
lr_errc_t spi_set_lsb(spi_hndl_t *p_hndl, bool_t lsb_first);

/* Set SPI bits per word for the SPI handle.

   If 'with_ioctl' is TRUE, SPI related ioctl(2) is used to set the parameter
   in the system (therefore an error may occur).
   If FALSE, the function merely modifies the passed handle to correspond to
   the passed parameter (therefore always successes). The updated parameter
   will be used by the subsequent spi_transmit() calls, therefore any problem
   with the parameter will be recognized at that stage.
 */
lr_errc_t spi_set_bits_per_word(
    spi_hndl_t *p_hndl, int bits_per_word, bool_t with_ioctl);

/* Set SPI speed in Hz for the SPI handle.
   'with_ioctl' has the same meaning as for spi_set_bits_per_word().
 */
lr_errc_t spi_set_speed(
    spi_hndl_t *p_hndl, unsigned speed_hz, bool_t with_ioctl);

/* Set transmission delay (usecs) for the SPI handle macro.
   The updated parameter will be used by the subsequent spi_transmit() calls,
   therefore any problem with the parameter will be recognized at that stage.

   NOTE: Always successes, no result code is returned.
 */
#define spi_set_delay(hndl, delay) (hndl)->delay_us = (delay)

/* Set CS-change flag for the SPI handle macro.
   The updated parameter will be used by the subsequent spi_transmit() calls,
   therefore any problem with the parameter will be recognized at that stage.

   NOTE: Always successes, no result code is returned.
 */
#define spi_set_cs_change(hndl, cschng) (hndl)->cs_change = (cschng)

/* SPI transmit.
 */
lr_errc_t spi_transmit(spi_hndl_t *p_hndl, uint8_t *tx, uint8_t *rx, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __LR_SPI_H__ */
