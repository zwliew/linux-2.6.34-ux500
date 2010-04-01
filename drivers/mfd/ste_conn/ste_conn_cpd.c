/*
 * file ste_conn_cpd.c
 *
 * Copyright (C) ST-Ericsson AB 2010
 *
 * Linux Bluetooth HCI H:4 Driver for ST-Ericsson connectivity controller.
 * License terms: GNU General Public License (GPL), version 2
 *
 * Authors:
 * Par-Gunnar Hjalmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Henrik Possung (henrik.possung@stericsson.com) for ST-Ericsson.
 * Josef Kindberg (josef.kindberg@stericsson.com) for ST-Ericsson.
 * Dariusz Szymszak (dariusz.xd.szymczak@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson.
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

/* #define STE_CONN_DEBUG_LEVEL 20 */ /* Remove comment to enable higher debug */
#include "ste_conn_debug.h"

#define VERSION "1.0"

/* Reserve 1 byte for the HCI H:4 header */
#define STE_CONN_SKB_RESERVE  1

/*STE_CONN_SYS_CLK_OUT - Enable system clock out on chip.*/
#define ENABLE_SYS_CLK_OUT

/* Bluetooth Opcode Group Field */
#define HCI_BT_OGF_LINK_CTRL    0x01
#define HCI_BT_OGF_LINK_POLICY  0x02
#define HCI_BT_OGF_CTRL_BB      0x03
#define HCI_BT_OGF_LINK_INFO    0x04
#define HCI_BT_OGF_LINK_STATUS  0x05
#define HCI_BT_OGF_LINK_TESTING 0x06
#define HCI_BT_OGF_VS           0x3F

/* Bluetooth Opcode Command Field */
#define HCI_BT_OCF_READ_LOCAL_VERSION_INFO	0x0001
#define HCI_BT_OCF_RESET					0x0003
#define HCI_BT_OCF_VS_STORE_IN_FS			0x0022
#define HCI_BT_OCF_VS_WRITE_FILE_BLOCK		0x002E
#define HCI_BT_OCF_VS_POWER_SWITCH_OFF		0x0140
#define HCI_BT_OCF_VS_SYSTEM_RESET			0x0312
#ifdef ENABLE_SYS_CLK_OUT
#define HCI_BT_OCF_VS_READ_REGISTER	0x0104
#define HCI_BT_OCF_VS_WRITE_REGISTER	0x0103
#endif


#define HCI_BT_EVT_CMD_COMPLETE			0x0E
#define HCI_BT_EVT_CMD_COMPL_NR_OF_PKTS_POS	0x02
#define HCI_BT_EVT_CMD_STATUS			0x0F
#define HCI_BT_EVT_CMD_STATUS_NR_OF_PKTS_POS	0x03

/* FM do-command identifiers. */
#define CPD_FM_DO_AIP_FADE_START 			0x0046
#define CPD_FM_DO_AUP_BT_FADE_START 		0x01C2
#define CPD_FM_DO_AUP_EXT_FADE_START 		0x0102
#define CPD_FM_DO_AUP_FADE_START 			0x00A2
#define CPD_FM_DO_FMR_SETANTENNA 			0x0663
#define CPD_FM_DO_FMR_SP_AFSWITCH_START 	0x04A3
#define CPD_FM_DO_FMR_SP_AFUPDATE_START 	0x0463
#define CPD_FM_DO_FMR_SP_BLOCKSCAN_START 	0x0683
#define CPD_FM_DO_FMR_SP_PRESETPI_START 	0x0443
#define CPD_FM_DO_FMR_SP_SCAN_START 		0x0403
#define CPD_FM_DO_FMR_SP_SEARCH_START 		0x03E3
#define CPD_FM_DO_FMR_SP_SEARCHPI_START 	0x0703
#define CPD_FM_DO_FMR_SP_TUNE_SETCHANNEL 	0x03C3
#define CPD_FM_DO_FMR_SP_TUNE_STEPCHANNEL 	0x04C3
#define CPD_FM_DO_FMT_PA_SETCONTROL 		0x01A4
#define CPD_FM_DO_FMT_PA_SETMODE 			0x01E4
#define CPD_FM_DO_FMT_SP_TUNE_SETCHANNEL 	0x0064
#define CPD_FM_DO_GEN_ANTENNACHECK_START 	0x02A1
#define CPD_FM_DO_GEN_GOTOMODE 				0x0041
#define CPD_FM_DO_GEN_POWERSUPPLY_SETMODE 	0x0221
#define CPD_FM_DO_GEN_SELECTREFERENCECLOCK	0x0201
#define CPD_FM_DO_GEN_SETPROCESSINGCLOCK 	0x0241
#define CPD_FM_DO_GEN_SETREFERENCECLOCKPLL 	0x01A1
#define CPD_FM_DO_TST_TX_RAMP_START 		0x0147

/* FM interrupt values for do-command related interrupts. */
#define CPD_FM_IRPT_OPERATION_SUCCEEDED	0x0000
#define CPD_FM_IRPT_OPERATION_FAILED	0x0001
/*#define CPD_FM_IRPT_FIQ					0x????*/

/* BT VS OpCodes */
#define CPD_BT_VS_BT_ENABLE_OPCODE	0xFF10

#define HCI_BT_ERROR_NO_ERROR 0x00

#define CPD_MAKE_FIRST_BYTE_IN_CMD(__ocf) ((uint8_t)(__ocf & 0x00FF))
#define CPD_MAKE_SECOND_BYTE_IN_CMD(__ogf, __ocf) ((uint8_t)(__ogf << 2) | ((__ocf >> 8) & 0x0003))

#define STE_CONN_BT_PATCH_INFO_FILE             "ste_conn_patch_info.fw"
#define STE_CONN_BT_FACTORY_SETTINGS_INFO_FILE  "ste_conn_settings_info.fw"
#define STE_CONN_LINE_BUFFER_LENGTH             128
#define STE_CONN_FILENAME_MAX                   128

/* Maximum file chunk size which can be sent in one HCI packet */
#define CPD_SEND_FILE_MAX_CHUNK_SIZE          254
/* Size of file chunk ID */
#define CPD_FILE_CHUNK_ID_SIZE                1
#define CPD_VS_SEND_FILE_START_OFFSET_IN_CMD  4
#define CPD_BT_CMD_LENGTH_POSITION            3

#define STE_CONN_CPD_NAME                       "ste_conn_cpd\0"
#define STE_CONN_CPD_WQ_NAME                    "ste_conn_cpd_wq\0"

/* Defines needed for creation of some hci packets. */
#define HCI_CMD_H4_POS						0
#define HCI_CMD_OPCODE_POS					1
#define HCI_CMD_PARAM_LEN_POS					3
#define HCI_CMD_PARAM_POS					4
#define HCI_CMD_HDR_SIZE					4

/* Defines needed for HCI VS Store In FS cmd creation. */
#define HCI_VS_STORE_IN_FS_USR_ID_POS		HCI_CMD_PARAM_POS
#define HCI_VS_STORE_IN_FS_USR_ID_SIZE		1
#define HCI_VS_STORE_IN_FS_PARAM_LEN_POS	(HCI_VS_STORE_IN_FS_USR_ID_POS + \
											HCI_VS_STORE_IN_FS_USR_ID_SIZE)
#define HCI_VS_STORE_IN_FS_PARAM_LEN_SIZE	1
#define HCI_VS_STORE_IN_FS_PARAM_POS		(HCI_VS_STORE_IN_FS_PARAM_LEN_POS + \
											HCI_VS_STORE_IN_FS_PARAM_LEN_SIZE)
#define HCI_VS_STORE_IN_FS_USR_ID_BD_ADDR	0xFE

#define CPD_SET_MAIN_STATE(__cpd_new_state) \
	STE_CONN_SET_STATE("main_state", cpd_info->main_state, __cpd_new_state)
#define CPD_SET_BOOT_STATE(__cpd_new_state) \
	STE_CONN_SET_STATE("boot_state", cpd_info->boot_state, __cpd_new_state)
#define CPD_SET_CLOSING_STATE(__cpd_new_state) \
	STE_CONN_SET_STATE("closing_state", cpd_info->closing_state, __cpd_new_state)
#define CPD_SET_BOOT_FILE_LOAD_STATE(__cpd_new_state) \
	STE_CONN_SET_STATE("boot_file_load_state", cpd_info->boot_file_load_state, __cpd_new_state)
#define CPD_SET_BOOT_DOWNLOAD_STATE(__cpd_new_state) \
	STE_CONN_SET_STATE("boot_download_state", cpd_info->boot_download_state, __cpd_new_state)


/* ------------------ Internal type definitions --------------------------- */

#define LOGGER_DIRECTION_TX 0
#define LOGGER_DIRECTION_RX 1

/**
  * struct ste_conn_cpd_work_struct - Work structure for ste_conn CPD module.
  * @work: Work structure.
  * @skb:  Data packet.
  * @data: Private data for ste_conn CPD.
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
  * @CPD_STATE_INITIALIZING: CPD initializing.
  * @CPD_STATE_IDLE:         No user registered to CPD.
  * @CPD_STATE_BOOTING:      CPD booting after first user is registered.
  * @CPD_STATE_CLOSING:      CPD closing after last user has deregistered.
  * @CPD_STATE_RESETING:     CPD reset requested.
  * @CPD_STATE_ACTIVE:       CPD up and running with at least one user.
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
  * @BOOT_STATE_NOT_STARTED:                    Boot has not yet started.
  * @BOOT_STATE_READ_LOCAL_VERSION_INFORMATION: ReadLocalVersionInformation command has been sent.
  * @BOOT_STATE_SEND_BD_ADDRESS:		VS Store In FS cmd with bd address has been sent.
  * @BOOT_STATE_GET_FILES_TO_LOAD:              CPD is retreiving file to load.
  * @BOOT_STATE_DOWNLOAD_PATCH:                 CPD is downloading patches.
  * @BOOT_STATE_ACTIVATE_PATCHES_AND_SETTINGS:  CPD is activating patches and settings.
  * @BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_HIGH:	CPD is activating sys clock out.
  * @BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_LOW:CPD is activating sys clock out.
  * @BOOT_STATE_READY:                          CPD boot is ready.
  * @BOOT_STATE_FAILED:                         CPD boot failed.
  */
enum cpd_boot_state {
	BOOT_STATE_NOT_STARTED,
	BOOT_STATE_READ_LOCAL_VERSION_INFORMATION,
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
  * enum cpd_closing_state - CLOSING-state for CPD.
  * @CLOSING_STATE_RESET: HCI RESET_CMD has been sent.
  * @CLOSING_STATE_POWER_SWITCH_OFF: HCI VS_POWER_SWITCH_OFF command has been sent.
  * @CLOSING_STATE_GPIO_DEREGISTER:  Time to deregister GPIOs.
  */
enum cpd_closing_state {
	CLOSING_STATE_RESET,
	CLOSING_STATE_POWER_SWITCH_OFF,
	CLOSING_STATE_GPIO_DEREGISTER
};

/**
  * enum cpd_boot_file_load_state - BOOT_FILE_LOAD-state for CPD.
  * @BOOT_FILE_LOAD_STATE_LOAD_PATCH:           Loading patches.
  * @BOOT_FILE_LOAD_STATE_LOAD_STATIC_SETTINGS: Loading static settings.
  * @BOOT_FILE_LOAD_STATE_NO_MORE_FILES:        No more files to load.
  * @BOOT_FILE_LOAD_STATE_FAILED:               File loading failed.
  */
enum cpd_boot_file_load_state {
	BOOT_FILE_LOAD_STATE_LOAD_PATCH,
	BOOT_FILE_LOAD_STATE_LOAD_STATIC_SETTINGS,
	BOOT_FILE_LOAD_STATE_NO_MORE_FILES,
	BOOT_FILE_LOAD_STATE_FAILED
};

/**
  * enum cpd_boot_download_state - BOOT_DOWNLOAD state.
  * @BOOT_DOWNLOAD_STATE_PENDING: Download in progress.
  * @BOOT_DOWNLOAD_STATE_SUCCESS: Download successfully finished.
  * @BOOT_DOWNLOAD_STATE_FAILED:  Downloading failed.
  */
enum cpd_boot_download_state {
	BOOT_DOWNLOAD_STATE_PENDING,
	BOOT_DOWNLOAD_STATE_SUCCESS,
	BOOT_DOWNLOAD_STATE_FAILED
};

/**
  * enum cpd_fm_radio_mode - FM Radio mode.
  * It's needed because some FM do-commands generate interrupts
  * only when the fm driver is in specific mode and we need to know
  * if we should expect the interrupt.
  * @CPD_FM_RADIO_MODE_IDLE:  Radio mode is Idle (default).
  * @CPD_FM_RADIO_MODE_FMT:   Radio mode is set to FMT (transmitter).
  * @CPD_FM_RADIO_MODE_FMR:   Radio mode is set to FMR (receiver).
  */
enum cpd_fm_radio_mode {
	CPD_FM_RADIO_MODE_IDLE,
	CPD_FM_RADIO_MODE_FMT,
	CPD_FM_RADIO_MODE_FMR
};

/**
  * struct ste_conn_cpd_users - Stores all current users of CPD.
  * @bt_cmd:       	BT command channel user.
  * @bt_acl:       	BT ACL channel user.
  * @bt_evt:       	BT event channel user.
  * @fm_radio:     	FM radio channel user.
  * @gnss GNSS:    	GNSS channel user.
  * @debug Debug:  	Internal debug channel user.
  * @ste_tools:    	ST-E tools channel user.
  * @hci_logger:   	HCI logger channel user.
  * @us_ctrl: 		User space control channel user.
  * @bt_audio: 		BT audio command channel user.
  * @fm_radio_audio: 	FM audio command channel user.
  * @core: 	Core command channel user..
  * @nbr_of_users: 	Number of users currently registered (not including the HCI logger).
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
  * @hci_version:        HCI version of local controller.
  * @hci_revision:       HCI revision of local controller.
  * @lmp_pal_version:    LMP/PAL version of local controller.
  * @manufacturer:       Manufacturer of local controller.
  * @lmp_pal_subversion: LMP/PAL subbversion of local controller.
  *
  * According to Bluetooth HCI Read Local Version Information command.
  */
struct ste_conn_local_chip_info {
	uint8_t   hci_version;
	uint16_t  hci_revision;
	uint8_t   lmp_pal_version;
	uint16_t  manufacturer;
	uint16_t  lmp_pal_subversion;
};

/**
  * struct tx_list_item - Structure to store skb and sender name if skb can't be sent due to flow control.
  * @list:        list_head struct.
  * @skb:       sk_buff struct.
  * @dev:       sending device struct pointer.
  */
struct tx_list_item {
	struct list_head list;
	struct sk_buff *skb;
	struct ste_conn_device *dev;
};

/**
  * struct ste_conn_cpd_info - Main info structure for CPD.
  * @users:                Stores all users of CPD.
  * @local_chip_info:      Stores information of local controller.
  * @patch_file_name:      Stores patch file name.
  * @settings_file_name:   Stores settings file name.
  * @fw_file:              Stores firmware file (patch or settings).
  * @fw_file_rd_offset:    Current read offset in firmware file.
  * @chunk_id:             Stores current chunk ID of write file operations.
  * @main_state:           Current Main-state of CPD.
  * @boot_state:           Current BOOT-state of CPD.
  * @closing_state:        Current CLOSING-state of CPD.
  * @boot_file_load_state: Current BOOT_FILE_LOAD-state of CPD.
  * @boot_download_state:  Current BOOT_DOWNLOAD-state of CPD.
  * @wq:                   CPD workqueue.
  * @hci_logger_config:    Stores HCI logger configuration.
  * @setup_lock:           Spinlock for setup of CPD.
  * @dev:                  Device structure for STE Connectivity driver.
  * @h4_channels:	   HCI H:4 channel used by this device.
  * @tx_list_bt:	TX queue for HCI BT cmds when nr of cmds allowed is 0 (ste_conn internal flow ctrl).
  * @tx_list_fm:        TX queue for HCI FM cmds when nr of cmds allowed is 0 (ste_conn internal flow ctrl).
  * @tx_bt_mutex:  Mutex used to protect some global structures related to internal BT cmd flow control.
  * @tx_fm_mutex:  Mutex used to protect some global structures related to internal FM cmd flow control.
  * @tx_fm_audio_awaiting_irpt:  Indicates if an FM interrupt evt related to audio driver cmd is expected.
  * @fm_radio_mode:  Current FM radio mode.
  * @tx_nr_pkts_allowed_bt:  Number of packets allowed to send on BT HCI CMD H4 channel.
  * @tx_nr_pkts_allowed_fm:  Number of packets allowed to send on BT FM RADIO H4 channel.
  * @tx_nr_outstanding_cmds_bt:  Number of packets sent but not confirmed on BT HCI CMD H4 channel.
  * @hci_audio_cmd_opcode_bt:  Stores the OpCode of the last sent audio driver HCI BT CMD.
  * @hci_audio_fm_cmd_id:  Stores the Cmd Id of the last sent audio driver HCI FM RADIO cmd.
  */
struct ste_conn_cpd_info {
	struct ste_conn_cpd_users		users;
	struct ste_conn_local_chip_info		local_chip_info;
	char					*patch_file_name;
	char					*settings_file_name;
	const struct firmware			*fw_file;
	int					fw_file_rd_offset;
	uint8_t					chunk_id;
	enum cpd_main_state			main_state;
	enum cpd_boot_state			boot_state;
	enum cpd_closing_state			closing_state;
	enum cpd_boot_file_load_state		boot_file_load_state;
	enum cpd_boot_download_state		boot_download_state;
	struct workqueue_struct			*wq;
	struct ste_conn_cpd_hci_logger_config	hci_logger_config;
	spinlock_t				setup_lock;
	struct device				*dev;
	struct ste_conn_ccd_h4_channels		h4_channels;
	struct list_head			tx_list_bt;
	struct list_head			tx_list_fm;
	struct mutex				tx_bt_mutex;
	struct mutex				tx_fm_mutex;
	bool					tx_fm_audio_awaiting_irpt;
	enum cpd_fm_radio_mode			fm_radio_mode;
	int					tx_nr_pkts_allowed_bt;
	int					tx_nr_pkts_allowed_fm;
	int					tx_nr_outstanding_cmds_bt;
	uint16_t				hci_audio_cmd_opcode_bt;
	uint16_t				hci_audio_fm_cmd_id;
};

/* ------------------ Internal variable declarations ---------------------- */

/*
  * cpd_info - Main information object for CPD.
  */
static struct ste_conn_cpd_info *cpd_info;

/*
  * cpd_msg_reset_cmd_req - Hardcoded HCI Reset command.
  */
static const uint8_t cpd_msg_reset_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	CPD_MAKE_FIRST_BYTE_IN_CMD(HCI_BT_OCF_RESET),
	CPD_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_CTRL_BB, HCI_BT_OCF_RESET),
	0x00
};

/*
  * cpd_msg_read_local_version_information_cmd_req -
  * Hardcoded HCI Read Local Version Information command
  */
static const uint8_t cpd_msg_read_local_version_information_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	CPD_MAKE_FIRST_BYTE_IN_CMD(HCI_BT_OCF_READ_LOCAL_VERSION_INFO),
	CPD_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_LINK_INFO, HCI_BT_OCF_READ_LOCAL_VERSION_INFO),
	0x00
};

/*
  * cpd_msg_vs_store_in_fs_cmd_req - Hardcoded HCI Store in FS command.
  */
static const uint8_t cpd_msg_vs_store_in_fs_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	CPD_MAKE_FIRST_BYTE_IN_CMD(HCI_BT_OCF_VS_STORE_IN_FS),
	CPD_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, HCI_BT_OCF_VS_STORE_IN_FS),
	0x00, /* 1 byte for HCI cmd length. */
	0x00, /* 1 byte for User_Id. */
	0x00  /* 1 byte for vs_store_in_fs_cmd_req parameter data length. */
};

/*
  * cpd_msg_vs_write_file_block_cmd_req -
  * Hardcoded HCI Write File Block vendor specific command
  */
static const uint8_t cpd_msg_vs_write_file_block_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	CPD_MAKE_FIRST_BYTE_IN_CMD(HCI_BT_OCF_VS_WRITE_FILE_BLOCK),
	CPD_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, HCI_BT_OCF_VS_WRITE_FILE_BLOCK),
	0x00
};

/*
  * cpd_msg_vs_system_reset_cmd_req -
  * Hardcoded HCI System Reset vendor specific command
  */
static const uint8_t cpd_msg_vs_system_reset_cmd_req[] = {
	0x00, /* Reserved for H4 channel*/
	CPD_MAKE_FIRST_BYTE_IN_CMD(HCI_BT_OCF_VS_SYSTEM_RESET),
	CPD_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, HCI_BT_OCF_VS_SYSTEM_RESET),
	0x00
};

#ifdef ENABLE_SYS_CLK_OUT
/*
  * cpd_msg_vs_system_reset_cmd_req -
  * Hardcoded HCI Read_Register 0x40014004
  */
static const uint8_t cpd_msg_read_register_0x40014004[] = {
	0x00, /* Reserved for H4 channel*/
	CPD_MAKE_FIRST_BYTE_IN_CMD(HCI_BT_OCF_VS_READ_REGISTER),
	CPD_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, HCI_BT_OCF_VS_READ_REGISTER),
	0x05, /* HCI cmd length. */
	0x01, /* Register type = 32-bit*/
	0X04, /* Memory adress byte 1*/
	0X40, /* Memory adress byte 2*/
	0X01, /* Memory adress byte 3*/
	0X40, /* Memory adress byte 4*/
};

/*
  * cpd_msg_vs_system_reset_cmd_req -
  * Hardcoded HCI Write_Register 0x40014004
  */
static const uint8_t cpd_msg_write_register_0x40014004[] = {
	0x00, /* Reserved for H4 channel*/
	CPD_MAKE_FIRST_BYTE_IN_CMD(HCI_BT_OCF_VS_WRITE_REGISTER),
	CPD_MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, HCI_BT_OCF_VS_WRITE_REGISTER),
	0x09, /* HCI cmd length. */
	0x01, /* Register type = 32-bit*/
	0X04, /* Memory adress byte 1*/
	0X40, /* Memory adress byte 2*/
	0X01, /* Memory adress byte 3*/
	0X40, /* Memory adress byte 4*/
	0X00, /* Register value byte 1*/
	0X00, /* Register value byte 2*/
	0X00, /* Register value byte 3*/
	0X00 /* Register value byte 4*/
};
#endif /*ENABLE_SYS_CLK_OUT*/

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

/* ------------------ Internal function declarations ---------------------- */
static int cpd_check_for_audio_evt(int h4_channel, struct ste_conn_device **dev,
	const struct sk_buff * const skb);
static void cpd_update_internal_flow_control_h4_bt_cmd(const struct sk_buff * const skb);
static void cpd_update_internal_flow_control_fm(const struct sk_buff * const skb);
static bool cpd_fm_irpt_expected(uint16_t cmd_id);
static bool cpd_fm_is_do_cmd_irpt(uint16_t irpt_val);
static int  cpd_enable_hci_logger(struct sk_buff *skb);
static int  cpd_find_h4_user(int h4_channel, struct ste_conn_device **dev_store);
static int  cpd_add_h4_user(struct ste_conn_device *dev, const char * const name);
static int  cpd_remove_h4_user(struct ste_conn_device **dev);
static void cpd_chip_startup(void);
static void cpd_chip_shutdown(void);
static bool cpd_handle_internal_rx_data_bt_evt(struct sk_buff *skb);
static void cpd_create_work_item(work_func_t work_func, struct sk_buff *skb,
								void *data);
static bool cpd_handle_reset_cmd_complete_evt(uint8_t *data);
static bool cpd_handle_read_local_version_info_cmd_complete_evt(uint8_t *data);
static bool cpd_handle_vs_store_in_fs_cmd_complete_evt(uint8_t *data);
static bool cpd_handle_vs_write_file_block_cmd_complete_evt(uint8_t *data);
static bool cpd_handle_vs_power_switch_off_cmd_complete_evt(uint8_t *data);
static bool cpd_handle_vs_system_reset_cmd_complete_evt(uint8_t *data);
#ifdef ENABLE_SYS_CLK_OUT
static bool cpd_handle_vs_read_register_cmd_complete_evt(uint8_t *data);
static bool cpd_handle_vs_write_register_cmd_complete_evt(uint8_t *data);
#endif
static void cpd_work_power_off_chip(struct work_struct *work);
static void cpd_work_reset_after_error(struct work_struct *work);
static void cpd_work_load_patch_and_settings(struct work_struct *work);
static void cpd_work_continue_with_file_download(struct work_struct *work);
static bool cpd_get_file_to_load(const struct firmware *fw, char **file_name_p);
static char *cpd_get_one_line_of_text(char *wr_buffer, int max_nbr_of_bytes,
					char *rd_buffer, int *bytes_copied);
static void cpd_create_and_send_bt_cmd(const uint8_t *data, int length);
static void cpd_send_bd_address(void);
static void cpd_transmit_skb_to_ccd(struct sk_buff *skb, bool use_logger);
static void cpd_transmit_skb_to_ccd_with_flow_ctrl(struct sk_buff *skb,
		struct ste_conn_device *dev, const uint8_t h4_header);
static void cpd_transmit_skb_to_ccd_with_flow_ctrl_bt(struct sk_buff *skb,
		struct ste_conn_device *dev);
static void cpd_transmit_skb_to_ccd_with_flow_ctrl_fm(struct sk_buff *skb,
		struct ste_conn_device *dev);
static void cpd_transmit_skb_from_tx_queue_bt(void);
static void cpd_transmit_skb_from_tx_queue_fm(void);
static void cpd_read_and_send_file_part(void);
static void cpd_send_patch_file(void);
static void cpd_send_settings_file(void);

static void cpd_handle_reset_of_user(struct ste_conn_device **dev);
static void cpd_free_user_dev(struct ste_conn_device **dev);

/* ------------------ ste_conn API functions ------------------------------ */

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
	current_dev = kzalloc(sizeof(*current_dev), GFP_KERNEL);
	if (current_dev) {
		err = ste_conn_devices_get_h4_channel(name, &(current_dev->h4_channel));
		if (err) {
			STE_CONN_ERR("Couldn't find H4 channel for %s", name);
			goto error_handling;
		}
		current_dev->dev        = cpd_info->dev;
		current_dev->callbacks  = kmalloc(sizeof(*(current_dev->callbacks)), GFP_KERNEL);
		if (current_dev->callbacks) {
			memcpy((char *)current_dev->callbacks, (char *)cb, sizeof(*(current_dev->callbacks)));
		} else {
			STE_CONN_ERR("Couldn't allocate callbacks ");
			goto error_handling;
		}

		/* Retreive pointer to the correct CPD user structure */
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

int ste_conn_reset(struct ste_conn_device *dev)
{
	int err = 0;

	STE_CONN_INFO("ste_conn_reset");

	if (!cpd_info) {
		STE_CONN_ERR("Hardware not started");
		return -EACCES;
	}

	CPD_SET_CLOSING_STATE(CLOSING_STATE_POWER_SWITCH_OFF);
	CPD_SET_MAIN_STATE(CPD_STATE_RESETING);

	/* Shutdown the chip */
	cpd_chip_shutdown();

	/* Inform all registered users about the reset and free the user devices
	 * Don't send reset for debug and logging channels
	 */
	cpd_handle_reset_of_user(&(cpd_info->users.bt_cmd));
	cpd_handle_reset_of_user(&(cpd_info->users.bt_acl));
	cpd_handle_reset_of_user(&(cpd_info->users.bt_evt));
	cpd_handle_reset_of_user(&(cpd_info->users.fm_radio));
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

struct sk_buff *ste_conn_alloc_skb(unsigned int size, gfp_t priority)
{
	struct sk_buff *skb;

	STE_CONN_INFO("ste_conn_alloc_skb");

	/* Allocate the SKB and reserve space for the header */
	skb = alloc_skb(size + STE_CONN_SKB_RESERVE, priority);

	if (skb) {
		skb_reserve(skb, STE_CONN_SKB_RESERVE);
	}
	return skb;
}

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


	if (cpd_info->h4_channels.hci_logger_channel == dev->h4_channel) {
		/* Treat the HCI logger write differently.
		 * A write can only mean a change of configuration.
		 */
		err = cpd_enable_hci_logger(skb);
	} else if (cpd_info->h4_channels.core_channel == dev->h4_channel) {
		STE_CONN_ERR("Not possble to write data on core channel only supports enable /disable chip");
	} else if (CPD_STATE_ACTIVE == cpd_info->main_state) {
		/* Move the data pointer to the H:4 header position and store the H4 header */
		h4_header = skb_push(skb, STE_CONN_SKB_RESERVE);
		*h4_header = (uint8_t)dev->h4_channel;

		/* Because there are more users of some H4 channels (currently audio application
		 * for BT cmd and FM channel) we need to have an internal HCI cmd flow control
		 * in ste_conn driver. */
		if (cpd_info->h4_channels.bt_cmd_channel == *h4_header) {
			/*Temp workaround for not working flow control for FM*/
			/*cpd_info->h4_channels.fm_radio_channel == *h4_header) {*/
			cpd_transmit_skb_to_ccd_with_flow_ctrl(skb, dev, *h4_header);
		} else {
			/* Other channels are not affected by the flow control so transmit the sk_buffer to CCD */
			cpd_transmit_skb_to_ccd(skb, dev->logger_enabled);
		}
	} else {
		STE_CONN_ERR("Trying to transmit data when ste_conn CPD is not active");
		err = -EACCES;
		kfree_skb(skb);
	}

	return err;
}

/* ---------------- External functions -------------- */

void ste_conn_cpd_hw_registered(void)
{
	STE_CONN_INFO("ste_conn_cpd_hw_registered");

	/* Now it is time to shutdown the controller to reduce
	 * power consumption until any users register
	 */
	cpd_chip_shutdown();
}

void ste_conn_cpd_hw_deregistered(void)
{
	STE_CONN_INFO("ste_conn_cpd_hw_deregistered");

	CPD_SET_MAIN_STATE(CPD_STATE_INITIALIZING);
}

void ste_conn_cpd_data_received(struct sk_buff *skb)
{
	struct ste_conn_device *dev = NULL;
	uint8_t h4_channel;
	int err = 0;
	bool pkt_handled = false;

	STE_CONN_INFO("ste_conn_cpd_data_received");

	if (!skb) {
		STE_CONN_ERR("No data supplied");
		return;
	}

	h4_channel = *(skb->data);

	STE_CONN_DBG_DATA_CONTENT("Received %d bytes: 0x %02X %02X %02X %02X %02X %02X %02X %02X",
					skb->len, skb->data[0], skb->data[1],
					skb->data[2], skb->data[3], skb->data[4],
					skb->data[5], skb->data[6], skb->data[7]);

	/* If this is a BT HCI event we might have to handle it internally */
	if (cpd_info->h4_channels.bt_evt_channel == h4_channel) {
		pkt_handled = cpd_handle_internal_rx_data_bt_evt(skb);
		STE_CONN_DBG("pkt_handled %d", pkt_handled);

		/* If packet has been handled internally just return. */
		if (pkt_handled) {
			return;
		}
	}

	/* If it's BT EVT or FM channel data check if it's evt generated by previously sent audio app cmd.
	 * If so then that evt should be dispatched to BT or FM audio user, respectively. */
	err = cpd_check_for_audio_evt(h4_channel, &dev, skb);

	/* If it wasn't an audio related evt and no error occured proceed with checking other users. */
	if (!err && !dev) {
		err = cpd_find_h4_user(h4_channel, &dev);
	}

	if (!err) {
		/* If HCI logging is enabled for this channel, copy the data to
		 * the HCI logging output.
		 */
		if (cpd_info->users.hci_logger && dev->logger_enabled) {
			/* Alloc a new sk_buffer and copy the data into it.
			 * Then send it to the HCI logger. */
			struct sk_buff *skb_log = alloc_skb(skb->len + 1, GFP_KERNEL);
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
	cpd_info = kzalloc(sizeof(*cpd_info), GFP_KERNEL);

	if (cpd_info) {
		cpd_info->main_state           = CPD_STATE_INITIALIZING;
		cpd_info->boot_state           = BOOT_STATE_NOT_STARTED;
		cpd_info->closing_state        = CLOSING_STATE_POWER_SWITCH_OFF;
		cpd_info->boot_file_load_state = BOOT_FILE_LOAD_STATE_NO_MORE_FILES;

		cpd_info->dev = dev;

		cpd_info->tx_nr_pkts_allowed_bt = 1;
		cpd_info->tx_nr_pkts_allowed_fm = 1;
		cpd_info->tx_nr_outstanding_cmds_bt = 0;
		cpd_info->hci_audio_cmd_opcode_bt = 0xFFFF;
		cpd_info->hci_audio_fm_cmd_id = 0xFFFF;

		/* Get the H4 channel ID for all channels */
		ste_conn_devices_get_h4_channel(STE_CONN_DEVICES_BT_CMD,
						&(cpd_info->h4_channels.bt_cmd_channel));
		ste_conn_devices_get_h4_channel(STE_CONN_DEVICES_BT_ACL,
						&(cpd_info->h4_channels.bt_acl_channel));
		ste_conn_devices_get_h4_channel(STE_CONN_DEVICES_BT_EVT,
						&(cpd_info->h4_channels.bt_evt_channel));
		ste_conn_devices_get_h4_channel(STE_CONN_DEVICES_GNSS,
						&(cpd_info->h4_channels.gnss_channel));
		ste_conn_devices_get_h4_channel(STE_CONN_DEVICES_FM_RADIO,
						&(cpd_info->h4_channels.fm_radio_channel));
		ste_conn_devices_get_h4_channel(STE_CONN_DEVICES_DEBUG,
						&(cpd_info->h4_channels.debug_channel));
		ste_conn_devices_get_h4_channel(STE_CONN_DEVICES_STE_TOOLS,
						&(cpd_info->h4_channels.ste_tools_channel));
		ste_conn_devices_get_h4_channel(STE_CONN_DEVICES_HCI_LOGGER,
						&(cpd_info->h4_channels.hci_logger_channel));
		ste_conn_devices_get_h4_channel(STE_CONN_DEVICES_US_CTRL,
						&(cpd_info->h4_channels.us_ctrl_channel));
		ste_conn_devices_get_h4_channel(STE_CONN_DEVICES_CORE,
						&(cpd_info->h4_channels.core_channel));

		cpd_info->wq = create_singlethread_workqueue(STE_CONN_CPD_WQ_NAME);
		if (!cpd_info->wq) {
			STE_CONN_ERR("Could not create workqueue");
			err = -ENOMEM;
			goto error_handling;
		}

		/* Initialize linked lists for HCI BT and FM cmds that can't be sent due to internal ste_conn flow control. */
		INIT_LIST_HEAD(&cpd_info->tx_list_bt);
		INIT_LIST_HEAD(&cpd_info->tx_list_fm);

		/* Initialize tx mutexes. */
		mutex_init(&cpd_info->tx_bt_mutex);
		mutex_init(&cpd_info->tx_fm_mutex);

		/* Initialize the spin lock */
		spin_lock_init(&cpd_info->setup_lock);

		/* Initialize the character devices */
		ste_conn_char_devices_init(char_dev_usage, dev);

		/* Allocate file names that will be used, deallocated in ste_conn_cpd_exit */
		cpd_info->patch_file_name = kzalloc(STE_CONN_FILENAME_MAX + 1, GFP_KERNEL);
		if (!cpd_info->patch_file_name) {
			STE_CONN_ERR("Couldn't allocate name buffer for patch file.");
			err = -ENOMEM;
			goto error_handling_destroy_wq;
		}
			/* Allocate file names that will be used, deallocaded in ste_conn_cpd_exit  */
		cpd_info->settings_file_name = kzalloc(STE_CONN_FILENAME_MAX + 1, GFP_KERNEL);
		if (!cpd_info->settings_file_name) {
			STE_CONN_ERR("Couldn't allocate name buffers settings file.");
			err = -ENOMEM;
			goto error_handling_destroy_wq;
		}
	} else {
		STE_CONN_ERR("Couldn't allocate cpd_info");
		err = -ENOMEM;
		goto finished;
	}

	goto finished;

error_handling_destroy_wq:
	destroy_workqueue(cpd_info->wq);
error_handling:
	if (cpd_info) {
		kfree(cpd_info->patch_file_name);
		kfree(cpd_info->settings_file_name);
		kfree(cpd_info);
		cpd_info = NULL;
	}

finished:
	return err;
}

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

	/* Free everything allocated in cpd_info */
	kfree(cpd_info->patch_file_name);
	kfree(cpd_info->settings_file_name);

	/* Destroy mutexes. */
	mutex_destroy(&cpd_info->tx_bt_mutex);
	mutex_destroy(&cpd_info->tx_fm_mutex);

	if (cpd_info->fw_file) {
		release_firmware(cpd_info->fw_file);
	}
	destroy_workqueue(cpd_info->wq);
	kfree(cpd_info);
	cpd_info = NULL;
}

/* ---------------- Internal functions --------------------------------- */

/**
 * cpd_check_for_audio_evt() - Check if incoming data packet is an audio related packet.
 * @h4_channel:     H4 channel.
 * @dev:     Stored ste_conn device.
 * @skb:  skb with received packet.
 *
 * The cpd_check_for_audio_evt() checks if incoming data packet is an audio related
 * BT Cmd Cmpl/Cmd Status Evt or FM Evt.
 *
 * Returns:
 *   0 - if no error occured.
 *   -EINVAL - if ste_conn_device not found.
 */
static int cpd_check_for_audio_evt(int h4_channel, struct ste_conn_device **dev,
	const struct sk_buff * const skb)
{
	uint8_t *data = &skb->data[STE_CONN_SKB_RESERVE];
	uint8_t event_code = data[0];
	int err = 0;

	/* BT evt */
	if (h4_channel == cpd_info->h4_channels.bt_evt_channel) {
		uint16_t opcode = 0;

		cpd_update_internal_flow_control_h4_bt_cmd(skb);

		if (HCI_BT_EVT_CMD_COMPLETE == event_code) {
			opcode = (uint16_t)(data[3] | (data[4] << 8));
		} else if (HCI_BT_EVT_CMD_STATUS == event_code) {
			opcode = (uint16_t)(data[4] | (data[5] << 8));
		}

		if (opcode != 0 && opcode == cpd_info->hci_audio_cmd_opcode_bt) {
			STE_CONN_INFO("BT OpCode match = 0x%x", opcode);
			*dev = cpd_info->users.bt_audio;

			if (!*dev) {
				STE_CONN_ERR("H:4 channel not registered in cpd_info: 0x%X", h4_channel);
				err = -EINVAL;
			}
		}
	/* FM evt */
	/*Temp workaround for not working flow control for FM*/
	} else if (0) {/*h4_channel == cpd_info->h4_channels.fm_radio_channel) {*/

		cpd_update_internal_flow_control_fm(skb);

		/* 2nd byte for cmd cmpl evt is Status and equals always 0x00. */
		if (data[1] == 0x00) {
			/* Get the cmd id - bits 15-3 within cmd cmpl evt. */
			uint16_t cmd_id = (uint16_t)(((data[6] | (data[7] << 8)) & 0xFFF8) >> 3);

			if (cmd_id == cpd_info->hci_audio_fm_cmd_id) {
				STE_CONN_INFO("FM Audio Function Code match = 0x%x", cmd_id);
				*dev = cpd_info->users.fm_radio_audio;

				if (!*dev) {
					STE_CONN_ERR("H:4 channel not registered in cpd_info: 0x%X", h4_channel);
					err = -EINVAL;
				}
			}
		/* In case of an Interrupt 2nd byte is always 0xFE. */
		} else if (data[1] == 0xFE) {
			uint16_t irpt_val = (uint16_t)(data[3] | (data[4] << 8));

			if (cpd_fm_is_do_cmd_irpt(irpt_val) && (cpd_info->tx_fm_audio_awaiting_irpt)) {
				STE_CONN_INFO("FM Audio Interrupt match.");
				*dev = cpd_info->users.fm_radio_audio;

				cpd_info->tx_fm_audio_awaiting_irpt = false;

				if (!*dev) {
					STE_CONN_ERR("H:4 channel not registered in cpd_info: 0x%X", h4_channel);
					err = -EINVAL;
				}
			}
		}
	}

	return err;
}

/**
 * cpd_update_internal_flow_control_h4_bt_cmd() - Update number of tickets and number of outstanding commands for BT CMD channel.
 * @skb:  skb with received packet.
 *
 * The cpd_update_internal_flow_control_h4_bt_cmd() checks if incoming data packet is BT Cmd Cmpl/Cmd Status
 * Evt and if so updates number of tickets and number of outstanding commands. It also calls function to send queued
 * cmds (if the list of queued cmds is not empty).
 */
static void cpd_update_internal_flow_control_h4_bt_cmd(const struct sk_buff * const skb)
{
	uint8_t *data = &skb->data[STE_CONN_SKB_RESERVE];
	uint8_t event_code = data[0];
	uint16_t opcode;

	if (HCI_BT_EVT_CMD_COMPLETE == event_code) {
		/* Get the OpCode */
		opcode = (uint16_t)(data[3] | (data[4] << 8));

		/* If it's HCI Cmd Cmpl Evt then we might get some HCI tickets back. Also we can decrease the nr
		 * outstanding HCI cmds (if it's not NOP cmd or one of the cmds that generate both Cmd Status Evt
		 * and Cmd Cmpl Evt). Check if we have any HCI cmds waiting in the tx list
		 * and send them if there are tickets available. */
		if (opcode != 0 && opcode != CPD_BT_VS_BT_ENABLE_OPCODE) {
			(cpd_info->tx_nr_outstanding_cmds_bt)--;
		}
		cpd_info->tx_nr_pkts_allowed_bt = data[HCI_BT_EVT_CMD_COMPL_NR_OF_PKTS_POS];
		STE_CONN_DBG("New tx_nr_pkts_allowed_bt = %d", cpd_info->tx_nr_pkts_allowed_bt);
		STE_CONN_DBG("New tx_nr_outstanding_cmds_bt = %d", cpd_info->tx_nr_outstanding_cmds_bt);

		if (!list_empty(&cpd_info->tx_list_bt)) {
			cpd_transmit_skb_from_tx_queue_bt();
		}
	} else if (HCI_BT_EVT_CMD_STATUS == event_code) {
		opcode = (uint16_t)(data[4] | (data[5] << 8));

		/* If it's HCI Cmd Status Evt then we might get some HCI tickets back. Also we can decrease the nr
		 * outstanding HCI cmds (if it's not NOP cmd). Check if we have any HCI cmds waiting in the tx queue
		 * and send them if there are tickets available. */
		if (opcode != 0) {
			(cpd_info->tx_nr_outstanding_cmds_bt)--;
		}
		cpd_info->tx_nr_pkts_allowed_bt = data[HCI_BT_EVT_CMD_STATUS_NR_OF_PKTS_POS];
		STE_CONN_DBG("New tx_nr_pkts_allowed_bt = %d", cpd_info->tx_nr_pkts_allowed_bt);
		STE_CONN_DBG("New tx_nr_outstanding_cmds_bt = %d", cpd_info->tx_nr_outstanding_cmds_bt);

		if (!list_empty(&cpd_info->tx_list_bt)) {
			cpd_transmit_skb_from_tx_queue_bt();
		}
	}
}

/**
 * cpd_update_internal_flow_control_fm() - Update number of packets allowed for FM channel.
 * @skb:  skb with received packet.
 *
 * The cpd_update_internal_flow_control_fm() checks if incoming data packet is FM packet indicating that
 * the previous command has been handled and if so updates number of allowed packets. It also calls
 * function to send queued cmds (if the list of queued cmds is not empty).
 */
static void cpd_update_internal_flow_control_fm(const struct sk_buff * const skb)
{
	uint8_t *data = &skb->data[STE_CONN_SKB_RESERVE];

	/* 2nd byte for cmd cmpl evt is Status and equals always 0x00. */
	if (data[1] == 0x00) {
		/* Get the cmd id - bits 15-3 within cmd cmpl evt. */
		uint16_t cmd_id = (uint16_t)(((data[6] | (data[7] << 8)) & 0xFFF8) >> 3);

		/* If it's not an evt related to do-command update nbr of allowed pckts for FM
		 * (only one outstanding pckt allowed). */
		if (!cpd_fm_irpt_expected(cmd_id)) {
			cpd_info->tx_nr_pkts_allowed_fm = 1;
			STE_CONN_DBG("New tx_nr_pkts_allowed_fm = %d", cpd_info->tx_nr_pkts_allowed_fm);
			cpd_transmit_skb_from_tx_queue_fm();
		} else if (cmd_id == cpd_info->hci_audio_fm_cmd_id) {
			/* Remember if it is do-command cmpl evt related to cmd sent by the audio driver.
			 * We need that to dispatch correctly the interrupt that will come later. */
			cpd_info->tx_fm_audio_awaiting_irpt = true;
		}
	/* In case of an Interrupt 2nd byte is always 0xFE. */
	} else if (data[1] == 0xFE) {
		uint16_t irpt_val = (uint16_t)(data[3] | (data[4] << 8));

		if (cpd_fm_is_do_cmd_irpt(irpt_val)) {

			/* If it is an interrupt related to a do-command update the number of allowed FM cmds. */
			cpd_info->tx_nr_pkts_allowed_fm = 1;
			STE_CONN_DBG("New tx_nr_pkts_allowed_fm = %d", cpd_info->tx_nr_pkts_allowed_fm);

			cpd_transmit_skb_from_tx_queue_fm();
		}
	}
}

/**
 * cpd_fm_irpt_expected() - check if this FM command will generate an interrupt.
 * @cmd_id:     command identifier.
 *
 * The cpd_fm_irpt_expected() checks if this FM command will generate an interrupt.
 *
 * Returns:
 *   true if the command will generate an interrupt.
 *   false if it won't.
 */
static bool cpd_fm_irpt_expected(uint16_t cmd_id)
{
	bool retval = false;

	switch (cmd_id) {
	case CPD_FM_DO_AIP_FADE_START:
		if (cpd_info->fm_radio_mode == CPD_FM_RADIO_MODE_FMT) {
			retval = true;
		}
		break;

	case CPD_FM_DO_AUP_BT_FADE_START:
	case CPD_FM_DO_AUP_EXT_FADE_START:
	case CPD_FM_DO_AUP_FADE_START:
		if (cpd_info->fm_radio_mode == CPD_FM_RADIO_MODE_FMR) {
			retval = true;
		}
		break;

	case CPD_FM_DO_FMR_SETANTENNA:
	case CPD_FM_DO_FMR_SP_AFSWITCH_START:
	case CPD_FM_DO_FMR_SP_AFUPDATE_START:
	case CPD_FM_DO_FMR_SP_BLOCKSCAN_START:
	case CPD_FM_DO_FMR_SP_PRESETPI_START:
	case CPD_FM_DO_FMR_SP_SCAN_START:
	case CPD_FM_DO_FMR_SP_SEARCH_START:
	case CPD_FM_DO_FMR_SP_SEARCHPI_START:
	case CPD_FM_DO_FMR_SP_TUNE_SETCHANNEL:
	case CPD_FM_DO_FMR_SP_TUNE_STEPCHANNEL:
	case CPD_FM_DO_FMT_PA_SETCONTROL:
	case CPD_FM_DO_FMT_PA_SETMODE:
	case CPD_FM_DO_FMT_SP_TUNE_SETCHANNEL:
	case CPD_FM_DO_GEN_ANTENNACHECK_START:
	case CPD_FM_DO_GEN_GOTOMODE:
	case CPD_FM_DO_GEN_POWERSUPPLY_SETMODE:
	case CPD_FM_DO_GEN_SELECTREFERENCECLOCK:
	case CPD_FM_DO_GEN_SETPROCESSINGCLOCK:
	case CPD_FM_DO_GEN_SETREFERENCECLOCKPLL:
	case CPD_FM_DO_TST_TX_RAMP_START:
		retval = true;
		break;

	default:
		break;
	}

	if (retval) {
		STE_CONN_INFO("Cmd cmpl evt for FM do-command found, cmd_id = 0x%x.", cmd_id);
	}

	return retval;
}

/**
 * cpd_fm_is_do_cmd_irpt() - check if irpt_val is one of the FM do-command related interrupts.
 * @irpt_val:     interrupt value.
 *
 * The cpd_fm_is_do_cmd_irpt() checks if irpt_val is one of the FM do-command related interrupts.
 *
 * Returns:
 *   true if it's do-command related interrupt value.
 *   false if it's not.
 */
static bool cpd_fm_is_do_cmd_irpt(uint16_t irpt_val)
{
	bool retval = false;

	switch (irpt_val) {
	case CPD_FM_IRPT_OPERATION_SUCCEEDED:
	case CPD_FM_IRPT_OPERATION_FAILED:
		retval = true;
		break;

	default:
		break;
	}

	if (retval) {
		STE_CONN_INFO("Irpt evt for FM do-command found, irpt_val = 0x%x.", irpt_val);
	}

	return retval;
}


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
			if (cpd_info->hci_logger_config.bt_cmd_enable) {
				cpd_info->users.bt_cmd->logger_enabled = true;
			} else {
				cpd_info->users.bt_cmd->logger_enabled = false;
			}
		}
		if (cpd_info->users.bt_acl) {
			if (cpd_info->hci_logger_config.bt_acl_enable) {
				cpd_info->users.bt_acl->logger_enabled = true;
			} else {
				cpd_info->users.bt_acl->logger_enabled = false;
			}
		}
		if (cpd_info->users.bt_evt) {
			if (cpd_info->hci_logger_config.bt_evt_enable) {
				cpd_info->users.bt_evt->logger_enabled = true;
			} else {
				cpd_info->users.bt_evt->logger_enabled = false;
			}
		}
		if (cpd_info->users.fm_radio) {
			if (cpd_info->hci_logger_config.fm_radio_enable) {
				cpd_info->users.fm_radio->logger_enabled = true;
			} else {
				cpd_info->users.fm_radio->logger_enabled = false;
			}
		}
		if (cpd_info->users.gnss) {
			if (cpd_info->hci_logger_config.gnss_enable) {
				cpd_info->users.gnss->logger_enabled = true;
			} else {
				cpd_info->users.gnss->logger_enabled = false;
			}
		}
	} else {
		STE_CONN_ERR("Trying to configure HCI logger with bad structure");
		err = -EACCES;
	}
	kfree_skb(skb);
	return err;
}

/**
 * cpd_find_h4_user() - Get H4 user based on supplied H4 channel.
 * @h4_channel:     H4 channel.
 * @dev:     Stored ste_conn device.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL if bad channel is supplied or no user was found.
 */
static int cpd_find_h4_user(int h4_channel, struct ste_conn_device **dev)
{
	int err = 0;

	if (h4_channel == cpd_info->h4_channels.bt_cmd_channel) {
		*dev = cpd_info->users.bt_cmd;
	} else if (h4_channel == cpd_info->h4_channels.bt_acl_channel) {
		*dev = cpd_info->users.bt_acl;
	} else if (h4_channel == cpd_info->h4_channels.bt_evt_channel) {
		*dev = cpd_info->users.bt_evt;
	} else if (h4_channel == cpd_info->h4_channels.gnss_channel) {
		*dev = cpd_info->users.gnss;
	} else if (h4_channel == cpd_info->h4_channels.fm_radio_channel) {
		*dev = cpd_info->users.fm_radio;
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
	STE_CONN_DBG("*dev (0x%X)", (uint32_t) *dev);
	if (!*dev) {
		STE_CONN_ERR("H:4 channel not registered in cpd_info: 0x%X", h4_channel);
		err = -EINVAL;
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
		STE_CONN_ERR("NULL device supplied: dev 0x%X *dev 0x%X",
					(uint32_t)dev, (uint32_t)*dev);
		err = -EINVAL;
		goto error_handling;
	}

	if ((*dev)->h4_channel == cpd_info->h4_channels.bt_cmd_channel) {
		if (*dev == cpd_info->users.bt_cmd) {
			cpd_info->users.bt_cmd = NULL;
			cpd_info->users.nbr_of_users--;
		} else if (*dev == cpd_info->users.bt_audio) {
			cpd_info->users.bt_audio = NULL;
			cpd_info->users.nbr_of_users--;
		} else {
			err = -EINVAL;
		}

		/* If both BT Cmd channel users are de-registered we can clean the tx cmd list. */
		if (cpd_info->users.bt_cmd == NULL && cpd_info->users.bt_audio == NULL) {
			struct list_head *cursor, *next;
			struct tx_list_item *tmp;

			mutex_lock(&cpd_info->tx_bt_mutex);
			list_for_each_safe(cursor, next, &cpd_info->tx_list_bt) {
					tmp = list_entry(cursor, struct tx_list_item, list);
					list_del(cursor);
					kfree_skb(tmp->skb);
					kfree(tmp);
			}

			/* Reset nbr of pckts allowed and number of outstanding bt cmds. */
			cpd_info->tx_nr_pkts_allowed_bt = 1;
			cpd_info->tx_nr_outstanding_cmds_bt = 0;
			/* Reset the hci_audio_cmd_opcode_bt. */
			cpd_info->hci_audio_cmd_opcode_bt = 0xFFFF;
			mutex_unlock(&cpd_info->tx_bt_mutex);
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

		/* If both FM Radio channel users are de-registered we can clean the tx cmd list. */
		if (cpd_info->users.fm_radio == NULL && cpd_info->users.fm_radio_audio == NULL) {
			struct list_head *cursor, *next;
			struct tx_list_item *tmp;

			mutex_lock(&cpd_info->tx_fm_mutex);
			list_for_each_safe(cursor, next, &cpd_info->tx_list_fm) {
				tmp = list_entry(cursor, struct tx_list_item, list);
				list_del(cursor);
				kfree(tmp->skb);
				kfree(tmp);
			}

			/* Reset nbr of pckts allowed. */
			cpd_info->tx_nr_pkts_allowed_fm = 1;
			/* Reset the hci_audio_cmd_opcode_bt. */
			cpd_info->hci_audio_fm_cmd_id = 0xFFFF;
			mutex_unlock(&cpd_info->tx_fm_mutex);
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
	STE_CONN_INFO("cpd_chip_shutdown");

	/* First do a quick power switch of the chip to assure a good state */
	ste_conn_ccd_set_chip_power(false);

	/* Wait 50ms before continuing to be sure that the chip detects chip power off */
	schedule_timeout_interruptible(timeval_to_jiffies(&time_50ms));

	ste_conn_ccd_set_chip_power(true);

	/* Wait 100ms before continuing to be sure that the chip is ready */
	schedule_timeout_interruptible(timeval_to_jiffies(&time_100ms));

	/* Then transmit HCI reset command to ensure the chip is using
	 * the correct transport */
	CPD_SET_CLOSING_STATE(CLOSING_STATE_RESET);
	cpd_create_and_send_bt_cmd(cpd_msg_reset_cmd_req,
				sizeof(cpd_msg_reset_cmd_req));
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
	uint8_t *data = &skb->data[STE_CONN_SKB_RESERVE];
	uint8_t event_code = data[0];

	/* First check the event code */
	if (HCI_BT_EVT_CMD_COMPLETE == event_code) {
		uint8_t hci_ogf = data[4] >> 2;
		uint16_t hci_ocf = data[3] | ((data[4] & 0x03) << 8);

		STE_CONN_DBG_DATA("hci_ogf = 0X%x", hci_ogf);
		STE_CONN_DBG_DATA("hci_ocf = 0X%x", hci_ocf);
		data += 5; /* Move to first byte after OCF */

		switch (hci_ogf) {
		case HCI_BT_OGF_LINK_CTRL:
			break;

		case HCI_BT_OGF_LINK_POLICY:
			break;

		case HCI_BT_OGF_CTRL_BB:
			switch (hci_ocf) {
			case HCI_BT_OCF_RESET:
				pkt_handled = cpd_handle_reset_cmd_complete_evt(data);
				break;

			default:
				break;
			}; /* switch (hci_ocf) */
			break;

		case HCI_BT_OGF_LINK_INFO:
			switch (hci_ocf) {
			case HCI_BT_OCF_READ_LOCAL_VERSION_INFO:
				pkt_handled = cpd_handle_read_local_version_info_cmd_complete_evt(data);
				break;

			default:
				break;
			}; /* switch (hci_ocf) */
			break;

		case HCI_BT_OGF_LINK_STATUS:
			break;

		case HCI_BT_OGF_LINK_TESTING:
			break;

		case HCI_BT_OGF_VS:
			switch (hci_ocf) {
			case HCI_BT_OCF_VS_STORE_IN_FS:
				pkt_handled = cpd_handle_vs_store_in_fs_cmd_complete_evt(data);
				break;
			case HCI_BT_OCF_VS_WRITE_FILE_BLOCK:
				pkt_handled = cpd_handle_vs_write_file_block_cmd_complete_evt(data);
				break;

			case HCI_BT_OCF_VS_POWER_SWITCH_OFF:
				pkt_handled = cpd_handle_vs_power_switch_off_cmd_complete_evt(data);
				break;

			case HCI_BT_OCF_VS_SYSTEM_RESET:
				pkt_handled = cpd_handle_vs_system_reset_cmd_complete_evt(data);
				break;
#ifdef ENABLE_SYS_CLK_OUT
			case HCI_BT_OCF_VS_READ_REGISTER:
				pkt_handled = cpd_handle_vs_read_register_cmd_complete_evt(data);
				break;
			case HCI_BT_OCF_VS_WRITE_REGISTER:
				pkt_handled = cpd_handle_vs_write_register_cmd_complete_evt(data);
				break;
#endif
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
 * cpd_create_work_item() - Create work item and add it to the work queue.
 * @work_func:	Work function.
 * @skb: 	Data packet.
 * @data: 	Private data for ste_conn CPD.
 *
 * The cpd_create_work_item() function creates work item and
 * add it to the work queue.
 */
static void cpd_create_work_item(work_func_t work_func, struct sk_buff *skb, void *data)
{
	struct ste_conn_cpd_work_struct *new_work;
	int wq_err = 1;

	new_work = kmalloc(sizeof(*new_work), GFP_KERNEL);

	if (new_work) {
		new_work->skb = skb;
		new_work->data = data;
		INIT_WORK(&new_work->work, work_func);

		wq_err = queue_work(cpd_info->wq, &new_work->work);

		if (!wq_err) {
			STE_CONN_ERR("Failed to queue work_struct because it's already in the queue!");
			kfree(new_work);
		}
	} else {
		STE_CONN_ERR("Failed to alloc memory for ste_conn_cpd_work_struct!");
	}
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

	STE_CONN_DBG("main_state = %x, boot_state = %x", cpd_info->main_state, cpd_info->boot_state);

	STE_CONN_INFO("Received Reset complete event with status 0x%X", status);

	if (cpd_info->main_state == CPD_STATE_BOOTING &&
	    cpd_info->boot_state == BOOT_STATE_NOT_STARTED) {

		/* Transmit HCI Read Local Version Information command */
		CPD_SET_BOOT_STATE(BOOT_STATE_READ_LOCAL_VERSION_INFORMATION);
		cpd_create_and_send_bt_cmd(cpd_msg_read_local_version_information_cmd_req,
						sizeof(cpd_msg_read_local_version_information_cmd_req));

		pkt_handled = true;
	} else if (cpd_info->main_state == CPD_STATE_BOOTING &&
		cpd_info->boot_state == BOOT_STATE_ACTIVATE_PATCHES_AND_SETTINGS) {
		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* The boot sequence is now finished successfully.
			 * Set states and signal to waiting thread.
			 */
			CPD_SET_BOOT_STATE(BOOT_STATE_READY);
			CPD_SET_MAIN_STATE(CPD_STATE_ACTIVE);
			wake_up_interruptible(&ste_conn_cpd_wait_queue);
		} else {
			STE_CONN_ERR("Received Reset complete event with status 0x%X", status);
			CPD_SET_BOOT_STATE(BOOT_STATE_FAILED);
			ste_conn_reset(NULL);
		}

		pkt_handled = true;

	} else if ((cpd_info->main_state == CPD_STATE_INITIALIZING) ||
		((CPD_STATE_CLOSING == cpd_info->main_state) &&
		 (CLOSING_STATE_RESET == cpd_info->closing_state))) {

		if (HCI_BT_ERROR_NO_ERROR != status) {
			/* Continue in case of error, the chip is going to be shut down anyway. */
			STE_CONN_ERR("CmdComplete for HciReset received with error 0x%X !", status);
		}

		cpd_create_work_item(cpd_work_power_off_chip, NULL, NULL);
		pkt_handled = true;

	}

	return pkt_handled;
}


#ifdef ENABLE_SYS_CLK_OUT
/**
 * cpd_handle_vs_read_register_cmd_complete_evt() - Handle a received HCI Command Complete event
 * for a VS ReadRegister command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cpd_handle_vs_read_register_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;

	if (cpd_info->boot_state == BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_HIGH) {
		uint8_t status = data[0];

		if (HCI_BT_ERROR_NO_ERROR == status) {
			/*Received good confirmation. Start work to continue.*/
			uint8_t data_out[(sizeof(cpd_msg_write_register_0x40014004))];
			memcpy(&data_out[0], cpd_msg_write_register_0x40014004, sizeof(cpd_msg_write_register_0x40014004));

			STE_CONN_INFO("Read register 0x40014004 value b1 0x%x", data[1]);
			STE_CONN_INFO("Read register 0x40014004 value b2 0x%x", data[2]);
			STE_CONN_INFO("Read register 0x40014004 value b3 0x%x", data[3]);
			STE_CONN_INFO("Read register 0x40014004 value b4 0x%x", data[4]);
			/*litle endian Set bit 12 to value 1*/
			data[2] = data[2] | 0x10;
			STE_CONN_INFO("Write register 0x40014004 value b1 0x%x", data[1]);
			STE_CONN_INFO("Write register 0x40014004 value b2 0x%x", data[2]);
			STE_CONN_INFO("Write register 0x40014004 value b3 0x%x", data[3]);
			STE_CONN_INFO("Write register 0x40014004 value b4 0x%x", data[4]);
			/*copy value to be written*/
			memcpy(&data_out[9], &data[1], 4);
			cpd_create_and_send_bt_cmd(&data_out[0], sizeof(cpd_msg_write_register_0x40014004));

		} else {
			STE_CONN_ERR("CmdComplete for ReadRegister received with error 0x%X", status);
			CPD_SET_BOOT_STATE(BOOT_STATE_FAILED);
			cpd_create_work_item(cpd_work_reset_after_error, NULL, NULL);
		}
		/*We have now handled the packet*/
		pkt_handled = true;

	} else if (cpd_info->boot_state == BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_LOW) {
			uint8_t status = data[0];

		if (HCI_BT_ERROR_NO_ERROR == status) {
			/*Received good confirmation. Start work to continue.*/
			uint8_t data_out[(sizeof(cpd_msg_write_register_0x40014004))];
			memcpy(&data_out[0], cpd_msg_write_register_0x40014004, sizeof(cpd_msg_write_register_0x40014004));

			STE_CONN_INFO("Read register 0x40014004 value b1 0x%x", data[1]);
			STE_CONN_INFO("Read register 0x40014004 value b2 0x%x", data[2]);
			STE_CONN_INFO("Read register 0x40014004 value b3 0x%x", data[3]);
			STE_CONN_INFO("Read register 0x40014004 value b4 0x%x", data[4]);
			/*litle endian Set bit 12 to value 0*/
			data[2] = data[2] & 0xEF;
			STE_CONN_INFO("Write register 0x40014004 value b1 0x%x", data[1]);
			STE_CONN_INFO("Write register 0x40014004 value b2 0x%x", data[2]);
			STE_CONN_INFO("Write register 0x40014004 value b3 0x%x", data[3]);
			STE_CONN_INFO("Write register 0x40014004 value b4 0x%x", data[4]);
			/*copy value to be written*/
			memcpy(&data_out[9], &data[1], 4);
			cpd_create_and_send_bt_cmd(&data_out[0], sizeof(cpd_msg_write_register_0x40014004));

		} else {
			STE_CONN_ERR("CmdComplete for ReadRegister received with error 0x%X", status);
			CPD_SET_BOOT_STATE(BOOT_STATE_FAILED);
			cpd_create_work_item(cpd_work_reset_after_error, NULL, NULL);
		}
		/*We have now handled the packet*/
		pkt_handled = true;
	}
	return pkt_handled;
}

/**
 * cpd_handle_vs_write_register_cmd_complete_evt() - Handle a received HCI Command Complete event
 * for a VS ReadRegister command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cpd_handle_vs_write_register_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;


	if (cpd_info->boot_state == BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_HIGH) {
		uint8_t status = data[0];
		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* Received good confirmation*/
			cpd_create_and_send_bt_cmd(cpd_msg_read_register_0x40014004,
						sizeof(cpd_msg_read_register_0x40014004));
			CPD_SET_BOOT_STATE(BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_LOW);
		} else {
			STE_CONN_ERR("CmdComplete for WriteRegister received with error 0x%X", status);
			CPD_SET_BOOT_STATE(BOOT_STATE_FAILED);
			cpd_create_work_item(cpd_work_reset_after_error, NULL, NULL);
		}
		/* We have now handled the packet */
		pkt_handled = true;


	} else 	if (cpd_info->boot_state == BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_LOW) {
		uint8_t status = data[0];
		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* Received good confirmation
			 * The boot sequence is now finished successfully
			 * Set states and signal to waiting thread.
			 */
			CPD_SET_BOOT_STATE(BOOT_STATE_READY);
			CPD_SET_MAIN_STATE(CPD_STATE_ACTIVE);
			wake_up_interruptible(&ste_conn_cpd_wait_queue);
		} else {
			STE_CONN_ERR("CmdComplete for WriteRegister received with error 0x%X", status);
			CPD_SET_BOOT_STATE(BOOT_STATE_FAILED);
			cpd_create_work_item(cpd_work_reset_after_error, NULL, NULL);
		}
		/* We have now handled the packet */
		pkt_handled = true;
	}

	return pkt_handled;
}
#endif /*ENABLE_SYS_CLK_OUT*/


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

	if (cpd_info->main_state  == CPD_STATE_BOOTING &&
	    cpd_info->boot_state == BOOT_STATE_READ_LOCAL_VERSION_INFORMATION) {
		/* We got an answer for our HCI command. Extract data */
		uint8_t status = data[0];

		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* The command worked. Store the data */
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

			/* Received good confirmation. Start work to continue. */
			CPD_SET_BOOT_STATE(BOOT_STATE_GET_FILES_TO_LOAD);
			cpd_create_work_item(cpd_work_load_patch_and_settings, NULL, NULL);

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
 * cpd_handle_vs_store_in_fs_cmd_complete_evt() - Handle a received HCI Command Complete event
 * for a VS StoreInFS command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cpd_handle_vs_store_in_fs_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;
	uint8_t status = data[0];

	STE_CONN_INFO("Received Store_in_FS complete event with status 0x%X", status);

	if (cpd_info->boot_state == BOOT_STATE_SEND_BD_ADDRESS) {
		if (HCI_BT_ERROR_NO_ERROR == status) {

			struct sk_buff *skb = NULL;

			/* Send HCI Reset command to activate patches */
			CPD_SET_BOOT_STATE(BOOT_STATE_ACTIVATE_PATCHES_AND_SETTINGS);

			/* Get HCI reset command to use based
			 * on connected connectivity controller. */
			skb = ste_conn_devices_get_reset_cmd(NULL, NULL);

			/* Transmit the received command */
			if (skb) {
				uint8_t *h4_header;

				STE_CONN_DBG("Got reset command, add H4 header and transmit");

				 /* Move the data pointer to the H:4 header position and store the H4 header */
				h4_header = skb_push(skb, STE_CONN_SKB_RESERVE);
				*h4_header = (uint8_t)cpd_info->h4_channels.bt_cmd_channel;

				cpd_transmit_skb_to_ccd(skb, cpd_info->hci_logger_config.bt_cmd_enable);
			} else {
				STE_CONN_DBG("No reset command found for this device");
				CPD_SET_BOOT_STATE(BOOT_STATE_READY);
				CPD_SET_MAIN_STATE(CPD_STATE_ACTIVE);
				wake_up_interruptible(&ste_conn_cpd_wait_queue);
			}

		} else {
			STE_CONN_ERR("CmdComplete for StoreInFS received with error 0x%X", status);
			CPD_SET_BOOT_STATE(BOOT_STATE_FAILED);
			cpd_create_work_item(cpd_work_reset_after_error, NULL, NULL);
		}
		/* We have now handled the packet */
		pkt_handled = true;
	}

	return pkt_handled;
}

/**
 * cpd_handle_vs_write_file_block_cmd_complete_evt() - Handle a received HCI Command Complete event
 * for a VS WriteFileBlock command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cpd_handle_vs_write_file_block_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;

	if ((cpd_info->boot_state == BOOT_STATE_DOWNLOAD_PATCH) &&
	    cpd_info->boot_download_state == BOOT_DOWNLOAD_STATE_PENDING) {
		uint8_t status = data[0];
		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* Received good confirmation. Start work to continue. */
			cpd_create_work_item(cpd_work_continue_with_file_download, NULL, NULL);
		} else {
			STE_CONN_ERR("CmdComplete for WriteFileBlock received with error 0x%X", status);
			CPD_SET_BOOT_DOWNLOAD_STATE(BOOT_DOWNLOAD_STATE_FAILED);
			CPD_SET_BOOT_STATE(BOOT_STATE_FAILED);
			if (cpd_info->fw_file) {
				release_firmware(cpd_info->fw_file);
				cpd_info->fw_file = NULL;
			}
			cpd_create_work_item(cpd_work_reset_after_error, NULL, NULL);
		}
		/* We have now handled the packet */
		pkt_handled = true;
	}

	return pkt_handled;
}

/**
 * cpd_handle_vs_power_switch_off_cmd_complete_evt() - Handle a received HCI Command Complete event
 * for a VS PowerSwitchOff command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cpd_handle_vs_power_switch_off_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;

	if (CLOSING_STATE_POWER_SWITCH_OFF == cpd_info->closing_state) {
		/* We were waiting for this but we don't need to do anything upon receival
		 * except warn for error status
		 */
		uint8_t status = data[0];

		if (HCI_BT_ERROR_NO_ERROR != status) {
			STE_CONN_ERR("CmdComplete for PowerSwitchOff received with error 0x%X", status);
		}

		pkt_handled = true;
	}

	return pkt_handled;
}

/**
 * cpd_handle_vs_system_reset_cmd_complete_evt() - Handle a received HCI Command Complete event for a VS SystemReset command.
 * @data: Pointer to received HCI data packet.
 *
 * Returns:
 *   True,  if packet was handled internally,
 *   False, otherwise.
 */
static bool cpd_handle_vs_system_reset_cmd_complete_evt(uint8_t *data)
{
	bool pkt_handled = false;
	uint8_t status = data[0];

	if (cpd_info->main_state == CPD_STATE_BOOTING &&
		cpd_info->boot_state == BOOT_STATE_ACTIVATE_PATCHES_AND_SETTINGS) {
#ifdef ENABLE_SYS_CLK_OUT
		STE_CONN_INFO("SYS_CLK_OUT Enabled");
		if (HCI_BT_ERROR_NO_ERROR == status) {
			cpd_create_and_send_bt_cmd(cpd_msg_read_register_0x40014004,
						sizeof(cpd_msg_read_register_0x40014004));
			CPD_SET_BOOT_STATE(BOOT_STATE_ACTIVATE_SYS_CLK_OUT_TOGGLE_HIGH);
		}
#else
		STE_CONN_INFO("SYS_CLK_OUT Disabled");
		if (HCI_BT_ERROR_NO_ERROR == status) {
			/* The boot sequence is now finished successfully.
			 * Set states and signal to waiting thread.
			 */
			CPD_SET_BOOT_STATE(BOOT_STATE_READY);
			CPD_SET_MAIN_STATE(CPD_STATE_ACTIVE);
			wake_up_interruptible(&ste_conn_cpd_wait_queue);
		}
#endif
		else {
			STE_CONN_ERR("Received Reset complete event with status 0x%X", status);
			CPD_SET_BOOT_STATE(BOOT_STATE_FAILED);
			ste_conn_reset(NULL);
		}
		pkt_handled = true;
	}
	return pkt_handled;
}

/** Transmit VS Power Switch Off
 * cpd_work_power_off_chip() - Work item to power off the chip.
 * @work: Reference to work data.
 *
 * The cpd_work_power_off_chip() function handles transmission of
 * the HCI command vs_power_switch_off command, closes the CCD,
 * and then powers off the chip.
 */
static void cpd_work_power_off_chip(struct work_struct *work)
{
	struct ste_conn_cpd_work_struct *current_work = NULL;
	struct sk_buff *skb = NULL;
	uint8_t *h4_header;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct ste_conn_cpd_work_struct, work);

	/* Get the VS Power Switch Off command to use based on
	 * connected connectivity controller */
	skb = ste_conn_devices_get_power_switch_off_cmd(NULL, NULL);

	/* Transmit the received command. If no command found for the device, just continue */
	if (skb) {
		STE_CONN_DBG("Got power_switch_off command, add H4 header and transmit");
		 /* Move the data pointer to the H:4 header position and store the H4 header */
		h4_header = skb_push(skb, STE_CONN_SKB_RESERVE);
		*h4_header = (uint8_t)cpd_info->h4_channels.bt_cmd_channel;

		CPD_SET_CLOSING_STATE(CLOSING_STATE_POWER_SWITCH_OFF);

		cpd_transmit_skb_to_ccd(skb, cpd_info->hci_logger_config.bt_cmd_enable);

		/* Mandatory to wait 500ms after the power_switch_off command has been
		 * transmitted, in order to make sure that the controller is ready. */
		schedule_timeout_interruptible(timeval_to_jiffies(&time_500ms));

	} else {
		STE_CONN_DBG("No power_switch_off command found for this device");
	}

	CPD_SET_CLOSING_STATE(CLOSING_STATE_GPIO_DEREGISTER);

	/* Close CCD, which will power off the chip */
	ste_conn_ccd_close();

	/* Chip shut-down finished, set correct state and wake up the cpd. */
	CPD_SET_MAIN_STATE(CPD_STATE_IDLE);
	wake_up_interruptible(&ste_conn_cpd_wait_queue);

	kfree(current_work);
}

/**
 * cpd_work_reset_after_error() - Handle reset.
 * @work: Reference to work data.
 *
 * Handle a reset after received command complete event.
 */
static void cpd_work_reset_after_error(struct work_struct *work)
{
	struct ste_conn_cpd_work_struct *current_work = NULL;
	struct sk_buff *skb = NULL;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct ste_conn_cpd_work_struct, work);
	skb = current_work->skb;

	ste_conn_reset(NULL);

	kfree(current_work);
}

/**
 * cpd_work_load_patch_and_settings() - Start loading patches and settings.
 * @work: Reference to work data.
 */
static void cpd_work_load_patch_and_settings(struct work_struct *work)
{
	struct ste_conn_cpd_work_struct *current_work = NULL;
	struct sk_buff *skb = NULL;
	int err = 0;
	bool file_found;
	const struct firmware *patch_info;
	const struct firmware *settings_info;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct ste_conn_cpd_work_struct, work);
	skb = current_work->skb;


	/* Check that we are in the right state */
	if (cpd_info->main_state  == CPD_STATE_BOOTING &&
	    cpd_info->boot_state == BOOT_STATE_GET_FILES_TO_LOAD) {
		/* Open patch info file. */
		err = request_firmware(&patch_info, STE_CONN_BT_PATCH_INFO_FILE, cpd_info->dev);
		if (err) {
			STE_CONN_ERR("Couldn't get patch info file");
			goto error_handling;
		}

		/* Now we have the patch info file. See if we can find the right patch file as well */
		file_found = cpd_get_file_to_load(patch_info, &(cpd_info->patch_file_name));

		/* Now we are finished with the patch info file */
		release_firmware(patch_info);

		if (!file_found) {
			STE_CONN_ERR("Couldn't find patch file! Major error!");
			goto error_handling;
		}

		/* Open settings info file. */
		err = request_firmware(&settings_info, STE_CONN_BT_FACTORY_SETTINGS_INFO_FILE, cpd_info->dev);
		if (err) {
			STE_CONN_ERR("Couldn't get settings info file");
			goto error_handling;
		}

		/* Now we have the settings info file. See if we can find the right settings file as well */
		file_found = cpd_get_file_to_load(settings_info, &(cpd_info->settings_file_name));

		/* Now we are finished with the patch info file */
		release_firmware(settings_info);

		if (!file_found) {
			STE_CONN_ERR("Couldn't find settings file! Major error!");
			goto error_handling;
		}

		/* We now all info needed */
		CPD_SET_BOOT_STATE(BOOT_STATE_DOWNLOAD_PATCH);
		CPD_SET_BOOT_DOWNLOAD_STATE(BOOT_DOWNLOAD_STATE_PENDING);
		CPD_SET_BOOT_FILE_LOAD_STATE(BOOT_FILE_LOAD_STATE_LOAD_PATCH);
		cpd_info->chunk_id = 0;
		cpd_info->fw_file_rd_offset = 0;
		cpd_info->fw_file = NULL;

		/* OK. Now it is time to download the patches */
		err = request_firmware(&(cpd_info->fw_file), cpd_info->patch_file_name, cpd_info->dev);
		if (err < 0) {
			STE_CONN_ERR("Couldn't get patch file");
			goto error_handling;
		}
		cpd_send_patch_file();
	}
	goto finished;

error_handling:
	CPD_SET_BOOT_STATE(BOOT_STATE_FAILED);
	ste_conn_reset(NULL);
finished:
	kfree(current_work);
}

/**
 * cpd_work_continue_with_file_download() - A file block has been written.
 * @work: Reference to work data.
 *
 * Handle a received HCI VS Write File Block Complete event.
 * Normally this means continue to send files to the controller.
 */
static void cpd_work_continue_with_file_download(struct work_struct *work)
{
	struct ste_conn_cpd_work_struct *current_work = NULL;
	struct sk_buff *skb = NULL;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct ste_conn_cpd_work_struct, work);
	skb = current_work->skb;

	/* Continue to send patches or settings to the controller */
	if (cpd_info->boot_file_load_state == BOOT_FILE_LOAD_STATE_LOAD_PATCH) {
		cpd_send_patch_file();
	} else if (cpd_info->boot_file_load_state == BOOT_FILE_LOAD_STATE_LOAD_STATIC_SETTINGS) {
		cpd_send_settings_file();
	} else {
		STE_CONN_INFO("No more files to load");
	}

	kfree(current_work);
}

/**
 * cpd_get_file_to_load() - Parse info file and find correct target file.
 * @fw:              Firmware structure containing file data.
 * @file_name: (out) Pointer to name of requested file.
 *
 * Returns:
 *   True,  if target file was found,
 *   False, otherwise.
 */
static bool cpd_get_file_to_load(const struct firmware *fw, char **file_name)
{
	char *line_buffer;
	char *curr_file_buffer;
	int bytes_left_to_parse = fw->size;
	int bytes_read = 0;
	bool file_found = false;

	curr_file_buffer = (char *)&(fw->data[0]);

	line_buffer = kzalloc(STE_CONN_LINE_BUFFER_LENGTH, GFP_KERNEL);

	if (line_buffer) {
		while (!file_found) {
			/* Get one line of text from the file to parse */
			curr_file_buffer = cpd_get_one_line_of_text(line_buffer,
								    min(STE_CONN_LINE_BUFFER_LENGTH,
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
				    && hci_rev == cpd_info->local_chip_info.hci_revision
				    && lmp_sub == cpd_info->local_chip_info.lmp_pal_subversion) {
					STE_CONN_DBG( \
					"File name = %s HCI Revision = 0x%X LMP PAL Subversion = 0x%X", \
					*file_name, hci_rev, lmp_sub);

					/* Name has already been stored above. Nothing more to do */
					file_found = true;
				} else {
					/* Zero the name buffer so it is clear to next read */
					memset(*file_name, 0x00, STE_CONN_FILENAME_MAX + 1);
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
 * cpd_get_one_line_of_text()- Replacement function for stdio function fgets.
 * @wr_buffer:         Buffer to copy text to.
 * @max_nbr_of_bytes: Max number of bytes to read, i.e. size of rd_buffer.
 * @rd_buffer:        Data to parse.
 * @bytes_copied:     Number of bytes copied to wr_buffer.
 *
 * The cpd_get_one_line_of_text() function extracts one line of text from input file.
 *
 * Returns:
 *   Pointer to next data to read.
 */
static char *cpd_get_one_line_of_text(char *wr_buffer, int max_nbr_of_bytes,
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
 * cpd_read_and_send_file_part() - Transmit a part of the supplied file to the controller
 *
 * The cpd_read_and_send_file_part() function transmit a part of the supplied file to the controller.
 * If nothing more to read, set the correct states.
 */
static void cpd_read_and_send_file_part(void)
{
	int bytes_to_copy;

	/* Calculate number of bytes to copy;
	 * either max bytes for HCI packet or number of bytes left in file
	 */
	bytes_to_copy = min((int)CPD_SEND_FILE_MAX_CHUNK_SIZE,
						(int)(cpd_info->fw_file->size - cpd_info->fw_file_rd_offset));

	if (bytes_to_copy > 0) {
		struct sk_buff *skb;

		/* There are bytes to transmit. Allocate a sk_buffer. */
		skb = alloc_skb(sizeof(cpd_msg_vs_write_file_block_cmd_req) +
						CPD_SEND_FILE_MAX_CHUNK_SIZE +
						CPD_FILE_CHUNK_ID_SIZE,
						GFP_KERNEL);
		if (!skb) {
			STE_CONN_ERR("Couldn't allocate sk_buffer");
			CPD_SET_BOOT_STATE(BOOT_STATE_FAILED);
			ste_conn_reset(NULL);
			return;
		}

		/* Copy the data from the HCI template */
		memcpy(skb_put(skb, sizeof(cpd_msg_vs_write_file_block_cmd_req)),
		       cpd_msg_vs_write_file_block_cmd_req,
		       sizeof(cpd_msg_vs_write_file_block_cmd_req));
		skb->data[0] = (uint8_t)cpd_info->h4_channels.bt_cmd_channel;

		/* Store the chunk ID */
		skb->data[CPD_VS_SEND_FILE_START_OFFSET_IN_CMD] = cpd_info->chunk_id;
		skb_put(skb, 1);

		/* Copy the data from offset position and store the length */
		memcpy(skb_put(skb, bytes_to_copy),
		       &(cpd_info->fw_file->data[cpd_info->fw_file_rd_offset]),
		       bytes_to_copy);
		skb->data[CPD_BT_CMD_LENGTH_POSITION] = bytes_to_copy + CPD_FILE_CHUNK_ID_SIZE;

		/* Increase offset with number of bytes copied */
		cpd_info->fw_file_rd_offset += bytes_to_copy;

		cpd_transmit_skb_to_ccd(skb, cpd_info->hci_logger_config.bt_cmd_enable);
		cpd_info->chunk_id++;
	} else {
		/* Nothing more to read in file. */
		CPD_SET_BOOT_DOWNLOAD_STATE(BOOT_DOWNLOAD_STATE_SUCCESS);
		cpd_info->chunk_id = 0;
		cpd_info->fw_file_rd_offset = 0;
	}
}

/**
 * cpd_send_patch_file - Transmit patch file.
 *
 * The cpd_send_patch_file() function transmit patch file. The file is
 * read in parts to fit in HCI packets. When the complete file is transmitted,
 * the file is closed. When finished, continue with settings file.
 */
static void cpd_send_patch_file(void)
{
	int err = 0;

	/* Transmit a part of the supplied file to the controller.
	 * When nothing more to read, continue to close the patch file. */
	cpd_read_and_send_file_part();

	if (cpd_info->boot_download_state == BOOT_DOWNLOAD_STATE_SUCCESS) {
		/* Patch file finished. Release used resources */
		STE_CONN_DBG("Patch file finished, release used resources");
		if (cpd_info->fw_file) {
			release_firmware(cpd_info->fw_file);
			cpd_info->fw_file = NULL;
		}
		err = request_firmware(&(cpd_info->fw_file), cpd_info->settings_file_name, cpd_info->dev);
		if (err < 0) {
			STE_CONN_ERR("Couldn't get settings file (%d)", err);
			goto error_handling;
		}
		/* Now send the settings file */
		CPD_SET_BOOT_FILE_LOAD_STATE(BOOT_FILE_LOAD_STATE_LOAD_STATIC_SETTINGS);
		CPD_SET_BOOT_DOWNLOAD_STATE(BOOT_DOWNLOAD_STATE_PENDING);
		cpd_send_settings_file();
	}
	return;

error_handling:
	CPD_SET_BOOT_STATE(BOOT_STATE_FAILED);
	ste_conn_reset(NULL);
}

/**
 * cpd_send_settings_file() - Transmit settings file.
 *
 * The cpd_send_settings_file() function transmit settings file.
 * The file is read in parts to fit in HCI packets. When finished,
 * close the settings file and send HCI reset to activate settings and patches.
 */
static void cpd_send_settings_file(void)
{
	/* Transmit a file part */
	cpd_read_and_send_file_part();

	if (cpd_info->boot_download_state == BOOT_DOWNLOAD_STATE_SUCCESS) {

		/* Settings file finished. Release used resources */
		STE_CONN_DBG("Settings file finished, release used resources");
		if (cpd_info->fw_file) {
			release_firmware(cpd_info->fw_file);
			cpd_info->fw_file = NULL;
		}

		CPD_SET_BOOT_FILE_LOAD_STATE(BOOT_FILE_LOAD_STATE_NO_MORE_FILES);

		/* Create and send HCI VS Store In FS cmd with bd address. */
		cpd_send_bd_address();
	}
}

/**
 * cpd_create_and_send_bt_cmd() - Copy and send sk_buffer.
 * @data:   Data to send.
 * @length: Length in bytes of data.
 *
 * The cpd_create_and_send_bt_cmd() function allocate sk_buffer, copy supplied data to it,
 * and send the sk_buffer to CCD. Note that the data must contain the H:4 header.
 */
static void cpd_create_and_send_bt_cmd(const uint8_t *data, int length)
{
	struct sk_buff *skb;

	skb = alloc_skb(length, GFP_KERNEL);
	if (skb) {
		memcpy(skb_put(skb, length), data, length);
		skb->data[0] = (uint8_t)cpd_info->h4_channels.bt_cmd_channel;

		STE_CONN_INFO("Sent %d bytes: 0x %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
						skb->len, skb->data[0], skb->data[1],
						skb->data[2], skb->data[3], skb->data[4],
						skb->data[5], skb->data[6], skb->data[7],
						skb->data[8], skb->data[9],
						skb->data[10], skb->data[11], skb->data[12],
						skb->data[13], skb->data[14], skb->data[15]);

		cpd_transmit_skb_to_ccd(skb, cpd_info->hci_logger_config.bt_cmd_enable);
	} else {
		STE_CONN_ERR("Couldn't allocate sk_buff with length %d", length);
	}
}

/**
 * cpd_send_bd_address() - Send HCI VS cmd with BD address to the chip.
 */
static void cpd_send_bd_address(void)
{
	uint8_t tmp[sizeof(cpd_msg_vs_store_in_fs_cmd_req) + BT_BDADDR_SIZE];

	/* Send vs_store_in_fs_cmd with BD address. */
	memcpy(tmp,	cpd_msg_vs_store_in_fs_cmd_req,
			sizeof(cpd_msg_vs_store_in_fs_cmd_req));

	tmp[HCI_CMD_PARAM_LEN_POS] = HCI_VS_STORE_IN_FS_USR_ID_SIZE +
		HCI_VS_STORE_IN_FS_PARAM_LEN_SIZE + BT_BDADDR_SIZE;
	tmp[HCI_VS_STORE_IN_FS_USR_ID_POS] = HCI_VS_STORE_IN_FS_USR_ID_BD_ADDR;
	tmp[HCI_VS_STORE_IN_FS_PARAM_LEN_POS] = BT_BDADDR_SIZE;

	/* Now copy the bd address received from user space control app. */
	memcpy(&(tmp[HCI_VS_STORE_IN_FS_PARAM_POS]), ste_conn_bd_address, sizeof(ste_conn_bd_address));

	CPD_SET_BOOT_STATE(BOOT_STATE_SEND_BD_ADDRESS);

	cpd_create_and_send_bt_cmd(tmp, sizeof(tmp));
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
		struct sk_buff *skb_log = alloc_skb(skb->len + 1, GFP_KERNEL);
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
 * cpd_transmit_skb_to_ccd_with_flow_ctrl() - Check the h4_header and call appropriate function to handle the skb.
 * @skb:        Data packet.
 * @dev: pointer to ste_conn_device struct.
 * @h4_header: H4 channel id.
 *
 * The cpd_transmit_skb_to_ccd_with_flow_ctrl() Checks the h4_header and call appropriate function to handle the skb.
 */
static void cpd_transmit_skb_to_ccd_with_flow_ctrl(struct sk_buff *skb,
		struct ste_conn_device *dev, const uint8_t h4_header)
{
	if (cpd_info->h4_channels.bt_cmd_channel == h4_header) {
		cpd_transmit_skb_to_ccd_with_flow_ctrl_bt(skb, dev);
	} else if (cpd_info->h4_channels.fm_radio_channel == h4_header) {
		cpd_transmit_skb_to_ccd_with_flow_ctrl_fm(skb, dev);
	}
}

/**
 * cpd_transmit_skb_to_ccd_with_flow_ctrl_bt() - Send the BT skb to the CCD if it is allowed or queue it.
 * @skb:        Data packet.
 * @dev: pointer to ste_conn_device struct.
 *
 * The cpd_transmit_skb_to_ccd_with_flow_ctrl_bt() function checks if there are tickets available and if so
 * transmits buffer to CCD. Otherwise the skb and user name is stored in a list for later sending.
 * If enabled, copy the transmitted data to the HCI logger as well.
 */
static void cpd_transmit_skb_to_ccd_with_flow_ctrl_bt(struct sk_buff *skb,
		struct ste_conn_device *dev)
{
	/* Because there are more users of some H4 channels (currently audio application
	 * for BT cmd and FM channel) we need to have an internal HCI cmd flow control
	 * in ste_conn driver. So check here how many tickets we have and store skb in a queue
	 * if there are no tickets left. The skb will be sent later when we get more ticket(s). */
	mutex_lock(&cpd_info->tx_bt_mutex);

	if ((cpd_info->tx_nr_pkts_allowed_bt - cpd_info->tx_nr_outstanding_cmds_bt) > 0) {
		(cpd_info->tx_nr_pkts_allowed_bt)--;
		(cpd_info->tx_nr_outstanding_cmds_bt)++;
		STE_CONN_DBG("New tx_nr_pkts_allowed_bt = %d", cpd_info->tx_nr_pkts_allowed_bt);
		STE_CONN_DBG("New tx_nr_outstanding_cmds_bt = %d", cpd_info->tx_nr_outstanding_cmds_bt);

		/* If it's cmd from audio app store the OpCode,
		 * it'll be used later to decide where to dispatch cmd cmpl evt. */
		if (cpd_info->users.bt_audio == dev) {
			cpd_info->hci_audio_cmd_opcode_bt = (uint16_t)(skb->data[1] | (skb->data[2] << 8));
			STE_CONN_INFO("Sending cmd from audio driver, saving OpCode = 0x%x",
				cpd_info->hci_audio_cmd_opcode_bt);
		}

		cpd_transmit_skb_to_ccd(skb, dev->logger_enabled);
	} else {
		struct tx_list_item *item = kzalloc(sizeof(*item), GFP_KERNEL);
		STE_CONN_INFO("Not allowed to send cmd to CCD, storing in tx queue.");
		if (item) {
			item->dev = dev;
			item->skb = skb;
			list_add_tail(&item->list, &cpd_info->tx_list_bt);
		} else {
			STE_CONN_ERR("Failed to alloc memory!");
		}
	}
	mutex_unlock(&cpd_info->tx_bt_mutex);
}

/**
 * cpd_transmit_skb_to_ccd_with_flow_ctrl_fm() - Send the FM skb to the CCD if it is allowed or queue it.
 * @skb:		Data packet.
 * @dev: pointer to ste_conn_device struct.
 *
 * The cpd_transmit_skb_to_ccd_with_flow_ctrl_fm() function checks if there are tickets available and if so
 * transmits buffer to CCD. Otherwise the skb and user name is stored in a list for later sending.
 * Also it updates the FM radio mode if it's FM GOTOMODE cmd, this is needed to
 * know how to handle some FM do-commands cmpl events.
 * If enabled, copy the transmitted data to the HCI logger as well.
 */
static void cpd_transmit_skb_to_ccd_with_flow_ctrl_fm(struct sk_buff *skb,
		struct ste_conn_device *dev)
{
	uint16_t cmd_id = (uint16_t)(((skb->data[5] | (skb->data[6] << 8)) & 0xFFF8) >> 3);

	if (CPD_FM_DO_GEN_GOTOMODE == cmd_id) {
		cpd_info->fm_radio_mode = skb->data[7];
		STE_CONN_INFO("FM Radio mode changed to %d", cpd_info->fm_radio_mode);
	}

	/* Because there are more users of some H4 channels (currently audio application
	 * for BT cmd and FM channel) we need to have an internal HCI cmd flow control
	 * in ste_conn driver. So check here how many tickets we have and store skb in a queue
	 * there are no tickets left. The skb will be sent later when we get more ticket(s). */
	mutex_lock(&cpd_info->tx_fm_mutex);

	if (cpd_info->tx_nr_pkts_allowed_fm) {
		(cpd_info->tx_nr_pkts_allowed_fm)--;
		STE_CONN_DBG("FM Nr_of_pckts_allowed changed to %d", cpd_info->tx_nr_pkts_allowed_fm);

		/* If it's cmd from audio app store the cmd id (bits 15-3),
		 * it'll be used later to decide where to dispatch cmd cmpl evt. */
		if (cpd_info->users.fm_radio_audio == dev) {
			cpd_info->hci_audio_fm_cmd_id = cmd_id;
		}

		cpd_transmit_skb_to_ccd(skb, dev->logger_enabled);
	} else {
		struct tx_list_item *item = kzalloc(sizeof(*item), GFP_KERNEL);
		STE_CONN_INFO("Not allowed to send cmd to CCD, storing in tx queue.");
		if (item) {
			item->dev = dev;
			item->skb = skb;
			list_add_tail(&item->list, &cpd_info->tx_list_fm);
		} else {
			STE_CONN_ERR("Failed to alloc memory!");
		}
	}
	mutex_unlock(&cpd_info->tx_fm_mutex);
}


/**
 * cpd_transmit_skb_from_tx_queue_bt() - Check/update flow control info and transmit skb from
 * the BT tx_queue to CCD if allowed.
 *
 * The cpd_transmit_skb_from_tx_queue_bt() function checks if there are tickets available and cmds waiting in the tx queue
 * and if so transmits them to CCD.
 */
static void cpd_transmit_skb_from_tx_queue_bt(void)
{
	struct list_head *cursor, *next;
	struct tx_list_item *tmp;
	struct ste_conn_device *dev;
	struct sk_buff *skb;

	STE_CONN_INFO("cpd_transmit_skb_from_tx_queue_bt");

	mutex_lock(&cpd_info->tx_bt_mutex);

	list_for_each_safe(cursor, next, &cpd_info->tx_list_bt) {
		tmp = list_entry(cursor, struct tx_list_item, list);
		skb = tmp->skb;
		dev = tmp->dev;

		if (dev) {
			if ((cpd_info->tx_nr_pkts_allowed_bt - cpd_info->tx_nr_outstanding_cmds_bt) > 0) {

				(cpd_info->tx_nr_pkts_allowed_bt)--;
				(cpd_info->tx_nr_outstanding_cmds_bt)++;
				STE_CONN_DBG("New tx_nr_pkts_allowed_bt = %d", cpd_info->tx_nr_pkts_allowed_bt);
				STE_CONN_DBG("New tx_nr_outstanding_cmds_bt = %d", cpd_info->tx_nr_outstanding_cmds_bt);

				/* If it's cmd from audio app store the OpCode,
				 * it'll be used later to decide where to dispatch cmd cmpl evt. */
				if (cpd_info->users.bt_audio == dev) {
					cpd_info->hci_audio_cmd_opcode_bt = (uint16_t)(skb->data[1] | (skb->data[2] << 8));
					STE_CONN_INFO("Sending cmd from audio driver, saving OpCode = 0x%x",
						cpd_info->hci_audio_cmd_opcode_bt);
				}

				cpd_transmit_skb_to_ccd(skb, dev->logger_enabled);
				list_del(cursor);
				kfree(tmp);
			} else {
				/* If no more pckts allowed just return, we'll get back here after next cmd cmpl/cmd status evt. */
				return;
			}
		} else {
			STE_CONN_ERR("H4 user not found!");
			kfree_skb(skb);
		}
	}

	mutex_unlock(&cpd_info->tx_bt_mutex);
}

/**
 * cpd_transmit_skb_from_tx_queue_fm() - Check/update flow control info and transmit skb from
 * the FM tx_queue to CCD if allowed.
 *
 * The cpd_transmit_skb_from_tx_queue_fm() function checks if there are tickets available and cmds waiting in the tx queue
 * and if so transmits them to CCD.
 */
static void cpd_transmit_skb_from_tx_queue_fm(void)
{
	struct list_head *cursor, *next;
	struct tx_list_item *tmp;
	struct ste_conn_device *dev;
	struct sk_buff *skb;

	STE_CONN_INFO("cpd_transmit_skb_from_tx_queue_fm");

	mutex_lock(&cpd_info->tx_fm_mutex);

	list_for_each_safe(cursor, next, &cpd_info->tx_list_fm) {
		tmp = list_entry(cursor, struct tx_list_item, list);
		skb = tmp->skb;
		dev = tmp->dev;

		if (dev) {
			if (cpd_info->tx_nr_pkts_allowed_fm) {
				(cpd_info->tx_nr_pkts_allowed_fm)--;
				STE_CONN_DBG("New tx_nr_pkts_allowed_fm = %d", cpd_info->tx_nr_pkts_allowed_fm);

				/* If it's cmd from audio app store the cmd id (bits 15-3),
				 * it'll be used later to decide where to dispatch cmd cmpl evt. */
				if (cpd_info->users.fm_radio_audio == dev) {
					uint16_t cmd_id = (uint16_t)(((skb->data[5] | (skb->data[6] << 8)) & 0xFFF8) >> 3);
					cpd_info->hci_audio_fm_cmd_id = cmd_id;
				}

				cpd_transmit_skb_to_ccd(skb, dev->logger_enabled);
				list_del(cursor);
				kfree(tmp);
			} else {
				/* If no more pckts allowed just return, we'll get back here after next cmd cmpl/cmd status evt. */
				return;
			}
		} else {
			STE_CONN_ERR("H4 user not found!");
			kfree_skb(skb);
		}
	}

	mutex_unlock(&cpd_info->tx_fm_mutex);
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


EXPORT_SYMBOL(ste_conn_register);
EXPORT_SYMBOL(ste_conn_deregister);
EXPORT_SYMBOL(ste_conn_reset);
EXPORT_SYMBOL(ste_conn_alloc_skb);
EXPORT_SYMBOL(ste_conn_write);

MODULE_AUTHOR("Par-Gunnar Hjalmdahl ST-Ericsson");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Linux Bluetooth HCI H:4 Connectivity Device Driver ver " VERSION);
MODULE_VERSION(VERSION);
