/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Linux Wrapper for V4l2 FM Driver for CG2900.
 *
 * Author: Hemant Gupta/hemant.gupta@stericsson.com for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2
 */

#include "stefmapi.h"
#include "fmdriver.h"
#include <string.h>
#include "videodev.h"
#include "v4l2-ioctl.h"
#include "wait.h"
#include <linux/init.h>		/* Initdata */
#include <linux/videodev2.h>	/* v4l2 radio structs           */
#include <media/v4l2-common.h>	/* v4l2 common structs */
#include "platformosapi.h"

#define RADIO_CG2900_VERSION KERNEL_VERSION(1, 1, 0)
#define BANNER "ST-Ericsson FM Radio Card driver v1.1.0"

#define FMR_HRTZ_MULTIPLIER				1000000
#define FMR_EU_US_LOW_FREQ_IN_KHZ			87.5
#define FMR_EU_US_HIGH_FREQ_IN_KHZ			108
#define FMR_JAPAN_LOW_FREQ_IN_KHZ			76
#define FMR_JAPAN_HIGH_FREQ_IN_KHZ			90
#define FMR_CHINA_LOW_FREQ_IN_KHZ			70
#define FMR_CHINA_HIGH_FREQ_IN_KHZ			108

/* freq in Hz to V4l2 freq (units of 62.5Hz) */
#define HZ_TO_V4L2(X)  (2*(X)/125)
/* V4l2 freq (units of 62.5Hz) to freq in Hz */
#define V4L2_TO_HZ(X)  (((X)*125)/(2))

/* ------------------ Internal function declarations ---------------------- */
static int cg2900_open(struct file *file);
static int cg2900_release(struct file *file);
static ssize_t cg2900_read(struct file *file,
			   char __user *data, size_t count, loff_t *pos);
static unsigned int cg2900_poll(struct file *file,
				struct poll_table_struct *wait);
static int vidioc_querycap(struct file *file,
			   void *priv, struct v4l2_capability *query_caps);
static int vidioc_get_tuner(struct file *file,
			    void *priv, struct v4l2_tuner *tuner);
static int vidioc_set_tuner(struct file *file,
			    void *priv, struct v4l2_tuner *tuner);
static int vidioc_get_modulator(struct file *file,
				void *priv, struct v4l2_modulator *modulator);
static int vidioc_set_modulator(struct file *file,
				void *priv, struct v4l2_modulator *modulator);
static int vidioc_get_frequency(struct file *file,
				void *priv, struct v4l2_frequency *freq);
static int vidioc_set_frequency(struct file *file,
				void *priv, struct v4l2_frequency *freq);
static int vidioc_query_ctrl(struct file *file,
			     void *priv, struct v4l2_queryctrl *query_ctrl);
static int vidioc_get_ctrl(struct file *file,
			   void *priv, struct v4l2_control *ctrl);
static int vidioc_set_ctrl(struct file *file,
			   void *priv, struct v4l2_control *ctrl);
static int vidioc_get_ext_ctrls(struct file *file,
				void *priv, struct v4l2_ext_controls *ext_ctrl);
static int vidioc_set_ext_ctrls(struct file *file,
				void *priv, struct v4l2_ext_controls *ext_ctrl);
static int vidioc_set_hw_freq_seek(struct file *file,
				   void *priv,
				   struct v4l2_hw_freq_seek *freq_seek);
static int vidioc_get_audio(struct file *file,
			    void *priv, struct v4l2_audio *audio);
static int vidioc_set_audio(struct file *file,
			    void *priv, struct v4l2_audio *audio);
static int vidioc_get_input(struct file *filp, void *priv, unsigned int *input);
static int vidioc_set_input(struct file *filp, void *priv, unsigned int input);
static void cg2900_convert_err_to_v4l2(char status_byte, char *out_byte);
static int __init cg2900_init(void);
static void __exit cg2900_exit(void);

static u32 freq_low;
static u32 freq_high;

/**************************************************************************
* Module parameters
**************************************************************************/
static int radio_nr = -1;

/* Grid (kHz) */
/* 0: 50 kHz (China) */
/* 1: 100 kHz (Europe, Japan) */
/* 2: 200 kHz (USA) */
static int grid;

/* FM Band (MHz) */
/* 0: 87.5 - 108 MHz (USA, Europe)*/
/* 1: 76   -  90 MHz (Japan) */
/* 2: 70   - 108 MHz (China wide band) */
static int band;

/* cg2900_poll_queue - Main Wait Queue for polling (Scan/seek) */
static wait_queue_head_t cg2900_poll_queue;

/* cg2900_read_queue - Main Wait Queue for receiving RDS Data */
static DECLARE_WAIT_QUEUE_HEAD(cg2900_read_queue);

/**
 * enum fmd_gocmd_t - Seek status of FM Radio.
 * @FMR_SEEK_NONE: No seek in progress.
 * @FMR_SEEK_IN_PROGRESS: Seek is in progress.
 * Seek status of FM Radio.
 */
enum fm_seek_status_t {
	FMR_SEEK_NONE,
	FMR_SEEK_IN_PROGRESS
};

/**
 * enum fm_power_state_t - Power states of FM Radio.
 * @FMR_SWITCH_OFF: FM Radio is switched off.
 * @FMR_SWITCH_ON: FM Radio is switched on.
 * @FMR_STANDBY: FM Radio in standby state.
 * Power states of FM Radio.
 */
enum fm_power_state_t {
	FMR_SWITCH_OFF,
	FMR_SWITCH_ON,
	FMR_STANDBY
};

/**
  * struct cg2900_device_t - Stores FM Device Info.
  * @v_state: state of FM Radio
  * @v_muted: FM Radio Mute/Unmute status
  * @v_seekstatus: seek status
  * @v_rx_rds_enabled: Rds enable/disable status for FM Rx
  * @v_tx_rds_enabled: Rds enable/disable status for FM Tx
  * @v_rx_stereo_status: Stereo Mode status for FM Rx
  * @v_tx_stereo_status: Stereo Mode status for FM Tx
  * @v_volume: Analog Volume Gain of FM Radio
  * @v_rssi_threshold: rssi Thresold set on FM Radio
  * @v_frequency: Frequency tuned on FM Radio in V4L2 Format
  * @v_audiopath: Audio Balance
  * @v_waitonreadqueue: Flag for waiting on read queue.
  * @v_fm_mode: Enum for storing the current FM Mode.
  *
  */
struct cg2900_device_t {
	u8 v_state;
	u8 v_muted;
	u8 v_seekstatus;
	bool v_rx_rds_enabled;
	bool v_tx_rds_enabled;
	bool v_rx_stereo_status;
	bool v_tx_stereo_status;
	int v_volume;
	u16 v_rssi_threshold;
	u32 v_frequency;
	u32 v_audiopath;
	bool v_waitonreadqueue;
	enum ste_fm_mode_t v_fm_mode;
} __attribute__ ((packed));

/* Global Structure to store the maintain FM Driver device info */
static struct cg2900_device_t cg2900_device;

/* V4l2 File Operation Structure */
static const struct v4l2_file_operations cg2900_fops = {
	.owner = THIS_MODULE,
	.open = cg2900_open,
	.release = cg2900_release,
	.read = cg2900_read,
	.poll = cg2900_poll,
	.ioctl = video_ioctl2,
};

/* V4L2 IOCTL Operation Structure */
static const struct v4l2_ioctl_ops cg2900_ioctl_ops = {
	.vidioc_querycap = vidioc_querycap,
	.vidioc_g_tuner = vidioc_get_tuner,
	.vidioc_s_tuner = vidioc_set_tuner,
	.vidioc_g_modulator = vidioc_get_modulator,
	.vidioc_s_modulator = vidioc_set_modulator,
	.vidioc_g_frequency = vidioc_get_frequency,
	.vidioc_s_frequency = vidioc_set_frequency,
	.vidioc_queryctrl = vidioc_query_ctrl,
	.vidioc_g_ctrl = vidioc_get_ctrl,
	.vidioc_s_ctrl = vidioc_set_ctrl,
	.vidioc_g_ext_ctrls = vidioc_get_ext_ctrls,
	.vidioc_s_ext_ctrls = vidioc_set_ext_ctrls,
	.vidioc_s_hw_freq_seek = vidioc_set_hw_freq_seek,
	.vidioc_g_audio = vidioc_get_audio,
	.vidioc_s_audio = vidioc_set_audio,
	.vidioc_g_input = vidioc_get_input,
	.vidioc_s_input = vidioc_set_input,
};

/* V4L2 Video Device Structure */
static struct video_device cg2900_video_device = {
	.name = "STE CG2900 FM Rx/Tx Radio",
	.vfl_type = VID_TYPE_TUNER | VID_TYPE_CAPTURE,
	.fops = &cg2900_fops,
	.ioctl_ops = &cg2900_ioctl_ops,
	.release = video_device_release_empty,
};

static u16 no_of_scan_freq;
static u16 no_of_block_scan_freq;
static u32 scanfreq_rssi_level[MAX_CHANNELS_TO_SCAN];
static u16 block_scan_rssi_level[MAX_CHANNELS_FOR_BLOCK_SCAN];
static u32 scanfreq[MAX_CHANNELS_TO_SCAN];
static int users;

/* ------------------ Internal Function definitions ---------------------- */

/**
 * vidioc_querycap()- Query FM Driver Capabilities.
 * This function is used to query the capabilities of the
 * FM Driver. This function is called when the application issues the IOCTL
 * VIDIOC_QUERYCAP.
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @query_caps: v4l2_capability structure.
 *
 * Returns: 0
 */
static int vidioc_querycap(struct file *file,
			   void *priv, struct v4l2_capability *query_caps)
{
	FM_DEBUG_REPORT("vidioc_querycap");
	memset(query_caps, 0, sizeof(*query_caps));
	strlcpy(query_caps->driver,
		"CG2900 Driver", sizeof(query_caps->driver));
	strlcpy(query_caps->card, "CG2900 FM Radio", sizeof(query_caps->card));
	strcpy(query_caps->bus_info, "platform");
	query_caps->version = RADIO_CG2900_VERSION;
	query_caps->capabilities = V4L2_CAP_TUNER |
	    V4L2_CAP_MODULATOR |
	    V4L2_CAP_RADIO |
	    V4L2_CAP_READWRITE |
	    V4L2_CAP_RDS_CAPTURE | V4L2_CAP_HW_FREQ_SEEK | V4L2_CAP_RDS_OUTPUT;
	FM_DEBUG_REPORT("vidioc_querycap returning 0");
	return 0;
}

/**
 * vidioc_get_tuner()- Get FM Tuner Features.
 * This function is used to get the tuner features.
 * This function is called when the application issues the IOCTL
 * VIDIOC_G_TUNER
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @tuner: v4l2_tuner structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_get_tuner(struct file *file,
			    void *priv, struct v4l2_tuner *tuner)
{
	u8 status = STE_STATUS_OK;
	u8 mode;
	bool rds_enabled;
	u16 rssi;
	FM_DEBUG_REPORT("vidioc_get_tuner");

	if (tuner->index > 0) {
		FM_ERR_REPORT("vidioc_get_tuner: Only 1 tuner supported");
		return -EINVAL;
	}

	memset(tuner, 0, sizeof(*tuner));
	strcpy(tuner->name, "CG2900 FM Receiver");
	tuner->type = V4L2_TUNER_RADIO;
	tuner->rangelow = HZ_TO_V4L2(freq_low);
	tuner->rangehigh = HZ_TO_V4L2(freq_high);
	tuner->capability = V4L2_TUNER_CAP_LOW	/* Frequency steps = 1/16 kHz */
	    | V4L2_TUNER_CAP_STEREO	/* Can receive stereo */
	    | V4L2_TUNER_CAP_RDS;	/* Supports RDS Capture  */

	if (cg2900_device.v_fm_mode == STE_FM_RX_MODE) {
		status = ste_fm_get_mode(&mode);
		FM_DEBUG_REPORT("vidioc_get_tuner: mode = %d, ", mode);
		if (STE_STATUS_OK == status) {
			switch (mode) {
			case STE_MODE_STEREO:
				tuner->audmode = V4L2_TUNER_MODE_STEREO;
				tuner->rxsubchans = V4L2_TUNER_SUB_STEREO;
				tuner->rxsubchans &= ~V4L2_TUNER_SUB_MONO;
				break;
			case STE_MODE_MONO:
			default:
				tuner->audmode = V4L2_TUNER_SUB_MONO;
				tuner->rxsubchans = V4L2_TUNER_SUB_MONO;
				tuner->rxsubchans &= ~V4L2_TUNER_SUB_STEREO;
				break;
			}
		} else {
			/* Get mode API failed, set mode to mono */
			tuner->audmode = V4L2_TUNER_MODE_MONO;
			tuner->rxsubchans = V4L2_TUNER_SUB_MONO;
			tuner->rxsubchans &= ~V4L2_TUNER_SUB_STEREO;
		}

		status = ste_fm_get_rds_status(&rds_enabled);
		if (STE_STATUS_OK == status) {
			if (STE_TRUE == rds_enabled)
				tuner->rxsubchans |= V4L2_TUNER_SUB_RDS;
			else
				tuner->rxsubchans &= ~V4L2_TUNER_SUB_RDS;
		} else {
			tuner->rxsubchans &= ~V4L2_TUNER_SUB_RDS;
		}
	} else {
		tuner->audmode = V4L2_TUNER_SUB_MONO;
		tuner->rxsubchans = V4L2_TUNER_SUB_MONO;
		tuner->rxsubchans &= ~V4L2_TUNER_SUB_RDS;
	}

	if (cg2900_device.v_fm_mode == STE_FM_RX_MODE) {
		status = ste_fm_get_signal_strength(&rssi);
		if (STE_STATUS_OK == status)
			tuner->signal = rssi;
		else
			tuner->signal = 0;
	} else {
		tuner->signal = 0;
	}

	if (STE_STATUS_OK == status)
		return 0;
	else
		return -EINVAL;
}

/**
 * vidioc_set_tuner()- Set FM Tuner Features.
 * This function is used to set the tuner features.
 * It also sets the default FM Rx settings.
 * This function is called when the application issues the IOCTL
 * VIDIOC_S_TUNER
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @tuner: v4l2_tuner structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_set_tuner(struct file *file,
			    void *priv, struct v4l2_tuner *tuner)
{
	bool rds_status = STE_FALSE;
	bool stereo_status = STE_FALSE;
	u8 status = STE_STATUS_OK;

	FM_DEBUG_REPORT("vidioc_set_tuner");
	if (tuner->index != 0) {
		FM_ERR_REPORT("vidioc_set_tuner: Only 1 tuner supported");
		return -EINVAL;
	}

	if (cg2900_device.v_fm_mode != STE_FM_RX_MODE) {
		/* FM Rx mode should be configured as earlier mode was not
		 * FM Rx*/
		if (0 == band) {
			freq_low = FMR_EU_US_LOW_FREQ_IN_KHZ *
			    FMR_HRTZ_MULTIPLIER;
			freq_high = FMR_EU_US_HIGH_FREQ_IN_KHZ *
			    FMR_HRTZ_MULTIPLIER;
		} else if (1 == band) {
			freq_low = FMR_JAPAN_LOW_FREQ_IN_KHZ *
			    FMR_HRTZ_MULTIPLIER;
			freq_high = FMR_JAPAN_HIGH_FREQ_IN_KHZ *
			    FMR_HRTZ_MULTIPLIER;
		} else {
			freq_low = FMR_CHINA_LOW_FREQ_IN_KHZ *
			    FMR_HRTZ_MULTIPLIER;
			freq_high = FMR_CHINA_HIGH_FREQ_IN_KHZ *
			    FMR_HRTZ_MULTIPLIER;
		}
		cg2900_device.v_fm_mode = STE_FM_RX_MODE;
		cg2900_device.v_rx_rds_enabled =
		    (tuner->rxsubchans & V4L2_TUNER_SUB_RDS) ?
		    STE_TRUE : STE_FALSE;
		if (tuner->rxsubchans & V4L2_TUNER_SUB_STEREO)
			stereo_status = STE_TRUE;
		else if (tuner->rxsubchans & V4L2_TUNER_SUB_MONO)
			stereo_status = STE_FALSE;
		cg2900_device.v_rx_stereo_status = stereo_status;
		ste_fm_set_rx_default_settings(freq_low,
					       band,
					       grid,
					       cg2900_device.v_rx_rds_enabled,
					       cg2900_device.
					       v_rx_stereo_status);
		ste_fm_set_rssi_threshold(cg2900_device.v_rssi_threshold);
	} else {
		/* Mode was FM Rx only, change the RDS settings or stereo mode
		 * if they are changed by application */
		rds_status = (tuner->rxsubchans & V4L2_TUNER_SUB_RDS) ?
		    STE_TRUE : STE_FALSE;
		if (tuner->rxsubchans & V4L2_TUNER_SUB_STEREO)
			stereo_status = STE_TRUE;
		else if (tuner->rxsubchans & V4L2_TUNER_SUB_MONO)
			stereo_status = STE_FALSE;
		if (stereo_status != cg2900_device.v_rx_stereo_status) {
			cg2900_device.v_rx_stereo_status = stereo_status;
			if (STE_TRUE == stereo_status)
				status =
				    ste_fm_set_mode(FMD_STEREOMODE_BLENDING);
			else
				status = ste_fm_set_mode(FMD_STEREOMODE_MONO);
		}
		if (rds_status != cg2900_device.v_rx_rds_enabled) {
			cg2900_device.v_rx_rds_enabled = rds_status;
			if (STE_TRUE == rds_status)
				status = ste_fm_rds_on();
			else
				status = ste_fm_rds_off();
		}
	}

	if (STE_STATUS_OK == status)
		return 0;
	else
		return -EINVAL;
}

/**
 * vidioc_get_modulator()- Get FM Modulator Features.
 * This function is used to get the modulator features.
 * This function is called when the application issues the IOCTL
 * VIDIOC_G_MODULATOR
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @modulator: v4l2_modulator structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_get_modulator(struct file *file,
				void *priv, struct v4l2_modulator *modulator)
{
	u8 status = STE_STATUS_OK;
	bool rds_enabled;
	u8 mode;
	FM_DEBUG_REPORT("vidioc_get_modulator");

	if (modulator->index > 0) {
		FM_ERR_REPORT("vidioc_get_modulator: Only 1 "
			      "modulator supported");
		return -EINVAL;
	}

	memset(modulator, 0, sizeof(*modulator));
	strcpy(modulator->name, "CG2900 FM Transmitter");
	modulator->rangelow = freq_low;
	modulator->rangehigh = freq_high;
	modulator->capability = V4L2_TUNER_CAP_NORM	/* Frequency steps =
							1/16 kHz */
	    | V4L2_TUNER_CAP_STEREO	/* Can receive stereo */
	    | V4L2_TUNER_CAP_RDS;	/* Supports RDS Capture  */

	if (cg2900_device.v_fm_mode == STE_FM_TX_MODE) {
		status = ste_fm_get_mode(&mode);
		FM_DEBUG_REPORT("vidioc_get_modulator: mode = %d", mode);
		if (STE_STATUS_OK == status) {
			switch (mode) {
				/* stereo */
			case 0:
				modulator->txsubchans = V4L2_TUNER_SUB_STEREO;
				break;
				/* mono */
			case 1:
				modulator->txsubchans = V4L2_TUNER_SUB_MONO;
				break;
				/* Switching or Blending, set mode as Stereo */
			default:
				modulator->txsubchans = V4L2_TUNER_SUB_STEREO;
			}
		} else {
			/* Get mode API failed, set mode to mono */
			modulator->txsubchans = V4L2_TUNER_SUB_MONO;
		}
		status = ste_fm_get_rds_status(&rds_enabled);
		if (STE_STATUS_OK == status) {
			if (STE_TRUE == rds_enabled)
				modulator->txsubchans |= V4L2_TUNER_SUB_RDS;
			else
				modulator->txsubchans &= ~V4L2_TUNER_SUB_RDS;
		} else {
			modulator->txsubchans &= ~V4L2_TUNER_SUB_RDS;
		}
	} else {
		modulator->txsubchans = V4L2_TUNER_SUB_MONO;
		modulator->txsubchans &= ~V4L2_TUNER_SUB_RDS;
	}

	if (STE_STATUS_OK == status)
		return 0;
	else
		return -EINVAL;
}

/**
 * vidioc_set_modulator()- Set FM Modulator Features.
 * This function is used to set the Modulaotr features.
 * It also sets the default FM Tx settings.
 * This function is called when the application issues the IOCTL
 * VIDIOC_S_MODULATOR
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @modulator: v4l2_modulator structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_set_modulator(struct file *file,
				void *priv, struct v4l2_modulator *modulator)
{
	bool rds_status = STE_FALSE;
	bool stereo_status = STE_FALSE;
	u8 status = STE_STATUS_OK;

	FM_DEBUG_REPORT("vidioc_set_modulator");
	if (modulator->index != 0) {
		FM_ERR_REPORT("vidioc_set_modulator: Only 1 "
			      "modulator supported");
		return -EINVAL;
	}

	if (cg2900_device.v_fm_mode != STE_FM_TX_MODE) {
		/* FM Tx mode should be configured as earlier mode was not
		 * FM Tx*/
		if (0 == band) {
			freq_low = FMR_EU_US_LOW_FREQ_IN_KHZ *
			    FMR_HRTZ_MULTIPLIER;
			freq_high = FMR_EU_US_HIGH_FREQ_IN_KHZ *
			    FMR_HRTZ_MULTIPLIER;
		} else if (1 == band) {
			freq_low = FMR_JAPAN_LOW_FREQ_IN_KHZ *
			    FMR_HRTZ_MULTIPLIER;
			freq_high = FMR_JAPAN_HIGH_FREQ_IN_KHZ *
			    FMR_HRTZ_MULTIPLIER;
		} else {
			freq_low = FMR_CHINA_LOW_FREQ_IN_KHZ *
			    FMR_HRTZ_MULTIPLIER;
			freq_high = FMR_CHINA_HIGH_FREQ_IN_KHZ *
			    FMR_HRTZ_MULTIPLIER;
		}
		cg2900_device.v_fm_mode = STE_FM_TX_MODE;
		cg2900_device.v_rx_rds_enabled = STE_FALSE;
		cg2900_device.v_tx_rds_enabled =
		    (modulator->txsubchans & V4L2_TUNER_SUB_RDS) ?
		    STE_TRUE : STE_FALSE;
		if (modulator->txsubchans & V4L2_TUNER_SUB_STEREO)
			stereo_status = STE_TRUE;
		else if (modulator->txsubchans & V4L2_TUNER_SUB_MONO)
			stereo_status = STE_FALSE;
		cg2900_device.v_tx_stereo_status = stereo_status;
		ste_fm_set_tx_default_settings(freq_low,
					       band,
					       grid,
					       cg2900_device.v_tx_rds_enabled,
					       cg2900_device.
					       v_tx_stereo_status);
	} else {
		/* Mode was FM Tx only, change the RDS settings or stereo mode
		 * if they are changed by application */
		rds_status = (modulator->txsubchans & V4L2_TUNER_SUB_RDS) ?
		    STE_TRUE : STE_FALSE;
		if (modulator->txsubchans & V4L2_TUNER_SUB_STEREO)
			stereo_status = STE_TRUE;
		else if (modulator->txsubchans & V4L2_TUNER_SUB_MONO)
			stereo_status = STE_FALSE;
		if (stereo_status != cg2900_device.v_tx_stereo_status) {
			cg2900_device.v_tx_stereo_status = stereo_status;
			status = ste_fm_set_mode(stereo_status);
		}
		if (rds_status != cg2900_device.v_tx_rds_enabled) {
			cg2900_device.v_tx_rds_enabled = rds_status;
			status = ste_fm_tx_rds(rds_status);
		}
	}

	if (STE_STATUS_OK == status)
		return 0;
	else
		return -EINVAL;
}

/**
 * vidioc_get_frequency()- Get the Current FM Frequnecy.
 * This function is used to get the currently tuned
 * frequency on FM Radio. This function is called when the application
 * issues the IOCTL VIDIOC_G_FREQUENCY
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @freq: v4l2_frequency structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_get_frequency(struct file *file,
				void *priv, struct v4l2_frequency *freq)
{
	u8 status;
	u32 frequency;
	int ret_val = 0;

	FM_DEBUG_REPORT("vidioc_get_frequency: Status = %d",
			cg2900_device.v_seekstatus);
	status = ste_fm_get_frequency(&frequency);
	if (cg2900_device.v_seekstatus == FMR_SEEK_IN_PROGRESS) {
		/*  Check if seek is finished or not */
		if (STE_EVENT_SEARCH_CHANNEL_FOUND == global_event) {
			/* seek is finished */
			os_lock();
			cg2900_device.v_frequency = HZ_TO_V4L2(frequency);
			freq->frequency = cg2900_device.v_frequency;
			/* FM_DEBUG_REPORT("VIDIOC_G_FREQUENCY: "\
			   "STE_EVENT_SEARCH_CHANNEL_FOUND, "\
			   "freq= %d SearchFreq = %d",
			   cg2900_device.v_frequency,
			   frequency);  */
			cg2900_device.v_seekstatus = FMR_SEEK_NONE;
			global_event = STE_EVENT_NO_EVENT;
			os_unlock();
			ret_val = 0;
		}
	} else {
		os_lock();
		if (STE_STATUS_OK == status) {
			cg2900_device.v_frequency = HZ_TO_V4L2(frequency);
			freq->frequency = cg2900_device.v_frequency;
			ret_val = 0;
		} else {
			freq->frequency = cg2900_device.v_frequency;
			ret_val = -EINVAL;
		}
		os_unlock();
	}
	FM_DEBUG_REPORT("vidioc_get_frequency: Status = %d, returning",
			cg2900_device.v_seekstatus);
	return ret_val;
}

/**
 * vidioc_set_frequency()- Set the FM Frequnecy.
 * This function is used to set the frequency
 * on FM Radio. This function is called when the application
 * issues the IOCTL VIDIOC_S_FREQUENCY
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @freq: v4l2_frequency structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_set_frequency(struct file *file,
				void *priv, struct v4l2_frequency *freq)
{
	u32 frequency = freq->frequency;
	u8 status;

	FM_DEBUG_REPORT("vidioc_set_frequency: Frequency = "
			"%d ", V4L2_TO_HZ(frequency));

	os_lock();
	global_event = STE_EVENT_NO_EVENT;
	no_of_scan_freq = 0;
	os_unlock();

	cg2900_device.v_seekstatus = FMR_SEEK_NONE;
	cg2900_device.v_frequency = frequency;
	status = ste_fm_set_frequency(V4L2_TO_HZ(frequency));
	if (STE_STATUS_OK == status)
		return 0;
	else
		return -EINVAL;
}

/**
 * vidioc_query_ctrl()- Query the FM Driver control features.
 * This function is used to query the control features on FM Radio.
 * This function is called when the application
 * issues the IOCTL VIDIOC_QUERYCTRL
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @query_ctrl: v4l2_queryctrl structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_query_ctrl(struct file *file,
			     void *priv, struct v4l2_queryctrl *query_ctrl)
{
	FM_DEBUG_REPORT("vidioc_query_ctrl");
	/* Check which control is requested */
	switch (query_ctrl->id) {
		/* Audio Mute. This is a hardware function in the CG2900
		radio */
	case V4L2_CID_AUDIO_MUTE:
		FM_DEBUG_REPORT("vidioc_query_ctrl:  V4L2_CID_AUDIO_MUTE");
		query_ctrl->type = V4L2_CTRL_TYPE_BOOLEAN;
		query_ctrl->minimum = 0;
		query_ctrl->maximum = 1;
		query_ctrl->step = 1;
		query_ctrl->default_value = 0;
		query_ctrl->flags = 0;
		strncpy(query_ctrl->name, "CG2900 Mute", 32);
		break;

		/* Audio Volume. Not implemented in
		 * hardware. just a dummy function. */
	case V4L2_CID_AUDIO_VOLUME:
		FM_DEBUG_REPORT("vidioc_query_ctrl: V4L2_CID_AUDIO_VOLUME");
		strncpy(query_ctrl->name, "CG2900 Volume", 32);
		query_ctrl->minimum = MIN_ANALOG_VOLUME;
		query_ctrl->maximum = MAX_ANALOG_VOLUME;
		query_ctrl->step = 1;
		query_ctrl->default_value = MAX_ANALOG_VOLUME;
		query_ctrl->flags = 0;
		query_ctrl->type = V4L2_CTRL_TYPE_INTEGER;
		break;

	case V4L2_CID_AUDIO_BALANCE:
		FM_DEBUG_REPORT("vidioc_query_ctrl:   V4L2_CID_AUDIO_BALANCE ");
		strncpy(query_ctrl->name, "CG2900 Audio Balance", 32);
		query_ctrl->type = V4L2_CTRL_TYPE_INTEGER;
		query_ctrl->minimum = 0x0000;
		query_ctrl->maximum = 0xFFFF;
		query_ctrl->step = 0x0001;
		query_ctrl->default_value = 0x0000;
		query_ctrl->flags = 0;
		break;

		/* Explicitely list some CIDs to produce a
		 * verbose output in debug mode. */
	case V4L2_CID_AUDIO_BASS:
		FM_DEBUG_REPORT("vidioc_query_ctrl: "
				"V4L2_CID_AUDIO_BASS (unsupported)");
		return -EINVAL;

	case V4L2_CID_AUDIO_TREBLE:
		FM_DEBUG_REPORT("vidioc_query_ctrl: "
				"V4L2_CID_AUDIO_TREBLE (unsupported)");
		return -EINVAL;

	default:
		FM_DEBUG_REPORT("vidioc_query_ctrl: "
				"--> unsupported id = %d", (int)query_ctrl->id);
		return -EINVAL;
	}

	return 0;
}

/**
 * vidioc_get_ctrl()- Get the value of a particular Control.
 * This function is used to get the value of a
 * particular control from the FM Driver. This function is called
 * when the application issues the IOCTL VIDIOC_G_CTRL
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @ctrl: v4l2_control structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_get_ctrl(struct file *file,
			   void *priv, struct v4l2_control *ctrl)
{
	u8 status = STE_STATUS_OK;
	u8 value;
	u16 rssi;
	u8 antenna;
	u16 conclusion;
	int ret_val = -EINVAL;

	FM_DEBUG_REPORT("vidioc_get_ctrl");

	switch (ctrl->id) {
	case V4L2_CID_AUDIO_VOLUME:
		status = ste_fm_get_volume(&value);
		if (STE_STATUS_OK == status) {
			ctrl->value = value;
			cg2900_device.v_volume = value;
			ret_val = 0;
		}
		break;
	case V4L2_CID_AUDIO_MUTE:
		ctrl->value = cg2900_device.v_muted;
		ret_val = 0;
		break;
	case V4L2_CID_AUDIO_BALANCE:
		ctrl->value = cg2900_device.v_audiopath;
		ret_val = 0;
		break;
	case V4L2_CID_CG2900_RADIO_RSSI_THRESHOLD:
		ctrl->value = cg2900_device.v_rssi_threshold;
		ret_val = 0;
		break;
	case V4L2_CID_CG2900_RADIO_SELECT_ANTENNA:
		status = ste_fm_get_antenna(&antenna);
		FM_DEBUG_REPORT("vidioc_get_ctrl: Antenna = %d", antenna);
		if (STE_STATUS_OK == status) {
			ctrl->value = antenna;
			ret_val = 0;
		}
		break;
	case V4L2_CID_CG2900_RADIO_RDS_AF_UPDATE_GET_RESULT:
		status = ste_fm_af_update_get_result(&rssi);
		FM_DEBUG_REPORT("vidioc_get_ctrl: AF RSSI Level = %d", rssi);
		if (STE_STATUS_OK == status) {
			ctrl->value = rssi;
			ret_val = 0;
		}
		break;
	case V4L2_CID_CG2900_RADIO_RDS_AF_SWITCH_GET_RESULT:
		status = ste_fm_af_switch_get_result(&conclusion);
		FM_DEBUG_REPORT("vidioc_get_ctrl: AF Switch conclusion = %d",
				conclusion);
		if (STE_STATUS_OK == status) {
			ctrl->value = conclusion;
			ret_val = 0;
		}
		break;
	default:
		FM_DEBUG_REPORT("vidioc_get_ctrl: "
				"unsupported (id = %d)", (int)ctrl->id);
		ret_val = -EINVAL;
	}
	return ret_val;
}

/**
 * vidioc_set_ctrl()- Set the value of a particular Control.
 * This function is used to set the value of a
 * particular control from the FM Driver. This function is called when the
 * application issues the IOCTL VIDIOC_S_CTRL
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @ctrl: v4l2_control structure.
 *
 * Returns:
 *   0 when no error
 *   -ERANGE when the parameter is out of range.
 *   -EINVAL: otherwise
 */
static int vidioc_set_ctrl(struct file *file,
			   void *priv, struct v4l2_control *ctrl)
{
	u8 status = STE_STATUS_OK;
	int ret_val;
	FM_DEBUG_REPORT("vidioc_set_ctrl");
	/* Check which control is requested */
	switch (ctrl->id) {
		/*Audio Mute. This is a hardware function in the CG2900 radio*/
	case V4L2_CID_AUDIO_MUTE:
		FM_DEBUG_REPORT("vidioc_set_ctrl: "
				"V4L2_CID_AUDIO_MUTE, "
				"value = %d", ctrl->value);
		if (ctrl->value > 1 && ctrl->value < 0)
			return -ERANGE;
		if (ctrl->value) {
			FM_DEBUG_REPORT("vidioc_set_ctrl: Ctrl_Id = "
					"V4L2_CID_AUDIO_MUTE, "
					"Muting the Radio");
			status = ste_fm_mute();
		} else {
			FM_DEBUG_REPORT("vidioc_set_ctrl: "
					"Ctrl_Id = V4L2_CID_AUDIO_MUTE, "
					"UnMuting the Radio");
			status = ste_fm_unmute();
		}
		if (STE_STATUS_OK == status) {
			cg2900_device.v_muted = ctrl->value;
			ret_val = 0;
		} else {
			ret_val = -EINVAL;
		}
		break;

	case V4L2_CID_AUDIO_VOLUME:
		FM_DEBUG_REPORT("vidioc_set_ctrl: "
				"V4L2_CID_AUDIO_VOLUME, "
				"value = %d", ctrl->value);
		if (ctrl->value > 20 && ctrl->value < 0)
			return -ERANGE;
		status = ste_fm_set_volume(ctrl->value);
		if (STE_STATUS_OK == status) {
			cg2900_device.v_volume = ctrl->value;
			ret_val = 0;
		} else {
			ret_val = -EINVAL;
		}
		break;

	case V4L2_CID_AUDIO_BALANCE:
		FM_DEBUG_REPORT("vidioc_set_ctrl: "
				"V4L2_CID_AUDIO_BALANCE, "
				"value = %d", ctrl->value);
		status = ste_fm_set_audio_balance(ctrl->value);
		if (STE_STATUS_OK == status) {
			cg2900_device.v_audiopath = ctrl->value;
			ret_val = 0;
		} else {
			ret_val = -EINVAL;
		}
		break;

	case V4L2_CID_CG2900_RADIO_CHIP_STATE:
		FM_DEBUG_REPORT("vidioc_set_ctrl: "
				"V4L2_CID_CG2900_RADIO_CHIP_STATE, "
				"value = %d", ctrl->value);
		if (V4L2_CG2900_RADIO_STANDBY == ctrl->value)
			status = ste_fm_standby();
		else if (V4L2_CG2900_RADIO_POWERUP == ctrl->value)
			status = ste_fm_power_up_from_standby();
		if (STE_STATUS_OK == status) {
			if (V4L2_CG2900_RADIO_STANDBY == ctrl->value)
				cg2900_device.v_state = FMR_STANDBY;
			else if (V4L2_CG2900_RADIO_POWERUP == ctrl->value)
				cg2900_device.v_state = FMR_SWITCH_ON;
			ret_val = 0;
		} else {
			ret_val = -EINVAL;
		}
		break;

	case V4L2_CID_CG2900_RADIO_SELECT_ANTENNA:
		FM_DEBUG_REPORT("vidioc_set_ctrl: "
				"V4L2_CID_CG2900_RADIO_SELECT_ANTENNA, "
				"value = %d", ctrl->value);
		status = ste_fm_select_antenna(ctrl->value);
		if (STE_STATUS_OK == status)
			ret_val = 0;
		else
			ret_val = -EINVAL;
		break;

	case V4L2_CID_CG2900_RADIO_BANDSCAN:
		FM_DEBUG_REPORT("vidioc_set_ctrl: "
				"V4L2_CID_CG2900_RADIO_BANDSCAN, "
				"value = %d", ctrl->value);
		if (V4L2_CG2900_RADIO_BANDSCAN_START == ctrl->value) {
			no_of_scan_freq = 0;
			status = ste_fm_start_band_scan();
		} else if (V4L2_CG2900_RADIO_BANDSCAN_STOP == ctrl->value) {
			status = ste_fm_stop_scan();
		}
		if (STE_STATUS_OK == status) {
			cg2900_device.v_seekstatus = FMR_SEEK_IN_PROGRESS;
			ret_val = 0;
		} else {
			ret_val = -EINVAL;
		}
		break;

	case V4L2_CID_CG2900_RADIO_BLOCKSCAN_START:
		FM_DEBUG_REPORT("vidioc_set_ctrl: "
				"V4L2_CID_CG2900_RADIO_BLOCKSCAN_START");
		no_of_block_scan_freq = 0;
		status = ste_fm_start_block_scan();
		if (STE_STATUS_OK == status) {
			cg2900_device.v_seekstatus = FMR_SEEK_IN_PROGRESS;
			ret_val = 0;
		} else {
			ret_val = -EINVAL;
		}
		break;

	case V4L2_CID_CG2900_RADIO_RSSI_THRESHOLD:
		FM_DEBUG_REPORT("vidioc_set_ctrl: "
				"V4L2_CID_CG2900_RADIO_RSSI_THRESHOLD "
				"= %d", ctrl->value);
		status = ste_fm_set_rssi_threshold(ctrl->value);
		if (STE_STATUS_OK == status) {
			cg2900_device.v_rssi_threshold = ctrl->value;
			ret_val = 0;
		} else {
			ret_val = -EINVAL;
		}
		break;

	case V4L2_CID_CG2900_RADIO_RDS_AF_UPDATE_START:
		FM_DEBUG_REPORT("vidioc_set_ctrl: "
				"V4L2_CID_CG2900_RADIO_RDS_AF_UPDATE_START "
				"freq = %d Hz", ctrl->value);
		status = ste_fm_af_update_start(ctrl->value);
		if (STE_STATUS_OK == status)
			ret_val = 0;
		else
			ret_val = -EINVAL;
		break;

	default:
		FM_DEBUG_REPORT("vidioc_set_ctrl: "
				"unsupported (id = %d)", (int)ctrl->id);
		ret_val = -EINVAL;
	}

	return ret_val;
}

/**
 * vidioc_get_ext_ctrls()- Get the values of a particular control.
 * This function is used to get the value of a
 * particular control from the FM Driver. This is used when the data to
 * be received is more than 1 paramter. This function is called when the
 * application issues the IOCTL VIDIOC_G_EXT_CTRLS
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @ext_ctrl: v4l2_ext_controls structure.
 *
 * Returns:
 *   0 when no error
 *   -ENOSPC: when there is no space to copy the data into the buffer provided
 * by application.
 *   -EINVAL: otherwise
 */
static int vidioc_get_ext_ctrls(struct file *file,
				void *priv, struct v4l2_ext_controls *ext_ctrl)
{
	u32 *dest_buffer;
	int index = 0;
	int count = 0;
	int ret_val = -EINVAL;
	u8 status;

	FM_DEBUG_REPORT("vidioc_get_ext_ctrls: Id = %04x,"
			"ext_ctrl->ctrl_class = %04x",
			ext_ctrl->controls->id, ext_ctrl->ctrl_class);
	if (ext_ctrl->ctrl_class != V4L2_CTRL_CLASS_FM_TX &&
	    ext_ctrl->ctrl_class != V4L2_CTRL_CLASS_USER) {
		FM_ERR_REPORT("vidioc_get_ext_ctrls: Unsupported "
			      "ctrl_class = %04x", ext_ctrl->ctrl_class);
		return ret_val;
	}

	switch (ext_ctrl->controls->id) {
	case V4L2_CID_CG2900_RADIO_BANDSCAN_GET_RESULTS:
		{
			if (cg2900_device.v_seekstatus ==
				FMR_SEEK_IN_PROGRESS) {
				if (global_event ==
				    STE_EVENT_SCAN_CHANNELS_FOUND) {
					/* Check to get Scan Result */
					status =
					    ste_fm_get_scan_result
					    (&no_of_scan_freq, scanfreq,
					     scanfreq_rssi_level);
					if (STE_STATUS_OK != status) {
						FM_ERR_REPORT
						    ("vidioc_get_ext_ctrls: "
						     "ste_fm_get_scan_result: "
						     "returned %d", status);
						return ret_val;
					}
				}
			}
			FM_DEBUG_REPORT("vidioc_get_ext_ctrls: "
					"SeekStatus = %d, GlobalEvent = %d, "
					"numchannels = %d",
					cg2900_device.v_seekstatus,
					global_event, no_of_scan_freq);
			if (ext_ctrl->ctrl_class == V4L2_CTRL_CLASS_USER) {
				if (ext_ctrl->controls->size == 0 &&
				    ext_ctrl->controls->string == NULL) {
					if (cg2900_device.v_seekstatus ==
					    FMR_SEEK_IN_PROGRESS &&
					    STE_EVENT_SCAN_CHANNELS_FOUND
					    == global_event) {
						os_lock();
						ext_ctrl->controls->size =
						    no_of_scan_freq;
						cg2900_device.v_seekstatus
						    = FMR_SEEK_NONE;
						global_event =
						    STE_EVENT_NO_EVENT;
						os_unlock();
						return -ENOSPC;
					}
				} else if (ext_ctrl->controls->string != NULL) {
					dest_buffer =
					    (u32 *) ext_ctrl->controls->string;
					while (index < no_of_scan_freq) {
						*(dest_buffer + count + 0) =
						    HZ_TO_V4L2(scanfreq[index]);
						*(dest_buffer + count + 1) =
						    scanfreq_rssi_level[index];
						count += 2;
						index++;
					}
					ret_val = 0;
					return ret_val;
				}
			}
			break;
		}
	case V4L2_CID_CG2900_RADIO_BLOCKSCAN_GET_RESULTS:
		{
			if (cg2900_device.v_seekstatus ==
				FMR_SEEK_IN_PROGRESS) {
				if (global_event ==
				    STE_EVENT_BLOCK_SCAN_CHANNELS_FOUND) {
					/* Check to get BlockScan Result */
					status =
					    ste_fm_get_block_scan_result
					    (&no_of_block_scan_freq,
					     block_scan_rssi_level);
					if (STE_STATUS_OK != status) {
						FM_ERR_REPORT
						    ("vidioc_get_ext_ctrls: "
						     "ste_fm_get_block_scan_"
						     "result: " "returned %d",
						     status);
						return ret_val;
					}
				}
			}
			FM_DEBUG_REPORT("vidioc_get_ext_ctrls: "
					"SeekStatus = %d, GlobalEvent = %d, "
					"numchannels = %d",
					cg2900_device.v_seekstatus,
					global_event, no_of_block_scan_freq);
			if (ext_ctrl->ctrl_class == V4L2_CTRL_CLASS_USER) {
				if (ext_ctrl->controls->size == 0 &&
				    ext_ctrl->controls->string == NULL) {
					if (cg2900_device.v_seekstatus ==
					    FMR_SEEK_IN_PROGRESS &&
					    STE_EVENT_BLOCK_SCAN_CHANNELS_FOUND
					    == global_event) {
						os_lock();
						ext_ctrl->controls->size =
						    no_of_block_scan_freq;
						cg2900_device.v_seekstatus
						    = FMR_SEEK_NONE;
						global_event =
						    STE_EVENT_NO_EVENT;
						os_unlock();
						return -ENOSPC;
					}
				} else if (ext_ctrl->controls->size >=
					   no_of_block_scan_freq &&
					   ext_ctrl->controls->string != NULL) {
					dest_buffer =
					    (u32 *) ext_ctrl->controls->string;
					while (index < no_of_block_scan_freq) {
						*(dest_buffer + index) =
						    block_scan_rssi_level
						    [index];
						index++;
					}
					ret_val = 0;
					return ret_val;
				}
			}
			break;
		}
	case V4L2_CID_RDS_TX_DEVIATION:
		{
			FM_DEBUG_REPORT("vidioc_get_ext_ctrls: "
					"V4L2_CID_RDS_TX_DEVIATION");
			if (V4L2_CTRL_CLASS_FM_TX != ext_ctrl->ctrl_class) {
				FM_ERR_REPORT("Invalid Ctrl Class = %d",
					      ext_ctrl->ctrl_class);
				return -EINVAL;
			}
			status = ste_fm_tx_get_rds_deviation((u16 *) &
							     ext_ctrl->
							     controls->value);
			if (status == STE_STATUS_OK)
				ret_val = 0;
			break;
		}
	case V4L2_CID_PILOT_TONE_ENABLED:
		{
			FM_DEBUG_REPORT("vidioc_get_ext_ctrls: "
					"V4L2_CID_PILOT_TONE_ENABLED");
			if (V4L2_CTRL_CLASS_FM_TX != ext_ctrl->ctrl_class) {
				FM_ERR_REPORT("Invalid Ctrl Class = %d",
					      ext_ctrl->ctrl_class);
				return -EINVAL;
			}
			status = ste_fm_tx_get_pilot_tone_status((bool *) &
								 ext_ctrl->
								 controls->
								 value);
			if (status == STE_STATUS_OK)
				ret_val = 0;
			break;
		}
	case V4L2_CID_PILOT_TONE_DEVIATION:
		{
			FM_DEBUG_REPORT("vidioc_get_ext_ctrls: "
					"V4L2_CID_PILOT_TONE_DEVIATION");
			if (V4L2_CTRL_CLASS_FM_TX != ext_ctrl->ctrl_class) {
				FM_ERR_REPORT("Invalid Ctrl Class = %d",
					      ext_ctrl->ctrl_class);
				return -EINVAL;
			}
			status = ste_fm_tx_get_pilot_deviation((u16 *) &
							       ext_ctrl->
							       controls->value);
			if (status == STE_STATUS_OK)
				ret_val = 0;
			break;
		}
	case V4L2_CID_TUNE_PREEMPHASIS:
		{
			FM_DEBUG_REPORT("vidioc_get_ext_ctrls: "
					"V4L2_CID_TUNE_PREEMPHASIS");
			if (V4L2_CTRL_CLASS_FM_TX != ext_ctrl->ctrl_class) {
				FM_ERR_REPORT("Invalid Ctrl Class = %d",
					      ext_ctrl->ctrl_class);
				return -EINVAL;
			}
			status = ste_fm_tx_get_preemphasis((u8 *) &ext_ctrl->
							   controls->value);
			if (status == STE_STATUS_OK)
				ret_val = 0;
			break;
		}
	case V4L2_CID_TUNE_POWER_LEVEL:
		{
			FM_DEBUG_REPORT("vidioc_get_ext_ctrls: "
					"V4L2_CID_TUNE_POWER_LEVEL");
			if (V4L2_CTRL_CLASS_FM_TX != ext_ctrl->ctrl_class) {
				FM_ERR_REPORT("Invalid Ctrl Class = %d",
					      ext_ctrl->ctrl_class);
				return -EINVAL;
			}
			status = ste_fm_tx_get_power_level((u16 *) &ext_ctrl->
							   controls->value);
			if (status == STE_STATUS_OK)
				ret_val = 0;
			break;
		}
	default:
		{
			FM_DEBUG_REPORT("vidioc_get_ext_ctrls: "
					"unsupported (id = %d)",
					(int)ext_ctrl->controls->id);
			ret_val = -EINVAL;
		}
	}

	FM_DEBUG_REPORT("vidioc_get_ext_ctrls: returning = %d", ret_val);
	return ret_val;
}

/**
 * vidioc_set_ext_ctrls()- Set the values of a particular control.
 * This function is used to set the value of a
 * particular control on the FM Driver. This is used when the data to
 * be set is more than 1 paramter. This function is called when the
 * application issues the IOCTL VIDIOC_S_EXT_CTRLS
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @ext_ctrl: v4l2_ext_controls structure.
 *
 * Returns:
 *   0 when no error
 *   -ENOSPC: when there is no space to copy the data into the buffer provided
 * by application.
 *   -EINVAL: otherwise
 */
static int vidioc_set_ext_ctrls(struct file *file,
				void *priv, struct v4l2_ext_controls *ext_ctrl)
{
	int ret_val = -EINVAL;
	u8 status;

	FM_DEBUG_REPORT("vidioc_set_ext_ctrls: Id = %04x, ctrl_class = %04x",
			ext_ctrl->controls->id, ext_ctrl->ctrl_class);

	if (ext_ctrl->ctrl_class != V4L2_CTRL_CLASS_FM_TX &&
	    ext_ctrl->ctrl_class != V4L2_CTRL_CLASS_USER) {
		FM_ERR_REPORT("vidioc_set_ext_ctrls: Unsupported "
			      "ctrl_class = %04x", ext_ctrl->ctrl_class);
		return ret_val;
	}

	switch (ext_ctrl->controls->id) {
	case V4L2_CID_CG2900_RADIO_RDS_AF_SWITCH_START:
		{
			u32 af_switch_freq;
			u16 af_switch_pi;
			u32 *pointer;
			if (ext_ctrl->ctrl_class == V4L2_CTRL_CLASS_USER) {
				if (ext_ctrl->controls->size == 2 &&
				    ext_ctrl->controls->string != NULL) {
					pointer =
					    (u32 *) ext_ctrl->controls->string;
					os_mem_copy(&af_switch_freq,
						    pointer, sizeof(u32));
					os_mem_copy(&af_switch_pi,
						    pointer + sizeof(u32),
						    sizeof(u16));
					FM_DEBUG_REPORT("vidioc_set_ext_ctrls: "
							"V4L2_CID_CG2900_RADIO_"
							"RDS_AF_SWITCH_START: "
							"AF Switch Freq =%d Hz "
							"AF Switch PI = %04x",
							af_switch_freq,
							af_switch_pi);
					if (af_switch_freq <
					    (FMR_CHINA_LOW_FREQ_IN_KHZ
					     * FMR_HRTZ_MULTIPLIER) ||
					    af_switch_freq >
					    (FMR_CHINA_HIGH_FREQ_IN_KHZ
					     * FMR_HRTZ_MULTIPLIER)) {
						FM_ERR_REPORT("Invalid Freq "
							      "= %04x",
							      af_switch_freq);
					} else {
						status =
						    ste_fm_af_switch_start
						    (af_switch_freq,
						     af_switch_pi);
						if (STE_STATUS_OK == status)
							ret_val = 0;
					}
				}
			}
			break;
		}
	case V4L2_CID_RDS_TX_DEVIATION:
		{
			FM_DEBUG_REPORT("vidioc_set_ext_ctrls: "
					"V4L2_CID_RDS_TX_DEVIATION, "
					"Value = %d",
					ext_ctrl->controls->value);
			if (ext_ctrl->controls->value <= 0 &&
			    ext_ctrl->controls->value > 750) {
				FM_ERR_REPORT("Invalid RDS Deviation = %02x",
					      ext_ctrl->controls->value);
			} else {
				status =
				    ste_fm_tx_set_rds_deviation(ext_ctrl->
								controls->
								value);
				if (STE_STATUS_OK == status)
					ret_val = 0;
			}
			break;
		}
	case V4L2_CID_RDS_TX_PI:
		{
			FM_DEBUG_REPORT("vidioc_set_ext_ctrls: "
					"V4L2_CID_RDS_TX_PI, PI = %04x",
					ext_ctrl->controls->value);
			if (ext_ctrl->controls->value <= 0x0000 &&
			    ext_ctrl->controls->value > 0xFFFF) {
				FM_ERR_REPORT("Invalid PI = %04x",
					      ext_ctrl->controls->value);
			} else {
				status =
				    ste_fm_tx_set_pi_code(ext_ctrl->controls->
							  value);
				if (STE_STATUS_OK == status)
					ret_val = 0;
			}
			break;
		}
	case V4L2_CID_RDS_TX_PTY:
		{
			FM_DEBUG_REPORT("vidioc_set_ext_ctrls: "
					"V4L2_CID_RDS_TX_PTY, PTY = %d",
					ext_ctrl->controls->value);
			if (ext_ctrl->controls->value < 0 &&
			    ext_ctrl->controls->value > 31) {
				FM_ERR_REPORT("Invalid PTY = %02x",
					      ext_ctrl->controls->value);
			} else {
				status =
				    ste_fm_tx_set_pty_code(ext_ctrl->controls->
							   value);
				if (STE_STATUS_OK == status)
					ret_val = 0;
			}
			break;
		}
	case V4L2_CID_RDS_TX_PS_NAME:
		{
			if (ext_ctrl->controls->size > MAX_PSN_SIZE
			    || ext_ctrl->controls->string == NULL) {
				FM_ERR_REPORT("Invalid PSN");
			} else {
				FM_DEBUG_REPORT("vidioc_set_ext_ctrls: "
						"V4L2_CID_RDS_TX_PS_NAME, "
						"PSN = %s, Len = %d",
						ext_ctrl->controls->string,
						ext_ctrl->controls->size);

				status =
				    ste_fm_tx_set_program_station_name
				    (ext_ctrl->controls->string,
				     ext_ctrl->controls->size);
				if (STE_STATUS_OK == status)
					ret_val = 0;
			}
			break;
		}
	case V4L2_CID_RDS_TX_RADIO_TEXT:
		{
			if (ext_ctrl->controls->size >= MAX_RT_SIZE
			    || ext_ctrl->controls->string == NULL) {
				FM_ERR_REPORT("Invalid RT");
			} else {
				FM_DEBUG_REPORT("vidioc_set_ext_ctrls: "
						"V4L2_CID_RDS_TX_RADIO_TEXT, "
						"RT = %s, Len = %d",
						ext_ctrl->controls->string,
						ext_ctrl->controls->size);
				status =
				    ste_fm_tx_set_radio_text(ext_ctrl->
							     controls->string,
							     ext_ctrl->
							     controls->size);
				if (STE_STATUS_OK == status)
					ret_val = 0;
			}
			break;
		}
	case V4L2_CID_PILOT_TONE_ENABLED:
		{
			bool enable;
			FM_DEBUG_REPORT("vidioc_set_ext_ctrls: "
					"V4L2_CID_PILOT_TONE_ENABLED, "
					"Value = %d",
					ext_ctrl->controls->value);
			if (1 == ext_ctrl->controls->value) {
				enable = true;
			} else if (0 == ext_ctrl->controls->value) {
				enable = false;
			} else {
				FM_ERR_REPORT("Unsupported Value = %d",
					      ext_ctrl->controls->value);
				ret_val = -EINVAL;
				goto err;
			}
			status = ste_fm_tx_set_pilot_tone_status(enable);
			if (STE_STATUS_OK == status)
				ret_val = 0;
			break;
		}
	case V4L2_CID_PILOT_TONE_DEVIATION:
		{
			FM_DEBUG_REPORT("vidioc_set_ext_ctrls: "
					"V4L2_CID_PILOT_TONE_DEVIATION, "
					"Value = %d",
					ext_ctrl->controls->value);
			if (ext_ctrl->controls->value <= 0 &&
			    ext_ctrl->controls->value > 1000) {
				FM_ERR_REPORT("Invalid Pilot Deviation = %02x",
					      ext_ctrl->controls->value);
			} else {
				status =
				    ste_fm_tx_set_pilot_deviation(ext_ctrl->
								  controls->
								  value);
				if (STE_STATUS_OK == status)
					ret_val = 0;
			}
			break;
		}
	case V4L2_CID_TUNE_PREEMPHASIS:
		{
			u8 preemphasis;
			FM_DEBUG_REPORT("vidioc_set_ext_ctrls: "
					"V4L2_CID_TUNE_PREEMPHASIS, "
					"Value = %d",
					ext_ctrl->controls->value);
			if (V4L2_PREEMPHASIS_50_uS ==
				ext_ctrl->controls->value) {
				preemphasis = FMD_EMPHASIS_50US;
			} else if (V4L2_PREEMPHASIS_75_uS ==
				   ext_ctrl->controls->value) {
				preemphasis = FMD_EMPHASIS_75US;
			} else {
				FM_ERR_REPORT("Unsupported Preemphasis = %d",
					      ext_ctrl->controls->value);
				ret_val = -EINVAL;
				goto err;
			}
			status = ste_fm_tx_set_preemphasis(preemphasis);
			if (STE_STATUS_OK == status)
				ret_val = 0;
			break;
		}
	case V4L2_CID_TUNE_POWER_LEVEL:
		{
			FM_DEBUG_REPORT("vidioc_set_ext_ctrls: "
					"V4L2_CID_TUNE_POWER_LEVEL, "
					"Value = %d",
					ext_ctrl->controls->value);
			if (ext_ctrl->controls->value <= 88 &&
			    ext_ctrl->controls->value > 123) {
				FM_ERR_REPORT("Invalid Power Level = %02x",
					      ext_ctrl->controls->value);
			} else {
				status =
				    ste_fm_tx_set_power_level(ext_ctrl->
							      controls->value);
				if (STE_STATUS_OK == status)
					ret_val = 0;
			}
			break;
		}
	default:
		{
			FM_ERR_REPORT("vidioc_set_ext_ctrls: "
				      "Unsupported Id = %04x",
				      ext_ctrl->controls->id);
			ret_val = -EINVAL;
		}
	}
err:
	return ret_val;
}

/**
 * vidioc_set_hw_freq_seek()- seek Up/Down Frequency.
 * This function is used to start seek
 * on the FM Radio. Direction if seek is as inicated by the parameter
 * inside the v4l2_hw_freq_seek structure. This function is called when the
 * application issues the IOCTL VIDIOC_S_HW_FREQ_SEEK
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @freq_seek: v4l2_hw_freq_seek structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_set_hw_freq_seek(struct file *file,
				   void *priv,
				   struct v4l2_hw_freq_seek *freq_seek)
{
	u8 status;

	FM_DEBUG_REPORT("vidioc_set_hw_freq_seek");
	FM_DEBUG_REPORT("vidioc_set_hw_freq_seek: Status = %d, "
			"Upwards = %d, Wrap Around = %d",
			cg2900_device.v_seekstatus,
			freq_seek->seek_upward, freq_seek->wrap_around);
	if (cg2900_device.v_seekstatus == FMR_SEEK_IN_PROGRESS) {
		FM_DEBUG_REPORT("vidioc_set_hw_freq_seek: "
				"VIDIOC_S_HW_FREQ_SEEK, "
				"freq_seek in progress");
		return -EINVAL;
	}

	os_lock();
	global_event = STE_EVENT_NO_EVENT;
	no_of_scan_freq = 0;
	os_unlock();

	if (1 == freq_seek->seek_upward)
		status = ste_fm_search_up_freq();
	else if (0 == freq_seek->seek_upward)
		status = ste_fm_search_down_freq();
	else
		return -EINVAL;
	if (STE_STATUS_OK != status) {
		return -EINVAL;
	} else {
		cg2900_device.v_seekstatus = FMR_SEEK_IN_PROGRESS;
		return 0;
	}
}

/**
 * vidioc_get_audio()- Get Audio features of FM Driver.
 * This function is used to get the audio features of FM Driver.
 * This function is imlemented as a dumy function.
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @audio: (out) v4l2_audio structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_get_audio(struct file *file,
			    void *priv, struct v4l2_audio *audio)
{
	FM_DEBUG_REPORT("vidioc_get_audio");

	strcpy(audio->name, "");
	audio->capability = 0;
	audio->mode = 0;

	return 0;
}

/**
 * vidioc_set_audio()- Set Audio features of FM Driver.
 * This function is used to set the audio features of FM Driver.
 * This function is imlemented as a dumy function.
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @audio: v4l2_audio structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_set_audio(struct file *file,
			    void *priv, struct v4l2_audio *audio)
{
	FM_DEBUG_REPORT("vidioc_set_audio");
	if (audio->index != 0)
		return -EINVAL;
	return 0;
}

/**
 * vidioc_get_input()- Get the Input Value
 * This function is used to get the Input.
 * This function is imlemented as a dumy function.
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @input: (out) Value to be stored.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_get_input(struct file *file, void *priv, unsigned int *input)
{
	FM_DEBUG_REPORT("vidioc_get_input");
	*input = 0;
	return 0;
}

/**
 * vidioc_set_input()- Set the input value.
 * This function is used to set input.
 * This function is imlemented as a dumy function.
 *
 * @file: File structure.
 * @priv: Previous data of file structure.
 * @input: Value to set
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int vidioc_set_input(struct file *file, void *priv, unsigned int input)
{
	FM_DEBUG_REPORT("vidioc_set_input");
	if (input != 0)
		return -EINVAL;
	return 0;
}

/**
 * cg2900_convert_err_to_v4l2()- Convert Error Bits to V4L2 RDS format.
 * This function converts the error bits in RDS Block
 * as received from Chip into V4L2 RDS data specification.
 *
 * @status_byte: The status byte as received in RDS Group for
 * particular RDS Block
 * @out_byte: byte to store the modified byte with the err bits
 * alligned as per V4L2 RDS Specifications.
 */
static void cg2900_convert_err_to_v4l2(char status_byte, char *out_byte)
{
	if ((status_byte & RDS_ERROR_STATUS_MASK) == RDS_ERROR_STATUS_MASK) {
		/* Uncorrectable Block */
		*out_byte = (*out_byte | V4L2_RDS_BLOCK_ERROR);
	} else if (((status_byte & RDS_UPTO_TWO_BITS_CORRECTED)
		    == RDS_UPTO_TWO_BITS_CORRECTED) ||
		   ((status_byte & RDS_UPTO_FIVE_BITS_CORRECTED)
		    == RDS_UPTO_FIVE_BITS_CORRECTED)) {
		/* Corrected Bits in Block */
		*out_byte = (*out_byte | V4L2_RDS_BLOCK_CORRECTED);
	}
}

/**
 * cg2900_open()- This function nitializes and switches on FM.
 * This is called when the application opens the character device.
 *
 * @file: File structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int cg2900_open(struct file *file)
{
	u8 status;
	int ret_val = -EINVAL;
	struct video_device *vdev = video_devdata(file);

	lock_kernel();
	users++;
	FM_DEBUG_REPORT("cg2900_open: users = %d", users);

	if (users > 1) {
		FM_INFO_REPORT("cg2900_open: FM already switched on!!!");
		unlock_kernel();
		return 0;
	} else {
		status = ste_fm_init();
		if (STE_STATUS_OK == status) {
			FM_DEBUG_REPORT("cg2900_open: Switching on FM");
			status = ste_fm_switch_on(&(vdev->dev));
			if (STE_STATUS_OK == status) {
				cg2900_device.v_state = FMR_SWITCH_ON;
				cg2900_device.v_frequency =
				    HZ_TO_V4L2(freq_low);
				cg2900_device.v_rx_rds_enabled = STE_FALSE;
				cg2900_device.v_muted = STE_FALSE;
				cg2900_device.v_audiopath = 0;
				cg2900_device.v_seekstatus = FMR_SEEK_NONE;
				cg2900_device.v_rssi_threshold
				    = STE_FM_DEFAULT_RSSI_THRESHOLD;
				global_event = STE_EVENT_NO_EVENT;
				no_of_scan_freq = 0;
				cg2900_device.v_fm_mode = STE_FM_IDLE_MODE;
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
 * cg2900_release()- This function switches off FM.
 * This function switches off FM and releases the resources.
 * This is called when the application closes the character
 * device.
 *
 * @file: File structure.
 *
 * Returns:
 *   0 when no error
 *   -EINVAL: otherwise
 */
static int cg2900_release(struct file *file)
{
	u8 status;
	int ret_val = -EINVAL;

	if (users > 0) {
		users--;
		FM_DEBUG_REPORT("cg2900_release: users = %d", users);
	} else {
		FM_ERR_REPORT("cg2900_release: No users registered "
			      "with FM Driver");
		return ret_val;
	}

	if (0 == users) {
		FM_DEBUG_REPORT("cg2900_release: Switching Off FM");
		status = ste_fm_switch_off();
		status = ste_fm_deinit();
		if (STE_STATUS_OK == status) {
			cg2900_device.v_state = FMR_SWITCH_OFF;
			cg2900_device.v_frequency = 0;
			cg2900_device.v_rx_rds_enabled = STE_FALSE;
			cg2900_device.v_muted = STE_FALSE;
			cg2900_device.v_seekstatus = FMR_SEEK_NONE;
			global_event = STE_EVENT_NO_EVENT;
			no_of_scan_freq = 0;
			ret_val = 0;
		}
	}
	return ret_val;
}

/**
 * cg2900_read()- This function is invoked when the application
 * calls read() to receive RDS Data.
 *
 * @file: File structure.
 * @data: buffer provided by application for receving the data.
 * @count: Number of bytes that application wants to read from driver
 * @pos: offset
 *
 * Returns:
 *   Number of bytes copied to the user buffer
 *   -EFAULT: If there is problem in copying data to buffer supplied
 *    by application
 *   -EIO: If the number of bytes to be read are not a multiple of
 *    struct  v4l2_rds_data.
 *   -EAGAIN: More than 22 blocks requested to be read or read
 *    was called in non blocking mode and no data was available for reading.
 *   -EINTR: If read was interrupted by a signal before data was avaialble.
 *   0 when no data available for reading.
 */
static ssize_t cg2900_read(struct file *file,
			   char __user *data, size_t count, loff_t *pos)
{
	int current_rds_grp;
	int index = 0;
	int blocks_to_read;
	int ret;
	struct v4l2_rds_data rdsbuf[MAX_RDS_GROUPS * NUM_OF_RDS_BLOCKS];
	struct v4l2_rds_data *rdslocalbuf = rdsbuf;

	FM_DEBUG_REPORT("cg2900_read");

	blocks_to_read = (count / sizeof(struct v4l2_rds_data));

	if (count % sizeof(struct v4l2_rds_data) != 0) {
		FM_ERR_REPORT("cg2900_read: Invalid Number of bytes %d "
			      "requested to read", count);
		return -EIO;
	}
	if (blocks_to_read > MAX_RDS_GROUPS * NUM_OF_RDS_BLOCKS) {
		FM_ERR_REPORT("cg2900_read: Too many blocks(%d) "
			      "requested to be read", blocks_to_read);
		return -EAGAIN;
	}

	if (file->f_flags & O_NONBLOCK) {
		/* Non blocking mode selected by application */
		if (ste_fm_rds_info.rds_head == ste_fm_rds_info.rds_tail) {
			/* Check if data is available, else return */
			FM_DEBUG_REPORT("cg2900_read: Non Blocking mode "
					"selected by application, returning as "
					"no RDS data is available");
			return -EAGAIN;
		}
	} else if ((cg2900_device.v_rx_rds_enabled) &&
		   (ste_fm_rds_info.rds_head == ste_fm_rds_info.rds_tail)) {
		/* Blocking mode selected by application
		 * if data is not available block on read queue */
		FM_DEBUG_REPORT("cg2900_read: Blocking mode "
				"selected by application, waiting till "
				"rds data is available");
		cg2900_device.v_waitonreadqueue = true;
		ret = wait_event_interruptible(cg2900_read_queue,
					       (ste_fm_rds_info.rds_head !=
						ste_fm_rds_info.rds_tail));
		cg2900_device.v_waitonreadqueue = false;
		FM_DEBUG_REPORT("cg2900_read: "
				"wait_event_interruptible returned = %d", ret);
		if (ret == -ERESTARTSYS)
			return -EINTR;
	}

	current_rds_grp = ste_fm_rds_info.rds_group_sent;
	if ((cg2900_device.v_rx_rds_enabled) &&
	    (ste_fm_rds_info.rds_head != ste_fm_rds_info.rds_tail) &&
	    (ste_fm_rds_buf[ste_fm_rds_info.rds_tail]
	     [current_rds_grp].block1 != 0x0000)) {
		os_lock();
		while (index < blocks_to_read) {
			/* Check which Block needs to be transferred next */
			switch (ste_fm_rds_info.rds_block_sent %
				NUM_OF_RDS_BLOCKS) {
			case 0:
				(rdslocalbuf + index)->lsb =
				    ste_fm_rds_buf[ste_fm_rds_info.rds_tail]
				    [current_rds_grp].block1;
				(rdslocalbuf + index)->msb =
				    ste_fm_rds_buf[ste_fm_rds_info.rds_tail]
				    [current_rds_grp].block1 >> 8;
				(rdslocalbuf + index)->block =
				    (ste_fm_rds_buf[ste_fm_rds_info.rds_tail]
				     [current_rds_grp].status1
				     & RDS_BLOCK_MASK) >> 2;
				cg2900_convert_err_to_v4l2(ste_fm_rds_buf
							   [ste_fm_rds_info.
							    rds_tail]
							   [current_rds_grp].
							   status1,
							   &(rdslocalbuf +
							     index)->block);
				break;
			case 1:
				(rdslocalbuf + index)->lsb =
				    ste_fm_rds_buf[ste_fm_rds_info.rds_tail]
				    [current_rds_grp].block2;
				(rdslocalbuf + index)->msb =
				    ste_fm_rds_buf[ste_fm_rds_info.rds_tail]
				    [current_rds_grp].block2 >> 8;
				(rdslocalbuf + index)->block =
				    (ste_fm_rds_buf[ste_fm_rds_info.rds_tail]
				     [current_rds_grp].status2
				     & RDS_BLOCK_MASK) >> 2;
				cg2900_convert_err_to_v4l2(ste_fm_rds_buf
							   [ste_fm_rds_info.
							    rds_tail]
							   [current_rds_grp].
							   status2,
							   &(rdslocalbuf +
							     index)->block);
				break;
			case 2:
				(rdslocalbuf + index)->lsb =
				    ste_fm_rds_buf[ste_fm_rds_info.rds_tail]
				    [current_rds_grp].block3;
				(rdslocalbuf + index)->msb =
				    ste_fm_rds_buf[ste_fm_rds_info.rds_tail]
				    [current_rds_grp].block3 >> 8;
				(rdslocalbuf + index)->block =
				    (ste_fm_rds_buf[ste_fm_rds_info.rds_tail]
				     [current_rds_grp].status3
				     & RDS_BLOCK_MASK) >> 2;
				cg2900_convert_err_to_v4l2(ste_fm_rds_buf
							   [ste_fm_rds_info.
							    rds_tail]
							   [current_rds_grp].
							   status3,
							   &(rdslocalbuf +
							     index)->block);
				break;
			case 3:
				(rdslocalbuf + index)->lsb =
				    ste_fm_rds_buf[ste_fm_rds_info.rds_tail]
				    [current_rds_grp].block4;
				(rdslocalbuf + index)->msb =
				    ste_fm_rds_buf[ste_fm_rds_info.rds_tail]
				    [current_rds_grp].block4 >> 8;
				(rdslocalbuf + index)->block =
				    (ste_fm_rds_buf[ste_fm_rds_info.rds_tail]
				     [current_rds_grp].status4
				     & RDS_BLOCK_MASK) >> 2;
				cg2900_convert_err_to_v4l2(ste_fm_rds_buf
							   [ste_fm_rds_info.
							    rds_tail]
							   [current_rds_grp].
							   status4,
							   &(rdslocalbuf +
							     index)->block);
				current_rds_grp++;
				if (current_rds_grp == MAX_RDS_GROUPS) {
					ste_fm_rds_info.rds_tail++;
					current_rds_grp = 0;
				}
				break;
			default:
				FM_ERR_REPORT("Invalid RDS Group!!!");
				os_unlock();
				return 0;
			}
			/* FM_DEBUG_REPORT("%02x%02x %02x ",
			   (rdslocalbuf + index)->msb, \
			   (rdslocalbuf + index)->lsb, \
			   (rdslocalbuf + index)->block); */
			index++;
			ste_fm_rds_info.rds_block_sent++;
			if (ste_fm_rds_info.rds_block_sent == NUM_OF_RDS_BLOCKS)
				ste_fm_rds_info.rds_block_sent = 0;
		}
		/* Update the RDS Group Count Sent to Application */
		ste_fm_rds_info.rds_group_sent = current_rds_grp;
		if (ste_fm_rds_info.rds_tail == MAX_RDS_BUFFER)
			ste_fm_rds_info.rds_tail = 0;

		if (copy_to_user(data, rdslocalbuf, count)) {
			os_unlock();
			FM_ERR_REPORT("cg2900_read: Error "
				      "in copying, returning");
			return -EFAULT;
		}
		os_unlock();
		return count;
	} else {
		FM_INFO_REPORT("cg2900_read: returning 0");
		return 0;
	}
}

/**
 * cg2900_poll()- Check if the operation is complete or not.
 * This function is invoked by application on calling poll() and is used to
 * wait till the desired operation seek/Band Scan are complete.
 * Driver blocks till the seek/BandScan Operation is complete. The application
 * decides to read the results of seek/Band Scan based on the returned value of
 * this function.
 *
 * @file: File structure.
 * @wait: poll table
 *
 * Returns:
 *   POLLRDBAND when Scan Band is complete and data is available for reading.
 *   POLLRDNORM when seek station is complete.
 *   POLLHUP when seek station/Band Scan is stopped by user using Stop Scan.
 */

static unsigned int cg2900_poll(struct file *file,
				struct poll_table_struct *wait)
{

	FM_DEBUG_REPORT("cg2900_poll");

	poll_wait(file, &cg2900_poll_queue, wait);
	if (cg2900_device.v_seekstatus == FMR_SEEK_IN_PROGRESS) {
		if ((global_event == STE_EVENT_SCAN_CHANNELS_FOUND) ||
		    (global_event == STE_EVENT_BLOCK_SCAN_CHANNELS_FOUND)) {
			/* Scan Completed, send event to application */
			FM_DEBUG_REPORT("poll_wait returning POLLRDBAND");
			return POLLRDBAND;
		} else if (global_event == STE_EVENT_SEARCH_CHANNEL_FOUND) {
			FM_DEBUG_REPORT("cg2900_poll: " "returning POLLRDNORM");
			FM_DEBUG_REPORT("poll_wait returning POLLRDNORM");
			return POLLRDNORM;
		} else if (global_event == STE_EVENT_SCAN_CANCELLED) {
			/* Scan/Search cancelled by User */
			os_lock();
			no_of_scan_freq = 0;
			no_of_block_scan_freq = 0;
			os_unlock();
			cg2900_device.v_seekstatus = FMR_SEEK_NONE;
			global_event = STE_EVENT_NO_EVENT;
			FM_DEBUG_REPORT("cg2900_poll: "
					"Cancel operation "
					"returning POLLHUP");
			FM_DEBUG_REPORT("poll_wait returning POLLHUP");
			return POLLHUP;
		}
	}
	FM_DEBUG_REPORT("poll_wait returning 0");
	return 0;

}

/**
 * cg2900_init()- This function initializes the FM Driver.
 * This Fucntion is called whenever the Driver is loaded from
 * Video4Linux driver. It registers the FM Driver with Video4Linux
 * as a character device.
 *
 * Returns:
 *   0 on success
 * -EINVAL on error
 */
static int __init cg2900_init(void)
{
	FM_INFO_REPORT(BANNER);
	radio_nr = 0;
	grid = 1;
	band = 0;
	FM_DEBUG_REPORT("cg2900_init: radio_nr= %d.", radio_nr);
	/* Initialize the parameters */
	if (video_register_device(&cg2900_video_device,
				  VFL_TYPE_RADIO, radio_nr) == -1) {
		FM_ERR_REPORT("cg2900_init: video_register_device err");
		return -EINVAL;
	}
	init_waitqueue_head(&cg2900_poll_queue);
	users = 0;
	return 0;
}

/**
 * cg2900_deinit()- This function deinitializes the FM Driver.
 * This Fucntion is called whenever the Driver is unloaded from
 * VideoForLinux driver.
 */
static void __exit cg2900_exit(void)
{
	FM_INFO_REPORT("cg2900_exit");

	/* Try to Switch Off FM in case it is still switched on */
	ste_fm_switch_off();
	ste_fm_deinit();
	video_unregister_device(&cg2900_video_device);
}

/* ------------------ External Function definitions ---------------------- */
void wake_up_poll_queue(void)
{
	FM_DEBUG_REPORT("wake_up_poll_queue");
	wake_up_interruptible(&cg2900_poll_queue);
}

void wake_up_read_queue(void)
{
	FM_DEBUG_REPORT("wake_up_read_queue");
	if (cg2900_device.v_waitonreadqueue)
		wake_up_interruptible(&cg2900_read_queue);
}

module_init(cg2900_init);
module_exit(cg2900_exit);
MODULE_AUTHOR("Hemant Gupta");
MODULE_LICENSE("GPL v2");

module_param(radio_nr, int, S_IRUGO);

module_param(grid, int, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(grid, "Grid:" "0=50 kHz" "*1=100 kHz*" "2=200 kHz");

module_param(band, int, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(band, "Band:" "*0=87.5-108 MHz*" "1=76-90 MHz" "2=70-108 MHz");
