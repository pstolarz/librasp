/*
   Copyright (c) 2015,2016 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __LR_PLATFORM_H__
#define __LR_PLATFORM_H__

#define GPIO_NUM    54

#define PAGE_SZ     0x1000

/* The STC is a free running counter that increments at the rate of 1MHz */
#define STC_FREQ_HZ         1000000

/* I/O base real addresses */
#define BCM2708_PERI_BASE   0x20000000
#define BCM2709_PERI_BASE   0x3f000000

/* Real relative base addresses of the I/O peripherals
 */
/* Fake frame buffer device (actually the multicore sync block */
#define MCORE_BASE_RA       0x0000
#define IC0_BASE_RA         0x2000
/* System Timer */
#define ST_BASE_RA          0x3000
/* Message -based Parallel Host Interface */
#define MPHI_BASE_RA        0x6000
/* DMA controller */
#define DMA_BASE_RA         0x7000
/* ARM control block */
#define ARM_BASE_RA         0xb000
/* Power Management, Reset controller and Watchdog registers */
#define PM_BASE_RA          0x100000
/* PCM Clock */
#define PCM_CLOCK_BASE_RA   0x101098
/* Hardware RNG */
#define RNG_BASE_RA         0x104000
/* GPIO */
#define GPIO_BASE_RA        0x200000
/* Uart 0 */
#define UART0_BASE_RA       0x201000
/* MMC interface */
#define MMCI0_BASE_RA       0x202000
/* I2S */
#define I2S_BASE_RA         0x203000
/* SPI0 */
#define SPI0_BASE_RA        0x204000
/* BSC0 I2C/TWI */
#define BSC0_BASE_RA        0x205000
/* Uart 1 */
#define UART1_BASE_RA       0x215000
/* eMMC interface */
#define EMMC_BASE_RA        0x300000
/* SMI */
#define SMI_BASE_RA         0x600000
/* BSC1 I2C/TWI */
#define BSC1_BASE_RA        0x804000
/* DTC_OTG USB controller */
#define USB_BASE_RA         0x980000

/* GPIO regs
 */
/* GPIO Function Select 0 */
#define GPFSEL0             0x0000
/* GPIO Function Select 1 */
#define GPFSEL1             0x0004
/* GPIO Function Select 2 */
#define GPFSEL2             0x0008
/* GPIO Function Select 3 */
#define GPFSEL3             0x000c
/* GPIO Function Select 4 */
#define GPFSEL4             0x0010
/* GPIO Function Select 5 */
#define GPFSEL5             0x0014
/* GPIO Pin Output Set 0 */
#define GPSET0              0x001c
/* GPIO Pin Output Set 1 */
#define GPSET1              0x0020
/* GPIO Pin Output Clear 0 */
#define GPCLR0              0x0028
/* GPIO Pin Output Clear 1 */
#define GPCLR1              0x002c
/* GPIO Pin Level 0 */
#define GPLEV0              0x0034
/* GPIO Pin Level 1 */
#define GPLEV1              0x0038
/* GPIO Pin Event Detect Status 0 */
#define GPEDS0              0x0040
/* GPIO Pin Event Detect Status 1 */
#define GPEDS1              0x0044
/* GPIO Pin Rising Edge Detect Enable 0 */
#define GPREN0              0x004c
/* GPIO Pin Rising Edge Detect Enable 1 */
#define GPREN1              0x0050
/* GPIO Pin Falling Edge Detect Enable 0 */
#define GPFEN0              0x0058
/* GPIO Pin Falling Edge Detect Enable 1 */
#define GPFEN1              0x005c
/* GPIO Pin High Detect Enable 0 */
#define GPHEN0              0x0064
/* GPIO Pin High Detect Enable 1 */
#define GPHEN1              0x0068
/* GPIO Pin Low Detect Enable 0 */
#define GPLEN0              0x0070
/* GPIO Pin Low Detect Enable 1 */
#define GPLEN1              0x0074
/* GPIO Pin Async. Rising Edge Detect 0 */
#define GPAREN0             0x007c
/* GPIO Pin Async. Rising Edge Detect 1 */
#define GPAREN1             0x0080
/* GPIO Pin Async. Falling Edge Detect 0 */
#define GPAFEN0             0x0088
/* GPIO Pin Async. Falling Edge Detect 1 */
#define GPAFEN1             0x008c
/* GPIO Pin Pull-up/down Enable */
#define GPPUD               0x0094
/* GPIO Pin Pull-up/down Enable Clock 0 */
#define GPPUDCLK0           0x0098
/* GPIO Pin Pull-up/down Enable Clock 1 */
#define GPPUDCLK1           0x009c

/* System Timer Counter (STC) regs
 */
/* STC control/status */
#define ST_CS               0x0000
/* STC lower 32 bits */
#define ST_CLO              0x0004
/* STC Upper 32 bits */
#define ST_CHI              0x0008

#endif /* __LR_PLATFORM_H__ */
