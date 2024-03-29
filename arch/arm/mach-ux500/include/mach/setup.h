/*
 * Copyright (C) 2009 ST-Ericsson.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * These symbols are needed for board-specific files to call their
 * own cpu-specific files
 */
#ifndef __ASM_ARCH_SETUP_H
#define __ASM_ARCH_SETUP_H

#include <asm/mach/time.h>
#include <linux/init.h>

struct sys_timer;
struct amba_device;

extern void __init ux500_map_io(void);
extern void __init u5500_map_io(void);
extern void __init u8500_map_io(void);

extern void __init ux500_init_devices(void);
extern void __init u5500_init_devices(void);
extern void __init u8500_init_devices(void);
extern void __init u8500_init_regulators(void);

extern void __init ux500_init_irq(void);

extern void __init amba_add_devices(struct amba_device *devs[], int num);

extern struct sys_timer u8500_timer;
extern void __init db8500_prcm_timer_init(void);

#define __IO_DEV_DESC(x, sz)	{		\
	.virtual	= IO_ADDRESS(x),	\
	.pfn		= __phys_to_pfn(x),	\
	.length		= sz,			\
	.type		= MT_DEVICE,		\
}

#define __MEM_DEV_DESC(x, sz)	{		\
	.virtual	= IO_ADDRESS(x),	\
	.pfn		= __phys_to_pfn(x),	\
	.length		= sz,			\
	.type		= MT_MEMORY,		\
}

#endif /*  __ASM_ARCH_SETUP_H */
