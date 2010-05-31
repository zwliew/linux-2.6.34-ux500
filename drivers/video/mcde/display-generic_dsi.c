/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * ST-Ericsson MCDE generic DCS display driver
 *
 * Author: Marcus Lorentzon <marcus.xm.lorentzon@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#include <video/mcde_display.h>
#include <video/mcde_display-generic_dsi.h>

static int generic_platform_enable(struct mcde_display_device *dev)
{
	int ret = 0;
	struct mcde_display_generic_platform_data *pdata =
		dev->dev.platform_data;

	dev_dbg(&dev->dev, "%s: Reset & power on generic display\n", __func__);

	if (pdata->regulator) {
		ret = regulator_enable(pdata->regulator);
		if (ret < 0) {
			dev_err(&dev->dev, "%s:Failed to enable regulator\n"
				, __func__);
			goto out;
		}
	}
	if (pdata->reset_gpio)
		gpio_set_value(pdata->reset_gpio, pdata->reset_high);
	mdelay(pdata->reset_delay);
	if (pdata->reset_gpio)
		gpio_set_value(pdata->reset_gpio, !pdata->reset_high);
out:
	return ret;
}

static int generic_platform_disable(struct mcde_display_device *dev)
{
	int ret = 0;
	struct mcde_display_generic_platform_data *pdata =
		dev->dev.platform_data;

	dev_dbg(&dev->dev, "%s:Reset & power off generic display\n", __func__);

	if (pdata->regulator) {
		ret = regulator_disable(pdata->regulator);
		if (ret < 0)
			dev_err(&dev->dev, "%s:Failed to disable regulator\n"
				, __func__);
	}
	return ret;
}

static int __devinit generic_probe(struct mcde_display_device *dev)
{
	int ret = 0;
	struct mcde_display_generic_platform_data *pdata =
		dev->dev.platform_data;

	if (pdata == NULL) {
		dev_err(&dev->dev, "%s:Platform data missing\n", __func__);
		ret = -EINVAL;
		goto no_pdata;
	}

	if (dev->port->type != MCDE_PORTTYPE_DSI) {
		dev_err(&dev->dev,
			"%s:Invalid port type %d\n",
			__func__, dev->port->type);
		ret = -EINVAL;
		goto invalid_port_type;
	}

	if (!dev->platform_enable && !dev->platform_disable) {
		pdata->generic_platform_enable = true;
		if (pdata->reset_gpio) {
			ret = gpio_request(pdata->reset_gpio, NULL);
			if (ret) {
				dev_warn(&dev->dev,
					"%s:Failed to request gpio %d\n",
					__func__, pdata->reset_gpio);
				goto gpio_request_failed;
			}
			gpio_direction_output(pdata->reset_gpio,
				!pdata->reset_high);
		}
		if (pdata->regulator_id) {
			pdata->regulator = regulator_get(NULL,
				pdata->regulator_id);
			if (IS_ERR(pdata->regulator)) {
				ret = PTR_ERR(pdata->regulator);
				dev_warn(&dev->dev,
					"%s:Failed to get regulator '%s'\n",
					__func__, pdata->regulator_id);
				pdata->regulator = NULL;
				goto regulator_get_failed;
			}
			regulator_set_voltage(pdata->regulator,
					pdata->min_supply_voltage,
					pdata->max_supply_voltage);
		}
		dev->platform_enable = generic_platform_enable;
		dev->platform_disable = generic_platform_disable;
	}

	if (dev->video_mode.xres == 0 || dev->video_mode.yres == 0) {
		dev->video_mode.xres = dev->native_x_res;
		dev->video_mode.yres = dev->native_y_res;
	}

	/* TODO: Remove when DSI send command uses interrupts */
	dev->prepare_for_update = NULL;
	dev->platform_enable = generic_platform_enable,
	dev->platform_disable = generic_platform_disable,

	dev_info(&dev->dev, "Generic display probed\n");

	goto out;

regulator_get_failed:
	if (pdata->generic_platform_enable && pdata->reset_gpio)
		gpio_free(pdata->reset_gpio);
gpio_request_failed:
invalid_port_type:
no_pdata:
out:
	return ret;
}

static int __devexit generic_remove(struct mcde_display_device *dev)
{
	struct mcde_display_generic_platform_data *pdata =
		dev->dev.platform_data;

	dev->set_power_mode(dev, MCDE_DISPLAY_PM_OFF);

	if (pdata->generic_platform_enable) {
		if (pdata->regulator)
			regulator_put(pdata->regulator);
		if (pdata->reset_gpio) {
			gpio_direction_input(pdata->reset_gpio);
			gpio_free(pdata->reset_gpio);
		}
	}

	return 0;
}

static int generic_resume(struct mcde_display_device *ddev)
{
	int ret;
	/* set_power_mode will handle call platform_enable */
	ret = ddev->set_power_mode(ddev, MCDE_DISPLAY_PM_STANDBY);
	if (ret < 0)
		dev_warn(&ddev->dev, "%s:Failed to resume display\n"
			, __func__);
	return ret;
}

static int generic_suspend(struct mcde_display_device *ddev, pm_message_t state)
{
	int ret;
	/* set_power_mode will handle call platform_disable */
	ret = ddev->set_power_mode(ddev, MCDE_DISPLAY_PM_OFF);
	if (ret < 0)
		dev_warn(&ddev->dev, "%s:Failed to suspend display\n"
			, __func__);
	return ret;
}

static struct mcde_display_driver generic_driver = {
	.probe	= generic_probe,
	.remove = generic_remove,
	.suspend = generic_suspend,
	.resume = generic_resume,
	.driver = {
		.name	= "mcde_disp_generic",
	},
};

/* Module init */
static int __init mcde_display_generic_init(void)
{
	pr_info("%s\n", __func__);

	return mcde_display_driver_register(&generic_driver);
}
module_init(mcde_display_generic_init);

static void __exit mcde_display_generic_exit(void)
{
	pr_info("%s\n", __func__);

	mcde_display_driver_unregister(&generic_driver);
}
module_exit(mcde_display_generic_exit);

MODULE_AUTHOR("Marcus Lorentzon <marcus.xm.lorentzon@stericsson.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ST-Ericsson MCDE generic DCS display driver");
