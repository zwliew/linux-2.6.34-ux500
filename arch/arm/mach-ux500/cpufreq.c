/*
 * CPU frequency module for U8500
 *
 * Copyright 2009 STMicroelectronics.
 * Copyright 2009 ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */
#include <linux/cpufreq.h>
#include <linux/delay.h>

#include <mach/prcmu-fw-api.h>
#include <mach/prcmu-regs.h>

static struct cpufreq_frequency_table freq_table[] = {
	{
		.index = 0,
		.frequency = 200000,
	},
	{
		.index = 1,
		.frequency = 300000,
	},
	{
		.index = 2,
		.frequency = 600000,
	},
	{
		.index = 3,
		.frequency = CPUFREQ_TABLE_END
	}
};

static struct freq_attr *u8500_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

int u8500_verify_speed(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy, freq_table);
}

static int u8500_target(struct cpufreq_policy *policy, unsigned int target_freq,
			unsigned int relation)
{
	struct cpufreq_freqs freqs;
	unsigned int idx;
	enum arm_opp_t op;


	/* scale the target frequency to one of the extremes supported */
	if (target_freq < policy->cpuinfo.min_freq)
		target_freq = policy->cpuinfo.min_freq;
	if (target_freq > policy->cpuinfo.max_freq)
		target_freq = policy->cpuinfo.max_freq;

	/* Lookup the next frequency */
	if (cpufreq_frequency_table_target
	    (policy, freq_table, target_freq, relation, &idx)) {
		return -EINVAL;
	}

	freqs.old = policy->cur;
	freqs.new = freq_table[idx].frequency;
	freqs.cpu = policy->cpu;

	/* pre-change notification */
	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	switch (idx) {
	case 0:
		op = ARM_EXTCLK; break;
	case 1:
		op = ARM_50_OPP; break;
	case 2:
		op = ARM_100_OPP; break;
	default:
		 printk(KERN_INFO "u8500-cpufreq : Error in OPP\n");
		 return -EINVAL;
	}

	/* request the PRCM unit for opp change */
	if (prcmu_set_arm_opp(op))
		return -EINVAL;

	/* post change notification */
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	return 0;
}

unsigned int u8500_getspeed(unsigned int cpu)
{
	/* request the prcm to get the current ARM opp */
	enum arm_opp_t opp = prcmu_get_arm_opp();

	switch (opp) {
	case ARM_EXTCLK: return freq_table[0].frequency;
	case ARM_50_OPP: return freq_table[1].frequency;
	case ARM_100_OPP: return freq_table[2].frequency;
	default:
			  return -EINVAL;
	}
	return 0;
}

static int __cpuinit u8500_cpu_init(struct cpufreq_policy *policy)
{
	int res;

	/* get policy fields based on the table */
	res = cpufreq_frequency_table_cpuinfo(policy, freq_table);
	if (!res)
		cpufreq_frequency_table_get_attr(freq_table, policy->cpu);
	else {
		printk(KERN_INFO "u8500-cpufreq : Null CPUFreq Tables!!!\n");
		return -EINVAL;
	}

	policy->min = policy->cpuinfo.min_freq;
	policy->max = policy->cpuinfo.max_freq;
	policy->cur = u8500_getspeed(policy->cpu);

	policy->governor = CPUFREQ_DEFAULT_GOVERNOR;

	/* FIXME : Need to take time measurement across the target()
	 *	   function with no/some/all drivers in the notification
	 *	   list.
	 */
	policy->cpuinfo.transition_latency = 200 * 1000; /* in ns */

	/* policy sharing between dual CPUs */
	cpumask_copy(policy->cpus, &cpu_present_map);

	policy->shared_type = CPUFREQ_SHARED_TYPE_ALL;

	return res;
}

static int u8500_cpu_exit(struct cpufreq_policy *policy)
{
	return 0;
}

static struct cpufreq_driver u8500_driver = {
	.flags = CPUFREQ_STICKY,
	.verify = u8500_verify_speed,
	.target = u8500_target,
	.get = u8500_getspeed,
	.init = u8500_cpu_init,
	.exit = u8500_cpu_exit,
	.name = "U8500",
	.attr = u8500_cpufreq_attr,
};

static int __init u8500_cpufreq_init(void)
{
	return cpufreq_register_driver(&u8500_driver);
}

device_initcall(u8500_cpufreq_init);
