/*
 * Copyright (C) 2010 ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/mmc/host.h>
#include <linux/delay.h>

#include <mach/devices.h>
#include <mach/mmc.h>
#include <mach/stmpe2401.h>
#include <mach/tc35892.h>
#include <mach/ab8500.h>

/*
 * SDI0 (SD/MMC card)
 */

static int mmc_configure(struct amba_device *dev)
{
	int pin[2];
	int ret;
	int i;

	/* Level-shifter GPIOs */
	if (MOP500_PLATFORM_ID == platform_id)	{
		pin[0] = EGPIO_PIN_18;
		pin[1] = EGPIO_PIN_19;
	} else if (HREF_PLATFORM_ID == platform_id) {
		pin[0] = EGPIO_PIN_17;
		pin[1] = EGPIO_PIN_18;
	} else
		BUG();

	ret = gpio_request(pin[0], "level shifter");
	if (!ret)
		ret = gpio_request(pin[1], "level shifter");

	if (!ret) {
		gpio_direction_output(pin[0], 1);
		gpio_direction_output(pin[1], 1);

		gpio_set_value(pin[0], 1);
#if defined(CONFIG_LEVELSHIFTER_HREF_V1_PLUS)
		gpio_set_value(pin[1], 0);
#else
		gpio_set_value(pin[1], 1);
#endif
	} else
		dev_err(&dev->dev, "unable to configure gpios\n");

#if defined(CONFIG_GPIO_TC35892)
	if (HREF_PLATFORM_ID == platform_id) {
		/* Enabling the card detection interrupt */
		tc35892_set_gpio_intr_conf(EGPIO_PIN_3, EDGE_SENSITIVE,
				TC35892_BOTH_EDGE);
		tc35892_set_intr_enable(EGPIO_PIN_3, ENABLE_INTERRUPT);
	}
#endif

	for (i = 18; i <= 28; i++)
		nmk_gpio_set_pull(i, NMK_GPIO_PULL_UP);

	stm_gpio_altfuncenable(GPIO_ALT_SDMMC);
	return ret;
}

static void mmc_set_power(struct device *dev, int power_on)
{
	if (platform_id == MOP500_PLATFORM_ID)
		gpio_set_value(EGPIO_PIN_18, !!power_on);
	else if (platform_id == HREF_PLATFORM_ID)
		gpio_set_value(EGPIO_PIN_17, !!power_on);
}

static void mmc_restore_default(struct amba_device *dev)
{

}

static int mmc_card_detect(void (*callback)(void *parameter), void *host)
{
	/*
	 * Card detection interrupt request
	 */
#if defined(CONFIG_GPIO_STMPE2401)
	if (MOP500_PLATFORM_ID == platform_id)
		stmpe2401_set_callback(EGPIO_PIN_16, callback, host);
#endif

#if defined(CONFIG_GPIO_TC35892)
	if (HREF_PLATFORM_ID == platform_id) {
		tc35892_set_gpio_intr_conf(EGPIO_PIN_3, EDGE_SENSITIVE, TC35892_BOTH_EDGE);
		tc35892_set_intr_enable(EGPIO_PIN_3, ENABLE_INTERRUPT);
		tc35892_set_callback(EGPIO_PIN_3, callback, host);
	}
#endif
	return 0;
}

static int mmc_get_carddetect_intr_value(void)
{
	int status = 0;

	if (MOP500_PLATFORM_ID == platform_id)
		status = gpio_get_value(EGPIO_PIN_16);
	else if (HREF_PLATFORM_ID == platform_id)
		status = gpio_get_value(EGPIO_PIN_3);

	return status;
}

static struct mmc_board mmc_data = {
	.init = mmc_configure,
	.exit = mmc_restore_default,
	.set_power = mmc_set_power,
	.card_detect = mmc_card_detect,
	.card_detect_intr_value = mmc_get_carddetect_intr_value,
	.dma_fifo_addr = U8500_SDI0_BASE + SD_MMC_TX_RX_REG_OFFSET,
	.dma_fifo_dev_type_rx = DMA_DEV_SD_MM0_RX,
	.dma_fifo_dev_type_tx = DMA_DEV_SD_MM0_TX,
	.level_shifter = 1,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED |
					MMC_CAP_MMC_HIGHSPEED,
#ifdef CONFIG_REGULATOR
	.supply = "v-MMC-SD" /* tying to VAUX3 regulator */
#endif
};

/*
 * SDI1 (SDIO WLAN)
 */

/* sdio specific configurations */
static int sdio_configure(struct amba_device *dev)
{
	int i;
	int status;

	status = gpio_request(215, "sdio_init");
	if (status) {
		dev_err(&dev->dev, "Unable to request gpio 215");
		return status;
	}

	gpio_direction_output(215, 1);
	gpio_set_value(215, 0);
	mdelay(10);
	gpio_set_value(213, 1);
	mdelay(10);
	gpio_set_value(215, 1);
	mdelay(10);
	gpio_set_value(213, 0);
	mdelay(10);

	gpio_free(215);

	for (i = 208; i <= 214; i++)
		nmk_gpio_set_pull(i, NMK_GPIO_PULL_UP);

	stm_gpio_altfuncenable(GPIO_ALT_SDIO);

	return 0;
}

static void sdio_restore_default(struct amba_device *dev)
{
    stm_gpio_altfuncdisable(GPIO_ALT_SDIO);
}

static struct mmc_board sdi1_data = {
	.init = sdio_configure,
	.exit = sdio_restore_default,
	.dma_fifo_addr = U8500_SDI1_BASE + SD_MMC_TX_RX_REG_OFFSET,
	.dma_fifo_dev_type_rx = DMA_DEV_SD_MM1_RX,
	.dma_fifo_dev_type_tx = DMA_DEV_SD_MM1_TX,
	.level_shifter = 0,
#ifdef CONFIG_U8500_SDIO_CARD_IRQ
	.caps = (MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ),
#else
	.caps = MMC_CAP_4_BIT_DATA,
#endif
	.is_sdio = 1,
};

/*
 * SDI2 (POPed eMMC on v1)
 */

static int sdi2_init(struct amba_device *dev)
{
	int i;

	for (i = 128; i <= 138; i++)
		nmk_gpio_set_pull(i, NMK_GPIO_PULL_UP);

	stm_gpio_altfuncenable(GPIO_ALT_SDMMC2);

	return 0;
}

static void sdi2_exit(struct amba_device *dev)
{
	stm_gpio_altfuncdisable(GPIO_ALT_SDMMC2);
}

static struct mmc_board sdi2_data = {
	.init = sdi2_init,
	.exit = sdi2_exit,
	.dma_fifo_addr = U8500_SDI2_BASE + SD_MMC_TX_RX_REG_OFFSET,
	.dma_fifo_dev_type_rx = DMA_DEV_SD_MM2_RX,
	.dma_fifo_dev_type_tx = DMA_DEV_SD_MM2_TX,
	.level_shifter = 0,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA,
};

/*
 * SDI4 (On-board EMMC)
 */

static int emmc_configure(struct amba_device *dev)
{
	int i;

	for (i = 197; i <= 207; i++)
		nmk_gpio_set_pull(i, NMK_GPIO_PULL_UP);
	stm_gpio_altfuncenable(GPIO_ALT_EMMC);

#ifndef CONFIG_REGULATOR
	if (!cpu_is_u8500ed()) {
		int val;

		/* On V1 MOP, regulator to on-board eMMC is off by default */
		val = ab8500_read(AB8500_REGU_CTRL2,
					AB8500_REGU_VAUX12_REGU_REG);
		ab8500_write(AB8500_REGU_CTRL2, AB8500_REGU_VAUX12_REGU_REG,
					val | 0x4);

		val = ab8500_read(AB8500_REGU_CTRL2,
				AB8500_REGU_VAUX2_SEL_REG);
		ab8500_write(AB8500_REGU_CTRL2, AB8500_REGU_VAUX2_SEL_REG,
					0x0C);
	}
#endif

	return 0;
}

static void emmc_restore_default(struct amba_device *dev)
{
	stm_gpio_altfuncdisable(GPIO_ALT_EMMC);
}

static struct mmc_board emmc_data = {
	.init = emmc_configure,
	.exit = emmc_restore_default,
	.dma_fifo_addr = U8500_SDI4_BASE + SD_MMC_TX_RX_REG_OFFSET,
	.dma_fifo_dev_type_rx = DMA_DEV_SD_MM4_RX,
	.dma_fifo_dev_type_tx = DMA_DEV_SD_MM4_TX,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA | MMC_CAP_MMC_HIGHSPEED,
#ifdef CONFIG_REGULATOR
	.supply = "v-eMMC" /* tying to VAUX1 regulator */
#endif
};

static int __init mop500_sdi_init(void)
{
	if (!cpu_is_u8500ed())
		u8500_register_amba_device(&ux500_sdi2_device, &sdi2_data);

	u8500_register_amba_device(&ux500_sdi4_device, &emmc_data);
	u8500_register_amba_device(&ux500_sdi0_device, &mmc_data);
#ifdef CONFIG_U8500_SDIO
	u8500_register_amba_device(&ux500_sdi1_device, &sdi1_data);
#endif

	return 0;
}

fs_initcall(mop500_sdi_init);
