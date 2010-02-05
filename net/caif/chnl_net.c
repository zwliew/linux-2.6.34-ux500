/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/version.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>
#include <linux/moduleparam.h>
#include <linux/ip.h>
#include <linux/sched.h>
#include <linux/sockios.h>
#include <linux/caif/if_caif.h>
#include <net/rtnetlink.h>
#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cfcnfg.h>
#include <net/caif/generic/cfpkt.h>
#include <net/caif/caif_dev.h>

#define CAIF_CONNECT_TIMEOUT 30
#define SIZE_MTU 1500
#define SIZE_MTU_MAX 4080
#define SIZE_MTU_MIN 68
#define CAIF_NET_DEFAULT_QUEUE_LEN 500

/*This list isi protected by the rtnl lock. */
static LIST_HEAD(chnl_net_list);

MODULE_LICENSE("GPL");
MODULE_ALIAS_RTNL_LINK("caif");

struct chnl_net {
	struct layer chnl;
	struct net_device_stats stats;
	struct caif_channel_config config;
	struct list_head list_field;
	struct net_device *netdev;
	char name[256];
	wait_queue_head_t netmgmt_wq;
	/* Flow status to remember and control the transmission. */
	bool flowenabled;
};


static struct chnl_net *find_device(char *name)
{
	struct list_head *list_node;
	struct list_head *n;
	struct chnl_net *dev = NULL;
	struct chnl_net *tmp;
	ASSERT_RTNL();
	list_for_each_safe(list_node, n, &chnl_net_list) {
		tmp = list_entry(list_node, struct chnl_net, list_field);
		/* Find from name. */
		if (name) {
			if (!strncmp(tmp->name, name, sizeof(tmp->name)))
				dev = tmp;
			else if (!strncmp(tmp->netdev->name,
					  name,
					  sizeof(tmp->netdev->name)))
				dev = tmp;
		} else
			/* Get the first element if name is not specified. */
			dev = tmp;
		if (dev)
			break;
	}
	return dev;
}

static void robust_list_del(struct list_head *delete_node)
{
	struct list_head *list_node;
	struct list_head *n;
	ASSERT_RTNL();
	list_for_each_safe(list_node, n, &chnl_net_list) {
		if (list_node == delete_node) {
			list_del(list_node);
			break;
		}
	}
}

static int chnl_recv_cb(struct layer *layr, struct cfpkt *pkt)
{
	struct sk_buff *skb;
	struct chnl_net *priv  = NULL;
	int pktlen;
	int err = 0;

	priv = container_of(layr, struct chnl_net, chnl);

	if (!priv)
		return -EINVAL;

	/* Get length of CAIF packet. */
	pktlen = cfpkt_getlen(pkt);

	skb = (struct sk_buff *) cfpkt_tonative(pkt);
	/* Pass some minimum information and
	 * send the packet to the net stack.
	 */
	skb->dev = priv->netdev;
	skb->protocol = htons(ETH_P_IP);

	if (priv->config.type == CAIF_CHTY_DATAGRAM_LOOP) {
		/* We change the header, so the checksum is corrupted. */
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	} else {
		skb->ip_summed = CHECKSUM_COMPLETE;
	}

	/* FIXME: Drivers should call this in tasklet context. */
	if (in_interrupt())
		netif_rx(skb);
	else
		netif_rx_ni(skb);

	/* Update statistics. */
	priv->netdev->stats.rx_packets++;
	priv->netdev->stats.rx_bytes += pktlen;

	return err;
}

static void chnl_flowctrl_cb(struct layer *layr, enum caif_ctrlcmd flow,
				int phyid)
{
	struct chnl_net *priv  = NULL;
	pr_debug("CAIF: %s(): NET flowctrl func called flow: %s.\n",
		__func__,
		flow == CAIF_CTRLCMD_FLOW_ON_IND ? "ON" :
		flow == CAIF_CTRLCMD_INIT_RSP ? "INIT" :
		flow == CAIF_CTRLCMD_FLOW_OFF_IND ? "OFF" :
		flow == CAIF_CTRLCMD_DEINIT_RSP ? "CLOSE/DEINIT" :
		flow == CAIF_CTRLCMD_INIT_FAIL_RSP ? "OPEN_FAIL" :
		flow == CAIF_CTRLCMD_REMOTE_SHUTDOWN_IND ?
		 "REMOTE_SHUTDOWN" : "UKNOWN CTRL COMMAND");

	priv = container_of(layr, struct chnl_net, chnl);

	switch (flow) {
	case CAIF_CTRLCMD_FLOW_OFF_IND:
	case CAIF_CTRLCMD_DEINIT_RSP:
	case CAIF_CTRLCMD_INIT_FAIL_RSP:
	case CAIF_CTRLCMD_REMOTE_SHUTDOWN_IND:
		priv->flowenabled = false;
		netif_tx_disable(priv->netdev);
		wake_up_interruptible(&priv->netmgmt_wq);
	break;
	case CAIF_CTRLCMD_FLOW_ON_IND:
	case CAIF_CTRLCMD_INIT_RSP:
		priv->flowenabled = true;
		netif_wake_queue(priv->netdev);
		wake_up_interruptible(&priv->netmgmt_wq);
	break;
	default:
		break;
	}
}

static int chnl_net_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct chnl_net *priv;
	struct cfpkt *pkt = NULL;
	int len;
	int result = -1;
	ASSERT_RTNL();

	/* Get our private data. */
	priv = (struct chnl_net *)netdev_priv(dev);
	if (!priv)
		return -ENOSPC;


	if (skb->len > priv->netdev->mtu) {
		pr_warning("CAIF: %s(): Size of skb exceeded MTU\n", __func__);
		return -ENOSPC;
	}

	if (!priv->flowenabled) {
		pr_debug("CAIF: %s(): dropping packets flow off\n", __func__);
		return NETDEV_TX_BUSY;
	}

	if (priv->config.type == CAIF_CHTY_DATAGRAM_LOOP) {
		struct iphdr *hdr;
		__be32 swap;
		/* Retrieve IP header. */
		hdr = ip_hdr(skb);
		/* Change source and destination address. */
		swap = hdr->saddr;
		hdr->saddr = hdr->daddr;
		hdr->daddr = swap;
	}
	/* Store original SKB length. */
	len = skb->len;

	pkt = cfpkt_fromnative(CAIF_DIR_OUT, (void *) skb);

	/* Send the packet down the stack. */
	result = priv->chnl.dn->transmit(priv->chnl.dn, pkt);
	if (result) {
		if (result == CFGLU_ERETRY)
			result = NETDEV_TX_BUSY;
		return result;
	}

	/* Update statistics. */
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += len;

	return NETDEV_TX_OK;
}


static int chnl_net_open(struct net_device *dev)
{
	struct chnl_net *priv = NULL;
	int result = -1;
	ASSERT_RTNL();

	priv = (struct chnl_net *)netdev_priv(dev);
	pr_debug("CAIF: %s(): dev name: %s\n", __func__, priv->name);

	if (!priv) {
		pr_debug("CAIF: %s(): chnl_net_open: no priv\n", __func__);
		return -ENODEV;
	}
	result = caifdev_adapt_register(&priv->config, &priv->chnl);
	if (result != 0) {
		pr_debug("CAIF: %s(): err: "
			 "Unable to register and open device, Err:%d\n",
			__func__,
			result);
		return -ENODEV;
	}
	result = wait_event_interruptible(priv->netmgmt_wq, priv->flowenabled);

	if (result == -ERESTARTSYS) {
		pr_debug("CAIF: %s(): wait_event_interruptible"
			 " woken by a signal\n", __func__);
		return -ERESTARTSYS;
	} else
		pr_debug("CAIF: %s(): Flow on recieved\n", __func__);

	return 0;
}

static int chnl_net_stop(struct net_device *dev)
{
	struct chnl_net *priv;
	int result = -1;
	ASSERT_RTNL();
	priv = (struct chnl_net *)netdev_priv(dev);

	result = caifdev_adapt_unregister(&priv->chnl);
	if (result != 0) {
		pr_debug("CAIF: %s(): chnl_net_stop: err: "
			 "Unable to STOP device, Err:%d\n",
			 __func__, result);
		return -EBUSY;
	}
	result = wait_event_interruptible(priv->netmgmt_wq,
					  !priv->flowenabled);

	if (result == -ERESTARTSYS) {
		pr_debug("CAIF: %s(): wait_event_interruptible woken by"
			 " signal, signal_pending(current) = %d\n",
			 __func__,
			 signal_pending(current));
	} else {
		pr_debug("CAIF: %s(): disconnect received\n", __func__);

	}

	return 0;
}

int chnl_net_init(struct net_device *dev)
{
	struct chnl_net *priv;
	ASSERT_RTNL();
	priv = (struct chnl_net *)netdev_priv(dev);
	strncpy(priv->config.name, dev->name, sizeof(priv->config.name));
	strncpy(priv->name, dev->name, sizeof(priv->name));
	return 0;
}

void chnl_net_uninit(struct net_device *dev)
{
	struct chnl_net *priv;
	ASSERT_RTNL();
	priv = (struct chnl_net *)netdev_priv(dev);
	/* If someone already have the lock it's already protected */
	robust_list_del(&priv->list_field);
	dev_put(dev);
}

//official-kernel-patch-cut-here
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 28))
//official-kernel-patch-resume-here
static const struct net_device_ops netdev_ops = {
	.ndo_open = chnl_net_open,
	.ndo_stop = chnl_net_stop,
	.ndo_init = chnl_net_init,
	.ndo_uninit = chnl_net_uninit,
	.ndo_start_xmit = chnl_net_hard_start_xmit,
};
//official-kernel-patch-cut-here
#endif
//official-kernel-patch-resume-here

static void ipcaif_net_init(struct net_device *dev)
{
	struct chnl_net *priv;
//official-kernel-patch-cut-here
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 28))
	dev->open = chnl_net_open;
	dev->stop = chnl_net_stop;
	dev->init = chnl_net_init;
	dev->uninit = chnl_net_uninit;
	dev->hard_start_xmit = chnl_net_hard_start_xmit;
#else
//official-kernel-patch-resume-here
	dev->netdev_ops = &netdev_ops;
//official-kernel-patch-cut-here
#endif
//official-kernel-patch-resume-here
	dev->destructor = free_netdev;
	dev->flags |= IFF_NOARP;
	dev->flags |= IFF_POINTOPOINT;
	dev->needed_headroom = CAIF_NEEDED_HEADROOM;
	dev->needed_tailroom = CAIF_NEEDED_TAILROOM;
	dev->mtu = SIZE_MTU;
	dev->tx_queue_len = CAIF_NET_DEFAULT_QUEUE_LEN;

	priv = (struct chnl_net *)netdev_priv(dev);
	priv->chnl.receive = chnl_recv_cb;
	priv->chnl.ctrlcmd = chnl_flowctrl_cb;
	priv->netdev = dev;
	priv->config.type = CAIF_CHTY_DATAGRAM;
	priv->config.phy_pref = CFPHYPREF_HIGH_BW;
	priv->config.priority = CAIF_PRIO_LOW;
	priv->config.u.dgm.connection_id = -1; /* Insert illegal value */
	priv->flowenabled = false;

	ASSERT_RTNL();
	init_waitqueue_head(&priv->netmgmt_wq);
	list_add(&priv->list_field, &chnl_net_list);
}

static int delete_device(struct chnl_net *dev)
{
	ASSERT_RTNL();
	if (dev->netdev)
		unregister_netdevice(dev->netdev);
	return 0;
}

static int ipcaif_fill_info(struct sk_buff *skb, const struct net_device *dev)
{
	struct chnl_net *priv;
	u8 loop;
	priv = (struct chnl_net *)netdev_priv(dev);
	NLA_PUT_U32(skb, IFLA_CAIF_IPV4_CONNID,
		    priv->config.u.dgm.connection_id);
	NLA_PUT_U32(skb, IFLA_CAIF_IPV6_CONNID,
		    priv->config.u.dgm.connection_id);
	loop = priv->config.type == CAIF_CHTY_DATAGRAM_LOOP;
	NLA_PUT_U8(skb, IFLA_CAIF_LOOPBACK, loop);


	return 0;
nla_put_failure:
	return -EMSGSIZE;

}

static void caif_netlink_parms(struct nlattr *data[],
				struct caif_channel_config *parms)
{
	if (!data) {
		pr_warning("CAIF: %s: no params data found\n", __func__);
		return;
	}
	if (data[IFLA_CAIF_IPV4_CONNID])
		parms->u.dgm.connection_id =
			nla_get_u32(data[IFLA_CAIF_IPV4_CONNID]);
	if (data[IFLA_CAIF_IPV6_CONNID])
		parms->u.dgm.connection_id =
			nla_get_u32(data[IFLA_CAIF_IPV6_CONNID]);
	if (data[IFLA_CAIF_LOOPBACK]) {
		if (nla_get_u8(data[IFLA_CAIF_LOOPBACK]))
			parms->type = CAIF_CHTY_DATAGRAM_LOOP;
		else
			parms->type = CAIF_CHTY_DATAGRAM;
	}
}

//official-kernel-patch-cut-here
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 32))
//official-kernel-patch-resume-here
static int ipcaif_newlink(struct net *src_net, struct net_device *dev,
			  struct nlattr *tb[], struct nlattr *data[])
//official-kernel-patch-cut-here
#else
static int ipcaif_newlink(struct net_device *dev, struct nlattr *tb[],
			struct nlattr *data[])
#endif
//official-kernel-patch-resume-here
{
	int err;
	struct chnl_net *caifdev;
	ASSERT_RTNL();
	caifdev = netdev_priv(dev);
	caif_netlink_parms(data, &caifdev->config);
	err = register_netdevice(dev);
	if (err) {
		pr_warning("CAIF: %s(): device rtml registration failed\n",
			   __func__);
		goto out;
	}
	dev_hold(dev);
out:
	return err;
}

static int ipcaif_changelink(struct net_device *dev, struct nlattr *tb[],
				struct nlattr *data[])
{
	struct chnl_net *caifdev;
	ASSERT_RTNL();
	caifdev = netdev_priv(dev);
	caif_netlink_parms(data, &caifdev->config);
	netdev_state_change(dev);
	return 0;
}

static size_t ipcaif_get_size(const struct net_device *dev)
{
	return
		/* IFLA_CAIF_IPV4_CONNID */
		nla_total_size(4) +
		/* IFLA_CAIF_IPV6_CONNID */
		nla_total_size(4) +
		/* IFLA_CAIF_LOOPBACK */
		nla_total_size(2) +
		0;
}

static const struct nla_policy ipcaif_policy[IFLA_CAIF_MAX + 1] = {
	[IFLA_CAIF_IPV4_CONNID]	      = { .type = NLA_U32 },
	[IFLA_CAIF_IPV6_CONNID]	      = { .type = NLA_U32 },
	[IFLA_CAIF_LOOPBACK]	      = { .type = NLA_U8 }
};


static struct rtnl_link_ops ipcaif_link_ops __read_mostly = {
	.kind		= "caif",
	.priv_size	= (size_t)sizeof(struct chnl_net),
	.setup		= ipcaif_net_init,
	.maxtype	= IFLA_CAIF_MAX,
	.policy		= ipcaif_policy,
	.newlink	= ipcaif_newlink,
	.changelink	= ipcaif_changelink,
	.get_size	= ipcaif_get_size,
	.fill_info	= ipcaif_fill_info,

};

int chnl_net_ioctl(unsigned int cmd, unsigned long arg, bool from_user_land)
{
	struct chnl_net *priv;
	int result = -1;
	struct chnl_net *dev;
	struct net_device *netdevptr;
	int ret;
	struct ifreq ifreq;
	struct ifcaif_param param;
	rtnl_lock();
	if (from_user_land) {
		if (copy_from_user(&ifreq, (const void *)arg, sizeof(ifreq)))
			return -EFAULT;
	} else
		memcpy(&ifreq, (void *)arg, sizeof(ifreq));

	if (cmd == SIOCCAIFNETREMOVE) {
		pr_debug("CAIF: %s(): %s\n", __func__, ifreq.ifr_name);
		dev = find_device(ifreq.ifr_name);
		if (!dev)
			ret = -ENODEV;
		else
			ret = delete_device(dev);
		rtnl_unlock();
		return ret;
	}

	if (cmd != SIOCCAIFNETNEW) {
		rtnl_unlock();
		return -ENOIOCTLCMD;
	}
	if (ifreq.ifr_ifru.ifru_data != NULL) {
		if (from_user_land) {
			ret = copy_from_user(&param,
					ifreq.ifr_ifru.ifru_data,
					sizeof(param));
			if (ret) {
				rtnl_unlock();
				return -EFAULT;
			}
		} else
			memcpy(&param,
				ifreq.ifr_ifru.ifru_data,
				sizeof(param));
		ifreq.ifr_ifru.ifru_data = &param;
	}

	netdevptr = alloc_netdev(sizeof(struct chnl_net),
				 ifreq.ifr_name, ipcaif_net_init);
	if (!netdevptr) {
		rtnl_unlock();
		return	-ENODEV;
	}
	dev_hold(netdevptr);
	priv = (struct chnl_net *)netdev_priv(netdevptr);
	priv->config.u.dgm.connection_id = param.ipv4_connid;

	if (param.loop)
		priv->config.type = CAIF_CHTY_DATAGRAM_LOOP;
	else
		priv->config.type = CAIF_CHTY_DATAGRAM;

	result = register_netdevice(priv->netdev);

	if (result < 0) {
		pr_warning("CAIF: %s(): can't register netdev %s %d\n",
			   __func__, ifreq.ifr_name, result);
		dev_put(netdevptr);
		rtnl_unlock();
		return	-ENODEV;
	}
	pr_debug("CAIF: %s(): netdev channel open:%s\n", __func__, priv->name);
	rtnl_unlock();
	return 0;
};

struct net_device *chnl_net_create(char *name,
				   struct caif_channel_config *config)
{
	struct net_device *dev;
	ASSERT_RTNL();
	dev = alloc_netdev(sizeof(struct chnl_net), name, ipcaif_net_init);
	if (!dev)
		return NULL;
	((struct chnl_net *)netdev_priv(dev))->config = *config;
	dev_hold(dev);
	return dev;
}
EXPORT_SYMBOL(chnl_net_create);

static int __init chnl_init_module(void)
{
	int err = -1;
	caif_register_ioctl(chnl_net_ioctl);
	err = rtnl_link_register(&ipcaif_link_ops);
	if (err < 0) {
		rtnl_link_unregister(&ipcaif_link_ops);
		return err;
	}
	return 0;
}

static void __exit chnl_exit_module(void)
{
	struct chnl_net *dev = NULL;
	struct list_head *list_node;
	struct list_head *_tmp;
	rtnl_lock();
	list_for_each_safe(list_node, _tmp, &chnl_net_list) {
		dev = list_entry(list_node, struct chnl_net, list_field);
		delete_device(dev);
	}
	rtnl_unlock();
	rtnl_link_unregister(&ipcaif_link_ops);
	caif_register_ioctl(NULL);
}

module_init(chnl_init_module);
module_exit(chnl_exit_module);
