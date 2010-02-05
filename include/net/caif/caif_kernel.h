/*
 * CAIF Kernel Internal interface for configuring and accessing
 * CAIF Channels.
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/ sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CAIF_KERNEL_H_
#define CAIF_KERNEL_H_
#include <linux/caif/caif_config.h>
struct sk_buff;

/*!\page  caif_kernel.h
 * This is the specification of the CAIF kernel internal interface to
 * CAIF Channels.
 * This interface follows the pattern used in Linux device drivers with a
 *  struct \ref caif_device
 * holding control data handling each device instance.
 *
 * The functional interface consists of a few basic functions:
 *  - \ref caif_add_device             Configure and connect the CAIF
 *               channel to the remote end. Configuration is described in
 *               \ref caif_channel_config.
 *  - \ref caif_remove_device          Disconnect and remove the channel.
 *  - \ref caif_transmit               Sends a CAIF message on the link.
 *  - \ref caif_device.receive_cb      Receive callback function for
 *                receiving packets.
 *  - \ref caif_device.control_cb      Control information from the CAIF stack.
 *  - \ref caif_flow_control           Send flow control message to remote end.
 *
 *
 * Details:
 * \see { caif_kernel }
 *
 * \code
 *
#include <net/caif/caif_kernel.h>"

	static void my_receive(struct caif_device *dev, struct sk_buff *skb)
	{
	...
	}

	static void my_control(struct caif_device *dev, enum caif_control ctrl)
	{
	....
	}

	int kernel_caif_usage_example()
	{
		struct sk_buff *skb;
		char *message = "hello";

		// Connect the channel
		struct caif_device caif_dev = {
			.caif_config = {
				.name = "MYDEV",
				.priority = CAIF_PRIO_NORMAL,
				.type = CAIF_CHTY_UTILITY,
				.phy_pref = CAIF_PHYPREF_LOW_LAT,
				.u.utility.name = "CAIF_PSOCK_TEST",
				.u.utility.params = {0x01},
				.u.utility.paramlen = 1,
		},

		.receive_cb = my_receive,
		.control_cb = my_control,

		};
	ret = caif_add_device(&caif_dev);
	if (ret)
	goto error;

	// Send a packet
	skb = caif_create_skb(message, strlen(message));
	ret = caif_transmit(&caif_dev, skb);
	if (ret)
		goto error;

	// Remove device
	ret = caif_remove_device(&caif_dev);
	if (ret)
		goto error;

}

* \endcode
*
* \section Linux Socket Buffer (SKB)
    *          When sending out packets on a connection (\ref caif_transmit)
    *          the CAIF stack will add CAIF protocol headers.
    *          This requires space in the SKB.
    *          CAIF has defined \ref CAIF_SKB_HEAD_RESERVE for minimum
    *          required reserved head-space in the packet and
    *          \ref CAIF_SKB_TAIL_RESERVE for minimum reserved tail-space.
    *
    *          \b NOTE The Linux kernel SKB operations panic if not
    *                  enough space is available!
    *
    */

    /*! \addtogroup caif_kernel
     *  @{
     */

struct caif_device;

	/** Minimum required CAIF socket buffer head-space */
#define CAIF_SKB_HEAD_RESERVE 32

	/** Minimum required CAIF socket buffer tail-space */
#define CAIF_SKB_TAIL_RESERVE 32

	/** CAIF control information (used in \ref caif_device.control_cb)
	 *   used for receiving control information from the modem.
	 */
enum caif_control {
	/** Modem has sent Flow-ON, Clients can start transmitting
	 *  data using \ref caif_transmit.
	 */
	CAIF_CONTROL_FLOW_ON = 0,
	/** Modem has sent Flow-OFF, Clients must stop transmitting
	 * data using \ref caif_transmit.
	 */
	CAIF_CONTROL_FLOW_OFF = 1,

	/** Channel creation is complete. This is an acknowledgement to
	 *  \ref caif_add_device from the modem.
	 *  The channel is ready for transmit (Flow-state is ON).
	 */
	CAIF_CONTROL_DEV_INIT = 3,

	/** Spontaneous close request from the modem, only applicable
	 *   for utility link. The client should respond by calling
	 *   \ref caif_remove_device.
	 */
	CAIF_CONTROL_REMOTE_SHUTDOWN = 4,

	/** Channel disconnect is complete. This is an acknowledgement to
	 *  \ref caif_remove_device from the modem.
	 *  \ref caif_transmit or \ref caif_flow_control must not be
	 *  called after this.
	 */
	CAIF_CONTROL_DEV_DEINIT = 5,

	/** Channel creation has failed. This is a negative acknowledgement
	 *  to \ref caif_add_device from the modem.
	 */
	CAIF_CONTROL_DEV_INIT_FAILED = 6
};

/** Flow control information (used in \ref caif_device.control_cb) used
 *  for controlling outgoing flow.
 */
enum caif_flowctrl {
	/** Flow Control is ON, transmit function can start sending data */
	CAIF_FLOWCTRL_ON = 0,
	/** Flow Control is OFF, transmit function should stop sending data */
	CAIF_FLOWCTRL_OFF = 1,
};

/** Transmits CAIF packets on channel.
 * This function is non-blocking and safe to use in tasklet context.
 * The CAIF stack takes ownership of the socket buffer (SKB) after calling
 * \ref caif_transmit.
 * This means that the user cannot access the SKB afterwards; this applies
 * even in error situations.
 *
 * @return 0 on success, < 0 upon error.
 *
 * @param[in] skb         Socket buffer holding data to be written.
 * @param[in] dev         Structure used when creating the channel
 *
 *
 * Error codes:
 *  - \b ENOTCONN,   The channel is not connected.
 *  - \b EPROTO,     Protocol error (or SKB is faulty)
 *  - \b EIO         IO error (unspecified error)
 */
int caif_transmit(struct caif_device *dev, struct sk_buff *skb);

/** Function for sending flow ON / OFF to remote end.
 * This function is non-blocking and safe to use in tasklet context.
 *
 * @param[in] dev        Reference to device data.
 * @param[in] flow       Flow control information.

 * @return 0 on success, < 0 upon error.
 * Error codes:
 *  - \b ENOTCONN,   The channel is not connected.
 *  - \b EPROTO,     Protocol error.
 *  - \b EIO         IO error (unspecified error).
 */
int caif_flow_control(struct caif_device *dev, enum caif_flowctrl flow);

/** Handle for kernel internal CAIF channels.
 * All fields in this structure must be filled in by client before calling
 * \ref caif_add_device (except _caif_handle).
 */
struct caif_device {

    /** Channel configuration parameter. Contains information about type
     *	and configuration of the channel.
     *  This must be set before calling \ref caif_add_device.
     */
	struct caif_channel_config caif_config;


    /** Callback function for receiving CAIF Packets from channel.
     * This callback is called from softirq context (tasklet).
     * The receiver <b> must </b> free the SKB.
     * <b> DO NOT BLOCK IN THIS FUNCTION! </b>
     *
     * If the client has to do blocking operations then
     * it must start its own work queue (or kernel thread).
     *
     * @param[in] dev       Reference to device data.
     * @param[in] skb       Socket buffer with received data.
     */
	void (*receive_cb) (struct caif_device *dev, struct sk_buff *skb);


    /** Callback function for notifying flow control from remote end - see
     *  \ref caif_control.
     * This callback is called from from softirq context (tasklet).
     *
     * <b> DO NOT BLOCK IN THIS FUNCTION! </b>
     *
     * Client must not call \ref caif_transmit from this function.
     *
     * If the client has queued packets to send then
     * it must start its own thread to do \ref caif_transmit.
     *
     * @param[in] dev        Reference to device data.
     * @param[in] ctrl       CAIF control info \ref caif_control.
     *                       e.g. Flow control
     *                       \ref CAIF_CONTROL_FLOW_ON or
     *                       \ref CAIF_CONTROL_FLOW_OFF
     */
	void (*control_cb) (struct caif_device *dev, enum caif_control ctrl);

    /** This is a CAIF private attribute, holding CAIF internal reference
     * to the CAIF stack. Do not update this field.
     */
	void *_caif_handle;

    /** This field may be filled in by client for their own usage. */
	void *user_data;
};

/** Add (connect) a CAIF Channel.
 * This function is non-blocking. The channel connect is reported in
 * \ref caif_device.control_cb.
 * The channel is not open until \ref caif_device.control_cb is called with
 * \ref CAIF_CONTROL_DEV_INIT.
 * If setting up the channel fails then \ref caif_device.control_cb is called
 * with \ref CAIF_CONTROL_DEV_INIT_FAILED.
 *
 * \ref caif_transmit, \ref caif_flow_control or \ref caif_remove_device must
 * not be called before receiveing CAIF_CONTROL_DEV_INIT.
 * @return 0 on success, < 0 on failure.
 *
 * Error codes:
 * - \b -EINVAL    Invalid arguments
 * - \b -ENODEV    No PHY device exists.
 * - \b -EIO       IO error (unspecified error)
 */
int caif_add_device(struct caif_device *dev);

/** Disconnect a CAIF Channel
 * This function is non-blocking.
 * The channel has not been disconnected until \ref caif_device : control_cb is
 * called with \ref CAIF_CONTROL_DEV_DEINIT.
 * \ref caif_transmit or \ref caif_flow_control \b must not be called after
 * receiving \ref CAIF_CONTROL_DEV_DEINIT.
 * The client is responsible for freeing the \ref caif_device structure after
 * receiving  \ref CAIF_CONTROL_DEV_DEINIT (if applicable).
 * @return 0 on success.
 *
 * - \b EIO       IO error (unspecified error)
 */
int caif_remove_device(struct caif_device *caif_dev);

/** Convenience function for allocating a socket buffer for usage with CAIF
 * and copy user data into the socket buffer.
 * @param[in] data 		User data to send with CAIF.
 * @param[in] data_length 	Length of data to send.
 * @return 	New socket buffer containing user data.
 */
struct sk_buff *caif_create_skb(unsigned char *data, unsigned int data_length);

/** Convenience function for extracting data from a socket buffer (SKB) and
 *  then destroying the SKB.
 *  Copies data from the SKB and then frees the SKB.
 * @param[in] skb	SKB to extract data from. SKB will be freed after
 *			extracting data.
 *
 * @param[in] data	User data buffer to extract packet data into.
 * @param[in] max_length User data buffer length,
 * @return number of bytes extracted; < 0 upon error.
 *
 */
int caif_extract_and_destroy_skb(struct sk_buff *skb, unsigned char *data,
				 unsigned int max_length);

/*! @} */

#endif				/* CAIF_KERNEL_H_ */
