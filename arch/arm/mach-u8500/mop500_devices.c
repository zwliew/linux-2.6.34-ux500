/*
 * Copyright (C) 2009 ST Ericsson.
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

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>

#include <mach/kpd.h>
#include <mach/stmpe2401.h>
#include <mach/stmpe1601.h>
#include <mach/tc35892.h>
#include <mach/av8100_p.h>
#include <mach/ab8500.h>

#include <mach/u8500_tsc.h>

#define BOARD_NAME      "BOARD MOP500"
#define MOP500_PLATFORM_ID 0
#define HREF_PLATFORM_ID 1
#define IRQ_KP 1 /*To DO*/
extern int platform_id;

static struct stmpe2401_platform_data stmpe_data = {
	.gpio_base = 268,
	.irq    = GPIO_TO_IRQ(217),
};
static struct av8100_platform_data av8100_plat_data = {
	/*.gpio_base = 224,*/
	.irq    = GPIO_TO_IRQ(192),
};

#ifdef CONFIG_I2C_BOARDINFO
#ifdef CONFIG_GPIO_STMPE1601
static struct stmpe1601_platform_data stmpe1601_data = {
	.gpio_base = (268 + 24), /* 268 GPIOs + 24 extended GPIOs ofSTMPE2401*/
	.irq    = GPIO_TO_IRQ(218),
};
#endif
static struct tc35892_platform_data tc35892_data = {
	.gpio_base = 268,
	.irq    = GPIO_TO_IRQ(217),
};

#if defined(CONFIG_GPIO_STMPE2401)
static struct i2c_board_info __initdata nmdk_i2c0_egpio_devices[] = {
	{
		/* stmpe2401 @ 0x84(w),0x85(r) */
		I2C_BOARD_INFO("stmpe2401", 0x42),
		.platform_data = &stmpe_data,
	}
};
#endif

#if defined(CONFIG_GPIO_TC35892)
static struct i2c_board_info __initdata nmdk_i2c0_egpio1_devices[] = {
	{

		I2C_BOARD_INFO("tc35892", 0x42),
		.platform_data = &tc35892_data,
	}
};
#endif

/**
 * Touch panel related platform specific initialization
 */
#if defined(CONFIG_U8500_TSC)

/**
 * tp_gpio_board_init : configure the voltage for touch
 * panel controller. This function can be used to configure
 * the voltage for touch panel controller.
 */
int tp_gpio_board_init(void)
{
	int val;

	/** Set the voltage for Bu21013 controller */
	val = ab8500_read(AB8500_REGU_CTRL2, AB8500_REGU_VAUX12_REGU_REG);

	ab8500_write(AB8500_REGU_CTRL2, AB8500_REGU_VAUX12_REGU_REG, (val|0x1));

	val = ab8500_read(AB8500_REGU_CTRL2, AB8500_REGU_VAUX1_SEL_REG);

	ab8500_write(AB8500_REGU_CTRL2, AB8500_REGU_VAUX1_SEL_REG, 0x0C);

	if (platform_id == MOP500_PLATFORM_ID)	{
		/** why set directtion is not working ~ FIXME */
		/* gpio_direction_output(270,1); */
		gpio_set_value(TOUCHP_CS0, 1);
	} else	{
	if (platform_id == HREF_PLATFORM_ID)	{
		gpio_direction_output(EGPIO_PIN_13, 1);
		gpio_set_value(EGPIO_PIN_13, 1);
		}
	}
	return 0;
}

/**
 * tp_gpio_board_init : deconfigures the touch panel controller
 * This function can be used to deconfigures the chip selection
 * for touch panel controller.
 */
int tp_gpio_board_exit(void)
{
	int val;

	if (platform_id == MOP500_PLATFORM_ID)	{
		/** why set directtion is not working ~ FIXME */
		/* gpio_direction_output(270,1); */
		gpio_set_value(TOUCHP_CS0, 0);
	} else if (platform_id == HREF_PLATFORM_ID) {
		gpio_direction_output(EGPIO_PIN_13, 0);
		gpio_set_value(EGPIO_PIN_13, 0);
	}
	return 0;
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

	if (platform_id == MOP500_PLATFORM_ID) 	{
		retval = stmpe2401_set_callback(TOUCHP_IRQ, callback,
			(void *)data);
		if (retval < 0)
			printk(KERN_ERR "tp_init_irq: stmpe2401_set_callback failed \n");
	} else if (platform_id == HREF_PLATFORM_ID) {
		retval = tc35892_set_callback(EGPIO_PIN_12, callback,
			(void *)data);
		if (retval < 0)
			printk(KERN_ERR "tp_init_irq: tc35892_set_callback failed \n");
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
	if (platform_id == MOP500_PLATFORM_ID)
		stmpe2401_remove_callback(TOUCHP_IRQ);
	else if (platform_id == HREF_PLATFORM_ID)
		tc35892_remove_callback(EGPIO_PIN_12);

	return retval;
}

/**
 * tp_pen_down_irq_enable : enable the interrupt for touch panel controller
 * This function can be used to enable the interrupt for touch panel controller.
 */
int tp_pen_down_irq_enable(void)
{
	int retval = 0;
	if (platform_id == MOP500_PLATFORM_ID)	{
		/* do nothing */
	} else if (platform_id == HREF_PLATFORM_ID) {
		retval = tc35892_set_gpio_intr_conf(EGPIO_PIN_12,
			EDGE_SENSITIVE,	TC35892_FALLING_EDGE_OR_LOWLEVEL);
		if (retval < 0)
			printk(KERN_ERR "tc35892_set_gpio_intr_conf failed\n");
		retval = tc35892_set_intr_enable(EGPIO_PIN_12,
			ENABLE_INTERRUPT);
		if (retval < 0)
			printk(KERN_ERR "tc35892_set_intr_enable failed \n");
	}
	return retval;
}

/**
 * tp_pen_down_irq_disable : disable the interrupt for
 * touch panel controller
 * This function can be used to disable the interrupt for
 * touch panel controller.
 */
int tp_pen_down_irq_disable(void)
{
	int retval = 0;

	if (platform_id == MOP500_PLATFORM_ID) {
		/* do nothing */
	} else if (platform_id == HREF_PLATFORM_ID) {
		retval = tc35892_set_intr_enable(EGPIO_PIN_12,
				DISABLE_INTERRUPT);
		if (retval < 0)
			printk(KERN_ERR "tsc_irqdis:"
			"tc35892_set_intr_enable failed \n");
	}
	return retval;
}

/**
 * tp_read_pin_val : get the interrupt pin value for touch panel controller
 * This function can be used to get the interrupt pin value for touch panel
 * controller.
 */
int tp_read_pin_val(void)
{
	int data = 0;

	if (platform_id == MOP500_PLATFORM_ID)
		data = gpio_get_value(TOUCHP_IRQ);
	else if (platform_id == HREF_PLATFORM_ID)
		data = gpio_get_value(EGPIO_PIN_12);

	return data;
}

static struct tp_device tsc_plat_device = {
	.cs_en = tp_gpio_board_init,
	.cs_dis = tp_gpio_board_exit,
	.irq_init = tp_init_irq,
	.irq_exit = tp_exit_irq,
	.pirq_en  = tp_pen_down_irq_enable,
	.pirq_dis = tp_pen_down_irq_disable,
	.pirq_read_val = tp_read_pin_val,
};
#endif

static struct i2c_board_info __initdata u8500_i2c0_devices[] = {
#if defined(CONFIG_GPIO_STMPE1601)
	{
		I2C_BOARD_INFO("stmpe1601", 0x40),
		.platform_data = &stmpe1601_data,
	},
#endif
#if defined(CONFIG_U8500_HDMI)
	{
		I2C_BOARD_INFO("av8100", 0x70),
	},
#endif
#if defined(CONFIG_U8500_UIB_KEYPAD)
	{
	 /* FIXME - address TBD, TODO */
	 I2C_BOARD_INFO("uib-kpd", 0x45),
	},
#endif
#if defined(CONFIG_U8500_AUDIO_STEPUP)
	{
	 /* Audio step-up */
	 I2C_BOARD_INFO("tps61052", 0x33),
	}
#endif
};
static struct i2c_board_info __initdata u8500_i2c1_devices[] = {
#if defined(CONFIG_U8500_AUDIO_GPS)
	{
	 /* GPS - Address TBD, FIXME */
	 I2C_BOARD_INFO("gps", 0x68),
	},
#endif
#if defined(CONFIG_MFD_AB3550_CORE)
	{
		/* AB3550 */
		I2C_BOARD_INFO("ab3550", 0x4A),
	}
#endif
};

static struct i2c_board_info __initdata u8500_i2c2_devices[] = {
#if defined(CONFIG_U8500_AUDIO_MEMS)
	{
	 /* ST 3D accelerometer @ 0x3A(w),0x3B(r) */
	 I2C_BOARD_INFO("lis302dl", 0x1D),
	},
#endif
#if defined(CONFIG_U8500_MAGNETOMETER)
	{
	 /* ASAHI KASEI AK8974 magnetometer, addr TBD FIXME */
	 I2C_BOARD_INFO("ak8974", 0x1),
	},
#endif
#if defined(CONFIG_U8500_LIGHT_SENSOR)
	{
	 /* Rohm BH1780GLI light sensor addr TBD, FIXME */
	 I2C_BOARD_INFO("bh1780gli", 0x45),
	},
#endif
#if defined(CONFIG_U8500_LED)
	{
	 /* RGB LED driver, there are 1st and 2nd, TODO */
	 I2C_BOARD_INFO("lp5521tmx", 0x33),
	}
#endif
};

static struct i2c_board_info __initdata u8500_i2c3_devices[] = {
#if defined(CONFIG_U8500_NFC)
	{
	 /* NFC - Address TBD, FIXME */
	 I2C_BOARD_INFO("nfc", 0x68),
	},
#endif
#if defined(CONFIG_U8500_TSC)
	{
		/* Touschscreen */
		I2C_BOARD_INFO(DRIVER_TP1, I2C3_TOUCHP_ADDRESS),
		.platform_data = &tsc_plat_device,
	},
#endif

};
#endif

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

spinlock_t cr_lock = SPIN_LOCK_UNLOCKED;

extern irqreturn_t u8500_kp_intrhandler(/*int irq,*/ void *dev_id);

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
		DEBUG_KP(("\nkp_results_autoscan keyp_cnt = %d, pressed = %d ",
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
		____atomic_set_bit(KPINTR_LKBIT, &kp->lockbits);

		if (kp->board->irqdis_int)
			kp->board->irqdis_int(kp);

		/* schedule_delayed_work(&kp->kscan_work,
			kp->board->debounce_period); */
		schedule_work(&kp->kscan_work);
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


/*
 * Initializes the key scan table (lookup table) as per pre-defined the scan
 * codes to be passed to upper layer with respective key codes
 */
u8 const kpd_lookup_tbl[MAX_KPROW][MAX_KPROW] = {
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_9, KEY_RESERVED, KEY_RESERVED},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_POWER, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_7,
	KEY_VOLUMEUP, KEY_6, KEY_BACK, KEY_RESERVED},
	{KEY_RESERVED, KEY_3, KEY_RESERVED, KEY_8,
	KEY_4, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_5,
	KEY_RESERVED, KEY_RESERVED, KEY_SEND, KEY_RESERVED},
	{KEY_MENU, KEY_RESERVED, KEY_END, KEY_VOLUMEDOWN,
	KEY_0, KEY_1, KEY_RESERVED, KEY_RESERVED},
	{KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_ENTER},
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
static struct keypad_device keypad_board = {
	.init = u8500_kp_init_key_hardware,
	.exit = u8500_kp_exit_key_hardware,
	.irqen = u8500_kp_key_irqen,
	.irqdis_int = u8500_kp_key_irqdis,
	/* .irqdis = u8500_kp_key_irqdis, */
	/* .autoscan_disable = u8500_kp_disable_autoscan, */
	.autoscan_results = u8500_kp_results_autoscan,
	/* .autoscan_en=u8500_kp_autoscan_en, */
	.kcode_tbl = (u8 *) kpd_lookup_tbl,
	.krow = KPD_NB_ROWS,
	.kcol = KPD_NB_COLUMNS,
	.debounce_period = KEYPAD_DEBOUNCE_PERIOD_STUIB,
	.irqtype = 0,
	.irq = KEYPAD_INT,
	/* .irq=GPIO_TO_IRQ(KEYPAD_INT), */
	.int_status = KP_INT_DISABLED,
	.int_line_behaviour = INT_LINE_NOTSET,
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

#ifdef CONFIG_STM_I2S
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

};
#endif

static struct hsi_board_info __initdata stm_hsi_devices[] = {
#ifdef CONFIG_HSI
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
#endif
};

static struct platform_device *u8500_platform_devices[] __initdata = {
	/*TODO - add platform devices here */
#ifdef CONFIG_KEYPAD_U8500
	&keypad_device,
#endif

};
__init void add_u8500_platform_devices(void)
{
	printk(KERN_INFO "%s(): registering device resources\n", __func__);

#ifdef CONFIG_I2C_BOARDINFO
#if defined(CONFIG_GPIO_STMPE2401) || defined(CONFIG_GPIO_TC35892)
	if (MOP500_PLATFORM_ID == platform_id) {
		i2c_register_board_info(0, nmdk_i2c0_egpio_devices,
				ARRAY_SIZE(nmdk_i2c0_egpio_devices));
	} else if (HREF_PLATFORM_ID == platform_id) {
		i2c_register_board_info(0, nmdk_i2c0_egpio1_devices,
				ARRAY_SIZE(nmdk_i2c0_egpio1_devices));
	}
#endif
	i2c_register_board_info(0, u8500_i2c0_devices,
				ARRAY_SIZE(u8500_i2c0_devices));
	i2c_register_board_info(1, u8500_i2c1_devices,
				ARRAY_SIZE(u8500_i2c1_devices));
	i2c_register_board_info(2, u8500_i2c2_devices,
				ARRAY_SIZE(u8500_i2c2_devices));
	i2c_register_board_info(3, u8500_i2c3_devices,
				ARRAY_SIZE(u8500_i2c3_devices));
#endif
#ifdef CONFIG_STM_I2S
	i2s_register_board_info(stm_i2s_board_info,
				ARRAY_SIZE(stm_i2s_board_info));
#endif
	hsi_register_board_info(stm_hsi_devices, ARRAY_SIZE(stm_hsi_devices));

	platform_add_devices(u8500_platform_devices,
			     ARRAY_SIZE(u8500_platform_devices));
}
