/*
* Overview:
*   kernel space interface to AB8500
*
* Copyright (C) 2009 ST Ericsson
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include "ste_audio_io_dev.h"

static const struct file_operations audioio_fops;
static struct class *audioio_class;
static struct cdev audioio_cdev;
static int major;
static struct clk * clk_ptr;

static int ste_audioio_open(struct inode *inode, struct file *filp);
static int ste_audioio_release(struct inode *inode, struct file *filp);
static int ste_audioio_ioctl(struct inode *inode, struct file *file,
					unsigned int cmd, unsigned long arg);
static int ste_audioio_cmd_parser(unsigned int cmd, unsigned long arg);


/*
RETURN VALUE
       Usually, on success zero is returned.  A  few  ioctls  use  the  return
       value as an output parameter and return a nonnegative value on success.
       On error, -1 is returned, and errno is set appropriately.

ERRORS
       EBADF  d is not a valid descriptor.

       EFAULT argp references an inaccessible memory area.

       ENOTTY d is not associated with a character special device.

       ENOTTY The specified request does not apply to the kind of object  that
              the descriptor d references.

       EINVAL Request or argp is not valid.

*/
/**
 * \brief Check IOCTL type, command no and access direction
 * \param  inode  value corresponding to the file descriptor
 * \param file	 value corresponding to the file descriptor
 * \param cmd	IOCTL command code
 * \param arg	Command argument
 * \return 0 on success otherwise negative error code
 */
static int ste_audioio_ioctl(struct inode *inode, struct file *file,
					unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	int err = 0;

	/* Check type and command number */
	if (_IOC_TYPE(cmd) != AUDIOIO_IOC_MAGIC)
		return -ENOTTY;

	/* IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,(void __user *)arg,
								_IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,(void __user *)arg,
								_IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	retval = ste_audioio_cmd_parser(cmd,arg);

	return retval;
}
/**
 * \brief IOCTL call to read the value from AB8500 device
 * \param cmd IOCTL command code
 * \param arg  Command argument
 * \return 0 on success otherwise negative error code
 */

static int process_read_register_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_data_t	*audio_dev_data;

	audio_dev_data = (audioio_data_t *)&cmd_data;

	if (copy_from_user(audio_dev_data, (void __user *)arg,
							sizeof(audioio_data_t)))
		return -EFAULT;

	retval =  ste_audioio_core_api_access_read(audio_dev_data);
	if (0 != retval)
		return retval;

	if (copy_to_user((void __user *)arg, audio_dev_data,
							sizeof(audioio_data_t)))
		return -EFAULT;
	return 0;
}
/**
 * \brief IOCTL call to write the given value to the AB8500 device
 * \param cmd IOCTL command code
 * \param arg  Command argument
 * \return 0 on success otherwise negative error code
 */

static int process_write_register_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_data_t	*audio_dev_data;

	audio_dev_data = (audioio_data_t *)&cmd_data;

	if (copy_from_user(audio_dev_data, (void __user *)arg,
						sizeof(audioio_data_t)))
		return  -EFAULT;

	retval = ste_audioio_core_api_access_write(audio_dev_data);

	return retval;
}
/**
 * \brief IOCTL call to control the power on/off of hardware components
 * \param cmd IOCTL command code
 * \param arg  Command argument
 * \return 0 on success otherwise negative error code
 */

static int process_pwr_ctrl_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_pwr_ctrl_t *audio_pwr_ctrl;

	audio_pwr_ctrl=(audioio_pwr_ctrl_t *)&cmd_data;

	if (copy_from_user(audio_pwr_ctrl, (void __user *)arg,
						sizeof(audioio_pwr_ctrl_t)))
		return -EFAULT;

	retval = ste_audioio_core_api_power_control_transducer(audio_pwr_ctrl);

	return retval;
}

static int process_pwr_sts_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_pwr_ctrl_t *audio_pwr_sts;

	audio_pwr_sts=(audioio_pwr_ctrl_t *)&cmd_data;

	if (copy_from_user(audio_pwr_sts, (void __user *)arg,
						sizeof(audioio_pwr_ctrl_t)))
		return  -EFAULT;

	retval = ste_audioio_core_api_power_status_transducer(audio_pwr_sts);
	if (0 != retval)
		return retval;

	if (copy_to_user((void __user *)arg, audio_pwr_sts,
						sizeof(audioio_pwr_ctrl_t)))
		return -EFAULT;

	return 0;
}

static int process_lp_ctrl_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_loop_ctrl_t *audio_lp_ctrl;

	audio_lp_ctrl=(audioio_loop_ctrl_t *)&cmd_data;

	if (copy_from_user(audio_lp_ctrl, (void __user *)arg,
						sizeof(audioio_loop_ctrl_t)))
		return -EFAULT;

	retval =  ste_audioio_core_api_loop_control(audio_lp_ctrl);

	return retval;
}

static int process_lp_sts_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_loop_ctrl_t *audio_lp_sts;

	audio_lp_sts=(audioio_loop_ctrl_t *)&cmd_data;


	if (copy_from_user(audio_lp_sts, (void __user *)arg,
						sizeof(audioio_loop_ctrl_t)))
		return  -EFAULT;

	retval = ste_audioio_core_api_loop_status(audio_lp_sts);
	if (0 != retval)
		return retval;

	if (copy_to_user((void __user *)arg, audio_lp_sts,
						sizeof(audioio_loop_ctrl_t)))
		return -EFAULT;
	return 0;
}

static int process_get_trnsdr_gain_capability_cmd(unsigned int cmd,
							unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_get_gain_t* audio_trnsdr_gain;

	audio_trnsdr_gain=(audioio_get_gain_t *)&cmd_data;

	if (copy_from_user(audio_trnsdr_gain, (void __user *)arg,
						sizeof(audioio_get_gain_t)))
		return -EFAULT;

	retval = ste_audioio_core_api_get_transducer_gain_capability(
							audio_trnsdr_gain);
	if (0 != retval)
		return retval;

	if (copy_to_user((void __user *)arg, audio_trnsdr_gain,
						sizeof(audioio_get_gain_t)))
		return -EFAULT;
	return 0;
}

static int process_gain_cap_loop_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_gain_loop_t* audio_gain_loop;

	audio_gain_loop=(audioio_gain_loop_t *)&cmd_data;

	if (copy_from_user(audio_gain_loop, (void __user *)arg,
						sizeof(audioio_gain_loop_t)))
		return  -EFAULT;

	retval=ste_audioio_core_api_gain_capabilities_loop(audio_gain_loop);
	if (0 != retval)
		return retval;

	if (copy_to_user((void __user *)arg, audio_gain_loop,
						sizeof(audioio_gain_loop_t)))
		return -EFAULT;
	return 0;
}


static int process_support_loop_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_support_loop_t *audio_spprt_loop;

	audio_spprt_loop=(audioio_support_loop_t *)&cmd_data;

	if (copy_from_user(audio_spprt_loop, (void __user *)arg,
						sizeof(audioio_support_loop_t)))
		return  -EFAULT;

	retval=ste_audioio_core_api_supported_loops(audio_spprt_loop);
	if (0 != retval)
		return retval;

	if (copy_to_user((void __user *)arg, audio_spprt_loop,
						sizeof(audioio_support_loop_t)))
		return -EFAULT;
	return 0;
}


static int process_gain_desc_trnsdr_cmd(unsigned int cmd, unsigned long arg)

{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_gain_desc_trnsdr_t *audio_gain_desc;

	audio_gain_desc = (audioio_gain_desc_trnsdr_t *)&cmd_data;

	if (copy_from_user(audio_gain_desc, (void __user *)arg,
					sizeof(audioio_gain_desc_trnsdr_t)))
		return  -EFAULT;

	retval=ste_audioio_core_api_gain_descriptor_transducer(audio_gain_desc);
	if (0 != retval)
		return retval;

	if (copy_to_user((void __user *)arg, audio_gain_desc,
					sizeof(audioio_gain_desc_trnsdr_t)))
		return -EFAULT;
	return 0;
}


static int process_gain_ctrl_trnsdr_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_gain_ctrl_trnsdr_t *audio_gain_ctrl;

	audio_gain_ctrl=(audioio_gain_ctrl_trnsdr_t *)&cmd_data;

	if (copy_from_user(audio_gain_ctrl, (void __user *)arg,
					sizeof(audioio_gain_ctrl_trnsdr_t)))
		return -EFAULT;

	retval = ste_audioio_core_api_gain_control_transducer(audio_gain_ctrl);

	return retval;
}

static int process_gain_query_trnsdr_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_gain_ctrl_trnsdr_t *audio_gain_query;

	audio_gain_query = (audioio_gain_ctrl_trnsdr_t *)&cmd_data;

	if (copy_from_user(audio_gain_query, (void __user *)arg,
					sizeof(audioio_gain_ctrl_trnsdr_t)))
		return -EFAULT;

	retval=ste_audioio_core_api_gain_query_transducer(audio_gain_query);
	if (0 != retval)
		return retval;

	if (copy_to_user((void __user *)arg, audio_gain_query,
					sizeof(audioio_gain_ctrl_trnsdr_t)))
		return -EFAULT;
	return 0;
}

static int process_mute_ctrl_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_mute_trnsdr_t *audio_mute_ctrl;

	audio_mute_ctrl = (audioio_mute_trnsdr_t *)&cmd_data;

	if (copy_from_user(audio_mute_ctrl , (void __user *)arg,
						sizeof(audioio_mute_trnsdr_t)))
		return -EFAULT;

	retval = ste_audioio_core_api_mute_control_transducer(audio_mute_ctrl);

	return retval;
}

static int process_mute_sts_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_mute_trnsdr_t *audio_mute_sts;

	audio_mute_sts = (audioio_mute_trnsdr_t *)&cmd_data;

	if (copy_from_user(audio_mute_sts, (void __user *)arg,
						sizeof(audioio_mute_trnsdr_t)))
		return -EFAULT;

	retval=ste_audioio_core_api_mute_status_transducer(audio_mute_sts);
	if (0 != retval)
		return retval;

	if (copy_to_user((void __user *)arg, audio_mute_sts,
						sizeof(audioio_mute_trnsdr_t)))
		return-EFAULT;
	return 0;
}

static int process_fade_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_fade_ctrl_t *audio_fade;

	audio_fade = (audioio_fade_ctrl_t *)&cmd_data;

	if (copy_from_user(audio_fade , (void __user *)arg,
						sizeof(audioio_fade_ctrl_t)))
		return -EFAULT;

	retval =  ste_audioio_core_api_fading_control(audio_fade);

	return retval;
}

static int process_burst_ctrl_cmd(unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	audioio_cmd_data_t cmd_data;
	audioio_burst_ctrl_t *audio_burst;

	audio_burst = (audioio_burst_ctrl_t  *)&cmd_data;

	if (copy_from_user(audio_burst , (void __user *)arg,
						sizeof(audioio_burst_ctrl_t)))
		return -EFAULT;

	retval = ste_audioio_core_api_burstmode_control(audio_burst);

	return retval;

	return 0;
}


static int ste_audioio_cmd_parser(unsigned int cmd, unsigned long arg)
{
    int retval = 0;

	switch (cmd) {
	case AUDIOIO_READ_REGISTER:
		retval = process_read_register_cmd(cmd,arg);
	break;

	case AUDIOIO_WRITE_REGISTER:
		retval = process_write_register_cmd(cmd,arg);
	break;

	case AUDIOIO_PWR_CTRL_TRNSDR:
		retval=process_pwr_ctrl_cmd(cmd,arg);
	break;

	case AUDIOIO_PWR_STS_TRNSDR:
		retval=process_pwr_sts_cmd(cmd,arg);
	break;

	case AUDIOIO_LOOP_CTRL:
		 retval=process_lp_ctrl_cmd(cmd,arg);
	break;

	case AUDIOIO_LOOP_STS:
		 retval=process_lp_sts_cmd(cmd,arg);
	break;

	case AUDIOIO_GET_TRNSDR_GAIN_CAPABILITY:
		retval=process_get_trnsdr_gain_capability_cmd(cmd,arg);
	break;

	case AUDIOIO_GAIN_CAP_LOOP:
		retval=process_gain_cap_loop_cmd(cmd,arg);
	break;

	case AUDIOIO_SUPPORT_LOOP:
		retval=process_support_loop_cmd(cmd,arg);
	break;

	case AUDIOIO_GAIN_DESC_TRNSDR:
		retval=process_gain_desc_trnsdr_cmd(cmd,arg);
	break;

	case AUDIOIO_GAIN_CTRL_TRNSDR:
		retval=process_gain_ctrl_trnsdr_cmd(cmd,arg);
	break;

	case AUDIOIO_GAIN_QUERY_TRNSDR :
		retval=process_gain_query_trnsdr_cmd(cmd,arg);
	break;

	case AUDIOIO_MUTE_CTRL_TRNSDR:
		retval=process_mute_ctrl_cmd(cmd,arg);
	break;

	case AUDIOIO_MUTE_STS_TRNSDR:
		retval=process_mute_sts_cmd(cmd,arg);
	break;

	case AUDIOIO_FADE_CTRL:
		retval=process_fade_cmd(cmd,arg);
	break;

	case AUDIOIO_BURST_CTRL:
		retval=process_burst_ctrl_cmd(cmd,arg);
	break;
	}
	return retval;
}


static int ste_audioio_open(struct inode *inode, struct file *filp)
{
	if (!try_module_get(THIS_MODULE))
		return -ENODEV;
	module_put(THIS_MODULE);

	clk_ptr = clk_get_sys("msp1", NULL);
	clk_enable(clk_ptr);

	stm_gpio_altfuncenable(GPIO_ALT_MSP_1);

	ste_audioio_core_api_powerup_audiocodec();

	return 0;
}

static int ste_audioio_release(struct inode *inode, struct file *filp)
{
	clk_disable(clk_ptr);
	clk_put(clk_ptr);

	stm_gpio_altfuncdisable(GPIO_ALT_MSP_1);

	/* power down acodec TBD */
	module_put(THIS_MODULE);
	return 0;
}

static const struct file_operations audioio_fops = {
	.owner   = THIS_MODULE,
	.ioctl   = ste_audioio_ioctl,
	.open    = ste_audioio_open,
	.release = ste_audioio_release,
};

static int __init ste_audioio_init(void)
{
	int res;
	dev_t dev;


	printk(KERN_INFO "audioio dev entries driver\n");

	res = alloc_chrdev_region(&dev, 0, 1, "audioio");
	major = MAJOR(dev);
	if (res < 0) {
		printk(KERN_ERR "couldn't allocate region\n");
		goto out;
	}

	cdev_init(&audioio_cdev, &audioio_fops);
	res = cdev_add(&audioio_cdev, dev, 1);
	if (res < 0) {
		printk(KERN_ERR "cdev_add failed with err = %d\n", res);
		goto out;
	}

	audioio_class = class_create(THIS_MODULE, "audioio");
	if (IS_ERR(audioio_class)) {
		res = PTR_ERR(audioio_class);
		goto out_unreg_chrdev;
	}

	printk(KERN_INFO "Audioio-dev module initialized with "
		"major no: %d\n", major);

	ste_audioio_core_api_init_data();

	return 0;

out_unreg_chrdev:
	class_destroy(audioio_class);
out:
	printk(KERN_ERR "%s: Driver initialization failed\n", __FILE__);
	return res;
}
module_init(ste_audioio_init);

static void __exit ste_audioio_exit(void)
{
	ste_audioio_core_api_free_data();
	cdev_del(&audioio_cdev);
	class_destroy(audioio_class);
	unregister_chrdev_region(MKDEV(major, 0), 1);
}
module_exit(ste_audioio_exit);

