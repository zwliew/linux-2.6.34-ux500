/*
 * Copyright (C) STMicroelectronics 2009
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 * Author: Kumar Sanghvi <kumar.sanghvi@stericsson.com>
 *
 * PRCMU f/w APIs
 */
#ifndef __MACH_PRCMU_FW_API_H
#define __MACH_PRCMU_FW_API_H

#include <linux/interrupt.h>
#include "prcmu-fw-defs_v1.h"

/**
 * enum hw_acc_dev - enum for hw accelerators
 * @HW_ACC_SVAMMDSP: for SVAMMDSP
 * @HW_ACC_SVAPIPE:  for SVAPIPE
 * @HW_ACC_SIAMMDSP: for SIAMMDSP
 * @HW_ACC_SIAPIPE: for SIAPIPE
 * @HW_ACC_SGA: for SGA
 * @HW_ACC_B2R2: for B2R2
 * @HW_ACC_MCDE: for MCDE
 * @HW_ACC_ESRAM1: for ESRAM1
 * @HW_ACC_ESRAM2: for ESRAM2
 * @HW_ACC_ESRAM3: for ESRAM3
 * @HW_ACC_ESRAM4: for ESRAM4
 *
 * Different hw accelerators which can be turned ON/
 * OFF or put into retention the ESRAMs.
 * Used with EPOD API.
 */
enum hw_acc_dev{
	HW_ACC_SVAMMDSP = 0,
	HW_ACC_SVAPIPE = 1,
	HW_ACC_SIAMMDSP = 2,
	HW_ACC_SIAPIPE = 3,
	HW_ACC_SGA = 4,
	HW_ACC_B2R2 = 5,
	HW_ACC_MCDE = 6,
	HW_ACC_ESRAM1 = 7,
	HW_ACC_ESRAM2 = 8,
	HW_ACC_ESRAM3 = 9,
	HW_ACC_ESRAM4 = 10,
};

/**
 * union sia_sva_pwron_enable_t - Power ON for SIA-SVA
 * @req_field: requested state for hardware semaphore,
 * 	ab4500 fifo.
 * @complete_field:
 *
 */
union sia_sva_pwron_enable_t {
	struct {
		u8 hsem:1;
		u8 fifo_4500_it:1;
		u8 bit_fill:6;
	} req_field;
	u8 complete_field;
};

/**
 * struct sia_sva_auto_power_t - Auto power policy struct
 * @sia_pwr_policy:	SIA power policy
 * @sva_pwr_policy:	SVA power policy
 * @sia_pwron_enable:	SIA power ON Enable
 * @sva_pwron_enable:	SVA power ON Enable
 * @auto_pwron_enable:	Auto power ON Enable
 *
 */
struct sia_sva_auto_power_t {
	enum sia_sva_pwr_policy_t sia_pwr_policy:4;
	enum sia_sva_pwr_policy_t sva_pwr_policy:4;
	union sia_sva_pwron_enable_t sia_pwron_enable;
	union sia_sva_pwron_enable_t sva_pwron_enable;
	enum auto_enable_t auto_pwron_enable;
};

int prcmu_set_rc_a2p(enum romcode_write_t);
enum romcode_read_t prcmu_get_rc_p2a(void);
int prcmu_set_fifo_4500wu(enum intr_wakeup_t);
enum ap_pwrst_t prcmu_get_xp70_current_state(void);
int prcmu_set_ap_mode(enum ap_pwrst_trans_t);
int prcmu_set_ddr_pwrst(enum ddr_pwrst_t);
int prcmu_set_arm_opp(enum arm_opp_t);
int prcmu_get_arm_opp(void);
int prcmu_set_ape_opp(enum ape_opp_t);
int prcmu_get_ape_opp(void);
int prcmu_set_hwacc(enum hw_acc_dev, enum hw_accst_t);
/*mbox_2_arm_stat_t prcmu_get_m2a_status(void); */
/*mbox_to_arm_err_t prcmu_get_m2a_error(void); */
enum dvfs_stat_t prcmu_get_m2a_dvfs_status(void);
enum mbox_2_arm_hwacc_pwr_stat_t prcmu_get_m2a_hwacc_status(void);
int prcmu_set_irq_wakeup(uint32_t);
int prcmu_apply_ap_state_transition(enum ap_pwrst_trans_t transition,
				    enum ddr_pwrst_t ddr_state_req,
				    int _4500_fifo_wakeup);
int prcmu_configure_auto_pwr_mgt(
		struct sia_sva_auto_power_t,
		struct sia_sva_auto_power_t);

int prcmu_i2c_read(u8, u8);
int prcmu_i2c_write(u8, u8, u8);
int prcmu_i2c_get_status(void);
int prcmu_i2c_get_bank(void);
int prcmu_i2c_get_reg(void);
int prcmu_i2c_get_val(void);

int prcmu_ac_wake_req(void);
int prcmu_ac_sleep_req(void);
int prcmu_configure_wakeup_events(u32, u32, int);
int prcmu_get_wakeup_reason(u32 *, u8 *);
int prcmu_ack_wakeup_reason(void);
void prcmu_set_callback_cawakereq(void (*func)(u8));
void prcmu_set_callback_modem_reset_request(void (*func)(void));
void prcmu_system_reset(void);
int prcmu_is_ca_wake_req_pending(void);
int prcmu_read_ack_mb7(void);
int prcmu_enable_mcde(void);

#endif /* __MACH_PRCMU_FW_API_V1_H */
