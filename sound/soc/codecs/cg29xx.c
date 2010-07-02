/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Ola Lilja ola.o.lilja@stericsson.com,
 *         for ST-Ericsson.
 *
 * License terms:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/bitops.h>
#include "cg29xx.h"


#define CG29XX_SUPPORTED_RATE (SNDRV_PCM_RATE_8000 | \
	SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000)

#define CG29XX_SUPPORTED_FMT (SNDRV_PCM_FMTBIT_S16_LE | \
			      SNDRV_PCM_FMTBIT_S24_LE)

static int cg29xx_pcm_prepare(
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai);

static int cg29xx_pcm_hw_params(
	struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *hw_params,
	struct snd_soc_dai *dai);

static void cg29xx_pcm_shutdown(
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai);

static int cg29xx_set_dai_sysclk(
	struct snd_soc_dai *codec_dai,
	int clk_id,
	unsigned int freq, int dir);

static int cg29xx_set_dai_fmt(
	struct snd_soc_dai *codec_dai,
	unsigned int fmt);

struct snd_soc_dai cg29xx_codec_dai[] = {
	{
		.name = "nullcodec_0",
		.playback = {
			.stream_name = "nullcodec_0",
			.channels_min = 1,
			.channels_max = 2,
			.rates = CG29XX_SUPPORTED_RATE,
			.formats = CG29XX_SUPPORTED_FMT,
		},
		.capture = {
			.stream_name = "nullcodec_0",
			.channels_min = 1,
			.channels_max = 2,
			.rates = CG29XX_SUPPORTED_RATE,
			.formats = CG29XX_SUPPORTED_FMT,
		},
		.ops = {
			.prepare = cg29xx_pcm_prepare,
			.hw_params = cg29xx_pcm_hw_params,
			.shutdown = cg29xx_pcm_shutdown,
			.set_sysclk = cg29xx_set_dai_sysclk,
			.set_fmt = cg29xx_set_dai_fmt
		},
	}
};
EXPORT_SYMBOL_GPL(cg29xx_codec_dai);

static int cg29xx_set_dai_sysclk(
	struct snd_soc_dai *codec_dai,
	int clk_id,
	unsigned int freq, int dir)
{
	printk(
		KERN_DEBUG
		"%s: %s called\n",
		__FILE__,
		__func__);
	return 0;
}

static int cg29xx_set_dai_fmt(
	struct snd_soc_dai *codec_dai,
	unsigned int fmt)
{
	printk(
		KERN_DEBUG
		"%s: %s called.\n",
		__FILE__,
		__func__);
	return 0;
}

static int cg29xx_pcm_prepare(
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	printk(KERN_DEBUG "%s: %s called.\n",
		__FILE__,
		__func__);
	return 0;
}

static void cg29xx_pcm_shutdown(
	struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	printk(KERN_DEBUG "%s: %s called.\n",
		__FILE__,
		__func__);
}

static int cg29xx_pcm_hw_params(
	struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *hw_params,
	struct snd_soc_dai *dai)
{
	printk(KERN_DEBUG "%s: %s called.\n", __FILE__, __func__);
	return 0;
}

static unsigned int nullcodec_codec_read(
	struct snd_soc_codec *codec,
	unsigned int ctl)
{
	printk(
		KERN_DEBUG
		"%s: %s called.\n",
		__FILE__,
		__func__);
	return 0;
}

static int nullcodec_codec_write(
	struct snd_soc_codec *codec,
	unsigned int ctl,
	unsigned int value)
{
	printk(
		KERN_DEBUG "%s: %s called.\n", __FILE__, __func__);
	return 0;
}

static int nullcodec_codec_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret;

	printk(KERN_DEBUG "%s called. pdev = %p.\n", __func__, pdev);

	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;
	codec->name = "NullCodec";
	codec->owner = THIS_MODULE;
	codec->dai = &cg29xx_codec_dai[0];
	codec->num_dai = 1;
	codec->read = nullcodec_codec_read;
	codec->write = nullcodec_codec_write;
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);


	mutex_init(&codec->mutex);

	socdev->codec = codec;

	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(
			KERN_ERR
			"%s: Error: to create new PCMs. error %d\n",
			__func__,
			ret);
		goto err1;
	}

	ret = snd_soc_init_card(socdev);
	if (ret < 0) {
		printk(
			KERN_ERR
			"%s: Error:"
			" Failed to register card.  error %d.\n",
			__func__,
			ret);
		goto err2;
	}

	return 0;

err2:
	snd_soc_free_pcms(socdev);
err1:
	kfree(codec);
	return ret;
}

static int nullcodec_codec_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	if (!codec)
		return 0;

	snd_soc_free_pcms(socdev);
	kfree(socdev->codec);

	printk(KERN_DEBUG "%s : pdev=%p.\n", __func__, pdev);
	return 0;
}

static int nullcodec_codec_suspend(struct platform_device *pdev,
				pm_message_t state)
{
	printk(KERN_DEBUG "%s : pdev=%p.\n", __func__, pdev);
	return 0;
}

static int nullcodec_codec_resume(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s : pdev=%p.\n", __func__, pdev);
	return 0;
}

struct snd_soc_codec_device soc_codec_dev_cg29xx = {
	.probe =	nullcodec_codec_probe,
	.remove =	nullcodec_codec_remove,
	.suspend =	nullcodec_codec_suspend,
	.resume =	nullcodec_codec_resume
};
EXPORT_SYMBOL_GPL(soc_codec_dev_cg29xx);

static int __devinit cg29xx_init(void)
{
	int ret1;
	int ret2 = 0;

	printk(
		KERN_DEBUG
		"%s called.\n",
		__func__);

	printk(
		KERN_DEBUG
		"%s: Register codec-dai.\n",
		__func__);
	ret1 = snd_soc_register_dai(&cg29xx_codec_dai[0]);
	if (ret1 < 0) {
		printk(
			KERN_DEBUG
			"%s: Error %d: Failed to register codec-dai.\n",
			__func__,
			ret1);
		ret2 = -1;
	}

	return ret2;
}

static void cg29xx_exit(void)
{
	printk(KERN_DEBUG "%s called.\n", __func__);

	printk(
		KERN_DEBUG
		"%s: Un-register codec-dai.\n",
		__func__);
	snd_soc_unregister_dai(&cg29xx_codec_dai[0]);
}

module_init(cg29xx_init);
module_exit(cg29xx_exit);

MODULE_DESCRIPTION("CG29xx driver");
MODULE_AUTHOR("www.stericsson.com");
MODULE_LICENSE("GPL");
