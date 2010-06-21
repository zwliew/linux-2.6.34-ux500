/*
 * Copyright (C) STMicroelectronics 2009
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 * Author: Sundar Iyer <sundar.iyer@stericsson.com>
 *
 * AB8500 peripheral regulators
 *
 * the AB8500 supports the following regulators,
 * LDOs - VAUDIO, VANAMIC2/2, VDIGMIC, VUSB, VINTCORE12, VTVOUT, VRTC,
 *	  VAUX1/2/3, VSIM/VRF1, VPLL, VANA, VRefDDR
 * Charge pumps - VCHPS, VBBN
 *
 * Based on the regulator to be accessed, the AB8500 client will use
 * either normal SPI/I2C or APE_I2C to access the regulator controls.
 *
 * FIXME : Regulator IDs constants are defined here until the ab8500 driver
 *	   is made into a mfd compliant code.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/consumer.h>

#include <mach/prcmu-fw-api.h>
#include <mach/ab8500.h>

#define AB8500_NUM_REGULATORS	(11)

#define AB8500_LDO_VAUX1        (1)
#define AB8500_LDO_VAUX2        (2)
#define AB8500_LDO_VTVOUT	(3)
#define AB8500_DCDC_VBUS	(4)
#define AB8500_LDO_VAUDIO	(5)
#define AB8500_LDO_VAMIC1	(6)
#define AB8500_LDO_VAMIC2	(7)
#define AB8500_LDO_VDMIC	(8)
#define AB8500_LDO_VINTCORE	(9)
#define AB8500_LDO_VAUX3	(10)

/* defines for regulator masks */
#define MASK_ENABLE		(0x1)
#define MASK_DISABLE		(0x0)

/* audio supplies */
#define MASK_LDO_VAUDIO		(0x2)
#define MASK_LDO_VDMIC 		(0x4)
#define MASK_LDO_VAMIC1 	(0x8)
#define MASK_LDO_VAMIC2		(0x10)

/* misc. supplies */
#define MASK_LDO_VTVOUT		(0x2)
#define MASK_LDO_VINTCORE	(0x4)

/* aux supplies */
#define MASK_LDO_VAUX1		(0x3)
#define MASK_LDO_VAUX2		(0xc)
#define MASK_LDO_VAUX3		(0x3)

#define MASK_LDO_VAUX1_SHIFT	(0x0)
#define MASK_LDO_VAUX2_SHIFT    (0x2)
#define MASK_LDO_VAUX3_SHIFT    (0x0)

/* regulator voltages */
#define VAUX1_VOLTAGE_2_5V	(0x8)
#define VAUX2_VOLTAGE_2_9V	(0xd)
#define VAUX3_VOLTAGE_2_9V	(0xd)


static int ab8500_ldo_enable(struct regulator_dev *rdev)
{
	int regulator_id, ret, val;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= AB8500_NUM_REGULATORS)
		return -EINVAL;

	if (!cpu_is_u8500v11()) {
		/* FIXME : as reads from regulator bank are not supported yet
		 *         enable both the VAUX1/2 by default. Add appropriate
		 *         read calls when I2C_APE support is available
		 */
		if ((regulator_id == AB8500_LDO_VAUX1) ||
				(regulator_id == AB8500_LDO_VAUX2))
			ab8500_write(AB8500_REGU_CTRL2,
					AB8500_REGU_VAUX12_REGU_REG, 0x5);
	}

	switch (regulator_id) {
	case AB8500_LDO_VAUX1:
		if (!cpu_is_u8500v11()) {
			ab8500_write(AB8500_REGU_CTRL2,
					AB8500_REGU_VAUX1_SEL_REG,
						VAUX1_VOLTAGE_2_5V);
			break;
		}
		val = ab8500_read(AB8500_REGU_CTRL2,
			AB8500_REGU_VAUX12_REGU_REG);
		val = val & ~MASK_LDO_VAUX1;
		val = val | (1 << MASK_LDO_VAUX1_SHIFT);
		ab8500_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX12_REGU_REG, val);
		break;
	case AB8500_LDO_VAUX2:
		if (!cpu_is_u8500v11()) {
			ab8500_write(AB8500_REGU_CTRL2,
					AB8500_REGU_VAUX2_SEL_REG,
					VAUX2_VOLTAGE_2_9V);
			break;
		}
		val = ab8500_read(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX12_REGU_REG);
		val = val & ~MASK_LDO_VAUX2;
		val = val | (1 << MASK_LDO_VAUX2_SHIFT);
		ab8500_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX12_REGU_REG, val);
		break;
	case AB8500_LDO_VAUX3:
		val = ab8500_read(AB8500_REGU_CTRL2,
				AB8500_REGU_VRF1VAUX3_REGU_REG);
		val = val & ~MASK_LDO_VAUX3;
		val = val | (1 << MASK_LDO_VAUX1_SHIFT);
		ab8500_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VRF1VAUX3_REGU_REG, val);
		/* delay for regulator stabilization */
		msleep(50);
		break;
	case AB8500_LDO_VTVOUT:
		val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_MISC1_REG,
				(val | MASK_LDO_VTVOUT));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot enable TVOUT LDO\n");
			return -EINVAL;
		}
		break;
	case AB8500_LDO_VAUDIO:
		val = ab8500_read(AB8500_REGU_CTRL1,
					AB8500_REGU_VAUDIO_SUPPLY_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG,
				(val | MASK_LDO_VAUDIO));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot enable TVOUT LDO\n");
			return -EINVAL;
		}
		break;
	case AB8500_LDO_VDMIC:
		val = ab8500_read(AB8500_REGU_CTRL1,
					AB8500_REGU_VAUDIO_SUPPLY_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG,
				(val | MASK_LDO_VDMIC));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot enable VDMIC LDO\n");
			return -EINVAL;
		}
		break;
	case AB8500_LDO_VAMIC1:
		val = ab8500_read(AB8500_REGU_CTRL1,
					AB8500_REGU_VAUDIO_SUPPLY_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG,
				(val | MASK_LDO_VAMIC1));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot enable VAMIC1 LDO\n");
			return -EINVAL;
		}
		break;
	case AB8500_LDO_VAMIC2:
		val = ab8500_read(AB8500_REGU_CTRL1,
					AB8500_REGU_VAUDIO_SUPPLY_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG,
				(val | MASK_LDO_VAMIC2));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot enable VAMIC2 LDO\n");
			return -EINVAL;
		}
		break;
	case AB8500_LDO_VINTCORE:
		val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_MISC1_REG,
				(val | MASK_LDO_VINTCORE));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot enable VINTCORE12 LDO\n");
			return -EINVAL;
		}
		break;
	default:
		dev_dbg(rdev_get_dev(rdev), "unknown regulator id\n");
		return -EINVAL;
	}
	return 0;
}

static int ab8500_ldo_disable(struct regulator_dev *rdev)
{
	int regulator_id, ret, val;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= AB8500_NUM_REGULATORS)
		return -EINVAL;

	if (!cpu_is_u8500v11()) {
		/* FIXME : as reads from regulator bank are not supported yet
		 *         disable both the VAUX1/2 by default. Add appropriate
		 *         read calls when I2C_APE support is available
		 */
		if ((regulator_id == AB8500_LDO_VAUX1) ||
				(regulator_id == AB8500_LDO_VAUX2)) {
			ab8500_write(AB8500_REGU_CTRL2,
					AB8500_REGU_VAUX12_REGU_REG,
						MASK_DISABLE);
			return 0;
		}
	}

	switch (regulator_id) {
	case AB8500_LDO_VAUX1:
		val = ab8500_read(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX12_REGU_REG);
		ab8500_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX12_REGU_REG,
					val & ~MASK_LDO_VAUX1);
		break;
	case AB8500_LDO_VAUX2:
		val = ab8500_read(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX12_REGU_REG);
		ab8500_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX12_REGU_REG,
					val & ~MASK_LDO_VAUX2);
		break;
	case AB8500_LDO_VAUX3:
		val = ab8500_read(AB8500_REGU_CTRL2,
				AB8500_REGU_VRF1VAUX3_REGU_REG);
		ab8500_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VRF1VAUX3_REGU_REG,
					val & ~MASK_LDO_VAUX3);
		/* delay for regulator stabilization */
		msleep(50);
		break;
	case AB8500_LDO_VTVOUT:
		val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_MISC1_REG,
					(val & ~MASK_LDO_VTVOUT));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot disable TVOUT LDO\n");
			return -EINVAL;
		}
		break;
	case AB8500_LDO_VINTCORE:
		/* FIXME : allow voltage control using set_voltage helpers */
		val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_MISC1_REG,
					(val & ~MASK_LDO_VINTCORE));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot disable VINTCORE12 LDO\n");
			return -EINVAL;
		}
		break;
	case AB8500_LDO_VAUDIO:
		val = ab8500_read(AB8500_REGU_CTRL1,
					AB8500_REGU_VAUDIO_SUPPLY_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG,
					(val & ~MASK_LDO_VAUDIO));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot disable VAUDIO LDO\n");
			return -EINVAL;
		}
		break;
	case AB8500_LDO_VDMIC:
		val = ab8500_read(AB8500_REGU_CTRL1,
					AB8500_REGU_VAUDIO_SUPPLY_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG,
					(val & ~MASK_LDO_VDMIC));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot disable VDMIC LDO\n");
			return -EINVAL;
		}
		break;
	case AB8500_LDO_VAMIC1:
		val = ab8500_read(AB8500_REGU_CTRL1,
					AB8500_REGU_VAUDIO_SUPPLY_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG,
					(val & ~MASK_LDO_VAMIC1));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot disble VAMIC1 LDO\n");
			return -EINVAL;
		}
		break;
	case AB8500_LDO_VAMIC2:
		val = ab8500_read(AB8500_REGU_CTRL1,
					AB8500_REGU_VAUDIO_SUPPLY_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG,
					(val & ~MASK_LDO_VAMIC2));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot disable VAMIC2 LDO\n");
			return -EINVAL;
		}
		break;
	default:
		dev_dbg(rdev_get_dev(rdev), "unknown regulator id\n");
		return -EINVAL;
	}
	return 0;
}

static int ab8500_ldo_is_enabled(struct regulator_dev *rdev)
{
	int regulator_id, val;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= AB8500_NUM_REGULATORS)
		return -EINVAL;

	switch (regulator_id) {
	case AB8500_LDO_VAUX1:
		val = ab8500_read(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX12_REGU_REG);
		if (val & MASK_LDO_VAUX1)
			return true;
		break;
	case AB8500_LDO_VAUX2:
		val = ab8500_read(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX12_REGU_REG);
		if (val & MASK_LDO_VAUX2)
			return true;
		break;
	case AB8500_LDO_VAUX3:
		val = ab8500_read(AB8500_REGU_CTRL2,
					AB8500_REGU_VRF1VAUX3_REGU_REG);
		if (val & MASK_LDO_VAUX3)
			return true;
		break;
	case AB8500_LDO_VTVOUT:
		val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
		if (val & MASK_LDO_VTVOUT)
			return true;
		break;
	case AB8500_LDO_VINTCORE:
		val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
		if (val & MASK_LDO_VINTCORE)
			return true;
		break;
	case AB8500_LDO_VAUDIO:
		val = ab8500_read(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG);
		if (val & MASK_LDO_VAUDIO)
			return true;
		break;
	case AB8500_LDO_VDMIC:
		val = ab8500_read(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG);
		if (val & MASK_LDO_VDMIC)
			return true;
		break;
	case AB8500_LDO_VAMIC1:
		val = ab8500_read(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG);
		if (val & MASK_LDO_VAMIC1)
			return true;
		break;
	case AB8500_LDO_VAMIC2:
		val = ab8500_read(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG);
		if (val & MASK_LDO_VAMIC2)
			return true;
		break;
	default:
		dev_dbg(rdev_get_dev(rdev), "unknown regulator id\n");
		return -EINVAL;
	}
	return 0;
}

/*
 * regulator layout
 * @voltage: supported voltage
 * @regval: register value to be written
 */
static struct regulator_voltage {
	int voltage;
	int regval;
};

/* voltage table for VAUXn regulators */
static struct regulator_voltage vauxn_table[] = {
	{ .voltage = 1100000, .regval  = 0x0, },
	{ .voltage = 1200000, .regval  = 0x1, },
	{ .voltage = 1300000, .regval  = 0x2, },
	{ .voltage = 1400000, .regval  = 0x3, },
	{ .voltage = 1500000, .regval  = 0x4, },
	{ .voltage = 1800000, .regval  = 0x5, },
	{ .voltage = 1850000, .regval  = 0x6, },
	{ .voltage = 1900000, .regval  = 0x7, },
	{ .voltage = 2500000, .regval  = 0x8, },
	{ .voltage = 2650000, .regval  = 0x9, },
	{ .voltage = 2700000, .regval  = 0xa, },
	{ .voltage = 2750000, .regval  = 0xb, },
	{ .voltage = 2800000, .regval  = 0xc, },
	{ .voltage = 2900000, .regval  = 0xd, },
	{ .voltage = 3000000, .regval  = 0xe, },
	{ .voltage = 3300000, .regval  = 0xf, },
};

/* voltage table for VINTCORE12 regulator */
static struct regulator_voltage vintcore_table[] = {
	{ .voltage = 1200000, .regval  = 0x0, },
	{ .voltage = 1225000, .regval  = 0x1, },
	{ .voltage = 1250000, .regval  = 0x2, },
	{ .voltage = 1275000, .regval  = 0x3, },
	{ .voltage = 1300000, .regval  = 0x4, },
	{ .voltage = 1325000, .regval  = 0x5, },
	{ .voltage = 1350000, .regval  = 0x6, },
};

/*
 * get the supported voltage from the associated regulator table
 * and return the corresponding register masks.
 * In case of the un-supported voltage, return error
 */
static int ab8500_get_best_voltage(struct regulator_voltage *voltage_table,
				int table_len, int min_uV, int max_uV)
{
	int i;
	int bestmatch = INT_MAX;

	/* check the supported voltage */
	for (i = 0; i < table_len; i++) {
		if ((voltage_table[i].voltage >= min_uV) &&
		    (voltage_table[i].voltage <= max_uV) &&
		    (voltage_table[i].voltage < bestmatch))
			return i;
	}

	return -EINVAL;
}

static int ab8500_ldo_set_voltage(struct regulator_dev *rdev,
					int min_uV, int max_uV)
{
	int regulator_id, val, val1, i;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= AB8500_NUM_REGULATORS)
		return -EINVAL;

	switch (regulator_id) {
	case AB8500_LDO_VAUX1:
		val = ab8500_get_best_voltage(vauxn_table,
				ARRAY_SIZE(vauxn_table), min_uV, max_uV);
		if (val < 0) {
			dev_dbg(rdev_get_dev(rdev), "%dvolts not supported\n",
					max_uV);
			return -EINVAL;
		}
		ab8500_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX1_SEL_REG,
				vauxn_table[val].regval);
		break;
	case AB8500_LDO_VAUX2:
		val = ab8500_get_best_voltage(vauxn_table,
				ARRAY_SIZE(vauxn_table), min_uV, max_uV);
		if (val < 0) {
			dev_dbg(rdev_get_dev(rdev), "%dvolts not supported\n",
					max_uV);
			return -EINVAL;
		}
		ab8500_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX2_SEL_REG,
				vauxn_table[val].regval);
		break;
	case AB8500_LDO_VAUX3:
		val = ab8500_get_best_voltage(vauxn_table,
				ARRAY_SIZE(vauxn_table), min_uV, max_uV);
		if (val < 0) {
			dev_dbg(rdev_get_dev(rdev), "%dvolts not supported\n",
					max_uV);
			return -EINVAL;
		}
		ab8500_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VRF1VAUX3_SEL_REG,
				vauxn_table[val].regval);
		break;
	case AB8500_LDO_VINTCORE:
		val = ab8500_get_best_voltage(vintcore_table,
				ARRAY_SIZE(vintcore_table), min_uV, max_uV);
		if (val < 0) {
			dev_dbg(rdev_get_dev(rdev), "%dvolts not supported\n",
					max_uV);
			return -EINVAL;
		}
		val1 = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
		val1 = val | (vintcore_table[val].regval << 0x3);
		ab8500_write(AB8500_REGU_CTRL2,
				AB8500_REGU_MISC1_REG, val1);
		break;
	default:
		dev_dbg(rdev_get_dev(rdev), "unknown regulator id\n");
		return -EINVAL;
	}

	return 0;
}

/* fixed typical voltages */
#define LDO_VTVOUT_VOLTAGE	(2000000) /* 2.0v typical */
#define LDO_VAUDIO_VOLTAGE	(2000000) /* 2.0v typical */
#define LDO_VDMIC_VOLTAGE	(1800000) /* 1.8V typical */
#define LDO_VAMIC1_VOLTAGE	(2050000) /* 2.05V typical */
#define LDO_VAMIC2_VOLTAGE	(2050000) /* 2.05V typical */

/* masks for reading the regulator voltages */
#define VAUXSEL_VOLTAGE_MASK	(0xf)
#define VINTCORE_VOLTAGE_MASK	(0x38)

static int ab8500_ldo_get_voltage(struct regulator_dev *rdev)
{
	int regulator_id, val;
	struct ab8500_regulator_info *info = rdev_get_drvdata(rdev);

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= AB8500_NUM_REGULATORS)
		return -EINVAL;

	/*
	 * for VAUXn/VINTCORE regulators, we read the configuration.
	 * for the rest, we return the typical voltage for the LDOs
	 */
	switch (regulator_id) {
	case AB8500_LDO_VAUX1:
		val = ab8500_read(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX1_SEL_REG);
		return vauxn_table[val & VAUXSEL_VOLTAGE_MASK].voltage;
		break;
	case AB8500_LDO_VAUX2:
		val = ab8500_read(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX2_SEL_REG);
		return vauxn_table[val & VAUXSEL_VOLTAGE_MASK].voltage;
		break;
	case AB8500_LDO_VAUX3:
		val = ab8500_read(AB8500_REGU_CTRL2,
				AB8500_REGU_VRF1VAUX3_SEL_REG);
		return vauxn_table[val & VAUXSEL_VOLTAGE_MASK].voltage;
		break;
	case AB8500_LDO_VTVOUT:
		return LDO_VTVOUT_VOLTAGE;
		break;
	case AB8500_LDO_VINTCORE:
		val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
		return vintcore_table[val & VINTCORE_VOLTAGE_MASK].voltage;
		break;
	case AB8500_LDO_VAUDIO:
		return LDO_VAUDIO_VOLTAGE;
		break;
	case AB8500_LDO_VDMIC:
		return LDO_VDMIC_VOLTAGE;
		break;
	case AB8500_LDO_VAMIC1:
		return LDO_VAMIC1_VOLTAGE;
		break;
	case AB8500_LDO_VAMIC2:
		return LDO_VAMIC2_VOLTAGE;
		break;
	default:
		dev_dbg(rdev_get_dev(rdev), "unknown regulator id\n");
		return -EINVAL;
	}

	return 0;
}

static struct regulator_ops ab8500_ldo_ops = {
	.enable			= ab8500_ldo_enable,
	.disable		= ab8500_ldo_disable,
	.is_enabled		= ab8500_ldo_is_enabled,
	.set_voltage		= ab8500_ldo_set_voltage,
	.get_voltage		= ab8500_ldo_get_voltage,
};

static int ab8500_dcdc_enable(struct regulator_dev *rdev)
{
	int regulator_id, ret;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= AB8500_NUM_REGULATORS)
		return -EINVAL;

	switch (regulator_id) {
	case AB8500_DCDC_VBUS:
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_VUSB_CTRL_REG, MASK_ENABLE);
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot enable VBUS\n");
			return -EINVAL;
		}
		break;
	default:
		dev_dbg(rdev_get_dev(rdev), "unknown regulator id\n");
		return -EINVAL;
	}
	return 0;
}

static int ab8500_dcdc_disable(struct regulator_dev *rdev)
{
	int regulator_id, ret;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= AB8500_NUM_REGULATORS)
		return -EINVAL;

	switch (regulator_id) {
	case AB8500_DCDC_VBUS:
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_VUSB_CTRL_REG, MASK_DISABLE);
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot disable VBUS\n");
			return -EINVAL;
		}
		break;
	default:
		dev_dbg(rdev_get_dev(rdev), "unknown regulator id\n");
		return -EINVAL;
	}
	return 0;
}

static int ab8500_dcdc_is_enabled(struct regulator_dev *rdev)
{
	int regulator_id, ret;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= AB8500_NUM_REGULATORS)
		return -EINVAL;

	switch (regulator_id) {
	case AB8500_DCDC_VBUS:
		ret = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_VUSB_CTRL_REG);
		if (ret & MASK_ENABLE)
			return true;
		else
			return false;
	default:
		dev_dbg(rdev_get_dev(rdev), "unknown regulator id\n");
		return -EINVAL;
	}
}

/* operations for DC-DC convertor supplies (VBUS, VSMPS1/2) */
static struct regulator_ops ab8500_dcdc_ops = {
	.enable		= ab8500_dcdc_enable,
	.disable	= ab8500_dcdc_disable,
	.is_enabled	= ab8500_dcdc_is_enabled,
};

static struct regulator_desc ab8500_desc[AB8500_NUM_REGULATORS] = {
	{
		.name   = "LDO-VAUX1",
		.id     = AB8500_LDO_VAUX1,
		.ops    = &ab8500_ldo_ops,
		.type   = REGULATOR_VOLTAGE,
		.owner  = THIS_MODULE,
	},
	{
		.name  	= "LDO-VAUX2",
		.id 	= AB8500_LDO_VAUX2,
		.ops   	= &ab8500_ldo_ops,
		.type  	= REGULATOR_VOLTAGE,
		.owner 	= THIS_MODULE,
	},
	{
		.name   = "LDO-VTVOUT",
		.id     = AB8500_LDO_VTVOUT,
		.ops    = &ab8500_ldo_ops,
		.type   = REGULATOR_VOLTAGE,
		.owner  = THIS_MODULE,
	},
	{
		.name   = "DCDC-VBUS",
		.id     = AB8500_DCDC_VBUS,
		.ops    = &ab8500_dcdc_ops,
		.type   = REGULATOR_VOLTAGE,
		.owner  = THIS_MODULE,
	},
	{
		.name   = "LDO-VAUDIO",
		.id     = AB8500_LDO_VAUDIO,
		.ops    = &ab8500_ldo_ops,
		.type   = REGULATOR_VOLTAGE,
		.owner  = THIS_MODULE,
	},
	{
		.name   = "LDO-VAMIC1",
		.id     = AB8500_LDO_VAMIC1,
		.ops    = &ab8500_ldo_ops,
		.type   = REGULATOR_VOLTAGE,
		.owner  = THIS_MODULE,
	},
	{
		.name   = "LDO-VAMIC2",
		.id     = AB8500_LDO_VAMIC2,
		.ops    = &ab8500_ldo_ops,
		.type   = REGULATOR_VOLTAGE,
		.owner  = THIS_MODULE,
	},
	{
		.name   = "LDO-VDMIC",
		.id     = AB8500_LDO_VDMIC,
		.ops    = &ab8500_ldo_ops,
		.type   = REGULATOR_VOLTAGE,
		.owner  = THIS_MODULE,
	},
	{
		.name   = "LDO-VINTCORE",
		.id     = AB8500_LDO_VINTCORE,
		.ops    = &ab8500_ldo_ops,
		.type   = REGULATOR_VOLTAGE,
		.owner  = THIS_MODULE,
	},
	{
		.name   = "LDO-VAUX3",
		.id     = AB8500_LDO_VAUX3,
		.ops    = &ab8500_ldo_ops,
		.type   = REGULATOR_VOLTAGE,
		.owner  = THIS_MODULE,
	},

};

static int __devinit ab8500_regulator_probe(struct platform_device *pdev)
{
	struct regulator_dev *rdev;
	struct regulator_init_data *init_data = pdev->dev.platform_data;

	rdev = regulator_register(&ab8500_desc[pdev->id], &pdev->dev, init_data, NULL);
	if (IS_ERR(rdev)) {
		dev_dbg(&pdev->dev, "couldn't register regulator\n");
		return PTR_ERR(rdev);
	}

	platform_set_drvdata(pdev, rdev);

	return 0;
}

static int __devexit ab8500_regulator_remove(struct platform_device *pdev)
{
	struct regulator_dev *reg_dev = platform_get_drvdata(pdev);

	regulator_unregister(reg_dev);

	return 0;
}

static struct platform_driver ab8500_regulator_driver = {
	.driver = {
		.name = "ab8500-regulator",
	},
	.probe	= ab8500_regulator_probe,
	.remove = __devexit_p(ab8500_regulator_remove),
};

static int __init ab8500_regulator_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&ab8500_regulator_driver);
	if (ret) {
		printk(KERN_INFO
		 "ab8500_regulator : platform_driver_register fails\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit ab8500_regulator_exit(void)
{
	platform_driver_unregister(&ab8500_regulator_driver);
}

/* replacing subsys_call as we want amba devices to be "powered" on */
arch_initcall(ab8500_regulator_init);
module_exit(ab8500_regulator_exit);

MODULE_AUTHOR("STMicroelectronics/ST-Ericsson");
MODULE_DESCRIPTION("AB8500 Voltage Framework");
MODULE_LICENSE("GPL");



