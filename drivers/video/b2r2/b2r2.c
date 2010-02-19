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

FILE        :  b2r2.c

DESCRIPTION :  Implements  interrupt handling (top and bottom half) and
               hardware init & reset features.

*/


/** B2R2 register defines */

#include "b2r2.h"

/** B2R2 register structure */
volatile struct s_B2R2 * B2R2_System=NULL;
struct          b2r2_jobcontext   jobcontext;


/** Interrupt handler top-half and bottom-half synchronization mechanism
    Use this mechanism to decrease interrupt latency */

__u8 writeIndex = 0;
__u8 readIndex = 0;
__u8 no_of_interrupts;

t_uint32 b2r2_interrupt_array[B2R2_MAX_INTERRUPTS];
t_uint32 b2r2_interrupt_source[B2R2_INTERRUPT_SOURCES];

/** Spin lock and tasklet declaration */
void u8500_b2r2_tasklet(unsigned long);
DECLARE_TASKLET(b2r2_tasklet, u8500_b2r2_tasklet, 0);
spinlock_t irq_lock = SPIN_LOCK_UNLOCKED;

/** b2r2 event */

b2r2_event_list b2r2_event;


void  clear_interrupts(void)
{

    B2R2_System->BLT_ITM0 = 0x0;
    B2R2_System->BLT_ITM1 = 0x0;
    B2R2_System->BLT_ITM2 = 0x0;
    B2R2_System->BLT_ITM3 = 0x0;
}


void  save_disable_interrupts(void)
{

   b2r2_interrupt_source[0]=B2R2_System->BLT_ITM0;
   b2r2_interrupt_source[1]=B2R2_System->BLT_ITM1;
   b2r2_interrupt_source[2]=B2R2_System->BLT_ITM2;
   b2r2_interrupt_source[3]=B2R2_System->BLT_ITM3;

   B2R2_System->BLT_ITM0 = 0x0;
   B2R2_System->BLT_ITM1 = 0x0;
   B2R2_System->BLT_ITM2 = 0x0;
   B2R2_System->BLT_ITM3 = 0x0;
}


void  restore_enable_interrupts(void)
{

    B2R2_System->BLT_ITM0 = b2r2_interrupt_source[0];
    B2R2_System->BLT_ITM1 = b2r2_interrupt_source[1];
    B2R2_System->BLT_ITM2 = b2r2_interrupt_source[2];
    B2R2_System->BLT_ITM3 = b2r2_interrupt_source[3];
}




int b2r2_hw_reset(void)
{

  u32 uTimeOut =B2R2_DRIVER_TIMEOUT_VALUE;

  B2R2_System->BLT_CTL |= B2R2BLT_CTLGLOBAL_soft_reset ;
  B2R2_System->BLT_CTL = 0x00000000 ;


  dbgprintk("wait for B2R2 to be idle..\n");

  /** Wait for B2R2 to be idle (on a timeout rather than while loop) */

  while ((uTimeOut > 0) && ((B2R2_System->BLT_STA1 & B2R2BLT_STA1BDISP_IDLE) == 0x0)) uTimeOut--;
  if(uTimeOut == 0)
  {
	dbgprintk("error-> after software reset B2R2 is not idle\n");
	return -EAGAIN;
  }

   return 0;

}


void process_events(t_uint32 status)
{


   int       i,index;
   b2r2_job  *temp1,*temp2,*temp3;

   dbgprintk("Enters process_events \n");
   dbgprintk("status 0x%x \n",status);

    switch(status)
    {
      /** CQ1 event */
      B2R2_CQ1_INTERRUPT_CONTEXT:
      {
		index=0;
        break;
      }

      /** CQ2 event */
      B2R2_CQ2_INTERRUPT_CONTEXT:
      {
        index=1;
        break;
      }

      /** AQ1 event */
      B2R2_AQ1_INTERRUPT_CONTEXT:
      {
        index=2;
        break;
      }

      /** AQ2 event */
      B2R2_AQ2_INTERRUPT_CONTEXT:
      {
        index=3;
        break;
      }

      /** AQ3 event */
      B2R2_AQ3_INTERRUPT_CONTEXT:
      {
        index=4;
        break;
      }

      /** AQ4 event */
      B2R2_AQ4_INTERRUPT_CONTEXT:
      {
        index=5;
        break;
      }


      /** Handle error case */
      B2R2_ERROR_INTERRUPT_CONTEXT:
      {
		 index=6;
		 break;

      }

      default:
      {
	  index=-1;
	  return;
      }



     } /** end of switch case */


     dbgprintk("b2r2 context_index 0x%x \n",index);

     temp1=jobcontext.b2r2_head[index];

     temp3=NULL;

     for(i=0;i<jobcontext.index[index];i++)
     {
		if(temp1->job.job_done==0)
		{
		   temp1->job.job_done=1;

		   dbgprintk("wake_up_interruptible is called \n");

	       dbgprintk("tasklet job status jobid 0x%x jobdone 0x%x \n",
	                 temp1->job.jobid,
	                 temp1->job.job_done);

           wake_up_interruptible(&temp1->event);

           if(temp3!=NULL)
           {

              /** Anyway,there is no need to to check this condition as
                  some job is pending if not null */
              if(temp3->job.job_done == 0)
              {
				switch(temp3->job.interrupt_target)
				  {
				  case 0:  /** ARM */
				    B2R2_System->BLT_ITM0 |= temp3->job.interrupt_context;
				    break;
				  case 1:  /** SAA */
				    B2R2_System->BLT_ITM1 |= temp3->job.interrupt_context;
				    break;
				  case 2:  /** SVA */
				    B2R2_System->BLT_ITM2 |= temp3->job.interrupt_context;
				    break;
				  case 3:  /** SIA */
				    B2R2_System->BLT_ITM3 |= temp3->job.interrupt_context;
				    break;
				  }

				trigger_job(&(temp3->job));
		      }
	       }
           break;

	    }
	    else
	    {
		  temp3=temp1;
		  temp2=temp1->nextjob;
		  temp1=temp2;

	    }

     }


     B2R2_System->BLT_ITS=B2R2_System->BLT_ITS;



#if 0

//while(!(B2R2_System->BLT_STA1 & 0x1));

#ifdef B2R2_RESET_WORKAROUND

     b2r2_hw_reset();

#endif
#endif

     dbgprintk("Returns process_events \n");

}


/** Interrupt handler */
irqreturn_t b2r2_tophalf(int irq, void *x)
{
    dbgprintk("interrupt hit hit hit..\n");

    /** Interprocess locking */
    spin_lock(&irq_lock);

    /** clear b2r2 interrupts to avoid re-enter interrupt handler */
    clear_interrupts();

#ifndef B2R2_ONLY_TOPHALF

    /** Save the context to array */
    b2r2_interrupt_array[writeIndex]=B2R2_System->BLT_ITS;

    B2R2_System->BLT_ITS &=(~B2R2BLT_ITSMask);

    /** Increse write index and check for overflow */

	writeIndex = ((writeIndex + 1)%B2R2_MAX_INTERRUPTS);
	no_of_interrupts++;

		if(no_of_interrupts > (B2R2_MAX_INTERRUPTS - 1))
		dbgprintk("Error:Overflow in interrupt handler\n");

    /** Schedule the tasklet */
    tasklet_schedule(&b2r2_tasklet);

#else

    process_events(B2R2_System->BLT_ITS);

#endif

    /** Interprocess locking */
    spin_unlock(&irq_lock);

    return IRQ_HANDLED;

}


void u8500_b2r2_tasklet(unsigned long tasklet_data)
{

	unsigned int count;
	unsigned long flags;
	t_uint32 irq_status;


	dbgprintk("Enters u8500_b2r2_tasklet\n");

    //save_disable_interrupts();

    /** Disable interrupts while handling counter */
	spin_lock_irqsave(&irq_lock, flags);
	count = no_of_interrupts;
	no_of_interrupts = 0;
	spin_unlock_irqrestore(&irq_lock, flags);

   /** Process counter number of interrupts */

    while(count--) {


			spin_lock_irqsave(&irq_lock, flags);
			irq_status = b2r2_interrupt_array[readIndex];
			spin_unlock_irqrestore(&irq_lock, flags);
            readIndex = ((readIndex + 1) % B2R2_MAX_INTERRUPTS);
            /** Process corresponding interrupt using case statement */
            process_events(irq_status);

   }

   //restore_enable_interrupts();

   dbgprintk("Returns u8500_b2r2_tasklet\n");
}




/**

B2R2 Hardware reset & initilize

	1)Register interrupt handler

	2)B2R2 Register map

	3)For resetting B2R2 hardware,write to B2R2 Control register the
	  B2R2BLT_CTLGLOBAL_soft_reset and then polling for on
	  B2R2 status register for B2R2BLT_STA1BDISP_IDLE flag.

	4)Wait for B2R2 hardware to be idle (on a timeout rather than while loop)

	5)Driver status reset

	6)Recover from any error without any leaks.

*/



int  b2r2_init_hw(struct platform_device *pdev)
{
  int result=0;
  struct resource *res;
  u32 uTimeOut =B2R2_DRIVER_TIMEOUT_VALUE;

  dbgprintk("b2r2_init_hw started..\n");

  dbgprintk("register interrupt handler..\n");

  /**

  IRQ_B2R2 is defined according to target configured.

  */
  result = request_irq(IRQ_B2R2, b2r2_tophalf, 0, "b2r2-interrupt",0);

	if (result) {

		dbgprintk("failed to register IRQ for B2R2 ,freeing...\n");
		dbgprintk("b2r2_init_hw fail at at b2r2_init_hw_fail1 0x%x.\n",result);
		goto b2r2_init_hw_fail1;
	}


  dbgprintk("  map b2r2 registers..\n");

  /** B2R2 Register map */
  res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (res == NULL)
	goto b2r2_init_hw_fail2;

  B2R2_System = (struct s_B2R2 *) ioremap(res->start, res->end - res->start + 1);
  if (B2R2_System==NULL) {

    dbgprintk("ioremap failed\n");
    result=-ENOMEM;
    dbgprintk("b2r2_init_hw fail at at b2r2_init_hw_fail2 0x%x.\n",result);
    goto b2r2_init_hw_fail2;
  }

  dbgprintk("b2r2 structure address %u \n",(unsigned int)&B2R2_System);



  dbgprintk("do a global reset..\n");

  /**
      1)Do a global reset
      2)No Step by Step mode
  */
  B2R2_System->BLT_CTL |= B2R2BLT_CTLGLOBAL_soft_reset ;
  B2R2_System->BLT_CTL = 0x00000000 ;


  dbgprintk("wait for B2R2 to be idle..\n");

  /** Wait for B2R2 to be idle (on a timeout rather than while loop) */

  while ((uTimeOut > 0) && ((B2R2_System->BLT_STA1 & B2R2BLT_STA1BDISP_IDLE) == 0x0)) uTimeOut--;
  if(uTimeOut == 0)
  {
	dbgprintk("error-> after software reset B2R2 is not idle\n");
	result=-EAGAIN;
	dbgprintk("b2r2_init_hw fail at at b2r2_init_hw_fail3 0x%x.\n",result);
	goto b2r2_init_hw_fail3;
  }

  dbgprintk("b2r2_init_hw ended..\n");


  /** Driver status reset*/

  init_b2r2_jobquqeue();

  return result;

/** Recover from any error without any leaks */

	b2r2_init_hw_fail3:

	b2r2_init_hw_fail2:

    /** Unmap B2R2 registers */

	if (B2R2_System) iounmap(B2R2_System);

	B2R2_System=NULL;

	b2r2_init_hw_fail1:

    /** Free B2R2 interrupt handler */

	free_irq(IRQ_B2R2,0);

	return result;

}


/**

B2R2 Hardware exit

	1)Unmap B2R2 registers

	2)Free B2R2 interrupt handler

*/


void  b2r2_exit_hw(void)
{

	dbgprintk("b2r2_exit_hw started..\n");

	dbgprintk("unmap b2r2 registers..\n");

	/** Unmap B2R2 registers */

	if (B2R2_System) iounmap(B2R2_System);

	B2R2_System=NULL;

	dbgprintk("free interrupt handler..\n");

    /** Free B2R2 interrupt handler */

	free_irq(IRQ_B2R2,0);

	dbgprintk("b2r2_exit_hw ended..\n");

	exit_b2r2_jobqueue();


}




int process_ioctl(void *element, unsigned int cmd, void *arg)
{


	struct job_context *b2r2 =(struct job_context* )arg;

	if(!b2r2)
		return -EINVAL;


    switch(cmd)
    {

         case B2R2_QUEUE_JOB:
		{
            add_b2r2_jobqueue(b2r2);
		  break;
		}


	    case B2R2_DEQUEUE_JOB:
		{
            remove_b2r2_jobqueue(b2r2);
          break;
	    }

        /** Handle the default case */
        default:
        break;

    }/* end switch */

    return 0;

}













