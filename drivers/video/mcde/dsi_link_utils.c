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
#include <linux/delay.h>
#include <mach/mcde_common.h>

extern struct mcdefb_info *gpar[];
#ifdef TESTING
char * pbitmapData; /** to be removed */
#endif


dsi_error dsiPllConf(dsi_pll_ctl pll_ctl, mcde_ch_id chid, dsi_link link)
{
	dsi_error retVal = DSI_OK;
	u32 waitTimeout = 0xFFFFFFFF;
	u8  pll_sts;

	/** Clock Selection and enabling */
	dsisetPLLcontrol(link, chid, pll_ctl);
	dsisetPLLmode(link, chid, DSI_PLL_START);

	/** Wait Till  pll LOCKS */
	waitTimeout = 0xFFFF;
	dsigetlinkstatus(link, chid, &pll_sts);
	while ( !(pll_sts & DSI_PLL_LOCK) && (waitTimeout > 0) )
	{
		waitTimeout-- ;
		dsigetlinkstatus(link, chid, &pll_sts);
	}
	if (waitTimeout == 0)
	{
		dbgprintk(MCDE_ERROR_INFO, "dsiPLLconf:PLL lOCK FAILED!!!!\n");
		retVal = DSI_PLL_PROGRAM_ERROR;
	}
	return retVal;
}
dsi_error dsiLinkConf (struct dsi_link_conf *pdsiLinkConf, mcde_ch_id chid, dsi_link link)
{
	dsi_error retVal = DSI_OK;
	dsi_interface dsiInterface;
	dsi_if_state dsiInterfaceState; /** interface enable/disable */
	dsi_te_en tearing;
	switch (pdsiLinkConf->dsiInterfaceMode)
	{
		case DSI_VIDEO_MODE:
			dsiInterface = DSI_INTERFACE_1;
			dsiInterfaceState = DSI_IF_ENABLE;
			/** enable the interface */
			retVal = dsisetInterface(link, chid, dsiInterfaceState, dsiInterface);
			/** set the interface1 mode (VideoMode/Command mode */
			retVal = dsisetInterface1mode(link, DSI_VIDEO_MODE, chid);
			/** set the interface in LP/HS mode */
			dsisetInterfaceInLpm(link, chid, pdsiLinkConf->videoModeType, dsiInterface);
		break;
		case DSI_COMMAND_MODE:
			if (pdsiLinkConf->dsiInterface == DSI_INTERFACE_1)
			{
				dsiInterface = DSI_INTERFACE_1;
				dsiInterfaceState = DSI_IF_ENABLE;
				/** enable the interface */
				retVal = dsisetInterface(link, chid, dsiInterfaceState, dsiInterface);
				/** set the interface1 mode (VideoMode/Command mode */
				retVal = dsisetInterface1mode(link, DSI_COMMAND_MODE, chid);
				/** set the interface in LP/HS mode */
				dsisetInterfaceInLpm(link, chid, pdsiLinkConf->commandModeType, dsiInterface);
			}else
			{
				dsiInterface = DSI_INTERFACE_2;
				dsiInterfaceState = DSI_IF_ENABLE;
				/** enable the interface */
				retVal = dsisetInterface(link, chid, dsiInterfaceState, dsiInterface);
				/** set the interface in LP/HS mode */
				dsisetInterfaceInLpm(link, chid, pdsiLinkConf->commandModeType, dsiInterface);
			}
			break;
		case DSI_INTERFACE_BOTH:
			dsiInterface = DSI_INTERFACE_1;
			dsiInterfaceState = DSI_IF_ENABLE;
			/** enable the interface */
			retVal = dsisetInterface(link, chid, dsiInterfaceState, dsiInterface);
			/** set the interface1 mode (VideoMode/Command mode */
			retVal = dsisetInterface1mode(link, DSI_VIDEO_MODE, chid);
			/** set the interface in LP/HS mode */
			dsisetInterfaceInLpm(link, chid, pdsiLinkConf->videoModeType, dsiInterface);

			dsiInterface = DSI_INTERFACE_2;
			/** enable the interface */
			retVal = dsisetInterface(link, chid, dsiInterfaceState, dsiInterface);
			/** set the interface in LP/HS mode */
			dsisetInterfaceInLpm(link, chid, pdsiLinkConf->commandModeType, dsiInterface);
		break;
		case DSI_INTERFACE_NONE:
			dsiInterface = DSI_INTERFACE_1;
			dsiInterfaceState = DSI_IF_DISABLE;
			/** disable the interface */
			retVal = dsisetInterface(link, chid, dsiInterfaceState, dsiInterface);
			dsiInterface = DSI_INTERFACE_2;
			/** disable the interface */
			retVal = dsisetInterface(link, chid, dsiInterfaceState, dsiInterface);

		break;
		default:
			retVal = DSI_INVALID_PARAMETER;
			break;
	}

        /** configuring the TE values for If1 */
	tearing.te_sel = DSI_IF_TE;
	tearing.interface = DSI_INTERFACE_1;
	dsisetTE(link, tearing, pdsiLinkConf->if1TeCtrl, chid);

	/** configuring the TE values for If2 */
	tearing.te_sel = DSI_IF_TE;
	tearing.interface = DSI_INTERFACE_2;
	dsisetTE(link, tearing, pdsiLinkConf->if2TeCtrl, chid);

	/** configuring the TE values for reg type */
	tearing.te_sel = DSI_REG_TE;
	dsisetTE(link, tearing, pdsiLinkConf->regTeCtrl, chid);

	/** configuring read,BTA,EOTGen,HostEOTGen,CheckSumGen,Continious clock and padding values */
	dsireadset(link, pdsiLinkConf->rdCtrl, chid);
	dsisetBTAmode(link, pdsiLinkConf->btaMode, chid);
	dsisetdispEOTGenmode(link, pdsiLinkConf->displayEotGenMode, chid);
	dsisetdispHOSTEOTGenmode(link, pdsiLinkConf->hostEotGenMode, chid);
	dsisetdispCHKSUMGenmode(link, pdsiLinkConf->dispChecksumGenMode, chid);
	dsisetdispECCGenmode(link, pdsiLinkConf->dispEccGenMode, chid);
	dsisetCLKHSsendingmode(link, pdsiLinkConf->clockContiniousMode, chid);
	dsisetpaddingval(link, chid, pdsiLinkConf->paddingValue);

	return retVal;
}
dsi_error dsiLinkEnable (struct dsi_link_conf *pdsiLinkConf, mcde_ch_id chid, dsi_link link)
{
	dsi_error retVal = DSI_OK;
	u32 waitTimeout = 0xFFFFFFFF;
	u8  link_sts;

	/** Enable the DSI link, Link Clock and the Data Lane */
	dsisetlinkstate(link, pdsiLinkConf->dsiLinkState, chid);
	dsisetlanestate(link, chid, pdsiLinkConf->clockLaneMode, DSI_CLK_LANE);
	dsisetlanestate(link, chid, pdsiLinkConf->dataLane1Mode, DSI_DATA_LANE1);

	if (pdsiLinkConf->clockLaneMode == DSI_LANE_ENABLE)
	{
	  /** Wait Till  clock lane are ready */
	  waitTimeout = 0xFFFFFFFF;
	  dsigetlinkstatus(link, chid, &link_sts);
	  while ( !(link_sts & DSI_CLKLANE_READY) && (waitTimeout > 0) )
	  {
		waitTimeout-- ;
		dsigetlinkstatus(link, chid, &link_sts);
	  }
	  if (waitTimeout == 0)
	  {
		retVal = DSI_CLOCK_LANE_NOT_READY;
		dbgprintk(MCDE_ERROR_INFO, "dsiLinkConf:DSI  Clock Lane not ready :...... FAILED!!!!\n");
	  }
	}
	/** Wait Till  data1 lane are ready */
	if (pdsiLinkConf->clockLaneMode == DSI_LANE_ENABLE)
	{
	  waitTimeout = 0xFFFFFFFF;
	  dsigetlinkstatus(link, chid, &link_sts);
	  while ( !(link_sts & DSI_DAT1_READY) && (waitTimeout > 0) )
	  {
		waitTimeout-- ;
		dsigetlinkstatus(link, chid, &link_sts);
	  }
	  if (waitTimeout == 0)
	  {
		retVal = DSI_DATA_LANE1_NOT_READY;
		dbgprintk(MCDE_ERROR_INFO, "dsiLinkConf:DSI  Data Lane1 not ready :...... FAILED!!!!\n");
	  }
	}
	if(pdsiLinkConf->dataLane2Mode == DSI_LANE_ENABLE)
	{
		dsisetlanestate(link, chid, pdsiLinkConf->dataLane2Mode, DSI_DATA_LANE2);
		/*************** Wait Till  DATA1  lane are ready ********/
		//waitTimeout = 0xFFFF;
		dsigetlinkstatus(link, chid, &link_sts);
		while ( !(link_sts & DSI_DAT2_READY) && (waitTimeout > 0) )
		{
		waitTimeout-- ;
		dsigetlinkstatus(link, chid, &link_sts);
		}
		if (waitTimeout == 0)
		{
			retVal = DSI_DATA_LANE2_NOT_READY;
			dbgprintk(MCDE_ERROR_INFO, "dsiLinkConf:DSI  Data Lane2 not ready :...... FAILED!!!!\n");
		}
	}
	return retVal;
}

u32 dsiLinkInit(struct dsi_link_conf *pdsiLinkConf, struct dsi_dphy_static_conf dphyStaticConf, mcde_ch_id chid, dsi_link link)
{
    dsi_pll_ctl         pll_ctl;

    pll_ctl.multiplier = 0x0;
    pll_ctl.division_ratio  = 0x0;

    pll_ctl.pll_in_sel = DSI_PLL_IN_CLK_27;

    if (pdsiLinkConf->commandModeType == DSI_CLK_LANE_HSM || pdsiLinkConf->videoModeType == DSI_CLK_LANE_HSM)
    {
	pll_ctl.pll_master = DSI_PLL_SLAVE;
		pll_ctl.pll_out_sel = DSI_INTERNAL_PLL;
    }
	else
    {
	pll_ctl.pll_master = DSI_PLL_MASTER;
		pll_ctl.pll_out_sel = DSI_SYSTEM_PLL;
    }

    /*Main enable-start enable*/
    dsiLinkConf (pdsiLinkConf,chid, link);
    dsiLinkEnable (pdsiLinkConf, chid, link);

    dsiset_hs_clock(link, dphyStaticConf, chid);
    dsiPllConf(pll_ctl, chid, link);

    return(0);

}

/**
 Initialization of TPO Display using LP mode and DSI direct command registor  & dispaly of two alternating color
 */
int mcde_dsi_test_dsi_HS_directcommand_mode(struct fb_info *info,u32 key)
{
  struct dsi_dphy_static_conf dphyStaticConf;
  struct dsi_link_conf dsiLinkConf;
  struct mcdefb_info *currentpar = info->par;
  unsigned int retVal= 0;
  unsigned int Field = 1;
  int num, temp;
  int No_Loop;
  unsigned char PixelRed;
  unsigned char PixelGreen;
  unsigned char PixelBlue;
  int pixel_nb ;


  dsiLinkConf.dsiInterfaceMode = DSI_INTERFACE_NONE;
  dsiLinkConf.clockContiniousMode = DSI_CLK_CONTINIOUS_HS_DISABLE;
  dsiLinkConf.dsiLinkState = DSI_ENABLE;
  dsiLinkConf.clockLaneMode = DSI_LANE_ENABLE;
  dsiLinkConf.dataLane1Mode = DSI_LANE_ENABLE;
  dsiLinkConf.dataLane2Mode = DSI_LANE_DISABLE;
  dphyStaticConf.datalane1swappinmode = HS_SWAP_PIN_DISABLE;
  dsiLinkConf.commandModeType = DSI_CLK_LANE_HSM;
  dphyStaticConf.ui_x4 = 0;
  /*** To configure the dsi clock and enable the clock lane and data1 lane */
  dsiLinkInit(&dsiLinkConf, dphyStaticConf, currentpar->chid, currentpar->dsi_lnk_no);

  mcde_dsi_taaldisplay_init(info);

  num=0;
  No_Loop=10;
  PixelRed=0x00;
  PixelGreen=0xFF;
  PixelBlue=0x00;
  pixel_nb = info->var.xres*info->var.yres;
  /** The Screen is filled with one color by LP commands, in the next loop the color changes */

  while(num < No_Loop)//Color Changess in each loop
  {

	retVal = dsiHSdcslongwrite(VC_ID0, 4, TPO_CMD_RAMWR,  PixelRed,PixelGreen, PixelBlue,0,0,0,0,0,0,0,0,0,0,0,0, currentpar->chid, currentpar->dsi_lnk_no);
	retVal = dsiHSdcslongwrite(VC_ID0, 4, TPO_CMD_RAMWR_CONTINUE,  PixelRed,PixelGreen, PixelBlue,
								PixelRed, PixelGreen,PixelBlue,
								PixelRed,	PixelGreen,PixelBlue,
								PixelRed,PixelGreen,PixelBlue,
								0,0,0, currentpar->chid, currentpar->dsi_lnk_no);
	for (temp=4; temp< (pixel_nb);temp= temp +4)
	{
	  retVal = dsiHSdcslongwrite(VC_ID0, 13, TPO_CMD_RAMWR_CONTINUE,  PixelRed,PixelGreen, PixelBlue,
								PixelRed, PixelGreen,PixelBlue,
								PixelRed,	PixelGreen,PixelBlue,
								PixelRed,PixelGreen,PixelBlue,
								0,0,0, currentpar->chid, currentpar->dsi_lnk_no);
	}
	for (temp=0;temp<1000;temp++);//delay

	if(Field == 1)  /** odd field */
	{
		Field=0; /** set even field */
		PixelRed = 0xFF;
		PixelGreen	=0x00;
		PixelBlue =0x00;
	}
	else if(Field == 0) /** even field */
	{
		Field=1; /** set odd field */
		PixelRed=0x00;
		PixelGreen=0xFF;
		PixelBlue=0x00;
	}


	num++;
  }
return retVal;
}

int mcde_dsi_test_LP_directcommand_mode(struct fb_info *info,u32 key)
{
  struct dsi_dphy_static_conf dphyStaticConf;
  struct dsi_link_conf dsiLinkConf;
  struct mcdefb_info *currentpar = info->par;
  //unsigned long flags;
  unsigned int retVal= 0;
  unsigned int Field = 1;
  //int uTimeout = 0xFFFFFFFF;
  int num, temp;
  int No_Loop;
  unsigned char PixelRed;
  unsigned char PixelGreen;
  unsigned char PixelBlue;
  int pixel_nb ;

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

  /** INITIALISE DSI(TPO) DISPLAY to run in direct command mode */
 // dsidisplayinitLPcmdmode(currentpar->chid, currentpar->dsi_lnk_no);
//  mcde_dsi_tpodisplay_init(info);

  mcde_dsi_taaldisplay_init(info);
  dbgprintk(MCDE_DEBUG_INFO, "Writing Pixel For Display to the Display Buffer in LP Command Mode\n");

  num=0;
  No_Loop=10;

  PixelRed=0x00;
  PixelGreen=0xFF;
  PixelBlue=0x00;
  pixel_nb = info->var.xres*info->var.yres;
  /** The Screen is filled with one color by LP commands, in the next loop the color changes */

  while(num < No_Loop)//Color Changess in each loop
  {
    retVal= dsiLPdcslongwrite(VC_ID0, 4, TPO_CMD_RAMWR,  PixelRed,PixelGreen, PixelBlue,0,0,0,0,0,0,0,0,0,0,0,0, currentpar->chid, currentpar->dsi_lnk_no);
    retVal = dsiLPdcslongwrite(VC_ID0, 4, TPO_CMD_RAMWR_CONTINUE,  PixelRed,PixelGreen, PixelBlue,
				PixelRed, PixelGreen,PixelBlue,
				PixelRed,	PixelGreen,PixelBlue,
				PixelRed,PixelGreen,PixelBlue,
				0,0,0, currentpar->chid, currentpar->dsi_lnk_no);
    for (temp=4; temp< (pixel_nb);temp= temp +4)
    {
	retVal = dsiLPdcslongwrite(VC_ID0, 13, TPO_CMD_RAMWR_CONTINUE,  PixelRed,PixelGreen, PixelBlue,
					PixelRed, PixelGreen,PixelBlue,
					PixelRed,	PixelGreen,PixelBlue,
					PixelRed,PixelGreen,PixelBlue,
					0,0,0, currentpar->chid, currentpar->dsi_lnk_no);
    }
    for (temp=0;temp<1000;temp++);//delay

    if(Field == 1)  /** odd field */
    {
	Field=0; /** set even field */
	PixelRed = 0xFF;
	PixelGreen	=0x00;
	PixelBlue =0x00;
    }
    else if(Field == 0) /** even field */
    {
	Field=1; /** set odd field */
	PixelRed=0x00;
	PixelGreen=0xFF;
	PixelBlue=0x00;
    }
    num++;
  }
  return retVal;
}

void mcde_dsi_taaldisplay_init(struct fb_info *info)
{
	struct mcdefb_info *currentpar = info->par;
	int timeout;	volatile u32 __iomem *mcde_dsi_clk;

	if(currentpar->chid==MCDE_CH_C0) {
		//currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_data_ctl|=0x380;

		/* Link enable */
		printk(KERN_ERR "mctl_main_data_ctl\n");
		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_data_ctl=0x1;
		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_phy_ctl=0x5;

		//currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_data_ctl|=0x380;

		/* PLL start */
		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_en=0x438;

		mdelay(100);

		/**  mcde dsi clock */
		//mcde_dsi_clk=(u32 *) ioremap(0xA0350EF0, (0xA0350EF0+3)-0xA0350EF0 + 1);
		//*mcde_dsi_clk=0xA1010C;
		//iounmap(mcde_dsi_clk);

		mdelay(100);

		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_pll_ctl=0x00000001;//0x10000;
		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_en=0x439;

		/* wait for lanes ready */
		timeout=0xFFFF;
		while(!((currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_main_sts) & 0xE) && (timeout > 0))
		{
			timeout--;
		}
		if(timeout == 0) {
			printk(KERN_INFO "DSI CLK LANE DAT1 DAT2 NOT READY\n");
		}
		else {
			printk(KERN_INFO "DSI CLK LANE DAT1 DAT2 READY %x\n", timeout);
		}

		currentpar->dsi_lnk_registers[DSI_LINK0]->cmd_mode_ctl=0x3FF0040;
		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_dphy_static=0x300;//0x3c0;
		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_dphy_timeout=0xffffffff;
		currentpar->dsi_lnk_registers[DSI_LINK0]->mctl_ulpout_time=0x201;

		mdelay(100);


		/** send DSI commands to make display up */

		/** make the display up ~ send commands */

		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_main_settings=0x210500;
		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_wrdat0=0x11;
		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_send=0x1;

		mdelay(150); /** sleep for 150 ms */

		/** send teaing command with low power mode */

		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_main_settings=0x221500;
		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_wrdat0=0x35;
		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_send=0x1;

		mdelay(100);

		/** send color mode */

		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_main_settings=0x221500;
		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_wrdat0=0xf73a;
		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_send=0x1;

		mdelay(100);

		/** send power on command */
		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_main_settings=0x210500;
		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_wrdat0=0xF729;
		currentpar->dsi_lnk_registers[DSI_LINK0]->direct_cmd_send=0x1;

		mdelay(100); /** check for display to up ok ~ primary display */
	}

	if(currentpar->chid==MCDE_CH_C1) {
		if(currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_en!=0x9) {
			mcde_dsi_clk=(u32 *) ioremap(0xA0350EF0, (0xA0350EF0+3)-0xA0350EF0 + 1);
			*mcde_dsi_clk =0xA1010C;
			iounmap(mcde_dsi_clk);

			mdelay(100);

			currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_data_ctl |=0x1;
			currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_phy_ctl|=0x1;
#ifndef CONFIG_FB_MCDE_MULTIBUFFER
			currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_pll_ctl |=0x104A0;//0x104A2;
#else
			currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_pll_ctl |=0x1049E;//0x104A2;
#endif
			currentpar->dsi_lnk_registers[DSI_LINK2]->mctl_main_en |=0x9;
	    }

		mdelay(100);

		currentpar->dsi_lnk_registers[DSI_LINK1]->mctl_main_data_ctl|=0x380;

		currentpar->dsi_lnk_registers[DSI_LINK1]->mctl_main_data_ctl=0x1;
		currentpar->dsi_lnk_registers[DSI_LINK1]->mctl_main_phy_ctl=0x5;

		//currentpar->dsi_lnk_registers[DSI_LINK1]->mctl_main_data_ctl|=0x380;//0x380;

		currentpar->dsi_lnk_registers[DSI_LINK1]->mctl_main_en=0x438;

		mdelay(100);

		currentpar->dsi_lnk_registers[DSI_LINK1]->mctl_pll_ctl=0x10000;
		currentpar->dsi_lnk_registers[DSI_LINK1]->mctl_main_en=0x439;

		mdelay(100); /** check for pll lock */

		currentpar->dsi_lnk_registers[DSI_LINK1]->cmd_mode_ctl=0x3FF0040;
		currentpar->dsi_lnk_registers[DSI_LINK1]->mctl_dphy_static=0x3c0;
		currentpar->dsi_lnk_registers[DSI_LINK1]->mctl_dphy_timeout=0xffffffff;
		currentpar->dsi_lnk_registers[DSI_LINK1]->mctl_ulpout_time=0x201;

		mdelay(100);

		/** make the display up ~ send commands */

		/** sleep on */

		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_main_settings=0x210500;
		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_wrdat0=0x11;
		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_send=0x1;

	    mdelay(150); /** sleep for 150 ms */

		/** send teaing command with low power mode */

		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_main_settings=0x221500;
		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_wrdat0=0x35;
		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_send=0x1;

		mdelay(100);

		/** send color mode */

		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_main_settings=0x221500;
		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_wrdat0=0xf73a;
		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_send=0x1;

		mdelay(100);

		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_main_settings=0x210500;
		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_wrdat0=0xF729;
		currentpar->dsi_lnk_registers[DSI_LINK1]->direct_cmd_send=0x1;

		mdelay(100); /** check for display to up ok ~ secondary display */
	}

	dbgprintk(MCDE_ERROR_INFO, "\n>> TAAL Dispaly Initialisation done\n\n\n");
}


void mcde_dsi_tpodisplay_init(struct fb_info *info)
{
    u32 ret_val;
    int i;
    struct mcdefb_info *currentpar = info->par;

    dbgprintk(MCDE_ERROR_INFO, "\n\n\n////////////////////////////////////////////////////////////\n") ;
    dbgprintk(MCDE_ERROR_INFO, "// TPO screen startup procedure                           //\n") ;
    dbgprintk(MCDE_ERROR_INFO, "////////////////////////////////////////////////////////////\n") ;

    /*
    *! Settings for Gamma corrections and seletions
    */

    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## Gamma correction\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x39,0x10,0x010002E0,0x130D0601,0x1B090C0A,0x1426200E,currentpar->dsi_lnk_no,currentpar->chid);

    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## Gamma correction\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x39,0x10,0x282221E1,0x110F092B,0x035030624,0x3E29220C,currentpar->dsi_lnk_no,currentpar->chid);

    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## Gamma correction\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x39,0x10,0x010002E2,0x130D0601,0x1B090C0A,0x1426200E,currentpar->dsi_lnk_no,currentpar->chid);

    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## Gamma correction\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x39,0x10,0x282221E3,0x110F092B,0x35030624,0x3E29220C,currentpar->dsi_lnk_no,currentpar->chid);

    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## Gamma correction\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x39,0x10,0x010002E4,0x130D0601,0x1B090C0A,0x1426200E,currentpar->dsi_lnk_no,currentpar->chid);

    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## Gamma correction\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x39,0x10,0x282221E5,0x110F092B,0x035030624,0x3E29220C,currentpar->dsi_lnk_no,currentpar->chid);

    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## Gamma correction\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x15,0x02,0x000001EA,0x00000000,0x00000000,0x00000000,currentpar->dsi_lnk_no,currentpar->chid);

    /*
    *! Display Settings for initialization
    */

    /* sending the direct command -- This is Key packet */
    dbgprintk(MCDE_ERROR_INFO, "\n## Key packet\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x39,0x04,0x6455AAF6,0x00000000,0x00000000,0x00000000,currentpar->dsi_lnk_no,currentpar->chid);

    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## Display Interface Mode\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x15,0x02,0x000000B0,0x00000000,0x00000000,0x00000000,currentpar->dsi_lnk_no,currentpar->chid);

    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## LTPS Function 3\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x15,0x02,0x00004CBC,0x00000000,0x00000000,0x00000000,currentpar->dsi_lnk_no,currentpar->chid);

    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## Display function setting\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x15,0x02,0x000008B7,0x00000000,0x00000000,0x00000000,currentpar->dsi_lnk_no,currentpar->chid);

    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## Color Mode\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x15,0x02,0x0000F73A,0x00000000,0x00000000,0x00000000,currentpar->dsi_lnk_no,currentpar->chid);
    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## Memory data access control\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x15,0x02,0x0000C036,0x00000000,0x00000000,0x00000000,currentpar->dsi_lnk_no,currentpar->chid);

    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## Display Inversion ON\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x05,0x01,0x00000021,0x00000000,0x00000000,0x00000000,currentpar->dsi_lnk_no,currentpar->chid);

    /* sending the direct command */
    dbgprintk(MCDE_ERROR_INFO, "\n## Sleep Out\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x05,0x01,0x00000011,0x00000000,0x00000000,0x00000000,currentpar->dsi_lnk_no,currentpar->chid);

    dbgprintk(MCDE_ERROR_INFO, "\n## Delay loop\n") ;
    for (i=0; i<100000; i++);


    /* sending the direct command DISPON*/
    dbgprintk(MCDE_ERROR_INFO, "\n## Display ON\n");
    ret_val = dsisenddirectcommand(DSI_CLK_LANE_LPM,0x05,0x01,0x00000029,0x00000000,0x00000000,0x00000000,currentpar->dsi_lnk_no,currentpar->chid);


    dbgprintk(MCDE_ERROR_INFO, "\n>> TPO Dispaly Initialisation done\n\n\n");

}


int mcde_dsi_start(struct fb_info *info)
{

  unsigned int retVal= 0;

  mcde_dsi_taaldisplay_init(info);

  return retVal;
}

#ifdef _cplusplus
}
#endif /* _cplusplus */

