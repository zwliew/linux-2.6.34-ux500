/*
 * Copyright (C) 2009 STMicroelectronics
 * Copyright (C) 2010 ST Ericsson.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * AB8500 peripheral regulators
 *
 * LDOs VAUX1/2
 *
 * FIXME : PRCMU I2C calls to be replaced by ab8500_* calls once integrated
 *	   with the ab8500 driver.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/consumer.h>

#include <mach/prcmu-fw-api.h>
#include <mach/ab8500.h>

#define AB8500_NUM_REGULATORS	(5)

#define AB8500_LDO_VAUX1        (1)
#define AB8500_LDO_VAUX2        (2)

static int ab8500_ldo_enable(struct regulator_dev *rdev)
{
	int regulator_id;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= AB8500_NUM_REGULATORS)
		return -EINVAL;

	/* FIXME : as reads from regulator bank are not supported yet
	 *	   enable both the VAUX1/2 by default. Add appropriate
	 *	   read calls when I2C_APE support is available
	 */
	if ((regulator_id == AB8500_LDO_VAUX1) ||
		(regulator_id == AB8500_LDO_VAUX2))
		prcmu_i2c_write(AB8500_REGU_CTRL2,
			AB8500_REGU_VAUX12_REGU_REG, 0x5);

	switch (regulator_id) {
	case AB8500_LDO_VAUX1:
		prcmu_i2c_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX1_SEL_REG, 0x0F);
		break;
	case AB8500_LDO_VAUX2:
		prcmu_i2c_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX2_SEL_REG, 0x0D);
		break;
	default:
		dev_dbg(rdev_get_dev(rdev), "unknown regulator id\n");
		return -EINVAL;
	}
	return 0;
}

static int ab8500_ldo_disable(struct regulator_dev *rdev)
{
	int regulator_id;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= AB8500_NUM_REGULATORS)
		return -EINVAL;

	switch (regulator_id) {
	case AB8500_LDO_VAUX1:
	case AB8500_LDO_VAUX2:
		prcmu_i2c_write(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX12_REGU_REG, 0x0);
	}
	return 0;
}

static int ab8500_ldo_is_enabled(struct regulator_dev *rdev)
{
	int regulator_id;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= AB8500_NUM_REGULATORS)
		return -EINVAL;

	/* FIXME : once the APE_I2C read is supported, add code */
	switch (regulator_id) {
	case AB8500_LDO_VAUX1:
	case AB8500_LDO_VAUX2:
	default:
		return 0;
	}
}

/* operations for LDOs (VAUX1/2) generalized */
static struct regulator_ops ab8500_ldo_ops = {
	.enable			= ab8500_ldo_enable,
	.disable		= ab8500_ldo_disable,
	.is_enabled		= ab8500_ldo_is_enabled,
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

MODULE_AUTHOR("STMicroelectronics/STEricsson");
MODULE_DESCRIPTION("AB8500 Voltage Framework");
MODULE_LICENSE("GPL");



