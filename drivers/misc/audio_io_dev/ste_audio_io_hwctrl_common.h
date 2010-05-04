/*
* Overview:
*   Internal definitions for Audio I/O driver
*
* Copyright (C) 2009 ST Ericsson
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/


#ifndef __AUDIOIO_HWCTRL_COMMON_H__
#define __AUDIOIO_HWCTRL_COMMON_H__

#include <linux/types.h>
#include "ste_audio_io_ioctl.h"
/*
 * Defines
 */


#define MAX_GAIN				100
#define MIN_GAIN				0
#define	MAX_NO_CHANNELS				4
#define	MAX_NO_GAINS				3
#define	MAX_NO_LOOPS 				1
#define	MAX_NO_LOOP_GAINS			1

typedef struct __gain_descriptor
{
	int min_gain;
	int max_gain;
	uint gain_step;
}gain_descriptor_t;


/*Number of channels for each transducer*/
extern const uint transducer_no_of_channels[MAX_NO_TRANSDUCERS];

/*Maximum number of gains in each transducer path
 (all channels of a specific transducer have same max no of gains)*/
extern const uint	transducer_no_of_gains[MAX_NO_TRANSDUCERS];

/*Maximum number of supported loops for each transducer*/
extern const uint	transducer_max_no_of_supported_loops[MAX_NO_TRANSDUCERS];

/*Maximum number of gains supported by loops for each transducer*/
extern const uint	transducer_no_of_loop_gains[MAX_NO_TRANSDUCERS];

/*Supported Loop Indexes for each transducer*/
extern const uint transducer_no_of_supported_loops[MAX_NO_TRANSDUCERS];

extern const int32 hs_analog_gain_table[16] ;

extern gain_descriptor_t gain_descriptor[MAX_NO_TRANSDUCERS][MAX_NO_CHANNELS][MAX_NO_GAINS];

#endif

/* End of audio_io_hwctrl_common.h */
