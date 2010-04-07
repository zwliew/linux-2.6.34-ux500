/*
 * Copyright (C) 2009 ST-Ericsson SA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/i2c.h>
#include <linux/hsi.h>
#include <linux/i2s/i2s.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/power_supply.h>

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>

#include <mach/devices.h>
#include <mach/kpd.h>
#include <mach/stmpe2401.h>
#include <mach/stmpe1601.h>
#include <mach/tc35892.h>
#include <mach/av8100_p.h>
#include <mach/ab8500.h>
#include <mach/ab8500_bm.h>
#include <mach/mmc.h>
#include <mach/setup.h>
#include <mach/i2c.h>

#include <mach/u8500_tsc.h>

#define IRQ_KP 1 /*To DO*/

int href_v1_board;
extern int platform_id;

#define MOP500_PLATFORM_ID	0
#define HREF_PLATFORM_ID	1

/*
 * This is only non-static because MCDE accesses it directly.  Make this static
 * once MCDE is fixed to not depend on these variables.
 */
int platform_id = MOP500_PLATFORM_ID;

/* we have equally similar boards with very minimal
 * changes, so we detect the platform during boot
 */
static int __init board_id_setup(char *str)
{
	if (!str)
		return 1;

	switch (*str) {
	case '0':
		printk(KERN_INFO "MOP500 platform\n");
		platform_id = MOP500_PLATFORM_ID;
		break;
	case '1':
		printk(KERN_INFO "HREF platform\n");
		platform_id = HREF_PLATFORM_ID;
		break;
	default:
		printk(KERN_INFO "Unknown board_id=%c\n", *str);
		break;
	};

	return 1;
}
__setup("board_id=", board_id_setup);

static struct gpio_altfun_data gpio_altfun_table[] = {
	__GPIO_ALT(GPIO_ALT_I2C_4, 4, 5, 0, NMK_GPIO_ALT_B, "i2c4"),
	__GPIO_ALT(GPIO_ALT_I2C_1, 16, 17, 0, NMK_GPIO_ALT_B, "i2c1"),
	__GPIO_ALT(GPIO_ALT_I2C_2, 10, 11, 0, NMK_GPIO_ALT_B, "i2c2"),
	__GPIO_ALT(GPIO_ALT_I2C_0, 147, 148, 0, NMK_GPIO_ALT_A, "i2c0"),
	__GPIO_ALT(GPIO_ALT_I2C_3, 229, 230, 0, NMK_GPIO_ALT_C, "i2c3"),
	__GPIO_ALT(GPIO_ALT_UART_2, 29, 32, 0, NMK_GPIO_ALT_C, "uart2"),
	__GPIO_ALT(GPIO_ALT_SSP_0, 143, 146, 0, NMK_GPIO_ALT_A, "ssp0"),
	__GPIO_ALT(GPIO_ALT_SSP_1, 139, 142, 0, NMK_GPIO_ALT_A, "ssp1"),
	__GPIO_ALT(GPIO_ALT_USB_OTG, 256, 267, 0, NMK_GPIO_ALT_A, "usb"),
	__GPIO_ALT(GPIO_ALT_UART_1, 4, 7, 0, NMK_GPIO_ALT_A, "uart1"),
	__GPIO_ALT(GPIO_ALT_UART_0_NO_MODEM, 0, 3, 0, NMK_GPIO_ALT_A, "uart0"),
	__GPIO_ALT(GPIO_ALT_UART_0_MODEM, 0, 3, 1, NMK_GPIO_ALT_A, "uart0"),
	__GPIO_ALT(GPIO_ALT_UART_0_MODEM, 33, 36, 0, NMK_GPIO_ALT_C, "uart0"),
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
#ifndef CONFIG_U8500_TSC_EXT_CLK_SHARE
	__GPIO_ALT(GPIO_ALT_TP_SET_EXT_CLK, 228, 228, 0, NMK_GPIO_ALT_A, "u8500_tp"),
#endif
};

static struct stmpe2401_platform_data stmpe_data = {
	.gpio_base = U8500_NR_GPIO,
	.irq    = GPIO_TO_IRQ(217),
};

static struct av8100_platform_data av8100_plat_data = {
	.irq    = GPIO_TO_IRQ(192),
};

static struct stmpe1601_platform_data stmpe1601_data = {
	.gpio_base = (U8500_NR_GPIO + 24), /* U8500_NR_GPIO GPIOs + 24 extended
					      GPIOs of STMPE2401*/
	.irq    = GPIO_TO_IRQ(218),
};

static struct tc35892_platform_data tc35892_data = {
	.gpio_base = U8500_NR_GPIO,
	.irq    = GPIO_TO_IRQ(217),
};

static struct i2c_board_info __initdata nmdk_i2c0_egpio_devices[] = {
	{
		/* stmpe2401 @ 0x84(w),0x85(r) */
		I2C_BOARD_INFO("stmpe2401", 0x42),
		.platform_data = &stmpe_data,
	}
};

static struct i2c_board_info __initdata nmdk_i2c0_egpio1_devices[] = {
	{

		I2C_BOARD_INFO("tc35892", 0x42),
		.platform_data = &tc35892_data,
	}
};

/**
 * Touch panel related platform specific initialization
 */
#if defined(CONFIG_U8500_TSC)

/**
 * tp_gpio_board_init : configures the touch panel.
 * This function can be used to configures
 * the voltage and reset the touch panel controller.
 */
int tp_gpio_board_init(void)
{
#ifndef CONFIG_REGULATOR
	int val;
#endif
	int retval = 0;
#if (defined CONFIG_U8500_TSC_EXT_CLK_9_6 && !defined CONFIG_U8500_TSC_EXT_CLK_SHARE)
	void __iomem *clk_base;
	unsigned int clk_value;
#endif
#ifndef CONFIG_REGULATOR
	/** Set the voltage for Bu21013 controller */
	val = ab8500_read(AB8500_REGU_CTRL2, AB8500_REGU_VAUX12_REGU_REG);

	retval = ab8500_write(AB8500_REGU_CTRL2, AB8500_REGU_VAUX12_REGU_REG,
				(val | 0x1));
	if (retval < 0)
		return -1;
	val = ab8500_read(AB8500_REGU_CTRL2, AB8500_REGU_VAUX1_SEL_REG);

	retval = ab8500_write(AB8500_REGU_CTRL2, AB8500_REGU_VAUX1_SEL_REG,
				0x0C);
	if (retval < 0)
		return -1;
#endif
#ifdef CONFIG_U8500_TSC_EXT_CLK_9_6
#ifndef CONFIG_U8500_TSC_EXT_CLK_SHARE
	retval = stm_gpio_altfuncenable(GPIO_ALT_TP_SET_EXT_CLK);
	if (retval < 0) {
		printk(KERN_ERR " ext clock stm_gpio_altfuncenable failed  \n");
		return -1;
	}
	clk_base = (void __iomem *)IO_ADDRESS(U8500_PRCMU_BASE + 0x1CC);
	clk_value = readl(clk_base);
	writel(0x840000, clk_base);
#endif
#endif
	if (platform_id == MOP500_PLATFORM_ID) {
		/** why set directtion is not working ~ FIXME */
		/* gpio_direction_output(270,1); */
		gpio_set_value(TOUCHP_CS0, 1);
	} else if (platform_id == HREF_PLATFORM_ID) {
		retval = gpio_direction_output(EGPIO_PIN_13, 1);
		if (retval < 0) {
			printk(KERN_ERR " reset gpio_direction_output init failed  \n");
			return -1;
		}
		gpio_set_value(EGPIO_PIN_13, 1);
	}
	return retval;
}

/**
 * tp_gpio_board_exit : deconfigures the touch panel controller
 * This function can be used to deconfigures the chip selection
 * for touch panel controller.
 */
int tp_gpio_board_exit(void)
{
	int retval = 0;
	if (platform_id == MOP500_PLATFORM_ID) {
		/** why set directtion is not working ~ FIXME */
		/* gpio_direction_output(270,1); */
		gpio_set_value(TOUCHP_CS0, 0);
	} else if (platform_id == HREF_PLATFORM_ID) {
		retval = gpio_direction_output(EGPIO_PIN_13, 0);
		if (retval < 0) {
			printk(KERN_ERR " reset gpio_direction_output exit failed  \n");
			return -1;
		}
		gpio_set_value(EGPIO_PIN_13, 0);
	}
	return retval;
}

/**
 * tp_init_irq : sets the callback for touch panel controller
 * @parameter: function pointer for touch screen callback handler.
 * @data: data to be passed for touch panel callback handler.
 * This function can be used to set the callback handler for
 * touch panel controller.
 */
int tp_init_irq(void (*callback)(void *parameter), void *data)
{
	int retval = 0;

	if (platform_id == MOP500_PLATFORM_ID) {
		retval = stmpe2401_set_callback(TOUCHP_IRQ, callback,
				(void *)data);
		if (retval < 0)
			printk(KERN_ERR " stmpe2401_set_callback failed \n");
	} else if (platform_id == HREF_PLATFORM_ID) {
		if (href_v1_board == 0) {
			retval = tc35892_set_callback(EGPIO_PIN_12, callback,
					(void *)data);
			if (retval < 0)
				printk(KERN_ERR " tc35892_set_callback failed \n");
		}
	}
	return retval;
}

/**
 * tp_exit_irq : removes the callback for touch panel controller
 * This function can be used to removes the callback handler for
 * touch panel controller.
 */
int tp_exit_irq(void)
{
	int retval = 0;
	if (platform_id == MOP500_PLATFORM_ID) {
		retval = stmpe2401_remove_callback(TOUCHP_IRQ);
		if (retval < 0)
			printk(KERN_ERR " stmpe2401_remove_callback failed \n");
	} else if (platform_id == HREF_PLATFORM_ID) {
		if (href_v1_board == 0) {
			retval = tc35892_remove_callback(EGPIO_PIN_12);
			if (retval < 0)
				printk(KERN_ERR " tc35892_remove_callback failed \n");
		}
	}
	return retval;
}

/**
 * tp_pen_down_irq_enable : enable the interrupt for touch panel controller
 * This function can be used to enable the interrupt for touch panel controller.
 */
int tp_pen_down_irq_enable(void)
{
	int retval = 0;
	if (platform_id == MOP500_PLATFORM_ID) {
		/* do nothing */
	} else if (platform_id == HREF_PLATFORM_ID) {
		if (href_v1_board == 0) {
			retval = tc35892_set_gpio_intr_conf(EGPIO_PIN_12,
				EDGE_SENSITIVE,	TC35892_FALLING_EDGE_OR_LOWLEVEL);
			if (retval < 0)
				printk(KERN_ERR " tc35892_set_gpio_intr_conf failed\n");
			retval = tc35892_set_intr_enable(EGPIO_PIN_12,
				ENABLE_INTERRUPT);
			if (retval < 0)
				printk(KERN_ERR " tc35892_set_intr_enable failed \n");
		}
	}
	return retval;
}

/**
 * tp_pen_down_irq_disable : disable the interrupt
 * This function can be used to disable the interrupt for
 * touch panel controller.
 */
int tp_pen_down_irq_disable(void)
{
	int retval = 0;
	if (platform_id == MOP500_PLATFORM_ID) {
		/* do nothing */
	} else if (platform_id == HREF_PLATFORM_ID) {
		if (href_v1_board == 0) {
			retval = tc35892_set_intr_enable(EGPIO_PIN_12,
					DISABLE_INTERRUPT);
			if (retval < 0)
				printk(KERN_ERR " tc35892_set_intr_enable failed \n");
		}
	}
	return retval;
}

/**
 * tp_read_pin_val : get the interrupt pin value
 * This function can be used to get the interrupt pin value for touch panel
 * controller.
 */
int tp_read_pin_val(void)
{
	int data = 0;
	unsigned int touch_gpio_pin = 84;

	if (platform_id == MOP500_PLATFORM_ID)
		data = gpio_get_value(TOUCHP_IRQ);
	else if (platform_id == HREF_PLATFORM_ID) {
		if (href_v1_board == 0)
			data = gpio_get_value(EGPIO_PIN_12);
		else
			data = gpio_get_value(touch_gpio_pin);
	}
	return data;
}
/**
 * tp_board_href_v1 : update the href v1 flag
 * This function can be used to update the board.
 */
int tp_board_href_v1(void)
{
	unsigned int touch_gpio_pin = 84;
	if (platform_id == HREF_PLATFORM_ID) {
		if (u8500_is_earlydrop())
			href_v1_board = 0;
		else
			href_v1_board = gpio_get_value(touch_gpio_pin);
	} else
		href_v1_board = 0;
	return href_v1_board;
}

static struct tp_device tsc_plat_device = {
	.cs_en = tp_gpio_board_init,
	.cs_dis = tp_gpio_board_exit,
	.irq_init = tp_init_irq,
	.irq_exit = tp_exit_irq,
	.pirq_en  = tp_pen_down_irq_enable,
	.pirq_dis = tp_pen_down_irq_disable,
	.pirq_read_val = tp_read_pin_val,
	.board_href_v1 = tp_board_href_v1,
	.irq = GPIO_TO_IRQ(84),
};
#endif

static struct i2c_board_info __initdata u8500_i2c0_devices[] = {
	{
		I2C_BOARD_INFO("stmpe1601", 0x40),
		.platform_data = &stmpe1601_data,
	},
	{
		I2C_BOARD_INFO("av8100", 0x70),
		.platform_data = &av8100_plat_data,
	},
	{
	 /* FIXME - address TBD, TODO */
	 I2C_BOARD_INFO("uib-kpd", 0x45),
	},
	{
	 /* Audio step-up */
	 I2C_BOARD_INFO("tps61052", 0x33),
	}
};
static struct i2c_board_info __initdata u8500_i2c1_devices[] = {
	{
	 /* GPS - Address TBD, FIXME */
	 I2C_BOARD_INFO("gps", 0x68),
	},
	{
		/* AB3550 */
		I2C_BOARD_INFO("ab3550", 0x4A),
	}
};

static struct i2c_board_info __initdata u8500_i2c2_devices[] = {
	{
	 /* ST 3D accelerometer @ 0x3A(w),0x3B(r) */
	 I2C_BOARD_INFO("lis302dl", 0x1D),
	},
	{
	 /* ASAHI KASEI AK8974 magnetometer, addr TBD FIXME */
	 I2C_BOARD_INFO("ak8974", 0x1),
	},
	{
	 /* Rohm BH1780GLI light sensor addr TBD, FIXME */
	 I2C_BOARD_INFO("bh1780gli", 0x45),
	},
	{
	 /* RGB LED driver, there are 1st and 2nd, TODO */
	 I2C_BOARD_INFO("lp5521tmx", 0x33),
	}
};

static struct i2c_board_info __initdata u8500_i2c3_devices[] = {
	{
	 /* NFC - Address TBD, FIXME */
	 I2C_BOARD_INFO("nfc", 0x68),
	},
#if defined(CONFIG_U8500_TSC)
	{
		/* Touschscreen */
		I2C_BOARD_INFO(DRIVER_TP, I2C3_TOUCHP_ADDRESS),
		.platform_data = &tsc_plat_device,
	},
#endif

};

#ifdef CONFIG_KEYPAD_U8500

#define DEBUG_KP(x)
/* #define DEBUG_KP(x) printk x */
/* Keypad platform specific code */
#ifdef CONFIG_U8500_KEYPAD_DEBUG

#define KEYPAD_NAME             "KEYPAD"

#undef NMDK_DEBUG
#undef NMDK_DEBUG_PFX
#undef NMDK_DBG

#define NMDK_DEBUG      CONFIG_U8500_KEYPAD_DEBUG
#define NMDK_DEBUG_PFX  KEYPAD_NAME
#define NMDK_DBG        KERN_ERR

#endif

/**
 * Keypad related platform specific constants
 * These values may be modified for fine tuning
 */
#define KPD_NB_ROWS 0xFF
#define KPD_NB_COLUMNS 0xFF
#define KEYPAD_DEBOUNCE_PERIOD_STUIB 64 /* Debounce time 64ms */
#define NB_SCAN_CYCLES 8

/* spinlock_t cr_lock = SPIN_LOCK_UNLOCKED; */
static DEFINE_SPINLOCK(cr_lock);

extern irqreturn_t u8500_kp_intrhandler(/*int irq,*/ void *dev_id);

extern struct mutex u8500_keymap_mutex;
extern u8 u8500_keymap; /* Default keymap is keymap 0 */

/*
 * Initializes the key scan table (lookup table) as per pre-defined the scan
 * codes to be passed to upper layer with respective key codes
 */
u8 const kpd_lookup_tbl1[MAX_KPROW][MAX_KPROW] = {
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_9, KEY_RESERVED, KEY_RESERVED},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_POWER, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_7,
	KEY_VOLUMEUP, KEY_RIGHT, KEY_BACK, KEY_RESERVED},
	{KEY_RESERVED, KEY_3, KEY_RESERVED, KEY_DOWN,
	KEY_LEFT, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_UP,
	KEY_RESERVED, KEY_RESERVED, KEY_SEND, KEY_RESERVED},
	{KEY_MENU, KEY_RESERVED, KEY_END, KEY_VOLUMEDOWN,
	KEY_0, KEY_1, KEY_RESERVED, KEY_RESERVED},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_ENTER},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_2, KEY_RESERVED}
};


/*
 * Alternative keymap for seperate applications, can be enabled from sysfs
 */
u8 const kpd_lookup_tbl2[MAX_KPROW][MAX_KPROW] = {
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_9, KEY_RESERVED, KEY_RESERVED},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_ENTER, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_7,
	KEY_KPDOT, KEY_6, KEY_BACK, KEY_RESERVED},
	{KEY_RESERVED, KEY_3, KEY_RESERVED, KEY_8,
	KEY_4, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_5,
	KEY_RESERVED, KEY_RESERVED, KEY_LEFT, KEY_RESERVED},
	{KEY_UP, KEY_RESERVED, KEY_RIGHT, KEY_MENU,
	KEY_0, KEY_1, KEY_RESERVED, KEY_RESERVED},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_DOWN},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_2, KEY_RESERVED}
};


/*
 * Key table for a complete 8X8 keyboard
{KEY_ESC, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_SPACE},
{KEY_GRAVE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7},
{KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_INSERT,
KEY_HOME},
{KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U},
{KEY_I, KEY_O, KEY_P, KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_BACKSLASH,
KEY_DELETE, KEY_END},
{KEY_CAPSLOCK, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J},
{KEY_K, KEY_L, KEY_SEMICOLON, KEY_APOSTROPHE, KEY_ENTER, KEY_DOT,
KEY_COMMA, KEY_SLASH},
{KEY_LEFTSHIFT, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M}
};
*/



/**
 * u8500_kp_results_autoscan :  This function gets scanned key data from stmpe
 * @keys : o/p parameter, returns keys pressed.
 * This function can be used in both polling or interrupt usage.
 */
int u8500_kp_results_autoscan(struct keypad_t *kp)
{
	int err;
	t_stmpe1601_key_status keys;
	u8 kcode, i;
	static int keyp_cnt;
	mutex_lock(&u8500_keymap_mutex);
	if (u8500_keymap == 0)
		kp->board->kcode_tbl = (u8 *) kpd_lookup_tbl1;
	else if (u8500_keymap == 1)
		kp->board->kcode_tbl = (u8 *) kpd_lookup_tbl2;
	mutex_unlock(&u8500_keymap_mutex);
	/**
	 * read key data from stmpe1601
	 */
	err = stmpe1601_keypressed(&keys);
	if (keys.button_pressed) {
		for (i = 0; i < keys.button_pressed; i++) {
			kcode = kp->board->kcode_tbl[(keys.button[i] & 0x7) *
			8 + ((keys.button[i] >> 3) & 0x7)];
			input_report_key(kp->inp_dev, kcode, 1);
			if (kcode == KEY_RESERVED)
				printk(KERN_ERR "Error in key detection."
				" Key not present");
			DEBUG_KP(("\nkey (%d, %d)-->DOWN, kcode = %d",
				(keys.button[i] >> 3) & 0x7,
			(keys.button[i] & 0x7), kcode));
			keyp_cnt++;
		}
	}
	if (keys.button_released) {
		for (i = 0; i < keys.button_released; i++) {
			kcode = kp->board->kcode_tbl[(keys.released[i] & 0x7) *
			8 + ((keys.released[i] >> 3) & 0x7)];
			input_report_key(kp->inp_dev, kcode, 0);
			if (kcode == KEY_RESERVED)
				printk(KERN_ERR "Error in key detection."
				" Key not present");
			DEBUG_KP(("\nkey (%d, %d)-->UP, kcode = %d",
			(keys.released[i] >> 3) & 0x7,
			(keys.released[i] & 0x7), kcode));
			keyp_cnt--;
		}
	}
	if (keys.button_pressed || keys.button_released)
		DEBUG_KP(("\nkp_results_autoscan keyp_cnt = %d, pressed = %d "
			"released = %d\n",
			keyp_cnt, keys.button_pressed, keys.button_released));
	return keyp_cnt;
}

/**
 * u8500_kp_intrhandler - keypad interrupt handler
 * @dev_id : platform data
 * checks for valid interrupt, disables interrupt to avoid any nested interrupt
 * starts work queue for further key processing with debouncing logic
 */
irqreturn_t u8500_kp_intrhandler(/*int irq, */void *dev_id)
{
	struct keypad_t *kp = (struct keypad_t *)dev_id;
	/* printk("\n Kp interrupt"); */
	/* if (irq != kp->irq) return IRQ_NONE; */
	if (!(test_bit(KPINTR_LKBIT, &kp->lockbits))) {
		set_bit(KPINTR_LKBIT, &kp->lockbits);

		if (kp->board->irqdis_int)
			kp->board->irqdis_int(kp);

		/* schedule_delayed_work(&kp->kscan_work,
			kp->board->debounce_period); */
		schedule_delayed_work(&kp->kscan_work, 0);
	}
	return IRQ_HANDLED;
}

/**
 * u8500_kp_init_key_hardware -  keypad hardware initialization
 * @kp keypad configuration for this platform
 * Initializes the keypad hardware specific parameters.
 * This function will be called by u8500_keypad_init function during init
 * Initialize keypad interrupt handler for interrupt mode operation if enabled
 * Initialize Keyscan matrix
 *
 */
int u8500_kp_init_key_hardware(struct keypad_t *kp)
{
	int err;
	t_stmpe1601_key_config kpconfig;

	DEBUG_KP(("u8500_kp_init_key_hardware\n"));
	/*
	* Check if stmpe1601 is initialised properly, otherwise exit from here
	*/
	err = stmpe1601_read_info();
	if (err) {
		printk(KERN_ERR "\n Error in keypad init, keypad controller"
			"not initialised.");
		return -EIO;
	}
	/* setup Columns GPIOs (inputs) */
	kpconfig.columns  = kp->board->kcol; /* 0xFF 8 columns */
	kpconfig.rows     = kp->board->krow; /* 0xFF 8 rows */
	/* 8, number of cycles for key data updating */
	kpconfig.ncycles  = NB_SCAN_CYCLES;
	/* de-bounce time 64 ms */
	kpconfig.debounce = kp->board->debounce_period;
	kpconfig.scan     = STMPE1601_SCAN_OFF;
	err =  stmpe1601_keypad_init(kpconfig);
	if (err)
		printk(KERN_ERR "Couldn't setup keypad configuration\n");

	if (!kp->mode) {
		/*
		 * true if interrupt mode operation
		 */
		DEBUG_KP(("\nsetting up keypad interrupt & "
			"enable keypad intr\n"));
		err = stmpe1601_set_callback(kp->board->irq,
			u8500_kp_intrhandler, (void *)kp);
		if (err) {
			printk(KERN_ERR "\nCouldn't setup keypad callback\n");
			return err;
		}
		err = kp->board->irqen(kp);
			if (err) {
				printk(KERN_ERR "Couldn't enable the keypad "
					"source interrupt\n");
				return err;
			}
	}
	err = stmpe1601_keypad_scan(STMPE1601_SCAN_ON);
	if (err)
		printk(KERN_ERR "Couldn't enable keypad scan\n");

	return 0;
}

/**
 * u8500_kp_exit_key_hardware- keypad hardware exit function
 *
 * This function will be called by u8500_keypad_exit function during module
 * exit, frees keypad interrupt if enabled
 */
int u8500_kp_exit_key_hardware(struct keypad_t *kp)
{
	int err;

	DEBUG_KP(("nomadik_kp_exit_key_hardware\n"));
	err = stmpe1601_keypad_scan(STMPE1601_SCAN_OFF);
	if (err)
		printk(KERN_ERR "Couldn't disable keypad scan\n");

	if (!kp->mode)	{	/* true if interrupt mode operation */
		err = kp->board->irqdis(kp);
		if (err) {
			printk(KERN_ERR "Couldn't disable keypad callback\n");
			return err;
		}
		err = stmpe1601_remove_callback(kp->board->irq);
		if (err) {
			printk(KERN_ERR "Couldn't remove keypad callback\n");
			return err;
		}
	}
	return 0;
}
/**
 * u8500_kp_key_irqdis- disables keypad interrupt
 *
 * disables keypad interrupt
 */
int u8500_kp_key_irqdis(struct keypad_t *kp)
{
	unsigned long flags;
	/* Enable KEYPAD interrupt of STMPE 1601 */
	int err;
	/* DEBUG_KP(("u8500_kp_key_irqdis\n")); */
	err = stmpe1601_irqdis(kp->board->irq);
	/* err = stmpe1601_intsrc_set_state(STMPE1601_0,
		STMPE1601_KEYPAD_IRQ, STMPE1601_DISABLE_INTERRUPT );*/
	if (err) {
		printk(KERN_ERR "Error in disabling Keypad Interrupt.");
		return err;
	}
	spin_lock_irqsave(&cr_lock, flags);
	kp->board->int_status = KP_INT_DISABLED;
	spin_unlock_irqrestore(&cr_lock, flags);
	return 0;
}

/**
 * u8500_kp_key_irqen- enables keypad interrupt
 *
 * enables keypad interrupt
 */
int u8500_kp_key_irqen(struct keypad_t *kp)
{
	unsigned long flags;
	/* Enable KEYPAD interrupt of STMPE 1601*/
	int err;
	/* DEBUG_KP(("u8500_kp_key_irqen\n")); */
	err = stmpe1601_irqen(kp->board->irq);
	/* err = stmpe1601_intsrc_set_state(STMPE1601_0, STMPE1601_KEYPAD_IRQ,
		 STMPE1601_ENABLE_INTERRUPT ); */
	if (err)
		printk(KERN_ERR "Error in enabling Keypad Interrupt.= %d", err);
	spin_lock_irqsave(&cr_lock, flags);
	kp->board->int_status = KP_INT_ENABLED;
	spin_unlock_irqrestore(&cr_lock, flags);
	return 0;
}


static struct keypad_device keypad_board = {
	.init = u8500_kp_init_key_hardware,
	.exit = u8500_kp_exit_key_hardware,
	.irqen = u8500_kp_key_irqen,
	.irqdis_int = u8500_kp_key_irqdis,
	/* .irqdis = u8500_kp_key_irqdis, */
	/* .autoscan_disable = u8500_kp_disable_autoscan, */
	.autoscan_results = u8500_kp_results_autoscan,
	/* .autoscan_en=u8500_kp_autoscan_en, */
	.kcode_tbl = (u8 *) kpd_lookup_tbl1,
	.krow = KPD_NB_ROWS,
	.kcol = KPD_NB_COLUMNS,
	.debounce_period = KEYPAD_DEBOUNCE_PERIOD_STUIB,
	.irqtype = 0,
	.irq = KEYPAD_INT,
	/* .irq=GPIO_TO_IRQ(KEYPAD_INT), */
	.int_status = KP_INT_DISABLED,
	.int_line_behaviour = INT_LINE_NOTSET,
	.enable_wakeup = true,
};

static struct resource keypad_resources[] = {
	[0] = {
		.start = KEYPAD_INT,
		.end = KEYPAD_INT,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device keypad_device = {
	.name = "u8500-kp",
	.id = 0,
	.dev = {
		.platform_data = &keypad_board,
		},
	.num_resources = ARRAY_SIZE(keypad_resources),
	.resource = keypad_resources,
};
#endif /* CONFIG_KEYPAD_U8500 */

static struct i2s_board_info stm_i2s_board_info[] __initdata = {
	{
	 .modalias = "i2s_device.0",
	 .platform_data = NULL,
	 .id = (MSP_0_CONTROLLER - 1),
	 .chip_select = 0,
	 },
	{
	 .modalias = "i2s_device.1",
	 .platform_data = NULL,
	 .id = (MSP_1_CONTROLLER - 1),
	 .chip_select = 1,
	 },
	{
		/*
		 * XXX mop500_devices.c did not have this originally, and this
		 * was in device.c.  Required?
		 */
		.modalias	= "i2s_device.2",
		.platform_data	= NULL, /* can be fill some data */
		.id		= (MSP_2_CONTROLLER - 1),
		.chip_select	= 2,
	}

};

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
 * The board uses 4 i2c controllers, initialize all of
 * them with slave data setup time of 250 ns,
 * Tx & Rx FIFO threshold values as 1 and standard
 * mode of operation
 */
U8500_I2C_CONTROLLER(0, 0xe, 1, 1, 400000, I2C_FREQ_MODE_FAST);
U8500_I2C_CONTROLLER(1, 0xe, 1, 1, 400000, I2C_FREQ_MODE_FAST);
U8500_I2C_CONTROLLER(2,	0xe, 1, 1, 400000, I2C_FREQ_MODE_FAST);
U8500_I2C_CONTROLLER(3,	0xe, 1, 1, 400000, I2C_FREQ_MODE_FAST);
U8500_I2C_CONTROLLER(4,	0xe, 1, 1, 400000, I2C_FREQ_MODE_FAST);

static struct hsi_board_info __initdata stm_hsi_devices[] = {
	{.type = "HSI_LOOPBACK", .flags = 0, .controller_id = 0,
	 .chan_num = 0, .mode = 1},
	{.type = "HSI_LOOPBACK", .flags = 0, .controller_id = 1,
	 .chan_num = 0, .mode = 1},
	{.type = "HSI_LOOPBACK", .flags = 0, .controller_id = 0,
	 .chan_num = 1, .mode = 1},
	{.type = "HSI_LOOPBACK", .flags = 0, .controller_id = 1,
	 .chan_num = 1, .mode = 1},
	{.type = "HSI_LOOPBACK", .flags = 0, .controller_id = 0,
	 .chan_num = 2, .mode = 1},
	{.type = "HSI_LOOPBACK", .flags = 0, .controller_id = 1,
	 .chan_num = 2, .mode = 1},
	{.type = "HSI_LOOPBACK", .flags = 0, .controller_id = 0,
	 .chan_num = 3, .mode = 1},
	{.type = "HSI_LOOPBACK", .flags = 0, .controller_id = 1,
	 .chan_num = 3, .mode = 1},
};

static struct platform_device ab8500_gpadc_device = {
	.name = "ab8500_gpadc"
};

static struct ab8500_bm_platform_data ab8500_bm_plat_data = {
	.name = POWER_SUPPLY_TECHNOLOGY_LION,
	.termination_vol = 4200,
	.op_cur_lvl = CH_OP_CUR_LVL_0P9,
	.ip_vol_lvl = CH_VOL_LVL_4P2,
};

static struct platform_device ab8500_bm_device = {
	.name = "ab8500_bm",
	.dev = {
		.platform_data = &ab8500_bm_plat_data,
	},
};

static struct platform_device *u8500_platform_devices[] __initdata = {
	/*TODO - add platform devices here */
#ifdef CONFIG_KEYPAD_U8500
	&keypad_device,
#endif

};

static struct amba_device *amba_board_devs[] __initdata = {
	&ux500_uart0_device,
	&ux500_uart1_device,
	&ux500_uart2_device,
	&u8500_ssp0_device,
	&u8500_ssp1_device,
	&ux500_spi0_device,
	&u8500_msp2_spi_device,
};

static struct platform_device *platform_board_devs[] __initdata = {
	&u8500_msp0_device,
	&u8500_msp1_device,
	&u8500_msp2_device,
	&u8500_hsit_device,
	&u8500_hsir_device,
	&u8500_shrm_device,
	&u8500_ab8500_device,
	&ab8500_gpadc_device,
	&ab8500_bm_device,
	&ux500_musb_device,
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC0
	&u8500_mcde2_device,
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELC0 */
#ifdef CONFIG_FB_U8500_MCDE_CHANNELC1
	&u8500_mcde3_device,
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELC1 */
#ifdef CONFIG_FB_U8500_MCDE_CHANNELB
	&u8500_mcde1_device,
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELB */
#ifdef CONFIG_FB_U8500_MCDE_CHANNELA
	&u8500_mcde0_device,
#endif	/* CONFIG_FB_U8500_MCDE_CHANNELA */
	&ux500_b2r2_device,
	&u8500_pmem_device,
	&u8500_pmem_mio_device,
	&u8500_pmem_hwb_device,
};

static void __init mop500_platdata_init(void)
{
}


static void __init mop500_i2c_init(void)
{
	u8500_register_device(&u8500_i2c0_device, &u8500_i2c_0);
	u8500_register_device(&ux500_i2c_controller1, &u8500_i2c_1);
	u8500_register_device(&ux500_i2c_controller2, &u8500_i2c_2);
	u8500_register_device(&ux500_i2c_controller3, &u8500_i2c_3);

	if (!u8500_is_earlydrop())
		u8500_register_device(&u8500_i2c4_device, &u8500_i2c_4);
}

static void __init mop500_init_machine(void)
{
	stm_gpio_set_altfunctable(gpio_altfun_table,
				  ARRAY_SIZE(gpio_altfun_table));

	u8500_init_regulators();
	u8500_init_devices();

	mop500_platdata_init();

	amba_add_devices(amba_board_devs, ARRAY_SIZE(amba_board_devs));
	platform_add_devices(platform_board_devs, ARRAY_SIZE(platform_board_devs));

	mop500_i2c_init();

	/* enable RTC as a wakeup capable */
	device_init_wakeup(&ux500_rtc_device.dev, true);

	stm_gpio_altfuncenable(GPIO_ALT_UART_0_NO_MODEM);
	stm_gpio_altfuncenable(GPIO_ALT_UART_1);
	stm_gpio_altfuncenable(GPIO_ALT_UART_2);

	if (MOP500_PLATFORM_ID == platform_id)
		i2c_register_board_info(0, nmdk_i2c0_egpio_devices,
				ARRAY_SIZE(nmdk_i2c0_egpio_devices));
	else if (HREF_PLATFORM_ID == platform_id)
		i2c_register_board_info(0, nmdk_i2c0_egpio1_devices,
				ARRAY_SIZE(nmdk_i2c0_egpio1_devices));

	i2c_register_board_info(0, u8500_i2c0_devices,
				ARRAY_SIZE(u8500_i2c0_devices));
	i2c_register_board_info(1, u8500_i2c1_devices,
				ARRAY_SIZE(u8500_i2c1_devices));
	i2c_register_board_info(2, u8500_i2c2_devices,
				ARRAY_SIZE(u8500_i2c2_devices));
	i2c_register_board_info(3, u8500_i2c3_devices,
				ARRAY_SIZE(u8500_i2c3_devices));
	i2s_register_board_info(stm_i2s_board_info,
				ARRAY_SIZE(stm_i2s_board_info));
	hsi_register_board_info(stm_hsi_devices, ARRAY_SIZE(stm_hsi_devices));

	platform_add_devices(u8500_platform_devices,
			     ARRAY_SIZE(u8500_platform_devices));
}

static int __init u8500_i2call_init(void)
{
	stm_gpio_altfuncenable(GPIO_ALT_I2C_0);
	stm_gpio_altfuncenable(GPIO_ALT_I2C_1);
	stm_gpio_altfuncenable(GPIO_ALT_I2C_2);
	stm_gpio_altfuncenable(GPIO_ALT_I2C_3);

	/* Enable pullups for UART Rx lines. */
	nmk_gpio_set_pull(2, NMK_GPIO_PULL_UP);
	nmk_gpio_set_pull(4, NMK_GPIO_PULL_UP);
	nmk_gpio_set_pull(29, NMK_GPIO_PULL_UP);

	return 0;
}
subsys_initcall(u8500_i2call_init);
MACHINE_START(NOMADIK, "ST-Ericsson U8500 Platform")
	/* Maintainer: ST-Ericsson */
	.phys_io	= UX500_UART2_BASE,
	.io_pg_offst	= (IO_ADDRESS(UX500_UART2_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.map_io		= u8500_map_io,
	.init_irq	= ux500_init_irq,
	.timer		= &u8500_timer,
	.init_machine	= mop500_init_machine,
MACHINE_END
