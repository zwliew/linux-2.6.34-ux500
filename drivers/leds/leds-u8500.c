/*
 * Copyright (C) 2009 ST-Ericsson SA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <mach/ab8500.h>

static void lcd_backlight_brightness_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	if (value == LED_OFF) {
		/* Putting of the backlight */
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG, 0x00);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL1_REG, 0xFF);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL2_REG, 0x03);
	} else if ((value > LED_OFF) && (value <= LED_HALF)) {
		/*Putting on the backlight at half brightness */
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG, 0x01);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL1_REG, 0xFF);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL2_REG, 0x01);
	} else if ((value > LED_HALF) && (value <= LED_FULL)) {
		/*Putting on the backlight in full brightness */
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG, 0x01);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL1_REG, 0xFF);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL2_REG, 0x03);
	}
}

enum led_brightness (lcd_backlight_brightness_get)(struct led_classdev *led_cdev)
{
	int value = 0;
	value = ab8500_read(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG);
	if (!(value & 0x01))
		return 0;

	value = ab8500_read(AB8500_MISC, AB8500_PWM_OUT_CTRL2_REG);
	if ((value & 0x03) == 0x03)
		return 255;
	else if ((value & 0x03) == 0x01)
		return 127;

	return 0;
}

/*
 * Initializing led_classdev structure
 */
static struct led_classdev lcd_backlight = {
	.name			= "lcd-backlight",
	.brightness_set		= lcd_backlight_brightness_set,
	.brightness_get		= lcd_backlight_brightness_get,
};

/*
 * u8500_leds_probe
 * Register led class device structure
 */
static int __init u8500_leds_probe(struct platform_device *pdev)
{
	int ret;
	ret = led_classdev_register(NULL, &lcd_backlight);

	if (ret < 0)
		return ret;
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG, 0x01);
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL1_REG, 0xFF);
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL2_REG, 0x03);
	return 0;
}

/*
 * u8500_leds_remove
 * Unregister led class device structure
 */
static int u8500_leds_remove(struct platform_device *pdev)
{
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG, 0x00);
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL1_REG, 0x00);
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL2_REG, 0x00);

	led_classdev_unregister(&lcd_backlight);
	return 0;
}

#ifdef CONFIG_PM
/*
 * u8500_leds_suspend - Turn down the backlight
 * This function will call the standard led class interface function
 */
static int u8500_leds_suspend(struct platform_device *dev, pm_message_t state)
{
	led_classdev_suspend(&lcd_backlight);
	return 0;
}

/*
 * u8500_leds_resume - Turn on the backlight to previous brightness
 * This function will call the standard led class interface function
 */
static int u8500_leds_resume(struct platform_device *dev)
{
	led_classdev_resume(&lcd_backlight);
	return 0;
}
#else
#define u8500_leds_suspend NULL
#define u8500_leds_resume NULL
#endif

static struct platform_driver u8500_leds_driver = {
	.driver		= {
		.name	= "u8500-leds",
		.owner	= THIS_MODULE,
	},
	.probe		= u8500_leds_probe,
	.remove		= u8500_leds_remove,
	.suspend  = u8500_leds_suspend,
	.resume  	= u8500_leds_resume,
};

/*
 * u8500_leds_init - Driver initialization
 */
static int __init u8500_leds_init(void)
{
	return platform_driver_register(&u8500_leds_driver);
}

/*
 * u8500_leds_exit - Driver exit by unregistering the driver
 */
static void __exit u8500_leds_exit(void)
{
	platform_driver_unregister(&u8500_leds_driver);
}

module_init(u8500_leds_init);
module_exit(u8500_leds_exit);

MODULE_AUTHOR("ST Ericsson");
MODULE_DESCRIPTION("LEDS class framework implementation for u8500 Platform (LCD backlight)");
MODULE_LICENSE("GPL");
