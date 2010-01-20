#ifndef U8500_msp_dai_H
#define U8500_msp_dai_H

/*
 *
 * sound/soc/u8500/u8500_msp_dai.h
 *
 *
 * Copyright (C) 2007 Ericsson AB
 * License terms: GNU General Public License (GPL) version 2
 * ALSA SoC I2S driver for the U8500 platforms.
 */

#include <linux/types.h>
#include <linux/spinlock.h>

#define U8500_NBR_OF_DAI 2

extern struct snd_soc_dai u8500_msp_dai[U8500_NBR_OF_DAI];

int u8500_platform_registerI2S(void);
int u8500_i2s_send_data(void *data, size_t bytes, int dai_idx);
int u8500_i2s_receive_data(void *data, size_t bytes, int dai_idx);

#endif
