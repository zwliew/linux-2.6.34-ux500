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

#ifndef __ASSEMBLER__
extern int u8500_is_earlydrop(void);
#endif

/*
 * Base address definitions for U8500 Onchip IPs. All the
 * peripherals are contained in a single 1 Mbyte region, with
 * AHB peripherals at the bottom and APB peripherals at the
 * top of the region. PER stands for PERIPHERAL region which
 * itself divided into sub regions.
 */
#define U8500_PER3_BASE		0x80000000
#define U8500_STM_BASE		0x80100000
#define U8500_STM_REG_BASE	(U8500_STM_BASE+0xF000)
#define U8500_PER2_BASE		0x80110000
#define U8500_PER1_BASE		0x80120000
#define U8500_B2R2_BASE  	0x80130000
#define U8500_HSEM_BASE		0x80140000
#define U8500_PER4_BASE		0x80150000

#define U8500_PER6_BASE		0xa03c0000
#define U8500_PER5_BASE		0xa03e0000
#define U8500_PER7_BASE_ED	0xa03d0000

#define U8500_SVA_BASE		0xa0100000
#define U8500_SIA_BASE		0xa0200000

#define U8500_SGA_BASE		0xa0300000
#define U8500_MCDE_BASE		0xa0350000
#define U8500_DMA_BASE_ED	0xa0362000
#define U8500_DMA_BASE_V1	0x801C0000

#define U8500_SCU_BASE		0xa0410000
#define U8500_GIC_CPU_BASE	0xa0410100
#define U8500_TWD_BASE		0xa0410600
#define U8500_GIC_DIST_BASE	0xa0411000
#define U8500_L2CC_BASE		0xa0412000

#define U8500_TWD_SIZE		0x100

/* gpio's are scattered in the memory map */
#define U8500_GPIO_BASE		(U8500_PER1_BASE + 0xE000)
#define U8500_GPIO0_BASE	(U8500_PER1_BASE + 0xE000)
#define U8500_GPIO1_BASE	(U8500_PER3_BASE + 0xE000)
#define U8500_GPIO2_BASE	(U8500_PER2_BASE + 0xE000)
#define U8500_GPIO3_BASE	(U8500_PER5_BASE + 0x1E000)

#define GPIO_BANK0_BASE		(U8500_PER1_BASE + 0xE000)
#define GPIO_BANK1_BASE		(U8500_PER1_BASE + 0xE000 + 0x80)

#define GPIO_BANK2_BASE		(U8500_PER3_BASE + 0xE000)
#define GPIO_BANK3_BASE		(U8500_PER3_BASE + 0xE000 + 0x80)
#define GPIO_BANK4_BASE		(U8500_PER3_BASE + 0xE000 + 0x100)
#define GPIO_BANK5_BASE		(U8500_PER3_BASE + 0xE000 + 0x180)

#define GPIO_BANK6_BASE		(U8500_PER2_BASE + 0xE000)
#define GPIO_BANK7_BASE		(U8500_PER2_BASE + 0xE000 + 0x80)

#define GPIO_BANK8_BASE		(U8500_PER5_BASE + 0x1E000)

/* per7 base addressess */
#define U8500_CR_BASE_ED	(U8500_PER7_BASE_ED + 0x8000)
#define U8500_MTU0_BASE_ED	(U8500_PER7_BASE_ED + 0xa000)
#define U8500_MTU1_BASE_ED	(U8500_PER7_BASE_ED + 0xb000)
#define U8500_TZPC0_BASE_ED	(U8500_PER7_BASE_ED + 0xc000)
#define U8500_CLKRST7_BASE	(U8500_PER7_BASE_ED + 0xf000)

/* per6 base addressess */
#define U8500_RNG_BASE		(U8500_PER6_BASE + 0x0000)
#define U8500_PKA_BASE		(U8500_PER6_BASE + 0x1000)
#define U8500_PKAM_BASE		(U8500_PER6_BASE + 0x2000)
#define U8500_CRYPTO0_BASE	(U8500_PER6_BASE + 0xa000)
#define U8500_CRYPTO1_BASE	(U8500_PER6_BASE + 0xb000)
#define U8500_CLKRST6_BASE	(U8500_PER6_BASE + 0xf000)

#define U8500_MTU0_BASE_V1	(U8500_PER6_BASE + 0x6000)
#define U8500_MTU1_BASE_V1	(U8500_PER6_BASE + 0x7000)
#define U8500_CR_BASE_V1	(U8500_PER6_BASE + 0x8000)

/* per5 base addressess */
#define U8500_USBOTG_BASE	(U8500_PER5_BASE + 0x00000)
#define U8500_CLKRST5_BASE	(U8500_PER5_BASE + 0x1f000)

/* per4 base addressess */
#define U8500_BACKUPRAM0_BASE	(U8500_PER4_BASE + 0x00000)
#define U8500_BACKUPRAM1_BASE	(U8500_PER4_BASE + 0x01000)
#define U8500_RTT0_BASE		(U8500_PER4_BASE + 0x02000)
#define U8500_RTT1_BASE		(U8500_PER4_BASE + 0x03000)
#define U8500_RTC_BASE		(U8500_PER4_BASE + 0x04000)
#define U8500_SCR_BASE		(U8500_PER4_BASE + 0x05000)
#define U8500_DMC_BASE		(U8500_PER4_BASE + 0x06000)
#define U8500_PRCMU_BASE	(U8500_PER4_BASE + 0x07000)
#define U8500_PRCMU_TCDM_BASE	(U8500_PER4_BASE + 0x0f000)

/* per3 base addressess */
#define U8500_FSMC_BASE		(U8500_PER3_BASE + 0x0000)
#define U8500_SSP0_BASE		(U8500_PER3_BASE + 0x2000)
#define U8500_SSP1_BASE		(U8500_PER3_BASE + 0x3000)
#define U8500_I2C0_BASE		(U8500_PER3_BASE + 0x4000)
#define U8500_SDI2_BASE		(U8500_PER3_BASE + 0x5000)
#define U8500_SKE_BASE		(U8500_PER3_BASE + 0x6000)
#define U8500_UART2_BASE	(U8500_PER3_BASE + 0x7000)
#define U8500_SDI5_BASE		(U8500_PER3_BASE + 0x8000)
#define U8500_CLKRST3_BASE	(U8500_PER3_BASE + 0xf000)

/* per2 base addressess */
#define U8500_I2C3_BASE		(U8500_PER2_BASE + 0x0000)
#define U8500_SPI2_BASE		(U8500_PER2_BASE + 0x1000)
#define U8500_SPI1_BASE		(U8500_PER2_BASE + 0x2000)
#define U8500_PWL_BASE		(U8500_PER2_BASE + 0x3000)
#define U8500_SDI4_BASE		(U8500_PER2_BASE + 0x4000)
#define U8500_MSP2_BASE		(U8500_PER2_BASE + 0x7000)
#define U8500_SDI1_BASE		(U8500_PER2_BASE + 0x8000)
#define U8500_SDI3_BASE		(U8500_PER2_BASE + 0x9000)
#define U8500_SPI0_BASE		(U8500_PER2_BASE + 0xa000)
#define U8500_HSIR_BASE		(U8500_PER2_BASE + 0xb000)
#define U8500_HSIT_BASE		(U8500_PER2_BASE + 0xc000)
#define U8500_CLKRST2_BASE	(U8500_PER2_BASE + 0xf000)

/* per1 base addresses */
#define U8500_UART0_BASE	(U8500_PER1_BASE + 0x0000)
#define U8500_UART1_BASE	(U8500_PER1_BASE + 0x1000)
#define U8500_I2C1_BASE		(U8500_PER1_BASE + 0x2000)
#define U8500_MSP0_BASE		(U8500_PER1_BASE + 0x3000)
#define U8500_MSP1_BASE		(U8500_PER1_BASE + 0x4000)
#define U8500_SDI0_BASE		(U8500_PER1_BASE + 0x6000)
#define U8500_I2C2_BASE		(U8500_PER1_BASE + 0x8000)
#define U8500_SPI3_BASE		(U8500_PER1_BASE + 0x9000)
#define U8500_I2C4_BASE		(U8500_PER1_BASE + 0xa000)
#define U8500_SLIM0_BASE	(U8500_PER1_BASE + 0xb000)
#define U8500_CLKRST1_BASE	(U8500_PER1_BASE + 0xf000)

#define U8500_SHRM_GOP_INTERRUPT_BASE	0xB7C00040

#define U8500_ESRAM_BASE	0x40000000
#define U8500_ESRAM_DMA_LCLA_OFFSET	0x80000
#define U8500_ESRAM_DMA_LCPA_OFFSET	0x84000

/* Last page of Boot ROM */
#define U8500_BOOTROM_BASE	0x9001f000
#define U8500_BOOTROM_ASIC_ID_OFFSET	0x0ff4

#define U8500_DMA_LCLA_BASE (U8500_ESRAM_BASE + U8500_ESRAM_DMA_LCLA_OFFSET)
#define U8500_DMA_LCPA_BASE (U8500_ESRAM_BASE + U8500_ESRAM_DMA_LCPA_OFFSET)

/* UART specific declaration */
#define UART_FIFO_SIZE			64
#define UART_PER_ID                     0x00380802
#define UART_TX_RX_HALF			0x2D
#define UART_CONTROL_MASK_RTSFLOW   	0x04000
#define UART_CONTROL_MASK_CTSFLOW   	0x08000

#define UART_PER_MASK                   0x00ffffff

#define STM_UART_DR    (*(volatile unsigned char *)(U8500_UART0_BASE))
#define STM_UART_LCRH  (*(volatile unsigned char *)(U8500_UART0_BASE + 0x2c))
#define STM_UART_CR    (*(volatile unsigned char *)(U8500_UART0_BASE + 0x30))
#define STM_UART_FR    (*(volatile unsigned char *)(U8500_UART0_BASE  + 0x18))


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

/* MCDE specific declaration */
#define U8500_MCDE_REGISTER_BANK_SIZE 0x20
#define U8500_MCDE_BASE_SIZE 0x200

#define U8500_MCDE_EXTSRC_BASE (U8500_MCDE_BASE + U8500_MCDE_BASE_SIZE)
#define U8500_MCDE_EXTSRC_SIZE (U8500_MCDE_REGISTER_BANK_SIZE * 0x10)

#define U8500_MCDE_OVERLAY_BASE (U8500_MCDE_BASE + 0x400)
#define U8500_MCDE_OVERLAY_SIZE (0x20 * 0x10)

#define U8500_MCDE_CHANNELA_CONFIG_BASE (U8500_MCDE_BASE + 0x600)	        /* MCDE channel config registers */
#define U8500_MCDE_CHANNELB_CONFIG_BASE (U8500_MCDE_BASE + 0x600)	        /* MCDE channel config registers */
#define U8500_MCDE_CHANNELC0_CONFIG_BASE (U8500_MCDE_BASE + 0x600)	        /* MCDE channel config registers */
#define U8500_MCDE_CHANNELC1_CONFIG_BASE (U8500_MCDE_BASE + 0x600)	        /* MCDE channel config registers */
#define U8500_MCDE_CHANNEL_CONFIG_SIZE (0x20 * 0x10)

#define U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE    (U8500_MCDE_BASE + 0x800)	/* MCDE channel A specific registers */
#define U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_SIZE    0x200

#define U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_BASE    (U8500_MCDE_BASE + 0xA00)	/* MCDE channel B specific registers */
#define U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_SIZE    0x200

#define U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_BASE (U8500_MCDE_BASE + 0xC00) /* MCDE channel C0/C1 speicific registers */
#define U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_SIZE 0xCB

#define U8500_MCDE_DSI_CHANNEL_BASE (U8500_MCDE_BASE + 0xE00)	        /* MCDE channelC0 config registers */
#define U8500_MCDE_DSI_CHANNEL_SIZE 0x20
#define U8500_MCDE_DSI_SIZE	0xF4
#define U8500_MCDE_DSI_CLOCK_OFFSET 0xF0     /* MCDE DSI clock */
#define U8500_DSI_LINK1_BASE 0xA0351000
#define U8500_DSI_LINK_SIZE 0x1000
#define U8500_DSI_LINK_COUNT 0x3
#define U8500_DSI_LINK2_BASE (U8500_DSI_LINK1_BASE + U8500_DSI_LINK_SIZE)
#define U8500_DSI_LINK3_BASE (U8500_DSI_LINK2_BASE + U8500_DSI_LINK_SIZE)

#define PRCM_HDMICLK_MGT_REG	0x80157058 /** HDMI clock selection */
#define PRCM_MCDECLK_MGT_REG	0x80157064 /** MCDE clock selection */
#define PRCM_TVCLK_MGT_REG	0x8015707c /** MCDE clock selection */

#endif				/* __ASM_ARCH_HARDWARE_H */
