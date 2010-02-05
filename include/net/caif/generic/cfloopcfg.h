/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CFLOOPCFG_H_
#define CFLOOPCFG_H_
#include <net/caif/generic/caif_layer.h>

struct cfloopcfg *cfloopcfg_create(void);
void cfloopcfg_add_phy_layer(struct cfloopcfg *cnfg,
			     enum cfcnfg_phy_type phy_type,
			     struct layer *phy_layer);
struct layer *cflooplayer_create(void);
#endif				/* CFLOOPCFG_H_ */
