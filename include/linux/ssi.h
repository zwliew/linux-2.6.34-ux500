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

#define SSIREXINTR_PORT1 0x20000
#define SSIREXINTR_PORT0 0x10000

#define ANY_SSI_CONTROLLER	-1
#define ANY_CHANNEL		-1
#define CHANNEL(channel)	(1<<channel)
#define SSI_PREFIX "ssi:"

#define SSIT_CTRLR_ID 0x0
#define SSIR_CTRLR_ID 0x1

enum {
	SSI_EXCEP_SIGNAL,
	SSI_EXCEP_TIMEOUT,
	SSI_EXCEP_BREAK,
	SSI_RXCHANNELS_OVERRUN,
};

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

ssi_pin_state ssi_get_cawake(void);
ssi_pin_state ssi_get_acwake(void);
int ssi_set_acwake(ssi_pin_state value);
enum ssi_mode{
       SSI_INTELLIGENT_MODE=0x0,
       SSI_INTERRUPT_MODE,
       SSI_DMA_MODE,
};


struct ssi_device {
	int ctrlrid;
	enum ssi_mode curr_mode;
	unsigned int chid;
	char modalias[BUS_ID_SIZE];
	struct ssi_channel *ch;
	struct device device;
};


#define to_ssi_device(dev)	container_of(dev, struct ssi_device, device)

struct ssi_device_driver {
	u32		ctrl_mask;
	u32		ch_mask;
	u32		excep_mask;
	void 			(*excep_event) (unsigned int c_id,
						unsigned int event, void *arg);
	int			(*probe)(struct ssi_device *dev);
	int			(*remove)(struct ssi_device *dev);
	int			(*suspend)(struct ssi_device *dev,
						pm_message_t mesg);
	int			(*resume)(struct ssi_device *dev);
	struct device_driver 	driver;
};


#define to_ssi_device_driver(drv) container_of(drv, \
						struct ssi_device_driver, \
						driver)
extern struct bus_type ssi_bus_type;
#if 1

int register_ssi_driver(struct ssi_device_driver *driver);
void unregister_ssi_driver(struct ssi_device_driver *driver);
int ssi_open(struct ssi_device *dev);
int ssi_write(struct ssi_device *dev, t_data_buf data, unsigned int count);
void ssi_write_cancel(struct ssi_device *dev);
int ssi_read(struct ssi_device *dev, t_data_buf data,unsigned int count);
void ssi_read_cancel(struct ssi_device *dev);
int ssi_ioctl(struct ssi_device *dev, unsigned int command, void *arg);
void ssi_close(struct ssi_device *dev);
void ssi_dev_set_cb(struct ssi_device *dev, void (*r_cb)(struct ssi_device *dev)
					, void (*w_cb)(struct ssi_device *dev));
void ssi_dev_set_event_handler(int channel_num, int event_mask,
				void (*channel_event_handler) (int channel_num, unsigned int event, void *arg));
#endif
#endif
