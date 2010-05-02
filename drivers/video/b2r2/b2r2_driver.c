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

FILE        :  b2r2_driver.c

DESCRIPTION :  Implements B2R2 module init,exit,probe,mmap and various ioctl
               functions.

*/


/** include file */


#include "b2r2.h"


extern struct s_B2R2 * B2R2_System;
t_uint32 *pmu_b2r2_clock;



/** B2R2 Driver file operations */


static int b2r2_open(struct inode *inode, struct file *filp)
{

	b2r2_instance_memory *instance;

	dbgprintk("open.\n");

	instance=(struct b2r2_instance_memory *)kmalloc(sizeof(b2r2_instance_memory),GFP_KERNEL);

	instance->head=NULL;
	instance->memory_counter=0;

    filp->private_data =instance;

    return 0;
}


static int b2r2_release(struct inode *inode, struct file *filp)
{

    int i;
    b2r2_memory   *memory;

    b2r2_instance_memory *instance;

    dbgprintk("release.\n");

    instance=(struct b2r2_instance_memory *)filp->private_data;

    if(instance->memory_counter>0)
    {

		 for(i=0;i<instance->memory_counter;i++)
	 {

           memory=instance->head;

           /** Deallocate DMA memory */
		   dma_free_coherent(NULL,memory->size_of_memory,
           (void*)memory->logical_address,memory->physical_address);

           instance->head=instance->head->next;
           kfree(memory);
           memory=NULL;
	 }

    }

    kfree(instance);

    instance=NULL;

    return 0;
}


int process_memory_allocate(void *element,void *arg)
{

   int ret = 0;

   b2r2_instance_memory *instance;
   b2r2_memory          *memory,*temp = NULL;
   b2r2_driver_memory   *memory_request;


   dbgprintk("entering process_memory_allocate.\n");

   instance=(struct b2r2_instance_memory *)element;

   dbgprintk(" instance memory at address 0x%x.\n",(unsigned int)element);

   memory_request=(struct b2r2_driver_memory *)arg;

   memory=(struct b2r2_memory *)kmalloc(sizeof(b2r2_memory),GFP_KERNEL);

   dbgprintk(" memory allocated at address 0x%x.\n",(unsigned int)memory);

   memory->next=NULL;

   memory->logical_address=(t_uint32)
   dma_alloc_coherent(NULL, memory_request->size_of_memory,&memory_request->physical_address,
   GFP_DMA|GFP_KERNEL);

   if(memory->logical_address==0) return -1;
   if(memory_request->physical_address==0) return -1;

   memory->size_of_memory=memory_request->size_of_memory;

   dbgprintk("physical memory 0x%x.\n",memory_request->physical_address);

   memory->physical_address=memory_request->physical_address;
   instance->present=memory;

   if(temp==NULL)
   {
	   dbgprintk("instance head is null so assign address 0x%x.\n",(unsigned int)memory);

	   instance->head=memory;

	   dbgprintk("after assignment the address at instance head 0x%x.\n",(unsigned int)instance->head);
   }
   else
   {

	 dbgprintk("instance head is not null,so add it at head assign address 0x%x \n",(unsigned int)memory);

     temp=instance->head;
     instance->head=memory;
     instance->head->next=temp;

   }

   instance->memory_counter++;

   dbgprintk("leaving process_memory_allocate.\n");
   return ret;
}


int process_memory_deallocate(void *element,void *arg)
{

   int ret = 0;
   int    i;

   b2r2_instance_memory *instance;
   b2r2_memory          *temp1,*temp2,*temp3;
   b2r2_driver_memory   *memory_request;

   dbgprintk("entering process_memory_deallocate.\n");

   instance=(struct b2r2_instance_memory *)element;

   dbgprintk(" instance memory at address 0x%x.\n",(unsigned int)element);

   memory_request=(struct b2r2_driver_memory *)arg;

   dbgprintk(" memory request at address 0x%x.\n",(unsigned int)memory_request);

   dbgprintk(" memory request physical address 0x%x.\n",(unsigned int)memory_request->physical_address);

   temp1=instance->head;

   dbgprintk(" instance->head 0x%x.\n",(unsigned int)instance->head);

   dbgprintk(" instance->head physical address 0x%x.\n",(unsigned int)temp1->physical_address);

   temp2=temp1->next;
   temp3=NULL;

   for(i=0;i<instance->memory_counter;i++)
   {

     if(temp1->physical_address==memory_request->physical_address)
	 break;

     temp3=temp1;
     temp2=temp1->next;
     temp1=temp2;
     temp2=temp1->next;
   }

   instance->memory_counter--;

   dbgprintk(" going for dma free coherent memory \n");

   if(memory_request->size_of_memory==0) ret=-1;
   if(temp1->logical_address==0) ret=-1;
   if(temp1->physical_address==0) ret=-1;

   /** Deallocate DMA memory */
   dma_free_coherent(NULL,memory_request->size_of_memory,
   (void*)temp1->logical_address,temp1->physical_address);

   kfree(temp1);
   temp1=NULL;

   if(instance->memory_counter==0)
   instance->head=NULL;

   if(temp3==NULL)
   instance->head=temp2;
   else
   temp3->next=temp2;

   dbgprintk("leaving process_memory_deallocate.\n");
   return ret;

}




/**
 * b2r2_mmap - This routine implements mmap
 * @filp: file pointer.
 * @vma	:virtual file pointer.
 */



static int b2r2_mmap(struct file *filp, struct vm_area_struct *vma)
{

	int ret = 0;
	long length = vma->vm_end - vma->vm_start;
	//char memory_area = vma->vm_pgoff;
	b2r2_instance_memory *instance;

	dbgprintk("mmap enter.\n");

	vma->vm_pgoff = 0;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

#if defined(CONFIG_L2CACHE_ENABLE)

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

#endif

	instance=(struct b2r2_instance_memory *)filp->private_data;

    dbgprintk("mmap length %u \n",length);

    dbgprintk("physical memory 0x%x.\n",instance->present->physical_address);

	ret = remap_pfn_range(vma, vma->vm_start,
			(instance->present->physical_address)>>PAGE_SHIFT,
			length, vma->vm_page_prot);

#if defined(CONFIG_L2CACHE_ENABLE)

	flush_cache_range(vma, vma->vm_start, vma->vm_end);
	outer_flush_range(instance->present->physical_address, (instance->present->physical_address + (vma->vm_end - vma->vm_start)));
	//l2x0_flush_range(instance->present->physical_address, (instance->present->physical_address + (vma->vm_end - vma->vm_start)));

#endif

    dbgprintk("mmap return %u \n",ret);

    dbgprintk("mmap exit.\n");

	return ret;

}




/**
 * b2r2_ioctl - This routine implements ioctl
 * @inode: inode pointer.
 * @file: file pointer.
 * @cmd	:ioctl command.
 * @arg: input argument for ioctl.
 */



static int b2r2_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	char 	args[256];
	char	*targ 	= NULL;
	int     error;


	dbgprintk("ioctl.\n");

	b2r2_assert(inode==NULL);
	b2r2_assert(file==NULL);

	/** Create Temparory kernel memory for argument */

	if(_IOC_SIZE(cmd) < 256)
		targ = args;
	else
	targ = vmalloc(B2R2_MAX_IOCTL_SIZE);

	if(!targ)
	return -ENOMEM;


	/** Copy arguments into temparory kernel buffer  */

	switch (_IOC_DIR(cmd))
	{
	case _IOC_NONE:
		break;
	case _IOC_WRITE:
	case (_IOC_WRITE | _IOC_READ):
	case _IOC_READ:

		if (_IOC_SIZE(cmd) > B2R2_MAX_IOCTL_SIZE)
		{
			if(_IOC_SIZE(cmd) >= 256)
				vfree(targ);
			return -EINVAL; /* Arguments are too big. */
		}

		if(copy_from_user(targ, (void *)arg, _IOC_SIZE(cmd))) {
			if(_IOC_SIZE(cmd) >= 256)
				vfree(targ);
			return -EFAULT;
		}
		break;
	}

    /** Process actual ioctl */

    switch(cmd)
    {

     case B2R2_ALLOCATE_MEMORY:
		error=process_memory_allocate(file->private_data,(void *)targ);
		break;
	 case B2R2_DEALLOCATE_MEMORY:
	    error=process_memory_deallocate(file->private_data,(void *)targ);
	    break;
	 default:
	    error=process_ioctl(file->private_data,cmd,(void *)targ);
	    break;

    }


	/**  Copy results back into user buffer  */
	switch (_IOC_DIR(cmd))
	{
	case _IOC_READ:
	case (_IOC_WRITE | _IOC_READ):

		if (copy_to_user((void *)arg, (void *)targ, _IOC_SIZE(cmd)))
		{
			if(_IOC_SIZE(cmd) >= 256)
				vfree(targ);

			return -EFAULT;
		}
		break;
	}

	/*  Handle ioctls not recognized by the driver  */

	if(_IOC_SIZE(cmd) >= 256)
		vfree(targ);

	return error;
}









static struct file_operations b2r2_fops = {


  .owner =    THIS_MODULE,
  .open =     b2r2_open,
  .mmap = b2r2_mmap,
  .release =  b2r2_release,
  .ioctl = b2r2_ioctl
};











DECLARE_MUTEX(b2r2_module_init_exit);

static struct miscdevice b2r2_miscdev =
{
	B2R2_DRIVER_MINOR_NUMBER,
	"b2r2",
	&b2r2_fops
};




/**
 * b2r2_probe - This routine loads b2r2 driver
 * @pdev: platform device.
 *
 * 1)Acquire module init&exit mutex.
 *
 * 2)Enable B2R2 power domain & check it.
 *
 * 3)Register B2R2 as a miscellaneous driver.
 *
 * 4)Init B2R2 hardware.
 *
 * 5)Release module init&exit mutex.
 *
 * 6)Recover from any error without any leaks.
 *
*/

static int __init b2r2_probe(struct platform_device *pdev)
{
  int result=0;
  struct resource *res;
  volatile u32 __iomem *b2r2_clk;


  dbgprintk("init started.\n");

  b2r2_assert(pdev==NULL);

  /** Aquire module init and exit mutex */
  // down_interruptible(&b2r2_module_init_exit);

  /**  b2r2 clock ~ PRCMU */
  b2r2_clk=(u32 *) ioremap(0x801571E4, (0x801571E4+3)-0x801571E4 + 1);
  *b2r2_clk=0xC;
  iounmap(b2r2_clk);

  /** Enable B2R2 power domain & check it.*/
  res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
  if (res == NULL)
	goto b2r2_init_fail2;

  pmu_b2r2_clock = (t_uint32*) ioremap(res->start, res->end - res->start + 1);
  if (pmu_b2r2_clock == NULL)
		goto b2r2_init_fail2;

  //pmu_enable_b2r2 = (t_uint32*)IO_ADDRESS(PRCM_B2R2CLK_MGT_REG);

  *(pmu_b2r2_clock)|=B2R2_CLK_FLAG;

  /** Register b2r2 driver */
  result = misc_register(&b2r2_miscdev);
  if (result) {
		dbgprintk("registering misc device fails\n");
      goto b2r2_init_fail2;
  }

 /** Init B2R2 hardware */
  result = b2r2_init_hw(pdev);
  if (result < 0) {
    dbgprintk("can't be initilized 0x%x\n",result);
    dbgprintk("init done fail at b2r2_init_fail3 0x%x.\n",result);
    goto b2r2_init_fail3;
  }

  /** Release module init and exit mutex */
  //up(&b2r2_module_init_exit);

  dbgprintk("init done.\n");

  return result;




/** Recover from any error if something fails */

b2r2_init_fail3:

  /** Unregister B2R2 driver */
  misc_deregister(&b2r2_miscdev);

b2r2_init_fail2:

  /**
    Check if B2R2 power domain is disabled
    if not disabled then disable it.
  */

  (*pmu_b2r2_clock)|=~B2R2_CLK_FLAG;

    /** Release module init and exit mutex */
  //up(&b2r2_module_init_exit);

  dbgprintk("init done with errors.\n");

  return result;

}



/**
 * b2r2_remove - This routine unloads b2r2 driver
 * @pdev: platform device.
 *
 * 1)Acquire module init and exit mutex.
 *
 * 2)Exit B2R2 hardware.
 *
 * 3)De-register B2R2 Driver.
 *
 * 4)  Disable B2R2 power domain & check it.
 *
 * 5)  Release module init and exit mutex.
 *
 *
 *
 */




static int b2r2_remove(struct platform_device *pdev)
{

  //t_uint32 *pmu_disable_b2r2;

  dbgprintk("exiting started..\n");

  b2r2_assert(pdev==NULL);

  /** Aquire module init and exit mutex */

  //down_interruptible(&b2r2_module_init_exit);

  /** Exit B2R2 hardware */
  b2r2_exit_hw();

  /** Unregister B2R2 driver */
  misc_deregister(&b2r2_miscdev);

  /**
    Check if B2R2 power domain is disabled
    if not disabled then disable it.
  */

  (*pmu_b2r2_clock)|= ~B2R2_CLK_FLAG;

  /** Release module init and exit mutex */
  //up(&b2r2_module_init_exit);

  dbgprintk("exiting end..\n");

  return 0;

}


#ifdef CONFIG_U8500_PM

//extern int g_u8500_sleep_mode;

/** B2R2 Driver suspend

   1)Check if b2r2 hardware is in idle
     status to enter into suspend status

   2)In normal suspend case, it is enough to
     disable the clock and in deep-sleep case
     ,no extra opertion required
*/

int u8500_b2r2_suspend(struct platform_device *dev, pm_message_t state)
{


    //t_uint32 *pmu_disable_b2r2;

    b2r2_assert(dev==NULL);

    /** Check if b2r2 hardware is in idle
        status to enter into suspend status */
    if ((B2R2_System->BLT_STA1 & B2R2BLT_STA1BDISP_IDLE) != 0x0)
    return -EBUSY;

    /** In normal suspend case, it is enough to
        disable the clock and in deep-sleep case
        ,no extra opertion required */

   (*pmu_b2r2_clock) |=~B2R2_CLK_FLAG;


	return 0;


    }


/**
    B2R2 Driver resume

    1)In wakeup,we need to enable the clock

    2)In case of deep sleep,we have to reset the b2r2
      hardware

*/


int u8500_b2r2_resume(struct platform_device *dev)
{

   int result=0;
   t_uint32 *pmu_enable_b2r2;

   b2r2_assert(dev==NULL);

   /** In wakeup,we need to enable the clock */

  (*pmu_b2r2_clock)=B2R2_CLK_FLAG;

   /** In case of deep sleep,we have to reset the b2r2
       hardware */
   //if ( g_u8500_sleep_mode == DEEP_SLEEP )
  {

   result=b2r2_hw_reset();

   }

   return result;

}

#endif


static struct platform_driver platform_b2r2_driver = {
	.probe	= b2r2_probe,
	.remove = b2r2_remove,
	.driver = {
		.name	= "U8500-B2R2",
	},

#ifdef CONFIG_U8500_PM

        /* TODO implement power mgmt functions */
        .suspend = u8500_b2r2_suspend,
        .resume = u8500_b2r2_resume,
#endif
};


static int __init b2r2_init(void)
{
	dbgprintk("b2r2_init called \n");
	return platform_driver_register(&platform_b2r2_driver);
}

module_init(b2r2_init);

static void __exit b2r2_exit(void)
{
	dbgprintk("b2r2_exit called \n");
	platform_driver_unregister(&platform_b2r2_driver);
	return;
}

module_exit(b2r2_exit);


/** Module is having GPL license */

MODULE_LICENSE("GPL");

/** Module author & discription */

MODULE_AUTHOR("Preetham Rao K (preetham.rao@stericsson.com)");
MODULE_DESCRIPTION("Loadble driver for B2R2");
