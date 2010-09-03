/*
 * Copyright (C) 2009 STMicroelectronics
 * Copyright (C) 2009 ST-Ericsson SA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/** @file  stm_musb.c
  * @brief This file contains the USB controller and Phy initialization
  * with default as interrupt mode was implemented
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include "musb_core.h"
#include "stm_musb.h"
#include <mach/musb_db8500.h>

static u8 ulpi_read_register(struct musb *musb, u8 address);
static u8 ulpi_write_register(struct musb *musb, u8 address, u8 data);
/* callback argument for AB8500 callback functions */
static struct musb *musb_status;
static spinlock_t musb_ulpi_spinlock;
static unsigned musb_power;

#if 0
static void
ab8500_bm_ulpi_test(void)
{
	u8 x;

	x = ulpi_read_register(musb_status, ULPI_VIDLO);
	if (x != 0x83)
		printk(KERN_ALERT "Unexpected USB PHY VID-LO: 0x%02x\n", x);

	x = ulpi_read_register(musb_status, ULPI_VIDHI);
	if (x != 0x04)
		printk(KERN_ALERT "Unexpected USB PHY VID-HI: 0x%02x\n", x);

	x = ulpi_read_register(musb_status, ULPI_PIDLO);
	if (x != 0x00)
		printk(KERN_ALERT "Unexpected USB PHY PID-LO: 0x%02x\n", x);

	x = ulpi_read_register(musb_status, ULPI_PIDHI);
	if (x != 0x45)
		printk(KERN_ALERT "Unexpected USB PHY PID-HI: 0x%02x\n", x);

	ulpi_write_register(musb_status, ULPI_SCRATCH, 0xAA);

	x = ulpi_read_register(musb_status, ULPI_SCRATCH);
	if (x != 0xAA)
		printk(KERN_ALERT "Unexpected ULPI "
		    "read-back value: 0x%02x\n", x);

	ulpi_write_register(musb_status, ULPI_SCRATCH, 0x55);

	x = ulpi_read_register(musb_status, ULPI_SCRATCH);
	if (x != 0x55)
		printk(KERN_ALERT "Unexpected ULPI "
		    "read-back value: 0x%02x\n", x);
	else
		printk(KERN_INFO "USB: ULPI read-back value OK\n");
}
#endif

void
ab8500_bm_usb_state_changed_wrapper(u8 bm_usb_state)
{
	if ((bm_usb_state == AB8500_BM_USB_STATE_RESET_HS) ||
	    (bm_usb_state == AB8500_BM_USB_STATE_RESET_FS)) {
		musb_power = 0;
#if 0
		/* for debugging purpose only */
		ab8500_bm_ulpi_test();
#endif
	}
#if 0
	printk(KERN_INFO "Please implement ab8500_bm_usb_state_changed"
	    "(%d AB8500_BM_USB_STATE_XXX, %d mA)\n",
	    bm_usb_state, musb_power);
#endif

	ab8500_bm_usb_state_changed(bm_usb_state, musb_power);
}

void
ab8500_bm_ulpi_set_char_speed_mode(u8 mode)
{
	if (musb_status == NULL) {
		printk(KERN_ALERT "musb_status is NULL. "
		    "Cannot write to ULPI.\n");
		return;
	}

	mode &= 3;
	mode <<= 5;
	mode |= ulpi_read_register(musb_status, ULPI_ULINKSTAT) &
	    ~ULPI_ULINKSTAT_CHARSPEED_MODE;

	printk(KERN_INFO "Writing 0x%02x to USB char-speed mode bits\n", mode);

	ulpi_write_register(musb_status, ULPI_ULINKSTAT, mode);
}

void
ab8500_bm_ulpi_set_char_suspend_mode(u8 suspend)
{
	if (musb_status == NULL) {
		printk(KERN_ALERT "musb_status is NULL. "
		    "Cannot write to ULPI.\n");
		return;
	}

	suspend &= 1;
	suspend <<= 7;
	suspend |= ulpi_read_register(musb_status, ULPI_ULINKSTAT) &
	    ~ULPI_ULINKSTAT_SUSPEND_MODE;

	printk(KERN_INFO "Writing 0x%02x to USB suspend mode bits\n", suspend);

	ulpi_write_register(musb_status, ULPI_ULINKSTAT, suspend);
}

/* Sys interfaces */
static struct kobject *usbstatus_kobj;
static ssize_t usb_cable_status
		(struct kobject *kobj, struct attribute *attr, char *buf)
{
	u8 is_active = 0;

	if (strcmp(attr->name, "cable_connect") == 0) {
		is_active = musb_status->is_active;
		sprintf(buf, "%d\n", is_active);
	}
	return strlen(buf);
}

static struct attribute usb_cable_connect_attribute = \
			{.name = "cable_connect", .mode = S_IRUGO};
static struct attribute *usb_status[] = {
	&usb_cable_connect_attribute,
	NULL
};

struct sysfs_ops usb_sysfs_ops = {
	.show  = usb_cable_status,
};

static struct kobj_type ktype_usbstatus = {
	.sysfs_ops = &usb_sysfs_ops,
	.default_attrs = usb_status,
};

/*
 * A structure was declared as global for timer in USB host mode
 */
static struct timer_list notify_timer;

/**
 * ulpi_read_register() - Read the usb register from address writing into ULPI
 * @musb: struct musb pointer.
 * @address: address for reading from ULPI register of USB
 *
 * This function read the value from the specific address in USB host mode.
 */
static u8 ulpi_read_register(struct musb *musb, u8 address)
{
	void __iomem *mbase = musb->mregs;
	unsigned long flags;
	int count = 200;
	u8 val;

	spin_lock_irqsave(&musb_ulpi_spinlock, flags);

	/* set ULPI register address */
	musb_writeb(mbase, OTG_UREGADDR, address);

	/* request a read access */
	val = musb_readb(mbase, OTG_UREGCTRL);
	val |= OTG_UREGCTRL_URW;
	musb_writeb(mbase, OTG_UREGCTRL, val);

	/* perform access */
	val = musb_readb(mbase, OTG_UREGCTRL);
	val |= OTG_UREGCTRL_REGREQ;
	musb_writeb(mbase, OTG_UREGCTRL, val);

	/* wait for completion with a time-out */
	do {
		udelay(10);
		val = musb_readb(mbase, OTG_UREGCTRL);
		count--;
	} while (!(val & OTG_UREGCTRL_REGCMP) && (count > 0));

	/* check for time-out */
	if (!(val & OTG_UREGCTRL_REGCMP) && (count == 0)) {
		spin_unlock_irqrestore(&musb_ulpi_spinlock, flags);
		printk(KERN_ALERT "U8500 USB : ULPI read timed out\n");
		return 0;
	}

	/* acknowledge completion */
	val &= ~OTG_UREGCTRL_REGCMP;
	musb_writeb(mbase, OTG_UREGCTRL, val);

	/* get data */
	val = musb_readb(mbase, OTG_UREGDATA);
	spin_unlock_irqrestore(&musb_ulpi_spinlock, flags);

	return val;
}

/**
 * ulpi_write_register() - Write to a usb phy's ULPI register
 * using the Mentor ULPI wrapper functionality
 * @musb: struct musb pointer.
 * @address: address of ULPI register
 * @data: data for ULPI register
 * This function writes the value given by data to the specific address
 */
static u8 ulpi_write_register(struct musb *musb, u8 address, u8 data)
{
	void __iomem *mbase = musb->mregs;
	unsigned long flags;
	int count = 200;
	u8 val;

	spin_lock_irqsave(&musb_ulpi_spinlock, flags);

	/* First write to ULPI wrapper registers */
	/* set ULPI register address */
	musb_writeb(mbase, OTG_UREGADDR, address);

	/* request a write access */
	val = musb_readb(mbase, OTG_UREGCTRL);
	val &= ~OTG_UREGCTRL_URW;
	musb_writeb(mbase, OTG_UREGCTRL, val);

	/* Write data to ULPI wrapper data register */
	musb_writeb(mbase, OTG_UREGDATA, data);

	/* perform access  */
	val = musb_readb(mbase, OTG_UREGCTRL);
	val |= OTG_UREGCTRL_REGREQ;
	musb_writeb(mbase, OTG_UREGCTRL, val);

	/* wait for completion with a time-out */
	do {
		udelay(10);
		val = musb_readb(mbase, OTG_UREGCTRL);
		count--;
	} while (!(val & OTG_UREGCTRL_REGCMP) && (count > 0));

	/* check for time-out */
	if (!(val & OTG_UREGCTRL_REGCMP) && (count == 0)) {
		spin_unlock_irqrestore(&musb_ulpi_spinlock, flags);
		printk(KERN_ALERT "U8500 USB : ULPI write timed out\n");
		return 0;
	}

	/* acknowledge completion */
	val &= ~OTG_UREGCTRL_REGCMP;
	musb_writeb(mbase, OTG_UREGCTRL, val);

	spin_unlock_irqrestore(&musb_ulpi_spinlock, flags);

	return 0;

}

/**
 * musb_stm_hs_otg_init() - Initialize the USB for paltform specific.
 * @musb: struct musb pointer.
 *
 * This function initialize the USB with the given musb structure information.
 */
int __init musb_stm_hs_otg_init(struct musb *musb)
{
	u8 val;
	int status;

	if (musb->clock)
		clk_enable(musb->clock);
	status = stm_gpio_altfuncenable(GPIO_ALT_USB_OTG);
	if (status) {
		DBG(1, "U8500 USB : Problem in enabling alternate function\n");
		return -EBUSY;
	}

	/* enable ULPI interface */
	val = musb_readb(musb->mregs, OTG_TOPCTRL);
	val |= OTG_TOPCTRL_MODE_ULPI;
	musb_writeb(musb->mregs, OTG_TOPCTRL, val);

	/* do soft reset */
	val = musb_readb(musb->mregs, 0x7F);
	val |= 0x2;
	musb_writeb(musb->mregs, 0x7F, val);

	return 0;
}
/**
 * musb_stm_fs_init() - Initialize the file system for the USB.
 * @musb: struct musb pointer.
 *
 * This function initialize the file system of USB.
 */
int __init musb_stm_fs_init(struct musb *musb)
{
	return 0;
}
/**
 * musb_platform_enable() - Enable the USB.
 * @musb: struct musb pointer.
 *
 * This function enables the USB.
 */
void musb_platform_enable(struct musb *musb)
{
}
/**
 * musb_platform_disable() - Disable the USB.
 * @musb: struct musb pointer.
 *
 * This function disables the USB.
 */
void musb_platform_disable(struct musb *musb)
{
}

/**
 * musb_platform_try_idle() - Check the USB state active or not.
 * @musb: struct musb pointer.
 * @timeout: set the timeout to keep the host in idle mode.
 *
 * This function keeps the USB host in idle state based on the musb inforamtion.
 */
void musb_platform_try_idle(struct musb *musb, unsigned long timeout)
{
	if (musb->board_mode != MUSB_PERIPHERAL) {
		unsigned long default_timeout =
			jiffies + msecs_to_jiffies(1000);
		static unsigned long last_timer;

		if (timeout == 0)
			timeout = default_timeout;

		/* Never idle if active, or when VBUS
			 timeout is not set as host */
		if (musb->is_active || ((musb->a_wait_bcon == 0)
			&& (musb->xceiv.state == OTG_STATE_A_WAIT_BCON))) {
			DBG(4, "%s active, deleting timer\n",
				otg_state_string(musb));
			del_timer(&notify_timer);
			last_timer = jiffies;
			return;
		}

		if (time_after(last_timer, timeout)) {
			if (!timer_pending(&notify_timer))
				last_timer = timeout;
			else {
				DBG(4,
				"Longer idle timer already pending,ignoring\n");
				return;
			}
		}
		last_timer = timeout;

		DBG(4, "%s inactive, for idle timer for %lu ms\n",
		otg_state_string(musb),
		(unsigned long)jiffies_to_msecs(timeout - jiffies));
		mod_timer(&notify_timer, timeout);
	}
}

/**
 * set_vbus() - Set the Vbus for the USB.
 * @musb: struct musb pointer.
 * @is_on: set Vbus for USB or not.
 *
 * This function set the Vbus for USB.
 */
static void set_vbus(struct musb *musb, int is_on)
{
	u8 devctl;
	/* HDRC controls CPEN, but beware current surges during device
	 * connect. They can trigger transient overcurrent conditions
	 * that must be ignored.
	 */

	devctl = musb_readb(musb->mregs, MUSB_DEVCTL);

	if (is_on) {
		musb->is_active = 1;
		musb->xceiv.default_a = 1;
		musb->xceiv.state = OTG_STATE_A_WAIT_VRISE;
		devctl |= MUSB_DEVCTL_SESSION;

		MUSB_HST_MODE(musb);
	} else {
		musb->is_active = 0;

		/* NOTE:  we're skipping A_WAIT_VFALL -> A_IDLE and
		 * jumping right to B_IDLE...
		 */

		musb->xceiv.default_a = 0;
		musb->xceiv.state = OTG_STATE_B_IDLE;
		devctl &= ~MUSB_DEVCTL_SESSION;

		MUSB_DEV_MODE(musb);
	}
	musb_writeb(musb->mregs, MUSB_DEVCTL, devctl);

	DBG(1, "VBUS %s, devctl %02x "
		/* otg %3x conf %08x prcm %08x */ "\n",
		otg_state_string(musb),
		musb_readb(musb->mregs, MUSB_DEVCTL));
}
/**
 * set_power() - Set the power for the USB transceiver.
 * @x: struct usb_transceiver pointer.
 * @mA: set mA power for USB.
 *
 * This function set the power for the USB.
 */
static int set_power(struct otg_transceiver *x, unsigned mA)
{
	musb_power = mA;

	ab8500_bm_usb_state_changed_wrapper(AB8500_BM_USB_STATE_CONFIGURED);

	return 0;
}
/**
 * musb_platform_set_mode() - Set the mode for the USB driver.
 * @musb: struct musb pointer.
 * @musb_mode: usb mode.
 *
 * This function set the mode for the USB.
 */
int musb_platform_set_mode(struct musb *musb, u8 musb_mode)
{
	u8 devctl = musb_readb(musb->mregs, MUSB_DEVCTL);

	devctl |= MUSB_DEVCTL_SESSION;
	musb_writeb(musb->mregs, MUSB_DEVCTL, devctl);

	switch (musb_mode) {
	case MUSB_HOST:
		otg_set_host(&musb->xceiv, musb->xceiv.host);
		break;
	case MUSB_PERIPHERAL:
		otg_set_peripheral(&musb->xceiv, musb->xceiv.gadget);
		break;
	case MUSB_OTG:
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
/**
 * funct_host_notify_timer() - Initialize the timer for USB host driver.
 * @data: usb host data.
 *
 * This function runs the timer for the USB host mode.
 */
static void funct_host_notify_timer(unsigned long data)
{
	struct musb *musb = (struct musb *)data;
	uint8_t *reg_base = (uint8_t *)musb->mregs;
	uint8_t temp = 0;
	uint8_t devctl = 0;

	if (musb->xceiv.state != OTG_STATE_A_HOST) {
		temp = ulpi_read_register(musb, 0x13);
		if (temp & 0x08) {
			devctl = musb_readb(reg_base, MUSB_DEVCTL);
			DBG(1, "devctl = %x\n", devctl);
			musb_writeb(reg_base, MUSB_DEVCTL,
					devctl | MUSB_DEVCTL_SESSION);
		}
		mod_timer(&notify_timer, jiffies + msecs_to_jiffies(1000));
	} else
		del_timer(&notify_timer);
}
/**
 * musb_platform_init() - Initialize the platform USB driver.
 * @musb: struct musb pointer.
 *
 * This function initialize the USB controller and Phy.
 */
int __init musb_platform_init(struct musb *musb)
{
	int ret;

	ret = musb_stm_hs_otg_init(musb);
	if (ret < 0)
		return ret;
	if (is_host_enabled(musb))
		musb->board_set_vbus = set_vbus;
	if (is_peripheral_enabled(musb))
		musb->xceiv.set_power = set_power;

	ret = musb_phy_en(musb->board_mode);
	if (ret < 0)
		return ret;

	if (musb_status == NULL) {
		musb_status = musb;
		spin_lock_init(&musb_ulpi_spinlock);
	}

	/* Registering usb device  for sysfs */
	usbstatus_kobj = kzalloc(sizeof(struct kobject), GFP_KERNEL);

	if (usbstatus_kobj == NULL)
		ret = -ENOMEM;
	usbstatus_kobj->ktype = &ktype_usbstatus;
	kobject_init(usbstatus_kobj, usbstatus_kobj->ktype);

	ret = kobject_set_name(usbstatus_kobj, "usb_status");
	if (ret)
		kfree(usbstatus_kobj);

	ret = kobject_add(usbstatus_kobj, NULL, "usb_status");
	if (ret)
		kfree(usbstatus_kobj);

	if (musb->board_mode != MUSB_PERIPHERAL) {
		init_timer(&notify_timer);
		notify_timer.expires = jiffies + msecs_to_jiffies(1000);
		notify_timer.function = funct_host_notify_timer;
		notify_timer.data = (unsigned long)musb;
		add_timer(&notify_timer);
	}
	ret = musb_force_detect(musb->board_mode);
	if (ret < 0)
		return ret;
	return 0;
}
/**
 * musb_platform_exit() - unregister the platform USB driver.
 * @musb: struct musb pointer.
 *
 * This function unregisters the USB controller.
 */
int musb_platform_exit(struct musb *musb)
{
	int status = 0;
	musb->clock = 0;
	status = stm_gpio_altfuncdisable(GPIO_ALT_USB_OTG);
	if (status) {
		DBG(1, "U8500 USB : Problem in disabling alternate function\n");
		return -EBUSY;
	}
	if (musb->board_mode != MUSB_PERIPHERAL)
		del_timer_sync(&notify_timer);

	musb_status = NULL;

	return 0;
}
