/*
 * Copyright (C) 2009 ST Ericsson.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/usb/musb.h>
#include <mach/ab8500.h>

#ifdef CONFIG_REGULATOR
#include <linux/regulator/consumer.h>
struct regulator *musb_vape_supply, *musb_vbus_supply;
#endif

int boot_time_flag;
void usb_kick_watchdog(void);
void usb_host_insert_handler(void);
void usb_host_phy_en(int);
void usb_device_insert_handler(void);
void usb_device_remove_handler(void);
void usb_device_phy_en(int);

/**
 * usb_kick_watchdog() - Kick the watch dog timer
 *
 * This function used to Kick the watch dog timer
 */
void usb_kick_watchdog(void)
{
	ab8500_write(AB8500_SYS_CTRL2_BLOCK,
			AB8500_MAIN_WDOG_CTRL_REG, 0x1);
	ab8500_write(AB8500_SYS_CTRL2_BLOCK,
			AB8500_MAIN_WDOG_CTRL_REG, 0x3);
	mdelay(10);
	ab8500_write(AB8500_SYS_CTRL2_BLOCK,
			AB8500_MAIN_WDOG_CTRL_REG, 0x0);
	mdelay(10);
}
/**
 * usb_host_phy_en() - for enabling the 5V to usb host
 * @enable: to enabling the Phy for host.
 *
 * This function used to set the voltage for USB host mode
 */
void usb_host_phy_en(int enable)
{
	if (enable == 1) {
		if (boot_time_flag) {
			ab8500_write(AB8500_REGU_CTRL1,
					AB8500_REGU_VUSB_CTRL_REG, 0x1);
			ab8500_write(AB8500_USB, AB8500_USB_PHY_CTRL_REG, 0x1);
		} else {
		#ifdef CONFIG_REGULATOR
			regulator_enable(musb_vbus_supply);
			regulator_set_optimum_mode(musb_vape_supply, 1);
		#else
			ab8500_write(AB8500_REGU_CTRL1,
					AB8500_REGU_VUSB_CTRL_REG, 0x1);
		#endif
			ab8500_write(AB8500_USB,
					AB8500_USB_PHY_CTRL_REG, 0x1);
		}
	} else {
		if (boot_time_flag) {
			ab8500_write(AB8500_REGU_CTRL1,
					AB8500_REGU_VUSB_CTRL_REG, 0x0);
			ab8500_write(AB8500_USB,
					AB8500_USB_PHY_CTRL_REG, 0x0);
			boot_time_flag = 0;
		} else {
		#ifdef CONFIG_REGULATOR
			regulator_enable(musb_vbus_supply);
			regulator_set_optimum_mode(musb_vape_supply, 0);
		#else
			ab8500_write(AB8500_REGU_CTRL1,
					AB8500_REGU_VUSB_CTRL_REG, 0x0);
		#endif
			ab8500_write(AB8500_USB,
					AB8500_USB_PHY_CTRL_REG, 0x0);
		}
	}
}
/**
 * usb_host_insert_handler() - Detect the USB host cable insertion
 *
 * This function used to detect the USB host cable insertion.
 */
void usb_host_insert_handler(void)
{
	usb_host_phy_en(1);
}
/**
 * usb_host_remove_handler() - Removed the USB host cable
 *
 * This function used to detect the USB host cable removed.
 */
void usb_host_remove_handler(void)
{
	usb_host_phy_en(0);
}
/**
 * usb_device_phy_en() - for enabling the 5V to usb gadget
 * @enable: to enabling the Phy for device.
 *
 * This function used to set the voltage for USB gadget mode.
 */
void usb_device_phy_en(int enable)
{
	if (enable == 1) {
		usb_kick_watchdog();
		if (boot_time_flag) {
			ab8500_write(AB8500_REGU_CTRL1,
					AB8500_REGU_VUSB_CTRL_REG, 0x1);
			ab8500_write(AB8500_USB, AB8500_USB_PHY_CTRL_REG, 0x2);
		} else {
		#ifdef CONFIG_REGULATOR
			regulator_enable(musb_vbus_supply);
			regulator_set_optimum_mode(musb_vape_supply, 1);
		#else
			ab8500_write(AB8500_REGU_CTRL1,
					AB8500_REGU_VUSB_CTRL_REG, 0x1);
		#endif
			ab8500_write(AB8500_USB, AB8500_USB_PHY_CTRL_REG, 0x2);
		}
	} else {
		if (boot_time_flag) {
			ab8500_write(AB8500_REGU_CTRL1,
					AB8500_REGU_VUSB_CTRL_REG, 0x0);
			ab8500_write(AB8500_USB,
					AB8500_USB_PHY_CTRL_REG, 0x0);
			boot_time_flag = 0;
		} else {
		#ifdef CONFIG_REGULATOR
			regulator_disable(musb_vbus_supply);
			regulator_set_optimum_mode(musb_vape_supply, 0);
		#else
			ab8500_write(AB8500_REGU_CTRL1,
					AB8500_REGU_VUSB_CTRL_REG, 0);
		#endif
			ab8500_write(AB8500_USB, AB8500_USB_PHY_CTRL_REG, 0);
		}
	}
}
/**
 * usb_device_insert_handler() - for enabling the 5V to usb device
 *
 * This function used to set the voltage for USB device mode
 */
void usb_device_insert_handler(void)
{
	usb_device_phy_en(1);
}
/**
 * usb_device_remove_handler() - remove the 5V to usb device
 *
 * This function used to remove the voltage for USB device mode.
 */
void usb_device_remove_handler(void)
{
	usb_device_phy_en(0);
}
/**
 * musb_phy_en : register USB callback handlers for ab8500
 * @mode: value for mode.
 *
 * This function is used to register USB callback handlers for ab8500.
 */
int musb_phy_en(u8 mode)
{
	u8 ab8500_rev;
	u8 ret;

#ifdef CONFIG_REGULATOR
	musb_vape_supply = regulator_get(NULL, "v-ape");
	if (IS_ERR(musb_vape_supply)) {
		ret = PTR_ERR(musb_vape_supply);
		printk(KERN_WARNING "musb_u8500 : failed to get v-ape supply\n");
		return -1;
	}
	regulator_enable(musb_vape_supply);
	musb_vbus_supply = regulator_get(NULL, "v-bus");
	if (IS_ERR(musb_vbus_supply)) {
		ret = PTR_ERR(musb_vbus_supply);
		printk(KERN_WARNING "musb_u8500 : failed to get v-bus supply\n");
		return -1;
	}
#endif
	ab8500_rev = ab8500_read(AB8500_MISC, AB8500_REV_REG);
	if ((ab8500_rev == AB8500_REV_10) || (ab8500_rev == AB8500_REV_11)) {
		if (mode == MUSB_HOST) {
			ab8500_write
				(AB8500_INTERRUPT,
					AB8500_IT_MASK20_REG, AB8500_IT_MASK20_MASK);
			ab8500_set_callback_handler
				(AB8500_ID_WAKEUP_RISING,
					usb_host_insert_handler, NULL);
			ab8500_write
				(AB8500_INTERRUPT, AB8500_IT_MASK21_REG,
					AB8500_IT_MASK21_MASK);
			ab8500_set_callback_handler
				(AB8500_ID_WAKEUP_FALLING,
					usb_host_remove_handler, NULL);
			usb_kick_watchdog();
		} else if (mode == MUSB_PERIPHERAL) {
			ab8500_write
				(AB8500_INTERRUPT, AB8500_IT_MASK2_REG,
						AB8500_IT_MASK2_MASK);
			ab8500_set_callback_handler
				(AB8500_VBUS_RISING, usb_device_insert_handler, NULL);
			ab8500_set_callback_handler
				(AB8500_VBUS_FALLING, usb_device_remove_handler, NULL);
		}
	}
	return 0;
}
/**
 * musb_force_detect : detect the USB cable during boot time.
 * @mode: value for mode.
 *
 * This function is used to detect the USB cable during boot time.
 */
int musb_force_detect(u8 mode)
{
	u8 ab8500_rev;
	int usb_status = 0;
	ab8500_rev = ab8500_read(AB8500_MISC, AB8500_REV_REG);
	if ((ab8500_rev == AB8500_REV_10) || (ab8500_rev == AB8500_REV_11)) {
		if (mode == MUSB_HOST) {
			usb_status = ab8500_read
				(AB8500_INTERRUPT, AB8500_IT_SOURCE20_REG);
			if (usb_status & AB8500_SRC_INT_USB_HOST) {
				boot_time_flag = 1;
				usb_host_phy_en(1);
			}
		} else if (mode == MUSB_PERIPHERAL) {
			usb_status = ab8500_read
				(AB8500_INTERRUPT, AB8500_IT_SOURCE2_REG);
			if (usb_status & AB8500_SRC_INT_USB_DEVICE) {
				boot_time_flag = 1;
				usb_device_phy_en(1);
			}
		}
	}
	return 0;
}
