/*---------------------------------------------------------------------------*/
/* © copyright STEricsson,2009. All rights reserved. For   */
/* information, STEricsson reserves the right to license    */
/* this software concurrently under separate license conditions.             */
/*                                                                           */
/* This program is free software; you can redistribute it and/or modify it   */
/* under the terms of the GNU Lesser General Public License as published     */
/* by the Free Software Foundation; either version 2.1 of the License,       */
/* or (at your option)any later version.                                     */
/*                                                                           */
/* This program is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See                  */
/* the GNU Lesser General Public License for more details.                   */
/*                                                                           */
/* You should have received a copy of the GNU Lesser General Public License  */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.      */
/*---------------------------------------------------------------------------*/


#ifndef __B2R2_H
#define __B2R2_H

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

/** Linux include files:charachter driver and memory functions  */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/page-flags.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/cache.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>

#include <asm/dma.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/bug.h>

#include <linux/platform_device.h>
#include <linux/miscdevice.h>


#include <asm/hardware/cache-l2x0.h>

/** Implementation include files */

#include "b2r2_defines.h"
#include "b2r2_array.h"
#include "b2r2_structures.h"
#include "b2r2_driver_usr_api.h"


/** b2r2 event list */

typedef struct b2r2_event_list{

    /** Event queue */

	wait_queue_head_t cq1;
	wait_queue_head_t cq2;
	wait_queue_head_t aq1;
	wait_queue_head_t aq2;
	wait_queue_head_t aq3;
	wait_queue_head_t aq4;

    /** irq status */

	t_uint32  cq1_irq;
	t_uint32  cq2_irq;
	t_uint32  aq1_irq;
	t_uint32  aq2_irq;
	t_uint32  aq3_irq;
	t_uint32  aq4_irq;

	/** irq error */

	t_uint32  error;

}b2r2_event_list;



typedef struct b2r2_memory
{

 t_uint32   physical_address;
 t_uint32   logical_address;
 t_uint32   size_of_memory;
 struct     b2r2_memory *next;

}b2r2_memory;


typedef struct b2r2_instance_memory
{

 t_uint32   memory_counter;
 t_uint32   instance_number;
 struct     b2r2_memory *head;
 struct     b2r2_memory *present;

}b2r2_instance_memory;


int b2r2_hw_reset(void);
int  b2r2_init_hw(struct platform_device *pdev);
void b2r2_exit_hw(void);
int  process_ioctl(void *open_id, unsigned int cmd, void *arg);

#ifdef _cplusplus
}
#endif /* _cplusplus */

#endif /* !defined(__B2R2_H) */
