/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif				/* KERN_VERSION_2_6_27 */
#include <linux/workqueue.h>
#include <linux/completion.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

/* CAIF header files. */
#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cfcnfg.h>
#include <net/caif/generic/cfpkt.h>
#include <net/caif/caif_spi.h>
#include <net/caif/caif_chr.h>

MODULE_LICENSE("GPL");

/* SPI frame alignment. */
static int spi_frm_align = 2;
module_param(spi_frm_align, int, S_IRUGO);
MODULE_PARM_DESC(spi_frm_align, "SPI frame alignment.");

static void phyif_mspi_xfer(struct work_struct *work);
static LIST_HEAD(phyif_mspi_list);
static spinlock_t phyif_mspi_list_lock;

static void phyif_mspi_debug(struct cfspi_phy *cfspi, int mask)
{
	if (mask & SPI_TX_DEBUG) {
		uint8 *ptr = NULL;
		int i;

		ptr = (uint8 *) cfspi->xfer.va_tx;
		printk(KERN_INFO "Tx data %s (%p):", cfspi->dev->name, ptr);
		for (i = 0; i < cfspi->xfer.tx_dma_len; i++) {
			if (!(i % 10))
				printk("\n");
			printk(KERN_INFO "[0x%.2x]", ptr[i]);
		}
		printk("\n");
	}

	if (mask & SPI_RX_DEBUG) {
		uint8 *ptr = NULL;
		int i;

		ptr = (uint8 *) cfspi->xfer.va_rx;
		printk(KERN_INFO "Rx data %s (%p):", cfspi->dev->name, ptr);
		for (i = 0; i < cfspi->xfer.rx_dma_len; i++) {
			if (!(i % 10))
				printk("\n");
			printk(KERN_INFO "[0x%.2x]", ptr[i]);
		}
		printk("\n");
	}

	if (mask & SPI_LOG_DEBUG) {
		printk(KERN_INFO "MSPI: tx_dma_len: %d.\n",
		       cfspi->xfer.tx_dma_len);
		printk(KERN_INFO "MSPI: rx_dma_len: %d.\n",
		       cfspi->xfer.rx_dma_len);
		printk(KERN_INFO "MSPI: tx_cpck_len: %d.\n",
		       cfspi->tx_cpck_len);
		printk(KERN_INFO "MSPI: rx_cpck_len: %d.\n",
		       cfspi->rx_cpck_len);
	}
}

void
cfpkt_extract(struct cfpkt *pkt, void *buf, unsigned int buflen,
	      unsigned int *actual_len)
{
	uint16 pklen;
	pklen = cfpkt_getlen(pkt);
	if (likely(buflen < pklen))
		pklen = buflen;
	*actual_len = pklen;
	cfpkt_extr_head(pkt, buf, pklen);
}

static void phyif_mspi_xfer(struct work_struct *work)
{
	struct cfspi_phy *cfspi = NULL;
	struct cfspil *cfspil = NULL;
	uint8 *ptr = NULL;
	bool eot_sent = true;
	bool send_read_cmd = true;
	unsigned long flags;

	cfspi = container_of(work, struct cfspi_phy, work);
	cfspil = (struct cfspil *) cfspi->layer.up;

 master_loop:
	/* Wait for slave talk or transmit event. */
	wait_event_interruptible(cfspi->wait,
				 test_bit(SPI_XFER, &cfspi->state));

	/* Check whether this is the first burst in the transfer. */
	if (eot_sent) {
		/* Make sure that the minimum toggle time is respected. */
		udelay(MIN_TRANSITION_TIME_USEC);

		/* Signal that we are ready to receive data. */
		cfspi->dev->sig_xfer(true, cfspi->dev);

		/* Clear state. */
		eot_sent = false;
		send_read_cmd = true;
	}

	/* Check whether we have a committed frame. */
	if (cfspi->tx_cpck_len) {
		struct cfpkt *cfpkt;
		int len;
		struct caif_packet_funcs f;

		/* Get CAIF frame from SPI layer. */
		cfpkt = caifdev_phy_spi_getxmitpkt(cfspil);
		BUG_ON(cfpkt == NULL);

		/* Get CAIF packet functions. */
		f = cfcnfg_get_packet_funcs();

		/* Copy CAIF frame to the TX DMA buffer. */
		ptr = (uint8 *) cfspi->xfer.va_tx;
		cfpkt_extract(cfpkt, ptr, (SPI_DMA_BUF_LEN - SPI_CMD_SZ),
				&len);

		BUG_ON(len != cfspi->tx_cpck_len);

		/* Free packet. */
		f.cfpkt_destroy(cfpkt);
	}

	/* Get length of next frame to commit. */
	cfspi->tx_npck_len = caifdev_phy_spi_xmitlen(cfspil);

	if (cfspi->tx_npck_len > SPI_DMA_BUF_LEN) {
		printk(KERN_WARNING
		       "MSPI: TX len error: %d.\n", cfspi->tx_npck_len);
	}

	/* Add SPI command for the master device. The SPI comand is added to
	 * the end of the frame. */
	ptr = (uint8 *) cfspi->xfer.va_tx + cfspi->tx_cpck_len;
	if (cfspi->tx_npck_len) {
		*ptr++ = (SPI_CMD_WR & 0x0FF);
		*ptr++ = ((SPI_CMD_WR & 0xFF00) >> 8);
		/* As long as we are writing, we have to send a RD before EOT*/
		send_read_cmd = true;
	} else if (!cfspi->rx_npck_len) {
		if (send_read_cmd == true) {
			*ptr++ = (SPI_CMD_RD & 0x0FF);
			*ptr++ = ((SPI_CMD_RD & 0xFF00) >> 8);
			send_read_cmd = false;
		} else {
			*ptr++ = (SPI_CMD_EOT & 0x0FF);
			*ptr++ = ((SPI_CMD_EOT & 0xFF00) >> 8);
			eot_sent = true;
		}
	} else {
		*ptr++ = (SPI_CMD_RD & 0x0FF);
		*ptr++ = ((SPI_CMD_RD & 0xFF00) >> 8);
		send_read_cmd = false;
	}

	/* Add length to TX DMA buffer. */
	*ptr++ = (cfspi->tx_npck_len & 0x00FF);
	*ptr++ = ((cfspi->tx_npck_len & 0xFF00) >> 8);

	/* Calculate length of DMAs. */
	cfspi->xfer.tx_dma_len = cfspi->tx_cpck_len + SPI_CMD_SZ;
	cfspi->xfer.rx_dma_len = cfspi->rx_cpck_len + SPI_IND_SZ;

	/* Add SPI TX frame alignment padding if necessary. */
	if (cfspi->tx_cpck_len && (cfspi->xfer.tx_dma_len % spi_frm_align)) {
		cfspi->xfer.tx_dma_len += spi_frm_align -
		    (cfspi->xfer.tx_dma_len % spi_frm_align);
	}

	/* Add SPI RX frame alignment padding if necessary. */
	if (cfspi->rx_cpck_len && (cfspi->xfer.rx_dma_len % spi_frm_align)) {
		cfspi->xfer.rx_dma_len += spi_frm_align -
		    (cfspi->xfer.rx_dma_len % spi_frm_align);
	}

	/* Print out the TX data. */
	/*phyif_mspi_debug(cfspi, SPI_TX_DEBUG); */

	/* Wait for slave to synchronize. */
	wait_event_interruptible(cfspi->wait,
				 test_bit(SPI_SS_ON, &cfspi->state));
	clear_bit(SPI_SS_ON, &cfspi->state);

	/* Start transfer. */
	if (cfspi->dev->init_xfer(&cfspi->xfer, cfspi->dev))
		printk(KERN_ERR "MSPI: init_xfer failed.\n");

	/* Wait for transfer completion. */
	wait_for_completion(&cfspi->comp);

	/* Wait for slave to synchronize. */
	wait_event_interruptible(cfspi->wait,
				 test_bit(SPI_SS_OFF, &cfspi->state));
	clear_bit(SPI_SS_OFF, &cfspi->state);
	/*printk(KERN_ERR "MSPI: 3 %s.\n", cfspi->dev->name); */

	/* Check the SPI indication and length. */
	ptr = (uint8 *) cfspi->xfer.va_rx;
	cfspi->cmd = *ptr++;
	cfspi->cmd |= (((*ptr++) << 8) & 0xFF00);
	cfspi->rx_npck_len = *ptr++;
	cfspi->rx_npck_len |= (((*ptr++) << 8) & 0xFF00);

	BUG_ON(cfspi->cmd != SPI_CMD_IND);

	BUG_ON(cfspi->rx_cpck_len > SPI_DMA_BUF_LEN);

	/* Check whether we received a CAIF packet. */
	if (cfspi->rx_cpck_len) {
		struct cfpkt *pkt = NULL;
		struct caif_packet_funcs f;

		/* Get CAIF packet functions. */
		f = cfcnfg_get_packet_funcs();

		/* Get a suitable CAIF packet and copy in data. */
		pkt = f.cfpkt_create_recv_pkt((unsigned char *)
					      cfspi->xfer.va_rx + SPI_IND_SZ,
					      cfspi->rx_cpck_len);

		/* Push received packet up the stack. */
		if (cfspi->layer.up->receive(cfspi->layer.up, pkt) != 0) {
			printk(KERN_WARNING "MSPI: RX err.\n");
			phyif_mspi_debug(cfspi, SPI_LOG_DEBUG | SPI_RX_DEBUG);
		}
	}

	/* Print out the RX data. */
	/*phyif_mspi_debug(cfspi, SPI_RX_DEBUG); */

	/* Reiterate, if necessary. */
	if (eot_sent == true) {
		/* Reset state. */
		cfspi->tx_cpck_len = 0;
		cfspi->rx_cpck_len = 0;

		/* Make sure that the minimum toggle time is respected. */
		if (SPI_XFER_TIME_USEC
		    (cfspi->xfer.tx_dma_len,
		     cfspi->dev->clk_mhz) < MIN_TRANSITION_TIME_USEC) {
			udelay(MIN_TRANSITION_TIME_USEC -
			       SPI_XFER_TIME_USEC(cfspi->xfer.tx_dma_len,
						  cfspi->dev->clk_mhz));
		}

		/* De-assert transfer signal. */
		cfspi->dev->sig_xfer(false, cfspi->dev);

		/* Check whether we need to clear the xfer bit.
		 * Spin lock is needed for packet insertion.
		 * Test and clear of different bits
		 * are not supported. */
		spin_lock_irqsave(&cfspi->lock, flags);
		if ((!caifdev_phy_spi_xmitlen(cfspil))
		    && (!test_bit(SPI_SS_ON, &cfspi->state))) {
			clear_bit(SPI_XFER, &cfspi->state);
		}
		spin_unlock_irqrestore(&cfspi->lock, flags);
	} else {
		/* Update state. */
		cfspi->tx_cpck_len = cfspi->tx_npck_len;
		cfspi->rx_cpck_len = cfspi->rx_npck_len;
	}

	goto master_loop;
}

static void phyif_mspi_ss_cb(bool assert, struct cfspi_ifc *ifc)
{
	struct cfspi_phy *cfspi = (struct cfspi_phy *)ifc->priv;

	if (!in_interrupt())
		spin_lock(&cfspi->lock);
	if (assert) {
		set_bit(SPI_SS_ON, &cfspi->state);
		set_bit(SPI_XFER, &cfspi->state);
	} else {
		set_bit(SPI_SS_OFF, &cfspi->state);
	}
	if (!in_interrupt())
		spin_unlock(&cfspi->lock);

	/* Wake up the xfer thread. */
	wake_up_interruptible(&cfspi->wait);
}

static void phyif_mspi_xfer_done_cb(struct cfspi_ifc *ifc)
{
	struct cfspi_phy *cfspi = (struct cfspi_phy *)ifc->priv;

	/* Transfer done, complete work queue */
	complete(&cfspi->comp);
}

static int phyif_mspi_tx(struct layer *layr, struct cfpkt *cfpkt)
{
	struct cfspi_phy *cfspi = NULL;

	cfspi = container_of(layr, struct cfspi_phy, layer);

	/* We should have an SPI layer above us. */
	BUG_ON(cfpkt != NULL);

	spin_lock(&cfspi->lock);
	if (!test_and_set_bit(SPI_XFER, &cfspi->state)) {
		/* Wake up xfer thread. */
		wake_up_interruptible(&cfspi->wait);
	}
	spin_unlock(&cfspi->lock);

	return 0;
}

static int phyif_mspi_probe(struct platform_device *pdev)
{
	struct cfspi_phy *cfspi = NULL;
	struct cfspi_dev *dev;
	int res;

	dev = (struct cfspi_dev *)pdev->dev.platform_data;

	/* Allocate a CAIF SPI device structure. */
	cfspi = kzalloc(sizeof(struct cfspi_phy), GFP_KERNEL);
	if (!cfspi) {
		CAIFLOG_TRACE(KERN_ERR "SSPI:: kmalloc failed.\n");
		return -ENOMEM;
	}

	/* Assign the SPI device. */
	cfspi->dev = dev;
	/* Assign the device ifc to this SPI interface. */
	dev->ifc = &cfspi->ifc;

	/* Allocate DMA buffers. */
	cfspi->xfer.va_tx =
	    dma_alloc_coherent(NULL, SPI_DMA_BUF_LEN, &cfspi->xfer.pa_tx,
			       GFP_KERNEL);
	if (!cfspi->xfer.va_tx) {
		printk(KERN_WARNING
		       "SSPI: failed to allocate DMA TX buffer.\n");
		res = -ENODEV;
		goto err_dma_alloc_tx;
	}

	cfspi->xfer.va_rx =
	    dma_alloc_coherent(NULL, SPI_DMA_BUF_LEN, &cfspi->xfer.pa_rx,
			       GFP_KERNEL);
	if (!cfspi->xfer.va_rx) {
		printk(KERN_WARNING
		       "SSPI: failed to allocate DMA TX buffer.\n");
		res = -ENODEV;
		goto err_dma_alloc_rx;
	}

	/* Initialize the work queue. */
	INIT_WORK(&cfspi->work, phyif_mspi_xfer);

	/* Initialize spin lock. */
	spin_lock_init(&cfspi->lock);

	/* Initialize wait queue. */
	init_waitqueue_head(&cfspi->wait);

	/* Create work thread. */
	cfspi->wq = create_singlethread_workqueue(dev->name);
	if (!cfspi->wq) {
		printk(KERN_WARNING "SSPI: failed to create work queue.\n");
		res = -ENODEV;
		goto err_create_wq;
	}

	/* Initialize work queue. */
	init_completion(&cfspi->comp);

	/* Fill in PHY information. */
	cfspi->layer.transmit = phyif_mspi_tx;
	cfspi->layer.receive = NULL;

	/* Register PHYIF. */
	res = caifdev_phy_loop_register(&cfspi->layer, CFPHYTYPE_CAIF,
					false,false);
	if (res < 0) {
		printk(KERN_ERR "SSPI: Reg. error: %d.\n", res);
		goto err_phyif_register;
	}

	/* Set up the ifc. */
	cfspi->ifc.ss_cb = phyif_mspi_ss_cb;
	cfspi->ifc.xfer_done_cb = phyif_mspi_xfer_done_cb;
	cfspi->ifc.priv = cfspi;

	/* Add CAIF SPI device to list. */
	spin_lock(&phyif_mspi_list_lock);
	list_add_tail(&cfspi->list, &phyif_mspi_list);
	spin_unlock(&phyif_mspi_list_lock);

	/* Schedule the work queue. */
	queue_work(cfspi->wq, &cfspi->work);

	return res;

 err_phyif_register:
	destroy_workqueue(cfspi->wq);
 err_create_wq:
	dma_free_coherent(NULL, SPI_DMA_BUF_LEN, cfspi->xfer.va_rx,
			  cfspi->xfer.pa_rx);
 err_dma_alloc_rx:
	dma_free_coherent(NULL, SPI_DMA_BUF_LEN, cfspi->xfer.va_tx,
			  cfspi->xfer.pa_tx);
 err_dma_alloc_tx:
	return res;
}

static int phyif_mspi_remove(struct platform_device *pdev)
{
	struct list_head *list_node;
	struct list_head *n;
	struct cfspi_phy *cfspi = NULL;
	struct cfspi_dev *dev;

	dev = (struct cfspi_dev *)pdev->dev.platform_data;

	spin_lock(&phyif_mspi_list_lock);
	list_for_each_safe(list_node, n, &phyif_mspi_list) {
		cfspi = list_entry(list_node, struct cfspi_phy, list);
		/* Find the corresponding device. */
		if (cfspi->dev == dev) {
			/* Remove from list. */
			list_del(list_node);
			/* Free DMA buffers. */
			dma_free_coherent(NULL, SPI_DMA_BUF_LEN,
					  cfspi->xfer.va_rx, cfspi->xfer.pa_rx);
			dma_free_coherent(NULL, SPI_DMA_BUF_LEN,
					  cfspi->xfer.va_tx, cfspi->xfer.pa_tx);
			destroy_workqueue(cfspi->wq);
			/* Free allocated structure. */
			kfree(cfspi);

			/* Unregister CAIF physical layer. */
			BUG_ON(caifdev_phy_unregister(&cfspi->layer));

			spin_unlock(&phyif_mspi_list_lock);
			return 0;
		}
	}
	spin_unlock(&phyif_mspi_list_lock);

	return -ENODEV;
}

static struct platform_driver phyif_mspi_driver = {
	.probe = phyif_mspi_probe,
	.remove = phyif_mspi_remove,
	.driver = {
		   .name = "phyif_mspi",
		   .owner = THIS_MODULE,
		   },
};

static void __exit phyif_mspi_exit_module(void)
{
	struct list_head *list_node;
	struct list_head *n;
	struct cfspi_phy *cfspi = NULL;

	spin_lock(&phyif_mspi_list_lock);
	list_for_each_safe(list_node, n, &phyif_mspi_list) {
		cfspi = list_entry(list_node, struct cfspi_phy, list);
		/* Remove from list. */
		list_del(list_node);
		/* Free DMA buffers. */
		dma_free_coherent(NULL, SPI_DMA_BUF_LEN, cfspi->xfer.va_rx,
				  cfspi->xfer.pa_rx);
		dma_free_coherent(NULL, SPI_DMA_BUF_LEN, cfspi->xfer.va_tx,
				  cfspi->xfer.pa_tx);
		destroy_workqueue(cfspi->wq);
		/* Free allocated structure. */
		kfree(cfspi);

		/* Unregister CAIF physical layer. */
		BUG_ON(caifdev_phy_unregister(&cfspi->layer));
	}
	spin_unlock(&phyif_mspi_list_lock);

	/* Unregister platform driver. */
	platform_driver_unregister(&phyif_mspi_driver);
}

static int __init phyif_mspi_init_module(void)
{
	/* Initialize spin lock. */
	spin_lock_init(&phyif_mspi_list_lock);

	/* Register platform driver. */
	return platform_driver_register(&phyif_mspi_driver);
}

module_init(phyif_mspi_init_module);
module_exit(phyif_mspi_exit_module);
