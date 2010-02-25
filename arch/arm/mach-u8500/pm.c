/*
 * Copyright 2009 STMicroelectronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

/* U8500 platform suspend/resume support */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>

#include <asm/memory.h>
#include <asm/system.h>

#include <mach/prcmu-fw-api.h>

static int u8500_pm_begin(suspend_state_t state)
{
	return 0;
}

static int u8500_pm_prepare(void)
{
	return 0;
}

static int u8500_pm_suspend(void)
{
	return 0;
}

static int u8500_pm_enter(suspend_state_t state)
{
	int ret;

	switch (state) {
	case PM_SUSPEND_STANDBY:
		prcmu_apply_ap_state_transition(APEXECUTE_TO_APIDLE,
				DDR_PWR_STATE_UNCHANGED, 0);
		ret = 0;
		break;
	case PM_SUSPEND_MEM:
		prcmu_apply_ap_state_transition(APEXECUTE_TO_APSLEEP,
				DDR_PWR_STATE_UNCHANGED, 0);
		ret = 0;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static void u8500_pm_finish(void)
{

}

static int u8500_pm_valid(suspend_state_t state)
{
	return (state == PM_SUSPEND_MEM || state == PM_SUSPEND_STANDBY);
}

static struct platform_suspend_ops u8500_pm_ops = {
	.begin = u8500_pm_begin,
	.prepare = u8500_pm_prepare,
	.enter = u8500_pm_enter,
	.finish = u8500_pm_finish,
	.valid = u8500_pm_valid,
};

static int __init u8500_pm_init(void)
{
	suspend_set_ops(&u8500_pm_ops);
	return 0;
}

device_initcall(u8500_pm_init);
