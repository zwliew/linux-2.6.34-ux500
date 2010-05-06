/*
 * drivers/mfd/ste_conn/ste_stlc2690.c
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Authors:
 * Par-Gunnar Hjalmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Henrik Possung (henrik.possung@stericsson.com) for ST-Ericsson.
 * Josef Kindberg (josef.kindberg@stericsson.com) for ST-Ericsson.
 * Dariusz Szymszak (dariusz.xd.szymczak@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson. * License terms:  GNU General Public License (GPL), version 2
 *
 * Linux Bluetooth HCI H:4 Driver for ST-Ericsson connectivity controller STLC2690.
 */

#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/skbuff.h>
#include <linux/gfp.h>
#include <linux/stat.h>
#include <linux/types.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/mutex.h>
#include <linux/list.h>

#include <linux/mfd/ste_conn.h>
#include <mach/ste_conn_devices.h>
#include "bluetooth_defines.h"
#include "ste_stlc2690.h"
#include "ste_conn_cpd.h"
#include "ste_conn_ccd.h"
#include "ste_conn_debug.h"

#define STLC2690_LINE_BUFFER_LENGTH		128
#define STLC2690_FILENAME_MAX			128

#define STLC2690_NAME				"ste_conn_stlc2690\0"
#define STLC2690_WQ_NAME			"ste_conn_stlc2690_wq\0"
#define STLC2690_PATCH_INFO_FILE		"ste_conn_patch_info.fw"
#define STLC2690_FACTORY_SETTINGS_INFO_FILE	"ste_conn_settings_info.fw"

/* Supported chips */
#define STLC2690_SUPP_MANUFACTURER		0x30
#define STLC2690_SUPP_REVISION_MIN		0x0500
#define STLC2690_SUPP_REVISION_MAX		0x06FF

/* Size of file chunk ID */
#define STLC2690_FILE_CHUNK_ID_SIZE			1
#define STLC2690_VS_SEND_FILE_START_OFFSET_IN_CMD	4
#define STLC2690_BT_CMD_LENGTH_POSITION			3

/* State setting macros */
#define STLC2690_SET_BOOT_STATE(__stlc2690_new_state) \
	STE_CONN_SET_STATE("boot_state", stlc2690_info->boot_state, __stlc2690_new_state)
#define STLC2690_SET_BOOT_FILE_LOAD_STATE(__stlc2690_new_state) \
	STE_CONN_SET_STATE("boot_file_load_state", stlc2690_info->boot_file_load_state, __stlc2690_new_state)
#define STLC2690_SET_BOOT_DOWNLOAD_STATE(__stlc2690_new_state) \
	STE_CONN_SET_STATE("boot_download_state", stlc2690_info->boot_download_state, __stlc2690_new_state)

/** STLC2690_H4_CHANNEL_BT_CMD - Bluetooth HCI H:4 channel
 * for Bluetooth commands in the ST-Ericsson connectivity controller.
 */
#define STLC2690_H4_CHANNEL_BT_CMD		0x01

/** STLC2690_H4_CHANNEL_BT_ACL - Bluetooth HCI H:4 channel
 * for Bluetooth ACL data in the ST-Ericsson connectivity controller.
 */
#define STLC2690_H4_CHANNEL_BT_ACL		0x02

/** STLC2690_H4_CHANNEL_BT_EVT - Bluetooth HCI H:4 channel
 * for Bluetooth events in the ST-Ericsson connectivity controller.
 */
#define STLC2690_H4_CHANNEL_BT_EVT		0x04

/** STLC2690_H4_CHANNEL_HCI_LOGGER - Bluetooth HCI H:4 channel
 * for logging all transmitted H4 packets (on all channels).
 */
#define STLC2690_H4_CHANNEL_HCI_LOGGER		0xFA

/** STLC2690_H4_CHANNEL_US_CTRL - Bluetooth HCI H:4 channel
 * for user space control of the ST-Ericsson connectivity controller.
 */
#define STLC2690_H4_CHANNEL_US_CTRL		0xFC

/** STLC2690_H4_CHANNEL_CORE - Bluetooth HCI H:4 channel
 * for user space control of the ST-Ericsson connectivity controller.
 */
#define STLC2690_H4_CHANNEL_CORE		0xFD

/**
  * struct stlc2690_work_struct - Work structure for ste_conn CPD module.
  * @work: Work structure.
  * @skb:  Data packet.
  * @data: Private data for ste_conn CPD.
  *
  * This structure is used to pack work for work queue.
  */
struct stlc2690_work_struct{
	struct work_struct work;
	struct sk_buff *skb;
	void *data;
};

/**
  * enum stlc2690_boot_state - BOOT-state for CPD.
  * @BOOT_STATE_NOT_STARTED:                    Boot has not yet started.
  * @BOOT_STATE_SEND_BD_ADDRESS:		VS Store In FS command with bd address has been sent.
  * @BOOT_STATE_GET_FILES_TO_LOAD:              CPD is retreiving file to load.
  * @BOOT_STATE_DOWNLOAD_PATCH:                 CPD is downloading patches.
  * @BOOT_STATE_ACTIVATE_PATCHES_AND_SETTINGS:  CPD is activating patches and settings.
  * @BOOT_STATE_READY:                          CPD boot is ready.
  * @BOOT_STATE_FAILED:                         CPD boot failed.
  */
enum stlc2690_boot_state {
	BOOT_STATE_NOT_STARTED,
	BOOT_STATE_SEND_BD_ADDRESS,
	BOOT_STATE_GET_FILES_TO_LOAD,
	BOOT_STATE_DOWNLOAD_PATCH,
	BOOT_STATE_ACTIVATE_PATCHES_AND_SETTINGS,
	BOOT_STATE_READY,
	BOOT_STATE_FAILED
};

/**
  * enum stlc2690_boot_file_load_state - BOOT_FILE_LOAD-state for CPD.
  * @BOOT_FILE_LOAD_STATE_LOAD_PATCH:           Loading patches.
  * @BOOT_FILE_LOAD_STATE_LOAD_STATIC_SETTINGS: Loading static settings.
  * @BOOT_FILE_LOAD_STATE_NO_MORE_FILES:        No more files to load.
  * @BOOT_FILE_LOAD_STATE_FAILED:               File loading failed.
  */
enum stlc2690_boot_file_load_state {
	BOOT_FILE_LOAD_STATE_LOAD_PATCH,
	BOOT_FILE_LOAD_STATE_LOAD_STATIC_SETTINGS,
	BOOT_FILE_LOAD_STATE_NO_MORE_FILES,
	BOOT_FILE_LOAD_STATE_FAILED
};

/**
  * enum stlc2690_boot_download_state - BOOT_DOWNLOAD state.
  * @BOOT_DOWNLOAD_STATE_PENDING: Download in progress.
  * @BOOT_DOWNLOAD_STATE_SUCCESS: Download successfully finished.
  * @BOOT_DOWNLOAD_STATE_FAILED:  Downloading failed.
  */
enum stlc2690_boot_download_state {
	BOOT_DOWNLOAD_STATE_PENDING,
	BOOT_DOWNLOAD_STATE_SUCCESS,
	BOOT_DOWNLOAD_STATE_FAILED
};

/**
  * struct stlc2690_device_id - Structure for connecting H4 channel to named user.
  * @name:        Name of device.
  * @h4_channel:  HCI H:4 channel used by this device.
  */
struct stlc2690_device_id {
	char *name;
	int   h4_channel;
};

/**
  * struct stlc2690_info - Main info structure for STLC2690.
  * @patch_file_name:		Stores patch file name.
  * @settings_file_name:	Stores settings file name.
  * @fw_file:			Stores firmware file (patch or settings).
  * @fw_file_rd_offset:		Current read offset in firmware file.
  * @chunk_id:			Stores current chunk ID of write file operations.
  * @boot_state:		Current BOOT-state of STLC2690.
  * @boot_file_load_state:	Current BOOT_FILE_LOAD-state of STLC2690.
  * @boot_download_state:	Current BOOT_DOWNLOAD-state of STLC2690.
  * @wq:			STLC2690 workqueue.
  * @chip_dev:			Chip info.
  */
struct stlc2690_info {
	char					*patch_file_name;
	char					*settings_file_name;
	const struct firmware			*fw_file;
	int					fw_file_rd_offset;
	uint8_t					chunk_id;
	enum stlc2690_boot_state		boot_state;
	enum stlc2690_boot_file_load_state	boot_file_load_state;
	enum stlc2690_boot_download_state	boot_download_state;
	struct workqueue_struct			*wq;
	struct ste_conn_cpd_chip_dev		chip_dev;
};

static struct stlc2690_info *stlc2690_info;

/*
  * stlc2690_msg_reset_cmd_req - Hardcoded HCI Reset command.
  */
static const uint8_t stlc2690_msg_reset_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	HCI_BT_MAKE_FIRST_BYTE_IN_CMD(HCI_BT_OCF_RESET),
	HCI_BT_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_CTRL_BB, HCI_BT_OCF_RESET),
	0x00
};

/*
  * stlc2690_msg_vs_store_in_fs_cmd_req - Hardcoded HCI Store in FS command.
  */
static const uint8_t stlc2690_msg_vs_store_in_fs_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	HCI_BT_MAKE_FIRST_BYTE_IN_CMD(STLC2690_BT_OCF_VS_STORE_IN_FS),
	HCI_BT_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, STLC2690_BT_OCF_VS_STORE_IN_FS),
	0x00, /* 1 byte for HCI command length. */
	0x00, /* 1 byte for User_Id. */
	0x00  /* 1 byte for vs_store_in_fs_cmd_req parameter data length. */
};

/*
  * stlc2690_msg_vs_write_file_block_cmd_req -
  * Hardcoded HCI Write File Block vendor specific command
  */
static const uint8_t stlc2690_msg_vs_write_file_block_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	HCI_BT_MAKE_FIRST_BYTE_IN_CMD(STLC2690_BT_OCF_VS_WRITE_FILE_BLOCK),
	HCI_BT_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, STLC2690_BT_OCF_VS_WRITE_FILE_BLOCK),
	0x00
};

#define STLC2690_NBR_OF_DEVS 6

/*
 * stlc2690_channels() - Array containing available H4 channels for the STLC2690
 * ST-Ericsson Connectivity controller.
 */
struct stlc2690_device_id stlc2690_channels[STLC2690_NBR_OF_DEVS] = {
	{STE_CONN_DEVICES_BT_CMD,		STLC2690_H4_CHANNEL_BT_CMD},
	{STE_CONN_DEVICES_BT_ACL,		STLC2690_H4_CHANNEL_BT_ACL},
	{STE_CONN_DEVICES_BT_EVT,		STLC2690_H4_CHANNEL_BT_EVT},
	{STE_CONN_DEVICES_HCI_LOGGER,		STLC2690_H4_CHANNEL_HCI_LOGGER},
	{STE_CONN_DEVICES_US_CTRL,		STLC2690_H4_CHANNEL_US_CTRL},
	{STE_CONN_DEVICES_CORE,			STLC2690_H4_CHANNEL_CORE}
};

/*
 * Internal function declarations
 */
static int stlc2690_chip_startup(struct ste_conn_cpd_chip_dev *dev);
static bool stlc2690_data_from_chip(struct ste_conn_cpd_chip_dev *dev, struct ste_conn_device *cpd_dev, struct sk_buff *skb);
static int stlc2690_get_h4_channel(char *name, int *h4_channel);
static bool stlc2690_check_chip_support(struct ste_conn_cpd_chip_dev *dev);
static bool stlc2690_handle_internal_rx_data_bt_evt(struct sk_buff *skb);
static void stlc2690_create_work_item(work_func_t work_func, struct sk_buff *skb, void *data);
static bool stlc2690_handle_reset_cmd_complete_evt(uint8_t *data);
static bool stlc2690_handle_vs_store_in_fs_cmd_complete_evt(uint8_t *data);
static bool stlc2690_handle_vs_write_file_block_cmd_complete_evt(uint8_t *data);
static void stlc2690_work_reset_after_error(struct work_struct *work);
static void stlc2690_work_load_patch_and_settings(struct work_struct *work);
static void stlc2690_work_continue_with_file_download(struct work_struct *work);
static bool stlc2690_get_file_to_load(const struct firmware *fw, char **file_name_p);
static char *stlc2690_get_one_line_of_text(char *wr_buffer, int max_nbr_of_bytes,
					char *rd_buffer, int *bytes_copied);
static void stlc2690_read_and_send_file_part(void);
static void stlc2690_send_patch_file(void);
static void stlc2690_send_settings_file(void);
static void stlc2690_create_and_send_bt_cmd(const uint8_t *data, int length);
static void stlc2690_send_bd_address(void);

static struct ste_conn_cpd_id_callbacks stlc2690_id_callbacks = {
	.check_chip_support = stlc2690_check_chip_support
};


/*
 * Internal functions
 */

/**
 * stlc2690_chip_startup() - Start the chip.
 * @dev:	Chip info.
 *
 * The stlc2690_chip_startup() function downloads patches and other needed
 * start procedures.
 *
 * Returns:
 *   0 if there is no error.
 */
static int stlc2690_chip_startup(struct ste_conn_cpd_chip_dev *dev)
{
	int err = 0;

	/* Start the boot sequence */
	STLC2690_SET_BOOT_STATE(BOOT_STATE_GET_FILES_TO_LOAD);
	stlc2690_create_work_item(stlc2690_work_load_patch_and_settings, NULL, NULL);

	return err;
}

/**
 * stlc2690_data_from_chip() - Called when data shall be sent to the chip.
 * @dev:	Chip info.
 * @cpd_dev:	STE_CONN user for this packet.
 * @skb:	Packet received.
 *
 * The stlc2690_data_from_chip() function checks if packet is a response for a
 * packet it itself has transmitted.
 *
 * Returns:
 *   true if packet is handled by this driver.
 *   false otherwise.
 */
static bool stlc2690_data_from_chip(struct ste_conn_cpd_chip_dev *dev,
				  struct ste_conn_device *cpd_dev,
				  struct sk_buff *skb)
{
	bool packet_handled = false;
	int h4_channel;

	if (cpd_dev) {
		h4_channel = cpd_dev->h4_channel;
	} else {
		/* This means that this was done during start-up and the H:4
		 * header should be valid (this can't be e.g. a BT Audio user).
		 */
		h4_channel = skb->data[0];
	}

	/* Then check if this is a response to data we have sent */
	packet_handled = stlc2690_handle_internal_rx_data_bt_evt(skb);

	return packet_handled;
}

/**
 * stlc2690_get_h4_channel() - Returns H:4 channel for the name.
 * @name:	Chip info.
 * @h4_channel:	STE_CONN user for this packet.
 *
 * Returns:
 *   0 if there is no error.
 *   -ENXIO if channel is not found.
 */
static int stlc2690_get_h4_channel(char *name, int *h4_channel)
{
	int i;
	int err = -ENXIO;

	*h4_channel = -1;

	for (i = 0; *h4_channel == -1 && i < STLC2690_NBR_OF_DEVS; i++) {
		if (0 == strncmp(name, stlc2690_channels[i].name, STE_CONN_MAX_NAME_SIZE)) {
			/* Device found. Return H4 channel */
			*h4_channel = stlc2690_channels[i].h4_channel;
			err = 0;
		}
	}

	return err;
}

/**
 * stlc2690_check_chip_support() - Checks if connected chip is handled by this driver.
 * @h4_channel:	H:4 channel for this packet.
 * @skb:	Packet to check.
 *
 * If supported return true and fill in @callbacks.
 *
 * Returns:
 *   true if chip is handled by this driver.
 *   false otherwise.
 */
static bool stlc2690_check_chip_support(struct ste_conn_cpd_chip_dev *dev)
{
	bool ret_val = false;

	STE_CONN_INFO("stlc2690_check_chip_support");

	/* Check if this is a CG2690 revision. We do not care about the sub-version at the moment.
	 * Change this if necessary */
	if ((dev->chip.manufacturer != STLC2690_SUPP_MANUFACTURER) ||
	    (dev->chip.hci_revision < STLC2690_SUPP_REVISION_MIN) ||
	    (dev->chip.hci_revision > STLC2690_SUPP_REVISION_MAX)) {
		STE_CONN_DBG("Chip not supported by STLC2690 driver\n\tMan: 0x%02X\n\tRev: 0x%04X\n\tSub: 0x%04X",
				dev->chip.manufacturer, dev->chip.hci_revision, dev->chip.hci_sub_version);
		goto finished;
	}

	STE_CONN_INFO("Chip supported by the STLC2690 driver");
	ret_val = true;
	/* Store needed data */
	dev->user_data = stlc2690_info;
	memcpy(&(stlc2690_info->chip_dev), dev, sizeof(*dev));
	/* Set the callbacks */
	dev->callbacks.chip_startup = stlc2690_chip_startup;
	dev->callbacks.data_from_chip = stlc2690_data_from_chip;
	dev->callbacks.get_h4_channel = stlc2690_get_h4_channel;

finished:
	return ret_val;
}

/**
 * stlc2690_handle_internal_rx_data_bt_evt() - Check if received data should be handled in CPD.
 * @skb: Data packet
 * The stlc2690_handle_internal_rx_data_bt_evt() function checks if received data should be handled in CPD.
 * If so handle it correctly. Received data is always HCI BT Event.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool stlc2690_handle_internal_rx_data_bt_evt(struct sk_buff *skb)
{
	bool pkt_handled = false;
	uint8_t *data = &(skb->data[STE_CONN_SKB_RESERVE]);
	uint8_t event_code = data[0];

	/* First check the event code */
	if (HCI_BT_EVT_CMD_COMPLETE == event_code) {
		uint8_t hci_ogf = HCI_BT_OPCODE_TO_OGF(data[4]);
		uint16_t hci_ocf = HCI_BT_OPCODE_TO_OCF(data[3], data[4]);

		STE_CONN_DBG_DATA("Command complete: OGF = 0x%02X, OCF = 0x%04X",
				  hci_ogf, hci_ocf);
		data += 5; /* Move to first byte after OCF */

		switch (hci_ogf) {
		case HCI_BT_OGF_LINK_CTRL:
			break;

		case HCI_BT_OGF_LINK_POLICY:
			break;

		case HCI_BT_OGF_CTRL_BB:
			switch (hci_ocf) {
			case HCI_BT_OCF_RESET:
				pkt_handled = stlc2690_handle_reset_cmd_complete_evt(data);
				break;

			default:
				break;
			}; /* switch (hci_ocf) */
			break;

		case HCI_BT_OGF_LINK_INFO:
			break;

		case HCI_BT_OGF_LINK_STATUS:
			break;

		case HCI_BT_OGF_LINK_TESTING:
			break;

		case HCI_BT_OGF_VS:
			switch (hci_ocf) {
			case STLC2690_BT_OCF_VS_STORE_IN_FS:
				pkt_handled = stlc2690_handle_vs_store_in_fs_cmd_complete_evt(data);
				break;
			case STLC2690_BT_OCF_VS_WRITE_FILE_BLOCK:
				pkt_handled = stlc2690_handle_vs_write_file_block_cmd_complete_evt(data);
				break;
			default:
				break;
			}; /* switch (hci_ocf) */
			break;

		default:
			break;
		}; /* switch (hci_ogf) */
	}

	if (pkt_handled && skb) {
		kfree_skb(skb);
	}
	return pkt_handled;
}

/**
 * stlc2690_create_work_item() - Create work item and add it to the work queue.
 * @work_func:	Work function.
 * @skb: 	Data packet.
 * @data: 	Private data for ste_conn CPD.
 *
 * The stlc2690_create_work_item() function creates work item and
 * add it to the work queue.
 */
static void stlc2690_create_work_item(work_func_t work_func, struct sk_buff *skb, void *data)
{
	struct stlc2690_work_struct *new_work;
	int wq_err = 1;

	new_work = kmalloc(sizeof(*new_work), GFP_ATOMIC);

	if (new_work) {
		new_work->skb = skb;
		new_work->data = data;
		INIT_WORK(&new_work->work, work_func);

		wq_err = queue_work(stlc2690_info->wq, &new_work->work);

		if (!wq_err) {
			STE_CONN_ERR("Failed to queue work_struct because it's already in the queue!");
			kfree(new_work);
		}
	} else {
		STE_CONN_ERR("Failed to alloc memory for stlc2690_work_struct!");
	}
}

/**
 * stlc2690_handle_reset_cmd_complete_evt() - Handle a received HCI Command Complete event for a Reset command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool stlc2690_handle_reset_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;
	uint8_t status = data[0];

	STE_CONN_INFO("Received Reset complete event with status 0x%X", status);

	if (stlc2690_info->boot_state == BOOT_STATE_ACTIVATE_PATCHES_AND_SETTINGS) {
		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* The boot sequence is now finished successfully.
			 * Set states and signal to waiting thread.
			 */
			STLC2690_SET_BOOT_STATE(BOOT_STATE_READY);
			ste_conn_cpd_chip_startup_finished(0);
		} else {
			STE_CONN_ERR("Received Reset complete event with status 0x%X", status);
			STLC2690_SET_BOOT_STATE(BOOT_STATE_FAILED);
			ste_conn_cpd_chip_startup_finished(-EIO);
		}
		pkt_handled = true;
	}

	return pkt_handled;
}

/**
 * stlc2690_handle_vs_store_in_fs_cmd_complete_evt() - Handle a received HCI Command Complete event
 * for a VS StoreInFS command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool stlc2690_handle_vs_store_in_fs_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;
	uint8_t status = data[0];

	STE_CONN_INFO("Received Store_in_FS complete event with status 0x%X", status);

	if (stlc2690_info->boot_state == BOOT_STATE_SEND_BD_ADDRESS) {
		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* Send HCI SystemReset command to activate patches */
			STLC2690_SET_BOOT_STATE(BOOT_STATE_ACTIVATE_PATCHES_AND_SETTINGS);
			stlc2690_create_and_send_bt_cmd(stlc2690_msg_reset_cmd_req,
						      sizeof(stlc2690_msg_reset_cmd_req));
		} else {
			STE_CONN_ERR("Command Complete for StoreInFS received with error 0x%X", status);
			STLC2690_SET_BOOT_STATE(BOOT_STATE_FAILED);
			stlc2690_create_work_item(stlc2690_work_reset_after_error, NULL, NULL);
		}
		/* We have now handled the packet */
		pkt_handled = true;
	}

	return pkt_handled;
}

/**
 * stlc2690_handle_vs_write_file_block_cmd_complete_evt() - Handle a received HCI Command Complete event
 * for a VS WriteFileBlock command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool stlc2690_handle_vs_write_file_block_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;

	if ((stlc2690_info->boot_state == BOOT_STATE_DOWNLOAD_PATCH) &&
	    (stlc2690_info->boot_download_state == BOOT_DOWNLOAD_STATE_PENDING)) {
		uint8_t status = data[0];
		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* Received good confirmation. Start work to continue. */
			stlc2690_create_work_item(stlc2690_work_continue_with_file_download, NULL, NULL);
		} else {
			STE_CONN_ERR("Command Complete for WriteFileBlock received with error 0x%X", status);
			STLC2690_SET_BOOT_DOWNLOAD_STATE(BOOT_DOWNLOAD_STATE_FAILED);
			STLC2690_SET_BOOT_STATE(BOOT_STATE_FAILED);
			if (stlc2690_info->fw_file) {
				release_firmware(stlc2690_info->fw_file);
				stlc2690_info->fw_file = NULL;
			}
			stlc2690_create_work_item(stlc2690_work_reset_after_error, NULL, NULL);
		}
		/* We have now handled the packet */
		pkt_handled = true;
	}

	return pkt_handled;
}

/**
 * stlc2690_work_reset_after_error() - Handle reset.
 * @work: Reference to work data.
 *
 * Handle a reset after received command complete event.
 */
static void stlc2690_work_reset_after_error(struct work_struct *work)
{
	struct stlc2690_work_struct *current_work = NULL;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct stlc2690_work_struct, work);

	ste_conn_cpd_chip_startup_finished(-EIO);

	kfree(current_work);
}

/**
 * stlc2690_work_load_patch_and_settings() - Start loading patches and settings.
 * @work: Reference to work data.
 */
static void stlc2690_work_load_patch_and_settings(struct work_struct *work)
{
	struct stlc2690_work_struct *current_work = NULL;
	int err = 0;
	bool file_found;
	const struct firmware *patch_info;
	const struct firmware *settings_info;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct stlc2690_work_struct, work);

	/* Check that we are in the right state */
	if (stlc2690_info->boot_state == BOOT_STATE_GET_FILES_TO_LOAD) {
		/* Open patch info file. */
		err = request_firmware(&patch_info, STLC2690_PATCH_INFO_FILE, stlc2690_info->chip_dev.dev);
		if (err) {
			STE_CONN_ERR("Couldn't get patch info file");
			goto error_handling;
		}

		/* Now we have the patch info file. See if we can find the right patch file as well */
		file_found = stlc2690_get_file_to_load(patch_info, &(stlc2690_info->patch_file_name));

		/* Now we are finished with the patch info file */
		release_firmware(patch_info);

		if (!file_found) {
			STE_CONN_ERR("Couldn't find patch file! Major error!");
			goto error_handling;
		}

		/* Open settings info file. */
		err = request_firmware(&settings_info, STLC2690_FACTORY_SETTINGS_INFO_FILE, stlc2690_info->chip_dev.dev);
		if (err) {
			STE_CONN_ERR("Couldn't get settings info file");
			goto error_handling;
		}

		/* Now we have the settings info file. See if we can find the right settings file as well */
		file_found = stlc2690_get_file_to_load(settings_info, &(stlc2690_info->settings_file_name));

		/* Now we are finished with the patch info file */
		release_firmware(settings_info);

		if (!file_found) {
			STE_CONN_ERR("Couldn't find settings file! Major error!");
			goto error_handling;
		}

		/* We now all info needed */
		STLC2690_SET_BOOT_STATE(BOOT_STATE_DOWNLOAD_PATCH);
		STLC2690_SET_BOOT_DOWNLOAD_STATE(BOOT_DOWNLOAD_STATE_PENDING);
		STLC2690_SET_BOOT_FILE_LOAD_STATE(BOOT_FILE_LOAD_STATE_LOAD_PATCH);
		stlc2690_info->chunk_id = 0;
		stlc2690_info->fw_file_rd_offset = 0;
		stlc2690_info->fw_file = NULL;

		/* OK. Now it is time to download the patches */
		err = request_firmware(&(stlc2690_info->fw_file), stlc2690_info->patch_file_name, stlc2690_info->chip_dev.dev);
		if (err < 0) {
			STE_CONN_ERR("Couldn't get patch file");
			goto error_handling;
		}
		stlc2690_send_patch_file();
	}
	goto finished;

error_handling:
	STLC2690_SET_BOOT_STATE(BOOT_STATE_FAILED);
	ste_conn_cpd_chip_startup_finished(-EIO);
finished:
	kfree(current_work);
}

/**
 * stlc2690_work_continue_with_file_download() - A file block has been written.
 * @work: Reference to work data.
 *
 * Handle a received HCI VS Write File Block Complete event.
 * Normally this means continue to send files to the controller.
 */
static void stlc2690_work_continue_with_file_download(struct work_struct *work)
{
	struct stlc2690_work_struct *current_work = NULL;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct stlc2690_work_struct, work);

	/* Continue to send patches or settings to the controller */
	if (stlc2690_info->boot_file_load_state == BOOT_FILE_LOAD_STATE_LOAD_PATCH) {
		stlc2690_send_patch_file();
	} else if (stlc2690_info->boot_file_load_state == BOOT_FILE_LOAD_STATE_LOAD_STATIC_SETTINGS) {
		stlc2690_send_settings_file();
	} else {
		STE_CONN_INFO("No more files to load");
	}

	kfree(current_work);
}

/**
 * stlc2690_get_file_to_load() - Parse info file and find correct target file.
 * @fw:              Firmware structure containing file data.
 * @file_name: (out) Pointer to name of requested file.
 *
 * Returns:
 *   True,  if target file was found,
 *   False, otherwise.
 */
static bool stlc2690_get_file_to_load(const struct firmware *fw, char **file_name)
{
	char *line_buffer;
	char *curr_file_buffer;
	int bytes_left_to_parse = fw->size;
	int bytes_read = 0;
	bool file_found = false;

	curr_file_buffer = (char *)&(fw->data[0]);

	line_buffer = kzalloc(STLC2690_LINE_BUFFER_LENGTH, GFP_ATOMIC);

	if (line_buffer) {
		while (!file_found) {
			/* Get one line of text from the file to parse */
			curr_file_buffer = stlc2690_get_one_line_of_text(line_buffer,
								    min(STLC2690_LINE_BUFFER_LENGTH,
									(int)(fw->size - bytes_read)),
								    curr_file_buffer,
								    &bytes_read);

			bytes_left_to_parse -= bytes_read;
			if (bytes_left_to_parse <= 0) {
				/* End of file => Leave while loop */
				STE_CONN_ERR("Reached end of file. No file found!");
				break;
			}

			/* Check if the line of text is a comment or not, comments begin with '#' */
			if (*line_buffer != '#') {
				uint32_t     hci_rev = 0;
				uint32_t     lmp_sub = 0;

				STE_CONN_DBG("Found a valid line <%s>", line_buffer);

				/* Check if we can find the correct HCI revision and LMP subversion
				 * as well as a file name in the text line
				 * Store the filename if the actual file can be found in the file system
				 */
				if (sscanf(line_buffer, "%x%x%s", &hci_rev, &lmp_sub, *file_name) == 3
				    && hci_rev == stlc2690_info->chip_dev.chip.hci_revision
				    && lmp_sub == stlc2690_info->chip_dev.chip.hci_sub_version) {
					STE_CONN_DBG( \
					"File name = %s HCI Revision = 0x%X LMP PAL Subversion = 0x%X", \
					*file_name, hci_rev, lmp_sub);

					/* Name has already been stored above. Nothing more to do */
					file_found = true;
				} else {
					/* Zero the name buffer so it is clear to next read */
					memset(*file_name, 0x00, STLC2690_FILENAME_MAX + 1);
				}
			}
		}
		kfree(line_buffer);
	} else {
		STE_CONN_ERR("Failed to allocate: file_name 0x%X, line_buffer 0x%X",
					(uint32_t)file_name, (uint32_t)line_buffer);
	}

	return file_found;
}

/**
 * stlc2690_get_one_line_of_text()- Replacement function for stdio function fgets.
 * @wr_buffer:         Buffer to copy text to.
 * @max_nbr_of_bytes: Max number of bytes to read, i.e. size of rd_buffer.
 * @rd_buffer:        Data to parse.
 * @bytes_copied:     Number of bytes copied to wr_buffer.
 *
 * The stlc2690_get_one_line_of_text() function extracts one line of text from input file.
 *
 * Returns:
 *   Pointer to next data to read.
 */
static char *stlc2690_get_one_line_of_text(char *wr_buffer, int max_nbr_of_bytes,
					char *rd_buffer, int *bytes_copied)
{
	char *curr_wr = wr_buffer;
	char *curr_rd = rd_buffer;
	char in_byte;

	*bytes_copied = 0;

	do {
		*curr_wr = *curr_rd;
		in_byte = *curr_wr;
		curr_wr++;
		curr_rd++;
		(*bytes_copied)++;
	} while ((*bytes_copied <= max_nbr_of_bytes) && (in_byte != '\0') && (in_byte != '\n'));
	*curr_wr = '\0';
	return curr_rd;
}

/**
 * stlc2690_read_and_send_file_part() - Transmit a part of the supplied file to the controller
 *
 * The stlc2690_read_and_send_file_part() function transmit a part of the supplied file to the controller.
 * If nothing more to read, set the correct states.
 */
static void stlc2690_read_and_send_file_part(void)
{
	int bytes_to_copy;

	/* Calculate number of bytes to copy;
	 * either max bytes for HCI packet or number of bytes left in file
	 */
	bytes_to_copy = min((int)HCI_BT_SEND_FILE_MAX_CHUNK_SIZE,
			    (int)(stlc2690_info->fw_file->size - stlc2690_info->fw_file_rd_offset));

	if (bytes_to_copy > 0) {
		struct sk_buff *skb;
		struct ste_conn_cpd_hci_logger_config *logger_config = ste_conn_cpd_get_hci_logger_config();

		/* There are bytes to transmit. Allocate a sk_buffer. */
		skb = alloc_skb(sizeof(stlc2690_msg_vs_write_file_block_cmd_req) +
				HCI_BT_SEND_FILE_MAX_CHUNK_SIZE +
				STLC2690_FILE_CHUNK_ID_SIZE, GFP_ATOMIC);
		if (!skb) {
			STE_CONN_ERR("Couldn't allocate sk_buffer");
			STLC2690_SET_BOOT_STATE(BOOT_STATE_FAILED);
			ste_conn_cpd_chip_startup_finished(-EIO);
			return;
		}

		/* Copy the data from the HCI template */
		memcpy(skb_put(skb, sizeof(stlc2690_msg_vs_write_file_block_cmd_req)),
		       stlc2690_msg_vs_write_file_block_cmd_req,
		       sizeof(stlc2690_msg_vs_write_file_block_cmd_req));
		skb->data[0] = STLC2690_H4_CHANNEL_BT_CMD;

		/* Store the chunk ID */
		skb->data[STLC2690_VS_SEND_FILE_START_OFFSET_IN_CMD] = stlc2690_info->chunk_id;
		skb_put(skb, 1);

		/* Copy the data from offset position and store the length */
		memcpy(skb_put(skb, bytes_to_copy),
		       &(stlc2690_info->fw_file->data[stlc2690_info->fw_file_rd_offset]),
		       bytes_to_copy);
		skb->data[STLC2690_BT_CMD_LENGTH_POSITION] = bytes_to_copy + STLC2690_FILE_CHUNK_ID_SIZE;

		/* Increase offset with number of bytes copied */
		stlc2690_info->fw_file_rd_offset += bytes_to_copy;

		if (logger_config) {
			ste_conn_cpd_send_to_chip(skb, logger_config->bt_cmd_enable);
		} else {
			ste_conn_cpd_send_to_chip(skb, false);
		}
		stlc2690_info->chunk_id++;
	} else {
		/* Nothing more to read in file. */
		STLC2690_SET_BOOT_DOWNLOAD_STATE(BOOT_DOWNLOAD_STATE_SUCCESS);
		stlc2690_info->chunk_id = 0;
		stlc2690_info->fw_file_rd_offset = 0;
	}
}

/**
 * stlc2690_send_patch_file - Transmit patch file.
 *
 * The stlc2690_send_patch_file() function transmit patch file. The file is
 * read in parts to fit in HCI packets. When the complete file is transmitted,
 * the file is closed. When finished, continue with settings file.
 */
static void stlc2690_send_patch_file(void)
{
	int err = 0;

	/* Transmit a part of the supplied file to the controller.
	 * When nothing more to read, continue to close the patch file. */
	stlc2690_read_and_send_file_part();

	if (stlc2690_info->boot_download_state == BOOT_DOWNLOAD_STATE_SUCCESS) {
		/* Patch file finished. Release used resources */
		STE_CONN_DBG("Patch file finished, release used resources");
		if (stlc2690_info->fw_file) {
			release_firmware(stlc2690_info->fw_file);
			stlc2690_info->fw_file = NULL;
		}
		err = request_firmware(&(stlc2690_info->fw_file), stlc2690_info->settings_file_name, stlc2690_info->chip_dev.dev);
		if (err < 0) {
			STE_CONN_ERR("Couldn't get settings file (%d)", err);
			goto error_handling;
		}
		/* Now send the settings file */
		STLC2690_SET_BOOT_FILE_LOAD_STATE(BOOT_FILE_LOAD_STATE_LOAD_STATIC_SETTINGS);
		STLC2690_SET_BOOT_DOWNLOAD_STATE(BOOT_DOWNLOAD_STATE_PENDING);
		stlc2690_send_settings_file();
	}
	return;

error_handling:
	STLC2690_SET_BOOT_STATE(BOOT_STATE_FAILED);
	ste_conn_cpd_chip_startup_finished(err);
}

/**
 * stlc2690_send_settings_file() - Transmit settings file.
 *
 * The stlc2690_send_settings_file() function transmit settings file.
 * The file is read in parts to fit in HCI packets. When finished,
 * close the settings file and send HCI reset to activate settings and patches.
 */
static void stlc2690_send_settings_file(void)
{
	/* Transmit a file part */
	stlc2690_read_and_send_file_part();

	if (stlc2690_info->boot_download_state == BOOT_DOWNLOAD_STATE_SUCCESS) {

		/* Settings file finished. Release used resources */
		STE_CONN_DBG("Settings file finished, release used resources");
		if (stlc2690_info->fw_file) {
			release_firmware(stlc2690_info->fw_file);
			stlc2690_info->fw_file = NULL;
		}

		STLC2690_SET_BOOT_FILE_LOAD_STATE(BOOT_FILE_LOAD_STATE_NO_MORE_FILES);

		/* Create and send HCI VS Store In FS command with bd address. */
		stlc2690_send_bd_address();
	}
}

/**
 * stlc2690_create_and_send_bt_cmd() - Copy and send sk_buffer.
 * @data:   Data to send.
 * @length: Length in bytes of data.
 *
 * The stlc2690_create_and_send_bt_cmd() function allocate sk_buffer, copy supplied data to it,
 * and send the sk_buffer to CCD. Note that the data must contain the H:4 header.
 */
static void stlc2690_create_and_send_bt_cmd(const uint8_t *data, int length)
{
	struct sk_buff *skb;
	struct ste_conn_cpd_hci_logger_config *logger_config;

	skb = alloc_skb(length, GFP_ATOMIC);
	if (skb) {
		int err = 0;

		memcpy(skb_put(skb, length), data, length);
		skb->data[0] = STLC2690_H4_CHANNEL_BT_CMD;

		logger_config = ste_conn_cpd_get_hci_logger_config();
		if (logger_config) {
			err = ste_conn_cpd_send_to_chip(skb, logger_config->bt_cmd_enable);
		} else {
			err = ste_conn_cpd_send_to_chip(skb, false);
		}
		if (err) {
			STE_CONN_ERR("Failed to transmit to chip (%d)", err);
			kfree_skb(skb);
		}
	} else {
		STE_CONN_ERR("Couldn't allocate sk_buff with length %d", length);
	}
}

/**
 * stlc2690_send_bd_address() - Send HCI VS command with BD address to the chip.
 */
static void stlc2690_send_bd_address(void)
{
	uint8_t tmp[sizeof(stlc2690_msg_vs_store_in_fs_cmd_req) + BT_BDADDR_SIZE];

	/* Send vs_store_in_fs_cmd with BD address. */
	memcpy(tmp, stlc2690_msg_vs_store_in_fs_cmd_req,
		sizeof(stlc2690_msg_vs_store_in_fs_cmd_req));

	tmp[HCI_BT_CMD_PARAM_LEN_POS] = STLC2690_VS_STORE_IN_FS_USR_ID_SIZE +
		STLC2690_VS_STORE_IN_FS_PARAM_LEN_SIZE + BT_BDADDR_SIZE;
	tmp[STLC2690_VS_STORE_IN_FS_USR_ID_POS] = STLC2690_VS_STORE_IN_FS_USR_ID_BD_ADDR;
	tmp[STLC2690_VS_STORE_IN_FS_PARAM_LEN_POS] = BT_BDADDR_SIZE;

	/* Now copy the bd address received from user space control app. */
	memcpy(&(tmp[STLC2690_VS_STORE_IN_FS_PARAM_POS]), ste_conn_bd_address, sizeof(ste_conn_bd_address));

	STLC2690_SET_BOOT_STATE(BOOT_STATE_SEND_BD_ADDRESS);

	stlc2690_create_and_send_bt_cmd(tmp, sizeof(tmp));
}

/**
 * stlc2690_init() - Initialize module.
 *
 * The ste_conn_init() function initialize the CG2690 driver,
 * then register to the CPD.
 *
 * Returns:
 *   0 if success.
 *   -ENOMEM for failed alloc or structure creation.
 *   Error codes generated by ste_conn_cpd_register_chip_driver.
 */
static int __init stlc2690_init(void)
{
	int err = 0;

	STE_CONN_INFO("stlc2690_init");

	stlc2690_info = kzalloc(sizeof(*stlc2690_info), GFP_ATOMIC);
	if (!stlc2690_info) {
		STE_CONN_ERR("Couldn't allocate stlc2690_info");
		err = -ENOMEM;
		goto finished;
	}

	stlc2690_info->wq = create_singlethread_workqueue(STLC2690_WQ_NAME);
	if (!stlc2690_info->wq) {
		STE_CONN_ERR("Could not create workqueue");
		err = -ENOMEM;
		goto err_handling_free_info;
	}

	/* Allocate file names that will be used, deallocated in ste_conn_cpd_exit */
	stlc2690_info->patch_file_name = kzalloc(STLC2690_FILENAME_MAX + 1, GFP_ATOMIC);
	if (!stlc2690_info->patch_file_name) {
		STE_CONN_ERR("Couldn't allocate name buffer for patch file.");
		err = -ENOMEM;
		goto err_handling_destroy_wq;
	}
		/* Allocate file names that will be used, deallocaded in ste_conn_cpd_exit  */
	stlc2690_info->settings_file_name = kzalloc(STLC2690_FILENAME_MAX + 1, GFP_ATOMIC);
	if (!stlc2690_info->settings_file_name) {
		STE_CONN_ERR("Couldn't allocate name buffers settings file.");
		err = -ENOMEM;
		goto err_handling_free_patch_name;
	}

	err = ste_conn_cpd_register_chip_driver(&stlc2690_id_callbacks);
	if (err) {
		STE_CONN_ERR("Couldn't register chip driver (%d)", err);
		goto err_handling_free_settings_name;
	}

	goto finished;

err_handling_free_settings_name:
	kfree(stlc2690_info->settings_file_name);
err_handling_free_patch_name:
	kfree(stlc2690_info->patch_file_name);
err_handling_destroy_wq:
	destroy_workqueue(stlc2690_info->wq);
err_handling_free_info:
	kfree(stlc2690_info);
	stlc2690_info = NULL;
finished:
	return err;
}

/**
 * stlc2690_exit() - Remove module.
 */
static void __exit stlc2690_exit(void)
{
	STE_CONN_INFO("stlc2690_exit");

	if (stlc2690_info) {
		kfree(stlc2690_info->settings_file_name);
		kfree(stlc2690_info->patch_file_name);
		destroy_workqueue(stlc2690_info->wq);
		kfree(stlc2690_info);
		stlc2690_info = NULL;
	}
}

module_init(stlc2690_init);
module_exit(stlc2690_exit);

MODULE_AUTHOR("Par-Gunnar Hjalmdahl ST-Ericsson");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Linux STLC2690 Connectivity Device Driver");
