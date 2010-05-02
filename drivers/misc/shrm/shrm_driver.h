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


#ifndef __SHRM_DRIVER_H__
#define __SHRM_DRIVER_H__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>

#include <mach/shrm.h>

#include <linux/cdev.h>

#define BOOT_INIT  (0)
#define BOOT_INFO_SYNC  (1)
#define BOOT_DONE  (2)

/*
 * Struct definition to hold information about SHM
 */
struct shrm_dev {
	u8 ca_wake_irq;
	u8 ac_read_notif_0_irq;
	u8 ac_read_notif_1_irq;
	u8 ca_msg_pending_notif_0_irq;
	u8 ca_msg_pending_notif_1_irq;
	void __iomem *intr_base;
	void __iomem *ape_common_fifo_base;
	void __iomem *ape_audio_fifo_base;
	void __iomem *cmt_common_fifo_base;
	void __iomem *cmt_audio_fifo_base;

	unsigned int *ape_common_fifo_base_phy;
	unsigned int *ape_audio_fifo_base_phy;
	unsigned int *cmt_common_fifo_base_phy;
	unsigned int *cmt_audio_fifo_base_phy;

	int ape_common_fifo_size;
	int ape_audio_fifo_size;
	int cmt_common_fifo_size;
	int cmt_audio_fifo_size;

	void __iomem *ac_common_shared_wptr;
	void __iomem *ac_common_shared_rptr;
	void __iomem *ca_common_shared_wptr;
	void __iomem *ca_common_shared_rptr;

	void __iomem *ac_audio_shared_wptr;
	void __iomem *ac_audio_shared_rptr;
	void __iomem *ca_audio_shared_wptr;
	void __iomem *ca_audio_shared_rptr;

};

struct t_queue_element {
	struct list_head entry;
	u32 offset;
	u32 size;
	u32 no;
};

struct t_message_queue {
      unsigned char *p_fifo_base;
      u32 size;
      u32 readptr;
      u32 writeptr;
      u32 no;
      spinlock_t update_lock;
      struct semaphore tx_mutex;
      atomic_t q_rp;
      wait_queue_head_t wq_readable;
      struct list_head msg_list;
};

struct t_isadev_context {
      struct t_message_queue dl_queue;
      struct semaphore tx_mutex;
      u8 device_id;
};

struct t_isa_driver_context {
	atomic_t is_open[4];
	struct t_isadev_context *p_isadev[4];
	struct cdev cdev;     /* Char device structure */
};


#endif
