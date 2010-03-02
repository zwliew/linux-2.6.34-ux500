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
#include "prcmu-fw_ed.h"
#include "prcmu-fw_v1.h"

#define NAME "PRCMU"
#define PM_DEBUG 0
#define dbg_printk(format, arg...) (PM_DEBUG & 1) ? \
	(printk(KERN_ALERT NAME ": " format , ## arg)) : \
		({do {} while (0); })

void __iomem *rtc_register_base ;

/* TASKLET declarations */
static void prcmu_ack_mb7_status_tasklet(unsigned long);
DECLARE_TASKLET(prcmu_ack_mb7_tasklet, prcmu_ack_mb7_status_tasklet, 0);

static void prcmu_ack_mb0_wkuph_status_tasklet(unsigned long);
DECLARE_TASKLET(prcmu_ack_mb0_wkuph_tasklet, \
		prcmu_ack_mb0_wkuph_status_tasklet, 0);


/* WAITQUEUES */
DECLARE_WAIT_QUEUE_HEAD(ack_mb0_queue);
DECLARE_WAIT_QUEUE_HEAD(ack_mb5_queue);
DECLARE_WAIT_QUEUE_HEAD(ack_mb7_queue);


/* function pointer for shrm callback sequence for modem waking up arm */
static void (*prcmu_modem_wakes_arm_shrm)(void);

/* function pointer for shrm callback sequence for modem requesting reset */
static void (*prcmu_modem_reset_shrm)(void);


/* Internal functions */

/**
 * _wait_for_req_complete () -  used for PWRSTTRH for AckMb0 and AckMb1 on V1
 * @num:			Mailbox number to operate on
 *
 * Polling loop to ensure that PRCMU FW has completed the requested op
 */
int _wait_for_req_complete(mailbox_t num)
{
	int timeout = 1000;

	if (u8500_is_earlydrop()) {

		/* Clear any error/status */
		writel(0, PRCM_ACK_MB0_ED);
		/* Set an interrupt to XP70 */
		writel(1 << num, PRCM_MBOX_CPU_SET);
		/* As of now only polling method, need to check if interrupt
		 * possible ?? TODO */
		while ((readl(PRCM_MBOX_CPU_VAL) & (1 << num)) && timeout--)
			cpu_relax();

		if (!timeout)
			return -EBUSY;
		else
			return readl(PRCM_ACK_MB0_ED);


	} else {

		/* checking any already on-going transaction */
		while ((readl(PRCM_MBOX_CPU_VAL) & (1 << num)) && timeout--)
			cpu_relax();

		timeout = 1000;


		/* Clear any error/status */
		if (num == REQ_MB0) {
			writeb(0, PRCM_ACK_MB0_AP_PWRST_STATUS);
		} else if (num == REQ_MB1) {
			writel(0, PRCM_ACK_MB1);
		}


		/* Set an interrupt to XP70 */
		writel(1 << num, PRCM_MBOX_CPU_SET);
		/* As of now only polling method, need to check if interrupt
		 * possible ?? TODO */
		while ((readl(PRCM_MBOX_CPU_VAL) & (1 << num)) && timeout--)
			cpu_relax();


		if (!timeout)
			return -EBUSY;

		return 0;

	}

}

/**
 * prcmu_request_mailbox0 - request op on mailbox0
 * @req:		    mailbox0 type param
 *
 * Fill up the mailbox 0 required fields and call polling loop
 */
int prcmu_request_mailbox0(req_mb0_t *req)
{
	if (u8500_is_earlydrop()) {
		writel(req->complete_field, PRCM_REQ_MB0_ED);
		return _wait_for_req_complete(REQ_MB0_ED);

	} else {
		writel(req->complete_field, PRCM_REQ_MB0);
		return _wait_for_req_complete(REQ_MB0);
	}
}

/**
 * prcmu_request_mailbox1 - request op on mailbox1
 * @req:		    mailbox1 type param
 *
 * Fill up the mailbox 1 required fields and call polling loop
 */
int prcmu_request_mailbox1(req_mb1_t *req)
{
	if (u8500_is_earlydrop()) {
		writew(req->complete_field, PRCM_REQ_MB1_ED);
		return _wait_for_req_complete(REQ_MB1_ED);
	} else {
		writew(req->complete_field, PRCM_REQ_MB1);
		return _wait_for_req_complete(REQ_MB1);
	}
}

/**
 * prcmu_request_mailbox2 - request op on mailbox2
 * @req:		    mailbox2 type param
 *
 * Fill up the mailbox 2 required fields and call polling loop
 */
int prcmu_request_mailbox2(req_mb2_t *req)
{
	if (u8500_is_earlydrop()) {
		writel(req->complete_field[0], PRCM_REQ_MB2_ED);
		writel(req->complete_field[1], PRCM_REQ_MB2_ED + 4);
		writel(req->complete_field[2], PRCM_REQ_MB2_ED + 8);
		return _wait_for_req_complete(REQ_MB2_ED);

	} else {
		writel(req->complete_field[0], PRCM_REQ_MB2);
		writel(req->complete_field[1], PRCM_REQ_MB2 + 4);
		writel(req->complete_field[2], PRCM_REQ_MB2 + 8);
		return _wait_for_req_complete(REQ_MB2);
	}

}

/* Exported APIs */

/**
 * prcmu_get_boot_status - PRCMU boot status checking
 * Returns: the current PRCMU boot status
 */
int prcmu_get_boot_status(void)
{
	if (u8500_is_earlydrop()) {
		return readb(PRCM_BOOT_STATUS_ED);
	} else {
		return readb(PRCM_BOOT_STATUS);
	}
}
EXPORT_SYMBOL(prcmu_get_boot_status);

/**
 * prcmu_set_rc_a2p - This function is used to run few power state sequences
 * @val: 	Value to be set, i.e. transition requested
 * Returns: 0 on success, -EINVAL on invalid argument
 *
 * This function is used to run the following power state sequences -
 * any state to ApReset,  ApDeepSleep to ApExecute, ApExecute to ApDeepSleep
 */
int prcmu_set_rc_a2p(romcode_write_t val)
{
	if (u8500_is_earlydrop()) {
		if (val < RDY_2_DS_ED || val > RDY_2_XP70_RST_ED)
			return -EINVAL;
		writeb(val, PRCM_ROMCODE_A2P_ED);
		return 0;
	}


	if (val < RDY_2_DS || val > RDY_2_XP70_RST)
		return -EINVAL;
	writeb(val, PRCM_ROMCODE_A2P);
	return 0;
}
EXPORT_SYMBOL(prcmu_set_rc_a2p);

/**
 * prcmu_get_rc_p2a - This function is used to get power state sequences
 * Returns: the power transition that has last happened
 *
 * This function can return the following transitions-
 * any state to ApReset,  ApDeepSleep to ApExecute, ApExecute to ApDeepSleep
 */
romcode_read_t prcmu_get_rc_p2a(void)
{
	if (u8500_is_earlydrop()) {
		return readb(PRCM_ROMCODE_P2A_ED);
	}

	return readb(PRCM_ROMCODE_P2A);
}
EXPORT_SYMBOL(prcmu_get_rc_p2a);

/**
 * prcmu_get_current_mode - Return the current XP70 power mode
 * Returns: Returns the current AP(ARM) power mode: init,
 * apBoot, apExecute, apDeepSleep, apSleep, apIdle, apReset
 */
ap_pwrst_t prcmu_get_xp70_current_state(void)
{
	if (u8500_is_earlydrop()) {
		return readb(PRCM_AP_PWR_STATE_ED);
	}

	return readb(PRCM_XP70_CUR_PWR_STATE);
}
EXPORT_SYMBOL(prcmu_get_xp70_current_state);

/**
 * prcmu_set_ap_mode - set the appropriate AP power mode
 * @ap_pwrst_trans: Transition to be requested to move to new AP power mode
 * Returns: 0 on success, non-zero on failure
 *
 * This function set the appropriate AP power mode.
 * The caller can check the status following this call.
 */
int prcmu_set_ap_mode(ap_pwrst_trans_t ap_pwrst_trans)
{
	req_mb0_t request = { {0} };

	if (u8500_is_earlydrop()) {
		if (ap_pwrst_trans
				&& (ap_pwrst_trans < APEXECUTE_TO_APSLEEP_ED
				|| ap_pwrst_trans > APEXECUTE_TO_APIDLE_ED))
			return -EINVAL;
		request.req_field.ap_pwrst_trans = ap_pwrst_trans;
		return prcmu_request_mailbox0(&request);
	}

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
 * @fifo_4500wu: The 4500 fifo interrupt to be configured as wakeup or not
 * Returns: 0 on success, non-zero on failure
 *
 * This function de/configures 4500 fifo interrupt as wake-up events
 * The caller can check the status following this call.
 */
int prcmu_set_fifo_4500wu(intr_wakeup_t fifo_4500wu)
{
	req_mb0_t request = { {0} };

	if (u8500_is_earlydrop()) {
		if (fifo_4500wu < INTR_NOT_AS_WAKEUP_ED \
				|| fifo_4500wu > INTR_AS_WAKEUP_ED)
			return -EINVAL;
		request.req_field.fifo_4500wu = fifo_4500wu;
		return prcmu_request_mailbox0(&request);
	}

	if (fifo_4500wu < INTR_NOT_AS_WAKEUP || fifo_4500wu > INTR_AS_WAKEUP)
		return -EINVAL;
	request.req_field.fifo_4500wu = fifo_4500wu;
	return prcmu_request_mailbox0(&request);
}
EXPORT_SYMBOL(prcmu_set_fifo_4500wu);

/**
 * prcmu_set_ddr_pwrst - set the appropriate DDR power mode
 * @ddr_pwrst: Power mode of DDR to which DDR needs to be switched
 * Returns: 0 on success, non-zero on failure
 *
 * This function is not supported on ED by the PRCMU firmware
 */
int prcmu_set_ddr_pwrst(ddr_pwrst_t ddr_pwrst)
{
	req_mb0_t request = { {0} };

	if (u8500_is_earlydrop()) {
		if (ddr_pwrst < DDR_PWR_STATE_UNCHANGED_ED || \
				ddr_pwrst > TOBEDEFINED_ED)
			return -EINVAL;
		request.req_field.ddr_pwrst = ddr_pwrst;
		return prcmu_request_mailbox0(&request);
	}

	if (ddr_pwrst < DDR_PWR_STATE_UNCHANGED ||  \
			ddr_pwrst > DDR_PWR_STATE_OFFHIGHLAT)
		return -EINVAL;
	request.req_field.ddr_pwrst = ddr_pwrst;
	return prcmu_request_mailbox0(&request);
}
EXPORT_SYMBOL(prcmu_set_ddr_pwrst);

/**
 * prcmu_set_arm_opp - set the appropriate ARM OPP
 * @arm_opp: The new ARM operating point to which transition is to be made
 * Returns: 0 on success, non-zero on failure
 *
 * This function set the appropriate ARM operation mode
 * The caller can check the status following this call.
 * NOTE : THE PRCMU DOES NOT ISSUE AN IT TO ARM FOR A DVFS
 *	  XISITION REQUEST. HENCE WE DO NOT NEED A MAILBOX
 *	  TIMEOUT WAIT HERE FOR OPP DVFS
 */
int prcmu_set_arm_opp(arm_opp_t arm_opp)
{
	int timeout = 200;
	arm_opp_t current_arm_opp;

	if (u8500_is_earlydrop()) {
		if (arm_opp < ARM_NO_CHANGE_ED || arm_opp > ARM_EXTCLK_ED)
			return -EINVAL;
		/* check for any ongoing AP state transitions */
		while ((readl(PRCM_MBOX_CPU_VAL) & 1) && timeout--)
			cpu_relax();
		if (!timeout)
			return -EBUSY;
		/* check for any ongoing ARM DVFS */
		timeout = 200;
		while ((readb(PRCM_M2A_DVFS_STAT_ED) == 0xff) && timeout--)
			cpu_relax();
		if (!timeout)
			return -EBUSY;

		/* Clear any error/status */
		writel(0, PRCM_ACK_MB0_ED);

		writeb(arm_opp, PRCM_ARM_OPP_ED);
		writeb(APE_NO_CHANGE, PRCM_APE_OPP_ED);
		writel(2, PRCM_MBOX_CPU_SET);

		timeout = 1000;
		while ((readl(PRCM_MBOX_CPU_VAL) & 2) && timeout--)
			cpu_relax();

		/* TODO: read mbox to ARM error here.. */
		return 0;
	}


	if (arm_opp < ARM_NO_CHANGE || arm_opp > ARM_EXTCLK)
		return -EINVAL;

	/* check for any ongoing AP state transitions */
	while ((readl(PRCM_MBOX_CPU_VAL) & 1) && timeout--)
		cpu_relax();
	if (!timeout)
		return -EBUSY;

	/* check for any ongoing ARM DVFS */
	timeout = 200;
	while ((readb(PRCM_ACK_MB1_CURR_DVFS_STATUS) == 0xff) && timeout--)
		cpu_relax();
	if (!timeout)
		return -EBUSY;

	/* clear the mailbox */
	writel(0, PRCM_ACK_MB1);

	/* write 0x0 into the header for ARM/APE operating point mgmt */
	writel(0x0, _PRCM_MBOX_HEADER);

	/* write ARMOPP request into the mailbox */
	writel(arm_opp, PRCM_REQ_MB1_ARMOPP);
	writel(APE_NO_CHANGE, PRCM_REQ_MB1_APEOPP);

	_wait_for_req_complete(REQ_MB1);

	current_arm_opp = readb(PRCM_ACK_MB1_CURR_ARMOPP);
	if (arm_opp != current_arm_opp) {
		printk(KERN_WARNING
			"u8500-prcm : requested ARM DVFS Not Complete\n");
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(prcmu_set_arm_opp);

/**
 * prcmu_get_arm_opp - get the appropriate ARM OPP
 *
 * Returns: the current ARM OPP
 */
int prcmu_get_arm_opp(void)
{
	return readb(PRCM_ACK_MB1_CURR_ARMOPP);
}
EXPORT_SYMBOL(prcmu_get_arm_opp);

/**
 * prcmu_set_ape_opp - set the appropriate APE OPP
 * @ape_opp: The new APE operating point to which transition is to be made
 * Returns: 0 on success, non-zero on failure
 *
 * This function set the appropriate APE operation mode
 * The caller can check the status following this call.
 */
int prcmu_set_ape_opp(ape_opp_t ape_opp)
{
	int timeout = 200;
	ape_opp_t current_ape_opp;

	if (ape_opp < APE_NO_CHANGE || ape_opp > APE_50_OPP)
		return -EINVAL;

	/* check for any ongoing AP state transitions */
	while ((readl(PRCM_MBOX_CPU_VAL) & 1) && timeout--)
		cpu_relax();
	if (!timeout)
		return -EBUSY;

	/* check for any ongoing ARM DVFS */
	timeout = 200;
	while ((readb(PRCM_ACK_MB1_CURR_DVFS_STATUS) == 0xff) && timeout--)
		cpu_relax();
	if (!timeout)
		return -EBUSY;

	/* clear the mailbox */
	writel(0, PRCM_ACK_MB1);

	/* write 0x0 into the header for ARM/APE operating point mgmt */
	writel(0x0, _PRCM_MBOX_HEADER);

	/* write ARMOPP request into the mailbox */
	writel(ARM_NO_CHANGE, PRCM_REQ_MB1_ARMOPP);
	writel(ape_opp, PRCM_REQ_MB1_APEOPP);

	/* trigger the XP70 IT11 to the XP70 */
	writel(PRCM_XP70_TRIG_IT11, PRCM_MBOX_CPU_SET);

	timeout = 1000;
	while ((readl(PRCM_MBOX_CPU_VAL) & PRCM_XP70_TRIG_IT11)
			&& timeout--)
		cpu_relax();

	current_ape_opp = readb(PRCM_ACK_MB1_CURR_APEOPP);
	if (ape_opp != current_ape_opp) {
		printk(KERN_WARNING
			"u8500-prcm : requested APE DVFS Not Complete\n");
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(prcmu_set_ape_opp);

/**
 * prcmu_get_arm_opp - get the appropriate ARM OPP
 *
 * Returns: the current ARM OPP
 */
int prcmu_get_ape_opp(void)
{
	return readb(PRCM_ACK_MB1_CURR_APEOPP);
}
EXPORT_SYMBOL(prcmu_get_ape_opp);

/**
 * prcmu_set_ape_opp - set the appropriate h/w accelerator to power mode
 * @hw_acc: the hardware accelerator being considered
 * @hw_accst: The new h/w accelerator state(on/off/retention)
 * Returns: 0 on success, non-zero on failure
 *
 * This function set the appropriate hardware accelerator to the requested
 * power mode.
 * The caller can check the status following this call.
 */
int prcmu_set_hwacc_st(hw_acc_t hw_acc, hw_accst_t hw_accst)
{
	req_mb2_t request;

	if (u8500_is_earlydrop()) {
		if (hw_acc < SVAMMDSP_ED || hw_acc > ESRAM4_ED ||
				hw_accst < HW_NO_CHANGE_ED || hw_accst \
				> HW_ON_ED)
			return -EINVAL;
		memset(&request, 0x0, sizeof(request));
		request.hw_accst_list[hw_acc].hw_accst = hw_accst;
		return prcmu_request_mailbox2(&request);
	}

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
 * Returns: The status from MBOX to ARM during the last request.
 * See the list of status/transitions for details.
 */
mbox_2_arm_stat_ed_t prcmu_get_m2a_status(void)
{
	return readb(PRCM_M2A_STATUS_ED);
}
EXPORT_SYMBOL(prcmu_get_m2a_status);

/**
 * prcmu_get_m2a_error - Get the error that occured in last request
 * Returns: The error from MBOX to ARM during the last request.
 * See the list of errors for details.
 */
mbox_to_arm_err_ed_t prcmu_get_m2a_error(void)
{
	return readb(PRCM_M2A_ERROR_ED);
}
EXPORT_SYMBOL(prcmu_get_m2a_error);

/**
 * prcmu_get_m2a_dvfs_status - Get the state of DVFS transition
 * Returns: Either that state transition on DVFS is on going
 * or completed, 0 if not used.
 */
dvfs_stat_t prcmu_get_m2a_dvfs_status(void)
{
	if (u8500_is_earlydrop()) {
		return readb(PRCM_M2A_DVFS_STAT_ED);
	}

	return 0;
}
EXPORT_SYMBOL(prcmu_get_m2a_dvfs_status);

/**
 * prcmu_get_m2a_hwacc_status - Get the state of HW Accelerator
 * Returns: Either that state transition on hardware accelerator
 * is on going or completed, 0 if not used.
 */
mbox_2_arm_hwacc_pwr_stat_t prcmu_get_m2a_hwacc_status(void)
{
	if (u8500_is_earlydrop()) {
		return readb(PRCM_M2A_HWACC_STAT_ED);
	}

	return 0;
}
EXPORT_SYMBOL(prcmu_get_m2a_hwacc_status);

/**
 * prcmu_set_irq_wakeup - set the ARM IRQ wake-up events
 * @irq: ARM IRQ that needs to be set as wake up source
 * Returns: 0 on success, -EINVAL on invalid argument
 */
int prcmu_set_irq_wakeup(uint32_t irq)
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

/* FIXME : Make these generic enough to pass backup
 *	   addresses for re-using the same functions
 *	   for sleep/deep sleep modes
 */
/* Context Backups for ApSleep */
unsigned int PRCC_backup[15];

#define _PRCC_CLK_RST1_BASE 	IO_ADDRESS(U8500_PER1_BASE + 0xF000)
#define _PRCC_CLK_RST2_BASE 	IO_ADDRESS(U8500_PER2_BASE + 0xF000)
#define _PRCC_CLK_RST3_BASE 	IO_ADDRESS(U8500_PER3_BASE + 0xF000)
#define _PRCC_CLK_RST5_BASE 	IO_ADDRESS(U8500_PER5_BASE + 0x1F000)
#define _PRCC_CLK_RST6_BASE 	IO_ADDRESS(U8500_PER6_BASE + 0xF000)
#define _PRCC_CLK_RST7_BASE 	IO_ADDRESS(U8500_PER7_BASE_ED + 0xF000)

void pm_save_config_PRCC(void)
{
	uint8_t i = 0;

	PRCC_backup[i++] = readl(_PRCC_CLK_RST1_BASE + 0x10);
	PRCC_backup[i++] = readl(_PRCC_CLK_RST1_BASE + 0x14);
	PRCC_backup[i++] = readl(_PRCC_CLK_RST2_BASE + 0x10);
	PRCC_backup[i++] = readl(_PRCC_CLK_RST2_BASE + 0x14);
	PRCC_backup[i++] = readl(_PRCC_CLK_RST3_BASE + 0x10);
	PRCC_backup[i++] = readl(_PRCC_CLK_RST3_BASE + 0x14);
	PRCC_backup[i++] = readl(_PRCC_CLK_RST5_BASE + 0x10);
	PRCC_backup[i++] = readl(_PRCC_CLK_RST5_BASE + 0x14);
	PRCC_backup[i++] = readl(_PRCC_CLK_RST6_BASE + 0x10);
	PRCC_backup[i++] = readl(_PRCC_CLK_RST6_BASE + 0x14);
	PRCC_backup[i++] = readl(_PRCC_CLK_RST7_BASE + 0x10);
	PRCC_backup[i++] = readl(_PRCC_CLK_RST7_BASE + 0x14);
}

void pm_restore_config_PRCC(void)
{
	uint8_t i = 0;

	writel(PRCC_backup[i++], _PRCC_CLK_RST1_BASE + 0x0);
	writel(PRCC_backup[i++], _PRCC_CLK_RST1_BASE + 0x8);
	writel(PRCC_backup[i++], _PRCC_CLK_RST2_BASE + 0x0);
	writel(PRCC_backup[i++], _PRCC_CLK_RST2_BASE + 0x8);
	writel(PRCC_backup[i++], _PRCC_CLK_RST3_BASE + 0x0);
	writel(PRCC_backup[i++], _PRCC_CLK_RST3_BASE + 0x8);
	writel(PRCC_backup[i++], _PRCC_CLK_RST5_BASE + 0x0);
	writel(PRCC_backup[i++], _PRCC_CLK_RST5_BASE + 0x8);
	writel(PRCC_backup[i++], _PRCC_CLK_RST6_BASE + 0x0);
	writel(PRCC_backup[i++], _PRCC_CLK_RST6_BASE + 0x8);
	writel(PRCC_backup[i++], _PRCC_CLK_RST7_BASE + 0x0);
	writel(PRCC_backup[i++], _PRCC_CLK_RST7_BASE + 0x8);
}

void __iomem *uart_backup_base, *uart_register_base;

void pm_save_config_UART(void)
{
	uart_register_base = ioremap(U8500_UART2_BASE, SZ_4K);
	if (!uart_register_base) {
		printk(KERN_WARNING
		 "u8500-prcm : cannot allocate uart register base\n");
		return -EINVAL;
	}

	uart_backup_base = kzalloc(SZ_4K, GFP_KERNEL);
	if (!uart_backup_base) {
		printk(KERN_WARNING
		 "u8500-prcm : cannot allocate uart register backup base\n");
		return -EINVAL;
	}

	/* Copy UART config */
	memcpy(uart_backup_base, uart_register_base, SZ_4K);
}

void __iomem *ab8500_backup_base, *ab8500_register_base;

void pm_restore_config_UART(void)
{
	memcpy(uart_register_base, uart_backup_base, SZ_4K);
}

/**
 * prcmu_apply_ap_state_transition - PRCMU State Transition function
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
/* FIXME : get these from platform/header files instead */
/* GIC BAse Address */
#define GIC_BASE_ADDR           IO_ADDRESS(0xA0411000)

/* ITs enabled for GIC. 104 is due to skipping of the STI and PPI sets.
 * Rfer page 648 of the DB8500V1 spec v2.5
 */
#define DIST_ENABLE_SET         (GIC_BASE_ADDR + 0x104)
#define DIST_PENDING_SET        (GIC_BASE_ADDR + 0x200)
#define DIST_ENABLE_CLEAR       (GIC_BASE_ADDR + 0x180)
#define DIST_ACTIVE_BIT         (GIC_BASE_ADDR + 0x304)

#define PRCM_DEBUG_NOPWRDOWN_VAL        IO_ADDRESS(0x80157194)
#define PRCM_POWER_STATE_VAL            IO_ADDRESS(0x8015725C)

int prcmu_apply_ap_state_transition(ap_pwrst_trans_t transition,
					ddr_pwrst_t ddr_state_req,
					int _4500_fifo_wakeup)
{
	int ret = 0, flags;
	int sync = 0;
	unsigned int val = 0, timeout, tmp ;

	if (u8500_is_earlydrop()) {
		/* The PRCMU does do state transition validation, so we wl not
		   repeat it, just go ahead and call it */
		if (transition == APBOOT_TO_APEXECUTE_ED)
			sync = 1;

		val = (ddr_state_req << 16) | (_4500_fifo_wakeup << 8) | \
		      transition;
		writel(val, PRCM_REQ_MB0_ED);
		writel(1, PRCM_MBOX_CPU_SET);

		while ((readl(PRCM_MBOX_CPU_VAL) & 1) && sync)
			cpu_relax();

		switch (transition) {

		case APEXECUTE_TO_APSLEEP_ED:
		case APEXECUTE_TO_APIDLE_ED:
			__asm__ __volatile__("dsb\n\t" "wfi\n\t":::"memory");
			break;
		case APEXECUTE_TO_APDEEPSLEEP_ED:
			printk(KERN_INFO "TODO: deep sleep \n");
			break;
		case APBOOT_TO_APEXECUTE_ED:
			break;
		default:
			ret = -EINVAL;
		}
		return ret;
	}

	switch (transition) {
	case APEXECUTE_TO_APSLEEP:

		/* PROGRAM THE ITMASK* registers for a Sleep */
		writel(0x00000000, PRCM_ARMITMSK31TO0);
		writel(0x00000100, PRCM_ARMITMSK63TO32);
		writel(0x00000000, PRCM_ARMITMSK95TO64);
		writel(0x00000000, PRCM_ARMITMSK127TO96);

		/* probe the GIC if its already frozen */
		tmp = readl(PRCM_A9_MASK_ACK);
		if (tmp & 0x00000001)
			printk(KERN_WARNING "GIC is already frozen\n");

		/* write to GIC freeze */
		tmp = readl(PRCM_A9_MASK_REQ);
		tmp |= 0x01;
		writel(tmp, PRCM_A9_MASK_REQ);

		/* we wait for a random timeout. as of now the
		 * the GIC freeze isnt ACKed; so wait
		 */
		timeout = 100;
		while (timeout--)
			cpu_relax();

		/* PROGRAM WAKEUP EVENTS */
		/* write CfgWkUpsH in the Header */
		writeb(WKUPCFGH, PRCM_MBOX_HEADER_REQ_MB0);

		/* write to the mailbox */
		/* E8500Wakeup_ARMITMGMT Bit (1<<17). it is interpreted by
		 * the firmware to set the register prcm_armit_maskxp70_it
		 * (which is btw secure and thus only accessed by the xp70)
		 */
		writel((1<<17), PRCM_REQ_MB0_WKUP_8500);
		writel(0x0, PRCM_REQ_MB0_WKUP_4500);

		/* SIGNAL MAILBOX */
		/* set the MBOX_CPU_SET bit to set an IT to xP70 */
		writel(1 << 0, PRCM_MBOX_CPU_SET);

		/* wait for corresponding MB0X_CPU_VAL bit to be cleared */
		while ((readl(PRCM_MBOX_CPU_VAL) & (1 << 0)) && timeout--)
			cpu_relax();
		if (!timeout) {
			printk(KERN_INFO
			 "Timeout in prcmu_configure_wakeup_events!!\n");
			return -EBUSY;
		}

		/* CREATE MAILBOX FOR EXECUTE TO IDLE POWER TRANSITION */
		/* Write PwrStTrH=0 header to request a Power state xsition */
		writeb(0x0, PRCM_MBOX_HEADER_REQ_MB0);

		/* write request to the MBOX0 */
		writeb(APEXECUTE_TO_APSLEEP, PRCM_REQ_MB0_PWRSTTRH_APPWRST);
		writeb(OFF, PRCM_REQ_MB0_PWRSTTRH_APPLLST);
		writeb(ON, PRCM_REQ_MB0_PWRSTTRH_ULPCLKST);
		writeb(0x56, PRCM_REQ_MB0_PWRSTTRH_BYTEFILL);

		/* As per the sync logic, we are supposed to be the final CPU.
		 * If the other CPU isnt in wfi, better exit by putting
		 * ourselves in wfi
		 */
		if (smp_processor_id()) {
			if (!(readl(PRCM_ARM_WFI_STANDBY) & 0x8)) {
				printk(KERN_WARNING
				"Other CPU(CPU%d) is not in WFI!(0x%x)\
				 Aborting attempt\n",
					!smp_processor_id(),
					readl(PRCM_ARM_WFI_STANDBY));
				__asm__ __volatile__(
					"dsb\n\t" "wfi\n\t" : : : "memory");
				return 0;
			}
		} else /* running on CPU0, check for CPU1 WFI standby */ {
			if (!(readl(PRCM_ARM_WFI_STANDBY) & 0x10)) {
				printk(KERN_WARNING
				"Other CPU(CPU%d) is not in WFI!(0x%x)\
				 Aborting attempt\n",
					!smp_processor_id(),
					readl(PRCM_ARM_WFI_STANDBY));
				__asm__ __volatile__(
					"dsb\n\t" "wfi\n\t" : : : "memory");
				return 0;
			}
		}

		/* Enable RTC-INT in the GIC */
		if (!(readl(PRCM_ARMITMSK31TO0) & 0x40000))
			writel((readl(PRCM_ARMITMSK31TO0) | 0x40000),
						PRCM_ARMITMSK31TO0);


		/* FIXME : later on, the ARM should not request a Idle if one
		 *         of the ITSTATUS0/5 are still alive!!!
		 */
		if (readl(PRCM_ITSTATUS0) == 0x80) {
			printk(KERN_WARNING "PRCM_ITSTATUS0 Not cleared\n");
			__asm__ __volatile__(
				"dsb\n\t" "wfi\n\t" : : : "memory");
			return -EBUSY;
		}

		/*
		 *
		 * REQUEST POWER XSITION
		 *
		 */

		/* clear ACK MB0 */
		writeb(0x0, PRCM_ACK_MB0);

		/* trigger the XP70 IT10 to the XP70 */
		writel(1, PRCM_MBOX_CPU_SET);

		timeout = 200;
		while (timeout--)
			cpu_relax();

		printk(KERN_INFO "u8500-prcm : To Sleep\n");

		/* Restore PRCC configs */
		pm_save_config_PRCC();

		pm_save_config_UART();

		/* here comes the wfi */
		__asm__ __volatile__(
			"dsb\n\t" "wfi\n\t" : : : "memory");

		/* Restore PRCC Configs */
		pm_restore_config_PRCC();

		/* Restore UART Configs */
		pm_restore_config_UART();

		if (readb(PRCM_ACK_MB0_AP_PWRST_STATUS) == 0xF3)
			printk(KERN_INFO "u8500-prcm : Woken from Sleep OK\n");

		break;
	case APEXECUTE_TO_APIDLE:
		/* Copy the current GIC set enable config as wakeup */
		for (val = 0; val < 4; val++) {
			tmp = readl(DIST_ENABLE_SET + (val * 4));
			writel(tmp, PRCM_ARMITMSK31TO0 + (val * 4));
		}

		/* PROGRAM WAKEUP EVENTS */
		/* write CfgWkUpsH in the Header */
		writeb(WKUPCFGH, PRCM_MBOX_HEADER_REQ_MB0);

		/* write to the mailbox */
		/* E8500Wakeup_ARMITMGMT Bit (1<<17). it is interpreted by
		 * the firmware to set the register prcm_armit_maskxp70_it
		 * (which is btw secure and thus only accessed by the xp70)
		 */
		writel((1<<17), PRCM_REQ_MB0_WKUP_8500);
		writel(0x0, PRCM_REQ_MB0_WKUP_4500);

		/* SIGNAL MAILBOX */
		/* set the MBOX_CPU_SET bit to set an IT to xP70 */
		writel(1 << 0, PRCM_MBOX_CPU_SET);

		/* wait for corresponding MB0X_CPU_VAL bit to be cleared */
		while ((readl(PRCM_MBOX_CPU_VAL) & (1 << 0)) && timeout--)
			cpu_relax();
		if (!timeout) {
			printk(KERN_INFO
			 "Timeout in prcmu_configure_wakeup_events!!\n");
			return -EBUSY;
		}

		/* CREATE MAILBOX FOR EXECUTE TO IDLE POWER TRANSITION */
		/* Write PwrStTrH=0 header to request a Power state xsition */
		writeb(0x0, PRCM_MBOX_HEADER_REQ_MB0);

		/* write request to the MBOX0 */
		writeb(APEXECUTE_TO_APIDLE, PRCM_REQ_MB0_PWRSTTRH_APPWRST);
		writeb(ON, PRCM_REQ_MB0_PWRSTTRH_APPLLST);
		writeb(ON, PRCM_REQ_MB0_PWRSTTRH_ULPCLKST);
		writeb(0x55, PRCM_REQ_MB0_PWRSTTRH_BYTEFILL);

		/* As per the sync logic, we are supposed to be the final CPU.
		 * If the other CPU isnt in wfi, better exit by putting
		 * ourselves in wfi
		 */
		if (smp_processor_id()) {
			if (!(readl(PRCM_ARM_WFI_STANDBY) & 0x8)) {
				printk(KERN_WARNING
					"Other CPU(CPU%d) is not in WFI!(0x%x)\
					 Aborting attempt\n",
					!smp_processor_id(),
					readl(PRCM_ARM_WFI_STANDBY));
				__asm__ __volatile__(
					"dsb\n\t" "wfi\n\t" : : : "memory");
				return 0;
			}
		} else /* running on CPU0, check for CPU1 WFI standby */ {
			if (!(readl(PRCM_ARM_WFI_STANDBY) & 0x10)) {
				printk(KERN_WARNING
					"Other CPU(CPU%d) is not in WFI!(0x%x)\
					Aborting attempt\n",
					!smp_processor_id(),
					readl(PRCM_ARM_WFI_STANDBY));
				__asm__ __volatile__(
					"dsb\n\t" "wfi\n\t" : : : "memory");
				return 0;
			}
		}

		/* FIXME : temporary hack to let UART2 interrupts enable the
		 *         wake-up from ARM
		 */
		if (!(readl(PRCM_ARMITMSK31TO0) & 0x04000000))
			writel((readl(PRCM_ARMITMSK31TO0) | 0x04000000),
					 PRCM_ARMITMSK31TO0);

		/* FIXME : tempoeary hack to wake up from RTC */
		if (!(readl(PRCM_ARMITMSK31TO0) & 0x40000))
			writel((readl(PRCM_ARMITMSK31TO0) | 0x40000),
					 PRCM_ARMITMSK31TO0);

		/* FIXME : later on, the ARM should not request a Idle if one
		 *	   of the ITSTATUS0/5 are still alive!!!
		 */
		if (readl(PRCM_ITSTATUS0) == 0x80) {
			printk(KERN_WARNING "PRCM_ITSTATUS0 Not cleared\n");
			__asm__ __volatile__(
				"dsb\n\t" "wfi\n\t" : : : "memory");
			return -EBUSY;
		}

		/*
		 *
		 * REQUEST POWER XSITION
		 *
		 */

		/* clear ACK MB0 */
		writeb(0x0, PRCM_ACK_MB0);

		/* trigger the XP70 IT10 to the XP70 */
		writel(1, PRCM_MBOX_CPU_SET);

		/* here comes the wfi */
		__asm__ __volatile__("dsb\n\t" "wfi\n\t" : : : "memory");

		writeb(RDWKUPACKH, PRCM_MBOX_HEADER_REQ_MB0);

		/* Set an interrupt to XP70 */
		writel(1 , PRCM_MBOX_CPU_SET);
		while ((readl(PRCM_MBOX_CPU_VAL) & 1) && timeout--)
			cpu_relax();
		if (!timeout)
			return -EBUSY;

		break;
	case APEXECUTE_TO_APDEEPSLEEP:
		printk(KERN_INFO "TODO: deep sleep \n");
		break;
	case APBOOT_TO_APEXECUTE:
		break;
	default:
		ret = -EINVAL;
	}
	return ret;


}
EXPORT_SYMBOL(prcmu_apply_ap_state_transition);



/**
 * prcmu_i2c_read - PRCMU - 4500 communication using PRCMU I2C
 * @reg: - db8500 register bank to be accessed
 * @slave:  - db8500 register to be accessed
 * Returns: ACK_MB5  value containing the status
 */
int prcmu_i2c_read(u8 reg, u8 slave)
{
	uint8_t i2c_status;
	uint8_t i2c_val;

	dbg_printk("\nprcmu_4500_i2c_read: \
	  bank=%x;reg=%x;\n", reg, slave);

	/* code related to FW 1_0_0_16 */
	writeb(I2CREAD, PRCM_REQ_MB5_I2COPTYPE);
	writeb(reg, PRCM_REQ_MB5_I2CREG);
	writeb(slave, PRCM_REQ_MB5_I2CSLAVE);
	writeb(0, PRCM_REQ_MB5_I2CVAL);

	/* clear any previous ack */
	writel(0, PRCM_ACK_MB5);

	/* Set an interrupt to XP70 */
	writel(PRCM_XP70_TRIG_IT17, PRCM_MBOX_CPU_SET);

	dbg_printk("\n readl PRCM_ARM_IT1_VAL =  %d \
			", readb(PRCM_ARM_IT1_VAL));
	dbg_printk("\nwaiting at wait queue");


	/* wait for interrupt */
	wait_event_interruptible(ack_mb5_queue, \
	  !(readl(PRCM_MBOX_CPU_VAL) & (PRCM_XP70_TRIG_IT17)));

	dbg_printk("\nAfter wait queue");

	/* retrieve values */
	dbg_printk("ack-mb5:transfer status = %x\n", \
			readb(PRCM_ACK_MB5 + 0x1));
	dbg_printk("ack-mb5:reg_add = %x\n", readb(PRCM_ACK_MB5 + 0x2));
	dbg_printk("ack-mb5:slave_add = %x\n", \
			readb(PRCM_ACK_MB5 + 0x2));
	dbg_printk("ack-mb5:reg_val = %d\n", readb(PRCM_ACK_MB5 + 0x3));


	/* return ack_mb5.req_field.reg_val for a
	   req->req_field.I2C_op == I2C_READ */

	i2c_status = readb(PRCM_ACK_MB5 + 0x1);
	i2c_val = readb(PRCM_ACK_MB5 + 0x3);


	if (i2c_status == I2C_RD_OK)
		return i2c_val;
	else {

		printk(KERN_INFO "prcmu_request_mailbox5:read return status = \
				%d\n", i2c_status);
		return -EINVAL;
	}

}
EXPORT_SYMBOL(prcmu_i2c_read);


/**
 * prcmu_i2c_write - PRCMU-db8500 communication using PRCMU I2C
 * @reg: - db8500 register bank to be accessed
 * @slave:  - db800 register to be written to
 * @reg_data: - the data to write
 * Returns: ACK_MB5 value containing the status
 */
int prcmu_i2c_write(u8 reg, u8 slave, u8 reg_data)
{
	uint8_t i2c_status;
	uint32_t timeout;

	/* NOTE : due to the I2C workaround, use
	 *	  MB5 for data, MB4 for the header
	 */

	/* request I2C workaround header 0x0F */
	writeb(0x0F, PRCM_MBOX_HEADER_REQ_MB4);

	/* prepare the data for mailbox 5 */

	/* register bank and I2C command */
	writeb((reg << 1) | I2CWRITE, PRCM_REQ_MB5 + 0x0);
	/* APE_I2C comm. specifics */
	writeb((1 << 3) | 0x0, PRCM_REQ_MB5 + 0x1);
	writeb(slave, PRCM_REQ_MB5_I2CSLAVE);
	writeb(reg_data, PRCM_REQ_MB5_I2CVAL);

	/* we request the mailbox 4 */
	writel(PRCM_XP70_TRIG_IT14, PRCM_MBOX_CPU_SET);

	/* we wait for ACK mailbox 5 */
	/* FIXME : regularise this code with mailbox constants */
	while ((readl(PRCM_ARM_IT1_VAL) & (0x1 << 5)) != (0x1 << 5))
		cpu_relax();

	/* we clear the ACK mailbox 5 interrupt */
	writel((0x1 << 5), PRCM_ARM_IT1_CLEAR);

	i2c_status = readb(PRCM_ACK_MB5 + 0x1);
	if (i2c_status == I2C_WR_OK)
		return I2C_WR_OK;
	else {
		printk(KERN_INFO "ape-i2c: i2c_status : 0x%x\n", i2c_status);
		return -EINVAL;
	}
}
EXPORT_SYMBOL(prcmu_i2c_write);

int prcmu_i2c_get_status()
{
	return readb(PRCM_ACK_MB5_STATUS);
}
EXPORT_SYMBOL(prcmu_i2c_get_status);

int prcmu_i2c_get_bank()
{
	return readb(PRCM_ACK_MB5_BANK);
}
EXPORT_SYMBOL(prcmu_i2c_get_bank);

int prcmu_i2c_get_reg()
{
	return readb(PRCM_ACK_MB5_REG);
}
EXPORT_SYMBOL(prcmu_i2c_get_reg);

int prcmu_i2c_get_val()
{
	return readb(PRCM_ACK_MB5_VAL);
}
EXPORT_SYMBOL(prcmu_i2c_get_val);



/**
 * prcmu_wakeup_modem - should be called whenever ARM wants to wakeup Modem
 *
 * Mailbox used : AckMb7
 * ACK          : HostPortAck
 */
int prcmu_arm_wakeup_modem()
{
	uint32_t prcm_hostaccess = readl(PRCM_HOSTACCESS_REQ);
	prcm_hostaccess = prcm_hostaccess | ARM_WAKEUP_MODEM;

	/* 1. write to the PRCMU host_port_req register */
	writel(prcm_hostaccess, PRCM_HOSTACCESS_REQ);

	/* 2. wait for HostPortAck message in AckMb7
	   wait_event_interruptible(ack_mb7_queue, (readl(PRCM_ARM_IT1_VAL)
				& (1<<7)));
				*/

	if (prcmu_read_ack_mb7() == HOST_PORT_ACK)
		return 0;
	else {
		printk(KERN_INFO "\nprcmu_arm_wakeup_modem: ack_mb7 \
				status = %x", prcmu_read_ack_mb7());
		return -EINVAL;
	}



}
EXPORT_SYMBOL(prcmu_arm_wakeup_modem);

/**
 * prcmu_arm_free_modem - called when ARM no longer needs to talk to modem
 *
 * Mailbox used - None
 * ACK          - None
 */
int prcmu_arm_free_modem()
{
	/* clear the PRCMU host_port_req register */
	uint32_t orig_val = readl(PRCM_HOSTACCESS_REQ);
	orig_val = orig_val & 0xFFFFFFFE;

	/* TODO - write the correct mask value here !! */
	writel(orig_val, PRCM_HOSTACCESS_REQ);

	return 0;
}
EXPORT_SYMBOL(prcmu_arm_free_modem);



/**
 * prcmu_configure_wakeup_events - configure 8500 and 4500 hw events on which
 * 			 PRCMU should wakeup AP
 *
 * Mailbox : PRCM_REQ_MB0
 * Header  : WKUPCFGH
 * ACK     : None
 */
int prcmu_configure_wakeup_events(uint32_t event_8500_mask, \
		uint32_t event_4500_mask)
{
	int timeout = 200;

	/* write CfgWkUpsH in the Header */
	writeb(WKUPCFGH, PRCM_MBOX_HEADER_REQ_MB0);

	/* write to the mailbox */
	writel(event_8500_mask, PRCM_REQ_MB0_WKUP_8500);
	writel(event_4500_mask, PRCM_REQ_MB0_WKUP_4500);

	/* set the corresponding MBOX_CPU_SET bit to set an interrupt to xP70 */
	writel(1 << 0, PRCM_MBOX_CPU_SET);

	/* wait for corresponding MB0X_CPU_VAL bit to be cleared */
	while ((readl(PRCM_MBOX_CPU_VAL) & (1 << 0)) && timeout--)
		cpu_relax();

	if (!timeout) {
		printk(KERN_INFO "\nTimeout prcmu_configure_wakeup_events\n");
	} else {
		printk(KERN_INFO "\nprcmu_configure_wakeup_events \
				done successfully!!\n");
	}

	/* No ack for this service. Directly return */
	return 0;



}
EXPORT_SYMBOL(prcmu_configure_wakeup_events);


/**
 * prcmu_get_wakeup_reason - Retrieve the event which caused AP wakeup
 * @event_8500: - corresponds to 8500 events
 * @event_4500: - corresponds to 4500 events
 *
 *
 * Mailbox : PRCM_ACK_MB0
 * Header  : WKUPH
 * ACK     : None
 */
int prcmu_get_wakeup_reason(uint32_t *event_8500, \
		unsigned char *event_4500 /* 20 bytes */)
{

	int i = 0;

	/* read the Rdp field */
	uint8_t rdp = readb(PRCM_ACK_MB0_PINGPONG_RDP);

	/* right now, ping pong not in place :) */
	/* read the event fields */
	if (rdp == 0) {
		*event_8500 = readl(PRCM_ACK_MB0_WK0_EVENT_8500);
		for (i = 0; i < PRCM_ACK_MB0_EVENT_4500_NUMBERS; i++)
			event_4500[i] = readb(PRCM_ACK_MB0_WK0_EVENT_4500);
	} else {
		*event_8500 = readl(PRCM_ACK_MB0_WK1_EVENT_8500);
		for (i = 0; i < PRCM_ACK_MB0_EVENT_4500_NUMBERS; i++)
			event_4500[i] = readb(PRCM_ACK_MB0_WK1_EVENT_8500);

	}


	/* No ack. Directly return */
	return 0;


}
EXPORT_SYMBOL(prcmu_get_wakeup_reason);


/**
 * prcmu_clear_wakeup_reason - Clear the wakeup fields in AckMb0 after reading
 *
 *
 * Mailbox : PRCM_ACK_MB0
 * Header  : None
 * ACK     : None
 */
int prcmu_clear_wakeup_reason()
{
	int i = 0;

	/* write the header WKUPH for AckMb0 */
	writeb(WKUPH, PRCM_MBOX_HEADER_ACK_MB0);

	/* Ping pong not in place in firmware as of now */
	/* read the Rdp field */
	uint8_t rdp = readb(PRCM_ACK_MB0_PINGPONG_RDP);

	/* clear the event fields */
	if (rdp == 0) {
		writel(0, PRCM_ACK_MB0_WK0_EVENT_8500);
		for (i = 0; i < 20; i++)
			writeb(0, PRCM_ACK_MB0_WK0_EVENT_4500 + i);
	} else {
		writel(0, PRCM_ACK_MB0_WK1_EVENT_8500);
		for (i = 0; i < 20; i++)
			writeb(0, PRCM_ACK_MB0_WK1_EVENT_4500 + i);

	}


	/* No ack. Directly return */
	return 0;
}
EXPORT_SYMBOL(prcmu_clear_wakeup_reason);


/**
 * prcmu_ack_wakeup_reason - Arm acks to PRCMU that it read the wakeup reason
 *
 *
 * Mailbox used : PRCM_ACK_MB0
 * Header used  : RDWKUPACKH
 * ACK          : None
 */
int prcmu_ack_wakeup_reason()
{
	uint8_t rdp;

	/* write RDWKUPACKH in the Header */
	writeb(RDWKUPACKH, PRCM_MBOX_HEADER_REQ_MB0);

	/* toggle the Rdp field in AckMb0 */
	rdp = readb(PRCM_ACK_MB0_PINGPONG_RDP);
	rdp = !rdp;
	writeb(rdp, PRCM_ACK_MB0_PINGPONG_RDP);

	/*No ack for this service. Directly return */
	return 0;
}
EXPORT_SYMBOL(prcmu_ack_wakeup_reason);


/**
 * prcmu_set_callback_modem_wakes_arm - Set the callback function to call
 * 			when modem requests for Arm wakeup
 *
 * Mailbox used - None
 * Ack - None
 */
void prcmu_set_callback_modem_wakes_arm(void (*func)(void))
{
	prcmu_modem_wakes_arm_shrm = func;
}
EXPORT_SYMBOL(prcmu_set_callback_modem_wakes_arm);


/**
 * prcmu_set_callback_modem_reset_request - Set the callback function to call
 * 			when modem requests for Reset
 *
 * Mailbox used - None
 * Ack - None
 */
void prcmu_set_callback_modem_reset_request(void (*func)(void))
{
	prcmu_modem_reset_shrm = func;
}
EXPORT_SYMBOL(prcmu_set_callback_modem_reset_request);



/**
 * prcmu_read_ack_mb7 - Read the AckMb7 Status message
 *
 *
 * Mailbox  : AckMb7
 * Header   : None
 * ACK      : None
 * associated IT : prcm_arm_it1_val[7]
 */
int prcmu_read_ack_mb7()
{
	return readb(PRCM_ACK_MB7);
}
EXPORT_SYMBOL(prcmu_read_ack_mb7);


/**
 * prcmu_ack_mb0_wkuph_status_tasklet - tasklet for ack mb0 IRQ
 * @tasklet_data: 	tasklet data
 *
 * Tasklet for handling wakeup events given by IRQ corresponding to
 * AckMb0
 */
void prcmu_ack_mb0_wkuph_status_tasklet(unsigned long tasklet_data)
{
	uint32_t event_8500 = 0;
	unsigned char event_4500[20];

	prcmu_get_wakeup_reason(&event_8500, event_4500);

	/* RTC wakeup signal */
	if (event_8500 & (1 << 0)) {

	}

	/* RTT0 wakeup signal */
	if (event_8500 & (1 << 1)) {

	}

	/* RTT1 wakeup signal */
	if (event_8500 & (1 << 2)) {

	}

	/* HSI0 wakeup signal */
	if (event_8500 & (1 << 3)) {

	}

	/* HSI1 wakeup signal */
	if (event_8500 & (1 << 4)) {

	}

	/* ca_wake_req signal  - modem wakes up ARM */
	if (event_8500 & (1 << 5)) {
		/* call shrm driver sequence */
		if (prcmu_modem_wakes_arm_shrm != NULL) {
			(*prcmu_modem_wakes_arm_shrm)();

			/* clear the wakeup reason */
			prcmu_clear_wakeup_reason();
		} else {
			printk(KERN_INFO "\n prcmu: SHRM callback for \
					ca_wake_req not registered!!\n");
		}

	}

	/* USB wakeup signal */
	if (event_8500 & (1<<6)) {

	}

	/* ape_int_4500 signal */
	if (event_8500 & (1<<7)) {

	}


	/* fifo4500it signal */
	if (event_8500 & (1<<8)) {

	}



}

/**
 * prcmu_ack_mb7_status_tasklet - tasklet for mb7 IRQ
 * @tasklet_data: 	tasklet data
 *
 * Tasklet for handling IPC communication related to AckMb7
 */
void prcmu_ack_mb7_status_tasklet(unsigned long tasklet_data)
{
	/* read the ack mb7 value and carry out actions accordingly */
	uint8_t ack_mb7 = readb(PRCM_ACK_MB7);

	switch (ack_mb7) {
	case MOD_SW_RESET_REQ:
		/*forward the reset request to ARM */
		break;
	case CA_SLEEP_REQ:
		/* modem no longer requires to communicate
		 * with ARM so ARM can go to sleep */
		break;
	case HOST_PORT_ACK:
		/* modem up.ARM can communicate to modem */
		/* wake_up_interruptible- prcmu_arm_wakeup_modem api*/
		wake_up_interruptible(&ack_mb7_queue);
		break;
	};

}

/**
 * prcmu_ack_mbox_irq_handler - IRQ handler for Ack mailboxes
 * @irq:	the irq number
 *
 * IRQ Handler for the Ack Mailboxes
 * Find out the arm_it1_val bit and carry out the tasks
 * accordingly
 */
irqreturn_t prcmu_ack_mbox_irq_handler(int irq, void *ctrlr)
{
	uint8_t prcm_arm_it1_val = 0;

	dbg_printk("\nInside prcmu_ack_mbox_irq_handler !!\n");

	/* check the prcm_arm_it1_val register to find which ACK mailbox is
	 * filled by PRCMU fw */
	prcm_arm_it1_val = readb(PRCM_ARM_IT1_VAL);

	dbg_printk(" prcm_arm_it1_val = %d ", prcm_arm_it1_val);

	if (prcm_arm_it1_val & (1 << 0)) {
		dbg_printk("\n Inside IRQ handler for Ack mb0 ");
		wake_up_interruptible(&ack_mb0_queue);

		dbg_printk("\n readb(PRCM_MBOX_HEADER_ACK_MB0) = %x", \
				readb(PRCM_MBOX_HEADER_ACK_MB0));

		/*read the header */
		if ((readb(PRCM_MBOX_HEADER_ACK_MB0)) == PWRSTTRH) {
			/* call the callback for AckMb0_PWRSTTRH */
			/*tasklet_schedule(&prcmu_ack_mb0_pwrst_tasklet);*/
			/* call wake_up_event_interruptible for mb0 */
			dbg_printk("\n Inside IRQ handler for Ack mb0  \
					PWRSTTRH and waking up ");

		} else if ((readb(PRCM_MBOX_HEADER_ACK_MB0)) == WKUPH) {
			dbg_printk("\n IRQ handler for Ack mb0 WKUPH  ");
			tasklet_schedule(&prcmu_ack_mb0_wkuph_tasklet);

		}
	} else if (prcm_arm_it1_val & (1<<1)) {

	} else if (prcm_arm_it1_val & (1<<2)) {

	} else if (prcm_arm_it1_val & (1<<3)) {

	} else if (prcm_arm_it1_val & (1<<4)) {

	} else if (prcm_arm_it1_val & (1<<5)) {
		/* No header reading required */
		/* call wake_up_event_interruptible for mb5 transaction */
		dbg_printk("\nInside prcmu IRQ handler for mb5 and calling wakeup");
		wake_up_interruptible(&ack_mb5_queue);
	} else if (prcm_arm_it1_val & (1<<6)) {

	} else if (prcm_arm_it1_val & (1<<7)) {
		 /* No header reading required */
		tasklet_schedule(&prcmu_ack_mb7_tasklet);
	}

	/* clear arm_it1_val bits */
	writeb(255, PRCM_ARM_IT1_CLEAR);
	return IRQ_HANDLED;
}


/**
 * prcmu_fw_init - arch init call for the Linux PRCMU fw init logic
 *
 */
static int prcmu_fw_init(void)
{
	int err = 0;
	/* configure the wake-up events */
	uint32_t event_8500 = (1 << 17) | (1 << 0);
	uint32_t event_4500 = 0x0;

	if (u8500_is_earlydrop()) {
		int i;
		int status = prcmu_get_boot_status();
		if (status != 0xFF || status != 0x2F)
		{
			printk("PRCMU Firmware not ready\n");
			return -EIO;
		}
		/* This can be enabled once PRCMU fw flashing is default */
		prcmu_apply_ap_state_transition(APBOOT_TO_APEXECUTE_ED,
				DDR_PWR_STATE_UNCHANGED_ED, 0);

		/* for the time being assign wake-up duty to all interrupts */
		/* Enable all interrupts as wake-up source */
		for (i = 0; i < 128; i++)
			prcmu_set_irq_wakeup(i);

		return 0;
	}

	/* clear the arm_it1_val to low the IT#47 */
	writeb(0xFF, PRCM_ARM_IT1_CLEAR);

	/* init irqs */
        err = request_irq(IRQ_PRCM_ACK_MBOX, prcmu_ack_mbox_irq_handler, \
			IRQF_TRIGGER_RISING, "prcmu_ack_mbox", NULL);
        if (err<0) {
                printk(KERN_ERR "\nUnable to allocate irq IRQ_PRCM_ACK_MBOX!!\n");
                err = -EBUSY;
                free_irq(IRQ_PRCM_ACK_MBOX,NULL);
		goto err_return;
        }


        if (prcmu_get_xp70_current_state() == AP_BOOT)
                prcmu_apply_ap_state_transition(APBOOT_TO_APEXECUTE, DDR_PWR_STATE_UNCHANGED, 0);
        else if (prcmu_get_xp70_current_state() == AP_EXECUTE) {
	        printk(KERN_INFO "PRCMU firmware booted.\n");
	}
        else {
                printk(KERN_WARNING "WARNING - PRCMU firmware not yet booted!!!\n");
                return -ENODEV;
        }

	/* write CfgWkUpsH in the Header */
	writeb(WKUPCFGH, PRCM_MBOX_HEADER_REQ_MB0);

	/* write to the mailbox */
	/* E8500Wakeup_ARMITMGMT Bit (1<<17). it is interpreted by
	 * the firmware to set the register prcm_armit_maskxp70_it
	 * (which is btw secure and thus only accessed by the xp70)
	 */
	writel(event_8500, PRCM_REQ_MB0_WKUP_8500);
	writel(event_4500, PRCM_REQ_MB0_WKUP_4500);

	_wait_for_req_complete(REQ_MB0);

err_return:
	return err;
}

arch_initcall(prcmu_fw_init);
