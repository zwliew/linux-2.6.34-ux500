/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * ST-Ericsson HDMI display driver
 *
 * Author: Per Persson <per-xb-persson@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/io.h>

#include <video/mcde_display.h>
#include <video/mcde_display-av8100.h>
#include <video/av8100.h>
#include <video/hdmi.h>

static int hdmi_try_video_mode(
	struct mcde_display_device *ddev, struct mcde_video_mode *video_mode);
static int hdmi_set_video_mode(
	struct mcde_display_device *ddev, struct mcde_video_mode *video_mode);

struct mcde_video_mode video_modes_supp[] =
{
/*	xres,	yres,	pixclk,	hbp,	hfp,	vbp,	vfp,	interlaced */
#ifndef CONFIG_AV8100_SDTV
	/* 640_480_60_P */
	{640,	480,	39682,	112, 	48,	33, 	12, 0, 0, 0},
	/* 720_480_60_P */
	{720,	480,	37000,	104,	34,	30,	15, 0, 0, 0},
	/* 720_576_50_P */
	{720,	576,	37037,	132,	12,	44,	5, 0, 0, 0},
	/* 1280_720_60_P */
	{1280,	720,	13468,	256,	114,	20,	10, 0, 0, 0},
	/* 1280_720_50_P */
	{1280,	720,	13468,	260,	440,	25,	5, 0, 0, 0},
	/* 1920_1080_30_P */
	{1920,	1080,	13468,	189,	91,	36,	9, 0, 0, 0},
	/* 1920_1080_24_P */
	{1920,	1080,	13468,	170,	660,	36,	9, 0, 0, 0},
	/* 1920_1080_25_P */
	{1920,	1080,	13468,	192,	528,	36,	9, 0, 0, 0},
#endif /* CONFIG_AV8100_SDTV */
	/* 720_480_60_I) */
	{720,	480,	74074,	126,	12,	44,	1, 0, 0, 1},
	/* 720_576_50_I) */
	{720,	576,	74074,	132,	12,	44,	5, 0, 0, 1},
#ifndef CONFIG_AV8100_SDTV
	/* 1920_1080_50_I) */
	{1920,	1080,	13468,	192,	528,	20,	25, 0, 0, 1},
	/* 1920_1080_60_I) */
	{1920,	1080,	13468,	192,	88,	20,	25, 0, 0, 1},
#endif /* CONFIG_AV8100_SDTV */
};

#define AV8100_MAX_LEVEL 255

static int hdmi_try_video_mode(
	struct mcde_display_device *ddev, struct mcde_video_mode *video_mode)
{
	int res = -EINVAL;
	int index = 0;
	int match_level = AV8100_MAX_LEVEL;
	int found_index = -1;

	if (ddev == NULL || video_mode == NULL) {
		pr_warning("%s:ddev = NULL or video_mode = NULL\n", __func__);
		goto hdmi_try_video_mode_out;
	}

	dev_vdbg(&ddev->dev, "%s\n", __func__);

#ifdef CONFIG_AV8100_SDTV
	video_mode->interlaced = true;
#endif /* CONFIG_AV8100_SDTV */

	while (index < sizeof(video_modes_supp)/
		sizeof(struct mcde_video_mode)) {
		/* 1. Check if all parameters match */
		if (memcmp(video_mode, &video_modes_supp[index],
			sizeof(struct mcde_video_mode)) == 0) {
			match_level = 1;
			found_index = index;
			break;
		}

		/* 2. Check if xres,yres,htot,vtot,interlaced match */
		if ((match_level > 2) &&
			(video_mode->xres == video_modes_supp[index].xres) &&
			(video_mode->yres == video_modes_supp[index].yres) &&
			((video_mode->xres + video_mode->hbp +
				video_mode->hfp) ==
			(video_modes_supp[index].xres +
				video_modes_supp[index].hbp +
				video_modes_supp[index].hfp)) &&
			((video_mode->yres + video_mode->vbp1 +
				video_mode->vbp2 + video_mode->vfp1 +
				video_mode->vfp2) ==
			(video_modes_supp[index].yres +
				video_modes_supp[index].vbp1 +
				video_modes_supp[index].vbp2 +
				video_modes_supp[index].vfp1 +
				video_modes_supp[index].vfp2)) &&
			(video_mode->interlaced ==
				video_modes_supp[index].interlaced)) {
			match_level = 2;
			found_index = index;
		}

		/* 3. Check if xres,yres,pixelclock,interlaced match */
		if ((match_level > 3) &&
			(video_mode->xres == video_modes_supp[index].xres) &&
			(video_mode->yres == video_modes_supp[index].yres) &&
			(video_mode->interlaced ==
				video_modes_supp[index].interlaced) &&
			(video_mode->pixclock ==
				video_modes_supp[index].pixclock)) {
			match_level = 3;
			found_index = index;
		}

		/* 4. Check if xres,yres,interlaced match */
		if ((match_level > 4) &&
			(video_mode->xres == video_modes_supp[index].xres) &&
			(video_mode->yres == video_modes_supp[index].yres) &&
			(video_mode->interlaced ==
				video_modes_supp[index].interlaced)) {
			match_level = 4;
			found_index = index;
		}

		index++;
	}

	if (found_index != -1) {
		res = 0;
		memset(video_mode, 0, sizeof(struct mcde_video_mode));
		memcpy(video_mode, &video_modes_supp[found_index],
			sizeof(struct mcde_video_mode));

		dev_dbg(&ddev->dev, "%s:HDMI video_mode %d chosen. Level:%d\n",
			__func__, found_index, match_level);
	} else {
		dev_dbg(&ddev->dev, "video_mode not accepted\n");
		dev_dbg(&ddev->dev, "xres:%d yres:%d pixclock:%d hbp:%d hfp:%d "
			"vfp1:%d vfp2:%d vbp1:%d vbp2:%d intlcd:%d\n",
			video_mode->xres, video_mode->yres,
			video_mode->pixclock, video_mode->hbp,
			video_mode->hfp, video_mode->vfp1, video_mode->vfp2,
			video_mode->vbp1, video_mode->vbp2,
			video_mode->interlaced);
	}

hdmi_try_video_mode_out:
	return res;
}

static int hdmi_set_video_mode(
	struct mcde_display_device *dev, struct mcde_video_mode *video_mode)
{
	int ret = -EINVAL;
	bool update = 0;
	union av8100_configuration av8100_config;
	struct mcde_display_hdmi_platform_data *pdata = dev->dev.platform_data;
	struct av8100_status status;

	/* TODO check video_mode_params */
	if (dev == NULL || video_mode == NULL) {
		pr_warning("%s:ddev = NULL or video_mode = NULL\n", __func__);
		goto out;
	}

	dev_vdbg(&dev->dev, "%s:\n", __func__);
	dev_vdbg(&dev->dev, "%s:xres:%d yres:%d hbp:%d hfp:%d vbp1:%d vfp1:%d "
		"vbp2:%d vfp2:%d interlaced:%d\n", __func__,
		video_mode->xres,
		video_mode->yres,
		video_mode->hbp,
		video_mode->hfp,
		video_mode->vbp1,
		video_mode->vfp1,
		video_mode->vbp2,
		video_mode->vfp2,
		video_mode->interlaced);

	memset(&(dev->video_mode), 0, sizeof(struct mcde_video_mode));
	memcpy(&(dev->video_mode), video_mode, sizeof(struct mcde_video_mode));

	if (dev->port->pixel_format == MCDE_PORTPIXFMT_DSI_YCBCR422)
		mcde_chnl_set_col_convert(dev->chnl_state,
						&pdata->rgb_2_yCbCr_convert);
	ret = mcde_chnl_set_video_mode(dev->chnl_state, &dev->video_mode);
	if (ret < 0) {
		dev_warn(&dev->dev, "Failed to set video mode\n");
		goto out;
	}

	/* TODO: We shouldn't need to shutdown */
	status = av8100_status_get();
	if (status.av8100_state >= AV8100_OPMODE_STANDBY) {
		/* Disable interrupts */
		ret = av8100_disable_interrupt();
		if (ret != AV8100_OK) {
			dev_err(&dev->dev,
				"%s:av8100_disable_interrupt failed\n",
				__func__);
			goto out;
		}

		ret = av8100_powerdown();
		if (ret != AV8100_OK) {
			dev_err(&dev->dev, "av8100_powerdown failed\n");
			goto out;
		}

		/* TODO: What delay is needed here */
		msleep(10);
	}

	status = av8100_status_get();
	if (status.av8100_state < AV8100_OPMODE_STANDBY) {
		ret = av8100_powerup();
		if (ret != AV8100_OK) {
			dev_err(&dev->dev, "av8100_powerup failed\n");
			goto out;
		}

		ret = av8100_download_firmware(NULL, 0, I2C_INTERFACE);
		if (ret != AV8100_OK) {
			dev_err(&dev->dev, "av8100_download_firmware failed\n");
			goto out;
		}
	}

	/* Get current av8100 video output format */
	ret = av8100_conf_get(AV8100_COMMAND_VIDEO_OUTPUT_FORMAT,
		&av8100_config);
	if (ret != AV8100_OK) {
		dev_err(&dev->dev, "%s:av8100_conf_get "
			"AV8100_COMMAND_VIDEO_OUTPUT_FORMAT failed\n",
			__func__);
		goto out;
	}

	av8100_config.video_output_format.video_output_cea_vesa =
/* TODO: Remove #ifdefs in driver code. This is a dynamic property! */
#ifdef CONFIG_AV8100_SDTV
		dev->video_mode.yres == 576 ? AV8100_CEA21_22_576I_PAL_50HZ
					: AV8100_CEA6_7_NTSC_60HZ;
#else
		av8100_video_output_format_get(
			dev->video_mode.xres,
			dev->video_mode.yres,
			dev->video_mode.xres +
				dev->video_mode.hbp + dev->video_mode.hfp,
			dev->video_mode.yres +
				dev->video_mode.vbp1 + dev->video_mode.vfp1 +
				dev->video_mode.vbp2 + dev->video_mode.vfp2,
			dev->video_mode.pixclock,
			dev->video_mode.interlaced);
#endif
	if (AV8100_VIDEO_OUTPUT_CEA_VESA_MAX ==
		av8100_config.video_output_format.video_output_cea_vesa) {
		dev_err(&dev->dev, "%s:video output format not found "
			"\n", __func__);
		goto out;
	}

	ret = av8100_conf_prep(AV8100_COMMAND_VIDEO_OUTPUT_FORMAT,
		&av8100_config);
	if (ret != AV8100_OK) {
		dev_err(&dev->dev, "%s:av8100_conf_prep "
			"AV8100_COMMAND_VIDEO_OUTPUT_FORMAT failed\n",
			__func__);
		goto out;
	}

	/* Get current av8100 video input format */
	ret = av8100_conf_get(AV8100_COMMAND_VIDEO_INPUT_FORMAT,
		&av8100_config);
	if (ret != AV8100_OK) {
		dev_err(&dev->dev, "%s:av8100_conf_get "
			"AV8100_COMMAND_VIDEO_INPUT_FORMAT failed\n",
			__func__);
		goto out;
	}

	/* Set correct av8100 video input pixel format */
	switch (dev->port->pixel_format) {
	case MCDE_PORTPIXFMT_DSI_16BPP:
	default:
		av8100_config.video_input_format.input_pixel_format =
			AV8100_INPUT_PIX_RGB565;
		break;
	case MCDE_PORTPIXFMT_DSI_18BPP:
		av8100_config.video_input_format.input_pixel_format =
			AV8100_INPUT_PIX_RGB666;
		break;
	case MCDE_PORTPIXFMT_DSI_18BPP_PACKED:
		av8100_config.video_input_format.input_pixel_format =
			AV8100_INPUT_PIX_RGB666P;
		break;
	case MCDE_PORTPIXFMT_DSI_24BPP:
		av8100_config.video_input_format.input_pixel_format =
			AV8100_INPUT_PIX_RGB888;
		break;
	case MCDE_PORTPIXFMT_DSI_YCBCR422:
		av8100_config.video_input_format.input_pixel_format =
			/*
			 * The following is expected:
			 * AV8100_INPUT_PIX_YCBCR422;
			 * However 565 is used for now and the colour converter
			 * is used to transform the correct colour.
			 */
			AV8100_INPUT_PIX_RGB565;
		break;
	}

	/*  Set ui_x4 */
	av8100_config.video_input_format.ui_x4 = dev->port->phy.dsi.ui;

	ret = av8100_conf_prep(AV8100_COMMAND_VIDEO_INPUT_FORMAT,
		&av8100_config);
	if (ret != AV8100_OK) {
		dev_err(&dev->dev, "%s:av8100_conf_prep "
				"AV8100_COMMAND_VIDEO_INPUT_FORMAT failed\n",
				__func__);
		goto out;
	}

	ret = av8100_conf_w(AV8100_COMMAND_VIDEO_INPUT_FORMAT,
		NULL, NULL, I2C_INTERFACE);
	if (ret != AV8100_OK) {
		dev_err(&dev->dev, "%s:av8100_conf_w "
				"AV8100_COMMAND_VIDEO_INPUT_FORMAT failed\n",
				__func__);
		goto out;
	}

/* TODO: Remove #ifdefs in driver code. This is a dynamic property! */
#ifdef CONFIG_AV8100_SDTV
	if (dev->port->pixel_format != MCDE_PORTPIXFMT_DSI_YCBCR422) {
		av8100_config.color_space_conversion_format =
							col_cvt_rgb_to_denc;
	} else {
		av8100_config.color_space_conversion_format =
							col_cvt_yuv422_to_denc;
	}
#else
	if (dev->port->pixel_format == MCDE_PORTPIXFMT_DSI_YCBCR422) {
		av8100_config.color_space_conversion_format =
							col_cvt_yuv422_to_rgb;
	} else {
		av8100_config.color_space_conversion_format =
							col_cvt_identity;
	}
#endif

	ret = av8100_conf_prep(
		AV8100_COMMAND_COLORSPACECONVERSION,
		&av8100_config);
	if (ret != AV8100_OK) {
		dev_err(&dev->dev, "%s:av8100_configuration_prepare "
			"AV8100_COMMAND_COLORSPACECONVERSION failed\n",
			__func__);
		goto out;
	}

	ret = av8100_conf_w(
			AV8100_COMMAND_COLORSPACECONVERSION,
			NULL, NULL, I2C_INTERFACE);
	if (ret != AV8100_OK) {
		dev_err(&dev->dev, "%s:av8100_conf_w "
			"AV8100_COMMAND_COLORSPACECONVERSION failed\n",
			__func__);
		goto out;
	}

	/* Set video output format */
	ret = av8100_conf_w(AV8100_COMMAND_VIDEO_OUTPUT_FORMAT,
		NULL, NULL, I2C_INTERFACE);
	if (ret != AV8100_OK) {
		dev_err(&dev->dev, "av8100_conf_w failed\n");
		goto out;
	}

	/* Set audio input format */
	ret = av8100_conf_w(AV8100_COMMAND_AUDIO_INPUT_FORMAT,
		NULL, NULL, I2C_INTERFACE);
	if (ret != AV8100_OK) {
		dev_err(&dev->dev, "%s:av8100_conf_w "
				"AV8100_COMMAND_AUDIO_INPUT_FORMAT failed\n",
			__func__);
		goto out;
	}

	/* Get current av8100 video denc settings format */
	ret = av8100_conf_get(AV8100_COMMAND_DENC,
		&av8100_config);
	if (ret != AV8100_OK) {
		dev_err(&dev->dev, "%s:av8100_conf_get "
				"AV8100_COMMAND_DENC failed\n", __func__);
		goto out;
	}
#ifdef CONFIG_AV8100_SDTV
	update = true;
	if (dev->video_mode.yres == 576) {
		av8100_config.denc_format.standard_selection = AV8100_PAL_BDGHI;
		av8100_config.denc_format.cvbs_video_format  = AV8100_CVBS_625;
	} else {
		av8100_config.denc_format.standard_selection = AV8100_NTSC_M;
		av8100_config.denc_format.cvbs_video_format  = AV8100_CVBS_525;
	}
#else
	update = (av8100_config.denc_format.on_off != 0);
#endif

	if (update) {
#ifdef CONFIG_AV8100_SDTV
		av8100_config.denc_format.on_off = 1;
#else
		av8100_config.denc_format.on_off = 0;
#endif
		ret = av8100_conf_prep(AV8100_COMMAND_DENC,
			&av8100_config);
		if (ret != AV8100_OK) {
			dev_err(&dev->dev, "%s:av8100_conf_prep "
				"AV8100_COMMAND_DENC failed\n", __func__);
			goto out;
		}

		/* TODO: prepare depending on OUT fmt */
		ret = av8100_conf_w(AV8100_COMMAND_DENC,
			NULL, NULL, I2C_INTERFACE);
		if (ret != AV8100_OK) {
			dev_err(&dev->dev, "%s:av8100_conf_w "
				"AV8100_COMMAND_DENC failed\n", __func__);
			goto out;
		}
	}

	dev->update_flags |= UPDATE_FLAG_VIDEO_MODE;

	return ret;
out:
	return ret;
}

static int hdmi_on_first_update(struct mcde_display_device *dev)
{
	int ret = 0;
	union av8100_configuration av8100_config;

	dev->first_update = false;

	/* HDMI on*/
	av8100_config.hdmi_format.hdmi_mode = AV8100_HDMI_ON;
	av8100_config.hdmi_format.hdmi_format = AV8100_HDMI;
	av8100_config.hdmi_format.dvi_format = AV8100_DVI_CTRL_CTL0;

	ret = av8100_conf_prep(AV8100_COMMAND_HDMI,
		&av8100_config);
	if (ret != AV8100_OK) {
		dev_err(&dev->dev, "%s:av8100_conf_prep "
			"AV8100_COMMAND_HDMI failed\n", __func__);
		goto out;
	}

	/* Enable interrupts */
	ret = av8100_enable_interrupt();
	if (ret != AV8100_OK) {
		dev_err(&dev->dev, "%s:av8100_enable_interrupt failed\n",
			__func__);
		goto out;
	}

	ret = av8100_conf_w(AV8100_COMMAND_HDMI, NULL,
		NULL, I2C_INTERFACE);
	if (ret != AV8100_OK) {
		dev_err(&dev->dev, "%s:av8100_conf_w "
			"AV8100_COMMAND_HDMI failed\n", __func__);
		goto out;
	}

	return ret;
out:
	return ret;
}

static int __devinit hdmi_probe(struct mcde_display_device *dev)
{
	int ret = EINVAL;
	struct mcde_display_hdmi_platform_data *pdata =
		dev->dev.platform_data;

	if (pdata == NULL) {
		dev_err(&dev->dev, "%s:Platform data missing\n", __func__);
		goto no_pdata;
	}

	if (dev->port->type != MCDE_PORTTYPE_DSI) {
		dev_err(&dev->dev, "%s:Invalid port type %d\n",
			__func__, dev->port->type);
		goto invalid_port_type;
	}

	/* DSI use clock continous mode if AV8100_CHIPVER_1 > 1 */
	if (av8100_ver_get() > AV8100_CHIPVER_1)
		dev->port->phy.dsi.clk_cont = true;

	dev->prepare_for_update = NULL;
	dev->on_first_update = hdmi_on_first_update;
	dev->try_video_mode = hdmi_try_video_mode;
	dev->set_video_mode = hdmi_set_video_mode;

	dev_info(&dev->dev, "HDMI display probed\n");

	ret = 0;
	goto out;
invalid_port_type:
no_pdata:
out:
	return ret;
}

static int __devexit hdmi_remove(struct mcde_display_device *dev)
{
	struct mcde_display_hdmi_platform_data *pdata =
		dev->dev.platform_data;

	dev->set_power_mode(dev, MCDE_DISPLAY_PM_OFF);

	if (pdata->hdmi_platform_enable) {
		if (pdata->regulator)
			regulator_put(pdata->regulator);
		if (pdata->reset_gpio) {
			gpio_direction_input(pdata->reset_gpio);
			gpio_free(pdata->reset_gpio);
		}
	}

	return 0;
}

static int hdmi_resume(struct mcde_display_device *ddev)
{
	int ret;
	ret = av8100_powerup();
	if (ret != AV8100_OK) {
		dev_err(&ddev->dev, "%s:av8100_powerup failed\n", __func__);
		goto out;
	}

	ret = av8100_download_firmware(NULL, 0, I2C_INTERFACE);
	if (ret != AV8100_OK) {
		dev_err(&ddev->dev, "%s:av8100_download_firmware failed\n",
			__func__);
		goto out;
	}

out:
	return ret;
}

static int hdmi_suspend(struct mcde_display_device *ddev, pm_message_t state)
{
	int ret;
	ret = av8100_powerdown();
	if (ret != AV8100_OK) {
		dev_err(&ddev->dev, "%s:av8100_powerdown failed\n", __func__);
		goto out;
	}
out:
	return ret;
}

static struct mcde_display_driver hdmi_driver = {
	.probe	= hdmi_probe,
	.remove = hdmi_remove,
	.suspend = hdmi_suspend,
	.resume = hdmi_resume,
	.driver = {
		.name	= "av8100_hdmi",
	},
};

/* Module init */
static int __init mcde_display_hdmi_init(void)
{
	int retval;
	pr_info("%s\n", __func__);

	retval = mcde_display_driver_register(&hdmi_driver);

	return retval;
}
late_initcall(mcde_display_hdmi_init);

static void __exit mcde_display_hdmi_exit(void)
{
	pr_info("%s\n", __func__);

	mcde_display_driver_unregister(&hdmi_driver);
}
module_exit(mcde_display_hdmi_exit);

MODULE_AUTHOR("Per Persson <per.xb.persson@stericsson.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ST-Ericsson hdmi display driver");
