/*
 * linux/arch/arm/mach-u8500/timer.c
 *
 * Copyright (C) 2008 STMicroelectronics
 * Copyright (C) 2009 Alessandro Rubini, somewhat based on at91sam926x
 * Copyright (C) 2009 ST-Ericsson SA
 *	added support to u8500 platform, heavily based on 8815
 * 	Author: Srinidhi KASAGAR <srinidhi.kasagar@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/clockchips.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/jiffies.h>
#include <asm/mach/time.h>
#include <mach/mtu.h>
#include <mach/setup.h>

#define TIMER_CTRL	0x80	/* No divisor */
#define TIMER_PERIODIC	0x40
#define TIMER_SZ32BIT	0x02

static u32	u8500_count;		/* accumulated count */
static u32	u8500_cycle;		/* write-once */
static __iomem void *mtu0_base;
static __iomem void *mtu1_base;

/*
 * clocksource: the MTU device is a decrementing counters, so we negate
 * the value being read.
 */
static cycle_t u8500_read_timer(void)
{
	u32 count = readl(mtu1_base + MTU_VAL(0));
	return ~count;
}

static void u8500_timer_reset(void);

static void u8500_clocksource_resume(void)
{
	u8500_timer_reset();
}

static struct clocksource u8500_clksrc = {
	.name		= "mtu_1",
	.rating		= 120,
	.read		= u8500_read_timer,
	.shift		= 20,
	.mask = CLOCKSOURCE_MASK(32),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
	.resume		= u8500_clocksource_resume,
};

/*
 * Clockevent device: currently only periodic mode is supported
 */
static void u8500_clkevt_mode(enum clock_event_mode mode,
			     struct clock_event_device *dev)
{
	unsigned long flags;

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		/* enable interrupts -- and count current value? */
		raw_local_irq_save(flags);
		writel(1, mtu0_base + MTU_IMSC);
		raw_local_irq_restore(flags);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		BUG(); /* Not yet supported */
		/* FALLTHROUGH */
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_UNUSED:
		/* disable irq */
		raw_local_irq_save(flags);
		writel(0, mtu0_base + MTU_IMSC);
		raw_local_irq_restore(flags);
		break;
	case CLOCK_EVT_MODE_RESUME:
		break;
	}
}

static struct clock_event_device u8500_clkevt = {
	.name		= "mtu_0",
	.features	= CLOCK_EVT_FEAT_PERIODIC,
	.shift		= 32,
	.rating		= 100,
	.set_mode	= u8500_clkevt_mode,
	.irq		= IRQ_MTU0,
};

/*
 * IRQ Handler for the timer 0 of the MTU block. The irq is not shared
 * as we are the only users of mtu0 by now.
 */
static irqreturn_t u8500_timer_interrupt(int irq, void *dev_id)
{
	/* ack: "interrupt clear register" */
	writel(1 << 0, mtu0_base + MTU_ICR);

	u8500_clkevt.event_handler(&u8500_clkevt);

	return IRQ_HANDLED;
}

/*
 * Set up timer interrupt, and return the current time in seconds.
 */
static struct irqaction u8500_timer_irq = {
	.name		= "U8500 Timer Tick",
	.flags		= IRQF_DISABLED | IRQF_TIMER,
	.handler	= u8500_timer_interrupt,
};

static void u8500_timer_reset(void)
{
	u32 cr;

	writel(0, mtu0_base + MTU_CR(0)); /* off */
	writel(0, mtu1_base + MTU_CR(0)); /* off */

	/* Timer: configure load and background-load, and fire it up */
	writel(u8500_cycle, mtu0_base + MTU_LR(0));
	writel(u8500_cycle, mtu0_base + MTU_BGLR(0));
	cr = MTU_CRn_PERIODIC | (MTU_CRn_PRESCALE_1 << 2) | MTU_CRn_32BITS;
	writel(cr, mtu0_base + MTU_CR(0));
	writel(cr | MTU_CRn_ENA, mtu0_base + MTU_CR(0));

	/* CS: configure load and background-load, and fire it up */
	writel(u8500_cycle, mtu1_base + MTU_LR(0));
	writel(u8500_cycle, mtu1_base + MTU_BGLR(0));
	cr = (MTU_CRn_PRESCALE_1 << 2) | MTU_CRn_32BITS;
	writel(cr, mtu1_base + MTU_CR(0));
	writel(cr | MTU_CRn_ENA, mtu1_base + MTU_CR(0));
}

static void __init u8500_timer_init(void)
{
	unsigned long rate;
	struct clk *clk0;
	struct clk *clk1;
	int bits;

#ifdef CONFIG_LOCAL_TIMERS
	twd_base = (void *)IO_ADDRESS(UX500_TWD_BASE);
#endif
	clk0 = clk_get_sys("mtu0", NULL);
	BUG_ON(IS_ERR(clk0));

	clk1 = clk_get_sys("mtu1", NULL);
	BUG_ON(IS_ERR(clk1));

	clk_enable(clk0);
	clk_enable(clk1);

	rate = clk_get_rate(clk0);
	u8500_cycle = (rate + HZ/2) / HZ;

	/* Save global pointer to mtu, used by functions above */
	if (cpu_is_u8500ed()) {
		mtu0_base = (void *)IO_ADDRESS(U8500_MTU0_BASE_ED);
		mtu1_base = (void *)IO_ADDRESS(U8500_MTU1_BASE_ED);
	} else {
		mtu0_base = (void *)IO_ADDRESS(UX500_MTU0_BASE);
		mtu1_base = (void *)IO_ADDRESS(UX500_MTU1_BASE);
	}

	/* Init the timer and register clocksource */
	u8500_timer_reset();

	/* register db8500-prcmu timer as always-on clock source */
	db8500_prcm_timer_init();

	u8500_clksrc.mult = clocksource_hz2mult(rate, u8500_clksrc.shift);
	bits =  8*sizeof(u8500_count);

	clocksource_register(&u8500_clksrc);

	/* Register irq and clockevents */
	setup_irq(IRQ_MTU0, &u8500_timer_irq);
	u8500_clkevt.mult = div_sc(rate, NSEC_PER_SEC, u8500_clkevt.shift);
	u8500_clkevt.cpumask = cpumask_of(0);
	clockevents_register_device(&u8500_clkevt);
	{
		extern void u8500_rtc_init(unsigned int cpu);
		u8500_rtc_init(0);
	}
}

static void u8500_timer_suspend(void)
{
	/* not supported yet */
}

struct sys_timer u8500_timer = {
	.init		= u8500_timer_init,
	.suspend	= u8500_timer_suspend,
	.resume		= u8500_timer_reset,
};
