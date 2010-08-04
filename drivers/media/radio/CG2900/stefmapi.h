/*
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
  * struct ste_fm_rds_buf_t - RDS Group Receiving Structure
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
struct ste_fm_rds_buf_t {
	u16 block1;
	u16 block2;
	u16 block3;
	u16 block4;
	u8 status1;
	u8 status2;
	u8 status3;
	u8 status4;
};

/**
  * struct ste_fm_rds_info_t - RDS Information Structure
  * @rds_head: RDS Queue Head for storing next valid data.
  * @rds_tail: RDS Queue Tail for retreiving next valid data.
  * @rds_group_sent: Number of RDS Groups sent to Application.
  * @rds_block_sent: Number of RDS Blocks sent to Application.
  *
  * Structure for storing the RDS data queue information.
  *
  */
struct ste_fm_rds_info_t {
	u8 rds_head;
	u8 rds_tail;
	u8 rds_group_sent;
	u8 rds_block_sent;
};

/**
 * enum ste_fm_flags_t - FM API Flags
 * @STE_FALSE: Equivalent to boolean false.
 * @STE_TRUE: Equivalent to boolean true.
 * FM API Flags.
 */
enum ste_fm_flags_t {
	STE_FALSE,
	STE_TRUE
};

/**
 * enum ste_fm_status_t - Status codes returned by FM API Layer.
 * @STE_STATUS_OK: No Error.
 * @STE_STATUS_SYSTEM_ERROR: Error occurred in last operation.
 * Various Status codes returned by FM API Layer.
 */
enum ste_fm_status_t {
	STE_STATUS_OK,
	STE_STATUS_SYSTEM_ERROR
};

/**
 * enum ste_fm_state_t - States of FM Driver.
 * @STE_FM_STATE_DEINITIALIZED: FM driver is not initialized.
 * @STE_FM_STATE_INITIALIZED: FM driver is initialized.
 * @STE_FM_STATE_SWITCHED_ON: FM driver is switched on and in active state.
 * @STE_FM_STATE_STAND_BY: FM Radio is switched on but not in active state.
 * Various states of FM Driver.
 */
enum ste_fm_state_t {
	STE_FM_STATE_DEINITIALIZED,
	STE_FM_STATE_INITIALIZED,
	STE_FM_STATE_SWITCHED_ON,
	STE_FM_STATE_STAND_BY
} ;

/**
 * enum fmd_gocmd_t - FM Driver Command state .
 * @STE_FM_IDLE_MODE: FM Radio is in Idle Mode.
 * @STE_FM_RX_MODE: FM Radio is configured in Rx mode.
 * @STE_FM_TX_MODE: FM Radio is configured in Tx mode.
 * Various Modes of the FM Radio.
 */
enum ste_fm_mode_t {
	STE_FM_IDLE_MODE,
	STE_FM_RX_MODE,
	STE_FM_TX_MODE
};

/**
 * enum ste_fm_band_t - Various Frequency band supported.
 * @STE_FM_BAND_US_EU: European / US Band.
 * @STE_FM_BAND_JAPAN: Japan Band.
 * @STE_FM_BAND_CHINA: China Band.
 * @STE_FM_BAND_CUSTOM: Custom Band.
 * Various Frequency band supported.
 */
enum ste_fm_band_t {
	STE_FM_BAND_US_EU,
	STE_FM_BAND_JAPAN,
	STE_FM_BAND_CHINA,
	STE_FM_BAND_CUSTOM
};

/**
 * enum ste_fm_band_t - Various Frequency grids supported.
 * @STE_FM_GRID_50: 50 kHz spacing.
 * @STE_FM_GRID_100: 100 kHz spacing.
 * @STE_FM_GRID_200: 200 kHz spacing.
 * Various Frequency grids supported.
 */
enum ste_fm_grid_t {
	STE_FM_GRID_50,
	STE_FM_GRID_100,
	STE_FM_GRID_200
};

/**
 * enum ste_fm_event_t - Various Events reported by FM API layer.
 * @STE_EVENT_NO_EVENT: No Event.
 * @STE_EVENT_SEARCH_CHANNEL_FOUND: Seek operation is completed.
 * @STE_EVENT_SCAN_CHANNELS_FOUND: Band Scan is completed.
 * @STE_EVENT_BLOCK_SCAN_CHANNELS_FOUND: Block Scan is completed.
 * @STE_EVENT_SCAN_CANCELLED: Scan/Seek is cancelled.
 * Various Events reported by FM API layer.
 */
enum ste_fm_event_t {
	STE_EVENT_NO_EVENT,
	STE_EVENT_SEARCH_CHANNEL_FOUND,
	STE_EVENT_SCAN_CHANNELS_FOUND,
	STE_EVENT_BLOCK_SCAN_CHANNELS_FOUND,
	STE_EVENT_SCAN_CANCELLED
};

/**
 * enum ste_fm_direction_t - Directions used while seek.
 * @STE_DIR_DOWN: Search in downwards direction.
 * @STE_DIR_UP: Search in upwards direction.
 * Directions used while seek.
 */
enum ste_fm_direction_t {
	STE_DIR_DOWN,
	STE_DIR_UP
};

/**
 * enum ste_fm_stereo_mode_t - Stereo Modes.
 * @STE_MODE_MONO: Mono Mode.
 * @STE_MODE_STEREO: Stereo Mode.
 * Stereo Modes.
 */
enum ste_fm_stereo_mode_t {
	STE_MODE_MONO,
	STE_MODE_STEREO
};

#define STE_FM_DEFAULT_RSSI_THRESHOLD			100

#define MAX_RDS_BUFFER					10
#define MAX_RDS_GROUPS					22
#define MIN_ANALOG_VOLUME				0
#define MAX_ANALOG_VOLUME				20
#define NUM_OF_RDS_BLOCKS				4
#define RDS_BLOCK_MASK					0x1C
#define RDS_ERROR_STATUS_MASK			0x03
#define RDS_UPTO_TWO_BITS_CORRECTED	0x01
#define RDS_UPTO_FIVE_BITS_CORRECTED	0x02
#define MAX_RT_SIZE						65
#define MAX_PSN_SIZE						9
#define DEFAULT_CHANNELS_TO_SCAN 		32
#define MAX_CHANNELS_TO_SCAN 			99
#define MAX_CHANNELS_FOR_BLOCK_SCAN 	198

extern u8 global_event;
extern struct ste_fm_rds_buf_t ste_fm_rds_buf[MAX_RDS_BUFFER][MAX_RDS_GROUPS];
extern struct ste_fm_rds_info_t ste_fm_rds_info;

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
u8 ste_fm_init(void);

/**
 * ste_fm_deinit()- De-initializes FM Radio.
 * De-initializes the Variables and structures required for FM Driver.
 *
 * Returns:
 *	 STE_STATUS_OK, if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_deinit(void);

/**
 * ste_fm_switch_on()- Start up procedure of the FM radio.
 * @device: Character device requesting the operation.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_switch_on(
		struct device *device
		);


/**
 * ste_fm_switch_off()- Switches off FM radio
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_switch_off(void);

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
u8 ste_fm_standby(void);

/**
 * ste_fm_power_up_from_standby()- Power Up FM Radio from Standby mode.
 * It retruns the FM radio to the same state as it was before
 * going to Standby.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_power_up_from_standby(void);

/**
 * ste_fm_set_rx_default_settings()- Loads FM Rx Default Settings.
 * @freq: Frequency in Hz to be set on the FM Radio.
 * @band: Band To be Set.
 * (0: US/EU, 1: Japan, 2: China, 3: Custom)
 * @grid: Grid specifying Spacing.
 * (0: 50 KHz, 1: 100 KHz, 2: 200 Khz)
 * @enable_rds: Flag indicating enable or disable rds transmission.
 * @enable_stereo: Flag indicating enable or disable stereo mode.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_set_rx_default_settings(
		u32 freq,
		u8 band,
		u8 grid,
		bool enable_rds,
		bool enable_stereo
		);

/**
 * ste_fm_set_tx_default_settings()- Loads FM Tx Default Settings.
 * @freq: Frequency in Hz to be set on the FM Radio.
 * @band: Band To be Set.
 * (0: US/EU, 1: Japan, 2: China, 3: Custom)
 * @grid: Grid specifying Spacing.
 * (0: 50 KHz, 1: 100 KHz, 2: 200 Khz)
 * @enable_rds: Flag indicating enable or disable rds transmission.
 * @enable_stereo: Flag indicating enable or disable stereo mode.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_set_tx_default_settings(
		u32 freq,
		u8 band,
		u8 grid,
		bool enable_rds,
		bool enable_stereo
		);

/**
 * ste_fm_set_grid()- Sets the Grid on the FM Radio.
 * @grid: Grid specifying Spacing.
 * (0: 50 KHz,1: 100 KHz,2: 200 Khz)
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_set_grid(
		u8 grid
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
u8 ste_fm_set_band(
		u8 band
		);

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
u8 ste_fm_search_up_freq(void);

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
u8 ste_fm_search_down_freq(void);

/**
 * ste_fm_start_band_scan()- Band Scan.
 *
 * Searches for available Stations in the entire Band starting from
 * current freq.
 * If the operation is started successfully, the chip will generate
 * the irpt_OperationSucced. interrupt when the operation is completed.
 * After completion the chip will still be tuned the original station before
 * starting the Scan. on reception of interrupt, the host should call the AP
 * ste_fm_get_scan_result() to retrieve the Stations and corresponding
 * RSSI of stations found in the Band.
 * Till the interrupt is received, no more API's should be called
 * except ste_fm_stop_scan, ste_fm_switch_off, ste_fm_standby and
 * ste_fm_get_frequency.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation started successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_start_band_scan(void);

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
u8 ste_fm_stop_scan(void);

/**
 * ste_fm_get_scan_result()- Retreives Band Scan Result
 * Retrieves the Scan Band Results of the stations found and
 * the corressponding RSSI values of the stations.
 * @num_of_scanfreq: (out) Number of Stations found
 * during Scanning.
 * @scan_freq: (out) Frequency of Stations in Hz
 * found during Scanning.
 * @scan_freq_rssi_level: (out) RSSI level of Stations
 * found during Scanning.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_get_scan_result(
		u16 *num_of_scanfreq,
		u32 *scan_freq,
		u32 *scan_freq_rssi_level
		);

/**
 * ste_fm_start_block_scan()- Block Scan.
 *
 * Searches for RSSI level of all the channels between the start and stop
 * channels. If the operation is started successfully, the chip will generate
 * the irpt_OperationSucced interrupt when the operation is completed.
 * After completion the chip will still be tuned the original station before
 * starting the Scan. On reception of interrupt, the host should call the AP
 * ste_fm_get_block_scan_result() to retrieve the RSSI of channels.
 * Till the interrupt is received, no more API's should be called from Host
 * except ste_fm_stop_scan, ste_fm_switch_off, ste_fm_standby and
 * ste_fm_get_frequency.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation started successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_start_block_scan(void);

/**
 * ste_fm_get_scan_result()- Retreives Band Scan Result
 * Retrieves the Scan Band Results of the stations found and
 * the corressponding RSSI values of the stations.
 * @num_of_scanchan: (out) Number of Stations found
 * during Scanning.
 * @scan_freq_rssi_level: (out) RSSI level of Stations
 * found during Scanning.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_get_block_scan_result(
		u16 *num_of_scanchan,
		u16 *scan_freq_rssi_level
		);

/**
 * ste_fm_tx_get_rds_deviation()- Gets RDS Deviation.
 * Retrieves the RDS Deviation level set for FM Tx.
 * @deviation: (out) Rds Deviation.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_get_rds_deviation(
		u16 *deviation
		);

/**
 * ste_fm_tx_set_rds_deviation()- Sets RDS Deviation.
 * Sets the RDS Deviation level on FM Tx.
 * @deviation: Rds Deviation to set on FM Tx.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_set_rds_deviation(
		u16 deviation
		);

/**
 * ste_fm_tx_set_pi_code()- Sets PI code for RDS Transmission.
 * Sets the Program Identification code to be transmitted.
 * @pi_code: PI code to be transmitted.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_set_pi_code(
		u16 pi_code
		);

/**
 * ste_fm_tx_set_pty_code()- Sets PTY code for RDS Transmission.
 * Sets the Program Type code to be transmitted.
 * @pty_code: PTY code to be transmitted.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_set_pty_code(
		u16 pty_code
		);

/**
 * ste_fm_tx_set_program_station_name()- Sets PSN for RDS Transmission.
 * Sets the Program Station Name to be transmitted.
 * @psn: Program Station Name to be transmitted.
 * @len: Length of Program Station Name to be transmitted.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_set_program_station_name(
		char *psn,
		u8 len
		);

/**
 * ste_fm_tx_set_radio_text()- Sets RT for RDS Transmission.
 * Sets the radio text to be transmitted.
 * @rt: Radio Text to be transmitted.
 * @len: Length of Radio Text to be transmitted.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_set_radio_text(
		char *rt,
		u8 len
		);

/**
 * ste_fm_tx_get_rds_deviation()- Gets Pilot Tone status
 * Gets the current status of pilot tone for FM Tx.
 * @enable: (out) Flag indicating Pilot Tone is enabled or disabled.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_get_pilot_tone_status(
		bool *enable
		);

/**
 * ste_fm_tx_set_pilot_tone_status()- Enables/Disables Pilot Tone.
 * Enables or disables the pilot tone for FM Tx.
 * @enable: Flag indicating enabling or disabling Pilot Tone.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_set_pilot_tone_status(
		bool enable
		);

/**
 * ste_fm_tx_get_pilot_deviation()- Gets Pilot Deviation.
 * Retrieves the Pilot Tone Deviation level set for FM Tx.
 * @deviation: (out) Pilot Tone Deviation.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_get_pilot_deviation(
		u16 *deviation
		);

/**
 * ste_fm_tx_set_pilot_deviation()- Sets Pilot Deviation.
 * Sets the Pilot Tone Deviation level on FM Tx.
 * @deviation: Pilot Tone Deviation to set.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_set_pilot_deviation(
		u16 deviation
		);

/**
 * ste_fm_tx_get_preemphasis()- Gets Pre-emhasis level.
 * Retrieves the Preemphasis level set for FM Tx.
 * @preemphasis: (out) Preemphasis level.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_get_preemphasis(
		u8 *preemphasis
		);

/**
 * ste_fm_tx_set_preemphasis()- Sets Pre-emhasis level.
 * Sets the Preemphasis level on FM Tx.
 * @preemphasis: Preemphasis level.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_set_preemphasis(
		u8 preemphasis
		);

/**
 * ste_fm_tx_get_power_level()- Gets Power level.
 * Retrieves the Power level set for FM Tx.
 * @power_level: (out) Power level.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_get_power_level(
		u16 *power_level
		);

/**
 * ste_fm_tx_set_power_level()- Sets Power level.
 * Sets the Power level for FM Tx.
 * @power_level: Power level.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_set_power_level(
		u16 power_level
		);

/**
 * ste_fm_tx_rds()- Enable or disable Tx RDS.
 * Enable or disable RDS transmission.
 * @enable_rds: Flag indicating enabling or disabling RDS.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_tx_rds(
		bool enable_rds
		);

/**
 * ste_fm_set_audio_balance()- Sets Audio Balance.
 * @balance: Audio Balnce to be Set in Percentage.
 * (-100: Right Mute.... 0: Both on.... 100: Left Mute)
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_set_audio_balance(
		s8 balance
		);

/**
 * ste_fm_set_volume()- Sets the Analog Out Gain of FM Chip.
 * @vol_level: Volume Level to be set on Tuner (0-20).
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_set_volume(
		u8 vol_level
		);

/**
 * ste_fm_get_volume()- Gets the currently set Analog Out Gain of FM Chip.
 * @vol_level: (out)Volume Level set on Tuner (0-20).
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_get_volume(
		u8 *vol_level
		);

/**
 * ste_fm_rds_off()- Disables the RDS decoding algorithm in FM chip
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_rds_off(void);

/**
 * ste_fm_rds_on()- Enables the RDS decoding algorithm in FM chip
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_rds_on(void);

/**
 * ste_fm_get_rds_status()- Retrieves the status whether RDS is enabled or not
 * @rds_status: (out) Status of RDS
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_get_rds_status(
		bool *rds_status
		);

/**
 * ste_fm_mute()- Mutes the Audio output from FM Chip
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_mute(void);

/**
 * ste_fm_unmute()- Unmutes the Audio output from FM Chip
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_unmute(void);

/**
 * ste_fm_get_frequency()- Gets the Curently tuned Frequency on FM Radio
 * @freq: (out) Frequency in Hz set on the FM Radio.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_get_frequency(
		u32 *freq
		);

/**
 * ste_fm_set_frequency()- Sets the frequency on FM Radio
 * @new_freq: Frequency in Hz to be set on the FM Radio.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_set_frequency(
		u32 new_freq
		);

/**
 * ste_fm_get_signal_strength()- Gets the RSSI level.
 * @signal_strength: (out) RSSI level of the currently
 * tuned frequency.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_get_signal_strength(
		u16 *signal_strength
		);

/**
 * ste_fm_get_af_updat()- Retrives results of AF Update
 * @af_update_rssi: (out) RSSI level of the Alternative frequency.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_af_update_get_result(
			u16 *af_update_rssi
			);


/**
 * ste_fm_af_update_start()- PErforms AF Update.
 * @af_freq: AF frequency in Hz whose RSSI is to be retrived.
 * tuned frequency.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */

u8 ste_fm_af_update_start(
			u32 af_freq
			);

/**
 * ste_fm_af_switch_get_result()- Retrives the AF switch result.
 * @af_switch_conclusion: (out) Conclusion of the AF Switch.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_af_switch_get_result(
			u16 *af_switch_conclusion
			);

/**
 * ste_fm_af_switch_start()- PErforms AF switch.
 * @af_switch_freq: Alternate Frequency in Hz to be switched.
 * @af_switch_pi: picode of the Alternative frequency.
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_af_switch_start(
		u32 af_switch_freq,
		u16 af_switch_pi
		);

/**
 * ste_fm_get_mode()- Gets the mode of the Radio tuner.
 * @cur_mode: (out) Current mode set on FM Radio
 * (0: Stereo, 1: Mono, 2: Blending, 3: Switching).
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_get_mode(
		u8 *cur_mode
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
u8 ste_fm_set_mode(
		u8 mode
		);

/**
 * ste_fm_select_antenna()- Selects the Antenna of the Radio tuner.
 * @antenna: (0: Embedded, 1: Wired.)
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_select_antenna(
		u8 antenna
		);

/**
 * ste_fm_get_antenna()- Retreives the currently selected antenna.
 * @antenna: out (0: Embedded, 1: Wired.)
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_get_antenna(
		u8 *antenna
		);

/**
 * ste_fm_get_rssi_threshold()- Gets the rssi threshold currently
 * set on FM radio.
 * @rssi_thresold: (out) Current rssi threshold set.
 *
 * Returns:
 *	 STE_STATUS_OK,  if operation completed successfully.
 *	 STE_STATUS_SYSTEM_ERROR, otherwise.
 */
u8 ste_fm_get_rssi_threshold(
		u16 *rssi_thresold
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
u8 ste_fm_set_rssi_threshold(
		u16 rssi_thresold
		);

/**
 * wake_up_poll_queue()- Wakes up the Task waiting on Poll Queue.
 * This function is called when Scan Band or seek has completed.
 */
void wake_up_poll_queue(void);

/**
 * void wake_up_read_queue()- Wakes up the Task waiting on Read Queue.
 * This function is called when RDS data is available for reading by
 * application.
 */
void wake_up_read_queue(void);
#endif /* STE_FM_API_H */
