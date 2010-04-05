/*
 * ab8500_gpadc.c - AB8500 GPADC Driver
 *
 * Copyright (C) 2010 ST-Ericsson SA
 * Licensed under GPLv2.
 *
 * Author: Arun R Murthy <arun.murthy@stericsson.com>
 */

#define KMSG_COMPONENT "ab8500_gpadc"
#define pr_fmt(fmt) KMSG_COMPONENT ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/completion.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <mach/ab8500.h>
#include <mach/ab8500_gpadc.h>

/* Interrupt */
#define GP_SW_ADC_CONV_END      39

static struct ab8500_gpadc_device_info *di;

EXPORT_SYMBOL(ab8500_gpadc_conversion);

/**
 * ab8500_gpadc_conversion()
 * @input:	analog ip to be converted to digital data
 *
 * This function converts the selected analog i/p to digital
 * data. Thereafter calibration has to be made to obtain the
 * data in the required quantity measurement.
 *
 **/
int ab8500_gpadc_conversion(int input)
{
	int ret, val, low_data, high_data, data;

	if (!di)
		return -ENOMEM;

	mutex_lock(&di->ab8500_gpadc_lock);
	/* Enable VTVout LDO this is required for GPADC */
#if defined(CONFIG_REGULATOR)
	if (!regulator_is_enabled(di->regu))
		regulator_enable(di->regu);
#else
	val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
	ret = ab8500_write(AB8500_REGU_CTRL1,
			   AB8500_REGU_MISC1_REG, (val | 0x06));
	if (ret < 0)
		pr_debug("gpadc_conversion: enable VTVout LDO failed\n");
#endif
	/* Check if ADC is not busy, lock and proceed */
	do {
		val = ab8500_read(AB8500_GPADC, AB8500_GPADC_STAT_REG);
	} while (val);
	/* Enable GPADC */
	val = ab8500_read(AB8500_GPADC, AB8500_GPADC_CTRL1_REG);
	ret = ab8500_write(AB8500_GPADC, AB8500_GPADC_CTRL1_REG, (val | 0x01));
	if (ret < 0)
		pr_debug("gpadc_conversion: enable gpadc failed\n");
	/* Select the input source and set AVG samples to 16 */
	ret =
	    ab8500_write(AB8500_GPADC, AB8500_GPADC_CTRL2_REG, (input | 0x60));
	if (ret < 0)
		pr_debug("gpadc_conversion: set avg samples failed\n");
	/* Enable ADC, Buffering and select falling edge, start Conversion */
	val = ab8500_read(AB8500_GPADC, AB8500_GPADC_CTRL1_REG);
	ret = ab8500_write(AB8500_GPADC, AB8500_GPADC_CTRL1_REG, (val | 0x40));
	if (ret < 0)
		pr_debug("gpadc_conversion: select falling edge failed\n");
	val = ab8500_read(AB8500_GPADC, AB8500_GPADC_CTRL1_REG);
	ret = ab8500_write(AB8500_GPADC, AB8500_GPADC_CTRL1_REG, (val | 0x04));
	if (ret < 0)
		pr_debug("gpadc_conversion: start sw sonversion failed\n");
	/* wait for completion of conversion */
	wait_for_completion_timeout(&di->ab8500_gpadc_complete, HZ);

	/* Read the converted RAW data */
	low_data = ab8500_read(AB8500_GPADC, AB8500_GPADC_MANDATAL_REG);
	high_data = ab8500_read(AB8500_GPADC, AB8500_GPADC_MANDATAH_REG);

	data = (high_data << 8) | low_data;
	/* Disable GPADC */
	ret = ab8500_write(AB8500_GPADC, AB8500_GPADC_CTRL1_REG, 0x00);
	if (ret < 0)
		pr_debug("gpadc_conversion: disable gpadc failed\n");
	/* Disable VTVout LDO this is required for GPADC */
#if defined(CONFIG_REGULATOR)
	regulator_disable(di->regu);
#else
	val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
	ret = ab8500_write(AB8500_REGU_CTRL1,
			   AB8500_REGU_MISC1_REG, (val & 0xF9));
	if (ret < 0)
		pr_debug("gpadc_conversion: disable VTVout LDO failed\n");
#endif
	mutex_unlock(&di->ab8500_gpadc_lock);
	return data;
}

/**
 * ab8500_bm_gpswadcconvend_handler()
 *
 * This is the call back handler for end of conversion interrupt.
 * It notifies saying that the s/w conversion is completed.
 **/
static void ab8500_bm_gpswadcconvend_handler(void)
{
	complete(&di->ab8500_gpadc_complete);
}

/**
 * ab8500_gpadc_probe() - probe of ab8500 gpadc driver
 *
 * @pdev:	pointer to the platform_device structure
 *
 * This function is called after the driver is registered to the
 * platform device framework. It registers the callback handler
 * for gpadc interrupts and enables gpadc interrupt.
 *
 **/
static int __devinit ab8500_gpadc_probe(struct platform_device *pdev)
{
	int ret, val;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		pr_err("Error: No memory\n");
		return -ENOMEM;
	}

	/* Initialize mutex */
	mutex_init(&di->ab8500_gpadc_lock);

	/* Initialize completion used to notify completion of ceinversion */
	init_completion(&di->ab8500_gpadc_complete);

	/* Register call back handler  - SwAdcComplete */
	ret = ab8500_set_callback_handler(GP_SW_ADC_CONV_END,
					  ab8500_bm_gpswadcconvend_handler,
					  NULL);
	if (ret < 0) {
		pr_err("Failed to register call back handler\n");
		goto fail;
	}
	/* Unmask the sw ADC completion interrupt */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK5_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK5_REG, (val & 0x7F));
	if (ret) {
		pr_debug("ab8500_bm_hw_presence_en(): write failed\n");
		goto fail;
	}
#if defined(CONFIG_REGULATOR)
	di->regu = regulator_get(NULL, "ab8500-gpadc");
	if (IS_ERR(di->regu)) {
		ret = PTR_ERR(di->regu);
		pr_err("failed to get vtvout LDO\n");
		goto fail;
	}
#endif
	return 0;
fail:
	kfree(di);
	di = NULL;
	return ret;
}

/**
 * ab8500_gpadc_remove() - remove of ab8500 gpadc driver
 *
 * @pdev:	pointer to the platform_device structure
 *
 * This function is called when the driver gets registered to the
 * platform device framework. It unregisters the callback handler
 * for gpadc interrupts and disables gpadc interrupt.
 *
 **/
static int __devexit ab8500_gpadc_remove(struct platform_device *pdev)
{
	int ret, val;

	/* remove callback handlers  - completion of Sw ADC conversion */
	ret = ab8500_remove_callback_handler(GP_SW_ADC_CONV_END,
			ab8500_bm_gpswadcconvend_handler);
	if (ret < 0)
		pr_debug
		    ("failed to remove callback handler-completion of sw ADC conversion\n");
	/* mask Sw GPADC conversion interrupt */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK5_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK5_REG, (val | 0x80));
	if (ret) {
		pr_debug("ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}
#if defined(CONFIG_REGULATOR)
	regulator_put(di->regu);
#endif
	kfree(di);
	return 0;
}

/*
 * struct ab8500_gpadc_driver: AB8500 GPADC platform structure
 *
 * @probe:	The probe function to be called
 * @remove:	The remove fucntion to be called
 * @driver:	The driver data
 */
static struct platform_driver ab8500_gpadc_driver = {
	.probe = ab8500_gpadc_probe,
	.remove = __exit_p(ab8500_gpadc_remove),
	.driver = {
		   .name = "ab8500_gpadc",
		   },
};

/**
 * ab8500_gpadc_init() - register the device
 *
 * This fucntion registers the ab8500 gpadc driver as a platform device.
 **/
static int __init ab8500_gpadc_init(void)
{
	return platform_driver_register(&ab8500_gpadc_driver);
}

/**
 * ab8500_gpadc_exit() - unregister the device
 *
 * This fucntion unregisters the ab8500 gpadc driver as a platform device.
 **/
static void __exit ab8500_gpadc_exit(void)
{
	platform_driver_unregister(&ab8500_gpadc_driver);
}

module_init(ab8500_gpadc_init);
module_exit(ab8500_gpadc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ST-Ericsson, Arun R Murthy");
MODULE_ALIAS("Platform:ab8500-gpadc");
MODULE_DESCRIPTION("AB8500 GPADC driver");
