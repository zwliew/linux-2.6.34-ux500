/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Per Sigmond / Per.Sigmond@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

/* CAIF header files. */
#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cfcnfg.h>
#include <net/caif/generic/cfpkt.h>
#include <net/caif/generic/cffrml.h>
#include <net/caif/caif_chr.h>
#include <linux/caif/caif_config.h>
#include <net/caif/caif_actions.h>
MODULE_LICENSE("GPL");

#define COUNTER_DEBUG 0

#define CHNL_CHR_READ_QUEUE_HIGH 2000
#define CHNL_CHR_READ_QUEUE_LOW 100

static LIST_HEAD(caif_chrdev_list);
static spinlock_t list_lock;

#define CONN_STATE_OPEN_BIT           1
#define CONN_STATE_PENDING_BIT        2
#define CONN_REMOTE_SHUTDOWN_BIT      4
#define TX_FLOW_ON_BIT                1
#define RX_FLOW_ON_BIT                2

#define STATE_IS_OPEN(dev) test_bit(CONN_STATE_OPEN_BIT,\
				    (void *) &(dev)->conn_state)
#define STATE_IS_REMOTE_SHUTDOWN(dev) test_bit(CONN_REMOTE_SHUTDOWN_BIT,\
				    (void *) &(dev)->conn_state)
#define STATE_IS_PENDING(dev) test_bit(CONN_STATE_PENDING_BIT,\
				       (void *) &(dev)->conn_state)

#define SET_STATE_OPEN(dev) set_bit(CONN_STATE_OPEN_BIT,\
				    (void *) &(dev)->conn_state)
#define SET_STATE_CLOSED(dev) clear_bit(CONN_STATE_OPEN_BIT,\
					(void *) &(dev)->conn_state)
#define SET_PENDING_ON(dev) set_bit(CONN_STATE_PENDING_BIT,\
				    (void *) &(dev)->conn_state)
#define SET_PENDING_OFF(dev) clear_bit(CONN_STATE_PENDING_BIT,\
				       (void *) &(dev)->conn_state)
#define SET_REMOTE_SHUTDOWN(dev) set_bit(CONN_REMOTE_SHUTDOWN_BIT,\
				    (void *) &(dev)->conn_state)

#define SET_REMOTE_SHUTDOWN_OFF(dev) clear_bit(CONN_REMOTE_SHUTDOWN_BIT,\
				    (void *) &(dev)->conn_state)

#define RX_FLOW_IS_ON(dev) test_bit(RX_FLOW_ON_BIT,\
				    (void *) &(dev)->flow_state)
#define TX_FLOW_IS_ON(dev) test_bit(TX_FLOW_ON_BIT,\
				    (void *) &(dev)->flow_state)

#define SET_RX_FLOW_OFF(dev) clear_bit(RX_FLOW_ON_BIT,\
				       (void *) &(dev)->flow_state)
#define SET_RX_FLOW_ON(dev) set_bit(RX_FLOW_ON_BIT,\
				    (void *) &(dev)->flow_state)
#define SET_TX_FLOW_OFF(dev) clear_bit(TX_FLOW_ON_BIT,\
				       (void *) &(dev)->flow_state)
#define SET_TX_FLOW_ON(dev) set_bit(TX_FLOW_ON_BIT,\
				    (void *) &(dev)->flow_state)

#define CHR_READ_FLAG 0x01
#define CHR_WRITE_FLAG 0x02

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfsdir;
#include <linux/debugfs.h>

#endif

struct caif_char_dev {
	struct layer layer;
	u32 conn_state;
	u32 flow_state;
	struct cfpktq *pktq;
	char name[256];		/* Redundant! Already in struct miscdevice */
	struct miscdevice misc;
	int file_mode;
	struct caif_packet_funcs pktf;
	struct caif_channel_config config;
	/* Access to this struct and below layers */
	struct mutex mutex;
	int read_queue_len;
	spinlock_t read_queue_len_lock;
	wait_queue_head_t read_wq;
	wait_queue_head_t mgmt_wq;
	/* List of misc test devices */
	struct list_head list_field;
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_device_dir;
	atomic_t num_open;
	atomic_t num_close;
	atomic_t num_init;
	atomic_t num_init_resp;
	atomic_t num_init_fail_resp;
	atomic_t num_deinit;
	atomic_t num_deinit_resp;
	atomic_t num_remote_shutdown_ind;
	atomic_t num_tx_flow_off_ind;
	atomic_t num_tx_flow_on_ind;
	atomic_t num_rx_flow_off;
	atomic_t num_rx_flow_on;
#endif
#if COUNTER_DEBUG
	unsigned long counter;
	int mismatch_reported;
#endif
};

static void drain_queue(struct caif_char_dev *dev);

/* Packet Receive Callback function called from CAIF Stack */
static int caif_chrrecv_cb(struct layer *layr, struct cfpkt *pkt)
{
	struct caif_char_dev *dev;
	int read_queue_high;
#if COUNTER_DEBUG
	unsigned long *data_p;
#endif
	dev = container_of(layr, struct caif_char_dev, layer);

	pr_debug("CAIF: %s(): data received: %d bytes.\n",
		       __func__, dev->pktf.cfpkt_getlen(pkt));

	/* NOTE: This function may be called in Tasklet context! */
#if COUNTER_DEBUG
	dev->pktf.cfpkt_raw_extract(pkt, (void **) &data_p, 0);

	if (data_p[0] == 1) {
		dev->counter = data_p[0];
		dev->mismatch_reported = 0;
	}

	if ((dev->counter != data_p[0]) && !dev->mismatch_reported) {
		pr_warning("CAIF: %s(): WARNING - caif_chrrecv_cb(): "
			   "sequence: expected %ld, got %ld\n",
			__func__, dev->counter, data_p[0]);
		dev->mismatch_reported = 1;
	}

	if (!(dev->counter % 100000))
		pr_debug("CAIF: %s(): %ld\n", __func__, dev->counter);

	dev->counter++;
#endif

	/* The queue has its own lock */
	dev->pktf.cfpkt_queue(dev->pktq, pkt, 0);
	spin_lock(&dev->read_queue_len_lock);
	dev->read_queue_len++;
	read_queue_high = (dev->read_queue_len > CHNL_CHR_READ_QUEUE_HIGH);
	spin_unlock(&dev->read_queue_len_lock);

	if (RX_FLOW_IS_ON(dev) && read_queue_high) {

#ifdef CONFIG_DEBUG_FS
		atomic_inc(&dev->num_rx_flow_off);
#endif
		SET_RX_FLOW_OFF(dev);

		/* Send flow off (NOTE: must not sleep) */
		pr_debug("CAIF: %s(): sending flow OFF"
			 " (queue len = %d)\n",
			 __func__,
			 dev->read_queue_len);
		caif_assert(dev->layer.dn);
		caif_assert(dev->layer.dn->ctrlcmd);
		(void) dev->layer.dn->modemcmd(dev->layer.dn,
					       CAIF_MODEMCMD_FLOW_OFF_REQ);
	}

	/* Signal reader that data is available. */
	wake_up_interruptible(&dev->read_wq);

	return 0;
}

/* Packet Flow Control Callback function called from CAIF */
static void caif_chrflowctrl_cb(struct layer *layr, enum caif_ctrlcmd flow, int phyid)
{
	struct caif_char_dev *dev;


	/* NOTE: This function may be called in Tasklet context! */
	pr_debug("CAIF: %s(): AT flowctrl func called flow: %s.\n",
		 __func__,
		 flow == CAIF_CTRLCMD_FLOW_ON_IND ? "ON" :
		 flow == CAIF_CTRLCMD_FLOW_OFF_IND ? "OFF" :
		 flow == CAIF_CTRLCMD_INIT_RSP ? "INIT_RSP" :
		 flow == CAIF_CTRLCMD_DEINIT_RSP ? "DEINIT_RSP" :
		 flow == CAIF_CTRLCMD_INIT_FAIL_RSP ? "INIT_FAIL_RSP" :
		 flow ==
		 CAIF_CTRLCMD_REMOTE_SHUTDOWN_IND ? "REMOTE_SHUTDOWN" :
		 "UKNOWN CTRL COMMAND");

	dev = container_of(layr, struct caif_char_dev, layer);

	switch (flow) {
	case CAIF_CTRLCMD_FLOW_ON_IND:
		pr_debug("CAIF: %s(): CAIF_CTRLCMD_FLOW_ON_IND\n",
			      __func__);
#ifdef CONFIG_DEBUG_FS
		atomic_inc(&dev->num_tx_flow_on_ind);
#endif
		/* Signal reader that data is available. */
		SET_TX_FLOW_ON(dev);
		wake_up_interruptible(&dev->mgmt_wq);
		break;

	case CAIF_CTRLCMD_FLOW_OFF_IND:
#ifdef CONFIG_DEBUG_FS
		atomic_inc(&dev->num_tx_flow_off_ind);
#endif
		pr_debug("CAIF: %s(): CAIF_CTRLCMD_FLOW_OFF_IND\n",
			  __func__);
		SET_TX_FLOW_OFF(dev);
		break;

	case CAIF_CTRLCMD_INIT_RSP:
		pr_debug("CAIF: %s(): CAIF_CTRLCMD_INIT_RSP\n",
			  __func__);
#ifdef CONFIG_DEBUG_FS
		atomic_inc(&dev->num_init_resp);
#endif
		/* Signal reader that data is available. */
		caif_assert(STATE_IS_OPEN(dev));
		SET_PENDING_OFF(dev);
		SET_TX_FLOW_ON(dev);
		wake_up_interruptible(&dev->mgmt_wq);
		break;

	case CAIF_CTRLCMD_DEINIT_RSP:
		pr_debug("CAIF: %s(): CAIF_CTRLCMD_DEINIT_RSP\n",
			  __func__);
#ifdef CONFIG_DEBUG_FS
		atomic_inc(&dev->num_deinit_resp);
#endif
		caif_assert(!STATE_IS_OPEN(dev));
		SET_PENDING_OFF(dev);
		wake_up_interruptible(&dev->mgmt_wq);
		break;

	case CAIF_CTRLCMD_INIT_FAIL_RSP:
		pr_debug("CAIF: %s(): CAIF_CTRLCMD_INIT_FAIL_RSP\n",
			  __func__);
#ifdef CONFIG_DEBUG_FS
		atomic_inc(&dev->num_init_fail_resp);
#endif
		caif_assert(STATE_IS_OPEN(dev));
		SET_STATE_CLOSED(dev);
		SET_PENDING_OFF(dev);
		SET_TX_FLOW_OFF(dev);
		wake_up_interruptible(&dev->mgmt_wq);
		break;

	case CAIF_CTRLCMD_REMOTE_SHUTDOWN_IND:
		pr_debug("CAIF: %s(): CAIF_CTRLCMD_REMOTE_SHUTDOWN_IND\n",
			  __func__);
#ifdef CONFIG_DEBUG_FS
		atomic_inc(&dev->num_remote_shutdown_ind);
#endif
		SET_TX_FLOW_OFF(dev);
		SET_REMOTE_SHUTDOWN(dev);
		SET_TX_FLOW_OFF(dev);

		drain_queue(dev);
		SET_RX_FLOW_ON(dev);
		dev->file_mode = 0;

		wake_up_interruptible(&dev->mgmt_wq);
		wake_up_interruptible(&dev->read_wq);
		break;

	default:
		pr_debug("CAIF: %s(): Unexpected flow command %d\n",
			 __func__, flow);
	}
}

/* Device Read function called from Linux kernel */
ssize_t caif_chrread(struct file *filp, char __user *buf, size_t count,
		     loff_t *f_pos)
{
	struct cfpkt *pkt = NULL;
	unsigned char *rxbuf = NULL;
	size_t len;
	int result;
	struct caif_char_dev *dev = filp->private_data;
	ssize_t ret = -EIO;
	int read_queue_low;


	if (dev == NULL) {
		pr_debug("CAIF: %s(): private_data not set!\n",
			  __func__);
		return -EBADFD;
	}

	/* I want to be alone on dev (except status and queue) */
	if (mutex_lock_interruptible(&dev->mutex)) {
		pr_debug("CAIF: %s(): mutex_lock_interruptible got signalled\n",
			 __func__);
		return -ERESTARTSYS;
	}

	caif_assert(dev->pktq);

	if (!STATE_IS_OPEN(dev)) {
		/* Device is closed or closing. */
		if (!STATE_IS_PENDING(dev)) {
			pr_debug("CAIF: %s(): device is closed (by remote)\n",
				 __func__);
			ret = -EPIPE;
		} else {
			pr_debug("CAIF: %s(): device is closing...\n",
				 __func__);
			ret = -EBADF;
		}
		goto read_error;
	}

	/* Device is open or opening. */
	if (STATE_IS_PENDING(dev)) {
		pr_debug("CAIF: %s(): device is opening...\n",
			 __func__);

		if (filp->f_flags & O_NONBLOCK) {
			/* We can't block. */
			pr_debug("CAIF: %s(): state pending and O_NONBLOCK\n",
				 __func__);
			ret = -EAGAIN;
			goto read_error;
		}

		/* Blocking mode; state is pending and we need to wait
		 * for its conclusion. (Shutdown_ind set pending off.)
		 */
		result =
		    wait_event_interruptible(dev->mgmt_wq,
					     !STATE_IS_PENDING(dev));
		if (result == -ERESTARTSYS) {
			pr_debug("CAIF: %s(): wait_event_interruptible"
				 " woken by a signal (1)", __func__);
			ret = -ERESTARTSYS;
			goto read_error;
		}
	}
	if (STATE_IS_REMOTE_SHUTDOWN(dev)) {
		pr_debug("CAIF: %s(): received remote_shutdown indication\n",
			 __func__);
		ret = -ESHUTDOWN;
		goto read_error;
	}

	/* Block if we don't have any received buffers.
	 * The queue has its own lock.
	 */
	while ((pkt = dev->pktf.cfpkt_qpeek(dev->pktq)) == NULL) {

		if (filp->f_flags & O_NONBLOCK) {
			pr_debug("CAIF: %s(): O_NONBLOCK\n", __func__);
			ret = -EAGAIN;
			goto read_error;
		}
		pr_debug("CAIF: %s(): wait_event\n", __func__);

		/* Let writers in. */
		mutex_unlock(&dev->mutex);

		/* Block reader until data arrives or device is closed. */
		if (wait_event_interruptible(dev->read_wq,
					     dev->pktf.cfpkt_qpeek(dev->pktq)
					     || STATE_IS_REMOTE_SHUTDOWN(dev)
					     || !STATE_IS_OPEN(dev)) ==
		    -ERESTARTSYS) {
			pr_debug("CAIF: %s():_event_interruptible woken by "
			         "a signal, signal_pending(current) = %d\n",
				__func__,
			        signal_pending(current));
			return -ERESTARTSYS;
		}

		pr_debug("CAIF: %s(): awake\n", __func__);
		if (STATE_IS_REMOTE_SHUTDOWN(dev)) {
			pr_debug("CAIF: %s received remote_shutdown\n",
				 __func__);
			ret = -ESHUTDOWN;
			goto read_error;
		}

		/* I want to be alone on dev (except status and queue). */
		if (mutex_lock_interruptible(&dev->mutex)) {
			pr_debug("CAIF: %s():"
				 "mutex_lock_interruptible got signalled\n",
				 __func__);
			return -ERESTARTSYS;
		}

		if (!STATE_IS_OPEN(dev)) {
			/* Someone closed the link, report error. */
			pr_debug("CAIF: %s(): remote end shutdown!\n",
				 __func__);
			ret = -EPIPE;
			goto read_error;
		}
	}

	/* The queue has its own lock. */
	len = dev->pktf.cfpkt_getlen(pkt);

	/* Check max length that can be copied. */
	if (len > count) {
		pr_debug("CAIF: %s(): user buffer too small\n", __func__);
		ret = -EINVAL;
		goto read_error;
	}

	/* Get packet from queue.
	 * The queue has its own lock.
	 */
	pkt = dev->pktf.cfpkt_dequeue(dev->pktq);

	spin_lock(&dev->read_queue_len_lock);
	dev->read_queue_len--;
	read_queue_low = (dev->read_queue_len < CHNL_CHR_READ_QUEUE_LOW);
	spin_unlock(&dev->read_queue_len_lock);

	if (!RX_FLOW_IS_ON(dev) && read_queue_low) {
#ifdef CONFIG_DEBUG_FS
		atomic_inc(&dev->num_rx_flow_on);
#endif
		SET_RX_FLOW_ON(dev);

		/* Send flow on. */
		pr_debug("CAIF: %s(): sending flow ON (queue len = %d)\n",
			__func__,
			dev->read_queue_len);
		caif_assert(dev->layer.dn);
		caif_assert(dev->layer.dn->ctrlcmd);
		(void) dev->layer.dn->modemcmd(dev->layer.dn,
					       CAIF_MODEMCMD_FLOW_ON_REQ);

		caif_assert(dev->read_queue_len >= 0);
	}

	result = dev->pktf.cfpkt_raw_extract(pkt, (void **) &rxbuf, len);

	caif_assert(result >= 0);

	if (result < 0) {
		pr_debug("CAIF: %s(): cfpkt_raw_extract failed\n", __func__);
		dev->pktf.cfpkt_destroy(pkt);
		ret = -EINVAL;
		goto read_error;
	}

	/* Copy data from the RX buffer to the user buffer. */
	if (copy_to_user(buf, rxbuf, len)) {
		pr_debug("CAIF: %s(): copy_to_user returned non zero", __func__);
		dev->pktf.cfpkt_destroy(pkt);
		ret = -EINVAL;
		goto read_error;
	}

	/* Free packet. */
	dev->pktf.cfpkt_destroy(pkt);

	/* Let the others in. */
	mutex_unlock(&dev->mutex);
	return len;

read_error:
	mutex_unlock(&dev->mutex);
	return ret;
}

/* Device write function called from Linux kernel (misc device) */
ssize_t caif_chrwrite(struct file *filp, const char __user *buf,
		      size_t count, loff_t *f_pos)
{
	struct cfpkt *pkt = NULL;
	struct caif_char_dev *dev = filp->private_data;
	unsigned char *txbuf;
	ssize_t ret = -EIO;
	int result;

	if (dev == NULL) {
		pr_debug("CAIF: %s(): private_data not set!\n",
			      __func__);
		ret = -EBADFD;
		goto write_error_no_unlock;
	}

	if (count > CAIF_MAX_PAYLOAD_SIZE) {
		pr_debug("CAIF: %s() buffer too long\n", __func__);
		ret = -EINVAL;
		goto write_error_no_unlock;
	}

	/* I want to be alone on dev (except status and queue). */
	if (mutex_lock_interruptible(&dev->mutex)) {
		pr_debug("CAIF: %s():"
			 "mutex_lock_interruptible got signalled\n",
			__func__);
		ret = -ERESTARTSYS;
		goto write_error_no_unlock;
	}

	caif_assert(dev->pktq);

	if (!STATE_IS_OPEN(dev)) {
		/* Device is closed or closing. */
		if (!STATE_IS_PENDING(dev)) {
			pr_debug("CAIF: %s(): device is closed (by remote)\n",
				 __func__);
			ret = -EPIPE;
		} else {
			pr_debug("CAIF: %s(): device is closing...\n", __func__);
			ret = -EBADF;
		}
		goto write_error;
	}

	/* Device is open or opening. */
	if (STATE_IS_PENDING(dev)) {
		pr_debug("CAIF: device is opening...\n");

		if (filp->f_flags & O_NONBLOCK) {
			/* We can't block */
			pr_debug("CAIF: %s(): state pending and O_NONBLOCK\n",
				 __func__);
			ret = -EAGAIN;
			goto write_error;
		}

		/* Blocking mode; state is pending and we need to wait
		 * for its conclusion. (Shutdown_ind set pending off.)
		 */
		result =
		    wait_event_interruptible(dev->mgmt_wq,
					     !STATE_IS_PENDING(dev));
		if (result == -ERESTARTSYS) {
			pr_debug("CAIF: %s(): wait_event_interruptible"
				 " woken by a signal (1)", __func__);
			ret = -ERESTARTSYS;
			goto write_error;
		}
	}
	if (STATE_IS_REMOTE_SHUTDOWN(dev)) {
		pr_debug("CAIF: received remote_shutdown indication\n");
		ret = -ESHUTDOWN;
		goto write_error;
	}



	if (!TX_FLOW_IS_ON(dev)) {

		/* Flow is off. Check non-block flag. */
		if (filp->f_flags & O_NONBLOCK) {
			pr_debug("CAIF: %s(): O_NONBLOCK and tx flow off",
				 __func__);
			ret = -EAGAIN;
			goto write_error;
		}

		/* Let readers in. */
		mutex_unlock(&dev->mutex);

		/* Wait until flow is on or device is closed. */
		if (wait_event_interruptible(dev->mgmt_wq, TX_FLOW_IS_ON(dev)
					     || !STATE_IS_OPEN(dev)
					     || STATE_IS_REMOTE_SHUTDOWN(dev)
				) == -ERESTARTSYS) {
			pr_debug("CAIF: %s(): wait_event_interruptible"
				 " woken by a signal (1)", __func__);
			ret = -ERESTARTSYS;
			goto write_error_no_unlock;
		}

		/* I want to be alone on dev (except status and queue). */
		if (mutex_lock_interruptible(&dev->mutex)) {
			pr_debug("CAIF: %s():"
				 "mutex_lock_interruptible got signalled\n",
				 __func__);
			ret = -ERESTARTSYS;
			goto write_error_no_unlock;
		}

		if (!STATE_IS_OPEN(dev)) {
			/* Someone closed the link, report error. */
			pr_debug("CAIF: %s(): remote end shutdown!\n",
				      __func__);
			ret = -EPIPE;
			goto write_error;
		}
		if (STATE_IS_REMOTE_SHUTDOWN(dev)) {
			pr_debug("CAIF: received remote_shutdown indication\n");
			ret = -ESHUTDOWN;
			goto write_error;
		}
	}

	/* Create packet, buf=NULL means no copying. */
	pkt = dev->pktf.cfpkt_create_xmit_pkt((const unsigned char *) NULL,
					      count);
	if (pkt == NULL) {
		pr_debug("CAIF: %s():cfpkt_create_pkt returned NULL\n",
			 __func__);
		ret = -EIO;
		goto write_error;
	}

	if (dev->pktf.cfpkt_raw_append(pkt, (void **) &txbuf, count) < 0) {
		pr_debug("CAIF: %s(): cfpkt_raw_append failed\n", __func__);
		dev->pktf.cfpkt_destroy(pkt);
		ret = -EINVAL;
		goto write_error;
	}

	/* Copy data into buffer. */
	if (copy_from_user(txbuf, buf, count)) {
		pr_debug("CAIF: %s(): copy_from_user returned non zero.\n",
			  __func__);
		dev->pktf.cfpkt_destroy(pkt);
		ret = -EINVAL;
		goto write_error;
	}

	/* Send the packet down the stack. */
	caif_assert(dev->layer.dn);
	caif_assert(dev->layer.dn->transmit);

	do {
		ret = dev->layer.dn->transmit(dev->layer.dn, pkt);

		/*
		 * FIXME: If ret == -EAGAIN we may spin in a tight loop
		 * until the CAIF phy device can take more packets.
		 * we should back off before re-trying (sleep).
		 */
		if (likely(ret >= 0) || (ret != -EAGAIN))
			break;

		/* EAGAIN - retry. */
		if (filp->f_flags & O_NONBLOCK) {
			pr_debug("CAIF: %s(): NONBLOCK and transmit failed,"
				 " error = %d\n", __func__, ret);
			ret = -EAGAIN;
			goto write_error;
		}

		/* Let readers in. */
		mutex_unlock(&dev->mutex);

		/* Wait until flow is on or device is closed. */
		if (wait_event_interruptible(dev->mgmt_wq, TX_FLOW_IS_ON(dev)
				     || !STATE_IS_OPEN(dev)
				     || STATE_IS_REMOTE_SHUTDOWN(dev)) ==
		    -ERESTARTSYS) {
			pr_debug("CAIF: %s(): wait_event_interruptible"
				 " woken by a signal (1)", __func__);
			ret = -ERESTARTSYS;
			goto write_error_no_unlock;
		}

		/* I want to be alone on dev (except status and queue). */
		if (mutex_lock_interruptible(&dev->mutex)) {
			pr_debug("CAIF: %s():"
				 "mutex_lock_interruptible got signalled\n",
				 __func__);
			ret = -ERESTARTSYS;
			goto write_error_no_unlock;
		}

	} while (ret == -EAGAIN);

	if (ret < 0) {
		dev->pktf.cfpkt_destroy(pkt);
		pr_debug("CAIF: transmit failed, error = %d\n",
			      ret);

		goto write_error;
	}

	mutex_unlock(&dev->mutex);
	return count;

write_error:
	mutex_unlock(&dev->mutex);
write_error_no_unlock:
	return ret;
}

static unsigned int caif_chrpoll(struct file *filp, poll_table *waittab)
{
	struct caif_char_dev *dev = filp->private_data;
	unsigned int mask = 0;


	if (dev == NULL) {
		pr_debug("CAIF: %s(): private_data not set!\n",
			      __func__);
		return -EBADFD;
	}

	/* I want to be alone on dev (except status and queue). */
	if (mutex_lock_interruptible(&dev->mutex)) {
		pr_debug("CAIF: %s():"
			 "mutex_lock_interruptible got signalled\n",
			__func__);
		goto poll_error;
	}

	caif_assert(dev->pktq);

	if (STATE_IS_REMOTE_SHUTDOWN(dev)) {
		pr_debug("CAIF: %s(): not open\n", __func__);
		goto poll_error;
	}

	if (!STATE_IS_OPEN(dev)) {
		pr_debug("CAIF: %s(): not open\n", __func__);
		goto poll_error;
	}

	poll_wait(filp, &dev->read_wq, waittab);

	if (dev->pktf.cfpkt_qpeek(dev->pktq) != NULL)
		mask |= (POLLIN | POLLRDNORM);


	if (TX_FLOW_IS_ON(dev))
		mask |= (POLLOUT | POLLWRNORM);

	mutex_unlock(&dev->mutex);
	pr_debug("CAIF: %s(): caif_chrpoll mask=0x%04x...\n",
		      __func__, mask);

	return mask;

poll_error:
	mask |= POLLERR;
	mutex_unlock(&dev->mutex);
	return mask;
}

/* Usage:
 * minor >= 0 : find from minor
 * minor < 0 and name == name : find from name
 * minor < 0 and name == NULL : get first
 */

static struct caif_char_dev *find_device(int minor, char *name,
					 int remove_from_list)
{
	struct list_head *list_node;
	struct list_head *n;
	struct caif_char_dev *dev = NULL;
	struct caif_char_dev *tmp;
	spin_lock(&list_lock);
	pr_debug("CAIF: %s(): start looping \n", __func__);
	list_for_each_safe(list_node, n, &caif_chrdev_list) {
		tmp = list_entry(list_node, struct caif_char_dev, list_field);
		if (minor >= 0) {	/* find from minor */
			if (tmp->misc.minor == minor)
				dev = tmp;

		} else if (name) {	/* find from name */
			if (!strncmp(tmp->name, name, sizeof(tmp->name)))
				dev = tmp;
		} else {	/* take first */
			dev = tmp;
		}

		if (dev) {
			pr_debug("CAIF: %s(): match %d, %s \n",
				      __func__, minor, name);
			if (remove_from_list)
				list_del(list_node);
			break;
		}
	}
	spin_unlock(&list_lock);
	return dev;
}


static void drain_queue(struct caif_char_dev *dev)
{
	struct cfpkt *pkt;

	/* Empty the queue. */
	do {
		/* The queue has its own lock. */
		pkt = dev->pktf.cfpkt_dequeue(dev->pktq);

		if (!pkt)
			break;

		pr_debug("CAIF: %s(): freeing packet from read queue\n",
		     __func__);
		dev->pktf.cfpkt_destroy(pkt);

	} while (1);

	spin_lock(&dev->read_queue_len_lock);
	dev->read_queue_len = 0;
	spin_unlock(&dev->read_queue_len_lock);
}

int caif_chropen(struct inode *inode, struct file *filp)
{
	struct caif_char_dev *dev = NULL;
	int result = -1;
	int minor = iminor(inode);
	int mode = 0;
	int ret = -EIO;


	dev = find_device(minor, NULL, 0);

	if (dev == NULL) {
		pr_debug("CAIF: [%s] COULD NOT FIND DEVICE\n", __func__);
		return -EBADF;
	}

	pr_debug("CAIF: dev=%p OPEN=%d, TX_FLOW=%d, RX_FLOW=%d\n", dev,
		      STATE_IS_OPEN(dev),
		      TX_FLOW_IS_ON(dev), RX_FLOW_IS_ON(dev));

	pr_debug("CAIF: get mutex\n");

	/* I want to be alone on dev (except status and queue). */
	if (mutex_lock_interruptible(&dev->mutex)) {
		pr_debug("CAIF: %s():"
			 "mutex_lock_interruptible got signalled\n",
			__func__);
		return -ERESTARTSYS;
	}
#ifdef CONFIG_DEBUG_FS
	atomic_inc(&dev->num_open);
#endif
	filp->private_data = dev;

	switch (filp->f_flags & O_ACCMODE) {
	case O_RDONLY:
		mode = CHR_READ_FLAG;
		break;
	case O_WRONLY:
		mode = CHR_WRITE_FLAG;
		break;
	case O_RDWR:
		mode = CHR_READ_FLAG | CHR_WRITE_FLAG;
		break;
	}

	/* If device is not open, make sure device is in fully closed state. */
	if (!STATE_IS_OPEN(dev)) {
		/* Has link close response been received
		 * (if we ever sent it)?
		 */
		if (STATE_IS_PENDING(dev)) {
			/* Still waiting for close response from remote.
			 * If opened non-blocking, report "would block".
			 */
			if (filp->f_flags & O_NONBLOCK) {
				pr_debug("CAIF: O_NONBLOCK && close pending\n");
				ret = -EAGAIN;
				goto open_error;
			}

			pr_debug("CAIF: %s(): "
				 "wait for close response from remote...\n",
				 __func__);

			/* Blocking mode; close is pending and we need to wait
			 * for its conclusion. (Shutdown_ind set pending off.)
			 */
			result =
			    wait_event_interruptible(dev->mgmt_wq,
						     !STATE_IS_PENDING(dev));
			if (result == -ERESTARTSYS) {
				pr_debug("CAIF: %s(): wait_event_interruptible"
					 " woken by a signal (1)", __func__);
				ret = -ERESTARTSYS;
				goto open_error;
			}
		}
	}

	/* Device is now either closed, pending open or open */
	if (STATE_IS_OPEN(dev) && !STATE_IS_PENDING(dev)) {
		/* Open */
		pr_debug("CAIF: %s():"
			 " Device is already opened (dev=%p) check access "
		         "f_flags = 0x%x file_mode = 0x%x\n",
		         __func__, dev, mode, dev->file_mode);

		if (mode & dev->file_mode) {
			pr_debug("CAIF: %s():Access mode already in use 0x%x\n",
			     __func__, mode);
			ret = -EBUSY;
			goto open_error;
		}
	} else {
		/* We are closed or pending open.
		 * If closed:       send link setup
		 * If pending open: link setup already sent (we could have been
		 *                  interrupted by a signal last time)
		 */
		if (!STATE_IS_OPEN(dev)) {
			/* First opening of file; connect lower layers: */

			dev->layer.receive = caif_chrrecv_cb;

			SET_STATE_OPEN(dev);
			SET_PENDING_ON(dev);

			/* Register this channel. */
			result =
			    caifdev_adapt_register(&dev->config, &dev->layer);
			if (result < 0) {
				pr_debug("CAIF: %s():can't register channel\n",
					 __func__);
				ret = -EIO;
				SET_STATE_CLOSED(dev);
				SET_PENDING_OFF(dev);
				goto open_error;
			}
#ifdef CONFIG_DEBUG_FS
			atomic_inc(&dev->num_init);
#endif
		}

		/* If opened non-blocking, report "success".
		 */
		if (filp->f_flags & O_NONBLOCK) {
			pr_debug("CAIF: %s(): O_NONBLOCK success\n", __func__);
			ret = 0;
			goto open_success;
		}

		pr_debug("CAIF: WAIT FOR CONNECT RESPONSE \n");
		result =
		    wait_event_interruptible(dev->mgmt_wq,
					     !STATE_IS_PENDING(dev));
		if (result == -ERESTARTSYS) {
			pr_debug("CAIF: %s(): wait_event_interruptible"
				 " woken by a signal (2)", __func__);
			ret = -ERESTARTSYS;
			goto open_error;
		}

		if (!STATE_IS_OPEN(dev)) {
			/* Lower layers said "no". */
			pr_debug("CAIF: %s(): caif_chropen: CLOSED RECEIVED\n",
			     __func__);
			ret = -EPIPE;
			goto open_error;
		}

		pr_debug("CAIF: %s(): caif_chropen: CONNECT RECEIVED\n",
			      __func__);


		SET_RX_FLOW_ON(dev);

		/* Send flow on. */
		pr_debug("CAIF: %s(): sending flow ON at open\n", __func__);

		caif_assert(dev->layer.dn);
		caif_assert(dev->layer.dn->ctrlcmd);
		(void) dev->layer.dn->modemcmd(dev->layer.dn,
					       CAIF_MODEMCMD_FLOW_ON_REQ);

	}
open_success:
	/* Open is OK. */
	dev->file_mode |= mode;

	pr_debug("CAIF: %s(): Open - file mode = %x\n",
		      __func__, dev->file_mode);

	pr_debug("CAIF: %s(): CONNECTED \n", __func__);

	mutex_unlock(&dev->mutex);
	return 0;

open_error:
	mutex_unlock(&dev->mutex);
	return ret;
}

int caif_chrrelease(struct inode *inode, struct file *filp)
{
	struct caif_char_dev *dev = NULL;
	int minor = iminor(inode);
	int result;
	int mode = 0;
	int tx_flow_state_was_on;


	dev = find_device(minor, NULL, 0);
	if (dev == NULL) {
		pr_debug("CAIF: %s(): Could not find device\n", __func__);
		return -EBADF;
	}

	/* I want to be alone on dev (except status queue). */
	if (mutex_lock_interruptible(&dev->mutex)) {
		pr_debug("CAIF: %s():"
			 "mutex_lock_interruptible got signalled\n",
			__func__);
		return -ERESTARTSYS;
	}
#ifdef CONFIG_DEBUG_FS
	atomic_inc(&dev->num_close);
#endif

	/* Is the device open? */
	if (!STATE_IS_OPEN(dev)) {
		pr_debug("CAIF: %s(): Device not open (dev=%p) \n",
			      __func__, dev);
		mutex_unlock(&dev->mutex);
		return 0;
	}

	/* Is the device waiting for link setup response? */
	if (STATE_IS_PENDING(dev)) {
		pr_debug("CAIF: %s(): Device is open pending (dev=%p) \n",
			      __func__, dev);
		mutex_unlock(&dev->mutex);
		/* What to return here? Seems that EBADF is the closest :-| */
		return -EBADF;
	}

	switch (filp->f_flags & O_ACCMODE) {
	case O_RDONLY:
		mode = CHR_READ_FLAG;
		break;
	case O_WRONLY:
		mode = CHR_WRITE_FLAG;
		break;
	case O_RDWR:
		mode = CHR_READ_FLAG | CHR_WRITE_FLAG;
		break;
	}

	dev->file_mode &= ~mode;
	if (dev->file_mode) {
		pr_debug("CAIF: %s(): Device is kept open by someone else,"
		         " don't close. CAIF connection - file_mode = %x\n",
			 __func__, dev->file_mode);
		mutex_unlock(&dev->mutex);
		return 0;
	}

	/* IS_CLOSED have double meaning:
	 * 1) Spontanous Remote Shutdown Request.
	 * 2) Ack on a channel teardown(disconnect)
	 * Must clear bit, in case we previously received
	 * a remote shudown request.
	 */

	SET_STATE_CLOSED(dev);
	SET_PENDING_ON(dev);
	tx_flow_state_was_on = TX_FLOW_IS_ON(dev);
	SET_TX_FLOW_OFF(dev);
	result = caifdev_adapt_unregister(&dev->layer);

	if (result < 0) {
		pr_debug("CAIF: %s(): caifdev_adapt_unregister() failed\n",
			 __func__);
		SET_STATE_CLOSED(dev);
		SET_PENDING_OFF(dev);
		SET_TX_FLOW_OFF(dev);
		mutex_unlock(&dev->mutex);
		return -EIO;
	}

	if (STATE_IS_REMOTE_SHUTDOWN(dev)) {
		SET_PENDING_OFF(dev);
		SET_REMOTE_SHUTDOWN_OFF(dev);
	}
#ifdef CONFIG_DEBUG_FS
	atomic_inc(&dev->num_deinit);
#endif

	/* We don't wait for close response here. Close pending state will be
	 * cleared by flow control callback when response arrives.
	 */

	/* Empty the queue */
	drain_queue(dev);
	SET_RX_FLOW_ON(dev);
	dev->file_mode = 0;

	mutex_unlock(&dev->mutex);
	return 0;
}

const struct file_operations caif_chrfops = {
	.owner = THIS_MODULE,
	.read = caif_chrread,
	.write = caif_chrwrite,
	.open = caif_chropen,
	.release = caif_chrrelease,
	.poll = caif_chrpoll,
};

int chrdev_create(struct caif_channel_create_action *action)
{
	struct caif_char_dev *dev = NULL;
	int result;

	/* Allocate device */
	dev = kmalloc(sizeof(*dev), GFP_KERNEL);

	if (!dev) {
		pr_err("CAIF: %s kmalloc failed.\n", __func__);
		return -ENOMEM;
	}

	pr_debug("CAIF: %s(): dev=%p \n", __func__, dev);
	memset(dev, 0, sizeof(*dev));

	mutex_init(&dev->mutex);
	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->mgmt_wq);
	spin_lock_init(&dev->read_queue_len_lock);

	/* Fill in some information concerning the misc device. */
	dev->misc.minor = MISC_DYNAMIC_MINOR;
	strncpy(dev->name, action->name.name, sizeof(dev->name));
	dev->misc.name = dev->name;
	dev->misc.fops = &caif_chrfops;

	/* Register the device. */
	result = misc_register(&dev->misc);

	/* Lock in order to try to stop someone from opening the device
	 * too early. The misc device has its own lock. We cannot take our
	 * lock until misc_register() is finished, because in open() the
	 * locks are taken in this order (misc first and then dev).
	 * So anyone managing to open the device between the misc_register
	 * and the mutex_lock will get a "device not found" error. Don't
	 * think it can be avoided.
	 */
	if (mutex_lock_interruptible(&dev->mutex)) {
		pr_debug("CAIF: %s():"
			 "mutex_lock_interruptible got signalled\n",
			__func__);
		return -ERESTARTSYS;
	}

	if (result < 0) {
		pr_err("CAIF: chnl_chr: error - %d, can't register misc.\n",
			      result);
		mutex_unlock(&dev->mutex);
		goto err_failed;
	}

	dev->pktf = cfcnfg_get_packet_funcs();

	dev->pktq = dev->pktf.cfpktq_create();
	if (!dev->pktq) {
		pr_err("CAIF: %s(): queue create failed.\n", __func__);
		result = -ENOMEM;
		mutex_unlock(&dev->mutex);
		misc_deregister(&dev->misc);
		goto err_failed;
	}

	strncpy(action->name.name, dev->misc.name, sizeof(action->name.name)-1);
	action->name.name[sizeof(action->name.name)-1] = '\0';
	action->major = MISC_MAJOR;
	action->minor = dev->misc.minor;

	dev->config = action->config;
	pr_debug("CAIF: dev: Registered dev with name=%s minor=%d, dev=%p\n",
		      dev->misc.name, dev->misc.minor, dev->misc.this_device);

	dev->layer.ctrlcmd = caif_chrflowctrl_cb;
	SET_STATE_CLOSED(dev);
	SET_PENDING_OFF(dev);
	SET_TX_FLOW_OFF(dev);
	SET_RX_FLOW_ON(dev);

	/* Add the device. */
	spin_lock(&list_lock);
	list_add(&dev->list_field, &caif_chrdev_list);
	spin_unlock(&list_lock);

	pr_debug("CAIF: %s(): Creating device %s\n",
		 __func__, action->name.name);

#ifdef CONFIG_DEBUG_FS
	if (debugfsdir != NULL) {
		dev->debugfs_device_dir =
		    debugfs_create_dir(dev->misc.name, debugfsdir);
		debugfs_create_u32("conn_state", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir, &dev->conn_state);
		debugfs_create_u32("flow_state", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir, &dev->flow_state);
		debugfs_create_u32("num_open", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir,
				   (u32 *) &dev->num_open);
		debugfs_create_u32("num_close", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir,
				   (u32 *) &dev->num_close);
		debugfs_create_u32("num_init", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir,
				   (u32 *) &dev->num_init);
		debugfs_create_u32("num_init_resp", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir,
				   (u32 *) &dev->num_init_resp);
		debugfs_create_u32("num_init_fail_resp", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir,
				   (u32 *) &dev->num_init_fail_resp);
		debugfs_create_u32("num_deinit", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir,
				   (u32 *) &dev->num_deinit);
		debugfs_create_u32("num_deinit_resp", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir,
				   (u32 *) &dev->num_deinit_resp);
		debugfs_create_u32("num_remote_shutdown_ind",
				   S_IRUSR | S_IWUSR, dev->debugfs_device_dir,
				   (u32 *) &dev->num_remote_shutdown_ind);
		debugfs_create_u32("num_tx_flow_off_ind", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir,
				   (u32 *) &dev->num_tx_flow_off_ind);
		debugfs_create_u32("num_tx_flow_on_ind", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir,
				   (u32 *) &dev->num_tx_flow_on_ind);
		debugfs_create_u32("num_rx_flow_off", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir,
				   (u32 *) &dev->num_rx_flow_off);
		debugfs_create_u32("num_rx_flow_on", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir,
				   (u32 *) &dev->num_rx_flow_on);
		debugfs_create_u32("read_queue_len", S_IRUSR | S_IWUSR,
				   dev->debugfs_device_dir,
				   (u32 *) &dev->read_queue_len);
	}
#endif
	mutex_unlock(&dev->mutex);
	return 0;
err_failed:
	action->name.name[0] = '\0';
	action->major = -1;
	action->minor = -1;
	kfree(dev);
	return result;
}

int chrdev_remove(char *name)
{
	int ret = 0;

	struct caif_char_dev *dev = NULL;

	/* Find device from name. */
	dev = find_device(-1, name, 0);
	if (!dev)
		return -EBADF;

	if (mutex_lock_interruptible(&dev->mutex)) {
		pr_debug("CAIF: %s():"
			 "mutex_lock_interruptible got signalled\n",
			__func__);
		return -ERESTARTSYS;
	}

	if (STATE_IS_OPEN(dev)) {
		pr_debug("CAIF: %s(): Device is opened "
			 "(dev=%p) file_mode = 0x%x\n",
		         __func__, dev, dev->file_mode);
		mutex_unlock(&dev->mutex);
		return -EBUSY;
	}

	/* Remove from list. */
	(void) find_device(-1, name, 1);

	drain_queue(dev);
	ret = misc_deregister(&dev->misc);

	cfglu_free(dev->pktq);

#ifdef CONFIG_DEBUG_FS
	if (dev->debugfs_device_dir != NULL)
		debugfs_remove_recursive(dev->debugfs_device_dir);
#endif

	mutex_unlock(&dev->mutex);
	pr_debug("CAIF: %s(): Removing device %s\n", __func__, dev->name);
	kfree(dev);

	return ret;
}

int chrdev_mgmt(int action, union caif_action *param)
{

	switch (action) {
	case CAIF_ACT_CREATE_DEVICE:
		return chrdev_create(&param->create_channel);
	case CAIF_ACT_DELETE_DEVICE:
		return chrdev_remove(param->delete_channel.name);
	default:
		return -EINVAL;
	}
}

int caif_chrinit_module(void)
{
#ifdef CONFIG_DEBUG_FS
	debugfsdir = debugfs_create_dir("chnl_chr", NULL);
#endif

	spin_lock_init(&list_lock);
	caif_register_chrdev(chrdev_mgmt);
	return 0;
}

void caif_chrexit_module(void)
{
	int result;

	do {
		/* Remove any device (the first in the list). */
		result = chrdev_remove(NULL);
	} while (result == 0);

	caif_unregister_chrdev();

#ifdef CONFIG_DEBUG_FS
	if (debugfsdir != NULL)
		debugfs_remove_recursive(debugfsdir);
#endif

}

module_init(caif_chrinit_module);
module_exit(caif_chrexit_module);
