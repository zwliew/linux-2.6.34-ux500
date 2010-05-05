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

#ifndef __SHRM_PRIVATE_INCLUDED
#define __SHRM_PRIVATE_INCLUDED

#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <mach/shrm.h>

#define GOP_OUTPUT_REGISTER_BASE (0x0)
#define GOP_SET_REGISTER_BASE    (0x4)
#define GOP_CLEAR_REGISTER_BASE  (0x8)
#define GOP_TOGGLE_REGISTER_BASE (0xc)


#define GOP_AUDIO_AC_READ_NOTIFICATION_BIT (0)
#define GOP_AUDIO_CA_MSG_PENDING_NOTIFICATION_BIT (1)
#define GOP_COMMON_AC_READ_NOTIFICATION_BIT (2)
#define GOP_COMMON_CA_MSG_PENDING_NOTIFICATION_BIT (3)
#define GOP_CA_WAKE_REQ_BIT (7)
#define GOP_AUDIO_CA_READ_NOTIFICATION_BIT (23)
#define GOP_AUDIO_AC_MSG_PENDING_NOTIFICATION_BIT (24)
#define GOP_COMMON_CA_READ_NOTIFICATION_BIT (25)
#define GOP_COMMON_AC_MSG_PENDING_NOTIFICATION_BIT (26)
#define GOP_CA_WAKE_ACK_BIT (27)

#define L2_MSG_MAPID_OFFSET (24)
#define L1_MSG_MAPID_OFFSET (28)

#define SHM_DEBUG_ENABLE (0)

#define SHRM_SLEEP_STATE (0)
#define SHRM_PTR_FREE (1)
#define SHRM_PTR_BUSY (2)
#define SHRM_IDLE (3)

#if SHM_DEBUG_ENABLE

#define dbgprintk(format, args...) \
	do { \
			printk(KERN_ALERT "SHM:"); \
			printk("%s:", __func__); \
			printk(format, ##args); \
	} while (0)

#else

#define dbgprintk(format, args...) \

#endif


typedef void (*MSG_PENDING_NOTIF)(const unsigned int Wptr);

struct fifo_write_params {
	unsigned int writer_local_rptr;
	unsigned int writer_local_wptr;
	unsigned int shared_wptr;
	unsigned int shared_rptr;
	unsigned int availablesize32b;
	unsigned int end_addr_fifo;
	unsigned int *fifo_virtual_addr;

} ;

struct fifo_read_params{
	unsigned int reader_local_rptr;
	unsigned int reader_local_wptr;
	unsigned int shared_wptr;
	unsigned int shared_rptr;
	unsigned int availablesize32b;
	unsigned int end_addr_fifo;
	unsigned int *fifo_virtual_addr;

} ;

extern struct shrm_dev *pshm_dev;

void shm_protocol_init(received_msg_handler common_rx_handler, \
			   received_msg_handler audio_rx_handler);
void shm_fifo_init(void);
void shm_write_msg_to_fifo(u8 channel, u8 l2header, void *addr, u32 length);
int shm_write_msg(u8 l2_header, void *addr, u32 length);

unsigned char is_the_only_one_unread_message(u8 channel, u32 length);
unsigned char read_remaining_messages_common(void);
unsigned char read_remaining_messages_audio(void);
unsigned char read_one_l2msg_audio(unsigned char *p_l2_msg, \
						unsigned int *p_len);
unsigned char read_one_l2msg_common(unsigned char *p_l2_msg, \
						unsigned int *p_len);
void receive_messages_common(void);
void receive_messages_audio(void);

void update_ac_common_writer_local_rptr_with_shared_rptr(void);
void update_ac_audio_writer_local_rptr_with_shared_rptr(void);
void update_ca_common_reader_local_wptr_with_shared_wptr(void);
void update_ca_audio_reader_local_wptr_with_shared_wptr(void);
void update_ac_common_shared_wptr_with_writer_local_wptr(void);
void update_ac_audio_shared_wptr_with_writer_local_wptr(void);
void update_ca_common_shared_rptr_with_reader_local_rptr(void);
void update_ca_audio_shared_rptr_with_reader_local_rptr(void);


void get_writer_pointers(u8 msg_type, u32 *WriterLocalRptr, \
			u32 *WriterLocalWptr, u32 *SharedWptr);
void get_reader_pointers(u8 msg_type, u32 *ReaderLocalRptr, \
			u32 *ReaderLocalWptr, u32 *SharedRptr);
unsigned char read_boot_info_req(unsigned int *pConfig, \
					unsigned int *pVersion);
void write_boot_info_resp(unsigned int Config, unsigned int Version);

void send_ac_msg_pending_notification_0(void);
void send_ac_msg_pending_notification_1(void);
void ca_msg_read_notification_0(void);
void ca_msg_read_notification_1(void);

void set_ca_msg_0_read_notif_send(unsigned char val);
unsigned char get_ca_msg_0_read_notif_send(void);
void set_ca_msg_1_read_notif_send(unsigned char val);
unsigned char get_ca_msg_1_read_notif_send(void);

irqreturn_t ca_wake_irq_handler(int irq, void *ctrlr);
irqreturn_t ac_read_notif_0_irq_handler(int irq, void *ctrlr);
irqreturn_t ac_read_notif_1_irq_handler(int irq, void *ctrlr);
irqreturn_t ca_msg_pending_notif_0_irq_handler(int irq, void *ctrlr);
irqreturn_t ca_msg_pending_notif_1_irq_handler(int irq, void *ctrlr);

void shm_ca_msgpending_0_tasklet(unsigned long);
void shm_ca_msgpending_1_tasklet(unsigned long);
void shm_ac_read_notif_0_tasklet(unsigned long);
void shm_ac_read_notif_1_tasklet(unsigned long);
void shm_ca_wake_req_tasklet(unsigned long);

unsigned char get_boot_state(void);

int get_ca_wake_req_state(void);

#endif

