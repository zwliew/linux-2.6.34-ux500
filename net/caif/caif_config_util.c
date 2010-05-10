/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author:	Sjur Brendeland sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/module.h>
#include <linux/spinlock.h>
#include <net/caif/cfctrl.h>
#include <net/caif/cfcnfg.h>
#include <net/caif/caif_dev.h>

//deprecated-functionality-below
#include <linux/caif/caif_config.h>
int
channel_config_2_link_param(struct cfcnfg *cnfg,
		struct caif_channel_config *s, struct cfctrl_link_param *l)
{
	struct dev_info *dev_info;
	enum cfcnfg_phy_preference pref;
	memset(l, 0, sizeof(*l));
	l->priority = s->priority;
	if (s->phy_name[0] != '\0') {
		l->phyid = cfcnfg_get_named(cnfg, s->phy_name);
	} else {
		switch (s->phy_pref) {
		case CAIF_PHYPREF_UNSPECIFIED:
			pref = CFPHYPREF_UNSPECIFIED;
			break;
		case CAIF_PHYPREF_LOW_LAT:
			pref = CFPHYPREF_LOW_LAT;
			break;
		case CAIF_PHYPREF_HIGH_BW:
			pref = CFPHYPREF_HIGH_BW;
			break;
		case CAIF_PHYPREF_LOOP:
			pref = CFPHYPREF_LOOP;
			break;
		default:
			return -ENODEV;
		}
		dev_info = cfcnfg_get_phyid(cnfg, pref);
		if (dev_info == NULL)
			return -ENODEV;
		l->phyid = dev_info->id;
	}
	switch (s->type) {
	case CAIF_CHTY_AT:
		l->linktype = CFCTRL_SRV_VEI;
		l->chtype = 0x02;
		l->endpoint = 0x00;
		break;
	case CAIF_CHTY_AT_CTRL:
		l->linktype = CFCTRL_SRV_VEI;
		l->chtype = 0x00;
		l->endpoint = 0x00;
		break;
	case CAIF_CHTY_AT_PAIRED:
		l->linktype = CFCTRL_SRV_VEI;
		l->chtype = 0x03;
		l->endpoint = 0x00;
		break;
	case CAIF_CHTY_VIDEO:
		l->linktype = CFCTRL_SRV_VIDEO;
		l->chtype = 0x00;
		l->endpoint = 0x00;
		l->u.video.connid = s->u.dgm.connection_id;
		break;
	case CAIF_CHTY_DATAGRAM:
		l->linktype = CFCTRL_SRV_DATAGRAM;
		l->chtype = 0x00;
		l->u.datagram.connid = s->u.dgm.connection_id;
		break;
	case CAIF_CHTY_DATAGRAM_LOOP:
		l->linktype = CFCTRL_SRV_DATAGRAM;
		l->chtype = 0x03;
		l->endpoint = 0x00;
		l->u.datagram.connid = s->u.dgm.connection_id;
		break;
	case CAIF_CHTY_DEBUG:
		l->linktype = CFCTRL_SRV_DBG;
		l->endpoint = 0x01;	/* ACC SIDE */
		l->chtype = 0x00;	/* Single channel with interactive
					 * debug and print-out mixed.
					 */
		break;
	case CAIF_CHTY_DEBUG_INTERACT:
		l->linktype = CFCTRL_SRV_DBG;
		l->endpoint = 0x01;	/* ACC SIDE */
		l->chtype = 0x02;	/* Interactive debug only */
		break;
	case CAIF_CHTY_DEBUG_TRACE:
		l->linktype = CFCTRL_SRV_DBG;
		l->endpoint = 0x01;	/* ACC SIDE */
		l->chtype = 0x01;	/* Debug print-out only */
		break;
	case CAIF_CHTY_RFM:
		l->linktype = CFCTRL_SRV_RFM;
		l->u.datagram.connid = s->u.rfm.connection_id;
		strncpy(l->u.rfm.volume, s->u.rfm.volume,
			sizeof(l->u.rfm.volume)-1);
		l->u.rfm.volume[sizeof(l->u.rfm.volume)-1] = 0;
		break;
	case CAIF_CHTY_UTILITY:
		l->linktype = CFCTRL_SRV_UTIL;
		l->endpoint = 0x00;
		l->chtype = 0x00;
		l->u.utility.fifosize_bufs = s->u.utility.fifosize_bufs;
		l->u.utility.fifosize_kb = s->u.utility.fifosize_kb;
		strncpy(l->u.utility.name, s->u.utility.name,
			sizeof(l->u.utility.name)-1);
		l->u.utility.name[sizeof(l->u.utility.name)-1] = 0;
		l->u.utility.paramlen = s->u.utility.paramlen;
		if (l->u.utility.paramlen > sizeof(l->u.utility.params))
			l->u.utility.paramlen = sizeof(l->u.utility.params);
		memcpy(l->u.utility.params, s->u.utility.params,
		       l->u.utility.paramlen);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL(channel_config_2_link_param);
//deprecated-functionality-above
int connect_req_to_link_param(struct cfcnfg *cnfg,
				struct caif_connect_request *s,
				struct cfctrl_link_param *l)
{
	struct dev_info *dev_info;
	enum cfcnfg_phy_preference pref;
	memset(l, 0, sizeof(*l));
	l->priority = s->priority;

	if (s->link_name[0] != '\0')
		l->phyid = cfcnfg_get_named(cnfg, s->link_name);
	else {
		switch (s->link_selector) {
		case CAIF_LINK_HIGH_BANDW:
			pref = CFPHYPREF_HIGH_BW;
			break;
		case CAIF_LINK_LOW_LATENCY:
			pref = CFPHYPREF_LOW_LAT;
			break;
		default:
			return -EINVAL;
		}
		dev_info = cfcnfg_get_phyid(cnfg, pref);
		if (dev_info == NULL)
			return -ENODEV;
		l->phyid = dev_info->id;
	}
	switch (s->protocol) {
	case CAIFPROTO_AT:
		l->linktype = CFCTRL_SRV_VEI;
		if (s->sockaddr.u.at.type == CAIF_ATTYPE_PLAIN)
			l->chtype = 0x02;
		else
			l->chtype = s->sockaddr.u.at.type;
		l->endpoint = 0x00;
		break;
	case CAIFPROTO_DATAGRAM:
		l->linktype = CFCTRL_SRV_DATAGRAM;
		l->chtype = 0x00;
		l->u.datagram.connid = s->sockaddr.u.dgm.connection_id;
		break;
	case CAIFPROTO_DATAGRAM_LOOP:
		l->linktype = CFCTRL_SRV_DATAGRAM;
		l->chtype = 0x03;
		l->endpoint = 0x00;
		l->u.datagram.connid = s->sockaddr.u.dgm.connection_id;
		break;
	case CAIFPROTO_RFM:
		l->linktype = CFCTRL_SRV_RFM;
		l->u.datagram.connid = s->sockaddr.u.rfm.connection_id;
		strncpy(l->u.rfm.volume, s->sockaddr.u.rfm.volume,
			sizeof(l->u.rfm.volume)-1);
		l->u.rfm.volume[sizeof(l->u.rfm.volume)-1] = 0;
		break;
	case CAIFPROTO_UTIL:
		l->linktype = CFCTRL_SRV_UTIL;
		l->endpoint = 0x00;
		l->chtype = 0x00;
		strncpy(l->u.utility.name, s->sockaddr.u.util.service,
			sizeof(l->u.utility.name)-1);
		l->u.utility.name[sizeof(l->u.utility.name)-1] = 0;
		caif_assert(sizeof(l->u.utility.name) > 10);
		l->u.utility.paramlen = s->param.size;
		if (l->u.utility.paramlen > sizeof(l->u.utility.params))
			l->u.utility.paramlen = sizeof(l->u.utility.params);

		memcpy(l->u.utility.params, s->param.data,
		       l->u.utility.paramlen);

		break;
//deprecated-functionality-below
	case CAIFPROTO_DEBUG:
		l->linktype = CFCTRL_SRV_DBG;
		l->endpoint = s->sockaddr.u.dbg.service;
		l->chtype = s->sockaddr.u.dbg.type;
		break;
//deprecated-functionality-above
	default:
		return -EINVAL;
	}
	return 0;
}
