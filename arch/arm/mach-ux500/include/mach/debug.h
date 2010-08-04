/*
 * Copyright (c) 2009 STMicroelectronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */
#ifndef __INC_DBG_H
#define __INC_DBG_H

/* Store a submitter ID, unique for each HCL. */

struct driver_debug_st {
	int mtd;
	int gpio;
	int mmc;
	int ssp;
	int mtu;
	int msp;
	int spi;
	int touch;
	int dma;
	int rtc;
	int acodec;
	int tourg;
	int alsa;
	int keypad;
	int mcde;
	int power;
	int i2c;
	int hsi;
};

#define stm_error(format, arg...) printk(KERN_ERR DRIVER_DEBUG_PFX ":ERROR " format "\n" , ## arg)
#define stm_warn(format, arg...)  printk(KERN_WARNING DRIVER_DEBUG_PFX ":WARNING " format "\n" , ## arg)
#define stm_info(format, arg...)  printk(KERN_INFO DRIVER_DEBUG_PFX ":INFO" format "\n" , ## arg)


#define stm_dbg(debug, format, arg...) (!(DRIVER_DEBUG)) ? ({do {} while(0); }) : debug == 1 ? (printk(DRIVER_DBG DRIVER_DEBUG_PFX ": " format "\n" , ## arg)) : ({do {} while (0); })

#define stm_dbg2(debug, format, arg...)	(!(DRIVER_DEBUG)) ? ({do {} while(0); }) : debug == 2 ? (printk(DRIVER_DBG DRIVER_DEBUG_PFX ": " format "\n" , ## arg)) : ({do {} while (0); })

#define stm_dbg3(debug, format, arg...) (!(DRIVER_DEBUG)) ? ({do {} while(0); }) : debug == 3 ? (printk(DRIVER_DBG DRIVER_DEBUG_PFX ": " format "\n" , ## arg)) : ({do {} while (0); })

#define stm_dbg4(format, arg...)	(DRIVER_DEBUG & 1) ? (printk(DRIVER_DBG DRIVER_DEBUG_PFX ": " format "\n" , ## arg)) : ({do {} while (0); })

extern struct driver_debug_st DBG_ST;
#endif

/* __INC_DBG_H */

/* End of file - debug.h */
