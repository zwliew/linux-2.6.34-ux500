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


/**

MODULE      :  B2R2 Device Driver

FILE        :  b2r2_array.h

DESCRIPTION :  Defines context and status of B2R2 queue array

NOTES       :  User application should infer this definition to
               work for various queue of B2R2.

*/


#ifndef __B2R2_ARRAY_H__
#define __B2R2_ARRAY_H__

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

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


#include "b2r2.h"


	typedef struct  job_context{

	 b2r2_context context;
	 unsigned int jobid;

	 unsigned int queue_address;
	 unsigned int control;
	 unsigned int pace_control;
	 unsigned int interrupt_status;

	 t_uint32     job_done;
     t_uint32     interrupt_target;
     t_uint32     interrupt_context;

     }job_context;


    typedef struct b2r2_job
    {

	struct  job_context  job;
	wait_queue_head_t    event;
	struct  b2r2_job     *nextjob;

    }b2r2_job;


    typedef struct b2r2_jobcontext
	{

	  b2r2_job*            b2r2_head[B2R2_NUMBER_OF_CONTEXTS];
	  rwlock_t		       lock[B2R2_NUMBER_OF_CONTEXTS];
	  t_uint32             index[B2R2_NUMBER_OF_CONTEXTS];
	  t_uint32             jobid;
	  struct               semaphore  context_lock[B2R2_NUMBER_OF_CONTEXTS];

    }b2r2_jobcontext;


    void init_b2r2_jobquqeue(void);
    void add_b2r2_jobqueue(job_context *job);
    void remove_b2r2_jobqueue(job_context *job);
    void exit_b2r2_jobqueue(void);
    void trigger_job(job_context *job);



#ifdef _cplusplus
}
#endif /* _cplusplus */

#endif /* !defined(__B2R2_ARRAY_H__) */












