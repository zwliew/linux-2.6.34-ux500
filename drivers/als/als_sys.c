/*
 *  als_sys.c - Ambient Light Sensor Sysfs support.
 *
 *  Copyright (C) 2009 Intel Corp
 *  Copyright (C) 2009 Zhang Rui <rui.zhang@intel.com>
 *
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kdev_t.h>
#include <linux/idr.h>

MODULE_AUTHOR("Zhang Rui <rui.zhang@intel.com>");
MODULE_DESCRIPTION("Ambient Light Sensor sysfs/class support");
MODULE_LICENSE("GPL");

#define ALS_ID_PREFIX "als"
#define ALS_ID_FORMAT ALS_ID_PREFIX "%d"

static struct class *als_class;

static DEFINE_IDR(als_idr);
static DEFINE_SPINLOCK(idr_lock);

/**
 * als_device_register - register a new Ambient Light Sensor class device
 * @parent:	the device to register.
 *
 * Returns the pointer to the new device
 */
struct device *als_device_register(struct device *dev)
{
	int id, err;
	struct device *alsdev;

again:
	if (unlikely(idr_pre_get(&als_idr, GFP_KERNEL) == 0))
		return ERR_PTR(-ENOMEM);

	spin_lock(&idr_lock);
	err = idr_get_new(&als_idr, NULL, &id);
	spin_unlock(&idr_lock);

	if (unlikely(err == -EAGAIN))
		goto again;
	else if (unlikely(err))
		return ERR_PTR(err);

	id = id & MAX_ID_MASK;
	alsdev = device_create(als_class, dev, MKDEV(0, 0), NULL,
			ALS_ID_FORMAT, id);

	if (IS_ERR(alsdev)) {
		spin_lock(&idr_lock);
		idr_remove(&als_idr, id);
		spin_unlock(&idr_lock);
	}

	return alsdev;
}
EXPORT_SYMBOL(als_device_register);

/**
 * als_device_unregister - removes the registered ALS class device
 * @dev:	the class device to destroy.
 */
void als_device_unregister(struct device *dev)
{
	int id;

	if (likely(sscanf(dev_name(dev), ALS_ID_FORMAT, &id) == 1)) {
		device_unregister(dev);
		spin_lock(&idr_lock);
		idr_remove(&als_idr, id);
		spin_unlock(&idr_lock);
	} else
		dev_dbg(dev->parent,
			"als_device_unregister() failed: bad class ID!\n");
}
EXPORT_SYMBOL(als_device_unregister);

static int __init als_init(void)
{
	als_class = class_create(THIS_MODULE, "als");
	if (IS_ERR(als_class)) {
		printk(KERN_ERR "als_sys.c: couldn't create sysfs class\n");
		return PTR_ERR(als_class);
	}
	return 0;
}

static void __exit als_exit(void)
{
	class_destroy(als_class);
}

subsys_initcall(als_init);
module_exit(als_exit);
