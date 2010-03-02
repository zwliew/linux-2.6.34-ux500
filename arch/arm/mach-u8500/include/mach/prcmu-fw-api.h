/*
 * Copyright (c) 2009 ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef __MACH_PRCMU_FW_API_H
#define __MACH_PRCMU_FW_API_H

#include <linux/interrupt.h>
#include "prcmu-fw-defs_v1.h"


int prcmu_get_boot_status(void);
int prcmu_set_rc_a2p(romcode_write_t);
romcode_read_t prcmu_get_rc_p2a(void);
int prcmu_set_fifo_4500wu(intr_wakeup_t);
ap_pwrst_t prcmu_get_xp70_current_state(void);
int prcmu_set_ap_mode(ap_pwrst_trans_t);
int prcmu_set_ddr_pwrst(ddr_pwrst_t);
int prcmu_set_arm_opp(arm_opp_t);
int prcmu_get_arm_opp(void);
int prcmu_set_ape_opp(ape_opp_t);
int prcmu_get_ape_opp(void);
int prcmu_set_hwacc_st(hw_acc_t, hw_accst_t);
/*mbox_2_arm_stat_t prcmu_get_m2a_status(void); */
/*mbox_to_arm_err_t prcmu_get_m2a_error(void); */
dvfs_stat_t prcmu_get_m2a_dvfs_status(void);
mbox_2_arm_hwacc_pwr_stat_t prcmu_get_m2a_hwacc_status(void);
int prcmu_set_irq_wakeup(uint32_t);
int prcmu_apply_ap_state_transition(ap_pwrst_trans_t transition,
				    ddr_pwrst_t ddr_state_req,
				    int _4500_fifo_wakeup);

int prcmu_i2c_read(u8, u8);
int prcmu_i2c_write(u8, u8, u8);
int prcmu_i2c_get_status(void);
int prcmu_i2c_get_bank(void);
int prcmu_i2c_get_reg(void);
int prcmu_i2c_get_val(void);

int prcmu_arm_wakeup_modem(void);
int prcmu_arm_free_modem(void);
int prcmu_configure_wakeup_events(uint32_t, uint32_t);
int prcmu_get_wakeup_reason(uint32_t *, unsigned char *);
int prcmu_clear_wakeup_reason(void);
int prcmu_ack_wakeup_reason(void);
int prcmu_modem_wakup_ape(void);
int prcmu_modem_freed_ape(void);
int prcmu_read_ack_mb7(void);
irqreturn_t prcmu_ack_mbox_irq_handler(int, void *);

#endif /* __MACH_PRCMU_FW_API_V1_H */
