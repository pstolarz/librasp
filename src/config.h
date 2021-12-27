/*
   Copyright (c) 2015,2021 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

/* Add support for system clock driver. Usage of this driver is problematic
   due to its inaccuracy. */
#ifndef CONFIG_CLOCK_SYS_DRIVER
//# define CONFIG_CLOCK_SYS_DRIVER
#endif

/* BCM's GPIO events support. Usage of these events is intended via kernel
   executives (interrupt specific functionality). BCM's events usage in the
   userland is problematic and may lead to unexpected results. */
#ifndef CONFIG_BCM_GPIO_EVENTS
//# define CONFIG_BCM_GPIO_EVENTS
#endif

/* 1-wire write with pullup support; requires "wire" kernel module patch. */
#ifndef CONFIG_WRITE_PULLUP
//# define CONFIG_WRITE_PULLUP
#endif

/* Fast CRC calculation (using lookup table). */
#ifndef CONFIG_FAST_CRC
# define CONFIG_FAST_CRC
#endif

#endif /* __CONFIG_H__ */
