/*
 * Copyright (C) ST-Ericsson SA 2010
 * Author: Deepak KARDA/ deepak.karda@stericsson.com for ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2.
 */


#include <linux/types.h>
#include "ste_audio_io_hwctrl_common.h"

/* Number of channels for each transducer */
const uint transducer_no_of_channels[MAX_NO_TRANSDUCERS] =
	{
	1,	/* Earpiece */
	2,	/* HS */
	2, 	/* IHF */
	1, 	/* VibL */
	1, 	/* VibR */
	1,	/* Mic1A */
	1,	/* Mic1B */
	1, 	/* Mic2 */
	2, 	/* LinIn */
	2, 	/* DMIC12 */
	2, 	/* DMIC34 */
	2, 	/* /DMIC56 */
	4	/* MultiMic */
	};

/* Maximum number of gains in each transducer path
 (all channels of a specific transducer have same max no of gains) */
const uint	transducer_no_of_gains[MAX_NO_TRANSDUCERS] =
	{
	2, 	/* Ear		g3 and g4 */
	3, 	/* HS 		g3 and g4 and analog */
	1, 	/* IHF		g3 */
	1, 	/* VibL		g3 */
	1, 	/* VibR		g3 */
	2, 	/* Mic1A		g1 and analog */
	2, 	/* Mic1A		g1 and analog */
	2, 	/* Mic2		g1 and analog */
	2, 	/* LinIn		g1 and analog */
	1,	/* DMIC12	g1 */
	1,	/* DMIC34	g1 */
	1,	/* DMIC56	g1 */
	1	/* MultiMic	g1 */
	};

/* Maximum number of supported loops for each transducer */
const uint	transducer_max_no_of_supported_loops[MAX_NO_TRANSDUCERS] =
	{
	0, 	/* Ear		g3 and g4 */
	0, 	/* HS 		g3 and g4 and analog */
	0, 	/* IHF		g3 */
	0, 	/* VibL		g3 */
	0, 	/* VibR		g3 */
	1, 	/* Mic1A		SideTone */
	1, 	/* Mic1A		SideTone */
	1, 	/* Mic2		SideTone */
	0, 	/* LinIn */
	0,	/* DMIC12	ANC */
	0,	/* DMIC34	ANC */
	0,	/* DMIC56 */
	1	/* MultiMic	ANC */
	};

/* Maximum number of gains supported by loops for each transducer */
const uint	transducer_no_of_loop_gains[MAX_NO_TRANSDUCERS] =
	{
	0, 	/* Ear		g3 and g4 */
	0, 	/* HS 		g3 and g4 and analog */
	0, 	/* IHF		g3 */
	0, 	/* VibL		g3 */
	0, 	/* VibR		g3 */
	1, 	/* Mic1A		SideTone g2 */
	1, 	/* Mic1A		SideTone g2 */
	1, 	/* Mic2		SideTone g2 */
	0, 	/* LinIn */
	0,	/* DMIC12	ANC */
	0,	/* DMIC34	ANC */
	0,	/* DMIC56 */
	1	/* MultiMic	ANC */
	};

/* Supported Loop Indexes for each transducer */
const uint transducer_no_of_supported_loops[MAX_NO_TRANSDUCERS] =
	{
	0, 	/* Ear		g3 and g4 */
	0, 	/* HS 		g3 and g4 and analog */
	0, 	/* IHF		g3 */
	0, 	/* VibL		g3 */
	0, 	/* VibR		g3 */
	2, 	/* Mic1A		SideTone g2 */
	2, 	/* Mic1A		SideTone g2 */
	2, 	/* Mic2		SideTone g2 */
	0, 	/* LinIn */
	0,	/* DMIC12	ANC */
	0,	/* DMIC34	ANC */
	0,	/* DMIC56 */
	1	/* MultiMic	ANC */
	};



struct gain_descriptor_t gain_descriptor[MAX_NO_TRANSDUCERS]\
		[MAX_NO_CHANNELS][MAX_NO_GAINS] =
{
	/* gainIndex=0	1			2
	EDestinationEar */
    {{{-63, 0, 1},	{-1, 8, 1},	{0, 0, 0} } ,/* channelIndex=0 */
    {{0, 0, 0},		{0, 0, 0},	{0, 0, 0} } ,/* channelIndex=1 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=2 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } } ,/* channelIndex=3 */

    /* EDestinationHS */
    {{{-63, 0, 1}, 	{-1, 8, 1},	{-32, 4, 2} } , /* channelIndex=0 */
    {{-63, 0, 1}, 	{-1, 8, 1},	{-32, 4, 2} } , /* channelIndex=1 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,	  /* channelIndex=2 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } } ,	  /* channelIndex=3 */

    /* EDestinationIHF */
    {{{-63, 0, 1}, 	{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=0 */
    {{-63, 0, 1}, 	{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=1 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=2 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } } ,/* channelIndex=3 */

    /* EDestinationVibL */
    {{{-63, 0, 1}, 	{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=0 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=1 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=2 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } } ,/* channelIndex=3 */

    /* EDestinationVibR */
    {{{-63, 0, 1}, 	{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=0 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=1 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=2 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } } ,/* channelIndex=3 */

    /* ESourceMic1A */
    {{{-32, 31, 1}, {0, 31, 1}, 	{0, 0, 0} } ,/* channelIndex=0 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=1 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=2 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } } ,/* channelIndex=3 */

    /* ESourceMic1B */
    {{{-32, 31, 1}, {0, 31, 1}, 	{0, 0, 0} } ,/* channelIndex=0 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=1 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=2 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } } ,/* channelIndex=3 */

    /* ESourceMic2 */
    {{{-32, 31, 1}, {0, 31, 1}, 	{0, 0, 0} } ,/* channelIndex=0 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=1 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=2 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } } ,/* channelIndex=3 */

    /* ESourceLin */
    {{{-32, 31, 1}, {-10, 20, 2},	{0, 0, 0} } ,/* channelIndex=0 */
    {{-32, 31, 1}, 	{-10, 20, 2},	{0, 0, 0} } ,/* channelIndex=1 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=2 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } } ,/* channelIndex=3 */

    /* ESourceDMic12 */
	{{{-32, 31, 1}, {0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=0 */
    {{-32, 31, 1},	{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=1 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=2 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } } ,/* channelIndex=3 */

    /* ESourceDMic34 */
    {{{-32, 31, 1}, {0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=0 */
    {{-32, 31, 1},	{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=1 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=2 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } } ,/* channelIndex=3 */

    /* ESourceDMic56 */
    {{{-32, 31, 1}, {0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=0 */
    {{-32, 31, 1},	{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=1 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=2 */
    {{0, 0, 0},		{0, 0, 0}, 	{0, 0, 0} } } ,/* channelIndex=3 */

    /* ESourceMultiMic */
    {{{-32, 31, 1}, {0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=0 */
    {{-32, 31, 1},	{0, 0, 0}, 	{0, 0, 0} } ,/* channelIndex=1 */
    {{-32, 31, 1},		{0, 0, 0}, 	{0, 0, 0} } ,
							/* channelIndex=2 */
    {{-32, 31, 1},		{0, 0, 0}, 	{0, 0, 0} } }
						/* channelIndex=3 */

};


const int hs_analog_gain_table[16] = {4, 2, 0, -2, -4, -6, -8, -10,
				-12, -14, -16, -18, -20, -24, -28, -32};



