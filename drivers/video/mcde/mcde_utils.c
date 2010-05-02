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


#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/mcde_common.h>
extern struct mcdefb_info *gpar[];

/** bitmap of which overlays are in use (set) and unused (cleared) */
u32 mcde_ovl_bmp;
/** color conversion coefficients */
u32 img_rgb_2_ycbcr[COLCONV_COEFF_OFF]={0x00420081,0x00190070,0x03A203EE,0x03DA03B5,0x00700080,0x00100080};
u32 img_ycbcr_2_rgb[COLCONV_COEFF_OFF]={0x0330012A,0x039C0199,0x012A0000,0x0000012A,0x01990321,0x00870115};

/** get x line length word aligned, and in bytes */
/*static inline */unsigned long get_line_length(int x, int bpp)
{
	return (unsigned long)((((x*bpp)+31)&~31) >> 3);
}

unsigned long claim_mcde_lock(mcde_ch_id chid)
{
	unsigned long flags;
	spin_lock_irqsave(&(gpar[chid]->mcde_spin_lock), flags);
	return flags;
}

void release_mcde_lock(mcde_ch_id chid, unsigned long flags)
{
	spin_unlock_irqrestore(&(gpar[chid]->mcde_spin_lock), flags);
}
int convertbpp(u8 bpp)
{
	int hw_bpp = 0;

	switch (bpp)
	{
		case MCDE_U8500_PANEL_1BPP:
			hw_bpp = MCDE_PAL_1_BIT;
			break;

		case MCDE_U8500_PANEL_2BPP:
			hw_bpp = MCDE_PAL_2_BIT;
			break;

		case MCDE_U8500_PANEL_4BPP:
			hw_bpp = MCDE_PAL_4_BIT;
			break;

		case MCDE_U8500_PANEL_8BPP:
			hw_bpp = MCDE_PAL_8_BIT;
			break;

		case MCDE_U8500_PANEL_16BPP:
			hw_bpp = MCDE_RGB565_16_BIT;
			break;

		case MCDE_U8500_PANEL_24BPP_PACKED:
			hw_bpp = MCDE_RGB_PACKED_24_BIT;
			break;

		case MCDE_U8500_PANEL_24BPP:
			hw_bpp = MCDE_RGB_UNPACKED_24_BIT;
			break;

		case MCDE_U8500_PANEL_32BPP:
			hw_bpp = MCDE_ARGB_32_BIT;
			break;

		default:
			hw_bpp =  -EINVAL;
	}

	return hw_bpp;
}
/** channel specific enable*/
void mcdefb_enable(struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;

	/** ---- turn on FLOWEN for this channel */
	mcdesetchnlXflowmode(currentpar->chid, MCDE_FLOW_ENABLE);

	/** ---- turn on POWEREN for this channel */
	mcdesetchnlXpowermode(currentpar->chid, MCDE_POWER_ENABLE);
}

/** channel specific disable */
void mcdefb_disable(struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;

	/** ---- turn off POWEREN for this channel */
	mcdesetchnlXpowermode(currentpar->chid, MCDE_POWER_DISABLE);

	/** ---- turn off FLOWEN for this channel */
	mcdesetchnlXflowmode(currentpar->chid, MCDE_FLOW_DISABLE);

}
int mcde_enable(struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	u32 retVal=0;

	/** ---- enable MCDE */
	mcdesetstate(currentpar->chid, MCDE_ENABLE);
	gpar[currentpar->chid]->regbase->mcde_cr |= 0x80000000;
	mcdefb_enable(info);
	return retVal;
}
int mcde_disable(struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	u32 retVal=0;

	/** ---- enable MCDE */
	mcdesetstate(currentpar->chid, MCDE_DISABLE);
	mcdefb_disable(info);
	return retVal;
}

int  mcde_alloc_source_buffer(struct mcde_sourcebuffer_alloc source_buff ,struct fb_info *info, u32 *pkey, u8 isUserRequest)
{
	u32 xorig = source_buff.xwidth;
	u32 yorig = source_buff.yheight;
	u8   input_bpp = source_buff.bpp;
	u32 framesize=0;
	u32 retVal = 0;
	u8 __iomem *vbuffaddr;
	dma_addr_t dma;
	u32  line_length;
	u8 buffidx;
	struct mcdefb_info *currentpar = info->par;

	/** calculate the line jump for getting the frame size */
	if (input_bpp == MCDE_YCbCr_8_BIT)
		line_length = xorig * 2;
	else
		line_length = get_line_length(input_bpp,xorig);

	if (source_buff.doubleBufferingEnabled)
		framesize = (line_length * yorig) * 2;
	else
		framesize = (line_length * yorig);

	for (buffidx = isUserRequest*MCDE_MAX_FRAMEBUFF; buffidx < (MCDE_MAX_FRAMEBUFF + (isUserRequest*MCDE_MAX_FRAMEBUFF)); buffidx++)
	{
		if (!(mcde_ovl_bmp & (1 << buffidx)))
			break;
	}
	if (buffidx == (MCDE_MAX_FRAMEBUFF + (isUserRequest*MCDE_MAX_FRAMEBUFF)))
	{
		dbgprintk(MCDE_ERROR_INFO, "Unable to allocate memory for FB, as it reached max FB\n");
		return -EINVAL;
	}
	/** allocate memory */
	vbuffaddr = (char __iomem *) dma_alloc_coherent(info->dev,framesize,&dma,GFP_KERNEL|GFP_DMA);
	if (!vbuffaddr)
	{
		dbgprintk(MCDE_ERROR_INFO, "Unable to allocate external source memory for new framebuffer\n");
		return -ENOMEM;
	}

	mcde_ovl_bmp |= (1 << (buffidx));
	*pkey = buffidx;

	currentpar->buffaddr[buffidx].cpuaddr = (u32) vbuffaddr;
	currentpar->buffaddr[buffidx].dmaaddr = dma;
	currentpar->buffaddr[buffidx].bufflength = framesize;

	if (isUserRequest == TRUE)
	{
		source_buff.buff_addr.dmaaddr = dma;
		source_buff.buff_addr.bufflength = framesize;
	}else if (currentpar->tot_ovl_used < MCDE_MAX_FRAMEBUFF)
	{
		currentpar->tot_ovl_used++;
		currentpar->mcde_cur_ovl_bmp = buffidx;
		currentpar->mcde_ovl_bmp_arr[currentpar->tot_ovl_used -1] = currentpar->mcde_cur_ovl_bmp;
	}

	return retVal;
}
int  mcde_dealloc_source_buffer(struct fb_info *info, u32 srcbufferindex, u8 isUserRequest)
{
	u32 retVal = MCDE_OK;
	struct mcdefb_info *currentpar = info->par;

	if (!(mcde_ovl_bmp & (1 << srcbufferindex)))
	{
		dbgprintk(MCDE_ERROR_INFO, "mcde_dealloc_source_buffer: Not a valid source buffer ID\n");
		return -EINVAL;
	}

	mcde_ovl_bmp &= ~(1 << srcbufferindex);
	if (currentpar->buffaddr[srcbufferindex].dmaaddr != 0x0)
		dma_free_coherent(info->dev, currentpar->buffaddr[srcbufferindex].bufflength, (void*)currentpar->buffaddr[srcbufferindex].cpuaddr, currentpar->buffaddr[srcbufferindex].dmaaddr);
	currentpar->buffaddr[srcbufferindex].cpuaddr = 0x0 ;
	currentpar->buffaddr[srcbufferindex].dmaaddr = 0x0;
	currentpar->buffaddr[srcbufferindex].bufflength = 0x0;
	if (isUserRequest == FALSE && (currentpar->tot_ovl_used > 0 && currentpar->tot_ovl_used < MCDE_MAX_FRAMEBUFF))
	{
		currentpar->tot_ovl_used--;
		currentpar->mcde_cur_ovl_bmp = currentpar->mcde_ovl_bmp_arr[currentpar->tot_ovl_used - 1];
		currentpar->mcde_ovl_bmp_arr[currentpar->tot_ovl_used] = -1;
	}
	return retVal;
}

int  mcde_conf_extsource(struct mcde_ext_conf ext_src_config ,struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	struct mcde_ext_src_ctrl control;
	u32 retVal = 0;

	/** requested overlay id is not equal to the current overlay id */
	if(ext_src_config.provr_id != currentpar->mcde_cur_ovl_bmp)
	{
		dbgprintk(MCDE_ERROR_INFO, "mcde_conf_extsource: Not a valid overlay ID\n");
		return -1;
	}

	/**---- ext src config registers */
	mcdesetextsrcconf(currentpar->chid, currentpar->mcde_cur_ovl_bmp, ext_src_config);

	control.fs_ctrl = MCDE_FS_FREQ_UNCHANGED;
	control.fs_div = MCDE_FS_FREQ_DIV_ENABLE;
	control.ovr_ctrl = MCDE_MULTI_CH_CTRL_PRIMARY_OVR;
	control.sel_mode = MCDE_BUFFER_SOFTWARE_SELECT;
	/**---- ext src control registers */
	mcdesetextsrcctrl(currentpar->chid, currentpar->mcde_cur_ovl_bmp, control);
	/** set the source buffer address */
	mcdesetbufferaddr(currentpar->chid, currentpar->mcde_cur_ovl_bmp, ext_src_config.buf_id, currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr);

	return retVal;
}

int  mcde_conf_channel_color_key(struct fb_info *info, struct mcde_channel_color_key chnannel_color_key)
{
	struct mcdefb_info *currentpar = info->par;
	int retVal = MCDE_OK;

	mcdesetcolorkey(currentpar->chid, chnannel_color_key.color_key, chnannel_color_key.color_key_type);
	mcdesetcflowXcolorkeyctrl(currentpar->chid, chnannel_color_key.key_ctrl);
	return retVal;
}
int  mcde_conf_blend_ctrl(struct fb_info *info, struct mcde_blend_control blend_ctrl)
{
	struct mcdefb_info *currentpar = info->par;
	struct mcde_ovr_comp ovr_comp;
	struct mcde_ovr_conf2 ovr_conf2;
	int retVal = MCDE_OK;

	ovr_conf2.alpha_value = blend_ctrl.ovr1_blend_ctrl.alpha_value;
	ovr_conf2.ovr_blend = blend_ctrl.ovr1_blend_ctrl.ovr_blend;
	ovr_conf2.ovr_opaq = blend_ctrl.ovr1_blend_ctrl.ovr_opaq;
	ovr_conf2.pixoff = 0;
	ovr_conf2.watermark_level = 0x20;
	mcdesetovrconf2(currentpar->chid, blend_ctrl.ovr1_id, ovr_conf2);
	ovr_comp.ch_id = currentpar->chid;
	ovr_comp.ovr_xpos = blend_ctrl.ovr1_blend_ctrl.ovr_xpos;
	ovr_comp.ovr_ypos = blend_ctrl.ovr1_blend_ctrl.ovr_ypos;
	ovr_comp.ovr_zlevel = blend_ctrl.ovr1_blend_ctrl.ovr_zlevel;
	mcdesetovrcomp(currentpar->chid, blend_ctrl.ovr1_id, ovr_comp);

	if (blend_ctrl.ovr2_enable)
	{
	ovr_conf2.alpha_value = blend_ctrl.ovr2_blend_ctrl.alpha_value;
	ovr_conf2.ovr_blend = blend_ctrl.ovr2_blend_ctrl.ovr_blend;
	ovr_conf2.ovr_opaq = blend_ctrl.ovr2_blend_ctrl.ovr_opaq;
	ovr_conf2.pixoff = 0;
	ovr_conf2.watermark_level = 0x20;
	mcdesetovrconf2(currentpar->chid, blend_ctrl.ovr2_id, ovr_conf2);

	ovr_comp.ch_id = currentpar->chid;
	ovr_comp.ovr_xpos = blend_ctrl.ovr2_blend_ctrl.ovr_xpos;
	ovr_comp.ovr_ypos = blend_ctrl.ovr2_blend_ctrl.ovr_ypos;
	ovr_comp.ovr_zlevel = blend_ctrl.ovr2_blend_ctrl.ovr_zlevel;
	mcdesetovrcomp(currentpar->chid, blend_ctrl.ovr2_id, ovr_comp);
	}
	mcdesetblendctrl(currentpar->chid, blend_ctrl);
	return retVal;
}
int  mcde_conf_rotation(struct fb_info *info, mcde_rot_dir rot_dir, mcde_roten rot_ctrl, u32 rot_addr0, u32 rot_addr1)
{
	struct mcdefb_info *currentpar = info->par;
	int retVal = MCDE_OK;
	mcdesetrotation(currentpar->chid, rot_dir, rot_ctrl);
	mcdesetrotaddr(currentpar->chid, rot_addr0, MCDE_ROTATE0);
	mcdesetrotaddr(currentpar->chid, rot_addr1, MCDE_ROTATE1);
	return retVal;
}
int  mcde_set_buffer(struct fb_info *info, u32 buffer_address, mcde_buffer_id buff_id)
{
	struct mcdefb_info *currentpar = info->par;
	int retVal = MCDE_OK;
	mcdesetbufferaddr(currentpar->chid, currentpar->mcde_cur_ovl_bmp, buff_id, buffer_address);
	return retVal;
}
int  mcde_conf_color_conversion(struct fb_info *info, struct mcde_conf_color_conv color_conv_ctrl)
{
	struct mcdefb_info *currentpar = info->par;
	struct mcde_chx_rgb_conv_coef convCoef = {0};
	int retVal = MCDE_OK;

	if(color_conv_ctrl.convert_format == COLOR_CONV_NONE)
	  return retVal;

	if (color_conv_ctrl.convert_format == COLOR_CONV_YUV_RGB)
	{
		/** write on the RGB conversion registers */
		convCoef.Yr_red = 0x330;
		convCoef.Yr_green = 0x12A;
		convCoef.Yr_blue = 0x39C;
		convCoef.Cr_red = 0x199;
		convCoef.Cr_green = 0x12A;
		convCoef.Cr_blue = 0x0;
		convCoef.Cb_red = 0x0;
		convCoef.Cb_green = 0x12A;
		convCoef.Cb_blue = 0x199;
		convCoef.Off_red = 0x321;
		convCoef.Off_green = 0x87;
		convCoef.Off_blue = 0x115;
	} else if (color_conv_ctrl.convert_format == COLOR_CONV_RGB_YUV)
	{
		/** write on the RGB conversion registers */
		convCoef.Yr_red = 0x42;
		convCoef.Yr_green = 0x81;
		convCoef.Yr_blue = 0x19;
		convCoef.Cr_red = 0x70;
		convCoef.Cr_green = 0x3A2;
		convCoef.Cr_blue = 0x3EE;
		convCoef.Cb_red = 0x3DA;
		convCoef.Cb_green = 0x3B5;
		convCoef.Cb_blue = 0x70;
		convCoef.Off_red = 0x80;
		convCoef.Off_green = 0x10;
		convCoef.Off_blue = 0x80;
	} else if (color_conv_ctrl.convert_format == COLOR_CONV_YUV422_YUV444)
	{
		convCoef.Yr_red = 263;
		convCoef.Yr_green = 516;
		convCoef.Yr_blue = 100;
		convCoef.Cr_red = 450;
		convCoef.Cr_green = -377;
		convCoef.Cr_blue = -73;
		convCoef.Cb_red = -152;
		convCoef.Cb_green = -298;
		convCoef.Cb_blue = 450;
		convCoef.Off_red = 128;
		convCoef.Off_green = 16;
		convCoef.Off_blue = 128;
	}
	mcdesetcolorconvmatrix(currentpar->chid, convCoef);
	mcdesetcolorconvctrl(currentpar->chid, currentpar->mcde_cur_ovl_bmp, color_conv_ctrl.col_ctrl);

	return retVal;
}
int  mcde_conf_overlay(struct mcde_conf_overlay ovrlayConfig ,struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	struct mcde_ovr_control ovr_cr;
	struct mcde_ovr_config ovr_conf;
	struct mcde_ovr_conf2 ovr_conf2;
	//struct mcde_ovr_clip ovr_clip;
	struct mcde_ovr_comp ovr_comp;
	struct mcde_conf_color_conv color_conv_ctrl;
	u32  lineJump;
	u32 retVal = 0;

	/** SET OVERLAY REGISTERS */
	ovr_cr.ovr_state = ovrlayConfig.ovr_state;
	ovr_cr.col_ctrl = ovrlayConfig.col_ctrl;
	ovr_cr.pal_control = ovrlayConfig.pal_control;
	ovr_cr.priority = ovrlayConfig.priority;
	ovr_cr.color_key = ovrlayConfig.color_key;
	ovr_cr.rot_burst_req = ovrlayConfig.rot_burst_req;
	ovr_cr.burst_req = ovrlayConfig.burst_req;
	ovr_cr.outstnd_req = ovrlayConfig.outstnd_req;
	ovr_cr.alpha = ovrlayConfig.alpha;
	ovr_cr.clip = ovrlayConfig.clip;

	/**---- overlay ctrl registers */
	mcdesetovrctrl(currentpar->chid, currentpar->mcde_cur_ovl_bmp, ovr_cr);

	ovr_conf.line_per_frame = ovrlayConfig.yheight;//info->var.yres;
	ovr_conf.ovr_ppl = ovrlayConfig.xwidth;//info->var.xres;
	ovr_conf.src_id = currentpar->mcde_cur_ovl_bmp;

	/**---- overlay config registers */
	mcdesetovrlayconf(currentpar->chid, currentpar->mcde_cur_ovl_bmp, ovr_conf);

	ovr_conf2.alpha_value = ovrlayConfig.alpha_value;
	ovr_conf2.ovr_blend = ovrlayConfig.ovr_blend;
	ovr_conf2.ovr_opaq = ovrlayConfig.ovr_opaq;
	ovr_conf2.pixoff = ovrlayConfig.pixoff;
	ovr_conf2.watermark_level = ovrlayConfig.watermark_level;

	mcdesetovrconf2(currentpar->chid, currentpar->mcde_cur_ovl_bmp, ovr_conf2);

	if (ovrlayConfig.bpp == MCDE_YCbCr_8_BIT)
		lineJump = info->var.xres * 2;
	else
		lineJump = get_line_length(ovrlayConfig.bpp,ovr_conf.ovr_ppl);

	mcdesetovrljinc(currentpar->chid, currentpar->mcde_cur_ovl_bmp, lineJump);

	/**---- overlay composition registers */
	/** base overlay is always in background */

	ovr_comp.ch_id = currentpar->chid;
	ovr_comp.ovr_xpos = ovrlayConfig.ovr_xpos;
	ovr_comp.ovr_ypos = ovrlayConfig.ovr_ypos;
	ovr_comp.ovr_zlevel = ovrlayConfig.ovr_zlevel;
	mcdesetovrcomp(currentpar->chid, currentpar->mcde_cur_ovl_bmp, ovr_comp);

	color_conv_ctrl.convert_format = ovrlayConfig.convert_format;
	color_conv_ctrl.convert_format = ovrlayConfig.col_ctrl;
	mcde_conf_color_conversion(info, color_conv_ctrl);

	return retVal;
}
int  mcde_extsrc_ovl_create(struct mcde_overlay_create *extsrc_ovl ,struct fb_info *info, u32 *pkey)
{
	struct mcde_sourcebuffer_alloc source_buff;
	struct mcde_conf_overlay ovrlayConfig;
	struct mcde_ext_conf config;
	struct mcde_conf_color_conv color_conv_ctrl;
	struct mcdefb_info *currentpar = info->par;
	u32 mcdeOverlayId;
	s32  hw_bpp = 0;
	u32 flags;
	u8 ovl_fore = extsrc_ovl->fg;
	mcde_colorconv_type convert_format = COLOR_CONV_NONE;

	if (extsrc_ovl->bpp == MCDE_YCbCr_8_BIT)
	{
		if (!(currentpar->tvout)) convert_format = COLOR_CONV_YUV_RGB;
		hw_bpp = MCDE_YCbCr_8_BIT;
	}
	else
		hw_bpp = convertbpp(extsrc_ovl->bpp);

	if (hw_bpp < 0) return -EINVAL;

	if (hw_bpp <= MCDE_PAL_8_BIT)
		info->fix.visual = FB_VISUAL_PSEUDOCOLOR;

	/**cant be bigger than the base overlay*/
	if ((extsrc_ovl->xwidth > info->var.xres) || (extsrc_ovl->yheight > info->var.yres))
		return -EINVAL;
	if (extsrc_ovl->usedefault == 0)
	{
	  /** allocate memory */
	  source_buff.xwidth = extsrc_ovl->xwidth;
	  source_buff.yheight = extsrc_ovl->yheight;
	  source_buff.bpp = extsrc_ovl->bpp;
	  source_buff.doubleBufferingEnabled = 0;
	  flags = claim_mcde_lock(currentpar->chid);
	  mcde_alloc_source_buffer(source_buff, info, &mcdeOverlayId, FALSE);
	  release_mcde_lock(currentpar->chid, flags);
	  *pkey = mcdeOverlayId;
	} else
	  *pkey = currentpar->mcde_cur_ovl_bmp;

	/** SET EXTERNAL SRC REGISTERS */
	config.ovr_pxlorder = MCDE_PIXEL_ORDER_LITTLE;
	config.endianity = MCDE_BYTE_LITTLE;
	config.rgb_format = currentpar->bgrinput;
	config.bpp = (u32)hw_bpp;
	config.provr_id = currentpar->mcde_cur_ovl_bmp;
	config.buf_num = MCDE_BUFFER_USED_1;
	config.buf_id = MCDE_BUFFER_ID_0;
	mcde_conf_extsource(config ,info);

	/** SET OVERLAY REGISTERS */
	ovrlayConfig.ovr_state = MCDE_OVERLAY_ENABLE;
	if (convert_format == COLOR_CONV_YUV_RGB)
		ovrlayConfig.col_ctrl = MCDE_COL_CONV_NOT_SAT;
	else
		ovrlayConfig.col_ctrl = MCDE_COL_CONV_DISABLE;
	if (currentpar->actual_bpp <= MCDE_PAL_8_BIT)
		ovrlayConfig.pal_control = MCDE_PAL_ENABLE;
	else
		ovrlayConfig.pal_control = MCDE_PAL_GAMA_DISABLE;
	ovrlayConfig.priority = 0x0;
	ovrlayConfig.color_key = MCDE_COLOR_KEY_DISABLE;
	ovrlayConfig.rot_burst_req = MCDE_ROTATE_BURST_WORD_4;
	ovrlayConfig.burst_req = MCDE_BURST_WORD_HW_8;
	ovrlayConfig.outstnd_req = MCDE_OUTSTND_REQ_4;
	ovrlayConfig.alpha = MCDE_OVR_PREMULTIPLIED_ALPHA_DISABLE;
	ovrlayConfig.clip = MCDE_OVR_CLIP_DISABLE;
	ovrlayConfig.alpha_value = 0x64;
	ovrlayConfig.ovr_blend = MCDE_CONST_ALPHA_SOURCE;
	ovrlayConfig.ovr_opaq = MCDE_OVR_OPAQUE_ENABLE;
	ovrlayConfig.pixoff = 0x0;
	ovrlayConfig.watermark_level = 0x20;
	ovrlayConfig.xbrcoor = 0x0;
	ovrlayConfig.xtlcoor = 0x0;
	ovrlayConfig.ybrcoor = 0x0;
	ovrlayConfig.ytlcoor = 0x0;
	ovrlayConfig.ovr_xpos = extsrc_ovl->xorig;
	ovrlayConfig.ovr_ypos = extsrc_ovl->yorig;
	ovrlayConfig.ovr_zlevel = ovl_fore;
	ovrlayConfig.xwidth = extsrc_ovl->xwidth;
	ovrlayConfig.yheight = extsrc_ovl->yheight;
	ovrlayConfig.bpp = extsrc_ovl->bpp;
	mcde_conf_overlay(ovrlayConfig, info);

	color_conv_ctrl.convert_format = convert_format;
	color_conv_ctrl.convert_format = ovrlayConfig.col_ctrl;
	mcde_conf_color_conversion(info, color_conv_ctrl);

	return 0;
}

int mcde_extsrc_ovl_remove(struct fb_info *info,u32 key)
{
	struct mcdefb_info *currentpar = info->par;
	unsigned long flags;
	u32 retVal = MCDE_OK;

	/** SPINLOCK in use : deal with the global variables now */
	flags = claim_mcde_lock(currentpar->chid);

	if ((key != currentpar->mcde_cur_ovl_bmp) || (currentpar->tot_ovl_used == 1))
	{
		dbgprintk(MCDE_ERROR_INFO, "Overlay not in use for this channel\n");
		release_mcde_lock(currentpar->chid, flags);
		return -EINVAL;
	}
	/** reset ext src and ovrlay registers to default value */
	mcderesetextsrcovrlay(currentpar->chid);
#ifdef TESTING
	currentpar->tot_ovl_used--;
	//mcde_ovl_bmp--;
	mcde_ovl_bmp &= ~(1 << key);
	dma_free_coherent(info->dev, currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].bufflength, currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].cpuaddr, currentpar->buffaddr[currentpar->mcde_cur_ovl_bmp].dmaaddr);

	currentpar->mcde_cur_ovl_bmp = currentpar->mcde_ovl_bmp_arr[currentpar->tot_ovl_used - 1];

	currentpar->buffaddr[key].cpuaddr = 0x0 ;
	currentpar->buffaddr[key].dmaaddr = 0x0;
	currentpar->buffaddr[key].bufflength = 0x0;
	currentpar->mcde_ovl_bmp_arr[currentpar->tot_ovl_used] = -1;
#endif
        retVal = mcde_dealloc_source_buffer(info, key, FALSE);
        release_mcde_lock(currentpar->chid, flags);
	return retVal;
}
int  mcde_conf_channel(struct mcde_ch_conf ch_config ,struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	struct mcde_chsyncmod sync_mod;
	u32 retVal = 0;

	if (currentpar->chid == CHANNEL_A || currentpar->chid == CHANNEL_B || currentpar->chid == CHANNEL_C0 || currentpar->chid == CHANNEL_C1)
	{
		sync_mod.out_synch_interface = ch_config.out_synch_interface;
		sync_mod.ch_synch_src = ch_config.ch_synch_src;
		mcdesetchnlsyncsrc(currentpar->chid, currentpar->chid, sync_mod);
		mcdesetchnlXconf(currentpar->chid, currentpar->chid, ch_config.chconfig);
		mcdesetswsync(currentpar->chid, currentpar->chid, ch_config.sw_trig);
		mcdesetchnlbckgndcol(currentpar->chid, currentpar->chid, ch_config.chbckgrndcolor);
		mcdesetchnlsyncprio(currentpar->chid, currentpar->chid, ch_config.ch_priority);
		if (sync_mod.out_synch_interface == MCDE_SYNCHRO_OUTPUT_SOURCE)
		{
			mcdesetchnlsyncevent(currentpar->chid, ch_config);
		}
		mcdesetpanelctrl(currentpar->chid, ch_config.control1);
	}

	return retVal;
}
/** ------------------------------------------------------------------------
   FUNCTION : mcde_conf_lcd_timing
   PURPOSE  : To configure the parameters for LCD configuration
   ------------------------------------------------------------------------ */
int  mcde_conf_lcd_timing(struct mcde_chnl_lcd_ctrl chnl_lcd_ctrl, struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	struct mcde_chx_lcd_timing0 lcdcontrol0;
	struct mcde_chx_lcd_timing1 lcdcontrol1;
	u32 retVal = 0;

	mcdesetchnlLCDctrlreg(currentpar->chid, chnl_lcd_ctrl.lcd_ctrl_reg);
	mcdesetchnlLCDhorizontaltiming(currentpar->chid, chnl_lcd_ctrl.lcd_horizontal_timing);
	mcdesetchnlLCDverticaltiming(currentpar->chid, chnl_lcd_ctrl.lcd_vertical_timing);

	lcdcontrol0.ps_delay0 = 0x0;
	lcdcontrol0.ps_delay1 = 0x0;
	lcdcontrol0.ps_sync_sel = 0x0;
	lcdcontrol0.ps_toggle_enable = 0x0;
	lcdcontrol0.ps_va_enable = 0x0;
	lcdcontrol0.rev_delay0 = 0x0;
	lcdcontrol0.rev_delay1 = 0x0;
	lcdcontrol0.rev_sync_sel = 0x0;
	lcdcontrol0.rev_toggle_enable = 0x0;
	lcdcontrol0.rev_va_enable = 0x0;
	mcdesetLCDtiming0ctrl(currentpar->chid, lcdcontrol0);

	lcdcontrol1.iclrev = 0x0;
	lcdcontrol1.iclsp = 0x0;
	lcdcontrol1.iclspl = 0x0;
	lcdcontrol1.ihs = 0x1;
	lcdcontrol1.io_enable = 0x0;
	lcdcontrol1.ipc = 0x0;
	lcdcontrol1.ivp = 0x0;
	lcdcontrol1.ivs = 0x1;
	lcdcontrol1.mcde_spl = 0x0;
	lcdcontrol1.spltgen = 0x0;
	lcdcontrol1.spl_delay0 = 0x0;
	lcdcontrol1.spl_delay1 = 0x0;
	lcdcontrol1.spl_sync_sel = 0x0;
	mcdesetLCDtiming1ctrl(currentpar->chid, lcdcontrol1);
	return retVal;
}
int  mcde_conf_chnlc(struct mcde_chc_config chnlc_config, struct mcde_chc_ctrl chnlc_control, struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	mcde_chc_panel panel_id;
	u32 retVal = MCDE_OK;

	if (currentpar->chid == MCDE_CH_C0)
		panel_id = MCDE_PANEL_C0;
	else if (currentpar->chid == MCDE_CH_C1)
		panel_id = MCDE_PANEL_C1;
	else return MCDE_INVALID_PARAMETER;

#if 0
	mcdesetchnlCconf(currentpar->chid, panel_id, chnlc_config);
	mcdesetchnlCctrl(currentpar->chid, chnlc_control);
#endif
	currentpar->ch_c_reg->mcde_chc_crc = 0x1d7d0017;//0x387b0027;
	/** Add any Channel "C' extra configuration */

	return retVal;

}
int  mcde_conf_dsi_chnl(mcde_dsi_conf dsi_conf, mcde_dsi_clk_config clk_config, struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	u8 cmdbyte_lsb;
	u8 cmdbyte_msb;
#if 0
	u16 sync_dma;
	u16 sync_sw;
#endif
	u32 retVal = 0;
	u16 packet_size = 1+info->var.xres*info->var.bits_per_pixel/8;

	cmdbyte_lsb = TPO_CMD_RAMWR_CONTINUE;
	cmdbyte_msb = TPO_CMD_NONE;

	mcdesetdsicommandword( currentpar->dsi_lnk_no, currentpar->chid, currentpar->mcdeDsiChnl, cmdbyte_lsb, cmdbyte_msb);

	mcdesetdsiclk(currentpar->dsi_lnk_no, currentpar->chid, clk_config);
#if 0
	mcdesetdsisyncpulse(currentpar->dsi_lnk_no, currentpar->chid, currentpar->mcdeDsiChnl, sync_dma, sync_sw);
#endif
	dsi_conf.blanking = 0;
	dsi_conf.vid_mode = currentpar->dsi_lnk_context.dsi_if_mode;
	dsi_conf.cmd8 = MCDE_DSI_CMD_8;
	dsi_conf.bit_swap = MCDE_DSI_NO_SWAP;
	dsi_conf.byte_swap = MCDE_DSI_NO_SWAP;
	dsi_conf.synchro = MCDE_DSI_OUT_VIDEO_DCS;
	dsi_conf.packing = MCDE_PACKING_RGB888_R;
	dsi_conf.words_per_frame = info->var.yres*packet_size;
	dsi_conf.words_per_packet = packet_size;
	mcdesetdsiconf(currentpar->dsi_lnk_no, currentpar->chid, currentpar->mcdeDsiChnl, dsi_conf);

	return retVal;
}
/** ------------------------------------------------------------------------
   FUNCTION : mcde_conf_dithering_ctrl
   PURPOSE  : To configure the parameters for dithering
   ------------------------------------------------------------------------ */
int  mcde_conf_dithering_ctrl(struct mcde_dithering_ctrl_conf dithering_ctrl_conf, struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	u32 retVal = MCDE_OK;

	retVal = mcdesetditherctrl(currentpar->chid, dithering_ctrl_conf.mcde_chx_dither_ctrl);
	retVal = mcdesetditheroffset(currentpar->chid, dithering_ctrl_conf.mcde_chx_dithering_offset);
	mcdesetditheringctrl(currentpar->chid, dithering_ctrl_conf.dithering_ctrl);

	return retVal;
}

/**
 * mcde_conf_scan_mode() - This routine configures the TV scan mode(progressive/Interlaced).
 * @scan_mode: Progressive/Interlaced.
 * @info: frame buffer information.
 *
 *
 */
int  mcde_conf_scan_mode(mcde_scan_mode scan_mode, struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	u32 retVal = MCDE_OK;

	retVal = mcdesetscanmode(currentpar->chid, scan_mode);
	if (retVal == MCDE_OK)
		info->var.vmode = scan_mode;
	return retVal;
}

#ifdef _cplusplus
}
#endif /* _cplusplus */



