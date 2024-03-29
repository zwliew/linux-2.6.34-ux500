/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License, version 2
 * Author: Jayeeta Banerjee <jayeeta.banerjee@stericsson.com> for ST-Ericsson
 */

#ifndef __TC35893_KEYPAD_H
#define __TC35893_KEYPAD_H

/*
 * Keypad related platform specific constants
 * These values may be modified for fine tuning
 */
#define TC_KPD_ROWS 		0x8
#define TC_KPD_COLUMNS 		0x8
#define TC_KPD_DEBOUNCE_PERIOD 	0xA3
#define TC_KPD_SETTLE_TIME 	0xA3

/**
 * struct tc35893_platform_data - data structure for platform specific data
 * @kcode_tbl:		lookup table for keycodes
 * @krow:		mask for available rows, value is 0xFF
 * @kcol:               mask for available columns, value is 0xFF
 * @debounce_period:	platform specific debounce time
 * @settle_time:	platform specific settle down time
 * @irqtype:		type of interrupt, falling or rising edge
 * @irq:		irq no,
 * @enable_wakeup:	specifies if keypad event can wake up system from sleep
 * @scan_enable:	represents keypad hardware scan enable status
 * @op_mode:		Specifies polling of interrupt mode
 */
struct tc35893_platform_data {
	u8			*kcode_tbl;
	u8 			krow;
	u8 			kcol;
	u8 			debounce_period;
	u8			settle_time;
	unsigned long		irqtype;
	int			irq;
	bool			enable_wakeup;
	unsigned char  		scan_enable;
	int			op_mode;
};

#endif	/*__TC35893_KEYPAD_H*/
