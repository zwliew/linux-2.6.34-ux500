/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name          : lsm303dlh.h
* Authors            : MH - C&I BU - Application Team
*		     : Carmine Iascone (carmine.iascone@st.com)
*		     : Matteo Dameno (matteo.dameno@st.com)
* Version            : V 0.3
* Date               : 24/02/2010
*
********************************************************************************
*
* Author: Mian Yousaf Kaukab <mian.yousaf.kaukab@stericsson.com>
* Copyright 2010 (c) ST-Ericsson AB
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
*
*******************************************************************************/

#ifndef __LSM303DLH_H__
#define __LSM303DLH_H__

#include <linux/ioctl.h>

#define LSM303DLH_IOCTL_BASE 0xA2

/* Accelerometer IOCTLs */
#define LSM303DLH_A_SET_RANGE _IOW(LSM303DLH_IOCTL_BASE, 1, char)
#define LSM303DLH_A_SET_MODE _IOW(LSM303DLH_IOCTL_BASE, 2, char)
#define LSM303DLH_A_SET_RATE _IOW(LSM303DLH_IOCTL_BASE, 3, char)

/* Magnetometer IOCTLs */
#define LSM303DLH_M_SET_RANGE _IOW(LSM303DLH_IOCTL_BASE, 11, char)
#define LSM303DLH_M_SET_MODE _IOW(LSM303DLH_IOCTL_BASE, 12, char)
#define LSM303DLH_M_SET_RATE _IOW(LSM303DLH_IOCTL_BASE, 13, char)
#define LSM303DLH_M_GET_GAIN _IOR(LSM303DLH_IOCTL_BASE, 14, short[3])


/* ioctl: LSM303DLH_A_SET_RANGE */
#define LSM303DLH_A_RANGE_2G 0x00
#define LSM303DLH_A_RANGE_4G 0x01
#define LSM303DLH_A_RANGE_8G 0x03

/* ioctl: LSM303DLH_A_SET_MODE */
#define LSM303DLH_A_MODE_OFF 0x00
#define LSM303DLH_A_MODE_NORMAL 0x01
#define LSM303DLH_A_MODE_LP_HALF 0x02
#define LSM303DLH_A_MODE_LP_1 0x03
#define LSM303DLH_A_MODE_LP_2 0x02
#define LSM303DLH_A_MODE_LP_5 0x05
#define LSM303DLH_A_MODE_LP_10 0x06

/* ioctl: LSM303DLH_A_SET_BW */
#define LSM303DLH_A_RATE_50 0x00
#define LSM303DLH_A_RATE_100 0x01
#define LSM303DLH_A_RATE_400 0x02
#define LSM303DLH_A_RATE_1000 0x03

/* Magnetometer gain setting
 * ioctl: LSM303DLH_M_SET_RANGE
 */
#define LSM303DLH_M_RANGE_1_3G  0x01
#define LSM303DLH_M_RANGE_1_9G  0x02
#define LSM303DLH_M_RANGE_2_5G  0x03
#define LSM303DLH_M_RANGE_4_0G  0x04
#define LSM303DLH_M_RANGE_4_7G  0x05
#define LSM303DLH_M_RANGE_5_6G  0x06
#define LSM303DLH_M_RANGE_8_1G  0x07

/* Magnetometer capturing mode  */
/* ioctl: LSM303DLH_M_SET_MODE */
#define LSM303DLH_M_MODE_CONTINUOUS 0
#define LSM303DLH_M_MODE_SINGLE 1
#define LSM303DLH_M_MODE_SLEEP 3

/* Magnetometer output data rate */
/* ioctl: LSM303DLH_M_SET_BANDWIDTH */
#define LSM303DLH_M_RATE_00_75 0x00 /* 00.75 Hz */
#define LSM303DLH_M_RATE_01_50 0x01 /* 01.50 Hz */
#define LSM303DLH_M_RATE_03_00 0x02 /* 03.00 Hz */
#define LSM303DLH_M_RATE_07_50 0x03 /* 07.50 Hz */
#define LSM303DLH_M_RATE_15_00 0x04 /* 15.00 Hz */
#define LSM303DLH_M_RATE_30_00 0x05 /* 30.00 Hz */
#define LSM303DLH_M_RATE_75_00 0x06 /* 75.00 Hz */

#ifdef __KERNEL__
struct lsm303dlh_platform_data {
	int (*register_irq)(struct device *dev,
			void *data,
			void (*gpio_ev_irq_handler)(void *data));
	int (*free_irq)(struct device *dev);

	u8 axis_map_x; /* [0-2] */
	u8 axis_map_y; /* [0-2] */
	u8 axis_map_z; /* [0-2] */

	u8 negative_x; /* [0-1] */
	u8 negative_y; /* [0-1] */
	u8 negative_z; /* [0-1] */
};
#endif /* __KERNEL__ */

#endif  /* __LSM303DLH_H__ */
