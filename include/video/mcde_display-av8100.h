/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * ST-Ericsson MCDE HDMI display driver
 *
 * Author: Per Persson <per-xb-persson@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */
#ifndef __DISPLAY_AV8100__H__
#define __DISPLAY_AV8100__H__

#include <linux/regulator/consumer.h>

#include "mcde_display.h"

#define GPIO_AV8100_RSTN			196

struct mcde_display_hdmi_platform_data {
	/* Platform info */
	int reset_gpio;
	bool reset_high;
	const char *regulator_id;
	int reset_delay; /* ms */
	u32 ddb_id;

	/* Driver data */
	bool hdmi_platform_enable;
	struct regulator *regulator;
};

#endif /* __DISPLAY_AV8100__H__ */
