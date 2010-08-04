/*
 * include/linux/mfd/ste_conn.h
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Authors:
 * Par-Gunnar Hjalmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Henrik Possung (henrik.possung@stericsson.com) for ST-Ericsson.
 * Josef Kindberg (josef.kindberg@stericsson.com) for ST-Ericsson.
 * Dariusz Szymszak (dariusz.xd.szymczak@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson. * License terms:  GNU General Public License (GPL), version 2
 *
 * Linux Bluetooth HCI H:4 Driver for ST-Ericsson connectivity controller.
 */

#ifndef _STE_CONN_H_
#define _STE_CONN_H_

#include <linux/skbuff.h>
#include <mach/ste_conn_devices.h>

struct ste_conn_callbacks;

/**
  * struct ste_conn_device - Device structure for ste_conn user.
  * @h4_channel:      HCI H:4 channel used by this device.
  * @callbacks:       Callback functions registered by this device.
  * @logger_enabled:  True if HCI logger is enabled for this channel, false otherwise.
  * @user_data:       Arbitrary data used by caller.
  * @dev:             Parent device this driver is connected to.
  *
  * Defines data needed to access an HCI channel.
  */
struct ste_conn_device {
  int h4_channel;
  struct ste_conn_callbacks *callbacks;
  bool logger_enabled;
  void *user_data;
  struct device *dev;
};

/**
  * struct ste_conn_callbacks - Callback structure for ste_conn user.
  * @read_cb:  Callback function called when data is received from the connectivity controller.
  * @reset_cb: Callback function called when the connectivity controller has been reset.
  *
  * Defines the callback functions provided from the caller.
  */
struct ste_conn_callbacks {
  void (*read_cb) (struct ste_conn_device *dev, struct sk_buff *skb);
  void (*reset_cb) (struct ste_conn_device *dev);
};

/**
  * struct ste_conn_revision_data - Contains revision data for the local controller.
  * @revision:  Revision of the controller, e.g. to indicate that it is a CG2900 controller.
  * @sub_version: Subversion of the controller, e.g. to indicate a certain tape-out of the controller.
  *
  * The values to match retrieved values to each controller may be retrieved from the manufacturer.
  */
struct ste_conn_revision_data {
	int revision;
	int sub_version;
};

/**
 * ste_conn_register() - Register ste_conn user.
 * @name:   Name of HCI H:4 channel to register to.
 * @cb:     Callback structure to use for the H:4 channel.
 *
 * The ste_conn_register() function register ste_conn user.
 *
 * Returns:
 *   Pointer to ste_conn device structure if successful.
 *   NULL upon failure.
 */
extern struct ste_conn_device *ste_conn_register(char *name, struct ste_conn_callbacks *cb);

/**
 * ste_conn_deregister() - Remove registration of ste_conn user.
 * @dev: ste_conn device.
 *
 * The ste_conn_deregister() function removes the ste_conn user.
 */
extern void ste_conn_deregister(struct ste_conn_device *dev);

/**
 * ste_conn_reset() - Reset the ste_conn controller.
 * @dev: ste_conn device.
 *
 * The ste_conn_reset() function reset the ste_conn controller.
 *
 * Returns:
 *   0 if there is no error.
 *   -EACCES if driver has not been initialized.
 */
extern int ste_conn_reset(struct ste_conn_device *dev);

/**
 * ste_conn_alloc_skb() - Alloc an sk_buff structure for ste_conn handling.
 * @size:     Size in number of octets.
 * @priority: Allocation priority, e.g. GFP_KERNEL.
 *
 * The ste_conn_alloc_skb() function allocates an sk_buff structure for ste_conn
 * handling.
 *
 * Returns:
 *   Pointer to sk_buff buffer structure if successful.
 *   NULL upon allocation failure.
 */
extern struct sk_buff *ste_conn_alloc_skb(unsigned int size, gfp_t priority);

/**
 * ste_conn_write() - Send data to the connectivity controller.
 * @dev: ste_conn device.
 * @skb: Data packet.
 *
 * The ste_conn_write() function sends data to the connectivity controller.
 *
 * Returns:
 *   0 if there is no error.
 *   -EACCES if driver has not been initialized or trying to write while driver is not active.
 *   -EINVAL if NULL pointer was supplied.
 *   Error codes returned from cpd_enable_hci_logger.
 */
extern int ste_conn_write(struct ste_conn_device *dev, struct sk_buff *skb);

/**
 * ste_conn_get_local_revision() - Read revision of the controller connected to this driver.
 * @rev_data: Revision data structure to fill. Must be allocated by caller.
 *
 * The ste_conn_get_local_revision() function returns the revision data of the
 * local controller if available. If data is not available, e.g. because the
 * controller has not yet been started this function will return false.
 *
 * Returns:
 *   true if revision data is available.
 *   false if no revision data is available.
 */
extern bool ste_conn_get_local_revision(struct ste_conn_revision_data *rev_data);

#endif /* _STE_CONN_H_ */
