/*
 * Copyright ST-Ericsson 2009.
 *
 * Author: Arun Murthy <arun.murthy@stericsson.com>
 * Licensed under GPLv2.
 */

struct ab8500_bm_platform_data {
	int name;
	int termination_vol;	/* voltage in mV */
	int op_cur_lvl;		/* o/p current level in mA */
	int ip_vol_lvl;		/* i/p current level in mV */
};

/* ChVoltLevel */
#define CH_VOL_LVL_3P5		0x00
#define CH_VOL_LVL_4P05		0x16
#define CH_VOL_LVL_4P1		0x18
#define CH_VOL_LVL_4P6		0x4D

/* ChOutputCurrentLevel */
#define CH_OP_CUR_LVL_0P1	0x00
#define CH_OP_CUR_LVL_0P9	0x08
#define CH_OP_CUR_LVL_1P4	0x0D
#define CH_OP_CUR_LVL_1P5	0x0E
#define CH_OP_CUR_LVL_1P6	0x0F

/* UsbChCurrLevel */
#define USB_CH_IP_CUR_LVL_0P05	0x00
#define USB_CH_IP_CUR_LVL_0P09	0x10
#define USB_CH_IP_CUR_LVL_0P19	0x20
#define USB_CH_IP_CUR_LVL_0P29	0x30
#define USB_CH_IP_CUR_LVL_0P38	0x40
#define USB_CH_IP_CUR_LVL_0P45	0x50
#define USB_CH_IP_CUR_LVL_0P5	0x60
