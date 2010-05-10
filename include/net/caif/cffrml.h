/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CFFRML_H_
#define CFFRML_H_
#include <net/caif/generic/cfglue.h>
#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cflst.h>

struct cffrml;
struct layer *cffrml_create(uint16 phyid, bool DoFCS);
void cffrml_set_uplayer(struct layer *this, struct layer *up);
void cffrml_set_dnlayer(struct layer *this, struct layer *dn);
void cffrml_destroy(struct layer *layer);

#endif /* CFFRML_H_ */
