/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License terms:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <mach/av8100.h>

/* defines for av8100_ */
#define AV8100_COMMAND_OFFSET		0x10
#define AV8100_COMMAND_MAX_LENGTH	0x81
#define GPIO_AV8100_RSTN 196
#define GPIO_AV8100_INT 192
#define AV8100_DRIVER_MINOR_NUMBER 240

/* Standby register */
#define STANDBY_HPDS_HDMI_PLUGGED		0x04
#define STANDBY_CVBS_TV_CABLE_PLUGGED	0x08

/* Standby interrupts */
#define HDMI_HOTPLUG_INTERRUPT 			0x1
#define HDMI_HOTPLUG_INTERRUPT_MASK		0xFE
#define CVBS_PLUG_INTERRUPT				0x2
#define CVBS_PLUG_INTERRUPT_MASK		0xFD
#define STANDBY_INTERRUPT_MASK_ALL		0xF0

/* General interrupts */
#define UNDER_OVER_FLOW_INTERRUPT		0x20
#define UNDER_OVER_FLOW_INTERRUPT_MASK	0xDF
#define TE_INTERRUPT					0x40
#define TE_INTERRUPT_MASK				0xCF
#define GENERAL_INTERRUPT_MASK_ALL		0x00

/* General status */
#define AV8100_GENERAL_STATUS_UC_READY 0x8

#define REG_16_8_LSB(p)  (unsigned char)(p & 0xFF)
#define REG_16_8_MSB(p)  (unsigned char)((p & 0xFF00)>>8)
#define REG_32_8_MSB(p)  (unsigned char)((p & 0xFF000000)>>24)
#define REG_32_8_MMSB(p) (unsigned char)((p & 0x00FF0000)>>16)
#define REG_32_8_MLSB(p) (unsigned char)((p & 0x0000FF00)>>8)
#define REG_32_8_LSB(p)   (unsigned char)(p & 0x000000FF)

/**
 * struct av8100_cea - CEA(consumer electronic access) standard structure
 * @cea_id:
 * @cea_nb:
 * @vtotale:
 **/

 typedef struct {
     char cea_id[40] ;
     int cea_nb ;
     int vtotale;
     int vactive;
     int vsbp ;
     int vslen ;
     int vsfp;
     char vpol[5];
     int htotale;
     int hactive;
     int hbp ;
     int hslen ;
     int hfp;
     int frequence;
     char hpol[5];
     int reg_line_duration;
     int blkoel_duration;
     int uix4;
     int pll_mult;
     int pll_div;
}av8100_cea;

/**
 * struct av8100_data - av8100_ internal structure
 * @client:	pointer to i2c client
 * @work:	work_struct scheduled during bottom half
 * @sem:	semaphore used for data protection
 * @device_type: hdmi or cvbs
 * @edid:	extended display identification data
 **/
struct av8100_data{
    struct i2c_client	*client;
    struct work_struct	work;
    struct semaphore	sem;
    char   device_type;
    char   edid[127];
};

/**
 * struct av8100_platform_data - av8100_ platform data
 * @irq:	irq num
 **/
struct av8100_platform_data {
    unsigned	gpio_base;
    int irq;
};

typedef struct {
    u16	c0;
    u16	c1;
    u16	c2;
    u16	c3;
    u16	c4;
    u16	c5;
    u16	c6;
    u16	c7;
    u16	c8;
    u16	a_offset;
    u16	b_offset;
    u16	c_offset;
    u8	l_max;
    u8	l_min;
    u8	c_max;
    u8	c_min;
} av8100_color_space_conversion_cmd;

/**
 * struct av8100_video_input_format_cmd - video input format structure
 * @dsi_input_mode:
 * @input_pixel_format:
 * @total_horizontal_pixel:
 **/
typedef struct {
    av8100_dsi_mode            dsi_input_mode;
    av8100_pixel_format        input_pixel_format;
    unsigned short                      total_horizontal_pixel;           /*number of total horizontal pixels in the frame*/
    unsigned short                      total_horizontal_active_pixel;    /*number of total horizontal active pixels in the frame*/
    unsigned short                      total_vertical_lines;             /*number of total vertical lines in the frame*/
    unsigned short                      total_vertical_active_lines;      /*number of total vertical active lines*/
    av8100_video_mode          video_mode;
    av8100_dsi_nb_data_lane    nb_data_lane;
    unsigned char                       nb_virtual_ch_command_mode;
    unsigned char                       nb_virtual_ch_video_mode;
    unsigned short                      TE_line_nb;                        /* Tearing effect line number*/
    av8100_te_config           TE_config;
    unsigned long                      master_clock_freq;                  /* Master clock frequency in HZ */
    unsigned char			ui_x4;
} av8100_video_input_format_cmd;

/**
 * struct av8100_video_output_format_cmd - video output format structure
 * @dsi_input_mode:
 * @input_pixel_format:
 * @total_horizontal_pixel:
 **/
typedef struct {
    av8100_output_CEA_VESA       video_output_cea_vesa;
    av8100_video_sync_pol        vsync_polarity;
    av8100_video_sync_pol        hsync_polarity;
    unsigned short                       total_horizontal_pixel;           /*number of total horizontal pixels in the frame*/
    unsigned short                       total_horizontal_active_pixel;    /*number of total horizontal active pixels in the frame*/
    unsigned short                       total_vertical_in_half_lines;             /*number of total vertical lines in the frame*/
    unsigned short                       total_vertical_active_in_half_lines;      /*number of total vertical active lines*/
    unsigned short                       hsync_start_in_pixel;
    unsigned short                       hsync_length_in_pixel;
    unsigned short                       vsync_start_in_half_line;
    unsigned short                       vsync_length_in_half_line;
    unsigned long                       pixel_clock_freq_Hz;
} av8100_video_output_format_cmd;
/**
 * struct av8100_pattern_generator_cmd - pattern generator format structure
 * @pattern_type:
 * @pattern_video_format:
 * @pattern_audio_mode:
 **/
typedef struct {
    av8100_pattern_type          pattern_type;
    av8100_pattern_format        pattern_video_format;
    av8100_pattern_audio         pattern_audio_mode;
} av8100_pattern_generator_cmd;
/**
 * struct av8100_audio_input_format_cmd - audio input format structure
 * @audio_input_if_format:
 * @i2s_input_nb:
 * @sample_audio_freq:
 **/
typedef struct {
    av8100_audio_if_format       audio_input_if_format;            /* mode of the MSP*/
    unsigned char                         i2s_input_nb;                    /* 0, 1 2 3 4*/
    av8100_sample_freq           sample_audio_freq;
    av8100_audio_word_length     audio_word_lg;
    av8100_audio_format          audio_format;
    av8100_audio_if_mode         audio_if_mode;
    av8100_audio_mute            audio_mute;
} av8100_audio_input_format_cmd;
/**
 * struct av8100_video_scaling_format_cmd - video scaling format structure
 * @h_start_in_pixel:
 * @h_stop_in_pixel:
 * @v_start_in_line:
 **/
typedef struct {
    unsigned short                       h_start_in_pixel;
    unsigned short                       h_stop_in_pixel;
    unsigned short                       v_start_in_line;
    unsigned short                       v_stop_in_line;
} av8100_video_scaling_format_cmd;
/**
 * struct av8100_hdmi_cmd - hdmi command structure
 * @hdmi_mode:
 * @hdmi_format:
 * @dvi_format:
 **/
typedef struct {
    av8100_hdmi_mode             hdmi_mode;
    av8100_hdmi_format           hdmi_format;
    av8100_DVI_format            dvi_format; /* used only if HDMI_format = DVI*/
} av8100_hdmi_cmd;

/**
 * struct av8100_denc_cmd - denc command structure
 **/
typedef struct {
	av8100_TV_lines                    tv_lines;
	av8100_TV_std                      tv_std;
	av8100_DENC_State                  denc;
	av8100_macrovision_state           macrovision;
	av8100_internal_generator_state    internal_generator;
	av8100_chroma_cws_capture_state    chroma;
} av8100_denc_cmd;

/* STWav8100 Private functions */
