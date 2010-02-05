/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <net/caif/generic/cfglue.h>
#include <net/caif/generic/cfpkt.h>
#include <net/caif/generic/cflst.h>

void cflst_init(struct layer **lst)
{
	*lst = NULL;
}

struct layer *cflst_remove(struct layer **lst, struct layer *elem)
{
	struct layer *tmp;
	if (*lst == NULL)
		return NULL;

	tmp = (*lst);
	(*lst) = (*lst)->next;
	return tmp;
}

/* Adds an element from the queue. */
void cflst_insert(struct layer **lst, struct layer *node)
{
	struct layer *tmp;
	node->next = NULL;
	if ((*lst) == NULL) {
		(*lst) = node;
		return;
	}
	tmp = *lst;
	while (tmp->next != NULL)
		tmp = tmp->next;
	tmp->next = node;
}

int cflst_put(struct layer **lst, uint8 id, struct layer *node)
{
	if (cflst_get(lst, id) != NULL) {
		pr_err("CAIF: %s(): cflst_put duplicate key\n", __func__);
		return CFGLU_EINVAL;
	}
	node->id = id;
	cflst_insert(lst, node);
	return CFGLU_EOK;
}

struct layer *cflst_get(struct layer * *lst, uint8 id)
{
	struct layer *node;
	for (node = (*lst); node != NULL; node = node->next) {
		if (id == node->id)
			return node;
	}
	return NULL;
}

struct layer *cflst_del(struct layer * *lst, uint8 id)
{
	struct layer *iter;
	struct layer *node = NULL;

	if ((*lst) == NULL)
		return NULL;

	if ((*lst)->id == id) {
		node = (*lst);
		(*lst) = (*lst)->next;
		node->next = NULL;

		return node;
	}

	for (iter = (*lst); iter->next != NULL; iter = iter->next) {
		if (id == iter->next->id) {
			node = iter->next;
			iter->next = iter->next->next;
			node->next = NULL;
			return node;
		}
	}
	return NULL;
}
