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


#include <mach/shrm.h>
#include "shrm_driver.h"
#include "shrm_private.h"
#include <linux/hrtimer.h>
#include <mach/prcmu-fw-api.h>

static u8 boot_state = BOOT_INIT;
static unsigned char recieve_common_msg[8*1024];
static unsigned char recieve_audio_msg[8*1024];
static received_msg_handler p_common_rx_handler;
static received_msg_handler p_audio_rx_handler;
static struct hrtimer timer;
static char is_earlydrop;
static char shrm_rx_state = SHRM_SLEEP_STATE;
static char shrm_tx_state = SHRM_SLEEP_STATE;

/** Spin lock and tasklet declaration */
DECLARE_TASKLET(shm_ca_0_tasklet, shm_ca_msgpending_0_tasklet, 0);
DECLARE_TASKLET(shm_ca_1_tasklet, shm_ca_msgpending_1_tasklet, 0);
DECLARE_TASKLET(shm_ac_read_0_tasklet, shm_ac_read_notif_0_tasklet, 0);
DECLARE_TASKLET(shm_ac_read_1_tasklet, shm_ac_read_notif_1_tasklet, 0);
DECLARE_TASKLET(shm_ca_wake_tasklet, shm_ca_wake_req_tasklet, 0);

spinlock_t ca_common_lock ;
spinlock_t ca_audio_lock ;

static enum hrtimer_restart callback(struct hrtimer *timer)
{
	if ((shrm_rx_state == SHRM_IDLE) && (shrm_tx_state == SHRM_IDLE)) {
		shrm_rx_state = SHRM_SLEEP_STATE;
		shrm_tx_state = SHRM_SLEEP_STATE;
	}

	dbgprintk("hrtimer_restart \n");

	return HRTIMER_NORESTART;
}


static void shrm_cawake_req_callback(u8);
static void shrm_modem_reset_req_callback(void);

void shm_ca_wake_req_tasklet(unsigned long tasklet_data)
{
	dbgprintk("++ \n");
	if (pshm_dev) {
		/*initialize the FIFO Variables*/
		if (boot_state == BOOT_INIT)
			shm_fifo_init();

		/*send ca_wake_ack_interrupt to CMU*/
		writel((1<<GOP_CA_WAKE_ACK_BIT), \
			pshm_dev->intr_base+GOP_SET_REGISTER_BASE);
	} else {
		printk(KERN_ALERT "error ca_wake_irq_handler pshm_dev Null\n");
		BUG_ON(1);
	}
	dbgprintk("-- \n");
}

void shm_ca_msgpending_0_tasklet(unsigned long tasklet_data)
{
	unsigned int reader_local_rptr;
	unsigned int reader_local_wptr;
	unsigned int shared_rptr;

	dbgprintk("++ \n");

	/** Interprocess locking */
	spin_lock(&ca_common_lock);
	if (pshm_dev) {

		/**Update_reader_local_wptr with shared_wptr*/
		update_ca_common_reader_local_wptr_with_shared_wptr();
		get_reader_pointers(0, &reader_local_rptr, \
					&reader_local_wptr, &shared_rptr);

		set_ca_msg_0_read_notif_send(0);

		if (boot_state == BOOT_DONE) {
			shrm_rx_state = SHRM_PTR_FREE;
			if (reader_local_rptr != shared_rptr)
				ca_msg_read_notification_0();
			if (reader_local_rptr != reader_local_wptr)
				receive_messages_common();
			else {
				shrm_rx_state = SHRM_IDLE;
				hrtimer_start(&timer,
						ktime_set(0, 2*NSEC_PER_MSEC),
						HRTIMER_MODE_REL);
			}
		} else {
			unsigned int config = 0, version = 0;
			/* BOOT phase.only a BOOT_RESP should be in FIFO*/
			if (boot_state != BOOT_INFO_SYNC) {
				if (read_boot_info_req(&config, &version)) {
					/*SendReadNotification*/
					ca_msg_read_notification_0();
					/*Check the version number before
						sending Boot info response*/

					/*send MsgPending notification*/
					write_boot_info_resp(config, version);
					boot_state = BOOT_INFO_SYNC;
					printk(KERN_ALERT "u8500-shrm:" \
							"BOOT_INFO_SYNC\n");
					send_ac_msg_pending_notification_0();
				 } else {
					 printk(KERN_ALERT "pshm_dev Null\n");
					 BUG_ON(1);
				 }
			} else {
				ca_msg_read_notification_0();
				printk(KERN_ALERT "u8500-shrm : BOOT_INFO_SYNC\n");
			}
		}
	} else {
		printk(KERN_ALERT "error condition pshm_dev is Null\n");
		BUG_ON(1);
	}
		  /** Interprocess locking */
	spin_unlock(&ca_common_lock);
	dbgprintk("-- \n");
}

void shm_ca_msgpending_1_tasklet(unsigned long tasklet_data)
{
	unsigned int reader_local_rptr;
	unsigned int reader_local_wptr;
	unsigned int shared_rptr;

/* This function is called when CaMsgPendingNotification Trigerred by CMU.
	 It means that CMU has wrote a message into Ca Audio FIFO*/

	dbgprintk("++ \n");
	/* Interprocess locking */
	spin_lock(&ca_audio_lock);
	if (pshm_dev) {
		/* Update_reader_local_wptr(with shared_wptr)*/
		update_ca_audio_reader_local_wptr_with_shared_wptr();
		get_reader_pointers(1, &reader_local_rptr, &reader_local_wptr, \
								&shared_rptr);

		set_ca_msg_1_read_notif_send(0);

		if (boot_state == BOOT_DONE) {
			/* Check we already read the message*/
			if (reader_local_rptr != shared_rptr)
				ca_msg_read_notification_1();
			 if (reader_local_rptr != reader_local_wptr)
				receive_messages_audio();
		} else {
			printk(KERN_ALERT "Boot Error\n");
			BUG_ON(1);
		}
	} else {
		printk(KERN_ALERT "error condition pshm_dev is Null\n");
		BUG_ON(1);
	}

	 /* Interprocess locking */
	spin_unlock(&ca_audio_lock);
	dbgprintk("-- \n");
}

void shm_ac_read_notif_0_tasklet(unsigned long tasklet_data)
{
	unsigned int writer_local_rptr;
	unsigned int writer_local_wptr;
	unsigned int shared_wptr;

	dbgprintk("++ \n");
	if (pshm_dev) {
		/* Update writer_local_rptrwith shared_rptr*/
		update_ac_common_writer_local_rptr_with_shared_rptr();
		get_writer_pointers(0, &writer_local_rptr, \
					&writer_local_wptr, &shared_wptr);

		if (boot_state == BOOT_INFO_SYNC) {
			/* BOOT_RESP sent by APE has been received by CMT*/
			boot_state = BOOT_DONE;
			printk(KERN_ALERT "u8500-shrm : IPC_ISA BOOT_DONE\n");
		} else if (boot_state == BOOT_DONE) {
			if (writer_local_rptr != writer_local_wptr) {
				shrm_tx_state = SHRM_PTR_FREE;
				send_ac_msg_pending_notification_0();
		} else {
				shrm_tx_state = SHRM_IDLE;
				/*hrtimer_start(&timer,
				  ktime_set(0, 2*NSEC_PER_MSEC),
				  HRTIMER_MODE_REL);*/
			}
		} else {
			printk(KERN_ALERT "Error Case\n");
			BUG_ON(1);
		}
	} else {
		printk(KERN_ALERT "error condition pshm_dev is Null\n");
		BUG_ON(1);
	}

	dbgprintk("-- \n");
}

void shm_ac_read_notif_1_tasklet(unsigned long tasklet_data)
{
	unsigned int writer_local_rptr;
	unsigned int writer_local_wptr;
	unsigned int shared_wptr;

	dbgprintk("++ \n");

	if (pshm_dev) {

		/* Update writer_local_rptr(with shared_rptr)*/
		update_ac_audio_writer_local_rptr_with_shared_rptr();
		get_writer_pointers(1, &writer_local_rptr, \
				&writer_local_wptr, &shared_wptr);

		if (boot_state == BOOT_DONE) {
			if (writer_local_rptr != writer_local_wptr)
				send_ac_msg_pending_notification_1();
		} else {
			printk(KERN_ALERT "Error Case\n");
			BUG_ON(1);
		}
	} else {
		printk(KERN_ALERT "error condition pshm_dev is Null\n");
		BUG_ON(1);
	}

	dbgprintk("-- \n");
}


void shm_protocol_init(received_msg_handler common_rx_handler,
		      received_msg_handler audio_rx_handler)
{
	boot_state = BOOT_INIT;
	printk(KERN_ALERT "u8500-shrm: IPC_ISA BOOT_INIT\n");
	p_common_rx_handler = common_rx_handler;
	p_audio_rx_handler = audio_rx_handler;
	spin_lock_init(&ca_common_lock);
	spin_lock_init(&ca_audio_lock);
	is_earlydrop = u8500_is_earlydrop();
	if (is_earlydrop != 0x01) {
		hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		timer.function = callback;
	}
	/* register callback with PRCMU for ca_wake_req */
	prcmu_set_callback_cawakereq(&shrm_cawake_req_callback);

	/* register callback with PRCMU for ca_wake_req */
	prcmu_set_callback_modem_reset_request(&shrm_modem_reset_req_callback);

	/* check if there is any initial pending ca_wake_req */
	if (prcmu_is_ca_wake_req_pending())
		shrm_cawake_req_callback(1);

}

void shrm_modem_reset_req_callback(void)
{
	/* Call the PRCMU reset API */
	prcmu_system_reset();
}

void shrm_cawake_req_callback(u8 ca_wake_state)
{
	if (ca_wake_state) {
		if (pshm_dev) {
			/*initialize the FIFO Variables*/
			if (boot_state == BOOT_INIT)
				shm_fifo_init();
		} else {
			printk(KERN_ALERT "error condition \
					ca_wake_irq_handler pshm_dev Null\n");
			BUG_ON(1);
		}

		prcmu_ac_wake_req();

		/*send ca_wake_ack_interrupt to CMU*/
		writel((1<<GOP_CA_WAKE_ACK_BIT), \
				pshm_dev->intr_base+GOP_SET_REGISTER_BASE);

		dbgprintk(KERN_ALERT "shrm_cawake_req_callback high\n");
	} else {

			dbgprintk(KERN_ALERT "shrm_cawake_req_callback slep\n");
			prcmu_ac_sleep_req();
			shrm_tx_state = SHRM_SLEEP_STATE;

	}

}


/** This function is called when Cawake req occurs*/

irqreturn_t ca_wake_irq_handler(int irq, void *ctrlr)
{

	dbgprintk("++ \n");

	if (pshm_dev) {
		/*initialize the FIFO Variables*/
		if (boot_state == BOOT_INIT) {
			shm_fifo_init();

			if (is_earlydrop != 0x01) {

				shrm_rx_state = SHRM_SLEEP_STATE;
				shrm_tx_state = SHRM_SLEEP_STATE;

				/* call Modem Wake Up */
				shrm_tx_state = SHRM_PTR_FREE;
			}
		}
	} else {
		printk(KERN_ALERT "error condition ca_wake_irq_handler \
				pshm_dev Null\n");
		BUG_ON(1);
	}

	dbgprintk(KERN_INFO "\n Inside ca_wake_irq_handler !!");

	/*Clear the interrupt*/
	writel((1<<GOP_CA_WAKE_REQ_BIT), \
		pshm_dev->intr_base+GOP_CLEAR_REGISTER_BASE);

	/*send ca_wake_ack_interrupt to CMU*/
	writel((1<<GOP_CA_WAKE_ACK_BIT), \
		pshm_dev->intr_base+GOP_SET_REGISTER_BASE);

	shrm_rx_state = SHRM_IDLE;

	dbgprintk("-- \n");
	return IRQ_HANDLED;
}


/** This function is called when AcReadNotification interruption occurs.
It means that pending messages have been read by CMT. It is the acknowledgement
of the previous AcMsgPendingNotification*/

irqreturn_t ac_read_notif_0_irq_handler(int irq, void *ctrlr)
{
	dbgprintk("++ \n");
	tasklet_schedule(&shm_ac_read_0_tasklet);

	/*Clear the interrupt*/
	writel((1<<GOP_COMMON_AC_READ_NOTIFICATION_BIT), \
		pshm_dev->intr_base+GOP_CLEAR_REGISTER_BASE);
	dbgprintk("-- \n");
	return IRQ_HANDLED;
}

/** This function is called when AcReadNotification interruption occurs.
 It means that pending messages have been read by CMT. It is the acknowledgement
 of the previous AcMsgPendingNotification*/

irqreturn_t ac_read_notif_1_irq_handler(int irq, void *ctrlr)
{
	dbgprintk("++ \n");
	tasklet_schedule(&shm_ac_read_1_tasklet);
    /*Clear the interrupt*/
	writel((1<<GOP_AUDIO_AC_READ_NOTIFICATION_BIT), \
		pshm_dev->intr_base+GOP_CLEAR_REGISTER_BASE);

	dbgprintk("-- \n");
	return IRQ_HANDLED;
}

/** This function is called when CaMsgPendingNotification Trigerred by CMU.
 It means that CMU has wrote a message into Ca Common FIFO*/

irqreturn_t ca_msg_pending_notif_0_irq_handler(int irq, void *ctrlr)
{

	dbgprintk("++ \n");

	tasklet_schedule(&shm_ca_0_tasklet);
	/*Clear the interrupt*/
	writel((1<<GOP_COMMON_CA_MSG_PENDING_NOTIFICATION_BIT), \
			pshm_dev->intr_base+GOP_CLEAR_REGISTER_BASE);
	dbgprintk("-- \n");
	return IRQ_HANDLED;
}

/** This function is called when CaMsgPendingNotification Trigerred by CMU.
	 It means that CMU has wrote a message into Ca Audio FIFO*/

irqreturn_t ca_msg_pending_notif_1_irq_handler(int irq, void *ctrlr)
{
	dbgprintk("++ \n");

	tasklet_schedule(&shm_ca_1_tasklet);
	/*Clear the interrupt*/
	writel((1<<GOP_AUDIO_CA_MSG_PENDING_NOTIFICATION_BIT), \
			pshm_dev->intr_base+GOP_CLEAR_REGISTER_BASE);
	dbgprintk("-- \n");
	return IRQ_HANDLED;

}


int shm_write_msg(u8 l2_header, void *addr, u32 length)
{
	unsigned char channel = 0;

	if (boot_state == BOOT_DONE) {
		if ((l2_header == 0) || (l2_header == 1) || \
			(l2_header == 3) || (l2_header == 0xC0) || \
						(l2_header == 0xC1)) {
			channel = 0;

			dbgprintk("prcmu_ac_wake_req shrm_tx_state=%d \n",
					shrm_tx_state);
			if (shrm_tx_state == SHRM_SLEEP_STATE) {

				/* call Modem Wake Up */
				dbgprintk("prcmu_ac_wake_req1 \n");
				prcmu_ac_wake_req();
				dbgprintk("prcmu_ac_wake_req 2 \n");

				shrm_tx_state = SHRM_PTR_FREE;
			} else if (shrm_tx_state == SHRM_IDLE)
				shrm_tx_state = SHRM_PTR_FREE;

		} else if ((l2_header == 2) || (l2_header == 0x80) || \
						(l2_header == 0x81)) {
			channel = 1;
		} else
			BUG_ON(1);
		shm_write_msg_to_fifo(channel, l2_header, addr, length);

		/* notify only if new msg copied is the only unread one
		   otherwise it means that reading process is ongoing*/

		if (is_the_only_one_unread_message(channel, length)) {
			/*Send Message Pending Noitication to CMT*/
			if (channel == 0)
				send_ac_msg_pending_notification_0();
			else
				send_ac_msg_pending_notification_1();
		}
	 } else	{
		printk(KERN_ALERT "error after boot done  call this fn\n");
		BUG_ON(1);
	}

	 return 0;
}


/** This function to update shared write pointer and
	   send a AcMsgPendingNotification to CMT*/

void send_ac_msg_pending_notification_0(void)
{
	dbgprintk("++ \n");
	update_ac_common_shared_wptr_with_writer_local_wptr();
    /*Trigger AcMsgPendingNotification to CMU*/
	writel((1<<GOP_COMMON_AC_MSG_PENDING_NOTIFICATION_BIT), \
			pshm_dev->intr_base+GOP_SET_REGISTER_BASE);

	if (shrm_tx_state == SHRM_PTR_FREE)
		shrm_tx_state = SHRM_PTR_BUSY;


	dbgprintk("-- \n");
}

/* This function to update shared write pointer and
	   send a AcMsgPendingNotification to CMT*/

void send_ac_msg_pending_notification_1()
{
	dbgprintk("++ \n");
	/* Update shared_wptr with writer_local_wptr)*/
	update_ac_audio_shared_wptr_with_writer_local_wptr();
    /*Trigger AcMsgPendingNotification to CMU*/
	writel((1<<GOP_AUDIO_AC_MSG_PENDING_NOTIFICATION_BIT), \
			pshm_dev->intr_base+GOP_SET_REGISTER_BASE);

	dbgprintk("-- \n");
}

/*This function sends a CaReadNotification*/
void ca_msg_read_notification_0()
{
	dbgprintk("++ \n");
	if (get_ca_msg_0_read_notif_send() == 0) {
		update_ca_common_shared_rptr_with_reader_local_rptr();
		   /*Trigger CaMsgReadNotification to CMU*/
		writel((1<<GOP_COMMON_CA_READ_NOTIFICATION_BIT), \
			pshm_dev->intr_base+GOP_SET_REGISTER_BASE);
		set_ca_msg_0_read_notif_send(1);

		shrm_rx_state = SHRM_PTR_BUSY;
	}

	dbgprintk("-- \n");
}

/**This function sends a CaReadNotification*/
void ca_msg_read_notification_1()
{
	dbgprintk("++ \n");
	if (get_ca_msg_1_read_notif_send() == 0) {
		update_ca_audio_shared_rptr_with_reader_local_rptr();
		/*Trigger CaMsgReadNotification to CMU*/
		writel((1<<GOP_AUDIO_CA_READ_NOTIFICATION_BIT), \
			pshm_dev->intr_base+GOP_SET_REGISTER_BASE);
		set_ca_msg_1_read_notif_send(1);
	}
   dbgprintk("-- \n");
}

/** This function read messages from the FIFO*/
void receive_messages_common()
{
	unsigned char l2_header;
	unsigned int len;

	l2_header = read_one_l2msg_common(recieve_common_msg, &len);
	/*Send Recieve_Call_back to Upper Layer*/
	if (p_common_rx_handler) {
		(*p_common_rx_handler)(l2_header, &recieve_common_msg, len);
	} else {
		printk(KERN_ALERT "p_common_rx_handler is Null\n");
		BUG_ON(1);
	}
	/*SendReadNotification*/
	ca_msg_read_notification_0();

	while (read_remaining_messages_common())	{
		l2_header = read_one_l2msg_common(recieve_common_msg, \
								&len);
		/*Send Recieve_Call_back to Upper Layer*/
		if (p_common_rx_handler) {
			(*p_common_rx_handler)(l2_header, \
					&recieve_common_msg, len);
		} else {
			printk(KERN_ALERT "p_common_rx_handler is Null\n");
			BUG_ON(1);
		}
	}
}

/** This function read messages from the FIFO*/
void receive_messages_audio()
{
	unsigned char l2_header;
	unsigned int len;

	l2_header = read_one_l2msg_audio(recieve_audio_msg, &len);
	/*Send Recieve_Call_back to Upper Layer*/

	if (p_audio_rx_handler) {
		(*p_audio_rx_handler)(l2_header, &recieve_audio_msg, len);
	} else {
		printk(KERN_ALERT "p_audio_rx_handler is Null\n");
		BUG_ON(1);
	}

	/*SendReadNotification*/
	ca_msg_read_notification_1();
	while (read_remaining_messages_audio())	{
		l2_header = read_one_l2msg_audio(recieve_audio_msg, &len);
		/*Send Recieve_Call_back to Upper Layer*/
		if (p_audio_rx_handler) {
			(*p_audio_rx_handler)(l2_header, \
				&recieve_audio_msg, len);
		} else {
			printk(KERN_ALERT "p_audio_rx_handler is Null\n");
			BUG_ON(1);
		}
	}
}

unsigned char get_boot_state()
{
	return boot_state;
}







