/*
 * Copyright (C) 2007-2009 ST-Ericsson AB
 * License terms: GNU General Public License (GPL) version 2
 * ABX core access functions
 * Author: Linus Walleij <linus.walleij@stericsson.com>
 */

#include <linux/device.h>
#include <linux/regulator/machine.h>

#ifndef MFD_ABX_H
#define MFD_ABX_H

#define ABUNKNOWN	(0)
#define	AB3000		(1)
#define	AB3100		(2)
#define AB3550		(3)

/* AB3100, STR register flags */
#define AB3100_STR_ONSWA				(0x01)
#define AB3100_STR_ONSWB				(0x02)
#define AB3100_STR_ONSWC				(0x04)
#define AB3100_STR_DCIO					(0x08)
#define AB3100_STR_BOOT_MODE				(0x10)
#define AB3100_STR_SIM_OFF				(0x20)
#define AB3100_STR_BATT_REMOVAL				(0x40)
#define AB3100_STR_VBUS					(0x80)

/*
 * AB3100 contains 8 regulators, one external regulator controller
 * and a buck converter, further the LDO E and buck converter can
 * have separate settings if they are in sleep mode, this is
 * modeled as a separate regulator.
 */
#define ABX_NUM_REGULATORS (10)

/*
 * Number of banks in the chip. These are mapped to I2C clients.
 */
#define ABX_NUM_BANKS (2)

/*
 * Number of event registers.
 */
#if defined(CONFIG_AB3100_CORE)
	#define ABX_NUM_EVENT_REG (3)
#elif defined(CONFIG_MFD_AB3550_CORE)
	#define ABX_NUM_EVENT_REG (5)
#endif

struct abx;
struct abx_devinfo;

/**
 * struct abx_dev
 * @abx: the underlying core structure
 * @devinfo: information specific to the subdevice
 *
 * This structure is PRIVATE and devices using it should NOT
 * access ANY fields. It is used as a token for calling the
 * ABX functions.
 */
struct abx_dev {
	struct abx *abx;
	struct abx_devinfo *devinfo;
};

/**
 * struct ab3100_platform_data
 * Data supplied to initialize board connections to the AB3100
 * @reg_constraints: regulator constraints for target board
 *     the order of these constraints are: LDO A, C, D, E,
 *     F, G, H, K, EXT and BUCK.
 * @reg_initvals: initial values for the regulator registers
 *     plus two sleep settings for LDO E and the BUCK converter.
 *     exactly ABX_NUM_REGULATORS+2 values must be sent in.
 *     Order: LDO A, C, E, E sleep, F, G, H, K, EXT, BUCK,
 *     BUCK sleep, LDO D. (LDO D need to be initialized last.)
 * @external_voltage: voltage level of the external regulator.
 */
struct ab3100_platform_data {
	struct regulation_constraints reg_constraints[ABX_NUM_REGULATORS];
	u8 reg_initvals[ABX_NUM_REGULATORS+2];
	int external_voltage;
};

int abx_set_register_interruptible(struct abx_dev *abx_dev, u8 bank, u8 reg,
	u8 value);
int abx_get_register_interruptible(struct abx_dev *abx_dev, u8 bank, u8 reg,
	u8 *value);
int abx_get_register_page_interruptible(struct abx_dev *abx_dev, u8 bank,
	u8 first_reg, u8 *regvals, u8 numregs);
/**
 * This function modifies selected bits of a target register.
 *
 * @bitmask: Set bits to modify to 1
 * @bitvalues: The bits selected in bitmask will be written to the value
 *	contained in bitvalues
 */
int abx_mask_and_set_register_interruptible(struct abx_dev *abx_dev, u8 bank,
	 u8 reg, u8 bitmask, u8 bitvalues);
u8 abx_get_chip_type(struct abx_dev *abx_dev);
int abx_event_register(struct abx_dev *abx_dev, struct notifier_block *nb);
int abx_event_unregister(struct abx_dev *abx_dev, struct notifier_block *nb);
int abx_event_registers_startup_state_get(struct abx_dev *abx_dev, u8 *event);

#endif
