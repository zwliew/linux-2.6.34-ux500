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
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/io.h>

// SOC framework
#include <sound/soc.h>

// Platform driver
#include "u8500_pcm.h"
#include "u8500_msp_dai.h"

// AB3550 codec
extern struct snd_soc_dai ab3550_codec_dai;
extern struct snd_soc_codec_device soc_codec_dev_ab3550;




// Create the snd_soc_dai_link struct ---------------------------------------

static int u8500_ab3550_startup(struct snd_pcm_substream *substream)
{
  printk(KERN_DEBUG "MOP500_AB3550: u8500_ab3550_startup\n");

  return 0;
}

static void u8500_ab3550_shutdown(struct snd_pcm_substream *substream)
{
  printk(KERN_DEBUG "MOP500_AB3550: u8500_ab3550_shutdown\n");
}

static int u8500_ab3550_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	printk(KERN_DEBUG "MOP500_AB3550: u8500_ab3550_hw_params\n");

	return 0;
}

static struct snd_soc_ops u8500_ab3550_ops = {
  .startup = u8500_ab3550_startup,
  .shutdown = u8500_ab3550_shutdown,
  .hw_params = u8500_ab3550_hw_params,
};

struct snd_soc_dai_link u8500_ab3550_dai[2] = {
  {
    .name = "ab3550_0",
    .stream_name = "ab3550_0",
    .cpu_dai = &u8500_msp_dai[0],
    .codec_dai = &ab3550_codec_dai,
    .init = NULL,
    .ops = &u8500_ab3550_ops,
  },
  {
    .name = "ab3550_1",
    .stream_name = "ab3550_1",
    .cpu_dai = &u8500_msp_dai[1],
    .codec_dai = &ab3550_codec_dai,
    .init = NULL,
    .ops = &u8500_ab3550_ops,
  },
};
EXPORT_SYMBOL(u8500_ab3550_dai);



// Create the snd_soc_device struct ---------------------------------------

static struct snd_soc_card u8500_ab3550 = {
  .name = "u8500-ab3550",
  .probe = NULL,
  .dai_link = u8500_ab3550_dai,
  .num_links = 2,
  .platform = &u8500_soc_platform,
};

struct snd_soc_device u8500_ab3550_snd_devdata = {
  .card = &u8500_ab3550,
  .codec_dev = &soc_codec_dev_ab3550,
};
EXPORT_SYMBOL(u8500_ab3550_snd_devdata);



// Machine driver init and exit --------------------------------------------

static struct platform_device *u8500_ab3550_snd_device;

static int __init u8500_ab3550_init(void)
{
  int ret, i;
  struct snd_soc_device *socdev;
  struct snd_soc_codec *codec;

  printk(KERN_DEBUG "MOP500_AB3550: u8500_ab3550_init\n");

  // Register platform
  printk(KERN_DEBUG "MOP500_AB3550: Register platform.\n");
  ret = snd_soc_register_platform(&u8500_soc_platform);
  if (ret < 0)
	  printk(KERN_DEBUG "MOP500_AB3550: Error: Failed to register platform.\n");

  printk(KERN_DEBUG "MOP500_AB3550: Register codec dai.\n");
  snd_soc_register_dai(&ab3550_codec_dai);
  if (ret < 0)
	  printk(KERN_DEBUG "MOP500_AB3550: Error: Failed to register codec dai.\n");

  for (i = 0; i < U8500_NBR_OF_DAI; i++) {
	printk(KERN_DEBUG "MOP500_AB3550: Register MSP dai %d.\n", i);
	ret = snd_soc_register_dai(&u8500_msp_dai[i]);
	if (ret < 0)
	  printk(KERN_DEBUG "MOP500_AB3550: Error: Failed to register MSP dai %d.\n", i);
  }

  // Allocate platform device
  printk(KERN_DEBUG "MOP500_AB3550: Allocate platform device.\n");
  u8500_ab3550_snd_device = platform_device_alloc("soc-audio", -1);
  if (!u8500_ab3550_snd_device)
      return -ENOMEM;

  // Set platform drvdata
  printk(KERN_DEBUG "MOP500_AB3550: Set platform drvdata.\n");
  platform_set_drvdata(u8500_ab3550_snd_device, &u8500_ab3550_snd_devdata);
  if (ret < 0)
	  printk(KERN_DEBUG "MOP500_AB3550: Error: Failed to set drvdata.\n");

  // Add platform device
  printk(KERN_DEBUG "MOP500_AB3550: Add device.\n");
  u8500_ab3550_snd_devdata.dev = &u8500_ab3550_snd_device->dev;
  ret = platform_device_add(u8500_ab3550_snd_device);
  if (ret) {
	printk(KERN_DEBUG "MOP500_AB3550: Error: Failed to add platform device.\n");
    platform_device_put(u8500_ab3550_snd_device);
  }

  // Register IS2-driver
  printk(KERN_DEBUG "MOP500_AB3550: Register I2S-driver.\n");
  ret = u8500_platform_registerI2S();
  if (ret < 0)
    printk(KERN_DEBUG "MOP500_AB3550: Error: Failed to register I2S-driver.\n");

  return ret;
}



static void __exit u8500_ab3550_exit(void)
{
  printk(KERN_ALERT "MOP500_AB35500: u8500_ab3550_exit\n");

  platform_device_unregister(u8500_ab3550_snd_device);
}

module_init(u8500_ab3550_init);
module_exit(u8500_ab3550_exit);

MODULE_LICENSE("GPL");
