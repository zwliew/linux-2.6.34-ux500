/*
 *  Copyright (C) 2009 ST-Ericsson SA
 *  Copyright (C) 2009 STMicroelectronics
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
#include <linux/io.h>
#include <linux/mutex.h>

#include <asm/clkdev.h>

#include <mach/hardware.h>
#include "clock.h"

#define PRCC_PCKEN		0x00
#define PRCC_PCKDIS		0x04
#define PRCC_KCKEN		0x08
#define PRCC_KCKDIS		0x0C

#define PRCM_YYCLKEN0_MGT_SET	0x510
#define PRCM_YYCLKEN1_MGT_SET	0x514
#define PRCM_YYCLKEN0_MGT_CLR	0x518
#define PRCM_YYCLKEN1_MGT_CLR	0x51C
#define PRCM_YYCLKEN0_MGT_VAL	0x520
#define PRCM_YYCLKEN1_MGT_VAL	0x524

#define PRCM_SVAMMDSPCLK_MGT	0x008
#define PRCM_SIAMMDSPCLK_MGT	0x00C
#define PRCM_SGACLK_MGT		0x014
#define PRCM_UARTCLK_MGT	0x018
#define PRCM_MSP02CLK_MGT	0x01C
#define PRCM_MSP1CLK_MGT	0x288
#define PRCM_I2CCLK_MGT		0x020
#define PRCM_SDMMCCLK_MGT	0x024
#define PRCM_SLIMCLK_MGT	0x028
#define PRCM_PER1CLK_MGT	0x02C
#define PRCM_PER2CLK_MGT	0x030
#define PRCM_PER3CLK_MGT	0x034
#define PRCM_PER5CLK_MGT	0x038
#define PRCM_PER6CLK_MGT	0x03C
#define PRCM_PER7CLK_MGT	0x040
#define PRCM_LCDCLK_MGT		0x044
#define PRCM_BMLCLK_MGT		0x04C
#define PRCM_HSITXCLK_MGT	0x050
#define PRCM_HSIRXCLK_MGT	0x054
#define PRCM_HDMICLK_MGT	0x058
#define PRCM_APEATCLK_MGT	0x05C
#define PRCM_APETRACECLK_MGT	0x060
#define PRCM_MCDECLK_MGT	0x064
#define PRCM_IPI2CCLK_MGT	0x068
#define PRCM_DSIALTCLK_MGT	0x06C
#define PRCM_DMACLK_MGT		0x074
#define PRCM_B2R2CLK_MGT	0x078
#define PRCM_TVCLK_MGT		0x07C
#define PRCM_UNIPROCLK_MGT	0x278
#define PRCM_SSPCLK_MGT		0x280
#define PRCM_RNGCLK_MGT		0x284
#define PRCM_UICCCLK_MGT	0x27C

#define PRCM_MGT_ENABLE		(1 << 8)

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

		if (clk->clk_src != NULL) {
			if (clk->clk_src->is_clk_src == 1) {
				struct clk *clk_pll_src = (struct clk *)clk->clk_src;
				if (clk_pll_src->enabled == 0) {
					clk_pll_src->ops->enable(clk_pll_src);
				}
				clk_pll_src->enabled++;
			}
		}
	}
}

int clk_enable(struct clk *clk)
{
	unsigned long flags;

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

		if (clk->clk_src != NULL) {
			if (clk->clk_src->is_clk_src == 1) {
				struct clk *clk_pll_src = (struct clk *)clk->clk_src;
				clk_pll_src->enabled--;
				if (clk_pll_src->enabled == 0) {
					clk_pll_src->ops->disable(clk_pll_src);
				}
			}
		}
	}
}

void clk_disable(struct clk *clk)
{
	unsigned long flags;

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
	if (!rate) {
		if (clk->parent_periph)
			rate = clk_get_rate(clk->parent_periph);
		else if (clk->parent_cluster)
			rate = clk_get_rate(clk->parent_cluster);
	}

	return rate;
}
EXPORT_SYMBOL(clk_get_rate);

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	/* TODO */
	return rate;
}
EXPORT_SYMBOL(clk_round_rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	clk->rate = rate;
	return 0;
}
EXPORT_SYMBOL(clk_set_rate);

static void clk_prcmu_enable(struct clk *clk)
{
	void __iomem *cg_set_reg = __io_address(U8500_PRCMU_BASE)
				   + PRCM_YYCLKEN0_MGT_SET + clk->prcmu_cg_off;

	writel(1 << clk->prcmu_cg_bit, cg_set_reg);
}

static void clk_prcmu_disable(struct clk *clk)
{
	void __iomem *cg_clr_reg = __io_address(U8500_PRCMU_BASE)
				   + PRCM_YYCLKEN0_MGT_CLR + clk->prcmu_cg_off;

	writel(1 << clk->prcmu_cg_bit, cg_clr_reg);
}

/* ED doesn't have the combined set/clr registers */
static void clk_prcmu_ed_enable(struct clk *clk)
{
	void __iomem *addr = __io_address(U8500_PRCMU_BASE)
			     + clk->prcmu_cg_mgt;

	writel(readl(addr) | PRCM_MGT_ENABLE, addr);
}

static void clk_prcmu_ed_disable(struct clk *clk)
{
	void __iomem *addr = __io_address(U8500_PRCMU_BASE)
			     + clk->prcmu_cg_mgt;

	writel(readl(addr) & ~PRCM_MGT_ENABLE, addr);
}

static struct clkops clk_prcmu_ops = {
	.enable = clk_prcmu_enable,
	.disable = clk_prcmu_disable,
};

static void clk_pllsrc_enable(struct clk *clk)
{
	/* To enable pll */
}

static void clk_pllsrc_disable(struct clk *clk)
{
	/* To disable pll */
}

static struct clkops clk_pll_ops = {
	.enable = clk_pllsrc_enable,
	.disable = clk_pllsrc_disable,
};

static unsigned int clkrst_base[] = {
	[1] = U8500_CLKRST1_BASE,
	[2] = U8500_CLKRST2_BASE,
	[3] = U8500_CLKRST3_BASE,
	[5] = U8500_CLKRST5_BASE,
	[6] = U8500_CLKRST6_BASE,
	[7] = U8500_CLKRST7_BASE_ED,
};

static void clk_prcc_enable(struct clk *clk)
{
	void __iomem *addr = __io_address(clkrst_base[clk->cluster]);

	if (clk->prcc_kernel != -1)
		writel(1 << clk->prcc_kernel, addr + PRCC_KCKEN);

	if (clk->prcc_bus != -1)
		writel(1 << clk->prcc_bus, addr + PRCC_PCKEN);
}

static void clk_prcc_disable(struct clk *clk)
{
	void __iomem *addr = __io_address(clkrst_base[clk->cluster]);

	if (clk->prcc_bus != -1)
		writel(1 << clk->prcc_bus, addr + PRCC_PCKDIS);

	if (clk->prcc_kernel != -1)
		writel(1 << clk->prcc_kernel, addr + PRCC_KCKDIS);
}

static struct clkops clk_prcc_ops = {
	.enable = clk_prcc_enable,
	.disable = clk_prcc_disable,
};

static struct clk clk_32khz = {
	.rate = 32000,
};

/* CLOCK Source Structures */
static DEFINE_CLK_SRC_PLL(soc0_pll);
static DEFINE_CLK_SRC_PLL(soc1_pll);
static DEFINE_CLK_SRC_PLL(ddr_pll);
static DEFINE_CLK_SRC_MAIN(ulp38m4);
static DEFINE_CLK_SRC_MAIN(sysclk);
static DEFINE_CLK_SRC_MAIN(clk32k);

/*
 * PRCMU level clock gating
 */

/* Bank 0 */
static DEFINE_PRCMU_CLK(svaclk,		0x0, 2, SVAMMDSPCLK);
static DEFINE_PRCMU_CLK(siaclk,		0x0, 3, SIAMMDSPCLK);
static DEFINE_PRCMU_CLK(sgaclk,		0x0, 4, SGACLK);
static DEFINE_PRCMU_CLK_RATE(uartclk,	0x0, 5, UARTCLK, 38400000);
static DEFINE_PRCMU_CLK(msp02clk,	0x0, 6, MSP02CLK);
static DEFINE_PRCMU_CLK(msp1clk,	0x0, 7, MSP1CLK); /* v1 */
static DEFINE_PRCMU_CLK_RATE(i2cclk,	0x0, 8, I2CCLK, 24000000);
static DEFINE_PRCMU_CLK_RATE(sdmmcclk,	0x0, 9, SDMMCCLK, 50000000);
static DEFINE_PRCMU_CLK(slimclk,	0x0, 10, SLIMCLK);
static DEFINE_PRCMU_CLK(per1clk,	0x0, 11, PER1CLK);
static DEFINE_PRCMU_CLK(per2clk,	0x0, 12, PER2CLK);
static DEFINE_PRCMU_CLK(per3clk,	0x0, 13, PER3CLK);
static DEFINE_PRCMU_CLK(per5clk,	0x0, 14, PER5CLK);
static DEFINE_PRCMU_CLK_RATE(per6clk,	0x0, 15, PER6CLK, 133330000);
static DEFINE_PRCMU_CLK_RATE(per7clk,	0x0, 16, PER7CLK, 100000000);
static DEFINE_PRCMU_CLK(lcdclk,		0x0, 17, LCDCLK);
static DEFINE_PRCMU_CLK(bmlclk,		0x0, 18, BMLCLK);
static DEFINE_PRCMU_CLK(hsitxclk,	0x0, 19, HSITXCLK);
static DEFINE_PRCMU_CLK(hsirxclk,	0x0, 20, HSIRXCLK);
static DEFINE_PRCMU_CLK(hdmiclk,	0x0, 21, HDMICLK);
static DEFINE_PRCMU_CLK(apeatclk,	0x0, 22, APEATCLK);
static DEFINE_PRCMU_CLK(apetraceclk,	0x0, 23, APETRACECLK);
static DEFINE_PRCMU_CLK(mcdeclk,	0x0, 24, MCDECLK);
static DEFINE_PRCMU_CLK(ipi2clk,	0x0, 25, IPI2CCLK);
static DEFINE_PRCMU_CLK(dsialtclk,	0x0, 26, DSIALTCLK); /* v1 */
static DEFINE_PRCMU_CLK(dmaclk,		0x0, 27, DMACLK);
static DEFINE_PRCMU_CLK(b2r2clk,	0x0, 28, B2R2CLK);
static DEFINE_PRCMU_CLK(tvclk,		0x0, 29, TVCLK);
static DEFINE_PRCMU_CLK(uniproclk,	0x0, 30, UNIPROCLK); /* v1 */
static DEFINE_PRCMU_CLK_RATE(sspclk,	0x0, 31, SSPCLK, 48000000); /* v1 */

/* Bank 1 */
static DEFINE_PRCMU_CLK(rngclk,		0x4, 0, RNGCLK); /* v1 */
static DEFINE_PRCMU_CLK(uiccclk,	0x4, 1, UICCCLK); /* v1 */

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
	CLK(gpio0,	"gpio.0",	NULL),
	CLK(gpio0,	"gpio.1",	NULL),
	CLK(gpio0,	"gpioblock0",	NULL),
	CLK(slimbus0,	"slimbus0",	NULL),
	CLK(i2c2, "nmk-i2c.2", NULL),
	CLK(sdi0,	"sdi0",		NULL),
	CLK(msp0,	"msp0",		NULL),
	CLK(msp0,	"MSP_I2S.0",	NULL),
	CLK(i2c1, "nmk-i2c.1", NULL),
	CLK(uart1,	"uart1",	NULL),
	CLK(uart0,	"uart0",	NULL),

	/* Peripheral Cluster #3 */
	CLK(gpio2,	"gpio.2",	NULL),
	CLK(gpio2,	"gpio.3",	NULL),
	CLK(gpio2,	"gpio.4",	NULL),
	CLK(gpio2,	"gpio.5",	NULL),
	CLK(gpio2,	"gpioblock2",	NULL),
	CLK(sdi5,	"sdi5",		NULL),
	CLK(uart2,	"uart2",	NULL),
	CLK(ske,	"ske",		NULL),
	CLK(sdi2,	"sdi2",		NULL),
	CLK(i2c0, "nmk-i2c.0", NULL),
	CLK(fsmc,	"fsmc",		NULL),

	/* Peripheral Cluster #5 */
	CLK(gpio3,	"gpio.8",	NULL),
	CLK(gpio3,	"gpioblock3",	NULL),

	/* Peripheral Cluster #6 */
	CLK(hash1,	"hash1",	NULL),
	CLK(pka,	"pka",		NULL),
	CLK(hash0,	"hash0",	NULL),
	CLK(cryp0,	"cryp0",	NULL),

	/* PRCMU level clock gating */

	/* Bank 0 */
	CLK(svaclk,	"sva",		NULL),
	CLK(siaclk,	"sia",		NULL),
	CLK(sgaclk,	"sga",		NULL),
	CLK(uartclk,	"UART",		NULL),
	CLK(msp02clk,	"MSP02",	NULL),
	CLK(i2cclk,	"I2C",		NULL),
	CLK(sdmmcclk,	"sdmmc",	NULL),
	CLK(slimclk,	"slim",		NULL),
	CLK(per1clk,	"PERIPH1",	NULL),
	CLK(per2clk,	"PERIPH2",	NULL),
	CLK(per3clk,	"PERIPH3",	NULL),
	CLK(per5clk,	"PERIPH5",	NULL),
	CLK(per6clk,	"PERIPH6",	NULL),
	CLK(per7clk,	"PERIPH7",	NULL),
	CLK(lcdclk,	"lcd",		NULL),
	CLK(bmlclk,	"bml",		NULL),
	CLK(hsitxclk,	"stm-hsi.0",	NULL),
	CLK(hsirxclk,	"stm-hsi.1",	NULL),
	CLK(hdmiclk,	"hdmi",		NULL),
	CLK(apeatclk,	"apeat",	NULL),
	CLK(apetraceclk,	"apetrace",	NULL),
	CLK(mcdeclk,	"mcde",		NULL),
	CLK(ipi2clk,	"ipi2",		NULL),
	CLK(dmaclk,	"STM-DMA.0",	NULL),
	CLK(b2r2clk,	"b2r2",		NULL),
	CLK(b2r2clk,	"b2r2_bus",	NULL),
	CLK(tvclk,	"tv",		NULL),

	/* With device names */
	CLK(mcdeclk,	"U8500-MCDE.0",	"mcde"),
	CLK(hdmiclk,	"U8500-MCDE.0",	"hdmi"),
	CLK(tvclk,	"U8500-MCDE.0",	"tv"),
	CLK(lcdclk,	"U8500-MCDE.0",	"lcd"),
	CLK(mcdeclk,	"U8500-MCDE.1",	"mcde"),
	CLK(hdmiclk,	"U8500-MCDE.1",	"hdmi"),
	CLK(tvclk,	"U8500-MCDE.1",	"tv"),
	CLK(lcdclk,	"U8500-MCDE.1",	"lcd"),
	CLK(mcdeclk,	"U8500-MCDE.2",	"mcde"),
	CLK(hdmiclk,	"U8500-MCDE.2",	"hdmi"),
	CLK(tvclk,	"U8500-MCDE.2",	"tv"),
	CLK(lcdclk,	"U8500-MCDE.2",	"lcd"),
	CLK(mcdeclk,	"U8500-MCDE.3",	"mcde"),
	CLK(hdmiclk,	"U8500-MCDE.3",	"hdmi"),
	CLK(tvclk,	"U8500-MCDE.3",	"tv"),
	CLK(lcdclk,	"U8500-MCDE.3",	"lcd"),
	CLK(b2r2clk,	"U8500-B2R2.0",	NULL),

	/* Register the clock sources */
	CLK(soc0_pll, NULL,	"soc0_pll"),
	CLK(soc1_pll,	NULL, "soc1_pll"),
	CLK(ddr_pll, NULL,	"ddr_pll"),
	CLK(ulp38m4, NULL,	"ulp38m4"),
	CLK(sysclk,	NULL, "sysclk"),
	CLK(clk32k,	NULL, "clk32k"),
};

static struct clk_lookup u8500_ed_clkregs[] = {
	/* Peripheral Cluster #1 */
	CLK(spi3_ed,	"spi3",		NULL),
	CLK(msp1_ed,	"msp1",		NULL),
	CLK(msp1_ed,	"MSP_I2S.1",	NULL),

	/* Peripheral Cluster #2 */
	CLK(gpio1_ed,	"gpio.6",	NULL),
	CLK(gpio1_ed,	"gpio.7",	NULL),
	CLK(gpio1_ed,	"gpioblock1",	NULL),
	CLK(ssitx_ed,	"ssitx",	NULL),
	CLK(ssirx_ed,	"ssirx",	NULL),
	CLK(spi0_ed,	"spi0",		NULL),
	CLK(sdi3_ed,	"sdi3",		NULL),
	CLK(sdi1_ed,	"sdi1",		NULL),
	CLK(msp2_ed,	"msp2",		NULL),
	CLK(msp2_ed,	"MSP_I2S.2",	NULL),
	CLK(sdi4_ed,	"sdi4",		NULL),
	CLK(pwl_ed,	"pwl",		NULL),
	CLK(spi1_ed,	"spi1",		NULL),
	CLK(spi2_ed,	"spi2",		NULL),
	CLK(i2c3_ed, "nmk-i2c.3", NULL),

	/* Peripheral Cluster #3 */
	CLK(ssp1_ed,	"ssp1",		NULL),
	CLK(ssp0_ed,	"ssp0",		NULL),

	/* Peripheral Cluster #5 */
	CLK(usb_ed,	"musb_hdrc.0",	"usb"),

	/* Peripheral Cluster #6 */
	CLK(dmc_ed,	"dmc",		NULL),
	CLK(cryp1_ed,	"cryp1",	NULL),
	CLK(rng_ed,	"rng",		NULL),

	/* Peripheral Cluster #7 */
	CLK(tzpc0_ed,	"tzpc0",	NULL),
	CLK(mtu1_ed,	"mtu1",		NULL),
	CLK(mtu0_ed,	"mtu0",		NULL),
	CLK(wdg_ed,	"wdg",		NULL),
	CLK(cfgreg_ed,	"cfgreg",	NULL),
};

static struct clk_lookup u8500_v1_clkregs[] = {
	/* Peripheral Cluster #1 */
	CLK(i2c4, "nmk-i2c.4", NULL),
	CLK(spi3_v1,	"spi3",		NULL),
	CLK(msp1_v1,	"msp1",		NULL),
	CLK(msp1_v1,	"MSP_I2S.1",	NULL),

	/* Peripheral Cluster #2 */
	CLK(gpio1_v1,	"gpio.6",	NULL),
	CLK(gpio1_v1,	"gpio.7",	NULL),
	CLK(gpio1_v1,	"gpioblock1",	NULL),
	CLK(ssitx_v1,	"ssitx",	NULL),
	CLK(ssirx_v1,	"ssirx",	NULL),
	CLK(spi0_v1,	"spi0",		NULL),
	CLK(sdi3_v1,	"sdi3",		NULL),
	CLK(sdi1_v1,	"sdi1",		NULL),
	CLK(msp2_v1,	"msp2",		NULL),
	CLK(msp2_v1,	"MSP_I2S.2",	NULL),
	CLK(sdi4_v1,	"sdi4",		NULL),
	CLK(pwl_v1,	"pwl",		NULL),
	CLK(spi1_v1,	"spi1",		NULL),
	CLK(spi2_v1,	"spi2",		NULL),
	CLK(i2c3_v1, "nmk-i2c.3", NULL),

	/* Peripheral Cluster #3 */
	CLK(ssp1_v1,	"ssp1",		NULL),
	CLK(ssp0_v1,	"ssp0",		NULL),

	/* Peripheral Cluster #5 */
	CLK(usb_v1,	"musb_hdrc.0",	"usb"),

	/* Peripheral Cluster #6 */
	CLK(mtu1_v1,	"mtu1",		NULL),
	CLK(mtu0_v1,	"mtu0",		NULL),
	CLK(cfgreg_v1,	"cfgreg",	NULL),
	CLK(hash1,	"hash1",	NULL),
	CLK(unipro_v1,	"unipro",	NULL),
	CLK(rng_v1,	"rng",		NULL),

	/* PRCMU level clock gating */

	/* Bank 0 */
	CLK(msp1clk,	"MSP1",		NULL),
	CLK(uniproclk,	"uniproclk",	NULL),
	CLK(sspclk,	"SSP",		NULL),
	CLK(dsialtclk,	"dsialt",	NULL),

	/* Bank 1 */
	CLK(rngclk,	"rng",		NULL),
	CLK(uiccclk,	"uicc",		NULL),
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

/* these are the clocks which are default from the bootloader */
static const char *u8500_boot_clk[] = {
	"uart0",
	"uart1",
	"uart2",
	"gpioblock0",
	"gpioblock1",
	"gpioblock2",
	"gpioblock3",
	"mtu0",
	"mtu1",
	"ssp0",
	"ssp1",
	"spi0",
	"spi1",
	"spi2",
	"spi3",
	"msp0",
	"msp1",
	"msp2",
	"nmk-i2c.0",
	"nmk-i2c.1",
	"nmk-i2c.2",
	"nmk-i2c.3",
	"nmk-i2c.4",
};

struct clk *boot_clks[ARRAY_SIZE(u8500_boot_clk)];

/* we disable a majority of peripherals enabled by default
 * but without drivers
 */
static int __init u8500_boot_clk_disable(void)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(u8500_boot_clk); i++) {
		if (!boot_clks[i])
			continue;

		clk_disable(boot_clks[i]);
		clk_put(boot_clks[i]);
	}

	return 0;
}
late_initcall_sync(u8500_boot_clk_disable);

static void u8500_amba_clk_enable(void)
{
	int i = 0;

	writel(~0x0  & ~(1 << 9), IO_ADDRESS(U8500_PER1_BASE + 0xF000 + 0x04));
	writel(~0x0, IO_ADDRESS(U8500_PER1_BASE + 0xF000 + 0x0C));

	writel(~0x0 & ~(1 << 11), IO_ADDRESS(U8500_PER2_BASE + 0xF000 + 0x04));
	writel(~0x0, IO_ADDRESS(U8500_PER2_BASE + 0xF000 + 0x0C));

	/*GPIO,UART2 are enabled for booting*/
	writel(0xBF, IO_ADDRESS(U8500_PER3_BASE + 0xF000 + 0x04));
	writel(~0x0 & ~(1 << 6), IO_ADDRESS(U8500_PER3_BASE + 0xF000 + 0x0C));

	for (i = 0; i < ARRAY_SIZE(u8500_boot_clk); i++) {
		boot_clks[i] = clk_get_sys(u8500_boot_clk[i], NULL);
		clk_enable(boot_clks[i]);
	}
}

int __init clk_init(void)
{
	if (cpu_is_u8500ed()) {
		clk_prcmu_ops.enable = clk_prcmu_ed_enable;
		clk_prcmu_ops.disable = clk_prcmu_ed_disable;
		clk_i2cclk.rate = 48000000;
	} else if (cpu_is_u8500v1()) {
		void __iomem *sdmmclkmgt = __io_address(U8500_PRCMU_BASE)
					   + PRCM_SDMMCCLK_MGT;
		unsigned int val;

		/* Switch SDMMCCLK to 52Mhz instead of 104Mhz */
		val = readl(sdmmclkmgt);
		val = (val & ~0x1f) | 16;
		writel(val, sdmmclkmgt);
	} else if (cpu_is_u5500()) {
		clk_prcmu_ops.enable = NULL;
		clk_prcmu_ops.disable = NULL;
		clk_prcc_ops.enable = NULL;
		clk_prcc_ops.disable = NULL;
	}

	clks_register(u8500_common_clkregs, ARRAY_SIZE(u8500_common_clkregs));

	if (cpu_is_u8500ed())
		clks_register(u8500_ed_clkregs, ARRAY_SIZE(u8500_ed_clkregs));
	else
		clks_register(u8500_v1_clkregs, ARRAY_SIZE(u8500_v1_clkregs));

	if (cpu_is_u8500() && !cpu_is_u8500ed())
		u8500_amba_clk_enable();
	return 0;
}

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>

static LIST_HEAD(clocks);
static DEFINE_MUTEX(clocks_mutex);

static void get_pll_clk_src(struct clk *clk, unsigned int value)
{
	if (value & PLL_SW_SOC0) {
		clk->clk_src = (struct clk *)&clk_soc0_pll;
		if (clk->enabled > 0)
			clk->clk_src->enabled++;
	} else if (value & PLL_SW_SOC1) {
		clk->clk_src = (struct clk *)&clk_soc1_pll;
		if (clk->enabled > 0)
			clk->clk_src->enabled++;
	} else if (value & PLL_SW_DDR) {
		clk->clk_src = (struct clk *)&clk_ddr_pll;
		if (clk->enabled > 0)
			clk->clk_src->enabled++;
	}
	return;
}

static void get_clk38_src(struct clk *clk, unsigned int value)
{
	if (value & CLK38_SRC) {
		clk->clk_src = (struct clk *)&clk_ulp38m4;
		if (clk->enabled > 0)
			clk->clk_src->enabled++;
	} else {
		clk->clk_src = (struct clk *)&clk_sysclk;
		if (clk->enabled > 0)
			clk->clk_src->enabled++;
	}
	return;
}

static void get_clk_src(struct clk *clk, unsigned int value)
{
	unsigned int mode_value;
	void __iomem *mode_base;

	if (value & CLK38) {
		get_clk38_src(clk, value);
	} else {
		mode_base =
		(void __iomem *)IO_ADDRESS(U8500_PRCMU_BASE + 0x0E8);
		mode_value = readl(mode_base);
		if (mode_value & MODE_CLK32KHZ) {
			clk->clk_src = (struct clk *)&clk_clk32k;
			if (clk->enabled > 0)
				clk->clk_src->enabled++;
		} else if (mode_value & MODE_CLK38_4MHZ) {
			get_clk38_src(clk, value);
		} else if (mode_value & MODE_PLL_CLK) {
			get_pll_clk_src(clk, value);
		} else
			printk(KERN_INFO "no clk src is set \n");
	}
	return;
}

/*
 * 	This function is called to update the clock tree
 *	It is called after each vape operating point change
 */
void update_clk_tree(void)
{
	struct clk *clk;
	unsigned int val;
	void __iomem *addr;

	/* Source clock structure enable count value initialized to zero */
	clk_clk32k.enabled = 0;
	clk_ulp38m4.enabled = 0;
	clk_sysclk.enabled = 0;
	clk_ddr_pll.enabled = 0;
	clk_soc0_pll.enabled = 0;
	clk_soc1_pll.enabled = 0;

	list_for_each_entry(clk, &clocks, list) {
		/* Figure out its clock source by reading the MODE, yyCLK38,
		* yyCLK38SRC and yyCLKPLLSW registers Update the yyCLK/clock
		* source relationship
		*/
		if (!clk->parent_periph && !clk->parent_cluster && !clk->is_clk_src) {
			addr = __io_address(U8500_PRCMU_BASE) + clk->prcmu_cg_mgt;
			val = readl(addr);
			get_clk_src(clk, val);
		}
	}
	return;
}
EXPORT_SYMBOL(update_clk_tree);

struct clk *clk_get_parent(struct clk *clk)
{
	struct clk *prcmu_clk;
	unsigned int val;
	void __iomem *addr;

	if (clk == NULL)
		return NULL;

	if (clk->is_clk_src == 1) {
		return NULL;
	} else if ((clk->parent_cluster != NULL) && (clk->parent_periph == NULL))	{
		prcmu_clk = (struct clk *) clk->parent_cluster;
	} else if ((clk->parent_cluster != NULL) && (clk->parent_periph != NULL)) {
		prcmu_clk = (struct clk *) clk->parent_periph;
	} else if ((clk->parent_cluster == NULL) && (clk->parent_periph == NULL)) {
		prcmu_clk = (struct clk *) clk;
	} else
		return NULL;

	addr = __io_address(U8500_PRCMU_BASE) + prcmu_clk->prcmu_cg_mgt;
	val = readl(addr);
	get_clk_src(prcmu_clk, val);
	return prcmu_clk->clk_src;
}
EXPORT_SYMBOL(clk_get_parent);

int clk_set_parent(struct clk *clk, struct clk *new_parent)
{
	unsigned int val;
	unsigned int mode_value;
	void __iomem *mode_base;
	void __iomem *addr;
	struct clk *prcmu_clk = NULL;
	int clked = 0;
	int enable_count = 0;
	int loop_count = 0;
	unsigned long flags;
	int ret = 0;

	/* Set the parent clock source to parent */
	if ((new_parent == NULL) || (clk == NULL)) {
			return -EINVAL;
	} else if ((clk->parent_cluster != NULL) && (clk->parent_periph == NULL)) {
			return -EINVAL;
	} else if ((clk->parent_cluster != NULL) && (clk->parent_periph != NULL)) {
			/* Has a peripheral parent register */
			prcmu_clk = (struct clk *) clk->parent_periph;
	} else if ((clk->parent_cluster == NULL) && (clk->parent_periph == NULL)) {
			/* Clk itself is a PRCMU clk */
			prcmu_clk = (struct clk *) clk;
	}

	/* Holding the lock so that val register is not modified in between */
	/* NOTE : Dont use printk() further in the code untill the lock is held */
	spin_lock_irqsave(&clocks_lock, flags);

	/* Read the value of the configuration register */
	addr = __io_address(U8500_PRCMU_BASE) + prcmu_clk->prcmu_cg_mgt;
	val = readl(addr);

	/* If the clock is enabled turn it down before re enabling */
	if (val & ENABLE_BIT) {
		clked = 1;
		enable_count = prcmu_clk->enabled;
		loop_count = enable_count;
		while (loop_count) {
			__clk_disable(prcmu_clk);
			loop_count -= 1;
		}
		val = readl(addr);
	}

	/* Get the current source clock if not known*/
	if (prcmu_clk->clk_src == NULL)
		get_clk_src(prcmu_clk, val);

	/* Consider the value in mode register */
	mode_base = (void __iomem *)IO_ADDRESS(U8500_PRCMU_BASE + 0x0E8);
	mode_value = readl(mode_base);

	/* Check which clock is requested to be the new source clock */
	if ((strcmp(new_parent->name, (&clk_ulp38m4)->name) == 0) || (strcmp(new_parent->name, (&clk_sysclk)->name)) == 0) {
		/* Override in the register since mode is not 38MHz */
		if (!(mode_value & MODE_CLK38_4MHZ))
			val = val | CLK38;

		/* Check which is the type of source clock to be configured */
		if (strcmp(new_parent->name, (&clk_ulp38m4)->name) == 0) {
			val = val | CLK38_SRC;
			prcmu_clk->clk_src = (struct clk *)&clk_ulp38m4;
		} else if (strcmp(new_parent->name, (&clk_sysclk)->name) == 0) {
			val = val & ~CLK38_SRC;
			prcmu_clk->clk_src = (struct clk *)&clk_sysclk;
		}
	} else if (strcmp(new_parent->name, (&clk_soc0_pll)->name) == 0) {
		/* Change to a different PLL source only if appropriate mode is set */
		if (mode_value & MODE_PLL_CLK) {
			/* If mode is appropriate then CLK38 bit should be 0 */
			if (val & CLK38)
				val = val & ~CLK38;
			/* Make the PLL bits to 0 */
			val = val & ~PLL_SELECT_BITS;
			/* Write with appropriate PLL bits */
			val = val | PLL_SW_SOC0;
			/* Change the clock source */
			prcmu_clk->clk_src = (struct clk *)&clk_soc0_pll;
		} else {
			/* Cannot switch to a differnt PLL source if it is not a PLL mode */
			ret = -EBUSY;
			goto out;
		}
	} else if (strcmp(new_parent->name, (&clk_ddr_pll)->name) == 0) {
		/* Change to a different PLL source only if appropriate mode is set */
		if (mode_value & MODE_PLL_CLK) {
			if (val & CLK38)
				val = val & ~CLK38;
			val = val & ~PLL_SELECT_BITS;
			val = val | PLL_SW_DDR;
			prcmu_clk->clk_src = (struct clk *)&clk_ddr_pll;
		} else {
			ret = -EBUSY;
			goto out;
		}
	} else if (strcmp(new_parent->name, (&clk_soc1_pll)->name) == 0) {
		/* Change to a different PLL source only if appropriate mode is set */
		if (mode_value & MODE_PLL_CLK) {
			if (val & CLK38)
				val = val & ~CLK38;
			val = val & ~PLL_SELECT_BITS;
			val = val | PLL_SW_SOC1;
			prcmu_clk->clk_src = (struct clk *)&clk_soc1_pll;
		} else {
			ret = -EBUSY;
			goto out;
		}
	} else {
			ret = -EINVAL;
			goto out;
	}

	/* write back the val value */
	writel(val, addr);
	ret = 0;

out:		if (clked) {
			/* Re-enable the clock if already was enabled */
			loop_count = enable_count;
			while (loop_count) {
				__clk_enable(prcmu_clk);
				loop_count -= 1;
			}
			clked = 0;
		}

		/* Release the spin lock */
		spin_unlock_irqrestore(&clocks_lock, flags);
		return ret;
}
EXPORT_SYMBOL(clk_set_parent);


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

	printk(KERN_INFO "Periph1 PCKEN : 0x%x\n",
		readl(IO_ADDRESS(U8500_PER1_BASE + 0xF000 + 0x10)));
	printk(KERN_INFO "Periph1 KCKEN : 0x%x\n",
		readl(IO_ADDRESS(U8500_PER1_BASE + 0xF000 + 0x14)));
	printk(KERN_INFO "Periph2 PCKEN : 0x%x\n",
		readl(IO_ADDRESS(U8500_PER2_BASE + 0xF000 + 0x10)));
	printk(KERN_INFO "Periph2 KCKEN : 0x%x\n",
		readl(IO_ADDRESS(U8500_PER2_BASE + 0xF000 + 0x14)));
	printk(KERN_INFO "Periph3 PCKEN : 0x%x\n",
		readl(IO_ADDRESS(U8500_PER3_BASE + 0xF000 + 0x10)));
	printk(KERN_INFO "Periph3 KCKEN : 0x%x\n",
		readl(IO_ADDRESS(U8500_PER3_BASE + 0xF000 + 0x14)));
	printk(KERN_INFO "Periph5 PCKEN : 0x%x\n",
		readl(IO_ADDRESS(U8500_PER5_BASE + 0x1F000 + 0x10)));
	printk(KERN_INFO "Periph5 KCKEN : 0x%x\n",
		readl(IO_ADDRESS(U8500_PER5_BASE + 0x1F000 + 0x14)));
	printk(KERN_INFO "Periph6 PCKEN : 0x%x\n",
		readl(IO_ADDRESS(U8500_PER6_BASE + 0xF000 + 0x10)));
	printk(KERN_INFO "Periph6 KCKEN : 0x%x\n",
		readl(IO_ADDRESS(U8500_PER6_BASE + 0xF000 + 0x14)));

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
