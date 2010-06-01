/*
 * Copyright (C) 2010 ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/gpio.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <mach/i2c.h>
#include <mach/devices.h>
#include <mach/setup.h>

#ifdef CONFIG_FB_MCDE
/*
 * This is only for MCDE to build.  Remove this once MCDE is fixed to not
 * depend on this variable.
 */
int platform_id;
#endif

/*
 * GPIO
 */

static struct gpio_altfun_data gpio_altfun_table[] = {
	__GPIO_ALT(GPIO_ALT_I2C_4, 4, 5, 0, NMK_GPIO_ALT_B, "i2c4"),
	__GPIO_ALT(GPIO_ALT_I2C_1, 16, 17, 0, NMK_GPIO_ALT_B, "i2c1"),
	__GPIO_ALT(GPIO_ALT_I2C_2, 8, 9, 0, NMK_GPIO_ALT_B, "i2c2"),
	__GPIO_ALT(GPIO_ALT_I2C_0, 147, 148, 0, NMK_GPIO_ALT_A, "i2c0"),
	__GPIO_ALT(GPIO_ALT_I2C_3, 229, 230, 0, NMK_GPIO_ALT_C, "i2c3"),
	__GPIO_ALT(GPIO_ALT_UART_2, 177, 180, 0, NMK_GPIO_ALT_A, "uart2"),
	__GPIO_ALT(GPIO_ALT_SSP_0, 143, 146, 0, NMK_GPIO_ALT_A, "ssp0"),
	__GPIO_ALT(GPIO_ALT_SSP_1, 139, 142, 0, NMK_GPIO_ALT_A, "ssp1"),
	__GPIO_ALT(GPIO_ALT_USB_OTG, 256, 267, 0, NMK_GPIO_ALT_A, "usb"),
	__GPIO_ALT(GPIO_ALT_UART_1, 200, 203, 0, NMK_GPIO_ALT_A, "uart1"),
	__GPIO_ALT(GPIO_ALT_UART_0_NO_MODEM, 28, 29, 0, NMK_GPIO_ALT_A, "uart0"),
	__GPIO_ALT(GPIO_ALT_MSP_0, 12, 15, 0, NMK_GPIO_ALT_A, "msp0"),
	__GPIO_ALT(GPIO_ALT_MSP_1, 33, 36, 0, NMK_GPIO_ALT_A, "msp1"),
	__GPIO_ALT(GPIO_ALT_MSP_2, 192, 196, 0, NMK_GPIO_ALT_A, "msp2"),
	__GPIO_ALT(GPIO_ALT_HSIR, 219, 221, 0, NMK_GPIO_ALT_A, "hsir"),
	__GPIO_ALT(GPIO_ALT_HSIT, 222, 224, 0, NMK_GPIO_ALT_A, "hsit"),
	__GPIO_ALT(GPIO_ALT_EMMC, 197, 207, 0, NMK_GPIO_ALT_A, "emmc"),
	__GPIO_ALT(GPIO_ALT_SDMMC, 18, 28, 0, NMK_GPIO_ALT_A, "sdmmc"),
	__GPIO_ALT(GPIO_ALT_SDIO, 208, 214, 0, NMK_GPIO_ALT_A, "sdio"),
	__GPIO_ALT(GPIO_ALT_TRACE, 70, 74, 0, NMK_GPIO_ALT_C, "stm"),
	__GPIO_ALT(GPIO_ALT_SDMMC2, 128, 138, 0, NMK_GPIO_ALT_A, "mmc2"),
#ifndef CONFIG_FB_NOMADIK_MCDE_CHANNELB_DISPLAY_VUIB_WVGA
	__GPIO_ALT(GPIO_ALT_LCD_PANELB_ED, 78, 85, 1, NMK_GPIO_ALT_A, "mcde tvout"),
	__GPIO_ALT(GPIO_ALT_LCD_PANELB_ED, 150, 150, 0, NMK_GPIO_ALT_B, "mcde tvout"),
	__GPIO_ALT(GPIO_ALT_LCD_PANELB, 78, 81, 1, NMK_GPIO_ALT_A, "mcde tvout"),
	__GPIO_ALT(GPIO_ALT_LCD_PANELB, 150, 150, 0, NMK_GPIO_ALT_B, "mcde tvout"),
#else
	__GPIO_ALT(GPIO_ALT_LCD_PANELB, 153, 171, 1, NMK_GPIO_ALT_B, "mcde tvout"),
	__GPIO_ALT(GPIO_ALT_LCD_PANELB, 64, 77, 0, NMK_GPIO_ALT_A, "mcde tvout"),
#endif
	__GPIO_ALT(GPIO_ALT_LCD_PANELA, 68, 68, 0, NMK_GPIO_ALT_A, "mcde tvout"),
	__GPIO_ALT(GPIO_ALT_MMIO_INIT_BOARD, 141, 142, 0, NMK_GPIO_ALT_B, "mmio"),
	__GPIO_ALT(GPIO_ALT_MMIO_CAM_SET_I2C, 8, 9, 0, NMK_GPIO_ALT_A, "mmio"),
	__GPIO_ALT(GPIO_ALT_MMIO_CAM_SET_EXT_CLK, 227, 228, 0, NMK_GPIO_ALT_A, "mmio"),
#ifdef CONFIG_TOUCHP_EXT_CLK
	__GPIO_ALT(GPIO_ALT_TP_SET_EXT_CLK, 228, 228, 0, NMK_GPIO_ALT_A, "u8500_tp1"),
#endif
};

/*
 * I2C
 */

#define U8500_I2C_CONTROLLER(id, _slsu, _tft, _rft, clk, _sm) \
static struct nmk_i2c_controller u8500_i2c_##id = { \
	/*				\
	 * slave data setup time, which is	\
	 * 250 ns,100ns,10ns which is 14,6,2	\
	 * respectively for a 48 Mhz	\
	 * i2c clock			\
	 */				\
	.slsu		= _slsu,	\
	/* Tx FIFO threshold */		\
	.tft		= _tft,		\
	/* Rx FIFO threshold */		\
	.rft		= _rft,		\
	/* std. mode operation */	\
	.clk_freq	= clk,		\
	.sm		= _sm,		\
}

/*
 * The board uses 3 i2c controllers, initialize all of
 * them with slave data setup time of 250 ns,
 * Tx & Rx FIFO threshold values as 1 and standard
 * mode of operation
 */

U8500_I2C_CONTROLLER(1, 0xe, 1, 1, 400000, I2C_FREQ_MODE_FAST);
U8500_I2C_CONTROLLER(2,	0xe, 1, 1, 400000, I2C_FREQ_MODE_FAST);
U8500_I2C_CONTROLLER(3,	0xe, 1, 1, 400000, I2C_FREQ_MODE_FAST);

static struct amba_device *amba_board_devs[] __initdata = {
	&ux500_uart0_device,
	&ux500_uart1_device,
	&ux500_uart2_device,
};

static struct platform_device *u5500_platform_devices[] __initdata = {
#ifdef CONFIG_U5500_MLOADER_HELPER
	&mloader_helper_device,
#endif
};

static void __init u5500_init_machine(void)
{
	stm_gpio_set_altfunctable(gpio_altfun_table,
				  ARRAY_SIZE(gpio_altfun_table));

	u5500_init_devices();

	amba_add_devices(amba_board_devs, ARRAY_SIZE(amba_board_devs));

	u8500_register_device(&ux500_i2c_controller1, &u8500_i2c_1);
	u8500_register_device(&ux500_i2c_controller2, &u8500_i2c_2);
	u8500_register_device(&ux500_i2c_controller3, &u8500_i2c_3);

	platform_add_devices(u5500_platform_devices,
			     ARRAY_SIZE(u5500_platform_devices));
}

MACHINE_START(NOMADIK, "ST-Ericsson U5500 Platform")
	.phys_io	= UX500_UART0_BASE,
	.io_pg_offst	= (IO_ADDRESS(UX500_UART0_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.map_io		= u5500_map_io,
	.init_irq	= ux500_init_irq,
	.timer		= &u8500_timer,
	.init_machine	= u5500_init_machine,
MACHINE_END
