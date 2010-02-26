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
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <asm/dma.h>
#include "u8500_msp_dai.h"
#include "u8500_pcm.h"

#include <mach/msp.h>
#include <linux/i2s/i2s.h>

struct u8500_dai_private_t{
	spinlock_t lock;
	struct i2s_device *i2s;
	u8 rx_active;
	u8 tx_active;
};

struct u8500_dai_private_t msp_dai_private[U8500_NBR_OF_DAI] =
{
	/* DAI 0 */
	{
		.lock = SPIN_LOCK_UNLOCKED,
		.i2s = NULL,
		.tx_active = 0,
		.rx_active = 0,
	},
	/* DAI 1  */
	{
		.lock = SPIN_LOCK_UNLOCKED,
		.i2s = NULL,
		.tx_active = 0,
		.rx_active = 0,
	},
};

static void compile_msp_config(struct msp_generic_config *msp_config,struct snd_pcm_substream *substream)
{
  int channels;

  msp_config->tx_clock_sel = 0;
	msp_config->rx_clock_sel = 0;
	msp_config->tx_frame_sync_sel = 0;
	msp_config->rx_frame_sync_sel = 0;
	msp_config->input_clock_freq = MSP_INPUT_FREQ_48MHZ;
	msp_config->srg_clock_sel = 0;
  msp_config->rx_frame_sync_pol = RX_FIFO_SYNC_HI;
	msp_config->tx_frame_sync_pol = TX_FIFO_SYNC_HI;
  msp_config->rx_fifo_config = RX_FIFO_ENABLE;
	msp_config->tx_fifo_config = TX_FIFO_ENABLE;
  msp_config->spi_clk_mode = SPI_CLK_MODE_NORMAL;
	msp_config->spi_burst_mode = 0;
  msp_config->handler = u8500_pcm_dma_eot_handler;
	msp_config->tx_callback_data =  substream;
	msp_config->tx_data_enable = 0;
	msp_config->rx_callback_data =  substream;
	msp_config->loopback_enable = 0;
	msp_config->multichannel_configured = 0;
	msp_config->def_elem_len = 0;
  msp_config->default_protocol_desc = 0;
  msp_config->protocol_desc.rx_phase_mode = MSP_SINGLE_PHASE;
  msp_config->protocol_desc.tx_phase_mode = MSP_SINGLE_PHASE;
  msp_config->protocol_desc.rx_phase2_start_mode = MSP_PHASE2_START_MODE_IMEDIATE;
  msp_config->protocol_desc.tx_phase2_start_mode = MSP_PHASE2_START_MODE_IMEDIATE;
  msp_config->protocol_desc.rx_bit_transfer_format = MSP_BTF_MS_BIT_FIRST;
  msp_config->protocol_desc.tx_bit_transfer_format = MSP_BTF_MS_BIT_FIRST;
  msp_config->protocol_desc. rx_frame_length_1 = MSP_FRAME_LENGTH_2;
  msp_config->protocol_desc. rx_frame_length_2 = MSP_FRAME_LENGTH_2;
  msp_config->protocol_desc.tx_frame_length_1 = MSP_FRAME_LENGTH_2;
  msp_config->protocol_desc.tx_frame_length_2 = MSP_FRAME_LENGTH_2;
  msp_config->protocol_desc. rx_element_length_1 = MSP_ELEM_LENGTH_32;
  msp_config->protocol_desc.rx_element_length_2 = MSP_ELEM_LENGTH_32;
  msp_config->protocol_desc.tx_element_length_1 = MSP_ELEM_LENGTH_32;
  msp_config->protocol_desc.tx_element_length_2 = MSP_ELEM_LENGTH_32;
  msp_config->protocol_desc.rx_data_delay = MSP_DELAY_0;
  msp_config->protocol_desc.tx_data_delay = MSP_DELAY_0;
  msp_config->protocol_desc.rx_clock_pol = MSP_FALLING_EDGE;
  msp_config->protocol_desc.tx_clock_pol = MSP_RISING_EDGE;
  msp_config->protocol_desc. rx_frame_sync_pol = MSP_FRAME_SYNC_POL_ACTIVE_HIGH;
  msp_config->protocol_desc.tx_frame_sync_pol = MSP_FRAME_SYNC_POL_ACTIVE_HIGH;
  msp_config->protocol_desc.rx_half_word_swap = MSP_HWS_NO_SWAP;
  msp_config->protocol_desc.tx_half_word_swap = MSP_HWS_NO_SWAP;
  msp_config->protocol_desc.compression_mode = MSP_COMPRESS_MODE_LINEAR;
  msp_config->protocol_desc.expansion_mode = MSP_EXPAND_MODE_LINEAR;
  msp_config->protocol_desc.spi_clk_mode = MSP_SPI_CLOCK_MODE_NON_SPI;
  msp_config->protocol_desc.spi_burst_mode = MSP_SPI_BURST_MODE_DISABLE;
  msp_config->protocol_desc.frame_sync_ignore = MSP_FRAME_SYNC_IGNORE;
  msp_config->protocol_desc.frame_period = 63;
  msp_config->protocol_desc.frame_width = 31;
	msp_config->protocol_desc.total_clocks_for_one_frame = 64;
  msp_config->protocol = MSP_I2S_PROTOCOL; //MSP_PCM_PROTOCOL
	msp_config->direction = MSP_BOTH_T_R_MODE;
	msp_config->frame_size = 0;
	msp_config->frame_freq = 48000;
	channels = 2;
	msp_config->data_size = (channels == 1) ? MSP_DATA_SIZE_16BIT : MSP_DATA_SIZE_32BIT;
	msp_config->work_mode = MSP_DMA_MODE;
  msp_config->multichannel_configured = 1;
	msp_config->multichannel_config.tx_multichannel_enable = 1;
	msp_config->multichannel_config.tx_channel_0_enable = 0x0000003; //Channel 1 and channel 2
	msp_config->multichannel_config.tx_channel_1_enable = 0x0000000; //Channel 1 and channel 2
	msp_config->multichannel_config.tx_channel_2_enable = 0x0000000; //Channel 1 and channel 2
	msp_config->multichannel_config.tx_channel_3_enable = 0x0000000; //Channel 1 and channel 2
	msp_config->multichannel_config.rx_multichannel_enable = 1;
	msp_config->multichannel_config.rx_channel_0_enable = 0x0000003; //Channel 1 and channel 2
	msp_config->multichannel_config.rx_channel_1_enable = 0x0000000; //Channel 1 and channel 2
	msp_config->multichannel_config.rx_channel_2_enable = 0x0000000; //Channel 1 and channel 2
	msp_config->multichannel_config.rx_channel_3_enable = 0x0000000; //Channel 1 and channel 2
}

static void u8500_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *msp_dai)
{
	int ret = 0;
	unsigned long flags;
	struct u8500_dai_private_t *dai_private;

  printk(KERN_ALERT "STW4500: u8500_dai_shutdown\n");

	if (!(dai_private = msp_dai->private_data))
		return;

	spin_lock_irqsave(&dai_private->lock, flags);

	/* Mark the stopped direction as inactive */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dai_private->tx_active = 0;
	} else {
		dai_private->rx_active = 0;
	}

	if (0 == dai_private->tx_active && 0 == dai_private->rx_active) {
		msp_dai->private_data = NULL;


    ret = i2s_cleanup(dai_private->i2s->controller, DISABLE_ALL);

    if (ret)
			printk(KERN_WARNING "Error closing i2s\n");

	} else {

    if(dai_private->tx_active)
      ret = i2s_cleanup(dai_private->i2s->controller, DISABLE_RECEIVE);
    else
      ret = i2s_cleanup(dai_private->i2s->controller, DISABLE_TRANSMIT);

    if (ret)
			printk(KERN_WARNING "Error closing i2s\n");
  }

  spin_unlock_irqrestore(&dai_private->lock, flags);

  return;
}

static int u8500_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *msp_dai)
{
	struct u8500_dai_private_t *dai_private;

  printk(KERN_ALERT "STW4500: u8500_dai_startup\n");

	dai_private=&msp_dai_private[msp_dai->id];
	/* Store pointer to private data */
	msp_dai->private_data = dai_private;

  if (dai_private->i2s == NULL)
	{
		printk(KERN_ALERT "u8500_pcm_open: Error: i2sdrv.i2s == NULL.\n");
		return -1;
	}
	if (dai_private->i2s->controller == NULL)
	{
		printk(KERN_ALERT "u8500_pcm_open: Error: i2sdrv.i2s->controller == NULL.\n");
		return -1;
	}


	return 0;
}

static int u8500_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *msp_dai)
{
	int ret = 0;
	unsigned long flags_private;
	struct u8500_dai_private_t *dai_private;
	struct msp_generic_config msp_config;

  printk(KERN_ALERT "STW4500: u8500_dai_prepare\n");

	dai_private = msp_dai->private_data;

	spin_lock_irqsave(&dai_private->lock, flags_private);

  /* Is the other direction already active? */
	if ((substream->stream == SNDRV_PCM_STREAM_PLAYBACK &&
	     dai_private->rx_active) ||
	    (substream->stream == SNDRV_PCM_STREAM_CAPTURE &&
	     dai_private->tx_active)) {
		/* TODO: Check if sample rate match ! */
    /*
    if (  != runtime->rate) {
			printk(KERN_ERR "CPU_DAI configuration mismatch with already opened stream!\n");
			ret = -EBUSY;
			goto cleanup;
		}
    */
  } else if (!dai_private->rx_active && !dai_private->tx_active) {
		ret = 0;

    compile_msp_config(&msp_config,substream);

    ret = i2s_setup(dai_private->i2s->controller, &msp_config);
	  if (ret < 0) {
		  printk(KERN_ALERT "u8500_dai_prepare: i2s_setup failed! ret = %d\n", ret);
		  goto cleanup;
	  }
	}

	/* Mark the newly started stream as active */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dai_private->tx_active = 1;
	} else {
		dai_private->rx_active = 1;
	}

cleanup:
	spin_unlock_irqrestore(&dai_private->lock, flags_private);
	return ret;
}

static int u8500_dai_hw_params(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *params, struct snd_soc_dai *msp_dai)
{
  printk(KERN_ALERT "STW4500: u8500_dai_hw_params\n");

	return 0;
}

static int u8500_dai_set_dai_fmt(struct snd_soc_dai *msp_dai, unsigned int fmt)
{
  printk(KERN_ALERT "STW4500: u8500_dai_set_dai_fmt\n");
	return 0;
}

static int u8500_dai_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *msp_dai)
{
	unsigned long flags;
	int ret = 0;
	struct u8500_dai_private_t *u8500_dai_private = msp_dai->private_data;

  printk(KERN_ALERT "STW4500: u8500_dai_trigger\n");

	spin_lock_irqsave(&u8500_dai_private->lock, flags);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		ret = 0;
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		ret = 0;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		ret = 0;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		ret = 0;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock_irqrestore(&u8500_dai_private->lock, flags);
	return ret;
}

int u8500_i2s_send_data(void *data, size_t bytes, int dai_idx)
{
  unsigned long flags;
  struct u8500_dai_private_t *dai_private;
  struct i2s_message message;
  struct i2s_device *i2s_dev;
  int ret = 0;
  dai_private=&msp_dai_private[dai_idx];

  spin_lock_irqsave(&dai_private->lock, flags);

  i2s_dev=dai_private->i2s;


  if(!dai_private->tx_active)
  {
    printk(KERN_ERR "u8500_i2s_send_data: I2S controller not available\n");
    goto cleanup;
  }

	message.txbytes = bytes;
	message.txdata = data;
	message.rxbytes = 0;
	message.rxdata = NULL;
	message.dma_flag = 1;

  ret = i2s_transfer(i2s_dev->controller, &message);
	if(ret < 0)
	{
		printk(KERN_ERR "u8500_i2s_send_data: Error: i2s_transfer failed!\n");
		goto cleanup;
	}

cleanup:
	spin_unlock_irqrestore(&dai_private->lock, flags);
	return ret;
}

int u8500_i2s_receive_data(void *data, size_t bytes, int dai_idx)
{
	unsigned long flags;
  struct u8500_dai_private_t *dai_private;
  struct i2s_message message;
  struct i2s_device *i2s_dev;
  int ret = 0;
  dai_private=&msp_dai_private[dai_idx];

  spin_lock_irqsave(&dai_private->lock, flags);

  i2s_dev=dai_private->i2s;


  if(!dai_private->rx_active)
  {
    printk(KERN_ERR "u8500_i2s_receive_data: I2S controller not available\n");
    goto cleanup;
  }

  message.rxbytes = bytes;
	message.rxdata = data;
	message.txbytes = 0;
	message.txdata = NULL;
	message.dma_flag = 1;

  ret = i2s_transfer(i2s_dev->controller, &message);
	if(ret < 0)
	{
		printk(KERN_ERR "u8500_i2s_receive_data: Error: i2s_transfer failed!\n");
		goto cleanup;
	}

cleanup:
	spin_unlock_irqrestore(&dai_private->lock, flags);
	return ret;
}

#define u8500_I2S_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |	\
			SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000)
#define u8500_I2S_FORMATS ( SNDRV_PCM_FMTBIT_S16_LE )

#define dai_suspend NULL
#define dai_resume NULL
#define u8500_dai_set_dai_sysclk NULL

struct snd_soc_dai u8500_msp_dai[U8500_NBR_OF_DAI] =
{
	{
		.name = "u8500_i2s-0",
		.id = 0,
		//.type = SND_SOC_DAI_I2S,
		.suspend = dai_suspend,
		.resume = dai_resume,
		.playback = {
			.channels_min = 2,
			.channels_max = 2,
			.rates = u8500_I2S_RATES,
			.formats = u8500_I2S_FORMATS,},
		.capture = {
			.channels_min = 2,
			.channels_max = 2,
			.rates = u8500_I2S_RATES,
			.formats = u8500_I2S_FORMATS,},
		.ops = {
			.set_sysclk=u8500_dai_set_dai_sysclk,
      .set_fmt=u8500_dai_set_dai_fmt,
      .startup = u8500_dai_startup,
			.shutdown = u8500_dai_shutdown,
			.prepare = u8500_dai_prepare,
			.trigger = u8500_dai_trigger,
			.hw_params = u8500_dai_hw_params,
     },
		.private_data = &msp_dai_private[0],
	},
	{
		.name = "u8500_i2s-1",
		.id = 1,
		//.type = SND_SOC_DAI_I2S,
		.suspend = dai_suspend,
		.resume = dai_resume,
		.playback = {
			.channels_min = 2,
			.channels_max = 2,
			.rates = u8500_I2S_RATES,
			.formats = u8500_I2S_FORMATS,},
		.capture = {
			.channels_min = 2,
			.channels_max = 2,
			.rates = u8500_I2S_RATES,
			.formats = u8500_I2S_FORMATS,},
		.ops = {
			.set_sysclk=u8500_dai_set_dai_sysclk,
      .set_fmt=u8500_dai_set_dai_fmt,
      .startup = u8500_dai_startup,
			.shutdown = u8500_dai_shutdown,
			.prepare = u8500_dai_prepare,
			.trigger = u8500_dai_trigger,
			.hw_params = u8500_dai_hw_params,
    },
		.private_data = &msp_dai_private[1],
	},
};

EXPORT_SYMBOL(u8500_msp_dai);



static int i2sdrv_probe(struct i2s_device *i2s)
{
	unsigned long flags;
  struct u8500_dai_private_t *u8500_dai_private;

  u8500_dai_private=&msp_dai_private[0];
  printk(KERN_DEBUG "i2sdrv_probe: Enter.\n");

  spin_lock_irqsave(&u8500_dai_private->lock, flags);
  u8500_dai_private->i2s = i2s;
  spin_unlock_irqrestore(&u8500_dai_private->lock, flags);

	try_module_get(i2s->controller->dev.parent->driver->owner);
	i2s_set_drvdata(i2s, (void*)u8500_dai_private);

	return 0;
}



static int i2sdrv_remove(struct i2s_device *i2s)
{
  unsigned long flags;
  struct u8500_dai_private_t *u8500_dai_private = i2s_get_drvdata(i2s);

  printk(KERN_DEBUG "U8500_PCM: i2sdrv_remove\n");

  spin_lock_irqsave(&u8500_dai_private->lock, flags);

  u8500_dai_private->i2s = NULL;
  i2s_set_drvdata(i2s, NULL);
  spin_unlock_irqrestore(&u8500_dai_private->lock, flags);


  printk(KERN_ALERT "i2sdrv_remove: module_put\n");
  module_put(i2s->controller->dev.parent->driver->owner);

  return 0;
}



static const struct i2s_device_id dev_id_table[] = {
	        { "i2s_device.0", 0, 0 },
		        { },
};
MODULE_DEVICE_TABLE(i2s, acodec_id_table);



static struct i2s_driver i2sdrv_i2s = {
  .driver = {
    .name = "nomadik_acodec",
    .owner = THIS_MODULE,
  },
  .probe = i2sdrv_probe,
  .remove = __devexit_p(i2sdrv_remove),
  .id_table = dev_id_table,
};

int u8500_platform_registerI2S(void)
{
  int ret;

  printk(KERN_ALERT "u8500_platform_registerI2S: Register I2S-driver.\n");
  ret = i2s_register_driver(&i2sdrv_i2s);
  if (ret < 0) {
    printk(KERN_ALERT "u8500_platform_registerI2S: Unable to register I2S-driver\n");
  }

  return ret;
}
