/*
 * Copyright (C) ST-Ericsson AB 2009
 *
 * Author: Fredrik Allansson <fredrik.allansson@stericsson.com> for ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/kernel.h>
#include <linux/debugfs.h>

#include "b2r2_node_split.h"
#include "b2r2_internal.h"
#include "b2r2_hw.h"

/******************
 * Debug printing
 ******************/
static u32 debug;
static u32 verbose;

#define pdebug(...) \
	do { \
		if (debug) \
			printk(__VA_ARGS__); \
	} while (false)

#define pverbose(...) \
	do { \
		if (verbose) \
			printk(__VA_ARGS__); \
	} while (false)

#define PTRACE_ENTRY() pdebug(KERN_INFO LOG_TAG "::%s\n", __func__)

#define LOG_TAG "b2r2_node_split"


/************************
 * Macros and constants
 ************************/
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define INSTANCES_DEFAULT_SIZE 10
#define INSTANCES_GROW_SIZE 5

/******************
 * Internal types
 ******************/


/********************
 * Global variables
 ********************/

/**
 * VMX values for different color space conversions
 */
static const u32 vmx_rgb_to_yuv[] = {
	B2R2_VMX0_RGB_TO_YUV_601_VIDEO,
	B2R2_VMX1_RGB_TO_YUV_601_VIDEO,
	B2R2_VMX2_RGB_TO_YUV_601_VIDEO,
	B2R2_VMX3_RGB_TO_YUV_601_VIDEO,
};

static const u32 vmx_yuv_to_rgb[] = {
	B2R2_VMX0_YUV_TO_RGB_601_VIDEO,
	B2R2_VMX1_YUV_TO_RGB_601_VIDEO,
	B2R2_VMX2_YUV_TO_RGB_601_VIDEO,
	B2R2_VMX3_YUV_TO_RGB_601_VIDEO,
};

static const u32 vmx_rgb_to_bgr[] = {
	B2R2_VMX0_RGB_TO_BGR,
	B2R2_VMX1_RGB_TO_BGR,
	B2R2_VMX2_RGB_TO_BGR,
	B2R2_VMX3_RGB_TO_BGR,
};

/* TODO: Find out iVMX values for BGR <--> YUV */
static const u32 vmx_bgr_to_yuv[] = {
	0,
	0,
	0,
	0,
};

static const u32 vmx_yuv_to_bgr[] = {
	0,
	0,
	0,
	0,
};

/********************************************
 * Forward declaration of private functions
 ********************************************/

static int analyze_fmt_conv(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count);
static int analyze_color_fill(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count);
static int analyze_copy(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count,
		u32 *buf_count);
static int analyze_scaling(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count,
		u32 *buf_count);
static int analyze_rotation(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count,
		u32 *buf_count);
static int analyze_transform(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count,
		u32 *buf_count);
static int analyze_rot_scale(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count,
		u32 *buf_count);
static int analyze_scale_factors(struct b2r2_node_split_job *this);

static int calculate_rot_scale_node_count(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req);
static void setup_rot_scale_windows(struct b2r2_node_split_job *this);

static void configure_src(struct b2r2_node *node,
		struct b2r2_node_split_buf *src, const u32 *ivmx);
static void configure_dst(struct b2r2_node *node,
		struct b2r2_node_split_buf *dst, const u32 *ivmx,
		struct b2r2_node **next);
static void configure_blend(struct b2r2_node *node, u32 flags, u32 global_alpha,
		bool swap_fg_bg);
static void configure_clip(struct b2r2_node *node,
		struct b2r2_blt_rect *clip_rect);

static int configure_tile(struct b2r2_node_split_job *this,
		struct b2r2_node *node, struct b2r2_node **next);
static void configure_direct_fill(struct b2r2_node *node, u32 color,
		struct b2r2_node_split_buf *dst, struct b2r2_node **next);
static void configure_fill(struct b2r2_node *node, u32 color,
		enum b2r2_blt_fmt fmt, struct b2r2_node_split_buf *dst,
		const u32 *ivmx, struct b2r2_node **next);
static void configure_direct_copy(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, struct b2r2_node **next);
static void configure_copy(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, const u32 *ivmx,
		struct b2r2_node **next);
static void configure_rotate(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, const u32 *ivmx,
		struct b2r2_node **next);
static void configure_scale(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, u16 h_rsf, u16 v_rsf,
		const u32 *ivmx, struct b2r2_node **next);
static void configure_tmp_buf(struct b2r2_node_split_buf *this, int index,
		struct b2r2_blt_rect *window);
static void configure_rot_scale(struct b2r2_node_split_job *this,
		struct b2r2_node *node, struct b2r2_node **next);

static void set_buf(struct b2r2_node_split_buf *buf, u32 addr,
		const struct b2r2_blt_img *img,
		const struct b2r2_blt_rect *rect, bool color_fill, u32 color);
static void constrain_window(struct b2r2_blt_rect *window,
		enum b2r2_blt_fmt fmt, u32 max_size);
static void setup_tmp_bufs(struct b2r2_node_split_job *this);
static int calculate_tile_count(s32 area_width, s32 area_height, s32 tile_width,
		s32 tile_height);

static inline enum b2r2_ty get_alpha_range(enum b2r2_blt_fmt fmt);
static inline u8 get_alpha(enum b2r2_blt_fmt fmt, u32 pixel);
static inline bool fmt_has_alpha(enum b2r2_blt_fmt fmt);
static inline bool is_rgb_fmt(enum b2r2_blt_fmt fmt);
static inline bool is_bgr_fmt(enum b2r2_blt_fmt fmt);
static inline bool is_yuv_fmt(enum b2r2_blt_fmt fmt);
static inline int fmt_byte_pitch(enum b2r2_blt_fmt fmt, u32 width);
static inline enum b2r2_native_fmt to_native_fmt(enum b2r2_blt_fmt fmt);
static inline enum b2r2_fmt_type get_fmt_type(enum b2r2_blt_fmt fmt);
static inline char *type2str(enum b2r2_op_type type);

static inline bool is_transform(const struct b2r2_blt_request *req);
static inline int calculate_scale_factor(u32 from, u32 to, u16 *sf_out);
static inline s32 rescale(s32 dim, u16 sf);
static inline s32 inv_rescale(s32 dim, u16 sf);
static inline bool is_upscale(u16 h_rsf, u16 v_rsf);
static inline u16 calc_global_alpha(enum b2r2_blt_fmt fmt, u32 color,
		u16 global_alpha);

static void set_target(struct b2r2_node *node, u32 addr,
		struct b2r2_node_split_buf *buf);
static void set_src(struct b2r2_src_config *src, u32 addr,
		struct b2r2_node_split_buf *buf);
static void set_src_1(struct b2r2_node *node, u32 addr,
		struct b2r2_node_split_buf *buf);
static void set_src_2(struct b2r2_node *node, u32 addr,
		struct b2r2_node_split_buf *buf);
static void set_src_3(struct b2r2_node *node, u32 addr,
		struct b2r2_node_split_buf *buf);
static void set_ivmx(struct b2r2_node *node, const u32 *vmx_values);

static void reset_nodes(struct b2r2_node *node);
static void dump_nodes(struct b2r2_node *first);

/********************
 * Public functions
 ********************/

/**
 * b2r2_node_split_analyze() - analyzes the request
 */
int b2r2_node_split_analyze(const struct b2r2_blt_request *req,
		u32 max_buf_size, u32 *node_count, struct b2r2_work_buf **bufs,
		u32 *buf_count, struct b2r2_node_split_job *this)
{
	int ret;
	bool color_fill;

	PTRACE_ENTRY();

	memset(this, 0, sizeof(*this));

	/* Copy parameters */
	this->flags = req->user_req.flags;
	this->transform = req->user_req.transform;
	this->max_buf_size = max_buf_size;
	this->global_alpha = req->user_req.global_alpha;
	this->buf_count = 0;
	this->node_count = 0;

	/* Check for color fill */
	color_fill = (this->flags & (B2R2_BLT_FLAG_SOURCE_FILL |
				B2R2_BLT_FLAG_SOURCE_FILL_RAW)) != 0;

	/* Check for blending */
	this->blend = ((this->flags &
				(B2R2_BLT_FLAG_PER_PIXEL_ALPHA_BLEND |
				B2R2_BLT_FLAG_GLOBAL_ALPHA_BLEND)) != 0);

	/* Check for clipping */
	this->clip = (this->flags &
				B2R2_BLT_FLAG_DESTINATION_CLIP) != 0;
	if (this->clip)
		memcpy(&this->clip_rect, &req->user_req.dst_clip_rect,
				sizeof(this->clip_rect));

	/* Configure the source and destination buffers */
	set_buf(&this->src, req->src_resolved.physical_address,
		&req->user_req.src_img, &req->user_req.src_rect, color_fill,
		req->user_req.src_color);
	set_buf(&this->dst, req->dst_resolved.physical_address,
		&req->user_req.dst_img, &req->user_req.dst_rect, false, 0);

	/* Do the analysis depending on the type of operation */
	if (color_fill) {
		ret = analyze_color_fill(this, req, &this->node_count);
	} else {

		if (is_transform(req)) {
			ret = analyze_transform(this, req, &this->node_count,
						&this->buf_count);
		} else {
			ret = analyze_copy(this, req, &this->node_count,
						&this->buf_count);
		}
	}

	if (ret < 0) {
		printk(KERN_ERR "%s: Analysis failed!\n", __func__);
		goto error;
	}

	*buf_count = this->buf_count;
	*node_count = this->node_count;

	if (this->buf_count > 0)
		*bufs = &this->work_bufs[0];

	pdebug(KERN_INFO "%s: src.rect=(%d, %d, %d, %d) d_src=(%d, %d), "
			"dst.rect(%d, %d, %d, %d) d_dst=(%d, %d)\n", __func__,
			this->src.rect.x, this->src.rect.y,
			this->src.rect.width, this->src.rect.height,
			this->src.dx, this->src.dy,
			this->dst.rect.x, this->dst.rect.y,
			this->dst.rect.width, this->dst.rect.height,
			this->dst.dx, this->dst.dy);
	pdebug(KERN_INFO "%s: src.window=(%d, %d, %d, %d) d_src=(%d, %d), "
			"dst.window(%d, %d, %d, %d) d_dst=(%d, %d)\n", __func__,
			this->src.window.x, this->src.window.y,
			this->src.window.width, this->src.window.height,
			this->src.dx, this->src.dy,
			this->dst.window.x, this->dst.window.y,
			this->dst.window.width, this->dst.window.height,
			this->dst.dx, this->dst.dy);

	pdebug(KERN_INFO "%s: node_count=%d, buf_count=%d, type=%s\n", __func__,
			*node_count, *buf_count, type2str(this->type));
	pdebug(KERN_INFO "%s: blend=%d, clip=%d, color_fill=%d\n", __func__,
			this->blend, this->clip, color_fill);

	return 0;

error:
	return ret;
}

/**
 * b2r2_node_split_configure() - configures the node list
 */
int b2r2_node_split_configure(struct b2r2_node_split_job *this,
		struct b2r2_node *first)
{
 int ret;
	struct b2r2_node *node = first;
	struct b2r2_node *last = first;

	bool last_row = false;
	bool last_col = false;

	s32 src_width;
	s32 dst_width;
	s32 dst_height;

	reset_nodes(node);

	pdebug(KERN_INFO "%s: src_rect=(%d, %d, %d, %d), "
			"dst_rect=(%d, %d, %d, %d)\n", __func__,
			this->src.rect.x, this->src.rect.y,
			this->src.rect.width, this->src.rect.height,
			this->dst.rect.x, this->dst.rect.y,
			this->dst.rect.width,
			this->dst.rect.height);
	pdebug(KERN_INFO "%s: d_src=(%d, %d) d_dst=(%d, %d)\n",
		__func__, this->src.dx, this->src.dy, this->dst.dx,
		this->dst.dy);

	/* Save these so we can do restore them when moving to a new row */
	src_width = this->src.window.width;
	dst_width = this->dst.window.width;
	dst_height = this->dst.window.height;

	reset_nodes(node);

	do {
		if (node == NULL) {
			/* DOH! This is an internal error (to few nodes
			   allocated) */
			printk(KERN_ERR "%s: Internal error! Out of nodes!\n",
				__func__);
			ret = -ENOMEM;
			goto error;
		}

		last_col = ABS(this->src.window.x + this->src.dx -
					this->src.rect.x) >=
				this->src.rect.width;
		last_row = ABS(this->src.window.y + this->src.dy -
					this->src.rect.y) >=
				this->src.rect.height;

		if (last_col) {
			pdebug(KERN_INFO "%s: last col\n", __func__);
			this->src.window.width = this->src.rect.width -
				ABS(this->src.rect.x - this->src.window.x);
			if (this->rotation)
				this->dst.window.height =
					this->dst.rect.height -
					ABS(this->dst.window.y -
						this->dst.rect.y);
			else
				this->dst.window.width =
					this->dst.rect.width -
					ABS(this->dst.window.x -
						this->dst.rect.x);
		}

		if (last_row) {
			pdebug(KERN_INFO "%s: last row\n", __func__);
			this->src.window.height = this->src.rect.height -
				ABS(this->src.rect.y - this->src.window.y);
			if (this->rotation)
				this->dst.window.width =
					this->dst.rect.width -
					ABS(this->dst.rect.x -
						this->dst.window.x);
			else
				this->dst.window.height =
					this->dst.rect.height -
					ABS(this->dst.rect.y -
						this->dst.window.y);
		}

		pdebug(KERN_INFO "%s: src=(%d, %d, %d, %d), "
				"dst=(%d, %d, %d, %d)\n", __func__,
				this->src.window.x, this->src.window.y,
				this->src.window.width, this->src.window.height,
				this->dst.window.x, this->dst.window.y,
				this->dst.window.width,
				this->dst.window.height);

		configure_tile(this, node, &last);

		this->src.window.x += this->src.dx;
		if (this->rotation)
			this->dst.window.y += this->dst.dy;
		else
			this->dst.window.x += this->dst.dx;

		if (last_col) {
			/* Reset the window and slide vertically */
			this->src.window.x = this->src.rect.x;
			this->src.window.width = src_width;

			this->src.window.y += this->src.dy;

			if (this->rotation) {
				this->dst.window.y = this->dst.rect.y;
				this->dst.window.height = dst_height;

				this->dst.window.x += this->dst.dx;
			} else {
				this->dst.window.x = this->dst.rect.x;
				this->dst.window.width = dst_width;

				this->dst.window.y += this->dst.dy;
			}
		}

		node = last;

	} while (!(last_row && last_col));

	return 0;
error:
	return ret;
}

/**
 * b2r2_node_split_assign_buffers() - assigns temporary buffers to the node list
 */
int b2r2_node_split_assign_buffers(struct b2r2_node_split_job *this,
		struct b2r2_node *first, struct b2r2_work_buf *bufs,
		u32 buf_count)
{
	struct b2r2_node *node = first;

	while (node != NULL) {
		/* The indices are offset by one */
		if (node->dst_tmp_index) {
			BUG_ON(node->dst_tmp_index > buf_count);

			node->node.GROUP1.B2R2_TBA =
				bufs[node->dst_tmp_index - 1].phys_addr;
		}
		if (node->src_tmp_index) {
			u32 addr = bufs[node->src_tmp_index - 1].phys_addr;

			BUG_ON(node->src_tmp_index > buf_count);

			switch (node->src_index) {
			case 1:
				node->node.GROUP3.B2R2_SBA = addr;
				break;
			case 2:
				node->node.GROUP4.B2R2_SBA = addr;
				break;
			case 3:
				node->node.GROUP5.B2R2_SBA = addr;
				break;
			default:
				BUG_ON(1);
				break;
			}
		}

		node = node->next;
	}

	return 0;
}

/**
 * b2r2_node_split_unassign_buffers() - releases temporary buffers
 */
void b2r2_node_split_unassign_buffers(struct b2r2_node_split_job *this,
		struct b2r2_node *first)
{
	return;
}

/**
 * b2r2_node_split_cancel() - cancels and releases a job instance
 */
void b2r2_node_split_cancel(struct b2r2_node_split_job *this)
{
	pdebug("%s: Releasing job\n", __func__);

	memset(this, 0, sizeof(*this));

	return;
}

/**********************
 * Private functions
 **********************/

/**
 * analyze_fmt_conv() - analyze the format conversions needed for a job
 */
static int analyze_fmt_conv(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count)
{
	pdebug(KERN_INFO LOG_TAG "::%s: Analyzing fmt conv...\n", __func__);

	if (is_rgb_fmt(this->src.fmt)) {
		if (is_yuv_fmt(this->dst.fmt))
			this->ivmx = &vmx_rgb_to_yuv[0];
		else if (is_bgr_fmt(this->dst.fmt))
			this->ivmx = &vmx_rgb_to_bgr[0];
	} else if (is_yuv_fmt(this->src.fmt)) {
		if (is_rgb_fmt(this->dst.fmt))
			this->ivmx = &vmx_yuv_to_rgb[0];
		else if (is_bgr_fmt(this->dst.fmt))
			this->ivmx = &vmx_yuv_to_bgr[0];
	} else if (is_bgr_fmt(this->src.fmt)) {
		if (is_rgb_fmt(this->dst.fmt))
			this->ivmx = &vmx_rgb_to_bgr[0];
		else if (is_yuv_fmt(this->dst.fmt))
			this->ivmx = &vmx_bgr_to_yuv[0];
	}

	if (this->dst.type == B2R2_FMT_TYPE_RASTER) {
		pdebug(KERN_INFO LOG_TAG "::%s: --> RASTER\n", __func__);
		*node_count = 1;
	} else if (this->dst.type == B2R2_FMT_TYPE_SEMI_PLANAR) {
		pdebug(KERN_INFO LOG_TAG "::%s: --> SEMI PLANAR\n", __func__);
		*node_count = 2;
	} else if (this->dst.type == B2R2_FMT_TYPE_PLANAR) {
		pdebug(KERN_INFO LOG_TAG "::%s: --> PLANAR\n", __func__);
		*node_count = 3;
	} else {
		/* That's strange... */
		BUG_ON(1);
	}

	return 0;
}

/**
 * analyze_color_fill() - analyze a color fill operation
 */
static int analyze_color_fill(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count)
{
	int ret;
	pdebug(KERN_INFO LOG_TAG "::%s: Analyzing color fill...\n", __func__);

	/* Destination must be raster for raw fill to work */
	if ((this->flags & B2R2_BLT_FLAG_SOURCE_FILL_RAW) &&
			(this->dst.type != B2R2_FMT_TYPE_RASTER)) {
		printk(KERN_ERR "%s: Raw fill requires raster destination\n",
			__func__);
		ret = -EINVAL;
		goto error;
	}

	if ((!this->blend) && ((this->flags & B2R2_BLT_FLAG_SOURCE_FILL_RAW) ||
			(this->dst.fmt == B2R2_BLT_FMT_32_BIT_ARGB8888) ||
			(this->dst.fmt == B2R2_BLT_FMT_32_BIT_ABGR8888) ||
			(this->dst.fmt == B2R2_BLT_FMT_32_BIT_AYUV8888))) {
		pdebug(KERN_INFO LOG_TAG "::%s: Direct fill!\n", __func__);
		this->type = B2R2_DIRECT_FILL;

		/* The color format will be the same as the dst fmt */
		this->src.fmt = this->dst.fmt;

		/* The entire destination rectangle will be  */
		memcpy(&this->dst.window, &this->dst.rect,
			sizeof(this->dst.window));
		*node_count = 1;
	} else {
		pdebug(KERN_INFO LOG_TAG "::%s: Normal fill!\n", __func__);
		this->type = B2R2_FILL;

		/* Determine the fill color format */
		if (this->flags & B2R2_BLT_FLAG_SOURCE_FILL_RAW) {
			/* The color format will be the same as the dst fmt */
			this->src.fmt = this->dst.fmt;
		} else {
			/* If the dst fmt is YUV the fill fmt will be as well */
			if (is_yuv_fmt(this->dst.fmt)) {
				this->src.fmt =	B2R2_BLT_FMT_32_BIT_AYUV8888;
			} else if (is_rgb_fmt(this->dst.fmt)) {
				this->src.fmt =	B2R2_BLT_FMT_32_BIT_ARGB8888;
			} else if (is_bgr_fmt(this->dst.fmt)) {
				this->src.fmt =	B2R2_BLT_FMT_32_BIT_ABGR8888;
			} else {
				/* Wait, what? */
				printk(KERN_ERR "%s: "
					"Illegal destination format for fill",
					__func__);
				ret = -EINVAL;
				goto error;
			}
		}

		/* If the destination is a multibuffer format, the operation
		   must be treated like a scale operation (multiple nodes).
		   Hence the need for src and dst windows */
		if (this->dst.type != B2R2_FMT_TYPE_RASTER) {
			memcpy(&this->dst.window, &this->dst.rect,
					sizeof(this->dst.window));
			this->dst.window.width = MIN(this->dst.window.width,
					B2R2_RESCALE_MAX_WIDTH);

			/* We need a rectangle and a window for the source */
			memcpy(&this->src.rect, &this->dst.rect,
			       sizeof(this->src.rect));
			memcpy(&this->src.window, &this->dst.window,
			       sizeof(this->src.window));
		}

		/* We will need to swap the FG and the BG since color fill from
		   S2 doesn't work */
		if (this->blend)
			this->swap_fg_bg = true;

		/* Also, B2R2 seems to ignore the pixel alpha value */
		if (((this->flags & B2R2_BLT_FLAG_PER_PIXEL_ALPHA_BLEND)
					!= 0) &&
				((this->flags & B2R2_BLT_FLAG_SOURCE_FILL_RAW)
				 == 0) && fmt_has_alpha(this->src.fmt)) {
			u8 pixel_alpha = get_alpha(this->src.fmt,
							this->dst.color);
			u32 new_global = pixel_alpha * this->global_alpha / 255;

			this->global_alpha = (u8)new_global;
		}

		ret = analyze_fmt_conv(this, req, node_count);
		if (ret < 0)
			goto error;
	}

	memcpy(&this->dst.window, &this->dst.rect, sizeof(this->dst.window));

	this->dst.dx = this->dst.window.width;
	this->dst.dy = this->dst.window.height;

	return 0;

error:
	return ret;

}

/**
 * analyze_transform() - analyze a transform operation (rescale, rotate, etc.)
 */
static int analyze_transform(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count,
		u32 *buf_count)
{
	int ret;

	bool is_scaling;
	this->rotation = (this->transform & B2R2_BLT_TRANSFORM_CCW_ROT_90)
				!= 0;

	pdebug(KERN_INFO LOG_TAG "::%s: Analyzing transform...\n", __func__);

	if (this->rotation) {
		is_scaling = (this->src.rect.width != this->dst.rect.height) ||
			(this->src.rect.height != this->dst.rect.width);
	} else {
		is_scaling = (this->src.rect.width != this->dst.rect.width) ||
			(this->src.rect.height != this->dst.rect.height);
	}

	is_scaling = is_scaling ||
			(this->src.type == B2R2_FMT_TYPE_SEMI_PLANAR) ||
			(this->src.type == B2R2_FMT_TYPE_PLANAR);

	/* Check which type of transform */
	if (is_scaling && this->rotation) {
		ret = analyze_rot_scale(this, req, node_count, buf_count);
		if (ret < 0)
			goto error;
	} else if (is_scaling) {
		ret = analyze_scaling(this, req, node_count, buf_count);
		if (ret < 0)
			goto error;
	} else if (this->rotation) {
		ret = analyze_rotation(this, req, node_count, buf_count);
		if (ret < 0)
			goto error;
	} else {
		/* No additional nodes are required for a flip */
		ret = analyze_copy(this, req, node_count, buf_count);
		if (ret < 0)
			goto error;
		this->type = B2R2_FLIP;
	}

	/* Modify the horizontal and vertical scan order if FLIP */
	if (req->user_req.transform & B2R2_BLT_TRANSFORM_FLIP_H) {
		this->src.hso = B2R2_TY_HSO_RIGHT_TO_LEFT;
		this->src.rect.x += this->src.rect.width - 1;
		this->src.window.x = this->src.rect.x;
		this->src.dx = -this->src.window.width;
	} else {
		this->src.dx = this->src.window.width;
	}
	if (req->user_req.transform & B2R2_BLT_TRANSFORM_FLIP_V) {
		this->src.vso = B2R2_TY_VSO_BOTTOM_TO_TOP;
		this->src.rect.y += this->src.rect.height - 1;
		this->src.window.y = this->src.rect.y;
		this->src.dy = -this->src.window.height;
	} else {
		this->src.dy = this->src.window.height;
	}

	/* Set the dx and dy of the destination as well */
	this->dst.dx = this->dst.window.width;
	if (this->dst.vso == B2R2_TY_VSO_BOTTOM_TO_TOP)
		this->dst.dy = -this->dst.window.height;
	else
		this->dst.dy = this->dst.window.height;

	return 0;

error:
	return ret;
}

/**
 * analyze_copy() - analyze a copy operation
 */
static int analyze_copy(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count,
		u32 *buf_count)
{
	int ret;

	pdebug(KERN_INFO LOG_TAG "::%s: Analyzing copy...\n", __func__);

	memcpy(&this->src.window, &this->src.rect, sizeof(this->src.window));
	memcpy(&this->dst.window, &this->dst.rect, sizeof(this->dst.window));

	if (!this->blend && (this->src.fmt == this->dst.fmt) &&
			(this->src.type == B2R2_FMT_TYPE_RASTER)) {
		this->type = B2R2_DIRECT_COPY;
		*node_count = 1;
	} else {
		u32 copy_count = 0;
		u32 nbr_rows = 0;
		u32 nbr_cols = 0;

		this->type = B2R2_COPY;

		ret = analyze_fmt_conv(this, req, &copy_count);
		if (ret < 0)
			goto error;

		if (this->buf_count > 0) {
			setup_tmp_bufs(this);

			constrain_window(&this->src.window,
					this->tmp_bufs[0].fmt,
					this->work_bufs[0].size);

			this->dst.window.width = this->src.window.width;
			this->dst.window.height = this->src.window.height;

			/* Each copy will require an additional node */
			copy_count++;
		}

		/* Calculate how many rows and columns this will result in */
		nbr_cols = this->src.rect.width / this->src.window.width;
		if (this->src.rect.width % this->src.window.width)
			nbr_cols++;

		nbr_rows = this->src.rect.height / this->src.window.height;
		if (this->src.rect.height % this->src.window.height)
			nbr_rows++;

		/* Finally, calculate the node count */
		*node_count = copy_count * nbr_rows * nbr_cols;
	}

	this->src.dx = this->src.window.width;
	this->src.dy = this->src.window.height;
	this->dst.dx = this->dst.window.width;
	this->dst.dy = this->dst.window.height;

	return 0;

error:
	return ret;
}

/**
 * analyze_rot_scaling() - analyzes a combined rotation and scaling op
 */
static int analyze_rot_scale(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count,
		u32 *buf_count)
{
	int ret;

	ret = analyze_scale_factors(this);
	if (ret < 0)
		goto error;

	/* All rotations are written from the bottom up */
	this->dst.vso = B2R2_TY_VSO_BOTTOM_TO_TOP;
	this->dst.rect.y += this->dst.rect.height - 1;

	/* We will always need one tmp buffer */
	this->buf_count = 1;
	setup_tmp_bufs(this);

	/* Calculate the windows for this operation */
	setup_rot_scale_windows(this);

	pdebug(KERN_INFO LOG_TAG "::%s: tmp1=(%d, %d, %d, %d)\n", __func__,
		this->tmp_bufs[1].window.x, this->tmp_bufs[1].window.y,
		this->tmp_bufs[1].window.width,
		this->tmp_bufs[1].window.height);

	/* Calculate the number of nodes needed */
	ret = calculate_rot_scale_node_count(this, req);
	if (ret < 0)
		goto error;

	/* Setup the parameters for moving the window */
	this->src.dx = this->src.window.width;
	this->src.dy = this->src.window.height;
	this->dst.dx = this->dst.window.width;
	this->dst.dy = -this->dst.window.height;

	/* The first temporary buffer will never move */
	this->tmp_bufs[0].dx = 0;
	this->tmp_bufs[0].dy = 0;

	/* The second will, however */
	this->tmp_bufs[1].dx = this->tmp_bufs[1].window.width;
	this->tmp_bufs[1].dy = this->tmp_bufs[1].window.height;

	this->type = B2R2_SCALE_AND_ROTATE;


	return 0;

error:
	return ret;
}

/**
 * setup_rot_scale_windows() - computes the window sizes for a combined
 *                             rotation and scaling operation
 */
static void setup_rot_scale_windows(struct b2r2_node_split_job *this)
{
	struct b2r2_node_split_buf *tmp = &this->tmp_bufs[0];

	s32 old_width;
	s32 old_height;

	/* Start out assuming we can fit the entire rectangle in the tmp buf */
	memcpy(&this->src.window, &this->src.rect, sizeof(this->src.window));
	memcpy(&this->dst.window, &this->dst.rect, sizeof(this->dst.window));

	/* Limit the width to the maximum for scaling */
	this->src.window.width = MIN(this->src.window.width,
					B2R2_RESCALE_MAX_WIDTH);

	/* Set up the tmp buffer window and constrain it */
	tmp->window.width = rescale(this->src.window.width, this->horiz_sf);
	tmp->window.height = rescale(this->src.window.height, this->vert_sf);

	old_width = tmp->window.width;
	old_height = tmp->window.height;

	constrain_window(&tmp->window, tmp->fmt, this->max_buf_size);

	/* Update the source window with the constrained dimensions */
	if (tmp->window.width != old_width)
		this->src.window.width = inv_rescale(tmp->window.width,
							this->horiz_sf);
	if (tmp->window.height != old_height)
		this->src.window.height = inv_rescale(tmp->window.height,
							this->vert_sf);

	/* Set the destination window */
	this->dst.window.width = tmp->window.height;
	this->dst.window.height = tmp->window.width;

	/* We will use the second tmp buf to store information about the
	   rotation */
	memcpy(&this->tmp_bufs[1], &this->tmp_bufs[0],
			sizeof(this->tmp_bufs[1]));
	tmp = &this->tmp_bufs[1];

	/* This window will be smaller since rotation has a smaller maximum
	   width than scaling */
	tmp->window.width = B2R2_ROTATE_MAX_WIDTH;
#ifdef B2R2_ROTATION_HEIGHT_BUGFIX
	if (tmp->window.height > 16)
		tmp->window.height -= tmp->window.height % 16;
#endif

	/* The tmp rectangles will be the same size as the tmp buffer */
	memcpy(&this->tmp_bufs[0].rect, &this->tmp_bufs[0].window,
			sizeof(this->tmp_bufs[0].rect));
	memcpy(&this->tmp_bufs[1].rect, &this->tmp_bufs[0].window,
		sizeof(this->tmp_bufs[1].rect));

}

/**
 * calculate_rot_scale_node_count() - calculates the number of nodes required
 *                                    for a combined rotation and scaling
 */
static int calculate_rot_scale_node_count(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req)
{
	int ret;

	u32 copy_count;
	u32 rot_count;
	u32 scale_count;

	s32 right_width;
	s32 bottom_height;

	int rot_per_inner;
	int rot_per_right;
	int rot_per_bottom;
	int rot_per_bottom_right;

	int nbr_inner_rows;
	int nbr_inner_cols;

	struct b2r2_node_split_buf *tmp = &this->tmp_bufs[1];

	/* First check how many nodes we need for a simple copy */
	ret = analyze_fmt_conv(this, req, &copy_count);
	if (ret < 0)
		goto error;

	/* The number of rotation nodes per tile will vary depending on whether
	   it is an "inner" tile or an "edge" tile, since they may have
	   different sizes. */

	/* Because of the rotation height and width are flipped. */
	nbr_inner_cols = this->dst.rect.height / this->dst.window.height;
	nbr_inner_rows = this->dst.rect.width / this->dst.window.width;

	right_width = (this->dst.rect.height % this->dst.window.height);
	bottom_height = (this->dst.rect.width % this->dst.window.width);

	rot_per_inner = calculate_tile_count(this->dst.window.height,
				this->dst.window.width, tmp->window.width,
				tmp->window.height);
	rot_per_right = calculate_tile_count(right_width,
				this->dst.window.width, tmp->window.width,
				tmp->window.height);
	rot_per_bottom = calculate_tile_count(this->dst.window.height,
				bottom_height, tmp->window.width,
				tmp->window.height);
	rot_per_bottom_right = calculate_tile_count(right_width,
				bottom_height, tmp->window.width,
				tmp->window.height);

	/* First all the "inner" tile rotations */
	rot_count = rot_per_inner * nbr_inner_cols * nbr_inner_rows;

	/* Then all the "right edge" tile rotations */
	rot_count += rot_per_right * nbr_inner_rows;

	/* Then all the "bottom edge" tile rotations */
	rot_count += rot_per_bottom * nbr_inner_cols;

	/* Then the "bottom right edge" tile rotations */
	rot_count += rot_per_bottom_right;

	/* There will be one scale node per "inner" tile */
	scale_count = nbr_inner_cols * nbr_inner_rows;

	/* Update the scale count with the "edge" tiles */
	if (right_width)
		scale_count += nbr_inner_rows;
	if (bottom_height)
		scale_count += nbr_inner_cols;
	if (right_width && bottom_height)
		scale_count++;

	pdebug(KERN_INFO LOG_TAG "::%s: rot_per_inner=%d, rot_per_right=%d, "
		"rot_per_bottom=%d, rot_per_bottom_right=%d\n", __func__,
		rot_per_inner, rot_per_right, rot_per_bottom,
		rot_per_bottom_right);

	pdebug(KERN_INFO LOG_TAG "::%s: nbr_inner_cols=%d, nbr_inner_rows=%d\n",
		__func__, nbr_inner_cols, nbr_inner_rows);

	this->node_count = (scale_count + rot_count) * copy_count;

	return 0;
error:
	return ret;
}

/**
 * analyze_scaling() - analyze a rescale operation
 */
static int analyze_scaling(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count,
		u32 *buf_count)
{
	int ret;
	u32 copy_count;
	u32 nbr_rows;
	u32 nbr_cols;

	pdebug(KERN_INFO LOG_TAG "::%s: Analyzing scaling...\n", __func__);

	ret = analyze_scale_factors(this);
	if (ret < 0)
		goto error;

	/* Find out how many nodes a simple copy would require */
	ret = analyze_fmt_conv(this, req, &copy_count);
	if (ret < 0)
		goto error;


	memcpy(&this->src.window, &this->src.rect, sizeof(this->src.window));
	memcpy(&this->dst.window, &this->dst.rect, sizeof(this->dst.window));

	this->src.window.width = MIN(this->src.window.width,
					B2R2_RESCALE_MAX_WIDTH);
	if (this->horiz_rescale)
		this->dst.window.width = rescale(this->src.window.width,
						this->horiz_sf);
	else
		this->dst.window.width = this->src.window.width;

	/* Check if dest is a temp buffer */
	if (*buf_count > 0) {
		s32 old_width = this->dst.window.width;
		s32 old_height = this->dst.window.height;

		/* The operation will require one more node (to copy to tmp) */
		copy_count++;

		setup_tmp_bufs(this);

		constrain_window(&this->dst.window, this->tmp_bufs[0].fmt,
					this->work_bufs[0].size);

		/* The dimensions might have changed */
		if (this->dst.window.width != old_width) {
			if (this->horiz_rescale)
				this->src.window.width =
					inv_rescale(this->dst.window.width,
							this->horiz_sf);
			else
				this->src.window.width =
					this->dst.window.width;
		}
		if (this->dst.window.height != old_height) {
			if (this->vert_rescale)
				this->src.window.height =
					inv_rescale(this->dst.window.height,
							this->vert_sf);
			else
				this->src.window.height =
					this->dst.window.height;
		}
		if (this->vert_rescale)
			this->src.window.height =
				inv_rescale(this->dst.window.height,
						this->vert_sf);
	}

	nbr_cols = this->src.rect.width / this->src.window.width;
	if (this->src.rect.width % this->src.window.width)
		nbr_cols++;

	nbr_rows = this->src.rect.height / this->src.window.height;
	if (this->src.rect.height % this->src.window.height)
		nbr_rows++;

	pdebug(KERN_INFO LOG_TAG "::%s: Each stripe will require %d nodes\n",
			__func__, copy_count);

	*node_count = copy_count * nbr_rows * nbr_cols;

	this->type = B2R2_SCALE;

	return 0;

error:
	return ret;

}

/**
 * analyze_rotation() - analyze a rotate operation
 */
static int analyze_rotation(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count,
		u32 *buf_count)
{
	int ret;

	u32 copy_count;
	u32 nbr_rows;
	u32 nbr_cols;

	pdebug(KERN_INFO LOG_TAG "::%s: Analyzing rotation...\n", __func__);

	/* Find out how many nodes a simple copy would require */
	ret = analyze_fmt_conv(this, req, &copy_count);
	if (ret < 0)
		goto error;

	this->type = B2R2_ROTATE;


	/* The vertical scan order of the destination must be flipped */
	if (this->dst.vso == B2R2_TY_VSO_TOP_TO_BOTTOM) {
		this->dst.rect.y += this->dst.rect.height;
		this->dst.vso = B2R2_TY_VSO_BOTTOM_TO_TOP;
	} else {
		this->dst.rect.y -= this->dst.rect.height;
		this->dst.vso = B2R2_TY_VSO_TOP_TO_BOTTOM;
	}
	memcpy(&this->src.window, &this->src.rect, sizeof(this->src.window));
	memcpy(&this->dst.window, &this->dst.rect, sizeof(this->dst.window));

	this->src.window.width = B2R2_ROTATE_MAX_WIDTH;

#ifdef B2R2_ROTATION_HEIGHT_BUGFIX
	/*
	 * Because of a possible bug in the hardware, B2R2 cannot handle
	 * stripes that are not a multiple of 16 high. We need one extra node
	 * per stripe.
	 */
	if (this->src.window.height > 16)
		this->src.window.height -= this->src.window.height % 16;
#endif

	this->dst.window.width = this->src.window.height;
	this->dst.window.height = this->src.window.width;

	if (this->buf_count > 0) {
		/* One more node to copy to the tmp buf */
		copy_count++;

		setup_tmp_bufs(this);

		constrain_window(&this->src.window, this->tmp_bufs[0].fmt,
				this->work_bufs[0].size);

		this->dst.window.width = this->src.window.height;
		this->dst.window.height = this->src.window.width;
	}
	/* Calculate how many rows and columns this will result in */
	nbr_cols = this->src.rect.width / this->src.window.width;
	if (this->src.rect.width % this->src.window.width)
		nbr_cols++;

	nbr_rows = this->src.rect.height / this->src.window.height;
	if (this->src.rect.height % this->src.window.height)
		nbr_rows++;

	/* Finally, calculate the node count */
	*node_count = copy_count * nbr_rows * nbr_cols;

	return 0;

error:
	return ret;
}

/**
 * analyze_scale_factors() - determines the scale factors for the op
 */
static int analyze_scale_factors(struct b2r2_node_split_job *this)
{
	int ret;

	u16 hsf;
	u16 vsf;

	if (this->rotation) {
		ret = calculate_scale_factor(this->src.rect.width,
					this->dst.rect.height, &hsf);
		if (ret < 0)
			goto error;

		ret = calculate_scale_factor(this->src.rect.height,
					this->dst.rect.width, &vsf);
		if (ret < 0)
			goto error;
	} else {
		ret = calculate_scale_factor(this->src.rect.width,
					this->dst.rect.width, &hsf);
		if (ret < 0)
			goto error;

		ret = calculate_scale_factor(this->src.rect.height,
					this->dst.rect.height, &vsf);
		if (ret < 0)
			goto error;
	}

	this->horiz_rescale = hsf != (1 << 10);
	this->vert_rescale = vsf != (1 << 10);

	this->horiz_sf = hsf;
	this->vert_sf = vsf;

	return 0;
error:
	return ret;
}

/**
 * configure_tile() - configures one tile of a blit operation
 */
static int configure_tile(struct b2r2_node_split_job *this,
		struct b2r2_node *node, struct b2r2_node **next)
{
	int ret;
	struct b2r2_node *last;
	struct b2r2_node_split_buf *src = &this->src;
	struct b2r2_node_split_buf *dst = &this->dst;

	/* Do the configuration depending on operation type */
	switch (this->type) {
	case B2R2_DIRECT_FILL:
		configure_direct_fill(node, this->src.color, dst, &last);
		break;
	case B2R2_DIRECT_COPY:
		configure_direct_copy(node, &this->src, dst, &last);
		break;
	case B2R2_FILL:
		configure_fill(node, this->src.color, this->src.fmt,
				dst, this->ivmx, &last);
		break;
	case B2R2_FLIP: /* FLIP is just a copy with different VSO/HSO */
	case B2R2_COPY:
		if (this->buf_count > 0) {
			/* First do a copy to the tmp buf */
			configure_tmp_buf(&this->tmp_bufs[0], 1, &src->window);

			configure_copy(node, src, &this->tmp_bufs[0],
					this->ivmx, &node);

			/* Then set the source as the tmp buf */
			src = &this->tmp_bufs[0];

			/* We will not need the iVMX now */
			this->ivmx = NULL;
		}

		configure_copy(node, src, dst, this->ivmx, &last);
		break;
	case B2R2_ROTATE:
		if (this->buf_count > 0) {
			/* First do a copy to the tmp buf */
			configure_tmp_buf(&this->tmp_bufs[0], 1, &src->window);

			configure_copy(node, src, &this->tmp_bufs[0],
					this->ivmx, &node);

			/* Then set the source as the tmp buf */
			src = &this->tmp_bufs[0];

			/* We will not need the iVMX now */
			this->ivmx = NULL;
		}

		configure_rotate(node, src, dst, this->ivmx, &last);

		break;
	case B2R2_SCALE:
		if (this->buf_count > 0) {
			/* Scaling will be done first */
			configure_tmp_buf(&this->tmp_bufs[0], 1, &dst->window);

			dst = &this->tmp_bufs[0];
		}

		configure_scale(node, src, dst, this->horiz_sf, this->vert_sf,
				this->ivmx, &last);

		if (this->buf_count > 0) {
			/* Then copy the tmp buf to the destination */
			node = last;
			src = dst;
			dst = &this->dst;
			configure_copy(node, src, dst, NULL, &last);
		}
		break;
	case B2R2_SCALE_AND_ROTATE:
		configure_rot_scale(this, node, &last);
		break;
	default:
		printk(KERN_ERR "%s: Unsupported request\n", __func__);
		ret = -ENOSYS;
		goto error;

		break;
	}

	/* Scale and rotate will configure its own blending */
	if (this->type != B2R2_SCALE_AND_ROTATE) {

		/* Configure blending and clipping */
		do {
			BUG_ON(node == NULL);

			if (this->blend)
				configure_blend(node, this->flags,
						this->global_alpha,
						this->swap_fg_bg);

			if (this->clip)
				configure_clip(node, &this->clip_rect);

			node = node->next;

		} while (node != last);
	}

	/* Consume the nodes */
	*next = last;

	return 0;

error:
	return ret;
}

/**
 * configure_rot_scale() - configures a combined rotation and scaling op
 */
static void configure_rot_scale(struct b2r2_node_split_job *this,
		struct b2r2_node *node, struct b2r2_node **next)
{
	struct b2r2_node *rot_start;
	struct b2r2_node *last;

	struct b2r2_node_split_buf *tmp = &this->tmp_bufs[0];
	struct b2r2_blt_rect dst_win;


#ifdef B2R2_ROTATION_HEIGHT_BUGFIX
	bool has_leftovers = false;
	bool is_leftover = false;
#endif

	memcpy(&dst_win, &this->dst.window, sizeof(dst_win));

	tmp->rect.x = 0;
	tmp->rect.y = 0;
	tmp->rect.width = this->dst.window.height;
	tmp->rect.height = this->dst.window.width;

	memcpy(&tmp->window, &tmp->rect, sizeof(tmp->window));

	configure_scale(node, &this->src, tmp, this->horiz_sf,
			this->vert_sf, this->ivmx, &node);

	tmp->window.x = 0;
	tmp->window.y = 0;
	tmp->window.width = MIN(tmp->rect.width, B2R2_ROTATE_MAX_WIDTH);
	tmp->window.height = tmp->rect.height;

#ifdef B2R2_ROTATION_HEIGHT_BUGFIX
	if (tmp->rect.height > 16) {
		has_leftovers = true;
		tmp->window.height -= tmp->rect.height % 16;
	}
#endif

	tmp->dx = tmp->window.width;
	tmp->dy = tmp->window.height;

	this->dst.window.width = tmp->window.height;
	this->dst.window.height = tmp->window.width;

	rot_start = node;

	pdebug(KERN_INFO LOG_TAG "::%s: tmp_rect=(%d, %d, %d, %d)\n", __func__,
		tmp->rect.x, tmp->rect.y, tmp->rect.width, tmp->rect.height);
	do {

		if (tmp->window.x + tmp->dx > tmp->rect.width) {
			pdebug(KERN_INFO LOG_TAG "::%s: last col\n", __func__);
			tmp->window.width = tmp->rect.width - tmp->window.x;
			this->dst.window.height = tmp->window.width;
		}

		pdebug(KERN_INFO LOG_TAG "::%s: \ttmp=(%3d, %3d, %3d, %3d) "
			"\tdst=(%3d, %3d, %3d, %3d)\n", __func__, tmp->window.x,
			tmp->window.y, tmp->window.width, tmp->window.height,
			this->dst.window.x, this->dst.window.y,
			this->dst.window.width, this->dst.window.height);

		configure_rotate(node, tmp, &this->dst, NULL, &node);

#ifdef B2R2_ROTATION_HEIGHT_BUGFIX
		if (has_leftovers && !is_leftover) {
			is_leftover = true;

			tmp->window.y += tmp->dy;
			tmp->window.height = tmp->rect.height % 16;

			this->dst.window.x += tmp->dy;
			this->dst.window.width = tmp->window.height;

			continue;
		} else if (has_leftovers) {
			is_leftover = false;

			tmp->window.y -= tmp->dy;
			tmp->window.height = tmp->rect.height -
						tmp->window.height;

			this->dst.window.x -= tmp->dy;
			this->dst.window.width = tmp->window.height;
		}
#endif

		tmp->window.x += tmp->dx;
		this->dst.window.y -= tmp->dx;

	} while (tmp->window.x < tmp->rect.width);

	/* Configure blending and clipping for the rotation nodes */
	last = node;
	node = rot_start;
	do {
		BUG_ON(node == NULL);

		if (this->blend)
			configure_blend(node, this->flags,
					this->global_alpha, false);
		if (this->clip)
			configure_clip(node, &this->clip_rect);

		node = node->next;
	} while (node != last);

	memcpy(&this->dst.window, &dst_win, sizeof(this->dst.window));

	*next = node;
}

/**
 * configure_direct_fill() - configures the given node for direct fill
 *
 * @node  - the node to configure
 * @color - the fill color
 * @dst   - the destination buffer
 * @next  - the next empty node in the node list
 *
 * This operation will always consume one node only.
 */
static void configure_direct_fill(struct b2r2_node *node, u32 color,
		struct b2r2_node_split_buf *dst, struct b2r2_node **next)
{
	PTRACE_ENTRY();

	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_COLOR_FILL | B2R2_CIC_SOURCE_1;
	node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_1_DIRECT_FILL;

	/* Target setup */
	set_target(node, dst->addr, dst);

	/* Source setup */

	/* It seems B2R2 checks so that source and dest has the same format */
	node->node.GROUP3.B2R2_STY = to_native_fmt(dst->fmt);
	node->node.GROUP2.B2R2_S1CF = color;
	node->node.GROUP2.B2R2_S2CF = 0;

	/* Consume the node */
	*next = node->next;
}

/**
 * configure_direct_copy() - configures the node for direct copy
 *
 * @node - the node to configure
 * @src  - the source buffer
 * @dst  - the destination buffer
 * @next - the next empty node in the node list
 *
 * This operation will always consume one node only.
 */
static void configure_direct_copy(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, struct b2r2_node **next)
{
	PTRACE_ENTRY();

	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_1;
	node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_1_DIRECT_COPY;

	/* Source setup, use the base function to avoid altering the INS */
	set_src(&node->node.GROUP3, src->addr, src);

	/* Target setup */
	set_target(node, dst->addr, dst);

	/* Consume the node */
	*next = node->next;
}

/**
 * configure_fill() - configures the given node for color fill
 *
 * @node  - the node to configure
 * @color - the fill color
 * @fmt   - the source color format
 * @dst   - the destination buffer
 * @next  - the next empty node in the node list
 *
 * A normal fill operation can be combined with any other per pixel operations
 * such as blend.
 *
 * This operation will consume as many nodes as are required to write to the
 * destination format.
 */
static void configure_fill(struct b2r2_node *node, u32 color,
		enum b2r2_blt_fmt fmt, struct b2r2_node_split_buf *dst,
		const u32 *ivmx, struct b2r2_node **next)
{
	struct b2r2_node *last;

	PTRACE_ENTRY();

	/* Configure the destination */
	configure_dst(node, dst, ivmx, &last);

	do {
		BUG_ON(node == NULL);

		node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_1 |
							B2R2_CIC_COLOR_FILL;
		node->node.GROUP0.B2R2_INS |=
				B2R2_INS_SOURCE_1_COLOR_FILL_REGISTER;

		/* B2R2 has a bug that disables color fill from S2. As a
		   workaround we use S1 for the color. */
		node->node.GROUP3.B2R2_STY = to_native_fmt(fmt);
		node->node.GROUP2.B2R2_S1CF = color;
		node->node.GROUP2.B2R2_S2CF = 0;

		/* Setup the iVMX for color conversion */
		if (ivmx != NULL)
			set_ivmx(node, ivmx);

		if  ((dst->type == B2R2_FMT_TYPE_PLANAR) ||
				(dst->type == B2R2_FMT_TYPE_SEMI_PLANAR)) {

			node->node.GROUP0.B2R2_INS |=
					B2R2_INS_RESCALE2D_ENABLED;
			node->node.GROUP8.B2R2_FCTL =
					B2R2_FCTL_HF2D_MODE_ENABLE_RESIZER |
					B2R2_FCTL_LUMA_HF2D_MODE_ENABLE_RESIZER;
			node->node.GROUP9.B2R2_RSF =
					(1 << (B2R2_RSF_HSRC_INC_SHIFT + 10)) |
					(1 << (B2R2_RSF_VSRC_INC_SHIFT + 10));
			node->node.GROUP9.B2R2_RZI =
					B2R2_RZI_DEFAULT_HNB_REPEAT |
					B2R2_RZI_DEFAULT_VNB_REPEAT;

			node->node.GROUP10.B2R2_RSF =
					(1 << (B2R2_RSF_HSRC_INC_SHIFT + 10)) |
					(1 << (B2R2_RSF_VSRC_INC_SHIFT + 10));
			node->node.GROUP10.B2R2_RZI =
					B2R2_RZI_DEFAULT_HNB_REPEAT |
					B2R2_RZI_DEFAULT_VNB_REPEAT;
		}

		node = node->next;

	} while (node != last);

	/* Consume the nodes */
	*next = node;
}

/**
 * configure_copy() - configures the given node for a copy operation
 *
 * @node - the node to configure
 * @src  - the source buffer
 * @dst  - the destination buffer
 * @ivmx - the iVMX to use for color conversion
 * @next - the next empty node in the node list
 *
 * This operation will consume as many nodes as are required to write to the
 * destination format.
 */
static void configure_copy(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, const u32 *ivmx,
		struct b2r2_node **next)
{
	struct b2r2_node *last;

	PTRACE_ENTRY();

	configure_dst(node, dst, ivmx, &last);

	/* Configure the source for each node */
	do {
		BUG_ON(node == NULL);

		node->node.GROUP0.B2R2_ACK |= B2R2_ACK_MODE_BYPASS_S2_S3;

		/* Configure the source(s) */
		configure_src(node, src, ivmx);

		node = node->next;
	} while (node != last);

	/* Consume the nodes */
	*next = node;
}

/**
 * configure_rotate() - configures the given node for rotation
 *
 * @node - the node to configure
 * @src  - the source buffer
 * @dst  - the destination buffer
 * @ivmx - the iVMX to use for color conversion
 * @next - the next empty node in the node list
 *
 * This operation will consume as many nodes are are required by the combination
 * of rotating and writing the destination format.
 */
static void configure_rotate(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, const u32 *ivmx,
		struct b2r2_node **next)
{
	struct b2r2_node *last;

	configure_copy(node, src, dst, ivmx, &last);

	do {
		BUG_ON(node == NULL);

		node->node.GROUP0.B2R2_INS |= B2R2_INS_ROTATION_ENABLED;

		node = node->next;

	} while (node != last);

	/* Consume the nodes */
	*next = node;
}

/**
 * configure_scale() - configures the given node for scaling
 *
 * @node  - the node to configure
 * @src   - the source buffer
 * @dst   - the destination buffer
 * @h_rsf - the horizontal rescale factor
 * @v_rsf - the vertical rescale factor
 * @ivmx  - the iVMX to use for color conversion
 * @next  - the next empty node in the node list
 */
static void configure_scale(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, u16 h_rsf, u16 v_rsf,
		const u32 *ivmx, struct b2r2_node **next)
{
	struct b2r2_node *last;

	u32 fctl = 0;
	u32 rsf = 0;
	u32 rzi = 0;

	/* Configure horizontal rescale */
	fctl |= B2R2_FCTL_HF2D_MODE_ENABLE_RESIZER |
			B2R2_FCTL_LUMA_HF2D_MODE_ENABLE_RESIZER;
	rsf &= ~(0xffff << B2R2_RSF_HSRC_INC_SHIFT);
	rsf |= h_rsf << B2R2_RSF_HSRC_INC_SHIFT;
	rzi |= B2R2_RZI_DEFAULT_HNB_REPEAT;

	/* Configure vertical rescale */
	fctl |= B2R2_FCTL_VF2D_MODE_ENABLE_RESIZER |
			B2R2_FCTL_LUMA_VF2D_MODE_ENABLE_RESIZER;
	rsf &= ~(0xffff << B2R2_RSF_VSRC_INC_SHIFT);
	rsf |= v_rsf << B2R2_RSF_VSRC_INC_SHIFT;
	rzi |= B2R2_RZI_DEFAULT_VNB_REPEAT;

	configure_copy(node, src, dst, ivmx, &last);

	do {
		BUG_ON(node == NULL);

		node->node.GROUP0.B2R2_CIC |= B2R2_CIC_FILTER_CONTROL |
						B2R2_CIC_RESIZE_LUMA |
						B2R2_CIC_RESIZE_CHROMA;
		node->node.GROUP0.B2R2_INS |=
				B2R2_INS_RESCALE2D_ENABLED;

		/* Set the filter control and rescale registers */
		node->node.GROUP8.B2R2_FCTL |= fctl;
		if ((src->type == B2R2_FMT_TYPE_SEMI_PLANAR) ||
				(src->type == B2R2_FMT_TYPE_PLANAR) ||
				(dst->type == B2R2_FMT_TYPE_SEMI_PLANAR) ||
				(dst->type == B2R2_FMT_TYPE_PLANAR)) {
			node->node.GROUP9.B2R2_RSF =
					h_rsf/2 << B2R2_RSF_HSRC_INC_SHIFT |
					v_rsf/2 << B2R2_RSF_VSRC_INC_SHIFT;

			/* Set the Luma rescale registers as well */
			node->node.GROUP10.B2R2_RSF |= rsf;
			node->node.GROUP10.B2R2_RZI |= rzi;
		} else {
			node->node.GROUP9.B2R2_RSF = rsf;
		}
		node->node.GROUP9.B2R2_RZI |= rzi;

		node = node->next;

	} while (node != last);

	/* Consume the nodes */
	*next = node;
}

/**
 * configure_src() - configures the source registers and the iVMX
 *
 * @node - the node to configure
 * @src  - the source buffer
 * @ivmx - the iVMX to use for color conversion
 *
 * This operation will not consume any nodes
 */
static void configure_src(struct b2r2_node *node,
		struct b2r2_node_split_buf *src, const u32 *ivmx)
{
	struct b2r2_node_split_buf tmp_buf;

	PTRACE_ENTRY();

	/* Configure S1 - S3 */
	switch (src->type) {
	case B2R2_FMT_TYPE_RASTER:
		set_src_2(node, src->addr, src);
		break;
	case B2R2_FMT_TYPE_SEMI_PLANAR:
		memcpy(&tmp_buf, src, sizeof(tmp_buf));

		/* Vertical and horizontal resolution is half */
		tmp_buf.window.x >>= 1;
		tmp_buf.window.y >>= 1;
		tmp_buf.window.width >>= 1;
		tmp_buf.window.height >>= 1;

		set_src_3(node, src->addr, src);
		set_src_2(node, tmp_buf.chroma_addr, &tmp_buf);
		break;
	case B2R2_FMT_TYPE_PLANAR:
		memcpy(&tmp_buf, src, sizeof(tmp_buf));

		node->node.GROUP0.B2R2_INS |= B2R2_INS_RESCALE2D_ENABLED;

		/* Vertical and horizontal resolution is half */
		tmp_buf.pitch >>= 1;

		tmp_buf.window.x >>= 1;
		tmp_buf.window.y >>= 1;
		tmp_buf.window.width >>= 1;
		tmp_buf.window.height >>= 1;

		set_src_2(node, tmp_buf.chroma_cr_addr, &tmp_buf);

		set_src_1(node, tmp_buf.chroma_addr, &tmp_buf);

		set_src_3(node, src->addr, src);
		break;
	default:
		/* Should never, ever happen */
		BUG_ON(1);
		break;
	}

	/* Configure the iVMX for color space conversions */
	if (ivmx != NULL)
		set_ivmx(node, ivmx);
}

/**
 * configure_dst() - configures the destination registers of the given node
 *
 * @node - the node to configure
 * @ivmx - the iVMX to use for color conversion
 * @dst  - the destination buffer
 *
 * This operation will consume as many nodes as are required to write the
 * destination format.
 */
static void configure_dst(struct b2r2_node *node,
		struct b2r2_node_split_buf *dst, const u32 *ivmx,
		struct b2r2_node **next)
{
	int nbr_planes;
	int i;

	struct b2r2_node_split_buf dst_planes[3];

	PTRACE_ENTRY();

	memcpy(&dst_planes[0], dst, sizeof(dst_planes[0]));

	/* Check how many planes we should use */
	switch (dst->type) {
	case B2R2_FMT_TYPE_SEMI_PLANAR:
		nbr_planes = 2;

		memcpy(&dst_planes[1], dst, sizeof(dst_planes[1]));

		dst_planes[1].addr = dst->chroma_addr;
		dst_planes[1].plane_selection = B2R2_TTY_CHROMA_NOT_LUMA;
		break;
	case B2R2_FMT_TYPE_PLANAR:
		nbr_planes = 3;

		memcpy(&dst_planes[1], dst, sizeof(dst_planes[1]));
		memcpy(&dst_planes[2], dst, sizeof(dst_planes[2]));

		dst_planes[1].addr = dst->chroma_addr;
		dst_planes[1].plane_selection = B2R2_TTY_CHROMA_NOT_LUMA;

		dst_planes[2].addr = dst->chroma_cr_addr;
		dst_planes[2].plane_selection = B2R2_TTY_CHROMA_NOT_LUMA;
		break;
	default:
		nbr_planes = 1;
		break;
	}

	/* Configure one node for each plane */
	for (i = 0; i < nbr_planes; i++) {

		set_target(node, dst_planes[i].addr, &dst_planes[i]);

		node = node->next;
	}

	pdebug(KERN_INFO LOG_TAG "::%s: %d nodes consumed\n", __func__,
			nbr_planes);

	/* Consume one node */
	*next = node;
}

/**
 * configure_blend() - configures the given node for alpha blending
 *
 * @node         - the node to configure
 * @flags        - the flags passed in the blt_request
 * @global_alpha - the global alpha to use (if enabled in flags)
 * @swap_fg_bg   - if true, fg will be on s1 instead of s2
 *
 * This operation will not consume any nodes.
 *
 * NOTE: This method should be called _AFTER_ the destination has been
 *       configured.
 *
 * WARNING: Take care when using this with semi-planar or planar sources since
 *          either S1 or S2 will be overwritten!
 */
static void configure_blend(struct b2r2_node *node, u32 flags, u32 global_alpha,
		bool swap_fg_bg)
{
	PTRACE_ENTRY();

	node->node.GROUP0.B2R2_ACK &= ~(B2R2_ACK_MODE_BYPASS_S2_S3);

	/* Check if the foreground is premultiplied */
	if ((flags & B2R2_BLT_FLAG_SRC_IS_NOT_PREMULT) != 0)
		node->node.GROUP0.B2R2_ACK |= B2R2_ACK_MODE_BLEND_NOT_PREMULT;
	else
		node->node.GROUP0.B2R2_ACK |= B2R2_ACK_MODE_BLEND_PREMULT;


	/* Check if global alpha blend should be enabled */
	if ((flags & B2R2_BLT_FLAG_GLOBAL_ALPHA_BLEND) != 0)
		node->node.GROUP0.B2R2_ACK |=
			global_alpha <<	(B2R2_ACK_GALPHA_ROPID_SHIFT - 1);
	else
		node->node.GROUP0.B2R2_ACK |=
			(128 << B2R2_ACK_GALPHA_ROPID_SHIFT);

	/* Copy the destination config to the appropriate source and clear any
	   clashing flags */
	if (swap_fg_bg) {
		/* S1 will be foreground, S2 background */
		node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_2;
		node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_2_FETCH_FROM_MEM;
		node->node.GROUP0.B2R2_ACK |= B2R2_ACK_SWAP_FG_BG;

		node->node.GROUP4.B2R2_SBA = node->node.GROUP1.B2R2_TBA;
		node->node.GROUP4.B2R2_STY = node->node.GROUP1.B2R2_TTY;
		node->node.GROUP4.B2R2_SXY = node->node.GROUP1.B2R2_TXY;
		node->node.GROUP4.B2R2_SSZ = node->node.GROUP1.B2R2_TSZ;

		node->node.GROUP4.B2R2_STY &= ~(B2R2_S2TY_A1_SUBST_KEY_MODE |
				B2R2_S2TY_CHROMA_LEFT_EXT_AVERAGE);
	} else {
		/* S1 will be background, S2 foreground */
		node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_1;
		node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_1_FETCH_FROM_MEM;

		node->node.GROUP3.B2R2_SBA = node->node.GROUP1.B2R2_TBA;
		node->node.GROUP3.B2R2_STY = node->node.GROUP1.B2R2_TTY;
		node->node.GROUP3.B2R2_SXY = node->node.GROUP1.B2R2_TXY;

		node->node.GROUP3.B2R2_STY &= ~(B2R2_S1TY_A1_SUBST_KEY_MODE |
					B2R2_S1TY_ENABLE_ROTATION);
	}
}

/**
 * configure_clip() - configures destination clipping for the given node
 *
 *   @node      - the node to configure
 *   @clip_rect - the clip rectangle
 *
 *  This operation does not consume any nodes.
 */
static void configure_clip(struct b2r2_node *node,
		struct b2r2_blt_rect *clip_rect)
{
	PTRACE_ENTRY();

	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_CLIP_WINDOW;
	node->node.GROUP0.B2R2_INS |= B2R2_INS_RECT_CLIP_ENABLED;

	/* Clip window setup */
	node->node.GROUP6.B2R2_CWO = ((clip_rect->y & 0x7FFF) << 16) |
			(clip_rect->x & 0x7FFF);
	node->node.GROUP6.B2R2_CWS =
			(((clip_rect->y + clip_rect->height) & 0x7FFF) << 16) |
			((clip_rect->x + clip_rect->width) & 0x7FFF);
}

/**
 * configure_tmp_buf() - configures a temporary buffer
 */
static void configure_tmp_buf(struct b2r2_node_split_buf *this, int index,
		struct b2r2_blt_rect *window)
{
	memcpy(&this->window, window, sizeof(this->window));

	this->pitch = fmt_byte_pitch(this->fmt, this->window.width);
	this->height = this->window.height;
	this->tmp_buf_index = index;

	/* Temporary buffers rects are always positioned in origo */
	this->window.x = 0;
	this->window.y = 0;
}

/**
 * set_buf() - configures the given buffer with the provided values
 *
 * @addr       - the physical base address
 * @img        - the blt image to base the buffer on
 * @rect       - the rectangle to use
 * @color_fill - determines whether the buffer should be used for color fill
 * @color      - the color to use in case of color fill
 */
static void set_buf(struct b2r2_node_split_buf *buf, u32 addr,
		const struct b2r2_blt_img *img,
		const struct b2r2_blt_rect *rect, bool color_fill, u32 color)
{
	memset(buf, 0, sizeof(*buf));

	buf->fmt = img->fmt;
	buf->type = get_fmt_type(img->fmt);

	if (color_fill) {
		buf->type = B2R2_FMT_TYPE_RASTER;
		buf->color = color;
	} else {
		buf->addr = addr;

		buf->alpha_range = get_alpha_range(img->fmt);

		buf->pitch = fmt_byte_pitch(img->fmt, img->width);
		buf->height = img->height;

		switch (buf->type) {
		case B2R2_FMT_TYPE_SEMI_PLANAR:
			buf->chroma_addr = (u32)(((u8 *)addr) +
					img->width * img->height);
			break;
		case B2R2_FMT_TYPE_PLANAR:
			buf->chroma_addr = (u32)(((u8 *)addr) +
					img->width * img->height);
			buf->chroma_cr_addr = (u32)(((u8 *)buf->chroma_addr) +
					(buf->pitch/2) * (buf->height/2));
			break;
		default:
			break;
		}

		memcpy(&buf->rect, rect, sizeof(buf->rect));
	}
}

/**
 * constrain_window() - resizes a window to fit the given size restraints
 *
 * NOTE: Width will be kept constant.
 */
static void constrain_window(struct b2r2_blt_rect *window,
		enum b2r2_blt_fmt fmt, u32 max_size)
{
	u32 current_size;
	u32 pitch;

	pitch = fmt_byte_pitch(fmt, window->width);
	current_size = pitch * window->height;

	if (current_size > max_size) {
		u32 new_height;

		new_height = max_size / pitch;

		if (new_height == 0) {
			/* Couldn't even fit one scan line */
			window->width = B2R2_RESCALE_MAX_WIDTH;
			pitch = fmt_byte_pitch(fmt, window->width);
			new_height = max_size / pitch;

			/* If we can't even fit 128 pixels, something has gone
			   terribly wrong */
			BUG_ON(new_height == 0);
		}

		window->height = new_height;
	}
}

/**
 * setup_tmp_bufs() - configure the temporary buffers
 */
static void setup_tmp_bufs(struct b2r2_node_split_job *this)
{
	int i;
	u32 size;
	u32 pitch;
	enum b2r2_blt_fmt fmt;

	if (is_rgb_fmt(this->dst.fmt))
		fmt = B2R2_BLT_FMT_32_BIT_ARGB8888;
	else if (is_bgr_fmt(this->dst.fmt))
		fmt = B2R2_BLT_FMT_32_BIT_ABGR8888;
	else if (is_yuv_fmt(this->dst.fmt))
		fmt = B2R2_BLT_FMT_32_BIT_AYUV8888;
	else
		/* Wait, what? */
		BUG_ON(1);

	/* Check if the tmp buffer can be as large as the dest rect */
	pitch = fmt_byte_pitch(fmt, this->dst.rect.width);
	size = pitch * this->dst.rect.height;

	size = MIN(size, this->max_buf_size);

	for (i = 0; i < this->buf_count; i++) {
		this->work_bufs[i].size = size;
		this->work_bufs[i].phys_addr = i;

		memset(&this->tmp_bufs[i], 0, sizeof(this->tmp_bufs[i]));

		this->tmp_bufs[i].fmt = fmt;
		this->tmp_bufs[i].type = B2R2_FMT_TYPE_RASTER;
		this->tmp_bufs[i].alpha_range = get_alpha_range(fmt);
	}
}

/**
 * get_alpha_range() - returns the alpha range of the given format
 */
static inline enum b2r2_ty get_alpha_range(enum b2r2_blt_fmt fmt)
{
	switch (fmt) {
	case B2R2_BLT_FMT_24_BIT_ARGB8565:
	case B2R2_BLT_FMT_32_BIT_ARGB8888:
	case B2R2_BLT_FMT_32_BIT_AYUV8888:
	case B2R2_BLT_FMT_8_BIT_A8:
	case B2R2_BLT_FMT_32_BIT_ABGR8888:
		return B2R2_TY_ALPHA_RANGE_255; /* 0 - 255 */
		break;
	default:
		break;
	}

	return B2R2_TY_ALPHA_RANGE_128; /* 0 - 128 */
}

/**
 * get_alpha() - parses the alpha value from the given pixel
 */
static inline u8 get_alpha(enum b2r2_blt_fmt fmt, u32 pixel)
{
	u8 alpha;

	switch (fmt) {
	case B2R2_BLT_FMT_32_BIT_ARGB8888:
	case B2R2_BLT_FMT_32_BIT_ABGR8888:
	case B2R2_BLT_FMT_32_BIT_AYUV8888:
		alpha = (pixel >> 24);
		break;
	case B2R2_BLT_FMT_24_BIT_ARGB8565:
		alpha = (pixel & 0xfff) >> 16;
		break;
	case B2R2_BLT_FMT_16_BIT_ARGB4444:
		alpha = (pixel & 0xfff) >> 9;
		break;
	case B2R2_BLT_FMT_16_BIT_ARGB1555:
		alpha = (pixel >> 15) * 255;
		break;
	case B2R2_BLT_FMT_1_BIT_A1:
		alpha = pixel * 255;
		break;
	case B2R2_BLT_FMT_8_BIT_A8:
		alpha = pixel;
		break;
	default:
		alpha = 255;
	}

	return alpha;
}

/**
 * fmt_has_alpha() - returns whether the given format carries an alpha value
 */
static inline bool fmt_has_alpha(enum b2r2_blt_fmt fmt)
{
	switch (fmt) {
	case B2R2_BLT_FMT_16_BIT_ARGB4444:
	case B2R2_BLT_FMT_16_BIT_ARGB1555:
	case B2R2_BLT_FMT_32_BIT_ARGB8888:
	case B2R2_BLT_FMT_32_BIT_ABGR8888:
	case B2R2_BLT_FMT_24_BIT_ARGB8565:
	case B2R2_BLT_FMT_32_BIT_AYUV8888:
	case B2R2_BLT_FMT_1_BIT_A1:
	case B2R2_BLT_FMT_8_BIT_A8:
		return true;
	default:
		return false;
	}
}

/**
 * is_rgb_fmt() - returns whether the given format is a rgb format
 */
static inline bool is_rgb_fmt(enum b2r2_blt_fmt fmt)
{
	switch (fmt) {
	case B2R2_BLT_FMT_16_BIT_ARGB4444:
	case B2R2_BLT_FMT_16_BIT_ARGB1555:
	case B2R2_BLT_FMT_16_BIT_RGB565:
	case B2R2_BLT_FMT_24_BIT_RGB888:
	case B2R2_BLT_FMT_32_BIT_ARGB8888:
	case B2R2_BLT_FMT_24_BIT_ARGB8565:
	case B2R2_BLT_FMT_1_BIT_A1:
	case B2R2_BLT_FMT_8_BIT_A8:
		return true;
	default:
		return false;
	}
}

/**
 * is_bgr_fmt() - returns whether the given format is a bgr format
 */
static inline bool is_bgr_fmt(enum b2r2_blt_fmt fmt)
{
	return (fmt == B2R2_BLT_FMT_32_BIT_ABGR8888);
}

/**
 * is_yuv_fmt() - returns whether the given format is a yuv format
 */
static inline bool is_yuv_fmt(enum b2r2_blt_fmt fmt)
{
	switch (fmt) {
	case B2R2_BLT_FMT_24_BIT_YUV888:
	case B2R2_BLT_FMT_32_BIT_AYUV8888:
	case B2R2_BLT_FMT_Y_CB_Y_CR:
	case B2R2_BLT_FMT_CB_Y_CR_Y:
	case B2R2_BLT_FMT_YUV420_PACKED_PLANAR:
	case B2R2_BLT_FMT_YUV422_PACKED_PLANAR:
	case B2R2_BLT_FMT_YUV420_PACKED_SEMI_PLANAR:
	case B2R2_BLT_FMT_YUV422_PACKED_SEMI_PLANAR:
	case B2R2_BLT_FMT_YUV420_PACKED_SEMIPLANAR_MB_STE:
	case B2R2_BLT_FMT_YUV422_PACKED_SEMIPLANAR_MB_STE:
		return true;
	default:
		return false;
	}
}

/**
 * get_fmt_byte_pitch() - returns the pitch of a pixmap with the given width
 */
static inline int fmt_byte_pitch(enum b2r2_blt_fmt fmt, u32 width)
{
	int pitch;

	switch (fmt) {

	case B2R2_BLT_FMT_1_BIT_A1:
		pitch = width >> 3; /* Shift is faster than division */
		if ((width & 0x3) != 0) /* Check for remainder */
			pitch++;
		return pitch;

	case B2R2_BLT_FMT_8_BIT_A8:                        /* Fall through */
	case B2R2_BLT_FMT_YUV420_PACKED_PLANAR:            /* Fall through */
	case B2R2_BLT_FMT_YUV420_PACKED_SEMIPLANAR_MB_STE: /* Fall through */
	case B2R2_BLT_FMT_YUV420_PACKED_SEMI_PLANAR:       /* Fall through */
	case B2R2_BLT_FMT_YUV422_PACKED_PLANAR:            /* Fall through */
	case B2R2_BLT_FMT_YUV422_PACKED_SEMIPLANAR_MB_STE: /* Fall through */
	case B2R2_BLT_FMT_YUV422_PACKED_SEMI_PLANAR:
		return width;

	case B2R2_BLT_FMT_16_BIT_ARGB4444: /* Fall through */
	case B2R2_BLT_FMT_16_BIT_ARGB1555: /* Fall through */
	case B2R2_BLT_FMT_16_BIT_RGB565:   /* Fall through */
	case B2R2_BLT_FMT_Y_CB_Y_CR:       /* Fall through */
	case B2R2_BLT_FMT_CB_Y_CR_Y:
		return width << 1;

	case B2R2_BLT_FMT_24_BIT_RGB888:   /* Fall through */
	case B2R2_BLT_FMT_24_BIT_ARGB8565: /* Fall through */
	case B2R2_BLT_FMT_24_BIT_YUV888:
		return width * 3;

	case B2R2_BLT_FMT_32_BIT_ARGB8888: /* Fall through */
	case B2R2_BLT_FMT_32_BIT_ABGR8888: /* Fall through */
	case B2R2_BLT_FMT_32_BIT_AYUV8888:
		return width << 2;

	default:
		/* Should never, ever happen */
		BUG_ON(1);
		return 0;
	}
}

/**
 * to_native_fmt() - returns the native B2R2 format
 */
static inline enum b2r2_native_fmt to_native_fmt(enum b2r2_blt_fmt fmt)
{

	switch (fmt) {
	case B2R2_BLT_FMT_UNUSED:
		return B2R2_NATIVE_RGB565;
	case B2R2_BLT_FMT_1_BIT_A1:
		return B2R2_NATIVE_A1;
	case B2R2_BLT_FMT_8_BIT_A8:
		return B2R2_NATIVE_A8;
	case B2R2_BLT_FMT_16_BIT_RGB565:
		return B2R2_NATIVE_RGB565;
	case B2R2_BLT_FMT_16_BIT_ARGB4444:
		return B2R2_NATIVE_ARGB4444;
	case B2R2_BLT_FMT_16_BIT_ARGB1555:
		return B2R2_NATIVE_ARGB1555;
	case B2R2_BLT_FMT_24_BIT_ARGB8565:
		return B2R2_NATIVE_ARGB8565;
	case B2R2_BLT_FMT_24_BIT_RGB888:
		return B2R2_NATIVE_RGB888;
	case B2R2_BLT_FMT_24_BIT_YUV888:
		return B2R2_NATIVE_YCBCR888;
	case B2R2_BLT_FMT_32_BIT_ABGR8888: /* Not actually supported by HW */
	case B2R2_BLT_FMT_32_BIT_ARGB8888:
		return B2R2_NATIVE_ARGB8888;
	case B2R2_BLT_FMT_32_BIT_AYUV8888:
		return B2R2_NATIVE_AYCBCR8888;
	case B2R2_BLT_FMT_CB_Y_CR_Y:
		return B2R2_NATIVE_YCBCR422R;
	case B2R2_BLT_FMT_Y_CB_Y_CR:
		return B2R2_NATIVE_YCBCR422R;
	case B2R2_BLT_FMT_YUV420_PACKED_PLANAR:
		return B2R2_NATIVE_YCBCR42X_MB;
	case B2R2_BLT_FMT_YUV420_PACKED_SEMIPLANAR_MB_STE:
		return B2R2_NATIVE_YCBCR42X_MBN;
	case B2R2_BLT_FMT_YUV420_PACKED_SEMI_PLANAR:
		return B2R2_NATIVE_YCBCR42X_R2B;
	case B2R2_BLT_FMT_YUV422_PACKED_PLANAR:
		return B2R2_NATIVE_YCBCR42X_MB;
	case B2R2_BLT_FMT_YUV422_PACKED_SEMIPLANAR_MB_STE:
		return B2R2_NATIVE_YCBCR42X_MBN;
	case B2R2_BLT_FMT_YUV422_PACKED_SEMI_PLANAR:
		return B2R2_NATIVE_YCBCR42X_R2B;
	default:
		/* Should never ever happen */
		return B2R2_NATIVE_BYTE;
	}
}

/**
 * get_fmt_type() - returns the type of the given format (raster, planar, etc.)
 */
static inline enum b2r2_fmt_type get_fmt_type(enum b2r2_blt_fmt fmt)
{
	switch (fmt) {
	case B2R2_BLT_FMT_16_BIT_ARGB4444:
	case B2R2_BLT_FMT_16_BIT_ARGB1555:
	case B2R2_BLT_FMT_16_BIT_RGB565:
	case B2R2_BLT_FMT_24_BIT_RGB888:
	case B2R2_BLT_FMT_32_BIT_ARGB8888:
	case B2R2_BLT_FMT_Y_CB_Y_CR:
	case B2R2_BLT_FMT_CB_Y_CR_Y:
	case B2R2_BLT_FMT_32_BIT_ABGR8888:
	case B2R2_BLT_FMT_24_BIT_ARGB8565:
	case B2R2_BLT_FMT_24_BIT_YUV888:
	case B2R2_BLT_FMT_32_BIT_AYUV8888:
	case B2R2_BLT_FMT_1_BIT_A1:
	case B2R2_BLT_FMT_8_BIT_A8:
		return B2R2_FMT_TYPE_RASTER;
	case B2R2_BLT_FMT_YUV420_PACKED_PLANAR:
	case B2R2_BLT_FMT_YUV422_PACKED_PLANAR:
		return B2R2_FMT_TYPE_PLANAR;
	case B2R2_BLT_FMT_YUV420_PACKED_SEMI_PLANAR:
	case B2R2_BLT_FMT_YUV422_PACKED_SEMI_PLANAR:
	case B2R2_BLT_FMT_YUV420_PACKED_SEMIPLANAR_MB_STE:
	case B2R2_BLT_FMT_YUV422_PACKED_SEMIPLANAR_MB_STE:
		return B2R2_FMT_TYPE_SEMI_PLANAR;
	default:
		return B2R2_FMT_TYPE_RASTER;
	}
}

/**
 * type2str() - returns a string representation of the type (for debug prints)
 */
static inline char *type2str(enum b2r2_op_type type)
{
	switch (type) {
	case B2R2_DIRECT_COPY:
		return "B2R2_DIRECT COPY";
	case B2R2_DIRECT_FILL:
		return "B2R2_DIRECT_FILL";
	case B2R2_COPY:
		return "B2R2_COPY";
	case B2R2_FILL:
		return "B2R2_FILL";
	case B2R2_SCALE:
		return "B2R2_SCALE";
	case B2R2_ROTATE:
		return "B2R2_ROTATE";
	case B2R2_SCALE_AND_ROTATE:
		return "B2R2_SCALE_AND_ROTATE";
	case B2R2_FLIP:
		return "B2R2_FLIP";
	default:
		return "UNKNOWN";
	}
}

/**
 * is_transform() - returns whether the given request is a transform operation
 */
static inline bool is_transform(const struct b2r2_blt_request *req)
{
	return (req->user_req.transform != B2R2_BLT_TRANSFORM_NONE) ||
			(req->user_req.src_rect.width !=
				req->user_req.dst_rect.width) ||
			(req->user_req.src_rect.height !=
				req->user_req.dst_rect.height);
}

/**
 * calculate_scale_factor() - calculates the scale factor between the given
 *                            values
 */
static inline int calculate_scale_factor(u32 from, u32 to, u16 *sf_out)
{
	int ret;
	u32 sf = (from << 10) / to;

	if ((sf & 0xffff0000) != 0) {
		/* Overflow error */
		pdebug(KERN_ERR LOG_TAG "::%s: "
			"Scale factor too large\n", __func__);
		ret = -EINVAL;
		goto error;
	} else if (sf == 0) {
		pdebug(KERN_ERR LOG_TAG "::%s: "
			"Scale factor too small\n", __func__);
		ret = -EINVAL;
		goto error;
	}

	*sf_out = (u16)sf;

	return 0;

error:
	return ret;
}

/**
 * calculate_tile_count() - calculates how many tiles will fit
 */
static int calculate_tile_count(s32 area_width, s32 area_height, s32 tile_width,
		s32 tile_height)
{
	int nbr_cols;
	int nbr_rows;

	pdebug(KERN_INFO LOG_TAG "::%s: area=(%d, %d) tile=(%d, %d)\n",
			__func__, area_width, area_height, tile_width,
			tile_height);

	if (area_width == 0 || area_height == 0 || tile_width == 0 ||
			tile_height == 0)
		return 0;

	nbr_cols = area_width / tile_width;
	if (area_width % tile_width)
		nbr_cols++;
	nbr_rows = area_height / tile_height;
	if (area_height % tile_height)
		nbr_rows++;

	return nbr_cols * nbr_rows;
}

/**
 * rescale() - rescales the given dimension
 */
static inline s32 rescale(s32 dim, u16 sf)
{
	s32 tmp = dim << 10;

	tmp = tmp / sf;

	return tmp;
}

/**
 * inv_rescale() - does an inverted rescale of the given dimension
 */
static inline s32 inv_rescale(s32 dim, u16 sf)
{
	return (dim * (s32)sf) >> 10;
}

/**
 * is_upscale() - returns whether the given scale factors result in an overall
 *                upscaling (i.e. the destination area is larger than the
 *                source area).
 */
static inline bool is_upscale(u16 h_rsf, u16 v_rsf)
{
	u32 product;

	/* The product will be in 12.20 fixed point notation */
	product = (u32)h_rsf * (u32)v_rsf;

	/* If the product is smaller than 1, we have an overall upscale */
	return product < (1 << 20);
}

/**
 * calc_global_alpha() - recalculates the global alpha so that it contains the
 *                       per pixel alpha value as well.
 */
static inline u16 calc_global_alpha(enum b2r2_blt_fmt fmt, u32 color,
		u16 global_alpha)
{
	u16 per_pixel_alpha = 0xff;
	switch (fmt) {
	case B2R2_BLT_FMT_16_BIT_ARGB4444:
		per_pixel_alpha = (color & 0x0000ffff) >> 12;
		per_pixel_alpha *= 16;
		break;
	case B2R2_BLT_FMT_16_BIT_ARGB1555:
		per_pixel_alpha = (color & 0x0000ffff) >> 15;
		per_pixel_alpha *= 255;
		break;
	case B2R2_BLT_FMT_24_BIT_ARGB8565:
		per_pixel_alpha = (color & 0x00ffffff) >> 16;
		break;
	case B2R2_BLT_FMT_32_BIT_ARGB8888:
	case B2R2_BLT_FMT_32_BIT_ABGR8888:
	case B2R2_BLT_FMT_32_BIT_AYUV8888:
		per_pixel_alpha = (color >> 24) & 0x000000ff;
		break;
	default:
		break;
	}

	global_alpha = (u16)(((u32)per_pixel_alpha*(u32)global_alpha)/0xff);

	return global_alpha;
}

/**
 * set_target() - sets the target registers of the given node
 */
static void set_target(struct b2r2_node *node, u32 addr,
		struct b2r2_node_split_buf *buf)
{
	PTRACE_ENTRY();

	if (buf->tmp_buf_index) {
		pdebug(KERN_INFO LOG_TAG "::%s: target is tmp\n", __func__);
		node->dst_tmp_index = buf->tmp_buf_index;
	}

	node->node.GROUP1.B2R2_TBA = addr;
	node->node.GROUP1.B2R2_TTY = buf->pitch | to_native_fmt(buf->fmt) |
			buf->alpha_range | buf->hso | buf->vso;
	node->node.GROUP1.B2R2_TSZ =
			((buf->window.width & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
			((buf->window.height & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);
	node->node.GROUP1.B2R2_TXY =
			((buf->window.x & 0xffff) << B2R2_XY_X_SHIFT) |
			((buf->window.y & 0xffff) << B2R2_XY_Y_SHIFT);

	pdebug(KERN_INFO LOG_TAG "::%s: addr=%p, rect=(%d, %d, %d, %d), "
		"pitch=%d\n", __func__, (void *)addr, buf->window.x,
		buf->window.y, buf->window.width, buf->window.height,
		buf->pitch);
}

/**
 * set_src() - configures the given source register with the given values
 */
static void set_src(struct b2r2_src_config *src, u32 addr,
		struct b2r2_node_split_buf *buf)
{
	PTRACE_ENTRY();

	src->B2R2_SBA = addr;
	src->B2R2_STY = buf->pitch | to_native_fmt(buf->fmt) |
			buf->alpha_range | buf->hso | buf->vso;
	src->B2R2_SSZ = ((buf->window.width & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
			((buf->window.height & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);
	src->B2R2_SXY = ((buf->window.x & 0xffff) << B2R2_XY_X_SHIFT) |
			((buf->window.y & 0xffff) << B2R2_XY_Y_SHIFT);

}

/**
 * set_src_1() - sets the source 1 registers of the given node
 */
static void set_src_1(struct b2r2_node *node, u32 addr,
		struct b2r2_node_split_buf *buf)
{
	PTRACE_ENTRY();

	if (buf->tmp_buf_index) {
		pdebug(KERN_INFO LOG_TAG "::%s: src1 is tmp\n", __func__);
		node->src_tmp_index = buf->tmp_buf_index;
	}

	node->src_index = 1;

	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_1;
	node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_1_FETCH_FROM_MEM;

	pdebug(KERN_INFO LOG_TAG "::%s: addr=%p, rect=(%d, %d, %d, %d), "
		"pitch=%d\n", __func__, (void *)addr, buf->window.x,
		buf->window.y, buf->window.width, buf->window.height,
		buf->pitch);

	node->node.GROUP3.B2R2_SBA = addr;
	node->node.GROUP3.B2R2_STY = buf->pitch | to_native_fmt(buf->fmt) |
			buf->alpha_range | buf->hso | buf->vso;
	node->node.GROUP3.B2R2_SXY =
			((buf->window.x & 0xffff) << B2R2_XY_X_SHIFT) |
			((buf->window.y & 0xffff) << B2R2_XY_Y_SHIFT);

	/* Source 1 has no size register */
}

/**
 * set_src_2() - sets the source 2 registers of the given node
 */
static void set_src_2(struct b2r2_node *node, u32 addr,
		struct b2r2_node_split_buf *buf)
{
	PTRACE_ENTRY();

	if (buf->tmp_buf_index) {
		pdebug(KERN_INFO LOG_TAG "::%s: src2 is tmp\n", __func__);
		node->src_tmp_index = buf->tmp_buf_index;
	}

	node->src_index = 2;

	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_2;
	node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_2_FETCH_FROM_MEM;

	pdebug(KERN_INFO LOG_TAG "::%s: addr=%p, rect=(%d, %d, %d, %d), "
		"pitch=%d\n", __func__, (void *)addr, buf->window.x,
		buf->window.y, buf->window.width, buf->window.height,
		buf->pitch);

	set_src(&node->node.GROUP4, addr, buf);
}

/**
 * set_src_3() - sets the source 3 registers of the given node
 */
static void set_src_3(struct b2r2_node *node, u32 addr,
		struct b2r2_node_split_buf *buf)
{
	PTRACE_ENTRY();

	if (buf->tmp_buf_index) {
		pdebug(KERN_INFO LOG_TAG "::%s: src3 is tmp\n", __func__);
		node->src_tmp_index = buf->tmp_buf_index;
	}

	node->src_index = 3;

	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_3;
	node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_3_FETCH_FROM_MEM;

	pdebug(KERN_INFO LOG_TAG "::%s: addr=%p, rect=(%d, %d, %d, %d), "
		"pitch=%d\n", __func__, (void *)addr, buf->window.x,
		buf->window.y, buf->window.width, buf->window.height,
		buf->pitch);

	set_src(&node->node.GROUP5, addr, buf);
}

/**
 * set_ivmx() - configures the iVMX registers with the given values
 */
static void set_ivmx(struct b2r2_node *node, const u32 *vmx_values)
{
	PTRACE_ENTRY();

	node->node.GROUP0.B2R2_CIC |= 0xffff;
	node->node.GROUP0.B2R2_INS |= B2R2_INS_IVMX_ENABLED;

	node->node.GROUP15.B2R2_VMX0 = vmx_values[0];
	node->node.GROUP15.B2R2_VMX1 = vmx_values[1];
	node->node.GROUP15.B2R2_VMX2 = vmx_values[2];
	node->node.GROUP15.B2R2_VMX3 = vmx_values[3];
}

/**
 * reset_nodes() - clears the node list
 */
static void reset_nodes(struct b2r2_node *node)
{
	PTRACE_ENTRY();

	while (node != NULL) {
		memset(&node->node, 0, sizeof(node->node));

		/* TODO: Implement support for short linked lists */
		node->node.GROUP0.B2R2_CIC = 0x7ffff;

		if (node->next != NULL)
			node->node.GROUP0.B2R2_NIP =
					node->next->physical_address;
/*
		else
			node->node.GROUP0.B2R2_INS =
				B2R2_INS_BLITCOMPIRQ_ENABLED;
*/
		node = node->next;
	}
}

/**
 * dump_nodes() - prints the node list
 */
static void dump_nodes(struct b2r2_node *first)
{
	struct b2r2_node *node = first;
	while (node != NULL) {
		pverbose(KERN_INFO "\nNEW NODE:\n=============\n");
		pverbose(KERN_INFO "B2R2_ACK: \t%#010x\n",
				node->node.GROUP0.B2R2_ACK);
		pverbose(KERN_INFO "B2R2_INS: \t%#010x\n",
				node->node.GROUP0.B2R2_INS);
		pverbose(KERN_INFO "B2R2_CIC: \t%#010x\n",
				node->node.GROUP0.B2R2_CIC);
		pverbose(KERN_INFO "B2R2_NIP: \t%#010x\n",
				node->node.GROUP0.B2R2_NIP);

		pverbose(KERN_INFO "B2R2_TSZ: \t%#010x\n",
				node->node.GROUP1.B2R2_TSZ);
		pverbose(KERN_INFO "B2R2_TXY: \t%#010x\n",
				node->node.GROUP1.B2R2_TXY);
		pverbose(KERN_INFO "B2R2_TTY: \t%#010x\n",
				node->node.GROUP1.B2R2_TTY);
		pverbose(KERN_INFO "B2R2_TBA: \t%#010x\n",
				node->node.GROUP1.B2R2_TBA);

		pverbose(KERN_INFO "B2R2_S2CF: \t%#010x\n",
				node->node.GROUP2.B2R2_S2CF);
		pverbose(KERN_INFO "B2R2_S1CF: \t%#010x\n",
				node->node.GROUP2.B2R2_S1CF);

		pverbose(KERN_INFO "B2R2_S1SZ: \t%#010x\n",
				node->node.GROUP3.B2R2_SSZ);
		pverbose(KERN_INFO "B2R2_S1XY: \t%#010x\n",
				node->node.GROUP3.B2R2_SXY);
		pverbose(KERN_INFO "B2R2_S1TY: \t%#010x\n",
				node->node.GROUP3.B2R2_STY);
		pverbose(KERN_INFO "B2R2_S1BA: \t%#010x\n",
				node->node.GROUP3.B2R2_SBA);

		pverbose(KERN_INFO "B2R2_S2SZ: \t%#010x\n",
				node->node.GROUP4.B2R2_SSZ);
		pverbose(KERN_INFO "B2R2_S2XY: \t%#010x\n",
				node->node.GROUP4.B2R2_SXY);
		pverbose(KERN_INFO "B2R2_S2TY: \t%#010x\n",
				node->node.GROUP4.B2R2_STY);
		pverbose(KERN_INFO "B2R2_S2BA: \t%#010x\n",
				node->node.GROUP4.B2R2_SBA);

		pverbose(KERN_INFO "B2R2_S3SZ: \t%#010x\n",
				node->node.GROUP5.B2R2_SSZ);
		pverbose(KERN_INFO "B2R2_S3XY: \t%#010x\n",
				node->node.GROUP5.B2R2_SXY);
		pverbose(KERN_INFO "B2R2_S3TY: \t%#010x\n",
				node->node.GROUP5.B2R2_STY);
		pverbose(KERN_INFO "B2R2_S3BA: \t%#010x\n",
				node->node.GROUP5.B2R2_SBA);

		pverbose(KERN_INFO "B2R2_CWS: \t%#010x\n",
				node->node.GROUP6.B2R2_CWS);
		pverbose(KERN_INFO "B2R2_CWO: \t%#010x\n",
				node->node.GROUP6.B2R2_CWO);

		pverbose(KERN_INFO "B2R2_FCTL: \t%#010x\n",
				node->node.GROUP8.B2R2_FCTL);
		pverbose(KERN_INFO "B2R2_RSF: \t%#010x\n",
				node->node.GROUP9.B2R2_RSF);
		pverbose(KERN_INFO "B2R2_RZI: \t%#010x\n",
				node->node.GROUP9.B2R2_RZI);
		pverbose(KERN_INFO "B2R2_LUMA_RSF: \t%#010x\n",
				node->node.GROUP10.B2R2_RSF);
		pverbose(KERN_INFO "B2R2_LUMA_RZI: \t%#010x\n",
				node->node.GROUP10.B2R2_RZI);


		pverbose(KERN_INFO "B2R2_IVMX0: \t%#010x\n",
				node->node.GROUP15.B2R2_VMX0);
		pverbose(KERN_INFO "B2R2_IVMX1: \t%#010x\n",
				node->node.GROUP15.B2R2_VMX1);
		pverbose(KERN_INFO "B2R2_IVMX2: \t%#010x\n",
				node->node.GROUP15.B2R2_VMX2);
		pverbose(KERN_INFO "B2R2_IVMX3: \t%#010x\n",
				node->node.GROUP15.B2R2_VMX3);

		node = node->next;
	}
}

#ifdef CONFIG_DEBUG_FS
static struct dentry *dir;
static struct dentry *debug_file;
static struct dentry *verbose_file;

static int init_debugfs(void)
{
	dir = debugfs_create_dir("b2r2_node_split", NULL);
	if (dir == NULL)
		goto error;

	debug_file = debugfs_create_bool("debug", 644, dir, &debug);
	if (debug_file == NULL)
		goto error_free_dir;

	verbose_file = debugfs_create_bool("verbose", 644, dir, &verbose);
	if (verbose_file == NULL)
		goto error_free_debug;

	return 0;

error_free_debug:
	debugfs_remove(debug_file);
error_free_dir:
	debugfs_remove(dir);
error:
	return -1;
}

static void exit_debugfs(void)
{
	if (verbose_file != NULL)
		debugfs_remove(verbose_file);
	if (debug_file != NULL)
		debugfs_remove(debug_file);
	if (dir != NULL)
		debugfs_remove(dir);

	verbose_file = NULL;
	debug_file = NULL;
	dir = NULL;
}

#else
static inline int init_debugfs(void) { return 0; }
static inline void exit_debugfs(void) {}
#endif

int b2r2_node_split_init(void)
{
	return init_debugfs();
}

void b2r2_node_split_exit(void)
{
	exit_debugfs();
}
