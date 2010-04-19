/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 * Author: Sundar Iyer <sundar.iyer@stericsson.com>
 * Author: David Paris <david.paris-nonst@stericsson.com>
 *
 * ux500 core context/save for low power modes
 */

#include <linux/linkage.h>
#include <linux/init.h>
#include <mach/hardware.h>
#include <asm/hardware/cache-l2x0.h>


/* Define for DeepSleep specific case */
#define _BACKUPRAM1_BASE 		IO_ADDRESS(U8500_BACKUPRAM1_BASE)

#define BACKUPRAMCPU0	(_BACKUPRAM1_BASE + 0xF80)
#define BACKUPRAMCPU1	(_BACKUPRAM1_BASE + 0xFA0)

#define UX500L2CCBASE	IO_ADDRESS(UX500_L2CC_BASE)

ENTRY(ux500_cpu_context_deepsleep)
	.code 32
	.extern prcmu_apply_ap_state_transition

	push {r0-r4, lr}

	mov r2, r0
	cmp r0, #0
	bne load_cpu1_ctxt

	ldr r0, =BACKUPRAMCPU0
	b common_ctxt

load_cpu1_ctxt:
	ldr r0, =BACKUPRAMCPU1

common_ctxt:
	/* Save SystemControl Register */
	mrc p15, 0, r1, c1, c0, 0
	str r1,[r0,#0x00]

	/* Save TTBR0 */
	mrc p15, 0, r1, c2, c0, 0
	str r1,[r0,#0x04]

	/* Save TTBR1 */
	mrc p15, 0, r1, c2, c0, 1
	str	r1,[r0,#0x08]

	/* save TTBRC */
	mrc p15, 0, r1, c2, c0, 2
	str r1,[r0,#0x0C]

	/* Save DACR */
	mrc p15, 0, r1, c3, c0, 0
	str r1,[r0,#0x10]

	cmp r2, #0
	bne store_ret_addr_for_cpu1

	ldr r1, =_return_from_deepsleep_cpu0
	str r1,[r0,#0x14]
	b common_continue

store_ret_addr_for_cpu1:
	ldr r1, =_return_from_deepsleep_cpu1
	str r1,[r0,#0x14]

common_continue:
	/* save CPSR */
	mrs r1, cpsr
	str r1,[r0,#0x18]

	/* save Current stack (Must not be used after) */
	str sp,[r0,#0x1C]

	/* Save ARM Context */
	mov r0, r2
	bl save_processor_context

	cmp r2, #0
	bne _cpu1_wait_deepsleep

	stmfd sp!, {r0, r1, r2}
	bl ux500_flush_all
	ldmfd sp!, {r0, r1, r2}

	mov r0, #0x4	/* APEXECUTE_TO_APDEEPSLEEP */
	mov r1, #0x0	/* DDR_PWR_STATE_UNCHANGED */
	mov r2, #0x0	/* INTR_NOT_AS_WAKEUP */

	bl prcmu_apply_ap_state_transition

	b _return_from_deepsleep_cpu0

_cpu1_wait_deepsleep:
	dsb
	wfi
	b _return_from_deepsleep_cpu1

_return_from_deepsleep_cpu0:
	mov r0, #0

	b _exit_deeps

_return_from_deepsleep_cpu1:
	mov r0, #1

_exit_deeps:
	bl restore_processor_context

	pop {r0-r4, pc}
ENDPROC(ux500_cpu_context_deepsleep)

ENTRY(save_processor_context)
	.extern arm_cntxt_cpu0
	.extern arm_cntxt_cpu1

	.code 32

	push {r0-r3, lr}

	/* load the backup pointer accordingly */
	cmp r0, #0
	bne save_cpu1_ctxt
	ldr r1, =arm_cntxt_cpu0
	b save_ctxt

save_cpu1_ctxt:
	ldr r1, =arm_cntxt_cpu1

save_ctxt:
	ldr r1, [r1]

	mrs r2, cpsr
	str r2, [r1], #+4

	/* clear up all mode bits which change with the mode. */
	bic r2, r2, #0xf

	/*
	 * enter all modes and save banked registers
	 * suffix to CPSR : c-control, f-flags, s-status, x-xtension
	 */

	/*
	 * Enter FIQ mode, Interrupts disabled
	 * Save: r8-r14 and spsr
	 * Assume: r2 depicts the processor in Supervisor mode
	 */
	orr r3, r2, #0x1
	msr cpsr_cxsf, r3
	mrs r3, spsr
	stmia r1!, {r3,r8-r14}

	/*
	 * Enter IRQ mode, Interrupts disabled
	 * Save: r13,r14 and spsr
	 */
	orr r3, r2, #0x2
	msr cpsr_cxsf, r3 /* Enter IRQ mode with IRQ/FIQ disable */
	mrs r3, spsr
	stmia r1!, {r3,r13,r14}

	/*
	 * Enter Abort mode, irq/fiq disabled
	 * Save: r13,r14 and sps
	 */
	orr r3, r2, #0x7
	msr cpsr_cxsf, r3
	mrs r3, spsr
	stmia r1!, {r3, r4-r14}

	/*
	 * Enter Undef mode, irq/fiq disable
	 * Save: r13,r14 and spsr
	 */
	orr r3, r2, #0xB
	msr cpsr_cxsf, r3
	mrs r3, spsr
	stmia r1!, {r3,r13,r14}

	/* go back to the SVC mode now */
	orr r3, r2, #0x3
	msr cpsr_cxsf, r3

	/* in SVC mode, save all CP15 configs */

	/* Non-secure Vector Base Address Register */
	mrc p15, 0, r2, c12, c0, 0
	str r2, [r1], #4

	/* Primary Region Remap reg */
	mrc p15, 0, r2, c10, c2, 0
	str r2, [r1], #4

	mrc p15, 0, r2, c10, c2, 1
	str r2, [r1], #4

	/* Context ID reg */
	mrc p15, 0, r2, c13, c0, 1
	str r2, [r1], #4

	/* Thread ID  registers */
	mrc p15, 0, r2, c13, c0, 2
	str r2, [r1], #4

	mrc p15, 0, r2, c13, c0, 3
	str r2, [r1], #4

	mrc p15, 0, r2, c13, c0, 4
	str r2, [r1], #4

	/* Cache Size Selection Register */
	mrc p15, 2, r2, c0, c0,  0
	str r2, [r1], #4

	/* PMNC */
	mrc p15, 0, r2, c9, c12, 0
	str r2, [r1], #4

	/* PMCNTENSET register */
	mrc p15, 0, r2, c9, c12, 1
	str r2, [r1], #4

	/* PMSELR register */
	mrc p15, 0, r2, c9, c12, 5
	str r2, [r1], #4

	/* PMCCNTR register */
	mrc p15, 0, r2, c9, c13, 0
	str r2, [r1], #4

	/* PMXEVTYPER register */
	mrc p15, 0, r2, c9, c13, 1
	str r2, [r1], #4

	/* PMUSERENR register */
	mrc p15, 0, r2, c9, c14, 0
	str r2, [r1], #4

	/* PMINTENSET register */
	mrc p15, 0, r2, c9, c14, 1
	str r2, [r1], #4

	/* PMINTENCLR register */
	mrc p15, 0, r2, c9, c14, 2
	str r2, [r1], #4

	/* CPACR register */
	mrc p15, 0, r2, c1, c0, 2
	str r2, [r1], #4

	cmp r0, #0
	bne backup_ptr_cpu1

	ldr r0, =arm_cntxt_cpu0
	b update_backup_ptr

backup_ptr_cpu1:
	ldr r0, =arm_cntxt_cpu1

update_backup_ptr:
	str r1, [r0]

	pop {r0-r3,pc}
ENDPROC(save_processor_context)


ENTRY (restore_processor_context)
	.code 32
	.extern arm_cntxt_cpu0
	.extern arm_cntxt_cpu1

	push {r0-r3, lr}

	cmp r0, #0
	bne restore_cpu1_cntxt
	ldr r1, =arm_cntxt_cpu0
	b restore_cntxt

restore_cpu1_cntxt:
	ldr r1, =arm_cntxt_cpu1

restore_cntxt:
	ldr r1, [r1]

	/* restore the CP15 configs */
	sub r1, r1, #4

	/* CPACR register */
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c1, c0, 2

	/* PMINTENCLR register */
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c9, c14, 2

	/* PMINTENSET register */
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c9, c14, 1

	/* PMUSERENR register */
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c9, c14, 0

	/* PMXEVTYPER register */
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c9, c13, 1

	/* PMCCNTR register */
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c9, c13, 0

	/* PMSELR register */
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c9, c12, 5

	/* PMCNTENSET register */
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c9, c12, 1

	/* PMNC register */
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c9, c12, 0

	/* Cache Size Selection register */
	ldr r2, [r1], #-4
	mcr p15, 2, r2, c0, c0,  0

	/* Thread IDs registers */
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c13, c0, 4
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c13, c0, 3
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c13, c0, 2

	/* Context ID register */
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c13, c0, 1

	/* memory region map registers */
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c10, c2, 1
	ldr r2, [r1], #-4
	mcr p15, 0, r2, c10, c2, 0

	/* Non-secure Vector Base Address register */
	ldr r2, [r1]
	mcr p15, 0, r2, c12, c0, 0

	/* backup the value of cpsr */
	mrs r2, cpsr
	orr r2, r2, #0xC0
	bic r2, r2, #0xf

        /*
         * Enter Undef mode, irq/fiq disabled
         * restore: r13,r14 and spsr
         */
	orr r3, r2, #0xB
	msr cpsr_cxsf, r3
	ldmdb r1!, {r3,r13,r14}
	msr spsr_cxsf, r3

        /*
         * Enter Abort mode, irq/fiq disabled
         * restore: r13,r14 and spsr
         */
	orr r3, r2, #0x7
	msr cpsr_cxsf, r3
	ldmdb r1!, {r3, r4-r14}
	msr spsr_cxsf, r3

        /*
         * Enter IRQ mode, irq disabled
         * restore: r13,r14 and spsr
         */
	orr r3, r2, #0x2
	msr cpsr_cxsf, r3
	ldmdb r1!, {r3,r13,r14}
	msr spsr_cxsf, r3

        /*
         * Enter FIQ mode, irq disabled
         * restore: r13,r14 and spsr
         */
	orr r3, r2, #0x1
	msr cpsr_cxsf, r3
	ldmdb r1!, {r3,r8-r14}
	msr spsr_cxsf, r3

	/*
         * Enter SVC mode, irq disabled
         * restore: cpsr
         */
	orr r3, r2, #0x3
	msr cpsr_cxsf, r3

	ldr r2, [r1, #-4]!
	msr cpsr, r2
	msr cpsr_cxsf, r3

	cmp r0, #0
	bne backup_ptr_restore_cpu1
	ldr r0, =arm_cntxt_cpu0
	b update_backup_restore_ptr

backup_ptr_restore_cpu1:
	ldr r0, =arm_cntxt_cpu1

update_backup_restore_ptr:
	sub r1, r1, #4
	str r1, [r0]

	pop {r0-r3, pc}
ENDPROC(restore_processor_context)

ENTRY(ux500_flush_all)
	.code 32

	/*
	 *  Flush the entire cache system.
	 *  The data cache flush is now achieved using atomic clean/invalidates
	 *  working outwards from L1 cache. This is done using Set/Way based
	 *  cache maintainance instructions.
	 *  The instruction cache can still be invalidated back to the point of
	 *  unification in a single instruction.
	 *
	 */
	stmfd   sp!, {r4-r5, r7, r9-r11, lr}
	mrc     p15, 1, r0, c0, c0, 1           @ read clidr
        ands    r3, r0, #0x7000000              @ extract loc from clidr
        mov     r3, r3, lsr #23                 @ left align loc bit field
        beq     finished                        @ if loc is 0, no need to clean
        mov     r10, #0                         @ start clean at cache level 0
loop1:
        add     r2, r10, r10, lsr #1            @ work 3x current cache level
        mov     r1, r0, lsr r2                  @ extract cache type from clidr
        and     r1, r1, #7                      @ mask bits for current cache
        cmp     r1, #2                          @ see cache at this level
        blt     skip                            @ skip if no cache, or i-cache
        mcr     p15, 2, r10, c0, c0, 0          @ select current cache level
        isb                                     @ isb to sych new cssr&csidr
        mrc     p15, 1, r1, c0, c0, 0           @ read the new csidr
        and     r2, r1, #7                      @ extract length of cache lines
        add     r2, r2, #4                      @ add 4 (line length offset)
        ldr     r4, =0x3ff
        ands    r4, r4, r1, lsr #3              @ find maximum on the way size
        clz     r5, r4                          @ bit position of way size inc
        ldr     r7, =0x7fff
        ands    r7, r7, r1, lsr #13             @ max number of the index size
loop2:
        mov     r9, r4                          @ create copy of max way size
loop3:
	orr     r11, r10, r9, lsl r5		@ way and cache number into r11
	orr     r11, r11, r7, lsl r2		@ way and cache number into r11
        mcr     p15, 0, r11, c7, c14, 2         @ clean & invalidate by set/way
        subs    r9, r9, #1                      @ decrement the way
        bge     loop3
        subs    r7, r7, #1                      @ decrement the index
        bge     loop2
skip:
        add     r10, r10, #2                    @ increment cache number
        cmp     r3, r10
        bgt     loop1
finished:
        mov     r10, #0                         @ swith back to cache level 0
        mcr     p15, 2, r10, c0, c0, 0          @ current cache level in cssr
        dsb
        isb

	/* L2 cache cleaning */

	ldr r0, =UX500L2CCBASE
	ldr r2, [r0, #L2X0_AUX_CTRL]

	ldr r2, =0xff
	str r2, [r0, #L2X0_CLEAN_WAY]
	2:
	ldr r3, [r0, #L2X0_CLEAN_WAY]
	cmp r3, #0
	bne 2b

	ldr r2, =0xff
	str r2, [r0, #L2X0_INV_WAY]
	1:
	ldr r3, [r0, #L2X0_INV_WAY]
	cmp r3, #0
	bne 1b

	mcr p15, 0, r0, c7, c5, 6		@ flush BTAC/BTB
	isb

	ldmfd sp!, {r4-r5,r7,r9-r11, pc}

ENDPROC(ux500_flush_all)
