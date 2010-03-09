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
	__GPIO_ALT(GPIO_ALT_UART_2, 177, 180, 0, GPIO_ALTF_A, "uart2"),
	__GPIO_ALT(GPIO_ALT_SSP_0, 143, 146, 0, GPIO_ALTF_A, "ssp0"),
	__GPIO_ALT(GPIO_ALT_SSP_1, 139, 142, 0, GPIO_ALTF_A, "ssp1"),
	__GPIO_ALT(GPIO_ALT_USB_OTG, 256, 267, 0, GPIO_ALTF_A, "usb"),
	__GPIO_ALT(GPIO_ALT_UART_1, 200, 203, 0, GPIO_ALTF_A, "uart1"),
	__GPIO_ALT(GPIO_ALT_UART_0_NO_MODEM, 28, 29, 0, GPIO_ALTF_A, "uart0"),
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
		.irq 		= IRQ_GPIO0,
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
		.block_size	= 4,
	},
};

static struct gpio_platform_data gpio0_platform_data = {
	.gpio_data 		= gpio0_block_data,
	.gpio_block_size 	= ARRAY_SIZE(gpio0_block_data),
	.altfun_table 		= gpio_altfun_table,
	.altfun_table_size	= ARRAY_SIZE(gpio_altfun_table)
};

struct amba_device u5500_gpio0_device = {
	.dev		= {
		.bus_id	= "gpioblock0",
		.platform_data = &gpio0_platform_data,
	},
	.res		= {
		.start	= U5500_GPIO0_BASE,
		.end	= U5500_GPIO0_BASE + SZ_4K - 1,
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
		.block_size	= 19,
	}
};

static struct gpio_platform_data gpio1_platform_data = {
	.gpio_data		= gpio1_block_data,
	.gpio_block_size	= ARRAY_SIZE(gpio1_block_data),
	.altfun_table		= gpio_altfun_table,
	.altfun_table_size	= ARRAY_SIZE(gpio_altfun_table)

};

struct amba_device u5500_gpio1_device = {
	.dev		= {
		.bus_id	= "gpioblock1",
		.platform_data	= &gpio1_platform_data,
	},
	.res		= {
		.start	= U5500_GPIO1_BASE,
		.end	= U5500_GPIO1_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.periphid	= GPIO_PER_ID,
	.irq		= {IRQ_GPIO2, NO_IRQ},
};

static struct gpio_block_data gpio2_block_data[] = {
	{
		.irq		= IRQ_GPIO3,
		.base_offset	= 0x0,
		.blocks_per_irq	= 1,
		.block_base	= 96,
		.block_size	= 6,
	}
};

static struct gpio_platform_data gpio2_platform_data = {
	.gpio_data		= gpio2_block_data,
	.gpio_block_size	= ARRAY_SIZE(gpio2_block_data),
	.altfun_table		= gpio_altfun_table,
	.altfun_table_size	= ARRAY_SIZE(gpio_altfun_table)
};

struct amba_device u5500_gpio2_device = {
	.dev		= {
		.bus_id	= "gpioblock2",
		.platform_data = &gpio2_platform_data,
	},
	.res		= {
		.start	= U5500_GPIO2_BASE,
		.end	= U5500_GPIO2_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.periphid	= GPIO_PER_ID,
	.irq		= {IRQ_GPIO3, NO_IRQ},
};

static struct gpio_block_data gpio3_block_data[] = {
	{
		.irq		= IRQ_GPIO4,
		.base_offset	= 0x0,
		.blocks_per_irq	= 1,
		.block_base	= 128,
		.block_size	= 21,
	 }
};

static struct gpio_platform_data gpio3_platform_data = {
	.gpio_data		= gpio3_block_data,
	.gpio_block_size	= ARRAY_SIZE(gpio3_block_data),
	.altfun_table		= gpio_altfun_table,
	.altfun_table_size	= ARRAY_SIZE(gpio_altfun_table)

};

struct amba_device u5500_gpio3_device = {
	.dev		= {
		.bus_id	= "gpioblock3",
		.platform_data = &gpio3_platform_data,
	},
	.res		= {
		.start	= U5500_GPIO3_BASE,
		.end	= U5500_GPIO3_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.periphid	= GPIO_PER_ID,
	.irq		= {IRQ_GPIO4, NO_IRQ},
};

static struct gpio_block_data gpio4_block_data[] = {
	{
		.irq		= IRQ_GPIO5,
		.base_offset	= 0x0,
		.blocks_per_irq	= 1,
		.block_base	= 160,
		.block_size	= 32,
	},
	{
		.irq		= IRQ_GPIO6,
		.base_offset	= 0x80,
		.blocks_per_irq	= 1,
		.block_base	= 192,
		.block_size	= 32,
	},
	{
		.irq		= IRQ_GPIO7,
		.base_offset	= 0x100,
		.blocks_per_irq	= 1,
		.block_base	= 224,
		.block_size	= 4,
	}
};
static struct gpio_platform_data gpio4_platform_data = {
	.gpio_data		= gpio4_block_data,
	.gpio_block_size	= ARRAY_SIZE(gpio4_block_data),
	.altfun_table		= gpio_altfun_table,
	.altfun_table_size	= ARRAY_SIZE(gpio_altfun_table)

};
struct amba_device u5500_gpio4_device = {
	.dev		= {
		.bus_id	= "gpioblock4",
		.platform_data = &gpio4_platform_data,
	},
	.res		= {
		.start	= U5500_GPIO4_BASE,
		.end	= U5500_GPIO4_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.periphid	= GPIO_PER_ID,
	.irq		= {IRQ_GPIO5, NO_IRQ},
};
