/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/hardirq.h>
#include <net/caif/cfpkt.h>
#include <net/caif/cflst.h>


void cflst_init(struct cflayer **lst)
{
	*lst = NULL;
}

struct cflayer *cflst_remove(struct cflayer **lst, struct cflayer *elem)
{
	struct cflayer *tmp;
	if (*lst == NULL)
		return NULL;

	tmp = (*lst);
	(*lst) = (*lst)->next;
	return tmp;
}

/* Adds an element from the queue. */
void cflst_insert(struct cflayer **lst, struct cflayer *node)
{
	struct cflayer *tmp;
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

int cflst_put(struct cflayer **lst, u8 id, struct cflayer *node)
{
	if (cflst_get(lst, id) != NULL) {
		pr_err("CAIF: %s(): cflst_put duplicate key\n", __func__);
		return -EINVAL;
	}
	node->id = id;
	cflst_insert(lst, node);
	return 0;
}

struct cflayer *cflst_get(struct cflayer * *lst, u8 id)
{
	struct cflayer *node;
	for (node = (*lst); node != NULL; node = node->next) {
		if (id == node->id)
			return node;
	}
	return NULL;
}

struct cflayer *cflst_del(struct cflayer * *lst, u8 id)
{
	struct cflayer *iter;
	struct cflayer *node = NULL;

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
