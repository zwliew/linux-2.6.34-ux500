/*
 * Copyright (C) STMicroelectronics 2009
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 * Author: Sundar Iyer <sundar.iyer@stericsson.com>
 *
 */

#include "pm.h"

#define U8500_EXT_BACKUPRAM_ADDR	(0x80151FDC)
#define U8500_CPU0_PUBLIC_BACKUP	(0x80151FD8)
#define U8500_CPU1_PUBLIC_BACKUP	(0x80151FE0)
#define U8500_BACKUPRAM_SIZE		(64 * 1024)

#define ENABLE	(1)
#define DISABLE	(0)

/* pointers for backups/contexts to be saved */
dma_addr_t *arm_cntxt_cpu0, *arm_cntxt_cpu1, *ux500_backup_ptr;

static int u8500_pm_begin(suspend_state_t state)
{
	return 0;
}

static int u8500_pm_prepare(void)
{
	return 0;
}

/* poweroff support */
#include <linux/signal.h>
#include <mach/ab8500.h>
/*
 * This function is used from pm.h to shut down the u8500 system by
 * turning off the ab8500
 */
void u8500_pm_poweroff(void)
{
	sigset_t old, all;
	int val;

	sigfillset(&all);
	if (!sigprocmask(SIG_BLOCK, &all, &old)) {
		val = ab8500_read(AB8500_SYS_CTRL1_BLOCK, AB8500_CTRL1_REG);
		val |= 0x1;
		ab8500_write(AB8500_SYS_CTRL1_BLOCK, AB8500_CTRL1_REG, val);
		(void) sigprocmask(SIG_SETMASK, &old, NULL);
	}
	return;
}


/* GPIO context */
static struct ux500_gpio_regs gpio_context[UX500_NR_GPIO_BANKS];

/* gpio bank addrs */
uint32_t gpio_banks[] = {
	GPIO_BANK_BASE(0),
	GPIO_BANK_BASE(1),
	GPIO_BANK_BASE(2),
	GPIO_BANK_BASE(3),
	GPIO_BANK_BASE(4),
	GPIO_BANK_BASE(5),
	GPIO_BANK_BASE(6),
	GPIO_BANK_BASE(7),
	GPIO_BANK_BASE(8),
};

/**
 * ux500_save_gpio_context() - save GPIO periph context
 *
 */
static void ux500_save_gpio_context(void)
{
	uint32_t i, base;

	for (i = 0; i < UX500_NR_GPIO_BANKS; i++) {

		base = gpio_banks[i];

		gpio_context[i].gpio_dat = readl(base + NMK_GPIO_DAT);
		gpio_context[i].gpio_dat_set = readl(base + NMK_GPIO_DATS);
		gpio_context[i].gpio_dat_clr = readl(base + NMK_GPIO_DATC);
		gpio_context[i].gpio_pdis = readl(base + NMK_GPIO_PDIS);
		gpio_context[i].gpio_dir = readl(base + NMK_GPIO_DIR);
		gpio_context[i].gpio_dir_set = readl(base + NMK_GPIO_DIRS);
		gpio_context[i].gpio_dir_clr = readl(base + NMK_GPIO_DIRC);
		gpio_context[i].gpio_slpm = readl(base + NMK_GPIO_SLPC);
		gpio_context[i].gpio_altfunc_a = readl(base + NMK_GPIO_AFSLA);
		gpio_context[i].gpio_altfunc_b = readl(base + NMK_GPIO_AFSLB);
		gpio_context[i].gpio_rimsc = readl(base + NMK_GPIO_RIMSC);
		gpio_context[i].gpio_fsmsc = readl(base + NMK_GPIO_FWIMSC);
		gpio_context[i].gpio_int_status = readl(base + NMK_GPIO_IS);
		gpio_context[i].gpio_int_clr = readl(base + NMK_GPIO_IC);
		gpio_context[i].gpio_rwmsc = readl(base + NMK_GPIO_RWIMSC);
		gpio_context[i].gpio_fwmsc = readl(base + NMK_GPIO_FWIMSC);
		gpio_context[i].gpio_wakeup_status = readl(base + NMK_GPIO_WKS);
	}

	return;
}

/**
 * ux500_restore_gpio_context() - restore GPIO periph context
 *
 */
static void ux500_restore_gpio_context(void)
{
	uint32_t i, base;

	for (i = 0; i < UX500_NR_GPIO_BANKS; i++) {

		base = gpio_banks[i];

		writel(gpio_context[i].gpio_dat, base + NMK_GPIO_DAT);
		writel(gpio_context[i].gpio_dat_set, base + NMK_GPIO_DATS);
		writel(gpio_context[i].gpio_dat_clr, base + NMK_GPIO_DATC);
		writel(gpio_context[i].gpio_pdis, base + NMK_GPIO_PDIS);
		writel(gpio_context[i].gpio_dir, base + NMK_GPIO_DIR);
		writel(gpio_context[i].gpio_dir_set, base + NMK_GPIO_DIRS);
		writel(gpio_context[i].gpio_dir_clr, base + NMK_GPIO_DIRC);
		writel(gpio_context[i].gpio_slpm, base + NMK_GPIO_SLPC);
		writel(gpio_context[i].gpio_altfunc_a, base + NMK_GPIO_AFSLA);
		writel(gpio_context[i].gpio_altfunc_b, base + NMK_GPIO_AFSLB);
		writel(gpio_context[i].gpio_rimsc, base + NMK_GPIO_RIMSC);
		writel(gpio_context[i].gpio_fsmsc, base + NMK_GPIO_FWIMSC);
		writel(gpio_context[i].gpio_int_status, base + NMK_GPIO_IS);
		writel(gpio_context[i].gpio_int_clr, base + NMK_GPIO_IC);
		writel(gpio_context[i].gpio_rwmsc, base + NMK_GPIO_RWIMSC);
		writel(gpio_context[i].gpio_fwmsc, base + NMK_GPIO_FWIMSC);
		writel(gpio_context[i].gpio_wakeup_status, base + NMK_GPIO_WKS);
	}

	return;
}

/* GIC context */
/* local copy of max irqs */
static uint32_t ux500_max_irq;

/**
 * ux500_disable_gic_interrupts() - disable all GIC interrupts
 *
 */
static inline void ux500_disable_gic_interrupts(void)
{
	uint32_t base = IO_ADDRESS(U8500_GIC_DIST_BASE);

	/* disable all interrupts in the gic */
	writel(0xffffffff, base + GIC_DIST_ENABLE_CLEAR + 0x4);
	writel(0xffffffff, base + GIC_DIST_ENABLE_CLEAR + 0x8);
	writel(0xffffffff, base + GIC_DIST_ENABLE_CLEAR + 0xc);
	writel(0xffffffff, base + GIC_DIST_ENABLE_CLEAR + 0x10);

	return;
}

/**
 * ux500_save_gic_context() - save GIC context
 *
 * this function saves the gic context and also disables
 * all gic interrupts further.
 *
 */
void ux500_save_gic_context(void)
{
	uint32_t i, j;
	uint32_t base = IO_ADDRESS(U8500_GIC_DIST_BASE);

	for (i = 32, j = 0; i < ux500_max_irq; i += 16)
		gic_dist_config[j++] =
			readl(base + GIC_DIST_CONFIG + (i * 4)/16);

	for (i = 32, j = 0; i < ux500_max_irq; i += 4)
		gic_dist_target[j++] =
			readl(base + GIC_DIST_TARGET + (i * 4)/4);

	for (i = 0, j = 0; i < ux500_max_irq; i += 32)
		gic_enable_set_reg[j++] =
			readl(base + GIC_DIST_ENABLE_SET + (i * 4)/32);

	ux500_disable_gic_interrupts();

	return;
}

/**
 * ux500_restore_gic_context() - restore GIC context
 *
 */
void ux500_restore_gic_context(void)
{
	uint32_t i, j;
	uint32_t base = IO_ADDRESS(U8500_GIC_DIST_BASE);

	writel(0, base + GIC_DIST_CTRL);

	/* read back the register values */
	for (i = 32, j = 0; i < ux500_max_irq; i += 16)
		writel(gic_dist_config[j++],
				base + GIC_DIST_CONFIG + (i * 4)/16);

	for (i = 32, j = 0; i < ux500_max_irq; i += 4)
		writel(gic_dist_target[j++],
				base + GIC_DIST_TARGET + (i * 4)/4);

	for (i = 0, j = 0; i < ux500_max_irq; i += 32)
		writel(gic_enable_set_reg[j++],
				base + GIC_DIST_ENABLE_SET + (i * 4)/32);

	/*
	 * enables the GIC controller after we restore the
	 * interrupt configuration
	 */
	writel(1, base + GIC_DIST_CTRL);

	return;
}

/* PRCC context */
static struct ux500_prcc_contxt prcc_contxt[UX500_NR_PRCC_BANKS];
#define PERIPH_PRCC_BASE_OFFSET		(0xf000)
#define PERIPH5_PRCC_BASE_OFFSET	(0x1f000)

/* prcc bank addrs */
uint32_t prcc_banks[] = {
	PRCC_BANK_BASE(1),
	PRCC_BANK_BASE(2),
	PRCC_BANK_BASE(3),
	PRCC_BANK_BASE(5),
	PRCC_BANK_BASE(6),
};

/**
 * ux500_save_prcc_context() - save periph clock context
 *
 */
void ux500_save_prcc_context(void)
{
	uint32_t i, base;

	for (i = 0; i < UX500_NR_PRCC_BANKS; i++) {
		base = prcc_banks[i];
		/* periph 5 has a different layout */
		if (base == PRCC_BANK_BASE(5))
			base = prcc_banks[i] + PERIPH5_PRCC_BASE_OFFSET;
		else
			base = prcc_banks[i] + PERIPH_PRCC_BASE_OFFSET;

		prcc_contxt[i].periph_bus_clk = readl(base + PRCC_BCK_STATUS);
		prcc_contxt[i].periph_kern_clk = readl(base + PRCC_KCK_STATUS);
	}
	return;
}

/**
 * ux500_restore_prcc_context() - restore periph clock context
 *
 */
void ux500_restore_prcc_context(void)
{
	uint32_t i, base;

	for (i = 0; i < UX500_NR_PRCC_BANKS; i++) {
		base = prcc_banks[i];
		/* periph 5 has a different layout */
		if (base == PRCC_BANK_BASE(5))
			base = prcc_banks[i] + PERIPH5_PRCC_BASE_OFFSET;
		else
			base = prcc_banks[i] + PERIPH_PRCC_BASE_OFFSET;

		writel(prcc_contxt[i].periph_bus_clk, base + PRCC_BCK_EN);
		writel(prcc_contxt[i].periph_kern_clk, base + PRCC_KCK_EN);
	}

	return;
}

/* UART2(console) context */
static void __iomem *uart_backup_base, *uart_register_base;

/**
 * ux500_save_uart_context() - save uart2 (console)  context
 *
 */
void ux500_save_uart_context(void)
{
	/* FIXME : blind memcpy must be replaced */
	memcpy(uart_backup_base, uart_register_base, SZ_4K);
	return;
}

/**
 * ux500_restore_uart_context() - restore uart2 (console)  context
 *
 */
void ux500_restore_uart_context(void)
{
	memcpy(uart_register_base, uart_backup_base, SZ_4K);
	return;
}

/* ST ICN context */
static struct ux500_interconnect_contxt icn_contxt;

/**
 * ux500_save_icn_context() - save ICN context
 *
 */
void ux500_save_icn_context(void)
{
	uint32_t base = IO_ADDRESS(U8500_ICN_BASE);

	icn_contxt.ux500_hibw1_esram_in_pri_regs[0] =
		readb(base + NODE_HIBW1_ESRAM_IN_0_PRIORITY_REG);
	icn_contxt.ux500_hibw1_esram_in_pri_regs[1] =
		readb(base + NODE_HIBW1_ESRAM_IN_1_PRIORITY_REG);
	icn_contxt.ux500_hibw1_esram_in_pri_regs[2] =
		readb(base + NODE_HIBW1_ESRAM_IN_2_PRIORITY_REG);

	icn_contxt.ux500_hibw1_esram_in0_arb_regs[0] =
		readb(base + NODE_HIBW1_ESRAM_IN_0_ARB_1_LIMIT_REG);
	icn_contxt.ux500_hibw1_esram_in0_arb_regs[1] =
		readb(base + NODE_HIBW1_ESRAM_IN_0_ARB_2_LIMIT_REG);
	icn_contxt.ux500_hibw1_esram_in0_arb_regs[2] =
		readb(base + NODE_HIBW1_ESRAM_IN_0_ARB_3_LIMIT_REG);

	icn_contxt.ux500_hibw1_esram_in1_arb_regs[0] =
		readb(base + NODE_HIBW1_ESRAM_IN_1_ARB_1_LIMIT_REG);
	icn_contxt.ux500_hibw1_esram_in1_arb_regs[1] =
		readb(base + NODE_HIBW1_ESRAM_IN_1_ARB_2_LIMIT_REG);
	icn_contxt.ux500_hibw1_esram_in1_arb_regs[2] =
		readb(base + NODE_HIBW1_ESRAM_IN_1_ARB_3_LIMIT_REG);

	icn_contxt.ux500_hibw1_esram_in2_arb_regs[0] =
		readb(base + NODE_HIBW1_ESRAM_IN_2_ARB_1_LIMIT_REG);
	icn_contxt.ux500_hibw1_esram_in2_arb_regs[1] =
		readb(base + NODE_HIBW1_ESRAM_IN_2_ARB_2_LIMIT_REG);
	icn_contxt.ux500_hibw1_esram_in2_arb_regs[2] =
		readb(base + NODE_HIBW1_ESRAM_IN_2_ARB_3_LIMIT_REG);

	icn_contxt.ux500_hibw1_ddr_in_prio_regs[0] =
		readb(base + NODE_HIBW1_DDR_IN_0_PRIORITY_REG);
	icn_contxt.ux500_hibw1_ddr_in_prio_regs[1] =
		readb(base + NODE_HIBW1_DDR_IN_1_PRIORITY_REG);
	icn_contxt.ux500_hibw1_ddr_in_prio_regs[2] =
		readb(base + NODE_HIBW1_DDR_IN_2_PRIORITY_REG);

	icn_contxt.ux500_hibw1_ddr_in_limit_regs[0] =
		readb(base + NODE_HIBW1_DDR_IN_0_LIMIT_REG);
	icn_contxt.ux500_hibw1_ddr_in_limit_regs[1] =
		readb(base + NODE_HIBW1_DDR_IN_1_LIMIT_REG);
	icn_contxt.ux500_hibw1_ddr_in_limit_regs[2] =
		readb(base + NODE_HIBW1_DDR_IN_2_LIMIT_REG);

	icn_contxt.ux500_hibw1_ddr_out_prio_reg =
		readb(base + NODE_HIBW1_DDR_OUT_0_PRIORITY_REG);

	icn_contxt.ux500_hibw2_esram_in_pri_regs[0] =
		readb(base + NODE_HIBW2_ESRAM_IN_0_PRIORITY_REG);
	icn_contxt.ux500_hibw2_esram_in_pri_regs[1] =
		readb(base + NODE_HIBW2_ESRAM_IN_1_PRIORITY_REG);

	icn_contxt.ux500_hibw2_esram_in0_arblimit_regs[0] =
		readb(base + NODE_HIBW2_ESRAM_IN_0_ARB_1_LIMIT_REG);
	icn_contxt.ux500_hibw2_esram_in0_arblimit_regs[1] =
		 readb(base + NODE_HIBW2_ESRAM_IN_0_ARB_2_LIMIT_REG);
	icn_contxt.ux500_hibw2_esram_in0_arblimit_regs[2] =
		 readb(base + NODE_HIBW2_ESRAM_IN_0_ARB_3_LIMIT_REG);

	icn_contxt.ux500_hibw2_esram_in1_arblimit_regs[0] =
		readb(base + NODE_HIBW2_ESRAM_IN_1_ARB_1_LIMIT_REG);
	icn_contxt.ux500_hibw2_esram_in1_arblimit_regs[1] =
		readb(base + NODE_HIBW2_ESRAM_IN_1_ARB_2_LIMIT_REG);
	icn_contxt.ux500_hibw2_esram_in1_arblimit_regs[2] =
		readb(base + NODE_HIBW2_ESRAM_IN_1_ARB_3_LIMIT_REG);

	icn_contxt.ux500_hibw2_ddr_in_prio_regs[0] =
		readb(base + NODE_HIBW2_DDR_IN_0_PRIORITY_REG);
	icn_contxt.ux500_hibw2_ddr_in_prio_regs[1] =
		readb(base + NODE_HIBW2_DDR_IN_1_PRIORITY_REG);
	icn_contxt.ux500_hibw2_ddr_in_prio_regs[2] =
		readb(base + NODE_HIBW2_DDR_IN_2_PRIORITY_REG);
	icn_contxt.ux500_hibw2_ddr_in_prio_regs[3] =
		readb(base + NODE_HIBW2_DDR_IN_3_PRIORITY_REG);

	icn_contxt.ux500_hibw2_ddr_in_limit_regs[0] =
		readb(base + NODE_HIBW2_DDR_IN_0_LIMIT_REG);
	icn_contxt.ux500_hibw2_ddr_in_limit_regs[1] =
		readb(base + NODE_HIBW2_DDR_IN_1_LIMIT_REG);

	icn_contxt.ux500_esram12_in_prio_regs[0] =
		readb(base + NODE_ESRAM1_2_IN_0_PRIORITY_REG);
	icn_contxt.ux500_esram12_in_prio_regs[1] =
		readb(base + NODE_ESRAM1_2_IN_1_PRIORITY_REG);
	icn_contxt.ux500_esram12_in_prio_regs[2] =
		readb(base + NODE_ESRAM1_2_IN_2_PRIORITY_REG);
	icn_contxt.ux500_esram12_in_prio_regs[3] =
		readb(base + NODE_ESRAM1_2_IN_3_PRIORITY_REG);

	icn_contxt.ux500_esram12_in_arb_lim_regs[0] =
		readb(base + NODE_ESRAM1_2_IN_0_ARB_1_LIMIT_REG);
	icn_contxt.ux500_esram12_in_arb_lim_regs[1] =
		readb(base + NODE_ESRAM1_2_IN_0_ARB_2_LIMIT_REG);
	icn_contxt.ux500_esram12_in_arb_lim_regs[2] =
		readb(base + NODE_ESRAM1_2_IN_1_ARB_1_LIMIT_REG);
	icn_contxt.ux500_esram12_in_arb_lim_regs[3] =
		readb(base + NODE_ESRAM1_2_IN_1_ARB_2_LIMIT_REG);
	icn_contxt.ux500_esram12_in_arb_lim_regs[4] =
		readb(base + NODE_ESRAM1_2_IN_2_ARB_1_LIMIT_REG);
	icn_contxt.ux500_esram12_in_arb_lim_regs[5] =
		readb(base + NODE_ESRAM1_2_IN_2_ARB_2_LIMIT_REG);
	icn_contxt.ux500_esram12_in_arb_lim_regs[6] =
		readb(base + NODE_ESRAM1_2_IN_3_ARB_1_LIMIT_REG);
	icn_contxt.ux500_esram12_in_arb_lim_regs[7] =
		readb(base + NODE_ESRAM1_2_IN_3_ARB_2_LIMIT_REG);

	icn_contxt.ux500_esram34_in_prio_regs[0] =
		readb(base + NODE_ESRAM3_4_IN_0_PRIORITY_REG);
	icn_contxt.ux500_esram34_in_prio_regs[1] =
		readb(base + NODE_ESRAM3_4_IN_1_PRIORITY_REG);
	icn_contxt.ux500_esram34_in_prio_regs[2] =
		readb(base + NODE_ESRAM3_4_IN_2_PRIORITY_REG);
	icn_contxt.ux500_esram34_in_prio_regs[3] =
		readb(base + NODE_ESRAM3_4_IN_3_PRIORITY_REG);

	icn_contxt.ux500_esram34_in_arb_lim_regs[0] =
		readb(base + NODE_ESRAM3_4_IN_0_ARB_1_LIMIT_REG);
	icn_contxt.ux500_esram34_in_arb_lim_regs[1] =
		readb(base + NODE_ESRAM3_4_IN_0_ARB_2_LIMIT_REG);
	icn_contxt.ux500_esram34_in_arb_lim_regs[2] =
		readb(base + NODE_ESRAM3_4_IN_1_ARB_1_LIMIT_REG);
	icn_contxt.ux500_esram34_in_arb_lim_regs[3] =
		readb(base + NODE_ESRAM3_4_IN_1_ARB_2_LIMIT_REG);
	icn_contxt.ux500_esram34_in_arb_lim_regs[4] =
		readb(base + NODE_ESRAM3_4_IN_2_ARB_1_LIMIT_REG);
	icn_contxt.ux500_esram34_in_arb_lim_regs[5] =
		readb(base + NODE_ESRAM3_4_IN_2_ARB_2_LIMIT_REG);
	icn_contxt.ux500_esram34_in_arb_lim_regs[6] =
		readb(base + NODE_ESRAM3_4_IN_3_ARB_1_LIMIT_REG);
	icn_contxt.ux500_esram34_in_arb_lim_regs[7] =
		readb(base + NODE_ESRAM3_4_IN_3_ARB_2_LIMIT_REG);

	return;
}

/**
 * ux500_restore_icn_context() - restore ICN context
 *
 */
void ux500_restore_icn_context(void)
{
	uint32_t base = IO_ADDRESS(U8500_ICN_BASE);

	writel(icn_contxt.ux500_hibw1_esram_in_pri_regs[0],
		base + NODE_HIBW1_ESRAM_IN_0_PRIORITY_REG);
	writel(icn_contxt.ux500_hibw1_esram_in_pri_regs[1],
		base + NODE_HIBW1_ESRAM_IN_1_PRIORITY_REG);
	writel(icn_contxt.ux500_hibw1_esram_in_pri_regs[2],
		base + NODE_HIBW1_ESRAM_IN_2_PRIORITY_REG);

	writel(icn_contxt.ux500_hibw1_esram_in0_arb_regs[0],
		base + NODE_HIBW1_ESRAM_IN_0_ARB_1_LIMIT_REG);
	writel(icn_contxt.ux500_hibw1_esram_in0_arb_regs[1],
		base + NODE_HIBW1_ESRAM_IN_0_ARB_2_LIMIT_REG);
	writel(icn_contxt.ux500_hibw1_esram_in0_arb_regs[2],
		base + NODE_HIBW1_ESRAM_IN_0_ARB_3_LIMIT_REG);

	writel(icn_contxt.ux500_hibw1_esram_in1_arb_regs[0],
		base + NODE_HIBW1_ESRAM_IN_1_ARB_1_LIMIT_REG);
	writel(icn_contxt.ux500_hibw1_esram_in1_arb_regs[1],
		base + NODE_HIBW1_ESRAM_IN_1_ARB_2_LIMIT_REG);
	writel(icn_contxt.ux500_hibw1_esram_in1_arb_regs[2],
		base + NODE_HIBW1_ESRAM_IN_1_ARB_3_LIMIT_REG);

	writel(icn_contxt.ux500_hibw1_esram_in2_arb_regs[0],
		base + NODE_HIBW1_ESRAM_IN_2_ARB_1_LIMIT_REG);
	writel(icn_contxt.ux500_hibw1_esram_in2_arb_regs[1],
		base + NODE_HIBW1_ESRAM_IN_2_ARB_2_LIMIT_REG);
	writel(icn_contxt.ux500_hibw1_esram_in2_arb_regs[2],
		base + NODE_HIBW1_ESRAM_IN_2_ARB_3_LIMIT_REG);

	writel(icn_contxt.ux500_hibw1_ddr_in_prio_regs[0],
		base + NODE_HIBW1_DDR_IN_0_PRIORITY_REG);
	writel(icn_contxt.ux500_hibw1_ddr_in_prio_regs[1],
		base + NODE_HIBW1_DDR_IN_1_PRIORITY_REG);
	writel(icn_contxt.ux500_hibw1_ddr_in_prio_regs[2],
		base + NODE_HIBW1_DDR_IN_2_PRIORITY_REG);

	writel(icn_contxt.ux500_hibw1_ddr_in_limit_regs[0],
		base + NODE_HIBW1_DDR_IN_0_LIMIT_REG);
	writel(icn_contxt.ux500_hibw1_ddr_in_limit_regs[1],
		base + NODE_HIBW1_DDR_IN_1_LIMIT_REG);
	writel(icn_contxt.ux500_hibw1_ddr_in_limit_regs[2],
		base + NODE_HIBW1_DDR_IN_2_LIMIT_REG);

	writel(icn_contxt.ux500_hibw1_ddr_out_prio_reg,
		base + NODE_HIBW1_DDR_OUT_0_PRIORITY_REG);

	writel(icn_contxt.ux500_hibw2_esram_in_pri_regs[0],
		base + NODE_HIBW2_ESRAM_IN_0_PRIORITY_REG);
	writel(icn_contxt.ux500_hibw2_esram_in_pri_regs[1],
		base + NODE_HIBW2_ESRAM_IN_1_PRIORITY_REG);

	writel(icn_contxt.ux500_hibw2_esram_in0_arblimit_regs[0],
		base + NODE_HIBW2_ESRAM_IN_0_ARB_1_LIMIT_REG);
	writel(icn_contxt.ux500_hibw2_esram_in0_arblimit_regs[1],
		base + NODE_HIBW2_ESRAM_IN_0_ARB_2_LIMIT_REG);
	writel(icn_contxt.ux500_hibw2_esram_in0_arblimit_regs[2],
		base + NODE_HIBW2_ESRAM_IN_0_ARB_3_LIMIT_REG);

	writel(icn_contxt.ux500_hibw2_esram_in1_arblimit_regs[0],
		base + NODE_HIBW2_ESRAM_IN_1_ARB_1_LIMIT_REG);
	writel(icn_contxt.ux500_hibw2_esram_in1_arblimit_regs[1],
		base + NODE_HIBW2_ESRAM_IN_1_ARB_2_LIMIT_REG);
	writel(icn_contxt.ux500_hibw2_esram_in1_arblimit_regs[2],
		base + NODE_HIBW2_ESRAM_IN_1_ARB_3_LIMIT_REG);

	writel(icn_contxt.ux500_hibw2_ddr_in_prio_regs[0],
		base + NODE_HIBW2_DDR_IN_0_PRIORITY_REG);
	writel(icn_contxt.ux500_hibw2_ddr_in_prio_regs[1],
		base + NODE_HIBW2_DDR_IN_1_PRIORITY_REG);
	writel(icn_contxt.ux500_hibw2_ddr_in_prio_regs[2],
		base + NODE_HIBW2_DDR_IN_2_PRIORITY_REG);
	writel(icn_contxt.ux500_hibw2_ddr_in_prio_regs[3],
		base + NODE_HIBW2_DDR_IN_3_PRIORITY_REG);

	writel(icn_contxt.ux500_hibw2_ddr_in_limit_regs[0],
		base + NODE_HIBW2_DDR_IN_0_LIMIT_REG);
	writel(icn_contxt.ux500_hibw2_ddr_in_limit_regs[1],
		base + NODE_HIBW2_DDR_IN_1_LIMIT_REG);

	writel(icn_contxt.ux500_esram12_in_prio_regs[0],
		base + NODE_ESRAM1_2_IN_0_PRIORITY_REG);
	writel(icn_contxt.ux500_esram12_in_prio_regs[1],
		base + NODE_ESRAM1_2_IN_1_PRIORITY_REG);
	writel(icn_contxt.ux500_esram12_in_prio_regs[2],
		base + NODE_ESRAM1_2_IN_2_PRIORITY_REG);
	writel(icn_contxt.ux500_esram12_in_prio_regs[3],
		base + NODE_ESRAM1_2_IN_3_PRIORITY_REG);

	writel(icn_contxt.ux500_esram12_in_arb_lim_regs[0],
		base + NODE_ESRAM1_2_IN_0_ARB_1_LIMIT_REG);
	writel(icn_contxt.ux500_esram12_in_arb_lim_regs[1],
		base + NODE_ESRAM1_2_IN_0_ARB_2_LIMIT_REG);
	writel(icn_contxt.ux500_esram12_in_arb_lim_regs[2],
		base + NODE_ESRAM1_2_IN_1_ARB_1_LIMIT_REG);
	writel(icn_contxt.ux500_esram12_in_arb_lim_regs[3],
		base + NODE_ESRAM1_2_IN_1_ARB_2_LIMIT_REG);
	writel(icn_contxt.ux500_esram12_in_arb_lim_regs[4],
		base + NODE_ESRAM1_2_IN_2_ARB_1_LIMIT_REG);
	writel(icn_contxt.ux500_esram12_in_arb_lim_regs[5],
		base + NODE_ESRAM1_2_IN_2_ARB_2_LIMIT_REG);
	writel(icn_contxt.ux500_esram12_in_arb_lim_regs[6],
		base + NODE_ESRAM1_2_IN_3_ARB_1_LIMIT_REG);
	writel(icn_contxt.ux500_esram12_in_arb_lim_regs[7],
		base + NODE_ESRAM1_2_IN_3_ARB_2_LIMIT_REG);

	writel(icn_contxt.ux500_esram34_in_prio_regs[0],
		base + NODE_ESRAM3_4_IN_0_PRIORITY_REG);
	writel(icn_contxt.ux500_esram34_in_prio_regs[1],
		base + NODE_ESRAM3_4_IN_1_PRIORITY_REG);
	writel(icn_contxt.ux500_esram34_in_prio_regs[2],
		base + NODE_ESRAM3_4_IN_2_PRIORITY_REG);
	writel(icn_contxt.ux500_esram34_in_prio_regs[3],
		base + NODE_ESRAM3_4_IN_3_PRIORITY_REG);

	writel(icn_contxt.ux500_esram34_in_arb_lim_regs[0],
		base + NODE_ESRAM3_4_IN_0_ARB_1_LIMIT_REG);
	writel(icn_contxt.ux500_esram34_in_arb_lim_regs[1],
		base + NODE_ESRAM3_4_IN_0_ARB_2_LIMIT_REG);
	writel(icn_contxt.ux500_esram34_in_arb_lim_regs[2],
		base + NODE_ESRAM3_4_IN_1_ARB_1_LIMIT_REG);
	writel(icn_contxt.ux500_esram34_in_arb_lim_regs[3],
		base + NODE_ESRAM3_4_IN_1_ARB_2_LIMIT_REG);
	writel(icn_contxt.ux500_esram34_in_arb_lim_regs[4],
		base + NODE_ESRAM3_4_IN_2_ARB_1_LIMIT_REG);
	writel(icn_contxt.ux500_esram34_in_arb_lim_regs[5],
		base + NODE_ESRAM3_4_IN_2_ARB_2_LIMIT_REG);
	writel(icn_contxt.ux500_esram34_in_arb_lim_regs[6],
		base + NODE_ESRAM3_4_IN_3_ARB_1_LIMIT_REG);
	writel(icn_contxt.ux500_esram34_in_arb_lim_regs[7],
		base + NODE_ESRAM3_4_IN_3_ARB_2_LIMIT_REG);

	return;
}

/* SCU context */
static struct ux500_scu_context scu_contxt;

/**
 * ux500_save_scu_context() - save SCU context
 *
 */
void ux500_save_scu_context(void)
{
	uint32_t base = IO_ADDRESS(UX500_SCU_BASE);

	scu_contxt.scu_ctrl 		= readl(base + SCU_CTRL);
	scu_contxt.scu_cpu_pwrstatus 	= readl(base + SCU_CPU_STATUS);
	scu_contxt.scu_inv_all_nonsecure = readl(base + SCU_INVALIDATE);
	scu_contxt.scu_filter_start_addr = readl(base + SCU_FILTER_STARTADDR);
	scu_contxt.scu_filter_end_addr	= readl(base + SCU_FILTER_ENDADDR);
	scu_contxt.scu_access_ctrl_sac	= readl(base + SCU_ACCESS_CTRL_SAC);

	return;
}

/**
 * ux500_restore_scu_context() - restore SCU context
 *
 */
void ux500_restore_scu_context(void)
{
	uint32_t base = IO_ADDRESS(UX500_SCU_BASE);

	writel(scu_contxt.scu_ctrl, 		base + SCU_CTRL);
	writel(scu_contxt.scu_cpu_pwrstatus, 	base + SCU_CPU_STATUS);
	writel(scu_contxt.scu_inv_all_nonsecure, base + SCU_INVALIDATE);
	writel(scu_contxt.scu_filter_start_addr, base + SCU_FILTER_STARTADDR);
	writel(scu_contxt.scu_filter_end_addr, 	base + SCU_FILTER_ENDADDR);
	writel(scu_contxt.scu_access_ctrl_sac, 	base + SCU_ACCESS_CTRL_SAC);

	return;
}

/**
 * ux500_save_peripheral_context() - save all peripheral context
 *
 */
static void ux500_save_peripheral_context(void)
{
	/* save the inter-connect per */
	ux500_save_icn_context();
	/* save uart2 per */
	ux500_save_uart_context();
	/* save gpio per */
	ux500_save_gpio_context();
	/* save periph clock configs */
	ux500_save_prcc_context();
	/* save the scu config */
	ux500_save_scu_context();

	return;
}

/**
 * ux500_restore_peripheral_context() - restore peripheral context
 *
 */
static void ux500_restore_peripheral_context(void)
{
	/* restore scu */
	ux500_restore_scu_context();
	/* restore inter-connct */
	ux500_restore_icn_context();
	/* restore periph clock */
	ux500_restore_prcc_context();
	/* restore gpio per */
	ux500_restore_gpio_context();
	/* restore uart2 */
	ux500_restore_uart_context();

	return;
}

/**
 * ux500_save_core_context() - save core(gic/..) context
 *
 */
static void ux500_save_core_context(void)
{
	ux500_save_gic_context();
	return;
}

/*
 * FIXME : replace this hack for console gpio
 * 	   this config makes the gpio29 pins pull up
 *	   by deflt
 */
#define CONSOLE_GPIO_HACK	(0x60000000)

static int u8500_pm_enter(suspend_state_t state)
{
	uint32_t ret = 0;
	uint32_t cpu = smp_processor_id();

	/* FIXME : replace these with generic APIs. this is
	 * 	   a hack!!
	 */
	writel(readl(GPIO_BK0_DAT) | CONSOLE_GPIO_HACK, GPIO_BK0_DAT);

	/* configure the prcm for a sleep/deep sleep wakeup */
	prcmu_configure_wakeup_events(
			(PRCMU_WAKEUPBY_MODEM |
				PRCMU_WAKEUPBY_ARMITMGMT |
				PRCMU_WAKEUPBY_APE4500INT |
				PRCMU_WAKEUPBY_GPIOS |
				PRCMU_WAKEUPBY_RTCRTT),
			0x0, LOW_POWER_WAKEUP);

	/*
	 * enforce the IOCR; make all gpios input be default
	 * we will have no console now onwards
	 */
	writel(ENABLE, PRCMU_IOCR);

	/* save peripheral context */
	ux500_save_peripheral_context();

	switch (state) {
	case PM_SUSPEND_STANDBY:

		/*
		 * NOTE: due to a bug on the HW, the PRCM_A9_MASK_ACK
		 *	 doesnt work. as a result, reading the register
		 * 	 wont help and we skip it for now. enable the
		 *	 mask for now.
		 */
		writel(ENABLE, PRCM_A9_MASK_REQ);

		/* request the PRCM for the sleep */
		prcmu_apply_ap_state_transition(APEXECUTE_TO_APSLEEP,
			DDR_PWR_STATE_UNCHANGED, 0);

		break;
	case PM_SUSPEND_MEM:

		/* ROM code addresses to store backup contents */
		/* pass the physical address of back up to ROM code */
		writel(virt_to_phys(ux500_backup_ptr),
				IO_ADDRESS(U8500_EXT_BACKUPRAM_ADDR));
		writel(IO_ADDRESS(U8500_BACKUPRAM0_BASE),
			IO_ADDRESS(U8500_CPU0_PUBLIC_BACKUP));
		writel(IO_ADDRESS(U8500_BACKUPRAM0_BASE),
			IO_ADDRESS(U8500_CPU1_PUBLIC_BACKUP));

		/* core context to be saved */
		ux500_save_core_context();

		/* final context saving, and then into deep sleep */
		ux500_cpu_context_deepsleep(cpu);

		/* gone into deep sleep */

		/* woken up; restore contexts */
		ux500_restore_gic_context();

		break;
	default:
		ret = -EINVAL;
	}

	ux500_restore_peripheral_context();

	/* disable ioforce */
	writel(DISABLE, PRCMU_IOCR);

	/* debug print:  */
	if (readb(PRCM_ACK_MB0_AP_PWRST_STATUS) == DEEPSLEEP_TO_EXECUTEOK)
		printk(KERN_INFO
			"ux500: Wakeup from ApDeepSleep successful\n");
	else if (readb(PRCM_ACK_MB0_AP_PWRST_STATUS) == SLEEP_TO_EXECUTEOK)
		printk(KERN_INFO
			"ux500: Wakeup from ApSleep successful\n");
	else
		printk(KERN_INFO
			"ux500: Wakeup Status=0x%x\n",
				readb(PRCM_ACK_MB0_AP_PWRST_STATUS));

	return ret;
}

static void u8500_pm_finish(void)
{

}

static int u8500_pm_valid(suspend_state_t state)
{
	return (state == PM_SUSPEND_MEM || state == PM_SUSPEND_STANDBY);
}

static struct platform_suspend_ops u8500_pm_ops = {
	.begin = u8500_pm_begin,
	.prepare = u8500_pm_prepare,
	.enter = u8500_pm_enter,
	.finish = u8500_pm_finish,
	.valid = u8500_pm_valid,
};

#define GIC_SUPPORTED_INT_MASK	(0x1f)
/* FIXME : remove this later when board files have such defines */
#define MOP500_UART2RX_GPIO	(29)

static int __init u8500_pm_init(void)
{
	uint32_t base;

	/* find out the total interrupts on this platform */
	base = IO_ADDRESS(U8500_GIC_DIST_BASE);

	/* find how many interrupts are supported */
	ux500_max_irq = readl(base + GIC_DIST_CTR) & GIC_SUPPORTED_INT_MASK;
	ux500_max_irq = (ux500_max_irq + 1) * 32;

	/* GIC supports maximum 1020 interrupts */
	if (ux500_max_irq > max(1020, NR_IRQS))
		ux500_max_irq = max(1020, NR_IRQS);

	/* allocate backup pointers for CPU0/1 */
	arm_cntxt_cpu1 = kzalloc(SZ_1K, GFP_KERNEL);
	if (!arm_cntxt_cpu1) {
		printk(KERN_WARNING
			 "ux500-pm: backup ptr allocation failed\n");
		return -ENOMEM;
	}

	arm_cntxt_cpu0 = kzalloc(SZ_1K, GFP_KERNEL);
	if (!arm_cntxt_cpu0) {
		printk(KERN_WARNING
			"ux500-pm: backup ptr allocation failed\n");
		return -ENOMEM;
	}

	/* allocate backup pointers for UART context */
	uart_register_base = ioremap(U8500_UART2_BASE, SZ_4K);
	if (!uart_register_base)
		printk(KERN_WARNING "u8500-pm: uart register base NULL\n");

	uart_backup_base = kzalloc(SZ_4K, GFP_KERNEL);
	if (!uart_backup_base)
		printk(KERN_WARNING "u8500-pm: uart register backup NULL\n");

	/* make the UART2-Rx/GPIO29 as a wakeup event */
	set_irq_type(GPIO_TO_IRQ(MOP500_UART2RX_GPIO), IRQ_TYPE_EDGE_BOTH);
	set_irq_wake(GPIO_TO_IRQ(MOP500_UART2RX_GPIO), ENABLE);

	/* allocate backup pointer for RAM data */
	ux500_backup_ptr = (void *)__get_free_pages(GFP_KERNEL,
					get_order(U8500_BACKUPRAM_SIZE));
	if (!ux500_backup_ptr) {
		printk(KERN_WARNING "ux500-pm: couldnt allocate backup ptr\n");
		return -ENOMEM;
	}

	/* register the global power off hook */
	pm_power_off = u8500_pm_poweroff;

	suspend_set_ops(&u8500_pm_ops);

	return 0;
}

device_initcall(u8500_pm_init);
