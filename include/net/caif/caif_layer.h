/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Braendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CAIF_LAYER_H_
#define CAIF_LAYER_H_

#include <net/caif/generic/cfglue.h>

struct layer;
struct cfpkt;
struct cfpktq;
struct payload_info;
struct caif_packet_funcs;

#define CAIF_MAX_FRAMESIZE 4096
#define CAIF_MAX_PAYLOAD_SIZE (4096 - 64)
#define CAIF_NEEDED_HEADROOM (10)
#define CAIF_NEEDED_TAILROOM (2)


#define CAIF_LAYER_NAME_SZ 16
#define CAIF_SUCCESS    1
#define CAIF_FAILURE    0

/*
 * CAIF Control Signaling.
 *  These commands are sent upwards in the CAIF stack. They are used for
 *  signaling originating from the modem.
 *  These are either responses (*_RSP) or events (*_IND).
 */
enum caif_ctrlcmd {
	/* Flow Control is OFF, transmit function should stop sending data */
	CAIF_CTRLCMD_FLOW_OFF_IND,
	/* Flow Control is ON, transmit function can start sending data */
	CAIF_CTRLCMD_FLOW_ON_IND,
	/* Remote end modem has decided to close down channel */
	CAIF_CTRLCMD_REMOTE_SHUTDOWN_IND,
	/* Called initially when the layer below has finished initialization */
	CAIF_CTRLCMD_INIT_RSP,
	/* Called when de-initialization is complete */
	CAIF_CTRLCMD_DEINIT_RSP,
	/* Called if initialization fails */
	CAIF_CTRLCMD_INIT_FAIL_RSP,
	/* Called if physical interface cannot send more packets. */
	_CAIF_CTRLCMD_PHYIF_FLOW_OFF_IND,
	/* Called if physical interface is able to send packets again. */
	_CAIF_CTRLCMD_PHYIF_FLOW_ON_IND,
	/* Called if physical interface is going down. */
	_CAIF_CTRLCMD_PHYIF_DOWN_IND,
};

/*
 * Modem Control Signaling.
 *  These are requests sent 'downwards' in the stack.
 *  Flow ON, OFF can be indicated to the modem.
 */
enum caif_modemcmd {
	/* Flow Control is ON, transmit function can start sending data */
	CAIF_MODEMCMD_FLOW_ON_REQ = 0,
	/* Flow Control is OFF, transmit function should stop sending data */
	CAIF_MODEMCMD_FLOW_OFF_REQ = 1,
	/* Notify physical layer that it is in use */
	_CAIF_MODEMCMD_PHYIF_USEFULL = 3,
	/* Notify physical layer that it is no longer in use */
	_CAIF_MODEMCMD_PHYIF_USELESS = 4
};

/*
 * CAIF Packet Direction.
 * Indicate if a packet is to be sent out or to be received in.
 */
enum caif_direction {
	CAIF_DIR_IN = 0,	/* Incoming packet received. */
	CAIF_DIR_OUT = 1	/* Outgoing packet to be transmitted. */
};

/*
 * This structure defines the generic layered structure in CAIF.
 *  It is inspired by the "Protocol Layer Design Pattern" (Streams).
 *
 *  It defines a generic layering structure, used by all CAIF Layers and the
 *  layers interfacing CAIF.
 *
 *  In order to integrate with CAIF an adaptation layer on top of the CAIF stack
 *  and PHY layer below the CAIF stack
 *  must be implemented. These layer must follow the design principles below.
 *
 *  Principles for layering of protocol layers:
 *    -# All layers must use this structure. If embedding it, then place this
 *	     structure first in the layer specific structure.
 *    -# Each layer should not depend on any others layer private data.
 *    -# In order to send data upwards do
 *	 layer->up->receive(layer->up, packet);
 *    -# In order to send data downwards do
 *	 layer->dn->transmit(layer->dn, info, packet);
 */
struct layer {

	struct layer *up;	/* Pointer to the layer above */
	struct layer *dn;	/* Pointer to the layer below */
	/*
	 *  Receive Function.
	 *  Contract: Each layer must implement a receive function passing the
	 *  CAIF packets upwards in the stack.
	 *	Packet handling rules:
	 *	      -# The CAIF packet (cfpkt) cannot be accessed after
	 *		     passing it to the next layer using up->receive().
	 *	      -# If parsing of the packet fails, the packet must be
	 *		     destroyed and -1 returned from the function.
	 *	      -# If parsing succeeds (and above layers return OK) then
	 *		      the function must return a value > 0.
	 *
	 *  @param[in] layr Pointer to the current layer the receive function is
	 *		implemented for (this pointer).
	 *  @param[in] cfpkt Pointer to CaifPacket to be handled.
	 *  @return result < 0 indicates an error, 0 or positive value
	 *	     indicates success.
	 */
	int (*receive)(struct layer *layr, struct cfpkt *cfpkt);

	/*
	 *  Transmit Function.
	 *  Contract: Each layer must implement a transmit function passing the
	 *	CAIF packet downwards in the stack.
	 *	Packet handling rules:
	 *	      -# The CAIF packet (cfpkt) ownership is passed to the
	 *		 transmit function. This means that the the packet
	 *		 cannot be accessed after passing it to the below
	 *		 layer using dn->transmit().
	 *
	 *	      -# If transmit fails, however, the ownership is returned
	 *		 to thecaller. The caller of "dn->transmit()" must
	 *		 destroy or resend packet.
	 *
	 *	      -# Return value less than zero means error, zero or
	 *		 greater than zero means OK.
	 *
	 *  @param[in] layr  Pointer to the current layer the receive function
	 *			    is	implemented for (this pointer).
	 *  @param[in] cfpkt Pointer to CaifPacket to be handled.
	 *  @return	     result < 0 indicates an error, 0 or positive value
	 *		     indicate success.
	 */
	int (*transmit) (struct layer *layr, struct cfpkt *cfpkt);

	/*
	 *  Control Function used to signal upwards in the CAIF stack.
	 *  Used for signaling responses (CAIF_CTRLCMD_*_RSP)
	 *  and asynchronous events from the modem  (CAIF_CTRLCMD_*_IND)
	 *
	 *  @param[in] layr  Pointer to the current layer the receive function
	 *		     is implemented for (this pointer).
	 *  @param[in] ctrl  Control Command.
	 */
	void (*ctrlcmd) (struct layer *layr, enum caif_ctrlcmd ctrl,
			 int phyid);

	/*
	 *  Control Function used for controlling the modem. Used to signal
	 *  down-wards in the CAIF stack.
	 *  @returns 0 on success, < 0 upon failure.
	 *  @param[in] layr  Pointer to the current layer the receive function
	 *		     is implemented for (this pointer).
	 *  @param[in] ctrl  Control Command.
	 */
	int (*modemcmd) (struct layer *layr, enum caif_modemcmd ctrl);

	struct layer *next;	/*
				 *   Pointer to chain of layers, up/dn will
				 *   then point at the first element of a
				 *   which then should be iterated through
				 *   the next pointer.
				 */
	unsigned short prio;	/* Priority of this layer */
	unsigned int id;	/* The identity of this layer. */
	unsigned int type;	/* The type of this layer */
	char name[CAIF_LAYER_NAME_SZ];		/* Name of the layer */
};
/*
 * Set the up pointer for a specified layer.
 *  @param layr Layer where up pointer shall be set.
 *  @param above Layer above.
 */
#define layer_set_up(layr, above) ((layr)->up = (struct layer *)(above))

/*
 * Set the dn pointer for a specified layer.
 *  @param layr Layer where down pointer shall be set.
 *  @param below Layer below.
 */
#define layer_set_dn(layr, below) ((layr)->dn = (struct layer *)(below))

/* Physical Device info, holding information about physical layer. */
struct dev_info {
	/* Pointer to native physical device */
	void *dev;

	/*
	 * Physical ID of the physical connection used by the logical CAIF
	 * connection. Used by service layers to identify their physical id
	 * to Caif MUX (CFMUXL)so that the MUX can add the correct physical
	 * ID to the packet.
	 */
	unsigned int id;
};

/* Transmit info, passed downwards in protocol layers.  */
struct payload_info {
	/* Information about the receiving device */
	struct dev_info *dev_info;

	/* Header length, used to align pay load on 32bit boundary.  */
	unsigned short hdr_len;

	/*
	 * Channel ID of the logical CAIF connection.
	 * Used by mux to insert channel id into the caif packet.
	 */
	unsigned short channel_id;
};

#endif	/* CAIF_LAYER_H_ */
