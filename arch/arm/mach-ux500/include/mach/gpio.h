/*----------------------------------------------------------------------------------*/
/*  copyright STMicroelectronics, 2007.                                            */
/*                                                                                  */
/* This program is free software; you can redistribute it and/or modify it under    */
/* the terms of the GNU General Public License as published by the Free	            */
/* Software Foundation; either version 2.1 of the License, or (at your option)      */
/* any later version.                                                               */
/*                                                                                  */
/* This program is distributed in the hope that it will be useful, but WITHOUT      */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS    */
/* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.   */
/*                                                                                  */
/* You should have received a copy of the GNU General Public License                */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.             */
/*----------------------------------------------------------------------------------*/
#ifndef __MACH_GPIO_H
#define __MACH_GPIO_H

#ifndef __LINUX_GPIO_H
#error "Do not include this file directly, include <linux/gpio.h> instead."
#endif

#define ARCH_NR_GPIOS       309 /* 292+17 for STMPE1601*/

#include <mach/hardware.h>
#include <asm-generic/gpio.h>
#include <mach/irqs.h>

/*
 * Macro to decorate plain GPIO numbers
 */
#define GPIO(x)				(x)
#define stm_get_gpio_base(base, offset) base
/*
 * Macros to get IRQ number from GPIO pin and vice-versa
 */
#define GPIO_TO_IRQ(gpio)	(gpio + MAX_CHIP_IRQ)
#define IRQ_TO_GPIO(irq)	(irq - MAX_CHIP_IRQ)

/*
 * Standard GPIOLIB APIs (additional APIs in include/asm-generic/gpio.h)
 */
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

static inline int gpio_get_value(unsigned int gpio)
{
	return __gpio_get_value(gpio);
}

static inline void gpio_set_value(unsigned int gpio, int value)
{
	__gpio_set_value(gpio, value);
}

/*
 * Special values for gpio_set_value() to enable platform-specific
 * GPIO configurations, in addition to named values for 0 and 1
 */
#define GPIO_LOW		0
#define GPIO_HIGH		1
#define GPIO_PULLUP_DIS 0xA
#define GPIO_PULLUP_EN  0xB
#define GPIO_ALTF_A		0xAFA	/* Alternate function A    */
#define GPIO_ALTF_B		0xAFB	/* Alternate function B    */
#define GPIO_ALTF_C		0xAFC	/* Alternate function C    */
#define GPIO_RESET		0xAFD	/* Input with pull-up/down */

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

/* GPIO pin data*/
typedef enum {
	GPIO_DATA_LOW,		/* GPIO pin status is low. */
	GPIO_DATA_HIGH		/* GPIO pin status is high. */
} gpio_data;

struct gpio_altfun_data {
	gpio_alt_function altfun;
	int start;
	int end;
	int cont;
	int type;
	char dev_name[20];
};

struct clk;
struct gpio_platform_data {
	struct gpio_block_data *gpio_data;
	int gpio_block_size;
	struct gpio_altfun_data *altfun_table;
	int altfun_table_size;
	struct clk *clk; /* FIXME put this somewhere more appropriate */
};

struct gpio_block_data {
	u32 block_base;
	u32 block_size;
	u32 base_offset;
	int blocks_per_irq;
	int irq;
};

extern int stm_gpio_set_altfunctable(struct gpio_altfun_data *table, int size);
extern int stm_gpio_altfuncenable(gpio_alt_function alt_func);
extern int stm_gpio_altfuncdisable(gpio_alt_function alt_func);

#endif				/* __INC_GPIO_H */
