/*
 * Copyright (C) 2010 ST Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/io.h>

#include <mach/devices.h>
#include <mach/hardware.h>
#include <mach/mcde.h>
#include <mach/mcde_common.h>

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
/* Channel C0 */

#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT

static struct resource mcde2_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end   = U8500_MCDE_BASE + (U8500_MCDE_BASE_SIZE  - 1),
		.name  = "mcde_top",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end   = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name  = "mcde_extsrc",
		.flags = IORESOURCE_MEM,
	},
  [2] = {
		.start = U8500_MCDE_OVERLAY_BASE,
		.end   = U8500_MCDE_OVERLAY_BASE + (U8500_MCDE_OVERLAY_SIZE - 1),
		.name  = "mcde_overlay",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELC0_CONFIG_BASE,
    .end   = U8500_MCDE_CHANNELC0_CONFIG_BASE +	(U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name  = "mcde_chc0_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_BASE,
		.end   = U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_BASE +	(U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_SIZE - 1),
		.name  = "mcde_chc0c1_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end   = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_CHANNEL_SIZE - 1),
		.name  = "mcde_dsi_channel",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end   = U8500_DSI_LINK1_BASE + ((U8500_DSI_LINK_SIZE * U8500_DSI_LINK_COUNT) - 1),
		.name  = "dsi_link",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end   = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name  = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end   = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name  = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end   = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name  = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end   = IRQ_DISP,
		.name  = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
	[11] = {
		.start = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE +
			(U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_SIZE - 1),
		.name = "cha_specific",
		.flags = IORESOURCE_MEM,
	},
	[12] = {
		.start = U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_BASE +
			(U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_SIZE - 1),
		.name = "chb_specific",
		.flags = IORESOURCE_MEM,
	},
};

#else	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct resource mcde2_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end = U8500_MCDE_BASE + (U8500_MCDE_REGISTER_SIZE  - 1),
		.name = "mcde_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name = "mcde_extsrc_base",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVL_BASE,
		.end = U8500_MCDE_OVL_BASE + (U8500_MCDE_OVL_SIZE - 1),
		.name = "mcde_overlay_base",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELC0_CONFIG_BASE,
		.end = U8500_MCDE_CHANNELC0_CONFIG_BASE +
			(U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name = "chc0_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELC_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELC_SPECIFIC_REGISTER_BASE +
			(U8500_MCDE_CHANNELC_SPECIFIC_REGISTER_SIZE - 1),
		.name = "chb_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name = "mcde_dsi_channel_base",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end = U8500_DSI_LINK1_BASE +
			((U8500_DSI_LINK_SIZE*U8500_DSI_LINK_COUNT) - 1),
		.name = "dsi_link_base",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
	[11] = {
		.start = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE +
			(U8500_MCDE_CHANNEL_SPECIFIC_REGISTER_SIZE - 1),
		.name = "cha_specific",
		.flags = IORESOURCE_MEM,
	},
};

#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct mcde_channel_data mcde2_channel_data = {
	.channelid  = CHANNEL_C0,
	.nopan = 1,
	.nowrap = 1,
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
	.restype = CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_TYPE,
	.inbpp = CONFIG_FB_U8500_MCDE_CHANNELC0_INPUT_BPP,
	.outbpp = CONFIG_FB_U8500_MCDE_CHANNELC0_OUTPUT_BPP,
	.bpp16_type = CONFIG_FB_U8500_MCDE_CHANNELC0_INPUT_16BPP_TYPE,
	.bgrinput = CONFIG_FB_U8500_MCDE_CHANNELC0_INPUT_BGR,
#else
	.restype = "WVGA",
	.inbpp = 16,
	.outbpp = 0x2,
	.bpp16_type = 1,
	.bgrinput = 0x0,
#endif
};

struct platform_device u8500_mcde2_device = {
	.name = "U8500-MCDE",
	.id = 2,
	.dev = {
		.bus_id = "mcde_bus",
		.coherent_dma_mask = ~0,
		.platform_data = &mcde2_channel_data,
	},
	.num_resources = ARRAY_SIZE(mcde2_resources),
	.resource = mcde2_resources
};
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELC0 */

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1
/* Channel C1 */

#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT

static struct resource mcde3_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end   = U8500_MCDE_BASE + (U8500_MCDE_BASE_SIZE  - 1),
		.name  = "mcde_top",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end   = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name  = "mcde_extsrc",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVERLAY_BASE,
		.end   = U8500_MCDE_OVERLAY_BASE + (U8500_MCDE_OVERLAY_SIZE - 1),
		.name  = "mcde_overlay",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELC1_CONFIG_BASE,
    .end   = U8500_MCDE_CHANNELC1_CONFIG_BASE +	(U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name  = "mcde_chc1_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_BASE,
		.end   = U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_BASE + (U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_SIZE - 1),
		.name  = "mcde_chc0c1_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end   = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name  = "mcde_dsi_channel",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end   = U8500_DSI_LINK1_BASE +	((U8500_DSI_LINK_SIZE * U8500_DSI_LINK_COUNT) - 1),
		.name  = "dsi_link",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
};

#else	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct resource mcde3_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end = U8500_MCDE_BASE + (U8500_MCDE_REGISTER_SIZE  - 1),
		.name = "mcde_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name = "mcde_extsrc_base",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVL_BASE,
		.end = U8500_MCDE_OVL_BASE + (U8500_MCDE_OVL_SIZE - 1),
		.name = "mcde_overlay_base",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELC1_CONFIG_BASE,
		.end = U8500_MCDE_CHANNELC1_CONFIG_BASE +
			(U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name = "chc1_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELC_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELC_SPECIFIC_REGISTER_BASE +
			(U8500_MCDE_CHANNELC_SPECIFIC_REGISTER_SIZE - 1),
		.name = "chb_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name = "mcde_dsi_channel_base",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end = U8500_DSI_LINK1_BASE +
			((U8500_DSI_LINK_SIZE*U8500_DSI_LINK_COUNT) - 1),
		.name = "dsi_link_base",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
};

#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct mcde_channel_data mcde3_channel_data = {
	.channelid  = CHANNEL_C1,
	.nopan = 1,
	.nowrap = 1,
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1
	.restype = CONFIG_FB_U8500_MCDE_CHANNELC1_DISPLAY_TYPE,
	.inbpp = CONFIG_FB_U8500_MCDE_CHANNELC1_INPUT_BPP,
	.outbpp = CONFIG_FB_U8500_MCDE_CHANNELC1_OUTPUT_BPP,
	.bpp16_type = CONFIG_FB_U8500_MCDE_CHANNELC1_INPUT_16BPP_TYPE,
	.bgrinput = CONFIG_FB_U8500_MCDE_CHANNELC1_INPUT_BGR,
#else
	.restype = "WVGA",
	.inbpp = 16,
	.outbpp = 0x2,
	.bpp16_type = 1,
	.bgrinput = 0x0,
#endif
};

struct platform_device u8500_mcde3_device = {
	.name = "U8500-MCDE",
	.id = 3,
	.dev = {
		.bus_id = "mcde_bus",
		.coherent_dma_mask = ~0,
		.platform_data = &mcde3_channel_data,
	},

	.num_resources = ARRAY_SIZE(mcde3_resources),
	.resource = mcde3_resources
};
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELC1 */

#ifdef CONFIG_FB_U8500_MCDE_CHANNELB
/* Channel B */

#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT

static struct resource mcde1_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end   = U8500_MCDE_BASE + (U8500_MCDE_BASE_SIZE  - 1),
		.name  = "mcde_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end   = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name  = "mcde_extsrc",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVERLAY_BASE,
		.end   = U8500_MCDE_OVERLAY_BASE + (U8500_MCDE_OVERLAY_SIZE - 1),
		.name  = "mcde_overlay",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELB_CONFIG_BASE,
		.end   = U8500_MCDE_CHANNELB_CONFIG_BASE + (U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name  = "mcde_chb_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_BASE,
		.end   = U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_BASE + (U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_SIZE - 1),
		.name  = "mcde_chb_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end   = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name  = "mcde_dsi_channel",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end   = U8500_DSI_LINK1_BASE + ((U8500_DSI_LINK_SIZE*U8500_DSI_LINK_COUNT) - 1),
		.name  = "dsi_link",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end   = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name  = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
};

#else	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct resource mcde1_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end = U8500_MCDE_BASE + (U8500_MCDE_REGISTER_SIZE  - 1),
		.name = "mcde_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name = "mcde_extsrc_base",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVL_BASE,
		.end = U8500_MCDE_OVL_BASE + (U8500_MCDE_OVL_SIZE - 1),
		.name = "mcde_overlay_base",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELB_CONFIG_BASE,
		.end = U8500_MCDE_CHANNELB_CONFIG_BASE +
			(U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name = "chb_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_BASE +
			 (U8500_MCDE_CHANNEL_SPECIFIC_REGISTER_SIZE - 1),
		.name = "chb_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name = "mcde_dsi_channel_base",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end = U8500_DSI_LINK1_BASE +
			((U8500_DSI_LINK_SIZE*U8500_DSI_LINK_COUNT) - 1),
		.name = "dsi_link_base",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
};

#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct mcde_channel_data mcde1_channel_data = {
	.channelid  = CHANNEL_B,
	.nopan = 1,
	.nowrap = 1,
#ifdef CONFIG_FB_U8500_MCDE_CHANNELB
	.restype = CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_TYPE,
	.inbpp = CONFIG_FB_U8500_MCDE_CHANNELB_INPUT_BPP,
	.outbpp = CONFIG_FB_U8500_MCDE_CHANNELB_OUTPUT_BPP,
	.bpp16_type = CONFIG_FB_U8500_MCDE_CHANNELB_INPUT_16BPP_TYPE,
	.bgrinput = CONFIG_FB_U8500_MCDE_CHANNELB_INPUT_BGR,
#else
	.restype = "PAL",
	.inbpp = 16,
	.outbpp = 0x2,
	.bpp16_type = 1,
	.bgrinput = 0x0,
#endif
	.gpio_alt_func =  GPIO_ALT_LCD_PANELB
};

struct platform_device u8500_mcde1_device = {
	.name = "U8500-MCDE",
	.id = 1,
	.dev = {
		.bus_id = "mcde_bus",
		.coherent_dma_mask = ~0,
		.platform_data = &mcde1_channel_data,
	},
	.num_resources = ARRAY_SIZE(mcde1_resources),
	.resource = mcde1_resources
};
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELB */

#ifdef CONFIG_FB_U8500_MCDE_CHANNELA
/* Channel A */

#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT

static struct resource mcde0_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end   = U8500_MCDE_BASE + (U8500_MCDE_BASE_SIZE  - 1),
		.name  = "mcde_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end   = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name  = "mcde_extsrc",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVERLAY_BASE,
		.end   = U8500_MCDE_OVERLAY_BASE + (U8500_MCDE_OVERLAY_SIZE - 1),
		.name  = "mcde_overlay",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELA_CONFIG_BASE,
		.end   = U8500_MCDE_CHANNELA_CONFIG_BASE + (U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name  = "mcde_cha_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE,
		.end   = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE + (U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_SIZE - 1),
		.name  = "mcde_cha_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end   = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name  = "mcde_dsi_channel",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end   = U8500_DSI_LINK1_BASE +	((U8500_DSI_LINK_SIZE * U8500_DSI_LINK_COUNT) - 1),
		.name  = "dsi_link",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
};

#else	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct resource mcde0_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end = U8500_MCDE_BASE + (U8500_MCDE_REGISTER_SIZE  - 1),
		.name = "mcde_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name = "mcde_extsrc_base",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVL_BASE,
		.end = U8500_MCDE_OVL_BASE + (U8500_MCDE_OVL_SIZE - 1),
		.name = "mcde_overlay_base",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELA_CONFIG_BASE,
		.end = U8500_MCDE_CHANNELA_CONFIG_BASE +
			(U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name = "cha_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE +
			(U8500_MCDE_CHANNEL_SPECIFIC_REGISTER_SIZE - 1),
		.name = "cha_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name = "mcde_dsi_channel_base",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end = U8500_DSI_LINK1_BASE +
			((U8500_DSI_LINK_SIZE*U8500_DSI_LINK_COUNT) - 1),
		.name = "dsi_link_base",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
};

#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct mcde_channel_data mcde0_channel_data = {
	.channelid  = CHANNEL_A,
	.nopan = 1,
	.nowrap = 1,
	.restype = CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_TYPE,
	.inbpp = CONFIG_FB_U8500_MCDE_CHANNELA_INPUT_BPP,
	.outbpp = CONFIG_FB_U8500_MCDE_CHANNELA_OUTPUT_BPP,
	.bpp16_type = CONFIG_FB_U8500_MCDE_CHANNELA_INPUT_16BPP_TYPE,
	.bgrinput = CONFIG_FB_U8500_MCDE_CHANNELA_INPUT_BGR,
	.gpio_alt_func =  GPIO_ALT_LCD_PANELA
};

struct platform_device u8500_mcde0_device = {
	.name = "U8500-MCDE",
	.id = 0,
	.dev = {
		.bus_id = "mcde_bus",
		.coherent_dma_mask = ~0,
		.platform_data = &mcde0_channel_data,
	},
	.num_resources = ARRAY_SIZE(mcde0_resources),
	.resource = mcde0_resources
};
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELA */
