/*
   Copyright (c) 2015,2016,2022 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdarg.h>
#include "config.h"
#include "librasp/common.h"

/* various printf utils */
void dbg_printf(const char *format, ...);
void info_printf(const char *format, ...);
void warn_printf(const char *format, ...);
void err_printf(const char *format, ...);

#define EXEC_RG(c) if ((ret=(c))!=LREC_SUCCESS) goto finish;
#define EXEC_G(c) if ((c)!=LREC_SUCCESS) goto finish;

/* for performance reason don't waste time for checking
   return code if it's guaranteed to be success */
#if CONFIG_CLOCK_SYS_DRIVER
# define EXECLK_RG(c) EXEC_RG(c)
# define EXECLK_G(c)  EXEC_G(c)
#else
# define EXECLK_RG(c) (c)
# define EXECLK_G(c)  (c)
#endif

#endif /* __COMMON_H__ */
