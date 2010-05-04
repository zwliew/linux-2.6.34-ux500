/*
* Overview:
*   Header File for ste_audio_io_dev.c
*
* Copyright (C) 2009 ST Ericsson
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/


#ifndef _AUDIOIO_DEV_H_
#define _AUDIOIO_DEV_H_

#include "ste_audio_io_ioctl.h"
#include "ste_audio_io_core.h"

typedef union __audioio_cmd_data {
	audioio_burst_ctrl_t  audioio_burst_ctrl;
	audioio_fade_ctrl_t  audioio_fade_ctrl;
	audioio_mute_trnsdr_t  audioio_mute_trnsdr;
	audioio_gain_ctrl_trnsdr_t  audioio_gain_ctrl_trnsdr;
	audioio_gain_desc_trnsdr_t  audioio_gain_desc_trnsdr;
	audioio_support_loop_t  audioio_support_loop;
	audioio_gain_loop_t  audioio_gain_loop;
	audioio_get_gain_t  audioio_get_gain;
	audioio_loop_ctrl_t  audioio_loop_ctrl;
	audioio_pwr_ctrl_t  audioio_pwr_ctrl;
	audioio_data_t  audioio_data;
}audioio_cmd_data_t;


#endif /* _AUDIOIO_DEV_H_ */



