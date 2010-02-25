/*
 * Copyright (C) ST-Ericsson AB 2009
 *
 * Author: Paul Wannbäck <paul.wannback@stericsson.com> for ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <asm/dma-mapping.h>
#include "b2r2_internal.h"

static void free_nodes(struct b2r2_node *first_node)
{
	struct b2r2_node *node = first_node;
	int no_of_nodes = 0;

	while (node) {
		no_of_nodes++;
		node = node->next;
	}

	dma_free_coherent(b2r2_blt_device(),
			  no_of_nodes * sizeof(struct b2r2_node),
			  first_node,
			  first_node->physical_address -
			  offsetof(struct b2r2_node, node));
}

struct b2r2_node * b2r2_blt_alloc_nodes(int no_of_nodes)
{
	u32 physical_address;
	struct b2r2_node *nodes;
	struct b2r2_node *tmpnode;

	if (no_of_nodes <= 0) {
		dev_err(b2r2_blt_device(), "%s: Wrong number of nodes (%d)",
			__func__, no_of_nodes);
		return NULL;
	}

	/* Allocate the memory */
	nodes = (struct b2r2_node *) dma_alloc_coherent(b2r2_blt_device(),
				      no_of_nodes * sizeof(struct b2r2_node),
				      &physical_address, GFP_DMA | GFP_KERNEL);

	if (nodes == NULL) {
		dev_err(b2r2_blt_device(),
			"%s: Failed to alloc memory for nodes",
			__func__);
		return NULL;
	}

	/* Build the linked list */
	tmpnode = nodes;
	physical_address += offsetof(struct b2r2_node, node);
	while (no_of_nodes--) {
		tmpnode->physical_address = physical_address;
		if (no_of_nodes)
			tmpnode->next = tmpnode + 1;
		else
			tmpnode->next = NULL;

		tmpnode++;
		physical_address += sizeof(struct b2r2_node);
	}

	return nodes;
}

void b2r2_blt_free_nodes(struct b2r2_node *first_node)
{
	free_nodes(first_node);
}

