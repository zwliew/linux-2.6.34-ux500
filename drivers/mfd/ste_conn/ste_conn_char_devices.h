/*
 * drivers/mfd/ste_conn/ste_conn_char_devices.h
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Authors:
 * Par-Gunnar Hjalmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Henrik Possung (henrik.possung@stericsson.com) for ST-Ericsson.
 * Josef Kindberg (josef.kindberg@stericsson.com) for ST-Ericsson.
 * Dariusz Szymszak (dariusz.xd.szymczak@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson.
 * License terms:  GNU General Public License (GPL), version 2
 *
 * Linux Bluetooth HCI H:4 Driver for ST-Ericsson connectivity controller.
 */

#ifndef _STE_CONN_CHAR_DEVICES_H_
#define _STE_CONN_CHAR_DEVICES_H_

/**
 * ste_conn_char_devices_init() - Initialize char device module.
 * @char_dev_usage: Bitmask indicating what char devs to enable.
 * @dev:            Parent device for the driver.
 *
 * The ste_conn_char_devices_init() function initialize the char device module.
 */
extern void ste_conn_char_devices_init(int char_dev_usage, struct device *dev);

/**
 * ste_conn_char_devices_exit() - Release the char device module.
 *
 * The ste_conn_char_devices_init() function releases
 * the char device module.
 */
extern void ste_conn_char_devices_exit(void);

#endif /* _STE_CONN_CHAR_DEVICES_H_ */
