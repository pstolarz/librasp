#ifndef __CONFIG_H__
#define __CONFIG_H__

/* Add support for system clock driver. Usage of this driver is problematic
   due to its inaccuracy. */
#undef CONFIG_CLOCK_SYS_DRIVER

/* BCM's GPIO events support. Usage of these events is intended via kernel
   executives (interrupt specific functionality). BCM's events usage in the
   userland is problematic and may lead to unexpected results. */
#undef CONFIG_BCM_GPIO_EVENTS

/* 1-wire write w/ pullup support; requires "wire" kernel module patch. */
#undef CONFIG_WRITE_PULLUP

#endif /* __CONFIG_H__ */
