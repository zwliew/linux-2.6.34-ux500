/*
 * CAIF Channel Configuration definitions.
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/ sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CAIF_CONFIG_H_
#define CAIF_CONFIG_H_

/**
 * enum caif_phy_preference  -	Types of physical HW interfaces
 *				towards modem defined in CAIF stack
 * @CAIF_PHYPREF_UNSPECIFIED:	Default physical interface
 * @CAIF_PHYPREF_LOW_LAT:	Default physical interface for low-latency
 *				traffic
 * @CAIF_PHYPREF_HIGH_BW:	Default physical interface for high-bandwidth
 *				traffic
 * @CAIF_PHYPREF_LOOP:		TEST Loopback interface, simulating modem
 *				responses
 *
 * For client convenience, two special types are defined:
 * CAIF_PHYPREF_LOW_LAT is the preferred low-latency physical link.
 * Typically used for "control" purposes.
 * CAIF_PHYPREF_HIGH_BW is the preferred high-bandwidth physical link.
 * Typically used for "payload" purposes.
 */
enum caif_phy_preference {
	CAIF_PHYPREF_UNSPECIFIED,
	CAIF_PHYPREF_LOW_LAT,
	CAIF_PHYPREF_HIGH_BW,
	CAIF_PHYPREF_LOOP
};

/*
 * Define CAIF channel priority.
 * Used when setting up a channel to specify the
 * priority level of the channel.
 */
#define CAIF_PRIO_MIN	       0x01	/* Min priority for a channel */
#define CAIF_PRIO_LOW	       0x04	/* low-priority channel. */
#define CAIF_PRIO_NORMAL       0x0f	/* normal/default priority level. */
#define CAIF_PRIO_HIGH	       0x14	/* high priority level */
#define CAIF_PRIO_MAX	       0x1F	/* Max priority for channel */


/**
 * enum caif_channel_type - Types of CAIF channel type defined in CAIF Stack.
 * @CAIF_CHTY_AT:		Classical AT
 * @CAIF_CHTY_AT_CTRL:		AT control only
 * @CAIF_CHTY_AT_PAIRED:	Paired control and data
 * @CAIF_CHTY_DATAGRAM:		Datagram. Requires: connection_id
 * @CAIF_CHTY_DATAGRAM_LOOP:	Datagram loopback (testing purposes only)
 * @CAIF_CHTY_VIDEO:		Video channel
 * @CAIF_CHTY_DEBUG:		Debug service (Debug server and
 *					       interactive debug)
 * @CAIF_CHTY_DEBUG_TRACE:	Debug server only
 * @CAIF_CHTY_DEBUG_INTERACT:	Debug interactive
 * @CAIF_CHTY_RFM:		RFM service. Params: connection_id, volume
 * @CAIF_CHTY_UTILITY:		Utility (Psock) service.
 *				Params: fifo_kb,fifo_pkt, name, psock_param
 *
 * This is used for channel configuration, specifying the type of channel.
 */
enum caif_channel_type {
	CAIF_CHTY_AT,
	CAIF_CHTY_AT_CTRL,
	CAIF_CHTY_AT_PAIRED,
	CAIF_CHTY_DATAGRAM,
	CAIF_CHTY_DATAGRAM_LOOP,
	CAIF_CHTY_VIDEO,
	CAIF_CHTY_DEBUG,
	CAIF_CHTY_DEBUG_TRACE,
	CAIF_CHTY_DEBUG_INTERACT,
	CAIF_CHTY_RFM,
	CAIF_CHTY_UTILITY
};

/**
 *struct caif_channel_config - This structure is used for configuring
 *			     CAIF channels.
 * @name:		     Mandatory - Nickname for this device
 * @type:		     Mandatory - Define the type of caif service
 * @priority:		     Mandatory - Value between	CAIF_PRIO_MIN and
 *			     CAIF_PRIO_MAX.
 *			     CAIF_PRIO_LOW, CAIF_PRIO_NORMAL, CAIF_PRIO_HIGH
 *			     are suggested values.
 * @phy_pref:		     Either: Specify type of physical interface to use.
 * @phy_name:		     Or: Specify identity of the physical interface.
 *
 * @u:			     Union of channel type-specific configuration
 *			     parameters
 *
 * @u.dgm:		     CAIF_CHTYPE_DATAGRAM
 * @u.dgm.connection_id:     Mandatory - Connection ID must be specified.
 *
 * @u.video:		     CAIF_CHTYPE_VIDEO
 * @u.video.connection_id:   Mandatory - Connection ID must be specified.
 *
 * @u.rfm		     CAIF_CHTYPE_RFM
 * @u.rfm.connection_id:     Mandatory - Connection ID must be specified.
 * @u.rfm.volume:	     Mandatory - Volume to mount.
 *
 * @u.utility:		     CAIF_CHTYPE_UTILITY
 * @u.utility.fifosize_kb:   Psock: FIFO size in KB
 * @u.utility.fifosize_bufs: Psock: Number of signal buffers
 * @u.utility.name:	     Psock: Name of service
 * @u.utility.params:	     Psock: Channel config parameters
 * @u.utility.paramlen:	     Psock: Length of channel config parameters
 *
 *
 * It holds configuration parameters for setting up all defined CAIF
 * channel types.
 * The four first fields are mandatory, then physical interface can be specified
 * either by name or by prefered characteristics.
 * The rest of the configuration fields are held in a union for each
 * channel type and are channel type specific.
 */
struct caif_channel_config {
	char name[16];
	enum caif_channel_type type;
	unsigned priority;
	enum caif_phy_preference phy_pref;
	char phy_name[16];

	/** Union of channel type-specific configuration parameters.
	 * 'switched' by attribute type.
	 */
	union {
		/* CAIF_CHTYPE_DATAGRAM */
		struct {
			unsigned connection_id;
		} dgm;
		/* CAIF_CHTYPE_VIDEO */
		struct {
			unsigned connection_id;
		} video;
		/* CAIF_CHTYPE_RFM */
		struct {
			unsigned connection_id;
			char volume[20];
		} rfm;
		/* CAIF_CHTYPE_UTILITY */
		struct {
			unsigned fifosize_kb;
			unsigned fifosize_bufs;
			char name[16];
			unsigned char params[256];
			int paramlen;
		} utility;

	} u;
};

#endif				/* CAIF_CONFIG_H_ */
