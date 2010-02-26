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
#ifndef U8500_PCM_H
#define U8500_PCM_H

#include <mach/msp.h>

#define NMDK_BUFFER_SIZE 	(64*1024)

#define U8500_PLATFORM_MIN_RATE_PLAYBACK   48000
#define U8500_PLATFORM_MAX_RATE_PLAYBACK   48000
#define U8500_PLATFORM_MIN_RATE_CAPTURE    48000
#define U8500_PLATFORM_MAX_RATE_CAPTURE    48000
#define U8500_PLATFORM_MAX_NO_OF_RATES		1

#define NUMBER_OUTPUT_DEVICE				5
#define NUMBER_INPUT_DEVICE					10

#define NO_OF_STREAMS 2

enum alsa_state {
	ALSA_STATE_PAUSED,
	ALSA_STATE_RUNNING
};

extern struct snd_soc_platform u8500_soc_platform;

typedef struct {
	int stream_id;		/* stream identifier  */
	int active;		/* we are using this stream for transfer now */
	int period;		/* current transfer period */
	int periods;		/* current count of periods registerd in the DMA engine */
	enum alsa_state state;
	unsigned int old_offset;
	struct semaphore alsa_sem;
	struct completion alsa_com;
} u8500_pcm_stream_t;

struct u8500_pcm_private_t {
	u8500_pcm_stream_t streams[2];
};

irqreturn_t u8500_pcm_dma_eot_handler(void *data, int irq);

#endif
