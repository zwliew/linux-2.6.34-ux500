/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * MOP500/HREF500 ed/v1 Display platform devices
 *
 * Author: Marcus Lorentzon <marcus.xm.lorentzon@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <video/mcde.h>

static struct resource mcde_resources[] = {
	[0] = {
		.name  = MCDE_IO_AREA,
		.start = U8500_MCDE_BASE,
		.end   = U8500_MCDE_BASE + 0x1000 - 1, /* TODO: Fix size */
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name  = MCDE_IO_AREA,
		.start = U8500_DSI_LINK1_BASE,
		.end   = U8500_DSI_LINK1_BASE + U8500_DSI_LINK_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.name  = MCDE_IO_AREA,
		.start = U8500_DSI_LINK2_BASE,
		.end   = U8500_DSI_LINK2_BASE + U8500_DSI_LINK_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.name  = MCDE_IO_AREA,
		.start = U8500_DSI_LINK3_BASE,
		.end   = U8500_DSI_LINK3_BASE + U8500_DSI_LINK_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.name  = MCDE_IRQ,
		.start = IRQ_DISP,
		.end   = IRQ_DISP,
		.flags = IORESOURCE_IRQ,
	},
};

static void dev_release_noop(struct device *dev)
{
	/* Do nothing */
}

static struct mcde_platform_data mcde_pdata = {
	.num_dsilinks = 3,
	/* YCbCr to AB8500 on D[8:15]: connect LSB Ch B */
	.outmux = { 0, 3, 0, 0, 0 },
	.syncmux = 0x01,
	.regulator_id = "v-ana",
	.clock_dsi_id = "hdmi",
	.clock_dsi_lp_id = "tv",
	.clock_mcde_id = "mcde",
};

struct platform_device ux500_mcde_device = {
	.name = "mcde",
	.id = -1,
	.dev = {
		.release = dev_release_noop,
		.platform_data = &mcde_pdata,
	},
	.num_resources = ARRAY_SIZE(mcde_resources),
	.resource = mcde_resources,
};




