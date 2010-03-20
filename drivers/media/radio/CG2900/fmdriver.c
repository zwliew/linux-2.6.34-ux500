/*
 * file fmdriver.c
 *
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

/** the count of interrupt sources of the system */
#define MAX_COUNT_OF_IRQS 16

#define ASCVAL(x)(((x) <= 9) ? (x) + '0' : (x) - 10 + 'a')

/** Variable for state of library */
static bool g_fmd_initalized = STE_FALSE;

/** Variables for synchronization of Interrupts to other function calls */
static bool g_fmd_interrupt_mutex = STE_FALSE;
static int  g_fmd_interrupt_semaphore;
/** STE_TRUE if an interrupt needs to be handled */
static bool g_interrupt_received[MAX_COUNT_OF_IRQS] = {STE_FALSE};
/** forward declaration of the interrupt handler function */
static char g_tempstring[50];

/**
 * enum fmd_gocmd_e - FM Driver Command state .
 * @fmd_gocmd_none: FM Driver in Idle state
 * @fmd_gocmd_setmode: FM Driver in setmode state
 * @fmd_gocmd_setfrequency: FM Driver in Set frequency state.
 * @fmd_gocmd_setantenna: FM Driver in Setantenna state
 * @fmd_gocmd_setmute: FM Driver in Setmute state
 * @fmd_gocmd_seek: FM Driver in seek mode
 * @fmd_gocmd_seekstop: FM Driver in seek stop level state.
 * @fmd_gocmd_scanband: FM Driver in Scanband mode
 * @fmd_gocmd_stepfrequency: FM Driver in StepFreq state
 * @fmd_gocmd_genpowerup: FM Driver in Power UP state
 */
enum fmd_gocmd_e {
		fmd_gocmd_none,
		fmd_gocmd_setmode,
		fmd_gocmd_setfrequency,
		fmd_gocmd_setantenna,
		fmd_gocmd_setmute,
		fmd_gocmd_seek,
		fmd_gocmd_seekstop,
		fmd_gocmd_scanband,
		fmd_gocmd_stepfrequency,
		fmd_gocmd_genpowerup,
};

/**
 * struct fmd_rdsgroup_s - Main rds group structure.
 * @block: Array for RDS Block(s) received.
 * @status: Array of Status of corresponding RDS block(s).
 */
struct fmd_rdsgroup_s {
		unsigned short block[4];
		unsigned char  status[4];
} ;

/**
 * struct struct fmd_state_info_s - Main FM state info structure.
 * @mode: fm mode(idle/RX/TX).
 * @logginglevel: 0 = off, 1 = basic, 2 = all.
 * @rx_grid:  Rceiver Grid
 * @rx_freqrange: Receiver freq range
 * @rx_stereomode: Rceiver Stereo mode
 * @rx_volume:  Receiver volume level
 * @rx_balance: Rceiver Balance level
 * @rx_mute: Receiver Mute
 * @rx_antenna: Receiver Antenna
 * @rx_deemphasis: Receiver Deemphasis
 * @rx_rdson:  Receiver RDS ON
 * @rx_seek_stoplevel:  RDS seek stop Level
 * @gocmd:  pipe line Command
 * @rdsgroup:  Array of RDS group Buffer
 * @rdsbufcnt:  Number of RDS Groups available
 * @callback:  Callback registered by upper layers.
 */
struct fmd_state_info_s {
		uint8_t			mode;
		uint8_t    		logginglevel;
		uint8_t			rx_grid;
		uint8_t			rx_freqrange;
		uint32_t    		rx_stereomode;
		uint8_t			rx_volume;
		int8_t			rx_balance;
		bool			rx_mute;
		uint8_t    		rx_antenna;
		uint8_t			rx_deemphasis;
		bool   			rx_rdson;
		uint16_t		rx_seek_stoplevel;

		enum fmd_gocmd_e 	gocmd;
		struct fmd_rdsgroup_s  	rdsgroup[22];
		uint8_t    		rdsbufcnt;
		fmd_radio_cb 	callback;
} fmd_state_info;

/**
 * struct fmd_data_s - Main structure for FM data exchange.
 * @cmd_id: Command Id of the command being exchanged.
 * @num_parameters: Number of parameters
 * @parameters: Pointer to FM data parameters.
 */
struct fmd_data_s {
		uint32_t cmd_id;
		uint16_t num_parameters;
		uint8_t *parameters;
} fmd_data;

/* ------------------ Internal function declarations ---------------------- */
static void fmd_criticalsection_enter(void);
static void fmd_criticalsection_leave(void);
static void fmd_event_name(
		unsigned long event,
		char *str
		);
static void fmd_interrupt_name(
		unsigned long interrupt,
		char *str
		);
static void fmd_interrupt_handler(
		const unsigned long interrupt
		);
static void fmd_handle_interrupt(void);
static void fmd_handle_synchronized_interrupt(
		unsigned long interrupt
		);
static void fmd_callback(
		void *context,
		uint32_t event,
		uint32_t event_int_data,
		bool event_boolean_data
		);
static unsigned short fmd_rx_frequency_to_channel(
		void *context,
		uint32_t freq
		);
static uint32_t fmd_rx_channel_to_frequency(
		void *context,
		unsigned short chan
		);
static bool fmd_go_cmd_busy(void);
static unsigned int fmd_send_cmd_and_read_resp(
		const unsigned long cmd_id,
		const unsigned int num_parameters,
		const unsigned short *parameters,
		unsigned int *resp_num_parameters,
		unsigned short *resp_parameters
		);
static unsigned int fmd_send_cmd(
		const unsigned long cmd_id ,
		const unsigned int num_parameters,
		const unsigned short *parameters
		);
static unsigned int fmd_read_resp(
		unsigned long *cmd_id,
		unsigned int *num_parameters,
		unsigned short *parameters
		);
static void fmd_process_fm_function(
		uint8_t *packet_buffer
		);
static unsigned int fmd_st_write_file_block(
		uint32_t file_block_id,
		uint8_t *file_block,
		uint16_t file_block_length
		);

/* ------------------ Internal function definitions ---------------------- */

/**
  * fmd_criticalsection_enter() - Critical Section for FM Driver.
  * This is used for interrupt syncronisation.
  */
static void fmd_criticalsection_enter(void)
{
	g_fmd_interrupt_semaphore++;
	g_fmd_interrupt_mutex = STE_TRUE;
}

/**
  * fmd_criticalsection_leave() - Critical Section for FM Driver.
  * This is used for interrupt syncronisation.
  */
static void fmd_criticalsection_leave(void)
{
	g_fmd_interrupt_semaphore--;
	if (g_fmd_interrupt_semaphore == 0)
		g_fmd_interrupt_mutex = STE_FALSE;
	fmd_handle_interrupt(); /* execute buffered interrupts */
}

/**
  * fmd_event_name() - Converts the event to a Displayable String
  * @event: event that has occurred.
  * @str: (out) Pointer to the buffer to store event Name.
  */
static void fmd_event_name(
		unsigned long event,
		char *str
		)
{
	switch (event) 	{
	case FMD_EVENT_ANTENNA_STATUS_CHANGED:
		strcpy(str, "FMD_EVENT_ANTENNA_STATUS_CHANGED");
		break;
	case FMD_EVENT_FREQUENCY_CHANGED:
		strcpy(str, "FMD_EVENT_FREQUENCY_CHANGED");
		break;
	case FMD_EVENT_FREQUENCY_RANGE_CHANGED:
		strcpy(str, "FMD_EVENT_FREQUENCY_RANGE_CHANGED");
		break;
	case FMD_EVENT_PRESET_CHANGED:
		strcpy(str, "FMD_EVENT_PRESET_CHANGED");
		break;
	case FMD_EVENT_SEEK_COMPLETED:
		strcpy(str, "FMD_EVENT_SEEK_COMPLETED");
		break;
	case FMD_EVENT_ACTION_FINISHED:
		strcpy(str, "FMD_EVENT_ACTION_FINISHED");
		break;
	case FMD_EVENT_RDSGROUP_RCVD:
		strcpy(str, "FMD_EVENT_RDSGROUP_RCVD");
		break;
	case FMD_EVENT_SEEK_STOPPED:
		strcpy(str, "FMD_EVENT_SEEK_STOPPED");
		break;
	default:
		strcpy(str, "Unknown Event");
		break;
	}
}

/**
  * fmd_interrupt_name() - Converts the interrupt to a Displayable String
  * @interrupt: interrupt received from FM Chip
  * @str:  (out) Pointer to the buffer to store interrupt Name.
  */
static void fmd_interrupt_name(
		unsigned long interrupt,
		char *str
		)
{
	switch (interrupt) {
	case 0:
		strcpy(str, "irpt_OperationSucceeded");
		break;
	case 1:
		strcpy(str, "irpt_OperationFailed");
		break;
	case 2:
	case 10:
	case 11:
	case 12:
	case 13:
		strcpy(str, "-");
		break;
	case 3:
		strcpy(str, "irpt_BufferFull/Empty");
		break;
	case 4:
		strcpy(str, "irpt_SignalQualityLow/MuteStatusChanged");
		break;
	case 5:
		strcpy(str, "irpt_MonoStereoTransition");
		break;
	case 6:
		strcpy(str, "irpt_RdsSyncFound/InputOverdrive");
		break;
	case 7:
		strcpy(str, "irpt_RdsSyncLost");
		break;
	case 8:
		strcpy(str, "irpt_PiCodeChanged");
		break;
	case 9:
		strcpy(str, "irpt_RequestedBlockAvailable");
		break;
	case 14:
		strcpy(str, "irpt_WarmBootReady");
		break;
	case 15:
		strcpy(str, "irpt_ColdBootReady");
		break;
	default:
		strcpy(str, "IRPT Unknown");
		break;
	}
}

/**
  * fmd_interrupt_handler() - The function is implemented to get the
  * interrupt indication from device.
  * @interrupt: interrupt received from FM Chip
  */
static void fmd_interrupt_handler(
		const unsigned long interrupt
		)
{
	int i;
	for (i = 0; i < MAX_COUNT_OF_IRQS; i++) {
		if (interrupt & (1 << i)) {
			g_interrupt_received[i] = STE_TRUE;
			fmd_handle_interrupt();
		}
	}
}

/**
  * fmd_handle_interrupt() - This function synchronizes Interupts in that
  * way that it is only executed when no criticalsection is active.
  * This is also the function that executes the interrupt handlers
  */
static void fmd_handle_interrupt(void)
{
	int i;
	if (!g_fmd_interrupt_mutex) {
		/* only execute when there is no other function executed */

		/* check if an interrupt was requested */
		for (i = 0; i < MAX_COUNT_OF_IRQS; i++) {
			if (g_interrupt_received[i]) {
				/*handle interrupt(now the irq is
				 * synchronized!)
				 */
				fmd_handle_synchronized_interrupt(i);
			}
		}
	}
}

/**
  * fmd_handle_synchronized_interrupt() - This function processes the
  * interrupt received from FM Chip and calls the corresponding callback
  * register by upper layers with proper parameters.
  * @interrupt: interrupt received from FM Chip
  */
static void fmd_handle_synchronized_interrupt(
		unsigned long interrupt
		)
{
	char			irpt_name[50];

	fmd_criticalsection_enter();

	fmd_interrupt_name(interrupt, irpt_name);
	FM_DEBUG_REPORT("%s", irpt_name);

	switch (interrupt) {
	case IRPT_OPERATIONSUCCEEDED:
	{
		switch (fmd_state_info.gocmd) {
		case fmd_gocmd_setantenna:
		/* Signal host */
		fmd_state_info.gocmd = fmd_gocmd_none;
		fmd_callback((void *)&fmd_state_info,
			FMD_EVENT_ACTION_FINISHED , 0,
			STE_TRUE);
		fmd_callback((void *)&fmd_state_info,
			FMD_EVENT_ANTENNA_STATUS_CHANGED ,
			0, STE_FALSE);
			break;
		case fmd_gocmd_setmode:
			/* Signal host */
			fmd_state_info.gocmd = fmd_gocmd_none;
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_ACTION_FINISHED , 0,
				STE_TRUE);
			break;
		case fmd_gocmd_setmute:
			/* Signal host */
			fmd_state_info.gocmd = fmd_gocmd_none;
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_ACTION_FINISHED , 0,
				STE_TRUE);
			break;
		case fmd_gocmd_setfrequency:
			/* Signal host */
			fmd_state_info.gocmd = fmd_gocmd_none;
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_ACTION_FINISHED , 0,
				STE_TRUE);
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_FREQUENCY_CHANGED , 0,
				STE_FALSE);
			break;

		case fmd_gocmd_seek:
			/* Signal host */
			fmd_state_info.gocmd = fmd_gocmd_none;
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_ACTION_FINISHED , 0,
				STE_TRUE);
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_SEEK_COMPLETED , 0,
				STE_FALSE);
			break;

		case fmd_gocmd_scanband:
			/* Signal host */
			fmd_state_info.gocmd = fmd_gocmd_none;
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_ACTION_FINISHED , 0,
				STE_TRUE);
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_SCAN_BAND_COMPLETED , 0,
				STE_FALSE);
			break;

		case fmd_gocmd_stepfrequency:
			/* Signal host */
			fmd_state_info.gocmd = fmd_gocmd_none;
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_ACTION_FINISHED , 0,
				STE_TRUE);
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_FREQUENCY_CHANGED , 0,
				STE_FALSE);
			break;

		case fmd_gocmd_seekstop:
			/* Signal host */
			fmd_state_info.gocmd = fmd_gocmd_none;
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_ACTION_FINISHED , 0,
				STE_TRUE);
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_SEEK_STOPPED , 0,
				STE_FALSE);
			break;

		case fmd_gocmd_genpowerup:
			/* Signal host */
			fmd_state_info.gocmd = fmd_gocmd_none;
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_ACTION_FINISHED , 0,
				STE_TRUE);
			break;

		default:
			/* Clear the variable */
			fmd_state_info.gocmd = fmd_gocmd_none;
			break;

		}
		break;
	}

	case IRPT_OPERATIONFAILED:
	{
		switch (fmd_state_info.gocmd) {

		case fmd_gocmd_seek:
			/* Signal host */
			fmd_state_info.gocmd = fmd_gocmd_none;
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_ACTION_FINISHED , 0,
				STE_FALSE);
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_SEEK_COMPLETED , 0,
				STE_FALSE);
			break;

		case fmd_gocmd_scanband:
			/* Signal host */
			fmd_state_info.gocmd = fmd_gocmd_none;
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_ACTION_FINISHED , 0,
				STE_FALSE);
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_SCAN_BAND_COMPLETED , 0,
				STE_FALSE);
			break;

		case fmd_gocmd_seekstop:
			/* Signal host */
			fmd_state_info.gocmd = fmd_gocmd_none;
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_ACTION_FINISHED , 0,
				STE_FALSE);
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_SEEK_STOPPED , 0,
				STE_FALSE);
			break;

		default:
			/* Clear */
			fmd_state_info.gocmd = fmd_gocmd_none;
			break;
		}
		break;
	}

	case IRPT_BUFFERFULL:
	{
		switch (fmd_state_info.mode)	{
		case FMD_MODE_RX:
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_RDSGROUP_RCVD , 0,
				STE_FALSE);;
			break;

		default:
		/* Do Nothing */
		break;
		}
		break;
	}

	case IRPT_COLDBOOTREADY:
	case IRPT_WARMBOOTREADY:
	{
		switch (fmd_state_info.gocmd) {
		case fmd_gocmd_genpowerup:
			/* Signal host */
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_GEN_POWERUP ,
				0, STE_TRUE);
			break;
		default:
			/* Do Nothing */
			break;
		}
		break;
	}
	}
	g_interrupt_received[interrupt] = STE_FALSE;
	fmd_criticalsection_leave();
}

/**
  * fmd_callback() - Callback function for upper layers.
  * Callback function that calls the registered callback of upper
  * layers with proper parameters.
  * @context: Pointer to the Context of the FM Driver
  * @event: event for which the callback function was caled
  * from FM Driver.
  * @event_int_data: Data corresponding the event (if any),
  * otherwise it is 0.
  * @event_boolean_data: Signifying whether the event is called from FM Driver
  * on receiving irpt_OperationSucceeded or irpt_OperationFailed.
  */
static void fmd_callback(
		void *context,
		uint32_t event,
		uint32_t event_int_data,
		bool event_boolean_data
		)
{
	fmd_event_name(event, g_tempstring);
	FM_DEBUG_REPORT("%s %x, %x %d", g_tempstring,
		(unsigned int)event , (unsigned int)event_int_data,
		(unsigned int)event_boolean_data);

	if (fmd_state_info.callback)
		fmd_state_info.callback((void *)&context,
				event,
				event_int_data,
				event_boolean_data);
}

/**
  * fmd_rx_frequency_to_channel() - Converts frequency to channel number.
  * Converts the Frequency in Khz to corresponding
  * Channel number. This is used for FM Rx
  * @context: Pointer to the Context of the FM Driver
  * @freq: Frequency in Khz
  *
  * Returns:
  *   Channel Number corresponding to the given Frequency.
  */
static unsigned short fmd_rx_frequency_to_channel(
		void *context,
		uint32_t freq
		)
{
	uint8_t		 range;
	uint32_t     minfreq, maxfreq, freq_interval;

	fmd_get_freq_range(context, &range);
	fmd_get_freq_range_properties(context, range, &minfreq,
		&maxfreq, &freq_interval);

	if (freq > maxfreq)
		freq = maxfreq;
	else if (freq < minfreq)
		freq = minfreq;
	return (unsigned short)((freq - minfreq) / 50);
}

/**
  * fmd_rx_channel_to_frequency() - Converts Channel number to frequency.
  * Converts the Channel Number to corresponding
  * Frequency in Khz. This is used for FM Rx
  * @context: Pointer to the Context of the FM Driver
  * @chan: Channel Number to be converted
  *
  * Returns:
  *   Frequency corresponding to the corresponding channel.
  */
static uint32_t fmd_rx_channel_to_frequency(
		void *context,
		unsigned short chan
		)
{
	uint8_t		 range;
	uint32_t     freq, minfreq, maxfreq, freq_interval;

	fmd_get_freq_range(context, &range);
	fmd_get_freq_range_properties(context, range, &minfreq,
		&maxfreq, &freq_interval);
	freq = minfreq + (chan*50);

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
  *   	1 otherwise
  */
static bool fmd_go_cmd_busy(void)
{
	return (fmd_state_info.gocmd != fmd_gocmd_none);
}

/**
  * fmd_send_cmd_and_read_resp() - This function sends the HCI Command
  * to Protocol Driver and Reads back the Response Packet.
  * @cmd_id: Command Id to be sent to FM Chip.
  * @num_parameters: Number of parameters of the command sent.
  * @parameters: Pointer to buffer containing the Buffer to be sent.
  * @resp_num_parameters: (out) Number of paramters of the response packet.
  * @resp_parameters: (out) Pointer to the buffer of the response packet.
  *
  * Returns:
  *   FMD_RESULT_SUCCESS: If the command is sent successfully and the response
  * 			  receioved is also correct.
  *   FMD_RESULT_RESPONSE_WRONG: If the received response is not correct.
  */
static unsigned int fmd_send_cmd_and_read_resp(
		const unsigned long cmd_id,
		const unsigned int num_parameters,
		const unsigned short *parameters,
		unsigned int *resp_num_parameters,
		unsigned short *resp_parameters
		)
{
	unsigned int	result;
	unsigned long readCmdID;

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
	FM_DEBUG_REPORT("fmd_send_cmd_and_read_resp: "\
		"returning %d", result);
	return result;
}

/**
  * fmd_send_cmd() - This function sends the HCI Command
  * to Protocol Driver.
  * @cmd_id: Command Id to be sent to FM Chip.
  * @num_parameters: Number of parameters of the command sent.
  * @parameters: Pointer to buffer containing the Buffer to be sent.
  *
  * Returns:
  *   0: If the command is sent successfully to Lower Layers.
  *   1: Otherwise
  */
static unsigned int fmd_send_cmd(
		const unsigned long cmd_id ,
		const unsigned int num_parameters,
		const unsigned short *parameters
		)
{
	uint32_t TotalLength = 2 + num_parameters * 2  + 5;
	uint8_t *FMData = (uint8_t *)os_mem_alloc(TotalLength);
	uint32_t CmdIDtemp;
	unsigned int err = 0;

	/* Wait till response of the last command is received */
	os_get_hal_sem();

	/* HCI encapsulation */
	FMData[0] = HCI_PACKET_INDICATOR_FM_CMD_EVT;
	FMData[1] = TotalLength - 2 ;
	FMData[2] = CATENA_OPCODE;
	FMData[3] = FM_WRITE ;
	FMData[4] = FM_FUNCTION_WRITECOMMAND;

	CmdIDtemp = cmd_id << 3;
	FMData[5] = (uint8_t)CmdIDtemp;
	FMData[6] = CmdIDtemp >> 8 ;
	FMData[5] |= num_parameters;

	os_mem_copy((FMData + 7), (void *)parameters, num_parameters * 2);

	/* Send the Packet */
	err = ste_fm_send_packet(TotalLength , FMData);

	os_mem_free(FMData);

	return err;
}

/**
  * fmd_read_resp() - This function reads the response packet of the previous
  * command sent to FM Chip and copies it to the buffer provided as parameter.
  * @cmd_id: (out) Pointer to Command Id received from FM Chip.
  * @num_parameters: (out) Number of paramters of the response packet.
  * @parameters: (out) Pointer to the buffer of the response packet.
  *
  * Returns:
  *   0: If the response buffer is copied successfully.
  *   1: Otherwise
  */
static unsigned int fmd_read_resp(
		unsigned long *cmd_id,
		unsigned int *num_parameters,
		unsigned short *parameters
		)
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
  * @packet_buffer: Pointer to the received Buffer.
  */
static void fmd_process_fm_function(
		uint8_t *packet_buffer
		)
{
	switch (packet_buffer[0]) {
	case FM_FUNCTION_ENABLE:
		{
			FM_DEBUG_REPORT("FM_FUNCTION_ENABLE ,"\
				"command successed received");
				os_set_cmd_sem();
		}
		break;
	case FM_FUNCTION_DISABLE:
		{
			FM_DEBUG_REPORT("FM_FUNCTION_DISABLE ,"\
				"command successed received");
				os_set_cmd_sem();
		}
		break;
	case FM_FUNCTION_RESET:
		{
			FM_DEBUG_REPORT("FM_FUNCTION_RESET ,"\
				"command successed received");
				os_set_cmd_sem();
		}
		break;
	case FM_FUNCTION_WRITECOMMAND:
		{
			/* flip the header bits to take into account
			* of little endian format */
			fmd_data.cmd_id = packet_buffer[2] << 8 |
				packet_buffer[1];
			fmd_data.num_parameters =  fmd_data.cmd_id & 0x07;
			fmd_data.cmd_id = fmd_data.cmd_id >> 3;

			if (fmd_data.num_parameters)	{
				fmd_data.parameters = &packet_buffer[3];
				os_mem_copy(fmd_data.parameters ,
					&packet_buffer[3] ,
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
			FM_DEBUG_REPORT("FM_FUNCTION_GETINTMASKALL ,"\
				"command successed received");
				os_set_cmd_sem();
		}
		break;

	case FM_FUNCTION_SETINTMASK:
		break;
	case FM_FUNCTION_GETINTMASK:
		{
			FM_DEBUG_REPORT("FM_FUNCTION_GETINTMASK ,"\
				"command successed received");
				os_set_cmd_sem();
		}
		break;

	case FM_FUNCTION_FMFWDOWNLOAD:
		{
			FM_DEBUG_REPORT("fmd_process_fm_function: "\
				"FM_FUNCTION_FMFWDOWNLOAD,"\
				"Block Id = %02x",
				packet_buffer[1]);
			os_set_cmd_sem();
		}
		break;

	default:
		{
			FM_DEBUG_REPORT("fmd_process_fm_function: "\
				"default case, FM Func = %d",
				packet_buffer[0]);
		}
		break;
	}
}

/**
  * fmd_st_write_file_block() - download firmware.
  * This Function adds the header for downloading
  * the firmware and coeffecient files and sends it to Protocol Driver.
  * @file_block_id: Block ID of the F/W to be transmitted to FM Chip
  * @file_block: Pointer to the buffer containing the bytes to be sent.
  * @file_block_length: Size of the Firmware buffer.
  */
static unsigned int fmd_st_write_file_block(
		uint32_t file_block_id,
		uint8_t *file_block,
		uint16_t file_block_length
		)
{
	unsigned int err = 0;

	file_block[0] = HCI_PACKET_INDICATOR_FM_CMD_EVT;
	file_block[1] = file_block_length + 4;
	file_block[2] = CATENA_OPCODE;
	file_block[3] = FM_WRITE ;
	file_block[4] = FM_FUNCTION_FMFWDOWNLOAD ;
	file_block[5] = file_block_id ;
	/* Send the Packet */
	err = ste_fm_send_packet(file_block_length + 6, file_block);

	/* wait till response comes */
	os_get_cmd_sem();

	return err;
}

/* ------------------ External function definitions ---------------------- */

uint8_t fmd_init(
		void **context
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;

	*context = (void *)&fmd_state_info;
	fmd_state_info.logginglevel = 2;

	fmd_criticalsection_enter();

	if (os_fm_driver_init() == 0) {
		g_fmd_initalized = STE_TRUE;
		fmd_state_info.mode = FMD_MODE_IDLE;
		fmd_state_info.rx_freqrange = FMD_FREQRANGE_FMEUROAMERICA;
		fmd_state_info.rx_grid = FMD_GRID_100KHZ;
		fmd_state_info.rx_stereomode = FMD_STEREOMODE_AUTO;
		fmd_state_info.rx_volume = 20;
		fmd_state_info.rx_balance = 0;
		fmd_state_info.rx_mute = STE_FALSE;
		fmd_state_info.rx_antenna = FMD_ANTENNA_EMBEDDED;
		fmd_state_info.rx_deemphasis = FMD_EMPHASIS_75US;
		fmd_state_info.rx_rdson = STE_FALSE;
		fmd_state_info.rx_seek_stoplevel = 0x0100;
		fmd_state_info.gocmd = fmd_gocmd_none;
		fmd_state_info.callback = NULL;
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();

	FM_DEBUG_REPORT("fmd_init returning = %d", err);
	return err;
}

uint8_t fmd_register_callback(
		void *context,
		fmd_radio_cb callback
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		state->callback = callback;
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_get_version(
		void *context,
		uint16_t version[]
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) 	{
		unsigned int	IOresult;
		unsigned int    response_count;
		unsigned short  response_data[32];

		IOresult = fmd_send_cmd_and_read_resp(CMD_GEN_GETVERSION, 0,
			NULL, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else 	{
			os_mem_copy(version, response_data,
				sizeof(unsigned short)*7);
			err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();

	return err;
}

uint8_t fmd_set_mode(
		void *context,
		uint8_t mode
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	unsigned int   IOresult;
	unsigned int   response_count;
	unsigned short response_data[32];
	fmd_criticalsection_enter();
	if (g_fmd_initalized) {
		unsigned short parameters[1];
		switch (mode) {
		case FMD_MODE_RX:
			parameters[0] = 0x01;
			break;
		case FMD_MODE_TX:
			parameters[0] = 0x02;
			break;
		case FMD_MODE_IDLE:
		default:
			parameters[0] = 0x00;
			break;
		}

		IOresult = fmd_send_cmd_and_read_resp(CMD_GEN_GOTOMODE, 1,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else {
			state->mode = mode;
			state->gocmd = fmd_gocmd_setmode;
			err = FMD_RESULT_WAIT;
		}
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_get_mode(
		void *context,
		uint8_t *mode
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned int   response_count;
		unsigned short response_data[8];
		IOresult = fmd_send_cmd_and_read_resp(CMD_GEN_GETMODE, 0,
			NULL, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else {
			switch (response_data[0]) {
			case 0x00:
			default:
				*mode = FMD_MODE_IDLE;
				break;
			case 0x01:
				*mode = FMD_MODE_RX;
				break;
			case 0x02:
				*mode = FMD_MODE_TX;
				break;
			}
			err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();

	return err;
}

uint8_t fmd_is_freq_range_supported(
		void *context,
		uint8_t range,
		bool *supported
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		switch (range)	{
		case FMD_FREQRANGE_FMEUROAMERICA:
		case FMD_FREQRANGE_FMJAPAN:
		case FMD_FREQRANGE_FMCHINA:
			*supported = STE_TRUE;
			break;
		default:
			*supported = STE_FALSE;
			break;
		}

		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_get_freq_range_properties(
		void *context,
		uint8_t range,
		uint32_t *minfreq,
		uint32_t *maxfreq,
		uint32_t *freqinterval
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		switch (range)	{
		case FMD_FREQRANGE_FMEUROAMERICA:
			*minfreq =  87500;
			*maxfreq = 108000;
			*freqinterval = 50;
			break;
		case FMD_FREQRANGE_FMJAPAN:
			*minfreq = 76000;
			*maxfreq = 90000;
			*freqinterval = 50;
			break;
		case FMD_FREQRANGE_FMCHINA:
			*minfreq =  70000;
			*maxfreq = 108000;
			*freqinterval = 100;
			break;
		default:
			break;
		}
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_set_antenna(
		void *context,
		uint8_t ant
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[32];

		switch (ant) {
		case FMD_ANTENNA_EMBEDDED:
			parameters[0] = 0x0000;
			break;
		case FMD_ANTENNA_WIRED:
		default:
			parameters[0] = 0x0001;
			break;
		}

		IOresult = fmd_send_cmd_and_read_resp(CMD_FMR_SETANTENNA, 1,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else {
			state->rx_antenna = ant;
			state->gocmd = fmd_gocmd_setantenna;
			err = FMD_RESULT_WAIT;
		}
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_get_antenna(
		void *context,
		uint8_t *ant
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		*ant = state->rx_antenna;
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_set_freq_range(
		void *context,
		uint8_t range
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[8];
		unsigned int   response_count;
		unsigned short response_data[8];

		parameters[1] = 0;
		parameters[2] = 760;
		switch (range) {
		case FMD_FREQRANGE_FMEUROAMERICA:
		default:
			parameters[0] = 0;
			break;
		case FMD_FREQRANGE_FMJAPAN:
			parameters[0] = 1;
			break;
		case FMD_FREQRANGE_FMCHINA:
			parameters[0] = 2;
			break;
		}

		IOresult = fmd_send_cmd_and_read_resp(CMD_FMR_TN_SETBAND, 3,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else {
			state->rx_freqrange = range;
			fmd_callback((void *)&fmd_state_info,
				FMD_EVENT_FREQUENCY_RANGE_CHANGED ,
				0, STE_FALSE);
			err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_get_freq_range(
		void *context,
		uint8_t *range
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		*range = state->rx_freqrange;

		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_set_grid(
		void *context,
		uint8_t grid
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[32];

		switch (grid) {
		case FMD_GRID_50KHZ:
		default:
			parameters[0] = 0;
			break;
		case FMD_GRID_100KHZ:
			parameters[0] = 1;
			break;
		case FMD_GRID_200KHZ:
			parameters[0] = 2;
			break;
		}

		IOresult = fmd_send_cmd_and_read_resp(CMD_FMR_TN_SETGRID, 1,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else {
			state->rx_grid = grid;
			err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_get_grid(
		void *context,
		uint8_t *grid
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		*grid = state->rx_grid;
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_set_deemphasis(
		void *context,
		uint8_t deemphasis
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[32];

		switch (deemphasis) {
		case FMD_EMPHASIS_50US:
			parameters[0] = 1;
			break;
		case FMD_EMPHASIS_75US:
		default:
			parameters[0] = 2;
			break;
		}

		IOresult = fmd_send_cmd_and_read_resp(CMD_FMR_RP_SETDEEMPHASIS,
			1, parameters, &response_count,
			response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else {
			state->rx_deemphasis = deemphasis;
			err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_get_deemphasis(
		void *context,
		uint8_t *deemphasis
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		*deemphasis = state->rx_deemphasis;
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_set_frequency(
		void *context,
		uint32_t freq
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[8];

		parameters[0] = fmd_rx_frequency_to_channel(context, freq);

		IOresult = fmd_send_cmd_and_read_resp(
				CMD_FMR_SP_TUNE_SETCHANNEL,
				1, parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else {
			state->gocmd = fmd_gocmd_setfrequency;
			err = FMD_RESULT_WAIT;
		}
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_get_frequency(
		void *context,
		uint32_t *freq
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned int   response_count;
		unsigned short response_data[32];

		IOresult = fmd_send_cmd_and_read_resp(
				CMD_FMR_SP_TUNE_GETCHANNEL,
				0, NULL, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else {
			*freq = fmd_rx_channel_to_frequency(context,
				response_data[0]);
			err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_set_stereo_mode(
		void *context,
		uint32_t mode
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[8];
		unsigned int   response_count;
		unsigned short response_data[32];

		switch (mode) {
		case FMD_STEREOMODE_MONO:
			parameters[0] = 1;
			break;
		case FMD_STEREOMODE_STEREO:
			parameters[0] = 0;
			break;
		case FMD_STEREOMODE_AUTO:
			parameters[0] = 2;
			break;
		}

		IOresult = fmd_send_cmd_and_read_resp(CMD_FMR_RP_STEREO_SETMODE,
				1, parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else {
			state->rx_stereomode = mode;
			err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_get_stereo_mode(
		void *context,
		uint32_t *mode
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		*mode = state->rx_stereomode;
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_get_signal_strength(
		void *context,
		uint32_t *strength
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned int   response_count;
		unsigned short response_data[8];

		IOresult = fmd_send_cmd_and_read_resp(CMD_FMR_RP_GETRSSI,
			0, NULL, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else {
			*strength = response_data[0];
			err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_get_pilot_state(
		void *context,
		bool *on
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned int   response_count;
		unsigned short response_data[8];

		IOresult = fmd_send_cmd_and_read_resp(CMD_FMR_RP_GETSTATE,
			0, NULL, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else {
			*on = (response_data[0] == 0x0001);
			err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_set_stop_level(
		void *context,
		uint16_t stoplevel
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		state->rx_seek_stoplevel = stoplevel;
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;

	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_get_stop_level(
		void *context,
		uint16_t *stoplevel
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		*stoplevel = state->rx_seek_stoplevel;
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_seek(
		void *context,
		bool upwards
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[8];
		unsigned int   response_count;
		unsigned short response_data[32];

		if (upwards)
			parameters[0] = 0x0000;
		else
			parameters[0] = 0x0001;
		parameters[1] = state->rx_seek_stoplevel;
		parameters[2] = 0x7FFF;
		/*0x7FFF disables "running snr" criterion for search*/
		parameters[3] = 0x7FFF;
		/*0x7FFF disables "final snr" criterion for search*/

		IOresult = fmd_send_cmd_and_read_resp(CMD_FMR_SP_SEARCH_START,
				4, parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else {
			state->gocmd = fmd_gocmd_seek;
			err = FMD_RESULT_ONGOING;
		}
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_scan_band(
		void *context
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[8];
		unsigned int   response_count;
		unsigned short response_data[32];

		parameters[0] = 0x0020;
		parameters[1] = state->rx_seek_stoplevel;
		parameters[2] = 0x7FFF;
		parameters[3] = 0x7FFF;

		IOresult = fmd_send_cmd_and_read_resp(CMD_FMR_SP_SCAN_START,
			4, parameters, &response_count,
			response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else {
			state->gocmd = fmd_gocmd_scanband;
			err = FMD_RESULT_ONGOING;
		}
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_get_scan_band_info(
		void *context,
		uint32_t index,
		uint16_t *numchannels,
		uint16_t *channels,
		uint16_t *rssi
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[32];

		parameters[0] = index;

		IOresult = fmd_send_cmd_and_read_resp(
			CMD_FMR_SP_SCAN_GETRESULT, 1,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else {
			*numchannels = response_data[0];
			channels[0] = response_data[1];
			rssi[0] = response_data[2];
			channels[1] = response_data[3];
			rssi[1] = response_data[4];
			channels[2] = response_data[5];
			rssi[2] = response_data[6];
			err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_step_frequency(
		void *context,
		bool upwards
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[8];

		if (upwards)
			parameters[0] = 0x0000;
		else
			parameters[0] = 0x0001;

		IOresult = fmd_send_cmd_and_read_resp(
				CMD_FMR_SP_TUNE_STEPCHANNEL,
				1, parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else {
			state->gocmd = fmd_gocmd_stepfrequency;
			err = FMD_RESULT_WAIT;
		}
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_stop_seeking(
		void *context
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (g_fmd_initalized) {
		if ((state->gocmd == fmd_gocmd_seek)
			|| (state->gocmd == fmd_gocmd_scanband)) {
			unsigned int   IOresult;
			unsigned int   response_count;
			unsigned short response_data[32];

			IOresult = fmd_send_cmd_and_read_resp(
				CMD_FMR_SP_STOP, 0,
				NULL, &response_count, response_data);
			if (IOresult != FMD_RESULT_SUCCESS)
				err = IOresult;
			else {
				state->gocmd = fmd_gocmd_seekstop;
				err = FMD_RESULT_WAIT;
			}
		} else
			err = FMD_RESULT_PRECONDITIONS_VIOLATED;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_get_rds(
		void *context,
		bool *on
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		*on = fmd_state_info.rx_rdson;
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_query_rds_signal(
		void *context,
		bool *is_signal
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned int   response_count;
		unsigned short response_data[8];

		IOresult = fmd_send_cmd_and_read_resp(CMD_FMR_GETSTATE, 0,
			NULL, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else {
			*is_signal = (response_data[3] == 0x0001);
			err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_buffer_set_size(
		void *context,
		uint8_t size
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int	IOresult;
		unsigned short  parameters[1];
		unsigned int    response_count;
		unsigned short  response_data[8];

		if (size > 22)
			err = FMD_RESULT_IO_ERROR;
		else {
			parameters[0] = size;

			IOresult = fmd_send_cmd_and_read_resp(
					CMD_FMR_DP_BUFFER_SETSIZE, 1,
				parameters, &response_count, response_data);
			if (IOresult != FMD_RESULT_SUCCESS)
				err = IOresult;
			else
				err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_buffer_set_threshold(
		void *context,
		uint8_t threshold
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int	IOresult;
		unsigned short  parameters[1];
		unsigned int    response_count;
		unsigned short  response_data[8];
		if (threshold > 22)
			err = FMD_RESULT_IO_ERROR;
		else {
			parameters[0] = threshold;

			IOresult = fmd_send_cmd_and_read_resp(
					CMD_FMR_DP_BUFFER_SETTHRESHOLD, 1,
				parameters, &response_count, response_data);
			if (IOresult != FMD_RESULT_SUCCESS)
				err = IOresult;
			else
				err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_set_ctrl(
		void *context,
		uint8_t onoffState
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int	IOresult;
		unsigned short  parameters[1];
		unsigned int    response_count;
		unsigned short  response_data[8];

		switch (onoffState) {
		case FMD_SWITCH_OFF_RDS_SIMULATOR:
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

		IOresult = fmd_send_cmd_and_read_resp(CMD_FMR_DP_SETCONTROL, 1,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else
			err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_rx_get_low_level_rds_groups(
		void *context,
		uint8_t index,
		uint8_t *rds_buf_count,
		uint16_t *block1,
		uint16_t *block2,
		uint16_t *block3,
		uint16_t *block4,
		uint8_t *status1,
		uint8_t *status2,
		uint8_t *status3,
		uint8_t *status4
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		*rds_buf_count = state->rdsbufcnt;
		*block1  = state->rdsgroup[index].block[0];
		*block2  = state->rdsgroup[index].block[1];
		*block3  = state->rdsgroup[index].block[2];
		*block4  = state->rdsgroup[index].block[3];
		*status1 = state->rdsgroup[index].status[0];
		*status2 = state->rdsgroup[index].status[1];
		*status3 = state->rdsgroup[index].status[2];
		*status4 = state->rdsgroup[index].status[3];
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_set_audio_dac(
		void *context,
		uint8_t dac_state
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[8];

		parameters[0] = dac_state;

		IOresult = fmd_send_cmd_and_read_resp(CMD_AUP_SETAUDIODAC, 1,
				parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else
			err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_set_balance(
		void *context,
		int8_t balance
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[8];
		parameters[0] = (((signed short)balance)*0x7FFF) / 100;
		IOresult = fmd_send_cmd_and_read_resp(CMD_AUP_SETBALANCE, 1,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else {
			state->rx_balance = balance;
			err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_get_balance(
		void *context,
		int8_t *balance
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		*balance = state->rx_balance;
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_set_volume(
		void *context,
		uint8_t volume
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[8];

		parameters[0] = (((unsigned short)volume)*0x7FFF) / 100;

		IOresult = fmd_send_cmd_and_read_resp(CMD_AUP_SETVOLUME, 1,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else {
			state->rx_volume = volume;
			err = FMD_RESULT_SUCCESS;
		}
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_get_volume(
		void *context,
		uint8_t *volume
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		*volume = state->rx_volume;
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_set_mute(
		void *context,
		bool mute_on
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[2];
		unsigned int   response_count;
		unsigned short response_data[8];

		state->rx_mute = mute_on;
		if (!mute_on)
			parameters[0] = 0x0000;
		else
			parameters[0] = 0x0001;
		parameters[1] = 0x0001;

		IOresult = fmd_send_cmd_and_read_resp(CMD_AUP_SETMUTE, 2,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else {
			state->gocmd = fmd_gocmd_setmute;
			err = FMD_RESULT_WAIT;
		}
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_get_mute(
		void *context,
		bool *mute_on
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		*mute_on = state->rx_mute;
		err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_bt_set_ctrl(
		void *context,
		bool up_conversion
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[8];

		if (up_conversion)
			parameters[0] = 0x0000;
		else
			parameters[0] = 0x0001;

		IOresult = fmd_send_cmd_and_read_resp(CMD_AUP_BT_SETCONTROL, 1,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else
			err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_bt_set_mode(
		void *context,
		uint8_t output
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[8];

		switch (output) {
		case FMD_OUTPUT_DISABLED:
		default:
			parameters[0] = 0x0000;
			break;
		case FMD_OUTPUT_I2S:
			parameters[0] = 0x0001;
			break;
		case FMD_OUTPUT_PARALLEL:
			parameters[0] = 0x0002;
			break;
		case FMD_OUTPUT_FIFO:
			parameters[0] = 0x0003;
			break;
		}

		IOresult = fmd_send_cmd_and_read_resp(CMD_AUP_BT_SETMODE, 1,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else
			err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_ext_set_mode(
		void *context,
		uint8_t output
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[8];

		switch (output) {
		case FMD_OUTPUT_DISABLED:
		default:
			parameters[0] = 0x0000;
			break;
		case FMD_OUTPUT_I2S:
			parameters[0] = 0x0001;
			break;
		case FMD_OUTPUT_PARALLEL:
			parameters[0] = 0x0002;
			break;
		}

		IOresult = fmd_send_cmd_and_read_resp(CMD_AUP_EXT_SETMODE, 1,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else
			err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_ext_set_ctrl(
		void *context,
		bool up_conversion
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[8];

		if (up_conversion)
			parameters[0] = 0x0000;
		else
			parameters[0] = 0x0001;

		IOresult = fmd_send_cmd_and_read_resp(CMD_AUP_EXT_SETCONTROL, 1,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else
			err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_ext_set_mute(
		void *context,
		bool mute_on
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[2];
		unsigned int   response_count;
		unsigned short response_data[8];

		state->rx_mute = mute_on;
		if (!mute_on)
			parameters[0] = 0x0000;
		else
			parameters[0] = 0x0001;

		IOresult = fmd_send_cmd_and_read_resp(CMD_AUP_EXT_SETMUTE, 1,
			parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else
			err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_power_up(
		void *context
		)
{
	struct fmd_state_info_s *state = (struct fmd_state_info_s *)context;
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned int   response_count;
		unsigned short response_data[8];


		IOresult = fmd_send_cmd_and_read_resp(CMD_GEN_POWERUP, 0,
			NULL, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS) {
			err = IOresult;
		} else {
			state->gocmd = fmd_gocmd_genpowerup;
			err = FMD_RESULT_WAIT;
		}
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_goto_standby(
		void *context
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned int   response_count;
		unsigned short response_data[8];

		IOresult = fmd_send_cmd_and_read_resp(CMD_GEN_GOTOSTANDBY, 0,
			NULL, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else
			err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_goto_power_down(
		void *context
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned int   response_count;
		unsigned short response_data[8];

		IOresult = fmd_send_cmd_and_read_resp(CMD_GEN_GOTOPOWERDOWN, 0,
				NULL, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else
			err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_select_ref_clk(
		void *context,
		uint16_t ref_clk
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[8];

		parameters[0] = ref_clk;

		IOresult = fmd_send_cmd_and_read_resp(
				CMD_GEN_SELECTREFERENCECLOCK,
				1, parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else
			err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

uint8_t fmd_select_ref_clk_pll(
		void *context,
		uint16_t freq
		)
{
	int err = FMD_RESULT_FEATURE_UNSUPPORTED;
	fmd_criticalsection_enter();

	if (fmd_go_cmd_busy())
		err = FMD_RESULT_BUSY;
	else if (g_fmd_initalized) {
		unsigned int   IOresult;
		unsigned short parameters[1];
		unsigned int   response_count;
		unsigned short response_data[8];

		parameters[0] = freq;

		IOresult = fmd_send_cmd_and_read_resp(
				CMD_GEN_SETREFERENCECLOCKPLL,
				1, parameters, &response_count, response_data);
		if (IOresult != FMD_RESULT_SUCCESS)
			err = IOresult;
		else
			err = FMD_RESULT_SUCCESS;
	} else
		err = FMD_RESULT_IO_ERROR;
	fmd_criticalsection_leave();
	return err;
}

unsigned int fmd_send_fm_ip_enable(void)
{
	uint8_t FMIP_EnableCmd[5];
	unsigned int err = 0;

	FMIP_EnableCmd[0] = HCI_PACKET_INDICATOR_FM_CMD_EVT;
	FMIP_EnableCmd[1] = 0x03 ; /* Length of following Bytes */
	FMIP_EnableCmd[2] = CATENA_OPCODE;
	FMIP_EnableCmd[3] = FM_WRITE ;
	FMIP_EnableCmd[4] = FM_FUNCTION_ENABLE;

	/* Send the Packet */
	err = ste_fm_send_packet(5 , FMIP_EnableCmd);

	/* wait till response comes */
	os_get_cmd_sem();

	return err;
}

unsigned int fmd_send_fm_ip_disable(void)
{
	uint8_t FMIP_DisableCmd[5];
	unsigned int err = 0;

	FMIP_DisableCmd[0] = HCI_PACKET_INDICATOR_FM_CMD_EVT;
	FMIP_DisableCmd[1] = 0x03 ; /* Length of following Bytes */
	FMIP_DisableCmd[2] = CATENA_OPCODE;
	FMIP_DisableCmd[3] = FM_WRITE ;
	FMIP_DisableCmd[4] = FM_FUNCTION_DISABLE;

	/* Send the Packet */
	err = ste_fm_send_packet(5 , FMIP_DisableCmd);

	/* wait till response comes */
	os_get_cmd_sem();

	return err;
}

unsigned int fmd_send_fm_ip_reset(void)
{
	uint8_t FMIP_ResetCmd[5];
	unsigned int err = 0;

	FMIP_ResetCmd[0] = HCI_PACKET_INDICATOR_FM_CMD_EVT;
	FMIP_ResetCmd[1] = 0x03 ; /* Length of following Bytes */
	FMIP_ResetCmd[2] = CATENA_OPCODE;
	FMIP_ResetCmd[3] = FM_WRITE ;
	FMIP_ResetCmd[4] = FM_FUNCTION_RESET;

	/* Send the Packet */
	err = ste_fm_send_packet(5 , FMIP_ResetCmd);

	/* wait till response comes */
	os_get_cmd_sem();

	return err;
}

unsigned int fmd_send_fm_firmware(
		uint8_t *fw_buffer,
		uint16_t fw_size
		)
{
	unsigned int err = 0;
	uint16_t bytes_to_write = ST_WRITE_FILE_BLK_SIZE - 4;
	uint16_t bytes_remaining = fw_size;
	uint8_t fm_firmware_data[ST_WRITE_FILE_BLK_SIZE + 6];
	uint32_t block_id = 0;

	while (bytes_remaining > 0) {
		if (bytes_remaining < ST_WRITE_FILE_BLK_SIZE - 4)
			bytes_to_write = bytes_remaining;

		/* Six bytes of HCI Header for FM Firmware
		 * so shift the firmware data by 6 bytes
		 */
		os_mem_copy(fm_firmware_data + 6, fw_buffer, bytes_to_write);
		err = fmd_st_write_file_block(block_id , fm_firmware_data ,
			bytes_to_write);
		if (!err) {
			block_id++;
			fw_buffer += bytes_to_write;
			bytes_remaining -= bytes_to_write;

			if (block_id == 256)
				block_id = 0;
		} else {
			FM_DEBUG_REPORT("fmd_send_fm_firmware: "\
				"Failed to download %d"\
				"Block, error = %d",
				(unsigned int)block_id, err);
				return err;
		}
	}

	return err;
}

void fmd_receive_data(
		uint32_t packet_length,
		uint8_t *packet_buffer
		)
{
	if (packet_buffer != NULL) {
		if (packet_length == 0) {
			uint32_t interrupt;
			interrupt = ((uint32_t) *(packet_buffer + 1) << 8) |
				((uint32_t) *packet_buffer) ;
			FM_DEBUG_REPORT("interrupt = %04x",
				(unsigned int)interrupt);
			fmd_interrupt_handler(interrupt);
		} else {
			switch (packet_buffer[0]) {
			case FM_CMD_STATUS_CMD_SUCCESS:
			{
				fmd_process_fm_function(packet_buffer+1);
			}
			break;
			case FM_CMD_STATUS_HCI_ERR_HW_FAILURE:
			{
				FM_DEBUG_REPORT(
					"FM_CMD_STATUS_HCI_ERR_HW_FAILURE");
			}
			break;
			case FM_CMD_STATUS_HCI_ERR_INVALID_PARAMETERS:
			{
				FM_DEBUG_REPORT(
				"FM_CMD_STATUS_HCI_ERR_INVALID_PARAMETERS");
			}
			break;
			case FM_CMD_STATUS_IP_UNINIT:
			{
				FM_DEBUG_REPORT("FM_CMD_STATUS_IP_UNINIT");
			}
			break;
			case FM_CMD_STATUS_HCI_ERR_UNSPECIFIED_ERROR:
			{
				FM_DEBUG_REPORT(
				"FM_CMD_STATUS_HCI_ERR_UNSPECIFIED_ERROR");
			}
			break;
			case FM_CMD_STATUS_HCI_ERR_CMD_DISALLOWED:
			{
				FM_DEBUG_REPORT(
				"FM_CMD_STATUS_HCI_ERR_CMD_DISALLOWED");
			}
			break;
			case FM_CMD_STATUS_WRONG_SEQ_NUM:
			{
				FM_DEBUG_REPORT(
					"FM_CMD_STATUS_WRONG_SEQ_NUM");
			}
			break;
			case FM_CMD_STATUS_UNKOWNFILE_TYPE:
			{
				FM_DEBUG_REPORT(
					"FM_CMD_STATUS_UNKOWNFILE_TYPE");
			}
			break;
			case FM_CMD_STATUS_FILE_VERSION_MISMATCH:
			{
				FM_DEBUG_REPORT(
					"FM_CMD_STATUS_FILE_VERSION_MISMATCH");
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
	unsigned int   response_count;
	unsigned short response_data[16];
	unsigned short cnt;
	unsigned short index = 0;
	unsigned int   IOresult;
	struct fmd_rdsgroup_s rdsgroup;

	if (fmd_state_info.rx_rdson) {
		/* get group count*/
		IOresult = fmd_send_cmd_and_read_resp(
			CMD_FMR_DP_BUFFER_GETGROUPCOUNT, 0, NULL,
			&response_count, response_data);
		if (IOresult == FMD_RESULT_SUCCESS) {
			/* read RDS groups */
			cnt = response_data[0];
			if (cnt > MAX_RDS_GROUPS_READ)
				cnt = MAX_RDS_GROUPS_READ;
			fmd_state_info.rdsbufcnt = cnt;
			while (cnt-- && fmd_state_info.rx_rdson) {
				IOresult = fmd_send_cmd_and_read_resp(
					CMD_FMR_DP_BUFFER_GETGROUP, 0, NULL,
					&response_count,
					(unsigned short *)&rdsgroup);
				if (IOresult == FMD_RESULT_SUCCESS) {
					if (fmd_state_info.rx_rdson)
						fmd_state_info.rdsgroup[index++]
						= rdsgroup;
				}
			}
		}
	}
}

void fmd_hexdump(
		char prompt,
		uint8_t *arr,
		int num_bytes
		)
{
	int i;
	uint8_t tmpVal;
	static unsigned char pkt_write[512], *pkt_ptr;
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

