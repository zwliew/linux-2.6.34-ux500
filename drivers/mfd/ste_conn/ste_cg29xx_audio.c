/*
 * drivers/mfd/ste_conn/ste_cg29xx_audio.c
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Authors:
 * Par-Gunnar Hjalmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson.
 * License terms:  GNU General Public License (GPL), version 2
 *
 * Linux Bluetooth Audio Driver for ST-Ericsson controller.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/mfd/ste_conn.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/skbuff.h>
#include <linux/list.h>
#include <linux/sched.h>

#include <linux/mfd/ste_cg29xx_audio.h>
#include "ste_conn_debug.h"
#include "bluetooth_defines.h"
#include "ste_cg2900.h"

#define VERSION "1.0"

/* Char device op codes */
#define CHAR_DEV_OP_CODE_SET_DAI_CONF			0x01
#define CHAR_DEV_OP_CODE_GET_DAI_CONF			0x02
#define CHAR_DEV_OP_CODE_CONFIGURE_ENDPOINT		0x03
#define CHAR_DEV_OP_CODE_CONNECT_AND_START_STREAM	0x04
#define CHAR_DEV_OP_CODE_STOP_STREAM			0x05

/* Device names */
#define STE_CG29XX_AUDIO_DEVICE_NAME			"ste_cg29xx_audio"

/* Type of channel used */
#define BT_CHANNEL_USED					0x00
#define FM_CHANNEL_USED					0x01

#define CG29XX_AUDIO_MAX_NBR_OF_USERS			10
#define CG29XX_AUDIO_FIRST_USER				1

#define FM_RX_STREAM_HANDLE				10
#define FM_TX_STREAM_HANDLE				20

#define SET_RESP_STATE(__state_var, __new_state) \
	STE_CONN_SET_STATE("resp_state", __state_var, __new_state)

/**
 * enum chip_response_state - State when communicating with the CG29XX controller.
 *  @IDLE: No outstanding packets to the controller.
 *  @WAITING: Packet has been sent to the controller. Waiting for response.
 *  @RESP_RECEIVED: Response from controller has been received but not yet handled.
 */
enum chip_response_state {
	IDLE,
	WAITING,
	RESP_RECEIVED
};

/**
 * enum cg29xx_audio_main_state - Main state for the CG29XX Audio driver.
 *  @OPENED: Audio driver has registered to STE_CONN.
 *  @CLOSED: Audio driver is not registered to STE_CONN.
 *  @RESET: A reset of STE_CONN has occurred and no user has re-opened the audio driver.
 */
enum cg29xx_audio_main_state {
	OPENED,
	CLOSED,
	RESET
};

/**
 * struct cg29xx_audio_char_dev_info - CG29XX character device info structure.
 *  @session: Stored session for the char device.
 *  @stored_data: Data returned when executing last command, if any.
 *  @stored_data_len: Length of @stored_data in bytes.
 *  @management_mutex: Mutex for handling access to char dev management.
 *  @rw_mutex: Mutex for handling access to char dev writes and reads.
 */
struct cg29xx_audio_char_dev_info {
	int		session;
	uint8_t		*stored_data;
	int		stored_data_len;
	struct mutex	management_mutex;
	struct mutex	rw_mutex;
};

/**
 * struct cg29xx_audio_user - CG29XX audio user info structure.
 *  @session: Stored session for the char device.
 *  @resp_state: State for controller communications.
 */
struct cg29xx_audio_user {
	int				session;
	enum chip_response_state	resp_state;
};

/**
 * struct endpoint_list - List for storing endpoint configuration nodes.
 *  @ep_list: Pointer to first node in list.
 *  @management_mutex: Mutex for handling access to list.
 */
struct endpoint_list {
	struct list_head		ep_list;
	struct mutex			management_mutex;
};

/**
 * struct endpoint_config_node - Node for storing endpoint configuration.
 *  @list:		list_head struct.
 *  @endpoint_id:	Endpoint ID.
 *  @config:		Stored configuration for this endpoint.
 */
struct endpoint_config_node {
	struct list_head list;
	enum ste_cg29xx_audio_endpoint_id endpoint_id;
	union ste_cg29xx_audio_endpoint_configuration_union config;
};

/**
 * struct ste_cg29xx_audio_info - Main CG29XX Audio driver info structure.
 *  @state: Current state of the CG29XX Audio driver.
 *  @dev: The misc device created by this driver.
 *  @ste_conn_dev_bt: STE_CONN device registered by this driver for the BT audio channel.
 *  @ste_conn_dev_fm: STE_CONN device registered by this driver for the FM audio channel.
 *  @management_mutex: Mutex for handling access to CG29XX Audio driver management.
 *  @bt_channel_mutex: Mutex for handling access to BT audio channel.
 *  @fm_channel_mutex: Mutex for handling access to FM audio channel.
 *  @nbr_of_users_active: Number of sessions open in the CG29XX Audio driver.
 *  @bt_queue: Received BT events.
 *  @fm_queue: Received FM events.
 *  @audio_sessions: Pointers to currently opened sessions (maps session ID to user info).
 *  @i2s_config: I2S configuration.
 *  @i2s_pcm_config: PCM_I2S configuration.
 *  @i2s_config_known: @true if @i2s_config has been set, @false otherwise.
 *  @i2s_pcm_config_known: @true if @i2s_pcm_config has been set, @false otherwise.
 *  @endpoints: List containing the endpoint configurations.
 */
struct ste_cg29xx_audio_info {
	enum cg29xx_audio_main_state		state;
	struct miscdevice			dev;
	struct ste_conn_device			*ste_conn_dev_bt;
	struct ste_conn_device			*ste_conn_dev_fm;
	struct mutex				management_mutex;
	struct mutex				bt_channel_mutex;
	struct mutex				fm_channel_mutex;
	int					nbr_of_users_active;
	struct sk_buff_head			bt_queue;
	struct sk_buff_head			fm_queue;
	struct cg29xx_audio_user		*audio_sessions[CG29XX_AUDIO_MAX_NBR_OF_USERS];
	/* DAI configurations */
	struct ste_cg29xx_dai_port_conf_i2s	i2s_config;
	struct ste_cg29xx_dai_port_conf_i2s_pcm	i2s_pcm_config;
	bool					i2s_config_known;
	bool					i2s_pcm_config_known;
	/* Endpoint configurations */
	struct endpoint_list			endpoints;
};

/**
 * struct cg29xx_audio_cb_info - Callback info structure registered in STE_CONN @user_data.
 *  @channel: Stores if this device handles BT or FM events.
 *  @user: Audio user currently awaiting data on the channel.
 */
struct cg29xx_audio_cb_info {
	int				channel;
	struct cg29xx_audio_user	*user;
};

/* Generic internal functions */
static void ste_cg29xx_audio_read_cb(struct ste_conn_device *dev, struct sk_buff *skb);
static void ste_cg29xx_audio_reset_cb(struct ste_conn_device *dev);
static struct cg29xx_audio_user *get_session_user(int session);

/* Endpoint list functions */
static void add_endpoint(struct ste_cg29xx_audio_endpoint_configuration *ep_config,
			 struct endpoint_list *list);
static void del_endpoint_private(enum ste_cg29xx_audio_endpoint_id endpoint_id,
				 struct endpoint_list *list);
static union ste_cg29xx_audio_endpoint_configuration_union *find_endpoint(enum ste_cg29xx_audio_endpoint_id endpoint_id,
									  struct endpoint_list *list);
static void flush_endpoint_list(struct endpoint_list *list);

/* Internal endpoint connection functions */
static int conn_start_i2s_to_fm_rx(struct cg29xx_audio_user *audio_user, unsigned int *stream_handle);
static int conn_start_i2s_to_fm_tx(struct cg29xx_audio_user *audio_user, unsigned int *stream_handle);
static int conn_start_pcm_to_sco(struct cg29xx_audio_user *audio_user, unsigned int *stream_handle);
static int conn_stop_pcm_to_sco(struct cg29xx_audio_user *audio_user, unsigned int stream_handle);

/* Character device functions for userspace module test */
static int ste_cg29xx_audio_char_device_open(struct inode *inode,
					struct file *filp);
static int ste_cg29xx_audio_char_device_release(struct inode *inode,
					struct file *filp);
static ssize_t ste_cg29xx_audio_char_device_read(struct file *filp,
						char __user *buf, size_t count,
							loff_t 	*f_pos);
static ssize_t ste_cg29xx_audio_char_device_write(struct file *filp,
						const char __user *buf, size_t count,
							loff_t *f_pos);
static unsigned int ste_cg29xx_audio_char_device_poll(struct file *filp,
						poll_table *wait);

static int __init ste_cg2900_audio_init(void);
static void __exit ste_cg2900_audio_exit(void);

static struct timeval time_5s = {
	.tv_sec = 5,
	.tv_usec = 0
};

/* ste_cg29xx_audio wait queues */
static DECLARE_WAIT_QUEUE_HEAD(cg29xx_audio_wq_bt);
static DECLARE_WAIT_QUEUE_HEAD(cg29xx_audio_wq_fm);

static struct ste_cg29xx_audio_info *cg29xx_audio_info;

static const struct file_operations char_dev_fops = {
	.open = ste_cg29xx_audio_char_device_open,
	.release = ste_cg29xx_audio_char_device_release,
	.read = ste_cg29xx_audio_char_device_read,
	.write = ste_cg29xx_audio_char_device_write,
	.poll = ste_cg29xx_audio_char_device_poll
};

static struct ste_conn_callbacks ste_cg29xx_audio_cb = {
	.read_cb = ste_cg29xx_audio_read_cb,
	.reset_cb = ste_cg29xx_audio_reset_cb
};

static struct cg29xx_audio_cb_info cb_info_bt = {
	.channel = BT_CHANNEL_USED,
	.user = NULL
};
static struct cg29xx_audio_cb_info cb_info_fm = {
	.channel = FM_CHANNEL_USED,
	.user = NULL
};

/*
 * 	Internal helper functions
 */

/**
 * ste_cg29xx_audio_read_cb() - Handle data received from STE connectivity driver.
 * @dev: Device receiving data.
 * @skb: Buffer with data coming form device.
 */
static void ste_cg29xx_audio_read_cb(struct ste_conn_device *dev, struct sk_buff *skb)
{
	struct cg29xx_audio_cb_info *cb_info;

	STE_CONN_INFO("ste_cg29xx_audio_read_cb");

	if (!dev) {
		STE_CONN_ERR("NULL supplied as dev");
		return;
	}

	if (!skb) {
		STE_CONN_ERR("NULL supplied as skb");
		return;
	}

	cb_info = (struct cg29xx_audio_cb_info *)dev->user_data;
	if (!cb_info) {
		STE_CONN_ERR("NULL supplied as cb_info");
		return;
	}
	if (!(cb_info->user)) {
		STE_CONN_ERR("NULL supplied as cb_info->user");
		return;
	}

	/* Mark that packet has been received */
	SET_RESP_STATE(cb_info->user->resp_state, RESP_RECEIVED);

	/* Handle packet depending on channel */
	if (cb_info->channel == BT_CHANNEL_USED) {
		skb_queue_tail(&(cg29xx_audio_info->bt_queue), skb);
		wake_up_interruptible(&cg29xx_audio_wq_bt);
	} else if (cb_info->channel == FM_CHANNEL_USED) {
		skb_queue_tail(&(cg29xx_audio_info->fm_queue), skb);
		wake_up_interruptible(&cg29xx_audio_wq_fm);
	} else {
		/* Unhandled channel; free the packet */
		STE_CONN_ERR("Received callback on bad channel %d", cb_info->channel);
		kfree_skb(skb);
	}
}

/**
 * ste_cg29xx_audio_reset_cb() - Reset callback function.
 * @dev: CPD device reseting.
 */
static void ste_cg29xx_audio_reset_cb(struct ste_conn_device *dev)
{
	STE_CONN_INFO("ste_cg29xx_audio_reset_cb");
	mutex_lock(&cg29xx_audio_info->management_mutex);
	cg29xx_audio_info->nbr_of_users_active = 0;
	cg29xx_audio_info->state = RESET;
	mutex_unlock(&cg29xx_audio_info->management_mutex);
}

/**
 * get_session_user() - Check that supplied session is within valid range.
 * @session: Session ID.
 *
 * Returns:
 *   Audio_user if there is no error.
 *   NULL for bad session ID.
 */
static struct cg29xx_audio_user *get_session_user(int session)
{
	struct cg29xx_audio_user *audio_user = NULL;

	if (session < CG29XX_AUDIO_FIRST_USER || session >= CG29XX_AUDIO_MAX_NBR_OF_USERS) {
		STE_CONN_ERR("Calling with invalid session %d", session);
		goto finished;
	}

	audio_user = cg29xx_audio_info->audio_sessions[session];
	if (!audio_user) {
		STE_CONN_ERR("Calling with non-opened session %d", session);
	}

finished:
	return audio_user;
}

/**
 * add_endpoint() - Add endpoint node to the supplied list and copies supplied config to node.
 * @ep_config:  Endpoint configuration.
 * @list:  List of endpoints.
 *
 * If a node already exists for the supplied endpoint, the old node is removed
 * and replaced by the new node.
 */
static void add_endpoint(struct ste_cg29xx_audio_endpoint_configuration *ep_config,
			 struct endpoint_list *list)
{
	struct endpoint_config_node *item = kzalloc(sizeof(*item), GFP_KERNEL);

	if (item) {
		/* Store values */
		item->endpoint_id = ep_config->endpoint_id;
		memcpy(&(item->config), &(ep_config->config), sizeof(item->config));

		mutex_lock(&(list->management_mutex));

		/* Check if endpoint ID already exist in list. If that is the case, remove it. */
		if (!list_empty(&(list->ep_list))) {
			del_endpoint_private(ep_config->endpoint_id, list);
		}

		list_add_tail(&(item->list), &(list->ep_list));

		mutex_unlock(&(list->management_mutex));
	} else {
		STE_CONN_ERR("Failed to alloc memory!");
	}
}


/**
 * del_endpoint_private() - Deletes an endpoint from the supplied endpoint list.
 * @endpoint_id:  Endpoint ID.
 * @list:  List of endpoints.
 *
 * This function is not protected by any semaphore.
 */
static void del_endpoint_private(enum ste_cg29xx_audio_endpoint_id endpoint_id,
				 struct endpoint_list *list)
{
	struct list_head *cursor, *next;
	struct endpoint_config_node *tmp;

	list_for_each_safe(cursor, next, &(list->ep_list)) {
		tmp = list_entry(cursor, struct endpoint_config_node, list);
		if (tmp->endpoint_id == endpoint_id) {
			list_del(cursor);
			kfree(tmp);
		}
	}
}

/**
 * find_endpoint() - Finds endpoint identified by @endpoint_id in the supplied list.
 * @endpoint_id:  Endpoint ID.
 * @list:  List of endpoints.
 *
 * Returns:
 *   Endpoint configuration if there is no error.
 *   NULL if no configuration can be found for @endpoint_id.
 */
static union ste_cg29xx_audio_endpoint_configuration_union *find_endpoint(enum ste_cg29xx_audio_endpoint_id endpoint_id,
									  struct endpoint_list *list)
{
	struct list_head *cursor, *next;
	struct endpoint_config_node *tmp;
	struct endpoint_config_node *ret_ep = NULL;

	mutex_lock(&list->management_mutex);

	list_for_each_safe(cursor, next, &(list->ep_list)) {
		tmp = list_entry(cursor, struct endpoint_config_node, list);
		if (tmp->endpoint_id == endpoint_id) {
			ret_ep = tmp;
		}
	}
	mutex_unlock(&list->management_mutex);

	if (ret_ep) {
		return &(ret_ep->config);
	} else {
		return NULL;
	}
}

/**
 * flush_endpoint_list() - Deletes all stored endpoints in the supplied endpoint list.
 * @list:  List of endpoints.
 */
static void flush_endpoint_list(struct endpoint_list *list)
{
	struct list_head *cursor, *next;
	struct endpoint_config_node *tmp;

	mutex_lock(&list->management_mutex);
	list_for_each_safe(cursor, next, &(list->ep_list)) {
		tmp = list_entry(cursor, struct endpoint_config_node, list);
		list_del(cursor);
		kfree(tmp);
	}
	mutex_unlock(&list->management_mutex);
}

/**
 * receive_fm_legacy_response() - Wait for and handle an FM Legacy response.
 * @audio_user:  Audio user to check for.
 * @resp_lsb:  LSB of FM command to wait for.
 * @resp_msb:  MSB of FM command to wait for.
 *
 * This function first waits for FM response (up to 5 seconds) and when one arrives,
 * it checks that it is the one we are waiting for and also that no error has occurred.
 *
 * Returns:
 *   0 if there is no error.
 *   -ECOMM if no response was received.
 *   -EIO for other errors.
 */
static int receive_fm_legacy_response(struct cg29xx_audio_user *audio_user, uint8_t resp_lsb, uint8_t resp_msb)
{
	int err = 0;
	struct sk_buff *skb;

	/* Wait for callback to receive command complete and then wake us up again. */
	if (0 > wait_event_interruptible_timeout(cg29xx_audio_wq_fm,
			audio_user->resp_state == RESP_RECEIVED,
			timeval_to_jiffies(&time_5s))) {
		/* We timed out or an error occurred */
		STE_CONN_ERR("Error occurred while waiting for return packet.");
		err = -ECOMM;
		goto finished;
	} else {
		/* OK, now we should have received answer. Let's check it. */
		skb = skb_dequeue_tail(&cg29xx_audio_info->fm_queue);
		if (!skb) {
			STE_CONN_ERR("No skb in queue when it should be there");
			err = -EIO;
			goto finished;
		}

		/* Check if we received the correct event */
		if (CG2900_FM_GEN_OPCODE_LEGACY_API == skb->data[CG2900_FM_USER_GEN_OPCODE_OFFSET_CMD_CMPL]) {
			/* FM Legacy Command complete event */
			uint8_t status = skb->data[CG2900_FM_USER_LEG_STATUS_OFFSET_CMD_CMPL];
			uint8_t fm_function = skb->data[CG2900_FM_USER_LEG_FUNC_OFFSET_CMD_CMPL];
			uint8_t fm_resp_hdr_lsb = skb->data[CG2900_FM_USER_LEG_HDR_OFFSET_CMD_CMPL];
			uint8_t fm_resp_hdr_msb = skb->data[CG2900_FM_USER_LEG_HDR_OFFSET_CMD_CMPL + 1];

			if (fm_function != CG2900_FM_CMD_PARAMETERS_WRITECOMMAND ||
			    fm_resp_hdr_lsb != resp_lsb || fm_resp_hdr_msb != resp_msb) {
				STE_CONN_ERR("Received unexpected packet func 0x%X cmd 0x%02X%02X",
					     fm_function, fm_resp_hdr_msb, fm_resp_hdr_lsb);
				err = -EIO;
				goto error_handling_free_skb;
			}

			if (status != CG2900_FM_CMD_STATUS_COMMAND_SUCCEEDED) {
				STE_CONN_ERR("FM Command failed (%d)", status);
				err = -EIO;
				goto error_handling_free_skb;
			}
			/* Operation succeeded. We are now done */
		} else {
			STE_CONN_ERR("Received unknown FM packet. 0x%X %X %X %X %X",
				     skb->data[0], skb->data[1], skb->data[2], skb->data[3], skb->data[4]);
			err = -EIO;
			goto error_handling_free_skb;
		}
	}

error_handling_free_skb:
	kfree_skb(skb);
finished:
	return err;
}

/**
 * receive_bt_cmd_complete() - Wait for and handle an BT Command Complete event.
 * @audio_user:  Audio user to check for.
 * @resp_lsb:  LSB of BT command to wait for.
 * @resp_msb:  MSB of BT command to wait for.
 * @data:  Pointer to buffer if any received data should be stored (except status).
 * @data_len:  Length of @data in bytes.
 *
 * This function first waits for BT Command Complete event (up to 5 seconds)
 * and when one arrives, it checks that it is the one we are waiting for and
 * also that no error has occurred.
 * If @data is supplied it also copies received data into @data.
 *
 * Returns:
 *   0 if there is no error.
 *   -ECOMM if no response was received.
 *   -EIO for other errors.
 */
static int receive_bt_cmd_complete(struct cg29xx_audio_user *audio_user, uint8_t resp_lsb,
				   uint8_t resp_msb, void *data, int data_len)
{
	int err = 0;
	struct sk_buff *skb;

	/* Wait for callback to receive command complete and then wake us up again. */
	if (0 > wait_event_interruptible_timeout(cg29xx_audio_wq_bt,
			audio_user->resp_state == RESP_RECEIVED,
			timeval_to_jiffies(&time_5s))) {
		/* We timed out or an error occurred */
		STE_CONN_ERR("Error occurred while waiting for return packet.");
		err = -ECOMM;
		goto finished;
	} else {
		uint8_t evt_id;

		/* OK, now we should have received answer. Let's check it. */
		skb = skb_dequeue_tail(&cg29xx_audio_info->bt_queue);
		if (!skb) {
			STE_CONN_ERR("No skb in queue when it should be there");
			err = -EIO;
			goto finished;
		}
		evt_id = skb->data[HCI_BT_EVT_ID_OFFSET];
		if (evt_id == HCI_BT_EVT_CMD_COMPLETE) {
			uint8_t op_lsb = skb->data[HCI_BT_EVT_CMD_COMPL_OP_CODE_OFFSET];
			uint8_t op_msb = skb->data[HCI_BT_EVT_CMD_COMPL_OP_CODE_OFFSET + 1];
			uint8_t status = skb->data[HCI_BT_EVT_CMD_COMPL_STATUS_OFFSET];

			if (resp_lsb != op_lsb || resp_msb != op_msb) {
				STE_CONN_ERR("Received cmd complete for unexpected command: 0x%02X%02X",
					     op_msb, op_lsb);
				err = -EIO;
				goto error_handling_free_skb;
			}
			if (status != HCI_BT_ERROR_NO_ERROR) {
				STE_CONN_ERR("Received command complete with err %d", status);
				err = -EIO;
				goto error_handling_free_skb;
			}
			/* Copy the rest of the parameters if a buffer has been supplied.
			 * The caller must have set the length correctly.
			 */
			if (data) {
				memcpy(data, &(skb->data[HCI_BT_EVT_CMD_COMPL_STATUS_OFFSET + 1]), data_len);
			}
			/* Operation succeeded. We are now done */
		} else {
			STE_CONN_ERR("We did not receive the event we expected (0x%X)", evt_id);
			err = -EIO;
			goto error_handling_free_skb;
		}
	}

error_handling_free_skb:
	kfree_skb(skb);
finished:
	return err;
}

/**
 * conn_start_i2s_to_fm_rx() - Start an audio stream connecting FM RX to I2S.
 * @audio_user:  Audio user to check for.
 * @stream_handle: [out] Pointer where to store the stream handle.
 *
 * This function sets up an FM RX to I2S stream.
 * It does this by first setting the output mode and then the configuration of
 * the External Sample Rate Converter.
 *
 * Returns:
 *   0 if there is no error.
 *   -ECOMM if no response was received.
 *   -ENOMEM upon allocation errors.
 *   Errors from @ste_conn_write and @receive_fm_legacy_response.
 *   -EIO for other errors.
 */
static int conn_start_i2s_to_fm_rx(struct cg29xx_audio_user *audio_user, unsigned int *stream_handle)
{
	int err = 0;
	union ste_cg29xx_audio_endpoint_configuration_union *fm_config;
	struct sk_buff *skb;
	uint8_t *data;

	fm_config = find_endpoint(STE_CG29XX_AUDIO_ENDPOINT_ID_FM_RX, &(cg29xx_audio_info->endpoints));
	if (!fm_config) {
		STE_CONN_ERR("FM RX not configured before stream start");
		err = -EIO;
		goto finished;
	}

	if (!(cg29xx_audio_info->i2s_config_known)) {
		STE_CONN_ERR("I2S DAI not configured before stream start");
		err = -EIO;
		goto finished;
	}

	/* Use mutex to assure that only ONE command is sent at any time on each channel */
	mutex_lock(&cg29xx_audio_info->fm_channel_mutex);

	/*
	 * Now set the output mode of the External Sample Rate Converter by
	 * sending HCI_Write command with AUP_EXT_SetMode.
	 */
	STE_CONN_DBG("FM: AUP_EXT_SetMode");

	skb = ste_conn_alloc_skb(CG2900_FM_CMD_LEN_AUP_EXT_SET_MODE, GFP_KERNEL);
	if (!skb) {
		STE_CONN_ERR("Could not allocate skb");
		err = -ENOMEM;
		goto finished_unlock_mutex;
	}

	/* Enter data into the skb */
	data = skb_put(skb, CG2900_FM_CMD_LEN_AUP_EXT_SET_MODE);

	data[0] = CG2900_FM_CMD_LEN_AUP_EXT_SET_MODE - 1; /* Length */
	data[1] = CG2900_FM_GEN_OPCODE_LEGACY_API;
	data[2] = CG2900_FM_CMD_LEG_PARAMETERS_WRITE;
	data[3] = CG2900_FM_CMD_PARAMETERS_WRITECOMMAND;
	data[4] = CG2900_FM_CMD_ID_AUP_EXT_SET_MODE_LSB;
	data[5] = CG2900_FM_CMD_ID_AUP_EXT_SET_MODE_MSB;
	data[6] = HCI_SET_U16_DATA_LSB(CG2900_FM_CMD_AUP_EXT_SET_MODE_I2S);
	data[7] = HCI_SET_U16_DATA_MSB(CG2900_FM_CMD_AUP_EXT_SET_MODE_I2S);

	cb_info_fm.user = audio_user;
	SET_RESP_STATE(audio_user->resp_state, WAITING);

	/* Send packet to controller */
	err = ste_conn_write(cg29xx_audio_info->ste_conn_dev_fm, skb);
	if (err) {
		STE_CONN_ERR("Error occurred while waiting for return packet.");
		goto error_handling_free_skb;
	}

	err = receive_fm_legacy_response(audio_user,
					 CG2900_FM_RSP_ID_AUP_EXT_SET_MODE_LSB,
					 CG2900_FM_RSP_ID_AUP_EXT_SET_MODE_MSB);
	if (err) {
		goto finished_unlock_mutex;
	}

	SET_RESP_STATE(audio_user->resp_state, IDLE);

	/*
	 * Now configure the External Sample Rate Converter by sending
	 * HCI_Write command with AUP_EXT_SetControl.
	 */
	STE_CONN_DBG("FM: AUP_EXT_SetControl");

	skb = ste_conn_alloc_skb(CG2900_FM_CMD_LEN_AUP_EXT_SET_CONTROL, GFP_KERNEL);
	if (!skb) {
		STE_CONN_ERR("Could not allocate skb");
		err = -ENOMEM;
		goto finished_unlock_mutex;
	}

	/* Enter data into the skb */
	data = skb_put(skb, CG2900_FM_CMD_LEN_AUP_EXT_SET_CONTROL);

	data[0] = CG2900_FM_CMD_LEN_AUP_EXT_SET_CONTROL - 1; /* Length */
	data[1] = CG2900_FM_GEN_OPCODE_LEGACY_API;
	data[2] = CG2900_FM_CMD_LEG_PARAMETERS_WRITE;
	data[3] = CG2900_FM_CMD_PARAMETERS_WRITECOMMAND;
	data[4] = CG2900_FM_CMD_ID_AUP_EXT_SET_CONTROL_LSB;
	data[5] = CG2900_FM_CMD_ID_AUP_EXT_SET_CONTROL_MSB;
	if (fm_config->fm.sample_rate >= STE_CG29XX_AUDIO_ENDPOINT_CONFIGURATION_SAMPLE_RATE_44_1_KHZ) {
		data[6] = HCI_SET_U16_DATA_LSB(CG2900_FM_CMD_SET_CONTROL_CONVERSION_UP);
		data[7] = HCI_SET_U16_DATA_MSB(CG2900_FM_CMD_SET_CONTROL_CONVERSION_UP);
	} else {
		data[6] = HCI_SET_U16_DATA_LSB(CG2900_FM_CMD_SET_CONTROL_CONVERSION_DOWN);
		data[7] = HCI_SET_U16_DATA_MSB(CG2900_FM_CMD_SET_CONTROL_CONVERSION_DOWN);
	}

	cb_info_fm.user = audio_user;
	SET_RESP_STATE(audio_user->resp_state, WAITING);

	/* Send packet to controller */
	err = ste_conn_write(cg29xx_audio_info->ste_conn_dev_fm, skb);
	if (err) {
		STE_CONN_ERR("Error occurred while waiting for return packet.");
		goto error_handling_free_skb;
	}

	err = receive_fm_legacy_response(audio_user,
					 CG2900_FM_RSP_ID_AUP_EXT_SET_CONTROL_LSB,
					 CG2900_FM_RSP_ID_AUP_EXT_SET_CONTROL_MSB);
	if (err) {
		goto finished_unlock_mutex;
	}

	/* So far we only support one FM RX stream. Set that handle. */
	*stream_handle = FM_RX_STREAM_HANDLE;
	STE_CONN_DBG("stream_handle set to %d", *stream_handle);

	goto finished_unlock_mutex;

error_handling_free_skb:
	kfree_skb(skb);
finished_unlock_mutex:
	SET_RESP_STATE(audio_user->resp_state, IDLE);
	mutex_unlock(&cg29xx_audio_info->fm_channel_mutex);
finished:
	return err;
}

/**
 * conn_start_i2s_to_fm_tx() - Start an audio stream connecting FM TX to I2S.
 * @audio_user:  Audio user to check for.
 * @stream_handle: [out] Pointer where to store the stream handle.
 *
 * This function sets up an I2S to FM TX stream.
 * It does this by first setting the Audio Input source and then setting the
 * configuration and input source of BT sample rate converter.
 *
 * Returns:
 *   0 if there is no error.
 *   -ECOMM if no response was received.
 *   -ENOMEM upon allocation errors.
 *   Errors from @ste_conn_write and @receive_fm_legacy_response.
 *   -EIO for other errors.
 */
static int conn_start_i2s_to_fm_tx(struct cg29xx_audio_user *audio_user, unsigned int *stream_handle)
{
	int err = 0;
	union ste_cg29xx_audio_endpoint_configuration_union *fm_config;
	struct sk_buff *skb;
	uint8_t *data;

	fm_config = find_endpoint(STE_CG29XX_AUDIO_ENDPOINT_ID_FM_TX, &(cg29xx_audio_info->endpoints));
	if (!fm_config) {
		STE_CONN_ERR("FM TX not configured before stream start");
		err = -EIO;
		goto finished;
	}

	if (!(cg29xx_audio_info->i2s_config_known)) {
		STE_CONN_ERR("I2S DAI not configured before stream start");
		err = -EIO;
		goto finished;
	}

	/* Use mutex to assure that only ONE command is sent at any time on each channel */
	mutex_lock(&cg29xx_audio_info->fm_channel_mutex);

	/*
	 * Select Audio Input Source by sending HCI_Write command with
	 * AIP_SetMode.
	 */
	STE_CONN_DBG("FM: AIP_SetMode");

	skb = ste_conn_alloc_skb(CG2900_FM_CMD_LEN_AIP_SET_MODE, GFP_KERNEL);
	if (!skb) {
		STE_CONN_ERR("Could not allocate skb");
		err = -ENOMEM;
		goto finished_unlock_mutex;
	}

	/* Enter data into the skb */
	data = skb_put(skb, CG2900_FM_CMD_LEN_AIP_SET_MODE);

	data[0] = CG2900_FM_CMD_LEN_AIP_SET_MODE - 1; /* Length */
	data[1] = CG2900_FM_GEN_OPCODE_LEGACY_API;
	data[2] = CG2900_FM_CMD_LEG_PARAMETERS_WRITE;
	data[3] = CG2900_FM_CMD_PARAMETERS_WRITECOMMAND;
	data[4] = CG2900_FM_CMD_ID_AIP_SET_MODE_LSB;
	data[5] = CG2900_FM_CMD_ID_AIP_SET_MODE_MSB;
	data[6] = HCI_SET_U16_DATA_LSB(CG2900_FM_CMD_AIP_SET_MODE_INPUT_DIGITAL);
	data[7] = HCI_SET_U16_DATA_MSB(CG2900_FM_CMD_AIP_SET_MODE_INPUT_DIGITAL);

	cb_info_fm.user = audio_user;
	SET_RESP_STATE(audio_user->resp_state, WAITING);

	/* Send packet to controller */
	err = ste_conn_write(cg29xx_audio_info->ste_conn_dev_fm, skb);
	if (err) {
		STE_CONN_ERR("Error occurred while waiting for return packet.");
		goto error_handling_free_skb;
	}

	err = receive_fm_legacy_response(audio_user,
					 CG2900_FM_RSP_ID_AIP_SET_MODE_LSB,
					 CG2900_FM_RSP_ID_AIP_SET_MODE_MSB);
	if (err) {
		goto finished_unlock_mutex;
	}

	SET_RESP_STATE(audio_user->resp_state, IDLE);

	/*
	 * Now configure the BT sample rate converter by sending HCI_Write
	 * command with AIP_BT_SetControl.
	 */
	STE_CONN_DBG("FM: AIP_BT_SetControl");

	skb = ste_conn_alloc_skb(CG2900_FM_CMD_LEN_AIP_BT_SET_CONTROL, GFP_KERNEL);
	if (!skb) {
		STE_CONN_ERR("Could not allocate skb");
		err = -ENOMEM;
		goto finished_unlock_mutex;
	}

	/* Enter data into the skb */
	data = skb_put(skb, CG2900_FM_CMD_LEN_AIP_BT_SET_CONTROL);

	data[0] = CG2900_FM_CMD_LEN_AIP_BT_SET_CONTROL - 1; /* Length */
	data[1] = CG2900_FM_GEN_OPCODE_LEGACY_API;
	data[2] = CG2900_FM_CMD_LEG_PARAMETERS_WRITE;
	data[3] = CG2900_FM_CMD_PARAMETERS_WRITECOMMAND;
	data[4] = CG2900_FM_CMD_ID_AIP_BT_SET_CONTROL_LSB;
	data[5] = CG2900_FM_CMD_ID_AIP_BT_SET_CONTROL_MSB;
	if (fm_config->fm.sample_rate >= STE_CG29XX_AUDIO_ENDPOINT_CONFIGURATION_SAMPLE_RATE_44_1_KHZ) {
		data[6] = HCI_SET_U16_DATA_LSB(CG2900_FM_CMD_SET_CONTROL_CONVERSION_UP);
		data[7] = HCI_SET_U16_DATA_MSB(CG2900_FM_CMD_SET_CONTROL_CONVERSION_UP);
	} else {
		data[6] = HCI_SET_U16_DATA_LSB(CG2900_FM_CMD_SET_CONTROL_CONVERSION_DOWN);
		data[7] = HCI_SET_U16_DATA_MSB(CG2900_FM_CMD_SET_CONTROL_CONVERSION_DOWN);
	}

	cb_info_fm.user = audio_user;
	SET_RESP_STATE(audio_user->resp_state, WAITING);

	/* Send packet to controller */
	err = ste_conn_write(cg29xx_audio_info->ste_conn_dev_fm, skb);
	if (err) {
		STE_CONN_ERR("Error occurred while waiting for return packet.");
		goto error_handling_free_skb;
	}

	err = receive_fm_legacy_response(audio_user,
					 CG2900_FM_RSP_ID_AIP_BT_SET_CONTROL_LSB,
					 CG2900_FM_RSP_ID_AIP_BT_SET_CONTROL_MSB);
	if (err) {
		goto finished_unlock_mutex;
	}

	SET_RESP_STATE(audio_user->resp_state, IDLE);


	/*
	 * Now set input of the BT sample rate converter by sending HCI_Write
	 * command with AIP_BT_SetMode.
	 */
	STE_CONN_DBG("FM: AIP_BT_SetMode");

	skb = ste_conn_alloc_skb(CG2900_FM_CMD_LEN_AIP_BT_SET_MODE, GFP_KERNEL);
	if (!skb) {
		STE_CONN_ERR("Could not allocate skb");
		err = -ENOMEM;
		goto finished_unlock_mutex;
	}

	/* Enter data into the skb */
	data = skb_put(skb, CG2900_FM_CMD_LEN_AIP_BT_SET_MODE);

	data[0] = CG2900_FM_CMD_LEN_AIP_BT_SET_MODE - 1; /* Length */
	data[1] = CG2900_FM_GEN_OPCODE_LEGACY_API;
	data[2] = CG2900_FM_CMD_LEG_PARAMETERS_WRITE;
	data[3] = CG2900_FM_CMD_PARAMETERS_WRITECOMMAND;
	data[4] = CG2900_FM_CMD_ID_AIP_BT_SET_MODE_LSB;
	data[5] = CG2900_FM_CMD_ID_AIP_BT_SET_MODE_MSB;
	data[6] = HCI_SET_U16_DATA_LSB(CG2900_FM_CMD_AIP_BT_SET_MODE_INPUT_I2S);
	data[7] = HCI_SET_U16_DATA_MSB(CG2900_FM_CMD_AIP_BT_SET_MODE_INPUT_I2S);

	cb_info_fm.user = audio_user;
	SET_RESP_STATE(audio_user->resp_state, WAITING);

	/* Send packet to controller */
	err = ste_conn_write(cg29xx_audio_info->ste_conn_dev_fm, skb);
	if (err) {
		STE_CONN_ERR("Error occurred while waiting for return packet.");
		goto error_handling_free_skb;
	}

	err = receive_fm_legacy_response(audio_user,
					 CG2900_FM_RSP_ID_AIP_BT_SET_MODE_LSB,
					 CG2900_FM_RSP_ID_AIP_BT_SET_MODE_MSB);
	if (err) {
		goto finished_unlock_mutex;
	}

	/* So far we only support one FM TX stream. Set that handle. */
	*stream_handle = FM_TX_STREAM_HANDLE;
	STE_CONN_DBG("stream_handle set to %d", *stream_handle);

	goto finished_unlock_mutex;

error_handling_free_skb:
	kfree_skb(skb);
finished_unlock_mutex:
	SET_RESP_STATE(audio_user->resp_state, IDLE);
	mutex_unlock(&cg29xx_audio_info->fm_channel_mutex);
finished:
	return err;
}

/**
 * conn_start_pcm_to_sco() - Start an audio stream connecting Bluetooth (e)SCO to PCM_I2S.
 * @audio_user:  Audio user to check for.
 * @stream_handle: [out] Pointer where to store the stream handle.
 *
 * This function sets up a BT to_from PCM_I2S stream.
 * It does this by first setting the Session configuration and then starting
 * the Audio Stream.
 *
 * Returns:
 *   0 if there is no error.
 *   -ECOMM if no response was received.
 *   -ENOMEM upon allocation errors.
 *   Errors from @ste_conn_write and @receive_bt_cmd_complete.
 *   -EIO for other errors.
 */
static int conn_start_pcm_to_sco(struct cg29xx_audio_user *audio_user, unsigned int *stream_handle)
{
	int err = 0;
	uint8_t temp_session_id;
	union ste_cg29xx_audio_endpoint_configuration_union *bt_config;
	struct sk_buff *skb;
	uint8_t *data;

	bt_config = find_endpoint(STE_CG29XX_AUDIO_ENDPOINT_ID_BT_SCO_INOUT, &(cg29xx_audio_info->endpoints));
	if (!bt_config) {
		STE_CONN_ERR("BT not configured before stream start");
		err = -EIO;
		goto finished;
	}

	if (!(cg29xx_audio_info->i2s_pcm_config_known)) {
		STE_CONN_ERR("I2S_PCM DAI not configured before stream start");
		err = -EIO;
		goto finished;
	}

	/* Use mutex to assure that only ONE command is sent at any time on each channel */
	mutex_lock(&cg29xx_audio_info->bt_channel_mutex);

	/*
	 * First send HCI_VS_Set_Session_Configuration command
	 */
	STE_CONN_DBG("BT: HCI_VS_Set_Session_Configuration");

	skb = ste_conn_alloc_skb(CG2900_BT_LEN_VS_SET_SESSION_CONFIGURATION, GFP_KERNEL);
	if (!skb) {
		STE_CONN_ERR("Could not allocate skb");
		err = -ENOMEM;
		goto finished_unlock_mutex;
	}

	/* Enter data into the skb */
	data = skb_put(skb, CG2900_BT_LEN_VS_SET_SESSION_CONFIGURATION);
	/* Default all bytes to 0 so we don't have to set reserved bytes below. */
	memset(data, 0, CG2900_BT_LEN_VS_SET_SESSION_CONFIGURATION);

	data[0] = CG2900_BT_VS_SET_SESSION_CONFIGURATION_LSB;
	data[1] = CG2900_BT_VS_SET_SESSION_CONFIGURATION_MSB;
	data[2] = CG2900_BT_LEN_VS_SET_SESSION_CONFIGURATION - 3; /* Parameter length */
	data[3] = 1; /* Number of streams */
	data[4] = 0; /* Media type: Audio */
	data[5] = bt_config->sco.sample_rate << 4; /* Media configuration: Stored sample rate, Mono */
	/* data[6] - data[10] SBC codec params (not used for SCO) */
	/* Input Virtual Port (VP) configuration */
	data[11] = 0x04; /* VP Type: BT SCO */
	data[12] = 0x08; /* SCO handle: DUMMY LSB */
	data[13] = 0x00; /* SCO handle: DUMMY MSB */
	/* data[14] - data[23] reserved */
	/* Output Virtual Port (VP) configuration */
	data[24] = 0x00; /* VP Type: PCM */
	data[25] = 0x00; /* PCM index: PCM bus 0 */
	if (cg29xx_audio_info->i2s_pcm_config.sco_slots_used) {
		data[26] = 0x01; /* PCM slots used */
	} else {
		data[26] = 0x00; /* PCM slots used */
	}
	data[27] = cg29xx_audio_info->i2s_pcm_config.slot_0_start; /* Slot 0 start */
	data[28] = cg29xx_audio_info->i2s_pcm_config.slot_1_start; /* Slot 1 start */
	data[29] = cg29xx_audio_info->i2s_pcm_config.slot_2_start; /* Slot 2 start */
	data[30] = cg29xx_audio_info->i2s_pcm_config.slot_3_start; /* Slot 3 start */
	/* data[31] - data[36] reserved */

	cb_info_bt.user = audio_user;
	SET_RESP_STATE(audio_user->resp_state, WAITING);

	/* Send packet to controller */
	err = ste_conn_write(cg29xx_audio_info->ste_conn_dev_bt, skb);
	if (err) {
		STE_CONN_ERR("Error occurred while waiting for return packet.");
		goto error_handling_free_skb;
	}

	err = receive_bt_cmd_complete(audio_user,
				      CG2900_BT_VS_SET_SESSION_CONFIGURATION_LSB,
				      CG2900_BT_VS_SET_SESSION_CONFIGURATION_MSB,
				      &temp_session_id, 1);
	if (err) {
		goto finished_unlock_mutex;
	}
	*stream_handle = temp_session_id;

	STE_CONN_DBG("stream_handle set to %d", *stream_handle);

	SET_RESP_STATE(audio_user->resp_state, IDLE);

	/*
	 * Now start the stream by sending HCI_VS_Session_Control comman
	 */
	STE_CONN_DBG("BT: HCI_VS_Session_Control");

	skb = ste_conn_alloc_skb(CG2900_BT_LEN_VS_SESSION_CONTROL, GFP_KERNEL);
	if (!skb) {
		STE_CONN_ERR("Could not allocate skb");
		err = -ENOMEM;
		goto finished_unlock_mutex;
	}

	/* Enter data into the skb */
	data = skb_put(skb, CG2900_BT_LEN_VS_SESSION_CONTROL);

	data[0] = CG2900_BT_VS_SESSION_CONTROL_LSB;
	data[1] = CG2900_BT_VS_SESSION_CONTROL_MSB;
	data[2] = CG2900_BT_LEN_VS_SESSION_CONTROL - 3; /* Parameter length */
	data[3] = (uint8_t)(*stream_handle); /* Session ID */
	data[4] = CG2900_BT_SESSION_START;

	cb_info_bt.user = audio_user;
	SET_RESP_STATE(audio_user->resp_state, WAITING);

	/* Send packet to controller */
	err = ste_conn_write(cg29xx_audio_info->ste_conn_dev_bt, skb);
	if (err) {
		STE_CONN_ERR("Error occurred while waiting for return packet.");
		goto error_handling_free_skb;
	}

	err = receive_bt_cmd_complete(audio_user,
				      CG2900_BT_VS_SESSION_CONTROL_LSB,
				      CG2900_BT_VS_SESSION_CONTROL_MSB,
				      NULL, 0);

	goto finished_unlock_mutex;

error_handling_free_skb:
	kfree_skb(skb);
finished_unlock_mutex:
	SET_RESP_STATE(audio_user->resp_state, IDLE);
	mutex_unlock(&cg29xx_audio_info->bt_channel_mutex);
finished:
	return err;
}


/**
 * conn_stop_pcm_to_sco() - Stops an audio stream connecting Bluetooth (e)SCO to PCM_I2S.
 * @audio_user:  Audio user to check for.
 * @stream_handle: Handle of the audio stream.
 *
 * This function stops a BT to_from PCM_I2S stream.
 * It does this by first stopping the Audio Stream and then resetting the
 * Session configuration.
 *
 * Returns:
 *   0 if there is no error.
 *   -ECOMM if no response was received.
 *   -ENOMEM upon allocation errors.
 *   Errors from @ste_conn_write and @receive_bt_cmd_complete.
 *   -EIO for other errors.
 */
static int conn_stop_pcm_to_sco(struct cg29xx_audio_user *audio_user, unsigned int stream_handle)
{
	int err = 0;
	union ste_cg29xx_audio_endpoint_configuration_union *bt_config;
	struct sk_buff *skb;
	uint8_t *data;

	bt_config = find_endpoint(STE_CG29XX_AUDIO_ENDPOINT_ID_BT_SCO_INOUT, &(cg29xx_audio_info->endpoints));
	if (!bt_config) {
		STE_CONN_ERR("BT not configured before stream start");
		err = -EIO;
		goto finished;
	}

	/* Use mutex to assure that only ONE command is sent at any time on each channel */
	mutex_lock(&cg29xx_audio_info->bt_channel_mutex);

	/*
	 * Now stop the stream by sending HCI_VS_Session_Control command
	 */
	STE_CONN_DBG("BT: HCI_VS_Session_Control");

	skb = ste_conn_alloc_skb(CG2900_BT_LEN_VS_SESSION_CONTROL, GFP_KERNEL);
	if (!skb) {
		STE_CONN_ERR("Could not allocate skb");
		err = -ENOMEM;
		goto finished_unlock_mutex;
	}

	/* Enter data into the skb */
	data = skb_put(skb, CG2900_BT_LEN_VS_SESSION_CONTROL);

	data[0] = CG2900_BT_VS_SESSION_CONTROL_LSB;
	data[1] = CG2900_BT_VS_SESSION_CONTROL_MSB;
	data[2] = CG2900_BT_LEN_VS_SESSION_CONTROL - 3; /* Parameter length */
	data[3] = (uint8_t)stream_handle; /* Session ID */
	data[4] = CG2900_BT_SESSION_STOP;

	cb_info_bt.user = audio_user;
	SET_RESP_STATE(audio_user->resp_state, WAITING);

	/* Send packet to controller */
	err = ste_conn_write(cg29xx_audio_info->ste_conn_dev_bt, skb);
	if (err) {
		STE_CONN_ERR("Error occurred while waiting for return packet.");
		goto error_handling_free_skb;
	}

	err = receive_bt_cmd_complete(audio_user,
				      CG2900_BT_VS_SESSION_CONTROL_LSB,
				      CG2900_BT_VS_SESSION_CONTROL_MSB,
				      NULL, 0);
	if (err) {
		goto finished_unlock_mutex;
	}

	SET_RESP_STATE(audio_user->resp_state, IDLE);

	/*
	 * Now delete the stream by sending HCI_VS_Reset_Session_Configuration
	 * command
	 */
	STE_CONN_DBG("BT: HCI_VS_Reset_Session_Configuration");

	skb = ste_conn_alloc_skb(CG2900_BT_LEN_VS_RESET_SESSION_CONFIGURATION, GFP_KERNEL);
	if (!skb) {
		STE_CONN_ERR("Could not allocate skb");
		err = -ENOMEM;
		goto finished_unlock_mutex;
	}

	/* Enter data into the skb */
	data = skb_put(skb, CG2900_BT_LEN_VS_RESET_SESSION_CONFIGURATION);

	data[0] = CG2900_BT_VS_RESET_SESSION_CONFIGURATION_LSB;
	data[1] = CG2900_BT_VS_RESET_SESSION_CONFIGURATION_MSB;
	data[2] = CG2900_BT_LEN_VS_RESET_SESSION_CONFIGURATION - 3; /* Parameter length */
	data[3] = (uint8_t)stream_handle; /* Session ID */

	cb_info_bt.user = audio_user;
	SET_RESP_STATE(audio_user->resp_state, WAITING);

	/* Send packet to controller */
	err = ste_conn_write(cg29xx_audio_info->ste_conn_dev_bt, skb);
	if (err) {
		STE_CONN_ERR("Error occurred while waiting for return packet.");
		goto error_handling_free_skb;
	}

	err = receive_bt_cmd_complete(audio_user,
				      CG2900_BT_VS_RESET_SESSION_CONFIGURATION_LSB,
				      CG2900_BT_VS_RESET_SESSION_CONFIGURATION_MSB,
				      NULL, 0);

	goto finished_unlock_mutex;

error_handling_free_skb:
	kfree_skb(skb);
finished_unlock_mutex:
	SET_RESP_STATE(audio_user->resp_state, IDLE);
	mutex_unlock(&cg29xx_audio_info->bt_channel_mutex);
finished:
	return err;
}

/*
 * 	External methods
 */
int ste_cg29xx_audio_open(unsigned int *session)
{
	int err = 0;
	int i;

	STE_CONN_INFO("ste_cg29xx_audio_open");

	mutex_lock(&cg29xx_audio_info->management_mutex);

	if (!session) {
		STE_CONN_ERR("NULL supplied as session.");
		err = -EINVAL;
		goto finished;
	}

	*session = 0;

	/* First find a free session to use and allocate the session structure */
	for (i = CG29XX_AUDIO_FIRST_USER; i < CG29XX_AUDIO_MAX_NBR_OF_USERS && !(*session); i++) {
		if (!cg29xx_audio_info->audio_sessions[i]) {
			cg29xx_audio_info->audio_sessions[i] = kzalloc(sizeof(*(cg29xx_audio_info->audio_sessions[0])), GFP_KERNEL);
			if (!cg29xx_audio_info->audio_sessions[i]) {
				STE_CONN_ERR("Could not allocate user");
				err = -ENOMEM;
				goto finished;
			}
			STE_CONN_DBG("Found free session %d", i);
			*session = i;
			cg29xx_audio_info->nbr_of_users_active++;
		}
	}

	if (!(*session)) {
		STE_CONN_ERR("Couldn't find free user");
		err = -EMFILE;
		goto finished;
	}

	SET_RESP_STATE(cg29xx_audio_info->audio_sessions[*session]->resp_state, IDLE)
	cg29xx_audio_info->audio_sessions[*session]->session = *session;

	if (cg29xx_audio_info->nbr_of_users_active == 1) {
		/* First user so register to STE_CONN. First the BT audio device. */
		cg29xx_audio_info->ste_conn_dev_bt = ste_conn_register(STE_CONN_DEVICES_BT_AUDIO, &ste_cg29xx_audio_cb);
		if (!cg29xx_audio_info->ste_conn_dev_bt) {
			STE_CONN_ERR("Failed to register BT audio channel");
			err = -EIO;
			goto error_handling;
		} else {
			/* Store the callback info structure */
			cg29xx_audio_info->ste_conn_dev_bt->user_data = &cb_info_bt;
		}
		/* Then the FM audio device */
		cg29xx_audio_info->ste_conn_dev_fm = ste_conn_register(STE_CONN_DEVICES_FM_RADIO_AUDIO, &ste_cg29xx_audio_cb);
		if (!cg29xx_audio_info->ste_conn_dev_fm) {
			STE_CONN_ERR("Failed to register FM audio channel");
			err = -EIO;
			goto error_handling;
		} else {
			/* Store the callback info structure */
			cg29xx_audio_info->ste_conn_dev_fm->user_data = &cb_info_fm;
		}
		cg29xx_audio_info->state = OPENED;
	}

	goto finished;

error_handling:
	if (cg29xx_audio_info->ste_conn_dev_bt) {
		ste_conn_deregister(cg29xx_audio_info->ste_conn_dev_bt);
		cg29xx_audio_info->ste_conn_dev_bt = NULL;
	}
	cg29xx_audio_info->nbr_of_users_active--;
	kfree(cg29xx_audio_info->audio_sessions[*session]);
	cg29xx_audio_info->audio_sessions[*session] = NULL;
finished:
	mutex_unlock(&cg29xx_audio_info->management_mutex);
	return err;
}
EXPORT_SYMBOL(ste_cg29xx_audio_open);

int ste_cg29xx_audio_close(unsigned int *session)
{
	int err = 0;
	struct cg29xx_audio_user *audio_user;

	STE_CONN_INFO("ste_cg29xx_audio_close");

	if (cg29xx_audio_info->state != OPENED) {
		STE_CONN_ERR("Audio driver not open");
		err = -EIO;
		goto finished;
	}

	if (!session) {
		STE_CONN_ERR("NULL pointer supplied");
		err = -EINVAL;
		goto finished;
	}

	audio_user = get_session_user(*session);
	if (!audio_user) {
		STE_CONN_ERR("Invalid session ID");
		err = -EINVAL;
		goto finished;
	}

	mutex_lock(&cg29xx_audio_info->management_mutex);

	if (!(cg29xx_audio_info->audio_sessions[*session])) {
		STE_CONN_ERR("Session %d not opened", *session);
		err = -EACCES;
		goto err_unlock_mutex;
	}

	kfree(cg29xx_audio_info->audio_sessions[*session]);
	cg29xx_audio_info->audio_sessions[*session] = NULL;
	cg29xx_audio_info->nbr_of_users_active--;

	if (cg29xx_audio_info->nbr_of_users_active == 0) {
		/* No more sessions open. Deregister from STE_CONN */
		ste_conn_deregister(cg29xx_audio_info->ste_conn_dev_fm);
		ste_conn_deregister(cg29xx_audio_info->ste_conn_dev_bt);
		cg29xx_audio_info->state = CLOSED;
	}

	*session = 0;

err_unlock_mutex:
	mutex_unlock(&cg29xx_audio_info->management_mutex);
finished:
	return err;
}
EXPORT_SYMBOL(ste_cg29xx_audio_close);

int ste_cg29xx_audio_set_dai_configuration(unsigned int session,
					struct ste_cg29xx_dai_config *config)
{
	int err = 0;
	struct cg29xx_audio_user *audio_user;
	struct ste_cg29xx_dai_port_conf_i2s *i2s = NULL;
	struct ste_cg29xx_dai_port_conf_i2s_pcm *i2s_pcm = NULL;
	struct sk_buff *skb = NULL;
	uint8_t *data = NULL;
	struct ste_conn_revision_data rev_data;

	STE_CONN_INFO("ste_cg29xx_audio_set_dai_configuration");

	if (cg29xx_audio_info->state != OPENED) {
		STE_CONN_ERR("Audio driver not open");
		err = -EIO;
		goto finished;
	}

	audio_user = get_session_user(session);
	if (!audio_user) {
		err = -EINVAL;
		goto finished;
	}

	/* Check that the chip connected is a chip we support */
	if (!ste_conn_get_local_revision(&rev_data)) {
		STE_CONN_ERR("Couldn't retrieve revision data");
		err = -EIO;
		goto finished;
	}

	/* We only support the following chips:
	 * ST-Ericsson CG2900 PG1
	 */
	if (CG2900_PG1_REV != rev_data.revision ||
	    CG2900_PG1_SUB_VER != rev_data.sub_version) {
		STE_CONN_ERR("Chip rev 0x%04X sub 0x%04X not supported by Audio driver",
			     rev_data.revision, rev_data.sub_version);
		err = -EIO;
		goto finished;
	}

	/* Use mutex to assure that only ONE command is sent at any time on each channel */
	mutex_lock(&cg29xx_audio_info->bt_channel_mutex);

	/*
	 * Send following commands for the supported chips.
	 * For CG2900 PG1 = HCI_Cmd_VS_Set_Hardware_Configuration
	 */

	/* Allocate the sk_buffer. The length is actually a max length since
	 * length varies depending on logical transport.
	 */
	skb = ste_conn_alloc_skb(CG2900_BT_LEN_VS_SET_HARDWARE_CONFIGURATION, GFP_KERNEL);
	if (skb) {
		/* Format HCI command
		 *
		 * [vp][ltype][direction + mode][Bitclk][PCM duration]
		 * Start with the 2 byte op code.
		 */
		data = skb_put(skb, 2);
		data[0] = CG2900_BT_VS_SET_HARDWARE_CONFIGURATION_LSB;
		data[1] = CG2900_BT_VS_SET_HARDWARE_CONFIGURATION_MSB;

		/* Now create each command depending on received configuration */
		switch (config->port) {
		case STE_CG29XX_DAI_PORT_0_I2S:
			i2s = (struct ste_cg29xx_dai_port_conf_i2s *) &config->conf;

			/* We will now have 5 bytes of data (including length field) */
			data = skb_put(skb, 5);

			/* Parameters begin -----*/
			data[0] = 0x04; /* Parameter length */
			data[1] = STE_CG29XX_DAI_PORT_PROTOCOL_I2S;
			data[2] = 0x00; /* Virtual port if multiple on vp, PCM /I2S index */
			data[3] = i2s->half_period; /* WS Half period size */
			data[4] = i2s->mode; /* WS Sel */

			/* Store the new configuration */
			mutex_lock(&cg29xx_audio_info->management_mutex);
			memcpy(&(cg29xx_audio_info->i2s_config), &(config->conf.i2s), sizeof(config->conf.i2s));
			cg29xx_audio_info->i2s_config_known = true;
			mutex_unlock(&cg29xx_audio_info->management_mutex);
			break;

		case STE_CG29XX_DAI_PORT_1_I2S_PCM:
			i2s_pcm = (struct ste_cg29xx_dai_port_conf_i2s_pcm *) &config->conf;

			/* We will now have 7 bytes of data (including length field) */
			data = skb_put(skb, 7);

			/* Parameters begin -----*/
			data[0] = 0x06; /* Parameter total length */
			data[1] = STE_CG29XX_DAI_PORT_PROTOCOL_PCM;

			if (i2s_pcm->protocol == STE_CG29XX_DAI_PORT_PROTOCOL_PCM) { /* PCM (logical) */
				data[2] = 0x00; /* Virtual port if multiple on vp, PCM /I2S index */

				if (i2s_pcm->slot_0_dir == STE_CG29XX_DAI_DIRECTION_PORT_B_TX_PORT_A_RX)
					data[3] |= 0x10; /* Set bit 4 to 1 */
				if (i2s_pcm->slot_1_dir == STE_CG29XX_DAI_DIRECTION_PORT_B_TX_PORT_A_RX)
					data[3] |= 0x20; /* Set bit 5 to 1 */
				if (i2s_pcm->slot_2_dir == STE_CG29XX_DAI_DIRECTION_PORT_B_TX_PORT_A_RX)
					data[3] |= 0x40; /* Set bit 6 to 1 */
				if (i2s_pcm->slot_3_dir == STE_CG29XX_DAI_DIRECTION_PORT_B_TX_PORT_A_RX)
					data[3] |= 0x80; /* Set bit 7 to 1 */
				if (i2s_pcm->mode == STE_CG29XX_DAI_MODE_MASTER)
					data[3] |= 0x02; /* Set bit 1 to 1 for Master */
				data[4] = i2s_pcm->clk;
				data[5] = i2s_pcm->duration & 0x00FF;
				data[6] = (i2s_pcm->duration >> 8) & 0x00FF;

				/* Store the new configuration */
				mutex_lock(&cg29xx_audio_info->management_mutex);
				memcpy(&(cg29xx_audio_info->i2s_pcm_config), &(config->conf.i2s_pcm), sizeof(config->conf.i2s_pcm));
				cg29xx_audio_info->i2s_pcm_config_known = true;
				mutex_unlock(&cg29xx_audio_info->management_mutex);
			} else {
				/* Short solution for PG1 chip, don't support I2S over the PCM/I2S bus... */
				STE_CONN_ERR("I2S not supported over the PCM/I2S bus");
				err = -EACCES;
				goto error_handling_free_skb;
			}
			break;

		default:
			STE_CONN_ERR("Unknown port configuration %d", config->port);
			err = -EACCES;
			goto error_handling_free_skb;
			break;
		};
	} else {
		STE_CONN_ERR("Could not allocate skb");
		err = -ENOMEM;
		goto finished_unlock_mutex;
	}

	cb_info_bt.user = audio_user;
	SET_RESP_STATE(audio_user->resp_state, WAITING);

	/* Send packet to controller */
	err = ste_conn_write(cg29xx_audio_info->ste_conn_dev_bt, skb);
	if (err) {
		STE_CONN_ERR("Error occurred while waiting for return packet.");
		goto error_handling_free_skb;
	}

	err = receive_bt_cmd_complete(audio_user,
				      CG2900_BT_VS_SET_HARDWARE_CONFIGURATION_LSB,
				      CG2900_BT_VS_SET_HARDWARE_CONFIGURATION_MSB,
				      NULL, 0);

	goto finished_unlock_mutex;

error_handling_free_skb:
	kfree_skb(skb);
finished_unlock_mutex:
	SET_RESP_STATE(audio_user->resp_state, IDLE);
	mutex_unlock(&cg29xx_audio_info->bt_channel_mutex);
finished:
	return err;
}
EXPORT_SYMBOL(ste_cg29xx_audio_set_dai_configuration);

int ste_cg29xx_audio_get_dai_configuration(unsigned int session,
					struct ste_cg29xx_dai_config *config)
{
	int err = 0;
	struct cg29xx_audio_user *audio_user;

	STE_CONN_INFO("ste_cg29xx_audio_get_dai_configuration");

	if (cg29xx_audio_info->state != OPENED) {
		STE_CONN_ERR("Audio driver not open");
		err = -EIO;
		goto finished;
	}

	if (!config) {
		STE_CONN_ERR("NULL supplied as config structure");
		err = -EINVAL;
		goto finished;
	}

	audio_user = get_session_user(session);
	if (!audio_user) {
		err = -EINVAL;
		goto finished;
	}

	/* Return DAI configuration based on the received port.
	 * If port has not been configured return error.
	 */
	switch (config->port) {
	case STE_CG29XX_DAI_PORT_0_I2S:
		mutex_lock(&cg29xx_audio_info->management_mutex);
		if (cg29xx_audio_info->i2s_config_known) {
			memcpy(&(config->conf.i2s), &(cg29xx_audio_info->i2s_config), sizeof(config->conf.i2s));
		} else {
			err = -EIO;
		}
		mutex_unlock(&cg29xx_audio_info->management_mutex);
		break;

	case STE_CG29XX_DAI_PORT_1_I2S_PCM:
		mutex_lock(&cg29xx_audio_info->management_mutex);
		if (cg29xx_audio_info->i2s_pcm_config_known) {
			memcpy(&(config->conf.i2s_pcm), &(cg29xx_audio_info->i2s_pcm_config), sizeof(config->conf.i2s_pcm));
		} else {
			err = -EIO;
		}
		mutex_unlock(&cg29xx_audio_info->management_mutex);
		break;

	default:
		STE_CONN_ERR("Unknown port configuration %d", config->port);
		err = -EIO;
		break;
	};

finished:
	return err;
}
EXPORT_SYMBOL(ste_cg29xx_audio_get_dai_configuration);

int ste_cg29xx_audio_configure_endpoint(unsigned int session,
					struct ste_cg29xx_audio_endpoint_configuration *configuration)
{
	int err = 0;
	struct cg29xx_audio_user *audio_user;

	STE_CONN_INFO("ste_cg29xx_audio_configure_endpoint");

	if (cg29xx_audio_info->state != OPENED) {
		STE_CONN_ERR("Audio driver not open");
		err = -EIO;
		goto finished;
	}

	if (!configuration) {
		STE_CONN_ERR("NULL supplied as configuration structure");
		err = -EINVAL;
		goto finished;
	}

	audio_user = get_session_user(session);
	if (!audio_user) {
		err = -EINVAL;
		goto finished;
	}

	switch (configuration->endpoint_id) {
	case STE_CG29XX_AUDIO_ENDPOINT_ID_BT_SCO_INOUT:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_BT_A2DP_SRC:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_FM_RX:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_FM_TX:
		add_endpoint(configuration, &(cg29xx_audio_info->endpoints));
		break;
	case STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_0_I2S:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_1_I2S_PCM:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_SLIMBUS_VOICE:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_SLIMBUS_AUDIO:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_BT_A2DP_SNK:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_ANALOG_OUT:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_AUDIO_IN:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_AUDIO_OUT:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_VOICE_IN:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_VOICE_OUT:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_TONE_IN:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_BURST_BUFFER_IN:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_BURST_BUFFER_OUT:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_MUSIC_DECODER:
	case STE_CG29XX_AUDIO_ENDPOINT_ID_HCI_AUDIO_IN:
	default:
		STE_CONN_ERR("Unknown endpoint_id %d", configuration->endpoint_id);
		err = -EACCES;
		break;
	}


finished:
	return err;
}
EXPORT_SYMBOL(ste_cg29xx_audio_configure_endpoint);

int ste_cg29xx_audio_connect_and_start_stream(unsigned int session,
					enum ste_cg29xx_audio_endpoint_id endpoint_1,
					enum ste_cg29xx_audio_endpoint_id endpoint_2,
					unsigned int *stream_handle)
{
	int err = 0;
	struct cg29xx_audio_user *audio_user;

	STE_CONN_INFO("ste_cg29xx_audio_connect_and_start_stream");

	if (cg29xx_audio_info->state != OPENED) {
		STE_CONN_ERR("Audio driver not open");
		err = -EIO;
		goto finished;
	}

	audio_user = get_session_user(session);
	if (!audio_user) {
		err = -EINVAL;
		goto finished;
	}

	/* First handle the endpoints */
	switch (endpoint_1) {
	case STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_0_I2S:
		switch (endpoint_2) {
		case STE_CG29XX_AUDIO_ENDPOINT_ID_FM_RX:
			err = conn_start_i2s_to_fm_rx(audio_user, stream_handle);
			break;

		case STE_CG29XX_AUDIO_ENDPOINT_ID_FM_TX:
			err = conn_start_i2s_to_fm_tx(audio_user, stream_handle);
			break;

		default:
			STE_CONN_ERR("Endpoint config not handled: ep1: %d, ep2: %d",
				     endpoint_1, endpoint_2);
			err = -EINVAL;
			goto finished;
			break;
		}
		break;

	case STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_1_I2S_PCM:
		switch (endpoint_2) {
		case STE_CG29XX_AUDIO_ENDPOINT_ID_BT_SCO_INOUT:
			err = conn_start_pcm_to_sco(audio_user, stream_handle);
			break;

		default:
			STE_CONN_ERR("Endpoint config not handled: ep1: %d, ep2: %d",
				     endpoint_1, endpoint_2);
			err = -EINVAL;
			goto finished;
			break;
		}
		break;

	case STE_CG29XX_AUDIO_ENDPOINT_ID_BT_SCO_INOUT:
		switch (endpoint_2) {
		case STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_1_I2S_PCM:
			err = conn_start_pcm_to_sco(audio_user, stream_handle);
			break;

		default:
			STE_CONN_ERR("Endpoint config not handled: ep1: %d, ep2: %d",
				     endpoint_1, endpoint_2);
			err = -EINVAL;
			goto finished;
			break;
		}
		break;

	case STE_CG29XX_AUDIO_ENDPOINT_ID_FM_RX:
		switch (endpoint_2) {
		case STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_0_I2S:
			err = conn_start_i2s_to_fm_rx(audio_user, stream_handle);
			break;

		default:
			STE_CONN_ERR("Endpoint config not handled: ep1: %d, ep2: %d",
				     endpoint_1, endpoint_2);
			err = -EINVAL;
			goto finished;
			break;
		}
		break;

	case STE_CG29XX_AUDIO_ENDPOINT_ID_FM_TX:
		switch (endpoint_2) {
		case STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_0_I2S:
			err = conn_start_i2s_to_fm_tx(audio_user, stream_handle);
			break;

		default:
			STE_CONN_ERR("Endpoint config not handled: ep1: %d, ep2: %d",
				     endpoint_1, endpoint_2);
			err = -EINVAL;
			goto finished;
			break;
		}
		break;

	default:
		STE_CONN_ERR("Endpoint config not handled: ep1: %d, ep2: %d",
			     endpoint_1, endpoint_2);
		err = -EINVAL;
		goto finished;
		break;
	}

finished:
	return err;
}
EXPORT_SYMBOL(ste_cg29xx_audio_connect_and_start_stream);

int ste_cg29xx_audio_stop_stream(unsigned int session,
				unsigned int stream_handle)
{
	int err = 0;
	struct cg29xx_audio_user *audio_user;

	STE_CONN_INFO("ste_cg29xx_audio_stop_stream");

	if (cg29xx_audio_info->state != OPENED) {
		STE_CONN_ERR("Audio driver not open");
		err = -EIO;
		goto finished;
	}

	audio_user = get_session_user(session);
	if (!audio_user) {
		err = -EINVAL;
		goto finished;
	}

	if (stream_handle == FM_RX_STREAM_HANDLE) {
		/* Nothing to do here at the moment */
	} else if (stream_handle == FM_TX_STREAM_HANDLE) {
		/* Nothing to do here at the moment */
	} else {
		/* BT SCO stream */
		err = conn_stop_pcm_to_sco(audio_user, stream_handle);
	}

finished:
	return err;
}
EXPORT_SYMBOL(ste_cg29xx_audio_stop_stream);

/*
 *	Character devices for userspace module test
 */

/**
 * ste_cg29xx_audio_char_device_open() - Open char device.
 * @inode: Device driver information.
 * @filp:  Pointer to the file struct.
 *
 * Returns:
 *   0 if there is no error.
 *   -ENOMEM if allocation failed.
 *   Errors from @ste_cg29xx_audio_open.
 */
static int ste_cg29xx_audio_char_device_open(struct inode *inode,
						struct file *filp)
{
	int err = 0;
	struct cg29xx_audio_char_dev_info *char_dev_info;

	STE_CONN_INFO("ste_cg29xx_audio_char_device_open");

	/* Allocate the char dev info structure. It will be stored inside
	 * the file pointer and supplied when file_ops are called.
	 * It's free'd in ste_cg29xx_audio_char_device_release.
	 */
	char_dev_info = kzalloc(sizeof(*char_dev_info), GFP_KERNEL);
	if (!char_dev_info) {
		STE_CONN_ERR("Couldn't allocate char_dev_info");
		err = -ENOMEM;
		goto finished;
	}
	filp->private_data = char_dev_info;

	mutex_init(&char_dev_info->management_mutex);
	mutex_init(&char_dev_info->rw_mutex);

	mutex_lock(&char_dev_info->management_mutex);
	err = ste_cg29xx_audio_open(&char_dev_info->session);
	mutex_unlock(&char_dev_info->management_mutex);
	if (err) {
		STE_CONN_ERR("Failed to open CG29XX Audio driver (%d)", err);
		goto error_handling_free_mem;
	}

	goto finished;

error_handling_free_mem:
	kfree(char_dev_info);
	filp->private_data = NULL;
finished:
	return err;
}

/**
 * ste_cg29xx_audio_char_device_release() - Release char device.
 * @inode: Device driver information.
 * @filp:  Pointer to the file struct.
 *
 * Returns:
 *   0 if there is no error.
 *   -EBADF if NULL pointer was supplied in private data.
 *   Errors from @ste_cg29xx_audio_close.
 */
static int ste_cg29xx_audio_char_device_release(struct inode *inode,
						struct file *filp)
{
	int err = 0;
	struct cg29xx_audio_char_dev_info *char_dev_info =
			(struct cg29xx_audio_char_dev_info *)filp->private_data;

	STE_CONN_INFO("ste_conn_char_device_release");

	if (!char_dev_info) {
		STE_CONN_ERR("No dev supplied in private data");
		err = -EBADF;
		goto finished;
	}

	mutex_lock(&char_dev_info->management_mutex);
	err = ste_cg29xx_audio_close(&char_dev_info->session);
	if (err) {
		/* Just print the error. Still free the char_dev_info since we
		 * don't know the filp structure is valid after this call
		 */
		STE_CONN_ERR("Error when closing CG29XX audio driver (%d)", err);
	}
	mutex_unlock(&char_dev_info->management_mutex);

	kfree(char_dev_info);
	filp->private_data = NULL;

finished:
	return err;
}

/**
 * ste_cg29xx_audio_char_device_read() - Return information to the user from last @write call.
 * @filp:  Pointer to the file struct.
 * @buf:   Received buffer.
 * @count: Size of buffer.
 * @f_pos: Position in buffer.
 *
 * The ste_cg29xx_audio_char_device_read() function returns information from
 * the last @write call to same char device.
 * The data is in the following format:
 *   - OpCode of command for this data
 *   - Data content (Length of data is determined by the command OpCode, i.e. fixed for each command)
 *
 * Returns:
 *   Bytes successfully read (could be 0).
 *   -EBADF if NULL pointer was supplied in private data.
 *   -EFAULT if copy_to_user fails.
 *   -ENOMEM upon allocation failure.
 */
static ssize_t ste_cg29xx_audio_char_device_read(struct file *filp,
						char __user *buf, size_t count,
						loff_t *f_pos)
{
	struct cg29xx_audio_char_dev_info *dev =
		(struct cg29xx_audio_char_dev_info *)filp->private_data;
	unsigned int bytes_to_copy = 0;
	int err = 0;

	mutex_lock(&dev->rw_mutex);
	STE_CONN_INFO("ste_cg29xx_audio_char_device_read");

	if (!dev) {
		STE_CONN_ERR("No dev supplied in private data");
		err = -EBADF;
		goto error_handling;
	}

	if (dev->stored_data_len == 0) {
		/* No data to read */
		bytes_to_copy = 0;
		goto finished;
	}

	bytes_to_copy = min(count, (unsigned int)(dev->stored_data_len));
	if (bytes_to_copy < dev->stored_data_len) {
		STE_CONN_ERR("Not enough buffer to store all data. Throwing away rest of data.");
	}

	err = copy_to_user(buf, dev->stored_data, bytes_to_copy);
	/* Throw away all data, even though not all was copied.
	 * This char device is primarily for testing purposes so we can keep
	 * such a limitation.
	 */
	kfree(dev->stored_data);
	dev->stored_data = NULL;
	dev->stored_data_len = 0;

	if (err) {
		STE_CONN_ERR("copy_to_user error %d", err);
		err = -EFAULT;
		goto error_handling;
	}

	goto finished;

error_handling:
	mutex_unlock(&dev->rw_mutex);
	return (ssize_t)err;
finished:
	mutex_unlock(&dev->rw_mutex);
	return bytes_to_copy;
}

/**
 * ste_cg29xx_audio_char_device_write() - Call CG29XX Audio API function determined by supplied data content.
 * @filp:  Pointer to the file struct.
 * @buf:   Write buffer.
 * @count: Size of the buffer write.
 * @f_pos: Position of buffer.
 *
 * ste_cg29xx_audio_char_device_write() function executes supplied data and
 * interprets it as if it was a function call to the CG29XX Audio API.
 * The data is according to:
 *   * OpCode (4 bytes)
 *   * Data according to OpCode (see API). No padding between parameters
 *
 * OpCodes are:
 *   * CHAR_DEV_OP_CODE_SET_DAI_CONF 0x00000001
 *   * CHAR_DEV_OP_CODE_GET_DAI_CONF 0x00000002
 *   * CHAR_DEV_OP_CODE_CONFIGURE_ENDPOINT 0x00000003
 *   * CHAR_DEV_OP_CODE_CONNECT_AND_START_STREAM 0x00000004
 *   * CHAR_DEV_OP_CODE_STOP_STREAM 0x00000005
 *
 * Returns:
 *   Bytes successfully written (could be 0). Equals input @count if successful.
 *   -EBADF if NULL pointer was supplied in private data.
 *   -EFAULT if copy_from_user fails.
 *   Error codes from all CG29XX Audio API functions.
 */
static ssize_t ste_cg29xx_audio_char_device_write(struct file *filp,
						const char __user *buf, size_t count,
						loff_t *f_pos)
{
	uint8_t *rec_data;
	struct cg29xx_audio_char_dev_info *dev =
		(struct cg29xx_audio_char_dev_info *)filp->private_data;
	int err = 0;
	int op_code = 0;

	STE_CONN_INFO("ste_cg29xx_audio_char_device_write count %d", count);

	if (!dev) {
		STE_CONN_ERR("No dev supplied in private data");
		err = -EBADF;
		goto finished;
	}

	mutex_lock(&dev->rw_mutex);

	rec_data = kmalloc(count, GFP_KERNEL);
	if (rec_data) {
		uint8_t *data = rec_data;
		/* Variables used when calling the different functions */
		unsigned int stream_handle;
		struct ste_cg29xx_dai_config dai_config;
		struct ste_cg29xx_audio_endpoint_configuration ep_config;
		enum ste_cg29xx_audio_endpoint_id endpoint_1;
		enum ste_cg29xx_audio_endpoint_id endpoint_2;
		int bytes_left = count;

		err = copy_from_user(rec_data, buf, count);
		if (err) {
			STE_CONN_ERR("copy_from_user failed (%d)", err);
			kfree(rec_data);
			err = -EFAULT;
			goto finished_mutex_unlock;
		}

		op_code = data[0];
		STE_CONN_DBG("op_code %d", op_code);
		/* OpCode is int size to keep data int aligned */
		data += sizeof(unsigned int);
		bytes_left -= sizeof(unsigned int);

		switch (op_code) {
		case CHAR_DEV_OP_CODE_SET_DAI_CONF:
			STE_CONN_DBG("CHAR_DEV_OP_CODE_SET_DAI_CONF %d", sizeof(dai_config));
			if (bytes_left < sizeof(dai_config)) {
				STE_CONN_ERR("Not enough data supplied for CHAR_DEV_OP_CODE_SET_DAI_CONF");
				err = -EINVAL;
				goto finished_mutex_unlock;
			}
			memcpy(&dai_config, data, sizeof(dai_config));
			STE_CONN_DBG("dai_config.port %d", dai_config.port);
			err = ste_cg29xx_audio_set_dai_configuration(dev->session, &dai_config);
			break;

		case CHAR_DEV_OP_CODE_GET_DAI_CONF:
			STE_CONN_DBG("CHAR_DEV_OP_CODE_GET_DAI_CONF %d", sizeof(dai_config));
			if (bytes_left < sizeof(dai_config)) {
				STE_CONN_ERR("Not enough data supplied for CHAR_DEV_OP_CODE_GET_DAI_CONF");
				err = -EINVAL;
				goto finished_mutex_unlock;
			}
			/* Only need to copy the port really, but let's copy
			 * like this for simplicity. It's only test functionality
			 * after all. */
			memcpy(&dai_config, data, sizeof(dai_config));
			STE_CONN_DBG("dai_config.port %d", dai_config.port);
			err = ste_cg29xx_audio_get_dai_configuration(dev->session, &dai_config);
			if (!err) {
				/* Command succeeded. Store data so it can be returned when calling read */
				if (dev->stored_data) {
					STE_CONN_ERR("Data already allocated (%d bytes). Throwing it away.",
						     dev->stored_data_len);
					kfree(dev->stored_data);
				}
				dev->stored_data_len = sizeof(op_code) + sizeof(dai_config);
				dev->stored_data = kmalloc(dev->stored_data_len, GFP_KERNEL);
				memcpy(dev->stored_data, &op_code, sizeof(op_code));
				memcpy(&(dev->stored_data[sizeof(op_code)]), &dai_config, sizeof(dai_config));
			}
			break;

		case CHAR_DEV_OP_CODE_CONFIGURE_ENDPOINT:
			STE_CONN_DBG("CHAR_DEV_OP_CODE_CONFIGURE_ENDPOINT %d", sizeof(ep_config));
			if (bytes_left < sizeof(ep_config)) {
				STE_CONN_ERR("Not enough data supplied for CHAR_DEV_OP_CODE_CONFIGURE_ENDPOINT");
				err = -EINVAL;
				goto finished_mutex_unlock;
			}
			memcpy(&ep_config, data, sizeof(ep_config));
			STE_CONN_DBG("ep_config.endpoint_id %d", ep_config.endpoint_id);
			err = ste_cg29xx_audio_configure_endpoint(dev->session, &ep_config);
			break;

		case CHAR_DEV_OP_CODE_CONNECT_AND_START_STREAM:
			STE_CONN_DBG("CHAR_DEV_OP_CODE_CONNECT_AND_START_STREAM %d", (sizeof(endpoint_1) + sizeof(endpoint_2)));
			if (bytes_left < (sizeof(endpoint_1) + sizeof(endpoint_2))) {
				STE_CONN_ERR("Not enough data supplied for CHAR_DEV_OP_CODE_CONNECT_AND_START_STREAM");
				err = -EINVAL;
				goto finished_mutex_unlock;
			}
			memcpy(&endpoint_1, data, sizeof(endpoint_1));
			data += sizeof(endpoint_1);
			memcpy(&endpoint_2, data, sizeof(endpoint_2));
			STE_CONN_DBG("endpoint_1 %d endpoint_2 %d", endpoint_1, endpoint_2);

			err = ste_cg29xx_audio_connect_and_start_stream(dev->session,
				endpoint_1, endpoint_2, &stream_handle);
			if (!err) {
				/* Command succeeded. Store data so it can be returned when calling read */
				if (dev->stored_data) {
					STE_CONN_ERR("Data already allocated (%d bytes). Throwing it away.",
						     dev->stored_data_len);
					kfree(dev->stored_data);
				}
				dev->stored_data_len = sizeof(op_code) + sizeof(stream_handle);
				dev->stored_data = kmalloc(dev->stored_data_len, GFP_KERNEL);
				memcpy(dev->stored_data, &op_code, sizeof(op_code));
				memcpy(&(dev->stored_data[sizeof(op_code)]), &stream_handle, sizeof(stream_handle));
				STE_CONN_DBG("stream_handle %d", stream_handle);
			}
			break;

		case CHAR_DEV_OP_CODE_STOP_STREAM:
			if (bytes_left < sizeof(stream_handle)) {
				STE_CONN_ERR("Not enough data supplied for CHAR_DEV_OP_CODE_STOP_STREAM");
				err = -EINVAL;
				goto finished_mutex_unlock;
			}
			STE_CONN_DBG("CHAR_DEV_OP_CODE_STOP_STREAM %d", sizeof(stream_handle));
			memcpy(&stream_handle, data, sizeof(stream_handle));
			STE_CONN_DBG("stream_handle %d", stream_handle);
			err = ste_cg29xx_audio_stop_stream(dev->session, stream_handle);
			break;

		default:
			STE_CONN_ERR("Received bad op_code %d", op_code);
			break;
		};
	}

finished_mutex_unlock:
	kfree(rec_data);
	mutex_unlock(&dev->rw_mutex);
finished:
	if (err) {
		return err;
	} else {
		return count;
	}
}


/**
 * ste_cg29xx_audio_char_device_poll() - Handle POLL call to the interface.
 * @filp:  Pointer to the file struct.
 * @wait:  Poll table supplied to caller.
 *
 * This function is used by the User Space application to see if the device is
 * still open and if there is any data available for reading.
 *
 * Returns:
 *   Mask of current set POLL values
 */
static unsigned int ste_cg29xx_audio_char_device_poll(struct file *filp,
						poll_table *wait)
{
	struct cg29xx_audio_char_dev_info *dev =
		(struct cg29xx_audio_char_dev_info *)filp->private_data;
	unsigned int mask = 0;

	if (!dev) {
		STE_CONN_ERR("No dev supplied in private data");
		return POLLERR | POLLRDHUP;
	}

	if (RESET == cg29xx_audio_info->state) {
		mask |= POLLERR | POLLRDHUP | POLLPRI;
	}

	if (dev->stored_data) {
		mask |= POLLIN | POLLRDNORM;
	}
	return mask;
}


/*
 * 	Module related methods
 */

/**
 * ste_cg2900_audio_init() - Initialize module.
 *
 * Initialize the module and register misc device.
 *
 * Returns:
 *   0 if there is no error.
 *   -ENOMEM if allocation fails.
 *   -EEXIST if device has already been started.
 *   Error codes from misc_register.
 */
static int __init ste_cg2900_audio_init(void)
{
	int err = 0;

	STE_CONN_INFO("ste_cg2900_audio_init ver %s", VERSION);

	if (cg29xx_audio_info) {
		STE_CONN_ERR("ST-E CG29XX Audio driver already initiated");
		err = -EEXIST;
		goto finished;
	}

	/* Initialize private data. */
	cg29xx_audio_info = kzalloc(sizeof(*cg29xx_audio_info), GFP_KERNEL);
	if (!cg29xx_audio_info) {
		STE_CONN_ERR("Could not alloc cg29xx_audio_info struct.");
		err = -ENOMEM;
		goto finished;
	}

	/* Initiate the mutexes */
	mutex_init(&(cg29xx_audio_info->management_mutex));
	mutex_init(&(cg29xx_audio_info->bt_channel_mutex));
	mutex_init(&(cg29xx_audio_info->fm_channel_mutex));
	mutex_init(&(cg29xx_audio_info->endpoints.management_mutex));

	/* Initiate the SKB queues */
	skb_queue_head_init(&(cg29xx_audio_info->bt_queue));
	skb_queue_head_init(&(cg29xx_audio_info->fm_queue));

	/* Initiate the endpoint list */
	INIT_LIST_HEAD(&(cg29xx_audio_info->endpoints.ep_list));

	/* Prepare and register MISC device */
	cg29xx_audio_info->dev.minor = MISC_DYNAMIC_MINOR;
	cg29xx_audio_info->dev.name = STE_CG29XX_AUDIO_DEVICE_NAME;
	cg29xx_audio_info->dev.fops = &char_dev_fops;
	cg29xx_audio_info->dev.parent = NULL;

	err = misc_register(&(cg29xx_audio_info->dev));
	if (err) {
		STE_CONN_ERR("Error %d registering misc dev!", err);
		goto error_handling;
	}

	goto finished;

error_handling:
	mutex_destroy(&cg29xx_audio_info->management_mutex);
	mutex_destroy(&cg29xx_audio_info->bt_channel_mutex);
	mutex_destroy(&cg29xx_audio_info->fm_channel_mutex);
	mutex_destroy(&cg29xx_audio_info->endpoints.management_mutex);
	kfree(cg29xx_audio_info);
	cg29xx_audio_info = NULL;
finished:
	return err;
}

/**
 * ste_cg2900_audio_exit() - Remove module.
 *
 * Remove misc device and free resources.
 */
static void __exit ste_cg2900_audio_exit(void)
{
	STE_CONN_INFO("ste_cg2900_audio_exit");

	if (cg29xx_audio_info) {
		int err = 0;

		err = misc_deregister(&cg29xx_audio_info->dev);
		if (err) {
			STE_CONN_ERR("Error %d deregistering misc dev!", err);
		}

		mutex_destroy(&cg29xx_audio_info->management_mutex);
		mutex_destroy(&cg29xx_audio_info->bt_channel_mutex);
		mutex_destroy(&cg29xx_audio_info->fm_channel_mutex);

		flush_endpoint_list(&(cg29xx_audio_info->endpoints));

		mutex_destroy(&cg29xx_audio_info->endpoints.management_mutex);

		kfree(cg29xx_audio_info);
		cg29xx_audio_info = NULL;
	}
}

module_init(ste_cg2900_audio_init);
module_exit(ste_cg2900_audio_exit);

MODULE_AUTHOR("Par-Gunnar Hjalmdahl ST-Ericsson");
MODULE_AUTHOR("Kjell Andersson ST-Ericsson");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Linux Bluetooth Audio ST-Ericsson controller ver " VERSION);
MODULE_VERSION(VERSION);
