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
#include <linux/io.h>

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

struct hw_usage{
	char b2r2:1;
	char mcde:1;
	char esram1:1;
	char esram2:1;
	char esram3:1;
	char esram4:1;
};
struct hw_usage hw_usg_state = {0};

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

/* Spinlocks */
spinlock_t req_mb0_lock;
spinlock_t req_mb1_lock;
spinlock_t req_mb2_lock;
spinlock_t req_mb3_lock;
spinlock_t req_mb4_lock;
spinlock_t req_mb5_lock;
spinlock_t req_mb6_lock;
spinlock_t req_mb7_lock;
spinlock_t irq_wake_lock;
spinlock_t ac_wake_lock;
spinlock_t ca_wake_lock;

/* Global var to determine if ca_wake_req is pending or not */
static int ca_wake_req_pending;

/* function pointer for shrm callback sequence for modem waking up arm */
static void (*prcmu_ca_wake_req_shrm_callback)(u8);

/* function pointer for shrm callback sequence for modem requesting reset */
static void (*prcmu_modem_reset_shrm)(void);

static int prcmu_set_hwacc_st(enum hw_acc_t hw_acc, enum hw_accst_t hw_accst);

/* Internal functions */

/**
 * _wait_for_req_complete () -  used for PWRSTTRH for AckMb0 and AckMb1 on V1
 * @num:			Mailbox number to operate on
 *
 * Polling loop to ensure that PRCMU FW has completed the requested op
 */
int _wait_for_req_complete(enum mailbox_t num)
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
int prcmu_request_mailbox0(union req_mb0_t *req)
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
int prcmu_request_mailbox1(union req_mb1_t *req)
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
int prcmu_request_mailbox2(union req_mb2_t *req)
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
	if (u8500_is_earlydrop())
		return readb(PRCM_BOOT_STATUS_ED);
	else
		return readb(PRCM_BOOT_STATUS);
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
int prcmu_set_rc_a2p(enum romcode_write_t val)
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
enum romcode_read_t prcmu_get_rc_p2a(void)
{
	if (u8500_is_earlydrop())
		return readb(PRCM_ROMCODE_P2A_ED);


	return readb(PRCM_ROMCODE_P2A);
}
EXPORT_SYMBOL(prcmu_get_rc_p2a);

/**
 * prcmu_get_current_mode - Return the current XP70 power mode
 * Returns: Returns the current AP(ARM) power mode: init,
 * apBoot, apExecute, apDeepSleep, apSleep, apIdle, apReset
 */
enum ap_pwrst_t prcmu_get_xp70_current_state(void)
{
	if (u8500_is_earlydrop())
		return readb(PRCM_AP_PWR_STATE_ED);


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
int prcmu_set_ap_mode(enum ap_pwrst_trans_t ap_pwrst_trans)
{
	union req_mb0_t request = { {0} };

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
int prcmu_set_fifo_4500wu(enum intr_wakeup_t fifo_4500wu)
{
	union req_mb0_t request = { {0} };

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
int prcmu_set_ddr_pwrst(enum ddr_pwrst_t ddr_pwrst)
{
	union req_mb0_t request = { {0} };

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
int prcmu_set_arm_opp(enum arm_opp_t arm_opp)
{
	int timeout = 200;
	enum arm_opp_t current_arm_opp;
	unsigned long flags;

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

	/* protect the writes for concurrent access */
	spin_lock_irqsave(&req_mb1_lock, flags);

	/* clear the mailbox */
	writel(0, PRCM_ACK_MB1);

	/* write 0x0 into the header for ARM/APE operating point mgmt */
	writel(0x0, _PRCM_MBOX_HEADER);

	/* write ARMOPP request into the mailbox */
	writel(arm_opp, PRCM_REQ_MB1_ARMOPP);
	writel(APE_NO_CHANGE, PRCM_REQ_MB1_APEOPP);

	_wait_for_req_complete(REQ_MB1);

	spin_unlock_irqrestore(&req_mb1_lock, flags);

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
int prcmu_set_ape_opp(enum ape_opp_t ape_opp)
{
	int timeout = 200;
	enum ape_opp_t current_ape_opp;
	unsigned long flags;

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

	/* protect writes for concurrent access */
	spin_lock_irqsave(&req_mb1_lock, flags);

	/* clear the mailbox */
	writel(0, PRCM_ACK_MB1);

	/* write 0x0 into the header for ARM/APE operating point mgmt */
	writel(0x0, _PRCM_MBOX_HEADER);

	/* write ARMOPP request into the mailbox */
	writel(ARM_NO_CHANGE, PRCM_REQ_MB1_ARMOPP);
	writel(ape_opp, PRCM_REQ_MB1_APEOPP);

	_wait_for_req_complete(REQ_MB1);

	spin_unlock_irqrestore(&req_mb1_lock, flags);

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


int prcmu_set_hwacc(enum hw_acc_dev hw_acc, enum hw_accst_t hw_accst)
{
	enum hw_acc_t hw_acc_device;

	/* check on which device is being used */
	switch (hw_acc) {
	case HW_ACC_SVAMMDSP:
		hw_acc_device = SVAMMDSP;
		break;
	case HW_ACC_SVAPIPE:
		hw_acc_device = SVAPIPE;
		break;
	case HW_ACC_SIAMMDSP:
		hw_acc_device = SIAMMDSP;
		break;
	case HW_ACC_SIAPIPE:
		hw_acc_device = SIAPIPE;
		break;
	case HW_ACC_SGA:
		hw_acc_device = SGA;
		break;
	case HW_ACC_B2R2:
		if (hw_accst == HW_ON)
			hw_usg_state.b2r2 = 1;
		else if (hw_accst == HW_OFF)
			hw_usg_state.b2r2 = 0;

		if (hw_usg_state.mcde == 1)
			return -EINVAL;
		hw_acc_device = B2R2MCDE;
		break;
	case HW_ACC_MCDE:
		if (hw_accst == HW_ON)
			hw_usg_state.mcde = 1;
		else if (hw_accst == HW_OFF)
			hw_usg_state.mcde = 0;

		if (hw_usg_state.b2r2 == 1)
			return -EINVAL;
		hw_acc_device = B2R2MCDE;
		break;
	case HW_ACC_ESRAM1:
		if (hw_accst == HW_ON)
			hw_usg_state.esram1 = 1;
		else if (hw_accst == HW_OFF)
			hw_usg_state.esram1 = 0;

		if (hw_usg_state.esram2 == 1)
			return -EINVAL;
		hw_acc_device = ESRAM1;
		break;
	case HW_ACC_ESRAM2:
		if (hw_accst == HW_ON)
			hw_usg_state.esram2 = 1;
		else if (hw_accst == HW_OFF)
			hw_usg_state.esram2 = 0;

		if (hw_usg_state.esram1 == 1)
			return -EINVAL;
		hw_acc_device = ESRAM2;
		break;
	case HW_ACC_ESRAM3:
		if (hw_accst == HW_ON)
			hw_usg_state.esram3 = 1;
		else if (hw_accst == HW_OFF)
			hw_usg_state.esram3 = 0;

		if (hw_usg_state.esram4 == 1)
			return -EINVAL;
		hw_acc_device = ESRAM3;
		break;
	case HW_ACC_ESRAM4:
		if (hw_accst == HW_ON)
			hw_usg_state.esram4 = 1;
		else if (hw_accst == HW_OFF)
			hw_usg_state.esram4 = 0;

		if (hw_usg_state.esram3 == 1)
			return -EINVAL;
		hw_acc_device = ESRAM4;
		break;
	default:
		return -EINVAL;
	};
	return prcmu_set_hwacc_st(hw_acc_device, hw_accst);
}
EXPORT_SYMBOL(prcmu_set_hwacc);


/**
 * prcmu_set_hwacc_st - set the appropriate h/w accelerator to power mode
 * @hw_acc: the hardware accelerator being considered
 * @hw_accst: The new h/w accelerator state(on/off/retention)
 * Returns: 0 on success, non-zero on failure
 *
 * This function set the appropriate hardware accelerator to the requested
 * power mode.
 * The caller can check the status following this call.
 */
static int prcmu_set_hwacc_st(enum hw_acc_t hw_acc, enum hw_accst_t hw_accst)
{
	union req_mb2_t request;
	int err = 0;
	unsigned long flags;

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

	/* protect writes for concurrent access */
	spin_lock_irqsave(&req_mb2_lock, flags);

	/* write the header into mailbox 2 */
	writeb(DPS_H, PRCM_MBOX_HEADER_REQ_MB2);
	/* fill out the request */
	writel(0x0, PRCM_REQ_MB2_DPS_SVAMMDSP);
	writel(0x0, PRCM_REQ_MB2_DPS_SGA);
	writeb(hw_accst, PRCM_REQ_MB2 + hw_acc);
	/* request for completion */
	err = _wait_for_req_complete(REQ_MB2);

	spin_unlock_irqrestore(&req_mb2_lock, flags);
	/*printk("\n readb(PRCM_ACK_MB2) = %x\n", readb(PRCM_ACK_MB2));*/
	if (readb(PRCM_ACK_MB2) == HWACC_PWRST_OK)
		err = 0;
	else
		err = -EINVAL;
	return err;
}

/**
 * prcmu_get_m2a_status - Get the status or transition of the last request
 * Returns: The status from MBOX to ARM during the last request.
 * See the list of status/transitions for details.
 */
enum mbox_2_arm_stat_ed_t prcmu_get_m2a_status(void)
{
	return readb(PRCM_M2A_STATUS_ED);
}
EXPORT_SYMBOL(prcmu_get_m2a_status);

/**
 * prcmu_get_m2a_error - Get the error that occured in last request
 * Returns: The error from MBOX to ARM during the last request.
 * See the list of errors for details.
 */
enum mbox_to_arm_err_ed_t prcmu_get_m2a_error(void)
{
	return readb(PRCM_M2A_ERROR_ED);
}
EXPORT_SYMBOL(prcmu_get_m2a_error);

/**
 * prcmu_get_m2a_dvfs_status - Get the state of DVFS transition
 * Returns: Either that state transition on DVFS is on going
 * or completed, 0 if not used.
 */
enum dvfs_stat_t prcmu_get_m2a_dvfs_status(void)
{
	if (u8500_is_earlydrop())
		return readb(PRCM_M2A_DVFS_STAT_ED);


	return 0;
}
EXPORT_SYMBOL(prcmu_get_m2a_dvfs_status);

/**
 * prcmu_get_m2a_hwacc_status - Get the state of HW Accelerator
 * Returns: Either that state transition on hardware accelerator
 * is on going or completed, 0 if not used.
 */
enum mbox_2_arm_hwacc_pwr_stat_t prcmu_get_m2a_hwacc_status(void)
{
	if (u8500_is_earlydrop())
		return readb(PRCM_M2A_HWACC_STAT_ED);


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
	unsigned long flags;

	spin_lock_irqsave(&irq_wake_lock, flags);

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
	} else {
		spin_unlock_irqrestore(&irq_wake_lock, flags);
		return -EINVAL;
	}

	writel(1 << irq | readl((volatile unsigned long *)(mask_reg)),
			(volatile unsigned long *)(mask_reg));
	spin_unlock_irqrestore(&irq_wake_lock, flags);
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
}

void __iomem *uart_backup_base, *uart_register_base;

/* TODO : return success value to let decide to proceed
 * 	  for low power request or not
 */
void pm_save_config_UART(void)
{
	uart_register_base = ioremap(U8500_UART2_BASE, SZ_4K);
	if (!uart_register_base) {
		printk(KERN_WARNING
		 "u8500-prcm : cannot allocate uart register base\n");
	}

	uart_backup_base = kzalloc(SZ_4K, GFP_KERNEL);
	if (!uart_backup_base) {
		printk(KERN_WARNING
		 "u8500-prcm : cannot allocate uart register backup base\n");
	}

	/* Copy UART config */
	memcpy(uart_backup_base, uart_register_base, SZ_4K);
}

void __iomem *ab8500_backup_base, *ab8500_register_base;

void pm_restore_config_UART(void)
{
	memcpy(uart_register_base, uart_backup_base, SZ_4K);
}

/* Deep Sleep context save */

/* public context backup */
/* TODO : remove hard coded backup RAM addresses */
void a9ss_save_public_config(void)
{
	int val;

	/* this happens for CPU0 only as of now */
	/* CP15 SCTLR */
	__asm__ __volatile__(
			"mrc p15, 0, %0, c1, c1, 2"
			: "=r" (val)
			:
			: "cc");
	writel(val, IO_ADDRESS(0x80151F80));

	/* CP15 TTBR0 */
	__asm__ __volatile__(
			"mrc p15, 0, %0, c2, c0, 0"
			: "=r" (val)
			:
			: "cc");
	writel(val, IO_ADDRESS(0x80151F84));

	/* CP15 TTBR1 */
	__asm__ __volatile__(
			"mrc p15, 0, %0, c2, c0, 1"
			: "=r" (val)
			:
			: "cc");
	writel(val, IO_ADDRESS(0x80151F88));

	/* CP15 TTBCR */
	__asm__ __volatile__(
			"mrc p15, 0, %0, c2, c0, 2"
			: "=r" (val)
			:
			: "cc");
	writel(val, IO_ADDRESS(0x80151F8C));

	/* CP15 DACR */
	__asm__ __volatile__(
			"mrc p15, 0, %0, c3, c0, 0"
			: "=r" (val)
			:
			: "cc");
	writel(val, IO_ADDRESS(0x80151F90));

	/* CPSR */
	__asm__ __volatile__(
			"mrs %0, cpsr"
			: "=r" (val)
			:
			: "cc");
	writel(val, IO_ADDRESS(0x80151F98));

	return;
}

void a9ss_restore_public_config(void)
{
	int val;

	/* CPSR */
	val = readl(IO_ADDRESS(0x80151F98));
	__asm__ __volatile__(
			"msr cpsr, %0"
			:
			: "r" (val));

	/* CP15 DACR */
	val = readl(IO_ADDRESS(0x80151F90));
	__asm__ __volatile__(
			"mcr p15, 0, %0, c3, c0, 0"
			:
			: "r" (val));

	/* CP15 TTBCR */
	val = readl(IO_ADDRESS(0x80151F8C));
	__asm__ __volatile__(
			"mcr p15, 0, %0, c2, c0, 2"
			:
			: "r" (val));

	/* CP15 TTBR1 */
	val = readl(IO_ADDRESS(0x80151F88));
	__asm__ __volatile__(
			"mcr p15, 0, %0, c2, c0, 1"
			:
			: "r" (val));

	/* CP15 TTBR0 */
	val = readl(IO_ADDRESS(0x80151F84));
	__asm__ __volatile__(
			"mcr p15, 0, %0, c2, c0, 0"
			:
			: "r" (val));

	/* CP15 SCTLR */
	val = readl(IO_ADDRESS(0x80151F80));
	__asm__ __volatile__(
			"mrc p15, 0, %0, c1, c1, 2"
			:
			: "r" (val));

	return;
}

/* ARM SCU backup */
#include <mach/scu.h>
unsigned int scu_backup_config[5];

void a9ss_save_scu_config(void)
{
	scu_backup_config[0] = __raw_readl(IO_ADDRESS(U8500_SCU_BASE)
							+ SCU_CTRL);
	scu_backup_config[1] = __raw_readl(IO_ADDRESS(U8500_SCU_BASE)
							+ SCU_CONFIG);
	scu_backup_config[2] = __raw_readl(IO_ADDRESS(U8500_SCU_BASE)
							+ SCU_CPU_STATUS);
	scu_backup_config[3] = __raw_readl(IO_ADDRESS(U8500_SCU_BASE)
							+ SCU_INVALIDATE);

	return;
}

void a9ss_restore_scu_config(void)
{
	__raw_writel(scu_backup_config[0], IO_ADDRESS(U8500_SCU_BASE)
							+ SCU_CTRL);
	__raw_writel(scu_backup_config[1], IO_ADDRESS(U8500_SCU_BASE)
							+ SCU_CONFIG);
	__raw_writel(scu_backup_config[2], IO_ADDRESS(U8500_SCU_BASE)
							+ SCU_CPU_STATUS);
	__raw_writel(scu_backup_config[3], IO_ADDRESS(U8500_SCU_BASE)
							+ SCU_INVALIDATE);

	return;
}

/* FIXME : get these from platform/header files instead */
/* GIC BAse Address */
#define GIC_BASE_ADDR           IO_ADDRESS(0xA0411000)

/* ITs enabled for GIC. 104 is due to skipping of the STI and PPI sets.
 *  * Rfer page 648 of the DB8500V1 spec v2.5
 *   */
#define DIST_ENABLE_SET         (GIC_BASE_ADDR + 0x104)
#define DIST_PENDING_SET        (GIC_BASE_ADDR + 0x200)
#define DIST_ENABLE_CLEAR       (GIC_BASE_ADDR + 0x180)
#define DIST_ACTIVE_BIT         (GIC_BASE_ADDR + 0x304)

#define PRCM_DEBUG_NOPWRDOWN_VAL        IO_ADDRESS(0x80157194)
#define PRCM_POWER_STATE_VAL            IO_ADDRESS(0x8015725C)

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
int prcmu_apply_ap_state_transition(enum ap_pwrst_trans_t transition,
					enum ddr_pwrst_t ddr_state_req,
					int _4500_fifo_wakeup)
{
	int ret = 0;
	int sync = 0;
	unsigned int val = 0, timeout = 0, tmp ;

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
			__asm__ __volatile__("dsb\n\t" "wfi\n\t" \
					: : : "memory");
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

		prcmu_configure_wakeup_events((1 << 17), 0x0);

		spin_lock(&req_mb0_lock);

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
				__asm__ __volatile__(
					"dsb\n\t" "wfi\n\t" : : : "memory");
				spin_unlock(&req_mb0_lock);
				return 0;
			}
		} else /* running on CPU0, check for CPU1 WFI standby */ {
			if (!(readl(PRCM_ARM_WFI_STANDBY) & 0x10)) {
				__asm__ __volatile__(
					"dsb\n\t" "wfi\n\t" : : : "memory");
				spin_unlock(&req_mb0_lock);
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
			spin_unlock(&req_mb0_lock);
			return -EBUSY;
		}

		/*
		 *
		 * REQUEST POWER XSITION
		 *
		 */

		/* clear ACK MB0 */
		writeb(0x0, PRCM_ACK_MB0);

		_wait_for_req_complete(REQ_MB0);

		spin_unlock(&req_mb0_lock);

		printk(KERN_INFO "u8500-prcm : To Deep Sleep\n");

		/* Context Saving beings */

		pm_save_config_PRCC();

		pm_save_config_UART();

		a9ss_save_public_config();

		a9ss_save_scu_config();

		/* TODO : take a backup of the SP and the public rom code
		 *	  entry point
		 */

		/* here comes the wfi */
		__asm__ __volatile__(
				"dsb\n\t" "wfi\n\t" : : : "memory");

		a9ss_restore_scu_config();

		a9ss_restore_public_config();

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

		prcmu_configure_wakeup_events((1 << 17), 0x0);
#if 0
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
#endif
		spin_lock(&req_mb0_lock);

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
				__asm__ __volatile__(
					"dsb\n\t" "wfi\n\t" : : : "memory");
				spin_unlock(&req_mb0_lock);
				return 0;
			}
		} else /* running on CPU0, check for CPU1 WFI standby */ {
			if (!(readl(PRCM_ARM_WFI_STANDBY) & 0x10)) {
				__asm__ __volatile__(
					"dsb\n\t" "wfi\n\t" : : : "memory");
				spin_unlock(&req_mb0_lock);
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
			spin_unlock(&req_mb0_lock);
			return -EBUSY;
		}

		/*
		 *
		 * REQUEST POWER XSITION
		 *
		 */

		/* clear ACK MB0 */
		writeb(0x0, PRCM_ACK_MB0);

		_wait_for_req_complete(REQ_MB0);
		spin_unlock(&req_mb0_lock);


		/* here comes the wfi */
		__asm__ __volatile__("dsb\n\t" "wfi\n\t" : : : "memory");

		prcmu_configure_wakeup_events((1 << 5), 0x0);

		break;
	case APEXECUTE_TO_APDEEPSLEEP:
		/* PROGRAM THE ITMASK* registers for deep sleep  */
		writel(0x00000000, PRCM_ARMITMSK31TO0);
		writel(0x00000100, PRCM_ARMITMSK63TO32);
		writel(0x00000000, PRCM_ARMITMSK95TO64);
		writel(0x00000000, PRCM_ARMITMSK127TO96);

		/* we skip the GIC freeze due to the FIQ being
		 * not handled by the ARM later on
		 */
		prcmu_configure_wakeup_events((1 << 17), 0x0);

		spin_lock(&req_mb0_lock);

		/* CREATE MAILBOX FOR EXECUTE TO IDLE POWER TRANSITION */
		/* Write PwrStTrH=0 header to request a Power state xsition */
		writeb(0x0, PRCM_MBOX_HEADER_REQ_MB0);

		/* write request to the MBOX0 */
		writeb(APEXECUTE_TO_APDEEPSLEEP, PRCM_REQ_MB0_PWRSTTRH_APPWRST);
		writeb(OFF, PRCM_REQ_MB0_PWRSTTRH_APPLLST);
		writeb(ON, PRCM_REQ_MB0_PWRSTTRH_ULPCLKST);
		writeb(0x56, PRCM_REQ_MB0_PWRSTTRH_BYTEFILL);

		/* As per the sync logic, we are supposed to be the final CPU.
		 * If the other CPU isnt in wfi, better exit by putting
		 * ourselves in wfi
		 */
		if (smp_processor_id()) {
			if (!(readl(PRCM_ARM_WFI_STANDBY) & 0x8)) {
				__asm__ __volatile__(
					"dsb\n\t" "wfi\n\t" : : : "memory");
				spin_unlock(&req_mb0_lock);
				return 0;
			}
		} else /* running on CPU0, check for CPU1 WFI standby */ {
			if (!(readl(PRCM_ARM_WFI_STANDBY) & 0x10)) {
				__asm__ __volatile__(
					"dsb\n\t" "wfi\n\t" : : : "memory");
				spin_unlock(&req_mb0_lock);
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
			spin_unlock(&req_mb0_lock);
			return -EBUSY;
		}

		/*
		 *
		 * REQUEST POWER XSITION
		 *
		 */

		/* clear ACK MB0 */
		writeb(0x0, PRCM_ACK_MB0);

		_wait_for_req_complete(REQ_MB0);
		spin_unlock(&req_mb0_lock);

		printk(KERN_INFO "u8500-prcm : To Sleep\n");

		/* Restore PRCC configs */
		pm_save_config_PRCC();

		pm_save_config_UART();

		a9ss_save_public_config();

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
 *
 * Locking is responsibility of caller.
 * This API is called from Ab8500 read which has locking in place.
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
 *
 * Locking is responsibility of caller.
 * This API is called from Ab8500 write which has locking in place
 */
int prcmu_i2c_write(u8 reg, u8 slave, u8 reg_data)
{
	uint8_t i2c_status;

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
 * prcmu_ac_wake_req - should be called whenever ARM wants to wakeup Modem
 *
 * Mailbox used : AckMb7
 * ACK          : HostPortAck
 */
int prcmu_ac_wake_req()
{
	u32 prcm_hostaccess;
	unsigned long flags;
	int timeout = 1000;

	spin_lock_irqsave(&ac_wake_lock, flags);
	prcm_hostaccess = readl(PRCM_HOSTACCESS_REQ);
	prcm_hostaccess = prcm_hostaccess | ARM_WAKEUP_MODEM;

	/* write to the PRCMU host_port_req register */
	writel(prcm_hostaccess, PRCM_HOSTACCESS_REQ);
	spin_unlock_irqrestore(&ac_wake_lock, flags);

	/* 2. wait for HostPortAck message in AckMb7
	   wait_event_interruptible(ack_mb7_queue, (readl(PRCM_ARM_IT1_VAL)
				& (1<<7)));
				*/
	while ((prcmu_read_ack_mb7() != HOST_PORT_ACK) && timeout--)
		cpu_relax();

	if (timeout)
		return 0;
	else {
		dbg_printk(KERN_INFO "\nprcmu_ac_wake_req: ack_mb7 \
				status = %x", prcmu_read_ack_mb7());
		return -EINVAL;
	}

}
EXPORT_SYMBOL(prcmu_ac_wake_req);

/**
 * prcmu_ac_sleep_req - called when ARM no longer needs to talk to modem
 *
 * Mailbox used - None
 * ACK          - None
 */
int prcmu_ac_sleep_req()
{
	/* clear the PRCMU host_port_req register */
	u32 orig_val;
	unsigned long flags;

	spin_lock_irqsave(&ac_wake_lock, flags);
	orig_val = readl(PRCM_HOSTACCESS_REQ);
	orig_val = orig_val & 0xFFFFFFFE;

	writel(orig_val, PRCM_HOSTACCESS_REQ);
	spin_unlock_irqrestore(&ac_wake_lock, flags);

	return 0;
}
EXPORT_SYMBOL(prcmu_ac_sleep_req);



/**
 * prcmu_configure_wakeup_events - configure 8500 and 4500 hw events
 * @event_8500_mask:	db8500 wakeup events
 * @event_4500_mask:	Ab8500 wakeup events
 *
 * Mailbox : PRCM_REQ_MB0
 * Header  : WKUPCFGH
 * ACK     : None
 */
int prcmu_configure_wakeup_events(u32 event_8500_mask, \
		u32 event_4500_mask)
{
	int err = 0;
	unsigned long flags;

	spin_lock_irqsave(&req_mb0_lock, flags);

	/* write CfgWkUpsH in the Header */
	writeb(WKUPCFGH, PRCM_MBOX_HEADER_REQ_MB0);

	/* write to the mailbox */
	writel(event_8500_mask, PRCM_REQ_MB0_WKUP_8500);
	writel(event_4500_mask, PRCM_REQ_MB0_WKUP_4500);

	err = _wait_for_req_complete(REQ_MB0);
	spin_unlock_irqrestore(&req_mb0_lock, flags);

	if (err) {
		dbg_printk(KERN_INFO "\nTimeout configure_wakeup_events\n");
	} else {
		/*dbg_printk(KERN_INFO "\nprcmu_configure_wakeup_events \
				done successfully!!\n");*/
	}

	/* No ack for this service. Directly return */
	return err;



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
int prcmu_get_wakeup_reason(u32 *event_8500, u8 *event_4500 /* 20 bytes */)
{
	int i = 0;

	/* read the Rdp field */
	u8 rdp = readb(PRCM_ACK_MB0_PINGPONG_RDP);

	if (rdp) {
		*event_8500 = readl(PRCM_ACK_MB0_WK1_EVENT_8500);
		for (i = 0; i < PRCM_ACK_MB0_EVENT_4500_NUMBERS; i++)
			event_4500[i] = readb(PRCM_ACK_MB0_WK1_EVENT_4500 + i);
	} else {
		*event_8500 = readl(PRCM_ACK_MB0_WK0_EVENT_8500);
		for (i = 0; i < PRCM_ACK_MB0_EVENT_4500_NUMBERS; i++)
			event_4500[i] = readb(PRCM_ACK_MB0_WK0_EVENT_4500 + i);
	}

	/* No ack. Directly return */
	return 0;
}
EXPORT_SYMBOL(prcmu_get_wakeup_reason);


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
	int err = 0;
	unsigned long flags;

	spin_lock_irqsave(&req_mb0_lock, flags);
	/* Acknowledge the wakeup events */
	writeb(RDWKUPACKH, PRCM_MBOX_HEADER_REQ_MB0);
	err = _wait_for_req_complete(REQ_MB0);
	spin_unlock_irqrestore(&req_mb0_lock, flags);

	return err;
}
EXPORT_SYMBOL(prcmu_ack_wakeup_reason);


/**
 * prcmu_is_ca_wake_req_pending - determine if ca_wake_req is pending
 *
 * Mailbox used - None
 * Ack - None
 *
 * To be used by shrm driver to check if any ca_wake_req is pending
 * initially and service the request accordingly.
 */
int prcmu_is_ca_wake_req_pending(void)
{
	int retval;
	unsigned long flags;

	spin_lock_irqsave(&ca_wake_lock, flags);
	retval  = ca_wake_req_pending;
	if (ca_wake_req_pending)
		ca_wake_req_pending--;
	spin_unlock_irqrestore(&ca_wake_lock, flags);

	return retval;
}
EXPORT_SYMBOL(prcmu_is_ca_wake_req_pending);

/**
 * prcmu_set_callback_cawakereq - callback of shrm for ca_wake_req
 * @func:	Function pointer of shrm
 *
 * To call the registered shrm callback whenever ca_wake_req is got
 */
void prcmu_set_callback_cawakereq(void (*func)(u8))
{
	prcmu_ca_wake_req_shrm_callback = func;
}
EXPORT_SYMBOL(prcmu_set_callback_cawakereq);


/**
 * prcmu_set_callback_modem_reset_request - callback of shrm for reset
 * @func:	Function pointer of shrm
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
 * prcmu_system_reset - System reset
 *
 * Sets the APE_SOFRST register which fires interrupt to fw
 */
void prcmu_system_reset(void)
{
	writel(1, PRCM_APE_SOFTRST);
}
EXPORT_SYMBOL(prcmu_system_reset);

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
	unsigned long flags;

	prcmu_get_wakeup_reason(&event_8500, event_4500);

	dbg_printk("\nprcmu_ack_mb0_wkuph_status_tasklet:event_8500 = %x\n",
			event_8500);

	/* ca_wake_req signal  - modem wakes up ARM */
	if (event_8500 & (1 << 5)) {
		/* call shrm driver callback */
		if (prcmu_ca_wake_req_shrm_callback != NULL)
			(*prcmu_ca_wake_req_shrm_callback)(1);
		else {
			printk(KERN_INFO "\n prcmu: SHRM callback for \
					ca_wake_req not registered!!\n");
			/* SHRM driver not initialized */
			spin_lock_irqsave(&ca_wake_lock, flags);
			ca_wake_req_pending++;
			spin_unlock_irqrestore(&ca_wake_lock, flags);
		}
	}
	prcmu_ack_wakeup_reason();

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
	u8 ack_mb7 = readb(PRCM_ACK_MB7);

	switch (ack_mb7) {
	case MOD_SW_RESET_REQ:
		/*forward the reset request to ARM */
		break;
	case CA_SLEEP_REQ:
		/* modem no longer requires to communicate
		 * with ARM so ARM can go to sleep */
		if (prcmu_ca_wake_req_shrm_callback != NULL)
			(*prcmu_ca_wake_req_shrm_callback)(0);
		else {
			printk(KERN_INFO "\n prcmu: SHRM callback for \
					ca_wake_req not registered!!\n");
		}
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
 * @ctrlr:	control
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

	dbg_printk(" prcm_arm_it1_val = %x ", prcm_arm_it1_val);

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

		/* clear the bit 0 */
		writeb(0x01, PRCM_ARM_IT1_CLEAR);
		prcm_arm_it1_val = readb(PRCM_ARM_IT1_VAL);
		dbg_printk(" prcm_arm_it1_val = %d ", prcm_arm_it1_val);
	} else if (prcm_arm_it1_val & (1<<1)) {
		dbg_printk("\n IRQ handler for Ack mb1\n");

		/* clear the bit 1 */
		writeb(0x02, PRCM_ARM_IT1_CLEAR);
	} else if (prcm_arm_it1_val & (1<<2)) {
		dbg_printk("\n IRQ handler for Ack mb2\n");

		/* clear the bit 2 */
		writeb(0x04, PRCM_ARM_IT1_CLEAR);
	} else if (prcm_arm_it1_val & (1<<3)) {
		dbg_printk("\n IRQ handler for Ack mb3\n");

		/* clear the bit 3 */
		writeb(0x08, PRCM_ARM_IT1_CLEAR);
	} else if (prcm_arm_it1_val & (1<<4)) {
		dbg_printk("\n IRQ handler for Ack mb4\n");

		/* clear the bit 4 */
		writeb(0x10, PRCM_ARM_IT1_CLEAR);
	} else if (prcm_arm_it1_val & (1<<5)) {
		/* No header reading required */
		/* call wake_up_event_interruptible for mb5 transaction */
		dbg_printk("\nInside prcmu IRQ handler for mb5 ");
		wake_up_interruptible(&ack_mb5_queue);

		/* clear the bit 5 */
		writeb(0x20, PRCM_ARM_IT1_CLEAR);
	} else if (prcm_arm_it1_val & (1<<6)) {
		dbg_printk("\n IRQ handler for Ack mb6\n");

		/* clear the bit 6 */
		writeb(0x40, PRCM_ARM_IT1_CLEAR);
	} else if (prcm_arm_it1_val & (1<<7)) {
		/* No header reading required */
		dbg_printk("\n IRQ handler for Ack mb7\n");
		tasklet_schedule(&prcmu_ack_mb7_tasklet);

		/* clear the bit 7 */
		writeb(0x80, PRCM_ARM_IT1_CLEAR);
	}
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
	u32 event_8500 = 0x0;
	u32 event_4500 = 0x0;

	if (u8500_is_earlydrop()) {
		int i;
		int status = prcmu_get_boot_status();
		if (status != 0xFF || status != 0x2F) {
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

	/* init spinlocks */
	spin_lock_init(&req_mb0_lock);
	spin_lock_init(&req_mb1_lock);
	spin_lock_init(&req_mb2_lock);
	spin_lock_init(&req_mb3_lock);
	spin_lock_init(&req_mb4_lock);
	spin_lock_init(&req_mb5_lock);
	spin_lock_init(&req_mb6_lock);
	spin_lock_init(&req_mb7_lock);
	spin_lock_init(&irq_wake_lock);
	spin_lock_init(&ac_wake_lock);
	spin_lock_init(&ca_wake_lock);

	/* configure the wakeup events */
	event_8500 = (1 << 5);
	event_4500 = 0x0;
	prcmu_configure_wakeup_events(event_8500, event_4500);
	dbg_printk("(WkUpCfgOk=0xEA)PRCM_ACK_MB0_AP_PWRST_STATUS = %x\n",
			readb(PRCM_ACK_MB0_AP_PWRST_STATUS));

	/* init irqs */
	err = request_irq(IRQ_PRCM_ACK_MBOX,
			prcmu_ack_mbox_irq_handler, IRQF_TRIGGER_HIGH,
			"prcmu_ack_mbox", NULL);
	if (err < 0) {
		printk(KERN_ERR "\nFailed to allocate \
				IRQ_PRCM_ACK_MBOX!!\n");
		err = -EBUSY;
		goto err_return;
	}

	if (prcmu_get_xp70_current_state() == AP_BOOT)
		prcmu_apply_ap_state_transition(APBOOT_TO_APEXECUTE, \
				DDR_PWR_STATE_UNCHANGED, 0);
	else if (prcmu_get_xp70_current_state() == AP_EXECUTE) {
		printk(KERN_INFO "PRCMU firmware booted.\n");
	} else {
		printk(KERN_WARNING "WARNING - PRCMU firmware not yet booted!!!\n");
		return -ENODEV;
	}

	return 0;

err_return:
	free_irq(IRQ_PRCM_ACK_MBOX, NULL);
	return err;
}

arch_initcall(prcmu_fw_init);
