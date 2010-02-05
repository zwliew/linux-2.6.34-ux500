/*
 * Copyright (C) ST-Ericsson AB 2009
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

static struct list_head cfgifc_list;

void cfgifc_exit_module(void)
{
}

int cfgifc_init_module(void)
{
	return -ENODEV;
}

int cfgifc_register(struct shm_cfgifc_t *mbx_ifc)
{
	return 0;
}
EXPORT_SYMBOL(cfgifc_register);

struct shm_cfgifc_t *cfgifc_get(unsigned char *name)
{
	/* Hook up the adaptation layer. TODO: Make phy_type dynamic. */
	cfcnfg_add_adapt_layer(caifdev.cfg, linktype, connid,
			       CFPHYTYPE_SERIAL, adap_layer);

	return NULL;
}
EXPORT_SYMBOL(cfgifc_get);

void cfgifc_put(struct shm_cfgifc_t *mbx_ifc)
{
	/* Hook up the adaptation layer. TODO: Make phy_type dynamic. */
	cfcnfg_add_adapt_layer(caifdev.cfg, linktype, connid,
			       CFPHYTYPE_SERIAL, adap_layer);

	return 0;
}
EXPORT_SYMBOL(cfgifc_put);

module_init(cfgifc_init_module);
module_exit(cfgifc_exit_module);
