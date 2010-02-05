/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland / sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>
#include <linux/tty.h>
#include <linux/file.h>
#include <linux/if_arp.h>
#include <net/caif/caif_device.h>
#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cfcnfg.h>
#include <linux/err.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sjur Brendeland<sjur.brandeland@stericsson.com>");
MODULE_DESCRIPTION("CAIF serial device TTY line discipline");
MODULE_LICENSE("GPL");
MODULE_ALIAS_LDISC(N_CAIF);

//official-kernel-patch-cut-here
/* FIXME:Remove this when CAIF is included in tty.h */
#ifndef N_CAIF
#define N_CAIF N_MOUSE
#endif
//official-kernel-patch-resume-here
#define CAIF_SENDING	1
#define CAIF_UART_TX_COMPLETED	2
#define CAIF_FLOW_OFF_SENT	4
#define MAX_WRITE_CHUNK	     4096
#define ON 1
#define OFF 0
#define CAIF_MAX_MTU 4096

struct net_device *device;
char *ser_ttyname = "/dev/ttyS0";
module_param(ser_ttyname, charp, S_IRUGO);
MODULE_PARM_DESC(ser_ttyname, "TTY to open.");

int ser_loop;
module_param(ser_loop, bool, S_IRUGO);
MODULE_PARM_DESC(ser_loop, "Run in simulated loopback mode.");

int ser_use_stx;
module_param(ser_use_stx, bool, S_IRUGO);
MODULE_PARM_DESC(ser_use_stx, "STX enabled or not.");

int ser_write_chunk = MAX_WRITE_CHUNK;
module_param(ser_write_chunk, int, S_IRUGO);

MODULE_PARM_DESC(ser_write_chunk, "Maximum size of data written to UART.");


static int caif_net_open(struct net_device *dev);
static int caif_net_close(struct net_device *dev);

struct ser_device {
	struct caif_dev_common common;
	struct net_device *dev;
	struct sk_buff_head head;
	int xoff;
	struct tty_struct *tty;
	bool tx_started;
	unsigned long state;
	struct file *file;
	char *tty_name;
};

static int ser_phy_tx(struct ser_device *ser, struct sk_buff *skb);
static void caifdev_setup(struct net_device *dev);
static void ser_tx_wakeup(struct tty_struct *tty);

static void ser_receive(struct tty_struct *tty, const u8 *data,
			char *flags, int count)
{
	struct sk_buff *skb = NULL;
	struct ser_device *ser;
	int ret;
	u8 *p;
	ser = tty->disc_data;

	/*
	 * Workaround for garbage at start of transmission,
	 * only enable if STX handling is not enables
	 */
	if (!ser->common.use_stx && !ser->tx_started) {
		dev_info(&ser->dev->dev,
			"Bytes received before initial transmission -"
			"bytes discarded.\n");
		return;
	}

	BUG_ON(ser->dev == NULL);

	/* Get a suitable caif packet and copy in data. */
	skb = netdev_alloc_skb(ser->dev, count+1);
	BUG_ON(skb == NULL);
	p = skb_put(skb, count);
	memcpy(p, data, count);

	skb->protocol = htons(ETH_P_CAIF);
	skb_reset_mac_header(skb);
	skb->dev = ser->dev;

	/* Push received packet up the stack. */
	ret = netif_rx(skb);
	if (!ret) {
		ser->dev->stats.rx_packets++;
		ser->dev->stats.rx_bytes += count;
	} else
		++ser->dev->stats.rx_dropped;
}

static int handle_tx(struct ser_device *ser)
{
	struct tty_struct *tty;
	struct sk_buff *skb;
	char *buf;
	int tty_wr, len, room, pktlen;
	tty = ser->tty;

	/*
	 * NOTE: This workaround is not really needed when STX is enabled.
	 * Remove?
	 */
	if (ser->tx_started == false)
		ser->tx_started = true;

	if (test_and_set_bit(CAIF_SENDING, &ser->state)) {
		set_bit(CAIF_UART_TX_COMPLETED, &ser->state);
		return 0;
	}

	do {
		skb = skb_peek(&ser->head);
		if (skb != NULL && skb->len == 0) {
			struct sk_buff *tmp;
			tmp = skb_dequeue(&ser->head);
			BUG_ON(tmp != skb);
			kfree_skb(skb);
			skb = skb_peek(&ser->head);
		}

		if (skb == NULL) {
			if (test_and_clear_bit(
				    CAIF_FLOW_OFF_SENT,
				    &ser->state)) {
				if (ser->common.flowctrl != NULL)
					ser->common.flowctrl(ser->dev, ON);
			}
			break;
		}


		buf = skb->data;
		pktlen = len = skb->len;

		clear_bit(CAIF_UART_TX_COMPLETED, &ser->state);
		room = tty_write_room(tty);
		if (room > ser_write_chunk)
			room = ser_write_chunk;

		if (len > room)
			len = room;

		if (!ser_loop) {
			tty_wr = tty->ops->write(tty, buf, len);
		} else {
			tty_wr = len;
			ser_receive(tty, buf, 0, len);
		}
		ser->dev->stats.tx_packets++;
		ser->dev->stats.tx_bytes += tty_wr;
		if (tty_wr > 0)
			skb_pull(skb, tty_wr);

		if (ser_loop)
			ser_tx_wakeup(tty);

	} while (test_bit(CAIF_UART_TX_COMPLETED, &(ser->state)));

	clear_bit(CAIF_SENDING, &ser->state);
	return 0;
}

static int ser_phy_tx(struct ser_device *ser, struct sk_buff *skb)
{
	if (skb_peek(&ser->head) !=  NULL) {
		if (!test_and_set_bit(CAIF_FLOW_OFF_SENT, &ser->state)
		    && ser->common.flowctrl != NULL)
			ser->common.flowctrl(ser->dev, OFF);
	}
	skb_queue_tail(&ser->head, skb);
	if (!test_bit(CAIF_SENDING, &ser->state))
		handle_tx(ser);
	return 0;
}

static int caif_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct ser_device *ser;
	if (!dev)
		return -EINVAL;
	ser = netdev_priv(dev);
	return ser_phy_tx(ser, skb);
}


static void ser_tx_wakeup(struct tty_struct *tty)
{
	struct ser_device *ser;
	ser = tty->disc_data;
	if (ser == NULL)
		return;
	set_bit(CAIF_UART_TX_COMPLETED, &ser->state);
	if (ser->tty != tty)
		return;
	handle_tx(ser);
}

static void remove_caif_phy_dev(struct net_device *dev)
{
	/* Remove may be called inside or outside of rtnl_lock */
	int islocked = rtnl_is_locked();
	if (!islocked)
		rtnl_lock();
	dev_close(dev);
	/* device is freed automagically by net-sysfs */
	unregister_netdevice(dev);
	if (!islocked)
		rtnl_unlock();
}

static int ser_open(struct tty_struct *tty)
{
	struct ser_device *ser;
	/* Use global device to map tty with ser */
	ser = netdev_priv(device);
	if (ser->file == NULL ||
		tty != (struct tty_struct *)ser->file->private_data) {
		dev_err(&ser->dev->dev,
			  "Cannot install ldisc %s from userspace!",
			  tty->name);
		return -EINVAL;
	}
	tty->receive_room = 4096;
	ser->tty = tty;
	tty->disc_data = ser;
	set_bit(TTY_DO_WRITE_WAKEUP, &tty->flags);
	return 0;
}

static void ser_close(struct tty_struct *tty)
{
	struct ser_device *ser;
	ser = tty->disc_data;
}

static int start_ldisc(struct ser_device *ser)
{
	struct file *f;
	mm_segment_t oldfs;
	struct termios tio;
	int ldiscnr = N_CAIF;
	int ret;
	f = filp_open(ser->tty_name, 0, 0);
	if (IS_ERR(f)) {
		dev_err(&ser->dev->dev, "CAIF cannot open:%s\n", ser->tty_name);
		ret = -EINVAL;
		goto error;
	}
	if (f == NULL || f->f_op == NULL || f->f_op->unlocked_ioctl == NULL) {
		dev_err(&ser->dev->dev, "TTY cannot do IOCTL:%s\n",
			ser->tty_name);
		ret = -EINVAL;
		goto error;
	}

	ser->file = f;
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	f->f_op->unlocked_ioctl(f, TCFLSH, 0x2);
	memset(&tio, 0, sizeof(tio));
	tio.c_cflag = B115200 | CRTSCTS | CS8 | CLOCAL | CREAD;
	f->f_op->unlocked_ioctl(f, TCSETS, (long unsigned int)&tio);
	f->f_op->unlocked_ioctl(f, TIOCSETD, (long unsigned int)&ldiscnr);
	set_fs(oldfs);
	return 0;
error:
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	return ret;
}

/* The line discipline structure. */
static struct tty_ldisc_ops caif_ldisc = {
	.owner =	THIS_MODULE,
	.magic =	TTY_LDISC_MAGIC,
	.name =		"n_caif",
	.open =		ser_open,
	.close =	ser_close,
	.receive_buf =	ser_receive,
	.write_wakeup =	ser_tx_wakeup
};


static int register_ldisc(struct ser_device *ser)
{
	int result;
	result = tty_register_ldisc(N_CAIF, &caif_ldisc);

	if (result < 0) {
		dev_err(&ser->dev->dev,
			"cannot register CAIF ldisc=%d err=%d\n",
			N_CAIF,
			result);
		return result;
	}
	return result;
}
//official-kernel-patch-cut-here
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27))
#define tty_ldisc tty_ldisc_ops
#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 28))
//official-kernel-patch-resume-here

static const struct net_device_ops netdev_ops = {
	.ndo_open = caif_net_open,
	.ndo_stop = caif_net_close,
	.ndo_start_xmit = caif_xmit
};
//official-kernel-patch-cut-here
#endif
//official-kernel-patch-resume-here
static void caifdev_setup(struct net_device *dev)
{
	struct ser_device *serdev = netdev_priv(dev);
	dev->features = 0;
//official-kernel-patch-cut-here
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 28))
	dev->open = caif_net_open;
	dev->stop = caif_net_close;
	dev->hard_start_xmit = caif_xmit;
#else
//official-kernel-patch-resume-here

	dev->netdev_ops = &netdev_ops;
//official-kernel-patch-cut-here
#endif
//official-kernel-patch-resume-here

	dev->type = ARPHRD_CAIF;
	dev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_POINTOPOINT;
	dev->mtu = CAIF_MAX_MTU;
	dev->hard_header_len = CAIF_NEEDED_HEADROOM;
	dev->tx_queue_len = 0;
	dev->destructor = free_netdev;
	skb_queue_head_init(&serdev->head);
	serdev->common.link_select = CAIF_LINK_LOW_LATENCY;
	serdev->common.use_frag = true;
	serdev->common.use_stx = ser_use_stx;
	serdev->common.use_fcs = true;
	serdev->xoff = 0;
	serdev->dev = dev;
}

static int caif_net_open(struct net_device *dev)
{
	struct ser_device *ser;
	int ret;
	ser = netdev_priv(dev);
	ret = register_ldisc(ser);
	if (ret)
		return ret;
	ret = start_ldisc(ser);
	if (ret) {
		dev_err(&ser->dev->dev, "CAIF: %s() - open failed:%d\n",
			__func__, ret);
		tty_unregister_ldisc(N_CAIF);
		return ret;
	}
	netif_wake_queue(dev);
	ser->xoff = 0;
	return 0;
}

static int caif_net_close(struct net_device *dev)
{
	struct ser_device *ser = netdev_priv(dev);
	netif_stop_queue(dev);
	/* Close the file handle */
	if (ser->file)
		fput(ser->file);
	return tty_unregister_ldisc(N_CAIF);
}

static int register_setenv(char *ttyname,
			   struct net_device **device)
{
	struct net_device *dev;
	struct ser_device *ser;
	int result;
	dev = alloc_netdev(sizeof(*ser), "caifser%d", caifdev_setup);
	if (!dev)
		return -ENODEV;
	ser = netdev_priv(dev);
	pr_info("CAIF: %s(): "
		"Starting CAIF Physical LDisc on tty:%s\n",
		__func__, ttyname);
	ser->tty_name = ttyname;
	netif_stop_queue(dev);
	ser->dev = dev;

	*device = dev;
	result = register_netdev(dev);
	if (result) {
		free_netdev(dev);
		*device = 0;
		return -ENODEV;
	}
	return 0;
}

static int __init caif_ser_init(void)
{
	return register_setenv(ser_ttyname, &device);
}

static void __exit caif_ser_exit(void)
{
	remove_caif_phy_dev(device);
}

module_init(caif_ser_init);
module_exit(caif_ser_exit);
