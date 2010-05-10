/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/tty.h>

#include <net/caif/caif_layer.h>
#include <net/caif/cfcnfg.h>
#include <net/caif/cfpkt.h>
#include <net/caif/caif_chr.h>

MODULE_LICENSE("GPL");
int serial_use_stx = 0;
module_param(serial_use_stx, bool, S_IRUGO);
MODULE_PARM_DESC(serial_use_stx, "STX enabled or not.");

#define WRITE_BUF_SIZE	256
#define READ_BUF_SIZE	256

unsigned char sbuf_wr[WRITE_BUF_SIZE];

struct cflayer ser_phy;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
static struct tty_ldisc_ops phyif_ldisc;
#else
static struct tty_ldisc phyif_ldisc;
#endif				/* KERN_VERSION_2_6_27 */

struct tty_struct *pser_tty;

static bool tx_started;

static int ser_open(struct tty_struct *tty)
{
	int result;

	pser_tty = tty;

	/* Configure the attached TTY. */

	/* Register physical interface. */
	result =
	    caifdev_phy_register(&ser_phy, CFPHYTYPE_FRAG,
			    CFPHYPREF_LOW_LAT,true,serial_use_stx);
	if (result < 0) {
		printk(KERN_WARNING
		       "phyif_ser: err: %d, can't register phy.\n", result);
	}

	return result;
}

static void ser_receive(struct tty_struct *tty, const u8 *data,
			char *flags, int count)
{
	struct cfpkt *pkt = NULL;
	struct caif_packet_funcs f;

	/* Get CAIF packet functions. */
	f = cfcnfg_get_packet_funcs();

	/* Workaround for garbage at start of transmission,
	 * only enable if STX handling is not enabled
	 */
	if (!serial_use_stx && !tx_started) {
		printk(KERN_WARNING
		       "Bytes received before first transmission."
		       " Bytes discarded. \n");
		return;
	}

	/* Get a suitable CAIF packet and copy in data. */
	pkt = f.cfpkt_create_recv_pkt(data, count);

	/* Push received packet up the stack. */
	ser_phy.up->receive(ser_phy.up, pkt);
}

int ser_phy_tx(struct cflayer *layr, struct cfpkt *cfpkt)
{
	size_t tty_wr, actual_len;
	bool cont;
	struct caif_packet_funcs f;

	if (!pser_tty)
		return -ENOTCONN;

	/* Get CAIF packet functions. */
	f = cfcnfg_get_packet_funcs();

	/* NOTE: This workaround is not really needed when STX is enabled.
	 * Remove?
	 */
	if (tx_started == false)
		tx_started = true;

	do {
		char *bufp;
		/* By default we assume that we will extract
		 * all data in one go.
		 */
		cont = false;

		/* Extract data from the packet. */
		f.cfpkt_extract(cfpkt, sbuf_wr, WRITE_BUF_SIZE, &actual_len);

		/* Check whether we need to extract more data. */
		if (actual_len == WRITE_BUF_SIZE)
			cont = true;

		bufp = sbuf_wr;
		/* Write the data on the tty driver.
		 * NOTE: This loop will be spinning until UART is ready for
		 *	 sending data.
		 *	 It might be looping forever if we get UART problems.
		 *	 This part should be re-written!
		 */
		do {
			tty_wr =
			    pser_tty->ops->write(pser_tty, bufp, actual_len);
			/* When not whole buffer is written,
			 * forward buffer pointer and try again
			 */
			actual_len -= tty_wr;
			bufp += tty_wr;
		} while (actual_len);
	} while (cont == true);

	/* The packet is sent. As we have come to the end of the
	 * line, we need to free the packet.
	 */
	f.cfpkt_destroy(cfpkt);

	return 0;
}

static int __init phyif_ser_init(void)
{
	int result;

	/* Fill in some information about our PHY. */
	ser_phy.transmit = ser_phy_tx;
	ser_phy.receive = NULL;
	ser_phy.ctrlcmd = NULL;
	ser_phy.modemcmd = NULL;

	memset(&phyif_ldisc, 0, sizeof(phyif_ldisc));
	phyif_ldisc.magic = TTY_LDISC_MAGIC;
	phyif_ldisc.name = "n_phyif";
	phyif_ldisc.open = ser_open;
	phyif_ldisc.receive_buf = ser_receive;
	phyif_ldisc.owner = THIS_MODULE;

	result = tty_register_ldisc(N_MOUSE, &phyif_ldisc);

	if (result < 0) {
		printk(KERN_WARNING
		       "phyif_ser: err: %d, can't register ldisc.\n", result);
		return result;
	}

	return result;
}

static void __exit phyif_ser_exit(void)
{
	(void) tty_unregister_ldisc(N_MOUSE);
}

module_init(phyif_ser_init);
module_exit(phyif_ser_exit);

MODULE_ALIAS_LDISC(N_MOUSE);
