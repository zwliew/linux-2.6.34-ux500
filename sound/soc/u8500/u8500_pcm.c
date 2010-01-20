/*
 *
 * sound/soc/u8500/u8500_pcm.c
 *
 *
 * Copyright (C) 2010 ST-Ericsson AB
 * License terms: GNU General Public License (GPL) version 2
 * ALSA SoC PCM driver for the u8500 platforms.
 */

#include <asm/page.h>

#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>

#include <linux/dma-mapping.h>

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "u8500_pcm.h"
#include "u8500_msp_dai.h"



// Local variables -------------------------------------------------------------------

// struct dma_chan
struct u8500_pcm_transfer {
	spinlock_t lock;
	dma_addr_t addr;
	u32 dma_chan_id;
	u32 period;
	struct dma_chan *dma_chan;
};

static struct snd_pcm_hardware u8500_pcm_hw_playback = {
	.info =
	    (SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_MMAP |
	     SNDRV_PCM_INFO_RESUME | SNDRV_PCM_INFO_PAUSE),
	.formats =
	    SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE |
	    SNDRV_PCM_FMTBIT_S16_BE | SNDRV_PCM_FMTBIT_U16_BE,
	.rates = SNDRV_PCM_RATE_KNOT,
	.rate_min = U8500_PLATFORM_MIN_RATE_PLAYBACK,
	.rate_max = U8500_PLATFORM_MAX_RATE_PLAYBACK,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = NMDK_BUFFER_SIZE,
	.period_bytes_min = 128,
	.period_bytes_max = PAGE_SIZE,
	.periods_min = NMDK_BUFFER_SIZE / PAGE_SIZE,
	.periods_max = NMDK_BUFFER_SIZE / 128
};

static struct snd_pcm_hardware u8500_pcm_hw_capture = {
	.info =
	    (SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_MMAP |
	     SNDRV_PCM_INFO_RESUME | SNDRV_PCM_INFO_PAUSE),
	.formats =
	    SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE |
	    SNDRV_PCM_FMTBIT_S16_BE | SNDRV_PCM_FMTBIT_U16_BE,
	.rates = SNDRV_PCM_RATE_KNOT,
	.rate_min = U8500_PLATFORM_MIN_RATE_CAPTURE,
	.rate_max = U8500_PLATFORM_MAX_RATE_CAPTURE,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = NMDK_BUFFER_SIZE,
	.period_bytes_min = 128,
	.period_bytes_max = PAGE_SIZE,
	.periods_min = NMDK_BUFFER_SIZE / PAGE_SIZE,
	.periods_max = NMDK_BUFFER_SIZE / 128
};

static struct u8500_pcm_private_t u8500_pcm_private;



// DMA operations -----------------------------------------------------------------------------

/**
 * nomadik_alsa_dma_start - used to transmit or recive a dma chunk
 * @stream - specifies the playback/record stream structure
 */
static void u8500_pcm_dma_start(struct snd_pcm_substream *substream)
{
	unsigned int offset, dma_size;
	struct snd_pcm_runtime *runtime = substream->runtime;
	int stream_id = substream->pstr->stream;
	u8500_pcm_stream_t* stream_p = &u8500_pcm_private.streams[stream_id];

	printk(KERN_DEBUG "u8500_pcm_dma_start: Enter.\n");

	dma_size = frames_to_bytes(runtime, runtime->period_size);
	offset = dma_size * stream_p->period;
	stream_p->old_offset = offset;
	printk(KERN_DEBUG "u8500_pcm_dma_start: address = %x  size=%d\n", (runtime->dma_addr + offset), dma_size);

	if (substream->pstr->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		u8500_i2s_send_data((void*)(runtime->dma_addr + offset), dma_size, 0);
	}
	else {
		u8500_i2s_receive_data((void *)(runtime->dma_addr + offset), dma_size, 0);
	}

	stream_p->period++;
	stream_p->period %= runtime->periods;
	stream_p->periods++;
}

static void u8500_pcm_dma_hw_free(struct device *dev, struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_dma_buffer *buf = runtime->dma_buffer_p;

	if (runtime->dma_area == NULL)
		return;

	if (buf != &substream->dma_buffer) {
		dma_free_coherent(buf->dev.dev, buf->bytes, buf->area, buf->addr);
		kfree(runtime->dma_buffer_p);
	}

	snd_pcm_set_runtime_buffer(substream, NULL);
}

/**
 * dma_eot_handler
 * @data - pointer to structure set in the dma callback handler
 * @event - specifies the DMA event: transfer complete/error
 *
 *  This is the PCM tasklet handler linked to a pipe, its role is to tell
 *  the PCM middler layer whene the buffer position goes across the prescribed
 *  period size.To inform of this the snd_pcm_period_elapsed is called.
 *
 * this callback will be called in case of DMA_EVENT_TC only
 */
irqreturn_t u8500_pcm_dma_eot_handler(void *data, int irq)
{
	struct snd_pcm_substream *substream = data;
	int stream_id = substream->pstr->stream;
	u8500_pcm_stream_t* stream_p = &u8500_pcm_private.streams[stream_id];

	/* snd_pcm_period_elapsed() is _not_ to be protected
	 */
	printk(KERN_DEBUG "u8500_pcm_dma_eot_handler: Enter.\n");

	if (substream)
		snd_pcm_period_elapsed(substream);
	if(stream_p->state == ALSA_STATE_PAUSED)
		return IRQ_HANDLED;
	if (stream_p->active == 1) {
		u8500_pcm_dma_start(substream);
	}
	return IRQ_HANDLED;
}
EXPORT_SYMBOL(u8500_pcm_dma_eot_handler);




// snd_pcm_ops -----------------------------------------------------------------------------

static int u8500_pcm_open(struct snd_pcm_substream *substream)
{
	int status;
	struct snd_pcm_runtime *runtime = substream->runtime;
	int stream_id = substream->pstr->stream;
	u8500_pcm_stream_t* stream_p = &u8500_pcm_private.streams[stream_id];

	printk(KERN_DEBUG "u8500_pcm_open: Enter.\n");

	status = 0;
	printk(KERN_DEBUG "u8500_pcm_open: Setting HW-config.\n");
	if (stream_id == SNDRV_PCM_STREAM_PLAYBACK)
	{
		runtime->hw = u8500_pcm_hw_playback;
	}
	else
	{
		runtime->hw = u8500_pcm_hw_capture;
	}

	init_MUTEX(&(stream_p->alsa_sem));
	init_completion(&(stream_p->alsa_com));
	stream_p->state = ALSA_STATE_RUNNING;
	printk(KERN_ALERT "u8500_pcm_open: Exit.\n");

	return 0;
}

static int u8500_pcm_close(struct snd_pcm_substream *substream)
{
	printk(KERN_DEBUG "u8500_pcm_close: Enter.\n");

	return 0;
}

static int u8500_pcm_hw_params(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *hw_params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_dma_buffer *buf = runtime->dma_buffer_p;
	int ret = 0;
	int size;

	printk(KERN_DEBUG "u8500_pcm_hw_params: Enter.\n");

	size = params_buffer_bytes(hw_params);

	if (buf) {
		if (buf->bytes >= size)
			goto out;
		u8500_pcm_dma_hw_free(NULL, substream);
	}

	if (substream->dma_buffer.area != NULL && substream->dma_buffer.bytes >= size) {
		buf = &substream->dma_buffer;
	} else {
		buf = kmalloc(sizeof(struct snd_dma_buffer), GFP_KERNEL);
		if (!buf)
			goto nomem;

		buf->dev.type = SNDRV_DMA_TYPE_DEV;
		buf->dev.dev = NULL;
		buf->area = dma_alloc_coherent(NULL, size, &buf->addr, GFP_KERNEL);
		buf->bytes = size;
		buf->private_data = NULL;

		if (!buf->area)
			goto free;
	}
	snd_pcm_set_runtime_buffer(substream, buf);
	ret = 1;
 out:
	runtime->dma_bytes = size;
	return ret;

 free:
	kfree(buf);
 nomem:
	return -ENOMEM;
}

static int u8500_pcm_hw_free(struct snd_pcm_substream *substream)
{
	printk(KERN_ALERT "u8500_pcm_hw_free: Enter.\n");

	u8500_pcm_dma_hw_free(NULL, substream);

	return 0;
}

static int u8500_pcm_prepare(struct snd_pcm_substream *substream)
{
	printk(KERN_DEBUG "u8500_pcm_prepare: Enter.\n");

	return 0;
}

static int u8500_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int stream_id = substream->pstr->stream;
	u8500_pcm_stream_t* stream_p = &u8500_pcm_private.streams[stream_id];

	printk(KERN_DEBUG "u8500_pcm_trigger: Enter.\n");

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		// Start DMA transfer
		printk(KERN_DEBUG "u8500_pcm_trigger: TRIGGER START\n");
		if (stream_p->active == 0) {
			stream_p->active = 1;
			u8500_pcm_dma_start(substream);
			break;
		}
		printk(KERN_DEBUG "u8500_pcm_trigger: Error: H/w is busy\n");
		return -EINVAL;

	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		// Pause DMA transfer
		printk(KERN_ALERT "u8500_pcm_trigger: SNDRV_PCM_TRIGGER_PAUSE\n");
	if (stream_p->state == ALSA_STATE_RUNNING) {
		stream_p->state = ALSA_STATE_PAUSED;
	}

		break;

	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		// Resume DMA transfer
		printk(KERN_DEBUG "u8500_pcm_trigger: SNDRV_PCM_TRIGGER_RESUME\n");
		if (stream_p->state == ALSA_STATE_PAUSED) {
			stream_p->state = ALSA_STATE_RUNNING;
			u8500_pcm_dma_start(substream);
		}
		break;

	case SNDRV_PCM_TRIGGER_STOP:
		// Stop DMA transfer
		printk(KERN_DEBUG "u8500_pcm_trigger: SNDRV_PCM_TRIGGER_STOP\n");
		if (stream_p->active == 1) {
			stream_p->active = 0;
			stream_p->period = 0;
		}
		break;

	default:
		printk(KERN_ERR "u8500_pcm_trigger: Invalid command in pcm trigger.\n");
		return -EINVAL;
	}

	return 0;
}

static snd_pcm_uframes_t u8500_pcm_pointer(struct snd_pcm_substream *substream)
{
	unsigned int offset;
	struct snd_pcm_runtime *runtime = substream->runtime;
	int stream_id = substream->pstr->stream;
	u8500_pcm_stream_t* stream_p = &u8500_pcm_private.streams[stream_id];

	printk(KERN_DEBUG "u8500_pcm_pointer: Enter.\n");

	offset = bytes_to_frames(runtime, stream_p->old_offset);
	if (offset < 0 || stream_p->old_offset < 0)
		printk(KERN_DEBUG "u8500_pcm_pointer: Offset=%i %i\n", offset, stream_p->old_offset);

	return offset;
}

static int u8500_pcm_mmap(struct snd_pcm_substream *substream,
			 struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	printk(KERN_DEBUG "u8500_pcm_mmap: Enter.\n");

	return dma_mmap_coherent(NULL, vma, runtime->dma_area, runtime->dma_addr, runtime->dma_bytes);
}

static struct snd_pcm_ops u8500_pcm_ops = {
	.open		= u8500_pcm_open,
	.close		= u8500_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= u8500_pcm_hw_params,
	.hw_free	= u8500_pcm_hw_free,
	.prepare	= u8500_pcm_prepare,
	.trigger	= u8500_pcm_trigger,
	.pointer	= u8500_pcm_pointer,
	.mmap		= u8500_pcm_mmap
};



// snd_soc_platform -----------------------------------------------------------------------------

int u8500_pcm_new(struct snd_card *card, struct snd_soc_dai *dai, struct snd_pcm *pcm)
{
	int i;

	printk(KERN_DEBUG "u8500_pcm_new: Enter.\n");

	printk(KERN_DEBUG "u8500_pcm_new: pcm = %d\n", (int)pcm);

	pcm->info_flags = 0;
	strcpy(pcm->name, "nomadik_asoc");

	for (i = 0; i < NO_OF_STREAMS; i++)
	{
		;
		u8500_pcm_private.streams[i].active = 0;
		u8500_pcm_private.streams[i].period = 0;
		u8500_pcm_private.streams[i].periods = 0;
		u8500_pcm_private.streams[i].old_offset = 0;
	}

	printk(KERN_DEBUG "u8500_pcm_new: pcm->name = %s\n", pcm->name);

	return 0;
}

static void u8500_pcm_free(struct snd_pcm *pcm)
{
	printk(KERN_DEBUG "u8500_pcm_free: Enter.\n");

	//u8500_pcm_dma_hw_free(NULL, substream);
}

static int u8500_pcm_suspend(struct snd_soc_dai *dai)
{
	printk(KERN_DEBUG "u8500_pcm_suspend: Enter.\n");

	return 0;
}

static int u8500_pcm_resume(struct snd_soc_dai *dai)
{
	printk(KERN_DEBUG "u8500_pcm_resume: Enter.\n");

	return 0;
}

struct snd_soc_platform u8500_soc_platform = {
	.name           = "u8500-audio",
	.pcm_ops        = &u8500_pcm_ops,
	.pcm_new        = u8500_pcm_new,
	.pcm_free       = u8500_pcm_free,
	.suspend        = u8500_pcm_suspend,
	.resume         = u8500_pcm_resume,
};
EXPORT_SYMBOL(u8500_soc_platform);












