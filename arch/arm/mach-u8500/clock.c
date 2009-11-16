/*
 *  linux/arch/arm/mach-u8500/clock.c
 *
 *  Copyright (C) ST Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/mutex.h>

#include <asm/clkdev.h>
#include <asm/io.h>

#include <mach/hardware.h>
#include <mach/prcmu-regs.h>
#include "clock.h"

static DEFINE_SPINLOCK(clocks_lock);

static void __clk_enable(struct clk *clk)
{
	if (clk->enabled++ == 0) {
		if (clk->parent_cluster)
			__clk_enable(clk->parent_cluster);

		if (clk->parent_periph)
			__clk_enable(clk->parent_periph);

		if (clk->ops && clk->ops->enable)
			clk->ops->enable(clk);
	}
}

int clk_enable(struct clk *clk)
{
	unsigned long flags;

	if (!clk || IS_ERR(clk))
		return -EINVAL;

	spin_lock_irqsave(&clocks_lock, flags);
	__clk_enable(clk);
	spin_unlock_irqrestore(&clocks_lock, flags);

	return 0;
}
EXPORT_SYMBOL(clk_enable);

static void __clk_disable(struct clk *clk)
{
	if (--clk->enabled == 0) {
		if (clk->ops && clk->ops->disable)
			clk->ops->disable(clk);

		if (clk->parent_periph)
			__clk_disable(clk->parent_periph);

		if (clk->parent_cluster)
			__clk_disable(clk->parent_cluster);
	}
}

void clk_disable(struct clk *clk)
{
	unsigned long flags;

	if (!clk || IS_ERR(clk))
		return;

	WARN_ON(!clk->enabled);

	spin_lock_irqsave(&clocks_lock, flags);
	__clk_disable(clk);
	spin_unlock_irqrestore(&clocks_lock, flags);
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	unsigned long rate;

	if (clk->ops && clk->ops->get_rate)
		return clk->ops->get_rate(clk);

	rate = clk->rate;
	if (!rate && clk->parent_periph)
		rate = clk_get_rate(clk->parent_periph);

	return rate;
}
EXPORT_SYMBOL(clk_get_rate);

static void clk_prcmu_enable(struct clk *clk)
{
	void __iomem *cg_set_reg = (void __iomem *)PRCM_YYCLKEN0_MGT_SET
				   + clk->prcmu_cg_off;

	writel(1 << clk->prcmu_cg_bit, cg_set_reg);
}

static void clk_prcmu_disable(struct clk *clk)
{
	void __iomem *cg_clr_reg = (void __iomem *)PRCM_YYCLKEN0_MGT_CLR
				    + clk->prcmu_cg_off;

	writel(1 << clk->prcmu_cg_bit, cg_clr_reg);
}

/* ED doesn't have the combined set/clr registers */

static void clk_prcmu_ed_enable(struct clk *clk)
{
	unsigned int val = readl(clk->prcmu_cg_mgt);

	val |= 1 << 8;
	writel(val, clk->prcmu_cg_mgt);
}

static void clk_prcmu_ed_disable(struct clk *clk)
{
	unsigned int val = readl(clk->prcmu_cg_mgt);

	val &= ~(1 << 8);
	writel(val, clk->prcmu_cg_mgt);
}

static struct clkops clk_prcmu_ops = {
	.enable = clk_prcmu_enable,
	.disable = clk_prcmu_disable,
};

static void clk_prcc_enable(struct clk *clk)
{
	if (clk->prcc_kernel != -1)
		writel(1 << (clk->prcc_kernel), clk->prcc_base + 0x8);

	if (clk->prcc_bus != -1)
		writel(1 << (clk->prcc_bus), clk->prcc_base + 0x0);
}

static void clk_prcc_disable(struct clk *clk)
{
	if (clk->prcc_bus != -1)
		writel(1 << (clk->prcc_bus), clk->prcc_base + 0x4);

	if (clk->prcc_kernel != -1)
		writel(1 << (clk->prcc_kernel), clk->prcc_base + 0xc);
}

static struct clkops clk_prcc_ops = {
	.enable = clk_prcc_enable,
	.disable = clk_prcc_disable,
};

static struct clk clk_32khz = {
	.rate = 32000,
};

/*
 * PRCMU level clock gating
 */

/* Bank 0 */
static DEFINE_PRCMU_CLK(svaclk, 0x0, 2, SVAMMDSPCLK);
static DEFINE_PRCMU_CLK(siaclk, 0x0, 3, SIAMMDSPCLK);
static DEFINE_PRCMU_CLK(sgaclk, 0x0, 4, SGACLK);
static DEFINE_PRCMU_CLK_RATE(uartclk, 0x0, 5, UARTCLK, 38400000);
static DEFINE_PRCMU_CLK(msp02clk, 0x0, 6, MSP02CLK);
static DEFINE_PRCMU_CLK(msp1clk, 0x0, 7, MSP1CLK); /* v1 */
static DEFINE_PRCMU_CLK(i2cclk, 0x0, 8, I2CCLK);
static DEFINE_PRCMU_CLK(sdmmcclk, 0x0, 9, SDMMCCLK);
static DEFINE_PRCMU_CLK(slimclk, 0x0, 10, SLIMCLK);
static DEFINE_PRCMU_CLK(per1clk, 0x0, 11, PER1CLK);
static DEFINE_PRCMU_CLK(per2clk, 0x0, 12, PER2CLK);
static DEFINE_PRCMU_CLK(per3clk, 0x0, 13, PER3CLK);
static DEFINE_PRCMU_CLK(per5clk, 0x0, 14, PER5CLK);
static DEFINE_PRCMU_CLK(per6clk, 0x0, 15, PER6CLK);
static DEFINE_PRCMU_CLK(per7clk, 0x0, 16, PER7CLK);
static DEFINE_PRCMU_CLK(lcdclk, 0x0, 17, LCDCLK);
static DEFINE_PRCMU_CLK(bmlclk, 0x0, 18, BMLCLK);
static DEFINE_PRCMU_CLK(hsitxclk, 0x0, 19, HSITXCLK);
static DEFINE_PRCMU_CLK(hsirxclk, 0x0, 20, HSIRXCLK);
static DEFINE_PRCMU_CLK(hdmiclk, 0x0, 21, HDMICLK);
static DEFINE_PRCMU_CLK(apeatclk, 0x0, 22, APEATCLK);
static DEFINE_PRCMU_CLK(apetraceclk, 0x0, 23, APETRACECLK);
static DEFINE_PRCMU_CLK(mcdeclk, 0x0, 24, MCDECLK);
static DEFINE_PRCMU_CLK(ipi2clk, 0x0, 25, IPI2CCLK);
static DEFINE_PRCMU_CLK(dsialtclk, 0x0, 26, DSIALTCLK); /* v1 */
static DEFINE_PRCMU_CLK(dmaclk, 0x0, 27, DMACLK);
static DEFINE_PRCMU_CLK(b2r2clk, 0x0, 28, B2R2CLK);
static DEFINE_PRCMU_CLK(tvclk, 0x0, 29, TVCLK);
static DEFINE_PRCMU_CLK(uniproclk, 0x0, 30, UNIPROCLK); /* v1 */
static DEFINE_PRCMU_CLK(sspclk, 0x0, 31, SSPCLK); /* v1 */

/* Bank 1 */
static DEFINE_PRCMU_CLK(rngclk, 0x4, 0, RNGCLK); /* v1 */
static DEFINE_PRCMU_CLK(uiccclk, 0x4, 1, UICCCLK); /* v1 */

/*
 * PRCC level clock gating
 * Format: per#, clk, PCKEN bit, KCKEN bit, parent
 */

/* Peripheral Cluster #1 */
static DEFINE_PRCC_CLK(1, i2c4, 	10, 9, &clk_i2cclk);
static DEFINE_PRCC_CLK(1, gpio0,	9, -1, NULL);
static DEFINE_PRCC_CLK(1, slimbus0, 	8,  8, &clk_slimclk);
static DEFINE_PRCC_CLK(1, spi3_ed, 	7,  7, NULL);
static DEFINE_PRCC_CLK(1, spi3_v1, 	7, -1, NULL);
static DEFINE_PRCC_CLK(1, i2c2, 	6,  6, &clk_i2cclk);
static DEFINE_PRCC_CLK(1, sdi0,		5,  5, &clk_sdmmcclk);
static DEFINE_PRCC_CLK(1, msp1_ed, 	4,  4, &clk_msp02clk);
static DEFINE_PRCC_CLK(1, msp1_v1, 	4,  4, &clk_msp1clk);
static DEFINE_PRCC_CLK(1, msp0, 	3,  3, &clk_msp02clk);
static DEFINE_PRCC_CLK(1, i2c1, 	2,  2, &clk_i2cclk);
static DEFINE_PRCC_CLK(1, uart1, 	1,  1, &clk_uartclk);
static DEFINE_PRCC_CLK(1, uart0, 	0,  0, &clk_uartclk);

/* Peripheral Cluster #2 */

static DEFINE_PRCC_CLK(2, gpio1_ed,	12, -1, NULL);
static DEFINE_PRCC_CLK(2, ssitx_ed, 	11, -1, NULL);
static DEFINE_PRCC_CLK(2, ssirx_ed, 	10, -1, NULL);
static DEFINE_PRCC_CLK(2, spi0_ed, 	 9, -1, NULL);
static DEFINE_PRCC_CLK(2, sdi3_ed, 	 8,  6, &clk_sdmmcclk);
static DEFINE_PRCC_CLK(2, sdi1_ed, 	 7,  5, &clk_sdmmcclk);
static DEFINE_PRCC_CLK(2, msp2_ed, 	 6,  4, &clk_msp02clk);
static DEFINE_PRCC_CLK(2, sdi4_ed, 	 4,  2, &clk_sdmmcclk);
static DEFINE_PRCC_CLK(2, pwl_ed,	 3,  1, NULL);
static DEFINE_PRCC_CLK(2, spi1_ed, 	 2, -1, NULL);
static DEFINE_PRCC_CLK(2, spi2_ed, 	 1, -1, NULL);
static DEFINE_PRCC_CLK(2, i2c3_ed, 	 0,  0, &clk_i2cclk);

static DEFINE_PRCC_CLK(2, gpio1_v1,	11, -1, NULL);
static DEFINE_PRCC_CLK(2, ssitx_v1, 	10,  7, NULL);
static DEFINE_PRCC_CLK(2, ssirx_v1, 	 9,  6, NULL);
static DEFINE_PRCC_CLK(2, spi0_v1, 	 8, -1, NULL);
static DEFINE_PRCC_CLK(2, sdi3_v1, 	 7,  5, &clk_sdmmcclk);
static DEFINE_PRCC_CLK(2, sdi1_v1, 	 6,  4, &clk_sdmmcclk);
static DEFINE_PRCC_CLK(2, msp2_v1, 	 5,  3, &clk_msp02clk);
static DEFINE_PRCC_CLK(2, sdi4_v1, 	 4,  2, &clk_sdmmcclk);
static DEFINE_PRCC_CLK(2, pwl_v1,	 3,  1, NULL);
static DEFINE_PRCC_CLK(2, spi1_v1, 	 2, -1, NULL);
static DEFINE_PRCC_CLK(2, spi2_v1, 	 1, -1, NULL);
static DEFINE_PRCC_CLK(2, i2c3_v1, 	 0,  0, &clk_i2cclk);

/* Peripheral Cluster #3 */
static DEFINE_PRCC_CLK(3, gpio2, 	8, -1, NULL);
static DEFINE_PRCC_CLK(3, sdi5, 	7,  7, &clk_sdmmcclk);
static DEFINE_PRCC_CLK(3, uart2, 	6,  6, &clk_uartclk);
static DEFINE_PRCC_CLK(3, ske, 		5,  5, &clk_32khz);
static DEFINE_PRCC_CLK(3, sdi2, 	4,  4, &clk_sdmmcclk);
static DEFINE_PRCC_CLK(3, i2c0, 	3,  3, &clk_i2cclk);
static DEFINE_PRCC_CLK(3, ssp1_ed, 	2,  2, &clk_i2cclk);
static DEFINE_PRCC_CLK(3, ssp0_ed, 	1,  1, &clk_i2cclk);
static DEFINE_PRCC_CLK(3, ssp1_v1, 	2,  2, &clk_sspclk);
static DEFINE_PRCC_CLK(3, ssp0_v1, 	1,  1, &clk_sspclk);
static DEFINE_PRCC_CLK(3, fsmc, 	0, -1, NULL);

/* Peripheral Cluster #4 is in the always on domain */

/* Peripheral Cluster #5 */
static DEFINE_PRCC_CLK(5, gpio3, 	1, -1, NULL);
static DEFINE_PRCC_CLK(5, usb_ed, 	0,  0, &clk_i2cclk);
static DEFINE_PRCC_CLK(5, usb_v1, 	0,  0, NULL);

/* Peripheral Cluster #6 */

static DEFINE_PRCC_CLK(6, mtu1_v1, 	8, -1, NULL);
static DEFINE_PRCC_CLK(6, mtu0_v1, 	7, -1, NULL);
static DEFINE_PRCC_CLK(6, cfgreg_v1, 	6,  6, NULL);
static DEFINE_PRCC_CLK(6, dmc_ed, 	6,  6, NULL);
static DEFINE_PRCC_CLK(6, hash1, 	5, -1, NULL);
static DEFINE_PRCC_CLK(6, unipro_v1, 	4,  1, &clk_uniproclk);
static DEFINE_PRCC_CLK(6, cryp1_ed, 	4, -1, NULL);
static DEFINE_PRCC_CLK(6, pka, 		3, -1, NULL);
static DEFINE_PRCC_CLK(6, hash0, 	2, -1, NULL);
static DEFINE_PRCC_CLK(6, cryp0, 	1, -1, NULL);
static DEFINE_PRCC_CLK(6, rng_ed, 	0,  0, &clk_i2cclk);
static DEFINE_PRCC_CLK(6, rng_v1, 	0,  0, &clk_rngclk);

/* Peripheral Cluster #7 */

static DEFINE_PRCC_CLK(7, tzpc0_ed, 	4, -1, NULL);
static DEFINE_PRCC_CLK(7, mtu1_ed, 	3, -1, NULL);
static DEFINE_PRCC_CLK(7, mtu0_ed, 	2, -1, NULL);
static DEFINE_PRCC_CLK(7, wdg_ed, 	1, -1, NULL);
static DEFINE_PRCC_CLK(7, cfgreg_ed, 	0, -1, NULL);

/*
 * TODO: Ensure names match with devices and then remove unnecessary entries
 * when all drivers use the clk API.
 */

static struct clk_lookup u8500_common_clkregs[] = {
	/* Peripheral Cluster #1 */
	CLK(gpio0, "gpioblock0", NULL),
	CLK(slimbus0, "slimbus0", NULL),
	CLK(i2c2, "STM-I2C.2", NULL),
	CLK(sdi0, NULL, "MMC"), /* remove */
	CLK(sdi0, "sdi0", NULL),
	CLK(msp0, "msp0", NULL),
	CLK(msp0, "MSP_I2S.0", NULL),
	CLK(i2c1, "STM-I2C.1", NULL),
	CLK(uart1, "uart1", NULL),
	CLK(uart0, "uart0", NULL),

	/* Peripheral Cluster #3 */
	CLK(gpio2, "gpioblock2", NULL),
	CLK(sdi5, "sdi5", NULL),
	CLK(uart2, "uart2", NULL),
	CLK(ske, "ske", NULL),
	CLK(sdi2, "sdi2", NULL),
	CLK(i2c0, "STM-I2C.0", NULL),
	CLK(fsmc, "fsmc", NULL),

	/* Peripheral Cluster #5 */
	CLK(gpio3, "gpioblock3", NULL),

	/* Peripheral Cluster #6 */
	CLK(hash1, "hash1", NULL),
	CLK(pka, "pka", NULL),
	CLK(hash0, "hash0", NULL),
	CLK(cryp0, "cryp0", NULL),

	/*
	 * PRCMU level clock gating
	 */

	/* Bank 0 */
	CLK(svaclk, "sva", NULL),
	CLK(siaclk, "sia", NULL),
	CLK(sgaclk, "sga", NULL),
	CLK(uartclk, "UART", NULL),
	CLK(msp02clk, "MSP02", NULL),
	CLK(i2cclk, "I2C", NULL),
	CLK(sdmmcclk, "sdmmc", NULL),
	CLK(slimclk, "SLIM", NULL),
	CLK(per1clk, "PERIPH1", NULL),
	CLK(per2clk, "PERIPH2", NULL),
	CLK(per3clk, "PERIPH3", NULL),
	CLK(per5clk, "PERIPH5", NULL),
	CLK(per6clk, "PERIPH6", NULL),
	CLK(per7clk, "PERIPH7", NULL),
	CLK(lcdclk, "lcd", NULL),
	CLK(bmlclk, "bml", NULL),
	CLK(hsitxclk, "stm-hsi.0", NULL),
	CLK(hsirxclk, "stm-hsi.1", NULL),
	CLK(hdmiclk, "hdmi", NULL),
	CLK(apeatclk, "apeat", NULL),
	CLK(apetraceclk, "apetrace", NULL),
	CLK(mcdeclk, "mcde", NULL),
	CLK(ipi2clk, "ipi2", NULL),
	CLK(dmaclk, "STM-DMA.0", NULL),
	CLK(b2r2clk, "b2r2", NULL),
	CLK(tvclk, "tv", NULL),

	/* With device names */

	CLK(mcdeclk, "U8500-MCDE.0", "mcde"),
	CLK(hdmiclk, "U8500-MCDE.0", "hdmi"),
	CLK(tvclk, "U8500-MCDE.0", "tv"),
	CLK(lcdclk, "U8500-MCDE.0", "lcd"),
	CLK(mcdeclk, "U8500-MCDE.1", "mcde"),
	CLK(hdmiclk, "U8500-MCDE.1", "hdmi"),
	CLK(tvclk, "U8500-MCDE.1", "tv"),
	CLK(lcdclk, "U8500-MCDE.1", "lcd"),
	CLK(mcdeclk, "U8500-MCDE.2", "mcde"),
	CLK(hdmiclk, "U8500-MCDE.2", "hdmi"),
	CLK(tvclk, "U8500-MCDE.2", "tv"),
	CLK(lcdclk, "U8500-MCDE.2", "lcd"),
	CLK(mcdeclk, "U8500-MCDE.3", "mcde"),
	CLK(hdmiclk, "U8500-MCDE.3", "hdmi"),
	CLK(tvclk, "U8500-MCDE.3", "tv"),
	CLK(lcdclk, "U8500-MCDE.3", "lcd"),
	CLK(b2r2clk, "U8500-B2R2.0", NULL),
};

static struct clk_lookup u8500_ed_clkregs[] = {
	/* Peripheral Cluster #1 */
	CLK(spi3_ed, "spi3", NULL),
	CLK(msp1_ed, "msp1", NULL),
	CLK(msp1_ed, "MSP_I2S.1", NULL),

	/* Peripheral Cluster #2 */
	CLK(gpio1_ed, "gpioblock1", NULL),
	CLK(ssitx_ed, "ssitx", NULL),
	CLK(ssirx_ed, "ssirx", NULL),
	CLK(spi0_ed, "spi0", NULL),
	CLK(sdi3_ed, "sdi3", NULL),
	CLK(sdi1_ed, "sdi1", NULL),
	CLK(msp2_ed, "msp2", NULL),
	CLK(msp2_ed, "MSP_I2S.2", NULL),
	CLK(sdi4_ed, NULL, "EMMC"), /* remove */
	CLK(sdi4_ed, "sdi4", NULL),
	CLK(pwl_ed, "pwl", NULL),
	CLK(spi1_ed, "spi1", NULL),
	CLK(spi2_ed, "spi2", NULL),
	CLK(i2c3_ed, "STM-I2C.3", NULL),

	/* Peripheral Cluster #3 */
	CLK(ssp1_ed, "ssp1", NULL),
	CLK(ssp0_ed, "ssp0", NULL),

	/* Peripheral Cluster #5 */
	CLK(usb_ed, "musb_hdrc.0", "usb"),

	/* Peripheral Cluster #6 */
	CLK(dmc_ed, "dmc", NULL),
	CLK(cryp1_ed, "cryp1", NULL),
	CLK(rng_ed, "rng", NULL),

	/* Peripheral Cluster #7 */
	CLK(tzpc0_ed, "tzpc0", NULL),
	CLK(mtu1_ed, "mtu1", NULL),
	CLK(mtu0_ed, "mtu0", NULL),
	CLK(wdg_ed, "wdg", NULL),
	CLK(cfgreg_ed, "cfgreg", NULL),
};

static struct clk_lookup u8500_v1_clkregs[] = {
	/* Peripheral Cluster #1 */
	CLK(i2c4, "STM-I2C.4", NULL),
	CLK(spi3_v1, "spi3", NULL),
	CLK(msp1_v1, "msp1", NULL),
	CLK(msp1_v1, "MSP_I2S.1", NULL),

	/* Peripheral Cluster #2 */
	CLK(gpio1_v1, "gpioblock1", NULL),
	CLK(ssitx_v1, "ssitx", NULL),
	CLK(ssirx_v1, "ssirx", NULL),
	CLK(spi0_v1, "spi0", NULL),
	CLK(sdi3_v1, "sdi3", NULL),
	CLK(sdi1_v1, "sdi1", NULL),
	CLK(msp2_v1, "msp2", NULL),
	CLK(msp2_v1, "MSP_I2S.2", NULL),
	CLK(sdi4_v1, NULL, "EMMC"), /* remove */
	CLK(sdi4_v1, "sdi4", NULL),
	CLK(pwl_v1, "pwl", NULL),
	CLK(spi1_v1, "spi1", NULL),
	CLK(spi2_v1, "spi2", NULL),
	CLK(i2c3_v1, "STM-I2C.3", NULL),

	/* Peripheral Cluster #3 */
	CLK(ssp1_v1, "ssp1", NULL),
	CLK(ssp0_v1, "ssp0", NULL),

	/* Peripheral Cluster #5 */
	CLK(usb_ed, "musb_hdrc.0", "usb"),

	/* Peripheral Cluster #6 */
	CLK(mtu1_v1, "mtu1", NULL),
	CLK(mtu0_v1, "mtu0", NULL),
	CLK(cfgreg_v1, "cfgreg", NULL),
	CLK(hash1, "hash1", NULL),
	CLK(unipro_v1, "unipro", NULL),
	CLK(rng_v1, "rng", NULL),

	/*
	 * PRCMU level clock gating
	 */

	/* Bank 0 */
	CLK(msp1clk, "MSP1", NULL),
	CLK(uniproclk, "uniproclk", NULL),
	CLK(sspclk, "SSP", NULL),
	CLK(dsialtclk, "dsialt", NULL),

	/* Bank 1 */
	CLK(rngclk, "rng", NULL),
	CLK(uiccclk, "uicc", NULL),
};

static void clk_register(struct clk *clk);

static void clks_register(struct clk_lookup *clks, size_t num)
{
	int i;

	for (i = 0; i < num; i++) {
		clkdev_add(&clks[i]);
		clk_register(clks[i].clk);
	}
}

static int __init clk_init(void)
{
	if (u8500_is_earlydrop()) {
		clk_prcmu_ops.enable = clk_prcmu_ed_enable;
		clk_prcmu_ops.disable = clk_prcmu_ed_disable;
	}

	clks_register(u8500_common_clkregs, ARRAY_SIZE(u8500_common_clkregs));

	if (u8500_is_earlydrop())
		clks_register(u8500_ed_clkregs, ARRAY_SIZE(u8500_ed_clkregs));
	else
		clks_register(u8500_v1_clkregs, ARRAY_SIZE(u8500_v1_clkregs));

	return 0;
}
arch_initcall(clk_init);

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>

static LIST_HEAD(clocks);
static DEFINE_MUTEX(clocks_mutex);

static void clk_register(struct clk *clk)
{
	mutex_lock(&clocks_mutex);

	/* Ignore duplicate clocks */
	if (clk->list.prev != NULL || clk->list.next != NULL) {
		mutex_unlock(&clocks_mutex);
		return;
	}

	list_add(&clk->list, &clocks);
	mutex_unlock(&clocks_mutex);
}

/*
 * The following makes it possible to view the status (especially reference
 * count and reset status) for the clocks in the platform by looking into the
 * special file <debugfs>/u8500_clocks
 */
static void *u8500_clocks_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

static void *u8500_clocks_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void u8500_clocks_stop(struct seq_file *m, void *v)
{
}

static int u8500_clocks_show(struct seq_file *m, void *v)
{
	struct clk *clk;

	list_for_each_entry(clk, &clocks, list) {
		struct clk *child;
		bool first = true;

		seq_printf(m, "%-11s %d", clk->name, clk->enabled);

		list_for_each_entry(child, &clocks, list) {
			if (!child->enabled)
				continue;

			if (child->parent_cluster == clk ||
					child->parent_periph == clk) {
				if (first) {
					first = false;
					seq_printf(m, "\t[active children:");
				}

				seq_printf(m, " %s", child->name);
			}
		}

		seq_printf(m, "%s\n", first ? "" : "]");
	}

	return 0;
}

static const struct seq_operations u8500_clocks_op = {
	.start	= u8500_clocks_start,
	.next	= u8500_clocks_next,
	.stop	= u8500_clocks_stop,
	.show	= u8500_clocks_show
};

static int u8500_clocks_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &u8500_clocks_op);
}

static const struct file_operations u8500_clocks_operations = {
	.open           = u8500_clocks_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = seq_release,
};

static int __init init_clk_read_debugfs(void)
{
	/* Expose a simple debugfs interface to view all clocks */
	(void) debugfs_create_file("u8500_clocks", S_IFREG | S_IRUGO,
				   NULL, NULL, &u8500_clocks_operations);
	return 0;
}
/*
 * This needs to come in after the arch_initcall() for the
 * overall clocks, because debugfs is not available until
 * the subsystems come up.
 */
module_init(init_clk_read_debugfs);
#else
static void clk_register(struct clk *clk) { }
#endif
