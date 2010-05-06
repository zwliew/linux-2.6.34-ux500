/*
 * drivers/mfd/ste_conn/ste_conn_cpd.c
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
#include "ste_conn_cpd.h"
#include "ste_conn_ccd.h"
#include "ste_conn_char_devices.h"
#include "ste_conn_debug.h"
#include "bluetooth_defines.h"

#define STE_CONN_CPD_NAME			"ste_conn_cpd\0"
#define STE_CONN_CPD_WQ_NAME			"ste_conn_cpd_wq\0"

#define CPD_SET_MAIN_STATE(__cpd_new_state) \
	STE_CONN_SET_STATE("main_state", cpd_info->main_state, __cpd_new_state)
#define CPD_SET_BOOT_STATE(__cpd_new_state) \
	STE_CONN_SET_STATE("boot_state", cpd_info->boot_state, __cpd_new_state)

#define LOGGER_DIRECTION_TX 0
#define LOGGER_DIRECTION_RX 1


/*
 *	Internal type definitions
 */

/**
  * struct ste_conn_cpd_work_struct - Work structure for ste_conn CPD module.
  * @work:	Work structure.
  * @skb:	Data packet.
  * @data:	Private data for ste_conn CPD.
  *
  * This structure is used to pack work for work queue.
  */
struct ste_conn_cpd_work_struct{
	struct work_struct work;
	struct sk_buff *skb;
	void *data;
};

/**
  * enum cpd_main_state - Main-state for CPD.
  * @CPD_STATE_INITIALIZING:	CPD initializing.
  * @CPD_STATE_IDLE:		No user registered to CPD.
  * @CPD_STATE_BOOTING:		CPD booting after first user is registered.
  * @CPD_STATE_CLOSING:		CPD closing after last user has deregistered.
  * @CPD_STATE_RESETING:	CPD reset requested.
  * @CPD_STATE_ACTIVE:		CPD up and running with at least one user.
  */
enum cpd_main_state {
	CPD_STATE_INITIALIZING,
	CPD_STATE_IDLE,
	CPD_STATE_BOOTING,
	CPD_STATE_CLOSING,
	CPD_STATE_RESETING,
	CPD_STATE_ACTIVE
};

/**
  * enum cpd_boot_state - BOOT-state for CPD.
  * @BOOT_STATE_NOT_STARTED:			Boot has not yet started.
  * @BOOT_STATE_READ_LOCAL_VERSION_INFORMATION:	ReadLocalVersionInformation command has been sent.
  * @BOOT_STATE_READY:				CPD boot is ready.
  * @BOOT_STATE_FAILED:				CPD boot failed.
  */
enum cpd_boot_state {
	BOOT_STATE_NOT_STARTED,
	BOOT_STATE_READ_LOCAL_VERSION_INFORMATION,
	BOOT_STATE_READY,
	BOOT_STATE_FAILED
};

/**
  * struct ste_conn_cpd_users - Stores all current users of CPD.
  * @bt_cmd:		BT command channel user.
  * @bt_acl:		BT ACL channel user.
  * @bt_evt:		BT event channel user.
  * @fm_radio:		FM radio channel user.
  * @gnss GNSS:		GNSS channel user.
  * @debug Debug:	Internal debug channel user.
  * @ste_tools:		ST-E tools channel user.
  * @hci_logger:	HCI logger channel user.
  * @us_ctrl:		User space control channel user.
  * @bt_audio:		BT audio command channel user.
  * @fm_radio_audio:	FM audio command channel user.
  * @core:		Core command channel user.
  * @nbr_of_users:	Number of users currently registered (not including the HCI logger).
  */
struct ste_conn_cpd_users {
	struct ste_conn_device *bt_cmd;
	struct ste_conn_device *bt_acl;
	struct ste_conn_device *bt_evt;
	struct ste_conn_device *fm_radio;
	struct ste_conn_device *gnss;
	struct ste_conn_device *debug;
	struct ste_conn_device *ste_tools;
	struct ste_conn_device *hci_logger;
	struct ste_conn_device *us_ctrl;
	struct ste_conn_device *bt_audio;
	struct ste_conn_device *fm_radio_audio;
	struct ste_conn_device *core;
	int nbr_of_users;
};

/**
  * struct ste_conn_local_chip_info - Stores local controller info.
  * @version_set:		true if version data is valid.
  * @hci_version:		HCI version of local controller.
  * @hci_revision:		HCI revision of local controller.
  * @lmp_pal_version:		LMP/PAL version of local controller.
  * @manufacturer:		Manufacturer of local controller.
  * @lmp_pal_subversion:	LMP/PAL sub-version of local controller.
  *
  * According to Bluetooth HCI Read Local Version Information command.
  */
struct ste_conn_local_chip_info {
	bool		version_set;
	uint8_t		hci_version;
	uint16_t	hci_revision;
	uint8_t		lmp_pal_version;
	uint16_t	manufacturer;
	uint16_t	lmp_pal_subversion;
};

/**
 * struct chip_handler_item - Structure to store chip handler callbacks.
 * @list:	list_head struct.
 * @callbacks:	Chip handler callback struct.
 */
struct chip_handler_item {
	struct list_head list;
	struct ste_conn_cpd_id_callbacks callbacks;
};

/**
  * struct ste_conn_cpd_info - Main info structure for CPD.
  * @users:		Stores all users of CPD.
  * @local_chip_info:	Stores information of local controller.
  * @main_state:	Current Main-state of CPD.
  * @boot_state:	Current BOOT-state of CPD.
  * @wq:		CPD workqueue.
  * @hci_logger_config:	Stores HCI logger configuration.
  * @dev:		Device structure for STE Connectivity driver.
  * @chip_dev:		Device structure for chip driver.
  * @h4_channels:	HCI H:4 channel used by this device.
  */
struct ste_conn_cpd_info {
	struct ste_conn_cpd_users		users;
	struct ste_conn_local_chip_info		local_chip_info;
	enum cpd_main_state			main_state;
	enum cpd_boot_state			boot_state;
	struct workqueue_struct			*wq;
	struct ste_conn_cpd_hci_logger_config	hci_logger_config;
	struct device				*dev;
	struct ste_conn_cpd_chip_dev		chip_dev;
	struct ste_conn_ccd_h4_channels		h4_channels;
};

/*
 *	Internal variable declarations
 */

/*
 * cpd_info - Main information object for CPD.
 */
static struct ste_conn_cpd_info *cpd_info;

/*
 * cpd_chip_handlers - List of the register handlers for different chips.
 */
LIST_HEAD(cpd_chip_handlers);

/*
  * cpd_msg_reset_cmd_req - Hardcoded HCI Reset command.
  */
static const uint8_t cpd_msg_reset_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	HCI_BT_RESET_CMD_LSB,
	HCI_BT_RESET_CMD_MSB,
	0x00
};

/*
  * cpd_msg_read_local_version_information_cmd_req -
  * Hardcoded HCI Read Local Version Information command
  */
static const uint8_t cpd_msg_read_local_version_information_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	HCI_BT_READ_LOCAL_VERSION_CMD_LSB,
	HCI_BT_READ_LOCAL_VERSION_CMD_MSB,
	0x00
};

/*
  * ste_conn_cpd_wait_queue - Main Wait Queue in CPD.
  */
static DECLARE_WAIT_QUEUE_HEAD(ste_conn_cpd_wait_queue);

/*
  * time_15s - 15 second time struct.
  */
static struct timeval time_15s = {
	.tv_sec = 15,
	.tv_usec = 0
};

/*
  * time_500ms - 500 millisecond time struct.
  */
static struct timeval time_500ms = {
	.tv_sec = 0,
	.tv_usec = 500 * USEC_PER_MSEC
};

/*
  * time_100ms - 100 millisecond time struct.
  */
static struct timeval time_100ms = {
	.tv_sec = 0,
	.tv_usec = 100 * USEC_PER_MSEC
};

/*
  * time_50ms - 50 millisecond time struct.
  */
static struct timeval time_50ms = {
	.tv_sec = 0,
	.tv_usec = 50 * USEC_PER_MSEC
};

/*
 *	Internal function declarations
 */

static int  cpd_enable_hci_logger(struct sk_buff *skb);
static int  cpd_find_h4_user(int h4_channel, struct ste_conn_device **dev, const struct sk_buff * const skb);
static int  cpd_add_h4_user(struct ste_conn_device *dev, const char * const name);
static int  cpd_remove_h4_user(struct ste_conn_device **dev);
static void cpd_chip_startup(void);
static void cpd_chip_shutdown(void);
static bool cpd_handle_internal_rx_data_bt_evt(struct sk_buff *skb);
static bool cpd_handle_reset_cmd_complete_evt(uint8_t *data);
static bool cpd_handle_read_local_version_info_cmd_complete_evt(uint8_t *data);
static void cpd_create_and_send_bt_cmd(const uint8_t *data, int length);
static void cpd_transmit_skb_to_ccd(struct sk_buff *skb, bool use_logger);
static void cpd_handle_reset_of_user(struct ste_conn_device **dev);
static void cpd_free_user_dev(struct ste_conn_device **dev);

/*
 *	ste_conn API functions
 */

struct ste_conn_device *ste_conn_register(char  *name,
					struct ste_conn_callbacks *cb)
{
	struct ste_conn_device *current_dev;
	int err = 0;

	STE_CONN_INFO("ste_conn_register %s", name);

	if (!cpd_info) {
		STE_CONN_ERR("Hardware not started");
		return NULL;
	}

	/* Wait for state CPD_STATE_IDLE or CPD_STATE_ACTIVE. */
	if (wait_event_interruptible_timeout(ste_conn_cpd_wait_queue,
						((CPD_STATE_IDLE   == cpd_info->main_state) ||
						(CPD_STATE_ACTIVE == cpd_info->main_state)),
						timeval_to_jiffies(&time_50ms)) <= 0) {
		STE_CONN_ERR(
			"ste_conn_register currently busy (0x%X). Try again.",
			cpd_info->main_state);
		return NULL;
	}

	/* Allocate device */
	current_dev = kzalloc(sizeof(*current_dev), GFP_ATOMIC);
	if (current_dev) {
		if (!cpd_info->chip_dev.callbacks.get_h4_channel) {
			STE_CONN_ERR("No channel handler registered");
			err = -ENXIO;
			goto error_handling;
		}
		err = cpd_info->chip_dev.callbacks.get_h4_channel(name, &(current_dev->h4_channel));
		if (err) {
			STE_CONN_ERR("Couldn't find H4 channel for %s", name);
			goto error_handling;
		}
		current_dev->dev        = cpd_info->dev;
		current_dev->callbacks  = kmalloc(sizeof(*(current_dev->callbacks)), GFP_ATOMIC);
		if (current_dev->callbacks) {
			memcpy((char *)current_dev->callbacks, (char *)cb, sizeof(*(current_dev->callbacks)));
		} else {
			STE_CONN_ERR("Couldn't allocate callbacks ");
			goto error_handling;
		}

		/* Retrieve pointer to the correct CPD user structure */
		err = cpd_add_h4_user(current_dev, name);

		if (!err) {
			STE_CONN_DBG(
				"H:4 channel 0x%X registered",
				current_dev->h4_channel);
		} else {
			STE_CONN_ERR(
				"H:4 channel 0x%X already registered or other error (%d)",
				current_dev->h4_channel, err);
			goto error_handling;
		}
	} else {
		STE_CONN_ERR("Couldn't allocate current dev");
		goto error_handling;
	}

	if ((CPD_STATE_ACTIVE != cpd_info->main_state) &&
		(cpd_info->users.nbr_of_users == 1)) {
		/* Open CCD and start-up the chip */
		ste_conn_ccd_set_chip_power(true);

		/* Wait 100ms before continuing to be sure that the chip is ready */
		schedule_timeout_interruptible(timeval_to_jiffies(&time_100ms));

		err = ste_conn_ccd_open();

		if (err) {
			/* Remove the user. If there is no error it will be freed as well */
			cpd_remove_h4_user(&current_dev);
			goto finished;
		}

		cpd_chip_startup();

		/* Wait up to 15 seconds for chip to start */
		STE_CONN_DBG("Wait up to 15 seconds for chip to start..");
		wait_event_interruptible_timeout(ste_conn_cpd_wait_queue,
						((CPD_STATE_ACTIVE == cpd_info->main_state) ||
						(CPD_STATE_IDLE   == cpd_info->main_state)),
						timeval_to_jiffies(&time_15s));
		if (CPD_STATE_ACTIVE != cpd_info->main_state) {
			STE_CONN_ERR(
			"ST-Ericsson Connectivity Controller Driver failed to start");

			/* Close CCD and power off the chip */
			ste_conn_ccd_close();

			/* Remove the user. If there is no error it will be freed as well */
			cpd_remove_h4_user(&current_dev);

			/* Chip shut-down finished, set correct state. */
			CPD_SET_MAIN_STATE(CPD_STATE_IDLE);
		}
	}
	goto finished;

error_handling:
	cpd_free_user_dev(&current_dev);
finished:
	return current_dev;
}
EXPORT_SYMBOL(ste_conn_register);

void ste_conn_deregister(struct ste_conn_device *dev)
{
	int h4_channel;
	int err = 0;

	STE_CONN_INFO("ste_conn_deregister");

	if (!cpd_info) {
		STE_CONN_ERR("Hardware not started");
		return;
	}

	if (!dev) {
		STE_CONN_ERR("Calling with NULL pointer");
		return;
	}

	h4_channel = dev->h4_channel;
	/* Remove the user. If there is no error it will be freed as well */
	err = cpd_remove_h4_user(&dev);

	if (!err) {
		STE_CONN_DBG(
			"H:4 channel 0x%X deregistered",
			h4_channel);

		/* If this was the last user, shutdown the chip and close CCD */
		if (0 == cpd_info->users.nbr_of_users) {
			/* Make sure that the chip not have been shut down already. */
			if (CPD_STATE_IDLE != cpd_info->main_state) {
				CPD_SET_MAIN_STATE(CPD_STATE_CLOSING);
				cpd_chip_shutdown();

				/* Wait up to 15 seconds for chip to shut-down */
				STE_CONN_DBG("Wait up to 15 seconds for chip to shut-down..");
				wait_event_interruptible_timeout(ste_conn_cpd_wait_queue,
								(CPD_STATE_IDLE == cpd_info->main_state),
								timeval_to_jiffies(&time_15s));

				if (CPD_STATE_IDLE != cpd_info->main_state) {
					STE_CONN_ERR(
					"ST-Ericsson Connectivity Controller Driver was shut-down with problems.");

					/* Close CCD and power off the chip */
					ste_conn_ccd_close();

					/* Chip shut-down finished, set correct state. */
					CPD_SET_MAIN_STATE(CPD_STATE_IDLE);
				}

			}
		}
	} else {
		STE_CONN_ERR(
			"Trying to deregister non-registered H:4 channel 0x%X or other error %d",
			h4_channel, err);
	}
}
EXPORT_SYMBOL(ste_conn_deregister);

int ste_conn_reset(struct ste_conn_device *dev)
{
	int err = 0;

	STE_CONN_INFO("ste_conn_reset");

	if (!cpd_info) {
		STE_CONN_ERR("Hardware not started");
		return -EACCES;
	}

	CPD_SET_MAIN_STATE(CPD_STATE_RESETING);

	/* Shutdown the chip */
	cpd_chip_shutdown();

	/* Inform all registered users about the reset and free the user devices
	 * Don't send reset for debug and logging channels
	 */
	cpd_handle_reset_of_user(&(cpd_info->users.bt_cmd));
	cpd_handle_reset_of_user(&(cpd_info->users.bt_audio));
	cpd_handle_reset_of_user(&(cpd_info->users.bt_acl));
	cpd_handle_reset_of_user(&(cpd_info->users.bt_evt));
	cpd_handle_reset_of_user(&(cpd_info->users.fm_radio));
	cpd_handle_reset_of_user(&(cpd_info->users.fm_radio_audio));
	cpd_handle_reset_of_user(&(cpd_info->users.gnss));
	cpd_handle_reset_of_user(&(cpd_info->users.core));

	cpd_info->users.nbr_of_users = 0;

	/* Reset finished. We are now idle until first user is registered */
	CPD_SET_MAIN_STATE(CPD_STATE_IDLE);

	/* Send wake-up since this might have been called from a failed boot.
	 * No harm done if it is a CPD user who called.
	 */
	wake_up_interruptible(&ste_conn_cpd_wait_queue);

	return err;
}
EXPORT_SYMBOL(ste_conn_reset);

struct sk_buff *ste_conn_alloc_skb(unsigned int size, gfp_t priority)
{
	struct sk_buff *skb;

	STE_CONN_INFO("ste_conn_alloc_skb");
	STE_CONN_DBG("size %d bytes", size);

	/* Allocate the SKB and reserve space for the header */
	skb = alloc_skb(size + STE_CONN_SKB_RESERVE, priority);

	if (skb) {
		skb_reserve(skb, STE_CONN_SKB_RESERVE);
	}
	return skb;
}
EXPORT_SYMBOL(ste_conn_alloc_skb);

int ste_conn_write(struct ste_conn_device *dev, struct sk_buff *skb)
{
	int err = 0;
	uint8_t *h4_header;

	STE_CONN_INFO("ste_conn_write");

	if (!cpd_info) {
		STE_CONN_ERR("Hardware not started");
		return -EACCES;
	}

	if (!dev) {
		STE_CONN_ERR("ste_conn_write with no device");
		return -EINVAL;
	}

	if (!skb) {
		STE_CONN_ERR("ste_conn_write with no sk_buffer");
		return -EINVAL;
	}

	STE_CONN_DBG("Length %d bytes", skb->len);


	if (cpd_info->h4_channels.hci_logger_channel == dev->h4_channel) {
		/* Treat the HCI logger write differently.
		 * A write can only mean a change of configuration.
		 */
		err = cpd_enable_hci_logger(skb);
	} else if (cpd_info->h4_channels.core_channel == dev->h4_channel) {
		STE_CONN_ERR("Not possible to write data on core channel only supports enable / disable chip");
	} else if (CPD_STATE_ACTIVE == cpd_info->main_state) {
		/* Move the data pointer to the H:4 header position and store the H4 header */
		h4_header = skb_push(skb, STE_CONN_SKB_RESERVE);
		*h4_header = (uint8_t)dev->h4_channel;

		/* Check if the chip handler wants to handle this packet. If not, send it to CCD. */
		if (!cpd_info->chip_dev.callbacks.data_to_chip ||
		    !(cpd_info->chip_dev.callbacks.data_to_chip(&(cpd_info->chip_dev), dev, skb))) {
			cpd_transmit_skb_to_ccd(skb, dev->logger_enabled);
		}
	} else {
		STE_CONN_ERR("Trying to transmit data when ste_conn CPD is not active");
		err = -EACCES;
	}

	return err;
}
EXPORT_SYMBOL(ste_conn_write);

bool ste_conn_get_local_revision(struct ste_conn_revision_data *rev_data)
{
	bool data_available = false;

	if (!rev_data) {
		STE_CONN_ERR("Calling with rev_data NULL");
		goto finished;
	}

	if (cpd_info->local_chip_info.version_set) {
		data_available = true;
		rev_data->revision = cpd_info->local_chip_info.hci_revision;
		rev_data->sub_version = cpd_info->local_chip_info.lmp_pal_subversion;
	}

finished:
	return data_available;
}
EXPORT_SYMBOL(ste_conn_get_local_revision);

/*
 *	External functions
 */

void ste_conn_cpd_hw_registered(void)
{
	bool run_shutdown = true;

	STE_CONN_INFO("ste_conn_cpd_hw_registered");

	CPD_SET_MAIN_STATE(CPD_STATE_INITIALIZING);
	CPD_SET_BOOT_STATE(BOOT_STATE_NOT_STARTED);

	/* This might look strange, but we need to read out
	 * the revision info in order to be able to shutdown the chip properly.
	 */
	ste_conn_ccd_set_chip_power(true);

	/* Wait 100ms before continuing to be sure that the chip is ready */
	schedule_timeout_interruptible(timeval_to_jiffies(&time_100ms));

	/* Transmit HCI reset command to ensure the chip is using
	 * the correct transport
	 */
	cpd_create_and_send_bt_cmd(cpd_msg_reset_cmd_req,
				sizeof(cpd_msg_reset_cmd_req));

	/* Wait up to 500 milliseconds for revision to be read out */
	STE_CONN_DBG("Wait up to 500 milliseconds for revision to be read.");
	wait_event_interruptible_timeout(ste_conn_cpd_wait_queue,
					 (BOOT_STATE_READY == cpd_info->boot_state),
					 timeval_to_jiffies(&time_500ms));
	if (BOOT_STATE_READY != cpd_info->boot_state) {
		bool chip_handled = false;
		struct list_head *cursor;
		struct chip_handler_item *tmp;

		STE_CONN_ERR("Could not read out revision from the chip. This is typical when running stubbed CCD.");

		STE_CONN_ERR("Switching to default value: man 0x%04X, rev 0x%04X, sub 0x%04X",
			     ste_conn_default_manufacturer, ste_conn_default_hci_revision,
			     ste_conn_default_sub_version);

		cpd_info->chip_dev.chip.manufacturer = ste_conn_default_manufacturer;
		cpd_info->chip_dev.chip.hci_revision = ste_conn_default_hci_revision;
		cpd_info->chip_dev.chip.hci_sub_version = ste_conn_default_sub_version;

		memset(&(cpd_info->chip_dev.callbacks), 0, sizeof(cpd_info->chip_dev.callbacks));

		list_for_each(cursor, &cpd_chip_handlers) {
			tmp = list_entry(cursor, struct chip_handler_item, list);
			chip_handled = tmp->callbacks.check_chip_support(&(cpd_info->chip_dev));
			if (chip_handled) {
				STE_CONN_INFO("Chip handler found");
				CPD_SET_BOOT_STATE(BOOT_STATE_READY);
				break;
			}
		}
		run_shutdown = false;
	}

	/* Read out the channels for connected chip */
	if (cpd_info->chip_dev.callbacks.get_h4_channel) {
		/* Get the H4 channel ID for all channels */
		cpd_info->chip_dev.callbacks.get_h4_channel(STE_CONN_DEVICES_BT_CMD,
			&(cpd_info->h4_channels.bt_cmd_channel));
		cpd_info->chip_dev.callbacks.get_h4_channel(STE_CONN_DEVICES_BT_ACL,
			&(cpd_info->h4_channels.bt_acl_channel));
		cpd_info->chip_dev.callbacks.get_h4_channel(STE_CONN_DEVICES_BT_EVT,
			&(cpd_info->h4_channels.bt_evt_channel));
		cpd_info->chip_dev.callbacks.get_h4_channel(STE_CONN_DEVICES_GNSS,
			&(cpd_info->h4_channels.gnss_channel));
		cpd_info->chip_dev.callbacks.get_h4_channel(STE_CONN_DEVICES_FM_RADIO,
			&(cpd_info->h4_channels.fm_radio_channel));
		cpd_info->chip_dev.callbacks.get_h4_channel(STE_CONN_DEVICES_DEBUG,
			&(cpd_info->h4_channels.debug_channel));
		cpd_info->chip_dev.callbacks.get_h4_channel(STE_CONN_DEVICES_STE_TOOLS,
			&(cpd_info->h4_channels.ste_tools_channel));
		cpd_info->chip_dev.callbacks.get_h4_channel(STE_CONN_DEVICES_HCI_LOGGER,
			&(cpd_info->h4_channels.hci_logger_channel));
		cpd_info->chip_dev.callbacks.get_h4_channel(STE_CONN_DEVICES_US_CTRL,
			&(cpd_info->h4_channels.us_ctrl_channel));
		cpd_info->chip_dev.callbacks.get_h4_channel(STE_CONN_DEVICES_CORE,
			&(cpd_info->h4_channels.core_channel));
	}

	/* Now it is time to shutdown the controller to reduce
	 * power consumption until any users register
	 */
	if (run_shutdown) {
		cpd_chip_shutdown();
	} else {
		ste_conn_cpd_chip_shutdown_finished(0);
	}
}
EXPORT_SYMBOL(ste_conn_cpd_hw_registered);

void ste_conn_cpd_hw_deregistered(void)
{
	STE_CONN_INFO("ste_conn_cpd_hw_deregistered");

	CPD_SET_MAIN_STATE(CPD_STATE_INITIALIZING);
}
EXPORT_SYMBOL(ste_conn_cpd_hw_deregistered);

void ste_conn_cpd_data_received(struct sk_buff *skb)
{
	struct ste_conn_device *dev = NULL;
	uint8_t h4_channel;
	int err = 0;

	STE_CONN_INFO("ste_conn_cpd_data_received");

	if (!skb) {
		STE_CONN_ERR("No data supplied");
		return;
	}

	h4_channel = *(skb->data);

	/* First check if this is the response for something we have sent internally */
	if ((cpd_info->main_state == CPD_STATE_BOOTING ||
	     cpd_info->main_state == CPD_STATE_INITIALIZING) &&
	    (HCI_BT_EVT_H4_CHANNEL == h4_channel) &&
	    cpd_handle_internal_rx_data_bt_evt(skb)) {
		STE_CONN_DBG("Received packet handled internally");
		return;
	}

	/* Find out where to route the data */
	err = cpd_find_h4_user(h4_channel, &dev, skb);

	/* Check if the chip handler wants to deal with the packet. */
	if (!err && cpd_info->chip_dev.callbacks.data_from_chip &&
	    cpd_info->chip_dev.callbacks.data_from_chip(&(cpd_info->chip_dev), dev, skb)) {
		return;
	}

	if (!err && dev) {
		/* If HCI logging is enabled for this channel, copy the data to
		 * the HCI logging output.
		 */
		if (cpd_info->users.hci_logger && dev->logger_enabled) {
			/* Alloc a new sk_buffer and copy the data into it.
			 * Then send it to the HCI logger. */
			struct sk_buff *skb_log = alloc_skb(skb->len + 1, GFP_ATOMIC);
			if (skb_log) {
				memcpy(skb_put(skb_log, skb->len), skb->data, skb->len);
				skb_log->data[0] = (uint8_t) LOGGER_DIRECTION_RX;

				cpd_info->users.hci_logger->callbacks->read_cb(cpd_info->users.hci_logger, skb_log);
			} else {
				STE_CONN_ERR("Couldn't allocate skb_log");
			}
		}

		/* Remove the H4 header */
		(void)skb_pull(skb, STE_CONN_SKB_RESERVE);

		/* Call the Read callback */
		if (dev->callbacks->read_cb) {
			dev->callbacks->read_cb(dev, skb);
		}
	} else {
		STE_CONN_ERR("H:4 channel: 0x%X,  does not match device", h4_channel);
		kfree_skb(skb);
	}
}
EXPORT_SYMBOL(ste_conn_cpd_data_received);

int ste_conn_cpd_register_chip_driver(struct ste_conn_cpd_id_callbacks *callbacks)
{
	int err = 0;
	struct chip_handler_item *item;

	STE_CONN_INFO("ste_conn_cpd_register_chip_driver");

	if (!callbacks) {
		STE_CONN_ERR("NULL supplied as callbacks");
		err = -EINVAL;
		goto finished;
	}

	item = kzalloc(sizeof(*item), GFP_ATOMIC);
	if (item) {
		memcpy(&(item->callbacks), callbacks, sizeof(callbacks));
		list_add_tail(&item->list, &cpd_chip_handlers);
	} else {
		STE_CONN_ERR("Failed to alloc memory!");
		err = -ENOMEM;
	}

finished:
	return err;
}
EXPORT_SYMBOL(ste_conn_cpd_register_chip_driver);

int ste_conn_cpd_chip_startup_finished(int err)
{
	int ret_val = 0;

	STE_CONN_INFO("ste_conn_cpd_chip_startup_finished (%d)", err);

	if (err) {
		ste_conn_reset(NULL);
	} else {
		CPD_SET_MAIN_STATE(CPD_STATE_ACTIVE);
		wake_up_interruptible(&ste_conn_cpd_wait_queue);
	}

	return ret_val;
}
EXPORT_SYMBOL(ste_conn_cpd_chip_startup_finished);

int ste_conn_cpd_chip_shutdown_finished(int err)
{
	int ret_val = 0;

	STE_CONN_INFO("ste_conn_cpd_chip_shutdown_finished (%d)", err);

	/* Close CCD, which will power off the chip */
	ste_conn_ccd_close();

	/* Chip shut-down finished, set correct state and wake up the cpd. */
	CPD_SET_MAIN_STATE(CPD_STATE_IDLE);
	wake_up_interruptible(&ste_conn_cpd_wait_queue);

	return ret_val;
}
EXPORT_SYMBOL(ste_conn_cpd_chip_shutdown_finished);

int ste_conn_cpd_send_to_chip(struct sk_buff *skb, bool use_logger)
{
	int err = 0;

	cpd_transmit_skb_to_ccd(skb, use_logger);

	return err;
}
EXPORT_SYMBOL(ste_conn_cpd_send_to_chip);

struct ste_conn_device *ste_conn_cpd_get_bt_cmd_dev(void)
{
	if (cpd_info) {
		return cpd_info->users.bt_cmd;
	} else {
		return NULL;
	}
}
EXPORT_SYMBOL(ste_conn_cpd_get_bt_cmd_dev);

struct ste_conn_device *ste_conn_cpd_get_fm_radio_dev(void)
{
	if (cpd_info) {
		return cpd_info->users.fm_radio;
	} else {
		return NULL;
	}
}
EXPORT_SYMBOL(ste_conn_cpd_get_fm_radio_dev);

struct ste_conn_device *ste_conn_cpd_get_bt_audio_dev(void)
{
	if (cpd_info) {
		return cpd_info->users.bt_audio;
	} else {
		return NULL;
	}
}
EXPORT_SYMBOL(ste_conn_cpd_get_bt_audio_dev);

struct ste_conn_device *ste_conn_cpd_get_fm_audio_dev(void)
{
	if (cpd_info) {
		return cpd_info->users.fm_radio_audio;
	} else {
		return NULL;
	}
}
EXPORT_SYMBOL(ste_conn_cpd_get_fm_audio_dev);

struct ste_conn_cpd_hci_logger_config *ste_conn_cpd_get_hci_logger_config(void)
{
	if (cpd_info) {
		return &(cpd_info->hci_logger_config);
	} else {
		return NULL;
	}
}
EXPORT_SYMBOL(ste_conn_cpd_get_hci_logger_config);

int ste_conn_cpd_get_h4_channel(char *name, int *h4_channel)
{
	if (!name || !h4_channel) {
		STE_CONN_ERR("NULL supplied as in-parameter");
		return -EINVAL;
	}

	if (cpd_info && cpd_info->chip_dev.callbacks.get_h4_channel) {
		return cpd_info->chip_dev.callbacks.get_h4_channel(name, h4_channel);
	} else {
		return -ENXIO;
	}
}
EXPORT_SYMBOL(ste_conn_cpd_get_h4_channel);

int ste_conn_cpd_init(int char_dev_usage, struct device *dev)
{
	int err = 0;

	STE_CONN_INFO("ste_conn_cpd_init");

	if (cpd_info) {
		STE_CONN_ERR("CPD already initiated");
		err = -EBUSY;
		goto finished;
	}
	/* First allocate our info structure and initialize the parameters */
	cpd_info = kzalloc(sizeof(*cpd_info), GFP_ATOMIC);

	if (cpd_info) {
		cpd_info->main_state           = CPD_STATE_INITIALIZING;
		cpd_info->boot_state           = BOOT_STATE_NOT_STARTED;

		cpd_info->dev = dev;
		cpd_info->chip_dev.dev = dev;

		cpd_info->wq = create_singlethread_workqueue(STE_CONN_CPD_WQ_NAME);
		if (!cpd_info->wq) {
			STE_CONN_ERR("Could not create workqueue");
			err = -ENOMEM;
			goto error_handling;
		}

		/* Initialize the character devices */
		ste_conn_char_devices_init(char_dev_usage, dev);
	} else {
		STE_CONN_ERR("Couldn't allocate cpd_info");
		err = -ENOMEM;
		goto finished;
	}

	goto finished;

error_handling:
	if (cpd_info) {
		kfree(cpd_info);
		cpd_info = NULL;
	}

finished:
	return err;
}
EXPORT_SYMBOL(ste_conn_cpd_init);

void ste_conn_cpd_exit(void)
{
	STE_CONN_INFO("ste_conn_cpd_exit");

	if (!cpd_info) {
		STE_CONN_ERR("cpd not initiated");
		return;
	}

	/* Remove initialized character devices */
	ste_conn_char_devices_exit();

	/* Free the user devices */
	cpd_free_user_dev(&(cpd_info->users.bt_cmd));
	cpd_free_user_dev(&(cpd_info->users.bt_acl));
	cpd_free_user_dev(&(cpd_info->users.bt_evt));
	cpd_free_user_dev(&(cpd_info->users.fm_radio));
	cpd_free_user_dev(&(cpd_info->users.gnss));
	cpd_free_user_dev(&(cpd_info->users.debug));
	cpd_free_user_dev(&(cpd_info->users.ste_tools));
	cpd_free_user_dev(&(cpd_info->users.hci_logger));
	cpd_free_user_dev(&(cpd_info->users.us_ctrl));
	cpd_free_user_dev(&(cpd_info->users.core));

	destroy_workqueue(cpd_info->wq);
	kfree(cpd_info);
	cpd_info = NULL;
}
EXPORT_SYMBOL(ste_conn_cpd_exit);

/*
 *	Internal functions
 */

/**
 * cpd_enable_hci_logger() - Enable HCI logger for each device.
 * @skb:     Received sk buffer.
 *
 * The cpd_enable_hci_logger() change HCI logger configuration
 * for all registered devices.
 *
 * Returns:
 *   0 if there is no error.
 *   -EACCES if bad structure was supplied.
 */
static int cpd_enable_hci_logger(struct sk_buff *skb)
{
	int err = 0;

	if (skb->len == sizeof(struct ste_conn_cpd_hci_logger_config)) {
		/* First store the logger config */
		memcpy(&(cpd_info->hci_logger_config), skb->data,
			sizeof(struct ste_conn_cpd_hci_logger_config));

		/* Then go through all devices and set the right settings */
		if (cpd_info->users.bt_cmd) {
			cpd_info->users.bt_cmd->logger_enabled = cpd_info->hci_logger_config.bt_cmd_enable;
		}
		if (cpd_info->users.bt_audio) {
			cpd_info->users.bt_audio->logger_enabled = cpd_info->hci_logger_config.bt_audio_enable;
		}
		if (cpd_info->users.bt_acl) {
			cpd_info->users.bt_acl->logger_enabled = cpd_info->hci_logger_config.bt_acl_enable;
		}
		if (cpd_info->users.bt_evt) {
			cpd_info->users.bt_evt->logger_enabled = cpd_info->hci_logger_config.bt_evt_enable;
		}
		if (cpd_info->users.fm_radio) {
			cpd_info->users.fm_radio->logger_enabled = cpd_info->hci_logger_config.fm_radio_enable;
		}
		if (cpd_info->users.fm_radio_audio) {
			cpd_info->users.fm_radio_audio->logger_enabled = cpd_info->hci_logger_config.fm_radio_audio_enable;
		}
		if (cpd_info->users.gnss) {
			cpd_info->users.gnss->logger_enabled = cpd_info->hci_logger_config.gnss_enable;
		}
	} else {
		STE_CONN_ERR("Trying to configure HCI logger with bad structure");
		err = -EACCES;
	}
	kfree_skb(skb);
	return err;
}

/**
 * cpd_find_bt_audio_user() - Check if incoming data packet is an audio related packet.
 * @h4_channel:     H4 channel.
 * @dev:     Stored ste_conn device.
 * @skb:  skb with received packet.
 * Returns:
 *   0 - if no error occurred.
 *   -EINVAL - if ste_conn_device not found.
 */
static int cpd_find_bt_audio_user(int h4_channel, struct ste_conn_device **dev,
	const struct sk_buff * const skb)
{
	int err = 0;

	if (!cpd_info->chip_dev.callbacks.is_bt_audio_user ||
	    !cpd_info->chip_dev.callbacks.is_bt_audio_user(h4_channel, skb)) {
		/* Not a BT audio event */
	} else {
		*dev = cpd_info->users.bt_audio;
		if (!(*dev)) {
			STE_CONN_ERR("H:4 channel not registered in cpd_info: 0x%X", h4_channel);
			err = -ENXIO;
		}

	}
	return err;
}


/**
 * cpd_find_fm_audio_user() - Check if incoming data packet is an audio related packet.
 * @h4_channel:     H4 channel.
 * @dev:     Stored ste_conn device.
 * @skb:  skb with received packet.
 * Returns:
 *   0 - if no error occurred.
 *   -EINVAL - if ste_conn_device not found.
 */
static int cpd_find_fm_audio_user(int h4_channel, struct ste_conn_device **dev,
	const struct sk_buff * const skb)
{
	int err = 0;

	if (!cpd_info->chip_dev.callbacks.is_fm_audio_user ||
	    !cpd_info->chip_dev.callbacks.is_fm_audio_user(h4_channel, skb)) {
		/* Not an FM audio event */
	} else {
		*dev = cpd_info->users.fm_radio_audio;
		if (!(*dev)) {
			STE_CONN_ERR("H:4 channel not registered in cpd_info: 0x%X", h4_channel);
			err = -ENXIO;
		}

	}

	return err;
}



/**
 * cpd_find_h4_user() - Get H4 user based on supplied H4 channel.
 * @h4_channel:     H4 channel.
 * @dev:     Stored ste_conn device.
 * @skb:    (optional) skb with received packet. Set to NULL if NA.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if bad channel is supplied or no user was found.
 */
static int cpd_find_h4_user(int h4_channel, struct ste_conn_device **dev,
		const struct sk_buff * const skb)
{
	int err = 0;

	if (h4_channel == cpd_info->h4_channels.bt_cmd_channel) {
		*dev = cpd_info->users.bt_cmd;
	} else if (h4_channel == cpd_info->h4_channels.bt_acl_channel) {
		*dev = cpd_info->users.bt_acl;
	} else if (h4_channel == cpd_info->h4_channels.bt_evt_channel) {
		*dev = cpd_info->users.bt_evt;
		/*Check if it's event generated by previously sent audio user command.
		* If so then that event should be dispatched to audio user*/
		err = cpd_find_bt_audio_user(h4_channel, dev, skb);
	} else if (h4_channel == cpd_info->h4_channels.gnss_channel) {
		*dev = cpd_info->users.gnss;
	} else if (h4_channel == cpd_info->h4_channels.fm_radio_channel) {
		*dev = cpd_info->users.fm_radio;
		/* Check if it's an event generated by previously sent audio user command.
		 * If so then that event should be dispatched to audio user */
		err = cpd_find_fm_audio_user(h4_channel, dev, skb);
	} else if (h4_channel == cpd_info->h4_channels.debug_channel) {
		*dev = cpd_info->users.debug;
	} else if (h4_channel == cpd_info->h4_channels.ste_tools_channel) {
		*dev = cpd_info->users.ste_tools;
	} else if (h4_channel == cpd_info->h4_channels.hci_logger_channel) {
		*dev = cpd_info->users.hci_logger;
	} else if (h4_channel == cpd_info->h4_channels.us_ctrl_channel) {
		*dev = cpd_info->users.us_ctrl;
	} else if (h4_channel == cpd_info->h4_channels.core_channel) {
		*dev = cpd_info->users.core;
	} else {
		*dev = NULL;
		STE_CONN_ERR("Bad H:4 channel supplied: 0x%X", h4_channel);
		return -EINVAL;
	}

	return err;
}

/**
 * cpd_add_h4_user() - Add H4 user to user storage based on supplied H4 channel.
 * @dev:     Stored ste_conn device.
 * @name:	Device name to identify different devices that are using the same H4 channel.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL pointer or bad channel is supplied.
 *   -EBUSY if there already is a user for supplied channel.
 */
static int cpd_add_h4_user(struct ste_conn_device *dev, const char * const name)
{
	int err = 0;

	if (!dev) {
		STE_CONN_ERR("NULL device supplied");
		err = -EINVAL;
		goto error_handling;
	}

	if (dev->h4_channel == cpd_info->h4_channels.bt_cmd_channel) {
		if (!cpd_info->users.bt_cmd &&
			0 == strncmp(name, STE_CONN_DEVICES_BT_CMD, STE_CONN_MAX_NAME_SIZE)) {
			cpd_info->users.bt_cmd = dev;
			cpd_info->users.bt_cmd->logger_enabled =
				cpd_info->hci_logger_config.bt_cmd_enable;
			cpd_info->users.nbr_of_users++;
		} else if (!cpd_info->users.bt_audio &&
			0 == strncmp(name, STE_CONN_DEVICES_BT_AUDIO, STE_CONN_MAX_NAME_SIZE)) {
			cpd_info->users.bt_audio = dev;
			cpd_info->users.bt_audio->logger_enabled =
				cpd_info->hci_logger_config.bt_audio_enable;
			cpd_info->users.nbr_of_users++;
		} else {
			err = -EBUSY;
			STE_CONN_ERR("name %s bt_cmd 0x%X  bt_audio 0x%X", name, (int)cpd_info->users.bt_cmd, (int)cpd_info->users.bt_audio);
		}
	} else if (dev->h4_channel == cpd_info->h4_channels.bt_acl_channel) {
		if (!cpd_info->users.bt_acl) {
			cpd_info->users.bt_acl = dev;
			cpd_info->users.bt_acl->logger_enabled =
				cpd_info->hci_logger_config.bt_acl_enable;
			cpd_info->users.nbr_of_users++;
		} else {
			err = -EBUSY;
		}
	} else if (dev->h4_channel == cpd_info->h4_channels.bt_evt_channel) {
		if (!cpd_info->users.bt_evt) {
			cpd_info->users.bt_evt = dev;
			cpd_info->users.bt_evt->logger_enabled =
				cpd_info->hci_logger_config.bt_evt_enable;
			cpd_info->users.nbr_of_users++;
		} else {
			err = -EBUSY;
		}
	} else if (dev->h4_channel == cpd_info->h4_channels.gnss_channel) {
		if (!cpd_info->users.gnss) {
			cpd_info->users.gnss = dev;
			cpd_info->users.gnss->logger_enabled =
				cpd_info->hci_logger_config.gnss_enable;
			cpd_info->users.nbr_of_users++;
		} else {
			err = -EBUSY;
		}
	} else if (dev->h4_channel == cpd_info->h4_channels.fm_radio_channel) {
		if (!cpd_info->users.fm_radio &&
			0 == strncmp(name, STE_CONN_DEVICES_FM_RADIO, STE_CONN_MAX_NAME_SIZE)) {
			cpd_info->users.fm_radio = dev;
			cpd_info->users.fm_radio->logger_enabled =
				cpd_info->hci_logger_config.fm_radio_enable;
			cpd_info->users.nbr_of_users++;
		} else if (!cpd_info->users.fm_radio_audio &&
			0 == strncmp(name, STE_CONN_DEVICES_FM_RADIO_AUDIO, STE_CONN_MAX_NAME_SIZE)) {
			cpd_info->users.fm_radio_audio = dev;
			cpd_info->users.fm_radio_audio->logger_enabled =
				cpd_info->hci_logger_config.fm_radio_audio_enable;
			cpd_info->users.nbr_of_users++;
		} else {
			err = -EBUSY;
		}
	} else if (dev->h4_channel == cpd_info->h4_channels.debug_channel) {
		if (!cpd_info->users.debug) {
			cpd_info->users.debug = dev;
		} else {
			err = -EBUSY;
		}
	} else if (dev->h4_channel == cpd_info->h4_channels.ste_tools_channel) {
		if (!cpd_info->users.ste_tools) {
			cpd_info->users.ste_tools = dev;
		} else {
			err = -EBUSY;
		}
	} else if (dev->h4_channel == cpd_info->h4_channels.hci_logger_channel) {
		if (!cpd_info->users.hci_logger) {
			cpd_info->users.hci_logger = dev;
		} else {
			err = -EBUSY;
		}
	} else if (dev->h4_channel == cpd_info->h4_channels.us_ctrl_channel) {
		if (!cpd_info->users.us_ctrl) {
			cpd_info->users.us_ctrl = dev;
		} else {
			err = -EBUSY;
		}
	} else if (dev->h4_channel == cpd_info->h4_channels.core_channel) {
		if (!cpd_info->users.core) {
			cpd_info->users.nbr_of_users++;
			cpd_info->users.core = dev;
		} else {
			err = -EBUSY;
		}
	} else {
		err = -EINVAL;
		STE_CONN_ERR("Bad H:4 channel supplied: 0x%X", dev->h4_channel);
	}

	if (err) {
		STE_CONN_ERR("H:4 channel 0x%X, not registered", dev->h4_channel);
	}

error_handling:
	return err;
}

/**
 * cpd_remove_h4_user() - Remove H4 user from user storage.
 * @dev:     Stored ste_conn device.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if NULL pointer is supplied, bad channel is supplied, or if there
 *   is no user for supplied channel.
 */
static int cpd_remove_h4_user(struct ste_conn_device **dev)
{
	int err = 0;

	if (!dev || !(*dev)) {
		STE_CONN_ERR("NULL device supplied");
		err = -EINVAL;
		goto error_handling;
	}

	if ((*dev)->h4_channel == cpd_info->h4_channels.bt_cmd_channel) {
		STE_CONN_DBG("bt_cmd 0x%X bt_audio 0x%X dev 0x%X", (int)cpd_info->users.bt_cmd, (int)cpd_info->users.bt_audio, (int)*dev);

		if (*dev == cpd_info->users.bt_cmd) {
			cpd_info->users.bt_cmd = NULL;
			cpd_info->users.nbr_of_users--;
		} else if (*dev == cpd_info->users.bt_audio) {
			cpd_info->users.bt_audio = NULL;
			cpd_info->users.nbr_of_users--;
		} else {
			err = -EINVAL;
		}

		STE_CONN_DBG("bt_cmd 0x%X bt_audio 0x%X dev 0x%X", (int)cpd_info->users.bt_cmd, (int)cpd_info->users.bt_audio, (int)*dev);

		/* If both BT Command channel users are de-registered we inform the chip handler. */
		if (cpd_info->users.bt_cmd == NULL && cpd_info->users.bt_audio == NULL) {
			if (cpd_info->chip_dev.callbacks.last_bt_user_removed) {
				cpd_info->chip_dev.callbacks.last_bt_user_removed(&(cpd_info->chip_dev));
			}
		}
	} else if ((*dev)->h4_channel == cpd_info->h4_channels.bt_acl_channel) {
		if (*dev == cpd_info->users.bt_acl) {
			cpd_info->users.bt_acl = NULL;
			cpd_info->users.nbr_of_users--;
		} else {
			err = -EINVAL;
		}
	} else if ((*dev)->h4_channel == cpd_info->h4_channels.bt_evt_channel) {
		if (*dev == cpd_info->users.bt_evt) {
			cpd_info->users.bt_evt = NULL;
			cpd_info->users.nbr_of_users--;
		} else {
			err = -EINVAL;
		}
	} else if ((*dev)->h4_channel == cpd_info->h4_channels.gnss_channel) {
		if (*dev == cpd_info->users.gnss) {
			cpd_info->users.gnss = NULL;
			cpd_info->users.nbr_of_users--;
		} else {
			err = -EINVAL;
		}

		/* If the GNSS channel user is de-registered we inform the chip handler. */
		if (cpd_info->users.gnss == NULL) {
			if (cpd_info->chip_dev.callbacks.last_gnss_user_removed) {
				cpd_info->chip_dev.callbacks.last_gnss_user_removed(&(cpd_info->chip_dev));
			}
		}
	} else if ((*dev)->h4_channel == cpd_info->h4_channels.fm_radio_channel) {
		if (*dev == cpd_info->users.fm_radio) {
			cpd_info->users.fm_radio = NULL;
			cpd_info->users.nbr_of_users--;
		} else if (*dev == cpd_info->users.fm_radio_audio) {
			cpd_info->users.fm_radio_audio = NULL;
			cpd_info->users.nbr_of_users--;
		} else {
			err = -EINVAL;
		}

		/* If both FM Radio channel users are de-registered we inform the chip handler. */
		if (cpd_info->users.fm_radio == NULL && cpd_info->users.fm_radio_audio == NULL) {
			if (cpd_info->chip_dev.callbacks.last_fm_user_removed) {
				cpd_info->chip_dev.callbacks.last_fm_user_removed(&(cpd_info->chip_dev));
			}
		}
	} else if ((*dev)->h4_channel == cpd_info->h4_channels.debug_channel) {
		if (*dev == cpd_info->users.debug) {
			cpd_info->users.debug = NULL;
		} else {
			err = -EINVAL;
		}
	} else if ((*dev)->h4_channel == cpd_info->h4_channels.ste_tools_channel) {
		if (*dev == cpd_info->users.ste_tools) {
			cpd_info->users.ste_tools = NULL;
		} else {
			err = -EINVAL;
		}
	} else if ((*dev)->h4_channel == cpd_info->h4_channels.hci_logger_channel) {
		if (*dev == cpd_info->users.hci_logger) {
			cpd_info->users.hci_logger = NULL;
		} else {
			err = -EINVAL;
		}
	} else if ((*dev)->h4_channel == cpd_info->h4_channels.us_ctrl_channel) {
		if (*dev == cpd_info->users.us_ctrl) {
			cpd_info->users.us_ctrl = NULL;
		} else {
			err = -EINVAL;
		}
	} else if ((*dev)->h4_channel == cpd_info->h4_channels.core_channel) {
		if (*dev == cpd_info->users.core) {
			cpd_info->users.core = NULL;
			cpd_info->users.nbr_of_users--;
		} else {
			err = -EINVAL;
		}
	} else {
		err = -EINVAL;
		STE_CONN_ERR("Bad H:4 channel supplied: 0x%X", (*dev)->h4_channel);
		goto error_handling;
	}

	if (err) {
		STE_CONN_ERR("Trying to remove device that was not registered");
	}

	/* Free the device even if there is an error with the device.
	 * Also set to NULL to inform caller about the free.
	 */
	cpd_free_user_dev(dev);

error_handling:
	return err;
}

/**
 * cpd_chip_startup() - Start the connectivity controller and download patches and settings.
 */
static void cpd_chip_startup(void)
{
	STE_CONN_INFO("cpd_chip_startup");

	CPD_SET_MAIN_STATE(CPD_STATE_BOOTING);
	CPD_SET_BOOT_STATE(BOOT_STATE_NOT_STARTED);

	/* Transmit HCI reset command to ensure the chip is using
	 * the correct transport
	 */
	cpd_create_and_send_bt_cmd(cpd_msg_reset_cmd_req,
				sizeof(cpd_msg_reset_cmd_req));
}

/**
 * cpd_chip_shutdown() - Reset and power the chip off.
 */
static void cpd_chip_shutdown(void)
{
	int err = 0;

	STE_CONN_INFO("cpd_chip_shutdown");

	/* First do a quick power switch of the chip to assure a good state */
	ste_conn_ccd_set_chip_power(false);

	/* Wait 50ms before continuing to be sure that the chip detects chip power off */
	schedule_timeout_interruptible(timeval_to_jiffies(&time_50ms));

	ste_conn_ccd_set_chip_power(true);

	/* Wait 100ms before continuing to be sure that the chip is ready */
	schedule_timeout_interruptible(timeval_to_jiffies(&time_100ms));

	/* Let the chip handler finish the reset if any callback is registered.
	 * Otherwise we are finished */
	if (!cpd_info->chip_dev.callbacks.chip_shutdown) {
		STE_CONN_DBG("No registered handler. Finishing shutdown.");
		ste_conn_cpd_chip_shutdown_finished(err);
		return;
	}

	err = cpd_info->chip_dev.callbacks.chip_shutdown(&(cpd_info->chip_dev));
	if (err) {
		STE_CONN_DBG("chip_shutdown failed (%d). Finishing shutdown.", err);
		ste_conn_cpd_chip_shutdown_finished(err);
	}
}

/**
 * cpd_handle_internal_rx_data_bt_evt() - Check if received data should be handled in CPD.
 * @skb: Data packet
 * The cpd_handle_internal_rx_data_bt_evt() function checks if received data should be handled in CPD.
 * If so handle it correctly. Received data is always HCI BT Event.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cpd_handle_internal_rx_data_bt_evt(struct sk_buff *skb)
{
	bool pkt_handled = false;
	uint8_t *data = &(skb->data[STE_CONN_SKB_RESERVE]);
	uint8_t event_code = data[0];

	/* First check the event code */
	if (HCI_BT_EVT_CMD_COMPLETE == event_code) {
		uint8_t lsb = data[3];
		uint8_t msb = data[4];

		STE_CONN_DBG_DATA("Received Command Complete: LSB = 0x%02X, MSB=0x%02X", lsb, msb);
		data += 5; /* Move to first byte after OCF */

		if (lsb == HCI_BT_RESET_CMD_LSB &&
		    msb == HCI_BT_RESET_CMD_MSB) {
			pkt_handled = cpd_handle_reset_cmd_complete_evt(data);
		} else if (lsb == HCI_BT_READ_LOCAL_VERSION_CMD_LSB &&
			   msb == HCI_BT_READ_LOCAL_VERSION_CMD_MSB) {
			pkt_handled = cpd_handle_read_local_version_info_cmd_complete_evt(data);
		}
	}

	if (pkt_handled) {
		kfree_skb(skb);
	}
	return pkt_handled;
}

/**
 * cpd_handle_reset_cmd_complete_evt() - Handle a received HCI Command Complete event for a Reset command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cpd_handle_reset_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;
	uint8_t status = data[0];

	STE_CONN_INFO("Received Reset complete event with status 0x%X", status);

	if ((cpd_info->main_state == CPD_STATE_BOOTING ||
	     cpd_info->main_state == CPD_STATE_INITIALIZING) &&
	    cpd_info->boot_state == BOOT_STATE_NOT_STARTED) {
		/* Transmit HCI Read Local Version Information command */
		CPD_SET_BOOT_STATE(BOOT_STATE_READ_LOCAL_VERSION_INFORMATION);
		cpd_create_and_send_bt_cmd(cpd_msg_read_local_version_information_cmd_req,
					   sizeof(cpd_msg_read_local_version_information_cmd_req));

		pkt_handled = true;
	}

	return pkt_handled;
}

/**
 * cpd_handle_read_local_version_info_cmd_complete_evt() - Handle a received HCI Command Complete event
 * for a ReadLocalVersionInformation command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cpd_handle_read_local_version_info_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;
	bool chip_handled = false;
	struct list_head *cursor;
	struct chip_handler_item *tmp;

	if ((cpd_info->main_state == CPD_STATE_BOOTING ||
	     cpd_info->main_state == CPD_STATE_INITIALIZING) &&
	     cpd_info->boot_state == BOOT_STATE_READ_LOCAL_VERSION_INFORMATION) {
		/* We got an answer for our HCI command. Extract data */
		uint8_t status = data[0];
		int err = 0;

		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* The command worked. Store the data */
			cpd_info->local_chip_info.version_set = true;
			cpd_info->local_chip_info.hci_version = data[1];
			cpd_info->local_chip_info.hci_revision = data[2] | (data[3] << 8);
			cpd_info->local_chip_info.lmp_pal_version = data[4];
			cpd_info->local_chip_info.manufacturer = data[5] | (data[6] << 8);
			cpd_info->local_chip_info.lmp_pal_subversion = data[7] | (data[8] << 8);
			STE_CONN_DBG("Received Read Local Version Information with:\n \
							hci_version: 0x%X\n hci_revision: 0x%X\n lmp_pal_version: 0x%X\n \
							manufacturer: 0x%X\n lmp_pal_subversion: 0x%X", \
			cpd_info->local_chip_info.hci_version, cpd_info->local_chip_info.hci_revision, \
			cpd_info->local_chip_info.lmp_pal_version, cpd_info->local_chip_info.manufacturer, \
			cpd_info->local_chip_info.lmp_pal_subversion);

			ste_conn_devices_set_hci_revision(cpd_info->local_chip_info.hci_version,
							cpd_info->local_chip_info.hci_revision,
							cpd_info->local_chip_info.lmp_pal_version,
							cpd_info->local_chip_info.lmp_pal_subversion,
							cpd_info->local_chip_info.manufacturer);

			/* Received good confirmation. Find handler for the chip. */
			cpd_info->chip_dev.chip.hci_revision = cpd_info->local_chip_info.hci_revision;
			cpd_info->chip_dev.chip.hci_sub_version = cpd_info->local_chip_info.lmp_pal_subversion;
			cpd_info->chip_dev.chip.manufacturer = cpd_info->local_chip_info.manufacturer;

			memset(&(cpd_info->chip_dev.callbacks), 0, sizeof(cpd_info->chip_dev.callbacks));

			list_for_each(cursor, &cpd_chip_handlers) {
				tmp = list_entry(cursor, struct chip_handler_item, list);
				chip_handled = tmp->callbacks.check_chip_support(&(cpd_info->chip_dev));
				if (chip_handled) {
					STE_CONN_INFO("Chip handler found");
					break;
				}
			}

			if (cpd_info->main_state == CPD_STATE_INITIALIZING) {
				/* We are now finished with the start-up during HwRegistered operation */
				CPD_SET_BOOT_STATE(BOOT_STATE_READY);
				wake_up_interruptible(&ste_conn_cpd_wait_queue);
			} else if (!chip_handled) {
				STE_CONN_INFO("No chip handler found. Start-up complete");
				CPD_SET_BOOT_STATE(BOOT_STATE_READY);
				ste_conn_cpd_chip_startup_finished(0);
			} else {
				if (!cpd_info->chip_dev.callbacks.chip_startup) {
					ste_conn_cpd_chip_startup_finished(0);
				} else {
					err = cpd_info->chip_dev.callbacks.chip_startup(&(cpd_info->chip_dev));
					if (err) {
						ste_conn_cpd_chip_startup_finished(err);
					}
				}
			}
		} else {
			STE_CONN_ERR("Received Read Local Version Information with status 0x%X", status);
			CPD_SET_BOOT_STATE(BOOT_STATE_FAILED);
			ste_conn_reset(NULL);
		}
		/* We have now handled the packet */
		pkt_handled = true;
	}

	return pkt_handled;
}

/**
 * cpd_create_and_send_bt_cmd() - Copy and send sk_buffer.
 * @data:   Data to send.
 * @length: Length in bytes of data.
 *
 * The cpd_create_and_send_bt_cmd() function allocates sk_buffer, copy supplied data to it,
 * and send the sk_buffer to CCD. Note that the data must contain the H:4 header.
 */
static void cpd_create_and_send_bt_cmd(const uint8_t *data, int length)
{
	struct sk_buff *skb;

	skb = alloc_skb(length, GFP_ATOMIC);
	if (skb) {
		memcpy(skb_put(skb, length), data, length);
		skb->data[0] = HCI_BT_CMD_H4_CHANNEL;
		cpd_transmit_skb_to_ccd(skb, cpd_info->hci_logger_config.bt_cmd_enable);
	} else {
		STE_CONN_ERR("Couldn't allocate sk_buff with length %d", length);
	}
}

/**
 * cpd_transmit_skb_to_ccd() - Transmit buffer to CCD.
 * @skb:        Data packet.
 * @use_logger: True if HCI logger shall be used, false otherwise.
 *
 * The cpd_transmit_skb_to_ccd() function transmit buffer to CCD.
 * If enabled, copy the transmitted data to the HCI logger as well.
 */
static void cpd_transmit_skb_to_ccd(struct sk_buff *skb, bool use_logger)
{
	/* If HCI logging is enabled for this channel, copy the data to
	 * the HCI logging output.
	 */
	if (use_logger && cpd_info->users.hci_logger) {
		/* Alloc a new sk_buffer and copy the data into it. Then send it to the HCI logger. */
		struct sk_buff *skb_log = alloc_skb(skb->len + 1, GFP_ATOMIC);
		if (skb_log) {
			memcpy(skb_put(skb_log, skb->len), skb->data, skb->len);
			skb_log->data[0] = (uint8_t) LOGGER_DIRECTION_TX;

			cpd_info->users.hci_logger->callbacks->read_cb(cpd_info->users.hci_logger, skb_log);
		} else {
			STE_CONN_ERR("Couldn't allocate skb_log");
		}
	}

	ste_conn_ccd_write(skb);
}

/**
 * cpd_handle_reset_of_user - Calls the reset callback and frees the device.
 * @dev:  Pointer to ste_conn device.
 */
static void cpd_handle_reset_of_user(struct ste_conn_device **dev)
{
	if (*dev) {
		if ((*dev)->callbacks->reset_cb) {
			(*dev)->callbacks->reset_cb((*dev));
		}
		cpd_free_user_dev(dev);
	}
}

/**
 * cpd_free_user_dev - Frees user device and also sets it to NULL to inform caller.
 * @dev:    Pointer to user device.
 */
static void cpd_free_user_dev(struct ste_conn_device **dev)
{
	if (*dev) {
		if ((*dev)->callbacks) {
			kfree((*dev)->callbacks);
		}
		kfree(*dev);
		*dev = NULL;
	}
}

MODULE_AUTHOR("Par-Gunnar Hjalmdahl ST-Ericsson");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Linux Bluetooth HCI H:4 Connectivity Device Driver");
