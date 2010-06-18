/*
 * Copyright (C) 2010 ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/amba/bus.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/io.h>

static struct nmk_gpio_platform_data u5500_gpio_data[] = {
	GPIO_DATA("GPIO-0-31", 0, 32),
	GPIO_DATA("GPIO-32-63", 32, 4), /* 36..63 not routed to pin */
	GPIO_DATA("GPIO-64-95", 64, 19), /* 83..95 not routed to pin */
	GPIO_DATA("GPIO-96-127", 96, 6), /* 102..127 not routed to pin */
	GPIO_DATA("GPIO-128-159", 128, 21), /* 149..159 not routed to pin */
	GPIO_DATA("GPIO-160-191", 160, 32),
	GPIO_DATA("GPIO-192-223", 192, 32),
	GPIO_DATA("GPIO-224-255", 224, 4), /* 228..255 not routed to pin */
};

static struct resource u5500_gpio_resources[] = {
	GPIO_RESOURCE(0),
	GPIO_RESOURCE(1),
	GPIO_RESOURCE(2),
	GPIO_RESOURCE(3),
	GPIO_RESOURCE(4),
	GPIO_RESOURCE(5),
	GPIO_RESOURCE(6),
	GPIO_RESOURCE(7),
};

struct platform_device u5500_gpio_devs[] = {
	GPIO_DEVICE(0),
	GPIO_DEVICE(1),
	GPIO_DEVICE(2),
	GPIO_DEVICE(3),
	GPIO_DEVICE(4),
	GPIO_DEVICE(5),
	GPIO_DEVICE(6),
	GPIO_DEVICE(7),
};

#define U5500_PWM_SIZE 0x20
static struct resource u5500_pwm0_resource[] = {
	{
		.name = "PWM_BASE",
		.start = U5500_PWM_BASE,
		.end = U5500_PWM_BASE + U5500_PWM_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct resource u5500_pwm1_resource[] = {
	{
		.name = "PWM_BASE",
		.start = U5500_PWM_BASE + U5500_PWM_SIZE,
		.end = U5500_PWM_BASE + U5500_PWM_SIZE * 2 - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct resource u5500_pwm2_resource[] = {
	{
		.name = "PWM_BASE",
		.start = U5500_PWM_BASE + U5500_PWM_SIZE * 2,
		.end = U5500_PWM_BASE + U5500_PWM_SIZE * 3 - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct resource u5500_pwm3_resource[] = {
	{
		.name = "PWM_BASE",
		.start = U5500_PWM_BASE + U5500_PWM_SIZE * 3,
		.end = U5500_PWM_BASE + U5500_PWM_SIZE * 4 - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device u5500_pwm0_device = {
	.id = 0,
	.name = "pwm",
	.resource = u5500_pwm0_resource,
	.num_resources = ARRAY_SIZE(u5500_pwm0_resource),
};

struct platform_device u5500_pwm1_device = {
	.id = 1,
	.name = "pwm",
	.resource = u5500_pwm1_resource,
	.num_resources = ARRAY_SIZE(u5500_pwm1_resource),
};

struct platform_device u5500_pwm2_device = {
	.id = 2,
	.name = "pwm",
	.resource = u5500_pwm2_resource,
	.num_resources = ARRAY_SIZE(u5500_pwm2_resource),
};

struct platform_device u5500_pwm3_device = {
	.id = 3,
	.name = "pwm",
	.resource = u5500_pwm3_resource,
	.num_resources = ARRAY_SIZE(u5500_pwm3_resource),
};
