/*
 * Copyright (C) 2009 ST Ericsson.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/amba/bus.h>
#include <linux/amba/serial.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/i2s/i2s.h>
#include <linux/i2c.h>
#include <linux/hsi.h>
#include <linux/hsi-legacy.h>
#include <linux/gpio.h>
#include <linux/usb/musb.h>
#include <linux/regulator/machine.h>

#include <asm/irq.h>
#include <asm/mach-types.h>
#include <asm/hardware/gic.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/hardware/cache-l2x0.h>
#include <mach/irqs.h>
#include <mach/hardware.h>
#include <asm/dma.h>
#include <mach/dma.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-stm.h>
#include <linux/mmc/host.h>
#include <mach/mcde.h>
#include <mach/mcde_common.h>
#if defined(CONFIG_ANDROID_PMEM)
#include <asm/setup.h>
#include <linux/android_pmem.h>
#endif
#include <mach/msp.h>
#include <mach/i2c-stm.h>
#include <mach/hsi-stm.h>
#include <mach/shrm.h>
#include <mach/mmc.h>
#include <mach/ab8500.h>
#include <mach/stmpe2401.h>
#include <mach/tc35892.h>
#include <mach/uart.h>

#define MOP500_PLATFORM_ID	0
#define HREF_PLATFORM_ID	1

extern void __init add_u8500_platform_devices(void);
int platform_id = MOP500_PLATFORM_ID;

int u8500_is_earlydrop(void)
{
#if !defined(CONFIG_MACH_U5500_SIMULATOR)
	void __iomem *address = (void *)IO_ADDRESS(U8500_BOOTROM_BASE)
				+ U8500_BOOTROM_ASIC_ID_OFFSET;

	/* 0x01 for ED, 0xA0 for v1 */
	return (readl(address) & 0xff) == 0x01;
#else
	return 1;
#endif
}

/* we have equally similar boards with very minimal
 * changes, so we detect the platform during boot
 */
static int __init board_id_setup(char *str)
{
if (str) {
	switch (*str) {
	case '0': {
		printk(KERN_INFO "\nMOP500 platform \n");
		platform_id = MOP500_PLATFORM_ID;
		break;
		}
	case '1': {
		printk(KERN_INFO "\nHREF platform \n");
		platform_id = HREF_PLATFORM_ID;
		break;
		}
	default:
		printk(KERN_INFO "\n Unknown board_id=%c\n", *str);
		break;
	};
}
	return 1;
}
__setup("board_id=", board_id_setup);

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
#if defined(CONFIG_MACH_U5500_SIMULATOR)
	__GPIO_ALT(GPIO_ALT_UART_2, 177, 180, 0, GPIO_ALTF_A, "uart2"),
#else
	__GPIO_ALT(GPIO_ALT_UART_2, 29, 32, 0, GPIO_ALTF_C, "uart2"),
#endif
	__GPIO_ALT(GPIO_ALT_SSP_0, 143, 146, 0, GPIO_ALTF_A, "ssp0"),
	__GPIO_ALT(GPIO_ALT_SSP_1, 139, 142, 0, GPIO_ALTF_A, "ssp1"),
	__GPIO_ALT(GPIO_ALT_USB_OTG, 256, 267, 0, GPIO_ALTF_A, "usb"),
#if defined(CONFIG_MACH_U5500_SIMULATOR)
	__GPIO_ALT(GPIO_ALT_UART_1, 200, 203, 0, GPIO_ALTF_A, "uart1"),
	__GPIO_ALT(GPIO_ALT_UART_0_NO_MODEM, 28, 29, 0, GPIO_ALTF_A, "uart0"),
#else
	__GPIO_ALT(GPIO_ALT_UART_1, 4, 7, 0, GPIO_ALTF_A, "uart1"),
	__GPIO_ALT(GPIO_ALT_UART_0_NO_MODEM, 0, 3, 0, GPIO_ALTF_A, "uart0"),
	__GPIO_ALT(GPIO_ALT_UART_0_MODEM, 0, 3, 1, GPIO_ALTF_A, "uart0"),
	__GPIO_ALT(GPIO_ALT_UART_0_MODEM, 33, 36, 0, GPIO_ALTF_C, "uart0"),
#endif
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
	 .irq = IRQ_GPIO0,
	 .base_offset = 0x0,
	 .blocks_per_irq = 1,
	 .block_base = 0,
	 .block_size = 32,
	 },
	{
	 .irq = IRQ_GPIO1,
	 .base_offset = 0x080,
	 .blocks_per_irq = 1,
	 .block_base = 32,
#if defined(CONFIG_MACH_U5500_SIMULATOR)
	 .block_size = 4,
#else
	 .block_size = 5,
#endif
	 }
};
static struct gpio_platform_data gpio0_platform_data = {
	.gpio_data = gpio0_block_data,
	.gpio_block_size = ARRAY_SIZE(gpio0_block_data),

	.altfun_table = gpio_altfun_table,
	.altfun_table_size = ARRAY_SIZE(gpio_altfun_table)
};

static struct amba_device gpio0_device = {
	.dev = {
		.bus_id = "gpioblock0",
		.platform_data = &gpio0_platform_data,
		},
	.res = {
		.start = U8500_GPIO0_BASE,
		.end = U8500_GPIO0_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
		},
	.periphid = GPIO_PER_ID,
	.irq = {IRQ_GPIO0, NO_IRQ},
};

static struct gpio_block_data gpio1_block_data[] = {
	{
	 .irq = IRQ_GPIO2,
	 .base_offset = 0x0,
	 .blocks_per_irq = 1,
	 .block_base = 64,
#if defined(CONFIG_MACH_U5500_SIMULATOR)
	 .block_size = 19,
	}
#else
	 .block_size = 32,
	 },
	{
	 .irq = IRQ_GPIO3,
	 .base_offset = 0x080,
	 .blocks_per_irq = 1,
	 .block_base = 96,
	 .block_size = 2,
	 },
	{
	 .irq = IRQ_GPIO4,
	 .base_offset = 0x100,
	 .blocks_per_irq = 1,
	 .block_base = 128,
	 .block_size = 32,
	 },
	{
	 .irq = IRQ_GPIO5,
	 .base_offset = 0x180,
	 .blocks_per_irq = 1,
	 .block_base = 160,
	 .block_size = 12,
	 }
#endif
};
static struct gpio_platform_data gpio1_platform_data = {
	.gpio_data = gpio1_block_data,
	.gpio_block_size = ARRAY_SIZE(gpio1_block_data),
	.altfun_table = gpio_altfun_table,
	.altfun_table_size = ARRAY_SIZE(gpio_altfun_table)

};

static struct amba_device gpio1_device = {
	.dev = {
		.bus_id = "gpioblock1",
		.platform_data = &gpio1_platform_data,
		},
	.res = {
		.start = U8500_GPIO1_BASE,
		.end = U8500_GPIO1_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
		},
	.periphid = GPIO_PER_ID,
	.irq = {IRQ_GPIO2, NO_IRQ},
};

static struct gpio_block_data gpio2_block_data[] = {
#if defined(CONFIG_MACH_U5500_SIMULATOR)
	{
	 .irq = IRQ_GPIO3,
	 .base_offset = 0x0,
	 .blocks_per_irq = 1,
	 .block_base = 96,
	 .block_size = 6,
	}
#else
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
#endif
};
static struct gpio_platform_data gpio2_platform_data = {
	.gpio_data = gpio2_block_data,
	.gpio_block_size = ARRAY_SIZE(gpio2_block_data),
	.altfun_table = gpio_altfun_table,
	.altfun_table_size = ARRAY_SIZE(gpio_altfun_table)

};

static struct amba_device gpio2_device = {
	.dev = {
		.bus_id = "gpioblock2",
		.platform_data = &gpio2_platform_data,
		},
	.res = {
#if defined(CONFIG_MACH_U5500_SIMULATOR)
		.start = U8500_GPIO4_BASE,
		.end = U8500_GPIO4_BASE + SZ_4K - 1,
#else
		.start = U8500_GPIO2_BASE,
		.end = U8500_GPIO2_BASE + SZ_4K - 1,
#endif
		.flags = IORESOURCE_MEM,
		},
	.periphid = GPIO_PER_ID,
#if defined(CONFIG_MACH_U5500_SIMULATOR)
	.irq = {IRQ_GPIO3, NO_IRQ},
#else
	.irq = {IRQ_GPIO6, NO_IRQ},
#endif
};

static struct gpio_block_data gpio3_block_data[] = {
	{
#if defined(CONFIG_MACH_U5500_SIMULATOR)
	 .irq = IRQ_GPIO4,
	 .base_offset = 0x0,
	 .blocks_per_irq = 1,
	 .block_base = 128,
	 .block_size = 21,
#else
	 .irq = IRQ_GPIO8,
	 .base_offset = 0x0,
	 .blocks_per_irq = 1,
	 .block_base = 256,
	 .block_size = 12,
#endif
	 }
};
static struct gpio_platform_data gpio3_platform_data = {
	.gpio_data = gpio3_block_data,
	.gpio_block_size = ARRAY_SIZE(gpio3_block_data),
	.altfun_table = gpio_altfun_table,
	.altfun_table_size = ARRAY_SIZE(gpio_altfun_table)

};

static struct amba_device gpio3_device = {
	.dev = {
		.bus_id = "gpioblock3",
		.platform_data = &gpio3_platform_data,
		},
	.res = {
#if defined(CONFIG_MACH_U5500_SIMULATOR)
		.start = U8500_GPIO2_BASE,
		.end = U8500_GPIO2_BASE + SZ_4K - 1,
#else
		.start = U8500_GPIO3_BASE,
		.end = U8500_GPIO3_BASE + SZ_4K - 1,
#endif
		.flags = IORESOURCE_MEM,
		},
	.periphid = GPIO_PER_ID,
#if defined(CONFIG_MACH_U5500_SIMULATOR)
	.irq = {IRQ_GPIO4, NO_IRQ},
#else
	.irq = {IRQ_GPIO8, NO_IRQ},
#endif
};

#if defined(CONFIG_MACH_U5500_SIMULATOR)
static struct gpio_block_data gpio4_block_data[] = {
	{
		.irq = IRQ_GPIO5,
		.base_offset = 0x0,
		.blocks_per_irq = 1,
		.block_base = 160,
		.block_size = 32,
	},
	{
		.irq = IRQ_GPIO6,
		.base_offset = 0x80,
		.blocks_per_irq = 1,
		.block_base = 192,
		.block_size = 32,
	},
	{
		.irq = IRQ_GPIO7,
		.base_offset = 0x100,
		.blocks_per_irq = 1,
		.block_base = 224,
		.block_size = 4,
	}
};
static struct gpio_platform_data gpio4_platform_data = {
	.gpio_data = gpio4_block_data,
	.gpio_block_size = ARRAY_SIZE(gpio4_block_data),
	.altfun_table = gpio_altfun_table,
	.altfun_table_size = ARRAY_SIZE(gpio_altfun_table)

};
static struct amba_device gpio4_device = {
	.dev = {
		.bus_id = "gpioblock4",
		.platform_data = &gpio4_platform_data,
	},
	.res = {
		.start = U8500_GPIO3_BASE,
		.end = U8500_GPIO3_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	.periphid = GPIO_PER_ID,
	.irq = {IRQ_GPIO5, NO_IRQ},
};
#endif


/* MSP is being used as a platform device because the perif id of all MSPs
 * is same & hence probe would be called for
 * 2 drivers namely: msp-spi & msp-i2s.
 * Inorder to avoid this & bind a set of MSPs to each driver a platform
 * bus is being used- binding based on name.
 */
static int msp_i2s_init(gpio_alt_function gpio)
{
	return stm_gpio_altfuncenable(gpio);
}

static int msp_i2s_exit(gpio_alt_function gpio)
{
	return stm_gpio_altfuncdisable(gpio);
}

static struct msp_i2s_platform_data msp0_platform_data = {
	.id = MSP_0_I2S_CONTROLLER,
	.gpio_alt_func = GPIO_ALT_MSP_0,
	.msp_tx_dma_addr = DMA_DEV_MSP0_TX,
	.msp_rx_dma_addr = DMA_DEV_MSP0_RX,
	.msp_base_addr = U8500_MSP0_BASE,
	.msp_i2s_init = msp_i2s_init,
	.msp_i2s_exit = msp_i2s_exit,
};
static struct resource u8500_msp_0_resources[] = {
	[0] = {
	       .start = U8500_MSP0_BASE,
	       .end = U8500_MSP0_BASE + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = IRQ_MSP0,
	       .end = IRQ_MSP0,
	       .flags = IORESOURCE_IRQ}
};
static struct platform_device msp0_device = {
	.name = "MSP_I2S",
	.id = 0,
	.num_resources = 2,
	.resource = u8500_msp_0_resources,
	.dev = {
		.bus_id = "msp0",
		.platform_data = &msp0_platform_data,
		},

};
static struct msp_i2s_platform_data msp1_platform_data = {
	.id = MSP_1_I2S_CONTROLLER,
	.gpio_alt_func = GPIO_ALT_MSP_1,
	.msp_tx_dma_addr = DMA_DEV_MSP1_TX,
	.msp_rx_dma_addr = DMA_DEV_MSP1_RX,
	.msp_base_addr = U8500_MSP1_BASE,
	.msp_i2s_init = msp_i2s_init,
	.msp_i2s_exit = msp_i2s_exit,
};
static struct resource u8500_msp_1_resources[] = {
	[0] = {
	       .start = U8500_MSP1_BASE,
	       .end = U8500_MSP1_BASE + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = IRQ_MSP1,
	       .end = IRQ_MSP1,
	       .flags = IORESOURCE_IRQ}
};
static struct platform_device msp1_device = {
	.name = "MSP_I2S",
	.id = 1,
	.num_resources = 2,
	.resource = u8500_msp_1_resources,
	.dev = {
		.bus_id = "msp1",
		.platform_data = &msp1_platform_data,
		},

};
static struct msp_i2s_platform_data msp2_platform_data = {
	.id = MSP_2_I2S_CONTROLLER,
	.gpio_alt_func = GPIO_ALT_MSP_2,
	.msp_tx_dma_addr = DMA_DEV_MSP2_TX,
	.msp_rx_dma_addr = DMA_DEV_MSP2_RX,
	.msp_base_addr = U8500_MSP2_BASE,
	.msp_i2s_init = msp_i2s_init,
	.msp_i2s_exit = msp_i2s_exit,
};

static struct resource u8500_msp_2_resources[] = {
	[0] = {
		.start = U8500_MSP2_BASE,
		.end = U8500_MSP2_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_MSP2,
		.end = IRQ_MSP2,
		.flags = IORESOURCE_IRQ
	}
};

static struct platform_device msp2_device = {
	.name = "MSP_I2S",
	.id = 2,
	.num_resources = 2,
	.resource = u8500_msp_2_resources,
	.dev = {
		.bus_id = "msp2",
		.platform_data = &msp2_platform_data,
	},
};

#define NUM_MSP_CLIENTS 10

static struct nmdk_spi_master_cntlr msp2_spi_platform_data = {
	.enable_dma = 1,
	.id = MSP_2_CONTROLLER,
	.num_chipselect = NUM_MSP_CLIENTS,
	.base_addr = U8500_MSP2_BASE,
	.rx_fifo_addr = U8500_MSP2_BASE + MSP_TX_RX_REG_OFFSET,
	.rx_fifo_dev_type = DMA_DEV_MSP2_RX,
	.tx_fifo_addr = U8500_MSP2_BASE + MSP_TX_RX_REG_OFFSET,
	.tx_fifo_dev_type = DMA_DEV_MSP2_TX,
	.gpio_alt_func = GPIO_ALT_MSP_2,
	.device_name = "msp2",
};

static struct amba_device msp2_spi_device = {
	.dev = {
		.bus_id = "msp2",
		.platform_data = &msp2_spi_platform_data,
		},
	.res = {
		.start = U8500_MSP2_BASE,
		.end = U8500_MSP2_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
		},
	.dma_mask = ~0,
	.irq = {IRQ_MSP2, NO_IRQ},
	.periphid = MSP_PER_ID,
};

static struct resource u8500_i2c_0_resources[] = {
	[0] = {
		.start = U8500_I2C0_BASE,
		.end = U8500_I2C0_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_I2C0,
		.end = IRQ_I2C0,
		.flags = IORESOURCE_IRQ
	}
};

static struct resource u8500_i2c_1_resources[] = {
	[0] = {
		.start = U8500_I2C1_BASE,
		.end = U8500_I2C1_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_I2C1,
		.end = IRQ_I2C1,
		.flags = IORESOURCE_IRQ
	}
};

static struct resource u8500_i2c_2_resources[] = {
	[0] = {
		.start = U8500_I2C2_BASE,
		.end = U8500_I2C2_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_I2C2,
		.end = IRQ_I2C2,
		.flags = IORESOURCE_IRQ
	}
};

static struct resource u8500_i2c_3_resources[] = {
	[0] = {
		.start = U8500_I2C3_BASE,
		.end = U8500_I2C3_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_I2C3,
		.end = IRQ_I2C3,
		.flags = IORESOURCE_IRQ
	}
};

static struct resource u8500_i2c_4_resources[] = {
	[0] = {
		.start = U8500_I2C4_BASE,
		.end = U8500_I2C4_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
		},
	[1] = {
		.start = IRQ_I2C4,
		.end = IRQ_I2C4,
		.flags = IORESOURCE_IRQ
	}
};

static struct i2c_platform_data u8500_i2c_0_private_data = {
	.gpio_alt_func = GPIO_ALT_I2C_0,
	.name = "i2c0",
	.own_addr = I2C0_LP_OWNADDR,
	.mode = I2C_FREQ_MODE_STANDARD,
	.clk_freq = 100000,
	.slave_addressing_mode = I2C_7_BIT_ADDRESS,
	.digital_filter_control = I2C_DIGITAL_FILTERS_OFF,
	.dma_sync_logic_control = I2C_DISABLED,
	.start_byte_procedure = I2C_DISABLED,
	.slave_data_setup_time = 0xE,
	.bus_control_mode = I2C_BUS_MASTER_MODE,
	.i2c_loopback_mode = I2C_DISABLED,
	.xfer_mode = I2C_TRANSFER_MODE_INTERRUPT,
	.high_speed_master_code = 0,
	.i2c_tx_int_threshold = 1,
	.i2c_rx_int_threshold = 1
};

static struct i2c_platform_data u8500_i2c_1_private_data = {
	.gpio_alt_func = GPIO_ALT_I2C_1,
	.name = "i2c1",
	.own_addr = I2C1_LP_OWNADDR,
	.mode = I2C_FREQ_MODE_STANDARD,
	.clk_freq = 100000,
	.slave_addressing_mode = I2C_7_BIT_ADDRESS,
	.digital_filter_control = I2C_DIGITAL_FILTERS_OFF,
	.dma_sync_logic_control = I2C_DISABLED,
	.start_byte_procedure = I2C_DISABLED,
	.slave_data_setup_time = 0xE,
	.bus_control_mode = I2C_BUS_MASTER_MODE,
	.i2c_loopback_mode = I2C_DISABLED,
	.xfer_mode = I2C_TRANSFER_MODE_INTERRUPT,
	.high_speed_master_code = 0,
	.i2c_tx_int_threshold = 1,
	.i2c_rx_int_threshold = 1
};

static struct i2c_platform_data u8500_i2c_2_private_data = {
	.gpio_alt_func = GPIO_ALT_I2C_2,
	.name = "i2c2",
	.own_addr = I2C2_LP_OWNADDR,
	.mode = I2C_FREQ_MODE_STANDARD,
	.clk_freq = 100000,
	.slave_addressing_mode = I2C_7_BIT_ADDRESS,
	.digital_filter_control = I2C_DIGITAL_FILTERS_OFF,
	.dma_sync_logic_control = I2C_DISABLED,
	.start_byte_procedure = I2C_DISABLED,
	.slave_data_setup_time = 0xE,
	.bus_control_mode = I2C_BUS_MASTER_MODE,
	.i2c_loopback_mode = I2C_DISABLED,
	.xfer_mode = I2C_TRANSFER_MODE_INTERRUPT,
	.high_speed_master_code = 0,
	.i2c_tx_int_threshold = 1,
	.i2c_rx_int_threshold = 1
};

static struct i2c_platform_data u8500_i2c_3_private_data = {
	.gpio_alt_func = GPIO_ALT_I2C_3,
	.name = "i2c3",
	.own_addr = I2C3_LP_OWNADDR,
	.mode = I2C_FREQ_MODE_STANDARD,
	.clk_freq = 100000,
	.slave_addressing_mode = I2C_7_BIT_ADDRESS,
	.digital_filter_control = I2C_DIGITAL_FILTERS_OFF,
	.dma_sync_logic_control = I2C_DISABLED,
	.start_byte_procedure = I2C_DISABLED,
	.slave_data_setup_time = 0xE,
	.bus_control_mode = I2C_BUS_MASTER_MODE,
	.i2c_loopback_mode = I2C_DISABLED,
	.xfer_mode = I2C_TRANSFER_MODE_INTERRUPT,
	.high_speed_master_code = 0,
	.i2c_tx_int_threshold = 1,
	.i2c_rx_int_threshold = 1
};

static struct i2c_platform_data u8500_i2c_4_private_data = {
	.gpio_alt_func = GPIO_ALT_I2C_4,
	.name = "i2c4",
	.own_addr = I2C4_LP_OWNADDR,
	.mode = I2C_FREQ_MODE_STANDARD,
	.clk_freq = 100000,
	.slave_addressing_mode = I2C_7_BIT_ADDRESS,
	.digital_filter_control = I2C_DIGITAL_FILTERS_OFF,
	.dma_sync_logic_control = I2C_DISABLED,
	.start_byte_procedure = I2C_DISABLED,
	.slave_data_setup_time = 0xE,
	.bus_control_mode = I2C_BUS_MASTER_MODE,
	.i2c_loopback_mode = I2C_DISABLED,
	.xfer_mode = I2C_TRANSFER_MODE_INTERRUPT,
	.high_speed_master_code = 0,
	.i2c_tx_int_threshold = 1,
	.i2c_rx_int_threshold = 1
};

static struct platform_device u8500_i2c_0_controller = {
	.name = "STM-I2C",
	.id = 0,
	.num_resources = 2,
	.resource = u8500_i2c_0_resources,
	.dev = {
		.platform_data = &u8500_i2c_0_private_data}
};

static struct platform_device u8500_i2c_1_controller = {
	.name = "STM-I2C",
	.id = 1,
	.num_resources = 2,
	.resource = u8500_i2c_1_resources,
	.dev = {
		.platform_data = &u8500_i2c_1_private_data}
};

static struct platform_device u8500_i2c_2_controller = {
	.name = "STM-I2C",
	.id = 2,
	.num_resources = 2,
	.resource = u8500_i2c_2_resources,
	.dev = {
		.platform_data = &u8500_i2c_2_private_data}
};

static struct platform_device u8500_i2c_3_controller = {
	.name = "STM-I2C",
	.id = 3,
	.num_resources = 2,
	.resource = u8500_i2c_3_resources,
	.dev = {
		.platform_data = &u8500_i2c_3_private_data}
};

static struct platform_device u8500_i2c_4_controller = {
	.name = "STM-I2C",
	.id = 4,
	.num_resources = 2,
	.resource = u8500_i2c_4_resources,
	.dev = {
		.platform_data = &u8500_i2c_4_private_data}
};

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
/* Channel C0 */

#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT

static struct resource mcde2_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end   = U8500_MCDE_BASE + (U8500_MCDE_BASE_SIZE  - 1),
		.name  = "mcde_top",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end   = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name  = "mcde_extsrc",
		.flags = IORESOURCE_MEM,
	},
  [2] = {
		.start = U8500_MCDE_OVERLAY_BASE,
		.end   = U8500_MCDE_OVERLAY_BASE + (U8500_MCDE_OVERLAY_SIZE - 1),
		.name  = "mcde_overlay",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELC0_CONFIG_BASE,
    .end   = U8500_MCDE_CHANNELC0_CONFIG_BASE +	(U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name  = "mcde_chc0_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_BASE,
		.end   = U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_BASE +	(U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_SIZE - 1),
		.name  = "mcde_chc0c1_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end   = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_CHANNEL_SIZE - 1),
		.name  = "mcde_dsi_channel",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end   = U8500_DSI_LINK1_BASE + ((U8500_DSI_LINK_SIZE * U8500_DSI_LINK_COUNT) - 1),
		.name  = "dsi_link",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end   = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name  = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end   = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name  = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end   = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name  = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end   = IRQ_DISP,
		.name  = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
	[11] = {
		.start = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE +
			(U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_SIZE - 1),
		.name = "cha_specific",
		.flags = IORESOURCE_MEM,
	},
	[12] = {
		.start = U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_BASE +
			(U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_SIZE - 1),
		.name = "chb_specific",
		.flags = IORESOURCE_MEM,
	},
};

#else	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct resource mcde2_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end = U8500_MCDE_BASE + (U8500_MCDE_REGISTER_SIZE  - 1),
		.name = "mcde_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name = "mcde_extsrc_base",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVL_BASE,
		.end = U8500_MCDE_OVL_BASE + (U8500_MCDE_OVL_SIZE - 1),
		.name = "mcde_overlay_base",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELC0_CONFIG_BASE,
		.end = U8500_MCDE_CHANNELC0_CONFIG_BASE +
			(U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name = "chc0_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELC_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELC_SPECIFIC_REGISTER_BASE +
			(U8500_MCDE_CHANNELC_SPECIFIC_REGISTER_SIZE - 1),
		.name = "chb_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name = "mcde_dsi_channel_base",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end = U8500_DSI_LINK1_BASE +
			((U8500_DSI_LINK_SIZE*U8500_DSI_LINK_COUNT) - 1),
		.name = "dsi_link_base",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
	[11] = {
		.start = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE +
			(U8500_MCDE_CHANNEL_SPECIFIC_REGISTER_SIZE - 1),
		.name = "cha_specific",
		.flags = IORESOURCE_MEM,
	},
};

#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct mcde_channel_data mcde2_channel_data = {
	.channelid  = CHANNEL_C0,
	.nopan = 1,
	.nowrap = 1,
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
	.restype = CONFIG_FB_U8500_MCDE_CHANNELC0_DISPLAY_TYPE,
	.inbpp = CONFIG_FB_U8500_MCDE_CHANNELC0_INPUT_BPP,
	.outbpp = CONFIG_FB_U8500_MCDE_CHANNELC0_OUTPUT_BPP,
	.bpp16_type = CONFIG_FB_U8500_MCDE_CHANNELC0_INPUT_16BPP_TYPE,
	.bgrinput = CONFIG_FB_U8500_MCDE_CHANNELC0_INPUT_BGR,
#else
	.restype = "WVGA",
	.inbpp = 16,
	.outbpp = 0x2,
	.bpp16_type = 1,
	.bgrinput = 0x0,
#endif
};

static struct platform_device mcde2_device = {
	.name = "U8500-MCDE",
	.id = 2,
	.dev = {
		.bus_id = "mcde_bus",
		.coherent_dma_mask = ~0,
		.platform_data = &mcde2_channel_data,
	},
	.num_resources = ARRAY_SIZE(mcde2_resources),
	.resource = mcde2_resources
};
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELC0 */

#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1
/* Channel C1 */

#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT

static struct resource mcde3_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end   = U8500_MCDE_BASE + (U8500_MCDE_BASE_SIZE  - 1),
		.name  = "mcde_top",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end   = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name  = "mcde_extsrc",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVERLAY_BASE,
		.end   = U8500_MCDE_OVERLAY_BASE + (U8500_MCDE_OVERLAY_SIZE - 1),
		.name  = "mcde_overlay",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELC1_CONFIG_BASE,
    .end   = U8500_MCDE_CHANNELC1_CONFIG_BASE +	(U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name  = "mcde_chc1_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_BASE,
		.end   = U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_BASE + (U8500_MCDE_CHANNELC0C1_SPECIFIC_REGISTER_SIZE - 1),
		.name  = "mcde_chc0c1_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end   = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name  = "mcde_dsi_channel",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end   = U8500_DSI_LINK1_BASE +	((U8500_DSI_LINK_SIZE * U8500_DSI_LINK_COUNT) - 1),
		.name  = "dsi_link",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
};

#else	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct resource mcde3_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end = U8500_MCDE_BASE + (U8500_MCDE_REGISTER_SIZE  - 1),
		.name = "mcde_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name = "mcde_extsrc_base",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVL_BASE,
		.end = U8500_MCDE_OVL_BASE + (U8500_MCDE_OVL_SIZE - 1),
		.name = "mcde_overlay_base",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELC1_CONFIG_BASE,
		.end = U8500_MCDE_CHANNELC1_CONFIG_BASE +
			(U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name = "chc1_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELC_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELC_SPECIFIC_REGISTER_BASE +
			(U8500_MCDE_CHANNELC_SPECIFIC_REGISTER_SIZE - 1),
		.name = "chb_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name = "mcde_dsi_channel_base",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end = U8500_DSI_LINK1_BASE +
			((U8500_DSI_LINK_SIZE*U8500_DSI_LINK_COUNT) - 1),
		.name = "dsi_link_base",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
};

#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct mcde_channel_data mcde3_channel_data = {
	.channelid  = CHANNEL_C1,
	.nopan = 1,
	.nowrap = 1,
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1
	.restype = CONFIG_FB_U8500_MCDE_CHANNELC1_DISPLAY_TYPE,
	.inbpp = CONFIG_FB_U8500_MCDE_CHANNELC1_INPUT_BPP,
	.outbpp = CONFIG_FB_U8500_MCDE_CHANNELC1_OUTPUT_BPP,
	.bpp16_type = CONFIG_FB_U8500_MCDE_CHANNELC1_INPUT_16BPP_TYPE,
	.bgrinput = CONFIG_FB_U8500_MCDE_CHANNELC1_INPUT_BGR,
#else
	.restype = "WVGA",
	.inbpp = 16,
	.outbpp = 0x2,
	.bpp16_type = 1,
	.bgrinput = 0x0,
#endif
};

static struct platform_device mcde3_device = {
	.name = "U8500-MCDE",
	.id = 3,
	.dev = {
		.bus_id = "mcde_bus",
		.coherent_dma_mask = ~0,
		.platform_data = &mcde3_channel_data,
	},

	.num_resources = ARRAY_SIZE(mcde3_resources),
	.resource = mcde3_resources
};
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELC1 */

#ifdef CONFIG_FB_U8500_MCDE_CHANNELB
/* Channel B */

#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT

static struct resource mcde1_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end   = U8500_MCDE_BASE + (U8500_MCDE_BASE_SIZE  - 1),
		.name  = "mcde_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end   = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name  = "mcde_extsrc",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVERLAY_BASE,
		.end   = U8500_MCDE_OVERLAY_BASE + (U8500_MCDE_OVERLAY_SIZE - 1),
		.name  = "mcde_overlay",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELB_CONFIG_BASE,
		.end   = U8500_MCDE_CHANNELB_CONFIG_BASE + (U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name  = "mcde_chb_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_BASE,
		.end   = U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_BASE + (U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_SIZE - 1),
		.name  = "mcde_chb_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end   = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name  = "mcde_dsi_channel",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end   = U8500_DSI_LINK1_BASE + ((U8500_DSI_LINK_SIZE*U8500_DSI_LINK_COUNT) - 1),
		.name  = "dsi_link",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end   = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name  = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
};

#else	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct resource mcde1_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end = U8500_MCDE_BASE + (U8500_MCDE_REGISTER_SIZE  - 1),
		.name = "mcde_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name = "mcde_extsrc_base",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVL_BASE,
		.end = U8500_MCDE_OVL_BASE + (U8500_MCDE_OVL_SIZE - 1),
		.name = "mcde_overlay_base",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELB_CONFIG_BASE,
		.end = U8500_MCDE_CHANNELB_CONFIG_BASE +
			(U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name = "chb_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELB_SPECIFIC_REGISTER_BASE +
			 (U8500_MCDE_CHANNEL_SPECIFIC_REGISTER_SIZE - 1),
		.name = "chb_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name = "mcde_dsi_channel_base",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end = U8500_DSI_LINK1_BASE +
			((U8500_DSI_LINK_SIZE*U8500_DSI_LINK_COUNT) - 1),
		.name = "dsi_link_base",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
};

#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct mcde_channel_data mcde1_channel_data = {
	.channelid  = CHANNEL_B,
	.nopan = 1,
	.nowrap = 1,
#ifdef CONFIG_FB_U8500_MCDE_CHANNELB
	.restype = CONFIG_FB_U8500_MCDE_CHANNELB_DISPLAY_TYPE,
	.inbpp = CONFIG_FB_U8500_MCDE_CHANNELB_INPUT_BPP,
	.outbpp = CONFIG_FB_U8500_MCDE_CHANNELB_OUTPUT_BPP,
	.bpp16_type = CONFIG_FB_U8500_MCDE_CHANNELB_INPUT_16BPP_TYPE,
	.bgrinput = CONFIG_FB_U8500_MCDE_CHANNELB_INPUT_BGR,
#else
	.restype = "SDTV",
	.inbpp = 16,
	.outbpp = 0x2,
	.bpp16_type = 1,
	.bgrinput = 0x0,
#endif
	.gpio_alt_func =  GPIO_ALT_LCD_PANELB
};

static struct platform_device mcde1_device = {
	.name = "U8500-MCDE",
	.id = 1,
	.dev = {
		.bus_id = "mcde_bus",
		.coherent_dma_mask = ~0,
		.platform_data = &mcde1_channel_data,
	},
	.num_resources = ARRAY_SIZE(mcde1_resources),
	.resource = mcde1_resources
};
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELB */

#ifdef CONFIG_FB_U8500_MCDE_CHANNELA
/* Channel A */

#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT

static struct resource mcde0_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end   = U8500_MCDE_BASE + (U8500_MCDE_BASE_SIZE  - 1),
		.name  = "mcde_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end   = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name  = "mcde_extsrc",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVERLAY_BASE,
		.end   = U8500_MCDE_OVERLAY_BASE + (U8500_MCDE_OVERLAY_SIZE - 1),
		.name  = "mcde_overlay",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELA_CONFIG_BASE,
		.end   = U8500_MCDE_CHANNELA_CONFIG_BASE + (U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name  = "mcde_cha_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE,
		.end   = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE + (U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_SIZE - 1),
		.name  = "mcde_cha_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end   = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name  = "mcde_dsi_channel",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end   = U8500_DSI_LINK1_BASE +	((U8500_DSI_LINK_SIZE * U8500_DSI_LINK_COUNT) - 1),
		.name  = "dsi_link",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
};

#else	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct resource mcde0_resources[] = {
	[0] = {
		.start = U8500_MCDE_BASE,
		.end = U8500_MCDE_BASE + (U8500_MCDE_REGISTER_SIZE  - 1),
		.name = "mcde_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = U8500_MCDE_EXTSRC_BASE,
		.end = U8500_MCDE_EXTSRC_BASE + (U8500_MCDE_EXTSRC_SIZE - 1),
		.name = "mcde_extsrc_base",
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = U8500_MCDE_OVL_BASE,
		.end = U8500_MCDE_OVL_BASE + (U8500_MCDE_OVL_SIZE - 1),
		.name = "mcde_overlay_base",
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = U8500_MCDE_CHANNELA_CONFIG_BASE,
		.end = U8500_MCDE_CHANNELA_CONFIG_BASE +
			(U8500_MCDE_CHANNEL_CONFIG_SIZE - 1),
		.name = "cha_config",
		.flags = IORESOURCE_MEM,
	},
	[4] = {
		.start = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE,
		.end = U8500_MCDE_CHANNELA_SPECIFIC_REGISTER_BASE +
			(U8500_MCDE_CHANNEL_SPECIFIC_REGISTER_SIZE - 1),
		.name = "cha_specific",
		.flags = IORESOURCE_MEM,
	},
	[5] = {
		.start = U8500_MCDE_DSI_CHANNEL_BASE,
		.end = U8500_MCDE_DSI_CHANNEL_BASE + (U8500_MCDE_DSI_SIZE - 1),
		.name = "mcde_dsi_channel_base",
		.flags = IORESOURCE_MEM,
	},
	[6] = {
		.start = U8500_DSI_LINK1_BASE,
		.end = U8500_DSI_LINK1_BASE +
			((U8500_DSI_LINK_SIZE*U8500_DSI_LINK_COUNT) - 1),
		.name = "dsi_link_base",
		.flags = IORESOURCE_MEM,
	},
	[7] = {
		.start = PRCM_MCDECLK_MGT_REG,
		.end = PRCM_MCDECLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_mcde_clk",
		.flags = IORESOURCE_MEM,
	},
	[8] = {
		.start = PRCM_HDMICLK_MGT_REG,
		.end = PRCM_HDMICLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_hdmi_clk",
		.flags = IORESOURCE_MEM,
	},
	[9] = {
		.start = PRCM_TVCLK_MGT_REG,
		.end = PRCM_TVCLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_tv_clk",
		.flags = IORESOURCE_MEM,
	},
	[10] = {
		.start = IRQ_DISP,
		.end = IRQ_DISP,
		.name = "mcde_irq",
		.flags = IORESOURCE_IRQ
	},
};

#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

static struct mcde_channel_data mcde0_channel_data = {
	.channelid  = CHANNEL_A,
	.nopan = 1,
	.nowrap = 1,
	.restype = CONFIG_FB_U8500_MCDE_CHANNELA_DISPLAY_TYPE,
	.inbpp = CONFIG_FB_U8500_MCDE_CHANNELA_INPUT_BPP,
	.outbpp = CONFIG_FB_U8500_MCDE_CHANNELA_OUTPUT_BPP,
	.bpp16_type = CONFIG_FB_U8500_MCDE_CHANNELA_INPUT_16BPP_TYPE,
	.bgrinput = CONFIG_FB_U8500_MCDE_CHANNELA_INPUT_BGR,
	.gpio_alt_func =  GPIO_ALT_LCD_PANELA
};

static struct platform_device mcde0_device = {
	.name = "U8500-MCDE",
	.id = 0,
	.dev = {
		.bus_id = "mcde_bus",
		.coherent_dma_mask = ~0,
		.platform_data = &mcde0_channel_data,
	},
	.num_resources = ARRAY_SIZE(mcde0_resources),
	.resource = mcde0_resources
};
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELA */

static struct hsi_plat_data hsit_platform_data = {
	.dev_type = 0x0 /** transmitter */ ,
	.mode = 0x2 /** frame mode */ ,
	.divisor = 0x12 /** half HSIT freq */ ,
	.parity = 0x0 /** no parity */ ,
	.channels = 0x4 /** 4 channels */ ,
	.flushbits = 0x0 /** none */ ,
	.priority = 0x3 /** ch0,ch1 high while ch2,ch3 low */ ,
	.burstlen = 0x0 /** infinite b2b */ ,
	.preamble = 0x0 /** none */ ,
	.dataswap = 0x0 /** no swap */ ,
	.framelen = 0x1f /** 32 bits all channels */ ,
	.ch_base_span = {[0] = {.base = 0x0, .span = 0x3},
			 [1] = {.base = 0x4, .span = 0x3},
			 [2] = {.base = 0x8, .span = 0x7},
			 [3] = {.base = 0x10, .span = 0x7}
			 },
#ifdef CONFIG_STN8500_HSI_LEGACY
	.currmode = CONFIG_STN8500_HSI_TRANSFER_MODE,
#else
	.currmode = 1,
#endif
	.hsi_dma_info = {
			 [0] = {
				.reserve_channel = 0,
				.dir = MEM_TO_PERIPH,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.dst_dev_type = DMA_DEV_SLIM0_CH0_TX_HSI_TX_CH0,
				.src_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 [1] = {
				.reserve_channel = 0,
				.dir = MEM_TO_PERIPH,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.dst_dev_type = DMA_DEV_SLIM0_CH1_TX_HSI_TX_CH1,
				.src_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 [2] = {
				.reserve_channel = 0,
				.dir = MEM_TO_PERIPH,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.dst_dev_type = DMA_DEV_SLIM0_CH2_TX_HSI_TX_CH2,
				.src_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 [3] = {
				.reserve_channel = 0,
				.dir = MEM_TO_PERIPH,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.dst_dev_type = DMA_DEV_SLIM0_CH3_TX_HSI_TX_CH3,
				.src_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 },
	.watermark = 0x2 /** 1 free entries for all channels */ ,
	.gpio_alt_func = GPIO_ALT_HSIT,
};

static struct hsi_plat_data hsir_platform_data = {
	.dev_type = 0x1 /** receiver */ ,
	.mode = 0x3 /** pipelined */ ,
	.threshold = 0x22 /** 35 zero bits for break */ ,
	.parity = 0x0 /** no parity */ ,
	.detector = 0x0 /** oversampling */ ,
	.channels = 0x4 /** 4 channels */ ,
	.realtime = 0x0 /** disabled,no overwrite,all channels */ ,
	.framelen = 0x1f /** 32 bits all channels */ ,
	.preamble = 0x0 /** max timeout cycles */ ,
	.ch_base_span = {[0] = {.base = 0x0, .span = 0x3},
			 [1] = {.base = 0x4, .span = 0x3},
			 [2] = {.base = 0x8, .span = 0x7},
			 [3] = {.base = 0x10, .span = 0x7}
			 },
#ifdef CONFIG_STN8500_HSI_LEGACY
	.currmode = CONFIG_STN8500_HSI_TRANSFER_MODE,
#else
	.currmode = 1,
#endif
	.timeout = 0x0 /** immediate updation of channel buffer */ ,
	.hsi_dma_info = {
			 [0] = {
				.reserve_channel = 0,
				.dir = PERIPH_TO_MEM,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.src_dev_type = DMA_DEV_SLIM0_CH0_RX_HSI_RX_CH0,
				.dst_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 [1] = {
				.reserve_channel = 0,
				.dir = PERIPH_TO_MEM,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.src_dev_type = DMA_DEV_SLIM0_CH1_RX_HSI_RX_CH1,
				.dst_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 [2] = {
				.reserve_channel = 0,
				.dir = PERIPH_TO_MEM,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.src_dev_type = DMA_DEV_SLIM0_CH2_RX_HSI_RX_CH2,
				.dst_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 [3] = {
				.reserve_channel = 0,
				.dir = PERIPH_TO_MEM,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.src_dev_type = DMA_DEV_SLIM0_CH3_RX_HSI_RX_CH3,
				.dst_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 },
	.watermark = 0x2 /** 1 'occupated' entry for all channels */ ,
	.gpio_alt_func = GPIO_ALT_HSIR,
};

static struct resource u8500_hsit_resource[] = {
	[0] = {
		.start = U8500_HSIT_BASE,
		.end = U8500_HSIT_BASE + (SZ_4K - 1),
		.name = "hsit_iomem_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_HSITD0,
		.end = IRQ_HSITD1,
		.name = "hsit_irq",
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource u8500_hsir_resource[] = {
	[0] = {
		.start = U8500_HSIR_BASE,
		.end = U8500_HSIR_BASE + (SZ_4K - 1),
		.name = "hsir_iomem_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_HSIRD0,
		.end = IRQ_HSIRD1,
		.name = "hsir_irq",
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_HSIR_EXCEP,
		.end = IRQ_HSIR_EXCEP,
		.name = "hsir_irq_excep",
		.flags = IORESOURCE_IRQ,
	},
	[3] = {
		.start = IRQ_HSIR_CH0_OVRRUN,
		.end = IRQ_HSIR_CH0_OVRRUN,
		.name = "hsir_irq_ch0_overrun",
		.flags = IORESOURCE_IRQ,
	},
	[4] = {
		.start = IRQ_HSIR_CH1_OVRRUN,
		.end = IRQ_HSIR_CH1_OVRRUN,
		.name = "hsir_irq_ch1_overrun",
		.flags = IORESOURCE_IRQ,
	},
	[5] = {
		.start = IRQ_HSIR_CH2_OVRRUN,
		.end = IRQ_HSIR_CH2_OVRRUN,
		.name = "hsir_irq_ch2_overrun",
		.flags = IORESOURCE_IRQ,
	},
	[6] = {
		.start = IRQ_HSIR_CH3_OVRRUN,
		.end = IRQ_HSIR_CH3_OVRRUN,
		.name = "hsir_irq_ch3_overrun",
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device u8500_hsit_device = {
	.name = "stm-hsi",
	.id = 0,
	.dev = {
		.platform_data = &hsit_platform_data,
		},
	.num_resources = ARRAY_SIZE(u8500_hsit_resource),
	.resource = u8500_hsit_resource,
};

static struct platform_device u8500_hsir_device = {
	.name = "stm-hsi",
	.id = 1,
	.dev = {
		.platform_data = &hsir_platform_data,
		},
	.num_resources = ARRAY_SIZE(u8500_hsir_resource),
	.resource = u8500_hsir_resource,
};

static struct shrm_plat_data shrm_platform_data = {

       .pshm_dev_data =  0
};

static struct resource u8500_shrm_resources[] = {
	[0] = {
		.start = U8500_SHRM_GOP_INTERRUPT_BASE,
		.end = U8500_SHRM_GOP_INTERRUPT_BASE + ((4*4)-1),
		.name = "shrm_gop_register_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_CA_WAKE_REQ_V1,
		.end = IRQ_CA_WAKE_REQ_V1,
		.name = "ca_irq_wake_req",
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_AC_READ_NOTIFICATION_0_V1,
		.end = IRQ_AC_READ_NOTIFICATION_0_V1,
		.name = "ac_read_notification_0_irq",
		.flags = IORESOURCE_IRQ,
	},
	[3] = {
		.start = IRQ_AC_READ_NOTIFICATION_1_V1,
		.end = IRQ_AC_READ_NOTIFICATION_1_V1,
		.name = "ac_read_notification_1_irq",
		.flags = IORESOURCE_IRQ,
	},
	[4] = {
		.start = IRQ_CA_MSG_PEND_NOTIFICATION_0_V1,
		.end = IRQ_CA_MSG_PEND_NOTIFICATION_0_V1,
		.name = "ca_msg_pending_notification_0_irq",
		.flags = IORESOURCE_IRQ,
	},
	[5] = {
		.start = IRQ_CA_MSG_PEND_NOTIFICATION_1_V1,
		.end = IRQ_CA_MSG_PEND_NOTIFICATION_1_V1,
		.name = "ca_msg_pending_notification_1_irq",
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device u8500_shrm_device = {
	.name = "u8500_shrm",
	.id = 0,
	.dev = {
		.bus_id = "shrm_bus",
		.coherent_dma_mask = ~0,
		.platform_data = &shrm_platform_data,
	},

	.num_resources = ARRAY_SIZE(u8500_shrm_resources),
	.resource = u8500_shrm_resources
};

static struct resource b2r2_resources[] = {
	[0] = {
		.start = U8500_B2R2_BASE,
		.end = U8500_B2R2_BASE + ((4*1024)-1),
		.name = "b2r2_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = PRCM_B2R2CLK_MGT_REG,
		.end = PRCM_B2R2CLK_MGT_REG + (sizeof(u32) - 1),
		.name = "prcm_b2r2_clk",
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device b2r2_device = {
	.name = "U8500-B2R2",
	.id = 0,
	.dev = {
		.bus_id = "b2r2_bus",
		.coherent_dma_mask = ~0,
	},

	.num_resources = ARRAY_SIZE(b2r2_resources),
	.resource = b2r2_resources
};

static void __init early_pmem_generic_parse(char **p, struct android_pmem_platform_data * data)
{
	data->size = memparse(*p, p);
	if (**p == '@')
		data->start = memparse(*p + 1, p);
}

/********************************************************************************
 * Pmem device used by surface flinger
 ********************************************************************************/

static struct android_pmem_platform_data pmem_pdata = {
	.name = "pmem",
	.no_allocator = 1,	/* MemoryHeapBase is having an allocator */
	.cached = 1,
	.start = 0,
	.size = 0,
};

static void __init early_pmem(char **p)
{
	early_pmem_generic_parse(p, &pmem_pdata);
}
__early_param("pmem=", early_pmem);

static struct platform_device pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = {
		.platform_data = &pmem_pdata,
	},
};

/********************************************************************************
 * Pmem device used by PV video output node
 ********************************************************************************/

static struct android_pmem_platform_data pmem_mio_pdata = {
	.name = "pmem_mio",
	.no_allocator = 1,	/* We'll manage allocation */
	.cached = 0,
	.start = 0,
	.size = 0,
};

static void __init early_pmem_mio(char **p)
{
	early_pmem_generic_parse(p, &pmem_mio_pdata);
}
__early_param("pmem_mio=", early_pmem_mio);

static struct platform_device pmem_mio_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = {
		.platform_data = &pmem_mio_pdata,
	},
};

/********************************************************************************
 * Pmem device used by OMX components allocating buffers
 ********************************************************************************/

static struct android_pmem_platform_data pmem_hwb_pdata = {
	.name = "pmem_hwb",
	.no_allocator = 1,	/* We'll manage allocation */
	.cached = 0,
	.start = 0,
	.size = 0,
};

static void __init early_pmem_hwb(char **p)
{
	early_pmem_generic_parse(p, &pmem_hwb_pdata);
}
__early_param("pmem_hwb=", early_pmem_hwb);

static struct platform_device pmem_hwb_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = {
		.platform_data = &pmem_hwb_pdata,
	},
};

static struct amba_device rtc_device = {
	.dev = {
		.bus_id = "mb:15",
	},
	.res = {
		.start = U8500_RTC_BASE,
		.end = U8500_RTC_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	.irq = {IRQ_RTC_RTT, NO_IRQ},
	.periphid = RTC_PER_ID,
};

/*
 *  SOC specifc drivers whcih are used as platform devices
 */

#include "clock.h"
/*
 *  SOC specifc drivers whcih are used as platform devices
 */

static struct map_desc u8500_common_io_desc[] __initdata = {
	{IO_ADDRESS(U8500_BOOTROM_BASE), __phys_to_pfn(U8500_BOOTROM_BASE), SZ_4K,
	 MT_DEVICE_CACHED},
	{IO_ADDRESS(U8500_UART0_BASE), __phys_to_pfn(U8500_UART0_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(U8500_MSP0_BASE), __phys_to_pfn(U8500_MSP0_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(U8500_MSP1_BASE), __phys_to_pfn(U8500_MSP1_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(U8500_MSP2_BASE), __phys_to_pfn(U8500_MSP2_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(U8500_UART1_BASE), __phys_to_pfn(U8500_UART1_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(U8500_UART2_BASE), __phys_to_pfn(U8500_UART2_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(U8500_GIC_CPU_BASE), __phys_to_pfn(U8500_GIC_CPU_BASE),
	 SZ_4K, MT_DEVICE},
	{IO_ADDRESS(U8500_GIC_DIST_BASE), __phys_to_pfn(U8500_GIC_DIST_BASE),
	 SZ_4K, MT_DEVICE},
	{IO_ADDRESS(U8500_L2CC_BASE), __phys_to_pfn(U8500_L2CC_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(U8500_GPIO_BASE), __phys_to_pfn(U8500_GPIO_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(GPIO_BANK0_BASE), __phys_to_pfn(GPIO_BANK0_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(GPIO_BANK1_BASE), __phys_to_pfn(GPIO_BANK1_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(GPIO_BANK2_BASE), __phys_to_pfn(GPIO_BANK2_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(GPIO_BANK3_BASE), __phys_to_pfn(GPIO_BANK3_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(GPIO_BANK4_BASE), __phys_to_pfn(GPIO_BANK4_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(GPIO_BANK5_BASE), __phys_to_pfn(GPIO_BANK5_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(GPIO_BANK6_BASE), __phys_to_pfn(GPIO_BANK6_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(GPIO_BANK7_BASE), __phys_to_pfn(GPIO_BANK7_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(GPIO_BANK8_BASE), __phys_to_pfn(GPIO_BANK8_BASE), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(U8500_CLKRST1_BASE), __phys_to_pfn(U8500_CLKRST1_BASE),
	 SZ_4K, MT_DEVICE},
	{IO_ADDRESS(U8500_CLKRST2_BASE), __phys_to_pfn(U8500_CLKRST2_BASE),
	 SZ_4K, MT_DEVICE},
	{IO_ADDRESS(U8500_CLKRST3_BASE), __phys_to_pfn(U8500_CLKRST3_BASE),
	 SZ_4K, MT_DEVICE},
	{IO_ADDRESS(U8500_CLKRST5_BASE), __phys_to_pfn(U8500_CLKRST5_BASE),
	 SZ_4K, MT_DEVICE},
	{IO_ADDRESS(U8500_CLKRST6_BASE), __phys_to_pfn(U8500_CLKRST6_BASE),
	 SZ_4K, MT_DEVICE},
	{IO_ADDRESS(U8500_CLKRST7_BASE), __phys_to_pfn(U8500_CLKRST7_BASE),
	 SZ_4K, MT_DEVICE},
	{IO_ADDRESS(U8500_PRCMU_BASE), __phys_to_pfn(U8500_PRCMU_BASE),
	 SZ_4K, MT_DEVICE},
	{IO_ADDRESS(U8500_PRCMU_TCDM_BASE),
	__phys_to_pfn(U8500_PRCMU_TCDM_BASE), SZ_4K, MT_DEVICE},
	{IO_ADDRESS(U8500_BACKUPRAM0_BASE),
	 __phys_to_pfn(U8500_BACKUPRAM0_BASE), SZ_8K, MT_DEVICE},
	{IO_ADDRESS(U8500_B2R2_BASE), __phys_to_pfn(U8500_B2R2_BASE), SZ_4K,
	 MT_DEVICE},
};

static struct map_desc u8500_ed_io_desc[] __initdata = {
	{IO_ADDRESS(U8500_MTU0_BASE_ED), __phys_to_pfn(U8500_MTU0_BASE_ED), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(U8500_MTU1_BASE_ED), __phys_to_pfn(U8500_MTU1_BASE_ED), SZ_4K,
	 MT_DEVICE},
};

static struct map_desc u8500_v1_io_desc[] __initdata = {
	{IO_ADDRESS(U8500_MTU0_BASE_V1), __phys_to_pfn(U8500_MTU0_BASE_V1), SZ_4K,
	 MT_DEVICE},
	{IO_ADDRESS(U8500_MTU1_BASE_V1), __phys_to_pfn(U8500_MTU1_BASE_V1), SZ_4K,
	 MT_DEVICE},
};

static struct resource u8500_dma_resources[] = {
	[0] = {
		.start = U8500_DMA_BASE_V1,
		.end = U8500_DMA_BASE_V1 + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_DMA,
		.end = IRQ_DMA,
		.flags = IORESOURCE_IRQ}
};

static struct platform_device u8500_dma_device = {
	.name = "STM-DMA",
	.id = 0,
	.num_resources = 2,
	.resource = u8500_dma_resources
};

#define NUM_SSP_CLIENTS 10

static struct nmdk_spi_master_cntlr ssp0_platform_data = {
	.enable_dma = 1,
	.id = SSP_0_CONTROLLER,
	.num_chipselect = NUM_SSP_CLIENTS,
	.base_addr = U8500_SSP0_BASE,
	.rx_fifo_addr = U8500_SSP0_BASE + SSP_TX_RX_REG_OFFSET,
	.rx_fifo_dev_type = DMA_DEV_SSP0_RX,
	.tx_fifo_addr = U8500_SSP0_BASE + SSP_TX_RX_REG_OFFSET,
	.tx_fifo_dev_type = DMA_DEV_SSP0_TX,
	.gpio_alt_func = GPIO_ALT_SSP_0,
	.device_name = "ssp0",
};

static struct amba_device ssp0_device = {
	.dev = {
		.bus_id = "ssp0",
		.platform_data = &ssp0_platform_data,
		},
	.res = {
		.start = U8500_SSP0_BASE,
		.end = U8500_SSP0_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
		},
	.dma_mask = ~0,
	.irq = {IRQ_SSP0, NO_IRQ},
	.periphid = SSP_PER_ID,
};

static struct nmdk_spi_master_cntlr ssp1_platform_data = {
	.enable_dma = 1,
	.id = SSP_1_CONTROLLER,
	.num_chipselect = NUM_SSP_CLIENTS,
	.base_addr = U8500_SSP1_BASE,
	.rx_fifo_addr = U8500_SSP1_BASE + SSP_TX_RX_REG_OFFSET,
	.rx_fifo_dev_type = DMA_DEV_SSP1_RX,
	.tx_fifo_addr = U8500_SSP1_BASE + SSP_TX_RX_REG_OFFSET,
	.tx_fifo_dev_type = DMA_DEV_SSP1_TX,
	.gpio_alt_func = GPIO_ALT_SSP_1,
	.device_name = "ssp1",
};

static struct amba_device ssp1_device = {
	.dev = {
		.bus_id = "ssp1",
		.platform_data = &ssp1_platform_data,
		},
	.res = {
		.start = U8500_SSP1_BASE,
		.end = U8500_SSP1_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
		},
	.dma_mask = ~0,
	.irq = {IRQ_SSP1, NO_IRQ},
	.periphid = SSP_PER_ID,
};

#define NUM_SPI023_CLIENTS 10
static struct nmdk_spi_master_cntlr spi0_platform_data = {
	.enable_dma = 1,
	.id = SPI023_0_CONTROLLER,
	.num_chipselect = NUM_SPI023_CLIENTS,
	.base_addr = U8500_SPI0_BASE,
	.rx_fifo_addr = U8500_SPI0_BASE + SPI_TX_RX_REG_OFFSET,
	.rx_fifo_dev_type = DMA_DEV_SPI0_RX,
	.tx_fifo_addr = U8500_SPI0_BASE + SPI_TX_RX_REG_OFFSET,
	.tx_fifo_dev_type = DMA_DEV_SPI0_TX,
	.gpio_alt_func = GPIO_ALT_SSP_0,
	/*FIXME: using SSP for time being just for compilation */
	.device_name = "spi0",
};

static struct amba_device spi0_device = {
	.dev = {
		.bus_id = "spi0",
		.platform_data = &spi0_platform_data,
		},
	.res = {
		.start = U8500_SPI0_BASE,
		.end = U8500_SPI0_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
		},
	.dma_mask = ~0,
	.irq = {IRQ_SPI0, NO_IRQ},
	.periphid = SPI_PER_ID,
};

/* emmc specific configurations */
static int emmc_configure(struct amba_device *dev)
{
	int i;

	for (i = 197; i <= 207; i++)	{
		gpio_set_value(i, GPIO_HIGH);
		gpio_set_value(i, GPIO_PULLUP_DIS);
	}
	stm_gpio_altfuncenable(GPIO_ALT_EMMC);

	return 0;
}

static void emmc_restore_default(struct amba_device *dev)
{
	stm_gpio_altfuncdisable(GPIO_ALT_EMMC);
}

static struct mmc_board emmc_data = {
	.init = emmc_configure,
	.exit = emmc_restore_default,
	.dma_fifo_addr = U8500_SDI4_BASE + SD_MMC_TX_RX_REG_OFFSET,
	.dma_fifo_dev_type_rx = DMA_DEV_SD_MM4_RX,
	.dma_fifo_dev_type_tx = DMA_DEV_SD_MM4_TX,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA | MMC_CAP_MMC_HIGHSPEED,
#if CONFIG_REGULATOR
	.supply = "v-eMMC" /* tying to VAUX1 regulator */
#endif
};

static struct amba_device sdi4_device = {
	.dev = {
		.bus_id = "sdi4",
		.platform_data = &emmc_data,
	},
	.res = {
		.start = U8500_SDI4_BASE,
		.end = U8500_SDI4_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	.irq = {IRQ_SDMMC4, NO_IRQ },
	.periphid = SDI_PER_ID,
};

/* mmc specific configurations */
static int mmc_configure(struct amba_device *dev)
{
	int pin[2];
	int ret;
	int i;

	/* Level-shifter GPIOs */
	if (MOP500_PLATFORM_ID == platform_id)	{
		pin[0] = EGPIO_PIN_18;
		pin[1] = EGPIO_PIN_19;
	} else if (HREF_PLATFORM_ID == platform_id) {
		pin[0] = EGPIO_PIN_17;
		pin[1] = EGPIO_PIN_18;
	} else
		BUG();

	ret = gpio_request(pin[0], "level shifter");
	if (!ret)
		ret = gpio_request(pin[1], "level shifter");

	if (!ret) {
		gpio_direction_output(pin[0], 1);
		gpio_direction_output(pin[1], 1);

		gpio_set_value(pin[0], 1);
		gpio_set_value(pin[1], 1);
	} else
		dev_err(&dev->dev, "unable to configure gpios\n");

#if defined(CONFIG_GPIO_TC35892)
	if (HREF_PLATFORM_ID == platform_id) {
		/* Enabling the card detection interrupt */
		tc35892_set_gpio_intr_conf(EGPIO_PIN_3, EDGE_SENSITIVE,
				TC35892_BOTH_EDGE);
		tc35892_set_intr_enable(EGPIO_PIN_3, ENABLE_INTERRUPT);
	}
#endif

	for (i = 18; i <= 28; i++)	{
		gpio_set_value(i, GPIO_HIGH);
		gpio_set_value(i, GPIO_PULLUP_DIS);
	}

	stm_gpio_altfuncenable(GPIO_ALT_SDMMC);
	return ret;
}

static void mmc_set_power(struct device *dev, int power_on)
{
	if (platform_id == MOP500_PLATFORM_ID)
		gpio_set_value(EGPIO_PIN_18, !!power_on);
	else if (platform_id == HREF_PLATFORM_ID)
		gpio_set_value(EGPIO_PIN_17, !!power_on);
}

static void mmc_restore_default(struct amba_device *dev)
{

}

static int mmc_card_detect(void (*callback)(void *parameter), void *host)
{
	/*
	 * Card detection interrupt request
	 */
#if defined(CONFIG_GPIO_STMPE2401)
	if (MOP500_PLATFORM_ID == platform_id)
		stmpe2401_set_callback(EGPIO_PIN_16, callback, host);
#endif

#if defined(CONFIG_GPIO_TC35892)
	if (HREF_PLATFORM_ID == platform_id) {
		tc35892_set_gpio_intr_conf(EGPIO_PIN_3, EDGE_SENSITIVE, TC35892_BOTH_EDGE);
		tc35892_set_intr_enable(EGPIO_PIN_3, ENABLE_INTERRUPT);
		tc35892_set_callback(EGPIO_PIN_3, callback, host);
	}
#endif
	return 0;
}

static int mmc_get_carddetect_intr_value(void)
{
	int status = 0;

	if (MOP500_PLATFORM_ID == platform_id)
		status = gpio_get_value(EGPIO_PIN_16);
	else if (HREF_PLATFORM_ID == platform_id)
		status = gpio_get_value(EGPIO_PIN_3);

	return status;
}

static struct mmc_board mmc_data = {
	.init = mmc_configure,
	.exit = mmc_restore_default,
	.set_power = mmc_set_power,
	.card_detect = mmc_card_detect,
	.card_detect_intr_value = mmc_get_carddetect_intr_value,
	.dma_fifo_addr = U8500_SDI0_BASE + SD_MMC_TX_RX_REG_OFFSET,
	.dma_fifo_dev_type_rx = DMA_DEV_SD_MM0_RX,
	.dma_fifo_dev_type_tx = DMA_DEV_SD_MM0_TX,
	.level_shifter = 1,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED |
					MMC_CAP_MMC_HIGHSPEED,
};

static struct amba_device sdi0_device = {
	.dev = {
		.bus_id = "sdi0",
		.platform_data = &mmc_data,
	},
	.res = {
		.start = U8500_SDI0_BASE,
		.end = U8500_SDI0_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	.irq = {IRQ_SDMMC0, NO_IRQ },
		.periphid = SDI_PER_ID,
};


/* sdio specific configurations */
static int sdio_configure(struct amba_device *dev)
{
    int i;
    for (i = 208; i <= 214; i++) {
	gpio_set_value(i, GPIO_HIGH);
	gpio_set_value(i, GPIO_PULLUP_DIS);
    }
    stm_gpio_altfuncenable(GPIO_ALT_SDIO);
    /* enable WLAN_EN by making GPIO215 HIGH */
    gpio_direction_output(215, GPIO_HIGH);
    gpio_set_value(215, GPIO_HIGH);

    return 0;
}
static void sdio_restore_default(struct amba_device *dev)
{
    stm_gpio_altfuncdisable(GPIO_ALT_SDIO);
}

static struct mmc_board sdi1_data = {
	.init = sdio_configure,
	.exit = sdio_restore_default,
	.dma_fifo_addr = U8500_SDI1_BASE + SD_MMC_TX_RX_REG_OFFSET,
	.dma_fifo_dev_type_rx = DMA_DEV_SD_MM1_RX,
	.dma_fifo_dev_type_tx = DMA_DEV_SD_MM1_TX,
	.level_shifter = 0,
#ifdef CONFIG_U8500_SDIO_CARD_IRQ
	.caps = (MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ),
#else
	.caps = MMC_CAP_4_BIT_DATA,
#endif
	.is_sdio = 1,
};

static struct amba_device sdi1_device = {
	.dev = {
	    .bus_id = "sdi1",
	    .platform_data = &sdi1_data,
	},
	.res = {
	    .start = U8500_SDI1_BASE,
	    .end = U8500_SDI1_BASE + SZ_4K - 1,
	    .flags = IORESOURCE_MEM,
	},
	.irq = {IRQ_SDMMC1, NO_IRQ },
	.periphid = SDI_PER_ID,
};
static int sdi2_init(struct amba_device *dev)
{
	int i;

	for (i = 128; i <= 138; i++)	{
		gpio_set_value(i, GPIO_HIGH);
		gpio_set_value(i, GPIO_PULLUP_DIS);
	}
	stm_gpio_altfuncenable(GPIO_ALT_SDMMC2);
	return 0;
}

static void sdi2_exit(struct amba_device *dev)
{
	stm_gpio_altfuncdisable(GPIO_ALT_SDMMC2);
}

static struct mmc_board sdi2_data = {
	.init = sdi2_init,
	.exit = sdi2_exit,
	.dma_fifo_addr = U8500_SDI2_BASE + SD_MMC_TX_RX_REG_OFFSET,
	.dma_fifo_dev_type_rx = DMA_DEV_SD_MM2_RX,
	.dma_fifo_dev_type_tx = DMA_DEV_SD_MM2_TX,
	.level_shifter = 0,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA,
};

static struct amba_device sdi2_device = {
	.dev = {
		.bus_id = "sdi2",
		.platform_data = &sdi2_data,
	},
	.res = {
		.start = U8500_SDI2_BASE,
		.end = U8500_SDI2_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	.irq = {IRQ_SDMMC2, NO_IRQ},
	.periphid = SDI_PER_ID,
};

static struct resource ab8500_resources[] = {
	[0] = {
		.start = STW4500_IRQ,
		.end = STW4500_IRQ,
		.flags = IORESOURCE_IRQ
	}
};

/**
 * ab8500_spi_cs_enable - disables the chip select for STw4500
 */
void ab8500_spi_cs_enable(void)
{

}

/**
 * ab8500_spi_cs_disable - enables the chip select for STw4500
 */
void ab8500_spi_cs_disable(void)
{

}

static struct ab8500_device ab8500_board = {
	.cs_en = ab8500_spi_cs_enable,
	.cs_dis = ab8500_spi_cs_disable,
	.ssp_controller = SSP_0_CONTROLLER,
};

static struct platform_device ab8500_device = {
	.name = "ab8500",
	.id = 0,
	.dev = {
		.platform_data = &ab8500_board,
	},
	.num_resources = 1,
	.resource = ab8500_resources,
};

static struct platform_device ab8500_gpadc_device = {
	.name = "ab8500_gpadc"
};

static struct platform_device ab8500_bm_device = {
	.name = "ab8500_bm"
};

#if  defined(CONFIG_USB_MUSB_HOST)
#define MUSB_MODE	MUSB_HOST
#elif defined(CONFIG_USB_MUSB_PERIPHERAL)
#define MUSB_MODE	MUSB_PERIPHERAL
#elif defined(CONFIG_USB_MUSB_OTG)
#define MUSB_MODE	MUSB_OTG
#else
#define MUSB_MODE	MUSB_UNDEFINED
#endif
static struct musb_hdrc_config musb_hdrc_hs_otg_config = {
	.multipoint	= true,	/* multipoint device */
	.dyn_fifo	= true,	/* supports dynamic fifo sizing */
	.num_eps	= 16,	/* number of endpoints _with_ ep0 */
	.ram_bits	= 16,	/* ram address size */
};

static struct musb_hdrc_platform_data musb_hdrc_hs_otg_platform_data = {
	.mode	= MUSB_MODE,
	.clock	= "usb",	/* for clk_get() */
	.config = &musb_hdrc_hs_otg_config,
};

static struct resource usb_resources[] = {
	[0] = {
		.name	= "usb-mem",
		.start	=  U8500_USBOTG_BASE,
		.end	=  (U8500_USBOTG_BASE + SZ_64K - 1),
		.flags	=  IORESOURCE_MEM,
	},

	[1] = {
		.name   = "usb-irq",
		.start	= IRQ_USBOTG,
		.end	= IRQ_USBOTG,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device musb_device = {
	.name = "musb_hdrc",
	.id = 0,
	.dev = {
		.bus_id	= "musb_hdrc.0",	/* for clk_get() */
		.platform_data = &musb_hdrc_hs_otg_platform_data,
		.dma_mask = (u64 *)0xFFFFFFFF,
		.coherent_dma_mask = (u64)0xFFFFFFFF
	},
	.num_resources = ARRAY_SIZE(usb_resources),
	.resource = usb_resources,
};

static void __init u8500_map_io(void)
{
	iotable_init(u8500_common_io_desc, ARRAY_SIZE(u8500_common_io_desc));

	if (u8500_is_earlydrop())
		iotable_init(u8500_ed_io_desc, ARRAY_SIZE(u8500_ed_io_desc));
	else
		iotable_init(u8500_v1_io_desc, ARRAY_SIZE(u8500_v1_io_desc));
}

static void __init u8500_gic_init_irq(void)
{
	gic_dist_init(0, (void __iomem *)IO_ADDRESS(U8500_GIC_DIST_BASE), 29);
	gic_cpu_init(0, (void __iomem *)IO_ADDRESS(U8500_GIC_CPU_BASE));

	/*
	 * Init clocks here so that they are available for system timer
	 * initialization.
	 */
	clk_init();
}

static void uart0_init(void)
{
	stm_gpio_altfuncenable(GPIO_ALT_UART_0_NO_MODEM);
}

static void uart0_exit(void)
{
	stm_gpio_altfuncdisable(GPIO_ALT_UART_0_NO_MODEM);
}

struct uart_amba_plat_data uart0_plat = {
	.init = uart0_init,
	.exit = uart0_exit,
};

static void uart1_init(void)
{
	stm_gpio_altfuncenable(GPIO_ALT_UART_1);
}

static void uart1_exit(void)
{
	stm_gpio_altfuncdisable(GPIO_ALT_UART_1);
}

struct uart_amba_plat_data uart1_plat = {
	.init = uart1_init,
	.exit = uart1_exit,
};

static void uart2_init(void)
{
	stm_gpio_altfuncenable(GPIO_ALT_UART_2);
}

static void uart2_exit(void)
{
	stm_gpio_altfuncdisable(GPIO_ALT_UART_2);
}

struct uart_amba_plat_data uart2_plat = {
	.init = uart2_init,
	.exit = uart2_exit,
};

#define __MEM_4K_RESOURCE(x) \
	.res = {.start = (x), .end = (x) + SZ_4K - 1, .flags = IORESOURCE_MEM}

/* These are active devices on this board, FIXME
 * move this to board file
 */
#if defined(CONFIG_MACH_U5500_SIMULATOR)
/* Remap of uart0 and uart2 when using SVP5500
 * remove this when uart2 problem solved in SVP5500
 */
static struct amba_device uart2_device = {
	.dev = {.bus_id = "uart2", .platform_data = &uart0_plat, },
	__MEM_4K_RESOURCE(U8500_UART0_BASE),
	.irq = {IRQ_UART0, NO_IRQ},
};

static struct amba_device uart1_device = {
	.dev = {.bus_id = "uart1", .platform_data = &uart1_plat, },
	__MEM_4K_RESOURCE(U8500_UART1_BASE),
	.irq = {IRQ_UART1, NO_IRQ},
};

static struct amba_device uart0_device = {
	.dev = {.bus_id = "uart0", .platform_data = &uart2_plat, },
	__MEM_4K_RESOURCE(U8500_UART2_BASE),
	.irq = {IRQ_UART2, NO_IRQ},
};
#else
static struct amba_device uart0_device = {
	.dev = {.bus_id = "uart0", .platform_data = &uart0_plat, },
	__MEM_4K_RESOURCE(U8500_UART0_BASE),
	.irq = {IRQ_UART0, NO_IRQ},
};

static struct amba_device uart1_device = {
	.dev = {.bus_id = "uart1", .platform_data = &uart1_plat, },
	__MEM_4K_RESOURCE(U8500_UART1_BASE),
	.irq = {IRQ_UART1, NO_IRQ},
};

static struct amba_device uart2_device = {
	.dev = {.bus_id = "uart2", .platform_data = &uart2_plat, },
	__MEM_4K_RESOURCE(U8500_UART2_BASE),
	.irq = {IRQ_UART2, NO_IRQ},
};

#endif

static struct platform_device *core_devices[] __initdata = {
	&msp0_device,
	&msp1_device,
	&msp2_device,
	&u8500_i2c_0_controller,
#if !defined(CONFIG_MACH_U5500_SIMULATOR)
	&u8500_i2c_1_controller,
	&u8500_i2c_2_controller,
	&u8500_i2c_3_controller,
#endif
	&u8500_dma_device,
#if !defined(CONFIG_MACH_U8500_SIMULATOR)
	&u8500_hsit_device,
	&u8500_hsir_device,
	&u8500_shrm_device,
	&ab8500_device,
	&ab8500_gpadc_device,
	&ab8500_bm_device,
	&musb_device,
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
	&mcde2_device,
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELC0 */
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1
	&mcde3_device,
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELC1 */
#ifdef CONFIG_FB_U8500_MCDE_CHANNELB
	&mcde1_device,
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELB */
#ifdef CONFIG_FB_U8500_MCDE_CHANNELA
	&mcde0_device,
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELA */
	&b2r2_device,
	&pmem_device,
	&pmem_mio_device,
	&pmem_hwb_device,
#endif
};

static struct platform_device *core_v1_devices[] __initdata = {
	&u8500_i2c_4_controller,
};

static struct amba_device *amba_v1_devs[] __initdata = {
	&sdi2_device,	/* POP eMMC */
};

static struct amba_device *amba_devs[] __initdata = {
	&gpio0_device,
	&gpio1_device,
	&gpio2_device,
	&gpio3_device,
#if defined(CONFIG_MACH_U5500_SIMULATOR)
	&gpio4_device,
#endif
	&uart0_device,
	&uart1_device,
	&uart2_device,
#if !defined(CONFIG_MACH_U5500_SIMULATOR)
	&ssp0_device,
	&ssp1_device,
	&spi0_device,
	&msp2_spi_device,
	&sdi4_device,	/* On-board eMMC */
	&sdi0_device,	/* SD/MMC card */
#ifdef CONFIG_U8500_SDIO
	&sdi1_device, /* SDIO card */
#endif
	&rtc_device,
#endif
};
static struct i2s_board_info stm_i2s_board_info[] __initdata = {
	{
		/* Particular name can be given */
		.modalias	= "i2s_device.0",
		.platform_data	= NULL, /* can be fill some data */
		.id		= (MSP_0_CONTROLLER - 1),
		.chip_select	= 0,
	},
	{
		.modalias	= "i2s_device.1",
		.platform_data	= NULL, /* can be fill some data */
		.id		= (MSP_1_CONTROLLER - 1),
		.chip_select	= 1,
	},
	{
		.modalias	= "i2s_device.2",
		.platform_data	= NULL, /* can be fill some data */
		.id		= (MSP_2_CONTROLLER - 1),
		.chip_select	= 2,
	}
};

#ifdef CONFIG_CACHE_L2X0
static int __init u8500_l2x0_init(void)
{
	l2x0_init((void *)IO_ADDRESS(U8500_L2CC_BASE), 0x3e060000, 0x3e060000);
	return 0;
}
early_initcall(u8500_l2x0_init);
#endif

static void __init u8500_earlydrop_fixup(void)
{
	u8500_dma_resources[0].start = U8500_DMA_BASE_ED;
	u8500_dma_resources[0].end = U8500_DMA_BASE_ED + SZ_4K - 1;
	u8500_shrm_resources[1].start = IRQ_CA_WAKE_REQ_ED;
	u8500_shrm_resources[1].end = IRQ_CA_WAKE_REQ_ED;
	u8500_shrm_resources[2].start = IRQ_AC_READ_NOTIFICATION_0_ED;
	u8500_shrm_resources[2].end = IRQ_AC_READ_NOTIFICATION_0_ED;
	u8500_shrm_resources[3].start = IRQ_AC_READ_NOTIFICATION_1_ED;
	u8500_shrm_resources[3].end = IRQ_AC_READ_NOTIFICATION_1_ED;
	u8500_shrm_resources[4].start = IRQ_CA_MSG_PEND_NOTIFICATION_0_ED;
	u8500_shrm_resources[4].end = IRQ_CA_MSG_PEND_NOTIFICATION_0_ED;
	u8500_shrm_resources[5].start = IRQ_CA_MSG_PEND_NOTIFICATION_1_ED;
	u8500_shrm_resources[5].end = IRQ_CA_MSG_PEND_NOTIFICATION_1_ED;
#ifdef CONFIG_FB_U8500_MCDE_CHANNELB
	mcde1_channel_data.gpio_alt_func = GPIO_ALT_LCD_PANELB_ED;
#endif
}

static void __init amba_add_devices(struct amba_device *devs[], int num)
{
	int i;

	for (i = 0; i < num; i++) {
		struct amba_device *d = devs[i];
		amba_device_register(d, &iomem_resource);
	}
}

/* U8500 Power section */
#if CONFIG_REGULATOR

#define U8500_VAPE_REGULATOR_MIN_VOLTAGE       (1800000)
#define U8500_VAPE_REGULATOR_MAX_VOLTAGE       (2000000)

static int db8500_vape_regulator_init(void)
{ return 0; }

/* tying VAPE regulator to symbolic consumer devices */
static struct regulator_consumer_supply db8500_vape_consumers[] = {
};

/* VAPE supply, for interconnect */
static struct regulator_init_data db8500_vape_init = {
	.supply_regulator_dev = NULL,
	.constraints = {
		.name = "DB8500-VAPE",
		.min_uV = U8500_VAPE_REGULATOR_MIN_VOLTAGE,
		.max_uV = U8500_VAPE_REGULATOR_MAX_VOLTAGE,
		.input_uV = 1, /* notional, for set_mode* */
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE|
			REGULATOR_CHANGE_MODE|REGULATOR_CHANGE_DRMS,
		.valid_modes_mask = REGULATOR_MODE_NORMAL|REGULATOR_MODE_IDLE,
	},
	.num_consumer_supplies = ARRAY_SIZE(db8500_vape_consumers),
	.regulator_init = db8500_vape_regulator_init,
	.consumer_supplies = db8500_vape_consumers,
};

static struct platform_device db8500_vape_regulator_dev = {
	.name = "db8500-regulator",
	.id   = 0,
	.dev  = {
		.platform_data = &db8500_vape_init,
	},
};

/* VANA Supply, for analogue part of displays */
#define U8500_VANA_REGULATOR_MIN_VOLTAGE	(0)
#define U8500_VANA_REGULATOR_MAX_VOLTAGE	(1200000)

static int db8500_vana_regulator_init(void) { return 0; }

static struct regulator_consumer_supply db8500_vana_consumers[] = {
#ifdef CONFIG_FB_U8500_MCDE_CHANNELA
	{
		.dev = &mcde0_device.dev,
		.supply = "v-ana",
	},
#endif
#ifdef CONFIG_FB_U8500_MCDE_CHANNELB
	{
		.dev = &mcde1_device.dev,
		.supply = "v-ana",
	},
#endif
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
	{
		.dev = &mcde2_device.dev,
		.supply = "v-ana",
	},
#endif
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1
	{
		.dev = &mcde3_device.dev,
		.supply = "v-ana",
	},
#endif
};

static struct regulator_init_data db8500_vana_init = {
	.supply_regulator_dev = NULL,
	.constraints = {
		.name = "VANA",
		.min_uV = U8500_VANA_REGULATOR_MIN_VOLTAGE,
		.max_uV = U8500_VANA_REGULATOR_MAX_VOLTAGE,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
			REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_NORMAL |
				REGULATOR_MODE_IDLE,
	},
	.num_consumer_supplies = ARRAY_SIZE(db8500_vana_consumers),
	.regulator_init = db8500_vana_regulator_init,
	.consumer_supplies = db8500_vana_consumers,
};

static struct platform_device db8500_vana_regulator_dev = {
	.name = "db8500-regulator",
	.id   = 1,
	.dev  = {
		.platform_data = &db8500_vana_init,
	},
};

/* VAUX1 supply */
#define AB8500_VAUXN_LDO_MIN_VOLTAGE    (1100000)
#define AB8500_VAUXN_LDO_MAX_VOLTAGE    (3300000)

static int ab8500_vaux1_regulator_init(void) { return 0; }

static struct regulator_consumer_supply ab8500_vaux1_consumers[] = {
#ifdef CONFIG_FB_U8500_MCDE_CHANNELA
	{
		.dev = &mcde0_device.dev,
		.supply = "v-mcde",
	},
#endif
#ifdef CONFIG_FB_U8500_MCDE_CHANNELB
	{
		.dev = &mcde1_device.dev,
		.supply = "v-mcde",
	},
#endif
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
	{
		.dev = &mcde2_device.dev,
		.supply = "v-mcde",
	},
#endif
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1
	{
		.dev = &mcde3_device.dev,
		.supply = "v-mcde",
	},
#endif
};

static struct regulator_init_data ab8500_vaux1_init = {
	.supply_regulator_dev = NULL,
	.constraints = {
		.name = "AB8500-VAUX1",
		.min_uV = AB8500_VAUXN_LDO_MIN_VOLTAGE,
		.max_uV = AB8500_VAUXN_LDO_MIN_VOLTAGE,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE|
			REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_NORMAL|REGULATOR_MODE_IDLE,
	},
	.num_consumer_supplies = ARRAY_SIZE(ab8500_vaux1_consumers),
	.regulator_init = ab8500_vaux1_regulator_init,
	.consumer_supplies = ab8500_vaux1_consumers,
};

/* as the vaux2 is not a u8500 specific regulator, it is named
 * as the ab8500 series */
static struct platform_device ab8500_vaux1_regulator_dev = {
	.name = "ab8500-regulator",
	.id   = 0,
	.dev  = {
		.platform_data = &ab8500_vaux1_init,
	},
};

/* VAUX2 supply */
static int ab8500_vaux2_regulator_init(void) { return 0; }

/* supply for on-board eMMC */
static struct regulator_consumer_supply ab8500_vaux2_consumers[] = {
	{
		.dev    = &sdi4_device.dev,
		.supply = "v-eMMC",
	}
};

static struct regulator_init_data ab8500_vaux2_init = {
	.supply_regulator_dev = NULL,
	.constraints = {
		.name = "AB8500-VAUX2",
		.min_uV = AB8500_VAUXN_LDO_MIN_VOLTAGE,
		.max_uV = AB8500_VAUXN_LDO_MAX_VOLTAGE,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE|
			REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_NORMAL|REGULATOR_MODE_IDLE,
	},
	.num_consumer_supplies = ARRAY_SIZE(ab8500_vaux2_consumers),
	.regulator_init = ab8500_vaux2_regulator_init,
	.consumer_supplies = ab8500_vaux2_consumers,
};

static struct platform_device ab8500_vaux2_regulator_dev = {
	.name = "ab8500-regulator",
	.id   = 1,
	.dev  = {
		.platform_data = &ab8500_vaux2_init,
	},
};

/* supply for tvout, gpadc, TVOUT LDO */
#define AB8500_VTVOUT_LDO_MIN_VOLTAGE        (0)
#define AB8500_VTVOUT_LDO_MAX_VOLTAGE        (2000000)

static int ab8500_vtvout_regulator_init(void) { return 0; }

static struct regulator_consumer_supply ab8500_vtvout_consumers[] = {
};

static struct regulator_init_data ab8500_vtvout_init = {
	.supply_regulator_dev = NULL,
	.constraints = {
		.name = "AB8500-VTVOUT",
		.min_uV = AB8500_VTVOUT_LDO_MIN_VOLTAGE,
		.max_uV = AB8500_VTVOUT_LDO_MAX_VOLTAGE,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE|
			REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_NORMAL|REGULATOR_MODE_IDLE,
	},
	.num_consumer_supplies = ARRAY_SIZE(ab8500_vtvout_consumers),
	.regulator_init = ab8500_vtvout_regulator_init,
	.consumer_supplies = ab8500_vtvout_consumers,
};

static struct platform_device ab8500_vtvout_regulator_dev = {
	.name = "ab8500-regulator",
	.id   = 2,
	.dev  = {
		.platform_data = &ab8500_vtvout_init,
	},
};

/* supply for usb, VBUS LDO */
#define AB8500_VBUS_REGULATOR_MIN_VOLTAGE        (0)
#define AB8500_VBUS_REGULATOR_MAX_VOLTAGE        (5000000)

static int ab8500_vbus_regulator_init(void) { return 0; }

static struct regulator_consumer_supply ab8500_vbus_consumers[] = {
};

static struct regulator_init_data ab8500_vbus_init = {
	.supply_regulator_dev = NULL,
	.constraints = {
		.name = "AB8500-VBUS",
		.min_uV = AB8500_VBUS_REGULATOR_MIN_VOLTAGE,
		.max_uV = AB8500_VBUS_REGULATOR_MAX_VOLTAGE,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE|
			REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_NORMAL|REGULATOR_MODE_IDLE,
	},
	.num_consumer_supplies = ARRAY_SIZE(ab8500_vbus_consumers),
	.regulator_init = ab8500_vbus_regulator_init,
	.consumer_supplies = ab8500_vbus_consumers,
};

static struct platform_device ab8500_vbus_regulator_dev = {
	.name = "ab8500-regulator",
	.id   = 3,
	.dev  = {
		.platform_data = &ab8500_vbus_init,
	},
};

static struct platform_device *u8500_regulators[] = {
	&db8500_vape_regulator_dev,
	&db8500_vana_regulator_dev,
	&ab8500_vaux1_regulator_dev,
	&ab8500_vaux2_regulator_dev,
	&ab8500_vtvout_regulator_dev,
	&ab8500_vbus_regulator_dev,
};

#endif

static void __init u8500_platform_init(void)
{
#if CONFIG_REGULATOR
	/* we want the on-chip regulator before any device registration */
	platform_add_devices(u8500_regulators, ARRAY_SIZE(u8500_regulators));
#endif

	if (u8500_is_earlydrop())
		u8500_earlydrop_fixup();
	else {
		amba_add_devices(amba_v1_devs, ARRAY_SIZE(amba_v1_devs));
		platform_add_devices(core_v1_devices,
				     ARRAY_SIZE(core_v1_devices));
	}

	amba_add_devices(amba_devs, ARRAY_SIZE(amba_devs));
	platform_add_devices(core_devices, ARRAY_SIZE(core_devices));

#if !defined(CONFIG_MACH_U5500_SIMULATOR)
	i2s_register_board_info(stm_i2s_board_info,
			ARRAY_SIZE(stm_i2s_board_info));

	add_u8500_platform_devices();

	/* enable RTC as a wakeup capable */
	device_init_wakeup(&rtc_device.dev, true);

	/* enable all the alternate gpio's for all UART's
	 * FIXME - This should go in board files
	 */
	stm_gpio_altfuncenable(GPIO_ALT_UART_0_NO_MODEM);
	stm_gpio_altfuncenable(GPIO_ALT_UART_1);
	stm_gpio_altfuncenable(GPIO_ALT_UART_2);
#endif
}

extern struct sys_timer u8500_timer;

/* TODO: Here I used NOMADIK name, U8500 is not yet in 2.6.29
 * but is already registered in the kernel version > 2.6.31
 * till that time I'm forced to keep NOMADIK - srinidhi
 */
MACHINE_START(NOMADIK, "ST Ericsson U8500 Platform")
	/* Maintainer: ST-Ericsson */
#if defined(CONFIG_MACH_U5500_SIMULATOR)
	.phys_io = U8500_UART0_BASE,
	.io_pg_offst = (IO_ADDRESS(U8500_UART0_BASE) >> 18) & 0xfffc,
#else
	.phys_io = U8500_UART2_BASE,
	.io_pg_offst = (IO_ADDRESS(U8500_UART2_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = 0x00000100,
	.map_io = u8500_map_io,
	.init_irq = u8500_gic_init_irq,
	.timer = &u8500_timer,
	.init_machine = u8500_platform_init,
MACHINE_END
