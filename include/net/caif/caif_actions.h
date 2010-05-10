/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author:	Sjur Brendeland / sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CAIF_ACTION_H_
#define CAIF_ACTION_H_
#include <linux/caif/caif_config.h>
#include <linux/caif/caif_ioctl.h>

/*
 * Create and Configure a new CAIF device. Note that the device is not
 * implicitly connected. Type is struct caif_channel_create_action
 */
#define CAIF_ACT_CREATE_DEVICE 		 7

/*
 * Remove a CAIF device. Requires the device to be previously disconnected.
 * Type is struct caif_device_name)
 */
#define CAIF_ACT_DELETE_DEVICE           8
#endif
