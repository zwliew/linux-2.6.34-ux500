/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CAIF_CHR_H_
#define CAIF_CHR_H_

#include <net/caif/generic/caif_layer.h>
#include <net/caif/generic/cfcnfg.h>
#include <linux/caif/caif_config.h>
#include <linux/if.h>
#include <net/caif/caif_actions.h>
#include <net/caif/caif_device.h>
#include <net/caif/caif_dev.h>

extern int caif_dbg_level;

#define CAIFLOG_ON 1

#define CAIFLOG_MIN_LEVEL     1
#define CAIFLOG_LEVEL_ERROR   1
#define CAIFLOG_LEVEL_WARNING 2
#define CAIFLOG_LEVEL_TRACE   3
#define CAIFLOG_LEVEL_TRACE2  4
#define CAIFLOG_LEVEL_TRACE3  5
#define CAIFLOG_LEVEL_FUNC    6
#define CAIFLOG_MAX_LEVEL     7


/* Fatal error condition, halt the kernel */
#define CAIFLOG_FATAL(format, args...) do if ( \
	caif_dbg_level > CAIFLOG_LEVEL_ERROR)  \
	printk(KERN_ERR "<%s:%d, FATAL> " format, __func__, __LINE__ , \
	## args);\
  while (0)

/* CAIF error logging. */
#define CAIFLOG_ERROR(format, args...)	do if (\
caif_dbg_level > CAIFLOG_LEVEL_ERROR)  \
printk(KERN_ERR "<%s:%d, ERROR> " format, __func__, __LINE__ , ## args);\
  while (0)

/* CAIF warning logging. */
#define CAIFLOG_WARN(format, args...)	do if (\
caif_dbg_level > CAIFLOG_LEVEL_WARNING)	 \
printk(KERN_WARNING "<%s:%d, WARN> "  format, __func__, __LINE__ , ## args);\
  while (0)

/* CAIF trace control logging. Level 1 control trace (channel setup, etc.) */
#define CAIFLOG_TRACE(format, args...)	do if (\
caif_dbg_level > CAIFLOG_LEVEL_TRACE)  \
printk(KERN_WARNING "<%s:%d, TRACE> " format, __func__, __LINE__ , ## args); \
 while (0)

/* CAIF trace payload logging. Level payload trace */
#define CAIFLOG_TRACE2(format, args...) do if ( \
caif_dbg_level > CAIFLOG_LEVEL_TRACE2)	\
printk(KERN_WARNING "<%s:%d, TRACE2> " format, __func__, __LINE__ , ## args);\
  while (0)

/* CAIF trace detailed logging, including packet dumps */
#define CAIFLOG_TRACE3(format, args...) do if ( \
caif_dbg_level > CAIFLOG_LEVEL_TRACE3)	\
printk(KERN_WARNING "<%s:%d, TRACE3> " format, __func__, __LINE__ , ## args); \
 while (0)

/* CAIF trace entering function */
#define CAIFLOG_ENTER(format, args...)	do if (\
caif_dbg_level > CAIFLOG_LEVEL_FUNC)  \
printk(KERN_WARNING "<%s:%d, ENTER> " format, __func__, __LINE__ , ## args); \
 while (0)

/* CAIF trace exiting function */
#define CAIFLOG_EXIT(format, args...)	do if (\
caif_dbg_level > CAIFLOG_LEVEL_FUNC)  \
printk(KERN_WARNING "<%s:%d, EXIT> "  format, __func__, __LINE__ , ## args);\
  while (0)

#define IF_CAIF_TRACE(cmd) do if (caif_dbg_level > CAIFLOG_LEVEL_TRACE)\
 { cmd; } \
  while (0)


/*
 * Packet functions needed by the adaptation layer and PHY layer are exported
 *  in this structure.
 *
 */
struct caif_packet_funcs {

	/* Used to map from a "native" packet (e.g. Linux Socket Buffer)
	 *  to a CAIF packet.
	 *  @param dir - Direction, indicating whether this packet is to be
	 *  		sent or received.
	 *  @param nativepkt - The native packet to be transformed to a CAIF
	 *  		packet.
	 *  @returns the mapped CAIF packet CFPKT.
	 */
	struct cfpkt *
	    (*cfpkt_fromnative)(enum caif_direction dir, void *nativepkt);

	/* Used to map from a CAIF packet to a "native" packet
	 *  (e.g. Linux Socket Buffer).
	 *  @param pkt	- The CAIF packet to be transformed to a "native"
	 * 		  packet.
	 *  @returns the native packet transformed from a CAIF packet.
	 */
	void *(*cfpkt_tonative)(struct cfpkt *pkt);

	/* Used by "app" layer to create an outgoing CAIF packet to be sent
	 *  out of the CAIF Stack.
	 *  @param data - Packet data to copy into the packet.
	 *		  If NULL then copying will not take place.
	 *  @param len	- Length of data to copy into the packet.
	 *  @returns a new CAIF packet CFPKT.
	 *  @deprecated Use \b cfpkt_create_pkt (above) instead.
	 */
	struct cfpkt *
	    (*cfpkt_create_recv_pkt)(const unsigned char *data,
				      unsigned int len);

	/* Used by PHY layer to create an incoming CAIF packet to be
	 *  processed by the CAIF Stack.
	 *  @param data - Packet data to copy into the packet. If NULL then
	 *  copying will not take place.
	 *  @param len	- Length of data to copy into the packet.
	 *  @returns a new CAIF packet CFPKT.
	 *  @deprecated Use \b cfpkt_create_pkt (above) instead.
	 */
	struct cfpkt *
	    (*cfpkt_create_xmit_pkt)(const unsigned char *data,
				      unsigned int len);
	/* Releases a CAIF packet.
	 *  @param cfpkt	 Packet to destroy.
	 */
	void
	 (*cfpkt_destroy)(struct cfpkt *cfpkt);

	/* Append by giving user access to the packet buffer.
	 * @param pkt Packet to append to.
	 * @param buf Buffer inside pkt that user shall copy data into.
	 * @param buflen Length of buffer and number of bytes added to packet.
	 * @return < 0 on error.
	 */
	 int (*cfpkt_raw_append)(struct cfpkt *cfpkt, void **buf,
				unsigned int buflen);

	/* Extract by giving user access to the packet buffer.
	 * @param pkt Packet to extract from.
	 * @param buf Buffer inside pkt that user shall copy data from.
	 * @param buflen Length of buffer and number of bytes removed from
	 *        packet.
	 * @return < 0 on error
	 */
	 int (*cfpkt_raw_extract)(struct cfpkt *cfpkt, void **buf,
				 unsigned int buflen);

	/* Creates a packet queue. */
	struct cfpktq *(*cfpktq_create)(void);

	/* Inserts a packet into the packet queue, packets are ordered by
	 *  priority.
	 *  If the same priority is used packets are ordered as a FIFO.
	 */
	void (*cfpkt_queue)(struct cfpktq *pktq, struct cfpkt *pkt,
			     unsigned short prio);

	/* Peek into the first packet in the queue. */
	struct cfpkt *(*cfpkt_qpeek)(struct cfpktq *pktq);

	/* Dequeue a packet from the queue. */
	struct cfpkt *(*cfpkt_dequeue)(struct cfpktq *pktq);

	/* Get length of a packet */
	uint16(*cfpkt_getlen)(struct cfpkt *pkt);
};


struct caif_service_config;
extern int (*netdev_mgmt_func) (int action, union caif_action *param);

int caifdev_phy_register(struct layer *phyif, enum cfcnfg_phy_type phy_type,
			 enum cfcnfg_phy_preference phy_pref,
			 bool fcs, bool stx);

int caifdev_phy_unregister(struct layer *phyif);
int caifdev_phy_loop_register(struct layer *phyif,
			      enum cfcnfg_phy_type phy_type,
			      bool fcs, bool stx);
int caif_register_chrdev(int (*chrdev_mgmt)
			(int action, union caif_action *param));
void caif_unregister_chrdev(void);
struct caif_packet_funcs cfcnfg_get_packet_funcs(void);
struct net_device *chnl_net_create(char *name,
				   struct caif_channel_config *config);
struct caif_packet_funcs caif_get_packet_funcs(void);

#endif				/* CAIF_CHR_H_ */

