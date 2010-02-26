/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Ola Lilja ola.o.lilja@stericsson.com,
 *         Roger Nilsson 	roger.xr.nilsson@stericsson.com
 *         for ST-Ericsson.
 *
 * License terms:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#ifndef U8500_msp_dai_H
#define U8500_msp_dai_H

#include <linux/types.h>
#include <linux/spinlock.h>

#define U8500_NBR_OF_DAI 2

extern struct snd_soc_dai u8500_msp_dai[U8500_NBR_OF_DAI];

int u8500_platform_registerI2S(void);
int u8500_i2s_send_data(void *data, size_t bytes, int dai_idx);
int u8500_i2s_receive_data(void *data, size_t bytes, int dai_idx);

#endif
