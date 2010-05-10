/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author: Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <net/rtnetlink.h>
#include <net/caif/caif_actions.h>
#include <net/caif/caif_dev.h>
#include <net/caif/caif_chr.h>
#include <net/caif/cfloopcfg.h>
#include <net/caif/cfpkt.h>
#include <net/caif/cfcnfg.h>
#include <linux/caif/caif_ioctl.h>
#include <linux/caif/if_caif.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");

int caif_dbg_level = CAIFLOG_LEVEL_WARNING;
EXPORT_SYMBOL(caif_dbg_level);


struct class caif_class = {
	.name = "caif",
};

static ssize_t dbg_lvl_show(struct class *class, char *buf)
{
	return sprintf(buf, "%d\n", caif_dbg_level);
}

static ssize_t dbg_lvl_store(struct class *class, const char *buf,
			     size_t count)
{
	int val;


	if ((sscanf(buf, "%d", &val) != 1) || (val < CAIFLOG_MIN_LEVEL)
			|| (val > CAIFLOG_MAX_LEVEL)) {
		pr_warning("CAIF: %s(): Invalid value\n", __func__);
		return -EINVAL;
	}
	caif_dbg_level = val;
	return count;
}

CLASS_ATTR(dbg_lvl, 0644, dbg_lvl_show, dbg_lvl_store);

int serial_use_stx;
static int (*chrdev_mgmt_func) (int action, union caif_action *param);
struct caif_chr {
	struct cfloopcfg *loop;
	struct miscdevice misc;
};

static struct caif_chr caifdev;


int caifdev_open(struct inode *inode, struct file *filp)
{
	pr_debug("CAIF: %s(): Entered\n", __func__);
	return 0;
}

int caifdev_release(struct inode *inode, struct file *filp)
{
	pr_debug("CAIF: %s(): Entered\n", __func__);
	return 0;
}

static int netdev_create(struct caif_channel_create_action *action)
{
	struct net_device *dev;
	rtnl_lock();
	dev = chnl_net_create(action->name.name, &action->config);
	rtnl_unlock();
	return register_netdev(dev);
}

static int netdev_remove(char *name)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33))
	struct ifreq ifreq;
	strncpy(ifreq.ifr_name, name, sizeof(ifreq.ifr_name));
	ifreq.ifr_name[sizeof(ifreq.ifr_name)-1] = '\0';
	return caif_ioctl(SIOCCAIFNETREMOVE, (unsigned long) &ifreq, false);
#else
	return -EINVAL;
#endif
}

static int caifdev_ioctl(struct inode *inode, struct file *filp,
			 unsigned int cmd, unsigned long argp)
{
	union caif_action param;
	int ret;
	int type;
	int size;
	int operation;
	enum caif_dev_type devtype;

	if (argp == 0) {
		pr_info("CAIF: %s(): argument is null\n", __func__);
		return -EINVAL;
	}

	type = _IOC_TYPE(cmd);
	pr_info("CAIF: %s(): type = %d\n", __func__, type);

	if (type != CAIF_IOC_MAGIC) {
		pr_info("CAIF: %s(): unknown ioctl type\n", __func__);
		return -EINVAL;
	}

	/* Check whether command is valid before copying anything. */
	switch (cmd) {
	case CAIF_IOC_CONFIG_DEVICE:
	case CAIF_IOC_REMOVE_DEVICE:
		break;
	default:
		pr_info("CAIF: %s(): unknown ioctl command\n", __func__);
		return -EINVAL;
	}

	size = _IOC_SIZE(cmd);
	pr_info("CAIF: %s(): size = %d\n", __func__, size);
	if (copy_from_user(&param, (void *) argp, size)) {
		printk(KERN_WARNING
		       "caifdev_ioctl: copy_from_user returned non zero.\n");
		return -EINVAL;
	}


	switch (cmd) {
	case CAIF_IOC_CONFIG_DEVICE:
		operation = CAIF_ACT_CREATE_DEVICE;
		devtype = param.create_channel.name.devtype;
		break;
	case CAIF_IOC_REMOVE_DEVICE:
		operation = CAIF_ACT_DELETE_DEVICE;
		devtype = param.delete_channel.devtype;
		break;
	default:
		printk(KERN_INFO
			"caifdev_ioctl: OTHER ACTIONS NOT YET IMPLEMENTED\n");
		return -EINVAL;
	}

	if (devtype == CAIF_DEV_CHR) {
		pr_info("CAIF: %s(): device type CAIF_DEV_CHR\n", __func__);
		if (chrdev_mgmt_func == NULL) {
			printk(KERN_WARNING
				"caifdev_ioctl:"
				"DevType CHR is not registered\n");
			return -EINVAL;
		}
		ret = (*chrdev_mgmt_func)(operation, &param);
		if (ret < 0) {
			printk(KERN_INFO
				"caifdev_ioctl:"
				"error performing device operation\n");
			return ret;
		}

	} else if (devtype == CAIF_DEV_NET) {
		pr_info("CAIF: %s(): device type CAIF_DEV_NET\n", __func__);
		switch (cmd) {
		case CAIF_IOC_CONFIG_DEVICE:
			return netdev_create(&param.create_channel);
		case CAIF_IOC_REMOVE_DEVICE:
			return netdev_remove(param.delete_channel.name);
		default:
			return -EINVAL;
		}
	}


	if (copy_to_user((void *) argp, &param, size)) {
		printk(KERN_WARNING
		       "caifdev_ioctl: copy_to_user returned non zero.\n");
		return -EINVAL;
	}
	return 0;
}

const struct file_operations caifdev_fops = {
	.owner = THIS_MODULE,
	.open = caifdev_open,
	.ioctl = caifdev_ioctl,
	.release = caifdev_release,
};

void __exit caifdev_exit_module(void)
{
	class_remove_file(&caif_class, &class_attr_dbg_lvl);
	class_unregister(&caif_class);
	misc_deregister(&caifdev.misc);
}

int __init caifdev_init_module(void)
{
	int result;

	caifdev.misc.minor = MISC_DYNAMIC_MINOR;
	caifdev.misc.name = "caifconfig";
	caifdev.misc.fops = &caifdev_fops;

	result = misc_register(&caifdev.misc);

	if (result < 0) {
		printk(KERN_WARNING
		       "caifdev: err: %d, can't register misc.\n", result);
		goto err_misc_register_failed;
	}
	/* Register class for SYSFS. */
	result = class_register(&caif_class);
	if (unlikely(result)) {
		printk(KERN_WARNING
		       "caifdev: err: %d, can't create sysfs node.\n", result);
		goto err_class_register_failed;
	}

	/* Create SYSFS nodes. */
	result = class_create_file(&caif_class, &class_attr_dbg_lvl);
	if (unlikely(result)) {
		printk(KERN_WARNING
		       "caifdev: err: %d, can't create sysfs node.\n", result);
		goto err_sysfs_create_failed;
	}

	return result;

err_sysfs_create_failed:
	class_unregister(&caif_class);
err_class_register_failed:

err_misc_register_failed:
	return -ENODEV;
}

int caifdev_phy_register(struct cflayer *phyif, enum cfcnfg_phy_type phy_type,
				enum cfcnfg_phy_preference phy_pref,
				bool fcs, bool stx)
{
	u16 phyid;

	/* Hook up the physical interface.
	 * Right now we are not using the returned ID.
	 */
	cfcnfg_add_phy_layer(get_caif_conf(), phy_type, NULL, phyif, &phyid,
			phy_pref, fcs, stx);

	pr_debug("CAIF: caifdev_phy_register: "
			"phyif:%p phyid:%d == phyif->id:%d\n",
			(void *)phyif, phyid, phyif->id);
	return 0;
}
EXPORT_SYMBOL(caifdev_phy_register);

int caifdev_phy_unregister(struct cflayer *phyif)
{
	pr_debug("CAIF: caifdev_phy_unregister: phy:%p id:%d\n",
	       phyif, phyif->id);
	cfcnfg_del_phy_layer(get_caif_conf(), phyif);
	return 0;
}
EXPORT_SYMBOL(caifdev_phy_unregister);

int caif_register_chrdev(int (*chrdev_mgmt)
			  (int action, union caif_action *param))
{
	chrdev_mgmt_func = chrdev_mgmt;
	return 0;
}
EXPORT_SYMBOL(caif_register_chrdev);

void caif_unregister_chrdev()
{
	chrdev_mgmt_func = NULL;
}
EXPORT_SYMBOL(caif_unregister_chrdev);

int add_adaptation_layer(struct caif_channel_config *config,
                          struct cflayer *adap_layer)
{
	struct cfctrl_link_param param;

	if (channel_config_2_link_param(get_caif_conf(), config, &param) == 0)
		/* Hook up the adaptation layer. */
		return cfcnfg_add_adaptation_layer(get_caif_conf(),
						&param, adap_layer);

	return -EINVAL;
}
EXPORT_SYMBOL(add_adaptation_layer);

struct caif_packet_funcs cfcnfg_get_packet_funcs()
{
	struct caif_packet_funcs f;
	f.cfpkt_fromnative = cfpkt_fromnative;
	f.cfpkt_tonative = cfpkt_tonative;
	f.cfpkt_destroy = cfpkt_destroy;
	f.cfpkt_create_xmit_pkt = cfpkt_create_uplink;
	f.cfpkt_create_recv_pkt = cfpkt_create_uplink;
	f.cfpkt_dequeue = cfpkt_dequeue;
	f.cfpkt_qpeek = cfpkt_qpeek;
	f.cfpkt_queue = cfpkt_queue;
	f.cfpktq_create = cfpktq_create;
	f.cfpkt_raw_extract = cfpkt_raw_extract;
	f.cfpkt_raw_append = cfpkt_raw_append;
	f.cfpkt_getlen = cfpkt_getlen;
	return f;
}
EXPORT_SYMBOL(cfcnfg_get_packet_funcs);

module_exit(caifdev_exit_module);
subsys_initcall(caifdev_init_module);
