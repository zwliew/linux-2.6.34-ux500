/*
 * file ste_conn_hci_driver.c
 *
 * Copyright (C) ST-Ericsson AB 2010
 *
 * Linux Bluetooth HCI H:4 Driver for ST-Ericsson controller.
 * License terms: GNU General Public License (GPL), version 2
 *
 * Authors:
 * Pär-Gunnar Hjälmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Henrik Possung (henrik.possung@stericsson.com) for ST-Ericsson.
 * Josef Kindberg (josef.kindberg@stericsson.com) for ST-Ericsson.
 * Dariusz Szymszak (dariusz.xd.szymczak@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson.
 */

#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci.h>
#include <net/bluetooth/hci_core.h>

#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/rfkill.h>
#include <linux/sched.h>
#include <linux/timer.h>


#include <linux/mfd/ste_conn.h>
#include <mach/ste_conn_devices.h>
#include "ste_conn_cpd.h"
#include "ste_conn_debug.h"

#define VERSION "1.0"

/* HCI device type */
#define HCI_STE_CONN				HCI_VIRTUAL

/* HCI Driver name */
#define STE_CONN_HCI_NAME			"ste_conn_hci_driver\0"

/* BT HCI command OpCodes (LSB and MSB) */
#define HCI_RESET_CMD_LSB			0x03
#define HCI_RESET_CMD_MSB			0x0C

/* BT HCI Event OpCodes */
#define HCI_BT_EVT_CMD_COMPLETE			0x0E
#define HCI_BT_EVT_CMD_STATUS			0x0F

/* BT HCI Error codes */
#define HCI_BT_ERROR_NO_ERROR			0x00
#define HCI_BT_ERROR_CMD_DISALLOWED		0x0C

/* BT HCI Reset command parameters */
#define HCI_RESET_LEN				3
#define HCI_RESET_PARAM_LEN			0

/* BT HCI event positions */
#define HCI_EVT_OP_CODE_POS			0
#define HCI_EVT_LEN_POS				1
#define HCI_EVT_CMD_COMPLETE_CMD_LSB_POS	3
#define HCI_EVT_CMD_COMPLETE_CMD_MSB_POS	4
#define HCI_EVT_CMD_COMPLETE_STATUS_POS		5
#define HCI_EVT_CMD_STATUS_STATUS_POS		2
#define HCI_EVT_CMD_STATUS_CMD_LSB_POS		4
#define HCI_EVT_CMD_STATUS_CMD_MSB_POS		5

/* State-setting defines */
#define HCI_SET_RESET_STATE(__hci_reset_new_state) \
	STE_CONN_SET_STATE("hci reset_state", hci_info->hreset_state, __hci_reset_new_state)
#define HCI_SET_ENABLE_STATE(__hci_enable_new_state) \
	STE_CONN_SET_STATE("hci enable_state", hci_info->enable_state, __hci_enable_new_state)
#define SET_RFKILL_STATE(__new_state) \
	STE_CONN_SET_STATE("hci rfkill_state", hci_info->rfkill_state, __new_state)
#define SET_RFKILL_RF_STATE(__new_state) \
	STE_CONN_SET_STATE("hci rfkill_rf_state", hci_info->rfkill_rf_state, __new_state)

/**
  * enum ste_conn_hci_rfkill_rf_state - RFKill RF state.
  * @RFKILL_RF_ENABLED:		Radio is not disabled.
  * @RFKILL_HCI_RESET_SENT:	HCI reset has been sent. Waiting for reply.
  * @RFKILL_RF_DISABLED:	Radio is disabled.
  */
enum ste_conn_hci_rfkill_rf_state {
	RFKILL_RF_ENABLED,
	RFKILL_HCI_RESET_SENT,
	RFKILL_RF_DISABLED
};

/**
  * enum ste_conn_hci_reset_state - RESET-state for hci driver.
  *
  * @HCI_RESET_STATE_IDLE: No reset in progress.
  * @HCI_RESET_STATE_1_CB_RECIVED: One reset callback of three callbacks received.
  * @HCI_RESET_STATE_2_CB_RECIVED: Two reset callbacks of three callbacks received.
  * @HCI_RESET_STATE_UNREGISTER: Unregister and free hci device.
  * @HCI_RESET_STATE_REGISTER: Alloc and register hci device.
  * @HCI_RESET_STATE_FAILED:  Reset failed.
  */

enum ste_conn_hci_reset_state {
	HCI_RESET_STATE_IDLE,
	HCI_RESET_STATE_1_CB_RECIVED,
	HCI_RESET_STATE_2_CB_RECIVED,
	HCI_RESET_STATE_UNREGISTER,
	HCI_RESET_STATE_REGISTER,
	HCI_RESET_STATE_SUCCESS,
	HCI_RESET_STATE_FAILED
};

/**
  * This enum holds the different internal states of the HCI driver.
  *
  * @HCI_ENABLE_STATE_IDLE      The HCI driver is loaded but not opened.
  * @HCI_ENABLE_STATE_WAITING_BT_ENABLED_CC
  *                              The HCI driver is waiting for a command complete event from
  *                              the BT chip as a response to a BT Enable (true) command.
  * @HCI_ENABLE_STATE_BT_ENABLED
  *                              The BT chip is enabled.
  * @HCI_ENABLE_STATE_WAITING_BT_DISABLED_CC
  *                              The HCI driver is waiting for a command complete event from
  *                              the BT chip as a response to a BT Enable (false) command.
  * @HCI_ENABLE_STATE_BT_DISABLED
  *                              The BT chip is disabled.
  * @HCI_ENABLE_STATE_BT_ERROR The HCI driver is in a bad state, some thing has failed and
  *                              is not expected to work properly.
  */
enum ste_conn_hci_enable_state {
	HCI_ENABLE_STATE_IDLE,
	HCI_ENABLE_STATE_WAITING_BT_ENABLED_CC,
	HCI_ENABLE_STATE_BT_ENABLED,
	HCI_ENABLE_STATE_WAITING_BT_DISABLED_CC,
	HCI_ENABLE_STATE_BT_DISABLED,
	HCI_ENABLE_STATE_BT_ERROR
};

/**
  * struct ste_conn_hci_info - Specifies hci driver private data.
  *
  * This type specifies ste_conn hci driver private data.
  *
  * @cpd_bt_cmd:	Device structure for bt cmd channel.
  * @cpd_bt_evt:	Device structure for bt evt channel.
  * @cpd_bt_acl:	Device structure for bt acl channel.
  * @hdev:		Device structure for hci device.
  * @hreset_state:	Device enum for hci driver reset state.
  * @enable_state:	Device enum for hci driver BT enable state.
  * @rfkill:		RFKill structure.
  * @rfkill_state:	Current RFKill state.
  * @rfkill_rf_state:	Current RFKill RF state.
  */
struct ste_conn_hci_info {
	struct ste_conn_device			*cpd_bt_cmd;
	struct ste_conn_device			*cpd_bt_evt;
	struct ste_conn_device			*cpd_bt_acl;
	struct hci_dev				*hdev;
	enum ste_conn_hci_reset_state		hreset_state;
	enum ste_conn_hci_enable_state		enable_state;
	struct rfkill				*rfkill;
	int					rfkill_state;
	enum ste_conn_hci_rfkill_rf_state	rfkill_rf_state;
};

/**
  * struct hci_driver_dev_info - Specifies private data used when receiving
  * callbacks from CPD.
  *
  * @hdev:          Device structure for hci device.
  * @hci_data_type: Type of data according to BlueZ.
  */
struct hci_driver_dev_info {
	struct hci_dev *hdev;
	uint8_t hci_data_type;
};


/*
  * time_5s - 5 second time struct.
  */
static struct timeval time_5s = {
	.tv_sec = 5,
	.tv_usec = 0
};

/*
  * ste_conn_hci_driver_wait_queue - Main Wait Queue in hci driver.
  */
static DECLARE_WAIT_QUEUE_HEAD(ste_conn_hci_driver_wait_queue);

/* Variables where LSB and MSB for bt_enable cmd is stored */
static uint8_t bt_enable_cmd_lsb;
static uint8_t bt_enable_cmd_msb;

/* Module init and exit procedures */
static int __init ste_conn_hci_driver_init(void);
static void __exit ste_conn_hci_driver_exit(void);

/* HCI handlers */
static void remove_bt_users(struct ste_conn_hci_info *info);
static int ste_conn_hci_open(struct hci_dev *hdev);
static int ste_conn_hci_close(struct hci_dev *hdev);
static int ste_conn_hci_flush(struct hci_dev *hdev);
static int ste_conn_hci_send(struct sk_buff *skb);
static void ste_conn_hci_destruct(struct hci_dev *hdev);
static void ste_conn_hci_notify(struct hci_dev *hdev, unsigned int evt);
static int ste_conn_hci_ioctl(struct hci_dev *hdev, unsigned int cmd, unsigned long arg);

/* ste_conn driver handlers */
static void ste_conn_hci_cpd_read_cb(struct ste_conn_device *dev, struct sk_buff *skb);
static void ste_conn_hci_cpd_reset_cb(struct ste_conn_device *dev);

/* RFKill handling functions */
static int ste_conn_hci_rfkill_register(void);
static void ste_conn_hci_rfkill_deregister(void);
static int ste_conn_hci_rfkill_toggle_radio(void *data, int state);
static int ste_conn_hci_rfkill_get_state(void *data, int *state);

/*
  * struct ste_conn_hci_cb - Specifies callback structure for ste_conn user.
  *
  * This type specifies callback structure for ste_conn user.
  *
  * @read_cb:  Callback function called when data is received from the controller
  * @reset_cb: Callback function called when the controller has been reset
  */
static struct ste_conn_callbacks ste_conn_hci_cb = {
	.read_cb = ste_conn_hci_cpd_read_cb,
	.reset_cb = ste_conn_hci_cpd_reset_cb
};

struct ste_conn_hci_info *hci_info;

/* time_1s - 1 second time struct.*/
static struct timeval time_1s = {
	.tv_sec = 1,
	.tv_usec = 0
};

/*
  * ste_conn_hci_wait_queue - Main Wait Queue in HCI driver.
  */
static DECLARE_WAIT_QUEUE_HEAD(ste_conn_hci_wait_queue);

/* Internal functions */

/**
 * remove_bt_users() - unregister and remove any existing BT users.
 * @info: STE_CONN HCI info structure.
 */
static void remove_bt_users(struct ste_conn_hci_info *info)
{
	if (info->cpd_bt_cmd) {
		if (info->cpd_bt_cmd->user_data) {
			kfree(info->cpd_bt_cmd->user_data);
			info->cpd_bt_cmd->user_data = NULL;
		}
		ste_conn_deregister(info->cpd_bt_cmd);
		info->cpd_bt_cmd = NULL;
	}

	if (info->cpd_bt_evt) {
		if (info->cpd_bt_evt->user_data) {
			kfree(info->cpd_bt_evt->user_data);
			info->cpd_bt_evt->user_data = NULL;
		}
		ste_conn_deregister(info->cpd_bt_evt);
		info->cpd_bt_evt = NULL;
	}

	if (info->cpd_bt_acl) {
		if (info->cpd_bt_acl->user_data) {
			kfree(info->cpd_bt_acl->user_data);
			info->cpd_bt_acl->user_data = NULL;
		}
		ste_conn_deregister(info->cpd_bt_acl);
		info->cpd_bt_acl = NULL;
	}
}

/**
 * ste_conn_hci_open() - open ste_conn hci interface.
 * @hdev: HCI device being opened.
 *
 * BlueZ callback function for opening hci interface to device.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL pointer is supplied.
 *   -EOPNOTSUPP if supplied packet type is not supported.
 *   -EBUSY if device is already opened.
 *   -EACCES if opening of channels failed.
 */
static int ste_conn_hci_open(struct hci_dev *hdev)
{
	struct ste_conn_hci_info *info;
	struct hci_driver_dev_info *dev_info;
	struct sk_buff *enable_cmd;
	int err = 0;

	STE_CONN_INFO("Open ST-Ericsson connectivity hci driver ver %s", VERSION);

	if (!hdev) {
		STE_CONN_ERR("NULL supplied for hdev");
		err = -EINVAL;
		goto finished;
	}

	info = (struct ste_conn_hci_info *)hdev->driver_data;
	if (!info) {
		STE_CONN_ERR("NULL supplied for driver_data");
		err = -EINVAL;
		goto finished;
	}

	if (test_and_set_bit(HCI_RUNNING, &(hdev->flags))) {
		STE_CONN_ERR("Device already opened!");
		err = -EBUSY;
		goto finished;
	}

	if (!(info->cpd_bt_cmd)) {
		info->cpd_bt_cmd = ste_conn_register(STE_CONN_DEVICES_BT_CMD, &ste_conn_hci_cb);
		if (info->cpd_bt_cmd) {
			dev_info = kmalloc(sizeof(*dev_info), GFP_KERNEL);
			dev_info->hdev = hdev;
			dev_info->hci_data_type = HCI_COMMAND_PKT;
			info->cpd_bt_cmd->user_data = dev_info;
		} else {
			STE_CONN_ERR("Couldn't register STE_CONN_DEVICES_BT_CMD to CPD");
			err = -EACCES;
			goto handle_error;
		}
	}

	if (!(info->cpd_bt_evt)) {
		info->cpd_bt_evt = ste_conn_register(STE_CONN_DEVICES_BT_EVT, &ste_conn_hci_cb);
		if (info->cpd_bt_evt) {
			dev_info = kmalloc(sizeof(*dev_info), GFP_KERNEL);
			dev_info->hdev = hdev;
			dev_info->hci_data_type = HCI_EVENT_PKT;
			info->cpd_bt_evt->user_data = dev_info;
		} else {
			STE_CONN_ERR("Couldn't register STE_CONN_DEVICES_BT_EVT to CPD");
			err = -EACCES;
			goto handle_error;
		}
	}

	if (!(info->cpd_bt_acl)) {
		info->cpd_bt_acl = ste_conn_register(STE_CONN_DEVICES_BT_ACL, &ste_conn_hci_cb);
		if (info->cpd_bt_acl) {
			dev_info = kmalloc(sizeof(*dev_info), GFP_KERNEL);
			dev_info->hdev = hdev;
			dev_info->hci_data_type = HCI_ACLDATA_PKT;
			info->cpd_bt_acl->user_data = dev_info;
		} else {
			STE_CONN_ERR("Couldn't register STE_CONN_DEVICES_BT_ACL to CPD");
			err = -EACCES;
			goto handle_error;
		}
	}

	if(info->hreset_state == HCI_RESET_STATE_REGISTER){
		HCI_SET_RESET_STATE(HCI_RESET_STATE_IDLE);
	}

	/* Call ste_conn_devices.c implemented function that returns the chip dependant bt enable(true) HCI command.
	 * If NULL is returned, then no bt enable command should be sent to the chip.
	 */
	enable_cmd = ste_conn_devices_get_bt_enable_cmd(&bt_enable_cmd_lsb, &bt_enable_cmd_msb, true);
	if(enable_cmd != NULL) {
		/* Set the HCI state before sending command to chip. */
		HCI_SET_ENABLE_STATE(HCI_ENABLE_STATE_WAITING_BT_ENABLED_CC);

		/* Send command to chip */
		ste_conn_write(info->cpd_bt_cmd, enable_cmd);

		/* Wait for callback to receive command complete and then wake us up again. */
		wait_event_interruptible_timeout(ste_conn_hci_driver_wait_queue,
						(info->enable_state == HCI_ENABLE_STATE_BT_ENABLED),
						timeval_to_jiffies(&time_5s));
		/* Check the current state to se that it worked. */
		if (info->enable_state != HCI_ENABLE_STATE_BT_ENABLED) {
			STE_CONN_ERR("Could not enable BT core (%d).", info->enable_state);
			err = -EACCES;
			HCI_SET_ENABLE_STATE(HCI_ENABLE_STATE_BT_DISABLED);
			goto handle_error;
		}
	} else {
		/* The chip is enabled by default */
		HCI_SET_ENABLE_STATE(HCI_ENABLE_STATE_BT_ENABLED);
	}

	goto finished;

handle_error:
	remove_bt_users(info);
	clear_bit(HCI_RUNNING, &(hdev->flags));
finished:
	return err;

}

/**
 * ste_conn_hci_close() - close hci interface.
 * @hdev: HCI device being closed.
 *
 * BlueZ callback function for closing hci interface.
 * It flushes the interface first.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL pointer is supplied.
 *   -EOPNOTSUPP if supplied packet type is not supported.
 *   -EBUSY if device is not opened.
 */
static int ste_conn_hci_close(struct hci_dev *hdev)
{
	struct ste_conn_hci_info *info = NULL;
	struct sk_buff *enable_cmd;
	int err = 0;

	STE_CONN_INFO("ste_conn_hci_close");

	if (!hdev) {
		STE_CONN_ERR("NULL supplied for hdev");
		err = -EINVAL;
		goto finished;
	}

	info = (struct ste_conn_hci_info *)hdev->driver_data;
	if (!info) {
		STE_CONN_ERR("NULL supplied for driver_data");
		err = -EINVAL;
		goto finished;
	}

	if (!test_and_clear_bit(HCI_RUNNING, &(hdev->flags))) {
		STE_CONN_ERR("Device already closed!");
		err = -EBUSY;
		goto finished;
	}

	ste_conn_hci_flush(hdev);

	/*If reset has occured cpd will handle the deregistration of all users*/
	if(info->hreset_state == HCI_RESET_STATE_UNREGISTER){
		info->cpd_bt_cmd = NULL;
		info->cpd_bt_evt = NULL;
		info->cpd_bt_acl = NULL;
		goto finished;
	}

	/* Call ste_conn_devices.c implemented function that returns the chip dependant bt enable(false) HCI command.
	 * If NULL is returned, then no bt enable(false) command should be sent to the chip.
	 */
	enable_cmd = ste_conn_devices_get_bt_enable_cmd(&bt_enable_cmd_lsb, &bt_enable_cmd_msb, false);
	if(enable_cmd != NULL) {
		/* Set the HCI state before sending command to chip. */
		HCI_SET_ENABLE_STATE(HCI_ENABLE_STATE_WAITING_BT_DISABLED_CC);

		/* Send command to chip */
		ste_conn_write(info->cpd_bt_cmd, enable_cmd);

		/* Wait for callback to receive command complete and then wake us up again. */
		wait_event_interruptible_timeout(ste_conn_hci_driver_wait_queue,
						(info->enable_state == HCI_ENABLE_STATE_BT_DISABLED),
						timeval_to_jiffies(&time_5s));
		/* Check the current state to se that it worked. */
		if (info->enable_state != HCI_ENABLE_STATE_BT_DISABLED) {
			STE_CONN_ERR("Could not disable BT core.");
			HCI_SET_ENABLE_STATE(HCI_ENABLE_STATE_BT_ENABLED);
		}
	} else {
		/* The chip is enabled by default and we should not disable it */
		HCI_SET_ENABLE_STATE(HCI_ENABLE_STATE_BT_ENABLED);
	}

	/* Finally deregister all users and free allocated data */
	remove_bt_users(info);

finished:
	return err;
}

/**
 * ste_conn_hci_flush() - flush hci interface.
 * @hdev: HCI device being flushed.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL pointer is supplied.
 */
static int ste_conn_hci_flush(struct hci_dev *hdev)
{
	STE_CONN_INFO("ste_conn_hci_flush");

	if (!hdev) {
		STE_CONN_ERR("NULL supplied for hdev");
		return -EINVAL;
	}

	return 0;
}

/**
 * ste_conn_hci_send() - send packet to device.
 * @skb: sk buffer to be sent.
 *
 * BlueZ callback function for sending sk buffer.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL pointer is supplied.
 *   -EOPNOTSUPP if supplied packet type is not supported.
 *   -EACCES if write operation is blocked by RFKill.
 *   Error codes from ste_conn_write.
 */
static int ste_conn_hci_send(struct sk_buff *skb)
{
	struct hci_dev *hdev;
	struct ste_conn_hci_info *info;
	int err = 0;

	if (!skb) {
		STE_CONN_ERR("NULL supplied for skb");
		return -EINVAL;
	}

	hdev = (struct hci_dev *)(skb->dev);
	if (!hdev) {
		STE_CONN_ERR("NULL supplied for hdev");
		return -EINVAL;
	}

	info = (struct ste_conn_hci_info *)hdev->driver_data;
	if (!info) {
		STE_CONN_ERR("NULL supplied for info");
		return -EINVAL;
	}

	if (RFKILL_STATE_UNBLOCKED != info->rfkill_state) {
		STE_CONN_ERR("RF disabled by RFKill. Packet blocked");
		return -EACCES;
	}

	/* Update BlueZ stats */
	hdev->stat.byte_tx += skb->len;

	STE_CONN_DBG_DATA("Data transmit %d bytes", skb->len);

	switch (bt_cb(skb)->pkt_type) {
	case HCI_COMMAND_PKT:
		STE_CONN_DBG("Sending HCI_COMMAND_PKT");
		err =
		ste_conn_write(info->cpd_bt_cmd, skb);
		hdev->stat.cmd_tx++;
		break;
	case HCI_ACLDATA_PKT:
		STE_CONN_DBG("Sending HCI_ACLDATA_PKT");
		err =
		ste_conn_write(info->cpd_bt_acl, skb);
		hdev->stat.acl_tx++;
		break;
	default:
		STE_CONN_ERR
		("Trying to transmit unsupported packet type (0x%.2X)",
		bt_cb(skb)->pkt_type);
		err = -EOPNOTSUPP;
		break;
	};

	return err;
}

/**
 * ste_conn_hci_destruct() - destruct hci interface.
 * @hdev: HCI device being destructed.
 */
static void ste_conn_hci_destruct(struct hci_dev *hdev)
{
	STE_CONN_INFO("ste_conn_hci_destruct");

	if (!hci_info) {
		goto finish;
	}

	if (hci_info->hreset_state == HCI_RESET_STATE_UNREGISTER){
		if (hci_info->hdev) {
			hci_free_dev(hci_info->hdev);
		}
		HCI_SET_RESET_STATE(HCI_RESET_STATE_REGISTER);
	}
finish:
	return;
}

/**
 * hci_caif_hci_notify() - notify hci interface.
 * @hdev: HCI device notifying.
 * @evt:  Notification event.
 */
static void ste_conn_hci_notify(struct hci_dev *hdev, unsigned int evt)
{
	STE_CONN_INFO("ste_conn_hci_notify - evt = 0x%.2X", evt);
}

/**
 * ste_conn_hci_ioctl() - handle IOCTL call to the interface.
 * @hdev: HCI device.
 * @cmd:  IOCTL cmd.
 * @arg:  IOCTL arg.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL is supplied for hdev.
 */
static int ste_conn_hci_ioctl(struct hci_dev *hdev, unsigned int cmd, unsigned long arg)
{
	STE_CONN_INFO("ste_conn_hci_ioctl - cmd 0x%X", cmd);

	if (!hdev) {
		STE_CONN_ERR("NULL supplied for hdev");
		return -EINVAL;
	}

	return 0;
}

/**
 * ste_conn_hci_cpd_read_cb() - handle data received from connectivity protocol driver.
 * @dev: Device receiving data.
 * @skb: Buffer with data coming form device.
 */
static void ste_conn_hci_cpd_read_cb(struct ste_conn_device *dev, struct sk_buff *skb)
{
	int err = 0;
	struct hci_driver_dev_info *dev_info;
	uint8_t status;

	if (!dev) {
		STE_CONN_ERR("dev == NULL");
		return;
	}

	dev_info = (struct hci_driver_dev_info *)dev->user_data;

	if (!dev_info) {
		STE_CONN_ERR("dev_info == NULL");
		return;
	}

	/* Check if HCI Driver it self is excpecting a CC packet from the chip after a BT Enable cmd. */
	if ((hci_info->enable_state == HCI_ENABLE_STATE_WAITING_BT_ENABLED_CC ||
		hci_info->enable_state == HCI_ENABLE_STATE_WAITING_BT_DISABLED_CC) &&
		hci_info->cpd_bt_evt->h4_channel == dev->h4_channel &&
		skb->data[HCI_EVT_OP_CODE_POS] == HCI_BT_EVT_CMD_COMPLETE &&
		skb->data[HCI_EVT_CMD_COMPLETE_CMD_LSB_POS] == bt_enable_cmd_lsb &&
		skb->data[HCI_EVT_CMD_COMPLETE_CMD_MSB_POS] == bt_enable_cmd_msb) {
		/* This is the command complete event for the HCI_Cmd_VS_Bluetooth_Enable.
		 * Check result and update state.
		 */
		status = skb->data[HCI_EVT_CMD_COMPLETE_STATUS_POS];
		if (status == HCI_BT_ERROR_NO_ERROR || status == HCI_BT_ERROR_CMD_DISALLOWED) {
			/* The BT chip is enabled(disabled. Either it was enabled/disabled now (status 0x00)
			 * or it was already enabled/disabled (assuming status 0x0c is already enabled/disabled).
			 */
			if (hci_info->enable_state == HCI_ENABLE_STATE_WAITING_BT_ENABLED_CC) {
				HCI_SET_ENABLE_STATE(HCI_ENABLE_STATE_BT_ENABLED);
				STE_CONN_DBG("BT core is enabled.");
			} else {
				HCI_SET_ENABLE_STATE(HCI_ENABLE_STATE_BT_DISABLED);
				STE_CONN_DBG("BT core is disabled.");
			}
		} else {
			STE_CONN_ERR("Could not enable/disable BT core (0x%X).", status);
			/* This should not happend. Put state to ERROR. */
			HCI_SET_ENABLE_STATE(HCI_ENABLE_STATE_BT_ERROR);
		}

		kfree_skb(skb);

		/* Wake up whom ever is waiting for this result. */
		wake_up_interruptible(&ste_conn_hci_driver_wait_queue);
	} else if ((hci_info->enable_state == HCI_ENABLE_STATE_WAITING_BT_DISABLED_CC ||
		hci_info->enable_state == HCI_ENABLE_STATE_WAITING_BT_ENABLED_CC) &&
		hci_info->cpd_bt_evt->h4_channel == dev->h4_channel &&
		skb->data[HCI_EVT_OP_CODE_POS] == HCI_BT_EVT_CMD_STATUS &&
		skb->data[HCI_EVT_CMD_STATUS_CMD_LSB_POS] == bt_enable_cmd_lsb &&
		skb->data[HCI_EVT_CMD_STATUS_CMD_MSB_POS] == bt_enable_cmd_msb ) {
		/* Clear the status events since the Bluez is not expecting them. */
		STE_CONN_DBG("HCI Driver received Command Status(for BT enable):%x", skb->data[HCI_EVT_CMD_STATUS_STATUS_POS]);
		/* This is the command status event for the HCI_Cmd_VS_Bluetooth_Enable.
		 * Just free the packet.
		 */
		kfree_skb(skb);
	} else if ((RFKILL_HCI_RESET_SENT == hci_info->rfkill_rf_state) &&
		(dev == hci_info->cpd_bt_evt) &&
		(HCI_BT_EVT_CMD_COMPLETE == skb->data[HCI_EVT_OP_CODE_POS]) &&
		(HCI_RESET_CMD_LSB == skb->data[HCI_EVT_CMD_COMPLETE_CMD_LSB_POS]) &&
		(HCI_RESET_CMD_MSB == skb->data[HCI_EVT_CMD_COMPLETE_CMD_MSB_POS])) {
		STE_CONN_DBG("Received command complete for HCI reset with status 0x%X",
			skb->data[HCI_EVT_CMD_COMPLETE_STATUS_POS]);
		SET_RFKILL_RF_STATE(RFKILL_RF_DISABLED);
		wake_up_interruptible(&ste_conn_hci_wait_queue);
		kfree_skb(skb);
	} else {

		bt_cb(skb)->pkt_type = dev_info->hci_data_type;
		skb->dev = (struct net_device *)dev_info->hdev;
		/* Update BlueZ stats */
		dev_info->hdev->stat.byte_rx += skb->len;
		if (bt_cb(skb)->pkt_type == HCI_ACLDATA_PKT) {
			dev_info->hdev->stat.acl_rx++;
		} else {
			dev_info->hdev->stat.evt_rx++;
		}

		STE_CONN_DBG_DATA("Data receive %d bytes", skb->len);

		/* provide BlueZ with received frame*/
		err = hci_recv_frame(skb);

		if (err) {
			STE_CONN_ERR("Failed in supplying packet to BlueZ. Error %d", err);

			if (skb) {
				kfree_skb(skb);
			}
		}
	}
}

/**
 * ste_conn_hci_cpd_reset_cb() - Reset callback fuction.
 * @dev: CPD device reseting.
 */
static void ste_conn_hci_cpd_reset_cb(struct ste_conn_device *dev)
{
	int err = 0;
	struct hci_dev *hdev;
	struct hci_driver_dev_info *dev_info;

	STE_CONN_INFO("ste_conn_hci_cpd_reset_cb");

	if (!dev) {
		STE_CONN_ERR("NULL supplied for dev");
		HCI_SET_RESET_STATE(HCI_RESET_STATE_FAILED);
		goto finished;
	}

	dev_info = (struct hci_driver_dev_info *)dev->user_data;
	if (!dev_info) {
		STE_CONN_ERR("NULL supplied for dev_info");
		HCI_SET_RESET_STATE(HCI_RESET_STATE_FAILED);
		goto finished;
	}

	hdev = dev_info->hdev;

	//free userdata because dev will be deallocated when this cb returns.
	if (dev->user_data) {
		kfree(dev->user_data);
		dev->user_data = NULL;
	}

	/* bypass if reset is already in progress*/
	if((hci_info->hreset_state != HCI_RESET_STATE_IDLE) &&
		(hci_info->hreset_state != HCI_RESET_STATE_1_CB_RECIVED) &&
		(hci_info->hreset_state != HCI_RESET_STATE_2_CB_RECIVED)){
		STE_CONN_ERR("This should not happen here, bypass reset is already in progress");
		goto finished;
	}

	/*3 callback is expected one per h4 channel, eg cmd, acl and evt.
	Unregister hdev when we have recived reset from all channels */
	if(hci_info->hreset_state == HCI_RESET_STATE_IDLE){
		HCI_SET_RESET_STATE(HCI_RESET_STATE_1_CB_RECIVED);
		goto finished;
	} else if(hci_info->hreset_state == HCI_RESET_STATE_1_CB_RECIVED){
		HCI_SET_RESET_STATE(HCI_RESET_STATE_2_CB_RECIVED);
		goto finished;
	} else {
		HCI_SET_RESET_STATE(HCI_RESET_STATE_UNREGISTER);
	}

	/*Unregister hci device, close and destruct should be called by the bluetooth stack*/
	if (!hdev) {
		STE_CONN_ERR("NULL supplied for hdev");
		HCI_SET_RESET_STATE(HCI_RESET_STATE_FAILED);
		goto finished;
	}

	err = hci_unregister_dev(hdev);
	if (err) {
		STE_CONN_ERR("Can not unregister hci device! ERROR %d", err);
		HCI_SET_RESET_STATE(HCI_RESET_STATE_FAILED);
		goto finished;
	}

	/*Wait until hdev is unregistered and deallocated*/
	wait_event_interruptible_timeout(ste_conn_hci_driver_wait_queue,
					(hci_info->hreset_state == HCI_RESET_STATE_REGISTER),
					timeval_to_jiffies(&time_5s));

	if(hci_info->hreset_state != HCI_RESET_STATE_REGISTER){
		HCI_SET_RESET_STATE(HCI_RESET_STATE_FAILED);
		STE_CONN_ERR("Unregister hci device timed out");
		goto finished;
	}

	/*Alloc and register new hdev*/
	hci_info->hdev = hci_alloc_dev();
	if (!hci_info->hdev) {
		STE_CONN_ERR("Could not allocate mem for hci driver.");
		HCI_SET_RESET_STATE(HCI_RESET_STATE_FAILED);
		goto finished;
	}

	hci_info->hdev->type = HCI_STE_CONN;
	hci_info->hdev->driver_data = hci_info;
	hci_info->hdev->owner = THIS_MODULE;
	hci_info->hdev->open = ste_conn_hci_open;
	hci_info->hdev->close = ste_conn_hci_close;
	hci_info->hdev->flush = ste_conn_hci_flush;
	hci_info->hdev->send = ste_conn_hci_send;
	hci_info->hdev->destruct = ste_conn_hci_destruct;
	hci_info->hdev->notify = ste_conn_hci_notify;
	hci_info->hdev->ioctl = ste_conn_hci_ioctl;


	err = hci_register_dev(hci_info->hdev);
	if (err) {
		STE_CONN_ERR("Can not register hci device! ERROR %d", err);
		hci_free_dev(hci_info->hdev);
		HCI_SET_RESET_STATE(HCI_RESET_STATE_FAILED);

	}

finished:
	return;
}

/**
 * ste_conn_hci_rfkill_register() - Register to RFKill.
 *
 * Returns:
 *   0 if there is no error.
 *   -ENOMEM if allocation fails.
 *   Error codes from rfkill_register.
 */
static int ste_conn_hci_rfkill_register(void)
{
	int err = 0;

	STE_CONN_INFO("ste_conn_hci_rfkill_register");

	SET_RFKILL_STATE(RFKILL_STATE_SOFT_BLOCKED);
	SET_RFKILL_RF_STATE(RFKILL_RF_DISABLED);

	hci_info->rfkill = rfkill_allocate(&(hci_info->hdev->dev), RFKILL_TYPE_BLUETOOTH);
	if (!hci_info->rfkill) {
		STE_CONN_ERR("Could not allocate rfkill device.");
		err = -ENOMEM;
		goto finished;
	}

	hci_info->rfkill->name = STE_CONN_HCI_NAME;
	hci_info->rfkill->data = NULL;
	hci_info->rfkill->state = hci_info->rfkill_state;

	hci_info->rfkill->toggle_radio = ste_conn_hci_rfkill_toggle_radio;
	hci_info->rfkill->get_state    = ste_conn_hci_rfkill_get_state;

	err = rfkill_register(hci_info->rfkill);
	if (err) {
		STE_CONN_ERR("Could not register rfkill device.");
		goto err_free_rfkill;
	}
	STE_CONN_DBG("RFKill registration ... OK.");

	goto finished;

err_free_rfkill:
	STE_CONN_ERR("RFKill freeing rfkill device.");
	if (hci_info->rfkill) {
		rfkill_free(hci_info->rfkill);
		hci_info->rfkill = NULL;
	}

finished:
	return err;
}

/**
 * ste_conn_hci_rfkill_deregister() - Deregister from RFKill.
 */
static void ste_conn_hci_rfkill_deregister(void)
{
	if (hci_info->rfkill) {
		/* Unregister RFKill */
		rfkill_unregister(hci_info->rfkill);
		rfkill_free(hci_info->rfkill);
		hci_info->rfkill = NULL;
	}
}

/**
 * ste_conn_hci_rfkill_toggle_radio() - Called from RFKill upon state change.
 * @data:  Private data (NULL used).
 * @state: New RFKill state.
 *
 * Returns:
 *   0 if there is no error.
 *   -EBUSY if supplied new state is equal to current state or if disabling of
 *   state fails.
 *   -ENOTSUPP if supplied new state is not supported.
 *   -ENOMEM if allocation failed.
 */
static int ste_conn_hci_rfkill_toggle_radio(void *data, enum rfkill_state state)
{
	int err = 0;
	struct sk_buff *skb;
	uint8_t *data_ptr;

	STE_CONN_INFO("ste_conn_hci_rfkill_toggle_radio new state = %d", state);

	if (state == hci_info->rfkill_state) {
		STE_CONN_ERR("ste_conn_hci_rfkill_toggle_radio called without state change (%d)", state);
		err = -EBUSY;
		goto finished;
	}

	/* Store new state */
	SET_RFKILL_STATE(state);

	switch (state) {
	case RFKILL_STATE_SOFT_BLOCKED:
		STE_CONN_DBG("RFKILL disable radio.");

		/* We only need to disable if BT is actually running */
		if (!test_bit(HCI_RUNNING, &(hci_info->hdev->flags))) {
			STE_CONN_DBG("Setting blocked when device is not opened");
			SET_RFKILL_RF_STATE(RFKILL_RF_DISABLED);
			goto finished;
		}

		skb = ste_conn_alloc_skb(HCI_RESET_LEN, GFP_KERNEL);
		if (!skb) {
			STE_CONN_ERR("Could not allocate sk_buffer for HCI reset");
			err = -ENOMEM;
			SET_RFKILL_RF_STATE(RFKILL_RF_DISABLED);
			goto finished;
		}

		data_ptr = skb_put(skb, HCI_RESET_LEN);
		data_ptr[0] = HCI_RESET_CMD_LSB;
		data_ptr[1] = HCI_RESET_CMD_MSB;
		data_ptr[2] = HCI_RESET_PARAM_LEN;

		err = ste_conn_write(hci_info->cpd_bt_cmd, skb);
		if (err) {
			STE_CONN_ERR("ste_conn_write failed when sending HCI reset (%d)", err);
			kfree_skb(skb);
			goto finished;
		}

		SET_RFKILL_RF_STATE(RFKILL_HCI_RESET_SENT);

		/* Wait for reset to finish */
		if (wait_event_interruptible_timeout(ste_conn_hci_wait_queue,
				(RFKILL_RF_DISABLED == hci_info->rfkill_rf_state),
				timeval_to_jiffies(&time_1s)) <= 0) {
			STE_CONN_ERR("Couldn't disable RF in time");
			err = -EBUSY;
			SET_RFKILL_RF_STATE(RFKILL_RF_DISABLED);
		}
		break;

	case RFKILL_STATE_UNBLOCKED:
		STE_CONN_DBG("RFKILL enable radio.");
		/* Remove block of data TX. User will have to handle the reset
		 * of the chip
		 */
		SET_RFKILL_RF_STATE(RFKILL_RF_ENABLED);
		break;

	case RFKILL_STATE_HARD_BLOCKED:
		STE_CONN_ERR("RFKill hard disable radio. Should never happen!");
		err = -ENOTSUPP;
		break;

	default:
		STE_CONN_ERR("RFKill unknown state %d. Should never happen!", state);
		err = -ENOTSUPP;
		break;
	}

finished:
	return err;
}

/**
 * ste_conn_hci_rfkill_get_state - Called from RFKill to read current RFKill state.
 * @data:  Data supplied when registering (NULL).
 * @state: Current RFKill state (to be returned).
 *
 * Returns:
 *   0 if there is no error (no error can occur).
 */
static int ste_conn_hci_rfkill_get_state(void *data, enum rfkill_state *state)
{
	STE_CONN_INFO("ste_conn_hci_rfkill_get_state (%d)", hci_info->rfkill_state);

	*state = hci_info->rfkill_state;
	return 0;
}

/**
 * ste_conn_hci_driver_init() - Initialize module.
 *
 * Add hci device to ste_conn driver and exit.
 *
 * Returns:
 *   0 if there is no error.
 *   -ENOMEM if allocation fails.
 *   Error codes from hci_register_dev.
 */
static int __init ste_conn_hci_driver_init(void)
{
	int err = 0;

	STE_CONN_INFO("ST-Ericsson connectivity hci driver ver %s", VERSION);

	/* Initialize private data. */
	hci_info = kzalloc(sizeof(*hci_info), GFP_KERNEL);
	if (!hci_info) {
		STE_CONN_ERR("Could not alloc hci_info struct.");
		err = -ENOMEM;
		goto finished;
	}

	hci_info->hdev = hci_alloc_dev();
	if (!hci_info->hdev) {
		STE_CONN_ERR("Could not allocate mem for hci driver.");
		err = -ENOMEM;
		goto err_free_info;
	}

	hci_info->hdev->type = HCI_STE_CONN;
	hci_info->hdev->driver_data = hci_info;
	hci_info->hdev->owner = THIS_MODULE;
	hci_info->hdev->open = ste_conn_hci_open;
	hci_info->hdev->close = ste_conn_hci_close;
	hci_info->hdev->flush = ste_conn_hci_flush;
	hci_info->hdev->send = ste_conn_hci_send;
	hci_info->hdev->destruct = ste_conn_hci_destruct;
	hci_info->hdev->notify = ste_conn_hci_notify;
	hci_info->hdev->ioctl = ste_conn_hci_ioctl;

	HCI_SET_ENABLE_STATE(HCI_ENABLE_STATE_IDLE);
	HCI_SET_RESET_STATE(HCI_RESET_STATE_IDLE);

	err = hci_register_dev(hci_info->hdev);
	if (err) {
		STE_CONN_ERR("Can not register hci device! ERROR %d", err);
		goto err_free_hci;
	}

	/* Register to RFKill */
	err = ste_conn_hci_rfkill_register();
	if (err) {
		STE_CONN_ERR("RFKill registration error %d.", err);
		goto err_hci_dereg;
	}

	goto finished;

err_hci_dereg:
	hci_unregister_dev(hci_info->hdev);
err_free_hci:
	hci_free_dev(hci_info->hdev);
err_free_info:
	kfree(hci_info);
	hci_info = NULL;
finished:
	return err;
}

/**
 * ste_conn_hci_driver_exit() - Remove module.
 *
 * Remove hci device from ste_conn driver.
 */
static void __exit ste_conn_hci_driver_exit(void)
{
	int err = 0;

	STE_CONN_INFO("ste_conn hci driver exit.");

	if (hci_info) {
		/* Deregister from RFKill */
		ste_conn_hci_rfkill_deregister();

		if (hci_info->hdev) {
			err = hci_unregister_dev(hci_info->hdev);
			if (err) {
				STE_CONN_ERR("Can not unregister hci device! ERROR %d", err);
			}
			hci_free_dev(hci_info->hdev);
		}

		kfree(hci_info);
		hci_info = NULL;
	}
}


module_init(ste_conn_hci_driver_init);
module_exit(ste_conn_hci_driver_exit);

MODULE_AUTHOR("Par-Gunnar Hjalmdahl ST-Ericsson");
MODULE_AUTHOR("Henrik Possung ST-Ericsson");
MODULE_AUTHOR("Josef Kindberg ST-Ericsson");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Linux Bluetooth HCI H:4 Driver for ST-Ericsson controller ver " VERSION);
MODULE_VERSION(VERSION);
