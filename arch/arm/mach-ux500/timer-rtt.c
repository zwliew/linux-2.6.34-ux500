/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 * Author: Rabin Vincent <rabin.vincent@stericsson.com>
 *
 */
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/clockchips.h>
#include <asm/smp.h>
#include <asm/mach/time.h>

#define RATE_32K	32768
#define LATCH_32K  	((RATE_32K + HZ/2) / HZ)

#define RTT_IMSC	0x04
#define RTT_ICR		0x10
#define RTT_DR		0x14
#define RTT_LR		0x18
#define RTT_CR		0x1C

#define RTT_CR_RTTEN	(1 << 1)
#define RTT_CR_RTTOS	(1 << 0)

#define RTC_IMSC	0x10
#define RTC_RIS		0x14
#define RTC_MIS		0x18
#define RTC_ICR		0x1C
#define RTC_TDR		0x20
#define	RTC_TLR1	0x24
#define RTC_TCR		0x28

#define RTC_TCR_RTTOS	(1 << 0)
#define RTC_TCR_RTTEN	(1 << 1)
#define RTC_TCR_RTTSS	(1 << 2)

#define RTC_IMSC_TIMSC	(1 << 1)
#define RTC_ICR_TIC	(1 << 1)

static void rtc_writel(unsigned long val, unsigned long addr)
{
	writel(val, IO_ADDRESS(U8500_RTC_BASE) + addr);
}

static unsigned long rtc_readl(unsigned long addr)
{
	return readl(IO_ADDRESS(U8500_RTC_BASE) + addr);
}

static void u8500_rtc_set_mode(enum clock_event_mode mode,
			       struct clock_event_device *evt)
{
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		rtc_writel(RTC_TCR_RTTSS, RTC_TCR);
		rtc_writel(LATCH_32K, RTC_TLR1);
		break;

	case CLOCK_EVT_MODE_ONESHOT:
		rtc_writel(0, RTC_TCR);
		break;

	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
		rtc_writel(0, RTC_TCR);
		break;

	case CLOCK_EVT_MODE_RESUME:
		break;

	}
}

#include <linux/delay.h>
static int u8500_rtc_set_event(unsigned long delta,
			       struct clock_event_device *dev)
{
	/* succesive writes to TCR require a delay of
	 * 1 32KHz cycle. so safely delaying 50
	 */
	rtc_writel(RTC_TCR_RTTOS, RTC_TCR);
	udelay(50);
	rtc_writel(delta, RTC_TLR1);
	udelay(50);
	rtc_writel(RTC_TCR_RTTOS | RTC_TCR_RTTEN, RTC_TCR);
	udelay(50);

	return 0;
}

static int u8500_rtc_interrupt(int irq, void *dev)
{
	struct clock_event_device *clkevt = dev;

	/* we make sure if this is our rtt isr */
	if (rtc_readl(RTC_MIS) & 0x2) {
		rtc_writel(RTC_ICR_TIC, RTC_ICR);
		clkevt->event_handler(clkevt);
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}

static struct clock_event_device u8500_rtc = {
	.name		= "rtc",
	.features	= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.shift		= 32,
	.rating		= 500,
	.set_next_event	= u8500_rtc_set_event,
	.set_mode	= u8500_rtc_set_mode,
	.irq		= IRQ_RTC_RTT,
	.cpumask	= cpu_all_mask,
};

static struct irqaction u8500_rtc_irq = {
	.name		= "rtc",
	.flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_SHARED,
	.handler	= u8500_rtc_interrupt,
	.dev_id		= &u8500_rtc,
};

void u8500_rtc_init(unsigned int cpu)
{
	rtc_writel(0, RTC_TCR);
	rtc_writel(1, RTC_ICR);
	rtc_writel(RTC_IMSC_TIMSC, RTC_IMSC);

	u8500_rtc.mult = div_sc(RATE_32K, NSEC_PER_SEC, u8500_rtc.shift);
	u8500_rtc.max_delta_ns = clockevent_delta2ns(0xffffffff, &u8500_rtc);
	u8500_rtc.min_delta_ns = clockevent_delta2ns(0xff, &u8500_rtc);

	setup_irq(IRQ_RTC_RTT, &u8500_rtc_irq);
	clockevents_register_device(&u8500_rtc);
}

static void rtt0_writel(unsigned long val, unsigned long addr)
{
	writel(val, IO_ADDRESS(U8500_RTT0_BASE) + addr);
}

static unsigned long rtt0_readl(unsigned long addr)
{
	return readl(IO_ADDRESS(U8500_RTT0_BASE) + addr);
}

static void u8500_rtt_set_mode(enum clock_event_mode mode,
			       struct clock_event_device *evt)
{
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		rtt0_writel(0, RTT_CR);
		rtt0_writel(LATCH_32K, RTT_LR);
		break;

	case CLOCK_EVT_MODE_ONESHOT:
		rtt0_writel(0, RTT_CR);
		break;

	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
		rtt0_writel(0, RTT_CR);
		break;

	case CLOCK_EVT_MODE_RESUME:
		break;
	}
}
