/*
 * Copyright (C) ST-Ericsson SA 2009
 * Author: Rabin Vincent <rabin.vincent@stericsson.com>
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef __ASM_ARCH_TIMEX_H
#define __ASM_ARCH_TIMEX_H

/*
 * This is incorrect for ED.  Don't use in platform code.
 *
 * In generic code, this is only used in include/linux/jiffies.h to calculate
 * ACTHZ.  The value of ACTHZ stays the same whether CLOCK_TICK_RATE is
 * 133.33Mhz or 100Mhz, so we leave it at 133.33Mhz.
 */
#define CLOCK_TICK_RATE		133330000

#endif
