/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef SHM_H_
#define SHM_H_

#include <linux/list.h>
/*#include <linux/init.h>
#include <linux/workqueue.h>*/
#include <net/caif/caif_layer.h>

#define ESUCCESS    0

typedef int (*mbxifc_cb_t) (u16 mbx_msg, void *priv);

/* FIXME:why is the client here??*/

/* Shared memory mailbox client structure. */
struct shm_mbxclient_t {
	mbxifc_cb_t cb;
	void *priv;
};

typedef int (*mbxifc_init_t) (struct shm_mbxclient_t *mbx_client, void *priv);

typedef int (*mbxifc_send_t) (u16 mbx_msg, void *priv);

/* Shared memory mailbox interface structure. */
struct shm_mbxifc_t {
	mbxifc_init_t init;
	mbxifc_send_t send_msg;
	char name[16];
	void *priv;
	struct shm_mbxclient_t *client;
	struct list_head list;
};

int mbxifc_register(struct shm_mbxifc_t *client);

int mbxifc_send(u16 mbx_msg, void *priv);
struct shm_mbxifc_t *mbxifc_get(unsigned char *name);
void mbxifc_put(struct shm_mbxifc_t *mbx_ifc);

/* TODO: This needs to go into a shared memory interface. */
static int shm_base_addr = 0x81F00000;
module_param(shm_base_addr, int, S_IRUGO);
MODULE_PARM_DESC(shm_base_addr,
		 "Physical base address of the shared memory area");

static int shm_tx_addr = 0x81F00000;
module_param(shm_tx_addr, int, S_IRUGO);
MODULE_PARM_DESC(shm_tx_addr, "Physical start address for transmission area");

static int shm_rx_addr = 0x81F0C000;
module_param(shm_rx_addr, int, S_IRUGO);
MODULE_PARM_DESC(shm_rx_addr, "Physical start address for reception area");

static int shm_nr_tx_buf = 6;
module_param(shm_nr_tx_buf, int, S_IRUGO);
MODULE_PARM_DESC(shm_nr_tx_buf, "number of transmit buffers");

static int shm_nr_rx_buf = 6;
module_param(shm_nr_rx_buf, int, S_IRUGO);
MODULE_PARM_DESC(shm_nr_rx_buf, "number of receive buffers");

static int shm_tx_buf_len = 0x2000;
module_param(shm_tx_buf_len, int, S_IRUGO);
MODULE_PARM_DESC(shm_tx_buf_len, "size of transmit buffers");

static int shm_rx_buf_len = 0x2000;
module_param(shm_rx_buf_len, int, S_IRUGO);
MODULE_PARM_DESC(shm_rx_buf_len, "size of receive buffers");

/* Shared memory interface structure. */
struct shm_cfgifc_t {
	int base_addr;
	int tx_addr;
	int rx_addr;
	int nr_tx_buf;
	int nr_rx_buf;
	int tx_buf_len;
	int rx_buf_len;
	char name[16];
	struct list_head list;
};

struct shm_cfgifc_t *cfgifc_get(unsigned char *name);

#endif				/* SHM_H_ */
