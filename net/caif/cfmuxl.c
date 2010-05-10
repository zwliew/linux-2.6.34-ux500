/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <net/caif/generic/cfglue.h>
#include <net/caif/generic/cfpkt.h>
#include <net/caif/generic/cflst.h>
#include <net/caif/generic/cfmuxl.h>
#include <net/caif/generic/cfsrvl.h>
#include <net/caif/generic/cffrml.h>

#define container_obj(layr) cfglu_container_of(layr, struct cfmuxl, layer)

#define CAIF_CTRL_CHANNEL 0
#define UP_CACHE_SIZE 8
#define DN_CACHE_SIZE 8

struct cfmuxl {
	struct layer layer;
	struct layer *up_cache[UP_CACHE_SIZE];
	struct layer *dn_cache[DN_CACHE_SIZE];
	/*
	 * Set when inserting or removing downwards layers.
	 */
	cfglu_lock_t transmit_lock;

	/*
	 * Set when inserting or removing upwards layers.
	 */
	cfglu_lock_t receive_lock;

};

static int cfmuxl_receive(struct layer *layr, struct cfpkt *pkt);
static int cfmuxl_transmit(struct layer *layr, struct cfpkt *pkt);
static void cfmuxl_ctrlcmd(struct layer *layr, enum caif_ctrlcmd ctrl,
				int phyid);
static struct layer *get_up(struct cfmuxl *muxl, int id);

struct layer *cfmuxl_create()
{
	struct cfmuxl *this = cfglu_alloc(sizeof(struct cfmuxl));
	if (!this) {
		pr_warning("CAIF: %s(): Out of memory\n", __func__);
		return NULL;
	}
	memset(this, 0, sizeof(*this));
	this->layer.receive = cfmuxl_receive;
	this->layer.transmit = cfmuxl_transmit;
	this->layer.ctrlcmd = cfmuxl_ctrlcmd;
	cfglu_init_lock(this->transmit_lock);
	cfglu_init_lock(this->receive_lock);
	snprintf(this->layer.name, CAIF_LAYER_NAME_SZ, "mux");
	return &this->layer;
}

int cfmuxl_set_uplayer(struct layer *layr, struct layer *up, uint8 linkid)
{
	int res;
	struct cfmuxl *muxl = container_obj(layr);
	cfglu_lock(muxl->receive_lock);
	res = cflst_put(&muxl->layer.up, linkid, up);
	cfglu_unlock(muxl->receive_lock);
	return res;
}

bool cfmuxl_is_phy_inuse(struct layer *layr, uint8 phyid)
{
	struct layer *p;
	struct cfmuxl *muxl = container_obj(layr);
	bool match = false;
	cfglu_lock(muxl->receive_lock);

	for (p = layr->up; p != NULL; p = p->next) {
		if (cfsrvl_phyid_match(p, phyid)) {
			match = true;
			break;
		}
	}
	cfglu_unlock(muxl->receive_lock);
	return match;
}

uint8 cfmuxl_get_phyid(struct layer *layr, uint8 channel_id)
{
	struct layer *up;
	int phyid;
	struct cfmuxl *muxl = container_obj(layr);
	cfglu_lock(muxl->receive_lock);
	up = get_up(muxl, channel_id);
	if (up != NULL)
		phyid = cfsrvl_getphyid(up);
	else
		phyid = 0;
	cfglu_unlock(muxl->receive_lock);
	return phyid;
}

int cfmuxl_set_dnlayer(struct layer *layr, struct layer *dn, uint8 phyid)
{
	int ret;
	struct cfmuxl *muxl = (struct cfmuxl *) layr;
	cfglu_lock(muxl->transmit_lock);
	ret = cflst_put(&muxl->layer.dn, phyid, dn);
	cfglu_unlock(muxl->transmit_lock);
	return ret;
}

struct layer *cfmuxl_remove_dnlayer(struct layer *layr, uint8 phyid)
{
	struct cfmuxl *muxl = container_obj(layr);
	struct layer *dn;
	cfglu_lock(muxl->transmit_lock);
	memset(muxl->dn_cache, 0, sizeof(muxl->dn_cache));
	dn = cflst_del(&muxl->layer.dn, phyid);
	caif_assert(dn != NULL);
	cfglu_unlock(muxl->transmit_lock);
	return dn;
}

/* Invariant: lock is taken */
static struct layer *get_up(struct cfmuxl *muxl, int id)
{
	struct layer *up;
	int idx = id % UP_CACHE_SIZE;
	up = muxl->up_cache[idx];
	if (up == NULL || up->id != id) {
		up = cflst_get(&muxl->layer.up, id);
		muxl->up_cache[idx] = up;
	}
	return up;
}

/* Invariant: lock is taken */
static struct layer *get_dn(struct cfmuxl *muxl, struct dev_info *dev_info)
{
	struct layer *dn;
	int idx = dev_info->id % DN_CACHE_SIZE;
	dn = muxl->dn_cache[idx];
	if (dn == NULL || dn->id != dev_info->id) {
		dn = cflst_get(&muxl->layer.dn, dev_info->id);
		muxl->dn_cache[idx] = dn;
	}
	return dn;
}

struct layer *cfmuxl_remove_uplayer(struct layer *layr, uint8 id)
{
	struct layer *up;
	struct cfmuxl *muxl = container_obj(layr);
	cfglu_lock(muxl->receive_lock);
	memset(muxl->up_cache, 0, sizeof(muxl->up_cache));
	up = cflst_del(&muxl->layer.up, id);
	cfglu_unlock(muxl->receive_lock);
	return up;
}

static int cfmuxl_receive(struct layer *layr, struct cfpkt *pkt)
{
	int ret;
	struct cfmuxl *muxl = container_obj(layr);
	uint8 id;
	struct layer *up;
	if (cfpkt_extr_head(pkt, &id, 1) < 0) {
		pr_err("CAIF: %s(): erroneous Caif Packet\n", __func__);
		cfpkt_destroy(pkt);
		return CFGLU_EPKT;
	}

	up = get_up(muxl, id);
	if (up == NULL) {
		pr_info("CAIF: %s():Received data on unknown link ID = %d "
			"(0x%x)	 up == NULL", __func__, id, id);
		cfpkt_destroy(pkt);
		/*
		 * Don't return ERROR, since modem misbehaves and sends out
		 * flow before linksetup response.
		 */
		return /* CFGLU_EPROT; */ CFGLU_EOK;
	}

	ret = up->receive(up, pkt);


	return ret;
}

static int cfmuxl_transmit(struct layer *layr, struct cfpkt *pkt)
{
	int ret;
	struct cfmuxl *muxl = container_obj(layr);
	uint8 linkid;
	struct layer *dn;
	struct payload_info *info = cfpkt_info(pkt);
	dn = get_dn(muxl, cfpkt_info(pkt)->dev_info);
	if (dn == NULL) {
		pr_warning("CAIF: %s(): Send data on unknown phy "
			   "ID = %d (0x%x)\n",
			   __func__, info->dev_info->id, info->dev_info->id);
		return CFGLU_ENOTCONN;
	}
	info->hdr_len += 1;
	linkid = info->channel_id;
	cfpkt_add_head(pkt, &linkid, 1);
	ret = dn->transmit(dn, pkt);
	if (ret < 0) {
		/* Remove MUX protocol header upon error. */
		cfpkt_extr_head(pkt, &linkid, 1);
	}

	return ret;
}

static void cfmuxl_ctrlcmd(struct layer *layr, enum caif_ctrlcmd ctrl,
				int phyid)
{
	struct layer *p;
	struct cfmuxl *muxl = container_obj(layr);
	for (p = muxl->layer.up; p != NULL; p = p->next) {
		if (cfsrvl_phyid_match(p, phyid))
			p->ctrlcmd(p, ctrl, phyid);
	}
}
