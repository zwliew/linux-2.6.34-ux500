/*
 * Copyright (C) STMicroelectronics 2009
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 * Author: Kumar Sanghvi <kumar.sanghvi@stericsson.com>
 * Author: Sundar Iyer <sundar.iyer@stericsson.com>
 *
 * U8500 PRCM header definitions for u8500 ED cut
 */

#ifndef __MACH_PRCMU_FW_ED_H
#define __MACH_PRCMU_FW_ED_H

#include <mach/prcmu-fw-defs_ed.h>

#define _PRCMU_TCDM_BASE_ED  IO_ADDRESS(U8500_PRCMU_TCDM_BASE)
#define PRCM_BOOT_STATUS_ED    (_PRCMU_TCDM_BASE + 0xFFF)
#define PRCM_ROMCODE_A2P_ED    (_PRCMU_TCDM_BASE + 0xFFE)
#define PRCM_ROMCODE_P2A_ED    (_PRCMU_TCDM_BASE + 0xFFD)
#define PRCM_AP_PWR_STATE_ED   (_PRCMU_TCDM_BASE + 0xFFC)
#define PRCM_NOT_USED_ED       (_PRCMU_TCDM_BASE + 0xFFB)

#define PRCM_REQ_MB0_ED        (_PRCMU_TCDM_BASE + 0xFF8)
#define PRCM_DDRPWR_ST_ED      (PRCM_REQ_MB0 + 0x2)	/* 0xFFA */
#define PRCM_4500_FIFOWU_ED    (PRCM_REQ_MB0 + 0x1)	/* 0xFF9 */
#define PRCM_APPWR_STTR_ED     (PRCM_REQ_MB0 + 0x0)	/* 0xFF8 */

#define PRCM_REQ_MB1_ED        (_PRCMU_TCDM_BASE + 0xFF6)
#define PRCM_APE_OPP_ED        (PRCM_REQ_MB1 + 0x1)	/* 0xFF7 */
#define PRCM_ARM_OPP_ED        (PRCM_REQ_MB1 + 0x0)	/* 0xFF6 */

#define PRCM_REQ_MB2_ED        (_PRCMU_TCDM_BASE + 0xFEC) /* 0xFEC - 0xFF5 */

#define PRCM_ACK_MB0_ED        (_PRCMU_TCDM_BASE + 0xFE8)
#define PRCM_M2A_STATUS_ED     (PRCM_ACK_MB0 + 0x3)	/* 0xFEB */
#define PRCM_M2A_ERROR_ED      (PRCM_ACK_MB0 + 0x2)	/* 0xFEA */
#define PRCM_M2A_DVFS_STAT_ED  (PRCM_ACK_MB0 + 0x1)	/* 0xFE9 */
#define PRCM_M2A_HWACC_STAT_ED (PRCM_ACK_MB0 + 0x0)	/* 0xFE8 */

#define PRCM_REQ_MB6_ED        (_PRCMU_TCDM_BASE + 0xFE7)
#define PRCM_SVAMMDSPSTATUS_ED PRCM_REQ_MB6

#define PRCM_REQ_MB7_ED        0xFE6
#define PRCM_SIAMMDSPSTATUS_ED PRCM_REQ_MB7

enum mailbox_ed_t {
	REQ_MB0_ED = 0,	/* Uses XP70_IT_EVENT_10 */
	REQ_MB1_ED = 1,	/* Uses XP70_IT_EVENT_11 */
	REQ_MB2_ED = 2,	/* Uses XP70_IT_EVENT_12 */
};

/* Union declaration */

/* ARM to XP70 mailbox definition */
union req_mb0_ed_t {
	struct {
		enum ap_pwrst_trans_ed_t ap_pwrst_trans:8;
		enum intr_wakeup_ed_t fifo_4500wu:8;
		enum ddr_pwrst_ed_t ddr_pwrst:8;
		unsigned int unused:8;
	} req_field;
	unsigned int complete_field;
};

/* mailbox definition for ARM/APE operation */
union req_mb1_ed_t {
	struct {
		enum arm_opp_ed_t arm_opp:8;
		enum ape_opp_ed_t ape_opp:8;
	} req_field;
	unsigned short complete_field;
};

/* mailbox definition for hardware accelerator */
union req_mb2_ed_t {
	struct {
		enum hw_accst_ed_t hw_accst:8;
	} hw_accst_list[ESRAM4_ED + 1];
	unsigned int complete_field[3];
};

struct arm_irq_mask_ed_t {
	unsigned int arm_irq_mask[4];
};

#endif /* __MACH_PRCMU_FW_ED_H */
