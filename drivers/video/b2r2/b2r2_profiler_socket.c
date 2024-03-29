/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * Author: Johan Mossberg <johan.xx.mossberg@stericsson.com> for ST-Ericsson
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


#include <linux/module.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <asm/errno.h>

#include "b2r2_profiler_api.h"
#include "b2r2_internal.h"


/* TODO: Call the profiler in a seperate thread and have a circular buffer between
the B2R2 driver and that thread. That way the profiler can not slow down or kill the
B2R2 driver. Seems a bit overkill right now as there is only one B2R2 profiler and
we have full control over it but the situation may be different in the future. */


static const struct b2r2_profiler *b2r2_profiler;
static DECLARE_MUTEX(b2r2_profiler_lock);


int b2r2_register_profiler(const struct b2r2_profiler * const profiler)
{
	int return_value;

	return_value = down_interruptible(&b2r2_profiler_lock);
	if (return_value != 0)
		return return_value;

	if (b2r2_profiler != NULL) {
		return_value = -EUSERS;

		goto cleanup;
	}

	b2r2_profiler = profiler;

	return_value = 0;

cleanup:
	up(&b2r2_profiler_lock);

	return return_value;
}
EXPORT_SYMBOL(b2r2_register_profiler);

void b2r2_unregister_profiler(const struct b2r2_profiler * const profiler)
{
	down(&b2r2_profiler_lock);

	if (profiler == b2r2_profiler)
		b2r2_profiler = NULL;

	up(&b2r2_profiler_lock);
}
EXPORT_SYMBOL(b2r2_unregister_profiler);


bool is_profiler_registered_approx(void)
{
	/* No locking by design, to make it fast, hence the approx */
	if (b2r2_profiler != NULL)
		return true;
	else
		return false;
}

void b2r2_call_profiler_blt_done(const struct b2r2_blt_request * const request)
{
	int return_value;
	struct b2r2_blt_profiling_info blt_profiling_info;

	return_value = down_interruptible(&b2r2_profiler_lock);
	if (return_value != 0) {
		dev_err(b2r2_blt_device(),
			"%s: Failed to acquire semaphore, ret=%i. Lost profiler call!\n",
			__func__, return_value);

		return;
	}

	if (NULL == b2r2_profiler)
		goto cleanup;

	blt_profiling_info.nsec_active_in_cpu = request->nsec_active_in_cpu;
	blt_profiling_info.nsec_active_in_b2r2 = request->job.nsec_active_in_hw;
	blt_profiling_info.total_time_nsec = request->total_time_nsec;

	b2r2_profiler->blt_done(&request->user_req, request->request_id, &blt_profiling_info);

cleanup:
	up(&b2r2_profiler_lock);
}
