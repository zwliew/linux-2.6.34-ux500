/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Roger Nilsson 	roger.xr.nilsson@stericsson.com
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
#include <linux/device.h>
#include <linux/io.h>
#include <sound/soc.h>

#include "ux500_pcm.h"
#include "ux500_msp_dai.h"
#include "mach/hardware.h"
#include "../codecs/cg29xx.h"

static struct platform_device *ux500_cg29xx_platform_device;

static int ux500_cg29xx_hw_params(
	struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	int ret = 0;

	pr_debug("%s: Enter.\n", __func__);
	pr_debug("%s: substream->pcm->name = %s.\n"
		"substream->pcm->id = %s.\n"
		"substream->name = %s.\n"
		"substream->number = %d.\n",
		__func__,
		substream->pcm->name,
		substream->pcm->id,
		substream->name,
		substream->number);

	if (cpu_dai->ops.set_fmt) {
		ret = snd_soc_dai_set_fmt(
			cpu_dai,
			SND_SOC_DAIFMT_DSP_B | SND_SOC_DAIFMT_CBS_CFS);
		if (ret < 0) {
			pr_debug("%s: snd_soc_dai_set_fmt"
				" failed with %d.\n",
				__func__,
				ret);
			return ret;
		}
	}
  return ret;
}

static struct snd_soc_ops ux500_cg29xx_ops = {
	.hw_params = ux500_cg29xx_hw_params,
};

static int ux500_hdmi_hw_params(
	struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	int ret = 0;

	pr_debug("%s: Enter.\n", __func__);

	pr_debug("%s: substream->pcm->name = %s.\n"
		"substream->pcm->id = %s.\n"
		"substream->name = %s.\n"
		"substream->number = %d.\n",
		__func__,
		substream->pcm->name,
		substream->pcm->id,
		substream->name,
		substream->number);

	if (cpu_dai->ops.set_fmt) {
		ret = snd_soc_dai_set_fmt(
			cpu_dai, SND_SOC_DAIFMT_I2S |
			SND_SOC_DAIFMT_CBM_CFM);
		if (ret < 0) {
			pr_debug("%s: snd_soc_dai_set_fmt"
				" failed with %d.\n",
				__func__,
				ret);
			return ret;
		}
	}

	return 0;
}

static struct snd_soc_ops ux500_hdmi_ops = {
	.hw_params = ux500_hdmi_hw_params,
};

struct snd_soc_dai_link ux500_cg29xx_dai_links[] = {
	{
		.name = "cg29xx_0",
		.stream_name = "cg29xx_0",
		.cpu_dai = &ux500_msp_dai[0],
		.codec_dai = &cg29xx_codec_dai[0],
		.init = NULL,
		.ops = &ux500_cg29xx_ops,
	},
	{
		.name = "hdmi_0",
		.stream_name = "hdmi_0",
		.cpu_dai = &ux500_msp_dai[2],
		.codec_dai = &cg29xx_codec_dai[0],
		.init = NULL,
		.ops = &ux500_hdmi_ops,
	},
};

static struct snd_soc_card ux500_cg29xx = {
	.name = "cg29xx",
	.probe = NULL,
	.dai_link = ux500_cg29xx_dai_links,
	.num_links = ARRAY_SIZE(ux500_cg29xx_dai_links),
	.platform = &ux500_soc_platform,
};

struct snd_soc_device ux500_cg29xx_drvdata = {
	.card = &ux500_cg29xx,
	.codec_dev = &soc_codec_dev_cg29xx,
};

static int __init ux500_cg29xx_soc_init(void)
{
	int i;
	int ret = 0;

	pr_debug("%s: Enter.\n", __func__);
	pr_debug("%s: Card name: %s\n",
		__func__,
		ux500_cg29xx_drvdata.card->name);

	for (i = 0; i < ARRAY_SIZE(ux500_cg29xx_dai_links); i++) {
		pr_debug("%s: DAI-link %d, name: %s\n",
			__func__,
			i,
			ux500_cg29xx_drvdata.card->dai_link[i].name);
		pr_debug("%s: DAI-link %d, stream_name: %s\n",
			__func__,
			i,
			ux500_cg29xx_drvdata.card->dai_link[i].stream_name);
	}

	pr_debug("%s: Allocate platform device (%s).\n",
		__func__,
		ux500_cg29xx_drvdata.card->name);
	ux500_cg29xx_platform_device =
		platform_device_alloc("soc-audio", -1);
	if (!ux500_cg29xx_platform_device)
		return -ENOMEM;
	pr_debug("%s: Set platform drvdata (%s).\n",
		__func__,
		ux500_cg29xx_drvdata.card->name);
	platform_set_drvdata(
		ux500_cg29xx_platform_device,
		&ux500_cg29xx_drvdata);

	pr_debug("%s: Add platform device (%s).\n",
		__func__,
		ux500_cg29xx_drvdata.card->name);
	ux500_cg29xx_drvdata.dev = &ux500_cg29xx_platform_device->dev;

	ret = platform_device_add(ux500_cg29xx_platform_device);
	if (ret) {
		pr_debug("%s: Error: Failed to add platform device (%s).\n",
			__func__,
			ux500_cg29xx_drvdata.card->name);
		platform_device_put(ux500_cg29xx_platform_device);
	}

	return ret;
}
module_init(ux500_cg29xx_soc_init);

static void __exit ux500_cg29xx_soc_exit(void)
{
	pr_debug("%s: Enter.\n", __func__);

	pr_debug("%s: Un-register platform device (%s).\n",
		__func__,
		ux500_cg29xx_drvdata.card->name);
	platform_device_unregister(ux500_cg29xx_platform_device);
}
module_exit(ux500_cg29xx_soc_exit);

MODULE_LICENSE("GPL");
