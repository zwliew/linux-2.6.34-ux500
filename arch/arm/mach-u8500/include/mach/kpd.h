/*----------------------------------------------------------------------------------*/
/*  copyright STMicroelectronics, 2007.                                             */
/* Copyright (C)ST-Ericsson, 2009 						    */
/*                                                                                  */
/* This program is free software; you can redistribute it and/or modify it under    */
/* the terms of the GNU General Public License as published by the Free	            */
/* Software Foundation; either version 2.1 of the License, or (at your option)      */
/* any later version.                                                               */
/*                                                                                  */
/* This program is distributed in the hope that it will be useful, but WITHOUT      */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS    */
/* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.   */
/*                                                                                  */
/* You should have received a copy of the GNU General Public License                */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.             */
/*----------------------------------------------------------------------------------*/




#ifndef __U8500_KPD_H
#define __U8500_KPD_H

#ifndef CONFIG_AUTOSCAN_ENABLED
#define CONFIG_AUTOSCAN_ENABLED 1
#endif
#include <mach/hardware.h>

#define MAX_KPROW               8
#define MAX_KPCOL               8
/*keypad related Constants*/
#define KEYPAD_RELEASE_PERIOD   11	/*110 Msec, repeate key scan time */
#define KEYPAD_SCAN_PERIOD      4	/*40Msec for new keypress */
#define KEYPAD_STATE_DEFAULT    0
#define KEYPAD_STATE_PRESSED    1
#define KEYPAD_STATE_PRESSACK   2
#define KEYPAD_STATE_RELEASED   3
#define KEYPAD_STATE_RELEASEACK 2

#define KPINTR_LKBIT		0	/*bit used for interrupt locking */

struct keypad_t;
/**
 * struct keypad_device -
 * platform data for keypad driver
 * @init:		pointer to keypad init function
 * @exit:		pointer to keypad deinitialisation function
 * @autoscan_check:	pointer to read autoscan status function , not used currently
 * @autoscan_disable:	pointer to autoscan feature disable function, not used currently
 * @autoscan_results:	pointer to read autoscan results function
 * @autoscan_en:	pointer to enable autoscan feature function, not used currently
 * @irqen:		pointer to enable irq function
 * @irqdis:		pointer to disable irq function
 * @kcode_tbl:		lookup table for keycodes
 * @krow:		mask for available rows, value is 0xFF
 * @kcol:               mask for available columns, value is 0xFF
 * @irqdis_int:		pointer to disable irq function, to be called from ISR
 * @debounce_period:	platform specific debounce time, can be fine tuned later
 * @irqtype:		type of interrupt
 * @irq:		irq no,
 * @int_status:		interrupt status
 * @int_line_behaviour:	dynamis interrupt line behaviour
 */
struct keypad_device {
	int	(*init)(struct keypad_t *kp);
	int 	(*exit)(struct keypad_t *kp);
	int 	(*autoscan_check)(void);
	void 	(*autoscan_disable)(void);
	int 	(*autoscan_results)(struct keypad_t *kp);
	void 	(*autoscan_en)(void);
	int 	(*irqen)(struct keypad_t *kp);
	int 	(*irqdis)(struct keypad_t *kp); /* normal disable */
	u8 	*kcode_tbl;
	u8 	krow;
	u8 	kcol;
	int 	(*irqdis_int)(struct keypad_t *kp); /* func used wen disable in interrupt handler */
	u8 	debounce_period;
	unsigned long	irqtype;
	u8	irq; /*IRQ no*/
	u8      int_status;
	u8 	int_line_behaviour;
};

/**
 * struct keypad_t - keypad data structure used internally by keypad driver
 * @irq:		irq no
 * @mode:		0 for interrupt mode, 1 for polling mode
 * @key_state:		array for saving keystates
 * @lockbits: 		used for synchronisation in ISR
 * @inp_dev:		pointer to input device object
 * @address_for_data:	not used
 * @kscan_work:		work queue
 * @board: 		keypad platform device
 */
struct keypad_t {
	int irq;
	int mode;
	int key_state[MAX_KPROW][MAX_KPCOL];
	unsigned long lockbits;
	struct input_dev *inp_dev;
	void *address_for_data;
	struct delayed_work kscan_work;
	struct keypad_device *board;
};
/**
 * enum kp_int_status - enum for INTR status
 * @KP_INT_DISABLED:	interrupt disabled
 * @KP_INT_ENABLED:	interrupt enabled
 */
enum kp_int_status{
	KP_INT_DISABLED=0,
	KP_INT_ENABLED,
};
/**
 *enum kp_int_line_behaviour - enum for kp_int_line dynamic status
 * @INT_LINE_NOTSET: 	IRQ not asserted
 * @INT_LINE_SET: 	IRQ asserted
 */
enum kp_int_line_behaviour {
	INT_LINE_NOTSET=0,
	INT_LINE_SET,
};
#endif	/*__U8500_KPD_H*/
