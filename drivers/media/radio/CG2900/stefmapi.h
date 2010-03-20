/*
 * file stefmapi.h
 *
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Linux FM Host API's for ST-Ericsson FM Chip.
 *
 * Author: Hemant Gupta/hemant.gupta@stericsson.com for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2
 */

#ifndef STE_FM_API_H
#define STE_FM_API_H

#include <linux/device.h>

/* Callback function to receive RDS Data. */
typedef void  (*ste_fm_rds_cb)(void);

/**
  * struct ste_fm_rds_buf_s - RDS Group Receiving Structure
  * @block1: RDS Block A
  * @block2: RDS Block B
  * @block3: RDS Block C
  * @block4: RDS Block D
  * @status1: Status of received RDS Block A
  * @status2: Status of received RDS Block B
  * @status3: Status of received RDS Block C
  * @status4: Status of received RDS Block D
  *
  * Structure for receiving the RDS Group from FM Chip.
  *
  */
struct ste_fm_rds_buf_s {
	uint16_t block1;
	uint16_t block2;
	uint16_t block3;
	uint16_t block4;
	uint8_t status1;
	uint8_t status2;
	uint8_t status3;
	uint8_t status4;
};

#define STE_TRUE					1
#define STE_FALSE					0

#define STE_STATUS_OK					0
#define STE_STATUS_SYSTEM_ERROR				1
#define STE_STATUS_BUSY_ERROR				2
#define STE_STATUS_SEARCH_NO_CHANNEL_FOUND		3
#define STE_STATUS_SCAN_NO_CHANNEL_FOUND		4

#define STE_FM_BAND_US_EU				0
#define STE_FM_BAND_CHINA				1
#define STE_FM_BAND_JAPAN				2
#define STE_FM_BAND_CUSTOM				3

#define STE_FM_GRID_50					0
#define STE_FM_GRID_100					1
#define STE_FM_GRID_200					2

#define STE_EVENT_NO_EVENT				1
#define STE_EVENT_SEARCH_CHANNEL_FOUND			2
#define STE_EVENT_SCAN_CHANNELS_FOUND			3
#define STE_EVENT_NO_CHANNELS_FOUND			4

#define STE_FM_DEEMPHASIS_OFF				0
#define STE_FM_DEEMPHASIS_50us				1
#define STE_FM_DEEMPHASIS_75us				2

#define STE_DIR_DOWN					0
#define STE_DIR_UP					1

#define STE_FM_DAC_OFF					0
#define STE_FM_DAC_ON					1
#define STE_FM_DAC_LEFT_ONLY				2
#define STE_FM_DAC_RIGHT_ONLY				3

#define STE_STEREO_MODE_STEREO				0
#define STE_STEREO_MODE_FORCE_MONO			1
#define STE_STEREO_MODE_BLENDING			2
#define STE_STEREO_MODE_SWITCHING			3

#define STE_AFSWITCH_SUCCEEDED				0
#define STE_AFSWITCH_FAIL_LOW_RSSI			1
#define STE_AFSWITCH_FAIL_WRONG_PI			2
#define STE_AFSWITCH_FAIL_NO_RDS			3

#define STE_FM_DEFAULT_RSSI_THRESHOLD			100

#define MAX_RDS_BUFFER					10
#define MAX_RDS_GROUPS_READ			22
#define NUM_OF_RDS_BLOCKS				4

extern uint8_t global_event;
extern uint8_t rds_buf_count;
extern uint8_t head;
extern uint8_t tail;
extern uint8_t rds_group_sent;
extern uint8_t rds_block_sent;
extern struct ste_fm_rds_buf_s\
	ste_fm_rds_buf[MAX_RDS_BUFFER][MAX_RDS_GROUPS_READ];

/**
 * ste_fm_init()- Initializes FM Radio.
 * Initializes the Variables and structures required for FM Driver.
 * It also registers the callback to receive the events for command
 * completion, etc
 *
 * Returns:
 *   STE_STATUS_OK,  if Initialization successful
 *   STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_init(void);

/**
 * ste_fm_deinit()- De-initializes FM Radio.
 * De-initializes the Variables and structures required for FM Driver.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_deinit(void);

/**
 * ste_fm_switch_on()- Start up procedure of the FM radio.
 * @device: Pointer to char device requesting the operation.
 * @freq: Frequency in Hz to be set on the FM Radio.
 * @band: Band To be Set.
 * (0: US/EU, 1: Japan, 2: China, 3: Custom)
 * @grid: Grid specifying Spacing.
 * (0: 50 KHz, 1: 100 KHz, 2: 200 Khz)
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_switch_on(
		struct device *device,
		uint32_t freq,
		uint8_t band,
		uint8_t grid);

/**
 * ste_fm_switch_off()- Switches off FM radio
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_switch_off(void);

/**
 * ste_fm_standby()- Makes the FM Radio Go in Standby mode.
 *
 * The FM Radio memorizes the the last state, i.e. Volume, last
 * tuned station, etc that helps in resuming quickly to previous state.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_standby(void);

/**
 * ste_fm_power_up_from_standby()- Power Up FM Radio from Standby mode.
 * It retruns the FM radio to the same state as it was before
 * going to Standby.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_power_up_from_standby(void);

/**
 * ste_fm_set_grid()- Sets the Grid on the FM Radio.
 * @grid: Grid specifying Spacing.
 * (0: 50 KHz,1: 100 KHz,2: 200 Khz)
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_set_grid(
		uint8_t grid
		);

/**
 * ste_fm_set_band()- Sets the Band on the FM Radio.
 * @band: Band specifying Region.
 * (0: US_EU,1: Japan,2: China,3: Custom)
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_set_band(
		uint8_t band
		);

/**
 * ste_fm_step_up_freq()- Steps Up Current Frequency.
 * Steps up the frequency depending on the Grid
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_step_up_freq(void);

/**
 * ste_fm_step_down_freq()- Steps Down Current Frequency.
 * Steps down the frequency depending on the Grid
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_step_down_freq(void);

/**
 * ste_fm_search_up_freq()- seek Up.
 * Searches the next available station in Upward Direction
 * starting from the Current freq.
 *
 * If the operation is started successfully, the chip will generate the
 * irpt_OperationSucced. interrupt when the operation is completed
 * and will tune to the next available frequency.
 * If no station is found, the chip is still tuned to the original station
 * before starting the search
 * Till the interrupt is received, no more API's should be called
 * except ste_fm_stop_scan
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation started successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_search_up_freq(void);

/**
 * ste_fm_search_down_freq()- seek Down.
 * Searches the next available station in Downward Direction
 * starting from the Current freq.
 *
 * If the operation is started successfully, the chip will generate
 * the irpt_OperationSucced. interrupt when the operation is completed.
 * and will tune to the next available frequency. If no station is found,
 * the chip is still tuned to the original station before starting the search.
 * Till the interrupt is received, no more API's should be called
 * except ste_fm_stop_scan.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation started successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_search_down_freq(void);

/**
 * ste_fm_start_band_scan()- Band Scan.
 *
 * Searches for available Stations in the entire Band starting from
 * current freq.
 * If the operation is started successfully, the chip will generate
 * the irpt_OperationSucced. interrupt when the operation is completed.
 * After completion the chip will still be tuned the original station before
 * starting the Scan. on reception of interrupt, the host should call the API
 * ste_fm_get_scan_result() to retrieve the Stations and corresponding RSSI of
 * stations found in the Band.
 * Till the interrupt is received, no more API's should be called
 * except ste_fm_stop_scan.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation started successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_start_band_scan(void);

/**
 * ste_fm_stop_scan()- Stops an active ongoing seek or Band Scan.
 *
 * If the operation is started successfully, the chip will generate the
 * irpt_OperationSucced interrupt when the operation is completed.
 * Till the interrupt is received, no more API's should be called.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation started successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_stop_scan(void);

/**
 * ste_fm_get_scan_result()- Retreives Band Scan Result
 * Retrieves the Scan Band Results of the stations found and
 * the corressponding RSSI values of the stations.
 * @no_of_scanfreq: (out) Pointer to Number of Stations found
 * during Scanning.
 * @scanfreq: (out) Pointer to Frequency of Stations in Hz
 * found during Scanning.
 * @scanfreq_rssi_level: (out) Pointer to RSSI level of Stations
 * found during Scanning.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_get_scan_result(
		uint16_t *no_of_scanfreq,
		uint32_t *scanfreq,
		uint32_t *scanfreq_rssi_level);

/**
 * ste_fm_increase_volume()- Increases the Analog Out Gain of Chip.
 * Maximum level of Gain is 20.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_increase_volume(void);

/**
 * ste_fm_deccrease_volume()- Decreases the Analog Out Gain of Chip.
 * Minimum level of Gain is 0.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_deccrease_volume(void);

/**
 * ste_fm_set_audio_balance()- Sets Audio Balance.
 * @balance: Audio Balnce to be Set in Percentage.
 * (-100: Right Mute.... 0: Both on.... 100: Left Mute)
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_set_audio_balance(
		int8_t balance
		);

/**
 * ste_fm_set_volume()- Sets the Analog Out Gain of FM Chip.
 * @vol_level: Volume Level to be set on Tuner (0-20).
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_set_volume(
		uint8_t vol_level
		);

/**
 * ste_fm_set_volume()- Gets the currently set Analog Out Gain of FM Chip.
 * @vol_level: (out)Volume Level set on Tuner (0-20).
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_get_volume(
		uint8_t *vol_level
		);

/**
 * ste_fm_set_audio_dac()- Set Audio DAC state on FM chip
 * @state: state: Audio DAC state to be Set.
 * (0: Disabled, 1: Enabled,2: Right Mute, 3: Left Mute)
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_set_audio_dac(
		uint8_t state
		);

/**
 * ste_fm_rds_off()- Disables the RDS decoding algorithm in FM chip
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_rds_off(void);

/**
 * ste_fm_rds_on()- Enables the RDS decoding algorithm in FM chip
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_rds_on(void);

/**
 * ste_fm_get_rds_status()- Retrieves the status whether RDS is enabled or not
 * @enabled: (out) Pointer to the status of RDS
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_get_rds_status(
		bool *enabled
		);

/**
 * ste_fm_mute()- Mutes the Audio output from FM Chip
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_mute(void);

/**
 * ste_fm_unmute()- Unmutes the Audio output from FM Chip
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_unmute(void);

/**
 * ste_fm_get_frequency()- Gets the Curently tuned Frequency on FM Radio
 * @freq: (out) Pointer to Frequency in Hz set on the FM Radio.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_get_frequency(
		uint32_t *freq
		);

/**
 * ste_fm_set_frequency()- Gets the Curently tuned Frequency on FM Radio
 * @new_freq: Frequency in Hz to be set on the FM Radio.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_set_frequency(
		uint32_t new_freq
		);

/**
 * ste_fm_set_frequency()- Gets the Curently tuned Frequency on FM Radio
 * @signal_strength: (out) Pointer to the RSSI level of the currently
 * tuned frequency.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_get_signal_strength(
		uint16_t *signal_strength
		);

/**
 * ste_fm_get_mode()- Gets the mode of the Radio tuner.
 * @cur_mode: (out) Pointer to the current mode set on FM Radio
 * (0: Stereo, 1: Mono, 2: Blending, 3: Switching).
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_get_mode(
		uint8_t *cur_mode
		);

/**
 * ste_fm_set_mode()- Sets the mode on the Radio tuner.
 * @mode: mode to be set on FM Radio
 * (0: Stereo, 1: Mono, 2: Blending, 3: Switching.)
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_set_mode(
		uint8_t mode
		);

/**
 * ste_fm_select_antenna()- Selects the Antenna of the Radio tuner.
 * @antenna: (0: Embedded, 1: Wired.)
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_select_antenna(
		uint8_t antenna
		);

/**
 * ste_fm_get_rssi_threshold()- Gets the rssi threshold currently
 * set on FM radio.
 * @rssi_thresold: (out) Pointer to Current rssi threshold set.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_get_rssi_threshold(
		uint16_t *rssi_thresold
		);

/**
 * ste_fm_set_rssi_threshold()- Sets the rssi threshold to be used during
 * Band Scan and seek Stations
 * @rssi_thresold: rssi threshold to be set.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
uint8_t ste_fm_set_rssi_threshold(
		uint16_t rssi_thresold
		);

/**
 * wake_up_poll_queue()- Wakes up the Task waiting on Poll Queue.
 * This function is called when Scan Band or seek has completed.
 */
void wake_up_poll_queue(void);


#endif /* STE_FM_API_H */
