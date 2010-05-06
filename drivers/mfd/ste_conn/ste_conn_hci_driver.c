/*
 * drivers/mfd/ste_conn/ste_conn_hci_driver.c
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

#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci.h>
#include <net/bluetooth/hci_core.h>

#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/timer.h>


#include <linux/mfd/ste_conn.h>
#include <mach/ste_conn_devices.h>
#include "bluetooth_defines.h"
#include "ste_conn_cpd.h"
#include "ste_conn_debug.h"

/* HCI device type */
#define HCI_STE_CONN				HCI_VIRTUAL

/* HCI Driver name */
#define STE_CONN_HCI_NAME			"ste_conn_hci_driver\0"

/* State-setting defines */
#define HCI_SET_RESET_STATE(__hci_reset_new_state) \
	STE_CONN_SET_STATE("hreset_state", hci_info->hreset_state, __hci_reset_new_state)
#define HCI_SET_ENABLE_STATE(__hci_enable_new_state) \
	STE_CONN_SET_STATE("enable_state", hci_info->enable_state, __hci_enable_new_state)

/**
  * enum ste_conn_hci_reset_state - RESET-states of the HCI driver.
  *
  * @HCI_RESET_STATE_IDLE: No reset in progress.
  * @HCI_RESET_STATE_1_CB_RECIVED: One reset callback of three callbacks received.
  * @HCI_RESET_STATE_2_CB_RECIVED: Two reset callbacks of three callbacks received.
  * @HCI_RESET_STATE_UNREGISTER: Unregister and free hci device.
  * @HCI_RESET_STATE_REGISTER: Alloc and register hci device.
  * @HCI_RESET_STATE_SUCCESS: Reset success.
  * @HCI_RESET_STATE_FAILED:  Reset failed.
  * @HCI_RESET_STATE_IDLE:		No reset in progress.
  * @HCI_RESET_STATE_ACTIVATED:		Reset in progress.
  * @HCI_RESET_STATE_UNREGISTERED:	hdev is unregistered.
  */

enum ste_conn_hci_reset_state {
	HCI_RESET_STATE_IDLE,
	HCI_RESET_STATE_ACTIVATED,
	HCI_RESET_STATE_UNREGISTERED
};

/**
  * enum ste_conn_hci_enable_state - ENABLE-states of the HCI driver.
  *
  * @HCI_ENABLE_STATE_IDLE:			The HCI driver is loaded but not opened.
  * @HCI_ENABLE_STATE_WAITING_BT_ENABLED_CC:	The HCI driver is waiting for a command complete event from
  *						the BT chip as a response to a BT Enable (true) command.
  * @HCI_ENABLE_STATE_BT_ENABLED:		The BT chip is enabled.
  * @HCI_ENABLE_STATE_WAITING_BT_DISABLED_CC:	The HCI driver is waiting for a command complete event from
  *						the BT chip as a response to a BT Enable (false) command.
  * @HCI_ENABLE_STATE_BT_DISABLED:		The BT chip is disabled.
  * @HCI_ENABLE_STATE_BT_ERROR:			The HCI driver is in a bad state, some thing has failed and
  *						is not expected to work properly.
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
  * struct ste_conn_hci_info - Specifies HCI driver private data.
  *
  * This type specifies STE_CONN HCI driver private data.
  *
  * @cpd_bt_cmd:	Device structure for BT command channel.
  * @cpd_bt_evt:	Device structure for BT event channel.
  * @cpd_bt_acl:	Device structure for BT ACL channel.
  * @hdev:		Device structure for HCI device.
  * @hreset_state:	Device enum for HCI driver reset state.
  * @enable_state:	Device enum for HCI driver BT enable state.
  */
struct ste_conn_hci_info {
	struct ste_conn_device			*cpd_bt_cmd;
	struct ste_conn_device			*cpd_bt_evt;
	struct ste_conn_device			*cpd_bt_acl;
	struct hci_dev				*hdev;
	enum ste_conn_hci_reset_state		hreset_state;
	enum ste_conn_hci_enable_state		enable_state;
};

/**
  * struct hci_driver_dev_info - Specifies private data used when receiving callbacks from CPD.
  *
  * @hdev:          Device structure for HCI device.
  * @hci_data_type: Type of data according to BlueZ.
  */
struct hci_driver_dev_info {
	struct hci_dev *hdev;
	uint8_t hci_data_type;
};

/*
  * ste_conn_hci_driver_wait_queue - Main Wait Queue in HCI driver.
  */
static DECLARE_WAIT_QUEUE_HEAD(ste_conn_hci_driver_wait_queue);

/* Variables where LSB and MSB for bt_enable command is stored */
static uint8_t bt_enable_cmd_lsb;
static uint8_t bt_enable_cmd_msb;

/* Module init and exit procedures */
static int __init ste_conn_hci_driver_init(void);
static void __exit ste_conn_hci_driver_exit(void);

/* HCI handlers */
static void remove_bt_users(struct ste_conn_hci_info *info);
static int open_from_hci(struct hci_dev *hdev);
static int close_from_hci(struct hci_dev *hdev);
static int flush_from_hci(struct hci_dev *hdev);
static int send_from_hci(struct sk_buff *skb);
static void destruct_from_hci(struct hci_dev *hdev);
static void notify_from_hci(struct hci_dev *hdev, unsigned int evt);
static int ioctl_from_hci(struct hci_dev *hdev, unsigned int cmd, unsigned long arg);
static int register_to_bluez(void);

/* STE_CONN driver handlers */
static void read_cb(struct ste_conn_device *dev, struct sk_buff *skb);
static void reset_cb(struct ste_conn_device *dev);

/*
  * struct ste_conn_cb - Specifies callback structure for STE_CONN user.
  *
  * This type specifies callback structure for STE_CONN user.
  *
  * @read_cb:  Callback function called when data is received from the controller
  * @reset_cb: Callback function called when the controller has been reset
  */
static struct ste_conn_callbacks ste_conn_cb = {
	.read_cb = read_cb,
	.reset_cb = reset_cb
};

struct ste_conn_hci_info *hci_info;

/*
 * ste_conn_hci_wait_queue - Main Wait Queue in HCI driver.
 */
static DECLARE_WAIT_QUEUE_HEAD(ste_conn_hci_wait_queue);

/* Internal functions */

/**
 * remove_bt_users() - Unregister and remove any existing BT users.
 * @info: HCI driver info structure.
 */
static void remove_bt_users(struct ste_conn_hci_info *info)
{
	if (info->cpd_bt_cmd) {
		kfree(info->cpd_bt_cmd->user_data);
		info->cpd_bt_cmd->user_data = NULL;
		ste_conn_deregister(info->cpd_bt_cmd);
		info->cpd_bt_cmd = NULL;
	}

	if (info->cpd_bt_evt) {
		kfree(info->cpd_bt_evt->user_data);
		info->cpd_bt_evt->user_data = NULL;
		ste_conn_deregister(info->cpd_bt_evt);
		info->cpd_bt_evt = NULL;
	}

	if (info->cpd_bt_acl) {
		kfree(info->cpd_bt_acl->user_data);
		info->cpd_bt_acl->user_data = NULL;
		ste_conn_deregister(info->cpd_bt_acl);
		info->cpd_bt_acl = NULL;
	}
}

/**
 * open_from_hci() - Open HCI interface.
 * @hdev: HCI device being opened.
 *
 * BlueZ callback function for opening HCI interface to device.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL pointer is supplied.
 *   -EOPNOTSUPP if supplied packet type is not supported.
 *   -EBUSY if device is already opened.
 *   -EACCES if opening of channels failed.
 */
static int open_from_hci(struct hci_dev *hdev)
{
	struct ste_conn_hci_info *info;
	struct hci_driver_dev_info *dev_info;
	struct sk_buff *enable_cmd;
	int err = 0;

	STE_CONN_INFO("Open ST-Ericsson connectivity HCI driver ver");

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
		info->cpd_bt_cmd = ste_conn_register(STE_CONN_DEVICES_BT_CMD, &ste_conn_cb);
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
		info->cpd_bt_evt = ste_conn_register(STE_CONN_DEVICES_BT_EVT, &ste_conn_cb);
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
		info->cpd_bt_acl = ste_conn_register(STE_CONN_DEVICES_BT_ACL, &ste_conn_cb);
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

	if (info->hreset_state == HCI_RESET_STATE_ACTIVATED) {
		HCI_SET_RESET_STATE(HCI_RESET_STATE_IDLE);
	}

	/* Call ste_conn_devices.c implemented function that returns the chip dependant bt enable(true) HCI command.
	 * If NULL is returned, then no bt enable command should be sent to the chip.
	 */
	enable_cmd = ste_conn_devices_get_bt_enable_cmd(&bt_enable_cmd_lsb, &bt_enable_cmd_msb, true);
	if (enable_cmd != NULL) {
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
 * close_from_hci() - Close HCI interface.
 * @hdev: HCI device being closed.
 *
 * BlueZ callback function for closing HCI interface.
 * It flushes the interface first.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL pointer is supplied.
 *   -EOPNOTSUPP if supplied packet type is not supported.
 *   -EBUSY if device is not opened.
 */
static int close_from_hci(struct hci_dev *hdev)
{
	struct ste_conn_hci_info *info = NULL;
	struct sk_buff *enable_cmd;
	int err = 0;

	STE_CONN_INFO("close_from_hci");

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

	flush_from_hci(hdev);

	/* Do not do this if there is an reset ongoing */
	if (!hci_info->hreset_state == HCI_RESET_STATE_ACTIVATED) {
		/* Get the chip dependant BT Enable HCI command. The command
		 * parameter shall be set to false to disable the BT core.
		 * If NULL is returned, then no BT Enable command should be
		 * sent to the chip.
		 */
		enable_cmd = ste_conn_devices_get_bt_enable_cmd(&bt_enable_cmd_lsb, &bt_enable_cmd_msb, false);
		if (enable_cmd != NULL) {
			/* Set the HCI state before sending command to chip */
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
	}

	/* Finally deregister all users and free allocated data */
	remove_bt_users(info);

finished:
	return err;
}

/**
 * flush_from_hci() - Flush HCI interface.
 * @hdev: HCI device being flushed.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL pointer is supplied.
 */
static int flush_from_hci(struct hci_dev *hdev)
{
	STE_CONN_INFO("flush_from_hci");

	if (!hdev) {
		STE_CONN_ERR("NULL supplied for hdev");
		return -EINVAL;
	}

	return 0;
}

/**
 * send_from_hci() - Send packet to device.
 * @skb: sk buffer to be sent.
 *
 * BlueZ callback function for sending sk buffer.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL pointer is supplied.
 *   -EOPNOTSUPP if supplied packet type is not supported.
 *   Error codes from ste_conn_write.
 */
static int send_from_hci(struct sk_buff *skb)
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

	/* Update BlueZ stats */
	hdev->stat.byte_tx += skb->len;

	STE_CONN_DBG_DATA("Data transmit %d bytes", skb->len);

	switch (bt_cb(skb)->pkt_type) {
	case HCI_COMMAND_PKT:
		STE_CONN_DBG("Sending HCI_COMMAND_PKT");
		err = ste_conn_write(info->cpd_bt_cmd, skb);
		hdev->stat.cmd_tx++;
		break;
	case HCI_ACLDATA_PKT:
		STE_CONN_DBG("Sending HCI_ACLDATA_PKT");
		err = ste_conn_write(info->cpd_bt_acl, skb);
		hdev->stat.acl_tx++;
		break;
	default:
		STE_CONN_ERR("Trying to transmit unsupported packet type (0x%.2X)",
			     bt_cb(skb)->pkt_type);
		err = -EOPNOTSUPP;
		break;
	};

	return err;
}

/**
 * destruct_from_hci() - Destruct HCI interface.
 * @hdev: HCI device being destructed.
 */
static void destruct_from_hci(struct hci_dev *hdev)
{
	STE_CONN_INFO("destruct_from_hci");

	if (!hci_info) {
		goto finished;
	}

	/* When being reset, register a new hdev when hdev is deregistered */
	if (hci_info->hreset_state == HCI_RESET_STATE_ACTIVATED) {
		if (hci_info->hdev) {
			hci_free_dev(hci_info->hdev);
		}
		HCI_SET_RESET_STATE(HCI_RESET_STATE_UNREGISTERED);
	}
finished:
	return;
}

/**
 * notify_from_hci() - Notification from the HCI interface.
 * @hdev: HCI device notifying.
 * @evt:  Notification event.
 */
static void notify_from_hci(struct hci_dev *hdev, unsigned int evt)
{
	STE_CONN_INFO("notify_from_hci - evt = 0x%.2X", evt);
}

/**
 * ioctl_from_hci() - Handle IOCTL call to the HCI interface.
 * @hdev: HCI device.
 * @cmd:  IOCTL cmd.
 * @arg:  IOCTL arg.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL is supplied for hdev.
 */
static int ioctl_from_hci(struct hci_dev *hdev, unsigned int cmd, unsigned long arg)
{
	int err = 0;

	STE_CONN_INFO("ioctl_from_hci - cmd 0x%X", cmd);

	if (!hdev) {
		STE_CONN_ERR("NULL supplied for hdev");
		err = -EINVAL;
		goto finished;
	}

finished:
	return err;
}

/**
 * read_cb() - Callback for handling data received from STE_CONN driver.
 * @dev: Device receiving data.
 * @skb: Buffer with data coming from device.
 */
static void read_cb(struct ste_conn_device *dev, struct sk_buff *skb)
{
	int err = 0;
	struct hci_driver_dev_info *dev_info;
	uint8_t status;
	uint8_t op_code;
	uint8_t cmd_lsb;
	uint8_t cmd_msb;
	uint8_t status_lsb;
	uint8_t status_msb;

	if (!skb) {
		STE_CONN_ERR("NULL supplied for skb");
		goto finished;
	}

	if (!dev) {
		STE_CONN_ERR("dev == NULL");
		goto err_free_skb;
	}

	dev_info = (struct hci_driver_dev_info *)dev->user_data;

	if (!dev_info) {
		STE_CONN_ERR("dev_info == NULL");
		goto err_free_skb;
	}

	op_code = skb->data[HCI_BT_EVT_ID_OFFSET];
	cmd_lsb = skb->data[HCI_BT_EVT_CMD_COMPL_OP_CODE_OFFSET_LSB];
	cmd_msb = skb->data[HCI_BT_EVT_CMD_COMPL_OP_CODE_OFFSET_MSB];
	status_lsb = skb->data[HCI_BT_EVT_CMD_STATUS_OP_CODE_OFFSET_LSB];
	status_msb = skb->data[HCI_BT_EVT_CMD_STATUS_OP_CODE_OFFSET_MSB];

	/* Check if HCI Driver it self is expecting a Command Complete packet
	 * from the chip after a BT Enable command. */
	if ((hci_info->enable_state == HCI_ENABLE_STATE_WAITING_BT_ENABLED_CC ||
	     hci_info->enable_state == HCI_ENABLE_STATE_WAITING_BT_DISABLED_CC) &&
	    hci_info->cpd_bt_evt->h4_channel == dev->h4_channel &&
	    op_code == HCI_BT_EVT_CMD_COMPLETE &&
	    cmd_lsb == bt_enable_cmd_lsb &&
	    cmd_msb == bt_enable_cmd_msb) {
		/* This is the command complete event for the HCI_Cmd_VS_Bluetooth_Enable.
		 * Check result and update state.
		 */
		status = skb->data[HCI_BT_EVT_CMD_COMPL_STATUS_OFFSET];
		if (status == HCI_BT_ERROR_NO_ERROR || status == HCI_BT_ERROR_CMD_DISALLOWED) {
			/* The BT chip is enabled/disabled. Either it was enabled/disabled now
			 * (status NO_ERROR) or it was already enabled/disabled (assuming status
			 * CMD_DISALLOWED is already enabled/disabled).
			 */
			if (hci_info->enable_state == HCI_ENABLE_STATE_WAITING_BT_ENABLED_CC) {
				HCI_SET_ENABLE_STATE(HCI_ENABLE_STATE_BT_ENABLED);
				STE_CONN_INFO("BT core is enabled");
			} else {
				HCI_SET_ENABLE_STATE(HCI_ENABLE_STATE_BT_DISABLED);
				STE_CONN_INFO("BT core is disabled");
			}
		} else {
			STE_CONN_ERR("Could not enable/disable BT core (0x%X)", status);
			/* This should not happened. Put state to ERROR. */
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
		/* If err, skb have been freed in hci_recv_frame() */
		if (err) {
			STE_CONN_ERR("Failed in supplying packet to BlueZ (%d)", err);
		}
	}

	goto finished;

err_free_skb:
	kfree_skb(skb);
finished:
	return;
}

/**
 * reset_cb() - Callback for handling reset from STE_CONN driver.
 * @dev: CPD device resetting.
 */
static void reset_cb(struct ste_conn_device *dev)
{
	int err = 0;
	struct hci_dev *hdev;
	struct hci_driver_dev_info *dev_info;
	struct ste_conn_hci_info *info;

	STE_CONN_INFO("reset_cb");

	if (!dev) {
		STE_CONN_ERR("NULL supplied for dev");
		goto finished;
	}

	dev_info = (struct hci_driver_dev_info *)dev->user_data;
	if (!dev_info) {
		STE_CONN_ERR("NULL supplied for dev_info");
		goto finished;
	}

	hdev = dev_info->hdev;
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

	switch (dev_info->hci_data_type) {

	case HCI_EVENT_PKT:
		info->cpd_bt_evt = NULL;
		break;

	case HCI_COMMAND_PKT:
		info->cpd_bt_cmd = NULL;
		break;

	case HCI_ACLDATA_PKT:
		info->cpd_bt_acl = NULL;
		break;

	default:
		STE_CONN_ERR("Unknown HCI data type:%d", dev_info->hci_data_type);
		goto finished;
		break;
	}

	HCI_SET_RESET_STATE(HCI_RESET_STATE_ACTIVATED);

	/* Free userdata as device info structure will be freed by STE_CONN
	 * when this callback returns */
	kfree(dev->user_data);
	dev->user_data = NULL;

	/* Continue to deregister hdev if all channels has been reset else goto finished */
	if ((!info->cpd_bt_evt) && (!info->cpd_bt_cmd) && (!info->cpd_bt_acl)) {
		/* Deregister HCI device. Close and Destruct functions should
		 * in turn be called by BlueZ */
		STE_CONN_INFO("Deregister HCI device");
		err = hci_unregister_dev(hdev);
		if (err) {
			STE_CONN_ERR("Can not deregister HCI device! (%d)", err);
			/* Now we are in trouble. Try to register a new hdev
			 * anyway even though this will cost some memory */
		}

		wait_event_interruptible_timeout(ste_conn_hci_wait_queue,
						 (HCI_RESET_STATE_UNREGISTERED == hci_info->hreset_state),
						 timeval_to_jiffies(&time_5s));

		if (HCI_RESET_STATE_UNREGISTERED != hci_info->hreset_state) {
			/* Now we are in trouble. Try to register a new hdev
			 * anyway even though this will cost some memory */
			STE_CONN_ERR("Timeout expired. Could not deregister HCI device");
		}
		/* Init and register hdev */
		STE_CONN_INFO("Register HCI device");
		err = register_to_bluez();
		if (err) {
			STE_CONN_ERR("HCI Device registration error %d.", err);
			goto finished;
		}
	}

finished:
	return;
}

/**
 * register_to_bluez() - Initialize module.
 *
 * Alloc, init, and register HCI device to BlueZ.
 *
 * Returns:
 *   0 if there is no error.
 *   -ENOMEM if allocation fails.
 *   Error codes from hci_register_dev.
 */
static int register_to_bluez(void)
{
	int err = 0;

	hci_info->hdev = hci_alloc_dev();
	if (!hci_info->hdev) {
		STE_CONN_ERR("Could not allocate mem for HCI driver");
		kfree(hci_info);
		err = -ENOMEM;
		goto finished;
	}

	hci_info->hdev->type = HCI_STE_CONN;
	hci_info->hdev->driver_data = hci_info;
	hci_info->hdev->owner = THIS_MODULE;
	hci_info->hdev->open = open_from_hci;
	hci_info->hdev->close = close_from_hci;
	hci_info->hdev->flush = flush_from_hci;
	hci_info->hdev->send = send_from_hci;
	hci_info->hdev->destruct = destruct_from_hci;
	hci_info->hdev->notify = notify_from_hci;
	hci_info->hdev->ioctl = ioctl_from_hci;

	err = hci_register_dev(hci_info->hdev);
	if (err) {
		STE_CONN_ERR("Can not register HCI device (%d)", err);
		hci_free_dev(hci_info->hdev);
		kfree(hci_info);
		hci_info = NULL;
	}

	HCI_SET_ENABLE_STATE(HCI_ENABLE_STATE_IDLE);
	HCI_SET_RESET_STATE(HCI_RESET_STATE_IDLE);

finished:
       return err;
}

/**
 * ste_conn_hci_driver_init() - Initialize module.
 *
 * Allocate and initialize private data. Register to BlueZ and RFKill.
 *
 * Returns:
 *   0 if there is no error.
 *   -ENOMEM if allocation fails.
 *   Error codes from register_to_bluez and register_to_rfkill.
 */
static int __init ste_conn_hci_driver_init(void)
{
	int err = 0;
	STE_CONN_INFO("ste_conn_hci_driver_init");

	/* Initialize private data. */
	hci_info = kzalloc(sizeof(*hci_info), GFP_KERNEL);
	if (!hci_info) {
		STE_CONN_ERR("Could not alloc hci_info struct.");
		err = -ENOMEM;
		goto finished;
	}

	/*Init and register hdev*/
	err = register_to_bluez();
	if (err) {
		STE_CONN_ERR("HCI Device registration error (%d)", err);
		goto err_free_info;
	}

	goto finished;

err_free_info:
	kfree(hci_info);
	hci_info = NULL;
finished:
	return err;
}

/**
 * ste_conn_hci_driver_exit() - Remove module.
 *
 * Remove HCI device from STE_CONN driver.
 */
static void __exit ste_conn_hci_driver_exit(void)
{
	int err = 0;

	STE_CONN_INFO("ste_conn_hci_driver_exit");

	if (hci_info) {
		if (hci_info->hdev) {
			err = hci_unregister_dev(hci_info->hdev);
			if (err) {
				STE_CONN_ERR("Can not unregister HCI device (%d)", err);
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
MODULE_DESCRIPTION("Linux Bluetooth HCI H:4 Driver for ST-Ericsson controller");
