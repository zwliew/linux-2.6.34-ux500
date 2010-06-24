/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 * Author: Sundar Iyer <sundar.iyer@stericsson.com> for ST-Ericsson
 *
 * DB8500-PRCM Timer
 * The PRCM has 5 timers which are available in a always-on
 * power domain. we use the Timer 5 for our always-on clock source.
 */
#include <linux/io.h>
#include <linux/clockchips.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <mach/setup.h>
#include <mach/prcmu-regs.h>

#define RATE_32K		(32768)

#define TIMER_MODE_CONTINOUS	(0x1)
#define TIMER_DOWNCOUNT_VAL	(0xffffffff)

/*
 * clocksource: the prcm timer is a decrementing counters, so we negate
 * the value being read.
 */
static cycle_t db8500_prcm_read_timer(void)
{
	u32 count = readl(PRCM_TIMER_5_DOWNCOUNT);
	return ~count;
}

static struct clocksource db8500_prcm_clksrc = {
	.name           = "db8500-prcm-timer5",
	.rating         = 500,
	.read           = db8500_prcm_read_timer,
	.shift          = 10,
	.mask           = CLOCKSOURCE_MASK(32),
	.flags          = CLOCK_SOURCE_IS_CONTINUOUS,
};

void __init db8500_prcm_timer_init(void)
{
	/*
	 * The A9 sub system expects the timer to be configured as
	 * a continous looping timer. If this timer is not configured
	 * in such a mode, then do so.
	 * Later on, when the PRCM decides to init this timer, the A9
	 * can rest assure that it need not init the timer
	 */
	if (readl(PRCM_TIMER_5_MODE) != TIMER_MODE_CONTINOUS) {
		writel(TIMER_MODE_CONTINOUS, PRCM_TIMER_5_MODE);
		writel(TIMER_DOWNCOUNT_VAL, PRCM_TIMER_5_REF);
	}

	/* register the clock source */
	db8500_prcm_clksrc.mult = clocksource_hz2mult(RATE_32K,
			db8500_prcm_clksrc.shift);

	clocksource_register(&db8500_prcm_clksrc);

	return;
}

