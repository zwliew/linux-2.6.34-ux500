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

FILE        :  b2r2_array.c

DESCRIPTION :  Implements context and status changes functions
               of B2R2 queue array

*/

/** Include files */

#include <linux/spinlock.h>
#include <linux/delay.h>

#include "b2r2.h"


extern  struct b2r2_jobcontext   jobcontext;
extern  struct s_B2R2           *B2R2_System;


t_uint32  context_index(b2r2_context context)
{

   t_uint32  value=0xff;

   switch(context)
   {
      case B2R2_CONTEXT_CQ1:
      {

		value=0;

        break;
      }
      case B2R2_CONTEXT_CQ2:
      {
        value=1;

        break;
      }
      case B2R2_CONTEXT_AQ1:
      {
        value=2;

        break;
      }
      case B2R2_CONTEXT_AQ2:
      {
        value=3;

        break;
      }
      case B2R2_CONTEXT_AQ3:
      {
        value=4;

        break;
      }
      case B2R2_CONTEXT_AQ4:
      {
		value=5;

        break;
      }
      case B2R2_CONTEXT_ERROR:
      {
		value=6;

        break;
      }

     /** Handle the default case */
      default:
      break;

   }


   return value;


}


void  trigger_job(job_context *job)
{


   b2r2_assert(job==NULL);

   dbgprintk("context 0x%x \n",job->context);
   dbgprintk("BLT TRIG_IP 0x%x \n",job->queue_address);
   dbgprintk("BLT TRIG_CTL 0x%x \n",job->control);
   dbgprintk("BLT PACE/LNA_CTL 0x%x \n",job->pace_control);

	switch(job->context)
    {

	case B2R2_CONTEXT_CQ1:
	{
		B2R2_System->BLT_CQ1_TRIG_IP   =  job->queue_address;
		B2R2_System->BLT_CQ1_TRIG_CTL  =  job->control;
		B2R2_System->BLT_CQ1_PACE_CTL  =  job->pace_control;
		break;
    }

	case B2R2_CONTEXT_CQ2:
	{
		B2R2_System->BLT_CQ2_TRIG_IP   =  job->queue_address;
		B2R2_System->BLT_CQ2_TRIG_CTL  =  job->control;
		B2R2_System->BLT_CQ2_PACE_CTL  =  job->pace_control;
		break;
    }

	case B2R2_CONTEXT_AQ1:
	{
		B2R2_System->BLT_AQ1_IP   =  job->queue_address;
		B2R2_System->BLT_AQ1_CTL  =  job->control;
		/* Memory barrier */
		dsb();
		B2R2_System->BLT_AQ1_LNA  =  job->pace_control;
		break;
    }

	case B2R2_CONTEXT_AQ2:
	{
		B2R2_System->BLT_AQ2_IP   =  job->queue_address;
		B2R2_System->BLT_AQ2_CTL  =  job->control;
		/* Memory barrier */
		dsb();
		B2R2_System->BLT_AQ2_LNA  =  job->pace_control;
		break;
    }

	case B2R2_CONTEXT_AQ3:
	{
		B2R2_System->BLT_AQ3_IP   =  job->queue_address;
		B2R2_System->BLT_AQ3_CTL  =  job->control;
		/* Memory barrier */
		dsb();
		B2R2_System->BLT_AQ3_LNA  =  job->pace_control;
		break;
    }

	case B2R2_CONTEXT_AQ4:
	{
		B2R2_System->BLT_AQ4_IP   =  job->queue_address;
		B2R2_System->BLT_AQ4_CTL  =  job->control;
		/* Memory barrier */
		dsb();
		B2R2_System->BLT_AQ4_LNA  =  job->pace_control;
		break;
    }

    /** Handle the default case */
    default:
    break;

    }/* end switch */

}

 void init_b2r2_jobquqeue(void)
 {
	int i;

	for(i=0;i<B2R2_NUMBER_OF_CONTEXTS;i++)
	{
		jobcontext.index[i]=0;
		jobcontext.b2r2_head[i]=NULL;
		jobcontext.jobid=0;
		init_MUTEX(&jobcontext.context_lock[i]);
	}

 }

 void exit_b2r2_jobqueue(void)
{
	int i,j;
	b2r2_job  *job;

	for(i=0;i<B2R2_NUMBER_OF_CONTEXTS;i++)
	{
		if(jobcontext.index[i]>0)
		{
           for(j=0;j<jobcontext.index[i];j++)
           {
              job=jobcontext.b2r2_head[i]->nextjob;
              kfree(jobcontext.b2r2_head[i]);
              jobcontext.b2r2_head[i]=job;
		   }

		   jobcontext.index[i]=0;
	    }
    }
 }

DECLARE_MUTEX(jobid_mutex);

 void add_b2r2_jobqueue(job_context *job_to_add)
{
   b2r2_job     *newjob,*temp;
   int          index;
   int 		retVal = 0;

   b2r2_assert(job_to_add==NULL);

   dbgprintk("Enters add_b2r2_jobqueue \n");

   retVal = down_interruptible(&jobid_mutex);

   jobcontext.jobid++;

   dbgprintk("jobid is increased 0x%x \n",jobcontext.jobid);

   job_to_add->jobid=jobcontext.jobid;

   up(&jobid_mutex);

   job_to_add->job_done=0;

   dbgprintk("job context 0x%x \n",job_to_add->context);

   index=context_index(job_to_add->context);

   dbgprintk("b2r2 index 0x%x \n",index);

   retVal = down_interruptible(&jobcontext.context_lock[index]);

#ifndef B2R2_ONLY_TOPHALF
   write_lock_bh(&(jobcontext.lock[index]));
#else
   disable_irq(IRQ_B2R2);
#endif



   newjob=(struct b2r2_job* )kmalloc(sizeof(struct b2r2_job),GFP_USER);

   b2r2_assert(newjob==NULL);

   dbgprintk("new job is allocated at address 0x%x \n",(unsigned int)newjob);

   memcpy(&(newjob->job),job_to_add,sizeof(struct job_context));

   init_waitqueue_head(&newjob->event);

   dbgprintk("copied the given job \n");

	if(jobcontext.b2r2_head[index]==NULL)
	{
		dbgprintk("job head index is null \n");

		jobcontext.b2r2_head[index]=newjob;
		jobcontext.b2r2_head[index]->nextjob=NULL;

		dbgprintk("triggering the job \n");

		/** set the correct interrupt and it's target */
		switch(newjob->job.interrupt_target)
		{
		case 0:  /** ARM */
			B2R2_System->BLT_ITM0 |= newjob->job.interrupt_context;
			break;
		case 1:  /** SAA */
			B2R2_System->BLT_ITM1 |= newjob->job.interrupt_context;
			break;
		case 2:  /** SVA */
			B2R2_System->BLT_ITM2 |= newjob->job.interrupt_context;
			break;
		case 3:  /** SIA */
			B2R2_System->BLT_ITM3 |= newjob->job.interrupt_context;
			break;
		}

		trigger_job(&newjob->job);
	}
	else
	{

	   dbgprintk("job head index is not null \n");

	   temp=jobcontext.b2r2_head[index];
	   jobcontext.b2r2_head[index]=newjob;
	   jobcontext.b2r2_head[index]->nextjob=temp;

	   dbgprintk("previous job status jobid 0x%x jobdone 0x%x \n",
	                 jobcontext.b2r2_head[index]->nextjob->job.jobid,
	                 jobcontext.b2r2_head[index]->nextjob->job.job_done);

	   /** we require to trigger the job because early job is finished */
	   if(jobcontext.b2r2_head[index]->nextjob->job.job_done==1)
	   {
		   switch(newjob->job.interrupt_target)
		   {
		   case 0:  /** ARM */
			   B2R2_System->BLT_ITM0 |= newjob->job.interrupt_context;
			   break;
		   case 1:  /** SAA */
			   B2R2_System->BLT_ITM1 |= newjob->job.interrupt_context;
			   break;
		   case 2:  /** SVA */
			   B2R2_System->BLT_ITM2 |= newjob->job.interrupt_context;
			   break;
		   case 3:  /** SIA */
			   B2R2_System->BLT_ITM3 |= newjob->job.interrupt_context;
			   break;
		   }

		   trigger_job(&newjob->job);
	  }
	}

	jobcontext.index[index]++;

#ifndef B2R2_ONLY_TOPHALF
	write_unlock_bh(&(jobcontext.lock[index]));
#else
    enable_irq(IRQ_B2R2);
#endif
	up(&jobcontext.context_lock[index]);
	dbgprintk("Returns add_b2r2_jobqueue \n");

	return;
}

 void remove_b2r2_jobqueue(job_context *job_to_remove)
{
	  b2r2_job   *temp1,*temp2,*temp3;
      int        index,i;
      int retVal = 0;
      long timeout;

      dbgprintk("Enters remove_b2r2_jobqueue \n");

      b2r2_assert(job_to_remove==NULL);

	  temp3=NULL;
	  temp2=NULL;
	  index=context_index(job_to_remove->context);

      retVal = down_interruptible(&jobcontext.context_lock[index]);

#ifndef B2R2_ONLY_TOPHALF
	  write_lock_bh(&(jobcontext.lock[index]));
#else
      disable_irq(IRQ_B2R2);
#endif

	  temp1=jobcontext.b2r2_head[index];

	  for(i=0;i<jobcontext.index[index];i++)
	  {
		  dbgprintk("job number 0x%x jobid 0x%x jobaddress 0x%x nextjob address 0x%x \n",i,temp1->job.jobid,(unsigned int)temp1,(unsigned int)temp1->nextjob);
		  temp1=temp1->nextjob;
	  }

      temp1=jobcontext.b2r2_head[index];

      dbgprintk("index 0x%x \n",index);
	  dbgprintk("jobid 0x%x \n",job_to_remove->jobid);
	  dbgprintk("jobcontext index 0x%x \n",jobcontext.index[index]);
	  dbgprintk("jobcontext head  0x%x \n",(unsigned int)temp1);
	  dbgprintk("jobcontext jobid  0x%x \n",(unsigned int)temp1->job.jobid);

      for(i=0;i<jobcontext.index[index];i++)
	  {
		  if(job_to_remove->jobid==temp1->job.jobid)
		  break;

		  temp3=temp1;
		  if(temp1->nextjob!=NULL)
		  {
			temp2=temp1->nextjob;
	                temp1=temp2;
          }

		  if(temp1->nextjob!=NULL)
		  temp2=temp1->nextjob;
		  else
		  temp2=NULL;
      }

      dbgprintk("sleep for interrupt \n");

#ifdef B2R2_ONLY_TOPHALF
      enable_irq(IRQ_B2R2);
#endif

      // wait_event_interruptible(temp1->event,temp1->job.job_done==1);
      timeout = wait_event_interruptible_timeout(temp1->event,
						     temp1->job.job_done==1,
						 max(HZ/2, 2));
      if (timeout == 0) {
	      printk(KERN_INFO "%s: Timeout!\n", __func__);

	      printk(KERN_INFO "%s: JOB: %d\n", __func__, job_to_remove->jobid);
	      printk(KERN_INFO "===========================\n");
	      printk(KERN_INFO "CTL: %X\n", B2R2_System->BLT_CTL);
	      printk(KERN_INFO "ITM0: %X\n", B2R2_System->BLT_ITM0);
	      printk(KERN_INFO "ITS: %X\n", B2R2_System->BLT_ITS);
	      printk(KERN_INFO "STA1: %X\n", B2R2_System->BLT_STA1);
	      printk(KERN_INFO "NIP: %X\n", B2R2_System->BLT_NIP);
	      printk(KERN_INFO "CIC: %X\n", B2R2_System->BLT_CIC);
	      printk(KERN_INFO "INS: %X\n", B2R2_System->BLT_INS);
	      printk(KERN_INFO "ACK: %X\n", B2R2_System->BLT_ACK);
	      printk(KERN_INFO "TBA: %X\n", B2R2_System->BLT_TBA);
	      printk(KERN_INFO "TTY: %X\n", B2R2_System->BLT_TTY);
	      printk(KERN_INFO "TXY: %X\n", B2R2_System->BLT_TXY);
	      printk(KERN_INFO "TSZ: %X\n", B2R2_System->BLT_TSZ);
	      printk(KERN_INFO "S1BA: %X\n", B2R2_System->BLT_S1BA);
	      printk(KERN_INFO "S1TY: %X\n", B2R2_System->BLT_S1TY);
	      printk(KERN_INFO "S1XY: %X\n", B2R2_System->BLT_S1XY);
	      printk(KERN_INFO "S2BA: %X\n", B2R2_System->BLT_S2BA);
	      printk(KERN_INFO "S2TY: %X\n", B2R2_System->BLT_S2TY);
	      printk(KERN_INFO "S2XY: %X\n", B2R2_System->BLT_S2XY);
	      printk(KERN_INFO "S2SZ: %X\n", B2R2_System->BLT_S2SZ);
	      printk(KERN_INFO "S3BA: %X\n", B2R2_System->BLT_S3BA);
	      printk(KERN_INFO "S3TY: %X\n", B2R2_System->BLT_S3TY);
	      printk(KERN_INFO "S3XY: %X\n", B2R2_System->BLT_S3XY);
	      printk(KERN_INFO "S3SZ: %X\n", B2R2_System->BLT_S3SZ);

	      while ((B2R2_System->BLT_STA1 & 1) == 0) {
		      printk(KERN_INFO "\nB2R2 not idle. Resetting B2R2...\n");
		      b2r2_hw_reset();
		      mdelay(500);
	      }
	      printk(KERN_INFO "B2R2 reset done.\n");
      }

#ifdef B2R2_ONLY_TOPHALF
      disable_irq(IRQ_B2R2);
#endif

      jobcontext.index[index]--;

	  if(temp1==jobcontext.b2r2_head[index])
	  {
		jobcontext.b2r2_head[index]=temp1->nextjob;
	  }
	  else
	  {
		temp3->nextjob=temp2;
	  }

      dbgprintk("old job is deleted at address 0x%x \n",(unsigned int)temp1);

      kfree(temp1);

      temp1=NULL;

      if (jobcontext.index[index]==0)
	      jobcontext.b2r2_head[index]=NULL;

#ifndef B2R2_ONLY_TOPHALF
      write_unlock_bh(&(jobcontext.lock[index]));
#else
      enable_irq(IRQ_B2R2);
#endif

      up(&jobcontext.context_lock[index]);

       dbgprintk("Returns remove_b2r2_jobqueue \n");

      return;
 }

