
// $Copyright  $
/*****************************************************************************
* DESCRIPTION:
*****************************************************************************/
//name  irrc_ioctl.h



/*!\page  irrc_ioctl.h
 * This file defines the ioctl interface to irrc.
 *
 *\b Example - set modulation parameters:
 * \code
   fd = open("/dev/irrc",..);
   struct irrc_pulse_data irrc_config = {
	.carrier_data = {
		.carrier_high = 105,
		.carrier_low = 105
		}

	}
	.logical_pulse_data = {
		.modulation_method = HAL_IRRC_MODULATION_METHOD_HIGHLOW_HIGHLOW,
		.data0high = 200,
		.data0low = 300,
		.data1high = 400,
		.data1low = 450
	}
	.start_stop_pulse_data = {
		.start_high = 500,
		.start_low = 500,
		.stop_high = 600
	}
	.repeat_data = {
		.count = 1,	// single transmission
//		.interval = 0 	// no need to set interval because no retransmission is asked for (Count = 1).
	}
   };

   ioctl(fd,IRRC_IOC_CONFIGURE_PULSE_PARAM,&irrc_config);
   close(fd);
 * \endcode
 *
*/

#ifndef INCLUSION_GUARD_IRRC_H
#define INCLUSION_GUARD_IRRC_H

/*****************************************************************************
* REVISION HISTORY
*****************************************************************************/



/****************************************************************************
* Include
*****************************************************************************/
#include <linux/ioctl.h>		// _IOW(....)



/****************************************************************************
* Defines
*****************************************************************************/

/* MAX number of bytes in an IRRC frame */
#define IRRC_COMMAND_DATA_BUFFER_MAX_SIZE 128

/****************************************************************************
* Global Variables
*****************************************************************************/



/****************************************************************************
* Types
*****************************************************************************/

/**
 * This enumerated type defines the different modulation methods
 * when sending data using the infrared remote control.
 *
 * @param HAL_IRRC_MODULATION_METHOD_HIGHLOW_HIGHLOW
 *        A logical 0 and a logical 1 is expressed by a high-to-low transition both
 *        expressed by a pulse burst followed by a gap (HighLow). The length of pulse
 *        and gap makes a distinguish between a logical 0 and 1.
 *        Usage Example 1: pulse-distance modulation
 *        Uses fixed length pulses while gap defines whether it is “1”(long gap) or
 *        “0”(short gap) logical value. Protocol example: Sharp
 *        Usage Example 2: pulse-duration modulation
 *        Uses fixed gap length while pulses length defines whether it is “1”(long
 *        pulse width) or “0”(short pulse width) logical value. Protocol example: SONY SIRC.
 *
 * @param HAL_IRRC_MODULATION_METHOD_LOWHIGH_HIGHLOW
 *        A logical 0 is expressed by a low-to-high transition. A logical 1 is expressed
 *        by a high-to-low transition. Each bit contains pulse bursts of the applied
 *        carrier frequency. A logical 0 is represented by a pulse burst in the second
 *        half of the bit time. A logical 1 is represented by a pulse burst in the first
 *        half of the bit time.
 *        Usage example: Bi-phase modulation or so-called Manchester modulation
 *        (according to G.E. Thomas' convention).
 *        Each bit is transmitted in a fixed time (the "pulse period" or "window").
 *        These time windows have constant length and a change of signal's level inside
 *        of each time window is used for detection of each bit. If there is a positive
 *        change (from log. 0 to log. 1) the bit is evaluated as logical one. In opposite
 *        case, when there is negative change, the bit is evaluated as logical zero.
 *        Protocol example: RC-6 from Philips.
 *
 * @param HAL_IRRC_MODULATION_METHOD_HIGHLOW_LOWHIGH
 *        A logical 0 is expressed by a high-to-low transition and a logical 1 is expressed
 *        by a low-to-high transition. Each bit contains burst of pulses of the applied
 *        carrier frequency. A logical 0 is represented by a pulse burst in the first half
 *        of the bit time. A logical 1 is represented by a pulse burst in the second half
 *        of the bit time.
 *        Usage Example: Bi-phase modulation or so-called Manchester coding (according to
 *        IEEE 802.3 convention).
 *        This is the inverse convention of modulation LowHigh HighLow. Each bit is
 *        transmitted in a fixed time (the "period"). Protocol example: RC-5 from Philips.
 *
 * @param HAL_IRRC_MODULATION_METHOD_LOWHIGH_LOWHIGH
 *        A logical 0 and a logical 1 is expressed by a low-to-high transition both expressed
 *        by a gap followed by a pulse burst (LowHigh). The pulse length and gap length makes
 *        a distinguish between a logical 0 and 1.
 *        Usage Example: Inverse convention of modulation HighLow HighLow.
 */
enum irrc_modulation_method {
	HAL_IRRC_MODULATION_METHOD_HIGHLOW_HIGHLOW,
	HAL_IRRC_MODULATION_METHOD_LOWHIGH_HIGHLOW,
	HAL_IRRC_MODULATION_METHOD_HIGHLOW_LOWHIGH,
	HAL_IRRC_MODULATION_METHOD_LOWHIGH_LOWHIGH
};


/**
 * This type stores the carrier part of the infrared remote control pulse data.
 * The period of the carrier frequency is sum CarrierHigh and CarrierLow.
 * The duty cycle of the carrier frequency is calculated as CarrierHigh divided by
 * sum parameter CarrierHigh and parameter CarrierLow.
 *
 * @param CarrierHigh  The time interval (in nanoseconds) for the carrier high pulse.
 * @param CarrierLow   The time interval (in nanoseconds) for the carrier low pulse.
 */
struct irrc_carrier_data {
	unsigned long int carrier_high;
	unsigned long int carrier_low;
};


/**
 * This type stores the logical pulse part of the infrared remote control pulse data.
 *
 * @param ModulationMethod  The modulation method.
 * @param Data0High         The time interval (in nanoseconds) for data 0 high pulse.
 * @param Data0Low          The time interval (in nanoseconds) for data 0 low pulse.
 * @param Data1High         The time interval (in nanoseconds) for data 1 high pulse.
 * @param Data1Low          The time interval (in nanoseconds) for data 1 low pulse.
 */
struct irrc_logical_pulse_data {
	enum irrc_modulation_method modulation_method;
	unsigned long int data0high;
	unsigned long int data0low;
	unsigned long int data1high;
	unsigned long int data1low;
};


/**
 * This type stores the start and stop pulse part of the infrared remote control pulse data.
 *
 * @param StartHigh  		The time interval (in nanoseconds) for start bit high pulse.
 * @param StartLow   		The time interval (in nanoseconds) for start bit low pulse.
 * @param StopHigh   		The time interval (in nanoseconds) for stop bit high pulse.
 * @param frame_duration	The compleate frame duration (in nanoseconds) including start, stop and data pulses.
 */
struct irrc_start_stop_pulse_data {
	unsigned long int start_high;
	unsigned long int start_low;
	unsigned long int stop_high;
//	unsigned long int frame_duration;
};


/**
 * This type stores the repeat data part of the infrared remote control pulse data.
 *
 * @param Count     The number of repeated transmissions. A value of 0 means a single transmission
 *                  with no retransmissions.
 * @param Interval  The time interval (in milliseconds) between each start of retransmission
 *                  if Count is greater than 0.
 */
struct irrc_repeat_data {
	unsigned long int count;
	unsigned long int interval;
};


/**
 * This type stores the PULSE data part of the infrared remote control data.
 *
 * @param CarrierData         The carrier data.
 * @param LogicalPulseData    The logical pulse data.
 * @param StartStopPulseData  The start and stop pulse data.
 * @param RepeatData          The repeat data.
 */
struct irrc_pulse_data {
	struct irrc_carrier_data          carrier_data;
	struct irrc_logical_pulse_data    logical_pulse_data;
	struct irrc_start_stop_pulse_data start_stop_pulse_data;
	struct irrc_repeat_data           repeat_data;
};


/**
 * This type stores the COMMAND data part of the infrared remote control data.
 * The data bits are stored in a byte array, where the HIGHEST bit in a byte
 * is the first bit to be transmitted.
 *
 * @param size  The size of the command data in number of BITS.
 * @param data  The command data.
 */
struct irrc_cmd_data {
	unsigned long 	size;
	unsigned char 	data[IRRC_COMMAND_DATA_BUFFER_MAX_SIZE];
};

struct irrc_ioctl {
	struct irrc_cmd_data 	cmd_data;
	struct irrc_pulse_data 	pulse_data;
};


/****************************************************************************
* Defines
*****************************************************************************/

/** Use '0xF5' as magic number. '0xF5' seems not to be occupied in Documentation/ioctl/ioctl-number.txt*/
#define IRRC_IOC_MAGIC '0xF5'

/** Set command to specifying the shape of the remote control pulses  */
#define IRRC_IOC_CONFIGURE_PULSE_PARAM 	_IOW(IRRC_IOC_MAGIC, 1, struct irrc_ioctl)

/****************************************************************************
* Function Prototypes
*****************************************************************************/


#endif // INCLUSION_GUARD_IRRC_H


