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

/* Set SPI mode.
   ioctl() is used to set this parameter for the passed SPI handle.
 */
lr_errc_t spi_set_mode(spi_hndl_t *p_hndl, int mode);

/* Set LSB goes first specification.
   ioctl() is used to set this parameter for the passed SPI handle.
 */
lr_errc_t spi_set_lsb(spi_hndl_t *p_hndl, bool_t lsb_first);

/* Set SPI bits per word.
   ioctl() is used to set this parameter for the passed SPI handle.
 */
lr_errc_t spi_set_bits_per_word(spi_hndl_t *p_hndl, int bits_per_word);

/* Set SPI speed in Hz.
   ioctl() is used to set this parameter for the passed SPI handle.
 */
lr_errc_t spi_set_speed(spi_hndl_t *p_hndl, unsigned speed_hz);

/* SPI transmit.
   If a caller need to change SPI speed, bits per word, transmission delay or
   a device de-select specification only for a single transmission call (or
   group of calls) it need to change appropriate members of SPI handle pointed
   by 'p_hndl' before the spi_transmit() call (or calls) and restore them to
   the original values afterwards.

   SPI mode and LSB transfer specification can't be changed in this way (they
   require ioctl() call to set the params permanently for the SPI handle). Use
   spi_set_mode() and spi_set_lsb() API for this purpose.
 */
lr_errc_t spi_transmit(spi_hndl_t *p_hndl, uint8_t *tx, uint8_t *rx, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __LR_SPI_H__ */
