/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * ST-Ericsson MCDE frame buffer driver
 *
 * Author: Marcus Lorentzon <marcus.xm.lorentzon@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>

#include <linux/hwmem.h>
#include <linux/io.h>

#include <video/mcde_fb.h>

#define MCDE_FB_BPP_MAX		16
#define MCDE_FB_VXRES_MAX	1920
#define MCDE_FB_VYRES_MAX	2160

static struct fb_ops fb_ops;

struct pix_fmt_info {
	enum mcde_ovly_pix_fmt pix_fmt;

	__u32              bpp;
	struct fb_bitfield r;
	struct fb_bitfield g;
	struct fb_bitfield b;
	struct fb_bitfield a;
	__u32              nonstd;
};

struct pix_fmt_info pix_fmt_map[] = {
	{
		.pix_fmt = MCDE_OVLYPIXFMT_RGB565,
		.bpp = 16,
		.r = { .offset = 11, .length = 5 },
		.g = { .offset =  5, .length = 6 },
		.b = { .offset =  0, .length = 5 },
	}, {
		.pix_fmt = MCDE_OVLYPIXFMT_RGBA5551,
		.bpp = 16,
		.r = { .offset = 11, .length = 5 },
		.g = { .offset =  6, .length = 5 },
		.b = { .offset =  1, .length = 5 },
		.a = { .offset =  0, .length = 1 },
	}, {
		.pix_fmt = MCDE_OVLYPIXFMT_RGBA4444,
		.bpp = 16,
		.r = { .offset = 12, .length = 4 },
		.g = { .offset =  8, .length = 4 },
		.b = { .offset =  4, .length = 4 },
		.a = { .offset =  0, .length = 4 },
	}, {
		.pix_fmt = MCDE_OVLYPIXFMT_YCbCr422,
		.bpp = 16,
		.nonstd = MCDE_OVLYPIXFMT_YCbCr422,
	}, {
		.pix_fmt = MCDE_OVLYPIXFMT_RGB888,
		.bpp = 24,
		.r = { .offset =  0, .length = 8 },
		.g = { .offset =  8, .length = 8 },
		.b = { .offset = 16, .length = 8 },
	}, {
		.pix_fmt = MCDE_OVLYPIXFMT_RGBX8888,
		.bpp = 32,
		.r = { .offset =  0, .length = 8 },
		.g = { .offset =  8, .length = 8 },
		.b = { .offset = 16, .length = 8 },
	}, {
		.pix_fmt = MCDE_OVLYPIXFMT_RGBA8888,
		.bpp = 32,
		.r = { .offset =  0, .length = 8 },
		.g = { .offset =  8, .length = 8 },
		.b = { .offset = 16, .length = 8 },
		.a = { .offset = 24, .length = 8 },
	}
};

static struct platform_device mcde_fb_device = {
	.name = "mcde_fb",
	.id = -1,
};

/* Helpers */

static struct pix_fmt_info *find_pix_fmt_info(enum mcde_ovly_pix_fmt pix_fmt)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(pix_fmt_map); i++) {
		if (pix_fmt_map[i].pix_fmt == pix_fmt)
			return &pix_fmt_map[i];
	}
	return NULL;
}

static bool bitfield_cmp(struct fb_bitfield *bf1, struct fb_bitfield *bf2)
{
	return bf1->offset == bf2->offset &&
		bf1->length == bf2->length &&
		bf1->msb_right == bf2->msb_right;
}

static struct pix_fmt_info *var_to_pix_fmt_info(struct fb_var_screeninfo *var)
{
	int i;
	struct pix_fmt_info *info;

	if (var->nonstd)
		return find_pix_fmt_info(var->nonstd);

	for (i = 0; i < ARRAY_SIZE(pix_fmt_map); i++) {
		info = &pix_fmt_map[i];
		if (info->bpp == var->bits_per_pixel &&
					bitfield_cmp(&info->r, &var->red) &&
					bitfield_cmp(&info->g, &var->green) &&
					bitfield_cmp(&info->b, &var->blue) &&
					bitfield_cmp(&info->a, &var->transp))
			return info;
	}

	for (i = 0; i < ARRAY_SIZE(pix_fmt_map); i++) {
		info = &pix_fmt_map[i];
		if (var->bits_per_pixel == info->bpp)
			return info;
	}

	return NULL;
}

static void pix_fmt_info_to_var(struct pix_fmt_info *pix_fmt_info,
	struct fb_var_screeninfo *var)
{
	var->bits_per_pixel = pix_fmt_info->bpp;
	var->nonstd = pix_fmt_info->nonstd;
	var->red = pix_fmt_info->r;
	var->green = pix_fmt_info->g;
	var->blue = pix_fmt_info->b;
	var->transp = pix_fmt_info->a;
}

static int init_var_fmt(struct fb_var_screeninfo *var,
	u16 w, u16 h, u16 vw, u16 vh, enum mcde_ovly_pix_fmt pix_fmt,
	u32 rotate)
{
	struct pix_fmt_info *info;

	info = find_pix_fmt_info(pix_fmt);
	if (!info)
		return -EINVAL;

	var->bits_per_pixel = info->bpp;
	var->nonstd         = info->nonstd;
	var->red            = info->r;
	var->green          = info->g;
	var->blue           = info->b;
	var->transp         = info->a;
	var->grayscale      = false;

	var->xres = w;
	var->yres = h;
	var->xres_virtual = vw;
	var->yres_virtual = vh;
	var->xoffset = 0;
	var->yoffset = 0;
	var->activate = FB_ACTIVATE_NOW;
	var->rotate = rotate;
	return 0;
};

static int reallocate_fb_mem(struct fb_info *fbi, u32 size)
{
	dma_addr_t paddr;
	void __iomem *vaddr;
#ifdef CONFIG_MCDE_FB_AVOID_REALLOC
	u32 size_max;
#endif

	size = PAGE_ALIGN(size);

	if (size == fbi->screen_size)
		return 0;

	/* TODO: hwmem */
#ifdef CONFIG_MCDE_FB_AVOID_REALLOC
	if (!fbi->screen_base) {
		struct mcde_fb *mfb = to_mcde_fb(fbi);
		struct hwmem_alloc *alloc;
		uint32_t phys_addr;
		int name;
		size_max = MCDE_FB_BPP_MAX / 8 * MCDE_FB_VXRES_MAX *
				MCDE_FB_VYRES_MAX;
		alloc = hwmem_alloc(size_max, HWMEM_ALLOC_BUFFERED,
					(HWMEM_ACCESS_READ  | HWMEM_ACCESS_WRITE | HWMEM_ACCESS_IMPORT),
					HWMEM_MEM_CONTIGUOUS_SYS);

		if (IS_ERR(alloc))
			return PTR_ERR(alloc);
		name = hwmem_get_name(alloc);
		if (name < 0) {
			hwmem_release(alloc);
			return name;
		}

		(void)hwmem_pin(alloc, &phys_addr, NULL);
		paddr = phys_addr;
		vaddr = ioremap(phys_addr, size_max);

		fbi->screen_base = vaddr;
		fbi->fix.smem_start = paddr;
		mfb->alloc = alloc;
		mfb->alloc_name = name;
	}
#else
	vaddr = dma_alloc_coherent(fbi->dev, size, &paddr, GFP_KERNEL|GFP_DMA);
	if (!vaddr)
		return -ENOMEM;
#endif

#ifndef CONFIG_MCDE_FB_AVOID_REALLOC
	if (fbi->screen_base)
		dma_free_coherent(fbi->dev, fbi->screen_size,
			fbi->screen_base, fbi->fix.smem_start);
#endif

#ifndef CONFIG_MCDE_FB_AVOID_REALLOC
	fbi->screen_base = vaddr;
	fbi->fix.smem_start = paddr;
#endif
	fbi->screen_size = size;
	fbi->fix.smem_len = size;

	return 0;
}

static void free_fb_mem(struct fb_info *fbi)
{
	if (fbi->fix.smem_start) {
		dma_free_coherent(fbi->dev, fbi->screen_size,
			fbi->screen_base, fbi->fix.smem_start);
		fbi->fix.smem_start = 0;
		fbi->fix.smem_len = 0;
		fbi->screen_base = 0;
		fbi->screen_size = 0;
	}
}

static void init_fb(struct fb_info *fbi)
{
	struct mcde_fb *mfb = to_mcde_fb(fbi);

	strlcpy(fbi->fix.id, "mcde_fb", sizeof(fbi->fix.id));
	fbi->fix.type = FB_TYPE_PACKED_PIXELS;
	fbi->fix.visual = FB_VISUAL_TRUECOLOR;
	fbi->fix.xpanstep = 1;
	fbi->fix.ypanstep = 1;
	fbi->flags = FBINFO_HWACCEL_DISABLED;
	fbi->fbops = &fb_ops;
	fbi->pseudo_palette = &mfb->pseudo_palette[0];
}

static void get_ovly_info(struct fb_info *fbi, struct mcde_overlay *ovly,
	struct mcde_overlay_info *info)
{
	struct mcde_fb *mfb = to_mcde_fb(fbi);
	memset(info, 0, sizeof(*info));
	info->paddr = fbi->fix.smem_start +
		fbi->fix.line_length * fbi->var.yoffset;
	/* TODO: move mem check to check_var/pan_display */
	if (info->paddr + fbi->fix.line_length * fbi->var.yres >
		fbi->fix.smem_start + fbi->fix.smem_len)
		info->paddr = fbi->fix.smem_start;
	info->fmt = mfb->pix_fmt;
	info->stride = fbi->fix.line_length;
	if (ovly) {
		info->src_x = ovly->info.src_x;
		info->src_y = ovly->info.src_y;
		info->dst_x = ovly->info.dst_x;
		info->dst_y = ovly->info.dst_y;
		info->dst_z = ovly->info.dst_z;
	} else {
		info->src_x = 0;
		info->src_y = 0;
		info->dst_x = 0;
		info->dst_y = 0;
		info->dst_z = 0;
	}
	info->w = fbi->var.xres;
	info->h = fbi->var.yres;
	info->dirty.x = 0;
	info->dirty.y = 0;
	info->dirty.w = fbi->var.xres;
	info->dirty.h = fbi->var.yres;
}

void vmode_to_var(struct mcde_video_mode *video_mode,
	struct fb_var_screeninfo *var)
{
	/* TODO: use only 1 vbp and 1 vfp */
	var->xres           = video_mode->xres;
	var->yres           = video_mode->yres;
	var->pixclock       = video_mode->pixclock;
	var->upper_margin   = video_mode->vbp1 + video_mode->vbp2;
	var->lower_margin   = video_mode->vfp1 + video_mode->vfp2;
	var->left_margin    = video_mode->hbp;
	var->right_margin   = video_mode->hfp;
	var->vmode         |= video_mode->interlaced ?
				FB_VMODE_INTERLACED : FB_VMODE_NONINTERLACED;
}

void var_to_vmode(struct fb_var_screeninfo *var,
	struct mcde_video_mode *video_mode)
{
	video_mode->xres       = var->xres;
	video_mode->yres       = var->yres;
	video_mode->pixclock   = var->pixclock;
	video_mode->vbp1       = var->upper_margin / 2;
	video_mode->vfp1       = var->lower_margin / 2;
	video_mode->vbp2       = video_mode->vbp1 + var->upper_margin % 2;
	video_mode->vfp2       = video_mode->vfp1 + var->lower_margin % 2;
	video_mode->hbp        = var->left_margin;
	video_mode->hfp        = var->right_margin;
	video_mode->interlaced = (var->vmode & FB_VMODE_INTERLACED) ==
							FB_VMODE_INTERLACED;
}

enum mcde_display_rotation var_to_rotation(struct fb_var_screeninfo *var)
{
	enum mcde_display_rotation rot;

	switch (var->rotate) {
	case FB_ROTATE_UR:
		rot = MCDE_DISPLAY_ROT_0;
		break;
	case FB_ROTATE_CW:
		rot = MCDE_DISPLAY_ROT_90_CW;
		break;
	case FB_ROTATE_UD:
		rot = MCDE_DISPLAY_ROT_180_CW;
		break;
	case FB_ROTATE_CCW:
		rot = MCDE_DISPLAY_ROT_90_CCW;
		break;
	default:
		rot = MCDE_DISPLAY_ROT_0;
		break;
	}
	dev_vdbg(&mcde_fb_device.dev, "var_rot: %d -> mcde_rot: %d\n",
							var->rotate, rot);
	return rot;
}

static struct mcde_display_device *fb_to_display(struct fb_info *fbi)
{
	int i;
	struct mcde_fb *mfb = to_mcde_fb(fbi);

	for (i = 0; i < mfb->num_ovlys; i++) {
		if (mfb->ovlys[i])
			return mfb->ovlys[i]->ddev;
	}
	return NULL;
}

static int check_var(struct fb_var_screeninfo *var, struct fb_info *fbi,
	struct mcde_display_device *ddev)
{
	int ret;
	u16 w = -1, h = -1;
	struct mcde_video_mode vmode;
	struct pix_fmt_info *fmtinfo;

	/* TODO: check sizes/offsets/memory validity */

	/* Device physical size */
	mcde_dss_get_physical_size(ddev, &w, &h);
	var->width  = w;
	var->height = h;

	/* Rotation */
	if (var->rotate > 3) {
		dev_info(&(ddev->dev), "check_var failed var->rotate\n");
		return -EINVAL;
	}

	/* Video mode */
	var_to_vmode(var, &vmode);
	ret = mcde_dss_try_video_mode(ddev, &vmode);
	if (ret < 0) {
		dev_vdbg(&(ddev->dev), "check_var failed "
			"mcde_dss_try_video_mode with size = %x\n", ret);
		return ret;
	}
	vmode_to_var(&vmode, var);

	/* Pixel format */
	fmtinfo = var_to_pix_fmt_info(var);
	if (!fmtinfo) {
		dev_vdbg(&(ddev->dev), "check_var failed fmtinfo\n");
		return -EINVAL;
	}
	pix_fmt_info_to_var(fmtinfo, var);

	/* Not used */
	var->grayscale = 0;
	var->sync = 0;

	return 0;
}

static int apply_var(struct fb_info *fbi, struct mcde_display_device *ddev)
{
	int ret, i;
	struct mcde_fb *mfb = to_mcde_fb(fbi);
	struct fb_var_screeninfo *var;
	struct mcde_video_mode vmode;
	struct pix_fmt_info *fmt;
	u32 line_len, size;

	dev_vdbg(&(ddev->dev), "%s\n", __func__);

	var = &fbi->var;

	/* Reallocate memory */
	line_len = (fbi->var.bits_per_pixel * var->xres_virtual) / 8;
	line_len = ALIGN(line_len, MCDE_BUF_LINE_ALIGMENT);
	size = line_len * var->yres_virtual;
	ret = reallocate_fb_mem(fbi, size);
	if (ret) {
		dev_vdbg(&(ddev->dev), "apply_var failed with"
				"reallocate mem with size = %d\n", size);
		return ret;
	}
	fbi->fix.line_length = line_len;

	if (ddev) {
		/* Apply pixel format */
		fmt = var_to_pix_fmt_info(var);
		mfb->pix_fmt = fmt->pix_fmt;
		mcde_dss_set_pixel_format(ddev, mfb->pix_fmt);

		/* Apply rotation */
		mcde_dss_set_rotation(ddev, var_to_rotation(var));
		/* Apply video mode */
		memset(&vmode, 0, sizeof(struct mcde_video_mode));
		var_to_vmode(var, &vmode);
		mcde_dss_set_video_mode(ddev, &vmode);
		mcde_dss_apply_channel(ddev);
	}

	/* Apply overlay info */
	for (i = 0; i < mfb->num_ovlys; i++) {
		struct mcde_overlay *ovly = mfb->ovlys[i];
		struct mcde_overlay_info info;

		get_ovly_info(fbi, ovly, &info);
		(void) mcde_dss_apply_overlay(ovly, &info);
		ret = mcde_dss_update_overlay(ovly);
	}

	return 0;
}

/* FB ops */

static int mcde_fb_open(struct fb_info *fbi, int user)
{
	dev_vdbg(fbi->dev, "%s\n", __func__);
	return 0;
}

static int mcde_fb_release(struct fb_info *fbi, int user)
{
	dev_vdbg(fbi->dev, "%s\n", __func__);
	return 0;
}

static int mcde_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *fbi)
{
	struct mcde_display_device *ddev = fb_to_display(fbi);

	dev_vdbg(fbi->dev, "%s\n", __func__);

	if (!ddev) {
		printk(KERN_ERR "mcde_fb_check_var failed !ddev\n");
		return -ENODEV;
	}

	return check_var(var, fbi, ddev);
}

static int mcde_fb_set_par(struct fb_info *fbi)
{
	dev_vdbg(fbi->dev, "%s\n", __func__);

	return apply_var(fbi, fb_to_display(fbi));
}

static int mcde_fb_blank(int blank, struct fb_info *fbi)
{
	dev_vdbg(fbi->dev, "%s\n", __func__);
	/* REVIEW: is this supposed to be empty? */
	return 0;
}

static int mcde_fb_pan_display(struct fb_var_screeninfo *var,
	struct fb_info *fbi)
{
	dev_vdbg(fbi->dev, "%s\n", __func__);

	if (var->xoffset == fbi->var.xoffset &&
					var->yoffset == fbi->var.yoffset)
		return 0;

	fbi->var.xoffset = var->xoffset;
	fbi->var.yoffset = var->yoffset;
	return apply_var(fbi, fb_to_display(fbi));
}

static void mcde_fb_rotate(struct fb_info *fbi, int rotate)
{
	dev_vdbg(fbi->dev, "%s\n", __func__);
}

static int mcde_fb_ioctl(struct fb_info *fbi, unsigned int cmd,
							 unsigned long arg)
{
	struct mcde_fb *mfb = to_mcde_fb(fbi);

	if (cmd == MCDE_GET_BUFFER_NAME_IOC)
		return mfb->alloc_name;

	return -EINVAL;
}

static struct fb_ops fb_ops = {
	/* creg, cmap */
	.owner          = THIS_MODULE,
	.fb_open        = mcde_fb_open,
	.fb_release     = mcde_fb_release,
	.fb_read        = fb_sys_read,
	.fb_write       = fb_sys_write,
	.fb_fillrect    = sys_fillrect,
	.fb_copyarea    = sys_copyarea,
	.fb_imageblit   = sys_imageblit,
	.fb_check_var   = mcde_fb_check_var,
	.fb_set_par     = mcde_fb_set_par,
	.fb_blank       = mcde_fb_blank,
	.fb_pan_display = mcde_fb_pan_display,
	.fb_rotate      = mcde_fb_rotate,
	.fb_ioctl       = mcde_fb_ioctl,
};

/* FB driver */

struct fb_info *mcde_fb_create(struct mcde_display_device *ddev,
	u16 w, u16 h, u16 vw, u16 vh, enum mcde_ovly_pix_fmt pix_fmt,
	u32 rotate)
{
	int ret = 0;
	struct fb_info *fbi;
	struct mcde_fb *mfb;
	struct mcde_overlay *ovly = NULL;
	struct mcde_overlay_info ovly_info;

	dev_vdbg(&ddev->dev, "%s\n", __func__);
	if (!ddev->initialized) {
		dev_warn(&ddev->dev, "%s: Device not initialized\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	/* Init fb */
	fbi = framebuffer_alloc(sizeof(struct mcde_fb), &mcde_fb_device.dev);
	if (fbi == NULL) {
		ret = -ENOMEM;
		goto fb_alloc_failed;
	}
	init_fb(fbi);
	mfb = to_mcde_fb(fbi);

	ret = mcde_dss_enable_display(ddev);
	if (ret)
		goto display_enable_failed;

	/* Prepare var and allocate frame buffer memory */
	init_var_fmt(&fbi->var, w, h, vw, vh, pix_fmt, rotate);
	check_var(&fbi->var, fbi, ddev);
	ret = apply_var(fbi, ddev);
	if (ret)
		goto apply_var_failed;

	/* Setup overlay */
	get_ovly_info(fbi, NULL, &ovly_info);
	ovly = mcde_dss_create_overlay(ddev, &ovly_info);
	if (!ovly) {
		ret = PTR_ERR(ovly);
		goto ovly_alloc_failed;
	}
	mfb->ovlys[0] = ovly;
	mfb->num_ovlys = 1;

	ret = mcde_dss_enable_overlay(ovly);
	if (ret)
		goto ovly_enable_failed;

	mfb->id = ddev->id;

	/* Register framebuffer */
	ret = register_framebuffer(fbi);
	if (ret)
		goto fb_register_failed;

	goto out;
fb_register_failed:
	mcde_dss_disable_overlay(ovly);
ovly_enable_failed:
	mcde_dss_destroy_overlay(ovly);
ovly_alloc_failed:
	free_fb_mem(fbi);
apply_var_failed:
	mcde_dss_disable_display(ddev);
display_enable_failed:
	framebuffer_release(fbi);
	fbi = NULL;
fb_alloc_failed:
out:
	return ret ? ERR_PTR(ret) : fbi;
}
EXPORT_SYMBOL(mcde_fb_create);

int mcde_fb_attach_overlay(struct fb_info *fb_info, struct mcde_overlay *ovl)
{
	/* TODO: Attach extra overlay targets */
	return -EINVAL;
}

void mcde_fb_destroy(struct fb_info *fb_info)
{
	/* TODO: clean up */
}

/* Overlay fbs' platform device */
static int mcde_fb_probe(struct platform_device *pdev)
{
	return 0;
}

static int mcde_fb_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver mcde_fb_driver = {
	.probe  = mcde_fb_probe,
	.remove = mcde_fb_remove,
	.driver = {
		.name  = "mcde_fb",
		.owner = THIS_MODULE,
	},
};

/* MCDE fb init */

int __init mcde_fb_init(void)
{
	int ret;

	ret = platform_driver_register(&mcde_fb_driver);
	if (ret)
		goto fb_driver_failed;
	ret = platform_device_register(&mcde_fb_device);
	if (ret)
		goto fb_device_failed;

	goto out;
fb_device_failed:
	platform_driver_unregister(&mcde_fb_driver);
fb_driver_failed:
out:
	return ret;
}

void mcde_fb_exit(void)
{
	platform_device_unregister(&mcde_fb_device);
	platform_driver_unregister(&mcde_fb_driver);
}

