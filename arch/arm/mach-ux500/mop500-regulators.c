/*
 * Copyright (C) 2009 ST-Ericsson SA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/regulator/machine.h>

#include <mach/devices.h>

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

#ifdef CONFIG_SENSORS1P_MOP
extern struct platform_device sensors1p_device;
#endif

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
#ifdef CONFIG_SENSORS1P_MOP
	{
		.dev = &sensors1p_device.dev,
		.supply = "v-proximity",
	},
	{
		.dev = &sensors1p_device.dev,
		.supply = "v-hal",
	},
#endif

};

static struct regulator_init_data ab8500_vaux1_init = {
	.supply_regulator_dev = NULL,
	.constraints = {
		.name = "AB8500-VAUX1",
		.min_uV = AB8500_VAUXN_LDO_MIN_VOLTAGE,
		.max_uV = AB8500_VAUXN_LDO_MAX_VOLTAGE,
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

/* VAUX3 supply */
/* supply for MMC-SD */
static struct regulator_consumer_supply ab8500_vaux3_consumers[] = {
	{
		.dev    = &ux500_sdi0_device.dev,
		.supply = "v-MMC-SD"
	}
};

static struct regulator_init_data ab8500_vaux3_init = {
	.supply_regulator_dev = NULL,
	.constraints = {
		.name = "AB8500-VAUX3",
		.min_uV = AB8500_VAUXN_LDO_MIN_VOLTAGE,
		.max_uV = AB8500_VAUXN_LDO_MAX_VOLTAGE,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE|
			REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_NORMAL|REGULATOR_MODE_IDLE,
	},
	.num_consumer_supplies = ARRAY_SIZE(ab8500_vaux3_consumers),
	.consumer_supplies = ab8500_vaux3_consumers,
};

static struct platform_device ab8500_vaux3_regulator_dev = {
	.name = "ab8500-regulator",
	.id   = 9,
	.dev  = {
		.platform_data = &ab8500_vaux3_init,
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
	&ab8500_vaux3_regulator_dev,
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
