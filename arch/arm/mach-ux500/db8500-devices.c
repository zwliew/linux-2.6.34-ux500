/*
 * Copyright (C) 2010 ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/amba/bus.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/spi/spi.h>

#include <mach/dma.h>
#include <linux/spi/spi-stm.h>

/* gpio alternate funtions on this platform */
#define __GPIO_ALT(_fun, _start, _end, _cont, _type, _name) {	\
	.altfun = _fun,		\
	.start = _start,	\
	.end = _end,		\
	.cont = _cont,		\
	.type = _type,		\
	.dev_name = _name }

static struct gpio_altfun_data gpio_altfun_table[] = {
	__GPIO_ALT(GPIO_ALT_I2C_4, 4, 5, 0, GPIO_ALTF_B, "i2c4"),
	__GPIO_ALT(GPIO_ALT_I2C_1, 16, 17, 0, GPIO_ALTF_B, "i2c1"),
	__GPIO_ALT(GPIO_ALT_I2C_2, 8, 9, 0, GPIO_ALTF_B, "i2c2"),
	__GPIO_ALT(GPIO_ALT_I2C_0, 147, 148, 0, GPIO_ALTF_A, "i2c0"),
	__GPIO_ALT(GPIO_ALT_I2C_3, 229, 230, 0, GPIO_ALTF_C, "i2c3"),
	__GPIO_ALT(GPIO_ALT_UART_2, 29, 32, 0, GPIO_ALTF_C, "uart2"),
	__GPIO_ALT(GPIO_ALT_SSP_0, 143, 146, 0, GPIO_ALTF_A, "ssp0"),
	__GPIO_ALT(GPIO_ALT_SSP_1, 139, 142, 0, GPIO_ALTF_A, "ssp1"),
	__GPIO_ALT(GPIO_ALT_USB_OTG, 256, 267, 0, GPIO_ALTF_A, "usb"),
	__GPIO_ALT(GPIO_ALT_UART_1, 4, 7, 0, GPIO_ALTF_A, "uart1"),
	__GPIO_ALT(GPIO_ALT_UART_0_NO_MODEM, 0, 3, 0, GPIO_ALTF_A, "uart0"),
	__GPIO_ALT(GPIO_ALT_UART_0_MODEM, 0, 3, 1, GPIO_ALTF_A, "uart0"),
	__GPIO_ALT(GPIO_ALT_UART_0_MODEM, 33, 36, 0, GPIO_ALTF_C, "uart0"),
	__GPIO_ALT(GPIO_ALT_MSP_0, 12, 15, 0, GPIO_ALTF_A, "msp0"),
	__GPIO_ALT(GPIO_ALT_MSP_1, 33, 36, 0, GPIO_ALTF_A, "msp1"),
	__GPIO_ALT(GPIO_ALT_MSP_2, 192, 196, 0, GPIO_ALTF_A, "msp2"),
	__GPIO_ALT(GPIO_ALT_HSIR, 219, 221, 0, GPIO_ALTF_A, "hsir"),
	__GPIO_ALT(GPIO_ALT_HSIT, 222, 224, 0, GPIO_ALTF_A, "hsit"),
	__GPIO_ALT(GPIO_ALT_EMMC, 197, 207, 0, GPIO_ALTF_A, "emmc"),
	__GPIO_ALT(GPIO_ALT_SDMMC, 18, 28, 0, GPIO_ALTF_A, "sdmmc"),
	__GPIO_ALT(GPIO_ALT_SDIO, 208, 214, 0, GPIO_ALTF_A, "sdio"),
	__GPIO_ALT(GPIO_ALT_TRACE, 70, 74, 0, GPIO_ALTF_C, "stm"),
	__GPIO_ALT(GPIO_ALT_SDMMC2, 128, 138, 0, GPIO_ALTF_A, "mmc2"),
#ifndef CONFIG_FB_NOMADIK_MCDE_CHANNELB_DISPLAY_VUIB_WVGA
	__GPIO_ALT(GPIO_ALT_LCD_PANELB_ED, 78, 85, 1, GPIO_ALTF_A, "mcde tvout"),
	__GPIO_ALT(GPIO_ALT_LCD_PANELB_ED, 150, 150, 0, GPIO_ALTF_B, "mcde tvout"),
	__GPIO_ALT(GPIO_ALT_LCD_PANELB, 78, 81, 1, GPIO_ALTF_A, "mcde tvout"),
	__GPIO_ALT(GPIO_ALT_LCD_PANELB, 150, 150, 0, GPIO_ALTF_B, "mcde tvout"),
#else
	__GPIO_ALT(GPIO_ALT_LCD_PANELB, 153, 171, 1, GPIO_ALTF_B, "mcde tvout"),
	__GPIO_ALT(GPIO_ALT_LCD_PANELB, 64, 77, 0, GPIO_ALTF_A, "mcde tvout"),
#endif
	__GPIO_ALT(GPIO_ALT_LCD_PANELA, 68, 68, 0, GPIO_ALTF_A, "mcde tvout"),
	__GPIO_ALT(GPIO_ALT_MMIO_INIT_BOARD, 141, 142, 0, GPIO_ALTF_B, "mmio"),
	__GPIO_ALT(GPIO_ALT_MMIO_CAM_SET_I2C, 8, 9, 0, GPIO_ALTF_A, "mmio"),
	__GPIO_ALT(GPIO_ALT_MMIO_CAM_SET_EXT_CLK, 227, 228, 0, GPIO_ALTF_A, "mmio"),
#ifdef CONFIG_TOUCHP_EXT_CLK
	__GPIO_ALT(GPIO_ALT_TP_SET_EXT_CLK, 228, 228, 0, GPIO_ALTF_A, "u8500_tp1"),
#endif
};

static struct gpio_block_data gpio0_block_data[] = {
	{
		.irq		= IRQ_GPIO0,
		.base_offset	= 0x0,
		.blocks_per_irq	= 1,
		.block_base	= 0,
		.block_size	= 32,
	},
	{
		.irq		= IRQ_GPIO1,
		.base_offset	= 0x080,
		.blocks_per_irq	= 1,
		.block_base	= 32,
		.block_size	= 5,
	 }
};

static struct gpio_platform_data gpio0_platform_data = {
	.gpio_data		= gpio0_block_data,
	.gpio_block_size	= ARRAY_SIZE(gpio0_block_data),
	.altfun_table		= gpio_altfun_table,
	.altfun_table_size	= ARRAY_SIZE(gpio_altfun_table),
};

struct amba_device u8500_gpio0_device = {
	.dev		= {
		.bus_id	= "gpioblock0",
		.platform_data = &gpio0_platform_data,
	},
	.res = {
		.start	= U8500_GPIO0_BASE,
		.end	= U8500_GPIO0_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.periphid	= GPIO_PER_ID,
	.irq		= {IRQ_GPIO0, NO_IRQ},
};

static struct gpio_block_data gpio1_block_data[] = {
	{
		.irq		= IRQ_GPIO2,
		.base_offset	= 0x0,
		.blocks_per_irq	= 1,
		.block_base	= 64,
		.block_size	= 32,
	},
	{
		.irq		= IRQ_GPIO3,
		.base_offset	= 0x080,
		.blocks_per_irq	= 1,
		.block_base	= 96,
		.block_size	= 2,
	},
	{
		.irq		= IRQ_GPIO4,
		.base_offset	= 0x100,
		.blocks_per_irq	= 1,
		.block_base	= 128,
		.block_size	= 32,
	},
	{
		.irq		= IRQ_GPIO5,
		.base_offset	= 0x180,
		.blocks_per_irq	= 1,
		.block_base	= 160,
		.block_size	= 12,
	 }
};
static struct gpio_platform_data gpio1_platform_data = {
	.gpio_data		= gpio1_block_data,
	.gpio_block_size	= ARRAY_SIZE(gpio1_block_data),
	.altfun_table		= gpio_altfun_table,
	.altfun_table_size	= ARRAY_SIZE(gpio_altfun_table)
};

struct amba_device u8500_gpio1_device = {
	.dev		= {
		.bus_id = "gpioblock1",
		.platform_data = &gpio1_platform_data,
	},
	.res = {
		.start	= U8500_GPIO1_BASE,
		.end	= U8500_GPIO1_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.periphid	= GPIO_PER_ID,
	.irq		= {IRQ_GPIO2, NO_IRQ},
};

static struct gpio_block_data gpio2_block_data[] = {
	{
		.irq = IRQ_GPIO6,
		.base_offset = 0x0,
		.blocks_per_irq = 1,
		.block_base = 192,
		.block_size = 32,
	},
	{
		.irq = IRQ_GPIO7,
		.base_offset = 0x080,
		.blocks_per_irq = 1,
		.block_base = 224,
		.block_size = 7,
	}
};

static struct gpio_platform_data gpio2_platform_data = {
	.gpio_data		= gpio2_block_data,
	.gpio_block_size	= ARRAY_SIZE(gpio2_block_data),
	.altfun_table		= gpio_altfun_table,
	.altfun_table_size	= ARRAY_SIZE(gpio_altfun_table)
};

struct amba_device u8500_gpio2_device = {
	.dev		= {
		.bus_id = "gpioblock2",
		.platform_data = &gpio2_platform_data,
	},
	.res		= {
		.start	= U8500_GPIO2_BASE,
		.end	= U8500_GPIO2_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.periphid	= GPIO_PER_ID,
	.irq		= {IRQ_GPIO6, NO_IRQ},
};

static struct gpio_block_data gpio3_block_data[] = {
	{
		.irq = IRQ_GPIO8,
		.base_offset = 0x0,
		.blocks_per_irq = 1,
		.block_base = 256,
		.block_size = 12,
	}
};

static struct gpio_platform_data gpio3_platform_data = {
	.gpio_data = gpio3_block_data,
	.gpio_block_size = ARRAY_SIZE(gpio3_block_data),
	.altfun_table = gpio_altfun_table,
	.altfun_table_size = ARRAY_SIZE(gpio_altfun_table)

};

struct amba_device u8500_gpio3_device = {
	.dev		= {
		.bus_id = "gpioblock3",
		.platform_data = &gpio3_platform_data,
	},
	.res = {
		.start	= U8500_GPIO3_BASE,
		.end	= U8500_GPIO3_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.periphid	= GPIO_PER_ID,
	.irq		= {IRQ_GPIO8, NO_IRQ},
};

static struct resource u8500_i2c0_resources[] = {
	[0] = {
		.start	= U8500_I2C0_BASE,
		.end	= U8500_I2C0_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_I2C0,
		.end	= IRQ_I2C0,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device u8500_i2c0_device = {
	.name		= "STM-I2C",
	.id		= 0,
	.resource	= u8500_i2c0_resources,
	.num_resources	= ARRAY_SIZE(u8500_i2c0_resources),
};

static struct resource u8500_i2c4_resources[] = {
	[0] = {
		.start	= U8500_I2C4_BASE,
		.end	= U8500_I2C4_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_I2C4,
		.end	= IRQ_I2C4,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device u8500_i2c4_device = {
	.name		= "STM-I2C",
	.id		= 4,
	.resource	= u8500_i2c4_resources,
	.num_resources	= ARRAY_SIZE(u8500_i2c4_resources),
};

#define NUM_SSP_CLIENTS 10

static struct nmdk_spi_master_cntlr ssp0_platform_data = {
	.enable_dma		= 1,
	.id			= SSP_0_CONTROLLER,
	.num_chipselect		= NUM_SSP_CLIENTS,
	.base_addr		= U8500_SSP0_BASE,
	.rx_fifo_addr		= U8500_SSP0_BASE + SSP_TX_RX_REG_OFFSET,
	.rx_fifo_dev_type	= DMA_DEV_SSP0_RX,
	.tx_fifo_addr		= U8500_SSP0_BASE + SSP_TX_RX_REG_OFFSET,
	.tx_fifo_dev_type	= DMA_DEV_SSP0_TX,
	.gpio_alt_func		= GPIO_ALT_SSP_0,
	.device_name		= "ssp0",
};

struct amba_device u8500_ssp0_device = {
	.dev		= {
		.bus_id = "ssp0",
		.platform_data = &ssp0_platform_data,
	},
	.res = {
		.start	= U8500_SSP0_BASE,
		.end	= U8500_SSP0_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.dma_mask	= DMA_BIT_MASK(32),
	.irq		= {IRQ_SSP0, NO_IRQ},
	.periphid	= SSP_PER_ID,
};

static struct nmdk_spi_master_cntlr ssp1_platform_data = {
	.enable_dma		= 1,
	.id			= SSP_1_CONTROLLER,
	.num_chipselect		= NUM_SSP_CLIENTS,
	.base_addr		= U8500_SSP1_BASE,
	.rx_fifo_addr		= U8500_SSP1_BASE + SSP_TX_RX_REG_OFFSET,
	.rx_fifo_dev_type	= DMA_DEV_SSP1_RX,
	.tx_fifo_addr		= U8500_SSP1_BASE + SSP_TX_RX_REG_OFFSET,
	.tx_fifo_dev_type	= DMA_DEV_SSP1_TX,
	.gpio_alt_func		= GPIO_ALT_SSP_1,
	.device_name		= "ssp1",
};

struct amba_device u8500_ssp1_device = {
	.dev		= {
		.bus_id = "ssp1",
		.platform_data = &ssp1_platform_data,
	},
	.res		= {
		.start	= U8500_SSP1_BASE,
		.end	= U8500_SSP1_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.dma_mask	= DMA_BIT_MASK(32),
	.irq		= {IRQ_SSP1, NO_IRQ},
	.periphid	= SSP_PER_ID,
};
