/*
 * drivers/mfd/ste_conn/ste_cg2900.c
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
 * Linux Bluetooth HCI H:4 Driver for ST-Ericsson connectivity controller CG2900.
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
#include "ste_cg2900.h"
#include "ste_conn_cpd.h"
#include "ste_conn_ccd.h"
#include "ste_conn_debug.h"

/* ENABLE_SYS_CLK_OUT - Enable system clock out on chip */
#define ENABLE_SYS_CLK_OUT

#define CG2900_LINE_BUFFER_LENGTH		128
#define CG2900_FILENAME_MAX			128

#define CG2900_NAME				"ste_conn_cg2900\0"
#define CG2900_WQ_NAME				"ste_conn_cg2900_wq\0"
#define CG2900_PATCH_INFO_FILE			"ste_conn_patch_info.fw"
#define CG2900_FACTORY_SETTINGS_INFO_FILE	"ste_conn_settings_info.fw"

/* Size of file chunk ID */
#define CG2900_FILE_CHUNK_ID_SIZE		1
#define CG2900_VS_SEND_FILE_START_OFFSET_IN_CMD	4
#define CG2900_BT_CMD_LENGTH_POSITION		3

/* State setting macros */
#define CG2900_SET_BOOT_STATE(__cg2900_new_state) \
	STE_CONN_SET_STATE("boot_state", cg2900_info->boot_state, __cg2900_new_state)
#define CG2900_SET_CLOSING_STATE(__cg2900_new_state) \
	STE_CONN_SET_STATE("closing_state", cg2900_info->closing_state, __cg2900_new_state)
#define CG2900_SET_BOOT_FILE_LOAD_STATE(__cg2900_new_state) \
	STE_CONN_SET_STATE("boot_file_load_state", cg2900_info->boot_file_load_state, __cg2900_new_state)
#define CG2900_SET_BOOT_DOWNLOAD_STATE(__cg2900_new_state) \
	STE_CONN_SET_STATE("boot_download_state", cg2900_info->boot_download_state, __cg2900_new_state)

/** CG2900_H4_CHANNEL_BT_CMD - Bluetooth HCI H:4 channel
 * for Bluetooth commands in the ST-Ericsson connectivity controller.
 */
#define CG2900_H4_CHANNEL_BT_CMD		0x01

/** CG2900_H4_CHANNEL_BT_ACL - Bluetooth HCI H:4 channel
 * for Bluetooth ACL data in the ST-Ericsson connectivity controller.
 */
#define CG2900_H4_CHANNEL_BT_ACL		0x02

/** CG2900_H4_CHANNEL_BT_EVT - Bluetooth HCI H:4 channel
 * for Bluetooth events in the ST-Ericsson connectivity controller.
 */
#define CG2900_H4_CHANNEL_BT_EVT		0x04

/** CG2900_H4_CHANNEL_FM_RADIO - Bluetooth HCI H:4 channel
 * for FM radio in the ST-Ericsson connectivity controller.
 */
#define CG2900_H4_CHANNEL_FM_RADIO		0x08

/** CG2900_H4_CHANNEL_GNSS - Bluetooth HCI H:4 channel
 * for GNSS in the ST-Ericsson connectivity controller.
 */
#define CG2900_H4_CHANNEL_GNSS			0x09

/** CG2900_H4_CHANNEL_DEBUG - Bluetooth HCI H:4 channel
 * for internal debug data in the ST-Ericsson connectivity controller.
 */
#define CG2900_H4_CHANNEL_DEBUG			0x0B

/** CG2900_H4_CHANNEL_STE_TOOLS - Bluetooth HCI H:4 channel
 * for development tools data in the ST-Ericsson connectivity controller.
 */
#define CG2900_H4_CHANNEL_STE_TOOLS		0x0D

/** CG2900_H4_CHANNEL_HCI_LOGGER - Bluetooth HCI H:4 channel
 * for logging all transmitted H4 packets (on all channels).
 */
#define CG2900_H4_CHANNEL_HCI_LOGGER		0xFA

/** CG2900_H4_CHANNEL_US_CTRL - Bluetooth HCI H:4 channel
 * for user space control of the ST-Ericsson connectivity controller.
 */
#define CG2900_H4_CHANNEL_US_CTRL		0xFC

/** CG2900_H4_CHANNEL_CORE - Bluetooth HCI H:4 channel
 * for user space control of the ST-Ericsson connectivity controller.
 */
#define CG2900_H4_CHANNEL_CORE			0xFD

/**
  * struct cg2900_work_struct - Work structure for ste_conn CPD module.
  * @work: Work structure.
  * @skb:  Data packet.
  * @data: Private data for ste_conn CPD.
  *
  * This structure is used to pack work for work queue.
  */
struct cg2900_work_struct{
	struct work_struct work;
	struct sk_buff *skb;
	void *data;
};

/**
  * enum cg2900_boot_state - BOOT-state for CPD.
  * @BOOT_STATE_NOT_STARTED:                    Boot has not yet started.
  * @BOOT_STATE_SEND_BD_ADDRESS:		VS Store In FS command with bd address has been sent.
  * @BOOT_STATE_GET_FILES_TO_LOAD:              CPD is retreiving file to load.
  * @BOOT_STATE_DOWNLOAD_PATCH:                 CPD is downloading patches.
  * @BOOT_STATE_ACTIVATE_PATCHES_AND_SETTINGS:  CPD is activating patches and settings.
  * @BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_HIGH:	CPD is activating sys clock out.
  * @BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_LOW:CPD is activating sys clock out.
  * @BOOT_STATE_READY:                          CPD boot is ready.
  * @BOOT_STATE_FAILED:                         CPD boot failed.
  */
enum cg2900_boot_state {
	BOOT_STATE_NOT_STARTED,
	BOOT_STATE_SEND_BD_ADDRESS,
	BOOT_STATE_GET_FILES_TO_LOAD,
	BOOT_STATE_DOWNLOAD_PATCH,
	BOOT_STATE_ACTIVATE_PATCHES_AND_SETTINGS,
	BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_HIGH,
	BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_LOW,
	BOOT_STATE_READY,
	BOOT_STATE_FAILED
};

/**
  * enum cg2900_closing_state - CLOSING-state for CPD.
  * @CLOSING_STATE_RESET: HCI RESET_CMD has been sent.
  * @CLOSING_STATE_POWER_SWITCH_OFF: HCI VS_POWER_SWITCH_OFF command has been sent.
  * @CLOSING_STATE_SHUT_DOWN:  We have now shut down the chip.
  */
enum cg2900_closing_state {
	CLOSING_STATE_RESET,
	CLOSING_STATE_POWER_SWITCH_OFF,
	CLOSING_STATE_SHUT_DOWN
};

/**
  * enum cg2900_boot_file_load_state - BOOT_FILE_LOAD-state for CPD.
  * @BOOT_FILE_LOAD_STATE_LOAD_PATCH:           Loading patches.
  * @BOOT_FILE_LOAD_STATE_LOAD_STATIC_SETTINGS: Loading static settings.
  * @BOOT_FILE_LOAD_STATE_NO_MORE_FILES:        No more files to load.
  * @BOOT_FILE_LOAD_STATE_FAILED:               File loading failed.
  */
enum cg2900_boot_file_load_state {
	BOOT_FILE_LOAD_STATE_LOAD_PATCH,
	BOOT_FILE_LOAD_STATE_LOAD_STATIC_SETTINGS,
	BOOT_FILE_LOAD_STATE_NO_MORE_FILES,
	BOOT_FILE_LOAD_STATE_FAILED
};

/**
  * enum cg2900_boot_download_state - BOOT_DOWNLOAD state.
  * @BOOT_DOWNLOAD_STATE_PENDING: Download in progress.
  * @BOOT_DOWNLOAD_STATE_SUCCESS: Download successfully finished.
  * @BOOT_DOWNLOAD_STATE_FAILED:  Downloading failed.
  */
enum cg2900_boot_download_state {
	BOOT_DOWNLOAD_STATE_PENDING,
	BOOT_DOWNLOAD_STATE_SUCCESS,
	BOOT_DOWNLOAD_STATE_FAILED
};

/**
  * enum cg2900_fm_radio_mode - FM Radio mode.
  * It's needed because some FM do-commands generate interrupts
  * only when the fm driver is in specific mode and we need to know
  * if we should expect the interrupt.
  * @CG2900_FM_RADIO_MODE_IDLE:  Radio mode is Idle (default).
  * @CG2900_FM_RADIO_MODE_FMT:   Radio mode is set to FMT (transmitter).
  * @CG2900_FM_RADIO_MODE_FMR:   Radio mode is set to FMR (receiver).
  */
enum cg2900_fm_radio_mode {
	CG2900_FM_RADIO_MODE_IDLE  = 0,
	CG2900_FM_RADIO_MODE_FMT  = 1,
	CG2900_FM_RADIO_MODE_FMR  = 2
};

/**
  * struct tx_list_item - Structure to store skb and sender name if skb can't be sent due to flow control.
  * @list:      list_head struct.
  * @skb:       sk_buff struct.
  * @dev:       sending device struct pointer.
  */
struct tx_list_item {
	struct list_head list;
	struct sk_buff *skb;
	struct ste_conn_device *dev;
};

/**
  * struct cg2900_device_id - Structure for connecting H4 channel to named user.
  * @name:        Name of device.
  * @h4_channel:  HCI H:4 channel used by this device.
  */
struct cg2900_device_id {
	char *name;
	int   h4_channel;
};

/**
  * struct cg2900_info - Main info structure for CG2900.
  * @patch_file_name:		Stores patch file name.
  * @settings_file_name:	Stores settings file name.
  * @fw_file:			Stores firmware file (patch or settings).
  * @fw_file_rd_offset:		Current read offset in firmware file.
  * @chunk_id:			Stores current chunk ID of write file operations.
  * @boot_state:		Current BOOT-state of CG2900.
  * @closing_state:		Current CLOSING-state of CG2900.
  * @boot_file_load_state:	Current BOOT_FILE_LOAD-state of CG2900.
  * @boot_download_state:	Current BOOT_DOWNLOAD-state of CG2900.
  * @wq:			CG2900 workqueue.
  * @chip_dev:			Chip handler info.
  * @tx_list_bt:		TX queue for HCI BT commands when nr of commands allowed is 0 (ste_conn internal flow control).
  * @tx_list_fm:		TX queue for HCI FM commands when nr of commands allowed is 0 (ste_conn internal flow control).
  * @tx_bt_lock:		Spinlock used to protect some global structures related to internal BT command flow control.
  * @tx_fm_lock:		Spinlock used to protect some global structures related to internal FM command flow control.
  * @tx_fm_audio_awaiting_irpt:	Indicates if an FM interrupt event related to audio driver command is expected.
  * @fm_radio_mode:		Current FM radio mode.
  * @tx_nr_pkts_allowed_bt:	Number of packets allowed to send on BT HCI CMD H4 channel.
  * @hci_audio_cmd_opcode_bt:	Stores the OpCode of the last sent audio driver HCI BT CMD.
  * @hci_audio_fm_cmd_id:	Stores the command id of the last sent HCI FM RADIO command by the fm audio user.
  * @hci_fm_cmd_func:		Stores the command function of the last sent HCI FM RADIO command by the fm radio user.
  */
struct cg2900_info {
	char					*patch_file_name;
	char					*settings_file_name;
	const struct firmware			*fw_file;
	int					fw_file_rd_offset;
	uint8_t					chunk_id;
	enum cg2900_boot_state			boot_state;
	enum cg2900_closing_state		closing_state;
	enum cg2900_boot_file_load_state	boot_file_load_state;
	enum cg2900_boot_download_state		boot_download_state;
	struct workqueue_struct			*wq;
	struct ste_conn_cpd_chip_dev		chip_dev;
	struct list_head			tx_list_bt;
	struct list_head			tx_list_fm;
	spinlock_t				tx_bt_lock;
	spinlock_t				tx_fm_lock;
	bool					tx_fm_audio_awaiting_irpt;
	enum cg2900_fm_radio_mode		fm_radio_mode;
	int					tx_nr_pkts_allowed_bt;
	uint16_t				hci_audio_cmd_opcode_bt;
	uint16_t				hci_audio_fm_cmd_id;
	uint16_t				hci_fm_cmd_func;
};

static struct cg2900_info *cg2900_info;

/*
  * cg2900_msg_reset_cmd_req - Hardcoded HCI Reset command.
  */
static const uint8_t cg2900_msg_reset_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	HCI_BT_MAKE_FIRST_BYTE_IN_CMD(HCI_BT_OCF_RESET),
	HCI_BT_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_CTRL_BB, HCI_BT_OCF_RESET),
	0x00
};

/*
  * cg2900_msg_vs_store_in_fs_cmd_req - Hardcoded HCI Store in FS command.
  */
static const uint8_t cg2900_msg_vs_store_in_fs_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	HCI_BT_MAKE_FIRST_BYTE_IN_CMD(CG2900_BT_OCF_VS_STORE_IN_FS),
	HCI_BT_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, CG2900_BT_OCF_VS_STORE_IN_FS),
	0x00, /* 1 byte for HCI command length. */
	0x00, /* 1 byte for User_Id. */
	0x00  /* 1 byte for vs_store_in_fs_cmd_req parameter data length. */
};

/*
  * cg2900_msg_vs_write_file_block_cmd_req -
  * Hardcoded HCI Write File Block vendor specific command
  */
static const uint8_t cg2900_msg_vs_write_file_block_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	HCI_BT_MAKE_FIRST_BYTE_IN_CMD(CG2900_BT_OCF_VS_WRITE_FILE_BLOCK),
	HCI_BT_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, CG2900_BT_OCF_VS_WRITE_FILE_BLOCK),
	0x00
};

/*
  * cg2900_msg_vs_system_reset_cmd_req -
  * Hardcoded HCI System Reset vendor specific command
  */
static const uint8_t cg2900_msg_vs_system_reset_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	HCI_BT_MAKE_FIRST_BYTE_IN_CMD(CG2900_BT_OCF_VS_SYSTEM_RESET),
	HCI_BT_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, CG2900_BT_OCF_VS_SYSTEM_RESET),
	0x00
};

#ifdef ENABLE_SYS_CLK_OUT
/*
  * cg2900_msg_read_register_0x40014004 -
  * Hardcoded HCI Read_Register 0x40014004
  */
static const uint8_t cg2900_msg_read_register_0x40014004[] = {
	0x00, /* Reserved for H4 channel*/
	HCI_BT_MAKE_FIRST_BYTE_IN_CMD(CG2900_BT_OCF_VS_READ_REGISTER),
	HCI_BT_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, CG2900_BT_OCF_VS_READ_REGISTER),
	0x05, /* HCI command length. */
	0x01, /* Register type = 32-bit*/
	0X04, /* Memory address byte 1*/
	0X40, /* Memory address byte 2*/
	0X01, /* Memory address byte 3*/
	0X40  /* Memory address byte 4*/
};

/*
  * cg2900_msg_write_register_0x40014004 -
  * Hardcoded HCI Write_Register 0x40014004
  */
static const uint8_t cg2900_msg_write_register_0x40014004[] = {
	0x00, /* Reserved for H4 channel*/
	HCI_BT_MAKE_FIRST_BYTE_IN_CMD(CG2900_BT_OCF_VS_WRITE_REGISTER),
	HCI_BT_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, CG2900_BT_OCF_VS_WRITE_REGISTER),
	0x09, /* HCI command length. */
	0x01, /* Register type = 32-bit*/
	0X04, /* Memory address byte 1*/
	0X40, /* Memory address byte 2*/
	0X01, /* Memory address byte 3*/
	0X40, /* Memory address byte 4*/
	0X00, /* Register value byte 1*/
	0X00, /* Register value byte 2*/
	0X00, /* Register value byte 3*/
	0X00  /* Register value byte 4*/
};
#endif /*ENABLE_SYS_CLK_OUT*/

/*
  * time_500ms - 500 millisecond time struct.
  */
static struct timeval time_500ms = {
	.tv_sec = 0,
	.tv_usec = 500 * USEC_PER_MSEC
};

/*
 * cg2900_channels() - Array containing available H4 channels for the CG2900
 * ST-Ericsson Connectivity controller.
 */
struct cg2900_device_id cg2900_channels[] = {
	{STE_CONN_DEVICES_BT_CMD,		CG2900_H4_CHANNEL_BT_CMD},
	{STE_CONN_DEVICES_BT_ACL,		CG2900_H4_CHANNEL_BT_ACL},
	{STE_CONN_DEVICES_BT_EVT,		CG2900_H4_CHANNEL_BT_EVT},
	{STE_CONN_DEVICES_GNSS,			CG2900_H4_CHANNEL_GNSS},
	{STE_CONN_DEVICES_FM_RADIO,		CG2900_H4_CHANNEL_FM_RADIO},
	{STE_CONN_DEVICES_DEBUG,		CG2900_H4_CHANNEL_DEBUG},
	{STE_CONN_DEVICES_STE_TOOLS,		CG2900_H4_CHANNEL_STE_TOOLS},
	{STE_CONN_DEVICES_HCI_LOGGER,		CG2900_H4_CHANNEL_HCI_LOGGER},
	{STE_CONN_DEVICES_US_CTRL,		CG2900_H4_CHANNEL_US_CTRL},
	{STE_CONN_DEVICES_BT_AUDIO,		CG2900_H4_CHANNEL_BT_CMD},
	{STE_CONN_DEVICES_FM_RADIO_AUDIO,	CG2900_H4_CHANNEL_FM_RADIO},
	{STE_CONN_DEVICES_CORE,			CG2900_H4_CHANNEL_CORE}
};

/* ------------------ Internal function declarations ---------------------- */

static int cg2900_chip_startup(struct ste_conn_cpd_chip_dev *dev);
static int cg2900_chip_shutdown(struct ste_conn_cpd_chip_dev *dev);
static bool cg2900_data_to_chip(struct ste_conn_cpd_chip_dev *dev, struct ste_conn_device *cpd_dev, struct sk_buff *skb);
static bool cg2900_data_from_chip(struct ste_conn_cpd_chip_dev *dev, struct ste_conn_device *cpd_dev, struct sk_buff *skb);
static int cg2900_get_h4_channel(char *name, int *h4_channel);
static bool cg2900_is_bt_audio_user(int h4_channel, const struct sk_buff * const skb);
static bool cg2900_is_fm_audio_user(int h4_channel, const struct sk_buff * const skb);
static void cg2900_last_bt_user_removed(struct ste_conn_cpd_chip_dev *dev);
static void cg2900_last_fm_user_removed(struct ste_conn_cpd_chip_dev *dev);
static bool cg2900_check_chip_support(struct ste_conn_cpd_chip_dev *dev);
static void cg2900_update_internal_flow_control_bt_cmd_evt(const struct sk_buff * const skb);
static void cg2900_update_internal_flow_control_fm(const struct sk_buff * const skb);
static bool cg2900_fm_irpt_expected(uint16_t cmd_id);
static bool cg2900_fm_is_do_cmd_irpt(uint16_t irpt_val);
static bool cg2900_handle_internal_rx_data_bt_evt(struct sk_buff *skb);
static void cg2900_create_work_item(work_func_t work_func, struct sk_buff *skb, void *data);
static bool cg2900_handle_reset_cmd_complete_evt(uint8_t *data);
#ifdef ENABLE_SYS_CLK_OUT
static bool cg2900_handle_vs_read_register_cmd_complete_evt(uint8_t *data);
static bool cg2900_handle_vs_write_register_cmd_complete_evt(uint8_t *data);
#endif /* ENABLE_SYS_CLK_OUT */
static bool cg2900_handle_vs_store_in_fs_cmd_complete_evt(uint8_t *data);
static bool cg2900_handle_vs_write_file_block_cmd_complete_evt(uint8_t *data);
static bool cg2900_handle_vs_power_switch_off_cmd_complete_evt(uint8_t *data);
static bool cg2900_handle_vs_system_reset_cmd_complete_evt(uint8_t *data);
static void cg2900_work_power_off_chip(struct work_struct *work);
static void cg2900_work_reset_after_error(struct work_struct *work);
static void cg2900_work_load_patch_and_settings(struct work_struct *work);
static void cg2900_work_continue_with_file_download(struct work_struct *work);
static bool cg2900_get_file_to_load(const struct firmware *fw, char **file_name_p);
static char *cg2900_get_one_line_of_text(char *wr_buffer, int max_nbr_of_bytes,
					char *rd_buffer, int *bytes_copied);
static void cg2900_read_and_send_file_part(void);
static void cg2900_send_patch_file(void);
static void cg2900_send_settings_file(void);
static void cg2900_create_and_send_bt_cmd(const uint8_t *data, int length);
static void cg2900_send_bd_address(void);
static void cg2900_transmit_skb_to_ccd_with_flow_ctrl_bt_cmd(struct sk_buff *skb,
		struct ste_conn_device *dev);
static void cg2900_fm_reset_flow_ctrl(void);
static void cg2900_fm_parse_cmd(uint8_t *data, uint8_t *cmd_func, uint16_t *cmd_id);
static void cg2900_fm_parse_event(uint8_t *data, uint8_t *event, uint8_t *cmd_func,
				  uint16_t *cmd_id, uint16_t *intr_val);
static void cg2900_fm_update_mode(uint8_t *data);
static void cg2900_transmit_skb_to_ccd_with_flow_ctrl_fm(struct sk_buff *skb,
		struct ste_conn_device *dev);
static void cg2900_transmit_skb_from_tx_queue_bt(void);
static void cg2900_transmit_skb_from_tx_queue_fm(void);

static struct ste_conn_cpd_id_callbacks cg2900_id_callbacks = {
	.check_chip_support = cg2900_check_chip_support
};


/*
 *	Internal function
 */

/**
 * cg2900_chip_startup() - Start the chip.
 * @dev:	Chip info.
 *
 * The cg2900_chip_startup() function downloads patches and other needed start procedures.
 *
 * Returns:
 *   0 if there is no error.
 */
static int cg2900_chip_startup(struct ste_conn_cpd_chip_dev *dev)
{
	int err = 0;

	/* Start the boot sequence */
	CG2900_SET_BOOT_STATE(BOOT_STATE_GET_FILES_TO_LOAD);
	cg2900_create_work_item(cg2900_work_load_patch_and_settings, NULL, NULL);

	return err;
}

/**
 * cg2900_chip_shutdown() - Shut down the chip.
 * @dev:	Chip info.
 *
 * The cg2900_chip_shutdown() function shuts down the chip by sending PowerSwitchOff command.
 *
 * Returns:
 *   0 if there is no error.
 */
static int cg2900_chip_shutdown(struct ste_conn_cpd_chip_dev *dev)
{
	int err = 0;

	/* Transmit HCI reset command to ensure the chip is using
	 * the correct transport and to put BT part in reset */
	CG2900_SET_CLOSING_STATE(CLOSING_STATE_RESET);
	cg2900_create_and_send_bt_cmd(cg2900_msg_reset_cmd_req,
				      sizeof(cg2900_msg_reset_cmd_req));

	return err;
}

/**
 * cg2900_data_to_chip() - Called when data shall be sent to the chip.
 * @dev:	Chip info.
 * @cpd_dev:	STE_CONN user for this packet.
 * @skb:	Packet to transmit.
 *
 * The cg2900_data_to_chip() function updates flow control and itself
 * transmits packet to CCD if packet is BT command or FM radio.
 *
 * Returns:
 *   true if packet is handled by this driver.
 *   false otherwise.
 */
static bool cg2900_data_to_chip(struct ste_conn_cpd_chip_dev *dev,
				struct ste_conn_device *cpd_dev,
				struct sk_buff *skb)
{
	bool packet_handled = false;

	if (cpd_dev->h4_channel == CG2900_H4_CHANNEL_BT_CMD) {
		cg2900_transmit_skb_to_ccd_with_flow_ctrl_bt_cmd(skb, cpd_dev);
		packet_handled = true;
	} else if (cpd_dev->h4_channel == CG2900_H4_CHANNEL_FM_RADIO) {
		cg2900_transmit_skb_to_ccd_with_flow_ctrl_fm(skb, cpd_dev);
		packet_handled = true;
	}

	return packet_handled;
}

/**
 * cg2900_data_from_chip() - Called when data shall be sent to the chip.
 * @dev:	Chip info.
 * @cpd_dev:	STE_CONN user for this packet.
 * @skb:	Packet received.
 *
 * The cg2900_data_from_chip() function updates flow control and checks
 * if packet is a response for a packet it itself has transmitted.
 *
 * Returns:
 *   true if packet is handled by this driver.
 *   false otherwise.
 */
static bool cg2900_data_from_chip(struct ste_conn_cpd_chip_dev *dev,
				  struct ste_conn_device *cpd_dev,
				  struct sk_buff *skb)
{
	bool packet_handled = false;
	int h4_channel;

	h4_channel = skb->data[0];

	/* First check if we should update flow control */
	if (h4_channel == CG2900_H4_CHANNEL_BT_EVT) {
		cg2900_update_internal_flow_control_bt_cmd_evt(skb);
	} else if (h4_channel == CG2900_H4_CHANNEL_FM_RADIO) {
		cg2900_update_internal_flow_control_fm(skb);
	}

	/* Then check if this is a response to data we have sent */
	packet_handled = cg2900_handle_internal_rx_data_bt_evt(skb);

	return packet_handled;
}

/**
 * cg2900_get_h4_channel() - Returns H:4 channel for the name.
 * @name:	Chip info.
 * @h4_channel:	STE_CONN user for this packet.
 *
 * Returns:
 *   0 if there is no error.
 *   -ENXIO if channel is not found.
 */
static int cg2900_get_h4_channel(char *name, int *h4_channel)
{
	int i;
	int err = -ENXIO;

	*h4_channel = -1;

	for (i = 0; *h4_channel == -1 && i < ARRAY_SIZE(cg2900_channels); i++) {
		if (0 == strncmp(name, cg2900_channels[i].name, STE_CONN_MAX_NAME_SIZE)) {
			/* Device found. Return H4 channel */
			*h4_channel = cg2900_channels[i].h4_channel;
			err = 0;
		}
	}

	return err;
}

/**
 * cg2900_is_bt_audio_user() - Checks if this packet is for the BT audio user.
 * @h4_channel:	H:4 channel for this packet.
 * @skb:	Packet to check.
 *
 * Returns:
 *   true if packet is for BT audio user.
 *   false otherwise.
 */
static bool cg2900_is_bt_audio_user(int h4_channel, const struct sk_buff * const skb)
{
	uint8_t *data = &(skb->data[STE_CONN_SKB_RESERVE]);
	uint8_t event_code = data[0];
	bool bt_audio = false;

	if (h4_channel == CG2900_H4_CHANNEL_BT_EVT) {
		uint16_t opcode = 0;

		if (HCI_BT_EVT_CMD_COMPLETE == event_code) {
			opcode = HCI_BT_EVENT_GET_OPCODE(data[3], data[4]);
		} else if (HCI_BT_EVT_CMD_STATUS == event_code) {
			opcode = HCI_BT_EVENT_GET_OPCODE(data[4], data[5]);
		}

		if (opcode != 0 && opcode == cg2900_info->hci_audio_cmd_opcode_bt) {
			STE_CONN_DBG("BT OpCode match = 0x%04X", opcode);
			cg2900_info->hci_audio_cmd_opcode_bt = CG2900_BT_OPCODE_NONE;
			bt_audio = true;
		}
	}

	return bt_audio;
}

/**
 * cg2900_is_fm_audio_user() - Checks if this packet is for the FM audio user.
 * @h4_channel:	H:4 channel for this packet.
 * @skb:	Packet to check.
 *
 * Returns:
 *   true if packet is for BT audio user.
 *   false otherwise.
 */
static bool cg2900_is_fm_audio_user(int h4_channel, const struct sk_buff * const skb)
{
	uint8_t cmd_func = CG2900_FM_CMD_PARAMETERS_NONE;
	uint16_t cmd_id = CG2900_FM_CMD_NONE;
	uint16_t irpt_val = 0;
	uint8_t event = CG2900_FM_EVENT_UNKNOWN;
	bool bt_audio = false;

	cg2900_fm_parse_event(&(skb->data[0]), &event, &cmd_func, &cmd_id, &irpt_val);

	if (h4_channel == CG2900_H4_CHANNEL_FM_RADIO) {
		/* Check if command complete event FM legacy interface. */
		if (event == CG2900_FM_EVENT_CMD_COMPLETE) {
			if ((cmd_func == CG2900_FM_CMD_PARAMETERS_WRITECOMMAND) &&
			    cmd_id == cg2900_info->hci_audio_fm_cmd_id) {
				STE_CONN_DBG("FM Audio Function Code match = 0x%04X", cmd_id);
				bt_audio = true;
				goto finished;
			}
		}

		/* Check if Interrupt legacy interface. */
		if (event == CG2900_FM_EVENT_INTERRUPT) {
			if ((cg2900_fm_is_do_cmd_irpt(irpt_val)) &&
			(cg2900_info->tx_fm_audio_awaiting_irpt)) {
				bt_audio = true;
			}
		}
	}

finished:
	return bt_audio;
}

/**
 * cg2900_last_bt_user_removed() - Called when last BT user is removed.
 * @dev:	Chip handler info.
 *
 * Clears out TX queue for BT.
 */
static void cg2900_last_bt_user_removed(struct ste_conn_cpd_chip_dev *dev)
{
	struct list_head *cursor, *next;
	struct tx_list_item *tmp;

	spin_lock_bh(&cg2900_info->tx_bt_lock);
	list_for_each_safe(cursor, next, &(cg2900_info->tx_list_bt)) {
			tmp = list_entry(cursor, struct tx_list_item, list);
			list_del(cursor);
			kfree_skb(tmp->skb);
			kfree(tmp);
	}

	/* Reset number of packets allowed and number of outstanding bt commands. */
	cg2900_info->tx_nr_pkts_allowed_bt = 1;
	/* Reset the hci_audio_cmd_opcode_bt. */
	cg2900_info->hci_audio_cmd_opcode_bt = CG2900_BT_OPCODE_NONE;
	spin_unlock_bh(&cg2900_info->tx_bt_lock);
}

/**
 * cg2900_last_fm_user_removed() - Called when last FM user is removed.
 * @dev:	Chip handler info.
 *
 * Clears out TX queue for BT.
 */
static void cg2900_last_fm_user_removed(struct ste_conn_cpd_chip_dev *dev)
{
	spin_lock_bh(&cg2900_info->tx_fm_lock);
	cg2900_fm_reset_flow_ctrl();
	spin_unlock_bh(&cg2900_info->tx_fm_lock);
}

/**
 * cg2900_check_chip_support() - Checks if connected chip is handled by this driver.
 * @h4_channel:	H:4 channel for this packet.
 * @skb:	Packet to check.
 *
 * If supported return true and fill in @callbacks.
 *
 * Returns:
 *   true if chip is handled by this driver.
 *   false otherwise.
 */
static bool cg2900_check_chip_support(struct ste_conn_cpd_chip_dev *dev)
{
	bool ret_val = false;

	STE_CONN_INFO("cg2900_check_chip_support");

	/* Check if this is a CG2900 revision. We do not care about the sub-version at the moment.
	 * Change this if necessary */
	if ((dev->chip.manufacturer != CG2900_SUPP_MANUFACTURER) ||
	    (dev->chip.hci_revision < CG2900_SUPP_REVISION_MIN) ||
	    (dev->chip.hci_revision > CG2900_SUPP_REVISION_MAX)) {
		STE_CONN_DBG("Chip not supported by CG2900 driver\n\tMan: 0x%02X\n\tRev: 0x%04X\n\tSub: 0x%04X",
				dev->chip.manufacturer, dev->chip.hci_revision, dev->chip.hci_sub_version);
		goto finished;
	}

	STE_CONN_INFO("Chip supported by the CG2900 driver");
	ret_val = true;
	/* Store needed data */
	dev->user_data = cg2900_info;
	memcpy(&(cg2900_info->chip_dev), dev, sizeof(*dev));
	/* Set the callbacks */
	dev->callbacks.chip_shutdown = cg2900_chip_shutdown;
	dev->callbacks.chip_startup = cg2900_chip_startup;
	dev->callbacks.data_from_chip = cg2900_data_from_chip;
	dev->callbacks.data_to_chip = cg2900_data_to_chip;
	dev->callbacks.get_h4_channel = cg2900_get_h4_channel;
	dev->callbacks.is_bt_audio_user = cg2900_is_bt_audio_user;
	dev->callbacks.is_fm_audio_user = cg2900_is_fm_audio_user;
	dev->callbacks.last_bt_user_removed = cg2900_last_bt_user_removed;
	dev->callbacks.last_fm_user_removed = cg2900_last_fm_user_removed;

finished:
	return ret_val;
}

/**
 * cg2900_update_internal_flow_control_bt_cmd_evt() - Update number of tickets and number of outstanding commands for BT CMD channel.
 * @skb:  skb with received packet.
 *
 * The cg2900_update_internal_flow_control_bt_cmd_evt() checks if incoming data packet is BT Command Complete/Command Status
 * Event and if so updates number of tickets and number of outstanding commands. It also calls function to send queued
 * commands (if the list of queued commands is not empty).
 */
static void cg2900_update_internal_flow_control_bt_cmd_evt(const struct sk_buff * const skb)
{
	uint8_t *data = &(skb->data[STE_CONN_SKB_RESERVE]);
	uint8_t event_code = data[0];

	if (HCI_BT_EVT_CMD_COMPLETE == event_code) {
		/* If it's HCI Command Complete Event then we might get some HCI tickets back. Also we can decrease the number
		 * outstanding HCI commands (if it's not NOP command or one of the commands that generate both Command Status Event
		 * and Command Complete Event). Check if we have any HCI commands waiting in the tx list
		 * and send them if there are tickets available. */
		spin_lock_bh(&(cg2900_info->tx_bt_lock));
		cg2900_info->tx_nr_pkts_allowed_bt = data[HCI_BT_EVT_CMD_COMPL_NR_OF_PKTS_POS];
		STE_CONN_DBG("New tx_nr_pkts_allowed_bt = %d", cg2900_info->tx_nr_pkts_allowed_bt);

		if (!list_empty(&(cg2900_info->tx_list_bt))) {
			cg2900_transmit_skb_from_tx_queue_bt();
		}
		spin_unlock_bh(&(cg2900_info->tx_bt_lock));
	} else if (HCI_BT_EVT_CMD_STATUS == event_code) {
		/* If it's HCI Command Status Event then we might get some HCI tickets back. Also we can decrease the number
		 * outstanding HCI commands (if it's not NOP command). Check if we have any HCI commands waiting in the tx queue
		 * and send them if there are tickets available. */
		spin_lock_bh(&(cg2900_info->tx_bt_lock));
		cg2900_info->tx_nr_pkts_allowed_bt = data[HCI_BT_EVT_CMD_STATUS_NR_OF_PKTS_POS];
		STE_CONN_DBG("New tx_nr_pkts_allowed_bt = %d", cg2900_info->tx_nr_pkts_allowed_bt);
		if (!list_empty(&(cg2900_info->tx_list_bt))) {
			cg2900_transmit_skb_from_tx_queue_bt();
		}
		spin_unlock_bh(&(cg2900_info->tx_bt_lock));
	}
}

/**
 * cg2900_update_internal_flow_control_fm() - Update packets allowed for FM channel.
 * @skb:  skb with received packet.
 *
 * The cg2900_update_internal_flow_control_fm() checks if incoming data packet is FM packet
 * indicating that the previous command has been handled and if so update packets. It also calls
 * function to send queued commands (if the list of queued commands is not empty).
 */
static void cg2900_update_internal_flow_control_fm(const struct sk_buff * const skb)
{
	uint8_t cmd_func = CG2900_FM_CMD_PARAMETERS_NONE;
	uint16_t cmd_id = CG2900_FM_CMD_NONE;
	uint16_t irpt_val = 0;
	uint8_t event = CG2900_FM_EVENT_UNKNOWN;

	cg2900_fm_parse_event(&(skb->data[0]), &event, &cmd_func, &cmd_id, &irpt_val);

	/* Check if FM legacy command complete event. */
	if (event == CG2900_FM_EVENT_CMD_COMPLETE) {
		spin_lock_bh(&(cg2900_info->tx_fm_lock));
		/* Check if it's not an write command complete event, because then it cannot be a do command.
		 * Or if it's a write command complete event check that is not a do command complete event,
		 * before setting the outstanding fm packets to none.
		*/
		if (cmd_func != CG2900_FM_CMD_PARAMETERS_WRITECOMMAND || !cg2900_fm_irpt_expected(cmd_id)) {
			cg2900_info->hci_fm_cmd_func = CG2900_FM_CMD_PARAMETERS_NONE;
			cg2900_info->hci_audio_fm_cmd_id = CG2900_FM_CMD_NONE;
			STE_CONN_DBG("FM cmd outstanding cmd func 0x%x", cg2900_info->hci_fm_cmd_func);
			STE_CONN_DBG("FM cmd Audio outstanding cmd id 0x%x", cg2900_info->hci_audio_fm_cmd_id);
			cg2900_transmit_skb_from_tx_queue_fm();

		/* If there was a write do command complete event check if it is do command previously sent by the fm audio user.
		 * Then we need remember that in order to be able to dispatch the interrupt to the correct user.
		 */
		} else if (cmd_id == cg2900_info->hci_audio_fm_cmd_id) {
			cg2900_info->tx_fm_audio_awaiting_irpt = true;
			STE_CONN_DBG("FM Audio waiting for interrupt = true.");
		}
		spin_unlock_bh(&(cg2900_info->tx_fm_lock));
	/* Check if Interrupt FM legacy interrupt. */
	} else if (event == CG2900_FM_EVENT_INTERRUPT) {
		if (cg2900_fm_is_do_cmd_irpt(irpt_val)) {
			/* If it is an interrupt related to a do-command update the outstanding. */
			spin_lock_bh(&(cg2900_info->tx_fm_lock));
			cg2900_info->hci_fm_cmd_func = CG2900_FM_CMD_PARAMETERS_NONE;
			cg2900_info->hci_audio_fm_cmd_id = CG2900_FM_CMD_NONE;
			STE_CONN_DBG("FM cmd outstanding cmd func 0x%x", cg2900_info->hci_fm_cmd_func);
			STE_CONN_DBG("FM cmd Audio outstanding cmd id 0x%x", cg2900_info->hci_audio_fm_cmd_id);
			cg2900_info->tx_fm_audio_awaiting_irpt = false;
			STE_CONN_DBG("FM Audio waiting for interrupt = false.");
			cg2900_transmit_skb_from_tx_queue_fm();
			spin_unlock_bh(&(cg2900_info->tx_fm_lock));
		}
	}
}

/**
 * cg2900_fm_irpt_expected() - check if this FM command will generate an interrupt.
 * @cmd_id:     command identifier.
 *
 * The cg2900_fm_irpt_expected() checks if this FM command will generate an interrupt.
 *
 * Returns:
 *   true if the command will generate an interrupt.
 *   false if it won't.
 */
static bool cg2900_fm_irpt_expected(uint16_t cmd_id)
{
	bool retval = false;

	switch (cmd_id) {
	case CG2900_FM_DO_AIP_FADE_START:
		if (cg2900_info->fm_radio_mode == CG2900_FM_RADIO_MODE_FMT) {
			retval = true;
		}
		break;

	case CG2900_FM_DO_AUP_BT_FADE_START:
	case CG2900_FM_DO_AUP_EXT_FADE_START:
	case CG2900_FM_DO_AUP_FADE_START:
		if (cg2900_info->fm_radio_mode == CG2900_FM_RADIO_MODE_FMR) {
			retval = true;
		}
		break;

	case CG2900_FM_DO_FMR_SETANTENNA:
	case CG2900_FM_DO_FMR_SP_AFSWITCH_START:
	case CG2900_FM_DO_FMR_SP_AFUPDATE_START:
	case CG2900_FM_DO_FMR_SP_BLOCKSCAN_START:
	case CG2900_FM_DO_FMR_SP_PRESETPI_START:
	case CG2900_FM_DO_FMR_SP_SCAN_START:
	case CG2900_FM_DO_FMR_SP_SEARCH_START:
	case CG2900_FM_DO_FMR_SP_SEARCHPI_START:
	case CG2900_FM_DO_FMR_SP_TUNE_SETCHANNEL:
	case CG2900_FM_DO_FMR_SP_TUNE_STEPCHANNEL:
	case CG2900_FM_DO_FMT_PA_SETCONTROL:
	case CG2900_FM_DO_FMT_PA_SETMODE:
	case CG2900_FM_DO_FMT_SP_TUNE_SETCHANNEL:
	case CG2900_FM_DO_GEN_ANTENNACHECK_START:
	case CG2900_FM_DO_GEN_GOTOMODE:
	case CG2900_FM_DO_GEN_POWERSUPPLY_SETMODE:
	case CG2900_FM_DO_GEN_SELECTREFERENCECLOCK:
	case CG2900_FM_DO_GEN_SETPROCESSINGCLOCK:
	case CG2900_FM_DO_GEN_SETREFERENCECLOCKPLL:
	case CG2900_FM_DO_TST_TX_RAMP_START:
		retval = true;
		break;

	default:
		break;
	}

	if (retval) {
		STE_CONN_INFO("Following interrupt event expected for this Cmd cmpl evt, cmd_id = 0x%x.", cmd_id);
	}

	return retval;
}

/**
 * cg2900_fm_is_do_cmd_irpt() - check if irpt_val is one of the FM do-command related interrupts.
 * @irpt_val:     interrupt value.
 *
 * The cg2900_fm_is_do_cmd_irpt() checks if irpt_val is one of the FM do-command related interrupts.
 *
 * Returns:
 *   true if it's do-command related interrupt value.
 *   false if it's not.
 */
static bool cg2900_fm_is_do_cmd_irpt(uint16_t irpt_val)
{
	bool retval = false;

	if ((irpt_val & CG2900_FM_IRPT_OPERATION_SUCCEEDED) ||
	    (irpt_val & CG2900_FM_IRPT_OPERATION_FAILED)) {
		retval = true;
	}

	if (retval) {
		STE_CONN_INFO("Irpt evt for FM do-command found, irpt_val = 0x%x.", irpt_val);
	}

	return retval;
}

/**
 * cg2900_handle_internal_rx_data_bt_evt() - Check if received data should be handled in CPD.
 * @skb: Data packet
 * The cg2900_handle_internal_rx_data_bt_evt() function checks if received data should be handled in CPD.
 * If so handle it correctly. Received data is always HCI BT Event.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cg2900_handle_internal_rx_data_bt_evt(struct sk_buff *skb)
{
	bool pkt_handled = false;
	/* skb cannot be NULL here so it is safe to de-reference */
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
				pkt_handled = cg2900_handle_reset_cmd_complete_evt(data);
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
			case CG2900_BT_OCF_VS_STORE_IN_FS:
				pkt_handled = cg2900_handle_vs_store_in_fs_cmd_complete_evt(data);
				break;
			case CG2900_BT_OCF_VS_WRITE_FILE_BLOCK:
				pkt_handled = cg2900_handle_vs_write_file_block_cmd_complete_evt(data);
				break;

			case CG2900_BT_OCF_VS_POWER_SWITCH_OFF:
				pkt_handled = cg2900_handle_vs_power_switch_off_cmd_complete_evt(data);
				break;

			case CG2900_BT_OCF_VS_SYSTEM_RESET:
				pkt_handled = cg2900_handle_vs_system_reset_cmd_complete_evt(data);
				break;
#ifdef ENABLE_SYS_CLK_OUT
			case CG2900_BT_OCF_VS_READ_REGISTER:
				pkt_handled = cg2900_handle_vs_read_register_cmd_complete_evt(data);
				break;
			case CG2900_BT_OCF_VS_WRITE_REGISTER:
				pkt_handled = cg2900_handle_vs_write_register_cmd_complete_evt(data);
				break;
#endif /* ENABLE_SYS_CLK_OUT */
			default:
				break;
			}; /* switch (hci_ocf) */
			break;

		default:
			break;
		}; /* switch (hci_ogf) */
	}

	if (pkt_handled) {
		kfree_skb(skb);
	}
	return pkt_handled;
}

/**
 * cg2900_create_work_item() - Create work item and add it to the work queue.
 * @work_func:	Work function.
 * @skb: 	Data packet.
 * @data: 	Private data for ste_conn CPD.
 *
 * The cg2900_create_work_item() function creates work item and
 * add it to the work queue.
 */
static void cg2900_create_work_item(work_func_t work_func, struct sk_buff *skb, void *data)
{
	struct cg2900_work_struct *new_work;
	int wq_err = 1;

	new_work = kmalloc(sizeof(*new_work), GFP_ATOMIC);

	if (new_work) {
		new_work->skb = skb;
		new_work->data = data;
		INIT_WORK(&new_work->work, work_func);

		wq_err = queue_work(cg2900_info->wq, &new_work->work);

		if (!wq_err) {
			STE_CONN_ERR("Failed to queue work_struct because it's already in the queue!");
			kfree(new_work);
		}
	} else {
		STE_CONN_ERR("Failed to alloc memory for cg2900_work_struct!");
	}
}

/**
 * cg2900_handle_reset_cmd_complete_evt() - Handle a received HCI Command Complete event for a Reset command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cg2900_handle_reset_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;
	uint8_t status = data[0];

	STE_CONN_INFO("Received Reset complete event with status 0x%X", status);

	if (CLOSING_STATE_RESET == cg2900_info->closing_state) {
		if (HCI_BT_ERROR_NO_ERROR != status) {
			/* Continue in case of error, the chip is going to be shut down anyway. */
			STE_CONN_ERR("Command complete for HciReset received with error 0x%X !", status);
		}

		cg2900_create_work_item(cg2900_work_power_off_chip, NULL, NULL);
		pkt_handled = true;

	}

	return pkt_handled;
}


#ifdef ENABLE_SYS_CLK_OUT
/**
 * cg2900_handle_vs_read_register_cmd_complete_evt() - Handle a received HCI Command Complete event
 * for a VS ReadRegister command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cg2900_handle_vs_read_register_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;

	if (cg2900_info->boot_state == BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_HIGH) {
		uint8_t status = data[0];

		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* Received good confirmation. Start work to continue */
			uint8_t data_out[(sizeof(cg2900_msg_write_register_0x40014004))];
			memcpy(&data_out[0], cg2900_msg_write_register_0x40014004, sizeof(cg2900_msg_write_register_0x40014004));

			STE_CONN_DBG_DATA_CONTENT("Read register 0x40014004 value 0x%02X 0x%02X 0x%02X 0x%02X",
						  data[1], data[2], data[3], data[4]);
			/* Little endian Set bit 12 to value 1 */
			data[2] = data[2] | 0x10;
			STE_CONN_DBG_DATA_CONTENT("Write register 0x40014004 value 0x%02X 0x%02X 0x%02X 0x%02X",
						  data[1], data[2], data[3], data[4]);
			/* Copy value to be written */
			memcpy(&data_out[9], &data[1], 4);
			cg2900_create_and_send_bt_cmd(&data_out[0], sizeof(cg2900_msg_write_register_0x40014004));

		} else {
			STE_CONN_ERR("Command complete for ReadRegister received with error 0x%X", status);
			CG2900_SET_BOOT_STATE(BOOT_STATE_FAILED);
			cg2900_create_work_item(cg2900_work_reset_after_error, NULL, NULL);
		}
		/* We have now handled the packet */
		pkt_handled = true;

	} else if (cg2900_info->boot_state == BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_LOW) {
			uint8_t status = data[0];

		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* Received good confirmation. Start work to continue */
			uint8_t data_out[(sizeof(cg2900_msg_write_register_0x40014004))];
			memcpy(&data_out[0], cg2900_msg_write_register_0x40014004, sizeof(cg2900_msg_write_register_0x40014004));

			STE_CONN_DBG_DATA_CONTENT("Read register 0x40014004 value 0x%02X 0x%02X 0x%02X 0x%02X",
						  data[1], data[2], data[3], data[4]);
			/* Little endian Set bit 12 to value 0 */
			data[2] = data[2] & 0xEF;
			STE_CONN_DBG_DATA_CONTENT("Write register 0x40014004 value 0x%02X 0x%02X 0x%02X 0x%02X",
						  data[1], data[2], data[3], data[4]);
			/* Copy value to be written */
			memcpy(&data_out[9], &data[1], 4);
			cg2900_create_and_send_bt_cmd(&data_out[0], sizeof(cg2900_msg_write_register_0x40014004));

		} else {
			STE_CONN_ERR("Command complete for ReadRegister received with error 0x%X", status);
			CG2900_SET_BOOT_STATE(BOOT_STATE_FAILED);
			cg2900_create_work_item(cg2900_work_reset_after_error, NULL, NULL);
		}
		/* We have now handled the packet */
		pkt_handled = true;
	}
	return pkt_handled;
}

/**
 * cg2900_handle_vs_write_register_cmd_complete_evt() - Handle a received HCI Command Complete event
 * for a VS ReadRegister command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cg2900_handle_vs_write_register_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;

	if (cg2900_info->boot_state == BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_HIGH) {
		uint8_t status = data[0];

		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* Received good confirmation*/
			cg2900_create_and_send_bt_cmd(cg2900_msg_read_register_0x40014004,
						sizeof(cg2900_msg_read_register_0x40014004));
			CG2900_SET_BOOT_STATE(BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_LOW);
		} else {
			STE_CONN_ERR("Command complete for WriteRegister received with error 0x%X", status);
			CG2900_SET_BOOT_STATE(BOOT_STATE_FAILED);
			cg2900_create_work_item(cg2900_work_reset_after_error, NULL, NULL);
		}
		/* We have now handled the packet */
		pkt_handled = true;
	} else 	if (cg2900_info->boot_state == BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_LOW) {
		uint8_t status = data[0];

		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* Received good confirmation
			 * The boot sequence is now finished successfully
			 * Set states and signal to waiting thread.
			 */
			CG2900_SET_BOOT_STATE(BOOT_STATE_READY);
			ste_conn_cpd_chip_startup_finished(0);
		} else {
			STE_CONN_ERR("Command complete for WriteRegister received with error 0x%X", status);
			CG2900_SET_BOOT_STATE(BOOT_STATE_FAILED);
			cg2900_create_work_item(cg2900_work_reset_after_error, NULL, NULL);
		}
		/* We have now handled the packet */
		pkt_handled = true;
	}

	return pkt_handled;
}
#endif /* ENABLE_SYS_CLK_OUT */

/**
 * cg2900_handle_vs_store_in_fs_cmd_complete_evt() - Handle a received HCI Command Complete event
 * for a VS StoreInFS command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cg2900_handle_vs_store_in_fs_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;
	uint8_t status = data[0];

	STE_CONN_INFO("Received Store_in_FS complete event with status 0x%X", status);

	if (cg2900_info->boot_state == BOOT_STATE_SEND_BD_ADDRESS) {
		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* Send HCI SystemReset command to activate patches */
			CG2900_SET_BOOT_STATE(BOOT_STATE_ACTIVATE_PATCHES_AND_SETTINGS);
			cg2900_create_and_send_bt_cmd(cg2900_msg_vs_system_reset_cmd_req,
						      sizeof(cg2900_msg_vs_system_reset_cmd_req));
		} else {
			STE_CONN_ERR("Command complete for StoreInFS received with error 0x%X", status);
			CG2900_SET_BOOT_STATE(BOOT_STATE_FAILED);
			cg2900_create_work_item(cg2900_work_reset_after_error, NULL, NULL);
		}
		/* We have now handled the packet */
		pkt_handled = true;
	}

	return pkt_handled;
}

/**
 * cg2900_handle_vs_write_file_block_cmd_complete_evt() - Handle a received HCI Command Complete event
 * for a VS WriteFileBlock command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cg2900_handle_vs_write_file_block_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;

	if ((cg2900_info->boot_state == BOOT_STATE_DOWNLOAD_PATCH) &&
	    (cg2900_info->boot_download_state == BOOT_DOWNLOAD_STATE_PENDING)) {
		uint8_t status = data[0];
		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* Received good confirmation. Start work to continue. */
			cg2900_create_work_item(cg2900_work_continue_with_file_download, NULL, NULL);
		} else {
			STE_CONN_ERR("Command complete for WriteFileBlock received with error 0x%X", status);
			CG2900_SET_BOOT_DOWNLOAD_STATE(BOOT_DOWNLOAD_STATE_FAILED);
			CG2900_SET_BOOT_STATE(BOOT_STATE_FAILED);
			if (cg2900_info->fw_file) {
				release_firmware(cg2900_info->fw_file);
				cg2900_info->fw_file = NULL;
			}
			cg2900_create_work_item(cg2900_work_reset_after_error, NULL, NULL);
		}
		/* We have now handled the packet */
		pkt_handled = true;
	}

	return pkt_handled;
}

/**
 * cg2900_handle_vs_power_switch_off_cmd_complete_evt() - Handle a received HCI Command Complete event
 * for a VS PowerSwitchOff command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cg2900_handle_vs_power_switch_off_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;

	if (CLOSING_STATE_POWER_SWITCH_OFF == cg2900_info->closing_state) {
		/* We were waiting for this but we don't need to do anything upon receival
		 * except warn for error status
		 */
		uint8_t status = data[0];

		if (HCI_BT_ERROR_NO_ERROR != status) {
			STE_CONN_ERR("Command Complete for PowerSwitchOff received with error 0x%X", status);
		}

		pkt_handled = true;
	}

	return pkt_handled;
}

/**
 * cg2900_handle_vs_system_reset_cmd_complete_evt() - Handle a received HCI Command Complete event for a VS SystemReset command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cg2900_handle_vs_system_reset_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;
	uint8_t status = data[0];

	if (cg2900_info->boot_state == BOOT_STATE_ACTIVATE_PATCHES_AND_SETTINGS) {
#ifdef ENABLE_SYS_CLK_OUT
		STE_CONN_INFO("SYS_CLK_OUT Enabled");
		if (HCI_BT_ERROR_NO_ERROR == status) {
			CG2900_SET_BOOT_STATE(BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_HIGH);
			cg2900_create_and_send_bt_cmd(cg2900_msg_read_register_0x40014004,
						sizeof(cg2900_msg_read_register_0x40014004));
		}
#else
		STE_CONN_INFO("SYS_CLK_OUT Disabled");
		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* The boot sequence is now finished successfully.
			 * Set states and signal to waiting thread.
			 */
			CG2900_SET_BOOT_STATE(BOOT_STATE_READY);
			ste_conn_cpd_chip_startup_finished(0);
		}
#endif
		else {
			STE_CONN_ERR("Received Reset complete event with status 0x%X", status);
			CG2900_SET_BOOT_STATE(BOOT_STATE_FAILED);
			ste_conn_cpd_chip_startup_finished(-EIO);
		}
		pkt_handled = true;
	}
	return pkt_handled;
}

/** Transmit VS Power Switch Off
 * cg2900_work_power_off_chip() - Work item to power off the chip.
 * @work: Reference to work data.
 *
 * The cg2900_work_power_off_chip() function handles transmission of
 * the HCI command vs_power_switch_off command, closes the CCD,
 * and then powers off the chip.
 */
static void cg2900_work_power_off_chip(struct work_struct *work)
{
	struct cg2900_work_struct *current_work = NULL;
	struct sk_buff *skb = NULL;
	uint8_t *h4_header;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct cg2900_work_struct, work);

	/* Get the VS Power Switch Off command to use based on
	 * connected connectivity controller */
	skb = ste_conn_devices_get_power_switch_off_cmd(NULL, NULL);

	/* Transmit the received command. If no command found for the device, just continue */
	if (skb) {
		struct ste_conn_cpd_hci_logger_config *logger_config =
				ste_conn_cpd_get_hci_logger_config();

		STE_CONN_DBG("Got power_switch_off command, add H4 header and transmit");
		 /* Move the data pointer to the H:4 header position and store the H4 header */
		h4_header = skb_push(skb, STE_CONN_SKB_RESERVE);
		*h4_header = CG2900_H4_CHANNEL_BT_CMD;

		CG2900_SET_CLOSING_STATE(CLOSING_STATE_POWER_SWITCH_OFF);

		if (logger_config) {
			ste_conn_cpd_send_to_chip(skb, logger_config->bt_cmd_enable);
		} else {
			ste_conn_cpd_send_to_chip(skb, false);
		}

		/* Mandatory to wait 500ms after the power_switch_off command has been
		 * transmitted, in order to make sure that the controller is ready. */
		schedule_timeout_interruptible(timeval_to_jiffies(&time_500ms));

	} else {
		STE_CONN_ERR("Could not retrieve PowerSwitchOff command");
	}

	CG2900_SET_CLOSING_STATE(CLOSING_STATE_SHUT_DOWN);

	(void)ste_conn_cpd_chip_shutdown_finished(0);

	kfree(current_work);
}

/**
 * cg2900_work_reset_after_error() - Handle reset.
 * @work: Reference to work data.
 *
 * Handle a reset after received command complete event.
 */
static void cg2900_work_reset_after_error(struct work_struct *work)
{
	struct cg2900_work_struct *current_work = NULL;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct cg2900_work_struct, work);

	ste_conn_cpd_chip_startup_finished(-EIO);

	kfree(current_work);
}

/**
 * cg2900_work_load_patch_and_settings() - Start loading patches and settings.
 * @work: Reference to work data.
 */
static void cg2900_work_load_patch_and_settings(struct work_struct *work)
{
	struct cg2900_work_struct *current_work = NULL;
	int err = 0;
	bool file_found;
	const struct firmware *patch_info;
	const struct firmware *settings_info;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct cg2900_work_struct, work);

	/* Check that we are in the right state */
	if (cg2900_info->boot_state == BOOT_STATE_GET_FILES_TO_LOAD) {
		/* Open patch info file. */
		err = request_firmware(&patch_info, CG2900_PATCH_INFO_FILE, cg2900_info->chip_dev.dev);
		if (err) {
			STE_CONN_ERR("Couldn't get patch info file");
			goto error_handling;
		}

		/* Now we have the patch info file. See if we can find the right patch file as well */
		file_found = cg2900_get_file_to_load(patch_info, &(cg2900_info->patch_file_name));

		/* Now we are finished with the patch info file */
		release_firmware(patch_info);

		if (!file_found) {
			STE_CONN_ERR("Couldn't find patch file! Major error!");
			goto error_handling;
		}

		/* Open settings info file. */
		err = request_firmware(&settings_info, CG2900_FACTORY_SETTINGS_INFO_FILE, cg2900_info->chip_dev.dev);
		if (err) {
			STE_CONN_ERR("Couldn't get settings info file");
			goto error_handling;
		}

		/* Now we have the settings info file. See if we can find the right settings file as well */
		file_found = cg2900_get_file_to_load(settings_info, &(cg2900_info->settings_file_name));

		/* Now we are finished with the patch info file */
		release_firmware(settings_info);

		if (!file_found) {
			STE_CONN_ERR("Couldn't find settings file! Major error!");
			goto error_handling;
		}

		/* We now all info needed */
		CG2900_SET_BOOT_STATE(BOOT_STATE_DOWNLOAD_PATCH);
		CG2900_SET_BOOT_DOWNLOAD_STATE(BOOT_DOWNLOAD_STATE_PENDING);
		CG2900_SET_BOOT_FILE_LOAD_STATE(BOOT_FILE_LOAD_STATE_LOAD_PATCH);
		cg2900_info->chunk_id = 0;
		cg2900_info->fw_file_rd_offset = 0;
		cg2900_info->fw_file = NULL;

		/* OK. Now it is time to download the patches */
		err = request_firmware(&(cg2900_info->fw_file), cg2900_info->patch_file_name, cg2900_info->chip_dev.dev);
		if (err < 0) {
			STE_CONN_ERR("Couldn't get patch file");
			goto error_handling;
		}
		cg2900_send_patch_file();
	}
	goto finished;

error_handling:
	CG2900_SET_BOOT_STATE(BOOT_STATE_FAILED);
	ste_conn_cpd_chip_startup_finished(-EIO);
finished:
	kfree(current_work);
}

/**
 * cg2900_work_continue_with_file_download() - A file block has been written.
 * @work: Reference to work data.
 *
 * Handle a received HCI VS Write File Block Complete event.
 * Normally this means continue to send files to the controller.
 */
static void cg2900_work_continue_with_file_download(struct work_struct *work)
{
	struct cg2900_work_struct *current_work = NULL;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct cg2900_work_struct, work);

	/* Continue to send patches or settings to the controller */
	if (cg2900_info->boot_file_load_state == BOOT_FILE_LOAD_STATE_LOAD_PATCH) {
		cg2900_send_patch_file();
	} else if (cg2900_info->boot_file_load_state == BOOT_FILE_LOAD_STATE_LOAD_STATIC_SETTINGS) {
		cg2900_send_settings_file();
	} else {
		STE_CONN_INFO("No more files to load");
	}

	kfree(current_work);
}

/**
 * cg2900_get_file_to_load() - Parse info file and find correct target file.
 * @fw:              Firmware structure containing file data.
 * @file_name: (out) Pointer to name of requested file.
 *
 * Returns:
 *   True,  if target file was found,
 *   False, otherwise.
 */
static bool cg2900_get_file_to_load(const struct firmware *fw, char **file_name)
{
	char *line_buffer;
	char *curr_file_buffer;
	int bytes_left_to_parse = fw->size;
	int bytes_read = 0;
	bool file_found = false;

	curr_file_buffer = (char *)&(fw->data[0]);

	line_buffer = kzalloc(CG2900_LINE_BUFFER_LENGTH, GFP_ATOMIC);

	if (line_buffer) {
		while (!file_found) {
			/* Get one line of text from the file to parse */
			curr_file_buffer = cg2900_get_one_line_of_text(line_buffer,
								    min(CG2900_LINE_BUFFER_LENGTH,
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
				    && hci_rev == cg2900_info->chip_dev.chip.hci_revision
				    && lmp_sub == cg2900_info->chip_dev.chip.hci_sub_version) {
					STE_CONN_DBG( \
					"File name = %s HCI Revision = 0x%X LMP PAL Subversion = 0x%X", \
					*file_name, hci_rev, lmp_sub);

					/* Name has already been stored above. Nothing more to do */
					file_found = true;
				} else {
					/* Zero the name buffer so it is clear to next read */
					memset(*file_name, 0x00, CG2900_FILENAME_MAX + 1);
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
 * cg2900_get_one_line_of_text()- Replacement function for stdio function fgets.
 * @wr_buffer:         Buffer to copy text to.
 * @max_nbr_of_bytes: Max number of bytes to read, i.e. size of rd_buffer.
 * @rd_buffer:        Data to parse.
 * @bytes_copied:     Number of bytes copied to wr_buffer.
 *
 * The cg2900_get_one_line_of_text() function extracts one line of text from input file.
 *
 * Returns:
 *   Pointer to next data to read.
 */
static char *cg2900_get_one_line_of_text(char *wr_buffer, int max_nbr_of_bytes,
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
 * cg2900_read_and_send_file_part() - Transmit a part of the supplied file to the controller
 *
 * The cg2900_read_and_send_file_part() function transmit a part of the supplied file to the controller.
 * If nothing more to read, set the correct states.
 */
static void cg2900_read_and_send_file_part(void)
{
	int bytes_to_copy;

	/* Calculate number of bytes to copy;
	 * either max bytes for HCI packet or number of bytes left in file
	 */
	bytes_to_copy = min((int)HCI_BT_SEND_FILE_MAX_CHUNK_SIZE,
			    (int)(cg2900_info->fw_file->size - cg2900_info->fw_file_rd_offset));

	if (bytes_to_copy > 0) {
		struct sk_buff *skb;
		struct ste_conn_cpd_hci_logger_config *logger_config = ste_conn_cpd_get_hci_logger_config();

		/* There are bytes to transmit. Allocate a sk_buffer. */
		skb = alloc_skb(sizeof(cg2900_msg_vs_write_file_block_cmd_req) +
				HCI_BT_SEND_FILE_MAX_CHUNK_SIZE +
				CG2900_FILE_CHUNK_ID_SIZE, GFP_ATOMIC);
		if (!skb) {
			STE_CONN_ERR("Couldn't allocate sk_buffer");
			CG2900_SET_BOOT_STATE(BOOT_STATE_FAILED);
			ste_conn_cpd_chip_startup_finished(-EIO);
			return;
		}

		/* Copy the data from the HCI template */
		memcpy(skb_put(skb, sizeof(cg2900_msg_vs_write_file_block_cmd_req)),
		       cg2900_msg_vs_write_file_block_cmd_req,
		       sizeof(cg2900_msg_vs_write_file_block_cmd_req));
		skb->data[0] = CG2900_H4_CHANNEL_BT_CMD;

		/* Store the chunk ID */
		skb->data[CG2900_VS_SEND_FILE_START_OFFSET_IN_CMD] = cg2900_info->chunk_id;
		skb_put(skb, 1);

		/* Copy the data from offset position and store the length */
		memcpy(skb_put(skb, bytes_to_copy),
		       &(cg2900_info->fw_file->data[cg2900_info->fw_file_rd_offset]),
		       bytes_to_copy);
		skb->data[CG2900_BT_CMD_LENGTH_POSITION] = bytes_to_copy + CG2900_FILE_CHUNK_ID_SIZE;

		/* Increase offset with number of bytes copied */
		cg2900_info->fw_file_rd_offset += bytes_to_copy;

		if (logger_config) {
			ste_conn_cpd_send_to_chip(skb, logger_config->bt_cmd_enable);
		} else {
			ste_conn_cpd_send_to_chip(skb, false);
		}
		cg2900_info->chunk_id++;
	} else {
		/* Nothing more to read in file. */
		CG2900_SET_BOOT_DOWNLOAD_STATE(BOOT_DOWNLOAD_STATE_SUCCESS);
		cg2900_info->chunk_id = 0;
		cg2900_info->fw_file_rd_offset = 0;
	}
}

/**
 * cg2900_send_patch_file - Transmit patch file.
 *
 * The cg2900_send_patch_file() function transmit patch file. The file is
 * read in parts to fit in HCI packets. When the complete file is transmitted,
 * the file is closed. When finished, continue with settings file.
 */
static void cg2900_send_patch_file(void)
{
	int err = 0;

	/* Transmit a part of the supplied file to the controller.
	 * When nothing more to read, continue to close the patch file. */
	cg2900_read_and_send_file_part();

	if (cg2900_info->boot_download_state == BOOT_DOWNLOAD_STATE_SUCCESS) {
		/* Patch file finished. Release used resources */
		STE_CONN_DBG("Patch file finished, release used resources");
		if (cg2900_info->fw_file) {
			release_firmware(cg2900_info->fw_file);
			cg2900_info->fw_file = NULL;
		}
		err = request_firmware(&(cg2900_info->fw_file), cg2900_info->settings_file_name, cg2900_info->chip_dev.dev);
		if (err < 0) {
			STE_CONN_ERR("Couldn't get settings file (%d)", err);
			goto error_handling;
		}
		/* Now send the settings file */
		CG2900_SET_BOOT_FILE_LOAD_STATE(BOOT_FILE_LOAD_STATE_LOAD_STATIC_SETTINGS);
		CG2900_SET_BOOT_DOWNLOAD_STATE(BOOT_DOWNLOAD_STATE_PENDING);
		cg2900_send_settings_file();
	}
	return;

error_handling:
	CG2900_SET_BOOT_STATE(BOOT_STATE_FAILED);
	ste_conn_cpd_chip_startup_finished(err);
}

/**
 * cg2900_send_settings_file() - Transmit settings file.
 *
 * The cg2900_send_settings_file() function transmit settings file.
 * The file is read in parts to fit in HCI packets. When finished,
 * close the settings file and send HCI reset to activate settings and patches.
 */
static void cg2900_send_settings_file(void)
{
	/* Transmit a file part */
	cg2900_read_and_send_file_part();

	if (cg2900_info->boot_download_state == BOOT_DOWNLOAD_STATE_SUCCESS) {

		/* Settings file finished. Release used resources */
		STE_CONN_DBG("Settings file finished, release used resources");
		if (cg2900_info->fw_file) {
			release_firmware(cg2900_info->fw_file);
			cg2900_info->fw_file = NULL;
		}

		CG2900_SET_BOOT_FILE_LOAD_STATE(BOOT_FILE_LOAD_STATE_NO_MORE_FILES);

		/* Create and send HCI VS Store In FS command with bd address. */
		cg2900_send_bd_address();
	}
}

/**
 * cg2900_create_and_send_bt_cmd() - Copy and send sk_buffer.
 * @data:   Data to send.
 * @length: Length in bytes of data.
 *
 * The cg2900_create_and_send_bt_cmd() function allocate sk_buffer, copy supplied data to it,
 * and send the sk_buffer to CCD. Note that the data must contain the H:4 header.
 */
static void cg2900_create_and_send_bt_cmd(const uint8_t *data, int length)
{
	struct sk_buff *skb;
	struct ste_conn_cpd_hci_logger_config *logger_config;

	skb = alloc_skb(length, GFP_ATOMIC);
	if (skb) {
		int err = 0;

		memcpy(skb_put(skb, length), data, length);
		skb->data[0] = CG2900_H4_CHANNEL_BT_CMD;

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
 * cg2900_send_bd_address() - Send HCI VS command with BD address to the chip.
 */
static void cg2900_send_bd_address(void)
{
	uint8_t tmp[sizeof(cg2900_msg_vs_store_in_fs_cmd_req) + BT_BDADDR_SIZE];

	/* Send vs_store_in_fs_cmd with BD address. */
	memcpy(tmp, cg2900_msg_vs_store_in_fs_cmd_req,
		sizeof(cg2900_msg_vs_store_in_fs_cmd_req));

	tmp[HCI_BT_CMD_PARAM_LEN_POS] = CG2900_VS_STORE_IN_FS_USR_ID_SIZE +
		CG2900_VS_STORE_IN_FS_PARAM_LEN_SIZE + BT_BDADDR_SIZE;
	tmp[CG2900_VS_STORE_IN_FS_USR_ID_POS] = CG2900_VS_STORE_IN_FS_USR_ID_BD_ADDR;
	tmp[CG2900_VS_STORE_IN_FS_PARAM_LEN_POS] = BT_BDADDR_SIZE;

	/* Now copy the bd address received from user space control app. */
	memcpy(&(tmp[CG2900_VS_STORE_IN_FS_PARAM_POS]), ste_conn_bd_address, sizeof(ste_conn_bd_address));

	CG2900_SET_BOOT_STATE(BOOT_STATE_SEND_BD_ADDRESS);

	cg2900_create_and_send_bt_cmd(tmp, sizeof(tmp));
}

/**
 * cg2900_transmit_skb_to_ccd_with_flow_ctrl_bt_cmd() - Send the BT skb to the CCD if it is allowed or queue it.
 * @skb:        Data packet.
 * @dev: pointer to ste_conn_device struct.
 *
 * The cpd_transmit_skb_to_ccd_with_flow_ctrl_bt() function checks if there are tickets available and if so
 * transmits buffer to CCD. Otherwise the skb and user name is stored in a list for later sending.
 * If enabled, copy the transmitted data to the HCI logger as well.
 */
static void cg2900_transmit_skb_to_ccd_with_flow_ctrl_bt_cmd(struct sk_buff *skb,
		struct ste_conn_device *dev)
{
	/* Because there are more users of some H4 channels (currently audio application
	 * for BT command and FM channel) we need to have an internal HCI command flow control
	 * in ste_conn driver. So check here how many tickets we have and store skb in a queue
	 * if there are no tickets left. The skb will be sent later when we get more ticket(s). */
	spin_lock_bh(&(cg2900_info->tx_bt_lock));

	if ((cg2900_info->tx_nr_pkts_allowed_bt) > 0) {
		(cg2900_info->tx_nr_pkts_allowed_bt)--;
		STE_CONN_DBG("New tx_nr_pkts_allowed_bt = %d", cg2900_info->tx_nr_pkts_allowed_bt);

		/* If it's command from audio app store the OpCode,
		 * it'll be used later to decide where to dispatch command complete event. */
		if (ste_conn_cpd_get_bt_audio_dev() == dev) {
			cg2900_info->hci_audio_cmd_opcode_bt = (uint16_t)(skb->data[1] | (skb->data[2] << 8));
			STE_CONN_INFO("Sending cmd from audio driver, saving OpCode = 0x%x",
					cg2900_info->hci_audio_cmd_opcode_bt);
		}

		ste_conn_cpd_send_to_chip(skb, dev->logger_enabled);
	} else {
		struct tx_list_item *item = kzalloc(sizeof(*item), GFP_ATOMIC);
		STE_CONN_DBG("Not allowed to send cmd to CCD, storing in tx queue.");
		if (item) {
			item->dev = dev;
			item->skb = skb;
			list_add_tail(&item->list, &cg2900_info->tx_list_bt);
		} else {
			STE_CONN_ERR("Failed to alloc memory!");
		}
	}
	spin_unlock_bh(&(cg2900_info->tx_bt_lock));
}

/**
 * cg2900_fm_reset_flow_ctrl - Resets outstanding commands and clear fm tx list and set cpd fm mode to idle
 */
static void cg2900_fm_reset_flow_ctrl()
{
	struct list_head *cursor, *next;
	struct tx_list_item *tmp;

	STE_CONN_INFO("cg2900_fm_reset_flow_ctrl");

	list_for_each_safe(cursor, next, &cg2900_info->tx_list_fm) {
		tmp = list_entry(cursor, struct tx_list_item, list);
		list_del(cursor);
		kfree_skb(tmp->skb);
		kfree(tmp);
	}
	/* Reset the fm_cmd_id. */
	cg2900_info->hci_audio_fm_cmd_id = CG2900_FM_CMD_NONE;
	cg2900_info->hci_fm_cmd_func = CG2900_FM_CMD_PARAMETERS_NONE;
	STE_CONN_DBG("FM cmd outstanding cmd func 0x%x", cg2900_info->hci_fm_cmd_func);
	STE_CONN_DBG("FM cmd Audio outstanding cmd id 0x%x", cg2900_info->hci_audio_fm_cmd_id);

	cg2900_info->fm_radio_mode = CG2900_FM_RADIO_MODE_IDLE;
}


/**
 * cg2900_fm_parse_cmd - Parses a fm command packet
 * @data:	Fm command packet.
 * @cmd_func:	FM legacy command function
 * @cmd_id:	FM legacy command id
 */
static void cg2900_fm_parse_cmd(uint8_t *data, uint8_t *cmd_func, uint16_t *cmd_id)
{
	*cmd_func = CG2900_FM_CMD_PARAMETERS_NONE;
	*cmd_id = CG2900_FM_CMD_NONE;

	if (data[CG2900_FM_GEN_OPCODE_OFFSET_CMD] == CG2900_FM_GEN_OPCODE_LEGACY_API) {
		*cmd_func = data[4];
		STE_CONN_DBG("fm legacy cmd func 0x%x", *cmd_func);
		if (*cmd_func == CG2900_FM_CMD_PARAMETERS_WRITECOMMAND) {
			*cmd_id = (((data[5] | (data[6] << 8)) & 0xFFF8) >> 3);
			STE_CONN_DBG("fm legacy cmd id 0x%x", *cmd_id);
		}
	} else {
		STE_CONN_ERR("Not an fm legacy command 0x%x", data[CG2900_FM_GEN_OPCODE_OFFSET_CMD]);
	}
}


/**
 * cg2900_fm_parse_event - Parses a fm event packet
 * @data:	In:Fm event packet.
 * @event	Out: FM event
 * @cmd_func:	Out: FM legacy command function
 * @cmd_id:	Out: FM legacy command id
 * @intr_val:	Out: FM interrupt value
 */
static void cg2900_fm_parse_event(uint8_t *data, uint8_t *event, uint8_t *cmd_func, uint16_t *cmd_id, uint16_t *intr_val)
{
	*cmd_func = CG2900_FM_CMD_PARAMETERS_NONE;
	*cmd_id = CG2900_FM_CMD_NONE;
	*intr_val = 0;
	*event = CG2900_FM_EVENT_UNKNOWN;

	/* Command complete */
	if (data[CG2900_FM_GEN_OPCODE_OFFSET_CMD_CMPL] == CG2900_FM_GEN_OPCODE_LEGACY_API) {
		*event = CG2900_FM_EVENT_CMD_COMPLETE;
		*cmd_func = data[6];
		STE_CONN_DBG("fm legacy cmd func 0x%x", *cmd_func);
		if (*cmd_func == CG2900_FM_CMD_PARAMETERS_WRITECOMMAND) {
			*cmd_id = (((data[7] | (data[8] << 8)) & 0xFFF8) >> 3);
			STE_CONN_DBG("fm legacy cmd id 0x%x", *cmd_id);
		}
	}

	/* Interrupt */
	else if (data[CG2900_FM_GEN_OPCODE_OFFSET_INTERRUPT] == CG2900_FM_GEN_OPCODE_LEGACY_API) {
		*event = CG2900_FM_EVENT_INTERRUPT;
		*intr_val = (data[4] | (data[5] << 8));
		STE_CONN_DBG("fm legacy intr bit field 0x%x", *intr_val);
	} else {
		STE_CONN_ERR("Not an fm legacy command 0x%x", data[CG2900_FM_GEN_OPCODE_OFFSET_CMD]);
	}
}

/**
 * cg2900_fm_update_mode - Parses a FM command packet and updates the FM mode state machine
 * @data:		In:Fm command packet.
 */
static void cg2900_fm_update_mode(uint8_t *data)
{
	uint8_t cmd_func;
	uint16_t cmd_id;

	cg2900_fm_parse_cmd(data, &cmd_func, &cmd_id);

	if (cmd_func == CG2900_FM_CMD_PARAMETERS_WRITECOMMAND) {
		if (cmd_id == CG2900_FM_DO_GEN_GOTOMODE) {
			cg2900_info->fm_radio_mode = data[6];
			STE_CONN_INFO("FM Radio mode changed to 0x%x", cg2900_info->fm_radio_mode);
		}
	}
}

/**
 * cg2900_transmit_skb_to_ccd_with_flow_ctrl_fm() - Send the FM skb to the CCD if it is allowed or queue it.
 * @skb: Data packet.
 * @dev: pointer to ste_conn_device struct.
 *
 * The cg2900_transmit_skb_to_ccd_with_flow_ctrl_fm() function checks if chip is available and if so
 * transmits buffer to CCD. Otherwise the skb and user name is stored in a list for later sending.
 * Also it updates the FM radio mode if it's FM GOTOMODE command, this is needed to
 * know how to handle some FM do-commands complete events.
 * If enabled, copy the transmitted data to the HCI logger as well.
 */
static void cg2900_transmit_skb_to_ccd_with_flow_ctrl_fm(struct sk_buff *skb,
							 struct ste_conn_device *dev)
{
	uint8_t cmd_func = CG2900_FM_CMD_PARAMETERS_NONE;
	uint16_t cmd_id = CG2900_FM_CMD_NONE;

	cg2900_fm_parse_cmd(&(skb->data[0]), &cmd_func, &cmd_id);

	/* If there is an FM IP disable or reset send command and also reset the flow control and audio user */
	if (cmd_func == CG2900_FM_CMD_PARAMETERS_DISABLE ||
	    cmd_func == CG2900_FM_CMD_PARAMETERS_RESET) {
		spin_lock(&cg2900_info->tx_fm_lock);
		cg2900_fm_reset_flow_ctrl();
		spin_unlock(&cg2900_info->tx_fm_lock);
		ste_conn_cpd_send_to_chip(skb, dev->logger_enabled);
		return;
	}

	/* If there is a FM user and no FM audio user command pending
	 * Then just send fm command it is up to the user of the FM channel to handle it own flow controll.*/
	spin_lock_bh(&cg2900_info->tx_fm_lock);
	if (ste_conn_cpd_get_fm_radio_dev() == dev &&
	    cg2900_info->hci_audio_fm_cmd_id == CG2900_FM_CMD_NONE) {
		cg2900_info->hci_fm_cmd_func = cmd_func;
		STE_CONN_DBG("FM cmd outstanding cmd func 0x%x", cg2900_info->hci_fm_cmd_func);
		/*If a goto mode command update fm mode*/
		cg2900_fm_update_mode(&(skb->data[0]));
		ste_conn_cpd_send_to_chip(skb, dev->logger_enabled);

	} else if (ste_conn_cpd_get_fm_audio_dev() == dev &&
		   cg2900_info->hci_fm_cmd_func == CG2900_FM_CMD_PARAMETERS_NONE &&
		   cg2900_info->hci_audio_fm_cmd_id == CG2900_FM_CMD_NONE) {
		/* If it's command from fm audio user store the command id.
		 * It'll be used later to decide where to dispatch command complete event. */
		cg2900_info->hci_audio_fm_cmd_id = cmd_id;
		STE_CONN_DBG("FM cmd Audio outstanding cmd id 0x%x", cg2900_info->hci_audio_fm_cmd_id);
		ste_conn_cpd_send_to_chip(skb, dev->logger_enabled);

	} else {
		struct tx_list_item *item = kzalloc(sizeof(*item), GFP_ATOMIC);
		STE_CONN_DBG("Not allowed to send cmd to CCD, storing in tx queue.");
		if (item) {
			item->dev = dev;
			item->skb = skb;
			list_add_tail(&item->list, &cg2900_info->tx_list_fm);
		} else {
			STE_CONN_ERR("Failed to alloc memory!");
		}
	}
	spin_unlock_bh(&(cg2900_info->tx_fm_lock));
}


/**
 * cg2900_transmit_skb_from_tx_queue_bt() - Check/update flow control info and transmit skb from
 * the BT tx_queue to CCD if allowed.
 *
 * The cg2900_transmit_skb_from_tx_queue_bt() function checks if there are tickets available and commands waiting in the tx queue
 * and if so transmits them to CCD.
 */
static void cg2900_transmit_skb_from_tx_queue_bt(void)
{
	struct list_head *cursor, *next;
	struct tx_list_item *tmp;
	struct ste_conn_device *dev;
	struct sk_buff *skb;

	STE_CONN_INFO("cg2900_transmit_skb_from_tx_queue_bt");

	list_for_each_safe(cursor, next, &(cg2900_info->tx_list_bt)) {
		tmp = list_entry(cursor, struct tx_list_item, list);
		skb = tmp->skb;
		dev = tmp->dev;

		if (dev) {
			if ((cg2900_info->tx_nr_pkts_allowed_bt) > 0) {
				(cg2900_info->tx_nr_pkts_allowed_bt)--;
				STE_CONN_DBG("New tx_nr_pkts_allowed_bt = %d", cg2900_info->tx_nr_pkts_allowed_bt);

				/* If it's command from audio application, store the OpCode,
				 * it'll be used later to decide where to dispatch the command complete event. */
				if (ste_conn_cpd_get_bt_audio_dev() == dev) {
					cg2900_info->hci_audio_cmd_opcode_bt = (uint16_t)(skb->data[1] | (skb->data[2] << 8));
					STE_CONN_INFO("Sending cmd from audio driver, saving OpCode = 0x%x",
						      cg2900_info->hci_audio_cmd_opcode_bt);
				}

				ste_conn_cpd_send_to_chip(skb, dev->logger_enabled);
				list_del(cursor);
				kfree(tmp);
			} else {
				/* If no more packets allowed just return, we'll get back here after
				 * next command complete/command status event. */
				return;
			}
		} else {
			STE_CONN_ERR("H4 user not found!");
			kfree_skb(skb);
		}
	}
}

/**
 * cg2900_transmit_skb_from_tx_queue_fm() - Check/update flow control info and transmit skb from
 * the FM tx_queue to CCD if allowed.
 */
static void cg2900_transmit_skb_from_tx_queue_fm(void)
{
	struct list_head *cursor, *next;
	struct tx_list_item *tmp;
	struct ste_conn_device *dev;
	struct sk_buff *skb;

	STE_CONN_INFO("cg2900_transmit_skb_from_tx_queue_fm");

	list_for_each_safe(cursor, next, &(cg2900_info->tx_list_fm)) {
		tmp = list_entry(cursor, struct tx_list_item, list);
		skb = tmp->skb;
		dev = tmp->dev;

		if (dev) {
			if (cg2900_info->hci_audio_fm_cmd_id == CG2900_FM_CMD_NONE &&
			    cg2900_info->hci_fm_cmd_func == CG2900_FM_CMD_PARAMETERS_NONE) {
				uint16_t cmd_id;
				uint8_t cmd_func;

				cg2900_fm_parse_cmd(&(skb->data[0]), &cmd_func, &cmd_id);

				/* Store the FM command function ,
				 * it'll be used later to decide where to dispatch the command complete event. */
				if (ste_conn_cpd_get_fm_audio_dev() == dev) {
					cg2900_info->hci_audio_fm_cmd_id = cmd_id;
					STE_CONN_DBG("FM cmd Audio outstanding cmd id 0x%x", cg2900_info->hci_audio_fm_cmd_id);
				}
				if (ste_conn_cpd_get_fm_radio_dev() == dev) {
					cg2900_info->hci_fm_cmd_func = cmd_func;
					cg2900_fm_update_mode(&(skb->data[0]));
					STE_CONN_DBG("FM cmd outstanding cmd func 0x%x", cg2900_info->hci_fm_cmd_func);
				}
				ste_conn_cpd_send_to_chip(skb, dev->logger_enabled);
				list_del(cursor);
				kfree(tmp);
			}
		} else {
			STE_CONN_ERR("H4 user not found!");
			kfree_skb(skb);
		}
	}
}

/**
 * cg2900_init() - Initialize module.
 *
 * The ste_conn_init() function initialize the CG2900 driver,
 * then register to the CPD.
 *
 * Returns:
 *   0 if success.
 *   -ENOMEM for failed alloc or structure creation.
 *   Error codes generated by ste_conn_cpd_register_chip_driver.
 */
static int __init cg2900_init(void)
{
	int err = 0;

	STE_CONN_INFO("cg2900_init");

	cg2900_info = kzalloc(sizeof(*cg2900_info), GFP_ATOMIC);
	if (!cg2900_info) {
		STE_CONN_ERR("Couldn't allocate cg2900_info");
		err = -ENOMEM;
		goto finished;
	}

	/* Initialize linked lists for HCI BT and FM commands
	 * that can't be sent due to internal ste_conn flow control. */
	INIT_LIST_HEAD(&(cg2900_info->tx_list_bt));
	INIT_LIST_HEAD(&(cg2900_info->tx_list_fm));

	/* Initialize the spin locks */
	spin_lock_init(&(cg2900_info->tx_bt_lock));
	spin_lock_init(&(cg2900_info->tx_fm_lock));

	cg2900_info->tx_nr_pkts_allowed_bt = 1;
	cg2900_info->hci_audio_cmd_opcode_bt = CG2900_BT_OPCODE_NONE;
	cg2900_info->hci_audio_fm_cmd_id = CG2900_FM_CMD_NONE;
	cg2900_info->hci_fm_cmd_func = CG2900_FM_CMD_PARAMETERS_NONE;
	cg2900_info->fm_radio_mode = CG2900_FM_RADIO_MODE_IDLE;

	cg2900_info->wq = create_singlethread_workqueue(CG2900_WQ_NAME);
	if (!cg2900_info->wq) {
		STE_CONN_ERR("Could not create workqueue");
		err = -ENOMEM;
		goto err_handling_free_info;
	}

	/* Allocate file names that will be used, deallocated in ste_conn_cpd_exit */
	cg2900_info->patch_file_name = kzalloc(CG2900_FILENAME_MAX + 1, GFP_ATOMIC);
	if (!cg2900_info->patch_file_name) {
		STE_CONN_ERR("Couldn't allocate name buffer for patch file.");
		err = -ENOMEM;
		goto err_handling_destroy_wq;
	}
		/* Allocate file names that will be used, deallocaded in ste_conn_cpd_exit  */
	cg2900_info->settings_file_name = kzalloc(CG2900_FILENAME_MAX + 1, GFP_ATOMIC);
	if (!cg2900_info->settings_file_name) {
		STE_CONN_ERR("Couldn't allocate name buffers settings file.");
		err = -ENOMEM;
		goto err_handling_free_patch_name;
	}

	err = ste_conn_cpd_register_chip_driver(&cg2900_id_callbacks);
	if (err) {
		STE_CONN_ERR("Couldn't register chip driver (%d)", err);
		goto err_handling_free_settings_name;
	}

	goto finished;

err_handling_free_settings_name:
	kfree(cg2900_info->settings_file_name);
err_handling_free_patch_name:
	kfree(cg2900_info->patch_file_name);
err_handling_destroy_wq:
	destroy_workqueue(cg2900_info->wq);
err_handling_free_info:
	kfree(cg2900_info);
	cg2900_info = NULL;
finished:
	return err;
}

/**
 * cg2900_exit() - Remove module.
 */
static void __exit cg2900_exit(void)
{
	STE_CONN_INFO("cg2900_exit");

	if (cg2900_info) {
		kfree(cg2900_info->settings_file_name);
		kfree(cg2900_info->patch_file_name);
		destroy_workqueue(cg2900_info->wq);
		kfree(cg2900_info);
		cg2900_info = NULL;
	}
}

module_init(cg2900_init);
module_exit(cg2900_exit);

MODULE_AUTHOR("Par-Gunnar Hjalmdahl ST-Ericsson");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Linux CG2900 Connectivity Device Driver");
