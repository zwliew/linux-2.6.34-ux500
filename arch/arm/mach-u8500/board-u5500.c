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

#include <mach/i2c-stm.h>
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
 * I2C
 */

static struct i2c_platform_data u8500_i2c1_data = {
	.gpio_alt_func		= GPIO_ALT_I2C_1,
	.name			= "i2c1",
	.own_addr		= I2C1_LP_OWNADDR,
	.mode			= I2C_FREQ_MODE_STANDARD,
	.clk_freq		= 100000,
	.slave_addressing_mode	= I2C_7_BIT_ADDRESS,
	.digital_filter_control	= I2C_DIGITAL_FILTERS_OFF,
	.dma_sync_logic_control	= I2C_DISABLED,
	.start_byte_procedure	= I2C_DISABLED,
	.slave_data_setup_time	= 0xE,
	.bus_control_mode	= I2C_BUS_MASTER_MODE,
	.i2c_loopback_mode	= I2C_DISABLED,
	.xfer_mode		= I2C_TRANSFER_MODE_INTERRUPT,
	.high_speed_master_code	= 0,
	.i2c_tx_int_threshold	= 1,
	.i2c_rx_int_threshold	= 1
};

static struct i2c_platform_data u8500_i2c2_data = {
	.gpio_alt_func		= GPIO_ALT_I2C_2,
	.name			= "i2c2",
	.own_addr		= I2C2_LP_OWNADDR,
	.mode			= I2C_FREQ_MODE_STANDARD,
	.clk_freq		= 100000,
	.slave_addressing_mode	= I2C_7_BIT_ADDRESS,
	.digital_filter_control	= I2C_DIGITAL_FILTERS_OFF,
	.dma_sync_logic_control	= I2C_DISABLED,
	.start_byte_procedure	= I2C_DISABLED,
	.slave_data_setup_time	= 0xE,
	.bus_control_mode	= I2C_BUS_MASTER_MODE,
	.i2c_loopback_mode	= I2C_DISABLED,
	.xfer_mode		= I2C_TRANSFER_MODE_INTERRUPT,
	.high_speed_master_code	= 0,
	.i2c_tx_int_threshold	= 1,
	.i2c_rx_int_threshold	= 1
};

static struct i2c_platform_data u8500_i2c3_data = {
	.gpio_alt_func		= GPIO_ALT_I2C_3,
	.name			= "i2c3",
	.own_addr		= I2C3_LP_OWNADDR,
	.mode			= I2C_FREQ_MODE_STANDARD,
	.clk_freq		= 100000,
	.slave_addressing_mode	= I2C_7_BIT_ADDRESS,
	.digital_filter_control	= I2C_DIGITAL_FILTERS_OFF,
	.dma_sync_logic_control	= I2C_DISABLED,
	.start_byte_procedure	= I2C_DISABLED,
	.slave_data_setup_time	= 0xE,
	.bus_control_mode	= I2C_BUS_MASTER_MODE,
	.i2c_loopback_mode	= I2C_DISABLED,
	.xfer_mode		= I2C_TRANSFER_MODE_INTERRUPT,
	.high_speed_master_code	= 0,
	.i2c_tx_int_threshold	= 1,
	.i2c_rx_int_threshold	= 1
};

static struct amba_device *amba_board_devs[] __initdata = {
	&ux500_uart0_device,
	&ux500_uart1_device,
	&ux500_uart2_device,
};

static void __init u5500_init_machine(void)
{
	u5500_init_devices();

	amba_add_devices(amba_board_devs, ARRAY_SIZE(amba_board_devs));

	u8500_register_device(&ux500_i2c1_device, &u8500_i2c1_data);
	u8500_register_device(&ux500_i2c2_device, &u8500_i2c2_data);
	u8500_register_device(&ux500_i2c3_device, &u8500_i2c3_data);
}

MACHINE_START(NOMADIK, "ST-Ericsson U5500 Platform")
	.phys_io	= UX500_UART0_BASE,
	.io_pg_offst	= (IO_ADDRESS(UX500_UART0_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.map_io		= u5500_map_io,
	.init_irq	= u8500_init_irq,
	.timer		= &u8500_timer,
	.init_machine	= u5500_init_machine,
MACHINE_END
