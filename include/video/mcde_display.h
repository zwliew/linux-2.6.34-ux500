/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * ST-Ericsson MCDE display driver
 *
 * Author: Marcus Lorentzon <marcus.xm.lorentzon@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */
#ifndef __MCDE_DISPLAY__H__
#define __MCDE_DISPLAY__H__

#include <linux/device.h>
#include <linux/pm.h>

#include <video/mcde.h>

#define UPDATE_FLAG_PIXEL_FORMAT	0x1
#define UPDATE_FLAG_VIDEO_MODE		0x2
#define UPDATE_FLAG_ROTATION		0x4

#define to_mcde_display_device(__dev) \
	container_of((__dev), struct mcde_display_device, dev)

struct mcde_display_device {
	/* MCDE driver static */
	struct device     dev;
	const char       *name;
	int               id;
	struct mcde_port *port;

	/* MCDE dss driver internal */
	bool initialized;
	enum mcde_chnl chnl_id;
	enum mcde_fifo fifo;
	bool first_update;

	bool enabled;
	struct mcde_chnl_state *chnl_state;
	struct list_head ovlys;
	struct mcde_rectangle update_area;
	/* TODO: Remove once ESRAM allocator is done */
	u32 rotbuf1;
	u32 rotbuf2;

	/* Display driver internal */
	u16 native_x_res;
	u16 native_y_res;
	u16 physical_width;
	u16 physical_height;
	enum mcde_display_power_mode power_mode;
	enum mcde_ovly_pix_fmt default_pixel_format;
	enum mcde_ovly_pix_fmt pixel_format;
	enum mcde_display_rotation rotation;
	bool synchronized_update;
	struct mcde_video_mode video_mode;
	int update_flags;

	/* Driver API */
	void (*get_native_resolution)(struct mcde_display_device *dev,
		u16 *x_res, u16 *y_res);
	enum mcde_ovly_pix_fmt (*get_default_pixel_format)(
		struct mcde_display_device *dev);
	void (*get_physical_size)(struct mcde_display_device *dev,
		u16 *x_size, u16 *y_size);

	int (*set_power_mode)(struct mcde_display_device *dev,
		enum mcde_display_power_mode power_mode);
	enum mcde_display_power_mode (*get_power_mode)(
		struct mcde_display_device *dev);

	int (*try_video_mode)(struct mcde_display_device *dev,
		struct mcde_video_mode *video_mode);
	int (*set_video_mode)(struct mcde_display_device *dev,
		struct mcde_video_mode *video_mode);
	void (*get_video_mode)(struct mcde_display_device *dev,
		struct mcde_video_mode *video_mode);

	int (*set_pixel_format)(struct mcde_display_device *dev,
		enum mcde_ovly_pix_fmt pix_fmt);
	enum mcde_ovly_pix_fmt (*get_pixel_format)(
		struct mcde_display_device *dev);
	enum mcde_port_pix_fmt (*get_port_pixel_format)(
		struct mcde_display_device *dev);

	int (*set_rotation)(struct mcde_display_device *dev,
		enum mcde_display_rotation rotation);
	enum mcde_display_rotation (*get_rotation)(
		struct mcde_display_device *dev);

	int (*set_synchronized_update)(struct mcde_display_device *dev,
		bool enable);
	bool (*get_synchronized_update)(struct mcde_display_device *dev);

	int (*apply_config)(struct mcde_display_device *dev);
	int (*invalidate_area)(struct mcde_display_device *dev,
						struct mcde_rectangle *area);
	int (*update)(struct mcde_display_device *dev);
	int (*prepare_for_update)(struct mcde_display_device *dev,
		u16 x, u16 y, u16 w, u16 h);
	int (*on_first_update)(struct mcde_display_device *dev);
	int (*platform_enable)(struct mcde_display_device *dev);
	int (*platform_disable)(struct mcde_display_device *dev);
};

struct mcde_display_driver {
	int (*probe)(struct mcde_display_device *dev);
	int (*remove)(struct mcde_display_device *dev);
	void (*shutdown)(struct mcde_display_device *dev);
	int (*suspend)(struct mcde_display_device *dev,
		pm_message_t state);
	int (*resume)(struct mcde_display_device *dev);

	struct device_driver driver;
};

/* MCDE dsi (Used by MCDE display drivers) */

int mcde_display_dsi_dcs_write(struct mcde_display_device *dev,
	u8 cmd, u8 *data, int len);
int mcde_display_dsi_dcs_read(struct mcde_display_device *dev,
	u8 cmd, u8 *data, int *len);
int mcde_display_dsi_bta_sync(struct mcde_display_device *dev);

/* MCDE display bus */

int mcde_display_driver_register(struct mcde_display_driver *drv);
void mcde_display_driver_unregister(struct mcde_display_driver *drv);
int mcde_display_device_register(struct mcde_display_device *dev);
void mcde_display_device_unregister(struct mcde_display_device *dev);

void mcde_display_init_device(struct mcde_display_device *dev);

int mcde_display_init(void);
void mcde_display_exit(void);

#endif /* __MCDE_DISPLAY__H__ */

