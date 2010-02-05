/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/ sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CAIF_DEV_H_
#define CAIF_DEV_H_

#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cfcnfg.h>
#include <linux/caif/caif_config.h>
#include <linux/if.h>

struct caif_service_config;
int caifdev_adapt_register(struct caif_channel_config *config,
			   struct layer *adap_layer);
int caifdev_adapt_unregister(struct layer *adap_layer);
struct cfcnfg *get_caif_conf(void);
void caif_register_ioctl(int (*ioctl)(unsigned int cmd,
				      unsigned long arg,
				      bool));
int caif_ioctl(unsigned int cmd, unsigned long arg, bool from_use_land);
void caif_unregister_netdev(void);
int channel_config_2_link_param(struct cfcnfg *cnfg,
				struct caif_channel_config *s,
				struct cfctrl_link_param *l);

#endif /* CAIF_DEV_H_ */
