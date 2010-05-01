/*---------------------------------------------------------------------------*/
/* Copyright ST Ericsson 2009. 						     */
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

#ifndef _DSI_H_
#define _DSI_H_

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */
#include <mach/mcde.h>
#ifdef __KERNEL__

#define DSI_DPHY_Z_CALIB_OUT_VALID  0x1
/*******************************************************************************
DSI Error Enums
******************************************************************************/


 typedef enum
  {
      DSI_OK 				    = 0x1,	    /** No error.*/
      DSI_NO_PENDING_EVENT_ERROR	    = 0x2,
      DSI_NO_MORE_FILTER_PENDING_EVENT	    = 0x3,
      DSI_NO_MORE_PENDING_EVENT		    = 0x4,
      DSI_REMAINING_FILTER_PENDING_EVENTS   = 0x5,
      DSI_REMAINING_PENDING_EVENTS	    = 0x6,
      DSI_INTERNAL_EVENT		    = 0x7,
      DSI_INTERNAL_ERROR                    = 0x8,
      DSI_NOT_CONFIGURED                    = 0x9,
      DSI_REQUEST_PENDING                   = 0xA,
      DSI_PLL_PROGRAM_ERROR		    = 0xB,
      DSI_CLOCK_LANE_NOT_READY		    = 0xC,
      DSI_DATA_LANE1_NOT_READY		    = 0xD,
      DSI_DATA_LANE2_NOT_READY		    = 0xE,
      DSI_REQUEST_NOT_APPLICABLE            = 0x10,
      DSI_INVALID_PARAMETER                 = 0x11,
      DSI_UNSUPPORTED_FEATURE               = 0x12,
      DSI_UNSUPPORTED_HW                    = 0x13
  }dsi_error;

/********************************************************************************
DSI Interrupt Type Enums
********************************************************************************/
#define DSI_NO_INTERRUPT   0x0

typedef enum
{
    DSI_IRQ_TYPE_MCTL_MAIN                   = 0x01,
    DSI_IRQ_TYPE_CMD_MODE                    = 0x02,
    DSI_IRQ_TYPE_DIRECT_CMD_MODE             = 0x03,
    DSI_IRQ_TYPE_DIRECT_CMD_RD_MODE          = 0x04,
    DSI_IRQ_TYPE_VID_MODE                    = 0x05,
    DSI_IRQ_TYPE_TG                          = 0x06,
    DSI_IRQ_TYPE_DPHY_ERROR                  = 0x07,
    DSI_IRQ_TYPE_DPHY_CLK_TRIM_RD            = 0x08
} dsi_irq_type;




/*******************************************************************************
 DSI Main Setting Registers Enums and structures
******************************************************************************/

  typedef enum
  {
      DSI_INT_MODE_DISABLE = 0x00,
      DSI_INT_MODE_ENABLE = 0x01
  }dsi_int_mode;

  typedef enum
  {
      DSI_VSG_MODE_DISABLE = 0x00,
      DSI_VSG_MODE_ENABLE = 0x01
  }dsi_vsg_ctrl;

  typedef enum
  {
      DSI_TVG_MODE_DISABLE = 0x00,
      DSI_TVG_MODE_ENABLE = 0x01
  }dsi_tvg_ctrl;

  typedef enum
  {
      DSI_TBG_MODE_DISABLE = 0x00,
      DSI_TBG_MODE_ENABLE = 0x01
  }dsi_tbg_ctrl;

  typedef enum
  {
      DSI_RD_MODE_DISABLE = 0x00,
      DSI_RD_MODE_ENABLE = 0x01
  }dsi_rd_ctrl;


  typedef enum
  {
      DSI_SIGNAL_LOW = 0x0,
      DSI_SIGNAL_HIGH = 0x1
  }dsi_signal_state;

  typedef enum
  {
      DSI_LINK0 = 0x00,
      DSI_LINK1 = 0x01,
      DSI_LINK2 = 0x02
  }dsi_link;

  typedef enum
  {
      DSI_COMMAND_MODE = 0x0,
      DSI_VIDEO_MODE = 0x1,
      DSI_INTERFACE_BOTH = 0x2,
      DSI_INTERFACE_NONE = 0x3
  }dsi_interface_mode;

  typedef enum
  {
      DSI_INTERFACE_1 = 0x0,
      DSI_INTERFACE_2 = 0x1
  }dsi_interface;

  typedef enum
  {
      DSI_DISABLE = 0x0,
      DSI_ENABLE  = 0x1
  }dsi_link_state;

  typedef enum
  {
      DSI_SIGNAL_RESET = 0x0,
      DSI_SIGNAL_SET  = 0x1
  }dsi_stall_signal_state;

  typedef enum
  {
      DSI_IF1_DISABLE = 0x0,
      DSI_IF1_ENABLE  = 0x1
  }dsi_if1_state;

  typedef enum
  {
      DSI_IF_DISABLE = 0x0,
      DSI_IF_ENABLE  = 0x1
  }dsi_if_state;

  typedef enum
  {
      DSI_PLL_IN_CLK_27 = 0x0,/** from TV PLL*/
      DSI_PLL_IN_CLK_26 = 0x1 /** from system PLL*/
  }dsi_pll_clk_in;

  typedef enum
  {
      DSI_PLL_STOP = 0x0,
      DSI_PLL_START = 0x1
  }dsi_pll_mode;

  typedef enum
  {
      DSI_BTA_DISABLE = 0x0,
      DSI_BTA_ENABLE = 0x1
  }dsi_bta_mode;

  typedef enum
  {
      DSI_ECC_GEN_DISABLE = 0x0,
      DSI_ECC_GEN_ENABLE = 0x1
  }dsi_ecc_gen_mode;

  typedef enum
  {
      DSI_CHECKSUM_GEN_DISABLE = 0x0,
      DSI_CHECKSUM_GEN_ENABLE = 0x1
  }dsi_checksum_gen_mode;

  typedef enum
  {
      DSI_EOT_GEN_DISABLE = 0x0,
      DSI_EOT_GEN_ENABLE = 0x1
  }dsi_eot_gen_mode;

  typedef enum
  {
      DSI_HOST_EOT_GEN_DISABLE = 0x0,
      DSI_HOST_EOT_GEN_ENABLE = 0x1
  }dsi_host_eot_gen_mode;

  typedef enum
  {
      DSI_LANE_STOP = 0x0,
      DSI_LANE_START = 0x1
  }dsi_lane_state;
  typedef enum
  {
      DSI_LANE_DISABLE = 0x0,
      DSI_LANE_ENABLE = 0x1
  }dsi_lane_mode;
  typedef enum
  {
      DSI_CLK_CONTINIOUS_HS_DISABLE = 0x0,
      DSI_CLK_CONTINIOUS_HS_ENABLE = 0x1
  }dsi_clk_continious_hs_mode;
  typedef enum
  {
      DSI_INTERNAL_PLL = 0x0,
      DSI_SYSTEM_PLL = 0x1
  }pll_out_sel; /** DPHY HS bit clock select*/
  typedef enum
  {
      HS_INVERT_DISABLE = 0x0,
      HS_INVERT_ENABLE = 0x1
  }dsi_hs_invert_mode;
  typedef enum
  {
      HS_SWAP_PIN_DISABLE = 0x0,
      HS_SWAP_PIN_ENABLE = 0x1
  }dsi_swap_pin_mode;
  typedef enum
  {
      DSI_PLL_MASTER = 0x0,
      DSI_PLL_SLAVE  = 0x1
  }dsi_pll_mode_sel;

  typedef struct
  {
      u8               multiplier;
      u8               division_ratio;
      dsi_pll_clk_in      pll_in_sel;
      pll_out_sel         pll_out_sel;
      dsi_pll_mode_sel    pll_master;
  }dsi_pll_ctl;

  typedef enum
  {
      DSI_REG_TE = 0x00,
      DSI_IF_TE = 0x01
  }dsi_te_sel;

  typedef struct
  {
      dsi_te_sel      te_sel;
      dsi_interface   interface;
  }dsi_te_en;

  typedef enum
  {
	  DSI_TE_DISABLE = 0x0,
      DSI_TE_ENABLE  = 0x1
  }dsi_te_ctrl;

  typedef enum
  {
      DSI_CLK_LANE = 0x00,
      DSI_DATA_LANE1 = 0x01,
      DSI_DATA_LANE2 = 0x02
  }dsi_lane;

  typedef enum
  {
      DSI_DAT_LANE1 = 0x0,
      DSI_DAT_LANE2 = 0x1
  }dsi_data_lane;

  typedef enum
  {
      DSI_CLK_LANE_START = 0x00,
      DSI_CLK_LANE_IDLE  = 0x01,
      DSI_CLK_LANE_HS    = 0x02,
      DSI_CLK_LANE_ULPM  = 0x03
  }dsi_clk_lane_state;
  typedef enum
  {
      DSI_CLK_LANE_LPM  = 0x0,
      DSI_CLK_LANE_HSM  = 0x01
  }dsi_interface_mode_type;
  typedef enum
  {
      DSI_DATA_LANE_START = 0x000,
      DSI_DATA_LANE_IDLE  = 0x001,
      DSI_DATA_LANE_WRITE = 0x002,
      DSI_DATA_LANE_ULPM  = 0x003,
      DSI_DATA_LANE_READ  = 0x004
  }dsi_data_lane_state;

  typedef struct
  {
      u8  clk_div;
      u16  hs_tx_timeout;
      u16  lp_rx_timeout;
  }dsi_dphy_timeout;

  typedef enum
  {
      DSI_PLL_LOCK  = 0x01,
      DSI_CLKLANE_READY = 0x02,
      DSI_DAT1_READY    = 0x04,
      DSI_DAT2_READY    = 0x08,
      DSI_HSTX_TO_ERROR = 0x10,
      DSI_LPRX_TO_ERROR = 0x20,
      DSI_CRS_UNTERM_PCK = 0x40,
      DSI_VRS_UNTERM_PCK = 0x80
   }dsi_link_status;

   typedef struct
   {
       u16             if_data;
       dsi_signal_state   if_valid;
       dsi_signal_state   if_start;
       dsi_signal_state   if_frame_sync;
   }dsi_int_read;

   typedef enum
   {
       DSI_ERR_SOT_HS_1  = 0x1,
       DSI_ERR_SOT_HS_2  = 0x2,
       DSI_ERR_SOTSYNC_1 = 0x4,
       DSI_ERR_SOTSYNC_2 = 0x8,
       DSI_ERR_EOTSYNC_1 = 0x10,
       DSI_ERR_EOTSYNC_2 = 0x20,
       DSI_ERR_ESC_1     = 0x40,
       DSI_ERR_ESC_2     = 0x80,
       DSI_ERR_SYNCESC_1 = 0x100,
       DSI_ERR_SYNCESC_2 = 0x200,
       DSI_ERR_CONTROL_1 = 0x400,
       DSI_ERR_CONTROL_2 = 0x800,
       DSI_ERR_CONT_LP0_1 = 0x1000,
       DSI_ERR_CONT_LP0_2 = 0x2000,
       DSI_ERR_CONT_LP1_1 = 0x4000,
       DSI_ERR_CONT_LP1_2 = 0x8000,
   }dsi_dphy_err;

   typedef enum
   {
       DSI_VIRTUAL_CHANNEL_0 = 0x0,
       DSI_VIRTUAL_CHANNEL_1 = 0x1,
       DSI_VIRTUAL_CHANNEL_2 = 0x2,
       DSI_VIRTUAL_CHANNEL_3 = 0x3
   }dsi_virtual_ch;

   typedef enum
   {
       DSI_ERR_NO_TE = 0x1,
       DSI_ERR_TE_MISS = 0x2,
       DSI_ERR_SDI1_UNDERRUN = 0x4,
       DSI_ERR_SDI2_UNDERRUN = 0x8,
       DSI_ERR_UNWANTED_RD   = 0x10,
       DSI_CSM_RUNNING       = 0x20
   }dsi_cmd_mode_sts;

   typedef enum
   {
       DSI_COMMAND_DIRECT = 0x0,
       DSI_COMMAND_GENERIC = 0x1
   }dsi_cmd_type;

   typedef struct
   {
       u16 rd_size;
       dsi_virtual_ch rd_id;
       dsi_cmd_type cmd_type;
   }dsi_cmd_rd_property;

   typedef enum
   {
       DSI_CMD_WRITE            = 0x0,
       DSI_CMD_READ             = 0x1,
       DSI_CMD_TE_REQUEST       = 0x4,
       DSI_CMD_TRIGGER_REQUEST  = 0x5,
       DSI_CMD_BTA_REQUEST      = 0x6
   }dsi_cmd_nat;

   typedef enum
   {
       DSI_CMD_SHORT     = 0x0,
       DSI_CMD_LONG      = 0x1
   }dsi_cmd_packet;

   typedef struct
   {
       u8  rddat0;
       u8  rddat1;
       u8  rddat2;
       u8  rddat3;
   }dsi_cmd_rddat;

   typedef struct
   {
       dsi_cmd_nat        cmd_nature;
       dsi_cmd_packet     packet_type;
       u8              cmd_header;
       dsi_virtual_ch     cmd_id;
       u8              cmd_size;
       dsi_link_state     cmd_lp_enable;
       u8              cmd_trigger_val;
   }dsi_cmd_main_setting;

   typedef enum
   {
       DSI_CMD_TRANSMISSION = 0x1,
       DSI_WRITE_COMPLETED  = 0x2,
       DSI_TRIGGER_COMPLETED = 0x4,
       DSI_READ_COMPLETED    = 0x8,
       DSI_ACKNOWLEDGE_RECEIVED = 0x10,
       DSI_ACK_WITH_ERR_RECEIVED = 0x20,
       DSI_TRIGGER_RECEIVED      = 0x40,
       DSI_TE_RECEIVED           = 0x80,
       DSI_BTA_COMPLETED         = 0x100,
       DSI_BTA_FINISHED          = 0x200,
       DSI_READ_COMPLETED_WITH_ERR = 0x400,
       DSI_TRIGGER_VAL             = 0x7800,
       DSI_ACK_VAL                 = 0xFFFF0000
   }dsi_direct_cmd_sts;

   typedef enum
   {
       DSI_TE_256 = 0x00,
       DSI_TE_512 = 0x01,
       DSI_TE_1024 = 0x02,
       DSI_TE_2048 = 0x03
   }dsi_te_timeout;

   typedef enum
   {
       DSI_ARB_MODE_FIXED = 0x0,
       DSI_ARB_MODE_ROUNDROBIN = 0x1
   }dsi_arb_mode;

   typedef struct
   {
       dsi_arb_mode   arb_mode;
       dsi_interface  arb_fixed_if;
   }dsi_arb_ctl;

   typedef enum
   {
       DSI_STARTON_VSYNC = 0x00,
       DSI_STARTON_VFP   = 0x01,
   }dsi_start_mode;

   typedef enum
   {
       DSI_STOPBEFORE_VSYNV = 0x0,
       DSI_STOPAT_LINEEND   = 0x1,
       DSI_STOPAT_ACTIVELINEEND = 0x2,
   }dsi_stop_mode;

   typedef enum
   {
       DSI_NO_BURST_MODE = 0x0,
       DSI_BURST_MODE    = 0x1,
   }dsi_burst_mode;

   typedef enum
   {
       DSI_VID_MODE_16_PACKED = 0x0,
       DSI_VID_MODE_18_PACKED = 0x1,
       DSI_VID_MODE_16_LOOSELY = 0x2,
       DSI_VID_MODE_18_LOOSELY = 0x3
   }dsi_vid_pixel_mode;

   typedef enum
   {
       DSI_SYNC_PULSE_NOTACTIVE = 0x0,
       DSI_SYNC_PULSE_ACTIVE    = 0x1
   }dsi_sync_pulse_active;

   typedef enum
   {
       DSI_SYNC_PULSE_HORIZONTAL_NOTACTIVE = 0x0,
       DSI_SYNC_PULSE_HORIZONTAL_ACTIVE    = 0x1
   }dsi_sync_pulse_horizontal;

   typedef enum
   {
       DSI_NULL_PACKET = 0x0,
       DSI_BLANKING_PACKET = 0x1,
       DSI_LP_MODE         = 0x2,
   }dsi_blanking_packet;

   typedef enum
   {
       DSI_RECOVERY_HSYNC = 0x0,
       DSI_RECOVERY_VSYNC = 0x1,
       DSI_RECOVERY_STOP  = 0x2,
       DSI_RECOVERY_HSYNC_VSYNC = 0x3
   }dsi_recovery_mode;

   typedef struct
   {
       dsi_start_mode      vid_start_mode;
       dsi_stop_mode       vid_stop_mode;
       dsi_virtual_ch      vid_id;
       u8               header;
       dsi_vid_pixel_mode  vid_pixel_mode;
       dsi_burst_mode      vid_burst_mode;
       dsi_sync_pulse_active sync_pulse_active;
       dsi_sync_pulse_horizontal sync_pulse_horizontal;
       dsi_blanking_packet       blkline_mode;
       dsi_blanking_packet       blkeol_mode;
       dsi_recovery_mode         recovery_mode;
   }dsi_vid_main_ctl;

   typedef struct
   {
       u16 vact_length;
       u8  vfp_length;
       u8  vbp_length;
       u8  vsa_length;
   }dsi_img_vertical_size;

   typedef struct
   {
       u8  hsa_length;
       u8  hbp_length;
       u16 hfp_length;
       u16 rgb_size;
   }dsi_img_horizontal_size;

   typedef struct
   {
       u16  line_val;
       u8   line_pos;
       u16  horizontal_val;
       u8   horizontal_pos;
   }dsi_img_position;

   typedef enum
   {
       DSI_VSG_RUNNING  = 0x1,
       DSI_ERR_MISSING_DATA = 0x2,
       DSI_ERR_MISSING_HSYNC = 0x4,
       DSI_ERR_MISSING_VSYNC = 0x8,
       DSI_ERR_SMALL_LENGTH  = 0x10,
       DSI_ERR_SMALL_HEIGHT  = 0x20,
       DSI_ERR_BURSTWRITE    = 0x40,
       DSI_ERR_LINEWRITE     = 0x80,
       DSI_ERR_LONGWRITE     = 0x100,
       DSI_ERR_VRS_WRONG_LENGTH = 0x200
   }dsi_vid_mode_sts;

   typedef enum
   {
       DSI_NULL_PACK = 0x0,
       DSI_LP          = 0x1,
   }dsi_burst_lp;

   typedef struct
   {
       dsi_burst_lp  burst_lp;
       u16        max_burst_limit;
       u16        max_line_limit;
       u16        exact_burst_limit;
   }dsi_vca_setting;

   typedef struct
   {
       u16     blkeol_pck;
       u16     blkline_event_pck;
       u16     blkline_pulse_pck;
       u16     vert_balnking_duration;
       u16     blkeol_duration;
   }dsi_vid_blanking;

   typedef struct
   {
       u8      col_red;
       u8      col_green;
       u8      col_blue;
       u8      pad_val;
   }dsi_vid_err_color;

   typedef enum
   {
       DSI_TVG_MODE_UNIQUECOLOR  = 0x0,
       DSI_TVG_MODE_STRIPES      = 0x1,
   }dsi_tvg_mode;

   typedef enum
   {
       DSI_TVG_STOP_FRAMEEND = 0x0,
       DSI_TVG_STOP_LINEEND  = 0x1,
       DSI_TVG_STOP_IMMEDIATE = 0x2,
   }dsi_tvg_stop_mode;



   typedef struct
   {
       u8      tvg_stripe_size;
       dsi_tvg_mode  tvg_mode;
       dsi_tvg_stop_mode stop_mode;
   }dsi_tvg_control;

   typedef struct
   {
       u16   tvg_nbline;
       u16   tvg_line_size;
   }dsi_tvg_img_size;

   typedef struct
   {
       u8  col_red;
       u8  col_green;
       u8  col_blue;
   }dsi_frame_color;

   typedef enum
   {
       DSI_TVG_COLOR1 = 0x0,
       DSI_TVG_COLOR2 = 0x1
   }dsi_color_type;

   typedef enum
   {
       DSI_TVG_STOPPED = 0x0,
       DSI_TVG_RUNNING = 0x1
   }dsi_tvg_state;

   typedef enum
   {
       DSI_TVG_STOP = 0x0,
       DSI_TVG_START = 0x1
   }dsi_tvg_ctrl_state;

   typedef enum
   {
       DSI_TBG_STOPPED = 0x0,
       DSI_TBG_RUNNING = 0x1
   }dsi_tbg_state;

   typedef enum
   {
       DSI_SEND_1BYTE   = 0x0,
       DSI_SEND_2BYTE   = 0x1,
       DSI_SEND_BURST_STOP_COUNTER = 0x3,
       DSI_SEND_BURST_STOP = 0x4
   }dsi_tbg_mode;

   typedef enum
   {
       DSI_ERR_FIXED = 0x1,
       DSI_ERR_UNCORRECTABLE = 0x2,
       DSI_ERR_CHECKSUM      = 0x4,
       DSI_ERR_UNDECODABLE   = 0x8,
       DSI_ERR_RECEIVE      = 0x10,
       DSI_ERR_OVERSIZE     = 0x20,
       DSI_ERR_WRONG_LENGTH  = 0x40,
       DSI_ERR_MISSING_EOT   = 0x80,
       DSI_ERR_EOT_WITH_ERR  = 0x100
   }dsi_direct_cmd_rd_sts_ctl;

   typedef enum
   {
       DSI_TVG_STS = 0x1,
       DSi_TBG_STS = 0x2
   }dsi_tg_sts_ctl;

typedef struct
{
    dsi_interface_mode   dsi_if_mode;
    dsi_interface        dsiInterface;
    dsi_if1_state         dsi_if1_state;
    dsi_link_state        dsi_link_state;
    dsi_int_mode          dsi_int_mode;
    dsi_interface_mode_type if_mode_type;
}dsi_link_context;

struct dsi_dphy_static_conf {
	dsi_hs_invert_mode clocklanehsinvermode;
	dsi_swap_pin_mode clocklaneswappinmode;
	dsi_hs_invert_mode datalane1hsinvermode;
	dsi_swap_pin_mode datalane1swappinmode;
	dsi_hs_invert_mode datalane2hsinvermode;
	dsi_swap_pin_mode datalane2swappinmode;
	u8 ui_x4; /** unit interval time for clock lane*/
};
struct dsi_link_conf {
	dsi_link_state dsiLinkState;
	dsi_interface dsiInterface;
	dsi_interface_mode dsiInterfaceMode;
	dsi_interface_mode_type videoModeType;  /** for LP/HS mode for vide mode */
	dsi_interface_mode_type commandModeType;/** for LP/HS mode for command mode */
	dsi_lane_mode  clockLaneMode;
	dsi_lane_mode  dataLane1Mode;
	dsi_lane_mode  dataLane2Mode;
	dsi_arb_mode   arbMode;
	dsi_te_ctrl    if1TeCtrl;
	dsi_te_ctrl    if2TeCtrl;
	dsi_te_ctrl    regTeCtrl;
	dsi_bta_mode   btaMode;
	dsi_rd_ctrl    rdCtrl;
	dsi_host_eot_gen_mode 	hostEotGenMode;
	dsi_eot_gen_mode 	displayEotGenMode;
	dsi_ecc_gen_mode 	dispEccGenMode;
	dsi_checksum_gen_mode	dispChecksumGenMode;
	dsi_clk_continious_hs_mode clockContiniousMode;
	u8			paddingValue;
};
#endif /** __KERNEL_ */

u32 dsiconfdphy1(mcde_pll_ref_clk pll_sel, mcde_ch_id chid, dsi_link link);
u32 dsidisplayinitLPcmdmode(mcde_ch_id chid, dsi_link link);

dsi_error dsisetlinkstate(dsi_link link, dsi_link_state linkState, mcde_ch_id chid);
u32 dsiLinkInit(struct dsi_link_conf *pdsiLinkConf, struct dsi_dphy_static_conf dphyStaticConf, mcde_ch_id chid, dsi_link link);
int mcde_dsi_test_LP_directcommand_mode(struct fb_info *info,u32 key);
int mcde_dsi_start(struct fb_info *info);
int mcde_dsi_test_dsi_HS_directcommand_mode(struct fb_info *info,u32 key);

dsi_error dsisetPLLcontrol(dsi_link link, mcde_ch_id chid, dsi_pll_ctl pll_ctl);
dsi_error dsisetPLLmode(dsi_link link, mcde_ch_id chid, dsi_pll_mode mode);
dsi_error dsigetlinkstatus(dsi_link link, mcde_ch_id chid, u8 *p_status);
dsi_error dsisetInterface(dsi_link link, mcde_ch_id chid, dsi_if_state state, dsi_interface interface);
dsi_error dsisetInterface1mode(dsi_link link, dsi_interface_mode mode, mcde_ch_id chid);
dsi_error dsisetInterfaceInLpm(dsi_link link, mcde_ch_id chid, dsi_interface_mode_type modType, dsi_interface interface);
dsi_error dsisetTEtimeout(dsi_link link, mcde_ch_id chid, u32 te_timeout);
dsi_error dsireadset(dsi_link link, dsi_rd_ctrl state, mcde_ch_id chid);
dsi_error dsisetBTAmode(dsi_link link, dsi_bta_mode mode, mcde_ch_id chid);
dsi_error dsisetdispEOTGenmode(dsi_link link, dsi_eot_gen_mode mode, mcde_ch_id chid);
dsi_error dsisetdispHOSTEOTGenmode(dsi_link link, dsi_host_eot_gen_mode mode, mcde_ch_id chid);
dsi_error dsisetdispCHKSUMGenmode(dsi_link link, dsi_checksum_gen_mode mode, mcde_ch_id chid);
dsi_error dsisetdispECCGenmode(dsi_link link, dsi_ecc_gen_mode mode, mcde_ch_id chid);
dsi_error dsisetCLKHSsendingmode(dsi_link link, dsi_clk_continious_hs_mode mode, mcde_ch_id chid);
dsi_error dsisetpaddingval(dsi_link link, mcde_ch_id chid, u8 padding);
dsi_error dsisetTE(dsi_link link, dsi_te_en tearing, dsi_te_ctrl state, mcde_ch_id chid);
dsi_error dsisetlaneULPwaittime(dsi_link link, mcde_ch_id chid, dsi_lane lane, u16 timeout);

dsi_error dsisetlanestate(dsi_link link, mcde_ch_id chid, dsi_lane_state mode, dsi_lane lane);
dsi_error dsiset_hs_clock(dsi_link link, struct dsi_dphy_static_conf dphyStaticConf, mcde_ch_id chid);
dsi_error dsisetDPHYtimeout(dsi_link link, mcde_ch_id chid, dsi_dphy_timeout timeout);
void mcde_dsi_tpodisplay_init(struct fb_info *info);
void mcde_dsi_taaldisplay_init(struct fb_info *info);

/* following Apis are used by stw5810 driver for configuration */
u32 dsisenddirectcommand(dsi_interface_mode_type mode_type, u32 cmd_head,u32 cmd_size,u32 cmd1,u32 cmd2,u32 cmd3,u32 cmd4, dsi_link link, mcde_ch_id chid);

u32 dsiLPdcslongwrite(u32 VC_ID, u32 NoOfParam,  u32 Param0,  u32 Param1,u32 Param2, u32 Param3,
			u32 Param4,u32 Param5,u32 Param6,u32 Param7,u32 Param8, u32 Param9,u32 Param10,u32 Param11,
			u32 Param12,u32 Param13,u32 Param14, u32 Param15, mcde_ch_id chid, dsi_link link);

u32 dsiLPdcsshortwrite1parm(u32 VC_ID, u32 Param0,u32 Param1, mcde_ch_id chid, dsi_link link);
u32 dsiLPdcsshortwritenoparam(u32 VC_ID, u32 Param0, mcde_ch_id chid, dsi_link link);

u32 dsiHSdcslongwrite(u32 VC_ID, u32 NoOfParam,  u32 Param0,  u32 Param1,u32 Param2, u32 Param3,
									u32 Param4,u32 Param5,u32 Param6,u32 Param7,u32 Param8, u32 Param9,u32 Param10,u32 Param11,
									u32 Param12,u32 Param13,u32 Param14, u32 Param15, mcde_ch_id chid, dsi_link link);

u32 dsiHSdcsshortwrite1parm(u32 VC_ID, u32 Param0,u32 Param1, mcde_ch_id chid, dsi_link link);
u32 dsiHSdcsshortwritenoparam(u32 VC_ID, u32 Param0, mcde_ch_id chid, dsi_link link);
u32 dsireaddata(u8* byte0, u8* byte1, u8* byte2, u8* byte3, mcde_ch_id chid, dsi_link link);


#ifdef _cplusplus
}
#endif /* _cplusplus */

#endif /* !defined(_DSI_H_) */


