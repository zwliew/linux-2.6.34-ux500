/*
 *  Copyright (C) 2007 ST Microelectronics
 *  Copyright (C) 2010 ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __MACH_GPIO_H
#define __MACH_GPIO_H

/*
 * 288 (#267 is the highest one actually hooked up) onchip GPIOs, plus enough
 * room for a couple of GPIO expanders.
 */
#define ARCH_NR_GPIOS       350

#include <plat/gpio.h>

/* Used by test applications */
#define GPIO_TOTAL_PINS		267

#include <mach/hardware.h>
#include <mach/irqs.h>

static inline int gpio_to_irq(unsigned int gpio)
{
	if (gpio_is_valid(gpio))
		return GPIO_TO_IRQ(gpio);
	else
		return -EINVAL;
}

static inline int irq_to_gpio(unsigned int irq)
{
	if (irq < NR_IRQS)
		return IRQ_TO_GPIO(irq);
	else
		return -EINVAL;
}

#define GPIO_ALTF_A	NMK_GPIO_ALT_A
#define GPIO_ALTF_B	NMK_GPIO_ALT_B
#define GPIO_ALTF_C	NMK_GPIO_ALT_C

/*
 * Alternate Function:
 *  refered in altfun_table to pointout particular altfun to be enabled
 *  when using GPIO_ALT_FUNCTION A/B/C enable/disable operation
 */
typedef enum {
	GPIO_ALT_UART_0_MODEM,
	GPIO_ALT_UART_0_NO_MODEM,
	GPIO_ALT_UART_1,
	GPIO_ALT_UART_2,
	GPIO_ALT_I2C_0,
	GPIO_ALT_I2C_1,
	GPIO_ALT_I2C_2,
	GPIO_ALT_I2C_3,
	GPIO_ALT_I2C_4,
	GPIO_ALT_MSP_0,
	GPIO_ALT_MSP_1,
	GPIO_ALT_MSP_2,
	GPIO_ALT_MSP_3,
	GPIO_ALT_SSP_0,
	GPIO_ALT_SSP_1,
	GPIO_ALT_MM_CARD,
	GPIO_ALT_SD_CARD,
	GPIO_ALT_DMA_0,
	GPIO_ALT_DMA_1,
	GPIO_ALT_HSIR,
	GPIO_ALT_CCIR656_INPUT,
	GPIO_ALT_CCIR656_OUTPUT,
	GPIO_ALT_LCD_PANELA,
	GPIO_ALT_LCD_PANELB_ED,
	GPIO_ALT_LCD_PANELB,
	GPIO_ALT_MDIF,
	GPIO_ALT_SDRAM,
	GPIO_ALT_HAMAC_AUDIO_DBG,
	GPIO_ALT_HAMAC_VIDEO_DBG,
	GPIO_ALT_CLOCK_RESET,
	GPIO_ALT_TSP,
	GPIO_ALT_IRDA,
	GPIO_ALT_USB_MINIMUM,
	GPIO_ALT_USB_I2C,
	GPIO_ALT_OWM,
	GPIO_ALT_PWL,
	GPIO_ALT_FSMC,
	GPIO_ALT_COMP_FLASH,
	GPIO_ALT_SRAM_NOR_FLASH,
	GPIO_ALT_FSMC_ADDLINE_0_TO_15,
	GPIO_ALT_SCROLL_KEY,
	GPIO_ALT_MSHC,
	GPIO_ALT_HPI,
	GPIO_ALT_USB_OTG,
	GPIO_ALT_SDIO,
	GPIO_ALT_HSMMC,
	GPIO_ALT_FSMC_ADD_DATA_0_TO_25,
	GPIO_ALT_HSIT,
	GPIO_ALT_NOR,
	GPIO_ALT_NAND,
	GPIO_ALT_KEYPAD,
	GPIO_ALT_VPIP,
	GPIO_ALT_CAM,
	GPIO_ALT_CCP1,
	GPIO_ALT_EMMC,
	GPIO_ALT_SDMMC,
	GPIO_ALT_TRACE,
	GPIO_ALT_MMIO_INIT_BOARD,
	GPIO_ALT_MMIO_CAM_SET_I2C,
	GPIO_ALT_MMIO_CAM_SET_EXT_CLK,
	GPIO_ALT_SDMMC2,
	GPIO_ALT_TP_SET_EXT_CLK,
	GPIO_ALT_FUNMAX		/* Add new alt func before this */
} gpio_alt_function;

struct gpio_altfun_data {
	gpio_alt_function altfun;
	int start;
	int end;
	int cont;
	int type;
	char dev_name[20];
};

extern int stm_gpio_set_altfunctable(struct gpio_altfun_data *table, int size);
extern int stm_gpio_altfuncenable(gpio_alt_function alt_func);
extern int stm_gpio_altfuncdisable(gpio_alt_function alt_func);

#define __GPIO_ALT(_fun, _start, _end, _cont, _type, _name) {	\
	.altfun = _fun,		\
	.start = _start,	\
	.end = _end,		\
	.cont = _cont,		\
	.type = _type,		\
	.dev_name = _name }

#define __GPIO_RESOURCE(soc, block)					\
	{								\
		.start	= soc##_GPIOBANK##block##_BASE,			\
		.end	= soc##_GPIOBANK##block##_BASE + 127,		\
		.flags	= IORESOURCE_MEM,				\
	},								\
	{								\
		.start	= IRQ_GPIO##block,				\
		.end	= IRQ_GPIO##block,				\
		.flags	= IORESOURCE_IRQ,				\
	}

#define __GPIO_DEVICE(soc, block)					\
	{								\
		.name 		= "gpio",				\
		.id		= block,				\
		.num_resources 	= 2,					\
		.resource	= &soc##_gpio_resources[block * 2],	\
		.dev = {						\
			.platform_data = &soc##_gpio_data[block],	\
		},							\
	}

#define GPIO_DATA(_name, first, num)					\
	{								\
		.name 		= _name,				\
		.first_gpio 	= first,				\
		.first_irq 	= NOMADIK_GPIO_TO_IRQ(first),		\
		.num_gpio	= num,					\
	}

#ifdef CONFIG_UX500_SOC_DB8500
#define GPIO_RESOURCE(block)	__GPIO_RESOURCE(U8500, block)
#define GPIO_DEVICE(block)	__GPIO_DEVICE(u8500, block)
#elif defined(CONFIG_UX500_SOC_DB5500)
#define GPIO_RESOURCE(block)	__GPIO_RESOURCE(U5500, block)
#define GPIO_DEVICE(block)	__GPIO_DEVICE(u5500, block)
#endif

#endif	/* __MACH_GPIO_H */
