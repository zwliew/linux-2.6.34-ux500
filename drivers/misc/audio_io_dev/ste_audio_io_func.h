/*
* Overview:
*   Header File for ste_audio_io_func.c
*
* Copyright (C) 2009 ST Ericsson
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/


#ifndef _AUDIOIO_FUNC_H_
#define _AUDIOIO_FUNC_H_

#include "ste_audio_io_ioctl.h"
#include <mach/ab8500.h>

/*#define DEBUG_AUDIOIO*/
int dump_acodec_registers(const char *);
void debug_print(char *);
#define MSG0	debug_print


#define AB8500_BLOCK_ADDR(address) ((address >> 8) & 0xFF)
#define AB8500_OFFSET_ADDR(address) (address & 0xFF)

#define HW_REG_READ(reg)  		ab8500_read(AB8500_BLOCK_ADDR(reg),\
  AB8500_OFFSET_ADDR(reg))

#define HW_REG_WRITE(reg,data)  	ab8500_write(AB8500_BLOCK_ADDR(reg),\
  AB8500_OFFSET_ADDR(reg),data)

unsigned int ab8500_acodec_modify_write(unsigned int reg,u8 mask_set,
								u8 mask_clear);

#define HW_ACODEC_MODIFY_WRITE(reg,mask_set,mask_clear)\
  ab8500_acodec_modify_write(reg,mask_set,mask_clear)


unsigned int ab8500_modify_write(unsigned int reg,u8 mask_set,u8 mask_clear);

int ste_audio_io_power_up_headset(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_down_headset(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_state_headset_query(void);
int ste_audio_io_set_headset_gain(AUDIOIO_CH_INDEX chnl_index,uint16 gain_index,
					int32 gain_value, uint32 linear);
int ste_audio_io_get_headset_gain(int *,int *,uint16);
int ste_audio_io_mute_headset(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_unmute_headset(AUDIOIO_CH_INDEX chnl_index, int *gain);
int ste_audio_io_mute_headset_state(void);
int ste_audio_io_enable_fade_headset(void);
int ste_audio_io_disable_fade_headset(void);
int ste_audio_io_switch_to_burst_mode_headset(int32 burst_fifo_interrupt_sample_count,
int32 burst_fifo_length,int32 burst_fifo_switch_frame,int32 burst_fifo_sample_number);
int ste_audio_io_switch_to_normal_mode_headset(void);

int ste_audio_io_power_up_earpiece(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_down_earpiece(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_state_earpiece_query(void);
int ste_audio_io_set_earpiece_gain(AUDIOIO_CH_INDEX chnl_index,
			uint16 gain_index,int32 gain_value, uint32 linear);
int ste_audio_io_get_earpiece_gain(int*,int*,uint16);
int ste_audio_io_mute_earpiece(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_unmute_earpiece(AUDIOIO_CH_INDEX chnl_index, int *gain);
int ste_audio_io_mute_earpiece_state(void);
int ste_audio_io_enable_fade_earpiece(void);
int ste_audio_io_disable_fade_earpiece(void);

int ste_audio_io_power_up_ihf(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_down_ihf(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_state_ihf_query(void);
int ste_audio_io_set_ihf_gain(AUDIOIO_CH_INDEX chnl_index,uint16 gain_index,
					int32 gain_value, uint32 linear);
int ste_audio_io_get_ihf_gain(int*,int*,uint16);
int ste_audio_io_mute_ihf(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_unmute_ihf(AUDIOIO_CH_INDEX chnl_index, int *gain);
int ste_audio_io_mute_ihf_state(void);
int ste_audio_io_enable_fade_ihf(void);
int ste_audio_io_disable_fade_ihf(void);

int ste_audio_io_power_up_vibl(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_down_vibl(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_state_vibl_query(void);
int ste_audio_io_set_vibl_gain(AUDIOIO_CH_INDEX chnl_index,uint16 gain_index,
					int32 gain_value, uint32 linear);
int ste_audio_io_get_vibl_gain(int*,int*,uint16);
int ste_audio_io_mute_vibl(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_unmute_vibl(AUDIOIO_CH_INDEX chnl_index, int *gain);
int ste_audio_io_mute_vibl_state(void);
int ste_audio_io_enable_fade_vibl(void);
int ste_audio_io_disable_fade_vibl(void);

int ste_audio_io_power_up_vibr(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_down_vibr(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_state_vibr_query(void);
int ste_audio_io_set_vibr_gain(AUDIOIO_CH_INDEX chnl_index,uint16 gain_index,
					int32 gain_value, uint32 linear);
int ste_audio_io_get_vibr_gain(int*,int*,uint16);
int ste_audio_io_mute_vibr(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_unmute_vibr(AUDIOIO_CH_INDEX chnl_index, int *gain);
int ste_audio_io_mute_vibr_state(void);
int ste_audio_io_enable_fade_vibr(void);
int ste_audio_io_disable_fade_vibr(void);

int ste_audio_io_power_up_mic1a(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_down_mic1a(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_state_mic1a_query(void);
int ste_audio_io_set_mic1a_gain(AUDIOIO_CH_INDEX chnl_index,uint16 gain_index,
					int32 gain_value, uint32 linear);
int ste_audio_io_get_mic1a_gain(int*,int*,uint16);
int ste_audio_io_mute_mic1a(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_unmute_mic1a(AUDIOIO_CH_INDEX chnl_index, int *gain);
int ste_audio_io_mute_mic1a_state(void);
int ste_audio_io_enable_fade_mic1a(void);
int ste_audio_io_disable_fade_mic1a(void);


int ste_audio_io_power_up_mic1b(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_down_mic1b(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_state_mic1b_query(void);
int ste_audio_io_set_mic1b_gain(AUDIOIO_CH_INDEX chnl_index,uint16 gain_index,
					int32 gain_value, uint32 linear);
int ste_audio_io_get_mic1b_gain(int*,int*,uint16);
int ste_audio_io_mute_mic1b(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_unmute_mic1b(AUDIOIO_CH_INDEX chnl_index, int *gain);
int ste_audio_io_mute_mic1b_state(void);
int ste_audio_io_enable_fade_mic1b(void);
int ste_audio_io_disable_fade_mic1b(void);


int ste_audio_io_power_up_mic2(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_down_mic2(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_state_mic2_query(void);
int ste_audio_io_set_mic2_gain(AUDIOIO_CH_INDEX chnl_index,uint16 gain_index,
					int32 gain_value, uint32 linear);
int ste_audio_io_get_mic2_gain(int*,int*,uint16);
int ste_audio_io_mute_mic2(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_unmute_mic2(AUDIOIO_CH_INDEX chnl_index, int *gain);
int ste_audio_io_mute_mic2_state(void);
int ste_audio_io_enable_fade_mic2(void);
int ste_audio_io_disable_fade_mic2(void);


int ste_audio_io_power_up_lin(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_down_lin(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_state_lin_query(void);
int ste_audio_io_set_lin_gain(AUDIOIO_CH_INDEX chnl_index,uint16 gain_index,
					int32 gain_value, uint32 linear);
int ste_audio_io_get_lin_gain(int*,int*,uint16);
int ste_audio_io_mute_lin(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_unmute_lin(AUDIOIO_CH_INDEX chnl_index, int *gain);
int ste_audio_io_mute_lin_state(void);
int ste_audio_io_enable_fade_lin(void);
int ste_audio_io_disable_fade_lin(void);


int ste_audio_io_power_up_dmic12(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_down_dmic12(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_state_dmic12_query(void);
int ste_audio_io_set_dmic12_gain(AUDIOIO_CH_INDEX chnl_index,uint16 gain_index,
					int32 gain_value, uint32 linear);
int ste_audio_io_get_dmic12_gain(int*,int*,uint16);
int ste_audio_io_mute_dmic12(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_unmute_dmic12(AUDIOIO_CH_INDEX chnl_index, int *gain);
int ste_audio_io_mute_dmic12_state(void);
int ste_audio_io_enable_fade_dmic12(void);
int ste_audio_io_disable_fade_dmic12(void);


int ste_audio_io_power_up_dmic34(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_down_dmic34(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_state_dmic34_query(void);
int ste_audio_io_set_dmic34_gain(AUDIOIO_CH_INDEX chnl_index,uint16 gain_index,
					int32 gain_value, uint32 linear);
int ste_audio_io_get_dmic34_gain(int*,int*,uint16);
int ste_audio_io_mute_dmic34(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_unmute_dmic34(AUDIOIO_CH_INDEX chnl_index, int *gain);
int ste_audio_io_mute_dmic34_state(void);
int ste_audio_io_enable_fade_dmic34(void);
int ste_audio_io_disable_fade_dmic34(void);


int ste_audio_io_power_up_dmic56(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_down_dmic56(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_power_state_dmic56_query(void);
int ste_audio_io_set_dmic56_gain(AUDIOIO_CH_INDEX chnl_index,uint16 gain_index,
					int32 gain_value, uint32 linear);
int ste_audio_io_get_dmic56_gain(int*,int*,uint16);
int ste_audio_io_mute_dmic56(AUDIOIO_CH_INDEX chnl_index);
int ste_audio_io_unmute_dmic56(AUDIOIO_CH_INDEX chnl_index, int *gain);
int ste_audio_io_mute_dmic56_state(void);
int ste_audio_io_enable_fade_dmic56(void);
int ste_audio_io_disable_fade_dmic56(void);

#endif

