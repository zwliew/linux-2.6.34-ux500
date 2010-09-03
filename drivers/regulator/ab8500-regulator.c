/*
 * Copyright (C) 2009 STMicroelectronics
 * Copyright (C) 2010 ST-Ericsson SA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
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
#include <linux/regulator/driver.h>
#include <linux/regulator/consumer.h>

#include <mach/prcmu-fw-api.h>
#include <mach/ab8500.h>

#define AB8500_NUM_REGULATORS	(10)

#define AB8500_LDO_VAUX1        (1)
#define AB8500_LDO_VAUX2        (2)
#define AB8500_LDO_VTVOUT	(3)
#define AB8500_DCDC_VBUS	(4)
#define AB8500_LDO_VAUDIO	(5)
#define AB8500_LDO_VAMIC1	(6)
#define AB8500_LDO_VAMIC2	(7)
#define AB8500_LDO_VDMIC	(8)
#define AB8500_LDO_VINTCORE	(9)

static int ab8500_ldo_enable(struct regulator_dev *rdev)
{
	int regulator_id, ret, val;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= AB8500_NUM_REGULATORS)
		return -EINVAL;

	/* FIXME : as reads from regulator bank are not supported yet
	 *	   enable both the VAUX1/2 by default. Add appropriate
	 *	   read calls when I2C_APE support is available
	 */
	if ((regulator_id == AB8500_LDO_VAUX1) ||
		(regulator_id == AB8500_LDO_VAUX2))
		ab8500_write(AB8500_REGU_CTRL2,
			AB8500_REGU_VAUX12_REGU_REG, 0x5);

	switch (regulator_id) {
	case AB8500_LDO_VAUX1:
		ab8500_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX1_SEL_REG, 0x0F);
		break;
	case AB8500_LDO_VAUX2:
		ab8500_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX2_SEL_REG, 0x0D);
		break;
	case AB8500_LDO_VTVOUT:
		val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_MISC1_REG, (val | 0x02));
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
				AB8500_REGU_VAUDIO_SUPPLY_REG, (val | 0x02));
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
				AB8500_REGU_VAUDIO_SUPPLY_REG, (val | 0x04));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot enable VAMIC1 LDO\n");
			return -EINVAL;
		}
		break;
	case AB8500_LDO_VAMIC1:
		val = ab8500_read(AB8500_REGU_CTRL1,
					AB8500_REGU_VAUDIO_SUPPLY_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG, (val | 0x08));
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
				AB8500_REGU_VAUDIO_SUPPLY_REG, (val | 0x10));
		if (ret < 0) {
			dev_dbg(rdev_get_dev(rdev),
					"cannot enable VAMIC2 LDO\n");
			return -EINVAL;
		}
		break;
	case AB8500_LDO_VINTCORE:
		val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_MISC1_REG, (val | 0x04));
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

	switch (regulator_id) {
	case AB8500_LDO_VAUX1:
	case AB8500_LDO_VAUX2:
		prcmu_i2c_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX12_REGU_REG, 0x0);
		break;
	case AB8500_LDO_VTVOUT:
		val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
		ret = ab8500_write(AB8500_REGU_CTRL1,
				AB8500_REGU_MISC1_REG, (val & ~0x02));
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
				AB8500_REGU_MISC1_REG, (val & ~0x04));
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
				AB8500_REGU_VAUDIO_SUPPLY_REG, (val & ~0x02));
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
				AB8500_REGU_VAUDIO_SUPPLY_REG, (val & ~0x04));
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
				AB8500_REGU_VAUDIO_SUPPLY_REG, (val & ~0x08));
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
				AB8500_REGU_VAUDIO_SUPPLY_REG, (val & ~0x10));
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

	/* FIXME : once the APE_I2C read is supported, add code for RegBank2 */
	switch (regulator_id) {
	case AB8500_LDO_VAUX1:
	case AB8500_LDO_VAUX2:
	case AB8500_LDO_VTVOUT:
		val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
		if (val & 0x02)
			return true;
		break;
	case AB8500_LDO_VINTCORE:
		val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
		if (val & 0x04)
			return true;
		break;
	case AB8500_LDO_VAUDIO:
		val = ab8500_read(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG);
		if (val & 0x02)
			return true;
		break;
	case AB8500_LDO_VDMIC:
		val = ab8500_read(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG);
		if (val & 0x04)
			return true;
		break;
	case AB8500_LDO_VAMIC1:
		val = ab8500_read(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG);
		if (val & 0x08)
			return true;
		break;
	case AB8500_LDO_VAMIC2:
		val = ab8500_read(AB8500_REGU_CTRL1,
				AB8500_REGU_VAUDIO_SUPPLY_REG);
		if (val & 0x10)
			return true;
		break;
	default:
		dev_dbg(rdev_get_dev(rdev), "unknown regulator id\n");
		return -EINVAL;
	}
	return 0;
}

static ab8500_ldo_set_voltage(struct regulator_dev *rdev,
					int min_uV, int max_uV)
{}

static ab8500_ldo_get_voltage(struct regulator_dev *rdev)
{}

/* operations for LDOs (VAUX1/2, TVOut) generalized */
static struct regulator_ops ab8500_ldo_ops = {
	.enable			= ab8500_ldo_enable,
	.disable		= ab8500_ldo_disable,
	.is_enabled		= ab8500_ldo_is_enabled,
	.set_voltage		= ab8500_ldo_set_voltage, /* TODO */
	.get_voltage		= ab8500_ldo_get_voltage, /* TODO */
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
				AB8500_REGU_VUSB_CTRL_REG, 0x1);
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
				AB8500_REGU_VUSB_CTRL_REG, 0x0);
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


/* operations for DC-DC convertor supplies (VBUS, VSMPS1/2) */
static struct regulator_ops ab8500_dcdc_ops = {
	.enable		= ab8500_dcdc_enable,
	.disable	= ab8500_dcdc_disable,
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
};

static int __devinit ab8500_regulator_probe(struct platform_device *pdev)
{
	struct regulator_dev *rdev;

	rdev = regulator_register(&ab8500_desc[pdev->id], &pdev->dev, NULL);
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



