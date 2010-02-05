/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CFLST_H_
#define CFLST_H_

#include <net/caif/generic/cfglue.h>

int cflst_put(struct layer **lst, uint8 id, struct layer *node);
struct layer *cflst_get(struct layer **lst, uint8 id);
struct layer *cflst_del(struct layer **lst, uint8 id);
#define CFLST_FIRST(lst) lst
#define CFLST_MORE(node) ((node) != NULL)
#define CFLST_NEXT(node) ((node)->next)
void cflst_init(struct layer **lst);
#endif	/* CFLST_H_ */
