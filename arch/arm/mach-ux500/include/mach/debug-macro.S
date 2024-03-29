/*
 * Debugging macro include header
 *
 *  Copyright (C) 2009 ST-Ericsson SA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <mach/hardware.h>

#ifdef CONFIG_MACH_U5500_SIMULATOR
#define UX500_DEBUG_UART	UX500_UART0_BASE
#else
#define UX500_DEBUG_UART	UX500_UART2_BASE
#endif

	.macro	addruart, rx, tmp
	mrc	p15, 0, \rx, c1, c0
	tst	\rx, #1			@MMU enabled?
	ldreq	\rx, =UX500_DEBUG_UART			@ no, physical address
	ldrne	\rx, =IO_ADDRESS(UX500_DEBUG_UART)	@ yes, virtual address
	.endm

#include <asm/hardware/debug-pl01x.S>
