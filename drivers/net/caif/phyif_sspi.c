/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2.
 */

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/string.h>
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
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif				/* CONFIG_DEBUG_FS */

/* CAIF header files. */
#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cfcnfg.h>
#include <net/caif/generic/cfpkt.h>
#include <net/caif/generic/cfspil.h>
#include <net/caif/caif_spi.h>
#include <net/caif/caif_chr.h>

MODULE_LICENSE("GPL");

/* SPI frame alignment. */
static int spi_frm_align = 2;
module_param(spi_frm_align, int, S_IRUGO);
MODULE_PARM_DESC(spi_frm_align, "SPI frame alignment.");

/* SPI padding options. */
module_param(spi_up_head_align, int, S_IRUGO);
MODULE_PARM_DESC(spi_up_head_align, "SPI uplink head alignment.");

module_param(spi_up_tail_align, int, S_IRUGO);
MODULE_PARM_DESC(spi_up_tail_align, "SPI uplink tail alignment.");

module_param(spi_down_head_align, int, S_IRUGO);
MODULE_PARM_DESC(spi_down_head_align, "SPI downlink head alignment.");

module_param(spi_down_tail_align, int, S_IRUGO);
MODULE_PARM_DESC(spi_down_tail_align, "SPI downlink tail alignment.");

#define CFSPI_DBG_PREFILL	0

#ifdef CONFIG_DEBUG_FS

#define DEBUGFS_BUF_SIZE	4096

static struct dentry *dbgfs_root;

static int dbgfs_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t dbgfs_state(struct file *file, char __user *user_buf,
			   size_t count, loff_t *ppos)
{
	char *buf;
	int len = 0;
	ssize_t size;
	struct cfspi_phy *cfspi = (struct cfspi_phy *)file->private_data;

	buf = kzalloc(DEBUGFS_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return 0;

	/* Print out debug information. */
	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
			"CAIF SPI debug information:\n");
	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
			"STATE: %d\n", cfspi->dbg_state);
	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
			"Previous CMD: 0x%x\n", cfspi->pcmd);
	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
			"Current CMD: 0x%x\n", cfspi->cmd);
	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
			"Previous TX len: %d\n", cfspi->tx_ppck_len);
	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
			"Previous RX len: %d\n", cfspi->rx_ppck_len);
	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
			"Current TX len: %d\n", cfspi->tx_cpck_len);
	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
			"Current RX len: %d\n", cfspi->rx_cpck_len);
	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
			"Next TX len: %d\n", cfspi->tx_npck_len);
	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
			"Next RX len: %d\n", cfspi->rx_npck_len);

	size = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return size;
}

static ssize_t print_frame(char *buf, size_t size, char *frm,
			   size_t count, size_t cut)
{
	int len = 0;
	int i;

	for (i = 0; i < count; i++) {
#ifdef CONFIG_ARM
		len += snprintf((buf + len), (size - len), "[0x%02X]",
				frm[i]);
#else
		len += snprintf((buf + len), (size - len), "[0x%02hhX]",
				frm[i]);
#endif	/* CONFIG_ARM */
		if ((i == cut) && (count > (cut * 2))) {
			/* Fast forward. */
			i = count - cut;
			len += snprintf((buf + len), (size - len),
					"--- %d bytes skipped ---\n",
					(count - (cut * 2)));
		}

		if ((!(i % 10)) && i) {
			len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
					"\n");
		}
	}

	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len), "\n");

	return len;
}

static ssize_t dbgfs_frame(struct file *file, char __user *user_buf,
			   size_t count, loff_t *ppos)
{
	char *buf;
	int len = 0;
	ssize_t size;
	struct cfspi_phy *cfspi;

	cfspi = (struct cfspi_phy *)file->private_data;

	buf = kzalloc(DEBUGFS_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return 0;

	/* Print out debug information. */
	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
			"Current frame:\n");

	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
			"Tx data (Len: %d):\n", cfspi->tx_cpck_len);

	len += print_frame((buf + len), (DEBUGFS_BUF_SIZE - len),
			   cfspi->xfer.va_tx,
			   (cfspi->tx_cpck_len + SPI_CMD_SZ), 100);

	len += snprintf((buf + len), (DEBUGFS_BUF_SIZE - len),
			"Rx data (Len: %d):\n", cfspi->rx_cpck_len);

	len += print_frame((buf + len), (DEBUGFS_BUF_SIZE - len),
			   cfspi->xfer.va_rx,
			   (cfspi->rx_cpck_len + SPI_CMD_SZ), 100);

	size = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return size;
}

static const struct file_operations dbgfs_state_fops = {
	.open = dbgfs_open,
	.read = dbgfs_state,
	.owner = THIS_MODULE
};

static const struct file_operations dbgfs_frame_fops = {
	.open = dbgfs_open,
	.read = dbgfs_frame,
	.owner = THIS_MODULE
};

static inline void cfspi_phy_dbg_state(struct cfspi_phy *cfspi, int state)
{
	cfspi->dbg_state = state;
};
#else
static inline void cfspi_phy_dbg_state(struct cfspi_phy *cfspi, int state)
{
};
#endif				/* CONFIG_DEBUG_FS */

static void cfspi_phy_slave_xfer(struct work_struct *work);
static LIST_HEAD(cfspi_phy_list);
static spinlock_t cfspi_phy_list_lock;

/* SPI uplink head alignment. */
static ssize_t show_up_head_align(struct device_driver *driver, char *buf)
{
	return sprintf(buf, "%d\n", spi_up_head_align);
}

static DRIVER_ATTR(up_head_align, S_IRUSR, show_up_head_align, NULL);

/* SPI uplink tail alignment. */
static ssize_t show_up_tail_align(struct device_driver *driver, char *buf)
{
	return sprintf(buf, "%d\n", spi_up_tail_align);
}

static DRIVER_ATTR(up_tail_align, S_IRUSR, show_up_tail_align, NULL);

/* SPI downlink head alignment. */
static ssize_t show_down_head_align(struct device_driver *driver, char *buf)
{
	return sprintf(buf, "%d\n", spi_down_head_align);
}

static DRIVER_ATTR(down_head_align, S_IRUSR, show_down_head_align, NULL);

/* SPI downlink tail alignment. */
static ssize_t show_down_tail_align(struct device_driver *driver, char *buf)
{
	return sprintf(buf, "%d\n", spi_down_tail_align);
}

static DRIVER_ATTR(down_tail_align, S_IRUSR, show_down_tail_align, NULL);

/* SPI frame alignment. */
static ssize_t show_frame_align(struct device_driver *driver, char *buf)
{
	return sprintf(buf, "%d\n", spi_frm_align);
}

static DRIVER_ATTR(frame_align, S_IRUSR, show_frame_align, NULL);

static void cfspi_phy_slave_xfer(struct work_struct *work)
{
	struct cfspi_phy *cfspi = NULL;
	struct cfspil *cfspil = NULL;
	uint8 *ptr = NULL;
	unsigned long flags;

	cfspi = container_of(work, struct cfspi_phy, work);
	cfspil = (struct cfspil *)cfspi->layer.up;

	/* Initialize state. */
	cfspi->cmd = SPI_CMD_EOT;

 slave_loop:

	cfspi_phy_dbg_state(cfspi, CFSPI_STATE_WAITING);

	/* Wait for master talk or transmit event. */
	wait_event_interruptible(cfspi->wait,
				 test_bit(SPI_XFER, &cfspi->state));

#if CFSPI_DBG_PREFILL
	/* Prefill buffers for easier debugging. */
	memset(cfspi->xfer.va_tx, 0xFF, SPI_DMA_BUF_LEN);
	memset(cfspi->xfer.va_rx, 0xFF, SPI_DMA_BUF_LEN);
#endif				/* CFSPI_DBG_PREFILL */

	cfspi_phy_dbg_state(cfspi, CFSPI_STATE_AWAKE);

	/* Check whether we have a committed frame. */
	if (cfspi->tx_cpck_len) {
		struct cfpkt *cfpkt;
		int len;
		struct caif_packet_funcs f;

		cfspi_phy_dbg_state(cfspi, CFSPI_STATE_FETCH_PKT);

		/* Get CAIF frame from SPI layer. */
		cfpkt = caifdev_phy_spi_getxmitpkt(cfspil);
		BUG_ON(cfpkt == NULL);

		/* Get CAIF packet functions. */
		f = cfcnfg_get_packet_funcs();

		/* Copy the CAIF frame to the TX DMA buffer after the SPI
		 * indication. */
		ptr = (uint8 *) cfspi->xfer.va_tx;
		ptr += SPI_IND_SZ;
		f.cfpkt_extract(cfpkt, ptr, (SPI_DMA_BUF_LEN - SPI_IND_SZ),
				&len);

		BUG_ON(len != cfspi->tx_cpck_len);

		/* Free packet. */
		f.cfpkt_destroy(cfpkt);
	}

	cfspi_phy_dbg_state(cfspi, CFSPI_STATE_GET_NEXT);

	/* Get length of next frame to commit. */
	cfspi->tx_npck_len = caifdev_phy_spi_xmitlen(cfspil);

	BUG_ON(cfspi->tx_npck_len > SPI_DMA_BUF_LEN);

	/* Add indication and length at the beginning of the frame. */
	ptr = (uint8 *) cfspi->xfer.va_tx;
	*ptr++ = (SPI_CMD_IND & 0x0FF);
	*ptr++ = ((SPI_CMD_IND & 0xFF00) >> 8);
	*ptr++ = (cfspi->tx_npck_len & 0x00FF);
	*ptr++ = ((cfspi->tx_npck_len & 0xFF00) >> 8);

	/* Calculate length of DMAs. */
	cfspi->xfer.tx_dma_len = cfspi->tx_cpck_len + SPI_IND_SZ;
	cfspi->xfer.rx_dma_len = cfspi->rx_cpck_len + SPI_CMD_SZ;

	/* Add SPI TX frame alignment padding, if necessary. */
	if (cfspi->tx_cpck_len && (cfspi->xfer.tx_dma_len % spi_frm_align)) {
		cfspi->xfer.tx_dma_len += spi_frm_align -
		    (cfspi->xfer.tx_dma_len % spi_frm_align);
	}

	/* Add SPI RX frame alignment padding, if necessary. */
	if (cfspi->rx_cpck_len && (cfspi->xfer.rx_dma_len % spi_frm_align)) {
		cfspi->xfer.rx_dma_len += spi_frm_align -
		    (cfspi->xfer.rx_dma_len % spi_frm_align);
	}

	cfspi_phy_dbg_state(cfspi, CFSPI_STATE_INIT_XFER);

	/* Start transfer. */
	BUG_ON(cfspi->dev->init_xfer(&cfspi->xfer, cfspi->dev));

	cfspi_phy_dbg_state(cfspi, CFSPI_STATE_WAIT_ACTIVE);

	/* TODO: We might be able to make an assumption if this is the
	 * first loop. Make sure that minimum toggle time is respected. */
	udelay(MIN_TRANSITION_TIME_USEC);

	cfspi_phy_dbg_state(cfspi, CFSPI_STATE_SIG_ACTIVE);

	/* Signal that we are ready to recieve data. */
	cfspi->dev->sig_xfer(true, cfspi->dev);

	cfspi_phy_dbg_state(cfspi, CFSPI_STATE_WAIT_XFER_DONE);

	/* Wait for transfer completion. */
	wait_for_completion(&cfspi->comp);

	cfspi_phy_dbg_state(cfspi, CFSPI_STATE_XFER_DONE);

	if (cfspi->cmd == SPI_CMD_EOT) {
		/* Clear the master talk bit. A xfer is always at least two
		 * bursts.
		 */
		clear_bit(SPI_SS_ON, &cfspi->state);
	}

	cfspi_phy_dbg_state(cfspi, CFSPI_STATE_WAIT_INACTIVE);

	/* Make sure that the minimum toggle time is respected. */
	if (SPI_XFER_TIME_USEC(cfspi->xfer.tx_dma_len, cfspi->dev->clk_mhz) <
	    MIN_TRANSITION_TIME_USEC) {
		udelay(MIN_TRANSITION_TIME_USEC -
		       SPI_XFER_TIME_USEC
		       (cfspi->xfer.tx_dma_len, cfspi->dev->clk_mhz));
	}

	cfspi_phy_dbg_state(cfspi, CFSPI_STATE_SIG_INACTIVE);

	/* De-assert transfer signal. */
	cfspi->dev->sig_xfer(false, cfspi->dev);

	/* Check whether we received a CAIF packet. */
	if (cfspi->rx_cpck_len) {
		struct cfpkt *pkt = NULL;
		struct caif_packet_funcs f;

		cfspi_phy_dbg_state(cfspi, CFSPI_STATE_DELIVER_PKT);

		/* Get CAIF packet functions. */
		f = cfcnfg_get_packet_funcs();

		/* Get a suitable CAIF packet and copy in data. */
		pkt = f.cfpkt_create_recv_pkt((unsigned char *)
					      cfspi->xfer.va_rx,
					      cfspi->rx_cpck_len);

		/* Push received packet up the stack. */
		BUG_ON(cfspi->layer.up->receive(cfspi->layer.up, pkt) != 0);
	}
#ifdef CONFIG_DEBUG_FS
	/* Store previous command. */
	cfspi->pcmd = cfspi->cmd;
#endif				/* CONFIG_DEBUG_FS */

	/* Check the next SPI command and length. */
	ptr = (uint8 *) cfspi->xfer.va_rx;
	ptr += cfspi->rx_cpck_len;
	cfspi->cmd = *ptr++;
	cfspi->cmd |= (((*ptr++) << 8) & 0xFF00);
	cfspi->rx_npck_len = *ptr++;
	cfspi->rx_npck_len |= (((*ptr++) << 8) & 0xFF00);

	BUG_ON(cfspi->rx_cpck_len > SPI_DMA_BUF_LEN);

	BUG_ON(cfspi->cmd > SPI_CMD_EOT);

#ifdef CONFIG_DEBUG_FS
	/* Store previous transfer. */
	cfspi->tx_ppck_len = cfspi->tx_cpck_len;
	cfspi->rx_ppck_len = cfspi->rx_cpck_len;
#endif				/* CONFIG_DEBUG_FS */

	/* Check whether the master issued an EOT command. */
	if (cfspi->cmd == SPI_CMD_EOT) {
		/* Reset state. */
		cfspi->tx_cpck_len = 0;
		cfspi->rx_cpck_len = 0;
	} else {
		/* Update state. */
		cfspi->tx_cpck_len = cfspi->tx_npck_len;
		cfspi->rx_cpck_len = cfspi->rx_npck_len;
	}

	/* Check whether we need to clear the xfer bit.
	 * Spin lock needed for packet insertion.
	 * Test and clear of different bits
	 * are not supported. */
	spin_lock_irqsave(&cfspi->lock, flags);
	if ((cfspi->cmd == SPI_CMD_EOT) && (!caifdev_phy_spi_xmitlen(cfspil))
	    && (!test_bit(SPI_SS_ON, &cfspi->state))) {
		clear_bit(SPI_XFER, &cfspi->state);
	}
	spin_unlock_irqrestore(&cfspi->lock, flags);

	goto slave_loop;
}

static void cfspi_phy_ss_cb(bool assert, struct cfspi_ifc *ifc)
{
	struct cfspi_phy *cfspi = (struct cfspi_phy *)ifc->priv;

	/* The slave device does not need to report deassertion. */
	BUG_ON(assert == false);

	if (!in_interrupt())
		spin_lock(&cfspi->lock);

	set_bit(SPI_SS_ON, &cfspi->state);
	set_bit(SPI_XFER, &cfspi->state);

	if (!in_interrupt())
		spin_unlock(&cfspi->lock);

	/* Wake up the xfer thread. */
	wake_up_interruptible(&cfspi->wait);
}

static void cfspi_phy_xfer_done_cb(struct cfspi_ifc *ifc)
{
	struct cfspi_phy *cfspi = (struct cfspi_phy *)ifc->priv;

	/* Transfer done, complete work queue */
	complete(&cfspi->comp);
}

static int cfspi_phy_tx(struct layer *layr, struct payload_info *info,
			struct cfpkt *cfpkt)
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

static int phyif_sspi_probe(struct platform_device *pdev)
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
		       "SSPI: failed to allocate dma TX buffer.\n");
		res = -ENODEV;
		goto err_dma_alloc_tx;
	}

	cfspi->xfer.va_rx =
	    dma_alloc_coherent(NULL, SPI_DMA_BUF_LEN, &cfspi->xfer.pa_rx,
			       GFP_KERNEL);
	if (!cfspi->xfer.va_rx) {
		printk(KERN_WARNING
		       "SSPI: failed to allocate dma TX buffer.\n");
		res = -ENODEV;
		goto err_dma_alloc_rx;
	}

	/* Initialize the work queue. */
	INIT_WORK(&cfspi->work, cfspi_phy_slave_xfer);

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

#ifdef CONFIG_DEBUG_FS
	/* Create debugfs entries. */
	cfspi->dbgfs_dir = debugfs_create_dir(dev->name, dbgfs_root);
	if (!cfspi->dbgfs_dir) {
		printk(KERN_WARNING "SSPI: failed to create debugfs dir.\n");
		res = -ENODEV;
		goto err_create_dbgfs_dir;
	}

	cfspi->dbgfs_state = debugfs_create_file("state", S_IRUGO,
						 cfspi->dbgfs_dir, cfspi,
						 &dbgfs_state_fops);
	if (!cfspi->dbgfs_state) {
		printk(KERN_WARNING "SSPI: failed to create debugfs state.\n");
		res = -ENODEV;
		goto err_create_dbgfs_state;
	}

	cfspi->dbgfs_frame = debugfs_create_file("frame", S_IRUGO,
						 cfspi->dbgfs_dir, cfspi,
						 &dbgfs_frame_fops);
	if (!cfspi->dbgfs_frame) {
		printk(KERN_WARNING "SSPI: failed to create debugfs frame.\n");
		res = -ENODEV;
		goto err_create_dbgfs_frame;
	}
#endif				/* CONFIG_DEBUG_FS */

	/* Fill in PHY information. */
	cfspi->layer.transmit = cfspi_phy_tx;
	cfspi->layer.receive = NULL;

	/* Register PHYIF. */
	res = caifdev_phy_register(&cfspi->layer, CFPHYTYPE_SPI,
				   CFPHYPREF_HIGH_BW);
	if (res < 0) {
		printk(KERN_ERR "SSPI: Reg. error: %d.\n", res);
		goto err_phyif_register;
	}

	/* Set up the ifc. */
	cfspi->ifc.ss_cb = cfspi_phy_ss_cb;
	cfspi->ifc.xfer_done_cb = cfspi_phy_xfer_done_cb;
	cfspi->ifc.priv = cfspi;

	/* Add CAIF SPI device to list. */
	spin_lock(&cfspi_phy_list_lock);
	list_add_tail(&cfspi->list, &cfspi_phy_list);
	spin_unlock(&cfspi_phy_list_lock);

	/* Schedule the work queue. */
	queue_work(cfspi->wq, &cfspi->work);

	return res;

 err_phyif_register:
#ifdef CONFIG_DEBUG_FS
	debugfs_remove(cfspi->dbgfs_frame);
 err_create_dbgfs_frame:
	debugfs_remove(cfspi->dbgfs_state);
 err_create_dbgfs_state:
	debugfs_remove(cfspi->dbgfs_dir);
 err_create_dbgfs_dir:
#endif				/* CONFIG_DEBUG_FS */
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

static int phyif_sspi_remove(struct platform_device *pdev)
{
	struct list_head *list_node;
	struct list_head *n;
	struct cfspi_phy *cfspi = NULL;
	struct cfspi_dev *dev;

	dev = (struct cfspi_dev *)pdev->dev.platform_data;

	spin_lock(&cfspi_phy_list_lock);
	list_for_each_safe(list_node, n, &cfspi_phy_list) {
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

#ifdef CONFIG_DEBUG_FS
			/* Destroy debugfs directory and files. */
			debugfs_remove(cfspi->dbgfs_dir);
			debugfs_remove(cfspi->dbgfs_state);
			debugfs_remove(cfspi->dbgfs_frame);
#endif				/* CONFIG_DEBUG_FS */

			/* Free allocated structure. */
			kfree(cfspi);

			/* Unregister CAIF physical layer. */
			BUG_ON(caifdev_phy_unregister(&cfspi->layer));

			spin_unlock(&cfspi_phy_list_lock);
			return 0;
		}
	}
	spin_unlock(&cfspi_phy_list_lock);

	return -ENODEV;
}

static struct platform_driver phyif_sspi_driver = {
	.probe = phyif_sspi_probe,
	.remove = phyif_sspi_remove,
	.driver = {
		   .name = "phyif_sspi",
		   .owner = THIS_MODULE,
		   },
};

static void __exit cfspi_phy_exit_module(void)
{
	struct list_head *list_node;
	struct list_head *n;
	struct cfspi_phy *cfspi = NULL;

	spin_lock(&cfspi_phy_list_lock);
	list_for_each_safe(list_node, n, &cfspi_phy_list) {
		cfspi = list_entry(list_node, struct cfspi_phy, list);
		/* Remove from list. */
		list_del(list_node);
		/* Free DMA buffers. */
		dma_free_coherent(NULL, SPI_DMA_BUF_LEN, cfspi->xfer.va_rx,
				  cfspi->xfer.pa_rx);
		dma_free_coherent(NULL, SPI_DMA_BUF_LEN, cfspi->xfer.va_tx,
				  cfspi->xfer.pa_tx);
		destroy_workqueue(cfspi->wq);

#ifdef CONFIG_DEBUG_FS
		/* Destroy debugfs directory and files. */
		debugfs_remove(cfspi->dbgfs_dir);
		debugfs_remove(cfspi->dbgfs_state);
		debugfs_remove(cfspi->dbgfs_frame);
#endif				/* CONFIG_DEBUG_FS */

		/* Free allocated structure. */
		kfree(cfspi);

		/* Unregister CAIF physical layer. */
		BUG_ON(caifdev_phy_unregister(&cfspi->layer));
	}
	spin_unlock(&cfspi_phy_list_lock);

	/* Unregister platform driver. */
	platform_driver_unregister(&phyif_sspi_driver);

	/* Destroy sysfs files. */
	driver_remove_file(&phyif_sspi_driver.driver,
			   &driver_attr_up_head_align);
	driver_remove_file(&phyif_sspi_driver.driver,
			   &driver_attr_up_tail_align);
	driver_remove_file(&phyif_sspi_driver.driver,
			   &driver_attr_down_head_align);
	driver_remove_file(&phyif_sspi_driver.driver,
			   &driver_attr_down_tail_align);
	driver_remove_file(&phyif_sspi_driver.driver, &driver_attr_frame_align);

#ifdef CONFIG_DEBUG_FS
	/* Destroy debugfs root directory. */
	debugfs_remove(dbgfs_root);
#endif				/* CONFIG_DEBUG_FS */
}

static int __init cfspi_phy_init_module(void)
{
	int result;

	/* Initialize spin lock. */
	spin_lock_init(&cfspi_phy_list_lock);

	/* Register platform driver. */
	platform_driver_register(&phyif_sspi_driver);

	/* Create sysfs files. */
	result =
	    driver_create_file(&phyif_sspi_driver.driver,
			       &driver_attr_up_head_align);
	if (result) {
		printk(KERN_ERR "Sysfs creation failed 1.\n");
		goto err_create_up_head_align;
	}

	result =
	    driver_create_file(&phyif_sspi_driver.driver,
			       &driver_attr_up_tail_align);
	if (result) {
		printk(KERN_ERR "Sysfs creation failed 2.\n");
		goto err_create_up_tail_align;
	}

	result =
	    driver_create_file(&phyif_sspi_driver.driver,
			       &driver_attr_down_head_align);
	if (result) {
		printk(KERN_ERR "Sysfs creation failed 3.\n");
		goto err_create_down_head_align;
	}

	result =
	    driver_create_file(&phyif_sspi_driver.driver,
			       &driver_attr_down_tail_align);
	if (result) {
		printk(KERN_ERR "Sysfs creation failed 4.\n");
		goto err_create_down_tail_align;
	}

	result =
	    driver_create_file(&phyif_sspi_driver.driver,
			       &driver_attr_frame_align);
	if (result) {
		printk(KERN_ERR "Sysfs creation failed 5.\n");
		goto err_create_frame_align;
	}
#ifdef CONFIG_DEBUG_FS
	dbgfs_root = debugfs_create_dir("phyif_sspi", NULL);

	if (!dbgfs_root) {
		printk(KERN_ERR "Debugfs root creation failed.\n");
		result = -ENODEV;
		goto err_create_dbgfs_dir;
	}
#endif				/* CONFIG_DEBUG_FS */

	return result;

#ifdef CONFIG_DEBUG_FS
 err_create_dbgfs_dir:
	driver_remove_file(&phyif_sspi_driver.driver, &driver_attr_frame_align);
#endif				/* CONFIG_DEBUG_FS */

 err_create_frame_align:
	driver_remove_file(&phyif_sspi_driver.driver,
			   &driver_attr_down_tail_align);
 err_create_down_tail_align:
	driver_remove_file(&phyif_sspi_driver.driver,
			   &driver_attr_down_head_align);
 err_create_down_head_align:
	driver_remove_file(&phyif_sspi_driver.driver,
			   &driver_attr_up_tail_align);
 err_create_up_tail_align:
	driver_remove_file(&phyif_sspi_driver.driver,
			   &driver_attr_up_head_align);
 err_create_up_head_align:

	return result;
}

module_init(cfspi_phy_init_module);
module_exit(cfspi_phy_exit_module);
