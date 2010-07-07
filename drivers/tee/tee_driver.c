/*
 * Copyright (C) ST-Ericsson SA 2010
 * Author: Martin Hovang <martin.xm.hovang@stericsson.com>
 * Author: Joakim Bech <joakim.xx.bech@stericsson.com>
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/tee.h>
#include <linux/slab.h>

#define TEED_NAME "tee"

#define TEED_STATE_OPEN_DEV 0
#define TEED_STATE_OPEN_SESSION 1

static int tee_open(struct inode *inode, struct file *file);
static int tee_release(struct inode *inode, struct file *file);
static int tee_read(struct file *filp, char __user *buffer,
		    size_t length, loff_t *offset);
static int tee_write(struct file *filp, const char __user *buffer,
		     size_t length, loff_t *offset);

static inline void set_emsg(struct tee_session *ts, u32 msg)
{
	ts->err = msg;
	ts->origin = TEED_ORIGIN_DRIVER;
}

static int copy_ta(struct tee_session *ts,
		   struct tee_write *buf)
{
	ts->ta = kmalloc(buf->ta_size, GFP_KERNEL);
	if (ts->ta == NULL) {
		printk(KERN_INFO "[%s] error, out of memory (ta)\n",
		       __func__);
		set_emsg(ts, TEED_ERROR_OUT_OF_MEMORY);
		return -ENOMEM;
	}

	ts->ta_size = buf->ta_size;

	memcpy(ts->ta, buf->ta, buf->ta_size);
	return 0;
}

static int copy_uuid(struct tee_session *ts,
		     struct tee_write *buf)
{
	ts->uuid = kmalloc(sizeof(struct tee_uuid), GFP_KERNEL);

	if (ts->uuid == NULL) {
		printk(KERN_INFO "[%s] error, out of memory (uuid)\n",
		       __func__);
		set_emsg(ts, TEED_ERROR_OUT_OF_MEMORY);
		return -ENOMEM;
	}

	memcpy(ts->uuid, buf->uuid, sizeof(struct tee_uuid));

	return 0;
}

static int open_tee_device(struct tee_session *ts,
			   struct tee_write *buf)
{
	int ret;

	if (buf->id != TEED_OPEN_SESSION) {
		set_emsg(ts, TEED_ERROR_BAD_STATE);
		return -EINVAL;
	}

	if (buf->ta) {
		ret = copy_ta(ts, buf);
	} else if (buf->uuid) {
		ret = copy_uuid(ts, buf);
	} else {
		set_emsg(ts, TEED_ERROR_COMMUNICATION);
		return -EINVAL;
	}

	ts->id = 0;
	ts->state = TEED_STATE_OPEN_SESSION;
	return ret;
}

static int invoke_command(struct tee_session *ts,
			  struct tee_write *kbuf,
			  struct tee_write __user *ubuf)
{
	int i;
	int ret = 0;
	struct tee_operation *ubuf_op =
		(struct tee_operation *)kbuf->inData;

	ts->idata = kmalloc(sizeof(struct tee_operation), GFP_KERNEL);

	if (!ts->idata) {
		if (ts->idata == NULL) {
			printk(KERN_INFO "[%s] error, out of memory "
			       "(idata)\n", __func__);
			set_emsg(ts, TEED_ERROR_OUT_OF_MEMORY);
			ret = -ENOMEM;
			goto err;
		}
	}

	/* Copy memrefs to kernel space */
	ts->idata->flags = ubuf_op->flags;
	ts->cmd = kbuf->cmd;

	for (i = 0; i < 4; ++i) {
		if (ubuf_op->flags & (1 << i)) {
			ts->idata->shm[i].buffer =
				kmalloc(ubuf_op->shm[i].size,
					GFP_KERNEL);

			if (!ts->idata->shm[i].buffer) {
				printk(KERN_ERR "[%s] out of memory\n",
				       __func__);
				set_emsg(ts, TEED_ERROR_OUT_OF_MEMORY);
				ret = -ENOMEM;
				goto err;
			}
			/*
			 * Copy all shared memory operations to a local
			 * kernel buffer.
			 */
			memcpy(ts->idata->shm[i].buffer,
			       ubuf_op->shm[i].buffer,
			       ubuf_op->shm[i].size);

			ts->idata->shm[i].size = ubuf_op->shm[i].size;
			ts->idata->shm[i].flags = ubuf_op->shm[i].flags;

			/* Secure world expects physical addresses. */
			ts->idata->shm[i].buffer =
				virt_to_phys(ts->idata->shm[i].buffer);
		} else {
			ts->idata->shm[i].buffer = NULL;
			ts->idata->shm[i].size = 0;
			ts->idata->shm[i].flags = 0;
		}
	}

	/* To call secure world */
	if (call_sec_world(ts)) {
		ret = -EINVAL;
		goto err;
	}

	/*
	 * Convert physical addresses back to virtual address so the kernel can
	 * free the buffers when closing the session.
	 */
	for (i = 0; i < 4; ++i) {
		if (ubuf_op->flags & (1 << i)) {
			ts->idata->shm[i].buffer =
				phys_to_virt(ts->idata->shm[i].buffer);
		}
	}

	for (i = 0; i < 4; ++i) {
		if (ubuf_op->flags & (1 << i)) {
			u32 bytes_left = copy_to_user(ubuf_op->shm[i].buffer,
					 ts->idata->shm[i].buffer,
					 ts->idata->shm[i].size);
			if (bytes_left != 0) {
				printk(KERN_ERR "[%s] Failed to copy result to "
				       "user space (%d bytes left).\n", __func__, bytes_left);
			}
		}
	}

err:
	if (ret) {
		for (i = 0; i < 4; ++i)
			kfree(ts->idata->shm[i].buffer);

		kfree(ts->idata);
	}

	return ret;
}

static int tee_open(struct inode *inode, struct file *filp)
{
	struct tee_session *ts;

	filp->private_data = kmalloc(sizeof(struct tee_session), GFP_KERNEL);

	if (filp->private_data == NULL)
		return -ENOMEM;

	ts = (struct tee_session *) (filp->private_data);
	ts->state = TEED_STATE_OPEN_DEV;
	ts->err = TEED_SUCCESS;
	ts->origin = TEED_ORIGIN_DRIVER;
	ts->sync = kmalloc(sizeof(struct mutex), GFP_KERNEL);
	mutex_init(ts->sync);
	ts->ta = NULL;
	ts->uuid = NULL;
	ts->id = 0;
	ts->idata = NULL;
	ts->ta_size = 0;
	ts->err = TEED_SUCCESS;

	return 0;
}

static int tee_release(struct inode *inode, struct file *filp)
{
	struct tee_session *ts;
	int i;

	ts = (struct tee_session *) (filp->private_data);

	if (ts != NULL) {
		if (ts->idata) {
			for (i = 0; i < 4; ++i) {
				kfree(ts->idata->shm[i].buffer);
				ts->idata->shm[i].buffer = NULL;
			}

		}

		kfree(ts->idata);
		ts->idata = NULL;

		kfree(ts->sync);
		ts->sync = NULL;

		kfree(ts->ta);
		ts->ta = NULL;
	}

	kfree(filp->private_data);
	filp->private_data = NULL;

	return 0;
}

/*
 * Called when a process, which already opened the dev file, attempts
 * to read from it. This function gets the current status of the session.
 */
static int tee_read(struct file *filp, char __user *buffer,
		    size_t length, loff_t *offset)
{
	struct tee_read buf;
	struct tee_session *ts;

	if (length != sizeof(struct tee_read)) {
		printk(KERN_INFO "[%s] error, incorrect input length\n",
		       __func__);
		return -EINVAL;
	}

	ts = (struct tee_session *) (filp->private_data);

	if (ts == NULL || ts->sync == NULL) {
		printk(KERN_INFO "[%s] error, private_data not initialized\n",
		       __func__);
		return -EINVAL;
	}

	mutex_lock(ts->sync);

	buf.id = ts->err;
	buf.origin = ts->origin;

	mutex_unlock(ts->sync);

	if (copy_to_user(buffer, &buf, length)) {
		printk(KERN_INFO "[%s] error, copy_to_user failed!\n",
		       __func__);
		return -EINVAL;
	}

	return length;
}

/*
 * Called when a process writes to a dev file
 */
static int tee_write(struct file *filp, const char __user *buffer,
		     size_t length, loff_t *offset)
{
	struct tee_write buf;
	struct tee_session *ts;
	int ret = length;

	if (length != sizeof(struct tee_write)) {
		printk(KERN_INFO "[%s] error, incorrect input length\n",
		       __func__);
		return -EINVAL;
	}

	if (copy_from_user(&buf, buffer, length)) {
		printk(KERN_INFO "[%s] error, tee_session copy_from_user "
		       "failed\n", __func__);
		return -EINVAL;
	}

	ts = (struct tee_session *) (filp->private_data);

	if (ts == NULL || ts->sync == NULL) {
		printk(KERN_INFO "[%s] error, private_data not initialized\n",
		       __func__);
		return -EINVAL;
	}

	mutex_lock(ts->sync);

	switch (ts->state) {
	case TEED_STATE_OPEN_DEV:
		ret = open_tee_device(ts, &buf);
		break;

	case TEED_STATE_OPEN_SESSION:
		switch (buf.id) {
		case TEED_INVOKE:
			ret = invoke_command(ts, &buf,
					     (struct tee_write *)buffer);
			break;

		case TEED_CLOSE_SESSION:
			/* no caching implemented yet... */
			kfree(ts->ta);
			ts->ta = NULL;

			ts->state = TEED_STATE_OPEN_DEV;
			break;

		default:
			set_emsg(ts, TEED_ERROR_BAD_PARAMETERS);
			ret = -EINVAL;
		}
		break;
	default:
		printk(KERN_INFO "[%s] unknown state\n", __func__);
		set_emsg(ts, TEED_ERROR_BAD_STATE);
		ret = -EINVAL;
	}

	/*
	 * We expect that ret has value zero when reaching the end here. If
	 * it has any other value some error must have occured.
	 */
	if (!ret)
		ret = length;
	else
		ret = -EINVAL;

	mutex_unlock(ts->sync);

	return ret;
}

static const struct file_operations tee_fops = {
	.owner = THIS_MODULE,
	.read = tee_read,
	.write = tee_write,
	.open = tee_open,
	.release = tee_release,
};

static struct miscdevice tee_dev = {
	MISC_DYNAMIC_MINOR,
	TEED_NAME,
	&tee_fops
};

static int __init tee_init(void)
{
	int err = 0;

	err = misc_register(&tee_dev);

	if (err) {
		printk(KERN_ERR "[%s] error %d adding character device TEE\n",
		       __func__, err);
	}

	return err;
}

static void __exit tee_exit(void)
{
	misc_deregister(&tee_dev);
}

module_init(tee_init);
module_exit(tee_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Trusted Execution Enviroment driver");
