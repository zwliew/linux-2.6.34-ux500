/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CFSERL_H_
#define CFSERL_H_
#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cfglue.h>

struct layer *cfserl_create(int type, int instance, bool use_stx);

#endif				/* CFSERL_H_ */
