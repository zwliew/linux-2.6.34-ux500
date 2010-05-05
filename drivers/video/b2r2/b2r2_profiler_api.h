/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * Author:Johan Mossberg <johan.xx.mossberg@stericsson.com> for ST-Ericsson
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

#ifndef _LINUX_VIDEO_B2R2_PROFILER_API_H
#define _LINUX_VIDEO_B2R2_PROFILER_API_H

#include "b2r2_blt.h"

/**
 * struct b2r2_blt_profiling_info - Profiling information for a blit
 *
 * @nsec_active_in_cpu: The number of nanoseconds the job was active in the CPU.
 *                      This is an approximate value, check out the code for more
 *                      info.
 * @nsec_active_in_b2r2: The number of nanoseconds the job was active in B2R2. This
 *                       is an approximate value, check out the code for more info.
 * @total_time_nsec: The total time the job took in nano seconds. Includes ideling.
 */
struct b2r2_blt_profiling_info {
	s32 nsec_active_in_cpu;
	s32 nsec_active_in_b2r2;
	s32 total_time_nsec;
};

/**
 * struct b2r2_profiler - B2R2 profiler.
 *
 * The callbacks are never run concurrently. No heavy stuff must be done in the
 * callbacks as this might adversely affect the B2R2 driver. The callbacks must
 * not call the B2R2 profiler API as this will cause a deadlock. If the callbacks
 * call into the B2R2 driver care must be taken as deadlock situations can arise.
 *
 * @blt_done: Called when a blit has finished, timed out or been canceled.
 */
struct b2r2_profiler {
	void (*blt_done)(const struct b2r2_blt_req * const request, const s32 request_id, const struct b2r2_blt_profiling_info * const blt_profiling_info);
};

/**
 * b2r2_register_profiler() - Registers a profiler.
 *
 * Currently only one profiler can be registered at any given time.
 *
 * @profiler: The profiler
 *
 * Returns 0 on success, negative error code on failure
 */
int b2r2_register_profiler(const struct b2r2_profiler * const profiler);

/**
 * b2r2_unregister_profiler() - Unregisters a profiler.
 *
 * @profiler: The profiler
 */
void b2r2_unregister_profiler(const struct b2r2_profiler * const profiler);

#endif /* #ifdef _LINUX_VIDEO_B2R2_PROFILER_API_H */
