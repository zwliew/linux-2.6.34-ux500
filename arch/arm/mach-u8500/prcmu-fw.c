/*
 * Copyright 2009 ST-Ericsson.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

/* Routines to talk to PRCMU firmware services */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>

#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/prcmu-regs.h>
#include "prcmu-fw.h"

/* Internal functions */
int32_t _wait_for_req_complete(mailbox_t num)
{
	int timeout = 1000;

	/* Clear any error/status */
	writel(0, PRCM_ACK_MB0);
	/* Set an interrupt to XP70 */
	writel(1 << num, PRCM_MBOX_CPU_SET);
	/* As of now only polling method, need to check if interrupt
	 * possible ?? TODO */
	while ((readl(PRCM_MBOX_CPU_VAL) & (1 << num)) && timeout--)
		cpu_relax();

	if (!timeout)
		return -EBUSY;
	else
		return readl(PRCM_ACK_MB0);
	/*TODO sent error from std. linux list */
}

int32_t prcmu_request_mailbox0(req_mb0_t *req)
{
	writel(req->complete_field, PRCM_REQ_MB0);
	return _wait_for_req_complete(REQ_MB0);
}

int32_t prcmu_request_mailbox1(req_mb1_t *req)
{
	writew(req->complete_field, PRCM_REQ_MB1);
	return _wait_for_req_complete(REQ_MB1);
}

int32_t prcmu_request_mailbox2(req_mb2_t *req)
{
	writel(req->complete_field[0], PRCM_REQ_MB2);
	writel(req->complete_field[1], PRCM_REQ_MB2 + 4);
	writel(req->complete_field[2], PRCM_REQ_MB2 + 8);
	return _wait_for_req_complete(REQ_MB2);
}

/* Exported APIs */

/**
 * prcmu_get_boot_status - PRCMU boot status checking
 *
 * Returns: the current PRCMU boot status
 **/
uint8_t prcmu_get_boot_status(void)
{
	return readb(PRCM_BOOT_STATUS);
}
EXPORT_SYMBOL(prcmu_get_boot_status);

/**
 * prcmu_set_rc_a2p - This function is used to run few power state sequences
 *
 * @val: Value to be set, i.e. transition requested
 * Returns: 0 on success, -EINVAL on invalid argument
 *
 * This function is used to run the following power state sequences -
 * any state to ApReset,  ApDeepSleep to ApExecute, ApExecute to ApDeepSleep
 **/
int32_t prcmu_set_rc_a2p(romcode_write_t val)
{
	if (val < RDY_2_DS || val > RDY_2_XP70_RST)
		return -EINVAL;
	writeb(val, PRCM_ROMCODE_A2P);
	return 0;
}
EXPORT_SYMBOL(prcmu_set_rc_a2p);

/**
 * prcmu_get_rc_p2a - This function is used to get power state sequences
 *
 * Returns: the power transition that has last happened
 *
 * This function can return the following transitions-
 * any state to ApReset,  ApDeepSleep to ApExecute, ApExecute to ApDeepSleep
 **/
romcode_read_t prcmu_get_rc_p2a(void)
{
	return readb(PRCM_ROMCODE_P2A);
}
EXPORT_SYMBOL(prcmu_get_rc_p2a);

/**
 * prcmu_get_current_mode - Return the current AP power mode
 *
 * Returns: Returns the current AP(ARM) power mode: init,
 * apBoot, apExecute, apDeepSleep, apSleep, apIdle, apReset
 **/
ap_pwrst_t prcmu_get_current_mode(void)
{
	return readb(PRCM_AP_PWR_STATE);
}
EXPORT_SYMBOL(prcmu_get_current_mode);

/**
 * prcmu_set_ap_mode - set the appropriate AP power mode
 *
 * @ap_pwrst_trans: Transition to be requested to move to new AP power mode
 * Returns: 0 on success, non-zero on failure
 *
 * This function set the appropriate AP power mode.
 * The caller can check the status following this call.
 **/
int32_t prcmu_set_ap_mode(ap_pwrst_trans_t ap_pwrst_trans)
{
	req_mb0_t request = { {0}
	};
	if (ap_pwrst_trans
	    && (ap_pwrst_trans < APEXECUTE_TO_APSLEEP
		|| ap_pwrst_trans > APEXECUTE_TO_APIDLE))
		return -EINVAL;
	request.req_field.ap_pwrst_trans = ap_pwrst_trans;
	return prcmu_request_mailbox0(&request);
}
EXPORT_SYMBOL(prcmu_set_ap_mode);

/**
 * prcmu_set_fifo_4500wu - Configures 4500 fifo interrupt as wake-up events
 *
 * @fifo_4500wu: The 4500 fifo interrupt to be configured as wakeup or not
 * Returns: 0 on success, non-zero on failure
 *
 * This function de/configures 4500 fifo interrupt as wake-up events
 * The caller can check the status following this call.
 **/
int32_t prcmu_set_fifo_4500wu(intr_wakeup_t fifo_4500wu)
{
	req_mb0_t request = { {0}
	};
	if (fifo_4500wu < INTR_NOT_AS_WAKEUP || fifo_4500wu > INTR_AS_WAKEUP)
		return -EINVAL;
	request.req_field.fifo_4500wu = fifo_4500wu;
	return prcmu_request_mailbox0(&request);
}
EXPORT_SYMBOL(prcmu_set_fifo_4500wu);

/**
 * prcmu_set_ddr_pwrst - set the appropriate DDR power mode
 *
 * @ddr_pwrst: Power mode of DDR to which DDR needs to be switched
 * Returns: 0 on success, non-zero on failure
 *
 * This function is not supported on ED by the PRCMU firmware
 **/
int32_t prcmu_set_ddr_pwrst(ddr_pwrst_t ddr_pwrst)
{
	req_mb0_t request = { {0}
	};
	if (ddr_pwrst < DDR_PWR_STATE_UNCHANGED || ddr_pwrst > TOBEDEFINED)
		return -EINVAL;
	request.req_field.ddr_pwrst = ddr_pwrst;
	return prcmu_request_mailbox0(&request);
}
EXPORT_SYMBOL(prcmu_set_ddr_pwrst);

/**
 * prcmu_set_arm_opp - set the appropriate ARM OPP
 *
 * @arm_opp: The new ARM operating point to which transition is to be made
 * Returns: 0 on success, non-zero on failure
 *
 * This function set the appropriate ARM operation mode
 * The caller can check the status following this call.
 **/
#if 0
int32_t prcmu_set_arm_opp(arm_opp_t arm_opp)
{
	req_mb1_t request = { {0}
	};
	if (arm_opp < ARM_NO_CHANGE || arm_opp > ARM_EXTCLK)
		return -EINVAL;
	request.req_field.arm_opp = arm_opp;
	return prcmu_request_mailbox1(&request);
}
EXPORT_SYMBOL(prcmu_set_arm_opp);
#endif
int32_t prcmu_set_arm_opp(arm_opp_t arm_opp)
{
	int timeout = 200;
	if (arm_opp < ARM_NO_CHANGE || arm_opp > ARM_EXTCLK)
		return -EINVAL;
/* check for any ongoing AP state transitions */
	while ((readl(PRCM_MBOX_CPU_VAL) & 1) && timeout--)
		cpu_relax();
	if (!timeout)
		return -EBUSY;
/* check for any ongoing ARM DVFS */
	timeout = 200;
	while ((readb(PRCM_M2A_DVFS_STAT) == 0xff) && timeout--)
		cpu_relax();
	if (!timeout)
		return -EBUSY;

	/* Clear any error/status */
	writel(0, PRCM_ACK_MB0);

	writeb(arm_opp, PRCM_ARM_OPP);
	writeb(APE_NO_CHANGE, PRCM_APE_OPP);
	writel(2, PRCM_MBOX_CPU_SET);

	timeout = 1000;
	while ((readl(PRCM_MBOX_CPU_VAL) & 2) && timeout--)
		cpu_relax();

	/* TODO: read mbox to ARM error here.. */
	return 0;

}
EXPORT_SYMBOL(prcmu_set_arm_opp);

/**
 * prcmu_set_ape_opp - set the appropriate APE OPP
 *
 * @ape_opp: The new APE operating point to which transition is to be made
 * Returns: 0 on success, non-zero on failure
 *
 * This function set the appropriate APE operation mode
 * The caller can check the status following this call.
 **/
int32_t prcmu_set_ape_opp(ape_opp_t ape_opp)
{
	req_mb1_t request = { {0}
	};
	if (ape_opp < APE_NO_CHANGE || ape_opp > APE_100_OPP)
		return -EINVAL;
	request.req_field.ape_opp = ape_opp;
	return prcmu_request_mailbox1(&request);
}
EXPORT_SYMBOL(prcmu_set_ape_opp);

/**
 * prcmu_set_ape_opp - set the appropriate h/w accelerator to power mode
 *
 * @hw_acc: the hardware accelerator being considered - see the list for
 *          details
 * @hw_accst: The new h/w accelerator state(on/off/retention)
 * Returns: 0 on success, non-zero on failure
 *
 * This function set the appropriate hardware accelerator to the requested
 * power mode.
 * The caller can check the status following this call.
 **/
int32_t prcmu_set_hwacc_st(hw_acc_t hw_acc, hw_accst_t hw_accst)
{
	req_mb2_t request;
	if (hw_acc < SVAMMDSP || hw_acc > ESRAM4 ||
	    hw_accst < HW_NO_CHANGE || hw_accst > HW_ON)
		return -EINVAL;
	memset(&request, 0x0, sizeof(request));
	request.hw_accst_list[hw_acc].hw_accst = hw_accst;
	return prcmu_request_mailbox2(&request);
}
EXPORT_SYMBOL(prcmu_set_hwacc_st);

/**
 * prcmu_get_m2a_status - Get the status or transition of the last request
 *
 * Returns: The status from MBOX to ARM during the last request.
 * See the list of status/transitions for details.
 **/
mbox_2_arm_stat_t prcmu_get_m2a_status(void)
{
	return readb(PRCM_M2A_STATUS);
}
EXPORT_SYMBOL(prcmu_get_m2a_status);

/**
 * prcmu_get_m2a_error - Get the error that occured in last request
 *
 * Returns: The error from MBOX to ARM during the last request.
 * See the list of errors for details.
 **/
mbox_to_arm_err_t prcmu_get_m2a_error(void)
{
	return readb(PRCM_M2A_ERROR);
}
EXPORT_SYMBOL(prcmu_get_m2a_error);

/**
 * prcmu_get_m2a_dvfs_status - Get the state of DVFS transition
 *
 * Returns: Either that state transition on DVFS is on going
 * or completed, 0 if not used.
 **/
mbox_2_armdvfs_stat_t prcmu_get_m2a_dvfs_status(void)
{
	return readb(PRCM_M2A_DVFS_STAT);
}
EXPORT_SYMBOL(prcmu_get_m2a_dvfs_status);

/**
 * prcmu_get_m2a_hwacc_status - Get the state of HW Accelerator
 *
 * Returns: Either that state transition on hardware accelerator
 * is on going or completed, 0 if not used.
 **/
mbox_2_arm_hwacc_pwr_stat_t prcmu_get_m2a_hwacc_status(void)
{
	return readb(PRCM_M2A_HWACC_STAT);
}
EXPORT_SYMBOL(prcmu_get_m2a_hwacc_status);

/**
 * prcmu_set_irq_wakeup - set the ARM IRQ wake-up events
 * @irq: ARM IRQ that needs to be set as wake up source
 *
 * Returns: 0 on success, -EINVAL on invalid argument
 **/
int32_t prcmu_set_irq_wakeup(uint32_t irq)
{
	unsigned long mask_reg;
	if (irq < 32) {
		mask_reg = PRCM_ARMITMSK31TO0;
	} else if (irq < 64) {
		mask_reg = PRCM_ARMITMSK63TO32;
		irq -= 32;
	} else if (irq < 96) {
		mask_reg = PRCM_ARMITMSK95TO64;
		irq -= 64;
	} else if (irq < 128) {
		mask_reg = PRCM_ARMITMSK127TO96;
		irq -= 96;
	} else
		return -EINVAL;

	writel(1 << irq | readl((volatile unsigned long *)(mask_reg)),
	       (volatile unsigned long *)(mask_reg));
	return 0;
}
EXPORT_SYMBOL(prcmu_set_irq_wakeup);

/**
 * prcmu_apply_ap_state_transition - PRCMU State Transition function
 *
 * @transition: Transition to be requested to move to new AP power mode
 * @ddr_state_req: Power mode of DDR to which DDR needs to be switched
 * @_4500_fifo_wakeup: 4500 fifo interrupt to be configured as wakeup or not
 * Returns: 0 on success, -EINVAL on failure
 *
 * NOTES: This routine assumes that ARM side housekeeping that
 * needs to be done before entering sleep/deep-sleep, eg. the routines
 * begin(), prepare() in platform_suspend_ops have done their job
 * this routine only makes the required state transition and post-transition
 * ARM side handling e.g. calling WFI for the non-boot cpus for idle/sleep
 * or either waiting for the go-ahead from the deep-sleep secure interrupt
 * handler and then setting the romcode before entering wfi. The typical
 * users of this routine are the CPUIdle and LDM suspend ie. pm_suspend().
 * The accelerators also need this service but currently this routine does
 * not support it. This also assumes that the non-boot cpu's are in wfi
 * and not wfe.
 */
int32_t prcmu_apply_ap_state_transition(ap_pwrst_trans_t transition,
					ddr_pwrst_t ddr_state_req,
					int _4500_fifo_wakeup)
{
	int ret = 0;
	int sync = 0;
	unsigned int val = 0;

	/* The PRCMU does do state transition validation, so we will not
	   repeat it, just go ahead and call it */
	if (transition == APBOOT_TO_APEXECUTE)
		sync = 1;

	val = (ddr_state_req << 16) | (_4500_fifo_wakeup << 8) | transition;
	writel(val, PRCM_REQ_MB0);
	writel(1, PRCM_MBOX_CPU_SET);

	while ((readl(PRCM_MBOX_CPU_VAL) & 1) && sync)
		cpu_relax();

	switch (transition) {

	case APEXECUTE_TO_APSLEEP:
	case APEXECUTE_TO_APIDLE:
	      __asm__ __volatile__("dsb\n\t" "wfi\n\t" : : : "memory");
		break;

	case APEXECUTE_TO_APDEEPSLEEP:
		printk("TODO: deep sleep \n");
		break;

	case APBOOT_TO_APEXECUTE:
		break;

	default:
		ret = -EINVAL;
	}
	return ret;
}
EXPORT_SYMBOL(prcmu_apply_ap_state_transition);

static int prcmu_fw_init(void)
{
	int i;
#if 0
    int status = prcmu_get_boot_status();

    if (status != 0xFF || status != 0x2F) {
	printk("PRCMU Firmware not ready\n");
	return -EIO;
    }
    /* This can be enabled once PRCMU firmware flashing becomes default */
    prcmu_apply_ap_state_transition(APBOOT_TO_APEXECUTE,
					DDR_PWR_STATE_UNCHANGED, 0);

#endif

	/* for the time being assign wake-up duty to all interrupts */
	/* Enable all interrupts as wake-up source */
	for (i = 0; i < 128; i++)
		prcmu_set_irq_wakeup(i);

	return 0;
}

arch_initcall(prcmu_fw_init);
