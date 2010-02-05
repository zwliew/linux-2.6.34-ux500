/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>

#include <net/caif/caif_chr.h>
#include <net/caif/caif_spi.h>

MODULE_LICENSE("GPL");

struct sspi_struct {
	struct cfspi_dev sdev;
	struct cfspi_dev *mdev;
	struct cfspi_xfer *xfer;
};

extern int spi_mspi_connect(struct cfspi_dev *sdev, struct cfspi_dev **mdev);
extern int spi_mspi_init_xfer(struct cfspi_xfer *xfer, struct cfspi_dev *mdev);
extern void spi_mspi_sig_xfer(bool xfer, struct cfspi_dev *mdev);

static struct sspi_struct slave;
static struct platform_device slave_device;

static int sspi_init_xfer(struct cfspi_xfer *xfer, struct cfspi_dev *dev)
{
	int res = 0;

	struct sspi_struct *sspi = (struct sspi_struct *)dev->priv;

	/* Connect to simulated master. */
	res = spi_mspi_init_xfer(xfer, sspi->mdev);
	if (res) {
		printk(KERN_WARNING
			"sspi_init_xfer: failed to connect master.\n");
		res = -EFAULT;
	}

	return res;
}

void sspi_sig_xfer(bool xfer, struct cfspi_dev *dev)
{
	struct sspi_struct *sspi = (struct sspi_struct *)dev->priv;

	/* Signal simulated master. */
	spi_mspi_sig_xfer(xfer, sspi->mdev);
}

static int __init sspi_init(void)
{
	int res = 0;

	/* Initialize simulated slave device. */
	slave.sdev.init_xfer = sspi_init_xfer;
	slave.sdev.sig_xfer = sspi_sig_xfer;
	slave.sdev.clk_mhz = 13;
	slave.sdev.priv = &slave;
	slave.sdev.name = "spi_sspi";

	/* Initialize platform device. */
	slave_device.name = "phyif_sspi";
	slave_device.dev.platform_data = &slave.sdev;

	/* Register platform device. */
	res = platform_device_register(&slave_device);
	if (res) {
		printk(KERN_WARNING "sspi_init: failed to register dev.\n");
		return -ENODEV;
	}

	/* Connect to simulated master. */
	res = spi_mspi_connect(&slave.sdev, &slave.mdev);
	if (res) {
		printk(KERN_WARNING "sspi_init: failed to connect master.\n");
		return -ENODEV;
	}

	return res;
}

static void __exit sspi_exit(void)
{
	/* Unregister platform device. */
	platform_device_unregister(&slave_device);
}

module_init(sspi_init);
module_exit(sspi_exit);
