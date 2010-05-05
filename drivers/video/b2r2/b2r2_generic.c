#include <linux/kernel.h>
#include <linux/debugfs.h>

#include "b2r2_internal.h"
#include "b2r2_global.h"

/******************
 * Debug printing
 ******************/
static u32 debug = 0;
static u32 debug_areas = 0;
static u32 verbose = 0;
static u32 errors = 1;

#define B2R2_GENERIC_DEBUG

#define pdebug(...) \
	do { \
		if (debug) \
			printk(KERN_INFO __VA_ARGS__); \
	} while (false)

#define pdebug_areas(...) \
	do { \
		if (debug_areas) \
			printk(KERN_INFO __VA_ARGS__); \
	} while (false)

#define pverbose(...) \
	do { \
		if (verbose) \
			printk(KERN_INFO __VA_ARGS__); \
	} while (false)

#define err_msg(...) \
	do { \
		if (errors) \
			printk(KERN_INFO __VA_ARGS__); \
	} while (false)

#define PTRACE_ENTRY() pdebug(LOG_TAG "::%s\n", __func__)

#define LOG_TAG "b2r2_generic"

#define B2R2_GENERIC_WORK_BUF_WIDTH 16
#define B2R2_GENERIC_WORK_BUF_HEIGHT 16
#define B2R2_GENERIC_WORK_BUF_PITCH (16 * 4)
#define B2R2_GENERIC_WORK_BUF_FMT B2R2_NATIVE_ARGB8888

/********************
 * Private functions
 ********************/
/**
 * reset_nodes() - clears the node list
 */
static void reset_nodes(struct b2r2_node *node)
{
	pdebug("%s ENTRY\n", __func__);

	while (node != NULL) {
		memset(&(node->node), 0, sizeof(node->node));

		/* TODO: Implement support for short linked lists */
		node->node.GROUP0.B2R2_CIC = 0x7fffc;

		if (node->next != NULL)
			node->node.GROUP0.B2R2_NIP = node->next->physical_address;

		node = node->next;
	}
	pdebug("%s DONE\n", __func__);
}

/**
 * dump_nodes() - prints the node list
 */
static void dump_nodes(struct b2r2_node *first)
{
	struct b2r2_node *node = first;
	pverbose("%s ENTRY\n", __func__);
	while (node != NULL) {
		pverbose( "\nNODE START:\n=============\n");
		pverbose("B2R2_ACK: \t0x%.8x\n",
				node->node.GROUP0.B2R2_ACK);
		pverbose("B2R2_INS: \t0x%.8x\n",
				node->node.GROUP0.B2R2_INS);
		pverbose("B2R2_CIC: \t0x%.8x\n",
				node->node.GROUP0.B2R2_CIC);
		pverbose("B2R2_NIP: \t0x%.8x\n",
				node->node.GROUP0.B2R2_NIP);

		pverbose("B2R2_TSZ: \t0x%.8x\n",
				node->node.GROUP1.B2R2_TSZ);
		pverbose("B2R2_TXY: \t0x%.8x\n",
				node->node.GROUP1.B2R2_TXY);
		pverbose("B2R2_TTY: \t0x%.8x\n",
				node->node.GROUP1.B2R2_TTY);
		pverbose("B2R2_TBA: \t0x%.8x\n",
				node->node.GROUP1.B2R2_TBA);

		pverbose("B2R2_S2CF: \t0x%.8x\n",
				node->node.GROUP2.B2R2_S2CF);
		pverbose("B2R2_S1CF: \t0x%.8x\n",
				node->node.GROUP2.B2R2_S1CF);

		pverbose("B2R2_S1SZ: \t0x%.8x\n",
				node->node.GROUP3.B2R2_SSZ);
		pverbose("B2R2_S1XY: \t0x%.8x\n",
				node->node.GROUP3.B2R2_SXY);
		pverbose("B2R2_S1TY: \t0x%.8x\n",
				node->node.GROUP3.B2R2_STY);
		pverbose("B2R2_S1BA: \t0x%.8x\n",
				node->node.GROUP3.B2R2_SBA);

		pverbose("B2R2_S2SZ: \t0x%.8x\n",
				node->node.GROUP4.B2R2_SSZ);
		pverbose("B2R2_S2XY: \t0x%.8x\n",
				node->node.GROUP4.B2R2_SXY);
		pverbose("B2R2_S2TY: \t0x%.8x\n",
				node->node.GROUP4.B2R2_STY);
		pverbose("B2R2_S2BA: \t0x%.8x\n",
				node->node.GROUP4.B2R2_SBA);

		pverbose("B2R2_S3SZ: \t0x%.8x\n",
				node->node.GROUP5.B2R2_SSZ);
		pverbose("B2R2_S3XY: \t0x%.8x\n",
				node->node.GROUP5.B2R2_SXY);
		pverbose("B2R2_S3TY: \t0x%.8x\n",
				node->node.GROUP5.B2R2_STY);
		pverbose("B2R2_S3BA: \t0x%.8x\n",
				node->node.GROUP5.B2R2_SBA);

		pverbose("B2R2_CWS: \t0x%.8x\n",
				node->node.GROUP6.B2R2_CWS);
		pverbose("B2R2_CWO: \t0x%.8x\n",
				node->node.GROUP6.B2R2_CWO);

		pverbose("B2R2_FCTL: \t0x%.8x\n",
				node->node.GROUP8.B2R2_FCTL);
		pverbose("B2R2_RSF: \t0x%.8x\n",
				node->node.GROUP9.B2R2_RSF);
		pverbose("B2R2_RZI: \t0x%.8x\n",
				node->node.GROUP9.B2R2_RZI);
		pverbose("B2R2_HFP: \t0x%.8x\n",
				node->node.GROUP9.B2R2_HFP);
		pverbose("B2R2_VFP: \t0x%.8x\n",
				node->node.GROUP9.B2R2_VFP);
		pverbose("B2R2_LUMA_RSF: \t0x%.8x\n",
				node->node.GROUP10.B2R2_RSF);
		pverbose("B2R2_LUMA_RZI: \t0x%.8x\n",
				node->node.GROUP10.B2R2_RZI);
		pverbose("B2R2_LUMA_HFP: \t0x%.8x\n",
				node->node.GROUP10.B2R2_HFP);
		pverbose("B2R2_LUMA_VFP: \t0x%.8x\n",
				node->node.GROUP10.B2R2_VFP);


		pverbose("B2R2_IVMX0: \t0x%.8x\n",
				node->node.GROUP15.B2R2_VMX0);
		pverbose("B2R2_IVMX1: \t0x%.8x\n",
				node->node.GROUP15.B2R2_VMX1);
		pverbose("B2R2_IVMX2: \t0x%.8x\n",
				node->node.GROUP15.B2R2_VMX2);
		pverbose("B2R2_IVMX3: \t0x%.8x\n",
				node->node.GROUP15.B2R2_VMX3);
		pverbose("\n=============\nNODE END\n");

		node = node->next;
	}
	pverbose("%s DONE\n", __func__);
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

static unsigned int get_pitch(enum b2r2_blt_fmt format, u32 width)
{
	switch (format) {
	case B2R2_BLT_FMT_1_BIT_A1: {
		int pitch = width >> 3;
		/* Check for remainder */
		if (width & 7)
			pitch++;
		return pitch;
		break;
	}
	case B2R2_BLT_FMT_8_BIT_A8:
		return width;
		break;
	case B2R2_BLT_FMT_16_BIT_RGB565: /* all 16 bits/pixel RGB formats */
	case B2R2_BLT_FMT_16_BIT_ARGB1555:
	case B2R2_BLT_FMT_16_BIT_ARGB4444:
		return width * 2;
		break;
	case B2R2_BLT_FMT_24_BIT_RGB888: /* all 24 bits/pixel RGB formats */
	case B2R2_BLT_FMT_24_BIT_ARGB8565:
		return width * 3;
		break;
	case B2R2_BLT_FMT_32_BIT_ARGB8888: /* all 32 bits/pixel RGB formats */
	case B2R2_BLT_FMT_32_BIT_ABGR8888:
		return width * 4;
		break;
	case B2R2_BLT_FMT_32_BIT_AYUV8888:
		return width * 4;
		break;
	case B2R2_BLT_FMT_Y_CB_Y_CR:
	case B2R2_BLT_FMT_CB_Y_CR_Y:
		/* width of the buffer must be a multiple of 4 */
		if (width & 3)
			return 0;
		return width * 2;
		break;
	case B2R2_BLT_FMT_YUV420_PACKED_SEMI_PLANAR: /* fall through, same pitch and pointers */
	case B2R2_BLT_FMT_YUV422_PACKED_SEMI_PLANAR:
		/* width of the buffer must be a multiple of 2 */
		if (width & 1)
			return 0;
		/* return pitch of the Y-buffer. U and V pitch can be derived from it */
		return width;
		break;
	case B2R2_BLT_FMT_YUV420_PACKED_SEMIPLANAR_MB_STE:
	case B2R2_BLT_FMT_YUV422_PACKED_SEMIPLANAR_MB_STE:
		/* width of the buffer must be a multiple of 32
		 * making the chrominance width a multiple of 16,
		 * which is required by the chrominance macro-blocks
		 * being 16x8 pixels.
		 */
		if (width & 31)
			return 0;
		/* return pitch of the Y-buffer. U and V pitch can be derived from it */
		return width;
		break;
	default:
		err_msg("%s: Unable to determine pitch "
			"for fmt=%#010x width=%d\n", __func__,
			format, width);
		return 0;
	}
}

static s32 validate_buf(const struct b2r2_blt_img *image)
{
	u32 expect_buf_size;
	u32 pitch;

	if (image->width <= 0 || image->height <= 0) {
		err_msg("%s Error: width=%d or height=%d negative\n", __func__,
			image->width, image->height);
		return -EINVAL;
	}

	if (image->pitch == 0) {
		/* autodetect pitch based on format and width */
		pitch = get_pitch(image->fmt, image->width);
	} else
		pitch = image->pitch;

	expect_buf_size = pitch * image->height;

	if (pitch == 0) {
		return -EINVAL;
	}

	/* format specific adjustments */
	switch (image->fmt) {
	case B2R2_BLT_FMT_YUV420_PACKED_SEMI_PLANAR: /* fall through, same size */
	case B2R2_BLT_FMT_YUV420_PACKED_SEMIPLANAR_MB_STE:
		/* include space occupied by U and V data
		 * 1 byte per pixel, half resolution, for each U and V */
		expect_buf_size += ((pitch >> 1) * (image->height >> 1)) * 2;
		break;
	case B2R2_BLT_FMT_YUV422_PACKED_SEMI_PLANAR: /* fall through, same size */
	case B2R2_BLT_FMT_YUV422_PACKED_SEMIPLANAR_MB_STE:
		/* include space occupied by U and V data
		 * 1 byte per pixel, half horizontal resolution, for each U and V */
		expect_buf_size += ((pitch >> 1) * image->height) * 2;
		break;
	default:
		break;
	}

	if (image->buf.len < expect_buf_size) {
		err_msg("%s Error: Invalid buffer size:"
			"\nfmt=%#010x buf.len=%d expect_buf_size=%d\n", __func__,
			image->fmt, image->buf.len, expect_buf_size);
		return -EINVAL;
	}

	if (image->buf.type == B2R2_BLT_PTR_VIRTUAL) {
		err_msg("%s Error: Virtual pointers not supported yet.\n",
			__func__);
		return -EINVAL;
	}
	return 0;
}

static void setup_input_stage(const struct b2r2_blt_request *req,
							  struct b2r2_node *node,
							  struct b2r2_work_buf *out_buf)
{
	/* Horizontal and vertical scaling factors in 6.10 fixed point format */
	s32 h_scf = 1 << 10;
	s32 v_scf = 1 << 10;
	const struct b2r2_blt_rect *src_rect = &(req->user_req.src_rect);
	const struct b2r2_blt_rect *dst_rect = &(req->user_req.dst_rect);
	const struct b2r2_blt_img *src_img = &(req->user_req.src_img);
	const struct b2r2_blt_img *dst_img = &(req->user_req.dst_img);
	u32 src_pitch = 0;
	/* horizontal and vertical scan order for out_buf */
	enum b2r2_ty dst_hso = B2R2_TY_HSO_LEFT_TO_RIGHT;
	enum b2r2_ty dst_vso = B2R2_TY_VSO_TOP_TO_BOTTOM;
	u32 fctl = 0;
	u32 rsf = 0;
	u32 rzi = 0;
	bool yuv_semi_planar = src_img->fmt == B2R2_BLT_FMT_YUV420_PACKED_SEMI_PLANAR ||
		src_img->fmt == B2R2_BLT_FMT_YUV422_PACKED_SEMI_PLANAR ||
		src_img->fmt == B2R2_BLT_FMT_YUV420_PACKED_SEMIPLANAR_MB_STE ||
		src_img->fmt == B2R2_BLT_FMT_YUV422_PACKED_SEMIPLANAR_MB_STE;

	pdebug("%s ENTRY\n", __func__);

	if ((req->user_req.flags &
			(B2R2_BLT_FLAG_SOURCE_FILL | B2R2_BLT_FLAG_SOURCE_FILL_RAW)) != 0) {
		enum b2r2_native_fmt fill_fmt = 0;
		u32 src_color = req->user_req.src_color;

		/* Determine format in src_color */
		switch(dst_img->fmt) {
		/* ARGB formats */
		case B2R2_BLT_FMT_16_BIT_ARGB4444:
		case B2R2_BLT_FMT_16_BIT_ARGB1555:
		case B2R2_BLT_FMT_16_BIT_RGB565:
		case B2R2_BLT_FMT_24_BIT_RGB888:
		case B2R2_BLT_FMT_32_BIT_ARGB8888:
		case B2R2_BLT_FMT_32_BIT_ABGR8888:
		case B2R2_BLT_FMT_24_BIT_ARGB8565:
		case B2R2_BLT_FMT_1_BIT_A1:
		case B2R2_BLT_FMT_8_BIT_A8:
			if ((req->user_req.flags & B2R2_BLT_FLAG_SOURCE_FILL) != 0) {
				fill_fmt = B2R2_NATIVE_ARGB8888;
			} else {
				/* SOURCE_FILL_RAW */
				fill_fmt = to_native_fmt(dst_img->fmt);
				if (dst_img->fmt == B2R2_BLT_FMT_32_BIT_ABGR8888) {
					/* Color is read from a register, where it is stored
					 * in ABGR format. Set up IVMX.
					 */
					node->node.GROUP0.B2R2_INS |= B2R2_INS_IVMX_ENABLED;
					node->node.GROUP15.B2R2_VMX0 = B2R2_VMX0_RGB_TO_BGR;
					node->node.GROUP15.B2R2_VMX1 = B2R2_VMX1_RGB_TO_BGR;
					node->node.GROUP15.B2R2_VMX2 = B2R2_VMX2_RGB_TO_BGR;
					node->node.GROUP15.B2R2_VMX3 = B2R2_VMX3_RGB_TO_BGR;
				}
			}
			break;
		case B2R2_BLT_FMT_YUV420_PACKED_PLANAR:
		case B2R2_BLT_FMT_YUV422_PACKED_PLANAR:
		case B2R2_BLT_FMT_Y_CB_Y_CR:
		case B2R2_BLT_FMT_CB_Y_CR_Y:
		case B2R2_BLT_FMT_YUV420_PACKED_SEMI_PLANAR:
		case B2R2_BLT_FMT_YUV422_PACKED_SEMI_PLANAR:
		case B2R2_BLT_FMT_YUV420_PACKED_SEMIPLANAR_MB_STE:
		case B2R2_BLT_FMT_YUV422_PACKED_SEMIPLANAR_MB_STE:
			/* Set up IVMX */
			node->node.GROUP0.B2R2_INS |= B2R2_INS_IVMX_ENABLED;
			node->node.GROUP15.B2R2_VMX0 = B2R2_VMX0_YUV_TO_RGB_601_VIDEO;
			node->node.GROUP15.B2R2_VMX1 = B2R2_VMX1_YUV_TO_RGB_601_VIDEO;
			node->node.GROUP15.B2R2_VMX2 = B2R2_VMX2_YUV_TO_RGB_601_VIDEO;
			node->node.GROUP15.B2R2_VMX3 = B2R2_VMX3_YUV_TO_RGB_601_VIDEO;
			if ((req->user_req.flags & B2R2_BLT_FLAG_SOURCE_FILL) != 0) {
				fill_fmt = B2R2_NATIVE_AYCBCR8888;
			} else {
				/* SOURCE_FILL_RAW */
				fill_fmt = to_native_fmt(dst_img->fmt);
			}
			break;
		default:
			src_color = 0;
			fill_fmt = B2R2_NATIVE_ARGB8888;
			break;
		}

		node->node.GROUP1.B2R2_TBA = out_buf->phys_addr;
		node->node.GROUP1.B2R2_TTY =
			(B2R2_GENERIC_WORK_BUF_PITCH << B2R2_TY_BITMAP_PITCH_SHIFT) |
			B2R2_GENERIC_WORK_BUF_FMT |
			B2R2_TY_ALPHA_RANGE_255 |
			B2R2_TY_HSO_LEFT_TO_RIGHT |
			B2R2_TY_VSO_TOP_TO_BOTTOM;
		/* Set color fill on SRC2 channel */
		node->node.GROUP4.B2R2_SBA = 0;//req->src_resolved.physical_address;
		node->node.GROUP4.B2R2_STY =
			(0 << B2R2_TY_BITMAP_PITCH_SHIFT) |
			fill_fmt |
			get_alpha_range(dst_img->fmt) |
			B2R2_TY_HSO_LEFT_TO_RIGHT |
			B2R2_TY_VSO_TOP_TO_BOTTOM;

		node->node.GROUP0.B2R2_CIC |= B2R2_CIC_COLOR_FILL;
		node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_2_COLOR_FILL_REGISTER;
		node->node.GROUP2.B2R2_S2CF = src_color;

		node->node.GROUP0.B2R2_ACK |= B2R2_ACK_MODE_BYPASS_S2_S3;

		pdebug("%s DONE\n", __func__);
		return;
	}

	if (src_img->pitch == 0) {
		/* Determine pitch based on format and width of the image. */
		src_pitch = get_pitch(src_img->fmt, src_img->width);
	} else {
		src_pitch = src_img->pitch;
	}

	pdebug("%s transform=%#010x\n", __func__, req->user_req.transform);
	if (req->user_req.transform & B2R2_BLT_TRANSFORM_CCW_ROT_90) {
		h_scf = (src_rect->width << 10) / dst_rect->height;
		v_scf = (src_rect->height << 10) / dst_rect->width;
	} else {
		h_scf = (src_rect->width << 10) / dst_rect->width;
		v_scf = (src_rect->height << 10) / dst_rect->height;
	}

	/* Configure horizontal rescale */
	if (h_scf != (1 << 10)) {
		pdebug("%s: Scaling horizontally by 0x%.8x"
				"\ns(%d, %d)->d(%d, %d)\n", __func__,
				h_scf, src_rect->width, src_rect->height,
				dst_rect->width, dst_rect->height);
	}
	fctl |= B2R2_FCTL_HF2D_MODE_ENABLE_RESIZER;
	rsf &= ~(0xffff << B2R2_RSF_HSRC_INC_SHIFT);
	rsf |= h_scf << B2R2_RSF_HSRC_INC_SHIFT;
	rzi |= B2R2_RZI_DEFAULT_HNB_REPEAT;

	/* Configure vertical rescale */
	if (v_scf != (1 << 10)) {
		pdebug("%s: Scaling vertically by 0x%.8x"
				"\ns(%d, %d)->d(%d, %d)\n", __func__,
				v_scf, src_rect->width, src_rect->height,
				dst_rect->width, dst_rect->height);
	}
	fctl |= B2R2_FCTL_VF2D_MODE_ENABLE_RESIZER;
	rsf &= ~(0xffff << B2R2_RSF_VSRC_INC_SHIFT);
	rsf |= v_scf << B2R2_RSF_VSRC_INC_SHIFT;
	rzi |= 2 << B2R2_RZI_VNB_REPEAT_SHIFT;

	/* Adjustments that depend on the source format */
	switch (src_img->fmt) {
	case B2R2_BLT_FMT_32_BIT_ABGR8888:
		/* Set up IVMX */
		node->node.GROUP0.B2R2_INS |= B2R2_INS_IVMX_ENABLED;

		node->node.GROUP15.B2R2_VMX0 = B2R2_VMX0_RGB_TO_BGR;
		node->node.GROUP15.B2R2_VMX1 = B2R2_VMX1_RGB_TO_BGR;
		node->node.GROUP15.B2R2_VMX2 = B2R2_VMX2_RGB_TO_BGR;
		node->node.GROUP15.B2R2_VMX3 = B2R2_VMX3_RGB_TO_BGR;
		break;
	/* Luma handled in the same way for the semi-planar formats */
	case B2R2_BLT_FMT_YUV420_PACKED_SEMI_PLANAR:
	case B2R2_BLT_FMT_YUV422_PACKED_SEMI_PLANAR:
	case B2R2_BLT_FMT_YUV420_PACKED_SEMIPLANAR_MB_STE:
	case B2R2_BLT_FMT_YUV422_PACKED_SEMIPLANAR_MB_STE: {
		/* Set the Luma rescale registers if YUV format is used.
		 */
		u32 rsf_luma = 0;
		u32 rzi_luma = 0;
		/* Set up IVMX */
		node->node.GROUP0.B2R2_INS |= B2R2_INS_IVMX_ENABLED;
		node->node.GROUP15.B2R2_VMX0 = B2R2_VMX0_YUV_TO_RGB_601_VIDEO;
		node->node.GROUP15.B2R2_VMX1 = B2R2_VMX1_YUV_TO_RGB_601_VIDEO;
		node->node.GROUP15.B2R2_VMX2 = B2R2_VMX2_YUV_TO_RGB_601_VIDEO;
		node->node.GROUP15.B2R2_VMX3 = B2R2_VMX3_YUV_TO_RGB_601_VIDEO;

		fctl |= B2R2_FCTL_LUMA_HF2D_MODE_ENABLE_RESIZER |
			B2R2_FCTL_LUMA_VF2D_MODE_ENABLE_RESIZER;

		rsf_luma |= h_scf << B2R2_RSF_HSRC_INC_SHIFT;
		rzi_luma |= B2R2_RZI_DEFAULT_HNB_REPEAT;

		rsf_luma |= v_scf << B2R2_RSF_VSRC_INC_SHIFT;
		rzi_luma |= 2 << B2R2_RZI_VNB_REPEAT_SHIFT;

		node->node.GROUP10.B2R2_RSF |= rsf_luma;
		node->node.GROUP10.B2R2_RZI |= rzi_luma;

		switch (src_img->fmt) {
		case B2R2_BLT_FMT_YUV420_PACKED_SEMI_PLANAR:
		case B2R2_BLT_FMT_YUV420_PACKED_SEMIPLANAR_MB_STE:
			/* Chrominance is always half the luminance size
			 * so chrominance resizer is always active.
			 */
			fctl |= B2R2_FCTL_HF2D_MODE_ENABLE_RESIZER |
				B2R2_FCTL_VF2D_MODE_ENABLE_RESIZER;

			rsf &= ~(0xffff << B2R2_RSF_HSRC_INC_SHIFT);
			rsf |= (h_scf >> 1) << B2R2_RSF_HSRC_INC_SHIFT;
			rsf &= ~(0xffff << B2R2_RSF_VSRC_INC_SHIFT);
			rsf |= (v_scf >> 1) << B2R2_RSF_VSRC_INC_SHIFT;
			break;
		case B2R2_BLT_FMT_YUV422_PACKED_SEMI_PLANAR:
		case B2R2_BLT_FMT_YUV422_PACKED_SEMIPLANAR_MB_STE:
			/* Chrominance is always half the luminance size
			 * only in horizontal direction.
			 */
			fctl |= B2R2_FCTL_HF2D_MODE_ENABLE_RESIZER |
				B2R2_FCTL_VF2D_MODE_ENABLE_RESIZER;

			rsf &= ~(0xffff << B2R2_RSF_HSRC_INC_SHIFT);
			rsf |= (h_scf >> 1) << B2R2_RSF_HSRC_INC_SHIFT;
			rsf &= ~(0xffff << B2R2_RSF_VSRC_INC_SHIFT);
			rsf |= v_scf << B2R2_RSF_VSRC_INC_SHIFT;
			break;
		default:
			break;
		}
		break;
	}
	default:
		break;
	}

	/* Set the filter control and rescale registers */
	node->node.GROUP8.B2R2_FCTL |= fctl;
	node->node.GROUP9.B2R2_RSF |= rsf;
	node->node.GROUP9.B2R2_RZI |= rzi;

	/* Flip transform is done before potential rotation.
	 * This can be achieved with appropriate scan order.
	 * Transform stage will only do rotation.
	 */
	if (req->user_req.transform & B2R2_BLT_TRANSFORM_FLIP_H) {
		dst_hso = B2R2_TY_HSO_RIGHT_TO_LEFT;
	}
	if (req->user_req.transform & B2R2_BLT_TRANSFORM_FLIP_V) {
		dst_vso = B2R2_TY_VSO_BOTTOM_TO_TOP;
	}

	/* Set target buffer */
	node->node.GROUP1.B2R2_TBA = out_buf->phys_addr;
	node->node.GROUP1.B2R2_TTY =
		(B2R2_GENERIC_WORK_BUF_PITCH << B2R2_TY_BITMAP_PITCH_SHIFT) |
		B2R2_GENERIC_WORK_BUF_FMT |
		B2R2_TY_ALPHA_RANGE_255 |
		dst_hso | dst_vso;

	/* Handle YUV formats */
	if (yuv_semi_planar)
	{
		/* Set up chrominance buffer on source 2, luminance on source 3.
		 * src_pitch and physical_address apply to luminance,
		 * corresponding chrominance values have to be derived.
		 * U and V are interleaved at half the luminance resolution,
		 * which makes the pitch of the UV plane equal to luminance pitch.
		 */
		u32 chroma_addr = req->src_resolved.physical_address +
			src_pitch * src_img->height;
		u32 chroma_pitch = src_pitch;

		enum b2r2_native_fmt src_fmt = to_native_fmt(src_img->fmt);
		enum b2r2_ty alpha_range = B2R2_TY_ALPHA_RANGE_255;//get_alpha_range(src_img->fmt);

		node->node.GROUP4.B2R2_SBA = chroma_addr;
		node->node.GROUP4.B2R2_STY =
			(chroma_pitch << B2R2_TY_BITMAP_PITCH_SHIFT) |
			src_fmt | alpha_range |
			B2R2_TY_HSO_LEFT_TO_RIGHT |
			B2R2_TY_VSO_TOP_TO_BOTTOM;

		node->node.GROUP5.B2R2_SBA = req->src_resolved.physical_address;
		node->node.GROUP5.B2R2_STY =
			(src_pitch << B2R2_TY_BITMAP_PITCH_SHIFT) |
			src_fmt | alpha_range |
			B2R2_TY_HSO_LEFT_TO_RIGHT |
			B2R2_TY_VSO_TOP_TO_BOTTOM;

		node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_3_FETCH_FROM_MEM;
	} else {
		/* Set source buffer on SRC2 channel */
		node->node.GROUP4.B2R2_SBA = req->src_resolved.physical_address;
		node->node.GROUP4.B2R2_STY =
			(src_pitch << B2R2_TY_BITMAP_PITCH_SHIFT) |
			to_native_fmt(src_img->fmt) |
			get_alpha_range(src_img->fmt) |
			B2R2_TY_HSO_LEFT_TO_RIGHT |
			B2R2_TY_VSO_TOP_TO_BOTTOM;
	}

	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_FILTER_CONTROL |
					B2R2_CIC_RESIZE_LUMA | B2R2_CIC_SOURCE_2 |
					B2R2_CIC_RESIZE_CHROMA;
	node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_2_FETCH_FROM_MEM |
			B2R2_INS_RESCALE2D_ENABLED;

	node->node.GROUP0.B2R2_ACK |= B2R2_ACK_MODE_BYPASS_S2_S3;

	pdebug("%s DONE\n", __func__);
}

static void setup_transform_stage(const struct b2r2_blt_request *req,
								  struct b2r2_node *node,
								  struct b2r2_work_buf *out_buf,
								  struct b2r2_work_buf *in_buf)
{
	/* horizontal and vertical scan order for out_buf */
	enum b2r2_ty dst_hso = B2R2_TY_HSO_LEFT_TO_RIGHT;
	enum b2r2_ty dst_vso = B2R2_TY_VSO_TOP_TO_BOTTOM;
	enum b2r2_blt_transform transform = req->user_req.transform;

	pdebug("%s ENTRY\n", __func__);
	if (transform & B2R2_BLT_TRANSFORM_CCW_ROT_90) {
		node->node.GROUP0.B2R2_INS |= B2R2_INS_ROTATION_ENABLED;
	}

	if (transform & B2R2_BLT_TRANSFORM_CCW_ROT_90) {
		/* Scan order must be flipped otherwise contents will
		 * be mirrored vertically. Leftmost column of in_buf
		 * would become top instead of bottom row of out_buf.
		 */
		dst_vso = B2R2_TY_VSO_BOTTOM_TO_TOP;
	}

	/* Set target buffer */
	node->node.GROUP1.B2R2_TBA = out_buf->phys_addr;
	node->node.GROUP1.B2R2_TTY =
		(B2R2_GENERIC_WORK_BUF_PITCH << B2R2_TY_BITMAP_PITCH_SHIFT) |
		B2R2_GENERIC_WORK_BUF_FMT |
		B2R2_TY_ALPHA_RANGE_255 |
		dst_hso | dst_vso;

	/* Set source buffer on SRC2 channel */
	node->node.GROUP4.B2R2_SBA = in_buf->phys_addr;
	node->node.GROUP4.B2R2_STY =
		(B2R2_GENERIC_WORK_BUF_PITCH << B2R2_TY_BITMAP_PITCH_SHIFT) |
		B2R2_GENERIC_WORK_BUF_FMT |
		B2R2_TY_ALPHA_RANGE_255 |
		B2R2_TY_HSO_LEFT_TO_RIGHT |
		B2R2_TY_VSO_TOP_TO_BOTTOM;

	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_2;
	node->node.GROUP0.B2R2_INS |= B2R2_INS_SOURCE_2_FETCH_FROM_MEM;
	node->node.GROUP0.B2R2_ACK |= B2R2_ACK_MODE_BYPASS_S2_S3;

	pdebug("%s DONE\n", __func__);
}

/*
static void setup_mask_stage(const struct b2r2_blt_request req,
							 struct b2r2_node *node,
							 struct b2r2_work_buf *out_buf,
							 struct b2r2_work_buf *in_buf);
*/

static void setup_dst_read_stage(const struct b2r2_blt_request *req,
								 struct b2r2_node *node,
								 struct b2r2_work_buf *out_buf)
{
	const struct b2r2_blt_img *dst_img = &(req->user_req.dst_img);
	u32 dst_pitch = 0;
	if (dst_img->pitch == 0) {
		/* Determine pitch based on format and width of the image. */
		dst_pitch = get_pitch(dst_img->fmt, dst_img->width);
	} else {
		dst_pitch = dst_img->pitch;
	}

	pdebug("%s ENTRY\n", __func__);

	if (dst_img->fmt == B2R2_BLT_FMT_32_BIT_ABGR8888) {
		pdebug("%s ABGR on dst_read\n", __func__);
		/* Set up IVMX */
		node->node.GROUP0.B2R2_INS |= B2R2_INS_IVMX_ENABLED;

		node->node.GROUP15.B2R2_VMX0 = B2R2_VMX0_RGB_TO_BGR;
		node->node.GROUP15.B2R2_VMX1 = B2R2_VMX1_RGB_TO_BGR;
		node->node.GROUP15.B2R2_VMX2 = B2R2_VMX2_RGB_TO_BGR;
		node->node.GROUP15.B2R2_VMX3 = B2R2_VMX3_RGB_TO_BGR;
	}

	/* Set target buffer */
	node->node.GROUP1.B2R2_TBA = out_buf->phys_addr;
	node->node.GROUP1.B2R2_TTY =
		(B2R2_GENERIC_WORK_BUF_PITCH << B2R2_TY_BITMAP_PITCH_SHIFT) |
		B2R2_GENERIC_WORK_BUF_FMT |
		B2R2_TY_ALPHA_RANGE_255 |
		B2R2_TY_HSO_LEFT_TO_RIGHT |
		B2R2_TY_VSO_TOP_TO_BOTTOM;

	/* Set source buffer on SRC2 channel */
	node->node.GROUP4.B2R2_SBA = req->dst_resolved.physical_address;
	node->node.GROUP4.B2R2_STY =
		(dst_pitch << B2R2_TY_BITMAP_PITCH_SHIFT) |
		to_native_fmt(dst_img->fmt) |
		get_alpha_range(dst_img->fmt) |
		B2R2_TY_HSO_LEFT_TO_RIGHT |
		B2R2_TY_VSO_TOP_TO_BOTTOM;

	node->node.GROUP0.B2R2_ACK |= B2R2_ACK_MODE_BYPASS_S2_S3;
	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_2;
	node->node.GROUP0.B2R2_INS |=
		B2R2_INS_SOURCE_2_FETCH_FROM_MEM;

	pdebug("%s DONE\n", __func__);
}

static void setup_blend_stage(const struct b2r2_blt_request *req,
							  struct b2r2_node *node,
							  struct b2r2_work_buf *bg_buf,
							  struct b2r2_work_buf *fg_buf)
{
	u32 global_alpha = req->user_req.global_alpha;
	pdebug("%s ENTRY\n", __func__);

	node->node.GROUP0.B2R2_ACK = 0;

	if (req->user_req.flags &
			(B2R2_BLT_FLAG_GLOBAL_ALPHA_BLEND |
			B2R2_BLT_FLAG_PER_PIXEL_ALPHA_BLEND)) {
		/* Some kind of blending needs to be done.
		 * It is not possible in B2R2 to disable per pixel blending
		 * and only use global alpha if the source color format
		 * contains embedded alpha.
		 */
		if ((req->user_req.flags & B2R2_BLT_FLAG_SRC_IS_NOT_PREMULT) != 0)
			node->node.GROUP0.B2R2_ACK |= B2R2_ACK_MODE_BLEND_NOT_PREMULT;
		else
			node->node.GROUP0.B2R2_ACK |= B2R2_ACK_MODE_BLEND_PREMULT;

		/* global_alpha register accepts 0..128 range,
		* global_alpha in the request is 0..255, remap needed */
		if ((req->user_req.flags & B2R2_BLT_FLAG_GLOBAL_ALPHA_BLEND) != 0) {
			if (global_alpha == 255) {
				global_alpha = 128;
			} else {
				global_alpha >>= 1;
			}
		} else {
			/* Use solid global_alpha if global alpha blending is not set */
			global_alpha = 128;
		}

		node->node.GROUP0.B2R2_ACK |=
			global_alpha << (B2R2_ACK_GALPHA_ROPID_SHIFT);

		/* Set background on SRC1 channel */
		node->node.GROUP3.B2R2_SBA = bg_buf->phys_addr;
		node->node.GROUP3.B2R2_STY =
			(B2R2_GENERIC_WORK_BUF_PITCH << B2R2_TY_BITMAP_PITCH_SHIFT) |
			B2R2_GENERIC_WORK_BUF_FMT |
			B2R2_TY_ALPHA_RANGE_255 |
			B2R2_TY_HSO_LEFT_TO_RIGHT |
			B2R2_TY_VSO_TOP_TO_BOTTOM;

		/* Set foreground on SRC2 channel */
		node->node.GROUP4.B2R2_SBA = fg_buf->phys_addr;
		node->node.GROUP4.B2R2_STY =
			(B2R2_GENERIC_WORK_BUF_PITCH << B2R2_TY_BITMAP_PITCH_SHIFT) |
			B2R2_GENERIC_WORK_BUF_FMT |
			B2R2_TY_ALPHA_RANGE_255 |
			B2R2_TY_HSO_LEFT_TO_RIGHT |
			B2R2_TY_VSO_TOP_TO_BOTTOM;

		/* Set target buffer */
		node->node.GROUP1.B2R2_TBA = bg_buf->phys_addr;
		node->node.GROUP1.B2R2_TTY =
			(B2R2_GENERIC_WORK_BUF_PITCH << B2R2_TY_BITMAP_PITCH_SHIFT) |
			B2R2_GENERIC_WORK_BUF_FMT |
			B2R2_TY_ALPHA_RANGE_255 |
			B2R2_TY_HSO_LEFT_TO_RIGHT |
			B2R2_TY_VSO_TOP_TO_BOTTOM;

		node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_1 | B2R2_CIC_SOURCE_2;
		node->node.GROUP0.B2R2_INS |=
			B2R2_INS_SOURCE_1_FETCH_FROM_MEM |
			B2R2_INS_SOURCE_2_FETCH_FROM_MEM;
	} else {
		/* No blending, foreground goes on SRC2. No global alpha.
		 * EMACSOC TODO: The blending stage should be skipped altogether
		 * if no blending is to be done. Probably could go directly from
		 * transform to writeback.
		 */
		node->node.GROUP0.B2R2_ACK |= B2R2_ACK_MODE_BYPASS_S2_S3;
		node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_2;
		node->node.GROUP0.B2R2_INS |=
			B2R2_INS_SOURCE_2_FETCH_FROM_MEM;

		node->node.GROUP4.B2R2_SBA = fg_buf->phys_addr;
		node->node.GROUP4.B2R2_STY =
			(B2R2_GENERIC_WORK_BUF_PITCH << B2R2_TY_BITMAP_PITCH_SHIFT) |
			B2R2_GENERIC_WORK_BUF_FMT |
			B2R2_TY_ALPHA_RANGE_255 |
			B2R2_TY_HSO_LEFT_TO_RIGHT |
			B2R2_TY_VSO_TOP_TO_BOTTOM;

		node->node.GROUP1.B2R2_TBA = bg_buf->phys_addr;
		node->node.GROUP1.B2R2_TTY =
			(B2R2_GENERIC_WORK_BUF_PITCH << B2R2_TY_BITMAP_PITCH_SHIFT) |
			B2R2_GENERIC_WORK_BUF_FMT |
			B2R2_TY_ALPHA_RANGE_255 |
			B2R2_TY_HSO_LEFT_TO_RIGHT |
			B2R2_TY_VSO_TOP_TO_BOTTOM;
	}

	pdebug("%s DONE\n", __func__);
}

static void setup_writeback_stage(const struct b2r2_blt_request *req,
								  struct b2r2_node *node,
								  struct b2r2_work_buf *in_buf)
{
	const struct b2r2_blt_img *dst_img = &(req->user_req.dst_img);
	u32 dst_pitch = 0;
	if (dst_img->pitch == 0) {
		/* Determine pitch based on format and width of the image. */
		dst_pitch = get_pitch(dst_img->fmt, dst_img->width);
	} else {
		dst_pitch = dst_img->pitch;
	}

	pdebug("%s ENTRY\n", __func__);

	if (dst_img->fmt == B2R2_BLT_FMT_32_BIT_ABGR8888) {
		pdebug("%s ABGR on writeback\n", __func__);
		/* Set up OVMX */
		node->node.GROUP0.B2R2_INS |= B2R2_INS_OVMX_ENABLED;

		node->node.GROUP16.B2R2_VMX0 = B2R2_VMX0_RGB_TO_BGR;
		node->node.GROUP16.B2R2_VMX1 = B2R2_VMX1_RGB_TO_BGR;
		node->node.GROUP16.B2R2_VMX2 = B2R2_VMX2_RGB_TO_BGR;
		node->node.GROUP16.B2R2_VMX3 = B2R2_VMX3_RGB_TO_BGR;
	}

	 /* bypass ALU, use SRC2 for clipping */
	node->node.GROUP0.B2R2_ACK = B2R2_ACK_MODE_BYPASS_S2_S3;
	node->node.GROUP0.B2R2_CIC |= B2R2_CIC_SOURCE_2;
	node->node.GROUP0.B2R2_INS |=
		B2R2_INS_SOURCE_2_FETCH_FROM_MEM | B2R2_INS_RECT_CLIP_ENABLED;

	/* Set target buffer */
	node->node.GROUP1.B2R2_TBA = req->dst_resolved.physical_address;
	node->node.GROUP1.B2R2_TTY =
		(dst_pitch << B2R2_TY_BITMAP_PITCH_SHIFT) |
		to_native_fmt(dst_img->fmt) |
		get_alpha_range(dst_img->fmt) |
		B2R2_TY_HSO_LEFT_TO_RIGHT |
		B2R2_TY_VSO_TOP_TO_BOTTOM;

	/* Set source buffer on SRC2 channel */
	node->node.GROUP4.B2R2_SBA = in_buf->phys_addr;
	node->node.GROUP4.B2R2_STY =
		(B2R2_GENERIC_WORK_BUF_PITCH << B2R2_TY_BITMAP_PITCH_SHIFT) |
		B2R2_GENERIC_WORK_BUF_FMT |
		B2R2_TY_ALPHA_RANGE_255 |
		B2R2_TY_HSO_LEFT_TO_RIGHT |
		B2R2_TY_VSO_TOP_TO_BOTTOM;

	pdebug("%s DONE\n", __func__);
}

/*******************
 * Public functions
 *******************/
int b2r2_generic_analyze(const struct b2r2_blt_request *req,
						 s32 *work_buf_width,
						 s32 *work_buf_height,
						 u32 *work_buf_count,
						 u32 *node_count)
{
	/* Need at least 4 nodes, read or fill input, read dst, blend
	 * and write back the result */
	u32 n_nodes = 4;
	/* Need at least 2 bufs, 1 for blend output and 1 for input */
	u32 n_work_bufs = 2;
	/* Horizontal and vertical scaling factors in 6.10 fixed point format */
	s32 h_scf = 1 << 10;
	s32 v_scf = 1 << 10;
	bool is_src_fill = (req->user_req.flags &
			(B2R2_BLT_FLAG_SOURCE_FILL | B2R2_BLT_FLAG_SOURCE_FILL_RAW)) != 0;

	struct b2r2_blt_rect src_rect;
	struct b2r2_blt_rect dst_rect;

	if (req == NULL || work_buf_width == NULL || work_buf_height == NULL ||
			work_buf_count == NULL || node_count == NULL) {
		err_msg("%s Error: Invalid in or out pointers:\n"
				"req=0x%p\n"
				"work_buf_width=0x%p work_buf_height=0x%p work_buf_count=0x%p\n"
				"node_count=0x%p.\n",
				__func__,
				req,
				work_buf_width, work_buf_height, work_buf_count,
				node_count);
		return -EINVAL;
	}

	*node_count = 0;
	*work_buf_width = 0;
	*work_buf_height = 0;
	*work_buf_count = 0;

	if (req->user_req.transform != B2R2_BLT_TRANSFORM_NONE) {
		n_nodes++;
		n_work_bufs++;
	}

	/* EMACSOC TODO: Check for unsupported formats. */

	if ((req->user_req.flags & B2R2_BLT_FLAG_SOURCE_COLOR_KEY) != 0 &&
			(req->user_req.flags & B2R2_BLT_FLAG_DEST_COLOR_KEY) != 0) {
		printk(KERN_INFO "%s Error: Invalid combination: source and "
				"destination color keying.\n", __func__);
		return -EINVAL;
	}

	if ((req->user_req.flags &
			(B2R2_BLT_FLAG_SOURCE_FILL |
			B2R2_BLT_FLAG_SOURCE_FILL_RAW)) &&
			(req->user_req.flags &
			(B2R2_BLT_FLAG_SOURCE_COLOR_KEY |
			B2R2_BLT_FLAG_DEST_COLOR_KEY))) {
		printk(KERN_INFO "%s Error: Invalid combination: source_fill and color keying.\n",
				__func__);
		return -EINVAL;
	}

	if ((req->user_req.flags &
			(B2R2_BLT_FLAG_PER_PIXEL_ALPHA_BLEND |
			B2R2_BLT_FLAG_GLOBAL_ALPHA_BLEND)) &&
			(req->user_req.flags &
			(B2R2_BLT_FLAG_DEST_COLOR_KEY |
			B2R2_BLT_FLAG_SOURCE_COLOR_KEY))) {
		printk(KERN_INFO "%s Error: Invalid combination: blending and color keying.\n",
				__func__);
		return -EINVAL;
	}

	if ((req->user_req.flags & B2R2_BLT_FLAG_SOURCE_MASK) &&
			(req->user_req.flags &
			(B2R2_BLT_FLAG_DEST_COLOR_KEY |
			B2R2_BLT_FLAG_SOURCE_COLOR_KEY))) {
		printk(KERN_INFO "%s Error: Invalid combination: source mask and color keying.\n",
				__func__);
		return -EINVAL;
	}

	if (req->user_req.flags &
			(B2R2_BLT_FLAG_DEST_COLOR_KEY |
			B2R2_BLT_FLAG_SOURCE_COLOR_KEY |
			B2R2_BLT_FLAG_SOURCE_MASK)) {
		printk(KERN_INFO "%s Error: Unsupported: source mask, color keying.\n", __func__);
		return -ENOSYS;
	}

	/* Check for invalid dimensions that would hinder scale calculations */
	src_rect = req->user_req.src_rect;
	dst_rect = req->user_req.dst_rect;
	/* Check for invalid src_rect unless src_fill is enabled */
	if (!is_src_fill && (src_rect.x < 0 || src_rect.y < 0 ||
		src_rect.x + src_rect.width > req->user_req.src_img.width ||
		src_rect.y + src_rect.height > req->user_req.src_img.height)) {
		printk(KERN_INFO "%s Error: src_rect outside src_img:\n"
				"src(x,y,w,h)=(%d, %d, %d, %d) src_img(w,h)=(%d, %d).\n",
				__func__,
				src_rect.x, src_rect.y, src_rect.width, src_rect.height,
				req->user_req.src_img.width, req->user_req.src_img.height);
		return -EINVAL;
	}

	if (!is_src_fill && (src_rect.width <= 0 || src_rect.height <= 0)) {
		printk(KERN_INFO "%s Error: Invalid source dimensions:\n"
				"src(w,h)=(%d, %d).\n",
				__func__,
				src_rect.width, src_rect.height);
		return -EINVAL;
	}

	if (dst_rect.width <= 0 || dst_rect.height <= 0) {
		printk(KERN_INFO "%s Error: Invalid dest dimensions:\n"
				"dst(w,h)=(%d, %d).\n",
				__func__,
				dst_rect.width, dst_rect.height);
		return -EINVAL;
	}

	/* Check for invalid image params */
	if (!is_src_fill && validate_buf(&(req->user_req.src_img)))
		return -EINVAL;

	if (validate_buf(&(req->user_req.dst_img)))
		return -EINVAL;

	if (is_src_fill) {
		/* Params correct for a source fill operation.
		 * No need for further checking. */
		*work_buf_width = B2R2_GENERIC_WORK_BUF_WIDTH;
		*work_buf_height = B2R2_GENERIC_WORK_BUF_HEIGHT;
		*work_buf_count = n_work_bufs;
		*node_count = n_nodes;
		/* EMACSOC TODO: Account for multi-buffer format during writeback stage */
		pdebug("%s DONE buf_w=%d buf_h=%d buf_count=%d node_count=%d\n",
				__func__,
				*work_buf_width, *work_buf_height, *work_buf_count, *node_count);
		return 0;
	}

	/*
	 * Calculate scaling factors, all transform enum values
	 * that include rotation have the CCW_ROT_90 bit set.
	 */
	if (req->user_req.transform & B2R2_BLT_TRANSFORM_CCW_ROT_90) {
		h_scf = (src_rect.width << 10) / dst_rect.height;
		v_scf = (src_rect.height << 10) / dst_rect.width;
	} else {
		h_scf = (src_rect.width << 10) / dst_rect.width;
		v_scf = (src_rect.height << 10) / dst_rect.height;
	}

	/*
	 * Check for degenerate/out_of_range scaling factors.
	 */
	if (h_scf <= 0 || v_scf <= 0 || h_scf > 0x7C00 || v_scf > 0x7C00) {
		err_msg("%s Error: Dimensions result in degenerate or "
				"out of range scaling:\n"
				"src(w,h)=(%d, %d) "
				"dst(w,h)=(%d,%d).\n"
				"h_scf=0x%.8x, v_scf=0x%.8x\n",
				__func__,
				src_rect.width, src_rect.height,
				dst_rect.width, dst_rect.height,
				h_scf, v_scf);
		return -EINVAL;
	}

	*work_buf_width = B2R2_GENERIC_WORK_BUF_WIDTH;
	*work_buf_height = B2R2_GENERIC_WORK_BUF_HEIGHT;
	*work_buf_count = n_work_bufs;
	*node_count = n_nodes;
	/* EMACSOC TODO: Account for multi-buffer format during writeback stage */
	pdebug("%s DONE buf_w=%d buf_h=%d buf_count=%d node_count=%d\n",
			__func__,
			*work_buf_width, *work_buf_height, *work_buf_count, *node_count);
	return 0;
}

/*
 *
 */
int b2r2_generic_configure(const struct b2r2_blt_request *req,
						   struct b2r2_node *first,
						   struct b2r2_work_buf *tmp_bufs,
						   u32 buf_count)
{
	struct b2r2_node *node = NULL;
	struct b2r2_work_buf *in_buf = NULL;
	struct b2r2_work_buf *out_buf = NULL;
	struct b2r2_work_buf *empty_buf = NULL;

#ifdef B2R2_GENERIC_DEBUG
	u32 needed_bufs = 0;
	u32 needed_nodes = 0;
	s32 work_buf_width = 0;
	s32 work_buf_height = 0;
	int invalid_req = b2r2_generic_analyze(req, &work_buf_width, &work_buf_height,
											&needed_bufs, &needed_nodes);
	if (invalid_req < 0) {
		err_msg("%s Error: Invalid request supplied, error=%d\n",
				__func__, invalid_req);
		return -EINVAL;
	} else {
		u32 n_nodes = 0;
		struct b2r2_node *node = first;
		while (node != NULL) {
			n_nodes++;
			node = node->next;
		}
		if (n_nodes < needed_nodes) {
			err_msg("%s Error: Not enough nodes %d < %d.\n",
					__func__, n_nodes, needed_nodes);
			return -EINVAL;
		}

		if (buf_count < needed_bufs) {
			err_msg("%s Error: Not enough buffers %d < %d.\n",
					__func__, buf_count, needed_bufs);
			return -EINVAL;
		}
	}
#endif

	reset_nodes(first);
	node = first;
	empty_buf = tmp_bufs;
	out_buf = empty_buf;
	empty_buf++;
	/* Prepare input tile. Color_fill or read from src */
	setup_input_stage(req, node, out_buf);
	in_buf = out_buf;
	out_buf = empty_buf;
	empty_buf++;
	node = node->next;

	if ((req->user_req.transform & B2R2_BLT_TRANSFORM_CCW_ROT_90) != 0) {
		setup_transform_stage(req, node, out_buf, in_buf);
		node = node->next;
		in_buf = out_buf;
		out_buf = empty_buf++;
	}
	/* EMACSOC TODO: mask */
	/*
	if (req->user_req.flags & B2R2_BLT_FLAG_SOURCE_MASK) {
		setup_mask_stage(req, node, out_buf, in_buf);
		node = node->next;
		in_buf = out_buf;
		out_buf = empty_buf++;
	}
	*/
	/* Read the part of destination that will be updated */
	setup_dst_read_stage(req, node, out_buf);
	node = node->next;
	setup_blend_stage(req, node, out_buf, in_buf);
	node = node->next;
	in_buf = out_buf;
	setup_writeback_stage(req, node, in_buf);
	/* Terminate the program chain */
	node->node.GROUP0.B2R2_NIP = 0;
	return 0;
}

void b2r2_generic_set_areas(const struct b2r2_blt_request *req,
							struct b2r2_node *first,
							struct b2r2_blt_rect *dst_rect_area)
{
	/* Nodes come in the following order: <input stage>, [transform], [src_mask]
	 * <dst_read>, <blend>, <writeback>
	 */
	struct b2r2_node *node = first;
	const struct b2r2_blt_rect *dst_rect = &(req->user_req.dst_rect);
	const struct b2r2_blt_rect *src_rect = &(req->user_req.src_rect);
	const enum b2r2_blt_fmt src_fmt = req->user_req.src_img.fmt;
	bool yuv_semi_planar =
		src_fmt == B2R2_BLT_FMT_YUV420_PACKED_SEMI_PLANAR ||
		src_fmt == B2R2_BLT_FMT_YUV422_PACKED_SEMI_PLANAR ||
		src_fmt == B2R2_BLT_FMT_YUV420_PACKED_SEMIPLANAR_MB_STE ||
		src_fmt == B2R2_BLT_FMT_YUV422_PACKED_SEMIPLANAR_MB_STE;
	s32 h_scf = 1 << 10;
	s32 v_scf = 1 << 10;
	s32 src_x = 0;
	s32 src_y = 0;
	s32 src_w = 0;
	s32 src_h = 0;
	u32 b2r2_rzi = 0;
	s32 clip_top = 0;
	s32 clip_left = 0;
	s32 clip_bottom = req->user_req.dst_img.height - 1;
	s32 clip_right = req->user_req.dst_img.width - 1;
	/* Dst coords inside the dst_rect, not the buffer */
	s32 dst_x = dst_rect_area->x;
	s32 dst_y = dst_rect_area->y;

	pdebug_areas("%s ENTRY\n", __func__);

	if (req->user_req.transform & B2R2_BLT_TRANSFORM_CCW_ROT_90) {
		h_scf = (src_rect->width << 10) / dst_rect->height;
		v_scf = (src_rect->height << 10) / dst_rect->width;
	} else {
		h_scf = (src_rect->width << 10) / dst_rect->width;
		v_scf = (src_rect->height << 10) / dst_rect->height;
	}

	if (req->user_req.transform & B2R2_BLT_TRANSFORM_CCW_ROT_90) {
		/* Normally the inverse transform for 90 degree rotation is given by
		 * | 0  1|   |x|   | y|
		 * |     | X | | = |  |
		 * |-1  0|   |y|   |-x|
		 * but screen coordinates are flipped in y direction
		 * (compared to usual Cartesian coordinates), hence the offsets.
		 */
		src_x = (dst_rect->height - dst_y - dst_rect_area->height) * h_scf;
		src_y = dst_x * v_scf;
		src_w = dst_rect_area->height * h_scf;
		src_h = dst_rect_area->width * v_scf;
	} else {
		src_x = dst_x * h_scf;
		src_y = dst_y * v_scf;
		src_w = dst_rect_area->width * h_scf;
		src_h = dst_rect_area->height * v_scf;
	}

	b2r2_rzi |= ((src_x & 0x3ff) << B2R2_RZI_HSRC_INIT_SHIFT) |
		((src_y & 0x3ff) << B2R2_RZI_VSRC_INIT_SHIFT);

	/* src_w must contain all the pixels that contribute
	 * to a particular tile.
	 * ((x + 0x3ff) >> 10) is equivalent to ceiling(x),
	 * expressed in 6.10 fixed point format.
	 * Every destination tile, maps to a certain area in the source
	 * rectangle. The area in source will most likely not be a rectangle
	 * with exact integer dimensions whenever arbitrary scaling is involved.
	 * Consider the following example.
	 * Suppose, that width of the current destination tile maps
	 * to 1.7 pixels in source, starting at x == 5.4, as calculated
	 * using the scaling factor.
	 * This means that while the destination tile is written,
	 * the source should be read from x == 5.4 up to x == 5.4 + 1.7 == 7.1
	 * Consequently, color from 3 pixels (x == 5, 6 and 7)
	 * needs to be read from source.
	 * The formula below the comment yields:
	 * ceil(0.4 + 1.7) == ceil(2.1) == 3
	 * (src_x & 0x3ff) is the fractional part of src_x,
	 * which is expressed in 6.10 fixed point format.
	 * Thus, width of the source area should be 3 pixels wide,
	 * starting at x == 5.
	 * However, the reading should not start at x == 5.0
	 * but a bit inside, namely x == 5.4
	 * The B2R2_RZI register is used to instruct the HW to do so.
	 * It contains the fractional part that will be added to
	 * the first pixel coordinate, before incrementing the current source
	 * coordinate with the step specified in B2R2_RSF register.
	 * The same applies to scaling in vertical direction.
	 */
	src_w = ((src_x & 0x3ff) + src_w + 0x3ff) >> 10;
	src_h = ((src_y & 0x3ff) + src_h + 0x3ff) >> 10;

	/* EMACSOC TODO: Remove this debug clamp */
	if (src_w > 128)
		src_w = 128;

	src_x >>= 10;
	src_y >>= 10;

	if (req->user_req.transform & B2R2_BLT_TRANSFORM_FLIP_H) {
		src_x = src_rect->width - src_x - src_w;
	}

	if (req->user_req.transform & B2R2_BLT_TRANSFORM_FLIP_V) {
		src_y = src_rect->height - src_y - src_h;
	}

	/* Translate the src/dst_rect coordinates into true
	 * src/dst_buffer coordinates
	 */
	src_x += src_rect->x;
	src_y += src_rect->y;

	dst_x += dst_rect->x;
	dst_y += dst_rect->y;

	/* Clamp the src coords to buffer dimensions
	 * to prevent illegal reads.
	 */
	if (src_x < 0) {
		src_x = 0;
	}
	if (src_y < 0) {
		src_y = 0;
	}
	if ((src_x + src_w) > req->user_req.src_img.width) {
		src_w = req->user_req.src_img.width - src_x;
	}
	if ((src_y + src_h) > req->user_req.src_img.height) {
		src_h = req->user_req.src_img.height - src_y;
	}

	/******************
	 * The input node
	 ******************/
	if (yuv_semi_planar) {
		/* Luma on SRC3 */
		node->node.GROUP5.B2R2_SXY = ((src_x & 0xffff) << B2R2_XY_X_SHIFT) |
			((src_y & 0xffff) << B2R2_XY_Y_SHIFT);
		node->node.GROUP5.B2R2_SSZ = ((src_w & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
			((src_h & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);

		/* Clear and set only the SRC_INIT bits */
		node->node.GROUP10.B2R2_RZI &= ~((0x3ff << B2R2_RZI_HSRC_INIT_SHIFT) |
										(0x3ff << B2R2_RZI_VSRC_INIT_SHIFT));
		node->node.GROUP10.B2R2_RZI |= b2r2_rzi;

		node->node.GROUP9.B2R2_RZI &= ~((0x3ff << B2R2_RZI_HSRC_INIT_SHIFT) |
										(0x3ff << B2R2_RZI_VSRC_INIT_SHIFT));
		/* Chroma goes on SRC2.
		 * Chroma is half the size of luma. Must round up the chroma size
		 * to handle cases when luma size is not divisible by 2.
		 * E.g. luma width==7 requires chroma width==4.
		 * Chroma width==7/2==3 is only enough for luma width==6.
		 */
		node->node.GROUP4.B2R2_SXY = (((src_x & 0xffff) >> 1) << B2R2_XY_X_SHIFT) |
			(((src_y & 0xffff) >> 1) << B2R2_XY_Y_SHIFT);
		node->node.GROUP4.B2R2_SSZ = ((((src_w + 1)& 0xfff) >> 1) << B2R2_SZ_WIDTH_SHIFT) |
			((((src_h + 1) & 0xfff) >> 1) << B2R2_SZ_HEIGHT_SHIFT);

		node->node.GROUP9.B2R2_RZI |= (b2r2_rzi >> 1) &
			((0x3ff << B2R2_RZI_HSRC_INIT_SHIFT) |
			 (0x3ff << B2R2_RZI_VSRC_INIT_SHIFT));
	} else {
		node->node.GROUP4.B2R2_SXY = ((src_x & 0xffff) << B2R2_XY_X_SHIFT) |
			((src_y & 0xffff) << B2R2_XY_Y_SHIFT);
		node->node.GROUP4.B2R2_SSZ = ((src_w & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
			((src_h & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);

		/* Clear and set only the SRC_INIT bits */
		node->node.GROUP9.B2R2_RZI &= ~((0x3ff << B2R2_RZI_HSRC_INIT_SHIFT) |
										(0x3ff << B2R2_RZI_VSRC_INIT_SHIFT));
		node->node.GROUP9.B2R2_RZI |= b2r2_rzi;
	}

	node->node.GROUP1.B2R2_TXY = 0;
	if (req->user_req.transform & B2R2_BLT_TRANSFORM_CCW_ROT_90) {
		/* dst_rect_area coordinates are specified after potential rotation.
		 * Input is read before rotation, hence the width and height
		 * need to be swapped.
		 * Horizontal and vertical flips are accomplished with
		 * suitable scanning order while writing to the temporary buffer.
		 */
		if ((req->user_req.transform & B2R2_BLT_TRANSFORM_FLIP_H) != 0) {
			node->node.GROUP1.B2R2_TXY |=
				((dst_rect_area->height - 1) & 0xffff) << B2R2_XY_X_SHIFT;
		}

		if (req->user_req.transform & B2R2_BLT_TRANSFORM_FLIP_V) {
			node->node.GROUP1.B2R2_TXY |=
				((dst_rect_area->width - 1) & 0xffff) << B2R2_XY_Y_SHIFT;
		}

		node->node.GROUP1.B2R2_TSZ =
				((dst_rect_area->height & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
				((dst_rect_area->width & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);
	} else {
		if ((req->user_req.transform & B2R2_BLT_TRANSFORM_FLIP_H) != 0) {
			node->node.GROUP1.B2R2_TXY |=
				((dst_rect_area->width - 1) & 0xffff) << B2R2_XY_X_SHIFT;
		}

		if ((req->user_req.transform & B2R2_BLT_TRANSFORM_FLIP_V) != 0) {
			node->node.GROUP1.B2R2_TXY |=
				((dst_rect_area->height - 1) & 0xffff) << B2R2_XY_Y_SHIFT;
		}

		node->node.GROUP1.B2R2_TSZ =
				((dst_rect_area->width & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
				((dst_rect_area->height & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);
	}

	if (req->user_req.flags &
		(B2R2_BLT_FLAG_SOURCE_FILL | B2R2_BLT_FLAG_SOURCE_FILL_RAW)) {
		/* Scan order for source fill should always be left-to-right
		 * and top-to-bottom. Fill the input tile from top left.
		 */
		node->node.GROUP1.B2R2_TXY = 0;
		node->node.GROUP4.B2R2_SSZ = node->node.GROUP1.B2R2_TSZ;
	}

	pdebug_areas("%s Input node done.\n", __func__);

	/******************
	 * Transform
	 ******************/
	if ((req->user_req.transform & B2R2_BLT_TRANSFORM_CCW_ROT_90) != 0) {
		/* Transform node operates on temporary buffers.
		 * Content always at top left, but scanning order
		 * has to be flipped during rotation.
		 * Width and height need to be considered as well, since
		 * a tile may not necessarily be filled completely.
		 * dst_rect_area dimensions are specified after potential rotation.
		 * Input is read before rotation, hence the width and height
		 * need to be swapped on src.
		 */
		node = node->next;

		node->node.GROUP4.B2R2_SXY = 0;
		node->node.GROUP4.B2R2_SSZ =
				((dst_rect_area->height & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
				((dst_rect_area->width & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);
		/* Bottom line written first */
		node->node.GROUP1.B2R2_TXY =
			((dst_rect_area->height - 1) & 0xffff) << B2R2_XY_Y_SHIFT;

		node->node.GROUP1.B2R2_TSZ =
				((dst_rect_area->width & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
				((dst_rect_area->height & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);
		pdebug_areas("%s Tranform node done.\n", __func__);
	}

	/******************
	 * Source mask
	 ******************/
	if (req->user_req.flags & B2R2_BLT_FLAG_SOURCE_MASK) {
		node = node->next;
		/* Same coords for mask as for the input stage.
		 * Should the mask be transformed together with source?
		 * EMACSOC TODO: Apply mask before any transform/scaling is done. Otherwise
		 * it will be dst_ not src_mask.
		 */
		pdebug_areas("%s Source mask node done.\n", __func__);
	}

	/**************************
	 * dst_read Use SRC1
	 **************************/
	node = node->next;
	node->node.GROUP4.B2R2_SXY =
			((dst_x & 0xffff) << B2R2_XY_X_SHIFT) |
			((dst_y & 0xffff) << B2R2_XY_Y_SHIFT);
	node->node.GROUP4.B2R2_SSZ =
			((dst_rect_area->width & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
			((dst_rect_area->height & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);
	node->node.GROUP1.B2R2_TXY = 0;
	node->node.GROUP1.B2R2_TSZ =
			((dst_rect_area->width & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
			((dst_rect_area->height & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);

	pdebug_areas("%s dst_read node done.\n", __func__);

	/*************
	 * blend
	 ************/
	node = node->next;
	node->node.GROUP3.B2R2_SXY = 0;
	node->node.GROUP3.B2R2_SSZ =
			((dst_rect_area->width & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
			((dst_rect_area->height & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);
	/* contents of the foreground temporary buffer always at top left */
	node->node.GROUP4.B2R2_SXY = 0;
	node->node.GROUP4.B2R2_SSZ =
			((dst_rect_area->width & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
			((dst_rect_area->height & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);

	node->node.GROUP1.B2R2_TXY = 0;
	node->node.GROUP1.B2R2_TSZ =
			((dst_rect_area->width & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
			((dst_rect_area->height & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);

	pdebug_areas("%s Blend node done.\n", __func__);

	/*************
	 * writeback
	 *************/
	/* EMACSOC TODO: Perhaps move the final clipping outside
	 * the pipeline. No need to process data that will be clipped here.
	 */
	node = node->next;
	if ((req->user_req.flags & B2R2_BLT_FLAG_DESTINATION_CLIP) != 0) {
		clip_left = req->user_req.dst_clip_rect.x;
		clip_top = req->user_req.dst_clip_rect.y;
		clip_right = clip_left + req->user_req.dst_clip_rect.width - 1;
		clip_bottom = clip_top + req->user_req.dst_clip_rect.height - 1;
	}
	/* Clamp the dst clip rectangle to buffer dimensions
	 * to prevent illegal writes. An illegal clip rectangle,
	 * e.g. outside the buffer will be ignored, resulting
	 * in nothing being clipped.
	 * EMACSOC TODO: It is not possible to set up
	 * a clip rectangle in B2R2 that will clip everything, which is
	 * what one might expect to happen when setting a clip rectangle
	 * outside the buffer for instance. Fix that. Perhaps by moving
	 * clipping out of the pipeline?
	 */
	if (clip_left < 0 || req->user_req.dst_img.width <= clip_left) {
		clip_left = 0;
	}

	if (clip_top < 0 || req->user_req.dst_img.height <= clip_top) {
		clip_top = 0;
	}

	if (clip_right < 0 || req->user_req.dst_img.width <= clip_right) {
		clip_right = req->user_req.dst_img.width - 1;
	}

	if (clip_bottom < 0 || req->user_req.dst_img.height <= clip_bottom) {
		clip_bottom = req->user_req.dst_img.height - 1;
	}

	/* Only allow writing inside the clip rect.
	 * INTNL bit in B2R2_CWO should be zero. */
	node->node.GROUP6.B2R2_CWO =
		((clip_top & 0x7fff) << B2R2_CWO_Y_SHIFT) |
		((clip_left & 0x7fff) << B2R2_CWO_X_SHIFT);
	node->node.GROUP6.B2R2_CWS =
		((clip_bottom & 0x7fff) << B2R2_CWS_Y_SHIFT) |
		((clip_right & 0x7fff) << B2R2_CWS_X_SHIFT);

	node->node.GROUP4.B2R2_SXY = 0;
	node->node.GROUP4.B2R2_SSZ =
			((dst_rect_area->width & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
			((dst_rect_area->height & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);
	node->node.GROUP1.B2R2_TXY =
			((dst_x & 0xffff) << B2R2_XY_X_SHIFT) |
			((dst_y & 0xffff) << B2R2_XY_Y_SHIFT);
	node->node.GROUP1.B2R2_TSZ =
			((dst_rect_area->width & 0xfff) << B2R2_SZ_WIDTH_SHIFT) |
			((dst_rect_area->height & 0xfff) << B2R2_SZ_HEIGHT_SHIFT);

	pdebug_areas("%s Writeback node done.\n", __func__);
	pdebug_areas("%s DONE\n", __func__);
	dump_nodes(first);
}

#undef B2R2_GENERIC_WORK_BUF_WIDTH
#undef B2R2_GENERIC_WORK_BUF_HEIGHT
#undef B2R2_GENERIC_WORK_BUF_PITCH
#undef B2R2_GENERIC_WORK_BUF_FMT
#undef B2R2_GENERIC_DEBUG
