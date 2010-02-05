/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CAIF_IOCTL_H_
#define CAIF_IOCTL_H_
#include <linux/caif/caif_config.h>

/*!\page  caif_ioctl.h
 * This file defines the management interface to CAIF.
 * It defines how CAIF channels are configured and become visible in the
 * Linux file system, typically under "/dev".
 *
 *\b Example - creating a new AT character device:
 * \code
   fd = open("/dev/caifconfig",..);
   struct caif_channel_create_action at_config = {
	 .name = "cnhl2",
	 .config = {
	    .channel = CAIF_CHTY_AT,
	    .phy_ref = CAIF_PHY_LOW_LAT,
	    .priority = CAIF_PRIO_HIGH
	 }};
   ioctl(fd, CAIF_IOC_CONFIG_DEVICE,&at_config);
   close(fd);
 * \endcode
 * This will cause a new AT channel to be available in at "/dev/chnl2".
 * This CAIF channel can then be connected by using \ref open.
 */

#define CAIF_IOC_MAGIC 'g'
#define DEVICE_NAME_LEN 16

/* Specifies the type of device to create: NET device or CHAR device*/
enum caif_dev_type {
	CAIF_DEV_CHR = 1,
	CAIF_DEV_NET = 2
};

/** Used for identifying devices, PHY interfaces, etc.*/
struct caif_device_name {
	char name[DEVICE_NAME_LEN];	/*!< Device name */
	enum caif_dev_type devtype;	/*!< Device type */
};

/**
 * CAIF ACTION for \ref CAIF_ACT_CHANNEL_CONFIG.
 * This structure is used to configure a new CAIF Channel and
 * create the corresponding character device.
 */
struct caif_channel_create_action {
	/** \b in  CAIF configuration request */
	struct caif_channel_config config;
	/** \b in/out Device name returned from ACTION */
	struct caif_device_name name;
	/** \b out Major device id */
	int major;
	/** \b out Minor device id */
	int minor;
};

/**
 * union caif_action
 * This union is used to create and delete CAIF channels.
 */
union caif_action {
	struct caif_device_name delete_channel;
	struct caif_channel_create_action create_channel;
};

/**
 * CAIF IOCTL for \ref CAIF_IOC_CHANNEL_CONFIG.
 * This structure is used to configure a new CAIF Channel and
 * create the corresponding character device.
 */

/** Create and Configure a new CAIF device.
 * Note that the device is not implicitly connected.
 */
#define CAIF_IOC_CONFIG_DEVICE		_IOWR(CAIF_IOC_MAGIC, 1,\
struct caif_channel_create_action)

/** Remove a CAIF device. Requires the device to be previously disconnected. */
#define CAIF_IOC_REMOVE_DEVICE		_IOWR(CAIF_IOC_MAGIC, 2,\
		struct caif_device_name)
#define CAIF__IOC_MAXNR				9
/*! @} */

#endif				/* CAIF_IOCTL_H_ */
