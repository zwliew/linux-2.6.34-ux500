/*
 * TEE service to handle the calls to trusted applications.
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Author: Joakim Bech <joakim.xx.bech@stericsson.com>
 * License terms: GNU General Public License (GPL) version 2
 */
#include <linux/kernel.h>
#include <linux/tee.h>

int __weak call_sec_world(struct tee_session *ts)
{
	printk(KERN_INFO "[%s] Generic call_sec_world called!\n", __func__);

	return 0;
}
