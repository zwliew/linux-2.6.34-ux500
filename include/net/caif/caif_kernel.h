/*
 * CAIF Kernel Internal interface for configuring and accessing
 * CAIF Channels.
 * Copyright (C) ST-Ericsson AB 2010
 * Author:	Sjur Brendeland/ sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CAIF_KERNEL_H_
#define CAIF_KERNEL_H_
#include <linux/caif/caif_config.h>
struct sk_buff;

/*
 * This is the specification of the CAIF kernel internal interface to
 * CAIF Channels.
 * This interface follows the pattern used in Linux device drivers with a
 *  struct  caif_device
 * holding control data handling each device instance.
 *
 * The functional interface consists of a few basic functions:
 *  -  caif_add_device		Configure and connect the CAIF
 *     				channel to the remote end. Configuration is
 * 				described in  caif_channel_config.
 *  -  caif_remove_device	Disconnect and remove the channel.
 *  -  caif_transmit		Sends a CAIF message on the link.
 *  -  caif_device.receive_cb	Receive callback function for
 *				receiving packets.
 *  -  caif_device.control_cb	Control information from the CAIF stack.
 *  -  caif_flow_control	Send flow control message to remote end.
 *
 *
 * EXAMPLE:

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

 *
 *  Linux Socket Buffer (SKB)
 *	    When sending out packets on a connection ( caif_transmit)
 *	    the CAIF stack will add CAIF protocol headers.
 *	    This requires space in the SKB.
 *	    CAIF has defined  CAIF_SKB_HEAD_RESERVE for minimum
 *	    required reserved head-space in the packet and
 *	    CAIF_SKB_TAIL_RESERVE for minimum reserved tail-space.
 *
 *
 */


struct caif_device;

/*
 * CAIF_SKB_HEAD_RESERVE - Minimum required CAIF socket buffer head-space.
 */
#define CAIF_SKB_HEAD_RESERVE 32

/*
 * CAIF_SKB_TAIL_RESERVE - Minimum required CAIF socket buffer tail-space.
 */
#define CAIF_SKB_TAIL_RESERVE 32

/**
 * enum caif_control - Kernel API: CAIF control information.
 * used for receiving control information from the modem.
 *
 * @CAIF_CONTROL_FLOW_ON:	Modem has sent Flow-ON, Clients can start
 *				transmitting data using caif_transmit.
 *
 * @CAIF_CONTROL_FLOW_OFF:	Modem has sent Flow-OFF, Clients must stop
 *				transmitting data using	 caif_transmit.
 *
 * @CAIF_CONTROL_DEV_INIT:	Channel creation is complete. This is an
 *				acknowledgement to caif_add_device from the
 *				modem. The channel is ready for transmit
 *				(Flow-state is ON).
 * @CAIF_CONTROL_REMOTE_SHUTDOWN:
 *				Spontaneous close request from the modem.
 *				This may be caused by modem requesting
 *				shutdown or CAIF Link Layer going down.
 *				The client should respond by calling
 *				caif_remove_device.
 * @CAIF_CONTROL_DEV_DEINIT:	Channel disconnect is complete. This is an
 *				acknowledgement to caif_remove_device from
 *				the modem. caif_transmit or caif_flow_control
 *				must not be called after this.
 * @CAIF_CONTROL_DEV_INIT_FAILED:
 *				Channel creation has failed. This is a
 *				negative acknowledgement to caif_add_device
 *				from the modem.
 */

enum caif_control {
	CAIF_CONTROL_FLOW_ON = 0,
	CAIF_CONTROL_FLOW_OFF = 1,
	CAIF_CONTROL_DEV_INIT = 3,
	CAIF_CONTROL_REMOTE_SHUTDOWN = 4,
	CAIF_CONTROL_DEV_DEINIT = 5,
	CAIF_CONTROL_DEV_INIT_FAILED = 6
};

/**
 * enum caif_flowctrl -	 Kernel API: Flow control information.
 *
 * @CAIF_FLOWCTRL_ON:	Flow Control is ON, transmit function can start
 *			sending data.
 *
 * @CAIF_FLOWCTRL_OFF:	Flow Control is OFF, transmit function should stop
 *			sending data.
 * Used in  caif_device.control_cb) for controlling outgoing flow.
 */
enum caif_flowctrl {
	CAIF_FLOWCTRL_ON,
	CAIF_FLOWCTRL_OFF
};

/**
 * caif_tranmit() - Kernel API: Transmits CAIF packets on channel.
 *
 * @skb:	 Socket buffer holding data to be written.
 * @dev:	 Structure used when creating the channel
 *
 * This function is non-blocking and safe to use in tasklet context.
 * The CAIF stack takes ownership of the socket buffer (SKB) after calling
 * caif_transmit.
 * This means that the user cannot access the SKB afterwards; this applies
 * even in error situations.
 *
 * returns 0 on success, < 0 upon error.
 *
 * Error codes:
 *  -ENOTCONN:   The channel is not connected.
 *
 *  -EPROTO:	   Protocol error (or SKB is faulty)
 *
 *  -EIO:	   IO error (unspecified error)
 */
int caif_transmit(struct caif_device *dev, struct sk_buff *skb);

/**
 * caif_flow_control() -  Kernel API: Function for sending flow ON/OFF
 *			  to remote end.
 * @dev:	Reference to device data.
 * @flow:	Flow control information.
 *
 * This function is non-blocking and safe to use in tasklet context.
 * It returns 0 on success, < 0 upon error.
 *
 * Error codes:
 * -ENOTCONN:	The channel is not connected.
 *
 * -EPROTO:	Protocol error.
 *
 * -EIO:		IO error (unspecified error).
 */
int caif_flow_control(struct caif_device *dev, enum caif_flowctrl flow);

/**
 * struct caif_device - Kernel API: Handle for kernel internal CAIF channels.
 *
 * @caif_config:	Channel configuration parameter. Contains information
 *			about type and configuration of the channel.
 *			This must be set before calling	 caif_add_device.
 *
 * @receive_cb:		Callback function for receiving CAIF Packets from
 *			channel. This callback is called from softirq
 *			context (tasklet). The receiver MUST free the SKB.
 *			If the client has to do blocking operations then
 *			it must start its own work queue (or kernel thread).
 * @control_cb:		Callback function for notifying flow control from
 *			remote end - see caif_control.
 *			This callback is called from from softirq context
 *			(tasklet). Client must not call caif_transmit from
 *			this function. If the client has queued packets to
 *			send then it must start its own thread to do
 *			caif_transmit.
 *
 * @user_data:		This field may be filled in by client for their
 *			own usage.
 *
 * All fields in this structure must be filled in by client before calling
 * caif_add_device (except _caif_handle).
 *
 */
struct caif_device {
	struct caif_channel_config caif_config;
	void (*receive_cb) (struct caif_device *dev, struct sk_buff *skb);
	void (*control_cb) (struct caif_device *dev, enum caif_control ctrl);
	/* This field may be filled in by client for their own usage. */
	void *user_data;
	void *_caif_handle; /* Internal reference used by CAIF */
};

/**
 * caif_add_device() - Kernel API: Add (connect) a CAIF Channel.
 * @dev: Pointer to caif device.
 *
 * This function is non-blocking. The channel connect is reported in
 * caif_device.control_cb.
 * The channel is not open until  caif_device.control_cb is called with
 * CAIF_CONTROL_DEV_INIT.
 * If setting up the channel fails then	 caif_device.control_cb is called
 * with	 CAIF_CONTROL_DEV_INIT_FAILED.
 *
 * caif_transmit,  caif_flow_control or	 caif_remove_device must
 * not be called before receiving CAIF_CONTROL_DEV_INIT.
 * returns 0 on success, < 0 on failure.
 *
 * Error codes:
 * -EINVAL:	 Invalid arguments
 *
 * -ENODEV:	 No PHY device exists.
 *
 * -EIO:	 IO error (unspecified error)
 */
int caif_add_device(struct caif_device *dev);

/**
 * caif_remove_device() - Kernel API: Disconnect a CAIF Channel.
 * @dev: Pointer to caif device.
 *
 * This function is non-blocking.
 * The channel has not been disconnected until	caif_device : control_cb is
 * called with	CAIF_CONTROL_DEV_DEINIT.
 * caif_transmit or  caif_flow_control	 must not be called after
 * receiving  CAIF_CONTROL_DEV_DEINIT.
 * The client is responsible for freeing the  caif_device structure after
 * receiving   CAIF_CONTROL_DEV_DEINIT (if applicable).
 * returns 0 on success.
 *
 */
int caif_remove_device(struct caif_device *dev);

/**
 * caif_create_skb() - Kerne API: Create SKB for CAIF payload.
 * @data:		User data to send with CAIF.
 * @data_length:	Length of data to send.
 *
 * Convenience function for allocating a socket buffer for usage with CAIF
 * and copy user data into the socket buffer.
 */
struct sk_buff *caif_create_skb(unsigned char *data, unsigned int data_length);

/**
 * caif_extrace_and_destroy_skb() - Kernel API: Extract and destroy skb.
 * @skb:	SKB to extract data from. SKB will be freed after
 *			extracting data.
 *
 * @data:	User data buffer to extract packet data into.
 * @max_length: User data buffer length,
 *
 * Convenience function for extracting data from a socket buffer (SKB) and
 * then destroying the SKB.
 * Copies data from the SKB and then frees the SKB.
 *
 * returns number of bytes extracted; < 0 upon error.
 *
 */
int caif_extract_and_destroy_skb(struct sk_buff *skb, unsigned char *data,
				 unsigned int max_length);

#endif				/* CAIF_KERNEL_H_ */
