/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * Author: Marcus Lorentzon <marcus.xm.lorentzon@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */
#include <linux/platform_device.h>
#include <mach/tc35892.h>
#include <mach/ab8500_denc.h>
#include <video/av8100.h>
#include <video/mcde_display.h>
#include <video/mcde_display-generic_dsi.h>
#include <video/mcde_display-av8100.h>
#include <video/mcde_display-ab8500.h>
#include <video/mcde_fb.h>
#include <video/mcde_dss.h>

#define DSI_UNIT_INTERVAL_0	0x9
#define DSI_UNIT_INTERVAL_1	0x9
#define DSI_UNIT_INTERVAL_2	0x6

static bool rotate_main = true;
static bool display_initialized_during_boot;

static int __init startup_graphics_setup(char *str)
{
	if (!str)
		return 1;

	switch (*str) {
	case '0':
		pr_info("No Startup graphics support\n");
		display_initialized_during_boot = false;
		break;
	case '1':
		pr_info("Startup graphics supported\n");
		display_initialized_during_boot = true;
		break;
	default:
		display_initialized_during_boot = false;
		break;
	};

	return 1;
}
__setup("startup_graphics=", startup_graphics_setup);

#ifdef CONFIG_DISPLAY_GENERIC_DSI_PRIMARY
static struct mcde_port port0 = {
	.type = MCDE_PORTTYPE_DSI,
	.mode = MCDE_PORTMODE_CMD,
	.pixel_format = MCDE_PORTPIXFMT_DSI_24BPP,
	.ifc = 1,
	.link = 0,
	.sync_src = MCDE_SYNCSRC_BTA,
	.update_auto_trig = false,
	.phy = {
		.dsi = {
			.virt_id = 0,
			.num_data_lanes = 2,
			.ui = DSI_UNIT_INTERVAL_0,
			.clk_cont = false,
		},
	},
};

struct mcde_display_generic_platform_data generic_display0_pdata = {
	.reset_gpio = EGPIO_PIN_15,
	.reset_delay = 1,
#ifdef CONFIG_REGULATOR
	.regulator_id = "v-display",
	.min_supply_voltage = 2500000, /* 2.5V */
	.max_supply_voltage = 2700000 /* 2.7V */
#endif
};

struct mcde_display_device generic_display0 = {
	.name = "mcde_disp_generic",
	.id = 0,
	.port = &port0,
	.chnl_id = MCDE_CHNL_A,
	.fifo = MCDE_FIFO_C0,
	.default_pixel_format = MCDE_OVLYPIXFMT_RGB565,
	.native_x_res = 864,
	.native_y_res = 480,
#ifdef CONFIG_DISPLAY_GENERIC_DSI_PRIMARY_VSYNC
	.synchronized_update = true,
#else
	.synchronized_update = false,
#endif
	/* TODO: Remove rotation buffers once ESRAM driver is completed */
	.rotbuf1 = U8500_ESRAM_BASE + 0x20000 * 4,
	.rotbuf2 = U8500_ESRAM_BASE + 0x20000 * 4 + 0x10000,
	.dev = {
		.platform_data = &generic_display0_pdata,
	},
};
#endif /* CONFIG_DISPLAY_GENERIC_DSI_PRIMARY */

#ifdef CONFIG_DISPLAY_GENERIC_DSI_SECONDARY
static struct mcde_port subdisplay_port = {
	.type = MCDE_PORTTYPE_DSI,
	.mode = MCDE_PORTMODE_CMD,
	.pixel_format = MCDE_PORTPIXFMT_DSI_24BPP,
	.ifc = 1,
	.link = 1,
	.sync_src = MCDE_SYNCSRC_BTA,
	.update_auto_trig = false,
	.phy = {
		.dsi = {
			.virt_id = 0,
			.num_data_lanes = 2,
			.ui = DSI_UNIT_INTERVAL_1,
			.clk_cont = false,
		},
	},

};

static struct mcde_display_generic_platform_data generic_subdisplay_pdata = {
	.reset_gpio = EGPIO_PIN_14,
	.reset_delay = 1,
#ifdef CONFIG_REGULATOR
	.regulator_id = "v-display",
	.min_supply_voltage = 2500000, /* 2.5V */
	.max_supply_voltage = 2700000 /* 2.7V */
#endif
};

static struct mcde_display_device generic_subdisplay = {
	.name = "mcde_disp_generic_subdisplay",
	.id = 1,
	.port = &subdisplay_port,
	.chnl_id = MCDE_CHNL_C1,
	.fifo = MCDE_FIFO_C1,
	.default_pixel_format = MCDE_OVLYPIXFMT_RGB565,
	.native_x_res = 864,
	.native_y_res = 480,
#ifdef CONFIG_DISPLAY_GENERIC_DSI_SECONDARY_VSYNC
	.synchronized_update = true,
#else
	.synchronized_update = false,
#endif
	.dev = {
		.platform_data = &generic_subdisplay_pdata,
	},
};
#endif /* CONFIG_DISPLAY_GENERIC_DSI_SECONDARY */


#ifdef CONFIG_DISPLAY_AB8500_TERTIARY
static struct mcde_port port_tvout1 = {
	.type = MCDE_PORTTYPE_DPI,
	.pixel_format = MCDE_PORTPIXFMT_DPI_24BPP,
	.ifc = 0,
	.link = 1,
	.sync_src = MCDE_SYNCSRC_OFF,
	.update_auto_trig = true,
	.phy = {
		.dpi = {
			.num_data_lanes = 4, /* DDR mode */
		},
	},
};

static struct ab8500_display_platform_data ab8500_display_pdata = {
	/* REVIEW use "#ifdef CONFIG_REGULATOR" as with other displays */
	/* TODO use this ID as soon as we switch to newer kernel with support or
	 * ab8500 TVout regulator
	.regulator_id = "v-tvout",
	 */
	/* REVIEW change name: use transform instead of convert? */
	.rgb_2_yCbCr_convert = {
		.matrix = {
			{0x42, 0x81, 0x19},
			{0xffda, 0xffb6, 0x70},
			{0x70, 0xffa2, 0xffee},
		},
		.offset = {0x80, 0x10, 0x80},
	}
};

static int ab8500_platform_enable(struct mcde_display_device *ddev)
{
	int res = 0;
	/* probe checks for pdata */
	struct ab8500_display_platform_data *pdata = ddev->dev.platform_data;

	dev_info(&ddev->dev, "%s\n", __func__);
	res = stm_gpio_altfuncenable(GPIO_ALT_LCD_PANELB);
	if (res != 0)
		goto alt_func_failed;

	if (pdata->regulator) {
		res = regulator_enable(pdata->regulator);
	if (res != 0)
		goto regu_failed;
	}

	return res;

regu_failed:
	(void) stm_gpio_altfuncdisable(GPIO_ALT_LCD_PANELB);
alt_func_failed:
	dev_warn(&ddev->dev, "Failure during %s\n", __func__);
	return res;
}

static int ab8500_platform_disable(struct mcde_display_device *ddev)
{
	int res;
	/* probe checks for pdata */
	struct ab8500_display_platform_data *pdata = ddev->dev.platform_data;

	dev_info(&ddev->dev, "%s\n", __func__);

	res = stm_gpio_altfuncdisable(GPIO_ALT_LCD_PANELB);
	if (res != 0)
		goto alt_func_failed;

	if (pdata->regulator) {
		res = regulator_disable(pdata->regulator);
	if (res != 0)
		goto regu_failed;
	}

	return res;
regu_failed:
	(void) stm_gpio_altfuncenable(GPIO_ALT_LCD_PANELB);
alt_func_failed:
	dev_warn(&ddev->dev, "Failure during %s\n", __func__);
	return res;

}

static struct mcde_display_device tvout_ab8500_display = {
	.name = "mcde_tv_ab8500",
	.id = 2,
	.port = &port_tvout1,
	.chnl_id = MCDE_CHNL_B,
	.fifo = MCDE_FIFO_B,
	.default_pixel_format = MCDE_OVLYPIXFMT_RGB565,
	.native_x_res = 720,
	.native_y_res = 576,
	.synchronized_update = true,
	.dev = {
		.platform_data = &ab8500_display_pdata,
	},

	/*
	* We might need to describe the std here:
	* - there are different PAL / NTSC formats (do they require MCDE
	*   settings?)
	*/
	.platform_enable = ab8500_platform_enable,
	.platform_disable = ab8500_platform_disable,
};

static struct ab8500_denc_platform_data ab8500_denc_pdata = {
	.ddr_enable = true,
	.ddr_little_endian = false,
};

static struct platform_device ab8500_denc = {
	.name = "ab8500_denc",
	.id = -1,
	.dev = {
		.platform_data = &ab8500_denc_pdata,
	},
};
#endif /* CONFIG_DISPLAY_AB8500_TERTIARY */

#ifdef CONFIG_DISPLAY_AV8100_TERTIARY
static struct mcde_port port2 = {
	.type = MCDE_PORTTYPE_DSI,
	.mode = MCDE_PORTMODE_CMD,
	.pixel_format = MCDE_PORTPIXFMT_DSI_24BPP,
	.ifc = 1,
	.link = 2,
#ifdef CONFIG_AV8100_HWTRIG_I2SDAT3
	.sync_src = MCDE_SYNCSRC_TE1,
#else
	.sync_src = MCDE_SYNCSRC_TE0,
#endif /* CONFIG_AV8100_HWTRIG_I2SDAT3 */
	.update_auto_trig = true,
	.phy = {
		.dsi = {
			.virt_id = 0,
			.num_data_lanes = 2,
			.ui = DSI_UNIT_INTERVAL_2,
			.clk_cont = false,
		},
	},
};

struct mcde_display_hdmi_platform_data av8100_hdmi_pdata = {
	.reset_gpio = 0,
	.reset_delay = 1,
	.regulator_id = NULL, /* TODO: "display_main" */
	.ddb_id = 1,
	.rgb_2_yCbCr_convert = {
		.matrix = {
			{0x42, 0x81, 0x19},
			{0xffda, 0xffb6, 0x70},
			{0x70, 0xffa2, 0xffee},
		},
		.offset = {0x80, 0x10, 0x80},
	}
};

static int av8100_platform_enable(struct mcde_display_device *dev)
{
	int ret = 0;
	struct mcde_display_hdmi_platform_data *pdata =
		dev->dev.platform_data;

	ret = stm_gpio_altfuncenable(GPIO_ALT_LCD_PANELA);
	if (ret != 0)
		goto alt_func_failed;

	if (pdata->reset_gpio)
		gpio_set_value(pdata->reset_gpio, pdata->reset_high);
	if (pdata->regulator)
		ret = regulator_enable(pdata->regulator);
alt_func_failed:
	return ret;
}

static int av8100_platform_disable(struct mcde_display_device *dev)
{
	int ret = 0;
	struct mcde_display_hdmi_platform_data *pdata =
		dev->dev.platform_data;

	ret = stm_gpio_altfuncdisable(GPIO_ALT_LCD_PANELA);
	if (ret != 0)
		goto alt_func_failed;
	if (pdata->reset_gpio)
		gpio_set_value(pdata->reset_gpio, !pdata->reset_high);
	if (pdata->regulator)
		ret = regulator_disable(pdata->regulator);
alt_func_failed:
	return ret;
}

static struct mcde_display_device av8100_hdmi = {
	.name = "av8100_hdmi",
	.id = 2,
	.port = &port2,
	.chnl_id = MCDE_CHNL_B,
	.fifo = MCDE_FIFO_B,
	.default_pixel_format = MCDE_OVLYPIXFMT_RGB565,
#ifdef CONFIG_AV8100_SDTV
	.native_x_res = 720,
	.native_y_res = 576,
#else
	.native_x_res = 1280,
	.native_y_res = 720,
#endif
	.synchronized_update = true,
	.dev = {
		.platform_data = &av8100_hdmi_pdata,
	},
	.platform_enable = av8100_platform_enable,
	.platform_disable = av8100_platform_disable,
};
#endif /* CONFIG_DISPLAY_AV8100_TERTIARY */

static struct fb_info *fbs[3] = { NULL, NULL, NULL };
static struct mcde_display_device *displays[3] = { NULL, NULL, NULL };

static int display_registered_callback(struct notifier_block *nb,
	unsigned long event, void *dev)
{
	struct mcde_display_device *ddev = dev;
	u16 width, height;
	bool rotate;

	if (event != MCDE_DSS_EVENT_DISPLAY_REGISTERED)
		return 0;

	if (ddev->id < 0 || ddev->id >= ARRAY_SIZE(fbs))
		return 0;

	mcde_dss_get_native_resolution(ddev, &width, &height);

	rotate = (ddev->id == 0 && rotate_main);
	if (rotate) {
		u16 tmp = height;
		height = width;
		width = tmp;
	}

	/* Create frame buffer */
	fbs[ddev->id] = mcde_fb_create(ddev,
		width, height,
		width, height * 2,
		ddev->default_pixel_format,
		rotate ? FB_ROTATE_CW : FB_ROTATE_UR);
	if (IS_ERR(fbs[ddev->id]))
		pr_warning("Failed to create fb for display %s\n", ddev->name);
	else
		pr_info("Framebuffer created (%s)\n", ddev->name);

	return 0;
}

static struct notifier_block display_nb = {
	.notifier_call = display_registered_callback,
};

static int framebuffer_registered_callback(struct notifier_block *nb,
	unsigned long event, void *data)
{
	int ret = 0;
	struct fb_event *event_data = data;
	struct fb_info *info;
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;

	if (event == FB_EVENT_FB_REGISTERED &&
		!display_initialized_during_boot) {
		if (event_data) {
			u8 *addr;
			info = event_data->info;
			if (!lock_fb_info(info))
				return -ENODEV;
			var = info->var;
			fix = info->fix;
			addr = ioremap(fix.smem_start,
					var.yres_virtual * fix.line_length);
			memset(addr, 0x00,
					var.yres_virtual * fix.line_length);
			var.yoffset = var.yoffset ? 0 : var.yres;
			if (info->fbops->fb_pan_display)
				ret = info->fbops->fb_pan_display(&var, info);
			unlock_fb_info(info);
		}
	}
	return ret;
}

static struct notifier_block framebuffer_nb = {
	.notifier_call = framebuffer_registered_callback,
};

int __init init_display_devices(void)
{
	int ret;

	ret = fb_register_client(&framebuffer_nb);
	if (ret)
		pr_warning("Failed to register framebuffer notifier\n");

	ret = mcde_dss_register_notifier(&display_nb);
	if (ret)
		pr_warning("Failed to register dss notifier\n");

#ifdef CONFIG_DISPLAY_GENERIC_DSI_PRIMARY
	if (display_initialized_during_boot)
		generic_display0.power_mode = MCDE_DISPLAY_PM_STANDBY;
	ret = mcde_display_device_register(&generic_display0);
	if (ret)
		pr_warning("Failed to register generic display device 0\n");
	displays[0] = &generic_display0;
#endif

#ifdef CONFIG_DISPLAY_GENERIC_DSI_SECONDARY
	ret = mcde_display_device_register(&generic_subdisplay);
	if (ret)
		pr_warning("Failed to register generic sub display device\n");
	displays[1] = &generic_subdisplay;
#endif

#ifdef CONFIG_DISPLAY_AV8100_TERTIARY
	ret = mcde_display_device_register(&av8100_hdmi);
	if (ret)
		pr_warning("Failed to register av8100_hdmi\n");
	displays[2] = &av8100_hdmi;
#endif
#ifdef CONFIG_DISPLAY_AB8500_TERTIARY
	ret = platform_device_register(&ab8500_denc);
	if (ret)
		pr_warning("Failed to register ab8500_denc device\n");
	else {
		ret = mcde_display_device_register(&tvout_ab8500_display);
		if (ret)
			pr_warning("Failed to register ab8500 tvout device\n");
		displays[2] = &tvout_ab8500_display;
	}
#endif

	return ret;
}

module_init(init_display_devices);
