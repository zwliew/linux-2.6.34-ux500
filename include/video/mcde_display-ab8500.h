/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * AB8500 tvout driver interface
 *
 * Author: Marcel Tunnissen <marcel.tuennissen@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */
#ifndef __DISPLAY_AB8500__H__
#define __DISPLAY_AB8500__H__

struct ab8500_display_platform_data {
	/* Platform info */
	const char *regulator_id;
	/* Driver data */
	struct regulator *regulator;
};

#endif /* __DISPLAY_AB8500__H__*/

