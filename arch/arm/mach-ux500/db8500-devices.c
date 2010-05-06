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

static struct nmk_gpio_platform_data u8500_gpio_data[] = {
	GPIO_DATA("GPIO-0-31", 0, 32),
	GPIO_DATA("GPIO-32-63", 32, 5), /* 37..63 not routed to pin */
	GPIO_DATA("GPIO-64-95", 64, 32),
	GPIO_DATA("GPIO-96-127", 96, 2), /* 98..127 not routed to pin */
	GPIO_DATA("GPIO-128-159", 128, 32),
	GPIO_DATA("GPIO-160-191", 160, 12), /* 172..191 not routed to pin */
	GPIO_DATA("GPIO-192-223", 192, 32),
	GPIO_DATA("GPIO-224-255", 224, 7), /* 231..255 not routed to pin */
	GPIO_DATA("GPIO-256-288", 256, 12), /* 268..288 not routed to pin */
};

static struct resource u8500_gpio_resources[] = {
	GPIO_RESOURCE(0),
	GPIO_RESOURCE(1),
	GPIO_RESOURCE(2),
	GPIO_RESOURCE(3),
	GPIO_RESOURCE(4),
	GPIO_RESOURCE(5),
	GPIO_RESOURCE(6),
	GPIO_RESOURCE(7),
	GPIO_RESOURCE(8),
};

struct platform_device u8500_gpio_devs[] = {
	GPIO_DEVICE(0),
	GPIO_DEVICE(1),
	GPIO_DEVICE(2),
	GPIO_DEVICE(3),
	GPIO_DEVICE(4),
	GPIO_DEVICE(5),
	GPIO_DEVICE(6),
	GPIO_DEVICE(7),
	GPIO_DEVICE(8),
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
	.name		= "nmk-i2c",
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
	.name		= "nmk-i2c",
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
		.init_name = "ssp0",
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
		.init_name = "ssp1",
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
