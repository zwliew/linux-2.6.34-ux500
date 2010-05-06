/*
 * drivers/mfd/ste_conn/ste_conn_cpd.h
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Authors:
 * Par-Gunnar Hjalmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Henrik Possung (henrik.possung@stericsson.com) for ST-Ericsson.
 * Josef Kindberg (josef.kindberg@stericsson.com) for ST-Ericsson.
 * Dariusz Szymszak (dariusz.xd.szymczak@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson. * License terms:  GNU General Public License (GPL), version 2
 *
 * Linux Bluetooth HCI H:4 Driver for ST-Ericsson connectivity controller.
 */

#ifndef _STE_CONN_CPD_H_
#define _STE_CONN_CPD_H_

#include <linux/skbuff.h>
#include <linux/device.h>

/* Reserve 1 byte for the HCI H:4 header */
#define STE_CONN_SKB_RESERVE  1

/**
  * struct ste_conn_cpd_hci_logger_config - Configures the HCI logger.
  * @bt_cmd_enable: Enable BT command logging.
  * @bt_acl_enable: Enable BT ACL logging.
  * @bt_evt_enable: Enable BT event logging.
  * @gnss_enable: Enable GNSS logging.
  * @fm_radio_enable: Enable FM radio logging.
  * @bt_audio_enable: Enable BT audio command logging.
  * @fm_radio_audio_enable: Enable FM radio audio command logging.
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
 * struct ste_conn_cpd_chip_info - Chip info structure.
 * @manufacturer:	Chip manufacturer.
 * @hci_revision:	Chip revision, i.e. which chip is this.
 * @hci_sub_version:	Chip sub-version, i.e. which tape-out is this.
 *
 * Note that these values match the Bluetooth Assigned Numbers,
 * see http://www.bluetooth.org/
 */
struct ste_conn_cpd_chip_info {
	int manufacturer;
	int hci_revision;
	int hci_sub_version;
};

struct ste_conn_cpd_chip_dev;

/**
 * struct ste_conn_cpd_chip_callbacks - Callback functions registered by chip handler.
 * @chip_startup:		Called when chip is started up.
 * @chip_shutdown:		Called when chip is shut down.
 * @data_to_chip:		Called when data shall be transmitted to chip. Return true when CPD shall not send it to chip.
 * @data_from_chip:		Called when data shall be transmitted to user. Return true when packet is taken care of by chip handler.
 * @get_h4_channel:		Connects channel name with H:4 channel number.
 * @is_bt_audio_user:		Return true if current packet is for the BT audio user.
 * @is_fm_audio_user:		Return true if current packet is for the FM audio user.
 * @last_bt_user_removed:	Last BT channel user has been removed.
 * @last_fm_user_removed:	Last FM channel user has been removed.
 * @last_gnss_user_removed:	Last GNSS channel user has been removed.
 *
 * Note that some callbacks may be NULL. They must always be NULL checked before calling.
 */
struct ste_conn_cpd_chip_callbacks {
	int (*chip_startup)(struct ste_conn_cpd_chip_dev *dev);
	int (*chip_shutdown)(struct ste_conn_cpd_chip_dev *dev);
	bool (*data_to_chip)(struct ste_conn_cpd_chip_dev *dev, struct ste_conn_device *cpd_dev, struct sk_buff *skb);
	bool (*data_from_chip)(struct ste_conn_cpd_chip_dev *dev, struct ste_conn_device *cpd_dev, struct sk_buff *skb);
	int (*get_h4_channel)(char *name, int *h4_channel);
	bool (*is_bt_audio_user)(int h4_channel, const struct sk_buff * const skb);
	bool (*is_fm_audio_user)(int h4_channel, const struct sk_buff * const skb);
	void (*last_bt_user_removed)(struct ste_conn_cpd_chip_dev *dev);
	void (*last_fm_user_removed)(struct ste_conn_cpd_chip_dev *dev);
	void (*last_gnss_user_removed)(struct ste_conn_cpd_chip_dev *dev);
};

/**
 * struct ste_conn_cpd_chip_dev - Chip handler info structure.
 * @dev:	Parent device from CPD.
 * @chip:	Chip info such as manufacturer.
 * @callbacks:	Callback structure for the chip handler.
 * @user_data:	Arbitrary data set by chip handler.
 */
struct ste_conn_cpd_chip_dev {
	struct device				*dev;
	struct ste_conn_cpd_chip_info		chip;
	struct ste_conn_cpd_chip_callbacks	callbacks;
	void					*user_data;
};

/**
 * struct ste_conn_cpd_chip_dev - Chip handler identification callbacks.
 * @check_chip_support:	Called when chip is connected. If chip is supported by driver, return true and fill in @callbacks in @dev.
 *
 * Note that the callback may be NULL. It must always be NULL checked before calling.
 */
struct ste_conn_cpd_id_callbacks {
	bool (*check_chip_support)(struct ste_conn_cpd_chip_dev *dev);
};

/**
 * ste_conn_cpd_init() - Initialize module.
 * @char_dev_usage: Bitmask indicating what char device to enable.
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
 * The ste_conn_cpd_data_received() function checks which channel
 * the data was received on and send to the right user.
 */
extern void ste_conn_cpd_data_received(struct sk_buff *skb);


/**
 * ste_conn_cpd_register_chip_driver() - Register a chip handler.
 * @callbacks: Callbacks to call when chip is connected.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL is supplied as @callbacks.
 *   -ENOMEM if allocation fails or work queue can't be created.
 */
extern int ste_conn_cpd_register_chip_driver(struct ste_conn_cpd_id_callbacks *callbacks);

/**
 * ste_conn_cpd_chip_startup_finished() - Called from chip handler when start-up is finished.
 * @err:	Result of the start-up.
 *
 * Returns:
 *   0 if there is no error.
 */
extern int ste_conn_cpd_chip_startup_finished(int err);

/**
 * ste_conn_cpd_chip_shutdown_finished() - Called from chip handler when shutdown is finished.
 * @err:	Result of the shutdown.
 *
 * Returns:
 *   0 if there is no error.
 */
extern int ste_conn_cpd_chip_shutdown_finished(int err);


/**
 * ste_conn_cpd_send_to_chip() - Send data to chip.
 * @skb:	Packet to transmit.
 * @use_logger:	true if hci_logger should copy data content.
 *
 * Returns:
 *   0 if there is no error.
 */
extern int ste_conn_cpd_send_to_chip(struct sk_buff *skb, bool use_logger);


/**
 * ste_conn_cpd_get_bt_cmd_dev() - Return user of the BT command H:4 channel. May be NULL.
 *
 * Returns:
 *   User of the BT command H:4 channel.
 *   NULL if no user is registered.
 */
extern struct ste_conn_device *ste_conn_cpd_get_bt_cmd_dev(void);


/**
 * ste_conn_cpd_get_fm_radio_dev() - Return user of the FM radio H:4 channel. May be NULL.
 *
 * Returns:
 *   User of the FM radio H:4 channel.
 *   NULL if no user is registered.
 */
extern struct ste_conn_device *ste_conn_cpd_get_fm_radio_dev(void);


/**
 * ste_conn_cpd_get_bt_audio_dev() - Return user of the BT audio H:4 channel. May be NULL.
 *
 * Returns:
 *   User of the BT audio H:4 channel.
 *   NULL if no user is registered.
 */
extern struct ste_conn_device *ste_conn_cpd_get_bt_audio_dev(void);


/**
 * ste_conn_cpd_get_fm_audio_dev() - Return user of the FM audio H:4 channel. May be NULL.
 *
 * Returns:
 *   User of the FM audio H:4 channel.
 *   NULL if no user is registered.
 */
extern struct ste_conn_device *ste_conn_cpd_get_fm_audio_dev(void);


/**
 * ste_conn_cpd_get_hci_logger_config() - Return HCI Logger configuration. May be NULL.
 *
 * Returns:
 *   HCI logger configuration.
 *   NULL if CPD has not yet been started.
 */
extern struct ste_conn_cpd_hci_logger_config *ste_conn_cpd_get_hci_logger_config(void);


/**
 * ste_conn_cpd_get_h4_channel() - Get H:4 channel by name.
 * @name:	Name to look up.
 * @h4_channel:	H:4 channel to fill in.
 *
 * This function will check with respective chip handler which channels are supported
 *
 * Returns:
 *   0 if channel was found and @h4_channel has been filled in.
 *   -ENXIO if channel was not found.
 *   -EINVAL if NULL is supplied as parameter.
 */
extern int ste_conn_cpd_get_h4_channel(char *name, int *h4_channel);

#endif /* _STE_CONN_CPD_H_ */
