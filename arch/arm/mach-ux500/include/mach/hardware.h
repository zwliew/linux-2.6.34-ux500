/*
 * Copyright (C) 2009 ST-Ericsson.
 *
 * U8500 hardware definitions
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#define IO_BASE			0xF0000000	/* VA of IO  */
#define IO_SIZE			0x1FF00000	/* VA Size for IO */
#define IO_START		0x10100000	/* PA of IO */

/*
 * macro to get at IO space when running virtually
 */
#define IO_ADDRESS(x)		(((x) & 0x0fffffff) + (((x) >> 4) & 0x0f000000) + IO_BASE)
#define __io_address(n)		__io(IO_ADDRESS(n))
#define io_p2v(n)		__io_address(n)

#include <mach/db8500-regs.h>
#include <mach/db5500-regs.h>

#ifdef CONFIG_UX500_SOC_DB8500
#define UX500(periph)		U8500_##periph##_BASE
#elif defined(CONFIG_UX500_SOC_DB5500)
#define UX500(periph)		U5500_##periph##_BASE
#endif

#define UX500_BACKUPRAM0_BASE	UX500(BACKUPRAM0)
#define UX500_BACKUPRAM1_BASE	UX500(BACKUPRAM1)
#define UX500_B2R2_BASE		UX500(B2R2)

#define UX500_CLKRST1_BASE	UX500(CLKRST1)
#define UX500_CLKRST2_BASE	UX500(CLKRST2)
#define UX500_CLKRST3_BASE	UX500(CLKRST3)
#define UX500_CLKRST5_BASE	UX500(CLKRST5)
#define UX500_CLKRST6_BASE	UX500(CLKRST6)

#define UX500_DMA_BASE		UX500(DMA)
#define UX500_FSMC_BASE		UX500(FSMC)

#define UX500_GIC_CPU_BASE	UX500(GIC_CPU)
#define UX500_GIC_DIST_BASE	UX500(GIC_DIST)

#define UX500_I2C1_BASE		UX500(I2C1)
#define UX500_I2C2_BASE		UX500(I2C2)
#define UX500_I2C3_BASE		UX500(I2C3)

#define UX500_L2CC_BASE		UX500(L2CC)
#define UX500_MCDE_BASE		UX500(MCDE)
#define UX500_MTU0_BASE		UX500(MTU0)
#define UX500_MTU1_BASE		UX500(MTU1)
#define UX500_PRCMU_BASE	UX500(PRCMU)

#define UX500_RNG_BASE		UX500(RNG)
#define UX500_RTC_BASE		UX500(RTC)

#define UX500_SCU_BASE		UX500(SCU)

#define UX500_SDI0_BASE		UX500(SDI0)
#define UX500_SDI1_BASE		UX500(SDI1)
#define UX500_SDI2_BASE		UX500(SDI2)
#define UX500_SDI3_BASE		UX500(SDI3)
#define UX500_SDI4_BASE		UX500(SDI4)

#define UX500_SPI0_BASE		UX500(SPI0)
#define UX500_SPI1_BASE		UX500(SPI1)
#define UX500_SPI2_BASE		UX500(SPI2)
#define UX500_SPI3_BASE		UX500(SPI3)

#define UX500_SIA_BASE		UX500(SIA)
#define UX500_SVA_BASE		UX500(SVA)

#define UX500_TWD_BASE		UX500(TWD)

#define UX500_UART0_BASE	UX500(UART0)
#define UX500_UART1_BASE	UX500(UART1)
#define UX500_UART2_BASE	UX500(UART2)

#define UX500_USBOTG_BASE	UX500(USBOTG)

#define U8500_ESRAM_BASE	0x40000000
#define U8500_ESRAM_DMA_LCLA_OFFSET	0x80000
#define U8500_ESRAM_DMA_LCPA_OFFSET	0x84000

#define U8500_DMA_LCLA_BASE (U8500_ESRAM_BASE + U8500_ESRAM_DMA_LCLA_OFFSET)
#define U8500_DMA_LCPA_BASE (U8500_ESRAM_BASE + U8500_ESRAM_DMA_LCPA_OFFSET)

/* SSP specific declaration */
#define SSP_PER_ID                      0x01080022
#define SSP_PER_MASK                    0x0fffffff

/* SSP specific declaration */
#define SPI_PER_ID                      0x00080023
#define SPI_PER_MASK                    0x0fffffff

/* MSP specific declaration */
#define MSP_PER_ID			0x00280021
#define MSP_PER_MASK			0x00ffffff

/* DMA specific declaration */
#define DMA_PER_ID			0x8A280080
#define DMA_PER_MASK			0xffffffff

#define GPIO_TOTAL_PINS                 267
#define GPIO_PER_ID                     0x1f380060
#define GPIO_PER_MASK                   0xffffffff

/* RTC specific declaration */
#define RTC_PER_ID                      0x00280031
#define RTC_PER_MASK                    0x00ffffff

/*
 * FIFO offsets for IPs
 */
#define I2C_TX_REG_OFFSET	(0x10)
#define I2C_RX_REG_OFFSET	(0x18)
#define UART_TX_RX_REG_OFFSET	(0)
#define MSP_TX_RX_REG_OFFSET	(0)
#define SSP_TX_RX_REG_OFFSET	(0x8)
#define SPI_TX_RX_REG_OFFSET	(0x8)
#define SD_MMC_TX_RX_REG_OFFSET (0x80)

#define MSP_0_CONTROLLER 1
#define MSP_1_CONTROLLER 2
#define MSP_2_CONTROLLER 3

#define SSP_0_CONTROLLER 4
#define SSP_1_CONTROLLER 5

#define SPI023_0_CONTROLLER 6
#define SPI023_1_CONTROLLER 7
#define SPI023_2_CONTROLLER 8
#define SPI023_3_CONTROLLER 9

/* MSP related board specific declaration************************/

#define MSP_DATA_DELAY       MSP_DELAY_0
#define MSP_TX_CLOCK_EDGE    MSP_FALLING_EDGE
#define MSP_RX_CLOCK_EDGE    MSP_FALLING_EDGE
#define NUM_MSP_CONTROLLER 3

/* I2C configuration
 *  *  *
 *   *   */
#define I2C0_LP_OWNADDR 0x31
#define I2C1_LP_OWNADDR 0x60
#define I2C2_LP_OWNADDR 0x70
#define I2C3_LP_OWNADDR 0x80
#define I2C4_LP_OWNADDR 0x90

/* SDMMC specific declarations */
#define SDI_PER_ID		0x00480180
#define SDI_PER_MASK		0x00ffffff
/* B2R2 clock management register */
#define PRCM_B2R2CLK_MGT_REG	0x80157078 /** B2R2 clock selection */

#include <mach/mcde-base.h>

#ifndef __ASSEMBLY__

#include <asm/cputype.h>

/* TODO: dynamic detection */
static inline bool cpu_is_u8500(void)
{
#ifdef CONFIG_UX500_SOC_DB8500
	return 1;
#else
	return 0;
#endif
}

static inline bool cpu_is_u8500ed(void)
{
#ifdef CONFIG_MACH_U8500_SIMULATOR
	/*
	 * SVP8500v1 unfortunately does not implement the changed MIDR register
	 * on v1, but instead maintains the old ED revision.
	 *
	 * So we hardcode this assuming that only SVP8500v1 is supported.  If
	 * SVP8500ed is required, another Kconfig option will have to be added.
	 */
	return 0;
#else
	return cpu_is_u8500() && ((read_cpuid_id() & 15) == 0);
#endif
}

static inline bool cpu_is_u8500v1(void)
{
#ifdef CONFIG_MACH_U8500_SIMULATOR
	/* See comment in cpu_is_u8500ed() */
	return cpu_is_u8500();
#else
	return cpu_is_u8500() && ((read_cpuid_id() & 15) == 1);
#endif
}

static inline bool cpu_is_u5500(void)
{
#ifdef CONFIG_UX500_SOC_DB5500
	return 1;
#else
	return 0;
#endif
}

/* Deprecated, don't use in new code.  Just call cpu_is_u8500ed() directly. */
static inline int u8500_is_earlydrop(void)
{
	return cpu_is_u8500ed();
}

#endif

#endif				/* __ASM_ARCH_HARDWARE_H */
