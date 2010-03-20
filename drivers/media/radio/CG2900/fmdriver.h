/*
 * file fmdriver.h
 *
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Linux FM Driver for CG2900 FM Chip
 *
 * Author: Hemant Gupta/hemant.gupta@stericsson.com for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2
 */

#ifndef _FMDRIVER_H_
#define _FMDRIVER_H_

#define IRPT_OPERATIONSUCCEEDED         0
#define IRPT_OPERATIONFAILED            1
#define IRPT_BUFFERFULL                 3
#define IRPT_SIGNALQUALITYLOW           4
#define IRPT_MONOSTEREOTRANSITION       5
#define IRPT_RDSSYNCFOUND               6
#define IRPT_RDSSYNCLOST                7
#define IRPT_PICODECHANGED              8
#define IRPT_REQUESTEDBLOCKAVAILABLE    9
#define IRPT_BUFFER_CLEARED            13
#define IRPT_WARMBOOTREADY             14
#define IRPT_COLDBOOTREADY             15
#define IRPT_BUFFEREMPTY                3
#define IRPT_MUTESTATUSCHANGED          4
#define IRPT_OVERMODULATION             5
#define IRPT_INPUTOVERDRIVE             6

#define CMD_AIP_BT_SETCONTROL			0x01A6
#define CMD_AIP_BT_SETMODE			0x01E6
#define CMD_AIP_FADE_START			0x0046
#define CMD_AIP_LEVEL_GETSTATUS			0x0106
#define CMD_AIP_LEVEL_SETMODE			0x00E6
#define CMD_AIP_OVERDRIVE_SETCONTROL		0x0166
#define CMD_AIP_OVERDRIVE_SETMODE		0x0126
#define CMD_AIP_SETAUDIOADC			0x00A6
#define CMD_AIP_SETAUDIOADC_CONTROL		0x00C6
#define CMD_AIP_SETBALANCE			0x0086
#define CMD_AIP_SETGAIN				0x0066
#define CMD_AIP_SETMIXER			0x0146
#define CMD_AIP_SETMODE				0x01C6
#define CMD_AUP_BT_SETBALANCE			0x0142
#define CMD_AUP_BT_SETCONTROL			0x00E2
#define CMD_AUP_BT_SETMODE			0x00C2
#define CMD_AUP_BT_SETVOLUME			0x0122
#define CMD_AUP_EXT_SETCONTROL			0x0182
#define CMD_AUP_EXT_SETMODE			0x0162
#define CMD_AUP_EXT_FADESTART			0x0102
#define CMD_AUP_EXT_SETMUTE			0x01E2
#define CMD_AUP_FADE_START			0x00A2
#define CMD_AUP_SETAUDIODAC			0x0082
#define CMD_AUP_SETBALANCE			0x0042
#define CMD_AUP_SETMUTE				0x0062
#define CMD_AUP_SETVOLUME			0x0022
#define CMD_FMR_DP_BLOCKREQUEST_CONTROL		0x0583
#define CMD_FMR_DP_BLOCKREQUEST_GETBLOCK	0x05A3
#define CMD_FMR_DP_BUFFER_GETGROUP		0x0303
#define CMD_FMR_DP_BUFFER_GETGROUPCOUNT		0x0323
#define CMD_FMR_DP_BUFFER_SETSIZE		0x0343
#define CMD_FMR_DP_BUFFER_SETTHRESHOLD		0x06C3
#define CMD_FMR_DP_GETSTATE			0x0283
#define CMD_FMR_DP_PICORRELATION_SETCONTROL	0x0363
#define CMD_FMR_DP_QUALITY_GETRESULT		0x05E3
#define CMD_FMR_DP_QUALITY_START		0x05C3
#define CMD_FMR_DP_SETCONTROL			0x02A3
#define CMD_FMR_DP_SETEBLOCKREJECTION		0x02E3
#define CMD_FMR_DP_SETGROUPREJECTION		0x0543
#define CMD_FMR_GETSTATE			0x04E3
#define CMD_FMR_RP_GETRSSI			0x0083
#define CMD_FMR_RP_GETSTATE			0x0063
#define CMD_FMR_RP_HIGHCUT_SETCONTROL		0x0263
#define CMD_FMR_RP_HIGHCUT_SETMODE		0x0243
#define CMD_FMR_RP_SETDEEMPHASIS		0x00C3
#define CMD_FMR_RP_SETRSSIQUALITYTHRESHOLD	0x00A3
#define CMD_FMR_RP_SOFTMUTE_SETCONTROL		0x0223
#define CMD_FMR_RP_SOFTMUTE_SETMODE		0x0203
#define CMD_FMR_RP_STEREO_SETCONTROL_BLENDINGPILOT 0x0163
#define CMD_FMR_RP_STEREO_SETCONTROL_BLENDINGRSSI 0x0143
#define CMD_FMR_RP_STEREO_SETMODE		0x0123
#define CMD_FMR_RP_THRESHOLDEXTENSION_SETCONTROL 0x0103
#define CMD_FMR_RP_THRESHOLDEXTENSION_SETMODE	0x00E3
#define CMD_FMR_RP_XSTEREO_SETCONTROL		0x01E3
#define CMD_FMR_RP_XSTEREO_SETMODE		0x01C3
#define CMD_FMR_SETANTENNA			0x0663
#define CMD_FMR_SETCOEXFILTER			0x06E3
#define CMD_FMR_SETPROCESSINGMODE		0x0643
#define CMD_FMR_SP_AFSWITCH_GETRESULT		0x0603
#define CMD_FMR_SP_AFSWITCH_START		0x04A3
#define CMD_FMR_SP_AFUPDATE_GETRESULT		0x0483
#define CMD_FMR_SP_AFUPDATE_START		0x0463
#define CMD_FMR_SP_BLOCKSCAN_GETRESULT			0x06A3
#define CMD_FMR_SP_BLOCKSCAN_START			0x0683
#define CMD_FMR_SP_PRESETPI_GETRESULT			0x0623
#define CMD_FMR_SP_PRESETPI_START			0x0443
#define CMD_FMR_SP_SCAN_GETRESULT			0x0423
#define CMD_FMR_SP_SCAN_START				0x0403
#define CMD_FMR_SP_SEARCH_START				0x03E3
#define CMD_FMR_SP_SEARCHPI_START			0x0703
#define CMD_FMR_SP_STOP					0x0383
#define CMD_FMR_SP_TUNE_GETCHANNEL			0x03A3
#define CMD_FMR_SP_TUNE_SETCHANNEL			0x03C3
#define CMD_FMR_SP_TUNE_STEPCHANNEL			0x04C3
#define CMD_FMR_TN_SETBAND				0x0023
#define CMD_FMR_TN_SETGRID				0x0043
#define CMD_FMR_TN_SETINJECTIONMODE			0x0523
#define CMD_FMR_TN_SETINJECTIONMODEAFUPDATE		0x0563
#define CMD_FMT_DP_BUFFER_GETPOSITION			0x0204
#define CMD_FMT_DP_BUFFER_SETGROUP			0x0244
#define CMD_FMT_DP_BUFFER_SETSIZE			0x0224
#define CMD_FMT_DP_BUFFER_SETTHRESHOLD			0x0284
#define CMD_FMT_DP_SETCONTROL				0x0264
#define CMD_FMT_GETSTATE				0x0084
#define CMD_FMT_PA_SETCONTROL				0x01A4
#define CMD_FMT_PA_SETMODE				0x01E4
#define CMD_FMT_RP_LIMITER_SETCONTROL			0x01C4
#define CMD_FMT_RP_LIMITER_SETMODE			0x0124
#define CMD_FMT_RP_LIMITER_SETTIMING			0x02C4
#define CMD_FMT_RP_MUTE_SETCONTROL			0x0104
#define CMD_FMT_RP_MUTE_SETCONTROL_BEEP			0x0304
#define CMD_FMT_RP_MUTE_SETMODE				0x00E4
#define CMD_FMT_RP_SETPILOTDEVIATION			0x02A4
#define CMD_FMT_RP_SETPREEMPHASIS			0x00C4
#define CMD_FMT_RP_SETRDSDEVIATION			0x0344
#define CMD_FMT_RP_STEREO_SETMODE			0x0164
#define CMD_FMT_SETCOEXFILTER				0x02E4
#define CMD_FMT_SP_TUNE_GETCHANNEL			0x0184
#define CMD_FMT_SP_TUNE_SETCHANNEL			0x0064
#define CMD_FMT_TN_DUALCHANNEL_SETCONTROL		0x0384
#define CMD_FMT_TN_DUALCHANNEL_SETMODE			0x0364
#define CMD_FMT_TN_SETBAND				0x0024
#define CMD_FMT_TN_SETGRID				0x0044
#define CMD_FMT_TN_SETINJECTIONMODE			0x0144
#define CMD_GEN_ANTENNA_GETINFO				0x02C1
#define CMD_GEN_ANTENNACHECK_START			0x02A1
#define CMD_GEN_COHABITATION_ASK			0x01E1
#define CMD_GEN_COHABITATION_SETMODE			0x01C1
#define CMD_GEN_COMMANDINVALID				0x1001
#define CMD_GEN_GETMODE					0x0021
#define CMD_GEN_GETREGISTERVALUE			0x00E1
#define CMD_GEN_GETVERSION				0x00C1
#define CMD_GEN_GOTOMODE				0x0041
#define CMD_GEN_GOTOPOWERDOWN				0x0081
#define CMD_GEN_GOTOSTANDBY				0x0061
#define CMD_GEN_POWERUP					0x0141
#define CMD_GEN_PARAMETERINVALID			0x0FE1
#define CMD_GEN_POWERSUPPLY_SETMODE			0x0221
#define CMD_GEN_SELECTREFERENCECLOCK			0x0201
#define CMD_GEN_SETPROCESSINGCLOCK			0x0241
#define CMD_GEN_SETPROCESSPARAMETERS			0x0261
#define CMD_GEN_SETPRODUCTIONTESTMODE			0x0181
#define CMD_GEN_SETREFERENCECLOCK			0x0161
#define CMD_GEN_SETREFERENCECLOCKPLL			0x01A1
#define CMD_GEN_SETREGISTERVALUE			0x0101
#define CMD_GEN_SETTEMPERATURE				0x0281
#define CMD_TST_FPGA_SETDELTAPHI			0x0127
#define CMD_TST_FPGA_SETFSCOR				0x0107
#define CMD_TST_FPGA_SETRSSI				0x00E7
#define CMD_TST_STRESS_START				0x00A7
#define CMD_TST_STRESS_STOP				0x00C7
#define CMD_TST_TONE_CONNECT				0x0047
#define CMD_TST_TONE_ENABLE				0x0027
#define CMD_TST_TONE_GETFSCOR				0x0087
#define CMD_TST_TONE_SETPAR				0x0067
#define CMD_TST_TX_RAMP_START				0x0147

/* Error/return codes  */
#define FMD_RESULT_SUCCESS				0x00
#define FMD_RESULT_PRECONDITIONS_VIOLATED		0x01
#define FMD_RESULT_PARAMETER_INVALID			0x02
#define FMD_RESULT_IO_ERROR				0x04
#define FMD_RESULT_FEATURE_UNSUPPORTED			0x05
#define FMD_RESULT_BUSY					0x06
#define FMD_RESULT_ONGOING				0x07
#define FMD_RESULT_WAIT					0x08
#define FMD_RESULT_RESPONSE_WRONG			0x09

/* Frequency ranges  */
#define FMD_FREQRANGE_FMEUROAMERICA                 ((uint8_t) 0x01)
#define FMD_FREQRANGE_FMJAPAN                       ((uint8_t) 0x02)
#define FMD_FREQRANGE_FMCHINA                       ((uint8_t) 0x03)

/* Callback event codes  */
#define FMD_EVENT_ANTENNA_STATUS_CHANGED      ((uint32_t) 0x00000001)
#define FMD_EVENT_FREQUENCY_CHANGED           ((uint32_t) 0x00000002)
#define FMD_EVENT_FREQUENCY_RANGE_CHANGED     ((uint32_t) 0x00000003)
#define FMD_EVENT_PRESET_CHANGED              ((uint32_t) 0x00000004)
#define FMD_EVENT_SEEK_COMPLETED              ((uint32_t) 0x00000005)
#define FMD_EVENT_SCAN_BAND_COMPLETED         ((uint32_t) 0x00000006)
#define FMD_EVENT_ACTION_FINISHED             ((uint32_t) 0x00008001)
#define FMD_EVENT_RDSGROUP_RCVD               ((uint32_t) 0x00008002)
#define FMD_EVENT_SEEK_STOPPED		   ((uint32_t) 0x00008003)
#define FMD_EVENT_GEN_POWERUP                 ((uint32_t) 0x00008004)

/* Stereo modes  */
#define FMD_STEREOMODE_MONO                         ((uint32_t) 0x00000000)
#define FMD_STEREOMODE_STEREO                       ((uint32_t) 0x00000001)
#define FMD_STEREOMODE_AUTO                         ((uint32_t) 0x00000002)

/* Radio modes  */
#define FMD_MODE_IDLE	0x00
#define FMD_MODE_RX		0x01
#define FMD_MODE_TX		0x02

/* FM Antenna selection  */
#define FMD_ANTENNA_EMBEDDED	0x00
#define FMD_ANTENNA_WIRED		0x01

/* FM tuning grid  */
#define FMD_GRID_50KHZ	0x00
#define FMD_GRID_100KHZ	0x01
#define FMD_GRID_200KHZ	0x02

/* FM Deemphasis settings  */
#define FMD_EMPHASIS_50US	0x01
#define FMD_EMPHASIS_75US	0x02

/* Mixer modes  */
#define FMD_MIXERMODE_PASS			((uint32_t) 0x00000000)
#define FMD_MIXERMODE_SUM			((uint32_t) 0x00000001)
#define FMD_MIXERMODE_LEFT			((uint32_t) 0x00000002)
#define FMD_MIXERMODE_RIGHT                  	((uint32_t) 0x00000003)

#define FMD_AUTOMUTEMODE_OFF			((uint32_t) 0x00000000)
#define FMD_AUTOMUTEMODE_ON			((uint32_t) 0x00000001)
#define FMD_AUTOMUTEMODE_AUTOMATIC		((uint32_t) 0x00000002)

#define FMD_AUTOMUTETYPE_MUTE			((uint32_t) 0x00000000)
#define FMD_AUTOMUTETYPE_RDS			((uint32_t) 0x00000001)

/* Output of BT sample Rate Convertor  */
#define FMD_OUTPUT_DISABLED  0x00
#define FMD_OUTPUT_I2S  0x01
#define FMD_OUTPUT_PARALLEL  0x02
#define FMD_OUTPUT_FIFO  0x03

/* on Off States for RDS   */
#define FMD_SWITCH_OFF_RDS_SIMULATOR  0x00
#define FMD_SWITCH_OFF_RDS  0x01
#define FMD_SWITCH_ON_RDS  0x02
#define FMD_SWITCH_ON_RDS_ENHANCED_MODE  0x03

#define ST_WRITE_FILE_BLK_SIZE		254
#define CATENA_OPCODE                 0xFE

#define FM_WRITE 0x00
#define FM_READ   0x01

/* HCI Events  */
#define HCI_COMMAND_COMPLETE_EVENT 0x0E
#define HCI_VS_DBG_EVENT           0xFF


/* HCI Packets indicators  */
#define HCI_PACKET_INDICATOR_CMD            0x01
 #define HCI_PACKET_INDICATOR_EVENT          0x04
 #define HCI_PACKET_INDICATOR_FM_CMD_EVT     0x08

/* HCI Command opcodes  */
#define HCI_CMD_FM                          0xFD50
#define HCI_CMD_VS_WRITE_FILE_BLOCK         0xFC2E
#define FM_EVENT                            0x15

#define FM_FUNCTION_ENABLE        0x00
#define FM_FUNCTION_DISABLE       0x01
#define FM_FUNCTION_RESET         0x02
#define FM_FUNCTION_WRITECOMMAND  0x10
#define FM_FUNCTION_SETINTMASKALL 0x20
#define FM_FUNCTION_GETINTMASKALL 0x21
#define FM_FUNCTION_SETINTMASK    0x22
#define FM_FUNCTION_GETINTMASK    0x23
#define FM_FUNCTION_FMFWDOWNLOAD  0x30

/*  FM status in return parameter Size: 1 Byte  */

/* Command succeeded  */
#define FM_CMD_STATUS_CMD_SUCCESS                0x00
/* HCI_ERR_HW_FAILURE when no response from the IP.8  */
#define FM_CMD_STATUS_HCI_ERR_HW_FAILURE         0x03
/* HCI_ERR_INVALID_PARAMETERS. */
#define FM_CMD_STATUS_HCI_ERR_INVALID_PARAMETERS 0x12
/* When the host tries to send a command to an IP that hasn't been
 * initialized.
 */
#define FM_CMD_STATUS_IP_UNINIT                  0x15
/* HCI_ERR_UNSPECIFIED_ERROR: any other error  */
#define FM_CMD_STATUS_HCI_ERR_UNSPECIFIED_ERROR  0x1F
/* HCI_ERR_CMD_DISALLOWED when the host asks for an unauthorized operation
 * (FM state transition for instance)
 */
#define FM_CMD_STATUS_HCI_ERR_CMD_DISALLOWED     0x0C
/* Wrong sequence number for FM FW download command  */
#define FM_CMD_STATUS_WRONG_SEQ_NUM              0xF1
/* Unknown file type for FM FW download command  */
#define FM_CMD_STATUS_UNKOWNFILE_TYPE            0xF2
/* File version mismatch for FM FW download command   */
#define FM_CMD_STATUS_FILE_VERSION_MISMATCH      0xF3

/* Callback function to receive radio events. */
typedef void(*fmd_radio_cb)(
		void  *context,
		uint32_t event,
		uint32_t event_int_data,
		bool event_boolean_data
		);

/**
 * fmd_init() - Initialize the FM Driver internal structures.
 * @context: (out) Pointer to Pointer of FM Driver Context
 * Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 */
uint8_t fmd_init(
		void  **context
		);

/**
 * fmd_register_callback() - Function to register callback function.
 * This fucntion registers the callback function provided by upper layers.
 * @context: Pointer to FM Driver Context
 * @callback: Fmradio call back Function pointer
 * Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_register_callback(
		void  *context,
		fmd_radio_cb callback
		);

/**
 * fmd_get_version() - Retrives the FM HW and FW version.
 * @context: Pointer to FM Driver Context
 * @version: Version Array
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_get_version(
		void  *context,
		uint16_t version[]
		);

/**
 * fmd_set_mode() - Starts a transition to the given mode.
 * @context: Pointer to FM Driver Context
 * @mode: Transition mode
 *  Returns:
 *	 FMD_RESULT_WAIT,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */

uint8_t fmd_set_mode(
		void  *context,
		uint8_t mode
		);

/**
 * fmd_get_mode() - Gets the current mode.
 * @context: Pointer to FM Driver Context
 * @mode: Current Transition mode
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */

uint8_t fmd_get_mode(
		void  *context,
		uint8_t *mode
		);

/**
 * fmd_is_freq_range_supported() - Checks if Freq range is supported or not.
 * @context: Pointer to FM Driver Context
 * @range: range of freq
 * @supported:Available freq range.
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_is_freq_range_supported(
		void  *context,
		uint8_t range,
		bool  *supported
		);

/**
 * fmd_get_freq_range_properties() - Retrives Freq Range Properties.
 * @context: Pointer to FM Driver Context
 * @range: range of freq
 * @minfreq: Pointer to Minimum Frequency of the Band
 * @maxfreq: Pointer to Maximum Frequency of the Band
 * @freqinterval: Pointer to the Frequency Interval
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_get_freq_range_properties(
		void  *context,
		uint8_t range,
		uint32_t  *minfreq,
		uint32_t  *maxfreq,
		uint32_t  *freqinterval
		);

/**
 * fmd_set_antenna() - Selects the antenna to be used in receive mode.
 * embedded - Selects the embedded antenna, wired- Selects the wired antenna.
 * @context: Pointer to FM Driver Context
 * @ant: Antenna Type
 *  Returns:
 *	 FMD_RESULT_WAIT,  if set antenna started successfully.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_set_antenna(
		void  *context,
		uint8_t ant
		);

/**
 * fmd_get_antenna() - Retrives the currently used antenna type.
 * @context: Pointer to FM Driver Context
 * @ant: Pointer to Antenna
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_get_antenna(
		void  *context,
		uint8_t *ant
		);

/**
 * fmd_set_freq_range() - Sets the FM band.
 * @context: Pointer to FM Driver Context
 * @range: freq range
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_set_freq_range(
		void  *context,
		uint8_t range
		);

/**
 * fmd_get_freq_range() - Gets the FM band currently in use.
 * @context: Pointer to FM Driver Context
 * @range: Pointer to base freq range.
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_get_freq_range(
		void  *context,
		uint8_t *range
		);

/**
 * fmd_rx_set_grid() - Sets the tuning grid.
 * @context: Pointer to FM Driver Context
 * @grid: Tuning grid size
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_set_grid(
		void  *context,
		uint8_t grid
		);

/**
 * fmd_rx_get_grid() - Gets the current tuning grid.
 * @context: Pointer to FM Driver Context
 * @grid: pointer to base Grid to be retrieved.
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_rx_get_grid(
		void  *context,
		uint8_t *grid
		);

/**
 * fmd_rx_set_deemphasis() - Sets the De-emphasis level.
 * Sets the de-emphasis characteristic of the receiver
 * FMD_EMPHASIS_50US &  FMD_EMPHASIS_75US.
 * @context: Pointer to FM Driver Context
 * @deemphasis: Char of Rx level.
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_set_deemphasis(
		void  *context,
		uint8_t deemphasis
		);

/**
 * fmd_rx_get_deemphasis() - Get the De-emphasis level.
 * Gets the currently used de-emphasis characteristic
 * of the receiver FMD_EMPHASIS_50US &  FMD_EMPHASIS_75US.
 * @context: Pointer to FM Driver Context
 * @deemphasis: Pointer to base De-emphasis.
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_rx_get_deemphasis(
		void  *context,
		uint8_t *deemphasis
		);

/**
 * fmd_rx_set_frequency() - Sets the FM Channel.
 * @context: Pointer to FM Driver Context
 * @freq: Frequency to Set in Khz
 *  Returns:
 *	 FMD_RESULT_WAIT,  if set frequency operation started
 * successfully.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_set_frequency(
		void  *context,
		uint32_t freq
		);

/**
 * fmd_rx_get_frequency() - Gets the currently used FM Channel.
 * @context: Pointer to FM Driver Context
 * @freq: Pointer to base freq
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_get_frequency(
		void  *context,
		uint32_t  *freq
		);

/**
 * fmd_rx_set_stereo_mode() - Sets the stereomode functionality.
 * @context: Pointer to FM Driver Context
 * @mode: FMD_STEREOMODE_MONO, FMD_STEREOMODE_STEREO and
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_set_stereo_mode(
		void  *context,
		uint32_t mode
		);

/**
 * fmd_rx_get_stereo_mode() - Gets the currently used FM mode.
 * FMD_STEREOMODE_MONO, FMD_STEREOMODE_STEREO and
 * FMD_STEREOMODE_AUTO.
 * @context: Pointer to FM Driver Context
 * @mode: Pointer to mode
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_rx_get_stereo_mode(
		void  *context,
		uint32_t  *mode
		);

/**
 * fmd_rx_get_signal_strength() - Gets the RSSI level of current frequency.
 * @context: Pointer to FM Driver Context
 * @strength: Pointer to base Strength
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_get_signal_strength(
		void  *context,
		uint32_t  *strength
		);

/**
 * fmd_rx_get_pilot_state() - Gets Pilot State.
 * @context: Pointer to FM Driver Context
 * @on: Pointer to Pilot state for FM Rx.
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_get_pilot_state(
		void  *context,
		bool  *on
		);

/**
 * fmd_rx_set_stop_level() - Sets the FM Rx Seek stop level.
 * @context: Pointer to FM Driver Context
 * @stoplevel: seek stop level
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_rx_set_stop_level(
		void  *context,
		uint16_t stoplevel
		);

/**
 * fmd_rx_get_stop_level() - Gets the current FM Rx Seek stop level.
 * @context: Pointer to FM Driver Context
 * @stoplevel: Pointer to base stop level
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_rx_get_stop_level(
		void  *context,
		uint16_t  *stoplevel
		);

/**
 * fmd_rx_seek() - Perform FM Seek.
 * Starts searching relative to the actual channel with
 * a specific direction, stop.
 * @context: Pointer to FM Driver Context
 *  level and optional noise levels
 * @upwards: scan up
 *  Returns:
 *	 FMD_RESULT_ONGOING,  if seek started successfully.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_seek(
		void  *context,
		bool upwards
		);

/**
 * fmd_rx_stop_seeking() - Stops a currently active seek or scan band.
 * @context: Pointer to FM Driver Context
 *  Returns:
 *	 FMD_RESULT_WAIT,  if stop seek started successfully.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_PRECONDITIONS_VIOLATED, if FM Driver is
 * not currently in Seek or Scan State..
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_stop_seeking(
		void  *context
		);

/**
 * fmd_rx_scan_band() - Starts Band Scan.
 * Starts scanning the active band for the strongest
 * channels above a threshold.
 * @context: Pointer to FM Driver Context
 * Returns:
 *   FMD_RESULT_ONGOING,  if scan band started successfully.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_scan_band(
		void  *context
		);

/**
 * fmd_rx_get_scan_band_info() - Retrieves Channels found during scanning.
 * Retrives the scanned active band
 * for the strongest channels above a threshold.
 * @context: Pointer to FM Driver Context
 * @index: Index value to retrieve the channels.
 * @numchannels: Pointer to Number of channels found
 * @channels: Pointer to channels found
 * @rssi:  Pointer to rssi of channels found
 * Returns:
 *   FMD_RESULT_ONGOING,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_get_scan_band_info(
		void  *context,
		uint32_t index,
		uint16_t  *numchannels,
		uint16_t  *channels,
		uint16_t  *rssi
		);

/**
 * fmd_rx_step_frequency() - Steps one channel up or down.
 * @context: Pointer to FM Driver Context
 * @upwards: IF true step upwards else downwards.
 * Returns:
 *   FMD_RESULT_WAIT,  if step frequency started successfully.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_step_frequency(
		void  *context,
		bool upwards
		);

/**
 * fmd_rx_get_rds() - Gets the current status of RDS transmission.
 * @context: Pointer to FM Driver Context
 * @on: pointer to RDS status
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *       FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_rx_get_rds(
		void  *context,
		bool  *on
		);

/**
 * fmd_rx_query_rds_signal() - Gets information about the transmitter.
 * @context: Pointer to FM Driver Context
 * @is_signal:TX RDS signal
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *       FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_query_rds_signal(
		void  *context,
		bool  *is_signal
		);

/**
 * fmd_rx_buffer_set_size() - Sets the number of groups that the data buffer.
 * can contain and clears the buffer.
 * @context: Pointer to FM Driver Context
 * @size: buffer size
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_buffer_set_size(
		void  *context,
		uint8_t size
		);

/**
 * fmd_rx_buffer_set_threshold() - Sets the group number at which the buffer.
 * full interrupt must be generated.
 * The interrupt will be set after reception of the group.
 * @context: Pointer to FM Driver Context
 * @threshold: threshold level.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_buffer_set_threshold(
		void  *context,
		uint8_t threshold
		);

/**
 * fmd_rx_set_ctrl() - Enables or disables demodulation of RDS data.
 * @context: Pointer to FM Driver Context
 * @onoffState : Rx Set ON /OFF control
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_rx_set_ctrl(
		void  *context,
		uint8_t onoffState
		);

/**
 * fmd_rx_get_low_level_rds_groups() - Gets Low level RDS group data.
 * @context: Pointer to FM Driver Context
 * @index: RDS group index
 * @rds_buf_count: Count for RDS buffer
 * @block1: RDS Block 1
 * @block2: RDS Block 2
 * @block3: RDS Block 3
 * @block4: RDS Block 4
 * @status1: RDS data status 1
 * @status2: RDS data status 2
 * @status3: RDS data status 3
 * @status4: RDS data status 4
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *       FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_rx_get_low_level_rds_groups(
		void  *context,
		uint8_t index,
		uint8_t *rds_buf_count,
		uint16_t  *block1,
		uint16_t  *block2,
		uint16_t  *block3,
		uint16_t  *block4,
		uint8_t  *status1,
		uint8_t  *status2,
		uint8_t  *status3,
		uint8_t  *status4
		);

/**
 * fmd_set_audio_dac() - Enables or disables the audio DAC.
 * @context: Pointer to FM Driver Context
 * @dac_state: DAC state
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_set_audio_dac(
		void  *context,
		uint8_t dac_state
		);

/**
 * fmd_set_volume() - Sets the receive audio volume.
 * @context: Pointer to FM Driver Context
 * @volume: Audio volume level
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_set_volume(
		void  *context,
		uint8_t volume
		);

/**
 * fmd_get_volume() - Retrives the current audio volume.
 * @context: Pointer to FM Driver Context
 * @volume: Pointer to base type Volume
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_get_volume(
		void  *context,
		uint8_t *volume
		);

/**
 * fmd_set_balance() - Controls the receiver audio balance.
 * @context: Pointer to FM Driver Context
 * @balance: Audiio balance level
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_set_balance(
		void  *context,
		int8_t balance
		);

/**
 * fmd_get_balance() - Retrives the receiver current audio balance level.
 * @context: Pointer to FM Driver Context
 * @balance: Pointer to Audio balance level
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_get_balance(
		void  *context,
		int8_t  *balance
		);

/**
 * fmd_set_mute() - Enables or disables muting of the analog audio(DAC).
 * @context: Pointer to FM Driver Context
 * @mute_on: bool of mute on
 * Returns:
 *   FMD_RESULT_WAIT,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_set_mute(
		void  *context,
		bool mute_on
		);

/**
 * fmd_get_mute() - Retrives the current state of analog audio(DAC).
 * @context: Pointer to FM Driver Context
 * @mute_on: Pointer to base type mute on
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
uint8_t fmd_get_mute(
		void  *context,
		bool  *mute_on
		);

/**
 * fmd_bt_set_ctrl() - Sets parameters of the BT Sample Rate Converter.
 * The sample rate conversion type(up or down) of the digital output
 * should be set to "up" if the required audio output rate exceeds
 * 41.3 kHz, otherwise the conversion type is down"
 * @context: Pointer to FM Driver Context
 * @up_conversion: Rate convertor -up
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_bt_set_ctrl(
		void  *context,
		bool up_conversion
		);

/**
 * fmd_bt_set_mode() - Sets the output of the BT Sample Rate Converter.
 * @context: Pointer to FM Driver Context
 * @output: output mode
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_bt_set_mode(
		void  *context,
		uint8_t output
		);

/**
 * fmd_ext_set_mode() - Sets the output mode of the Ext Sample Rate Converter.
 * @context: Pointer to FM Driver Context
 * @output: output mode
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_ext_set_mode(
		void  *context,
		uint8_t output
		);

/**
 * fmd_ext_set_ctrl() - Sets parameters of the Ext Sample Rate Converter.
 * The sample rate conversion type(up or down)
 * of the digital output should be set to "up" if the required audio
 * output rate exceeds 41.3 kHz, otherwise the conversion type is "down".
 * @context: Pointer to FM Driver Context
 * @up_conversion: boolean type-Upconversion for sample rate.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_ext_set_ctrl(
		void  *context,
		bool up_conversion
		);

/**
 * fmd_ext_set_mute() - Enables or disables muting of the audio channel.
 * @context: Pointer to FM Driver Context
 * @mute_on: bool to Mute
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_ext_set_mute(
		void  *context,
		bool mute_on
		);

/**
 * fmd_power_up() - Puts the system in Powerup state.
 * @context: Pointer to FM Driver Context
 * Returns:
 *   FMD_RESULT_WAIT,  if power up command sent successfully to chip.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_power_up(
		void  *context
		);

/**
 * fmd_goto_standby() - Puts the system in standby mode.
 * @context: Pointer to FM Driver Context
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_goto_standby(
		void  *context
		);

/**
 * fmd_goto_power_down() - Puts the system in Powerdown mode.
 * @context: Pointer to FM Driver Context
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_goto_power_down(
		void  *context
		);

/**
 * fmd_select_ref_clk() - Selects the FM reference clock.
 * @context: Pointer to FM Driver Context
 * @ref_clk: Ref Clock.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_select_ref_clk(
		void  *context,
		uint16_t ref_clk
		);

/**
 * fmd_select_ref_clk_pll() - Selects the freq of Referece Clock.
 * Sets frequency and offset correction properties of the external
 * reference clock of the PLL
 * @context: Pointer to FM driver Lib.
 * @freq: PLL Frequency.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
uint8_t fmd_select_ref_clk_pll(
		void  *context,
		uint16_t freq
		);

/**
 * fmd_send_fm_ip_enable()- Enables the FM IP.
 * Returns:
 *   0,  if operation completed successfully.
 *   1, otherwise.
 */
unsigned int fmd_send_fm_ip_enable(void);

/**
 * fmd_send_fm_ip_disable()- Disables the FM IP.
 * Returns:
 *   0,  if operation completed successfully.
 *   1, otherwise.
 */
unsigned int fmd_send_fm_ip_disable(void);

/**
 * fmd_send_fm_ip_reset() - Resets the FM IP.
 * Returns:
 *   0,  if operation completed successfully.
 *   1, otherwise.
 */
unsigned int fmd_send_fm_ip_reset(void);

/**
 * fmd_send_fm_firmware() - Send the FM Firmware File to Device.
 * @fw_buffer: Pointer to the Firmware Array
 * @fw_size: Size of firmware to be downloaded
 * Returns:
 *   0,  if operation completed successfully.
 *   1, otherwise.
 */
unsigned int fmd_send_fm_firmware(
		uint8_t *fw_buffer,
		uint16_t fw_size
		);

/**
 * fmd_receive_data() - Processes the FM data received from device.
 * @packet_length: Length of received Data Packet
 * @packet_buffer: Pointer to the received Data packet
 * Returns:
 *   0,  if operation completed successfully.
 *   1, otherwise.
 */
void fmd_receive_data(
		uint32_t packet_length,
		uint8_t *packet_buffer
		);

/**
 * fmd_int_bufferfull() - RDS Groups availabe for reading by Host.
 * Gets the number of groups that are available in the
 * buffer. This function is called in RX mode to read RDS groups.
 */
void fmd_int_bufferfull(void);

/**
 * fmd_hexdump() - Displays the HCI Data Bytes exchanged with FM Chip.
 * @prompt: Prompt signifying the direction '<' for Rx '>' for Tx
 * @buffer: Pointer to the buffer to be displayed.
 * @num_bytes: Number of bytes of the buffer.
 */
void fmd_hexdump(
		char prompt,
		uint8_t *buffer,
		int num_bytes
		);

#endif /* _FMDRIVER_H_  */
