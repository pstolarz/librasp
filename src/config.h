/*
   Copyright (c) 2015,2021,2022 Piotr Stolarz
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
# define CONFIG_CLOCK_SYS_DRIVER 0
#endif

/* BCM's GPIO events support. Usage of these events is intended via kernel
   executives (interrupt specific functionality). BCM's events usage in the
   userland is problematic and may lead to unexpected results. */
#ifndef CONFIG_BCM_GPIO_EVENTS
# define CONFIG_BCM_GPIO_EVENTS 0
#endif

/* 1-wire write with pullup support; requires "wire" kernel module patch. */
#ifndef CONFIG_WRITE_PULLUP
# define CONFIG_WRITE_PULLUP 0
#endif

/* If a parameter is defined w/o value assigned, it is assumed as configured.
 */
#define __XEXT1(__prm) (1##__prm)
#define __EXT1(__prm) __XEXT1(__prm)

#ifdef CONFIG_CLOCK_SYS_DRIVER
# if (__EXT1(CONFIG_CLOCK_SYS_DRIVER) == 1)
#  undef CONFIG_CLOCK_SYS_DRIVER
#  define CONFIG_CLOCK_SYS_DRIVER 1
# endif
#endif

#ifdef CONFIG_BCM_GPIO_EVENTS
# if (__EXT1(CONFIG_BCM_GPIO_EVENTS) == 1)
#  undef CONFIG_BCM_GPIO_EVENTS
#  define CONFIG_BCM_GPIO_EVENTS 1
# endif
#endif

#ifdef CONFIG_WRITE_PULLUP
# if (__EXT1(CONFIG_WRITE_PULLUP) == 1)
#  undef CONFIG_WRITE_PULLUP
#  define CONFIG_WRITE_PULLUP 1
# endif
#endif

#undef __EXT1
#undef __XEXT1

#endif /* __CONFIG_H__ */
