/*
 * Copyright (C) 2009 ST Ericsson.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef ASMARM_ARCH_SMP_H
#define ASMARM_ARCH_SMP_H


#include <asm/hardware/gic.h>

#define hard_smp_processor_id()			\
	({						\
		unsigned int cpunum;			\
		__asm__("mrc p15, 0, %0, c0, c0, 5"	\
			: "=r" (cpunum));		\
		cpunum &= 0x0F;				\
	})

/*
 * We use IRQ1 as the IPI
 */
static inline void smp_cross_call(cpumask_t callmap)
{
	gic_raise_softirq(callmap, 1);
}

/*
 * Do nothing on MPcore.
 */
static inline void smp_cross_call_done(cpumask_t callmap)
{
}

#endif
