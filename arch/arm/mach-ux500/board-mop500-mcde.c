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

#define DSI_UNIT_INTERVAL_0	0xA
#define DSI_UNIT_INTERVAL_1	0xA
#define DSI_UNIT_INTERVAL_2 	0x6

static bool rotate_main = true;
static bool display_initialize_during_boot;

static int __init startup_graphics_setup(char *str)
{
	if (!str)
		return 1;

	switch (*str) {
	case '0':
		pr_info("No Startup graphics support\n");
		display_initialize_during_boot = false;
		break;
	case '1':
		pr_info("Startup graphics supported\n");
		display_initialize_during_boot = true;
		break;
	default:
		display_initialize_during_boot = false;
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
	.synchronized_update = false,
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
	.synchronized_update = false,
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
#ifdef AV8100_HW_TE_I2SDAT3
	.sync_src = MCDE_SYNCSRC_TE1,
#else
	.sync_src = MCDE_SYNCSRC_TE0,
#endif /* AV8100_HW_TE_I2SDAT3 */
	.update_auto_trig = true,
	.phy = {
		.dsi = {
			.virt_id = 0,
			.num_data_lanes = 2,
			.ui = DSI_UNIT_INTERVAL_2,
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
	bool display_initialized;

	if (event != MCDE_DSS_EVENT_DISPLAY_REGISTERED)
		return 0;

	if (ddev->id < 0 || ddev->id >= ARRAY_SIZE(fbs))
		return 0;

	mcde_dss_get_native_resolution(ddev, &width, &height);

	display_initialized = (ddev->id == 0 && display_initialize_during_boot);
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
		rotate ? FB_ROTATE_CW : FB_ROTATE_UR,
		display_initialized);
	if (IS_ERR(fbs[ddev->id]))
		pr_warning("Failed to create fb for display %s\n", ddev->name);
	else
		pr_info("Framebuffer created (%s)\n", ddev->name);

	return 0;
}

static struct notifier_block display_nb = {
	.notifier_call = display_registered_callback,
};

int __init init_display_devices(void)
{
	int ret;

	ret = mcde_dss_register_notifier(&display_nb);
	if (ret)
		pr_warning("Failed to register dss notifier\n");

#ifdef CONFIG_DISPLAY_GENERIC_DSI_PRIMARY
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
