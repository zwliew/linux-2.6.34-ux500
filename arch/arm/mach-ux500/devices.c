/*
 * Copyright (C) 2009 ST-Ericsson SA
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
#include <linux/gpio.h>
#include <linux/usb/musb.h>
#include <linux/regulator/machine.h>
#include <linux/dma-mapping.h>

#include <asm/irq.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <mach/irqs.h>
#include <mach/hardware.h>
#include <mach/devices.h>
#include <asm/dma.h>
#include <mach/dma.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-stm.h>
#include <linux/mmc/host.h>
#include <asm/setup.h>
#include <linux/android_pmem.h>
#include <mach/msp.h>
#include <mach/i2c.h>
#include <mach/shrm.h>
#include <mach/mmc.h>
#include <mach/ab8500.h>
#include <mach/stmpe2401.h>
#include <mach/tc35892.h>
#include <mach/uart.h>
#include <mach/setup.h>

void __init u8500_register_device(struct platform_device *dev, void *data)
{
	int ret;

	dev->dev.platform_data = data;

	ret = platform_device_register(dev);
	if (ret)
		dev_err(&dev->dev, "unable to register device: %d\n", ret);
}

void __init u8500_register_amba_device(struct amba_device *dev, void *data)
{
	int ret;

	dev->dev.platform_data = data;

	ret = amba_device_register(dev, &iomem_resource);
	if (ret)
		dev_err(&dev->dev, "unable to register device: %d\n", ret);
}

#ifdef CONFIG_UX500_SOC_DB8500
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
struct platform_device u8500_msp0_device = {
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
struct platform_device u8500_msp1_device = {
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

struct platform_device u8500_msp2_device = {
	.name = "MSP_I2S",
	.id = 2,
	.num_resources = 2,
	.resource = u8500_msp_2_resources,
	.dev = {
		.bus_id = "msp2",
		.platform_data = &msp2_platform_data,
	},
};
#endif

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

struct amba_device u8500_msp2_spi_device = {
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

#define UX500_I2C_RESOURCES(id, size)		\
static struct resource ux500_i2c_resources_##id[] = {	\
	[0] = {					\
		.start	= UX500_I2C##id##_BASE,	\
		.end	= UX500_I2C##id##_BASE + size - 1, \
		.flags	= IORESOURCE_MEM,	\
	},					\
	[1] = {					\
		.start	= IRQ_I2C##id,		\
		.end	= IRQ_I2C##id,		\
		.flags	= IORESOURCE_IRQ	\
	}					\
}


UX500_I2C_RESOURCES(1, SZ_4K);
UX500_I2C_RESOURCES(2, SZ_4K);
UX500_I2C_RESOURCES(3, SZ_4K);

#define UX500_I2C_PDEVICE(cid)		\
 struct platform_device ux500_i2c_controller##cid = { \
	.name = "nmk-i2c",		\
	.id	 = cid,			\
	.num_resources = 2,		\
	.resource = ux500_i2c_resources_##cid	\
}

UX500_I2C_PDEVICE(1);
UX500_I2C_PDEVICE(2);
UX500_I2C_PDEVICE(3);


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

struct platform_device u8500_shrm_device = {
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
		.start	= UX500_B2R2_BASE,
		.end	= UX500_B2R2_BASE + ((4*1024)-1),
		.name	= "b2r2_base",
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= PRCM_B2R2CLK_MGT_REG,
		.end	= PRCM_B2R2CLK_MGT_REG + (sizeof(u32) - 1),
		.name	= "prcm_b2r2_clk",
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device ux500_b2r2_device = {
	.name	= "U8500-B2R2",
	.id	= 0,
	.dev	= {
		.bus_id = "b2r2_bus",
		.coherent_dma_mask = ~0,
	},
	.num_resources	= ARRAY_SIZE(b2r2_resources),
	.resource	= b2r2_resources,
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

struct platform_device u8500_pmem_device = {
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
	.cached = 1,
	.start = 0,
	.size = 0,
};

static void __init early_pmem_mio(char **p)
{
	early_pmem_generic_parse(p, &pmem_mio_pdata);
}
__early_param("pmem_mio=", early_pmem_mio);

struct platform_device u8500_pmem_mio_device = {
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
	.cached = 1,
	.start = 0,
	.size = 0,
};

static void __init early_pmem_hwb(char **p)
{
	early_pmem_generic_parse(p, &pmem_hwb_pdata);
}
__early_param("pmem_hwb=", early_pmem_hwb);

struct platform_device u8500_pmem_hwb_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = {
		.platform_data = &pmem_hwb_pdata,
	},
};

struct amba_device ux500_rtc_device = {
	.dev		= {
		.bus_id = "mb:15",
	},
	.res		= {
		.start	= UX500_RTC_BASE,
		.end	= UX500_RTC_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.irq		= {IRQ_RTC_RTT, NO_IRQ},
	.periphid	= RTC_PER_ID,
};

static struct resource ux500_dma_resources[] = {
	[0] = {
		.start	= UX500_DMA_BASE,
		.end	= UX500_DMA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_DMA,
		.end	= IRQ_DMA,
		.flags	= IORESOURCE_IRQ
	},
};

struct platform_device ux500_dma_device = {
	.name		= "STM-DMA",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ux500_dma_resources),
	.resource	= ux500_dma_resources
};

#define NUM_SPI023_CLIENTS 10
static struct nmdk_spi_master_cntlr spi0_platform_data = {
	.enable_dma		= 1,
	.id			= SPI023_0_CONTROLLER,
	.num_chipselect 	= NUM_SPI023_CLIENTS,
	.base_addr		= UX500_SPI0_BASE,
	.rx_fifo_addr		= UX500_SPI0_BASE + SPI_TX_RX_REG_OFFSET,
	.rx_fifo_dev_type	= DMA_DEV_SPI0_RX,
	.tx_fifo_addr		= UX500_SPI0_BASE + SPI_TX_RX_REG_OFFSET,
	.tx_fifo_dev_type	= DMA_DEV_SPI0_TX,
	.gpio_alt_func		= GPIO_ALT_SSP_0,
	.device_name		= "spi0",
};

struct amba_device ux500_spi0_device = {
	.dev		= {
		.bus_id	= "spi0",
		.platform_data = &spi0_platform_data,
	},
	.res		= {
		.start	= UX500_SPI0_BASE,
		.end	= UX500_SPI0_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.dma_mask	= DMA_BIT_MASK(32),
	.irq		= {IRQ_SPI0, NO_IRQ},
	.periphid	= SPI_PER_ID,
};

struct amba_device ux500_sdi0_device = {
	.dev		= {
		.init_name = "sdi0",
	},
	.res		= {
		.start	= UX500_SDI0_BASE,
		.end	= UX500_SDI0_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.irq		= {IRQ_SDMMC0, NO_IRQ},
	.periphid	= SDI_PER_ID,
};

struct amba_device ux500_sdi1_device = {
	.dev		= {
		.init_name = "sdi1",
	},
	.res		= {
		.start	= UX500_SDI1_BASE,
		.end	= UX500_SDI1_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.irq		= {IRQ_SDMMC1, NO_IRQ},
	.periphid	= SDI_PER_ID,
};

struct amba_device ux500_sdi2_device = {
	.dev		= {
		.init_name = "sdi2",
	},
	.res		= {
		.start	= UX500_SDI2_BASE,
		.end	= UX500_SDI2_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.irq		= {IRQ_SDMMC2, NO_IRQ},
	.periphid	= SDI_PER_ID,
};

struct amba_device ux500_sdi4_device = {
	.dev 		= {
		.init_name = "sdi4",
	},
	.res 		= {
		.start	= UX500_SDI4_BASE,
		.end	= UX500_SDI4_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.irq		= {IRQ_SDMMC4, NO_IRQ},
	.periphid	= SDI_PER_ID,
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

struct platform_device u8500_ab8500_device = {
	.name = "ab8500",
	.id = 0,
	.dev = {
		.platform_data = &ab8500_board,
	},
	.num_resources = 1,
	.resource = ab8500_resources,
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
		.start	=  UX500_USBOTG_BASE,
		.end	=  (UX500_USBOTG_BASE + SZ_64K - 1),
		.flags	=  IORESOURCE_MEM,
	},

	[1] = {
		.name   = "usb-irq",
		.start	= IRQ_USBOTG,
		.end	= IRQ_USBOTG,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device ux500_musb_device = {
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
struct amba_device ux500_uart2_device = {
	.dev = {.bus_id = "uart2", .platform_data = &uart0_plat, },
	__MEM_4K_RESOURCE(UX500_UART0_BASE),
	.irq = {IRQ_UART0, NO_IRQ},
};

struct amba_device ux500_uart1_device = {
	.dev = {.bus_id = "uart1", .platform_data = &uart1_plat, },
	__MEM_4K_RESOURCE(UX500_UART1_BASE),
	.irq = {IRQ_UART1, NO_IRQ},
};

struct amba_device ux500_uart0_device = {
	.dev = {.bus_id = "uart0", .platform_data = &uart2_plat, },
	__MEM_4K_RESOURCE(UX500_UART2_BASE),
	.irq = {IRQ_UART2, NO_IRQ},
};
#else
struct amba_device ux500_uart0_device = {
	.dev = {.bus_id = "uart0", .platform_data = &uart0_plat, },
	__MEM_4K_RESOURCE(UX500_UART0_BASE),
	.irq = {IRQ_UART0, NO_IRQ},
};

struct amba_device ux500_uart1_device = {
	.dev = {.bus_id = "uart1", .platform_data = &uart1_plat, },
	__MEM_4K_RESOURCE(UX500_UART1_BASE),
	.irq = {IRQ_UART1, NO_IRQ},
};

struct amba_device ux500_uart2_device = {
	.dev = {.bus_id = "uart2", .platform_data = &uart2_plat, },
	__MEM_4K_RESOURCE(UX500_UART2_BASE),
	.irq = {IRQ_UART2, NO_IRQ},
};

#endif

void __init amba_add_devices(struct amba_device *devs[], int num)
{
	int i;

	for (i = 0; i < num; i++) {
		struct amba_device *d = devs[i];
		amba_device_register(d, &iomem_resource);
	}
}

/* U8500 Power section */
#ifdef CONFIG_REGULATOR

#define U8500_VAPE_REGULATOR_MIN_VOLTAGE       (1800000)
#define U8500_VAPE_REGULATOR_MAX_VOLTAGE       (2000000)

/* tying VAPE regulator to symbolic consumer devices */
static struct regulator_consumer_supply db8500_vape_consumers[] = {
	{
		.supply = "v-ape",
	},
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

static struct regulator_consumer_supply db8500_vana_consumers[] = {
#ifdef CONFIG_FB_U8500_MCDE_CHANNELA
	{
		.dev = &u8500_mcde0_device.dev,
		.supply = "v-ana",
	},
#endif
#ifdef CONFIG_FB_U8500_MCDE_CHANNELB
	{
		.dev = &u8500_mcde1_device.dev,
		.supply = "v-ana",
	},
#endif
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
	{
		.dev = &u8500_mcde2_device.dev,
		.supply = "v-ana",
	},
#endif
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1
	{
		.dev = &u8500_mcde3_device.dev,
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

static struct regulator_consumer_supply ab8500_vaux1_consumers[] = {
#ifdef CONFIG_FB_U8500_MCDE_CHANNELA
	{
		.dev = &u8500_mcde0_device.dev,
		.supply = "v-mcde",
	},
#endif
#ifdef CONFIG_FB_U8500_MCDE_CHANNELB
	{
		.dev = &u8500_mcde1_device.dev,
		.supply = "v-mcde",
	},
#endif
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
	{
		.dev = &u8500_mcde2_device.dev,
		.supply = "v-mcde",
	},
#endif
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1
	{
		.dev = &u8500_mcde3_device.dev,
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
/* supply for on-board eMMC */
static struct regulator_consumer_supply ab8500_vaux2_consumers[] = {
	{
		.dev    = &ux500_sdi4_device.dev,
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

static struct regulator_consumer_supply ab8500_vtvout_consumers[] = {
	{
		.supply = "v-tvout",
	},
	{
		.supply = "ab8500-gpadc",
	}
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

static struct regulator_consumer_supply ab8500_vbus_consumers[] = {
	{
		.supply = "v-bus",
	}
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
	.consumer_supplies = ab8500_vbus_consumers,
};

static struct platform_device ab8500_vbus_regulator_dev = {
	.name = "ab8500-regulator",
	.id   = 3,
	.dev  = {
		.platform_data = &ab8500_vbus_init,
	},
};

/* supply for ab8500-vaudio, VAUDIO LDO */
#define AB8500_VAUDIO_REGULATOR_MIN_VOLTAGE        (1925000)
#define AB8500_VAUDIO_REGULATOR_MAX_VOLTAGE        (2075000)

static struct regulator_consumer_supply ab8500_vaudio_consumers[] = {
	{
		.supply = "v-audio",
	}
};

static struct regulator_init_data ab8500_vaudio_init = {
	.supply_regulator_dev = NULL,
	.constraints = {
		.name = "AB8500-VAUDIO",
		.min_uV = AB8500_VAUDIO_REGULATOR_MIN_VOLTAGE,
		.max_uV = AB8500_VAUDIO_REGULATOR_MAX_VOLTAGE,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE|
			REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_NORMAL|REGULATOR_MODE_IDLE,
	},
	.num_consumer_supplies = ARRAY_SIZE(ab8500_vaudio_consumers),
	.consumer_supplies = ab8500_vaudio_consumers,
};

static struct platform_device ab8500_vaudio_regulator_dev = {
	.name = "ab8500-regulator",
	.id   = 4,
	.dev  = {
		.platform_data = &ab8500_vaudio_init,
	},
};

/* supply for v-anamic1 VAMic1-LDO */
#define AB8500_VAMIC1_REGULATOR_MIN_VOLTAGE        (2000000)
#define AB8500_VAMIC1_REGULATOR_MAX_VOLTAGE        (2100000)

static struct regulator_consumer_supply ab8500_vamic1_consumers[] = {
	{
		.supply = "v-amic1",
	}
};

static struct regulator_init_data ab8500_vamic1_init = {
	.supply_regulator_dev = NULL,
	.constraints = {
		.name = "AB8500-VAMIC1",
		.min_uV = AB8500_VAMIC1_REGULATOR_MIN_VOLTAGE,
		.max_uV = AB8500_VAMIC1_REGULATOR_MAX_VOLTAGE,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE|
			REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_NORMAL|REGULATOR_MODE_IDLE,
	},
	.num_consumer_supplies = ARRAY_SIZE(ab8500_vamic1_consumers),
	.consumer_supplies = ab8500_vamic1_consumers,
};

static struct platform_device ab8500_vamic1_regulator_dev = {
	.name = "ab8500-regulator",
	.id   = 5,
	.dev  = {
		.platform_data = &ab8500_vamic1_init,
	},
};

/* supply for v-amic2, VAMIC2 LDO, reuse constants for AMIC1 */
static struct regulator_consumer_supply ab8500_vamic2_consumers[] = {
	{
		.supply = "v-amic2",
	}
};

static struct regulator_init_data ab8500_vamic2_init = {
	.supply_regulator_dev = NULL,
	.constraints = {
		.name = "AB8500-VAMIC2",
		.min_uV = AB8500_VAMIC1_REGULATOR_MIN_VOLTAGE,
		.max_uV = AB8500_VAMIC1_REGULATOR_MAX_VOLTAGE,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE|
			REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_NORMAL|REGULATOR_MODE_IDLE,
	},
	.num_consumer_supplies = ARRAY_SIZE(ab8500_vamic2_consumers),
	.consumer_supplies = ab8500_vamic2_consumers,
};

static struct platform_device ab8500_vamic2_regulator_dev = {
	.name = "ab8500-regulator",
	.id   = 6,
	.dev  = {
		.platform_data = &ab8500_vamic2_init,
	},
};

/* supply for v-dmic, VDMIC LDO */
#define AB8500_VDMIC_REGULATOR_MIN_VOLTAGE	(1700000)
#define AB8500_VDMIC_REGULATOR_MAX_VOLTAGE	(1950000)

static struct regulator_consumer_supply ab8500_vdmic_consumers[] = {
	{
		.supply = "v-dmic",
	}
};

static struct regulator_init_data ab8500_vdmic_init = {
	.supply_regulator_dev = NULL,
	.constraints = {
		.name = "AB8500-VDMIC",
		.min_uV = AB8500_VDMIC_REGULATOR_MIN_VOLTAGE,
		.max_uV = AB8500_VDMIC_REGULATOR_MAX_VOLTAGE,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE|
			REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_NORMAL|REGULATOR_MODE_IDLE,
	},
	.num_consumer_supplies = ARRAY_SIZE(ab8500_vdmic_consumers),
	.consumer_supplies = ab8500_vdmic_consumers,
};

static struct platform_device ab8500_vdmic_regulator_dev = {
	.name = "ab8500-regulator",
	.id   = 7,
	.dev  = {
		.platform_data = &ab8500_vdmic_init,
	},
};

/* supply for v-intcore12, VINTCORE12 LDO */
#define AB8500_VINTCORE_REGULATOR_MIN_VOLTAGE      (1200000)
#define AB8500_VINTCORE_REGULATOR_MAX_VOLTAGE      (1350000)

static struct regulator_consumer_supply ab8500_vintcore_consumers[] = {
	{
		.supply = "v-intcore",
	}
};

static struct regulator_init_data ab8500_vintcore_init = {
	.supply_regulator_dev = NULL,
	.constraints = {
		.name = "AB8500-VINTCORE",
		.min_uV = AB8500_VINTCORE_REGULATOR_MIN_VOLTAGE,
		.max_uV = AB8500_VINTCORE_REGULATOR_MAX_VOLTAGE,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE|
			REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_NORMAL|REGULATOR_MODE_IDLE,
	},
	.num_consumer_supplies = ARRAY_SIZE(ab8500_vintcore_consumers),
	.consumer_supplies = ab8500_vintcore_consumers,
};

static struct platform_device ab8500_vintcore_regulator_dev = {
	.name = "ab8500-regulator",
	.id   = 8,
	.dev  = {
		.platform_data = &ab8500_vintcore_init,
	},
};
static struct platform_device *u8500_regulators[] = {
	&db8500_vape_regulator_dev,
	&db8500_vana_regulator_dev,
	&ab8500_vaux1_regulator_dev,
	&ab8500_vaux2_regulator_dev,
	&ab8500_vtvout_regulator_dev,
	&ab8500_vbus_regulator_dev,
	&ab8500_vaudio_regulator_dev,
	&ab8500_vamic1_regulator_dev,
	&ab8500_vamic2_regulator_dev,
	&ab8500_vdmic_regulator_dev,
	&ab8500_vintcore_regulator_dev,
};

#endif

/* FIXME: move this to the appropriate file */
void __init u8500_init_regulators(void)
{
#ifdef CONFIG_REGULATOR
	/* we want the on-chip regulator before any device registration */
	platform_add_devices(u8500_regulators, ARRAY_SIZE(u8500_regulators));
#endif
}
