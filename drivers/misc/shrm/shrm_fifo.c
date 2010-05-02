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


static unsigned char msg_audio_counter;
static unsigned char msg_common_counter;

struct fifo_write_params ape_shm_fifo_0;
struct fifo_write_params ape_shm_fifo_1;
struct fifo_read_params cmt_shm_fifo_0;
struct fifo_read_params cmt_shm_fifo_1;


static unsigned char cmt_read_notif_0_send;
static unsigned char cmt_read_notif_1_send;

void shm_fifo_init(void)
{
	ape_shm_fifo_0.writer_local_wptr	= 0;
	ape_shm_fifo_0.writer_local_rptr	= 0;
	*((unsigned int *)pshm_dev->ac_common_shared_wptr) = 0;
	*((unsigned int *)pshm_dev->ac_common_shared_rptr) = 0;
	ape_shm_fifo_0.shared_wptr		= 0;
	ape_shm_fifo_0.shared_rptr		= 0;
	ape_shm_fifo_0.availablesize32b = pshm_dev->ape_common_fifo_size;
	ape_shm_fifo_0.end_addr_fifo    = pshm_dev->ape_common_fifo_size;
	ape_shm_fifo_0.fifo_virtual_addr = pshm_dev->ape_common_fifo_base;


	cmt_shm_fifo_0.reader_local_rptr	= 0;
	cmt_shm_fifo_0.reader_local_wptr	= 0;
	cmt_shm_fifo_0.shared_wptr	= \
			*((unsigned int *)pshm_dev->ca_common_shared_wptr);
	cmt_shm_fifo_0.shared_rptr	= \
			*((unsigned int *)pshm_dev->ca_common_shared_rptr);
	cmt_shm_fifo_0.availablesize32b	= pshm_dev->cmt_common_fifo_size;
	cmt_shm_fifo_0.end_addr_fifo	= pshm_dev->cmt_common_fifo_size;
	cmt_shm_fifo_0.fifo_virtual_addr = pshm_dev->cmt_common_fifo_base;

	ape_shm_fifo_1.writer_local_wptr	= 0;
	ape_shm_fifo_1.writer_local_rptr	= 0;
	ape_shm_fifo_1.shared_wptr		= 0;
	ape_shm_fifo_1.shared_rptr		= 0;
	*((unsigned int *)pshm_dev->ac_audio_shared_wptr) = 0;
	*((unsigned int *)pshm_dev->ac_audio_shared_rptr) = 0;
	ape_shm_fifo_1.availablesize32b = pshm_dev->ape_audio_fifo_size;
	ape_shm_fifo_1.end_addr_fifo    = pshm_dev->ape_audio_fifo_size;
	ape_shm_fifo_1.fifo_virtual_addr = pshm_dev->ape_audio_fifo_base;

	cmt_shm_fifo_1.reader_local_rptr	= 0;
	cmt_shm_fifo_1.reader_local_wptr	= 0;
	cmt_shm_fifo_1.shared_wptr		= \
			*((unsigned int *)pshm_dev->ca_audio_shared_wptr);
	cmt_shm_fifo_1.shared_rptr		= \
			*((unsigned int *)pshm_dev->ca_audio_shared_rptr);
	cmt_shm_fifo_1.availablesize32b	= pshm_dev->cmt_audio_fifo_size;
	cmt_shm_fifo_1.end_addr_fifo	= pshm_dev->cmt_audio_fifo_size;
	cmt_shm_fifo_1.fifo_virtual_addr = pshm_dev->cmt_audio_fifo_base;
}


unsigned char read_boot_info_req(unsigned int *p_config, \
				unsigned int *p_version)
{
	struct fifo_read_params *p_fifo = &cmt_shm_fifo_0;
	unsigned int *p_msg;
	unsigned int header = 0;
	unsigned char msgtype;

	/* Read L1 header read content of reader_local_rptr */
	 p_msg = (unsigned int *)  \
		(p_fifo->reader_local_rptr+ \
		p_fifo->fifo_virtual_addr);
	 header = *p_msg;
	 msgtype = (header&0xF0000000)>>L1_MSG_MAPID_OFFSET;
	 if (msgtype == 1) {
		*p_config = (header>>8)&0xFF;
		*p_version = header&0xFF;
		p_fifo->reader_local_rptr += 1;

		return 1;
	 } else {
		 printk(KERN_ALERT "Read_Boot_Info_Req Fatal ERROR \n");
		 BUG_ON(1);
	 }

	 return 0;
}

void write_boot_info_resp(unsigned int config, unsigned int version)
{
	struct fifo_write_params *p_fifo = &ape_shm_fifo_0;
	unsigned int *p_msg;

	/** Read L1 header read content of reader_local_rptr */
	p_msg = (unsigned int *) \
		(p_fifo->writer_local_wptr+p_fifo->fifo_virtual_addr);

	*p_msg = ((0x2<<L1_MSG_MAPID_OFFSET)| \
		((config<<8)&0xFF00)|(version & 0xFF));

	p_fifo->writer_local_wptr += 1;
	p_fifo->availablesize32b -= 1;

}

/*Function Which Writes the data into Fifo in IPC zone*/
void shm_write_msg_to_fifo(u8 channel, u8 l2header, void *addr, u32 length)
{
	struct fifo_write_params *p_fifo;
	unsigned int l1_header = 0, l2_header = 0;
	unsigned int requiredsizeb32;
	unsigned int size = 0;
	unsigned int *p_msg;
	unsigned char *p_src;

	if (channel == 0) {
		p_fifo = &ape_shm_fifo_0;
		/* build L1 header*/
		l1_header = ((0x3<<L1_MSG_MAPID_OFFSET)| \
			(((msg_common_counter++)<<20)&0xFF00000)| \
						((length+4)&0xFFFFF));
	} else if (channel == 1) {
		p_fifo = &ape_shm_fifo_1;

		/* build L1 header*/
		l1_header = ((0x3<<L1_MSG_MAPID_OFFSET)| \
			(((msg_audio_counter++)<<20)&0xFF00000)| \
						((length+4)&0xFFFFF));
	} else{
		printk(KERN_ALERT "Wrong Channel\n");
		BUG_ON(1);
	}
	/*L2 size in 32b*/
	requiredsizeb32 = ((length+3)/4);
	/*Add size of L1 & L2 header*/
	requiredsizeb32 += 2;

	/*if availablesize32b = or < requiredsizeb32 then error*/
	if (p_fifo->availablesize32b <= requiredsizeb32) {
		/*  Fatal ERROR - should never happens	*/
		printk(KERN_ALERT "wr_wptr= %x \n", p_fifo->writer_local_wptr);
		printk(KERN_ALERT "wr_rptr= %x \n", p_fifo->writer_local_rptr);

		printk(KERN_ALERT "shared_wptr= %x \n", p_fifo->shared_wptr);
		printk(KERN_ALERT "shared_rptr= %x \n", p_fifo->shared_rptr);
		printk(KERN_ALERT "availsize= %x \n", p_fifo->availablesize32b);
		printk(KERN_ALERT "end__fifo= %x \n", p_fifo->end_addr_fifo);
		printk(KERN_ALERT "Buffer overflow should never happens\n");
		BUG_ON(1);
	}

	/**Need to take care race condition for p_fifo->availablesize32b
	& p_fifo->writer_local_rptr with Ac_Read_notification interrupt.
	One option could be use stack variable for LocalRptr and recompute
	p_fifo->availablesize32b,based on flag enabled in the
	Ac_read_notification */

	l2_header = ((l2header<<24)|((0<<20)&0xF00000)|((length)&0xFFFFF));
	/*Check Local Rptr is less than or equal to Local WPtr */
	if (p_fifo->writer_local_rptr <= p_fifo->writer_local_wptr) {
		p_msg = (unsigned int *) \
			(p_fifo->fifo_virtual_addr+p_fifo->writer_local_wptr);

		/*check enough place bewteen writer_local_wptr & end of FIFO*/
		if ((p_fifo->end_addr_fifo-p_fifo->writer_local_wptr) >= \
							requiredsizeb32) {
			/*Add L1 header and L2 header*/
			*p_msg = l1_header;
			p_msg++;
			*p_msg = l2_header;
			p_msg++;

			/*copy the l2 message in 1 memcpy*/
			memcpy((void *)p_msg, addr, length);
			/* UpdateWptr*/
			p_fifo->writer_local_wptr += requiredsizeb32;
			p_fifo->availablesize32b -= requiredsizeb32;
			p_fifo->writer_local_wptr %= p_fifo->end_addr_fifo;
		} else {
		/* message is split between and of FIFO and beg of FIFO*/
		/* copy first part from writer_local_wptr to end of FIFO */
			size = \
			(p_fifo->end_addr_fifo-p_fifo->writer_local_wptr);

			if (size == 1) {
				/*Add L1 header*/
				*p_msg = l1_header;
				p_msg++;
				/* UpdateWptr*/
				p_fifo->writer_local_wptr = 0;
				p_fifo->availablesize32b -= size;
				/* copy second part from beg of FIFO
					with remaining part of msg*/
				p_msg = \
				(unsigned int *)p_fifo->fifo_virtual_addr;
				*p_msg = l2_header;
				p_msg++;

				/*copy the l3 message in 1 memcpy*/
				memcpy((void *)p_msg, addr, length);
				/* UpdateWptr */
				p_fifo->writer_local_wptr += \
						requiredsizeb32-size;
				p_fifo->availablesize32b -= \
						(requiredsizeb32-size);
			} else if (size == 2) {
				/*Add L1 header and L2 header*/
				*p_msg = l1_header;
				p_msg++;
				*p_msg = l2_header;
				p_msg++;

				/* UpdateWptr*/
				p_fifo->writer_local_wptr = 0;
				p_fifo->availablesize32b -= size;

				/* copy second part from beg of FIFO
				 with remaining part of msg*/
				p_msg = \
				(unsigned int *)p_fifo->fifo_virtual_addr;
				/*copy the l3 message in 1 memcpy*/
				memcpy((void *)p_msg, addr, length);

				/* UpdateWptr */
				p_fifo->writer_local_wptr += \
						requiredsizeb32-size;
				p_fifo->availablesize32b -= \
						(requiredsizeb32-size);
			} else {
				/*Add L1 header and L2 header*/
				*p_msg = l1_header;
				p_msg++;
				*p_msg = l2_header;
				p_msg++;

				/*copy the l2 message in 1 memcpy*/
				memcpy((void *)p_msg, addr, (size-2)*4);

				/* UpdateWptr*/
				p_fifo->writer_local_wptr = 0;
				p_fifo->availablesize32b -= size;

				/* copy second part from beg of FIFO
					with remaining part of msg*/
				p_msg = (unsigned int *) \
					p_fifo->fifo_virtual_addr;
				p_src = \
					(unsigned char *)addr+((size-2)*4);
				memcpy((void *)p_msg, p_src, \
					(length-((size-2)*4)));

				/* UpdateWptr */
				p_fifo->writer_local_wptr += \
							requiredsizeb32-size;
				p_fifo->availablesize32b -= \
							(requiredsizeb32-size);
			}

		}
	} else {
		/* writer_local_rptr > writer_local_wptr*/
		p_msg = (unsigned int *) \
			(p_fifo->fifo_virtual_addr+p_fifo->writer_local_wptr);
		/*Add L1 header and L2 header*/
		*p_msg = l1_header;
		p_msg++;
		*p_msg = l2_header;
		p_msg++;
		/* copy message possbile between writer_local_wptr up
		to writer_local_rptr
		copy the l3 message in 1 memcpy*/
		memcpy((void *)p_msg, addr, length);
		/* UpdateWptr*/
		p_fifo->writer_local_wptr += requiredsizeb32;
		p_fifo->availablesize32b -= requiredsizeb32;

	}

}


/** This function read one message from the FIFO*/

unsigned char read_one_l2msg_common(unsigned char *p_l2_msg, \
					unsigned int *p_len)
{
	struct fifo_read_params *p_fifo = &cmt_shm_fifo_0;

	unsigned int *p_msg;
	unsigned int l1_header = 0;
	unsigned int l2_header = 0;
	unsigned int length;
	unsigned char msgtype;
	unsigned int msg_size_32b;
	unsigned int size = 0;

	/* Read L1 header read content of reader_local_rptr*/
	p_msg = (unsigned int *) \
		(p_fifo->reader_local_rptr+p_fifo->fifo_virtual_addr);
	l1_header = *p_msg++;
	msgtype = (l1_header&0xF0000000)>>28;

	if (msgtype == 3) {
		if (p_fifo->reader_local_rptr == (p_fifo->end_addr_fifo-1)) {
			l2_header = \
				(*((unsigned int *)p_fifo->fifo_virtual_addr));
			length = l2_header&0xFFFFF;
		} else {
		 /** Read L2 header,Msg size & content of reader_local_rptr*/
			l2_header = *p_msg;
			length = l2_header&0xFFFFF;
		}

		*p_len = length;
		msg_size_32b = ((length+3)/4);
		msg_size_32b += 2;

		if (p_fifo->reader_local_rptr + msg_size_32b \
				<= p_fifo->end_addr_fifo) {
				/* Skip L2 header*/
			p_msg++;

			/*read msg between reader_local_rptr and end of FIFO*/
			memcpy((void *)p_l2_msg, (void *)p_msg, length);
			/* UpdateLocalRptr*/
			p_fifo->reader_local_rptr += msg_size_32b;
			p_fifo->reader_local_rptr %= p_fifo->end_addr_fifo;
		} else {

		/* msg split between end of FIFO and beg*/
		/* copy first part of msg*/
		/*read msg between reader_local_rptr and end of FIFO*/
			size = p_fifo->end_addr_fifo-p_fifo->reader_local_rptr;
			if (size == 1) {
				p_msg = (unsigned int *) \
					(p_fifo->fifo_virtual_addr);
				/* Skip L2 header*/
				p_msg++;
				memcpy((void *)p_l2_msg, \
					(void *)(p_msg), length);
			} else if (size == 2) {
				/* Skip L2 header*/
				p_msg++;
				p_msg = (unsigned int *) \
					(p_fifo->fifo_virtual_addr);
				memcpy((void *)p_l2_msg, (void *)(p_msg), \
								length);
			} else {
				/* Skip L2 header*/
				p_msg++;
				memcpy((void *)p_l2_msg, \
					(void *)p_msg, ((size-2)*4));
				/* copy second part of msg*/
				p_l2_msg += ((size-2)*4);
				p_msg = (unsigned int *) \
					(p_fifo->fifo_virtual_addr);
				memcpy((void *)p_l2_msg, (void *)(p_msg), \
							(length-((size-2)*4)));
			}
			p_fifo->reader_local_rptr = \
				(p_fifo->reader_local_rptr+msg_size_32b) % \
							p_fifo->end_addr_fifo;

		}
	 } else	 {
		/*  Fatal ERROR - should never happens	*/
		printk(KERN_ALERT "wr_wptr= %x \n", p_fifo->reader_local_wptr);
		printk(KERN_ALERT "wr_rptr= %x \n", p_fifo->reader_local_rptr);
		printk(KERN_ALERT "shared_wptr= %x \n", p_fifo->shared_wptr);
		printk(KERN_ALERT "shared_rptr= %x \n", p_fifo->shared_rptr);
		printk(KERN_ALERT "availsize= %x \n", p_fifo->availablesize32b);
		printk(KERN_ALERT "end_fifo= %x \n", p_fifo->end_addr_fifo);
		/*  Fatal ERROR - should never happens	*/
		printk(KERN_ALERT "Fatal ERROR - should never happens\n");
		BUG_ON(1);
	 }

	 return (l2_header>>24)&0xFF;
 }

unsigned char read_remaining_messages_common()
{
	struct fifo_read_params *p_fifo = &cmt_shm_fifo_0;
	/** There won't be any Race condition reader_local_rptr &
	p_fifo->reader_local_wptr with CaMsgpending Notification Interrupt*/
	return ((p_fifo->reader_local_rptr != p_fifo->reader_local_wptr) ? \
								1 : 0);
}

/** This function copies one L2 Msg from the Fifo and returns the l2_header*/

unsigned char read_one_l2msg_audio(unsigned char *p_l2_msg, unsigned int *p_len)
{
	struct fifo_read_params *p_fifo = &cmt_shm_fifo_1;

	unsigned int *p_msg;
	unsigned int l1_header = 0;
	unsigned int l2_header = 0;
	unsigned int length;
	unsigned char msgtype;
	unsigned int msg_size_32b;
	unsigned int size = 0;

	/** Read L1 header read content of reader_local_rptr*/
	 p_msg = (unsigned int *) \
		(p_fifo->reader_local_rptr+p_fifo->fifo_virtual_addr);
	 l1_header = *p_msg++;
	 msgtype = (l1_header&0xF0000000)>>28;

	if (msgtype == 3) {
		if (p_fifo->reader_local_rptr == (p_fifo->end_addr_fifo-1)) {
			l2_header = (*((unsigned int *) \
					p_fifo->fifo_virtual_addr));
			length = l2_header&0xFFFFF;
		} else {
		 /** Read L2 header,Msg size & content of reader_local_rptr*/
			l2_header = *p_msg;
			length = l2_header&0xFFFFF;
		}

		*p_len = length;
		msg_size_32b = ((length+3)/4);
		msg_size_32b += 2;

		if (p_fifo->reader_local_rptr + msg_size_32b  \
					<= p_fifo->end_addr_fifo) {
			/**Skip L2 header*/
			p_msg++;

			/*read msg between reader_local_rptr and end of FIFO*/
			memcpy((void *)p_l2_msg, (void *)p_msg, length);
			/* UpdateLocalRptr*/
			p_fifo->reader_local_rptr += msg_size_32b;
			p_fifo->reader_local_rptr %= p_fifo->end_addr_fifo;

		} else {

			/* msg split between end of FIFO and beg*/
			/* copy first part of msg*/
			/*read msg between reader_local_rptr and end of FIFO*/
			size = p_fifo->end_addr_fifo-p_fifo->reader_local_rptr;
			if (size == 1) {
				p_msg = (unsigned int *) \
						(p_fifo->fifo_virtual_addr);
				/*Skip L2 header*/
				p_msg++;
				memcpy((void *)p_l2_msg, (void *)(p_msg), \
								length);
			} else if (size == 2) {
				/*Skip L2 header*/
				p_msg++;

				p_msg = (unsigned int *) \
					(p_fifo->fifo_virtual_addr);
				memcpy((void *)p_l2_msg, (void *)(p_msg), \
								length);
			} else {
				/**Skip L2 header*/
				p_msg++;
				memcpy((void *)p_l2_msg, (void *)p_msg, \
								((size-2)*4));
				/* copy second part of msg*/
				p_l2_msg += ((size-2)*4);
				p_msg = (unsigned int *) \
					(p_fifo->fifo_virtual_addr);
				memcpy((void *)p_l2_msg, (void *)(p_msg), \
							(length-((size-2)*4)));

			}
			p_fifo->reader_local_rptr = \
				(p_fifo->reader_local_rptr+msg_size_32b) % \
						p_fifo->end_addr_fifo;

		}
	 } else {
		/*  Fatal ERROR - should never happens	*/
		printk(KERN_ALERT "wr_local_wptr= %x \n", \
						p_fifo->reader_local_wptr);
		printk(KERN_ALERT "wr_local_rptr= %x \n", \
						p_fifo->reader_local_rptr);
		printk(KERN_ALERT "shared_wptr= %x \n", \
						p_fifo->shared_wptr);
		printk(KERN_ALERT "shared_rptr= %x \n", p_fifo->shared_rptr);
		printk(KERN_ALERT "availsize=%x \n", p_fifo->availablesize32b);
		printk(KERN_ALERT "end_fifo= %x \n", p_fifo->end_addr_fifo);
		/*  Fatal ERROR - should never happens	*/
		printk(KERN_ALERT "Fatal ERROR - should never happens\n");
		BUG_ON(1);
	 }

	 return (l2_header>>24)&0xFF;


 }

unsigned char read_remaining_messages_audio()
{
	struct fifo_read_params *p_fifo = &cmt_shm_fifo_1;

	return ((p_fifo->reader_local_rptr != p_fifo->reader_local_wptr) ? \
									1 : 0);
}

unsigned char is_the_only_one_unread_message(u8 channel, u32 length)
{
	struct fifo_write_params *p_fifo;
	unsigned int Messagesize32b = 0;
	unsigned char is_only_one_unread_msg = 0;

	if (channel == 0)
		p_fifo = &ape_shm_fifo_0;
	else if (channel == 1)
		p_fifo = &ape_shm_fifo_1;
	else {
		printk(KERN_ALERT "Fatal ERROR - should never happens\n");
		BUG_ON(1);
	}

	/*L3 size in 32b*/
	Messagesize32b = ((length+3)/4);
	/*Add size of L1 & L2 header*/
	Messagesize32b += 2;
    /*possibility of race condition with Ac Read notification interrupt.
	need to check ?*/
	if (p_fifo->writer_local_wptr > p_fifo->writer_local_rptr)
		is_only_one_unread_msg = \
			((p_fifo->writer_local_rptr + Messagesize32b) == \
			p_fifo->writer_local_wptr) ? 1 : 0;
	else
		/*Msg split between end of fifo and starting of Fifo*/
		is_only_one_unread_msg = \
			(((p_fifo->writer_local_rptr + Messagesize32b)% \
			p_fifo->end_addr_fifo) == p_fifo->writer_local_wptr) ? \
									1 : 0;

	return is_only_one_unread_msg  ;
}

/** Ca_Common_reader_local_wptr*/
void update_ca_common_reader_local_wptr_with_shared_wptr()
{
	struct fifo_read_params *p_fifo = &cmt_shm_fifo_0;

	p_fifo->shared_wptr = \
		(*((unsigned int *)pshm_dev->ca_common_shared_wptr));
	p_fifo->reader_local_wptr = p_fifo->shared_wptr;
}

/** Update Ca_Audio_reader_local_wptr*/
void update_ca_audio_reader_local_wptr_with_shared_wptr()
{
	struct fifo_read_params *p_fifo = &cmt_shm_fifo_1;

	p_fifo->shared_wptr = \
		(*((unsigned int *)pshm_dev->ca_audio_shared_wptr));
	p_fifo->reader_local_wptr = p_fifo->shared_wptr;
}

/** Update Ac_Common_writer_local_rptr*/
void update_ac_common_writer_local_rptr_with_shared_rptr()
{
	struct fifo_write_params *p_fifo;
	unsigned int free_space_in32b = 0;

	p_fifo = &ape_shm_fifo_0;

	p_fifo->shared_rptr = \
		(*((unsigned int *)pshm_dev->ac_common_shared_rptr));

	if (p_fifo->shared_rptr >= p_fifo->writer_local_rptr)
		free_space_in32b = \
			(p_fifo->shared_rptr-p_fifo->writer_local_rptr);
	else {
		free_space_in32b = \
			(p_fifo->end_addr_fifo-p_fifo->writer_local_rptr);
		free_space_in32b += p_fifo->shared_rptr;
	}

	/*Chance of race condition of below variables with write_msg*/
	p_fifo->availablesize32b += free_space_in32b;
	p_fifo->writer_local_rptr = p_fifo->shared_rptr;
}

/** Update Ac_Audio_writer_local_rptr*/

void update_ac_audio_writer_local_rptr_with_shared_rptr()
{
	struct fifo_write_params *p_fifo;
	unsigned int free_space_in32b = 0;

	p_fifo = &ape_shm_fifo_1;
	p_fifo->shared_rptr = \
		(*((unsigned int *)pshm_dev->ac_audio_shared_rptr));

	if (p_fifo->shared_rptr >= p_fifo->writer_local_rptr)
		free_space_in32b = \
			(p_fifo->shared_rptr-p_fifo->writer_local_rptr);
	else {
		free_space_in32b = \
			(p_fifo->end_addr_fifo-p_fifo->writer_local_rptr);
		free_space_in32b += p_fifo->shared_rptr;
	}

	/*Chance of race condition of below variables with write_msg*/
	p_fifo->availablesize32b += free_space_in32b;
	p_fifo->writer_local_rptr = p_fifo->shared_rptr;
}

/** Update ac_audio_shared_wptr*/
void update_ac_common_shared_wptr_with_writer_local_wptr()
{
	struct fifo_write_params *p_fifo;

	p_fifo = &ape_shm_fifo_0;
	/*Update shared pointer fifo offset of the IPC zone*/
	(*((unsigned int *)pshm_dev->ac_common_shared_wptr)) = \
					 p_fifo->writer_local_wptr;

	p_fifo->shared_wptr = p_fifo->writer_local_wptr;
}

/** Update ac_audio_shared_wptr*/
void update_ac_audio_shared_wptr_with_writer_local_wptr()
{
	struct fifo_write_params *p_fifo;

	p_fifo = &ape_shm_fifo_1;
	/*Update shared pointer fifo offset of the IPC zone*/
	(*((unsigned int *)pshm_dev->ac_audio_shared_wptr)) = \
					p_fifo->writer_local_wptr;
	p_fifo->shared_wptr = p_fifo->writer_local_wptr;
}

/** Update ca_common_shared_rptr*/
void update_ca_common_shared_rptr_with_reader_local_rptr()
{
	struct fifo_read_params *p_fifo;

	p_fifo = &cmt_shm_fifo_0;

	/*Update shared pointer fifo offset of the IPC zone*/
	(*((unsigned int *)pshm_dev->ca_common_shared_rptr)) = \
					p_fifo->reader_local_rptr;
	p_fifo->shared_rptr = p_fifo->reader_local_rptr;
}

/** Update ca_audio_shared_rptr*/
void update_ca_audio_shared_rptr_with_reader_local_rptr()
{
	struct fifo_read_params *p_fifo;

	p_fifo = &cmt_shm_fifo_1;

	/*Update shared pointer fifo offset of the IPC zone*/
	(*((unsigned int *)pshm_dev->ca_audio_shared_rptr)) = \
					p_fifo->reader_local_rptr;
	p_fifo->shared_rptr = p_fifo->reader_local_rptr;
}


void get_reader_pointers(u8 msg_type, u32 *reader_local_rptr, \
			u32 *reader_local_wptr, u32 *shared_rptr)
{
	struct fifo_read_params *p_fifo;

	switch (msg_type) {
	case 0:
		p_fifo = &cmt_shm_fifo_0;
		*reader_local_rptr = p_fifo->reader_local_rptr;
		*reader_local_wptr = p_fifo->reader_local_wptr;
		*shared_rptr = p_fifo->shared_rptr;
		break;
	case 1:
		p_fifo = &cmt_shm_fifo_1;
		*reader_local_rptr = p_fifo->reader_local_rptr;
		*reader_local_wptr = p_fifo->reader_local_wptr;
		*shared_rptr = p_fifo->shared_rptr;
	break;
	default:
	/*Message is not supported*/
	/*  Fatal ERROR - should never happens	*/
	break;
	}

}

void get_writer_pointers(u8 msg_type, u32 *writer_local_rptr, \
			 u32 *writer_local_wptr, u32 *shared_wptr)
{
	struct fifo_write_params *p_fifo;

	switch (msg_type) {
	case 0:
		p_fifo = &ape_shm_fifo_0;
		*writer_local_rptr = p_fifo->writer_local_rptr;
		*writer_local_wptr = p_fifo->writer_local_wptr;
		*shared_wptr = p_fifo->shared_wptr;
	break;
	case 1:
		p_fifo = &ape_shm_fifo_1;
		*writer_local_rptr = p_fifo->writer_local_rptr;
		*writer_local_wptr = p_fifo->writer_local_wptr;
		*shared_wptr = p_fifo->shared_wptr;
	break;
	default:
	/*Message is not supported*/
	/*  Fatal ERROR - should never happens	*/
	printk(KERN_ALERT "Fatal ERROR - should never happens\n");
	break;
	}
}

void set_ca_msg_0_read_notif_send(unsigned char val)
{
	cmt_read_notif_0_send = val;
}

unsigned char get_ca_msg_0_read_notif_send(void)
{
	return cmt_read_notif_0_send;
}

void set_ca_msg_1_read_notif_send(unsigned char val)
{
	cmt_read_notif_1_send = val;
}

unsigned char get_ca_msg_1_read_notif_send(void)
{
	return cmt_read_notif_1_send;
}

