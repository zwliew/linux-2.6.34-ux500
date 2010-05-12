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
#include <linux/dma-mapping.h>
#include <asm/dma-mapping.h>


#include "b2r2_node_split.h"
#include "b2r2_internal.h"
#include "b2r2_hw.h"

/******************
 * Debug printing
 ******************/
static u32 debug;
static u32 verbose;

static u32 hf_coeffs_addr = 0;
static u32 vf_coeffs_addr = 0;
static void *hf_coeffs = NULL;
static void *vf_coeffs = NULL;
#define HF_TABLE_SIZE 64
#define VF_TABLE_SIZE 40

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

#define PTRACE_ENTRY() pverbose(KERN_INFO LOG_TAG "::%s\n", __func__)

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

/* TODO: Verify iVMX values for BGR <--> YUV */
static const u32 vmx_bgr_to_yuv[] = {
	B2R2_VMX0_BGR_TO_YUV_601_VIDEO,
	B2R2_VMX1_BGR_TO_YUV_601_VIDEO,
	B2R2_VMX2_BGR_TO_YUV_601_VIDEO,
	B2R2_VMX3_BGR_TO_YUV_601_VIDEO,
};

static const u32 vmx_yuv_to_bgr[] = {
	B2R2_VMX0_YUV_TO_BGR_601_VIDEO,
	B2R2_VMX1_YUV_TO_BGR_601_VIDEO,
	B2R2_VMX2_YUV_TO_BGR_601_VIDEO,
	B2R2_VMX3_YUV_TO_BGR_601_VIDEO,
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
static int rot_node_count(s32 width, s32 height);

static void configure_src(struct b2r2_node *node,
		struct b2r2_node_split_buf *src, const u32 *ivmx);
static int configure_dst(struct b2r2_node *node,
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
static int configure_fill(struct b2r2_node *node, u32 color,
		enum b2r2_blt_fmt fmt, struct b2r2_node_split_buf *dst,
		const u32 *ivmx, struct b2r2_node **next);
static void configure_direct_copy(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, struct b2r2_node **next);
static int configure_copy(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, const u32 *ivmx,
		struct b2r2_node **next);
static int configure_rotate(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, const u32 *ivmx,
		struct b2r2_node **next);
static int configure_scale(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, u16 h_rsf, u16 v_rsf,
		const u32 *ivmx, struct b2r2_node **next);
static void configure_tmp_buf(struct b2r2_node_split_buf *this, int index,
		struct b2r2_blt_rect *window);
static int configure_rot_scale(struct b2r2_node_split_job *this,
		struct b2r2_node *node, struct b2r2_node **next);

static int check_rect(const struct b2r2_blt_img *img,
		const struct b2r2_blt_rect *rect,
		const struct b2r2_blt_rect *clip);
static void set_buf(struct b2r2_node_split_buf *buf, u32 addr,
		const struct b2r2_blt_img *img,
		const struct b2r2_blt_rect *rect, bool color_fill, u32 color);
static int constrain_window(struct b2r2_blt_rect *window,
		enum b2r2_blt_fmt fmt, u32 max_size);
static int setup_tmp_buf(struct b2r2_node_split_buf *this, u32 max_size,
		enum b2r2_blt_fmt pref_fmt, u32 pref_width, u32 pref_height);
static int calculate_tile_count(s32 area_width, s32 area_height, s32 tile_width,
		s32 tile_height);

static inline enum b2r2_ty get_alpha_range(enum b2r2_blt_fmt fmt);
static inline u32 set_alpha(enum b2r2_blt_fmt fmt, u8 alpha, u32 color);
static inline u8  get_alpha(enum b2r2_blt_fmt fmt, u32 pixel);
static inline bool fmt_has_alpha(enum b2r2_blt_fmt fmt);
static inline bool is_rgb_fmt(enum b2r2_blt_fmt fmt);
static inline bool is_bgr_fmt(enum b2r2_blt_fmt fmt);
static inline bool is_yuv_fmt(enum b2r2_blt_fmt fmt);
static inline bool is_yuv420_fmt(enum b2r2_blt_fmt fmt);
static inline int fmt_byte_pitch(enum b2r2_blt_fmt fmt, u32 width);
static inline int fmt_bpp(enum b2r2_blt_fmt fmt);
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

	/* Configure the source and destination buffers */
	set_buf(&this->src, req->src_resolved.physical_address,
			&req->user_req.src_img, &req->user_req.src_rect,
			color_fill, req->user_req.src_color);

	set_buf(&this->dst, req->dst_resolved.physical_address,
			&req->user_req.dst_img, &req->user_req.dst_rect, false,
			0);

	pdebug(KERN_INFO LOG_TAG "::%s:\n"
		"\t\tsrc.rect=(%4d, %4d, %4d, %4d)\t"
		"dst.rect=(%4d, %4d, %4d, %4d)\n", __func__, this->src.rect.x,
		this->src.rect.y, this->src.rect.width, this->src.rect.height,
		this->dst.rect.x, this->dst.rect.y, this->dst.rect.width,
		this->dst.rect.height);

	/* Check for blending */
	if (this->flags & B2R2_BLT_FLAG_GLOBAL_ALPHA_BLEND)
		this->blend = true;
	else if (this->flags & B2R2_BLT_FLAG_PER_PIXEL_ALPHA_BLEND)
		this->blend = fmt_has_alpha(this->src.fmt) ||
				fmt_has_alpha(this->dst.fmt);

	/* Check for clipping */
	this->clip = (this->flags &
				B2R2_BLT_FLAG_DESTINATION_CLIP) != 0;
	if (this->clip) {
		s32 l = req->user_req.dst_clip_rect.x;
		s32 r = l + req->user_req.dst_clip_rect.width;
		s32 t = req->user_req.dst_clip_rect.y;
		s32 b = t + req->user_req.dst_clip_rect.height;

		/* Intersect the clip and buffer rects */
		if (l < 0)
			l = 0;
		if (r > req->user_req.dst_img.width)
			r = req->user_req.dst_img.width;
		if (t < 0)
			t = 0;
		if (b > req->user_req.dst_img.height)
			b = req->user_req.dst_img.height;

		this->clip_rect.x = l;
		this->clip_rect.y = t;
		this->clip_rect.width = r - l;
		this->clip_rect.height = b - t;
	} else {
		/* Set the clip rectangle to the buffer bounds */
		this->clip_rect.x = 0;
		this->clip_rect.y = 0;
		this->clip_rect.width = req->user_req.dst_img.width;
		this->clip_rect.height = req->user_req.dst_img.height;
	}

	/* The clip rectangle is "inclusive" so we have to subtract one
	   from the width and height */
	this->clip_rect.width -= 1;
	this->clip_rect.height -= 1;

	/* Validate the destination */
	ret = check_rect(&req->user_req.dst_img, &req->user_req.dst_rect,
			&this->clip_rect);
	if (ret < 0)
		goto error;

	/* Validate the source (if not color fill) */
	if (!color_fill) {
		ret = check_rect(&req->user_req.src_img,
					&req->user_req.src_rect, NULL);
		if (ret < 0)
			goto error;
	}

	/* YUV <==> BGR is unsupported
	   TODO: Implement support */
	if (!color_fill) {
		if ((is_yuv_fmt(this->src.fmt) &&
					is_bgr_fmt(this->dst.fmt)) ||
				(is_bgr_fmt(this->src.fmt) &&
					is_yuv_fmt(this->dst.fmt))) {
			printk(KERN_DEBUG LOG_TAG "::%s: "
				"Unsupported operation %s\n",
				"YUV <==> BGR", __func__);
			ret = -ENOSYS;
			goto unsupported;
		}
	}

	/* Do the analysis depending on the type of operation */
	if (color_fill) {
		ret = analyze_color_fill(this, req, &this->node_count);
	} else {

		if (is_transform(req) ||
				(this->src.type == B2R2_FMT_TYPE_SEMI_PLANAR) ||
				(this->src.type == B2R2_FMT_TYPE_PLANAR) ||
				(this->dst.type == B2R2_FMT_TYPE_SEMI_PLANAR) ||
				(this->dst.type == B2R2_FMT_TYPE_PLANAR)) {
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

	pverbose(KERN_INFO "%s: src=%d dst=%d\n"
			"\t\tsrc.rect=(%4d, %4d, %4d, %4d)\td_src=(%4d, %4d)\n"
			"\t\tdst.rect=(%4d, %4d, %4d, %4d)\td_dst=(%4d, %4d)\n",
			__func__, req->user_req.src_img.buf.fd,
			req->user_req.dst_img.buf.fd,
			this->src.rect.x, this->src.rect.y,
			this->src.rect.width, this->src.rect.height,
			this->src.dx, this->src.dy,
			this->dst.rect.x, this->dst.rect.y,
			this->dst.rect.width, this->dst.rect.height,
			this->dst.dx, this->dst.dy);
	pverbose(KERN_INFO "%s:\n"
			"\t\tsrc.window=(%4d, %4d, %4d, %4d)\td_src=(%d, %d)\n"
			"\t\tdst.window=(%4d, %4d, %4d, %4d)\td_dst=(%d, %d)\n",
			__func__, this->src.window.x, this->src.window.y,
			this->src.window.width, this->src.window.height,
			this->src.dx, this->src.dy,
			this->dst.window.x, this->dst.window.y,
			this->dst.window.width, this->dst.window.height,
			this->dst.dx, this->dst.dy);

	pverbose(KERN_INFO "\t\t\tnode_count=%d, buf_count=%d, type=%s\n",
			*node_count, *buf_count, type2str(this->type));
	pverbose(KERN_INFO "\t\t\tblend=%d, clip=%d, color_fill=%d\n",
			this->blend, this->clip, color_fill);

	return 0;

error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
unsupported:
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

	s32 src_win_width;
	s32 src_win_height;
	s32 dst_win_width;
	s32 dst_win_height;

	pverbose(KERN_INFO "%s: src_rect=(%d, %d, %d, %d), "
			"dst_rect=(%d, %d, %d, %d)\n", __func__,
			this->src.rect.x, this->src.rect.y,
			this->src.rect.width, this->src.rect.height,
			this->dst.rect.x, this->dst.rect.y,
			this->dst.rect.width,
			this->dst.rect.height);

	pverbose(KERN_INFO "%s: d_src=(%d, %d) d_dst=(%d, %d)\n",
		__func__, this->src.dx, this->src.dy, this->dst.dx,
		this->dst.dy);

	reset_nodes(node);

	/* Save these so we can do restore them when moving to a new row */
	src_win_width = this->src.window.width;
	src_win_height = this->src.window.height;
	dst_win_width = this->dst.window.width;
	dst_win_height = this->dst.window.height;

	do {
		if (node == NULL) {
			/* DOH! This is an internal error (to few nodes
			   allocated) */
			printk(KERN_ERR LOG_TAG "%s: "
				"Internal error! Out of nodes!\n",
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
			pverbose(KERN_INFO "%s: last col\n", __func__);
			this->src.window.width = this->src.rect.width -
				ABS(this->src.rect.x - this->src.window.x);
			if (this->src.window.width != src_win_width) { /* To not introduce precision issues on "normal" tiles */
				s32 new_dst_width =
					this->horiz_rescale ? rescale(this->src.window.width, this->horiz_sf) : this->src.window.width;
				if (this->rotation)
					this->dst.window.height = new_dst_width;
				else
					this->dst.window.width = new_dst_width;
			}
		}

		if (last_row) {
			pverbose(KERN_INFO "%s: last row\n", __func__);
			this->src.window.height = this->src.rect.height -
				ABS(this->src.rect.y - this->src.window.y);
			if (this->src.window.height != src_win_height) { /* To not introduce precision issues on "normal" tiles */
				s32 new_dst_height =
					this->vert_rescale ? rescale(this->src.window.height, this->vert_sf) : this->src.window.height;
				if (this->rotation)
					this->dst.window.width = new_dst_height;
				else
					this->dst.window.height = new_dst_height;
			}
		}

		pverbose(KERN_INFO "%s: src=(%d, %d, %d, %d), "
				"dst=(%d, %d, %d, %d)\n", __func__,
				this->src.window.x, this->src.window.y,
				this->src.window.width, this->src.window.height,
				this->dst.window.x, this->dst.window.y,
				this->dst.window.width,
				this->dst.window.height);

		ret = configure_tile(this, node, &last);
		if (ret < 0)
			goto error;

		this->src.window.x += this->src.dx;
		if (this->rotation)
			this->dst.window.y += this->dst.dy;
		else
			this->dst.window.x += this->dst.dx;

		if (last_col) {
			/* Reset the window and slide vertically */
			this->src.window.x = this->src.rect.x;
			this->src.window.width = src_win_width;

			this->src.window.y += this->src.dy;

			if (this->rotation) {
				this->dst.window.y = this->dst.rect.y;
				this->dst.window.height = dst_win_height;

				this->dst.window.x += this->dst.dx;
			} else {
				this->dst.window.x = this->dst.rect.x;
				this->dst.window.width = dst_win_width;

				this->dst.window.y += this->dst.dy;
			}
		}

		node = last;

	} while (!(last_row && last_col));

	return 0;
error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);

	{
		int nbr_nodes = 0;
		while (first != NULL) {
			nbr_nodes++;
			first = first->next;
		}

		printk(KERN_ERR LOG_TAG "::%s: Asked for %d nodes, got %d\n",
			__func__, this->node_count, nbr_nodes);
		printk(KERN_ERR LOG_TAG "::%s: src.rect=(%4d, %4d, %4d, %4d)\t"
			"dst.rect=(%4d, %4d, %4d, %4d)\n", __func__,
			this->src.rect.x, this->src.rect.y,
			this->src.rect.width, this->src.rect.height,
			this->dst.rect.x, this->dst.rect.y,
			this->dst.rect.width, this->dst.rect.height);
	}

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
			u32 addr = bufs[node->dst_tmp_index - 1].phys_addr;

			BUG_ON(node->dst_tmp_index > buf_count);

			node->node.GROUP1.B2R2_TBA =
				bufs[node->dst_tmp_index - 1].phys_addr;
			pverbose(KERN_INFO LOG_TAG "::%s: "
				"%p Assigning %p as destination\n", __func__,
				node, (void *)addr);
		}
		if (node->src_tmp_index) {
			u32 addr = bufs[node->src_tmp_index - 1].phys_addr;

			pverbose(KERN_INFO LOG_TAG "::%s: "
				"%p Assigning %p as source", __func__,
				node, (void *)addr);

			BUG_ON(node->src_tmp_index > buf_count);

			switch (node->src_index) {
			case 1:
				pverbose("1\n");
				node->node.GROUP3.B2R2_SBA = addr;
				break;
			case 2:
				pverbose("2\n");
				node->node.GROUP4.B2R2_SBA = addr;
				break;
			case 3:
				pverbose("3\n");
				node->node.GROUP5.B2R2_SBA = addr;
				break;
			default:
				BUG_ON(1);
				break;
			}
		}

		pverbose(KERN_INFO LOG_TAG "::%s: tba=%p\tsba=%p\n", __func__,
			(void *)node->node.GROUP1.B2R2_TBA,
			(void *)node->node.GROUP4.B2R2_SBA);

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
	pverbose("%s: Releasing job\n", __func__);

	memset(this, 0, sizeof(*this));

	return;
}

/**********************
 * Private functions
 **********************/

static int check_rect(const struct b2r2_blt_img *img,
		const struct b2r2_blt_rect *rect,
		const struct b2r2_blt_rect *clip)
{
	int ret;

	s32 l, r, b, t;

	/* Check rectangle dimensions*/
	if ((rect->width <= 0) || (rect->height <= 0)) {
		printk(KERN_ERR LOG_TAG "::%s: "
			"Illegal rect (%d, %d, %d, %d)\n",
			__func__, rect->x,rect->y, rect->width, rect->height);
		ret = -EINVAL;
		goto error;
	}

	/* If we are using clip we should only look at the intersection of the
	   rects */
	if (clip) {
		l = MAX(rect->x, clip->x);
		t = MAX(rect->y, clip->y);
		r = MIN(rect->x + rect->width, clip->x + clip->width);
		b = MIN(rect->y + rect->height, clip->y + clip->height);
	} else {
		l = rect->x;
		t = rect->y;
		r = rect->x + rect->width;
		b = rect->y + rect->height;
	}

	/* Check so that the rect isn't outside the buffer */
	if ((l < 0) || (t < 0)) {
		printk(KERN_ERR LOG_TAG "::%s: "
			"rect origin outside buffer\n", __func__);
		ret = -EINVAL;
		goto error;
	}

	if ((r > img->width) ||	(t > img->height)) {
		printk(KERN_ERR LOG_TAG "::%s: "
			"rect ends outside buffer\n", __func__);
		ret = -EINVAL;
		goto error;
	}

	return 0;
error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
	return ret;
}

/**
 * analyze_fmt_conv() - analyze the format conversions needed for a job
 */
static int analyze_fmt_conv(struct b2r2_node_split_job *this,
		const struct b2r2_blt_request *req, u32 *node_count)
{
	pverbose(KERN_INFO LOG_TAG "::%s: Analyzing fmt conv...\n", __func__);

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
		pverbose(KERN_INFO LOG_TAG "::%s: --> RASTER\n", __func__);
		*node_count = 1;
	} else if (this->dst.type == B2R2_FMT_TYPE_SEMI_PLANAR) {
		pverbose(KERN_INFO LOG_TAG "::%s: --> SEMI PLANAR\n", __func__);
		*node_count = 2;
	} else if (this->dst.type == B2R2_FMT_TYPE_PLANAR) {
		pverbose(KERN_INFO LOG_TAG "::%s: --> PLANAR\n", __func__);
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
	pverbose(KERN_INFO LOG_TAG "::%s: Analyzing color fill...\n", __func__);

	/* Destination must be raster for raw fill to work */
	if ((this->flags & B2R2_BLT_FLAG_SOURCE_FILL_RAW) &&
			(this->dst.type != B2R2_FMT_TYPE_RASTER)) {
		printk(KERN_ERR "%s: Raw fill requires raster destination\n",
			__func__);
		ret = -EINVAL;
		goto error;
	}

	/* We will try to fill the entire rectangle in one go */
	memcpy(&this->dst.window, &this->dst.rect, sizeof(this->dst.window));

	/* Check if this is a direct fill */
	if ((!this->blend) && ((this->flags & B2R2_BLT_FLAG_SOURCE_FILL_RAW) ||
			(this->dst.fmt == B2R2_BLT_FMT_32_BIT_ARGB8888) ||
			(this->dst.fmt == B2R2_BLT_FMT_32_BIT_ABGR8888) ||
			(this->dst.fmt == B2R2_BLT_FMT_32_BIT_AYUV8888))) {
		pverbose(KERN_INFO LOG_TAG "::%s: Direct fill!\n", __func__);
		this->type = B2R2_DIRECT_FILL;

		/* The color format will be the same as the dst fmt */
		this->src.fmt = this->dst.fmt;

		/* The entire destination rectangle will be  */
		memcpy(&this->dst.window, &this->dst.rect,
			sizeof(this->dst.window));
		*node_count = 1;
	} else {
		pverbose(KERN_INFO LOG_TAG "::%s: Normal fill!\n", __func__);
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
				/* Color will still be ARGB, we will translate
				   using IVMX (configured later) */
				this->src.fmt =	B2R2_BLT_FMT_32_BIT_ARGB8888;
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

		/* Also, B2R2 seems to ignore the pixel alpha value */
		if (((this->flags & B2R2_BLT_FLAG_PER_PIXEL_ALPHA_BLEND)
					!= 0) &&
				((this->flags & B2R2_BLT_FLAG_SOURCE_FILL_RAW)
					== 0) && fmt_has_alpha(this->src.fmt)) {
			u8 pixel_alpha = get_alpha(this->src.fmt,
							this->src.color);
			u32 new_global = pixel_alpha * this->global_alpha / 255;

			this->global_alpha = (u8)new_global;

			/* Set the pixel alpha to full opaque so we don't get
			   any nasty suprises */
			this->src.color = set_alpha(this->src.fmt, 0xFF,
						this->src.color);
		}

		ret = analyze_fmt_conv(this, req, node_count);
		if (ret < 0)
			goto error;
	}

	this->dst.dx = this->dst.window.width;
	this->dst.dy = this->dst.window.height;

	return 0;

error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
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

	/* The transform enum is defined so that all rotation transforms are
	   masked with the rotation flag */
	this->rotation = (this->transform & B2R2_BLT_TRANSFORM_CCW_ROT_90) != 0;

	pverbose(KERN_INFO LOG_TAG "::%s: Analyzing transform...\n", __func__);

	/* Check for scaling */
	if (this->rotation) {
		is_scaling = (this->src.rect.width != this->dst.rect.height) ||
			(this->src.rect.height != this->dst.rect.width);
	} else {
		is_scaling = (this->src.rect.width != this->dst.rect.width) ||
			(this->src.rect.height != this->dst.rect.height);
	}

	/* Plane separated formats must be treated as scaling */
	is_scaling = is_scaling ||
			(this->src.type == B2R2_FMT_TYPE_SEMI_PLANAR) ||
			(this->src.type == B2R2_FMT_TYPE_PLANAR) ||
			(this->dst.type == B2R2_FMT_TYPE_SEMI_PLANAR) ||
			(this->dst.type == B2R2_FMT_TYPE_PLANAR);

	if (is_scaling && this->rotation && this->blend) {
		/* TODO: This is unsupported. Fix it! */
		printk(KERN_DEBUG LOG_TAG "::%s: Unsupported operation\n",
			__func__);
		ret = -ENOSYS;
		goto unsupported;
	}

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

	/* Modify the horizontal and vertical scan order if FLIP (the enum
	   values are set up so all flip combination values are masked with a
	   flip flag) */
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
	if (this->dst.hso == B2R2_TY_HSO_RIGHT_TO_LEFT)
		this->dst.dx = -this->dst.window.width;
	else
		this->dst.dx = this->dst.window.width;
	if (this->dst.vso == B2R2_TY_VSO_BOTTOM_TO_TOP)
		this->dst.dy = -this->dst.window.height;
	else
		this->dst.dy = this->dst.window.height;

	return 0;

error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
unsupported:
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

	pverbose(KERN_INFO LOG_TAG "::%s: Analyzing copy...\n", __func__);

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
			setup_tmp_buf(&this->tmp_bufs[0], this->max_buf_size,
				this->dst.fmt, this->dst.rect.width,
				this->dst.rect.height);

			this->work_bufs[0].size = this->tmp_bufs[0].pitch *
					this->tmp_bufs[0].height;

			ret = constrain_window(&this->src.window,
					this->tmp_bufs[0].fmt,
					this->work_bufs[0].size);
			if (ret < 0)
				goto error;

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
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
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

	struct b2r2_node_split_buf *tmp;

	s32 tmp_width;
	s32 tmp_height;

	/* We will always need one tmp buffer */
	this->buf_count = 1;
	tmp = &this->tmp_bufs[0];

	ret = analyze_scale_factors(this);
	if (ret < 0)
		goto error;

	/* All rotations are written from the bottom up */
	this->dst.vso = B2R2_TY_VSO_BOTTOM_TO_TOP;
	this->dst.rect.y += this->dst.rect.height - 1;

	/* We will try to to the entire rectangle at once */
	memcpy(&this->src.window, &this->src.rect, sizeof(this->src.window));
	memcpy(&this->dst.window, &this->dst.rect, sizeof(this->dst.window));

	/* The source width will be limited by the rescale max width */
	this->src.window.width = MIN(this->src.window.width,
			B2R2_RESCALE_MAX_WIDTH);

	tmp_width = rescale(this->src.window.width, this->horiz_sf);
	tmp_height = rescale(this->src.window.height, this->vert_sf);

	/* Setup the tmp buf from the source window */
	ret = setup_tmp_buf(tmp, this->max_buf_size, this->dst.fmt, tmp_width,
			tmp_height);
	if (ret < 0)
		goto error;

	pdebug(KERN_INFO LOG_TAG "::%s: tmp_width=%d\ttmp_height=%d\n",
		__func__, tmp_width, tmp_height);
	pdebug(KERN_INFO LOG_TAG "::%s: "
		"tmp.rect.width=%d\ttmp.rect.height=%d\n", __func__,
		tmp->rect.width, tmp->rect.height);
	pdebug(KERN_INFO LOG_TAG "::%s: hsf=%d\tvsf=%d\n",
		__func__, this->horiz_sf, this->vert_sf);

	this->work_bufs[0].size = tmp->pitch * tmp->height;
	tmp->tmp_buf_index = 1;

	/* If the entire source window couldn't fit into the tmp buf we need
	   to modify it */
	if (tmp_width != tmp->rect.width)
		this->src.window.width = inv_rescale(tmp->rect.width,
			this->horiz_sf);
	if (tmp_height != tmp->rect.height)
		this->src.window.height = inv_rescale(tmp->rect.height,
			this->vert_sf);

	/* The source window will slide normally */
	this->src.dx = this->src.window.width;
	this->src.dy = this->src.window.height;

	/* Setup the destination window (also make sure it is never larger
	   than the rectangle) */
	this->dst.window.width = MIN(this->dst.rect.width, tmp->rect.height);
	this->dst.window.height = MIN(this->dst.rect.height, tmp->rect.width);

	/* The destination will slide upwards because of the rotation */
	this->dst.dx = this->dst.window.width;
	this->dst.dy = -this->dst.window.height;

	/* Calculate the number of nodes needed */
	ret = calculate_rot_scale_node_count(this, req);
	if (ret < 0)
		goto error;

	this->type = B2R2_SCALE_AND_ROTATE;

	pdebug(KERN_INFO LOG_TAG "::%s: Rot scale:\n", __func__);
	pdebug(KERN_INFO LOG_TAG "::%s: node_count=%d\n", __func__,
		this->node_count);

	return 0;

error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
	return ret;
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
	u32 tile_rots;
	u32 scale_count;

	s32 src_right_width;
	s32 src_bottom_height;
	s32 dst_right_width;
	s32 dst_bottom_height;

	int nbr_full_rows;
	int nbr_full_cols;

	/* First check how many nodes we need for a simple copy */
	ret = analyze_fmt_conv(this, req, &copy_count);
	if (ret < 0)
		goto error;

	/* The number of rotation nodes per tile will vary depending on whether
	   it is an "inner" tile or an "edge" tile, since they may have
	   different sizes. */
	nbr_full_cols = this->src.rect.width / this->src.window.width;
	nbr_full_rows = this->src.rect.height / this->src.window.height;

	src_right_width = this->src.rect.width % this->src.window.width;
	if (src_right_width)
		dst_right_width =
			this->horiz_rescale ? rescale(src_right_width, this->horiz_sf) : src_right_width;
	else
		dst_right_width = 0;

	src_bottom_height = this->src.rect.height % this->src.window.height;
	if (src_bottom_height)
		dst_bottom_height =
			this->horiz_rescale ? rescale(src_bottom_height, this->vert_sf) : src_bottom_height;
	else
		dst_bottom_height = 0;

	pdebug(KERN_INFO "dst.window=\t(%4d, %4d, %4d, %4d)\n",
		this->dst.window.x, this->dst.window.y, this->dst.window.width,
		this->dst.window.height);
	pdebug(KERN_INFO "src.window=\t(%4d, %4d, %4d, %4d)\n",
		this->src.window.x, this->src.window.y, this->src.window.width,
		this->src.window.height);
	pdebug(KERN_INFO "right_width=%d, bottom_height=%d\n", dst_right_width,
		dst_bottom_height);

	/* Update the rot_count and scale_count with all the "inner" tiles */
	tile_rots = rot_node_count(this->dst.window.height,
				this->dst.window.width);
	rot_count = tile_rots * nbr_full_cols * nbr_full_rows;
	scale_count = nbr_full_cols * nbr_full_rows;

	pdebug(KERN_INFO LOG_TAG "::%s: inner=%d\n", __func__,
		tile_rots);

	/* Update with "right tile" rotations (one tile per row) */
	if (dst_right_width) {
		tile_rots = rot_node_count(dst_right_width, this->dst.window.width);
		rot_count += tile_rots * nbr_full_rows;
		scale_count += nbr_full_rows;

		pdebug(KERN_INFO LOG_TAG "::%s: right=%d\n", __func__,
			tile_rots);
	}

	/* Update with "bottom tile" rotations (one tile per col) */
	if (dst_bottom_height) {
		tile_rots = rot_node_count(this->dst.window.height,
			dst_bottom_height);
		rot_count += tile_rots * nbr_full_cols;
		scale_count += nbr_full_cols;

		pdebug(KERN_INFO LOG_TAG "::%s: bottom=%d\n", __func__,
			tile_rots);
	}

	/* Update with "bottom right tile" rotations (ever only one tile) */
	if (dst_right_width && dst_bottom_height) {
		tile_rots = rot_node_count(dst_right_width, dst_bottom_height);
		rot_count += tile_rots;
		scale_count++;

		pdebug(KERN_INFO LOG_TAG "::%s: bottom_right=%d\n", __func__,
			tile_rots);
	}

	/* Finally calculate the total node count */
	this->node_count = (scale_count + rot_count) * copy_count;

	pdebug(KERN_INFO LOG_TAG "::%s: nbr_full_cols=%d, nbr_full_rows=%d\n",
		__func__, nbr_full_cols, nbr_full_rows);

	pdebug(KERN_INFO LOG_TAG "::%s: node_count=%d\n", __func__,
		this->node_count);

	return 0;
error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
	return ret;
}

static int rot_node_count(s32 width, s32 height)
{
	u32 node_count;

	node_count = width / B2R2_ROTATE_MAX_WIDTH;
	if (width % B2R2_ROTATE_MAX_WIDTH)
		node_count++;
#ifdef B2R2_ROTATION_HEIGHT_BUGFIX
	if ((height > 16) && (height % 16))
		node_count *= 2;
#endif

	return node_count;
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

	pverbose(KERN_INFO LOG_TAG "::%s: Analyzing scaling...\n", __func__);

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

		ret = setup_tmp_buf(&this->tmp_bufs[0], this->max_buf_size,
			this->dst.fmt, this->dst.rect.width,
			this->dst.rect.height);
		if (ret < 0)
			goto error;
		this->work_bufs[0].size = this->tmp_bufs[0].pitch *
			this->tmp_bufs[0].height;
		this->tmp_bufs[0].tmp_buf_index = 1;

		ret = constrain_window(&this->dst.window, this->tmp_bufs[0].fmt,
					this->work_bufs[0].size);
		if (ret < 0)
			goto error;

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
	}

	nbr_cols = this->src.rect.width / this->src.window.width;
	if (this->src.rect.width % this->src.window.width)
		nbr_cols++;

	nbr_rows = this->src.rect.height / this->src.window.height;
	if (this->src.rect.height % this->src.window.height)
		nbr_rows++;

	pverbose(KERN_INFO LOG_TAG "::%s: Each stripe will require %d nodes\n",
			__func__, copy_count);

	*node_count = copy_count * nbr_rows * nbr_cols;

	this->type = B2R2_SCALE;

	return 0;

error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
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

	pverbose(KERN_INFO LOG_TAG "::%s: Analyzing rotation...\n", __func__);

	/* Find out how many nodes a simple copy would require */
	ret = analyze_fmt_conv(this, req, &copy_count);
	if (ret < 0)
		goto error;

	this->type = B2R2_ROTATE;

	/* Blending cannot be combined with rotation */
	if (this->blend)
		this->buf_count = 1;

	/* The vertical scan order of the destination must be flipped */
	if (this->dst.vso == B2R2_TY_VSO_TOP_TO_BOTTOM) {
		this->dst.rect.y += (this->dst.rect.height - 1);
		this->dst.vso = B2R2_TY_VSO_BOTTOM_TO_TOP;
	} else {
		this->dst.rect.y -= (this->dst.rect.height - 1);
		this->dst.vso = B2R2_TY_VSO_TOP_TO_BOTTOM;
	}

	memcpy(&this->src.window, &this->src.rect, sizeof(this->src.window));
	memcpy(&this->dst.window, &this->dst.rect, sizeof(this->dst.window));

	this->src.window.width = B2R2_ROTATE_MAX_WIDTH;

	if (this->buf_count > 0) {
		struct b2r2_node_split_buf *tmp = &this->tmp_bufs[0];

		/* One more node to copy to the tmp buf */
		copy_count++;

		ret = setup_tmp_buf(tmp, this->max_buf_size, this->dst.fmt,
			this->src.window.height, this->src.window.width);
		if (ret < 0)
			goto error;

		this->work_bufs[0].size = tmp->pitch * tmp->height;
		tmp->tmp_buf_index = 1;

		/* The temporary buffer will be written bottom up */
		tmp->vso = B2R2_TY_VSO_BOTTOM_TO_TOP;

		if (this->src.window.width > tmp->rect.height)
			this->src.window.width = tmp->rect.height;
		if (this->src.window.height > tmp->rect.width)
			this->src.window.height = tmp->rect.width;
	}

#ifdef B2R2_ROTATION_HEIGHT_BUGFIX
	/*
	 * Because of a possible bug in the hardware, B2R2 cannot handle
	 * stripes that are not a multiple of 16 high. We need one extra node
	 * per stripe.
	 */
	if ((this->src.window.height > 16) && (this->src.window.height % 16))
		this->src.window.height -= this->src.window.height % 16;
#endif

	/* Set the size of the destination window */
	this->dst.window.width = this->src.window.height;
	this->dst.window.height = this->src.window.width;

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
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
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
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
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
		ret = configure_fill(node, this->src.color, this->src.fmt,
				dst, this->ivmx, &last);
		if (ret < 0)
			goto error;

		break;
	case B2R2_FLIP: /* FLIP is just a copy with different VSO/HSO */
	case B2R2_COPY:
		if (this->buf_count > 0) {
			/* First do a copy to the tmp buf */
			configure_tmp_buf(&this->tmp_bufs[0], 1, &src->window);

			ret = configure_copy(node, src, &this->tmp_bufs[0],
					this->ivmx, &node);
			if (ret < 0)
				goto error;

			/* Then set the source as the tmp buf */
			src = &this->tmp_bufs[0];

			/* We will not need the iVMX now */
			this->ivmx = NULL;
		}

		ret = configure_copy(node, src, dst, this->ivmx, &last);
		if (ret < 0)
			goto error;

		break;
	case B2R2_ROTATE:
		if (this->buf_count > 0) {
			struct b2r2_node_split_buf *tmp = &this->tmp_bufs[0];

			memcpy(&tmp->window, &dst->window, sizeof(tmp->window));

			tmp->window.x = 0;
			tmp->window.y = tmp->window.height - 1;

			/* Rotate to the temp buf */
			ret = configure_rotate(node, src, tmp, this->ivmx, &node);
			if (ret < 0)
				goto error;

			/* Then do a copy to the destination */
			ret = configure_copy(node, tmp, dst, NULL, &last);
			if (ret < 0)
				goto error;

		} else {
			/* Just do a rotation */
			ret = configure_rotate(node, src, dst, this->ivmx,
						&last);
			if (ret < 0)
				goto error;

		}

		break;
	case B2R2_SCALE:
		if (this->buf_count > 0) {
			/* Scaling will be done first */
			configure_tmp_buf(&this->tmp_bufs[0], 1, &dst->window);

			dst = &this->tmp_bufs[0];
		}

		ret = configure_scale(node, src, dst, this->horiz_sf, this->vert_sf,
				this->ivmx, &last);
		if (ret < 0)
			goto error;

		if (this->buf_count > 0) {
			/* Then copy the tmp buf to the destination */
			node = last;
			src = dst;
			dst = &this->dst;
			ret = configure_copy(node, src, dst, NULL, &last);
			if (ret < 0)
				goto error;
		}
		break;
	case B2R2_SCALE_AND_ROTATE:
		ret = configure_rot_scale(this, node, &last);
		if (ret < 0)
			goto error;
		break;
	default:
		printk(KERN_ERR "%s: Unsupported request\n", __func__);
		ret = -ENOSYS;
		goto error;

		break;
	}

	/* Scale and rotate will configure its own blending and clipping */
	if (this->type != B2R2_SCALE_AND_ROTATE) {

		/* Configure blending and clipping */
		do {
			if (node == NULL) {
				printk(KERN_ERR LOG_TAG "::%s: "
					"Internal error! Out of nodes!\n",
					__func__);
				ret = -ENOMEM;
				goto error;
			}

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
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
	return ret;
}

/**
 * configure_rot_scale() - configures a combined rotation and scaling op
 */
static int configure_rot_scale(struct b2r2_node_split_job *this,
		struct b2r2_node *node, struct b2r2_node **next)
{
	int ret;

	bool last_row;
	bool last_col;

	struct b2r2_node *rot_start;
	struct b2r2_node *last;

	struct b2r2_node_split_buf *tmp = &this->tmp_bufs[0];
	struct b2r2_blt_rect dst_win;

	if (node == NULL) {
		printk(KERN_ERR LOG_TAG "::%s: Out of nodes!\n",
			__func__);
		ret = -ENOMEM;
		goto error;
	}

	memcpy(&dst_win, &this->dst.window, sizeof(dst_win));

	tmp->rect.x = 0;
	tmp->rect.y = 0;
	tmp->rect.width = this->dst.window.height;
	tmp->rect.height = this->dst.window.width;

	memcpy(&tmp->window, &tmp->rect, sizeof(tmp->window));
	pdebug(KERN_INFO LOG_TAG "::%s:Rot rescale:\n", __func__);
	pdebug(KERN_INFO LOG_TAG "::%s:\tsrc=(%4d, %4d, %4d, %4d)\t"
		"tmp=(%4d, %4d, %4d, %4d)\n", __func__, this->src.window.x,
		this->src.window.y, this->src.window.width,
		this->src.window.height, tmp->window.x, tmp->window.y,
		tmp->window.width, tmp->window.height);

	configure_scale(node, &this->src, tmp, this->horiz_sf,
			this->vert_sf, this->ivmx, &node);

	tmp->window.width = MIN(tmp->rect.width, B2R2_ROTATE_MAX_WIDTH);
#ifdef B2R2_ROTATION_HEIGHT_BUGFIX
	if ((tmp->window.height > 16) && (tmp->window.height % 16))
		tmp->window.height -= tmp->window.height % 16;
#endif

	tmp->dx = tmp->window.width;
	tmp->dy = tmp->window.height;

	this->dst.window.width = tmp->window.height;
	this->dst.window.height = tmp->window.width;

	rot_start = node;

	pdebug(KERN_INFO LOG_TAG "::%s: tmp_rect=(%d, %d, %d, %d)\n", __func__,
		tmp->rect.x, tmp->rect.y, tmp->rect.width, tmp->rect.height);
	do {

		last_col = tmp->window.x + tmp->dx >= tmp->rect.width;
		last_row = tmp->window.y + tmp->dy >= tmp->rect.height;

		if (node == NULL) {
			printk(KERN_ERR LOG_TAG "::%s: Out of nodes!\n",
				__func__);
			ret = -ENOMEM;
			goto error;
		}

		if (last_col) {
			tmp->window.width = tmp->rect.width - tmp->window.x;
			this->dst.window.height = tmp->window.width;
		}

		if (last_row) {
			tmp->window.height = tmp->rect.height - tmp->window.y;
			this->dst.window.width = tmp->window.height;
		}

		pdebug(KERN_INFO LOG_TAG "::%s: \ttmp=(%4d, %4d, %4d, %4d) "
			"\tdst=(%4d, %4d, %4d, %4d)\n", __func__, tmp->window.x,
			tmp->window.y, tmp->window.width, tmp->window.height,
			this->dst.window.x, this->dst.window.y,
			this->dst.window.width, this->dst.window.height);

		configure_rotate(node, tmp, &this->dst, NULL, &last);

		tmp->window.x += tmp->dx;
		this->dst.window.y -= tmp->dx;

		/* Slide vertically if we reached the end of the row */
		if (last_col) {
			tmp->window.x = 0;
			tmp->window.y += tmp->dy;
			this->dst.window.x += tmp->dy;
			this->dst.window.y = dst_win.y;
		}

		/* Consume the nodes */
		node = last;

	} while (!(last_row && last_col));



	/* Configure blending and clipping for the rotation nodes */
	node = rot_start;
	pdebug(KERN_INFO LOG_TAG "::%s: "
		"Configuring clipping and blending. rot_start=%p, last=%p\n",
		__func__, rot_start, last);
	do {
		if (node == NULL) {
			printk(KERN_ERR LOG_TAG "::%s: Out of nodes!\n",
				__func__);
			ret = -ENOMEM;
			goto error;
		}

		if (this->blend) {
			configure_blend(node, this->flags,
					this->global_alpha, false);
		}
		if (this->clip)
			configure_clip(node, &this->clip_rect);

		node = node->next;
	} while (node != last);

	memcpy(&this->dst.window, &dst_win, sizeof(this->dst.window));

	*next = node;

	return 0;
error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
	return ret;
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
static int configure_fill(struct b2r2_node *node, u32 color,
		enum b2r2_blt_fmt fmt, struct b2r2_node_split_buf *dst,
		const u32 *ivmx, struct b2r2_node **next)
{
	int ret;
	struct b2r2_node *last;

	PTRACE_ENTRY();

	/* Configure the destination */
	ret = configure_dst(node, dst, ivmx, &last);
	if (ret < 0)
		goto error;

	do {
		if (node == NULL) {
			printk(KERN_ERR LOG_TAG "::%s: "
			"Internal error! Out of nodes!\n", __func__);
			ret = -ENOMEM;
			goto error;
		}

		node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_2 |
							B2R2_CIC_COLOR_FILL;
		node->node.GROUP0.B2R2_INS |=
				B2R2_INS_SOURCE_2_COLOR_FILL_REGISTER;
		node->node.GROUP0.B2R2_ACK |= B2R2_ACK_MODE_BYPASS_S2_S3;

		/* B2R2 has a bug that disables color fill from S2. As a
		   workaround we use S1 for the color. */
		node->node.GROUP2.B2R2_S1CF = 0;
		node->node.GROUP2.B2R2_S2CF = color;

		/* TO BE REMOVED: */
		set_src_2(node, dst->addr, dst);
		node->node.GROUP4.B2R2_STY = to_native_fmt(fmt);

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

	return 0;
error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
	return ret;
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
static int configure_copy(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, const u32 *ivmx,
		struct b2r2_node **next)
{
	int ret;

	struct b2r2_node *last;

	PTRACE_ENTRY();

	pverbose(KERN_INFO LOG_TAG "::%s:\n", __func__);
	pverbose(KERN_INFO "\tsrc=(%4d, %4d, %4d, %4d)\n"
			"\tdst=(%4d, %4d, %4d, %4d)\n", src->window.x,
			src->window.y, src->window.width, src->window.height,
			dst->window.x, dst->window.y, dst->window.width,
			dst->window.height);

	ret = configure_dst(node, dst, ivmx, &last);
	if (ret < 0)
		goto error;

	/* Configure the source for each node */
	do {
		if (node == NULL) {
			printk(KERN_ERR LOG_TAG "::%s: "
			" Internal error! Out of nodes!\n", __func__);
			ret = -ENOMEM;
			goto error;
		}

		node->node.GROUP0.B2R2_ACK |= B2R2_ACK_MODE_BYPASS_S2_S3;

		/* Configure the source(s) */
		configure_src(node, src, ivmx);

		node = node->next;
	} while (node != last);

	/* Consume the nodes */
	*next = node;

	return 0;
error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
	return ret;
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
static int configure_rotate(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, const u32 *ivmx,
		struct b2r2_node **next)
{
	int ret;

	struct b2r2_node *last;

	ret = configure_copy(node, src, dst, ivmx, &last);
	if (ret < 0)
		goto error;

	do {
		if (node == NULL) {
			printk(KERN_ERR LOG_TAG "::%s: "
				"Internal error! Out of nodes!\n", __func__);
			ret = -ENOMEM;
			goto error;
		}

		node->node.GROUP0.B2R2_INS |= B2R2_INS_ROTATION_ENABLED;

		node = node->next;

	} while (node != last);

	/* Consume the nodes */
	*next = node;

	return 0;
error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
	return ret;
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
static int configure_scale(struct b2r2_node *node,
		struct b2r2_node_split_buf *src,
		struct b2r2_node_split_buf *dst, u16 h_rsf, u16 v_rsf,
		const u32 *ivmx, struct b2r2_node **next)
{
	int ret;

	struct b2r2_node *last;

	u32 fctl = 0;
	u32 rsf = 0;
	u32 rzi = 0;

	pdebug(KERN_INFO LOG_TAG "::%s:\n"
		"\t\tsrc=(%4d, %4d, %4d, %4d)\tdst=(%4d, %4d, %4d, %4d)\n",
		__func__, src->window.x, src->window.y, src->window.width,
		src->window.height, dst->window.x, dst->window.y,
		dst->window.width, dst->window.height);

	/* Configure horizontal rescale */
	fctl |= B2R2_FCTL_HF2D_MODE_ENABLE_RESIZER |
			B2R2_FCTL_LUMA_HF2D_MODE_ENABLE_RESIZER;
	rsf &= ~(0xffff << B2R2_RSF_HSRC_INC_SHIFT);
	rsf |= h_rsf << B2R2_RSF_HSRC_INC_SHIFT;
	rzi |= B2R2_RZI_DEFAULT_HNB_REPEAT;
	fctl |= B2R2_FCTL_HF2D_MODE_ENABLE_COLOR_CHANNEL_FILTER;

	/* Configure vertical rescale */
	fctl |= B2R2_FCTL_VF2D_MODE_ENABLE_RESIZER |
			B2R2_FCTL_LUMA_VF2D_MODE_ENABLE_RESIZER;
	rsf &= ~(0xffff << B2R2_RSF_VSRC_INC_SHIFT);
	rsf |= v_rsf << B2R2_RSF_VSRC_INC_SHIFT;
	rzi |= B2R2_RZI_DEFAULT_VNB_REPEAT;
	fctl |= B2R2_FCTL_VF2D_MODE_ENABLE_COLOR_CHANNEL_FILTER;

	ret = configure_copy(node, src, dst, ivmx, &last);
	if (ret < 0)
		goto error;

	do {
		if (node == NULL) {
			printk(KERN_ERR LOG_TAG "::%s: "
				"Internal error! Out of nodes!\n", __func__);
			ret = -ENOMEM;
			goto error;
		}

		node->node.GROUP0.B2R2_CIC |= B2R2_CIC_FILTER_CONTROL |
						B2R2_CIC_RESIZE_LUMA |
						B2R2_CIC_RESIZE_CHROMA;
		node->node.GROUP0.B2R2_INS |=
				B2R2_INS_RESCALE2D_ENABLED;

		/* Set the filter control and rescale registers */
		node->node.GROUP8.B2R2_FCTL |= fctl;
		if ((src->type == B2R2_FMT_TYPE_SEMI_PLANAR) ||
				(src->type == B2R2_FMT_TYPE_PLANAR)) {
			u32 chroma_vsf = v_rsf;
			u32 chroma_hsf = h_rsf;

			if ((dst->type != B2R2_FMT_TYPE_SEMI_PLANAR) &&
				(dst->type != B2R2_FMT_TYPE_PLANAR))
				chroma_hsf = h_rsf / 2;

			/* YUV420 needs to be vertically upsampled */
			if (is_yuv420_fmt(src->fmt) !=
					is_yuv420_fmt(dst->fmt))
				chroma_vsf /= 2;

			node->node.GROUP9.B2R2_RSF =
					chroma_hsf << B2R2_RSF_HSRC_INC_SHIFT |
					chroma_vsf << B2R2_RSF_VSRC_INC_SHIFT;

			/* Set the Luma rescale registers as well */
			node->node.GROUP10.B2R2_RSF |= rsf;
			node->node.GROUP10.B2R2_RZI |= rzi;
			node->node.GROUP8.B2R2_FCTL |=
				B2R2_FCTL_LUMA_HF2D_MODE_ENABLE_FILTER |
				B2R2_FCTL_LUMA_VF2D_MODE_ENABLE_FILTER;
			node->node.GROUP10.B2R2_HFP = hf_coeffs_addr;
			node->node.GROUP10.B2R2_VFP = vf_coeffs_addr;
		} else {
			node->node.GROUP9.B2R2_RSF = rsf;
		}
		node->node.GROUP9.B2R2_HFP = hf_coeffs_addr;
		node->node.GROUP9.B2R2_VFP = vf_coeffs_addr;
		node->node.GROUP9.B2R2_RZI |= rzi;

		node = node->next;

	} while (node != last);

	/* Consume the nodes */
	*next = node;

	return 0;
error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
	return ret;
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

		/* Horizontal resolution is half */
		tmp_buf.window.x >>= 1;
		tmp_buf.window.width >>= 1;

		/* If the buffer is in YUV420 format, the vertical resolution
		   is half as well  */
		if (is_yuv420_fmt(src->fmt)) {
			tmp_buf.window.height >>= 1;
			tmp_buf.window.y >>= 1;
		}

		set_src_3(node, src->addr, src);
		set_src_2(node, tmp_buf.chroma_addr, &tmp_buf);
		break;
	case B2R2_FMT_TYPE_PLANAR:
		memcpy(&tmp_buf, src, sizeof(tmp_buf));

		/* Each chroma buffer will have half as many values per line as
		   the luma buffer */
		tmp_buf.pitch >>= 1;

		/* Horizontal resolution is half */
		tmp_buf.window.x >>= 1;
		tmp_buf.window.width >>= 1;

		/* If the buffer is in YUV420 format, the vertical resolution
		   is half as well  */
		if (is_yuv420_fmt(src->fmt)) {
			tmp_buf.window.height >>= 1;
			tmp_buf.window.y >>= 1;
		}

		set_src_3(node, src->addr, src);                   /* Y */
		set_src_2(node, tmp_buf.chroma_addr, &tmp_buf);    /* U */
		set_src_1(node, tmp_buf.chroma_cr_addr, &tmp_buf); /* V */

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
static int configure_dst(struct b2r2_node *node,
		struct b2r2_node_split_buf *dst, const u32 *ivmx,
		struct b2r2_node **next)
{
	int ret;
	int nbr_planes = 1;
	int i;

	struct b2r2_node_split_buf dst_planes[3];

	PTRACE_ENTRY();

	memcpy(&dst_planes[0], dst, sizeof(dst_planes[0]));

	if (dst->type != B2R2_FMT_TYPE_RASTER) {
		/* There will be at least 2 planes */
		nbr_planes = 2;

		memcpy(&dst_planes[1], dst, sizeof(dst_planes[1]));

		dst_planes[1].addr = dst->chroma_addr;
		dst_planes[1].plane_selection = B2R2_TTY_CHROMA_NOT_LUMA;

		/* Horizontal resolution is half */
		dst_planes[1].window.x >>= 1;
		dst_planes[1].window.width >>= 1;

		/* If the buffer is in YUV420 format, the vertical resolution
		   is half as well  */
		if (is_yuv420_fmt(dst->fmt)) {
			dst_planes[1].window.height >>= 1;
			dst_planes[1].window.y >>= 1;
		}

		if (dst->type == B2R2_FMT_TYPE_PLANAR) {
			/* There will be a third plane as well */
			nbr_planes = 3;

			/* The chroma planes have half the luma pitch */
			dst_planes[1].pitch >>= 1;

			memcpy(&dst_planes[2], &dst_planes[1],
				sizeof(dst_planes[2]));
			dst_planes[2].addr = dst->chroma_cr_addr;

			/* The second plane will be Cb */
			dst_planes[1].chroma_selection = B2R2_TTY_CB_NOT_CR;
		}

	}

	/* Configure one node for each plane */
	for (i = 0; i < nbr_planes; i++) {

		if (node == NULL) {
			printk(KERN_ERR LOG_TAG "::%s: "
				"Internal error! Out of nodes!\n", __func__);
			ret = -ENOMEM;
			goto error;
		}

		set_target(node, dst_planes[i].addr, &dst_planes[i]);

		node = node->next;
	}

	pverbose(KERN_INFO LOG_TAG "::%s: %d nodes consumed\n", __func__,
			nbr_planes);

	/* Consume the nodes */
	*next = node;

	return 0;
error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
	return ret;

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
	if ((flags & B2R2_BLT_FLAG_GLOBAL_ALPHA_BLEND) != 0) {

		/* B2R2 expects the global alpha to be in 0...128 range */
		global_alpha = (global_alpha*128)/255;

		node->node.GROUP0.B2R2_ACK |=
			global_alpha <<	B2R2_ACK_GALPHA_ROPID_SHIFT;
	} else {
		node->node.GROUP0.B2R2_ACK |=
			(128 << B2R2_ACK_GALPHA_ROPID_SHIFT);
	}

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
		node->node.GROUP3.B2R2_STY |= node->node.GROUP1.B2R2_TTY;
		node->node.GROUP3.B2R2_SXY = node->node.GROUP1.B2R2_TXY;

		node->node.GROUP3.B2R2_STY &= ~(B2R2_S1TY_A1_SUBST_KEY_MODE);

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
	node->node.GROUP6.B2R2_CWO =
			((clip_rect->y & 0x7FFF) << B2R2_CWO_Y_SHIFT) |
			((clip_rect->x & 0x7FFF) << B2R2_CWO_X_SHIFT);
	node->node.GROUP6.B2R2_CWS =
		(((clip_rect->y + clip_rect->height) & 0x7FFF)
							<< B2R2_CWO_Y_SHIFT) |
		(((clip_rect->x + clip_rect->width) & 0x7FFF)
							<< B2R2_CWO_X_SHIFT);
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
static int constrain_window(struct b2r2_blt_rect *window,
		enum b2r2_blt_fmt fmt, u32 max_size)
{
	int ret;

	u32 current_size;
	u32 pitch;

	pitch = fmt_byte_pitch(fmt, window->width);
	current_size = pitch * window->height;

	if (current_size > max_size) {
		window->width = MIN(window->width, B2R2_RESCALE_MAX_WIDTH);
		pitch = fmt_byte_pitch(fmt, window->width);
		window->height = MIN(window->height, max_size / pitch);

		if (window->height == 0) {
			printk(KERN_ERR LOG_TAG "::%s: Not enough tmp mem\n",
				__func__);
			ret = -ENOMEM;
			goto error;
		}
	}

	return 0;
error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
	return ret;
}

/**
 * setup_tmp_buf() - configure a temporary buffer
 */
static int setup_tmp_buf(struct b2r2_node_split_buf *tmp, u32 max_size,
		enum b2r2_blt_fmt pref_fmt, u32 pref_width, u32 pref_height)
{
	int ret;

	enum b2r2_blt_fmt fmt;

	u32 width;
	u32 height;
	u32 pitch;
	u32 size;

	/* Determine what format we should use for the tmp buf */
	if (is_rgb_fmt(pref_fmt)) {
		fmt = B2R2_BLT_FMT_32_BIT_ARGB8888;
	} else if (is_bgr_fmt(pref_fmt)) {
		fmt = B2R2_BLT_FMT_32_BIT_ABGR8888;
	} else if (is_yuv_fmt(pref_fmt)) {
		fmt = B2R2_BLT_FMT_32_BIT_AYUV8888;
	} else {
		/* Wait, what? */
		printk(KERN_ERR LOG_TAG "::%s: "
			"Cannot create tmp buf from this fmt (%d)\n", __func__,
			pref_fmt);
		ret = -EINVAL;
		goto error;
	}

	/* See if we can fit the entire preferred rectangle */
	width = pref_width;
	height = pref_height;
	pitch = fmt_byte_pitch(fmt, width);
	size = pitch * height;

	if (size > max_size) {
		/* We need to limit the size, so we choose a different width */
		width = MIN(width, B2R2_RESCALE_MAX_WIDTH);
		pitch = fmt_byte_pitch(fmt, width);
		height = MIN(height, max_size / pitch);
		size = pitch * height;
	}

	/* We should at least have enough room for one scanline */
	if (height == 0) {
		printk(KERN_ERR LOG_TAG "::%s: Not enough tmp mem!\n",
			__func__);
		ret = -ENOMEM;
		goto error;
	}

	memset(tmp, 0, sizeof(*tmp));

	tmp->fmt = fmt;
	tmp->type = B2R2_FMT_TYPE_RASTER;
	tmp->height = height;
	tmp->pitch = pitch;

	tmp->rect.width = width;
	tmp->rect.height = tmp->height;
	tmp->alpha_range = B2R2_TY_ALPHA_RANGE_255;

	return 0;
error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
	return ret;

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
	default:
		return B2R2_TY_ALPHA_RANGE_128; /* 0 - 128 */
	}
}

/**
 * get_alpha() - returns the pixel alpha in 0...255 range
 */
static inline u8 get_alpha(enum b2r2_blt_fmt fmt, u32 pixel)
{
	switch (fmt) {
	case B2R2_BLT_FMT_32_BIT_ARGB8888:
	case B2R2_BLT_FMT_32_BIT_ABGR8888:
	case B2R2_BLT_FMT_32_BIT_AYUV8888:
		return (pixel >> 24) & 0xff;
	case B2R2_BLT_FMT_24_BIT_ARGB8565:
		return (pixel & 0xfff) >> 16;
	case B2R2_BLT_FMT_16_BIT_ARGB4444:
		return (((pixel >> 12) & 0xf) * 255) / 15;
	case B2R2_BLT_FMT_16_BIT_ARGB1555:
		return (pixel >> 15) * 255;
	case B2R2_BLT_FMT_1_BIT_A1:
		return pixel * 255;
	case B2R2_BLT_FMT_8_BIT_A8:
		return pixel;
	default:
		return 255;
	}
}

/**
 * set_alpha() - returns a color value with the alpha component set
 */
static inline u32 set_alpha(enum b2r2_blt_fmt fmt, u8 alpha, u32 color)
{
	u32 alpha_mask;

	switch (fmt) {
	case B2R2_BLT_FMT_32_BIT_ARGB8888:
	case B2R2_BLT_FMT_32_BIT_ABGR8888:
	case B2R2_BLT_FMT_32_BIT_AYUV8888:
		color &= 0x00ffffff;
		alpha_mask = alpha << 24;
		break;
	case B2R2_BLT_FMT_24_BIT_ARGB8565:
		color &= 0x00ffff;
		alpha_mask = alpha << 16;
		break;
	case B2R2_BLT_FMT_16_BIT_ARGB4444:
		color &= 0x0fff;
		alpha_mask = (alpha << 8) & 0xF000;
		break;
	case B2R2_BLT_FMT_16_BIT_ARGB1555:
		color &= 0x7fff;
		alpha_mask = (alpha / 255) << 15 ;
		break;
	case B2R2_BLT_FMT_1_BIT_A1:
		color = 0;
		alpha_mask = (alpha / 255);
		break;
	case B2R2_BLT_FMT_8_BIT_A8:
		color = 0;
		alpha_mask = alpha;
		break;
	default:
		alpha_mask = 0;
	}

	return color | alpha_mask;
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
 * is_yuv420_fmt() - returns whether the given format is a yuv420 format
 */
static inline bool is_yuv420_fmt(enum b2r2_blt_fmt fmt)
{

	switch (fmt) {
	case B2R2_BLT_FMT_YUV420_PACKED_PLANAR:
	case B2R2_BLT_FMT_YUV420_PACKED_SEMI_PLANAR:
	case B2R2_BLT_FMT_YUV420_PACKED_SEMIPLANAR_MB_STE:
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

static inline int fmt_bpp(enum b2r2_blt_fmt fmt)
{
	switch (fmt) {

	case B2R2_BLT_FMT_16_BIT_ARGB4444: /* Fall through */
	case B2R2_BLT_FMT_16_BIT_ARGB1555: /* Fall through */
	case B2R2_BLT_FMT_16_BIT_RGB565:   /* Fall through */
	case B2R2_BLT_FMT_Y_CB_Y_CR:       /* Fall through */
	case B2R2_BLT_FMT_CB_Y_CR_Y:
		return 2;

	case B2R2_BLT_FMT_24_BIT_RGB888:   /* Fall through */
	case B2R2_BLT_FMT_24_BIT_ARGB8565: /* Fall through */
	case B2R2_BLT_FMT_24_BIT_YUV888:
		return 3;

	case B2R2_BLT_FMT_32_BIT_ARGB8888: /* Fall through */
	case B2R2_BLT_FMT_32_BIT_ABGR8888: /* Fall through */
	case B2R2_BLT_FMT_32_BIT_AYUV8888:
		return 4;
	default:
		return 1;
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
	case B2R2_BLT_FMT_YUV420_PACKED_SEMI_PLANAR:
	case B2R2_BLT_FMT_YUV422_PACKED_SEMI_PLANAR:
		return B2R2_NATIVE_YCBCR42X_R2B;
	case B2R2_BLT_FMT_YUV420_PACKED_SEMIPLANAR_MB_STE:
	case B2R2_BLT_FMT_YUV422_PACKED_SEMIPLANAR_MB_STE:
		return B2R2_NATIVE_YCBCR42X_MBN;
	case B2R2_BLT_FMT_YUV420_PACKED_PLANAR:
	case B2R2_BLT_FMT_YUV422_PACKED_PLANAR:
		return B2R2_NATIVE_YUV;
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

	/* Assume normal nearest neighbor scaling:

	        sf = (src - min_step) / (dst - 1)
	*/
	u32 sf = ((from << 10) - 1) / (to - 1);

	if ((sf & 0xffff0000) != 0) {
		/* Overflow error */
		pverbose(KERN_ERR LOG_TAG "::%s: "
			"Scale factor too large\n", __func__);
		ret = -EINVAL;
		goto error;
	} else if (sf == 0) {
		pverbose(KERN_ERR LOG_TAG "::%s: "
			"Scale factor too small\n", __func__);
		ret = -EINVAL;
		goto error;
	}

	*sf_out = (u16)sf;

	return 0;

error:
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
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

	pverbose(KERN_INFO LOG_TAG "::%s: area=(%d, %d) tile=(%d, %d)\n",
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

	tmp = (tmp - 1) / sf + 1;

	return tmp;
}

/**
 * inv_rescale() - does an inverted rescale of the given dimension
 */
static inline s32 inv_rescale(s32 dim, u16 sf)
{
	s32 new_dim = (dim - 1) * (s32)sf + 1;

	if (new_dim & 0x3FF)
		new_dim += (1 << 10);

	return new_dim >> 10;
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
	u32 buf_width = buf->pitch / fmt_bpp(buf->fmt);

	u32 l;
	u32 r;
	u32 t;
	u32 b;

	PTRACE_ENTRY();

	if (buf->tmp_buf_index) {
		pverbose(KERN_INFO LOG_TAG "::%s: %p target is tmp\n", __func__,
				node);
		node->dst_tmp_index = buf->tmp_buf_index;
	}

	node->node.GROUP1.B2R2_TBA = addr;
	node->node.GROUP1.B2R2_TTY = buf->pitch | to_native_fmt(buf->fmt) |
			buf->alpha_range | buf->chroma_selection | buf->hso |
			buf->vso | buf->plane_selection;
	node->node.GROUP1.B2R2_TSZ =
			((buf->window.width & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
			((buf->window.height & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);
	node->node.GROUP1.B2R2_TXY =
			((buf->window.x & 0xffff) << B2R2_XY_X_SHIFT) |
			((buf->window.y & 0xffff) << B2R2_XY_Y_SHIFT);

	/* Check if the rectangle is outside the buffer */
	if (buf->vso == B2R2_TY_VSO_BOTTOM_TO_TOP)
		t = buf->window.y - (buf->window.height - 1);
	else
		t = buf->window.y;

	if (buf->hso == B2R2_TY_HSO_RIGHT_TO_LEFT)
		l = buf->window.x - (buf->window.width - 1);
	else
		l = buf->window.x;

	r = l + buf->window.width;
	b = t + buf->window.height;

	/* Clip to the destination buffer to prevent memory overwrites */
	if ((l < 0) || (r > buf_width) || (t < 0) || (b > buf->height)) {
		pverbose(KERN_INFO LOG_TAG "::%s: "
			"Window outside buffer. Clipping...\n", __func__);

		/* The clip rectangle is including the borders */
		l = MAX(l, 0);
		r = MIN(r, buf_width) - 1;
		t = MAX(t, 0);
		b = MIN(b, buf->height) - 1;

		node->node.GROUP0.B2R2_CIC |= B2R2_CIC_CLIP_WINDOW;
		node->node.GROUP0.B2R2_INS |= B2R2_INS_RECT_CLIP_ENABLED;
		node->node.GROUP6.B2R2_CWO =
			((l & 0x7FFF) << B2R2_CWS_X_SHIFT) |
			((t & 0x7FFF) << B2R2_CWS_Y_SHIFT);
		node->node.GROUP6.B2R2_CWS =
			((r & 0x7FFF) << B2R2_CWO_X_SHIFT) |
			((b & 0x7FFF) << B2R2_CWO_Y_SHIFT);
	}

	pverbose(KERN_INFO LOG_TAG "::%s: addr=%p, rect=(%d, %d, %d, %d), "
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
		pverbose(KERN_INFO LOG_TAG "::%s: %p src1 is tmp\n", __func__,
				node);
		node->src_tmp_index = buf->tmp_buf_index;
	}

	node->src_index = 1;

	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_1;
	node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_1_FETCH_FROM_MEM;

	pverbose(KERN_INFO LOG_TAG "::%s: addr=%p, rect=(%d, %d, %d, %d), "
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
		pverbose(KERN_INFO LOG_TAG "::%s: %p src2 is tmp\n", __func__,
				node);
		node->src_tmp_index = buf->tmp_buf_index;
	}

	node->src_index = 2;

	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_2;
	node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_2_FETCH_FROM_MEM;

	pverbose(KERN_INFO LOG_TAG "::%s: addr=%p, rect=(%d, %d, %d, %d), "
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
		pverbose(KERN_INFO LOG_TAG "::%s: %p src3 is tmp\n", __func__,
				node);
		node->src_tmp_index = buf->tmp_buf_index;
	}

	node->src_index = 3;

	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_3;
	node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_3_FETCH_FROM_MEM;

	pverbose(KERN_INFO LOG_TAG "::%s: addr=%p, rect=(%d, %d, %d, %d), "
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

	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_IVMX;
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

		node->src_tmp_index = 0;
		node->dst_tmp_index = 0;

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
	printk(KERN_ERR LOG_TAG "::%s: ERROR!\n", __func__);
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
	if (hf_coeffs == NULL) {
		unsigned char hf[HF_TABLE_SIZE] = {
								0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
								0xff, 0x03, 0xfd, 0x08, 0x3e, 0xf9, 0x04, 0xfe,
								0xfd, 0x06, 0xf8, 0x13, 0x3b, 0xf4, 0x07, 0xfc,
								0xfb, 0x08, 0xf5, 0x1f, 0x34, 0xf1, 0x09, 0xfb,
								0xfb, 0x09, 0xf2, 0x2b, 0x2a, 0xf1, 0x09, 0xfb,
								0xfb, 0x09, 0xf2, 0x35, 0x1e, 0xf4, 0x08, 0xfb,
								0xfc, 0x07, 0xf5, 0x3c, 0x12, 0xf7, 0x06, 0xfd,
								0xfe, 0x04, 0xfa, 0x3f, 0x07, 0xfc, 0x03, 0xff};

		hf_coeffs = dma_alloc_coherent(b2r2_blt_device(),
				sizeof(hf), &hf_coeffs_addr, GFP_DMA | GFP_KERNEL);
		if (hf_coeffs != NULL)
			memcpy(hf_coeffs, hf, sizeof(hf));
	}

	if (vf_coeffs == NULL) {
		unsigned char vf[VF_TABLE_SIZE] = {
								0x00, 0x00, 0x40, 0x00, 0x00,
								0xfd, 0x09, 0x3c, 0xfa, 0x04,
								0xf9, 0x13, 0x39, 0xf5, 0x06,
								0xf5, 0x1f, 0x31, 0xf3, 0x08,
								0xf3, 0x2a, 0x28, 0xf3, 0x08,
								0xf3, 0x34, 0x1d, 0xf5, 0x07,
								0xf5, 0x3b, 0x12, 0xf9, 0x05,
								0xfa, 0x3f, 0x07, 0xfd, 0x03};
		vf_coeffs = dma_alloc_coherent(b2r2_blt_device(),
				sizeof(vf), &vf_coeffs_addr, GFP_DMA | GFP_KERNEL);
		if (vf_coeffs != NULL)
			memcpy(vf_coeffs, vf, sizeof(vf));
	}

	return init_debugfs();
}

void b2r2_node_split_exit(void)
{
	if (hf_coeffs != NULL) {
		dma_free_coherent(b2r2_blt_device(),
				HF_TABLE_SIZE, hf_coeffs, hf_coeffs_addr);
		hf_coeffs = NULL;
	}

	if (vf_coeffs != NULL) {
		dma_free_coherent(b2r2_blt_device(),
				VF_TABLE_SIZE, vf_coeffs, vf_coeffs_addr);
		vf_coeffs = NULL;
	}

	exit_debugfs();
}
