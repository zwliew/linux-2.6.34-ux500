/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/sched.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
#include <linux/semaphore.h>
#else
#include <linux/semaphore.h>
#endif				/* KERN_VERSION_2_6_27 */
#include <linux/list.h>
#include <linux/workqueue.h>

#ifdef PHYIF_SHM_USE_DMA
#include <linux/dma-mapping.h>
#endif				/* PHYIF_SHM_USE_DMA */
#include <linux/io.h>

/* CAIF header files. */
#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cfcnfg.h>
#include <net/caif/generic/cfpkt.h>

/* CAIF linux header files. */
#include <net/caif/caif_chr.h>
#include <net/caif/caif_shm.h>

MODULE_LICENSE("GPL");

#define SHM_INSTANCES		1

static char *mbxifc_name = "cfmbx";
module_param(mbxifc_name, charp, S_IRUGO);
MODULE_PARM_DESC(mbxifc_name,
		 "Name of the shared memory mailbox interface.");

static char *mbxcfg_name = "cfcfg";
module_param(mbxcfg_name, charp, S_IRUGO);
MODULE_PARM_DESC(mbxcfg_name,
		 "Name of the shared memory configuration interface.");

#define SHM_CMD_DUMMY		0x00

#define SHM_CMD_MASK		(0x3F << 10)
#define SHM_FULL_MASK		(0x0F << 0)
#define SHM_EMPTY_MASK		(0x0F << 4)

#define SHM_SET_CMD(x)		((x & 0x3F) << 10)
#define SHM_GET_CMD(x)		((x >>	10) & 0x3F)

#define SHM_SET_FULL(x)		(((x+1) & 0x0F) << 0)
#define SHM_GET_FULL(x)		(((x >> 0) & 0x0F) - 1)

#define SHM_SET_EMPTY(x)	(((x+1) & 0x0F) << 4)
#define SHM_GET_EMPTY(x)	(((x >> 4) & 0x0F) - 1)

struct shm_pck_desc_t {
	/* Offset from start of shared memory area to start of
	 * shared memory CAIF frame.
	 */
	uint32 frm_ofs;
	/* Length of CAIF frame. */
	uint32 frm_len;
};

struct shm_caif_frm_t{
	/* Number of bytes of padding before the CAIF frame. */
	uint8 hdr_ofs;
};


/* Maximum number of CAIF buffers per shared memory buffer. */
#define SHM_MAX_CAIF_FRMS_PER_BUF	10

/* Size in bytes of the descriptor area
 * (With end of descriptor signalling).
 */
#define SHM_CAIF_DESC_SIZE	((SHM_MAX_CAIF_FRMS_PER_BUF + 1) * \
				  sizeof(struct shm_pck_desc_t))

/* Offset to the first CAIF frame within a shared memory buffer.
 * Aligned on 32 bytes.
 */
#define SHM_CAIF_FRM_OFS	(SHM_CAIF_DESC_SIZE + (SHM_CAIF_DESC_SIZE % 32))
/* Number of bytes for CAIF shared memory header. */
#define SHM_HDR_LEN					1
/* Number of bytes for alignment of the CAIF service layer payload. */
#define SHM_PAYLOAD_ALIGN_LEN		4
/* Number of padding bytes for the complete CAIF frame. */
#define SHM_FRM_PAD_LEN				4

struct shm_layer {
	struct layer shm_phy;
	struct shm_cfgifc_t *cfg_ifc;
	struct shm_mbxifc_t *mbx_ifc;
	char cfg_name[16];
	struct shm_mbxclient_t mbx_client;
	char mbx_name[16];
#ifdef PHYIF_SHM_USE_DMA
	struct dmaifc_t *dma_ifc;
#endif				/* PHYIF_SHM_USE_DMA */
	/* Lists for shared memory buffer synchronization and handling. */
	struct list_head tx_empty_list;
	struct list_head rx_empty_list;
	struct list_head tx_pend_list;
	struct list_head rx_pend_list;
	struct list_head tx_full_list;
	struct list_head rx_full_list;
	struct work_struct sig_work;
	struct work_struct rx_work;
	struct work_struct flow_work;
	struct workqueue_struct *sig_work_queue;
	struct workqueue_struct *rx_work_queue;
	struct workqueue_struct *flow_work_queue;

    /* Wait queue for sender to check for space to write in mailbox. */
    wait_queue_head_t mbx_space_wq;
	int tx_empty_available;

};

static struct shm_layer shm_layer[SHM_INSTANCES];

/* Shared memory buffer structure. */
struct shm_buf_t{
	int index;
	int len;
	int frames;
	unsigned char *desc_ptr;
	int frm_ofs;
	int phy_addr;
#ifdef PHYIF_SHM_USE_DMA
	dma_addr_t *dma_ptr;
#endif				/* PHYIF_SHM_USE_DMA */
	struct list_head list;
};

static DEFINE_SPINLOCK(lock);

static void phyif_shm_sig_work_func(struct work_struct *work);
static void phyif_shm_rx_work_func(struct work_struct *work);
static void phyif_shm_flow_work_func(struct work_struct *work);

static void phyif_shm_sig_work_func(struct work_struct *work)
{
	/* TODO: We assume that this call is not reentrant as
	 *	 that might change the order of the buffers which
	 *	 is not allowed. Option is to lock the whole function.
	 */
	int ret;
	uint16 mbox_msg;
	struct shm_layer *pshm = container_of(work, struct shm_layer, sig_work);

	do {
		struct shm_buf_t *pbuf;
		unsigned long flags;

		/* Initialize mailbox message. */
		mbox_msg = 0x00;

		spin_lock(&lock);

		/* Check for pending transmit buffers. */
		if (!list_empty(&pshm->tx_pend_list)) {
			pbuf =
			    list_entry(pshm->tx_pend_list.next,
				       struct shm_buf_t, list);
			list_del_init(&pbuf->list);

			/* Release mutex. */
			spin_unlock(&lock);

			/* Grab spin lock. */
			spin_lock_irqsave(&lock, flags);

			list_add_tail(&pbuf->list, &pshm->tx_full_list);

			/* Release spin lock. */
			spin_unlock_irqrestore(&lock, flags);

			/* Value index is never changed,
			 * so read access should be safe.
			 */
			mbox_msg |= SHM_SET_FULL(pbuf->index);

			spin_lock(&lock);
		}

		/* Check for pending receive buffers. */
		if (!list_empty(&pshm->rx_pend_list)) {

			pbuf = list_entry(pshm->rx_pend_list.next,
				       struct shm_buf_t, list);
			list_del_init(&pbuf->list);

			/* Release mutex. */
			spin_unlock(&lock);

			/* Grab spin lock. */
			spin_lock_irqsave(&lock, flags);

			list_add_tail(&pbuf->list, &pshm->rx_empty_list);

			/* Release spin lock. */
			spin_unlock_irqrestore(&lock, flags);

			/* Value index is never changed,
			 * so read access should be safe.
			 */
			mbox_msg |= SHM_SET_EMPTY(pbuf->index);

			spin_lock(&lock);
		}

		/* Release mutex. */
		spin_unlock(&lock);

		if (mbox_msg) {
			do {
				long timeout = 3;
				ret = pshm->mbx_ifc->send_msg(mbox_msg,
					pshm->mbx_ifc->priv);

				if (ret) {
					interruptible_sleep_on_timeout(
						&pshm->mbx_space_wq, timeout);
				}

			} while (ret);
		}

	} while (mbox_msg);
}

static void phyif_shm_rx_work_func(struct work_struct *work)
{
	struct shm_buf_t *pbuf;
	struct caif_packet_funcs f;
	struct cfpkt *pkt;
	unsigned long flags;
	struct shm_layer *pshm;
	/* TODO: We assume that this call is not reentrant as that might
	 *	 change the order of the buffers which is not possible.
	 *	 Option is to lock the whole function.
	 */

	pshm = container_of(work, struct shm_layer, rx_work);

	/* Get CAIF packet functions. */
	f = cfcnfg_get_packet_funcs();

	do {
		struct shm_pck_desc_t *pck_desc;

		/* Grab spin lock. */
		spin_lock_irqsave(&lock, flags);

		/* Check for received buffers. */
	if (list_empty(&pshm->rx_full_list)) {
			/* Release spin lock. */
			spin_unlock_irqrestore(&lock, flags);
			break;
		}
		pbuf =
		    list_entry(pshm->rx_full_list.next, struct shm_buf_t, list);
		list_del_init(&pbuf->list);

		/* Release spin lock. */
		spin_unlock_irqrestore(&lock, flags);

		/* Retrieve pointer to start of the packet descriptor area. */
		pck_desc = (struct shm_pck_desc_t *) pbuf->desc_ptr;

		/* Check whether descriptor contains a CAIF shared memory
		 * frame.
		 */
		while (pck_desc->frm_ofs) {
			unsigned int frm_buf_ofs;
			unsigned int frm_pck_ofs;
			unsigned int frm_pck_len;

			/* Check whether offset is within buffer limits
			 * (lower).
			 */
			if (pck_desc->frm_ofs <
			    (pbuf->phy_addr - shm_base_addr)) {
				printk(KERN_WARNING
				       "phyif_shm_rx_work_func:"
				       " Frame offset too small: %d\n",
				       pck_desc->frm_ofs);
				break;
			}

			/* Check whether offset is within buffer limits
			 * (higher).
			 */
			if (pck_desc->frm_ofs >
			    ((pbuf->phy_addr - shm_base_addr) +
			     pbuf->len)) {
				printk(KERN_WARNING
				       "phyif_shm_rx_work_func:"
				       " Frame offset too big: %d\n",
				       pck_desc->frm_ofs);
				break;
			}

			/* Calculate offset from start of buffer. */
			frm_buf_ofs =
			    pck_desc->frm_ofs - (pbuf->phy_addr -
						 shm_base_addr);

			/* Calculate offset and length of CAIF packet while
			 * taking care of the shared memory header.
			 */
			frm_pck_ofs =
			    frm_buf_ofs + SHM_HDR_LEN +
			    (*(pbuf->desc_ptr + frm_buf_ofs));
			frm_pck_len =
			    (pck_desc->frm_len - SHM_HDR_LEN -
			     (*(pbuf->desc_ptr + frm_buf_ofs)));

			/* Check whether CAIF packet is within buffer limits. */
			if ((frm_pck_ofs + pck_desc->frm_len) > pbuf->len) {
				printk(KERN_WARNING
				       "phyif_shm_rx_work_func: "
				       "caif packet too big: offset:"
				       "%d, len: %d\n",
				       frm_pck_ofs, pck_desc->frm_len);
				break;
			}

			/* Get a suitable CAIF packet and copy in data. */
			pkt =
			    f.cfpkt_create_recv_pkt((pbuf->desc_ptr +
						     frm_pck_ofs),
						    frm_pck_len);

			/* Push received packet up the stack. */
			pshm->shm_phy.up->receive(pshm->shm_phy.up, pkt);

			/* Move to next packet descriptor. */
			pck_desc++;
		};

		spin_lock(&lock);
		list_add_tail(&pbuf->list, &pshm->rx_pend_list);
		spin_unlock(&lock);

	} while (1);

	/* Schedule signaling work queue. */
	(void) queue_work(pshm->sig_work_queue, &pshm->sig_work);
}

static void phyif_shm_flow_work_func(struct work_struct *work)
{
	struct shm_layer *pshm;

	pshm = container_of(work, struct shm_layer, flow_work);

	if (!pshm->shm_phy.up->ctrlcmd) {
		printk(KERN_WARNING "phyif_shm_flow_work_func: No flow up.\n");
		return;
	}

	/* Re-enable the flow.*/
	pshm->shm_phy.up->ctrlcmd(pshm->shm_phy.up,
		_CAIF_CTRLCMD_PHYIF_FLOW_ON_IND, 0);
}

static int phyif_shm_mbx_msg_cb(u16 mbx_msg, void *priv)
{
	struct shm_layer *pshm;
	struct shm_buf_t *pbuf;
	unsigned long flags;

	pshm = (struct shm_layer *) priv;
	/* TODO: Do we need the spin locks, since this is assumed to be
	 * called from an IRQ context?
	 *
	 *	 We are also assuming that this call is not be reentrant as
	 *	 that might change the order of the buffers which is not
	 *	 possible. Option is to lock the whole function.
	 */

	/* Check for received buffers. */
	if (mbx_msg & SHM_FULL_MASK) {
		int idx;

		/* Grab spin lock. */
		spin_lock_irqsave(&lock, flags);

		/* Check whether we have any outstanding buffers. */
		if (list_empty(&pshm->rx_empty_list)) {
			/* Release spin lock. */
			spin_unlock_irqrestore(&lock, flags);

			/* We print even in IRQ context... */
			printk(KERN_WARNING
			    "phyif_shm_mbx_msg_cb:"
			    " No empty Rx buffers to fill: msg:%x \n",
			    mbx_msg);

			/* Bail out. */
			goto err_rx_sync;
		}

		pbuf =
		    list_entry(pshm->rx_empty_list.next,
					struct shm_buf_t, list);
		idx = pbuf->index;

		/* Check buffer synchronization. */
		if (idx != SHM_GET_FULL(mbx_msg)) {
			/* Release spin lock. */
			spin_unlock_irqrestore(&lock, flags);

			/* We print even in IRQ context... */
			printk(KERN_WARNING
			       "phyif_shm_mbx_msg_cb: RX full out of sync:"
			       " idx:%d, msg:%x \n",
			       idx, mbx_msg);

			/* Bail out. */
			goto err_rx_sync;
		}

		list_del_init(&pbuf->list);
		list_add_tail(&pbuf->list, &pshm->rx_full_list);

		/* Release spin lock. */
		spin_unlock_irqrestore(&lock, flags);

		/* Schedule RX work queue. */
		(void) queue_work(pshm->rx_work_queue, &pshm->rx_work);
	}

err_rx_sync:
	/* Check for emptied buffers. */
	if (mbx_msg & SHM_EMPTY_MASK) {
		int idx;

		/* Grab spin lock. */
		spin_lock_irqsave(&lock, flags);

	/* Check whether we have any outstanding buffers. */
	if (list_empty(&pshm->tx_full_list)) {
			/* Release spin lock. */
			spin_unlock_irqrestore(&lock, flags);

			/* We print even in IRQ context... */
			printk(KERN_WARNING
				"phyif_shm_mbx_msg_cb:"
				" No TX to empty: msg:%x \n",
			       mbx_msg);

			/* Bail out. */
			goto err_tx_sync;
		}

		pbuf =
		    list_entry(pshm->tx_full_list.next, struct shm_buf_t, list);
		idx = pbuf->index;

		/* Check buffer synchronization. */
		if (idx != SHM_GET_EMPTY(mbx_msg)) {
			/* Release spin lock. */
			spin_unlock_irqrestore(&lock, flags);

			/* We print even in IRQ context... */
			printk(KERN_WARNING
			       "phyif_shm_mbx_msg_cb: TX empty out of sync:"
			       " idx:%d, msg:%x \n",
			       idx, mbx_msg);

			/* Bail out. */
			goto err_tx_sync;
		}

		list_del_init(&pbuf->list);

		/* Reset buffer parameters. */
		pbuf->frames = 0;
		pbuf->frm_ofs = SHM_CAIF_FRM_OFS;

		list_add_tail(&pbuf->list, &pshm->tx_empty_list);

		/* Check whether we have to wake up the transmitter. */
		if (!pshm->tx_empty_available) {
			pshm->tx_empty_available = 1;

			/* Release spin lock. */
			spin_unlock_irqrestore(&lock, flags);

			/* Schedule flow re-enable work queue. */
			(void) queue_work(pshm->flow_work_queue,
					  &pshm->flow_work);

		} else {
			/* Release spin lock. */
			spin_unlock_irqrestore(&lock, flags);
		}
	}

err_tx_sync:
    return ESUCCESS;
}

void shm_phy_fctrl(struct layer *layr, enum caif_ctrlcmd on, int phyid)
{
	/* We have not yet added flow control. */
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

static int shm_send_pkt(struct shm_layer *pshm, struct cfpkt *cfpkt,
				bool append)
{
	unsigned long flags;
	struct shm_buf_t *pbuf;
	unsigned int extlen;
	unsigned int frmlen = 0;
	struct shm_pck_desc_t *pck_desc;
	struct shm_caif_frm_t *frm;
	struct caif_packet_funcs f;

	/* Get CAIF packet functions. */
	f = cfcnfg_get_packet_funcs();

	/* Grab spin lock. */
	spin_lock_irqsave(&lock, flags);

	if (append) {
		if (list_empty(&pshm->tx_pend_list)) {
			/* Release spin lock. */
			spin_unlock_irqrestore(&lock, flags);
			return CFGLU_ERETRY;
		}

		/* Get the last pending buffer. */
		pbuf = list_entry(pshm->tx_pend_list.prev, struct shm_buf_t,
					list);

		/* Check that we don't exceed the descriptor area. */
		if (pbuf->frames >= SHM_MAX_CAIF_FRMS_PER_BUF) {
			/* Release spin lock. */
			spin_unlock_irqrestore(&lock, flags);
			return CFGLU_ERETRY;
		}
	} else {
		if (list_empty(&pshm->tx_empty_list)) {
			/* Update blocking condition. */
			pshm->tx_empty_available = 0;

			/* Release spin lock. */
			spin_unlock_irqrestore(&lock, flags);

			if (!pshm->shm_phy.up->ctrlcmd) {
				printk(KERN_WARNING "shm_phy_tx: No flow up.\n");
				return CFGLU_ERETRY;
			}

			pshm->shm_phy.up->ctrlcmd(pshm->shm_phy.up,
				_CAIF_CTRLCMD_PHYIF_FLOW_OFF_IND, 0);

			return CFGLU_ERETRY;
		}

		/* Get the first free buffer. */
		pbuf = list_entry(pshm->tx_empty_list.next, struct shm_buf_t,
					list);
	}

	list_del_init(&pbuf->list);

	/* Release spin lock. */
	spin_unlock_irqrestore(&lock, flags);

	/* TODO: The CAIF stack will have to give a hint on what is
	 *	 in the payload in order to align it.
	 */
	frm = (struct shm_caif_frm_t *) (pbuf->desc_ptr + pbuf->frm_ofs);
	frm->hdr_ofs = 0;
	frmlen += SHM_HDR_LEN + frm->hdr_ofs;

	/* Add length of CAIF frame. */
	frmlen += f.cfpkt_getlen(cfpkt);

	/* Add tail padding if needed. */
	if (frmlen % SHM_FRM_PAD_LEN)
		frmlen += SHM_FRM_PAD_LEN - (frmlen % SHM_FRM_PAD_LEN);

	/* Verify that packet, header and additional padding can fit
	 * within the buffer frame area.
	 */
	if (frmlen >= (pbuf->len - pbuf->frm_ofs)) {
		if (append) {
			/* Put back packet as end of pending queue. */
			list_add_tail(&pbuf->list, &pshm->tx_pend_list);
			return CFGLU_ENOSPC;
		} else {
			/* Put back packet as start of empty queue. */
			list_add(&pbuf->list, &pshm->tx_empty_list);
			return CFGLU_ENOSPC;
		}
	}

	/* Extract data from the packet to the shared memory CAIF frame
	 * taking into account the shared memory header byte and possible
	 * payload alignment bytes.
	 */

	cfpkt_extract(cfpkt,
		      (pbuf->desc_ptr + pbuf->frm_ofs + SHM_HDR_LEN +
		       frm->hdr_ofs), pbuf->len, &extlen);

	/* Fill in the shared memory packet descriptor area. */
	pck_desc = (struct shm_pck_desc_t *) (pbuf->desc_ptr);
	/* Forward to current frame. */
	pck_desc += pbuf->frames;
	pck_desc->frm_ofs =
	    (pbuf->phy_addr - shm_base_addr) + pbuf->frm_ofs;
	pck_desc->frm_len = frmlen;

	/* Terminate packet descriptor area. */
	pck_desc++;
	pck_desc->frm_ofs = 0;

	/* Update buffer parameters. */
	pbuf->frames++;
	pbuf->frm_ofs += frmlen + (frmlen % 32);

	spin_lock(&lock);
	/* Assign buffer as pending. */
	list_add_tail(&pbuf->list, &pshm->tx_pend_list);
	spin_unlock(&lock);

	/* Schedule signaling work queue. */
	(void) queue_work(pshm->sig_work_queue, &pshm->sig_work);

	/* The packet is sent. As we have come to the end of the line we need
	 * to free the packet.
	 */
	f.cfpkt_destroy(cfpkt);

	return ESUCCESS;
}

int shm_phy_tx(struct layer *layr, struct cfpkt *cfpkt)
{
	struct shm_layer *pshm;
	int result;

	/* TODO: We need a mutex here if this function can be called from
	 *	 different contexts.
	 */

	pshm = container_of(layr, struct shm_layer, shm_phy);

	/* First try to append the frame. */
	result = shm_send_pkt(pshm, cfpkt, true);
	if (!result)
		return result;

	/* Try a new buffer. */
	result = shm_send_pkt(pshm, cfpkt, false);

	return result;
}

static int __init phyif_shm_init(void)
{
	int result = -1;
	int i, j;

	/* Initialize the shared memory instances. */
	for (i = 0; i < SHM_INSTANCES; i++) {
		/* Initialize structures in a clean state. */
		memset(&shm_layer[i], 0, sizeof(shm_layer[i]));

		/* Fill in some information about our PHY. */
		shm_layer[i].shm_phy.transmit = shm_phy_tx;
		shm_layer[i].shm_phy.receive = NULL;
		shm_layer[i].shm_phy.ctrlcmd = shm_phy_fctrl;

		/* TODO: Instance number should be appended. */
		sprintf(shm_layer[i].cfg_name, "%s%d", mbxcfg_name, i);
		sprintf(shm_layer[i].mbx_name, "%s%d", mbxifc_name, i);

		/* Initialize queues. */
		INIT_LIST_HEAD(&(shm_layer[i].tx_empty_list));
		INIT_LIST_HEAD(&(shm_layer[i].rx_empty_list));
		INIT_LIST_HEAD(&(shm_layer[i].tx_pend_list));
		INIT_LIST_HEAD(&(shm_layer[i].rx_pend_list));
		INIT_LIST_HEAD(&(shm_layer[i].tx_full_list));
		INIT_LIST_HEAD(&(shm_layer[i].rx_full_list));

		INIT_WORK(&shm_layer[i].sig_work, phyif_shm_sig_work_func);
		INIT_WORK(&shm_layer[i].rx_work, phyif_shm_rx_work_func);
		INIT_WORK(&shm_layer[i].flow_work, phyif_shm_flow_work_func);

		/*
		 * Create the RX work thread.
		 * Create the signaling work thread.
		 * Create the flow re-enable work thread.
		 * TODO: Instance number should be appended.
		 */
		shm_layer[i].rx_work_queue =
		    create_singlethread_workqueue("phyif_shm_rx");

		shm_layer[i].sig_work_queue =
		    create_singlethread_workqueue("phyif_shm_sig");

		shm_layer[i].flow_work_queue =
			create_singlethread_workqueue("phyif_shm_flow");

		init_waitqueue_head(&(shm_layer[i].mbx_space_wq));
		shm_layer[i].tx_empty_available = 1;

		/* Connect to the shared memory configuration module. */
		/*shm_layer[i].cfg_ifc = cfgifc_get(shm_layer[i].cfg_name);*/

		/* Initialize the shared memory transmit buffer queues. */
		for (j = 0; j < shm_nr_tx_buf; j++) {
			struct shm_buf_t *tx_buf =
			    kmalloc(sizeof(struct shm_buf_t), GFP_KERNEL);
			tx_buf->index = j;
			tx_buf->len = shm_tx_buf_len;
			if ((i % 2) == 0) {
				tx_buf->phy_addr =
				    shm_tx_addr + (shm_tx_buf_len * j);
			} else {
				tx_buf->phy_addr =
				    shm_rx_addr + (shm_tx_buf_len * j);
			}
			tx_buf->desc_ptr =
			    ioremap(tx_buf->phy_addr, shm_tx_buf_len);
			tx_buf->frames = 0;
			tx_buf->frm_ofs = SHM_CAIF_FRM_OFS;
			list_add_tail(&tx_buf->list,
				      &shm_layer[i].tx_empty_list);
		}

		/* Initialize the shared memory receive buffer queues. */
		for (j = 0; j < shm_nr_rx_buf; j++) {
			struct shm_buf_t *rx_buf =
			    kmalloc(sizeof(struct shm_buf_t), GFP_KERNEL);
			rx_buf->index = j;
			rx_buf->len = shm_rx_buf_len;
			if ((i % 2) == 0) {
				rx_buf->phy_addr =
				    shm_rx_addr + (shm_rx_buf_len * j);
			} else {
				rx_buf->phy_addr =
				    shm_tx_addr + (shm_rx_buf_len * j);
			}
			rx_buf->desc_ptr =
			    ioremap(rx_buf->phy_addr, shm_rx_buf_len);
			list_add_tail(&rx_buf->list,
				      &shm_layer[i].rx_empty_list);
		}

		/* Connect to the shared memory mailbox module. */
		shm_layer[i].mbx_ifc = mbxifc_get(shm_layer[i].mbx_name);
		if (!shm_layer[i].mbx_ifc) {
			printk(KERN_WARNING
			       "phyif_shm_init: can't find mailbox: %s.\n",
			       shm_layer[i].mbx_name);
			/* CLEANUP !!! */
			return CFGLU_ENXIO;
		}

		/* Fill in some info about ourselves. */
		shm_layer[i].mbx_client.cb = phyif_shm_mbx_msg_cb;
		shm_layer[i].mbx_client.priv = (void *) &shm_layer[i];

		shm_layer[i].mbx_ifc->init(&shm_layer[i].mbx_client,
					   shm_layer[i].mbx_ifc->priv);

		if ((i % 2) == 0) {
			/* Register physical interface. */
			result =
			    caifdev_phy_register(&shm_layer[i].shm_phy,
						 CFPHYTYPE_CAIF,
						 CFPHYPREF_UNSPECIFIED,
						 false, false);
		} else {
			/* Register loop interface. */
			result =
			    caifdev_phy_loop_register(&shm_layer
						      [i].shm_phy,
						      CFPHYTYPE_CAIF,
						      false, false);
		}

		if (result < 0) {
			printk(KERN_WARNING
			       "phyif_shm_init: err: %d, "
			       "can't register phy layer.\n",
			       result);
			/* CLEANUP !!! */
		}
	}
	return result;
}

static void __exit phyif_shm_exit(void)
{
	struct shm_buf_t *pbuf;
	uint8 i = 0;
	for (i = 0; i < SHM_INSTANCES; i++) {

		/* Unregister callbacks from mailbox interface. */
		shm_layer[i].mbx_client.cb = NULL;

		/* TODO: wait for completion or timeout */
		while (!(list_empty(&shm_layer[i].tx_pend_list))) {
			pbuf =
			    list_entry(shm_layer[i].tx_pend_list.next,
			       struct shm_buf_t, list);

			list_del(&pbuf->list);
			kfree(pbuf);
		}

		/* TODO: wait for completion or timeout */
		while (!(list_empty(&shm_layer[i].tx_full_list))) {
			pbuf =
			    list_entry(shm_layer[i].tx_full_list.next,
				       struct shm_buf_t, list);
			list_del(&pbuf->list);
			kfree(pbuf);
		}

		while (!(list_empty(&shm_layer[i].tx_empty_list))) {
			pbuf =
			    list_entry(shm_layer[i].tx_empty_list.next,
			       struct shm_buf_t, list);
			list_del(&pbuf->list);
			kfree(pbuf);
		}

		/* TODO: wait for completion or timeout */
		while (!(list_empty(&shm_layer[i].rx_full_list))) {
			pbuf =
			    list_entry(shm_layer[i].tx_full_list.next,
			       struct shm_buf_t, list);
			list_del(&pbuf->list);
			kfree(pbuf);
		}

		/* TODO: wait for completion or timeout */
		while (!(list_empty(&shm_layer[i].rx_pend_list))) {
			pbuf =
			    list_entry(shm_layer[i].tx_pend_list.next,
				       struct shm_buf_t, list);
			list_del(&pbuf->list);
			kfree(pbuf);
		}

		while (!(list_empty(&shm_layer[i].rx_empty_list))) {
			pbuf =
			    list_entry(shm_layer[i].rx_empty_list.next,
				       struct shm_buf_t, list);
			list_del(&pbuf->list);
			kfree(pbuf);
		}

		/* Destroy work queues. */
		destroy_workqueue(shm_layer[i].rx_work_queue);
		destroy_workqueue(shm_layer[i].sig_work_queue);
	}
}

module_init(phyif_shm_init);
module_exit(phyif_shm_exit);
