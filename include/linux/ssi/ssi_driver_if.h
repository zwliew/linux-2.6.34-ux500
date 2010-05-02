/*
 * ssi_driver_if.h
 *
 * Header for the SSI driver low level interface.
 *
 * Copyright (C) 2007-2008 Nokia Corporation. All rights reserved.
 *
 * Contact: Carlos Chinea <carlos.chinea@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */
#ifndef __SSI_DRIVER_IF_H__
#define __SSI_DRIVER_IF_H__

#include <linux/device.h>


/* The number of ports handled by the driver. (MAX:2) */
#define SSI_MAX_SUPPORTED_CH		8
#define SSI_MAX_PORTS		2
#define SSI_DEFAULT_PORT	1
#define SSI_CONTROL_CHANNEL	0x7
#define SSI_DATA_CHANNEL	0x4

/*
 * Masks used to enable or disable the reception of certain hardware events
 * for the ssi_device_drivers
 */
#define SSI_EVENT_CLEAR			0x00
#define SSI_EVENT_MASK			0xFF
#define SSI_EVENT_BREAK_DETECTED_MASK	0x01
#define SSI_EVENT_ERROR_MASK		0x02

#define ANY_SSI_CONTROLLER	-1
#define ANY_CHANNEL		-1
#define CHANNEL(channel)	(1<<channel)

enum {
	SSI_EVENT_BREAK_DETECTED = 0,
	SSI_EVENT_SIGNAL_ERROR,
	SSI_EVENT_ERROR,
};

typedef enum {
	SSI_PIN_LOW = 0,
	SSI_PIN_HIGH,
} ssi_pin_state;

/*
* The memory allocated by dma_alloc_coherent() is supplied by caller
* both physical and logical address is required for that
*/
typedef struct {
	dma_addr_t phys_address;
	u32*  log_address;
}t_data_buf;

int ssi_open(int channel_num);
int ssi_write(int channel_num, t_data_buf data, unsigned int count);
void ssi_write_cancel(int channel_num);
int ssi_read(int channel_num, t_data_buf data, unsigned int count);
void ssi_read_cancel(int channel_num);
ssi_pin_state ssi_get_cawake();
ssi_pin_state ssi_get_acwake();
int ssi_set_acwake(ssi_pin_state value);
void ssi_dev_set_cb(int channel_num, void (*r_cb)(int channel_num, u32 *addr, int count, int error)
					, void (*w_cb)(int channel_num, u32 *addr, int count, int error));
void ssi_dev_set_event_handler(int channel_num, int event_mask,
				void (*channel_event_handler) (int channel, unsigned int event, void *arg));
void ssi_close(int channel_num);

#endif
