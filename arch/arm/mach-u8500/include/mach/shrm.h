/*----------------------------------------------------------------------------*/
/* Copyright (C) ST Ericsson, 2009.                                           */
/*                                                                            */
/* This program is free software; you can redistribute it and/or modify it    */
/* under the terms of the GNU General Public License as published by the Free */
/* Software Foundation; either version 2.1 of the License, or (at your option */
/* any later version.                                                         */
/*                                                                            */
/* This program is distributed in the hope that it will be useful, but WITHOUT*/
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or      */
/* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for   */
/*more details.   							      */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.       */
/*----------------------------------------------------------------------------*/

#ifndef __SHM_DRIVER_IF_H__
#define __SHM_DRIVER_IF_H__

#include <linux/device.h>

struct shrm_plat_data {

	int  *pshm_dev_data;
	struct shm_device *shm_ctrlr;
};

typedef void (*rx_cb)(void *data, unsigned int length);
typedef void (*received_msg_handler)(unsigned char l2_header, \
			void *msg_ptr, unsigned int length);

#endif
