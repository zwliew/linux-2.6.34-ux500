/*---------------------------------------------------------------------------*/
/* © copyright STEricsson,2009. All rights reserved. For   */
/* information, STEricsson reserves the right to license    */
/* this software concurrently under separate license conditions.             */
/*                                                                           */
/* This program is free software; you can redistribute it and/or modify it   */
/* under the terms of the GNU Lesser General Public License as published     */
/* by the Free Software Foundation; either version 2.1 of the License,       */
/* or (at your option)any later version.                                     */
/*                                                                           */
/* This program is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See                  */
/* the GNU Lesser General Public License for more details.                   */
/*                                                                           */
/* You should have received a copy of the GNU Lesser General Public License  */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.      */
/*---------------------------------------------------------------------------*/

#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT
/* HW V1 */

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

/** Linux include files:charachter driver and memory functions  */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/init.h>

#include <asm/dma.h>
#include <asm/uaccess.h>
#include <mach/mcde_common.h>
#include <mach/irqs.h>
#include <mach/ab8500.h>

#include <mach/tc35892.h>
#include <mach/mcde_a0.h>


void  mcde_test(struct fb_info *info);

DECLARE_MUTEX(mcde_module_mutex);

bool mcde_test_enable_lock = true;
bool mcde_test_enable_lock_prints = false;

void mcde_fb_lock(struct fb_info *info, const char *caller)
{
	struct mcdefb_info *currentpar = info->par;

	if (mcde_test_enable_lock_prints)
		printk(KERN_INFO "%s(%p) from %s\n", __func__, info, caller);
	if (!mcde_test_enable_lock)
		return;
	down(&currentpar->fb_sem);

	/* Wait for imsc to be zero */
	/* We don't want to interfere with a refresh interrupt */
#ifdef CONFIG_FB_MCDE_MULTIBUFFER
	if ((currentpar->regbase->mcde_imscpp & currentpar->vcomp_irq) != 0) {
		int wait = 5;

		printk(KERN_INFO "%s(%p), mcde_imscpp != 0, is %X. "
		       "Waiting...\n", __func__, info,
		       currentpar->regbase->mcde_imscpp);
		while (wait-- && currentpar->regbase->mcde_imscpp != 0)
			mdelay(100);
		printk(KERN_INFO "%s(%p), mcde_imscpp = 0, is %X\n",
		       __func__, info,
		       currentpar->regbase->mcde_imscpp);
	}
#endif
}

void mcde_fb_unlock(struct fb_info *info, const char *caller)
{
	struct mcdefb_info *currentpar = info->par;
	if (mcde_test_enable_lock_prints)
		printk(KERN_INFO "%s(%p) from %s\n", __func__, info, caller);
	if (!mcde_test_enable_lock)
		return;
	up(&currentpar->fb_sem);
}

#define DSI_TAAL_DISPLAY

int mcde_debug = MCDE_DEFAULT_LOG_LEVEL;

char isVideoModeChanged = 0;
unsigned int mcde_4500_plugstatus=0;

/** platform specific dectetion */
extern int platform_id;
bool isHdmiEnabled = FALSE;
extern u32 mcde_ovl_bmp;

struct fb_info *g_info;
/** video modes database */
struct fb_videomode mcde_modedb[] = {
        {
                /** 640x350 @ 85Hz ~ VMODE_640_350_85_P*/
                NULL, 85, 640, 350, KHZ2PICOS(31500),
                96, 32, 60, 32, 64, 3,
                FB_SYNC_HOR_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 640x400 @ 85Hz ~ VMODE_640_400_85_P */
                NULL, 85, 640, 400, KHZ2PICOS(31500),
                96, 32, 41, 1, 64, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 720x400 @ 85Hz ~ VMODE_720_400_85_P*/
                NULL, 85, 720, 400, KHZ2PICOS(35500),
                108, 36, 42, 1, 72, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 640x480 @ 60Hz  ~ VMODE_640_480_60_P*/
                "VGA", 60, 640, 480, KHZ2PICOS(25175),
                0x21,0x40,0x07,0x24,0x40,0x19,
                FB_SYNC_HOR_HIGH_ACT|FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 640x480 @ 60Hz  VMODE_640_480_CRT_60_P */
                "CRT", 60, 640, 480, KHZ2PICOS(25175),
                0x29,0x09,0x19,0x02,0x61,0x02,
                FB_SYNC_HOR_HIGH_ACT|FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },{
                /** 240x320 @ 60Hz  ~ VMODE_240_320_60_P*/
                "QVGA_Portrait", 60, 240, 320, KHZ2PICOS(25175),
                0x13,0x2f,0x04,0x0f,0x13,0x04,
                FB_SYNC_HOR_HIGH_ACT|FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },{
                /** 320x240 @ 60Hz  ~ VMODE_320_240_60_P */
                "QVGA_Landscape", 60, 320, 240, KHZ2PICOS(25175),
                0x13,0x2f,0x04,0x0f,0x13,0x04,
                FB_SYNC_HOR_HIGH_ACT|FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },{
                /** 712x568 @ 72Hz  ~ VMODE_712_568_60_P */
                "SDTV", 72, 712, 568, KHZ2PICOS(31500),
                140, 2, 2, 2, 2, 24,
                0, FB_VMODE_INTERLACED
        }, {
                /** 640x480 @ 75Hz  ~ VMODE_640_480_75_P */
                NULL, 75, 640, 480, KHZ2PICOS(31500),
                120, 16, 16, 1, 64, 3,
                0, FB_VMODE_NONINTERLACED
        }, {
                /** 640x480 @ 85Hz ~ VMODE_640_480_85_P */
                NULL, 85, 640, 480, KHZ2PICOS(36000),
                80, 56, 25, 1, 56, 3,
                0, FB_VMODE_NONINTERLACED
        }, {
				/* * 480x864 @ 60Hz  ~ VMODE_480_864_60_P*/
				"WVGA_Portrait", 60, 480, 864, KHZ2PICOS(40000),
				88, 40, 23, 1, 128, 4,
				FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		    /** 864x480 @ 60Hz  ~ VMODE_864_480_60_P*/
                "WVGA", 60, 864, 480, KHZ2PICOS(40000),
                88, 40, 23, 1, 128, 4,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		    /** 800x600 @ 56Hz  ~VMODE_800_600_56_P*/
                "VUIB WVGA", 56, 800, 600, KHZ2PICOS(36000),
                128, 24, 22, 1, 72, 2,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 800x600 @ 60Hz ~ VMODE_800_600_60_P*/
                NULL, 60, 800, 600, KHZ2PICOS(40000),
                88, 40, 23, 1, 128, 4,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		        /** 800x600 @ 72Hz  ~ VMODE_800_600_72_P*/
                NULL, 72, 800, 600, KHZ2PICOS(50000),
                64, 56, 23, 37, 120, 6,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 800x600 @ 75Hz ~ VMODE_800_600_75_P*/
                NULL, 75, 800, 600, KHZ2PICOS(49500),
                160, 16, 21, 1, 80, 3,
		        FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 800x600 @ 85Hz ~ VMODE_800_600_85_P*/
                NULL, 85, 800, 600, KHZ2PICOS(56250),
                152, 32, 27, 1, 64, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		        /** 1024x768 @ 60Hz ~ VMODE_1024_768_60_P */
                NULL, 60, 1024, 768, KHZ2PICOS(65000),
                160, 24, 29, 3, 136, 6,
                0, FB_VMODE_NONINTERLACED
        }, {
                /** 1024x768 @ 70Hz ~ VMODE_1024_768_70_P */
                NULL, 70, 1024, 768, KHZ2PICOS(75000),
                144, 24, 29, 3, 136, 6,
                0, FB_VMODE_NONINTERLACED
        }, {
                /** 1024x768 @ 75Hz ~ VMODE_1024_768_75_P */
                NULL, 75, 1024, 768, KHZ2PICOS(78750),
                176, 16, 28, 1, 96, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1024x768 @ 85Hz ~ VMODE_1024_768_85_P */
                NULL, 85, 1024, 768, KHZ2PICOS(94500),
                208, 48, 36, 1, 96, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1152x864 @ 75Hz ~ VMODE_1152_864_75_P*/
                NULL, 75, 1152, 864, KHZ2PICOS(108000),
                256, 64, 32, 1, 128, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1280x960 @ 60Hz ~ VMODE_1280_960_60_P*/
                NULL, 60, 1280, 960, KHZ2PICOS(108000),
                312, 96, 36, 1, 112, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		    /** 1280x960 @ 85Hz ~ VMODE_1280_960_85_P*/
                NULL, 85, 1280, 960, KHZ2PICOS(148500),
                224, 64, 47, 1, 160, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		        /** 1280x1024 @ 60Hz ~ VMODE_1280_1024_60_P*/
                NULL, 60, 1280, 1024, KHZ2PICOS(108000),
                248, 48, 38, 1, 112, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1280x1024 @ 75Hz ~ VMODE_1280_1024_75_P*/
                NULL, 75, 1280, 1024, KHZ2PICOS(135000),
                248, 16, 38, 1, 144, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1280x1024 @ 85Hz ~ VMODE_1280_1024_8_P*/
                NULL, 85, 1280, 1024, KHZ2PICOS(157500),
                224, 64, 44, 1, 160, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1600x1200 @ 60Hz VMODE_1600_1200_60_P*/
                NULL, 60, 1600, 1200, KHZ2PICOS(162000),
                304, 64, 46, 1, 192, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1600x1200 @ 65Hz VMODE_1600_1200_65_P*/
                NULL, 65, 1600, 1200, KHZ2PICOS(175500),
                304, 64, 46, 1, 192, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		        /** 1600x1200 @ 70Hz VMODE_1600_1200_70_P*/
                NULL, 70, 1600, 1200, KHZ2PICOS(189000),
                304, 64, 46, 1, 192, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1600x1200 @ 75HzVMODE_1600_1200_75_P */
                NULL, 75, 1600, 1200, KHZ2PICOS(202500),
                304, 64, 46, 1, 192, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1600x1200 @ 85Hz VMODE_1600_1200_85_P*/
                NULL, 85, 1600, 1200, KHZ2PICOS(229500),
                304, 64, 46, 1, 192, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1792x1344 @ 60Hz VMODE_1792_1344_60_P */
                NULL, 60, 1792, 1344, KHZ2PICOS(204750),
                328, 128, 46, 1, 200, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1792x1344 @ 75Hz VMODE_1792_1344_75_P */
                NULL, 75, 1792, 1344, KHZ2PICOS(261000),
                352, 96, 69, 1, 216, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		        /** 1856x1392 @ 60Hz VMODE_1856_1392_60_P */
                NULL, 60, 1856, 1392, KHZ2PICOS(218250),
                352, 96, 43, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1856x1392 @ 75Hz VMODE_1856_1392_75_P */
                NULL, 75, 1856, 1392, KHZ2PICOS(288000),
                352, 128, 104, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1920x1440 @ 60Hz VMODE_1920_1440_60_P */
                NULL, 60, 1920, 1440, KHZ2PICOS(234000),
                344, 128, 56, 1, 208, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1920x1440 @ 75Hz VMODE_1920_1440_75_P */
                NULL, 75, 1920, 1440, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },
        {
                /** 720x480 @ 60Hz  VMODE_720_480_60_P */
                NULL, 60, 720, 480, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },
        {
                /** 720x480 @ 60Hz VMODE_720_480_60_I */
                NULL, 60, 720, 480, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_INTERLACED
        },
        {
                /** 720x576 @ 50Hz VMODE_720_576_50_P */
                NULL, 50, 720, 576, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },
        {
                /** 720x576 @ 50Hz VMODE_720_576_50_I */
                NULL, 50, 720, 576, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_INTERLACED
        },
        {
                /** 1280x720 @ 50Hz VMODE_1280_720_50_P */
                NULL, 50, 1280, 720, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },
        {
                /** 1280x720 @ 60Hz VMODE_1280_720_50_I */
                NULL, 60, 1280, 720, KHZ2PICOS(297000),
                0, // left margin
                0, // right margin
                0, // upper margin
                0, // lower margin
                0, // hsync_len
                0,  // vsync_len
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },
        {
                /** 1920x1080 @ 50Hz  VMODE_1920_1080_50_I */
                NULL, 50, 1920, 1080, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_INTERLACED
        },
        {
                /** 1920x1080 @ 60Hz VMODE_1920_1080_60_I */
                NULL, 60, 1920, 1080, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_INTERLACED
        },
        {
                /** 1920x1080 @ 60Hz VMODE_1920_1080_60_P */
                NULL, 60, 1920, 1080, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },
        {
                /** 1920x1080 @ 30Hz VMODE_1920_1080_30_P */
                "HDMI C0", 30, 1920, 1080, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },
};

/**  TODO check these default values for QVGA display */
static struct fb_var_screeninfo mcde_default_var __initdata = {
	/** 240x320, 16bpp @ 60 Hz */
	.xres				= 240,
	.yres				= 320,
	.xres_virtual			= 240,
	.yres_virtual			= 320,
	.bits_per_pixel			= MCDE_U8500_PANEL_16BPP,
	.red				= { 11, 5, 0 },
	.green				= {  5, 6, 0 },
	.blue				= {  0, 5, 0 },
	.activate			= FB_ACTIVATE_NOW,
	.height				= -1,
	.width				= -1,
	.pixclock			= KHZ2PICOS(25175),
	.left_margin			= 0x13,
	.right_margin			= 0x2F,
	.upper_margin			= 0x4,
	.lower_margin			= 0xF,
	.hsync_len			= 0x13,
	.vsync_len			= 0x4,
	.vmode				= FB_VMODE_NONINTERLACED,

};

static struct fb_fix_screeninfo mcde_fix __initdata = {
	.id =		"ST MCDE",
	.type =		FB_TYPE_PACKED_PIXELS,
	.visual =	FB_VISUAL_TRUECOLOR,
	.accel =	FB_ACCEL_NONE,
};
/** global data */
u8 registerInterruptHandler = 0;


/** To be removed I2C */
u8 i2c_init_done=0;

u8 prcmu_init_done = 0;

u32 num_of_display_devices = 0;
u32 PrimaryDisplayFlow;
/** current fb_info */
struct mcdefb_info *gpar[NUM_MCDE_FLOWS];

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MCDE-DSI driver for Nomadik 8500 Platform");

static void mcde_conf_video_mode(struct fb_info *);
static int __init mcde_probe(struct platform_device *);
static int mcde_remove(struct platform_device *);
static int mcde_check_var(struct fb_var_screeninfo *, struct fb_info *);
static int mcde_set_par(struct fb_info *info);
static int mcde_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info *info);
static int mcde_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
static int mcde_ioctl(struct fb_info *info,unsigned int cmd, unsigned long arg);
static int mcde_mmap(struct fb_info *info, struct vm_area_struct *vma);
static int mcde_blank(int blank_mode, struct fb_info *info);
static int mcde_set_video_mode(struct fb_info *info, mcde_video_mode videoMode);
void find_restype_from_video_mode(struct fb_info *info, mcde_video_mode videoMode);
void mcde_change_video_mode(struct fb_info *info);
static void mcde_environment_setup(void);

void u8500_mcde_tasklet_1(unsigned long);
void u8500_mcde_tasklet_2(unsigned long);
void u8500_mcde_tasklet_3(unsigned long);
void u8500_mcde_tasklet_4(unsigned long);

DECLARE_TASKLET(mcde_tasklet_1, u8500_mcde_tasklet_1, 0);
DECLARE_TASKLET(mcde_tasklet_2, u8500_mcde_tasklet_2, 0);
DECLARE_TASKLET(mcde_tasklet_3, u8500_mcde_tasklet_3, 0);

int interrupt_counter=0;
int csm_running_counter=0;

unsigned int av8100_started=0;

void u8500_mcde_tasklet_4(unsigned long tasklet_data)
{
#ifdef CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_HDMI
    av8100_started=1;
#endif

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_HDMI
	if (gpar[MCDE_CH_C0]->regbase->mcde_mispp & 0x4 || gpar[MCDE_CH_C0]->regbase->mcde_rispp & 0x4)
		  /** vcomp */
	   gpar[MCDE_CH_C0]->regbase->mcde_rispp &= 0x4;
	while(gpar[MCDE_CH_C0]->dsi_lnk_registers[DSI_LINK2]->cmd_mode_sts & 0x20);

	gpar[MCDE_CH_C0]->ch_regbase1[MCDE_CH_C0]->mcde_chnl_synchsw=0x1;
#endif
}

EXPORT_SYMBOL(u8500_mcde_tasklet_4);


void u8500_mcde_tasklet_1(unsigned long tasklet_data)
{
	/* Temporary fix for FIFO overflow; avoid handling of the very first interrupt */
	static int ignore_first = 0;
	mcde_ch_id pixel_pipeline = gpar[MCDE_CH_C0]->pixel_pipeline;

#ifndef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_HDMI
	ignore_first = 1;
#endif

	/* Channel C0 interrupt */

#ifndef CONFIG_FB_MCDE_MULTIBUFFER
	while(gpar[MCDE_CH_C0]->dsi_lnk_registers[DSI_LINK0]->cmd_mode_sts & 0x20);

	gpar[MCDE_CH_C0]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_send=0x1;

        /**  wait till tearing recieved */
	while ((gpar[MCDE_CH_C0]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_sts & 0x80)!=0)
	{
		if(gpar[MCDE_CH_C0]->dsi_lnk_registers[DSI_LINK0]->cmd_mode_sts&0x22)
		{
			break;
		}
	}

	/** clear status flag */
	gpar[MCDE_CH_C0]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_sts_clr=0xffffffff;
	if (ignore_first == 1) {
		ignore_first = 0;
	}
	else {
		gpar[MCDE_CH_C0]->ch_regbase1[pixel_pipeline]->mcde_chnl_synchsw = MCDE_CHNLSYNCHSW_SW_TRIG;
	}
#else
#ifndef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_HDMI
	 while(gpar[MCDE_CH_C0]->dsi_lnk_registers[DSI_LINK0]->cmd_mode_sts & 0x20);

	/**  send tearing command */
	gpar[MCDE_CH_C0]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_send=0x1;
	/**  wait till tearing recieved */
	while ((gpar[MCDE_CH_C0]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_sts & 0x80)!=0) {
		if(gpar[MCDE_CH_C0]->dsi_lnk_registers[DSI_LINK0]->cmd_mode_sts&0x22) {
			break;
		}
	}

#ifdef CONFIG_MCDE_HDMI_MAINDISP_SAME_FB
	gpar[MCDE_CH_C0]->extsrc_regbase[0]->mcde_extsrc_a0 = gpar[MCDE_CH_A]->extsrc_regbase[0]->mcde_extsrc_a0;
#else
	gpar[MCDE_CH_C0]->extsrc_regbase[0]->mcde_extsrc_a0 = gpar[MCDE_CH_C0]->clcd_event.base;
#endif

	//Mask the interrupt
	gpar[MCDE_CH_C0]->regbase->mcde_imscpp = 0; //gpar[2]->regbase->mcde_imsc = 0x0;
	gpar[MCDE_CH_C0]->clcd_event.event=1;

	/** send software sync */
	gpar[MCDE_CH_C0]->ch_regbase1[pixel_pipeline]->mcde_chnl_synchsw = MCDE_CHNLSYNCHSW_SW_TRIG; //gpar[2]->ch_regbase1[2]->mcde_chsyn_sw = 0x1;

#else
	/* Switch buffer address (MCDE designer claims better to use buffer IDs than address)  FIXME */
	gpar[MCDE_CH_C0]->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_a0 = gpar[MCDE_CH_C0]->clcd_event.base;
	gpar[MCDE_CH_C0]->regbase->mcde_imscpp = 0x0;
	gpar[MCDE_CH_C0]->clcd_event.event=1;
#endif
	wake_up(&gpar[MCDE_CH_C0]->clcd_event.wait);
#endif
}


void u8500_mcde_tasklet_2(unsigned long tasklet_data)
{

#ifndef CONFIG_FB_MCDE_MULTIBUFFER
	   while(gpar[3]->dsi_lnk_registers[DSI_LINK1]->cmd_mode_sts & 0x20);


	    /**  wait till tearing recieved */


	   gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_send=0x1;

       /**  wait till tearing recieved */

	    while ((gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_sts & 0x80)!=0)
	    {
            if(gpar[3]->dsi_lnk_registers[DSI_LINK1]->cmd_mode_sts&0x22)
            {

                 break;
            }
	    }

       /** clear status flag */
       gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_sts_clr=0xffffffff;
#endif


#ifdef CONFIG_FB_MCDE_MULTIBUFFER
	/** send DSI command for RAMWR */
	gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_main_settings=0x43908;
	gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_wrdat0=0x2c;
	gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_send=0x1;

      while(gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_sts & 0x1);

       gpar[3]->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_a0 = gpar[3]->clcd_event.base;
	   //Mask the interrupt
  gpar[3]->regbase->mcde_imscpp = 0; //gpar[3]->regbase->mcde_imsc = 0x0;
	   gpar[3]->clcd_event.event=1;

	   /** send software sync */
  gpar[3]->ch_regbase1[3]->mcde_chnl_synchsw = MCDE_CHNLSYNCHSW_SW_TRIG; //gpar[3]->ch_regbase1[3]->mcde_chsyn_sw = 0x1;
	   wake_up(&gpar[3]->clcd_event.wait);
#else
  gpar[3]->ch_regbase1[3]->mcde_chnl_synchsw = MCDE_CHNLSYNCHSW_SW_TRIG; //gpar[3]->ch_regbase1[3]->mcde_chsyn_sw = 0x1;
#endif

}



void u8500_mcde_tasklet_3(unsigned long tasklet_data)
{
#ifdef  CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_VUIB_WVGA
#ifdef CONFIG_FB_MCDE_MULTIBUFFER
           gpar[1]->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_a0 = gpar[1]->clcd_event.base;
	   //Mask the interrupt
  gpar[1]->regbase->mcde_imscpp = 0; //gpar[1]->regbase->mcde_imsc = 0x0;
	   gpar[1]->clcd_event.event=1;
	   wake_up(&gpar[1]->clcd_event.wait);
#endif
#else
       /** do nothing */
#endif
}


static irqreturn_t u8500_mcde_interrupt_handler(int irq,void *dev_id)
{

/** for dual display both the channels need to be controlled */
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
	if (gpar[2]->regbase->mcde_mispp & 0xC0) {
	   gpar[2]->regbase->mcde_rispp &= 0xC0;

	   while(gpar[2]->dsi_lnk_registers[DSI_LINK0]->cmd_mode_sts & 0x20);
	   while(gpar[3]->dsi_lnk_registers[DSI_LINK1]->cmd_mode_sts & 0x20);

	   while ((gpar[2]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_sts & 0x80)!=0)
		   {
		               if(gpar[2]->dsi_lnk_registers[DSI_LINK0]->cmd_mode_sts&0x22)
		               {
		                    break;
		               }
	       }

			while ((gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_sts & 0x80)!=0)
			{
				if(gpar[3]->dsi_lnk_registers[DSI_LINK1]->cmd_mode_sts&0x22)
				{
					 break;
				}
			}

			/** clear status flag */
	        gpar[2]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_sts_clr=0xffffffff;
	        /** clear status flag */
            gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_sts_clr=0xffffffff;

            gpar[2]->ch_regbase1[2]->mcde_chnl_synchsw = MCDE_CHNLSYNCHSW_SW_TRIG;
            gpar[3]->ch_regbase1[3]->mcde_chnl_synchsw = MCDE_CHNLSYNCHSW_SW_TRIG;
	}
#endif

#else
		/** check for null */
	if(gpar[MCDE_CH_C0]!=0) {
    /* Channel C0 */
		if (gpar[MCDE_CH_C0]->regbase->mcde_mispp & MCDE_MISPP_VSCC0MIS) //gpar[2]->regbase->mcde_mis & 0x10)
		{
			  /** vsync */
			gpar[MCDE_CH_C0]->regbase->mcde_rispp &= MCDE_RISPP_VSCC0RIS; //gpar[2]->regbase->mcde_ris &= 0x10;
		}

		if (gpar[MCDE_CH_C0]->regbase->mcde_mispp & gpar[MCDE_CH_C0]->vcomp_irq) //gpar[2]->regbase->mcde_mis & 0x40)
		{
			  /** vcomp */
			gpar[MCDE_CH_C0]->regbase->mcde_rispp &= gpar[MCDE_CH_C0]->vcomp_irq; //gpar[2]->regbase->mcde_ris &= 0x40;
			u8500_mcde_tasklet_1(0);
		}

		if (gpar[MCDE_CH_C0]->regbase->mcde_imscchnl & 	MCDE_MISCHNL_CHNL_C0) {//regbase->mcde_mis & 0x1000
			/** read buffer end */
			gpar[MCDE_CH_C0]->regbase->mcde_rischnl &= MCDE_MISCHNL_CHNL_C0;	//regbase->mcde_ris &= 0x1000;

			//tasklet_schedule(&mcde_tasklet_1);
			u8500_mcde_tasklet_1(0);
		}
	}

		/** check for null */
		if(gpar[3]!=0)
		{
    /* Channel C1 */
    if (gpar[3]->regbase->mcde_mispp & MCDE_MISPP_VSCC1MIS) //gpar[3]->regbase->mcde_mis & 0x20)
			{
			  /** vsync */
      gpar[3]->regbase->mcde_rispp &= MCDE_RISPP_VSCC1RIS; //gpar[3]->regbase->mcde_ris &= 0x20;
			}

    if (gpar[3]->regbase->mcde_mispp & MCDE_MISPP_VCMPC1MIS) //gpar[3]->regbase->mcde_mis & 0x80)
			{
			  /** vcomp */
      gpar[3]->regbase->mcde_rispp &= MCDE_RISPP_VCMPC1RIS; //gpar[3]->regbase->mcde_ris &= 0x80;

			   /** tasklet  */
			   //tasklet_schedule(&mcde_tasklet_2);
			   u8500_mcde_tasklet_2(0);
		}
		}
#endif
	/** check for null */
	if(gpar[1]!=0) {
		/* Channel B */
		if (gpar[1]->regbase->mcde_mispp & MCDE_MISPP_VCMPBMIS) //gpar[1]->regbase->mcde_mis & 0x8)
		{
			/** vcomp */
			gpar[1]->regbase->mcde_rispp &= MCDE_RISPP_VCMPBRIS; //gpar[1]->regbase->mcde_ris &= 0x8;

			/** tasklet  */
			//tasklet_schedule(&mcde_tasklet_3);
			u8500_mcde_tasklet_3(0);
		}
	}

	interrupt_counter++;

	return IRQ_HANDLED;
}


static int mcde_ioctl(struct fb_info *info,
		unsigned int cmd, unsigned long arg)
{
	struct mcdefb_info *currentpar = info->par;
	struct mcde_overlay_create extsrc_ovl;
	struct mcde_sourcebuffer_alloc source_buff;
	struct mcde_ext_conf ext_src_config;
	struct mcde_conf_overlay overlay_config;
	struct mcde_ch_conf ch_config;
	struct mcde_chnl_lcd_ctrl chnl_lcd_ctrl;
	struct mcde_channel_color_key chnannel_color_key;
	struct mcde_conf_color_conv color_conv_ctrl;
	struct mcde_blend_control blend_ctrl;
	struct mcde_source_buffer src_buffer;
	struct mcde_dithering_ctrl_conf dithering_ctrl_conf;
	char rot_dir = 0;
	mcde_roten rot_ctrl;
	u32 videoMode;
	u32 rem_key;
	u32 create_key;
	u32 flags;
	u32 scanMode = 0;
	void __user *argp = (void __user *) arg;

	mcde_fb_lock(info, __func__);
	memset (&blend_ctrl, 0, sizeof(struct mcde_blend_control));
	switch (cmd)
	{
		/*
		 *   MCDE_IOCTL_OVERLAY_CREATE: This ioctl is used eigther to create a new overlay or configure the existing overlay.
		 */
		case MCDE_IOCTL_OVERLAY_CREATE:

			if (copy_from_user(&extsrc_ovl, argp, sizeof(struct mcde_overlay_create)))
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to copy data from user space\n");
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_extsrc_ovl_create(&extsrc_ovl,info,&create_key) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to create a new overlay surface\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			extsrc_ovl.key = ((create_key) << PAGE_SHIFT);

			if (copy_to_user(argp, &extsrc_ovl, sizeof(struct mcde_overlay_create))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			break;

		/*
		 *   MCDE_IOCTL_OVERLAY_REMOVE: This ioctl is used to remove the overlay(not base overlay).
		 */
		case MCDE_IOCTL_OVERLAY_REMOVE:

			if (copy_from_user(&rem_key, argp, sizeof(u32))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			rem_key = rem_key >> PAGE_SHIFT;
			dbgprintk(MCDE_DEBUG_INFO, "remove overlay id  %d\n",rem_key);

			/** dont touch base overlay 0 */
			if(rem_key < num_of_display_devices)
			{
				dbgprintk(MCDE_ERROR_INFO, "Cant remove base overlay 0\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			if (mcde_extsrc_ovl_remove(info,rem_key) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "cant remove overlay id  %d\n",rem_key);
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			break;
		/*
		 *   MCDE_IOCTL_ALLOC_FRAMEBUFFER: This ioctl is used to allocate the memory for frame buffer.
		 */
		case MCDE_IOCTL_ALLOC_FRAMEBUFFER:
			if (copy_from_user(&source_buff, argp, sizeof(struct mcde_sourcebuffer_alloc))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			flags = claim_mcde_lock(currentpar->chid);
			if (mcde_alloc_source_buffer(source_buff,info,&create_key, TRUE) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to allocate framebuffer\n");
				release_mcde_lock(currentpar->chid, flags);
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			source_buff.buff_addr.dmaaddr = currentpar->buffaddr[create_key].dmaaddr;
			source_buff.buff_addr.bufflength = currentpar->buffaddr[create_key].bufflength;
			source_buff.key = ((create_key) << PAGE_SHIFT);
			release_mcde_lock(currentpar->chid, flags);

			if (copy_to_user(argp, &source_buff, sizeof(struct mcde_sourcebuffer_alloc))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			break;
		/*
		 *   MCDE_IOCTL_DEALLOC_FRAMEBUFFER: This ioctl is used to de -allocate the memory used for frame buffer.
		 */
		case MCDE_IOCTL_DEALLOC_FRAMEBUFFER:
			if (copy_from_user(&rem_key, argp, sizeof(u32))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			rem_key = rem_key >> PAGE_SHIFT;
			dbgprintk(MCDE_ERROR_INFO, "remove source buffer id  %d\n",rem_key);

			if((rem_key <= MCDE_MAX_FRAMEBUFF) &&  (rem_key > 2*MCDE_MAX_FRAMEBUFF))
			{
				dbgprintk(MCDE_ERROR_INFO, "Cant remove the source buffer\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			//flags = claim_mcde_lock(currentpar->chid);
			if (mcde_dealloc_source_buffer(info, rem_key, TRUE) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to deallocate framebuffer\n");
			//	release_mcde_lock(currentpar->chid, flags);
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			//release_mcde_lock(currentpar->chid, flags);
			break;
		/*
		 *   MCDE_IOCTL_CONFIGURE_EXTSRC: This ioctl is used to configure the external source configuration.
		 */
		case MCDE_IOCTL_CONFIGURE_EXTSRC:
			if (copy_from_user(&ext_src_config, argp, sizeof(struct mcde_ext_conf))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_extsource(ext_src_config,info) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to configure the extsrc reg\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			break;
		/*
		 *   MCDE_IOCTL_CONFIGURE_OVRLAY: This ioctl is used to configure the overlay configuration.
		 */
		case MCDE_IOCTL_CONFIGURE_OVRLAY:
			if (copy_from_user(&overlay_config, argp, sizeof(struct mcde_conf_overlay))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_overlay(overlay_config,info) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to configure overlay reg\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			break;
		/*
		 *   MCDE_IOCTL_CONFIGURE_CHANNEL: This ioctl is used to configure the channel configuration.
		 */
		case MCDE_IOCTL_CONFIGURE_CHANNEL:
			if (copy_from_user(&ch_config, argp, sizeof(struct mcde_ch_conf))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_channel(ch_config,info) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to configure the channel reg\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			break;
		/*
		 *   MCDE_IOCTL_CONFIGURE_PANEL: This ioctl is used to configure the panel(output interface) configuration.
		 */
		case MCDE_IOCTL_CONFIGURE_PANEL:
			if (copy_from_user(&chnl_lcd_ctrl, argp, sizeof(struct mcde_chnl_lcd_ctrl))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_lcd_timing(chnl_lcd_ctrl,info) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to configure the panel timings\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_MCDE_ENABLE: This ioctl is used to enable the flow(A/B/C0/C1) and power of the MCDE.
		 */
		case MCDE_IOCTL_MCDE_ENABLE:
			if (mcde_enable(info) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to enable MCDE\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_MCDE_DISABLE: This ioctl is used to disable the flow(A/B/C0/C1) and power of the MCDE.
		 */
		case MCDE_IOCTL_MCDE_DISABLE:
			if (mcde_disable(info) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to disable MCDE\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_COLOR_KEYING_ENABLE: This ioctl is used to enable color keying.
		 */
		case MCDE_IOCTL_COLOR_KEYING_ENABLE:
			if (copy_from_user(&chnannel_color_key, argp, sizeof(struct mcde_channel_color_key))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_channel_color_key(info, chnannel_color_key) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to enable channel color keying\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_COLOR_KEYING_DISABLE: This ioctl is used to disable color keying.
		 */
		case MCDE_IOCTL_COLOR_KEYING_DISABLE:
			chnannel_color_key.key_ctrl = MCDE_CLR_KEY_DISABLE;
			if (mcde_conf_channel_color_key(info, chnannel_color_key) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to disable channel color keying\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_COLOR_COVERSION_ENABLE: This ioctl is used to enable color color conversion.
		 */
		case MCDE_IOCTL_COLOR_COVERSION_ENABLE:
			if (copy_from_user(&color_conv_ctrl, argp, sizeof(struct mcde_conf_color_conv))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_color_conversion(info, color_conv_ctrl) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to enable channel color keying\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_COLOR_COVERSION_DISABLE: This ioctl is used to disable color color conversion.
		 */
		case MCDE_IOCTL_COLOR_COVERSION_DISABLE:
			color_conv_ctrl.col_ctrl = MCDE_COL_CONV_DISABLE;
			if (mcde_conf_color_conversion(info, color_conv_ctrl) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to enable channel color keying\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_CHANNEL_BLEND_ENABLE: This ioctl is used to enable alpha blending.
		 */
		case MCDE_IOCTL_CHANNEL_BLEND_ENABLE:
			if (copy_from_user(&blend_ctrl, argp, sizeof(struct mcde_blend_control))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_blend_ctrl(info, blend_ctrl) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to enable channel color keying\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_CHANNEL_BLEND_DISABLE: This ioctl is used to disable blending.
		 */
		case MCDE_IOCTL_CHANNEL_BLEND_DISABLE:
			blend_ctrl.blenden = MCDE_BLEND_DISABLE;
			blend_ctrl.ovr2_enable = 0;
			blend_ctrl.ovr2_id=0;
			blend_ctrl.ovr1_id=0;
			if (mcde_conf_blend_ctrl(info, blend_ctrl) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to enable channel color keying\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_ROTATION_ENABLE: This ioctl is used to enable rotation.
		 */
		case MCDE_IOCTL_ROTATION_ENABLE:
			if (copy_from_user(&rot_dir, argp, sizeof(char))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			rot_ctrl = MCDE_ROTATION_ENABLE;
			mcde_conf_rotation(info, rot_dir, rot_ctrl, currentpar->rotationbuffaddr0.dmaaddr, currentpar->rotationbuffaddr1.dmaaddr);
			break;
		/*
		 *   MCDE_IOCTL_ROTATION_DISABLE: This ioctl is used to disable rotation.
		 */
		case MCDE_IOCTL_ROTATION_DISABLE:
			rot_ctrl = MCDE_ROTATION_DISABLE;
			mcde_conf_rotation(info, rot_dir, rot_ctrl, 0x0, 0x0);
			break;
		/*
		 *   MCDE_IOCTL_SET_VIDEOMODE: This ioctl is used to configure the video mode(for different refresh rates and resolutions).
		 */
		case MCDE_IOCTL_SET_VIDEOMODE:
			if (copy_from_user(&videoMode, argp, sizeof(u32)))
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to set video mode\n");
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			/** SPINLOCK in use :  */
			//flags = claim_mcde_lock(currentpar->chid);
			isVideoModeChanged = 1;
			if (mcde_dealloc_source_buffer(info, currentpar->mcde_cur_ovl_bmp, FALSE) != MCDE_OK)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to set video mode\n");
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
		        currentpar->isHwInitalized = 0;
			find_restype_from_video_mode(info, videoMode);
			mcde_set_video_mode(info, videoMode);
			mcde_change_video_mode(info);
			//release_mcde_lock(currentpar->chid, flags);
			break;
		/*
		 *   MCDE_IOCTL_CONFIGURE_DENC: This ioctl is used to configure denc.
		 */
		case MCDE_IOCTL_CONFIGURE_DENC:

			break;
		/*
		 *   MCDE_IOCTL_CONFIGURE_HDMI: This ioctl is used to configure HDMI.
		 */
		case MCDE_IOCTL_CONFIGURE_HDMI:

			break;
		/*
		 *   MCDE_IOCTL_SET_SOURCE_BUFFER: This ioctl is used to set the source buffer(frame buffer).
		 */
		case MCDE_IOCTL_SET_SOURCE_BUFFER:
			if (copy_from_user(&src_buffer, argp, sizeof(struct mcde_source_buffer)))
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to copy the data from user space in MCDE_IOCTL_SET_SOURCE_BUFFER\n");
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			mcde_set_buffer(info, src_buffer.buffaddr.dmaaddr, src_buffer.buffid);
			break;
		/*
		 *   MCDE_IOCTL_DITHERING_ENABLE: This ioctl is used to enable dithering.
		 */
		case MCDE_IOCTL_DITHERING_ENABLE:
			if (copy_from_user(&dithering_ctrl_conf, argp, sizeof(struct mcde_dithering_ctrl_conf)))
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to copy the data from user space in MCDE_IOCTL_DITHERING_ENABLE\n");
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			mcde_conf_dithering_ctrl(dithering_ctrl_conf, info);
			break;
		/*
		 *   MCDE_IOCTL_SET_SCAN_MODE: This ioctl is used to sconfigure scan mode.
		 */
		case MCDE_IOCTL_SET_SCAN_MODE:
			if (copy_from_user(&scanMode, argp, sizeof(u32)))
			{
				dbgprintk(MCDE_IOCTL_SET_SCAN_MODE, "Failed to copy the data from user space in MCDE_IOCTL_SET_SCAN_MODE\n");
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			mcde_conf_scan_mode(scanMode, info);
			break;
		/*
		 *   MCDE_IOCTL_GET_SCAN_MODE: This ioctl is used to get the scan mode.
		 */
		case MCDE_IOCTL_GET_SCAN_MODE:
			if (copy_to_user(argp, &info->var.vmode, sizeof(u32))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			break;
		/*
		 *   MCDE_IOCTL_TEST_DSI_HSMODE: This ioctl is used to test the dsi direct command high speed mode.
		 */
		case MCDE_IOCTL_TEST_DSI_HSMODE:
			if (copy_from_user(&rem_key, argp, sizeof(u32))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_dsi_test_dsi_HS_directcommand_mode(info,rem_key) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "HS direct command test case failed\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_TEST_DSI_LPMODE: This ioctl is used to test the dsi direct command low power mode.
		 */
		case MCDE_IOCTL_TEST_DSI_LPMODE:
			if (copy_from_user(&rem_key, argp, sizeof(u32))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_dsi_test_LP_directcommand_mode(info,rem_key) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Low Power direct command test case failed\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			break;
        /*
		 *   MCDE_IOCTL_TV_PLUG_STATUS: This ioctl is used to get the TV plugin status.
		 */
		case MCDE_IOCTL_TV_PLUG_STATUS:
#ifndef CONFIG_U8500_SIM8500
		    rem_key=ab8500_read(0xE,0xE00);
#endif
		    if(rem_key&0x4) rem_key=1;
		    else rem_key=0;

			if (copy_to_user(argp, &rem_key, sizeof(u32))) {
				mcde_fb_unlock(info, __func__);
				  return -EFAULT;
			}
			break;

	    /*
	     *  MCDE_IOCTL_TV_CHANGE_MODE: This ioctl is used to change TV mode
	     */
		case MCDE_IOCTL_TV_CHANGE_MODE:
			if (copy_from_user(&rem_key, argp, sizeof(u32))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}



#ifndef CONFIG_U8500_SIM8500

        if(rem_key==VMODE_720_480_60_P)
        {
             ab8500_write(0x6,0x600,0x8A);
             scanMode=MCDE_SCAN_PROGRESSIVE_MODE;
	}
        else if(rem_key==VMODE_720_480_60_I)
        {
	         ab8500_write(0x6,0x600,0x8A);
             scanMode=MCDE_SCAN_INTERLACED_MODE;
	    }

        else if(rem_key==VMODE_720_576_50_P)
        {
	         ab8500_write(0x6,0x600,0xA);
             scanMode=MCDE_SCAN_PROGRESSIVE_MODE;
	    }

        else if(rem_key==VMODE_720_576_50_I)
        {
	         ab8500_write(0x6,0x600,0xA);
             scanMode=MCDE_SCAN_INTERLACED_MODE;
	    }


         mcde_conf_scan_mode(scanMode,info);
#endif

            break;

	    /*
	     *  MCDE_IOCTL_TV_GET_MODE: This ioctl is used to get TV mode
	     */
		case MCDE_IOCTL_TV_GET_MODE:

            /** get the PAL and NTSC mode */
#ifndef CONFIG_U8500_SIM8500
            rem_key=ab8500_read(0x6,0x600);
#endif

            if(rem_key&0x40) rem_key=1;
            else rem_key=0;

            if (copy_to_user(argp, &rem_key, sizeof(u32))) {
              mcde_fb_unlock(info, __func__);
				      return -EFAULT;
            }
			break;

		default:
            mcde_fb_unlock(info, __func__);
			return -EINVAL;

	}

	mcde_fb_unlock(info, __func__);
	return 0;
}


void tv_detect_handler(void)
{
        printk(KERN_INFO "TV plugin detected\n");
        mcde_4500_plugstatus=1;
        return;
}


void tv_removed_handler(void)
{
        printk(KERN_INFO "TV plugin removed \n");
        mcde_4500_plugstatus=0;
        return;
}

void mcde_change_video_mode(struct fb_info *info)
{
	struct mcdefb_info *currentpar = (struct mcdefb_info *)info->par;

	if(currentpar->chid==MCDE_CH_B)
	{
       currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;
       currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf = (info->var.yres << MCDE_OVLCONF_LPF_SHIFT) |
                                                                (MCDE_EXT_SRC_2 << MCDE_OVLCONF_EXTSRC_ID_SHIFT) |
                                                                (info->var.xres);
       currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_ljinc=(info->var.xres)*(currentpar->chnl_info->inbpp/8);
    }

	if(currentpar->chid==MCDE_CH_C0)
	{
       currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;
       currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf = (info->var.yres << MCDE_OVLCONF_LPF_SHIFT) |
                                                                (MCDE_EXT_SRC_0 << MCDE_OVLCONF_EXTSRC_ID_SHIFT) |
                                                                (info->var.xres);
       currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_ljinc=(info->var.xres)*(currentpar->chnl_info->inbpp/8);
    }

	if(currentpar->chid==MCDE_CH_C1)
	{

       currentpar->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;
       currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf = (info->var.yres << MCDE_OVLCONF_LPF_SHIFT) |
                                                                (MCDE_EXT_SRC_1 << MCDE_OVLCONF_EXTSRC_ID_SHIFT) |
                                                                (info->var.xres);
       currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_ljinc=(info->var.xres)*(currentpar->chnl_info->inbpp/8);
    }
}


/**
 * mcde_hdmi_display_init_command_mode() - To initialize and powerup the HDMI device in command mode.
 *
 * This function initializez and power up HDMI/CVBS device.
 *
 */
void mcde_hdmi_display_init_command_mode()
{
#define GPIO1B_AFSLA_REG_OFFSET	0x20
#define GPIO1B_AFSLB_REG_OFFSET	0x24

	int timeout;
	volatile u32 __iomem *gpio_68_remap;

	struct mcdefb_info *currentpar = gpar[MCDE_CH_A];

	dbgprintk(MCDE_ERROR_INFO, "\n>> HDMI Dispaly Initialisation cmd mode \n\n\n");

	/* TODO */
	/* HW trig */
	//gpio_68_remap=(u32 *) ioremap(0x8000e024, (0x8000e024+3)-0x8000e024 + 1);
	gpio_68_remap=(u32 *) ioremap(U8500_GPIO1_BASE + GPIO1B_AFSLB_REG_OFFSET,
								(U8500_GPIO1_BASE + GPIO1B_AFSLB_REG_OFFSET + 3) - (U8500_GPIO1_BASE + GPIO1B_AFSLB_REG_OFFSET) + 1);

	*gpio_68_remap &=0xffffffef;
	iounmap(gpio_68_remap);

	//gpio_68_remap=(u32 *) ioremap(0x8000e020, (0x8000e020+3)-0x8000e020 + 1);
	gpio_68_remap=(u32 *) ioremap(U8500_GPIO1_BASE + GPIO1B_AFSLA_REG_OFFSET,
								(U8500_GPIO1_BASE + GPIO1B_AFSLA_REG_OFFSET + 3) - (U8500_GPIO1_BASE + GPIO1B_AFSLA_REG_OFFSET) + 1);
	*gpio_68_remap |=0x10;
	iounmap(gpio_68_remap);

	if((currentpar->chid==MCDE_CH_A) && (!(currentpar->regbase->mcde_cr & MCDE_CR_DSICMD2_EN))) //DSI2 in CMD mode
	{
		*(currentpar->prcm_mcde_clk) = 0x125; /** MCDE CLK = 160MHZ */
		*(currentpar->prcm_hdmi_clk) = 0x145; /** HDMI CLK = 76.8MHZ */
		*(currentpar->prcm_tv_clk) = 0x14E; /** HDMI CLK = 76.8MHZ */

		/* IF1_MODE */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_data_ctl=0x0;

		/* DAT2_ULPM_ENABLE
		 * DAT1_ULPM_ENABLE
		 * LANE2_EN
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_phy_ctl=0x31;

		/* IF1_EN
		 * DAT2_EN
		 * DAT1_EN
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_en=0x230;

		/* ARB_MODE */
		currentpar->dsi_lnk_registers[DSI_LINK2]->cmd_mode_ctl = 0x01FF0000;//0x40;

		/* IF1_MODE
		 * LINK_EN
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_data_ctl=0x1;

		/* IF1_EN
		 * IF2_EN
		 * DAT2_EN
		 * DAT1_EN
		 * CKLANE_EN
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_en=0x638;
		timeout=0xFFFF;
		while(!((currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_sts) & 0x2) && timeout>0)
		{
			timeout--;
		}
		if(timeout == 0)
			printk(KERN_INFO "DSI CLK LANE NOT READY\n");
		else
			printk(KERN_INFO "DSI CLK LANE READY...\n");

		/* PLL_MASTER
		 * PLL_DIV = 5
		 * PLL_MULT = 0x20
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_pll_ctl = 0x00520;

		/* PLL_START */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_en |= 0x1;

		timeout=0xFFFF;
		while(!((currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_sts) & 0x1) && timeout>0)
		{
			timeout--;
		}
		if(timeout == 0)
			printk(KERN_INFO "PLL NOT STARTED\n");
		else
			printk(KERN_INFO "DSI PLL STARTED AND LOCKED...\n");

		/* UI_X4 = 6 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_dphy_static =  0x180;

		/* LPRX_TO_VAL = 0x3FFF
		 * HSTX_TO_VAL = 0x3FFF
		 * CLK_DIV     = 0xB
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_dphy_timeout = 0xFFFFFFFB;

		/* DATA_ULPOUT_TIME   = 0xFF
		 * CKLANE_ULPOUT_TIME = 0x0
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_ulpout_time= 0x1FE00;

		/* LANE2_EN */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_phy_ctl=0x1;

		if(currentpar->video_mode == VMODE_640_480_CRT_60_P)
		{
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vsize = 0x1E00A842;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize1 = 0x920C422;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize2 = 0x500;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_dphy_time = 0x2986;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize1 = 0x0;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize2 = 0x0;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_pck_time = 0x650;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting1 = 0x10000;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting2 = 0x0;
		}

		if(currentpar->video_mode == VMODE_1280_720_60_P)
		{
			/* VACT_LENGTH = 0x2D0
			* VFP_LENGTH  = 0x5
			* VBP_LENGTH  = 0x14
			* VSA_LENGTH  = 0x5
			*/
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vsize = 0x2D005505;

			/* HFP_LENGTH = 0xE4
			* HBP_LENGTH = 0x72
			* HSA_LENGTH = 0x27
			*/
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize1 = 0xE41C827;

			/* RGB_SIZE = 0xA00 */
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize2 = 0xA00;

			/* REG_WAKEUP_TIME   = 0x1
			* REG_LINE_DURATION = 0x6AA
			*/
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_dphy_time = 0x26AA;

			/* BLKEOL_PCK        = 0x0
			* BLKLINE_EVENT_PCK = 0xD7
			*/
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize1 = 0xD7;

			/* BLKLINE_PULSE_PCK = 0x0 */
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize2 = 0x0;

			/* BLKEOL_DURATION = 0xA4 */
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_pck_time = 0xA4;

			/* BURST_LP        = 0x1
			* MAX_BURST_LIMIT = 0
			*/
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting1 = 0x10000;

			/* MAX_LINE_LIMIT    = 0
			* exact-burst-limit = 0
			*/
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting2 = 0x0;
		}

		if(currentpar->video_mode == VMODE_720_480_60_P)
		{
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vsize = 0x1E009786;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize1 = 0x8008882;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize2 = 0x5A0;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_dphy_time = 0x2724;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize1 = 0x0;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize2 = 0x0;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_pck_time = 0x3C0;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting1 = 0x10000;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting2 = 0x0;
		}

		if (currentpar->video_mode == VMODE_1920_1080_30_P)
		{
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vsize = 0x43804905;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize1 = 0x09916C2C;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize2 = 0xF00;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_dphy_time = 0x28E3;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize1 = 0x2;//0xD7;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize2 = 0x0;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_pck_time = 0xAB;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting1 = 0x10000;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting2 = 0x0;
		}

		/****************** MCDE CONFIGURATION ***************************/


		/** disable MCDE first then configure */
		currentpar->regbase->mcde_cr=(currentpar->regbase->mcde_cr & ~MCDE_ENABLE);

		currentpar->extsrc_regbase[MCDE_EXT_SRC_3]->mcde_extsrc_a0 = currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;

		/** configure mcde external registers */
		if (currentpar->chnl_info->inbpp==16)
			currentpar->extsrc_regbase[MCDE_EXT_SRC_3]->mcde_extsrc_conf=(MCDE_RGB565_16_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;
		if (currentpar->chnl_info->inbpp==32)
			currentpar->extsrc_regbase[MCDE_EXT_SRC_3]->mcde_extsrc_conf=(MCDE_ARGB_32_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;
		if (currentpar->chnl_info->inbpp==24)
			currentpar->extsrc_regbase[MCDE_EXT_SRC_3]->mcde_extsrc_conf=(MCDE_RGB_PACKED_24_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;

		currentpar->extsrc_regbase[MCDE_EXT_SRC_3]->mcde_extsrc_cr=MCDE_BUFFER_SOFTWARE_SELECT;

		/** configure mcde base registers */
		/* IFIFOCTRLWTRMRKLVL = 5
		 * SYNCMUX0 = 1, Channel C chip select 0
		 */
		currentpar->regbase->mcde_conf0 = 0x00005000;//0x5E145001;

		if(currentpar->video_mode == VMODE_1280_720_60_P)
		{
			printk(KERN_INFO "VMODE_1280_720_60_P\n");

			/** configure mcde overlay registers */
			currentpar->ovl_regbase[MCDE_OVERLAY_3]->mcde_ovl_cr=0x23CF0001;
			currentpar->ovl_regbase[MCDE_OVERLAY_3]->mcde_ovl_conf = (g_info->var.yres << MCDE_OVLCONF_LPF_SHIFT) |
		                                                                (MCDE_EXT_SRC_3 << MCDE_OVLCONF_EXTSRC_ID_SHIFT) |
		                                                                (g_info->var.xres);
			currentpar->ovl_regbase[MCDE_OVERLAY_3]->mcde_ovl_conf2=0x6000000;
			currentpar->ovl_regbase[MCDE_OVERLAY_3]->mcde_ovl_ljinc=(g_info->var.xres)*(currentpar->chnl_info->inbpp/8);
			currentpar->ovl_regbase[MCDE_OVERLAY_3]->mcde_ovl_comp = (MCDE_CH_A<<MCDE_OVLCOMP_CH_ID_SHIFT);

			/** configure mcde channel config registers */
			/* LPF = 0x2CF
			* PPL = 0x4FF
			*/
			currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_conf = 0x2CF04FF;

			/* OUT_SYNCH_SRC = LCD/TV 0
			* SRC_SYNCH     = output interfaces
			*/
			/* HW trigger for MCDE*/
			currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_synchmod = 4;

			/* background color R, G, B */
			currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_bckgndcol = 0x0;

			/* Channel prio */
			currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_prio = 0x0;

			/** configure channel A register */

			/* SYNCCTRL = 1, synchro of chA
			* C1EN
			* POWEREN
			* FLOEN
			*/
			currentpar->ch_regbase2[MCDE_CH_A]->mcde_cr0 = 0x3;

			/** configure mcde DSI formatter */
			/* DCSVID_NOTGEN 1, t as video or dcs CMD
			* 8b command
			*  video mode
			*/
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_conf0 = 0x42000;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_frame = 0x1c22d0;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_pkt = 0xA01;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_sync=0x0;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_cmdw = 0x002C003C;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_delay0=0xde3;

			/** Patch for HW TRIG MCDE **/
			/* Need to set some channelC registers */
			{
			volatile u32 __iomem *addr_tmp;

			/* mcde_crc */
			addr_tmp = (u32 *) ioremap(0xA0350C00, (0xA0350C00+3)-0xA0350C00 + 1);
			*addr_tmp |= 0x180;
			iounmap(addr_tmp);

			/* mcde_vscrc */
			addr_tmp = (u32 *) ioremap(0xA0350C5C, (0xA0350C5C+3)-0xA0350C5C + 1);
			*addr_tmp = 0xFF001;
			iounmap(addr_tmp);

			/* mcde_ctrlc */
			addr_tmp = (u32 *) ioremap(0xA0350CA8, (0xA0350CA8+3)-0xA0350CA8 + 1);
			*addr_tmp = 0xA0;
			iounmap(addr_tmp);
			}
		}

		if(currentpar->video_mode == VMODE_640_480_CRT_60_P)
		{
			printk(KERN_INFO "VMODE_640_480_CRT_60_P\n");

			/** configure mcde overlay registers */
            currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_cr=0x24b0061;
            currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf=0x1E00280;
            currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf2=0x5000000;//0xA000000;
            currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_ljinc=0x500;
            currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_comp=0x0000;//ChA

            /** configure mcde channel config registers */
            currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_conf = 0x1DF027F;
            currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_synchmod = MCDE_SYNCHRO_SOFTWARE;
            currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_bckgndcol=0xFF00; // Background color
            currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_prio=0x0;

			currentpar->ch_regbase2[MCDE_CH_A]->mcde_cr0 = 0x3;

            /** configure mcde DSI formatter */
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_conf0 = 0x42000;//0x43000
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_frame = 0x3F4C38;//1C2000;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_pkt = 0xF01;//0xA00;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_sync=0x0;//0x3;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_cmdw = 0x002C003C;//0x0;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_delay0=0x1285;
		}

		if(currentpar->video_mode == VMODE_720_480_60_P)
		{
			printk(KERN_INFO "VMODE_720_480_60_P\n");

			/** configure mcde overlay registers */
            currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_cr=0x24B00061;
            currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf=0x1E002D0;
            currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf2=0x5A00000;//0xA000000;
            currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_ljinc=0x5A0;
            currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_comp=0x0000;//ChA

            /** configure mcde channel config registers */
            currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_conf = 0x1DF02CF;
            currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_synchmod = MCDE_SYNCHRO_SOFTWARE;
            currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_bckgndcol=0xFF; // Background color
            currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_prio=0x0;

			currentpar->ch_regbase2[MCDE_CH_A]->mcde_cr0 = 0x3;

            /** configure mcde DSI formatter */
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_conf0=0x43000;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_frame=0xA8C00;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_pkt=0x5A0;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_sync=0x0;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_cmdw = 0x002C003C;
		}

		if(currentpar->video_mode == VMODE_1920_1080_30_P)
		{
			printk(KERN_INFO "VMODE_1920_1080_30_P\n");
			currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_cr=0x23CF0001;
			currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf=0x04380780;
			currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf2=0xA000000;
			currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_ljinc=0xF00;
			currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_comp=0x0000;//ChA
			currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_conf = 0x0437077f;
			currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_synchmod = MCDE_SYNCHRO_SOFTWARE;
			currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_bckgndcol = 0xFF;
			currentpar->ch_regbase1[MCDE_CH_A]->mcde_chnl_prio = 0x0;
			currentpar->ch_regbase2[MCDE_CH_A]->mcde_cr0 = 0x3;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_conf0 = 0x42000;//0x43000
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_frame = 0x1FA838;//0x3F4C38;//1C2000;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_pkt = 0x781;//0xF01;//0xA00;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_sync=0x0;//0x3;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_cmdw = 0x002C003C;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_delay0=0x942;//0x1285
		}

		/******************  END OF MCDE CONFIGURATION ***************************/

		/** VSG CONFIGURATION **/
		/*
		 * REG_BLKEOL_MODE  = 0x3
		 * REG_BLKLINE_MODE = 0x3
		 * BURST_MODE       = 1
		 * HEADER           = 0xE
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_main_ctl = 0x00164380;//0x1E4380;

		/* Fill color */
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_err_color = 0xFF;//0xFFFFFF;

		/* MCDEEN
		 * IFIFOCTRLEN
		 * DSIVID2_EN
		 */
		currentpar->regbase->mcde_cr |= 0x00008001;
		currentpar->regbase->mcde_cr |= MCDE_ENABLE;
	}

	return;
}
EXPORT_SYMBOL(mcde_hdmi_display_init_command_mode);


/**
 * mcde_hdmi_display_init_command_mode() - To initialize and powerup the HDMI device in command mode.
 *
 * This function initializez and power up HDMI/CVBS device.
 *
 */
void mcde_hdmi_display_init_video_mode()
{
	int timeout;
	struct mcdefb_info *currentpar = gpar[MCDE_CH_C0];

	dbgprintk(MCDE_ERROR_INFO, "\n>> HDMI Display Initialisation video mode\n\n\n");

	if((currentpar->chid==MCDE_CH_C0) && (!(currentpar->regbase->mcde_cr & MCDE_CR_DSIVID2_EN))) //DSI2 in VIDEO mode
	{
		*(currentpar->prcm_mcde_clk) = 0x125; /** MCDE CLK = 160MHZ */
		*(currentpar->prcm_hdmi_clk) = 0x145; /** HDMI CLK = 76.8MHZ */
		*(currentpar->prcm_tv_clk) = 0x14E; /** HDMI CLK = 76.8MHZ */

		/* IF1_MODE */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_data_ctl=0x2;

		/* DAT2_ULPM_ENABLE
		 * DAT1_ULPM_ENABLE
		 * LANE2_EN
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_phy_ctl=0x31;

		/* IF1_EN
		 * DAT2_EN
		 * DAT1_EN
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_en=0x230;

		/* ARB_MODE */
		currentpar->dsi_lnk_registers[DSI_LINK2]->cmd_mode_ctl = 0x40;

		/* IF1_MODE
		 * LINK_EN
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_data_ctl=0x3;

		/* IF1_EN
		 * DAT2_EN
		 * DAT1_EN
		 * CKLANE_EN
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_en=0x238;
		timeout=0xFFFF;
		while(!((currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_sts) & 0x2) && timeout>0)
		{
			timeout--;
		}
		if(timeout == 0)
			printk(KERN_INFO "DSI CLK LANE NOT READY\n");
		else
			printk(KERN_INFO "DSI CLK LANE READY...\n");

		/* PLL_MASTER
		 * PLL_DIV = 5
		 * PLL_MULT = 0x20
		 */
		if(currentpar->video_mode == VMODE_720_480_60_P)
		{
			/*currentpar->mcde_clkdsi = 0xE10110; //Pll mult = 24; div = 1*/
			mdelay(100);
//			currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_pll_ctl = 0x10498;
			currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_pll_ctl = 0x00520;
		}
		else
		{
			/*currentpar->mcde_clkdsi = 0xE10110;*/
		mdelay(100);
			currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_pll_ctl = 0x00520;
		}

		/* PLL_START */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_en |= 0x1;

		timeout=0xFFFF;
		while(!((currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_sts) & 0x1) && timeout>0)
		{
			timeout--;
		}
		if(timeout == 0)
			printk(KERN_INFO "PLL NOT STARTED\n");
		else
			printk(KERN_INFO "DSI PLL STARTED AND LOCKED...\n");

		/* UI_X4 = 6 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_dphy_static =  0x180;

		/* LPRX_TO_VAL = 0x3FFF
		 * HSTX_TO_VAL = 0x3FFF
		 * CLK_DIV     = 0xB
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_dphy_timeout = 0xFFFFFFFB;

		/* DATA_ULPOUT_TIME   = 0xFF
		 * CKLANE_ULPOUT_TIME = 0x0
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_ulpout_time= 0x1FE00;

		/* LANE2_EN */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_phy_ctl=0x1;

		if(currentpar->video_mode == VMODE_640_480_CRT_60_P)
		{
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vsize = 0x1E00A842;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize1 = 0x920C422;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize2 = 0x500;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_dphy_time = 0x2986;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize1 = 0x0;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize2 = 0x0;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_pck_time = 0x650;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting1 = 0x10000;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting2 = 0x0;
		}

        if(currentpar->video_mode == VMODE_1280_720_60_P)
		{
			/* VACT_LENGTH = 0x2D0
			* VFP_LENGTH  = 0x5
			* VBP_LENGTH  = 0x14
			* VSA_LENGTH  = 0x5
			*/
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vsize = 0x2D005505;

			/* HFP_LENGTH = 0xE4
			* HBP_LENGTH = 0x72
			* HSA_LENGTH = 0x27
			*/
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize1 = 0xE41C827;

			/* RGB_SIZE = 0xA00 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize2 = 0xA00;

			/* REG_WAKEUP_TIME   = 0x1
			* REG_LINE_DURATION = 0x6AA
			*/
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_dphy_time = 0x26AA;

			/* BLKEOL_PCK        = 0x0
			* BLKLINE_EVENT_PCK = 0xD7
			*/
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize1 = 0xD7;

			/* BLKLINE_PULSE_PCK = 0x0 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize2 = 0x0;

			/* BLKEOL_DURATION = 0xA4 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_pck_time = 0xA4;

			/* BURST_LP        = 0x1
			* MAX_BURST_LIMIT = 0
			*/
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting1 = 0x10000;

			/* MAX_LINE_LIMIT    = 0
			* exact-burst-limit = 0
			*/
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting2 = 0x0;
		}

		if(currentpar->video_mode == VMODE_720_480_60_P)
		{
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vsize = 0x1E009786;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize1 = 0x8008882;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize2 = 0x5A0;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_dphy_time = 0x2724;
	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize1 = 0x0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize2 = 0x0;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_pck_time = 0x3C0;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting1 = 0x10000;
			currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting2 = 0x0;
		}

		if (currentpar->video_mode == VMODE_1920_1080_30_P)
		{
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vsize = 0x43804905;
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize1 = 0x09916C2C;
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize2 = 0xF00;
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_dphy_time = 0x28E3;
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize1 = 0xD7;
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize2 = 0x0;
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_pck_time = 0xAB;
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting1 = 0x10000;
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting2 = 0x0;
		}

		/****************** MCDE CONFIGURATION ***************************/


		/** disable MCDE first then configure */
		currentpar->regbase->mcde_cr=(currentpar->regbase->mcde_cr & ~MCDE_ENABLE);

		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_a0 = currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;

		/** configure mcde external registers */
		if (currentpar->chnl_info->inbpp==16)
			currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_conf=(MCDE_RGB565_16_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;
		if (currentpar->chnl_info->inbpp==32)
			currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_conf=(MCDE_ARGB_32_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;
		if (currentpar->chnl_info->inbpp==24)
			currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_conf=(MCDE_RGB_PACKED_24_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;

		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_cr=(MCDE_FS_FREQ_DIV_DISABLE<<3)|MCDE_BUFFER_SOFTWARE_SELECT;

		/* VCMPC0IM */
		/*currentpar->regbase->mcde_imscpp |= 0x40;*/

		/* Enable buffer ready interrupt */
		currentpar->regbase->mcde_imscchnl |= MCDE_MISCHNL_CHNL_C0;

		/** configure mcde base registers */
		/* IFIFOCTRLWTRMRKLVL = 5
		 * SYNCMUX0 = 1, Channel C chip select 0
		 */
		currentpar->regbase->mcde_conf0 = 0x5E145001;

		if(currentpar->video_mode == VMODE_1280_720_60_P)
		{
			printk(KERN_INFO "VMODE_1280_720_60_P\n");

			/** configure mcde overlay registers */
			currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_cr=0x23CF0001; //0x22b00001;
			currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf=0x2D00500;
			currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf2=0xC000000;//0xA000000;
			currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_ljinc=0xA00;
			currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_comp=0x1000;

			/** configure mcde channel config registers */
			/* LPF = 0x2CF
			* PPL = 0x4FF
			*/
			currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_conf = 0x2CF04FF;

			/* OUT_SYNCH_SRC = LCD/TV 0
			* SRC_SYNCH     = output interfaces
			*/
			currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_synchmod = 0; // To be check if pb

			/* background color R, G, B */
			currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_bckgndcol = 0xFF;

			/* Channel prio */
			currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_prio = 0x0;

			/** configure channel C0 register */

			/* SYNCCTRL = 1, synchro of chC0
			* C1EN
			* POWEREN
			* FLOEN
			*/
            currentpar->ch_c_reg->mcde_crc = 0x3FF80000;
            mdelay(100);
            currentpar->ch_c_reg->mcde_crc = 0x3D790007;

			/** configure mcde DSI formatter */
			/* DCSVID_NOTGEN 1, t as video or dcs CMD
			* 8b command
			*  video mode
			*/
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_conf0 = 0x43000;

			/* FRAME = 0x1C2000 */
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_frame = 0x1C2000;

			/* PACKET = 0xA00 */
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_pkt = 0xA00;

            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_sync=0x3;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_cmdw = 0x002C003C;
		}

		if(currentpar->video_mode == VMODE_640_480_CRT_60_P)
		{
			printk(KERN_INFO "VMODE_640_480_CRT_60_P\n");

			/** configure mcde overlay registers */
            currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_cr=0x23CF0001; //0x22b00001;
            currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf=0x1E00280;
            currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf2=0x5000000;//0xA000000;
            currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_ljinc=0x500;
            currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_comp=0x1000;

            /** configure mcde channel config registers */
            currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_conf = 0x1DF027F;
            currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_synchmod=0; // To be check if pb
            currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_bckgndcol=0xFF00; // Background color
            currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_prio=0x0;

            /** configure channel C0 register */
            currentpar->ch_c_reg->mcde_crc = 0x3FF80000;
            mdelay(100);
            currentpar->ch_c_reg->mcde_crc = 0x3D790007;

            /** configure mcde DSI formatter */
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_conf0=0x43000;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_frame=0x96000;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_pkt=0x500;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_sync=0x3;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_cmdw = 0x002C003C;
		}

		if(currentpar->video_mode == VMODE_720_480_60_P)
		{
			printk(KERN_INFO "VMODE_720_480_60_P\n");

			/** configure mcde overlay registers */
            currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_cr=0x23CF0001; //0x22b00001;
            currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf=0x1E002D0;
            currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf2=0x5A00000;//0xA000000;
            currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_ljinc=0x5A0;
            currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_comp=0x1000;

            /** configure mcde channel config registers */
            currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_conf = 0x1DF02CF;
            currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_synchmod=0; // To be check if pb
            currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_bckgndcol=0xFF; // Background color
            currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_prio=0x0;

            /** configure channel C0 register */
            currentpar->ch_c_reg->mcde_crc = 0x3FF80000;
		mdelay(100);
            currentpar->ch_c_reg->mcde_crc = 0x3D790007;

            /** configure mcde DSI formatter */
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_conf0=0x43000;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_frame=0xA8C00;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_pkt=0x5A0;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_sync=0x0;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_cmdw = 0x002C003C;
		}

		if(currentpar->video_mode == VMODE_1920_1080_30_P)
		{
			printk(KERN_INFO "VMODE_1920_1080_30_P\n");
			currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_cr=0x23CF0001; //0x22b00001;
			currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf=0x04380780;
			currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf2=0xF000000;//0xA000000;
			currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_ljinc=0xF00;
			currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_comp=0x1000;
			currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_conf = 0x0437077f;
			currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_synchmod = 0; // To be check if pb
			currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_bckgndcol = 0xFF;
			currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chnl_prio = 0x0;
            currentpar->ch_c_reg->mcde_crc = 0x3FF80000;
            mdelay(100);
            currentpar->ch_c_reg->mcde_crc = 0x3D790007;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_conf0 = 0x43000;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_frame = 0x003F4800;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_pkt = 0xF00;
            currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_sync=0x3;
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID2]->mcde_dsi_cmdw = 0x002C003C;
	}

		/******************  END OF MCDE CONFIGURATION ***************************/

		/** VSG CONFIGURATION **/
		/*
		 * REG_BLKEOL_MODE  = 0x3
		 * REG_BLKLINE_MODE = 0x3
		 * BURST_MODE       = 1
		 * HEADER           = 0xE
		 */
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_main_ctl = 0x1E4380;

		/* Fill color */
		currentpar->dsi_lnk_registers[DSI_LINK2]->vid_err_color = 0xFFFFFF;

		/* MCDEEN
		 * IFIFOCTRLEN
		 * DSIVID2_EN
		 */
		currentpar->regbase->mcde_cr= 0x80008008;

		/* VID_EN */
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_data_ctl |= 0x4;
	}
	return;
}
EXPORT_SYMBOL(mcde_hdmi_display_init_video_mode);


/**
 Initialization of TPO Display using LP mode and DSI direct command registor  & dispaly of two alternating color
 */
void mcde_hdmi_test_directcommand_mode_highspeed(void)
{
  struct mcdefb_info *currentpar = g_info->par;
  int temp;
  unsigned char PixelRed;
  unsigned char PixelGreen;
  unsigned char PixelBlue;
  int pixel_nb ;
  u32 value = 0;

  PixelRed=0x00;
  PixelGreen=0x1F;
  PixelBlue=0x00;
  pixel_nb = (g_info->var.xres*g_info->var.yres)*(currentpar->chnl_info->inbpp/8);//info->var.xres*info->var.yres;
	for (temp=0; temp< (pixel_nb) ;temp= temp + 15)
	{
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_main_settings=0x103908;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat0=0xAAAAAA2c;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat1=0xAAAAAAAA;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat2=0xAAAAAAAA;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat3=0xAAAAAAAA;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_send=0x1;
	//mdelay(1);
	value = currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts;
	while(value != 0x2)
	{
		printk("Write data is not completed: value:%x\n", value);
		value = currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts;
	};
	//printk("Write data : value:%x\n", value);
	value = 0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts_clr = 0x2;
		//isReady = 0;
     }
//return retVal;
}
EXPORT_SYMBOL(mcde_hdmi_test_directcommand_mode_highspeed);

void mcde_send_hdmi_cmd_data(char* buf,int length, int dsicommand)
{
	struct mcdefb_info *currentpar = g_info->par;
	int value = 0x0;

	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_main_settings=length << 16 | 0x3908;//0x103908;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat0=buf[2] << 24 | buf[1] << 16 | buf[0] << 8 | dsicommand;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat1=buf[6] << 24 | buf[5] << 16 | buf[4] << 8 | buf[3];
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat2=buf[10] << 24 | buf[9] << 16 | buf[8] << 8 | buf[7];
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat3=buf[14] << 24 | buf[13] << 16 | buf[12] << 8 | buf[11];
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_send=0x1;
		//mdelay(1);
	value = currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts;
	while(!(value & 0x2))
	{
		//printk("mcde_send_hdmi_cmd_data:Write data is not completed: value:%x\n", value);
		value = currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts;
	};
		printk("mcde_send_hdmi_cmd_data:Write data : buf[0]:%x,buf[1]:%x,buf[2]:%x,dsicommand:%x, length:%x\n", buf[0], buf[1], buf[2],dsicommand, length);
	value = 0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts_clr = 0x2;
}
EXPORT_SYMBOL(mcde_send_hdmi_cmd_data);

void mcde_send_hdmi_cmd(char* buf,int length, int dsicommand)
{
	struct mcdefb_info *currentpar = g_info->par;
	int value = 0x0;

	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_main_settings=length << 16 | 0x3908;//0x103908;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat0= buf[0] << 8 | dsicommand;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_send=0x1;
		//mdelay(1);
	value = currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts;
	printk("mcde_send_hdmi_cmd:Write data : buf[0]:%x,dsicommand:%x, length:%x\n", buf[0],dsicommand, length);
	while(!(value & 0x2))
	{
		//printk("mcde_send_hdmi_cmd:Write data is not completed: value:%x\n", value);
		value = currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts;
	};
		printk("Write data : value:%x\n", value);
	value = 0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts_clr = 0x2;
}
EXPORT_SYMBOL(mcde_send_hdmi_cmd);
/* mcde_conf_video_mode() - To configure MCDE & DSI hardware registers.
 * @info: frame buffer information.
 *
 * This function configures MCDE & DSI hardware for the device (primary/secondary/HDMI/TVOUT)as per the given configuration
 *
 */
static void mcde_conf_video_mode(struct fb_info *info)
{
	struct mcdefb_info *currentpar = (struct mcdefb_info *)info->par;

#ifdef CONFIG_FB_U8500_MCDE_CHANNELB
#ifndef  CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_VUIB_WVGA
    /** use channel B for tvout & do nothing if it already enabled */
	if((currentpar->chid == MCDE_CH_B) && (!(currentpar->regbase->mcde_cr & MCDE_CR_DPIB_EN))) {
      /** disable MCDE first then configure */
      currentpar->regbase->mcde_cr = (currentpar->regbase->mcde_cr & ~MCDE_CR_MCDEEN);

		  /** configure mcde external registers */
		  currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_a0 = currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;

      switch(currentpar->chnl_info->inbpp)
      {
      case 16:
		default:
        currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_conf = (0x7 << MCDE_EXTSRCCONF_BPP_SHIFT) | /* RGB565 */
                                                                       (1 << MCDE_EXTSRCCONF_BUF_NB_SHIFT); /* 0x704; */
        break;

      case 32:
        currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_conf = (0xA << MCDE_EXTSRCCONF_BPP_SHIFT) | /* ARGB8888 */
                                                                        (1 << MCDE_EXTSRCCONF_BUF_NB_SHIFT); /* 0xA04; */
        break;

      case 24:
        currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_conf = (0x8 << MCDE_EXTSRCCONF_BPP_SHIFT) | /* RGB 24 */
                                                                        (1 << MCDE_EXTSRCCONF_BUF_NB_SHIFT); /*0x804; */
        break;
      }

		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_cr= MCDE_BUFFER_SOFTWARE_SELECT;

    /** configure mcde overlay registers */
    currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_cr = MCDE_OVLCR_OVLEN | (1 << MCDE_OVLCR_COLCCTRL_SHIFT) |
                                                           (0xB << MCDE_OVLCR_BURSTSIZE_SHIFT) |
                                                           (0x2 << MCDE_OVLCR_MAXOUTSTANDING_SHIFT) |
                                                           (0x2 << MCDE_OVLCR_ROTBURSTSIZE_SHIFT); /* 0x22b00003; 20*/

		/** 720 X 576 */
    currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf=(568 << MCDE_OVLCONF_LPF_SHIFT)|(MCDE_EXT_SRC_2<<MCDE_OVLCONF_EXTSRC_ID_SHIFT)|(0x2cc);

		/** rgb888 24 bit format packed data 3 bytes limited to 480 X 682 */
		if (currentpar->chnl_info->inbpp==24)
		  currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf=(0x1E0<<16)|(MCDE_EXT_SRC_2<<11)|(0x2AA);

    /** Tv out requires less watermark ,if other displays are enabled ~ 80 will do */
		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf2=0x500000;
		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_ljinc=(0x2cc)*(currentpar->chnl_info->inbpp/8);
		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_comp=(16<<16)|(MCDE_CH_B<<11)|16;

		/** configure mcde channel config registers */
		currentpar->ch_regbase1[MCDE_CH_B]->mcde_chnl_conf = 0x023702Cb;
		currentpar->ch_regbase1[MCDE_CH_B]->mcde_chnl_synchmod = 0x0;
		currentpar->ch_regbase1[MCDE_CH_B]->mcde_chnl_bckgndcol = 0x00FF0000;

       /** Configure Channel B registers for TVOUT */
    currentpar->ch_regbase2[MCDE_CH_B]->mcde_cr0=0x0;

		if (currentpar->chnl_info->outbpp == MCDE_BPP_1_TO_8)
		  currentpar->ch_regbase2[MCDE_CH_B]->mcde_cr1=0x20001000|4<<MCDE_OUTBPP_SHIFT;
		else if(currentpar->chnl_info->outbpp == MCDE_BPP_12)
		  currentpar->ch_regbase2[MCDE_CH_B]->mcde_cr1=0x20001000|5<<MCDE_OUTBPP_SHIFT;
		else if(currentpar->chnl_info->outbpp == MCDE_BPP_16)
		  currentpar->ch_regbase2[MCDE_CH_B]->mcde_cr1=0x20001000|7<<MCDE_OUTBPP_SHIFT;

    currentpar->ch_regbase2[MCDE_CH_B]->mcde_colkey=0x0;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_fcolkey=0x0;

    /**
     This are video settings and will fail in case of MIRE testing
     So go for GFX settings

		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv1=0x004C0096;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv2=0x001D0083;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv3=0x039203EB;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv4=0x03D403A9;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv5=0x00830080;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv6=0x00000080;
		*/

		currentpar->ch_regbase2[MCDE_CH_B]->mcde_rgbconv1=0x00420081;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_rgbconv2=0x00190070;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_rgbconv3=0x03A203EE;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_rgbconv4=0x03DA03B6;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_rgbconv5=0x00700080;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_rgbconv6=0x00100080;

    currentpar->ch_regbase2[MCDE_CH_B]->mcde_ffcoef0=0x0;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ffcoef1=0x0;
    currentpar->ch_regbase2[MCDE_CH_B]->mcde_ffcoef2=0x0;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_tvcr=0x00000007;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_tvbl1=0x00020018;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_tvisl=0x00170016;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_tvdvo=0x00020002;
		// TODO: Find replacement!
    //currentpar->ch_regbase2[MCDE_CH_B]->mcde_tvswh=0x011C0030;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_tvtim1=0x0000008C;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_tvlbalw=0x00020002;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_tvbl2=0x00020019;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_tvblu=0x002C9C83;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_lcdtim0=0x00000000;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_lcdtim1=0x00200000;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ditctrl=0x00000000;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ditoff=0x00000000;

		currentpar->ch_regbase2[MCDE_CH_B]->mcde_synchconf=0x000B0007;

    /** configure mcde base registers */
		currentpar->regbase->mcde_imscpp |=0x0;
		currentpar->regbase->mcde_conf0 |=0x00185000;
		currentpar->regbase->mcde_cr |=0x80000100;
		mdelay(100);

		/** enable channel flow now */
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_cr0=0x1;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_cr0=0x3;

		mdelay(100);
	}
#else /** CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_VUIB_WVGA */
    /** use channel B for tvout & do nothing if it already enabled */
	if((currentpar->chid==MCDE_CH_B)&&(!(currentpar->regbase->mcde_cr&0x100))) {
        /** disable MCDE first then configure */
        currentpar->regbase->mcde_cr=(currentpar->regbase->mcde_cr & ~ MCDE_ENABLE);

		/** configure mcde external registers */

		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;

        if (currentpar->chnl_info->inbpp==16)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_conf=0x700;
		if (currentpar->chnl_info->inbpp==32)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_conf=0xA00;
		if (currentpar->chnl_info->inbpp==24)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_conf=0x800|1<<12;


		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_cr= 0x6;


        /** configure mcde overlay registers */

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_cr=0x02300301;

		/** WVGA size  800 X 600 */

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf=(0x258<<16)|(MCDE_EXT_SRC_2<<11)|(0x31F);


		/** rgb888 24 bit format packed data 3 bytes limited to 480 X 682 */

		if (currentpar->chnl_info->inbpp==24)

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf=(0x1E0<<16)|(MCDE_EXT_SRC_2<<11)|(0x2AA);

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf2=0x006003FF;

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_ljinc=(0x320)*(currentpar->chnl_info->inbpp/8);

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_comp=(0<<16)|(MCDE_CH_B<<11)|0;


		/** configure mcde channel config registers */

		currentpar->ch_regbase1[MCDE_CH_B]->mcde_ch_conf=0x0257031F;

		currentpar->ch_regbase1[MCDE_CH_B]->mcde_chsyn_mod=0x00000004;

		currentpar->ch_regbase1[MCDE_CH_B]->mcde_chsyn_bck=0x00000000;


       /** Configure Channel B registers for VGA display */

        currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr0=0x0;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr1=0x6E006000;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvcr=0x02580018;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvbl1=0x00020007;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvdvo=0x00000018;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvtim1=0x03200090;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvbalw=0x004E001E;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_lcdtim1=0x00400000;


        /** configure mcde base registers */

		currentpar->regbase->mcde_imsc |=0x8;

		currentpar->regbase->mcde_cfg0 |=0x59237007;

		currentpar->regbase->mcde_cr |=0x80000100;


		mdelay(100);

		/** enable channel flow now */

		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr0=0x1;

		mdelay(100);

		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr0=0x3;

		mdelay(100);

	}
#endif /** CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_VUIB_WVGA */
#endif /** CONFIG_FB_U8500_MCDE_CHANNELB */


#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
	if((currentpar->chid == MCDE_CH_C0) && (!(currentpar->regbase->mcde_cr & currentpar->dsi_formatter)))
	{
		/** disable MCDE first then configure */
		currentpar->regbase->mcde_cr = currentpar->regbase->mcde_cr & ~MCDE_CR_MCDEEN;

		/** configure mcde external registers */

		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;

		switch(currentpar->chnl_info->inbpp) {
		case 16:
			currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_conf = (MCDE_RGB565_16_BIT << MCDE_EXTSRCCONF_BPP_SHIFT) |
				(MCDE_BUFFER_USED_1 << MCDE_EXTSRCCONF_BUF_NB_SHIFT) |
				(MCDE_BUFFER_ID_0 << MCDE_EXTSRCCONF_BUF_ID_SHIFT);
			break;

		case 32:
			currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_conf = (MCDE_ARGB_32_BIT << MCDE_EXTSRCCONF_BPP_SHIFT) |
				(MCDE_BUFFER_USED_1 << MCDE_EXTSRCCONF_BUF_NB_SHIFT) |
				(MCDE_BUFFER_ID_0 << MCDE_EXTSRCCONF_BUF_ID_SHIFT);
			break;

		case 24:
			currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_conf = (MCDE_RGB_PACKED_24_BIT << MCDE_EXTSRCCONF_BPP_SHIFT) |
				(MCDE_BUFFER_USED_1 << MCDE_EXTSRCCONF_BUF_NB_SHIFT) |
				(MCDE_BUFFER_ID_0 << MCDE_EXTSRCCONF_BUF_ID_SHIFT);
			break;

		default:
			printk(KERN_ERR "mcde_conf_video_mode: Unsupported bit depth: %d\n", currentpar->chnl_info->inbpp);
			break;
		}

		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_cr = MCDE_EXTSRCCR_FS_DIV_DISABLE | (0x2 << MCDE_EXTSRCCR_SEL_MOD_SHIFT); /* Software selection */

		/** configure mcde overlay registers */
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_cr = (0xB << MCDE_OVLCR_ROTBURSTSIZE_SHIFT) |
			(0x2 << MCDE_OVLCR_MAXOUTSTANDING_SHIFT) |
			(0xB << MCDE_OVLCR_BURSTSIZE_SHIFT) |
			MCDE_OVLCR_OVLEN; // 0x22b00001;


		/** rgb888 24 bit format packed data 3 bytes limited to 480 X 682 */
		/* This issue is supposed to be fixed on V1. Verify and remove me!
    if(currentpar->chnl_info->inbpp==24)
			currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf = (480 << MCDE_OVLCONF_LPF_SHIFT) |
				(MCDE_EXT_SRC_0 << MCDE_OVLCONF_EXTSRC_ID_SHIFT) |
				(682 << MCDE_OVLCONF_PPL_SHIFT);
        */

		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_comp = currentpar->pixel_pipeline << MCDE_OVLCOMP_CH_ID_SHIFT;

		currentpar->ch_regbase1[currentpar->pixel_pipeline]->mcde_chnl_synchmod = 0x2 << MCDE_CHNLSYNCHMOD_SRC_SYNCH_SHIFT; /* MCDE_SYNCHRO_SOFTWARE; */
		currentpar->ch_regbase1[currentpar->pixel_pipeline]->mcde_chnl_bckgndcol = 0xff << MCDE_CHNLBCKGNDCOL_B_SHIFT; /* blue */

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_WVGA_PORTRAIT

		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf = (0x360 << 16) | (MCDE_EXT_SRC_0 << 11) | (0x1E0);
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_ljinc = 0x1E0 * (currentpar->chnl_info->inbpp / 8);
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf2 = 0x004003FF;

    currentpar->ch_regbase1[currentpar->pixel_pipeline]->mcde_chnl_conf = (863 << MCDE_CHNLCONF_LPF_SHIFT) | (479 << MCDE_CHNLCONF_PPL_SHIFT); /* screen parameters */

    currentpar->ch_regbase2[currentpar->pixel_pipeline]->mcde_rotadd0 = U8500_ESRAM_BASE + 0x20000 * 4;
		currentpar->ch_regbase2[currentpar->pixel_pipeline]->mcde_rotadd1 = U8500_ESRAM_BASE + 0x20000 * 4 + 0x10000;
		currentpar->ch_regbase2[currentpar->pixel_pipeline]->mcde_cr0 = (0xB << 25) | (1 << 24) | (1 << 1) | 1;
		currentpar->ch_regbase2[currentpar->pixel_pipeline]->mcde_cr1 = (1 << 29) | (0x9 << 25) | (0x5 << 10);
    currentpar->ch_regbase2[currentpar->pixel_pipeline]->mcde_rot_conf = (0x1 << 8) | (1 << 3) | (0x7);
		currentpar->regbase->mcde_conf0 = 0x22487001;

#else

  #ifdef CONFIG_MCDE_HDMI_MAINDISP_SAME_FB
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf2 = 0xE5 << MCDE_OVLCONF2_PIXELFETCHWATERMARKLEVEL_SHIFT;
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_ljinc = 0xA00;
  #else
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf2 = 0xa200000;
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_ljinc = 0x360 * (currentpar->chnl_info->inbpp/8);
  #endif

    currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf = (0x1E0<<16)|(MCDE_EXT_SRC_0<<11)|(0x360);
    currentpar->ch_regbase1[currentpar->pixel_pipeline]->mcde_chnl_conf = 0x1df035f; /** screen parameters */
    currentpar->ch_regbase2[currentpar->pixel_pipeline]->mcde_cr0 = (1 << 1) | 1;
		currentpar->ch_regbase2[currentpar->pixel_pipeline]->mcde_cr1 = (1 << 29) | (0x7 << 25) | (0x5 << 10);

		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_cr=0x22b00001;
		currentpar->regbase->mcde_conf0 = 0x22487001;

#endif

		/* Configure channel C0 register */
		/*currentpar->ch_c_reg->mcde_crc = MCDE_CRC_FLOEN | MCDE_CRC_POWEREN | MCDE_CRC_C1EN | MCDE_CRC_WMLVL2 | MCDE_CRC_CS1EN | MCDE_CRC_CS2EN |
			MCDE_CRC_CS1POL | MCDE_CRC_CS2POL | MCDE_CRC_CD1POL | MCDE_CRC_CD2POL | MCDE_CRC_RES1POL | MCDE_CRC_RES2POL |
			(0x1 << MCDE_CRC_SYNCCTRL_SHIFT); // 0x387B0027; TODO: Clean up! Some of these bits are probably not needed
    */

    /** Patch for HW TRIG MCDE **/
    currentpar->ch_c_reg->mcde_crc |= 0x180;
    currentpar->ch_c_reg->mcde_vscrc[0] = 0xFF001;
    currentpar->ch_c_reg->mcde_ctrlc[0] = 0xA0;

		/** configure mcde base registers */
/*		currentpar->regbase->mcde_conf0 |= MCDE_CONF0_SYNCMUX0 | (0x5 << MCDE_CONF0_IFIFOCTRLWTRMRKLVL_SHIFT) |
			(0x4 << MCDE_CONF0_OUTMUX0_SHIFT) | (0x2 << MCDE_CONF0_OUTMUX1_SHIFT) |
			(0x7 << MCDE_CONF0_OUTMUX3_SHIFT) | (0x5 << MCDE_CONF0_OUTMUX4_SHIFT); // 0x5E145001;
		currentpar->regbase->mcde_cr |= MCDE_CR_MCDEEN | MCDE_CR_DSICMD0_EN; //0x80000004;
*/
		currentpar->regbase->mcde_cr |= MCDE_CR_MCDEEN | currentpar->dsi_formatter;

    mdelay(100);

    // TODO!
    currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_en &= ~0x400; /* Disable IF2_EN */
		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_en |= 0x200; /* Enable IF1_EN */
		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_data_ctl &= ~0x2; /* Set interface 1 to command mode */

		/** configure mcde DSI formatter */
		currentpar->mcde_dsi_channel_reg[currentpar->dsi_mode]->mcde_dsi_conf0 = (0x2<<20) | (0x1<<18) | (0x1<<13);
		currentpar->mcde_dsi_channel_reg[currentpar->dsi_mode]->mcde_dsi_frame = (1 + (864 * 24 / 8)) * 480;
		currentpar->mcde_dsi_channel_reg[currentpar->dsi_mode]->mcde_dsi_pkt   = 1 + (864 * 24 / 8);
		currentpar->mcde_dsi_channel_reg[currentpar->dsi_mode]->mcde_dsi_cmdw  = 0x2c003c;

		mdelay(100);

#ifdef CONFIG_MCDE_HDMI_MAINDISP_SAME_FB
		currentpar->regbase->mcde_imscpp |= MCDE_IMSCPP_VCMPC0IM;
		wait_event(currentpar->clcd_event.wait,currentpar->clcd_event.event==1);
		currentpar->clcd_event.event = 0;
#endif

		/* Do a software sync */
#ifndef CONFIG_FB_U8500_MCDE_CHANNELC1
		currentpar->ch_regbase1[currentpar->pixel_pipeline]->mcde_chnl_synchsw = MCDE_CHNLSYNCHSW_SW_TRIG;
#endif

		mdelay(100);
	}
#endif  /** CONFIG_FB_U8500_MCDE_CHANNELC0 */


#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1

	if((currentpar->chid==MCDE_CH_C1)&&(!(currentpar->regbase->mcde_cr&0x2)))
	{

		/** disable MCDE first then configure */
        currentpar->regbase->mcde_cr=(currentpar->regbase->mcde_cr & ~ MCDE_ENABLE);


		/** configure mcde external registers */

		currentpar->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;

		if (currentpar->chnl_info->inbpp==16)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_conf=(MCDE_RGB565_16_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;
		if (currentpar->chnl_info->inbpp==32)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_conf=(MCDE_ARGB_32_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;
		if (currentpar->chnl_info->inbpp==24)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_conf=(MCDE_RGB_PACKED_24_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0|1<<12;

		currentpar->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_cr=(MCDE_FS_FREQ_DIV_DISABLE<<3)|MCDE_BUFFER_SOFTWARE_SELECT;


		/** configure mcde overlay registers */

		currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_cr=0x22b00001;

		currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf=(0x1E0<<16)|(MCDE_EXT_SRC_1<<11)|(0x360);

		//currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf=(0x100<<16)|(MCDE_EXT_SRC_1<<11)|(0x100);

		/** rgb888 24 bit format packed data 3 bytes limited to 480 X 682 */

		if(currentpar->chnl_info->inbpp==24)

		currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf=(0x1E0<<16)|(MCDE_EXT_SRC_1<<11)|(0x2AA);

		currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf2=0xa200000;

		currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_ljinc=(0x360)*(currentpar->chnl_info->inbpp/8);

		currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_comp=(MCDE_CH_C1<<11);



        /** configure mcde channel config registers */

		currentpar->ch_regbase1[MCDE_CH_C1]->mcde_ch_conf=0x1df035f; /** screen parameters */

		currentpar->ch_regbase1[MCDE_CH_C1]->mcde_chsyn_mod=MCDE_SYNCHRO_SOFTWARE;

		currentpar->ch_regbase1[MCDE_CH_C1]->mcde_chsyn_bck=0xff;


        /** configure channel C1 register */

		currentpar->ch_c_reg->mcde_crc |= 0x387B002b;


        /** configure mcde base registers */

#ifndef CONFIG_FB_U8500_MCDE_CHANNELC0

		currentpar->regbase->mcde_imsc |=0x80;
#else

        currentpar->regbase->mcde_imsc |=0xC0;
#endif



		currentpar->regbase->mcde_cfg0 |=0x5E145001;

		currentpar->regbase->mcde_cr |=0x80000002;


		mdelay(100);



        /** configure mcde DSI formatter */

		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD1]->mcde_dsi_conf0=(0x2<<20)|(0x1<<18)|(0x1<<13);

		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD1]->mcde_dsi_frame=(1+(864*24/8))*480;

		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD1]->mcde_dsi_pkt=1+(864*24/8);

		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD1]->mcde_dsi_cmd=(0x3<<4)|MCDE_DSI_VID_MODE_SHIFT;

		mdelay(100);


		/** send DSI command for RAMWR */

		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_main_settings=0x43908;
		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_wrdat0=0x2c;
		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_send=0x1;



		mdelay(100);

        currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_main_settings|=0x200004;

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0

        currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chsyn_sw=MCDE_NEW_FRAME_SYNCHRO;

#endif

		/** do a software sync */
		currentpar->ch_regbase1[MCDE_CH_C1]->mcde_chsyn_sw=MCDE_NEW_FRAME_SYNCHRO;

		mdelay(100);


	}

#endif /** CONFIG_FB_U8500_MCDE_CHANNELC1 */
}


/**
 * mcde_blank() - To disable/enable the device (primary/secondary/HDMI/TVOUT).
 * @blank_mode: enable/disable.
 * @info: frame buffer information.
 *
 * This function configures MCDE & DSI hardware to disable/enable the device(primary/secondary/HDMI/TVOUT) based on the blank state
 *
 */
static int mcde_blank(int blank_mode, struct fb_info *info)
{
	mcde_fb_lock(info, __func__);
	if (blank_mode != 0) {
		/* Disable display engine */
		mcde_disable(info);
	} else {
		/* Enable display engine */
		mcde_enable(info);
	}
	mcde_fb_unlock(info, __func__);

	return 0;
}
/**
 * mcde_set_par_internal() - To set the video mode according to info->var.
 * @info: frame buffer information.
 *
 * This function configures MCDE & DSI hardware for the device(primary/secondary/HDMI/TVOUT) according to info->var.
 *
 */
static int mcde_set_par_internal(struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	u8 border=0;

	/* FIXME border length could be adjustable */
	//if (currentpar->tvout) border=0;
	info->fix.line_length = get_line_length((info->var.xres + (border * 2)), info->var.bits_per_pixel);
	info->fix.visual = (info->var.bits_per_pixel <= 8) ? FB_VISUAL_PSEUDOCOLOR : FB_VISUAL_TRUECOLOR;

	if (info->var.bits_per_pixel > 8)
		currentpar->palette_size = 0;
	else
		currentpar->palette_size = 1 << info->var.bits_per_pixel;

#ifdef CONFIG_FB_MCDE_MULTIBUFFER
       if (currentpar->isHwInitalized == 0)
       {
	mcde_conf_video_mode(info);
	currentpar->isHwInitalized = 1;
       }
#else
	mcde_conf_video_mode(info);
#endif
	return 0;
}

static int mcde_set_par(struct fb_info *info)
{
	mcde_fb_lock(info, __func__);
	mcde_set_par_internal(info);
	mcde_fb_unlock(info, __func__);

  return 0;
}

/**
 * mcde_setpalettereg() - To set the palette configuration.
 * @info: frame buffer information.
 *
 * This function sets the palette configuration for the device(primary/secondary/HDMI/TVOUT).
 *
 */
static int mcde_setpalettereg(u_int red, u_int green, u_int blue,
		u_int trans, struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	mcde_palette palette;

	palette.alphared = (u16)red;
	palette.blue = (u8)blue;
	palette.green = green;
	mcdesetpalette(currentpar->chid, palette);
	return 0;
}
/**
 * mcde_setcolreg() - To set color register.
 * @info: frame buffer information.
 *
 * This function sets the color register for the device(primary/secondary/HDMI/TVOUT).
 *
 */
static int mcde_setcolreg(u_int regno, u_int red, u_int green,
		u_int blue, u_int transp, struct fb_info *info)
{
	u32 v;
	int ret=0;

	if (regno >= 256)  /* no. of hw registers */
		return -EINVAL;
	switch (info->fix.visual) {

		case FB_VISUAL_TRUECOLOR :

			if (regno >= 16)
				return -EINVAL;

#define CNVT_TOHW(val,width) ((((val)<<(width))+0x7FFF-(val))>>16)
			red = CNVT_TOHW(red, info->var.red.length);
			green = CNVT_TOHW(green, info->var.green.length);
			blue = CNVT_TOHW(blue, info->var.blue.length);
			transp = CNVT_TOHW(transp, info->var.transp.length);
#undef CNVT_TOHW

			v = (red << info->var.red.offset) |
				(green << info->var.green.offset) |
				(blue << info->var.blue.offset) |
				(transp << info->var.transp.offset);

			((u32 *) (info->pseudo_palette))[regno] = v;
			break;

		case FB_VISUAL_PSEUDOCOLOR:
			mcde_fb_lock(info, __func__);
			ret=mcde_setpalettereg(red, green, blue, transp, info);
			mcde_fb_unlock(info, __func__);
			break;
	}

	return ret;
}
/**
 * mcde_pan_display() - To display the frame buffer .
 * @var: variable screen info.
 * @info: frame buffer information.
 *
 * This function sets the frame buffer for display and unmasks the vsync interrupt for the device(primary/secondary/HDMI/TVOUT).
 *
 */
static int mcde_pan_display(struct fb_var_screeninfo *var,
   struct fb_info *info)
{
   struct mcdefb_info *currentpar = info->par;

   if(currentpar==0) return -1;

   if (var->vmode & FB_VMODE_YWRAP) {
	   if (var->yoffset >= info->var.yres_virtual
			   || var->xoffset)
		   return -EINVAL;
   } else {
	   if (var->xoffset + var->xres > info->var.xres_virtual ||
			   var->yoffset + var->yres > info->var.yres_virtual)
		   return -EINVAL;
   }

   mcde_fb_lock(info, __func__);

   info->var.xoffset = var->xoffset;
   info->var.yoffset = var->yoffset;

   if (var->vmode & FB_VMODE_YWRAP)
	   info->var.vmode |= FB_VMODE_YWRAP;
   else
	   info->var.vmode &= ~FB_VMODE_YWRAP;

#ifdef CONFIG_FB_MCDE_MULTIBUFFER
	currentpar->clcd_event.base = info->fix.smem_start + (var->yoffset * info->fix.line_length);

	if(currentpar->chid==MCDE_CH_A) {
		if(av8100_started==1) {
			gpar[MCDE_CH_A]->extsrc_regbase[MCDE_EXT_SRC_3]->mcde_extsrc_a0 = currentpar->clcd_event.base;
		}
	}

	if(currentpar->chid == MCDE_CH_C0)
	{
		if(gpar[MCDE_CH_C0]!=0)
		{
			gpar[MCDE_CH_C0]->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_a0 = currentpar->clcd_event.base;
			gpar[MCDE_CH_C0]->regbase->mcde_imscpp |= gpar[MCDE_CH_C0]->vcomp_irq;
			mcde_fb_unlock(info, __func__);
			wait_event(gpar[MCDE_CH_C0]->clcd_event.wait, gpar[MCDE_CH_C0]->clcd_event.event==1);
			gpar[MCDE_CH_C0]->clcd_event.event=0;
		}
	}

	if(currentpar->chid == MCDE_CH_C1)
	{
		if(gpar[MCDE_CH_C1]!=0)
		{
			gpar[MCDE_CH_C1]->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_a0 = currentpar->clcd_event.base;
			gpar[MCDE_CH_C1]->regbase->mcde_imscpp |= MCDE_IMSCPP_VCMPC1IM;
			mcde_fb_unlock(info, __func__);
			wait_event(gpar[MCDE_CH_C1]->clcd_event.wait, gpar[MCDE_CH_C1]->clcd_event.event==1);
			gpar[MCDE_CH_C1]->clcd_event.event=0;
		}
	}

	if(currentpar->chid == MCDE_CH_B)
	{
		/** For Dual display CHA HDMI + CH C0 local CLCD **/
		if(gpar[MCDE_CH_B]!=0)
		{
			gpar[MCDE_CH_B]->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_a0 = currentpar->clcd_event.base;
			gpar[MCDE_CH_B]->regbase->mcde_imscpp |= MCDE_IMSCPP_VCMPBIM;
			mcde_fb_unlock(info, __func__);
			wait_event(gpar[MCDE_CH_B]->clcd_event.wait, gpar[MCDE_CH_B]->clcd_event.event==1);
			gpar[MCDE_CH_B]->clcd_event.event=0;
		}
	}
#else
	mcde_fb_unlock(info, __func__);
#endif //CONFIG_FB_MCDE_MULTIBUFFER

	return 0;
}


/**
  Setting the video mode has been split into two parts.
  First part, xxxfb_check_var, must not write anything
  to hardware, it should only verify and adjust var.
  This means it doesn't alter par but it does use hardware
  data from it to check this var.
*/
static int mcde_check_var(struct fb_var_screeninfo *var,
		struct fb_info *info)
{
	int ret=0;
	u32 linelength;
	/** 5MB : max(lpf) * max (ppl) * (max (bpp) / 8) */
	u32 framesize = (MAX_LPF * MAX_PPL * 4);
	struct mcdefb_info *currentpar = info->par;

	/**
	 *  FB_VMODE_CONUPDATE and FB_VMODE_SMOOTH_XPAN are equal!
	 *  as FB_VMODE_SMOOTH_XPAN is only used internally
	 */
	if (var->vmode & FB_VMODE_CONUPDATE) {
		var->vmode |= FB_VMODE_YWRAP;
		var->xoffset = info->var.xoffset;
		var->yoffset = info->var.yoffset;
	}

	/**
	 *  Some very basic checks
	 */

	if ((var->xres > MAX_PPL) || (var->yres > MAX_LPF)) /** checking ppl,lpf*/
		return -EINVAL;

	if (!var->xres)
		var->xres = 1;
	if (!var->yres)
		var->yres = 1;
	if (var->xres > var->xres_virtual)
		var->xres_virtual = var->xres;
	if (var->yres > var->yres_virtual)
		var->yres_virtual = var->yres;

	if (var->xres_virtual < var->xoffset + var->xres)
		var->xres_virtual = var->xoffset + var->xres;
	if (var->yres_virtual < var->yoffset + var->yres)
		var->yres_virtual = var->yoffset + var->yres;

	/**
	 *  Memory limit
	 */
	linelength = get_line_length(var->xres, var->bits_per_pixel);
	if (linelength * var->yres_virtual > framesize)
		return -ENOMEM;


	/**
	 * Now that we checked it we alter var. The reason being is that the video
	 * mode passed in might not work but slight changes to it might make it
	 * work. This way we let the user know what is acceptable.
	 */

	memset(&(var->transp),0,sizeof(var->transp));


	switch (var->bits_per_pixel) {
		case MCDE_U8500_PANEL_8BPP:
			var->red.offset = 0;
			var->red.length = 8;
			var->green.offset = 0;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
			currentpar->actual_bpp = MCDE_PAL_8_BIT;
			break;

		case MCDE_U8500_PANEL_16BPP:

			switch (currentpar->bpp16_type) {

				case MCDE_U8500_PANEL_12BPP:    /** ARGB 0444 */
					var->red.offset = 8;
					var->red.length = 4;
					var->green.offset = 4;
					var->green.length = 4;
					var->blue.offset = 0;
					var->blue.length = 4;
					currentpar->actual_bpp = MCDE_RGB444_12_BIT;
					break;


				case MCDE_U8500_PANEL_16BPP_ARGB: /** ARGB 4444 */
					var->transp.offset = 12;
					var->transp.length = 4;
					var->red.offset = 8;
					var->red.length = 4;
					var->green.offset = 4;
					var->green.length = 4;
					var->blue.offset = 0;
					var->blue.length = 4;
					currentpar->actual_bpp = MCDE_ARGB_16_BIT;
					break;

				case MCDE_U8500_PANEL_16BPP_IRGB : /** IRGB 1555 */
					var->red.offset = 10;
					var->red.length = 5;
					var->green.offset = 5;
					var->green.length = 5;
					var->blue.offset = 0;
					var->blue.length = 5;
					var->transp.offset = 15;
					var->transp.length = 1;
					currentpar->actual_bpp = MCDE_IRGB1555_16_BIT;
					break;

				case MCDE_U8500_PANEL_16BPP_RGB: /** RGB 565 */
          /* Intentional fallthrough */
				default:
					var->red.offset = 11;
					var->red.length = 5;
					var->green.offset = 5;
					var->green.length = 6;
					var->blue.offset = 0;
					var->blue.length = 5;
					currentpar->actual_bpp = MCDE_RGB565_16_BIT;
					break;

			}
			break;

		case MCDE_U8500_PANEL_24BPP_PACKED:/** RGB 888 */
			var->red.offset = 16;
			var->red.length = 0;
			var->green.offset = 8;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
			var->bits_per_pixel = 24;  /* Reassign to actual value*/
			currentpar->actual_bpp = MCDE_RGB_PACKED_24_BIT;
			break;

		case MCDE_U8500_PANEL_24BPP:
			var->red.offset = 16;
			var->red.length = 8;
			var->green.offset = 8;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
			currentpar->actual_bpp = MCDE_RGB_UNPACKED_24_BIT;
			break;

		case MCDE_U8500_PANEL_32BPP:/** ARGB 8888 */
			var->red.offset = 16;
			var->red.length = 8;
			var->green.offset = 8;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
			var->transp.offset = 24;
			var->transp.length = 8;
			currentpar->actual_bpp = MCDE_ARGB_32_BIT;
			break;

		case MCDE_U8500_PANEL_YCBCR:
			currentpar->actual_bpp = MCDE_YCbCr_8_BIT;
			currentpar->outbpp = MCDE_BPP_1_TO_8;
			/**YCBCR is made of 2 byte per pixel*/
			var->bits_per_pixel = 16;
			break;

		default:
			ret = -EINVAL;
			break;
	}


	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;
	return ret;
}
/**
 * mcde_mmap() - Maps the kerenel FB memory to user space memory.
 * @info: frame buffer information.
 * @vma: virtual area memory structure.
 *
 * This routine maps the kerenel FB memory to user space memory and returns the source buffer index.
 *
 */
static int mcde_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct mcdefb_info *currentpar=NULL;
	unsigned long key = vma->vm_pgoff ;
	int ret = -EINVAL;

	/** Aquire module mutex */
//	down_interruptible(&mcde_module_mutex);

	currentpar = (struct mcdefb_info *) info->par;
	if (key > ((MCDE_MAX_FRAMEBUFF*2) -1)) return -EINVAL;

		if (key < num_of_display_devices) /* for primary & secondary devices */
		key = currentpar->mcde_cur_ovl_bmp;

	vma->vm_flags |= VM_RESERVED;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	/** its a framebuffer to be mapped */
	ret = remap_pfn_range(vma,vma->vm_start,((currentpar->buffaddr[key].dmaaddr) >> PAGE_SHIFT),
			(vma->vm_end - vma->vm_start), vma->vm_page_prot) ;

	/** Release module mutex */
//	up(&mcde_module_mutex);

	return ret;
}
/**
 * find_video_mode() - To find the respective video mode from the ipnut configuration.
 * @info: frame buffer information.
 *
 * This routine updates the video mode from input configuration to currentpar->video_mode.
 */
void find_video_mode(struct fb_info *info)
{
	struct mcdefb_info *currentpar=NULL;
	currentpar = (struct mcdefb_info *) info->par;

	if (strcmp(currentpar->chnl_info->restype,"SDTV")==0)
		currentpar->video_mode = VMODE_712_568_60_P;
	else if (strcmp(currentpar->chnl_info->restype,"VGA")==0)
		currentpar->video_mode = VMODE_640_480_60_P;
	else if (strcmp(currentpar->chnl_info->restype,"WVGA_Portrait")==0)
		currentpar->video_mode = VMODE_480_864_60_P;
	else if (strcmp(currentpar->chnl_info->restype,"WVGA")==0)
		currentpar->video_mode = VMODE_864_480_60_P;
	else if (strcmp(currentpar->chnl_info->restype,"HDTV")==0)
		currentpar->video_mode = VMODE_1920_1080_50_I;
	else if (strcmp(currentpar->chnl_info->restype,"QVGA_Portrait")==0)
		currentpar->video_mode = VMODE_240_320_60_P;
	else if (strcmp(currentpar->chnl_info->restype,"QVGA_Landscape")==0)
		currentpar->video_mode = VMODE_320_240_60_P;
	else if (strcmp(currentpar->chnl_info->restype,"VUIB WVGA")==0)
		currentpar->video_mode = VMODE_800_600_56_P;
	else if (strcmp(currentpar->chnl_info->restype,"HDMI C0")==0)
		currentpar->video_mode = VMODE_1920_1080_30_P;
	else
		printk(KERN_ERR "find_video_mode: Unsupported video mode: %s\n", currentpar->chnl_info->restype);
}

/**
 * find_restype_from_video_mode() - To find the restype from the given video mode.
 * @info: frame buffer information.
 * @videoMode: video mode.
 *
 * This routine updates the restype from the given video mode to currentpar->chnl_info->restype.
 */
void find_restype_from_video_mode(struct fb_info *info, mcde_video_mode videoMode)
{
	struct mcdefb_info *currentpar=NULL;
	currentpar = (struct mcdefb_info *) info->par;
	if(videoMode == VMODE_712_568_60_P)
		currentpar->chnl_info->restype = "SDTV";
	else if(videoMode == VMODE_640_480_60_P)
		currentpar->chnl_info->restype = "VGA";
	else if(videoMode == VMODE_480_864_60_P)
		currentpar->chnl_info->restype = "WVGA_Portrait";
	else if(videoMode == VMODE_864_480_60_P)
		currentpar->chnl_info->restype = "WVGA";
	else if(videoMode == VMODE_1920_1080_50_I)
		currentpar->chnl_info->restype = "HDTV";
	else if(videoMode == VMODE_240_320_60_P)
		currentpar->chnl_info->restype = "QVGA_Portrait";
	else if(videoMode == VMODE_320_240_60_P)
		currentpar->chnl_info->restype = "QVGA_Landscape";
	else if(videoMode == VMODE_800_600_56_P)
		currentpar->chnl_info->restype = "VUIB WVGA";
	else if(videoMode == VMODE_1920_1080_30_P)
		currentpar->chnl_info->restype = "HDMI C0";
	else {
		currentpar->chnl_info->restype = NULL;
		printk(KERN_ERR "find_restype_from_video_mode: Unsupported mode\n");
	}
}

/**
 * mcde_set_video_mode() - Sets the video mode configuration as per video mode.
 * @info: frame buffer information.
 * @videoMode: video mode.
 *
 * This routine updates the info->var structure based on the video mode, allocates the memory for frame buffer
 * and calls mcde_set_par() to configure the hardware.
 *
 */
static int mcde_set_video_mode(struct fb_info *info, mcde_video_mode videoMode)
{
   struct mcde_sourcebuffer_alloc source_buff;
   struct mcdefb_info *currentpar=NULL;
   s32 retVal = MCDE_OK;
   u32 mcdeOverlayId;

   currentpar = (struct mcdefb_info *) info->par;
  /* This should give a reasonable default video mode */
  /*  use the video database to search for appropriate mode...sets info->var also */
   info->var.activate = FB_ACTIVATE_NOW;
   info->var.height = -1;
   info->var.width = -1;

   if (!fb_find_mode(&info->var, info, currentpar->chnl_info->restype, mcde_modedb, NUM_TOTAL_MODES, &mcde_modedb[videoMode], currentpar->chnl_info->inbpp))
   {
	dbgprintk(MCDE_ERROR_INFO, "mcde_set_video_mode fb_find_mode failed\n");
	info->var = mcde_default_var;
   }

#ifdef CONFIG_FB_MCDE_MULTIBUFFER
   info->var.yres_virtual	= info->var.yres*2;
#endif

   printk(KERN_INFO "\nxres:%d yres:%d bpp:%d\n", info->var.xres, info->var.yres, info->var.bits_per_pixel);

   /** current monitor spec */
   info->monspecs.hfmin = 0;              /**hor freq min*/
   info->monspecs.hfmax = 100000;         /**hor freq max*/
   info->monspecs.vfmin = 0;              /**ver freq min*/
   info->monspecs.vfmax = 400;            /**ver freq max*/
   info->monspecs.dclkmin = 1000000;      /**pixclocl min */
   info->monspecs.dclkmax = 100000000;    /**pixclock max*/

   /**  allocate color map...*/
   retVal = fb_alloc_cmap(&info->cmap, 256, 0);
   if (retVal < 0)
	goto out_fbinfodealloc;

   /** allocate memory */
   source_buff.xwidth = info->var.xres;
   source_buff.yheight = info->var.yres;
   source_buff.bpp = info->var.bits_per_pixel;
#ifdef CONFIG_FB_MCDE_MULTIBUFFER
   source_buff.doubleBufferingEnabled = 1;
#else
   source_buff.doubleBufferingEnabled = 0;
#endif
   mcde_alloc_source_buffer(source_buff, info, &mcdeOverlayId, FALSE);
   if (!currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].cpuaddr) {
	dbgprintk(MCDE_ERROR_INFO, "Failed to allocate framebuffer memory\n");
	retVal=-ENOMEM;
	goto out_unmap;
   }
   memset((void *)currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].cpuaddr,0,currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].bufflength);
   info->fix.smem_start = currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;             /** physical address of frame buffer */
   info->fix.smem_len   = currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].bufflength;

   /** update info->screen_base, info->fix.line_length to access the FB by kernel */
   info->fix.line_length =  get_line_length(info->var.xres, info->var.bits_per_pixel);
   info->screen_base = (u8 *)currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].cpuaddr;

#ifndef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_HDMI
   /** initialize the hardware as per the configuration */
	/** in case of HDMI done in the mcde_hdmi_display_init_video_mode function called by AV8100 driver */
   mcde_set_par_internal(info);
#endif

   return retVal;

out_unmap:
   fb_dealloc_cmap(&info->cmap);
out_fbinfodealloc:

   return retVal;
}
/**
 * mcde_get_hdmi_flag() - To give HDMI inclusion status.
 *
 * This routine gives the Flow A for HDMI is included.
 */
bool mcde_get_hdmi_flag(void)
{
	printk("mcde_get_hdmi_flag................. %d\n", isHdmiEnabled);
	return isHdmiEnabled;
}
EXPORT_SYMBOL(mcde_get_hdmi_flag);

/**
 * mcde_dsi_set_params() - To update the fb_info structure with the mcde and dsi configuration parameters for the device.
 * @info: frame buffer information.
 *
 * This routine updates the currentpar structure with the mcde and dsi configuration parameters(lowpower/highspeed)
 * and initializes the mcde,hdmi,tvout clocks
 */
static int mcde_dsi_set_params(struct fb_info *info)
{
   struct mcdefb_info *currentpar=NULL;
   int retval = MCDE_OK;

   currentpar = (struct mcdefb_info *) info->par;
   find_video_mode(info);
   switch(currentpar->chid)
   {
	case CHANNEL_A:
		isHdmiEnabled = TRUE;
		currentpar->fifoOutput = MCDE_DSI_CMD2;
		currentpar->mcdeDsiChnl = MCDE_DSI_CH_CMD2;
		currentpar->output_conf = MCDE_CONF_DSI;

		/* HDMI resolution */
		/* default */
		currentpar->video_mode = VMODE_1280_720_60_P;
#ifdef CONFIG_FB_U8500_MCDE_HDMI_1920x1080P30
		currentpar->video_mode = VMODE_1920_1080_30_P;
#endif
#ifdef CONFIG_FB_U8500_MCDE_HDMI_1280x720P60
		currentpar->video_mode = VMODE_1280_720_60_P;
#endif
#ifdef CONFIG_FB_U8500_MCDE_HDMI_720x480P60
		currentpar->video_mode = VMODE_720_480_60_P;
#endif
#ifdef CONFIG_FB_U8500_MCDE_HDMI_640x480P60
		currentpar->video_mode = VMODE_640_480_CRT_60_P;
#endif
		if ((currentpar->fifoOutput >= MCDE_DSI_VID0) && (currentpar->fifoOutput <= MCDE_DSI_CMD2))
		{
			currentpar->dsi_lnk_no = 2;
		    currentpar->dsi_lnk_context.dsi_if_mode = DSI_COMMAND_MODE;
                    currentpar->dsi_lnk_context.dsiInterface = DSI_INTERFACE_2;
                    currentpar->dsi_lnk_context.if_mode_type = DSI_CLK_LANE_HSM;
                    currentpar->dsi_lnk_conf.dsiInterface = currentpar->dsi_lnk_context.dsiInterface;
		    currentpar->dsi_lnk_conf.clockContiniousMode = DSI_CLK_CONTINIOUS_HS_DISABLE;
		    currentpar->dsi_lnk_conf.dsiLinkState = DSI_ENABLE;
                    currentpar->dsi_lnk_conf.clockLaneMode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane1Mode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane2Mode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dsiInterfaceMode = currentpar->dsi_lnk_context.dsi_if_mode;
                    currentpar->dsi_lnk_conf.commandModeType = currentpar->dsi_lnk_context.if_mode_type;
			g_info = info;
                }
		break;
	case CHANNEL_B:
		PrimaryDisplayFlow = CHANNEL_B; /** for interrupt access */
		currentpar->fifoOutput = MCDE_DPI_B;
		//currentpar->video_mode =  VMODE_640_480_60_P;
		currentpar->output_conf = MCDE_CONF_DPIC0_LCDB;
		currentpar->mcdeDsiChnl = MCDE_DSI_CH_CMD0;
		if ((currentpar->fifoOutput >= MCDE_DSI_VID0) && (currentpar->fifoOutput <= MCDE_DSI_CMD2))
		{
		    currentpar->dsi_lnk_no = 0;
		    currentpar->dsi_lnk_context.dsi_if_mode = DSI_VIDEO_MODE;
                    currentpar->dsi_lnk_context.dsiInterface = DSI_INTERFACE_1;
                    currentpar->dsi_lnk_context.if_mode_type = DSI_CLK_LANE_HSM;
                    currentpar->dsi_lnk_conf.dsiInterface = currentpar->dsi_lnk_context.dsiInterface;
		    currentpar->dsi_lnk_conf.clockContiniousMode = DSI_CLK_CONTINIOUS_HS_DISABLE;
		    currentpar->dsi_lnk_conf.dsiLinkState = DSI_ENABLE;
                    currentpar->dsi_lnk_conf.clockLaneMode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane1Mode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane2Mode = DSI_LANE_DISABLE;
                    currentpar->dsi_lnk_conf.dsiInterfaceMode = currentpar->dsi_lnk_context.dsi_if_mode;
                    currentpar->dsi_lnk_conf.commandModeType = currentpar->dsi_lnk_context.if_mode_type;
                }
		break;

	case CHANNEL_C0:
		printk(KERN_INFO "%s: case CHANNEL_C0\n", __func__);
#ifndef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_HDMI
		currentpar->fifoOutput = MCDE_DSI_CMD0;
		currentpar->mcdeDsiChnl = MCDE_DSI_CH_CMD0;
		currentpar->output_conf = MCDE_CONF_DSI;
		if ((currentpar->fifoOutput >= MCDE_DSI_VID0) && (currentpar->fifoOutput <= MCDE_DSI_CMD2))
		{
		    currentpar->dsi_lnk_no = 0;/** This has to be changed to 1 */
		    currentpar->dsi_lnk_context.dsi_if_mode = DSI_COMMAND_MODE;
                    currentpar->dsi_lnk_context.dsiInterface = DSI_INTERFACE_2;
                    currentpar->dsi_lnk_context.if_mode_type = DSI_CLK_LANE_HSM;
                    currentpar->dsi_lnk_conf.dsiInterface = currentpar->dsi_lnk_context.dsiInterface;
		    currentpar->dsi_lnk_conf.clockContiniousMode = DSI_CLK_CONTINIOUS_HS_DISABLE;
		    currentpar->dsi_lnk_conf.dsiLinkState = DSI_ENABLE;
                    currentpar->dsi_lnk_conf.clockLaneMode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane1Mode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane2Mode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dsiInterfaceMode = currentpar->dsi_lnk_context.dsi_if_mode;
                    currentpar->dsi_lnk_conf.commandModeType = currentpar->dsi_lnk_context.if_mode_type;
		}
#else
		isHdmiEnabled = TRUE;
		currentpar->fifoOutput = MCDE_DSI_VID2;
		currentpar->mcdeDsiChnl = MCDE_DSI_CH_VID2;
		currentpar->output_conf = MCDE_CONF_DSI;
		currentpar->video_mode = VMODE_1920_1080_30_P;
		//currentpar->video_mode = VMODE_1280_720_60_P;
		//currentpar->video_mode = VMODE_640_480_CRT_60_P; // VGA 60FPS
		//currentpar->video_mode =VMODE_720_480_60_P; //480p 60FPS

		if ((currentpar->fifoOutput >= MCDE_DSI_VID0) && (currentpar->fifoOutput <= MCDE_DSI_CMD2))
		{
			printk(KERN_INFO "%s: case CHANNEL_C0, setting dsi_*\n", __func__);

			currentpar->dsi_lnk_no = 2;
			currentpar->dsi_lnk_context.dsi_if_mode = DSI_VIDEO_MODE;
			currentpar->dsi_lnk_context.dsiInterface = DSI_INTERFACE_2;
			currentpar->dsi_lnk_context.if_mode_type = DSI_CLK_LANE_HSM;
			currentpar->dsi_lnk_conf.dsiInterface = currentpar->dsi_lnk_context.dsiInterface;
			currentpar->dsi_lnk_conf.clockContiniousMode = DSI_CLK_CONTINIOUS_HS_DISABLE;
			currentpar->dsi_lnk_conf.dsiLinkState = DSI_ENABLE;
			currentpar->dsi_lnk_conf.clockLaneMode = DSI_LANE_ENABLE;
			currentpar->dsi_lnk_conf.dataLane1Mode = DSI_LANE_ENABLE;
			currentpar->dsi_lnk_conf.dataLane2Mode = DSI_LANE_DISABLE;
			currentpar->dsi_lnk_conf.dsiInterfaceMode = currentpar->dsi_lnk_context.dsi_if_mode;
			currentpar->dsi_lnk_conf.commandModeType = currentpar->dsi_lnk_context.if_mode_type;
			currentpar->dsi_lnk_conf.hostEotGenMode = DSI_HOST_EOT_GEN_ENABLE;
			currentpar->dsi_lnk_conf.displayEotGenMode = DSI_EOT_GEN_ENABLE;
                }
#endif
		break;

	case CHANNEL_C1:
		currentpar->fifoOutput = MCDE_DSI_CMD0;
		currentpar->mcdeDsiChnl = MCDE_DSI_CH_CMD0;
		currentpar->output_conf = MCDE_CONF_DSI;
		if ((currentpar->fifoOutput >= MCDE_DSI_VID0) && (currentpar->fifoOutput <= MCDE_DSI_CMD2))
		{
		    currentpar->dsi_lnk_no = 2;
		    currentpar->dsi_lnk_context.dsi_if_mode = DSI_COMMAND_MODE;
                    currentpar->dsi_lnk_context.dsiInterface = DSI_INTERFACE_2;
                    currentpar->dsi_lnk_context.if_mode_type = DSI_CLK_LANE_HSM;
                    currentpar->dsi_lnk_conf.dsiInterface = currentpar->dsi_lnk_context.dsiInterface;
		    currentpar->dsi_lnk_conf.clockContiniousMode = DSI_CLK_CONTINIOUS_HS_DISABLE;
		    currentpar->dsi_lnk_conf.dsiLinkState = DSI_ENABLE;
                    currentpar->dsi_lnk_conf.clockLaneMode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane1Mode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane2Mode = DSI_LANE_DISABLE;
                    currentpar->dsi_lnk_conf.dsiInterfaceMode = currentpar->dsi_lnk_context.dsi_if_mode;
                    currentpar->dsi_lnk_conf.commandModeType = currentpar->dsi_lnk_context.if_mode_type;
                    currentpar->dsi_lnk_conf.hostEotGenMode = DSI_HOST_EOT_GEN_ENABLE;
                    currentpar->dsi_lnk_conf.displayEotGenMode = DSI_EOT_GEN_ENABLE;
                }
		break;
    }
    if ((currentpar->fifoOutput >= MCDE_DSI_VID0) && (currentpar->fifoOutput <= MCDE_DSI_CMD2))
    {
	/** enabling the MCDE and HDMI clocks */
        if (currentpar->dsi_lnk_context.if_mode_type == DSI_CLK_LANE_LPM)
        {
		*(currentpar->prcm_mcde_clk) = 0x125; /** MCDE CLK = 160MHZ */
		*(currentpar->prcm_hdmi_clk) = 0x145; /** HDMI CLK = 76.8MHZ */
		currentpar->clk_config.pllout_divsel2 = MCDE_PLL_OUT_OFF;
		currentpar->clk_config.pllout_divsel1 = MCDE_PLL_OUT_OFF;
		currentpar->clk_config.pllout_divsel0 = MCDE_PLL_OUT_OFF;
	        currentpar->clk_config.pll4in_sel = MCDE_CLK27;
		currentpar->clk_config.txescdiv_sel = MCDE_DSI_MCDECLK;//MCDE_DSI_CLK27;
	        currentpar->clk_config.txescdiv = 16;
	        currentpar->dsi_lnk_conf.hostEotGenMode = DSI_HOST_EOT_GEN_DISABLE;
                currentpar->dsi_lnk_conf.displayEotGenMode = DSI_EOT_GEN_DISABLE;
        }
        if (currentpar->dsi_lnk_context.if_mode_type == DSI_CLK_LANE_HSM)
        {
		*(currentpar->prcm_mcde_clk) = 0x125; /** MCDE CLK = 160MHZ */
		*(currentpar->prcm_hdmi_clk) = 0x145; /** HDMI CLK = 76.8MHZ */
		*(currentpar->prcm_tv_clk) = 0x14E; /** HDMI CLK = 76.8MHZ */


		currentpar->clk_config.pllout_divsel2 = MCDE_PLL_OUT_OFF;
		    currentpar->clk_config.pllout_divsel1 = MCDE_PLL_OUT_OFF;
		currentpar->clk_config.pllout_divsel0 =MCDE_PLL_OUT_2;
	        currentpar->clk_config.pll4in_sel = MCDE_TV1CLK;//MCDE_HDMICLK;
		currentpar->clk_config.txescdiv_sel = MCDE_DSI_MCDECLK;
	        currentpar->clk_config.txescdiv = 16;
	        currentpar->dsi_lnk_conf.hostEotGenMode = DSI_HOST_EOT_GEN_ENABLE;
                currentpar->dsi_lnk_conf.displayEotGenMode = DSI_EOT_GEN_ENABLE;

        }
    }
    return retval;
}
#ifdef TESTING
/**
 * mcde_open() - This routine opens the device.
 * @info: frame buffer information.
 * @arg: input arguements
 *
 * This routine opens the requested frame buffer device.
 *
 */
static int mcde_open(struct fb_info *info, u32 arg)
{
   struct mcdefb_info *currentpar=NULL;
   int retval = MCDE_OK;

   currentpar = (struct mcdefb_info *) info->par;
   switch(currentpar->chid)
   {
	case CHANNEL_A:
		mcde_set_video_mode(info, VMODE_640_480_60_P);
		break;
	case CHANNEL_B:
		mcde_set_video_mode(info, VMODE_640_480_60_P);
		break;

	case CHANNEL_C0:
		mcde_set_video_mode(info, VMODE_640_480_60_P);
		break;

	case CHANNEL_C1:
		mcde_set_video_mode(info, VMODE_640_480_60_P);
		break;
    }

    return retval;
}
/**
 * mcde_open() - This routine closes the device.
 * @info: frame buffer information.
 * @arg: input arguements
 *
 * This routine closes the requested frame buffer device.
 *
 */
static int mcde_release(struct fb_info *info, u32 arg)
{
	dma_free_coherent(info->dev, info->fix.smem_len, info->screen_base, info->fix.smem_start);
	fb_dealloc_cmap(&info->cmap);
	return 0;
}
#endif

static struct fb_ops mcde_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = mcde_check_var,
#ifndef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_HDMI
	.fb_set_par = mcde_set_par,
#endif
	.fb_setcolreg = mcde_setcolreg,
	.fb_ioctl = mcde_ioctl,
	.fb_mmap = mcde_mmap,
	.fb_blank = mcde_blank,
	.fb_pan_display = mcde_pan_display,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
};

static int __init mcde_probe(struct platform_device *pdev)
{
	struct fb_info *info;
	struct mcdefb_info *currentpar=NULL;
	int retVal = -ENODEV;
	struct mcde_channel_data *channel_info;
	struct device *dev;
	struct resource *res;
	u8 irqline = 0, temp = 0;
	char __iomem *vbuffaddr;
	dma_addr_t dma;
	u8* extSrcAddr;
	u8 * ovrlaySrcAddr = NULL;
	u8 * chnlSyncAddr = NULL;
	u8 * dsiChanlRegAdd = NULL;
	u8  *dsilinkAddr = NULL;
	u32 rotationframesize = 0;
	int data_4500;

#ifdef  CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_VUIB_WVGA
	volatile u32 __iomem *clcd_clk;
#endif

    /** 5MB : max(lpf) * max (ppl) * (max (bpp) / 8)*/
#ifdef CONFIG_FB_MCDE_MULTIBUFFER
	//16Bpp and double buffering
	u32 framesize = (MAX_LPF * MAX_PPL * 2)*2;
#else
	//32 Bpp and single buffering
	u32 framesize = (MAX_LPF * MAX_PPL * 4);
#endif


	dev = &pdev->dev;
	channel_info = (struct mcde_channel_data *) dev->platform_data;

	printk(KERN_ERR "Init i2c\n");

	/** To be removed I2C */
	if(!i2c_init_done)
	{
		/** Make display reset to low */
		if(platform_id==0)
		{
			/** why set directtion is not working ~ FIXME */
//			gpio_direction_output(EGPIO_PIN_14,1);
//			gpio_direction_output(EGPIO_PIN_15,1);
// Removed EDtoV1
			gpio_set_value(EGPIO_PIN_14, 0);
			gpio_set_value(EGPIO_PIN_15, 0);
		}

		if(platform_id==1)
		{
// Removed EDtoV1
			gpio_direction_output(282,1);
			gpio_direction_output(283,1);
			gpio_set_value(282,0);
			gpio_set_value(283,0);
		}

		mdelay(1); /** let the low value settle  */

#ifndef CONFIG_U8500_SIM8500
#if 0
		data_4500=ab8500_read(0x4,0x409);
		data_4500|=0x1;
		ab8500_write(0x4,0x409,data_4500);

		data_4500=ab8500_read(0x4,0x41F);
		data_4500|=0x8;
		ab8500_write(0x4,0x41F,data_4500);

		mdelay(1);/** let the voltage settle */
#endif
#endif
		/** Make display reset to high */
		if(platform_id==0)
		{
			/** why set directtion is not working ~ FIXME */
//			gpio_direction_output(EGPIO_PIN_14,1);
//			gpio_direction_output(EGPIO_PIN_15,1);
// Removed EDtoV1
			gpio_set_value(EGPIO_PIN_14, 1);
			gpio_set_value(EGPIO_PIN_15, 1);
		}

		if(platform_id==1)
		{
// Removed EDtoV1
			gpio_direction_output(282,1);
			gpio_direction_output(283,1);
			gpio_set_value(282,1);
			gpio_set_value(283,1);
		}

		mdelay(1); /** let the high value settle  */
		i2c_init_done=1;
	}

	/* MCDE and DSI setup */
	if(!prcmu_init_done) {
		mcde_environment_setup();
		prcmu_init_done = 1;
	}

	printk(KERN_ERR "Allocing framebuffer\n");

	/** allocate fb_info + your device specific structure */
	info = framebuffer_alloc(sizeof(struct mcdefb_info), dev);
	if (!info) {
		dbgprintk(MCDE_ERROR_INFO, "Failed to allocate framebuffer structures\n");
		retVal = -ENOMEM;
		return retVal ;
	}


	currentpar = (struct mcdefb_info *) info->par;
	currentpar->chid = channel_info->channelid;
	currentpar->outbpp = channel_info->outbpp;
	currentpar->bpp16_type = channel_info->bpp16_type;
	currentpar->bgrinput = channel_info->bgrinput;
	currentpar->chnl_info = channel_info;

	init_MUTEX(&currentpar->fb_sem);

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
	if (currentpar->chid == CHANNEL_C0) {
#ifdef CONFIG_FB_U8500_MCDE_CHANNELA
		currentpar->pixel_pipeline = MCDE_CH_B; // Channel A used by HDMI
		currentpar->vcomp_irq = 0x2;
		currentpar->dsi_formatter = 0x20 | 0x20000; // DSIVID0_EN | FABMUX
		currentpar->dsi_mode = MCDE_DSI_CH_VID0;
#else
		currentpar->pixel_pipeline = MCDE_CH_A; // Channel B used by TV-out
		currentpar->vcomp_irq = 0x1;
		currentpar->dsi_formatter = 0x20; // DSIVID0_EN
		currentpar->dsi_mode = MCDE_DSI_CH_VID0;
#endif
	}
#endif

	/***********************/
	/** MCDE top register **/
	/***********************/
        res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL)
		goto out_fballoc;

	currentpar->regbase = (struct mcde_top_reg *) ioremap(res->start, res->end - res->start + 1);
	if (currentpar->regbase == NULL)
		goto out_res;

	/*******************************/
	/** External source registers **/
	/*******************************/
        res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res == NULL)
		goto out_unmap1;

	extSrcAddr = (u8 *) ioremap(res->start, res->end - res->start + 1);
	if (extSrcAddr == NULL)
		goto out_unmap1;
	for (temp = 0; temp < NUM_EXT_SRC; temp++)
		currentpar->extsrc_regbase[temp] = (struct mcde_ext_src_reg*)(extSrcAddr + (temp*U8500_MCDE_REGISTER_BANK_SIZE));

	/***********************/
	/** Overlay registers **/
	/***********************/
        res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (res == NULL)
		goto out_unmap2;

	ovrlaySrcAddr = (u8 *) ioremap(res->start, res->end - res->start + 1);
	if (ovrlaySrcAddr == NULL)
		goto out_unmap2;
	for (temp = 0; temp < NUM_OVERLAYS; temp++)
		currentpar->ovl_regbase[temp] = (struct mcde_ovl_reg*)(ovrlaySrcAddr + (temp*U8500_MCDE_REGISTER_BANK_SIZE));

	/**************************************/
	/**  Channel configuration registers **/
	/**************************************/
	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if (res == NULL)
		goto out_unmap3;

	chnlSyncAddr = (u8 *) ioremap(res->start, res->end - res->start + 1);
	if (chnlSyncAddr == NULL)
		goto out_unmap3;

	for (temp = 0; temp < NUM_MCDE_CHANNELS; temp++)
		currentpar->ch_regbase1[temp] = (struct mcde_chnl_conf_reg*)(chnlSyncAddr + (temp*U8500_MCDE_REGISTER_BANK_SIZE));

	/********************************/
	/** Channel specific registers **/
	/********************************/
	res = platform_get_resource(pdev, IORESOURCE_MEM, 4);
	if (res == NULL)
		goto out_unmap4;

	if (currentpar->chid == CHANNEL_A || currentpar->chid == CHANNEL_B)
	{
		currentpar->ch_regbase2[channel_info->channelid] = (struct mcde_chAB_reg *) ioremap(res->start, res->end - res->start + 1);
		if (currentpar->ch_regbase2[channel_info->channelid] == NULL)
			goto out_unmap4;
	} else if (currentpar->chid == CHANNEL_C0 || currentpar->chid == CHANNEL_C1) {
		currentpar->ch_c_reg = (struct mcde_chC0C1_reg *) ioremap(res->start, res->end - res->start + 1);
		if (currentpar->ch_c_reg == NULL)
			goto out_unmap4;

		if (currentpar->chid == CHANNEL_C0) {
			/* Also map channel A and B! */
			res = platform_get_resource(pdev, IORESOURCE_MEM, 10);
			currentpar->ch_regbase2[MCDE_CH_A] = (struct mcde_chAB_reg *) ioremap(res->start, res->end - res->start + 1);
			if (currentpar->ch_regbase2[MCDE_CH_A] == NULL) {
				goto out_unmap5;
			}

			res = platform_get_resource(pdev, IORESOURCE_MEM, 11);
			currentpar->ch_regbase2[MCDE_CH_B] = (struct mcde_chAB_reg *) ioremap(res->start, res->end - res->start + 1);
			if (currentpar->ch_regbase2[MCDE_CH_B] == NULL) {
				goto out_unmap6;
			}
		}
	}

	/*****************/
	/** DSI channel **/
	/*****************/
	res = platform_get_resource(pdev, IORESOURCE_MEM, 5);
	if (res == NULL)
		goto out_unmap7;

	dsiChanlRegAdd = (u8 *) ioremap(res->start, res->end - res->start + 1);
	if (dsiChanlRegAdd == NULL)
		goto out_unmap7;
	for (temp = 0; temp < NUM_DSI_CHANNEL; temp++)
		currentpar->mcde_dsi_channel_reg[temp] = (struct mcde_dsi_reg*)(dsiChanlRegAdd + (temp*U8500_MCDE_REGISTER_BANK_SIZE));

	/***************/
	/** DSI clock **/
	/***************/
	// TODO: Fix!
	currentpar->mcde_clkdsi = NULL;//(u32*)(dsiChanlRegAdd + (NOMADIK_MCDE_DSI_SIZE - 4));

	/**************/
	/** DSI link **/
	/**************/
	res = platform_get_resource(pdev, IORESOURCE_MEM, 6);
	if (res == NULL)
		goto out_unmap8;

	dsilinkAddr = (u8  *) ioremap(res->start, res->end - res->start + 1);
	if (dsilinkAddr == NULL)
		goto out_unmap8;
	for (temp = 0; temp < NUM_DSI_LINKS; temp++)
		currentpar->dsi_lnk_registers[temp] = (struct dsi_link_registers*)(dsilinkAddr + (temp*U8500_DSI_LINK_SIZE));

	/****************/
	/** MCDE clock **/
	/****************/
	res = platform_get_resource(pdev, IORESOURCE_MEM, 7);
	if (res == NULL)
		goto out_unmap9;

	currentpar->prcm_mcde_clk = (u32 *) ioremap(res->start, res->end - res->start + 1);
	if (currentpar->prcm_mcde_clk == NULL)
		goto out_unmap9;

  /****************/
  /** HDMI clock **/
  /****************/
	res = platform_get_resource(pdev, IORESOURCE_MEM, 8);
	if (res == NULL)
		goto out_unmap10;

	currentpar->prcm_hdmi_clk = (u32 *) ioremap(res->start, res->end - res->start + 1);
	if (currentpar->prcm_hdmi_clk == NULL)
		goto out_unmap10;
  /******************/
  /** TV-out clock **/
  /******************/
	res = platform_get_resource(pdev, IORESOURCE_MEM, 9);
	if (res == NULL)
		goto out_unmap11;

	currentpar->prcm_tv_clk = (u32 *) ioremap(res->start, res->end - res->start + 1);
	if (currentpar->prcm_tv_clk == NULL)
		goto out_unmap11;

	if (strcmp(channel_info->restype,"SDTV")==0)
		currentpar->tvout=1;
	else
		currentpar->tvout=0;

	printk(KERN_ERR "Registering interrupt handler\n");

  if (!registerInterruptHandler)
	{
		/** get your IRQ line */
		res = platform_get_resource(pdev,IORESOURCE_IRQ,0);
		if (res == NULL)
			goto out_unmap12;

		irqline = res->start;

		retVal = request_irq(res->start,u8500_mcde_interrupt_handler,IRQF_SHARED,"MCDE",dev);
		if(retVal) {
			dbgprintk(MCDE_ERROR_INFO, "Failed to request IRQ line\n");
			goto out_unmap12;
		}

		registerInterruptHandler = 1;
	}

	spin_lock_init(&currentpar->clcd_event.lock);
	init_waitqueue_head(&currentpar->clcd_event.wait);
	currentpar->clcd_event.event=0;

	info->dev = dev;
	info->fbops = &mcde_ops;
	info->flags = FBINFO_FLAG_DEFAULT;
	info->pseudo_palette = currentpar->cmap;

	info->fix = mcde_fix;
#ifdef CONFIG_FB_MCDE_MULTIBUFFER
	info->fix.ypanstep = 1;
#else
	info->fix.ypanstep   = channel_info->nopan  ? 0:1;
#endif
	info->fix.ywrapstep  = channel_info->nowrap ? 0:1;

	if (currentpar->chid == CHANNEL_B )
	{
	*(currentpar->prcm_mcde_clk) = 0x125; /** MCDE CLK = 160MHZ */
    *(currentpar->prcm_tv_clk) = 0x14E;     /** HDMI CLK = 76.8MHZ */

#ifdef  CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_VUIB_WVGA
	/**  clcd_clk */
    clcd_clk = (u32 *) ioremap(PRCM_LCDCLK_MGT_REG, 4);
	*clcd_clk=0x300;
	iounmap(clcd_clk);
#endif

  printk(KERN_ERR "Enable the channel specific GPIO\n");

	    //** enable the channel specific GPIO alternate functions */
		retVal = stm_gpio_altfuncenable(channel_info->gpio_alt_func);
		if (retVal)
		{
			dbgprintk(MCDE_ERROR_INFO, "Could not set MCDE GPIO alternate function correctly\n");
			/** re-valuate your request , bad pin request */
			retVal = -EADDRNOTAVAIL;
			goto out_irq;
		}


#ifndef CONFIG_U8500_SIM8500
    printk(KERN_ERR "4500 writings\n");

        data_4500=ab8500_read(0x2,0x200);
        data_4500|=0x4;
        ab8500_write(0x2,0x200,data_4500);

        data_4500=ab8500_read(0x2,0x200);
        data_4500|=0x2;
		ab8500_write(0x2,0x200,data_4500);

		data_4500=ab8500_read(0x2,0x200);
		data_4500|=0x4;
		ab8500_write(0x2,0x200,data_4500);

		data_4500=ab8500_read(0x2,0x20A);
		data_4500|=0x44;
		ab8500_write(0x2,0x20A,data_4500);

		data_4500=ab8500_read(0x2,0x20C);
		data_4500|=0x3;
		ab8500_write(0x2,0x20C,data_4500);

		data_4500=ab8500_read(0x3,0x380);
		data_4500|=0x6;
		ab8500_write(0x3,0x380,data_4500);

		ab8500_write(0x6,0x606,0x80);

		ab8500_write(0x6,0x600,0xA);
		ab8500_write(0x6,0x601,0x40);
		ab8500_write(0x6,0x608,0x10);
		ab8500_write(0x6,0x680,0x1);
		ab8500_write(0x6,0x681,0x0);

        /** enable TV pluguin detect */
		ab8500_write(AB8500_INTERRUPT,AB8500_IT_MASK1_REG,0xF9);
#if defined(CONFIG_4500_ED)
        ab8500_set_callback_handler(AB8500_TV_PLUG_DET,tv_detect_handler,NULL);
        ab8500_set_callback_handler(AB8500_TV_UNPLUG_DET,tv_removed_handler,NULL);
#endif
	ab8500_write(0x6,0x680,0x1D);

        mdelay(1000);
#endif
	}
	gpar[currentpar->chid] = currentpar;

	/** To update the per flow params */

  if (currentpar->chid == CHANNEL_C0 || currentpar->chid == CHANNEL_C1 ||  currentpar->chid == CHANNEL_B || currentpar->chid == CHANNEL_A)
  {
	  u32 errors_on_dsi;
	  u32 self_diagnostic_result;
	  bool retry;
	  int n_retry = 3;
	  u32 id1,id2,id3;

	if (currentpar->chid == CHANNEL_C0)
		printk(KERN_INFO "%s: Initializing DSI and display for C0\n",
			  __func__);
	  else if (currentpar->chid == CHANNEL_C1)
		printk(KERN_INFO "%s: Initializing DSI and display for C1\n",
			  __func__);
	  else if (currentpar->chid == CHANNEL_A)
		printk(KERN_INFO "%s: Initializing DSI and display for A\n",
			  __func__);
    else
		printk(KERN_INFO "%s: Initializing DSI and display for B\n",
			  __func__);

    mcde_dsi_set_params(info);
	  mcde_dsi_start(info);

	  printk(KERN_ERR "DSI started\n");

#ifndef PEPS_PLATFORM
	/** Make the screen up first */
	if (/*mcde_dsi_read_reg(info, 0x05, &errors_on_dsi) >= 0 ||*/
	    currentpar->chid == CHANNEL_C0)
	{
		do
		{
			mcde_dsi_write_reg(info, 0x01, 1);
			mdelay(150);

			mcde_dsi_start(info);

			/* Check that display is OK */
			/*mcde_dsi_read_reg(info, 0x05, &errors_on_dsi);
			mcde_dsi_read_reg(info, 0x0F, &self_diagnostic_result);

			mcde_dsi_read_reg(info, 0xDA, &id1);
			mcde_dsi_read_reg(info, 0xDB, &id2);
			mcde_dsi_read_reg(info, 0xDC, &id3);*/

			printk(KERN_INFO "%s: id1=%X, id2=%X, id3=%X\n",
			       __func__, id1, id2, id3);

			retry = errors_on_dsi != 0 ||
				(self_diagnostic_result != 0xC0 &&
				 self_diagnostic_result != 0x80 &&
				 self_diagnostic_result != 0x40);
			n_retry--;
			if (retry) {
				/* Oops! Pink display? */
				if (n_retry) {
					/* Display not initialized correctly,
					   try again */
					printk(
						KERN_WARNING
						"%s: Failed to initialize"
						" display, potential 'pink' "
						"display. "
					       " errors_on_dsi=%X, "
					       " self_diagnostic_result=%X, "
					       " retrying...\n", __func__,
					       errors_on_dsi,
						self_diagnostic_result);
				}
			}
			else
				printk(KERN_WARNING "%s: Display initialized OK, "
				       " errors_on_dsi=%X, "
				       " self_diagnostic_result=%X\n",
				       __func__,
				       errors_on_dsi, self_diagnostic_result);
		} while (n_retry && retry);
	}

	printk(KERN_ERR "Pink display success");
#endif
  }

      /* variable to find the num of display devices initalized */
  num_of_display_devices++;
  printk(KERN_ERR "Setting video mode\n");
       mcde_set_video_mode(info, currentpar->video_mode);

	if (currentpar->chid == CHANNEL_A || currentpar->chid == CHANNEL_B)
	{
		/** allocate memory for rotation*/
		rotationframesize = info->var.xres*16*8;  /** xres * MAX_READ_BURST_SIZE WORDS */
		vbuffaddr = (char __iomem *) dma_alloc_coherent(info->dev,(rotationframesize*2),&dma,GFP_KERNEL|GFP_DMA);
		if (!vbuffaddr)
		{
			dbgprintk(MCDE_ERROR_INFO, "Unable to allocate external source memory for new framebuffer\n");
			return -ENOMEM;
		}
		currentpar->rotationbuffaddr0.cpuaddr = (u32) vbuffaddr;
		currentpar->rotationbuffaddr0.dmaaddr = dma;
		currentpar->rotationbuffaddr0.bufflength = rotationframesize;

		currentpar->rotationbuffaddr1.cpuaddr = (u32) (vbuffaddr + rotationframesize);
		currentpar->rotationbuffaddr1.dmaaddr = (dma + (rotationframesize/4));
		currentpar->rotationbuffaddr1.bufflength = framesize;
	}
	/** update info->screen_base, info->fix.line_length to access the FB by kernel */
	info->fix.line_length =  get_line_length(info->var.xres, info->var.bits_per_pixel);
	info->screen_base = (u8 *)currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].cpuaddr;

  printk(KERN_ERR "Registering framebuffer\n");

  if (register_framebuffer(info) < 0)
		goto out_fbdealloc;

	/** register_framebuffer unsets the DMA mask, but we require it set for subsequent buffer allocations */
	info->dev->coherent_dma_mask = ~0x0;
	platform_set_drvdata(pdev, info);

	printk(KERN_ERR "Display initialization done, chid = %d\n", currentpar->chid);

	return 0;

/** error handling */
out_fbdealloc:
	stm_gpio_altfuncdisable(channel_info->gpio_alt_func);
out_irq:
	free_irq(irqline,NULL);
out_unmap12:
	iounmap(currentpar->prcm_tv_clk);
out_unmap11:
	iounmap(currentpar->prcm_hdmi_clk);
out_unmap10:
	iounmap(currentpar->prcm_mcde_clk);
out_unmap9:
	iounmap(currentpar->dsi_lnk_registers[0]);
out_unmap8:
	iounmap(currentpar->mcde_dsi_channel_reg[0]);
out_unmap7:
	if (currentpar->chid == CHANNEL_A || currentpar->chid == CHANNEL_B)
		iounmap(currentpar->ch_regbase2[currentpar->chid]);
	else if (currentpar->chid == CHANNEL_C0 || currentpar->chid == CHANNEL_C1)
	{
		iounmap(currentpar->ch_c_reg);
	}
out_unmap6:
  if (currentpar->chid == CHANNEL_C0)
    iounmap(currentpar->ch_regbase2[MCDE_CH_B]);
out_unmap5:
  if (currentpar->chid == CHANNEL_C0)
    iounmap(currentpar->ch_regbase2[MCDE_CH_A]);
out_unmap4:
	iounmap(currentpar->ch_regbase1[0]);
out_unmap3:
	iounmap(currentpar->ovl_regbase[0]);
out_unmap2:
	iounmap(currentpar->extsrc_regbase[0]);
out_unmap1:
	iounmap(currentpar->regbase);
out_res:
	platform_device_put(pdev);

out_fballoc:
	framebuffer_release(info);
	return retVal;

}

void  mcde_test(struct fb_info *info)
{
	struct mcdefb_info *currentpar=
		(struct mcdefb_info *) info->par;

	mcde_fb_lock(info, __func__);

	gpio_set_value(268, 0);
	gpio_set_value(269, 0);

	mdelay(10);

	gpio_set_value(268, 1);
	gpio_set_value(269, 1);

	mdelay(10); /** let the high value settle  */

	mdelay(200); /* Must wait at least 120 ms after reset */

	if (currentpar->chid == CHANNEL_C0 || currentpar->chid == CHANNEL_C1 || currentpar->chid == CHANNEL_B)
	{
		u32 errors_on_dsi;
		u32 self_diagnostic_result;
		bool retry;
		int n_retry = 3;
		u32 id1,id2,id3;

		mcde_dsi_set_params(info);

#ifndef PEPS_PLATFORM
		do
		{
			/** Make the screen up first */

			currentpar->dsi_lnk_registers[DSI_LINK2]->
				mctl_main_en=0x0;
			mcde_dsi_start(info);

			/* Check that display is OK */
			mcde_dsi_read_reg(info, 0x05, &errors_on_dsi);
			mcde_dsi_read_reg(info, 0x0F, &self_diagnostic_result);

			mcde_dsi_read_reg(info, 0xDA, &id1);
			mcde_dsi_read_reg(info, 0xDB, &id2);
			mcde_dsi_read_reg(info, 0xDC, &id3);

			printk(KERN_INFO "%s: id1=%X, id2=%X, id3=%X\n",
			       __func__, id1, id2, id3);

			retry = errors_on_dsi != 0 ||
				(self_diagnostic_result != 0xC0 &&
				 self_diagnostic_result != 0x80 &&
				 self_diagnostic_result != 0x40);
			n_retry--;
			if (retry) {
				/* Oops! Pink display? */
				if (n_retry) {
					/* Display not initialized correctly,
					   try again */
					printk(KERN_WARNING "%s: Failed to initialize"
					       " display, potential 'pink' display. "
					       " errors_on_dsi=%X, "
					       " self_diagnostic_result=%X, "
					       " retrying...\n", __func__,
					       errors_on_dsi, self_diagnostic_result);
					mcde_dsi_write_reg(info, 0x01, 1);
				}
			}
			else
				printk(KERN_WARNING "%s: Display initialized OK, "
				       " errors_on_dsi=%X, "
				       " self_diagnostic_result=%X\n",
				       __func__,
				       errors_on_dsi, self_diagnostic_result);
		} while (n_retry && retry);
#endif
	}

	/** initialize the hardware as per the configuration */
	mcde_set_par_internal(info);

	mcde_fb_unlock(info, __func__);
}

/**
 * mcde_remove() - This routine de-initializes and de-register the FB device.
 * @pdev: platform device.
 *
 * This routine removes the frame buffer device .This will be invoked for every
 * frame buffer device de-initialization.
 *
 */
int mcde_remove(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);
	struct mcdefb_info *currentpar;
	struct device *dev = &pdev->dev;
	struct mcde_channel_data *channel_info;
	u8 temp;

	/** Aquire module mutex */
//	down_interruptible(&mcde_module_mutex);

	if (info) {
		currentpar = (struct mcdefb_info *) info->par;
		channel_info = (struct mcde_channel_data *) dev->platform_data;
		if (registerInterruptHandler)
		{
			struct resource *res = platform_get_resource(pdev,IORESOURCE_IRQ,0);
			free_irq(res->start,NULL);
			registerInterruptHandler = 0;
		}

		for (temp=0;temp<MCDE_MAX_FRAMEBUFF;temp++)
		{
			if (mcde_ovl_bmp & (1 << temp))
				mcde_extsrc_ovl_remove(info,temp);
		}
		for (temp=MCDE_MAX_FRAMEBUFF;temp<2*MCDE_MAX_FRAMEBUFF;temp++)
		{
			if (mcde_ovl_bmp & (1 << temp))
				mcde_dealloc_source_buffer(info, temp, FALSE);
		}

		platform_set_drvdata(pdev,NULL);
		mcdefb_disable(info);
		unregister_framebuffer(info);
		stm_gpio_altfuncdisable(channel_info->gpio_alt_func);
		iounmap(currentpar->regbase);
		iounmap(currentpar->extsrc_regbase[0]);
		iounmap(currentpar->ovl_regbase[0]);
		iounmap(currentpar->ch_regbase1[0]);
		if (currentpar->chid == CHANNEL_A || currentpar->chid == CHANNEL_B)
			iounmap(currentpar->ch_regbase2[currentpar->chid]);
		else if (currentpar->chid == CHANNEL_C0 || currentpar->chid == CHANNEL_C1)
		{
			iounmap(currentpar->ch_c_reg);
		}
		iounmap(currentpar->dsi_lnk_registers[0]);
		iounmap(currentpar->mcde_dsi_channel_reg[0]);
		framebuffer_release(info);
	}
	/** Release module mutex */
//	up(&mcde_module_mutex);

	return 0;
}

//Temporary solution
static void mcde_environment_setup(void)
{
#define MCDE_SSP_ENABLE_ALTA		0x120
#define MCDE_SSP_CR0				0x00
#define MCDE_SSP_CR1				0x04
#define MCDE_SSP_DR					0x08
#define MCDE_SSP_CPSR				0x10
#define MCDE_PRCM_MMIP_LS_CLAMP_SET	0x420
#define MCDE_PRCM_APE_RESETN_CLR	0x1E8
#define MCDE_PRCM_EPOD_C_SET		0x410
#define MCDE_PRCM_SRAM_LS_SLEEP		0x304
#define MCDE_PRCM_MMIP_LS_CLAMP_CLR	0x424
#define MCDE_PRCM_POWER_STATE_SET	0x254
#define MCDE_PRCM_LCDCLK_MGT		0x044
#define MCDE_PRCM_MCDECLK_MGT		0x064
#define MCDE_PRCM_HDMICLK_MGT		0x058
#define MCDE_PRCM_TVCLK_MGT			0x07c
#define MCDE_PRCM_PLLDSI_FREQ		0x500
#define MCDE_PRCM_PLLDSI_ENABLE		0x504
#define MCDE_PRCM_APE_RESETN_SET	0x1E4
#define MCDE_PRCM_DSI_PLLOUT_SEL	0x530
#define MCDE_PRCM_DSITVCLK_DIV		0x52C
#define MCDE_PRCM_DSI_SW_RESET		0x324

	typedef struct
	{
		u32	base;
		u32	value;
		u32 post_process;
	}mcde_reg_set;

	u32 *reg;
	u32 index;

	mcde_reg_set mcde_reg[] = {
			//PRCMU SETUP FOR MCDE DSI
			{U8500_PRCMU_BASE + MCDE_PRCM_MMIP_LS_CLAMP_SET,	0x00400C00,		1},//Clamp the DSS output
			{U8500_PRCMU_BASE + MCDE_PRCM_APE_RESETN_CLR,		0x0000000C,		1},//Put the DSS MEM & logic in reset
			{U8500_PRCMU_BASE + MCDE_PRCM_EPOD_C_SET,			0x00200000,		1},//EPOD DSS MEM Supply
			{U8500_PRCMU_BASE + MCDE_PRCM_EPOD_C_SET,			0x00100000,		1},//EPOD DSS logic Supply
			{U8500_PRCMU_BASE + MCDE_PRCM_SRAM_LS_SLEEP,		0x30000155,		1},//Sleep exit for DSS
			{U8500_PRCMU_BASE + MCDE_PRCM_MMIP_LS_CLAMP_CLR,	0x00400C00,		1},//Clear the DSS o/p Clamp
			{U8500_PRCMU_BASE + MCDE_PRCM_POWER_STATE_SET,		0x00008000,		1},//PRCM_POWER_STATE_SET set dsi-csi on
			{U8500_PRCMU_BASE + MCDE_PRCM_LCDCLK_MGT,			0x00000148,		1},//enable LCD clk(@ 48MHZ)
			{U8500_PRCMU_BASE + MCDE_PRCM_MCDECLK_MGT,			0x00000125,		1},//enable MCDE Clk (@ 160 MHz)
			{U8500_PRCMU_BASE + MCDE_PRCM_HDMICLK_MGT,			0x00000145,		1},//enable HDMI Clk (@ 76.8 MHz)
			{U8500_PRCMU_BASE + MCDE_PRCM_TVCLK_MGT,			0x00000148,		1},//enable TV Clk (@ 48 MHz)

			//Program the dsi PLL
            {U8500_PRCMU_BASE + MCDE_PRCM_PLLDSI_FREQ,			0x00040120,		0},//set PLL config
            {U8500_PRCMU_BASE + MCDE_PRCM_PLLDSI_ENABLE,		0x00000001,		1},//PRCM_PLLDSI_ENABLE
			{U8500_PRCMU_BASE + MCDE_PRCM_APE_RESETN_SET,		0x0000400C,		1},//Release DSS MEM & logic and DSI PLL reset
			{U8500_PRCMU_BASE + MCDE_PRCM_DSI_PLLOUT_SEL,		0x00000202,		1},//PLL out selectect for dsi012
			{U8500_PRCMU_BASE + MCDE_PRCM_DSITVCLK_DIV,			0x07030808,		0},//PRCM_DSITVCLK_DIV and escape clk enable
			{U8500_PRCMU_BASE + MCDE_PRCM_DSI_SW_RESET,			0x00000007,		1},//PRCM_DSI_SW_RESET
	};

	for (index = 0; index < (sizeof(mcde_reg)/sizeof(mcde_reg_set)); index++)
	{
		reg = (u32 *) ioremap(mcde_reg[index].base, 0x4);
		*reg = mcde_reg[index].value;

//		printk(KERN_ERR "0x%08x 0x%08x\n", mcde_reg[index].base, mcde_reg[index].value);

		switch (mcde_reg[index].post_process) {
		case 0:
		default:
			/* do nothing */
			break;
		case 1:
			mdelay(10);
			break;
		}

		iounmap(reg);
	}
}


#ifdef CONFIG_PM
/**
 * u8500_mcde_suspend() - This routine puts the FB device in to sustend state.
 * @pdev: platform device.
 *
 * This routine stores the current state of the frame buffer device and puts in to suspend state.
 *
 */
int u8500_mcde_suspend(struct platform_device *pdev, pm_message_t state)
{
	dbgprintk(MCDE_DEBUG_INFO, "mop_mcde_suspend: called...\n");
	return 0;
}

/**
 * u8500_mcde_resume() - This routine resumes the FB device from sustend state.
 * @pdev: platform device.
 *
 * This routine restore back the current state of the frame buffer device resumes.
 *
 */
int u8500_mcde_resume(struct platform_device *pdev)
{
	dbgprintk(MCDE_DEBUG_INFO, "mop_mcde_resume: called...\n");
	return 0;
}

#else

#define u8500_mcde_suspend NULL
#define u8500_mcde_resume NULL

#endif

struct platform_driver mcde_driver = {
	.probe	= mcde_probe,
	.remove = mcde_remove,
	.driver = {
		.name	= "U8500-MCDE",
	},
        /** TODO implement power mgmt functions */
        .suspend = u8500_mcde_suspend,
        .resume = u8500_mcde_resume,
};
/**
 * mcde_init() - This routine inserts the driver.
 * @pdev: platform device.
 *
 * This module is built as a static module, platform driver init will be called mcde_init to load mcde module.
 *
 */
static int __init mcde_init(void)
{
	return platform_driver_register(&mcde_driver);
}
module_init(mcde_init);

/**
 * mcde_exit() - This routine exits the driver.
 * @pdev: platform device.
 *
 */
static void __exit mcde_exit(void)
{
	platform_driver_unregister(&mcde_driver);
	return;
}
module_exit(mcde_exit);

#ifdef _cplusplus
}
#endif /* _cplusplus */

#else	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */
/* HW ED */

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

/** Linux include files:charachter driver and memory functions  */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/init.h>

#include <asm/dma.h>
#include <asm/uaccess.h>
#include <mach/mcde_common.h>
#include <mach/irqs.h>
#include <mach/ab8500.h>

void  mcde_test(struct fb_info *info);
DECLARE_MUTEX(mcde_module_mutex);

bool mcde_test_enable_lock = true;
bool mcde_test_enable_lock_prints = false;

void mcde_fb_lock(struct fb_info *info, const char *caller)
{
	struct mcdefb_info *currentpar = info->par;

	if (mcde_test_enable_lock_prints)
		printk(KERN_INFO "%s(%p) from %s\n", __func__, info, caller);
	if (!mcde_test_enable_lock)
		return;
	down(&currentpar->fb_sem);

	/* Wait for imsc to be zero */
	/* We don't want to interfere with a refresh interrupt */
#ifdef CONFIG_FB_MCDE_MULTIBUFFER
	if ((currentpar->regbase->mcde_imsc & 0x40) != 0) {
		int wait = 5;

		printk(KERN_INFO "%s(%p), mcde_imsc != 0, is %X. "
		       "Waiting...\n", __func__, info,
		       currentpar->regbase->mcde_imsc);
		while (wait-- && currentpar->regbase->mcde_imsc != 0)
			mdelay(100);
		printk(KERN_INFO "%s(%p), mcde_imsc = 0, is %X\n",
		       __func__, info,
		       currentpar->regbase->mcde_imsc);
	}
#endif
}

void mcde_fb_unlock(struct fb_info *info, const char *caller)
{
	struct mcdefb_info *currentpar = info->par;
	if (mcde_test_enable_lock_prints)
		printk(KERN_INFO "%s(%p) from %s\n", __func__, info, caller);
	if (!mcde_test_enable_lock)
		return;
	up(&currentpar->fb_sem);
}

int mcde_debug = MCDE_DEFAULT_LOG_LEVEL;

char isVideoModeChanged = 0;
unsigned int mcde_4500_plugstatus=0;

/** platform specific dectetion */
extern int platform_id;
bool isHdmiEnabled = FALSE;
extern u32 mcde_ovl_bmp;

struct fb_info *g_info;
/** video modes database */
struct fb_videomode mcde_modedb[] __initdata = {
        {
                /** 640x350 @ 85Hz ~ VMODE_640_350_85_P*/
                NULL, 85, 640, 350, KHZ2PICOS(31500),
                96, 32, 60, 32, 64, 3,
                FB_SYNC_HOR_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 640x400 @ 85Hz ~ VMODE_640_400_85_P */
                NULL, 85, 640, 400, KHZ2PICOS(31500),
                96, 32, 41, 1, 64, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 720x400 @ 85Hz ~ VMODE_720_400_85_P*/
                NULL, 85, 720, 400, KHZ2PICOS(35500),
                108, 36, 42, 1, 72, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 640x480 @ 60Hz  ~ VMODE_640_480_60_P*/
                "VGA", 60, 640, 480, KHZ2PICOS(25175),
                0x21,0x40,0x07,0x24,0x40,0x19,
                FB_SYNC_HOR_HIGH_ACT|FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 640x480 @ 60Hz  VMODE_640_480_CRT_60_P */
                "CRT", 60, 640, 480, KHZ2PICOS(25175),
                0x29,0x09,0x19,0x02,0x61,0x02,
                FB_SYNC_HOR_HIGH_ACT|FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },{
                /** 240x320 @ 60Hz  ~ VMODE_240_320_60_P*/
                "QVGA_Portrait", 60, 240, 320, KHZ2PICOS(25175),
                0x13,0x2f,0x04,0x0f,0x13,0x04,
                FB_SYNC_HOR_HIGH_ACT|FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },{
                /** 320x240 @ 60Hz  ~ VMODE_320_240_60_P */
                "QVGA_Landscape", 60, 320, 240, KHZ2PICOS(25175),
                0x13,0x2f,0x04,0x0f,0x13,0x04,
                FB_SYNC_HOR_HIGH_ACT|FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },{
                /** 712x568 @ 72Hz  ~ VMODE_712_568_60_P */
                "SDTV", 72, 712, 568, KHZ2PICOS(31500),
                140, 2, 2, 2, 2, 24,
                0, FB_VMODE_INTERLACED
        }, {
                /** 640x480 @ 75Hz  ~ VMODE_640_480_75_P */
                NULL, 75, 640, 480, KHZ2PICOS(31500),
                120, 16, 16, 1, 64, 3,
                0, FB_VMODE_NONINTERLACED
        }, {
                /** 640x480 @ 85Hz ~ VMODE_640_480_85_P */
                NULL, 85, 640, 480, KHZ2PICOS(36000),
                80, 56, 25, 1, 56, 3,
                0, FB_VMODE_NONINTERLACED
        }, {
                /* * 480x864 @ 60Hz  ~ VMODE_480_864_60_P*/
                "WVGA_Portrait", 60, 480, 864, KHZ2PICOS(40000),
                88, 40, 23, 1, 128, 4,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		    /** 864x480 @ 60Hz  ~ VMODE_864_480_60_P*/
                "WVGA", 60, 864, 480, KHZ2PICOS(40000),
                88, 40, 23, 1, 128, 4,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		    /** 800x600 @ 56Hz  ~VMODE_800_600_56_P*/
                "VUIB WVGA", 56, 800, 600, KHZ2PICOS(36000),
                128, 24, 22, 1, 72, 2,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 800x600 @ 60Hz ~ VMODE_800_600_60_P*/
                NULL, 60, 800, 600, KHZ2PICOS(40000),
                88, 40, 23, 1, 128, 4,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		        /** 800x600 @ 72Hz  ~ VMODE_800_600_72_P*/
                NULL, 72, 800, 600, KHZ2PICOS(50000),
                64, 56, 23, 37, 120, 6,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 800x600 @ 75Hz ~ VMODE_800_600_75_P*/
                NULL, 75, 800, 600, KHZ2PICOS(49500),
                160, 16, 21, 1, 80, 3,
		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 800x600 @ 85Hz ~ VMODE_800_600_85_P*/
                NULL, 85, 800, 600, KHZ2PICOS(56250),
                152, 32, 27, 1, 64, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		        /** 1024x768 @ 60Hz ~ VMODE_1024_768_60_P */
                NULL, 60, 1024, 768, KHZ2PICOS(65000),
                160, 24, 29, 3, 136, 6,
                0, FB_VMODE_NONINTERLACED
        }, {
                /** 1024x768 @ 70Hz ~ VMODE_1024_768_70_P */
                NULL, 70, 1024, 768, KHZ2PICOS(75000),
                144, 24, 29, 3, 136, 6,
                0, FB_VMODE_NONINTERLACED
        }, {
                /** 1024x768 @ 75Hz ~ VMODE_1024_768_75_P */
                NULL, 75, 1024, 768, KHZ2PICOS(78750),
                176, 16, 28, 1, 96, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1024x768 @ 85Hz ~ VMODE_1024_768_85_P */
                NULL, 85, 1024, 768, KHZ2PICOS(94500),
                208, 48, 36, 1, 96, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1152x864 @ 75Hz ~ VMODE_1152_864_75_P*/
                NULL, 75, 1152, 864, KHZ2PICOS(108000),
                256, 64, 32, 1, 128, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1280x960 @ 60Hz ~ VMODE_1280_960_60_P*/
                NULL, 60, 1280, 960, KHZ2PICOS(108000),
                312, 96, 36, 1, 112, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		    /** 1280x960 @ 85Hz ~ VMODE_1280_960_85_P*/
                NULL, 85, 1280, 960, KHZ2PICOS(148500),
                224, 64, 47, 1, 160, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		        /** 1280x1024 @ 60Hz ~ VMODE_1280_1024_60_P*/
                NULL, 60, 1280, 1024, KHZ2PICOS(108000),
                248, 48, 38, 1, 112, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1280x1024 @ 75Hz ~ VMODE_1280_1024_75_P*/
                NULL, 75, 1280, 1024, KHZ2PICOS(135000),
                248, 16, 38, 1, 144, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1280x1024 @ 85Hz ~ VMODE_1280_1024_8_P*/
                NULL, 85, 1280, 1024, KHZ2PICOS(157500),
                224, 64, 44, 1, 160, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1600x1200 @ 60Hz VMODE_1600_1200_60_P*/
                NULL, 60, 1600, 1200, KHZ2PICOS(162000),
                304, 64, 46, 1, 192, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1600x1200 @ 65Hz VMODE_1600_1200_65_P*/
                NULL, 65, 1600, 1200, KHZ2PICOS(175500),
                304, 64, 46, 1, 192, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		        /** 1600x1200 @ 70Hz VMODE_1600_1200_70_P*/
                NULL, 70, 1600, 1200, KHZ2PICOS(189000),
                304, 64, 46, 1, 192, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1600x1200 @ 75HzVMODE_1600_1200_75_P */
                NULL, 75, 1600, 1200, KHZ2PICOS(202500),
                304, 64, 46, 1, 192, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1600x1200 @ 85Hz VMODE_1600_1200_85_P*/
                NULL, 85, 1600, 1200, KHZ2PICOS(229500),
                304, 64, 46, 1, 192, 3,
                FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1792x1344 @ 60Hz VMODE_1792_1344_60_P */
                NULL, 60, 1792, 1344, KHZ2PICOS(204750),
                328, 128, 46, 1, 200, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1792x1344 @ 75Hz VMODE_1792_1344_75_P */
                NULL, 75, 1792, 1344, KHZ2PICOS(261000),
                352, 96, 69, 1, 216, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
		        /** 1856x1392 @ 60Hz VMODE_1856_1392_60_P */
                NULL, 60, 1856, 1392, KHZ2PICOS(218250),
                352, 96, 43, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1856x1392 @ 75Hz VMODE_1856_1392_75_P */
                NULL, 75, 1856, 1392, KHZ2PICOS(288000),
                352, 128, 104, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1920x1440 @ 60Hz VMODE_1920_1440_60_P */
                NULL, 60, 1920, 1440, KHZ2PICOS(234000),
                344, 128, 56, 1, 208, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        }, {
                /** 1920x1440 @ 75Hz VMODE_1920_1440_75_P */
                NULL, 75, 1920, 1440, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },
        {
                /** 720x480 @ 60Hz  VMODE_720_480_60_P */
                NULL, 60, 720, 480, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },
        {
                /** 720x480 @ 60Hz VMODE_720_480_60_I */
                NULL, 60, 720, 480, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_INTERLACED
        },
        {
                /** 720x576 @ 50Hz VMODE_720_576_50_P */
                NULL, 50, 720, 576, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },
        {
                /** 720x576 @ 50Hz VMODE_720_576_50_I */
                NULL, 50, 720, 576, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_INTERLACED
        },
        {
                /** 1280x720 @ 50Hz VMODE_1280_720_50_P */
                NULL, 50, 1280, 720, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },
        {
                /** 1280x720 @ 60Hz VMODE_1280_720_50_I */
                NULL, 60, 1280, 720, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },
        {
                /** 1920x1080 @ 50Hz  VMODE_1920_1080_50_I */
                NULL, 50, 1920, 1080, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_INTERLACED
        },
        {
                /** 1920x1080 @ 60Hz VMODE_1920_1080_60_I */
                NULL, 60, 1920, 1080, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_INTERLACED
        },
        {
                /** 1920x1080 @ 60Hz VMODE_1920_1080_60_P */
                NULL, 60, 1920, 1080, KHZ2PICOS(297000),
                352, 144, 56, 1, 224, 3,
                FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED
        },
};

/**  TODO check these default values for QVGA display */
static struct fb_var_screeninfo mcde_default_var __initdata = {
	/** 240x320, 16bpp @ 60 Hz */
	.xres				= 240,
	.yres				= 320,
	.xres_virtual			= 240,
	.yres_virtual			= 320,
	.bits_per_pixel			= MCDE_U8500_PANEL_16BPP,
	.red				= { 11, 5, 0 },
	.green				= {  5, 6, 0 },
	.blue				= {  0, 5, 0 },
	.activate			= FB_ACTIVATE_NOW,
	.height				= -1,
	.width				= -1,
	.pixclock			= KHZ2PICOS(25175),
	.left_margin			= 0x13,
	.right_margin			= 0x2F,
	.upper_margin			= 0x4,
	.lower_margin			= 0xF,
	.hsync_len			= 0x13,
	.vsync_len			= 0x4,
	.vmode				= FB_VMODE_NONINTERLACED,

};

static struct fb_fix_screeninfo mcde_fix __initdata = {
	.id =		"ST MCDE",
	.type =		FB_TYPE_PACKED_PIXELS,
	.visual =	FB_VISUAL_TRUECOLOR,
	.accel =	FB_ACCEL_NONE,
};
/** global data */
u8 registerInterruptHandler = 0;


/** To be removed I2C */
u8 i2c_init_done=0;

u32 num_of_display_devices = 0;
u32 PrimaryDisplayFlow;
/** current fb_info */
struct mcdefb_info *gpar[NUM_MCDE_FLOWS];

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MCDE-DSI driver for Nomadik 8500 Platform");

static void mcde_conf_video_mode(struct fb_info *);
static int __init mcde_probe(struct platform_device *);
static int mcde_remove(struct platform_device *);
static int mcde_check_var(struct fb_var_screeninfo *, struct fb_info *);
static int mcde_set_par(struct fb_info *info);
static int mcde_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info *info);
static int mcde_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
static int mcde_ioctl(struct fb_info *info,unsigned int cmd, unsigned long arg);
static int mcde_mmap(struct fb_info *info, struct vm_area_struct *vma);
static int mcde_blank(int blank_mode, struct fb_info *info);
static int mcde_set_video_mode(struct fb_info *info, mcde_video_mode videoMode);
void find_restype_from_video_mode(struct fb_info *info, mcde_video_mode videoMode);
void mcde_change_video_mode(struct fb_info *info);


void u8500_mcde_tasklet_1(unsigned long);
void u8500_mcde_tasklet_2(unsigned long);
void u8500_mcde_tasklet_3(unsigned long);
void u8500_mcde_tasklet_4(unsigned long);

DECLARE_TASKLET(mcde_tasklet_1, u8500_mcde_tasklet_1, 0);
DECLARE_TASKLET(mcde_tasklet_2, u8500_mcde_tasklet_2, 0);
DECLARE_TASKLET(mcde_tasklet_3, u8500_mcde_tasklet_3, 0);


int interrupt_counter=0;
int csm_running_counter=0;

#if 0
void u8500_mcde_tasklet_4(unsigned long tasklet_data)
{
int i = 0;
#if 1
#ifndef CONFIG_FB_MCDE_MULTIBUFFER
	if (gpar[0]->regbase->mcde_mis & 0x4 || gpar[0]->regbase->mcde_ris & 0x4)
		  /** vcomp */
	   gpar[0]->regbase->mcde_ris &= 0x4;
	   while(gpar[0]->dsi_lnk_registers[DSI_LINK2]->cmd_mode_sts & 0x20);

#if 0

//printk("u8500_mcde_tasklet_4 FlowA...\n");
	   /**  send tearing command */

	   gpar[0]->dsi_lnk_registers[DSI_LINK2]->direct_cmd_send=0x1;

        /**  wait till tearing recieved */

	    while (gpar[0]->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts & 0x80!=0)
	    {
            if(gpar[0]->dsi_lnk_registers[DSI_LINK2]->cmd_mode_sts&0x22)
            {

                 break;
            }
	    }

	    /** clear status flag */
	   gpar[0]->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts_clr=0xffffffff;
#endif
#endif


#ifdef CONFIG_FB_MCDE_MULTIBUFFER
	/** send DSI command for RAMWR */
	gpar[0]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_main_settings=0x43908;
	gpar[0]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_wrdat0=0x2c;
	gpar[0]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_send=0x1;

	while(gpar[0]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_sts & 0x1);


           gpar[0]->extsrc_regbase[0]->mcde_extsrc_a0 = gpar[0]->clcd_event.base;
	   //Mask the interrupt
	   gpar[0]->regbase->mcde_imsc = 0x0;
	   gpar[0]->clcd_event.event=1;

	   /** send software sync */
	   gpar[0]->ch_regbase1[0]->mcde_chsyn_sw=0x1;
	   wake_up(&gpar[0]->clcd_event.wait);
#else
//gpar[0]->regbase->mcde_ris &= 0x40;

	for (i = 0 ; i< 480; i++)
    gpar[0]->ch_regbase1[0]->mcde_chsyn_sw=0x1;
    //gpar[0]->regbase->mcde_imsc |= 0x40;
    //gpar[0]->regbase->mcde_ris &= 0x40;
   // gpar[0]->regbase->mcde_imsc |= 0x40;
#endif
#endif

}
#endif


void u8500_mcde_tasklet_4(unsigned long tasklet_data)
{
	//int i = 0;
	//for (i = 0 ; i< 1; i++)
	//{
	if (gpar[0]->regbase->mcde_mis & 0x4 || gpar[0]->regbase->mcde_ris & 0x4)
		  /** vcomp */
	   gpar[0]->regbase->mcde_ris &= 0x4;
	while(gpar[0]->dsi_lnk_registers[DSI_LINK2]->cmd_mode_sts & 0x20);

	gpar[0]->ch_regbase1[0]->mcde_chsyn_sw=0x1;
	//}
}

EXPORT_SYMBOL(u8500_mcde_tasklet_4);
void u8500_mcde_tasklet_1(unsigned long tasklet_data)
{

#ifndef CONFIG_FB_MCDE_MULTIBUFFER
	   while(gpar[2]->dsi_lnk_registers[DSI_LINK0]->cmd_mode_sts & 0x20);

	   /**  send tearing command */

	   gpar[2]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_send=0x1;

        /**  wait till tearing recieved */

	    while ((gpar[2]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_sts & 0x80)!=0)
	    {
            if(gpar[2]->dsi_lnk_registers[DSI_LINK0]->cmd_mode_sts&0x22)
            {

                 break;
            }
	    }

	    /** clear status flag */
	   gpar[2]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_sts_clr=0xffffffff;

#endif


#ifdef CONFIG_FB_MCDE_MULTIBUFFER
	/** send DSI command for RAMWR */
	gpar[2]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_main_settings=0x43908;
	gpar[2]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_wrdat0=0x2c;
	gpar[2]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_send=0x1;

	while(gpar[2]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_sts & 0x1);


           gpar[2]->extsrc_regbase[0]->mcde_extsrc_a0 = gpar[2]->clcd_event.base;
	   //Mask the interrupt
	   gpar[2]->regbase->mcde_imsc = 0x0;
	   gpar[2]->clcd_event.event=1;

	   /** send software sync */
	   gpar[2]->ch_regbase1[MCDE_CH_A]->mcde_chsyn_sw=0x1;
	   wake_up(&gpar[2]->clcd_event.wait);
#else
    gpar[2]->ch_regbase1[MCDE_CH_A]->mcde_chsyn_sw=0x1;
#endif


}


void u8500_mcde_tasklet_2(unsigned long tasklet_data)
{

#ifndef CONFIG_FB_MCDE_MULTIBUFFER
	   while(gpar[3]->dsi_lnk_registers[DSI_LINK1]->cmd_mode_sts & 0x20);


	    /**  wait till tearing recieved */


	   gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_send=0x1;

       /**  wait till tearing recieved */

	    while ((gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_sts & 0x80)!=0)
	    {
            if(gpar[3]->dsi_lnk_registers[DSI_LINK1]->cmd_mode_sts&0x22)
            {

                 break;
            }
	    }

       /** clear status flag */
       gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_sts_clr=0xffffffff;
#endif


#ifdef CONFIG_FB_MCDE_MULTIBUFFER
	/** send DSI command for RAMWR */
	gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_main_settings=0x43908;
	gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_wrdat0=0x2c;
	gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_send=0x1;

      while(gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_sts & 0x1);

       gpar[3]->extsrc_regbase[1]->mcde_extsrc_a0 = gpar[3]->clcd_event.base;
	   //Mask the interrupt
	   gpar[3]->regbase->mcde_imsc = 0x0;
	   gpar[3]->clcd_event.event=1;

	   /** send software sync */
	   gpar[3]->ch_regbase1[3]->mcde_chsyn_sw=0x1;
	   wake_up(&gpar[3]->clcd_event.wait);
#else
       gpar[3]->ch_regbase1[3]->mcde_chsyn_sw=0x1;
#endif

}



void u8500_mcde_tasklet_3(unsigned long tasklet_data)
{


#ifdef  CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_VUIB_WVGA

#ifdef CONFIG_FB_MCDE_MULTIBUFFER
           gpar[1]->extsrc_regbase[2]->mcde_extsrc_a0 = gpar[1]->clcd_event.base;
	   //Mask the interrupt
	   gpar[1]->regbase->mcde_imsc = 0x0;
	   gpar[1]->clcd_event.event=1;
	   wake_up(&gpar[1]->clcd_event.wait);
#endif

#else

       /** do nothing */

#endif


}


static irqreturn_t u8500_mcde_interrupt_handler(int irq,void *dev_id)
{
/** for dual display both the channels need to be controlled */

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
	if (gpar[2]->regbase->mcde_mis & 0xC0)
	{

	   gpar[2]->regbase->mcde_ris &= 0xC0;

	   while(gpar[2]->dsi_lnk_registers[DSI_LINK0]->cmd_mode_sts & 0x20);
	   while(gpar[3]->dsi_lnk_registers[DSI_LINK1]->cmd_mode_sts & 0x20);


	   while ((gpar[2]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_sts & 0x80)!=0)
		   {
		               if(gpar[2]->dsi_lnk_registers[DSI_LINK0]->cmd_mode_sts&0x22)
		               {

		                    break;
		               }
	       }

			while ((gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_sts & 0x80)!=0)
			{
				if(gpar[3]->dsi_lnk_registers[DSI_LINK1]->cmd_mode_sts&0x22)
				{

					 break;
				}
			}

			/** clear status flag */
	        gpar[2]->dsi_lnk_registers[DSI_LINK0]->direct_cmd_sts_clr=0xffffffff;
	        /** clear status flag */
            gpar[3]->dsi_lnk_registers[DSI_LINK1]->direct_cmd_sts_clr=0xffffffff;

            gpar[2]->ch_regbase1[2]->mcde_chsyn_sw=0x1;
            gpar[3]->ch_regbase1[3]->mcde_chsyn_sw=0x1;
	}
#endif

#else
{
		/** check for null */

		if(gpar[2]!=0)

		{
		   /** C0 */
		   if (gpar[2]->regbase->mcde_mis & 0x10)
		   {
			  /** vsync */
			   gpar[2]->regbase->mcde_ris &= 0x10;

		   }

		   if (gpar[2]->regbase->mcde_mis & 0x4)
			{
			  /** vcomp */
			   gpar[2]->regbase->mcde_ris &= 0x4;

			   //tasklet_schedule(&mcde_tasklet_1);
			   u8500_mcde_tasklet_1(0);

			}
		}

		/** check for null */

		if(gpar[3]!=0)

		{

			/**  C1 */

			if (gpar[3]->regbase->mcde_mis & 0x20)
			{
			  /** vsync */
			   gpar[3]->regbase->mcde_ris &= 0x20;
			}

			if (gpar[3]->regbase->mcde_mis & 0x80)
			{
			  /** vcomp */
			   gpar[3]->regbase->mcde_ris &= 0x80;


			   /** tasklet  */
			   //tasklet_schedule(&mcde_tasklet_2);
			   u8500_mcde_tasklet_2(0);

			}

		}
}
#endif

/** check for null */

if(gpar[1]!=0)

{

	/**  B */

	if (gpar[1]->regbase->mcde_mis & 0x8)
	{
	  /** vcomp */
	   //gpar[1]->regbase->mcde_ris &= 0x8;

	   /** tasklet  */
	   //tasklet_schedule(&mcde_tasklet_3);
	    u8500_mcde_tasklet_3(0);
	}
}

    interrupt_counter++;
	return IRQ_HANDLED;
}



static int mcde_ioctl(struct fb_info *info,
		unsigned int cmd, unsigned long arg)
{
	struct mcdefb_info *currentpar = info->par;
	struct mcde_overlay_create extsrc_ovl;
	struct mcde_sourcebuffer_alloc source_buff;
	struct mcde_ext_conf ext_src_config;
	struct mcde_conf_overlay overlay_config;
	struct mcde_ch_conf ch_config;
	struct mcde_chnl_lcd_ctrl chnl_lcd_ctrl;
	struct mcde_channel_color_key chnannel_color_key;
	struct mcde_conf_color_conv color_conv_ctrl;
	struct mcde_blend_control blend_ctrl;
	struct mcde_source_buffer src_buffer;
	struct mcde_dithering_ctrl_conf dithering_ctrl_conf;
	char rot_dir = 0;
	mcde_roten rot_ctrl;
	u32 videoMode;
	u32 rem_key;
	u32 create_key;
	u32 flags;
	u32 scanMode = 0;
	void __user *argp = (void __user *) arg;
	mcde_fb_lock(info, __func__);
	memset (&blend_ctrl, 0, sizeof(struct mcde_blend_control));
	switch (cmd)
	{
		/*
		 *   MCDE_IOCTL_OVERLAY_CREATE: This ioctl is used eigther to create a new overlay or configure the existing overlay.
		 */
		case MCDE_IOCTL_OVERLAY_CREATE:

			if (copy_from_user(&extsrc_ovl, argp, sizeof(struct mcde_overlay_create)))
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to copy data from user space\n");
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_extsrc_ovl_create(&extsrc_ovl,info,&create_key) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to create a new overlay surface\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			extsrc_ovl.key = ((create_key) << PAGE_SHIFT);

			if (copy_to_user(argp, &extsrc_ovl, sizeof(struct mcde_overlay_create))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			break;

		/*
		 *   MCDE_IOCTL_OVERLAY_REMOVE: This ioctl is used to remove the overlay(not base overlay).
		 */
		case MCDE_IOCTL_OVERLAY_REMOVE:

			if (copy_from_user(&rem_key, argp, sizeof(u32))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			rem_key = rem_key >> PAGE_SHIFT;
			dbgprintk(MCDE_DEBUG_INFO, "remove overlay id  %d\n",rem_key);

			/** dont touch base overlay 0 */
			if(rem_key < num_of_display_devices)
			{
				dbgprintk(MCDE_ERROR_INFO, "Cant remove base overlay 0\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			if (mcde_extsrc_ovl_remove(info,rem_key) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "cant remove overlay id  %d\n",rem_key);
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			break;
		/*
		 *   MCDE_IOCTL_ALLOC_FRAMEBUFFER: This ioctl is used to allocate the memory for frame buffer.
		 */
		case MCDE_IOCTL_ALLOC_FRAMEBUFFER:
			if (copy_from_user(&source_buff, argp, sizeof(struct mcde_sourcebuffer_alloc))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			flags = claim_mcde_lock(currentpar->chid);
			if (mcde_alloc_source_buffer(source_buff,info,&create_key, TRUE) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to allocate framebuffer\n");
				release_mcde_lock(currentpar->chid, flags);
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			source_buff.buff_addr.dmaaddr = currentpar->buffaddr[create_key].dmaaddr;
			source_buff.buff_addr.bufflength = currentpar->buffaddr[create_key].bufflength;
			source_buff.key = ((create_key) << PAGE_SHIFT);
			release_mcde_lock(currentpar->chid, flags);

			if (copy_to_user(argp, &source_buff, sizeof(struct mcde_sourcebuffer_alloc))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			break;
		/*
		 *   MCDE_IOCTL_DEALLOC_FRAMEBUFFER: This ioctl is used to de -allocate the memory used for frame buffer.
		 */
		case MCDE_IOCTL_DEALLOC_FRAMEBUFFER:
			if (copy_from_user(&rem_key, argp, sizeof(u32))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			rem_key = rem_key >> PAGE_SHIFT;
			dbgprintk(MCDE_ERROR_INFO, "remove source buffer id  %d\n",rem_key);

			if((rem_key <= MCDE_MAX_FRAMEBUFF) &&  (rem_key > 2*MCDE_MAX_FRAMEBUFF))
			{
				dbgprintk(MCDE_ERROR_INFO, "Cant remove the source buffer\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			//flags = claim_mcde_lock(currentpar->chid);
			if (mcde_dealloc_source_buffer(info, rem_key, TRUE) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to deallocate framebuffer\n");
			//	release_mcde_lock(currentpar->chid, flags);
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			//release_mcde_lock(currentpar->chid, flags);
			break;
		/*
		 *   MCDE_IOCTL_CONFIGURE_EXTSRC: This ioctl is used to configure the external source configuration.
		 */
		case MCDE_IOCTL_CONFIGURE_EXTSRC:
			if (copy_from_user(&ext_src_config, argp, sizeof(struct mcde_ext_conf))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_extsource(ext_src_config,info) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to configure the extsrc reg\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			break;
		/*
		 *   MCDE_IOCTL_CONFIGURE_OVRLAY: This ioctl is used to configure the overlay configuration.
		 */
		case MCDE_IOCTL_CONFIGURE_OVRLAY:
			if (copy_from_user(&overlay_config, argp, sizeof(struct mcde_conf_overlay))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_overlay(overlay_config,info) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to configure overlay reg\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			break;
		/*
		 *   MCDE_IOCTL_CONFIGURE_CHANNEL: This ioctl is used to configure the channel configuration.
		 */
		case MCDE_IOCTL_CONFIGURE_CHANNEL:
			if (copy_from_user(&ch_config, argp, sizeof(struct mcde_ch_conf))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_channel(ch_config,info) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to configure the channel reg\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			break;
		/*
		 *   MCDE_IOCTL_CONFIGURE_PANEL: This ioctl is used to configure the panel(output interface) configuration.
		 */
		case MCDE_IOCTL_CONFIGURE_PANEL:
			if (copy_from_user(&chnl_lcd_ctrl, argp, sizeof(struct mcde_chnl_lcd_ctrl))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_lcd_timing(chnl_lcd_ctrl,info) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to configure the panel timings\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_MCDE_ENABLE: This ioctl is used to enable the flow(A/B/C0/C1) and power of the MCDE.
		 */
		case MCDE_IOCTL_MCDE_ENABLE:
			if (mcde_enable(info) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to enable MCDE\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_MCDE_DISABLE: This ioctl is used to disable the flow(A/B/C0/C1) and power of the MCDE.
		 */
		case MCDE_IOCTL_MCDE_DISABLE:
			if (mcde_disable(info) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to disable MCDE\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_COLOR_KEYING_ENABLE: This ioctl is used to enable color keying.
		 */
		case MCDE_IOCTL_COLOR_KEYING_ENABLE:
			if (copy_from_user(&chnannel_color_key, argp, sizeof(struct mcde_channel_color_key))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_channel_color_key(info, chnannel_color_key) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to enable channel color keying\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_COLOR_KEYING_DISABLE: This ioctl is used to disable color keying.
		 */
		case MCDE_IOCTL_COLOR_KEYING_DISABLE:
			chnannel_color_key.key_ctrl = MCDE_CLR_KEY_DISABLE;
			if (mcde_conf_channel_color_key(info, chnannel_color_key) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to disable channel color keying\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_COLOR_COVERSION_ENABLE: This ioctl is used to enable color color conversion.
		 */
		case MCDE_IOCTL_COLOR_COVERSION_ENABLE:
			if (copy_from_user(&color_conv_ctrl, argp, sizeof(struct mcde_conf_color_conv))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_color_conversion(info, color_conv_ctrl) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to enable channel color keying\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_COLOR_COVERSION_DISABLE: This ioctl is used to disable color color conversion.
		 */
		case MCDE_IOCTL_COLOR_COVERSION_DISABLE:
			color_conv_ctrl.col_ctrl = MCDE_COL_CONV_DISABLE;
			if (mcde_conf_color_conversion(info, color_conv_ctrl) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to enable channel color keying\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_CHANNEL_BLEND_ENABLE: This ioctl is used to enable alpha blending.
		 */
		case MCDE_IOCTL_CHANNEL_BLEND_ENABLE:
			if (copy_from_user(&blend_ctrl, argp, sizeof(struct mcde_blend_control))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_conf_blend_ctrl(info, blend_ctrl) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to enable channel color keying\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_CHANNEL_BLEND_DISABLE: This ioctl is used to disable blending.
		 */
		case MCDE_IOCTL_CHANNEL_BLEND_DISABLE:
			blend_ctrl.blenden = MCDE_BLEND_DISABLE;
			blend_ctrl.ovr2_enable = 0;
			blend_ctrl.ovr2_id=0;
			blend_ctrl.ovr1_id=0;
			if (mcde_conf_blend_ctrl(info, blend_ctrl) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to enable channel color keying\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_ROTATION_ENABLE: This ioctl is used to enable rotation.
		 */
		case MCDE_IOCTL_ROTATION_ENABLE:
			if (copy_from_user(&rot_dir, argp, sizeof(char))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			rot_ctrl = MCDE_ROTATION_ENABLE;
			mcde_conf_rotation(info, rot_dir, rot_ctrl, currentpar->rotationbuffaddr0.dmaaddr, currentpar->rotationbuffaddr1.dmaaddr);
			break;
		/*
		 *   MCDE_IOCTL_ROTATION_DISABLE: This ioctl is used to disable rotation.
		 */
		case MCDE_IOCTL_ROTATION_DISABLE:
			rot_ctrl = MCDE_ROTATION_DISABLE;
			mcde_conf_rotation(info, rot_dir, rot_ctrl, 0x0, 0x0);
			break;
		/*
		 *   MCDE_IOCTL_SET_VIDEOMODE: This ioctl is used to configure the video mode(for different refresh rates and resolutions).
		 */
		case MCDE_IOCTL_SET_VIDEOMODE:
			if (copy_from_user(&videoMode, argp, sizeof(u32)))
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to set video mode\n");
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			/** SPINLOCK in use :  */
			//flags = claim_mcde_lock(currentpar->chid);
			isVideoModeChanged = 1;
			if (mcde_dealloc_source_buffer(info, currentpar->mcde_cur_ovl_bmp, FALSE) != MCDE_OK)
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to set video mode\n");
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
		        currentpar->isHwInitalized = 0;
			find_restype_from_video_mode(info, videoMode);
			mcde_set_video_mode(info, videoMode);
			mcde_change_video_mode(info);
			//release_mcde_lock(currentpar->chid, flags);
			break;
		/*
		 *   MCDE_IOCTL_CONFIGURE_DENC: This ioctl is used to configure denc.
		 */
		case MCDE_IOCTL_CONFIGURE_DENC:

			break;
		/*
		 *   MCDE_IOCTL_CONFIGURE_HDMI: This ioctl is used to configure HDMI.
		 */
		case MCDE_IOCTL_CONFIGURE_HDMI:

			break;
		/*
		 *   MCDE_IOCTL_SET_SOURCE_BUFFER: This ioctl is used to set the source buffer(frame buffer).
		 */
		case MCDE_IOCTL_SET_SOURCE_BUFFER:
			if (copy_from_user(&src_buffer, argp, sizeof(struct mcde_source_buffer)))
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to copy the data from user space in MCDE_IOCTL_SET_SOURCE_BUFFER\n");
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			mcde_set_buffer(info, src_buffer.buffaddr.dmaaddr, src_buffer.buffid);
			break;
		/*
		 *   MCDE_IOCTL_DITHERING_ENABLE: This ioctl is used to enable dithering.
		 */
		case MCDE_IOCTL_DITHERING_ENABLE:
			if (copy_from_user(&dithering_ctrl_conf, argp, sizeof(struct mcde_dithering_ctrl_conf)))
			{
				dbgprintk(MCDE_ERROR_INFO, "Failed to copy the data from user space in MCDE_IOCTL_DITHERING_ENABLE\n");
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			mcde_conf_dithering_ctrl(dithering_ctrl_conf, info);
			break;
		/*
		 *   MCDE_IOCTL_SET_SCAN_MODE: This ioctl is used to sconfigure scan mode.
		 */
		case MCDE_IOCTL_SET_SCAN_MODE:
			if (copy_from_user(&scanMode, argp, sizeof(u32)))
			{
				dbgprintk(MCDE_IOCTL_SET_SCAN_MODE, "Failed to copy the data from user space in MCDE_IOCTL_SET_SCAN_MODE\n");
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			mcde_conf_scan_mode(scanMode, info);
			break;
		/*
		 *   MCDE_IOCTL_GET_SCAN_MODE: This ioctl is used to get the scan mode.
		 */
		case MCDE_IOCTL_GET_SCAN_MODE:
			if (copy_to_user(argp, &info->var.vmode, sizeof(u32))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}
			break;
		/*
		 *   MCDE_IOCTL_TEST_DSI_HSMODE: This ioctl is used to test the dsi direct command high speed mode.
		 */
		case MCDE_IOCTL_TEST_DSI_HSMODE:
			if (copy_from_user(&rem_key, argp, sizeof(u32))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_dsi_test_dsi_HS_directcommand_mode(info,rem_key) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "HS direct command test case failed\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}
			break;
		/*
		 *   MCDE_IOCTL_TEST_DSI_LPMODE: This ioctl is used to test the dsi direct command low power mode.
		 */
		case MCDE_IOCTL_TEST_DSI_LPMODE:
			if (copy_from_user(&rem_key, argp, sizeof(u32))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}

			if (mcde_dsi_test_LP_directcommand_mode(info,rem_key) < 0)
			{
				dbgprintk(MCDE_ERROR_INFO, "Low Power direct command test case failed\n");
				mcde_fb_unlock(info, __func__);
				return -EINVAL;
			}

			break;
        /*
		 *   MCDE_IOCTL_TV_PLUG_STATUS: This ioctl is used to get the TV plugin status.
		 */
		case MCDE_IOCTL_TV_PLUG_STATUS:
#ifndef CONFIG_U8500_SIM8500
		    rem_key=ab8500_read(0xE,0xE00);
#endif
		    if(rem_key&0x4) rem_key=1;
		    else rem_key=0;

		    if (copy_to_user(argp, &rem_key, sizeof(u32))) {
			    mcde_fb_unlock(info, __func__);
			    return -EFAULT;
		    }
			break;

	    /*
	     *  MCDE_IOCTL_TV_CHANGE_MODE: This ioctl is used to change TV mode
	     */
		case MCDE_IOCTL_TV_CHANGE_MODE:
			if (copy_from_user(&rem_key, argp, sizeof(u32))) {
				mcde_fb_unlock(info, __func__);
				return -EFAULT;
			}



#ifndef CONFIG_U8500_SIM8500

        if(rem_key==VMODE_720_480_60_P)
        {
             ab8500_write(0x6,0x600,0x8A);
             scanMode=MCDE_SCAN_PROGRESSIVE_MODE;
	}
        else if(rem_key==VMODE_720_480_60_I)
        {
	         ab8500_write(0x6,0x600,0x8A);
             scanMode=MCDE_SCAN_INTERLACED_MODE;
	    }

        else if(rem_key==VMODE_720_576_50_P)
        {
	         ab8500_write(0x6,0x600,0xA);
             scanMode=MCDE_SCAN_PROGRESSIVE_MODE;
	    }

        else if(rem_key==VMODE_720_576_50_I)
        {
	         ab8500_write(0x6,0x600,0xA);
             scanMode=MCDE_SCAN_INTERLACED_MODE;
	    }


         mcde_conf_scan_mode(scanMode,info);
#endif

            break;

	    /*
	     *  MCDE_IOCTL_TV_GET_MODE: This ioctl is used to get TV mode
	     */
		case MCDE_IOCTL_TV_GET_MODE:

            /** get the PAL and NTSC mode */
#ifndef CONFIG_U8500_SIM8500
            rem_key=ab8500_read(0x6,0x600);
#endif

            if(rem_key&0x40) rem_key=1;
            else rem_key=0;

		    if (copy_to_user(argp, &rem_key, sizeof(u32))) {
			    mcde_fb_unlock(info, __func__);
			    return -EFAULT;
		    }
			break;

		default:
			mcde_fb_unlock(info, __func__);
			return -EINVAL;

	}

	mcde_fb_unlock(info, __func__);
	return 0;
}


void tv_detect_handler(void)
{
        printk("TV plugin detected\n");
        mcde_4500_plugstatus=1;
        return;
}


void tv_removed_handler(void)
{
        printk("TV plugin removed \n");
        mcde_4500_plugstatus=0;
        return;
}


void mcde_change_video_mode(struct fb_info *info)

{

	struct mcdefb_info *currentpar = (struct mcdefb_info *)info->par;


	if(currentpar->chid==MCDE_CH_B)
	{
       currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;
       currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf=(info->var.yres<<16)|(MCDE_EXT_SRC_2<<11)|(info->var.xres);
       currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_ljinc=(info->var.xres)*(currentpar->chnl_info->inbpp/8);
    }

	if(currentpar->chid==MCDE_CH_C0)
	{
       currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;
       currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf=(info->var.yres<<16)|(MCDE_EXT_SRC_0<<11)|(info->var.xres);
       currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_ljinc=(info->var.xres)*(currentpar->chnl_info->inbpp/8);
    }

	if(currentpar->chid==MCDE_CH_C1)
	{

       currentpar->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;
       currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf=(info->var.yres<<16)|(MCDE_EXT_SRC_1<<11)|(info->var.xres);
       currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_ljinc=(info->var.xres)*(currentpar->chnl_info->inbpp/8);
    }


}
/**
 * mcde_configure_hdmi_channel() - To configure MCDE & DSI hardware registers for HDMI/CVBS.
 *
 * This function configures MCDE & DSI hardware for the HDMI/CVBS device.
 *
 */
void mcde_configure_hdmi_channel(void)
{
#if 1
	struct mcdefb_info *currentpar = gpar[MCDE_CH_A];
	if((currentpar->chid==MCDE_CH_A)&&(!(currentpar->regbase->mcde_cr&0x4)))
	{

//g_info->var.yres = 1;
		/** disable MCDE first then configure */
		currentpar->regbase->mcde_cr=(currentpar->regbase->mcde_cr & ~ MCDE_ENABLE);


		/** configure mcde external registers */

		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;

		if (currentpar->chnl_info->inbpp==16)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_conf=(MCDE_RGB565_16_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;
		if (currentpar->chnl_info->inbpp==32)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_conf=(MCDE_ARGB_32_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;
		if (currentpar->chnl_info->inbpp==24)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_conf=(MCDE_RGB_PACKED_24_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;

		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_cr=(MCDE_FS_FREQ_DIV_DISABLE<<3)|MCDE_BUFFER_SOFTWARE_SELECT;



        /** configure mcde overlay registers */

		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_cr=0x22b00001;

		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf=(g_info->var.yres<<16)|(MCDE_EXT_SRC_0<<11)|(g_info->var.xres);

		//currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf=(0x30<<16)|(MCDE_EXT_SRC_0<<11)|(0x10);

		/** rgb888 24 bit format packed data 3 bytes limited to 480 X 682 */

		if(currentpar->chnl_info->inbpp==24)

		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf=(g_info->var.yres<<16)|(MCDE_EXT_SRC_0<<11)|(0x2AA);

		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf2=0x5000000;//0xa200000;

		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_ljinc=(g_info->var.xres)*(currentpar->chnl_info->inbpp/8);

		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_comp=(MCDE_CH_A<<11);



        /** configure mcde channel config registers */

		currentpar->ch_regbase1[MCDE_CH_A]->mcde_ch_conf=((g_info->var.yres - 1)<<16)|(g_info->var.xres - 1);//0x1df0279; /** screen parameters */

		currentpar->ch_regbase1[MCDE_CH_A]->mcde_chsyn_mod=MCDE_SYNCHRO_SOFTWARE;

		currentpar->ch_regbase1[MCDE_CH_A]->mcde_chsyn_bck=0x0;

		currentpar->ch_regbase2[MCDE_CH_A]->mcde_ch_cr0=0x1;

		mdelay(100);

		currentpar->ch_regbase2[MCDE_CH_A]->mcde_ch_cr0=0x3;




		/** configure channel C0 register */

	//	currentpar->ch_c_reg->mcde_chc_crc |=0x387B0027;



        /** configure mcde base registers */


		currentpar->regbase->mcde_cfg0 |=0x5E145001;



		mdelay(100);

        /** configure mcde DSI formatter */
        if(currentpar->chnl_info->inbpp==24)
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_conf0=(0x2<<20)|(0x1<<18)|(0x1<<13);
		else if (currentpar->chnl_info->inbpp==16)
			currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_conf0=(0x0<<20)|(0x1<<18)|(0x1<<13);
			//currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_conf0=(0x2<<20)|(0x1<<18)|(0x1<<13);
		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_frame=(1+(g_info->var.xres*currentpar->chnl_info->inbpp/8))*g_info->var.yres;
//currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_frame=(1+(g_info->var.xres*24/8))*g_info->var.yres;
		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_pkt=1+(g_info->var.xres*currentpar->chnl_info->inbpp/8);
//currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_pkt=1+(g_info->var.xres*24/8);
		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD2]->mcde_dsi_cmd=(0x3<<4)|MCDE_DSI_VID_MODE_SHIFT; //0x2C;//

		mdelay(100);

		/** send DSI command for RAMWR */
#if 1
		currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_main_settings=0x43908;
		currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat0=0x3c;
		currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_send=0x1;



		mdelay(100);


	    currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_main_settings|=0x200004;
#endif

		/** do a software sync */

		currentpar->ch_regbase1[MCDE_CH_A]->mcde_chsyn_sw=MCDE_NEW_FRAME_SYNCHRO;


	   mdelay(100);
	/** for channelA */
	//currentpar->regbase->mcde_imsc |=0x4;
	currentpar->regbase->mcde_cr |=0x80000001;

	}
#endif
}
EXPORT_SYMBOL(mcde_configure_hdmi_channel);
/**
 * mcde_hdmi_display_init_command_mode() - To initialize and powerup the HDMI device in command mode.
 *
 * This function initializez and power up HDMI/CVBS device.
 *
 */
void mcde_hdmi_display_init_command_mode()
{

	//int i;
    struct mcdefb_info *currentpar = gpar[MCDE_CH_A];
  //  volatile u32 __iomem *mcde_dsi_clk;

	dbgprintk(MCDE_ERROR_INFO, "\n>> HDMI Dispaly Initialisation done\n\n\n");

	if(currentpar->chid==MCDE_CH_A)
	{


		*(currentpar->prcm_mcde_clk) = 0x125; /** MCDE CLK = 160MHZ */
		*(currentpar->prcm_hdmi_clk) = 0x145; /** HDMI CLK = 76.8MHZ */
		*(currentpar->prcm_tv_clk) = 0x14E; /** HDMI CLK = 76.8MHZ */

		//currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_data_ctl|=0x380;
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_data_ctl=0x1;
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_phy_ctl=0x5;

		//currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_data_ctl|=0x380;

		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_en=0x438;

		mdelay(100);


		/*currentpar->mcde_clkdsi = 0xA1010C;*/
		mdelay(100);


        /** configure DSI link2 which is having plls */

		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_data_ctl |= 0x1;
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_phy_ctl |= 0x1;

#ifndef CONFIG_FB_MCDE_MULTIBUFFER

		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_pll_ctl = 0x10511;//0x104A0;//0x104A2;
#else

		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_pll_ctl = 0x10511;//0x10511;//0x1049E;//0x104A2
#endif

		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_en=0x9;

		mdelay(100);

		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_pll_ctl |= 0x10000;
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_en |= 0x439;

		mdelay(100); /** check for pll lock */

		currentpar->dsi_lnk_registers[DSI_LINK2]->cmd_mode_ctl |= 0x3FF0040;
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_dphy_static |=  0x600;//0x3c0;
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_dphy_timeout |= 0xffffffff;
		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_ulpout_time |= 0x201;

		mdelay(100);
#if 0

		/** send DSI commands to make display up */

        /** make the display up ~ send commands */

		 currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_main_settings=0x210500;
		 currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat0=0x11;
		 currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_send=0x1;

		 mdelay(150); /** sleep for 150 ms */


		 /** send teaing command with low power mode */

		 currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_main_settings = 0x221500;
		 currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_wrdat0 = 0x35;
		 currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_send = 0x1;

		 mdelay(100);

		 /** send color mode */

		 currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_main_settings = 0x221500;
		 currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_wrdat0 = 0xf73a;
		 currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_send = 0x1;

		 mdelay(100);

		 /** send power on command */

		 currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_main_settings = 0x210500;
		 currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_wrdat0 = 0xF729;
		 currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_send = 0x1;

		 mdelay(100); /** check for display to up ok ~ HDMI display */
#endif
	}

	return;
}
EXPORT_SYMBOL(mcde_hdmi_display_init_command_mode);
/**
 * mcde_hdmi_display_init_command_mode() - To initialize and powerup the HDMI device in command mode.
 *
 * This function initializez and power up HDMI/CVBS device.
 *
 */
void mcde_hdmi_display_init_video_mode()
{

	//int i;
    struct mcdefb_info *currentpar = gpar[MCDE_CH_A];
//    volatile u32 __iomem *mcde_dsi_clk;

	dbgprintk(MCDE_ERROR_INFO, "\n>> HDMI Dispaly Initialisation done\n\n\n");

	if(currentpar->chid==MCDE_CH_A)
	{


		*(currentpar->prcm_mcde_clk) = 0x125; /** MCDE CLK = 160MHZ */
		*(currentpar->prcm_hdmi_clk) = 0x145; /** HDMI CLK = 76.8MHZ */
		*(currentpar->prcm_tv_clk) = 0x14E; /** HDMI CLK = 76.8MHZ */

		//currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_data_ctl|=0x380;
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_data_ctl=0x3; // enable link and video mode
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_phy_ctl=0x01; //wait burst time = 8, lane2 enable

		//currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_data_ctl|=0x380;

		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_en=0x38;

		mdelay(100);


		/*currentpar->mcde_clkdsi = 0xA1010C;*/
		mdelay(100);


        /** configure DSI link2 which is having plls */


		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_pll_ctl = 0x10511;//0x10513;//0x10511;//0x1049E;//0x104A2

		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_en |= 0x1;

		mdelay(100);

		currentpar->dsi_lnk_registers[DSI_LINK2]->cmd_mode_ctl |= 0x40;//0x3FF0040; /* command mode*/
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_dphy_static |=  0x600;//0x3c0;
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_dphy_timeout |= 0x7ffffffA;
		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_ulpout_time |= 0x0;//201;



	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vsize = 0x1E00A8C2;//0x1E0 << 20 | 0x0 << 12 | 0x0 << 6 | 0x0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize1 = 0x30120;//0x0 << 20 | 0x0 << 10 | 0x0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hsize2 = 0x780;//0x500;
	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_main_ctl = 0x1E3F85;//0x3000;



	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize1 = 0x0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_blksize2 = 0x0;


	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_pck_time = 0x0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_dphy_time = 0xA002;
	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_err_color = 0xFF;
	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vpos = 0x0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_hpos = 0x0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting1 = 0x0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->vid_vca_setting2 = 0x0;
	/** Test Video Mode regsiter */
		currentpar->dsi_lnk_registers[DSI_LINK2]->tvg_img_size = 0x1E00780;
	currentpar->dsi_lnk_registers[DSI_LINK2]->tvg_color1 |= 0xFF;
	currentpar->dsi_lnk_registers[DSI_LINK2]->tvg_color2 |= 0xFF;


		mdelay(100);
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_data_ctl |= 0x4; //VSG enable
		currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_data_ctl |= 0x8; //VTG enable

		currentpar->dsi_lnk_registers[DSI_LINK2]->tvg_ctl = 0x1; //VTG run
	}

	return;
}
EXPORT_SYMBOL(mcde_hdmi_display_init_video_mode);
/**
 Initialization of TPO Display using LP mode and DSI direct command registor  & dispaly of two alternating color
 */
void mcde_hdmi_test_directcommand_mode_highspeed(void)
{
#if 0
  struct dsi_dphy_static_conf dphyStaticConf;
  struct dsi_link_conf dsiLinkConf;
#endif
  struct mcdefb_info *currentpar = g_info->par;
#if 0
  unsigned int retVal= 0;
#endif
  int temp;
  unsigned char PixelRed;
  unsigned char PixelGreen;
  unsigned char PixelBlue;
  int pixel_nb ;
  u32 value = 0;

  PixelRed=0x00;
  PixelGreen=0x1F;
  PixelBlue=0x00;
  pixel_nb = (g_info->var.xres*g_info->var.yres)*(currentpar->chnl_info->inbpp/8);//info->var.xres*info->var.yres;
#if 0
  dsiLinkConf.dsiInterfaceMode = DSI_INTERFACE_NONE;
  dsiLinkConf.clockContiniousMode = DSI_CLK_CONTINIOUS_HS_DISABLE;
  dsiLinkConf.dsiLinkState = DSI_ENABLE;
  dsiLinkConf.clockLaneMode = DSI_LANE_ENABLE;
  dsiLinkConf.dataLane1Mode = DSI_LANE_ENABLE;
  dsiLinkConf.dataLane2Mode = DSI_LANE_DISABLE;
  dphyStaticConf.datalane1swappinmode = HS_SWAP_PIN_DISABLE;
  dphyStaticConf.clocklaneswappinmode = HS_SWAP_PIN_DISABLE;
  dsiLinkConf.commandModeType = DSI_CLK_LANE_LPM;
  dphyStaticConf.ui_x4 = 0;
  /*** To configure the dsi clock and enable the clock lane and data1 lane */
  dsiLinkInit(&dsiLinkConf, dphyStaticConf, currentpar->chid, currentpar->dsi_lnk_no);

  /** The Screen is filled with one color */


	retVal = dsiHSdcslongwrite(VC_ID0, 4, TPO_CMD_RAMWR,  PixelRed,PixelGreen, PixelBlue,0,0,0,0,0,0,0,0,0,0,0,0, currentpar->chid, currentpar->dsi_lnk_no);
	retVal = dsiHSdcslongwrite(VC_ID0, 4, TPO_CMD_RAMWR_CONTINUE,  PixelRed,PixelGreen, PixelBlue,
								PixelRed, PixelGreen,PixelBlue,
								PixelRed,	PixelGreen,PixelBlue,
								PixelRed,PixelGreen,PixelBlue,
								0,0,0, currentpar->chid, currentpar->dsi_lnk_no);
#endif
	for (temp=0; temp< (pixel_nb) ;temp= temp + 15)
	{
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_main_settings=0x103908;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat0=0xAAAAAA2c;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat1=0xAAAAAAAA;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat2=0xAAAAAAAA;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat3=0xAAAAAAAA;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_send=0x1;
	//mdelay(1);
	value = currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts;
	while(value != 0x2)
	{
		printk("Write data is not completed: value:%x\n", value);
		value = currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts;
	};
	//printk("Write data : value:%x\n", value);
	value = 0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts_clr = 0x2;
#if 0
	  retVal = dsiHSdcslongwrite(VC_ID0, 16, TPO_CMD_RAMWR_CONTINUE,  PixelRed,PixelGreen, PixelBlue,
								PixelRed, PixelGreen,PixelBlue,
								PixelRed,	PixelGreen,PixelBlue,
								PixelRed,PixelGreen,PixelBlue,
								PixelRed,PixelGreen,PixelBlue,currentpar->chid, currentpar->dsi_lnk_no);
	if( temp % 85 == 0)
		for(i = 0; i < 0xfff; i++);
#endif
		//isReady = 0;
     }
return ;
}
EXPORT_SYMBOL(mcde_hdmi_test_directcommand_mode_highspeed);

void mcde_send_hdmi_cmd_data(char* buf,int length, int dsicommand)
{
	struct mcdefb_info *currentpar = g_info->par;
	int value = 0x0;

	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_main_settings=length << 16 | 0x3908;//0x103908;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat0=buf[2] << 24 | buf[1] << 16 | buf[0] << 8 | dsicommand;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat1=buf[6] << 24 | buf[5] << 16 | buf[4] << 8 | buf[3];
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat2=buf[10] << 24 | buf[9] << 16 | buf[8] << 8 | buf[7];
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat3=buf[14] << 24 | buf[13] << 16 | buf[12] << 8 | buf[11];
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_send=0x1;
		//mdelay(1);
	value = currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts;
	while(!(value & 0x2))
	{
		//printk("mcde_send_hdmi_cmd_data:Write data is not completed: value:%x\n", value);
		value = currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts;
	};
		printk("mcde_send_hdmi_cmd_data:Write data : buf[0]:%x,buf[1]:%x,buf[2]:%x,dsicommand:%x, length:%x\n", buf[0], buf[1], buf[2],dsicommand, length);
	value = 0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts_clr = 0x2;
}
EXPORT_SYMBOL(mcde_send_hdmi_cmd_data);
void mcde_send_hdmi_cmd(char* buf,int length, int dsicommand)
{
	struct mcdefb_info *currentpar = g_info->par;
	int value = 0x0;

	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_main_settings=length << 16 | 0x3908;//0x103908;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_wrdat0= buf[0] << 8 | dsicommand;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_send=0x1;
		//mdelay(1);
	value = currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts;
	printk("mcde_send_hdmi_cmd:Write data : buf[0]:%x,dsicommand:%x, length:%x\n", buf[0],dsicommand, length);
	while(!(value & 0x2))
	{
		//printk("mcde_send_hdmi_cmd:Write data is not completed: value:%x\n", value);
		value = currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts;
	};
		printk("Write data : value:%x\n", value);
	value = 0;
	currentpar->dsi_lnk_registers[DSI_LINK2]->direct_cmd_sts_clr = 0x2;
}
EXPORT_SYMBOL(mcde_send_hdmi_cmd);
/* mcde_conf_video_mode() - To configure MCDE & DSI hardware registers.
 * @info: frame buffer information.
 *
 * This function configures MCDE & DSI hardware for the device (primary/secondary/HDMI/TVOUT)as per the given configuration
 *
 */
static void mcde_conf_video_mode(struct fb_info *info)
{
	struct mcdefb_info *currentpar = (struct mcdefb_info *)info->par;



#ifdef CONFIG_FB_U8500_MCDE_CHANNELB

#ifndef  CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_VUIB_WVGA

    /** use channel B for tvout & do nothing if it already enabled */
    if((currentpar->chid==MCDE_CH_B)&&(!(currentpar->regbase->mcde_cr&0x100)))
    {


        /** disable MCDE first then configure */
        currentpar->regbase->mcde_cr=(currentpar->regbase->mcde_cr & ~ MCDE_ENABLE);

		/** configure mcde external registers */

		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;

        if (currentpar->chnl_info->inbpp==16)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_conf=0x704;
		if (currentpar->chnl_info->inbpp==32)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_conf=0xA04;
		if (currentpar->chnl_info->inbpp==24)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_conf=0x804|1<<12;


		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_cr= MCDE_BUFFER_SOFTWARE_SELECT;




        /** configure mcde overlay registers */

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_cr=0x22b00003;

		/** 720 X 576 */

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf=(0x238<<16)|(MCDE_EXT_SRC_2<<11)|(0x2cc);

		/** rgb888 24 bit format packed data 3 bytes limited to 480 X 682 */

		if (currentpar->chnl_info->inbpp==24)

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf=(0x1E0<<16)|(MCDE_EXT_SRC_2<<11)|(0x2AA);


        /** Tv out requires less watermark ,if other displays are enabled ~ 80 will do */
		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf2=0x500000;

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_ljinc=(0x2cc)*(currentpar->chnl_info->inbpp/8);

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_comp=(16<<16)|(MCDE_CH_B<<11)|16;





		/** configure mcde channel config registers */

		currentpar->ch_regbase1[MCDE_CH_B]->mcde_ch_conf=0x023702Cb;

		currentpar->ch_regbase1[MCDE_CH_B]->mcde_chsyn_mod=0x0;

		currentpar->ch_regbase1[MCDE_CH_B]->mcde_chsyn_bck=0x00FF0000;


       /** Configure Channel B registers for TVOUT */

        currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr0=0x0;


		if (currentpar->chnl_info->outbpp==MCDE_BPP_1_TO_8)
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr1=0x20001000|4<<MCDE_OUTBPP_SHIFT;
		else if(currentpar->chnl_info->outbpp==MCDE_BPP_12)
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr1=0x20001000|5<<MCDE_OUTBPP_SHIFT;
		else if(currentpar->chnl_info->outbpp==MCDE_BPP_16)
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr1=0x20001000|7<<MCDE_OUTBPP_SHIFT;


        currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_colkey=0x0;

		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_fcolkey=0x0;


        /**

        This are video settings and will fail in case of MIRE testing
        So go for GFX settings

		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv1=0x004C0096;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv2=0x001D0083;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv3=0x039203EB;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv4=0x03D403A9;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv5=0x00830080;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv6=0x00000080;

		*/


		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv1=0x00420081;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv2=0x00190070;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv3=0x03A203EE;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv4=0x03DA03B6;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv5=0x00700080;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_rgbconv6=0x00100080;

	    currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_ffcoef0=0x0;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_ffcoef1=0x0;
        currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_ffcoef2=0x0;


		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvcr=0x011C0007;


		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvbl1=0x00020018;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvisl=0x00170016;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvdvo=0x00020002;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvswh=0x011C0030;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvtim1=0x02cc008C;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvbalw=0x00020002;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvbl2=0x00020019;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvblu=0x002C9C83;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_lcdtim0=0x00000000;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_lcdtim1=0x00200000;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_ditctrl=0x00000000;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_ditoff=0x00000000;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_ditoff=0x00000000;

		currentpar->ch_regbase2[MCDE_CH_B]->mcde_chsyn_con=0x000B0007;


        /** configure mcde base registers */

		currentpar->regbase->mcde_imsc |=0x0;

		currentpar->regbase->mcde_cfg0 |=0x00185000;

		currentpar->regbase->mcde_cr |=0x80000100;

		mdelay(100);

		/** enable channel flow now */

		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr0=0x1;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr0=0x3;

		mdelay(100);


	}

#else /** CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_VUIB_WVGA */


    /** use channel B for tvout & do nothing if it already enabled */
    if((currentpar->chid==MCDE_CH_B)&&(!(currentpar->regbase->mcde_cr&0x100)))
    {

        /** disable MCDE first then configure */
        currentpar->regbase->mcde_cr=(currentpar->regbase->mcde_cr & ~ MCDE_ENABLE);

		/** configure mcde external registers */

		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;

        if (currentpar->chnl_info->inbpp==16)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_conf=0x700;
		if (currentpar->chnl_info->inbpp==32)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_conf=0xA00;
		if (currentpar->chnl_info->inbpp==24)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_conf=0x800|1<<12;


		currentpar->extsrc_regbase[MCDE_EXT_SRC_2]->mcde_extsrc_cr= 0x6;


        /** configure mcde overlay registers */

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_cr=0x02300301;

		/** WVGA size  800 X 600 */

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf=(0x258<<16)|(MCDE_EXT_SRC_2<<11)|(0x31F);


		/** rgb888 24 bit format packed data 3 bytes limited to 480 X 682 */

		if (currentpar->chnl_info->inbpp==24)

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf=(0x1E0<<16)|(MCDE_EXT_SRC_2<<11)|(0x2AA);

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_conf2=0x004003FF;

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_ljinc=(0x320)*(currentpar->chnl_info->inbpp/8);

		currentpar->ovl_regbase[MCDE_OVERLAY_2]->mcde_ovl_comp=(0<<16)|(MCDE_CH_B<<11)|0;


		/** configure mcde channel config registers */

		currentpar->ch_regbase1[MCDE_CH_B]->mcde_ch_conf=0x0257031F;

		currentpar->ch_regbase1[MCDE_CH_B]->mcde_chsyn_mod=0x00000004;

		currentpar->ch_regbase1[MCDE_CH_B]->mcde_chsyn_bck=0x00000000;


       /** Configure Channel B registers for VGA display */

        currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr0=0x0;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr1=0x6E006000;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvcr=0x02580018;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvbl1=0x00020007;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvdvo=0x00000018;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvtim1=0x03200090;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_tvbalw=0x004E001E;
		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_lcdtim1=0x00400000;


        /** configure mcde base registers */

		currentpar->regbase->mcde_imsc |=0x8;

		currentpar->regbase->mcde_cfg0 |=0x59237007;

		currentpar->regbase->mcde_cr |=0x80000100;


		mdelay(100);

		/** enable channel flow now */

		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr0=0x1;

		mdelay(100);

		currentpar->ch_regbase2[MCDE_CH_B]->mcde_ch_cr0=0x3;

		mdelay(100);

	}
#endif /** CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_VUIB_WVGA */

#endif /** CONFIG_FB_U8500_MCDE_CHANNELB */


#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
	if((currentpar->chid==MCDE_CH_C0)&&(!(currentpar->regbase->mcde_cr&0x4)))
	{
		/** disable MCDE first then configure */
        currentpar->regbase->mcde_cr=(currentpar->regbase->mcde_cr & ~ MCDE_ENABLE);

		/** configure mcde external registers */
		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;

		if (currentpar->chnl_info->inbpp==16)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_conf=(MCDE_RGB565_16_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;
		if (currentpar->chnl_info->inbpp==32)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_conf=(MCDE_ARGB_32_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;
		if (currentpar->chnl_info->inbpp==24)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_conf=(MCDE_RGB_PACKED_24_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0|1<<12;

		currentpar->extsrc_regbase[MCDE_EXT_SRC_0]->mcde_extsrc_cr=(MCDE_FS_FREQ_DIV_DISABLE<<3)|MCDE_BUFFER_SOFTWARE_SELECT;

		/** configure mcde overlay registers */
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_cr=0xB2b00001;

		/** rgb888 24 bit format packed data 3 bytes limited to 480 X 682 */
		if(currentpar->chnl_info->inbpp==24)
			currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf=(0x1E0<<16)|(MCDE_EXT_SRC_0<<11)|(0x2AA);

		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf2=0x004003FF;
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_comp=(MCDE_CH_A<<11); /* Connecting overlay 0 with channel A */

		/** configure mcde channel config registers */
		currentpar->ch_regbase1[MCDE_CH_A]->mcde_chsyn_mod=MCDE_SYNCHRO_SOFTWARE;
		currentpar->ch_regbase1[MCDE_CH_A]->mcde_chsyn_bck=0xff;

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_WVGA_PORTRAIT
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf=(0x360<<16)|(MCDE_EXT_SRC_0<<11)|(0x1E0);
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_ljinc=(0x1E0)*(currentpar->chnl_info->inbpp/8);
		currentpar->ch_regbase1[MCDE_CH_A]->mcde_ch_conf=0x35f01df; /** screen parameters */

		/* U8500_ESRAM_BASE + 0x60000 */
		currentpar->ch_regbase2[MCDE_CH_A]->mcde_rotadd0 = U8500_ESRAM_BASE + 0x20000 * 5;
		currentpar->ch_regbase2[MCDE_CH_A]->mcde_rotadd1 = U8500_ESRAM_BASE + 0x20000 * 5 + 0x10000;
		currentpar->ch_regbase2[MCDE_CH_A]->mcde_ch_cr0 = (0xB << 24) | (0 << 15) | (1 << 6) | (1 << 1) | 1;
		currentpar->ch_regbase2[MCDE_CH_A]->mcde_ch_cr1 = (1 << 29) | (0x9 << 25) | (0x5 << 10);
		currentpar->regbase->mcde_cfg0 = 0x22487001;//0x5E147001;

#else
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf=(0x1E0<<16)|(MCDE_EXT_SRC_0<<11)|(0x360);
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_ljinc=(0x360)*(currentpar->chnl_info->inbpp/8);
		currentpar->ch_regbase1[MCDE_CH_A]->mcde_ch_conf=0x1df035f; /** screen parameters */
		currentpar->ch_regbase2[MCDE_CH_A]->mcde_ch_cr0 = (1 << 1) | 1;
		currentpar->ch_regbase2[MCDE_CH_A]->mcde_ch_cr1 = (1 << 29) | (0x7 << 25) | (0x5 << 10);

		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_conf2=0xa200000;
		currentpar->ovl_regbase[MCDE_OVERLAY_0]->mcde_ovl_cr=0x22b00001;
		currentpar->regbase->mcde_cfg0 = 0x22487001;

#endif

		/** configure channel C0 register */
		//currentpar->ch_c_reg->mcde_chc_crc |=0x387B0027;

		/** configure mcde base registers */
#ifndef CONFIG_FB_U8500_MCDE_CHANNELC1
		/*currentpar->regbase->mcde_imsc |=0x40;*/
#endif

		currentpar->regbase->mcde_cr |= 0x80000020;

		mdelay(100);

		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_en &= ~0x400; /* Disable IF2_EN */
		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_en |= 0x200; /* Enable IF1_EN */

		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_data_ctl &= ~0x2; /* Set interface 1 to command mode */

		/** configure mcde DSI formatter */
		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID0]->mcde_dsi_conf0=(0x2<<20)|(0x1<<18)|(0x0<<17)|(0x0<<16)|(0x1<<13);
		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID0]->mcde_dsi_frame=(1+(864*24/8))*480;
		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID0]->mcde_dsi_pkt=1+(864*24/8);
		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_VID0]->mcde_dsi_cmd=(0x3<<4)|MCDE_DSI_VID_MODE_SHIFT;

		mdelay(100);

		/** send DSI command for RAMWR */

		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_main_settings=0x43908;
		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_wrdat0=0x2c;
		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_send=0x1;

		mdelay(100);

		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_main_settings|=0x200004;
		//currentpar->regbase->mcde_imsc |= 0x4;

		/** do a software sync */
#ifndef CONFIG_FB_U8500_MCDE_CHANNELC1
		currentpar->ch_regbase1[MCDE_CH_A]->mcde_chsyn_sw=MCDE_NEW_FRAME_SYNCHRO;
#endif

		mdelay(100);
	}
#endif  /** CONFIG_FB_U8500_MCDE_CHANNELC0 */


#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1

	if((currentpar->chid==MCDE_CH_C1)&&(!(currentpar->regbase->mcde_cr&0x2)))
	{

		/** disable MCDE first then configure */
        currentpar->regbase->mcde_cr=(currentpar->regbase->mcde_cr & ~ MCDE_ENABLE);


		/** configure mcde external registers */

		currentpar->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_a0=currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;

		if (currentpar->chnl_info->inbpp==16)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_conf=(MCDE_RGB565_16_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;
		if (currentpar->chnl_info->inbpp==32)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_conf=(MCDE_ARGB_32_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0;
		if (currentpar->chnl_info->inbpp==24)
		currentpar->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_conf=(MCDE_RGB_PACKED_24_BIT<<8)|(MCDE_BUFFER_USED_1<<2)|MCDE_BUFFER_ID_0|1<<12;

		currentpar->extsrc_regbase[MCDE_EXT_SRC_1]->mcde_extsrc_cr=(MCDE_FS_FREQ_DIV_DISABLE<<3)|MCDE_BUFFER_SOFTWARE_SELECT;


		/** configure mcde overlay registers */

		currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_cr=0x22b00001;

		currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf=(0x1E0<<16)|(MCDE_EXT_SRC_1<<11)|(0x360);

		//currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf=(0x100<<16)|(MCDE_EXT_SRC_1<<11)|(0x100);

		/** rgb888 24 bit format packed data 3 bytes limited to 480 X 682 */

		if(currentpar->chnl_info->inbpp==24)

		currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf=(0x1E0<<16)|(MCDE_EXT_SRC_1<<11)|(0x2AA);

		currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_conf2=0xa200000;

		currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_ljinc=(0x360)*(currentpar->chnl_info->inbpp/8);

		currentpar->ovl_regbase[MCDE_OVERLAY_1]->mcde_ovl_comp=(MCDE_CH_C1<<11);



        /** configure mcde channel config registers */

		currentpar->ch_regbase1[MCDE_CH_C1]->mcde_ch_conf=0x1df035f; /** screen parameters */

		currentpar->ch_regbase1[MCDE_CH_C1]->mcde_chsyn_mod=MCDE_SYNCHRO_SOFTWARE;

		currentpar->ch_regbase1[MCDE_CH_C1]->mcde_chsyn_bck=0xff;


        /** configure channel C1 register */

		currentpar->ch_c_reg->mcde_chc_crc|=0x387B002b;


        /** configure mcde base registers */

#ifndef CONFIG_FB_U8500_MCDE_CHANNELC0

		currentpar->regbase->mcde_imsc |=0x80;
#else

        currentpar->regbase->mcde_imsc |=0xC0;
#endif



		currentpar->regbase->mcde_cfg0 |=0x5E145001;

		currentpar->regbase->mcde_cr |=0x80000002;


		mdelay(100);



        /** configure mcde DSI formatter */

		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD1]->mcde_dsi_conf0=(0x2<<20)|(0x1<<18)|(0x1<<13);

		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD1]->mcde_dsi_frame=(1+(864*24/8))*480;

		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD1]->mcde_dsi_pkt=1+(864*24/8);

		currentpar->mcde_dsi_channel_reg[MCDE_DSI_CH_CMD1]->mcde_dsi_cmd=(0x3<<4)|MCDE_DSI_VID_MODE_SHIFT;

		mdelay(100);


		/** send DSI command for RAMWR */

		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_main_settings=0x43908;
		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_wrdat0=0x2c;
		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_send=0x1;



		mdelay(100);

        currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_main_settings|=0x200004;

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0

        currentpar->ch_regbase1[MCDE_CH_C0]->mcde_chsyn_sw=MCDE_NEW_FRAME_SYNCHRO;

#endif

		/** do a software sync */
		currentpar->ch_regbase1[MCDE_CH_C1]->mcde_chsyn_sw=MCDE_NEW_FRAME_SYNCHRO;

		mdelay(100);


	}

#endif /** CONFIG_FB_U8500_MCDE_CHANNELC1 */



}
/**
 * mcde_blank() - To disable/enable the device (primary/secondary/HDMI/TVOUT).
 * @blank_mode: enable/disable.
 * @info: frame buffer information.
 *
 * This function configures MCDE & DSI hardware to disable/enable the device(primary/secondary/HDMI/TVOUT) based on the blank state
 *
 */
static int mcde_blank(int blank_mode, struct fb_info *info)
{
	mcde_fb_lock(info, __func__);
	if (blank_mode != 0) {
		/** ---- disable display engine */
		mcde_disable(info);
	} else {
		/**---- enable display engine */
		mcde_enable(info);
	}
	mcde_fb_unlock(info, __func__);

	return 0;
}
/**
 * mcde_set_par() - To set the video mode according to info->var.
 * @info: frame buffer information.
 *
 * This function configures MCDE & DSI hardware for the device(primary/secondary/HDMI/TVOUT) according to info->var.
 *
 */
static int mcde_set_par_internal(struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	u8 border=0;

	/** FIXME border length could be adjustable */
	//if (currentpar->tvout) border=0;
	info->fix.line_length = get_line_length((info->var.xres + (border * 2)), info->var.bits_per_pixel);
	info->fix.visual = (info->var.bits_per_pixel <= 8) ? FB_VISUAL_PSEUDOCOLOR : FB_VISUAL_TRUECOLOR;

	if (info->var.bits_per_pixel > 8)
		currentpar->palette_size = 0;
	else
		currentpar->palette_size = 1 << info->var.bits_per_pixel;

#ifdef CONFIG_FB_MCDE_MULTIBUFFER
       if (currentpar->isHwInitalized == 0)
       {
	mcde_conf_video_mode(info);
	currentpar->isHwInitalized = 1;
       }
#else
	mcde_conf_video_mode(info);
#endif
	return 0;
}

static int mcde_set_par(struct fb_info *info)
{
	mcde_fb_lock(info, __func__);
	mcde_set_par_internal(info);
	mcde_fb_unlock(info, __func__);
	return 0;
}
/**
 * mcde_setpalettereg() - To set the palette configuration.
 * @info: frame buffer information.
 *
 * This function sets the palette configuration for the device(primary/secondary/HDMI/TVOUT).
 *
 */
static int mcde_setpalettereg(u_int red, u_int green, u_int blue,
		u_int trans, struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
//	u_int val=0;
	mcde_palette palette;

	palette.alphared = (u16)red;
	palette.blue = (u8)blue;
	palette.green = green;
	mcdesetpalette(currentpar->chid, palette);
	return 0;
}
/**
 * mcde_setcolreg() - To set color register.
 * @info: frame buffer information.
 *
 * This function sets the color register for the device(primary/secondary/HDMI/TVOUT).
 *
 */
static int mcde_setcolreg(u_int regno, u_int red, u_int green,
		u_int blue, u_int transp, struct fb_info *info)
{
	u32 v;
	int ret=0;

	if (regno >= 256)  /** no. of hw registers */
		return -EINVAL;
	switch (info->fix.visual) {

		case FB_VISUAL_TRUECOLOR :

			if (regno >= 16)
				return -EINVAL;

#define CNVT_TOHW(val,width) ((((val)<<(width))+0x7FFF-(val))>>16)
			red = CNVT_TOHW(red, info->var.red.length);
			green = CNVT_TOHW(green, info->var.green.length);
			blue = CNVT_TOHW(blue, info->var.blue.length);
			transp = CNVT_TOHW(transp, info->var.transp.length);
#undef CNVT_TOHW

			v = (red << info->var.red.offset) |
				(green << info->var.green.offset) |
				(blue << info->var.blue.offset) |
				(transp << info->var.transp.offset);

			((u32 *) (info->pseudo_palette))[regno] = v;
			break;

		case FB_VISUAL_PSEUDOCOLOR:
			mcde_fb_lock(info, __func__);
			ret=mcde_setpalettereg(red, green, blue, transp, info);
			mcde_fb_unlock(info, __func__);
			break;
	}

	return ret;
}
/**
 * mcde_pan_display() - To display the frame buffer .
 * @var: variable screen info.
 * @info: frame buffer information.
 *
 * This function sets the frame buffer for display and unmasks the vsync interrupt for the device(primary/secondary/HDMI/TVOUT).
 *
 */
static int mcde_pan_display(struct fb_var_screeninfo *var,
   struct fb_info *info)
{

   struct mcdefb_info *currentpar = info->par;

   if(currentpar==0) return -1;

   if (var->vmode & FB_VMODE_YWRAP) {
	   if (var->yoffset >= info->var.yres_virtual
			   || var->xoffset)
		   return -EINVAL;
   } else {
	   if (var->xoffset + var->xres > info->var.xres_virtual ||
			   var->yoffset + var->yres > info->var.yres_virtual)
		   return -EINVAL;
   }

	mcde_fb_lock(info, __func__);
   info->var.xoffset = var->xoffset;
   info->var.yoffset = var->yoffset;

   if (var->vmode & FB_VMODE_YWRAP)
	   info->var.vmode |= FB_VMODE_YWRAP;
   else
	   info->var.vmode &= ~FB_VMODE_YWRAP;

#ifdef CONFIG_FB_MCDE_MULTIBUFFER
   currentpar->clcd_event.base = info->fix.smem_start + (var->yoffset * info->fix.line_length);

   //enable next base update interrupt
   /** unmask VSYNCs of channel C0 */
   if (currentpar->chid == CHANNEL_C0)
	gpar[currentpar->chid]->regbase->mcde_imsc |= 0x4;
   if (currentpar->chid == CHANNEL_C1)
	gpar[currentpar->chid]->regbase->mcde_imsc |= 0x80;

   if (currentpar->chid == MCDE_CH_B)
	gpar[currentpar->chid]->regbase->mcde_imsc |= 0x8;

	mcde_fb_unlock(info, __func__);
    wait_event(currentpar->clcd_event.wait,currentpar->clcd_event.event==1);
    currentpar->clcd_event.event = 0;
#else
	mcde_fb_unlock(info, __func__);
#endif

   return 0;
}


/**
  Setting the video mode has been split into two parts.
  First part, xxxfb_check_var, must not write anything
  to hardware, it should only verify and adjust var.
  This means it doesn't alter par but it does use hardware
   data from it to check this var.
    */

static int mcde_check_var(struct fb_var_screeninfo *var,
		struct fb_info *info)
{
	int ret=0;
	u32 linelength;
	/** 5MB : max(lpf) * max (ppl) * (max (bpp) / 8) */
	u32 framesize = (MAX_LPF * MAX_PPL * 4);
	struct mcdefb_info *currentpar = info->par;

	/**
	 *  FB_VMODE_CONUPDATE and FB_VMODE_SMOOTH_XPAN are equal!
	 *  as FB_VMODE_SMOOTH_XPAN is only used internally
	 */
	if (var->vmode & FB_VMODE_CONUPDATE) {
		var->vmode |= FB_VMODE_YWRAP;
		var->xoffset = info->var.xoffset;
		var->yoffset = info->var.yoffset;
	}

	/**
	 *  Some very basic checks
	 */

	if ((var->xres > MAX_PPL) || (var->yres > MAX_LPF)) /** checking ppl,lpf*/
		return -EINVAL;

	if (!var->xres)
		var->xres = 1;
	if (!var->yres)
		var->yres = 1;
	if (var->xres > var->xres_virtual)
		var->xres_virtual = var->xres;
	if (var->yres > var->yres_virtual)
		var->yres_virtual = var->yres;

	if (var->xres_virtual < var->xoffset + var->xres)
		var->xres_virtual = var->xoffset + var->xres;
	if (var->yres_virtual < var->yoffset + var->yres)
		var->yres_virtual = var->yoffset + var->yres;

	/**
	 *  Memory limit
	 */
	linelength = get_line_length(var->xres, var->bits_per_pixel);
	if (linelength * var->yres_virtual > framesize)
		return -ENOMEM;


	/**
	 * Now that we checked it we alter var. The reason being is that the video
	 * mode passed in might not work but slight changes to it might make it
	 * work. This way we let the user know what is acceptable.
	 */

	memset(&(var->transp),0,sizeof(var->transp));


	switch (var->bits_per_pixel) {
		case MCDE_U8500_PANEL_8BPP:
			var->red.offset = 0;
			var->red.length = 8;
			var->green.offset = 0;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
			currentpar->actual_bpp = MCDE_PAL_8_BIT;
			break;

		case MCDE_U8500_PANEL_16BPP:

			switch (currentpar->bpp16_type) {

				case MCDE_U8500_PANEL_12BPP:    /** ARGB 0444 */
					var->red.offset = 8;
					var->red.length = 4;
					var->green.offset = 4;
					var->green.length = 4;
					var->blue.offset = 0;
					var->blue.length = 4;
					currentpar->actual_bpp = MCDE_RGB444_12_BIT;
					break;


				case MCDE_U8500_PANEL_16BPP_ARGB: /** ARGB 4444 */
					var->transp.offset = 12;
					var->transp.length = 4;
					var->red.offset = 8;
					var->red.length = 4;
					var->green.offset = 4;
					var->green.length = 4;
					var->blue.offset = 0;
					var->blue.length = 4;
					currentpar->actual_bpp = MCDE_ARGB_16_BIT;
					break;

				case MCDE_U8500_PANEL_16BPP_IRGB : /** IRGB 1555 */
					var->red.offset = 10;
					var->red.length = 5;
					var->green.offset = 5;
					var->green.length = 5;
					var->blue.offset = 0;
					var->blue.length = 5;
					var->transp.offset = 15;
					var->transp.length = 1;
					currentpar->actual_bpp = MCDE_IRGB1555_16_BIT;
					break;

				case MCDE_U8500_PANEL_16BPP_RGB: /** RGB 565 */
				default:
					var->red.offset = 11;
					var->red.length = 5;
					var->green.offset = 5;
					var->green.length = 6;
					var->blue.offset = 0;
					var->blue.length = 5;
					currentpar->actual_bpp = MCDE_RGB565_16_BIT;
					break;

			}
			break;

		case MCDE_U8500_PANEL_24BPP_PACKED:/** RGB 888 */
			var->red.offset = 16;
			var->red.length = 0;
			var->green.offset = 8;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
			var->bits_per_pixel = 24;  /**reassign to actual value*/
			currentpar->actual_bpp = MCDE_RGB_PACKED_24_BIT;
			break;

		case MCDE_U8500_PANEL_24BPP:
			var->red.offset = 16;
			var->red.length = 8;
			var->green.offset = 8;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
			currentpar->actual_bpp = MCDE_RGB_UNPACKED_24_BIT;
			break;

		case MCDE_U8500_PANEL_32BPP:/** ARGB 8888 */
			var->red.offset = 16;
			var->red.length = 8;
			var->green.offset = 8;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
			var->transp.offset = 24;
			var->transp.length = 8;
			currentpar->actual_bpp = MCDE_ARGB_32_BIT;
			break;

		case MCDE_U8500_PANEL_YCBCR:
			currentpar->actual_bpp = MCDE_YCbCr_8_BIT;
			currentpar->outbpp = MCDE_BPP_1_TO_8;
			/**YCBCR is made of 2 byte per pixel*/
			var->bits_per_pixel = 16;
			break;

		default:
			ret = -EINVAL;
			break;
	}


	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;
	return ret;
}
/**
 * mcde_mmap() - Maps the kerenel FB memory to user space memory.
 * @info: frame buffer information.
 * @vma: virtual area memory structure.
 *
 * This routine maps the kerenel FB memory to user space memory and returns the source buffer index.
 *
 */
static int mcde_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct mcdefb_info *currentpar=NULL;
	unsigned long key = vma->vm_pgoff ;
	int ret = -EINVAL;

	/** Aquire module mutex */
//	down_interruptible(&mcde_module_mutex);

	currentpar = (struct mcdefb_info *) info->par;
	if (key > ((MCDE_MAX_FRAMEBUFF*2) -1)) return -EINVAL;

		if (key < num_of_display_devices) /* for primary & secondary devices */
		key = currentpar->mcde_cur_ovl_bmp;

	vma->vm_flags |= VM_RESERVED;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	/** its a framebuffer to be mapped */
	ret = remap_pfn_range(vma,vma->vm_start,((currentpar->buffaddr[key].dmaaddr) >> PAGE_SHIFT),
			(vma->vm_end - vma->vm_start), vma->vm_page_prot) ;

	/** Release module mutex */
//	up(&mcde_module_mutex);

	return ret;
}
/**
 * find_video_mode() - To find the respective video mode from the ipnut configuration.
 * @info: frame buffer information.
 *
 * This routine updates the video mode from input configuration to currentpar->video_mode.
 */
void find_video_mode(struct fb_info *info)
{
	   struct mcdefb_info *currentpar=NULL;
	   currentpar = (struct mcdefb_info *) info->par;
	   if (strcmp(currentpar->chnl_info->restype,"SDTV")==0)
		currentpar->video_mode = VMODE_712_568_60_P;
	   else if (strcmp(currentpar->chnl_info->restype,"VGA")==0)
		currentpar->video_mode = VMODE_640_480_60_P;
	   else if (strcmp(currentpar->chnl_info->restype,"WVGA_Portrait")==0)
		currentpar->video_mode = VMODE_480_864_60_P;
	   else if (strcmp(currentpar->chnl_info->restype,"WVGA")==0)
		currentpar->video_mode = VMODE_864_480_60_P;
	   else if (strcmp(currentpar->chnl_info->restype,"HDTV")==0)
		currentpar->video_mode = VMODE_1920_1080_50_I;
	   else if (strcmp(currentpar->chnl_info->restype,"QVGA_Portrait")==0)
		currentpar->video_mode = VMODE_240_320_60_P;
	   else if (strcmp(currentpar->chnl_info->restype,"QVGA_Landscape")==0)
		currentpar->video_mode = VMODE_320_240_60_P;
	   else if (strcmp(currentpar->chnl_info->restype,"VUIB WVGA")==0)
		currentpar->video_mode = VMODE_800_600_56_P;


}
/**
 * find_restype_from_video_mode() - To find the restype from the given video mode.
 * @info: frame buffer information.
 * @videoMode: video mode.
 *
 * This routine updates the restype from the given video mode to currentpar->chnl_info->restype.
 */
void find_restype_from_video_mode(struct fb_info *info, mcde_video_mode videoMode)
{
	   struct mcdefb_info *currentpar=NULL;
	   currentpar = (struct mcdefb_info *) info->par;
	   if(videoMode == VMODE_712_568_60_P)
		currentpar->chnl_info->restype = "SDTV";
	   else if(videoMode == VMODE_640_480_60_P)
		currentpar->chnl_info->restype = "VGA";
	   else if(videoMode == VMODE_480_864_60_P)
		currentpar->chnl_info->restype = "WVGA_Portrait";
	   else if(videoMode == VMODE_864_480_60_P)
		currentpar->chnl_info->restype = "WVGA";
	   else if(videoMode == VMODE_1920_1080_50_I)
		currentpar->chnl_info->restype = "HDTV";
	   else if(videoMode == VMODE_240_320_60_P)
		currentpar->chnl_info->restype = "QVGA_Portrait";
	   else if(videoMode == VMODE_320_240_60_P)
		currentpar->chnl_info->restype = "QVGA_Landscape";
	   else if(videoMode == VMODE_800_600_56_P)
		currentpar->chnl_info->restype = "VUIB WVGA";
	   else
		currentpar->chnl_info->restype = NULL;
}

/**
 * mcde_set_video_mode() - Sets the video mode configuration as per video mode.
 * @info: frame buffer information.
 * @videoMode: video mode.
 *
 * This routine updates the info->var structure based on the video mode, allocates the memory for frame buffer
 * and calls mcde_set_par() to configure the hardware.
 *
 */
static int mcde_set_video_mode(struct fb_info *info, mcde_video_mode videoMode)
{
   struct mcde_sourcebuffer_alloc source_buff;
   struct mcdefb_info *currentpar=NULL;
   struct mcde_channel_data *channel_info;
   s32 retVal = MCDE_OK;
   u32 mcdeOverlayId;

   currentpar = (struct mcdefb_info *) info->par;
   channel_info = (struct mcde_channel_data *) info->dev->platform_data;
   /** This should give a reasonable default video mode */
   /**  use the video database to search for appropriate mode...sets info->var also */
   info->var.activate = FB_ACTIVATE_NOW;
   info->var.height = -1;
   info->var.width = -1;

   if (!fb_find_mode(&info->var, info, currentpar->chnl_info->restype, mcde_modedb, NUM_TOTAL_MODES, &mcde_modedb[videoMode], currentpar->chnl_info->inbpp))
   {
	dbgprintk(MCDE_ERROR_INFO, "mcde_set_video_mode fb_find_mode failed\n");
	info->var = mcde_default_var;
   }
#ifdef CONFIG_FB_MCDE_MULTIBUFFER
   info->var.yres_virtual	= info->var.yres*2;
#endif

   /** current monitor spec */
   info->monspecs.hfmin = 0;              /**hor freq min*/
   info->monspecs.hfmax = 100000;         /**hor freq max*/
   info->monspecs.vfmin = 0;              /**ver freq min*/
   info->monspecs.vfmax = 400;            /**ver freq max*/
   info->monspecs.dclkmin = 1000000;      /**pixclocl min */
   info->monspecs.dclkmax = 100000000;    /**pixclock max*/

   /**  allocate color map...*/
   retVal = fb_alloc_cmap(&info->cmap, 256, 0);
   if (retVal < 0)
	goto out_fbinfodealloc;

   /** allocate memory */
   source_buff.xwidth = info->var.xres;
   source_buff.yheight = info->var.yres;
   source_buff.bpp = info->var.bits_per_pixel;
#ifdef CONFIG_FB_MCDE_MULTIBUFFER
   source_buff.doubleBufferingEnabled = 1;
#else
   source_buff.doubleBufferingEnabled = 0;
#endif
   mcde_alloc_source_buffer(source_buff, info, &mcdeOverlayId, FALSE);
   if (!currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].cpuaddr) {
	dbgprintk(MCDE_ERROR_INFO, "Failed to allocate framebuffer memory\n");
	retVal=-ENOMEM;
	goto out_unmap;
   }
   memset((void *)currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].cpuaddr,0,currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].bufflength);
   info->fix.smem_start = currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr;             /** physical address of frame buffer */
   info->fix.smem_len   = currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].bufflength;

   /** update info->screen_base, info->fix.line_length to access the FB by kernel */
   info->fix.line_length =  get_line_length(info->var.xres, info->var.bits_per_pixel);
   info->screen_base = (u8 *)currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].cpuaddr;

   /** initialize the hardware as per the configuration */
   mcde_set_par_internal(info);

   return retVal;

out_unmap:
   fb_dealloc_cmap(&info->cmap);
out_fbinfodealloc:

   return retVal;
}
/**
 * mcde_get_hdmi_flag() - To give HDMI inclusion status.
 *
 * This routine gives the Flow A for HDMI is included.
 */
bool mcde_get_hdmi_flag(void)
{
	printk("mcde_get_hdmi_flag.................\n");
	return isHdmiEnabled;
}
EXPORT_SYMBOL(mcde_get_hdmi_flag);
/**
 * mcde_dsi_set_params() - To update the fb_info structure with the mcde and dsi configuration parameters for the device.
 * @info: frame buffer information.
 *
 * This routine updates the currentpar structure with the mcde and dsi configuration parameters(lowpower/highspeed)
 * and initializes the mcde,hdmi,tvout clocks
 */

static int mcde_dsi_set_params(struct fb_info *info)
{
   struct mcdefb_info *currentpar=NULL;
   int retval = MCDE_OK;

   currentpar = (struct mcdefb_info *) info->par;
   find_video_mode(info);
   switch(currentpar->chid)
   {
	case CHANNEL_A:
		isHdmiEnabled = TRUE;
		currentpar->fifoOutput = MCDE_DSI_CMD0;
		//currentpar->video_mode =  VMODE_720_576_50_P;
		currentpar->mcdeDsiChnl = MCDE_DSI_CH_CMD0;
		currentpar->output_conf = MCDE_CONF_DSI;
		if ((currentpar->fifoOutput >= MCDE_DSI_VID0) && (currentpar->fifoOutput <= MCDE_DSI_CMD2))
		{
		    currentpar->dsi_lnk_no = 2;/** This has to be changed to 1 */
		    currentpar->dsi_lnk_context.dsi_if_mode = DSI_COMMAND_MODE;
                    currentpar->dsi_lnk_context.dsiInterface = DSI_INTERFACE_2;
                    currentpar->dsi_lnk_context.if_mode_type = DSI_CLK_LANE_HSM;
                    currentpar->dsi_lnk_conf.dsiInterface = currentpar->dsi_lnk_context.dsiInterface;
		    currentpar->dsi_lnk_conf.clockContiniousMode = DSI_CLK_CONTINIOUS_HS_DISABLE;
		    currentpar->dsi_lnk_conf.dsiLinkState = DSI_ENABLE;
                    currentpar->dsi_lnk_conf.clockLaneMode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane1Mode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane2Mode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dsiInterfaceMode = currentpar->dsi_lnk_context.dsi_if_mode;
                    currentpar->dsi_lnk_conf.commandModeType = currentpar->dsi_lnk_context.if_mode_type;
			g_info = info;
                }
		break;
	case CHANNEL_B:
		PrimaryDisplayFlow = CHANNEL_B; /** for interrupt access */
		currentpar->fifoOutput = MCDE_DPI_B;
		//currentpar->video_mode =  VMODE_640_480_60_P;
		currentpar->output_conf = MCDE_CONF_DPIC0_LCDB;
		currentpar->mcdeDsiChnl = MCDE_DSI_CH_CMD0;
		if ((currentpar->fifoOutput >= MCDE_DSI_VID0) && (currentpar->fifoOutput <= MCDE_DSI_CMD2))
		{
		    currentpar->dsi_lnk_no = 0;
		    currentpar->dsi_lnk_context.dsi_if_mode = DSI_VIDEO_MODE;
                    currentpar->dsi_lnk_context.dsiInterface = DSI_INTERFACE_1;
                    currentpar->dsi_lnk_context.if_mode_type = DSI_CLK_LANE_HSM;
                    currentpar->dsi_lnk_conf.dsiInterface = currentpar->dsi_lnk_context.dsiInterface;
		    currentpar->dsi_lnk_conf.clockContiniousMode = DSI_CLK_CONTINIOUS_HS_DISABLE;
		    currentpar->dsi_lnk_conf.dsiLinkState = DSI_ENABLE;
                    currentpar->dsi_lnk_conf.clockLaneMode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane1Mode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane2Mode = DSI_LANE_DISABLE;
                    currentpar->dsi_lnk_conf.dsiInterfaceMode = currentpar->dsi_lnk_context.dsi_if_mode;
                    currentpar->dsi_lnk_conf.commandModeType = currentpar->dsi_lnk_context.if_mode_type;
                }
		break;

	case CHANNEL_C0:
		currentpar->fifoOutput = MCDE_DSI_CMD0;
		currentpar->mcdeDsiChnl = MCDE_DSI_CH_CMD0;
		currentpar->output_conf = MCDE_CONF_DSI;
		if ((currentpar->fifoOutput >= MCDE_DSI_VID0) && (currentpar->fifoOutput <= MCDE_DSI_CMD2))
		{
		    currentpar->dsi_lnk_no = 0;/** This has to be changed to 1 */
		    currentpar->dsi_lnk_context.dsi_if_mode = DSI_COMMAND_MODE;
                    currentpar->dsi_lnk_context.dsiInterface = DSI_INTERFACE_2;
                    currentpar->dsi_lnk_context.if_mode_type = DSI_CLK_LANE_HSM;
                    currentpar->dsi_lnk_conf.dsiInterface = currentpar->dsi_lnk_context.dsiInterface;
		    currentpar->dsi_lnk_conf.clockContiniousMode = DSI_CLK_CONTINIOUS_HS_DISABLE;
		    currentpar->dsi_lnk_conf.dsiLinkState = DSI_ENABLE;
                    currentpar->dsi_lnk_conf.clockLaneMode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane1Mode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane2Mode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dsiInterfaceMode = currentpar->dsi_lnk_context.dsi_if_mode;
                    currentpar->dsi_lnk_conf.commandModeType = currentpar->dsi_lnk_context.if_mode_type;

                }
		break;

	case CHANNEL_C1:
		currentpar->fifoOutput = MCDE_DSI_CMD0;
		currentpar->mcdeDsiChnl = MCDE_DSI_CH_CMD0;
		/*currentpar->video_mode =  VMODE_1920_1080_60_P;*/
		currentpar->output_conf = MCDE_CONF_DSI;
		if ((currentpar->fifoOutput >= MCDE_DSI_VID0) && (currentpar->fifoOutput <= MCDE_DSI_CMD2))
		{
		    currentpar->dsi_lnk_no = 2;
		    currentpar->dsi_lnk_context.dsi_if_mode = DSI_COMMAND_MODE;
                    currentpar->dsi_lnk_context.dsiInterface = DSI_INTERFACE_2;
                    currentpar->dsi_lnk_context.if_mode_type = DSI_CLK_LANE_HSM;
                    currentpar->dsi_lnk_conf.dsiInterface = currentpar->dsi_lnk_context.dsiInterface;
		    currentpar->dsi_lnk_conf.clockContiniousMode = DSI_CLK_CONTINIOUS_HS_DISABLE;
		    currentpar->dsi_lnk_conf.dsiLinkState = DSI_ENABLE;
                    currentpar->dsi_lnk_conf.clockLaneMode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane1Mode = DSI_LANE_ENABLE;
                    currentpar->dsi_lnk_conf.dataLane2Mode = DSI_LANE_DISABLE;
                    currentpar->dsi_lnk_conf.dsiInterfaceMode = currentpar->dsi_lnk_context.dsi_if_mode;
                    currentpar->dsi_lnk_conf.commandModeType = currentpar->dsi_lnk_context.if_mode_type;
                    currentpar->dsi_lnk_conf.hostEotGenMode = DSI_HOST_EOT_GEN_ENABLE;
                    currentpar->dsi_lnk_conf.displayEotGenMode = DSI_EOT_GEN_ENABLE;
                }
		break;
    }
    if ((currentpar->fifoOutput >= MCDE_DSI_VID0) && (currentpar->fifoOutput <= MCDE_DSI_CMD2))
    {
	/** enabling the MCDE and HDMI clocks */
        if (currentpar->dsi_lnk_context.if_mode_type == DSI_CLK_LANE_LPM)
        {
		*(currentpar->prcm_mcde_clk) = 0x125; /** MCDE CLK = 160MHZ */
		*(currentpar->prcm_hdmi_clk) = 0x145; /** HDMI CLK = 76.8MHZ */
		currentpar->clk_config.pllout_divsel2 = MCDE_PLL_OUT_OFF;
		currentpar->clk_config.pllout_divsel1 = MCDE_PLL_OUT_OFF;
		currentpar->clk_config.pllout_divsel0 = MCDE_PLL_OUT_OFF;
	        currentpar->clk_config.pll4in_sel = MCDE_CLK27;
		currentpar->clk_config.txescdiv_sel = MCDE_DSI_MCDECLK;//MCDE_DSI_CLK27;
	        currentpar->clk_config.txescdiv = 16;
	        currentpar->dsi_lnk_conf.hostEotGenMode = DSI_HOST_EOT_GEN_DISABLE;
                currentpar->dsi_lnk_conf.displayEotGenMode = DSI_EOT_GEN_DISABLE;
        }
        if (currentpar->dsi_lnk_context.if_mode_type == DSI_CLK_LANE_HSM)
        {
		*(currentpar->prcm_mcde_clk) = 0x125; /** MCDE CLK = 160MHZ */
		*(currentpar->prcm_hdmi_clk) = 0x145; /** HDMI CLK = 76.8MHZ */
		*(currentpar->prcm_tv_clk) = 0x14E; /** HDMI CLK = 76.8MHZ */


		currentpar->clk_config.pllout_divsel2 = MCDE_PLL_OUT_OFF;
		    currentpar->clk_config.pllout_divsel1 = MCDE_PLL_OUT_OFF;
		currentpar->clk_config.pllout_divsel0 =MCDE_PLL_OUT_2;
	        currentpar->clk_config.pll4in_sel = MCDE_TV1CLK;//MCDE_HDMICLK;
		currentpar->clk_config.txescdiv_sel = MCDE_DSI_MCDECLK;
	        currentpar->clk_config.txescdiv = 16;
	        currentpar->dsi_lnk_conf.hostEotGenMode = DSI_HOST_EOT_GEN_ENABLE;
                currentpar->dsi_lnk_conf.displayEotGenMode = DSI_EOT_GEN_ENABLE;

        }
    }
    return retval;
}
#ifdef TESTING
/**
 * mcde_open() - This routine opens the device.
 * @info: frame buffer information.
 * @arg: input arguements
 *
 * This routine opens the requested frame buffer device.
 *
 */
static int mcde_open(struct fb_info *info, u32 arg)
{
   struct mcdefb_info *currentpar=NULL;
   int retval = MCDE_OK;

   currentpar = (struct mcdefb_info *) info->par;
   switch(currentpar->chid)
   {
	case CHANNEL_A:
		mcde_set_video_mode(info, VMODE_640_480_60_P);
		break;
	case CHANNEL_B:
		mcde_set_video_mode(info, VMODE_640_480_60_P);
		break;

	case CHANNEL_C0:
		mcde_set_video_mode(info, VMODE_640_480_60_P);
		break;

	case CHANNEL_C1:
		mcde_set_video_mode(info, VMODE_640_480_60_P);
		break;
    }

    return retval;
}
/**
 * mcde_open() - This routine closes the device.
 * @info: frame buffer information.
 * @arg: input arguements
 *
 * This routine closes the requested frame buffer device.
 *
 */
static int mcde_release(struct fb_info *info, u32 arg)
{
	dma_free_coherent(info->dev, info->fix.smem_len, info->screen_base, info->fix.smem_start);
	fb_dealloc_cmap(&info->cmap);
	return 0;
}
#endif
static struct fb_ops mcde_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = mcde_check_var,
	.fb_set_par = mcde_set_par,
	.fb_setcolreg = mcde_setcolreg,
	.fb_ioctl = mcde_ioctl,
	.fb_mmap = mcde_mmap,
	.fb_blank = mcde_blank,
	.fb_pan_display = mcde_pan_display,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,

};


static int __init mcde_probe(struct platform_device *pdev)
{
	struct fb_info *info;
	struct mcdefb_info *currentpar=NULL;
	int retVal = -ENODEV;
	struct mcde_channel_data *channel_info;
	struct device *dev;
	struct resource *res;
	u8 irqline = 0, temp = 0;
	char __iomem *vbuffaddr;
	dma_addr_t dma;
	u8* extSrcAddr;
	u8 * ovrlaySrcAddr = NULL;
	u8 * chnlSyncAddr = NULL;
	u8 * dsiChanlRegAdd = NULL;
	u8  *dsilinkAddr = NULL;
	u32 rotationframesize = 0;
	int data_4500;

#ifdef  CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_VUIB_WVGA
	volatile u32 __iomem *clcd_clk;
#endif

    /** 5MB : max(lpf) * max (ppl) * (max (bpp) / 8)*/
#ifdef CONFIG_FB_MCDE_MULTIBUFFER
	//16Bpp and double buffering
	u32 framesize = (MAX_LPF * MAX_PPL * 2)*2;
#else
	//32 Bpp and single buffering
	u32 framesize = (MAX_LPF * MAX_PPL * 4);
#endif


	dev = &pdev->dev;
	channel_info = (struct mcde_channel_data *) dev->platform_data;

    /** To be removed I2C */
    if(!i2c_init_done)

    {

		/** Make display reset to low */

		if(platform_id==0)
		{
			/** why set directtion is not working ~ FIXME */
			//gpio_direction_output(268,1);
			//gpio_direction_output(269,1);
			gpio_set_value(268, 0);
			gpio_set_value(269, 0);
		}

		if(platform_id==1)
		{
			gpio_direction_output(282,1);
			gpio_direction_output(283,1);
			gpio_set_value(282,0);
			gpio_set_value(283,0);
		}

          mdelay(1); /** let the low value settle  */


	#ifndef CONFIG_U8500_SIM8500
			data_4500=ab8500_read(0x4,0x409);

			data_4500|=0x1;

			ab8500_write(0x4,0x409,data_4500);

			data_4500=ab8500_read(0x4,0x41F);

			data_4500|=0x8;

			ab8500_write(0x4,0x41F,data_4500);

			mdelay(1);/** let the voltage settle */

	#endif

	    /** Make display reset to high */


		if(platform_id==0)
		{
			/** why set directtion is not working ~ FIXME */
			//gpio_direction_output(268,1);
			//gpio_direction_output(269,1);
			gpio_set_value(268, 1);
			gpio_set_value(269, 1);
		}

		if(platform_id==1)
		{
			gpio_direction_output(282,1);
			gpio_direction_output(283,1);
			gpio_set_value(282,1);
			gpio_set_value(283,1);
		}

		mdelay(1); /** let the high value settle  */

	i2c_init_done=1;

    }

	/** allocate fb_info + your device specific structure */
	info = framebuffer_alloc(sizeof(struct mcdefb_info), dev);
	if (!info) {
		dbgprintk(MCDE_ERROR_INFO, "Failed to allocate framebuffer structures\n");
		retVal = -ENOMEM;
		return retVal ;
	}


	currentpar = (struct mcdefb_info *) info->par;
	currentpar->chid = channel_info->channelid;
	currentpar->outbpp = channel_info->outbpp;
	currentpar->bpp16_type = channel_info->bpp16_type;
	currentpar->bgrinput = channel_info->bgrinput;
	currentpar->chnl_info = channel_info;
	init_MUTEX(&currentpar->fb_sem);

	/** MCDE top register */
        res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL)
		goto out_fballoc;

	currentpar->regbase = (struct mcde_register_base *) ioremap(res->start, res->end - res->start + 1);
	if (currentpar->regbase == NULL)
		goto out_res;

	/** external source registers*/
        res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res == NULL)
		goto out_unmap1;

	extSrcAddr = (u8 *) ioremap(res->start, res->end - res->start + 1);
	if (extSrcAddr == NULL)
		goto out_unmap1;
	for (temp = 0; temp < NUM_EXT_SRC; temp++)
		currentpar->extsrc_regbase[temp] = (struct mcde_ext_src_reg*)(extSrcAddr + (temp*U8500_MCDE_REGISTER_SIZE));
	/** overlay registers */
        res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (res == NULL)
		goto out_unmap2;

	ovrlaySrcAddr = (u8 *) ioremap(res->start, res->end - res->start + 1);
	if (ovrlaySrcAddr == NULL)
		goto out_unmap2;
	for (temp = 0; temp < NUM_OVERLAYS; temp++)
		currentpar->ovl_regbase[temp] = (struct mcde_ovl_reg*)(ovrlaySrcAddr + (temp*U8500_MCDE_REGISTER_SIZE));
	/**  channel specific registers */

	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if (res == NULL)
		goto out_unmap3;

	chnlSyncAddr = (u8 *) ioremap(res->start, res->end - res->start + 1);
	if (chnlSyncAddr == NULL)
		goto out_unmap3;

	for (temp = 0; temp < NUM_MCDE_CHANNELS; temp++)
		currentpar->ch_regbase1[temp] = (struct mcde_ch_synch_reg*)(chnlSyncAddr + (temp*U8500_MCDE_REGISTER_SIZE));
	res = platform_get_resource(pdev, IORESOURCE_MEM, 4);
	if (res == NULL)
		goto out_unmap4;

	if (currentpar->chid == CHANNEL_A || currentpar->chid == CHANNEL_B)
	{
		currentpar->ch_regbase2[channel_info->channelid] = (struct mcde_ch_reg *) ioremap(res->start, res->end - res->start + 1);
		if (currentpar->ch_regbase2[channel_info->channelid] == NULL)
			goto out_unmap4;
	}else
	{
		if (currentpar->chid == CHANNEL_C0 || currentpar->chid == CHANNEL_C1)
		{
			currentpar->ch_c_reg = (struct mcde_chc_reg *) ioremap(res->start, res->end - res->start + 1);
			if (currentpar->ch_c_reg == NULL)
					goto out_unmap4;
		}
		if (currentpar->chid == CHANNEL_C0)
		{
			res = platform_get_resource(pdev, IORESOURCE_MEM, 10);
			currentpar->ch_regbase2[MCDE_CH_A] = (struct mcde_ch_reg *) ioremap(res->start, res->end - res->start + 1);
		}
	}
	res = platform_get_resource(pdev, IORESOURCE_MEM, 5);
	if (res == NULL)
		goto out_unmap5;

	dsiChanlRegAdd = (u8 *) ioremap(res->start, res->end - res->start + 1);
	if (dsiChanlRegAdd == NULL)
		goto out_unmap5;
	for (temp = 0; temp < NUM_DSI_CHANNEL; temp++)
		currentpar->mcde_dsi_channel_reg[temp] = (struct mcde_dsi_reg*)(dsiChanlRegAdd + (temp*U8500_MCDE_REGISTER_SIZE));

	currentpar->mcde_clkdsi = (u32*)(dsiChanlRegAdd + (U8500_MCDE_DSI_SIZE - 4));

	res = platform_get_resource(pdev, IORESOURCE_MEM, 6);
	if (res == NULL)
		goto out_unmap6;

	dsilinkAddr = (u8  *) ioremap(res->start, res->end - res->start + 1);
	if (dsilinkAddr == NULL)
		goto out_unmap6;
	for (temp = 0; temp < NUM_DSI_LINKS; temp++)
		currentpar->dsi_lnk_registers[temp] = (struct dsi_link_registers*)(dsilinkAddr + (temp*U8500_DSI_LINK_SIZE));

	res = platform_get_resource(pdev, IORESOURCE_MEM, 7);
	if (res == NULL)
		goto out_unmap7;

	currentpar->prcm_mcde_clk = (u32 *) ioremap(res->start, res->end - res->start + 1);
	if (currentpar->prcm_mcde_clk == NULL)
		goto out_unmap7;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 8);
	if (res == NULL)
		goto out_unmap8;

	currentpar->prcm_hdmi_clk = (u32 *) ioremap(res->start, res->end - res->start + 1);
	if (currentpar->prcm_hdmi_clk == NULL)
		goto out_unmap8;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 9);
	if (res == NULL)
		goto out_unmap8;

	currentpar->prcm_tv_clk = (u32 *) ioremap(res->start, res->end - res->start + 1);
	if (currentpar->prcm_tv_clk == NULL)
		goto out_unmap9;

	if (strcmp(channel_info->restype,"SDTV")==0)
		currentpar->tvout=1;
	else
		currentpar->tvout=0;



	if (!registerInterruptHandler)
	{
		/** get your IRQ line */
		res = platform_get_resource(pdev,IORESOURCE_IRQ,0);
		if (res == NULL)
			goto out_unmap10;

		irqline = res->start;

		retVal = request_irq(res->start,u8500_mcde_interrupt_handler,IRQF_SHARED,"MCDE",dev);
		if(retVal) {
			dbgprintk(MCDE_ERROR_INFO, "Failed to request IRQ line\n");
			goto out_unmap9;
		}

		registerInterruptHandler = 1;
	}

	spin_lock_init(&currentpar->clcd_event.lock);
	init_waitqueue_head(&currentpar->clcd_event.wait);
	currentpar->clcd_event.event=0;

	info->dev = dev;
	info->fbops = &mcde_ops;
	info->flags = FBINFO_FLAG_DEFAULT;
	info->pseudo_palette = currentpar->cmap;

	info->fix = mcde_fix;
#ifdef CONFIG_FB_MCDE_MULTIBUFFER
	info->fix.ypanstep = 1;
#else
	info->fix.ypanstep   = channel_info->nopan  ? 0:1;
#endif
	info->fix.ywrapstep  = channel_info->nowrap ? 0:1;

	if (currentpar->chid == CHANNEL_B )
	{

	*(currentpar->prcm_mcde_clk) = 0x125; /** MCDE CLK = 160MHZ */
	//*(currentpar->prcm_hdmi_clk) = 0x145; /** HDMI CLK = 76.8MHZ */
    *(currentpar->prcm_tv_clk) = 0x14E;     /** HDMI CLK = 76.8MHZ */

#ifdef  CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_VUIB_WVGA
	/**  clcd_clk */
	clcd_clk=(u32 *) ioremap(0x80157044, (0x80157044+3)-0x80157044 + 1);
	*clcd_clk=0x300;
	iounmap(clcd_clk);
#endif

	    //** enable the channel specific GPIO alternate functions */
		retVal = stm_gpio_altfuncenable(channel_info->gpio_alt_func);
		if (retVal)
		{
			dbgprintk(MCDE_ERROR_INFO, "Could not set MCDE GPIO alternate function correctly\n");
			/** re-valuate your request , bad pin request */
			retVal = -EADDRNOTAVAIL;
			goto out_irq;
		}


#ifndef CONFIG_U8500_SIM8500
        data_4500=ab8500_read(0x2,0x200);

        data_4500|=0x4;

        ab8500_write(0x2,0x200,data_4500);

        data_4500=ab8500_read(0x2,0x200);

        data_4500|=0x2;

		ab8500_write(0x2,0x200,data_4500);

		data_4500=ab8500_read(0x2,0x200);

		data_4500|=0x4;

		ab8500_write(0x2,0x200,data_4500);


		data_4500=ab8500_read(0x2,0x20A);

		data_4500|=0x44;

		ab8500_write(0x2,0x20A,data_4500);

		data_4500=ab8500_read(0x2,0x20C);

		data_4500|=0x3;

		ab8500_write(0x2,0x20C,data_4500);

		data_4500=ab8500_read(0x3,0x380);

		data_4500|=0x6;

		ab8500_write(0x3,0x380,data_4500);



		ab8500_write(0x6,0x600,0xA);
		ab8500_write(0x6,0x601,0x40);
		ab8500_write(0x6,0x608,0x10);
		ab8500_write(0x6,0x680,0x1);

        /** enable TV pluguin detect */
		ab8500_write(AB8500_INTERRUPT,AB8500_IT_MASK1_REG,0xF9);
#if defined(CONFIG_4500_ED)
        ab8500_set_callback_handler(AB8500_TV_PLUG_DET,tv_detect_handler,NULL);
        ab8500_set_callback_handler(AB8500_TV_UNPLUG_DET,tv_removed_handler,NULL);
#endif
	ab8500_write(0x6,0x680,0x1D);

        mdelay(1000);
#endif
	}
	gpar[currentpar->chid] = currentpar;

	/** To update the per flow params */


if (currentpar->chid == CHANNEL_C0 || currentpar->chid == CHANNEL_C1 ||  currentpar->chid == CHANNEL_B || currentpar->chid == CHANNEL_A)
{
	u32 errors_on_dsi;
	u32 self_diagnostic_result;
	bool retry;
	int n_retry = 3;
	u32 id1,id2,id3;

	if (currentpar->chid == CHANNEL_C0)
		printk(KERN_INFO "%s: Initializing DSI and display for C0\n",
			__func__);
	else if (currentpar->chid == CHANNEL_C1)
		printk(KERN_INFO "%s: Initializing DSI and display for C1\n",
			__func__);
	else
		printk(KERN_INFO "%s: Initializing DSI and display for B\n",
			__func__);

	mcde_dsi_set_params(info);
	mcde_dsi_start(info);

	printk(KERN_ERR "DSI started\n");
#ifndef PEPS_PLATFORM

	/** Make the screen up first */

	if (mcde_dsi_read_reg(info, 0x05, &errors_on_dsi) >= 0 ||
	    currentpar->chid == CHANNEL_C0)
	{
		do
		{
			mcde_dsi_write_reg(info, 0x01, 1);
			mdelay(150);

			mcde_dsi_start(info);

			/* Check that display is OK */
			mcde_dsi_read_reg(info, 0x05, &errors_on_dsi);
			mcde_dsi_read_reg(info, 0x0F, &self_diagnostic_result);

			mcde_dsi_read_reg(info, 0xDA, &id1);
			mcde_dsi_read_reg(info, 0xDB, &id2);
			mcde_dsi_read_reg(info, 0xDC, &id3);

			printk(KERN_INFO "%s: id1=%X, id2=%X, id3=%X\n",
			       __func__, id1, id2, id3);

			retry = errors_on_dsi != 0 ||
				(self_diagnostic_result != 0xC0 &&
				 self_diagnostic_result != 0x80 &&
				 self_diagnostic_result != 0x40);
			n_retry--;
			if (retry) {
				/* Oops! Pink display? */
				if (n_retry) {
					/* Display not initialized correctly,
					   try again */
					printk(
						KERN_WARNING
						"%s: Failed to initialize"
						" display, potential 'pink' "
						"display. "
					       " errors_on_dsi=%X, "
					       " self_diagnostic_result=%X, "
					       " retrying...\n", __func__,
					       errors_on_dsi,
						self_diagnostic_result);
				}
			}
			else
				printk(KERN_WARNING "%s: Display initialized OK, "
				       " errors_on_dsi=%X, "
				       " self_diagnostic_result=%X\n",
				       __func__,
				       errors_on_dsi, self_diagnostic_result);
		} while (n_retry && retry);
	}

	printk(KERN_ERR "Pink display success");

#endif
}

      /* variable to find the num of display devices initalized */
      num_of_display_devices +=1;
      mcde_set_video_mode(info, currentpar->video_mode);

	if (currentpar->chid == CHANNEL_A || currentpar->chid == CHANNEL_B)
	{
		/** allocate memory for rotation*/
		rotationframesize = info->var.xres*16*8;  /** xres * MAX_READ_BURST_SIZE WORDS */
		vbuffaddr = (char __iomem *) dma_alloc_coherent(info->dev,(rotationframesize*2),&dma,GFP_KERNEL|GFP_DMA);
		if (!vbuffaddr)
		{
			dbgprintk(MCDE_ERROR_INFO, "Unable to allocate external source memory for new framebuffer\n");
			return -ENOMEM;
		}
		currentpar->rotationbuffaddr0.cpuaddr = (u32) vbuffaddr;
		currentpar->rotationbuffaddr0.dmaaddr = dma;
		currentpar->rotationbuffaddr0.bufflength = rotationframesize;

		currentpar->rotationbuffaddr1.cpuaddr = (u32) (vbuffaddr + rotationframesize);
		currentpar->rotationbuffaddr1.dmaaddr = (dma + (rotationframesize/4));
		currentpar->rotationbuffaddr1.bufflength = framesize;
	}
	/** update info->screen_base, info->fix.line_length to access the FB by kernel */
	info->fix.line_length =  get_line_length(info->var.xres, info->var.bits_per_pixel);
	info->screen_base = (u8 *)currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].cpuaddr;

	if (register_framebuffer(info) < 0)
		goto out_fbdealloc;

	/** register_framebuffer unsets the DMA mask, but we require it set for subsequent buffer allocations */
	info->dev->coherent_dma_mask = ~0x0;
	platform_set_drvdata(pdev, info);
	printk(KERN_ERR "Display initialization done, chid = %d\n", currentpar->chid);

	return 0;

/** error handling */
out_fbdealloc:
	stm_gpio_altfuncdisable(channel_info->gpio_alt_func);
out_irq:
	free_irq(irqline,NULL);
out_unmap10:
	iounmap(currentpar->prcm_tv_clk);
out_unmap9:
	iounmap(currentpar->prcm_hdmi_clk);
out_unmap8:
	iounmap(currentpar->prcm_mcde_clk);

out_unmap7:
	iounmap(currentpar->dsi_lnk_registers[0]);

out_unmap6:
	iounmap(currentpar->mcde_dsi_channel_reg[0]);
out_unmap5:
	if (currentpar->chid == CHANNEL_A || currentpar->chid == CHANNEL_B)
		iounmap(currentpar->ch_regbase2[currentpar->chid]);
	else if (currentpar->chid == CHANNEL_C0 || currentpar->chid == CHANNEL_C1)
	{
		iounmap(currentpar->ch_c_reg);
	}
out_unmap4:
	iounmap(currentpar->ch_regbase1[0]);
out_unmap3:
	iounmap(currentpar->ovl_regbase[0]);
out_unmap2:
	iounmap(currentpar->extsrc_regbase[0]);
out_unmap1:
	iounmap(currentpar->regbase);
out_res:
	platform_device_put(pdev);

out_fballoc:
	framebuffer_release(info);
	return retVal;

}

void  mcde_test(struct fb_info *info)
{
	struct mcdefb_info *currentpar=
		(struct mcdefb_info *) info->par;

	mcde_fb_lock(info, __func__);

	gpio_set_value(268, 0);
	gpio_set_value(269, 0);

	mdelay(10);

	gpio_set_value(268, 1);
	gpio_set_value(269, 1);

	mdelay(10); /** let the high value settle  */

	mdelay(200); /* Must wait at least 120 ms after reset */

	if (currentpar->chid == CHANNEL_C0 || currentpar->chid == CHANNEL_C1 ||  currentpar->chid == CHANNEL_B)
	{
		u32 errors_on_dsi;
		u32 self_diagnostic_result;
		bool retry;
		int n_retry = 3;
		u32 id1,id2,id3;

		mcde_dsi_set_params(info);

#ifndef PEPS_PLATFORM
		do
		{
			/** Make the screen up first */

			currentpar->dsi_lnk_registers[DSI_LINK2]->
				mctl_main_en=0x0;
			mcde_dsi_start(info);

			/* Check that display is OK */
			mcde_dsi_read_reg(info, 0x05, &errors_on_dsi);
			mcde_dsi_read_reg(info, 0x0F, &self_diagnostic_result);

			mcde_dsi_read_reg(info, 0xDA, &id1);
			mcde_dsi_read_reg(info, 0xDB, &id2);
			mcde_dsi_read_reg(info, 0xDC, &id3);

			printk(KERN_INFO "%s: id1=%X, id2=%X, id3=%X\n",
			       __func__, id1, id2, id3);

			retry = errors_on_dsi != 0 ||
				(self_diagnostic_result != 0xC0 &&
				 self_diagnostic_result != 0x80 &&
				 self_diagnostic_result != 0x40);
			n_retry--;
			if (retry) {
				/* Oops! Pink display? */
				if (n_retry) {
					/* Display not initialized correctly,
					   try again */
					printk(KERN_WARNING "%s: Failed to initialize"
					       " display, potential 'pink' display. "
					       " errors_on_dsi=%X, "
					       " self_diagnostic_result=%X, "
					       " retrying...\n", __func__,
					       errors_on_dsi, self_diagnostic_result);
					mcde_dsi_write_reg(info, 0x01, 1);
				}
			}
			else
				printk(KERN_WARNING "%s: Display initialized OK, "
				       " errors_on_dsi=%X, "
				       " self_diagnostic_result=%X\n",
				       __func__,
				       errors_on_dsi, self_diagnostic_result);
		} while (n_retry && retry);
#endif
	}


	/** initialize the hardware as per the configuration */
	mcde_set_par_internal(info);

	mcde_fb_unlock(info, __func__);
}

/**
 * mcde_remove() - This routine de-initializes and de-register the FB device.
 * @pdev: platform device.
 *
 * This routine removes the frame buffer device .This will be invoked for every
 * frame buffer device de-initialization.
 *
 */
int mcde_remove(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);
	struct mcdefb_info *currentpar;
	struct device *dev = &pdev->dev;
	struct mcde_channel_data *channel_info;
	u8 temp;

	/** Aquire module mutex */
//	down_interruptible(&mcde_module_mutex);

	if (info) {
		currentpar = (struct mcdefb_info *) info->par;
		channel_info = (struct mcde_channel_data *) dev->platform_data;
		if (registerInterruptHandler)
		{
			struct resource *res = platform_get_resource(pdev,IORESOURCE_IRQ,0);
			free_irq(res->start,NULL);
			registerInterruptHandler = 0;
		}

		for (temp=0;temp<MCDE_MAX_FRAMEBUFF;temp++)
		{
			if (mcde_ovl_bmp & (1 << temp))
				mcde_extsrc_ovl_remove(info,temp);
		}
		for (temp=MCDE_MAX_FRAMEBUFF;temp<2*MCDE_MAX_FRAMEBUFF;temp++)
		{
			if (mcde_ovl_bmp & (1 << temp))
				mcde_dealloc_source_buffer(info, temp, FALSE);
		}

		platform_set_drvdata(pdev,NULL);
		mcdefb_disable(info);
		unregister_framebuffer(info);
		stm_gpio_altfuncdisable(channel_info->gpio_alt_func);
		iounmap(currentpar->regbase);
		iounmap(currentpar->extsrc_regbase[0]);
		iounmap(currentpar->ovl_regbase[0]);
		iounmap(currentpar->ch_regbase1[0]);
		if (currentpar->chid == CHANNEL_A || currentpar->chid == CHANNEL_B)
			iounmap(currentpar->ch_regbase2[currentpar->chid]);
		else if (currentpar->chid == CHANNEL_C0 || currentpar->chid == CHANNEL_C1)
		{
			iounmap(currentpar->ch_c_reg);
		}
		iounmap(currentpar->dsi_lnk_registers[0]);
		iounmap(currentpar->mcde_dsi_channel_reg[0]);
		framebuffer_release(info);
	}
	/** Release module mutex */
//	up(&mcde_module_mutex);

	return 0;
}


#ifdef CONFIG_PM
/**
 * u8500_mcde_suspend() - This routine puts the FB device in to sustend state.
 * @pdev: platform device.
 *
 * This routine stores the current state of the frame buffer device and puts in to suspend state.
 *
 */
int u8500_mcde_suspend(struct platform_device *pdev, pm_message_t state)
{
	dbgprintk(MCDE_DEBUG_INFO, "mop_mcde_suspend: called...\n");
	return 0;
}
/**
 * u8500_mcde_resume() - This routine resumes the FB device from sustend state.
 * @pdev: platform device.
 *
 * This routine restore back the current state of the frame buffer device resumes.
 *
 */

int u8500_mcde_resume(struct platform_device *pdev)
{
	dbgprintk(MCDE_DEBUG_INFO, "mop_mcde_resume: called...\n");
	return 0;
}

#else

#define u8500_mcde_suspend NULL
#define u8500_mcde_resume NULL

#endif

struct platform_driver mcde_driver = {
	.probe	= mcde_probe,
	.remove = mcde_remove,
	.driver = {
		.name	= "U8500-MCDE",
	},
        /** TODO implement power mgmt functions */
        .suspend = u8500_mcde_suspend,
        .resume = u8500_mcde_resume,
};
/**
 * mcde_init() - This routine inserts the driver.
 * @pdev: platform device.
 *
 * This module is built as a static module, platform driver init will be called mcde_init to load mcde module.
 *
 */
static int __init mcde_init(void)
{
	return platform_driver_register(&mcde_driver);
}
module_init(mcde_init);
/**
 * mcde_exit() - This routine exits the driver.
 * @pdev: platform device.
 *
 */
static void __exit mcde_exit(void)
{
	platform_driver_unregister(&mcde_driver);
	return;
}
module_exit(mcde_exit);


#ifdef _cplusplus
}
#endif /* _cplusplus */

#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */
