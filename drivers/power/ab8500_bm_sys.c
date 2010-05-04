/*
 * ab8500_bm.c - AB8500 Battery Management Driver
 *
 * Copyright (C) 2010 ST-Ericsson SA
 * Licensed under GPLv2.
 *
 * TODO: This file to be replaced by a generic sysfs interface for
 * ab8500 clients.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/kobject.h>
#include <linux/types.h>
#include <mach/ab8500.h>
#include <mach/ab8500_sys.h>

/* Gas Gauge Default values */
# define NUM_OF_SAMPLES  0x28

static struct kobject *gauge_kobj;
static long int samples = NUM_OF_SAMPLES;
static long int capacity;


void ab8500_bm_sys_get_gg_capacity(int *value)
{
	/* Return capacity to the battery driver */
	*value = capacity;
}
EXPORT_SYMBOL(ab8500_bm_sys_get_gg_capacity);

void ab8500_bm_sys_get_gg_samples(int *value)
{
	/* Return number of samples to battery driver */
	*value = samples;
}
EXPORT_SYMBOL(ab8500_bm_sys_get_gg_samples);

static ssize_t gauge_set_gauge_ops(struct kobject *kobj, struct attribute *attr,
			const char *buf, size_t length)
{
	long int status;
	int ret, flag;

	if (strcmp(attr->name, "bat_capacity") == 0) {
		strict_strtol(buf, 10, &capacity);

	}

	else if (strcmp(attr->name, "samples") == 0) {
		strict_strtol(buf, 10, &samples);

		/* Stop the CC */
		ret = ab8500_write(AB8500_RTC, AB8500_RTC_CC_CONF_REG, 0x01);
		if (ret) {
			printk(KERN_ERR "sys::cc disable  failed\n");
		return ret;
		}

		/* Program the samples */
		ret =
		    ab8500_write(AB8500_GAS_GAUGE, AB8500_GASG_CC_NCOV_ACCU,
				 samples);
		if (ret) {
			printk(KERN_ERR "sysfs::cc write sample  failed\n");
		return ret;
		}

		/* Start the CC */
		ret = ab8500_write(AB8500_RTC, AB8500_RTC_CC_CONF_REG, 0x03);
		if (ret) {
			printk(KERN_ERR "sysfs::cc enable failed\n");
		return ret;
		}

	}

	else if (strcmp(attr->name, "start") == 0) {

		strict_strtol(buf, 10, &status);
		flag = status;

		switch (flag) {
		case 1:
			ret =
			    ab8500_write(AB8500_RTC, AB8500_RTC_CC_CONF_REG,
					 0x03);
			if (ret)
				printk(KERN_ERR "cc enable failed\n");
			break;

		case 0:
			ret =
			    ab8500_write(AB8500_RTC, AB8500_RTC_CC_CONF_REG,
					 0x01);
			if (ret)
				printk(KERN_ERR "cc disable  failed\n");
			break;

		}

	} else
		printk(KERN_ERR "Not valid sysfs entry ");

	return strlen(buf);
}

static struct attribute battery_capacity_attribute = {.name =
	    "bat_capacity", .owner = THIS_MODULE, .mode = S_IWUGO };
static struct attribute num_samples_attribute = {.name = "samples", .owner =
	    THIS_MODULE, .mode = S_IWUGO };
static struct attribute gauge_status_attribute = {.name = "start", .owner =
	    THIS_MODULE, .mode = S_IWUGO };

static struct attribute *gauge[] = {
	&battery_capacity_attribute,
	&num_samples_attribute,
	&gauge_status_attribute,
	NULL
};

struct sysfs_ops gauge_sysfs_ops = {
	.store = gauge_set_gauge_ops,
};

static struct kobj_type ktype_gauge = {
	.sysfs_ops = &gauge_sysfs_ops,
	.default_attrs = gauge,

};

void ab8500_bm_sys_deinit(void)
{
	kfree(gauge_kobj);
}
EXPORT_SYMBOL(ab8500_bm_sys_deinit);

int ab8500_bm_sys_init(void)
{

	int ret = 0;

	/* Register  sysfs */
	gauge_kobj = kzalloc(sizeof(struct kobject), GFP_KERNEL);
	if (gauge_kobj == NULL)
		return -ENOMEM;

	gauge_kobj->ktype = &ktype_gauge;
	kobject_init(gauge_kobj, gauge_kobj->ktype);

	ret = kobject_set_name(gauge_kobj, "ab8500_gas_gauge");
	if (ret)
		goto exit;

	ret = kobject_add(gauge_kobj, NULL, "ab8500_gas_gauge");
	if (ret)
		kfree(gauge_kobj);
	return ret;
exit:
	printk(KERN_ERR "kobject failed to set name ");
	kfree(gauge_kobj);
	return ret;
}
EXPORT_SYMBOL(ab8500_bm_sys_init);
