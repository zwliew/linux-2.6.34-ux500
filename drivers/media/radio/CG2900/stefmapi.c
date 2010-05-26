/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Linux FM Host API's for ST-Ericsson FM Chip.
 *
 * Author: Hemant Gupta/hemant.gupta@stericsson.com for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2
 */

#include <linux/slab.h>
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
/* RDS Tx PTY set to Other music */
#define OTHER_MUSIC				15

static bool fm_rds_status;
static bool fm_prev_rds_status;
static void *context;
/* hci revision and lmp sub version is hardcoded for the moment
 * as get version cmd does not work before firmware download */
static u32 hci_revision = 0x0700;
static u32 lmp_sub_version = 0x0011;
static u16 program_identification_code;
static u16 default_program_identification_code = 0x1234;
static u16 program_type_code;
static u16 default_program_type_code = OTHER_MUSIC;
static char program_service[MAX_PSN_SIZE];
static char default_program_service[MAX_PSN_SIZE] = "FM-Xmit ";
static char radio_text[MAX_RT_SIZE];
static char default_radio_text[MAX_RT_SIZE] = "Default Radio Text "
    "Default Radio Text Default Radio Text Default";
static bool a_b_flag = STE_FALSE;
static struct mutex rds_mutex;
struct ste_fm_rds_buf_t ste_fm_rds_buf[MAX_RDS_BUFFER][MAX_RDS_GROUPS];
struct ste_fm_rds_info_t ste_fm_rds_info;
static enum ste_fm_state_t ste_fm_state;
static enum ste_fm_mode_t ste_fm_mode;
u8 global_event;

/* ------------------ Internal function declarations ---------------------- */
static char *ste_fm_get_one_line_of_text(char *wr_buffer,
					 int max_nbr_of_bytes,
					 char *rd_buffer, int *bytes_copied);
static bool ste_fm_get_file_to_load(const struct firmware *fw,
				    char **file_name);
static u8 ste_fm_load_firmware(struct device *device);
static u8 ste_fm_transmit_rds_groups(void);
static void ste_fm_driver_callback(void *context,
				   u8 event, bool event_successful);
static void ste_fm_rds_callback(void);
static const char *str_stestatus(u8 status);

/* ------------------ Internal function definitions ---------------------- */
/**
 * ste_fm_get_one_line_of_text()- Get One line of text from a file.
 * Replacement function for stdio function fgets.This function extracts one
 * line of text from input file.
 * @wr_buffer: Buffer to copy text to.
 * @max_nbr_of_bytes: Max number of bytes to read, i.e. size of rd_buffer.
 * @rd_buffer: Data to parse.
 * @bytes_copied: Number of bytes copied to wr_buffer.
 *
 * Returns:
 *   Pointer to next data to read.
 */
static char *ste_fm_get_one_line_of_text(char *wr_buffer,
					 int max_nbr_of_bytes,
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
static bool ste_fm_get_file_to_load(const struct firmware *fw, char **file_name)
{
	char *line_buffer;
	char *curr_file_buffer;
	int bytes_left_to_parse = fw->size;
	int bytes_read = 0;
	bool file_found = false;

	curr_file_buffer = (char *)&(fw->data[0]);

	line_buffer = os_mem_alloc(STE_FM_LINE_BUFFER_LENGTH);

	if (line_buffer) {
		while (!file_found) {
			/* Get one line of text from the file to parse */
			curr_file_buffer =
			    ste_fm_get_one_line_of_text(line_buffer,
						min
						(STE_FM_LINE_BUFFER_LENGTH,
						 (int)(fw->size -
							bytes_read)),
							curr_file_buffer,
							&bytes_read);

			bytes_left_to_parse -= bytes_read;
			if (bytes_left_to_parse <= 0) {
				/* End of file => Leave while loop */
				FM_ERR_REPORT("Reached end of file."
					      "No file found!");
				break;
			}

			/* Check if the line of text is a comment
			 * or not, comments begin with '#' */
			if (*line_buffer != '#') {
				u32 hci_rev = 0;
				u32 lmp_sub = 0;

				FM_DEBUG_REPORT("Found a valid line <%s>",
						line_buffer);

				/* Check if we can find the correct
				 * HCI revision and LMP subversion
				 * as well as a file name in the text line
				 * Store the filename if the actual file can
				 * be found in the file system
				 */
				if (sscanf(line_buffer, "%x%x%s",
					   (unsigned int *)&hci_rev,
					   (unsigned int *)&lmp_sub,
					   *file_name) == 3
				    && hci_rev == hci_revision
				    && lmp_sub == lmp_sub_version) {
					FM_INFO_REPORT("File name = %s "
						       "HCI Revision"
						       "= 0x%X LMP PAL"
						       "Subversion = 0x%X",
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
					       STE_FM_FILENAME_MAX);
				}
			}
		}
		os_mem_free(line_buffer);
	} else {
		FM_ERR_REPORT("Failed to allocate:"
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
 *   STE_STATUS_OK, if firmware download is successful
 *   STE_STATUS_SYSTEM_ERROR, otherwise.
*/
static u8 ste_fm_load_firmware(struct device *device)
{
	int err = 0;
	bool file_found;
	u8 result = STE_STATUS_OK;
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
		FM_ERR_REPORT("ste_fm_load_firmware: "
			      "Couldn't get bt_src_coeff info file");
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	/* Now we have the bt_src_coeff info file.
	 * See if we can find the right bt_src_coeff file as well */
	bt_src_coeff_file_name = os_mem_alloc(STE_FM_FILENAME_MAX);
	if (bt_src_coeff_file_name != NULL) {
		file_found = ste_fm_get_file_to_load(bt_src_coeff_info,
						     &bt_src_coeff_file_name);

		/* Now we are finished with the bt_src_coeff info file */
		release_firmware(bt_src_coeff_info);

		if (!file_found) {
			FM_ERR_REPORT("ste_fm_load_firmware: "
				      "Couldn't find bt_src_coeff file!! "
				      "Major error!!!");
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_load_firmware: "
			      "Couldn't allocate memory for "
			      "bt_src_coeff_file_name");
		release_firmware(bt_src_coeff_info);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	/* Open ext_src_coeff info file. */
	err = request_firmware(&ext_src_coeff_info,
			       STE_FM_EXT_SRC_COEFF_INFO_FILE, device);
	if (err) {
		FM_ERR_REPORT("ste_fm_load_firmware: "
			      "Couldn't get ext_src_coeff_info info file");
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	/* Now we have the ext_src_coeff info file. See if we can
	 * find the right ext_src_coeff file as well */
	ext_src_coeff_file_name = os_mem_alloc(STE_FM_FILENAME_MAX);
	if (ext_src_coeff_file_name != NULL) {
		file_found = ste_fm_get_file_to_load(ext_src_coeff_info,
						     &ext_src_coeff_file_name);

		/* Now we are finished with the ext_src_coeff info file */
		release_firmware(ext_src_coeff_info);

		if (!file_found) {
			FM_ERR_REPORT("ste_fm_load_firmware: "
				      "Couldn't find ext_src_coeff_info "
				      "file!!! Major error!");
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_load_firmware: "
			      "Couldn't allocate memory for "
			      "ext_src_coeff_file_name");
		release_firmware(ext_src_coeff_info);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	/* Open fm_coeff info file. */
	err = request_firmware(&fm_coeff_info,
			       STE_FM_FM_COEFF_INFO_FILE, device);
	if (err) {
		FM_ERR_REPORT("ste_fm_load_firmware: "
			      "Couldn't get fm_coeff info file");
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	/* Now we have the fm_coeff_info info file.
	 * See if we can find the right fm_coeff_info file as well */
	fm_coeff_file_name = os_mem_alloc(STE_FM_FILENAME_MAX);
	if (fm_coeff_file_name != NULL) {
		file_found = ste_fm_get_file_to_load(fm_coeff_info,
						     &fm_coeff_file_name);

		/* Now we are finished with the fm_coeff info file */
		release_firmware(fm_coeff_info);

		if (!file_found) {
			FM_ERR_REPORT("ste_fm_load_firmware: "
				      "Couldn't find fm_coeff file!!! "
				      "Major error!");
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_load_firmware: "
			      "Couldn't allocate memory for "
			      "fm_coeff_file_name");
		release_firmware(fm_coeff_info);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	/* Open fm_prog info file. */
	err = request_firmware(&fm_prog_info, STE_FM_FM_PROG_INFO_FILE, device);
	if (err) {
		FM_ERR_REPORT("ste_fm_load_firmware: "
			      "Couldn't get fm_prog_info info file");
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	/* Now we have the fm_prog info file.
	 * See if we can find the right fm_prog file as well
	 */
	fm_prog_file_name = os_mem_alloc(STE_FM_FILENAME_MAX);
	if (fm_prog_file_name != NULL) {
		file_found = ste_fm_get_file_to_load(fm_prog_info,
						     &fm_prog_file_name);

		/* Now we are finished with fm_prog patch info file */
		release_firmware(fm_prog_info);

		if (!file_found) {
			FM_ERR_REPORT("ste_fm_load_firmware: "
				      "Couldn't find fm_prog_info file!!! "
				      "Major error!");
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_load_firmware: "
			      "Couldn't allocate memory for "
			      "fm_prog_file_name");
		release_firmware(fm_prog_info);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	/* OK. Now it is time to download the firmware */
	err = request_firmware(&bt_src_coeff_info,
			       bt_src_coeff_file_name, device);
	if (err < 0) {
		FM_ERR_REPORT("ste_fm_load_firmware: "
			      "Couldn't get bt_src_coeff file, err = %d", err);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	FM_INFO_REPORT("ste_fm_load_firmware: Downloading %s of %d bytes",
		       bt_src_coeff_file_name, bt_src_coeff_info->size);
	if (fmd_send_fm_firmware((u8 *) bt_src_coeff_info->data,
				 bt_src_coeff_info->size)) {
		FM_ERR_REPORT("ste_fm_load_firmware: Error in downloading %s",
			      bt_src_coeff_file_name);
		release_firmware(bt_src_coeff_info);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	/* Now we are finished with the bt_src_coeff info file */
	release_firmware(bt_src_coeff_info);
	err = request_firmware(&ext_src_coeff_info,
			       ext_src_coeff_file_name, device);
	if (err < 0) {
		FM_ERR_REPORT("ste_fm_load_firmware: "
			      "Couldn't get ext_src_coeff file, err = %d", err);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	FM_INFO_REPORT("ste_fm_load_firmware: Downloading %s of %d bytes",
		       ext_src_coeff_file_name, ext_src_coeff_info->size);
	if (fmd_send_fm_firmware((u8 *) ext_src_coeff_info->data,
				 ext_src_coeff_info->size)) {
		FM_ERR_REPORT("ste_fm_load_firmware: Error in downloading %s",
			      ext_src_coeff_file_name);
		release_firmware(ext_src_coeff_info);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	/* Now we are finished with the bt_src_coeff info file */
	release_firmware(ext_src_coeff_info);

	err = request_firmware(&fm_coeff_info, fm_coeff_file_name, device);
	if (err < 0) {
		FM_ERR_REPORT("ste_fm_load_firmware: "
			      "Couldn't get fm_coeff file, err = %d", err);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	FM_INFO_REPORT("ste_fm_load_firmware: Downloading %s of %d bytes",
		       fm_coeff_file_name, fm_coeff_info->size);
	if (fmd_send_fm_firmware((u8 *) fm_coeff_info->data,
				 fm_coeff_info->size)) {
		FM_ERR_REPORT("ste_fm_load_firmware: Error in downloading %s",
			      fm_coeff_file_name);
		release_firmware(fm_coeff_info);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	/* Now we are finished with the bt_src_coeff info file */
	release_firmware(fm_coeff_info);

	err = request_firmware(&fm_prog_info, fm_prog_file_name, device);
	if (err < 0) {
		FM_ERR_REPORT("ste_fm_load_firmware: "
			      "Couldn't get fm_prog file, err = %d", err);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	FM_INFO_REPORT("ste_fm_load_firmware: Downloading %s of %d bytes",
		       fm_prog_file_name, fm_prog_info->size);
	if (fmd_send_fm_firmware((u8 *) fm_prog_info->data,
		fm_prog_info->size)) {
		FM_ERR_REPORT("ste_fm_load_firmware: Error in downloading %s",
			      fm_prog_file_name);
		release_firmware(fm_prog_info);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

	/* Now we are finished with the bt_src_coeff info file */
	release_firmware(fm_prog_info);

error:
	/* Free Allocated memory, NULL checking done inside resp functions */
	os_mem_free(bt_src_coeff_file_name);
	os_mem_free(ext_src_coeff_file_name);
	os_mem_free(fm_coeff_file_name);
	os_mem_free(fm_prog_file_name);
	FM_DEBUG_REPORT("-ste_fm_load_firmware: returning %s",
			str_stestatus(result));
	return result;
}

/**
 * ste_fm_transmit_rds_groups()- Transmits the RDS Groups.
 * Stores the RDS Groups in Chip's buffer and each group is
 * transmitted every 87.6 ms.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
static u8 ste_fm_transmit_rds_groups(void)
{
	u8 result = STE_STATUS_OK;
	u16 group_position = 0;
	u8 block1[2];
	u8 block2[2];
	u8 block3[2];
	u8 block4[2];
	int index1 = 0;
	int index2 = 0;
	int group_0B_count = 0;
	int group_2A_count = 0;

	FM_INFO_REPORT("ste_fm_transmit_rds_groups");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		while (group_position < 20 && result == STE_STATUS_OK) {
			if (group_position < 4) {
				/* Transmit PSN in Group 0B */
				block1[0] = program_identification_code;
				block1[1] = program_identification_code >> 8;
				/* M/S bit set to Music */
				if (group_0B_count % 4 == 0) {
					/* Manipulate DI bit */
					block2[0] =
					    (0x08 | ((program_type_code & 0x07)
						     << 5))
					    + group_0B_count;
				} else {
					block2[0] =
					    (0x0C | ((program_type_code & 0x07)
						     << 5))
					    + group_0B_count;
				}
				block2[1] =
				    0x08 | ((program_type_code & 0x18) >> 3);
				block3[0] = program_identification_code;
				block3[1] = program_identification_code >> 8;
				block4[0] = program_service[index1 + 1];
				block4[1] = program_service[index1 + 0];
				index1 += 2;
				group_0B_count++;
			} else {
				/* Transmit RT in Group 2A */
				block1[0] = program_identification_code;
				block1[1] = program_identification_code >> 8;
				if (a_b_flag)
					block2[0] = (0x10 |
						     ((program_type_code & 0x07)
						      << 5)) + group_2A_count;
				else
					block2[0] = (0x00 |
						     ((program_type_code & 0x07)
						      << 5)) + group_2A_count;
				block2[1] = 0x20 | ((program_type_code & 0x18)
						    >> 3);
				block3[0] = radio_text[index2 + 1];
				block3[1] = radio_text[index2 + 0];
				block4[0] = radio_text[index2 + 3];
				block4[1] = radio_text[index2 + 2];
				index2 += 4;
				group_2A_count++;
			}
			FM_DEBUG_REPORT("%02x%02x "
					"%02x%02x "
					"%02x%02x "
					"%02x%02x ",
					block1[1], block1[0],
					block2[1], block2[0],
					block3[1], block3[0],
					block4[1], block4[0]);
			result = fmd_tx_set_group
			    (context,
			     group_position, block1, block2, block3, block4);
			group_position++;
			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_transmit_rds_groups: "
					      "fmd_tx_set_group failed %d",
					      (unsigned int)result);
				result = STE_STATUS_SYSTEM_ERROR;
				break;
			}
		}
		a_b_flag = !a_b_flag;
	} else {
		FM_ERR_REPORT("ste_fm_transmit_rds_groups: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_transmit_rds_groups: returning %s",
			str_stestatus(result));
	return result;
}

/**
 * ste_fm_driver_callback()- Callback function indicating the event.
 * This callback function is called on receiving irpt_CommandSucceeded,
 * irpt_CommandFailed, irpt_bufferFull, etc from FM chip.
 * @context: Pointer to the Context of the FM Driver
 * @event: event for which the callback function was caled
 * from FM Driver.
 * @event_successful: Signifying whether the event is called from FM Driver
 * on receiving irpt_OperationSucceeded or irpt_OperationFailed.
 */
static void ste_fm_driver_callback(void *context,
				   u8 event, bool event_successful)
{
	FM_DEBUG_REPORT("ste_fm_driver_callback: "
			"event = %02x, event_successful = %x",
			event, event_successful);

	if (event_successful) {
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

		case FMD_EVENT_SEEK_STOPPED:
			FM_DEBUG_REPORT("FMD_EVENT_SEEK_STOPPED");
			global_event = STE_EVENT_SCAN_CANCELLED;
			wake_up_poll_queue();
			break;

		case FMD_EVENT_SEEK_COMPLETED:
			FM_DEBUG_REPORT("FMD_EVENT_SEEK_COMPLETED");
			global_event = STE_EVENT_SEARCH_CHANNEL_FOUND;
			wake_up_poll_queue();
			break;

		case FMD_EVENT_SCAN_BAND_COMPLETED:
			FM_DEBUG_REPORT("FMD_EVENT_SCAN_BAND_COMPLETED");
			global_event = STE_EVENT_SCAN_CHANNELS_FOUND;
			wake_up_poll_queue();
			break;

		case FMD_EVENT_BLOCK_SCAN_COMPLETED:
			FM_DEBUG_REPORT("FMD_EVENT_BLOCK_SCAN_COMPLETED");
			global_event = STE_EVENT_BLOCK_SCAN_CHANNELS_FOUND;
			wake_up_poll_queue();
			break;

		case FMD_EVENT_AF_UPDATE_SWITCH_COMPLETE:
			FM_DEBUG_REPORT("FMD_EVENT_AF_UPDATE_SWITCH_COMPLETE");
			break;

		case FMD_EVENT_RDSGROUP_RCVD:
			FM_DEBUG_REPORT("FMD_EVENT_RDSGROUP_RCVD");
			os_set_rds_sem();
			break;

		default:
			FM_ERR_REPORT("ste_fm_driver_callback: "
				      "Unknown event = %x", event);
			break;
		}
	} else {
		switch (event) {
			/* Seek stop, band scan, seek, block scan could
			 * fail for some reason so wake up poll queue */
		case FMD_EVENT_SEEK_STOPPED:
			FM_DEBUG_REPORT("FMD_EVENT_SEEK_STOPPED");
			global_event = STE_EVENT_SCAN_CANCELLED;
			wake_up_poll_queue();
			break;
		case FMD_EVENT_SEEK_COMPLETED:
			FM_DEBUG_REPORT("FMD_EVENT_SEEK_COMPLETED");
			global_event = STE_EVENT_SEARCH_CHANNEL_FOUND;
			wake_up_poll_queue();
			break;
		case FMD_EVENT_SCAN_BAND_COMPLETED:
			FM_DEBUG_REPORT("FMD_EVENT_SCAN_BAND_COMPLETED");
			global_event = STE_EVENT_SCAN_CHANNELS_FOUND;
			wake_up_poll_queue();
			break;
		case FMD_EVENT_BLOCK_SCAN_COMPLETED:
			FM_DEBUG_REPORT("FMD_EVENT_BLOCK_SCAN_COMPLETED");
			global_event = STE_EVENT_BLOCK_SCAN_CHANNELS_FOUND;
			wake_up_poll_queue();
			break;
		default:
			FM_ERR_REPORT("ste_fm_driver_callback: "
				      "event = %x failed!!!!", event);
			break;
		}
	}
}

/**
 * ste_fm_rds_callback()- Function to retrieve the RDS groups.
 * This is called when the chip has received enough RDS groups
 * so an interrupt irpt_BufferFull is generated to read the groups.
 */
static void ste_fm_rds_callback(void)
{
	u8 index = 0;
	u8 rds_local_buf_count;
	FM_DEBUG_REPORT("ste_fm_rds_callback");
	if (fm_rds_status) {
		/* Wait till interrupt is RDS Buffer
		 * full interrupt is received */
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
			/* Status are in reverse order because of Endianness
			 * of status byte received from chip */
			fmd_rx_get_low_level_rds_groups(context,
							index,
							&rds_local_buf_count,
							&ste_fm_rds_buf
							[ste_fm_rds_info.
							 rds_head]
							[index].block1,
							&ste_fm_rds_buf
							[ste_fm_rds_info.
							 rds_head]
							[index].block2,
							&ste_fm_rds_buf
							[ste_fm_rds_info.
							 rds_head]
							[index].block3,
							&ste_fm_rds_buf
							[ste_fm_rds_info.
							 rds_head]
							[index].block4,
							&ste_fm_rds_buf
							[ste_fm_rds_info.
							 rds_head]
							[index].status2,
							&ste_fm_rds_buf
							[ste_fm_rds_info.
							 rds_head]
							[index].status1,
							&ste_fm_rds_buf
							[ste_fm_rds_info.
							 rds_head]
							[index].status4,
							&ste_fm_rds_buf
							[ste_fm_rds_info.
							 rds_head]
							[index].status3);
			/*FM_DEBUG_REPORT("%04x %04x "\
			"%04x %04x %02x %02x "\
			"%02x %02x", ste_fm_rds_buf[ste_fm_rds_info.rds_head]
			[index].block1,
			ste_fm_rds_buf[ste_fm_rds_info.rds_head][index].block2,
			ste_fm_rds_buf[ste_fm_rds_info.rds_head][index].block3,
			ste_fm_rds_buf[ste_fm_rds_info.rds_head][index].block4,
			ste_fm_rds_buf[ste_fm_rds_info.rds_head][index].status1,
			ste_fm_rds_buf[ste_fm_rds_info.rds_head][index].status2,
			ste_fm_rds_buf[ste_fm_rds_info.rds_head][index].status3,
			ste_fm_rds_buf[ste_fm_rds_info.rds_head]
			[index].status4); */
			index++;
		}
		ste_fm_rds_info.rds_head++;
		if (ste_fm_rds_info.rds_head == MAX_RDS_BUFFER)
			ste_fm_rds_info.rds_head = 0;
		wake_up_read_queue();
		mutex_unlock(&rds_mutex);
	}
}

/**
 * str_stestatus()- Returns the status value as a printable string.
 * @status: Status
 *
 * Returns:
 *	 Pointer to a string containing printable value of status
 */
static const char *str_stestatus(u8 status)
{
	switch (status) {
	case STE_STATUS_OK:
		return "STE_STATUS_OK";
	case STE_STATUS_SYSTEM_ERROR:
		return "STE_STATUS_SYSTEM_ERROR";
	default:
		return "Unknown status";
	}
}

/* ------------------ External function definitions ---------------------- */

u8 ste_fm_init(void)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_init");

	if (STE_FM_STATE_DEINITIALIZED == ste_fm_state) {

		mutex_init(&rds_mutex);

		/* Initalize the Driver */
		if (fmd_init(&context) != FMD_RESULT_SUCCESS) {
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Register the callback */
		if (fmd_register_callback(context,
					  (fmd_radio_cb) ste_fm_driver_callback)
		    != FMD_RESULT_SUCCESS) {
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* initialize global variables */
		global_event = STE_EVENT_NO_EVENT;
		ste_fm_state = STE_FM_STATE_INITIALIZED;
		ste_fm_mode = STE_FM_IDLE_MODE;
		memset(&ste_fm_rds_info, 0, sizeof(struct ste_fm_rds_info_t));
		memset(ste_fm_rds_buf, 0,
		       sizeof(struct ste_fm_rds_buf_t) *
		       MAX_RDS_BUFFER * MAX_RDS_GROUPS);
	} else {
		FM_ERR_REPORT("ste_fm_init: Already Initialized");
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_init: returning %s", str_stestatus(result));
	return result;

}

u8 ste_fm_deinit(void)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_deinit");

	if (STE_FM_STATE_INITIALIZED == ste_fm_state) {
		fmd_deinit();
		mutex_destroy(&rds_mutex);
		context = NULL;
		ste_fm_state = STE_FM_STATE_DEINITIALIZED;
		ste_fm_mode = STE_FM_IDLE_MODE;
	} else {
		FM_ERR_REPORT("ste_fm_deinit: Already de-Initialized");
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_deinit: returning %s", str_stestatus(result));
	return result;
}

u8 ste_fm_switch_on(struct device *device)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_switch_on");

	if (STE_FM_STATE_INITIALIZED == ste_fm_state) {
		/* Enable FM IP */
		FM_DEBUG_REPORT("ste_fm_switch_on: " "Sending FM IP Enable");

		if (fmd_send_fm_ip_enable()) {
			FM_ERR_REPORT("ste_fm_switch_on: "
				      "Error in fmd_send_fm_ip_enable");
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Now Download the Coefficient Files and FM Firmware */
		if (STE_STATUS_OK != ste_fm_load_firmware(device)) {
			FM_ERR_REPORT("ste_fm_switch_on: "
				      "Error in downloading firmware");
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Power up FM */
		result = fmd_power_up(context);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_switch_on: "
				      "fmd_power_up failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Enable the Ref Clk */
		FM_DEBUG_REPORT("ste_fm_switch_on: "
				"Sending fmd_select_ref_clk");
		result = fmd_select_ref_clk(context, 0x0001);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_switch_on: "
				      "fmd_select_ref_clk failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Select the Frequency of Ref Clk (28 MHz) */
		FM_DEBUG_REPORT("ste_fm_switch_on: "
				"Sending fmd_set_ref_clk_pll");
		result = fmd_set_ref_clk_pll(context, 0x32C8);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_switch_on: "
				      "fmd_select_ref_clk_pll failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_switch_on: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}
	ste_fm_state = STE_FM_STATE_SWITCHED_ON;
	ste_fm_mode = STE_FM_IDLE_MODE;
	memset(&ste_fm_rds_info, 0, sizeof(struct ste_fm_rds_info_t));
	memset(ste_fm_rds_buf, 0,
	       sizeof(struct ste_fm_rds_buf_t) *
	       MAX_RDS_BUFFER * MAX_RDS_GROUPS);

error:
	FM_DEBUG_REPORT("ste_fm_switch_on: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_switch_off(void)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_switch_off");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		/* Stop the RDS Thread if it is running */
		if (fm_rds_status) {
			fm_rds_status = STE_FALSE;
			os_stop_rds_thread();
		}
		result = fmd_goto_power_down(context);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_switch_off: "
				      "FMLGotoPowerdown failed = %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
		if (STE_STATUS_OK == result) {
			if (fmd_send_fm_ip_disable()) {
				FM_ERR_REPORT("ste_fm_switch_off: "
					      "Problem in fmd_send_fm_ip_"
					      "disable");
				result = STE_STATUS_SYSTEM_ERROR;
				goto error;
			}
		}
		if (STE_STATUS_OK == result) {
			ste_fm_state = STE_FM_STATE_INITIALIZED;
			ste_fm_mode = STE_FM_IDLE_MODE;
			memset(&ste_fm_rds_info, 0,
			       sizeof(struct ste_fm_rds_info_t));
			memset(ste_fm_rds_buf, 0,
			       sizeof(struct ste_fm_rds_buf_t) *
			       MAX_RDS_BUFFER * MAX_RDS_GROUPS);
		}
	} else {
		FM_ERR_REPORT("ste_fm_switch_off: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_switch_off: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_standby(void)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_standby");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_goto_standby(context);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_standby: "
				      "FMLGotoStandby failed, "
				      "err = %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
		ste_fm_state = STE_FM_STATE_STAND_BY;
	} else {
		FM_ERR_REPORT("ste_fm_standby: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_standby: returning %s", str_stestatus(result));
	return result;
}

u8 ste_fm_power_up_from_standby(void)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_power_up_from_standby");

	if (STE_FM_STATE_STAND_BY == ste_fm_state) {
		/* Power up FM */
		result = fmd_power_up(context);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_power_up_from_standby: "
				      "fmd_power_up failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		} else {
			ste_fm_state = STE_FM_STATE_SWITCHED_ON;
			if (STE_FM_TX_MODE == ste_fm_mode) {
				/* Enable the PA */
				result = fmd_tx_set_pa(context, STE_TRUE);
				if (FMD_RESULT_SUCCESS != result) {
					FM_ERR_REPORT
					    ("ste_fm_power_up_from_standby:"
					     " fmd_tx_set_pa " "failed %d",
					     (unsigned int)result);
					result = STE_STATUS_SYSTEM_ERROR;
					goto error;
				}
			}
		}
	} else {
		FM_ERR_REPORT("ste_fm_power_up_from_standby: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_power_up_from_standby: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_set_rx_default_settings(u32 freq,
				  u8 band,
				  u8 grid, bool enable_rds, bool enable_stereo)
{
	u8 result = STE_STATUS_OK;
	u8 vol_in_percentage;

	FM_INFO_REPORT("ste_fm_set_rx_default_settings: freq = %d Hz, "
		       "band = %d, grid = %d, RDS = %d, Stereo Mode = %d",
		       freq, band, grid, enable_rds, enable_stereo);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		ste_fm_mode = STE_FM_RX_MODE;

		FM_DEBUG_REPORT("ste_fm_set_rx_default_settings: "
				"Sending Set mode to Rx");
		result = fmd_set_mode(context, FMD_MODE_RX);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_rx_default_settings: "
				      "fmd_set_mode failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Set the Grid */
		FM_DEBUG_REPORT("ste_fm_set_rx_default_settings: "
				"Sending fmd_rx_set_grid ");
		result = fmd_rx_set_grid(context, grid);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_rx_default_settings: "
				      "fmd_rx_set_grid failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Set the Band */
		FM_DEBUG_REPORT("ste_fm_set_rx_default_settings: "
				"Sending Set fmd_set_freq_range");
		result = fmd_set_freq_range(context, band);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_rx_default_settings: "
				      "fmd_rx_set_grid failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Set the Frequency */
		FM_DEBUG_REPORT("ste_fm_set_rx_default_settings: "
				"Sending Set fmd_rx_set_frequency");
		result = fmd_rx_set_frequency(context, freq / 1000);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_rx_default_settings: "
				      "fmd_rx_set_frequency failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		FM_DEBUG_REPORT("ste_fm_set_rx_default_settings: "
				"SetFrequency interrupt received, "
				"Sending Set fmd_rx_set_stereo_mode");

		if (STE_TRUE == enable_stereo) {
			/* Set the Stereo Blending mode */
			result = fmd_rx_set_stereo_mode
			    (context, FMD_STEREOMODE_BLENDING);
		} else {
			/* Set the Mono mode */
			result = fmd_rx_set_stereo_mode
			    (context, FMD_STEREOMODE_MONO);
		}
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_rx_default_settings: "
				      "fmd_rx_set_stereo_mode "
				      "failed %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		FM_DEBUG_REPORT("ste_fm_set_rx_default_settings: "
				"Sending Set rds");

		if (STE_TRUE == enable_rds) {
			/* Enable RDS */
			result = ste_fm_rds_on();
		} else {
			/* Disable RDS */
			result = ste_fm_rds_off();
		}
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_rx_default_settings: "
				      "ste_fm_rds_on "
				      "failed %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Set the Analog Out Volume to Max */
		vol_in_percentage = (u8)
		    (((u16) (MAX_ANALOG_VOLUME) * 100)
		     / MAX_ANALOG_VOLUME);
		result = fmd_set_volume(context, vol_in_percentage);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_switch_on: "
				      "FMRSetVolume failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_rx_default_settings: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_set_rx_default_settings: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_set_tx_default_settings(u32 freq,
				  u8 band,
				  u8 grid, bool enable_rds, bool enable_stereo)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_set_tx_default_settings: freq = %d Hz, "
		       "band = %d, grid = %d, RDS = %d, Stereo Mode = %d",
		       freq, band, grid, enable_rds, enable_stereo);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		ste_fm_mode = STE_FM_TX_MODE;
		if (fm_rds_status) {
			fm_rds_status = STE_FALSE;
			os_stop_rds_thread();
			memset(&ste_fm_rds_info, 0,
			       sizeof(struct ste_fm_rds_info_t));
			memset(ste_fm_rds_buf, 0,
			       sizeof(struct ste_fm_rds_buf_t) *
			       MAX_RDS_BUFFER * MAX_RDS_GROUPS);
			os_sleep(50);
		}

		/* Switch To Tx mode */
		FM_DEBUG_REPORT("ste_fm_set_tx_default_settings: "
				"Sending Set mode to Tx");
		result = fmd_set_mode(context, FMD_MODE_TX);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_tx_default_settings: "
				      "fmd_set_mode failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Set the Grid */
		FM_DEBUG_REPORT("ste_fm_set_tx_default_settings: "
				"Sending fmd_tx_set_grid ");
		result = fmd_tx_set_grid(context, grid);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_tx_default_settings: "
				      "fmd_tx_set_grid failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Set the Band */
		FM_DEBUG_REPORT("ste_fm_set_tx_default_settings: "
				"Sending fmd_tx_set_freq_range");
		result = fmd_tx_set_freq_range(context, band);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_tx_default_settings: "
				      "fmd_tx_set_freq_range failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Set the Band */
		FM_DEBUG_REPORT("ste_fm_set_tx_default_settings: "
				"Sending fmd_tx_set_preemphasis");
		result = fmd_tx_set_preemphasis(context, FMD_EMPHASIS_75US);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_switch_on: "
				      "fmd_tx_set_preemphasis failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Set the Frequency */
		FM_DEBUG_REPORT("ste_fm_set_tx_default_settings: "
				"Sending Set fmd_tx_set_frequency");
		result = fmd_tx_set_frequency(context, (freq / 1000));
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_switch_on: "
				      "fmd_tx_set_frequency failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		FM_DEBUG_REPORT("ste_fm_set_tx_default_settings: "
				"SetFrequency interrupt received, "
				"Sending Set fmd_tx_enable_stereo_mode");

		/* Set the Stereo mode */
		result = fmd_tx_enable_stereo_mode(context, enable_stereo);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_tx_default_settings: "
				      "fmd_tx_enable_stereo_mode "
				      "failed %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		FM_DEBUG_REPORT("ste_fm_set_tx_default_settings: "
				"Sending Set fmd_tx_set_pa");

		/* Enable the PA */
		result = fmd_tx_set_pa(context, STE_TRUE);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_tx_default_settings: "
				      "fmd_tx_set_pa "
				      "failed %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		FM_DEBUG_REPORT("ste_fm_set_tx_default_settings: "
				"set PA interrupt received, "
				"Sending Set fmd_tx_set_signal_strength");

		/* Set the Signal Strength to Max */
		result = fmd_tx_set_signal_strength(context, 123);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_tx_default_settings: "
				      "fmd_tx_set_signal_strength "
				      "failed %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* Enable Tx RDS  */
		FM_DEBUG_REPORT("ste_fm_set_tx_default_settings: "
				"Sending Set ste_fm_tx_rds");
		result = ste_fm_tx_rds(enable_rds);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_tx_default_settings: "
				      "ste_fm_tx_rds "
				      "failed %x", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_tx_default_settings: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_set_tx_default_settings: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_set_grid(u8 grid)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_set_grid: Grid = %d", grid);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_rx_set_grid(context, grid);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_grid: "
				      "fmd_rx_set_grid failed");
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_grid: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_set_grid: returning %s", str_stestatus(result));
	return result;
}

u8 ste_fm_set_band(u8 band)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_set_band: Band = %d", band);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_set_freq_range(context, band);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_band: "
				      "fmd_set_freq_range failed %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_band: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_set_band: returning %s", str_stestatus(result));
	return result;
}

u8 ste_fm_search_up_freq(void)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_search_up_freq");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		if (fm_rds_status) {
			/* Stop RDS if it is active */
			result = ste_fm_rds_off();
			fm_prev_rds_status = STE_TRUE;
		}
		result = fmd_rx_seek(context, STE_DIR_UP);
		if (FMD_RESULT_ONGOING != result) {
			FM_ERR_REPORT("ste_fm_search_up_freq: "
				      "Error Code %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		} else {
			memset(&ste_fm_rds_info, 0,
			       sizeof(struct ste_fm_rds_info_t));
			memset(ste_fm_rds_buf, 0,
			       sizeof(struct ste_fm_rds_buf_t) *
			       MAX_RDS_BUFFER * MAX_RDS_GROUPS);
			result = STE_STATUS_OK;
		}
	} else {
		FM_ERR_REPORT("ste_fm_search_up_freq: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_search_up_freq: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_search_down_freq(void)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_search_down_freq");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		if (fm_rds_status) {
			/* Stop RDS if it is active */
			result = ste_fm_rds_off();
			fm_prev_rds_status = STE_TRUE;
		}
		result = fmd_rx_seek(context, STE_DIR_DOWN);
		if (FMD_RESULT_ONGOING != result) {
			FM_ERR_REPORT("ste_fm_search_down_freq: "
				      "Error Code %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		} else {
			memset(&ste_fm_rds_info, 0,
			       sizeof(struct ste_fm_rds_info_t));
			memset(ste_fm_rds_buf, 0,
			       sizeof(struct ste_fm_rds_buf_t) *
			       MAX_RDS_BUFFER * MAX_RDS_GROUPS);
			result = STE_STATUS_OK;
		}
	} else {
		FM_ERR_REPORT("ste_fm_search_down_freq: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_search_down_freq: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_start_band_scan(void)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_start_band_scan");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		if (fm_rds_status) {
			/* Stop RDS if it is active */
			result = ste_fm_rds_off();
			fm_prev_rds_status = STE_TRUE;
		}
		result = fmd_rx_scan_band(context, DEFAULT_CHANNELS_TO_SCAN);
		if (FMD_RESULT_ONGOING != result) {
			FM_ERR_REPORT("ste_fm_start_band_scan: "
				      "Error Code %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		} else {
			memset(&ste_fm_rds_info, 0,
			       sizeof(struct ste_fm_rds_info_t));
			memset(ste_fm_rds_buf, 0,
			       sizeof(struct ste_fm_rds_buf_t) *
			       MAX_RDS_BUFFER * MAX_RDS_GROUPS);
			result = STE_STATUS_OK;
		}
	} else {
		FM_ERR_REPORT("ste_fm_start_band_scan: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_start_band_scan: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_stop_scan(void)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_stop_scan");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_rx_stop_seeking(context);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_stop_scan: "
				      "Error Code %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		} else {
			memset(&ste_fm_rds_info, 0,
			       sizeof(struct ste_fm_rds_info_t));
			memset(ste_fm_rds_buf, 0,
			       sizeof(struct ste_fm_rds_buf_t) *
			       MAX_RDS_BUFFER * MAX_RDS_GROUPS);
			result = STE_STATUS_OK;
			if (fm_prev_rds_status) {
				/* Restart RDS if it was
				 * active earlier */
				ste_fm_rds_on();
				fm_prev_rds_status = STE_FALSE;
			}
		}
	} else {
		FM_ERR_REPORT("ste_fm_stop_scan: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_stop_scan: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_get_scan_result(u16 *num_of_scanfreq,
			  u32 *scan_freq, u32 *scan_freq_rssi_level)
{
	u8 result = STE_STATUS_OK;
	u32 cnt;
	u32 index;
	u32 minfreq;
	u32 maxfreq;
	u16 channels[3];
	u16 rssi[3];
	u8 freq_range;
	u8 max_channels = 0;

	FM_INFO_REPORT("ste_fm_get_scan_result");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {

		result = fmd_get_freq_range(context, &freq_range);

		if (STE_STATUS_OK == result)
			result = fmd_get_freq_range_properties(context,
							       freq_range,
							       &minfreq,
							       &maxfreq);
		result =
		    fmd_rx_get_max_channels_to_scan(context, &max_channels);

		if (STE_STATUS_OK != result) {
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}

		/* In 1 iteration we can retreive max 3 channels */
		cnt = (max_channels / 3) + 1;
		while ((cnt--) && (result == FMD_RESULT_SUCCESS)) {
			/* Get all channels, including empty ones.
			 * In 1 iteration at max 3 channels can be found.
			 */
			result = fmd_rx_get_scan_band_info(context, cnt * 3,
							   num_of_scanfreq,
							   channels, rssi);
			if (result == FMD_RESULT_SUCCESS) {
				index = cnt * 3;
				/* Convert Freq to Hz from channel number */
				scan_freq[index] = (minfreq +
						    channels[0] * 50) * 1000;
				scan_freq_rssi_level[index] = rssi[0];
				/* Convert Freq to Hz from channel number */
				scan_freq[index + 1] = (minfreq +
							channels[1] * 50) *
				    1000;
				scan_freq_rssi_level[index + 1] = rssi[1];
				/* Check if we donot overwrite the array */
				if (cnt < (max_channels / 3)) {
					/* Convert Freq to Hz from channel
					 * number */
					scan_freq[index + 2] =
					    (minfreq + channels[2] * 50) * 1000;
					scan_freq_rssi_level[index + 2]
					    = rssi[2];
				}
			}
		}
		if (fm_prev_rds_status) {
			/* Restart RDS if it was active earlier */
			result = ste_fm_rds_on();
			fm_prev_rds_status = STE_FALSE;
		}
	} else {
		FM_ERR_REPORT("ste_fm_get_scan_result: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_get_scan_result: returning %s",
			str_stestatus(result));
	return result;

}

u8 ste_fm_start_block_scan(void)
{
	u8 result = STE_STATUS_OK;
	u32 start_freq;
	u32 end_freq;
	u8 antenna;

	FM_INFO_REPORT("ste_fm_start_block_scan");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		if (fm_rds_status) {
			/* Stop RDS if it is active */
			result = ste_fm_rds_off();
			fm_prev_rds_status = STE_TRUE;
		}
		start_freq = 87500;
		end_freq = 90000;
		result = fmd_get_antenna(context, &antenna);
		result = fmd_rx_block_scan(context,
					   start_freq, end_freq, antenna);
		if (FMD_RESULT_ONGOING != result) {
			FM_ERR_REPORT("ste_fm_start_block_scan: "
				      "Error Code %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		} else {
			memset(&ste_fm_rds_info, 0,
			       sizeof(struct ste_fm_rds_info_t));
			memset(ste_fm_rds_buf, 0,
			       sizeof(struct ste_fm_rds_buf_t) *
			       MAX_RDS_BUFFER * MAX_RDS_GROUPS);
			result = STE_STATUS_OK;
		}
	} else {
		FM_ERR_REPORT("ste_fm_start_block_scan: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_start_block_scan: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_get_block_scan_result(u16 *num_of_scanchan,
				u16 *scan_freq_rssi_level)
{
	u8 result = STE_STATUS_OK;
	u32 cnt;
	u32 index;
	u16 rssi[6];

	FM_INFO_REPORT("ste_fm_get_block_scan_result");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		cnt = 33;
		while ((cnt--) && (result == FMD_RESULT_SUCCESS)) {
			/* Get all channels, including empty ones */
			result = fmd_rx_get_block_scan_result(context,
							      cnt * 6,
							      num_of_scanchan,
							      rssi);
			if (result == FMD_RESULT_SUCCESS) {
				index = cnt * 6;
				scan_freq_rssi_level[index]
				    = rssi[0];
				scan_freq_rssi_level[index + 1]
				    = rssi[1];
				scan_freq_rssi_level[index + 2]
				    = rssi[2];
				scan_freq_rssi_level[index + 3]
				    = rssi[3];
				scan_freq_rssi_level[index + 4]
				    = rssi[4];
				scan_freq_rssi_level[index + 5]
				    = rssi[5];
			}
		}
		if (STE_FM_RX_MODE == ste_fm_mode) {
			if (fm_prev_rds_status) {
				/* Restart RDS if it was
				 * active earlier */
				result = ste_fm_rds_on();
				fm_prev_rds_status = STE_FALSE;
			}
		} else if (STE_FM_TX_MODE == ste_fm_mode) {
			FM_DEBUG_REPORT("ste_fm_get_block_scan_result:"
					" Sending Set fmd_tx_set_pa");

			/* Enable the PA */
			result = fmd_tx_set_pa(context, STE_TRUE);
			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_get_block_scan_result:"
					      " fmd_tx_set_pa "
					      "failed %d",
					      (unsigned int)result);
				result = STE_STATUS_SYSTEM_ERROR;
				goto error;
			}
		}
	} else {
		FM_ERR_REPORT("ste_fm_get_block_scan_result: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_get_block_scan_result: returning %s",
			str_stestatus(result));
	return result;

}

u8 ste_fm_tx_rds(bool enable_rds)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_rds: enable_rds = %d", enable_rds);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		if (STE_TRUE == enable_rds) {
			/* Set the Tx Buffer Size */
			result = fmd_tx_buffer_set_size(context,
							MAX_RDS_GROUPS - 2);
			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_tx_rds: "
					      "fmd_tx_buffer_set_size "
					      "failed %d",
					      (unsigned int)result);
				result = STE_STATUS_SYSTEM_ERROR;
				goto error;
			} else {
				result = fmd_tx_set_rds(context, STE_TRUE);
			}

			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_tx_rds: "
					      "fmd_tx_set_rds "
					      "failed %d",
					      (unsigned int)result);
				result = STE_STATUS_SYSTEM_ERROR;
				goto error;
			} else {
				program_identification_code =
				    default_program_identification_code;
				program_type_code = default_program_type_code;
				os_mem_copy(program_service,
					    default_program_service,
					    MAX_PSN_SIZE);
				os_mem_copy(radio_text,
					    default_radio_text, MAX_RT_SIZE);
				radio_text[strlen(radio_text)] = 0x0D;
				ste_fm_transmit_rds_groups();
				result = STE_STATUS_OK;
			}
		} else {
			result = fmd_tx_set_rds(context, STE_FALSE);

			if (FMD_RESULT_SUCCESS != result) {
				FM_ERR_REPORT("ste_fm_tx_rds: "
					      "fmd_tx_set_rds "
					      "failed %d",
					      (unsigned int)result);
				result = STE_STATUS_SYSTEM_ERROR;
				goto error;
			}
		}
	} else {
		FM_ERR_REPORT("ste_fm_tx_rds: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_rds: returning %s", str_stestatus(result));

	return result;
}

u8 ste_fm_tx_set_pi_code(u16 pi_code)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_set_pi_code: PI = %04x", pi_code);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		program_identification_code = pi_code;
		result = ste_fm_transmit_rds_groups();
	} else {
		FM_ERR_REPORT("ste_fm_tx_set_pi_code: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_set_pi_code: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_tx_set_pty_code(u16 pty_code)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_set_pty_code: PI = %04x", pty_code);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		program_type_code = pty_code;
		result = ste_fm_transmit_rds_groups();
	} else {
		FM_ERR_REPORT("ste_fm_tx_set_pty_code: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_set_pty_code: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_tx_set_program_station_name(char *psn, u8 len)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_set_program_station_name: " "PSN = %s", psn);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		if (len < (MAX_PSN_SIZE - 1)) {
			int count = len - 1;
			while (count < (MAX_PSN_SIZE - 1))
				psn[count++] = ' ';
		}
		memset(program_service, 0, MAX_PSN_SIZE);
		os_mem_copy(program_service, psn, MAX_PSN_SIZE);
		result = ste_fm_transmit_rds_groups();
	} else {
		FM_ERR_REPORT("ste_fm_tx_set_program_station_name: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_set_program_station_name: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_tx_set_radio_text(char *rt, u8 len)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_set_radio_text: " "RT = %s", rt);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		rt[len] = 0x0D;
		memset(radio_text, 0, MAX_RT_SIZE);
		os_mem_copy(radio_text, rt, len + 1);

		result = ste_fm_transmit_rds_groups();
	} else {
		FM_ERR_REPORT("ste_fm_tx_set_radio_text: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_set_radio_text: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_tx_get_rds_deviation(u16 *deviation)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_get_rds_deviation");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_tx_get_rds_deviation(context, deviation);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_tx_get_rds_deviation: "
				      "fmd_tx_get_rds_deviation failed %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_tx_get_rds_deviation: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_get_rds_deviation: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_tx_set_rds_deviation(u16 deviation)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_set_rds_deviation: deviation = %d",
		       deviation);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_tx_set_rds_deviation(context, deviation);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_tx_set_rds_deviation: "
				      "fmd_tx_set_rds_deviation failed %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_tx_set_rds_deviation: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_set_rds_deviation: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_tx_get_pilot_tone_status(bool *enable)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_get_pilot_tone_status");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_tx_get_stereo_mode(context, enable);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_tx_get_pilot_tone_status: "
				      "fmd_tx_get_stereo_mode failed %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_tx_get_pilot_tone_status: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_get_pilot_tone_status: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_tx_set_pilot_tone_status(bool enable)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_set_pilot_tone_status: enable = %d", enable);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_tx_enable_stereo_mode(context, enable);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_tx_set_pilot_tone_status: "
				      "fmd_tx_enable_stereo_mode failed %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_tx_set_pilot_tone_status: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_set_pilot_tone_status: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_tx_get_pilot_deviation(u16 *deviation)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_get_pilot_deviation");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_tx_get_pilot_deviation(context, deviation);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_tx_get_pilot_deviation: "
				      "fmd_tx_get_pilot_deviation failed %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_tx_get_pilot_deviation: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_get_pilot_deviation: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_tx_set_pilot_deviation(u16 deviation)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_set_pilot_deviation: deviation = %d",
		       deviation);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_tx_set_pilot_deviation(context, deviation);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_tx_set_pilot_deviation: "
				      "fmd_tx_set_pilot_deviation failed %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_tx_set_pilot_deviation: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_set_pilot_deviation: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_tx_get_preemphasis(u8 *preemphasis)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_get_preemphasis");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_tx_get_preemphasis(context, preemphasis);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_tx_get_preemphasis: "
				      "fmd_tx_get_preemphasis failed %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_tx_get_preemphasis: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_get_preemphasis: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_tx_set_preemphasis(u8 preemphasis)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_set_preemphasis: preemphasis = %d",
		       preemphasis);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_tx_set_preemphasis(context, preemphasis);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_tx_set_preemphasis: "
				      "fmd_tx_set_preemphasis failed %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_tx_set_preemphasis: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_set_preemphasis: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_tx_get_power_level(u16 *power_level)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_get_power_level");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_tx_get_signal_strength(context, power_level);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_tx_get_power_level: "
				      "fmd_tx_get_signal_strength failed %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_tx_get_power_level: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_get_power_level: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_tx_set_power_level(u16 power_level)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_tx_set_power_level: power_level = %d",
		       power_level);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_tx_set_signal_strength(context, power_level);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_tx_set_power_level: "
				      "fmd_tx_set_preemphasis failed %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_tx_set_power_level: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_tx_set_power_level: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_set_audio_balance(s8 balance)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_set_audio_balance, balance = %d", balance);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_set_balance(context, balance);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("FMRSetAudioBalance : "
				      "Failed in fmd_set_balance, err = %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_audio_balance: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_set_audio_balance: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_set_volume(u8 vol_level)
{
	u8 result = STE_STATUS_OK;
	u8 vol_in_percentage;

	FM_INFO_REPORT("ste_fm_set_volume: Volume Level = %d", vol_level);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		vol_in_percentage =
		    (u8) (((u16) (vol_level) * 100) / MAX_ANALOG_VOLUME);
		result = fmd_set_volume(context, vol_in_percentage);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_increase_volume: "
				      "FMRSetVolume failed, err = %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_volume: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_set_volume: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_get_volume(u8 *vol_level)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_get_volume");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_get_volume(context, vol_level);
	} else {
		FM_ERR_REPORT("ste_fm_get_volume: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		*vol_level = 0;
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_get_volume: returning %s, VolLevel = %d",
			str_stestatus(result), *vol_level);
	return result;
}

u8 ste_fm_rds_off(void)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_rds_off");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_rx_set_rds(context, FMD_SWITCH_OFF_RDS);
		if (FMD_RESULT_SUCCESS == result) {
			/* Stop the RDS Thread */
			fm_rds_status = STE_FALSE;
			memset(&ste_fm_rds_info, 0,
			       sizeof(struct ste_fm_rds_info_t));
			memset(ste_fm_rds_buf, 0,
			       sizeof(struct ste_fm_rds_buf_t) *
			       MAX_RDS_BUFFER * MAX_RDS_GROUPS);
			FM_DEBUG_REPORT("ste_fm_rds_off: "
					"Stopping RDS Thread");
			os_stop_rds_thread();
		} else {
			FM_ERR_REPORT("ste_fm_rds_off: "
				      "fmd_rx_set_rds failed, err = %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_rds_off: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_rds_off: returning %s", str_stestatus(result));
	return result;
}

u8 ste_fm_rds_on(void)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_rds_on");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		FM_DEBUG_REPORT("ste_fm_rds_on:"
				" Sending fmd_rx_buffer_set_size");
		result = fmd_rx_buffer_set_size(context, MAX_RDS_GROUPS);
		if (FMD_RESULT_SUCCESS == result) {
			FM_DEBUG_REPORT("ste_fm_rds_on: "
					"Sending fmd_rx_buffer_set_threshold");
			result = fmd_rx_buffer_set_threshold(context,
							     MAX_RDS_GROUPS -
							     1);
		} else {
			FM_ERR_REPORT("ste_fm_rds_on: "
				      "fmd_rx_buffer_set_size failed, err = %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
		if (FMD_RESULT_SUCCESS == result) {
			FM_DEBUG_REPORT("ste_fm_rds_on: "
					"Sending fmd_rx_set_rds");
			result = fmd_rx_set_rds(context,
					FMD_SWITCH_ON_RDS_ENHANCED_MODE);
		} else {
			FM_ERR_REPORT("ste_fm_rds_on: "
				      "fmd_rx_buffer_set_threshold failed, "
				      "err = %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
		if (FMD_RESULT_SUCCESS == result) {
			/* Start the RDS Thread to
			 * read the RDS Buffers */
			fm_rds_status = STE_TRUE;
			memset(&ste_fm_rds_info, 0,
			       sizeof(struct ste_fm_rds_info_t));
			memset(ste_fm_rds_buf, 0,
			       sizeof(struct ste_fm_rds_buf_t) *
			       MAX_RDS_BUFFER * MAX_RDS_GROUPS);
			os_start_rds_thread(ste_fm_rds_callback);
		} else {
			FM_ERR_REPORT("ste_fm_rds_on: "
				      "fmd_rx_set_rds failed, err = %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_rds_on: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_rds_on: returning %s", str_stestatus(result));
	return result;
}

u8 ste_fm_get_rds_status(bool *rds_status)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_get_rds_status");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		if (STE_FM_RX_MODE == ste_fm_mode) {
			FM_DEBUG_REPORT("ste_fm_get_rds_status: "
					"fmd_rx_get_rds");
			result = fmd_rx_get_rds(context, rds_status);
		} else if (STE_FM_TX_MODE == ste_fm_mode) {
			FM_DEBUG_REPORT("ste_fm_get_rds_status: "
					"fmd_tx_get_rds");
			result = fmd_tx_get_rds(context, rds_status);
		}
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_get_rds_status: "
				      "fmd_get_rds failed, Error Code %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_get_rds_status: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_get_rds_status: returning %s, rds_status = %d",
			str_stestatus(result), *rds_status);
	return result;
}

u8 ste_fm_mute(void)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_mute");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		/* Mute Analog DAC */
		result = fmd_set_mute(context, STE_TRUE);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_mute: "
				      "fmd_set_mute failed %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		} else {
			/* Mute Ext Src */
			result = fmd_ext_set_mute(context, STE_TRUE);
			if (FMD_RESULT_SUCCESS != result) {
				result = STE_STATUS_SYSTEM_ERROR;
				goto error;
			}
		}
	} else {
		FM_ERR_REPORT("ste_fm_mute: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_mute: returning %s", str_stestatus(result));
	return result;
}

u8 ste_fm_unmute(void)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_unmute");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		/* Unmute Analog DAC */
		result = fmd_set_mute(context, STE_FALSE);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_mute: "
				      "fmd_set_mute failed %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		} else {
			/* Unmute Ext Src */
			result = fmd_ext_set_mute(context, STE_FALSE);
			if (FMD_RESULT_SUCCESS != result) {
				result = STE_STATUS_SYSTEM_ERROR;
				goto error;
			}
		}
	} else {
		FM_ERR_REPORT("ste_fm_unmute: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_unmute: returning %s", str_stestatus(result));
	return result;
}

u8 ste_fm_get_frequency(u32 *freq)
{
	u8 result = STE_STATUS_OK;
	u32 currentFreq = 0;

	FM_INFO_REPORT("ste_fm_get_frequency");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		if (STE_FM_RX_MODE == ste_fm_mode) {
			FM_DEBUG_REPORT("ste_fm_get_frequency: "
					"fmd_rx_get_frequency");
			result = fmd_rx_get_frequency(context,
						      (u32 *) &currentFreq);
		} else if (STE_FM_TX_MODE == ste_fm_mode) {
			FM_DEBUG_REPORT("ste_fm_get_frequency: "
					"fmd_tx_get_frequency");
			result = fmd_tx_get_frequency(context,
						      (u32 *) &currentFreq);
		}
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_get_frequency: "
				      "fmd_rx_get_frequency failed %d",
				      (unsigned int)result);
			*freq = 0;
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		} else {
			/* Convert To Hz */
			*freq = currentFreq * 1000;
			FM_DEBUG_REPORT("ste_fm_get_frequency: "
					"Current Frequency = %d Hz", *freq);
			if (fm_prev_rds_status) {
				/* Restart RDS if it was
				 * active earlier */
				ste_fm_rds_on();
				fm_prev_rds_status = STE_FALSE;
			}
		}
	} else {
		FM_ERR_REPORT("ste_fm_get_frequency: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		*freq = 0;
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_get_frequency: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_set_frequency(u32 new_freq)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_set_frequency, new_freq = %d", (int)new_freq);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		if (STE_FM_RX_MODE == ste_fm_mode) {
			FM_DEBUG_REPORT("ste_fm_set_frequency: "
					"fmd_rx_set_frequency");
			result = fmd_rx_set_frequency(context, new_freq / 1000);
		} else if (STE_FM_TX_MODE == ste_fm_mode) {
			FM_DEBUG_REPORT("ste_fm_set_frequency: "
					"fmd_tx_set_frequency");
			result = fmd_tx_set_frequency(context, new_freq / 1000);
		}
		if (result != FMD_RESULT_SUCCESS) {
			FM_ERR_REPORT("ste_fm_set_frequency: "
				      "fmd_rx_set_frequency failed %x",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		} else {
			if (STE_FM_TX_MODE == ste_fm_mode) {
				FM_DEBUG_REPORT("ste_fm_set_frequency:"
						" Sending Set" "fmd_tx_set_pa");

				/* Enable the PA */
				result = fmd_tx_set_pa(context, STE_TRUE);
				if (FMD_RESULT_SUCCESS != result) {
					FM_ERR_REPORT("ste_fm_set_frequency:"
						      " fmd_tx_set_pa "
						      "failed %d",
						      (unsigned int)result);
					result = STE_STATUS_SYSTEM_ERROR;
					goto error;
				}
			}
			memset(&ste_fm_rds_info, 0,
			       sizeof(struct ste_fm_rds_info_t));
			memset(ste_fm_rds_buf, 0,
			       sizeof(struct ste_fm_rds_buf_t) *
			       MAX_RDS_BUFFER * MAX_RDS_GROUPS);
			result = STE_STATUS_OK;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_frequency: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_set_frequency: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_get_signal_strength(u16 *signal_strength)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_get_signal_strength");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		if (STE_FM_RX_MODE == ste_fm_mode) {
			FM_DEBUG_REPORT("ste_fm_get_signal_strength: "
					"fmd_rx_get_signal_strength");
			result = fmd_rx_get_signal_strength(context,
							    signal_strength);
		} else if (STE_FM_TX_MODE == ste_fm_mode) {
			FM_DEBUG_REPORT("ste_fm_get_signal_strength: "
					"fmd_tx_get_signal_strength");
			result = fmd_tx_get_signal_strength(context,
							    signal_strength);
		}
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_get_signal_strength: "
				      "Error Code %d", (unsigned int)result);
			*signal_strength = 0;
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_get_signal_strength: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		*signal_strength = 0;
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_get_signal_strength: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_af_update_get_result(u16 *af_update_rssi)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_af_update_get_result");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_rx_get_af_update_result(context, af_update_rssi);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_af_update_get_result: "
				      "Error Code %d", (unsigned int)result);
			*af_update_rssi = 0;
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_af_update_get_result: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		*af_update_rssi = 0;
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_af_update_get_result: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_af_update_start(u32 af_freq)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_af_update_start");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_rx_af_update_start(context, af_freq / 1000);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_af_update_start: "
				      "Error Code %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_af_update_start: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_af_update_start: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_af_switch_get_result(u16 *af_switch_conclusion)
{
	u8 result = STE_STATUS_OK;
	u16 af_rssi;
	u16 af_pi;

	FM_INFO_REPORT("ste_fm_af_switch_get_result");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_rx_get_af_switch_results(context,
						      af_switch_conclusion,
						      &af_rssi, &af_pi);

		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_af_switch_get_result: "
				      "Error Code %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		} else {
			FM_DEBUG_REPORT("ste_fm_af_switch_get_result: "
					"AF Switch conclusion = %d "
					"AF Switch RSSI level = %d "
					"AF Switch PI code = %d ",
					*af_switch_conclusion, af_rssi, af_pi);
		}
	} else {
		FM_ERR_REPORT("ste_fm_af_switch_get_result: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_af_switch_get_result: returning %s",
			str_stestatus(result));
	return result;

}

u8 ste_fm_af_switch_start(u32 af_switch_freq, u16 af_switch_pi)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_af_switch_start");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_rx_af_switch_start(context,
						af_switch_freq / 1000,
						af_switch_pi);

		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_af_switch_start: "
				      "Error Code %d", (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_af_switch_start: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_af_switch_start: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_get_mode(u8 *cur_mode)
{
	u8 result = STE_STATUS_OK;
	bool stereo_mode;

	FM_INFO_REPORT("ste_fm_get_mode");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		if (STE_FM_RX_MODE == ste_fm_mode) {
			FM_DEBUG_REPORT("ste_fm_get_mode: "
					"fmd_rx_get_stereo_mode");
			result = fmd_rx_get_stereo_mode(context, cur_mode);
			switch (*cur_mode) {
			case FMD_STEREOMODE_OFF:
			case FMD_STEREOMODE_BLENDING:
				*cur_mode = STE_MODE_STEREO;
				break;
			case FMD_STEREOMODE_MONO:
			default:
				*cur_mode = STE_MODE_MONO;
				break;
			}
		} else if (STE_FM_TX_MODE == ste_fm_mode) {
			FM_DEBUG_REPORT("ste_fm_get_mode: "
					"fmd_tx_get_stereo_mode");
			result = fmd_tx_get_stereo_mode(context, &stereo_mode);
			if (STE_TRUE == stereo_mode)
				*cur_mode = STE_MODE_STEREO;
			else
				*cur_mode = STE_MODE_MONO;
		}
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_get_mode: "
				      "fmd_get_stereo_mode failed, "
				      "Error Code %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_get_mode: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		*cur_mode = STE_MODE_MONO;
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_get_mode: returning %s, mode = %d",
			str_stestatus(result), *cur_mode);
	return result;
}

u8 ste_fm_set_mode(u8 mode)
{
	u8 result = STE_STATUS_OK;
	bool enable_stereo_mode = STE_FALSE;

	FM_INFO_REPORT("ste_fm_set_mode: mode = %d", mode);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		if (STE_FM_RX_MODE == ste_fm_mode) {
			FM_DEBUG_REPORT("ste_fm_set_mode: "
					"fmd_rx_set_stereo_mode");
			result = fmd_rx_set_stereo_mode(context, mode);
		} else if (STE_FM_TX_MODE == ste_fm_mode) {
			FM_DEBUG_REPORT("ste_fm_set_mode: "
					"fmd_tx_set_stereo_mode");
			if (mode == STE_MODE_STEREO)
				enable_stereo_mode = STE_TRUE;
			result =
			    fmd_tx_enable_stereo_mode(context,
						      enable_stereo_mode);
		}
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_set_mode: "
				      "fmd_rx_set_stereo_mode failed, "
				      "Error Code %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_set_mode: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_set_mode: returning %s", str_stestatus(result));
	return result;
}

u8 ste_fm_select_antenna(u8 antenna)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_select_antenna: Antenna = %d", antenna);

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_set_antenna(context, antenna);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_select_antenna: "
				      "fmd_set_antenna failed, Error Code %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_select_antenna: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_select_antenna: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_get_antenna(u8 *antenna)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_get_antenna");

	if (STE_FM_STATE_SWITCHED_ON == ste_fm_state) {
		result = fmd_get_antenna(context, antenna);
		if (FMD_RESULT_SUCCESS != result) {
			FM_ERR_REPORT("ste_fm_get_antenna: "
				      "fmd_get_antenna failed, Error Code %d",
				      (unsigned int)result);
			result = STE_STATUS_SYSTEM_ERROR;
			goto error;
		}
	} else {
		FM_ERR_REPORT("ste_fm_get_antenna: "
			      "Invalid state of FM Driver = %d", ste_fm_state);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_get_antenna: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_get_rssi_threshold(u16 *rssi_thresold)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_get_rssi_threshold");

	result = fmd_rx_get_stop_level(context, rssi_thresold);
	if (FMD_RESULT_SUCCESS != result) {
		FM_ERR_REPORT("ste_fm_get_rssi_threshold: "
			      "fmd_rx_get_stop_level failed, Error Code %d",
			      (unsigned int)result);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_get_rssi_threshold: returning %s",
			str_stestatus(result));
	return result;
}

u8 ste_fm_set_rssi_threshold(u16 rssi_thresold)
{
	u8 result = STE_STATUS_OK;

	FM_INFO_REPORT("ste_fm_set_rssi_threshold: "
		       "RssiThresold = %d", rssi_thresold);

	result = fmd_rx_set_stop_level(context, rssi_thresold);
	if (FMD_RESULT_SUCCESS != result) {
		FM_ERR_REPORT("ste_fm_set_rssi_threshold: "
			      "fmd_rx_set_stop_level failed, Error Code %d",
			      (unsigned int)result);
		result = STE_STATUS_SYSTEM_ERROR;
		goto error;
	}

error:
	FM_DEBUG_REPORT("ste_fm_set_rssi_threshold: returning %s",
			str_stestatus(result));
	return result;
}

MODULE_AUTHOR("Hemant Gupta");
MODULE_LICENSE("GPL v2");
