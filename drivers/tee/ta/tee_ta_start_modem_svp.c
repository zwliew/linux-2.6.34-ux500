/*
 * Trusted application for starting the modem.
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Author: Shujuan Chen <shujuan.chen@stericsson.com>
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/elf.h>
#include <mach/hardware.h>

#include "tee_ta_start_modem.h"

static int reset_modem(unsigned long modem_start_addr)
{
	unsigned char *base = ioremap(ACCCON_BASE_SEC, 0x2FF);
	if (!base)
		return -ENOMEM;

	printk(KERN_INFO "[reset_modem] Setting modem start address!\n");
	writel(base + (ACCCON_CPUVEC_RESET_ADDR_OFFSET/sizeof(uint32_t)),
	       modem_start_addr);

	printk(KERN_INFO "[reset_modem] resetting the modem!\n");
	writel(base + (ACCCON_ACC_CPU_CTRL_OFFSET/sizeof(uint32_t)), 1);

	iounmap(base);

	return 0;
}

int tee_ta_start_modem(struct tee_ta_start_modem *data)
{
	int ret = 0;
	struct elfhdr *elfhdr;
	unsigned char *vaddr;

	vaddr = ioremap((unsigned long)data->access_image_descr.elf_hdr,
			sizeof(struct elfhdr));
	if (!vaddr)
		return -ENOMEM;

	elfhdr = (struct elfhdr *)readl(vaddr);
	printk(KERN_INFO "Reading in kernel:elfhdr 0x%x:elfhdr->entry=0x%x\n",
			(uint32_t)elfhdr, (uint32_t)elfhdr->e_entry);

	printk(KERN_INFO "[tee_ta_start_modem] reset modem()...\n");
	ret = reset_modem(elfhdr->e_entry);

	iounmap(vaddr);

	return ret;
}
