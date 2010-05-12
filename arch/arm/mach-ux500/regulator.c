/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 * Author: Sundar Iyer <sundar.iyer@stericsson.com>
 *
 * DB8500 specific regulators on AB8500
 *
 * Supplies on the AB8500 which are dedicated to AB8500 and AB8500 devices
 * are included here as machine specific -
 * VARM, VMODEM, VAPE, VMSPS3(VSAFE), VPLL, VANA, VBBN/VBBP
 *
 * following points must be noted :
 *  - VARM,VMODEM and VSAFE wont be accountable to the framework.
 *  - VAPE cannot be turned off as it is the interconnect supply.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/consumer.h>

#include <mach/ab8500.h>
#include <mach/prcmu-fw-api.h>

#define U8500_NUM_REGULATORS	(5)

#define DB8500_DCDC_VAPE        (0)
#define DB8500_LDO_VANA         (1)

#define REGULATOR_ENABLED       (1)
#define REGULATOR_DISABLED      (0)

uint32_t vape_mode = REGULATOR_MODE_NORMAL;

struct regulator_priv {
	unsigned int status;
	unsigned int operating_point;
	unsigned int opp_mode_dep_count;
};

static int dcdc_vape_get_voltage(struct regulator_dev *rdev)
{
	/* returning a dummy non-zero to support optimum_mode*
	 * helpers
	 */
	return 1;
}

static int dcdc_vape_set_mode(struct regulator_dev *rdev, unsigned int mode)
{
	int regulator_id;
	struct regulator_priv *priv = rdev_get_drvdata(rdev);

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= U8500_NUM_REGULATORS)
		return -EINVAL;

	dev_dbg(rdev_get_dev(rdev), "set_mode = %d\n", mode);

	/*
	 * we map here the various mode as
	 * REGULATOR_MODE_IDLE - AP 50OPP
	 * REGULATOR_MODE_NORMAL - AP 100OPP
	 *
	 */
	switch (mode) {
	case REGULATOR_MODE_IDLE:
		if (priv->operating_point == APE_50_OPP)
			return 0;
		/* request PRCMU for a transition to 50OPP */
		if (prcmu_set_ape_opp(APE_50_OPP)) {
			dev_dbg(rdev_get_dev(rdev),
					"APE 50OPP DVFS failed\n");
			return -EINVAL;
		}
		priv->operating_point = APE_50_OPP;
		break;
	case REGULATOR_MODE_NORMAL:
		if (priv->operating_point == APE_100_OPP)
			return 0;
		/* request PRCMU for a transition to 50OPP */
		if (prcmu_set_ape_opp(APE_100_OPP)) {
			dev_dbg(rdev_get_dev(rdev),
					"APE 100OPP DVFS failed\n");
			return -EINVAL;
		}
		priv->operating_point = APE_100_OPP;
		break;
	default:
		break;
	}

	return 0;
}

static int dcdc_vape_get_mode(struct regulator_dev *rdev)
{
	dev_dbg(rdev_get_dev(rdev),
			"get_mode returning %d\n", vape_mode);

	return vape_mode;

}

static unsigned int dcdc_vape_get_optimum_mode(struct regulator_dev *rdev,
		int input_uV, int output_uV, int load_uA)
{
	int regulator_id;
	struct regulator_priv *priv = rdev_get_drvdata(rdev);

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= U8500_NUM_REGULATORS)
		return -EINVAL;

	dev_dbg(rdev_get_dev(rdev), "get_optimum_mode: \
			input_uV=%d output_uV=%d load_uA=%d\n",
			input_uV, output_uV, load_uA);

	/* load_uA will be non-zero if any non-dvfs client is active */
	if (load_uA)
		priv->opp_mode_dep_count++;
	else
		priv->opp_mode_dep_count--;

	if (priv->opp_mode_dep_count) {
		return REGULATOR_MODE_NORMAL;
	} else {
		return REGULATOR_MODE_IDLE;
	}
}

static int dcdc_vape_enable(struct regulator_dev *rdev)
{
	int regulator_id;
	struct regulator_priv *priv = rdev_get_drvdata(rdev);

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= U8500_NUM_REGULATORS)
		return -EINVAL;

	priv->status = REGULATOR_ENABLED;

	return 0;
}

static int dcdc_vape_disable(struct regulator_dev *rdev)
{
	int regulator_id;
	struct regulator_priv *priv = rdev_get_drvdata(rdev);

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= U8500_NUM_REGULATORS)
		return -EINVAL;

	priv->status = REGULATOR_DISABLED;

	return 0;

}

static int dcdc_vape_is_enabled(struct regulator_dev *rdev)
{
	int regulator_id;
	struct regulator_priv *priv = rdev_get_drvdata(rdev);

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= U8500_NUM_REGULATORS)
		return -EINVAL;

	return priv->status;
}

static int ldo_vana_enable(struct regulator_dev *rdev)
{
	int regulator_id;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= U8500_NUM_REGULATORS)
		return -EINVAL;
	prcmu_i2c_write(AB8500_REGU_CTRL2,
			AB8500_REGU_VPLLVANA_REGU_REG, 0x05);
	return 0;
};

static int ldo_vana_disable(struct regulator_dev *rdev)
{
	int regulator_id;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= U8500_NUM_REGULATORS)
		return -EINVAL;

	return 0;
}

static struct regulator_ops dcdc_vape_ops = {
	.get_voltage            = dcdc_vape_get_voltage,
	.set_mode		= dcdc_vape_set_mode,
	.get_mode		= dcdc_vape_get_mode,
	.get_optimum_mode	= dcdc_vape_get_optimum_mode,
	.enable			= dcdc_vape_enable,
	.disable		= dcdc_vape_disable,
	.is_enabled		= dcdc_vape_is_enabled,
};

static struct regulator_ops ldo_vana_ops = {
	.enable                 = ldo_vana_enable,
	.disable                = ldo_vana_disable,
};

static struct regulator_desc db8500_desc[U8500_NUM_REGULATORS] = {
	{
		.name  	= "DCDC-VAPE",
		.id	= DB8500_DCDC_VAPE,
		.ops   	= &dcdc_vape_ops,
		.type  	= REGULATOR_VOLTAGE,
		.owner 	= THIS_MODULE,
	},
	{
		.name   = "LDO-VANA",
		.id     = DB8500_LDO_VANA,
		.ops    = &ldo_vana_ops,
		.type   = REGULATOR_VOLTAGE,
		.owner  = THIS_MODULE,
	},
};

static int __devinit db8500_regulator_probe(struct platform_device *pdev)
{
	struct regulator_dev *rdev;
	struct regulator_priv *priv;
	struct regulator_init_data *init_data = pdev->dev.platform_data;

	priv = kzalloc(sizeof(struct regulator_priv), GFP_KERNEL);
	if (!priv) {
		dev_dbg(&pdev->dev, "couldn't alloctae priv!!\n");
		return -EINVAL;
	}

	priv->status = 0;
	priv->operating_point = 0;
	priv->opp_mode_dep_count = 0;

	/* TODO : pass the regulator specific data to register */
	rdev = regulator_register(&db8500_desc[pdev->id], &pdev->dev,
							init_data, priv);
	if (IS_ERR(rdev)) {
		dev_dbg(&pdev->dev, "couldn't register regulator\n");
		return PTR_ERR(rdev);
	}

	platform_set_drvdata(pdev, rdev);

	return 0;
}

static int __devexit db8500_regulator_remove(struct platform_device *pdev)
{
	struct regulator_dev *reg_dev = platform_get_drvdata(pdev);

	regulator_unregister(reg_dev);

	return 0;
}

static struct platform_driver db8500_regulator_driver = {
	.driver = {
		.name = "db8500-regulator",
	},
	.probe	= db8500_regulator_probe,
	.remove = __devexit_p(db8500_regulator_remove),
};

static int __init db8500_regulator_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&db8500_regulator_driver);
	if (ret) {
		printk(KERN_INFO "db8500_regulator : platform_driver_register fails\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit db8500_regulator_exit(void)
{
	platform_driver_unregister(&db8500_regulator_driver);
}

/* replacing subsys_call as we want amba devices to be "powered" on */
arch_initcall(db8500_regulator_init);
module_exit(db8500_regulator_exit);

MODULE_AUTHOR("STMicroelectronics/ST-Ericsson");
MODULE_DESCRIPTION("DB8500 voltage framework");
MODULE_LICENSE("GPL");



