/*
 * Copyright (C) STMicroelectronics 2009
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 * Author: Kumar Sanghvi <kumar.sanghvi@stericsson.com>
 *
 * PRCMU definitions for U8500 ED cut
 */
#ifndef __MACH_PRCMU_FW_DEFS_ED_H
#define __MACH_PRCMU_FW_DEFS_ED_H

/**
 * enum state_t - ON/OFF state definition
 *
 * OFF: State is ON
 * ON: State is OFF
 */
enum state_ed_t {
	OFF_ED = 0x00,
	ON_ED = 0x01
};

/**
 * enum clk_arm_t - ARM Cortex A9 clock schemes
 *
 * A9_OFF:
 * A9_BOOT:
 * A9_OPPT1:
 * A9_OPPT2:
 * A9_EXTCLK:
 */
enum clk_arm_ed_t {
	A9_OFF_ED,
	A9_BOOT_ED,
	A9_OPPT1_ED,
	A9_OPPT2_ED,
	A9_EXTCLK_ED
};

/**
 * enum clk_gen_t - GEN#0/GEN#1 clock schemes
 *
 * GEN_OFF:
 * GEN_BOOT:
 * GEN_OPPT1:
 */
enum clk_gen_ed_t {
	GEN_OFF_ED,
	GEN_BOOT_ED,
	GEN_OPPT1_ED,
};

/* some information between arm and xp70 */

/**
 * enum romcode_write_t - Romcode message written by A9 AND read by XP70
 *
 * RDY_2_DS: Value set when ApDeepSleep state can be executed by XP70
 * RDY_2_XP70_RST: Value set when 0x0F has been successfully polled by the
 *                 romcode. The xp70 will go into self-reset
 */
enum romcode_write_ed_t {
	RDY_2_DS_ED = 0x09,
	RDY_2_XP70_RST_ED = 0x10
};

/**
 * enum romcode_read_t - Romcode message written by XP70 and read by A9
 *
 * INIT: Init value when romcode field is not used
 * FS_2_DS: Value set when power state is going from ApExecute to
 *          ApDeepSleep
 * END_DS: Value set when ApDeepSleep power state is reached coming from
 *         ApExecute state
 * DS_TO_FS: Value set when power state is going from ApDeepSleep to
 *           ApExecute
 * END_FS: Value set when ApExecute power state is reached coming from
 *         ApDeepSleep state
 * SWR: Value set when power state is going to ApReset
 * END_SWR: Value set when the xp70 finished executing ApReset actions and
 *          waits for romcode acknowledgment to go to self-reset
 */
enum romcode_read_ed_t {
	INIT_ED = 0x00,
	FS_2_DS_ED = 0x0A,
	END_DS_ED = 0x0B,
	DS_TO_FS_ED = 0x0C,
	END_FS_ED = 0x0D,
	SWR_ED = 0x0E,
	END_SWR_ED = 0x0F
};

/**
 * enum ap_pwrst_t - current power states defined in PRCMU firmware
 *
 * NO_PWRST: Current power state init
 * AP_BOOT: Current power state is apBoot
 * AP_EXECUTE: Current power state is apExecute
 * AP_DEEP_SLEEP: Current power state is apDeepSleep
 * AP_SLEEP: Current power state is apSleep
 * AP_IDLE: Current power state is apIdle
 * AP_RESET: Current power state is apReset
 */
enum ap_pwrst_ed_t {
	NO_PWRST_ED = 0x00,
	AP_BOOT_ED = 0x01,
	AP_EXECUTE_ED = 0x02,
	AP_DEEP_SLEEP_ED = 0x03,
	AP_SLEEP_ED = 0x04,
	AP_IDLE_ED = 0x05,
	AP_RESET_ED = 0x06
};

/**
 * enum ap_pwrst_trans_t - Transition states defined in PRCMU firmware
 *
 * NO_TRANSITION: No power state transition
 * APEXECUTE_TO_APSLEEP: Power state transition from ApExecute to ApSleep
 * APIDLE_TO_APSLEEP: Power state transition from ApIdle to ApSleep
 * APBOOT_TO_APEXECUTE: Power state transition from ApBoot to ApExecute
 * APEXECUTE_TO_APDEEPSLEEP: Power state transition from ApExecute to
 *                          ApDeepSleep
 * APEXECUTE_TO_APIDLE: Power state transition from ApExecute to ApIdle
 */
enum ap_pwrst_trans_ed_t {
	NO_TRANSITION_ED = 0x00,
	APEXECUTE_TO_APSLEEP_ED = 0xFB,
	APIDLE_TO_APSLEEP_ED = 0xFC,
	APBOOT_TO_APEXECUTE_ED = 0xFD,
	APEXECUTE_TO_APDEEPSLEEP_ED = 0xFE,
	APEXECUTE_TO_APIDLE_ED = 0xFF
};

/**
 * enum ddr_pwrst_t - DDR power states definition
 *
 * DDR_PWR_STATE_UNCHANGED: SDRAM and DDR controller state is unchanged
 * TOBEDEFINED: to be defined
 */
enum ddr_pwrst_ed_t {
	DDR_PWR_STATE_UNCHANGED_ED = 0x00,
	TOBEDEFINED_ED = 0x01
};

/**
 * enum arm_opp_t - ARM OPP states definition
 *
 * ARM_NO_CHANGE: The ARM operating point is unchanged
 * ARM_100_OPP: The new ARM operating point is arm100opp
 * ARM_50_OPP: The new ARM operating point is arm100opp
 * ARM_EXTCLK: The new ARM operating point is armExtClk
 */
enum arm_opp_ed_t {
	ARM_NO_CHANGE_ED = 0x00,
	ARM_100_OPP_ED = 0x01,
	ARM_50_OPP_ED = 0x02,
	ARM_EXTCLK_ED = 0x03
};

/**
 * enum ape_opp_t - APE OPP states definition
 *
 * APE_NO_CHANGE: The APE operating point is unchanged
 * APE_100_OPP: The new APE operating point is ape100opp
 */
enum ape_opp_ed_t {
	APE_NO_CHANGE_ED = 0x00,
	APE_100_OPP_ED = 0x01
};

/**
 * enum hw_accst_t - State definition for hardware accelerator
 *
 * HW_NO_CHANGE: The hardware accelerator state must remain unchanged
 * HW_OFF: The hardware accelerator must be switched off
 * HW_OFF_RAMRET: The hardware accelerator must be switched off with its
 *               internal RAM in retention
 * HW_ON: The hwa hadware accelerator hwa must be switched on
 */
enum hw_accst_ed_t {
	HW_NO_CHANGE_ED = 0x00,
	HW_OFF_ED = 0x01,
	HW_OFF_RAMRET_ED = 0x02,
	HW_ON_ED = 0x03
};

/**
 * enum mbox_2_arm_stat_t - Status messages definition for mbox_arm
 *
 * Status messages definition for mbox_arm coming from XP70 to ARM
 *
 * BOOT_TO_EXECUTEOK: The apBoot to apExecute state transition has been
 *                    completed
 * DEEPSLEEPOK: The apExecute to apDeepSleep state transition has been
 *              completed
 * SLEEPOK: The apExecute to apSleep state transition has been completed
 * IDLEOK: The apExecute to apIdle state transition has been completed
 * SOFTRESETOK: The A9 watchdog/ SoftReset state has been completed
 * SOFTRESETGO : The A9 watchdog/SoftReset state is on going
 * BOOT_TO_EXECUTE: The apBoot to apExecute state transition is on going
 * EXECUTE_TO_DEEPSLEEP: The apExecute to apDeepSleep state transition is on
 *                       going
 * DEEPSLEEP_TO_EXECUTE: The apDeepSleep to apExecute state transition is on
 *                       going
 * DEEPSLEEP_TO_EXECUTEOK: The apDeepSleep to apExecute state transition has
 *                         been completed
 * EXECUTE_TO_SLEEP: The apExecute to apSleep state transition is on going
 * SLEEP_TO_EXECUTE: The apSleep to apExecute state transition is on going
 * SLEEP_TO_EXECUTEOK: The apSleep to apExecute state transition has been
 *                     completed
 * EXECUTE_TO_IDLE: The apExecute to apIdle state transition is on going
 * IDLE_TO_EXECUTE: The apIdle to apExecute state transition is on going
 * IDLE_TO_EXECUTEOK: The apIdle to apExecute state transition has been
 *                    completed
 * INIT_STATUS: Status init
 */
enum mbox_2_arm_stat_ed_t {
	BOOT_TO_EXECUTEOK_ED = 0xFF,
	DEEPSLEEPOK_ED = 0xFE,
	SLEEPOK_ED = 0xFD,
	IDLEOK_ED = 0xFC,
	SOFTRESETOK_ED = 0xFB,
	SOFTRESETGO_ED = 0xFA,
	BOOT_TO_EXECUTE_ED = 0xF9,
	EXECUTE_TO_DEEPSLEEP_ED = 0xF8,
	DEEPSLEEP_TO_EXECUTE_ED = 0xF7,
	DEEPSLEEP_TO_EXECUTEOK_ED = 0xF6,
	EXECUTE_TO_SLEEP_ED = 0xF5,
	SLEEP_TO_EXECUTE_ED = 0xF4,
	SLEEP_TO_EXECUTEOK_ED = 0xF3,
	EXECUTE_TO_IDLE_ED = 0xF2,
	IDLE_TO_EXECUTE_ED = 0xF1,
	IDLE_TO_EXECUTEOK_ED = 0xF0,
	INIT_STATUS_ED = 0x00
};

/**
 * enum mbox_2_armdvfs_stat_t - DVFS status messages definition
 *
 * DVFS status messages definition for mbox_arm coming from XP70 to ARM
 * DVFS_GO: A state transition DVFS is on going
 * DVFS_ARM100OPPOK: The state transition DVFS has been completed for 100OPP
 * DVFS_ARM50OPPOK: The state transition DVFS has been completed for 50OPP
 * DVFS_ARMEXTCLKOK: The state transition DVFS has been completed for EXTCLK
 * DVFS_NOCHGTCLKOK: The state transition DVFS has been completed for
 *                   NOCHGCLK
 * DVFS_INITSTATUS: Value init
 */
enum mbox_2_armdvfs_stat_ed_t {
	DVFS_GO_ED = 0xFF,
	DVFS_ARM100OPPOK_ED = 0xFE,
	DVFS_ARM50OPPOK_ED = 0xFD,
	DVFS_ARMEXTCLKOK_ED = 0xFC,
	DVFS_NOCHGTCLKOK_ED = 0xFB,
	DVFS_INITSTATUS_ED = 0x00
};

/**
 * enum mbox_2_arm_hwacc_pwr_stat_t - Hardware Accelarator status message
 *
 * HWACC_PWRST_GO: A state transition on hardware accelerator is on going
 * HWACC_PWRST_OK: The state transition on hardware accelerator has been
 *                 completed
 * HWACC_PWRSTATUS_INIT: Value init
 */
enum mbox_2_arm_hwacc_pwr_stat_ed_t {
	HWACC_PWRST_GO_ED = 0xFF,
	HWACC_PWRST_OK_ED = 0xFE,
	HWACC_PWRSTATUS_INIT_ED = 0x00
};

/**
 * enum sva_mmdsp_stat_t - SVA MMDSP status messages
 *
 * SVA_MMDSP_GO: SVAMMDSP interrupt has happened
 * SVA_MMDSP_INIT: Status init
 */
enum sva_mmdsp_stat_ed_t {
	SVA_MMDSP_GO_ED = 0xFF,
	SVA_MMDSP_INIT_ED = 0x00
};

/**
 * enum sia_mmdsp_stat_t - SIA MMDSP status messages
 *
 * SIA_MMDSP_GO: SIAMMDSP interrupt has happened
 * SIA_MMDSP_INIT: Status init
 */
enum sia_mmdsp_stat_ed_t {
	SIA_MMDSP_GO_ED = 0xFF,
	SIA_MMDSP_INIT_ED = 0x00
};

/**
 * enum intr_wakeup_t - Configure STW4500 FIFO interrupt as wake-up
 *
 * INTR_NOT_AS_WAKEUP: The 4500 fifo interrupt is not configured as a
 *                     wake-up event
 * INTR_AS_WAKEUP: The 4500 fifo interrupt is configured as a wake-up event
 */
enum intr_wakeup_ed_t {
	INTR_NOT_AS_WAKEUP_ED = 0x0,
	INTR_AS_WAKEUP_ED = 0x1
} intr_wakeup_ed_t;

/**
 * enum mbox_to_arm_err_t - Error messages definition
 *
 * Error messages definition for mbox_arm coming from XP70 to ARM
 *
 * INIT_ERR: Init value
 * PLLARMLOCKP_ERR: PLLARM has not been correctly locked in given time
 * PLLDDRLOCKP_ERR: PLLDDR has not been correctly locked in the given time
 * PLLSOC0LOCKP_ERR: PLLSOC0 has not been correctly locked in the given time
 * PLLSOC1LOCKP_ERR: PLLSOC1 has not been correctly locked in the given time
 * ARMWFI_ERR: The ARM WFI has not been correctly executed in the given time
 * SYSCLKOK_ERR: The SYSCLK is not available in the given time
 * BOOT_ERR: Romcode has not validated the XP70 self reset in the given time
 * ROMCODESAVECONTEXT: The Romcode didn.t correctly save it secure context
 * VARMHIGHSPEEDVALTO_ERR: The ARM high speed supply value transfered
 *          through I2C has not been correctly executed in the given time
 * VARMHIGHSPEEDACCESS_ERR: The command value of VarmHighSpeedVal transfered
 *             through I2C has not been correctly executed in the given time
 * VARMLOWSPEEDVALTO_ERR:The ARM low speed supply value transfered through
 *                     I2C has not been correctly executed in the given time
 * VARMLOWSPEEDACCESS_ERR: The command value of VarmLowSpeedVal transfered
 *             through I2C has not been correctly executed in the given time
 * VARMRETENTIONVALTO_ERR: The ARM retention supply value transfered through
 *                     I2C has not been correctly executed in the given time
 * VARMRETENTIONACCESS_ERR: The command value of VarmRetentionVal transfered
 *             through I2C has not been correctly executed in the given time
 * VAPEHIGHSPEEDVALTO_ERR: The APE highspeed supply value transfered through
 *                     I2C has not been correctly executed in the given time
 * VSAFEHPVALTO_ERR: The SAFE high power supply value transfered through I2C
 *                         has not been correctly executed in the given time
 * VMODSEL1VALTO_ERR: The MODEM sel1 supply value transfered through I2C has
 *                             not been correctly executed in the given time
 * VMODSEL2VALTO_ERR: The MODEM sel2 supply value transfered through I2C has
 *                             not been correctly executed in the given time
 * VARMOFFACCESS_ERR: The command value of Varm ON/OFF transfered through
 *                     I2C has not been correctly executed in the given time
 * VAPEOFFACCESS_ERR: The command value of Vape ON/OFF transfered through
 *                     I2C has not been correctly executed in the given time
 * VARMRETACCES_ERR: The command value of Varm retention ON/OFF transfered
 *             through I2C has not been correctly executed in the given time
 * CURAPPWRSTISNOTBOOT:Generated when Arm want to do power state transition
 *             ApBoot to ApExecute but the power current state is not Apboot
 * CURAPPWRSTISNOTEXECUTE: Generated when Arm want to do power state
 *              transition from ApExecute to others power state but the
 *              power current state is not ApExecute
 * CURAPPWRSTISNOTSLEEPMODE: Generated when wake up events are transmitted
 *             but the power current state is not ApDeepSleep/ApSleep/ApIdle
 * CURAPPWRSTISNOTCORRECTDBG:  Generated when wake up events are transmitted
 *              but the power current state is not correct
 * ARMREGU1VALTO_ERR:The ArmRegu1 value transferred through I2C has not
 *                    been correctly executed in the given time
 * ARMREGU2VALTO_ERR: The ArmRegu2 value transferred through I2C has not
 *                    been correctly executed in the given time
 * VAPEREGUVALTO_ERR: The VApeRegu value transfered through I2C has not
 *                    been correctly executed in the given time
 * VSMPS3REGUVALTO_ERR: The VSmps3Regu value transfered through I2C has not
 *                      been correctly executed in the given time
 * VMODREGUVALTO_ERR: The VModemRegu value transfered through I2C has not
 *                    been correctly executed in the given time
 */
enum mbox_to_arm_err_ed_t {
	INIT_ERR_ED = 0x00,
	PLLARMLOCKP_ERR_ED = 0x01,
	PLLDDRLOCKP_ERR_ED = 0x02,
	PLLSOC0LOCKP_ERR_ED = 0x03,
	PLLSOC1LOCKP_ERR_ED = 0x04,
	ARMWFI_ERR_ED = 0x05,
	SYSCLKOK_ERR_ED = 0x06,
	BOOT_ERR_ED = 0x07,
	ROMCODESAVECONTEXT_ED = 0x08,
	VARMHIGHSPEEDVALTO_ERR_ED = 0x10,
	VARMHIGHSPEEDACCESS_ERR_ED = 0x11,
	VARMLOWSPEEDVALTO_ERR_ED = 0x12,
	VARMLOWSPEEDACCESS_ERR_ED = 0x13,
	VARMRETENTIONVALTO_ERR_ED = 0x14,
	VARMRETENTIONACCESS_ERR_ED = 0x15,
	VAPEHIGHSPEEDVALTO_ERR_ED = 0x16,
	VSAFEHPVALTO_ERR_ED = 0x17,
	VMODSEL1VALTO_ERR_ED = 0x18,
	VMODSEL2VALTO_ERR_ED = 0x19,
	VARMOFFACCESS_ERR_ED = 0x1A,
	VAPEOFFACCESS_ERR_ED = 0x1B,
	VARMRETACCES_ERR_ED = 0x1C,
	CURAPPWRSTISNOTBOOT_ED = 0x20,
	CURAPPWRSTISNOTEXECUTE_ED = 0x21,
	CURAPPWRSTISNOTSLEEPMODE_ED = 0x22,
	CURAPPWRSTISNOTCORRECTDBG_ED = 0x23,
	ARMREGU1VALTO_ERR_ED = 0x24,
	ARMREGU2VALTO_ERR_ED = 0x25,
	VAPEREGUVALTO_ERR_ED = 0x26,
	VSMPS3REGUVALTO_ERR_ED = 0x27,
	VMODREGUVALTO_ERR_ED = 0x28
};

enum hw_acc_ed_t {
	SVAMMDSP_ED = 0,
	SVAPIPE_ED = 1,
	SIAMMDSP_ED = 2,
	SIAPIPE_ED = 3,
	SGA_ED = 4,
	B2R2MCDE_ED = 5,
	ESRAM1_ED = 6,
	ESRAM2_ED = 7,
	ESRAM3_ED = 8,
	ESRAM4_ED = 9
} hw_acc_ed_t;

#endif /* __MACH_PRCMU_FW_API_DEFS_ED_H */
