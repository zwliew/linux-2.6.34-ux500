/*
* Overview:
*   Header File defining IOCTLs for Audio IO interface
*
* Copyright (C) 2009 ST Ericsson
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as 
* published by the Free Software Foundation.
*/


#ifndef _AUDIOIO_IOCTL_H_
#define _AUDIOIO_IOCTL_H_


#define AUDIOIO_IOC_MAGIC            'N'
#define AUDIOIO_READ_REGISTER        _IOWR(AUDIOIO_IOC_MAGIC, 1,\
								audioio_data_t)
#define AUDIOIO_WRITE_REGISTER       _IOW(AUDIOIO_IOC_MAGIC, 2,\
								audioio_data_t)
#define AUDIOIO_PWR_CTRL_TRNSDR      _IOW(AUDIOIO_IOC_MAGIC, 3,\
							audioio_pwr_ctrl_t)
#define AUDIOIO_PWR_STS_TRNSDR       _IOR(AUDIOIO_IOC_MAGIC, 4,\
							audioio_pwr_ctrl_t)
#define AUDIOIO_LOOP_CTRL            _IOW(AUDIOIO_IOC_MAGIC, 5,\
							audioio_loop_ctrl_t)
#define AUDIOIO_LOOP_STS             _IOR(AUDIOIO_IOC_MAGIC, 6,\
							audioio_loop_ctrl_t)
#define AUDIOIO_GET_TRNSDR_GAIN_CAPABILITY      _IOR(AUDIOIO_IOC_MAGIC, 7,\
							audioio_get_gain_t)
#define AUDIOIO_GAIN_CAP_LOOP        _IOR(AUDIOIO_IOC_MAGIC, 8,\
							audioio_gain_loop_t)
#define AUDIOIO_SUPPORT_LOOP         _IOR(AUDIOIO_IOC_MAGIC, 9,\
							audioio_support_loop_t)
#define AUDIOIO_GAIN_DESC_TRNSDR     _IOR(AUDIOIO_IOC_MAGIC, 10,\
						audioio_gain_desc_trnsdr_t)
#define AUDIOIO_GAIN_CTRL_TRNSDR     _IOW(AUDIOIO_IOC_MAGIC, 11,\
						audioio_gain_ctrl_trnsdr_t)
#define AUDIOIO_GAIN_QUERY_TRNSDR    _IOR(AUDIOIO_IOC_MAGIC, 12,\
						audioio_gain_ctrl_trnsdr_t)
#define AUDIOIO_MUTE_CTRL_TRNSDR     _IOW(AUDIOIO_IOC_MAGIC, 13,\
							audioio_mute_trnsdr_t)
#define AUDIOIO_MUTE_STS_TRNSDR      _IOR(AUDIOIO_IOC_MAGIC, 14,\
							audioio_mute_trnsdr_t)
#define AUDIOIO_FADE_CTRL       	_IOW(AUDIOIO_IOC_MAGIC, 15,\
							audioio_fade_ctrl_t)
#define AUDIOIO_BURST_CTRL            _IOW(AUDIOIO_IOC_MAGIC, 16,\
							audioio_burst_ctrl_t)
#define AUDIOIO_READ_ALL_ACODEC_REGS_CTRL     _IOW(AUDIOIO_IOC_MAGIC, 17,\
					audioio_read_all_acodec_reg_ctrl_t)

/* audio codec channel ids */
#define EAR_CH			0
#define HS_CH			1
#define IHF_CH			2
#define VIBL_CH			3
#define VIBR_CH			4
#define MIC1A_CH		5
#define MIC1B_CH		6
#define MIC2_CH			7
#define LIN_CH			8
#define DMIC12_CH		9
#define DMIC34_CH		10
#define DMIC56_CH		11
#define MULTI_MIC_CH		12

#define MAX_NO_TRANSDUCERS	13

#define AUDIOIO_TRUE		1
#define AUDIOIO_FALSE		0

/** 16 bit unsigned quantity that is 16 bit word aligned */
typedef unsigned short uint16;

/** 16 bit signed quantity that is 16 bit word aligned */
typedef signed short int16;

/** 32 bit unsigned quantity that is 32 bit word aligned */
typedef unsigned long uint32;

/** signed quantity that is 32 bit word aligned */
typedef signed long int32;



typedef enum AUDIOIO_COMMON_SWITCH {
	AUDIOIO_COMMON_OFF = 0,
	AUDIOIO_COMMON_ON,
	AUDIOIO_COMMON_ALLCHANNEL_UNSUPPORTED = 0xFFFF
} AUDIOIO_COMMON_SWITCH;


typedef enum AUDIOIO_HAL_HW_LOOPS {
	AUDIOIO_NOLOOP = 0,
	AUDIOIO_SIDETONE_LOOP = 0xFFFF
} AUDIOIO_HAL_HW_LOOPS;


typedef enum AUDIOIO_FADE_PERIOD {
	e_FADE_00,
	e_FADE_01,
	e_FADE_10,
	e_FADE_11
}AUDIOIO_FADE_PERIOD;

typedef enum AUDIOIO_CH_INDEX {
	e_CHANNEL_1 = 0x01,
	e_CHANNEL_2 = 0x02,
	e_CHANNEL_3 = 0x04,
	e_CHANNEL_4 = 0x08,
	e_CHANNEL_ALL = 0x0f
}AUDIOIO_CH_INDEX;

typedef struct __audioio_data  {
	unsigned char	block;
	unsigned char	addr;
	unsigned char	data;
}audioio_data_t;

typedef struct __audioio_pwr_ctrl {
	AUDIOIO_COMMON_SWITCH ctrl_switch;
	int channel_type;
	AUDIOIO_CH_INDEX channel_index;
}audioio_pwr_ctrl_t;

typedef struct __audioio_loop_ctrl  {
	AUDIOIO_HAL_HW_LOOPS hw_loop;
	AUDIOIO_COMMON_SWITCH ctrl_switch;
	int channel_type;
	AUDIOIO_CH_INDEX channel_index;
}audioio_loop_ctrl_t;

typedef struct __audioio_get_gain {
	uint32 num_channels;
	uint16 max_num_gain;
}audioio_get_gain_t;

typedef struct __audioio_gain_loop {
	uint16 num_loop;
	uint16 max_gains;
}audioio_gain_loop_t;

typedef struct __audioio_support_loop {
	uint16 spprtd_loop_index;
}audioio_support_loop_t;

typedef struct __audioio_gain_desc_trnsdr {
	AUDIOIO_CH_INDEX channel_index;
	int channel_type;
	uint16 gain_index;
	int32 min_gain;
	int32 max_gain;
	uint32 gain_step;
}audioio_gain_desc_trnsdr_t;

typedef struct __audioio_gain_ctrl_trnsdr {
	AUDIOIO_CH_INDEX channel_index;
	int channel_type;
	uint16 gain_index;
	int32 gain_value;
	uint32 linear;
}audioio_gain_ctrl_trnsdr_t;

typedef struct __audioio_mute_trnsdr  {
	int channel_type;
	AUDIOIO_CH_INDEX channel_index;
	AUDIOIO_COMMON_SWITCH ctrl_switch;
}audioio_mute_trnsdr_t;

typedef struct __audioio_fade_ctrl  {
	AUDIOIO_COMMON_SWITCH ctrl_switch;
	AUDIOIO_FADE_PERIOD fade_period;
	int channel_type;
	AUDIOIO_CH_INDEX channel_index;
}audioio_fade_ctrl_t;

typedef struct __audioio_burst_ctrl {
	AUDIOIO_COMMON_SWITCH ctrl_switch;
	int channel_type;
	int32 burst_fifo_interrupt_sample_count;
	int32 burst_fifo_length;/*BFIFOTx*/
	int32 burst_fifo_switch_frame;
	int32 burst_fifo_sample_number;
}audioio_burst_ctrl_t;

typedef struct __audioio_read_all_acodec_reg_ctrl_t {
	unsigned char data[200];
}audioio_read_all_acodec_reg_ctrl_t;


#endif
