/*---------------------------------------------------------------------------*/
/* © copyright STEricsson,2009. All rights reserved. For                     */
/* information, STEricsson reserves the right to license                     */
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

#include <mach/av8100.h>

/* defines for av8100_ */
#define av8100_command_offset 0x10
#define AV8100_COMMAND_MAX_LENGTH 0x81
#define GPIO_AV8100_RSTN 196
#define GPIO_AV8100_INT 192
#define AV8100_DRIVER_MINOR_NUMBER 240


#define HDMI_HOTPLUG_INTERRUPT 0x1
#define HDMI_HOTPLUG_INTERRUPT_MASK 0xFE
#define CVBS_PLUG_INTERRUPT 0x2
#define CVBS_PLUG_INTERRUPT_MASK 0xFD
#define TE_INTERRUPT_MASK 0x40
#define UNDER_OVER_FLOW_INTERRUPT_MASK 0x20

#define REG_16_8_LSB(p)  (unsigned char)(p && 0xFF)
#define REG_16_8_MSB(p)  (unsigned char)((p && 0xFF00)>>8)
#define REG_32_8_MSB(p)  (unsigned char)((p && 0xFF000000)>>24)
#define REG_32_8_MMSB(p) (unsigned char)((p && 0x00FF0000)>>16)
#define REG_32_8_MLSB(p) (unsigned char)((p && 0x0000FF00)>>8)
#define REG_32_8_LSB(p)   (unsigned char)(p && 0x000000FF)

/* Global data */
static struct i2c_driver av8100_driver;
static char buffer[AV8100_COMMAND_MAX_LENGTH];
/**
 * struct av8100_cea - CEA(consumer electronic access) standard structure
 * @cea_id:
 * @cea_nb:
 * @vtotale:
 **/
typedef struct {
    char cea_id[40] ;
    int cea_nb ;
    //float vtotale;
    int vtotale;
    int vactive;
    int vstovid ;
    int vslen ;
    int hvoffset;
    char vpol[5];
    int htotale;
    int hactive;
    int vidtohs ;
    int hslen ;
    int frequence;
    char hpol[5];
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
/* STWav8100 Private functions */

static int av8100_open(struct inode *inode, struct file *filp);
static int av8100_release(struct inode *inode, struct file *filp);
static int av8100_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static irqreturn_t av8100_intr_handler(int irq,struct av8100_data *av8100_Data);
static int av8100_write_multi_byte(struct i2c_client *client, unsigned char regOffset,unsigned char *buf,unsigned char nbytes);
static int av8100_write_single_byte(struct i2c_client *client, unsigned char reg,unsigned char data);
static int av8100_read_multi_byte(struct i2c_client *client,unsigned char reg,unsigned char *buf, unsigned char nbytes);
static int av8100_read_single_byte(struct i2c_client *client,unsigned char reg, unsigned char* val);
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
static int read_edid_info(struct i2c_client *i2c, char* buff);
static int av8100_send_command (struct i2c_client *i2cClient, int command_type, enum interface if_type);
static int av8100_powerup(struct i2c_client *i2c, const struct i2c_device_id *id);
static int av8100_probe(struct i2c_client *i2cClient,const struct i2c_device_id *id);
static int __exit av8100_remove(struct i2c_client *i2cClient);
static int av8100_downlaod_firmware(struct i2c_client *i2c, char regoffset, char* fw_buff, int numOfBytes, enum interface if_type);

void av8100_update_config_params();
