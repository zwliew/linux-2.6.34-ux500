/*
 *  linux/arch/arm/mach-realview/hotplug.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/smp.h>
#include <linux/completion.h>

#include <asm/cacheflush.h>

#include "pm.h"

extern volatile int pen_release;

static DECLARE_COMPLETION(cpu_killed);

static inline void platform_do_lowpower(unsigned int cpu)
{
	flush_cache_all();

	/*
	 * backup cpu specific context
	 * one woken upm we wud have restored
	 * our cpu specific context
	 */
	/* for SVP builds. SVP doesnt incorporate
	 * CONFIG_PM yet!
	 */
#ifdef CONFIG_PM
	ux500_cpu_context_deepsleep(cpu);
#else
	__asm__ __volatile__("dsb\n\t" "wfi\n\t" \
			: : : "memory");
#endif

	for (;;) {
		/*
		 * whiling around till some one releases
		 * the holding pen; if true, v r done!
		 */
		if (pen_release == cpu)
			break;
	}
}

int platform_cpu_kill(unsigned int cpu)
{
	return wait_for_completion_timeout(&cpu_killed, 5000);
}

/*
 * platform-specific code to shutdown a CPU
 *
 * Called with IRQs disabled
 */
void platform_cpu_die(unsigned int cpu)
{
#ifdef DEBUG
	unsigned int this_cpu = hard_smp_processor_id();

	if (cpu != this_cpu) {
		printk(KERN_CRIT "Eek! platform_cpu_die running on %u, should be %u\n",
			   this_cpu, cpu);
		BUG();
	}
#endif

	printk(KERN_NOTICE "CPU%u: shutdown\n", cpu);
	complete(&cpu_killed);

	platform_do_lowpower(cpu);
}

int mach_cpu_disable(unsigned int cpu)
{
	/*
	 * we don't allow CPU 0 to be shutdown (it is still too special
	 * e.g. clock tick interrupts)
	 */
	return cpu == 0 ? -EPERM : 0;
}
