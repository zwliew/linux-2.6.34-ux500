/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Ola Lilja ola.o.lilja@stericsson.com,
 *         Roger Nilsson roger.xr.nilsson@stericsson.com
 *         for ST-Ericsson.
 *
 * License terms:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#ifndef UX500_PCM_H
#define UX500_PCM_H

#include <mach/msp.h>

#define UX500_PLATFORM_BUFFER_SIZE 	(64*1024)

#define UX500_PLATFORM_MIN_RATE_PLAYBACK	8000
#define UX500_PLATFORM_MAX_RATE_PLAYBACK	48000
#define UX500_PLATFORM_MIN_RATE_CAPTURE		8000
#define UX500_PLATFORM_MAX_RATE_CAPTURE		48000
#define UX500_PLATFORM_MIN_CHANNELS   		1
#define UX500_PLATFORM_MAX_CHANNELS   		2
#define UX500_PLATFORM_MIN_PERIOD_BYTES		128

enum alsa_state {
	ALSA_STATE_PAUSED,
	ALSA_STATE_RUNNING
};

extern struct snd_soc_platform ux500_soc_platform;

struct ux500_pcm_private_s {
	int msp_id;
	int stream_id;
	int active;
	int period;
	int periods;
	enum alsa_state state;
	unsigned int old_offset;
	struct semaphore alsa_sem;
	struct completion alsa_com;
};

irqreturn_t ux500_pcm_dma_eot_handler(void *data, int irq);

#endif
