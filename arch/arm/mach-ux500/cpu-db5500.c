/*
 * Copyright (C) 2010 ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/io.h>

#include <asm/mach/map.h>

#include <mach/hardware.h>
#include <mach/devices.h>
#include <mach/setup.h>

static struct map_desc u5500_io_desc[] __initdata = {
	__IO_DEV_DESC(U5500_GPIO0_BASE, SZ_4K),
	__IO_DEV_DESC(U5500_GPIO1_BASE, SZ_4K),
	__IO_DEV_DESC(U5500_GPIO2_BASE, SZ_4K),
	__IO_DEV_DESC(U5500_GPIO3_BASE, SZ_4K),
	__IO_DEV_DESC(U5500_GPIO4_BASE, SZ_4K),
};

static struct amba_device *u5500_amba_devs[] __initdata = {
	&u5500_gpio0_device,
	&u5500_gpio1_device,
	&u5500_gpio2_device,
	&u5500_gpio3_device,
	&u5500_gpio4_device,
};

void __init u5500_map_io(void)
{
	ux500_map_io();

	iotable_init(u5500_io_desc, ARRAY_SIZE(u5500_io_desc));
}

void __init u5500_init_devices(void)
{
	ux500_init_devices();

	amba_add_devices(u5500_amba_devs, ARRAY_SIZE(u5500_amba_devs));
}
