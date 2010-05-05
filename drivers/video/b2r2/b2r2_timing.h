/*
 * Copyright (C) ST-Ericsson AB 2009
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

#ifndef _LINUX_DRIVERS_VIDEO_B2R2_TIMING_H_
#define _LINUX_DRIVERS_VIDEO_B2R2_TIMING_H_

/**
 * b2r2_get_curr_nsec() - Return the current nanosecond. Notice that the value
 *                        wraps when the u32 limit is reached.
 *
 */
u32 b2r2_get_curr_nsec(void);

#endif /* _LINUX_DRIVERS_VIDEO_B2R2_TIMING_H_ */
