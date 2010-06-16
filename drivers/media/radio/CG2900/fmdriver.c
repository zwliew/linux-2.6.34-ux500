/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Linux FM Driver for CG2900 FM Chip
 *
 * Author: Hemant Gupta/hemant.gupta@stericsson.com for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2
 */

#include <stdarg.h>
#include <string.h>
#include "fmdriver.h"
#include "platformosapi.h"

#define MAX_COUNT_OF_IRQS 16
#define CHANNEL_FREQUENCY_CONVERTER 50
#define MAX_NAME_SIZE	50
#define MIN_POWER_LEVEL 88
#define MAX_POWER_LEVEL 123
#define DEFAULT_RDS_DEVIATION 200
#define MAX_RDS_DEVIATION 750
#define DEFAULT_PILOT_DEVIATION 675
#define MAX_PILOT_DEVIATION 1000
#define DEFAULT_RSSI_THRESHOLD 0x0100
#define DEFAULT_PEAK_NOISE_VALUE 0x0035
#define AVERAGE_NOISE_MAX_VALUE 0x0030

#define ASCVAL(x)(((x) <= 9) ? (x) + '0' : (x) - 10 + 'a')

/**
 * enum fmd_gocmd_t - FM Driver Command state.
 * @FMD_STATE_NONE: FM Driver in Idle state
 * @FMD_STATE_MODE: FM Driver in setmode state
 * @FMD_STATE_FREQUENCY: FM Driver in Set frequency state.
 * @FMD_STATE_PA: FM Driver in SetPA state.
 * @FMD_STATE_PA_LEVEL: FM Driver in Setpalevl state.
 * @FMD_STATE_ANTENNA: FM Driver in Setantenna state
 * @FMD_STATE_MUTE: FM Driver in Setmute state
 * @FMD_STATE_SEEK: FM Driver in seek mode
 * @FMD_STATE_SEEK_STOP: FM Driver in seek stop level state.
 * @FMD_STATE_SCAN_BAND: FM Driver in Scanband mode
 * @FMD_STATE_TX_SET_CTRL: FM Driver in RDS control state
 * @FMD_STATE_TX_SET_THRSHLD: FM Driver in RDS threshld state
 * @FMD_STATE_GEN_POWERUP: FM Driver in Power UP state.
 * @FMD_STATE_SELECT_REF_CLK: FM Driver in Select Reference clock state.
 * @FMD_STATE_SET_REF_CLK_PLL: FM Driver in Set Reference Freq state.
 * @FMD_STATE_BLOCK_SCAN: FM Driver in Block Scan state.
 * @FMD_STATE_AF_UPDATE: FM Driver in AF Update State.
 * @FMD_STATE_AF_SWITCH: FM Driver in AF Switch State.
 * @FMD_STATE_MONOSTEREO_TRANSITION:FM Driver in Monostereo transition state.
 * Various states of the FM driver.
 */
enum fmd_gocmd_t {
	FMD_STATE_NONE,
	FMD_STATE_MODE,
	FMD_STATE_FREQUENCY,
	FMD_STATE_PA,
	FMD_STATE_PA_LEVEL,
	FMD_STATE_ANTENNA,
	FMD_STATE_MUTE,
	FMD_STATE_SEEK,
	FMD_STATE_SEEK_STOP,
	FMD_STATE_SCAN_BAND,
	FMD_STATE_TX_SET_CTRL,
	FMD_STATE_TX_SET_THRSHLD,
	FMD_STATE_GEN_POWERUP,
	FMD_STATE_SELECT_REF_CLK,
	FMD_STATE_SET_REF_CLK_PLL,
	FMD_STATE_BLOCK_SCAN,
	FMD_STATE_AF_UPDATE,
	FMD_STATE_AF_SWITCH,
	FMD_STATE_MONOSTEREO_TRANSITION
};

/**
 * struct fmd_rdsgroup_t - Rds group structure.
 * @block: Array for RDS Block(s) received.
 * @status: Array of Status of corresponding RDS block(s).
 * It stores the value and status of a particular RDS group
 * received.
 */
struct fmd_rdsgroup_t {
	u16 block[NUM_OF_RDS_BLOCKS];
	u8 status[NUM_OF_RDS_BLOCKS];
} __attribute__ ((packed));

/**
 * struct struct fmd_state_info_t - Main FM state info structure.
 * @fmd_initalized: Flag indicating FM Driver is initialized or not
 * @rx_freqrange: Receiver freq range
 * @rx_volume:  Receiver volume level
 * @rx_antenna: Receiver Antenna
 * @rdsbufcnt:  Number of RDS Groups available
 * @rx_seek_stoplevel:  RDS seek stop Level
 * @rx_rdson:  Receiver RDS ON
 * @rx_stereomode: Receiver Stereo mode
 * @max_channels_to_scan: Maximum Number of channels to Scan.
 * @tx_freqrange: Transmitter freq Range
 * @tx_preemphasis: Transmitter Pre emphiasis level
 * @tx_stereomode: Transmitter stero mode
 * @tx_rdson:  Enable RDS
 * @tx_pilotdev: PIlot freq deviation
 * @tx_rdsdev:  RDS deviation
 * @tx_strength:  TX Signal Stregnth
 * @irq_index:  Index where last interrupt is added to Interrupt queue
 * @interrupt_available_for_processing:  Flag indicating if interrupt is
 * available for processing or not.
 * @interrupt_queue: Circular Queue to store the received interrupt from chip.
 * @gocmd:  Command which is in progress.
 * @rdsgroup:  Array of RDS group Buffer
 * @callback: Callback registered by upper layers.
 */
struct fmd_state_info_t {
	bool fmd_initalized;
	u8 rx_freqrange;
	u8 rx_volume;
	u8 rx_antenna;
	u8 rdsbufcnt;
	u16 rx_seek_stoplevel;
	bool rx_rdson;
	u8 rx_stereomode;
	u8 tx_freqrange;
	u8 tx_preemphasis;
	bool tx_stereomode;
	u8 max_channels_to_scan;

	bool tx_rdson;
	u16 tx_pilotdev;
	u16 tx_rdsdev;
	u16 tx_strength;

	u8 irq_index;
	bool interrupt_available_for_processing;
	u16 interrupt_queue[MAX_COUNT_OF_IRQS];
	enum fmd_gocmd_t gocmd;
	struct fmd_rdsgroup_t rdsgroup[MAX_RDS_GROUPS];
	fmd_radio_cb callback;
} __attribute__ ((packed));

/**
 * struct fmd_data_t - Main structure for FM data exchange.
 * @cmd_id: Command Id of the command being exchanged.
 * @num_parameters: Number of parameters
 * @parameters: FM data parameters.
 */
struct fmd_data_t {
	u32 cmd_id;
	u16 num_parameters;
	u8 *parameters;
} __attribute__ ((packed));

static struct fmd_state_info_t fmd_state_info;
static struct fmd_data_t fmd_data;

static char event_name[FMD_EVENT_LAST_ELEMENT][MAX_NAME_SIZE] = {
	"FMD_EVENT_ANTENNA_STATUS_CHANGED",
	"FMD_EVENT_FREQUENCY_CHANGED",
	"FMD_EVENT_SEEK_COMPLETED",
	"FMD_EVENT_SCAN_BAND_COMPLETED",
	"FMD_EVENT_BLOCK_SCAN_COMPLETED",
	"FMD_EVENT_AF_UPDATE_SWITCH_COMPLETE",
	"FMD_EVENT_MONOSTEREO_TRANSITION_COMPLETE",
	"FMD_EVENT_SEEK_STOPPED",
	"FMD_EVENT_GEN_POWERUP",
	"FMD_EVENT_RDSGROUP_RCVD",
};

static char interrupt_name[MAX_COUNT_OF_IRQS][MAX_NAME_SIZE] = {
	"irpt_OperationSucceeded",
	"irpt_OperationFailed",
	"-",
	"irpt_BufferFull/Empty",
	"irpt_SignalQualityLow/MuteStatusChanged",
	"irpt_MonoStereoTransition",
	"irpt_RdsSyncFound/InputOverdrive",
	"irpt_RdsSyncLost",
	"irpt_PiCodeChanged",
	"irpt_RequestedBlockAvailable",
	"-",
	"-",
	"-",
	"-",
	"irpt_WarmBootReady",
	"irpt_ColdBootReady",
};

/* ------------------ Internal function declarations ---------------------- */
static void fmd_event_name(u8 event, char *event_name);
static void fmd_interrupt_name(u16 interrupt, char *interrupt_name);
static void fmd_add_interrupt_to_queue(u16 interrupt);
static void fmd_process_interrupt(u16 interrupt);
static void fmd_callback(void *context, u8 event, bool event_successful);
static u16 fmd_rx_frequency_to_channel(void *context, u32 freq);
static u32 fmd_rx_channel_to_frequency(void *context, u16 channel_number);
static u16 fmd_tx_frequency_to_channel(void *context, u32 freq);
static u32 fmd_tx_channel_to_frequency(void *context, u16 channel_number);
static bool fmd_go_cmd_busy(void);
static u8 fmd_send_cmd_and_read_resp(const u16 cmd_id,
				     const u16 num_parameters,
				     const u16 *parameters,
				     u16 *resp_num_parameters,
				     u16 *resp_parameters);
static u8 fmd_send_cmd(const u16 cmd_id,
		       const u16 num_parameters, const u16 *parameters);
static u8 fmd_read_resp(u16 *cmd_id, u16 *num_parameters, u16 *parameters);
static void fmd_process_fm_function(u8 *packet_buffer);
static u8 fmd_write_file_block(u32 file_block_id,
			       u8 *file_block, u16 file_block_length);

/* ------------------ Internal function definitions ---------------------- */

/**
  * fmd_event_name() - Converts the event to a displayable string.
  * @event: Event that has occurred.
  * @eventname: (out) Buffer to store event name.
  */
static void fmd_event_name(u8 event, char *eventname)
{
	if (eventname != NULL) {
		if (event < FMD_EVENT_LAST_ELEMENT)
			strcpy(eventname, event_name[event]);
		else
			strcpy(eventname, "FMD_EVENT_UNKNOWN");
	} else {
		FM_ERR_REPORT("fmd_event_name: Output Buffer is NULL!!!");
	}
}

/**
  * fmd_interrupt_name() - Converts the interrupt to a displayable string.
  * @interrupt: interrupt received from FM Chip
  * @interruptname: (out) Buffer to store interrupt name.
  */
static void fmd_interrupt_name(u16 interrupt, char *interruptname)
{
	int index;

	if (interruptname != NULL) {
		/* Convert Interrupt to Bit */
		for (index = 0; index < MAX_COUNT_OF_IRQS; index++) {
			if (interrupt & (1 << index)) {
				/* Match found, break the loop */
				break;
			}
		}
		if (index < MAX_COUNT_OF_IRQS)
			strcpy(interruptname, interrupt_name[index]);
		else
			strcpy(interruptname, "irpt_Unknown");
	} else {
		FM_ERR_REPORT("fmd_interrupt_name: Output Buffer is NULL!!!");
	}
}

/**
  * fmd_add_interrupt_to_queue() - Add interrupt to IRQ Queue.
  * @interrupt: interrupt received from FM Chip
  */
static void fmd_add_interrupt_to_queue(u16 interrupt)
{
	FM_DEBUG_REPORT("fmd_add_interrupt_to_queue : "
			"Interrupt Received = %04x", (u16) interrupt);

	/* Reset the index if it reaches the array limit */
	if (fmd_state_info.irq_index > MAX_COUNT_OF_IRQS - 1) {
		os_lock();
		fmd_state_info.irq_index = 0;
		os_unlock();
	}

	os_lock();
	fmd_state_info.interrupt_queue[fmd_state_info.irq_index] = interrupt;
	fmd_state_info.irq_index++;
	os_unlock();
	if (STE_FALSE == fmd_state_info.interrupt_available_for_processing) {
		os_lock();
		fmd_state_info.interrupt_available_for_processing = STE_TRUE;
		os_unlock();
		os_set_interrupt_sem();
	}
}

/**
  * fmd_process_interrupt() - Processes the Interrupt.
  * This function processes the interrupt received from FM Chip
  * and calls the corresponding callback registered by upper layers with
  * proper parameters.
  * @interrupt: interrupt received from FM Chip
  */
static void fmd_process_interrupt(u16 interrupt)
{
	char irpt_name[MAX_NAME_SIZE];

	fmd_interrupt_name(interrupt, irpt_name);
	FM_DEBUG_REPORT("%s", irpt_name);

	if (interrupt & IRPT_OPERATIONSUCCEEDED) {
		switch (fmd_state_info.gocmd) {
		case FMD_STATE_ANTENNA:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_ANTENNA;
			os_set_cmd_sem();
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_ANTENNA_STATUS_CHANGED,
				     STE_TRUE);
			break;
		case FMD_STATE_MODE:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_MODE;
			os_set_cmd_sem();
			break;
		case FMD_STATE_MUTE:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_MUTE;
			os_set_cmd_sem();
			break;
		case FMD_STATE_FREQUENCY:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_FREQUENCY;
			os_set_cmd_sem();
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_FREQUENCY_CHANGED, STE_TRUE);
			break;
		case FMD_STATE_SEEK:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_SEEK;
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_SEEK_COMPLETED, STE_TRUE);
			break;
		case FMD_STATE_SCAN_BAND:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_SCAN_BAND;
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_SCAN_BAND_COMPLETED, STE_TRUE);
			break;
		case FMD_STATE_SEEK_STOP:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_SEEK_STOP;
			os_set_cmd_sem();
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_SEEK_STOPPED, STE_TRUE);
			break;
		case FMD_STATE_PA:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_PA;
			os_set_cmd_sem();
			break;
		case FMD_STATE_PA_LEVEL:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_PA_LEVEL;
			os_set_cmd_sem();
			break;

		case FMD_STATE_GEN_POWERUP:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_GEN_POWERUP;
			os_set_cmd_sem();
			break;
		case FMD_STATE_SELECT_REF_CLK:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_SELECT_REF_CLK;
			os_set_cmd_sem();
			break;
		case FMD_STATE_SET_REF_CLK_PLL:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_SET_REF_CLK_PLL;
			os_set_cmd_sem();
			break;
		case FMD_STATE_BLOCK_SCAN:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_BLOCK_SCAN;
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_BLOCK_SCAN_COMPLETED, STE_TRUE);
			break;
		case FMD_STATE_AF_UPDATE:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_AF_UPDATE;
			os_set_cmd_sem();
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_AF_UPDATE_SWITCH_COMPLETE,
				     STE_TRUE);
			break;
		case FMD_STATE_AF_SWITCH:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_AF_SWITCH;
			os_set_cmd_sem();
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_AF_UPDATE_SWITCH_COMPLETE,
				     STE_TRUE);
			break;
		case FMD_STATE_TX_SET_CTRL:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_TX_SET_CTRL;
			os_set_cmd_sem();
			break;
		case FMD_STATE_TX_SET_THRSHLD:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_TX_SET_THRSHLD;
			os_set_cmd_sem();
			break;
		default:
			/* Clear the variable */
			fmd_state_info.gocmd = FMD_STATE_NONE;
			break;

		}
	}

	if (interrupt & IRPT_OPERATIONFAILED) {
		switch (fmd_state_info.gocmd) {

		case FMD_STATE_SEEK:
			/* Signal host */
			fmd_state_info.gocmd = FMD_STATE_NONE;
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_SEEK_COMPLETED, STE_FALSE);
			break;

		case FMD_STATE_SCAN_BAND:
			/* Signal host */
			fmd_state_info.gocmd = FMD_STATE_NONE;
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_SCAN_BAND_COMPLETED, STE_FALSE);
			break;

		case FMD_STATE_SEEK_STOP:
			/* Signal host */
			fmd_state_info.gocmd = FMD_STATE_NONE;
			os_set_cmd_sem();
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_SEEK_STOPPED, STE_FALSE);
			break;

		case FMD_STATE_BLOCK_SCAN:
			/* Signal host */
			fmd_state_info.gocmd = FMD_STATE_NONE;
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_BLOCK_SCAN_COMPLETED, STE_FALSE);
			break;

		case FMD_STATE_AF_UPDATE:
		case FMD_STATE_AF_SWITCH:
			/* Signal host */
			fmd_state_info.gocmd = FMD_STATE_NONE;
			os_set_cmd_sem();
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_AF_UPDATE_SWITCH_COMPLETE,
				     STE_FALSE);
			break;

		default:
			/* Clear */
			fmd_state_info.gocmd = FMD_STATE_NONE;
			break;
		}
	}

	if (interrupt & IRPT_RX_BUFFERFULL_TX_BUFFEREMPTY) {
		fmd_callback((void *)&fmd_state_info,
			     FMD_EVENT_RDSGROUP_RCVD, STE_TRUE);
	}

	if (interrupt & IRPT_RX_MONOSTEREO_TRANSITION) {
		switch (fmd_state_info.gocmd) {
		case FMD_STATE_MONOSTEREO_TRANSITION:
			/* Signal host */
			fmd_state_info.gocmd &=
			    ~FMD_STATE_MONOSTEREO_TRANSITION;
			os_set_cmd_sem();
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_MONOSTEREO_TRANSITION_COMPLETE,
				     STE_TRUE);
			break;
		default:

			break;
		}
	}
	if (interrupt & IRPT_COLDBOOTREADY) {
		switch (fmd_state_info.gocmd) {
		case FMD_STATE_GEN_POWERUP:
			/* Signal host */
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_GEN_POWERUP, STE_TRUE);
			break;
		default:
			/* Do Nothing */
			break;
		}
	}

	if (interrupt & IRPT_WARMBOOTREADY) {
		switch (fmd_state_info.gocmd) {
		case FMD_STATE_GEN_POWERUP:
			/* Signal host */
			fmd_state_info.gocmd &= ~FMD_STATE_GEN_POWERUP;
			os_set_cmd_sem();
			fmd_callback((void *)&fmd_state_info,
				     FMD_EVENT_GEN_POWERUP, STE_TRUE);
			break;
		default:
			/* Do Nothing */
			break;
		}
	}
}

/**
  * fmd_callback() - Callback function for upper layers.
  * Callback function that calls the registered callback of upper
  * layers with proper parameters.
  * @context: Context of the FM Driver
  * @event: event for which the callback function was called
  * from FM Driver.
  * @event_successful: Signifying whether the event is called from FM
  * Driver on receiving irpt_OperationSucceeded or irpt_OperationFailed.
  */
static void fmd_callback(void *context, u8 event, bool event_successful)
{
	char event_name_string[40];
	fmd_event_name(event, event_name_string);
	FM_DEBUG_REPORT("%s %x, %d", event_name_string,
			(unsigned int)event, (unsigned int)event_successful);

	if (fmd_state_info.callback)
		fmd_state_info.callback((void *)&context,
					event, event_successful);
}

/**
  * fmd_rx_frequency_to_channel() - Converts frequency to channel number.
  * Converts the Frequency in kHz to corresponding Channel number.
  * This is used for FM Rx.
  * @context: Context of the FM Driver.
  * @freq: Frequency in kHz.
  *
  * Returns:
  *   Channel Number corresponding to the given Frequency.
  */
static u16 fmd_rx_frequency_to_channel(void *context, u32 freq)
{
	u8 range;
	u32 minfreq;
	u32 maxfreq;

	fmd_get_freq_range(context, &range);
	fmd_get_freq_range_properties(context, range, &minfreq, &maxfreq);

	if (freq > maxfreq)
		freq = maxfreq;
	else if (freq < minfreq)
		freq = minfreq;

	/* Frequency in kHz needs to be divided with 50 kHz to get
	 * channel number for all FM Bands */

	return (u16) ((freq - minfreq) / CHANNEL_FREQUENCY_CONVERTER);
}

/**
  * fmd_rx_channel_to_frequency() - Converts Channel number to frequency.
  * Converts the Channel Number to corresponding Frequency in kHz.
  * This is used for FM Rx.
  * @context: Context of the FM Driver.
  * @channel_number: Channel Number to be converted.
  *
  * Returns:
  *   Frequency corresponding to the corresponding channel in kHz.
  */
static u32 fmd_rx_channel_to_frequency(void *context, u16 channel_number)
{
	u8 range;
	u32 freq;
	u32 minfreq;
	u32 maxfreq;

	fmd_get_freq_range(context, &range);
	fmd_get_freq_range_properties(context, range, &minfreq, &maxfreq);

	/* Channel Number needs to be multiplied with 50 kHz to get
	 * frequency in kHz for all FM Bands */

	freq = minfreq + (channel_number * CHANNEL_FREQUENCY_CONVERTER);

	if (freq > maxfreq)
		freq = maxfreq;
	else if (freq < minfreq)
		freq = minfreq;

	return freq;
}

/**
  * fmd_tx_frequency_to_channel() - Converts frequency to channel number.
  * Converts the Frequency in kHz to corresponding Channel number.
  * This is used for FM Tx.
  * @context: Context of the FM Driver.
  * @freq: Frequency in kHz.
  *
  * Returns:
  *   Channel Number corresponding to the given Frequency.
  */
static u16 fmd_tx_frequency_to_channel(void *context, u32 freq)
{
	u8 range;
	u32 minfreq;
	u32 maxfreq;

	fmd_tx_get_freq_range(context, &range);
	fmd_get_freq_range_properties(context, range, &minfreq, &maxfreq);

	if (freq > maxfreq)
		freq = maxfreq;
	else if (freq < minfreq)
		freq = minfreq;

	/* Frequency in kHz needs to be divided with 50 kHz to get
	 * channel number for all FM Bands */

	return (u16) ((freq - minfreq) / CHANNEL_FREQUENCY_CONVERTER);
}

/**
  * fmd_tx_channel_to_frequency() - Converts Channel number to frequency.
  * Converts the Channel Number to corresponding Frequency in kHz.
  * This is used for FM Tx.
  * @context: Context of the FM Driver.
  * @channel_number: Channel Number to be converted.
  *
  * Returns:
  *   Frequency corresponding to the corresponding channel in kHz.
  */
static u32 fmd_tx_channel_to_frequency(void *context, u16 channel_number)
{
	u8 range;
	u32 freq;
	u32 minfreq;
	u32 maxfreq;

	fmd_tx_get_freq_range(context, &range);
	fmd_get_freq_range_properties(context, range, &minfreq, &maxfreq);

	/* Channel Number needs to be multiplied with 50 kHz to get
	 * frequency in kHz for all FM Bands */

	freq = minfreq + (channel_number * CHANNEL_FREQUENCY_CONVERTER);

	if (freq > maxfreq)
		freq = maxfreq;
	else if (freq < minfreq)
		freq = minfreq;

	return freq;
}

/**
  * fmd_go_cmd_busy() - Function to check if FM Driver is busy or idle
  *
  * Returns:
  *   0 if FM Driver is Idle
  *   1 otherwise
  */
static bool fmd_go_cmd_busy(void)
{
	return (fmd_state_info.gocmd != FMD_STATE_NONE);
}

/**
  * fmd_send_cmd_and_read_resp() - Send command and read response.
  * This function sends the HCI Command to Protocol Driver and
  * Reads back the Response Packet.
  * @cmd_id: Command Id to be sent to FM Chip.
  * @num_parameters: Number of parameters of the command sent.
  * @parameters: Buffer containing the Buffer to be sent.
  * @resp_num_parameters: (out) Number of paramters of the response packet.
  * @resp_parameters: (out) Buffer of the response packet.
  *
  * Returns:
  *   FMD_RESULT_SUCCESS: If the command is sent successfully and the
  *   response received is also correct.
  *   FMD_RESULT_RESPONSE_WRONG: If the received response is not correct.
  */
static u8 fmd_send_cmd_and_read_resp(const u16 cmd_id,
				     const u16 num_parameters,
				     const u16 *parameters,
				     u16 *resp_num_parameters,
				     u16 *resp_parameters)
{
	u8 result;
	u16 readCmdID;

	result = fmd_send_cmd(cmd_id, num_parameters, parameters);
	if (result == FMD_RESULT_SUCCESS) {
		result = fmd_read_resp(&readCmdID, resp_num_parameters,
				       resp_parameters);
		if (result == FMD_RESULT_SUCCESS) {
			/* Check that the response belongs to the sent
			 * command */
			if (readCmdID != cmd_id)
				result = FMD_RESULT_RESPONSE_WRONG;
		}
	}
	FM_DEBUG_REPORT("fmd_send_cmd_and_read_resp: " "returning %d", result);
	return result;
}

/**
  * fmd_send_cmd() - This function sends the HCI Command
  * to Protocol Driver.
  * @cmd_id: Command Id to be sent to FM Chip.
  * @num_parameters: Number of parameters of the command sent.
  * @parameters: Buffer containing the Buffer to be sent.
  *
  * Returns:
  *   0: If the command is sent successfully to Lower Layers.
  *   1: Otherwise
  */
static u8 fmd_send_cmd(const u16 cmd_id,
		       const u16 num_parameters, const u16 *parameters)
{
	/* Total Length includes 5 bytes HCI Header, 2 bytes to store
	 * number of parameters and remaining bytes depending on
	 * number of paramters */
	u16 total_length = 2 + num_parameters * sizeof(u16) + 5;
	u8 *fm_data = (u8 *) os_mem_alloc(total_length);
	u16 cmd_id_temp;
	u8 err = 1;

	if (fm_data != NULL) {
		/* Wait till response of the last command is received */
		os_get_hal_sem();
		/* HCI encapsulation */
		fm_data[0] = HCI_PACKET_INDICATOR_FM_CMD_EVT;
		fm_data[1] = total_length - 2;
		fm_data[2] = CATENA_OPCODE;
		fm_data[3] = FM_WRITE;
		fm_data[4] = FM_FUNCTION_WRITECOMMAND;

		cmd_id_temp = cmd_id << 3;
		fm_data[5] = (u8) cmd_id_temp;
		fm_data[6] = cmd_id_temp >> 8;
		fm_data[5] |= num_parameters;

		os_mem_copy((fm_data + 7),
			    (void *)parameters, num_parameters * sizeof(u16));

		/* Send the Packet */
		err = ste_fm_send_packet(total_length, fm_data);

		os_mem_free(fm_data);
	}

	return err;
}

/**
  * fmd_read_resp() - This function reads the response packet of the previous
  * command sent to FM Chip and copies it to the buffer provided as parameter.
  * @cmd_id: (out) Command Id received from FM Chip.
  * @num_parameters: (out) Number of paramters of the response packet.
  * @parameters: (out) Buffer of the response packet.
  *
  * Returns:
  *   0: If the response buffer is copied successfully.
  *   1: Otherwise
  */
static u8 fmd_read_resp(u16 *cmd_id, u16 *num_parameters, u16 *parameters)
{
	/* Wait till response of the last command is received */
	os_get_read_response_sem();

	/* Fill the arguments */
	*cmd_id = fmd_data.cmd_id;
	*num_parameters = fmd_data.num_parameters;
	if (*num_parameters)
		os_mem_copy(parameters, fmd_data.parameters,
			    (fmd_data.num_parameters * 2));

	/* Response received, release semaphore so that a new command
	 * could be sent to chip */
	os_set_hal_sem();

	return 0;
}

/**
  * fmd_process_fm_function() - Process FM Function.
  * This function processes the Response buffer received
  * from lower layers for the FM function and performs the necessary action to
  * parse the same.
  * @packet_buffer: Received Buffer.
  */
static void fmd_process_fm_function(u8 *packet_buffer)
{
	switch (packet_buffer[0]) {
	case FM_FUNCTION_ENABLE:
		{
			FM_DEBUG_REPORT("FM_FUNCTION_ENABLE ,"
					"command success received");
			os_set_cmd_sem();
		}
		break;
	case FM_FUNCTION_DISABLE:
		{
			FM_DEBUG_REPORT("FM_FUNCTION_DISABLE ,"
					"command success received");
			os_set_cmd_sem();
		}
		break;
	case FM_FUNCTION_RESET:
		{
			FM_DEBUG_REPORT("FM_FUNCTION_RESET ,"
					"command success received");
			os_set_cmd_sem();
		}
		break;
	case FM_FUNCTION_WRITECOMMAND:
		{
			/* flip the header bits to take into account
			 * of little endian format */
			fmd_data.cmd_id = packet_buffer[2] << 8 |
			    packet_buffer[1];
			fmd_data.num_parameters = fmd_data.cmd_id & 0x07;
			fmd_data.cmd_id = fmd_data.cmd_id >> 3;

			if (fmd_data.num_parameters) {
				fmd_data.parameters = &packet_buffer[3];
				os_mem_copy(fmd_data.parameters,
					    &packet_buffer[3],
					    fmd_data.num_parameters * 2);
			}
			/* Release the semaphore so that upper layer
			 * can process the received event */
			os_set_read_response_sem();
		}
		break;
	case FM_FUNCTION_SETINTMASKALL:
		break;
	case FM_FUNCTION_GETINTMASKALL:
		{
			FM_DEBUG_REPORT("FM_FUNCTION_GETINTMASKALL ,"
					"command success received");
			os_set_cmd_sem();
		}
		break;

	case FM_FUNCTION_SETINTMASK:
		break;
	case FM_FUNCTION_GETINTMASK:
		{
			FM_DEBUG_REPORT("FM_FUNCTION_GETINTMASK ,"
					"command success received");
			os_set_cmd_sem();
		}
		break;

	case FM_FUNCTION_FMFWDOWNLOAD:
		{
			FM_DEBUG_REPORT("fmd_process_fm_function: "
					"FM_FUNCTION_FMFWDOWNLOAD,"
					"Block Id = %02x", packet_buffer[1]);
			os_set_cmd_sem();
		}
		break;

	default:
		{
			FM_DEBUG_REPORT("fmd_process_fm_function: "
					"default case, FM Func = %d",
					packet_buffer[0]);
		}
		break;
	}
}

/**
  * fmd_write_file_block() - download firmware.
  * This Function adds the header for downloading
  * the firmware and coeffecient files and sends it to Protocol Driver.
  * @file_block_id: Block ID of the F/W to be transmitted to FM Chip
  * @file_block: Buffer containing the bytes to be sent.
  * @file_block_length: Size of the Firmware buffer.
  */
static u8 fmd_write_file_block(u32 file_block_id,
			       u8 *file_block, u16 file_block_length)
{
	u8 err = 0;

	file_block[0] = HCI_PACKET_INDICATOR_FM_CMD_EVT;
	file_block[1] = file_block_length + 4;
	file_block[2] = CATENA_OPCODE;
	file_block[3] = FM_WRITE;
	file_block[4] = FM_FUNCTION_FMFWDOWNLOAD;
	file_block[5] = file_block_id;
	/* Send the Packet */
	err = ste_fm_send_packet(file_block_length + 6, file_block);

	/* wait till response comes */
	os_get_cmd_sem();

	return err;
}

/* ------------------ External function definitions ---------------------- */

void fmd_isr(void)
{
	int index = 0;

	FM_DEBUG_REPORT("+fmd_isr");

	if (fmd_state_info.interrupt_available_for_processing == STE_FALSE &&
	    fmd_state_info.fmd_initalized == STE_TRUE) {
		FM_DEBUG_REPORT("fmd_isr: Waiting on irq sem "
				"interrupt_available_for_processing = %d "
				"fmd_state_info.fmd_initalized = %d",
				fmd_state_info.
				interrupt_available_for_processing,
				fmd_state_info.fmd_initalized);
		os_get_interrupt_sem();
		FM_DEBUG_REPORT("fmd_isr: Waiting on irq sem "
				"interrupt_available_for_processing = %d "
				"fmd_state_info.fmd_initalized = %d",
				fmd_state_info.
				interrupt_available_for_processing,
				fmd_state_info.fmd_initalized);
	}

	if (fmd_state_info.interrupt_available_for_processing == STE_TRUE
	    && fmd_state_info.fmd_initalized == STE_TRUE) {
		while (index < MAX_COUNT_OF_IRQS) {
			if (fmd_state_info.interrupt_queue[index]
			    != IRPT_INVALID) {
				FM_DEBUG_REPORT("fmd_process_interrupt "
						"Interrupt = %04x",
						fmd_state_info.
						interrupt_queue[index]);
				fmd_process_interrupt(fmd_state_info.
						      interrupt_queue[index]);
				fmd_state_info.interrupt_queue[index]
				    = IRPT_INVALID;
			}
			index++;
		}
	}
	fmd_state_info.interrupt_available_for_processing = STE_FALSE;

	FM_DEBUG_REPORT("-fmd_isr");
}

u8 fmd_init(void **context)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	*context = (void *)&fmd_state_info;

	if (os_fm_driver_init() == 0) {
		memset(&fmd_state_info, 0, sizeof(fmd_state_info));
		fmd_state_info.fmd_initalized = STE_TRUE;
		fmd_state_info.gocmd = FMD_STATE_NONE;
		fmd_state_info.callback = NULL;
		fmd_state_info.rx_freqrange = FMD_FREQRANGE_EUROAMERICA;
		fmd_state_info.rx_stereomode = FMD_STEREOMODE_BLENDING;
		fmd_state_info.rx_volume = MAX_ANALOG_VOLUME;
		fmd_state_info.rx_antenna = FMD_ANTENNA_EMBEDDED;
		fmd_state_info.rx_rdson = STE_FALSE;
		fmd_state_info.rx_seek_stoplevel = DEFAULT_RSSI_THRESHOLD;
		fmd_state_info.tx_freqrange = FMD_FREQRANGE_EUROAMERICA;
		fmd_state_info.tx_preemphasis = FMD_EMPHASIS_75US;
		fmd_state_info.tx_pilotdev = DEFAULT_PILOT_DEVIATION;
		fmd_state_info.tx_rdsdev = DEFAULT_RDS_DEVIATION;
		fmd_state_info.tx_strength = MAX_POWER_LEVEL;
		fmd_state_info.max_channels_to_scan = DEFAULT_CHANNELS_TO_SCAN;
		fmd_state_info.tx_stereomode = STE_TRUE;
		fmd_state_info.irq_index = 0;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	FM_DEBUG_REPORT("fmd_init returning = %d", err);
	return err;
}

void fmd_deinit(void)
{
	os_set_interrupt_sem();
	os_fm_driver_deinit();
	memset(&fmd_state_info, 0, sizeof(fmd_state_info));
}

u8 fmd_register_callback(void *context, fmd_radio_cb callback)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		state->callback = callback;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_get_version(void *context, u16 version[7]
    )
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 response_count;
		u16 response_data[32];

		io_result = fmd_send_cmd_and_read_resp(CMD_GEN_GETVERSION, 0,
						       NULL, &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {
			os_mem_copy(version, response_data, sizeof(u16) * 7);
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_set_mode(void *context, u8 mode)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	u8 io_result;
	u16 response_count;
	u16 response_data[32];

	if (mode > FMD_MODE_TX) {
		err = FMD_RESULT_PARAMETER_INVALID;
	} else if (fmd_state_info.fmd_initalized) {
		u16 parameters[1];
		parameters[0] = mode;

		state->gocmd = FMD_STATE_MODE;
		io_result = fmd_send_cmd_and_read_resp(CMD_GEN_GOTOMODE, 1,
						       parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			state->gocmd = FMD_STATE_NONE;
			err = io_result;
		} else {
			os_get_cmd_sem();
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_get_freq_range_properties(void *context,
				 u8 range, u32 *minfreq, u32 *maxfreq)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		switch (range) {
		case FMD_FREQRANGE_EUROAMERICA:
			*minfreq = 87500;
			*maxfreq = 108000;
			break;
		case FMD_FREQRANGE_JAPAN:
			*minfreq = 76000;
			*maxfreq = 90000;
			break;
		case FMD_FREQRANGE_CHINA:
			*minfreq = 70000;
			*maxfreq = 108000;
			break;
		default:
			break;
		}
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_set_antenna(void *context, u8 antenna)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (antenna > FMD_ANTENNA_WIRED) {
		err = FMD_RESULT_PARAMETER_INVALID;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[32];

		parameters[0] = antenna;

		state->gocmd |= FMD_STATE_ANTENNA;
		io_result = fmd_send_cmd_and_read_resp(CMD_FMR_SETANTENNA, 1,
						       parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			state->gocmd = FMD_STATE_NONE;
			err = io_result;
		} else {
			state->rx_antenna = antenna;
			os_get_cmd_sem();
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_get_antenna(void *context, u8 * antenna)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*antenna = state->rx_antenna;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_set_freq_range(void *context, u8 range)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (range > FMD_FREQRANGE_CHINA) {
		err = FMD_RESULT_PARAMETER_INVALID;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[8];
		u16 response_count;
		u16 response_data[8];

		parameters[0] = range;
		parameters[1] = 0;
		parameters[2] = 760;

		io_result = fmd_send_cmd_and_read_resp(CMD_FMR_TN_SETBAND, 3,
						       parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {
			state->rx_freqrange = range;
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_get_freq_range(void *context, u8 * range)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (fmd_state_info.fmd_initalized) {
		*range = state->rx_freqrange;

		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;

	return err;
}

u8 fmd_rx_set_grid(void *context, u8 grid)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (grid > FMD_GRID_200KHZ) {
		err = FMD_RESULT_PARAMETER_INVALID;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[32];

		parameters[0] = grid;

		io_result = fmd_send_cmd_and_read_resp(CMD_FMR_TN_SETGRID, 1,
						       parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS)
			err = io_result;
		else
			err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_set_frequency(void *context, u32 freq)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[8];

		parameters[0] = fmd_rx_frequency_to_channel(context, freq);

		state->gocmd |= FMD_STATE_FREQUENCY;
		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMR_SP_TUNE_SETCHANNEL, 1,
					       parameters, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			state->gocmd = FMD_STATE_NONE;
			err = io_result;
		} else {
			os_get_cmd_sem();
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_get_frequency(void *context, u32 * freq)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 response_count;
		u16 response_data[32];

		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMR_SP_TUNE_GETCHANNEL, 0,
					       NULL, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {
			*freq = fmd_rx_channel_to_frequency(context,
							    response_data[0]);
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_set_stereo_mode(void *context, u8 mode)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (mode > FMD_STEREOMODE_BLENDING) {
		err = FMD_RESULT_PARAMETER_INVALID;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[8];
		u16 response_count;
		u16 response_data[32];

		parameters[0] = mode;

		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMR_RP_STEREO_SETMODE, 1,
					       parameters, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {
			state->rx_stereomode = mode;
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_get_stereo_mode(void *context, u8 * mode)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*mode = state->rx_stereomode;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_get_signal_strength(void *context, u16 * strength)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 response_count;
		u16 response_data[8];

		io_result = fmd_send_cmd_and_read_resp(CMD_FMR_RP_GETRSSI,
						       0, NULL, &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {
			*strength = response_data[0];
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_set_stop_level(void *context, u16 stoplevel)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		state->rx_seek_stoplevel = stoplevel;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_get_stop_level(void *context, u16 * stoplevel)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*stoplevel = state->rx_seek_stoplevel;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_seek(void *context, bool upwards)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[8];
		u16 response_count;
		u16 response_data[32];

		if (upwards)
			parameters[0] = 0x0000;
		else
			parameters[0] = 0x0001;
		parameters[1] = state->rx_seek_stoplevel;
		parameters[2] = DEFAULT_PEAK_NOISE_VALUE;
		parameters[3] = AVERAGE_NOISE_MAX_VALUE;
		/*0x7FFF disables "final snr" criterion for search */

		state->gocmd |= FMD_STATE_SEEK;
		io_result = fmd_send_cmd_and_read_resp(CMD_FMR_SP_SEARCH_START,
						       4, parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			state->gocmd = FMD_STATE_NONE;
			err = io_result;
		} else {
			err = FMD_RESULT_ONGOING;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_scan_band(void *context, u8 max_channels_to_scan)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (max_channels_to_scan > MAX_CHANNELS_TO_SCAN) {
		err = FMD_RESULT_PARAMETER_INVALID;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[8];
		u16 response_count;
		u16 response_data[32];

		parameters[0] = max_channels_to_scan;
		parameters[1] = state->rx_seek_stoplevel;
		parameters[2] = DEFAULT_PEAK_NOISE_VALUE;
		parameters[3] = AVERAGE_NOISE_MAX_VALUE;

		state->gocmd |= FMD_STATE_SCAN_BAND;
		state->max_channels_to_scan = max_channels_to_scan;
		io_result = fmd_send_cmd_and_read_resp(CMD_FMR_SP_SCAN_START,
						       4, parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			state->gocmd = FMD_STATE_NONE;
			err = io_result;
		} else {
			err = FMD_RESULT_ONGOING;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_get_max_channels_to_scan(void *context, u8 *max_channels_to_scan)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*max_channels_to_scan = fmd_state_info.max_channels_to_scan;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_get_scan_band_info(void *context,
			     u32 index,
			     u16 *numchannels, u16 *channels, u16 *rssi)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[32];

		parameters[0] = index;

		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMR_SP_SCAN_GETRESULT, 1,
					       parameters, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {
			*numchannels = response_data[0];
			channels[0] = response_data[1];
			rssi[0] = response_data[2];
			channels[1] = response_data[3];
			rssi[1] = response_data[4];
			channels[2] = response_data[5];
			rssi[2] = response_data[6];
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_block_scan(void *context, u32 start_freq, u32 stop_freq, u8 antenna)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	u16 start_channel;
	u16 stop_channel;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[8];
		u16 response_count;
		u16 response_data[32];
		start_channel = fmd_rx_frequency_to_channel(context,
							    start_freq);
		stop_channel = fmd_rx_frequency_to_channel(context, stop_freq);
		if (antenna > 1) {
			err = FMD_RESULT_PARAMETER_INVALID;
		} else {
			parameters[0] = start_channel;
			parameters[1] = stop_channel;
			parameters[2] = antenna;

			state->gocmd |= FMD_STATE_BLOCK_SCAN;
			io_result =
			    fmd_send_cmd_and_read_resp
			    (CMD_FMR_SP_BLOCKSCAN_START, 3, parameters,
			     &response_count, response_data);
			if (io_result != FMD_RESULT_SUCCESS) {
				state->gocmd = FMD_STATE_NONE;
				err = io_result;
			} else {
				err = FMD_RESULT_ONGOING;
			}
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_get_block_scan_result(void *context,
				u32 index, u16 *numchannels, u16 *rssi)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[32];

		parameters[0] = index;

		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMR_SP_BLOCKSCAN_GETRESULT,
					       1, parameters, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {
			*numchannels = response_data[0];
			rssi[0] = response_data[1];
			rssi[1] = response_data[2];
			rssi[2] = response_data[3];
			rssi[3] = response_data[4];
			rssi[4] = response_data[5];
			rssi[5] = response_data[6];
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_stop_seeking(void *context)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_state_info.fmd_initalized) {
		if ((state->gocmd & FMD_STATE_SEEK)
		    || (state->gocmd & FMD_STATE_SCAN_BAND)) {
			u8 io_result;
			u16 response_count;
			u16 response_data[32];

			state->gocmd &= ~FMD_STATE_SCAN_BAND;
			state->gocmd &= ~FMD_STATE_SEEK;
			state->gocmd |= FMD_STATE_SEEK_STOP;
			io_result =
			    fmd_send_cmd_and_read_resp(CMD_FMR_SP_STOP, 0, NULL,
						       &response_count,
						       response_data);
			if (io_result != FMD_RESULT_SUCCESS) {
				state->gocmd = FMD_STATE_NONE;
				err = io_result;
			} else {
				os_get_cmd_sem();
				err = FMD_RESULT_SUCCESS;
			}
		} else {
			err = FMD_RESULT_PRECONDITIONS_VIOLATED;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_af_update_start(void *context, u32 freq)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[8];

		parameters[0] = fmd_rx_frequency_to_channel(state, freq);

		state->gocmd |= FMD_STATE_AF_UPDATE;
		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMR_SP_AFUPDATE_START, 1,
					       parameters, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
			state->gocmd = FMD_STATE_NONE;
		} else {
			os_get_cmd_sem();
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_get_af_update_result(void *context, u16 * af_level)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 response_count;
		u16 response_data[32];

		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMR_SP_AFUPDATE_GETRESULT, 0,
					       NULL, &response_count,
					       response_data);

		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {

			*af_level = response_data[0];
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_af_switch_start(void *context, u32 freq, u16 picode)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;

	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[8];
		u16 response_count;
		u16 response_data[8];

		parameters[0] = fmd_rx_frequency_to_channel(state, freq);
		parameters[1] = picode;
		parameters[2] = 0xFFFF;
		parameters[3] = state->rx_seek_stoplevel;
		parameters[4] = 0x0000;

		state->gocmd |= FMD_STATE_AF_SWITCH;
		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMR_SP_AFSWITCH_START, 5,
					       parameters, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			state->gocmd = FMD_STATE_NONE;
			err = io_result;
		} else {
			os_get_cmd_sem();
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_get_af_switch_results(void *context,
				u16 *afs_conclusion,
				u16 *afs_level, u16 *afs_pi)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 response_count;
		u16 response_data[32];

		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMR_SP_AFSWITCH_GETRESULT, 0,
					       NULL, &response_count,
					       response_data);

		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {

			*afs_conclusion = response_data[0];
			*afs_level = response_data[1];
			*afs_pi = response_data[2];

			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_get_rds(void *context, bool * on)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*on = fmd_state_info.rx_rdson;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_buffer_set_size(void *context, u8 size)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[8];

		if (size > MAX_RDS_GROUPS) {
			err = FMD_RESULT_IO_ERROR;
		} else {
			parameters[0] = size;

			io_result =
			    fmd_send_cmd_and_read_resp
			    (CMD_FMR_DP_BUFFER_SETSIZE, 1, parameters,
			     &response_count, response_data);
			if (io_result != FMD_RESULT_SUCCESS)
				err = io_result;
			else
				err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_buffer_set_threshold(void *context, u8 threshold)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[8];
		if (threshold > MAX_RDS_GROUPS) {
			err = FMD_RESULT_IO_ERROR;
		} else {
			parameters[0] = threshold;

			io_result =
			    fmd_send_cmd_and_read_resp
			    (CMD_FMR_DP_BUFFER_SETTHRESHOLD, 1, parameters,
			     &response_count, response_data);
			if (io_result != FMD_RESULT_SUCCESS)
				err = io_result;
			else
				err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_set_rds(void *context, u8 on_off_state)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[8];

		switch (on_off_state) {
		case FMD_SWITCH_ON_RDS_SIMULATOR:
			parameters[0] = 0xFFFF;
			break;
		case FMD_SWITCH_OFF_RDS:
		default:
			parameters[0] = 0x0000;
			fmd_state_info.rx_rdson = STE_FALSE;
			break;
		case FMD_SWITCH_ON_RDS:
			parameters[0] = 0x0001;
			fmd_state_info.rx_rdson = STE_TRUE;
			break;
		case FMD_SWITCH_ON_RDS_ENHANCED_MODE:
			parameters[0] = 0x0002;
			fmd_state_info.rx_rdson = STE_TRUE;
			break;
		}

		io_result = fmd_send_cmd_and_read_resp(CMD_FMR_DP_SETCONTROL, 1,
						       parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS)
			err = io_result;
		else
			err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_rx_get_low_level_rds_groups(void *context,
				   u8 index,
				   u8 *rds_buf_count,
				   u16 *block1,
				   u16 *block2,
				   u16 *block3,
				   u16 *block4,
				   u8 *status1,
				   u8 *status2, u8 *status3, u8 *status4)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*rds_buf_count = state->rdsbufcnt;
		*block1 = state->rdsgroup[index].block[0];
		*block2 = state->rdsgroup[index].block[1];
		*block3 = state->rdsgroup[index].block[2];
		*block4 = state->rdsgroup[index].block[3];
		*status1 = state->rdsgroup[index].status[0];
		*status2 = state->rdsgroup[index].status[1];
		*status3 = state->rdsgroup[index].status[2];
		*status4 = state->rdsgroup[index].status[3];
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;

	return err;
}

u8 fmd_tx_set_pa(void *context, bool on)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[8];

		if (on)
			parameters[0] = 0x0001;
		else
			parameters[0] = 0x0000;

		state->gocmd |= FMD_STATE_PA;
		io_result = fmd_send_cmd_and_read_resp(CMD_FMT_PA_SETMODE, 1,
						       parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			state->gocmd = FMD_STATE_NONE;
			err = io_result;
		} else {
			os_get_cmd_sem();
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_set_signal_strength(void *context, u16 strength)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		if ((strength >= MIN_POWER_LEVEL)
		    && (strength <= MAX_POWER_LEVEL)) {
			u8 io_result;
			u16 parameters[1];
			u16 response_count;
			u16 response_data[8];

			parameters[0] = strength;

			state->gocmd |= FMD_STATE_PA_LEVEL;
			io_result =
			    fmd_send_cmd_and_read_resp(CMD_FMT_PA_SETCONTROL, 1,
						       parameters,
						       &response_count,
						       response_data);
			if (io_result != FMD_RESULT_SUCCESS) {
				state->gocmd = FMD_STATE_NONE;
				err = io_result;
			} else {
				state->tx_strength = strength;
				os_get_cmd_sem();
				err = FMD_RESULT_SUCCESS;
			}
		} else {
			err = FMD_RESULT_PARAMETER_INVALID;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_get_signal_strength(void *context, u16 * strength)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*strength = state->tx_strength;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_set_freq_range(void *context, u8 range)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (range > FMD_FREQRANGE_CHINA) {
		err = FMD_RESULT_PARAMETER_INVALID;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[8];
		u16 response_count;
		u16 response_data[32];

		parameters[0] = range;
		parameters[1] = 0;
		parameters[2] = 760;

		io_result = fmd_send_cmd_and_read_resp(CMD_FMT_TN_SETBAND, 3,
						       parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {
			state->tx_freqrange = range;
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_get_freq_range(void *context, u8 * range)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*range = state->tx_freqrange;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_set_grid(void *context, u8 grid)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (grid > FMD_GRID_200KHZ) {
		err = FMD_RESULT_PARAMETER_INVALID;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[32];

		parameters[0] = grid;

		io_result = fmd_send_cmd_and_read_resp(CMD_FMT_TN_SETGRID, 1,
						       parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS)
			err = io_result;
		else
			err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_set_preemphasis(void *context, u8 preemphasis)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[32];

		switch (preemphasis) {
		case FMD_EMPHASIS_50US:
			parameters[0] = 1;
			break;
		case FMD_EMPHASIS_75US:
		default:
			parameters[0] = 2;
			break;
		}

		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMT_RP_SETPREEMPHASIS, 1,
					       parameters, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {
			state->tx_preemphasis = preemphasis;
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_get_preemphasis(void *context, u8 * preemphasis)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*preemphasis = state->tx_preemphasis;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_set_frequency(void *context, u32 freq)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[8];
		u16 response_count;
		u16 response_data[8];

		parameters[0] = fmd_tx_frequency_to_channel(context, freq);

		state->gocmd |= FMD_STATE_FREQUENCY;
		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMT_SP_TUNE_SETCHANNEL, 1,
					       parameters, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			state->gocmd = FMD_STATE_NONE;
			err = io_result;
		} else {
			os_get_cmd_sem();
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_get_frequency(void *context, u32 * freq)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 response_count;
		u16 response_data[8];

		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMT_SP_TUNE_GETCHANNEL, 0,
					       NULL, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {
			*freq = fmd_tx_channel_to_frequency(context,
							    response_data[0]);
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_enable_stereo_mode(void *context, bool enable_stereo_mode)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[8];
		u16 response_count;
		u16 response_data[8];

		parameters[0] = enable_stereo_mode;

		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMT_RP_STEREO_SETMODE, 1,
					       parameters, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {
			state->tx_stereomode = enable_stereo_mode;
			err = FMD_RESULT_SUCCESS;
		}

	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_get_stereo_mode(void *context, bool * stereo_mode)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*stereo_mode = state->tx_stereomode;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_set_pilot_deviation(void *context, u16 deviation)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		if (deviation <= MAX_PILOT_DEVIATION) {
			u8 io_result;
			u16 parameters[1];
			u16 response_count;
			u16 response_data[8];
			parameters[0] = deviation;

			io_result =
			    fmd_send_cmd_and_read_resp
			    (CMD_FMT_RP_SETPILOTDEVIATION, 1, parameters,
			     &response_count, response_data);
			if (io_result != FMD_RESULT_SUCCESS) {
				err = io_result;
			} else {
				state->tx_pilotdev = deviation;
				err = FMD_RESULT_SUCCESS;
			}
		} else {
			err = FMD_RESULT_PARAMETER_INVALID;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_get_pilot_deviation(void *context, u16 * deviation)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*deviation = state->tx_pilotdev;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_set_rds_deviation(void *context, u16 deviation)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		if (deviation <= MAX_RDS_DEVIATION) {
			u8 io_result;
			u16 parameters[1];
			u16 response_count;
			u16 response_data[8];

			parameters[0] = deviation;

			io_result =
			    fmd_send_cmd_and_read_resp
			    (CMD_FMT_RP_SETRDSDEVIATION, 1, parameters,
			     &response_count, response_data);
			if (io_result != FMD_RESULT_SUCCESS) {
				err = io_result;
			} else {
				state->tx_rdsdev = deviation;
				err = FMD_RESULT_SUCCESS;
			}
		} else {
			err = FMD_RESULT_PARAMETER_INVALID;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_get_rds_deviation(void *context, u16 * deviation)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*deviation = state->tx_rdsdev;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_set_rds(void *context, bool on)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[8];

		if (on)
			parameters[0] = 0x0001;
		else
			parameters[0] = 0x0000;

		io_result = fmd_send_cmd_and_read_resp(CMD_FMT_DP_SETCONTROL, 1,
						       parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {
			state->tx_rdson = on;
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_set_group(void *context,
		    u16 position,
		    u8 block1[2], u8 block2[2], u8 block3[2], u8 block4[2]
    )
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[5];
		u16 response_count;
		u16 response_data[8];

		parameters[0] = position;
		os_mem_copy(&parameters[1], block1, 2);
		os_mem_copy(&parameters[2], block2, 2);
		os_mem_copy(&parameters[3], block3, 2);
		os_mem_copy(&parameters[4], block4, 2);

		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMT_DP_BUFFER_SETGROUP, 5,
					       parameters, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS)
			err = io_result;
		else
			err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_tx_buffer_set_size(void *context, u16 buffer_size)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[8];
		parameters[0] = buffer_size;
		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMT_DP_BUFFER_SETSIZE, 1,
					       parameters, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS)
			err = io_result;
		else
			err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;

}

u8 fmd_tx_get_rds(void *context, bool * on)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*on = state->tx_rdson;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_set_balance(void *context, s8 balance)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[8];
		parameters[0] = (((signed short)balance) * 0x7FFF) / 100;
		io_result = fmd_send_cmd_and_read_resp(CMD_AUP_SETBALANCE, 1,
						       parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS)
			err = io_result;
		else
			err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_set_volume(void *context, u8 volume)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[8];

		parameters[0] = (((u16) volume) * 0x7FFF) / 100;

		io_result = fmd_send_cmd_and_read_resp(CMD_AUP_SETVOLUME, 1,
						       parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			err = io_result;
		} else {
			state->rx_volume = volume;
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_get_volume(void *context, u8 * volume)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		*volume = state->rx_volume;
		err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_set_mute(void *context, bool mute_on)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[2];
		u16 response_count;
		u16 response_data[8];

		if (!mute_on)
			parameters[0] = 0x0000;
		else
			parameters[0] = 0x0001;
		parameters[1] = 0x0001;

		state->gocmd |= FMD_STATE_MUTE;
		io_result = fmd_send_cmd_and_read_resp(CMD_AUP_SETMUTE, 2,
						       parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			state->gocmd = FMD_STATE_NONE;
			err = io_result;
		} else {
			os_get_cmd_sem();
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_ext_set_mute(void *context, bool mute_on)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[2];
		u16 response_count;
		u16 response_data[8];

		if (!mute_on)
			parameters[0] = 0x0000;
		else
			parameters[0] = 0x0001;

		io_result = fmd_send_cmd_and_read_resp(CMD_AUP_EXT_SETMUTE, 1,
						       parameters,
						       &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS)
			err = io_result;
		else
			err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_power_up(void *context)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 response_count;
		u16 response_data[8];

		state->gocmd |= FMD_STATE_GEN_POWERUP;
		io_result = fmd_send_cmd_and_read_resp(CMD_GEN_POWERUP, 0,
						       NULL, &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			state->gocmd = FMD_STATE_NONE;
			err = io_result;
		} else {
			os_get_cmd_sem();
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_goto_standby(void *context)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 response_count;
		u16 response_data[8];

		io_result = fmd_send_cmd_and_read_resp(CMD_GEN_GOTOSTANDBY, 0,
						       NULL, &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS)
			err = io_result;
		else
			err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_goto_power_down(void *context)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 response_count;
		u16 response_data[8];

		io_result = fmd_send_cmd_and_read_resp(CMD_GEN_GOTOPOWERDOWN, 0,
						       NULL, &response_count,
						       response_data);
		if (io_result != FMD_RESULT_SUCCESS)
			err = io_result;
		else
			err = FMD_RESULT_SUCCESS;
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_select_ref_clk(void *context, u16 ref_clk)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[8];

		parameters[0] = ref_clk;

		state->gocmd |= FMD_STATE_SELECT_REF_CLK;
		io_result =
		    fmd_send_cmd_and_read_resp(CMD_GEN_SELECTREFERENCECLOCK, 1,
					       parameters, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			state->gocmd = FMD_STATE_NONE;
			err = io_result;
		} else {
			os_get_cmd_sem();
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_set_ref_clk_pll(void *context, u16 freq)
{
	struct fmd_state_info_t *state = (struct fmd_state_info_t *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	if (fmd_go_cmd_busy()) {
		err = FMD_RESULT_BUSY;
	} else if (fmd_state_info.fmd_initalized) {
		u8 io_result;
		u16 parameters[1];
		u16 response_count;
		u16 response_data[8];

		parameters[0] = freq;

		state->gocmd |= FMD_STATE_SET_REF_CLK_PLL;
		io_result =
		    fmd_send_cmd_and_read_resp(CMD_GEN_SETREFERENCECLOCKPLL, 1,
					       parameters, &response_count,
					       response_data);
		if (io_result != FMD_RESULT_SUCCESS) {
			state->gocmd = FMD_STATE_NONE;
			err = io_result;
		} else {
			os_get_cmd_sem();
			err = FMD_RESULT_SUCCESS;
		}
	} else {
		err = FMD_RESULT_IO_ERROR;
	}

	return err;
}

u8 fmd_send_fm_ip_enable(void)
{
	u8 FMIP_EnableCmd[5];
	u8 err = 0;

	FMIP_EnableCmd[0] = HCI_PACKET_INDICATOR_FM_CMD_EVT;
	FMIP_EnableCmd[1] = 0x03;	/* Length of following Bytes */
	FMIP_EnableCmd[2] = CATENA_OPCODE;
	FMIP_EnableCmd[3] = FM_WRITE;
	FMIP_EnableCmd[4] = FM_FUNCTION_ENABLE;

	/* Send the Packet */
	err = ste_fm_send_packet(5, FMIP_EnableCmd);

	/* wait till response comes */
	os_get_cmd_sem();

	return err;
}

u8 fmd_send_fm_ip_disable(void)
{
	u8 FMIP_DisableCmd[5];
	u8 err = 0;

	FMIP_DisableCmd[0] = HCI_PACKET_INDICATOR_FM_CMD_EVT;
	FMIP_DisableCmd[1] = 0x03;	/* Length of following Bytes */
	FMIP_DisableCmd[2] = CATENA_OPCODE;
	FMIP_DisableCmd[3] = FM_WRITE;
	FMIP_DisableCmd[4] = FM_FUNCTION_DISABLE;

	/* Send the Packet */
	err = ste_fm_send_packet(5, FMIP_DisableCmd);

	/* wait till response comes */
	os_get_cmd_sem();

	return err;
}

u8 fmd_send_fm_firmware(u8 *fw_buffer, u16 fw_size)
{
	u8 err = 0;
	u16 bytes_to_write = ST_WRITE_FILE_BLK_SIZE - 4;
	u16 bytes_remaining = fw_size;
	u8 fm_firmware_data[ST_WRITE_FILE_BLK_SIZE + 6];
	u32 block_id = 0;

	while (bytes_remaining > 0) {
		if (bytes_remaining < ST_WRITE_FILE_BLK_SIZE - 4)
			bytes_to_write = bytes_remaining;

		/* Six bytes of HCI Header for FM Firmware
		 * so shift the firmware data by 6 bytes
		 */
		os_mem_copy(fm_firmware_data + 6, fw_buffer, bytes_to_write);
		err = fmd_write_file_block(block_id, fm_firmware_data,
					   bytes_to_write);
		if (!err) {
			block_id++;
			fw_buffer += bytes_to_write;
			bytes_remaining -= bytes_to_write;

			if (block_id == 256)
				block_id = 0;
		} else {
			FM_DEBUG_REPORT("fmd_send_fm_firmware: "
					"Failed to download %d"
					"Block, error = %d",
					(unsigned int)block_id, err);
			return err;
		}
	}

	return err;
}

void fmd_receive_data(u16 packet_length, u8 *packet_buffer)
{
	if (packet_buffer != NULL) {
		if (packet_length == 0x04 &&
		    packet_buffer[0] == CATENA_OPCODE &&
		    packet_buffer[1] == FM_EVENT) {
			/* PG 1.0 interrupt Handling */
			u16 interrupt;
			interrupt = (u16) (packet_buffer[3] << 8) |
			    (u16) packet_buffer[2];
			FM_DEBUG_REPORT("interrupt = %04x",
					(unsigned int)interrupt);
			fmd_add_interrupt_to_queue(interrupt);
		} else if (packet_length == 0x06 &&
			   packet_buffer[0] == 0x00 &&
			   packet_buffer[1] == CATENA_OPCODE &&
			   packet_buffer[2] == 0x01 &&
			   packet_buffer[3] == FM_EVENT) {
			/* PG 2.0 interrupt Handling */
			u16 interrupt;
			interrupt = (u16) (packet_buffer[5] << 8) |
			    (u16) packet_buffer[4];
			FM_DEBUG_REPORT("interrupt = %04x",
					(unsigned int)interrupt);
			fmd_add_interrupt_to_queue(interrupt);
		} else if (packet_buffer[0] == 0x00 &&
			   packet_buffer[1] == CATENA_OPCODE &&
			   packet_buffer[2] == FM_WRITE) {
			/* Command Complete or RDS Data Handling */
			switch (packet_buffer[3]) {
			case FM_CMD_STATUS_CMD_SUCCESS:
				{
					fmd_process_fm_function(&packet_buffer
								[4]);
				}
				break;
			case FM_CMD_STATUS_HCI_ERR_HW_FAILURE:
				{
					FM_DEBUG_REPORT
					    ("FM_CMD_STATUS_HCI_ERR_HW_FAILURE"
					    );
				}
				break;
			case FM_CMD_STATUS_HCI_ERR_INVALID_PARAMETERS:
				{
					FM_DEBUG_REPORT
					    ("FM_CMD_STATUS_HCI_ERR_INVALID_"
					     "PARAMETERS");
				}
				break;
			case FM_CMD_STATUS_IP_UNINIT:
				{
					FM_DEBUG_REPORT
					    ("FM_CMD_STATUS_IP_UNINIT");
				}
				break;
			case FM_CMD_STATUS_HCI_ERR_UNSPECIFIED_ERROR:
				{
					FM_DEBUG_REPORT
					    ("FM_CMD_STATUS_HCI_ERR_UNSPECIFIED"
					     "_ERROR");
				}
				break;
			case FM_CMD_STATUS_HCI_ERR_CMD_DISALLOWED:
				{
					FM_DEBUG_REPORT
					    ("FM_CMD_STATUS_HCI_ERR_CMD_"
					     "DISALLOWED");
				}
				break;
			case FM_CMD_STATUS_WRONG_SEQ_NUM:
				{
					FM_DEBUG_REPORT
					    ("FM_CMD_STATUS_WRONG_SEQ_NUM");
				}
				break;
			case FM_CMD_STATUS_UNKOWNFILE_TYPE:
				{
					FM_DEBUG_REPORT
					    ("FM_CMD_STATUS_UNKOWNFILE_TYPE");
				}
				break;
			case FM_CMD_STATUS_FILE_VERSION_MISMATCH:
				{
					FM_DEBUG_REPORT
					    ("FM_CMD_STATUS_FILE_VERSION_"
					     "MISMATCH");
				}
				break;
			}
		}
	} else {
		FM_DEBUG_REPORT("fmd_receive_data: Buffer = NULL");
	}
}

void fmd_int_bufferfull(void)
{
	u16 response_count;
	u16 response_data[16];
	u16 cnt;
	u16 index = 0;
	u8 io_result;
	struct fmd_rdsgroup_t rdsgroup;

	if (fmd_state_info.rx_rdson) {
		/* get group count */
		io_result =
		    fmd_send_cmd_and_read_resp(CMD_FMR_DP_BUFFER_GETGROUPCOUNT,
					       0, NULL, &response_count,
					       response_data);
		if (io_result == FMD_RESULT_SUCCESS) {
			/* read RDS groups */
			cnt = response_data[0];
			if (cnt > MAX_RDS_GROUPS)
				cnt = MAX_RDS_GROUPS;
			fmd_state_info.rdsbufcnt = cnt;
			while (cnt-- && fmd_state_info.rx_rdson) {
				io_result =
				    fmd_send_cmd_and_read_resp
				    (CMD_FMR_DP_BUFFER_GETGROUP, 0, NULL,
				     &response_count, (u16 *) &rdsgroup);
				if (io_result == FMD_RESULT_SUCCESS)
					if (fmd_state_info.rx_rdson)
						fmd_state_info.rdsgroup[index++]
						    = rdsgroup;
			}
		}
	}
}

void fmd_hexdump(char prompt, u8 *arr, int num_bytes)
{
	int i;
	u8 tmpVal;
	static u8 pkt_write[512], *pkt_ptr;
	sprintf(pkt_write, "\n[%04x] %c", num_bytes, prompt);
	pkt_ptr = pkt_write + strlen(pkt_write);
	if (arr != NULL) {
		/* Copy the buffer only if the input buffer is not NULL */
		for (i = 0; i < num_bytes; i++) {
			*pkt_ptr++ = ' ';
			tmpVal = arr[i] >> 4;
			*pkt_ptr++ = ASCVAL(tmpVal);
			tmpVal = arr[i] & 0x0F;
			*pkt_ptr++ = ASCVAL(tmpVal);
			if (i > 20) {
				/* Print only 20 bytes at max */
				break;
			}
		}
	}
	*pkt_ptr++ = '\0';
	FM_DEBUG_REPORT("%s", pkt_write);
}

MODULE_AUTHOR("Hemant Gupta");
MODULE_LICENSE("GPL v2");
