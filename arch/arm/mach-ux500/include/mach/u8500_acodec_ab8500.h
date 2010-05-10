/* Header file for u8500 audiocodec specific data structures, enums
 * and private & public functions.
 * Author: Deepak Karda
 * Copyright (C) 2009 ST-Ericsson Pvt. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _U8500_ACODEC_STN4500_H_
#define _U8500_ACODEC_STN4500_H_

#include <mach/ab8500_codec_p.h>
#include <mach/ab8500_codec.h>
#include <mach/ab8500.h>

#define NUMBER_OUTPUT_DEVICE				5
#define NUMBER_INPUT_DEVICE					10
#define NUMBER_LOOPBACK_STATE				2
#define NUMBER_SWITCH_STATE					2
#define NUMBER_POWER_STATE					2
#define NUMBER_TDM_MODE_STATE				2
#define NUMBER_DIRECT_RENDERING_STATE		2
#define NUMBER_PCM_RENDERING_STATE			3


#define CODEC_MUTE      					0x20
#define DEFAULT_VOLUME      				0x64
#define DEFAULT_GAIN        				0x32
#define VOL_MAX                 			0x64
#define VOL_MIN                 			0x00
#define DEFAULT_OUTPUT_DEVICE   			AB8500_CODEC_DEST_HEADSET
#define DEFAULT_INPUT_DEVICE    			AB8500_CODEC_SRC_D_MICROPHONE_3
#define DEFAULT_LOOPBACK_STATE				DISABLE
#define DEFAULT_SWITCH_STATE				DISABLE
#define DEFAULT_TDM8_CH_MODE_STATE			DISABLE
#define DEFAULT_DIRECT_RENDERING_STATE		DISABLE
#define DEFAULT_BURST_FIFO_STATE			RENDERING_DISABLE
#define DEFAULT_FM_PLAYBACK_STATE			RENDERING_DISABLE
#define DEFAULT_FM_TX_STATE					RENDERING_DISABLE



#define MIN_RATE_PLAYBACK   48000
#define MAX_RATE_PLAYBACK   48000
#define MIN_RATE_CAPTURE    48000
#define MAX_RATE_CAPTURE    48000
#define MAX_NO_OF_RATES		1

#define ALSA_MSP_BT_NUM 		0
#define ALSA_MSP_PCM_NUM 		1
#define ALSA_MSP_HDMI_NUM 		2

typedef enum
{
	DISABLE,
	ENABLE
}t_u8500_bool_state;

typedef enum
{
	RENDERING_DISABLE,
	RENDERING_ENABLE,
	RENDERING_PENDING
}t_u8500_pmc_rendering_state;

typedef enum
{
	ACODEC_CONFIG_REQUIRED,
	ACODEC_CONFIG_NOT_REQUIRED
}t_u8500_acodec_config_need;

typedef enum
{
	CLASSICAL_MODE,
	TDM_8_CH_MODE
}t_u8500_mode;


typedef struct {
    unsigned int left_volume;
	unsigned int right_volume;
    unsigned int mute_state;
    t_u8500_bool_state power_state;
}u8500_io_dev_config_t;

typedef enum {
	NO_USER = 0,
	USER_ALSA = 2, /*To make it equivalent to user id for MSP*/
	USER_SAA,
}t_acodec_user;

typedef struct {
	u8500_io_dev_config_t output_config[NUMBER_OUTPUT_DEVICE];
	u8500_io_dev_config_t input_config[NUMBER_INPUT_DEVICE];
	//t_acodec_user  user;
	t_acodec_user   cur_user;
}t_u8500_codec_system_context;

typedef enum {
        T_CODEC_SAMPLING_FREQ_48KHZ = 48,
} acodec_sample_frequency;


struct acodec_configuration
{
	t_ab8500_codec_direction direction;
	acodec_sample_frequency input_frequency;
	acodec_sample_frequency output_frequency;
	codec_msp_srg_clock_sel_type mspClockSel;
	codec_msp_in_clock_freq_type mspInClockFreq;
	u32 channels;
	t_acodec_user user;
	t_u8500_acodec_config_need acodec_config_need;
	t_u8500_bool_state direct_rendering_mode;
	irqreturn_t (*handler)(void *data, int irq);
	void *tx_callback_data;
	void *rx_callback_data;
};

typedef enum
{
        ACODEC_DISABLE_ALL,
        ACODEC_DISABLE_TRANSMIT,
        ACODEC_DISABLE_RECEIVE,
}t_acodec_disable;

/*extern t_ab8500_codec_error u8500_acodec_set_volume(int input_vol_left,
					int input_vol_right,
					int output_vol_left,
					int output_vol_right, t_acodec_user user);*/

extern t_ab8500_codec_error u8500_acodec_open(int stream_id);
//extern t_ab8500_codec_error u8500_acodec_pause_transfer(void);
//extern t_ab8500_codec_error u8500_acodec_unpause_transfer(void);
extern t_ab8500_codec_error u8500_acodec_send_data(void *data, size_t bytes, int dma_flag);
extern t_ab8500_codec_error u8500_acodec_receive_data(void *data, size_t bytes, int dma_flag);
extern t_ab8500_codec_error u8500_acodec_close(t_acodec_disable flag);

extern t_ab8500_codec_error u8500_acodec_set_output_volume(t_ab8500_codec_dest dest_device,
					int left_volume,
					int right_volume,
					t_acodec_user user);

extern t_ab8500_codec_error u8500_acodec_get_output_volume(t_ab8500_codec_dest dest_device,
					int *p_left_volume,
					int *p_right_volume,
					t_acodec_user user);

extern t_ab8500_codec_error u8500_acodec_set_input_volume(t_ab8500_codec_src src_device,
					int left_volume,
					int right_volume,
					t_acodec_user user);

extern t_ab8500_codec_error u8500_acodec_get_input_volume(t_ab8500_codec_src src_device,
					int *p_left_volume,
					int *p_right_volume,
					t_acodec_user user);

extern t_ab8500_codec_error u8500_acodec_toggle_analog_lpbk(t_u8500_bool_state lpbk_state,
					t_acodec_user user);

extern t_ab8500_codec_error u8500_acodec_toggle_digital_lpbk(t_u8500_bool_state lpbk_state,
					t_acodec_user user);

extern t_ab8500_codec_error u8500_acodec_toggle_playback_mute_control(t_ab8500_codec_dest dest_device,
					t_u8500_bool_state mute_state,
					t_acodec_user user);
extern t_ab8500_codec_error u8500_acodec_toggle_capture_mute_control(t_ab8500_codec_src src_device,
					t_u8500_bool_state mute_state,
					t_acodec_user user);

extern t_ab8500_codec_error u8500_acodec_enable_audio_mode(struct acodec_configuration *acodec_config);
/*extern t_ab8500_codec_error u8500_acodec_enable_voice_mode(struct acodec_configuration *acodec_config);*/


extern t_ab8500_codec_error u8500_acodec_select_input(t_ab8500_codec_src input_device, t_acodec_user user);
extern t_ab8500_codec_error u8500_acodec_select_output(t_ab8500_codec_dest output_device, t_acodec_user user);

extern t_ab8500_codec_error u8500_acodec_allocate_ad_slot(t_ab8500_codec_src input_device,t_u8500_mode mode);
extern t_ab8500_codec_error u8500_acodec_unallocate_ad_slot(t_ab8500_codec_src input_device,t_u8500_mode mode);
extern t_ab8500_codec_error u8500_acodec_allocate_da_slot(t_ab8500_codec_dest output_device,t_u8500_mode mode);
extern t_ab8500_codec_error u8500_acodec_unallocate_da_slot(t_ab8500_codec_dest output_device,t_u8500_mode mode);

extern t_ab8500_codec_error u8500_acodec_set_src_power_cntrl(t_ab8500_codec_src input_device,t_u8500_bool_state pwr_state);
extern t_ab8500_codec_error u8500_acodec_set_dest_power_cntrl(t_ab8500_codec_dest output_device,t_u8500_bool_state pwr_state);

extern t_u8500_bool_state u8500_acodec_get_src_power_state(t_ab8500_codec_src input_device);
extern t_u8500_bool_state u8500_acodec_get_dest_power_state(t_ab8500_codec_dest output_device);
extern t_ab8500_codec_error u8500_acodec_set_burst_mode_fifo(t_u8500_pmc_rendering_state fifo_state);

extern t_ab8500_codec_error u8500_acodec_unsetuser(t_acodec_user user);
extern t_ab8500_codec_error u8500_acodec_setuser(t_acodec_user user);

extern void codec_power_init(void);
extern void u8500_acodec_powerdown(void);

//t_ab8500_codec_error acodec_msp_enable(t_touareg_codec_sample_frequency freq,int channels, t_acodec_user user);



#define TRG_CODEC_ADDRESS_ON_SPI_BUS 	(0x0D)

extern int ab8500_write(u8 block, u32 adr, u8 data);
extern int ab8500_read(u8 block, u32 adr);

#if 0
	#define FUNC_ENTER() printk("\n -Enter : %s",__FUNCTION__)
	#define FUNC_EXIT()  printk("\n -Exit  : %s",__FUNCTION__)
#else
	#define FUNC_ENTER()
	#define FUNC_EXIT()
#endif
#endif

