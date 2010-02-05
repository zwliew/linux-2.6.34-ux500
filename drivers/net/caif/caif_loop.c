/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland / sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <net/pkt_sched.h>
#include <linux/if_arp.h>
#include <net/caif/caif_device.h>
#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cfcnfg.h>

MODULE_LICENSE("GPL");

struct caif_loop_dev {
	struct caif_dev_common common;
	struct net_device *dev;
	int flow_on;
	int queue_on;
	int copycat;
};
#define CAIF_MAX_MTU 4096

static ssize_t store_queue_on(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t len)
{
	unsigned long val;
	struct net_device *netdev = to_net_dev(dev);
	struct caif_loop_dev *loopdev = netdev_priv(netdev);
	strict_strtoul(buf, 10, &val);
	loopdev->queue_on = val;
	if (loopdev && loopdev->common.flowctrl)
		loopdev->common.flowctrl(netdev, (int)val);
	else
		printk(KERN_WARNING "caif_loop:flowctrl not set\n");
	return len;
}

static ssize_t show_queue_on(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_dev(dev);
	struct caif_loop_dev *loopdev = netdev_priv(netdev);
	return sprintf(buf, "%u\n", loopdev->queue_on);
}

static ssize_t store_copycat(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t len)
{
	unsigned long val;
	struct net_device *netdev = to_net_dev(dev);
	struct caif_loop_dev *loopdev = netdev_priv(netdev);
	strict_strtoul(buf, 10, &val);
	loopdev->copycat = val;
	printk(KERN_INFO "caif_loop:copycat:%lu\n",val);
	return len;
}

static ssize_t show_copycat(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_dev(dev);
	struct caif_loop_dev *loopdev = netdev_priv(netdev);
	return sprintf(buf, "%u\n", loopdev->copycat);
}

static ssize_t store_flow_on(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf, size_t len)
{
	unsigned long val;
	struct net_device *netdev = to_net_dev(dev);
	struct caif_loop_dev *loopdev = netdev_priv(netdev);
	strict_strtoul(buf, 10, &val);
	loopdev->flow_on = val;
	return len;
}

static ssize_t show_flow_on(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_dev(dev);
	struct caif_loop_dev *loopdev = netdev_priv(netdev);
	return sprintf(buf, "%u\n", loopdev->flow_on);
}

static struct device_attribute attrs[] = {
	__ATTR(flow_on, S_IRUGO | S_IWUSR, show_flow_on, store_flow_on),
	__ATTR(queue_on, S_IRUGO | S_IWUSR, show_queue_on, store_queue_on),
	__ATTR(copycat, S_IRUGO | S_IWUSR, show_copycat, store_copycat),
};

static int sysfs_add(struct net_device *netdev)
{
	int i;
	int err;
	for (i = 0; i < ARRAY_SIZE(attrs); i++) {
		err = device_create_file(&netdev->dev, &attrs[i]);
		if (err)
			goto fail;
	}
	return 0;

 fail:
	while (--i >= 0)
		device_remove_file(&netdev->dev, &attrs[i]);
	return err;
}

static void sysfs_rem(struct net_device *netdev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attrs); i++)
		device_remove_file(&netdev->dev, &attrs[i]);
}

/*
 * Send a SKB from net_device to the CAIF stack.
 */
static int caif_recv(struct net_device *dev, struct sk_buff *skb)
{
	int ret,i;
	struct caif_loop_dev *caifd;
	skb->protocol = htons(ETH_P_CAIF);
	skb_reset_mac_header(skb);
	skb->dev = dev;
	dev->stats.rx_packets++;
	dev->stats.rx_bytes += skb->len;
	caifd = netdev_priv(dev);

	if (caifd->flow_on > 0) {
		--caifd->flow_on;
		return NET_XMIT_DROP;
	}
	for (i = caifd->copycat; i; i--) {
		struct sk_buff *clone;
		clone = skb_clone(skb,GFP_ATOMIC);
		netif_rx(clone);
	}
	ret = netif_rx(skb);
	if (ret == NET_RX_DROP)
		return NET_XMIT_DROP;
	return 0;
}

void debug_netdev(struct net_device *dev)
{
	struct netdev_queue *txq;
	struct Qdisc *q;
	txq = netdev_get_tx_queue(dev, 0);
	q = rcu_dereference(txq->qdisc);
	printk(KERN_INFO "txq:%p q:%p enqueue:%p - %s\n",
	       txq, q, q ? q->enqueue : NULL, q->ops ? q->ops->id : "");
}

static int caif_xmit(struct sk_buff *skb, struct net_device *dev)
{
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;
	return caif_recv(dev, skb);
}

static int caif_open(struct net_device *dev)
{
	struct caif_loop_dev *caifd;
	netif_wake_queue(dev);
	caifd = netdev_priv(dev);
	caifd->flow_on = 0;
	return 0;
}

static int caif_close(struct net_device *dev)
{
	netif_stop_queue(dev);
	return 0;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 28))
static const struct net_device_ops netdev_ops = {
	.ndo_open = caif_open,
	.ndo_stop = caif_close,
	.ndo_start_xmit = caif_xmit
};
#endif

static void caifdev_setup(struct net_device *dev)
{
	struct caif_loop_dev *loopdev = netdev_priv(dev);
	dev->features = 0;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 28))
	dev->open = caif_open;
	dev->stop = caif_close;
	dev->hard_start_xmit = caif_xmit;
#else

	dev->netdev_ops = &netdev_ops;
#endif
	dev->type = ARPHRD_CAIF;
	dev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_POINTOPOINT;
	dev->mtu = CAIF_MAX_MTU;
	dev->hard_header_len = CAIF_NEEDED_HEADROOM;
	dev->tx_queue_len = 0;
	dev->destructor = free_netdev;
	loopdev->common.link_select = CAIF_LINK_HIGH_BANDW;
	loopdev->common.use_frag = false;
	loopdev->common.use_stx = false;
	loopdev->common.use_fcs = false;

}

int create_caif_phy_dev(char *ifname, struct net_device **dev)
{
	struct caif_loop_dev *caifd;
	int err;
	if (!dev)
		return -1;
	*dev = alloc_netdev(sizeof(*caifd), ifname, caifdev_setup);
	if (!*dev)
		return -ENOMEM;
	caifd = netdev_priv(*dev);
	caifd->flow_on = 1;
	netif_stop_queue(*dev);
	caifd->dev = *dev;
	err = register_netdev(*dev);
	sysfs_add(*dev);
	if (err)
		goto err;
	return 0;
 err:
	printk(KERN_WARNING "Error in create_phy_loop\n");
	free_netdev(*dev);
	return err;
}

static void remove_caif_phy_dev(struct net_device *dev)
{
	sysfs_rem(dev);
	rtnl_lock();
	dev_close(dev);
	/* device is freed automagically by net-sysfs */
	unregister_netdevice(dev);
	rtnl_unlock();
}

static struct net_device *caif_phy_dev_inst;
static int __init phyif_loop_init(void)
{
	int result;
	result = create_caif_phy_dev("caifloop%d", &caif_phy_dev_inst);
	return result;
}

static void __exit phyif_loop_exit(void)
{
	remove_caif_phy_dev(caif_phy_dev_inst);
}

module_init(phyif_loop_init);
module_exit(phyif_loop_exit);
