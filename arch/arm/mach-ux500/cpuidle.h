/*
 * Copyright (C) STMicroelectronics 2009
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 * Author: Sundar Iyer <sundar.iyer@stericsson.com>
 *
 * CPUIdle header for U8500
 */
#ifndef __U8500_CPUIDLE_H
#define __U8500_CPUIDLE_H

#include <linux/cpuidle.h>

#define U8500_NUM_CSTATES  3

#define U8500_CSTATE_C0 0	/* running */
#define U8500_CSTATE_C1 1	/* wfi */
#define U8500_CSTATE_C2 2	/* wfi or wfi retention */

/* this is the c-state template structure used to populate the cpuidle
 * c-states for all cpu's */

struct u8500_cstate {
	u8 type;
	u32 sleep_latency;
	u32 wakeup_latency;
	u32 threshold;
	u32 power_usage;
	u32 flags;
	char desc[CPUIDLE_DESC_LEN];
};

#endif
