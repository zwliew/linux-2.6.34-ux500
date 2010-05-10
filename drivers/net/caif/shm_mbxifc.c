/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

/* Standard includes. */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/list.h>

#include <net/caif/caif_shm.h>

MODULE_LICENSE("GPL");

static struct list_head mbxifc_list;

void mbxifc_exit_module(void)
{
}

int mbxifc_init_module(void)
{
	INIT_LIST_HEAD(&mbxifc_list);

	return ESUCCESS;
}

int mbxifc_register(struct shm_mbxifc_t *mbxifc)
{
	if (!mbxifc)
		return -EINVAL;

	list_add_tail(&mbxifc->list, &mbxifc_list);
	printk(KERN_WARNING
	       "mbxifc_register: mailbox: %s at 0x%p added.\n",
	       mbxifc->name, mbxifc);

	return ESUCCESS;
}
EXPORT_SYMBOL(mbxifc_register);

int mbxifc_unregister(struct shm_mbxifc_t *mbx_ifc)
{
	struct shm_mbxifc_t *mbxifcreg;

	while (!(list_empty(&mbxifc_list))) {
		mbxifcreg = list_entry(mbxifc_list.next,
					struct shm_mbxifc_t, list);
		if (mbxifcreg == mbx_ifc) {
			list_del(&mbxifcreg->list);
			break;
		}
	}

	return ESUCCESS;
}
EXPORT_SYMBOL(mbxifc_unregister);

struct shm_mbxifc_t *mbxifc_get(unsigned char *name)
{
	struct shm_mbxifc_t *mbxifc;
	struct list_head *pos;
	list_for_each(pos, &mbxifc_list) {
		mbxifc = list_entry(pos, struct shm_mbxifc_t, list);
		if (strcmp(mbxifc->name, name) == 0) {
			list_del(&mbxifc->list);
			printk(KERN_WARNING
			       "mbxifc_get: mailbox: %s at 0x%p found.\n",
			       mbxifc->name, mbxifc);
			return mbxifc;
		}
	}

	printk(KERN_WARNING "mbxifc_get: no mailbox: %s found.\n", name);

	return NULL;
}
EXPORT_SYMBOL(mbxifc_get);

void mbxifc_put(struct shm_mbxifc_t *mbx_ifc)
{
	if (!mbx_ifc)
		return;

	list_add_tail(&mbx_ifc->list, &mbxifc_list);

	return;
}
EXPORT_SYMBOL(mbxifc_put);

module_init(mbxifc_init_module);
module_exit(mbxifc_exit_module);
