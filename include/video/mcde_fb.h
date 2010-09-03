/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * ST-Ericsson MCDE display sub system frame buffer driver
 *
 * Author: Marcus Lorentzon <marcus.xm.lorentzon@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */
#ifndef __MCDE_FB__H__
#define __MCDE_FB__H__

#include <linux/fb.h>

#include "mcde_dss.h"

#define to_mcde_fb(x) ((struct mcde_fb *)(x)->par)

#define MCDE_FB_MAX_NUM_OVERLAYS 3

struct mcde_fb {
	int num_ovlys;
	struct mcde_overlay *ovlys[MCDE_FB_MAX_NUM_OVERLAYS];
	u32 pseudo_palette[17];
	enum mcde_ovly_pix_fmt pix_fmt;
};

/* MCDE fbdev API */
struct fb_info *mcde_fb_create(struct mcde_display_device *ddev,
	u16 w, u16 h, u16 vw, u16 vh, enum mcde_ovly_pix_fmt pix_fmt,
	u32 rotate, bool display_initialized);
int mcde_fb_attach_overlay(struct fb_info *fb_info,
	struct mcde_overlay *ovl);
void mcde_fb_destroy(struct fb_info *fb_info);

/* MCDE fb driver */
int mcde_fb_init(void);
void mcde_fb_exit(void);

#endif /* __MCDE_FB__H__ */

