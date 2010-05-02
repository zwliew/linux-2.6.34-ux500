/*
 * sitw4500-dev.c - simple userspace interface to ab8500
 *
 * Copyright (C) 2009 STEricsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/cdev.h>

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <asm/types.h>
#include <asm/dma.h>
#include <mach/dma.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-stm.h>
#include <linux/spi/stm_spi023.h>
#include <linux/spi/spidev.h>

#include <asm/uaccess.h>
#include <mach/ab8500.h>
#include <mach/ab8500-dev.h>

MODULE_AUTHOR("kasagar");
MODULE_DESCRIPTION("User mode ab8500 device interface");
MODULE_LICENSE("GPL");

static const struct file_operations ab8500dev_fops;
static struct class *ab8500dev_class;
static struct cdev ab8500_cdev;
static int major=0;

static int
ab8500dev_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	int 			retval = 0;
	int			err = 0;
	unsigned char		data;
	struct ab8500dev_data	*ab8500dev;

	ab8500dev = kmalloc(sizeof(struct ab8500dev_data),GFP_KERNEL);
	if(!ab8500dev)	{
		retval = -ENOMEM;
		return retval;
	}

	/* Check type and command number */
        if (_IOC_TYPE(cmd) != AB8500_IOC_MAGIC) {
                retval =  -ENOTTY;
                goto clean_up;
        }

        /* Check access direction once here; don't repeat below.
         * IOC_DIR is from the user perspective, while access_ok is
         * from the kernel perspective; so they look reversed.
         */
        if (_IOC_DIR(cmd) & _IOC_READ)
                err = !access_ok(VERIFY_WRITE,
                                (void __user *)arg, _IOC_SIZE(cmd));
        if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
                err = !access_ok(VERIFY_READ,
                                (void __user *)arg, _IOC_SIZE(cmd));
        if (err) {
                retval = -EFAULT;
                goto clean_up;
        }


	switch (cmd)	{

	case AB8500_GET_REGISTER:
		{
		if(copy_from_user(ab8500dev, (void __user *)arg,
			sizeof(struct ab8500dev_data))) {
				retval =  -EFAULT;
				goto clean_up;
			}
		/*printk("block = %x\t addr=%x\t\n", ab8500dev->block, ab8500dev->addr);*/
		data = ab8500_read(ab8500dev->block,
				ab8500dev->addr);
		ab8500dev->data = data;
		/*printk("in kernel: bank = %x addr=%x data=%x\n", ab8500dev->block, ab8500dev->addr, ab8500dev->data);*/
		if (copy_to_user((void __user *)arg, ab8500dev, sizeof(struct ab8500dev_data))) {
			retval = -EFAULT;
			goto clean_up;
			}
		break;
		}


	case AB8500_SET_REGISTER:
		{
		if(copy_from_user(ab8500dev, (void __user *)arg,
			sizeof(struct ab8500dev_data))) {
				retval = -EFAULT;
				goto clean_up;
			}
		retval = ab8500_write(ab8500dev->block,ab8500dev->addr,
				ab8500dev->data);
		break;
		}
	}
clean_up:
	kfree(ab8500dev);
	return retval;
}

static int ab8500dev_open(struct inode *inode, struct file *filp)
{
	if (!try_module_get(THIS_MODULE))
		return -ENODEV;
	/*TODO*/
	/*filp->private_data = &ab8500dev_fops;*/
	module_put(THIS_MODULE);

	/*printk("AB8500 Module opened\n");*/

	return 0;
}

static int ab8500dev_release(struct inode *inode, struct file *filp)
{
	module_put(THIS_MODULE);
	return 0;
}

static const struct file_operations ab8500dev_fops = {
        .owner   = THIS_MODULE,
	.ioctl   = ab8500dev_ioctl,
        .open    = ab8500dev_open,
        .release = ab8500dev_release,
};

static int __init ab8500_dev_init(void)
{
	int res;
	dev_t dev;

	printk(KERN_INFO "ab8500 /dev entries driver\n");

	res = alloc_chrdev_region(&dev,0,1,"ab8500");
	major = MAJOR(dev);
	if(res < 0)	{
		printk("couldn't allocate region\n");
		goto out;
	}

	cdev_init(&ab8500_cdev,&ab8500dev_fops);
	res = cdev_add(&ab8500_cdev,dev,1);
	if(res < 0)	{
		printk("cdev_add failed with err = %d\n", res);
		goto out;
	}

	ab8500dev_class = class_create(THIS_MODULE, "ab8500dev");
	if (IS_ERR(ab8500dev_class)) {
		res = PTR_ERR(ab8500dev_class);
		goto out_unreg_chrdev;
        }
	printk(KERN_INFO "AB8500-dev module initialized with major no: %d\n",major);

	return 0;
out_unreg_chrdev:
	class_destroy(ab8500dev_class);
out:
	printk(KERN_ERR "%s: Driver initialization failed\n", __FILE__);
	return res;
}
module_init(ab8500_dev_init);

static void __exit ab8500_dev_exit(void)
{
	cdev_del(&ab8500_cdev);
	class_destroy(ab8500dev_class);
	unregister_chrdev_region(MKDEV(major,0),1);
}
module_exit(ab8500_dev_exit);
