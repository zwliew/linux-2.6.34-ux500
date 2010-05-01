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
#include <asm/io.h>

#include <mach/prcmu-fw-api.h>
#include <mach/prcmu-regs.h>

/* NOTES: uncomment this to use the PRCMU api */
#define USE_PRCMU_OP

#ifdef USE_PRCMU_OP
static struct cpufreq_frequency_table freq_table[] = {
	{
	 .index = 0,
	 .frequency = 300000,
	 },

	{
	 .index = 1,
	 .frequency = 600000,
	 },

	{
	 .index = 2,
	 .frequency = CPUFREQ_TABLE_END}
};
#else /* use pll post scaler, assuming max freq = 450Mhz */
#define _PLLDIVPS_LATENCY_US 50
static struct cpufreq_frequency_table freq_table[] = {
	{
	 .index = 0,
	 .frequency = 56250,	/* 450/8 */
	 },

	{
	 .index = 1,
	 .frequency = 112500,	/* 450/4 */
	 },

	{
	 .index = 2,
	 .frequency = 225000,	/* 450/2 */
	 },

	{
	 .index = 3,
	 .frequency = 450000,	/* 450/1 */
	 },

	{
	 .index = 4,
	 .frequency = CPUFREQ_TABLE_END}
};

#define PLLDIVPS_NUM_DIVIDERS 4
static int _u8500_plldivps_map[PLLDIVPS_NUM_DIVIDERS] = {
	0xf, 0x7, 0x3, 0x1
};

static void set_plldivps(int end)
{
	int cur, i, start;

	cur = readl(PRCM_ARM_PLLDIVPS);

	if (_u8500_plldivps_map[end] == cur)
		return;

	if (cur == 0xf)
		start = 0;
	if (cur == 0x7)
		start = 1;
	if (cur == 0x3)
		start = 2;
	if (cur == 0x1)
		start = 3;

	if (start < end)
		start++;
	else
		start--;

	for (i = start;; (start < end) ? i++ : i--) {
		writel(_u8500_plldivps_map[i], PRCM_ARM_PLLDIVPS);
		udelay(_PLLDIVPS_LATENCY_US);
		if (i == end)
			break;
	}
}

#endif

static struct freq_attr *u8500_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

int u8500_verify_speed(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy, freq_table);
}

#ifdef USE_PRCMU_OP
static int last_freq = 6000000;	/* TODO: remove temporary hack */
#endif

static int u8500_target(struct cpufreq_policy *policy, unsigned int target_freq,
			unsigned int relation)
{
	struct cpufreq_freqs freqs;
	unsigned int new_freq, idx;
	arm_opp_t op;

	/* Lookup the next frequency */
	if (cpufreq_frequency_table_target
	    (policy, freq_table, target_freq, relation, &idx)) {
		return -EINVAL;
	}

	/* Assuming freq_table entries have ordered "index"
	 * fields starting from 0
	 */
	new_freq = freq_table[idx].frequency;

	freqs.old = policy->cur;
	freqs.new = new_freq;
	freqs.cpu = policy->cpu;

	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

#ifdef USE_PRCMU_OP
	if (idx == 0)
		op = ARM_50_OPP;
	else if (idx == 1)
		op = ARM_100_OPP;
	last_freq = new_freq;
	prcmu_set_arm_opp(op);
#else
	set_plldivps(idx);
	/* writel(_u8500_plldivps_map[idx],PRCM_ARM_PLLDIVPS);
		udelay(_PLLDIVPS_LATENCY_US);
	*/
#endif
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	return 0;
}

unsigned int u8500_getspeed(unsigned int cpu)
{
	int div, i;
#ifdef USE_PRCMU_OP
	/* TODO: remove this and use prcmu_get_current_arm_opp(); */
	return last_freq;
#else
	div = readl(PRCM_ARM_PLLDIVPS);
	for (i = 0; i < PLLDIVPS_NUM_DIVIDERS; i++)
		if (_u8500_plldivps_map[i] == div)
			return freq_table[i].frequency;
#endif
	return 0;
}

static int __init u8500_cpu_init(struct cpufreq_policy *policy)
{
	int res;
	policy->cur = u8500_getspeed(policy->cpu);
	policy->governor = CPUFREQ_DEFAULT_GOVERNOR;

/*NOTES: transition_latency should be less when using post-scaler,
100 us is safe for now,
till we characterize it */
#ifdef USE_PRCMU_OP
	policy->cpuinfo.transition_latency = 200 * 1000; /* in ns */
#else
	/* in ns */
	policy->cpuinfo.transition_latency = _PLLDIVPS_LATENCY_US * 3 * 1000;
#endif
	/* policy->cpus = cpu_possible_map; TODO will cpufreq.c do this? */

	policy->shared_type = CPUFREQ_SHARED_TYPE_ALL;

	/* This fills the other policy fields (min, max, cpuinfo)
		according to the frequency table */
	res = cpufreq_frequency_table_cpuinfo(policy, freq_table);
	if (!res)
		cpufreq_frequency_table_get_attr(freq_table, policy->cpu);

	return res;
}

static int u8500_cpu_exit(struct cpufreq_policy *policy)
{
	return 0;
}

/* _target method is implemented as DVFS is driven by
 * softwareonly. _setpolicy method should be implemented
 * if DVFS is driven by hardware.
 */
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
