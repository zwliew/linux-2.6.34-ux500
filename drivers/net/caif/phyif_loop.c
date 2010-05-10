/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 *		Per Sigmond / Per.Sigmond@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/list.h>
#include <linux/sched.h>

/* Caif header files. */
#include <net/caif/caif_layer.h>
#include <net/caif/cfcnfg.h>
#include <net/caif/cfpkt.h>
#include <net/caif/caif_chr.h>

MODULE_LICENSE("GPL");

static int reentrant;
static int direct;
static int serial;
module_param(reentrant, bool, S_IRUGO);
module_param(direct, bool, S_IRUGO);
module_param(serial, bool, S_IRUGO);
MODULE_PARM_DESC(reentrant,
		 "Reentrant or not (default is workqueue implementation)");
MODULE_PARM_DESC(direct,
		 "Direct mode, looping packets directly back up the stack");

static struct cflayer cf_phy;
static struct cflayer loop_phy;
static spinlock_t ring_buffer_lock;

/* Start ring buffer */
#define RING_MAX_BUFFERS 16384

struct ring_buffer_element {
	struct cfpkt *cfpkt;
};

static struct {
	struct ring_buffer_element ring_buffer[RING_MAX_BUFFERS];
	int head_index;
	int tail_index;
} my_ring_buffer;

#define ring_buffer_index_plus_one(index) \
    ((index+1) < RING_MAX_BUFFERS ? (index + 1) : 0)

#define ring_buffer_increment_tail(rb) \
    ((rb)->tail_index = ring_buffer_index_plus_one((rb)->tail_index))

#define ring_buffer_increment_head(rb) \
    ((rb)->head_index = ring_buffer_index_plus_one((rb)->head_index))

#define ring_buffer_empty(rb) ((rb)->head_index == (rb)->tail_index)
#define ring_buffer_full(rb) (ring_buffer_index_plus_one((rb)->head_index)\
			      == (rb)->tail_index)
#define ring_buffer_tail_element(rb) ((rb)->ring_buffer[(rb)->tail_index])
#define ring_buffer_head_element(rb) ((rb)->ring_buffer[(rb)->head_index])
#define ring_buffer_size(rb) (((rb)->head_index >= (rb)->tail_index)) ?\
  ((rb)->head_index - (rb)->tail_index) : \
    (RING_MAX_BUFFERS - ((rb)->tail_index - (rb)->head_index))
/* End ring buffer */

static void work_func(struct work_struct *work);
static struct workqueue_struct *ploop_work_queue;
static DECLARE_WORK(loop_work, work_func);
static wait_queue_head_t buf_available;

static void work_func(struct work_struct *work)
{
	CAIFLOG_ENTER("");

	while (!ring_buffer_empty(&my_ring_buffer)) {
		struct cfpkt *cfpkt;

		/* Get packet */
		cfpkt = ring_buffer_tail_element(&my_ring_buffer).cfpkt;
		ring_buffer_tail_element(&my_ring_buffer).cfpkt = NULL;
		ring_buffer_increment_tail(&my_ring_buffer);

		/* Wake up writer */
		wake_up_interruptible(&buf_available);

		/* Push received packet up the caif stack. */
		cf_phy.up->receive(cf_phy.up, cfpkt);
	}
	/* Release access to loop queue. */
	CAIFLOG_EXIT("");
}

static int cf_phy_modemcmd(struct cflayer *layr, enum caif_modemcmd ctrl)
{
	switch (ctrl) {
	case _CAIF_MODEMCMD_PHYIF_USEFULL:
		CAIFLOG_TRACE("phyif_loop:Usefull");
		try_module_get(THIS_MODULE);
		break;
	case _CAIF_MODEMCMD_PHYIF_USELESS:
		CAIFLOG_TRACE("phyif_loop:Useless");
		module_put(THIS_MODULE);
		break;
	default:
		break;
	}
	return 0;
}

static int cf_phy_tx(struct cflayer *layr, struct cfpkt *pkt)
{
	int ret;
	CAIFLOG_ENTER("");

	/* Push received packet up the loop stack. */
	ret = loop_phy.up->receive(loop_phy.up, pkt);

	CAIFLOG_EXIT("");
	return ret;
}

static int
loop_phy_tx_reent(struct cflayer *layr, struct cfpkt *cfpkt)
{
	CAIFLOG_ENTER("");

	/* Push received packet up the caif stack. */
	cf_phy.up->receive(cf_phy.up, cfpkt);

	CAIFLOG_EXIT("");
	return 0;
}

static int loop_phy_tx(struct cflayer *layr, struct cfpkt *cfpkt)
{
	CAIFLOG_ENTER("");

	/* Block writer as long as ring buffer is full */
	spin_lock(&ring_buffer_lock);

	while (ring_buffer_full(&my_ring_buffer)) {
		spin_unlock(&ring_buffer_lock);

		if (wait_event_interruptible
		    (buf_available,
		     !ring_buffer_full(&my_ring_buffer)) == -ERESTARTSYS) {
			printk(KERN_WARNING
			       "loop_phy_tx: "
			       "wait_event_interruptible woken by a signal\n");
			return -ERESTARTSYS;
		}
		spin_lock(&ring_buffer_lock);
	}

	ring_buffer_head_element(&my_ring_buffer).cfpkt = cfpkt;
	ring_buffer_increment_head(&my_ring_buffer);
	spin_unlock(&ring_buffer_lock);

	/* Add this  work to the queue as we don't want to
	 * loop in the same context.
	 */
	(void) queue_work(ploop_work_queue, &loop_work);

	CAIFLOG_EXIT("");
	return 0;
}

static int cf_phy_tx_direct(struct cflayer *layr, struct cfpkt *pkt)
{
	int ret;

	CAIFLOG_ENTER("");
	CAIFLOG_TRACE("[%s] up:%p pkt:%p\n", __func__, cf_phy.up,
		    pkt);
	/* Push received packet back up the caif stack,
	 * via loop_phy_tx's work-queue
	 */
	ret = loop_phy_tx(layr, pkt);
	CAIFLOG_EXIT("");
	return ret;
}


static int __init phyif_loop_init(void)
{
	int result;

	CAIFLOG_ENTER("");
	printk("\nCompiled:%s:%s\nreentrant=%s direct=%s\n",
	       __DATE__, __TIME__,
	       (reentrant ? "yes" : "no"),
	       (direct ? "yes" : "no"));
	/* Fill in some information about our PHYs. */
	if (direct) {
		cf_phy.transmit = cf_phy_tx_direct;
		cf_phy.receive = NULL;
	} else {
		cf_phy.transmit = cf_phy_tx;
		cf_phy.receive = NULL;
	}
	if (reentrant)
		loop_phy.transmit = loop_phy_tx_reent;
	else
		loop_phy.transmit = loop_phy_tx;

	loop_phy.receive = NULL;
	cf_phy.modemcmd = cf_phy_modemcmd;

	/* Create work thread. */
	ploop_work_queue = create_singlethread_workqueue("phyif_loop");

	init_waitqueue_head(&buf_available);

	/* Initialize ring buffer */
	memset(&my_ring_buffer, 0, sizeof(my_ring_buffer));
	spin_lock_init(&ring_buffer_lock);
	cf_phy.id = -1;
	if (serial)
		result =
		    caifdev_phy_register(&cf_phy, CFPHYTYPE_FRAG,
					 CFPHYPREF_UNSPECIFIED,false,false);
	else
		result =
		    caifdev_phy_register(&cf_phy, CFPHYTYPE_CAIF,
					 CFPHYPREF_UNSPECIFIED,false,false);

	printk(KERN_WARNING "phyif_loop: ID = %d\n", cf_phy.id);

	if (result < 0) {
		printk(KERN_WARNING
		       "phyif_loop: err: %d, can't register phy.\n", result);
	}

	if (serial)
		result =
			caifdev_phy_loop_register(&loop_phy, CFPHYTYPE_FRAG,
						  false, false);
	else
		result = caifdev_phy_loop_register(&loop_phy, CFPHYTYPE_CAIF,
						  false, false);

	if (result < 0) {
		printk(KERN_WARNING
		       "phyif_loop: err: %d, can't register loop phy.\n",
		       result);
	}

	printk(KERN_WARNING "phyif_loop: ID = %d\n", cf_phy.id);
	CAIFLOG_EXIT("");
	return result;
}

static void __exit phyif_loop_exit(void)
{
	printk(KERN_WARNING "phyif_loop: ID = %d\n", cf_phy.id);

	caifdev_phy_unregister(&cf_phy);
	cf_phy.id = -1;
}

module_init(phyif_loop_init);
module_exit(phyif_loop_exit);
