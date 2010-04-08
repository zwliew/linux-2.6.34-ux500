/*
 * Copyright (C) 2010 ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>

#include <asm/mach/map.h>

#include <mach/hardware.h>
#include <mach/devices.h>
#include <mach/setup.h>

static struct map_desc u8500_io_desc[] __initdata = {
	__IO_DEV_DESC(U8500_MSP0_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_MSP1_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_MSP2_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_GPIO0_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_GPIO1_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_GPIO2_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_GPIO3_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_PRCMU_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_PRCMU_TCDM_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_ASIC_ID_BASE, SZ_4K),
};

static struct map_desc u8500_ed_io_desc[] __initdata = {
	__IO_DEV_DESC(U8500_CLKRST7_BASE_ED, SZ_4K),
	__IO_DEV_DESC(U8500_MTU0_BASE_ED, SZ_4K),
	__IO_DEV_DESC(U8500_MTU1_BASE_ED, SZ_4K),
};

static struct platform_device *u8500_platform_devs[] __initdata = {
	&u8500_gpio_devs[0],
	&u8500_gpio_devs[1],
	&u8500_gpio_devs[2],
	&u8500_gpio_devs[3],
	&u8500_gpio_devs[4],
	&u8500_gpio_devs[5],
	&u8500_gpio_devs[6],
	&u8500_gpio_devs[7],
	&u8500_gpio_devs[8],
};

void __init u8500_map_io(void)
{
	ux500_map_io();

	iotable_init(u8500_io_desc, ARRAY_SIZE(u8500_io_desc));
	if (cpu_is_u8500ed())
		iotable_init(u8500_ed_io_desc, ARRAY_SIZE(u8500_ed_io_desc));
}

static void __init u8500_earlydrop_fixup(void)
{
	ux500_dma_device.resource[0].start = U8500_DMA_BASE_ED;
	ux500_dma_device.resource[0].end = U8500_DMA_BASE_ED + SZ_4K - 1;
	u8500_shrm_device.resource[1].start = IRQ_CA_WAKE_REQ_ED;
	u8500_shrm_device.resource[1].end = IRQ_CA_WAKE_REQ_ED;
	u8500_shrm_device.resource[2].start = IRQ_AC_READ_NOTIFICATION_0_ED;
	u8500_shrm_device.resource[2].end = IRQ_AC_READ_NOTIFICATION_0_ED;
	u8500_shrm_device.resource[3].start = IRQ_AC_READ_NOTIFICATION_1_ED;
	u8500_shrm_device.resource[3].end = IRQ_AC_READ_NOTIFICATION_1_ED;
	u8500_shrm_device.resource[4].start = IRQ_CA_MSG_PEND_NOTIFICATION_0_ED;
	u8500_shrm_device.resource[4].end = IRQ_CA_MSG_PEND_NOTIFICATION_0_ED;
	u8500_shrm_device.resource[5].start = IRQ_CA_MSG_PEND_NOTIFICATION_1_ED;
	u8500_shrm_device.resource[5].end = IRQ_CA_MSG_PEND_NOTIFICATION_1_ED;
}

bool cpu_is_u8500v11(void)
{
	void __iomem *asicid = __io_address(U8500_ASIC_ID_BASE) + 0xff4;
	return cpu_is_u8500() && readl(asicid) == 0x008500A1;
}

void __init u8500_init_devices(void)
{
	if (u8500_is_earlydrop())
		u8500_earlydrop_fixup();

	ux500_init_devices();

	platform_add_devices(u8500_platform_devs,
			     ARRAY_SIZE(u8500_platform_devs));
}
