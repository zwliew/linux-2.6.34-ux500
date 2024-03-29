#ifndef _LINUX_VIDEO_B2R2_GENERIC_H
#define _LINUX_VIDEO_B2R2_GENERIC_H

#include "b2r2_internal.h"
#include "b2r2_blt.h"

/**
 * b2r2_generic_analyze()
 */
int b2r2_generic_analyze(const struct b2r2_blt_request *req,
						 s32 *work_buf_width,
						 s32 *work_buf_height,
						 u32 *work_buf_count,
						 u32 *node_count);
/**
 * b2r2_generic_configure()
 */
int b2r2_generic_configure(const struct b2r2_blt_request *req,
						   struct b2r2_node *first,
						   struct b2r2_work_buf *tmp_bufs,
						   u32 buf_count);
/**
 * b2r2_generic_set_areas()
 */
void b2r2_generic_set_areas(const struct b2r2_blt_request *req,
							struct b2r2_node *first,
							struct b2r2_blt_rect *dst_rect_area);
#endif
