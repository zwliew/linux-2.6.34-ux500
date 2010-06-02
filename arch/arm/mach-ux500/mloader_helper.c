/*
 * Copyright (C) ST-Ericsson SA 2010
 * Authors: Jonas Aaberg <jonas.aberg@stericsson.com>
 *          Paer-Olof Haakansson <par-olof.hakansson@stericsson.com>
 * for ST-Ericsson.
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>

static ssize_t mloader_sysfs_addr(struct device *dev,
				  struct device_attribute *attr,
				  char *buf);

static ssize_t mloader_sysfs_finalize(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count);


static DEVICE_ATTR(addr, S_IRUSR|S_IRGRP, mloader_sysfs_addr, NULL);
static DEVICE_ATTR(finalize, S_IWUSR, NULL, mloader_sysfs_finalize);

static unsigned int bootargs_memmap_modem_start;
static unsigned int bootargs_memmap_modem_total_size;

static unsigned int mloader_helper_shm_total_size;
module_param_named(shm_total_size, mloader_helper_shm_total_size, uint, 0600);
MODULE_PARM_DESC(shm_total_size, "Total Size of SHM shared memory");

static int __init bootargs_memmap(char *str)
{
	char *start_val_str;

	bootargs_memmap_modem_total_size = simple_strtoul(str, NULL, 0);

	start_val_str = strchr(str, '$');

	if (start_val_str == NULL)
		return -EINVAL;

	bootargs_memmap_modem_start = simple_strtoul(start_val_str + 1, NULL, 0);

	return 1;
}
__setup("memmap=", bootargs_memmap);

static int __init bootargs_shm_total_size(char *str)
{
	mloader_helper_shm_total_size = simple_strtoul(str, NULL, 0);

	return 1;
}
__setup(" mloader_helper.shm_total_size=", bootargs_shm_total_size);

static int __exit mloader_remove(struct platform_device *pdev)
{
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_addr.attr);
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_finalize.attr);

	return 0;
}


static struct platform_driver mloader_driver = {
	.driver = {
		.name = "mloader_helper",
	},
	.remove = __exit_p(mloader_remove),
};

struct mloader_helper {
	struct work_struct work;
	struct platform_device *pdev;
};

static void mloader_clean_up(struct work_struct *work)
{
	struct mloader_helper *m = container_of(work,
						struct mloader_helper,
						work);

	/* Remove this module */
	platform_device_unregister(m->pdev);

	platform_driver_unregister(&mloader_driver);
	kfree(m);

}

static ssize_t mloader_sysfs_addr(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	return sprintf(buf, "0x%x 0x%x 0x%x\n",
		       bootargs_memmap_modem_start,
		       bootargs_memmap_modem_total_size,
		       mloader_helper_shm_total_size);
}

static ssize_t mloader_sysfs_finalize(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct mloader_helper *m;

	m = kmalloc(sizeof(struct mloader_helper), GFP_KERNEL);

	m->pdev = container_of(dev,
			       struct platform_device,
			       dev);

	INIT_WORK(&m->work, mloader_clean_up);

	/* The module can not remove itself while being in a sysfs function,
	 * it has to use a workqueue.
	 */
	schedule_work(&m->work);

	return count;
}

static void mloader_release(struct device *dev)
{
	/* Nothing to release */
}

static int __init mloader_probe(struct platform_device *pdev)
{
	int ret = 0;

	pdev->dev.release = mloader_release;

	ret = sysfs_create_file(&pdev->dev.kobj, &dev_attr_addr.attr);
	if (ret)
		return ret;

	ret = sysfs_create_file(&pdev->dev.kobj, &dev_attr_finalize.attr);

	if (ret) {
		sysfs_remove_file(&pdev->dev.kobj, &dev_attr_addr.attr);
		return ret;
	}

	return 0;

}

static int __init mloader_init(void)
{
/*
 * mLoader_helper for Fairbanks. It exports the physical
 * address where the modem side ELF should be located in a sysfs
 * file to make it available for a user space utility.
 * When the mLoader utility has picked up these settings, this module is no
 * longer needed and can be removed by writing to sysfs finalize.
 *
 * The modem side should be loaded via mmap'ed /dev/mem
 *
 */

	return platform_driver_probe(&mloader_driver, mloader_probe);
}
module_init(mloader_init);


static void __exit mloader_exit(void)
{
	platform_driver_unregister(&mloader_driver);
}
module_exit(mloader_exit);

MODULE_AUTHOR("Jonas Aaberg <jonas.aberg@stericsson.com>");
MODULE_LICENSE("GPL");
