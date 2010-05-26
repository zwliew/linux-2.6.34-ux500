/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Linux Platform and OS Dependent File for FM Driver
 *
 * Author: Hemant Gupta/hemant.gupta@stericsson.com for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2
 */

#include "platformosapi.h"
#include "fmdriver.h"

/* STE Connectivity Driver */
#include <linux/mfd/ste_conn.h>
#include <mach/ste_conn_devices.h>
#include "ste_conn_cpd.h"
#include "ste_conn_debug.h"

#include <linux/module.h>
#include <linux/skbuff.h>

/* ------------------ Internal function declarations ---------------------- */
static void ste_fm_read_cb(struct ste_conn_device *dev, struct sk_buff *skb);
static void ste_fm_reset_cb(struct ste_conn_device *dev);
static int ste_fm_rds_thread(void *data);
static void os_start_irq_thread(void);
static void os_stop_irq_thread(void);
static int ste_fm_irq_thread(void *data);

/*
 * struct ste_fm_cb - Specifies callback structure for ste_conn user.
 *
 * This type specifies callback structure for ste_conn user.
 *
 * @read_cb:  Callback function called when data is received from the controller
 * @reos_set_cb: Callback function called when the controller has been reset
 */
static struct ste_conn_callbacks ste_fm_cb = {
	.read_cb = ste_fm_read_cb,
	.reset_cb = ste_fm_reset_cb
};

static struct semaphore cmd_sem;
static struct semaphore hal_sem;
static struct semaphore read_rsp_sem;
static struct semaphore rds_sem;
static struct semaphore interrupt_sem;
static struct task_struct *rds_thread_task;
static struct task_struct *irq_thread_task;
static struct ste_conn_device *ste_fm_cmd_evt;
static struct mutex write_mutex;
static struct mutex read_mutex;
static spinlock_t fm_spinlock;

/* Debug Level */
/* 1: Only Error Logs */
/* 2: Info Logs */
/* 3: Debug Logs */
/* 4: HCI Logs */
unsigned short ste_fm_debug_level = 1;
EXPORT_SYMBOL(ste_fm_debug_level);

static ste_fm_rds_cb cb_rds_func;
static bool rds_thread_required;
static bool irq_thread_required;
/* ------------------ Internal function definitions ---------------------- */

/**
  * ste_fm_read_cb() - Handle Received Data
  * This function handles data received from connectivity protocol driver.
  * @dev: Device receiving data.
  * @skb: Buffer with data coming form device.
  */
static void ste_fm_read_cb(struct ste_conn_device *dev, struct sk_buff *skb)
{
	mutex_lock(&read_mutex);
	FM_DEBUG_REPORT("ste_fm_read_cb");
	if (skb->data != NULL && skb->len != 0) {
		if (ste_fm_debug_level > 3)
			fmd_hexdump('<', skb->data, skb->len);
		/* The first byte is length of bytes following */
		fmd_receive_data(skb->data[0], &skb->data[1]);
	}
	kfree_skb(skb);
	mutex_unlock(&read_mutex);
}

/**
 * ste_fm_reset_cb() - Reset callback fuction.
 * @dev: CPD device reseting.
 */
static void ste_fm_reset_cb(struct ste_conn_device *dev)
{
	FM_DEBUG_REPORT("ste_fm_reset_cb: Device Reset");
}

/**
 * ste_fm_rds_thread() - Thread for receiving RDS data from Chip.
 * @data: Data beng passed as paramter on starting the thread.
 */
static int ste_fm_rds_thread(void *data)
{
	FM_DEBUG_REPORT("ste_fm_rds_thread Created Successfully");
	while (rds_thread_required) {
		if (cb_rds_func)
			cb_rds_func();
		os_sleep(100);
	}
	FM_DEBUG_REPORT("ste_fm_rds_thread Exiting!!!");
	return 0;
}

/**
 * os_start_irq_thread() - Function for starting Interrupt Thread.
 */
static void os_start_irq_thread(void)
{
	FM_DEBUG_REPORT("os_start_irq_thread");
	irq_thread_task = kthread_create(ste_fm_irq_thread, NULL, "IrqThread");
	if (IS_ERR(irq_thread_task)) {
		FM_ERR_REPORT("os_start_irq_thread: "
			      "Unable to Create IRQThread");
		irq_thread_task = NULL;
	}
	wake_up_process(irq_thread_task);
}

/**
 * os_stop_irq_thread() - Function for stopping Interrupt Thread.
 */
static void os_stop_irq_thread(void)
{
	FM_DEBUG_REPORT("os_stop_irq_thread");
	kthread_stop(irq_thread_task);
	irq_thread_task = NULL;
	FM_DEBUG_REPORT("-os_stop_irq_thread");
}

/**
 * ste_fm_irq_thread() - Thread for processing Interrupts received from Chip.
 * @data: Data beng passed as paramter on starting the thread.
 */
static int ste_fm_irq_thread(void *data)
{
	FM_DEBUG_REPORT("ste_fm_irq_thread Created Successfully");
	while (irq_thread_required) {
		fmd_isr();
		os_sleep(100);
	}
	FM_DEBUG_REPORT("ste_fm_irq_thread Exiting!!!");
	return 0;
}

/* ------------------ External function definitions ---------------------- */

int os_fm_driver_init(void)
{
	int ret_val;

	FM_DEBUG_REPORT("os_fm_driver_init");
	/* Initialize the semaphores */
	sema_init(&cmd_sem, 0);
	sema_init(&hal_sem, 1);
	sema_init(&read_rsp_sem, 0);
	sema_init(&rds_sem, 0);
	sema_init(&interrupt_sem, 0);

	cb_rds_func = NULL;
	rds_thread_required = STE_FALSE;
	irq_thread_required = STE_TRUE;
	ste_fm_cmd_evt = NULL;
	/* Create Mutex For Reading and Writing */
	mutex_init(&read_mutex);
	mutex_init(&write_mutex);
	spin_lock_init(&fm_spinlock);
	os_start_irq_thread();

	/* Register the FM Driver with
	 * Connectivity Protocol Driver */
	ste_fm_cmd_evt =
	    ste_conn_register(STE_CONN_DEVICES_FM_RADIO, &ste_fm_cb);
	if (ste_fm_cmd_evt == NULL) {
		FM_ERR_REPORT("os_fm_driver_init: "
			      "Couldn't register STE_CONN_DEVICES_FM_RADIO to "
			      "CPD. Either chip is not connected or "
			      "Protocol Driver is not initialized");
		ret_val = 1;
	} else {
		FM_INFO_REPORT("os_fm_driver_init: "
			       "Registered with CPD successfully!!!");
		ret_val = 0;
	}
	FM_DEBUG_REPORT("os_fm_driver_init: Returning %d", ret_val);
	return ret_val;
}

void os_fm_driver_deinit(void)
{
	FM_DEBUG_REPORT("os_fm_driver_deinit");
	irq_thread_required = STE_FALSE;
	mutex_destroy(&write_mutex);
	mutex_destroy(&read_mutex);
	os_stop_irq_thread();
	/* Deregister the FM Driver with Connectivity Protocol Driver */
	if (ste_fm_cmd_evt != NULL)
		ste_conn_deregister(ste_fm_cmd_evt);
}

void *os_mem_alloc(u32 num_bytes)
{
	void *alloc_pointer;

	FM_DEBUG_REPORT("os_mem_alloc for %d bytes", num_bytes);

	alloc_pointer = kmalloc(num_bytes, GFP_KERNEL);
	return alloc_pointer;
}

void os_mem_free(void *free_ptr)
{
	FM_DEBUG_REPORT("os_mem_free");
	if (NULL != free_ptr)
		kfree(free_ptr);
}

void os_mem_copy(void *dest, void *src, u32 num_bytes)
{
	FM_DEBUG_REPORT("os_mem_copy");
	memcpy(dest, (const void *)src, num_bytes);
}

void os_sleep(u32 milli_sec)
{
	FM_DEBUG_REPORT("os_sleep");
	schedule_timeout_interruptible(msecs_to_jiffies(milli_sec) + 1);
}

void os_lock(void) __acquires(fm_spinlock)
{
	FM_DEBUG_REPORT("os_lock");
	spin_lock(&fm_spinlock);
}

void os_unlock(void) __releases(fm_spinlock)
{
	FM_DEBUG_REPORT("os_unlock");
	spin_unlock(&fm_spinlock);
}

void os_get_cmd_sem(void)
{
	int ret_val;

	FM_DEBUG_REPORT("os_get_cmd_sem");

	ret_val = down_timeout(&cmd_sem, msecs_to_jiffies(10000) + 1);

	if (ret_val) {
		FM_ERR_REPORT("os_get_cmd_sem: down_timeout "
			      "returned error = %d", ret_val);
	}
}

void os_set_cmd_sem(void)
{
	FM_DEBUG_REPORT("os_set_cmd_sem");

	up(&cmd_sem);
}

void os_get_hal_sem(void)
{
	int ret_val;

	FM_DEBUG_REPORT("os_get_hal_sem");
	ret_val = down_timeout(&hal_sem, msecs_to_jiffies(500) + 1);

	if (ret_val) {
		FM_ERR_REPORT("os_get_hal_sem: down_timeout "
			      "returned error = %d", ret_val);
	}
}

void os_set_hal_sem(void)
{
	FM_DEBUG_REPORT("os_set_hal_sem");
	up(&hal_sem);
}

void os_get_read_response_sem(void)
{
	int ret_val;

	FM_DEBUG_REPORT("os_get_read_response_sem");
	ret_val = down_timeout(&read_rsp_sem, msecs_to_jiffies(500) + 1);

	if (ret_val) {
		FM_ERR_REPORT("os_get_read_response_sem: "
			      "down_timeout returned error = %d", ret_val);
	}
}

void os_set_read_response_sem(void)
{
	FM_DEBUG_REPORT("os_set_read_response_sem");
	up(&read_rsp_sem);
}

void os_get_rds_sem(void)
{
	int ret_val;

	FM_DEBUG_REPORT("os_get_rds_sem");
	ret_val = down_killable(&rds_sem);

	if (ret_val) {
		FM_ERR_REPORT("os_get_rds_sem: down_killable "
			      "returned error = %d", ret_val);
	}
}

void os_set_rds_sem(void)
{
	FM_DEBUG_REPORT("os_set_rds_sem");
	up(&rds_sem);
}

void os_get_interrupt_sem(void)
{
	int ret_val;

	FM_DEBUG_REPORT("os_get_interrupt_sem");

	ret_val = down_killable(&interrupt_sem);

	if (ret_val) {
		FM_ERR_REPORT("os_get_interrupt_sem: down_timeout "
			      "returned error = %d", ret_val);
	}
}

void os_set_interrupt_sem(void)
{
	FM_DEBUG_REPORT("os_set_interrupt_sem");
	up(&interrupt_sem);
}

void os_start_rds_thread(ste_fm_rds_cb cb_func)
{
	FM_DEBUG_REPORT("os_start_rds_thread");
	cb_rds_func = cb_func;
	rds_thread_required = STE_TRUE;
	rds_thread_task = kthread_create(ste_fm_rds_thread, NULL, "RdsThread");
	if (IS_ERR(rds_thread_task)) {
		FM_ERR_REPORT("os_start_rds_thread: "
			      "Unable to Create RdsThread");
		rds_thread_task = NULL;
	}
	wake_up_process(rds_thread_task);
}

void os_stop_rds_thread(void)
{
	FM_DEBUG_REPORT("os_stop_rds_thread");
	/* In case thread is waiting, set the rds sem */
	os_set_rds_sem();
	cb_rds_func = NULL;
	rds_thread_required = STE_FALSE;
	if (rds_thread_task) {
		kthread_stop(rds_thread_task);
		rds_thread_task = NULL;
	}
}

int ste_fm_send_packet(u16 num_bytes, u8 *send_buffer)
{
	int err = 0;
	struct sk_buff *skb;
	mutex_lock(&write_mutex);

	FM_DEBUG_REPORT("ste_fm_send_packet");
	if (ste_fm_debug_level > 3)
		fmd_hexdump('>', send_buffer, num_bytes);

	/* Allocate 1 byte less, since Protocol driver
	 * appends the FM Logical Channel Byte */
	skb = ste_conn_alloc_skb(num_bytes - 1, GFP_KERNEL);
	if (skb) {
		/* Copy the buffer removing the FM Header as this
		 * would be done by Protocol Driver */
		os_mem_copy(skb_put(skb, num_bytes - 1),
			    &send_buffer[1], num_bytes - 1);
	} else {
		FM_DEBUG_REPORT("Couldn't allocate sk_buff "
				"with length %d", num_bytes - 1);
	}
	if (ste_fm_cmd_evt != NULL) {
		err = ste_conn_write(ste_fm_cmd_evt, skb);
		if (err) {
			FM_ERR_REPORT("ste_fm_send_packet: "
				      "Failed to send(%d) bytes using "
				      "ste_conn_write, err = %d",
				      num_bytes - 1, err);
		}
	}

	mutex_unlock(&write_mutex);
	FM_DEBUG_REPORT("ste_fm_send_packet returning %d", err);
	return err;
}

MODULE_AUTHOR("Hemant Gupta");
MODULE_LICENSE("GPL v2");

module_param(ste_fm_debug_level, ushort, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(ste_fm_debug_level, "ste_fm_debug_level: "
		 " *1: Only Error Logs* "
		 " 2: Info Logs " " 3: Debug Logs " " 4: HCI Logs");
