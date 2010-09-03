/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License terms:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef _MCDE_COMMON_H_
#define _MCDE_COMMON_H_
#include <linux/fb.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>

#include <mach/bit_mask.h>
#include <mach/mcde.h>
#include <mach/mcde_reg.h>
#include <mach/dsi.h>
#include <mach/dsi_reg.h>

#define MCDE_NAME      "DRIVER MCDE"

#define MCDE_DEFAULT_LOG_LEVEL 3
#ifdef CONFIG_FB_NDK_MCDE
extern int mcde_debug;
module_param(mcde_debug, int, 0644);
#endif
#ifdef CONFIG_FB_NDK_MCDE
MODULE_PARM_DESC(mcde_debug,"Debug level for messages");
#define dbgprintk(num, format, args...) \
        do { \
                if(num >= mcde_debug ) \
                printk("MCDE:"format, ##args); \
        } while(0)
#else
#define dbgprintk(num, format, args...) \
        do { \
                printk("MCDE:"format, ##args); \
        } while(0)
#endif

#define MCDE_DEBUG_INFO	1
#define MCDE_ERROR_INFO 3

/** Global data */

#define MAX_LPF 1080
#define MAX_PPL 1920
#define NUM_MCDE_FLOWS 4
#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT
#define NUM_OVERLAYS	6
#define NUM_EXT_SRC	10
#define NUM_MCDE_CHANNELS	4
#else
#define NUM_OVERLAYS	16
#define NUM_EXT_SRC	16
#define NUM_MCDE_CHANNELS	16
#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */
#define NUM_FLOWS_A_B	2
#define NUM_DSI_LINKS	3
#define NUM_DSI_CHANNEL	6
#define MCDE_MAX_FRAMEBUFF 16
#define COLCONV_COEFF_OFF 6

/** clcd events for double buffering */
struct clcd_event_struct {
  spinlock_t lock;
  int event;
  wait_queue_head_t wait;
  unsigned int base;
};

/** bitmap of which overlays are in use (set) and unused (cleared) */
//static u32 mcde_ovl_bmp;

/** MCDE private struct - pointer available in fbinfo */
struct mcdefb_info {
	struct mcde_channel_data *chnl_info;
	mcde_ch_id chid;
	mcde_video_mode video_mode;
	mcde_fifo_output fifoOutput;
	mcde_output_conf output_conf;
	mcde_dsi_clk_config clk_config;
	mcde_ch_id     dsi_formatter_plugged_channel[NUM_DSI_CHANNEL];
	mcde_dsi_channel mcdeDsiChnl;
	mcde_out_bpp outbpp;
	int bpp16_type;
	u8 bgrinput;
	u8 isHwInitalized;
	u16 palette_size;
	u32 cmap[16];

	mcde_ch_id pixel_pipeline;
	u32 vcomp_irq;
	u32 dsi_formatter;
	u32 dsi_mode;
	u32 swap;
	u32 started;

	/** phy-virtual addresses allocated for framebuffer and overlays */
	struct mcde_addrmap buffaddr[MCDE_MAX_FRAMEBUFF*2];
	struct mcde_addrmap rotationbuffaddr0;
	struct mcde_addrmap rotationbuffaddr1;

	/** total number of overlays used in the system */
	u8 tot_ovl_used;

	/** bitmap of which overlays are in use (set) and unused (cleared) */
	u16 mcde_cur_ovl_bmp;
	u16 mcde_ovl_bmp_arr[MCDE_MAX_FRAMEBUFF];
	spinlock_t mcde_spin_lock;

	/** event for double buffering */
	struct clcd_event_struct clcd_event;
	u8 tvout;
	u32 actual_bpp;
#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT
	struct mcde_top_reg __iomem       *regbase;
#else
	struct mcde_register_base __iomem * regbase;
#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */
	struct mcde_ext_src_reg __iomem * extsrc_regbase[NUM_EXT_SRC];
	struct mcde_ovl_reg __iomem * ovl_regbase[NUM_OVERLAYS];
#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT
	struct mcde_chnl_conf_reg __iomem *ch_regbase1[NUM_MCDE_CHANNELS];
	struct mcde_chAB_reg __iomem      *ch_regbase2[NUM_FLOWS_A_B];
	struct mcde_chC0C1_reg __iomem    *ch_c_reg;
#else
	struct mcde_ch_synch_reg __iomem *ch_regbase1[NUM_MCDE_CHANNELS];
	struct mcde_ch_reg __iomem *ch_regbase2[NUM_FLOWS_A_B];
	struct mcde_chc_reg __iomem *ch_c_reg;
#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */
	struct mcde_dsi_reg __iomem *mcde_dsi_channel_reg[NUM_DSI_CHANNEL];
	volatile u32 __iomem *mcde_clkdsi;
	struct dsi_link_registers __iomem *dsi_lnk_registers[NUM_DSI_LINKS];
	volatile u32 __iomem *prcm_mcde_clk;
	volatile u32 __iomem *prcm_hdmi_clk;
	volatile u32 __iomem *prcm_tv_clk;
	dsi_link dsi_lnk_no;
	dsi_link_context dsi_lnk_context;
	struct dsi_link_conf dsi_lnk_conf;

	/* Added by QCSPWAN below for "pink display" */
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_dentry;
	char debugfs_name[10];
#endif
	/* To serialize access from user space */
	struct semaphore fb_sem;
	/* End QCSPWAN */
};


//volatile mcde_system_context  g_mcde_system_context;

#define NUM_TOTAL_MODES ARRAY_SIZE(mcde_modedb)

mcde_error mcdesetdsiclk(dsi_link link, mcde_ch_id chid, mcde_dsi_clk_config clk_config);
mcde_error mcdesetfifoctrl(dsi_link link, mcde_ch_id chid, struct mcde_fifo_ctrl fifo_ctrl);
mcde_error mcdesetoutputconf(dsi_link link, mcde_ch_id chid, mcde_output_conf output_conf);
mcde_error mcdesetdsicommandword(dsi_link link,mcde_ch_id chid,mcde_dsi_channel dsichannel,u8 cmdbyte_lsb,u8 cmdbyte_msb);
mcde_error mcdesetdsiconf(dsi_link link, mcde_ch_id chid, mcde_dsi_channel dsichannel, mcde_dsi_conf dsi_conf);
#endif
