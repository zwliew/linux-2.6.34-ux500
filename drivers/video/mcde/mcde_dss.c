/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * ST-Ericsson MCDE display sub system driver
 *
 * Author: Marcus Lorentzon <marcus.xm.lorentzon@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/err.h>

#include <video/mcde_dss.h>

#define to_overlay(x) container_of(x, struct mcde_overlay, kobj)

void overlay_release(struct kobject *kobj)
{
	struct mcde_overlay *ovly = to_overlay(kobj);

	kfree(ovly);
}

struct kobj_type ovly_type = {
	.release = overlay_release,
};

static int apply_overlay(struct mcde_overlay *ovly,
				struct mcde_overlay_info *info, bool force)
{
	int ret = 0;
	if (ovly->ddev->invalidate_area) {
		/* TODO: transform ovly coord to screen coords (vmode):
		 * add offset
		 */
		struct mcde_rectangle dirty = info->dirty;
		ret = ovly->ddev->invalidate_area(ovly->ddev, &dirty);
	}

	if (ovly->info.paddr != info->paddr || force)
		mcde_ovly_set_source_buf(ovly->state, info->paddr);

	if (ovly->info.stride != info->stride || ovly->info.fmt != info->fmt ||
									force)
		mcde_ovly_set_source_info(ovly->state, info->stride, info->fmt);
	if (ovly->info.src_x != info->src_x ||
					ovly->info.src_y != info->src_y ||
					ovly->info.w != info->w ||
					ovly->info.h != info->h || force)
		mcde_ovly_set_source_area(ovly->state,
				info->src_x, info->src_y, info->w, info->h);
	if (ovly->info.dst_x != info->dst_x || ovly->info.dst_y != info->dst_y
					|| ovly->info.dst_z != info->dst_z ||
					force)
		mcde_ovly_set_dest_pos(ovly->state,
					info->dst_x, info->dst_y, info->dst_z);

	mcde_ovly_apply(ovly->state);
	ovly->info = *info;

	return ret;
}

/* MCDE DSS operations */

int mcde_dss_enable_display(struct mcde_display_device *ddev,
				bool display_initialized)
{
	int ret;
	struct mcde_chnl_state *chnl;

	if (ddev->enabled)
		return 0;

	/* Acquire MCDE resources */
	chnl = mcde_chnl_get(ddev->chnl_id, ddev->fifo, ddev->port);
	if (IS_ERR(chnl)) {
		ret = PTR_ERR(chnl);
		dev_warn(&ddev->dev, "Failed to acquire MCDE channel\n");
		goto get_chnl_failed;
	}
	ddev->chnl_state = chnl;
	if (!display_initialized) {
		/* Initiate display communication */
		ret = ddev->set_power_mode(ddev, MCDE_DISPLAY_PM_STANDBY);
		if (ret < 0) {
			dev_warn(&ddev->dev, "Failed to initialize display\n");
			goto display_failed;
		}

		dev_dbg(&ddev->dev, "Display enabled, chnl=%d\n",
						ddev->chnl_id);
	} else {
		printk(KERN_INFO "Display already enabled, chnl=%d\n",
						ddev->chnl_id);
		ddev->power_mode = MCDE_DISPLAY_PM_ON;
		ret = 0;
	}
	ddev->enabled = true;
out:
	return ret;

display_failed:
	mcde_chnl_put(ddev->chnl_state);
	ddev->chnl_state = NULL;
get_chnl_failed:
	goto out;
}

void mcde_dss_disable_display(struct mcde_display_device *ddev)
{
	if (!ddev->enabled)
		return;

	/* TODO: Disable overlays */

	(void)ddev->set_power_mode(ddev, MCDE_DISPLAY_PM_OFF);
	mcde_chnl_put(ddev->chnl_state);
	ddev->chnl_state = NULL;

	ddev->enabled = false;

	dev_dbg(&ddev->dev, "Display disabled, chnl=%d\n", ddev->chnl_id);
}

int mcde_dss_apply_channel(struct mcde_display_device *ddev)
{
	if (ddev->apply_config)
		return ddev->apply_config(ddev);
	else
		return -EINVAL;
}

struct mcde_overlay *mcde_dss_create_overlay(struct mcde_display_device *ddev,
	struct mcde_overlay_info *info)
{
	struct mcde_overlay *ovly;

	ovly = kzalloc(sizeof(struct mcde_overlay), GFP_KERNEL);
	if (!ovly)
		return NULL;

	kobject_init(&ovly->kobj, &ovly_type); /* Local ref */
	kobject_get(&ovly->kobj); /* Creator ref */
	INIT_LIST_HEAD(&ovly->list);
	list_add(&ddev->ovlys, &ovly->list);
	ovly->info = *info;
	ovly->ddev = ddev;

	return ovly;
}

void mcde_dss_destroy_overlay(struct mcde_overlay *ovly)
{
	list_del(&ovly->list);
	if (ovly->state)
		mcde_dss_disable_overlay(ovly);
	kobject_put(&ovly->kobj);
}

int mcde_dss_enable_overlay(struct mcde_overlay *ovly)
{
	int ret = 0;

	if (!ovly->ddev->chnl_state)
		return -EINVAL;

	if (!ovly->state) {
		struct mcde_ovly_state *state;
		state = mcde_ovly_get(ovly->ddev->chnl_state);
		if (IS_ERR(state)) {
			ret = PTR_ERR(state);
			dev_warn(&ovly->ddev->dev,
				"Failed to acquire overlay\n");
			goto get_ovly_failed;
		}
		ovly->state = state;
	}

	apply_overlay(ovly, &ovly->info, true);

	dev_vdbg(&ovly->ddev->dev, "Overlay enabled, chnl=%d\n",
							ovly->ddev->chnl_id);

	goto out;
get_ovly_failed:
out:
	return ret;
}

int mcde_dss_apply_overlay(struct mcde_overlay *ovly,
						struct mcde_overlay_info *info)
{
	if (info == NULL)
		info = &ovly->info;
	return apply_overlay(ovly, info, false);
}

void mcde_dss_disable_overlay(struct mcde_overlay *ovly)
{
	if (!ovly->state)
		return;

	mcde_ovly_put(ovly->state);

	dev_dbg(&ovly->ddev->dev, "Overlay disabled, chnl=%d\n",
							ovly->ddev->chnl_id);

	ovly->state = NULL;
}

int mcde_dss_update_overlay(struct mcde_overlay *ovly)
{
	int ret = 0;

	dev_vdbg(&ovly->ddev->dev, "Overlay update, chnl=%d\n",
							ovly->ddev->chnl_id);

	if (!ovly->state || !ovly->ddev->update || !ovly->ddev->invalidate_area)
		return -EINVAL;

	ret = ovly->ddev->update(ovly->ddev);
	if (ret)
		goto out;
	ret = ovly->ddev->invalidate_area(ovly->ddev, NULL);

out:
	return ret;
}

void mcde_dss_get_native_resolution(struct mcde_display_device *ddev,
	u16 *x_res, u16 *y_res)
{
	ddev->get_native_resolution(ddev, x_res, y_res);
}

enum mcde_ovly_pix_fmt mcde_dss_get_default_pixel_format(
	struct mcde_display_device *ddev)
{
	return ddev->get_default_pixel_format(ddev);
}

void mcde_dss_get_physical_size(struct mcde_display_device *ddev,
	u16 *physical_width, u16 *physical_height)
{
	ddev->get_physical_size(ddev, physical_width, physical_height);
}

int mcde_dss_try_video_mode(struct mcde_display_device *ddev,
	struct mcde_video_mode *video_mode)
{
	return ddev->try_video_mode(ddev, video_mode);
}

int mcde_dss_set_video_mode(struct mcde_display_device *ddev,
	struct mcde_video_mode *vmode)
{
	int ret;
	struct mcde_video_mode old_vmode;

	ddev->get_video_mode(ddev, &old_vmode);
	if (memcmp(vmode, &old_vmode, sizeof(old_vmode)) == 0)
		return 0;

	ret = ddev->set_video_mode(ddev, vmode);
	if (ret)
		return ret;

	return ddev->invalidate_area(ddev, NULL);
}

void mcde_dss_get_video_mode(struct mcde_display_device *ddev,
	struct mcde_video_mode *video_mode)
{
	ddev->get_video_mode(ddev, video_mode);
}

int mcde_dss_set_pixel_format(struct mcde_display_device *ddev,
	enum mcde_ovly_pix_fmt pix_fmt)
{
	enum mcde_ovly_pix_fmt old_pix_fmt;

	old_pix_fmt = ddev->get_pixel_format(ddev);
	if (old_pix_fmt == pix_fmt)
		return 0;

	return ddev->set_pixel_format(ddev, pix_fmt);
}

int mcde_dss_get_pixel_format(struct mcde_display_device *ddev)
{
	return ddev->get_pixel_format(ddev);
}

int mcde_dss_set_rotation(struct mcde_display_device *ddev,
	enum mcde_display_rotation rotation)
{
	enum mcde_display_rotation old_rotation;

	old_rotation = ddev->get_rotation(ddev);
	if (old_rotation == rotation)
		return 0;

	return ddev->set_rotation(ddev, rotation);
}

enum mcde_display_rotation mcde_dss_get_rotation(
	struct mcde_display_device *ddev)
{
	return ddev->get_rotation(ddev);
}

int mcde_dss_set_synchronized_update(struct mcde_display_device *ddev,
	bool enable)
{
	int ret;
	ret = ddev->set_synchronized_update(ddev, enable);
	if (ret)
		return ret;
	if (ddev->chnl_state)
		mcde_chnl_enable_synchronized_update(ddev->chnl_state, enable);
	return 0;
}

bool mcde_dss_get_synchronized_update(struct mcde_display_device *ddev)
{
	return ddev->get_synchronized_update(ddev);
}

int __init mcde_dss_init(void)
{
	return 0;
}

void mcde_dss_exit(void)
{
}

