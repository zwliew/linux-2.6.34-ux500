/*
 * Copyright (C) ST-Ericsson SA 2010
 * License terms: GNU General Public License v2
 * AB3100 core access functions
 * Author: Linus Walleij <linus.walleij@stericsson.com>
 *
 * ABX500 core access functions.
 * The abx500 interface is used for the Analog Baseband chip
 * ab3100, ab3550, ab5500 and possibly comming. It is not used for
 * ab4500 and ab8500 since they are another family of chip.
 *
 * Author: Mattias Wallin <mattias.wallin@stericsson.com>
 * Author: Mattias Nilsson <mattias.i.nilsson@stericsson.com>
 * Author: Bengt Jonsson <bengt.g.jonsson@stericsson.com>
 * Author: Rickard Andersson <rickard.andersson@stericsson.com>
 */

#include <linux/device.h>
#include <linux/regulator/machine.h>

#ifndef MFD_ABX500_H
#define MFD_ABX500_H

#define AB3100_P1A	0xc0
#define AB3100_P1B	0xc1
#define AB3100_P1C	0xc2
#define AB3100_P1D	0xc3
#define AB3100_P1E	0xc4
#define AB3100_P1F	0xc5
#define AB3100_P1G	0xc6
#define AB3100_R2A	0xc7
#define AB3100_R2B	0xc8
#define AB3550_P1A	0x10
#define AB5500_1_0	0x20
#define AB5500_2_0	0x21
#define AB5500_2_1	0x22

/*
 * AB3100, EVENTA1, A2 and A3 event register flags
 * these are catenated into a single 32-bit flag in the code
 * for event notification broadcasts.
 */
#define AB3100_EVENTA1_ONSWA				(0x01<<16)
#define AB3100_EVENTA1_ONSWB				(0x02<<16)
#define AB3100_EVENTA1_ONSWC				(0x04<<16)
#define AB3100_EVENTA1_DCIO				(0x08<<16)
#define AB3100_EVENTA1_OVER_TEMP			(0x10<<16)
#define AB3100_EVENTA1_SIM_OFF				(0x20<<16)
#define AB3100_EVENTA1_VBUS				(0x40<<16)
#define AB3100_EVENTA1_VSET_USB				(0x80<<16)

#define AB3100_EVENTA2_READY_TX				(0x01<<8)
#define AB3100_EVENTA2_READY_RX				(0x02<<8)
#define AB3100_EVENTA2_OVERRUN_ERROR			(0x04<<8)
#define AB3100_EVENTA2_FRAMING_ERROR			(0x08<<8)
#define AB3100_EVENTA2_CHARG_OVERCURRENT		(0x10<<8)
#define AB3100_EVENTA2_MIDR				(0x20<<8)
#define AB3100_EVENTA2_BATTERY_REM			(0x40<<8)
#define AB3100_EVENTA2_ALARM				(0x80<<8)

#define AB3100_EVENTA3_ADC_TRIG5			(0x01)
#define AB3100_EVENTA3_ADC_TRIG4			(0x02)
#define AB3100_EVENTA3_ADC_TRIG3			(0x04)
#define AB3100_EVENTA3_ADC_TRIG2			(0x08)
#define AB3100_EVENTA3_ADC_TRIGVBAT			(0x10)
#define AB3100_EVENTA3_ADC_TRIGVTX			(0x20)
#define AB3100_EVENTA3_ADC_TRIG1			(0x40)
#define AB3100_EVENTA3_ADC_TRIG0			(0x80)

/* AB3100, STR register flags */
#define AB3100_STR_ONSWA				(0x01)
#define AB3100_STR_ONSWB				(0x02)
#define AB3100_STR_ONSWC				(0x04)
#define AB3100_STR_DCIO					(0x08)
#define AB3100_STR_BOOT_MODE				(0x10)
#define AB3100_STR_SIM_OFF				(0x20)
#define AB3100_STR_BATT_REMOVAL				(0x40)
#define AB3100_STR_VBUS					(0x80)

/*
 * AB3100 contains 8 regulators, one external regulator controller
 * and a buck converter, further the LDO E and buck converter can
 * have separate settings if they are in sleep mode, this is
 * modeled as a separate regulator.
 */
#define AB3100_NUM_REGULATORS				10

/**
 * struct ab3100
 * @access_mutex: lock out concurrent accesses to the AB3100 registers
 * @dev: pointer to the containing device
 * @i2c_client: I2C client for this chip
 * @testreg_client: secondary client for test registers
 * @chip_name: name of this chip variant
 * @chip_id: 8 bit chip ID for this chip variant
 * @event_subscribers: event subscribers are listed here
 * @startup_events: a copy of the first reading of the event registers
 * @startup_events_read: whether the first events have been read
 *
 * This struct is PRIVATE and devices using it should NOT
 * access ANY fields. It is used as a token for calling the
 * AB3100 functions.
 */
struct ab3100 {
	struct mutex access_mutex;
	struct device *dev;
	struct i2c_client *i2c_client;
	struct i2c_client *testreg_client;
	char chip_name[32];
	u8 chip_id;
	struct blocking_notifier_head event_subscribers;
	u8 startup_events[3];
	bool startup_events_read;
};

/**
 * struct ab3100_platform_data
 * Data supplied to initialize board connections to the AB3100
 * @reg_constraints: regulator constraints for target board
 *     the order of these constraints are: LDO A, C, D, E,
 *     F, G, H, K, EXT and BUCK.
 * @reg_initvals: initial values for the regulator registers
 *     plus two sleep settings for LDO E and the BUCK converter.
 *     exactly AB3100_NUM_REGULATORS+2 values must be sent in.
 *     Order: LDO A, C, E, E sleep, F, G, H, K, EXT, BUCK,
 *     BUCK sleep, LDO D. (LDO D need to be initialized last.)
 * @external_voltage: voltage level of the external regulator.
 */
struct ab3100_platform_data {
	struct regulator_init_data reg_constraints[AB3100_NUM_REGULATORS];
	u8 reg_initvals[AB3100_NUM_REGULATORS+2];
	int external_voltage;
};

/**
 * ab3100_event_register() - Register a notifier to an event
 * @ab3100: The ab3100 subdevice
 * @nb: The notifier block to be used
 */
int ab3100_event_register(struct ab3100 *ab3100,
			  struct notifier_block *nb);

/**
 * ab3100_event_unregister() - Unregister a notifier to an event
 * @ab3100: The ab3100 subdevice
 * @nb: The notifier block to be used
 */
int ab3100_event_unregister(struct ab3100 *ab3100,
			    struct notifier_block *nb);

/* AB3550, STR register flags */
#define AB3550_STR_ONSWA				(0x01)
#define AB3550_STR_ONSWB				(0x02)
#define AB3550_STR_ONSWC				(0x04)
#define AB3550_STR_DCIO					(0x08)
#define AB3550_STR_BOOT_MODE				(0x10)
#define AB3550_STR_SIM_OFF				(0x20)
#define AB3550_STR_BATT_REMOVAL				(0x40)
#define AB3550_STR_VBUS					(0x80)

/* Interrupt mask registers */
#define AB3550_IMR1 0x29
#define AB3550_IMR2 0x2a
#define AB3550_IMR3 0x2b
#define AB3550_IMR4 0x2c
#define AB3550_IMR5 0x2d

/**
 * enum ab3550_devid
 * @AB3550_DEVID_ADC: Analog Digital Converter subdevice id
 * @AB3550_DEVID_DAC: Digital Analog Converter subdevice id
 * @AB3550_DEVID_LEDS: Leds subdevice id
 * @AB3550_DEVID_POWER: Power subdevice id
 * @AB3550_DEVID_REGULATORS: Regulator subdevice id
 * @AB3550_DEVID_SIM: SIM subdevice id
 * @AB3550_DEVID_UART: UART subdevice id
 * @AB3550_DEVID_RTC: RTC subdevice id
 * @AB3550_DEVID_CHARGER: Charger subdevice id
 * @AB3550_DEVID_FUELGAUGE: Fuelgauge subdevice id
 * @AB3550_DEVID_VIBRATOR: Vibrator subdevice id
 * @AB3550_DEVID_CODEC: ALSA Codec subdevice id
 * @AB3550_NUM_DEVICES: Number of subdevices
 */

enum ab3550_devid {
	AB3550_DEVID_ADC,
	AB3550_DEVID_DAC,
	AB3550_DEVID_LEDS,
	AB3550_DEVID_POWER,
	AB3550_DEVID_REGULATORS,
	AB3550_DEVID_SIM,
	AB3550_DEVID_UART,
	AB3550_DEVID_RTC,
	AB3550_DEVID_CHARGER,
	AB3550_DEVID_FUELGAUGE,
	AB3550_DEVID_VIBRATOR,
	AB3550_DEVID_CODEC,
	AB3550_NUM_DEVICES,
};

/**
 * struct abx500_init_setting
 * @bank: I2C bank to set
 * @reg: I2C register address
 * @setting: I2C value to set on above register and bank
 *
 * Initial value of the registers for driver to use during setup.
 */
struct abx500_init_settings {
	u8 bank;
	u8 reg;
	u8 setting;
};

/**
 * struct ab3550_platform_data
 * @irq: Interrupt base and count used for events to the subdrivers
 * @dev_data: Device specific data
 * @dev_data_sz: Size of dev_data in bytes
 * @init_settings: Initial I2C register settings
 * @init_settings_sz: Size of init_settings in bytes
 *
 * Data supplied to initialize board connections to the AB3550
 */
struct ab3550_platform_data {
	struct {unsigned int base; unsigned int count; } irq;
	void *dev_data[AB3550_NUM_DEVICES];
	size_t dev_data_sz[AB3550_NUM_DEVICES];
	struct abx500_init_settings *init_settings;
	unsigned int init_settings_sz;
};

enum ab5500_devid {
	AB5500_DEVID_ADC,
	AB5500_DEVID_LEDS,
	AB5500_DEVID_POWER,
	AB5500_DEVID_REGULATORS,
	AB5500_DEVID_SIM,
	AB5500_DEVID_RTC,
	AB5500_DEVID_CHARGER,
	AB5500_DEVID_FUELGAUGE,
	AB5500_DEVID_VIBRATOR,
	AB5500_DEVID_CODEC,
	AB5500_DEVID_USB,
	AB5500_DEVID_OTP,
	AB5500_DEVID_VIDEO,
	AB5500_DEVID_DBIECI,
	AB5500_NUM_DEVICES,
};

enum ab5500_banks {
	AB5500_BANK_VIT_IO_I2C_CLK_TST_OTP = 0,
	AB5500_BANK_VDDDIG_IO_I2C_CLK_TST = 1,
	AB5500_BANK_VDENC = 2,
	AB5500_BANK_SIM_USBSIM  = 3,
	AB5500_BANK_LED = 4,
	AB5500_BANK_ADC  = 5,
	AB5500_BANK_RTC  = 6,
	AB5500_BANK_STARTUP  = 7,
	AB5500_BANK_DBI_ECI  = 8,
	AB5500_BANK_CHG  = 9,
	AB5500_BANK_FG_BATTCOM_ACC = 10,
	AB5500_BANK_USB = 11,
	AB5500_BANK_IT = 12,
	AB5500_BANK_VIBRA = 13,
	AB5500_BANK_AUDIO_HEADSETUSB = 14,
	AB5500_NUM_BANKS = 15,
};

/**
 * struct ab5500_platform_data - Platform data for ab5500
 * @irq: Interrupt base and count used for events to the subdrivers
 * @dev_data: Device specific data
 * @dev_data_sz: Size of dev_data in bytes
 * @init_settings: Initial I2C register settings
 * @init_settings_sz: Size of init_settings in bytes
 *
 * Data supplied to initialize board connections to the AB5500
 */
struct ab5500_platform_data {
	struct {unsigned int base; unsigned int count; } irq;
	void *dev_data[AB5500_NUM_DEVICES];
	size_t dev_data_sz[AB5500_NUM_DEVICES];
	struct abx500_init_settings *init_settings;
	unsigned int init_settings_sz;
};

/**
 * abx500_set_register_interruptible() - Set one target register
 * @dev: The AB subdevice
 * @bank: The I2C bank
 * @reg: The I2C register address
 * @value: The I2C value to written on above bank and reg
 *
 * Write to one I2C register.
 */
int abx500_set_register_interruptible(struct device *dev, u8 bank, u8 reg,
	u8 value);

/**
 * abx500_get_register_interruptible() - Get one target register
 * @dev: The AB subdevice
 * @bank: The I2C bank
 * @reg: The I2C register address
 * @value: The I2C value read from above bank and reg
 *
 * Read one I2C register.
 */
int abx500_get_register_interruptible(struct device *dev, u8 bank, u8 reg,
	u8 *value);

/**
 * abx500_get_register_page_interruptible() - Get several target registers
 * @dev: The AB subdevice
 * @bank: The I2C bank
 * @first_reg: The first I2C register address
 * @regvals: The I2C values read from above bank and registers
 * @numregs: The number of registers to read
 *
 * Read I2C registers.
 */
int abx500_get_register_page_interruptible(struct device *dev, u8 bank,
	u8 first_reg, u8 *regvals, u8 numregs);

/**
 * abx500_set_register_page_interruptible() - Set several target registers
 * @dev: The AB subdevice
 * @bank: The I2C bank
 * @first_reg: The first I2C register address
 * @regvals: The I2C values to be written to above bank and registers
 * @numregs: The number of registers to read
 *
 * Read I2C registers.
 */
int abx500_set_register_page_interruptible(struct device *dev, u8 bank,
	u8 first_reg, u8 *regvals, u8 numregs);

/**
 * abx500_mask_and_set_register_inerruptible() - Modifies selected bits of a
 *	target register
 *
 * @dev: The AB subdevice
 * @bank: The I2c bank number
 * @reg: The I2C register address
 * @bitmask: The bit mask to use
 * @bitvalues: The new bit values
 *
 * Updates the value of an I2C register:
 * value -> ((value & ~bitmask) | (bitvalues & bitmask))
 */
int abx500_mask_and_set_register_interruptible(struct device *dev, u8 bank,
	u8 reg, u8 bitmask, u8 bitvalues);

/**
 * abx500_get_chip_id() - Retreive the chip id
 * @dev: The AB subdevice
 *
 * Read the chip id from the chip
 */
int abx500_get_chip_id(struct device *dev);

/**
 * abx500_event_registers_startup_state_get() - Get the startup state
 * @dev: The AB subdevice
 * @event: The event bitmask of the startup events
 *
 * Get an event bitmask over which events was enabled at startup.
 * This function is depricated. Use abx500_startup_irq_enabled istead.
 */
int abx500_event_registers_startup_state_get(struct device *dev, u8 *event);

/**
 * abx500_startup_irq_enabled() - Check if an event was enabled at startup
 * @dev: The AB subdevice
 * @irq: The interrupt (or event)
 *
 * This functions returns 1 if an interrupt was generated at startup and
 * 0 otherwise. Since subdevices are not started during startup the state
 * is saved and subdevices can later ask through this functions what
 * startupevents caused the startup.
 */
int abx500_startup_irq_enabled(struct device *dev, unsigned int irq);

/**
 * struct abx500_ops - I2C access funtions
 * @get_chip_id: Get chip id
 * @get_register: Get one register
 * @set_register: Set one register
 * @get_register_page: Get several registers
 * @set_register_page: Set several registers
 * @mask_and_set_register: Mask and set register
 * @event_registers_startup_state_get: Get the startupstate
 * @startup_irq_enabled: Check if a an event was enabled at startup
 *
 * These operations should be used by a subdevice to register a set
 * of I2C register access funcions.
 */
struct abx500_ops {
	int (*get_chip_id) (struct device *);
	int (*get_register) (struct device *, u8, u8, u8 *);
	int (*set_register) (struct device *, u8, u8, u8);
	int (*get_register_page) (struct device *, u8, u8, u8 *, u8);
	int (*set_register_page) (struct device *, u8, u8, u8 *, u8);
	int (*mask_and_set_register) (struct device *, u8, u8, u8, u8);
	int (*event_registers_startup_state_get) (struct device *, u8 *);
	int (*startup_irq_enabled) (struct device *, unsigned int);
};

/**
 * abx500_regiser_ops() - Register abx500 I2C access funcions
 * @dev: The mixed signal core device
 * @ops: The set of functions to register
 *
 * Register a set of I2C access functions to the abx500 framework.
 * This is done to keep a separate namespace for several mixed signal
 * chip drivers to live side by side.
 */
int abx500_register_ops(struct device *dev, struct abx500_ops *ops);
#endif
