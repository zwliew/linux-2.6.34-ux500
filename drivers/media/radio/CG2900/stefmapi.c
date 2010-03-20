/*
 * file stefmapi.c
 *
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Linux FM Host API's for ST-Ericsson FM Chip.
 *
 * Author: Hemant Gupta/hemant.gupta@stericsson.com for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2
 */

#include "stefmapi.h"
#include "platformosapi.h"
#include "fmdriver.h"
#include "videodev.h"
#include "v4l2-ioctl.h"
#include "firmware.h"

#define STE_FM_BT_SRC_COEFF_INFO_FILE	"ste_fm_bt_src_coeff_info.fw"
#define STE_FM_EXT_SRC_COEFF_INFO_FILE	"ste_fm_ext_src_coeff_info.fw"
#define STE_FM_FM_COEFF_INFO_FILE	"ste_fm_fm_coeff_info.fw"
#define STE_FM_FM_PROG_INFO_FILE	"ste_fm_fm_prog_info.fw"
#define STE_FM_LINE_BUFFER_LENGTH	128
#define STE_FM_FILENAME_MAX		128

static	uint16_t	vol_index;
static 	bool	fm_init;
static 	bool	fm_power_on;
static 	bool	fm_stand_by;
static 	bool	fm_rds_status;
static 	bool	fm_prev_rds_status;
static void *context;
static uint8_t gfreq_range = FMD_FREQRANGE_FMEUROAMERICA;
static struct mutex	rds_mutex;

uint8_t global_event;
uint8_t head;
uint8_t tail;
uint8_t rds_group_sent;
uint8_t rds_block_sent;
struct ste_fm_rds_buf_s  ste_fm_rds_buf[MAX_RDS_BUFFER][MAX_RDS_GROUPS_READ];

static uint32_t hci_revision = 0x0700;
static uint32_t lmp_sub_version = 0x0011;

/* ------------------ Internal function declarations ---------------------- */
static const char *str_stestatus(
		uint8_t status
		);
static void  ste_fm_driver_callback(
		void *context,
		uint32_t event,
		uint32_t event_int_data,
		bool event_boolean_data
		);
static void ste_fm_rds_callback(void);
static char *ste_fm_get_one_line_of_text(
		char *wr_buffer,
		int max_nbr_of_bytes,
		char *rd_buffer,
		int *bytes_copied
		);
static bool ste_fm_get_file_to_load(
		const struct firmware *fw,
		char **file_name
		);
static uint8_t ste_fm_load_firmware(
		struct device *device
		);


/* ------------------ Internal function definitions ---------------------- */
/**
 * ste_fm_get_one_line_of_text()- Replacement function for stdio
 * function fgets.
 * The ste_fm_get_one_line_of_text() function extracts one line of text
 * from input file.
 * @wr_buffer: Buffer to copy text to.
 * @max_nbr_of_bytes: Max number of bytes to read, i.e. size of rd_buffer.
 * @rd_buffer: Data to parse.
 * @bytes_copied: Number of bytes copied to wr_buffer.
 *
 * Returns:
 *   Pointer to next data to read.
 */
static char *ste_fm_get_one_line_of_text(
		char *wr_buffer,
		int max_nbr_of_bytes,
		char *rd_buffer,
		int *bytes_copied
		)
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
	} while ((*bytes_copied <= max_nbr_of_bytes) && (in_byte != '\0')
		&& (in_byte != '\n'));
	*curr_wr = '\0';
	return curr_rd;
}

/**
 * ste_fm_get_file_to_load() - Parse info file and find correct target file.
 * @fw:	Firmware structure containing file data.
 * @file_name: (out) Pointer to name of requested file.
 *
 * Returns:
 *   True,  if target file was found,
 *   False, otherwise.
 */
static bool ste_fm_get_file_to_load(
		const struct firmware *fw,
		char **file_name
		)
{
	char *line_buffer;
	char *curr_file_buffer;
	int bytes_left_to_parse = fw->size;
	int bytes_read = 0;
	bool file_found = false;

	curr_file_buffer = (char *)&(fw->data[0]);

	line_buffer = kzalloc(STE_FM_LINE_BUFFER_LENGTH, GFP_KERNEL);

	if (line_buffer) {
		while (!file_found) {
			/* Get one line of text from the file to parse */
			curr_file_buffer = \
			ste_fm_get_one_line_of_text(line_buffer,
				min(STE_FM_LINE_BUFFER_LENGTH,
				(int)(fw->size - bytes_read)),
				curr_file_buffer,
				&bytes_read);

			bytes_left_to_parse -= bytes_read;
			if (bytes_left_to_parse <= 0) {
				/* End of file => Leave while loop */
				FM_ERR_REPORT("Reached end of file." \
					"No file found!");
				break;
			}

			/* Check if the line of text is a comment
			 * or not, comments begin with '#' */
			if (*line_buffer != '#') {
				uint32_t     hci_rev = 0;
				uint32_t     lmp_sub = 0;

				FM_DEBUG_REPORT("Found a valid line <%s>",
					line_buffer);

				/* Check if we can find the correct
				 * HCI revision and LMP subversion
				 * as well as a file name in the text line
				 * Store the filename if the actual file can
				 * be found in the file system
				 */
				if (sscanf(line_buffer, "%x%x%s",
					(unsigned int *)&hci_rev, \
					(unsigned int *)&lmp_sub,
					*file_name) == 3 \
					&& hci_rev == hci_revision
					&& lmp_sub == lmp_sub_version) {
						FM_INFO_REPORT(\
						"File name = %s " \
						"HCI Revision" \
						"= 0x%X LMP PAL" \
						"Subversion = 0x%X", \
						*file_name,
						(unsigned int)hci_rev,
						(unsigned int)lmp_sub);

						/* Name has already
						 * been stored above.
						 * Nothing more to do */
						file_found = true;
				} else {
					/* Zero the name buffer so
					 * it is clear to next read */
					memset(*file_name, 0x00,
					STE_FM_FILENAME_MAX + 1);
				}
			}
		}
		kfree(line_buffer);
	} else {
		FM_ERR_REPORT("Failed to allocate:" \
			"file_name 0x%X, line_buffer 0x%X",
			(unsigned int)file_name,
			(unsigned int)line_buffer);
	}

	return file_found;
}

/**
 * ste_fm_load_firmware() - Loads the FM Coeffecients and F/W file(s)
 * @device: Pointer to char device requesting the operation.
 *
 * Returns:
 *   STE_STATUS_OK, if firmware donload is successful
 *   STE_STATUS_SYSTEM_ERROR, otherwise.
*/
static uint8_t ste_fm_load_firmware(
		struct device *device
		)
{
	int err = 0;
	bool file_found;
	uint8_t result = STE_STATUS_OK;
	const struct firmware *bt_src_coeff_info;
	const struct firmware *ext_src_coeff_info;
	const struct firmware *fm_coeff_info;
	const struct firmware *fm_prog_info;
	char *bt_src_coeff_file_name = NULL;
	char *ext_src_coeff_file_name = NULL;
	char *fm_coeff_file_name = NULL;
	char *fm_prog_file_name = NULL;

	FM_INFO_REPORT("+ste_fm_load_firmware");

	/* Open bt_src_coeff info file. */
	err = request_firmware(&bt_src_coeff_info,
		STE_FM_BT_SRC_COEFF_INFO_FILE, device);
	if (err) {
		FM_ERR_REPORT("Couldn't get bt_src_coeff info file");
		result =  STE_STATUS_SYSTEM_ERROR;
		goto error_handling;
	}

	/* Now we have the bt_src_coeff info file.
	 * See if we can find the right bt_src_coeff file as well */
	bt_src_coeff_file_name = os_mem_alloc(STE_FM_FILENAME_MAX);
	file_found = ste_fm_get_file_to_load(bt_src_coeff_info,
		&bt_src_coeff_file_name);

	/* Now we are finished with the bt_src_coeff info file */
	release_firmware(bt_src_coeff_info);

	if (!file_found) {
		FM_ERR_REPORT("Couldn't find bt_src_coeff file! Major error!");
		result =  STE_STATUS_SYSTEM_ERROR;
		goto error_handling;
	}

	/* Open ext_src_coeff info file. */
	err = request_firmware(&ext_src_coeff_info,
		STE_FM_EXT_SRC_COEFF_INFO_FILE, device);
	if (err) {
		FM_ERR_REPORT("Couldn't get ext_src_coeff_info info file");
		result =  STE_STATUS_SYSTEM_ERROR;
		goto error_handling;
	}

	/* Now we have the ext_src_coeff info file. See if we can
	 * find the right ext_src_coeff file as well */
	ext_src_coeff_file_name = os_mem_alloc(STE_FM_FILENAME_MAX);
	file_found = ste_fm_get_file_to_load(ext_src_coeff_info,
		&ext_src_coeff_file_name);

	/* Now we are finished with the ext_src_coeff info file */
	release_firmware(ext_src_coeff_info);

	if (!file_found) {
		FM_ERR_REPORT("Couldn't find ext_src_coeff_info "\
					  "file! Major error!");
		result =  STE_STATUS_SYSTEM_ERROR;
		goto error_handling;
	}

	/* Open fm_coeff info file. */
	err = request_firmware(&fm_coeff_info,
		STE_FM_FM_COEFF_INFO_FILE, device);
	if (err) {
		FM_ERR_REPORT("Couldn't get fm_coeff info file");
		result =  STE_STATUS_SYSTEM_ERROR;
		goto error_handling;
	}

	/* Now we have the fm_coeff_info info file.
	 * See if we can find the right fm_coeff_info file as well */
	fm_coeff_file_name = os_mem_alloc(STE_FM_FILENAME_MAX);
	file_found = ste_fm_get_file_to_load(fm_coeff_info,
		&fm_coeff_file_name);

	/* Now we are finished with the fm_coeff info file */
	release_firmware(fm_coeff_info);

	if (!file_found) {
		FM_ERR_REPORT("Couldn't find fm_coeff file! Major error!");
		result =  STE_STATUS_SYSTEM_ERROR;
		goto error_handling;
	}

	/* Open fm_prog info file. */
	err = request_firmware(&fm_prog_info,
		STE_FM_FM_PROG_INFO_FILE, device);
	if (err) {
		FM_ERR_REPORT("Couldn't get fm_prog_info info file");
		result =  STE_STATUS_SYSTEM_ERROR;
		goto error_handling;
	}

	/* Now we have the fm_prog info file.
	 * See if we can find the right fm_prog file as well
	 */
	fm_prog_file_name = os_mem_alloc(STE_FM_FILENAME_MAX);
	file_found = ste_fm_get_file_to_load(fm_prog_info,
		&fm_prog_file_name);

	/* Now we are finished with fm_prog patch info file */
	release_firmware(fm_prog_info);

	if (!file_found) {
		FM_ERR_REPORT("Couldn't find fm_prog_info file! Major error!");
		result =  STE_STATUS_SYSTEM_ERROR;
		goto error_handling;
	}

	/* OK. Now it is time to download the firmware */
	err = request_firmware(&bt_src_coeff_info,
		bt_src_coeff_file_name, device);
	if (err < 0) {
		FM_ERR_REPORT("Couldn't get bt_src_coeff file, err = %d", err);
		result =  STE_STATUS_SYSTEM_ERROR;
		goto error_handling;
	}

	FM_INFO_REPORT("ste_fm_load_firmware: Downloading %s of %d bytes",
		bt_src_coeff_file_name, bt_src_coeff_info->size);
	if (fmd_send_fm_firmware((uint8_t *)bt_src_coeff_info->data,
		bt_src_coeff_info->size)) {
		FM_ERR_REPORT("ste_fm_load_firmware: Error in downloading %s",
			bt_src_coeff_file_name);
		result =  STE_STATUS_SYSTEM_ERROR;
		goto error_handling;
	}

	/* Now we are finished with the bt_src_coeff info file */
	release_firmware(bt_src_coeff_info);
	os_mem_free(bt_src_coeff_file_name);

	err = request_firmware(&ext_src_coeff_info,
		ext_src_coeff_file_name, device);
	if (err < 0) {
		FM_ERR_REPORT("Couldn't get ext_src_coeff file, err = %d", err)
			goto error_handling;
	}

	FM_INFO_REPORT("ste_fm_load_firmware: Downloading %s of %d bytes",
		ext_src_coeff_file_name, ext_src_coeff_info->size);
	if (fmd_send_fm_firmware((uint8_t *)ext_src_coeff_info->data,
		ext_src_coeff_info->size)) {
		FM_ERR_REPORT("ste_fm_load_firmware: Error in downloading %s",
			ext_src_coeff_file_name);
		result =  STE_STATUS_SYSTEM_ERROR;
		goto error_handling;
	}

	/* Now we are finished with the bt_src_coeff info file */
	release_firmware(ext_src_coeff_info);
	os_mem_free(ext_src_coeff_file_name);

	err = request_firmware(&fm_coeff_info,
				fm_coeff_file_name,	device);
	if (err < 0) {
		FM_ERR_REPORT("Couldn't get fm_coeff file, err = %d", err)
			goto error_handling;
	}

	FM_INFO_REPORT("ste_fm_load_firmware: Downloading %s of %d bytes",
		fm_coeff_file_name, fm_coeff_info->size);
	if (fmd_send_fm_firmware((uint8_t *)fm_coeff_info->data,
		fm_coeff_info->size)) {
		FM_ERR_REPORT("ste_fm_load_firmware: Error in downloading %s",
			fm_coeff_file_name);
		result =  STE_STATUS_SYSTEM_ERROR;
		goto error_handling;
	}

	/* Now we are finished with the bt_src_coeff info file */
	release_firmware(fm_coeff_info);
	os_mem_free(fm_coeff_file_name);

	err = request_firmware(&fm_prog_info, fm_prog_file_name, device);
	if (err < 0) {
		FM_ERR_REPORT("Couldn't get fm_prog file, err = %d", err)
			goto error_handling;
	}

	FM_INFO_REPORT("ste_fm_load_firmware: Downloading %s of %d bytes",
		fm_prog_file_name, fm_prog_info->size);
	if (fmd_send_fm_firmware((uint8_t *)fm_prog_info->data,
		fm_prog_info->size)) {
		FM_ERR_REPORT("ste_fm_load_firmware: Error in downloading %s",
			fm_prog_file_name);
		result =  STE_STATUS_SYSTEM_ERROR;
		goto error_handling;
	}

	/* Now we are finished with the bt_src_coeff info file */
	release_firmware(fm_prog_info);
	os_mem_free(fm_prog_file_name);

error_handling:
	FM_DEBUG_REPORT("-ste_fm_load_firmware returning = %d", result);
	return result;
}

/**
 * str_stestatus()- Returns the status value as a printable string.
 * @status: Status
 *
 * Returns:
 *	 Pointer to a string containing printable value of status
 */
static const char *str_stestatus(
		uint8_t status
		)
{
	switch (status) {
	case STE_STATUS_OK:
		return "STE_STATUS_OK";
	case STE_STATUS_SYSTEM_ERROR:
		return "STE_STATUS_SYSTEM_ERROR";
	case STE_STATUS_SEARCH_NO_CHANNEL_FOUND:
		return "STE_STATUS_SEARCH_NO_CHANNEL_FOUND";
	case STE_STATUS_SCAN_NO_CHANNEL_FOUND:
		return "STE_STATUS_SCAN_NO_CHANNEL_FOUND";
	default:
		return "Unknown status";
	}
}

/**
 * ste_fm_driver_callback()- Callback function indicating the event.
 * This callback function is called on receiving irpt_CommandSucceeded,
 * irpt_CommandFailed, irpt_bufferFull, etc from FM chip.
 * @context: Pointer to the Context of the FM Driver
 * @event: event for which the callback function was caled
 * from FM Driver.
 * @event_int_data: Data corresponding the event (if any),
 * otherwise it is 0.
 * @event_boolean_data: Signifying whether the event is called from FM Driver
 * on receiving irpt_OperationSucceeded or irpt_OperationFailed.
 */
static void  ste_fm_driver_callback(
		void *context,
		uint32_t event,
		uint32_t event_int_data,
		bool event_boolean_data
		)
{
	FM_DEBUG_REPORT("ste_fm_driver_callback: "\
		"event = %x, event_boolean_data = %x",
		event,
		event_boolean_data);
	switch (event) {
	case FMD_EVENT_GEN_POWERUP:
		FM_DEBUG_REPORT("FMD_EVENT_GEN_POWERUP");
		break;
	case FMD_EVENT_ANTENNA_STATUS_CHANGED:
		FM_DEBUG_REPORT("FMD_EVENT_ANTENNA_STATUS_CHANGED");
		break;
	case FMD_EVENT_FREQUENCY_CHANGED:
		FM_DEBUG_REPORT("FMD_EVENT_FREQUENCY_CHANGED ");
		break;

	case FMD_EVENT_FREQUENCY_RANGE_CHANGED:
		FM_DEBUG_REPORT("FMD_EVENT_FREQUENCY_RANGE_CHANGED");
		break;

	case FMD_EVENT_ACTION_FINISHED:
		os_set_cmd_sem();
		FM_DEBUG_REPORT("FMD_EVENT_ACTION_FINISHED ");
		break;

	case FMD_EVENT_PRESET_CHANGED:
		FM_DEBUG_REPORT("FMD_EVENT_PRESET_CHANGED ");
		break;

	case FMD_EVENT_SEEK_STOPPED:
		FM_DEBUG_REPORT("FMD_EVENT_SEEK_STOPPED");
		global_event = STE_EVENT_NO_CHANNELS_FOUND;
		wake_up_poll_queue();
		break;

	case FMD_EVENT_SEEK_COMPLETED:
		FM_DEBUG_REPORT("FMD_EVENT_SEEK_COMPLETED");
		global_event =  STE_EVENT_SEARCH_CHANNEL_FOUND ;
		wake_up_poll_queue();
		break;

	case FMD_EVENT_SCAN_BAND_COMPLETED:
		FM_DEBUG_REPORT("FMD_EVENT_SCAN_BAND_COMPLETED");
		global_event = STE_EVENT_SCAN_CHANNELS_FOUND;
		wake_up_poll_queue();
		break;

	case FMD_EVENT_RDSGROUP_RCVD:
		FM_DEBUG_REPORT("FMD_EVENT_RDSGROUP_RCVD");
		os_set_rds_sem();
		break;

	default:
		FM_ERR_REPORT("ste_fm_driver_callback: "\
			"Unknown event = %x",
			event);
		break;
	}
}

/**
 * ste_fm_rds_callback()- Function to retrieve the RDS groups from
 * chip on receiving
 * irpt_BufferFull interrupt from Chip.
 */
static void ste_fm_rds_callback(void)
{
	uint8_t index = 0;
	uint8_t rds_local_buf_count;
	FM_DEBUG_REPORT("ste_fm_rds_callback");
	if (fm_rds_status) {
		/* Wait till interrupt is RDS Buffer
		 * Full interrupt is received */
		os_get_rds_sem();
	}
	if (fm_rds_status) {
		/* RDS Data available, Read the Groups */
		mutex_lock(&rds_mutex);
		fmd_int_bufferfull();
		/* Atleast 1 RDS buffer is available,
		 * so read it and the actual buffers
		 * would be updated */
		rds_local_buf_count = 1;
		while (index < rds_local_buf_count) {
			fmd_rx_get_low_level_rds_groups(context,
				index,
				&rds_local_buf_count,
				&ste_fm_rds_buf[head][index].block1,
				&ste_fm_rds_buf[head][index].block2,
				&ste_fm_rds_buf[head][index].block3,
				&ste_fm_rds_buf[head][index].block4,
				&ste_fm_rds_buf[head][index].status2,
				&ste_fm_rds_buf[head][index].status1,
				&ste_fm_rds_buf[head][index].status4,
				&ste_fm_rds_buf[head][index].status3);
			 /*FM_DEBUG_REPORT("%04x %04x "\
			"%04x %04x %02x %02x "\
			"%02x %02x", ste_fm_rds_buf[head][index].block1,
			ste_fm_rds_buf[head][index].block2,
			ste_fm_rds_buf[head][index].block3,
			ste_fm_rds_buf[head][index].block4,
			ste_fm_rds_buf[head][index].status1,
			ste_fm_rds_buf[head][index].status2,
			ste_fm_rds_buf[head][index].status3,
			ste_fm_rds_buf[head][index].status4);*/
			index++;
		}
		head++;
		if (head == MAX_RDS_BUFFER)
			head = 0;
		mutex_unlock(&rds_mutex);
	}
}

/* ------------------ External function definitions ---------------------- */

uint8_t ste_fm_init(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_init");

	if (STE_FALSE == fm_init) {

		mutex_init(&rds_mutex);

		/* Initalize the Driver */
		if (fmd_init(&context) != FMD_RESULT_SUCCESS)
			return STE_STATUS_SYSTEM_ERROR;

		/* Register the callback */
		if (fmd_register_callback(context,
			(fmd_radio_cb)ste_fm_driver_callback)
			!= FMD_RESULT_SUCCESS)
			return STE_STATUS_SYSTEM_ERROR;

		/* initialize global variables */
		global_event = STE_EVENT_NO_EVENT;
		if (STE_STATUS_OK == result)	{
			fm_init = STE_TRUE;
			fm_power_on = STE_FALSE;
			fm_stand_by = STE_FALSE;
		}
	} else {
		FM_ERR_REPORT("ste_fm_init: Already Initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_init: returning %s", str_stestatus(result));
	return result;

}

uint8_t ste_fm_deinit(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_deinit");

	if (STE_TRUE == fm_init) {
		if (STE_STATUS_OK == result) {
			os_fm_driver_deinit();
			context = NULL;
			fm_power_on = STE_FALSE;
			fm_stand_by = STE_FALSE;
			fm_init = STE_FALSE;
		}
	} else {
		FM_ERR_REPORT("ste_fm_deinit: Already De-initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_deinit: returning %s", str_stestatus(result));
	return result;
}

uint8_t ste_fm_switch_on(
		struct device *device,
		uint32_t freq,
		uint8_t band,
		uint8_t grid
		)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_switch_on: freq = %d Hz, "\
		"band = %d, grid = %d", freq, band, grid);

	if (STE_TRUE == fm_init) {
		if (STE_FALSE == fm_power_on) {

			/* Enable FM IP */
			FM_DEBUG_REPORT("ste_fm_switch_on: "\
				"Sending FM IP Enable");

			if (fmd_send_fm_ip_enable()) {
				FM_ERR_REPORT("ste_fm_switch_on: "\
				"Error in fmd_send_fm_ip_enable");
				result =  STE_STATUS_SYSTEM_ERROR;
				goto error;
			}

			/* Now Download the Coefficient Files and FM Firmware */
			if (STE_STATUS_OK != ste_fm_load_firmware(device)) {
				FM_ERR_REPORT("ste_fm_switch_on: "\
				"Error in downloading firmware");
				result =  STE_STATUS_SYSTEM_ERROR;
				goto error;
			}

			if (STE_STATUS_OK == result) {
				/* Power up FM */
				result = fmd_power_up(context);
				if (FMD_RESULT_WAIT != result) {
					FM_ERR_REPORT("ste_fm_switch_on: "\
						"fmd_power_up failed %x",
						(unsigned int)result);
					result =  STE_STATUS_SYSTEM_ERROR;
					goto error;
				} else {
					/* Wait till cmd is complete */
					os_get_cmd_sem();
					result =  STE_STATUS_OK;
				}
			}

			if (STE_STATUS_OK == result) {
				/* Switch To Rx mode */
				FM_DEBUG_REPORT("ste_fm_switch_on: "\
					"fmd_power_up interrupt received, "\
					"Sending Set mode to Rx");
				result = fmd_set_mode(context, FMD_MODE_RX);
				if (FMD_RESULT_WAIT != result) {
					FM_ERR_REPORT("ste_fm_switch_on: "\
						"fmd_set_mode failed %x", \
						(unsigned int)result);
					result =  STE_STATUS_SYSTEM_ERROR;
					goto error;
				}  else {
					/* Wait till cmd is complete */
					os_get_cmd_sem();
					result =  STE_STATUS_OK;
				}
			}

			if (STE_STATUS_OK == result) {
				/* Power Up Audio DAC */
				FM_DEBUG_REPORT("ste_fm_switch_on: "\
					"Set mode To Rx interrupt "\
					"received, Sending Set "\
					"fmd_set_audio_dac");
				result = fmd_set_audio_dac
					(context, STE_FM_DAC_ON);
				if (FMD_RESULT_SUCCESS != result) {
					FM_ERR_REPORT("ste_fm_switch_on: "\
					"FMRSetAudioDAC failed %x", \
					(unsigned int)result);
					result =  STE_STATUS_SYSTEM_ERROR;
					goto error;
				} else
					result =  STE_STATUS_OK;
			}

			if (STE_STATUS_OK == result) {
				/* Enable the Ref Clk */
				FM_DEBUG_REPORT("ste_fm_switch_on: "\
					"Sending Set fmd_select_ref_clk");
				result = fmd_select_ref_clk(context, 0x0001);
				if (FMD_RESULT_SUCCESS != result) {
					FM_ERR_REPORT("ste_fm_switch_on: "\
					"fmd_select_ref_clk failed %x", \
					(unsigned int)result);
					result =  STE_STATUS_SYSTEM_ERROR;
					goto error;
				} else
					result =  STE_STATUS_OK;
			}

			if (STE_STATUS_OK == result) {
				/* Select the Frequency of Ref Clk (28 MHz)*/
				FM_DEBUG_REPORT("ste_fm_switch_on: "\
				"Sending Set fmd_select_ref_clk_pll");
				result = fmd_select_ref_clk_pll
					(context, 0x32C8);
				if (FMD_RESULT_SUCCESS != result) {
					FM_ERR_REPORT("ste_fm_switch_on: "\
					"fmd_select_ref_clk_pll failed %x", \
					(unsigned int)result);
					result =  STE_STATUS_SYSTEM_ERROR;
					goto error;
				} else
					result =  STE_STATUS_OK;
			}

			if (STE_STATUS_OK == result) {
				/* Set the Grid */
				result = fmd_rx_set_grid(context, grid);
				if (FMD_RESULT_SUCCESS != result) {
					FM_ERR_REPORT("ste_fm_switch_on: "\
					"fmd_rx_set_grid failed %x", \
					(unsigned int)result);
					result =  STE_STATUS_SYSTEM_ERROR;
					goto error;
				} else
					result =  STE_STATUS_OK;
			}

			if (STE_STATUS_OK == result) {
				switch (band) {
				case STE_FM_BAND_US_EU:
				default:
					gfreq_range =
						FMD_FREQRANGE_FMEUROAMERICA;
					break;
				case STE_FM_BAND_JAPAN:
					gfreq_range = FMD_FREQRANGE_FMJAPAN;
					break;
				case STE_FM_BAND_CHINA:
					gfreq_range = FMD_FREQRANGE_FMCHINA;
					break;
				}
			}

			if (STE_STATUS_OK == result) {
				/* Set the Band */
				FM_DEBUG_REPORT("ste_fm_switch_on: "\
				"Sending Set fmd_set_freq_range");
				result = fmd_set_freq_range(context,
						gfreq_range);
				if (FMD_RESULT_SUCCESS != result) {
					FM_ERR_REPORT("ste_fm_switch_on: "\
					"fmd_rx_set_grid failed %x",
					(unsigned int)result);
					result =  STE_STATUS_SYSTEM_ERROR;
					goto error;
				} else
					result =  STE_STATUS_OK;
			}

			if (STE_STATUS_OK == result) {
				/* Set the Frequency */
				FM_DEBUG_REPORT("ste_fm_switch_on: "\
					"Sending Set fmd_rx_set_frequency");
				result = fmd_rx_set_frequency(context,
					freq/1000);
				if (FMD_RESULT_WAIT != result) {
					FM_ERR_REPORT("ste_fm_switch_on: "\
					"fmd_rx_set_frequency failed %x",
					(unsigned int)result);
					result = STE_STATUS_SYSTEM_ERROR;
					goto error;
				} else {
					/* Wait till cmd is complete */
					os_get_cmd_sem();
					result =  STE_STATUS_OK;
				}
			}

			if (STE_STATUS_OK == result) {
				FM_DEBUG_REPORT("ste_fm_switch_on: "\
					"SetFrequency interrupt received, "\
					"Sending Set fmd_rx_set_stereo_mode");

				/* Set the Stereo mode */
				result =  fmd_rx_set_stereo_mode
					(context, FMD_STEREOMODE_STEREO);
				if (FMD_RESULT_SUCCESS != result) {
					FM_ERR_REPORT("ste_fm_switch_on: "\
						"fmd_rx_set_stereo_mode "\
						"failed %d",
						(unsigned int)result);
					result =  STE_STATUS_SYSTEM_ERROR;
					goto error;
				} else
					result =  STE_STATUS_OK;
			}

			if (STE_STATUS_OK == result) {
				/* Set the Up Sampling for Ext Audio Control */
				FM_DEBUG_REPORT("ste_fm_switch_on: "\
					"Sending Set fmd_ext_set_ctrl");
				result =  fmd_ext_set_ctrl(context, STE_TRUE);
				if (FMD_RESULT_SUCCESS != result) {
					FM_ERR_REPORT("ste_fm_switch_on: "\
						"fmd_ext_set_ctrl "\
						"failed %x",
						(unsigned int)result);
					result =  STE_STATUS_SYSTEM_ERROR;
					goto error;
				} else
					result =  STE_STATUS_OK;
			}

			if (STE_STATUS_OK == result) {
				/* Set the Digital Audio Ext Control */
				FM_DEBUG_REPORT("ste_fm_switch_on: "\
					"Sending Set fmd_ext_set_mode");
				result = fmd_ext_set_mode
					(context, FMD_OUTPUT_PARALLEL);
				if (FMD_RESULT_SUCCESS != result) {
					FM_ERR_REPORT("ste_fm_switch_on: "\
						"fmd_ext_set_mode "\
						"failed %x",
						(unsigned int)result);
					result =  STE_STATUS_SYSTEM_ERROR;
					goto error;
				} else
					result =  STE_STATUS_OK;
			}

			if (STE_STATUS_OK == result) {
				/* Set the Analog Out Volume to Max */
				uint8_t vol_in_percentage;
				fm_power_on = STE_TRUE;
				fm_stand_by = STE_FALSE;
				vol_index = 20;
				vol_in_percentage = (uint8_t) \
					(((uint16_t)(vol_index) * 100)/20);
				result = fmd_set_volume
					(context, vol_in_percentage);
				if (FMD_RESULT_SUCCESS != result) {
					FM_ERR_REPORT("ste_fm_switch_on: "\
						"FMRSetVolume failed %x", \
						(unsigned int)result);
					result =  STE_STATUS_SYSTEM_ERROR;
					goto error;
				} else
					result =  STE_STATUS_OK;
			}
		} else {
			FM_ERR_REPORT("ste_fm_switch_on: Already Switched on");
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_switch_on: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_switch_on: returning %s",
			str_stestatus(result));
	return result;
}

uint8_t ste_fm_switch_off(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_switch_off");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on) {
			/* Stop the RDS Thread if it is running */
			if (fm_rds_status) {
				fm_rds_status = STE_FALSE;
				os_stop_rds_thread();
			}
			result = fmd_goto_power_down
				(context);
			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_switch_off: "\
					"FMLGotoPowerdown failed = %d",
					(unsigned int)result);
				result =  STE_STATUS_SYSTEM_ERROR;
			}
			if (STE_STATUS_OK == result) {
				if (fmd_send_fm_ip_disable()) {
					FM_ERR_REPORT("ste_fm_switch_off: "\
					"Problem in fmd_send_fm_ip_disable");
					result = STE_STATUS_SYSTEM_ERROR;
				}
			}
			if (STE_STATUS_OK == result) {
				fm_power_on = STE_FALSE;
				fm_stand_by = STE_FALSE;
				head = 0;
				tail = 0;
				rds_group_sent = 0;
				rds_block_sent = 0;
				memset(ste_fm_rds_buf, 0,
					sizeof(struct ste_fm_rds_buf_s) *
					MAX_RDS_BUFFER *
					MAX_RDS_GROUPS_READ);
			}

		} else {
			FM_ERR_REPORT("ste_fm_switch_off: "\
				"Already Switched Off");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_switch_off: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_switch_off: returning %s",
			str_stestatus(result));
	return result;
}

uint8_t ste_fm_standby(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_standby");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_goto_standby(context);
			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_standby: "\
					"FMLGotoStandby failed, "\
					"err = %d", (unsigned int)result);
				result = STE_STATUS_SYSTEM_ERROR;
			}
			fm_stand_by = STE_TRUE;
		} else {
			FM_ERR_REPORT("ste_fm_standby: FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_standby: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_standby: returning %s", str_stestatus(result));
	return result;
}

uint8_t ste_fm_power_up_from_standby(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_power_up_from_standby");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_TRUE == fm_stand_by) {
			/* Power up FM */
			result = fmd_power_up(context);
			if (FMD_RESULT_WAIT != result) {
				FM_ERR_REPORT("ste_fm_power_up_from_standby: "\
				"fmd_power_up failed %x", (unsigned int)result);
				result =  STE_STATUS_SYSTEM_ERROR;
				goto error;
			} else {
				/* Wait till cmd is complete */
				os_get_cmd_sem();
				result =  STE_STATUS_OK;
			}
			fm_stand_by = STE_FALSE;
		} else {
			FM_ERR_REPORT("ste_fm_power_up_from_standby: "\
				"FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_power_up_from_standby: "\
				"FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

error:
	FM_DEBUG_REPORT("ste_fm_power_up_from_standby: returning %s",
		str_stestatus(result));
	return result;
}

uint8_t ste_fm_set_grid(
		uint8_t grid
		)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_set_grid: Grid = %d", grid);

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_rx_set_grid(context, grid);
			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_set_grid: "\
					"fmd_rx_set_grid failed");
				result =  STE_STATUS_SYSTEM_ERROR;
			}
		} else {
			FM_ERR_REPORT("ste_fm_set_grid: FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_grid: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_set_grid: returning %s", str_stestatus(result));
	return result;
}

uint8_t ste_fm_set_band(
		uint8_t band
		)
{
	uint8_t result = STE_STATUS_OK;
	uint8_t FreqRange = FMD_FREQRANGE_FMEUROAMERICA;

	FM_INFO_REPORT("ste_fm_set_band: Band = %d", band);

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			switch (band) {
			case STE_FM_BAND_US_EU:
			default:
				FreqRange = FMD_FREQRANGE_FMEUROAMERICA;
				break;
			case STE_FM_BAND_JAPAN:
				FreqRange = FMD_FREQRANGE_FMJAPAN;
				break;
			case STE_FM_BAND_CHINA:
				FreqRange = FMD_FREQRANGE_FMCHINA;
				break;
			}
			result = fmd_set_freq_range(context, FreqRange);
			if (FMD_RESULT_SUCCESS != result) {
					FM_ERR_REPORT("ste_fm_set_band: "\
					"fmd_set_freq_range failed %d", \
					(unsigned int)result);
				result =  STE_STATUS_SYSTEM_ERROR;
			}
		} else {
			FM_ERR_REPORT("ste_fm_set_band: FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_band: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_set_band: returning %s", str_stestatus(result));
	return result;
}

uint8_t ste_fm_step_up_freq(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_step_up_freq");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			if (fm_rds_status) {
				/* Stop RDS if it is active */
				result = ste_fm_rds_off();
				fm_prev_rds_status = STE_TRUE;
			}
			result = fmd_rx_step_frequency(context, STE_DIR_UP);
			if (result != FMD_RESULT_WAIT) {
				FM_ERR_REPORT("ste_fm_step_up_freq: "\
					"fmd_rx_step_frequency failed %x",
					(unsigned int)result);
				result = STE_STATUS_SYSTEM_ERROR;
			} else {
				/* Wait till cmd is complete */
				os_get_cmd_sem();
				head = 0;
				tail = 0;
				rds_group_sent = 0;
				rds_block_sent = 0;
				memset(ste_fm_rds_buf, 0,
					sizeof(struct ste_fm_rds_buf_s) *
					MAX_RDS_BUFFER *
					MAX_RDS_GROUPS_READ);
				result =  STE_STATUS_OK;
				if (fm_prev_rds_status) {
					/* Restart RDS if it was active
					 * earlier */
					result = ste_fm_rds_on();
					fm_prev_rds_status = STE_FALSE;
				}
			}
		} else {
			FM_ERR_REPORT("ste_fm_step_up_freq: "\
					"FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_step_up_freq: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_step_up_freq: returning %s",
		str_stestatus(result));
	return result;
}

uint8_t ste_fm_step_down_freq(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_step_down_freq");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			if (fm_rds_status) {
				/* Stop RDS if it is active */
				result = ste_fm_rds_off();
				fm_prev_rds_status = STE_TRUE;
			}
			result = fmd_rx_step_frequency(context, STE_DIR_DOWN);
			if (result != FMD_RESULT_WAIT) {
				FM_ERR_REPORT("ste_fm_step_down_freq: "\
					"fmd_rx_step_frequency failed %x",
					(unsigned int)result);
				result = STE_STATUS_SYSTEM_ERROR;
			} else {
				/* Wait till cmd is complete */
				os_get_cmd_sem();
				head = 0;
				tail = 0;
				rds_group_sent = 0;
				rds_block_sent = 0;
				memset(ste_fm_rds_buf, 0,
					sizeof(struct ste_fm_rds_buf_s) *
					MAX_RDS_BUFFER *
					MAX_RDS_GROUPS_READ);
				result =  STE_STATUS_OK;
				if (fm_prev_rds_status) {
					/* Restart RDS if it was active
					 * earlier */
					result = ste_fm_rds_on();
					fm_prev_rds_status = STE_FALSE;
				}
			}
		} else {
			FM_ERR_REPORT("ste_fm_step_down_freq: "\
				"FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_step_down_freq: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_step_down_freq: returning %s",
		str_stestatus(result));
	return result;
}

uint8_t ste_fm_search_up_freq(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_search_up_freq");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			if (fm_rds_status) {
				/* Stop RDS if it is active */
				result = ste_fm_rds_off();
				fm_prev_rds_status = STE_TRUE;
			}
			result = fmd_rx_seek(context, STE_DIR_UP);
			if (FMD_RESULT_ONGOING != result) {
				FM_ERR_REPORT("ste_fm_search_up_freq: "\
					"Error Code %d", \
					(unsigned int)result);
				result = STE_STATUS_SYSTEM_ERROR;
			} else {
				head = 0;
				tail = 0;
				rds_group_sent = 0;
				rds_block_sent = 0;
				memset(ste_fm_rds_buf, 0,
					sizeof(struct ste_fm_rds_buf_s) *
					MAX_RDS_BUFFER *
					MAX_RDS_GROUPS_READ);
				result =  STE_STATUS_OK;
			}
		} else {
			FM_ERR_REPORT("ste_fm_search_up_freq: FM not "\
					"switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_search_up_freq: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_search_up_freq: returning %s",
		str_stestatus(result));
	return result;
}

uint8_t ste_fm_search_down_freq(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_search_down_freq");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			if (fm_rds_status) {
				/* Stop RDS if it is active */
				result = ste_fm_rds_off();
				fm_prev_rds_status = STE_TRUE;
			}
			result = fmd_rx_seek(context, STE_DIR_DOWN);
			if (FMD_RESULT_ONGOING != result) {
				FM_ERR_REPORT("ste_fm_search_down_freq: "\
						"Error Code %d", \
						(unsigned int)result);
				result = STE_STATUS_SYSTEM_ERROR;
			} else {
				head = 0;
				tail = 0;
				rds_group_sent = 0;
				rds_block_sent = 0;
				memset(ste_fm_rds_buf, 0,
					sizeof(struct ste_fm_rds_buf_s) *
					MAX_RDS_BUFFER *
					MAX_RDS_GROUPS_READ);
				result =  STE_STATUS_OK;
			}
		} else {
			FM_ERR_REPORT("ste_fm_search_down_freq: "\
						"FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_search_down_freq: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_search_down_freq: returning %s",
		str_stestatus(result));
	return result;
}

uint8_t ste_fm_start_band_scan(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_start_band_scan");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			if (fm_rds_status) {
				/* Stop RDS if it is active */
				result = ste_fm_rds_off();
				fm_prev_rds_status = STE_TRUE;
			}
			result = fmd_rx_scan_band(context);
			if (FMD_RESULT_ONGOING != result) {
				FM_ERR_REPORT("ste_fm_start_band_scan: "\
					"Error Code %d", \
					(unsigned int)result);
				result = STE_STATUS_SYSTEM_ERROR;
			} else {
				head = 0;
				tail = 0;
				rds_group_sent = 0;
				rds_block_sent = 0;
				memset(ste_fm_rds_buf, 0,
					sizeof(struct ste_fm_rds_buf_s) *
					MAX_RDS_BUFFER *
					MAX_RDS_GROUPS_READ);
				result =  STE_STATUS_OK;
			}
		} else {
			FM_ERR_REPORT("ste_fm_start_band_scan: "\
				"FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_start_band_scan: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_start_band_scan: returning %s",
			str_stestatus(result));
	return result;
}

uint8_t ste_fm_stop_scan(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_stop_scan");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_rx_stop_seeking(context);
			if (FMD_RESULT_WAIT != result) {
				FM_ERR_REPORT("ste_fm_stop_scan: "\
					"Error Code %d", (unsigned int)result);
				result = STE_STATUS_SYSTEM_ERROR;
			} else {
				/* Wait till cmd is complete */
				os_get_cmd_sem();
				head = 0;
				tail = 0;
				rds_group_sent = 0;
				rds_block_sent = 0;
				memset(ste_fm_rds_buf, 0,
					sizeof(struct ste_fm_rds_buf_s) *
					MAX_RDS_BUFFER *
					MAX_RDS_GROUPS_READ);
				result =  STE_STATUS_OK;
				if (fm_prev_rds_status) {
					/* Restart RDS if it was
					 * active earlier */
					ste_fm_rds_on();
					fm_prev_rds_status = STE_FALSE;
				}
			}
		} else {
			FM_ERR_REPORT("ste_fm_stop_scan: FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_stop_scan: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_stop_scan: returning %s",
			str_stestatus(result));
	return result;
}

uint8_t ste_fm_get_scan_result(
		uint16_t *no_of_scanfreq,
		uint32_t *scanfreq,
		uint32_t *scanfreq_rssi_level
		)
{
	uint8_t result = STE_STATUS_OK;
	uint16_t cnt;
	uint16_t index;
	uint32_t minfreq, maxfreq, freq_interval;
	uint16_t num_of_channels_found = 0;
	uint16_t channels[3];
	uint16_t rssi[3];

	FM_INFO_REPORT("ste_fm_get_scan_result");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_get_freq_range_properties(context,
				gfreq_range,
				&minfreq,
				&maxfreq,
				&freq_interval);

			cnt = 11;
			while ((cnt--) && (result == FMD_RESULT_SUCCESS)) {
				/* Get all channels, including empty ones */
				result = fmd_rx_get_scan_band_info(
					context, cnt * 3, \
					&num_of_channels_found, channels, rssi);
				if (result == FMD_RESULT_SUCCESS) {
					index = cnt * 3;
					scanfreq[index] = (minfreq +
						channels[0] * 50) * 1000;
					scanfreq_rssi_level[index] = rssi[0];
					scanfreq[index + 1] = (minfreq +
						channels[1] * 50) * 1000;
					scanfreq_rssi_level[index + 1] =
						rssi[1];
					scanfreq[index + 2] = (minfreq +
						channels[2] * 50) * 1000;
					scanfreq_rssi_level[index + 2] =
						rssi[2];

					/* store total nr of channels
					 * (only once) */
					if (*no_of_scanfreq  == 0 &&
					    num_of_channels_found > 0) {
						*no_of_scanfreq =
						num_of_channels_found;
						global_event =
						STE_EVENT_SCAN_CHANNELS_FOUND;
					} else if (*no_of_scanfreq  == 0
						&& num_of_channels_found == 0) {
						global_event =
						STE_EVENT_NO_CHANNELS_FOUND;
					}
				}
			}
			if (fm_prev_rds_status) {
				/* Restart RDS if it was active earlier */
				result = ste_fm_rds_on();
				fm_prev_rds_status = STE_FALSE;
			}
		} else {
			FM_ERR_REPORT("ste_fm_get_scan_result: "\
				"FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_get_scan_result: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_get_scan_result: returning %s",
		str_stestatus(result));
	return result;

}

uint8_t ste_fm_increase_volume(void)
{
	uint8_t result = STE_STATUS_OK;
	uint8_t vol_in_percentage;

	FM_INFO_REPORT("ste_fm_increase_volume");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			if (vol_index + 1 < 20) {
				vol_index++;
				vol_in_percentage =
				(uint8_t)(((uint16_t)(vol_index) * 100)/20);
				result =
				fmd_set_volume(context, vol_in_percentage);
				if (FMD_RESULT_SUCCESS != result) {
						FM_DEBUG_REPORT(""\
						"ste_fm_increase_volume: "\
						"FMRSetVolume failed = %d",
						(unsigned int)result);
					result =  STE_STATUS_SYSTEM_ERROR;
				}
			} else {
				result = STE_STATUS_OK;
				FM_DEBUG_REPORT("ste_fm_increase_volume: "\
				    "Already at max Volume = %d", vol_index);
			}
			if (result != STE_STATUS_OK) {
				/* Reset the volume flag to original value,
				 * since API Failed */
				vol_index--;
			}
		} else {
			FM_ERR_REPORT("ste_fm_increase_volume: "\
						"FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_increase_volume: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_increase_volume: returning %s",
		str_stestatus(result));
	return result;
}

uint8_t ste_fm_deccrease_volume(void)
{
	uint8_t result = STE_STATUS_OK;
	uint8_t vol_in_percentage;

	FM_INFO_REPORT("ste_fm_deccrease_volume");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			if (vol_index - 1 > 0) {
				vol_index--;
				vol_in_percentage =
				(uint8_t) (((uint16_t)(vol_index) * 100)/20);
				result =
				fmd_set_volume(context, vol_in_percentage);
				if (FMD_RESULT_SUCCESS != result) {
					FM_ERR_REPORT(
					"ste_fm_increase_volume: "\
					"FMRSetVolume failed = %d", \
					(unsigned int) result);
					result =  STE_STATUS_SYSTEM_ERROR;
				}
			} else {
				result = STE_STATUS_OK;
				FM_ERR_REPORT("ste_fm_deccrease_volume: "\
					"Already at min Volume = %d",
					vol_index);
			}
			if (result != STE_STATUS_OK) {
				/* Reset the volume flag to original value,
				 * since API Failed */
				vol_index++;
			}
		} else {
			FM_ERR_REPORT("ste_fm_deccrease_volume: "\
						"FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_deccrease_volume: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_deccrease_volume: returning %s",
		str_stestatus(result));
	return result;
}

uint8_t ste_fm_set_audio_balance(
		int8_t balance
		)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_set_audio_balance, balance = %d", balance);

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_set_balance(context, balance);
			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("FMRSetAudioBalance : "\
					"Failed in fmd_set_balance, err = %d",
					(unsigned int)result);
				result =  STE_STATUS_SYSTEM_ERROR;
			}
		} else {
			FM_ERR_REPORT("ste_fm_set_audio_balance: "\
					"FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_audio_balance: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_set_audio_balance: returning %s",
		str_stestatus(result));
	return result;
}

uint8_t ste_fm_set_volume(
		uint8_t vol_level
		)
{
	uint8_t result = STE_STATUS_OK;
	uint8_t vol_in_percentage;

	FM_INFO_REPORT("ste_fm_set_volume: Volume Level = %d", vol_level);

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			vol_in_percentage = \
			(uint8_t) (((uint16_t)(vol_index) * 100)/20);
			result = fmd_set_volume(context, vol_in_percentage);
			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_increase_volume: "\
					"FMRSetVolume failed, err = %d",
					(unsigned int)result);
				result =  STE_STATUS_SYSTEM_ERROR;
			}
			if (STE_STATUS_OK == result)
				vol_index = vol_level;
		} else {
			FM_ERR_REPORT("ste_fm_set_volume: FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_volume: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_set_volume: returning %s",
			str_stestatus(result));
	return result;
}

uint8_t ste_fm_get_volume(
		uint8_t *vol_level
		)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_get_volume");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by)
			*vol_level = vol_index;
		else {
			FM_ERR_REPORT("ste_fm_set_volume: FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
			*vol_level = 0;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_volume: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
		*vol_level = 0;
	}

	FM_DEBUG_REPORT("ste_fm_get_volume: returning %s, VolLevel = %d",
		str_stestatus(result), *vol_level);
	return result;
}

uint8_t ste_fm_set_audio_dac(
		uint8_t state
		)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_set_audio_dac");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_set_audio_dac(context, state);
			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_set_audio_dac: "\
					"FMRSetAudioDAC failed %d", \
					(unsigned int)result);
				result =  STE_STATUS_SYSTEM_ERROR;
			}
		} else {
			FM_ERR_REPORT("ste_fm_set_audio_dac: "\
					"FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_audio_dac: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_set_audio_dac: "\
		"returning %s", str_stestatus(result));
	return result;
}

uint8_t ste_fm_rds_off(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_rds_off");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_rx_set_ctrl(context, FMD_SWITCH_OFF_RDS);
			if (FMD_RESULT_SUCCESS == result) {
				/* Stop the RDS Thread */
				fm_rds_status = STE_FALSE;
				head = 0;
				tail = 0;
				rds_group_sent = 0;
				rds_block_sent = 0;
				memset(ste_fm_rds_buf, 0,
					sizeof(struct ste_fm_rds_buf_s) *
					MAX_RDS_BUFFER *
					MAX_RDS_GROUPS_READ);
				FM_DEBUG_REPORT("ste_fm_rds_off: "\
					"Stopping RDS Thread");
				os_stop_rds_thread();
			}
		} else {
			FM_ERR_REPORT("ste_fm_rds_off: FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_rds_off: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_rds_off: returning %s", str_stestatus(result));
	return result;
}

uint8_t ste_fm_rds_on(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_rds_on");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			FM_DEBUG_REPORT("ste_fm_rds_on:"\
					" Sending fmd_rx_buffer_set_size");
			result = fmd_rx_buffer_set_size(context,
				MAX_RDS_GROUPS_READ);
			if (FMD_RESULT_SUCCESS == result) {
				FM_DEBUG_REPORT("ste_fm_rds_on: "\
					"Sending fmd_rx_buffer_set_threshold");
				result = fmd_rx_buffer_set_threshold(context,
					MAX_RDS_GROUPS_READ - 1);
			}
			if (FMD_RESULT_SUCCESS == result) {
				FM_DEBUG_REPORT("ste_fm_rds_on: "\
					"Sending fmd_rx_set_ctrl");
				result = fmd_rx_set_ctrl(context,
					FMD_SWITCH_ON_RDS_ENHANCED_MODE);
			}
			if (FMD_RESULT_SUCCESS == result) {
				/* Start the RDS Thread to
				 * read the RDS Buffers */
				fm_rds_status = STE_TRUE;
				head = 0;
				tail = 0;
				rds_group_sent = 0;
				rds_block_sent = 0;
				memset(ste_fm_rds_buf, 0,
					sizeof(struct ste_fm_rds_buf_s) *
					MAX_RDS_BUFFER *
					MAX_RDS_GROUPS_READ);
				os_start_rds_thread(ste_fm_rds_callback);
			}
		} else {
			FM_ERR_REPORT("ste_fm_rds_on: FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_rds_on: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_rds_on: returning %s", str_stestatus(result));
	return result;
}

uint8_t ste_fm_get_rds_status(
		bool *enabled
		)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_get_rds_status");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_rx_get_rds(context, (bool *)enabled);
			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_get_rds_status: "\
					"fmd_rx_get_rds failed %d", \
					(unsigned int)result);
				result =  STE_STATUS_SYSTEM_ERROR;
			}
		} else {
			FM_ERR_REPORT("ste_fm_get_rds_status: "\
			"FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
			*enabled = STE_FALSE;
		}
	} else {
		FM_ERR_REPORT("ste_fm_get_rds_status: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
		*enabled = STE_FALSE;
	}

	FM_DEBUG_REPORT("ste_fm_get_rds_status: returning %s, RDS Status = %d",
		str_stestatus(result), *enabled);
	return result;
}

uint8_t ste_fm_mute(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_mute");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			/* Mute Analog DAC */
			result = fmd_set_mute(context, STE_TRUE);
			if (FMD_RESULT_WAIT != result) {
				FM_ERR_REPORT("ste_fm_mute: "\
					"fmd_set_mute failed %d", \
					(unsigned int)result);
				result =  STE_STATUS_SYSTEM_ERROR;
			} else {
				/* Wait till cmd is complete */
				os_get_cmd_sem();
				/* Mute Ext Src */
				result = fmd_ext_set_mute(context, STE_TRUE);
				result = STE_STATUS_OK;
				/* if (FMD_RESULT_SUCCESS != result)
					result = STE_STATUS_SYSTEM_ERROR; */
			}
		} else {
			FM_ERR_REPORT("ste_fm_mute: FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_mute: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_mute: returning %s",
		str_stestatus(result));
	return result;
}

uint8_t ste_fm_unmute(void)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_unmute");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			/* Unmute Analog DAC */
			result = fmd_set_mute(context, STE_FALSE);
			if (FMD_RESULT_WAIT != result) {
				FM_ERR_REPORT("ste_fm_mute: "\
					"fmd_set_mute failed %d", \
					(unsigned int)result);
				result =  STE_STATUS_SYSTEM_ERROR;
			} else {
				/* Wait till cmd is complete */
				os_get_cmd_sem();
				/* Unmute Ext Src */
				result = fmd_ext_set_mute(context, STE_FALSE);
				result = STE_STATUS_OK;
				/* if (FMD_RESULT_SUCCESS != result)
					result = STE_STATUS_SYSTEM_ERROR; */
			}
		} else {
			FM_ERR_REPORT("ste_fm_unmute: FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_unmute: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_unmute: returning %s", str_stestatus(result));
	return result;
}

uint8_t ste_fm_get_frequency(
		uint32_t *freq
		)
{
	uint8_t result = STE_STATUS_OK;
	uint32_t currentFreq = 0;

	FM_INFO_REPORT("ste_fm_get_frequency");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_rx_get_frequency(context,
				(uint32_t *)&currentFreq);
			if (FMD_RESULT_SUCCESS != result) {
				result = STE_STATUS_SYSTEM_ERROR;
				FM_ERR_REPORT("ste_fm_get_frequency: "\
					"fmd_rx_get_frequency failed %d", \
					(unsigned int)result);
			}
			if (result == STE_STATUS_OK) {
				/* Convert To Hz */
				*freq = currentFreq * 1000;
				FM_DEBUG_REPORT("ste_fm_get_frequency: "\
				"Current Frequency = %d Hz", *freq);
				if (fm_prev_rds_status) {
					/* Restart RDS if it was
					 * active earlier */
					ste_fm_rds_on();
					fm_prev_rds_status = STE_FALSE;
				}
			} else {
				*freq = 0;
			}
		} else {
			FM_ERR_REPORT("ste_fm_get_frequency: "\
				"FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
			*freq = 0;
		}
	} else {
		FM_ERR_REPORT("ste_fm_get_frequency: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
		*freq = 0;
	}

	FM_DEBUG_REPORT("ste_fm_get_frequency: "\
		"returning %s", str_stestatus(result));
	return result;
}

uint8_t ste_fm_set_frequency(
		uint32_t new_freq
		)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_set_frequency, new_freq = %d", (int)new_freq);

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_rx_set_frequency(context, new_freq/1000);
			if (result != FMD_RESULT_WAIT) {
				FM_ERR_REPORT("ste_fm_set_frequency: "\
					"fmd_rx_set_frequency failed %x",
					(unsigned int)result);
				result = STE_STATUS_SYSTEM_ERROR;
			} else {
				/* Wait till cmd is complete */
				os_get_cmd_sem();
				head = 0;
				tail = 0;
				rds_group_sent = 0;
				rds_block_sent = 0;
				memset(ste_fm_rds_buf, 0,
					sizeof(struct ste_fm_rds_buf_s) *
					MAX_RDS_BUFFER *
					MAX_RDS_GROUPS_READ);
				result =  STE_STATUS_OK;
			}
		} else {
			FM_ERR_REPORT("ste_fm_set_frequency: "\
				      "FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_frequency: "\
			"FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_set_frequency: "\
		"returning %s", str_stestatus(result));
	return result;
}

uint8_t ste_fm_get_signal_strength(
		uint16_t *signal_strength
		)
{
	uint8_t result = STE_STATUS_OK;
	uint32_t sigStrength = 0;

	FM_INFO_REPORT("ste_fm_get_signal_strength");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_rx_get_signal_strength(context,
					&sigStrength);
			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_get_signal_strength: "\
				"Error Code %d", (unsigned int)result);
				*signal_strength = 0;
				return STE_STATUS_SYSTEM_ERROR;
			} else
				*signal_strength = sigStrength;
		} else {
			FM_ERR_REPORT("ste_fm_get_signal_strength: "\
				      "FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
			*signal_strength = 0;
		}
	} else {
		FM_ERR_REPORT("ste_fm_get_signal_strength: "\
			"FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
		*signal_strength = 0;
	}

	FM_DEBUG_REPORT("ste_fm_get_signal_strength: returning %s",
		str_stestatus(result));
	return result;
}

uint8_t ste_fm_get_mode(
		uint8_t *cur_mode
		)
{
	uint8_t result = STE_STATUS_OK;
	uint8_t mode;

	FM_INFO_REPORT("ste_fm_get_mode");

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_rx_get_stereo_mode(context,
				(uint32_t *)&mode);
			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_get_mode: "\
				"fmd_rx_get_stereo_mode failed, Error Code %d",
				(unsigned int)result);
				result =  STE_STATUS_SYSTEM_ERROR;
			}
			if (result == STE_STATUS_OK)
				*cur_mode = mode;
			else
				*cur_mode = 0;
		} else {
			FM_ERR_REPORT("ste_fm_get_mode: FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
			*cur_mode = 0;
		}
	} else {
		FM_ERR_REPORT("ste_fm_get_mode: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
		*cur_mode = 0;
	}

	FM_DEBUG_REPORT("ste_fm_get_mode: returning %s, mode = %d",
		str_stestatus(result), *cur_mode);
	return result;
}

uint8_t ste_fm_set_mode(
		uint8_t mode
		)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_set_mode: mode = %d", mode);

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_rx_set_stereo_mode(context, mode);
			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_set_mode: "\
				"fmd_rx_set_stereo_mode failed, Error Code %d",
				(unsigned int)result);
				result =  STE_STATUS_SYSTEM_ERROR;
			}
		} else {
			FM_ERR_REPORT("ste_fm_set_mode: FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_mode: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_set_mode: "\
		"returning %s", str_stestatus(result));
	return result;
}

uint8_t ste_fm_select_antenna(
		uint8_t antenna
		)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_SetAntenna: Antenna = %d", antenna);

	if (STE_TRUE == fm_init) {
		if (STE_TRUE == fm_power_on && STE_FALSE == fm_stand_by) {
			result = fmd_set_antenna(context, antenna);
			if (FMD_RESULT_WAIT != result) {
				FM_ERR_REPORT("ste_fm_select_antenna: "\
				"fmd_set_antenna failed, Error Code %d",
				(unsigned int)result);
				result =  STE_STATUS_SYSTEM_ERROR;
			} else {
				/* Wait till cmd is complete */
				os_get_cmd_sem();
				result =  STE_STATUS_OK;
			}
		} else {
			FM_ERR_REPORT("ste_fm_SetAntenna: FM not switched on");
			result = STE_STATUS_SYSTEM_ERROR;
		}
	} else {
		FM_ERR_REPORT("ste_fm_SetAntenna: FM not initialized");
		result = STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_SetAntenna: "\
		"returning %s", str_stestatus(result));
	return result;
}

uint8_t ste_fm_get_rssi_threshold(
		uint16_t *rssi_thresold
		)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_get_rssi_threshold");

	result = fmd_rx_get_stop_level(context, rssi_thresold);
	if (FMD_RESULT_SUCCESS != result) {
		FM_ERR_REPORT("ste_fm_get_rssi_threshold: "\
		"fmd_rx_get_stop_level failed, Error Code %d",
		(unsigned int)result);
		result =  STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_get_rssi_threshold: returning %s",
		str_stestatus(result));
	return result;
}

uint8_t ste_fm_set_rssi_threshold(
		uint16_t rssi_thresold
		)
{
	uint8_t result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_set_rssi_threshold: "\
		"RssiThresold = %d", rssi_thresold);

	result = fmd_rx_set_stop_level(context, rssi_thresold);
	if (FMD_RESULT_SUCCESS != result) {
		FM_ERR_REPORT("ste_fm_set_rssi_threshold: "\
		"fmd_rx_set_stop_level failed, Error Code %d",
		(unsigned int)result);
		result =  STE_STATUS_SYSTEM_ERROR;
	}

	FM_DEBUG_REPORT("ste_fm_set_rssi_threshold: returning %s",
		str_stestatus(result));
	return result;
}

MODULE_AUTHOR("Hemant Gupta");
MODULE_LICENSE("GPL v2");

