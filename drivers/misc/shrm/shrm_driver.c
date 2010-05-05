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

#include <linux/err.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/smp_lock.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <asm/atomic.h>
#include <linux/io.h>

#include <mach/isa_ioctl.h>
#include "shrm_driver.h"
#include "shrm_private.h"
#include "shrm_config.h"
#include <mach/shrm.h>


#ifdef CONFIG_HIGH_RES_TIMERS
#include <linux/hrtimer.h>
static struct hrtimer timer;
#endif


#define NAME "IPC_ISA"
#define ISA_DEVICES 4
/**debug functionality*/
#define ISA_DEBUG 0
#define dbg_printk(format, arg...) (ISA_DEBUG & 1) ? \
		(printk(KERN_ALERT NAME ": " format , ## arg)) : \
		({do {} while (0); })

#define ISI_MESSAGING (0)
#define RPC_MESSAGING (1)
#define AUDIO_MESSAGING (2)
#define SECURITY_MESSAGING (3)

#define SIZE_OF_FIFO (512*1024)

static unsigned char message_fifo[4][SIZE_OF_FIFO];

static unsigned char wr_isi_msg[10*1024];
static unsigned char wr_rpc_msg[10*1024];
static unsigned char wr_sec_msg[10*1024];
static unsigned char wr_audio_msg[10*1024];

/**global data*/
/*
int major:This variable is exported to user as module_param to specify
major number at load time
*/
static int major;
module_param(major, int, 0);
MODULE_PARM_DESC(major, "Major device number");
/**global fops mutex*/
static DEFINE_MUTEX(isa_lock);
/**global driver context pointer */
struct t_isa_driver_context *p_isa_context;
rx_cb common_rx;
rx_cb audio_rx;


struct shrm_dev *pshm_dev;

static void isi_receive(void *p_data, u32 n_bytes);
static void rpc_receive(void *p_data, u32 n_bytes);
static void audio_receive(void *p_data, u32 n_bytes);
static void security_receive(void *p_data, u32 n_bytes);

static void rx_common_l2msg_handler(unsigned char l2_header,
				 void *msg, unsigned int length)
{
#ifdef CONFIG_U8500_SHRM_LOOP_BACK
	unsigned char *pdata;
#endif
    dbgprintk("++ \n");

	switch (l2_header) {

	case 0:
		isi_receive(msg, length);
	break;
	case 1:
		rpc_receive(msg, length);
	break;
	case 3:
		security_receive(msg, length);
	break;
#ifdef CONFIG_U8500_SHRM_LOOP_BACK
	case 0xC0:
		pdata = (unsigned char *)msg;
		if ((*pdata == 0x50) || (*pdata == 0xAF))
			isi_receive(msg, length);
		else if ((*pdata == 0x0A) || (*pdata == 0xF5))
			rpc_receive(msg, length);
		else if ((*pdata == 0xFF) || (*pdata == 0x00))
			security_receive(msg, length);
	break;
#endif
	default:
	break;
	}

	dbgprintk("-- \n");
}

static void rx_audio_l2msg_handler(unsigned char l2_header,
				void *msg, unsigned int length)
{
	dbgprintk("++ \n");

	 audio_receive(msg, length);

	dbgprintk("-- \n");
}


static int __init shm_initialise_irq(struct shrm_dev *shm_dev_data)
{
	int err = 0;

	shm_protocol_init(rx_common_l2msg_handler, rx_audio_l2msg_handler);

	err = request_irq(shm_dev_data->ca_wake_irq,
			ca_wake_irq_handler, IRQF_TRIGGER_RISING,
				 "ca_wake-up", shm_dev_data);
		if (err < 0) {
			printk("Unable to allocate shm tx interrupt line\n");
			err = -EBUSY;
		free_irq(shm_dev_data->ca_wake_irq, shm_dev_data);
	}

	err = request_irq(shm_dev_data->ac_read_notif_0_irq, \
		ac_read_notif_0_irq_handler, 0, \
		"ac_read_notif_0", shm_dev_data);
	if (err < 0) {
		printk(KERN_ALERT "error ac_read_notif_0_irq interrupt line\n");
		err = -EBUSY;
		free_irq(shm_dev_data->ac_read_notif_0_irq, shm_dev_data);
	}

	err = request_irq(shm_dev_data->ac_read_notif_1_irq, \
		ac_read_notif_1_irq_handler, 0, \
		"ac_read_notif_1", shm_dev_data);
	if (err < 0) {
		printk(KERN_ALERT "error ac_read_notif_1_irq interrupt line\n");
		err = -EBUSY;
		free_irq(shm_dev_data->ac_read_notif_1_irq, shm_dev_data);
	}

	err = request_irq(shm_dev_data->ca_msg_pending_notif_0_irq, \
		 ca_msg_pending_notif_0_irq_handler, 0, \
		"ca_msg_pending_notif_0", shm_dev_data);
	if (err < 0) {
		printk(KERN_ALERT "error  ca_msg_pending_notif_0_irq line\n");
		err = -EBUSY;
		free_irq(shm_dev_data->ca_msg_pending_notif_0_irq, \
							shm_dev_data);
	}

	err = request_irq(shm_dev_data->ca_msg_pending_notif_1_irq, \
		 ca_msg_pending_notif_1_irq_handler, 0, \
		"ca_msg_pending_notif_1", shm_dev_data);
	if (err < 0) {
		printk(KERN_ALERT "error ca_msg_pending_notif_1_irq interrupt line\n");
		err = -EBUSY;
		free_irq(shm_dev_data->ca_msg_pending_notif_1_irq, \
							shm_dev_data);
	}


	return err;
}




static void free_shm_irq(struct shrm_dev *shm_dev_data)
{
	free_irq(shm_dev_data->ca_wake_irq, shm_dev_data);
	free_irq(shm_dev_data->ac_read_notif_0_irq, shm_dev_data);
	free_irq(shm_dev_data->ac_read_notif_1_irq, shm_dev_data);
	free_irq(shm_dev_data->ca_msg_pending_notif_0_irq, shm_dev_data);
	free_irq(shm_dev_data->ca_msg_pending_notif_1_irq, shm_dev_data);
}

/**
 * create_queue() - To create FIFO for Tx and Rx message buffering.
 * @q: message queue.
 * @devicetype: device type 0-isi,1-rpc,2-audio,3-security.
 *
 * This function creates a FIFO buffer of n_bytes size using
 * dma_alloc_coherent(). It also initializes all queue handling
 * locks, queue management pointers. It also initializes message list
 * which occupies this queue.
 *
 * It return -ENOMEM in case of no memory.
 */
static int create_queue(struct t_message_queue *q, u32 devicetype)
{
	q->p_fifo_base = (unsigned char *)&message_fifo[devicetype];
	q->size = SIZE_OF_FIFO;
	q->readptr = 0;
	q->writeptr = 0;
	q->no = 0;
	spin_lock_init(&q->update_lock);
	sema_init(&q->tx_mutex, 1);
	INIT_LIST_HEAD(&q->msg_list);
	init_waitqueue_head(&q->wq_readable);
	atomic_set(&q->q_rp, 0);

	return 0;
}
/**
 * delete_queue() - To delete FIFO and assiciated memory.
 * @q: message queue
 *
 * This function deletes FIFO created using create_queue() function.
 * It resets queue management pointers.
 */
static void delete_queue(struct t_message_queue *q)
{
	q->size = 0;
	q->readptr = 0;
	q->writeptr = 0;

}

/**
 * add_msg_to_queue() - Add a message inside inside queue
 *
 * @q: message queue
 * @size: size in bytes
 *
 * This function tries to allocate n_bytes of size in FIFO q.
 * It returns negative number when no memory can be allocated
 * currently.
 */
void add_msg_to_queue(struct t_message_queue *q, unsigned int size)
{
	struct t_queue_element *new_msg = NULL;

	dbgprintk("++ q->writeptr=%d \n", q->writeptr);
	new_msg = kmalloc(sizeof(struct t_queue_element),
						 GFP_KERNEL|GFP_ATOMIC);

	if (new_msg == NULL) {
		printk(KERN_ALERT ":memory overflow inside while(1) \n");
		BUG_ON(new_msg == NULL);
	}
	new_msg->offset = q->writeptr;
	new_msg->size = size;
	new_msg->no = q->no++;

	/*check for overflow condition*/
	if (q->readptr <= q->writeptr) {
		if (((q->writeptr-q->readptr) + size) >= q->size) {
			printk(KERN_ALERT "Buffer overflow**************************\n");
			BUG_ON(((q->writeptr-q->readptr) + size) >= q->size);
		}
	} else {
		if ((q->writeptr + size) >= q->readptr) {
			printk(KERN_ALERT "Buffer overflow**************************\n");
			BUG_ON((q->writeptr + size) >= q->readptr);
		}
	}


	q->writeptr = (q->writeptr + size) % q->size;

	if (list_empty(&q->msg_list)) {
		list_add_tail(&new_msg->entry, &q->msg_list);
		/*There can be 2 blocking calls read  and another select */

		atomic_set(&q->q_rp, 1);
		wake_up_interruptible(&q->wq_readable);
	} else
		list_add_tail(&new_msg->entry, &q->msg_list);


	dbgprintk("-- \n");
}

/**
 * remove_msg_from_queue() - To remove a message from the msg queue.
 *
 * @q: message queue
 *
 * This function delets a message from the message list associated with message
 * queue q and also updates read ptr.
 * If the message list is empty, then, event is set to block the select and
 * read calls of the paricular queue.
 *
 * The message list is FIFO style and message is always added to tail and
 * removed from head.
 */

void remove_msg_from_queue(struct t_message_queue *q)
{
	struct t_queue_element *old_msg = NULL;
	struct list_head *msg;

	dbgprintk("++ q->readptr %d\n", q->readptr);

	list_for_each(msg, &q->msg_list) {
		old_msg = list_entry(msg, struct t_queue_element, entry);
		if (old_msg == NULL) {
			printk(KERN_ALERT ":no message found\n");
			BUG_ON(old_msg == NULL);
		}
		break;
	}

	list_del(msg);

	q->readptr = (q->readptr + old_msg->size) % q->size;

	if (list_empty(&q->msg_list)) {
		dbgprintk("List is empty setting RP= 0\n");
		atomic_set(&q->q_rp, 0);
	}

	kfree(old_msg);

	dbgprintk("-- \n");
}

/**
 * get_size_of_new_msg() - retrieve new message from message list
 *
 * @q: message queue
 *
 * This function will retrieve most recent message from the corresponding
 * queue list. New message is always retrieved from head side.
 * It returns new message no, offset if FIFO and size.
 */
static int get_size_of_new_msg(struct t_message_queue *q)
{
	struct t_queue_element *new_msg = NULL;
	struct list_head *msg_list;

	dbgprintk("++\n");

	spin_lock_bh(&q->update_lock);
	list_for_each(msg_list, &q->msg_list) {
		new_msg = list_entry(msg_list, struct t_queue_element, entry);
		if (new_msg == NULL) {
			spin_unlock_bh(&q->update_lock);
			printk(KERN_ALERT NAME ":no message found\n");
			return -1;
		}
		break;
	}
	spin_unlock_bh(&q->update_lock);

	dbgprintk("--\n");
	return new_msg->size;
}

/**
 * isi_receive() - Rx Completion callback
 *
 * @p_data:message pointer
 * @n_bytes:message size
 *
 * This function is a callback to indicate ISI message reception is complete.
 * It updates Writeptr of the Fifo
 */
static void isi_receive(void *p_data, u32 n_bytes)
{
	u32 size = 0;
	unsigned char *psrc;
	struct t_message_queue *q;
	struct t_isadev_context *isidev = p_isa_context->p_isadev[0];

	dbgprintk("++\n");
	q = &isidev->dl_queue;
	spin_lock(&q->update_lock);
	/*Memcopy RX data first*/
	if ((q->writeptr+n_bytes) >= q->size) {
		dbgprintk("Inside Loop Back\n");
		psrc = (unsigned char *)p_data;
		size = (q->size-q->writeptr);
		/*Copy First Part of msg*/
		memcpy((q->p_fifo_base+q->writeptr), psrc, size);
		psrc += size;
		/*Copy Second Part of msg at the top of fifo*/
		memcpy(q->p_fifo_base, psrc, (n_bytes-size));
	} else {
		memcpy((q->p_fifo_base+q->writeptr), p_data, n_bytes);
	}
	add_msg_to_queue(q, n_bytes);
	spin_unlock(&q->update_lock);
	dbgprintk("--\n");
}

/**
 * rpc_receive() - Rx Completion callback
 *
 * @p_data:message pointer
 * @n_bytes:message size
 *
 * This function is a callback to indicate RPC message reception is complete.
 * It updates Writeptr of the Fifo
 */
static void rpc_receive(void *p_data, u32 n_bytes)
{
	u32 size = 0;
	unsigned char *psrc;
	struct t_message_queue *q;
	struct t_isadev_context *rpcdev = p_isa_context->p_isadev[1];

	dbgprintk("++\n");
	q = &rpcdev->dl_queue;
	spin_lock(&q->update_lock);
	/*Memcopy RX data first*/
	if ((q->writeptr+n_bytes) >= q->size) {
		psrc = (unsigned char *)p_data;
		size = (q->size-q->writeptr);
		/*Copy First Part of msg*/
		memcpy((q->p_fifo_base+q->writeptr), psrc, size);
		psrc += size;
		/*Copy Second Part of msg at the top of fifo*/
		memcpy(q->p_fifo_base, psrc, (n_bytes-size));
	} else {
		memcpy((q->p_fifo_base+q->writeptr), p_data, n_bytes);
	}

	add_msg_to_queue(q, n_bytes);
	spin_unlock(&q->update_lock);
	dbgprintk("--\n");
}

/**
 * audio_receive() - Rx Completion callback
 *
 * @p_data:message pointer
 * @n_bytes:message size
 *
 * This function is a callback to indicate audio message reception is complete.
 * It updates Writeptr of the Fifo
 */
static void audio_receive(void *p_data, u32 n_bytes)
{
	u32 size = 0;
	unsigned char *psrc;
	struct t_message_queue *q;
	struct t_isadev_context *audiodev = p_isa_context->p_isadev[2];

	dbgprintk("++\n");
	q = &audiodev->dl_queue;
	spin_lock(&q->update_lock);
	/*Memcopy RX data first*/
	if ((q->writeptr+n_bytes) >= q->size) {
		psrc = (unsigned char *)p_data;
		size = (q->size-q->writeptr);
		/*Copy First Part of msg*/
		memcpy((q->p_fifo_base+q->writeptr), psrc, size);
		psrc += size;
		/*Copy Second Part of msg at the top of fifo*/
		memcpy(q->p_fifo_base, psrc, (n_bytes-size));
	} else {
		memcpy((q->p_fifo_base+q->writeptr), p_data, n_bytes);
	}
	add_msg_to_queue(q, n_bytes);
	spin_unlock(&q->update_lock);
	dbgprintk("--\n");
}


/**
 * security_receive() - Rx Completion callback
 *
 * @p_data:message pointer
 * @n_bytes: message size
 *
 * This function is a callback to indicate security message reception
 * is complete.It updates Writeptr of the Fifo
 */
static void security_receive(void *p_data, u32 n_bytes)
{
	u32 size = 0;
	unsigned char *psrc;
	struct t_message_queue *q;
	struct t_isadev_context *secdev = p_isa_context->p_isadev[3];

	dbgprintk("++\n");
	q = &secdev->dl_queue;
	spin_lock(&q->update_lock);
	/*Memcopy RX data first*/
	if ((q->writeptr+n_bytes) >= q->size) {
		psrc = (unsigned char *)p_data;
		size = (q->size-q->writeptr);
		/*Copy First Part of msg*/
		memcpy((q->p_fifo_base+q->writeptr), psrc, size);
		psrc += size;
		/*Copy Second Part of msg at the top of fifo*/
		memcpy(q->p_fifo_base, psrc, (n_bytes-size));
	} else {
		memcpy((q->p_fifo_base+q->writeptr), p_data, n_bytes);
	}
	add_msg_to_queue(q, n_bytes);
	spin_unlock(&q->update_lock);
	dbgprintk("--\n");
}


/**
 * isa_select() - Select Interface
 *
 * @filp:file descriptor pointer
 * @wait:poll_table_struct pointer
 *
 * This function is used to perform non-blocking read operations. It allows
 * a process to determine whether it can read from one or more open files
 * without blocking. These calls can also block a process until any of a
 * given set of file descriptors becomes available for reading.
 * If a file is ready to read, POLLIN | POLLRDNORM bitmask is returned.
 * The driver method is called whenever the user-space program performs a select
 * system call involving a file descriptor associated with the driver.
 */
static unsigned int isa_select(struct file *filp, \
				struct poll_table_struct *wait)
{
	struct t_isadev_context *p_isadev;
	struct t_message_queue *q;
	unsigned int mask = 0;
	u32	m = iminor(filp->f_path.dentry->d_inode);
	dbg_printk("isa_select++\n");
	p_isadev = (struct t_isadev_context *)filp->private_data;

	if (p_isadev->device_id != m)
			return -1;

	q = &p_isadev->dl_queue;

	poll_wait(filp, &q->wq_readable, wait);

	if (atomic_read(&q->q_rp) == 1)
		mask = POLLIN | POLLRDNORM;

	dbg_printk("isa_select--\n");
	return mask;
}

/**
 * isa_read() - Read from device
 *
 * @filp:file descriptor
 * @buf:user buffer pointer
 * @len:size of requested data transfer
 * @ppos:not used
 *
 * This function is called whenever user calls read() system call.
 * It reads a oldest message from queue and copies it into user buffer and
 * returns its size.
 * If there is no message present in queue, then it blocks until new data is
 * available.
 */
static ssize_t isa_read(struct file *filp, char __user *buf,
				size_t len, loff_t *ppos)
{
	u32 size = 0;
	char *psrc;
	struct t_isadev_context *p_isadev;
	struct t_message_queue *q;
	u32 msgsize;

	dbgprintk("++\n");

	if (len <= 0)
		return -EFAULT;

	p_isadev = (struct t_isadev_context *)filp->private_data;
	q = &p_isadev->dl_queue;

	spin_lock_bh(&q->update_lock);
	if (list_empty(&q->msg_list)) {
		spin_unlock_bh(&q->update_lock);

		if (wait_event_interruptible(q->wq_readable, \
				atomic_read(&q->q_rp) == 1)) {
			return -ERESTARTSYS;
		}

	} else
		spin_unlock_bh(&q->update_lock);

	msgsize = get_size_of_new_msg(q);

	if ((q->readptr+msgsize) >= q->size) {
		dbgprintk("Inside Loop Back\n");

		psrc = (char *)buf;
		size = (q->size-q->readptr);
		/*Copy First Part of msg*/
		if (copy_to_user(psrc, \
			(unsigned char *)(q->p_fifo_base+q->readptr), size)) {
			printk(KERN_ALERT NAME ":copy_to_user failed\n");
			return -EFAULT;
		}
		psrc += size;
		/*Copy Second Part of msg at the top of fifo*/
		if (copy_to_user(psrc, \
			(unsigned char *)(q->p_fifo_base), (msgsize-size))) {
			printk(KERN_ALERT NAME ":copy_to_user failed\n");
			return -EFAULT;
		}
	} else {
		if (copy_to_user(buf, \
		(unsigned char *)(q->p_fifo_base+q->readptr), msgsize)) {
			printk(KERN_ALERT NAME ":copy_to_user failed\n");
			return -EFAULT;
		}
	}


	spin_lock_bh(&q->update_lock);
	remove_msg_from_queue(q);
	spin_unlock_bh(&q->update_lock);
	dbgprintk("--\n");
	return msgsize;
}
/**
 * isa_write() - Write to device
 *
 * @filp:file descriptor
 * @buf:user buffer pointer
 * @len:size of requested data transfer
 * @ppos:not used
 *
 * This function is called whenever user calls write() system call.
 * It checks if there is space available in queue, and copies the message
 * inside queue. If there is no space, it blocks until space becomes available.
 * It also schedules transfer thread to transmit the newly added message.
 */
static ssize_t isa_write(struct file *filp, const char __user *buf,
				 size_t len, loff_t *ppos)
{
	struct t_isadev_context *p_isadev;
	struct t_message_queue *q;
	void *addr = 0;
	unsigned char l2_header = 0;

	dbgprintk("++\n");
	if (len <= 0)
		return -EFAULT;

	p_isadev = (struct t_isadev_context *)filp->private_data;
	q = &p_isadev->dl_queue;
	down(&q->tx_mutex);

	switch (p_isadev->device_id) {
	case 0:
		dbg_printk("ISI \n");
		addr = (void *)wr_isi_msg;
#ifdef CONFIG_U8500_SHRM_LOOP_BACK
		printk(KERN_INFO "Loopback \n");
		l2_header = 0xC0;
#else
		l2_header = p_isadev->device_id;
#endif
		break;
	case 1:
		dbg_printk("RPC \n");
		addr = (void *)wr_rpc_msg;
#ifdef CONFIG_U8500_SHRM_LOOP_BACK
		l2_header = 0xC0;
#else
		l2_header = p_isadev->device_id;
#endif
		break;
	case 2:
		dbg_printk("Audio \n");
		addr = (void *)wr_audio_msg;
#ifdef CONFIG_U8500_SHRM_LOOP_BACK
		l2_header = 0x80;
#else
		l2_header = p_isadev->device_id;
#endif

		break;
	case 3:
		dbg_printk("Security \n");
		addr = (void *)wr_sec_msg;
#ifdef CONFIG_U8500_SHRM_LOOP_BACK
		l2_header = 0xC0;
#else
		l2_header = p_isadev->device_id;
#endif
		break;
	default:
		dbgprintk("Wrong device \n");
		break;
	}

	if (copy_from_user(addr, buf, len)) {
		printk(KERN_ALERT NAME ":copy_from_user failed\n");
		return -EFAULT;
	}

	/*Write msg to Fifo*/
	 shm_write_msg(l2_header, addr, len);

	 /*hrtimer_start(&timer, ktime_set(0, 2*NSEC_PER_MSEC),
					HRTIMER_MODE_REL);*/

	up(&q->tx_mutex);
	dbgprintk("--\n");
	return len;
}

/**
 * isa_ioctl() - To handle different ioctl commands supported by driver.
 *
 * @inode: structure is used by the kernel internally to represent files
 * @filp:file descriptor pointer
 * @cmd:ioctl command
 * @arg:input param
 *
 * Following ioctls are supported by this driver.
 * DLP_IOCTL_ALLOCATE_BUFFER - To allocate buffer for new uplink message.
 * This ioctl is called with required message size. It returns offset for
 * the allocates space in the queue. DLP_IOCTL_PUT_MESSAGE -  To indicate
 * new uplink message available in queuq for  transmission. Message is copied
 * from offset location returned by previous ioctl before calling this ioctl.
 * DLP_IOCTL_GET_MESSAGE - To check if any downlink message is available in
 * queue. It returns  offset for new message inside queue.
 * DLP_IOCTL_DEALLOCATE_BUFFER - To deallocate any buffer allocate for
 * downlink message once the message is copied. Message is copied from offset
 * location returned by previous ioctl before calling this ioctl.
 */
static int isa_ioctl(struct inode *inode, struct file *filp,
				unsigned cmd, unsigned long arg)
{
	int err = 0;
	struct t_isadev_context *p_isadev;
	u32 m = iminor(inode);
	p_isadev = (struct t_isadev_context *)filp->private_data;

	if (p_isadev->device_id != m)
		return -1;

	switch (cmd) {
	case DLP_IOC_ALLOCATE_BUFFER:
		dbg_printk(" DLP_IOC_ALLOCATE_BUFFER++\n");
	break;
	case DLP_IOC_PUT_MESSAGE:
		dbg_printk(" DLP_IOC_PUT_MESSAGE++\n");
	break;

	case DLP_IOC_GET_MESSAGE:
		dbg_printk(" DLP_IOC_GET_MESSAGE++\n");
	break;
	case DLP_IOC_DEALLOCATE_BUFFER:
		dbg_printk(" DLP_IOC_DEALLOCATE_BUFFER++\n");
	break;
	default:
		dbg_printk("Unknown IOCTL\n");
		err = -1;
	break;
	}
	return err;
}
/**
 * isa_mmap() - Maps kernel queue memory to user space.
 *
 * @filp:file descriptor pointer
 * @vma:virtual area memory structure.
 *
 * This function maps kernel FIFO into user space. This function
 * shall be called twice to map both uplink and downlink buffers.
 */
static int isa_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct t_isadev_context *p_isadev;
	int err = 0;
	u32	m = iminor(filp->f_path.dentry->d_inode);
	dbg_printk(" isa_mmap %d++\n", m);

	p_isadev = (struct t_isadev_context *)filp->private_data;
	dbg_printk(" isa_mmap--\n");
	return err;
}

/**
 * isa_close() - Close device file
 *
 * @inode:structure is used by the kernel internally to represent files
 * @filp:device file descriptor
 *
 * This function deletes structues associated with this file, deletes
 * queues, flushes and destroys workqueus and closes this file.
 * It also unregisters itself from l2mux driver.
 */
static int isa_close(struct inode *inode, struct file *filp)
{
	struct t_isadev_context *p_isadev;
	u8 m;
	mutex_lock(&isa_lock);
	m = iminor(filp->f_path.dentry->d_inode);
	dbg_printk("isa_close %d", m);

	if (atomic_dec_and_test(&p_isa_context->is_open[m])) {
		atomic_inc(&p_isa_context->is_open[m]);
		printk(KERN_ALERT NAME ":Device not opened yet\n");
		mutex_unlock(&isa_lock);
		return -ENODEV;
	}
	atomic_set(&p_isa_context->is_open[m], 1);

	p_isadev = (struct t_isadev_context *)filp->private_data;
	dbg_printk("p_isadev->device_id %d", p_isadev->device_id);

	dbg_printk(" Closed %d device\n", m);

	if (m == ISI_MESSAGING)
		printk(KERN_ALERT "Closed ISI_MESSAGING Device\n");
	else if (m == RPC_MESSAGING)
		printk(KERN_ALERT "Closed RPC_MESSAGING Device\n");
	else if (m == AUDIO_MESSAGING)
		printk(KERN_ALERT "Closed AUDIO_MESSAGING Device\n");
	else if (m == SECURITY_MESSAGING)
		printk(KERN_ALERT "Closed SECURITY_MESSAGING Device\n");
	else
		printk(KERN_ALERT NAME ":No such device present\n");

	mutex_unlock(&isa_lock);
	return 0;
}
/**
 * isa_open() -  Open device file
 *
 * @inode: structure is used by the kernel internally to represent files
 * @filp: device file descriptor
 *
 * This function performs initialization tasks needed to open SHM channel.
 * Following tasks are performed.
 * -return if device is already opened
 * -create uplink FIFO
 * -create downlink FIFO
 * -init delayed workqueue thread
 * -register to l2mux driver
 */
static int isa_open(struct inode *inode, struct file *filp)
{
	int err = 0;
	u8 m;
	struct t_isadev_context *p_isadev;
	dbgprintk("++\n");

	if (get_boot_state() != BOOT_DONE) {
		printk(KERN_ALERT "Boot is not done \n");
		return -EBUSY;
	}
	mutex_lock(&isa_lock);
	m = iminor(inode);

	if ((m != ISI_MESSAGING) && (m != RPC_MESSAGING) && \
		(m != AUDIO_MESSAGING) && (m != SECURITY_MESSAGING)) {
		printk(KERN_ALERT NAME ":No such device present\n");
		mutex_unlock(&isa_lock);
		return -ENODEV;
	}
	if (!atomic_dec_and_test(&p_isa_context->is_open[m])) {
		atomic_inc(&p_isa_context->is_open[m]);
		printk(KERN_ALERT NAME ":Device already opened\n");
		mutex_unlock(&isa_lock);
		return -EBUSY;
	}

	if (m == ISI_MESSAGING)
		printk(KERN_ALERT "Open ISI_MESSAGING Device\n");
	else if (m == RPC_MESSAGING)
		printk(KERN_ALERT "Open RPC_MESSAGING Device\n");
	else if (m == AUDIO_MESSAGING)
		printk(KERN_ALERT "Open AUDIO_MESSAGING Device\n");
	else if (m == SECURITY_MESSAGING)
		printk(KERN_ALERT "Open SECURITY_MESSAGING Device\n");
	else
		printk(KERN_ALERT NAME ":No such device present\n");

	p_isadev = p_isa_context->p_isadev[m];
	filp->private_data = p_isadev;

	mutex_unlock(&isa_lock);
	dbgprintk("--\n");
	return err;
}


/*
 * struct shm_driver: SHRM platform structure
 * @owner:	The probe funtion to be called
 * @open:	The remove funtion to be called
 * @release:	The driver data
 * @ioctl:	The probe funtion to be called
 * @mmap:	The remove funtion to be called
 * @read:	The driver data
 * @write:	The remove funtion to be called
 * @poll:	The driver data
 */
const struct file_operations isa_fops = {
	.owner	= THIS_MODULE,
	.open 	= isa_open,
	.release = isa_close,
	.ioctl 	= isa_ioctl,
	.mmap 	= isa_mmap,
	.read 	= isa_read,
	.write 	= isa_write,
	.poll	= isa_select,
};

/**
 * isa_init() - module insertion function
 *
 * This function registers module as a character driver using
 * register_chrdev_region() or alloc_chrdev_region. It adds this
 * driver to system using cdev_add() call. Major number is dynamically
 * allocated using alloc_chrdev_region() by default or left to user to specify
 * it during load time. For this variable major is used as module_param
 * Nodes to be created using
 * mknod /dev/isi c $major 0
 * mknod /dev/rpc c $major 1
 * mknod /dev/audio c $major 2
 * mknod /dev/sec c $major 3
 */
static int isa_init(void)
{
	dev_t	dev_id;
	int	retval, no_dev;
	struct t_isadev_context *p_isadev;

	p_isa_context = kzalloc(sizeof(struct t_isa_driver_context), \
							GFP_KERNEL);
	if (p_isa_context == NULL) {
		printk(KERN_ALERT NAME ":Failed to alloc memory\n");
		return -ENOMEM;
	}

	if (major) {
		dev_id = MKDEV(major, 0);
		retval = register_chrdev_region(dev_id, ISA_DEVICES, NAME);
	} else {
		retval = alloc_chrdev_region(&dev_id, 0, ISA_DEVICES, NAME);
		major = MAJOR(dev_id);
	}

	printk(KERN_ALERT NAME ": major %d\n", major);

	cdev_init(&p_isa_context->cdev, &isa_fops);
	p_isa_context->cdev.owner = THIS_MODULE;
	retval = cdev_add(&p_isa_context->cdev, dev_id, ISA_DEVICES);
	if (retval) {
		printk(KERN_ALERT NAME ":Failed to add char device\n");
		return retval;
	}

	for (no_dev = 0; no_dev < ISA_DEVICES; no_dev++)
		atomic_set(&p_isa_context->is_open[no_dev], 1);

	for (no_dev = 0; no_dev < ISA_DEVICES; no_dev++) {

		p_isadev = kzalloc(sizeof(struct t_isadev_context), GFP_KERNEL);
		if (p_isadev == NULL) {
			printk(KERN_ALERT NAME ":Failed to alloc memory\n");
			return -ENOMEM;
		}

		p_isadev->device_id = no_dev;

		retval = create_queue(&p_isadev->dl_queue, p_isadev->device_id);
		if (retval < 0) {
			printk(KERN_ALERT NAME ":create dl_queue failed\n");
			delete_queue(&p_isadev->dl_queue);
			kfree(p_isadev);
			return retval;
		}
		p_isa_context->p_isadev[p_isadev->device_id] = p_isadev;
		sema_init(&p_isadev->tx_mutex, 1);

	}

	printk(KERN_ALERT NAME ": Driver added\n");

	return retval;
}
/**
 * isa_exit() - module removal function
 *
 * This function removes this driver from system.
 */
static void isa_exit(void)
{
	int	no_dev;
	struct t_isadev_context *p_isadev;
	dev_t dev_id = MKDEV(major, 0);

	for (no_dev = 0; no_dev < ISA_DEVICES; no_dev++) {
		p_isadev = p_isa_context->p_isadev[no_dev];
		delete_queue(&p_isadev->dl_queue);
		kfree(p_isadev);
	}

	cdev_del(&p_isa_context->cdev);
	unregister_chrdev_region(dev_id, ISA_DEVICES);
	kfree(p_isa_context);

	printk(KERN_ALERT NAME ": Driver removed\n");
}

#ifdef CONFIG_HIGH_RES_TIMERS
static enum hrtimer_restart callback(struct hrtimer *timer)
{
	return HRTIMER_NORESTART;
}
#endif


static int __init shrm_probe(struct platform_device *pdev)
{
	int err = 0;
	struct resource *res;
	struct shrm_dev *shm_dev_data = NULL;
	unsigned int *p_edge;

	if (pdev == NULL)  {
		printk(KERN_ALERT "No device/platform_data found on shm device\n");
		return -ENODEV;
	}


	shm_dev_data = kzalloc(sizeof(struct shrm_dev), GFP_KERNEL);

	pshm_dev = shm_dev_data;
	if (shm_dev_data == NULL) {
		printk(KERN_ALERT "Could not allocate memory for struct shm_dev\n");
		return -ENOMEM;
	}

	/** initialise the SHM */

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		printk(KERN_ALERT "Unable to map Ca Wake up interrupt \n");
		err = -EBUSY;
		goto rollback_intr;
	}
	shm_dev_data->ca_wake_irq = res->start;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	if (!res) {
		printk(KERN_ALERT "Unable to map APE_Read_notif_common IRQ base \n");
		err = -EBUSY;
		goto rollback_intr;
	}
	shm_dev_data->ac_read_notif_0_irq = res->start;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 2);
	if (!res) {
		printk(KERN_ALERT "Unable to map APE_Read_notif_audio IRQ base \n");
		err = -EBUSY;
		goto rollback_intr;
	}
	shm_dev_data->ac_read_notif_1_irq = res->start;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 3);
	if (!res) {
		printk(KERN_ALERT "Unable to map Cmt_msg_pending_notif_common IRQ base \n");
		err = -EBUSY;
		goto rollback_intr;
	}
	shm_dev_data->ca_msg_pending_notif_0_irq = res->start;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 4);
	if (!res) {
		printk(KERN_ALERT "Unable to map Cmt_msg_pending_notif_audio IRQ base \n");
		err = -EBUSY;
		goto rollback_intr;
	}
	shm_dev_data->ca_msg_pending_notif_1_irq = res->start;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		printk(KERN_ALERT "Could not get SHM IO memory information\n");
		err = -ENODEV;
		goto rollback_intr;
	}

	shm_dev_data->intr_base = (void __iomem *)ioremap_nocache(res->start, \
					res->end - res->start + 1);

	if (!(shm_dev_data->intr_base)) {
		printk(KERN_ALERT "Unable to map register base \n");
		err = -EBUSY;
		goto rollback_intr;
	}

	shm_dev_data->ape_common_fifo_base_phy = \
			(unsigned int *)U8500_SHM_FIFO_APE_COMMON_BASE;
	shm_dev_data->ape_common_fifo_base = \
		(void __iomem *)ioremap_nocache( \
					U8500_SHM_FIFO_APE_COMMON_BASE, \
					SHM_FIFO_0_SIZE);
	shm_dev_data->ape_common_fifo_size = (SHM_FIFO_0_SIZE)/4;

	if (!(shm_dev_data->ape_common_fifo_base)) {
		printk(KERN_ALERT "Unable to map register base \n");
		err = -EBUSY;
		goto rollback_ape_common_fifo_base;
	}

	shm_dev_data->cmt_common_fifo_base_phy = \
		(unsigned int *)U8500_SHM_FIFO_CMT_COMMON_BASE;

	shm_dev_data->cmt_common_fifo_base = \
		(void __iomem *)ioremap_nocache( \
			U8500_SHM_FIFO_CMT_COMMON_BASE, SHM_FIFO_0_SIZE);
	shm_dev_data->cmt_common_fifo_size = (SHM_FIFO_0_SIZE)/4;

	if (!(shm_dev_data->cmt_common_fifo_base)) {
		printk(KERN_ALERT "Unable to map register base \n");
		err = -EBUSY;
		goto rollback_cmt_common_fifo_base;
	}

	shm_dev_data->ape_audio_fifo_base_phy = \
			(unsigned int *)U8500_SHM_FIFO_APE_AUDIO_BASE;
	shm_dev_data->ape_audio_fifo_base = \
		(void __iomem *)ioremap_nocache(U8500_SHM_FIFO_APE_AUDIO_BASE, \
							SHM_FIFO_1_SIZE);
	shm_dev_data->ape_audio_fifo_size = (SHM_FIFO_1_SIZE)/4;

	if (!(shm_dev_data->ape_audio_fifo_base)) {
		printk(KERN_ALERT "Unable to map register base \n");
		err = -EBUSY;
		goto rollback_ape_audio_fifo_base;
	}

	shm_dev_data->cmt_audio_fifo_base_phy = \
			(unsigned int *)U8500_SHM_FIFO_CMT_AUDIO_BASE;
	shm_dev_data->cmt_audio_fifo_base = \
		(void __iomem *)ioremap_nocache(U8500_SHM_FIFO_CMT_AUDIO_BASE, \
							SHM_FIFO_1_SIZE);
	shm_dev_data->cmt_audio_fifo_size = (SHM_FIFO_1_SIZE)/4;

	if (!(shm_dev_data->cmt_audio_fifo_base)) {
		printk(KERN_ALERT "Unable to map register base \n");
		err = -EBUSY;
		goto rollback_cmt_audio_fifo_base;
	}

	shm_dev_data->ac_common_shared_wptr = \
		(void __iomem *)ioremap(SHM_ACFIFO_0_WRITE_AMCU, SHM_PTR_SIZE);

	if (!(shm_dev_data->ac_common_shared_wptr)) {
		printk(KERN_ALERT "Unable to map register base \n");
		err = -EBUSY;
		goto rollback_ac_common_shared_wptr;
	}

	shm_dev_data->ac_common_shared_rptr = \
		(void __iomem *)ioremap(SHM_ACFIFO_0_READ_AMCU, SHM_PTR_SIZE);

	if (!(shm_dev_data->ac_common_shared_rptr)) {
		printk(KERN_ALERT "Unable to map register base \n");
		err = -EBUSY;
		goto rollback_map;
	}


	shm_dev_data->ca_common_shared_wptr = \
		(void __iomem *)ioremap(SHM_CAFIFO_0_WRITE_AMCU, SHM_PTR_SIZE);

	if (!(shm_dev_data->ca_common_shared_wptr)) {
		printk(KERN_ALERT "Unable to map register base \n");
		err = -EBUSY;
		goto rollback_map;
	}

	shm_dev_data->ca_common_shared_rptr = \
		(void __iomem *)ioremap(SHM_CAFIFO_0_READ_AMCU, SHM_PTR_SIZE);

	if (!(shm_dev_data->ca_common_shared_rptr)) {
		printk(KERN_ALERT "Unable to map register base \n");
		err = -EBUSY;
		goto rollback_map;
	}


	shm_dev_data->ac_audio_shared_wptr = \
		(void __iomem *)ioremap(SHM_ACFIFO_1_WRITE_AMCU, SHM_PTR_SIZE);

	if (!(shm_dev_data->ac_audio_shared_wptr)) {
		printk(KERN_ALERT "Unable to map register base \n");
		err = -EBUSY;
		goto rollback_map;
	}


	shm_dev_data->ac_audio_shared_rptr = \
		(void __iomem *)ioremap(SHM_ACFIFO_1_READ_AMCU, SHM_PTR_SIZE);

	if (!(shm_dev_data->ac_audio_shared_rptr)) {
		printk(KERN_ALERT "Unable to map register base \n");
		err = -EBUSY;
		goto rollback_map;
	}


	shm_dev_data->ca_audio_shared_wptr = \
		(void __iomem *)ioremap(SHM_CAFIFO_1_WRITE_AMCU, SHM_PTR_SIZE);

	if (!(shm_dev_data->ca_audio_shared_wptr)) {
		printk(KERN_ALERT "Unable to map register base \n");
		err = -EBUSY;
		goto rollback_map;
	}


	shm_dev_data->ca_audio_shared_rptr = \
		(void __iomem *)ioremap(SHM_CAFIFO_1_READ_AMCU, SHM_PTR_SIZE);

	if (!(shm_dev_data->ca_audio_shared_rptr)) {
		printk(KERN_ALERT "Unable to map register base \n");
		err = -EBUSY;
		goto rollback_map;
	}


	if (isa_init() != 0) {
		printk(KERN_ALERT "Driver Initialization Error \n");
		err = -EBUSY;
	}
	/* install handlers and tasklets */
	if (shm_initialise_irq(shm_dev_data)) {
		printk(KERN_ALERT "shm error in interrupt registration \n");
		goto rollback_irq;
	}

#ifdef CONFIG_HIGH_RES_TIMERS
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = callback;

	hrtimer_start(&timer, ktime_set(0, 2*NSEC_PER_MSEC), HRTIMER_MODE_REL);
#endif

	return err;

rollback_irq:
	free_shm_irq(shm_dev_data);
rollback_map:
	iounmap(pshm_dev->ac_common_shared_wptr);
	iounmap(pshm_dev->ac_common_shared_rptr);
	iounmap(pshm_dev->ca_common_shared_wptr);
	iounmap(pshm_dev->ca_common_shared_rptr);
	iounmap(pshm_dev->ac_audio_shared_wptr);
	iounmap(pshm_dev->ac_audio_shared_rptr);
	iounmap(pshm_dev->ca_audio_shared_wptr);
	iounmap(pshm_dev->ca_audio_shared_rptr);
rollback_ac_common_shared_wptr:
	iounmap(pshm_dev->cmt_audio_fifo_base);
rollback_cmt_audio_fifo_base:
	iounmap(pshm_dev->ape_audio_fifo_base);
rollback_ape_audio_fifo_base:
	iounmap(pshm_dev->cmt_common_fifo_base);
rollback_cmt_common_fifo_base:
	iounmap(pshm_dev->ape_common_fifo_base);
rollback_ape_common_fifo_base:
	iounmap(pshm_dev->intr_base);
rollback_intr:
	kfree(shm_dev_data);
	return err;
}

static int __exit shrm_remove(struct platform_device *pdev)
{

	free_shm_irq(pshm_dev);
	iounmap(pshm_dev->intr_base);
	iounmap(pshm_dev->ape_common_fifo_base);
	iounmap(pshm_dev->cmt_common_fifo_base);
	iounmap(pshm_dev->ape_audio_fifo_base);
	iounmap(pshm_dev->cmt_audio_fifo_base);
	iounmap(pshm_dev->ac_common_shared_wptr);
	iounmap(pshm_dev->ac_common_shared_rptr);
	iounmap(pshm_dev->ca_common_shared_wptr);
	iounmap(pshm_dev->ca_common_shared_rptr);
	iounmap(pshm_dev->ac_audio_shared_wptr);
	iounmap(pshm_dev->ac_audio_shared_rptr);
	iounmap(pshm_dev->ca_audio_shared_wptr);
	iounmap(pshm_dev->ca_audio_shared_rptr);
	kfree(pshm_dev);
	isa_exit();

	return 0;
}

#ifdef CONFIG_PM
/**
 * u8500_shrm_suspend() - This routine puts the SHRM in to sustend state.
 * @pdev: platform device.
 *
 * This routine checks the current ongoing communication with Modem by
 * examining the ca_wake state and prevents suspend if modem communication
 * is on-going.
 * If ca_wake = 1 (high), modem comm. is on-going; don't suspend
 * If ca_wake = 0 (low), no comm. with modem on-going.Allow suspend
 */
int u8500_shrm_suspend(struct platform_device *pdev, pm_message_t state)
{
	dbgprintk("\n u8500_shrm_suspend: called...\n");
	dbgprintk("\n ca_wake_req_state = %x\n", get_ca_wake_req_state());

	/* if ca_wake_req is high, prevent system suspend */
	if (get_ca_wake_req_state())
		return -EBUSY;
	else
		return 0;
}

/**
 * u8500_shrm_resume() - This routine resumes the SHRM from sustend state.
 * @pdev: platform device.
 *
 * This routine restore back the current state of the SHRM
 */
int u8500_shrm_resume(struct platform_device *pdev)
{
	dbgprintk("\n u8500_shrm_resume: called...\n");

	/* TODO:
	 * As of now, no state save takes place in suspend.
	 * So, nothing to restore in resume.
	 * Simply return as of now.
	 * State saved in suspend should be restored here.
	 */

	return 0;
}

#else

#define u8500_shrm_suspend NULL
#define u8500_shrm_resume NULL

#endif



/*
 * struct shrm_driver: SHRM platform structure
 * @probe:	The probe funtion to be called
 * @remove:	The remove funtion to be called
 * @driver:	The driver data
 */

static struct platform_driver shrm_driver = {
	.probe = shrm_probe,
	.remove = __exit_p(shrm_remove),
	.driver = {
		.name = "u8500_shrm",
		.owner = THIS_MODULE,
	},
#ifdef CONFIG_PM
	.suspend = u8500_shrm_suspend,
	.resume = u8500_shrm_resume,
#endif
};

static int __init shrm_driver_init(void)
{
	int err = 0;

	err = platform_driver_probe(&shrm_driver, shrm_probe);
	if (err < 0) {
		printk(KERN_ALERT "SHM Platform  register FAILED: %d\n", err);
		return err;
	}
	return 0;
}

static void __exit shrm_driver_exit(void)
{
	platform_driver_unregister(&shrm_driver);

	printk(KERN_ALERT "SHM Platform DRIVER removed\n");
}

module_init(shrm_driver_init);
module_exit(shrm_driver_exit);

MODULE_AUTHOR("Biju Das");
MODULE_DESCRIPTION("Shared Memory Modem Driver Interface");
MODULE_LICENSE("GPL");
