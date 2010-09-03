/*
 * CAIF Interface registration.
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 *
 * Borrowed heavily from file: pn_dev.c. Thanks to
 *  Remi Denis-Courmont <remi.denis-courmont@nokia.com>
 *  and Sakari Ailus <sakari.ailus@nokia.com>
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/if_arp.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <net/netns/generic.h>
#include <net/net_namespace.h>
#include <net/pkt_sched.h>
#include <net/caif/caif_device.h>
#include <net/caif/caif_dev.h>
#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cfpkt.h>
#include <net/caif/generic/cfcnfg.h>

MODULE_LICENSE("GPL");
#define TIMEOUT (HZ*1000)

/* Used for local tracking of the CAIF net devices */
struct caif_device_entry {
	struct layer layer;
	struct list_head list;
	atomic_t in_use;
	atomic_t state;
	uint16 phyid;
	struct net_device *netdev;
	wait_queue_head_t event;
};

struct caif_device_entry_list {
	struct list_head list;
	spinlock_t lock;
};

struct caif_net {
	struct caif_device_entry_list caifdevs;
};

int caif_net_id;
struct cfcnfg *cfg;

struct caif_device_entry_list *caif_device_list(struct net *net)
{
	struct caif_net *caifn;
	BUG_ON(!net);
	caifn = net_generic(net, caif_net_id);
	BUG_ON(!caifn);
	return &caifn->caifdevs;
}

/* Allocate new CAIF device. */
static struct caif_device_entry *caif_device_alloc(struct net_device *dev)
{
	struct caif_device_entry_list *caifdevs;
	struct caif_device_entry *caifd;
	caifdevs = caif_device_list(dev_net(dev));
	BUG_ON(!caifdevs);
	caifd = kzalloc(sizeof(*caifd), GFP_ATOMIC);
	if (!caifd)
		return NULL;
	caifd->netdev = dev;
	list_add(&caifd->list, &caifdevs->list);
	init_waitqueue_head(&caifd->event);
	return caifd;
}

static struct caif_device_entry *caif_get(struct net_device *dev)
{
	struct caif_device_entry_list *caifdevs =
	    caif_device_list(dev_net(dev));
	struct caif_device_entry *caifd;
	BUG_ON(!caifdevs);
	list_for_each_entry(caifd, &caifdevs->list, list) {
		if (caifd->netdev == dev)
			return caifd;
	}
	return NULL;
}

static void caif_device_destroy(struct net_device *dev)
{
	struct caif_device_entry_list *caifdevs =
	    caif_device_list(dev_net(dev));
	struct caif_device_entry *caifd;
	ASSERT_RTNL();
	if (dev->type != ARPHRD_CAIF)
		return;

	spin_lock_bh(&caifdevs->lock);
	caifd = caif_get(dev);
	if (caifd == NULL) {
		spin_unlock_bh(&caifdevs->lock);
		return;
	}

	list_del(&caifd->list);
	spin_unlock_bh(&caifdevs->lock);

	kfree(caifd);
	return;
}

static int transmit(struct layer *layer, struct cfpkt *pkt)
{
	struct caif_device_entry *caifd =
	    container_of(layer, struct caif_device_entry, layer);
	struct sk_buff *skb, *skb2;
	int ret = -EINVAL;
	skb = cfpkt_tonative(pkt);
	skb->dev = caifd->netdev;
	/*
	 * Don't allow SKB to be destroyed upon error, but signal resend
	 * notification to clients. We can't rely on the return value as
	 * congestion (NET_XMIT_CN) sometimes drops the packet, sometimes don't.
	 */
	if (netif_queue_stopped(caifd->netdev))
		return -EAGAIN;
	skb2 = skb_get(skb);

	ret = dev_queue_xmit(skb2);

	if (!ret)
		kfree_skb(skb);
	else
		return -EAGAIN;

	return 0;
}

static int modemcmd(struct layer *layr, enum caif_modemcmd ctrl)
{
	struct caif_device_entry *caifd;
	struct caif_dev_common *caifdev;
	caifd = container_of(layr, struct caif_device_entry, layer);
	caifdev = netdev_priv(caifd->netdev);
	if (ctrl == _CAIF_MODEMCMD_PHYIF_USEFULL) {
		atomic_set(&caifd->in_use, 1);
		wake_up_interruptible(&caifd->event);

	} else if (ctrl == _CAIF_MODEMCMD_PHYIF_USELESS) {
		atomic_set(&caifd->in_use, 0);
		wake_up_interruptible(&caifd->event);
	}
	return 0;
}

/*
 * Stuff received packets to associated sockets.
 * On error, returns non-zero and releases the skb.
 */
static int receive(struct sk_buff *skb, struct net_device *dev,
		   struct packet_type *pkttype, struct net_device *orig_dev)
{
	struct net *net;
	struct cfpkt *pkt;
	struct caif_device_entry *caifd;
	net = dev_net(dev);
	pkt = cfpkt_fromnative(CAIF_DIR_IN, skb);
	caifd = caif_get(dev);
	if (!caifd || !caifd->layer.up || !caifd->layer.up->ctrlcmd)
		return NET_RX_DROP;

	if (caifd->layer.up->receive(caifd->layer.up, pkt))
		return NET_RX_DROP;

	return 0;
}

static struct packet_type caif_packet_type __read_mostly = {
	.type = cpu_to_be16(ETH_P_CAIF),
	.func = receive,
};

static void dev_flowctrl(struct net_device *dev, int on)
{
	struct caif_device_entry *caifd = caif_get(dev);
	if (!caifd || !caifd->layer.up || !caifd->layer.up->ctrlcmd)
		return;

	caifd->layer.up->ctrlcmd(caifd->layer.up,
				 on ?
				 _CAIF_CTRLCMD_PHYIF_FLOW_ON_IND :
				 _CAIF_CTRLCMD_PHYIF_FLOW_OFF_IND,
				 caifd->layer.id);
}

/* notify Caif of device events */
static int caif_device_notify(struct notifier_block *me, unsigned long what,
			      void *arg)
{
	struct net_device *dev = arg;
	struct caif_device_entry *caifd = NULL;
	struct caif_dev_common *caifdev;
	int res = -EINVAL;
	enum caif_phy_preference phy_pref;
	enum cfcnfg_phy_type phy_type;

	if (dev->type != ARPHRD_CAIF)
		return 0;

	switch (what) {
	case NETDEV_REGISTER:
		pr_info("CAIF: %s():register %s\n", __func__, dev->name);
		caifd = caif_device_alloc(dev);
		if (caifd == NULL)
			break;
		caifdev = netdev_priv(dev);
		caifdev->flowctrl = dev_flowctrl;
		atomic_set(&caifd->state, what);
		res = 0;
		break;

	case NETDEV_UP:
		pr_info("CAIF: %s(): up %s\n", __func__, dev->name);
		caifd = caif_get(dev);
		if (caifd == NULL)
			break;
		caifdev = netdev_priv(dev);
		if (atomic_read(&caifd->state) == NETDEV_UP) {
			pr_info("CAIF: %s():%s already up\n",
				__func__, dev->name);
			break;
		}
		atomic_set(&caifd->state, what);
		caifd->layer.transmit = transmit;
		caifd->layer.modemcmd = modemcmd;
		if (caifdev->link_select == CAIF_PHYPREF_LOW_LAT)
			phy_pref = CAIF_LINK_LOW_LATENCY;
		else
			phy_pref = CAIF_LINK_HIGH_BANDW;

		if (caifdev->use_frag)
			phy_type = CFPHYTYPE_FRAG;
		else
			phy_type = CFPHYTYPE_CAIF;

		cfcnfg_add_phy_layer(get_caif_conf(),
				     phy_type,
				     dev,
				     &caifd->layer,
				     &caifd->phyid,
				     phy_pref,
				     caifdev->use_fcs,
				     caifdev->use_stx);
		strncpy(caifd->layer.name, dev->name,
			sizeof(caifd->layer.name) - 1);
		caifd->layer.name[sizeof(caifd->layer.name) - 1] = 0;
		break;

	case NETDEV_GOING_DOWN:
		caifd = caif_get(dev);
		if (caifd == NULL)
			break;
		pr_info("CAIF: %s():going down %s\n", __func__, dev->name);
		atomic_set(&caifd->state, what);
		if (!caifd || !caifd->layer.up || !caifd->layer.up->ctrlcmd)
			return -EINVAL;
		caifd->layer.up->ctrlcmd(caifd->layer.up,
					 _CAIF_CTRLCMD_PHYIF_DOWN_IND,
					 caifd->layer.id);
		res = wait_event_interruptible_timeout(caifd->event,
					atomic_read(&caifd->in_use) == 0,
					TIMEOUT);
		break;

	case NETDEV_DOWN:
		caifd = caif_get(dev);
		if (caifd == NULL)
			break;
		pr_info("CAIF: %s(): down %s\n", __func__, dev->name);
		if (atomic_read(&caifd->in_use))
			pr_warning("CAIF: %s(): "
				   "Unregistering an active CAIF device: %s\n",
				   __func__, dev->name);
		cfcnfg_del_phy_layer(get_caif_conf(), &caifd->layer);
		atomic_set(&caifd->state, what);
		break;

	case NETDEV_UNREGISTER:
		caifd = caif_get(dev);
		pr_info("CAIF: %s(): unregister %s\n", __func__, dev->name);
		atomic_set(&caifd->state, what);
		caif_device_destroy(dev);
		break;
	}
	return 0;
}

static struct notifier_block caif_device_notifier = {
	.notifier_call = caif_device_notify,
	.priority = 0,
};


struct cfcnfg *get_caif_conf(void)
{
	return cfg;
}
EXPORT_SYMBOL(get_caif_conf);

static int (*caif_ioctl_func)(unsigned int cmd, unsigned long arg, bool);

void caif_register_ioctl(
			 int (*ioctl)(unsigned int cmd,
				      unsigned long arg,
				      bool))
{
	caif_ioctl_func = ioctl;
}
EXPORT_SYMBOL(caif_register_ioctl);

int caif_ioctl(unsigned int cmd, unsigned long arg, bool from_user_land)
{
	if (caif_ioctl_func == NULL)
		return -EINVAL;
	return caif_ioctl_func(cmd, arg, from_user_land);
}
EXPORT_SYMBOL(caif_ioctl);

int caifdev_adapt_register(struct caif_channel_config *config,
			   struct layer *adap_layer)
{
	struct cfctrl_link_param param;

	if (channel_config_2_link_param(get_caif_conf(), config, &param) == 0)
		/* Hook up the adaptation layer. */
		return cfcnfg_add_adaptation_layer(get_caif_conf(),
						&param, adap_layer);

	return -EINVAL;
}
EXPORT_SYMBOL(caifdev_adapt_register);

int caifdev_adapt_unregister(struct layer *adap_layer)
{
	return cfcnfg_del_adapt_layer(get_caif_conf(), adap_layer);
}
EXPORT_SYMBOL(caifdev_adapt_unregister);

//official-kernel-patch-cut-here
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 32))
//official-kernel-patch-resume-here
/* Per-namespace Caif devices handling */
static int caif_init_net(struct net *net)
{
	struct caif_net *caifn = net_generic(net, caif_net_id);
	INIT_LIST_HEAD(&caifn->caifdevs.list);
	spin_lock_init(&caifn->caifdevs.lock);
	return 0;
}

static void caif_exit_net(struct net *net)
{
	struct net_device *dev;
	int res;
	rtnl_lock();
	for_each_netdev(net, dev) {
		if (dev->type != ARPHRD_CAIF)
			continue;
		res = dev_close(dev);
		caif_device_destroy(dev);
	}
	rtnl_unlock();
}

static struct pernet_operations caif_net_ops = {
	.init = caif_init_net,
	.exit = caif_exit_net,
	.id   = &caif_net_id,
	.size = sizeof(struct caif_net),
};
//official-kernel-patch-cut-here
#else

/* Per-namespace Caif devices handling */
static int caif_init_net(struct net *net)
{
	struct caif_net *caifn;
	int ret;
	caifn = kmalloc(sizeof(*caifn), GFP_KERNEL);
	if (!caifn)
		return -ENOMEM;
	INIT_LIST_HEAD(&caifn->caifdevs.list);
	spin_lock_init(&caifn->caifdevs.lock);
	ret = net_assign_generic(net, caif_net_id, caifn);
	if (ret)
		kfree(caifn);

	return 0;
}

static void caif_exit_net(struct net *net)
{
	struct caif_net *caifn = net_generic(net, caif_net_id);
	struct net_device *dev;
	int res;
	rtnl_lock();
	for_each_netdev(net, dev) {
		if (dev->type != ARPHRD_CAIF)
			continue;
		res = dev_close(dev);
		caif_device_destroy(dev);
	}
	rtnl_unlock();
	kfree(caifn);
}

static struct pernet_operations caif_net_ops = {
	.init = caif_init_net,
	.exit = caif_exit_net,
};

#endif
//official-kernel-patch-resume-here

/* Initialize Caif devices list */
int __init caif_device_init(void)
{
	int result;
	cfg = cfcnfg_create();
	if (!cfg) {
		pr_warning("CAIF: %s(): can't create cfcnfg.\n", __func__);
		goto err_cfcnfg_create_failed;
	}
//official-kernel-patch-cut-here
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 32))
//official-kernel-patch-resume-here
	result = register_pernet_device(&caif_net_ops);
//official-kernel-patch-cut-here
#else
	result = register_pernet_gen_device(&caif_net_id,&caif_net_ops);
#endif
//official-kernel-patch-resume-here

	if (result) {
		kfree(cfg);
		cfg = NULL;
		return result;
	}
	dev_add_pack(&caif_packet_type);
	register_netdevice_notifier(&caif_device_notifier);

	return result;
err_cfcnfg_create_failed:
	return -ENODEV;
}

void __exit caif_device_exit(void)
{
	cfcnfg_remove(cfg);
	dev_remove_pack(&caif_packet_type);
//official-kernel-patch-cut-here
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 32))
//official-kernel-patch-resume-here
	unregister_pernet_device(&caif_net_ops);
//official-kernel-patch-cut-here
#else
	unregister_pernet_gen_device(caif_net_id,&caif_net_ops);
#endif
//official-kernel-patch-resume-here
	unregister_netdevice_notifier(&caif_device_notifier);
}

module_init(caif_device_init);
module_exit(caif_device_exit);
