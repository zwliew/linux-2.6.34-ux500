/*----------------------------------------------------------------------------------*/
/* Author: Deepak Karda                                                             */
/* Copyright (C) 2009 ST-Ericsson Pvt. Ltd.                                         */
/*                                                                                  */
/* This program is free software; you can redistribute it and/or modify it under    */
/* the terms of the GNU General Public License as published by the Free             */
/* Software Foundation; either version 2.1 of the License, or (at your option)      */
/* any later version.                                                               */
/*                                                                                  */
/* This program is distributed in the hope that it will be useful, but WITHOUT      */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS    */
/* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.   */
/*                                                                                  */
/* You should have received a copy of the GNU General Public License                */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.             */
/*----------------------------------------------------------------------------------*/

/* This include must be defined at this point */
//#include <sound/driver.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/*  alsa system  */
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/control.h>
#include "u8500_alsa_ab8500.h"
#include <mach/msp.h>
#include <mach/debug.h>

#define ALSA_NAME		"DRIVER ALSA"
#define DRIVER_DEBUG	    CONFIG_STM_ALSA_DEBUG		/* enables/disables debug msgs */
#define DRIVER_DEBUG_PFX	ALSA_NAME               	/* msg header represents this module */
#define DRIVER_DBG	        KERN_ERR	                /* message level */

static struct platform_device *device;
static int active_user = 0;

/*
** Externel references
*/
#if DRIVER_DEBUG > 0
t_ab8500_codec_error dump_acodec_registers(void);
t_ab8500_codec_error dump_msp_registers(void);
#endif

extern int u8500_acodec_rates[MAX_NO_OF_RATES];
extern char *lpbk_state_in_texts[NUMBER_LOOPBACK_STATE];
extern char *switch_state_in_texts[NUMBER_SWITCH_STATE];
extern char *power_state_in_texts[NUMBER_POWER_STATE];
extern char *tdm_mode_state_in_texts[NUMBER_TDM_MODE_STATE];
extern char *direct_rendering_state_in_texts[NUMBER_DIRECT_RENDERING_STATE];
extern char *pcm_rendering_state_in_texts[NUMBER_PCM_RENDERING_STATE];
extern char *codec_dest_texts[NUMBER_OUTPUT_DEVICE];
extern char *codec_in_texts[NUMBER_INPUT_DEVICE];
extern struct driver_debug_st DBG_ST;
extern int second_config;
/*
** Declaration for local functions
*/
static int u8500_analog_lpbk_info(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_info *uinfo);
static int u8500_analog_lpbk_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
static int u8500_analog_lpbk_put(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);

static int u8500_digital_lpbk_info(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_info *uinfo);
static int u8500_digital_lpbk_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
static int u8500_digital_lpbk_put(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);

static int u8500_playback_vol_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo);
static int u8500_playback_vol_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uinfo);
static int u8500_playback_vol_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uinfo);

static int u8500_capture_vol_info(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_info *uinfo);
static int u8500_capture_vol_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
static int u8500_capture_vol_put(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);

static int u8500_playback_sink_info(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_info *uinfo);
static int u8500_playback_sink_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
static int u8500_playback_sink_put(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);

static int u8500_capture_src_ctrl_info(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_info *uinfo);
static int u8500_capture_src_ctrl_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
static int u8500_capture_src_ctrl_put(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);

static int u8500_playback_switch_ctrl_info(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_info *uinfo);
static int u8500_playback_switch_ctrl_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
static int u8500_playback_switch_ctrl_put(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);

static int u8500_capture_switch_ctrl_info(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_info *uinfo);
static int u8500_capture_switch_ctrl_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
static int u8500_capture_switch_ctrl_put(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);

static int u8500_playback_power_ctrl_info(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_info *uinfo);
static int u8500_playback_power_ctrl_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
static int u8500_playback_power_ctrl_put(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);

static int u8500_capture_power_ctrl_info(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_info *uinfo);
static int u8500_capture_power_ctrl_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
static int u8500_capture_power_ctrl_put(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);

static int u8500_tdm_mode_ctrl_info(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_info *uinfo);
static int u8500_tdm_mode_ctrl_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
static int u8500_tdm_mode_ctrl_put(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);

static int u8500_direct_rendering_mode_ctrl_info(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_info *uinfo);
static int u8500_direct_rendering_mode_ctrl_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
static int u8500_direct_rendering_mode_ctrl_put(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);

static int u8500_pcm_rendering_mode_ctrl_info(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_info *uinfo);
static int u8500_pcm_rendering_mode_ctrl_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
static int u8500_pcm_rendering_mode_ctrl_put(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);

#if 0 /* DUMP REGISTER CONTROL*/
static int u8500_dump_register_ctrl_info(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_info *uinfo);
static int u8500_dump_register_ctrl_get(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
static int u8500_dump_register_ctrl_put(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *uinfo);
#endif /* DUMP REGISTER CONTROL*/

/**
* configure_rate
* @substream - pointer to the playback/capture substream structure
*
*	This functions configures audio codec in to stream frequency frequency
*/

static int configure_rate(struct snd_pcm_substream *substream,t_u8500_acodec_config_need acodec_config_need)
{
	u8500_acodec_chip_t *chip = snd_pcm_substream_chip(substream);
	t_codec_sample_frequency sampling_frequency = 0;
	t_ab8500_codec_direction direction = 0;
	struct acodec_configuration acodec_config;
	int stream_id = substream->pstr->stream;

	FUNC_ENTER();
	switch (chip->freq)
	{
		case 48000:
			sampling_frequency = CODEC_SAMPLING_FREQ_48KHZ;
		break;
		default:
			printk("not supported frequnecy \n");
			stm_error("not supported frequnecy \n");
			return -EINVAL;
	}

	switch (stream_id) {
	case SNDRV_PCM_STREAM_PLAYBACK:
		direction = AB8500_CODEC_DIRECTION_OUT;
		break;
	case SNDRV_PCM_STREAM_CAPTURE:
		direction = AB8500_CODEC_DIRECTION_IN;
		break;
	default:
		stm_error(": wrong pcm stream\n");
		return -EINVAL;
	}


	stm_dbg(DBG_ST.alsa,"enabling audiocodec audio mode\n");
	acodec_config.direction = direction;
	acodec_config.input_frequency = T_CODEC_SAMPLING_FREQ_48KHZ;
	acodec_config.output_frequency = T_CODEC_SAMPLING_FREQ_48KHZ;
	acodec_config.mspClockSel = CODEC_MSP_APB_CLOCK;
	acodec_config.mspInClockFreq = CODEC_MSP_INPUT_FREQ_48MHZ;
	acodec_config.channels= chip->channels;
	acodec_config.user = 2;
	acodec_config.acodec_config_need = acodec_config_need;
	acodec_config.handler=dma_eot_handler;
	acodec_config.tx_callback_data= &chip->stream[ALSA_PCM_DEV][SNDRV_PCM_STREAM_PLAYBACK];
	acodec_config.rx_callback_data= &chip->stream[ALSA_PCM_DEV][SNDRV_PCM_STREAM_CAPTURE];
	acodec_config.direct_rendering_mode = chip->direct_rendering_mode;
	u8500_acodec_enable_audio_mode(&acodec_config);
	FUNC_EXIT();

	return 0;
}


/*
****************************************************************************************
*					           playback vol control									   *
****************************************************************************************
*/

struct snd_kcontrol_new u8500_playback_vol_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.subdevice = 0,
	.name = "PCM Playback Volume",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xfff,
	.info = u8500_playback_vol_info,
	.get = u8500_playback_vol_get,
	.put = u8500_playback_vol_put
};


/**
* u8500_playback_vol_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills playback volume info into user structure.
*/

static int u8500_playback_vol_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 100;
	uinfo->value.integer.step = 10;
	return 0;
}

/**
* u8500_playback_vol_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills the current volume setting to user structure.
*/

static int u8500_playback_vol_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);

	int *p_left_volume = NULL;
	int *p_right_volume = NULL;

	p_left_volume =  (int *)&uinfo->value.integer.value[0];
	p_right_volume = (int *)&uinfo->value.integer.value[1];

	u8500_acodec_get_output_volume(chip->output_device,p_left_volume,p_right_volume,USER_ALSA);
	return 0;
}

/**
* u8500_playback_vol_put
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure.
*
*	This functions sets the playback audio codec volume .
*/

static int u8500_playback_vol_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip = (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0, error = 0;

	if (chip->output_lvolume != uinfo->value.integer.value[0]
	    || chip->output_rvolume != uinfo->value.integer.value[1])
	{
		chip->output_lvolume = uinfo->value.integer.value[0];
		chip->output_rvolume = uinfo->value.integer.value[1];

		if (chip->output_lvolume > 100)
			chip->output_lvolume = 100;
		else if (chip->output_lvolume < 0)
			chip->output_lvolume = 0;

		if (chip->output_rvolume > 100)
			chip->output_rvolume = 100;
		else if (chip->output_rvolume < 0)
			chip->output_rvolume = 0;

		error = u8500_acodec_set_output_volume(chip->output_device,chip->output_lvolume,chip->output_rvolume,USER_ALSA);

		if (error)
		{
			stm_error(" : set volume for speaker/headphone failed\n");
			return changed;
		}
		changed = 1;
	}

	return changed;
}

/*
****************************************************************************************
*					           capture vol control									   *
****************************************************************************************
*/

struct snd_kcontrol_new u8500_capture_vol_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.subdevice = 1,
	.name = "PCM Capture Volume",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xfff,
	.info = u8500_capture_vol_info,
	.get = u8500_capture_vol_get,
	.put = u8500_capture_vol_put
};


/**
* u8500_capture_vol_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills capture volume info into user structure.
*/
static int u8500_capture_vol_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 100;
	uinfo->value.integer.step = 10;
	return 0;
}

/**
* u8500_capture_vol_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions returns the current capture volume setting to user structure.
*/

static int u8500_capture_vol_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);

	int *p_left_volume = NULL;
	int *p_right_volume = NULL;

	p_left_volume =  (int *)&uinfo->value.integer.value[0];
	p_right_volume = (int *)&uinfo->value.integer.value[1];

	u8500_acodec_get_input_volume(chip->input_device,p_left_volume,p_right_volume,USER_ALSA);
	return 0;
}

/**
* u8500_capture_vol_put
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure.
*
*	This functions sets the capture audio codec volume with values provided.
*/

static int u8500_capture_vol_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0, error = 0;

	if (chip->input_lvolume != uinfo->value.integer.value[0]
	    || chip->input_rvolume != uinfo->value.integer.value[1]) {
		chip->input_lvolume = uinfo->value.integer.value[0];
		chip->input_rvolume = uinfo->value.integer.value[1];

		if (chip->input_lvolume > 100)
			chip->input_lvolume = 100;
		else if (chip->input_lvolume < 0)
			chip->input_lvolume = 0;

		if (chip->input_rvolume > 100)
			chip->input_rvolume = 100;
		else if (chip->input_rvolume < 0)
			chip->input_rvolume = 0;

		error = u8500_acodec_set_input_volume(chip->input_device,
						  chip->input_rvolume,
						  chip->input_lvolume,
						  USER_ALSA);
		if (error) {
			stm_error(" : set input volume failed\n");
			return changed;
		}
		changed = 1;
	}

	return changed;
}

/*
****************************************************************************************
*					           playback sink control								   *
****************************************************************************************
*/

static struct snd_kcontrol_new u8500_playback_sink_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.subdevice = 0,
	.name = "PCM Playback Sink",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xffff,
	.info = u8500_playback_sink_info,
	.get = u8500_playback_sink_get,
	.put = u8500_playback_sink_put
};

/**
* u8500_playback_sink_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills playback device info into user structure.
*/
static int u8500_playback_sink_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->value.enumerated.items = NUMBER_OUTPUT_DEVICE;
	uinfo->count = 1;
	if (uinfo->value.enumerated.item >= NUMBER_OUTPUT_DEVICE)
		uinfo->value.enumerated.item = NUMBER_OUTPUT_DEVICE - 1;
	strcpy(uinfo->value.enumerated.name,
	       codec_dest_texts[uinfo->value.enumerated.item]);
	return 0;
}

/**
* u8500_playback_sink_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions returns the current playback device selected.
*/
static int u8500_playback_sink_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	uinfo->value.enumerated.item[0] = chip->output_device;
	return 0;
}

/**
* u8500_playback_sink_put
* @kcontrol - pointer to the snd_kcontrol structure
* @	.
*
*	This functions sets the playback device.
*/
static int u8500_playback_sink_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0, error;

	if (chip->output_device != uinfo->value.enumerated.item[0]) {
		chip->output_device = uinfo->value.enumerated.item[0];
		error =
		    u8500_acodec_select_output(chip->output_device,
						 USER_ALSA);
		if (error) {
			stm_error(" : select output failed\n");
			return changed;
		}
		changed = 1;
	}
	return changed;
}

/*
****************************************************************************************
*					           capture src control									   *
****************************************************************************************
*/

static struct snd_kcontrol_new u8500_capture_src_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.subdevice = 1,
	.name = "PCM Capture Source",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xffff,
	.info = u8500_capture_src_ctrl_info,
	.get = u8500_capture_src_ctrl_get,
	.put = u8500_capture_src_ctrl_put
};


/**
* u8500_capture_src_ctrl_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills capture device info into user structure.
*/
static int u8500_capture_src_ctrl_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->value.enumerated.items = NUMBER_INPUT_DEVICE;
	uinfo->count = 1;
	if (uinfo->value.enumerated.item >= NUMBER_INPUT_DEVICE)
		uinfo->value.enumerated.item = NUMBER_INPUT_DEVICE - 1;
	strcpy(uinfo->value.enumerated.name,
	       codec_in_texts[uinfo->value.enumerated.item]);
	return 0;
}

/**
* u8500_capture_src_ctrl_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions returns the current capture device selected.
*/
static int u8500_capture_src_ctrl_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	uinfo->value.enumerated.item[0] = chip->input_device;
	return 0;
}

/**
* u8500_capture_src_ctrl_put
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure,
*
*	This functions sets the capture device.
*/
static int u8500_capture_src_ctrl_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0, error;

	if (chip->input_device != uinfo->value.enumerated.item[0]) {
		chip->input_device = uinfo->value.enumerated.item[0];
		error =
		    u8500_acodec_select_input(chip->input_device, USER_ALSA);
		if (error) {
			stm_error(" : select input failed\n");
			return changed;
		}
		changed = 1;
	}
	return changed;
}

/*
***************************************************************************************
*						           analog lpbk control								  *
***************************************************************************************
*/

struct snd_kcontrol_new u8500_analog_lpbk_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.name = "Analog Loopback",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xfff,
	.info = u8500_analog_lpbk_info,
	.get = u8500_analog_lpbk_get,
	.put = u8500_analog_lpbk_put
};

/**
* u8500_analog_lpbk_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills playback device info into user structure.
*/
static int u8500_analog_lpbk_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->value.enumerated.items = NUMBER_LOOPBACK_STATE;
	uinfo->count = 1;
	if (uinfo->value.enumerated.item >= NUMBER_LOOPBACK_STATE)
		uinfo->value.enumerated.item = NUMBER_LOOPBACK_STATE - 1;
	strcpy(uinfo->value.enumerated.name,
	       lpbk_state_in_texts[uinfo->value.enumerated.item]);
	return 0;
}

/**
* u8500_analog_lpbk_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions returns the current playback device selected.
*/
static int u8500_analog_lpbk_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	uinfo->value.enumerated.item[0] = chip->analog_lpbk;
	return 0;
}

/**
* u8500_analog_lpbk_put
* @kcontrol - pointer to the snd_kcontrol structure
* @	.
*
*	This functions sets the playback device.
*/
static int u8500_analog_lpbk_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0;
	t_ab8500_codec_error error;

	if (chip->analog_lpbk != uinfo->value.enumerated.item[0])
	{
		chip->analog_lpbk = uinfo->value.enumerated.item[0];

		error = u8500_acodec_toggle_analog_lpbk(chip->analog_lpbk,USER_ALSA);

		if (AB8500_CODEC_OK != error)
		{
			stm_error(" : select u8500_acodec_set_analog_lpbk_state failed\n");
			return changed;
		}
		changed = 1;
	}
	return changed;
}

/*
****************************************************************************************
*					           digital lpbk control									   *
****************************************************************************************
*/

struct snd_kcontrol_new u8500_digital_lpbk_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.name = "Digital Loopback",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xfff,
	.info = u8500_digital_lpbk_info,
	.get = u8500_digital_lpbk_get,
	.put = u8500_digital_lpbk_put
};


/**
* u8500_digital_lpbk_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills playback device info into user structure.
*/
static int u8500_digital_lpbk_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->value.enumerated.items = NUMBER_LOOPBACK_STATE;
	uinfo->count = 1;
	if (uinfo->value.enumerated.item >= NUMBER_LOOPBACK_STATE)
		uinfo->value.enumerated.item = NUMBER_LOOPBACK_STATE - 1;
	strcpy(uinfo->value.enumerated.name,
	       lpbk_state_in_texts[uinfo->value.enumerated.item]);
	return 0;
}

/**
* u8500_digital_lpbk_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions returns the current playback device selected.
*/
static int u8500_digital_lpbk_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	uinfo->value.enumerated.item[0] = chip->digital_lpbk;
	return 0;
}

/**
* u8500_analog_lpbk_put
* @kcontrol - pointer to the snd_kcontrol structure
* @	.
*
*	This functions sets the playback device.
*/
static int u8500_digital_lpbk_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0;
	t_ab8500_codec_error error;

	if (chip->digital_lpbk != uinfo->value.enumerated.item[0])
	{
		chip->digital_lpbk = uinfo->value.enumerated.item[0];

		error = u8500_acodec_toggle_digital_lpbk(chip->digital_lpbk,USER_ALSA);

		/*if((error = u8500_acodec_set_output_volume(chip->output_device,50,50,USER_ALSA)))
		{
			stm_error(" : set output volume failed\n");
			return error;
		}

		if ((error = u8500_acodec_set_input_volume(chip->input_device,50,50,USER_ALSA)))
		{
			stm_error(" : set input volume failed\n");
			return error;
		}*/


		if (AB8500_CODEC_OK != error)
		{
			stm_error(" : select u8500_acodec_set_digital_lpbk_state failed\n");
			return changed;
		}
		changed = 1;
	}
	return changed;
}

/*
****************************************************************************************
*					           playback switch control								   *
****************************************************************************************
*/

struct snd_kcontrol_new u8500_playback_switch_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.subdevice = 0,
	.name = "PCM Playback Mute",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xfff,
	.info = u8500_playback_switch_ctrl_info,
	.get = u8500_playback_switch_ctrl_get,
	.put = u8500_playback_switch_ctrl_put
};

/**
* u8500_playback_switch_ctrl_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills playback device info into user structure.
*/
static int u8500_playback_switch_ctrl_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->value.enumerated.items = NUMBER_SWITCH_STATE;
	uinfo->count = 1;
	if (uinfo->value.enumerated.item >= NUMBER_SWITCH_STATE)
		uinfo->value.enumerated.item = NUMBER_SWITCH_STATE - 1;
	strcpy(uinfo->value.enumerated.name,
	       switch_state_in_texts[uinfo->value.enumerated.item]);
	return 0;
}

/**
* u8500_playback_switch_ctrl_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions returns the current playback device selected.
*/
static int u8500_playback_switch_ctrl_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	uinfo->value.enumerated.item[0] = chip->playback_switch;
	return 0;
}

/**
* u8500_playback_switch_ctrl_put
* @kcontrol - pointer to the snd_kcontrol structure
* @	.
*
*	This functions sets the playback device.
*/
static int u8500_playback_switch_ctrl_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0;
	t_ab8500_codec_error error;

	if (chip->playback_switch != uinfo->value.enumerated.item[0])
	{
		chip->playback_switch = uinfo->value.enumerated.item[0];

		error = u8500_acodec_toggle_playback_mute_control(chip->output_device, chip->playback_switch,USER_ALSA);

		if (AB8500_CODEC_OK != error)
		{
			stm_error(" : select u8500_playback_switch_ctrl_put failed\n");
			return changed;
		}
		changed = 1;
	}
	return changed;
}

/*
****************************************************************************************
*					           Capture switch control								   *
****************************************************************************************
*/

struct snd_kcontrol_new u8500_capture_switch_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.subdevice = 1,
	.name = "PCM Capture Mute",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xfff,
	.info = u8500_capture_switch_ctrl_info,
	.get = u8500_capture_switch_ctrl_get,
	.put = u8500_capture_switch_ctrl_put
};

/**
* u8500_capture_switch_ctrl_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills playback device info into user structure.
*/
static int u8500_capture_switch_ctrl_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->value.enumerated.items = NUMBER_SWITCH_STATE;
	uinfo->count = 1;
	if (uinfo->value.enumerated.item >= NUMBER_SWITCH_STATE)
		uinfo->value.enumerated.item = NUMBER_SWITCH_STATE - 1;
	strcpy(uinfo->value.enumerated.name,
	       switch_state_in_texts[uinfo->value.enumerated.item]);
	return 0;
}

/**
* u8500_capture_switch_ctrl_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions returns the current playback device selected.
*/
static int u8500_capture_switch_ctrl_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	uinfo->value.enumerated.item[0] = chip->capture_switch;
	return 0;
}

/**
* u8500_capture_switch_ctrl_put
* @kcontrol - pointer to the snd_kcontrol structure
* @	.
*
*	This functions sets the playback device.
*/
static int u8500_capture_switch_ctrl_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0;
	t_ab8500_codec_error error;

	if (chip->capture_switch != uinfo->value.enumerated.item[0])
	{
		chip->capture_switch = uinfo->value.enumerated.item[0];

		error = u8500_acodec_toggle_capture_mute_control(chip->input_device, chip->capture_switch,USER_ALSA);

		if (AB8500_CODEC_OK != error)
		{
			stm_error(" : select u8500_capture_switch_ctrl_put failed\n");
			return changed;
		}
		changed = 1;
	}
	return changed;
}

/*
****************************************************************************************
*					           playback power control								   *
****************************************************************************************
*/

struct snd_kcontrol_new u8500_playback_power_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.subdevice = 0,
	.name = "PCM Playback Power",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xfff,
	.info = u8500_playback_power_ctrl_info,
	.get = u8500_playback_power_ctrl_get,
	.put = u8500_playback_power_ctrl_put
};

/**
* u8500_playback_power_ctrl_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills playback device info into user structure.
*/
static int u8500_playback_power_ctrl_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->value.enumerated.items = NUMBER_POWER_STATE;
	uinfo->count = 1;
	if (uinfo->value.enumerated.item >= NUMBER_POWER_STATE)
		uinfo->value.enumerated.item = NUMBER_POWER_STATE - 1;
	strcpy(uinfo->value.enumerated.name,
	       power_state_in_texts[uinfo->value.enumerated.item]);
	return 0;
}

/**
* u8500_playback_power_ctrl_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions returns the current playback device selected.
*/
static int u8500_playback_power_ctrl_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	uinfo->value.enumerated.item[0] = u8500_acodec_get_dest_power_state(chip->output_device);
	return 0;
}

/**
* u8500_playback_power_ctrl_put
* @kcontrol - pointer to the snd_kcontrol structure
* @	.
*
*	This functions sets the playback device.
*/
static int u8500_playback_power_ctrl_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0;
	t_ab8500_codec_error error;
	t_u8500_bool_state power_state;

	power_state = u8500_acodec_get_dest_power_state(chip->output_device);

	if (power_state  != uinfo->value.enumerated.item[0])
	{
		power_state  = uinfo->value.enumerated.item[0];

		error = u8500_acodec_set_dest_power_cntrl(chip->output_device,power_state);

		if (AB8500_CODEC_OK != error)
		{
			stm_error(" : select u8500_acodec_set_dest_power_cntrl failed\n");
			return changed;
		}

		/* configure the volume settings for the acodec */
		if ((error = u8500_acodec_set_output_volume(chip->output_device,chip->output_lvolume,chip->output_rvolume,USER_ALSA)))
		{
			stm_error(" : set output volume failed\n");
			return error;
		}
		changed = 1;
	}
	return changed;
}

/*
****************************************************************************************
*					           capture power control								   *
****************************************************************************************
*/

struct snd_kcontrol_new u8500_capture_power_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.subdevice = 0,
	.name = "PCM Capture Power",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xfff,
	.info = u8500_capture_power_ctrl_info,
	.get = u8500_capture_power_ctrl_get,
	.put = u8500_capture_power_ctrl_put
};

/**
* u8500_capture_power_ctrl_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills playback device info into user structure.
*/
static int u8500_capture_power_ctrl_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->value.enumerated.items = NUMBER_POWER_STATE;
	uinfo->count = 1;
	if (uinfo->value.enumerated.item >= NUMBER_POWER_STATE)
		uinfo->value.enumerated.item = NUMBER_POWER_STATE - 1;
	strcpy(uinfo->value.enumerated.name,
	       power_state_in_texts[uinfo->value.enumerated.item]);
	return 0;
}

/**
* u8500_capture_power_ctrl_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions returns the current playback device selected.
*/
static int u8500_capture_power_ctrl_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	uinfo->value.enumerated.item[0] = u8500_acodec_get_src_power_state(chip->input_device);
	return 0;
}

/**
* u8500_capture_power_ctrl_put
* @kcontrol - pointer to the snd_kcontrol structure
* @	.
*
*	This functions sets the playback device.
*/
static int u8500_capture_power_ctrl_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0;
	t_ab8500_codec_error error;
	t_u8500_bool_state power_state;

	power_state = u8500_acodec_get_src_power_state(chip->input_device);

	if (power_state  != uinfo->value.enumerated.item[0])
	{
		power_state  = uinfo->value.enumerated.item[0];

		error = u8500_acodec_set_src_power_cntrl(chip->input_device,power_state);

		if (AB8500_CODEC_OK != error)
		{
			stm_error(" : select u8500_acodec_set_src_power_cntrl failed\n");
			return changed;
		}
		changed = 1;
	}
	return changed;
}
/*
****************************************************************************************
*					           TDM 8 channel mode control	     					   *
****************************************************************************************
*/

struct snd_kcontrol_new u8500_tdm_mode_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.subdevice = 0,
	.name = "TDM 8 Channal Mode",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xfff,
	.info = u8500_tdm_mode_ctrl_info,
	.get = u8500_tdm_mode_ctrl_get,
	.put = u8500_tdm_mode_ctrl_put
};

/**
* u8500_tdm_mode_ctrl_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills playback device info into user structure.
*/
static int u8500_tdm_mode_ctrl_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->value.enumerated.items = NUMBER_TDM_MODE_STATE;
	uinfo->count = 1;
	if (uinfo->value.enumerated.item >= NUMBER_TDM_MODE_STATE)
		uinfo->value.enumerated.item = NUMBER_TDM_MODE_STATE - 1;
	strcpy(uinfo->value.enumerated.name,
	       tdm_mode_state_in_texts[uinfo->value.enumerated.item]);
	return 0;
}

/**
* u8500_tdm_mode_ctrl_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions returns the current playback device selected.
*/
static int u8500_tdm_mode_ctrl_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	uinfo->value.enumerated.item[0] = chip->tdm8_ch_mode;
	return 0;
}

/**
* u8500_tdm_mode_ctrl_put
* @kcontrol - pointer to the snd_kcontrol structure
* @	.
*
*	This functions sets the playback device.
*/
static int u8500_tdm_mode_ctrl_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0;
	t_ab8500_codec_error error;

	chip->tdm8_ch_mode = uinfo->value.enumerated.item[0];

	if(ENABLE == chip->tdm8_ch_mode)
		printk("\n TDM 8 channel mode enabled\n");
	else
		printk("\n TDM 8 channel mode disabled\n");

	changed = 1;

	return changed;
}



/*
****************************************************************************************
*					           Direct Rendering Mode control	     				   *
****************************************************************************************
*/

struct snd_kcontrol_new u8500_direct_rendering_mode_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.name = "Direct Rendering Mode",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xfff,
	.info = u8500_direct_rendering_mode_ctrl_info,
	.get = u8500_direct_rendering_mode_ctrl_get,
	.put = u8500_direct_rendering_mode_ctrl_put
};

/**
* u8500_direct_rendering_mode_ctrl_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills playback device info into user structure.
*/
static int u8500_direct_rendering_mode_ctrl_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->value.enumerated.items = NUMBER_DIRECT_RENDERING_STATE;
	uinfo->count = 1;
	if (uinfo->value.enumerated.item >= NUMBER_DIRECT_RENDERING_STATE)
		uinfo->value.enumerated.item = NUMBER_DIRECT_RENDERING_STATE - 1;
	strcpy(uinfo->value.enumerated.name,
	       direct_rendering_state_in_texts[uinfo->value.enumerated.item]);
	return 0;
}

/**
* u8500_direct_rendering_mode_ctrl_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions returns the current playback device selected.
*/
static int u8500_direct_rendering_mode_ctrl_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	uinfo->value.enumerated.item[0] = chip->direct_rendering_mode;
	return 0;
}

/**
* u8500_direct_rendering_mode_ctrl_put
* @kcontrol - pointer to the snd_kcontrol structure
* @	.
*
*	This functions sets the playback device.
*/
static int u8500_direct_rendering_mode_ctrl_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0;
	t_ab8500_codec_error error;

	chip->direct_rendering_mode = uinfo->value.enumerated.item[0];

	if(ENABLE == chip->direct_rendering_mode)
	{
		stm_gpio_altfuncenable(GPIO_ALT_MSP_1);
		printk("\n stm_gpio_altfuncenable for GPIO_ALT_MSP_1\n");
		printk("\n Direct Rendering mode enabled\n");
	}
	else
	{
		stm_gpio_altfuncdisable(GPIO_ALT_MSP_1);
		printk("\n stm_gpio_altfuncdisable for GPIO_ALT_MSP_1\n");
		printk("\n Direct Rendering mode disabled\n");
	}

	changed = 1;

	return changed;
}


/*
****************************************************************************************
*					           PCM Rendering Mode control	     				   *
****************************************************************************************
*/

struct snd_kcontrol_new u8500_pcm_rendering_mode_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.name = "PCM Rendering Mode",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xfff,
	.info = u8500_pcm_rendering_mode_ctrl_info,
	.get = u8500_pcm_rendering_mode_ctrl_get,
	.put = u8500_pcm_rendering_mode_ctrl_put
};

/**
* u8500_pcm_rendering_mode_ctrl_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills playback device info into user structure.
*/
static int u8500_pcm_rendering_mode_ctrl_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->value.enumerated.items = NUMBER_PCM_RENDERING_STATE;
	uinfo->count = 3;
	if (uinfo->value.enumerated.item >= NUMBER_PCM_RENDERING_STATE)
		uinfo->value.enumerated.item = NUMBER_PCM_RENDERING_STATE - 1;
	strcpy(uinfo->value.enumerated.name,
	       pcm_rendering_state_in_texts[uinfo->value.enumerated.item]);
	return 0;
}

/**
* u8500_pcm_rendering_mode_ctrl_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions returns the current playback device selected.
*/
static int u8500_pcm_rendering_mode_ctrl_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	uinfo->value.enumerated.item[0] = chip->burst_fifo_mode;
	uinfo->value.enumerated.item[1] = chip->fm_playback_mode;
	uinfo->value.enumerated.item[2] = chip->fm_tx_mode;
	return 0;
}

/**
* u8500_pcm_rendering_mode_ctrl_put
* @kcontrol - pointer to the snd_kcontrol structure
* @	.
*
*	This functions sets the playback device.
*/
static int u8500_pcm_rendering_mode_ctrl_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0;
	t_ab8500_codec_error error;

	if(RENDERING_PENDING == uinfo->value.enumerated.item[0])
	{
		return changed;
	}
	if(chip->burst_fifo_mode != uinfo->value.enumerated.item[0])
	{
		chip->burst_fifo_mode = uinfo->value.enumerated.item[0];
		u8500_acodec_set_burst_mode_fifo(chip->burst_fifo_mode);
	}

	chip->fm_playback_mode = uinfo->value.enumerated.item[1];
	chip->fm_tx_mode = uinfo->value.enumerated.item[2];

	changed = 1;

	return changed;
}

#if 0 /* DUMP REGISTER CONTROL*/
/*
****************************************************************************************
*					           dump registers control	     				           *
****************************************************************************************
*/

struct snd_kcontrol_new u8500_dump_register_ctrl = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.device = 0,
	.name = "",
	.index = 0,
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.private_value = 0xfff,
	.info = u8500_dump_register_ctrl_info,
	.get = u8500_dump_register_ctrl_get,
	.put = u8500_dump_register_ctrl_put
};

/**
* u8500_dump_register_ctrl_info
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions fills playback device info into user structure.
*/
static int u8500_dump_register_ctrl_info(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_info *uinfo)
{

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->value.enumerated.items = NUMBER_PCM_RENDERING_STATE;
	uinfo->count = 1;
	if (uinfo->value.enumerated.item >= NUMBER_PCM_RENDERING_STATE)
		uinfo->value.enumerated.item = NUMBER_PCM_RENDERING_STATE - 1;
	strcpy(uinfo->value.enumerated.name,
	       pcm_rendering_state_in_texts[uinfo->value.enumerated.item]);
	return 0;
}

/**
* u8500_dump_register_ctrl_get
* @kcontrol - pointer to the snd_kcontrol structure
* @uinfo - pointer to the snd_ctl_elem_info structure, this is filled by the function
*
*	This functions returns the current playback device selected.
*/
static int u8500_dump_register_ctrl_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	uinfo->value.enumerated.item[0] = chip->burst_fifo_mode;
	uinfo->value.enumerated.item[1] = chip->fm_playback_mode;
	uinfo->value.enumerated.item[2] = chip->fm_tx_mode;
	return 0;
}

/**
* u8500_dump_register_ctrl_put
* @kcontrol - pointer to the snd_kcontrol structure
* @	.
*
*	This functions sets the playback device.
*/
static int u8500_dump_register_ctrl_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *uinfo)
{
	u8500_acodec_chip_t *chip =
	    (u8500_acodec_chip_t *) snd_kcontrol_chip(kcontrol);
	int changed = 0;
	t_ab8500_codec_error error;

	if(RENDERING_PENDING == uinfo->value.enumerated.item[0])
	{
		return changed;
	}
	if(chip->burst_fifo_mode != uinfo->value.enumerated.item[0])
	{
		chip->burst_fifo_mode = uinfo->value.enumerated.item[0];
		//u8500_acodec_set_burst_mode_fifo(chip->burst_fifo_mode);
	}

	chip->fm_playback_mode = uinfo->value.enumerated.item[1];
	chip->fm_tx_mode = uinfo->value.enumerated.item[2];

	changed = 1;

	return changed;
}

#endif /* DUMP REGISTER CONTROL*/


/* Hardware description , this structure (struct snd_pcm_hardware )
 * contains the definitions of the fundamental hardware configuration.
 * This configuration will be applied on the runtime structure
 */
static struct snd_pcm_hardware snd_u8500_playback_hw = {
	.info =
	    (SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_MMAP |
	     SNDRV_PCM_INFO_RESUME | SNDRV_PCM_INFO_PAUSE),
	.formats =
	    SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE |
	    SNDRV_PCM_FMTBIT_S16_BE | SNDRV_PCM_FMTBIT_U16_BE,
	.rates = SNDRV_PCM_RATE_KNOT,
	.rate_min = MIN_RATE_PLAYBACK,
	.rate_max = MAX_RATE_PLAYBACK,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = NMDK_BUFFER_SIZE,
	.period_bytes_min = 128,
	.period_bytes_max = PAGE_SIZE,
	.periods_min = NMDK_BUFFER_SIZE / PAGE_SIZE,
	.periods_max = NMDK_BUFFER_SIZE / 128
};

static struct snd_pcm_hardware snd_u8500_capture_hw = {
	.info =
	    (SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_MMAP |
	     SNDRV_PCM_INFO_RESUME | SNDRV_PCM_INFO_PAUSE),
	.formats =
	    SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE |
	    SNDRV_PCM_FMTBIT_S16_BE | SNDRV_PCM_FMTBIT_U16_BE,
	.rates = SNDRV_PCM_RATE_KNOT,
	.rate_min = MIN_RATE_CAPTURE,
	.rate_max = MAX_RATE_CAPTURE,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = NMDK_BUFFER_SIZE,
	.period_bytes_min = 128,
	.period_bytes_max = PAGE_SIZE,
	.periods_min = NMDK_BUFFER_SIZE / PAGE_SIZE,
	.periods_max = NMDK_BUFFER_SIZE / 128
};

static struct snd_pcm_hw_constraint_list constraints_rate = {
	.count = sizeof(u8500_acodec_rates) / sizeof(u8500_acodec_rates[0]),
	.list = u8500_acodec_rates,
	.mask = 0,
};

/**
 * snd_u8500_alsa_pcm_close
 * @substream - pointer to the playback/capture substream structure
 *
 *  This routine is used by alsa framework to close a pcm stream .
 *  Here a dma pipe is disabled and freed.
 */
static int snd_u8500_alsa_pcm_close(struct snd_pcm_substream *substream)
{
	int stream_id, error = 0;
	u8500_acodec_chip_t *chip = snd_pcm_substream_chip(substream);
	audio_stream_t *ptr_audio_stream = NULL;

	stream_id = substream->pstr->stream;
	ptr_audio_stream = &chip->stream[ALSA_PCM_DEV][stream_id];

	if(ENABLE == chip->direct_rendering_mode)
	{
		ptr_audio_stream->pipeId = -1;
		ptr_audio_stream->active = 0;
		ptr_audio_stream->period = 0;
		ptr_audio_stream->periods = 0;
		ptr_audio_stream->old_offset = 0;
		ptr_audio_stream->substream = NULL;

		if (!(--active_user)) {
			error = u8500_acodec_unsetuser(USER_ALSA);
		}
		return 0;
	}
	else
	{
	stm_close_alsa(chip,ALSA_PCM_DEV,stream_id);

	stm_dbg(DBG_ST.alsa," pipe: %i closed:\n", (int)ptr_audio_stream->pipeId);

	/* reset the different variables to default */

	ptr_audio_stream->pipeId = -1;
	ptr_audio_stream->active = 0;
	ptr_audio_stream->period = 0;
	ptr_audio_stream->periods = 0;
	ptr_audio_stream->old_offset = 0;
	ptr_audio_stream->substream = NULL;
	if (!(--active_user)) {
		/* Disable the MSP1 */
		error = u8500_acodec_unsetuser(USER_ALSA);
		u8500_acodec_close(ACODEC_DISABLE_ALL);
	}
	else {
		if(stream_id == SNDRV_PCM_STREAM_PLAYBACK)
			u8500_acodec_close(ACODEC_DISABLE_TRANSMIT);
		else if(stream_id == SNDRV_PCM_STREAM_CAPTURE)
			u8500_acodec_close(ACODEC_DISABLE_RECEIVE);
	}

	stm_hw_free(substream);

	return error;
}
}

static int configure_direct_rendering(struct snd_pcm_substream *substream)
{
	int error = 0, stream_id;
	int status = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	u8500_acodec_chip_t *chip = snd_pcm_substream_chip(substream);
	audio_stream_t *ptr_audio_stream = NULL;

	stream_id = substream->pstr->stream;
	stream_id = substream->pstr->stream;

	if (stream_id == SNDRV_PCM_STREAM_PLAYBACK)
	{
		runtime->hw = snd_u8500_playback_hw;
	}
	else
	{
		runtime->hw = snd_u8500_capture_hw;
	}

	stream_id = substream->pstr->stream;
	status = u8500_acodec_open(stream_id);
	if(status)
	{
		printk("failed in getting open\n");
		return -1;
	}
	if (!active_user)
		error = u8500_acodec_setuser(USER_ALSA);
	if (error)
		return error;
	else
		active_user++;

	if ((error = configure_rate(substream,ACODEC_CONFIG_NOT_REQUIRED)))
			return error;

	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x04))); //MSP_GCR

	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x08))); //MSP
	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x0C))); //MSP
	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x10))); //MSP

	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x30))); //MSP
	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x34))); //MSP
	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x38))); //MSP

	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x40))); //MSP
	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x44))); //MSP
	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x48))); //MSP
	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x4C))); //MSP

	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x60))); //MSP
	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x64))); //MSP
	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x68))); //MSP
	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x6C))); //MSP


	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x18))); //MSP
	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x20))); //MSP
	writel(0x0,((char *)(IO_ADDRESS(U8500_MSP1_BASE) + 0x2C))); //MSP


	status = stm_gpio_altfuncenable(GPIO_ALT_MSP_1);
	if (status)
	{
		printk("Error in stm_gpio_altfuncenable, status is %d\n",status);
	}

	printk("\n stm_gpio_altfuncenable for GPIO_ALT_MSP_1\n");

#if DRIVER_DEBUG > 0
	{
		dump_msp_registers();
		dump_acodec_registers();
	}
#endif

	ptr_audio_stream = &chip->stream[ALSA_PCM_DEV][stream_id];

	ptr_audio_stream->substream = substream;

	FUNC_EXIT();
	return 0;
}

/**
 * snd_u8500_alsa_pcm_open
 * @substream - pointer to the playback/capture substream structure
 *
 *  This routine is used by alsa framework to open a pcm stream .
 *  Here a dma pipe is requested and device is configured(default).
 */
static int snd_u8500_alsa_pcm_open(struct snd_pcm_substream *substream)
{
	int error = 0, stream_id, status = 0;
	u8500_acodec_chip_t *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	audio_stream_t *ptr_audio_stream = NULL;

	FUNC_ENTER();

	if(ENABLE == chip->direct_rendering_mode)
	{
		configure_direct_rendering(substream);
		return 0;
	}
	else
	{
	stream_id = substream->pstr->stream;
	status = u8500_acodec_open(stream_id);

	if(status)
	{
		printk("failed in getting open\n");
		return -1;
	}

	if (!active_user)
		error = u8500_acodec_setuser(USER_ALSA);
	if (error)
		return error;
	else
		active_user++;

	error =
	    snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
				       &constraints_rate);
	if (error < 0) {
		stm_error
		    (": error initializing hw sample rate constraint\n");
		return error;
	}

	/* configure the default sampling rate for the acodec */
	second_config = 0;

	if ((error = configure_rate(substream,ACODEC_CONFIG_REQUIRED)))
		return error;

	/* Set the hardware configuration */
	stream_id = substream->pstr->stream;
	if (stream_id == SNDRV_PCM_STREAM_PLAYBACK) {
		runtime->hw = snd_u8500_playback_hw;
		/* configure the output sink for the acodec */
		if ((error =
		     u8500_acodec_select_output(chip->output_device,
						  USER_ALSA))) {
			stm_error(" : select output failed\n");
			return error;
		}

		/* configure the volume settings for the acodec */
		if ((error = u8500_acodec_set_output_volume(chip->output_device,
											chip->output_lvolume,
											chip->output_rvolume,
											USER_ALSA))) {
			stm_error(" : set output volume failed\n");
			return error;
		}
	} else {
		runtime->hw = snd_u8500_capture_hw;
		/* configure the input source for the acodec */
		if ((error =
		     u8500_acodec_select_input(chip->input_device,
						 USER_ALSA))) {
			stm_error(" : select input failed\n");
			return error;
		}
		/*u8500_acodec_set_src_power_cntrl(AB8500_CODEC_SRC_D_MICROPHONE_1,ENABLE);
		u8500_acodec_set_src_power_cntrl(AB8500_CODEC_SRC_D_MICROPHONE_2,ENABLE);

		u8500_acodec_set_input_volume(AB8500_CODEC_SRC_D_MICROPHONE_1,
											chip->input_lvolume,
											chip->input_rvolume,
											USER_ALSA);

		u8500_acodec_set_input_volume(AB8500_CODEC_SRC_D_MICROPHONE_2,
											chip->input_lvolume,
											chip->input_rvolume,
											USER_ALSA);
											*/

		if ((error = u8500_acodec_set_input_volume(chip->input_device,
											chip->input_lvolume,
											chip->input_rvolume,
											USER_ALSA))) {
			stm_error(" : set input volume failed\n");
			return error;
		}
	}

	u8500_acodec_set_burst_mode_fifo(chip->burst_fifo_mode);

#if DRIVER_DEBUG > 0
	{
		dump_msp_registers();
		dump_acodec_registers();
	}
#endif

	ptr_audio_stream = &chip->stream[ALSA_PCM_DEV][stream_id];

	ptr_audio_stream->substream = substream;

	if(DISABLE == chip->direct_rendering_mode)
	{
		stm_config_hw(chip, substream,ALSA_PCM_DEV,stream_id);
	}
	init_MUTEX(&(ptr_audio_stream->alsa_sem));
	init_completion(&(ptr_audio_stream->alsa_com));
	ptr_audio_stream->state = ALSA_STATE_UNPAUSE;
	stm_dbg(DBG_ST.alsa," pipe: %i open:\n", (int)ptr_audio_stream->pipeId);

	FUNC_EXIT();
	return 0;
}
}

/**
 * snd_u8500_alsa_pcm_hw_params
 * @substream - pointer to the playback/capture substream structure
 * @hw_params - specifies the hw parameters like format/no of channels etc
 *
 *  This routine is used by alsa framework to allocate a dma buffer
 *  used to transfer the data from user space to kernel space
 *
 */
static int snd_u8500_alsa_pcm_hw_params(struct snd_pcm_substream *substream,
					  struct snd_pcm_hw_params *hw_params)
{
	return devdma_hw_alloc(NULL, substream, params_buffer_bytes(hw_params));
}

/**
 * snd_u8500_alsa_pcm_hw_free
 * @substream - pointer to the playback/capture substream structure
 *
 *  This routine is used by alsa framework to deallocate a dma buffer
 *  allocated berfore by snd_u8500_alsa_pcm_hw_params
 */
static int snd_u8500_alsa_pcm_hw_free(struct snd_pcm_substream *substream)
{
	/* Disable the MSP1 */
	stm_hw_free(substream);
	return 0;
}

/**
 * snd_u8500_alsa_pcm_prepare
 * @substream - pointer to the playback/capture substream structure
 *
 *  This callback is called whene the pcm is "prepared" Here is possible
 *  to set the format type ,sample rate ,etc.The callback is called as
 *  well everytime a recovery after an underrun happens.
 */

static int snd_u8500_alsa_pcm_prepare(struct snd_pcm_substream *substream)
{
	u8500_acodec_chip_t *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int error, stream_id;

	FUNC_ENTER();

	if (chip->freq != runtime->rate || chip->channels != runtime->channels)
	{
		stm_dbg(DBG_ST.alsa," freq not same, %d %d\n", chip->freq, runtime->rate);
		stm_dbg(DBG_ST.alsa," channels not same, %d %d\n", chip->channels,runtime->channels);
		if (chip->channels != runtime->channels)
		{
			chip->channels = runtime->channels;
			if((error = stm_config_hw(chip, substream,ALSA_PCM_DEV, -1)))
			{
				stm_dbg(DBG_ST.alsa,"In func %s, stm_config_hw fails",__FUNCTION__);
				return error;
			}
		}
		chip->freq = runtime->rate;
		second_config = 1;

                if(stream_id == SNDRV_PCM_STREAM_PLAYBACK)
			u8500_acodec_close(ACODEC_DISABLE_TRANSMIT);
		else if(stream_id == SNDRV_PCM_STREAM_CAPTURE)
			u8500_acodec_close(ACODEC_DISABLE_RECEIVE);

		if ((error = configure_rate(substream,ACODEC_CONFIG_NOT_REQUIRED)))
		{
			stm_dbg(DBG_ST.alsa,"In func %s, configure_rate fails",__FUNCTION__);
			return error;
		}
	}

	FUNC_EXIT();
	return 0;
}

/**
 * snd_u8500_alsa_pcm_trigger
 * @substream -  pointer to the playback/capture substream structure
 * @cmd - specifies the command : start/stop/pause/resume
 *
 *  This callback is called whene the pcm is started ,stopped or paused
 *  The action is specified in the second argument, SND_PCM_TRIGGER_XXX in
 *  <sound/pcm.h>.
 *  This callback is atomic and the interrupts are disabled , so you can't
 *  call other functions that need interrupts without possible risks
 */
static int snd_u8500_alsa_pcm_trigger(struct snd_pcm_substream *substream,
					int cmd)
{
	int stream_id = substream->pstr->stream;
	audio_stream_t *stream = NULL;
	u8500_acodec_chip_t *chip = snd_pcm_substream_chip(substream);
	int error = 0;

	FUNC_ENTER();

	stream = &chip->stream[ALSA_PCM_DEV][stream_id];

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		/* Start the pcm engine */
		stm_dbg(DBG_ST.alsa," TRIGGER START\n");
		if (stream->active == 0) {
			stream->active = 1;
			stm_trigger_alsa(stream);
			break;
		}
		stm_error(": H/w is busy\n");
		return -EINVAL;

	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		stm_dbg(DBG_ST.alsa," SNDRV_PCM_TRIGGER_PAUSE_PUSH\n");
		if (stream->active == 1) {
			stm_pause_alsa(stream);
		}
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		stm_dbg(DBG_ST.alsa," SNDRV_PCM_TRIGGER_PAUSE_RELEASE\n");
		if (stream->active == 1)
			stm_unpause_alsa(stream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		/* Stop the pcm engine */
		stm_dbg(DBG_ST.alsa," TRIGGER STOP\n");
		if (stream->active == 1)
			stm_stop_alsa(stream);
		break;
	default:
		stm_error(": invalid command in pcm trigger\n");
		return -EINVAL;
	}

	FUNC_EXIT();
	return error;
}

/**
 * snd_u8500_alsa_pcm_pointer
 * @substream - pointer to the playback/capture substream structure
 *
 *  This callback is called whene the pcm middle layer inquires the current
 *  hardware position on the buffer .The position is returned in frames
 *  ranged from 0 to buffer_size -1
 */
static snd_pcm_uframes_t snd_u8500_alsa_pcm_pointer(struct snd_pcm_substream
						      *substream)
{
	unsigned int offset;
	u8500_acodec_chip_t *chip = snd_pcm_substream_chip(substream);
	audio_stream_t *stream = &chip->stream[ALSA_PCM_DEV][substream->pstr->stream];
	struct snd_pcm_runtime *runtime = stream->substream->runtime;



	offset = bytes_to_frames(runtime, stream->old_offset);
	if (offset < 0 || stream->old_offset < 0)
		stm_dbg(DBG_ST.alsa," Offset=%i %i\n", offset, stream->old_offset);

	return offset;
}

static int snd_u8500_alsa_pcm_mmap(struct snd_pcm_substream *substream,
				     struct vm_area_struct *vma)
{
	return devdma_mmap(NULL, substream, vma);
}

static struct snd_pcm_ops snd_u8500_alsa_playback_ops = {
	.open = snd_u8500_alsa_pcm_open,
	.close = snd_u8500_alsa_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = snd_u8500_alsa_pcm_hw_params,
	.hw_free = snd_u8500_alsa_pcm_hw_free,
	.prepare = snd_u8500_alsa_pcm_prepare,
	.trigger = snd_u8500_alsa_pcm_trigger,
	.pointer = snd_u8500_alsa_pcm_pointer,
	.mmap = snd_u8500_alsa_pcm_mmap,
};

static struct snd_pcm_ops snd_u8500_alsa_capture_ops = {
	.open = snd_u8500_alsa_pcm_open,
	.close = snd_u8500_alsa_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = snd_u8500_alsa_pcm_hw_params,
	.hw_free = snd_u8500_alsa_pcm_hw_free,
	.prepare = snd_u8500_alsa_pcm_prepare,
	.trigger = snd_u8500_alsa_pcm_trigger,
	.pointer = snd_u8500_alsa_pcm_pointer,
	.mmap = snd_u8500_alsa_pcm_mmap,
};


/**
* u8500_alsa_pio_start
* @stream - pointer to the playback/capture audio_stream_t structure
*
*  This function sends/receive one chunck of stream data to/from MSP
*/
static void u8500_alsa_pio_start(audio_stream_t * stream)
{
	unsigned int offset, dma_size, stream_id;
	int ret_val;
	struct snd_pcm_substream *substream = stream->substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	stream_id = substream->pstr->stream;

	FUNC_ENTER();
	dma_size = frames_to_bytes(runtime, runtime->period_size);
	offset = dma_size * stream->period;
	stream->old_offset = offset;

	stm_dbg(DBG_ST.alsa," Transfer started\n");
	stm_dbg(DBG_ST.alsa," address = %x  size=%d\n",
		 (runtime->dma_addr + offset), dma_size);

	/* Send our stuff */
	if (stream_id == SNDRV_PCM_STREAM_PLAYBACK)
#ifdef CONFIG_U8500_ACODEC_DMA
		u8500_acodec_send_data((void*)(runtime->dma_addr + offset),
				      dma_size,1);
#else
		u8500_acodec_send_data((void*)(runtime->dma_area + offset),
				      dma_size,0);
#endif
	else
#ifdef CONFIG_U8500_ACODEC_DMA
		u8500_acodec_receive_data((void *)(runtime->dma_addr + offset), dma_size, 1);
#else
		u8500_acodec_receive_data((void *)(runtime->dma_area + offset), dma_size, 0);
#endif

	stream->period++;
	stream->period %= runtime->periods;
	stream->periods++;

	FUNC_EXIT();

}

/**
* acodec_feeding_thread
* @data - void pointer to the playback/capture audio_stream_t structure
*
*	This thread sends/receive data to MSP while stream is active
*/
static int acodec_feeding_thread(void *data)
{
	audio_stream_t *stream = (audio_stream_t *) data;

	FUNC_ENTER();
	daemonize("acodec_feeding_thread");
	allow_signal(SIGKILL);
	down(&stream->alsa_sem);

	while ((!signal_pending(current)) && (stream->active))
	{
		if (stream->state == ALSA_STATE_PAUSE)
			wait_for_completion(&(stream->alsa_com));

		u8500_alsa_pio_start(stream);
		if (stream->substream)
			snd_pcm_period_elapsed(stream->substream);
	}

	up(&stream->alsa_sem);

	FUNC_EXIT();
	return 0;
}

/**
* acodec_feeding_thread
* @stream - pointer to the playback/capture audio_stream_t structure
*
*	This function creates a kernel thread .
*/

static int spawn_acodec_feeding_thread(audio_stream_t * stream)
{
	pid_t pid;

	FUNC_ENTER();

	pid = kernel_thread(acodec_feeding_thread, stream, CLONE_FS | CLONE_SIGHAND);

	FUNC_EXIT();
	return 0;
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
static irqreturn_t dma_eot_handler(void *data, int irq)
{
	audio_stream_t *stream = data;

	/* snd_pcm_period_elapsed() is _not_ to be protected
	 */
	stm_dbg(DBG_ST.alsa,"One transfer complete.. going to start the next one\n");

	if (stream->substream)
		snd_pcm_period_elapsed(stream->substream);
	if(stream->state == ALSA_STATE_PAUSE)
		return IRQ_HANDLED;
	if (stream->active == 1) {
		u8500_alsa_dma_start(stream);
	}
	return IRQ_HANDLED;
}

/**
 * u8500_alsa_dma_start - used to transmit or recive a dma chunk
 * @stream - specifies the playback/record stream structure
 */
static void u8500_alsa_dma_start(audio_stream_t * stream)
{
	unsigned int offset, dma_size, stream_id;

	struct snd_pcm_substream *substream = stream->substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	u8500_acodec_chip_t *u8500_chip = NULL;
	stream_id = substream->pstr->stream;
	u8500_chip = snd_pcm_substream_chip(substream);

	dma_size = frames_to_bytes(runtime, runtime->period_size);
	offset = dma_size * stream->period;
	stream->old_offset = offset;

	if (stream_id == SNDRV_PCM_STREAM_PLAYBACK)
	{
		#ifdef CONFIG_U8500_ACODEC_DMA
			u8500_acodec_send_data((void*)(runtime->dma_addr + offset),dma_size,1);
		#else
			u8500_acodec_send_data((void*)(runtime->dma_area + offset),dma_size,0);
		#endif
	}
	else
	{
		#ifdef CONFIG_U8500_ACODEC_DMA
			u8500_acodec_receive_data((void *)(runtime->dma_addr + offset), dma_size, 1);
		#else
			u8500_acodec_receive_data((void *)(runtime->dma_area + offset), dma_size, 0);
		#endif
	}

	stm_dbg(DBG_ST.alsa," DMA Tx : add = %x  size=%d\n",(runtime->dma_addr + offset), dma_size);

	stream->period++;
	stream->period %= runtime->periods;
	stream->periods++;
}

/**
* u8500_audio_init
* @chip - pointer to u8500_acodec_chip_t structure.
*
* This function intialises the u8500 chip structure with default values
*/
static void u8500_audio_init(u8500_acodec_chip_t * chip)
{
	audio_stream_t *ptr_audio_stream = NULL;

	ptr_audio_stream = &chip->stream[ALSA_PCM_DEV][SNDRV_PCM_STREAM_PLAYBACK];
	/* Setup DMA stuff */
	ptr_audio_stream->id = "u8500 playback";
	ptr_audio_stream->stream_id = SNDRV_PCM_STREAM_PLAYBACK;

	/* default initialization  for playback */
	ptr_audio_stream->pipeId = -1;
	ptr_audio_stream->active = 0;
	ptr_audio_stream->period = 0;
	ptr_audio_stream->periods = 0;
	ptr_audio_stream->old_offset = 0;

	ptr_audio_stream = &chip->stream[ALSA_PCM_DEV][SNDRV_PCM_STREAM_CAPTURE];

	ptr_audio_stream->id = "u8500 capture";
	ptr_audio_stream->stream_id = SNDRV_PCM_STREAM_CAPTURE;

	/* default initialization  for capture */
	ptr_audio_stream->pipeId = -1;
	ptr_audio_stream->active = 0;
	ptr_audio_stream->period = 0;
	ptr_audio_stream->periods = 0;
	ptr_audio_stream->old_offset = 0;

	chip->freq = DEFAULT_SAMPLE_RATE;
	chip->channels = 1;
	chip->input_lvolume = DEFAULT_GAIN;
	chip->input_rvolume = DEFAULT_GAIN;
	chip->output_lvolume = DEFAULT_VOLUME;
	chip->output_rvolume = DEFAULT_VOLUME;
	chip->output_device = DEFAULT_OUTPUT_DEVICE;
	chip->input_device = DEFAULT_INPUT_DEVICE;
	chip->analog_lpbk = DEFAULT_LOOPBACK_STATE;
	chip->digital_lpbk = DEFAULT_LOOPBACK_STATE;
	chip->playback_switch = DEFAULT_SWITCH_STATE;
	chip->capture_switch = DEFAULT_SWITCH_STATE;
	chip->tdm8_ch_mode = DEFAULT_TDM8_CH_MODE_STATE;
	chip->direct_rendering_mode = DEFAULT_DIRECT_RENDERING_STATE;
	chip->burst_fifo_mode = DEFAULT_BURST_FIFO_STATE;
	chip->fm_playback_mode = DEFAULT_FM_PLAYBACK_STATE;
	chip->fm_tx_mode = DEFAULT_FM_TX_STATE;

}

/**
 * snd_card_u8500_alsa_pcm_new - constructor for a new pcm cmponent
 * @chip - pointer to chip specific data
 * @device - specifies the card number
 */
static int snd_card_u8500_alsa_pcm_new(u8500_acodec_chip_t * chip,
					 int device)
{
	struct snd_pcm *pcm;
	int err;

	if ((err = snd_pcm_new(chip->card, "u8500", device, 1, 1, &pcm)) < 0) {
		stm_error(": error in snd_pcm_new\n");
		return err;
	}

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
			&snd_u8500_alsa_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,
			&snd_u8500_alsa_capture_ops);

	pcm->private_data = chip;
	pcm->info_flags = 0;
	chip->pcm = pcm;
	strcpy(pcm->name, "u8500_alsa");

	u8500_audio_init(pcm->private_data);
	return 0;
}

static int __init u8500_alsa_probe(struct platform_device *devptr)
{
	int error;
	struct snd_card *card;
	u8500_acodec_chip_t *u8500_chip;

	/*Set currently active users to 0 */
	active_user = 0;

	/* Register card 1 as linux_audio_hwctrl is not able to open card0 in Android while alsa utils are able to see it ??? */
	card = snd_card_new(1, NULL, THIS_MODULE, sizeof(u8500_acodec_chip_t));
	if (card == NULL) {
		stm_error(": error in snd_card_new\n");
		return -ENOMEM;
	}

	u8500_chip = (u8500_acodec_chip_t *) card->private_data;
	u8500_chip->card = card;

	if ((error = snd_card_u8500_alsa_pcm_new(u8500_chip, 0)) < 0) {
		stm_error(": pcm interface can't be initialized\n\n");
		goto nodev;
	}

	if ((error =
	     snd_ctl_add(card,
			 snd_ctl_new1(&u8500_playback_vol_ctrl,
				      u8500_chip))) < 0) {
		stm_error(": error initializing u8500_playback_vol_ctrl interface \n\n");
		goto nodev;
	}

	if ((error =
	     snd_ctl_add(card,
			 snd_ctl_new1(&u8500_capture_vol_ctrl,
				      u8500_chip))) < 0) {
		stm_error(": error initializing u8500_capture_vol_ctrl interface \n\n");
		goto nodev;
	}

	if ((error =
	     snd_ctl_add(card,
			 snd_ctl_new1(&u8500_playback_sink_ctrl,
				      u8500_chip))) < 0) {
		stm_error(": error initializing playback ctrl interface \n\n");
		goto nodev;
	}

	if ((error =
	     snd_ctl_add(card,
			 snd_ctl_new1(&u8500_capture_src_ctrl,
				      u8500_chip))) < 0) {
		stm_error(": error initializing u8500_playback_sink_ctrl interface \n\n");
		goto nodev;
	}

	if ((error =
	     snd_ctl_add(card,
			 snd_ctl_new1(&u8500_analog_lpbk_ctrl,
				      u8500_chip))) < 0) {
		stm_error(": error initializing u8500_analog_lpbk_ctrl interface \n\n");
		goto nodev;
	}

	if ((error =
	     snd_ctl_add(card,
			 snd_ctl_new1(&u8500_digital_lpbk_ctrl,
				      u8500_chip))) < 0) {
		stm_error(": error initializing u8500_digital_lpbk_ctrl interface \n\n");
		goto nodev;
	}


	if ((error =
	     snd_ctl_add(card,
			 snd_ctl_new1(&u8500_playback_switch_ctrl,
				      u8500_chip))) < 0) {
		stm_error(": error initializing u8500_playback_switch_ctrl interface \n\n");
		goto nodev;
	}

	if ((error =
	     snd_ctl_add(card,
			 snd_ctl_new1(&u8500_capture_switch_ctrl,
				      u8500_chip))) < 0) {
		stm_error(": error initializing u8500_capture_switch_ctrl interface \n\n");
		goto nodev;
	}

	if ((error =
	     snd_ctl_add(card,
			 snd_ctl_new1(&u8500_playback_power_ctrl,
				      u8500_chip))) < 0) {
		stm_error(": error initializing u8500_playback_power_ctrl interface \n\n");
		goto nodev;
	}

	if ((error =
	     snd_ctl_add(card,
			 snd_ctl_new1(&u8500_capture_power_ctrl,
				      u8500_chip))) < 0) {
		stm_error(": error initializing u8500_capture_power_ctrl interface \n\n");
		goto nodev;
	}

	if ((error =
	     snd_ctl_add(card,
			 snd_ctl_new1(&u8500_tdm_mode_ctrl,
				      u8500_chip))) < 0) {
		stm_error(": error initializing u8500_tdm_mode_ctrl interface \n\n");
		goto nodev;
	}

	if ((error =
	     snd_ctl_add(card,
			 snd_ctl_new1(&u8500_direct_rendering_mode_ctrl,
				      u8500_chip))) < 0) {
		stm_error(": error initializing u8500_direct_rendering_mode_ctrl interface \n\n");
		goto nodev;
	}

	if ((error =
	     snd_ctl_add(card,
			 snd_ctl_new1(&u8500_pcm_rendering_mode_ctrl,
				      u8500_chip))) < 0) {
		stm_error(": error initializing u8500_pcm_rendering_mode_ctrl interface \n\n");
		goto nodev;
	}


	strcpy(card->driver, "u8500_alsa");
	strcpy(card->shortname, "u8500_alsa driver");
	strcpy(card->longname, "u8500 alsa driver");

	snd_card_set_dev(card, &devptr->dev);

	if ((error = snd_card_register(card)) == 0) {
		stm_info("u8500 audio support running..\n");
		platform_set_drvdata(devptr, card);
		return 0;
	}

      nodev:
	snd_card_free(card);
	return error;
}

static int __devexit u8500_alsa_remove(struct platform_device *devptr)
{
	snd_card_free(platform_get_drvdata(devptr));
	platform_set_drvdata(devptr, NULL);
	stm_info("u8500 audio support stopped\n");

	/*Set currently active users to 0 */
	active_user = 0;

	return 0;
}

static struct platform_driver u8500_alsa_driver = {
	.probe = u8500_alsa_probe,
	.remove = __devexit_p(u8500_alsa_remove),
	.driver = {
		   .name = U8500_ALSA_DRIVER,
		   },
};

/**
* u8500_alsa_init - Entry function of AB8500 alsa driver
*
* This function registers u8500 alsa driver with linux framework
*/
static int __init u8500_alsa_init(void)
{
	int err;

	if ((err = platform_driver_register(&u8500_alsa_driver)) < 0)
		return err;
	device =
	    platform_device_register_simple(U8500_ALSA_DRIVER, -1, NULL, 0);
	if (IS_ERR(device)) {
		platform_driver_unregister(&u8500_alsa_driver);
		return PTR_ERR(device);
	}
	//DBG_ST.acodec = 1;
	//DBG_ST.alsa = 1;

	return 0;
}

static void __exit u8500_alsa_exit(void)
{
	platform_device_unregister(device);
	platform_driver_unregister(&u8500_alsa_driver);
}

module_init(u8500_alsa_init);
module_exit(u8500_alsa_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AB8500 ALSA driver");
