/*
 * Copyright (C) 2009 ST Ericsson.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

/* AB8500 USB macros
 */
#define AB8500_REV_10 0x10
#define AB8500_REV_11 0x11
#define AB8500_USB_HOST 0x68
#define AB8500_ID_WAKEUP_RISING 90
#define AB8500_ID_WAKEUP_FALLING 96
#define AB8500_VBUS_RISING 15
#define AB8500_VBUS_FALLING 14
#define AB8500_IT_MASK20_MASK 0xFB
#define AB8500_IT_MASK21_MASK 0xFE
#define AB8500_IT_MASK2_MASK 0x3F
#define AB8500_SRC_INT_USB_DEVICE 0x80
#define AB8500_SRC_INT_USB_HOST 0x04
#define AB8500_VBUS_ENABLE 0x1
#define AB8500_VBUS_DISABLE 0x0
#define AB8500_USB_HOST_ENABLE 0x1
#define AB8500_USB_HOST_DISABLE 0x0
#define AB8500_USB_DEVICE_ENABLE 0x2
#define AB8500_USB_DEVICE_DISABLE 0x0
#define AB8500_MAIN_WATCHDOG_ENABLE 0x1
#define AB8500_MAIN_WATCHDOG_KICK 0x2
#define AB8500_MAIN_WATCHDOG_DISABLE 0x0

/* USB Macros
 */
#define WATCHDOG_DELAY 10
#define USB_ENABLE 1
#define USB_DISABLE 0

/* Specific functions for USB Phy enable and disable
 */
int musb_phy_en(u8 mode);
int musb_force_detect(u8 mode);
void usb_kick_watchdog(void);
void usb_host_insert_handler(void);
void usb_host_remove_handler(void);
void usb_host_phy_en(int);
void usb_device_insert_handler(void);
void usb_device_remove_handler(void);
void usb_device_phy_en(int);

