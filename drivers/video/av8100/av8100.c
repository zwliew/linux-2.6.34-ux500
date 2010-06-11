/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * AV8100 driver
 *
 * Author: Per Persson <per.xb.persson@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/timer.h>
#include <linux/mutex.h>

#include "av8100_regs.h"
#include <video/av8100.h>
#include <video/hdmi.h>
#include "av8100_fw.h"

#define AV8100_INT_EVENT 0x1
#define AV8100_TIMER_INT_EVENT 0x2

#define AV8100_TIMER_INTERRUPT_POLLING_TIME 250

#define AV8100_DRIVER_MINOR_NUMBER	240
#define GPIO_AV8100_RSTN		196
#define AV8100_MASTER_CLOCK_TIMING	0x3
#define AV8100_ON_TIME			1
#define AV8100_OFF_TIME			0
#define AV8100_COMMAND_OFFSET		0x10
#define AV8100_COMMAND_MAX_LENGTH	0x81

#define AV8100_TE_LINE_NB_14	14
#define AV8100_TE_LINE_NB_17	17
#define AV8100_TE_LINE_NB_18	18
#define AV8100_TE_LINE_NB_21	21
#define AV8100_TE_LINE_NB_22	22
#define AV8100_TE_LINE_NB_30	30
#define AV8100_TE_LINE_NB_38	38
#define AV8100_TE_LINE_NB_40	40
#define AV8100_UI_X4_DEFAULT	6

#define HDMI_REQUEST_FOR_REVOCATION_LIST_INPUT 2
#define HDMI_CEC_MESSAGE_WRITE_BUFFER_SIZE 16
#define HDMI_HDCP_SEND_KEY_SIZE 7
#define HDMI_INFOFRAME_DATA_SIZE 28
#define HDMI_CEC_MESSAGE_READBACK_MAXSIZE 16
#define HDMI_FUSE_AES_KEY_SIZE 16

#define REG_16_8_LSB(p)		(u8)(p & 0xFF)
#define REG_16_8_MSB(p)		(u8)((p & 0xFF00)>>8)
#define REG_32_8_MSB(p)		(u8)((p & 0xFF000000)>>24)
#define REG_32_8_MMSB(p)	(u8)((p & 0x00FF0000)>>16)
#define REG_32_8_MLSB(p)	(u8)((p & 0x0000FF00)>>8)
#define REG_32_8_LSB(p)		(u8)(p & 0x000000FF)
#define REG_10_8_MSB(p)		(u8)((p & 0x300)>>8)


struct av8100_config_t {
	struct i2c_client *client;
	struct i2c_device_id *id;
	struct av8100_video_input_format_cmd hdmi_video_input_cmd;
	struct av8100_audio_input_format_cmd hdmi_audio_input_cmd;
	struct av8100_video_output_format_cmd hdmi_video_output_cmd;
	struct av8100_video_scaling_format_cmd hdmi_video_scaling_cmd;
	struct av8100_color_space_conversion_format_cmd
					hdmi_color_space_conversion_cmd;
	struct av8100_cec_message_write_format_cmd
		hdmi_cec_message_write_cmd;
	struct av8100_cec_message_read_back_format_cmd
		hdmi_cec_message_read_back_cmd;
	struct av8100_denc_format_cmd hdmi_denc_cmd;
	struct av8100_hdmi_cmd hdmi_cmd;
	struct av8100_hdcp_send_key_format_cmd hdmi_hdcp_send_key_cmd;
	struct av8100_hdcp_management_format_cmd
		hdmi_hdcp_management_format_cmd;
	struct av8100_infoframes_format_cmd	hdmi_infoframes_cmd;
	struct av8100_edid_section_readback_format_cmd
		hdmi_edid_section_readback_cmd;
	struct av8100_pattern_generator_format_cmd hdmi_pattern_generator_cmd;
	struct av8100_fuse_aes_key_format_cmd hdmi_fuse_aes_key_cmd;
};

/**
 * struct av8100_cea - CEA(consumer electronic access) standard structure
 * @cea_id:
 * @cea_nb:
 * @vtotale:
 **/

struct av8100_cea {
	char cea_id[40];
	int cea_nb;
	int vtotale;
	int vactive;
	int vsbp;
	int vslen;
	int vsfp;
	char vpol[5];
	int htotale;
	int hactive;
	int hbp;
	int hslen;
	int hfp;
	int frequence;
	char hpol[5];
	int reg_line_duration;
	int blkoel_duration;
	int uix4;
	int pll_mult;
	int pll_div;
};

enum av8100_command_size {
	AV8100_COMMAND_VIDEO_INPUT_FORMAT_SIZE  = 0x17,
	AV8100_COMMAND_AUDIO_INPUT_FORMAT_SIZE  = 0x8,
	AV8100_COMMAND_VIDEO_OUTPUT_FORMAT_SIZE = 0x1E,
	AV8100_COMMAND_VIDEO_SCALING_FORMAT_SIZE = 0x11,
	AV8100_COMMAND_COLORSPACECONVERSION_SIZE = 0x1D,
	AV8100_COMMAND_CEC_MESSAGE_WRITE_SIZE = 0x12,
	AV8100_COMMAND_CEC_MESSAGE_READ_BACK_SIZE = 0x1,
	AV8100_COMMAND_DENC_SIZE = 0x6,
	AV8100_COMMAND_HDMI_SIZE = 0x4,
	AV8100_COMMAND_HDCP_SENDKEY_SIZE = 0xA,
	AV8100_COMMAND_HDCP_MANAGEMENT_SIZE = 0x4,
	AV8100_COMMAND_INFOFRAMES_SIZE = 0x21,
	AV8100_COMMAND_EDID_SECTION_READBACK_SIZE  = 0x3,
	AV8100_COMMAND_PATTERNGENERATOR_SIZE  = 0x4,
	AV8100_COMMAND_FUSE_AES_KEY_SIZE = 0x12,
};

DEFINE_MUTEX(av8100_hw_mutex);
#define LOCK_AV8100_HW mutex_lock(&av8100_hw_mutex)
#define UNLOCK_AV8100_HW mutex_unlock(&av8100_hw_mutex)

#define AV8100_DEBUG_EXTRA
#define AV8100_PLUGIN_DETECT_VIA_TIMER_INTERRUPTS

static int av8100_open(struct inode *inode, struct file *filp);
static int av8100_release(struct inode *inode, struct file *filp);
static int av8100_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg);
static int __devinit av8100_probe(struct i2c_client *i2cClient,
	const struct i2c_device_id *id);
static int __devexit av8100_remove(struct i2c_client *i2cClient);

static struct av8100_config_t *av8100_config;
static struct av8100_status	g_av8100_status = {0};
#ifdef AV8100_PLUGIN_DETECT_VIA_TIMER_INTERRUPTS
static struct timer_list av8100_timer;
#endif
static wait_queue_head_t av8100_event;
static int av8100_flag = 0x0;

static const struct file_operations av8100_fops = {
	.owner =    THIS_MODULE,
	.open =     av8100_open,
	.release =  av8100_release,
	.ioctl = av8100_ioctl
};

static struct miscdevice av8100_miscdev = {
	AV8100_DRIVER_MINOR_NUMBER,
	"av8100",
	&av8100_fops
};

struct av8100_cea av8100_all_cea[29] = {
/* cea id
 *	cea_nr	vtot	vact	vsbpp	vslen
 *	vsfp	vpol	htot	hact	hbp	hslen	hfp	freq
 * 	hpol	rld	bd	uix4	pm	pd */
{ "0  CUSTOM                            ",
	0,	0,	0,	0,	0,
	0,	"-",	800,	640,	16,	96,	10,	25200000,
	"-",	0,	0,	0,	0,	0},/*Settings to be defined*/
{ "1  CEA 1 VESA 4 640x480p @ 60 Hz     ",
	1,	525,	480,	33,	2,
	10,	"-",	800,	640,	49,	290,	146,	25200000,
	"-",	2438,	1270,	6,	32,	1},/*RGB888*/
{ "2  CEA 2 - 3    720x480p @ 60 Hz 4:3 ",
	2,	525,	480,	30,	6,
	9,	"-",	858,	720,	34,	130,	128,	27027000,
	"-",	1828,	0x3C0,	8,	24,	1},/*RGB565*/
{ "3  CEA 4        1280x720p @ 60 Hz    ",
	4,	750,	720,	20,	5,
	5,	"+",	1650,	1280,	114,	39,	228,	74250000,
	"+",	1706,	164,	6,	32,	1},/*RGB565*/
{ "4  CEA 5        1920x1080i @ 60 Hz   ",
	5,	1125,	540,	20,	5,
	0,	"+",	2200,	1920,	88,	44,	10,	74250000,
	"+",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "5  CEA 6-7      480i (NTSC)          ",
	6,	525,	240,	44,	5,
	0,	"-",	858,	720,	12,	64,	10,	13513513,
	"-",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "6  CEA 14-15    480p @ 60 Hz         ",
	14,	525,	480,	44,	5,
	0,	"-",	858,	720,	12,	64,	10,	27027000,
	"-",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "7  CEA 16       1920x1080p @ 60 Hz   ",
	16,	1125,	1080,	36,	5,
	0,	"+",	1980,	1280,	440,	40,	10,	133650000,
	"+",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "8  CEA 17-18    720x576p @ 50 Hz     ",
	17,	625,	576,	44,	5,
	0,	"-",	864,	720,	12,	64,	10,	27000000,
	"-",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "9  CEA 19       1280x720p @ 50 Hz    ",
	19,	750,	720,	25,	5,
	0,	"+",	1980,	1280,	440,	40,	10,	74250000,
	"+",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "10 CEA 20       1920 x 1080i @ 50 Hz ",
	20,	1125,	540,	20,	5,
	0,	"+",	2640,	1920,	528,	44,	10,	74250000,
	"+",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "11 CEA 21-22    576i (PAL)           ",
	21,	625,	288,	44,	5,
	0,	"-",	1728,	1440,	12,	64,	10,	27000000,
	"-",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "12 CEA 29/30    576p                 ",
	29,	625,	576,	44,	5,
	0,	"-",	864,	720,	12,	64,	10,	27000000,
	"-",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "13 CEA 31       1080p 50Hz           ",
	31,	1125,	1080,	44,	5,
	0,	"-",	2640,	1920,	12,	64,	10,	148500000,
	"-",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "14 CEA 32       1920x1080p @ 24 Hz   ",
	32,	1125,	1080,	36,	5,
	4,	"+",	2750,	1920,	660,	44,	153,	74250000,
	"+",	2844,	0x530,	6,	32,	1},/*RGB565*/
{ "15 CEA 33       1920x1080p @ 25 Hz   ",
	33,	1125,	1080,	36,	5,
	4,	"+",	2640,	1920,	528,	44,	10,	74250000,
	"+",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "16 CEA 34       1920x1080p @ 30Hz    ",
	34,	1125,	1080,	36,	5,
	4,	"+",	2200,	1920,	91,	44,	153,	74250000,
	"+",	2275,	0xAB,	6,	32,	1},/*RGB565*/
{ "17 CEA 60       1280x720p @ 24 Hz    ",
	60,	750,	720,	20,	5,
	5,	"+",	3300,	1280,	284,	50,	2276,	59400000,
	"+",	4266,	0xAD0,	5,	32,	1},/*RGB565*/
{ "18 CEA 61       1280x720p @ 25 Hz    ",
	61,	750,	720,	20,	5,
	5,	"+",	3960,	1280,	228,	39,	2503,	74250000,
	"+",	4096,	0x500,	5,	32,	1},/*RGB565*/
{ "19 CEA 62       1280x720p @ 30 Hz    ",
	62,	750,	720,	20,	5,
	5,	"+",	3300,	1280,	228,	39,	1820,	74250000,
	"+",	3413,	0x770,	5,	32,	1},/*RGB565*/
{ "20 VESA 9       800x600 @ 60 Hz      ",
	109,	628,	600,	28,	4,
	0,	"+",	1056,	800,	40,	128,	10,	20782080,
	"+",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "21 VESA 14      848x480  @ 60 Hz     ",
	114,	500,	480,	20,	5,
	0,	"+",	1056,	848,	24,	80,	10,	31680000,
	"-",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "22 VESA 16      1024x768 @ 60 Hz     ",
	116,	806,	768,	38,	6,
	0,	"-",	1344,	1024,	24,	135,	10,	65000000,
	"-",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "23 VESA 22      1280x768 @ 60 Hz     ",
	122,	802,	768,	34,	4,
	0,	"+",	1688,	1280,	48,	160,	10,	81250000,
	"-",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "24 VESA 23      1280x768 @ 60 Hz     ",
	123,	798,	768,	30,	7,
	0,	"+",	1664,	1280,	64,	128,	10,	79500000,
	"-",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "25 VESA 27      1280x800 @ 60 Hz     ",
	127,	823,	800,	23,	6,
	0,	"+",	1440,	1280,	48,	32,	10,	71000000,
	"+",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "26 VESA 28      1280x800 @ 60 Hz     ",
	128,	831,	800,	31,	6,
	0,	"+",	1680,	1280,	72,	128,	10,	83500000,
	"-",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "27 VESA 39      1360x768 @ 60 Hz     ",
	139,	790,	768,	22,	5,
	0,	"-",	1520,	1360,	48,	32,	10,	72000000,
	"+",	0,	0,	0,	0,	0},/*Settings to be define*/
{ "28 VESA 81      1360x768 @ 60 Hz     ",
	181,	798,	768,	30,	5,
	0,	"+",	1776,	1360,	72,	136,	10,	84750000,
	"-",	0,	0,	0,	0,	0} /*Settings to be define*/
};


static const struct i2c_device_id av8100_id[] = {
	{ "av8100", 0 },
	{ }
};

static struct i2c_driver av8100_driver = {
	.probe	= av8100_probe,
	.remove = av8100_remove,
	.driver = {
		.name	= "av8100",
	},
	.id_table	= av8100_id,
};

#ifdef AV8100_PLUGIN_DETECT_VIA_TIMER_INTERRUPTS
static void av8100_timer_int(unsigned long value)
{
	av8100_flag |= AV8100_TIMER_INT_EVENT;
	wake_up_interruptible(&av8100_event);

	if (g_av8100_status.av8100_state >= AV8100_OPMODE_STANDBY) {
		av8100_timer.expires = jiffies +
			AV8100_TIMER_INTERRUPT_POLLING_TIME;
		add_timer(&av8100_timer);
	}
}
#endif

static int av8100_thread(void *p)
{
	u8 cpd = 0;
	u8 stby = 0;
	u8 hpds = 0;
	u8 cpds = 0;
	u8 mclkrng = 0;
	int ret = 0;
#ifdef AV8100_PLUGIN_DETECT_VIA_TIMER_INTERRUPTS
	u8 hpds_old = 0xf;
	u8 cpds_old = 0xf;
#else
	u8 sid = 0;
	u8 oni = 0;
	u8 hpdi = 0;
	u8 cpdi = 0;
#endif

	while (1) {
		wait_event_interruptible(av8100_event, (av8100_flag != 0));
#ifdef AV8100_PLUGIN_DETECT_VIA_TIMER_INTERRUPTS
		if (av8100_flag & AV8100_TIMER_INT_EVENT) {
			if (g_av8100_status.av8100_state >=
				AV8100_OPMODE_STANDBY) {
				/* STANDBY */
				ret = av8100_register_standby_read(
					&cpd,
					&stby,
					&hpds,
					&cpds,
					&mclkrng);
				if (ret != 0)
					printk(KERN_DEBUG "av8100_register_"
						"standby_read fail\n");

				/* TVout plugin change */
				if ((cpds == 1) && (cpds != cpds_old)) {
					cpds_old = 1;

					g_av8100_status.av8100_plugin_status |=
						AV8100_CVBS_PLUGIN;

					/* TODO: notify */
					printk(KERN_DEBUG "TVout plugin detected\n");
				} else if ((cpds == 0) && (cpds != cpds_old)) {
					cpds_old = 0;

					g_av8100_status.av8100_plugin_status &=
						~AV8100_CVBS_PLUGIN;

					/* TODO: notify */
					printk(KERN_DEBUG "TVout plugout detected\n");
				}

				/* HDMI plugin change */
				if ((hpds == 1) && (hpds != hpds_old)) {
					hpds_old = 1;

					g_av8100_status.av8100_plugin_status |=
						AV8100_HDMI_PLUGIN;

					/* TODO: notify */
					printk(KERN_DEBUG "HDMI plugin detected\n");
				} else if ((hpds == 0) && (hpds != hpds_old)) {
					hpds_old = 0;

					g_av8100_status.av8100_plugin_status &=
						~AV8100_HDMI_PLUGIN;

					/* TODO: notify */
					printk(KERN_DEBUG "HDMI plugout detected\n");
				}
			}
		}
#else
		/* STANDBY_PENDING_INTERRUPT */
		ret = av8100_register_standby_pending_interrupt_read(
			&hpdi,
			&cpdi,
			&oni,
			&sid);

		if (ret)
			printk(KERN_DEBUG "av8100_register_standby_"
				"pending_interrupt_read failed\n");

		if (hpdi | cpdi | oni) {
			/* STANDBY */
			ret = av8100_register_standby_read(
					&cpd,
					&stby,
					&hpds,
					&cpds,
					&mclkrng);
			if (ret)
				printk(KERN_DEBUG "av8100_register_standby_"
					"read fail\n");
		}

		if (cpdi) {
			/* TVout plugin change */
			if (cpds)
				g_av8100_status.av8100_plugin_status |=
					AV8100_CVBS_PLUGIN;
			else
				g_av8100_status.av8100_plugin_status &=
						~AV8100_CVBS_PLUGIN;

			/* TODO: notify */
			printk(KERN_DEBUG "TVout plugin: %d\n", (
				g_av8100_status.av8100_plugin_status &
				AV8100_CVBS_PLUGIN) == AV8100_CVBS_PLUGIN);
		} else if (hpdi) {
			/* HDMI plugin change */
			if (hpds)
				g_av8100_status.av8100_plugin_status |=
					AV8100_HDMI_PLUGIN;
			else
				g_av8100_status.av8100_plugin_status &=
					~AV8100_HDMI_PLUGIN;

			/* TODO: notify */
			printk(KERN_DEBUG "HDMI plugin: %d\n", (
				g_av8100_status.av8100_plugin_status &
				AV8100_HDMI_PLUGIN) == AV8100_HDMI_PLUGIN);
		}

		if (hpdi | cpdi | oni) {
			/* Clear pending interrupts */
			ret = av8100_register_standby_pending_interrupt_write(
					hpdi,
					cpdi,
					oni);
			if (ret)
				printk(KERN_DEBUG "av8100_register_standby_"
					"read fail\n");
		}
#endif
		av8100_flag = 0;
	}

	return ret;
}

static irqreturn_t av8100_intr_handler(int irq, void *p)
{
	av8100_flag |= AV8100_INT_EVENT;
	wake_up_interruptible(&av8100_event);
	return IRQ_HANDLED;
}

static u16 av8100_get_te_line_nb(
	enum av8100_output_CEA_VESA output_video_format)
{
	u16 retval;

	switch (output_video_format) {
	case AV8100_CEA1_640X480P_59_94HZ:
	case AV8100_CEA2_3_720X480P_59_94HZ:
		retval = AV8100_TE_LINE_NB_30;
		break;

	case AV8100_CEA5_1920X1080I_60HZ:
	case AV8100_CEA6_7_NTSC_60HZ:
	case AV8100_CEA20_1920X1080I_50HZ:
		retval = AV8100_TE_LINE_NB_18;
		break;

	case AV8100_CEA4_1280X720P_60HZ:
		retval = AV8100_TE_LINE_NB_21;
		break;

	case AV8100_CEA17_18_720X576P_50HZ:
		retval = AV8100_TE_LINE_NB_40;
		break;

	case AV8100_CEA19_1280X720P_50HZ:
		retval = AV8100_TE_LINE_NB_22;
		break;

	case AV8100_CEA21_22_576I_PAL_50HZ:
		/* Different values below come from LLD,
		 * TODO: check if this is really needed
		 * if not merge with AV8100_CEA6_7_NTSC_60HZ case
		 */
#ifdef CONFIG_AV8100_SDTV
		retval = AV8100_TE_LINE_NB_18;
#else
		retval = AV8100_TE_LINE_NB_17;
#endif
		break;

	case AV8100_CEA32_1920X1080P_24HZ:
	case AV8100_CEA33_1920X1080P_25HZ:
	case AV8100_CEA34_1920X1080P_30HZ:
		retval = AV8100_TE_LINE_NB_38;
		break;

	case AV8100_CEA60_1280X720P_24HZ:
	case AV8100_CEA62_1280X720P_30HZ:
		retval = AV8100_TE_LINE_NB_21;
		break;

	case AV8100_CEA14_15_480p_60HZ:
	case AV8100_VESA14_848X480P_60HZ:
	case AV8100_CEA61_1280X720P_25HZ:
	case AV8100_CEA16_1920X1080P_60HZ:
	case AV8100_CEA31_1920x1080P_50Hz:
	case AV8100_CEA29_30_576P_50HZ:
	case AV8100_VESA9_800X600P_60_32HZ:
	case AV8100_VESA16_1024X768P_60HZ:
	case AV8100_VESA22_1280X768P_59_99HZ:
	case AV8100_VESA23_1280X768P_59_87HZ:
	case AV8100_VESA27_1280X800P_59_91HZ:
	case AV8100_VESA28_1280X800P_59_81HZ:
	case AV8100_VESA39_1360X768P_60_02HZ:
	case AV8100_VESA81_1366X768P_59_79HZ:
	default:
		/* TODO */
		retval = AV8100_TE_LINE_NB_14;
		break;
	}

	return retval;
}

static u16 av8100_get_ui_x4(
	enum av8100_output_CEA_VESA output_video_format)
{
	return AV8100_UI_X4_DEFAULT;
}

static int av8100_config_video_output_dep(enum av8100_output_CEA_VESA
	output_format)
{
	int retval = 0;
	union av8100_configuration config;

	/* video input */
	config.video_input_format.dsi_input_mode =
		AV8100_HDMI_DSI_COMMAND_MODE;
	config.video_input_format.input_pixel_format = AV8100_INPUT_PIX_RGB565;
	config.video_input_format.total_horizontal_pixel =
		av8100_all_cea[output_format].htotale;
	config.video_input_format.total_horizontal_active_pixel =
		av8100_all_cea[output_format].hactive;
	config.video_input_format.total_vertical_lines =
		av8100_all_cea[output_format].vtotale;
	config.video_input_format.total_vertical_active_lines =
		av8100_all_cea[output_format].vactive;

	switch (output_format) {
	case AV8100_CEA5_1920X1080I_60HZ:
	case AV8100_CEA20_1920X1080I_50HZ:
	case AV8100_CEA21_22_576I_PAL_50HZ:
	case AV8100_CEA6_7_NTSC_60HZ:
		config.video_input_format.video_mode =
			AV8100_VIDEO_INTERLACE;
		break;

	default:
		config.video_input_format.video_mode =
			AV8100_VIDEO_PROGRESSIVE;
		break;
	}

	config.video_input_format.nb_data_lane =
		AV8100_DATA_LANES_USED_2;
	config.video_input_format.nb_virtual_ch_command_mode = 0;
	config.video_input_format.nb_virtual_ch_video_mode = 0;
	config.video_input_format.ui_x4 = av8100_get_ui_x4(output_format);
	config.video_input_format.TE_line_nb = av8100_get_te_line_nb(
		output_format);
/* TODO: Dynamic test to detect AV8100 V1 or V2 */
#ifdef AV8100_HW_TE_I2SDAT3
	config.video_input_format.TE_config = AV8100_TE_GPIO_IT;
#else
	config.video_input_format.TE_config = AV8100_TE_IT_LINE;
#endif
	config.video_input_format.master_clock_freq = 0;

	retval = av8100_configuration_prepare(
		AV8100_COMMAND_VIDEO_INPUT_FORMAT, &config);

	/* DENC */
	switch (output_format) {
	case AV8100_CEA21_22_576I_PAL_50HZ:
		config.denc_format.cvbs_video_format = AV8100_CVBS_625;
		config.denc_format.standard_selection = AV8100_PAL_BDGHI;
		break;

	case AV8100_CEA6_7_NTSC_60HZ:
		config.denc_format.cvbs_video_format = AV8100_CVBS_525;
		config.denc_format.standard_selection = AV8100_NTSC_M;
		break;

	default:
		/* Not supported */
		break;
	}

	return retval;
}

static int av8100_config_init(void)
{
	int retval = 0;
	union av8100_configuration config;

	printk(KERN_DEBUG "%s\n", __func__);

	av8100_config = kzalloc(sizeof(struct av8100_config_t), GFP_KERNEL);
	if (!av8100_config) {
		pr_err("%s: Failed to allocate config\n", __func__);
		return AV8100_FAIL;
	}

	memset(&config, 0, sizeof(union av8100_configuration));
	memset(av8100_config, 0, sizeof(union av8100_configuration));

	/* Color conversion */
	/* TODO: Magic numbers. Move to platform data? */
	config.color_space_conversion_format.c0      = 0x0100;
	config.color_space_conversion_format.c1      = 0x0000;
	config.color_space_conversion_format.c2      = 0x0000;
	config.color_space_conversion_format.c3      = 0x0000;
	config.color_space_conversion_format.c4      = 0x0100;
	config.color_space_conversion_format.c5      = 0x0000;
	config.color_space_conversion_format.c6      = 0x0000;
	config.color_space_conversion_format.c7      = 0x0000;
	config.color_space_conversion_format.c8      = 0x0100;
	config.color_space_conversion_format.aoffset = 0x0000;
	config.color_space_conversion_format.boffset = 0x0000;
	config.color_space_conversion_format.coffset = 0x0000;
	config.color_space_conversion_format.lmax    = 0xff;
	config.color_space_conversion_format.lmin    = 0x00;
	config.color_space_conversion_format.cmax    = 0xff;
	config.color_space_conversion_format.cmin    = 0x00;
	retval = av8100_configuration_prepare(
		AV8100_COMMAND_COLORSPACECONVERSION, &config);
	if (retval)
		return AV8100_FAIL;

	/* DENC */
	config.denc_format.cvbs_video_format = AV8100_CVBS_625;
	config.denc_format.standard_selection = AV8100_PAL_BDGHI;
	config.denc_format.on_off = 0;
	config.denc_format.macrovision_on_off = 0;
	config.denc_format.internal_generator = 0;

	/* Video output */
	config.video_output_format.video_output_cea_vesa =
		AV8100_CEA4_1280X720P_60HZ;

	retval = av8100_configuration_prepare(
		AV8100_COMMAND_VIDEO_OUTPUT_FORMAT, &config);
	if (retval)
		return AV8100_FAIL;

	/* Video input */
	av8100_config_video_output_dep(
		config.video_output_format.video_output_cea_vesa);

	/* Pattern generator */
	config.pattern_generator_format.pattern_audio_mode =
		AV8100_PATTERN_AUDIO_OFF;
	config.pattern_generator_format.pattern_type =
		AV8100_PATTERN_GENERATOR;
	config.pattern_generator_format.pattern_video_format =
		AV8100_PATTERN_720P;
	retval = av8100_configuration_prepare(AV8100_COMMAND_PATTERNGENERATOR,
		&config);
	if (retval)
		return AV8100_FAIL;

	/* Audio input */
	config.audio_input_format.audio_input_if_format	=
		AV8100_AUDIO_I2SDELAYED_MODE;
	config.audio_input_format.i2s_input_nb = 1;
	config.audio_input_format.sample_audio_freq = AV8100_AUDIO_FREQ_48KHZ;
	config.audio_input_format.audio_word_lg = AV8100_AUDIO_16BITS;
	config.audio_input_format.audio_format = AV8100_AUDIO_LPCM_MODE;
	config.audio_input_format.audio_if_mode = AV8100_AUDIO_MASTER;
	config.audio_input_format.audio_mute = AV8100_AUDIO_MUTE_DISABLE;
	retval = av8100_configuration_prepare(
		AV8100_COMMAND_AUDIO_INPUT_FORMAT, &config);
	if (retval)
		return AV8100_FAIL;

	/* HDMI mode */
	config.hdmi_format.hdmi_mode	= AV8100_HDMI_ON;
	config.hdmi_format.hdmi_format	= AV8100_HDMI;
	config.hdmi_format.dvi_format	= AV8100_DVI_CTRL_CTL0;
	retval = av8100_configuration_prepare(AV8100_COMMAND_HDMI, &config);
	if (retval)
		return AV8100_FAIL;

	/* EDID section readback */
	config.edid_section_readback_format.address = 0xA0;
	config.edid_section_readback_format.block_number = 0;
	retval = av8100_configuration_prepare(
		AV8100_COMMAND_EDID_SECTION_READBACK, &config);
	if (retval)
		return AV8100_FAIL;

	return retval;
}

static void av8100_config_exit(void)
{
	printk(KERN_DEBUG "%s\n", __func__);

	kfree(av8100_config);
	av8100_config = NULL;
}

static void av8100_set_state(enum av8100_operating_mode state)
{
	g_av8100_status.av8100_state = state;
}

/**
 * write_single_byte() - Write a single byte to av8100
 * through i2c interface.
 * @client:	i2c client structure
 * @reg:	register offset
 * @data:	data byte to be written
 *
 * This funtion uses smbus byte write API to write a single byte to av8100
 **/
static int write_single_byte(struct i2c_client *client, u8 reg,
	u8 data)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, data);
	if (ret < 0)
		printk(KERN_DEBUG "i2c smbus write byte failed\n");

	return ret;
}

/**
 * read_single_byte() - read single byte from av8100
 * through i2c interface
 * @client:     i2c client structure
 * @reg:        register offset
 * @val:	register value
 *
 * This funtion uses smbus read block API to read single byte from the reg
 * offset.
 **/
static int read_single_byte(struct i2c_client *client, u8 reg, u8 *val)
{
	int value;

	value = i2c_smbus_read_byte_data(client, reg);
	if (value < 0) {
		printk(KERN_DEBUG "i2c smbus read byte failed,read data = %x "
			"from offset:%x\n" , value, reg);
		return AV8100_FAIL;
	}

	*val = (u8) value;
	return 0;
}

/**
 * write_multi_byte() - Write a multiple bytes to av8100 through
 * i2c interface.
 * @client:	i2c client structure
 * @buf:	buffer to be written
 * @nbytes:	nunmber of bytes to be written
 *
 * This funtion uses smbus block write API's to write n number of bytes to the
 * av8100
 **/
static int write_multi_byte(struct i2c_client *client, u8 reg,
		u8 *buf, u8 nbytes)
{
	int ret;

	ret = i2c_smbus_write_i2c_block_data(client, reg, nbytes, buf);
	if (ret < 0)
		printk(KERN_DEBUG "i2c smbus write multi byte error\n");

	return ret;
}

static int configuration_video_input_get(char *buffer,
	unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	buffer[0] = av8100_config->hdmi_video_input_cmd.dsi_input_mode;
	buffer[1] = av8100_config->hdmi_video_input_cmd.input_pixel_format;
	buffer[2] = REG_16_8_MSB(av8100_config->hdmi_video_input_cmd.
		total_horizontal_pixel);
	buffer[3] = REG_16_8_LSB(av8100_config->hdmi_video_input_cmd.
		total_horizontal_pixel);
	buffer[4] = REG_16_8_MSB(av8100_config->hdmi_video_input_cmd.
		total_horizontal_active_pixel);
	buffer[5] = REG_16_8_LSB(av8100_config->hdmi_video_input_cmd.
		total_horizontal_active_pixel);
	buffer[6] = REG_16_8_MSB(av8100_config->hdmi_video_input_cmd.
		total_vertical_lines);
	buffer[7] = REG_16_8_LSB(av8100_config->hdmi_video_input_cmd.
		total_vertical_lines);
	buffer[8] = REG_16_8_MSB(av8100_config->hdmi_video_input_cmd.
		total_vertical_active_lines);
	buffer[9] = REG_16_8_LSB(av8100_config->hdmi_video_input_cmd.
		total_vertical_active_lines);
	buffer[10] = av8100_config->hdmi_video_input_cmd.video_mode;
	buffer[11] = av8100_config->hdmi_video_input_cmd.nb_data_lane;
	buffer[12] = av8100_config->hdmi_video_input_cmd.
		nb_virtual_ch_command_mode;
	buffer[13] = av8100_config->hdmi_video_input_cmd.
		nb_virtual_ch_video_mode;
	buffer[14] = REG_16_8_MSB(av8100_config->hdmi_video_input_cmd.
		TE_line_nb);
	buffer[15] = REG_16_8_LSB(av8100_config->hdmi_video_input_cmd.
		TE_line_nb);
	buffer[16] = av8100_config->hdmi_video_input_cmd.TE_config;
	buffer[17] = REG_32_8_MSB(av8100_config->hdmi_video_input_cmd.
		master_clock_freq);
	buffer[18] = REG_32_8_MMSB(av8100_config->hdmi_video_input_cmd.
		master_clock_freq);
	buffer[19] = REG_32_8_MLSB(av8100_config->hdmi_video_input_cmd.
		master_clock_freq);
	buffer[20] = REG_32_8_LSB(av8100_config->hdmi_video_input_cmd.
		master_clock_freq);
	buffer[21] = av8100_config->hdmi_video_input_cmd.ui_x4;

	*length = AV8100_COMMAND_VIDEO_INPUT_FORMAT_SIZE - 1;
	return 0;

}

static int configuration_audio_input_get(char *buffer,
	unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	buffer[0] = av8100_config->hdmi_audio_input_cmd.
		audio_input_if_format;
	buffer[1] = av8100_config->hdmi_audio_input_cmd.i2s_input_nb;
	buffer[2] = av8100_config->hdmi_audio_input_cmd.sample_audio_freq;
	buffer[3] = av8100_config->hdmi_audio_input_cmd.audio_word_lg;
	buffer[4] = av8100_config->hdmi_audio_input_cmd.audio_format;
	buffer[5] = av8100_config->hdmi_audio_input_cmd.audio_if_mode;
	buffer[6] = av8100_config->hdmi_audio_input_cmd.audio_mute;

	*length = AV8100_COMMAND_AUDIO_INPUT_FORMAT_SIZE - 1;
	return 0;
}

static int configuration_video_output_get(char *buffer,
	unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	buffer[0] = av8100_config->hdmi_video_output_cmd.
		video_output_cea_vesa;

	if (buffer[0] == AV8100_CUSTOM) {
		buffer[1] = av8100_config->hdmi_video_output_cmd.
			vsync_polarity;
		buffer[2] = av8100_config->hdmi_video_output_cmd.
			hsync_polarity;
		buffer[3] = REG_16_8_MSB(av8100_config->
			hdmi_video_output_cmd.total_horizontal_pixel);
		buffer[4] = REG_16_8_LSB(av8100_config->
			hdmi_video_output_cmd.total_horizontal_pixel);
		buffer[5] = REG_16_8_MSB(av8100_config->
			hdmi_video_output_cmd.total_horizontal_active_pixel);
		buffer[6] = REG_16_8_LSB(av8100_config->
			hdmi_video_output_cmd.total_horizontal_active_pixel);
		buffer[7] = REG_16_8_MSB(av8100_config->
			hdmi_video_output_cmd.total_vertical_in_half_lines);
		buffer[8] = REG_16_8_LSB(av8100_config->
			hdmi_video_output_cmd.total_vertical_in_half_lines);
		buffer[9] = REG_16_8_MSB(av8100_config->
			hdmi_video_output_cmd.
				total_vertical_active_in_half_lines);
		buffer[10] = REG_16_8_LSB(av8100_config->
			hdmi_video_output_cmd.
				total_vertical_active_in_half_lines);
		buffer[11] = REG_16_8_MSB(av8100_config->
			hdmi_video_output_cmd.hsync_start_in_pixel);
		buffer[12] = REG_16_8_LSB(av8100_config->
			hdmi_video_output_cmd.hsync_start_in_pixel);
		buffer[13] = REG_16_8_MSB(av8100_config->
			hdmi_video_output_cmd.hsync_length_in_pixel);
		buffer[14] = REG_16_8_LSB(av8100_config->
			hdmi_video_output_cmd.hsync_length_in_pixel);
		buffer[15] = REG_16_8_MSB(av8100_config->
			hdmi_video_output_cmd.vsync_start_in_half_line);
		buffer[16] = REG_16_8_LSB(av8100_config->
			hdmi_video_output_cmd.vsync_start_in_half_line);
		buffer[17] = REG_16_8_MSB(av8100_config->
			hdmi_video_output_cmd.vsync_length_in_half_line);
		buffer[18] = REG_16_8_LSB(av8100_config->
			hdmi_video_output_cmd.vsync_length_in_half_line);
		buffer[19] = REG_16_8_MSB(av8100_config->
			hdmi_video_output_cmd.hor_video_start_pixel);
		buffer[20] = REG_16_8_LSB(av8100_config->
			hdmi_video_output_cmd.hor_video_start_pixel);
		buffer[21] = REG_16_8_MSB(av8100_config->
			hdmi_video_output_cmd.vert_video_start_pixel);
		buffer[22] = REG_16_8_LSB(av8100_config->
			hdmi_video_output_cmd.vert_video_start_pixel);
		buffer[23] = av8100_config->
			hdmi_video_output_cmd.video_type;
		buffer[24] = av8100_config->
			hdmi_video_output_cmd.pixel_repeat;
		buffer[25] = REG_32_8_MSB(av8100_config->
			hdmi_video_output_cmd.pixel_clock_freq_Hz);
		buffer[26] = REG_32_8_MMSB(av8100_config->
			hdmi_video_output_cmd.pixel_clock_freq_Hz);
		buffer[27] = REG_32_8_MLSB(av8100_config->
			hdmi_video_output_cmd.pixel_clock_freq_Hz);
		buffer[28] = REG_32_8_LSB(av8100_config->
			hdmi_video_output_cmd.pixel_clock_freq_Hz);

		*length = AV8100_COMMAND_VIDEO_OUTPUT_FORMAT_SIZE - 1;
	} else {
		*length = 1;
	}

	return 0;
}

static int configuration_video_scaling_get(char *buffer,
	unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	buffer[0] = REG_16_8_MSB(av8100_config->hdmi_video_scaling_cmd.
		h_start_in_pixel);
	buffer[1] = REG_16_8_LSB(av8100_config->hdmi_video_scaling_cmd.
		h_start_in_pixel);
	buffer[2] = REG_16_8_MSB(av8100_config->hdmi_video_scaling_cmd.
		h_stop_in_pixel);
	buffer[3] = REG_16_8_LSB(av8100_config->hdmi_video_scaling_cmd.
		h_stop_in_pixel);
	buffer[4] = REG_16_8_MSB(av8100_config->hdmi_video_scaling_cmd.
		v_start_in_line);
	buffer[5] = REG_16_8_LSB(av8100_config->hdmi_video_scaling_cmd.
		v_start_in_line);
	buffer[6] = REG_16_8_MSB(av8100_config->hdmi_video_scaling_cmd.
		v_stop_in_line);
	buffer[7] = REG_16_8_LSB(av8100_config->hdmi_video_scaling_cmd.
		v_stop_in_line);
	buffer[8] = REG_16_8_MSB(av8100_config->hdmi_video_scaling_cmd.
		h_start_out_pixel);
	buffer[9] = REG_16_8_LSB(av8100_config->hdmi_video_scaling_cmd
		.h_start_out_pixel);
	buffer[10] = REG_16_8_MSB(av8100_config->hdmi_video_scaling_cmd.
		h_stop_out_pixel);
	buffer[11] = REG_16_8_LSB(av8100_config->hdmi_video_scaling_cmd.
		h_stop_out_pixel);
	buffer[12] = REG_16_8_MSB(av8100_config->hdmi_video_scaling_cmd.
		v_start_out_line);
	buffer[13] = REG_16_8_LSB(av8100_config->hdmi_video_scaling_cmd.
		v_start_out_line);
	buffer[14] = REG_16_8_MSB(av8100_config->hdmi_video_scaling_cmd.
		v_stop_out_line);
	buffer[15] = REG_16_8_LSB(av8100_config->hdmi_video_scaling_cmd.
		v_stop_out_line);

	*length = AV8100_COMMAND_VIDEO_SCALING_FORMAT_SIZE - 1;
	return 0;
}

static int configuration_colorspace_conversion_get(char *buffer,
	unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	buffer[0] = REG_10_8_MSB(av8100_config->
		hdmi_color_space_conversion_cmd.c0);
	buffer[1] = REG_16_8_LSB(av8100_config->
		hdmi_color_space_conversion_cmd.c0);
	buffer[2] = REG_10_8_MSB(av8100_config->
		hdmi_color_space_conversion_cmd.c1);
	buffer[3] = REG_16_8_LSB(av8100_config->
		hdmi_color_space_conversion_cmd.c1);
	buffer[4] = REG_10_8_MSB(av8100_config->
		hdmi_color_space_conversion_cmd.c2);
	buffer[5] = REG_16_8_LSB(av8100_config->
		hdmi_color_space_conversion_cmd.c2);
	buffer[6] = REG_10_8_MSB(av8100_config->
		hdmi_color_space_conversion_cmd.c3);
	buffer[7] = REG_16_8_LSB(av8100_config->
		hdmi_color_space_conversion_cmd.c3);
	buffer[8] = REG_10_8_MSB(av8100_config->
		hdmi_color_space_conversion_cmd.c4);
	buffer[9] = REG_16_8_LSB(av8100_config->
		hdmi_color_space_conversion_cmd.c4);
	buffer[10] = REG_10_8_MSB(av8100_config->
		hdmi_color_space_conversion_cmd.c5);
	buffer[11] = REG_16_8_LSB(av8100_config->
		hdmi_color_space_conversion_cmd.c5);
	buffer[12] = REG_10_8_MSB(av8100_config->
		hdmi_color_space_conversion_cmd.c6);
	buffer[13] = REG_16_8_LSB(av8100_config->
		hdmi_color_space_conversion_cmd.c6);
	buffer[14] = REG_10_8_MSB(av8100_config->
		hdmi_color_space_conversion_cmd.c7);
	buffer[15] = REG_16_8_LSB(av8100_config->
		hdmi_color_space_conversion_cmd.c7);
	buffer[16] = REG_10_8_MSB(av8100_config->
		hdmi_color_space_conversion_cmd.c8);
	buffer[17] = REG_16_8_LSB(av8100_config->
		hdmi_color_space_conversion_cmd.c8);
	buffer[18] = REG_16_8_MSB(av8100_config->
		hdmi_color_space_conversion_cmd.aoffset);
	buffer[19] = REG_16_8_LSB(av8100_config->
		hdmi_color_space_conversion_cmd.aoffset);
	buffer[20] = REG_16_8_MSB(av8100_config->
		hdmi_color_space_conversion_cmd.boffset);
	buffer[21] = REG_16_8_LSB(av8100_config->
		hdmi_color_space_conversion_cmd.boffset);
	buffer[22] = REG_16_8_MSB(av8100_config->
		hdmi_color_space_conversion_cmd.coffset);
	buffer[23] = REG_16_8_LSB(av8100_config->
		hdmi_color_space_conversion_cmd.coffset);
	buffer[24] = av8100_config->hdmi_color_space_conversion_cmd.lmax;
	buffer[25] = av8100_config->hdmi_color_space_conversion_cmd.lmin;
	buffer[26] = av8100_config->hdmi_color_space_conversion_cmd.cmax;
	buffer[27] = av8100_config->hdmi_color_space_conversion_cmd.cmin;

	*length = AV8100_COMMAND_COLORSPACECONVERSION_SIZE - 1;
	return 0;
}

static int configuration_cec_message_write_get(char *buffer,
	unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	buffer[0] = av8100_config->hdmi_cec_message_write_cmd.buffer_length;
	memcpy(&buffer[1], av8100_config->hdmi_cec_message_write_cmd.buffer,
		HDMI_CEC_MESSAGE_WRITE_BUFFER_SIZE);

	*length = AV8100_COMMAND_CEC_MESSAGE_WRITE_SIZE - 1;
	return 0;
}

static int configuration_cec_message_read_get(char *buffer,
	unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	/* No buffer data */
	*length = AV8100_COMMAND_CEC_MESSAGE_READ_BACK_SIZE - 1;
	return 0;
}

static int configuration_denc_get(char *buffer,
	unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	buffer[0] = av8100_config->hdmi_denc_cmd.cvbs_video_format;
	buffer[1] = av8100_config->hdmi_denc_cmd.standard_selection;
	buffer[2] = av8100_config->hdmi_denc_cmd.on_off;
	buffer[3] = av8100_config->hdmi_denc_cmd.macrovision_on_off;
	buffer[4] = av8100_config->hdmi_denc_cmd.internal_generator;

	*length = AV8100_COMMAND_DENC_SIZE - 1;
	return 0;
}

static int configuration_hdmi_get(char *buffer, unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	buffer[0] = av8100_config->hdmi_cmd.hdmi_mode;
	buffer[1] = av8100_config->hdmi_cmd.hdmi_format;
	buffer[2] = av8100_config->hdmi_cmd.dvi_format;

	*length = AV8100_COMMAND_HDMI_SIZE - 1;
	return 0;
}

static int configuration_hdcp_sendkey_get(char *buffer,
	unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	buffer[0] = av8100_config->hdmi_hdcp_send_key_cmd.key_number;
	buffer[1] = 0;
	memcpy(&buffer[2], av8100_config->hdmi_hdcp_send_key_cmd.key,
		HDMI_HDCP_SEND_KEY_SIZE);

	*length = AV8100_COMMAND_HDCP_SENDKEY_SIZE - 1;
	return 0;
}

static int configuration_hdcp_management_get(char *buffer,
	unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	buffer[0] = av8100_config->hdmi_hdcp_management_format_cmd.
		request_hdcp_revocation_list;
	buffer[1] = av8100_config->hdmi_hdcp_management_format_cmd.
		request_encrypted_transmission;
	buffer[2] = av8100_config->hdmi_hdcp_management_format_cmd.
		oess_eess_encryption_use;

	*length = AV8100_COMMAND_HDCP_MANAGEMENT_SIZE - 1;
	return 0;
}

static int configuration_infoframe_get(char *buffer,
	unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	buffer[0] = av8100_config->hdmi_infoframes_cmd.type;
	buffer[1] = av8100_config->hdmi_infoframes_cmd.version;
	buffer[2] = av8100_config->hdmi_infoframes_cmd.length;
	buffer[3] = av8100_config->hdmi_infoframes_cmd.crc;
	memcpy(&buffer[4], av8100_config->hdmi_infoframes_cmd.data,
	HDMI_INFOFRAME_DATA_SIZE);

	*length = AV8100_COMMAND_INFOFRAMES_SIZE - 1;
	return 0;
}

static int av8100_edid_section_readback_get(char *buffer, unsigned int *length)
{
	buffer[0] = av8100_config->hdmi_edid_section_readback_cmd.address;
	buffer[1] = av8100_config->hdmi_edid_section_readback_cmd.
		block_number;

	*length = AV8100_COMMAND_EDID_SECTION_READBACK_SIZE - 1;
	return 0;
}

static int configuration_pattern_generator_get(char *buffer,
	unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	buffer[0] = av8100_config->hdmi_pattern_generator_cmd.pattern_type;
	buffer[1] = av8100_config->hdmi_pattern_generator_cmd.
		pattern_video_format;
	buffer[2] = av8100_config->hdmi_pattern_generator_cmd.
		pattern_audio_mode;

	*length = AV8100_COMMAND_PATTERNGENERATOR_SIZE - 1;
	return 0;
}

static int configuration_fuse_aes_key_get(char *buffer,
	unsigned int *length)
{
	if (!av8100_config)
		return AV8100_FAIL;

	buffer[0] = av8100_config->hdmi_fuse_aes_key_cmd.fuse_operation;
	memcpy(&buffer[1], av8100_config->hdmi_fuse_aes_key_cmd.key,
		HDMI_FUSE_AES_KEY_SIZE);

	*length = AV8100_COMMAND_FUSE_AES_KEY_SIZE - 1;
	return 0;
}

static int get_command_return_data(struct i2c_client *i2c,
	enum av8100_command_type command_type,
	u8 *command_buffer,
	u8 *buffer_length,
	u8 *buffer)
{
	int retval = 0;
	char val;
	int index = 0;

	/* Get the first return byte */
	retval = read_single_byte(i2c, AV8100_COMMAND_OFFSET, &val);
	if (retval)
		goto get_command_return_data_fail;

	if (val != (0x80 | command_type)) {
		retval = AV8100_FAIL;
		goto get_command_return_data_fail;
	}

	switch (command_type) {
	case AV8100_COMMAND_VIDEO_INPUT_FORMAT:
	case AV8100_COMMAND_AUDIO_INPUT_FORMAT:
	case AV8100_COMMAND_VIDEO_OUTPUT_FORMAT:
	case AV8100_COMMAND_VIDEO_SCALING_FORMAT:
	case AV8100_COMMAND_COLORSPACECONVERSION:
	case AV8100_COMMAND_CEC_MESSAGE_WRITE:
	case AV8100_COMMAND_DENC:
	case AV8100_COMMAND_HDMI:
	case AV8100_COMMAND_HDCP_SENDKEY:
	case AV8100_COMMAND_INFOFRAMES:
	case AV8100_COMMAND_PATTERNGENERATOR:
		/* Get the second return byte */
		retval = read_single_byte(i2c,
			AV8100_COMMAND_OFFSET + 1, &val);
		if (retval)
			goto get_command_return_data_fail;

		if (val) {
			retval = AV8100_FAIL;
			goto get_command_return_data_fail;
		}
		break;

	case AV8100_COMMAND_CEC_MESSAGE_READ_BACK:
		if ((buffer == NULL) ||	(buffer_length == NULL)) {
			retval = AV8100_FAIL;
			goto get_command_return_data_fail;
		}

		/* Get the return buffer length */
		retval = read_single_byte(i2c,
			AV8100_COMMAND_OFFSET + 3, &val);
		if (retval)
			goto get_command_return_data_fail;

		*buffer_length = val;

		if (*buffer_length >
			HDMI_CEC_MESSAGE_READBACK_MAXSIZE) {
			printk(KERN_DEBUG "CEC size too large %d\n",
				*buffer_length);
			*buffer_length = HDMI_CEC_MESSAGE_READBACK_MAXSIZE;
		}

#ifdef AV8100_DEBUG_EXTRA
		printk(KERN_DEBUG "return data: ");
#endif

		/* Get the return buffer */
		for (index = 0; index < *buffer_length; ++index) {
			retval = read_single_byte(i2c,
				AV8100_COMMAND_OFFSET + 4 + index,
				&val);
			if (retval) {
				*buffer_length = 0;
				goto get_command_return_data_fail;
			} else {
				*(buffer + index) = val;
#ifdef AV8100_DEBUG_EXTRA
				printk(KERN_DEBUG "%02x ", *(buffer + index));
#endif
			}
		}
#ifdef AV8100_DEBUG_EXTRA
		printk(KERN_DEBUG "\n");
#endif
		break;

	case AV8100_COMMAND_HDCP_MANAGEMENT:
		if ((buffer == NULL) ||	(buffer_length == NULL) ||
			(command_buffer == NULL)) {
			retval = AV8100_FAIL;
			goto get_command_return_data_fail;
		}

		/* Get the second return byte */
		retval = read_single_byte(i2c,
			AV8100_COMMAND_OFFSET + 1, &val);
		if (retval) {
			goto get_command_return_data_fail;
		} else {
			/* Check the second return byte */
			if (val)
				goto get_command_return_data_fail;
		}

		/* Get the return buffer length */
		if (command_buffer[0] ==
			HDMI_REQUEST_FOR_REVOCATION_LIST_INPUT) {
			*buffer_length = 0x1F;
		} else {
			*buffer_length = 0x0;
		}

#ifdef AV8100_DEBUG_EXTRA
		printk(KERN_DEBUG "return data: ");
#endif
		/* Get the return buffer */
		for (index = 0; index < *buffer_length; ++index) {
			retval = read_single_byte(i2c,
				AV8100_COMMAND_OFFSET + 2 + index,
				&val);
			if (retval) {
				*buffer_length = 0;
				goto get_command_return_data_fail;
			} else {
				*(buffer + index) = val;
#ifdef AV8100_DEBUG_EXTRA
				printk(KERN_DEBUG "%02x ", *(buffer + index));
#endif
			}
		}
#ifdef AV8100_DEBUG_EXTRA
		printk(KERN_DEBUG "\n");
#endif
		break;

	case AV8100_COMMAND_EDID_SECTION_READBACK:
		if ((buffer == NULL) ||	(buffer_length == NULL)) {
			retval = AV8100_FAIL;
			goto get_command_return_data_fail;
		}

		/* Return buffer length is fixed */
		*buffer_length = 0x80;

#ifdef AV8100_DEBUG_EXTRA
		printk(KERN_DEBUG "return data: ");
#endif
		/* Get the return buffer */
		for (index = 0; index < *buffer_length; ++index) {
			retval = read_single_byte(i2c,
				AV8100_COMMAND_OFFSET + 1 + index,
				&val);
			if (retval) {
				*buffer_length = 0;
				goto get_command_return_data_fail;
			} else {
				*(buffer + index) = val;
#ifdef AV8100_DEBUG_EXTRA
				printk(KERN_DEBUG "%02x ", *(buffer + index));
#endif
			}
		}
#ifdef AV8100_DEBUG_EXTRA
		printk(KERN_DEBUG "\n");
#endif
		break;

	case AV8100_COMMAND_FUSE_AES_KEY:
		if ((buffer == NULL) ||	(buffer_length == NULL)) {
			retval = AV8100_FAIL;
			goto get_command_return_data_fail;
		}

		/* Get the second return byte */
		retval = read_single_byte(i2c,
			AV8100_COMMAND_OFFSET + 1, &val);
		if (retval)
			goto get_command_return_data_fail;

		/* Check the second return byte */
		if (val) {
			retval = AV8100_FAIL;
			goto get_command_return_data_fail;
		}

		/* Return buffer length is fixed */
		*buffer_length = 0x2;

		/* Get CRC */
		retval = read_single_byte(i2c,
			AV8100_COMMAND_OFFSET + 1, &val);
		if (retval)
			goto get_command_return_data_fail;

		*(buffer + 0) = val;
#ifdef AV8100_DEBUG_EXTRA
		printk(KERN_DEBUG "CRC:%02x ", val);
#endif

		/* Get programmed status */
		retval = read_single_byte(i2c,
			AV8100_COMMAND_OFFSET + 1, &val);
		if (retval)
			goto get_command_return_data_fail;

		*(buffer + 1) = val;
#ifdef AV8100_DEBUG_EXTRA
		printk(KERN_DEBUG "programmed:%02x ", val);
#endif
		break;

	default:
		retval = AV8100_INVALID_COMMAND;
		break;
	}

	return retval;
get_command_return_data_fail:
	printk(KERN_DEBUG "get_command_return_data FAIL\n");
	return retval;
}

int av8100_powerup(void)
{
	int retval = 0;
	struct i2c_client *i2c;
	u8 cpd;
	u8 stby;
	u8 hpds;
	u8 cpds;
	u8 mclkrng;
	u8 off_time;
	u8 on_time;
	u8 fdl;
	u8 hld;
	u8 wa;
	u8 ra;

	if (!av8100_config)
		return AV8100_FAIL;

	i2c = av8100_config->client;

	/* Reset av8100 */
	gpio_set_value(GPIO_AV8100_RSTN, 1);
	av8100_set_state(AV8100_OPMODE_STANDBY);

	/* Master clock timing, running, search for plug */
	retval = av8100_register_standby_write(AV8100_STANDBY_CPD_HIGH,
		AV8100_STANDBY_STBY_HIGH, AV8100_MASTER_CLOCK_TIMING);
	if (retval) {
		pr_err("Failed to write the value to av8100 register\n");
		return -EFAULT;
	}

	retval = av8100_register_standby_read(&cpd, &stby, &hpds, &cpds,
		&mclkrng);
	if (retval) {
		pr_err("Failed to read the value from av8100 register\n");
		return -EFAULT;
	} else {
		printk(KERN_DEBUG "STANDBY_REG register cpd:%d stby:%d "
			"hpds:%d cpds:%d mclkrng:%0x\n", cpd, stby, hpds,
			cpds, mclkrng);
	}

	/* ON time & OFF time on 5v HDMI plug detect */
	retval = av8100_register_hdmi_5_volt_time_write(AV8100_OFF_TIME,
		AV8100_ON_TIME);
	if (retval) {
		pr_err("Failed to write the value to av8100 register\n");
		return -EFAULT;
	}

	retval = av8100_register_hdmi_5_volt_time_read(&off_time, &on_time);
	if (retval) {
		pr_err("Failed to read the value from av8100 register\n");
		return -EFAULT;
	} else {
		printk(KERN_DEBUG "AV8100_5_VOLT_TIME_REG register "
			"off_time:%0x on_time:%0x\n", off_time, on_time);
	}

	/* Device in hold mode, enable firmware download*/
	retval = av8100_register_general_control_write(
		AV8100_GENERAL_CONTROL_FDL_HIGH,
		AV8100_GENERAL_CONTROL_HLD_HIGH,
		AV8100_GENERAL_CONTROL_WA_LOW,
		AV8100_GENERAL_CONTROL_RA_LOW);
	if (retval) {
		pr_err("Failed to write the value to av8100 register\n");
		return -EFAULT;
	}

	retval = av8100_register_general_control_read(&fdl, &hld, &wa, &ra);
	if (retval) {
		pr_err("Failed to read the value from av8100 register\n");
		return -EFAULT;
	} else {
		printk(KERN_DEBUG "GENERAL_CONTROL_REG register fdl:%d "
			"hld:%d wa:%d ra:%d\n", fdl, hld, wa, ra);
	}

	av8100_set_state(AV8100_OPMODE_SCAN);
	return retval;
}

int av8100_powerdown(void)
{
	gpio_set_value(GPIO_AV8100_RSTN, 0);
	av8100_set_state(AV8100_OPMODE_SHUTDOWN);

	return 0;
}

int av8100_download_firmware(char *fw_buff, int nbytes,
	enum interface_type if_type)
{
	int retval = 0;
	int temp = 0x0;
	int increment = 15;
	int index = 0;
	int size = 0x0;
	int tempnext = 0x0;
	char val = 0x0;
	char CheckSum = 0;
	int cnt = 10;
	struct i2c_client *i2c;
	u8 cecrec;
	u8 cectrx;
	u8 uc;
	u8 onuvb;
	u8 hdcps;

	if (!av8100_config) {
		retval = AV8100_FAIL;
		goto av8100_download_firmware_out;
	}

	if (fw_buff == NULL) {
		/* use default fw buffer */
		fw_buff = av8100_fw_buff;
		nbytes = AV8100_FW_SIZE;
	}

	i2c = av8100_config->client;

	LOCK_AV8100_HW;

	temp = nbytes % increment;
	for (size = 0; size < (nbytes-temp); size = size + increment,
		index += increment) {
		if (if_type == I2C_INTERFACE) {
			retval = write_multi_byte(i2c,
				AV8100_FIRMWARE_DOWNLOAD_ENTRY, fw_buff + size,
				increment);
			if (retval) {
				printk(KERN_DEBUG "Failed to download the "
					"av8100 firmware\n");
				retval = -EFAULT;
				UNLOCK_AV8100_HW;
				goto av8100_download_firmware_out;
			}
		} else if (if_type == DSI_INTERFACE) {
			printk(KERN_DEBUG "DSI_INTERFACE is currently not supported\n");
			UNLOCK_AV8100_HW;
			goto av8100_download_firmware_out;
		} else {
			retval = AV8100_INVALID_INTERFACE;
			UNLOCK_AV8100_HW;
			goto av8100_download_firmware_out;
		}

		for (tempnext = size; tempnext < (increment+size); tempnext++)
			av8100_receivetab[tempnext] = fw_buff[tempnext];
	}

	/* Transfer last firmware bytes */
	if (if_type == I2C_INTERFACE) {
		retval = write_multi_byte(i2c,
			AV8100_FIRMWARE_DOWNLOAD_ENTRY, fw_buff + size, temp);
		if (retval) {
			printk(KERN_DEBUG "Failed to download the av8100 firmware\n");
			retval = -EFAULT;
			UNLOCK_AV8100_HW;
			goto av8100_download_firmware_out;
		}
	} else if (if_type == DSI_INTERFACE) {
		/* TODO: Add support for DSI firmware download */
		retval = AV8100_INVALID_INTERFACE;
		UNLOCK_AV8100_HW;
		goto av8100_download_firmware_out;
	} else {
		retval = AV8100_INVALID_INTERFACE;
		UNLOCK_AV8100_HW;
		goto av8100_download_firmware_out;
	}

	for (tempnext = size; tempnext < (size+temp); tempnext++)
		av8100_receivetab[tempnext] = fw_buff[tempnext];

	/* check transfer*/
	for (size = 0; size < nbytes; size++) {
		CheckSum = CheckSum ^ fw_buff[size];
		if (av8100_receivetab[size] != fw_buff[size]) {
			printk(KERN_DEBUG ">Fw download fail....i=%d\n", size);
			printk(KERN_DEBUG "Transm = %x, Receiv = %x\n",
				fw_buff[size], av8100_receivetab[size]);
		}
	}

	UNLOCK_AV8100_HW;

	retval = av8100_register_firmware_download_entry_read(&val);
	if (retval) {
		printk(KERN_DEBUG "Failed to read the value from the av8100 register\n");
		retval = -EFAULT;
		goto av8100_download_firmware_out;
	}

	printk(KERN_DEBUG "CheckSum:%x,val:%x\n", CheckSum, val);

	if (CheckSum != val) {
		printk(KERN_DEBUG ">Fw downloading.... FAIL CheckSum issue\n");
		printk(KERN_DEBUG "Checksum = %d\n", CheckSum);
		printk(KERN_DEBUG "Checksum read: %d\n", val);
		retval = AV8100_FWDOWNLOAD_FAIL;
		goto av8100_download_firmware_out;
	} else {
		printk(KERN_DEBUG ">Fw downloading.... success\n");
	}

	/* Set to idle mode */
	av8100_register_general_control_write(AV8100_GENERAL_CONTROL_FDL_LOW,
		AV8100_GENERAL_CONTROL_HLD_LOW,	AV8100_GENERAL_CONTROL_WA_LOW,
		AV8100_GENERAL_CONTROL_RA_LOW);
	if (retval) {
		printk(KERN_DEBUG "Failed to write the value to the av8100 registers\n");
		retval = -EFAULT;
		goto av8100_download_firmware_out;
	}

	/* Wait Internal Micro controler ready */
	cnt = 3;
	retval = av8100_register_general_status_read(&cecrec, &cectrx, &uc,
		&onuvb, &hdcps);
	while ((retval == 0) && (uc != 0x1) && (cnt-- > 0)) {
		printk(KERN_DEBUG "av8100 wait2\n");
		/* TODO */
		for (temp = 0; temp < 0xFFFFF; temp++)
			;

		retval = av8100_register_general_status_read(&cecrec, &cectrx,
			&uc, &onuvb, &hdcps);
	}

	if (retval)	{
		printk(KERN_DEBUG "Failed to read the value from the av8100 register\n");
		retval = -EFAULT;
		goto av8100_download_firmware_out;
	}

	av8100_set_state(AV8100_OPMODE_IDLE);

av8100_download_firmware_out:
	return retval;
}

int av8100_disable_interrupt(void)
{
	int retval;
	struct i2c_client *i2c;

	if (!av8100_config) {
		retval = AV8100_FAIL;
		goto av8100_disable_interrupt_out;
	}

	i2c = av8100_config->client;

	retval = av8100_register_standby_pending_interrupt_write(
			AV8100_STANDBY_PENDING_INTERRUPT_HPDI_LOW,
			AV8100_STANDBY_PENDING_INTERRUPT_CPDI_LOW,
			AV8100_STANDBY_PENDING_INTERRUPT_ONI_LOW);
	if (retval) {
		printk(KERN_DEBUG "Failed to write the value to av8100 register\n");
		retval = -EFAULT;
		goto av8100_disable_interrupt_out;
	}

	retval = av8100_register_general_interrupt_mask_write(
			AV8100_GENERAL_INTERRUPT_MASK_EOCM_LOW,
			AV8100_GENERAL_INTERRUPT_MASK_VSIM_LOW,
			AV8100_GENERAL_INTERRUPT_MASK_VSOM_LOW,
			AV8100_GENERAL_INTERRUPT_MASK_CECM_LOW,
			AV8100_GENERAL_INTERRUPT_MASK_HDCPM_LOW,
			AV8100_GENERAL_INTERRUPT_MASK_UOVBM_LOW,
			AV8100_GENERAL_INTERRUPT_MASK_TEM_LOW);
	if (retval) {
		printk(KERN_DEBUG "Failed to write the value to av8100 register\n");
		retval = -EFAULT;
		goto av8100_disable_interrupt_out;
	}

	retval = av8100_register_standby_interrupt_mask_write(
			AV8100_STANDBY_INTERRUPT_MASK_HPDM_LOW,
			AV8100_STANDBY_INTERRUPT_MASK_CPDM_LOW,
			AV8100_STANDBY_INTERRUPT_MASK_STBYGPIOCFG_INPUT,
			AV8100_STANDBY_INTERRUPT_MASK_IPOL_LOW);
	if (retval) {
		printk(KERN_DEBUG "Failed to write the value to av8100 register\n");
		retval = -EFAULT;
		goto av8100_disable_interrupt_out;
	}

#ifdef AV8100_PLUGIN_DETECT_VIA_TIMER_INTERRUPTS
	del_timer(&av8100_timer);
#endif

av8100_disable_interrupt_out:
	return retval;
}

int av8100_enable_interrupt(void)
{
	int retval;
	struct i2c_client *i2c;

	if (!av8100_config) {
		retval = AV8100_FAIL;
		goto av8100_enable_interrupt_out;
	}

	i2c = av8100_config->client;

	retval = av8100_register_standby_pending_interrupt_write(
			AV8100_STANDBY_PENDING_INTERRUPT_HPDI_LOW,
			AV8100_STANDBY_PENDING_INTERRUPT_CPDI_LOW,
			AV8100_STANDBY_PENDING_INTERRUPT_ONI_LOW);
	if (retval) {
		printk(KERN_DEBUG "Failed to write the value to av8100 register\n");
		retval = -EFAULT;
		goto av8100_enable_interrupt_out;
	}

	retval = av8100_register_general_interrupt_mask_write(
			AV8100_GENERAL_INTERRUPT_MASK_EOCM_LOW,
			AV8100_GENERAL_INTERRUPT_MASK_VSIM_LOW,
			AV8100_GENERAL_INTERRUPT_MASK_VSOM_LOW,
			AV8100_GENERAL_INTERRUPT_MASK_CECM_LOW,
			AV8100_GENERAL_INTERRUPT_MASK_HDCPM_LOW,
			AV8100_GENERAL_INTERRUPT_MASK_UOVBM_LOW,
			AV8100_GENERAL_INTERRUPT_MASK_TEM_HIGH);
	if (retval) {
		printk(KERN_DEBUG "Failed to write the value to av8100 register\n");
		retval = -EFAULT;
		goto av8100_enable_interrupt_out;
	}

	retval = av8100_register_standby_interrupt_mask_write(
			AV8100_STANDBY_INTERRUPT_MASK_HPDM_LOW,
			AV8100_STANDBY_INTERRUPT_MASK_CPDM_LOW,
			AV8100_STANDBY_INTERRUPT_MASK_STBYGPIOCFG_INPUT,
			AV8100_STANDBY_INTERRUPT_MASK_IPOL_LOW);
	if (retval) {
		printk(KERN_DEBUG "Failed to write the value to av8100 register\n");
		retval = -EFAULT;
		goto av8100_enable_interrupt_out;
	}

#ifdef AV8100_PLUGIN_DETECT_VIA_TIMER_INTERRUPTS
	init_timer(&av8100_timer);
	av8100_timer.expires = jiffies + AV8100_TIMER_INTERRUPT_POLLING_TIME;
	av8100_timer.function = av8100_timer_int;
	av8100_timer.data = 0;
	add_timer(&av8100_timer);
#endif

av8100_enable_interrupt_out:
	return retval;
}

static int register_write_internal(u8 offset, u8 value)
{
	int retval;
	struct i2c_client *i2c;

	if (!av8100_config) {
		retval = AV8100_FAIL;
		goto av8100_register_write_out;
	}

	i2c = av8100_config->client;

	/* Write to register */
	retval = write_single_byte(i2c, offset, value);
	if (retval) {
		printk(KERN_DEBUG "Failed to write the value to av8100 register\n");
		retval = -EFAULT;
	}

av8100_register_write_out:
	return retval;
}

int av8100_register_standby_write(
		u8 cpd, u8 stby, u8 mclkrng)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Set register value */
	val = AV8100_STANDBY_CPD(cpd) | AV8100_STANDBY_STBY(stby) |
		AV8100_STANDBY_MCLKRNG(mclkrng);

	/* Write to register */
	retval = register_write_internal(AV8100_STANDBY, val);
	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_hdmi_5_volt_time_write(u8 off_time, u8 on_time)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Set register value */
	val = AV8100_HDMI_5_VOLT_TIME_OFF_TIME(off_time) |
		AV8100_HDMI_5_VOLT_TIME_ON_TIME(on_time);

	/* Write to register */
	retval = register_write_internal(AV8100_HDMI_5_VOLT_TIME, val);
	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_standby_interrupt_mask_write(
		u8 hpdm, u8 cpdm, u8 stbygpiocfg, u8 ipol)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Set register value */
	val = AV8100_STANDBY_INTERRUPT_MASK_HPDM(hpdm) |
		AV8100_STANDBY_INTERRUPT_MASK_CPDM(cpdm) |
		AV8100_STANDBY_INTERRUPT_MASK_STBYGPIOCFG(stbygpiocfg) |
		AV8100_STANDBY_INTERRUPT_MASK_IPOL(ipol);

	/* Write to register */
	retval = register_write_internal(AV8100_STANDBY_INTERRUPT_MASK, val);
	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_standby_pending_interrupt_write(
		u8 hpdi, u8 cpdi, u8 oni)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Set register value */
	val = AV8100_STANDBY_PENDING_INTERRUPT_HPDI(hpdi) |
		AV8100_STANDBY_PENDING_INTERRUPT_CPDI(cpdi) |
		AV8100_STANDBY_PENDING_INTERRUPT_ONI(oni);

	/* Write to register */
	retval = register_write_internal(AV8100_STANDBY_PENDING_INTERRUPT, val);
	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_general_interrupt_mask_write(
		u8 eocm, u8 vsim, u8 vsom, u8 cecm, u8 hdcpm, u8 uovbm,	u8 tem)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Set register value */
	val = AV8100_GENERAL_INTERRUPT_MASK_EOCM(eocm) |
		AV8100_GENERAL_INTERRUPT_MASK_VSIM(vsim) |
		AV8100_GENERAL_INTERRUPT_MASK_VSOM(vsom) |
		AV8100_GENERAL_INTERRUPT_MASK_CECM(cecm) |
		AV8100_GENERAL_INTERRUPT_MASK_HDCPM(hdcpm) |
		AV8100_GENERAL_INTERRUPT_MASK_UOVBM(uovbm) |
		AV8100_GENERAL_INTERRUPT_MASK_TEM(tem);

	/* Write to register */
	retval = register_write_internal(AV8100_GENERAL_INTERRUPT_MASK,	val);
	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_general_interrupt_write(
		u8 eoci, u8 vsii, u8 vsoi, u8 ceci,	u8 hdcpi, u8 uovbi)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Set register value */
	val = AV8100_GENERAL_INTERRUPT_EOCI(eoci) |
		AV8100_GENERAL_INTERRUPT_VSII(vsii) |
		AV8100_GENERAL_INTERRUPT_VSOI(vsoi) |
		AV8100_GENERAL_INTERRUPT_CECI(ceci) |
		AV8100_GENERAL_INTERRUPT_HDCPI(hdcpi) |
		AV8100_GENERAL_INTERRUPT_UOVBI(uovbi);

	/* Write to register */
	retval = register_write_internal(AV8100_GENERAL_INTERRUPT, val);
	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_gpio_configuration_write(
		u8 dat3dir, u8 dat3val, u8 dat2dir, u8 dat2val,	u8 dat1dir,
		u8 dat1val, u8 ucdbg)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Set register value */
	val = AV8100_GPIO_CONFIGURATION_DAT3DIR(dat3dir) |
		AV8100_GPIO_CONFIGURATION_DAT3VAL(dat3val) |
		AV8100_GPIO_CONFIGURATION_DAT2DIR(dat2dir) |
		AV8100_GPIO_CONFIGURATION_DAT2VAL(dat2val) |
		AV8100_GPIO_CONFIGURATION_DAT1DIR(dat1dir) |
		AV8100_GPIO_CONFIGURATION_DAT1VAL(dat1val) |
		AV8100_GPIO_CONFIGURATION_UCDBG(ucdbg);

	/* Write to register */
	retval = register_write_internal(AV8100_GPIO_CONFIGURATION, val);
	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_general_control_write(
		u8 fdl, u8 hld, u8 wa, u8 ra)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Set register value */
	val = AV8100_GENERAL_CONTROL_FDL(fdl) |
		AV8100_GENERAL_CONTROL_HLD(hld) |
		AV8100_GENERAL_CONTROL_WA(wa) |
		AV8100_GENERAL_CONTROL_RA(ra);

	/* Write to register */
	retval = register_write_internal(AV8100_GENERAL_CONTROL, val);
	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_firmware_download_entry_write(
	u8 mbyte_code_entry)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Set register value */
	val = AV8100_FIRMWARE_DOWNLOAD_ENTRY_MBYTE_CODE_ENTRY(
		mbyte_code_entry);

	/* Write to register */
	retval = register_write_internal(AV8100_FIRMWARE_DOWNLOAD_ENTRY, val);
	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_write(
		u8 offset, u8 value)
{
	int retval = 0;
	struct i2c_client *i2c;

	LOCK_AV8100_HW;

	if (!av8100_config) {
		retval = AV8100_FAIL;
		goto av8100_register_write_out;
	}

	i2c = av8100_config->client;

	/* Write to register */
	retval = write_single_byte(i2c, offset, value);
	if (retval) {
		printk(KERN_DEBUG "Failed to write the value to av8100 register\n");
		retval = -EFAULT;
	}

av8100_register_write_out:
	UNLOCK_AV8100_HW;
	return retval;
}

int register_read_internal(u8 offset, u8 *value)
{
	int retval = 0;
	struct i2c_client *i2c;

	if (!av8100_config) {
		retval = AV8100_FAIL;
		goto av8100_register_read_out;
	}

	i2c = av8100_config->client;

	/* Read from register */
	retval = read_single_byte(i2c, offset, value);
	if (retval)	{
		printk(KERN_DEBUG "Failed to read the value from av8100 register\n");
		retval = -EFAULT;
		goto av8100_register_read_out;
	}

av8100_register_read_out:
	return retval;
}

int av8100_register_standby_read(
		u8 *cpd, u8 *stby, u8 *hpds, u8 *cpds, u8 *mclkrng)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Read from register */
	retval = register_read_internal(AV8100_STANDBY, &val);

	/* Set return params */
	*cpd		= AV8100_STANDBY_CPD_GET(val);
	*stby		= AV8100_STANDBY_STBY_GET(val);
	*hpds		= AV8100_STANDBY_HPDS_GET(val);
	*cpds		= AV8100_STANDBY_CPDS_GET(val);
	*mclkrng	= AV8100_STANDBY_MCLKRNG_GET(val);

	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_hdmi_5_volt_time_read(
		u8 *off_time, u8 *on_time)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Read from register */
	retval = register_read_internal(AV8100_HDMI_5_VOLT_TIME, &val);

	/* Set return params */
	*off_time	= AV8100_HDMI_5_VOLT_TIME_OFF_TIME_GET(val);
	*on_time	= AV8100_HDMI_5_VOLT_TIME_ON_TIME_GET(val);

	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_standby_interrupt_mask_read(
		u8 *hpdm, u8 *cpdm, u8 *stbygpiocfg, u8 *ipol)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Read from register */
	retval = register_read_internal(AV8100_STANDBY_INTERRUPT_MASK, &val);

	/* Set return params */
	*hpdm			= AV8100_STANDBY_INTERRUPT_MASK_HPDM_GET(val);
	*cpdm			= AV8100_STANDBY_INTERRUPT_MASK_CPDM_GET(val);
	*stbygpiocfg	= AV8100_STANDBY_INTERRUPT_MASK_STBYGPIOCFG_GET(val);
	*ipol			= AV8100_STANDBY_INTERRUPT_MASK_IPOL_GET(val);

	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_standby_pending_interrupt_read(
		u8 *hpdi, u8 *cpdi, u8 *oni, u8 *sid)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Read from register */
	retval = register_read_internal(AV8100_STANDBY_PENDING_INTERRUPT, &val);

	/* Set return params */
	*hpdi	= AV8100_STANDBY_PENDING_INTERRUPT_HPDI_GET(val);
	*cpdi	= AV8100_STANDBY_PENDING_INTERRUPT_CPDI_GET(val);
	*oni	= AV8100_STANDBY_PENDING_INTERRUPT_ONI_GET(val);
	*sid	= AV8100_STANDBY_PENDING_INTERRUPT_SID_GET(val);

	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_general_interrupt_mask_read(
		u8 *eocm,
		u8 *vsim,
		u8 *vsom,
		u8 *cecm,
		u8 *hdcpm,
		u8 *uovbm,
		u8 *tem)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Read from register */
	retval = register_read_internal(AV8100_GENERAL_INTERRUPT_MASK, &val);

	/* Set return params */
	*eocm	= AV8100_GENERAL_INTERRUPT_MASK_EOCM_GET(val);
	*vsim	= AV8100_GENERAL_INTERRUPT_MASK_VSIM_GET(val);
	*vsom	= AV8100_GENERAL_INTERRUPT_MASK_VSOM_GET(val);
	*cecm	= AV8100_GENERAL_INTERRUPT_MASK_CECM_GET(val);
	*hdcpm	= AV8100_GENERAL_INTERRUPT_MASK_HDCPM_GET(val);
	*uovbm	= AV8100_GENERAL_INTERRUPT_MASK_UOVBM_GET(val);
	*tem	= AV8100_GENERAL_INTERRUPT_MASK_TEM_GET(val);

	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_general_interrupt_read(
		u8 *eoci,
		u8 *vsii,
		u8 *vsoi,
		u8 *ceci,
		u8 *hdcpi,
		u8 *uovbi,
		u8 *tei)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Read from register */
	retval = register_read_internal(AV8100_GENERAL_INTERRUPT, &val);

	/* Set return params */
	*eoci	= AV8100_GENERAL_INTERRUPT_EOCI_GET(val);
	*vsii	= AV8100_GENERAL_INTERRUPT_VSII_GET(val);
	*vsoi	= AV8100_GENERAL_INTERRUPT_VSOI_GET(val);
	*ceci	= AV8100_GENERAL_INTERRUPT_CECI_GET(val);
	*hdcpi	= AV8100_GENERAL_INTERRUPT_HDCPI_GET(val);
	*uovbi	= AV8100_GENERAL_INTERRUPT_UOVBI_GET(val);
	*tei	= AV8100_GENERAL_INTERRUPT_TEI_GET(val);

	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_general_status_read(
		u8 *cecrec,
		u8 *cectrx,
		u8 *uc,
		u8 *onuvb,
		u8 *hdcps)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Read from register */
	retval = register_read_internal(AV8100_GENERAL_STATUS, &val);

	/* Set return params */
	*cecrec	= AV8100_GENERAL_STATUS_CECREC_GET(val);
	*cectrx	= AV8100_GENERAL_STATUS_CECTRX_GET(val);
	*uc		= AV8100_GENERAL_STATUS_UC_GET(val);
	*onuvb	= AV8100_GENERAL_STATUS_ONUVB_GET(val);
	*hdcps	= AV8100_GENERAL_STATUS_HDCPS_GET(val);

	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_gpio_configuration_read(
		u8 *dat3dir,
		u8 *dat3val,
		u8 *dat2dir,
		u8 *dat2val,
		u8 *dat1dir,
		u8 *dat1val,
		u8 *ucdbg)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Read from register */
	retval = register_read_internal(AV8100_GPIO_CONFIGURATION, &val);

	/* Set return params */
	*dat3dir	= AV8100_GPIO_CONFIGURATION_DAT3DIR_GET(val);
	*dat3val	= AV8100_GPIO_CONFIGURATION_DAT3VAL_GET(val);
	*dat2dir	= AV8100_GPIO_CONFIGURATION_DAT2DIR_GET(val);
	*dat2val	= AV8100_GPIO_CONFIGURATION_DAT2VAL_GET(val);
	*dat1dir	= AV8100_GPIO_CONFIGURATION_DAT1DIR_GET(val);
	*dat1val	= AV8100_GPIO_CONFIGURATION_DAT1VAL_GET(val);
	*ucdbg		= AV8100_GPIO_CONFIGURATION_UCDBG_GET(val);

	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_general_control_read(
		u8 *fdl,
		u8 *hld,
		u8 *wa,
		u8 *ra)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Read from register */
	retval = register_read_internal(AV8100_GENERAL_CONTROL, &val);
	/* Set return params */
	*fdl	= AV8100_GENERAL_CONTROL_FDL_GET(val);
	*hld	= AV8100_GENERAL_CONTROL_HLD_GET(val);
	*wa		= AV8100_GENERAL_CONTROL_WA_GET(val);
	*ra		= AV8100_GENERAL_CONTROL_RA_GET(val);

	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_firmware_download_entry_read(
	u8 *mbyte_code_entry)
{
	int retval;
	u8 val;

	LOCK_AV8100_HW;

	/* Read from register */
	retval = register_read_internal(AV8100_FIRMWARE_DOWNLOAD_ENTRY,	&val);

	/* Set return params */
	*mbyte_code_entry =
		AV8100_FIRMWARE_DOWNLOAD_ENTRY_MBYTE_CODE_ENTRY_GET(val);

	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_register_read(
		u8 offset,
		u8 *value)
{
	int retval = 0;
	struct i2c_client *i2c;

	LOCK_AV8100_HW;

	if (!av8100_config) {
		retval = AV8100_FAIL;
		goto av8100_register_read_out;
	}

	i2c = av8100_config->client;

	/* Read from register */
	retval = read_single_byte(i2c, offset, value);
	if (retval)	{
		printk(KERN_DEBUG "Failed to read the value from av8100 register\n");
		retval = -EFAULT;
		goto av8100_register_read_out;
	}

av8100_register_read_out:
	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_configuration_get(enum av8100_command_type command_type,
	union av8100_configuration *config)
{
	if (!av8100_config || !config)
		return AV8100_FAIL;

	/* Put configuration data to the corresponding data struct depending
	 * on command type */
	switch (command_type) {
	case AV8100_COMMAND_VIDEO_INPUT_FORMAT:
		memcpy(&config->video_input_format,
			&av8100_config->hdmi_video_input_cmd,
			sizeof(struct av8100_video_input_format_cmd));
		break;

	case AV8100_COMMAND_AUDIO_INPUT_FORMAT:
		memcpy(&config->audio_input_format,
			&av8100_config->hdmi_audio_input_cmd,
			sizeof(struct av8100_audio_input_format_cmd));
		break;

	case AV8100_COMMAND_VIDEO_OUTPUT_FORMAT:
		memcpy(&config->video_output_format,
			&av8100_config->hdmi_video_output_cmd,
			sizeof(struct av8100_video_output_format_cmd));
		break;

	case AV8100_COMMAND_VIDEO_SCALING_FORMAT:
		memcpy(&config->video_scaling_format,
			&av8100_config->hdmi_video_scaling_cmd,
			sizeof(struct av8100_video_scaling_format_cmd));
		break;

	case AV8100_COMMAND_COLORSPACECONVERSION:
		memcpy(&config->color_space_conversion_format,
			&av8100_config->hdmi_color_space_conversion_cmd,
			sizeof(struct
				av8100_color_space_conversion_format_cmd));
		break;

	case AV8100_COMMAND_CEC_MESSAGE_WRITE:
		memcpy(&config->cec_message_write_format,
			&av8100_config->hdmi_cec_message_write_cmd,
			sizeof(struct av8100_cec_message_write_format_cmd));
		break;

	case AV8100_COMMAND_CEC_MESSAGE_READ_BACK:
		memcpy(&config->cec_message_read_back_format,
			&av8100_config->hdmi_cec_message_read_back_cmd,
			sizeof(struct av8100_cec_message_read_back_format_cmd));
		break;

	case AV8100_COMMAND_DENC:
		memcpy(&config->denc_format, &av8100_config->hdmi_denc_cmd,
				sizeof(struct av8100_denc_format_cmd));
		break;

	case AV8100_COMMAND_HDMI:
		memcpy(&config->hdmi_format, &av8100_config->hdmi_cmd,
				sizeof(struct av8100_hdmi_cmd));
		break;

	case AV8100_COMMAND_HDCP_SENDKEY:
		memcpy(&config->hdcp_send_key_format,
			&av8100_config->hdmi_hdcp_send_key_cmd,
			sizeof(struct av8100_hdcp_send_key_format_cmd));
		break;

	case AV8100_COMMAND_HDCP_MANAGEMENT:
		memcpy(&config->hdcp_management_format,
			&av8100_config->hdmi_hdcp_management_format_cmd,
			sizeof(struct av8100_hdcp_management_format_cmd));
		break;

	case AV8100_COMMAND_INFOFRAMES:
		memcpy(&config->infoframes_format,
			&av8100_config->hdmi_infoframes_cmd,
			sizeof(struct av8100_infoframes_format_cmd));
		break;

	case AV8100_COMMAND_EDID_SECTION_READBACK:
		memcpy(&config->edid_section_readback_format,
			&av8100_config->hdmi_edid_section_readback_cmd,
			sizeof(struct
				av8100_edid_section_readback_format_cmd));
		break;

	case AV8100_COMMAND_PATTERNGENERATOR:
		memcpy(&config->pattern_generator_format,
			&av8100_config->hdmi_pattern_generator_cmd,
			sizeof(struct av8100_pattern_generator_format_cmd));
		break;

	case AV8100_COMMAND_FUSE_AES_KEY:
		memcpy(&config->fuse_aes_key_format,
			&av8100_config->hdmi_fuse_aes_key_cmd,
			sizeof(struct av8100_fuse_aes_key_format_cmd));
		break;

	default:
		return AV8100_FAIL;
		break;
	}

	return 0;
}

int av8100_configuration_prepare(enum av8100_command_type command_type,
	union av8100_configuration *config)
{
	if (!av8100_config || !config)
		return AV8100_FAIL;

	/* Put configuration data to the corresponding data struct depending
	 * on command type */
	switch (command_type) {
	case AV8100_COMMAND_VIDEO_INPUT_FORMAT:
		memcpy(&av8100_config->hdmi_video_input_cmd,
			&config->video_input_format,
			sizeof(struct av8100_video_input_format_cmd));
		break;

	case AV8100_COMMAND_AUDIO_INPUT_FORMAT:
		memcpy(&av8100_config->hdmi_audio_input_cmd,
			&config->audio_input_format,
			sizeof(struct av8100_audio_input_format_cmd));
		break;

	case AV8100_COMMAND_VIDEO_OUTPUT_FORMAT:
		memcpy(&av8100_config->hdmi_video_output_cmd,
			&config->video_output_format,
			sizeof(struct av8100_video_output_format_cmd));

		/* Set params that depend on video output */
		av8100_config_video_output_dep(av8100_config->
			hdmi_video_output_cmd.video_output_cea_vesa);
		break;

	case AV8100_COMMAND_VIDEO_SCALING_FORMAT:
		memcpy(&av8100_config->hdmi_video_scaling_cmd,
			&config->video_scaling_format,
			sizeof(struct av8100_video_scaling_format_cmd));
		break;

	case AV8100_COMMAND_COLORSPACECONVERSION:
		memcpy(&av8100_config->hdmi_color_space_conversion_cmd,
			&config->color_space_conversion_format,
			sizeof(struct
				av8100_color_space_conversion_format_cmd));
		break;

	case AV8100_COMMAND_CEC_MESSAGE_WRITE:
		memcpy(&av8100_config->hdmi_cec_message_write_cmd,
			&config->cec_message_write_format,
			sizeof(struct av8100_cec_message_write_format_cmd));
		break;

	case AV8100_COMMAND_CEC_MESSAGE_READ_BACK:
		memcpy(&av8100_config->hdmi_cec_message_read_back_cmd,
			&config->cec_message_read_back_format,
			sizeof(struct av8100_cec_message_read_back_format_cmd));
		break;

	case AV8100_COMMAND_DENC:
		memcpy(&av8100_config->hdmi_denc_cmd, &config->denc_format,
				sizeof(struct av8100_denc_format_cmd));
		break;

	case AV8100_COMMAND_HDMI:
		memcpy(&av8100_config->hdmi_cmd, &config->hdmi_format,
				sizeof(struct av8100_hdmi_cmd));
		break;

	case AV8100_COMMAND_HDCP_SENDKEY:
		memcpy(&av8100_config->hdmi_hdcp_send_key_cmd,
			&config->hdcp_send_key_format,
			sizeof(struct av8100_hdcp_send_key_format_cmd));
		break;

	case AV8100_COMMAND_HDCP_MANAGEMENT:
		memcpy(&av8100_config->hdmi_hdcp_management_format_cmd,
			&config->hdcp_management_format,
			sizeof(struct av8100_hdcp_management_format_cmd));
		break;

	case AV8100_COMMAND_INFOFRAMES:
		memcpy(&av8100_config->hdmi_infoframes_cmd,
			&config->infoframes_format,
			sizeof(struct av8100_infoframes_format_cmd));
		break;

	case AV8100_COMMAND_EDID_SECTION_READBACK:
		memcpy(&av8100_config->hdmi_edid_section_readback_cmd,
			&config->edid_section_readback_format,
			sizeof(struct
				av8100_edid_section_readback_format_cmd));
		break;

	case AV8100_COMMAND_PATTERNGENERATOR:
		memcpy(&av8100_config->hdmi_pattern_generator_cmd,
			&config->pattern_generator_format,
			sizeof(struct av8100_pattern_generator_format_cmd));
		break;

	case AV8100_COMMAND_FUSE_AES_KEY:
		memcpy(&av8100_config->hdmi_fuse_aes_key_cmd,
			&config->fuse_aes_key_format,
			sizeof(struct av8100_fuse_aes_key_format_cmd));
		break;

	default:
		return AV8100_FAIL;
		break;
	}

	return 0;
}

int av8100_configuration_write(enum av8100_command_type command_type,
	u8 *return_buffer_length,
	u8 *return_buffer, enum interface_type if_type)
{
	int retval = 0;
	u8 cmd_buffer[AV8100_COMMAND_MAX_LENGTH];
	u32 cmd_length = 0;
	struct i2c_client *i2c;

	if (return_buffer_length)
		*return_buffer_length = 0;

	if (!av8100_config)
		return AV8100_FAIL;

	i2c = av8100_config->client;

	memset(&cmd_buffer, 0x00, AV8100_COMMAND_MAX_LENGTH);

#ifdef AV8100_DEBUG_EXTRA
#define PRNK_MODE(_m) printk(KERN_DEBUG "cmd: " #_m "\n");
#else
#define PRNK_MODE(_m)
#endif

	/* Fill the command buffer with configuration data */
	switch (command_type) {
	case AV8100_COMMAND_VIDEO_INPUT_FORMAT:
		PRNK_MODE(AV8100_COMMAND_VIDEO_INPUT_FORMAT);
		configuration_video_input_get(cmd_buffer, &cmd_length);
		break;

	case AV8100_COMMAND_AUDIO_INPUT_FORMAT:
		PRNK_MODE(AV8100_COMMAND_AUDIO_INPUT_FORMAT);
		configuration_audio_input_get(cmd_buffer, &cmd_length);
		break;

	case AV8100_COMMAND_VIDEO_OUTPUT_FORMAT:
		PRNK_MODE(AV8100_COMMAND_VIDEO_OUTPUT_FORMAT);
		configuration_video_output_get(cmd_buffer, &cmd_length);
		break;

	case AV8100_COMMAND_VIDEO_SCALING_FORMAT:
		PRNK_MODE(AV8100_COMMAND_VIDEO_SCALING_FORMAT);
		configuration_video_scaling_get(cmd_buffer,
			&cmd_length);
		break;

	case AV8100_COMMAND_COLORSPACECONVERSION:
		PRNK_MODE(AV8100_COMMAND_COLORSPACECONVERSION);
		configuration_colorspace_conversion_get(cmd_buffer,
			&cmd_length);
		break;

	case AV8100_COMMAND_CEC_MESSAGE_WRITE:
		PRNK_MODE(AV8100_COMMAND_CEC_MESSAGE_WRITE);
		configuration_cec_message_write_get(cmd_buffer,
			&cmd_length);
		break;

	case AV8100_COMMAND_CEC_MESSAGE_READ_BACK:
		PRNK_MODE(AV8100_COMMAND_CEC_MESSAGE_READ_BACK);
		configuration_cec_message_read_get(cmd_buffer,
			&cmd_length);
		break;

	case AV8100_COMMAND_DENC:
		PRNK_MODE(AV8100_COMMAND_DENC);
		configuration_denc_get(cmd_buffer, &cmd_length);
		break;

	case AV8100_COMMAND_HDMI:
		PRNK_MODE(AV8100_COMMAND_HDMI);
		configuration_hdmi_get(cmd_buffer, &cmd_length);
		break;

	case AV8100_COMMAND_HDCP_SENDKEY:
		PRNK_MODE(AV8100_COMMAND_HDCP_SENDKEY);
		configuration_hdcp_sendkey_get(cmd_buffer, &cmd_length);
		break;

	case AV8100_COMMAND_HDCP_MANAGEMENT:
		PRNK_MODE(AV8100_COMMAND_HDCP_MANAGEMENT);
		configuration_hdcp_management_get(cmd_buffer,
			&cmd_length);
		break;

	case AV8100_COMMAND_INFOFRAMES:
		PRNK_MODE(AV8100_COMMAND_INFOFRAMES);
		configuration_infoframe_get(cmd_buffer, &cmd_length);
		break;

	case AV8100_COMMAND_EDID_SECTION_READBACK:
		PRNK_MODE(AV8100_COMMAND_EDID_SECTION_READBACK);
		av8100_edid_section_readback_get(cmd_buffer, &cmd_length);
		break;

	case AV8100_COMMAND_PATTERNGENERATOR:
		PRNK_MODE(AV8100_COMMAND_PATTERNGENERATOR);
		configuration_pattern_generator_get(cmd_buffer,
			&cmd_length);
		break;

	case AV8100_COMMAND_FUSE_AES_KEY:
		PRNK_MODE(AV8100_COMMAND_FUSE_AES_KEY);
		configuration_fuse_aes_key_get(cmd_buffer, &cmd_length);
		break;

	default:
		printk(KERN_INFO "Invalid command type\n");
		retval = AV8100_INVALID_COMMAND;
		break;
	}

	LOCK_AV8100_HW;

	if (if_type == I2C_INTERFACE) {
#ifdef AV8100_DEBUG_EXTRA
		{
		int cnt = 0;
		printk(KERN_DEBUG "av8100_configuration_write cmd_type:%02x length:%02x ",
			command_type, cmd_length);
		printk(KERN_DEBUG "buffer: ");
		while (cnt < cmd_length) {
			printk(KERN_DEBUG "%02x ", cmd_buffer[cnt]);
			cnt++;
		}
		printk(KERN_DEBUG "\n");
		}
#endif

		/* Write the command buffer */
		retval = write_multi_byte(i2c,
			AV8100_COMMAND_OFFSET + 1, cmd_buffer, cmd_length);
		if (retval) {
			UNLOCK_AV8100_HW;
			return retval;
		}

		/* Write the command */
		retval = write_single_byte(i2c, AV8100_COMMAND_OFFSET,
			command_type);
		if (retval) {
			UNLOCK_AV8100_HW;
			return retval;
		}

		/* TODO */
		mdelay(100);

		retval = get_command_return_data(i2c, command_type, cmd_buffer,
			return_buffer_length, return_buffer);
	} else if (if_type == DSI_INTERFACE) {
		/* TODO */
	} else {
		retval = AV8100_INVALID_INTERFACE;
		printk(KERN_INFO "Invalid command type\n");
	}

	if (command_type == AV8100_COMMAND_HDMI) {
		g_av8100_status.hdmi_on = ((av8100_config->hdmi_cmd.
			hdmi_mode == AV8100_HDMI_ON) &&
			(av8100_config->hdmi_cmd.hdmi_format == AV8100_HDMI));
	}

	UNLOCK_AV8100_HW;
	return retval;
}

int av8100_configuration_write_raw(enum av8100_command_type command_type,
	u8 buffer_length,
	u8 *buffer,
	u8 *return_buffer_length,
	u8 *return_buffer)
{
	int retval = 0;
	struct i2c_client *i2c;

	LOCK_AV8100_HW;

	if (return_buffer_length)
		*return_buffer_length = 0;

	if (!av8100_config) {
		retval = AV8100_FAIL;
		goto av8100_configuration_write_raw_out;
	}

	i2c = av8100_config->client;

	/* Write the command buffer */
	retval = write_multi_byte(i2c,
		AV8100_COMMAND_OFFSET + 1, buffer, buffer_length);
	if (retval)
		goto av8100_configuration_write_raw_out;

	/* Write the command */
	retval = write_single_byte(i2c, AV8100_COMMAND_OFFSET,
		command_type);
	if (retval)
		goto av8100_configuration_write_raw_out;

	/* TODO */
	mdelay(100);

	retval = get_command_return_data(i2c, command_type, buffer,
		return_buffer_length, return_buffer);

av8100_configuration_write_raw_out:
	UNLOCK_AV8100_HW;
	return retval;
}

struct av8100_status av8100_status_get(void)
{
	return g_av8100_status;
}

enum av8100_output_CEA_VESA av8100_video_output_format_get(int xres,
	int yres,
	int htot,
	int vtot,
	int pixelclk,
	bool interlaced)
{
	enum av8100_output_CEA_VESA index = 1;
	int yres_div = !interlaced ? 1 : 2;
	int hres_div = 1;
	long freq1;
	long freq2;

	/*
	* 720_576_I need a divider for hact and htot since
	* these params need to be twice as large as expected in av8100_all_cea,
	* which is used as input parameter to video input config.
	*/
	if ((xres == 720) && (yres == 576) && (interlaced == true))
		hres_div = 2;

	freq1 = 1000000 / htot * 1000000 / vtot / pixelclk + 1;
	while (index < sizeof(av8100_all_cea)/sizeof(struct av8100_cea)) {
		freq2 = av8100_all_cea[index].frequence /
			av8100_all_cea[index].htotale /
			av8100_all_cea[index].vtotale;

		printk(KERN_DEBUG "freq1:%ld freq2:%ld\n", freq1, freq2);
		if ((xres == av8100_all_cea[index].hactive / hres_div) &&
			(yres == av8100_all_cea[index].vactive * yres_div) &&
			(htot == av8100_all_cea[index].htotale / hres_div) &&
			(vtot == av8100_all_cea[index].vtotale) &&
			(abs(freq1 - freq2) < 2)) {
			break;
		}
		index++;
	}

	return index;
}

static int av8100_open(struct inode *inode, struct file *filp)
{
	printk(KERN_DEBUG "av8100_open is called\n");
	return 0;
}

static int av8100_release(struct inode *inode, struct file *filp)
{
	printk(KERN_DEBUG "av8100_release is called\n");
	return 0;
}

static int av8100_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
	return 0;
}

static int __devinit av8100_probe(struct i2c_client *i2cClient,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct av8100_platform_data *pdata = i2cClient->dev.platform_data;

	printk(KERN_DEBUG "%s\n", __func__);

	g_av8100_status.av8100_state = AV8100_OPMODE_SHUTDOWN;
	g_av8100_status.av8100_plugin_status = AV8100_PLUGIN_NONE;
	g_av8100_status.hdmi_on = false;

	ret = av8100_config_init();
	if (ret) {
		pr_info("av8100_config_init failed\n");
		goto err;
	}

	if (!i2c_check_functionality(i2cClient->adapter,
		I2C_FUNC_SMBUS_BYTE_DATA |
		I2C_FUNC_SMBUS_READ_WORD_DATA)) {
		ret = -ENODEV;
		pr_info("av8100 i2c_check_functionality failed\n");
		goto err;
	}

	init_waitqueue_head(&av8100_event);

	av8100_config->client = i2cClient;
	av8100_config->id = (struct i2c_device_id *) id;
	i2c_set_clientdata(i2cClient, av8100_config);

	kthread_run(av8100_thread, NULL, "hdmi_thread");

	ret = request_irq(pdata->irq, av8100_intr_handler,
			IRQF_TRIGGER_RISING, "av8100", av8100_config);
	if (ret) {
		printk(KERN_ERR	"av8100_hw request_irq %d failed %d\n",
			pdata->irq, ret);
		gpio_free(pdata->irq);
		goto err;
	}

	return ret;
err:
	return ret;
}

static int __devexit av8100_remove(struct i2c_client *i2cClient)
{
	printk(KERN_DEBUG "%s\n", __func__);

	av8100_config_exit();

	return 0;
}

int av8100_init(void)
{
	int ret;

	printk(KERN_DEBUG "%s\n", __func__);

	ret = i2c_add_driver(&av8100_driver);
	if (ret) {
		printk(KERN_DEBUG "av8100 i2c_add_driver failed\n");
		goto av8100_init_err;
	}

	ret = misc_register(&av8100_miscdev);
	if (ret) {
		printk(KERN_DEBUG "av8100 misc_register failed\n");
		goto av8100_init_err;
	}

	hdmi_init();

	return ret;

av8100_init_err:
	return ret;
}
module_init(av8100_init);

void av8100_exit(void)
{
	printk(KERN_DEBUG "%s\n", __func__);

	hdmi_exit();

	misc_deregister(&av8100_miscdev);
	i2c_del_driver(&av8100_driver);
}
module_exit(av8100_exit);

MODULE_AUTHOR("Per Persson <per.xb.persson@stericsson.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ST-Ericsson hdmi display driver");
