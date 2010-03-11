/*
 * Copyright (C) 2010 ST-Ericsson SA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/io.h>
#include <linux/clk.h>

#include <asm/hardware/cache-l2x0.h>
#include <asm/mach/map.h>

#include <mach/hardware.h>
#include <mach/devices.h>
#include <mach/setup.h>

#include "clock.h"

static struct map_desc ux500_io_desc[] __initdata = {
	__IO_DEV_DESC(U8500_RTC_BASE, SZ_4K),

	__IO_DEV_DESC(U8500_MSP0_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_MSP1_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_MSP2_BASE, SZ_4K),

	__IO_DEV_DESC(UX500_UART0_BASE, SZ_4K),
	__IO_DEV_DESC(UX500_UART1_BASE, SZ_4K),
	__IO_DEV_DESC(UX500_UART2_BASE, SZ_4K),

	__IO_DEV_DESC(UX500_GIC_CPU_BASE, SZ_4K),
	__IO_DEV_DESC(UX500_GIC_DIST_BASE, SZ_4K),
	__IO_DEV_DESC(UX500_L2CC_BASE, SZ_4K),

	__IO_DEV_DESC(UX500_CLKRST1_BASE, SZ_4K),
	__IO_DEV_DESC(UX500_CLKRST2_BASE, SZ_4K),
	__IO_DEV_DESC(UX500_CLKRST3_BASE, SZ_4K),
	__IO_DEV_DESC(UX500_CLKRST5_BASE, SZ_4K),
	__IO_DEV_DESC(UX500_CLKRST6_BASE, SZ_4K),

	__IO_DEV_DESC(UX500_MTU0_BASE, SZ_4K),
	__IO_DEV_DESC(UX500_MTU1_BASE, SZ_4K),

	__IO_DEV_DESC(UX500_BACKUPRAM0_BASE, SZ_8K),
};

static struct platform_device *ux500_platform_devs[] __initdata = {
	&ux500_dma_device,
};

static struct amba_device *ux500_amba_devs[] __initdata = {
	&ux500_rtc_device,
};

void __init ux500_map_io(void)
{
	iotable_init(ux500_io_desc, ARRAY_SIZE(ux500_io_desc));
}

void __init ux500_init_devices(void)
{
	platform_add_devices(ux500_platform_devs,
			     ARRAY_SIZE(ux500_platform_devs));
	amba_add_devices(ux500_amba_devs, ARRAY_SIZE(ux500_amba_devs));
}

void __init ux500_init_irq(void)
{
	gic_dist_init(0, (void __iomem *)IO_ADDRESS(UX500_GIC_DIST_BASE), 29);
	gic_cpu_init(0, (void __iomem *)IO_ADDRESS(UX500_GIC_CPU_BASE));

	/*
	 * Init clocks here so that they are available for system timer
	 * initialization.
	 */
	clk_init();
}

#ifdef CONFIG_CACHE_L2X0
static int __init ux500_l2x0_init(void)
{
	l2x0_init((void *)IO_ADDRESS(UX500_L2CC_BASE), 0x3e060000, 0x3e060000);
	return 0;
}
early_initcall(ux500_l2x0_init);
#endif
