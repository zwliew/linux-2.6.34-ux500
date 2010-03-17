/*
 * ab8500_gpadc.c - AB8500 GPADC Driver
 *
 * Copyright (C) 2010 ST-Ericsson
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
#include <mach/ab8500.h>
#include <mach/ab8500_gpadc.h>

/* Interrupt */
#define GP_SW_ADC_CONV_END      39

static struct completion ab8500_gpadc_complete;
static DEFINE_MUTEX(ab8500_gpadc_lock);

EXPORT_SYMBOL(ab8500_gpadc_conversion);

int ab8500_gpadc_conversion(int input)
{
	int ret, val, low_data, high_data, data;

	mutex_lock(&ab8500_gpadc_lock);
	/* Enable VTVout LDO this is required for GPADC */
	val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
	ret = ab8500_write(AB8500_REGU_CTRL1,
			   AB8500_REGU_MISC1_REG, (val | 0x06));
	if (ret < 0)
		pr_debug("gpadc_conversion: enable VTVout LDO failed\n");

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
	wait_for_completion_timeout(&ab8500_gpadc_complete, HZ);

	/* Read the converted RAW data */
	low_data = ab8500_read(AB8500_GPADC, AB8500_GPADC_MANDATAL_REG);
	high_data = ab8500_read(AB8500_GPADC, AB8500_GPADC_MANDATAH_REG);

	data = (high_data << 8) | low_data;
	/* Disable GPADC */
	ret = ab8500_write(AB8500_GPADC, AB8500_GPADC_CTRL1_REG, 0x00);
	if (ret < 0)
		pr_debug("gpadc_conversion: disable gpadc failed\n");
	/* Disable VTVout LDO this is required for GPADC */
	val = ab8500_read(AB8500_REGU_CTRL1, AB8500_REGU_MISC1_REG);
	ret = ab8500_write(AB8500_REGU_CTRL1,
			   AB8500_REGU_MISC1_REG, (val & 0xF9));
	if (ret < 0)
		pr_debug("gpadc_conversion: disable VTVout LDO failed\n");
	mutex_unlock(&ab8500_gpadc_lock);
	return data;
}

/* set callback handlers  - completion of Sw ADC conversion */
static void ab8500_bm_gpswadcconvend_handler(void)
{
	complete(&ab8500_gpadc_complete);
}

static int __devinit ab8500_gpadc_probe(struct platform_device *pdev)
{
	int ret, val;

	/* Initialize completion used to notify completion of ceinversion */
	init_completion(&ab8500_gpadc_complete);

	/* Register call back handler  - SwAdcComplete */
	ret = ab8500_set_callback_handler(GP_SW_ADC_CONV_END,
					  ab8500_bm_gpswadcconvend_handler,
					  NULL);
	if (ret < 0) {
		pr_err("Failed to register call back handler\n");
		return ret;
	}
	/* Unmask the sw ADC completion interrupt */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK5_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK5_REG, (val & 0x7F));
	if (ret) {
		pr_debug("ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}
	return 0;
}

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
	return 0;
}

static struct platform_driver ab8500_gpadc_driver = {
	.probe = ab8500_gpadc_probe,
	.remove = __exit_p(ab8500_gpadc_remove),
	.driver = {
		   .name = "ab8500_gpadc",
		   },
};

static int __init ab8500_gpadc_init(void)
{
	return platform_driver_register(&ab8500_gpadc_driver);
}

static void __exit ab8500_gpadc_exit(void)
{
	platform_driver_unregister(&ab8500_gpadc_driver);
}

module_init(ab8500_gpadc_init);
module_exit(ab8500_gpadc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("STEricsson, Arun R Murthy");
MODULE_ALIAS("Platform:ab8500-gpadc");
MODULE_DESCRIPTION("AB8500 GPADC driver");
