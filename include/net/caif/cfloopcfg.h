/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CFLOOPCFG_H_
#define CFLOOPCFG_H_
#include <net/caif/caif_layer.h>

struct cfloopcfg *cfloopcfg_create(void);
void cfloopcfg_add_phy_layer(struct cfloopcfg *cnfg,
			     enum cfcnfg_phy_type phy_type,
			     struct cflayer *phy_layer);
struct cflayer *cflooplayer_create(void);
#endif				/* CFLOOPCFG_H_ */
