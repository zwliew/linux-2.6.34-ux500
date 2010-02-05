/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland sjur.brandlenad@stericsson.com
 *		Per Sigmond per.sigmond@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/tcp.h>
#include <linux/uaccess.h>
#include <asm/atomic.h>

#include <linux/caif/caif_socket.h>
#include <linux/caif/caif_config.h>
#include <net/caif/generic/caif_layer.h>
#include <net/caif/caif_dev.h>
#include <net/caif/generic/cfpkt.h>

//official-kernel-patch-cut-here
#if !defined(trace_printk)
#define trace_printk(fmt, ...)
#endif
//official-kernel-patch-resume-here
MODULE_LICENSE("GPL");

#define CHNL_SKT_READ_QUEUE_HIGH 2000
#define CHNL_SKT_READ_QUEUE_LOW 100

int caif_sockbuf_size = 40000;
static struct kmem_cache *caif_sk_cachep;
static atomic_t caif_nr_socks = ATOMIC_INIT(0);

#define CONN_STATE_OPEN_BIT	      1
#define CONN_STATE_PENDING_BIT	      2
#define CONN_STATE_PEND_DESTROY_BIT   3
#define CONN_REMOTE_SHUTDOWN_BIT      4

#define TX_FLOW_ON_BIT		      1
#define RX_FLOW_ON_BIT		      2

#define STATE_IS_OPEN(cf_sk) test_bit(CONN_STATE_OPEN_BIT,\
				    (void *) &(cf_sk)->conn_state)
#define STATE_IS_REMOTE_SHUTDOWN(cf_sk) test_bit(CONN_REMOTE_SHUTDOWN_BIT,\
				    (void *) &(cf_sk)->conn_state)
#define STATE_IS_PENDING(cf_sk) test_bit(CONN_STATE_PENDING_BIT,\
				       (void *) &(cf_sk)->conn_state)
#define STATE_IS_PENDING_DESTROY(cf_sk) test_bit(CONN_STATE_PEND_DESTROY_BIT,\
				       (void *) &(cf_sk)->conn_state)

#define SET_STATE_PENDING_DESTROY(cf_sk) set_bit(CONN_STATE_PEND_DESTROY_BIT,\
				    (void *) &(cf_sk)->conn_state)
#define SET_STATE_OPEN(cf_sk) set_bit(CONN_STATE_OPEN_BIT,\
				    (void *) &(cf_sk)->conn_state)
#define SET_STATE_CLOSED(cf_sk) clear_bit(CONN_STATE_OPEN_BIT,\
					(void *) &(cf_sk)->conn_state)
#define SET_PENDING_ON(cf_sk) set_bit(CONN_STATE_PENDING_BIT,\
				    (void *) &(cf_sk)->conn_state)
#define SET_PENDING_OFF(cf_sk) clear_bit(CONN_STATE_PENDING_BIT,\
				       (void *) &(cf_sk)->conn_state)
#define SET_REMOTE_SHUTDOWN(cf_sk) set_bit(CONN_REMOTE_SHUTDOWN_BIT,\
				    (void *) &(cf_sk)->conn_state)

#define SET_REMOTE_SHUTDOWN_OFF(dev) clear_bit(CONN_REMOTE_SHUTDOWN_BIT,\
				    (void *) &(dev)->conn_state)
#define RX_FLOW_IS_ON(cf_sk) test_bit(RX_FLOW_ON_BIT,\
				    (void *) &(cf_sk)->flow_state)
#define TX_FLOW_IS_ON(cf_sk) test_bit(TX_FLOW_ON_BIT,\
				    (void *) &(cf_sk)->flow_state)

#define SET_RX_FLOW_OFF(cf_sk) clear_bit(RX_FLOW_ON_BIT,\
				       (void *) &(cf_sk)->flow_state)
#define SET_RX_FLOW_ON(cf_sk) set_bit(RX_FLOW_ON_BIT,\
				    (void *) &(cf_sk)->flow_state)
#define SET_TX_FLOW_OFF(cf_sk) clear_bit(TX_FLOW_ON_BIT,\
				       (void *) &(cf_sk)->flow_state)
#define SET_TX_FLOW_ON(cf_sk) set_bit(TX_FLOW_ON_BIT,\
				    (void *) &(cf_sk)->flow_state)

#define SKT_READ_FLAG 0x01
#define SKT_WRITE_FLAG 0x02
static struct dentry *debugfsdir;
#include <linux/debugfs.h>

#ifdef CONFIG_DEBUG_FS
struct debug_fs_counter {
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
};
struct debug_fs_counter cnt;
#define	dbfs_atomic_inc(v) atomic_inc(v)
#else
#define	dbfs_atomic_inc(v)
#endif

/* The AF_CAIF socket */
struct caifsock {
	/* NOTE: sk has to be the first member */
	struct sock sk;
	struct layer layer;
	char name[CAIF_LAYER_NAME_SZ];
	u32 conn_state;
	u32 flow_state;
	struct cfpktq *pktq;
	int file_mode;
	struct caif_channel_config config;
	int read_queue_len;
	spinlock_t read_queue_len_lock;
	struct dentry *debugfs_socket_dir;
};

static void drain_queue(struct caifsock *cf_sk);

/* Packet Receive Callback function called from CAIF Stack */
static int caif_sktrecv_cb(struct layer *layr, struct cfpkt *pkt)
{
	struct caifsock *cf_sk;
	int read_queue_high;
	cf_sk = container_of(layr, struct caifsock, layer);

	if (!STATE_IS_OPEN(cf_sk)) {
		/*FIXME: This should be allowed finally!*/
		pr_debug("CAIF: %s(): called after close request\n", __func__);
		cfpkt_destroy(pkt);
		return 0;
	}
	/* NOTE: This function may be called in Tasklet context! */

	/* The queue has its own lock */
	cfpkt_queue(cf_sk->pktq, pkt, 0);

	spin_lock(&cf_sk->read_queue_len_lock);
	cf_sk->read_queue_len++;

	read_queue_high = (cf_sk->read_queue_len > CHNL_SKT_READ_QUEUE_HIGH);
	spin_unlock(&cf_sk->read_queue_len_lock);

	if (RX_FLOW_IS_ON(cf_sk) && read_queue_high) {
		dbfs_atomic_inc(&cnt.num_rx_flow_off);
		SET_RX_FLOW_OFF(cf_sk);

		/* Send flow off (NOTE: must not sleep) */
		pr_debug("CAIF: %s():"
			" sending flow OFF (queue len = %d)\n",
			__func__,
		     cf_sk->read_queue_len);
		caif_assert(cf_sk->layer.dn);
		caif_assert(cf_sk->layer.dn->ctrlcmd);

		(void) cf_sk->layer.dn->modemcmd(cf_sk->layer.dn,
					       CAIF_MODEMCMD_FLOW_OFF_REQ);
	}

	/* Signal reader that data is available. */

	wake_up_interruptible(cf_sk->sk.sk_sleep);

	return 0;
}

/* Packet Flow Control Callback function called from CAIF */
static void caif_sktflowctrl_cb(struct layer *layr,
				enum caif_ctrlcmd flow,
				int phyid)
{
	struct caifsock *cf_sk;

	/* NOTE: This function may be called in Tasklet context! */
	trace_printk("CAIF: %s(): flowctrl func called: %s.\n",
		      __func__,
		      flow == CAIF_CTRLCMD_FLOW_ON_IND ? "ON" :
		      flow == CAIF_CTRLCMD_FLOW_OFF_IND ? "OFF" :
		      flow == CAIF_CTRLCMD_INIT_RSP ? "INIT_RSP" :
		      flow == CAIF_CTRLCMD_DEINIT_RSP ? "DEINIT_RSP" :
		      flow == CAIF_CTRLCMD_INIT_FAIL_RSP ? "INIT_FAIL_RSP" :
		      flow ==
		      CAIF_CTRLCMD_REMOTE_SHUTDOWN_IND ? "REMOTE_SHUTDOWN" :
		      "UKNOWN CTRL COMMAND");

	cf_sk = container_of(layr, struct caifsock, layer);
	switch (flow) {
	case CAIF_CTRLCMD_FLOW_ON_IND:
		dbfs_atomic_inc(&cnt.num_tx_flow_on_ind);
		/* Signal reader that data is available. */
		SET_TX_FLOW_ON(cf_sk);
		wake_up_interruptible(cf_sk->sk.sk_sleep);
		break;

	case CAIF_CTRLCMD_FLOW_OFF_IND:
		dbfs_atomic_inc(&cnt.num_tx_flow_off_ind);
		SET_TX_FLOW_OFF(cf_sk);
		break;

	case CAIF_CTRLCMD_INIT_RSP:
		dbfs_atomic_inc(&cnt.num_init_resp);
		/* Signal reader that data is available. */
		caif_assert(STATE_IS_OPEN(cf_sk));
		SET_PENDING_OFF(cf_sk);
		SET_TX_FLOW_ON(cf_sk);
		wake_up_interruptible(cf_sk->sk.sk_sleep);
		break;

	case CAIF_CTRLCMD_DEINIT_RSP:
		dbfs_atomic_inc(&cnt.num_deinit_resp);
		caif_assert(!STATE_IS_OPEN(cf_sk));
		SET_PENDING_OFF(cf_sk);
		if (!STATE_IS_PENDING_DESTROY(cf_sk)) {
			if (cf_sk->sk.sk_sleep != NULL)
				wake_up_interruptible(cf_sk->sk.sk_sleep);
		}
		sock_put(&cf_sk->sk);
		break;

	case CAIF_CTRLCMD_INIT_FAIL_RSP:
		dbfs_atomic_inc(&cnt.num_init_fail_resp);
		caif_assert(STATE_IS_OPEN(cf_sk));
		SET_STATE_CLOSED(cf_sk);
		SET_PENDING_OFF(cf_sk);
		SET_TX_FLOW_OFF(cf_sk);
		wake_up_interruptible(cf_sk->sk.sk_sleep);
		break;

	case CAIF_CTRLCMD_REMOTE_SHUTDOWN_IND:
		dbfs_atomic_inc(&cnt.num_remote_shutdown_ind);
		caif_assert(STATE_IS_OPEN(cf_sk));
		caif_assert(STATE_IS_PENDING(cf_sk));
		SET_REMOTE_SHUTDOWN(cf_sk);
		SET_TX_FLOW_OFF(cf_sk);
		drain_queue(cf_sk);
		SET_RX_FLOW_ON(cf_sk);
		cf_sk->file_mode = 0;
		wake_up_interruptible(cf_sk->sk.sk_sleep);
		break;

	default:
		pr_debug("CAIF: %s(): Unexpected flow command %d\n",
			      __func__, flow);
	}
}



static struct sk_buff *caif_alloc_send_skb(struct sock *sk,
				    unsigned long data_len,
				    int *err)
{
	struct sk_buff *skb;
	unsigned int sk_allocation = sk->sk_allocation;

	sk->sk_allocation |= GFP_KERNEL;

	/* Allocate a buffer structure to hold the signal. */
	skb = sock_alloc_send_skb(sk, data_len, 1, err);

	sk->sk_allocation = sk_allocation;

	return skb;
}

static int caif_recvmsg(struct kiocb *iocb, struct socket *sock,
				struct msghdr *m, size_t buf_len, int flags)

{
	struct sock *sk = sock->sk;
	struct caifsock *cf_sk = container_of(sk, struct caifsock, sk);
	struct cfpkt *pkt = NULL;
	size_t len;
	int result;
	struct sk_buff *skb;
	ssize_t ret = -EIO;
	int read_queue_low;

	if (cf_sk == NULL) {
		pr_debug("CAIF: %s(): private_data not set!\n",
			      __func__);
		ret = -EBADFD;
		goto read_error;
	}

	/* Don't do multiple iovec entries yet */
	if (m->msg_iovlen != 1)
		return -EOPNOTSUPP;

	if (unlikely(!buf_len))
		return -EINVAL;

	lock_sock(&(cf_sk->sk));

	caif_assert(cf_sk->pktq);

	if (!STATE_IS_OPEN(cf_sk)) {
		/* Socket is closed or closing. */
		if (!STATE_IS_PENDING(cf_sk)) {
			pr_debug("CAIF: %s(): socket is closed (by remote)\n",
				 __func__);
			ret = -EPIPE;
		} else {
			pr_debug("CAIF: %s(): socket is closing..\n", __func__);
			ret = -EBADF;
		}
		goto read_error;
	}

	/* Socket is open or opening. */
	if (STATE_IS_PENDING(cf_sk)) {
		pr_debug("CAIF: %s(): socket is opening...\n", __func__);

		if (flags & MSG_DONTWAIT) {
			/* We can't block. */
			pr_debug("CAIF: %s():state pending and MSG_DONTWAIT\n",
				 __func__);
			ret = -EAGAIN;
			goto read_error;
		}

		/*
		 * Blocking mode; state is pending and we need to wait
		 * for its conclusion.
		 */
		release_sock(&cf_sk->sk);

		result =
		    wait_event_interruptible(*cf_sk->sk.sk_sleep,
					     !STATE_IS_PENDING(cf_sk));

		lock_sock(&(cf_sk->sk));

		if (result == -ERESTARTSYS) {
			pr_debug("CAIF: %s(): wait_event_interruptible"
				 " woken by a signal (1)", __func__);
			ret = -ERESTARTSYS;
			goto read_error;
		}
	}

	if (STATE_IS_REMOTE_SHUTDOWN(cf_sk)) {
		pr_debug("CAIF: %s(): received remote_shutdown indication\n",
			 __func__);
		ret = -ESHUTDOWN;
		goto read_error;
	}

	/*
	 * Block if we don't have any received buffers.
	 * The queue has its own lock.
	 */
	while ((pkt = cfpkt_qpeek(cf_sk->pktq)) == NULL) {

		if (flags & MSG_DONTWAIT) {
			pr_debug("CAIF: %s(): MSG_DONTWAIT\n", __func__);
			ret = -EAGAIN;
			goto read_error;
		}
		pr_debug("CAIF: %s() wait_event\n", __func__);

		/* Let writers in. */
		release_sock(&cf_sk->sk);

		/* Block reader until data arrives or socket is closed. */
		if (wait_event_interruptible(*cf_sk->sk.sk_sleep,
					cfpkt_qpeek(cf_sk->pktq)
					|| STATE_IS_REMOTE_SHUTDOWN(cf_sk)
					|| !STATE_IS_OPEN(cf_sk)) ==
		    -ERESTARTSYS) {
			pr_debug("CAIF: %s():"
				" wait_event_interruptible woken by "
				"a signal, signal_pending(current) = %d\n",
				__func__,
				signal_pending(current));
			return -ERESTARTSYS;
		}

		pr_debug("CAIF: %s() awake\n", __func__);
		if (STATE_IS_REMOTE_SHUTDOWN(cf_sk)) {
			pr_debug("CAIF: %s(): "
				 "received remote_shutdown indication\n",
				 __func__);
			ret = -ESHUTDOWN;
			goto read_error_no_unlock;
		}

		/* I want to be alone on cf_sk (except status and queue). */
		lock_sock(&(cf_sk->sk));

		if (!STATE_IS_OPEN(cf_sk)) {
			/* Someone closed the link, report error. */
			pr_debug("CAIF: %s(): remote end shutdown!\n",
				      __func__);
			ret = -EPIPE;
			goto read_error;
		}
	}

	/* The queue has its own lock. */
	len = cfpkt_getlen(pkt);

	/* Check max length that can be copied. */
	if (len > buf_len) {
		pr_debug("CAIF: %s(): user buffer too small\n", __func__);
		ret = -EINVAL;
		goto read_error;
	}

	/*
	 * Get packet from queue.
	 * The queue has its own lock.
	 */
	pkt = cfpkt_dequeue(cf_sk->pktq);

	spin_lock(&cf_sk->read_queue_len_lock);
	cf_sk->read_queue_len--;
	read_queue_low = (cf_sk->read_queue_len < CHNL_SKT_READ_QUEUE_LOW);
	spin_unlock(&cf_sk->read_queue_len_lock);

	if (!RX_FLOW_IS_ON(cf_sk) && read_queue_low) {
		dbfs_atomic_inc(&cnt.num_rx_flow_on);
		SET_RX_FLOW_ON(cf_sk);

		/* Send flow on. */
		pr_debug("CAIF: %s(): sending flow ON (queue len = %d)\n",
			 __func__, cf_sk->read_queue_len);
		caif_assert(cf_sk->layer.dn);
		caif_assert(cf_sk->layer.dn->ctrlcmd);
		(void) cf_sk->layer.dn->modemcmd(cf_sk->layer.dn,
					       CAIF_MODEMCMD_FLOW_ON_REQ);

		caif_assert(cf_sk->read_queue_len >= 0);
	}
	skb = cfpkt_tonative(pkt);
	result = skb_copy_datagram_iovec(skb, 0, m->msg_iov, len);
	if (result) {
		pr_debug("CAIF: %s(): cfpkt_raw_extract failed\n", __func__);
		cfpkt_destroy(pkt);
		ret = -EFAULT;
		goto read_error;
	}
	if (unlikely(buf_len < len)) {
		len = buf_len;
		m->msg_flags |= MSG_TRUNC;
	}

	/* Free packet. */
	skb_free_datagram(sk, skb);

	/* Let the others in. */
	release_sock(&cf_sk->sk);
	return len;

read_error:
	release_sock(&cf_sk->sk);
read_error_no_unlock:
	return ret;
}

/* Send a signal as a consequence of sendmsg, sendto or caif_sendmsg. */
static int caif_sendmsg(struct kiocb *kiocb, struct socket *sock,
			struct msghdr *msg, size_t len)
{

	struct sock *sk = sock->sk;
	struct caifsock *cf_sk = container_of(sk, struct caifsock, sk);
	void *payload;
	size_t payload_size = msg->msg_iov->iov_len;
	struct cfpkt *pkt = NULL;
	struct payload_info info;
	unsigned char *txbuf;
	int err = 0;
	ssize_t ret = -EIO;
	int result;
	struct sk_buff *skb;
	caif_assert(msg->msg_iovlen == 1);

	if (cf_sk == NULL) {
		pr_debug("CAIF: %s(): private_data not set!\n",
			      __func__);
		ret = -EBADFD;
		goto write_error_no_unlock;
	}

	if (unlikely(msg->msg_iov->iov_base == NULL)) {
		pr_warning("CAIF: %s(): Buffer is NULL.\n", __func__);
		ret = -EINVAL;
		goto write_error_no_unlock;
	}

	payload = msg->msg_iov->iov_base;
	if (payload_size > CAIF_MAX_PAYLOAD_SIZE) {
		pr_debug("CAIF: %s(): buffer too long\n", __func__);
		ret = -EINVAL;
		goto write_error_no_unlock;
	}
	/* I want to be alone on cf_sk (except status and queue) */
	lock_sock(&(cf_sk->sk));

	caif_assert(cf_sk->pktq);

	if (!STATE_IS_OPEN(cf_sk)) {
		/* Socket is closed or closing */
		if (!STATE_IS_PENDING(cf_sk)) {
			pr_debug("CAIF: %s(): socket is closed (by remote)\n",
				 __func__);
			ret = -EPIPE;
		} else {
			pr_debug("CAIF: %s(): socket is closing...\n",
				 __func__);
			ret = -EBADF;
		}
		goto write_error;
	}

	/* Socket is open or opening */
	if (STATE_IS_PENDING(cf_sk)) {
		pr_debug("CAIF: %s(): socket is opening...\n", __func__);

		if (msg->msg_flags & MSG_DONTWAIT) {
			/* We can't block */
			trace_printk("CAIF: %s():state pending:"
				     "state=MSG_DONTWAIT\n", __func__);
			ret = -EAGAIN;
			goto write_error;
		}

		/* Let readers in */
		release_sock(&cf_sk->sk);

		/*
		 * Blocking mode; state is pending and we need to wait
		 * for its conclusion.
		 */
		result =
		    wait_event_interruptible(*cf_sk->sk.sk_sleep,
					     !STATE_IS_PENDING(cf_sk));

		/* I want to be alone on cf_sk (except status and queue) */
		lock_sock(&(cf_sk->sk));

		if (result == -ERESTARTSYS) {
			pr_debug("CAIF: %s(): wait_event_interruptible"
				 " woken by a signal (1)", __func__);
			ret = -ERESTARTSYS;
			goto write_error;
		}
	}

	if (STATE_IS_REMOTE_SHUTDOWN(cf_sk)) {
		pr_debug("CAIF: %s(): received remote_shutdown indication\n",
			 __func__);
		ret = -ESHUTDOWN;
		goto write_error;
	}

	if (!TX_FLOW_IS_ON(cf_sk)) {

		/* Flow is off. Check non-block flag */
		if (msg->msg_flags & MSG_DONTWAIT) {
			trace_printk("CAIF: %s(): MSG_DONTWAIT and tx flow off",
				 __func__);
			ret = -EAGAIN;
			goto write_error;
		}

		/* release lock before waiting */
		release_sock(&cf_sk->sk);

		/* Wait until flow is on or socket is closed */
		if (wait_event_interruptible(*cf_sk->sk.sk_sleep,
					TX_FLOW_IS_ON(cf_sk)
					|| !STATE_IS_OPEN(cf_sk)
					|| STATE_IS_REMOTE_SHUTDOWN(cf_sk)
					) == -ERESTARTSYS) {
			pr_debug("CAIF: %s():"
				 " wait_event_interruptible woken by a signal",
				 __func__);
			ret = -ERESTARTSYS;
			goto write_error_no_unlock;
		}

		/* I want to be alone on cf_sk (except status and queue) */
		lock_sock(&(cf_sk->sk));

		if (!STATE_IS_OPEN(cf_sk)) {
			/* someone closed the link, report error */
			pr_debug("CAIF: %s(): remote end shutdown!\n",
				      __func__);
			ret = -EPIPE;
			goto write_error;
		}

		if (STATE_IS_REMOTE_SHUTDOWN(cf_sk)) {
			pr_debug("CAIF: %s(): "
				 "received remote_shutdown indication\n",
				 __func__);
			ret = -ESHUTDOWN;
			goto write_error;
		}
	}

	/* Create packet, buf=NULL means no copying */
	skb = caif_alloc_send_skb(sk,
				payload_size + CAIF_NEEDED_HEADROOM +
				CAIF_NEEDED_TAILROOM,
				&err);

	if (skb == NULL) {
		pr_debug("CAIF: %s(): caif_alloc_send_skb returned NULL\n",
			  __func__);
		ret = -ENOMEM;
		goto write_error;
	}

	skb_reserve(skb, CAIF_NEEDED_HEADROOM);
	pkt = cfpkt_fromnative(CAIF_DIR_OUT, skb);
	caif_assert((void *)pkt == (void *)skb);

	if (cfpkt_raw_append(pkt, (void **) &txbuf, payload_size) < 0) {
		pr_debug("CAIF: %s(): cfpkt_raw_append failed\n", __func__);
		cfpkt_destroy(pkt);
		ret = -EINVAL;
		goto write_error;
	}

	/* Copy data into buffer. */
	if (copy_from_user(txbuf, payload, payload_size)) {
		pr_debug("CAIF: %s(): copy_from_user returned non zero.\n",
			 __func__);
		cfpkt_destroy(pkt);
		ret = -EINVAL;
		goto write_error;
	}
	memset(&info, 0, sizeof(info));

	/* Send the packet down the stack. */
	caif_assert(cf_sk->layer.dn);
	caif_assert(cf_sk->layer.dn->transmit);

	do {
		ret = cf_sk->layer.dn->transmit(cf_sk->layer.dn, pkt);

		if (likely((ret >= 0) || (ret != -EAGAIN)))
			break;

		/* EAGAIN - retry */
		if (msg->msg_flags & MSG_DONTWAIT) {
			pr_debug("CAIF: %s(): NONBLOCK and transmit failed,"
				 " error = %d\n", __func__, ret);
			ret = -EAGAIN;
			goto write_error;
		}

		/* Let readers in */
		release_sock(&cf_sk->sk);

		/* Wait until flow is on or socket is closed */
		if (wait_event_interruptible(*cf_sk->sk.sk_sleep,
					TX_FLOW_IS_ON(cf_sk)
					|| !STATE_IS_OPEN(cf_sk)
					|| STATE_IS_REMOTE_SHUTDOWN(cf_sk)
					) == -ERESTARTSYS) {
			pr_debug("CAIF: %s(): wait_event_interruptible"
				 " woken by a signal", __func__);
			ret = -ERESTARTSYS;
			goto write_error_no_unlock;
		}

		/* I want to be alone on cf_sk (except status and queue) */
		lock_sock(&(cf_sk->sk));

	} while (ret == -EAGAIN);

	if (ret < 0) {
		cfpkt_destroy(pkt);
		pr_debug("CAIF: %s(): transmit failed, error = %d\n",
			  __func__, ret);

		goto write_error;
	}

	release_sock(&cf_sk->sk);
	return payload_size;

write_error:
	release_sock(&cf_sk->sk);
write_error_no_unlock:
	return ret;
}

static unsigned int caif_poll(struct file *file, struct socket *sock,
						poll_table *wait)
{
	struct sock *sk = sock->sk;
	struct caifsock *cf_sk = container_of(sk, struct caifsock, sk);
	u32 mask = 0;

	poll_wait(file, sk->sk_sleep, wait);
	lock_sock(&(cf_sk->sk));
	if (cfpkt_qpeek(cf_sk->pktq) != NULL)
		mask |= (POLLIN | POLLRDNORM);
	if (!STATE_IS_OPEN(cf_sk))
		mask |= POLLHUP;
	else if (TX_FLOW_IS_ON(cf_sk))
		mask |= (POLLOUT | POLLWRNORM);
	release_sock(&cf_sk->sk);
	trace_printk("CAIF: %s(): poll mask=0x%04x...\n",
		      __func__, mask);
	return mask;
}

static void drain_queue(struct caifsock *cf_sk)
{
	struct cfpkt *pkt = NULL;

	/* Empty the queue */
	do {
		/* The queue has its own lock */
		pkt = cfpkt_dequeue(cf_sk->pktq);
		if (!pkt)
			break;
		pr_debug("CAIF: %s(): freeing packet from read queue\n",
			 __func__);
		cfpkt_destroy(pkt);

	} while (1);

	spin_lock(&cf_sk->read_queue_len_lock);
	cf_sk->read_queue_len = 0;
	spin_unlock(&cf_sk->read_queue_len_lock);
}


static int setsockopt(struct socket *sock,
			int lvl, int opt, char __user *ov, unsigned int ol)
{
	struct sock *sk = sock->sk;
	struct caifsock *cf_sk = container_of(sk, struct caifsock, sk);
	struct caif_channel_opt confopt;
	int res;

	if (lvl != SOL_CAIF) {
		pr_debug("CAIF: %s(): setsockopt bad level\n", __func__);
		return -ENOPROTOOPT;
	}

	switch (opt) {
	case CAIFSO_CHANNEL_CONFIG:
		if (ol < sizeof(struct caif_channel_opt)) {
			pr_debug("CAIF: %s(): setsockopt"
				 " CAIFSO_CHANNEL_CONFIG bad size\n", __func__);
			return -EINVAL;
		}
		res = copy_from_user(&confopt, ov, sizeof(confopt));
		if (res)
			return res;
		lock_sock(&(cf_sk->sk));
		cf_sk->config.priority = confopt.priority;
		cf_sk->config.phy_pref = confopt.link_selector;
		strncpy(cf_sk->config.phy_name, confopt.link_name,
			sizeof(cf_sk->config.phy_name));
		pr_debug("CAIF: %s(): Setting sockopt pri=%d pref=%d name=%s\n",
			      __func__,
			      cf_sk->config.priority,
			      cf_sk->config.phy_pref,
			      cf_sk->config.phy_name);
		release_sock(&cf_sk->sk);
		return 0;
/* TODO: Implement the remaining options:
 *	case CAIF_REQ_PARAM_OPT:
 *	case CAIF_RSP_PARAM_OPT:
 *	case CAIF_UTIL_FLOW_OPT:
 *	case CAIF_CONN_INFO_OPT:
 *	case CAIF_CONN_ID_OPT:
 */
	default:
		pr_debug("CAIF: %s(): unhandled option %d\n", __func__, opt);
		return -EINVAL;
	}


}

static int getsockopt(struct socket *sock,
			int lvl, int opt, char __user *ov, int __user *ol)
{
	return -EINVAL;
}

static int caif_channel_config(struct caifsock *cf_sk,
				struct sockaddr *sock_addr, int len,
				struct caif_channel_config *config)
{
	struct sockaddr_caif *addr = (struct sockaddr_caif *)sock_addr;


		if (len != sizeof(struct sockaddr_caif)) {
			pr_debug("CAIF: %s(): Bad address len (%d,%d)\n",
				 __func__, len, sizeof(struct sockaddr_caif));
			return -EINVAL;
	}
	if (sock_addr->sa_family != AF_CAIF) {
		pr_debug("CAIF: %s(): Bad address family (%d)\n",
			 __func__, sock_addr->sa_family);
		return -EAFNOSUPPORT;
	}

	switch (cf_sk->sk.sk_protocol) {
	case CAIFPROTO_AT:
		config->type = CAIF_CHTY_AT;
		break;
	case CAIFPROTO_DATAGRAM:
		config->type = CAIF_CHTY_DATAGRAM;
		break;
	case CAIFPROTO_DATAGRAM_LOOP:
		config->type = CAIF_CHTY_DATAGRAM_LOOP;
		config->u.dgm.connection_id = addr->u.dgm.connection_id;
		break;
	case CAIFPROTO_UTIL:
		config->type = CAIF_CHTY_UTILITY;
		strncpy(config->u.utility.name, addr->u.util.service,
				sizeof(config->u.utility.name));
		/* forcing the end of string to be null-terminated  */
		config->u.utility.name[sizeof(config->u.utility.name)-1] = '\0';
		break;
	case CAIFPROTO_RFM:
		config->type = CAIF_CHTY_RFM;
		config->u.rfm.connection_id = addr->u.rfm.connection_id;
		strncpy(config->u.rfm.volume, addr->u.rfm.volume,
			sizeof(config->u.rfm.volume));
		/* forcing the end of string to be null-terminated  */
		config->u.rfm.volume[sizeof(config->u.rfm.volume)-1] = '\0';
		break;
	default:
		pr_debug("CAIF: %s(): Bad caif protocol type (%d)\n",
			 __func__, cf_sk->sk.sk_protocol);
		return -EINVAL;
	}
	trace_printk("CAIF: %s(): Setting connect param PROTO=%s\n",
		 __func__,
		(cf_sk->sk.sk_protocol == CAIFPROTO_AT) ?
		"CAIFPROTO_AT" :
		(cf_sk->sk.sk_protocol == CAIFPROTO_DATAGRAM) ?
		"CAIFPROTO_DATAGRAM" :
		(cf_sk->sk.sk_protocol == CAIFPROTO_DATAGRAM_LOOP) ?
		"CAIFPROTO_DATAGRAM_LOOP" :
		(cf_sk->sk.sk_protocol == CAIFPROTO_UTIL) ?
		"CAIFPROTO_UTIL" :
		(cf_sk->sk.sk_protocol == CAIFPROTO_RFM) ?
		"CAIFPROTO_RFM" : "ERROR");
	return 0;
}


int caif_connect(struct socket *sock, struct sockaddr *uservaddr,
	       int sockaddr_len, int flags)
{
	struct caifsock *cf_sk = NULL;
	int result = -1;
	int mode = 0;
	int ret = -EIO;
	struct sock *sk = sock->sk;

	BUG_ON(sk == NULL);

	cf_sk = container_of(sk, struct caifsock, sk);

	trace_printk("CAIF: %s(): cf_sk=%p OPEN=%d, TX_FLOW=%d, RX_FLOW=%d\n",
		 __func__, cf_sk,
		STATE_IS_OPEN(cf_sk),
		TX_FLOW_IS_ON(cf_sk), RX_FLOW_IS_ON(cf_sk));

	sk->sk_state	= TCP_CLOSE;
	sock->state	= SS_UNCONNECTED;

	if (sock->type == SOCK_SEQPACKET) {
		sock->state	= SS_CONNECTED;
		sk->sk_state	= TCP_ESTABLISHED;
	} else
		goto out;

	/* I want to be alone on cf_sk (except status and queue) */
	lock_sock(&(cf_sk->sk));

	ret = caif_channel_config(cf_sk, uservaddr, sockaddr_len,
				&cf_sk->config);
	if (ret) {
		pr_debug("CAIF: %s(): Cannot set socket address\n",
		     __func__);
		goto open_error;
	}

	dbfs_atomic_inc(&cnt.num_open);
	mode = SKT_READ_FLAG | SKT_WRITE_FLAG;

	/* If socket is not open, make sure socket is in fully closed state */
	if (!STATE_IS_OPEN(cf_sk)) {
		/* Has link close response been received (if we ever sent it)?*/
		if (STATE_IS_PENDING(cf_sk)) {
			/*
			 * Still waiting for close response from remote.
			 * If opened non-blocking, report "would block"
			 */
			if (flags & MSG_DONTWAIT) {
				pr_debug("CAIF: %s(): MSG_DONTWAIT"
					" && close pending\n", __func__);
				ret = -EAGAIN;
				goto open_error;
			}

			pr_debug("CAIF: %s(): Wait for close response"
				 " from remote...\n", __func__);

			release_sock(&cf_sk->sk);

			/*
			 * Blocking mode; close is pending and we need to wait
			 * for its conclusion.
			 */
			result =
			    wait_event_interruptible(*cf_sk->sk.sk_sleep,
						     !STATE_IS_PENDING(cf_sk));

			lock_sock(&(cf_sk->sk));
			if (result == -ERESTARTSYS) {
				pr_debug("CAIF: %s(): wait_event_interruptible"
					 "woken by a signal (1)", __func__);
				ret = -ERESTARTSYS;
				goto open_error;
			}
		}
	}

	/* socket is now either closed, pending open or open */
	if (STATE_IS_OPEN(cf_sk) && !STATE_IS_PENDING(cf_sk)) {
		/* Open */
		pr_debug("CAIF: %s(): Socket is already opened (cf_sk=%p)"
			" check access f_flags = 0x%x file_mode = 0x%x\n",
			 __func__, cf_sk, mode, cf_sk->file_mode);

		if (mode & cf_sk->file_mode) {
			pr_debug("CAIF: %s(): Access mode already in use"
				"0x%x\n",
				__func__, mode);
			ret = -EBUSY;
			goto open_error;
		}
	} else {
		/* We are closed or pending open.
		 * If closed:	    send link setup
		 * If pending open: link setup already sent (we could have been
		 *		    interrupted by a signal last time)
		 */
		if (!STATE_IS_OPEN(cf_sk)) {
			/* First opening of file; connect lower layers: */
					/* Drain queue (very unlikely) */
			drain_queue(cf_sk);

			cf_sk->layer.receive = caif_sktrecv_cb;

			SET_STATE_OPEN(cf_sk);
			SET_PENDING_ON(cf_sk);

			/* Register this channel. */
			result =
			    caifdev_adapt_register(&cf_sk->config,
						&cf_sk->layer);
			if (result < 0) {
				pr_debug("CAIF: %s(): can't register channel\n",
					__func__);
				ret = -EIO;
				SET_STATE_CLOSED(cf_sk);
				SET_PENDING_OFF(cf_sk);
				goto open_error;
			}
			dbfs_atomic_inc(&cnt.num_init);
		}

		/* If opened non-blocking, report "success".
		 */
		if (flags & MSG_DONTWAIT) {
			pr_debug("CAIF: %s(): MSG_DONTWAIT success\n",
				 __func__);
			ret = 0;
			goto open_success;
		}

		trace_printk("CAIF: %s(): Wait for connect response\n",
			     __func__);

		/* release lock before waiting */
		release_sock(&cf_sk->sk);

		result =
		    wait_event_interruptible(*cf_sk->sk.sk_sleep,
					     !STATE_IS_PENDING(cf_sk));

		lock_sock(&(cf_sk->sk));

		if (result == -ERESTARTSYS) {
			pr_debug("CAIF: %s(): wait_event_interruptible"
				 "woken by a signal (2)", __func__);
			ret = -ERESTARTSYS;
			goto open_error;
		}

		if (!STATE_IS_OPEN(cf_sk)) {
			/* Lower layers said "no" */
			pr_debug("CAIF: %s(): Closed received\n", __func__);
			ret = -EPIPE;
			goto open_error;
		}

		trace_printk("CAIF: %s(): Connect received\n", __func__);
	}
open_success:
	/* Open is ok */
	cf_sk->file_mode |= mode;

	trace_printk("CAIF: %s(): Connected - file mode = %x\n",
		  __func__, cf_sk->file_mode);

	release_sock(&cf_sk->sk);
	return 0;
open_error:
	release_sock(&cf_sk->sk);
out:
	return ret;
}

static int caif_shutdown(struct socket *sock, int how)
{
	struct caifsock *cf_sk = NULL;
	int result;
	int tx_flow_state_was_on;
	struct sock *sk = sock->sk;
	int res = 0;

	if (how != SHUT_RDWR)
		return -EOPNOTSUPP; /* FIXME: ENOTSUP in userland for POSIX */

	cf_sk = container_of(sk, struct caifsock, sk);
	if (cf_sk == NULL) {
		pr_debug("CAIF: %s(): COULD NOT FIND SOCKET\n", __func__);
		return -EBADF;
	}

	/* I want to be alone on cf_sk (except status queue) */
	lock_sock(&(cf_sk->sk));
	dbfs_atomic_inc(&cnt.num_close);

	/* Is the socket open? */
	if (!STATE_IS_OPEN(cf_sk)) {
		pr_debug("CAIF: %s(): socket not open (cf_sk=%p) \n",
			      __func__, cf_sk);
		release_sock(&cf_sk->sk);
		return 0;
	}

	/* Is the socket waiting for link setup response? */
	if (STATE_IS_PENDING(cf_sk)) {
		pr_debug("CAIF: %s(): Socket is open pending (cf_sk=%p) \n",
				__func__, cf_sk);
		release_sock(&cf_sk->sk);
		/* What to return here? Seems that EBADF is the closest :-| */
		return -EBADF;
	}
	/* IS_CLOSED have double meaning:
	 * 1) Spontanous Remote Shutdown Request.
	 * 2) Ack on a channel teardown(disconnect)
	 * Must clear bit in case we previously received
	 * remote shudown request.
	 */

	SET_STATE_CLOSED(cf_sk);
	SET_PENDING_ON(cf_sk);
	sock->state = SS_DISCONNECTING; /* FIXME: Update the sock->states */
	tx_flow_state_was_on = TX_FLOW_IS_ON(cf_sk);
	SET_TX_FLOW_OFF(cf_sk);
	sock_hold(&cf_sk->sk);
	result = caifdev_adapt_unregister(&cf_sk->layer);

	if (result < 0) {
		pr_debug("CAIF: %s(): caifdev_adapt_unregister() failed\n",
			 __func__);
		SET_STATE_CLOSED(cf_sk);
		SET_PENDING_OFF(cf_sk);
		SET_TX_FLOW_OFF(cf_sk);
		release_sock(&cf_sk->sk);
		return -EIO;
	}
	if (STATE_IS_REMOTE_SHUTDOWN(cf_sk)) {
		SET_PENDING_OFF(cf_sk);
		SET_REMOTE_SHUTDOWN_OFF(cf_sk);
	}

	dbfs_atomic_inc(&cnt.num_deinit);

	/*
	 * We don't wait for close response here. Close pending state will be
	 * cleared by flow control callback when response arrives.
	 */
	drain_queue(cf_sk);
	SET_RX_FLOW_ON(cf_sk);
	cf_sk->file_mode = 0;

	release_sock(&cf_sk->sk);
	return res;
}
static ssize_t caif_sock_no_sendpage(struct socket *sock,
				     struct page *page,
				     int offset, size_t size, int flags)
{
	return -EOPNOTSUPP;
}

/* This function is called as part of close. */
static int caif_release(struct socket *sock)
{
	struct sock *sk = sock->sk;
	struct caifsock *cf_sk = NULL;

	caif_assert(sk != NULL);
	cf_sk = container_of(sk, struct caifsock, sk);

	caif_shutdown(sock, SHUT_RDWR);
	lock_sock(&(cf_sk->sk));

	sock->sk = NULL;

	/* Detach the socket from its process context by making it orphan. */
	sock_orphan(sk);

	/*
	 * Setting SHUTDOWN_MASK means that both send and receive are shutdown
	 * for the socket.
	 */
	sk->sk_shutdown = SHUTDOWN_MASK;

	/*
	 * Set the socket state to closed, the TCP_CLOSE macro is used when
	 * closing any socket.
	 */
	sk->sk_state = TCP_CLOSE;

	/* Flush out this sockets receive queue. */
	drain_queue(cf_sk);

	/* Finally release the socket. */
	STATE_IS_PENDING_DESTROY(cf_sk);
	release_sock(&cf_sk->sk);

	/*
	 * The rest of the cleanup will be handled from the
	 * caif_sock_destructor
	 */
	sock_put(sk);
	return 0;
}

static int caif_sock_ioctl(struct socket *sock,
		       unsigned int cmd,
		       unsigned long arg)

{
	return caif_ioctl(cmd, arg, true);
}

static struct proto_ops caif_ops = {
	.family = PF_CAIF,
	.owner = THIS_MODULE,
	.release = caif_release,
	.bind = sock_no_bind,
	.connect = caif_connect,
	.socketpair = sock_no_socketpair,
	.accept = sock_no_accept,
	.getname = sock_no_getname,
	.poll = caif_poll,
	.ioctl = caif_sock_ioctl,
	.listen = sock_no_listen,
	.shutdown = caif_shutdown,
	.setsockopt = setsockopt,
	.getsockopt = getsockopt,
	.sendmsg = caif_sendmsg,
	.recvmsg = caif_recvmsg,
	.mmap = sock_no_mmap,
	.sendpage = caif_sock_no_sendpage,
};

/* This function is called when a socket is finally destroyed. */
static void caif_sock_destructor(struct sock *sk)
{
	struct caifsock *cf_sk = NULL;
	cf_sk = container_of(sk, struct caifsock, sk);

	/* Error checks. */
	caif_assert(!atomic_read(&sk->sk_wmem_alloc));
	caif_assert(sk_unhashed(sk));
	caif_assert(!sk->sk_socket);

	if (!sock_flag(sk, SOCK_DEAD)) {
		pr_debug("CAIF: %s(): 0x%p", __func__, sk);
		return;
	}

	lock_sock(&(cf_sk->sk));

	if (STATE_IS_OPEN(cf_sk)) {
		pr_debug("CAIF: %s(): socket is opened (cf_sk=%p)"
			 " file_mode = 0x%x\n", __func__,
			 cf_sk, cf_sk->file_mode);
		release_sock(&cf_sk->sk);
		return;
	}
	drain_queue(cf_sk);
	cfglu_free(cf_sk->pktq);

	if (cf_sk->debugfs_socket_dir != NULL)
		debugfs_remove_recursive(cf_sk->debugfs_socket_dir);

	release_sock(&cf_sk->sk);
	trace_printk("CAIF: %s(): caif_sock_destructor: Removing socket %s\n",
		__func__, cf_sk->name);
	atomic_dec(&caif_nr_socks);
}

static int caif_create(struct net *net, struct socket *sock, int protocol,
		       int kern)
{
	struct sock *sk = NULL;
	struct caifsock *cf_sk = NULL;
	int result = 0;

	static struct proto prot = {.name = "PF_CAIF",
		.owner = THIS_MODULE,
		.obj_size = sizeof(struct caifsock),
	};
	prot.slab = caif_sk_cachep;

	if (net != &init_net)
		return -EAFNOSUPPORT;

	if (protocol < 0 || protocol >= CAIFPROTO_MAX)
		return -EPROTONOSUPPORT;

	/*
	 * Set the socket state to unconnected.	 The socket state is really
	 * not used at all in the net/core or socket.c but the
	 * initialization makes sure that sock->state is not uninitialized.
	 */
	sock->state = SS_UNCONNECTED;

	sk = sk_alloc(net, PF_CAIF, GFP_KERNEL, &prot);
	if (!sk)
		return -ENOMEM;

	cf_sk = container_of(sk, struct caifsock, sk);

	/* Store the protocol */
	sk->sk_protocol = (unsigned char) protocol;

	spin_lock_init(&cf_sk->read_queue_len_lock);

	/* Fill in some information concerning the misc socket. */
	snprintf(cf_sk->name, sizeof(cf_sk->name), "cf_sk%d",
		atomic_read(&caif_nr_socks));
	snprintf(cf_sk->config.name, sizeof(cf_sk->config.name), "caifconf%d",
		atomic_read(&caif_nr_socks));

	/*
	 * The sock->type specifies the socket type to use. The CAIF socket is
	 * a packet stream in the sence that it is packet based.
	 * CAIF trusts the reliability of the link, no resending is implemented.
	 */
	switch (sock->type) {
	case SOCK_SEQPACKET:
		sock->ops = &caif_ops;
		break;
	default:
		sk_free(sk);
		return -ESOCKTNOSUPPORT;
	}

	/*
	 * Lock in order to try to stop someone from opening the socket
	 * too early.
	 */
	lock_sock(&(cf_sk->sk));

	/* Initialize the nozero default sock structure data. */
	sock_init_data(sock, sk);

	sk->sk_destruct = caif_sock_destructor;
	sk->sk_sndbuf = caif_sockbuf_size;
	sk->sk_rcvbuf = caif_sockbuf_size;

	cf_sk->pktq = cfpktq_create();

	if (!cf_sk->pktq) {
		pr_err("CAIF: %s(): queue create failed.\n", __func__);
		result = -ENOMEM;
		release_sock(&cf_sk->sk);
		goto err_failed;
	}

	cf_sk->layer.ctrlcmd = caif_sktflowctrl_cb;
	SET_STATE_CLOSED(cf_sk);
	SET_PENDING_OFF(cf_sk);
	SET_TX_FLOW_OFF(cf_sk);
	SET_RX_FLOW_ON(cf_sk);

	/* Increase the number of sockets created. */
	atomic_inc(&caif_nr_socks);
	if (!IS_ERR(debugfsdir)) {
		cf_sk->debugfs_socket_dir =
			debugfs_create_dir(cf_sk->name, debugfsdir);
		debugfs_create_u32("conn_state", S_IRUSR | S_IWUSR,
				cf_sk->debugfs_socket_dir, &cf_sk->conn_state);
		debugfs_create_u32("flow_state", S_IRUSR | S_IWUSR,
				cf_sk->debugfs_socket_dir, &cf_sk->flow_state);
		debugfs_create_u32("read_queue_len", S_IRUSR | S_IWUSR,
				cf_sk->debugfs_socket_dir,
				(u32 *) &cf_sk->read_queue_len);
		debugfs_create_u32("identity", S_IRUSR | S_IWUSR,
				cf_sk->debugfs_socket_dir,
				(u32 *) &cf_sk->layer.id);
	}
	release_sock(&cf_sk->sk);
	return 0;
err_failed:
	sk_free(sk);
	return result;
}


static struct net_proto_family caif_family_ops = {
	.family = PF_CAIF,
	.create = caif_create,
	.owner = THIS_MODULE,
};

int af_caif_init(void)
{
	int err;
	err = sock_register(&caif_family_ops);

	if (!err)
		return err;

	return 0;
}

static int __init caif_sktinit_module(void)
{
	int stat;
#ifdef CONFIG_DEBUG_FS
	debugfsdir = debugfs_create_dir("chnl_skt", NULL);
	if (!IS_ERR(debugfsdir)) {
		atomic_inc(&caif_nr_socks);
		debugfs_create_u32("num_sockets", S_IRUSR | S_IWUSR,
				debugfsdir,
				(u32 *) &caif_nr_socks);
		debugfs_create_u32("num_open", S_IRUSR | S_IWUSR,
				debugfsdir,
				(u32 *) &cnt.num_open);
		debugfs_create_u32("num_close", S_IRUSR | S_IWUSR,
				debugfsdir,
				(u32 *) &cnt.num_close);
		debugfs_create_u32("num_init", S_IRUSR | S_IWUSR,
				debugfsdir,
				(u32 *) &cnt.num_init);
		debugfs_create_u32("num_init_resp", S_IRUSR | S_IWUSR,
				debugfsdir,
				(u32 *) &cnt.num_init_resp);
		debugfs_create_u32("num_init_fail_resp", S_IRUSR | S_IWUSR,
				debugfsdir,
				(u32 *) &cnt.num_init_fail_resp);
		debugfs_create_u32("num_deinit", S_IRUSR | S_IWUSR,
				debugfsdir,
				(u32 *) &cnt.num_deinit);
		debugfs_create_u32("num_deinit_resp", S_IRUSR | S_IWUSR,
				debugfsdir,
				(u32 *) &cnt.num_deinit_resp);
		debugfs_create_u32("num_remote_shutdown_ind",
				S_IRUSR | S_IWUSR, debugfsdir,
				(u32 *) &cnt.num_remote_shutdown_ind);
		debugfs_create_u32("num_tx_flow_off_ind", S_IRUSR | S_IWUSR,
				debugfsdir,
				(u32 *) &cnt.num_tx_flow_off_ind);
		debugfs_create_u32("num_tx_flow_on_ind", S_IRUSR | S_IWUSR,
				debugfsdir,
				(u32 *) &cnt.num_tx_flow_on_ind);
		debugfs_create_u32("num_rx_flow_off", S_IRUSR | S_IWUSR,
				debugfsdir,
				(u32 *) &cnt.num_rx_flow_off);
		debugfs_create_u32("num_rx_flow_on", S_IRUSR | S_IWUSR,
				debugfsdir,
				(u32 *) &cnt.num_rx_flow_on);
	}
#endif
	stat = af_caif_init();
	if (stat) {
		pr_err("CAIF: %s(): Failed to initialize CAIF socket layer.",
		       __func__);
		return stat;
	}
	return 0;
}

static void __exit caif_sktexit_module(void)
{
	sock_unregister(PF_CAIF);
	if (debugfsdir != NULL)
		debugfs_remove_recursive(debugfsdir);
}

module_init(caif_sktinit_module);
module_exit(caif_sktexit_module);

