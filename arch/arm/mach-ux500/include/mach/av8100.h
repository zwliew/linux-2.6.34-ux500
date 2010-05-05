/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License terms:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef __AV8100_H
#define __AV8100_H

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

typedef enum av8100_error
{
	AV8100_OK = 0x0,
	AV8100_INVALID_COMMAND = 0x1,
	AV8100_INVALID_INTERFACE = 0x2,
	AV8100_INVALID_IOCTL	 = 0x3,
	AV8100_COMMAND_FAIL = 0x4,
	AV8100_FWDOWNLOAD_FAIL = 0x5,
	AV8100_FAIL = 0xFF,
}av8100_error;

typedef enum interface
{
	I2C_INTERFACE = 0x0,
	DSI_INTERFACE = 0x1,
}interface;

/** AV8100 - DSI dcs command set */
typedef enum
{
    DCS_VSYNC_START 	= 0x1,
	DCS_VSYNC_END 		= 0x11,
	DCS_HSYNC_START 	= 0x21,
	DCS_HSYNC_END 		= 0x31,
	DCS_SHORT_WRITE 	= 0x15,
	DCS_LONG_WRITE 		= 0x39,
	DCS_RGB565_PACKED 	= 0xE,
	DCS_RGB666_PACKED 	= 0x1E,
	DCS_RGB666_UNPACKED 	= 0x2E,
	DCS_RGB888_PACKED 	= 0x3E,
	DCS_RAM_WRITE 		= 0x3C,
	DCS_RAM_WRITE_CONTINUE 	= 0x2C,
	DCS_FW_DOWNLOAD 	= 0xDB,
	DCS_WRITE_UC 		= 0xDC,
	DCS_READ_UC 		= 0xDB,
	DCS_NEXT_FILED_TYPE	= 0xDA,
	DCS_EXEC_UC 		= 0xDD,
}dsi_dcs_command_type;

/** AV8100 Operating modes */
typedef enum
{
	AV8100_OPMODE_SHUTDOWN = 0x1,
	AV8100_OPMODE_STANDBY,
	AV8100_OPMODE_SCAN,
	AV8100_OPMODE_INIT,
	AV8100_OPMODE_IDLE,
	AV8100_OPMODE_VIDEO
}av8100_operating_mode;

/** AV8100 status */
#define AV8100_PLUGIN_NONE 0x00
#define AV8100_HDMI_PLUGIN 0x01
#define	AV8100_CVBS_PLUGIN 0x02

/** AV8100 Command Type */
typedef enum
{
	AV8100_COMMAND_VIDEO_INPUT_FORMAT  = 0x1,
	AV8100_COMMAND_AUDIO_INPUT_FORMAT  = 0x2,
	AV8100_COMMAND_VIDEO_OUTPUT_FORMAT = 0x3,
	AV8100_COMMAND_VIDEO_SCALING_FORMAT,
	AV8100_COMMAND_COLORSPACECONVERSION,
	AV8100_COMMAND_CEC_MESSAGEWRITE,
	AV8100_COMMAND_CEC_MESSAGEREAD_BACK,
	AV8100_COMMAND_DENC,
	AV8100_COMMAND_HDMI,
	AV8100_COMMAND_HDCP_SENDKEY,
	AV8100_COMMAND_HDCP_MANAGEMENT,
	AV8100_COMMAND_INFOFRAMES,
	AV8100_COMMAND_EDID_SECTIONREADBACK,
	AV8100_COMMAND_PATTERNGENERATOR,

}av8100_command_type;

/** AV8100 Command Type */
typedef enum
{
	AV8100_COMMAND_VIDEO_INPUT_FORMAT_SIZE  = 0x17,
	AV8100_COMMAND_AUDIO_INPUT_FORMAT_SIZE  = 0x8,
	AV8100_COMMAND_VIDEO_OUTPUT_FORMAT_SIZE = 0x18,
	AV8100_COMMAND_VIDEO_SCALING_FORMAT_SIZE = 0x9,
	AV8100_COMMAND_COLORSPACECONVERSION_SIZE = 0x21,
	AV8100_COMMAND_CEC_MESSAGEWRITE_SIZE = 0x14,
	AV8100_COMMAND_CEC_MESSAGEREAD_BACK_SIZE = 0x14,
	AV8100_COMMAND_DENC_SIZE = 0x7,
	AV8100_COMMAND_HDMI_SIZE = 0x4,
	AV8100_COMMAND_HDCP_SENDKEY_SIZE = 0x9,
	AV8100_COMMAND_HDCP_MANAGEMENT_SIZE = 0x4,
	AV8100_COMMAND_INFOFRAMES_SIZE = 0x22,
	AV8100_COMMAND_EDID_SECTIONREADBACK_SIZE  = 0x81,
	AV8100_COMMAND_PATTERNGENERATOR_SIZE  = 0x4,
}av8100_command_size;

/** AV8100 structures register & command definitions */

/** AV8100 Internal registers ~ Register 0x0 to 0xF */
/** Internal registers for I2C operations */

typedef enum
{
	STANDBY_REG 					= 0x0,
	AV8100_5_VOLT_TIME_REG 			= 0x1,
	STANDBY_INTERRUPT_MASK_REG 		= 0x2,
	STANDBY_PENDING_INTERRUPT_REG 	= 0x3,
	GENERAL_INTERRUPT_MASK_REG 		= 0x4,
	GENERAL_INTERRUPT_REG 			= 0x5,
	GENERAL_STATUS_REG 				= 0x6,
	GPIO_CONFIGURATION_REG 			= 0x7,
	GENERAL_CONTROL_REG 			= 0x8,
	FIRMWARE_DOWNLOAD_ENTRY_REG 	= 0xF
}internal_reg;

struct av8100_registers_internal
{
    volatile char standby;
    volatile char av8100_5_volt_time;
    volatile char standby_interrupt_mask;
    volatile char standby_pending_interrupt;
    volatile char general_interrupt_mask;
    volatile char general_interrupt;
    volatile char general_status;
    volatile char gpio_configuration;
    volatile char general_control;
    volatile char firmware_download_entry;
};

/** AV8100 Video Input Format Command */
struct av8100_video_input_format_command
{

	volatile char Identifier;
	volatile char Mode;
	volatile char Pixel_format;
	volatile char Htotal[2];
	volatile char Hactive[2];
	volatile char Vtotal[2];
	volatile char Vactive[2];
	volatile char Videomode;
	volatile char Number_DSI;
	volatile char Virtualchannelcommandmode;
	volatile char Virtualchannelvideomode;
	volatile char Linenumber[2];
	volatile char Tearingeffect;
	volatile char Master_Clock_frequency[4];
};

/** AV8100 Audio Input Format Command */
struct av8100_audio_input_format_command
{

	volatile char Identifier;
	volatile char I2SOrTDM;
	volatile char NumberofI2SentriesFreq;
	volatile char BitFormat;
	volatile char LPCMOrCompress;
	volatile char SlaveOrMaster;
	volatile char Mute;

};

/** AV8100 Video Output Format Command */
struct av8100_video_output_format_command
{

	volatile char 	Identifier;
	volatile char 	Formatter;
	volatile char 	VSYNCpolarity;
	volatile char 	HSYNCpolarity;
	volatile char 	Htotal[2];
	volatile char 	Hactive[2];
	volatile char 	Vtotal[2];
	volatile char 	Vactive[2];
	volatile char 	HSYNCstart[2];
	volatile char 	HSYNClength[2];
	volatile char 	VSYNCstart[2];
	volatile char 	VSYNClength[2];
	volatile char 	Pixelfrequency[4];
};

/** AV8100 Video Video Scaling Format Command */
struct av8100_video_scaling_format_command
{
	volatile char Identifier;
	volatile char Hstart[2];
	volatile char Hstop[2];
	volatile char Vstart[2];
	volatile char Vstop[2];
};

/** AV8100 Video Colorspace Conversion Command */
struct av8100_colorspace_conversion_command
{
	volatile char Identifier;
	volatile char C0[2];
	volatile char C1[2];
	volatile char C2[2];
	volatile char C3[2];
	volatile char C4[2];
	volatile char C5[2];
	volatile char C6[2];
	volatile char C7[2];
	volatile char C8[21];
	volatile char AOFFSET[2];
	volatile char BOFFSET[2];
	volatile char COFFSET[2];
	volatile char AMINIMUM;
	volatile char AMAXIMUM;
	volatile char BMINIMUM;
	volatile char BMAXIMUM;
	volatile char CMINIMUM;
	volatile char CMAXIMUM;
};

/** AV8100 Video CEC message */
struct av8100_CEC_message
{
	volatile char Identifier;
	volatile char PhysicaladdressAB;
	volatile char PhysicaladdressCD;
	volatile char Bufferlength;
	volatile char BufferData[16];
};

/** AV8100 Video CEC message Readback Command */
struct av8100_CEC_message_readback_command
{
	volatile char Identifier;
};

/** AV8100 Video HDMI Command */
struct av8100_HDMI_command
{
	volatile char Identifier;
	volatile char OFF_ON_AVMUTE;
	volatile char HDMI_DVI;
	volatile char DVIcontrolbit;
};

/** AV8100 Video HDCP sendkey Command */
struct av8100_HDCP_sendkey_command
{
	volatile char Identifier;
	volatile char Keynumber;
	volatile char Key[7];
};

/** AV8100 Video HDCP management Command */
struct av8100_HDCP_management_command
{
	volatile char  Identifier;
	volatile char  RequestHDCPauthentication;
	volatile char  Requestencryptedtransmission;
	volatile char  OESS_EESS;
};

/** AV8100 Video Infoframe Command */
struct av8100_Infoframe_command
{
	volatile char  Identifier;
	volatile char  Infoframetype;
	volatile char  Infoframedata[30];
	volatile char  InfoframeCRC;
};

/** AV8100 Video EDID section readback Command */
struct av8100_EDIDsectionreadback_command
{
	volatile char  Identifier;
	volatile char  EDIDaddress;
	volatile char  EDIDblocknumber;
};

/** AV8100 Video Pattern Generator Command */
struct av8100_PatternGenerator_command
{
	volatile char  Identifier;
	volatile char  Testtypeselection;
	volatile char  VideoPatterngeneratorselection;
	volatile char  AudioSound;
};

/** AV8100 Video Command return */
struct av8100_command_return
{
	volatile char  Identifier;
	volatile char  OK_FAIL;
};

/** AV8100 Video CEC messgae readback command */
struct av8100_EDID_section_readback
{
	volatile char  Identifier;
	volatile char  OK_FAIL;
	volatile char  EDID[128];
};

typedef enum{
	AV8100_AUDIO_I2S_MODE,
	AV8100_AUDIO_I2SDELAYED_MODE, /* I2S Mode by default*/
	AV8100_AUDIO_TDM_MODE         /* 8 Channels by default*/
} av8100_audio_if_format;

typedef enum{
	AV8100_AUDIO_MUTE_DISABLE,
	AV8100_AUDIO_MUTE_ENABLE
} av8100_audio_mute;

typedef enum{
	AV8100_AUDIO_SLAVE,
	AV8100_AUDIO_MASTER
} av8100_audio_if_mode;

typedef enum{
	AV8100_AUDIO_LPCM_MODE,
	AV8100_AUDIO_COMPRESS_MODE
} av8100_audio_format;

typedef enum{
	AV8100_AUDIO_16BITS,
	AV8100_AUDIO_20BITS,
	AV8100_AUDIO_24BITS
} av8100_audio_word_length;

typedef enum{
	AV8100_AUDIO_FREQ_32KHZ,
	AV8100_AUDIO_FREQ_44_1KHZ,
	AV8100_AUDIO_FREQ_48KHZ,
	AV8100_AUDIO_FREQ_64KHZ,
	AV8100_AUDIO_FREQ_88_2KHZ,
	AV8100_AUDIO_FREQ_96KHZ,
	AV8100_AUDIO_FREQ_128KHZ,
	AV8100_AUDIO_FREQ_176_1KHZ,
	AV8100_AUDIO_FREQ_192KHZ
} av8100_sample_freq;


typedef enum{
	AV8100_PATTERN_AUDIO_OFF,
	AV8100_PATTERN_AUDIO_ON,
	AV8100_PATTERN_AUDIO_I2S_MEM
} av8100_pattern_audio;

typedef enum{
	AV8100_PATTERN_OFF,
	AV8100_PATTERN_GENERATOR,
	AV8100_PRODUCTION_TESTING
} av8100_pattern_type;

typedef enum{
	AV8100_NO_PATTERN,
	AV8100_PATTERN_VGA,
	AV8100_PATTERN_720P,
	AV8100_PATTERN_1080P
} av8100_pattern_format;

typedef enum{
	AV8100_HDMI_OFF,
	AV8100_HDMI_ON,
	AV8100_HDMI_AVMUTE
} av8100_hdmi_mode;

typedef enum{
	AV8100_HDMI,
	AV8100_DVI
} av8100_hdmi_format;

typedef enum{
	AV8100_DVI_CTRL_CTL0,
	AV8100_DVI_CTRL_CTL1,
	AV8100_DVI_CTRL_CTL2
} av8100_DVI_format;

typedef enum{
	AV8100_TV_LINES_625 = 0,
	AV8100_TV_LINES_525
} av8100_TV_lines;

typedef enum{
	AV8100_TV_STD_PALBDGHI = 0,
	AV8100_TV_STD_PALN,
	AV8100_TV_STD_NTSCM,
	AV8100_TV_STD_PALM
} av8100_TV_std;

typedef enum{
	AV8100_DENC_OFF = 0,
	AV8100_DENC_ON
} av8100_DENC_State;

typedef enum{
	AV8100_MACROVISION_OFF = 0,
	AV8100_MACROVISION_ON
} av8100_macrovision_state;

typedef enum{
	AV8100_INTERNAL_GENERATOR_OFF = 0,
	AV8100_INTERNAL_GENERATOR_ON
} av8100_internal_generator_state;

typedef enum{
	AV8100_CHROMA_CWS_CAPTURE_OFF = 0,
	AV8100_CHROMA_CWS_CAPTURE_ON
} av8100_chroma_cws_capture_state;

typedef enum{
	AV8100_SYNC_POSITIVE,
	AV8100_SYNC_NEGATIVE
} av8100_video_sync_pol;

typedef enum{

	AV8100_INPUT_PIX_RGB565,
	AV8100_INPUT_PIX_RGB666,
	AV8100_INPUT_PIX_RGB666P,
	AV8100_INPUT_PIX_RGB888,
	AV8100_INPUT_PIX_YCBCR422
} av8100_pixel_format;

typedef enum{
	AV8100_TE_OFF,                     /* NO TE*/
	AV8100_TE_DSI_LANE,                /* TE generated on DSI lane */
	AV8100_TE_IT_LINE,		    /* TE generated on IT line (GPIO) */
	AV8100_TE_DSI_IT                   /* TE generatedon both DSI lane & IT line*/
} av8100_te_config;

typedef enum{

	AV8100_DATA_LANES_USED_0,          /* 0 DSI data lane connected*/
	AV8100_DATA_LANES_USED_1,          /* 1 DSI data lane connected */
	AV8100_DATA_LANES_USED_2,	    /* 2 DSI data lane connected */
	AV8100_DATA_LANES_USED_3,          /* 3 DSI data lane connected */
	AV8100_DATA_LANES_USED_4	    /* 4 DSI data lane connected */
} av8100_dsi_nb_data_lane;

typedef enum{

	AV8100_VIDEO_INTERLACE,
	AV8100_VIDEO_PROGRESSIVE
} av8100_video_mode;

typedef enum{

	AV8100_HDMI_DSI_OFF,
	AV8100_HDMI_DSI_COMMAND_MODE,
	AV8100_HDMI_DSI_VIDEO_MODE
} av8100_dsi_mode;

/* AV8100 video modes */
typedef enum{
	AV8100_CUSTOM,
	AV8100_CEA1_640X480P_59_94HZ,
	AV8100_CEA2_3_720X480P_59_94HZ,   // new
	AV8100_CEA4_1280X720P_60HZ,
	AV8100_CEA5_1920X1080I_60HZ,
	AV8100_CEA6_7_NTSC_60HZ,          //new
	AV8100_CEA14_15_480p_60HZ,        //new
	AV8100_CEA16_1920X1080P_60HZ,     //new
	AV8100_CEA17_18_720X576P_50HZ,    //new
	AV8100_CEA19_1280X720P_50HZ,
	AV8100_CEA20_1920X1080I_50HZ,
	AV8100_CEA21_22_576I_PAL_50HZ,    //new
	AV8100_CEA29_30_576P_50HZ,        //new
	AV8100_CEA31_1920x1080P_50Hz,     //new
	AV8100_CEA32_1920X1080P_24HZ,
	AV8100_CEA33_1920X1080P_25HZ,
	AV8100_CEA34_1920X1080P_30HZ,
	AV8100_CEA60_1280X720P_24HZ,
	AV8100_CEA61_1280X720P_25HZ,
	AV8100_CEA62_1280X720P_30HZ,
	AV8100_VESA9_800X600P_60_32HZ,
	AV8100_VESA14_848X480P_60HZ,
	AV8100_VESA16_1024X768P_60HZ,
	AV8100_VESA22_1280X768P_59_99HZ,
	AV8100_VESA23_1280X768P_59_87HZ,
	AV8100_VESA27_1280X800P_59_91HZ,
	AV8100_VESA28_1280X800P_59_81HZ,
	AV8100_VESA39_1360X768P_60_02HZ,
	AV8100_VESA81_1366X768P_59_79HZ,
	AV8100_VIDEO_OUTPUT_CEA_VESA_MAX
} av8100_output_CEA_VESA;

/** AV8100 internal register access structure*/
struct av8100_register
{
	char  value;
	char offset;
};

/** AV8100 command configuration registers access structure*/
struct av8100_command_register
{
	unsigned char cmd_id;		/* input */
	unsigned char buf_len;		/* input, output */
	unsigned char buf[128];		/* input, output */
	unsigned char return_status;	/* output */
};

/* IOCTL return status */
#define HDMI_COMMAND_RETURN_STATUS_OK			0
#define HDMI_COMMAND_RETURN_STATUS_FAIL			1

#define HDMI_REQUEST_FOR_REVOCATION_LIST_INPUT	2
#define HDMI_CEC_MESSAGE_READBACK_MAXSIZE		16

/** AV8100 status structure*/
struct av8100_status
{
	char av8100_state;
	char av8100_plugin_status;
};

/** Maximum size of the structure need to passed to AV8100 */

#define AV8100_IOC_MAGIC 0xcc

/** IOCTL Operations for accessing information from AV8100 */

#define IOC_AV8100_READ_REGISTER 				_IOWR(AV8100_IOC_MAGIC,1,struct av8100_register)
#define IOC_AV8100_WRITE_REGISTER				_IOWR(AV8100_IOC_MAGIC,2,struct av8100_register)
#define IOC_AV8100_SEND_CONFIGURATION_COMMAND	_IOWR(AV8100_IOC_MAGIC,3,struct av8100_command_register)
//#define IOC_AV8100_READ_CONFIGURATION_COMMAND	_IOWR(AV8100_IOC_MAGIC,4,struct av8100_command_register)
#define IOC_AV8100_GET_STATUS					_IOWR(AV8100_IOC_MAGIC,4,struct av8100_status)
#define IOC_AV8100_ENABLE						_IOWR(AV8100_IOC_MAGIC,5,struct av8100_status)
#define IOC_AV8100_DISABLE						_IOWR(AV8100_IOC_MAGIC,6,struct av8100_status)
#define IOC_AV8100_SET_VIDEO_FORMAT				_IOWR(AV8100_IOC_MAGIC,7,struct av8100_status)
#define IOC_AV8100_HDMI_ON						_IOWR(AV8100_IOC_MAGIC,8,struct av8100_status)
#define IOC_AV8100_HDMI_OFF						_IOWR(AV8100_IOC_MAGIC,9,struct av8100_status)

#define AV8100_IOC_MAXNR (1)

#ifdef _cplusplus
}
#endif /* _cplusplus */

#endif /* !defined(__AV8100_H) */
