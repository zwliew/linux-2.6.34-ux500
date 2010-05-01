/*
 * Copyright (c) 2009 ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef __MACH_PRCMU_FW_API_H
#define __MACH_PRCMU_FW_API_H

/*
 * typedef state_t - ON/OFF state definition
 *
 * OFF: State is ON
 * ON: State is OFF
 */
typedef enum {
	OFF = 0x00,
	ON = 0x01
} state_t;

/*
 * typedef clk_arm_t - ARM Cortex A9 clock schemes
 *
 * A9_OFF:
 * A9_BOOT:
 * A9_OPPT1:
 * A9_OPPT2:
 * A9_EXTCLK:
 */
typedef enum {
	A9_OFF,
	A9_BOOT,
	A9_OPPT1,
	A9_OPPT2,
	A9_EXTCLK
} clk_arm_t;

/*
 * typedef clk_gen_t - GEN#0/GEN#1 clock schemes
 *
 * GEN_OFF:
 * GEN_BOOT:
 * GEN_OPPT1:
 */
typedef enum {
	GEN_OFF,
	GEN_BOOT,
	GEN_OPPT1,
} clk_gen_t;

/* some information between arm and xp70 */

/**
 * typedef romcode_write_t - Romcode message written by A9 AND read by XP70
 *
 * RDY_2_DS: Value set when ApDeepSleep state can be executed by XP70
 * RDY_2_XP70_RST: Value set when 0x0F has been successfully polled by the
 *                 romcode. The xp70 will go into self-reset
 **/
typedef enum {
	RDY_2_DS = 0x09,
	RDY_2_XP70_RST = 0x10
} romcode_write_t;

/**
 * typedef romcode_read_t - Romcode message written by XP70 and read by A9
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
 **/
typedef enum {
	INIT = 0x00,
	FS_2_DS = 0x0A,
	END_DS = 0x0B,
	DS_TO_FS = 0x0C,
	END_FS = 0x0D,
	SWR = 0x0E,
	END_SWR = 0x0F
} romcode_read_t;

/**
 * typedef ap_pwrst_t - current power states defined in PRCMU firmware
 *
 * NO_PWRST: Current power state init
 * AP_BOOT: Current power state is apBoot
 * AP_EXECUTE: Current power state is apExecute
 * AP_DEEP_SLEEP: Current power state is apDeepSleep
 * AP_SLEEP: Current power state is apSleep
 * AP_IDLE: Current power state is apIdle
 * AP_RESET: Current power state is apReset
 **/
typedef enum {
	NO_PWRST = 0x00,
	AP_BOOT = 0x01,
	AP_EXECUTE = 0x02,
	AP_DEEP_SLEEP = 0x03,
	AP_SLEEP = 0x04,
	AP_IDLE = 0x05,
	AP_RESET = 0x06
} ap_pwrst_t;

/**
 * typedef ap_pwrst_trans_t - Transition states defined in PRCMU firmware
 *
 * NO_TRANSITION: No power state transition
 * APEXECUTE_TO_APSLEEP: Power state transition from ApExecute to ApSleep
 * APIDLE_TO_APSLEEP: Power state transition from ApIdle to ApSleep
 * APBOOT_TO_APEXECUTE: Power state transition from ApBoot to ApExecute
 * APEXECUTE_TO_APDEEPSLEEP: Power state transition from ApExecute to
 *                          ApDeepSleep
 * APEXECUTE_TO_APIDLE: Power state transition from ApExecute to ApIdle
 **/
typedef enum {
	NO_TRANSITION = 0x00,
	APEXECUTE_TO_APSLEEP = 0xFB,
	APIDLE_TO_APSLEEP = 0xFC,
	APBOOT_TO_APEXECUTE = 0xFD,
	APEXECUTE_TO_APDEEPSLEEP = 0xFE,
	APEXECUTE_TO_APIDLE = 0xFF
} ap_pwrst_trans_t;

/**
 * typedef ddr_pwrst_t - DDR power states definition
 *
 * DDR_PWR_STATE_UNCHANGED: SDRAM and DDR controller state is unchanged
 * TOBEDEFINED: to be defined
 **/
typedef enum {
	DDR_PWR_STATE_UNCHANGED = 0x00,
	TOBEDEFINED = 0x01
} ddr_pwrst_t;

/**
 * typedef arm_opp_t - ARM OPP states definition
 *
 * ARM_NO_CHANGE: The ARM operating point is unchanged
 * ARM_100_OPP: The new ARM operating point is arm100opp
 * ARM_50_OPP: The new ARM operating point is arm100opp
 * ARM_EXTCLK: The new ARM operating point is armExtClk
 **/
typedef enum {
	ARM_NO_CHANGE = 0x00,
	ARM_100_OPP = 0x01,
	ARM_50_OPP = 0x02,
	ARM_EXTCLK = 0x03
} arm_opp_t;

/**
 * typedef ape_opp_t - APE OPP states definition
 *
 * APE_NO_CHANGE: The APE operating point is unchanged
 * APE_100_OPP: The new APE operating point is ape100opp
 **/
typedef enum {
	APE_NO_CHANGE = 0x00,
	APE_100_OPP = 0x01
} ape_opp_t;

/**
 * typedef hw_accst_t - State definition for hardware accelerator
 *
 * HW_NO_CHANGE: The hardware accelerator state must remain unchanged
 * HW_OFF: The hardware accelerator must be switched off
 * HW_OFF_RAMRET: The hardware accelerator must be switched off with its
 *               internal RAM in retention
 * HW_ON: The hwa hadware accelerator hwa must be switched on
 **/
typedef enum {
	HW_NO_CHANGE = 0x00,
	HW_OFF = 0x01,
	HW_OFF_RAMRET = 0x02,
	HW_ON = 0x03
} hw_accst_t;

/**
 * typedef mbox_2_arm_stat_t - Status messages definition for mbox_arm
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
 **/
typedef enum {
	BOOT_TO_EXECUTEOK = 0xFF,
	DEEPSLEEPOK = 0xFE,
	SLEEPOK = 0xFD,
	IDLEOK = 0xFC,
	SOFTRESETOK = 0xFB,
	SOFTRESETGO = 0xFA,
	BOOT_TO_EXECUTE = 0xF9,
	EXECUTE_TO_DEEPSLEEP = 0xF8,
	DEEPSLEEP_TO_EXECUTE = 0xF7,
	DEEPSLEEP_TO_EXECUTEOK = 0xF6,
	EXECUTE_TO_SLEEP = 0xF5,
	SLEEP_TO_EXECUTE = 0xF4,
	SLEEP_TO_EXECUTEOK = 0xF3,
	EXECUTE_TO_IDLE = 0xF2,
	IDLE_TO_EXECUTE = 0xF1,
	IDLE_TO_EXECUTEOK = 0xF0,
	INIT_STATUS = 0x00
} mbox_2_arm_stat_t;

/**
 * typedef mbox_2_armdvfs_stat_t - DVFS status messages definition
 *
 * DVFS status messages definition for mbox_arm coming from XP70 to ARM
 * DVFS_GO: A state transition DVFS is on going
 * DVFS_ARM100OPPOK: The state transition DVFS has been completed for 100OPP
 * DVFS_ARM50OPPOK: The state transition DVFS has been completed for 50OPP
 * DVFS_ARMEXTCLKOK: The state transition DVFS has been completed for EXTCLK
 * DVFS_NOCHGTCLKOK: The state transition DVFS has been completed for
 *                   NOCHGCLK
 * DVFS_INITSTATUS: Value init
 **/
typedef enum {
	DVFS_GO = 0xFF,
	DVFS_ARM100OPPOK = 0xFE,
	DVFS_ARM50OPPOK = 0xFD,
	DVFS_ARMEXTCLKOK = 0xFC,
	DVFS_NOCHGTCLKOK = 0xFB,
	DVFS_INITSTATUS = 0x00
} mbox_2_armdvfs_stat_t;

/**
 * typedef mbox_2_arm_hwacc_pwr_stat_t - Hardware Accelarator status message
 *
 * HWACC_PWRST_GO: A state transition on hardware accelerator is on going
 * HWACC_PWRST_OK: The state transition on hardware accelerator has been
 *                 completed
 * HWACC_PWRSTATUS_INIT: Value init
 **/
typedef enum {
	HWACC_PWRST_GO = 0xFF,
	HWACC_PWRST_OK = 0xFE,
	HWACC_PWRSTATUS_INIT = 0x00
} mbox_2_arm_hwacc_pwr_stat_t;

/**
 * typedef sva_mmdsp_stat_t - SVA MMDSP status messages
 *
 * SVA_MMDSP_GO: SVAMMDSP interrupt has happened
 * SVA_MMDSP_INIT: Status init
 **/
typedef enum {
	SVA_MMDSP_GO = 0xFF,
	SVA_MMDSP_INIT = 0x00
} sva_mmdsp_stat_t;

/**
 * typedef sia_mmdsp_stat_t - SIA MMDSP status messages
 *
 * SIA_MMDSP_GO: SIAMMDSP interrupt has happened
 * SIA_MMDSP_INIT: Status init
 **/
typedef enum {
	SIA_MMDSP_GO = 0xFF,
	SIA_MMDSP_INIT = 0x00
} sia_mmdsp_stat_t;

/**
 * typedef intr_wakeup_t - Configure STW4500 FIFO interrupt as wake-up
 *
 * INTR_NOT_AS_WAKEUP: The 4500 fifo interrupt is not configured as a
 *                     wake-up event
 * INTR_AS_WAKEUP: The 4500 fifo interrupt is configured as a wake-up event
 **/
typedef enum {
	INTR_NOT_AS_WAKEUP = 0x0,
	INTR_AS_WAKEUP = 0x1
} intr_wakeup_t;

/**
 * typedef mbox_to_arm_err_t - Error messages definition
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
 **/
typedef enum {
	INIT_ERR = 0x00,
	PLLARMLOCKP_ERR = 0x01,
	PLLDDRLOCKP_ERR = 0x02,
	PLLSOC0LOCKP_ERR = 0x03,
	PLLSOC1LOCKP_ERR = 0x04,
	ARMWFI_ERR = 0x05,
	SYSCLKOK_ERR = 0x06,
	BOOT_ERR = 0x07,
	ROMCODESAVECONTEXT = 0x08,
	VARMHIGHSPEEDVALTO_ERR = 0x10,
	VARMHIGHSPEEDACCESS_ERR = 0x11,
	VARMLOWSPEEDVALTO_ERR = 0x12,
	VARMLOWSPEEDACCESS_ERR = 0x13,
	VARMRETENTIONVALTO_ERR = 0x14,
	VARMRETENTIONACCESS_ERR = 0x15,
	VAPEHIGHSPEEDVALTO_ERR = 0x16,
	VSAFEHPVALTO_ERR = 0x17,
	VMODSEL1VALTO_ERR = 0x18,
	VMODSEL2VALTO_ERR = 0x19,
	VARMOFFACCESS_ERR = 0x1A,
	VAPEOFFACCESS_ERR = 0x1B,
	VARMRETACCES_ERR = 0x1C,
	CURAPPWRSTISNOTBOOT = 0x20,
	CURAPPWRSTISNOTEXECUTE = 0x21,
	CURAPPWRSTISNOTSLEEPMODE = 0x22,
	CURAPPWRSTISNOTCORRECTDBG = 0x23,
	ARMREGU1VALTO_ERR = 0x24,
	ARMREGU2VALTO_ERR = 0x25,
	VAPEREGUVALTO_ERR = 0x26,
	VSMPS3REGUVALTO_ERR = 0x27,
	VMODREGUVALTO_ERR = 0x28
} mbox_to_arm_err_t;

typedef enum {
	SVAMMDSP = 0,
	SVAPIPE = 1,
	SIAMMDSP = 2,
	SIAPIPE = 3,
	SGA = 4,
	B2R2MCDE = 5,
	ESRAM1 = 6,
	ESRAM2 = 7,
	ESRAM3 = 8,
	ESRAM4 = 9
} hw_acc_t;

uint8_t prcmu_get_boot_status(void);
int32_t prcmu_set_rc_a2p(romcode_write_t);
romcode_read_t prcmu_get_rc_p2a(void);
int32_t prcmu_set_fifo_4500wu(intr_wakeup_t);
ap_pwrst_t prcmu_get_current_mode(void);
int32_t prcmu_set_ap_mode(ap_pwrst_trans_t);
int32_t prcmu_set_ddr_pwrst(ddr_pwrst_t);
int32_t prcmu_set_arm_opp(arm_opp_t);
int32_t prcmu_set_ape_opp(ape_opp_t);
int32_t prcmu_set_hwacc_st(hw_acc_t, hw_accst_t);
mbox_2_arm_stat_t prcmu_get_m2a_status(void);
mbox_to_arm_err_t prcmu_get_m2a_error(void);
mbox_2_armdvfs_stat_t prcmu_get_m2a_dvfs_status(void);
mbox_2_arm_hwacc_pwr_stat_t prcmu_get_m2a_hwacc_status(void);
int32_t prcmu_set_irq_wakeup(uint32_t);
int32_t prcmu_apply_ap_state_transition(ap_pwrst_trans_t transition,
				    ddr_pwrst_t ddr_state_req,
				    int _4500_fifo_wakeup);

#endif /* __MACH_PRCMU_FW_API_H */
