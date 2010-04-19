/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 * Author: Sundar Iyer <sundar.iyer@stericsson.com>
 *
 * power management generic file
 */

#ifndef _PM_H_
#define _PM_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/memory.h>

#include <asm/system.h>

#include <mach/scu.h>
#include <mach/prcmu-regs.h>
#include <mach/prcmu-fw-api.h>
#include <mach/prcmu-fw-defs_v1.h>

#include "prcmu-fw_v1.h"

/* Peripheral Context Save/Restore */

/*
 * GPIO periph
 */
struct ux500_gpio_regs {
	uint32_t gpio_dat;
	uint32_t gpio_dat_set;
	uint32_t gpio_dat_clr;
	uint32_t gpio_pdis;
	uint32_t gpio_dir;
	uint32_t gpio_dir_set;
	uint32_t gpio_dir_clr;
	uint32_t gpio_slpm;
	uint32_t gpio_altfunc_a;
	uint32_t gpio_altfunc_b;
	uint32_t gpio_rimsc;
	uint32_t gpio_fsmsc;
	uint32_t gpio_int_status;
	uint32_t gpio_int_clr;
	uint32_t gpio_rwmsc;
	uint32_t gpio_fwmsc;
	uint32_t gpio_wakeup_status;
};

#define UX500_NR_GPIO_BANKS	(8)
#define GPIO_BANK_BASE(x) IO_ADDRESS(GPIO_BANK##x##_BASE)

/*
 * A9 sub system GIC context
 */
#define UX500_GIC_REG_SET	(100)

static uint32_t gic_enable_set_reg[UX500_GIC_REG_SET];
static uint32_t gic_dist_config[UX500_GIC_REG_SET];
static uint32_t gic_dist_target[UX500_GIC_REG_SET];

/*
 * Periph clock cluster context
 */
#define PRCC_BCK_EN		0x00
#define PRCC_KCK_EN		0x08
#define PRCC_BCK_STATUS		0x10
#define PRCC_KCK_STATUS		0x14

#define UX500_NR_PRCC_BANKS	(5)
#define PRCC_BANK_BASE(x) IO_ADDRESS(U8500_PER##x##_BASE)

struct ux500_prcc_contxt {
	uint32_t periph_bus_clk;
	uint32_t periph_kern_clk;
};

/*
 * ST-Interconnect context
 */
/* priority, bw limiter register offsets */
#define NODE_HIBW1_ESRAM_IN_0_PRIORITY_REG       0x00
#define NODE_HIBW1_ESRAM_IN_1_PRIORITY_REG       0x04
#define NODE_HIBW1_ESRAM_IN_2_PRIORITY_REG       0x08
#define NODE_HIBW1_ESRAM_IN_0_ARB_1_LIMIT_REG    0x24
#define NODE_HIBW1_ESRAM_IN_0_ARB_2_LIMIT_REG    0x28
#define NODE_HIBW1_ESRAM_IN_0_ARB_3_LIMIT_REG    0x2C
#define NODE_HIBW1_ESRAM_IN_1_ARB_1_LIMIT_REG    0x30
#define NODE_HIBW1_ESRAM_IN_1_ARB_2_LIMIT_REG    0x34
#define NODE_HIBW1_ESRAM_IN_1_ARB_3_LIMIT_REG    0x38
#define NODE_HIBW1_ESRAM_IN_2_ARB_1_LIMIT_REG    0x3C
#define NODE_HIBW1_ESRAM_IN_2_ARB_2_LIMIT_REG    0x40
#define NODE_HIBW1_ESRAM_IN_2_ARB_3_LIMIT_REG    0x44
#define NODE_HIBW1_DDR_IN_0_PRIORITY_REG         0x400
#define NODE_HIBW1_DDR_IN_1_PRIORITY_REG         0x404
#define NODE_HIBW1_DDR_IN_2_PRIORITY_REG         0x408
#define NODE_HIBW1_DDR_IN_0_LIMIT_REG            0x424
#define NODE_HIBW1_DDR_IN_1_LIMIT_REG            0x428
#define NODE_HIBW1_DDR_IN_2_LIMIT_REG            0x42C
#define NODE_HIBW1_DDR_OUT_0_PRIORITY_REG        0x430
#define NODE_HIBW2_ESRAM_IN_0_PRIORITY_REG       0x800
#define NODE_HIBW2_ESRAM_IN_1_PRIORITY_REG       0x804
#define NODE_HIBW2_ESRAM_IN_0_ARB_1_LIMIT_REG    0x818
#define NODE_HIBW2_ESRAM_IN_0_ARB_2_LIMIT_REG    0x81C
#define NODE_HIBW2_ESRAM_IN_0_ARB_3_LIMIT_REG    0x820
#define NODE_HIBW2_ESRAM_IN_1_ARB_1_LIMIT_REG    0x824
#define NODE_HIBW2_ESRAM_IN_1_ARB_2_LIMIT_REG    0x828
#define NODE_HIBW2_ESRAM_IN_1_ARB_3_LIMIT_REG    0x82C
#define NODE_HIBW2_DDR_IN_0_PRIORITY_REG         0xC00
#define NODE_HIBW2_DDR_IN_1_PRIORITY_REG         0xC04
#define NODE_HIBW2_DDR_IN_2_PRIORITY_REG         0xC08
#define NODE_HIBW2_DDR_IN_3_PRIORITY_REG         0xC0C
#define NODE_HIBW2_DDR_IN_0_LIMIT_REG            0xC30
#define NODE_HIBW2_DDR_IN_1_LIMIT_REG            0xC34
#define NODE_ESRAM1_2_IN_0_PRIORITY_REG          0x1400
#define NODE_ESRAM1_2_IN_1_PRIORITY_REG          0x1404
#define NODE_ESRAM1_2_IN_2_PRIORITY_REG          0x1408
#define NODE_ESRAM1_2_IN_3_PRIORITY_REG          0x140C
#define NODE_ESRAM1_2_IN_0_ARB_1_LIMIT_REG       0x1430
#define NODE_ESRAM1_2_IN_0_ARB_2_LIMIT_REG       0x1434
#define NODE_ESRAM1_2_IN_1_ARB_1_LIMIT_REG       0x1438
#define NODE_ESRAM1_2_IN_1_ARB_2_LIMIT_REG       0x143C
#define NODE_ESRAM1_2_IN_2_ARB_1_LIMIT_REG       0x1440
#define NODE_ESRAM1_2_IN_2_ARB_2_LIMIT_REG       0x1444
#define NODE_ESRAM1_2_IN_3_ARB_1_LIMIT_REG       0x1448
#define NODE_ESRAM1_2_IN_3_ARB_2_LIMIT_REG       0x144C
#define NODE_ESRAM3_4_IN_0_PRIORITY_REG          0x1800
#define NODE_ESRAM3_4_IN_1_PRIORITY_REG          0x1804
#define NODE_ESRAM3_4_IN_2_PRIORITY_REG          0x1808
#define NODE_ESRAM3_4_IN_3_PRIORITY_REG          0x180C
#define NODE_ESRAM3_4_IN_0_ARB_1_LIMIT_REG       0x1830
#define NODE_ESRAM3_4_IN_0_ARB_2_LIMIT_REG       0x1834
#define NODE_ESRAM3_4_IN_1_ARB_1_LIMIT_REG       0x1838
#define NODE_ESRAM3_4_IN_1_ARB_2_LIMIT_REG       0x183C
#define NODE_ESRAM3_4_IN_2_ARB_1_LIMIT_REG       0x1840
#define NODE_ESRAM3_4_IN_2_ARB_2_LIMIT_REG       0x1844
#define NODE_ESRAM3_4_IN_3_ARB_1_LIMIT_REG       0x1848
#define NODE_ESRAM3_4_IN_3_ARB_2_LIMIT_REG       0x184C

struct ux500_interconnect_contxt {
	uint32_t ux500_hibw1_esram_in_pri_regs[3];
	uint32_t ux500_hibw1_esram_in0_arb_regs[3];
	uint32_t ux500_hibw1_esram_in1_arb_regs[3];
	uint32_t ux500_hibw1_esram_in2_arb_regs[3];
	uint32_t ux500_hibw1_ddr_in_prio_regs[3];
	uint32_t ux500_hibw1_ddr_in_limit_regs[3];
	uint32_t ux500_hibw1_ddr_out_prio_reg;

	/* HiBw2 node registers */
	uint32_t ux500_hibw2_esram_in_pri_regs[2];
	uint32_t ux500_hibw2_esram_in0_arblimit_regs[3];
	uint32_t ux500_hibw2_esram_in1_arblimit_regs[3];
	uint32_t ux500_hibw2_ddr_in_prio_regs[4];
	uint32_t ux500_hibw2_ddr_in_limit_regs[4];
	uint32_t ux500_hibw2_ddr_out_prio_reg;

	/* ESRAM node registers */
	uint32_t ux500_esram_in_prio_regs[4];
	uint32_t ux500_esram_in_lim_regs[4];
	uint32_t ux500_esram12_in_prio_regs[4];
	uint32_t ux500_esram12_in_arb_lim_regs[8];
	uint32_t ux500_esram34_in_prio_regs[4];
	uint32_t ux500_esram34_in_arb_lim_regs[8];
};

/*
 * SCU context
 */
#define SCU_FILTER_STARTADDR	0x40
#define SCU_FILTER_ENDADDR	0x44
#define SCU_ACCESS_CTRL_SAC	0x50

struct ux500_scu_context {
	uint32_t scu_ctrl;
	uint32_t scu_cpu_pwrstatus;
	uint32_t scu_inv_all_nonsecure;
	uint32_t scu_filter_start_addr;
	uint32_t scu_filter_end_addr;
	uint32_t scu_access_ctrl_sac;
};

/* helpers to save/restore contexts */
void ux500_save_gic_context(void);
void ux500_restore_gic_context(void);

void ux500_save_uart_context(void);
void ux500_restore_uart_context(void);

void ux500_save_prcc_context(void);
void ux500_restore_prcc_context(void);

void ux500_save_icn_context(void);
void ux500_restore_icn_context(void);

void ux500_save_scu_context(void);
void ux500_restore_scu_context(void);

void ux500_cpu_context_deepsleep(uint8_t cpu);

#endif /* _PM_H */
