/*
 * Copyright (C) STMicroelectronics 2009
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 * Author: Sundar Iyer <sundar.iyer@stericsson.com>
 *
 * CPUIdle driver for U8500
 */

#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>
#include <linux/clockchips.h>
#include <mach/prcmu-fw-api.h>
#include "cpuidle.h"

struct u8500_cstate u8500_cstates[U8500_NUM_CSTATES] = {
	{
	 .type = U8500_CSTATE_C0,
	 .sleep_latency = 0,
	 .wakeup_latency = 0,
	 .threshold = 0,
	 .power_usage = -1,
	 .flags = CPUIDLE_FLAG_SHALLOW | CPUIDLE_FLAG_TIME_VALID,
	 .desc = "C0 - running",
	 },
	{
	 .type = U8500_CSTATE_C1,
	 .sleep_latency = 10,
	 .wakeup_latency = 10,
	 .threshold = 10,
	 .power_usage = 10,
	 .flags = CPUIDLE_FLAG_SHALLOW | CPUIDLE_FLAG_TIME_VALID,
	 .desc = "C1 - wait for interrupt",
	 },
	{
	 .type = U8500_CSTATE_C2,
	 .sleep_latency = 50,
	 .wakeup_latency = 50,
	 .threshold = 20000,
	 .power_usage = 5,
	 .flags = CPUIDLE_FLAG_SHALLOW | CPUIDLE_FLAG_TIME_VALID,
	 .desc = "C2 - WFI-Retention",
	 },
};

DEFINE_PER_CPU(struct cpuidle_device, u8500_cpuidle_dev);
DEFINE_PER_CPU(int, u8500_cpu_in_wfi) = {0};

struct cpuidle_driver u8500_cpuidle_drv = {
	.name = "U8500_CPUIdle",
	.owner = THIS_MODULE,
};

static void do_nothing(void *unused)
{
}

/*
 * cpu_idle_wait - Used to ensure that all the CPUs discard old value of
 * pm_idle and update to new pm_idle value. Required while changing pm_idle
 * handler on SMP systems.
 *
 * Caller must have changed pm_idle to the new value before the call. Old
 * pm_idle value will not be used by any CPU after the return of this function.
 */
void cpu_idle_wait(void)
{
	smp_mb();
	/* kick all the CPUs so that they exit out of pm_idle */
	smp_call_function(do_nothing, NULL, 1);
}
EXPORT_SYMBOL_GPL(cpu_idle_wait);

static int poll_idle(struct cpuidle_device *dev, struct cpuidle_state *state)
{
	ktime_t t1, t2;
	s64 diff;
	int ret;
	t1 = ktime_get();
	local_irq_enable();
	while (!need_resched())
		cpu_relax();

	t2 = ktime_get();
	diff = ktime_to_us(ktime_sub(t2, t1));
	if (diff > INT_MAX)
		diff = INT_MAX;

	ret = (int)diff;
	return ret;
}

static int wfi_idle(struct cpuidle_device *dev, struct cpuidle_state *state)
{
	ktime_t t1, t2;
	s64 diff;
	int ret;

	t1 = ktime_get();

	__asm__ __volatile__("dsb\n\t" "wfi\n\t" : : : "memory");

	t2 = ktime_get();
	diff = ktime_to_us(ktime_sub(t2, t1));
	if (diff > INT_MAX)
		diff = INT_MAX;

	ret = (int)diff;
	return ret;
}

static int wfi_retention_idle(struct cpuidle_device *dev,
			      struct cpuidle_state *state)
{
	ktime_t t1, t2;
	s64 diff;
	int ret, cpu, i = 0;

	t1 = ktime_get();

	per_cpu(u8500_cpu_in_wfi, smp_processor_id()) = 1;
	smp_wmb();

	cpu = smp_processor_id();
	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_ENTER, &cpu);

	for_each_online_cpu(cpu)
	    if (per_cpu(u8500_cpu_in_wfi, cpu) == 1)
		i++;

	if (i == num_online_cpus())
		prcmu_apply_ap_state_transition(APEXECUTE_TO_APIDLE,
						DDR_PWR_STATE_UNCHANGED, 0);
	else {
		__asm__ __volatile__("dsb\n\t" "wfi\n\t" : : : "memory");
	}

	local_irq_disable();
	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_EXIT, &cpu);
	local_irq_enable();

	per_cpu(u8500_cpu_in_wfi, smp_processor_id()) = 0;
	smp_wmb();

	t2 = ktime_get();
	diff = ktime_to_us(ktime_sub(t2, t1));
	if (diff > INT_MAX)
		diff = INT_MAX;

	ret = (int)diff;
	return ret;

}

static int u8500_enter_idle(struct cpuidle_device *dev,
			    struct cpuidle_state *state)
{
	struct u8500_cstate *cstate;
	int ret = 0;
	cstate = cpuidle_get_statedata(state);

	local_irq_disable();

	if (cstate->type == U8500_CSTATE_C1)
		ret = wfi_idle(dev, state);

	if (cstate->type == U8500_CSTATE_C2)
		ret = wfi_retention_idle(dev, state);

	if (cstate->type == U8500_CSTATE_C0) {
		ret = poll_idle(dev, state);

		/* irq already enabled so just return. */
		return ret;
	}

	local_irq_enable();

	return ret;
}

static void u8500_cpu_cstate_init(unsigned int cpu)
{
	struct cpuidle_device *dev;
	struct u8500_cstate *cstate;
	struct cpuidle_state *state;
	int i;

	printk("Initializing CPUIdle for CPU%d...\n", cpu);
	dev = &per_cpu(u8500_cpuidle_dev, cpu);
	dev->cpu = cpu;
	for (i = 0; i < U8500_NUM_CSTATES; i++) {
		cstate = &u8500_cstates[i];
		state = &dev->states[i];

		cpuidle_set_statedata(state, cstate);

		state->exit_latency =
		    cstate->sleep_latency + cstate->wakeup_latency;
		state->target_residency = cstate->threshold;
		state->flags = cstate->flags;
		state->enter = u8500_enter_idle;
		state->power_usage = cstate->power_usage;
		snprintf(state->name, CPUIDLE_NAME_LEN, "C%d", i);
		strncpy(state->desc, cstate->desc, CPUIDLE_DESC_LEN);
	}

	dev->state_count = U8500_NUM_CSTATES;

	if (dev->state_count >= 2)
		dev->safe_state = &dev->states[1];
	else
		dev->safe_state = &dev->states[0];

	if (cpuidle_register_device(dev)) {
		printk(KERN_ERR "%s: CPUidle register device failed\n",
		       __func__);
	}

}

static int __init u8500_cpuidle_init(void)
{
	int result = 0;
	unsigned int cpu;

	result = cpuidle_register_driver(&u8500_cpuidle_drv);
	if (result < 0)
		goto out;

	/* TODO: pass a per-cpu var to get the return value of
	   cpuidle_register_device */
	/* on_each_cpu(u8500_cpu_cstate_init, NULL, 0); */
	for_each_online_cpu(cpu)
	    u8500_cpu_cstate_init(cpu);
      out:
	return result;
}
static void __exit u8500_cpuidle_exit(void)
{
	/* TODO: disable cpuidle devices & unregister driver */
}

module_init(u8500_cpuidle_init);
module_exit(u8500_cpuidle_exit);

MODULE_DESCRIPTION("U8500 CPUIdle driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("STMicroelectronics/Sundar Iyer, ST-Ericsson");
