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
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <asm/dma.h>
#include "ux500_msp_dai.h"
#include "ux500_pcm.h"

#include <mach/msp.h>
#include <linux/i2s/i2s.h>

static struct ux500_msp_dai_private_s
	ux500_msp_dai_private[UX500_NBR_OF_DAI] =
{
	{
		.lock = __SPIN_LOCK_UNLOCKED(ux500_msp_dai_private[0].lock),
		.i2s = NULL,
		.tx_active = 0,
		.rx_active = 0,
		.fmt = 0,
		.rate = 0,
	},
	{
		.lock = __SPIN_LOCK_UNLOCKED(ux500_msp_dai_private[1].lock),
		.i2s = NULL,
		.tx_active = 0,
		.rx_active = 0,
		.fmt = 0,
		.rate = 0,
	},
	{
		.lock = __SPIN_LOCK_UNLOCKED(ux500_msp_dai_private[2].lock),
		.i2s = NULL,
		.tx_active = 0,
		.rx_active = 0,
		.fmt = 0,
		.rate = 0,
	},
};

static int ux500_msp_dai_i2s_probe(struct i2s_device *i2s)
{
	unsigned long flags;

	pr_debug("%s: Enter (chip_select = %d, i2s = %d).\n",
		__func__,
		(int)i2s->chip_select, (int)(i2s));

	spin_lock_irqsave(
		&ux500_msp_dai_private[i2s->chip_select].lock,
		flags);
	ux500_msp_dai_private[i2s->chip_select].i2s = i2s;
	spin_unlock_irqrestore(
		&ux500_msp_dai_private[i2s->chip_select].lock,
		flags);
	try_module_get(i2s->controller->dev.parent->driver->owner);
	i2s_set_drvdata(
		i2s,
		(void *)&ux500_msp_dai_private[i2s->chip_select]);

	return 0;
}

static int ux500_msp_dai_i2s_remove(struct i2s_device *i2s)
{
	unsigned long flags;
	struct ux500_msp_dai_private_s *ux500_msp_dai_private =
		i2s_get_drvdata(i2s);

	pr_debug("%s: Enter (chip_select = %d).\n",
		__func__,
		(int)i2s->chip_select);

	spin_lock_irqsave(&ux500_msp_dai_private->lock, flags);

	ux500_msp_dai_private->i2s = NULL;
	i2s_set_drvdata(i2s, NULL);
	spin_unlock_irqrestore(
		&ux500_msp_dai_private->lock,
		flags);

	pr_debug("%s: module_put\n",
		__func__);
	module_put(i2s->controller->dev.parent->driver->owner);

	return 0;
}

static const struct i2s_device_id dev_id_table[] = {
	{ "i2s_device.0", 0, 0 },
	{ "i2s_device.1", 0, 0 },
	{ "i2s_device.2", 0, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2s, dev_id_table);

static struct i2s_driver i2sdrv_i2s = {
	.driver = {
		.name = "ux500_asoc_i2s",
		.owner = THIS_MODULE,
	},
	.probe = ux500_msp_dai_i2s_probe,
	.remove = __devexit_p(ux500_msp_dai_i2s_remove),
	.id_table = dev_id_table,
};

int ux500_msp_dai_i2s_send_data(
	void *data,
	size_t bytes,
	int dai_idx)
{
	unsigned long flags;
	struct ux500_msp_dai_private_s *dai_private =
		&ux500_msp_dai_private[dai_idx];
	struct i2s_message message;
	struct i2s_device *i2s_dev;
	int ret = 0;

	pr_debug("%s: Enter MSP Index:%d bytes = %d).\n",
		__func__,
		dai_idx,
		(int)bytes);
	spin_lock_irqsave(&dai_private->lock, flags);

	i2s_dev = dai_private->i2s;

	if (!dai_private->tx_active) {
		pr_err("%s: The I2S controller is not available."
			"MSP index:%d\n",
			__func__,
			dai_idx);
		goto cleanup;
	}

	message.txbytes = bytes;
	message.txdata = data;
	message.rxbytes = 0;
	message.rxdata = NULL;
	message.dma_flag = 1;
	ret = i2s_transfer(i2s_dev->controller, &message);
	if (ret < 0) {
		pr_err("%s: Error: i2s_transfer failed. MSP index: %d\n",
			__func__,
			dai_idx);
		goto cleanup;
	}

cleanup:
	spin_unlock_irqrestore(&dai_private->lock, flags);
	return ret;
}

int ux500_msp_dai_i2s_receive_data(
	void *data,
	size_t bytes,
	int dai_idx)
{
	unsigned long flags;
	struct ux500_msp_dai_private_s *dai_private =
		&ux500_msp_dai_private[dai_idx];
	struct i2s_message message;
	struct i2s_device *i2s_dev;
	int ret = 0;

	pr_debug("%s: Enter MSP Index: %d, bytes = %d).\n",
		__func__,
		dai_idx,
		(int)bytes);

	spin_lock_irqsave(&dai_private->lock, flags);

	i2s_dev = dai_private->i2s;

	if (!dai_private->rx_active) {
		pr_err("%s: The I2S controller is not available."
			"MSP index: %d\n",
			__func__,
			dai_idx);
		goto cleanup;
	}

	message.rxbytes = bytes;
	message.rxdata = data;
	message.txbytes = 0;
	message.txdata = NULL;
	message.dma_flag = 1;
	ret = i2s_transfer(i2s_dev->controller, &message);
	if (ret < 0) {
		pr_err("%s: Error: i2s_transfer failed. Msp index: %d\n",
			__func__,
			dai_idx);
		goto cleanup;
	}

cleanup:
	spin_unlock_irqrestore(&dai_private->lock, flags);
	return ret;
}

static void ux500_msp_dai_shutdown(
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *msp_dai)
{
	int ret = 0;
	unsigned long flags;
	struct ux500_msp_dai_private_s *dai_private =
		msp_dai->private_data;

	pr_debug("%s: Enter (stream = %s).\n",
		__func__,
		substream->stream == SNDRV_PCM_STREAM_PLAYBACK ?
			"SNDRV_PCM_STREAM_PLAYBACK" :
			"SNDRV_PCM_STREAM_CAPTURE");

	if (dai_private == NULL)
		return;

	pr_info("%s: MSP Index: %d\n",
		__func__,
		(int)dai_private->i2s->chip_select);

	spin_lock_irqsave(&dai_private->lock, flags);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dai_private->tx_active = 0;
	else
		dai_private->rx_active = 0;

	if (0 == dai_private->tx_active &&
		0 == dai_private->rx_active) {

		msp_dai->private_data = NULL;
		ret = i2s_cleanup(dai_private->i2s->controller, DISABLE_ALL);

		if (ret) {
			pr_err("%s: Failed to close the i2s controller."
				"MSP index:%d\n",
				__func__,
				(int)dai_private->i2s->chip_select);
		}
	} else {

		if (dai_private->tx_active)
			ret = i2s_cleanup(
				dai_private->i2s->controller,
				DISABLE_RECEIVE);
		else
			ret = i2s_cleanup(
				dai_private->i2s->controller,
				DISABLE_TRANSMIT);

		if (ret) {
			pr_err("%s: Failed to close the i2s controller."
				"MSP index:%d\n",
				__func__,
				(int)dai_private->i2s->chip_select);
		}
	}

	spin_unlock_irqrestore(&dai_private->lock, flags);
}

static int ux500_msp_dai_startup(
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *msp_dai)
{
	struct ux500_msp_dai_private_s *dai_private =
		&ux500_msp_dai_private[msp_dai->id];

	pr_info("%s: MSP Index: %d).\n",
		__func__,
		msp_dai->id);

	msp_dai->private_data = dai_private;

	if (dai_private->i2s == NULL) {
		pr_err("%s: Error: MSP index: %d"
			"i2sdrv.i2s == NULL\n",
			__func__,
			msp_dai->id);
		return -1;
	}

	if (dai_private->i2s->controller == NULL) {
		pr_err("%s: Error: MSP index: %d"
			"i2sdrv.i2s->controller == NULL.\n",
			__func__,
			msp_dai->id);
		return -1;
	}

	return 0;
}

static void ux500_msp_dai_compile_msp_config(
	struct snd_pcm_substream *substream,
    unsigned int fmt,
    unsigned int rate,
    struct msp_generic_config *msp_config)
{
	struct msp_protocol_desc *prot_desc;
	struct snd_pcm_runtime *runtime = substream->runtime;

	msp_config->input_clock_freq = UX500_MSP_INTERNAL_CLOCK_FREQ;
	msp_config->rx_frame_sync_pol = RX_FIFO_SYNC_HI;
	msp_config->tx_frame_sync_pol = TX_FIFO_SYNC_HI;
	msp_config->rx_fifo_config = RX_FIFO_ENABLE;
	msp_config->tx_fifo_config = TX_FIFO_ENABLE;
	msp_config->spi_clk_mode = SPI_CLK_MODE_NORMAL;
	msp_config->spi_burst_mode = 0;
	msp_config->handler = ux500_pcm_dma_eot_handler;
	msp_config->tx_callback_data =  substream;
	msp_config->tx_data_enable = 0;
	msp_config->rx_callback_data =  substream;
	msp_config->loopback_enable = 0;
	msp_config->def_elem_len = 1;
	msp_config->direction = MSP_BOTH_T_R_MODE;
	msp_config->frame_size = 0;
	msp_config->data_size = MSP_DATA_SIZE_32BIT;
	msp_config->work_mode = MSP_DMA_MODE;
	msp_config->multichannel_configured = 0;
	msp_config->multichannel_config.tx_multichannel_enable = 0;
	msp_config->multichannel_config.rx_multichannel_enable = 0;

	switch (fmt &
		(SND_SOC_DAIFMT_FORMAT_MASK | SND_SOC_DAIFMT_MASTER_MASK)) {

	case SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS:
		pr_info("%s: SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS\n",
			__func__);
		msp_config->default_protocol_desc = 1;
		msp_config->protocol = MSP_I2S_PROTOCOL;
		msp_config->tx_clock_sel = TX_CLK_SEL_SRG;
		msp_config->tx_frame_sync_sel = TX_SYNC_SRG_PROG;
		msp_config->rx_clock_sel = RX_CLK_SEL_SRG;
		msp_config->rx_frame_sync_sel = RX_SYNC_SRG;
		msp_config->srg_clock_sel = 1 << SCKSEL_BIT;
		msp_config->frame_freq = rate;
		break;

	default:
	case SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBM_CFM:
		pr_info("%s: SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBM_CFM\n",
			__func__);
		msp_config->data_size = MSP_DATA_SIZE_16BIT;
		msp_config->default_protocol_desc = 0;
		msp_config->protocol = MSP_I2S_PROTOCOL;

		prot_desc = &msp_config->protocol_desc;

		prot_desc->rx_phase_mode = MSP_DUAL_PHASE;
		prot_desc->tx_phase_mode = MSP_DUAL_PHASE;
		prot_desc->rx_phase2_start_mode =
			MSP_PHASE2_START_MODE_FRAME_SYNC;
		prot_desc->tx_phase2_start_mode =
			MSP_PHASE2_START_MODE_FRAME_SYNC;
		prot_desc->rx_bit_transfer_format = MSP_BTF_MS_BIT_FIRST;
		prot_desc->tx_bit_transfer_format = MSP_BTF_MS_BIT_FIRST;
		prot_desc->rx_frame_length_1 = MSP_FRAME_LENGTH_1;
		prot_desc->rx_frame_length_2 = MSP_FRAME_LENGTH_1;
		prot_desc->tx_frame_length_1 = MSP_FRAME_LENGTH_1;
		prot_desc->tx_frame_length_2 = MSP_FRAME_LENGTH_1;
		prot_desc->rx_element_length_1 = MSP_ELEM_LENGTH_16;
		prot_desc->rx_element_length_2 = MSP_ELEM_LENGTH_16;
		prot_desc->tx_element_length_1 = MSP_ELEM_LENGTH_16;
		prot_desc->tx_element_length_2 = MSP_ELEM_LENGTH_16;
		prot_desc->rx_data_delay = MSP_DELAY_0;
		prot_desc->tx_data_delay = MSP_DELAY_0;
		prot_desc->rx_clock_pol = MSP_RISING_EDGE;
		prot_desc->tx_clock_pol = MSP_RISING_EDGE;
		prot_desc->rx_frame_sync_pol = MSP_FRAME_SYNC_POL_ACTIVE_HIGH;
		prot_desc->tx_frame_sync_pol = MSP_FRAME_SYNC_POL_ACTIVE_HIGH;
		prot_desc->rx_half_word_swap =
			MSP_HWS_BYTE_SWAP_IN_EACH_HALF_WORD;
		prot_desc->tx_half_word_swap = MSP_HWS_NO_SWAP;
		prot_desc->compression_mode = MSP_COMPRESS_MODE_LINEAR;
		prot_desc->expansion_mode = MSP_EXPAND_MODE_LINEAR;
		prot_desc->spi_clk_mode = MSP_SPI_CLOCK_MODE_NON_SPI;
		prot_desc->spi_burst_mode = MSP_SPI_BURST_MODE_DISABLE;
		prot_desc->frame_sync_ignore = MSP_FRAME_SYNC_IGNORE;

		msp_config->tx_clock_sel = 0;
		msp_config->tx_frame_sync_sel = 0;
		msp_config->rx_clock_sel = 0;
		msp_config->rx_frame_sync_sel = 0;
		msp_config->srg_clock_sel = 0;
		msp_config->frame_freq = rate;
		break;
	case SND_SOC_DAIFMT_DSP_B | SND_SOC_DAIFMT_CBS_CFS:
		prot_desc = &msp_config->protocol_desc;
		pr_info("%s: SND_SOC_DAIFMT_DSP_B | SND_SOC_DAIFMT_CBS_CFS\n",
			__func__);
		pr_info("%s: rate: %u channels: %d\n",
			__func__,
			rate,
			runtime->channels);
		msp_config->data_size = MSP_DATA_SIZE_16BIT;
		msp_config->default_protocol_desc = 0;
		msp_config->protocol = MSP_PCM_PROTOCOL;

		prot_desc->rx_phase_mode = MSP_SINGLE_PHASE;
		prot_desc->tx_phase_mode = MSP_SINGLE_PHASE;
		prot_desc->rx_phase2_start_mode =
			MSP_PHASE2_START_MODE_IMEDIATE;
		prot_desc->tx_phase2_start_mode =
			MSP_PHASE2_START_MODE_IMEDIATE;
		prot_desc->rx_bit_transfer_format = MSP_BTF_MS_BIT_FIRST;
		prot_desc->tx_bit_transfer_format = MSP_BTF_MS_BIT_FIRST;
		prot_desc->rx_frame_length_1 = MSP_FRAME_LENGTH_1;
		prot_desc->rx_frame_length_2 = MSP_FRAME_LENGTH_1;
		prot_desc->tx_frame_length_1 = MSP_FRAME_LENGTH_1;
		prot_desc->tx_frame_length_2 = MSP_FRAME_LENGTH_1;
		prot_desc->rx_element_length_1 = MSP_ELEM_LENGTH_16;
		prot_desc->rx_element_length_2 = MSP_ELEM_LENGTH_16;
		prot_desc->tx_element_length_1 = MSP_ELEM_LENGTH_16;
		prot_desc->tx_element_length_2 = MSP_ELEM_LENGTH_16;
		prot_desc->rx_data_delay = MSP_DELAY_0;
		prot_desc->tx_data_delay = MSP_DELAY_0;
		prot_desc->rx_clock_pol = MSP_FALLING_EDGE;
		prot_desc->tx_clock_pol = MSP_FALLING_EDGE;
		prot_desc->rx_frame_sync_pol = MSP_FRAME_SYNC_POL_ACTIVE_HIGH;
		prot_desc->tx_frame_sync_pol = MSP_FRAME_SYNC_POL_ACTIVE_HIGH;
		prot_desc->rx_half_word_swap = MSP_HWS_NO_SWAP;
		prot_desc->tx_half_word_swap = MSP_HWS_NO_SWAP;
		prot_desc->compression_mode = MSP_COMPRESS_MODE_LINEAR;
		prot_desc->expansion_mode = MSP_EXPAND_MODE_LINEAR;
		prot_desc->spi_clk_mode = MSP_SPI_CLOCK_MODE_NON_SPI;
		prot_desc->spi_burst_mode = MSP_SPI_BURST_MODE_DISABLE;
		prot_desc->frame_sync_ignore = MSP_FRAME_SYNC_IGNORE;

		prot_desc->frame_width = 0;
		switch (rate) {
		case 8000:
			prot_desc->frame_period = 249;
			break;
		case 16000:
			prot_desc->frame_period = 124;
			break;
		case 44100:
			prot_desc->frame_period = 63;
			break;
		case 48000:
		default:
			prot_desc->frame_period = 49;
			break;
		}
		prot_desc->total_clocks_for_one_frame =
			prot_desc->frame_period+1;

		msp_config->tx_clock_sel = TX_CLK_SEL_SRG;
		msp_config->tx_frame_sync_sel = TX_SYNC_SRG_PROG;
		msp_config->rx_clock_sel = RX_CLK_SEL_SRG;
		msp_config->rx_frame_sync_sel = RX_SYNC_SRG;
		msp_config->srg_clock_sel = 1 << SCKSEL_BIT;
		msp_config->frame_freq = rate;
		break;
	}
}

static int ux500_msp_dai_prepare(
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *msp_dai)
{
	int ret = 0;
	unsigned long flags_private;
	struct ux500_msp_dai_private_s *dai_private = msp_dai->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msp_generic_config msp_config;

	pr_info("%s: Enter (stream = %s, chip_select = %d).\n",
		__func__,
		substream->stream == SNDRV_PCM_STREAM_PLAYBACK ?
			"SNDRV_PCM_STREAM_PLAYBACK" :
			"SNDRV_PCM_STREAM_CAPTURE",
		(int)dai_private->i2s->chip_select);

	spin_lock_irqsave(&dai_private->lock, flags_private);

	if ((substream->stream == SNDRV_PCM_STREAM_PLAYBACK &&
		dai_private->rx_active) ||
		(substream->stream == SNDRV_PCM_STREAM_CAPTURE &&
		dai_private->tx_active)) {

		if (dai_private->rate != runtime->rate) {
			pr_err(
				"%s: Rate mismatch. index:%d"
				"rate:%d requested rate:%d\n",
				__func__,
				msp_dai->id,
				dai_private->rate,
				runtime->rate);
			ret = -EBUSY;
			goto cleanup;
		}
	} else if (!dai_private->rx_active && !dai_private->tx_active) {
		ret = 0;
		ux500_msp_dai_compile_msp_config(substream,
			dai_private->fmt,
			runtime->rate,
			&msp_config);

		ret = i2s_setup(dai_private->i2s->controller, &msp_config);
		if (ret < 0) {
			pr_err("%s: i2s_setup failed."
				" MSP index: %d return: %d\n",
				__func__,
				msp_dai->id,
				ret);
			goto cleanup;
		}
		dai_private->rate = runtime->rate;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dai_private->tx_active = 1;
	else
		dai_private->rx_active = 1;

cleanup:
	spin_unlock_irqrestore(&dai_private->lock, flags_private);
	return ret;
}

static int ux500_msp_dai_hw_params(
	struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *msp_dai)
{
	struct ux500_msp_dai_private_s *dai_private = msp_dai->private_data;

	pr_debug("%s: Enter stream: %s, MSP index: %d\n",
			__func__,
			substream->stream == SNDRV_PCM_STREAM_PLAYBACK ?
				"SNDRV_PCM_STREAM_PLAYBACK" :
				"SNDRV_PCM_STREAM_CAPTURE",
			(int)dai_private->i2s->chip_select);

	switch (dai_private->fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		if (params_channels(params) != 2) {
			pr_debug("%s: MSP index: %d:"
				"I2S format requires that channels: %d"
				" is set to two.\n",
				__func__,
				msp_dai->id,
				params_channels(params));
			return -EINVAL;
		}
		break;
	case SND_SOC_DAIFMT_DSP_B:
		if (params_channels(params) != 1) {
			pr_debug("%s: MSP index: %d:"
				" PCM format requires that channels: %d"
				" is set to one.\n",
				__func__,
				msp_dai->id,
				params_channels(params));
			return -EINVAL;
		}
		break;

	default:
		break;
	}

	return 0;
}

static int ux500_msp_dai_set_dai_fmt(
	struct snd_soc_dai *msp_dai,
	unsigned int fmt)
{
	struct ux500_msp_dai_private_s *dai_private =
		msp_dai->private_data;

	pr_debug("%s: MSP index: %d: Enter\n",
		__func__,
		(int)dai_private->i2s->chip_select);

	switch (fmt &
		(SND_SOC_DAIFMT_FORMAT_MASK | SND_SOC_DAIFMT_MASTER_MASK)) {
	case SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS:
	case SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBM_CFM:
	case SND_SOC_DAIFMT_DSP_B | SND_SOC_DAIFMT_CBS_CFS:
		break;

	default:
		pr_notice("Msp_dai: unsupported DAI format 0x%x\n",
			fmt);
		return -EINVAL;
	}

	dai_private->fmt = fmt;
	return 0;
}

static int ux500_msp_dai_trigger(
	struct snd_pcm_substream *substream,
	int cmd,
	struct snd_soc_dai *msp_dai)
{
	unsigned long flags;
	int ret = 0;
	struct ux500_msp_dai_private_s *dai_private =
		msp_dai->private_data;

	pr_debug("%s: Enter (stream = %s,"
		" chip_select = %d).\n",
		__func__,
		substream->stream == SNDRV_PCM_STREAM_PLAYBACK ?
			"SNDRV_PCM_STREAM_PLAYBACK" :
			"SNDRV_PCM_STREAM_CAPTURE",
		(int)dai_private->i2s->chip_select);

	spin_lock_irqsave(&dai_private->lock, flags);

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

	spin_unlock_irqrestore(&dai_private->lock, flags);
	return ret;
}

struct snd_soc_dai ux500_msp_dai[UX500_NBR_OF_DAI] =
{
	{
		.name = "ux500_i2s-0",
		.id = 0,
		.suspend = NULL,
		.resume = NULL,
		.playback = {
			.channels_min = UX500_MSP_MIN_CHANNELS,
			.channels_max = UX500_MSP_MAX_CHANNELS,
			.rates = UX500_I2S_RATES,
			.formats = UX500_I2S_FORMATS,
		},
		.capture = {
			.channels_min = UX500_MSP_MIN_CHANNELS,
			.channels_max = UX500_MSP_MAX_CHANNELS,
			.rates = UX500_I2S_RATES,
			.formats = UX500_I2S_FORMATS,
		},
		.ops = {
			.set_sysclk = NULL,
			.set_fmt = ux500_msp_dai_set_dai_fmt,
			.startup = ux500_msp_dai_startup,
			.shutdown = ux500_msp_dai_shutdown,
			.prepare = ux500_msp_dai_prepare,
			.trigger = ux500_msp_dai_trigger,
			.hw_params = ux500_msp_dai_hw_params,
		},
		.private_data = &ux500_msp_dai_private[0],
	},
	{
		.name = "ux500_i2s-1",
		.id = 1,
		.suspend = NULL,
		.resume = NULL,
		.playback = {
			.channels_min = UX500_MSP_MIN_CHANNELS,
			.channels_max = UX500_MSP_MAX_CHANNELS,
			.rates = UX500_I2S_RATES,
			.formats = UX500_I2S_FORMATS,
		},
		.capture = {
			.channels_min = UX500_MSP_MIN_CHANNELS,
			.channels_max = UX500_MSP_MAX_CHANNELS,
			.rates = UX500_I2S_RATES,
			.formats = UX500_I2S_FORMATS,
		},
		.ops = {
			.set_sysclk = NULL,
			.set_fmt = ux500_msp_dai_set_dai_fmt,
			.startup = ux500_msp_dai_startup,
			.shutdown = ux500_msp_dai_shutdown,
			.prepare = ux500_msp_dai_prepare,
			.trigger = ux500_msp_dai_trigger,
			.hw_params = ux500_msp_dai_hw_params,
		},
		.private_data = &ux500_msp_dai_private[1],
	},
	{
		.name = "ux500_i2s-2",
		.id = 2,
		.suspend = NULL,
		.resume = NULL,
		.playback = {
			.channels_min = UX500_MSP_MIN_CHANNELS,
			.channels_max = UX500_MSP_MAX_CHANNELS,
			.rates = UX500_I2S_RATES,
			.formats = UX500_I2S_FORMATS,
		},
		.capture = {
			.channels_min = UX500_MSP_MIN_CHANNELS,
			.channels_max = UX500_MSP_MAX_CHANNELS,
			.rates = UX500_I2S_RATES,
			.formats = UX500_I2S_FORMATS,
		},
		.ops = {
			.set_sysclk = NULL,
			.set_fmt = ux500_msp_dai_set_dai_fmt,
			.startup = ux500_msp_dai_startup,
			.shutdown = ux500_msp_dai_shutdown,
			.prepare = ux500_msp_dai_prepare,
			.trigger = ux500_msp_dai_trigger,
			.hw_params = ux500_msp_dai_hw_params,
		},
		.private_data = &ux500_msp_dai_private[2],
	},
};
EXPORT_SYMBOL(ux500_msp_dai);

static int __init ux500_msp_dai_init(void)
{
	int i;
	int ret = 0;

	pr_debug("%s: Enter.\n", __func__);

	ret = i2s_register_driver(&i2sdrv_i2s);
	if (ret < 0) {
		pr_err("%s: Unable to register I2S-driver\n",
			__func__);
		return ret;
	}

	for (i = 0; i < UX500_NBR_OF_DAI; i++) {
		pr_debug("%s: Register MSP dai %d.\n",
			__func__,
			i);
		ret = snd_soc_register_dai(&ux500_msp_dai[i]);
		if (ret < 0) {
			pr_err("MOP500_AB3550:"
				" Error: Failed to register MSP dai %d.\n",
				i);
			return ret;
		}
	}

	return ret;
}
module_init(ux500_msp_dai_init);

static void __exit ux500_msp_dai_exit(void)
{
	int i;

	pr_debug("%s: Enter.\n", __func__);

	i2s_unregister_driver(&i2sdrv_i2s);

	for (i = 0; i < UX500_NBR_OF_DAI; i++) {
		pr_debug("%s: Un-register MSP dai %d.\n",
			__func__,
			i);
		snd_soc_unregister_dai(&ux500_msp_dai[i]);
	}
}
module_exit(ux500_msp_dai_exit);

MODULE_LICENSE("GPL");
