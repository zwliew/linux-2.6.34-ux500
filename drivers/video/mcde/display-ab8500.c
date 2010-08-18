/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * AB8500 display driver
 *
 * Author: Marcel Tunnissen <marcel.tuennissen@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/regulator/consumer.h>
#include <video/mcde_display.h>
#include <video/mcde_display-ab8500.h>
#include <mach/ab8500_denc.h>

#define AB8500_DISP_TRACE	dev_dbg(&ddev->dev, "%s\n", __func__)

static int try_video_mode(struct mcde_display_device *ddev,
				struct mcde_video_mode *video_mode);
static int set_video_mode(struct mcde_display_device *ddev,
				struct mcde_video_mode *video_mode);
static int set_power_mode(struct mcde_display_device *ddev,
				enum mcde_display_power_mode power_mode);
static int on_first_update(struct mcde_display_device *ddev);
static int display_update(struct mcde_display_device *ddev);

static int __devinit ab8500_probe(struct mcde_display_device *ddev)
{
	int ret = 0;
	struct ab8500_display_platform_data *pdata = ddev->dev.platform_data;
	struct ab8500_denc_conf *driver_data;
	AB8500_DISP_TRACE;

	if (pdata == NULL) {
		dev_err(&ddev->dev, "%s:Platform data missing\n", __func__);
		ret = -EINVAL;
		goto no_pdata;
	}
	if (ddev->port->type != MCDE_PORTTYPE_DPI) {
		dev_err(&ddev->dev, "%s:Invalid port type %d\n", __func__,
							ddev->port->type);
		ret = -EINVAL;
		goto invalid_port_type;
	}
	ddev->dev.driver_data = kmalloc(sizeof(struct ab8500_denc_conf),
								GFP_KERNEL);
	if (!ddev->dev.driver_data) {
		dev_err(&ddev->dev, "Failed to allocate driver data\n");
		ret = -ENOMEM;
		goto no_driver_data;
	}
	driver_data = (struct ab8500_denc_conf *) ddev->dev.driver_data;
	memset(driver_data, 0, sizeof(struct ab8500_denc_conf));

	/* initialise the device */
	if (pdata->regulator_id) {
		pdata->regulator = regulator_get(&ddev->dev,
						pdata->regulator_id);
		if (IS_ERR(pdata->regulator)) {
				ret = PTR_ERR(pdata->regulator);
				dev_warn(&ddev->dev,
					"%s:Failed to get regulator '%s'\n",
					__func__, pdata->regulator_id);
				pdata->regulator = NULL;
				goto regulator_get_failed;
		}
		ret = regulator_enable(pdata->regulator);
		if (ret < 0) {
			dev_err(&ddev->dev, "%s:Failed to enable regulator\n",
								__func__);
			goto regu_failed;
		}
	} else {
		/* TODO remove as soon as we use a kernel with support
		 * for ab8500 supports regulators */
		/* enable tv voltage, no lp mode */
		ab8500_denc_regu_setup(true, false);
	}

	ddev->try_video_mode = try_video_mode;
	ddev->set_video_mode = set_video_mode;
	ddev->set_power_mode = set_power_mode;
	ddev->on_first_update = on_first_update;
	ddev->update = display_update;
	ddev->prepare_for_update = NULL;

out:
	return ret;
regu_failed:
regulator_get_failed:
	kfree(ddev->dev.driver_data);
no_driver_data:
invalid_port_type:
no_pdata:
	goto out;
}

static int __devexit ab8500_remove(struct mcde_display_device *ddev)
{
	struct ab8500_display_platform_data *pdata = ddev->dev.platform_data;
	AB8500_DISP_TRACE;

	if (pdata->regulator)
		regulator_put(pdata->regulator);

	kfree(ddev->dev.driver_data);
	ddev->dev.driver_data = NULL;
	return 0;
}

static int ab8500_resume(struct mcde_display_device *ddev)
{
	int res = 0;
	struct ab8500_display_platform_data *pdata = ddev->dev.platform_data;
	AB8500_DISP_TRACE;

	if (pdata->regulator) {
		res = regulator_enable(pdata->regulator);
		if (res < 0) {
			dev_err(&ddev->dev, "%s:Failed to enable regulator\n",
								__func__);
			goto regu_failed;
		}
	} else {
		/* TODO remove as soon as we use a kernel with support
		 * for ab8500 supports regulators */
		/* enable tv voltage, no lp mode */
		ab8500_denc_regu_setup(true, false);
	}

	ab8500_denc_power_up();
	ab8500_denc_reset(/* hard = */ true);

	return res;
regu_failed:
	return 0;
}

static int ab8500_suspend(struct mcde_display_device *ddev, pm_message_t state)
{
	int res = 0;
	struct ab8500_display_platform_data *pdata = ddev->dev.platform_data;
	AB8500_DISP_TRACE;

	if (pdata->regulator) {
		res = regulator_disable(pdata->regulator);
		if (res != 0) {
			dev_err(&ddev->dev, "%s:Failed to disable regulator\n",
								__func__);
			goto regu_failed;
		}
	}
	ab8500_denc_power_down();
	return res;
regu_failed:
	return res;
}


static struct mcde_display_driver ab8500_driver = {
	.probe	= ab8500_probe,
	.remove = ab8500_remove,
	.suspend = ab8500_suspend,
	.resume = ab8500_resume,
	.driver = {
		.name	= "mcde_tv_ab8500",
	},
};

static void print_vmode(struct mcde_video_mode *vmode)
{
	pr_debug("resolution: %dx%d\n", vmode->xres, vmode->yres);
	pr_debug("  pixclock: %d\n",    vmode->pixclock);
	pr_debug("       hbp: %d\n",    vmode->hbp);
	pr_debug("       hfp: %d\n",    vmode->hfp);
	pr_debug("      vbp1: %d\n",    vmode->vbp1);
	pr_debug("      vfp1: %d\n",    vmode->vfp1);
	pr_debug("      vbp2: %d\n",    vmode->vbp2);
	pr_debug("      vfp2: %d\n",    vmode->vfp2);
	pr_debug("interlaced: %s\n", vmode->interlaced ? "true" : "false");
}

static int try_video_mode(
	struct mcde_display_device *ddev, struct mcde_video_mode *video_mode)
{
	int res = -EINVAL;
	AB8500_DISP_TRACE;

	if (ddev == NULL || video_mode == NULL) {
		dev_warn(&ddev->dev, "%s:ddev = NULL or video_mode = NULL\n",
			__func__);
		return res;
	}

	/* TODO: move this part to MCDE: mcde_dss_try_video_mode? */
	if (video_mode->xres == 720) {
		/* check for PAL */
		if (video_mode->yres == 576) {
			/* set including SAV/EAV: */
			video_mode->hbp = 132;
			video_mode->hfp =  12;
			/*
			 * Total nr of active lines:  576
			 * Total nr of blanking lines: 49
			 */
			video_mode->vbp1 = 22;
			video_mode->vfp1 =  2;
			video_mode->vbp2 = 23;
			video_mode->vfp2 =  2;
			/* currently only support interlaced */
			video_mode->interlaced = true;
			video_mode->pixclock = 37037;
			res = 0;
		} else if (video_mode->yres == 480) {
			/* set including SAV/EAV */
			video_mode->hbp = 122;
			video_mode->hfp =  16;
			/*
			 * Total nr of active lines:  486.
			 * Total nr of blanking lines: 39.
			 */
			/* Why does the following work? As far as I know the
			 * total nr of vertical blanking lines between the
			 * fields equals to 19 or 20, of which 3 are vfp.
			 */
			video_mode->vbp1 = 19;
			video_mode->vfp1 = 3;
			video_mode->vbp2 = 20;
			video_mode->vfp2 = 3;
			/* currently only support interlaced */
			video_mode->interlaced = true;
			video_mode->pixclock = 37037;
			res = 0;
		}
	}

	if (res == 0)
		print_vmode(video_mode);
	else
		dev_warn(&ddev->dev,
			"%s:Failed to find video mode x=%d, y=%d\n",
			__func__, video_mode->xres, video_mode->yres);

	return res;

}

static int set_video_mode(
	struct mcde_display_device *ddev, struct mcde_video_mode *video_mode)
{
	int res = -EINVAL;
	struct ab8500_display_platform_data *pdata = ddev->dev.platform_data;
	struct ab8500_denc_conf *driver_data = (struct ab8500_denc_conf *)
							ddev->dev.driver_data;
	AB8500_DISP_TRACE;

	if (ddev == NULL || video_mode == NULL) {
		dev_warn(&ddev->dev, "%s:ddev = NULL or video_mode = NULL\n",
			__func__);
		goto out;
	}
	ddev->video_mode = *video_mode;
	if (video_mode->xres == 720) {
		/* check for PAL BDGHI and N */
		if (video_mode->yres == 576) {
			driver_data->TV_std = TV_STD_PAL_BDGHI;
			/* TODO: how to choose LOW DEF FILTER */
			driver_data->cr_filter = TV_CR_PAL_HIGH_DEF_FILTER;
			/* TODO: PAL N (e.g. uses a setup of 7.5 IRE) */
			driver_data->black_level_setup = false;
			res = 0;
		} else if (video_mode->yres == 480) { /* NTSC, PAL M */
			/* TODO: PAL M */
			driver_data->TV_std = TV_STD_NTSC_M;
			/* TODO: how to choose LOW DEF FILTER */
			driver_data->cr_filter = TV_CR_NTSC_HIGH_DEF_FILTER;
			driver_data->black_level_setup = true;
			res = 0;
		}
	}
	if (res < 0) {
		dev_warn(&ddev->dev, "%s:Failed to set video mode x=%d, y=%d\n",
			__func__, video_mode->xres, video_mode->yres);
		goto error;
	}

	driver_data->progressive 	= !video_mode->interlaced;
	driver_data->act_output		= true;
	driver_data->test_pattern   	= false;
	driver_data->partial_blanking	= true;
	driver_data->blank_all		= false;
	driver_data->suppress_col	= false;
	driver_data->phase_reset_mode	= TV_PHASE_RST_MOD_DISABLE;
	driver_data->dac_enable		= false;
	driver_data->act_dc_output  	= true;

	set_power_mode(ddev, MCDE_DISPLAY_PM_STANDBY);
	mcde_chnl_set_col_convert(ddev->chnl_state,
						&pdata->rgb_2_yCbCr_convert);
	mcde_chnl_stop_flow(ddev->chnl_state);
	res = mcde_chnl_set_video_mode(ddev->chnl_state, &ddev->video_mode);
	if (res < 0) {
		dev_warn(&ddev->dev, "%s:Failed to set video mode on channel\n",
			__func__);

		goto error;
	}
	ddev->update_flags |= UPDATE_FLAG_VIDEO_MODE;
	return res;
out:
error:
	return res;
}

static int set_power_mode(struct mcde_display_device *ddev,
	enum mcde_display_power_mode power_mode)
{
	int ret = 0;
	AB8500_DISP_TRACE;

	/* OFF -> STANDBY */
	if (ddev->power_mode == MCDE_DISPLAY_PM_OFF &&
					power_mode >= MCDE_DISPLAY_PM_OFF) {
		dev_vdbg(&ddev->dev, "/* OFF -> STANDBY */\n");
		if (ddev->platform_enable)
			ret = ddev->platform_enable(ddev);
		ab8500_denc_power_up();
		if (!ret) {
			ab8500_denc_reset(/* hard = */ true);
			ddev->power_mode = MCDE_DISPLAY_PM_STANDBY;
		}
	}
	/* STANDBY -> ON */
	if (ddev->power_mode == MCDE_DISPLAY_PM_STANDBY &&
					power_mode == MCDE_DISPLAY_PM_ON) {
		dev_vdbg(&ddev->dev, "/* STANDBY -> ON */\n");
		ddev->power_mode = MCDE_DISPLAY_PM_ON;
	}
	/* ON -> STANDBY */
	if (!ret && ddev->power_mode == MCDE_DISPLAY_PM_ON &&
					power_mode <= MCDE_DISPLAY_PM_STANDBY) {
		dev_vdbg(&ddev->dev, "/* ON -> STANDBY */\n");
		ab8500_denc_reset(/* hard = */ false);
		ddev->power_mode = MCDE_DISPLAY_PM_STANDBY;
	}
	/* STANDBY -> OFF */
	if (!ret && ddev->power_mode == MCDE_DISPLAY_PM_STANDBY &&
					power_mode == MCDE_DISPLAY_PM_OFF) {
		dev_vdbg(&ddev->dev, "/* STANDBY -> OFF */\n");
		if (ddev->platform_disable)
			ret = ddev->platform_disable(ddev);
		ab8500_denc_power_down();
		if (!ret)
			ddev->power_mode = MCDE_DISPLAY_PM_OFF;
	}

	return ret;
}

static int on_first_update(struct mcde_display_device *ddev)
{
	ab8500_denc_conf((struct ab8500_denc_conf *) ddev->dev.driver_data);
	ab8500_denc_conf_plug_detect(true, false, TV_PLUG_TIME_2S);
	ab8500_denc_mask_int_plug_det(false, false);
	ddev->first_update = false;
	return 0;
}

static int display_update(struct mcde_display_device *ddev)
{
	int ret;
	if (ddev->first_update)
		on_first_update(ddev);
	if (ddev->power_mode != MCDE_DISPLAY_PM_ON && ddev->set_power_mode) {
		ret = set_power_mode(ddev, MCDE_DISPLAY_PM_ON);
		if (ret < 0)
			goto error;
	}
	ret = mcde_chnl_update(ddev->chnl_state, &ddev->update_area);
	if (ret < 0)
		goto error;
out:
	return ret;
error:
	dev_warn(&ddev->dev, "%s:Failed to set power mode to on\n", __func__);
	goto out;
}

/* Module init */
static int __init mcde_display_tvout_ab8500_init(void)
{
	pr_debug("%s\n", __func__);

	return mcde_display_driver_register(&ab8500_driver);
}
late_initcall(mcde_display_tvout_ab8500_init);

static void __exit mcde_display_tvout_ab8500_exit(void)
{
	pr_debug("%s\n", __func__);

	mcde_display_driver_unregister(&ab8500_driver);
}
module_exit(mcde_display_tvout_ab8500_exit);

MODULE_AUTHOR("Marcel Tunnissen <marcel.tuennissen@stericsson.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ST-Ericsson MCDE TVout through AB8500 display driver");
