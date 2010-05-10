/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

/* Very simple simulated mailbox interface supporting
 * multiple mailbox instances.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>

#include <net/caif/caif_shm.h>

MODULE_LICENSE("GPL");

#define MBX_NR_OF_INSTANCES	2
#define MBX_SMBX_NAME		"cfmbx"

struct shm_smbx_t {
	struct shm_mbxifc_t local;
	struct shm_mbxifc_t *peer;
};

static struct shm_smbx_t shm_smbx[MBX_NR_OF_INSTANCES];

static int smbx_ifc_init(struct shm_mbxclient_t *mbx_client, void *priv)
{
	struct shm_smbx_t *mbx = (struct shm_smbx_t *) priv;
	mbx->local.client = mbx_client;
	return 0;
}

static int smbx_ifc_send_msg(u16 mbx_msg, void *priv)
{
	struct shm_smbx_t *mbx = (struct shm_smbx_t *) priv;
	mbx->peer->client->cb(mbx_msg, mbx->peer->client->priv);
	return 0;

}

static int __init shm_smbx_init(void)
{
	int i;

	for (i = 0; i < MBX_NR_OF_INSTANCES; i++) {
		/* Set up the mailbox interface. */
		shm_smbx[i].local.init = smbx_ifc_init;
		shm_smbx[i].local.send_msg = smbx_ifc_send_msg;
		sprintf(shm_smbx[i].local.name, "%s%d", MBX_SMBX_NAME, i);
		shm_smbx[i].local.priv = &shm_smbx[i];

		/* Set up the correct peer.
		 * (0 is connected to one and so forth).
		 */
		if (i % 2)
			shm_smbx[i].peer = &shm_smbx[i - 1].local;
		else
			shm_smbx[i].peer = &shm_smbx[i + 1].local;

		mbxifc_register(&(shm_smbx[i].local));
	}
	return 0;
}

static void shm_smbx_exit(void)
{
	/* Nothing to do. */
}

module_init(shm_smbx_init);
module_exit(shm_smbx_exit);
