/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/skbuff.h>
#include <net/caif/caif_kernel.h>
#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cfpkt.h>
#include <net/caif/generic/cfcnfg.h>
#include <net/caif/generic/cfglue.h>
#include <net/caif/caif_dev.h>
struct caif_kernelif {
	struct layer layer;
	struct caif_device *dev;
	struct cfctrl_link_param param;
};


/*
 * func caif_create_skb - Creates a CAIF SKB buffer
 * @data:		data to add to buffer
 * @data_length:	length of data
 */
struct sk_buff *caif_create_skb(unsigned char *data, unsigned int data_length)
{
	/* NOTE: Make room for CAIF headers when using SKB inside CAIF. */
	struct sk_buff *skb =
	    alloc_skb(data_length + CAIF_SKB_HEAD_RESERVE +
		      CAIF_SKB_TAIL_RESERVE, GFP_ATOMIC);
	if (skb == NULL)
		return NULL;
	skb_reserve(skb, CAIF_SKB_HEAD_RESERVE);

	memcpy(skb_put(skb, data_length), data, data_length);
	return skb;
}
EXPORT_SYMBOL(caif_create_skb);

int caif_extract_and_destroy_skb(struct sk_buff *skb, unsigned char *data,
			     unsigned int max_length)
{
	unsigned int len;
	len = skb->len;
	/*
	 * Note: skb_linearize only fails on an out of memory condition
	 * if we fail here we are NOT freeing the skb.
	 */
	if (!skb_linearize(skb) || skb->len > max_length)
		return CFGLU_EOVERFLOW;
	memcpy(data, skb->data, skb->len);
	kfree_skb(skb);
	return len;
}
EXPORT_SYMBOL(caif_extract_and_destroy_skb);

/*
 * NOTE: transmit takes ownership of the SKB.
 *       I.e. transmit only fails on severe errors.
 *       flow_off is not checked on transmit; this is client's responcibility.
 */
int caif_transmit(struct caif_device *dev, struct sk_buff *skb)
{
	struct caif_kernelif *chnlif =
	    (struct caif_kernelif *) dev->_caif_handle;
	struct cfpkt *pkt;
	pkt = cfpkt_fromnative(CAIF_DIR_OUT, (void *) skb);
	return chnlif->layer.dn->transmit(chnlif->layer.dn, pkt);
}
EXPORT_SYMBOL(caif_transmit);

int caif_flow_control(struct caif_device *dev, enum caif_flowctrl flow)
{
	enum caif_modemcmd modemcmd;
	struct caif_kernelif *chnlif =
	    (struct caif_kernelif *) dev->_caif_handle;
	switch (flow) {
	case CAIF_FLOWCTRL_ON:
		modemcmd = CAIF_MODEMCMD_FLOW_ON_REQ;
		break;
	case CAIF_FLOWCTRL_OFF:
		modemcmd = CAIF_MODEMCMD_FLOW_OFF_REQ;
		break;
	default:
		return -EINVAL;
	}
	return chnlif->layer.dn->modemcmd(chnlif->layer.dn, modemcmd);
}
EXPORT_SYMBOL(caif_flow_control);

static int chnlif_receive(struct layer *layr, struct cfpkt *cfpkt)
{
	struct caif_kernelif *chnl =
		container_of(layr, struct caif_kernelif, layer);
	struct sk_buff *skb;
	skb = (struct sk_buff *) cfpkt_tonative(cfpkt);
	chnl->dev->receive_cb(chnl->dev, skb);
	return CFGLU_EOK;
}

static void chnlif_flowctrl(struct layer *layr, enum caif_ctrlcmd ctrl,
			int phyid)
{
	struct caif_kernelif *chnl = (struct caif_kernelif *) layr;
	enum caif_control ctl;

	switch (ctrl) {
	case CAIF_CTRLCMD_FLOW_OFF_IND:
		ctl = CAIF_CONTROL_FLOW_OFF;
		break;
	case CAIF_CTRLCMD_FLOW_ON_IND:
		ctl = CAIF_CONTROL_FLOW_ON;
		break;
	case CAIF_CTRLCMD_REMOTE_SHUTDOWN_IND:
		ctl = CAIF_CONTROL_REMOTE_SHUTDOWN;
		break;
	case CAIF_CTRLCMD_DEINIT_RSP:
		ctl = CAIF_CONTROL_DEV_DEINIT;
		chnl->dev->_caif_handle = NULL;
		chnl->dev->control_cb(chnl->dev, ctl);
		memset(chnl, 0, sizeof(chnl));
		cfglu_free(chnl);
		return;

	case CAIF_CTRLCMD_INIT_RSP:
		ctl = CAIF_CONTROL_DEV_INIT;
		break;
	case CAIF_CTRLCMD_INIT_FAIL_RSP:
		ctl = CAIF_CONTROL_DEV_INIT_FAILED;
		break;
	default:
		return;
	}
	chnl->dev->control_cb(chnl->dev, ctl);
}

int caif_add_device(struct caif_device *dev)
{
	int ret;
	struct caif_kernelif *chnl = cfglu_alloc(sizeof(struct caif_kernelif));
	if (!chnl)
		return -ENOMEM;
	chnl->dev = dev;
	chnl->layer.ctrlcmd = chnlif_flowctrl;
	chnl->layer.receive = chnlif_receive;
	ret =
	    channel_config_2_link_param(get_caif_conf(), &dev->caif_config,
				&chnl->param);
	if (ret < 0) {
		ret = CFGLU_EBADPARAM;
		goto error;
	}
	if (cfcnfg_add_adaptation_layer(get_caif_conf(), &chnl->param,
				&chnl->layer)) {
		ret = CFGLU_ENOTCONN;
		goto error;
	}
	dev->_caif_handle = chnl;

	return CFGLU_EOK;
error:
	chnl->dev->_caif_handle = NULL;
	memset(chnl, 0, sizeof(chnl));
	cfglu_free(chnl);
	return ret;
}
EXPORT_SYMBOL(caif_add_device);

int caif_remove_device(struct caif_device *caif_dev)
{

	struct caif_kernelif *chnl =
	    container_of(caif_dev->_caif_handle, struct caif_kernelif, layer);
	return cfcnfg_del_adapt_layer(get_caif_conf(), &chnl->layer);
}
EXPORT_SYMBOL(caif_remove_device);
