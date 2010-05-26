/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Linux Platform and OS Dependent File for FM Driver
 *
 * Author: Hemant Gupta/hemant.gupta@stericsson.com for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2
 */

#ifndef PLATFORM_OSAPI_H
#define PLATFORM_OSAPI_H

#include <linux/module.h>	/* Modules                        */
#include <linux/init.h>		/* Initdata                       */
#include <linux/uaccess.h>	/* copy to/from user              */
#include <linux/smp_lock.h>	/* Lock & Unlock Kernel		  */
#include <linux/semaphore.h>	/* Semaphores */
#include <linux/version.h>      /* for KERNEL_VERSION MACRO     */
#include <linux/kthread.h>      /* for Kernel Thread  */
#include <linux/kernel.h>      /* for KERNEL */
#include <linux/timer.h>
#include <linux/mutex.h>
#include "stefmapi.h"

/* module_param declared in platformosapi.c */
extern unsigned short ste_fm_debug_level;

#define FM_DEBUG_REPORT(fmt, arg...) \
	if (ste_fm_debug_level > 2) { \
		printk(KERN_INFO "CG2900_FM_Driver: " fmt "\r\n" , ## arg); \
	}

#define FM_INFO_REPORT(fmt, arg...)    \
	if (ste_fm_debug_level > 1) { \
		printk(KERN_INFO "CG2900_FM_Driver: " fmt "\r\n" , ## arg); \
	}

#define FM_ERR_REPORT(fmt, arg...)               \
	if (ste_fm_debug_level > 0) { \
		printk(KERN_ERR "CG2900_FM_Driver: " fmt "\r\n" , ## arg); \
	}

/**
 * os_fm_driver_init()- Initializes the Mutex, Semaphore, etc for FM Driver.
 *
 * It also registes FM Driver with the Protocol Driver.
 *
 * Returns:
 *   1 if initialization fails, else 0 on success.
 */
int os_fm_driver_init(void);

/**
 * os_fm_driver_deinit() - Deinitializes the mutex, semaphores, etc.
 *
 * It also deregistes FM Driver with the Protocol Driver.
 *
 */
void os_fm_driver_deinit(void);


/**
 * os_mem_alloc() - Allocates requested bytes to a pointer.
 * @num_bytes: Number of bytes to be allocated.
 *
 * Returns:
 *   Pointer pointing to the memory allocated.
 */
void *os_mem_alloc(
		u32 num_bytes
		);

/**
 * os_mem_free() - Frees the memory allocated to a pointer
 * @free_ptr: Pointer to the Memory to be freed.
 */
void os_mem_free(
		void *free_ptr
		);

/**
 * os_mem_copy() - Copy memory of fixed bytes from Source to Destination.
 * @dest: Memory location where the data has to be copied.
 * @src: Memory location from where the data hasto be copied.
 * @num_bytes: Number of bytes to be copied from Source pointer to
 * Destination pointer.
 */
void os_mem_copy(
		void *dest,
		void *src,
		u32 num_bytes
		);

/**
 * os_sleep() - Suspends the current task for a specified time
 * @milli_sec: Time in ms to suspend the task
 */
void os_sleep(
		u32 milli_sec
		);

/**
 * os_lock() - Enter Critical Section
 */
void os_lock(void);

/**
 * os_unlock() - Leave Critical Section
 */
void os_unlock(void);

/**
 * os_get_cmd_sem() - Block on Command Semaphore.
 * This is required to ensure Flow Control in FM Driver.
 */
void os_get_cmd_sem(void);

/**
 * os_set_cmd_sem() - Unblock on Command Semaphore.
 * This is required to ensure Flow Control in FM Driver.
 */
void os_set_cmd_sem(void);

/**
 * os_get_hal_sem() - Block on HAL Semaphore.
 * This is required to ensure Flow Control in FM Driver.
 */
void os_get_hal_sem(void);

/**
 * os_set_hal_sem() - Unblock on HAL Semaphore.
 * This is required to ensure Flow Control in FM Driver.
 */
void os_set_hal_sem(void);

/**
 * os_get_read_response_sem() - Block on fmd_read_resp Semaphore.
 * This semaphore is blocked till the command complete event is
 * received from FM Chip for the last command sent.
 */
void os_get_read_response_sem(void);

/**
 * os_set_read_response_sem() - Unblock on fmd_read_resp Semaphore.
 * This semaphore is unblocked on receiving command complete event
 * from FM Chip for the last command sent.
 */
void os_set_read_response_sem(void);

/**
 * os_get_rds_sem() - Block on RDS Semaphore.
 * Till irpt_BufferFull is received, RDS Task is blocked.
 */
void os_get_rds_sem(void);

/**
 * os_set_rds_sem() - Unblock on RDS Semaphore.
 * on receiving  irpt_BufferFull, RDS Task is un-blocked.
 */
void os_set_rds_sem(void);

/**
 * os_get_interrupt_sem() - Block on Interrupt Semaphore.
 * Till Interrupt is received, Interrupt Task is blocked.
 */
void os_get_interrupt_sem(void);

/**
 * os_set_interrupt_sem() - Unblock on Interrupt Semaphore.
 * on receiving  Interrupt, Interrupt Task is un-blocked.
 */
void os_set_interrupt_sem(void);

/**
 * os_start_rds_thread() - Starts the RDS Thread for receiving RDS Data.
 * This is started by Application when it wants to receive RDS Data.
 * @cb_func: Callback function for receiving RDS Data
 */
void os_start_rds_thread(
		ste_fm_rds_cb cb_func
		);

/**
 * os_stop_rds_thread() - Stops the RDS Thread when Application does not
 * want to receive RDS.
 */
void os_stop_rds_thread(void);

/**
 * ste_fm_send_packet() - Sends the FM HCI Packet to the STE Protocol Driver.
 * @num_bytes: Number of bytes of Data to be sent including
 * Channel Identifier (08)
 * @send_buffer: Buffer containing the Data to be sent to Chip.
 *
 * Returns:
 * 0 if packet was sent successfully to STE Protocol Driver, otherwise the
 * corresponding error.
 */
int ste_fm_send_packet(
		u16 num_bytes,
		u8 *send_buffer
		);

#endif /* PLATFORM_OSAPI_H */

