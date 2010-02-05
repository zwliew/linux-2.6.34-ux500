/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CFSRVL_H_
#define CFSRVL_H_
#include <net/caif/generic/cfglue.h>
#include <stddef.h>

struct cfsrvl {
	struct layer layer;
	bool open;
	bool phy_flow_on;
	bool modem_flow_on;
	struct dev_info dev_info;
};

struct layer *cfvei_create(uint8 linkid, struct dev_info *dev_info);
struct layer *cfdgml_create(uint8 linkid, struct dev_info *dev_info);
struct layer *cfutill_create(uint8 linkid, struct dev_info *dev_info);
struct layer *cfvidl_create(uint8 linkid, struct dev_info *dev_info);
struct layer *cfrfml_create(uint8 linkid, struct dev_info *dev_info);
bool cfsrvl_phyid_match(struct layer *layer, int phyid);
void cfservl_destroy(struct layer *layer);
void cfsrvl_init(struct cfsrvl *service,
		 uint8 channel_id,
		 struct dev_info *dev_info);
bool cfsrvl_ready(struct cfsrvl *service, int *err);
uint8 cfsrvl_getphyid(struct layer *layer);

#endif				/* CFSRVL_H_ */
