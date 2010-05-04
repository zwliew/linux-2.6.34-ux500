/*
* Overview:
*   Header File  for audio hardware access
*
* Copyright (C) 2009 ST Ericsson
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/



#ifndef _AUDIOIO_CORE_H_
#define _AUDIOIO_CORE_H_

#include "ste_audio_io_ioctl.h"
#include "ste_audio_io_func.h"
#include "ste_audio_io_hwctrl_common.h"

#define MAX_NO_CHANNELS			4


typedef struct __transducer_context {
	/* public variables*/
	int gain[MAX_NO_CHANNELS];
	int is_muted[MAX_NO_CHANNELS];
	int is_power_up[MAX_NO_CHANNELS];

	/* public funcs*/
	int (*pwr_up_func)(AUDIOIO_CH_INDEX );
	int (*pwr_down_func)(AUDIOIO_CH_INDEX );
	int (*pwr_state_func)(void);
	int (*set_gain_func)(AUDIOIO_CH_INDEX,uint16,int32,uint32);
	int (*get_gain_func)(int *,int *,uint16);
	int (*mute_func)(AUDIOIO_CH_INDEX );
	int (*unmute_func)(AUDIOIO_CH_INDEX, int *);
	int (*mute_state_func)(void);
	int (*enable_fade_func)(void);
	int (*disable_fade_func)(void);
	int (*switch_to_burst_func)(int32,int32,int32,int32);
	int (*switch_to_normal_func)(void);
}transducer_context_t;

typedef struct __audiocodec_context {
	int	audio_codec_powerup;
	struct semaphore audio_io_sem;
	transducer_context_t transducer[MAX_NO_TRANSDUCERS];
}audiocodec_context_t;


int ste_audioio_core_api_access_read(audioio_data_t *dev_data);

int ste_audioio_core_api_access_write(audioio_data_t *dev_data);

int ste_audioio_core_api_power_control_transducer(audioio_pwr_ctrl_t *pwr_ctrl);

int ste_audioio_core_api_power_status_transducer(audioio_pwr_ctrl_t *pwr_ctrl);

int ste_audioio_core_api_loop_control(audioio_loop_ctrl_t *loop_ctrl);

int ste_audioio_core_api_loop_status(audioio_loop_ctrl_t *loop_ctrl);

int ste_audioio_core_api_get_transducer_gain_capability(
    audioio_get_gain_t *get_gain);

int ste_audioio_core_api_gain_capabilities_loop(audioio_gain_loop_t *gain_loop);

int ste_audioio_core_api_supported_loops(audioio_support_loop_t *support_loop);

int ste_audioio_core_api_gain_descriptor_transducer(
    audioio_gain_desc_trnsdr_t *gdesc_trnsdr);

int ste_audioio_core_api_gain_control_transducer(
    audioio_gain_ctrl_trnsdr_t *gctrl_trnsdr);

int ste_audioio_core_api_gain_query_transducer(
    audioio_gain_ctrl_trnsdr_t *gctrl_trnsdr);

int ste_audioio_core_api_mute_control_transducer(
    audioio_mute_trnsdr_t *mute_trnsdr);

int ste_audioio_core_api_mute_status_transducer(
    audioio_mute_trnsdr_t *mute_trnsdr);

int ste_audioio_core_api_fading_control(audioio_fade_ctrl_t *fade_ctrl);

int ste_audioio_core_api_burstmode_control(audioio_burst_ctrl_t *burst_ctrl);

int ste_audioio_core_api_powerup_audiocodec(void);

int ste_audioio_core_api_powerdown_audiocodec(void);

void ste_audioio_core_api_init_data(void);

void ste_audioio_core_api_free_data(void);

void ste_audioio_init_transducer_cnxt(void);

void ste_audioio_core_api_store_data(AUDIOIO_CH_INDEX channel_index,int *ptr,
								int value);

AUDIOIO_COMMON_SWITCH ste_audioio_core_api_get_status(
				AUDIOIO_CH_INDEX channel_index,int *ptr);

int return_device_id(void);

#endif /* _AUDIOIO_CORE_H_ */

