/*
 * file ste_conn_cpd.h
 *
 * Copyright (C) ST-Ericsson AB 2010
 *
 * Linux Bluetooth HCI H:4 Driver for ST-Ericsson connectivity controller.
 * License terms: GNU General Public License (GPL), version 2
 *
 * Authors:
 * Pär-Gunnar Hjälmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Henrik Possung (henrik.possung@stericsson.com) for ST-Ericsson.
 * Josef Kindberg (josef.kindberg@stericsson.com) for ST-Ericsson.
 * Dariusz Szymszak (dariusz.xd.szymczak@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson.
 */

#ifndef _STE_CONN_CPD_H_
#define _STE_CONN_CPD_H_

#include <linux/skbuff.h>
#include <linux/device.h>

/**
  * struct ste_conn_cpd_hci_logger_config - Configures the HCI logger.
  * @bt_cmd_enable: Enable BT cmd logging.
  * @bt_acl_enable: Enable BT acl logging.
  * @bt_evt_enable: Enable BT evt logging.
  * @gnss_enable: Enable GNSS logging.
  * @fm_radio_enable: Enable FM radio logging.
  * @bt_audio_enable: Enable BT audio cmd logging.
  * @fm_radio_audio_enable: Enable FM radio audio cmd logging.
  *
  * Set using ste_conn_write on STE_CONN_H4_CHANNEL_HCI_LOGGER H4 channel.
  */
struct ste_conn_cpd_hci_logger_config {
  bool bt_cmd_enable;
  bool bt_acl_enable;
  bool bt_evt_enable;
  bool gnss_enable;
  bool fm_radio_enable;
  bool bt_audio_enable;
  bool fm_radio_audio_enable;
};

/**
 * ste_conn_cpd_init() - Initialize module.
 * @char_dev_usage: Bitmask indicating what char devs to enable.
 * @dev:            Parent device this device is connected to.
 *
 * The ste_conn_cpd_init() function initialize the CPD module at system start-up.
 *
 * Returns:
 *   0 if there is no error.
 *   -EBUSY if CPD has already been initiated.
 *   -ENOMEM if allocation fails or work queue can't be created.
 */
extern int ste_conn_cpd_init(int char_dev_usage, struct device *dev);

/**
 * ste_conn_cpd_exit() - Release module.
 *
 * The ste_conn_cpd_exit() function remove CPD module at system shut-down.
 */
extern void ste_conn_cpd_exit(void);


/**
 * ste_conn_cpd_hw_registered() - Informs that driver is now connected to HW transport.
 *
 * The ste_conn_cpd_hw_registered() is called to inform that the driver is
 * connected to the HW transport.
 */
extern void ste_conn_cpd_hw_registered(void);

/**
 * ste_conn_cpd_hw_deregistered() - Informs that driver is disconnected from HW transport.
 *
 * The ste_conn_cpd_hw_deregistered() is called to inform that the driver is
 * disconnected from the HW transport.
 */
extern void ste_conn_cpd_hw_deregistered(void);

/**
 * ste_conn_cpd_data_received() - Data received from connectivity controller.
 * @skb: Data packet
 *
 * The ste_conn_cpd_data_received() function check which channel
 * the data was received on and send to the right user.
 */
extern void ste_conn_cpd_data_received(struct sk_buff *skb);

#endif /* _STE_CONN_CPD_H_ */
