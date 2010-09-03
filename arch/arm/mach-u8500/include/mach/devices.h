/*
 * Copyright (C) 2010 ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_DEVICES_H__
#define __ASM_ARCH_DEVICES_H__

struct platform_device;
struct amba_device;

void __init u8500_register_device(struct platform_device *dev, void *data);
void __init u8500_register_amba_device(struct amba_device *dev, void *data);

extern struct amba_device u8500_gpio0_device;
extern struct amba_device u8500_gpio1_device;
extern struct amba_device u8500_gpio2_device;
extern struct amba_device u8500_gpio3_device;
extern struct amba_device u8500_gpio4_device;
extern struct platform_device u8500_msp0_device;
extern struct platform_device u8500_msp1_device;
extern struct platform_device u8500_msp2_device;
extern struct amba_device u8500_msp2_spi_device;
extern struct platform_device u8500_i2c_0_controller;
extern struct platform_device u8500_i2c_1_controller;
extern struct platform_device u8500_i2c_2_controller;
extern struct platform_device u8500_i2c_3_controller;
extern struct platform_device u8500_i2c_4_controller;
extern struct platform_device u8500_mcde2_device;
extern struct platform_device u8500_mcde3_device;
extern struct platform_device u8500_mcde1_device;
extern struct platform_device u8500_mcde0_device;
extern struct platform_device u8500_hsit_device;
extern struct platform_device u8500_hsir_device;
extern struct platform_device u8500_shrm_device;
extern struct platform_device u8500_b2r2_device;
extern struct platform_device u8500_pmem_device;
extern struct platform_device u8500_pmem_mio_device;
extern struct platform_device u8500_pmem_hwb_device;
extern struct amba_device u8500_rtc_device;
extern struct platform_device u8500_dma_device;
extern struct amba_device u8500_ssp0_device;
extern struct amba_device u8500_ssp1_device;
extern struct amba_device u8500_spi0_device;
extern struct amba_device u8500_sdi4_device;
extern struct amba_device u8500_sdi0_device;
extern struct amba_device u8500_sdi1_device;
extern struct amba_device u8500_sdi2_device;
extern struct platform_device u8500_ab8500_device;
extern struct platform_device u8500_musb_device;
extern struct amba_device u8500_uart2_device;
extern struct amba_device u8500_uart1_device;
extern struct amba_device u8500_uart0_device;
extern struct amba_device u8500_uart0_device;
extern struct amba_device u8500_uart1_device;
extern struct amba_device u8500_uart2_device;

/*
 * Do not use inside drivers.  Check it in the board file and alter platform
 * data.
 */
extern int platform_id;
#define MOP500_PLATFORM_ID	0
#define HREF_PLATFORM_ID	1

#endif
