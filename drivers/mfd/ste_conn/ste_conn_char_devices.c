/*
 * file ste_conn_char_devices.c
 *
 * Copyright (C) ST-Ericsson AB 2010
 *
 * Linux Bluetooth HCI H:4 Driver for ST-Ericsson Connectivity Controller.
 * License terms: GNU General Public License (GPL), version 2
 *
 * Authors:
 * Pär-Gunnar Hjälmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Henrik Possung (henrik.possung@stericsson.com) for ST-Ericsson.
 * Josef Kindberg (josef.kindberg@stericsson.com) for ST-Ericsson.
 * Dariusz Szymszak (dariusz.xd.szymczak@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/skbuff.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/sched.h>

#include <linux/mfd/ste_conn.h>
#include <mach/ste_conn_devices.h>
#include "ste_conn_cpd.h"
#include "ste_conn_ccd.h"
#include "ste_conn_debug.h"

#define VERSION "1.0"

/* Ioctls */
#define STE_CONN_CHAR_DEV_IOCTL_RESET		_IOW('U', 210, int)
#define STE_CONN_CHAR_DEV_IOCTL_CHECK4RESET	_IOR('U', 212, int)

#define STE_CONN_CHAR_DEV_IOCTL_EVENT_RESET  1
#define STE_CONN_CHAR_DEV_IOCTL_EVENT_CLOSED 2

/* ------------------ Internal type definitions --------------------------- */

/**
  * enum ste_conn_char_reset_state - Reset state.
  * @STE_CONN_CHAR_IDLE:	Idle state.
  * @STE_CONN_CHAR_RESET:	Reset state.
  * @STE_CONN_CHAR_WAIT4RESET:	Wait for reset state.
  */
enum ste_conn_char_reset_state {
	STE_CONN_CHAR_IDLE,
	STE_CONN_CHAR_RESET,
	STE_CONN_CHAR_WAIT4RESET
};

/**
  * struct ste_conn_char_dev_user - Stores device information.
  * @cpd_dev:		Registered CPD device.
  * @cdev_device:	Registered device struct.
  * @cdev:		Registered Char Device.
  * @devt:		Assigned dev_t.
  * @name:		Name of device.
  * @rx_queue:		Data queue.
  * @rx_wait_queue:	Wait queue.
  * @reset_wait_queue:	Reset Wait queue.
  * @reset_state:	Reset state.
  * @read_mutex:	Read mutex.
  * @write_mutex:	Write mutex.
  */
struct ste_conn_char_dev_user {
	struct ste_conn_device 		*cpd_dev;
	struct device 			*cdev_device;
	struct cdev 			cdev;
	dev_t 				devt;
	char 				*name;
	struct sk_buff_head		rx_queue;
	wait_queue_head_t 		rx_wait_queue;
	wait_queue_head_t		reset_wait_queue;
	enum ste_conn_char_reset_state 	reset_state;
	struct mutex 			read_mutex;
	struct mutex			write_mutex;
};

/**
  * struct ste_conn_char_info - Stores all current users.
  * @bt_cmd_user:		BT command channel user.
  * @bt_acl_user:		BT ACL channel user.
  * @bt_evt_user:		BT event channel user.
  * @fm_radio_user:		FM radio channel user.
  * @gnss_user:			GNSS channel user.
  * @debug_user:		Debug channel user.
  * @ste_tools_user:		ST-E tools channel user.
  * @hci_logger_user:		HCI logger channel user.
  * @us_ctrl_user:		User space control channel user.
  * @bt_audio_user:		BT audio command channel user.
  * @fm_audio_user:		FM audio command channel user.
  * @core_user:		Core user.
  * @open_mutex:		Open mutex (used for both open and release).
  */
struct ste_conn_char_info {
	struct ste_conn_char_dev_user	bt_cmd_user;
	struct ste_conn_char_dev_user	bt_acl_user;
	struct ste_conn_char_dev_user	bt_evt_user;
	struct ste_conn_char_dev_user	fm_radio_user;
	struct ste_conn_char_dev_user	gnss_user;
	struct ste_conn_char_dev_user	debug_user;
	struct ste_conn_char_dev_user	ste_tools_user;
	struct ste_conn_char_dev_user	hci_logger_user;
	struct ste_conn_char_dev_user	us_ctrl_user;
	struct ste_conn_char_dev_user	bt_audio_user;
	struct ste_conn_char_dev_user	fm_audio_user;
	struct ste_conn_char_dev_user	core_user;
	struct mutex			open_mutex;
};

/* ------------------ Internal function declarations ---------------------- */

static int char_dev_setup_cdev(struct 	ste_conn_char_dev_user *dev_usr,
				struct 	device *dev,
					char *name);
static void char_dev_remove_cdev(struct ste_conn_char_dev_user *dev_usr);
static int ste_conn_char_device_open(struct inode *inode,
					struct file *filp);
static int ste_conn_char_device_release(struct inode *inode,
					struct file *filp);
static ssize_t ste_conn_char_device_read(struct file *filp,
						char __user *buf,
						size_t count,
						loff_t *f_pos);
static ssize_t ste_conn_char_device_write(struct file *filp,
						const char __user *buf,
						size_t count,
						loff_t *f_pos);
static long ste_conn_char_device_unlocked_ioctl(struct file *filp,
						unsigned int cmd,
						unsigned long arg);
static unsigned int ste_conn_char_device_poll(struct file *filp,
						poll_table *wait);
static void ste_conn_char_cpd_read_cb(struct ste_conn_device *dev,
					struct sk_buff *skb);
static void ste_conn_char_cpd_reset_cb(struct ste_conn_device *dev);

/* ------------------ Internal variable declarations ---------------------- */

/*
  * char_info - Main information object for char devices.
  */
static struct ste_conn_char_info *char_info;

/*
  * struct ste_conn_char_fops - Char devices file operations.
  * @read:    Function that reads from the char device.
  * @write:   Function that writes to the char device.
  * @unlocked_ioctl:   Function that performs IO operations with the char device.
  * @poll:    Function that checks if there are possible operations with the char device.
  * @open:    Function that opens the char device.
  * @release: Function that release the char device.
  */
static struct file_operations ste_conn_char_fops = {
	.read		= ste_conn_char_device_read,
	.write		= ste_conn_char_device_write,
	.unlocked_ioctl	= ste_conn_char_device_unlocked_ioctl,
	.poll		= ste_conn_char_device_poll,
	.open		= ste_conn_char_device_open,
	.release	= ste_conn_char_device_release
};

/*
  * struct ste_conn_char_cb - Callback structure for ste_conn user.
  * @read_cb:  Callback function called when data is received from the ste_conn.
  * @reset_cb: Callback function called when the controller has been reset.
  */
static struct ste_conn_callbacks ste_conn_char_cb = {
	.read_cb = ste_conn_char_cpd_read_cb,
	.reset_cb = ste_conn_char_cpd_reset_cb
};

/* ---------------- CPD callbacks --------------------------------- */

/**
 * ste_conn_char_cpd_read_cb() - handle data received from controller.
 * @dev: device receiving data.
 * @skb: buffer with data coming from controller.
 *
 * The ste_conn_char_cpd_read_cb() function handles data received
 * from connectivity protocol driver.
 */
static void ste_conn_char_cpd_read_cb(struct ste_conn_device *dev,
					struct sk_buff *skb)
{
	struct ste_conn_char_dev_user *char_dev =
		(struct ste_conn_char_dev_user *)dev->user_data;

	STE_CONN_INFO("ste_conn_char_cpd_read_cb");

	if (!char_dev) {
		STE_CONN_ERR("No char dev! Exiting");
		kfree_skb(skb);
		return;
	}

	skb_queue_tail(&char_dev->rx_queue, skb);

	wake_up_interruptible(&char_dev->rx_wait_queue);
}

/**
 * ste_conn_char_cpd_reset_cb() - handle reset from controller.
 * @dev: device reseting.
 *
 * The ste_conn_char_cpd_reset_cb() function handles reset from
 * connectivity protocol driver.
 */
static void ste_conn_char_cpd_reset_cb(struct ste_conn_device *dev)
{
	struct ste_conn_char_dev_user *char_dev =
		(struct ste_conn_char_dev_user *)dev->user_data;

	STE_CONN_INFO("ste_conn_char_cpd_reset_cb");

	if (!char_dev) {
		STE_CONN_ERR("char_dev == NULL");
		return;
	}

	char_dev->reset_state = STE_CONN_CHAR_RESET;

	wake_up_interruptible(&char_dev->rx_wait_queue);
	wake_up_interruptible(&char_dev->reset_wait_queue);
}

/* ---------------- File operation functions --------------------------------- */

/**
 * ste_conn_char_device_open() - open char device.
 * @inode: Device driver information.
 * @filp:  Pointer to the file struct.
 *
 * The ste_conn_char_device_open() function opens the char device.
 *
 * Returns:
 *   0 if there is no error.
 *   -EACCES if device was already registered to driver or if registration
 *   failed.
 */
static int ste_conn_char_device_open(struct inode *inode,
					struct file *filp)
{
	int err = 0;
	struct ste_conn_char_dev_user *dev; /*device information*/

	mutex_lock(&char_info->open_mutex);
	dev = container_of(inode->i_cdev, struct ste_conn_char_dev_user, cdev);
	filp->private_data = dev;

	STE_CONN_INFO("ste_conn_char_device_open %s", dev->name);

	if (dev->cpd_dev) {
		STE_CONN_ERR("Device already registered to CPD");
		err = -EACCES;
		goto error_handling;
	}
	/* First initiate wait queues for this device. */
	init_waitqueue_head(&dev->rx_wait_queue);
	init_waitqueue_head(&dev->reset_wait_queue);

	dev->reset_state = STE_CONN_CHAR_IDLE;

	/*Register to ste_conn*/
	dev->cpd_dev = ste_conn_register(dev->name, &ste_conn_char_cb);
	if (dev->cpd_dev) {
		dev->cpd_dev->user_data = dev;
	} else {
		STE_CONN_ERR("Couldn't register to CPD for H:4 channel %s",
				dev->name);
		err = -EACCES;
	}

error_handling:
	mutex_unlock(&char_info->open_mutex);
	return err;
}

/**
 * ste_conn_char_device_release() - release char device.
 * @inode: Device driver information.
 * @filp:  Pointer to the file struct.
 *
 * The ste_conn_char_device_release() function release the char device.
 *
 * Returns:
 *   0 if there is no error.
 *   -EBADF if NULL pointer was supplied in private data.
 */
static int ste_conn_char_device_release(struct inode *inode,
					struct file *filp)
{
	int err = 0;
	struct ste_conn_char_dev_user *dev =
		(struct ste_conn_char_dev_user *)filp->private_data;

	mutex_lock(&dev->read_mutex);
	mutex_lock(&dev->write_mutex);
	mutex_lock(&char_info->open_mutex);

	STE_CONN_INFO("ste_conn_char_device_release %s", dev->name);

	if (!dev) {
		STE_CONN_ERR("Calling with NULL pointer");
		err = -EBADF;
		goto error_handling;

	}

	if (dev->reset_state == STE_CONN_CHAR_IDLE) {
		ste_conn_deregister(dev->cpd_dev);
	}

	dev->cpd_dev = NULL;
	filp->private_data = NULL;
	wake_up_interruptible(&dev->rx_wait_queue);
	wake_up_interruptible(&dev->reset_wait_queue);

error_handling:
	mutex_unlock(&char_info->open_mutex);
	mutex_unlock(&dev->write_mutex);
	mutex_unlock(&dev->read_mutex);

	return err;
}

/**
 * ste_conn_char_device_read() - queue and copy buffer to user.
 * @filp:  Pointer to the file struct.
 * @buf:   Received buffer.
 * @count: Size of buffer.
 * @f_pos: Position in buffer.
 *
 * The ste_conn_char_device_read() function queues and copy
 * the received buffer to the user space char device.
 *
 * Returns:
 *   Bytes successfully read (could be 0).
 *   -EBADF if NULL pointer was supplied in private data.
 *   -EFAULT if copy_to_user fails.
 *   Error codes from wait_event_interruptible.
 */
static ssize_t ste_conn_char_device_read(struct file *filp,
						char __user *buf, size_t count,
						loff_t *f_pos)
{
	struct ste_conn_char_dev_user *dev =
		(struct ste_conn_char_dev_user *)filp->private_data;
	struct sk_buff *skb;
	int bytes_to_copy;
	int err = 0;

	mutex_lock(&dev->read_mutex);
	STE_CONN_INFO("ste_conn_char_device_read");

	if (!dev) {
		STE_CONN_ERR("Calling with NULL pointer");
		err = -EBADF;
		goto error_handling;
	}

	if (skb_queue_empty(&dev->rx_queue)) {
		err = wait_event_interruptible(dev->rx_wait_queue,
						 (!(skb_queue_empty(&dev->rx_queue))) ||
						 (STE_CONN_CHAR_RESET == dev->reset_state) ||
						 (dev->cpd_dev == NULL));
		if (err) {
			STE_CONN_ERR("Failed to wait for event");
			goto error_handling;
		}
	}

	if (!dev->cpd_dev) {
		STE_CONN_INFO("cpd_dev is empty - return with negative bytes");
		err = -EBADF;
		goto error_handling;
	}

	skb = skb_dequeue(&dev->rx_queue);

	if (!skb) {
		STE_CONN_INFO("skb queue is empty - return with zero bytes");
		bytes_to_copy = 0;
		goto finished;
	}

	bytes_to_copy = min(count, skb->len);

	err = copy_to_user(buf, skb->data, bytes_to_copy);
	if (err) {
		skb_queue_head(&dev->rx_queue, skb);
		err = -EFAULT;
		goto error_handling;
	}
	skb_pull(skb, bytes_to_copy);

	if (skb->len > 0) {
		skb_queue_head(&dev->rx_queue, skb);
	} else {
		kfree_skb(skb);
	}
	goto finished;

error_handling:
	mutex_unlock(&dev->read_mutex);
	return (ssize_t)err;
finished:
	mutex_unlock(&dev->read_mutex);
	return bytes_to_copy;
}

/**
 * ste_conn_char_device_write() - copy buffer from user and write to cpd.
 * @filp:  Pointer to the file struct.
 * @buf:   Write buffer.
 * @count: Size of the buffer write.
 * @f_pos: Position of buffer.
 *
 * The ste_conn_char_device_write() function copy buffer
 * from user and write to cpd.
 *
 * Returns:
 *   Bytes successfully written (could be 0).
 *   -EBADF if NULL pointer was supplied in private data.
 *   -EFAULT if copy_from_user fails.
 */
static ssize_t ste_conn_char_device_write(struct file *filp,
						const char __user *buf, size_t count,
						loff_t *f_pos)
{
	struct sk_buff *skb;
	struct ste_conn_char_dev_user *dev =
		(struct ste_conn_char_dev_user *)filp->private_data;
	int err = 0;

	mutex_lock(&dev->write_mutex);
	STE_CONN_INFO("ste_conn_char_device_write");

	if (!dev) {
		err = -EBADF;
		goto error_handling;
	}

	skb = ste_conn_alloc_skb(count, GFP_KERNEL);
	if (skb) {
		if (copy_from_user(skb_put(skb, count), buf, count)) {
			kfree_skb(skb);
			err = -EFAULT;
			goto error_handling;
		}
		ste_conn_write(dev->cpd_dev, skb);
		mutex_unlock(&dev->write_mutex);
		return count;
	} else {
		STE_CONN_ERR("Couldn't allocate sk_buff with length %d", count);
	}

error_handling:
	mutex_unlock(&dev->write_mutex);
	return err;
}

/**
 * ste_conn_char_device_unlocked_ioctl() - handle IOCTL call to the interface.
 * @filp:  Pointer to the file struct.
 * @cmd:   IOCTL cmd.
 * @arg:   IOCTL arg.
 *
 * The ste_conn_char_device_unlocked_ioctl() function handles IOCTL call to the interface.
 *
 * Returns:
 *   0 if there is no error.
 *   -EBADF if NULL pointer was supplied in private data.
 *   -EINVAL if supplied cmd is not supported.
 *   For cmd STE_CONN_CHAR_DEV_IOCTL_CHECK4RESET 0x01 is returned if device is
 *   reset and 0x02 is returned if device is closed.
 */
static long ste_conn_char_device_unlocked_ioctl(struct file *filp,
						unsigned int cmd,
						unsigned long arg)
{
	struct ste_conn_char_dev_user *dev =
		(struct ste_conn_char_dev_user *)filp->private_data;
	int err = 0;

	switch (cmd) {
	case STE_CONN_CHAR_DEV_IOCTL_RESET:
		if (!dev) {
			err = -EBADF;
			goto error_handling;
		}
		STE_CONN_INFO("ioctl reset command for device %s", dev->name);
		err = ste_conn_reset(dev->cpd_dev);
		break;

	case STE_CONN_CHAR_DEV_IOCTL_CHECK4RESET:
		if (!dev) {
			STE_CONN_INFO("ioctl check for reset command for device");
			/*Return positive value if closed */
			err = STE_CONN_CHAR_DEV_IOCTL_EVENT_CLOSED;
		} else if (dev->reset_state == STE_CONN_CHAR_RESET) {
			STE_CONN_INFO("ioctl check for reset command for device %s", dev->name);
			/*Return positive value if reset */
			err = STE_CONN_CHAR_DEV_IOCTL_EVENT_RESET;
		}
		break;
	default:
		STE_CONN_ERR("Unknown ioctl command");
		err = -EINVAL;
		break;
	};

error_handling:
	return err;
}

/**
 * ste_conn_char_device_poll() - handle POLL call to the interface.
 * @filp:  Pointer to the file struct.
 * @wait:  Poll table supplied to caller.
 *
 * The ste_conn_char_device_poll() function handles IOCTL call to the interface.
 *
 * Returns:
 *   Mask of current set POLL values
 */
static unsigned int ste_conn_char_device_poll(struct file *filp,
						poll_table *wait)
{
	struct ste_conn_char_dev_user *dev =
		(struct ste_conn_char_dev_user *)filp->private_data;
	unsigned int mask = 0;

	if (!dev) {
		STE_CONN_DBG("device not open");
		return POLLERR | POLLRDHUP;
	}

	poll_wait(filp, &dev->reset_wait_queue, wait);
	poll_wait(filp, &dev->rx_wait_queue, wait);

	if (!dev->cpd_dev) {
		mask |= POLLERR | POLLRDHUP;
	}

	if (!(skb_queue_empty(&dev->rx_queue))) {
		mask |= POLLIN | POLLRDNORM;
	}

	if (STE_CONN_CHAR_RESET == dev->reset_state) {
		mask |= POLLPRI;
	}

	return mask;
}

/* ---------------- Internal functions --------------------------------- */

/**
 * char_dev_setup_cdev() - Set up the char device structure for device.
 * @dev_usr:   	 Char device user.
 * @parent:  	 Parent device pointer.
 * @name:  	 Name of registered device.
 *
 * The char_dev_setup_cdev() function sets up the char_dev
 * structure for this device.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL pointer has been supplied.
 *   Error codes from cdev_add and device_create.
 */
static int char_dev_setup_cdev(struct ste_conn_char_dev_user *dev_usr,
				struct 	device *parent,
					char *name)
{
	int err = 0;
	struct ste_conn_ccd_driver_data *driver_data =
		(struct ste_conn_ccd_driver_data *)dev_get_drvdata(parent);

	if (!driver_data) {
		STE_CONN_ERR("Received driver data is empty");
		err = EINVAL;
		goto finished;
	}

	dev_usr->devt = MKDEV(MAJOR(parent->devt), driver_data->next_free_minor);

	STE_CONN_INFO("char_dev_setup_cdev");

	/* Store device name */
	dev_usr->name = name;

	cdev_init(&dev_usr->cdev, &ste_conn_char_fops);
	dev_usr->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev_usr->cdev, dev_usr->devt , 1);
	if (err) {
		STE_CONN_ERR("Failed to add char dev %d, error %d", err, dev_usr->devt);
		goto finished;
	} else {
		STE_CONN_INFO(
			"Added char device %s with major 0x%X and minor 0x%X",
			name, MAJOR(dev_usr->devt), MINOR(dev_usr->devt));
	}

	/* Create device node in file system. */
	dev_usr->cdev_device = device_create(parent->class,
		parent, dev_usr->devt, NULL, name);
	if (IS_ERR(dev_usr->cdev_device)) {
		STE_CONN_ERR("Error adding %s device!", name);
		err = (int)dev_usr->cdev_device;
		goto error_handling_cdev_del;
	}

	/*Init mutexs*/
	mutex_init(&dev_usr->read_mutex);
	mutex_init(&dev_usr->write_mutex);

	skb_queue_head_init(&dev_usr->rx_queue);

	driver_data->next_free_minor++;
	goto finished;

error_handling_cdev_del:
	cdev_del(&(dev_usr->cdev));

finished:
	return err;
}

/**
 * char_dev_remove_cdev() - Remove char device structure for device.
 * @dev_usr:   Char device user.
 *
 * The char_dev_remove_cdev() function releases the char_dev
 * structure for this device.
 */
static void char_dev_remove_cdev(struct ste_conn_char_dev_user *dev_usr)
{
	STE_CONN_INFO("char_dev_remove_cdev");

	if (!dev_usr) {
		return;
	}

	/*Delete device*/
	cdev_del(&(dev_usr->cdev));

	/* Remove device node in file system. */
	device_destroy(dev_usr->cdev_device->class, dev_usr->devt);

	kfree(dev_usr->cdev_device);
	dev_usr->cdev_device = NULL;

	kfree(dev_usr->name);
	dev_usr->name = NULL;

	mutex_destroy(&dev_usr->read_mutex);
	mutex_destroy(&dev_usr->write_mutex);
}

/* ---------------- External functions -------------- */

void ste_conn_char_devices_init(int char_dev_usage, struct device *dev)
{
	STE_CONN_INFO("ste_conn_char_devices_init");

	if (!dev) {
		STE_CONN_ERR("NULL supplied for dev");
		return;
	}

	if (!char_dev_usage) {
		STE_CONN_INFO("No char dev used in ste_conn.");
		return;
	} else {
		STE_CONN_INFO("ste_conn char devices ver %s, char_dev_usage 0x%X",
				VERSION, char_dev_usage);
	}

	if (char_info) {
		STE_CONN_ERR("Char devices already initiated");
		return;
	}

	/* Initialize private data. */
	char_info = kzalloc(sizeof(*char_info), GFP_KERNEL);
	if (!char_info) {
		STE_CONN_ERR("Could not alloc ste_conn_char_info struct.");
		return;
	}
	mutex_init(&char_info->open_mutex);

	if (char_dev_usage & STE_CONN_CHAR_DEV_BT) {
		char_dev_setup_cdev(&char_info->bt_cmd_user, dev,
					STE_CONN_DEVICES_BT_CMD);
		char_dev_setup_cdev(&char_info->bt_acl_user, dev,
					STE_CONN_DEVICES_BT_ACL);
		char_dev_setup_cdev(&char_info->bt_evt_user, dev,
					STE_CONN_DEVICES_BT_EVT);
	}

	if (char_dev_usage & STE_CONN_CHAR_DEV_FM_RADIO) {
		char_dev_setup_cdev(&char_info->fm_radio_user, dev,
					STE_CONN_DEVICES_FM_RADIO);
	}

	if (char_dev_usage & STE_CONN_CHAR_DEV_GNSS) {
		char_dev_setup_cdev(&char_info->gnss_user, dev,
					STE_CONN_DEVICES_GNSS);
	}

	if (char_dev_usage & STE_CONN_CHAR_DEV_DEBUG) {
		char_dev_setup_cdev(&char_info->debug_user, dev,
					STE_CONN_DEVICES_DEBUG);
	}

	if (char_dev_usage & STE_CONN_CHAR_DEV_STE_TOOLS) {
		char_dev_setup_cdev(&char_info->ste_tools_user, dev,
					STE_CONN_DEVICES_STE_TOOLS);
	}

	if (char_dev_usage & STE_CONN_CHAR_DEV_HCI_LOGGER) {
		char_dev_setup_cdev(&char_info->hci_logger_user, dev,
					STE_CONN_DEVICES_HCI_LOGGER);
	}

	if (char_dev_usage & STE_CONN_CHAR_DEV_US_CTRL) {
		char_dev_setup_cdev(&char_info->us_ctrl_user, dev,
					STE_CONN_DEVICES_US_CTRL);
	}

	if (char_dev_usage & STE_CONN_CHAR_DEV_BT_AUDIO) {
		char_dev_setup_cdev(&char_info->bt_audio_user, dev,
					STE_CONN_DEVICES_BT_AUDIO);
	}

	if (char_dev_usage & STE_CONN_CHAR_DEV_FM_RADIO_AUDIO) {
		char_dev_setup_cdev(&char_info->fm_audio_user, dev,
					STE_CONN_DEVICES_FM_RADIO_AUDIO);
	}
	if (char_dev_usage & STE_CONN_CHAR_DEV_CORE) {
		char_dev_setup_cdev(&char_info->core_user, dev,
					STE_CONN_DEVICES_CORE);
	}
}

void ste_conn_char_devices_exit(void)
{
	STE_CONN_INFO("ste_conn_char_devices_exit");

	if (char_info) {
		char_dev_remove_cdev(&char_info->bt_cmd_user);
		char_dev_remove_cdev(&char_info->bt_acl_user);
		char_dev_remove_cdev(&char_info->bt_evt_user);
		char_dev_remove_cdev(&char_info->fm_radio_user);
		char_dev_remove_cdev(&char_info->gnss_user);
		char_dev_remove_cdev(&char_info->debug_user);
		char_dev_remove_cdev(&char_info->ste_tools_user);
		char_dev_remove_cdev(&char_info->hci_logger_user);
		char_dev_remove_cdev(&char_info->us_ctrl_user);
		char_dev_remove_cdev(&char_info->bt_audio_user);
		char_dev_remove_cdev(&char_info->fm_audio_user);
		char_dev_remove_cdev(&char_info->core_user);

		mutex_destroy(&char_info->open_mutex);


		kfree(char_info);
		char_info = NULL;
	}
}

MODULE_AUTHOR("Henrik Possung ST-Ericsson");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ST-Ericsson Connectivity Char Devices Driver ver " VERSION);
MODULE_VERSION(VERSION);
