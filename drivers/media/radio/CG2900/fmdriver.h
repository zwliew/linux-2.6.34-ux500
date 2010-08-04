/*
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

#define IRPT_INVALID					0x0000
#define IRPT_OPERATIONSUCCEEDED				0x0001
#define IRPT_OPERATIONFAILED				0x0002
#define IRPT_RX_BUFFERFULL_TX_BUFFEREMPTY		0x0008
#define IRPT_RX_SIGNALQUALITYLOW_MUTE_STATUS_CHANGED	0x0010
#define IRPT_RX_MONOSTEREO_TRANSITION	0x0020
#define IRPT_TX_OVERMODULATION	0x0030
#define IRPT_RX_RDSSYNCFOUND_TX_OVERDRIVE		0x0040
#define IRPT_RDSSYNCLOST				0x0080
#define IRPT_PICODECHANGED				0x0100
#define IRPT_REQUESTEDBLOCKAVAILABLE			0x0200
#define IRPT_BUFFER_CLEARED				0x2000
#define IRPT_WARMBOOTREADY				0x4000
#define IRPT_COLDBOOTREADY				0x8000

#define CMD_AIP_BT_SETCONTROL				0x01A6
#define CMD_AIP_BT_SETMODE				0x01E6
#define CMD_AIP_FADE_START				0x0046
#define CMD_AIP_LEVEL_GETSTATUS				0x0106
#define CMD_AIP_LEVEL_SETMODE				0x00E6
#define CMD_AIP_OVERDRIVE_SETCONTROL			0x0166
#define CMD_AIP_OVERDRIVE_SETMODE			0x0126
#define CMD_AIP_SETAUDIOADC				0x00A6
#define CMD_AIP_SETAUDIOADC_CONTROL			0x00C6
#define CMD_AIP_SETBALANCE				0x0086
#define CMD_AIP_SETGAIN					0x0066
#define CMD_AIP_SETMIXER				0x0146
#define CMD_AIP_SETMODE					0x01C6
#define CMD_AUP_BT_SETBALANCE				0x0142
#define CMD_AUP_BT_SETCONTROL				0x00E2
#define CMD_AUP_BT_SETMODE				0x00C2
#define CMD_AUP_BT_SETVOLUME				0x0122
#define CMD_AUP_EXT_SETCONTROL				0x0182
#define CMD_AUP_EXT_SETMODE				0x0162
#define CMD_AUP_EXT_FADESTART				0x0102
#define CMD_AUP_EXT_SETMUTE				0x01E2
#define CMD_AUP_FADE_START				0x00A2
#define CMD_AUP_SETAUDIODAC				0x0082
#define CMD_AUP_SETBALANCE				0x0042
#define CMD_AUP_SETMUTE					0x0062
#define CMD_AUP_SETVOLUME				0x0022
#define CMD_FMR_DP_BLOCKREQUEST_CONTROL			0x0583
#define CMD_FMR_DP_BLOCKREQUEST_GETBLOCK		0x05A3
#define CMD_FMR_DP_BUFFER_GETGROUP			0x0303
#define CMD_FMR_DP_BUFFER_GETGROUPCOUNT			0x0323
#define CMD_FMR_DP_BUFFER_SETSIZE			0x0343
#define CMD_FMR_DP_BUFFER_SETTHRESHOLD			0x06C3
#define CMD_FMR_DP_GETSTATE				0x0283
#define CMD_FMR_DP_PICORRELATION_SETCONTROL		0x0363
#define CMD_FMR_DP_QUALITY_GETRESULT			0x05E3
#define CMD_FMR_DP_QUALITY_START			0x05C3
#define CMD_FMR_DP_SETCONTROL				0x02A3
#define CMD_FMR_DP_SETEBLOCKREJECTION			0x02E3
#define CMD_FMR_DP_SETGROUPREJECTION			0x0543
#define CMD_FMR_GETSTATE				0x04E3
#define CMD_FMR_RP_GETRSSI				0x0083
#define CMD_FMR_RP_GETSTATE				0x0063
#define CMD_FMR_RP_HIGHCUT_SETCONTROL			0x0263
#define CMD_FMR_RP_HIGHCUT_SETMODE			0x0243
#define CMD_FMR_RP_SETDEEMPHASIS			0x00C3
#define CMD_FMR_RP_SETRSSIQUALITYTHRESHOLD		0x00A3
#define CMD_FMR_RP_SOFTMUTE_SETCONTROL			0x0223
#define CMD_FMR_RP_SOFTMUTE_SETMODE			0x0203
#define CMD_FMR_RP_STEREO_SETCONTROL_BLENDINGPILOT 	0x0163
#define CMD_FMR_RP_STEREO_SETCONTROL_BLENDINGRSSI 	0x0143
#define CMD_FMR_RP_STEREO_SETMODE			0x0123
#define CMD_FMR_RP_THRESHOLDEXTENSION_SETCONTROL 	0x0103
#define CMD_FMR_RP_THRESHOLDEXTENSION_SETMODE		0x00E3
#define CMD_FMR_RP_XSTEREO_SETCONTROL			0x01E3
#define CMD_FMR_RP_XSTEREO_SETMODE			0x01C3
#define CMD_FMR_SETANTENNA				0x0663
#define CMD_FMR_SETCOEXFILTER				0x06E3
#define CMD_FMR_SETPROCESSINGMODE			0x0643
#define CMD_FMR_SP_AFSWITCH_GETRESULT			0x0603
#define CMD_FMR_SP_AFSWITCH_START			0x04A3
#define CMD_FMR_SP_AFUPDATE_GETRESULT			0x0483
#define CMD_FMR_SP_AFUPDATE_START			0x0463
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

#define ST_WRITE_FILE_BLK_SIZE				254
#define CATENA_OPCODE					0xFE

#define FM_WRITE					0x00
#define FM_READ						0x01

/* HCI Events  */
#define HCI_COMMAND_COMPLETE_EVENT			0x0E
#define HCI_VS_DBG_EVENT				0xFF

/* HCI Packets indicators  */
#define HCI_PACKET_INDICATOR_CMD			0x01
 #define HCI_PACKET_INDICATOR_EVENT			0x04
 #define HCI_PACKET_INDICATOR_FM_CMD_EVT		0x08

/* HCI Command opcodes  */
#define HCI_CMD_FM					0xFD50
#define HCI_CMD_VS_WRITE_FILE_BLOCK			0xFC2E
#define FM_EVENT					0x15

#define FM_FUNCTION_ENABLE				0x00
#define FM_FUNCTION_DISABLE				0x01
#define FM_FUNCTION_RESET				0x02
#define FM_FUNCTION_WRITECOMMAND			0x10
#define FM_FUNCTION_SETINTMASKALL			0x20
#define FM_FUNCTION_GETINTMASKALL			0x21
#define FM_FUNCTION_SETINTMASK				0x22
#define FM_FUNCTION_GETINTMASK				0x23
#define FM_FUNCTION_FMFWDOWNLOAD			0x30

/*  FM status in return parameter Size: 1 Byte */

/* Command succeeded */
#define FM_CMD_STATUS_CMD_SUCCESS			0x00
/* HCI_ERR_HW_FAILURE when no response from the IP */
#define FM_CMD_STATUS_HCI_ERR_HW_FAILURE		0x03
/* HCI_ERR_INVALID_PARAMETERS. */
#define FM_CMD_STATUS_HCI_ERR_INVALID_PARAMETERS	0x12
/* When the host tries to send a command to an IP that hasn't been
 * initialized.
 */
#define FM_CMD_STATUS_IP_UNINIT				0x15
/* HCI_ERR_UNSPECIFIED_ERROR: any other error */
#define FM_CMD_STATUS_HCI_ERR_UNSPECIFIED_ERROR		0x1F
/* HCI_ERR_CMD_DISALLOWED when the host asks for an unauthorized operation
 * (FM state transition for instance)
 */
#define FM_CMD_STATUS_HCI_ERR_CMD_DISALLOWED		0x0C
/* Wrong sequence number for FM FW download command */
#define FM_CMD_STATUS_WRONG_SEQ_NUM			0xF1
/* Unknown file type for FM FW download command */
#define FM_CMD_STATUS_UNKOWNFILE_TYPE			0xF2
/* File version mismatch for FM FW download command */
#define FM_CMD_STATUS_FILE_VERSION_MISMATCH		0xF3

/**
 * enum fmd_status_t - Status of some operations.
 * @FMD_RESULT_SUCCESS: Current operation is completed with success.
 * @FMD_RESULT_PRECONDITIONS_VIOLATED: Frequency has been changed.
 * @FMD_RESULT_PARAMETER_INVALID: Frequency Range has been changed.
 * @FMD_RESULT_IO_ERROR: Seek operation has completed.
 * @FMD_RESULT_FEATURE_UNSUPPORTED: Band Scan Completed.
 * @FMD_RESULT_BUSY: Block Scan completed.
 * @FMD_RESULT_ONGOING: Af Update or AF Switch is complete.
 * stopped.
 * @FMD_RESULT_RESPONSE_WRONG: FM IP Powerup has been powered up.
 * Various status returned for any operation by FM Driver.
 */
enum fmd_status_t {
	FMD_RESULT_SUCCESS,
	FMD_RESULT_PRECONDITIONS_VIOLATED,
	FMD_RESULT_PARAMETER_INVALID,
	FMD_RESULT_IO_ERROR,
	FMD_RESULT_FEATURE_UNSUPPORTED,
	FMD_RESULT_BUSY,
	FMD_RESULT_ONGOING,
	FMD_RESULT_RESPONSE_WRONG
};

/**
 * enum fmd_event_t - Events received.
 * @FMD_EVENT_ANTENNA_STATUS_CHANGED: Antenna has been changed.
 * @FMD_EVENT_FREQUENCY_CHANGED: Frequency has been changed.
 * @FMD_EVENT_SEEK_COMPLETED: Seek operation has completed.
 * @FMD_EVENT_SCAN_BAND_COMPLETED: Band Scan Completed.
 * @FMD_EVENT_BLOCK_SCAN_COMPLETED: Block Scan completed.
 * @FMD_EVENT_AF_UPDATE_SWITCH_COMPLETE: Af Update or AF Switch is complete.
 * @FMD_EVENT_MONOSTEREO_TRANSITION_COMPLETE: Mono stereo transition is
 * completed.
 * @FMD_EVENT_SEEK_STOPPED: Previous Seek/Band Scan/ Block Scan operation is
 * stopped.
 * @FMD_EVENT_GEN_POWERUP: FM IP Powerup has been powered up.
 * @FMD_EVENT_RDSGROUP_RCVD: RDS Groups Full interrupt.
 * @FMD_EVENT_LAST_ELEMENT: Last event, used for keeping count of
 * number of events.
 * Various events received from Fm driver for Upper Layer(s) processing.
 */
enum fmd_event_t {
	FMD_EVENT_ANTENNA_STATUS_CHANGED,
	FMD_EVENT_FREQUENCY_CHANGED,
	FMD_EVENT_SEEK_COMPLETED,
	FMD_EVENT_SCAN_BAND_COMPLETED,
	FMD_EVENT_BLOCK_SCAN_COMPLETED,
	FMD_EVENT_AF_UPDATE_SWITCH_COMPLETE,
	FMD_EVENT_MONOSTEREO_TRANSITION_COMPLETE,
	FMD_EVENT_SEEK_STOPPED,
	FMD_EVENT_GEN_POWERUP,
	FMD_EVENT_RDSGROUP_RCVD,
	FMD_EVENT_LAST_ELEMENT
};

/**
 * enum fmd_mode_t - FM Driver Modes.
 * @FMD_MODE_IDLE: FM Driver in Idle mode.
 * @FMD_MODE_RX: FM Driver in Rx mode.
 * @FMD_MODE_TX: FM Driver in Tx mode.
 * Various Modes of FM Radio.
 */
enum fmd_mode_t {
	FMD_MODE_IDLE,
	FMD_MODE_RX,
	FMD_MODE_TX
};

/**
 * enum fmd_antenna_t - Antenna selection.
 * @FMD_ANTENNA_EMBEDDED: Embedded Antenna.
 * @FMD_ANTENNA_WIRED: Wired Antenna.
 * Antenna to be used for FM Radio.
 */
enum fmd_antenna_t {
	FMD_ANTENNA_EMBEDDED,
	FMD_ANTENNA_WIRED
};

/**
 * enum fmd_grid_t - Grid used on FM Radio.
 * @FMD_GRID_50KHZ: 50  kHz grid spacing.
 * @FMD_GRID_100KHZ: 100  kHz grid spacing.
 * @FMD_GRID_200KHZ: 200  kHz grid spacing.
 * Spacing used on FM Radio.
 */
enum fmd_grid_t {
	FMD_GRID_50KHZ,
	FMD_GRID_100KHZ,
	FMD_GRID_200KHZ
};

/**
 * enum fmd_emphasis_t - De-emphasis/Pre-emphasis level.
 * @FMD_EMPHASIS_50US: 50 us de-emphasis/pre-emphasis level.
 * @FMD_EMPHASIS_75US: 100 us de-emphasis/pre-emphasi level.
 * De-emphasis/Pre-emphasis level used on FM Radio.
 */
enum fmd_emphasis_t {
	FMD_EMPHASIS_50US,
	FMD_EMPHASIS_75US
};

/**
 * enum fmd_freq_range_t - Frequency range.
 * @FMD_FREQRANGE_EUROAMERICA: EU/US Range (87.5 - 108 MHz).
 * @FMD_FREQRANGE_JAPAN: Japan Range (76 - 90 MHz).
 * @FMD_FREQRANGE_CHINA: China Range (70 - 108 MHz).
 * Various Frequency range(s) supported by FM Radio.
 */
enum fmd_freq_range_t {
	FMD_FREQRANGE_EUROAMERICA,
	FMD_FREQRANGE_JAPAN,
	FMD_FREQRANGE_CHINA
};

/**
 * enum fmd_stereo_mode_t - FM Driver Stereo Modes.
 * @FMD_STEREOMODE_OFF: Streo Blending Off.
 * @FMD_STEREOMODE_MONO: Mono Mode.
 * @FMD_STEREOMODE_BLENDING: Blending Mode.
 * Various Stereo Modes of FM Radio.
 */
enum fmd_stereo_mode_t {
	FMD_STEREOMODE_OFF,
	FMD_STEREOMODE_MONO,
	FMD_STEREOMODE_BLENDING
};

/*Remove the below two enum w.r.t audio_config redundancy*/

 /**
 * enum fmd_output_t - Output of Sample Rate Converter.
 * @FMD_OUTPUT_DISABLED: Sample Rate converter in disabled.
 * @FMD_OUTPUT_I2S: I2S Output from Sample rate converter.
 * @FMD_OUTPUT_PARALLEL: Parallel output from sample rate converter.
 * Sample Rate Converter's output to be set on Connectivity Controller.
 */
enum fmd_output_t {
	FMD_OUTPUT_DISABLED,
	FMD_OUTPUT_I2S,
	FMD_OUTPUT_PARALLEL
};

 /**
 * enum fmd_input_t - Audio Input to Sample Rate Converter.
 * @FMD_INPUT_ANALOG: Selects the ADC's as audio source
 * @FMD_INPUT_DIGITAL: Selects Digital Input as audio source.
 * Audio Input source for Sample Rate Converter.
 */
enum fmd_input_t {
	FMD_INPUT_ANALOG,
	FMD_INPUT_DIGITAL
};


/**
* enum fmd_rds_mode_t - RDS Mode to be selected for FM Rx.
* @FMD_SWITCH_OFF_RDS: RDS Decoding disabled in FM Chip.
* @FMD_SWITCH_ON_RDS: RDS Decoding enabled in FM Chip.
* @FMD_SWITCH_ON_RDS_ENHANCED_MODE: Enhanced RDS Mode.
* @FMD_SWITCH_ON_RDS_SIMULATOR: RDS Simulator switched on in FM Chip.
* RDS Mode to be selected for FM Rx.
*/
enum fmd_rds_mode_t {
	FMD_SWITCH_OFF_RDS,
	FMD_SWITCH_ON_RDS,
	FMD_SWITCH_ON_RDS_ENHANCED_MODE,
	FMD_SWITCH_ON_RDS_SIMULATOR
};

/* Callback function to receive radio events. */
typedef void(*fmd_radio_cb)(
		void  *context,
		u8 event,
		bool event_successful
		);

/**
 * fmd_init() - Initialize the FM Driver internal structures.
 * @context: (out) FM Driver Context
 * Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 */
u8 fmd_init(
		void  **context
		);

/**
 * fmd_deinit() - De-initialize the FM Driver.
 */
void fmd_deinit(void);

/**
 * fmd_register_callback() - Function to register callback function.
 * This function registers the callback function provided by upper layers.
 * @context: FM Driver Context
 * @callback: Fmradio call back Function pointer
 * Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_register_callback(
		void  *context,
		fmd_radio_cb callback
		);

/**
 * fmd_get_version() - Retrieves the FM HW and FW version.
 * @context: FM Driver Context
 * @version: (out) Version Array
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_get_version(
		void  *context,
		u16 version[7]
		);

/**
 * fmd_set_mode() - Starts a transition to the given mode.
 * @context: FM Driver Context
 * @mode: Transition mode
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if set mode done successfully.
 *	 FMD_RESULT_PARAMETER_INVALID, if parameter is invalid.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */

u8 fmd_set_mode(
		void  *context,
		u8 mode
		);

/**
 * fmd_get_freq_range_properties() - Retrieves Freq Range Properties.
 * @context: FM Driver Context
 * @range: range of freq
 * @minfreq: (out) Minimum Frequency of the Band in kHz.
 * @maxfreq: (out) Maximum Frequency of the Band in kHz
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_get_freq_range_properties(
		void  *context,
		u8 range,
		u32  *minfreq,
		u32  *maxfreq
		);

/**
 * fmd_set_antenna() - Selects the antenna to be used in receive mode.
 * embedded - Selects the embedded antenna, wired- Selects the wired antenna.
 * @context: FM Driver Context
 * @antenna: Antenna Type
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if set antenna done successfully.
 *	 FMD_RESULT_PARAMETER_INVALID, if parameter is invalid.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_set_antenna(
		void  *context,
		u8 antenna
		);

/**
 * fmd_get_antenna() - Retrieves the currently used antenna type.
 * @context: FM Driver Context
 * @antenna: (out) Antenna Selected on FM Radio.
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_get_antenna(
		void  *context,
		u8 *antenna
		);

/**
 * fmd_set_freq_range() - Sets the FM band.
 * @context: FM Driver Context
 * @range: freq range
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_PARAMETER_INVALID, if parameter is invalid.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_set_freq_range(
		void  *context,
		u8 range
		);

/**
 * fmd_get_freq_range() - Gets the FM band currently in use.
 * @context: FM Driver Context
 * @range: (out) Frequency Range set on FM Radio.
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_get_freq_range(
		void  *context,
		u8 *range
		);

/**
 * fmd_rx_set_grid() - Sets the tuning grid.
 * @context: FM Driver Context
 * @grid: Tuning grid size
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_PARAMETER_INVALID, if parameter is invalid.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_set_grid(
		void  *context,
		u8 grid
		);

/**
 * fmd_rx_set_frequency() - Sets the FM Channel.
 * @context: FM Driver Context
 * @freq: Frequency to Set in Khz
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if set frequency done successfully.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_set_frequency(
		void  *context,
		u32 freq
		);

/**
 * fmd_rx_get_frequency() - Gets the currently used FM Channel.
 * @context: FM Driver Context
 * @freq: (out) Current Frequency set on FM Radio.
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_get_frequency(
		void  *context,
		u32  *freq
		);

/**
 * fmd_rx_set_stereo_mode() - Sets the stereomode functionality.
 * @context: FM Driver Context
 * @mode: FMD_STEREOMODE_MONO, FMD_STEREOMODE_STEREO and
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_PARAMETER_INVALID, if parameter is invalid.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_set_stereo_mode(
		void  *context,
		u8 mode
		);

/**
 * fmd_rx_get_stereo_mode() - Gets the currently used FM mode.
 * FMD_STEREOMODE_MONO, FMD_STEREOMODE_STEREO and
 * FMD_STEREOMODE_AUTO.
 * @context: FM Driver Context
 * @mode: (out) Mode set on FM Radio, stereo or mono.
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_rx_get_stereo_mode(
		void  *context,
		u8 *mode
		);

/**
 * fmd_rx_get_signal_strength() - Gets the RSSI level of current frequency.
 * @context: FM Driver Context
 * @strength: (out) RSSI level of current channel.
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_get_signal_strength(
		void  *context,
		u16  *strength
		);

/**
 * fmd_rx_set_stop_level() - Sets the FM Rx Seek stop level.
 * @context: FM Driver Context
 * @stoplevel: seek stop level
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_rx_set_stop_level(
		void  *context,
		u16 stoplevel
		);

/**
 * fmd_rx_get_stop_level() - Gets the current FM Rx Seek stop level.
 * @context: FM Driver Context
 * @stoplevel: (out) RSSI Threshold set on FM Radio.
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_rx_get_stop_level(
		void  *context,
		u16  *stoplevel
		);

/**
 * fmd_rx_seek() - Perform FM Seek.
 * Starts searching relative to the actual channel with
 * a specific direction, stop.
 * @context: FM Driver Context
 *  level and optional noise levels
 * @upwards: scan up
 *  Returns:
 *	 FMD_RESULT_ONGOING,  if seek started successfully.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_seek(
		void  *context,
		bool upwards
		);

/**
 * fmd_rx_stop_seeking() - Stops a currently active seek or scan band.
 * @context: FM Driver Context
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if stop seek done successfully.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *	 FMD_RESULT_PRECONDITIONS_VIOLATED, if FM Driver is
 * not currently in Seek or Scan State..
 *	 FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_stop_seeking(
		void  *context
		);

/**
 * fmd_rx_af_update_start() - Perform AF update.
 * This is used to switch to a shortly tune to a AF freq,
 * measure its RSSI and tune back to the original frequency.
 * @context: FM Driver Context
 * @freq: Alternative frequncy in KHz to be set for AF updation.
 *  Returns:
 * FMD_RESULT_BUSY, if FM Driver is not in idle state.
 * FMD_RESULT_SUCCESS,  if no error.
 * FMD_RESULT_IO_ERROR, if there is an error.
 */
u8 fmd_rx_af_update_start(
		 void *context,
		 u32 freq
		 );

/**
 * fmd_rx_get_af_update_result() - Retrive result of AF update.
 * Retrive the RSSI level of the Alternative frequency.
 * @context: FM Driver Context
 * @af_level: RSSI level of the Alternative frequency.
 * Returns:
 * FMD_RESULT_BUSY, if FM Driver is not in idle state.
 * FMD_RESULT_SUCCESS,  if no error.
 * FMD_RESULT_IO_ERROR, if there is an error.
 */
u8 fmd_rx_get_af_update_result(
		 void *context,
		 u16 *af_level
		 );


/**
 * fmd_af_switch_start() -Performs AF switch.
 * @context: FM Driver Context
 * @freq: Frequency to Set in Khz.
 * @picode:programable id,unique for each station.
 *
 *  Returns:
 * FMD_RESULT_BUSY, if FM Driver is not in idle state.
 * FMD_RESULT_SUCCESS,  if no error.
 * FMD_RESULT_SUCCESS, if AF switch started successfully.
 * FMD_RESULT_IO_ERROR, if there is an error.
 */
u8 fmd_rx_af_switch_start(
		void *context,
		u32 freq,
		u16  picode
		);

/**
 * fmd_rx_get_af_switch_results() -Retrieves the results of AF Switch.
 * @context: FM Driver Context
 * @afs_conclusion: Conclusion of AF switch.
 * @afs_level: RSSI level of the Alternative frequnecy.
 * @afs_pi: PI code of the alternative channel (if found).
 * Returns:
 * FMD_RESULT_BUSY, if FM Driver is not in idle state.
 * FMD_RESULT_SUCCESS,  if no error.
 * FMD_RESULT_IO_ERROR, if there is an error.
 */
u8 fmd_rx_get_af_switch_results(
		 void *context,
		 u16 *afs_conclusion,
		 u16 *afs_level,
		 u16 *afs_pi
		 );

/**
 * fmd_rx_scan_band() - Starts Band Scan.
 * Starts scanning the active band for the strongest
 * channels above a threshold.
 * @context: FM Driver Context
 * @max_channels_to_scan: Maximum number of channels to scan.
 * Returns:
 *   FMD_RESULT_ONGOING,  if scan band started successfully.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_scan_band(
		void  *context,
		u8 max_channels_to_scan
		);

/**
 * fmd_rx_get_max_channels_to_scan() - Retreives the maximum channels.
 * Retrieves the maximum number of channels that can be found during
 * band scann.
 * @context: FM Driver Context
 * @max_channels_to_scan: (out) Maximum number of channels to scan.
 * Returns:
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_get_max_channels_to_scan(
		void *context,
		u8 *max_channels_to_scan
		);

/**
 * fmd_rx_get_scan_band_info() - Retrieves Channels found during scanning.
 * Retrieves the scanned active band
 * for the strongest channels above a threshold.
 * @context: FM Driver Context
 * @index: (out) Index value to retrieve the channels.
 * @numchannels: (out) Number of channels found during Band Scan.
 * @channels: (out) Channels found during band scan.
 * @rssi: (out) Rssi of channels found during Band scan.
 * Returns:
 *   FMD_RESULT_ONGOING,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_get_scan_band_info(
		void  *context,
		u32 index,
		u16  *numchannels,
		u16  *channels,
		u16  *rssi
		);

/**
 * fmd_rx_block_scan() - Starts Block Scan.
 * Starts block scan for retriving the RSSI level of channels
 * in the given block.
 * @context: FM Driver Context
 * @start_freq: Starting frequency of the block from where scanning has
 * to be started.
 * @stop_freq: End frequency of the block to be scanned.
 * @antenna: Antenna to be used during scanning.
 * Returns:
 *   FMD_RESULT_ONGOING,  if scan band started successfully.
 *   FMD_RESULT_PARAMETER_INVALID,  if parameters are invalid.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_block_scan(
		void *context,
		u32 start_freq,
		u32 stop_freq,
		u8 antenna
		);

/**
 * fmd_rx_get_block_scan_result() - Retrieves RSSI Level of channels.
 * Retrieves the RSSI level of the channels in the block.
 * @context: FM Driver Context
 * @index: (out) Index value to retrieve the channels.
 * @numchannels: (out) Number of channels found during Band Scan.
 * @rssi: (out) Rssi of channels found during Band scan.
 * Returns:
 *   FMD_RESULT_ONGOING,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_get_block_scan_result(
		void *context,
		u32 index,
		u16 *numchannels,
		u16 *rssi
		);

/**
 * fmd_rx_get_rds() - Gets the current status of RDS transmission.
 * @context: FM Driver Context
 * @on: (out) RDS status
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *       FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_rx_get_rds(
		void  *context,
		bool  *on
		);

/**
 * fmd_rx_buffer_set_size() - Sets the number of groups that the data buffer.
 * can contain and clears the buffer.
 * @context: FM Driver Context
 * @size: buffer size
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_buffer_set_size(
		void  *context,
		u8 size
		);

/**
 * fmd_rx_buffer_set_threshold() - RDS Buffer Threshold level in FM Chip.
 * Sets the group number at which the RDS buffer full interrupt must be
 * generated. The interrupt will be set after reception of the group.
 * @context: FM Driver Context
 * @threshold: threshold level.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_buffer_set_threshold(
		void  *context,
		u8 threshold
		);

/**
 * fmd_rx_set_rds() - Enables or disables demodulation of RDS data.
 * @context: FM Driver Context
 * @on_off_state : Rx Set ON /OFF control
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_rx_set_rds(
		void  *context,
		u8 on_off_state
		);

/**
 * fmd_rx_get_low_level_rds_groups() - Gets Low level RDS group data.
 * @context: FM Driver Context
 * @index: RDS group index
 * @rds_buf_count: Count for RDS buffer
 * @block1: (out) RDS Block 1
 * @block2: (out) RDS Block 2
 * @block3: (out) RDS Block 3
 * @block4: (out) RDS Block 4
 * @status1: (out) RDS data status 1
 * @status2: (out) RDS data status 2
 * @status3: (out) RDS data status 3
 * @status4: (out) RDS data status 4
 *  Returns:
 *	 FMD_RESULT_SUCCESS,  if no error.
 *	 FMD_RESULT_IO_ERROR, if there is an error.
 *       FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_rx_get_low_level_rds_groups(
		void  *context,
		u8 index,
		u8 *rds_buf_count,
		u16  *block1,
		u16  *block2,
		u16  *block3,
		u16  *block4,
		u8  *status1,
		u8  *status2,
		u8  *status3,
		u8  *status4
		);

/**
 * fmd_tx_set_pa() - Enables or disables the Power Amplifier.
 * @context: FM Driver Context
 * @on: Power Amplifier current state to set
 * Returns:
 *   FMD_RESULT_SUCCESS,  if set Power Amplifier done successfully.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_tx_set_pa(
		void  *context,
		bool on
		);

/**
 * fmd_tx_set_signal_strength() - Sets the RF-level of the output FM signal.
 * @context: FM Driver Context
 * @strength: Signal strength to be set for FM Tx in dBuV.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if set RSSI Level done successfully.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_PARAMETER_INVALID, if parameter is invalid.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_tx_set_signal_strength(
		void  *context,
		u16 strength
		);

/**
 * fmd_tx_get_signal_strength() - Retrieves current RSSI of FM Tx.
 * @context: FM Driver Context
 * @strength: (out) Strength of signal being transmitted in dBuV.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_tx_get_signal_strength(
		void  *context,
		u16  *strength
		);

/**
 * fmd_tx_set_freq_range() - Sets the FM band and specifies the custom band.
 * @context: FM Driver Context
 * @range: Freq range to set on FM Tx.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_PARAMETER_INVALID, if parameter is invalid.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_tx_set_freq_range(
		void  *context,
		u8 range
		);

/**
 * fmd_tx_get_freq_range() - Gets the FM band currently in use.
 * @context: FM Driver Context
 * @range: (out) Frequency Range set on Fm Tx.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_tx_get_freq_range(
		void  *context,
		u8 *range
		);

/**
 * fmd_tx_set_grid() - Sets the tuning grid size.
 * @context: FM Driver Context
 * @grid: FM Grid (50 Khz, 100 Khz, 200 Khz) to be set for FM Tx.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_PARAMETER_INVALID, if parameter is invalid.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_tx_set_grid(
		void  *context,
		u8 grid
		);

/**
 * fmd_tx_get_grid() - Gets the current tuning grid size.
 * @context: FM Driver Context
 * @grid: (out) FM Grid (50 Khz, 100 Khz, 200 Khz) currently set on FM Tx.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_tx_get_grid(
		void  *context,
		u8 *grid
		);

/**
 * fmd_tx_set_preemphasis() - Sets the Preemphasis characteristic of the Tx.
 * @context: FM Driver Context
 * @preemphasis: Pre-emphasis level to be set for FM Tx.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_tx_set_preemphasis(
		void  *context,
		u8 preemphasis
		);

/**
 * fmd_tx_get_preemphasis() - Gets the currently used Preemphasis char of th FM Tx.
 * @context: FM Driver Context.
 * @preemphasis: (out) Preemphasis Level used for FM Tx.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_tx_get_preemphasis(
		void  *context,
		u8 *preemphasis
		);

/**
 * fmd_tx_set_frequency() - Sets the FM Channel for Tx.
 * @context: FM Driver Context
 * @freq: Freq to be set for transmission.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if set frequency done successfully.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_tx_set_frequency(
		void  *context,
		u32 freq
		);

/**
 * fmd_rx_get_frequency() - Gets the currently used Channel for Tx.
 * @context: FM Driver Context
 * @freq: (out) Frequency set on FM Tx.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_tx_get_frequency(
		void  *context,
		u32  *freq
		);

/**
 * fmd_tx_enable_stereo_mode() - Sets Stereo mode state for TX.
 * @context: FM Driver Context
 * @enable_stereo_mode: Flag indicating enabling or disabling Stereo mode.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_tx_enable_stereo_mode(
		void  *context,
		bool enable_stereo_mode
		);

/**
 * fmd_tx_get_stereo_mode() - Gets the currently used FM Tx stereo mode.
 * @context: FM Driver Context
 * @stereo_mode: (out) Stereo Mode state set on FM Tx.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_tx_get_stereo_mode(
		void  *context,
		bool  *stereo_mode
		);

/**
 * fmd_tx_set_pilot_deviation() - Sets pilot deviation in HZ
 * @context: FM Driver Context
 * @deviation: Pilot deviation in HZ to set on FM Tx.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_tx_set_pilot_deviation(
		void  *context,
		u16 deviation
		);

/**
 * fmd_tx_get_pilot_deviation() - Retrieves the current pilot deviation.
 * @context: FM Driver Context
 * @deviation: (out) Pilot deviation set on FM Tx.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_tx_get_pilot_deviation(
		void  *context,
		u16  *deviation
		);

/**
 * fmd_tx_set_rds_deviation() - Sets Rds deviation in HZ.
 * @context: FM Driver Context
 * @deviation: RDS deviation in HZ.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_tx_set_rds_deviation(
		void  *context,
		u16 deviation
		);

/**
 * fmd_tx_get_rds_deviation() - Retrieves the current Rds deviation.
 * @context: FM Driver Context
 * @deviation: (out) RDS deviation currently set.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_tx_get_rds_deviation(
		void  *context,
		u16  *deviation
		);

/**
 * fmd_tx_set_rds() - Enables or disables RDS transmission for Tx.
 * @context: FM Driver Context
 * @on: Boolean - RDS ON
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_tx_set_rds(
		void  *context,
		bool on
		);

/**
 * fmd_rx_get_rds() - Gets the current status of RDS transmission for FM Tx.
 * @context: FM Driver Context
 * @on: (out) Rds enabled or disabled.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_tx_get_rds(
		void  *context,
		bool  *on
		);

/**
 * fmd_tx_set_group() - Programs a grp on a certain position in the RDS buffer.
 * @context: FM Driver Context
 * @position: RDS group position
 * @block1: Data to be transmitted in Block 1
 * @block2: Data to be transmitted in Block 2
 * @block3: Data to be transmitted in Block 3
 * @block4: Data to be transmitted in Block 4
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_tx_set_group(
		void  *context,
		u16 position,
		u8  block1[2],
		u8  block2[2],
		u8  block3[2],
		u8  block4[2]
		);

/**
 * fmd_tx_buffer_set_size() - Controls the size of the RDS buffer in groups.
 * @context: FM Driver Context
 * @buffer_size: RDS buffer size.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_tx_buffer_set_size(
		void  *context,
		u16 buffer_size
		);

/**
 * fmd_set_volume() - Sets the receive audio volume.
 * @context: FM Driver Context
 * @volume: Audio volume level
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_set_volume(
		void  *context,
		u8 volume
		);

/**
 * fmd_get_volume() - Retrives the current audio volume.
 * @context: FM Driver Context
 * @volume: Analog Volume level.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 */
u8 fmd_get_volume(
		void  *context,
		u8 *volume
		);

/**
 * fmd_set_balance() - Controls the receiver audio balance.
 * @context: FM Driver Context
 * @balance: Audio balance level
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_set_balance(
		void  *context,
		s8 balance
		);

/**
 * fmd_set_mute() - Enables or disables muting of the analog audio(DAC).
 * @context: FM Driver Context
 * @mute_on: bool of mute on
 * Returns:
 *   FMD_RESULT_SUCCESS,  if mute done successfully.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_set_mute(
		void  *context,
		bool mute_on
		);

/**
 * fmd_ext_set_mute() - Enables or disables muting of the audio channel.
 * @context: FM Driver Context
 * @mute_on: bool to Mute
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_ext_set_mute(
		void  *context,
		bool mute_on
		);

/**
 * fmd_power_up() - Puts the system in Powerup state.
 * @context: FM Driver Context
 * Returns:
 *   FMD_RESULT_SUCCESS,  if power up command sent successfully to chip.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_power_up(
		void  *context
		);

/**
 * fmd_goto_standby() - Puts the system in standby mode.
 * @context: FM Driver Context
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_goto_standby(
		void  *context
		);

/**
 * fmd_goto_power_down() - Puts the system in Powerdown mode.
 * @context: FM Driver Context
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_goto_power_down(
		void  *context
		);

/**
 * fmd_select_ref_clk() - Selects the FM reference clock.
 * @context: FM Driver Context
 * @ref_clk: Ref Clock.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_select_ref_clk(
		void  *context,
		u16 ref_clk
		);

/**
 * fmd_set_ref_clk_pll() - Sets the freq of Referece Clock.
 * Sets frequency and offset correction properties of the external
 * reference clock of the PLL
 * @context: FM driver context.
 * @freq: PLL Frequency/ 2 in kHz.
 * Returns:
 *   FMD_RESULT_SUCCESS,  if no error.
 *   FMD_RESULT_IO_ERROR, if there is an error.
 *   FMD_RESULT_BUSY, if FM Driver is not in idle state.
 *   FMD_RESULT_RESPONSE_WRONG, if wrong response received from chip.
 */
u8 fmd_set_ref_clk_pll(
		void  *context,
		u16 freq
		);

/**
 * fmd_send_fm_ip_enable()- Enables the FM IP.
 * Returns:
 *   0,  if operation completed successfully.
 *   1, otherwise.
 */
u8 fmd_send_fm_ip_enable(void);

/**
 * fmd_send_fm_ip_disable()- Disables the FM IP.
 * Returns:
 *   0,  if operation completed successfully.
 *   1, otherwise.
 */
u8 fmd_send_fm_ip_disable(void);

/**
 * fmd_send_fm_firmware() - Send the FM Firmware File to Device.
 * @fw_buffer: Firmware to be downloaded.
 * @fw_size: Size of firmware to be downloaded.
 * Returns:
 *   0,  if operation completed successfully.
 *   1, otherwise.
 */
u8 fmd_send_fm_firmware(
		u8 *fw_buffer,
		u16 fw_size
		);

/**
 * fmd_receive_data() - Processes the FM data received from device.
 * @packet_length: Length of received Data Packet
 * @packet_buffer: Received Data buffer.
 */
void fmd_receive_data(
		u16 packet_length,
		u8 *packet_buffer
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
 * @buffer: Buffer to be displayed.
 * @num_bytes: Number of bytes of the buffer.
 */
void fmd_hexdump(
		char prompt,
		u8 *buffer,
		int num_bytes
		);

/**
 * fmd_isr() - FM Interrupt Service Routine.
 * This function processes the Interrupt received from chip and performs the
 * necessary action.
 */
void fmd_isr(void);


#endif /* _FMDRIVER_H_  */
