/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * ST-Ericsson HDMI driver
 *
 * Author: Per Persson <per.xb.persson@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */

#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <video/hdmi.h>
#include <video/av8100.h>

static int hdmi_open(struct inode *inode, struct file *filp)
{
	int retval = 0;
	return retval;
}

static int hdmi_release(struct inode *inode, struct file *filp)
{
	int retval = 0;
	return retval;
}

static int hdmi_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	int value = 0;
	union av8100_configuration config;
	struct hdmi_register reg;
	struct hdmi_command_register command_reg;
	struct av8100_status status;

	switch (cmd) {
	case IOC_HDMI_POWER:
		/* Get desired power state on or off */
		if (copy_from_user(&value, (void *)arg, sizeof(int)))
			return -EINVAL;

		if (value == 0) {
			if (av8100_powerdown() != AV8100_OK) {
				printk(KERN_ERR "av8100_powerdown FAIL\n");
				return -EINVAL;
			}
		} else {
			if (av8100_powerup() != AV8100_OK) {
				printk(KERN_ERR "av8100_powerup FAIL\n");
				return -EINVAL;
			}
		}
		break;

	case IOC_HDMI_ENABLE_INTERRUPTS:
		if (av8100_enable_interrupt() != AV8100_OK) {
			printk(KERN_ERR "av8100_configuration_get FAIL\n");
			return -EINVAL;
		}
		break;

	case IOC_HDMI_DOWNLOAD_FW:
		if (av8100_download_firmware(NULL, 0, I2C_INTERFACE) !=
			AV8100_OK) {
			printk(KERN_ERR "av8100_configuration_get FAIL\n");
			return -EINVAL;
		}
		break;

	case IOC_HDMI_ONOFF:
		/* Get desired HDMI mode on or off */
		if (copy_from_user(&value, (void *)arg, sizeof(int)))
			return -EFAULT;

		if (av8100_configuration_get(AV8100_COMMAND_HDMI, &config) !=
			AV8100_OK) {
			printk(KERN_ERR "av8100_configuration_get FAIL\n");
			return -EINVAL;
		}
		if (value == 0)
			config.hdmi_format.hdmi_mode = AV8100_HDMI_OFF;
		else
			config.hdmi_format.hdmi_mode = AV8100_HDMI_ON;

		if (av8100_configuration_prepare(AV8100_COMMAND_HDMI, &config)
			!= AV8100_OK) {
			printk(KERN_ERR "av8100_configuration_prepare FAIL\n");
			return -EINVAL;
		}
		if (av8100_configuration_write(AV8100_COMMAND_HDMI, NULL, NULL,
			I2C_INTERFACE) != AV8100_OK) {
			printk(KERN_ERR "av8100_configuration_write FAIL\n");
			return -EINVAL;
		}
		break;

	case IOC_HDMI_REGISTER_WRITE:
		if (copy_from_user(&reg, (void *)arg,
			sizeof(struct hdmi_register))) {
			return -EINVAL;
		}

		if (av8100_register_write(reg.offset, reg.value) !=
			AV8100_OK) {
			printk(KERN_ERR "hdmi_register_write FAIL\n");
			return -EINVAL;
		}
		break;

	case IOC_HDMI_REGISTER_READ:
		if (copy_from_user(&reg, (void *)arg,
			sizeof(struct hdmi_register))) {
			return -EINVAL;
		}

		if (av8100_register_read(reg.offset, &reg.value) !=
			AV8100_OK) {
			printk(KERN_ERR "hdmi_register_write FAIL\n");
			return -EINVAL;
		}

		if (copy_to_user((void *)arg, (void *)&reg,
			sizeof(struct hdmi_register))) {
			return -EINVAL;
		}
		break;

	case IOC_HDMI_STATUS_GET:
		status = av8100_status_get();

		if (copy_to_user((void *)arg, (void *)&status,
			sizeof(struct av8100_status))) {
			return -EINVAL;
		}
		break;

	case IOC_HDMI_CONFIGURATION_WRITE:
		if (copy_from_user(&command_reg, (void *)arg,
			sizeof(struct hdmi_command_register)) != 0) {
			printk(KERN_ERR "IOC_HDMI_CONFIGURATION_WRITE fail 1\n");
			command_reg.return_status =
				HDMI_COMMAND_RETURN_STATUS_FAIL;
		} else {
			if (av8100_configuration_write_raw(command_reg.cmd_id,
					command_reg.buf_len,
					command_reg.buf,
					&(command_reg.buf_len),
					command_reg.buf) != AV8100_OK) {
				printk(KERN_ERR "IOC_HDMI_CONFIGURATION_WRITE fail 1\n");
				command_reg.return_status =
					HDMI_COMMAND_RETURN_STATUS_FAIL;
			}
		}

		if (copy_to_user((void *)arg, (void *)&command_reg,
			sizeof(struct hdmi_command_register)) != 0) {
			return -EINVAL;
		}
		break;

	default:
		break;
	}

	return retval;
}

static const struct file_operations hdmi_fops = {
	.owner =    THIS_MODULE,
	.open =     hdmi_open,
	.release =  hdmi_release,
	.ioctl = hdmi_ioctl
};

static struct miscdevice hdmi_miscdev = {
	HDMI_DRIVER_MINOR_NUMBER,
	"hdmi",
	&hdmi_fops
};

int __init hdmi_init(void)
{
	return misc_register(&hdmi_miscdev);
}

void hdmi_exit(void)
{
	misc_deregister(&hdmi_miscdev);
}
