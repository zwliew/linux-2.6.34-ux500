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
#ifndef __AV8100__H__
#define __AV8100__H__

/* Temp: TODO: remove (or move to menuconfig) */
/*#define CONFIG_AV8100_SDTV*/

struct av8100_platform_data {
    unsigned	gpio_base;
    int irq;
};

enum av8100_error {
	AV8100_OK = 0x0,
	AV8100_INVALID_COMMAND = 0x1,
	AV8100_INVALID_INTERFACE = 0x2,
	AV8100_INVALID_IOCTL = 0x3,
	AV8100_COMMAND_FAIL = 0x4,
	AV8100_FWDOWNLOAD_FAIL = 0x5,
	AV8100_FAIL = 0xFF,
};

enum av8100_command_type {
	AV8100_COMMAND_VIDEO_INPUT_FORMAT  = 0x1,
	AV8100_COMMAND_AUDIO_INPUT_FORMAT,
	AV8100_COMMAND_VIDEO_OUTPUT_FORMAT,
	AV8100_COMMAND_VIDEO_SCALING_FORMAT,
	AV8100_COMMAND_COLORSPACECONVERSION,
	AV8100_COMMAND_CEC_MESSAGE_WRITE,
	AV8100_COMMAND_CEC_MESSAGE_READ_BACK,
	AV8100_COMMAND_DENC,
	AV8100_COMMAND_HDMI,
	AV8100_COMMAND_HDCP_SENDKEY,
	AV8100_COMMAND_HDCP_MANAGEMENT,
	AV8100_COMMAND_INFOFRAMES,
	AV8100_COMMAND_EDID_SECTION_READBACK,
	AV8100_COMMAND_PATTERNGENERATOR,
	AV8100_COMMAND_FUSE_AES_KEY,
};

enum interface_type {
	I2C_INTERFACE = 0x0,
	DSI_INTERFACE = 0x1,
};

enum av8100_dsi_mode {

	AV8100_HDMI_DSI_OFF,
	AV8100_HDMI_DSI_COMMAND_MODE,
	AV8100_HDMI_DSI_VIDEO_MODE
};

enum av8100_pixel_format {

	AV8100_INPUT_PIX_RGB565,
	AV8100_INPUT_PIX_RGB666,
	AV8100_INPUT_PIX_RGB666P,
	AV8100_INPUT_PIX_RGB888,
	AV8100_INPUT_PIX_YCBCR422
};

enum av8100_video_mode {
	AV8100_VIDEO_INTERLACE,
	AV8100_VIDEO_PROGRESSIVE
};

enum av8100_dsi_nb_data_lane {
	AV8100_DATA_LANES_USED_0,
	AV8100_DATA_LANES_USED_1,
	AV8100_DATA_LANES_USED_2,
	AV8100_DATA_LANES_USED_3,
	AV8100_DATA_LANES_USED_4
};

enum av8100_te_config {
	AV8100_TE_OFF,		/* NO TE*/
	AV8100_TE_DSI_LANE,	/* TE generated on DSI lane */
	AV8100_TE_IT_LINE,	/* TE generated on IT line (GPIO) */
	AV8100_TE_DSI_IT	/* TE generatedon both DSI lane & IT line*/
};

enum av8100_audio_if_format {
	AV8100_AUDIO_I2S_MODE,
	AV8100_AUDIO_I2SDELAYED_MODE, /* I2S Mode by default*/
	AV8100_AUDIO_TDM_MODE         /* 8 Channels by default*/
};

enum av8100_sample_freq {
	AV8100_AUDIO_FREQ_32KHZ,
	AV8100_AUDIO_FREQ_44_1KHZ,
	AV8100_AUDIO_FREQ_48KHZ,
	AV8100_AUDIO_FREQ_64KHZ,
	AV8100_AUDIO_FREQ_88_2KHZ,
	AV8100_AUDIO_FREQ_96KHZ,
	AV8100_AUDIO_FREQ_128KHZ,
	AV8100_AUDIO_FREQ_176_1KHZ,
	AV8100_AUDIO_FREQ_192KHZ
};

enum av8100_audio_word_length {
	AV8100_AUDIO_16BITS,
	AV8100_AUDIO_20BITS,
	AV8100_AUDIO_24BITS
};

enum av8100_audio_format {
	AV8100_AUDIO_LPCM_MODE,
	AV8100_AUDIO_COMPRESS_MODE
};

enum av8100_audio_if_mode {
	AV8100_AUDIO_SLAVE,
	AV8100_AUDIO_MASTER
};

enum av8100_audio_mute {
	AV8100_AUDIO_MUTE_DISABLE,
	AV8100_AUDIO_MUTE_ENABLE
};

enum av8100_output_CEA_VESA {
	AV8100_CUSTOM,
	AV8100_CEA1_640X480P_59_94HZ,
	AV8100_CEA2_3_720X480P_59_94HZ,
	AV8100_CEA4_1280X720P_60HZ,
	AV8100_CEA5_1920X1080I_60HZ,
	AV8100_CEA6_7_NTSC_60HZ,
	AV8100_CEA14_15_480p_60HZ,
	AV8100_CEA16_1920X1080P_60HZ,
	AV8100_CEA17_18_720X576P_50HZ,
	AV8100_CEA19_1280X720P_50HZ,
	AV8100_CEA20_1920X1080I_50HZ,
	AV8100_CEA21_22_576I_PAL_50HZ,
	AV8100_CEA29_30_576P_50HZ,
	AV8100_CEA31_1920x1080P_50Hz,
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
};

enum av8100_video_sync_pol {
	AV8100_SYNC_POSITIVE,
	AV8100_SYNC_NEGATIVE
};

enum av8100_hdmi_mode {
	AV8100_HDMI_OFF,
	AV8100_HDMI_ON,
	AV8100_HDMI_AVMUTE
};

enum av8100_hdmi_format {
	AV8100_HDMI,
	AV8100_DVI
};

enum av8100_DVI_format {
	AV8100_DVI_CTRL_CTL0,
	AV8100_DVI_CTRL_CTL1,
	AV8100_DVI_CTRL_CTL2
};

enum av8100_pattern_type {
	AV8100_PATTERN_OFF,
	AV8100_PATTERN_GENERATOR,
	AV8100_PRODUCTION_TESTING
};

enum av8100_pattern_format {
	AV8100_NO_PATTERN,
	AV8100_PATTERN_VGA,
	AV8100_PATTERN_720P,
	AV8100_PATTERN_1080P
};

enum av8100_pattern_audio {
	AV8100_PATTERN_AUDIO_OFF,
	AV8100_PATTERN_AUDIO_ON,
	AV8100_PATTERN_AUDIO_I2S_MEM
};

struct av8100_video_input_format_cmd {
	enum av8100_dsi_mode		dsi_input_mode;
	enum av8100_pixel_format	input_pixel_format;
	unsigned short			total_horizontal_pixel;
	unsigned short			total_horizontal_active_pixel;
	unsigned short			total_vertical_lines;
	unsigned short			total_vertical_active_lines;
	enum av8100_video_mode		video_mode;
	enum av8100_dsi_nb_data_lane	nb_data_lane;
	unsigned char			nb_virtual_ch_command_mode;
	unsigned char			nb_virtual_ch_video_mode;
	unsigned short			TE_line_nb;
	enum av8100_te_config		TE_config;
	unsigned long			master_clock_freq;
	unsigned char			ui_x4;
};

struct av8100_audio_input_format_cmd {
	enum av8100_audio_if_format	audio_input_if_format;
	unsigned char			i2s_input_nb;
	enum av8100_sample_freq		sample_audio_freq;
	enum av8100_audio_word_length	audio_word_lg;
	enum av8100_audio_format	audio_format;
	enum av8100_audio_if_mode	audio_if_mode;
	enum av8100_audio_mute		audio_mute;
};

struct av8100_video_output_format_cmd {
	enum av8100_output_CEA_VESA	video_output_cea_vesa;
	enum av8100_video_sync_pol	vsync_polarity;
	enum av8100_video_sync_pol	hsync_polarity;
	unsigned short		total_horizontal_pixel;
	unsigned short		total_horizontal_active_pixel;
	unsigned short		total_vertical_in_half_lines;
	unsigned short		total_vertical_active_in_half_lines;
	unsigned short		hsync_start_in_pixel;
	unsigned short		hsync_length_in_pixel;
	unsigned short		vsync_start_in_half_line;
	unsigned short		vsync_length_in_half_line;
	unsigned short		hor_video_start_pixel;
	unsigned short		vert_video_start_pixel;
	enum av8100_video_mode	video_type;
	unsigned short		pixel_repeat;
	unsigned long		pixel_clock_freq_Hz;
};

struct av8100_video_scaling_format_cmd {
	unsigned short	h_start_in_pixel;
	unsigned short	h_stop_in_pixel;
	unsigned short	v_start_in_line;
	unsigned short	v_stop_in_line;
	unsigned short	h_start_out_pixel;
	unsigned short	h_stop_out_pixel;
	unsigned short	v_start_out_line;
	unsigned short	v_stop_out_line;
};

struct av8100_color_space_conversion_format_cmd {
	unsigned short	c0;
	unsigned short	c1;
	unsigned short	c2;
	unsigned short	c3;
	unsigned short	c4;
	unsigned short	c5;
	unsigned short	c6;
	unsigned short	c7;
	unsigned short	c8;
	unsigned short	aoffset;
	unsigned short	boffset;
	unsigned short	coffset;
	unsigned char	lmax;
	unsigned char	lmin;
	unsigned char	cmax;
	unsigned char	cmin;
};

struct av8100_cec_message_write_format_cmd {
	unsigned char buffer_length;
	unsigned char buffer[16];
};

struct av8100_cec_message_read_back_format_cmd {
};

enum av8100_cvbs_video_format {
	AV8100_CVBS_625,
	AV8100_CVBS_525,
};

enum av8100_standard_selection {
	AV8100_PAL_BDGHI,
	AV8100_PAL_N,
	AV8100_NTSC_M,
	AV8100_PAL_M
};

struct av8100_denc_format_cmd {
	enum av8100_cvbs_video_format cvbs_video_format;
	enum av8100_standard_selection standard_selection;
	unsigned char on_off;
	unsigned char macrovision_on_off;
	unsigned char internal_generator;
};

struct av8100_hdmi_cmd {
	enum av8100_hdmi_mode	hdmi_mode;
	enum av8100_hdmi_format	hdmi_format;
	enum av8100_DVI_format	dvi_format; /* used only if HDMI_format = DVI*/
};

struct av8100_hdcp_send_key_format_cmd {
	unsigned char key_number;
	unsigned char key[7];
};

struct av8100_hdcp_management_format_cmd {
	unsigned char request_hdcp_revocation_list;
	unsigned char request_encrypted_transmission;
	unsigned char oess_eess_encryption_use;
};

struct av8100_infoframes_format_cmd {
	unsigned char type;
	unsigned char version;
	unsigned char length;
	unsigned char crc;
	unsigned char data[28];
};

struct av8100_edid_section_readback_format_cmd {
	unsigned char address;
	unsigned char block_number;
};

struct av8100_pattern_generator_format_cmd {
	enum av8100_pattern_type	pattern_type;
	enum av8100_pattern_format	pattern_video_format;
	enum av8100_pattern_audio	pattern_audio_mode;
};

struct av8100_fuse_aes_key_format_cmd {
	unsigned char fuse_operation;
	unsigned char key[16];
};

union av8100_configuration {
	struct av8100_video_input_format_cmd	video_input_format;
	struct av8100_audio_input_format_cmd	audio_input_format;
	struct av8100_video_output_format_cmd	video_output_format;
	struct av8100_video_scaling_format_cmd	video_scaling_format;
	struct av8100_color_space_conversion_format_cmd
		color_space_conversion_format;
	struct av8100_cec_message_write_format_cmd
		cec_message_write_format;
	struct av8100_cec_message_read_back_format_cmd
		cec_message_read_back_format;
	struct av8100_denc_format_cmd		denc_format;
	struct av8100_hdmi_cmd			hdmi_format;
	struct av8100_hdcp_send_key_format_cmd	hdcp_send_key_format;
	struct av8100_hdcp_management_format_cmd hdcp_management_format;
	struct av8100_infoframes_format_cmd	infoframes_format;
	struct av8100_edid_section_readback_format_cmd
		edid_section_readback_format;
	struct av8100_pattern_generator_format_cmd pattern_generator_format;
	struct av8100_fuse_aes_key_format_cmd	fuse_aes_key_format;
};

enum av8100_operating_mode {
	AV8100_OPMODE_UNDEFINED = 0,
	AV8100_OPMODE_SHUTDOWN,
	AV8100_OPMODE_STANDBY,
	AV8100_OPMODE_SCAN,
	AV8100_OPMODE_INIT,
	AV8100_OPMODE_IDLE,
	AV8100_OPMODE_VIDEO
};

enum av8100_plugin_status {
	AV8100_PLUGIN_NONE = 0x0,
	AV8100_HDMI_PLUGIN = 0x1,
	AV8100_CVBS_PLUGIN = 0x2
};

struct av8100_status {
	enum av8100_operating_mode	av8100_state;
	enum av8100_plugin_status	av8100_plugin_status;
	int			hdmi_on;
};


int av8100_init(void);
void av8100_exit(void);
int av8100_powerup(void);
int av8100_powerdown(void);
int av8100_disable_interrupt(void);
int av8100_enable_interrupt(void);
int av8100_download_firmware(char *fw_buff, int numOfBytes,
	enum interface_type if_type);
int av8100_register_standby_write(
		unsigned char cpd,
		unsigned char stby,
		unsigned char mclkrng);
int av8100_register_hdmi_5_volt_time_write(
		unsigned char off_time,
		unsigned char on_time);
int av8100_register_standby_interrupt_mask_write(
		unsigned char hpdm,
		unsigned char cpdm,
		unsigned char stbygpiocfg,
		unsigned char ipol);
int av8100_register_standby_pending_interrupt_write(
		unsigned char hpdi,
		unsigned char cpdi,
		unsigned char oni);
int av8100_register_general_interrupt_mask_write(
		unsigned char eocm,
		unsigned char vsim,
		unsigned char vsom,
		unsigned char cecm,
		unsigned char hdcpm,
		unsigned char uovbm,
		unsigned char tem);
int av8100_register_general_interrupt_write(
		unsigned char eoci,
		unsigned char vsii,
		unsigned char vsoi,
		unsigned char ceci,
		unsigned char hdcpi,
		unsigned char uovbi);
int av8100_register_gpio_configuration_write(
		unsigned char dat3dir,
		unsigned char dat3val,
		unsigned char dat2dir,
		unsigned char dat2val,
		unsigned char dat1dir,
		unsigned char dat1val,
		unsigned char ucdbg);
int av8100_register_general_control_write(
		unsigned char fdl,
		unsigned char hld,
		unsigned char wa,
		unsigned char ra);
int av8100_register_firmware_download_entry_write(
	unsigned char mbyte_code_entry);
int av8100_register_write(
		unsigned char offset,
		unsigned char value);
int av8100_register_standby_read(
		unsigned char *cpd,
		unsigned char *stby,
		unsigned char *hpds,
		unsigned char *cpds,
		unsigned char *mclkrng);
int av8100_register_hdmi_5_volt_time_read(
		unsigned char *off_time,
		unsigned char *on_time);
int av8100_register_standby_interrupt_mask_read(
		unsigned char *hpdm,
		unsigned char *cpdm,
		unsigned char *stbygpiocfg,
		unsigned char *ipol);
int av8100_register_standby_pending_interrupt_read(
		unsigned char *hpdi,
		unsigned char *cpdi,
		unsigned char *oni,
		unsigned char *sid);
int av8100_register_general_interrupt_mask_read(
		unsigned char *eocm,
		unsigned char *vsim,
		unsigned char *vsom,
		unsigned char *cecm,
		unsigned char *hdcpm,
		unsigned char *uovbm,
		unsigned char *tem);
int av8100_register_general_interrupt_read(
		unsigned char *eoci,
		unsigned char *vsii,
		unsigned char *vsoi,
		unsigned char *ceci,
		unsigned char *hdcpi,
		unsigned char *uovbi,
		unsigned char *tei);
int av8100_register_general_status_read(
		unsigned char *cecrec,
		unsigned char *cectrx,
		unsigned char *uc,
		unsigned char *onuvb,
		unsigned char *hdcps);
int av8100_register_gpio_configuration_read(
		unsigned char *dat3dir,
		unsigned char *dat3val,
		unsigned char *dat2dir,
		unsigned char *dat2val,
		unsigned char *dat1dir,
		unsigned char *dat1val,
		unsigned char *ucdbg);
int av8100_register_general_control_read(
		unsigned char *fdl,
		unsigned char *hld,
		unsigned char *wa,
		unsigned char *ra);
int av8100_register_firmware_download_entry_read(
	unsigned char *mbyte_code_entry);
int av8100_register_read(
		unsigned char offset,
		unsigned char *value);
int av8100_configuration_get(enum av8100_command_type command_type,
	union av8100_configuration *config);
int av8100_configuration_prepare(enum av8100_command_type command_type,
	union av8100_configuration *config);
int av8100_configuration_write(enum av8100_command_type command_type,
		unsigned char *return_buffer_length,
		unsigned char *return_buffer, enum interface_type if_type);
int av8100_configuration_write_raw(enum av8100_command_type command_type,
	unsigned char buffer_length,
	unsigned char *buffer,
	unsigned char *return_buffer_length,
	unsigned char *return_buffer);
struct av8100_status av8100_status_get(void);
enum av8100_output_CEA_VESA av8100_video_output_format_get(int xres,
	int yres,
	int htot,
	int vtot,
	bool interlaced);

#endif /* __AV8100__H__ */
