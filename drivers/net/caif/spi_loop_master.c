/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/hrtimer.h>

#include <net/caif/caif_spi.h>

#ifdef CONFIG_UML
static inline void  __BUG_ON(unsigned long condition, int line)
{
	if (condition)
		printk(KERN_ERR "BUG_ON: file: %s, line: %d.\n", __FILE__, line);
	else
		return;
}
#undef BUG_ON
#define BUG_ON(x) __BUG_ON((unsigned long)(x), __LINE__)
#endif	/* CONFIG_UML */

MODULE_LICENSE("GPL");

struct mspi_struct {
	struct cfspi_dev mdev;
	struct cfspi_dev *sdev;
	struct cfspi_xfer *mxfer;
	struct cfspi_xfer *sxfer;
	struct hrtimer timer;
	struct platform_device pdev;
	spinlock_t lock;
};

static struct mspi_struct mspi;

extern int cfspi_mspi_dev_reg(struct cfspi_dev *dev, struct cfspi_ifc **ifc);

static enum hrtimer_restart mspi_hrtimer(struct hrtimer *timer)
{
	struct mspi_struct *mspi = NULL;

	mspi = container_of(timer, struct mspi_struct, timer);

	/* Call back directly to the ifcs. We insert some randomness
	 * by calling either the slave or the master first.
	 */
	if (jiffies % 2) {
		mspi->mdev.ifc->xfer_done_cb(mspi->mdev.ifc);
		mspi->sdev->ifc->xfer_done_cb(mspi->sdev->ifc);
	} else {
		mspi->sdev->ifc->xfer_done_cb(mspi->sdev->ifc);
		mspi->mdev.ifc->xfer_done_cb(mspi->mdev.ifc);
	}
	return HRTIMER_NORESTART;
}

static void mspi_xfer(struct mspi_struct *mspi)
{
	uint32_t xfer_time;
	uint16_t xfer_len;

	/* Calculate minimum transfer. */
	if (mspi->mxfer->tx_dma_len < mspi->sxfer->tx_dma_len)
		xfer_len = mspi->sxfer->tx_dma_len;
	else
		xfer_len = mspi->mxfer->tx_dma_len;

	/* Exchange data. */
	memcpy(mspi->sxfer->va_rx, mspi->mxfer->va_tx, xfer_len);
	memcpy(mspi->mxfer->va_rx, mspi->sxfer->va_tx, xfer_len);

	/* Clear transfers. */
	mspi->mxfer = NULL;
	mspi->sxfer = NULL;

	/* Schedule a high resolution timer for the transfer time. */
	xfer_time = SPI_XFER_TIME_USEC(xfer_len, mspi->mdev.clk_mhz);
	hrtimer_start(&mspi->timer, ktime_set(0, xfer_time * 1000),
		      HRTIMER_MODE_REL);
}

static int mspi_init_xfer(struct cfspi_xfer *xfer, struct cfspi_dev *dev)
{
	struct mspi_struct *mspi = (struct mspi_struct *)dev->priv;

	/* If both are ready then proceed with transfer. */
	spin_lock(&mspi->lock);
	/* Store transfer info. */
	mspi->mxfer = xfer;
	if ((mspi->mxfer != NULL) && (mspi->sxfer != NULL)) {
		mspi_xfer(mspi);
		spin_unlock(&mspi->lock);
	} else
		spin_unlock(&mspi->lock);

	return 0;
}

void mspi_sig_xfer(bool xfer, struct cfspi_dev *dev)
{
	struct mspi_struct *mspi = (struct mspi_struct *)dev->priv;

	/* Signal slave. */
	mspi->sdev->ifc->ss_cb(xfer, mspi->sdev->ifc);
}

int spi_mspi_connect(struct cfspi_dev *sdev, struct cfspi_dev **mdev)
{
	int res = 0;

	/* Initialize the high resolution timer. */
	hrtimer_init(&mspi.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mspi.timer.function = mspi_hrtimer;

	/* Initialize simulated master device. */
	mspi.mdev.init_xfer = mspi_init_xfer;
	mspi.mdev.sig_xfer = mspi_sig_xfer;
	mspi.mdev.clk_mhz = sdev->clk_mhz;
	mspi.mdev.priv = &mspi;
	mspi.mdev.name = "spi_mspi";
	mspi.sdev = sdev;

	/* Initialize platform device. */
	mspi.pdev.name = "cfspi_mspi";
	mspi.pdev.dev.platform_data = &mspi.mdev;

	/* Initialize spin lock. */
	spin_lock_init(&mspi.lock);

	/* Register platform device. */
	res = platform_device_register(&mspi.pdev);
	if (res) {
		printk(KERN_WARNING "sspi_init: failed to register dev.\n");
		return -ENODEV;
	}

	/* Assign this SPI device. */
	*mdev = &mspi.mdev;

	return res;
}
EXPORT_SYMBOL(spi_mspi_connect);

void spi_mspi_disconnect(struct cfspi_dev *mdev)
{
	struct mspi_struct *mspi = (struct mspi_struct *)mdev->priv;
	//platform_device_del(&mspi->pdev);
}
EXPORT_SYMBOL(spi_mspi_disconnect);

int spi_mspi_init_xfer(struct cfspi_xfer *xfer, struct cfspi_dev *mdev)
{
	struct mspi_struct *mspi = (struct mspi_struct *)mdev->priv;

	spin_lock(&mspi->lock);
	/* Store transfer info. */
	mspi->sxfer = xfer;
	spin_unlock(&mspi->lock);

	return 0;
}
EXPORT_SYMBOL(spi_mspi_init_xfer);

int spi_mspi_sig_xfer(bool xfer, struct cfspi_dev *mdev)
{
	struct mspi_struct *mspi = (struct mspi_struct *)mdev->priv;

	mdev->ifc->ss_cb(xfer, mdev->ifc);

	/* If both are ready then proceed with transfer. */
	spin_lock(&mspi->lock);
	if ((mspi->mxfer != NULL) && (mspi->sxfer != NULL)) {
		mspi_xfer(mspi);
		spin_unlock(&mspi->lock);
	} else
		spin_unlock(&mspi->lock);

	return 0;
}
EXPORT_SYMBOL(spi_mspi_sig_xfer);

static int __init mspi_init(void)
{
	/* Nothing to do here. Wait for slave to connect. */

	return 0;
}

static void __exit mspi_exit(void)
{
	/* Delete platform device. */
	pr_warning("exit (%p)\n",&mspi.pdev);
	//platform_device_del(&mspi.pdev);

}

module_init(mspi_init);
module_exit(mspi_exit);
