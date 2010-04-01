/*
 * file ste_conn_devices.h
 *
 * Copyright (C) ST-Ericsson AB 2010
 *
 * Board specific device support for the Linux Bluetooth HCI H4 Driver
 * for ST-Ericsson connectivity controller.
 * License terms: GNU General Public License (GPL), version 2
 *
 * Authors:
 * Pär-Gunnar Hjälmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Henrik Possung (henrik.possung@stericsson.com) for ST-Ericsson.
 * Josef Kindberg (josef.kindberg@stericsson.com) for ST-Ericsson.
 * Dariusz Szymszak (dariusz.xd.szymczak@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson.
 */

#ifndef _STE_CONN_DEVICES_H_
#define _STE_CONN_DEVICES_H_

#include <linux/types.h>
#include <linux/skbuff.h>

#define STE_CONN_MAX_NAME_SIZE 30

/** STE_CONN_DEVICES_BT_CMD - Bluetooth HCI H4 channel for Bluetooth commands in the ST-Ericsson connectivity controller.
 */
#define STE_CONN_DEVICES_BT_CMD		"ste_conn_bt_cmd\0"

/** STE_CONN_DEVICES_BT_ACL - Bluetooth HCI H4 channel for Bluetooth ACL data in the ST-Ericsson connectivity controller.
 */
#define STE_CONN_DEVICES_BT_ACL		"ste_conn_bt_acl\0"

/** STE_CONN_DEVICES_BT_EVT - Bluetooth HCI H4 channel for Bluetooth events in the ST-Ericsson connectivity controller.
 */
#define STE_CONN_DEVICES_BT_EVT		"ste_conn_bt_evt\0"

/** STE_CONN_DEVICES_FM_RADIO - Bluetooth HCI H4 channel for FM radio in the ST-Ericsson connectivity controller.
 */
#define STE_CONN_DEVICES_FM_RADIO	"ste_conn_fm_radio\0"

/** STE_CONN_DEVICES_GNSS - Bluetooth HCI H4 channel for GNSS in the ST-Ericsson connectivity controller.
 */
#define STE_CONN_DEVICES_GNSS		"ste_conn_gnss\0"

/** STE_CONN_DEVICES_DEBUG - Bluetooth HCI H4 channel for internal debug data in the ST-Ericsson connectivity controller.
 */
#define STE_CONN_DEVICES_DEBUG		"ste_conn_debug\0"

/** STE_CONN_DEVICES_STE_TOOLS - Bluetooth HCI H4 channel for development tools data in the ST-Ericsson connectivity controller.
 */
#define STE_CONN_DEVICES_STE_TOOLS	"ste_conn_ste_tools\0"

/** STE_CONN_DEVICES_HCI_LOGGER - Bluetooth HCI H4 channel for logging all transmitted H4 packets (on all channels).
 */
#define STE_CONN_DEVICES_HCI_LOGGER	"ste_conn_hci_logger\0"

/** STE_CONN_DEVICES_US_CTRL - Bluetooth HCI H:4 channel
 * for user space initialization and control of the ST-Ericsson connectivity controller.
 */
#define STE_CONN_DEVICES_US_CTRL	"ste_conn_us_ctrl\0"

/** STE_CONN_DEVICES_BT_AUDIO - Bluetooth HCI H:4 channel
 * for Bluetooth audio configuration related commands in the ST-Ericsson connectivity controller.
 */
#define STE_CONN_DEVICES_BT_AUDIO	"ste_conn_bt_audio\0"

/** STE_CONN_DEVICES_FM_RADIO_AUDIO - HCI H:4 channel
 * for FM audio configuration related commands in the ST-Ericsson connectivity controller.
 */
#define STE_CONN_DEVICES_FM_RADIO_AUDIO	"ste_conn_fm_audio\0"

/** STE_CONN_DEVICES_CORE- HCI H:4 channel
 * for core configuration related commands in the ST-Ericsson connectivity controller.
 */
#define STE_CONN_DEVICES_CORE	"ste_conn_core\0"

/** STE_CONN_NO_CHAR_DEV - Module parameter for
 * no character devices to be initiated.
 */
#define STE_CONN_NO_CHAR_DEV			0x00000000

/** STE_CONN_CHAR_DEV_BT - Module parameter for character device Bluetooth.
 */
#define STE_CONN_CHAR_DEV_BT			0x00000001

/** STE_CONN_CHAR_DEV_FM_RADIO - Module parameter for character device FM radio.
 */
#define STE_CONN_CHAR_DEV_FM_RADIO		0x00000002

/** STE_CONN_CHAR_DEV_GNSS - Module parameter for character device GNSS.
 */
#define STE_CONN_CHAR_DEV_GNSS			0x00000004

/** STE_CONN_CHAR_DEV_DEBUG - Module parameter for character device Debug.
 */
#define STE_CONN_CHAR_DEV_DEBUG			0x00000008

/** STE_CONN_CHAR_DEV_STE_TOOLS - Module parameter for character device STE tools.
 */
#define STE_CONN_CHAR_DEV_STE_TOOLS		0x00000010

/** STE_CONN_CHAR_DEV_CCD_DEBUG - Module parameter for character device CCD debug.
 */
#define STE_CONN_CHAR_DEV_CCD_DEBUG		0x00000020

/** STE_CONN_CHAR_DEV_HCI_LOGGER - Module parameter for character device HCI logger.
 */
#define STE_CONN_CHAR_DEV_HCI_LOGGER	0x00000040

/** STE_CONN_CHAR_DEV_US_CTRL - Module parameter for
 * character device user space control.
 */
#define STE_CONN_CHAR_DEV_US_CTRL		0x00000080

/** STE_CONN_CHAR_TEST_DEV - Module parameter for character device test device.
 */
#define STE_CONN_CHAR_TEST_DEV			0x00000100

/** STE_CONN_CHAR_DEV_BT_AUDIO - Module parameter for character device BT CMD audio application.
 */
#define STE_CONN_CHAR_DEV_BT_AUDIO		0x00000200

/** STE_CONN_CHAR_DEV_FM_RADIO_AUDIO - Module parameter for character device FM audio application.
 */
#define STE_CONN_CHAR_DEV_FM_RADIO_AUDIO			0x00000400

/** STE_CONN_CHAR_DEV_CORE - Module parameter for character device core.
 */
#define STE_CONN_CHAR_DEV_CORE			0x00000800

/** STE_CONN_CHAR_TEST_DEV - Module parameter for all character devices to be initiated.
 */
#define STE_CONN_ALL_CHAR_DEVS			0xFFFFFFFF

/**
 * ste_conn_devices_get_h4_channel() - Get H4 channel by name.
 * @name:       Name of connectivity user.
 * @h4_channel: HCI H4 channel to register to.
 *
 * Returns:
 *   0 if there is no error.
 *   -ENXIO if channel was not found.
 */
extern int ste_conn_devices_get_h4_channel(char *name, int *h4_channel);

/**
 * ste_conn_devices_enable_chip() - Enable the controller.
 */
extern void ste_conn_devices_enable_chip(void);

/**
 * ste_conn_devices_disable_chip() - Disable the controller.
 */
extern void ste_conn_devices_disable_chip(void);

/**
 * ste_conn_devices_set_hci_revision() - Stores HCI revision info for the connected connectivity controller.
 * @hci_version:    HCI version from the controller.
 * @hci_revision:   HCI revision from the controller.
 * @lmp_version:    LMP version from the controller.
 * @lmp_subversion: LMP subversion from the controller.
 * @manufacturer:   Manufacturer ID from the controller.
 *
 * See Bluetooth specification and white paper for used controller for details about parameters.
 */
extern void ste_conn_devices_set_hci_revision(uint8_t hci_version,
					uint16_t hci_revision,
					uint8_t lmp_version,
					uint8_t lmp_subversion,
					uint16_t manufacturer);

/**
 * ste_conn_devices_get_reset_cmd() - Get HCI reset command to use based on connected connectivity controller.
 * @op_lsb: LSB of HCI opcode in generated packet. NULL if not needed.
 * @op_msb: MSB of HCI opcode in generated packet. NULL if not needed.
 *
 * This command does not add the H4 channel header in front of the message.
 *
 * Returns:
 *   NULL      if no command shall be sent,
 *   sk_buffer with command otherwise.
 */
extern struct sk_buff *ste_conn_devices_get_reset_cmd(uint8_t *op_lsb, uint8_t *op_msb);

/**
 * ste_conn_devices_get_power_switch_off_cmd() - Get HCI power switch off command to use based on connected connectivity controller.
 * @op_lsb: LSB of HCI opcode in generated packet. NULL if not needed.
 * @op_msb: MSB of HCI opcode in generated packet. NULL if not needed.
 *
 * This command does not add the H4 channel header in front of the message.
 *
 * Returns:
 *   NULL      if no command shall be sent,
 *   sk_buffer with command otherwise.
 */
extern struct sk_buff *ste_conn_devices_get_power_switch_off_cmd(uint8_t *op_lsb, uint8_t *op_msb);

/**
 * ste_conn_devices_get_bt_enable_cmd() - Get HCI BT enable command to use based on connected connectivity controller.
 * @op_lsb: LSB of HCI opcode in generated packet. NULL if not needed.
 * @op_msb: MSB of HCI opcode in generated packet. NULL if not needed.
 * @bt_enable: true if Bluetooth IP shall be enabled, false otherwise.
 *
 * This command does not add the H4 channel header in front of the message.
 *
 * Returns:
 *   NULL      if no command shall be sent,
 *   sk_buffer with command otherwise.
 */
extern struct sk_buff *ste_conn_devices_get_bt_enable_cmd(uint8_t *op_lsb, uint8_t *op_msb, bool bt_enable);

/**
 * ste_conn_devices_init() - Initialize the board config.
 *
 * Returns:
 *   0 if there is no error.
 *   Error codes from gpio_request and gpio_direction_output.
 */
extern int ste_conn_devices_init(void);

/**
 * ste_conn_devices_exit() - Exit function for the board config.
 */
extern void ste_conn_devices_exit(void);

#endif /* _STE_CONN_DEVICES_H_ */
