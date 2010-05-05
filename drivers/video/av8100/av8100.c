/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License terms:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/smp_lock.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <asm/ioctl.h>
#include <asm/bitops.h>
#include <asm/io.h>
#include <asm/types.h>
#include <mach/hardware.h>
#include <mach/debug.h>
#include <linux/gpio.h>
#include <mach/mcde.h>
#include <mach/dsi.h>

#include <mach/av8100_p.h>
#include <mach/av8100_fw.h>

#define DRIVER_NAME	(av8100_driver.driver.name)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JAYARAMI REDDY");
MODULE_DESCRIPTION("AV8100 driver for U8500");

extern void u8500_mcde_tasklet_4(unsigned long);

/*#define AV8100_USE_KERNEL_THREAD*/

#if !defined CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_HDMI &&	\
	!defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_HDMI &&	\
	!defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV
#define TEST_PATTERN_TEST
#endif

#define HDMI_LOGGING
#define CONFIG_MCDE_COLOURED_PRINTS

#define PRNK_COL_BLACK		30
#define PRNK_COL_RED		31
#define PRNK_COL_GREEN		32
#define PRNK_COL_YELLOW		33
#define PRNK_COL_BLUE		34
#define PRNK_COL_MAGENTA	35
#define PRNK_COL_CYAN		36
#define PRNK_COL_WHITE		37

#ifdef CONFIG_MCDE_COLOURED_PRINTS
#define PRNK_COL(_col)	printk(KERN_ERR "%c[0;%d;40m\n", 0x1b, _col)
#else /* CONFIG_MCDE_COLOURED_PRINTS */
#define PRNK_COL(_col)
#endif /* CONFIG_MCDE_COLOURED_PRINTS */

#ifdef HDMI_LOGGING
#define HDMI_TRACE do {\
	PRNK_COL(PRNK_COL_YELLOW);\
	printk(KERN_DEBUG "HDMI send cmd %s\n", __func__);\
	PRNK_COL(PRNK_COL_WHITE);\
} while (0)
#else
#define HDMI_TRACE
#endif

static int av8100_open(struct inode *inode, struct file *filp);
static int av8100_release(struct inode *inode, struct file *filp);
static int av8100_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static void av8100_configure_hdmi(struct i2c_client *i2c);
static void av8100_configure_denc(struct av8100_data *av8100DataTemp);
static irqreturn_t av8100_intr_handler(int irq, void *p);

static int av8100_write_multi_byte(struct i2c_client *client, unsigned char regOffset,unsigned char *buf,unsigned char nbytes);
static int av8100_write_single_byte(struct i2c_client *client, unsigned char reg,unsigned char data);
#if 0
static int av8100_read_multi_byte(struct i2c_client *client,unsigned char reg,unsigned char *buf, unsigned char nbytes);
#endif
static int av8100_read_single_byte(struct i2c_client *client,unsigned char reg, unsigned char* val);
static void av8100_hdmi_on(struct av8100_data *av8100DataTemp);
static void av8100_hdmi_off(struct av8100_data *av8100DataTemp);
static int configure_av8100_video_input(char* buffer_temp);
static int configure_av8100_audio_input(char* buffer_temp);
static int configure_av8100_video_output(char* buffer_temp);
static int configure_av8100_video_scaling(char* buffer_temp);
static int configure_av8100_colorspace_conversion(char* buffer_temp);
static int configure_av8100_cec_message_write(char* buffer_temp);
static int configure_av8100_cec_message_read(char* buffer_temp);
static int configure_av8100_denc(char* buffer_temp);
static int configure_av8100_hdmi(char* buffer_temp);
static int configure_av8100_hdcp_senkey(char* buffer_temp);
static int configure_av8100_hdcp_management(char* buffer_temp);
static int configure_av8100_infoframe(char* buffer_temp);
static int configure_av8100_pattern_generator(char* buffer_temp);
#if 0
static int read_edid_info(struct i2c_client *i2c, char* buff);
#endif
static int av8100_send_command (struct i2c_client *i2cClient, char command_type, enum interface if_type);
static int av8100_powerdown(void);
static int av8100_powerup(struct i2c_client *i2c, const struct i2c_device_id *id);
static int av8100_enable_interrupt(struct i2c_client *i2c);
static int av8100_probe(struct i2c_client *i2cClient,const struct i2c_device_id *id);
static int __exit av8100_remove(struct i2c_client *i2cClient);
static int av8100_download_firmware(struct i2c_client *i2c, char regoffset, char* fw_buff, int numOfBytes, enum interface if_type);
static unsigned short av8100_get_ui_x4(av8100_output_CEA_VESA output_video_format);
static unsigned short av8100_get_te_line_nb(av8100_output_CEA_VESA output_video_format);
static void av8100_init_config_params(void);
static void av8100_config_output_dep_params(void);
#if defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_HDMI ||\
	defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV
static mcde_video_mode av8100_get_mcde_video_mode(av8100_output_CEA_VESA format);
#endif
static void av8100_set_state(av8100_operating_mode state);
static av8100_operating_mode av8100_get_state(void);

#define uint8 unsigned char

/** global data */
unsigned int  g_av8100_cmd_length=0;
unsigned int g_dcs_data_count_index = 0;
unsigned int g_dcs_data_count_last = 0;
struct av8100_data *av8100Data;
av8100_operating_mode g_av8100_state = AV8100_OPMODE_SHUTDOWN;

av8100_video_input_format_cmd     hdmi_video_input_cmd;
av8100_video_output_format_cmd    hdmi_video_output_cmd;
av8100_hdmi_cmd                   hdmi_cmd;
av8100_pattern_generator_cmd      hdmi_pattern_generator_cmd;
av8100_audio_input_format_cmd     hdmi_audio_input_cmd;
av8100_color_space_conversion_cmd hdmi_color_conversion_cmd;
av8100_denc_cmd                   hdmi_denc_cmd;

wait_queue_head_t av8100_event;
bool av8100_flag = 0x0;

int length = 0x0;
extern unsigned int isReady;
#define DSI_MAX_DATA_WRITE 15

av8100_cea av8100_all_cea[29] ={
/* cea id                                   cea_nr vtot   vact   vsbpp vslen  vsfp          vpol    htot    hact    hbp  hslen  hfp  freq       hpol rld,bd,uix4,pm,pd */
{ "0  CUSTOM                            "    , 0  ,0    , 0      ,0   , 0     , 0         ,  "-"   , 800   ,640   , 16  , 96   , 10, 25200000     ,"-",0,0,0,0,0},//Settings to be define
{ "1  CEA 1 VESA 4 640x480p @ 60 Hz     "    , 1  ,525  , 480   ,33   , 2     , 10        ,  "-"   , 800   ,640   , 49  , 290  , 146, 25200000    ,"-",2438,1270,6,32,1},//RGB888
{ "2  CEA 2 - 3    720x480p @ 60 Hz 4:3 "    , 2  ,525  , 480   ,30   , 6     , 9         ,  "-"   , 858   ,720   , 34  , 130  , 128, 27027000    ,"-",1828,0x3C0,8,24,1},//RGB565
{ "3  CEA 4        1280x720p @ 60 Hz    "    , 4  ,750  , 720   ,20   , 5     , 5         ,  "+"   , 1650  ,1280  , 114 , 39   , 228, 74250000    ,"+",1706,164,6,32,1},//RGB565
{ "4  CEA 5        1920x1080i @ 60 Hz   "    , 5  ,1125 , 540   ,20   , 5     , 0         ,  "+"   , 2200  ,1920  , 88  , 44   , 10, 74250000     ,"+",0,0,0,0,0},//Settings to be define
{ "5  CEA 6-7      480i (NTSC)          "    , 6  ,525  , 240   ,44   , 5     , 0         ,  "-"   , 1716  ,1440  , 12  , 64   , 10, 27000000     ,"-",0,0,0,0,0},//Settings to be define
{ "6  CEA 14-15    480p @ 60 Hz         "    , 14 ,525  , 480   ,44   , 5     , 0         ,  "-"   , 858   ,720   , 12  , 64   , 10, 27000000     ,"-",0,0,0,0,0},//Settings to be define
{ "7  CEA 16       1920x1080p @ 60 Hz   "    , 16 ,1125 , 1080  ,36   , 5     , 0         ,  "+"   , 1980  ,1280  , 440 , 40   , 10, 74250000     ,"+",0,0,0,0,0},//Settings to be define
{ "8  CEA 17-18    720x576p @ 50 Hz     "    , 17 ,625  , 576   ,44   , 5     , 0         ,  "-"   , 864   ,720   , 12  , 64   , 10, 27000000     ,"-",0,0,0,0,0},//Settings to be define
{ "9  CEA 19       1280x720p @ 50 Hz    "    , 19 ,750  , 720   ,25   , 5     , 0         ,  "+"   , 1980  ,1280  , 440 , 40   , 10, 74250000     ,"+",0,0,0,0,0},//Settings to be define
{ "10 CEA 20       1920 x 1080i @ 50 Hz "    , 20 ,1125 , 540   ,20   , 5     , 0         ,  "+"   , 2640  ,1920  , 528 , 44   , 10, 74250000     ,"+",0,0,0,0,0},//Settings to be define
{ "11 CEA 21-22    576i (PAL)           "    , 21 ,625  , 288   ,44   , 5     , 0         ,  "-"   , 1728  ,1440  , 12  , 64   , 10, 27000000     ,"-",0,0,0,0,0},//Settings to be define
{ "12 CEA 29/30    576p                 "    , 29 ,625  , 576   ,44   , 5     , 0         ,  "-"   , 864   ,720   , 12  , 64   , 10, 27000000     ,"-",0,0,0,0,0},//Settings to be define
{ "13 CEA 31       1080p 50Hz           "    , 31 ,1125 , 1080  ,44   , 5     , 0         ,  "-"   , 2750  ,1920  , 12  , 64   , 10, 27000000     ,"-",0,0,0,0,0},//Settings to be define
{ "14 CEA 32       1920x1080p @ 24 Hz   "    , 32 ,1125 , 1080  ,36   , 5     , 4         ,  "+"   , 2750  ,1920  , 660 , 44   , 153, 74250000    ,"+",2844,0x530,6,32,1},//RGB565
{ "15 CEA 33       1920x1080p @ 25 Hz   "    , 33 ,1125 , 1080  ,36   , 5     , 4         ,  "+"   , 2640  ,1920  , 528 , 44   , 10, 74250000     ,"+",0,0,0,0,0},//Settings to be define
{ "16 CEA 34       1920x1080p @ 30Hz    "    , 34 ,1125 , 1080  ,36   , 5     , 4         ,  "+"   , 2200  ,1920  , 91  , 44   , 153, 74250000    ,"+",2275,0xAB,6,32,1},//RGB565
{ "17 CEA 60       1280x720p @ 24 Hz    "    , 60 ,750  , 720   ,20   , 5     , 5         ,  "+"   , 3300  ,1280  , 284 , 50   , 2276, 29700000   ,"+",4266,0xAD0,5,32,1},//RGB565
{ "18 CEA 61       1280x720p @ 25 Hz    "    , 61 ,750  , 720   ,20   , 5     , 5         ,  "+"   , 3960  ,1280  , 228 , 39   , 2503 ,30937500     ,"+",4096,0x500,5,32,1},//RGB565
{ "19 CEA 62       1280x720p @ 30 Hz    "    , 62 ,750  , 720   ,20   , 5     , 5         ,  "+"   , 3300  ,1280  , 228 , 39   , 1820, 37125000   ,"+",3413,0x770,5,32,1},//RGB565
{ "20 VESA 9       800x600 @ 60 Hz      "   , 109 ,628  , 600   ,28   , 4     , 0         ,  "+"   , 1056  ,800   , 40  , 128  , 10, 40000000     ,"+",0,0,0,0,0},//Settings to be define
{ "21 VESA 14      848x480  @ 60 Hz     "   , 114 ,500  , 480   ,20   , 5     , 0         ,  "+"   , 1056  ,848   , 24  , 80   , 10, 31500000     ,"-",0,0,0,0,0},//Settings to be define
{ "22 VESA 16      1024x768 @ 60 Hz     "   , 116 ,806  , 768   ,38   , 6     , 0         ,  "-"   , 1344  ,1024  , 24  , 135  , 10, 65000000     ,"-",0,0,0,0,0},//Settings to be define
{ "23 VESA 22      1280x768 @ 60 Hz     "   , 122 ,802  , 768   ,34   , 4     , 0         ,  "+"   , 1688  ,1280  , 48  , 160  , 10, 81250000     ,"-",0,0,0,0,0},//Settings to be define
{ "24 VESA 23      1280x768 @ 60 Hz     "   , 123 ,798  , 768   ,30   , 7     , 0         ,  "+"   , 1664  ,1280  , 64  , 128  , 10, 79500000     ,"-",0,0,0,0,0},//Settings to be define
{ "25 VESA 27      1280x800 @ 60 Hz     "   , 127 ,823  , 800   ,23   , 6     , 0         ,  "+"   , 1440  ,1280  , 48  , 32   , 10, 71000000     ,"+",0,0,0,0,0},//Settings to be define
{ "26 VESA 28      1280x800 @ 60 Hz     "   , 128 ,831  , 800   ,31   , 6     , 0         ,  "+"   , 1680  ,1280  , 72  , 128  , 10, 83500000     ,"-",0,0,0,0,0},//Settings to be define
{ "27 VESA 39      1360x768 @ 60 Hz     "   , 139 ,790  , 768   ,22   , 5     , 0         ,  "-"   , 1520  ,1360  , 48  , 32   , 10, 72000000     ,"+",0,0,0,0,0},//Settings to be define
{ "28 VESA 81      1360x768 @ 60 Hz     "   , 181 ,798  , 768   ,30   , 5     , 0         ,  "+"   , 1776  ,1360  , 72  , 136  , 10, 84750000     ,"-",0,0,0,0,0}//Settings to be define
};

/** av8100 file operations */
static struct file_operations av8100_fops = {
	.owner =    THIS_MODULE,
	.open =     av8100_open,
	.release =  av8100_release,
	.ioctl = av8100_ioctl
};

/** av8100 misc device structure */
static struct miscdevice av8100_miscdev =
{
	AV8100_DRIVER_MINOR_NUMBER,
	"av8100",
	&av8100_fops
};

/**
 * av8100_write_multi_byte() - Write a multiple bytes to av8100 chip(av8100) through i2c interface.
 * @client:	i2c client structure
 * @buf:	buffer to be written
 * @nbytes:	nunmber of bytes to be written
 *
 * This funtion uses smbus block write API's to write n number of bytes to the av8100
 **/
static int av8100_write_multi_byte(struct i2c_client *client, uint8 regOffset,
		uint8 *buf,uint8 nbytes)
{
	int ret = AV8100_OK;
	//u8 temp;

	ret = i2c_smbus_write_i2c_block_data(client, regOffset,nbytes, buf);
	if (ret < 0)	{
		printk("i2c smbus write multi byte error\n");
		return ret;
	}
	length += nbytes;
	//printk("length:%d\n", length);
#if 0
	if(nbytes < 40)
	{
		for (temp = 0; temp < nbytes; temp++)
			printk("value:%0x\n", buf[temp]);
		printk("FW size:%d\n", length);
	}
#endif

	/* FIXME- workaround,i2c_smbus_write_i2c_block_data doesn't work on this platform!*/
#if 0
	for(temp=0;temp<nbytes-1;temp++)	{
		ret = i2c_smbus_write_byte_data(client,0xF/*(regOffset)*/,buf[temp]);
		if (ret < 0)	{
			printk("i2c smbus write byte error\n");
			return ret;
		}else
		printk("av8100_write_multi_byte ret:%x\n", ret);
	}
#endif
	return ret;
}

/**
 * av8100_write_single_byte() - Write a single byte to av8100 chip(av8100) through i2c interface.
 * @client:	i2c client structure
 * @reg:	register offset
 * @data:	data byte to be written
 *
 * This funtion uses smbus byte write API to write a single byte to av8100
 **/
static int av8100_write_single_byte(struct i2c_client *client, uint8 reg,
			 uint8 data)
{
	int ret = AV8100_OK;

	//printk("av8100_write_single_byte: reg=%x,data = %x\n", reg,data);
	ret = i2c_smbus_write_byte_data(client,reg,data);
	if(ret < 0)	{
		printk("i2c smbus write byte failed\n");
		return ret;
	}
	return ret;
}

/**
 * av8100_read_multi_byte() - read multiple bytes from av8100 chip(av8100) through i2c interface
 * @client:     i2c client structure
 * @reg:        register offset
 * @buf:	read the data in this buffer
 * @nbytes:	number of bytes to read
 *
 * This funtion uses smbus read block API to read multiple bytes from the reg offset.
 **/
#if 0
static int av8100_read_multi_byte(struct i2c_client *client,uint8 reg,
		uint8 *buf, uint8 nbytes)
{
	int ret = AV8100_OK;

	ret = i2c_smbus_read_i2c_block_data(client,reg,nbytes,buf);
	if(ret < 0){
		printk("i2c smbus read multi byte failed\n");
		return ret;
	}
	if (ret < nbytes){
		printk("i2c smbus read multi byte failed\n");
		return -EIO;
	}
	return ret;
}
#endif

/**
 * av8100_read_single_byte() - read single byte from av8100 chip(av8100) through i2c interface
 * @client:     i2c client structure
 * @reg:        register offset
 * @val:	register value
 *
 * This funtion uses smbus read block API to read single byte from the reg offset.
 **/
static int av8100_read_single_byte(struct i2c_client *client,uint8 reg, unsigned char* val)
{
	int ret = AV8100_OK;
	int value = 0x0;
	value = i2c_smbus_read_byte_data(client, reg);
	if(value < 0){
		printk("i2c smbus read byte failed,read data = %x from offset: %x\n" ,value,reg);
		return ret;
	}
	//printk("read data = %x from offset: %x\n" ,value,reg);
	*val = value;
	return ret;
}

/**
 * get_dcs_data_index_params() - This API is used to get the number of times the data to be sent with the commands.
 **/
void get_dcs_data_index_params(void)
{
	g_dcs_data_count_index = 0;
	g_dcs_data_count_last = 0;
	g_dcs_data_count_index = g_av8100_cmd_length/DSI_MAX_DATA_WRITE;
	g_dcs_data_count_last = g_av8100_cmd_length%DSI_MAX_DATA_WRITE;
}

/**
 * av8100_intr_handler() - To handle av8100 interrupts.
 * @irq:	irq context
 * @av8100Data:av8100 data
 *
 * This funtion handles the av8100 interrupts. It handles the plugin and plugout(HDMI/CVBS) interrupts.
 **/
static irqreturn_t av8100_intr_handler(int irq, void *p)
{

#if 0
	struct av8100_data *av8100DataTemp = p;
#endif

#if 0
	char val = 0x0;
	int retval = AV8100_OK;
	char val1 = 0x0;

	//printk("av8100_intr_handler is called\n");
	//mdelay(100);

	av8100_read_single_byte(av8100DataTemp->client, STANDBY_PENDING_INTERRUPT_REG, &val);
	printk("av8100_intr_handler is called, val:%x, av8100Data->client:%x\n",val, av8100DataTemp->client);
	if (start == 0/*val && HDMI_HOTPLUG_INTERRUPT*/)
	{
			printk("hdmi cable plugin interrupt\n");
		g_av8100_plugin_status = AV8100_HDMI_PLUGIN;
		av8100_set_state(AV8100_OPMODE_INIT);
		val = 0x0;//~ HDMI_HOTPLUG_INTERRUPT_MASK;
		start = 1;

		retval = av8100_write_single_byte(av8100DataTemp->client, STANDBY_PENDING_INTERRUPT_REG, val);
		if(retval != AV8100_OK)
		{
			printk("Failed to write the value in to the av8100 STANDBY_PENDING_INTERRUPT_REG register\n");
			return -EFAULT;
		}
		retval = av8100_send_command (av8100DataTemp->client, AV8100_COMMAND_VIDEO_INPUT_FORMAT, I2C_INTERFACE);
		if(retval != AV8100_OK)
		{
			printk("Failed to send the  AV8100_COMMAND_VIDEO_INPUT_FORMAT command\n");
			return -EFAULT;
		}

		retval = av8100_send_command (av8100DataTemp->client, AV8100_COMMAND_VIDEO_OUTPUT_FORMAT, I2C_INTERFACE);
		if(retval != AV8100_OK)
		{
			printk("Failed to send the  AV8100_COMMAND_VIDEO_INPUT_FORMAT command\n");
			return -EFAULT;
		}

		mcde_hdmi_display_init();
		mcde_configure_hdmi_channel();

		retval = av8100_write_single_byte(av8100DataTemp, GENERAL_INTERRUPT_MASK_REG, 0x60); /* enable TE */
		if(retval != AV8100_OK)
		{
			printk("Failed to write the value in to the av8100 register\n");
			return -EFAULT;
		}

		retval = av8100_send_command (av8100DataTemp->client, AV8100_COMMAND_HDMI, I2C_INTERFACE);
		if(retval != AV8100_OK)
		{
			printk("Failed to send the  AV8100_COMMAND_VIDEO_INPUT_FORMAT command\n");
			return -EFAULT;
		}


	}
	else if (val && CVBS_PLUG_INTERRUPT) {
		g_av8100_plugin_status = AV8100_CVBS_PLUGIN;
		av8100_set_state(AV8100_OPMODE_INIT);
		val = ~CVBS_PLUG_INTERRUPT_MASK;
		av8100_write_single_byte(av8100DataTemp->client, STANDBY_PENDING_INTERRUPT_REG, val);
	}
	else {
		g_av8100_plugin_status = AV8100_PLUGIN_NONE;
	}

	retval = av8100_read_single_byte(av8100DataTemp->client, 0x0/*STANDBY_PENDING_INTERRUPT_REG*/, &val);
	if(retval != AV8100_OK)
	{
		printk("Failed to read the  STANDBY_PENDING_INTERRUPT_REG\n");
		return -EFAULT;
	}

	printk("av8100_read_single_byte(av8100DataTemp->client, GENERAL_INTERRUPT_REG, &val1)\n");
	retval = av8100_read_single_byte(av8100DataTemp->client, GENERAL_INTERRUPT_REG, &val1);
	if(retval != AV8100_OK)
	{
		printk("Failed to read the  GENERAL_INTERRUPT_REG\n");
		return -EFAULT;
	}

	if(val1 && TE_INTERRUPT_MASK)
	{
		//printk("Received the TE interrupt\n");
		retval = av8100_write_single_byte(av8100DataTemp->client, GENERAL_INTERRUPT_REG, TE_INTERRUPT_MASK);
		if(retval != AV8100_OK)
		{
			printk("Failed to write the value in to the av8100 GENERAL_INTERRUPT_REG register\n");
			return -EFAULT;
		}
	}

	if(val1 && UNDER_OVER_FLOW_INTERRUPT_MASK)
	{
		//printk("Received the underflow/overflow interrupt\n");
		retval = av8100_write_single_byte(av8100DataTemp->client, GENERAL_INTERRUPT_REG, UNDER_OVER_FLOW_INTERRUPT_MASK);
		if(retval != AV8100_OK)
		{
			printk("Failed to write the value in to the av8100 GENERAL_INTERRUPT_REG register\n");
			return -EFAULT;
		}
	}
#endif

#ifdef AV8100_USE_KERNEL_THREAD
	av8100_flag = 1;
	wake_up_interruptible(&av8100_event);
#else
	u8500_mcde_tasklet_4(0);
#endif
	return IRQ_HANDLED;
}


/**
 * av8100_configure_hdmi() - To configure the av8100 chip for hdmi.
 * @av8100Data:av8100 data
 *
 * This funtion configures the video input, video output, test pattern and hdmi on commands.
 **/
static void av8100_configure_hdmi(struct i2c_client *i2c)
{
	char val = 0x0;
	int retval = AV8100_OK;

#if 0
	av8100_read_single_byte(i2c, STANDBY_PENDING_INTERRUPT_REG, &val);
	printk("av8100_intr_handler is called, val:%x, av8100Data->client:%x\n",val, i2c);
#endif

	retval = av8100_write_single_byte(i2c, STANDBY_PENDING_INTERRUPT_REG, val);
	if(retval != AV8100_OK)
	{
		printk("Failed to write the value in to the av8100 STANDBY_PENDING_INTERRUPT_REG register\n");
		return;
	}

#ifndef TEST_PATTERN_TEST
	retval = av8100_send_command (i2c, AV8100_COMMAND_VIDEO_INPUT_FORMAT,/*DSI_INTERFACE*/I2C_INTERFACE);
	if(retval != AV8100_OK)
	{
		printk("Failed to send the  AV8100_COMMAND_VIDEO_INPUT_FORMAT command\n");
		return;
	}

#ifdef CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV
	retval = av8100_send_command (i2c, AV8100_COMMAND_COLORSPACECONVERSION, I2C_INTERFACE);
	if(retval != AV8100_OK)
	{
		printk("Failed to send the AV8100_COMMAND_COLORSPACECONVERSION command\n");
		return;
	}
#endif /* CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV */

#else /* TEST_PATTERN_TEST */
#ifndef CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV

	retval = av8100_send_command (i2c, AV8100_COMMAND_PATTERNGENERATOR, /*DSI_INTERFACE*/I2C_INTERFACE);
	if(retval != AV8100_OK)
	{
		printk("Failed to send the  AV8100_COMMAND_PATTERNGENERATOR command\n");
		return;
	}
#endif /* CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV */
#endif
	retval = av8100_send_command (i2c, AV8100_COMMAND_VIDEO_OUTPUT_FORMAT, I2C_INTERFACE);
	if(retval != AV8100_OK)
	{
		printk("Failed to send the  AV8100_COMMAND_VIDEO_OUTPUT_FORMAT command\n");
		return;
	}
#ifndef TEST_PATTERN_TEST
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_HDMI
	mcde_hdmi_display_init_video_mode();
#endif
	retval = av8100_send_command (i2c, AV8100_COMMAND_AUDIO_INPUT_FORMAT, I2C_INTERFACE);
	if(retval != AV8100_OK)
	{
		printk("Failed to send the  AV8100_COMMAND_AUDIO_INPUT_FORMAT command\n");
		return;
	}
#endif /* TEST_PATTERN_TEST */
	av8100_configure_denc(av8100Data);
	return;
}

static void av8100_configure_denc(struct av8100_data *av8100DataTemp)
{
	if (AV8100_OK != av8100_send_command(av8100DataTemp->client,
				AV8100_COMMAND_DENC, I2C_INTERFACE)) {
		printk("Failed to send the  AV8100_COMMAND_DENC command\n");
	}
	return;
}

static void av8100_hdmi_on(struct av8100_data *av8100DataTemp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;
	hdmi_cmd.hdmi_mode = AV8100_HDMI_ON;
	hdmi_cmd.hdmi_format = AV8100_HDMI;

	retval = av8100_send_command (av8100DataTemp->client, AV8100_COMMAND_HDMI, I2C_INTERFACE);
	if(retval != AV8100_OK)
	{
		printk("Failed to send the  AV8100_COMMAND_HDMI command\n");
		return;
	}
	return;
}

static void av8100_hdmi_off(struct av8100_data *av8100DataTemp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;
	hdmi_cmd.hdmi_mode = AV8100_HDMI_OFF;
	hdmi_cmd.hdmi_format = AV8100_HDMI;

	retval = av8100_send_command (av8100DataTemp->client, AV8100_COMMAND_HDMI, I2C_INTERFACE);
	if(retval != AV8100_OK)
	{
		printk("Failed to send the  AV8100_COMMAND_HDMI command\n");
		return;
	}
	return;
}

/**
 * configure_av8100_video_input() - This function will be used to configure the video input of AV8100.
 * @buffer_temp:     configuration pointer
 *
 **/
static int configure_av8100_video_input(char* buffer_temp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;
	buffer_temp[0] = hdmi_video_input_cmd.dsi_input_mode;//mcde_get_dsi_mode_config();
	buffer_temp[1] = hdmi_video_input_cmd.input_pixel_format;//mcde_get_input_bpp();
	buffer_temp[2] = REG_16_8_MSB(hdmi_video_input_cmd.total_horizontal_pixel);
	buffer_temp[3] = REG_16_8_LSB(hdmi_video_input_cmd.total_horizontal_pixel);
	buffer_temp[4] = REG_16_8_MSB(hdmi_video_input_cmd.total_horizontal_active_pixel);
	buffer_temp[5] = REG_16_8_LSB(hdmi_video_input_cmd.total_horizontal_active_pixel);

	buffer_temp[6] = REG_16_8_MSB(hdmi_video_input_cmd.total_vertical_lines);
	buffer_temp[7] = REG_16_8_LSB(hdmi_video_input_cmd.total_vertical_lines);
	buffer_temp[8] = REG_16_8_MSB(hdmi_video_input_cmd.total_vertical_active_lines);
	buffer_temp[9] = REG_16_8_LSB(hdmi_video_input_cmd.total_vertical_active_lines);
	buffer_temp[10] = hdmi_video_input_cmd.video_mode;//mcde_get_hdmi_scan_mode();
	buffer_temp[11] = hdmi_video_input_cmd.nb_data_lane;//AV8100_DATA_LANES_USED_2;
	buffer_temp[12] = hdmi_video_input_cmd.nb_virtual_ch_command_mode;
	buffer_temp[13] = hdmi_video_input_cmd.nb_virtual_ch_video_mode;
	buffer_temp[14] = REG_16_8_MSB(hdmi_video_input_cmd.TE_line_nb);
	buffer_temp[15] = REG_16_8_LSB(hdmi_video_input_cmd.TE_line_nb);
	buffer_temp[16] =  hdmi_video_input_cmd.TE_config;
	buffer_temp[17] = REG_32_8_MSB(hdmi_video_input_cmd.master_clock_freq);
	buffer_temp[18] = REG_32_8_MMSB(hdmi_video_input_cmd.master_clock_freq);
	buffer_temp[19] = REG_32_8_MLSB(hdmi_video_input_cmd.master_clock_freq);
	buffer_temp[20] = REG_32_8_LSB(hdmi_video_input_cmd.master_clock_freq);
	buffer_temp[21] =  hdmi_video_input_cmd.ui_x4; /* UI value */

	g_av8100_cmd_length = AV8100_COMMAND_VIDEO_INPUT_FORMAT_SIZE - 1;
	return retval;

}
/**
 * configure_av8100_audio_output() - This function will be used to configure the audio input of AV8100.
 * @buffer_temp:     configuration pointer
 *
 **/
static int configure_av8100_audio_input(char* buffer_temp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;
	buffer_temp[0] = hdmi_audio_input_cmd.audio_input_if_format;
	buffer_temp[1] = hdmi_audio_input_cmd.i2s_input_nb;
	buffer_temp[2] = hdmi_audio_input_cmd.sample_audio_freq;
	buffer_temp[3] = hdmi_audio_input_cmd.audio_word_lg;
	buffer_temp[4] = hdmi_audio_input_cmd.audio_format;
	buffer_temp[5] = hdmi_audio_input_cmd.audio_if_mode;
	buffer_temp[6] = hdmi_audio_input_cmd.audio_mute;

	g_av8100_cmd_length = AV8100_COMMAND_AUDIO_INPUT_FORMAT_SIZE - 1;
	return retval;
}

/**
 * configure_av8100_video_output() - This function will be used to configure the video output of AV8100.
 * @buffer_temp:     configuration pointer
 *
 **/
static int configure_av8100_video_output(char* buffer_temp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;
	buffer_temp[0] = hdmi_video_output_cmd.video_output_cea_vesa;

	if (buffer_temp[0] == AV8100_CUSTOM) {
		buffer_temp[1] = hdmi_video_output_cmd.vsync_polarity;
		buffer_temp[2] = hdmi_video_output_cmd.hsync_polarity;
		buffer_temp[3] = REG_16_8_MSB(hdmi_video_output_cmd.total_horizontal_pixel);
		buffer_temp[4] = REG_16_8_LSB(hdmi_video_output_cmd.total_horizontal_pixel);
		buffer_temp[5] = REG_16_8_MSB(hdmi_video_output_cmd.total_horizontal_active_pixel);
		buffer_temp[6] = REG_16_8_LSB(hdmi_video_output_cmd.total_horizontal_active_pixel);
		buffer_temp[7] = REG_16_8_MSB(hdmi_video_output_cmd.total_vertical_in_half_lines);
		buffer_temp[8] = REG_16_8_LSB(hdmi_video_output_cmd.total_vertical_in_half_lines);
		buffer_temp[9] = REG_16_8_MSB(hdmi_video_output_cmd.total_vertical_active_in_half_lines);
		buffer_temp[10] = REG_16_8_LSB(hdmi_video_output_cmd.total_vertical_active_in_half_lines);
		buffer_temp[11] = REG_16_8_MSB(hdmi_video_output_cmd.hsync_start_in_pixel);
		buffer_temp[12] = REG_16_8_LSB(hdmi_video_output_cmd.hsync_start_in_pixel);
		buffer_temp[13] = REG_16_8_MSB(hdmi_video_output_cmd.hsync_length_in_pixel);
		buffer_temp[14] = REG_16_8_LSB(hdmi_video_output_cmd.hsync_length_in_pixel);
		buffer_temp[15] = REG_16_8_MSB(hdmi_video_output_cmd.vsync_start_in_half_line);
		buffer_temp[16] =  REG_16_8_LSB(hdmi_video_output_cmd.vsync_start_in_half_line);
		buffer_temp[17] = REG_16_8_MSB(hdmi_video_output_cmd.vsync_length_in_half_line);
		buffer_temp[18] = REG_16_8_LSB(hdmi_video_output_cmd.vsync_length_in_half_line);
		buffer_temp[19] = REG_32_8_MSB(hdmi_video_output_cmd.pixel_clock_freq_Hz);
		buffer_temp[20] = REG_32_8_MMSB(hdmi_video_output_cmd.pixel_clock_freq_Hz);
		buffer_temp[21] = REG_32_8_MLSB(hdmi_video_output_cmd.pixel_clock_freq_Hz);
		buffer_temp[22] = REG_32_8_LSB(hdmi_video_output_cmd.pixel_clock_freq_Hz);

		g_av8100_cmd_length = AV8100_COMMAND_VIDEO_OUTPUT_FORMAT_SIZE - 1;
	}
	else {
		g_av8100_cmd_length = 1;
	}

	return retval;
}

/**
 * configure_av8100_video_scaling() - This function will be used to configure the video scaling params of AV8100.
 * @buffer_temp:     configuration pointer
 *
 **/
static int configure_av8100_video_scaling(char* buffer_temp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;
	g_av8100_cmd_length = AV8100_COMMAND_VIDEO_SCALING_FORMAT_SIZE - 1;
	return retval;
}

/**
 * configure_av8100_colorspace_conversion() - This function will be used to configure the color conversion params of AV8100.
 * @buffer_temp:     configuration pointer
 *
 **/
static int configure_av8100_colorspace_conversion(char* buffer_temp)
{
	int retval = AV8100_OK;
	int i = 0;

	HDMI_TRACE;
	buffer_temp[i++] = REG_16_8_MSB(hdmi_color_conversion_cmd.c0);
	buffer_temp[i++] = REG_16_8_LSB(hdmi_color_conversion_cmd.c0);
	buffer_temp[i++] = REG_16_8_MSB(hdmi_color_conversion_cmd.c1);
	buffer_temp[i++] = REG_16_8_LSB(hdmi_color_conversion_cmd.c1);
	buffer_temp[i++] = REG_16_8_MSB(hdmi_color_conversion_cmd.c2);
	buffer_temp[i++] = REG_16_8_LSB(hdmi_color_conversion_cmd.c2);
	buffer_temp[i++] = REG_16_8_MSB(hdmi_color_conversion_cmd.c3);
	buffer_temp[i++] = REG_16_8_LSB(hdmi_color_conversion_cmd.c3);
	buffer_temp[i++] = REG_16_8_MSB(hdmi_color_conversion_cmd.c4);
	buffer_temp[i++] = REG_16_8_LSB(hdmi_color_conversion_cmd.c4);
	buffer_temp[i++] = REG_16_8_MSB(hdmi_color_conversion_cmd.c5);
	buffer_temp[i++] = REG_16_8_LSB(hdmi_color_conversion_cmd.c5);
	buffer_temp[i++] = REG_16_8_MSB(hdmi_color_conversion_cmd.c6);
	buffer_temp[i++] = REG_16_8_LSB(hdmi_color_conversion_cmd.c6);
	buffer_temp[i++] = REG_16_8_MSB(hdmi_color_conversion_cmd.c7);
	buffer_temp[i++] = REG_16_8_LSB(hdmi_color_conversion_cmd.c7);
	buffer_temp[i++] = REG_16_8_MSB(hdmi_color_conversion_cmd.c8);
	buffer_temp[i++] = REG_16_8_LSB(hdmi_color_conversion_cmd.c8);
	buffer_temp[i++] = REG_16_8_MSB(hdmi_color_conversion_cmd.a_offset);
	buffer_temp[i++] = REG_16_8_LSB(hdmi_color_conversion_cmd.a_offset);
	buffer_temp[i++] = REG_16_8_MSB(hdmi_color_conversion_cmd.b_offset);
	buffer_temp[i++] = REG_16_8_LSB(hdmi_color_conversion_cmd.b_offset);
	buffer_temp[i++] = REG_16_8_MSB(hdmi_color_conversion_cmd.c_offset);
	buffer_temp[i++] = REG_16_8_LSB(hdmi_color_conversion_cmd.c_offset);
	buffer_temp[i++] = hdmi_color_conversion_cmd.l_max;
	buffer_temp[i++] = hdmi_color_conversion_cmd.l_min;
	buffer_temp[i++] = hdmi_color_conversion_cmd.c_max;
	buffer_temp[i++] = hdmi_color_conversion_cmd.c_min;
	g_av8100_cmd_length = i;
	return retval;
}

/**
 * configure_av8100_cec_message_write() - This function will be used to configure the CEC message to AV8100.
 * @buffer_temp:     configuration pointer
 *
 **/
static int configure_av8100_cec_message_write(char* buffer_temp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;
	g_av8100_cmd_length = AV8100_COMMAND_CEC_MESSAGEWRITE_SIZE;
	return retval;
}

/**
 * configure_av8100_cec_message_read() - This function will be used to read CEC message from AV8100.
 * @buffer_temp:     configuration pointer
 *
 **/
static int configure_av8100_cec_message_read(char* buffer_temp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;
	g_av8100_cmd_length = AV8100_COMMAND_CEC_MESSAGEREAD_BACK_SIZE;
	return retval;
}

/**
 * configure_av8100_denc() - This function will be used to configure the denc,
 *                           which is used for SDTV (CVBS).
 * @buffer_temp:     configuration pointer
 *
 **/
static int configure_av8100_denc(char* buffer_temp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;

	/* Start with PAL std format */
	buffer_temp[0] = hdmi_denc_cmd.tv_lines;
	buffer_temp[1] = hdmi_denc_cmd.tv_std;
	buffer_temp[2] = hdmi_denc_cmd.denc;
	buffer_temp[3] = hdmi_denc_cmd.macrovision;
	buffer_temp[4] = hdmi_denc_cmd.internal_generator;
	buffer_temp[5] = hdmi_denc_cmd.chroma;

	g_av8100_cmd_length = AV8100_COMMAND_DENC_SIZE - 1;
	return retval;
}

/**
 * configure_av8100_hdmi() - This function will be used to configure the HDMI .
 * @buffer_temp:     configuration pointer
 *
 **/
static int configure_av8100_hdmi(char* buffer_temp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;
	buffer_temp[0] = hdmi_cmd.hdmi_mode;
	buffer_temp[1] = hdmi_cmd.hdmi_format;
	buffer_temp[2] = hdmi_cmd.dvi_format;

	g_av8100_cmd_length = AV8100_COMMAND_HDMI_SIZE - 1;
	return retval;
}

/**
 * configure_av8100_hdcp_senkey() - This function will be used to configure the hdcp send key.
 * @buffer_temp:     configuration pointer
 *
 **/
static int configure_av8100_hdcp_senkey(char* buffer_temp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;
	g_av8100_cmd_length = AV8100_COMMAND_HDCP_SENDKEY_SIZE;
	return retval;
}

/**
 * configure_av8100_hdcp_management() - This function will be used to configure the hdcp management.
 * @buffer_temp:     configuration pointer
 *
 **/
static int configure_av8100_hdcp_management(char* buffer_temp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;
	g_av8100_cmd_length = AV8100_COMMAND_HDCP_MANAGEMENT_SIZE;
	return retval;
}

/**
 * configure_av8100_infoframe() - This function will be used to configure the info frame.
 * @buffer_temp:     configuration pointer
 *
 **/
static int configure_av8100_infoframe(char* buffer_temp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;
	g_av8100_cmd_length = AV8100_COMMAND_INFOFRAMES_SIZE;
	return retval;
}

/**
 * configure_av8100_pattern_generator() - This function will be used to configure the pattern generator.
 * @buffer_temp:     configuration pointer
 *
 **/
static int configure_av8100_pattern_generator(char* buffer_temp)
{
	int retval = AV8100_OK;

	HDMI_TRACE;
	buffer_temp[0] = hdmi_pattern_generator_cmd.pattern_type;
	buffer_temp[1] = hdmi_pattern_generator_cmd.pattern_video_format;
	buffer_temp[2] = hdmi_pattern_generator_cmd.pattern_audio_mode;

	g_av8100_cmd_length = AV8100_COMMAND_PATTERNGENERATOR_SIZE - 1;
	return retval;
}

/**
 * read_edid_info() - This function will be used to EDID info.
 * @i2c:     i2c client device
 * @buff:    pointer to EDID buffer
 *
 **/
#if 0
static int read_edid_info(struct i2c_client *i2c, char* buff)
{
int retval = AV8100_OK;
return retval;
}
#endif

/**
 * av8100_download_firmware() - This function will be used to download firmware.
 * @i2c:     i2c client device
 * @fw_buff:    pointer to firmware buffer
 * @numOfBytes: number of bytes
 *
 **/
static int av8100_download_firmware(struct i2c_client *i2c, char regOffset, char* fw_buff, int numOfBytes, enum interface if_type)
{
	int retval = AV8100_OK;
	int temp = 0x0;
	int increment = 15, index = 0;
	int size = 0x0, tempnext = 0x0;
	char val = 0x0 ;
	char CheckSum = 0;
	int cnt = 10;

	temp = numOfBytes % increment;
	for(size=0;size<(numOfBytes-temp);size=size+increment, index+=increment)
	{
		if(if_type == I2C_INTERFACE) {
			retval = av8100_write_multi_byte(i2c, regOffset, fw_buff + size, increment);
			if(retval != AV8100_OK) {
				printk("Failed to download the av8100 firmware\n");
				return -EFAULT;
			}
		}
		else if(if_type == DSI_INTERFACE) {
			retval = dsiLPdcslongwrite(0, 0x10, DCS_FW_DOWNLOAD,fw_buff[index],fw_buff[index + 1],fw_buff[index + 2],
					fw_buff[index + 3],fw_buff[index + 4],fw_buff[index + 5],fw_buff[index + 6],fw_buff[index + 7], fw_buff[index + 8],
					fw_buff[index + 9],fw_buff[index + 10],fw_buff[index + 11],fw_buff[index + 12],fw_buff[index + 13], fw_buff[index + 14],
					CHANNEL_A, DSI_LINK2);
			if(retval != AV8100_OK) {
				printk("Failed to send the data through dsi interface\n");
				return retval;
			}
		}
		else
			return AV8100_INVALID_INTERFACE;

		for(tempnext=size;tempnext<(increment+size);tempnext++)
			ReceiveTab[tempnext] = fw_buff[tempnext];
	}

	// Transfer last firmware bytes
	if(if_type == I2C_INTERFACE) {
		retval = av8100_write_multi_byte(i2c, regOffset, fw_buff + size, temp);
		if(retval != AV8100_OK) {
			printk("Failed to download the av8100 firmware\n");
			return -EFAULT;
		}
	}
	else if(if_type == DSI_INTERFACE) {
		retval = dsiLPdcslongwrite(0, temp+1, DCS_FW_DOWNLOAD,fw_buff[index],fw_buff[index + 1],fw_buff[index + 2],
				fw_buff[index + 3],fw_buff[index + 4],fw_buff[index + 5],fw_buff[index + 6],fw_buff[index + 7], fw_buff[index + 8],
				fw_buff[index + 9],fw_buff[index + 10],fw_buff[index + 11],fw_buff[index + 12],fw_buff[index + 13], fw_buff[index + 14],
				CHANNEL_A, DSI_LINK2);

		if(retval != AV8100_OK) {
			printk("Failed to send the data through dsi interface\n");
			return retval;
		}
	}
	else
		return AV8100_INVALID_INTERFACE;

	for(tempnext=size;tempnext<(size+temp);tempnext++) {
		ReceiveTab[tempnext] = fw_buff[tempnext];
	}

	// check transfer
	for(size=0;size<numOfBytes;size++) {
		CheckSum = CheckSum ^ fw_buff[size];
		if(ReceiveTab[size] != fw_buff[size]) {
			printk(">Fw download fail....i=%d\n",size);
			printk("Transm = %x, Receiv = %x\n",fw_buff[size],ReceiveTab[size]);
		}
	}

	retval = av8100_read_single_byte(i2c, regOffset, &val);
	if(retval != AV8100_OK) {
		printk("Failed to read the value in to the av8100 register\n");
		return -EFAULT;
	}

	printk("CheckSum:%x,val:%x\n",CheckSum,val);

	if(CheckSum != val) {
		printk(">Fw downloading.... FAIL CheckSum issue\n");
		printk("Checksum = %d\n",CheckSum);
		printk("Checksum read: %d\n",val);
		return AV8100_FWDOWNLOAD_FAIL;
	}
	else
		printk(">Fw downloading.... success\n");

	/* Set to idle mode */
	retval = av8100_write_single_byte(i2c, GENERAL_CONTROL_REG, 0x0);
	if(retval != AV8100_OK) {
		printk("Failed to write the value in to the av8100 registers\n");
		return -EFAULT;
	}

	/* Wait Internal Micro controler ready */
	cnt = 3;
	retval = av8100_read_single_byte(i2c, GENERAL_STATUS_REG, &val);
	while((retval == AV8100_OK) && (val != AV8100_GENERAL_STATUS_UC_READY) && (cnt-- > 0)) {
		printk("av8100 wait2\n");
		/* TODO */
		for(temp=0;temp<0xFFFFF;temp++);

		retval = av8100_read_single_byte(i2c, GENERAL_STATUS_REG, &val);
	}

	if(retval != AV8100_OK)	{
		printk("Failed to read the value in to the av8100 register\n");
		return -EFAULT;
	}

	av8100_set_state(AV8100_OPMODE_IDLE);
	return retval;
}

/**
 * av8100_send_command() - This function will be used to send the command to AV8100.
 * @i2cClient:     i2c client device
 * @command_type:    command type
 * @if_type: interface type.
 *
 **/
static int av8100_send_command (struct i2c_client *i2cClient, char command_type, enum interface if_type)
{
	int retval = AV8100_OK, temp = 0, index = 0;
	char val = 0x0, val1 = 0x0;
	//char val2 = 0x0, val3 = 0x0;
	int reg_offset = AV8100_COMMAND_OFFSET + 1;
	char buffer[AV8100_COMMAND_MAX_LENGTH];

	g_av8100_cmd_length=0;
	memset(buffer, 0x00, AV8100_COMMAND_MAX_LENGTH);
	switch(command_type)
	{
		case AV8100_COMMAND_VIDEO_INPUT_FORMAT:
			configure_av8100_video_input(buffer);
		break;
		case AV8100_COMMAND_AUDIO_INPUT_FORMAT:
			configure_av8100_audio_input(buffer);
		break;
		case AV8100_COMMAND_VIDEO_OUTPUT_FORMAT:
			configure_av8100_video_output(buffer);
		break;
		case AV8100_COMMAND_VIDEO_SCALING_FORMAT:
			configure_av8100_video_scaling(buffer);
		break;
		case AV8100_COMMAND_COLORSPACECONVERSION:
			configure_av8100_colorspace_conversion(buffer);
		break;
		case AV8100_COMMAND_CEC_MESSAGEWRITE:
			configure_av8100_cec_message_write(buffer);
		break;
		case AV8100_COMMAND_CEC_MESSAGEREAD_BACK:
			configure_av8100_cec_message_read(buffer);
		break;
		case AV8100_COMMAND_DENC:
			configure_av8100_denc(buffer);
		break;
		case AV8100_COMMAND_HDMI:
			configure_av8100_hdmi(buffer);
		break;
		case AV8100_COMMAND_HDCP_SENDKEY:
			configure_av8100_hdcp_senkey(buffer);
		break;
		case AV8100_COMMAND_HDCP_MANAGEMENT:
			configure_av8100_hdcp_management(buffer);
		break;
		case AV8100_COMMAND_INFOFRAMES:
			configure_av8100_infoframe(buffer);
		break;
		case AV8100_COMMAND_EDID_SECTIONREADBACK:
		break;
		case AV8100_COMMAND_PATTERNGENERATOR:
			configure_av8100_pattern_generator(buffer);
		break;
		default:
			printk(KERN_INFO "Invalid command type\n");
			retval = AV8100_INVALID_COMMAND;

	}

#ifdef HDMI_LOGGING
	PRNK_COL(PRNK_COL_YELLOW);
	printk(KERN_DEBUG "HDMI send cmd parameters: ");
	{ int i;
		for (i = 0; i < g_av8100_cmd_length; i++)
			printk("0x%02x ", buffer[i]);
	}
	printk("\n");
	printk(KERN_DEBUG "HDMI send cmd: 0x%02x\n", command_type);
	PRNK_COL(PRNK_COL_WHITE);
#endif /* HDMI_LOGGING */
	if(if_type == I2C_INTERFACE)
	{
		/* writing the command configuration */
		retval = av8100_write_multi_byte(i2cClient, reg_offset, buffer, g_av8100_cmd_length);
		if(retval != AV8100_OK)
			return retval;

		/* writing the command */
		retval = av8100_write_single_byte(i2cClient, AV8100_COMMAND_OFFSET, command_type);
		if(retval != AV8100_OK)
			return retval;
		mdelay(10);
		retval = av8100_read_single_byte(i2cClient, AV8100_COMMAND_OFFSET, &val);
		if(retval != AV8100_OK)
			return retval;

		if(val != (0x80 | command_type)) //0x10 == 0x80 | Identifier
		            return AV8100_COMMAND_FAIL;

		retval = av8100_read_single_byte(i2cClient, AV8100_COMMAND_OFFSET+1, &val1);
		if(retval != AV8100_OK)
			return retval;

        if(val1 != AV8100_OK) // 0x11
            return AV8100_COMMAND_FAIL;


	}else
	if(if_type == DSI_INTERFACE)
	{
		get_dcs_data_index_params();
		for (temp = 0, index = 0; temp < g_dcs_data_count_index; temp++, /*index++*/ index +=15)
		{
#if 0
			retval = dsiLPdcslongwrite(0, 0x10, DCS_WRITE_UC,buffer[index],buffer[++index],buffer[++index],
					buffer[++index],buffer[++index],buffer[++index],buffer[++index],buffer[++index], buffer[++index],
					buffer[++index],buffer[++index],buffer[++index],buffer[++index],buffer[++index], buffer[++index],
					CHANNEL_A, DSI_LINK2);

			if(retval != AV8100_OK)
			{
				printk("Failed to send the data through dsi interface\n");
				return retval;
			}
#endif
			mcde_send_hdmi_cmd_data(&buffer[index],0x10, DCS_WRITE_UC);

		}
		if(g_dcs_data_count_last != 0)
		{
#if 0
			retval = dsiLPdcslongwrite(0, g_dcs_data_count_last+1, DCS_WRITE_UC,buffer[index],buffer[++index],buffer[++index],
					buffer[++index],buffer[++index],buffer[++index],buffer[++index],buffer[++index], buffer[++index],
					buffer[++index],buffer[++index],buffer[++index],buffer[++index],buffer[++index], buffer[++index],
					CHANNEL_A, DSI_LINK2);
			if(retval != AV8100_OK)
			{
				printk("Failed to send the data through dsi interface\n");
				return retval;
			}
#endif

			mcde_send_hdmi_cmd_data(&buffer[index],g_dcs_data_count_last+1, DCS_WRITE_UC);
		}
#if 0
		retval = dsiLPdcsshortwrite1parm(0, DCS_EXEC_UC,command_type, CHANNEL_A, DSI_LINK2);
		if(retval != AV8100_OK)
		{
			printk("Failed to send the command through dsi interface\n");
			return retval;
		}
#endif
#if 0
		mcde_send_hdmi_cmd(&command_type,2, DCS_EXEC_UC);
		dsireaddata(&val, &val1, &val2, &val3, CHANNEL_A, DSI_LINK2);
		printk("data read from dsi val:%x, val1:%x\n",val, val1);
		if(val != (0x80 | command_type)) //0x10 == 0x80 | Identifier
			return AV8100_COMMAND_FAIL;
        if(val1 != AV8100_OK) // 0x11
            return AV8100_COMMAND_FAIL;
		mcde_send_hdmi_cmd(&command_type,2, DCS_EXEC_UC);
#endif
#if 0
		/* writing the command */
		retval = av8100_write_single_byte(i2cClient, AV8100_COMMAND_OFFSET, command_type);
		if(retval != AV8100_OK)
			return retval;
#endif
		mcde_send_hdmi_cmd(&command_type,2, DCS_EXEC_UC);
		mdelay(10);
		retval = av8100_read_single_byte(i2cClient, AV8100_COMMAND_OFFSET, &val);
		if(retval != AV8100_OK)
			return retval;
		printk("value:%x\n", val);
		if(val != (0x80 | command_type)) //0x10 == 0x80 | Identifier
		            return AV8100_COMMAND_FAIL;

		retval = av8100_read_single_byte(i2cClient, AV8100_COMMAND_OFFSET+1, &val1);
		if(retval != AV8100_OK)
			return retval;
printk("value1:%x\n", val1);
        if(val1 != AV8100_OK) // 0x11
            return AV8100_COMMAND_FAIL;

printk("sending command is successful!!!val:%x, val1:%x\n", val, val1);
	}else
	{
		retval = AV8100_INVALID_INTERFACE;
		printk(KERN_INFO "Invalid command type\n");
	}
	return retval;
}

/**
 *
 * av8100_powerdown() - This function will be used to powerdown the AV8100.
 * @i2c:     i2c client device
 * @id:    device id
 *
 **/
static int av8100_powerdown(void)
{
	int retval = AV8100_OK;

	/** reset the av8100 */
	gpio_set_value(GPIO_AV8100_RSTN,GPIO_LOW);

	av8100_set_state(AV8100_OPMODE_SHUTDOWN);
	return retval;
}

/**
 *
 * av8100_powerup() - This function will be used to powerup the AV8100.
 * @i2c:     i2c client device
 * @id:    device id
 *
 **/
static int av8100_powerup(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	int retval = AV8100_OK;
	char val = 0x0;

	/* configuration length */
	g_av8100_cmd_length = 6;
	/** reset the av8100 */
	gpio_set_value(GPIO_AV8100_RSTN, 1);
	av8100_set_state(AV8100_OPMODE_STANDBY);

	retval = av8100_write_single_byte(i2c, STANDBY_REG, 0x3B); /* Device runing, master clock = 38.4Mhz, Enable searching CVBS cable*/
	if(retval != AV8100_OK)
	{
		printk("Failed to write the value in to av8100 register\n");
		return -EFAULT;
	}

	retval = av8100_read_single_byte(i2c, STANDBY_REG, &val);
	if(retval != AV8100_OK)
	{
		printk("Failed to read the value in to av8100 register\n");
		return -EFAULT;
	}else
	printk("STANDBY_REG register value:%0x\n",val);

	retval = av8100_write_single_byte(i2c, AV8100_5_VOLT_TIME_REG, 0x20); /* Define ON TIME & OFF time on 5v HDMI plug detect*/
	if(retval != AV8100_OK)
	{
		printk("Failed to write the value in to av8100 register\n");
		return -EFAULT;
	}
	retval = av8100_read_single_byte(i2c, AV8100_5_VOLT_TIME_REG, &val);
	if(retval != AV8100_OK)
	{
		printk("Failed to read the value in to av8100 register\n");
		return -EFAULT;
	}else
	printk("AV8100_5_VOLT_TIME_REG register value:%0x\n",val);


	retval = av8100_write_single_byte(i2c, GENERAL_CONTROL_REG, 0x30); /* Device in hold mode +  enable firmware download*/
	if(retval != AV8100_OK)
	{
		printk("Failed to write the value in to av8100 register\n");
		return -EFAULT;
	}
	retval = av8100_read_single_byte(i2c, GENERAL_CONTROL_REG, &val);
	if(retval != AV8100_OK)
	{
		printk("Failed to read the value in to av8100 register\n");
		return -EFAULT;
	}else
	printk("GENERAL_CONTROL_REG register value:%0x\n",val);

	av8100_set_state(AV8100_OPMODE_SCAN);
	return retval;
}

/**
 *
 * av8100_enable_interrupt() - This function will enable the hdmi/tvout plugin interrupts.
 * @i2c:     i2c client device
 * @id:    device id
 *
 **/
static int av8100_enable_interrupt(struct i2c_client *i2c)
{
	int retval = AV8100_OK;
#if defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_HDMI ||	\
	defined CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_HDMI||	\
	defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV
	char val = 0x0;
#endif

#if defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_HDMI ||\
	defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV
	val = TE_INTERRUPT;
	retval = av8100_write_single_byte(i2c, GENERAL_INTERRUPT_MASK_REG, val);
	if(retval != AV8100_OK)
	{
		printk("Failed to write the value in to av8100 register\n");
		return -EFAULT;
	}

	val = 0x0;
	retval = av8100_write_single_byte(i2c, STANDBY_INTERRUPT_MASK_REG, val);
	if(retval != AV8100_OK)
	{
		printk("Failed to write the value in to av8100 register\n");
		return -EFAULT;
	}
#endif

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_HDMI
	val = 0x60;//0x60; // tearing & overflow/underflow interrupts
	retval = av8100_write_single_byte(i2c, GENERAL_INTERRUPT_MASK_REG, val);
	if(retval != AV8100_OK)
	{
		printk("Failed to write the value in to av8100 register\n");
		return -EFAULT;
	}
#endif

	return retval;
}

/**
 * av8100_open() - This function will be used to open the av8100 device.
 * @inode:     pointer to inode
 * @filp:	pointer to file structure
 *
 **/
static int av8100_open(struct inode *inode, struct file *filp)
{
	int retval = AV8100_OK;
	printk("av8100_open is called\n");
	return retval;
}

/**
 * av8100_release() - This function will be used to close the av8100 device.
 * @inode:     pointer to inode
 * @filp:	pointer to file structure
 *
 **/
static int av8100_release(struct inode *inode, struct file *filp)
{
	int retval = AV8100_OK;
	printk("av8100_release is called\n");
	return retval;
}

/**
 * av8100_ioctl - 	This routine implements av8100 supported ioctls
 * @inode: 	inode pointer.
 * @file: 	file pointer.
 * @cmd	:	ioctl command.
 * @arg: 	input argument for ioctl.
 *
 * This Api supports the IOC_AV8100_READ_REGISTER,IOC_AV8100_WRITE_REGISTER,IOC_AV8100_SEND_CONFIGURATION_COMMAND
 * IOC_AV8100_READ_CONFIGURATION_COMMAND and IOC_AV8100_GET_STATUS ioctl commands.
 */
static int av8100_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	int								retval = AV8100_OK;
	struct av8100_register			internalReg;
	struct av8100_command_register	commandReg;
	struct av8100_status			status;
	av8100_output_CEA_VESA			video_output_format;
#if defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_HDMI ||\
	defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV
	mcde_video_mode 				mcde_mode;
#endif
	char							val = 0x0;

	/** Process actual ioctl */
	switch(cmd)
	{
	case IOC_AV8100_READ_REGISTER:
		if(copy_from_user(&internalReg, (void *)arg, sizeof(struct av8100_register))) {
			return -EFAULT;
		}

		if (internalReg.offset <= FIRMWARE_DOWNLOAD_ENTRY_REG) {
			retval = av8100_read_single_byte(av8100Data->client, internalReg.offset, &internalReg.value);

			if(retval != AV8100_OK)	{
				printk("Failed to read the value in to the av8100 register\n");
				return -EFAULT;
			}
		}
		else
			return AV8100_INVALID_COMMAND;

		if (copy_to_user((void *)arg, (void *)&internalReg, sizeof(struct av8100_register))) {
			return -EFAULT;
		}
		break;

	case IOC_AV8100_WRITE_REGISTER:
		if(copy_from_user(&internalReg, (void *)arg, sizeof(struct av8100_register))) {
			return -EFAULT;
		}

		if (internalReg.offset <= FIRMWARE_DOWNLOAD_ENTRY_REG) {
			retval = av8100_write_single_byte(av8100Data->client, internalReg.offset, internalReg.value);
			if(retval != AV8100_OK) {
				printk("Failed to write the value in to the av8100 register\n");
				return -EFAULT;
			}
		}
		else
			return AV8100_INVALID_COMMAND;
		//if(copy_to_user((void *)arg, (void *)&internalReg, sizeof(struct av8100_register))) {
		//	return -EFAULT;
		//}
	    break;

	case IOC_AV8100_SEND_CONFIGURATION_COMMAND:
		{
		commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_OK;
		val = 0x0;

		if(copy_from_user(&commandReg, (void *)arg, sizeof(struct av8100_command_register)) != 0) {
			printk("IOC_AV8100_SEND_CONFIGURATION_COMMAND fail 1\n");
			commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
		}

		if(commandReg.return_status == HDMI_COMMAND_RETURN_STATUS_OK) {
			if(commandReg.cmd_id >= AV8100_COMMAND_VIDEO_INPUT_FORMAT && commandReg.cmd_id <= AV8100_COMMAND_PATTERNGENERATOR) {
				/* Writing the command buffer */
				retval = av8100_write_multi_byte(av8100Data->client, AV8100_COMMAND_OFFSET + 1, commandReg.buf, commandReg.buf_len);
				if(retval != AV8100_OK) {
					printk("Failed to send the command to the av8100\n");
					commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
				}
			}
			else {
				commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
			}
		}

		if(commandReg.return_status == HDMI_COMMAND_RETURN_STATUS_OK) {
			/* Writing the command */
			retval = av8100_write_single_byte(av8100Data->client, AV8100_COMMAND_OFFSET, commandReg.cmd_id);
			if(retval != AV8100_OK) {
				printk("IOC_AV8100_SEND_CONFIGURATION_COMMAND fail 2\n");
				commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
			}
		}

		/* TODO */
		mdelay(100);

		if(commandReg.return_status == HDMI_COMMAND_RETURN_STATUS_OK) {
			/* Getting the first return byte */
			retval = av8100_read_single_byte(av8100Data->client, AV8100_COMMAND_OFFSET, &val);
			if(retval != AV8100_OK) {
				printk("IOC_AV8100_SEND_CONFIGURATION_COMMAND fail 3\n");
				commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
			}
			else {
				/* Checking the first return byte */
				if(val != (0x80 | commandReg.cmd_id)) { //offset 0x10 == 0x80 | Identifier
					printk("IOC_AV8100_SEND_CONFIGURATION_COMMAND fail 4 %0x\n", val);
					commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
				}
			}
		}

		commandReg.buf_len = 0;

		switch(commandReg.cmd_id) {
		case AV8100_COMMAND_VIDEO_INPUT_FORMAT:
		case AV8100_COMMAND_AUDIO_INPUT_FORMAT:
		case AV8100_COMMAND_VIDEO_OUTPUT_FORMAT:
		case AV8100_COMMAND_VIDEO_SCALING_FORMAT:
		case AV8100_COMMAND_COLORSPACECONVERSION:
		case AV8100_COMMAND_CEC_MESSAGEWRITE:
		case AV8100_COMMAND_DENC:
		case AV8100_COMMAND_HDMI:
		case AV8100_COMMAND_HDCP_SENDKEY:
		case AV8100_COMMAND_INFOFRAMES:
		case AV8100_COMMAND_PATTERNGENERATOR:
			if(commandReg.return_status == HDMI_COMMAND_RETURN_STATUS_OK) {
				/* Getting the second return byte */
				retval = av8100_read_single_byte(av8100Data->client, AV8100_COMMAND_OFFSET + 1, &val);
				if(retval != AV8100_OK) {
					printk("IOC_AV8100_SEND_CONFIGURATION_COMMAND fail 5\n");
					commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
				}
				else {
					/* Checking the second return byte */
					if (val != AV8100_OK) {
						printk("Invalid Response for the command %0x\n", val);
						commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
					}
				}
			}
			break;

		case AV8100_COMMAND_CEC_MESSAGEREAD_BACK:
			if(commandReg.return_status == HDMI_COMMAND_RETURN_STATUS_OK) {

#if 0
				retval = av8100_read_single_byte(av8100Data->client, AV8100_COMMAND_OFFSET + 1, &val);
				if(retval != AV8100_OK) {
					printk("IOC_AV8100_SEND_CONFIGURATION_COMMAND fail 6a\n");
					commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
				}
				else {
					printk("AV8100_COMMAND_OFFSET + 1:%x\n", val);
				}

				retval = av8100_read_single_byte(av8100Data->client, AV8100_COMMAND_OFFSET + 2, &val);
				if(retval != AV8100_OK) {
					printk("IOC_AV8100_SEND_CONFIGURATION_COMMAND fail 6b\n");
					commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
				}
				else {
					printk("AV8100_COMMAND_OFFSET + 2:%x\n", val);
				}
#endif
				/* Getting the buffer length */
				retval = av8100_read_single_byte(av8100Data->client, AV8100_COMMAND_OFFSET + 3, &val);
				if(retval != AV8100_OK) {
					printk("IOC_AV8100_SEND_CONFIGURATION_COMMAND fail 6\n");
					commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
				}
				else {
					commandReg.buf_len = val;

					if (commandReg.buf_len > HDMI_CEC_MESSAGE_READBACK_MAXSIZE) {
						printk("AV8100_COMMAND_CEC_MESSAGEREAD_BACK buf_len: %d larger than maxsize\n", commandReg.buf_len);
						commandReg.buf_len = 16;
					}
				}
			}

			if(commandReg.return_status == HDMI_COMMAND_RETURN_STATUS_OK) {
				int index = 0;

				/* Getting the buffer */
				while ((index < commandReg.buf_len) && (index < HDMI_CEC_MESSAGE_READBACK_MAXSIZE)) {
					retval = av8100_read_single_byte(av8100Data->client, AV8100_COMMAND_OFFSET + 4 + index, &val);
					if(retval != AV8100_OK) {
						printk("IOC_AV8100_SEND_CONFIGURATION_COMMAND fail 7\n");
						commandReg.buf_len = 0;
						commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
						break;
					}
					else {
						commandReg.buf[index] = val;
					}

					index++;
				}
			}
			break;

		case AV8100_COMMAND_HDCP_MANAGEMENT:
			if(commandReg.return_status == HDMI_COMMAND_RETURN_STATUS_OK) {

				/* Getting the second return byte */
				retval = av8100_read_single_byte(av8100Data->client, AV8100_COMMAND_OFFSET + 1, &val);
				if(retval != AV8100_OK) {
					printk("IOC_AV8100_SEND_CONFIGURATION_COMMAND fail 5\n");
					commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
				}
				else {
					/* Checking the second return byte */
					if (val != AV8100_OK) {
						printk("Invalid Response for the command\n");
						commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
					}
				}
			}

			if(commandReg.return_status == HDMI_COMMAND_RETURN_STATUS_OK) {
				int index = 0;

				/* Getting the buffer length */
				if(commandReg.buf[0] == HDMI_REQUEST_FOR_REVOCATION_LIST_INPUT) {
					commandReg.buf_len = 0x1F;
				}
				else {
					commandReg.buf_len = 0x0;
				}

				/* Getting the buffer */
				while (index < commandReg.buf_len) {
					retval = av8100_read_single_byte(av8100Data->client, AV8100_COMMAND_OFFSET + 2 + index, &val);
					if(retval != AV8100_OK) {
						printk("IOC_AV8100_SEND_CONFIGURATION_COMMAND fail 8\n");
						commandReg.buf_len = 0;
						commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
						break;
					}
					else {
						commandReg.buf[index] = val;
					}

					index++;
				}
			}
			break;

		case AV8100_COMMAND_EDID_SECTIONREADBACK:
			if(commandReg.return_status == HDMI_COMMAND_RETURN_STATUS_OK) {
				int index = 0;

				commandReg.buf_len = 0x80;

				/* Getting the buffer */
				while (index < commandReg.buf_len) {
					retval = av8100_read_single_byte(av8100Data->client, AV8100_COMMAND_OFFSET + 1 + index, &val);
					if(retval != AV8100_OK) {
						printk("IOC_AV8100_SEND_CONFIGURATION_COMMAND fail 9\n");
						commandReg.buf_len = 0;
						commandReg.return_status = HDMI_COMMAND_RETURN_STATUS_FAIL;
						break;
					}
					else {
						commandReg.buf[index] = val;
					}

					index++;
				}
			}
			break;
		}

		if(copy_to_user((void *)arg, (void *)&commandReg, sizeof(struct av8100_command_register)) != 0) {
			return -EFAULT;
		}
		}
	    break;
#if 0
	case IOC_AV8100_READ_CONFIGURATION_COMMAND:
		if(copy_from_user(&commandReg, (void *)arg, sizeof(struct av8100_command_register))) {
			return -EFAULT;
		}

		if(copy_to_user((void *)arg, (void *)&commandReg, sizeof(struct av8100_command_register))) {
			return -EFAULT;
		}
	    break;
#endif
	case IOC_AV8100_GET_STATUS:
		{
		char val = 0;

		status.av8100_state = av8100_get_state();
		status.av8100_plugin_status = 0;

		if(status.av8100_state != AV8100_OPMODE_SHUTDOWN) {
			retval = av8100_read_single_byte(av8100Data->client, STANDBY_REG, &val);
			if(retval != AV8100_OK)
			{
				printk("Failed to read the value in to av8100 register\n");
				return -EFAULT;
			}

			status.av8100_plugin_status = AV8100_PLUGIN_NONE;
			if(val & STANDBY_HPDS_HDMI_PLUGGED) {
				status.av8100_plugin_status |= AV8100_HDMI_PLUGIN;
			}

			if(val & STANDBY_CVBS_TV_CABLE_PLUGGED) {
				status.av8100_plugin_status |= AV8100_CVBS_PLUGIN;
			}
		}

		printk("av8100_state:%0x  av8100_plugin_status:%0x\n", status.av8100_state, status.av8100_plugin_status);

	    if(copy_to_user((void *)arg, (void *)&status, sizeof(struct av8100_status))) {
		return -EFAULT;
		}
		}
	    break;

	case IOC_AV8100_ENABLE:
		/* Use the latest video mode */
#if defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_HDMI ||\
	defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV
		mcde_mode = av8100_get_mcde_video_mode(hdmi_video_output_cmd.video_output_cea_vesa);
		mcde_hdmi_display_init_command_mode(mcde_mode);
#endif

		retval = av8100_powerup(av8100Data->client, 0);
		if(retval != AV8100_OK){
			printk("error in av8100_powerup\n");
			break;
		}

		g_av8100_state = AV8100_OPMODE_INIT;

		retval = av8100_download_firmware(av8100Data->client, FIRMWARE_DOWNLOAD_ENTRY_REG, av8100_fw_buff, fw_size, I2C_INTERFACE);
		if(retval != AV8100_OK){
			printk("error in av8100_download_firmware\n");
			break;
		}

		av8100_configure_hdmi(av8100Data->client);

		retval = av8100_enable_interrupt(av8100Data->client);
		if(retval != AV8100_OK){
			printk("error in av8100_enable_interrupt\n");
			break;
		}

		av8100_hdmi_on(av8100Data);
		break;

	case IOC_AV8100_DISABLE:
		av8100_hdmi_off(av8100Data);
		av8100_powerdown();
		break;

	case IOC_AV8100_SET_VIDEO_FORMAT:
		printk(KERN_INFO "IOC_AV8100_SET_VIDEO_FORMAT\n");

		if(copy_from_user(&video_output_format, (void *)arg, sizeof(av8100_output_CEA_VESA))) {
			return -EFAULT;
		}

		if (video_output_format < AV8100_VIDEO_OUTPUT_CEA_VESA_MAX) {
			/* hdmi_off */
			av8100_hdmi_off(av8100Data);

			/* powerdown */
			av8100_powerdown();

			/* mcde */
			hdmi_video_output_cmd.video_output_cea_vesa = video_output_format;
			av8100_config_output_dep_params();
#if defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_HDMI ||\
	defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV
			mcde_mode = av8100_get_mcde_video_mode(video_output_format);
			mcde_hdmi_display_init_command_mode(mcde_mode);
#endif

			/* powerup */
			retval = av8100_powerup(av8100Data->client, 0);
			if(retval != AV8100_OK){
				printk("error in av8100_powerup\n");
				break;
			}

			g_av8100_state = AV8100_OPMODE_INIT;

			/* download fw */
			retval = av8100_download_firmware(av8100Data->client, FIRMWARE_DOWNLOAD_ENTRY_REG, av8100_fw_buff, fw_size, I2C_INTERFACE);
			if(retval != AV8100_OK){
				printk("error in av8100_download_firmware\n");
				break;
			}

			hdmi_video_input_cmd.TE_line_nb = av8100_get_te_line_nb(video_output_format);
			hdmi_video_input_cmd.ui_x4 = av8100_get_ui_x4(video_output_format);

			/* configure hdmi */
			av8100_configure_hdmi(av8100Data->client);

			/* enable int */
			retval = av8100_enable_interrupt(av8100Data->client);
			if(retval != AV8100_OK){
				printk("error in av8100_enable_interrupt\n");
				break;
			}

			/* hdmi on */
			av8100_hdmi_on(av8100Data);
		}
		else {
			retval = AV8100_INVALID_COMMAND;
		}
		break;

#if 1
//Test
	case IOC_AV8100_HDMI_ON:
		printk(KERN_INFO "IOC_AV8100_HDMI_ON\n");
		av8100_hdmi_on(av8100Data);
		break;

	case IOC_AV8100_HDMI_OFF:
		printk(KERN_INFO "IOC_AV8100_HDMI_OFF\n");
		av8100_hdmi_off(av8100Data);
		break;
#endif

	 default:
	    return AV8100_INVALID_IOCTL;
	}
	return retval;
}

static unsigned short av8100_get_te_line_nb(av8100_output_CEA_VESA output_video_format)
{
	unsigned short retval;

	switch(output_video_format) {
	case AV8100_CEA1_640X480P_59_94HZ:
	case AV8100_CEA2_3_720X480P_59_94HZ:
		retval = 30;
		break;

	case AV8100_CEA5_1920X1080I_60HZ:
	case AV8100_CEA6_7_NTSC_60HZ:
	case AV8100_CEA20_1920X1080I_50HZ:
		retval = 18;
		break;

	case AV8100_CEA4_1280X720P_60HZ:
		retval = 21;
		break;

	case AV8100_CEA17_18_720X576P_50HZ:
		retval = 40;
		break;

	case AV8100_CEA19_1280X720P_50HZ:
		retval = 22;
		break;

	case AV8100_CEA21_22_576I_PAL_50HZ:
		/* Different values below come from LLD,
		 * TODO: check if this is really needed
		 * if not merge with AV8100_CEA6_7_NTSC_60HZ case
		 */
#ifdef CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV
		retval = 18;
#else
		retval = 17;
#endif
		break;

	case AV8100_CEA32_1920X1080P_24HZ:
	case AV8100_CEA33_1920X1080P_25HZ:
	case AV8100_CEA34_1920X1080P_30HZ:
		retval = 38;
		break;

	case AV8100_CEA60_1280X720P_24HZ:
	case AV8100_CEA62_1280X720P_30HZ:
		retval = 21;
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
		retval = 14;
		break;
	}

	return retval;
}

static unsigned short av8100_get_ui_x4(av8100_output_CEA_VESA output_video_format)
{
	unsigned short retval = 6;

	switch(output_video_format) {
	case AV8100_CEA1_640X480P_59_94HZ:
	case AV8100_CEA2_3_720X480P_59_94HZ:
	case AV8100_CEA4_1280X720P_60HZ:
	case AV8100_CEA5_1920X1080I_60HZ:
	case AV8100_CEA6_7_NTSC_60HZ:
	case AV8100_CEA17_18_720X576P_50HZ:
	case AV8100_CEA19_1280X720P_50HZ:
	case AV8100_CEA20_1920X1080I_50HZ:
	case AV8100_CEA21_22_576I_PAL_50HZ:
	case AV8100_CEA32_1920X1080P_24HZ:
	case AV8100_CEA33_1920X1080P_25HZ:
	case AV8100_CEA34_1920X1080P_30HZ:
	case AV8100_CEA60_1280X720P_24HZ:
	case AV8100_CEA62_1280X720P_30HZ:
		retval = 6;
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
		retval = 6;
		break;
	}

	return retval;
}

static void av8100_init_config_params(void)
{
	memset(&hdmi_video_output_cmd, 0x0, sizeof(av8100_video_output_format_cmd));


	/* SDTV mode settings */
#ifdef CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV

	/* Default output mode */
	hdmi_video_output_cmd.video_output_cea_vesa = AV8100_CEA21_22_576I_PAL_50HZ;
#ifdef CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_PAL_THRU_AV8100
	hdmi_video_output_cmd.video_output_cea_vesa = AV8100_CEA21_22_576I_PAL_50HZ;
#elif defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_NTSC_THRU_AV8100
	hdmi_video_output_cmd.video_output_cea_vesa = AV8100_CEA6_7_NTSC_60HZ;
#endif

	hdmi_color_conversion_cmd.c0       = 0xFFDA;
	hdmi_color_conversion_cmd.c1       = 0xFFB6;
	hdmi_color_conversion_cmd.c2       = 0x0070;
	hdmi_color_conversion_cmd.c3       = 0x0042;
	hdmi_color_conversion_cmd.c4       = 0x0081;
	hdmi_color_conversion_cmd.c5       = 0x0019;
	hdmi_color_conversion_cmd.c6       = 0x0070;
	hdmi_color_conversion_cmd.c7       = 0xFFA2;
	hdmi_color_conversion_cmd.c8       = 0xFFEE;
	hdmi_color_conversion_cmd.a_offset = 0x007F;
	hdmi_color_conversion_cmd.b_offset = 0x0010;
        hdmi_color_conversion_cmd.c_offset = 0x007F;
        hdmi_color_conversion_cmd.l_max    = 0xEB;
        hdmi_color_conversion_cmd.l_min    = 0x10;
        hdmi_color_conversion_cmd.c_max    = 0xF0;
        hdmi_color_conversion_cmd.c_min    = 0x10;

#else /* CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV */

	/* Default output mode */
	hdmi_video_output_cmd.video_output_cea_vesa = AV8100_CEA4_1280X720P_60HZ;

	/* HDMI mode settings */
#ifdef CONFIG_FB_U8500_MCDE_HDMI_640x480P60
	hdmi_video_output_cmd.video_output_cea_vesa = AV8100_CEA1_640X480P_59_94HZ;
#endif
#ifdef CONFIG_FB_U8500_MCDE_HDMI_720x480P60
	hdmi_video_output_cmd.video_output_cea_vesa = AV8100_CEA2_3_720X480P_59_94HZ;
#endif
#ifdef CONFIG_FB_U8500_MCDE_HDMI_1280x720P60
	hdmi_video_output_cmd.video_output_cea_vesa = AV8100_CEA4_1280X720P_60HZ;
#endif
#ifdef CONFIG_FB_U8500_MCDE_HDMI_1920x1080I60
	hdmi_video_output_cmd.video_output_cea_vesa = AV8100_CEA5_1920X1080I_60HZ;
#endif
#ifdef CONFIG_FB_U8500_MCDE_HDMI_1920x1080I50
	hdmi_video_output_cmd.video_output_cea_vesa = AV8100_CEA20_1920X1080I_50HZ;
#endif
#ifdef CONFIG_FB_U8500_MCDE_HDMI_1920x1080P30
	hdmi_video_output_cmd.video_output_cea_vesa = AV8100_CEA34_1920X1080P_30HZ;
#endif
#ifdef CONFIG_FB_U8500_MCDE_HDMI_720x576I50
	hdmi_video_output_cmd.video_output_cea_vesa = AV8100_CEA21_22_576I_PAL_50HZ;
#endif
#ifdef CONFIG_FB_U8500_MCDE_HDMI_720x480I60
	hdmi_video_output_cmd.video_output_cea_vesa = AV8100_CEA6_7_NTSC_60HZ;
#endif
#endif /* else of if CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV */
	av8100_config_output_dep_params();
}

static void av8100_config_output_dep_params(void)
{
	memset(&hdmi_cmd, 0x0, sizeof(av8100_hdmi_cmd));
	memset(&hdmi_video_input_cmd, 0x0, sizeof(av8100_video_input_format_cmd));
	memset(&hdmi_pattern_generator_cmd, 0x0, sizeof(av8100_pattern_generator_cmd));
	memset(&hdmi_audio_input_cmd, 0x0, sizeof(av8100_audio_input_format_cmd));
	memset(&hdmi_denc_cmd, 0x0, sizeof(av8100_denc_cmd));

	hdmi_video_input_cmd.dsi_input_mode = AV8100_HDMI_DSI_COMMAND_MODE;
	hdmi_video_input_cmd.total_horizontal_pixel = av8100_all_cea[
			hdmi_video_output_cmd.video_output_cea_vesa].htotale;
	hdmi_video_input_cmd.total_horizontal_active_pixel = av8100_all_cea[
			hdmi_video_output_cmd.video_output_cea_vesa].hactive;
	hdmi_video_input_cmd.total_vertical_lines = av8100_all_cea[
			hdmi_video_output_cmd.video_output_cea_vesa].vtotale;
	hdmi_video_input_cmd.total_vertical_active_lines = av8100_all_cea[
			hdmi_video_output_cmd.video_output_cea_vesa].vactive;

	hdmi_video_input_cmd.nb_data_lane       = AV8100_DATA_LANES_USED_2;
#ifdef CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV
	/* hdmi_video_input_cmd.input_pixel_format = AV8100_INPUT_PIX_YCBCR422;
	 */
	hdmi_video_input_cmd.input_pixel_format = AV8100_INPUT_PIX_RGB565;
	hdmi_video_input_cmd.video_mode         = AV8100_VIDEO_INTERLACE;

	/* only shared for channel A */
	hdmi_video_input_cmd.ui_x4 = av8100_get_ui_x4(
				hdmi_video_output_cmd.video_output_cea_vesa);
	hdmi_video_input_cmd.TE_config = AV8100_TE_IT_LINE;
	hdmi_video_input_cmd.TE_line_nb = av8100_get_te_line_nb(
				hdmi_video_output_cmd.video_output_cea_vesa);

	if (hdmi_video_output_cmd.video_output_cea_vesa ==
						AV8100_CEA21_22_576I_PAL_50HZ
	) {
		hdmi_denc_cmd.tv_lines = AV8100_TV_LINES_625;
		hdmi_denc_cmd.tv_std   = AV8100_TV_STD_PALBDGHI;
	} else if (
		hdmi_video_output_cmd.video_output_cea_vesa ==
						AV8100_CEA6_7_NTSC_60HZ
	) {
		hdmi_denc_cmd.tv_lines = AV8100_TV_LINES_525;
		hdmi_denc_cmd.tv_std   = AV8100_TV_STD_NTSCM;
	} else {
		printk(KERN_WARNING "AV8100: unsupported video mode, using default\n");
		hdmi_denc_cmd.tv_lines = AV8100_TV_LINES_625;
	}
	hdmi_denc_cmd.denc               = AV8100_DENC_ON;
	hdmi_denc_cmd.macrovision        = AV8100_MACROVISION_OFF;

#ifdef TEST_PATTERN_TEST
	hdmi_denc_cmd.internal_generator = AV8100_INTERNAL_GENERATOR_ON;
	hdmi_denc_cmd.chroma             = AV8100_CHROMA_CWS_CAPTURE_ON;
#else
	hdmi_denc_cmd.internal_generator = AV8100_INTERNAL_GENERATOR_OFF;
	hdmi_denc_cmd.chroma             = AV8100_CHROMA_CWS_CAPTURE_OFF;
#endif /* CONFIG_AV8100_TEST_PATTERN */

#else /* CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV */

	/** Config for command mode **/
#ifdef CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_HDMI
	hdmi_video_input_cmd.ui_x4 = av8100_get_ui_x4(hdmi_video_output_cmd.video_output_cea_vesa);
	hdmi_video_input_cmd.TE_line_nb = av8100_get_te_line_nb(hdmi_video_output_cmd.video_output_cea_vesa);
	//	hdmi_video_input_cmd.TE_config = AV8100_TE_DSI_IT; // IT on both I/F (DSI & GPIO)
	hdmi_video_input_cmd.TE_config = AV8100_TE_IT_LINE; // IT on both I/F (DSI & GPIO)
#endif

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_HDMI
	/** Config for video mode **/
	hdmi_video_input_cmd.dsi_input_mode = AV8100_HDMI_DSI_VIDEO_MODE;
	hdmi_video_input_cmd.TE_line_nb = 0;
	hdmi_video_input_cmd.TE_config = AV8100_TE_OFF; // No TE IT
#endif

	hdmi_video_input_cmd.input_pixel_format = AV8100_INPUT_PIX_RGB565;
	if (hdmi_video_output_cmd.video_output_cea_vesa ==
						AV8100_CEA5_1920X1080I_60HZ
		||
		hdmi_video_output_cmd.video_output_cea_vesa ==
						AV8100_CEA20_1920X1080I_50HZ
		||
		hdmi_video_output_cmd.video_output_cea_vesa ==
						AV8100_CEA21_22_576I_PAL_50HZ
		||
		hdmi_video_output_cmd.video_output_cea_vesa ==
						AV8100_CEA6_7_NTSC_60HZ
	) {
		hdmi_video_input_cmd.video_mode = AV8100_VIDEO_INTERLACE;
	} else {
		hdmi_video_input_cmd.video_mode = AV8100_VIDEO_PROGRESSIVE;
	}

	hdmi_pattern_generator_cmd.pattern_audio_mode = AV8100_PATTERN_AUDIO_OFF;
	hdmi_pattern_generator_cmd.pattern_type =  AV8100_PATTERN_GENERATOR;

	if(hdmi_video_output_cmd.video_output_cea_vesa == AV8100_CEA4_1280X720P_60HZ)
        hdmi_pattern_generator_cmd.pattern_video_format = AV8100_PATTERN_720P;
    else if(hdmi_video_output_cmd.video_output_cea_vesa == AV8100_CEA1_640X480P_59_94HZ)
        hdmi_pattern_generator_cmd.pattern_video_format = AV8100_PATTERN_VGA;
    else if(hdmi_video_output_cmd.video_output_cea_vesa == AV8100_CEA34_1920X1080P_30HZ)
        hdmi_pattern_generator_cmd.pattern_video_format = AV8100_PATTERN_1080P;
    else
        hdmi_pattern_generator_cmd.pattern_video_format = AV8100_PATTERN_720P;

	hdmi_denc_cmd.denc = AV8100_DENC_OFF;

	/* default audio configurations in i2s mode*/
	hdmi_audio_input_cmd.audio_input_if_format  =   AV8100_AUDIO_I2SDELAYED_MODE;            // mode of the MSP
	hdmi_audio_input_cmd.i2s_input_nb           =   1;                    // 0, 1 2 3 4
	hdmi_audio_input_cmd.sample_audio_freq      =   AV8100_AUDIO_FREQ_48KHZ;
	hdmi_audio_input_cmd.audio_word_lg          =   AV8100_AUDIO_16BITS;
	hdmi_audio_input_cmd.audio_format           =   AV8100_AUDIO_LPCM_MODE;
	hdmi_audio_input_cmd.audio_if_mode          =   AV8100_AUDIO_MASTER;
	hdmi_audio_input_cmd.audio_mute             =   AV8100_AUDIO_MUTE_DISABLE;

#endif /* if..else CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV */
	// HDMI mode
	hdmi_cmd.hdmi_mode   = AV8100_HDMI_ON;
	hdmi_cmd.hdmi_format = AV8100_HDMI;
	return;
}


#if defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_HDMI ||\
	defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV
static mcde_video_mode av8100_get_mcde_video_mode(av8100_output_CEA_VESA format)
{
	mcde_video_mode retval = VMODE_1280_720_60_P;

#define PRNK_MODE(_m, _n) \
        printk(KERN_DEBUG "AV8100 mapped mode: " #_m " on " #_n "\n");

	switch(format) {
	case AV8100_CEA1_640X480P_59_94HZ:
		PRNK_MODE(AV8100_CEA1_640X480P_59_94HZ, VMODE_640_480_60_P);
		retval = VMODE_640_480_60_P;
		break;
	case AV8100_CEA2_3_720X480P_59_94HZ:
		PRNK_MODE(AV8100_CEA2_3_720X480P_59_94HZ, VMODE_720_480_60_P);
		retval = VMODE_720_480_60_P;
		break;
	case AV8100_CEA5_1920X1080I_60HZ:
		PRNK_MODE(AV8100_CEA5_1920X1080I_60HZ, VMODE_1920_1080_60_I);
		retval = VMODE_1920_1080_60_I;
		break;
	case AV8100_CEA6_7_NTSC_60HZ:
		PRNK_MODE(AV8100_CEA6_7_NTSC_60HZ, VMODE_720_480_60_I);
		retval = VMODE_720_480_60_I;
		break;
	case AV8100_CEA4_1280X720P_60HZ:
		PRNK_MODE(AV8100_CEA4_1280X720P_60HZ, VMODE_1280_720_60_P);
		retval = VMODE_1280_720_60_P;
		break;
	case AV8100_CEA17_18_720X576P_50HZ:
		PRNK_MODE(AV8100_CEA17_18_720X576P_50HZ, VMODE_720_576_50_P);
		retval = VMODE_720_576_50_P;
		break;
	case AV8100_CEA19_1280X720P_50HZ:
		PRNK_MODE(AV8100_CEA19_1280X720P_50HZ, VMODE_1280_720_50_P);
		retval = VMODE_1280_720_50_P;
		break;
	case AV8100_CEA20_1920X1080I_50HZ:
		PRNK_MODE(AV8100_CEA20_1920X1080I_50HZ, VMODE_1920_1080_50_I);
		retval = VMODE_1920_1080_50_I;
		break;
	case AV8100_CEA21_22_576I_PAL_50HZ:
		PRNK_MODE(AV8100_CEA21_22_576I_PAL_50HZ, VMODE_720_576_50_I);
		retval = VMODE_720_576_50_I;
		break;
	case AV8100_CEA32_1920X1080P_24HZ:
		PRNK_MODE(AV8100_CEA32_1920X1080P_24HZ, VMODE_1920_1080_24_P);
		retval = VMODE_1920_1080_24_P;
		break;
	case AV8100_CEA33_1920X1080P_25HZ:
		PRNK_MODE(AV8100_CEA33_1920X1080P_25HZ, VMODE_1920_1080_25_P);
		retval = VMODE_1920_1080_25_P;
		break;
	case AV8100_CEA34_1920X1080P_30HZ:
		PRNK_MODE(AV8100_CEA34_1920X1080P_30HZ, VMODE_1920_1080_30_P);
		retval = VMODE_1920_1080_30_P;
		break;
	case AV8100_CEA60_1280X720P_24HZ:
		PRNK_MODE(AV8100_CEA60_1280X720P_24HZ, VMODE_1280_720_24_P);
		retval = VMODE_1280_720_24_P;
		break;
	case AV8100_CEA62_1280X720P_30HZ:
		PRNK_MODE(AV8100_CEA62_1280X720P_30HZ, VMODE_1280_720_30_P);
		retval = VMODE_1280_720_30_P;
		break;

	case AV8100_VESA9_800X600P_60_32HZ:
	case AV8100_CEA14_15_480p_60HZ:
	case AV8100_CEA16_1920X1080P_60HZ:
	case AV8100_CEA29_30_576P_50HZ:
	case AV8100_CEA31_1920x1080P_50Hz:
	case AV8100_CEA61_1280X720P_25HZ:
	case AV8100_VESA14_848X480P_60HZ:
	case AV8100_VESA16_1024X768P_60HZ:
	case AV8100_VESA22_1280X768P_59_99HZ:
	case AV8100_VESA23_1280X768P_59_87HZ:
	case AV8100_VESA27_1280X800P_59_91HZ:
	case AV8100_VESA28_1280X800P_59_81HZ:
	case AV8100_VESA39_1360X768P_60_02HZ:
	case AV8100_VESA81_1366X768P_59_79HZ:
	default:
		/* TODO */
		PRNK_MODE(AV8100_UNKNOWN_OR_TODO_MODE, VMODE_1280_720_60_P);
		retval = VMODE_1280_720_60_P;
		break;
	}

	return retval;
}
#endif

static void av8100_set_state(av8100_operating_mode state)
{
	g_av8100_state = state;
}

static av8100_operating_mode av8100_get_state(void)
{
	return g_av8100_state;
}


#ifdef AV8100_USE_KERNEL_THREAD
/**
 * av8100_thread() - This function will be used to send the data from 8500(MCDE/VTG) to hdmi chip.
 *
 **/
static int av8100_thread(void)
{
	int retval = AV8100_OK;
	char val = 0x0;

	daemonize("hdmi_feeding_thread");
	allow_signal(SIGKILL);

	while (1)
	{
		wait_event_interruptible(av8100_event, (av8100_flag == 1) );
		av8100_flag = 0;

		/* STANDBY interrupts */
		retval = av8100_read_single_byte(av8100Data->client, STANDBY_PENDING_INTERRUPT_REG, &val);
		if(retval != AV8100_OK)
		{
			printk("Failed to read the  STANDBY_PENDING_INTERRUPT_REG\n");
			return -EFAULT;
		}

		if (val && HDMI_HOTPLUG_INTERRUPT) {
			//printk("HDMI_HOTPLUG_INTERRUPT\n");

			/* TODO */
		}

		if (val && CVBS_PLUG_INTERRUPT) {
			//printk("CVBS_PLUG_INTERRUPT\n");

			/* TODO */
		}

		/* Clear interrupts */
		av8100_write_single_byte(av8100Data->client, STANDBY_PENDING_INTERRUPT_REG, STANDBY_INTERRUPT_MASK_ALL);

		/* General interrupts */
		retval = av8100_read_single_byte(av8100Data->client, GENERAL_INTERRUPT_REG, &val);
		if(retval != AV8100_OK)
		{
			printk("Failed to read the  GENERAL_INTERRUPT_REG\n");
			return -EFAULT;
		}

		if(val & TE_INTERRUPT)
		{
			u8500_mcde_tasklet_4(0);
		}

		if(val & UNDER_OVER_FLOW_INTERRUPT)
		{
			//printk("Received the underflow/overflow interrupt\n");

			/* TODO */
		}

		/* Clear interrupts */
		retval = av8100_write_single_byte(av8100Data->client, GENERAL_INTERRUPT_REG, GENERAL_INTERRUPT_MASK_ALL);
	}
	return 0;
}
#endif

/**
 * av8100_probe() - This function will be used to powerup and initialize the av8100.
 * @i2c:     i2c client device
 * @id:    device id
 *
 **/
static int av8100_probe(struct i2c_client *i2cClient,
			const struct i2c_device_id *id)
{
	int ret= 0;
#if defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_HDMI ||\
	defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV
	mcde_video_mode mcde_mode;
#endif

#ifndef TEST_PATTERN_TEST
	struct av8100_platform_data *pdata = i2cClient->dev.platform_data;
#endif

	//printk(KERN_INFO "av8100 probe\n");
	//printk("av8100 probe\n");

	PRNK_COL(PRNK_COL_GREEN);
	printk(KERN_DEBUG "av8100_probe\n");
	PRNK_COL(PRNK_COL_WHITE);

	av8100_init_config_params();

#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT
#if defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_HDMI ||\
	defined CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_SDTV
	mcde_mode = av8100_get_mcde_video_mode(hdmi_video_output_cmd.video_output_cea_vesa);
	PRNK_COL(PRNK_COL_GREEN);
	printk(KERN_DEBUG "av8100_probe: mcde_hdmi_display_init_command_mode\n");
	PRNK_COL(PRNK_COL_WHITE);
	mcde_hdmi_display_init_command_mode(mcde_mode);
#endif
#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

	/** Register av8100 driver */
	ret = misc_register(&av8100_miscdev);
	if (ret) {
		printk("registering av8100 driver as misc device fails\n");
		goto av8100_misc_fail;
	}
	if (!i2c_check_functionality(i2cClient->adapter, I2C_FUNC_SMBUS_BYTE_DATA |
			I2C_FUNC_SMBUS_READ_WORD_DATA)) {
			ret = -ENODEV;
			goto err;
	}

	av8100Data = kcalloc(1, sizeof(*av8100Data), GFP_KERNEL);
	if(!av8100Data)	{
		ret = -ENOMEM;
		goto err;
	}
	init_waitqueue_head(&av8100_event);
	//INIT_WORK(&av8100Data->work, u8500_av8100_wq);
	av8100Data->client = i2cClient;
	i2c_set_clientdata(i2cClient,av8100Data);
	/* Test pattern is local to AV8100 driver so if set no MCDE probe as no interaction with it */
	if(mcde_get_hdmi_flag())
	{
		printk("HDMI is included in the menuconfig\n");

		/** powerup the av8100IC */
		ret = av8100_powerup(i2cClient, id);
		if(ret != AV8100_OK){
			printk("error in init the hdmi client\n");
			goto err;
		}

		g_av8100_state = AV8100_OPMODE_INIT;

		ret = av8100_download_firmware(i2cClient, FIRMWARE_DOWNLOAD_ENTRY_REG, av8100_fw_buff, fw_size, I2C_INTERFACE);
		if(ret != AV8100_OK){
			printk("error in av8100 firmware download: failed\n");
			goto err;
		}

		av8100_configure_hdmi(i2cClient);

#ifndef TEST_PATTERN_TEST
#ifdef AV8100_USE_KERNEL_THREAD
		kernel_thread((void*)av8100_thread, NULL, CLONE_FS | CLONE_SIGHAND);
#endif

		ret = request_irq(pdata->irq, av8100_intr_handler,
				IRQF_TRIGGER_RISING, "av8100", av8100Data);
		if (ret) {
			printk(KERN_ERR	"unable to request for the irq %d\n", GPIO_TO_IRQ(192));
			gpio_free(pdata->irq);
			goto err;
		}
		ret = av8100_enable_interrupt(i2cClient);
		if(ret != AV8100_OK){
			printk("error in enabling the interrupts\n");
			goto err;
		}

		av8100_hdmi_on(av8100Data);
#endif /* TEST_PATTERN_TEST */
	}
	return ret;
err:
	kfree(i2cClient);
av8100_misc_fail:
	return ret;
}


/**
 * av8100_remove() - This function will be used to unload the av8100 driver.
 * @i2c:     i2c client device
 *
 **/
static int __exit av8100_remove(struct i2c_client *i2cClient)
{
	i2c_set_clientdata(i2cClient,NULL);
	return 0;
}

static const struct i2c_device_id av8100_id[] = {
	{ "av8100", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, av8100);

static struct i2c_driver av8100_driver = {
	.driver = {
		.name = "av8100",
		.owner = THIS_MODULE,
	},
	.probe 		= av8100_probe,
	.remove 	= __exit_p(av8100_remove),
	.id_table	= av8100_id,
};

static int __init av8100_init(void)
{
	return i2c_add_driver(&av8100_driver);
}

module_init(av8100_init);
static void __exit av8100_exit(void)
{
	i2c_del_driver(&av8100_driver);
}

module_exit(av8100_exit);
