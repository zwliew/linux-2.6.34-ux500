/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/ sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CAIF_DEVICE_H_
#define CAIF_DEVICE_H_
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/caif/caif_socket.h>
#include <net/caif/caif_device.h>

//official-kernel-patch-cut-here
/* FIXME: Remove ETH_P_CAIF when included in include/linux/if_ether.h */
#ifndef ETH_P_CAIF
#define ETH_P_CAIF	0x00F7		// ST-Ericsson CAIF protocol
#endif	/* ETH_P_CAIF */
/* FIXME: Remove ARPHRD_CAIF when included in include/linux/if_arp.h */
#ifndef ARPHRD_CAIF
#define ARPHRD_CAIF	822		// CAIF media type
#endif /* ARPHRD_CAIF */
//official-kernel-patch-resume-here
/**
 * struct caif_dev_common - shared between CAIF drivers and stack .
 * @flowctrl:		Flow Control callback function.
 * @link_select:	Indicate bandwidth and latency of device.
 * @use_frag:		CAIF Frames may be framented.
 * @use_fcs:		Indicate if Frame CheckSum (fcs) is used.
 * @use_stx:		Indicate STart of frame eXtension (stx) in use.
 *
 * This structure is shared between the CAIF drivers and the CAIF stack.
 */
struct caif_dev_common {
	void (*flowctrl)(struct net_device *net, int on);
	enum caif_link_selector link_select;
	int use_frag;
	int use_fcs;
	int use_stx;
};

#endif	/* CAIF_DEVICE_H_ */

