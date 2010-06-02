/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Dushyanth S R <dushyanth.sr@stericsson.com>
 * Modified by: Arun R Murthy <arun.murthy@stericsson.com>
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <mach/ab8500.h>

#define DISABLE_PWM1		0
#define DISABLE_PWM2		0
#define DISABLE_PWM3		0
#define ENABLE_PWM1		0x01
#define ENABLE_PWM2		0x02
#define PWM_DUTY_LOW_1024_1024	0xFF
#define PWM_DUTY_HI_1024_1024	0x03
#define PWM_DUTY_HI_512_1025	0x01
#define PWM_DUTY_LOW_1_1024	0x00
#define PWM_DUTY_HI_1_1024	0x00
#define PWM_FULL		255
#define PWM_HALF		127

static void lcd_backlight_brightness_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	int  pwm_val;
	int higher_val, lower_val;
#if defined(CONFIG_DISPLAY_GENERIC_DSI_SECONDARY)
	int val;
#endif

	switch (value) {
	case LED_OFF:
		/* Putting off the backlight */
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG,
				(DISABLE_PWM1 | DISABLE_PWM2 | DISABLE_PWM3));
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL1_REG,
						PWM_DUTY_LOW_1024_1024);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL2_REG,
						PWM_DUTY_HI_1024_1024);
		break;
	case LED_HALF:
		/* Putting on the backlight at half brightness */
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG,
						ENABLE_PWM1);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL1_REG,
						PWM_DUTY_LOW_1024_1024);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL2_REG,
						PWM_DUTY_HI_512_1025);
#if defined(CONFIG_DISPLAY_GENERIC_DSI_SECONDARY)
		val = ab8500_read(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG,
							(val | ENABLE_PWM2));
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL3_REG,
						PWM_DUTY_LOW_1024_1024);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL4_REG,
						PWM_DUTY_HI_512_1025);
#endif
		break;
	case LED_FULL:
		/* Putting on the backlight in full brightness */
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG,
						ENABLE_PWM1);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL1_REG,
						PWM_DUTY_LOW_1024_1024);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL2_REG,
						PWM_DUTY_HI_1024_1024);
#if defined(CONFIG_DISPLAY_GENERIC_DSI_SECONDARY)
		val = ab8500_read(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG,
							(val | ENABLE_PWM2));
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL3_REG,
						PWM_DUTY_LOW_1024_1024);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL4_REG,
						PWM_DUTY_HI_1024_1024);
#endif
		break;
	default:
		/*
		 * DutyPWMOut[9:0]: 0000000000 = 1/1024; 0000000001 = 2/1024;..
		 * 1111111110 = 1023/1024; 1111111111 = 1024/1024.
		 * Intensity that can be set through sysfs 0 - 255
		 * for 255 it corresponds to 1024/1024
		 * for value ?
		 * This can be calculated as (1024 * value) / 255
		 */
		pwm_val = ((1024 * value) / 255);
		/*
		 * get the first 8 bits that are be written to
		 * AB8500_PWM_OUT_CTRL1_REG[0:7]
		 */
		lower_val = pwm_val & 0x00FF;
		/*
		 * get bits [9:10] that are to be written to
		 * AB8500_PWM_OUT_CTRL2_REG[0:1]
		 */
		higher_val = ((pwm_val & 0x0300) >> 8);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG,
						ENABLE_PWM1);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL1_REG, lower_val);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL2_REG,
								higher_val);
#if defined(CONFIG_DISPLAY_GENERIC_DSI_SECONDARY)
		val = ab8500_read(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG,
							(val | ENABLE_PWM2));
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL3_REG, lower_val);
		ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL4_REG,
								higher_val);
#endif
		break;
	}
}

enum led_brightness (lcd_backlight_brightness_get)
				(struct led_classdev *led_cdev)
{
	int value = 0, high_val, low_val, pwm_val;

	value = ab8500_read(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG);
	if (!(value & ENABLE_PWM1))
		return 0;

	low_val = ab8500_read(AB8500_MISC, AB8500_PWM_OUT_CTRL1_REG);
	high_val = ab8500_read(AB8500_MISC, AB8500_PWM_OUT_CTRL2_REG);
	/*
	 * PWM value is of 10bit length bits, hence combining the
	 * lower AB8500_PWM_OUT_CTRL1_REG and higher
	 * AB8500_PWM_OUT_CTRL2_REG register.
	 */
	high_val = high_val << 8 | low_val;
	/*
	 * DutyPWMOut[9:0]: 0000000000 = 1/1024; 0000000001 = 2/1024;..
	 * 1111111110 = 1023/1024; 1111111111 = 1024/1024.
	 * Intensity that can be set through sysfs 0 - 255
	 * for 255 it corresponds to 1024/1024
	 * for ? it corresponds to high_val
	 * This can be calculated as (255 * higi_val) / 1024
	 */
	pwm_val = (255 * high_val) / 1024;

	return pwm_val;
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
 * ab8500_leds_probe
 * Register led class device structure
 */
static int __init ab8500_leds_probe(struct platform_device *pdev)
{
	int ret;
#if defined(CONFIG_DISPLAY_GENERIC_DSI_SECONDARY)
	int val;
#endif
	ret = led_classdev_register(NULL, &lcd_backlight);

	if (ret < 0)
		return ret;
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG,
						(ENABLE_PWM1 | ENABLE_PWM2));
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL1_REG,
					PWM_DUTY_LOW_1024_1024);
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL2_REG,
						PWM_DUTY_HI_1024_1024);
#if defined(CONFIG_DISPLAY_GENERIC_DSI_SECONDARY)
	val = ab8500_read(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG);
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG,
						(val | ENABLE_PWM2));
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL3_REG,
					PWM_DUTY_LOW_1024_1024);
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL4_REG,
						PWM_DUTY_HI_1024_1024);
#endif
	return 0;
}

/*
 * ab8500_leds_remove
 * Unregister led class device structure
 */
static int ab8500_leds_remove(struct platform_device *pdev)
{
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL7_REG,
				(DISABLE_PWM1 | DISABLE_PWM2 | DISABLE_PWM3));
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL1_REG,
					PWM_DUTY_LOW_1_1024);
	ab8500_write(AB8500_MISC, AB8500_PWM_OUT_CTRL2_REG,
					PWM_DUTY_HI_1_1024);

	led_classdev_unregister(&lcd_backlight);
	return 0;
}

#ifdef CONFIG_PM
/*
 * ab8500_leds_suspend - Turn down the backlight
 * This function will call the standard led class interface function
 */
static int ab8500_leds_suspend(struct platform_device *dev, pm_message_t state)
{
	led_classdev_suspend(&lcd_backlight);
	return 0;
}

/*
 * ab8500_leds_resume - Turn on the backlight to previous brightness
 * This function will call the standard led class interface function
 */
static int ab8500_leds_resume(struct platform_device *dev)
{
	led_classdev_resume(&lcd_backlight);
	return 0;
}
#else
#define ab8500_leds_suspend NULL
#define ab8500_leds_resume NULL
#endif

static struct platform_driver ab8500_leds_driver = {
	.driver		= {
		.name	= "ab8500-leds",
		.owner	= THIS_MODULE,
	},
	.probe		= ab8500_leds_probe,
	.remove		= ab8500_leds_remove,
	.suspend	= ab8500_leds_suspend,
	.resume		= ab8500_leds_resume,
};

/*
 * ab8500_leds_init - Driver initialization
 */
static int __init ab8500_leds_init(void)
{
	return platform_driver_register(&ab8500_leds_driver);
}

/*
 * ab8500_leds_exit - Driver exit by unregistering the driver
 */
static void __exit ab8500_leds_exit(void)
{
	platform_driver_unregister(&ab8500_leds_driver);
}

module_init(ab8500_leds_init);
module_exit(ab8500_leds_exit);

MODULE_AUTHOR("ST-Ericsson, Dushyanth S R");
MODULE_DESCRIPTION("LEDS class framework implementation for ab8500 Platform (LCD backlight)");
MODULE_LICENSE("GPL");
