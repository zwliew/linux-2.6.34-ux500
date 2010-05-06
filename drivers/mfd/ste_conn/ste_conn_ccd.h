/*
 * drivers/mfd/ste_conn/ste_conn_ccd.h
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

#ifndef _STE_CONN_CCD_H_
#define _STE_CONN_CCD_H_

#include <linux/skbuff.h>

#define BT_BDADDR_SIZE				6

struct ste_conn_ccd_h4_channels {
	int	bt_cmd_channel;
	int	bt_acl_channel;
	int	bt_evt_channel;
	int	gnss_channel;
	int	fm_radio_channel;
	int	debug_channel;
	int	ste_tools_channel;
	int	hci_logger_channel;
	int	us_ctrl_channel;
	int	core_channel;
};

struct ste_conn_ccd_driver_data {
	int	next_free_minor;
};

/* module_param declared in ste_conn_ccd.c */
extern uint8_t ste_conn_bd_address[BT_BDADDR_SIZE];
extern int ste_conn_default_manufacturer;
extern int ste_conn_default_hci_revision;
extern int ste_conn_default_sub_version;

/**
 * ste_conn_ccd_open() - Open the ste_conn CCD for data transfers.
 *
 * The ste_conn_ccd_open() function opens the ste_conn CCD for data transfers.
 *
 * Returns:
 *   0 if there is no error,
 *   -EACCES if write to transport failed,
 *   -EIO if transport has not been selected or chip did not answer to commands.
 */
extern int ste_conn_ccd_open(void);

/**
 * ste_conn_ccd_close() - Close the ste_conn CCD for data transfers.
 *
 * The ste_conn_ccd_close() function closes the ste_conn CCD for data transfers.
 */
extern void ste_conn_ccd_close(void);

/**
 * ste_conn_ccd_set_chip_power() - Enable or disable the connectivity controller.
 * @chip_on: true if chip shall be enabled.
 *
 * The ste_conn_ccd_set_chip_power() function enable or disable the connectivity controller.
 */
extern void ste_conn_ccd_set_chip_power(bool chip_on);

/**
 * ste_conn_ccd_write() - Transmit data packet to connectivity controller.
 * @skb: data packet.
 *
 * The ste_conn_ccd_write() function transmit data packet to connectivity controller.
 *
 * Returns:
 *   0 if there is no error.
 *   -EPERM if CCD has not been opened.
 *   Error codes from uart_send_skb_to_chip.
 */
extern int ste_conn_ccd_write(struct sk_buff *skb);

#endif /* _STE_CONN_CCD_H_ */
