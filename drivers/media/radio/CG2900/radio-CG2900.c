/*
 * file radio-CG2900.c
 *
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Linux Wrapper for V4l2 FM Driver for CG2900.
 *
 * Author: Hemant Gupta/hemant.gupta@stericsson.com for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2
 */

#include "stefmapi.h"
#include "platformosapi.h"
#include "fmdriver.h"
#include <string.h>
#include "videodev.h"
#include "v4l2-ioctl.h"
#include "wait.h"
#include <linux/init.h>		/* Initdata */

#define RADIO_CG2900_VERSION KERNEL_VERSION(0, 1, 1)
#define BANNER "ST-Ericsson FM Radio Card driver v0.1.1"

#define FMR_SEEK_NONE 					0
#define FMR_SEEK_IN_PROGRESS 				1

#define FMR_SWITCH_OFF					0
#define FMR_SWITCH_ON					1
#define FMR_STANDBY					2

#define STE_HRTZ_MULTIPLIER			1000000

#define SEARCH_TIME_PER_CH_GRID	120
#define MAX_SCAN_SEEK_TIME (SEARCH_TIME_PER_CH_GRID * 408)

/* freq in Hz to V4l2 freq (units of 62.5Hz) */
#define HZ_2_V4L2(X)  (2*(X)/125)
/* V4l2 freq (units of 62.5Hz) to freq in Hz */
#define V4L2_2_HZ(X)  (((X)*125)/(2))

static u32 freq_low;
static u32 freq_high;

/**************************************************************************
* Module parameters
**************************************************************************/
static int radio_nr;

/* Grid (kHz) */
/* 0: 200 kHz (USA) */
/* 1: 100 kHz (Europe, Japan) */
/* 2:  50 kHz (China) */
static int grid;

/* Bottom of Band (MHz) */
/* 0: 87.5 - 108 MHz (USA, Europe)*/
/* 1: 70   - 108 MHz (China wide band) */
/* 2: 76   -  90 MHz (Japan) */
static int band;

static uint32_t search_freq ;
static uint16_t no_of_scanfreq ;
static uint32_t scanfreq_rssi_level[32];
static uint32_t scanfreq[32];
static int users;

/* radio_cg2900_poll_queue - Main Wait Queue for polling (Scan/seek) */
static DECLARE_WAIT_QUEUE_HEAD(radio_cg2900_poll_queue);

/* ------------------ Internal function declarations ---------------------- */
static int radio_cg2900_open(
		struct file *file
		);
static int radio_cg2900_release(
		struct file *file
		);
static ssize_t  radio_cg2900_read(
		struct file *file,
		char __user *data,
		size_t count,
		loff_t *pos
		);
static unsigned int radio_cg2900_poll(
		struct file *file,
		struct poll_table_struct *wait
		);
static int vidioc_querycap(
		struct file *file,
		void *priv,
		struct v4l2_capability *arg
		);
static int vidioc_g_tuner(
		struct file *file,
		void *priv,
		struct v4l2_tuner *arg
		);
static int vidioc_s_tuner(
		struct file *file,
		void *priv,
		struct v4l2_tuner *arg
		);
static int vidioc_g_frequency(
		struct file *file,
		void *priv,
		struct v4l2_frequency *arg
		);
static int vidioc_s_frequency(
		struct file *file,
		void *priv,
		struct v4l2_frequency *arg
		);
static int vidioc_queryctrl(
		struct file *file,
		void *priv,
		struct v4l2_queryctrl *arg
		);
static int vidioc_g_ctrl(
		struct file *file,
		void *priv,
		struct v4l2_control *arg
		);
static int vidioc_s_ctrl(
		struct file *file,
		void *priv,
		struct v4l2_control *arg
		);
static int vidioc_g_ext_ctrls(
		struct file *file,
		void *priv,
		struct v4l2_ext_controls *arg
		);
static int vidioc_s_hw_freq_seek(
		struct file *file,
		void *priv,
		struct v4l2_hw_freq_seek *arg
		);
static int vidioc_g_audio(
		struct file *file,
		void *priv,
		struct v4l2_audio *arg
		);
static int vidioc_g_input(
		struct file *filp,
		void *priv,
		unsigned int *i);
static int vidioc_s_input(
		struct file *filp,
		void *priv,
		unsigned int i);
static int vidioc_s_audio(
		struct file *file,
		void *priv,
		struct v4l2_audio *a
		);
static void radio_cg2900_convert_err_to_v4l2(
		char status_byte,
		char *out_byte
		);
static int __init radio_cg2900_init(void);
static void __exit radio_cg2900_deinit(void);

/**
  * struct radio_cg2900_device - Stores FM Device Info.
  * @v_state: state of FM Radio
  * @v_volume: Analog Volume Gain of FM Radio
  * @v_frequency: Frequency tuned on FM Radio in V4L2 Format
  * @v_signalstrength: RSSI Level of the currently tuned frequency.
  * @v_muted: FM Radio Mute/Unmute status
  * @v_seekstatus: seek status
  * @v_audiopath: Audio Balance
  * @v_rds_enabled: Rds eable/disable status
  * @v_rssi_threshold: rssi Thresold set on FM Radio
  *
  */
struct radio_cg2900_device_s {
	u8		v_state;
	int           	v_volume;
	u32           	v_frequency;
	u32           	v_signalstrength;
	u8            	v_muted;
	u8            	v_seekstatus;
	u32           	v_audiopath;
	u8            	v_rds_enabled;
	u16          	v_rssi_threshold;
} radio_cg2900_device;

/* Global Structure Pointer to store the maintain FM Driver device info */
struct radio_cg2900_device_s *g_radio_cg2900_device = &radio_cg2900_device;

/* Time structure for Max time for scanning and seeking */
static struct timeval max_scan_seek_time = {
	.tv_sec = MAX_SCAN_SEEK_TIME,
	.tv_usec = 0
};

/* V4l2 File Operation Structure */
static const struct v4l2_file_operations radio_cg2900_fops = {
	.owner			= THIS_MODULE,
	.open           	= radio_cg2900_open,
	.release        	= radio_cg2900_release,
	.read        		= radio_cg2900_read,
	.poll        		= radio_cg2900_poll,
	.ioctl          	= video_ioctl2,
};

/* V4L2 IOCTL Operation Structure */
static const struct v4l2_ioctl_ops radio_cg2900_ioctl_ops = {
	.vidioc_querycap    	= vidioc_querycap,
	.vidioc_g_tuner     	= vidioc_g_tuner,
	.vidioc_s_tuner     	= vidioc_s_tuner,
	.vidioc_g_frequency 	= vidioc_g_frequency,
	.vidioc_s_frequency 	= vidioc_s_frequency,
	.vidioc_queryctrl   	= vidioc_queryctrl,
	.vidioc_g_ctrl      	= vidioc_g_ctrl,
	.vidioc_s_ctrl      	= vidioc_s_ctrl,
	.vidioc_g_ext_ctrls 	= vidioc_g_ext_ctrls,
	.vidioc_s_hw_freq_seek 	= vidioc_s_hw_freq_seek,
	.vidioc_g_audio     	= vidioc_g_audio,
	.vidioc_s_audio     	= vidioc_s_audio,
	.vidioc_g_input     	= vidioc_g_input,
	.vidioc_s_input     	= vidioc_s_input,
};

/* V4L2 Video Device Structure */
static struct video_device radio_cg2900_video_device = {
	.name			= "STE CG2900 FM Rx/Tx Radio",
	.vfl_type		= VID_TYPE_TUNER | VID_TYPE_CAPTURE,
	.fops			= &radio_cg2900_fops,
	.ioctl_ops 		= &radio_cg2900_ioctl_ops,
	.release    	= video_device_release_empty,
};

/* ------------------ Internal Function definitions ---------------------- */

/**
 * vidioc_querycap()- Query FM Driver Capabilities.
 * This function is used to query the capabilities of the
 * FM Driver. This function is called when the application issues the IOCTL
 * VIDIOC_QUERYCAP
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @arg: Pointer to the v4l2_capability structure.
 *
 * Returns: 0
 */
static int vidioc_querycap(
		struct file *file,
		void *priv,
		struct v4l2_capability *arg
		)
{
	struct v4l2_capability *v = arg;
	FM_DEBUG_REPORT("vidioc_querycap");
	memset(v, 0, sizeof(*v));
	strlcpy(v->driver, "CG2900 Driver", sizeof(v->driver));
	strlcpy(v->card, "CG2900 FM Radio", sizeof(v->card));
	strcpy(v->bus_info, "platform"); /* max. 32 bytes! */
	v->version = RADIO_CG2900_VERSION;
	v->capabilities = V4L2_CAP_TUNER |
		V4L2_CAP_RADIO  |
		V4L2_CAP_READWRITE |
		V4L2_CAP_RDS_CAPTURE |
		V4L2_CAP_HW_FREQ_SEEK |
		V4L2_CAP_RDS_OUTPUT;
	memset(v->reserved, 0, sizeof(v->reserved));
	FM_DEBUG_REPORT("vidioc_querycap returning 0");
	return 0;
}

/**
 * vidioc_g_tuner()- Get FM Tuner Features.
 * This function is used to get the tuner features.
 * This function is called when the application issues the IOCTL
 * VIDIOC_G_TUNER
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @arg: Pointer to the v4l2_tuner structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_g_tuner(
		struct file *file,
		void *priv,
		struct v4l2_tuner *arg
		)
{
	struct v4l2_tuner *v = arg;
	uint8_t status;
	uint32_t mode;
	uint16_t rssi;
	FM_DEBUG_REPORT("vidioc_g_tuner");

	if (v->index > 0)
		return -EINVAL;

	strcpy(v->name, "CG2900 FM Receiver/Transmitter ");
	v->type = V4L2_TUNER_RADIO;
	v->rangelow = HZ_2_V4L2(freq_low);
	v->rangehigh = HZ_2_V4L2(freq_high);
	v->capability = \
		V4L2_TUNER_CAP_LOW  /* Frequency steps = 1/16 kHz */
		| V4L2_TUNER_CAP_STEREO /* Can receive stereo */
		| V4L2_TUNER_CAP_RDS; /* Supports RDS Capture/Transmission  */
	status =  ste_fm_get_mode(&mode);
	FM_DEBUG_REPORT("vidioc_g_tuner: mode = %d, ", mode);
	if (STE_STATUS_OK == status) {
		switch (mode) {
			/* stereo */
		case 0:
			v->audmode = V4L2_TUNER_MODE_STEREO;
			v->rxsubchans = V4L2_TUNER_SUB_STEREO |
				V4L2_TUNER_SUB_RDS;
			break;
			/* mono */
		case 1:
			v->audmode = V4L2_TUNER_SUB_MONO;
			v->rxsubchans = V4L2_TUNER_SUB_STEREO |
				V4L2_TUNER_SUB_RDS;
			break;
			/* Switching or Blending, set mode as Stereo */
		default:
			v->rxsubchans = V4L2_TUNER_SUB_MONO;
			v->audmode = V4L2_TUNER_MODE_STEREO |
				V4L2_TUNER_SUB_RDS ;
		}
	} else {
		/* Get mode API failed, set mode to mono */
		v->audmode = V4L2_TUNER_MODE_MONO;
		v->rxsubchans = V4L2_TUNER_SUB_MONO |
				V4L2_TUNER_SUB_RDS;
	}

	status =  ste_fm_get_signal_strength(&rssi);
	if (STE_STATUS_OK == status) {
		v->signal = rssi;
		FM_DEBUG_REPORT("vidioc_g_tuner: rssi level = %d, "\
				"signal = 0x%04x", rssi, v->signal);
	} else {
		v->signal = 0;
	}
	memset(v->reserved, 0, sizeof(v->reserved));
	return 0;
}

/**
 * vidioc_s_tuner()- Set FM Tuner Features.
 * This function is used to set the tuner features.
 * This function is called when the application issues the IOCTL
 * VIDIOC_S_TUNER
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @arg: Pointer to the v4l2_tuner structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_s_tuner(
		struct file *file,
		void *priv,
		struct v4l2_tuner *arg
		)
{
	const struct v4l2_tuner *v = arg;

	FM_DEBUG_REPORT("vidioc_s_tuner");
	if (v->index != 0) {
		/* Sorry, only 1 tuner */
		return -EINVAL;
	}

	return 0;
}

/**
 * vidioc_g_frequency()- Get the Current FM Frequnecy.
 * This function is used to get the currently tuned
 * frequency on FM Radio. This function is called when the application
 * issues the IOCTL VIDIOC_G_FREQUENCY
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @arg: Pointer to the v4l2_frequency structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_g_frequency(
		struct file *file,
		void *priv,
		struct v4l2_frequency *arg
		)
{
	struct v4l2_frequency *v4l2_freq = arg;
	uint8_t status;
	u32 freq;
	int ret_val = 0;

	FM_DEBUG_REPORT("vidioc_g_frequency: Status = %d",
			g_radio_cg2900_device->v_seekstatus);
	/* Check whether seek was in progress or not */
	os_lock();

	if (g_radio_cg2900_device->v_seekstatus == FMR_SEEK_IN_PROGRESS) {
		/*  Check if seek is finished or not */
		if (STE_EVENT_SEARCH_CHANNEL_FOUND == global_event) {
			/* seek is finished */
			g_radio_cg2900_device->v_frequency =
				HZ_2_V4L2(search_freq);
			v4l2_freq->frequency =
				g_radio_cg2900_device->v_frequency;
			/* FM_DEBUG_REPORT("VIDIOC_G_FREQUENCY: \
				STE_EVENT_SEARCH_CHANNEL_FOUND, \
				freq= %d SearchFreq = %d", \
				g_radio_cg2900_device->v_frequency, \
				search_freq);  */
			g_radio_cg2900_device->v_seekstatus = FMR_SEEK_NONE;
			global_event = STE_EVENT_NO_EVENT;
			ret_val = 0;
		} else if (STE_EVENT_NO_CHANNELS_FOUND == global_event) {
			/* seek is finished */
			g_radio_cg2900_device->v_seekstatus = FMR_SEEK_NONE;
			global_event = STE_EVENT_NO_EVENT;
			v4l2_freq->frequency =
				g_radio_cg2900_device->v_frequency;
			ret_val = 0;
		}
	} else {
		global_event = STE_EVENT_NO_EVENT;
		status = ste_fm_get_frequency((uint32_t *)&freq);
		if (STE_STATUS_OK == status) {
			g_radio_cg2900_device->v_frequency = HZ_2_V4L2(freq);
			v4l2_freq->frequency =
				g_radio_cg2900_device->v_frequency;
			ret_val = 0;
		} else {
			v4l2_freq->frequency =
				g_radio_cg2900_device->v_frequency;
			ret_val = -EINVAL;
		}
	}
	os_unlock();
	FM_DEBUG_REPORT("vidioc_g_frequency: Status = %d, returning",
			g_radio_cg2900_device->v_seekstatus);
	return ret_val;
}

/**
 * vidioc_s_frequency()- Set the FM Frequnecy.
 * This function is used to set the frequency
 * on FM Radio. This function is called when the application
 * issues the IOCTL VIDIOC_S_FREQUENCY
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @arg: Pointer to the v4l2_frequency structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_s_frequency(
		struct file *file,
		void *priv,
		struct v4l2_frequency *arg
		)
{
	const struct v4l2_frequency *v = arg;
	__u32 freq = v->frequency;
	uint8_t status;

	FM_DEBUG_REPORT("vidioc_s_frequency: Frequency = "\
			"%d ",  V4L2_2_HZ(freq));

	os_lock();
	global_event = STE_EVENT_NO_EVENT;
	search_freq = 0;
	no_of_scanfreq = 0;
	os_unlock();

	g_radio_cg2900_device->v_seekstatus =  FMR_SEEK_NONE;
	g_radio_cg2900_device->v_frequency = freq;
	status = ste_fm_set_frequency(V4L2_2_HZ(freq));
	if (STE_STATUS_OK == status)
		return 0;
	else
		return -EINVAL;
}

/**
 * vidioc_queryctrl()- Query the FM Driver control features.
 * This function is used to query the control features on FM Radio.
 * This function is called when the application
 * issues the IOCTL VIDIOC_QUERYCTRL
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @arg: Pointer to the v4l2_queryctrl structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_queryctrl(
		struct file *file,
		void *priv,
		struct v4l2_queryctrl *arg
		)
{
	struct v4l2_queryctrl *v = arg;
	FM_DEBUG_REPORT("vidioc_queryctrl");
	/* Check which control is requested */
	switch (v->id) {
	/* Audio Mute. This is a hardware function in the CG2900 radio */
	case V4L2_CID_AUDIO_MUTE:
		FM_DEBUG_REPORT("vidioc_queryctrl:  V4L2_CID_AUDIO_MUTE");
		v->type = V4L2_CTRL_TYPE_BOOLEAN;
		v->minimum = 0;
		v->maximum = 1;
		v->step = 1;
		v->default_value = 0;
		v->flags = 0;
		strncpy(v->name, "CG2900 Mute", 32); /* Max 32 bytes */
		break;

		/* Audio Volume. Not implemented in
		 * hardware. just a dummy function. */
	case V4L2_CID_AUDIO_VOLUME:
		FM_DEBUG_REPORT("vidioc_queryctrl: V4L2_CID_AUDIO_VOLUME");
		strncpy(v->name, "CG2900 Volume", 32); /* Max 32 bytes */
		v->minimum = 0x00;
		v->maximum = 0x15;
		v->step = 1;
		v->default_value = 0x15;
		v->flags = 0;
		v->type = V4L2_CTRL_TYPE_INTEGER;
		break;

	case V4L2_CID_AUDIO_BALANCE:
		FM_DEBUG_REPORT("vidioc_queryctrl:   V4L2_CID_AUDIO_BALANCE ");
		strncpy(v->name, "CG2900 Audio Balance", 32);
		v->type = V4L2_CTRL_TYPE_INTEGER;
		v->minimum = 0x0000;
		v->maximum = 0xFFFF;
		v->step = 0x0001;
		v->default_value = 0x0000;
		v->flags = 0;
		break;

		/* Explicitely list some CIDs to produce a
		 * verbose output in debug mode. */
	case V4L2_CID_AUDIO_BASS:
		FM_DEBUG_REPORT("vidioc_queryctrl: "\
				"V4L2_CID_AUDIO_BASS (unsupported)");
		return -EINVAL;

	case V4L2_CID_AUDIO_TREBLE:
		FM_DEBUG_REPORT("vidioc_queryctrl: "\
				"V4L2_CID_AUDIO_TREBLE (unsupported)");
		return -EINVAL;

	default:
		FM_DEBUG_REPORT("vidioc_queryctrl: "\
				"--> unsupported id = %d", (int)v->id);
		return -EINVAL;
	}

	return 0;
}

/**
 * vidioc_g_ctrl()- Get the value of a particular Control.
 * This function is used to get the value of a
 * particular control from the FM Driver. This function is called
 * when the application issues the IOCTL VIDIOC_G_CTRL
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @arg: Pointer to the v4l2_control structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_g_ctrl(
		struct file *file,
		void *priv,
		struct v4l2_control *arg
		)
{
	struct v4l2_control *v = arg;
	uint8_t status;
	uint8_t value;
	uint16_t rssi;

	FM_DEBUG_REPORT("vidioc_g_ctrl") ;
	if (v->id == V4L2_CID_AUDIO_VOLUME) {
		status = ste_fm_get_volume(&value);
		v->value = value;
		g_radio_cg2900_device->v_volume = value;
	} else if (v->id == V4L2_CID_AUDIO_MUTE) {
		v->value = g_radio_cg2900_device->v_muted;
	} else if (v->id == V4L2_CID_AUDIO_BALANCE) {
		v->value = g_radio_cg2900_device->v_audiopath;
	} else if (v->id == V4L2_CID_CG2900_RADIO_RSSI_THRESHOLD) {
		v->value = g_radio_cg2900_device->v_rssi_threshold;
	} else if (v->id == V4L2_CID_CG2900_RADIO_RSSI_LEVEL) {
		status = ste_fm_get_signal_strength(&rssi);
		FM_DEBUG_REPORT("vidioc_g_ctrl: rssi = %d", rssi);
		v->value = rssi;
	}
	return 0;
}

/**
 * vidioc_s_ctrl()- Set the value of a particular Control.
 * This function is used to set the value of a
 * particular control from the FM Driver. This function is called when the
 * application issues the IOCTL VIDIOC_S_CTRL
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @arg: Pointer to the v4l2_control structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_s_ctrl(
		struct file *file,
		void *priv,
		struct v4l2_control *arg
		)
{
	struct v4l2_control *v = arg;
	uint8_t status = STE_STATUS_OK;
	int ret_val;
	FM_DEBUG_REPORT("vidioc_s_ctrl");
	/* Check which control is requested */
	switch (v->id) {
	/* Audio Mute. This is a hardware function in the CG2900 radio */
	case V4L2_CID_AUDIO_MUTE:
		FM_DEBUG_REPORT("vidioc_s_ctrl: "\
				"V4L2_CID_AUDIO_MUTE, "\
				"value = %d", v->value);
		if (v->value > 1 && v->value < 0)
			return -ERANGE;
		if (v->value) {
			FM_DEBUG_REPORT("vidioc_s_ctrl: Ctrl_Id = "\
					"V4L2_CID_AUDIO_MUTE, "\
					"Muting the Radio");
			status = ste_fm_mute();
		} else {
			FM_DEBUG_REPORT("vidioc_s_ctrl: "\
					"Ctrl_Id = V4L2_CID_AUDIO_MUTE, "\
					"UnMuting the Radio");
			status = ste_fm_unmute();
		}
		if (STE_STATUS_OK == status) {
			g_radio_cg2900_device->v_muted = v->value;
			ret_val = 0;
		} else
			ret_val = -EINVAL;
		break;

	case V4L2_CID_AUDIO_VOLUME:
		FM_DEBUG_REPORT("vidioc_s_ctrl: "\
				"V4L2_CID_AUDIO_VOLUME, "\
				"value = %d", v->value);
		if (v->value > 20 && v->value < 0)
			return -ERANGE;
		status = ste_fm_set_volume(v->value);
		if (STE_STATUS_OK == status) {
			g_radio_cg2900_device->v_volume = v->value;
			ret_val = 0;
		} else
			ret_val = -EINVAL;
		break;

	case V4L2_CID_AUDIO_BALANCE:
		FM_DEBUG_REPORT("vidioc_s_ctrl: "\
				"V4L2_CID_AUDIO_BALANCE, "\
				"value = %d", v->value);
		status = ste_fm_set_audio_balance(v->value);
		if (STE_STATUS_OK == status) {
			g_radio_cg2900_device->v_audiopath = v->value;
			ret_val = 0;
		} else
			ret_val = -EINVAL;
		break;

	case V4L2_CID_CG2900_RADIO_CHIP_STATE:
		FM_DEBUG_REPORT("vidioc_s_ctrl: "\
			"V4L2_CID_radio_cg2900_RADIO_CHIP_STATE, "\
			"value = %d", v->value);
		if (V4L2_CG2900_RADIO_STANDBY == v->value) {
			status = ste_fm_standby();
			g_radio_cg2900_device->v_state = FMR_STANDBY;
		} else if (V4L2_CG2900_RADIO_POWERUP == v->value) {
			status = ste_fm_power_up_from_standby();
			g_radio_cg2900_device->v_state = FMR_SWITCH_ON;
		}
		if (STE_STATUS_OK == status)
			ret_val = 0;
		else
			ret_val = -EINVAL;
		break;

	case V4L2_CID_CG2900_RADIO_RDS_STATE:
		FM_DEBUG_REPORT("vidioc_s_ctrl: "\
			"V4L2_CID_radio_cg2900_RADIO_RDS_STATE, "\
			"value = %d", v->value);
		if (V4L2_CG2900_RADIO_RDS_ON == v->value) {
			status = ste_fm_rds_on();
			g_radio_cg2900_device->v_rds_enabled = STE_TRUE;
		} else if (V4L2_CG2900_RADIO_RDS_OFF == v->value) {
			status = ste_fm_rds_off();
			g_radio_cg2900_device->v_rds_enabled = STE_FALSE;
		}
		if (STE_STATUS_OK == status)
			ret_val = 0;
		else
			ret_val = -EINVAL;
		break;

	case V4L2_CID_CG2900_RADIO_SELECT_ANTENNA:
		FM_DEBUG_REPORT("vidioc_s_ctrl: "\
			"V4L2_CID_radio_cg2900_RADIO_SELECT_ANTENNA, "\
			"value = %d", v->value);
		status = ste_fm_select_antenna(v->value);
		if (STE_STATUS_OK == status)
			ret_val = 0;
		else
			ret_val = -EINVAL;
		break;

	case V4L2_CID_CG2900_RADIO_BANDSCAN:
		FM_DEBUG_REPORT("vidioc_s_ctrl: "\
			"V4L2_CID_radio_cg2900_RADIO_BANDSCAN, "\
			"value = %d", v->value);
		if (V4L2_CG2900_RADIO_BANDSCAN_START == v->value) {
			no_of_scanfreq = 0;
			status = ste_fm_start_band_scan();
			g_radio_cg2900_device->v_seekstatus =
				FMR_SEEK_IN_PROGRESS;
		} else if (V4L2_CG2900_RADIO_BANDSCAN_STOP == v->value) {
			status = ste_fm_stop_scan();
			/* Clear the flag, just in case there
			 * is timeout in scan, application can clear
			  * this flag also */
			g_radio_cg2900_device->v_seekstatus = FMR_SEEK_NONE;
		}
		if (STE_STATUS_OK == status)
			ret_val = 0;
		else
			ret_val = -EINVAL;
		break;

	case V4L2_CID_CG2900_RADIO_RSSI_THRESHOLD:
		FM_DEBUG_REPORT("vidioc_s_ctrl: "\
				"V4L2_CID_radio_cg2900_RADIO_RSSI_THRESHOLD "\
				"= %d", v->value);
		status = ste_fm_set_rssi_threshold(v->value);
		if (STE_STATUS_OK == status) {
			g_radio_cg2900_device->v_rssi_threshold = v->value;
			ret_val = 0;
		} else
			ret_val = -EINVAL;
		break;

	default:
		FM_DEBUG_REPORT("vidioc_s_ctrl: "\
				"unsupported (id = %d)", (int)v->id);
		return -EINVAL;
	}

	return ret_val;
}

/**
 * vidioc_g_ext_ctrls()- Get the values of a particular control.
 * This function is used to get the value of a
 * particular control from the FM Driver. This is used when the data to
 * be received is more than 1 paramter. This function is called when the
 * application issues the IOCTL VIDIOC_G_EXT_CTRLS
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @arg: Pointer to the v4l2_ext_controls structure.
 *
 * Returns:
 *   0 when no error
 *   -ENOSPC: when there is no space to copy the data into the buffer provided
 * by application.
 *   -EINVAL: otherwise
 */
static int vidioc_g_ext_ctrls(
		struct file *file,
		void *priv,
		struct v4l2_ext_controls *arg
		)
{
	struct v4l2_ext_controls *v = arg;
	u32 *p;
	int index = 0;
	int count = 0;
	int ret_val;
	uint8_t status;

	FM_DEBUG_REPORT("vidioc_g_ext_ctrls: Id = %04x", v->controls->id) ;

	switch (v->controls->id) {
	case V4L2_CID_CG2900_RADIO_BANDSCAN_GET_RESULTS:
		{
			if (g_radio_cg2900_device->v_seekstatus ==
				FMR_SEEK_IN_PROGRESS) {
				if (global_event ==
					STE_EVENT_SCAN_CHANNELS_FOUND) {
					/* Check to get Scan Result */
					os_lock();
					status =
					ste_fm_get_scan_result(&no_of_scanfreq,
						scanfreq,
						scanfreq_rssi_level);
					os_unlock();
				}
			}
			FM_DEBUG_REPORT("vidioc_g_ext_ctrls: "\
					"SeekStatus = %d, GlobalEvent = %d, "\
					"numchannels = %d",
					g_radio_cg2900_device->v_seekstatus,
					global_event,
					no_of_scanfreq);
			if (v->ctrl_class == V4L2_CTRL_CLASS_USER) {
				if (v->controls->size == 0 && \
					v->controls->string == NULL) {
					if (
					g_radio_cg2900_device->v_seekstatus == \
						FMR_SEEK_IN_PROGRESS && \
						STE_EVENT_SCAN_CHANNELS_FOUND \
						== global_event) {
						v->controls->size = \
						no_of_scanfreq;
					g_radio_cg2900_device->v_seekstatus
							= FMR_SEEK_NONE;
						global_event = \
						STE_EVENT_NO_EVENT;
						return -ENOSPC;
					}
					if (
					g_radio_cg2900_device->v_seekstatus == \
						FMR_SEEK_IN_PROGRESS && \
						STE_EVENT_NO_CHANNELS_FOUND \
						== global_event) {
						v->count = 0;
					g_radio_cg2900_device->v_seekstatus
						= FMR_SEEK_NONE;
						global_event = \
						STE_EVENT_NO_EVENT;
						return 0;
					}
				} else if (v->controls->string != NULL) {
					os_lock();
					p = (u32 *) v->controls->string;
					while (index < no_of_scanfreq) {
						*(p + count + 0) = \
						HZ_2_V4L2(scanfreq[index]);
						*(p + count + 1) = \
						scanfreq_rssi_level[index];
						/* FM_DEBUG_REPORT \
						("VIDIOC_GET_SCAN: \
						freq = %d, RSSI = %d ", \
						*(p + count + 0), \
						*(p + count + 1)); */
						count += 2;
						index++;
					}
					ret_val = 0;
					os_unlock();
					return 0;
				}
			}
			break;
		}
	}
	return -EINVAL;
}

/**
 * vidioc_s_hw_freq_seek()- seek Up/Down Frequency.
 * This function is used to start seek
 * on the FM Radio. Direction if seek is as inicated by the parameter
 * inside the v4l2_hw_freq_seek structure. This function is called when the
 * application issues the IOCTL VIDIOC_S_HW_FREQ_SEEK
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @arg: Pointer to the v4l2_hw_freq_seek structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_s_hw_freq_seek(
		struct file *file,
		void *priv,
		struct v4l2_hw_freq_seek *arg
		)
{
	struct v4l2_hw_freq_seek *seek = arg;
	uint8_t status;

	FM_DEBUG_REPORT("vidioc_s_hw_freq_seek");
	FM_DEBUG_REPORT("vidioc_s_hw_freq_seek: Status = %d, "\
			"Upwards = %d, Wrap Around = %d", \
			g_radio_cg2900_device->v_seekstatus,
			seek->seek_upward,
			seek->wrap_around);
	if (g_radio_cg2900_device->v_seekstatus == FMR_SEEK_IN_PROGRESS) {
		FM_DEBUG_REPORT("vidioc_s_hw_freq_seek: "\
				"VIDIOC_S_HW_FREQ_SEEK, "\
				"seek in progress");
		return -EINVAL;
	}

	os_lock();
	global_event = STE_EVENT_NO_EVENT;
	search_freq = 0;
	no_of_scanfreq = 0;
	os_unlock();

	g_radio_cg2900_device->v_seekstatus = FMR_SEEK_IN_PROGRESS;
	if (1 == seek->seek_upward)
		status = ste_fm_search_up_freq();
	else if (0 == seek->seek_upward)
		status = ste_fm_search_down_freq();
	else
		return -EINVAL;
	if (STE_STATUS_OK != status)
		return -EINVAL;
	else
		return 0;
}

/**
 * vidioc_g_audio()- Get Audio features of FM Driver.
 * This function is used to get the audio features of FM Driver.
 * This function is imlemented as a dumy function.
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @arg: Pointer to the v4l2_audio structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_g_audio(
		struct file *file,
		void *priv,
		struct v4l2_audio *arg
		)
{
	struct v4l2_audio *v = arg;
	FM_DEBUG_REPORT("vidioc_g_audio");

	strcpy(v->name, ""); /* max. 16 bytes! */
	v->capability = 0;
	v->mode = 0;

	return 0;
}

/**
 * vidioc_s_audio()- Set Audio features of FM Driver.
 * This function is used to set the audio features of FM Driver.
 * This function is imlemented as a dumy function.
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @arg: Pointer to the v4l2_audio structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_s_audio(
		struct file *file,
		void *priv,
		struct v4l2_audio *arg
		)
{
	FM_DEBUG_REPORT("vidioc_s_audio");
	if (arg->index != 0)
		return -EINVAL;
	return 0;
}

/**
 * vidioc_g_input()- Get the Input Value
 * This function is used to get the Input.
 * This function is imlemented as a dumy function.
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @i: Pointer to store the value.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_g_input(
		struct file *file,
		void *priv,
		unsigned int *i
		)
{
	FM_DEBUG_REPORT("vidioc_g_input");
	*i = 0;
	return 0;
}

/**
 * vidioc_s_input()- Set the input value.
 * This function is used to set input.
 * This function is imlemented as a dumy function.
 *
 * @file: Pointer to the file structure
 * @priv: Pointer to the previous data of file structure.
 * @i: Value to set
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_s_input(
		struct file *file,
		void *priv,
		unsigned int i
		)
{
	FM_DEBUG_REPORT("vidioc_s_input");
	if (i != 0)
		return -EINVAL;
	return 0;
}

/**
 * radio_cg2900_convert_err_to_v4l2()- Convert Error Bits to V4L2 RDS format.
 * This function converts the error bits in RDS Block
 * as received from Chip into V4L2 RDS data specification.
 *
 * @status_byte: The status byte as received in RDS Group for
 * particular RDS Block
 * @out_byte: Pointer to the byte to store the modified byte with the err bits
 * alligned as per V4L2 RDS Specifications.
 */
static void radio_cg2900_convert_err_to_v4l2(
		char status_byte,
		char *out_byte
		)
{
	if ((status_byte & 0x03) == 0x03) {
		/* Uncorrectable Block */
		*out_byte = (*out_byte | V4L2_RDS_BLOCK_ERROR);
	} else if (((status_byte & 0x01) == 0x01) ||
			((status_byte & 0x02) == 0x02)) {
		/* Corrected Bits in Block */
		*out_byte = (*out_byte | V4L2_RDS_BLOCK_CORRECTED);
	}
}

/**
 * radio_cg2900_open()- This function nitializes and switches on FM.
 * This is called when the application opens the character device.
 *
 * @file: Pointer to the file structure
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int radio_cg2900_open(
		struct file *file
		)
{
	uint8_t status;
	int ret_val = -EINVAL;
	struct video_device *vdev = video_devdata(file);

	lock_kernel();
	users++;
	FM_DEBUG_REPORT("radio_cg2900_open: users = %d", users);

	if (users > 1) {
		FM_DEBUG_REPORT("radio_cg2900_open: FM already switched on!!!");
		unlock_kernel();
		return -EINVAL;
	} else {
		status = ste_fm_init();
		if (STE_STATUS_OK == status) {
			FM_DEBUG_REPORT("radio_cg2900_open: Switching on FM");
			if (0 == band) {
				freq_low = 87.5 * STE_HRTZ_MULTIPLIER;
				freq_high = 108 * STE_HRTZ_MULTIPLIER;
			} else if (1 == band) {
				freq_low = 70 * STE_HRTZ_MULTIPLIER;
				freq_high = 108 * STE_HRTZ_MULTIPLIER;
			} else {
				freq_low = 76 * STE_HRTZ_MULTIPLIER;
				freq_high = 90 * STE_HRTZ_MULTIPLIER;
			}
			status = ste_fm_switch_on(
					&(vdev->dev),
					freq_low,
					band,
					grid);
			if (STE_STATUS_OK == status) {
				g_radio_cg2900_device->v_state =
					FMR_SWITCH_ON;
				g_radio_cg2900_device->v_frequency =
					HZ_2_V4L2(freq_low);
				g_radio_cg2900_device->v_rds_enabled =
					STE_FALSE;
				g_radio_cg2900_device->v_muted =
					STE_FALSE;
				g_radio_cg2900_device->v_audiopath =
					0;
				g_radio_cg2900_device->v_seekstatus =
					FMR_SEEK_NONE;
				g_radio_cg2900_device->v_rssi_threshold
				= STE_FM_DEFAULT_RSSI_THRESHOLD;
				global_event = STE_EVENT_NO_EVENT;
				no_of_scanfreq = 0;
				ste_fm_set_rssi_threshold(
				g_radio_cg2900_device->v_rssi_threshold);
				ret_val = 0;
			} else {
				ste_fm_deinit();
				users--;
			}
		} else
			users--;
	}
	unlock_kernel();
	return ret_val;
}

/**
 * radio_cg2900_release()- This function switches off FM.
 * This function switches off FM and releases the resources.
 * This is called when the application closes the character
 * device.
 *
 * @file: Pointer to the file structure
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int radio_cg2900_release(
		struct file *file
		)
{
	uint8_t status;
	int ret_val = -EINVAL;

	users--;
	FM_DEBUG_REPORT("radio_cg2900_release: users = %d", users);

	if (0 == users) {
		FM_DEBUG_REPORT("radio_cg2900_release: Switching Off FM");
		status = ste_fm_switch_off();
		status = ste_fm_deinit();
		if (STE_STATUS_OK == status) {
			g_radio_cg2900_device->v_state = FMR_SWITCH_OFF;
			g_radio_cg2900_device->v_frequency = 0;
			g_radio_cg2900_device->v_rds_enabled = STE_FALSE;
			g_radio_cg2900_device->v_muted = STE_FALSE;
			g_radio_cg2900_device->v_seekstatus = FMR_SEEK_NONE;
			global_event = STE_EVENT_NO_EVENT;
			no_of_scanfreq = 0;
			ret_val = 0;
		}
	}
	return ret_val;
}

/**
 * radio_cg2900_read()- This function waits till RDS data is available.
 * This function is invoked when the application calls read() to receive
 * RDS Data.
 *
 * @file: Pointer to the file structure
 * @data: Pointer to the buffer provided by application for receving the data.
 * @count: Number of bytes that application wants to read from driver
 * @pos: Pointer to the offset
 *
 * Returns:
 *   Number of bytes copied to the user buffer
 *   -EFAULT: If there is problem in copying data to buffer supplied
 * by application
 *   -EIO: If the number of bytes to be read are not a multiple of
 *  struct  v4l2_rds_data.
 *   -EAGAIN: More than 22 blocks requested to be read.
 *   0 when no data available for reading.
 */
static ssize_t  radio_cg2900_read(
		struct file *file,
		char __user *data,
		size_t count,
		loff_t *pos
		)
{
	int current_rds_grp;
	int index = 0;
	int blocks_to_read;
	struct  v4l2_rds_data rdsbuf[MAX_RDS_GROUPS_READ * NUM_OF_RDS_BLOCKS];
	struct  v4l2_rds_data *rdslocalbuf = rdsbuf;

	FM_DEBUG_REPORT("radio_cg2900_read");

	blocks_to_read = (count / sizeof(struct v4l2_rds_data));

	if (count % sizeof(struct v4l2_rds_data) != 0) {
		FM_ERR_REPORT("radio_cg2900_read: Invalid Number of bytes %d "\
			"requested to read", count);
		return -EIO;
	}
	if (blocks_to_read > MAX_RDS_GROUPS_READ * NUM_OF_RDS_BLOCKS) {
		FM_ERR_REPORT("radio_cg2900_read: Too many blocks(%d) "\
			"requested to be read", blocks_to_read);
		return -EAGAIN;
	}
	current_rds_grp = rds_group_sent;
	if ((g_radio_cg2900_device->v_rds_enabled) &&
		(head != tail) &&
		(ste_fm_rds_buf[tail][current_rds_grp].block1 != 0x0000)) {
		os_lock();
		while (index < blocks_to_read) {
			switch (rds_block_sent % NUM_OF_RDS_BLOCKS) {
			case 0:
			(rdslocalbuf + index)->lsb = \
			ste_fm_rds_buf[tail][current_rds_grp].block1;
			(rdslocalbuf + index)->msb = \
			ste_fm_rds_buf[tail][current_rds_grp].block1 >> 8;
			(rdslocalbuf + index)->block = \
			(ste_fm_rds_buf[tail][current_rds_grp].status1 & 0x1C) \
			>> 2;
			radio_cg2900_convert_err_to_v4l2(
				ste_fm_rds_buf[tail][current_rds_grp].status1,
				&(rdslocalbuf + index)->block);
			break;
			case 1:
			(rdslocalbuf + index)->lsb = \
			ste_fm_rds_buf[tail][current_rds_grp].block2;
			(rdslocalbuf + index)->msb = \
			ste_fm_rds_buf[tail][current_rds_grp].block2 >> 8;
			(rdslocalbuf + index)->block = \
			(ste_fm_rds_buf[tail][current_rds_grp].status2 & 0x1C) \
			>> 2;
			radio_cg2900_convert_err_to_v4l2(
				ste_fm_rds_buf[tail][current_rds_grp].status2,
				&(rdslocalbuf + index)->block);
			break;
			case 2:
			(rdslocalbuf + index)->lsb = \
			ste_fm_rds_buf[tail][current_rds_grp].block3;
			(rdslocalbuf + index)->msb = \
			ste_fm_rds_buf[tail][current_rds_grp].block3 >> 8;
			(rdslocalbuf + index)->block = \
			(ste_fm_rds_buf[tail][current_rds_grp].status3 & 0x1C) \
			>> 2;
			radio_cg2900_convert_err_to_v4l2(
				ste_fm_rds_buf[tail][current_rds_grp].status3,
				&(rdslocalbuf + index)->block);
			break;
			case 3:
			(rdslocalbuf + index)->lsb = \
			ste_fm_rds_buf[tail][current_rds_grp].block4;
			(rdslocalbuf + index)->msb = \
			ste_fm_rds_buf[tail][current_rds_grp].block4 >> 8;
			(rdslocalbuf + index)->block = \
			(ste_fm_rds_buf[tail][current_rds_grp].status4 & 0x1C) \
			>> 2;
			radio_cg2900_convert_err_to_v4l2(
				ste_fm_rds_buf[tail][current_rds_grp].status4,
				&(rdslocalbuf + index)->block);
			current_rds_grp++;
			if (current_rds_grp == MAX_RDS_GROUPS_READ) {
				tail++;
				current_rds_grp = 0;
			}
			break;
			}
			/* FM_DEBUG_REPORT("%02x%02x %02x ",
			(rdslocalbuf + index)->msb, \
			(rdslocalbuf + index)->lsb, \
			(rdslocalbuf + index)->block); */
			index++;
			rds_block_sent++;
			if (rds_block_sent == NUM_OF_RDS_BLOCKS)
				rds_block_sent = 0;
		}
		/* Update the RDS Group Count Sent to Application */
		rds_group_sent = current_rds_grp;
		if (tail == MAX_RDS_BUFFER)
			tail = 0;

		if (copy_to_user(data, rdslocalbuf, count)) {
			os_unlock();
			FM_ERR_REPORT("radio_cg2900_read: Err "\
				"in copying, returning");
			return -EFAULT;
		}
		os_unlock();
		return count;
	} else {
		FM_INFO_REPORT("radio_cg2900_read: returning 0");
		return 0;
	}
}

/**
 * radio_cg2900_poll()- Check if the operation is compelte or not.
 * This function is invoked by application on calling poll() and is used to
 * wait till the desired operation seek/Band Scan are compelte.
 * Driver blocks till the seek/BandScan Operation is complete. The application
 * decides to read the results of seek/Band Scan based on the returned value of
 * this function.
 *
 * @file: Pointer to the file structure
 * @wait: Pointer to the poll table
 *
 * Returns:
 *   POLLRDBAND when Scan Band is complete and data is available for reading.
 *   POLLRDNORM when seek station is complete.
 *   POLLHUP when seek station/Band Scan is stopped by user using Stop Scan.
 *   POLLERR when timeout occurs
 */
static unsigned int  radio_cg2900_poll(
		struct file *file,
		struct poll_table_struct *wait
		)
{
	uint8_t status;
	int ret;

	FM_DEBUG_REPORT("radio_cg2900_poll");
	ret = wait_event_interruptible_timeout(radio_cg2900_poll_queue,
		((global_event == STE_EVENT_SCAN_CHANNELS_FOUND) ||
		(global_event == STE_EVENT_SEARCH_CHANNEL_FOUND) ||
		(global_event == STE_EVENT_NO_CHANNELS_FOUND)),
		timeval_to_jiffies(&max_scan_seek_time));
	FM_DEBUG_REPORT("+radio_cg2900_pol: Wait Ower, "\
			"v_seekstatus = %d, "\
			"global_event = %d",
			g_radio_cg2900_device->v_seekstatus,
			global_event);

	if (ret) {
		if (g_radio_cg2900_device->v_seekstatus ==
				FMR_SEEK_IN_PROGRESS) {
			if (global_event == STE_EVENT_SCAN_CHANNELS_FOUND) {
				/* Scan Completed, send event to application */
				return POLLRDBAND;
			} else if (global_event ==
				STE_EVENT_SEARCH_CHANNEL_FOUND) {
				uint32_t freq;
				/* Check to get Scan Result */
				status = ste_fm_get_frequency(&freq);
				FM_DEBUG_REPORT("radio_cg2900_poll: "\
				"New freq = %d Hz", freq);
				os_lock();
				search_freq = freq;
				os_unlock();
				/* seek Completed, send event to application */
				FM_DEBUG_REPORT("radio_cg2900_poll: "\
						"returning POLLRDNORM");
				return POLLRDNORM;
			}
		}
		/* Scan/Search cancelled by User */
		os_lock();
		no_of_scanfreq = 0;
		os_unlock();
		g_radio_cg2900_device->v_seekstatus = FMR_SEEK_NONE;
		global_event = STE_EVENT_NO_EVENT;
		FM_DEBUG_REPORT("radio_cg2900_poll: Cancel operation "\
				"returning POLLHUP");
		return POLLHUP;
	} else {
		/* Timeout */
		os_lock();
		no_of_scanfreq = 0;
		os_unlock();
		g_radio_cg2900_device->v_seekstatus = FMR_SEEK_NONE;
		global_event = STE_EVENT_NO_EVENT;
		FM_ERR_REPORT("radio_cg2900_poll: Timeout: returning POLLERR");
		return POLLERR;
	}
}

/**
 * radio_cg2900_init()- This function initializes the FM Driver.
 * This Fucntion is called whenever the Driver is loaded from
 * VideoForLinux driver. It registers the FM Driver with Video4Linux
 * as a character device.
 *
 * Returns:
 *   0 on success
 * -EINVAL on error
 */
static int __init radio_cg2900_init(void)
{
	FM_INFO_REPORT(BANNER);
	FM_DEBUG_REPORT("radio_cg2900_init: radio_nr= %d.", radio_nr);
	/* Initialize the parameters */
	radio_nr = 0;
	grid = 1;
	band = 0;
	if (video_register_device(&radio_cg2900_video_device, \
		VFL_TYPE_RADIO, radio_nr) == -1) {
		FM_ERR_REPORT("radio_cg2900_init: video_register_device err");
		return -EINVAL;
	}
	users = 0;
	return 0;
}

/**
 * radio_cg2900_deinit()- This function deinitializes the FM Driver.
 * This Fucntion is called whenever the Driver is unloaded from
 * VideoForLinux driver.
 */
static void __exit radio_cg2900_deinit(void)
{
	FM_INFO_REPORT("radio_cg2900_deinit");

	/* Try to Switch Off FM in case it is still switched on */
	ste_fm_switch_off();
	ste_fm_deinit();
	video_unregister_device(&radio_cg2900_video_device);
}

/* ------------------ External Function definitions ---------------------- */
void wake_up_poll_queue(void)
{
	FM_DEBUG_REPORT("wake_up_poll_queue");
	wake_up_interruptible(&radio_cg2900_poll_queue);
}

module_init(radio_cg2900_init);
module_exit(radio_cg2900_deinit);
MODULE_AUTHOR("Hemant Gupta");
MODULE_LICENSE("GPL v2");

module_param(radio_nr, int, S_IRUGO);

module_param(grid, int, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(grid, "Grid:"\
		"0=200 kHz"\
		"*1=100 kHz*"\
		"2=50k Hz");

module_param(band, int, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(band, "Band:"\
		"*0=87.5-108 MHz*"\
		"1=70-108 MHz"\
		"2=76-90 MHz");

