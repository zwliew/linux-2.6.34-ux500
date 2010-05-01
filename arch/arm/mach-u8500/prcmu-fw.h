/*
 * Copyright 2009 ST-Ericsson.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef __MACH_PRCMU_FW_H
#define __MACH_PRCMU_FW_H

#include <mach/prcmu-fw-api.h>

#define _PRCMU_TCDM_BASE     IO_ADDRESS(U8500_PRCMU_TCDM_BASE)
#define PRCM_BOOT_STATUS    (_PRCMU_TCDM_BASE + 0xFFF)
#define PRCM_ROMCODE_A2P    (_PRCMU_TCDM_BASE + 0xFFE)
#define PRCM_ROMCODE_P2A    (_PRCMU_TCDM_BASE + 0xFFD)
#define PRCM_AP_PWR_STATE   (_PRCMU_TCDM_BASE + 0xFFC)
#define PRCM_NOT_USED       (_PRCMU_TCDM_BASE + 0xFFB)

#define PRCM_REQ_MB0        (_PRCMU_TCDM_BASE + 0xFF8)
#define PRCM_DDRPWR_ST      (PRCM_REQ_MB0 + 0x2)	/* 0xFFA */
#define PRCM_4500_FIFOWU    (PRCM_REQ_MB0 + 0x1)	/* 0xFF9 */
#define PRCM_APPWR_STTR     (PRCM_REQ_MB0 + 0x0)	/* 0xFF8 */

#define PRCM_REQ_MB1        (_PRCMU_TCDM_BASE + 0xFF6)
#define PRCM_APE_OPP        (PRCM_REQ_MB1 + 0x1)	/* 0xFF7 */
#define PRCM_ARM_OPP        (PRCM_REQ_MB1 + 0x0)	/* 0xFF6 */

#define PRCM_REQ_MB2        (_PRCMU_TCDM_BASE + 0xFEC)	/* 0xFEC - 0xFF5 */

#define PRCM_ACK_MB0        (_PRCMU_TCDM_BASE + 0xFE8)
#define PRCM_M2A_STATUS     (PRCM_ACK_MB0 + 0x3)	/* 0xFEB */
#define PRCM_M2A_ERROR      (PRCM_ACK_MB0 + 0x2)	/* 0xFEA */
#define PRCM_M2A_DVFS_STAT  (PRCM_ACK_MB0 + 0x1)	/* 0xFE9 */
#define PRCM_M2A_HWACC_STAT (PRCM_ACK_MB0 + 0x0)	/* 0xFE8 */

#define PRCM_REQ_MB6        (_PRCMU_TCDM_BASE + 0xFE7)
#define PRCM_SVAMMDSPSTATUS PRCM_REQ_MB6

#define PRCM_REQ_MB7        0xFE6
#define PRCM_SIAMMDSPSTATUS PRCM_REQ_MB7

typedef enum {
	REQ_MB0 = 0,	/* Uses XP70_IT_EVENT_10 */
	REQ_MB1 = 1,	/* Uses XP70_IT_EVENT_11 */
	REQ_MB2 = 2,	/* Uses XP70_IT_EVENT_12 */
} mailbox_t;

/* Union declaration */

/* ARM to XP70 mailbox definition */
typedef union {
	struct {
		ap_pwrst_trans_t ap_pwrst_trans:8;
		intr_wakeup_t fifo_4500wu:8;
		ddr_pwrst_t ddr_pwrst:8;
		unsigned int unused:8;
	} req_field;
	unsigned int complete_field;
} req_mb0_t;

/* mailbox definition for ARM/APE operation */
typedef union {
	struct {
		arm_opp_t arm_opp:8;
		ape_opp_t ape_opp:8;
	} req_field;
	unsigned short complete_field;
} req_mb1_t;

/* mailbox definition for hardware accelerator */
typedef union {
	struct {
		hw_accst_t hw_accst:8;
	} hw_accst_list[ESRAM4 + 1];
	unsigned int complete_field[3];
} req_mb2_t;

typedef struct {
	unsigned int arm_irq_mask[4];
} arm_irq_mask_t;

#endif /* __MACH_PRCMU_FW_H */
