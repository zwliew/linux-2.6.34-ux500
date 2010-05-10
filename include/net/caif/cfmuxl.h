/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CFMUXL_H_
#define CFMUXL_H_
#include <net/caif/generic/caif_layer.h>

struct cfsrvl;
struct cffrml;

struct layer *cfmuxl_create(void);
int cfmuxl_set_uplayer(struct layer *layr, struct layer *up, uint8 linkid);
struct layer *cfmuxl_remove_dnlayer(struct layer *layr, uint8 phyid);
int cfmuxl_set_dnlayer(struct layer *layr, struct layer *up, uint8 phyid);
struct layer *cfmuxl_remove_uplayer(struct layer *layr, uint8 linkid);
bool cfmuxl_is_phy_inuse(struct layer *layr, uint8 phyid);
uint8 cfmuxl_get_phyid(struct layer *layr, uint8 channel_id);

#endif				/* CFMUXL_H_ */
