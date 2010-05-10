/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License terms:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef _MCDE_H_
#define _MCDE_H_
#include <linux/gpio.h>
#include <linux/fb.h>
#include <mach/mcde_ioctls.h>

#ifdef __KERNEL__

/*******************************************************************************
 MCDE Control Enums and structures
  ******************************************************************************/
/* typedef enum
{
	MCDE_CH_A_LSB = 0x00,
	MCDE_CH_A_MID = 0x01,
	MCDE_CH_A_MSB = 0x02,
	MCDE_CH_B_LSB = 0x03,
	MCDE_CH_B_MID = 0x04,
	MCDE_CH_B_MSB = 0x05,
	MCDE_CH_C_LSB = 0x06
}mcde_out_mux_cfg;

typedef enum
{
	MCDE_CH_NORMAL_MODE = 0x0,
	MCDE_CH_MUX_MODE	= 0x1
}mcde_ch_ctrl;
*/
  typedef enum
  {
      MCDE_CH_FIFO_A_ENABLE = 0x0,
      MCDE_CH_FIFO_B_ENABLE    = 0x1
  }mcde_ch_fifo_ab_mux_ctrl;

  typedef enum
  {
      MCDE_CH_FIFO_0_ENABLE = 0x0,
      MCDE_CH_FIFO_1_ENABLE    = 0x1
  }mcde_ch_fifo_01_mux_ctrl;

  typedef enum
  {
      MCDE_INPUT_FIFO_DISABLE = 0x0,
      MCDE_INPUT_FIFO_ENABLE  = 0x1
  }mcde_input_fifo_ctrl;

  typedef enum
  {
      MCDE_DPI_DISABLE = 0x0,
      MCDE_DPI_ENABLE  = 0x1
  }mcde_dpi_ctrl;

  typedef enum
  {
      MCDE_DBI_DISABLE = 0x0,
      MCDE_DBI_ENABLE  = 0x1
  }mcde_dbi_ctrl;

  typedef enum
  {
      MCDE_DSIVID_DISABLE = 0x0,
      MCDE_DSIVID_ENABLE  = 0x1
  }mcde_dsivid_ctrl;

  typedef enum
  {
      MCDE_DSICMD_DISABLE = 0x0,
      MCDE_DSICMD_ENABLE  = 0x1
  }mcde_dsicmd_ctrl;

  typedef enum
  {
     MCDE_WTRMRK_LEVEL_0,
     MCDE_WTRMRK_LEVEL_1,
     MCDE_WTRMRK_LEVEL_2,
     MCDE_WTRMRK_LEVEL_3,
     MCDE_WTRMRK_LEVEL_4,
     MCDE_WTRMRK_LEVEL_5,
     MCDE_WTRMRK_LEVEL_6,
     MCDE_WTRMRK_LEVEL_7
  }mcde_fifo_wrtmrk_level;
  typedef enum
  {
      MCDE_CH_AB_FRAMESYNC_DPI = 0x0,
      MCDE_CH_AB_FRAMESYNC_DBI = 0x1
  }mcde_ch_ab_framesync_cfg;

typedef enum
{
	MCDE_DISABLE = 0x0,
	MCDE_ENABLE = 0x1
}mcde_state;

typedef enum
{
      MCDE_CH_A_OUTPUT_FIFO_B = 0x0,
      MCDE_CH_A_OUTPUT_FIFO_C1 = 0x1
  }mcde_fifo_sel_bc1_cfg;

  typedef enum
  {
      MCDE_CH_A_OUTPUT_FIFO_A = 0x0,
      MCDE_CH_A_OUTPUT_FIFO_C0 = 0x1
  }mcde_fifo_sel_ac0_cfg;

  typedef enum
  {
      MCDE_CH_B_PULSE_PANEL7 = 0x0,
      MCDE_CH_C0C1_PULSE_PANEL = 0x1
  }mcde_sync_mux7_cfg;

  typedef enum
  {
      MCDE_CH_A_PULSE_PANEL = 0x0,
      MCDE_CH_B_PULSE_PANEL6 = 0x1
  }mcde_sync_mux6_cfg;

  typedef enum
  {
      MCDE_CH_B_CLK_PANEL5 = 0x0,
      MCDE_CH_C_CHIP_SELECT5 = 0x1
  }mcde_sync_mux5_cfg;

  typedef enum
  {
      MCDE_CH_A_CLK_PANEL4 = 0x0,
      MCDE_CH_B_CLK_PANEL4 = 0x1
  }mcde_sync_mux4_cfg;
/*
typedef enum
{
	MCDE_CHIPSELECT_C0 = 0x0,
	MCDE_CHIPSELECT_C1 = 0x1
}mcde_sync_mux23_cfg;

typedef enum
{
	MCDE_CH_A_OUT_ENABLE = 0x0,
	MCDE_CH_C_OUT_ENABLE = 0x1
}mcde_sync_mux1_cfg;
*/
  typedef enum
  {
      MCDE_CH_A_CLK_PANEL0 = 0x0,
      MCDE_CH_C_CHIP_SELECT0 = 0x1
  }mcde_sync_mux0_cfg;
/*
struct mcde_config
{
  mcde_out_mux_cfg       data_msb1;
  mcde_out_mux_cfg       data_msb0;
  mcde_out_mux_cfg       data_mid;
  mcde_out_mux_cfg       data_lsb1;
  mcde_out_mux_cfg       data_lsb0;
  mcde_fifo_wrtmrk_level  fifo_wtrmrk_level;
  mcde_ch_ab_framesync_cfg ch_b_frame_sync_ctrl;
  mcde_ch_ab_framesync_cfg ch_a_frame_sync_ctrl;
  mcde_fifo_sel_bc1_cfg   ch_b_fifo_sel;
  mcde_fifo_sel_ac0_cfg   ch_a_fifo_sel;
  mcde_sync_mux7_cfg      panel_mcdeblp;
  mcde_sync_mux6_cfg      panel_mcdeealp;
  mcde_sync_mux4_cfg      panel_mcdebcp;
  mcde_sync_mux4_cfg      panel_mcdeacp4;
  mcde_sync_mux23_cfg      panel_mcdeaspl;
  mcde_sync_mux23_cfg      panel_mcdeaps;
  mcde_sync_mux1_cfg     panel_mcdealp;
  mcde_sync_mux0_cfg     panel_mcdeacp;
};*/
  struct mcde_control
  {
      mcde_ch_fifo_ab_mux_ctrl ch_fifo_ab_mux;
      mcde_ch_fifo_01_mux_ctrl ch_fifo_01_mux;
      mcde_input_fifo_ctrl  input_fifo_enable;
      mcde_dpi_ctrl         dpi_a_enable;
      mcde_dpi_ctrl         dpi_b_enable;
      mcde_dbi_ctrl         dbi_c0_enable;
      mcde_dbi_ctrl         dbi_c1_enable;
      mcde_dsivid_ctrl      dsi_vid0_enable;
      mcde_dsivid_ctrl      dsi_vid1_enable;
      mcde_dsivid_ctrl      dsi_vid2_enable;
      mcde_dsicmd_ctrl      dsi_cmd0_enable;
      mcde_dsicmd_ctrl      dsi_cmd1_enable;
      mcde_dsicmd_ctrl      dsi_cmd2_enable;
  };
/*
struct mcde_ch_mode_ctrl
{
  mcde_ch_ctrl sync_ctrl_chA;
  mcde_ch_ctrl sync_ctrl_chB;
  mcde_ch_ctrl flow_ctrl_chA;
  mcde_ch_ctrl flow_ctrl_chB;
};
*/
  struct mcde_irq_status
  {
      u32 irq_status;
      u32 event_status;
      u8  is_new;
  } ;
/*
typedef enum
{
	MCDE_EVENT_MASTER_BUS_ERROR = 0x01,
	MCDE_EVENT_END_OF_FRAME_TRANSFER       = 0x02,
	MCDE_EVENT_SYNCHRO_CAPTURE_TRIGGER     = 0x04,
	MCDE_EVENT_HORIZONTAL_SYNCHRO_CAPTURE  = 0x08,
	MCDE_EVENT_VERTICAL_SYNCHRO_CAPTURE    = 0x10
} mcde_event_id;
*/
/*     Check *********************************/

typedef enum
{
  MCDE_CH_A_LSB = 0x00,
  MCDE_CH_A_MID = 0x01,
  MCDE_CH_A_MSB = 0x02,
  MCDE_CH_B_LSB = 0x03,
  MCDE_CH_B_MID = 0x04,
  MCDE_CH_B_MSB = 0x05,
  MCDE_CH_C_LSB = 0x06
}mcde_out_mux_ctrl;

typedef enum
{
  MCDE_FIFO_FA = 0x0,
  MCDE_FIFO_FB = 0x1,
  MCDE_FIFO_F0 = 0x2,
  MCDE_FIFO_F1 = 0x3
}mcde_fifo;

typedef enum
{
  MCDE_DPI_A = 0x0,
  MCDE_DPI_B = 0x1,
  MCDE_DBI_C0 = 0x2,
  MCDE_DBI_C1 = 0x3,
  MCDE_DSI_VID0 = 0x4,
  MCDE_DSI_VID1 = 0x5,
  MCDE_DSI_VID2 = 0x6,
  MCDE_DSI_CMD0 = 0x7,
  MCDE_DSI_CMD1 = 0x8,
  MCDE_DSI_CMD2 = 0x9
}mcde_fifo_output;

struct mcde_fifo_ctrl
{
  mcde_fifo_output out_fifoa;
  mcde_fifo_output out_fifob;
  mcde_fifo_output out_fifo0;
  mcde_fifo_output out_fifo1;
};

typedef enum
{
  MCDE_CH_NORMAL_MODE = 0x0,
  MCDE_CH_MUX_MODE    = 0x1
}mcde_ch_ctrl;

typedef enum
{
  MCDE_CHIPSELECT_C0 = 0x0,
  MCDE_CHIPSELECT_C1 = 0x1
}mcde_sync_mux_ctrl;

typedef enum
{
  MCDE_CH_A_OUT_ENABLE = 0x0,
  MCDE_CH_C_OUT_ENABLE = 0x1
}mcde_sync_mux1_ctrl;

typedef enum
{
  MCDE_CH_A_CLK_PANEL = 0x0,
  MCDE_CH_C_CHIP_SELECT = 0x1
}mcde_sync_mux0_ctrl;

typedef enum
{
  MCDE_CDI_CH_A = 0x0,
  MCDE_CDI_CH_B = 0x1
}mcde_cdi_ctrl;

typedef enum
{
  MCDE_CDI_DISABLE = 0x0,
  MCDE_CDI_ENABLE = 0x1
}mcde_cdi;
/*
typedef enum
{
  MCDE_DISABLE = 0x0,
  MCDE_ENABLE = 0x1
}mcde_state;

typedef struct
{
  t_mcde_out_mux_ctrl       data_msb1;
  t_mcde_out_mux_ctrl       data_msb0;
  t_mcde_out_mux_ctrl       data_mid;
  t_mcde_out_mux_ctrl       data_lsb1;
  t_mcde_out_mux_ctrl       data_lsb0;
  u8                   ififo_watermark;
}mcde_control;
*/
typedef enum
{
  MCDE_OUTPUT_FIFO_B = 0x0,
  MCDE_OUTPUT_FIFO_C1 = 0x1
}mcde_swap_b_c1_ctrl;

typedef enum
{
  MCDE_CONF_TVA_DPIC0_LCDB = 0x0,
  MCDE_CONF_TVB_DPIC1_LCDA = 0x1,
  MCDE_CONF_DPIC1_LCDA     = 0x2,
  MCDE_CONF_DPIC0_LCDB     = 0x3,
  MCDE_CONF_LCDA_LCDB      = 0x4,
  MCDE_CONF_DSI		   = 0x5
}mcde_output_conf;

typedef enum
{
  MCDE_OUTPUT_FIFO_A = 0x0,
  MCDE_OUTPUT_FIFO_C0 = 0x1
}mcde_swap_a_c0_ctrl;

/*
typedef struct
{
  u32    irq_status;
  u32    event_status;
  t_bool      is_new;
}mcde_irq_status;
*/
  typedef enum
  {
    MCDE_EVENT_MASTER_BUS_ERROR = 0x01,
    MCDE_EVENT_END_OF_FRAME_TRANSFER       = 0x02,
    MCDE_EVENT_SYNCHRO_CAPTURE_TRIGGER     = 0x04,
    MCDE_EVENT_HORIZONTAL_SYNCHRO_CAPTURE  = 0x08,
    MCDE_EVENT_VERTICAL_SYNCHRO_CAPTURE    = 0x10
  } mcde_event_id;

typedef struct
{
  u8   pllfreq_div;
  u8   datachannels_num;
  u8   delay_clk;
  u8   delay_d2;
  u8   delay_d1;
  u8   delay_d0;
}mcde_cdi_delay;
/*******************************************************************************
  External Source Enums and Structures
  ******************************************************************************/

  typedef enum
  {
      MCDE_EXT_SRC_0 = 0x0,
      MCDE_EXT_SRC_1 = 0x1,
      MCDE_EXT_SRC_2 = 0x2,
      MCDE_EXT_SRC_3 = 0x3,
      MCDE_EXT_SRC_4 = 0x4,
      MCDE_EXT_SRC_5 = 0x5,
      MCDE_EXT_SRC_6 = 0x6,
      MCDE_EXT_SRC_7 = 0x7,
      MCDE_EXT_SRC_8 = 0x8,
      MCDE_EXT_SRC_9 = 0x9,
      MCDE_EXT_SRC_10 = 0xA,
      MCDE_EXT_SRC_11 = 0xB,
      MCDE_EXT_SRC_12 = 0xC,
      MCDE_EXT_SRC_13 = 0xD,
      MCDE_EXT_SRC_14 = 0xE,
      MCDE_EXT_SRC_15 = 0xF
  }mcde_ext_src;


  typedef enum
  {
      MCDE_U8500_PANEL_1BPP=1,
      MCDE_U8500_PANEL_2BPP=2,
      MCDE_U8500_PANEL_4BPP=4,
      MCDE_U8500_PANEL_8BPP=8,
      MCDE_U8500_PANEL_16BPP=16,
      MCDE_U8500_PANEL_24BPP_PACKED=24,
      MCDE_U8500_PANEL_24BPP=25,
      MCDE_U8500_PANEL_32BPP=32,
      MCDE_U8500_PANEL_YCBCR=11,
  }mcde_fb_bpp;

  typedef enum
  {
      MCDE_U8500_PANEL_16BPP_RGB=35,
      MCDE_U8500_PANEL_16BPP_IRGB=36,
      MCDE_U8500_PANEL_16BPP_ARGB=37,
      MCDE_U8500_PANEL_12BPP=38,
  }mcde_fb_16bpp_type;


  typedef enum
  {
      MCDE_FS_FREQ_UNCHANGED = 0x0,
      MCDE_FS_FREQ_DIV_2  = 0x1
  }mcde_fs_div;

  typedef enum
  {
      MCDE_FS_FREQ_DIV_ENABLE = 0x0,
      MCDE_FS_FREQ_DIV_DISABLE = 0x1
  }mcde_fs_ctrl;

  typedef enum
  {
      MCDE_MULTI_CH_CTRL_ALL_OVR = 0x0,
      MCDE_MULTI_CH_CTRL_PRIMARY_OVR = 0x1
  }mcde_multi_ovr_ctrl;

  typedef enum
  {
      MCDE_BUFFER_SEL_EXT = 0x0,
      MCDE_BUFFER_AUTO_TOGGLE = 0x1,
      MCDE_BUFFER_SOFTWARE_SELECT = 0x2,
      MCDE_BUFFER_RESERVED
  }mcde_buffer_sel_mode;

  struct mcde_ext_src_ctrl
  {
      mcde_fs_div            fs_div;
      mcde_fs_ctrl           fs_ctrl;
      mcde_multi_ovr_ctrl    ovr_ctrl;
      mcde_buffer_sel_mode   sel_mode;
  };


/*******************************************************************************
  Overlay Enums and Structures
  ******************************************************************************/


/*  typedef enum
  {
      MCDE_OVERLAY_0,
      MCDE_OVERLAY_1,
      MCDE_OVERLAY_2,
      MCDE_OVERLAY_3,
      MCDE_OVERLAY_4,
      MCDE_OVERLAY_5,
      MCDE_OVERLAY_6,
      MCDE_OVERLAY_7
  }mcde_overlay_id; */







 /* typedef enum
  {
      MCDE_OVR_INTERLACE_DISABLE = 0x0,
      MCDE_OVR_INTERLACE_ENABLE = 0x1
  }mcde_ovr_interlace_ctrl;

  typedef enum
  {
      MCDE_OVR_INTERLACE_TOPFIELD = 0x0,
      MCDE_OVR_INTERLACE_BOTTOMFIELD = 0x1
  }mcde_ovr_interlace_mode;*/

  typedef enum
  {
      MCDE_CH_A = 0x0,
      MCDE_CH_B = 0x1,
      MCDE_CH_C0 = 0x2,
      MCDE_CH_C1 = 0x3
  }mcde_ch_id;

  typedef enum
  {
      MCDE_FETCH_INPROGRESS = 0x0,
      MCDE_FETCH_COMPLETE = 0x1
  }mcde_ovr_fetch_status;

  typedef enum
  {
      MCDE_OVR_READ_COMPLETE = 0x0,
      MCDE_OVR_READ_INPROGRESS = 0x1
  }mcde_ovr_read_status;

  struct mcde_ovr_control
  {
      mcde_rotate_req     rot_burst_req;
      mcde_outsnd_req     outstnd_req;
      mcde_burst_req      burst_req;
      u8                  priority;
      mcde_color_key_ctrl color_key;
      mcde_pal_ctrl       pal_control;
      mcde_col_conv_ctrl  col_ctrl;
      mcde_overlay_ctrl   ovr_state;
  mcde_ovr_alpha_enable alpha;
  mcde_ovr_clip_enable  clip;
  };

  struct mcde_ovr_config
  {
      u32  line_per_frame;
    /*  mcde_ovr_interlace_mode  ovr_interlace;
      mcde_ovr_interlace_ctrl  ovr_intlace_ctrl;*/
      mcde_ext_src        src_id;
      u16 ovr_ppl;
  };

  struct mcde_ovr_conf2
  {
      u8               alpha_value;
      u8               pixoff;
      mcde_ovr_opq_ctrl        ovr_opaq;
      mcde_blend_ctrl     ovr_blend;
      u32 watermark_level;
  };
  /*
  struct mcde_ovr_blend_ctrl
  {
      u8               alpha_value;
      mcde_ovr_opq_ctrl        ovr_opaq;
      mcde_blend_ctrl     ovr_blend;
      u8        ovr_zlevel;
      u16       ovr_xpos;
      u16        ovr_ypos;
  };*/

  struct mcde_ovr_comp
  {
      u8        ovr_zlevel;
      u16        ovr_ypos;
      mcde_ch_id   ch_id;
      u16       ovr_xpos;
  };

  struct mcde_ovr_clipincr
  {
      u32        lineincr;
      u32        yclip;
	  u32        xclip;
  };

struct mcde_ovr_clip
{
  u32 ytlcoor;
  u32 xtlcoor;
  u32 ybrcoor;
  u32 xbrcoor;
};

typedef struct
{
  u16        ovr_ypos;
  u16        ovr_xpos;
}mcde_ovr_xy;


typedef enum
{
  MCDE_OVERLAY_NOT_BLOCKED = 0x0,
  MCDE_OVERLAY_BLOCKED     = 0x1
}mcde_ovr_blocked_status;
  struct mcde_ovr_status
  {
	mcde_ovr_blocked_status ovrb_status;
      mcde_ovr_fetch_status   ovr_status;
      mcde_ovr_read_status    ovr_read;
  };

  /*******************************************************************************
   Channel Configuration Enums and Structures
   ******************************************************************************/



 /* struct
  {
      u16  InitDelay;
      u16  PPDelay;
  }mcde_ch_delay;*/

  struct mcde_chsyncconf
  {
      u16             swint_vcnt;
      mcde_frame_events  swint_vevent;
      u16             hwreq_vcnt;
      mcde_frame_events  hwreq_vevent;
  };

  struct mcde_chsyncmod
  {
      mcde_synchro_out_interface    out_synch_interface;
      mcde_synchro_source           ch_synch_src;
      mcde_sw_trigger               sw_trig;

  };



  struct mcde_ch_priority
  {
      u8  ch_priority;
  };

typedef enum
{
  MCDE_CHNL_RUNNING_NORMALLY = 0x0,
  MCDE_CHNL_ABORT_OCCURRED   = 0x1
}mcde_chnl_abort_state;

typedef enum
{
  MCDE_CHNL_READ_ONGOING = 0x0,
  MCDE_CHNL_READ_DONE    = 0x1
}mcde_chnl_read_status;

typedef struct
{
  mcde_chnl_abort_state   abort_state;
  mcde_chnl_read_status   read_state;
}mcde_chnl_state;
  /*******************************************************************************
 Channel A/B Enums and Structures
  ******************************************************************************/

  typedef enum
  {
	  MCDE_CLOCKWISE = 0x0,
	  MCDE_ANTICLOCKWISE = 0x1
  }mcde_rot_dir;

  typedef enum
  {
      MCDE_GAMMA_ENABLE = 0x0,
      MCDE_GAMMA_DISABLE = 0x1
  }mcde_gamma_ctrl;

  typedef enum
  {
	  MCDE_INPUT_YCrCb = 0x0,
	  MCDE_INPUT_RGB = 0x1
  }mcde_flicker_format;

  typedef enum
  {
	  MCDE_ALPHA_INPUTSRC = 0x0,
	  MCDE_ALPHA_REGISTER = 0x1
  }mcde_blend_control;

  typedef enum
  {
	  MCDE_FORCE_FILTER0 = 0x0,
	  MCDE_ADAPTIVE_FILTER = 0x1,
	  MCDE_TEST_MODE = 0x2
  }mcde_flicker_filter_mode;


  typedef enum
  {
	  MCDE_ROTATION_DISABLE = 0x0,
	  MCDE_ROTATION_ENABLE = 0x1
  }mcde_roten;

  typedef enum
  {
	  MCDE_CLR_ENHANCE_DISABLE = 0x0,
	  MCDE_CLR_ENHANCE_ENABLE = 0x1
  }mcde_clr_enhance_ctrl;

  typedef enum
  {
	  MCDE_BLEND_DISABLE = 0x0,
	  MCDE_BLEND_ENABLE = 0x1
  }mcde_blend_status;


  typedef enum
  {
      MCDE_VERTICAL_ACTIVE_FRAME_DISABLE = 0x0,
      MCDE_ALL_FRAME_ENABLE = 0x1
  }mcde_va_enable;

  typedef enum
  {
      MCDE_OUTPUT_NORMAL = 0x0,
      MCDE_OUTPUT_TOGGLE = 0x1
  }mcde_toggle_enable;

  typedef enum
  {
	  MCDE_SYNCHRO_HBP = 0x0,
	  MCDE_SYNCHRO_CLP = 0x1,
	  MCDE_SYNCHRO_HFP = 0x2,
	  MCDE_SYNCHRO_HSW = 0x3
  }mcde_loadsel;

  typedef enum
  {
      MCDE_DATA_RISING_EDGE = 0x0,
      MCDE_DATA_FALLING_EDGE = 0x1
  }mcde_data_lines;

  struct mcde_chx_control0
  {
      mcde_rotate_req    chx_read_request;
      u8              alpha_blend;
      mcde_rot_dir       rot_dir;
      mcde_gamma_ctrl    gamma_ctrl;
      mcde_flicker_format flicker_format;
      mcde_flicker_filter_mode  filter_mode;
      mcde_blend_control     blend_ctrl;
      mcde_key_ctrl       key_ctrl;
      mcde_roten          rot_enable;
      mcde_dithering_control dither_ctrl;
      mcde_clr_enhance_ctrl color_enhance;
      mcde_antiflicker_ctrl anti_flicker;
      mcde_blend_status       blend;
  };
  struct mcde_chx_rgb_conv_coef
  {
	  u16  Yr_red;
	  u16  Yr_green;
	  u16  Yr_blue;
	  u16  Cr_red;
	  u16  Cr_green;
	  u16  Cr_blue;
	  u16  Cb_red;
	  u16  Cb_green;
	  u16  Cb_blue;
	  u16  Off_red;
	  u16  Off_green;
	  u16  Off_blue;
  };

  struct mcde_chx_flickfilter_coef
  {
	  u8  threshold_ctrl0;
	  u8  threshold_ctrl1;
	  u8  threshold_ctrl2;
	  u8  Coeff0_N3;
	  u8  Coeff0_N2;
	  u8  Coeff0_N1;
	  u8  Coeff1_N3;
	  u8  Coeff1_N2;
	  u8  Coeff1_N1;
	  u8  Coeff2_N3;
	  u8  Coeff2_N2;
	  u8  Coeff2_N1;
  };

  struct mcde_chx_tv_control
  {
	  u16  num_lines;
	  mcde_tvmode tv_mode;
	  mcde_signal_level ifield;
	  mcde_display_mode sel_mode;
  };

  struct mcde_chx_tv_blanking_field
  {
	  u16  blanking_start;
	  u16  blanking_end;
  };

struct mcde_chx_tv_blanking2_field
  {
          u16  blanking_start;
          u16  blanking_end;
  };


  struct mcde_chx_tv_start_line
  {
	  u16  field2_start_line;
	  u16  field1_start_line;
  };

  struct mcde_chx_tv_dvo_offset
  {
	  u16  field2_window_offset;
	  u16  field1_window_offset;
  };

  struct mcde_chx_tv_swh_time
  {
	  u16  tv_swh2;
	  u16  tv_swh1;
  };

  struct mcde_chx_tv_timing1
  {
      u16  src_window_width;
      u16  destination_hor_offset;
  };

  struct mcde_chx_tv_lbalw_timing
  {
	  u16 active_line_width;
	  u16 line_blanking_width;
  };

  struct mcde_chx_tv_background_time
  {
	  u8  background_cr;
	  u8  background_cb;
	  u8  background_lu;
  };

struct mcde_chx_lcd_timing0
  {
	  mcde_va_enable  rev_va_enable;
	  mcde_toggle_enable  rev_toggle_enable;
	  mcde_loadsel  rev_sync_sel;
	  u8  rev_delay1;
	  u8  rev_delay0;
	  mcde_va_enable  ps_va_enable;
	  mcde_toggle_enable  ps_toggle_enable;
	  mcde_loadsel  ps_sync_sel;
	  u8  ps_delay1;
	  u8  ps_delay0;
  };

  struct mcde_chx_lcd_timing1
  {
      mcde_signal_level   io_enable;
      mcde_data_lines     ipc;
      mcde_signal_level   ihs;
      mcde_signal_level   ivs;
      mcde_signal_level   ivp;
      mcde_signal_level   iclspl;
      mcde_signal_level   iclrev;
      mcde_signal_level   iclsp;
      mcde_va_enable      mcde_spl;
      mcde_toggle_enable  spltgen;
      mcde_loadsel        spl_sync_sel;
      u8               spl_delay1;
      u8               spl_delay0;
  };


   struct mcde_chx_palette
  {
	  u16  alphared;
	  u8  green;
	  u8  blue;
  };

  struct mcde_chx_gamma
  {
	  u8  red;
	  u8  green;
	  u8  blue;
  };
 typedef enum
 {
 MCDE_ROTATE0 = 0x0,
 MCDE_ROTATE1 = 0x1
}mcde_rotate_num;


  /*******************************************************************************
   Channel C Enums
   ******************************************************************************/

  typedef enum
  {
      MCDE_SYNCHRO_NONE = 0x0,
      MCDE_SYNCHRO_C0 = 0x1,
      MCDE_SYNCHRO_C1 = 0x2,
      MCDE_SYNCHRO_PINGPONG = 0x3
  }mcde_sync_ctrl;

  typedef enum
  {
      MCDE_SIG_INACTIVE_POL_LOW = 0x0,
      MCDE_SIG_INACTIVE_POL_HIGH = 0x1
  }mcde_sig_pol;

  typedef enum
  {
      MCDE_CD_LOW = 0x0, /* CD low for data and high for command */
      MCDE_CD_HIGH = 0x1 /* CD high for data and low for command */
  }mcde_cd_polarity;

  typedef enum
  {
      MCDE_RES_INACTIVE = 0x0,
      MCDE_RES_ACTIVE = 0x1
  }mcde_resen;

  typedef enum
  {
      MCDE_CSEN_DEACTIVATED = 0x0,
      MCDE_CSEN_ACTIVATED = 0x1
  }mcde_cs_enable_rw;

  typedef enum
  {
      MCDE_PCLK_TVCLK1 = 0x0,
      MCDE_PCLK_72 = 0x1,
      MCDE_PCLK_TVCLK2 = 0x2
  }mcde_clk_sel;

  typedef enum
  {
      MCDE_SELECT_OUTBAND = 0x0,
      MCDE_SELECT_INBAND = 0x1,
  }mcde_inband_select;

  typedef enum
  {
      MCDE_BUS_SIZE_8 = 0x0,
      MCDE_BUS_SIZE_16 = 0x1
  }mcde_bus_size;

  typedef enum
  {
      MCDE_SYNCHRO_CAPTURE_DISABLE = 0x0,
      MCDE_SYNCHRO_CAPTURE_ENABLE = 0x1
  }mcde_synchro_capture;

  typedef enum
  {
      MCDE_VERTICAL_SYNCHRO_CAPTURE1 = 0x0,
      MCDE_VERTICAL_SYNCHRO_CHANNELA = 0x1,
  }mcde_synchro_select;

  typedef enum
  {
      MCDE_FIFO_WMLVL_4 = 0x0,
      MCDE_FIFO_WMLVL_8 = 0x1
  }mcde_fifo_wmlvl_sel;

  typedef enum
  {
      MCDE_CHANNEL_C_DISABLE = 0x0,
      MCDE_CHANNEL_C_ENABLE = 0x1
  }mcde_chc_enable;

  typedef enum
  {
      MCDE_POWER_DISABLE = 0x0,
      MCDE_POWER_ENABLE = 0x1
  }mcde_powen_select;

  typedef enum
  {
      MCDE_FLOW_DISABLE = 0x0,
      MCDE_FLOW_ENABLE = 0x1
  }mcde_flow_select;

  typedef enum
  {
      MCDE_DUPLX_DISABLE = 0x0,
      MCDE_DUPLX_ENABLE  = 0x1
  }mcde_duplx_ctrl;

  typedef enum
  {
      MCDE_DUPLX_MODE_NONE = 0x0,
      MCDE_DUPLX_MODE_16_TO_32 = 0x1,
      MCDE_DUPLX_MODE_24_TO_32_RS = 0x2,
      MCDE_DUPLX_MODE_24_TO_32_LS = 0x3
  }mcde_duplx_mode_select;

  typedef enum
  {
      MCDE_TRANSFER_8_1 = 0x0,
      MCDE_TRANSFER_8_2 = 0x1,
      MCDE_TRANSFER_8_3 = 0x2,
      MCDE_TRANSFER_16_1 = 0x4,
      MCDE_TRANSFER_16_2 = 0x5
  }mcde_bit_segmentation_select;

  typedef enum
  {
      MCDE_TRANSACTION_COMMAND = 0x0,
      MCDE_TRANSACTION_DATA = 0x1
  }mcde_transaction_type;

  typedef enum
  {
      MCDE_VSYNC_SELECT = 0x0,
      MCDE_HSYNC_SELECT = 0x1
  }mcde_vertical_sync_sel;

  typedef enum
  {
      MCDE_VSYNC_ACTIVE_HIGH = 0x0,
      MCDE_VSYNC_ACTIVE_LOW = 0x1,
  }mcde_vertical_sync_polarity;

  typedef enum
  {
      MCDE_STBCLK_DIV_1 = 0x0,
      MCDE_STBCLK_DIV_2 = 0x1,
      MCDE_STBCLK_DIV_4 = 0x2,
      MCDE_STBCLK_DIV_8 = 0x3,
      MCDE_STBCLK_DIV_16 = 0x4,
      MCDE_STBCLK_DIV_32 = 0x5,
      MCDE_STBCLK_DIV_64 = 0x6,
      MCDE_STBCLK_DIV_128 = 0x7
  }mcde_synchro_clk_div_factor;

  typedef enum
  {
      MCDE_PANEL_INTEL_SERIES = 0x0,
      MCDE_PANEL_MOTOROLA_SERIES = 0x1
  }mcde_panel_protocol;

  typedef enum
  {
      MCDE_PANEL_C0 = 0x0,
      MCDE_PANEL_C1 = 0x1
  }mcde_chc_panel;

  typedef enum
 {
    MCDE_TXFIFO_WRITE_DATA           = 0,
    MCDE_TXFIFO_READ_DATA,
    MCDE_TXFIFO_WRITE_COMMAND
 } mcde_txfifo_request_type;


  struct mcde_chc_ctrl
  {
      mcde_sync_ctrl sync;
      mcde_resen    resen;
      mcde_synchro_select  synsel;
      mcde_clk_sel   clksel;
  };

  struct mcde_chc_config
  {
      mcde_sig_pol  res_pol;
      mcde_sig_pol  rd_pol;
      mcde_sig_pol  wr_pol;
      mcde_cd_polarity  cd_pol;
      mcde_sig_pol  cs_pol;
      mcde_cs_enable_rw csen;
      mcde_inband_select inband_mode;
      mcde_bus_size      bus_size;
      mcde_synchro_capture syncen;
      mcde_fifo_wmlvl_sel  fifo_watermark;
      mcde_chc_enable      chcen;
  };

  struct mcde_pbc_config
  {
      mcde_duplx_ctrl       duplex_ctrl;
      mcde_duplx_mode_select  duplex_mode;
      mcde_bit_segmentation_select data_segment;
      mcde_bit_segmentation_select cmd_segment;
  };

  struct mcde_pbc_mux
  {
      u32  imux0;
      u32  imux1;
      u32  imux2;
      u32  imux3;
      u32  imux4;
  };

  struct mcde_pbc_bitctrl
  {
      u32  bit_ctrl0;
      u32  bit_ctrl1;
  };

  struct mcde_sync_conf
  {
      u8  debounce_length;
      mcde_vertical_sync_sel sync_sel;
      mcde_vertical_sync_polarity sync_pol;
      mcde_synchro_clk_div_factor clk_div;
      u16  vsp_max;
      u16 vsp_min;
  };

  struct mcde_sync_trigger
  {
      u16 trigger_delay_cx;
      u8  sync_delay_c1;
      u8  sync_delay_c2;
  };

  struct mcde_cd_timing_activate
  {
      u8 cs_cd_deactivate;
      u8 cs_cd_activate;
  };

  struct mcde_rw_timing
  {
      mcde_panel_protocol panel_type;
      u8  readwrite_activate;
      u8  readwrite_deactivate;
  };

  struct mcde_data_out_timing
  {
      u8  data_out_deactivate;
      u8  data_out_activate;
  };

  /*********************************************************************
   DSIX typedefs
   *********************************************************************/
   typedef enum
   {
	   MCDE_DSI_CH_VID0 = 0x0,
	   MCDE_DSI_CH_CMD0 = 0x1,
	   MCDE_DSI_CH_VID1 = 0x2,
	   MCDE_DSI_CH_CMD1 = 0x3,
	   MCDE_DSI_CH_VID2 = 0x4,
	   MCDE_DSI_CH_CMD2 = 0x5
   }mcde_dsi_channel;

   typedef enum
   {
	   MCDE_PLL_OUT_OFF = 0x0,
	   MCDE_PLL_OUT_1	= 0x1,
	   MCDE_PLL_OUT_2	= 0x2,
	   MCDE_PLL_OUT_4	= 0x3
   }mcde_pll_div_sel;

   typedef enum
   {
	   MCDE_CLK27 = 0x0,
	   MCDE_TV1CLK = 0x1,
	   MCDE_HDMICLK = 0x2,
	   MCDE_TV2CLK = 0x3,
	   MCDE_MXTALI = 0x4
   }mcde_pll_ref_clk;

   typedef enum
   {
	   MCDE_DSI_CLK27 = 0x0,
	   MCDE_DSI_MCDECLK = 0x1
   }mcde_clk_divider;

   typedef struct
   {
	   mcde_pll_div_sel  pllout_divsel2;
	   mcde_pll_div_sel  pllout_divsel1;
	   mcde_pll_div_sel  pllout_divsel0;
	   mcde_pll_ref_clk  pll4in_sel;
	   mcde_clk_divider  txescdiv_sel;
	   u32 		   txescdiv;
   }mcde_dsi_clk_config;

   typedef enum
   {
	   MCDE_PACKING_RGB565 = 0x0,
	   MCDE_PACKING_RGB666 = 0x1,
	   MCDE_PACKING_RGB888_R = 0x2,
	   MCDE_PACKING_RGB888_B = 0x3,
	   MCDE_PACKING_HDTV	 = 0x4
   }mcde_dsi_packing;

   typedef enum
   {
	   MCDE_DSI_OUT_GENERIC_CMD = 0x0,
	   MCDE_DSI_OUT_VIDEO_DCS	= 0x1
   }mcde_dsi_synchro;

   typedef enum
   {
	   MCDE_DSI_NO_SWAP = 0x0,
	   MCDE_DSI_SWAP	= 0x1,
   }mcde_dsi_swap;

   typedef enum
   {
	   MCDE_DSI_CMD_16	= 0x0,
	   MCDE_DSI_CMD_8	= 0x1
   }mcde_dsi_cmd;

   typedef enum
   {
	   MCDE_DSI_CMD_MODE = 0x0,
	   MCDE_DSI_VID_MODE = 0x1,
   }mcde_vid_mode;

   typedef struct
   {
	   mcde_dsi_packing  packing;
	   mcde_dsi_synchro  synchro;
	   mcde_dsi_swap	   byte_swap;
	   mcde_dsi_swap	   bit_swap;
	   mcde_dsi_cmd		   cmd8;
	   mcde_vid_mode	   vid_mode;
	   u8			   blanking;
	   u32 		   words_per_frame;
	   u32 		   words_per_packet;
    }mcde_dsi_conf;



/*******************************************************************************
MCDE Error Enums
******************************************************************************/
typedef enum
{
   MCDE_OK				   = 0x00,	   /* No error.*/
   MCDE_NO_PENDING_EVENT_ERROR		   = 0x01,
   MCDE_NO_MORE_FILTER_PENDING_EVENT	   = 0x02,
   MCDE_NO_MORE_PENDING_EVENT 		   = 0x03,
   MCDE_REMAINING_FILTER_PENDING_EVENTS    = 0x04,
   MCDE_REMAINING_PENDING_EVENTS	   = 0x05,
   MCDE_INTERNAL_EVENT			   = 0x06,
   MCDE_INTERNAL_ERROR			   = 0x07,
   MCDE_NOT_CONFIGURED			   = 0x08,
   MCDE_REQUEST_PENDING			   = 0x09,
   MCDE_REQUEST_NOT_APPLICABLE		   = 0x0A,
   MCDE_INVALID_PARAMETER 		   = 0x0B,
   MCDE_UNSUPPORTED_FEATURE		   = 0x0C,
   MCDE_UNSUPPORTED_HW			   = 0x0D
}mcde_error;

/*******************************************************************************
MCDE driver specific structures
******************************************************************************/

typedef enum {
        CHANNEL_A = 0x0,
        CHANNEL_B = 0x1,
       CHANNEL_C0 = 0x2,
        CHANNEL_C1 = 0x3,
}channel ;

typedef enum {
       RES_QVGA = 0x0,
       RES_VGA = 0x1,
       RES_WVGA = 0x2,
}screenres;

 typedef struct
 {
	u16  alphared;
	u8  green;
	u8  blue;
 }mcde_palette;

typedef struct
{
	u8  red;
	u8  green;
	u8  blue;
}t_mcde_gamma;

struct mcde_channel_data{
       channel channelid;
       u8 nopan;
       u8 nowrap;
       const char *restype;
       mcde_bpp_ctrl inbpp;
       mcde_out_bpp outbpp;
       u8 bpp16_type;
       u8 bgrinput;
       gpio_alt_function gpio_alt_func;
};
// per channel structure
struct mcde_ovlextsrc_conf{
	u16 ovl_id;           //bitmap of overlays used
	u8 num_ovl;
};

struct clut_addrmap {
	u32 clutaddr;
	u32 clutdmaaddr;
};


#endif //__KERNEL__

/* FUNCTION PROTOTYPE */
void mcdefb_enable(struct fb_info *info);
void mcdefb_disable(struct fb_info *info);
int mcde_enable(struct fb_info *info);
int mcde_disable(struct fb_info *info);
int convertbpp(u8 bpp);
int  mcde_conf_channel_color_key(struct fb_info *info, struct mcde_channel_color_key chnannel_color_key);
/*inline */unsigned long get_line_length(int x, int bpp);
/* inline */unsigned long claim_mcde_lock(mcde_ch_id chid);
/*inline*/ void release_mcde_lock(mcde_ch_id chid, unsigned long flags);
int  mcde_set_buffer(struct fb_info *info, u32 buffer_address, mcde_buffer_id buff_id);
int  mcde_conf_dithering_ctrl(struct mcde_dithering_ctrl_conf dithering_ctrl_conf, struct fb_info *info);
bool mcde_get_hdmi_flag(void);
void mcde_configure_hdmi_channel(void);
#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT
/* HW V1 */
void mcde_hdmi_display_init_command_mode(mcde_video_mode video_mode);
#else
/* HW ED */
void mcde_hdmi_display_init_command_mode(void);
#endif
void mcde_hdmi_display_init_video_mode(void);
void mcde_hdmi_test_directcommand_mode_highspeed(void);
void mcde_send_hdmi_cmd_data(char* buf,int length, int dsicommand);
void mcde_send_hdmi_cmd(char* buf,int length, int dsicommand);

int  mcde_extsrc_ovl_create(struct mcde_overlay_create *extsrc_ovl ,struct fb_info *info,u32 *pkey);
int  mcde_extsrc_ovl_remove(struct fb_info *info,u32 key);
int  mcde_alloc_source_buffer(struct mcde_sourcebuffer_alloc source_buff ,struct fb_info *info, u32 *pkey, u8 isUserRequest);
int  mcde_dealloc_source_buffer(struct fb_info *info, u32 srcbufferindex, u8 isUserRequest);
int  mcde_conf_extsource(struct mcde_ext_conf ext_src_config ,struct fb_info *info);
int  mcde_conf_overlay(struct mcde_conf_overlay ovrlayConfig ,struct fb_info *info);
int  mcde_conf_channel(struct mcde_ch_conf ch_config ,struct fb_info *info);
int  mcde_conf_lcd_timing(struct mcde_chnl_lcd_ctrl chnl_lcd_ctrl, struct fb_info *info);
int  mcde_conf_color_conversion_coeff(struct fb_info *info, mcde_colorconv_type color_conv_type);
int  mcde_conf_color_conversion(struct fb_info *info, struct mcde_conf_color_conv color_conv_ctrl);
int  mcde_conf_blend_ctrl(struct fb_info *info, struct mcde_blend_control blend_ctrl);
int  mcde_conf_rotation(struct fb_info *info, mcde_rot_dir rot_dir, mcde_roten rot_ctrl, u32 rot_addr0, u32 rot_addr1);
int  mcde_conf_chnlc(struct mcde_chc_config chnlc_config, struct mcde_chc_ctrl chnlc_control, struct fb_info *info);
int  mcde_conf_dsi_chnl(mcde_dsi_conf dsi_conf, mcde_dsi_clk_config clk_config, struct fb_info *info);
int  mcde_conf_scan_mode(mcde_scan_mode scan_mode, struct fb_info *info);

mcde_error mcdesetextsrcconf(mcde_ch_id chid, mcde_ext_src src_id, struct mcde_ext_conf config);
mcde_error mcdesetextsrcctrl(mcde_ch_id chid, mcde_ext_src src_id, struct mcde_ext_src_ctrl control);
mcde_error mcdesetbufferaddr(mcde_ch_id chid, mcde_ext_src src_id, mcde_buffer_id buffer_id, u32 address);
mcde_error mcdesetovrctrl(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_control ovr_cr);
mcde_error mcdesetovrlayconf(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_config ovr_conf);
mcde_error mcdesetovrconf2(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_conf2 ovr_conf2);
mcde_error mcdesetovrljinc(mcde_ch_id chid, mcde_overlay_id overlay, u32 ovr_ljinc);
mcde_error mcdesettopleftmargincrop(mcde_ch_id chid, mcde_overlay_id overlay, u32 ovr_topmargin, u16 ovr_leftmargin);
mcde_error mcdesetovrcomp(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_comp ovr_comp);
mcde_error mcdesetovrclip(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_clip ovr_clip);
mcde_error mcdesetovrstate(mcde_ch_id chid, mcde_overlay_id overlay, mcde_overlay_ctrl state);
mcde_error mcdesetovrpplandlpf(mcde_ch_id chid, mcde_overlay_id overlay, u16 ppl, u16 lpf);
mcde_error mcdesetstate(mcde_ch_id chid, mcde_state state);
mcde_error mcdesetchnlXconf(mcde_ch_id chid, u16 channelnum, struct mcde_chconfig config);
mcde_error mcdesetswsync(mcde_ch_id chid, u16 channelnum, mcde_sw_trigger sw_trig);
mcde_error mcdesetchnlbckgndcol(mcde_ch_id chid, u16 channelnum, struct mcde_ch_bckgrnd_col color);
mcde_error mcdesetchnlsyncprio(mcde_ch_id chid, u16 channelnum, u32 priority);
mcde_error mcdesetchnlXpowermode(mcde_ch_id chid, mcde_powen_select power);
mcde_error mcdesetflowXctrl(mcde_ch_id chid, struct mcde_chx_control0 control);
mcde_error mcdesetpanelctrl(mcde_ch_id chid, struct mcde_chx_control1 control);
mcde_error mcdesetcolorkey(mcde_ch_id chid, struct mcde_chx_color_key key, mcde_colorkey_type type);
mcde_error mcdesetcolorconvmatrix(mcde_ch_id chid, struct mcde_chx_rgb_conv_coef coef);
mcde_error mcdesetflickerfiltercoef(mcde_ch_id chid, struct mcde_chx_flickfilter_coef coef);
mcde_error mcdesetLCDtiming0ctrl(mcde_ch_id chid, struct mcde_chx_lcd_timing0 control);
mcde_error mcdesetLCDtiming1ctrl(mcde_ch_id channel, struct mcde_chx_lcd_timing1 control);
mcde_error mcdesetrotaddr(mcde_ch_id chid, u32 address, mcde_rotate_num rotnum);
mcde_error mcdesetpalette(mcde_ch_id chid, mcde_palette palette);
mcde_error mcdesetditherctrl(mcde_ch_id chid, struct mcde_chx_dither_ctrl control);
mcde_error mcdesetditheroffset(mcde_ch_id chid, struct mcde_chx_dithering_offset offset);
mcde_error mcdesetgammacoeff(mcde_ch_id chid, struct mcde_chx_gamma gamma);
mcde_error mcdesetoutmuxconf(mcde_ch_id chid, mcde_out_bpp outbpp);
mcde_error mcdesetchnlXpowermode(mcde_ch_id chid, mcde_powen_select power);
mcde_error mcdesetchnlXflowmode(mcde_ch_id chid, mcde_flow_select flow);
mcde_error mcdesetcflowXcolorkeyctrl(mcde_ch_id chid, mcde_key_ctrl key_ctrl);
mcde_error mcdesetblendctrl(mcde_ch_id chid, struct mcde_blend_control blend_ctrl);
mcde_error mcdesetrotation(mcde_ch_id chid, mcde_rot_dir rot_dir, mcde_roten rot_ctrl);
mcde_error mcdesetcolorconvctrl(mcde_ch_id chid, mcde_overlay_id overlay, mcde_col_conv_ctrl  col_ctrl);
mcde_error mcderesetextsrcovrlay(mcde_ch_id chid);
mcde_error mcdesetchnlsyncsrc(mcde_ch_id chid, u16 channelnum, struct mcde_chsyncmod sync_mod);
mcde_error mcdesetchnlsyncevent(mcde_ch_id chid, struct mcde_ch_conf ch_config);
mcde_error mcdesetchnlLCDctrlreg(mcde_ch_id chid, struct mcde_chnl_lcd_ctrl_reg lcd_ctrl_reg);
mcde_error mcdesetchnlLCDhorizontaltiming(mcde_ch_id chid, struct mcde_chnl_lcd_horizontal_timing lcd_horizontal_timing);
mcde_error mcdesetchnlLCDverticaltiming(mcde_ch_id chid, struct mcde_chnl_lcd_vertical_timing lcd_vertical_timing);
mcde_error mcdesetditheringctrl(mcde_ch_id chid, mcde_dithering_ctrl dithering_control);
mcde_error mcdesetscanmode(mcde_ch_id chid, mcde_scan_mode scan_mode);

#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT
/* HW V1 */
mcde_state mcdegetchannelstate(mcde_ch_id chid);
mcde_error mcdesetchnlCmode(mcde_ch_id chid, mcde_chc_panel panel_id, mcde_chc_enable state);
#endif /* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

#endif

