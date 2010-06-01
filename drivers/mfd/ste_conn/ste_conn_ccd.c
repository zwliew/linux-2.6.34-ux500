/*
 * drivers/mfd/ste_conn/ste_conn_ccd.c
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Authors:
 * Par-Gunnar Hjalmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Henrik Possung (henrik.possung@stericsson.com) for ST-Ericsson.
 * Josef Kindberg (josef.kindberg@stericsson.com) for ST-Ericsson.
 * Dariusz Szymszak (dariusz.xd.szymczak@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson.
 * License terms:  GNU General Public License (GPL), version 2
 *
 * Linux Bluetooth HCI H:4 Driver for ST-Ericsson connectivity controller.
 */

#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/skbuff.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/tty.h>
#include <linux/tty_ldisc.h>
#include <linux/poll.h>
#include <linux/timer.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/sched.h>

#include <linux/mfd/ste_conn.h>
#include "ste_conn_cpd.h"
#include "ste_conn_debug.h"
#include "ste_conn_ccd.h"

#define VERSION "1.0"

/* Device names */
#define STE_CONN_CDEV_NAME			"ste_conn_ccd_test"
#define STE_CONN_CLASS_NAME			"ste_conn_class"
#define STE_CONN_DEVICE_NAME			"ste_conn_driver"

/* Workqueues' names */
#define STE_CONN_UART_TX_WQ_NAME		"ste_conn_uart_tx_wq\0"
#define STE_CONN_CCD_WQ_NAME			"ste_conn_ccd_wq\0"

/* Standardized Bluetooth command channels */
#define HCI_BT_CMD_H4_CHANNEL			0x01
#define HCI_BT_ACL_H4_CHANNEL			0x02
#define HCI_BT_EVT_H4_CHANNEL			0x04

/* Default H4 channels which may change depending on connected controller */
#define HCI_FM_RADIO_H4_CHANNEL			0x08
#define HCI_GNSS_H4_CHANNEL			0x09

/* Different supported transports */
enum ccd_transport {
	CCD_TRANSPORT_UNKNOWN,
	CCD_TRANSPORT_UART,
	CCD_TRANSPORT_CHAR_DEV
};

/* Number of bytes to reserve at start of sk_buffer when receiving packet */
#define CCD_RX_SKB_RESERVE			8
/* Max size of received packet (not including reserved bytes) */
#define CCD_RX_SKB_MAX_SIZE		1024

/* Max size of bytes we can receive on the UART */
#define CCD_UART_RECEIVE_ROOM		65536

/* Size of the header in the different packets */
#define CCD_HCI_BT_EVT_HDR_SIZE		2
#define CCD_HCI_BT_ACL_HDR_SIZE		4
#define CCD_HCI_FM_RADIO_HDR_SIZE	1
#define CCD_HCI_GNSS_HDR_SIZE		3

/* Position of length field in the different packets */
#define CCD_HCI_EVT_LEN_POS		2
#define CCD_HCI_ACL_LEN_POS		3
#define CCD_FM_RADIO_LEN_POS		1
#define CCD_GNSS_LEN_POS		2

#define CCD_H4_HEADER_LEN		0x01

/* Bytes in the command Hci_Reset */
#define CCD_HCI_RESET_LSB		0x03
#define CCD_HCI_RESET_MSB		0x0C
#define CCD_HCI_RESET_PAYL_LEN		0x00
#define CCD_HCI_RESET_LEN		0x03

/* Bytes in the command Hci_Cmd_ST_Set_Uart_Baud_Rate */
#define CCD_SET_BAUD_RATE_LSB		0x09
#define CCD_SET_BAUD_RATE_MSB		0xFC
#define CCD_SET_BAUD_RATE_PAYL_LEN	0x01
#define CCD_SET_BAUD_RATE_LEN		0x04
#define CCD_BAUD_RATE_57600		0x03
#define CCD_BAUD_RATE_115200		0x02
#define CCD_BAUD_RATE_230400		0x01
#define CCD_BAUD_RATE_460800		0x00
#define CCD_BAUD_RATE_921600		0x20
#define CCD_BAUD_RATE_2000000		0x25
#define CCD_BAUD_RATE_3000000		0x27
#define CCD_BAUD_RATE_4000000		0x2B

/* Baud rate defines */
#define CCD_ZERO_BAUD_RATE		0
#define CCD_DEFAULT_BAUD_RATE		115200
#define CCD_HIGH_BAUD_RATE		921600

/* Bytes for the HCI Command Complete event */
#define HCI_BT_EVT_CMD_COMPLETE		0x0E
#define HCI_BT_EVT_CMD_COMPLETE_LEN	0x04

/* HCI error values */
#define HCI_BT_ERROR_NO_ERROR		0x00

/* HCI TTY line discipline value */
#ifndef N_HCI
#define N_HCI				15
#endif

/* IOCTLs for UART */
#define HCIUARTSETPROTO			_IOW('U', 200, int)
#define HCIUARTGETPROTO			_IOR('U', 201, int)
#define HCIUARTGETDEVICE		_IOR('U', 202, int)

/* UART break control parameters */
#define TTY_BREAK_ON			(-1)
#define TTY_BREAK_OFF			(0)


/**
  * enum ccd_state - Main-state for CCD.
  * @CCD_STATE_INITIALIZING: CCD initializing.
  * @CCD_STATE_OPENED:       CCD is opened (data can be sent).
  * @CCD_STATE_CLOSED:       CCD is closed (data cannot be sent).
  */
enum ccd_state {
	CCD_STATE_INITIALIZING,
	CCD_STATE_OPENED,
	CCD_STATE_CLOSED
};

/**
  * enum ccd_uart_rx_state - UART RX-state for CCD.
  * @W4_PACKET_TYPE:  Waiting for packet type.
  * @W4_EVENT_HDR:    Waiting for BT event header.
  * @W4_ACL_HDR:      Waiting for BT ACL header.
  * @W4_FM_RADIO_HDR: Waiting for FM header.
  * @W4_GNSS_HDR:     Waiting for GNSS header.
  * @W4_DATA:         Waiting for data in rest of the packet (after header).
  */
enum ccd_uart_rx_state {
	W4_PACKET_TYPE,
	W4_EVENT_HDR,
	W4_ACL_HDR,
	W4_FM_RADIO_HDR,
	W4_GNSS_HDR,
	W4_DATA
};

/**
  * enum ccd_sleep_state - Sleep-state for CCD.
  * @CHIP_AWAKE:  Chip is awake.
  * @CHIP_ASLEEP: Chip is asleep.
  */
enum ccd_sleep_state {
	CHIP_AWAKE,
	CHIP_ASLEEP
};

/**
  * enum ccd_baud_rate_change_state - Baud rate-state for CCD.
  * @BAUD_IDLE:		No baud rate change is ongoing.
  * @BAUD_SENDING_RESET: HCI reset has been sent. Waiting for command complete event.
  * @BAUD_START:	Set baud rate cmd scheduled for sending.
  * @BAUD_SENDING:	Set baud rate cmd sending in progress.
  * @BAUD_WAITING:	Set baud rate cmd sent, waiting for command complete event.
  * @BAUD_SUCCESS:	Baud rate change has succeeded.
  * @BAUD_FAIL:		Baud rate change has failed.
  */
enum ccd_baud_rate_change_state {
	BAUD_IDLE,
	BAUD_SENDING_RESET,
	BAUD_START,
	BAUD_SENDING,
	BAUD_WAITING,
	BAUD_SUCCESS,
	BAUD_FAIL
};

/**
  * struct ccd_uart_rx - UART RX info structure.
  * @rx_state: Current RX state.
  * @rx_count: Number of bytes left to receive.
  * @rx_skb:   SK_buffer to store the received data into.
  *
  * Contains all necessary information for receiving and packaging data on the
  * UART.
  */
struct ccd_uart_rx {
	enum ccd_uart_rx_state	rx_state;
	unsigned long		rx_count;
	struct sk_buff		*rx_skb;
};

/**
  * struct ccd_uart - UART info structure.
  * @tty:      TTY info structure.
  * @tx_wq:    TX working queue.
  * @rx_lock:  RX spin lock.
  * @rx_info:  UART RX info structure.
  * @tx_mutex: TX mutex.
  *
  * Contains all necessary information for the UART transport.
  */
struct ccd_uart {
	struct tty_struct	*tty;
	struct workqueue_struct	*tx_wq;
	spinlock_t		rx_lock;
	struct ccd_uart_rx	rx_info;
	struct mutex		tx_mutex;
};


/**
  * struct ccd_work_struct - Work structure for CCD module.
  * @work: Work structure.
  * @data: Pointer to private data.
  *
  * This structure is used to pack work for work queue.
  */
struct ccd_work_struct{
	struct work_struct work;
	void *data;
};

/**
  * struct test_char_dev_info - Stores device information.
  * @test_miscdev: Registered Misc Device.
  * @rx_queue:     RX data queue.
  */
struct test_char_dev_info {
	struct miscdevice	test_miscdev;
	struct sk_buff_head	rx_queue;
};

/**
  * struct test_char_dev_info - Main CCD info structure.
  * @ccd_class:       Class for the STE_CONN driver.
  * @dev:             Parent device for the STE_CONN driver.
  * @state:           CCD main state.
  * @sleep_state:     CCD sleep state.
  * @baud_rate_state: CCD baud rate change state.
  * @baud_rate: current baud rate setting.
  * @wq:              CCD work queue.
  * @timer:           CCD timer (for chip sleep).
  * @tx_queue:        TX queue for sending data to chip.
  * @c_uart:          UART info structure.
  * @char_test_dev:   Character device for testing purposes.
  * @h4_channels:     Stored H:4 channels.
  */
struct ccd_info {
	struct class			*ccd_class;
	struct device			*dev;
	enum ccd_state		state;
	enum ccd_sleep_state		sleep_state;
	enum ccd_baud_rate_change_state	baud_rate_state;
	int baud_rate;
	struct workqueue_struct		*wq;
	struct timer_list		timer;
	struct sk_buff_head		tx_queue;
	struct ccd_uart		*c_uart;
	struct test_char_dev_info	*char_test_dev;
	struct ste_conn_ccd_h4_channels	h4_channels;
};

#if 0
static int char_dev_usage = STE_CONN_ALL_CHAR_DEVS;
#else
static int char_dev_usage = STE_CONN_CHAR_DEV_GNSS | STE_CONN_CHAR_DEV_HCI_LOGGER |
	STE_CONN_CHAR_DEV_US_CTRL | STE_CONN_CHAR_DEV_BT_AUDIO | STE_CONN_CHAR_DEV_FM_RADIO_AUDIO | STE_CONN_CHAR_DEV_CORE;
#endif

static struct ccd_info *ccd_info;
static enum ccd_transport ste_conn_transport = CCD_TRANSPORT_UNKNOWN;
static int uart_default_baud = CCD_DEFAULT_BAUD_RATE;
static int uart_high_baud = CCD_HIGH_BAUD_RATE;
int ste_debug_level = STE_CONN_DEFAULT_DEBUG_LEVEL;
EXPORT_SYMBOL(ste_debug_level);
uint8_t ste_conn_bd_address[] = {0x00, 0x80, 0xDE, 0xAD, 0xBE, 0xEF};
EXPORT_SYMBOL(ste_conn_bd_address);
int bd_addr_count = BT_BDADDR_SIZE;

/* Setting default values to ST-E STLC2690 */
int ste_conn_default_manufacturer = 0x30;
EXPORT_SYMBOL(ste_conn_default_manufacturer);
int ste_conn_default_hci_revision = 0x0700;
EXPORT_SYMBOL(ste_conn_default_hci_revision);
int ste_conn_default_sub_version = 0x0011;
EXPORT_SYMBOL(ste_conn_default_sub_version);

/* Global parameter set to NULL by default */
static int use_sleep_timeout;
/* Global parameter set to NULL by default */
static unsigned long ccd_timeout_jiffies;

/* CCD wait queues */
static DECLARE_WAIT_QUEUE_HEAD(test_char_rx_wait_queue);
static DECLARE_WAIT_QUEUE_HEAD(ste_conn_ccd_wait_queue);

/* Time structures */
static struct timeval time_100ms = {
	.tv_sec = 0,
	.tv_usec = 100 * USEC_PER_MSEC
};

static struct timeval time_500ms = {
	.tv_sec = 0,
	.tv_usec = 500 * USEC_PER_MSEC
};

static struct timeval time_1s = {
	.tv_sec = 1,
	.tv_usec = 0
};

/* UART internal functions */
static int		uart_set_baud_rate(int baud);
static void		uart_finish_setting_baud_rate(struct tty_struct *tty);
static void		uart_send_skb_to_cpd(struct sk_buff *skb);
static int		uart_send_skb_to_chip(struct sk_buff *skb);
static int		uart_tx_wakeup(struct ccd_uart *c_uart);
static void		work_uart_do_transmit(struct work_struct *work);
static int		uart_tty_open(struct tty_struct *tty);
static void		uart_tty_close(struct tty_struct *tty);
static void		uart_tty_wakeup(struct tty_struct *tty);
static void		uart_tty_receive(struct tty_struct *tty, const uint8_t *data, char *flags, int count);
static int		uart_tty_ioctl(struct tty_struct *tty, struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t		uart_tty_read(struct tty_struct *tty, struct file *file, unsigned char __user *buf, size_t nr);
static ssize_t		uart_tty_write(struct tty_struct *tty, struct file *file, const unsigned char *data, size_t count);
static unsigned int	uart_tty_poll(struct tty_struct *tty, struct file *filp, poll_table *wait);
static void		uart_check_data_len(struct ccd_uart_rx *rx_info, int len);
static int		uart_receive_skb(struct ccd_uart *c_uart, const uint8_t *data, int count);

/* Generic internal functions */
static void		ccd_work_hw_registered(struct work_struct *work);
static void		ccd_work_hw_deregistered(struct work_struct *work);
static int		ccd_create_work_item(struct workqueue_struct *wq, work_func_t work_func, void *data);
static void		update_timer(void);
static void		ccd_timer_expired(unsigned long data);
static struct sk_buff	*alloc_rx_skb(unsigned int size, gfp_t priority);
static int		test_char_device_create(void);
static void		test_char_device_destroy(void);
static ssize_t		test_char_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t		test_char_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static unsigned int	test_char_dev_poll(struct file *filp, poll_table *wait);
static void		test_char_dev_tx_received(struct sk_buff_head *rx_queue, struct sk_buff *skb);
static int __init ste_conn_init(void);
static void __exit ste_conn_exit(void);
static struct sk_buff *alloc_set_baud_rate_cmd(int baud);

/*
  * struct test_char_dev_fops - Test char devices file operations.
  * @read: Function that reads from the char device.
  * @write: Function that writes to the char device.
  * @poll: Function that handles poll call to the fd.
 */
static struct file_operations test_char_dev_fops = {
	.read = test_char_dev_read,
	.write = test_char_dev_write,
	.poll = test_char_dev_poll
};


/* --------------- Exported functions ------------------------- */

int ste_conn_ccd_open(void)
{
	int err = 0;

	STE_CONN_INFO("ste_conn_ccd_open");

	switch (ste_conn_transport) {
	case CCD_TRANSPORT_UART:
		{
			uint8_t data[CCD_HCI_RESET_LEN + CCD_H4_HEADER_LEN];
			struct tty_struct *tty = NULL;
			int bytes_written;

			/* Chip has just been started up. It has a system to autodetect
			 * exact baud rate and transport to use. There are only a few commands
			 * it will recognize and HCI Reset is one of them.
			 * We therefore start with sending that before actually changing baud rate.
			 *
			 * Create the Hci_Reset packet
			 */
			data[0] = (uint8_t)ccd_info->h4_channels.bt_cmd_channel;
			data[1] = CCD_HCI_RESET_LSB;
			data[2] = CCD_HCI_RESET_MSB;
			data[3] = CCD_HCI_RESET_PAYL_LEN;

			/* Get the TTY info and send the packet */
			tty = ccd_info->c_uart->tty;
			ccd_info->baud_rate_state = BAUD_SENDING_RESET;
			STE_CONN_DBG("Sending HCI reset before baud rate change");
			bytes_written = tty->ops->write(tty, data, CCD_HCI_RESET_LEN + CCD_H4_HEADER_LEN);
			if (bytes_written != CCD_HCI_RESET_LEN + CCD_H4_HEADER_LEN) {
				STE_CONN_ERR("Only wrote %d bytes", bytes_written);
				ccd_info->baud_rate_state = BAUD_IDLE;
				err = -EACCES;
				goto finished;
			}

			/* Wait for command complete. If error, exit without changing baud rate. */
			wait_event_interruptible_timeout(ste_conn_ccd_wait_queue,
						BAUD_IDLE == ccd_info->baud_rate_state,
						timeval_to_jiffies(&time_500ms));
			if (BAUD_IDLE != ccd_info->baud_rate_state) {
				STE_CONN_ERR("Failed to send HCI Reset");
				ccd_info->baud_rate_state = BAUD_IDLE;
				err = -EIO;
				goto finished;
			}

			err = uart_set_baud_rate(uart_high_baud);
		}
		break;
	case CCD_TRANSPORT_CHAR_DEV:
		/* Nothing to be done in test mode. */
		break;
	default:
		STE_CONN_ERR("Unknown ste_conn_transport: 0x%X", ste_conn_transport);
		err = -EIO;
		goto finished;
		break;
	};

	if (!err) {
		ccd_info->state = CCD_STATE_OPENED;
	}

finished:
	return err;
}

void ste_conn_ccd_close(void)
{
	STE_CONN_INFO("ste_conn_ccd_close");

	ccd_info->state = CCD_STATE_CLOSED;

	switch (ste_conn_transport) {
	case CCD_TRANSPORT_UART:
		/* The chip is already shut down. Power off the chip. */
		ste_conn_ccd_set_chip_power(false);

		break;
	case CCD_TRANSPORT_CHAR_DEV:
		/* Nothing to be done in test mode. */
		break;
	default:
		STE_CONN_ERR("Unknown ste_conn_transport: 0x%X", ste_conn_transport);
		break;
	};
}

void ste_conn_ccd_set_chip_power(bool chip_on)
{
	STE_CONN_INFO("ste_conn_ccd_set_chip_power: %s", (chip_on ? "ENABLE" : "DISABLE"));

	switch (ste_conn_transport) {
	case CCD_TRANSPORT_UART: {
		int uart_baudrate = uart_default_baud;
		struct ktermios *old_termios;
		struct tty_struct *tty = NULL;

		if (chip_on) {
			ste_conn_devices_enable_chip();
		} else {
			ste_conn_devices_disable_chip();
			/* Setting baud rate to 0 will tell UART driver to shut off its clocks.*/
			uart_baudrate = CCD_ZERO_BAUD_RATE;
		}

		/* Now we have to set the digital baseband UART
		 * to default baudrate if chip is ON or to zero baudrate if
		 * chip is turning OFF.
		 */
		if (ccd_info->c_uart && ccd_info->c_uart->tty) {
			tty = ccd_info->c_uart->tty;
		} else {
			STE_CONN_ERR("Important structs not allocated!");
			return;
		}

		old_termios = kmalloc(sizeof(*old_termios), GFP_ATOMIC);
		if (!old_termios) {
			STE_CONN_ERR("Could not allocate termios");
			return;
		}

		mutex_lock(&(tty->termios_mutex));
		/* Now set the termios struct to the default or zero baudrate.
		 * Start by storing the old termios.
		 */
		memcpy(old_termios, tty->termios, sizeof(*old_termios));
		/* Now set the default baudrate or zero if chip is turning OFF.*/
		tty_encode_baud_rate(tty, uart_baudrate,
					uart_baudrate);
		/* Finally inform the driver */
		if (tty->ops->set_termios) {
			tty->ops->set_termios(tty, old_termios);
		}
		mutex_unlock(&(tty->termios_mutex));
		kfree(old_termios);
		break;
	}
	case CCD_TRANSPORT_CHAR_DEV:
		/* Nothing to be done in test mode. */
		break;
	default:
		STE_CONN_ERR("Unknown ste_conn_transport: 0x%X", ste_conn_transport);
		break;
	};
}

int ste_conn_ccd_write(struct sk_buff *skb)
{
	int err = 0;
	STE_CONN_INFO("ste_conn_ccd_write");

	if (CCD_STATE_CLOSED == ccd_info->state) {
		STE_CONN_ERR("Trying to write on a closed channel");
		return -EPERM; /* Operation not permitted */
	}

	/* (Re)start sleep timer. */
	update_timer();

	STE_CONN_DBG_DATA_CONTENT("Length: %d Data: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", skb->len,
					skb->data[0], skb->data[1], skb->data[2], skb->data[3], skb->data[4],
					skb->data[5], skb->data[6], skb->data[7], skb->data[8], skb->data[9]);

	/* Pass the buffer to physical transport. */
	switch (ste_conn_transport) {
	case CCD_TRANSPORT_UART:
		err = uart_send_skb_to_chip(skb);
		break;
	case CCD_TRANSPORT_CHAR_DEV:
		/* Test mode enabled, pass the buffer to the test char device. */
		test_char_dev_tx_received(&ccd_info->char_test_dev->rx_queue, skb);
		break;
	default:
		STE_CONN_ERR("Unknown ste_conn_transport: 0x%X", ste_conn_transport);
		break;
	};
	return err;
}


/* -------------- Internal functions --------------------------- */


/* -------------- UART functions --------------------------- */

/**
 * uart_set_baud_rate() - Sets new baud rate for the UART.
 * @baud: New baud rate.
 *
 * This function first sends the HCI command
 * Hci_Cmd_ST_Set_Uart_Baud_Rate. It then changes the baud rate in HW, and
 * finally it waits for the Command Complete event for the
 * Hci_Cmd_ST_Set_Uart_Baud_Rate command.
 *
 * Returns:
 *   0 if there is no error.
 *   -EALREADY if baud rate change is already in progress.
 *   -EFAULT if one or more of the UART related structs is not allocated.
 *   -ENOMEM if skb allocation has failed.
 *   -EPERM if setting the new baud rate has failed.
 *   Error codes generated by uart_send_skb_to_chip.
 */
static int uart_set_baud_rate(int baud)
{
	struct tty_struct *tty = NULL;
	int err = 0;
	struct sk_buff *skb;
	int old_baud_rate;

	STE_CONN_INFO("uart_set_baud_rate (%d baud)", baud);

	if (ccd_info->baud_rate_state != BAUD_IDLE) {
		STE_CONN_ERR("Trying to set new baud rate before old setting is finished");
		err = -EALREADY;
		goto finished;
	}

	if (ccd_info->c_uart && ccd_info->c_uart->tty) {
		tty = ccd_info->c_uart->tty;
	} else {
		STE_CONN_ERR("Important structs not allocated!");
		err = -EFAULT;
		goto finished;
	}

	/* Store old baud rate so that we can restore it if something goes wrong. */
	old_baud_rate = ccd_info->baud_rate;

	skb = alloc_set_baud_rate_cmd(baud);
	if (!skb) {
		err = -ENOMEM;
		STE_CONN_ERR("alloc_set_baud_rate_cmd failed");
		goto finished;
	}

	ccd_info->baud_rate_state = BAUD_START;
	ccd_info->baud_rate = baud;

	err = uart_send_skb_to_chip(skb);
	if (err) {
		STE_CONN_ERR("Failed to send change baud rate cmd!");
		ccd_info->baud_rate_state = BAUD_IDLE;
		ccd_info->baud_rate = old_baud_rate;
		goto finished;
	} else {
		STE_CONN_DBG("Set baud rate cmd scheduled for sending.");
	}

	/* Now wait for the command complete. It will come at the new baudrate */
	wait_event_interruptible_timeout(ste_conn_ccd_wait_queue,
					((BAUD_SUCCESS == ccd_info->baud_rate_state) ||
					 (BAUD_FAIL    == ccd_info->baud_rate_state)),
					timeval_to_jiffies(&time_1s));
	if (BAUD_SUCCESS == ccd_info->baud_rate_state) {
		STE_CONN_DBG("Baudrate changed to %d baud", baud);
	} else {
		STE_CONN_ERR("Failed to set new baudrate (%d)", ccd_info->baud_rate_state);
		err = -EPERM;
	}

	/* Finally flush the TTY so we are sure that is no bad data there */
	if (tty->ops->flush_buffer) {
		STE_CONN_DBG("Flushing TTY after baud rate change");
		tty->ops->flush_buffer(tty);
	}

	/* Finished. Set state to IDLE */
	ccd_info->baud_rate_state = BAUD_IDLE;

finished:
	return err;
}

/**
 * alloc_set_baud_rate_cmd() - Allocates new sk_buff and fills in the change baud rate hci cmd.
 * @baud: requested new baud rate.
 *
 * Returns:
 *   pointer to allocated sk_buff if successful;
 *   NULL otherwise.
 */
static struct sk_buff *alloc_set_baud_rate_cmd(int baud)
{
	struct sk_buff *skb;
	uint8_t data[CCD_SET_BAUD_RATE_LEN];
	uint8_t *h4;

	skb = ste_conn_alloc_skb(CCD_SET_BAUD_RATE_LEN, GFP_ATOMIC);
	if (skb) {
		/* Create the Hci_Cmd_ST_Set_Uart_Baud_Rate packet */
		data[0] = CCD_SET_BAUD_RATE_LSB;
		data[1] = CCD_SET_BAUD_RATE_MSB;
		data[2] = CCD_SET_BAUD_RATE_PAYL_LEN;

		switch (baud) {
		case 57600:
			data[3] = CCD_BAUD_RATE_57600;
			break;
		case 115200:
			data[3] = CCD_BAUD_RATE_115200;
			break;
		case 230400:
			data[3] = CCD_BAUD_RATE_230400;
			break;
		case 460800:
			data[3] = CCD_BAUD_RATE_460800;
			break;
		case 921600:
			data[3] = CCD_BAUD_RATE_921600;
			break;
		case 2000000:
			data[3] = CCD_BAUD_RATE_2000000;
			break;
		case 3000000:
			data[3] = CCD_BAUD_RATE_3000000;
			break;
		case 4000000:
			data[3] = CCD_BAUD_RATE_4000000;
			break;
		default:
			STE_CONN_ERR("Invalid speed requested (%d), using 115200 bps instead\n", baud);
			data[3] = CCD_BAUD_RATE_115200;
			baud = 115200;
			break;
		}
		memcpy(skb_put(skb, CCD_SET_BAUD_RATE_LEN), data, CCD_SET_BAUD_RATE_LEN);
		h4 = skb_push(skb, CCD_H4_HEADER_LEN);
		*h4 = (uint8_t)ccd_info->h4_channels.bt_cmd_channel;
	} else {
		STE_CONN_ERR("Failed to alloc skb!");
	}
	return skb;
}

/**
 * is_set_baud_rate_cmd() - Checks if data contains set baud rate hci cmd.
 * @data: Pointer to data array to check.
 *
 * Returns:
 *   true - if cmd found;
 *   false - otherwise.
 */
static bool is_set_baud_rate_cmd(const char *data)
{
	bool cmd_match = false;

	if ((data[0] == ccd_info->h4_channels.bt_cmd_channel) &&
		(data[1] == CCD_SET_BAUD_RATE_LSB) &&
		(data[2] == CCD_SET_BAUD_RATE_MSB) &&
		(data[3] == CCD_SET_BAUD_RATE_PAYL_LEN)) {
		cmd_match = true;
	}
	return cmd_match;
}


/**
 * uart_send_skb_to_cpd() - Sends packet received from UART to CPD.
 * @skb: Received data packet.
 *
 * This function checks if CCD is waiting for Command complete event,
 * see uart_set_baud_rate.
 * If it is waiting it checks if it is the expected packet and the status.
 * If not is passes the packet to CPD.
 */
static void uart_send_skb_to_cpd(struct sk_buff *skb)
{
	if (!skb) {
		STE_CONN_ERR("Received NULL as skb");
		return;
	}

	if (BAUD_WAITING == ccd_info->baud_rate_state) {
		/* Should only really be one packet received now:
		 * the CmdComplete for the SetBaudrate command
		 * Let's see if this is the packet we are waiting for.
		 */
		if ((ccd_info->h4_channels.bt_evt_channel == skb->data[0]) &&
		    (HCI_BT_EVT_CMD_COMPLETE == skb->data[1]) &&
		    (HCI_BT_EVT_CMD_COMPLETE_LEN == skb->data[2]) &&
		    (CCD_SET_BAUD_RATE_LSB == skb->data[4]) &&
		    (CCD_SET_BAUD_RATE_MSB == skb->data[5])) {
			/* We have received complete event for our baud rate
			 * change command
			 */
			if (HCI_BT_ERROR_NO_ERROR == skb->data[6]) {
				STE_CONN_DBG("Received baud rate change complete event OK");
				ccd_info->baud_rate_state = BAUD_SUCCESS;
			} else {
				STE_CONN_ERR("Received baud rate change complete event with status 0x%X", skb->data[6]);
				ccd_info->baud_rate_state = BAUD_FAIL;
			}
			wake_up_interruptible(&ste_conn_ccd_wait_queue);
			kfree_skb(skb);
		} else {
			/* Received other event. Should not really happen,
			 * but pass the data to CPD anyway.
			 */
			STE_CONN_DBG_DATA("Sending packet to CPD while waiting for CmdComplete");
			ste_conn_cpd_data_received(skb);
		}
	} else if (BAUD_SENDING_RESET == ccd_info->baud_rate_state) {
		/* Should only really be one packet received now:
		 * the CmdComplete for the Reset command
		 * Let's see if this is the packet we are waiting for.
		 */
		if ((ccd_info->h4_channels.bt_evt_channel == skb->data[0]) &&
			(HCI_BT_EVT_CMD_COMPLETE == skb->data[1]) &&
			(HCI_BT_EVT_CMD_COMPLETE_LEN == skb->data[2]) &&
			(CCD_HCI_RESET_LSB == skb->data[4]) &&
			(CCD_HCI_RESET_MSB == skb->data[5])) {
			/* We have received complete event for our baud rate
			 * change command
			 */
			if (HCI_BT_ERROR_NO_ERROR == skb->data[6]) {
				STE_CONN_DBG("Received HCI reset complete event OK");
				/* Go back to BAUD_IDLE since this was not really baud rate change
				 * but just a preparation of the chip to be ready to receive commands.
				 */
				ccd_info->baud_rate_state = BAUD_IDLE;
			} else {
				STE_CONN_ERR("Received HCI reset complete event with status 0x%X", skb->data[6]);
				ccd_info->baud_rate_state = BAUD_FAIL;
			}
			wake_up_interruptible(&ste_conn_ccd_wait_queue);
			kfree_skb(skb);
		} else {
			/* Received other event. Should not really happen,
			 * but pass the data to CPD anyway.
			 */
			STE_CONN_DBG_DATA("Sending packet to CPD while waiting for CmdComplete");
			ste_conn_cpd_data_received(skb);
		}
	} else {
		/* Just pass data to CPD */
		ste_conn_cpd_data_received(skb);
	}
}

/**
 * uart_send_skb_to_chip() - Transmit data packet to connectivity controller over UART.
 * @skb: Data packet.
 *
 * Returns:
 *   0 if there is no error.
 *   -ENODEV if UART has not been opened.
 *   Error codes from uart_tx_wakeup.
 */
static int uart_send_skb_to_chip(struct sk_buff *skb)
{
	int err = 0;

	if (!(ccd_info->c_uart)) {
		STE_CONN_ERR("Trying to transmit on an unopened UART");
		return -ENODEV;
	}

	/* Queue the sk_buffer... */
	skb_queue_tail(&ccd_info->tx_queue, skb);

	/* ... and call the common UART TX function */
	err = uart_tx_wakeup(ccd_info->c_uart);

	if (err) {
		STE_CONN_ERR("Failed to queue skb transmission over uart, freeing skb.");
		skb = skb_dequeue_tail(&ccd_info->tx_queue);
		kfree_skb(skb);
	}

	return err;
}

/**
 * uart_tx_wakeup() - Creates a transmit data work item that will transmit data packet to connectivity controller over UART.
 * @c_uart: connectivity UART structure.
 *
 * Returns:
 *   0 if success.
 *   Error code from ccd_create_work_item.
 */

static int uart_tx_wakeup(struct ccd_uart *c_uart)
 {
	int err = 0;

	err = ccd_create_work_item(c_uart->tx_wq, work_uart_do_transmit, (void *)c_uart);

	return err;
}

/**
 * work_uart_do_transmit() - Transmit data packet to connectivity controller over UART.
 * @work: Pointer to work info structure. Contains ccd_uart structure pointer.
 */
static void work_uart_do_transmit(struct work_struct *work)
{
	struct sk_buff *skb;
	struct tty_struct *tty = NULL;
	struct ccd_uart *c_uart = NULL;
	struct ccd_work_struct *current_work = NULL;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct ccd_work_struct, work);

	c_uart = (struct ccd_uart *)(current_work->data);

	if (c_uart && c_uart->tty) {
		tty = c_uart->tty;
	} else {
		STE_CONN_ERR("Important structs not allocated!");
		goto finished;
	}

	mutex_lock(&c_uart->tx_mutex);

	/* Retreive the first packet in the queue */
	skb = skb_dequeue(&ccd_info->tx_queue);
	while (skb) {
		int len;

		/* Tell TTY that there is data waiting and call the write function */
		set_bit(TTY_DO_WRITE_WAKEUP, &tty->flags);
		len = tty->ops->write(tty, skb->data, skb->len);
		STE_CONN_INFO("Written %d bytes to UART of %d bytes in packet", len, skb->len);

		/* If it's set baud rate cmd set correct baud state and after sending is finished
		 * inform the tty driver about the new baud rate.
		 */
		if ((BAUD_START == ccd_info->baud_rate_state) &&
			(is_set_baud_rate_cmd(skb->data))) {
			STE_CONN_INFO("UART set baud rate cmd found.");
			ccd_info->baud_rate_state = BAUD_SENDING;
		}

		/* Remove the bytes written from the sk_buffer */
		skb_pull(skb, len);

		/* If there is more data in this sk_buffer, put it at the start of the list
		 * and exit the loop
		 */
		if (skb->len) {
			skb_queue_head(&ccd_info->tx_queue, skb);
			break;
		}
		/* No more data in the sk_buffer. Free it and get next packet in queue.
		 * Check if set baud rate cmd is in sending progress, if so call proper
		 * function to handle that cmd since it requires special attention.
		 */
		if (BAUD_SENDING == ccd_info->baud_rate_state) {
			uart_finish_setting_baud_rate(tty);
		}

		kfree_skb(skb);
		skb = skb_dequeue(&ccd_info->tx_queue);
	}

	mutex_unlock(&c_uart->tx_mutex);

finished:
	kfree(current_work);
}

/**
 * uart_finish_setting_baud_rate() - handles sending the ste baud rate hci cmd.
 * @tty: pointer to a tty_struct used to communicate with tty driver.
 *
 * uart_finish_setting_baud_rate() makes sure that the set baud rate cmd has been really
 * sent out on the wire and then switches the tty driver to new baud rate.
 */
static void uart_finish_setting_baud_rate(struct tty_struct *tty)
{
	struct ktermios *old_termios;

	/* Give the tty driver 100 msec to send data and proceed, if it hasn't been send
	 * we can't do much about it anyway.
	 */
	schedule_timeout_interruptible(timeval_to_jiffies(&time_100ms));

	old_termios = kmalloc(sizeof(*old_termios), GFP_ATOMIC);
	if (!old_termios) {
		STE_CONN_ERR("Could not allocate termios");
		return;
	}
	mutex_lock(&(tty->termios_mutex));

	/* Now set the termios struct to the new baudrate. Start by storing
	 * the old termios.
	 */
	memcpy(old_termios, tty->termios, sizeof(*old_termios));
	/* Then set the new baudrate into the TTY struct */
	tty_encode_baud_rate(tty, ccd_info->baud_rate, ccd_info->baud_rate);
	/* Finally inform the driver */
	if (tty->ops->set_termios) {
		STE_CONN_DBG("Setting termios to new baud rate");
		tty->ops->set_termios(tty, old_termios);
		ccd_info->baud_rate_state = BAUD_WAITING;
	} else {
		STE_CONN_ERR("Not possible to set new baud rate");
		ccd_info->baud_rate_state = BAUD_IDLE;
	}
	mutex_unlock(&(tty->termios_mutex));
	kfree(old_termios);
}


/**
 * uart_tty_open() - Called when UART line discipline changed to N_HCI.
 * @tty: Pointer to associated TTY instance data.
 *
 * Returns:
 *   0 if there is no error.
 *   -EEXIST if UART was already opened.
 *   -ENOMEM if allocation fails.
 *   -EBUSY if another transport was already opened.
 *   -ECHILD if work queue could not be created.
 */
static int uart_tty_open(struct tty_struct *tty)
{
	struct ccd_uart *c_uart = (struct ccd_uart *) tty->disc_data;
	int err = 0;

	STE_CONN_INFO("uart_tty_open");

	if (c_uart) {
		err = -EEXIST;
		goto finished;
	}

	/* Allocate the CCD UART structure */
	c_uart = kzalloc(sizeof(*c_uart), GFP_ATOMIC);
	if (!c_uart) {
		STE_CONN_ERR("Can't allocate control structure");
		err = -ENOMEM;
		goto finished;
	}

	/* Initialize the tx_mutex here. It will be destroyed in uart_tty_close. */
	mutex_init(&c_uart->tx_mutex);

	/* Set the structure pointers and set the UART receive room */
	tty->disc_data = c_uart;
	c_uart->tty = tty;
	ccd_info->c_uart = c_uart;
	tty->receive_room = CCD_UART_RECEIVE_ROOM;

	/* Initialize the spin lock */
	spin_lock_init(&c_uart->rx_lock);

	/* Flush any pending characters in the driver and line discipline.
	* Don't use ldisc_ref here as the open path is before the ldisc is referencable
	*/

	if (tty->ldisc->ops->flush_buffer) {
		tty->ldisc->ops->flush_buffer(tty);
	}

	tty_driver_flush_buffer(tty);

	ccd_info->dev->parent = NULL;

	if (CCD_TRANSPORT_UNKNOWN == ste_conn_transport) {
		ste_conn_transport = CCD_TRANSPORT_UART;
	} else {
		STE_CONN_ERR(
		"Trying to set transport to UART when transport 0x%X already registered",
		(uint32_t)ste_conn_transport);
		err = -EBUSY;
		kfree(c_uart);
		goto finished;
	}

	/*Init uart tx workqueue */
	ccd_info->c_uart->tx_wq = create_singlethread_workqueue(STE_CONN_UART_TX_WQ_NAME);
	if (!ccd_info->c_uart->tx_wq) {
		STE_CONN_ERR("Could not create workqueue");
		err = -ECHILD; /* No child processes */
		goto finished;
	}

	/* Tell CPD that HW is connected */
	err = ccd_create_work_item(ccd_info->wq, ccd_work_hw_registered, NULL);

finished:
	return err;
}

/**
 * uart_tty_close() - Close UART tty.
 * @tty: Pointer to associated TTY instance data.
 *
 * The uart_tty_close() function is called when the line discipline is changed to something
 * else, the TTY is closed, or the TTY detects a hangup.
 */
static void uart_tty_close(struct tty_struct *tty)
{
	struct ccd_uart *c_uart = (struct ccd_uart *)tty->disc_data;

	STE_CONN_INFO("uart_tty_close");

	/* We can now destroy the tx_mutex initialized in uart_tty_open. */
	mutex_destroy(&c_uart->tx_mutex);

	/*Close uart rx work queue*/
	if (c_uart && c_uart->tx_wq) {
		destroy_workqueue(c_uart->tx_wq);
	} else {
		STE_CONN_ERR("Important structs not allocated!");
		return;
	}

	/* Detach from the TTY */
	tty->disc_data = NULL;
	c_uart->tty = NULL;

	/* Tell CPD that HW is disconnected */
	if (ccd_info) {
		(void)ccd_create_work_item(ccd_info->wq, ccd_work_hw_deregistered, (void *)c_uart);
	} else {
		STE_CONN_ERR("Important structs not allocated!");
	}
}

/**
 * uart_tty_wakeup() - callback function for transmit wakeup.
 * @tty: Pointer to associated TTY instance data.
 *
 * The uart_tty_wakeup() callback function is called when low level
 * device driver can accept more send data.
 */
static void uart_tty_wakeup(struct tty_struct *tty)
{
	struct ccd_uart *c_uart = (struct ccd_uart *)tty->disc_data;

	STE_CONN_INFO("uart_tty_wakeup");

	if (!c_uart) {
		return;
	}

	/* Clear the TTY_DO_WRITE_WAKEUP bit that is set in work_uart_do_transmit(). */
	clear_bit(TTY_DO_WRITE_WAKEUP, &tty->flags);

	if (tty != c_uart->tty) {
		return;
	}

	/* Start TX operation */
	uart_tx_wakeup(c_uart);
}

/**
 * uart_tty_receive() - Called by TTY low level driver when receive data is available.
 * @tty:   Pointer to TTY instance data
 * @data:  Pointer to received data
 * @flags: Pointer to flags for data
 * @count: Count of received data in bytes
 */
static void uart_tty_receive(struct tty_struct *tty, const uint8_t *data, char *flags, int count)
{
	struct ccd_uart *c_uart = (struct ccd_uart *)tty->disc_data;

	STE_CONN_INFO("uart_tty_receive");

	if (!c_uart || (tty != c_uart->tty)) {
		return;
	}

	STE_CONN_DBG_DATA("Received data with length = %d and first byte 0x%02X", count, data[0]);
	STE_CONN_DBG_DATA_CONTENT("Data: %02X %02X %02X %02X %02X %02X %02X", data[0], data[1], data[2], data[3], data[4], data[5], data[6]);

	/* Restart data ccd timer */
	spin_lock(&c_uart->rx_lock);
	update_timer();
	uart_receive_skb(c_uart, data, count);
	spin_unlock(&c_uart->rx_lock);

	/* Open TTY for more data */
	tty_unthrottle(tty);
}

/**
 * uart_tty_ioctl() -  Process IOCTL system call for the TTY device.
 * @tty:   Pointer to TTY instance data.
 * @file:  Pointer to open file object for device.
 * @cmd:   IOCTL command code.
 * @arg:   Argument for IOCTL call (cmd dependent).
 *
 * Returns:
 *   0 if there is no error.
 *   -EBADF if supplied TTY struct is not correct.
 *   Error codes from n_tty_iotcl_helper.
 */
static int uart_tty_ioctl(struct tty_struct *tty, struct file *file,
							unsigned int cmd, unsigned long arg)
{
	struct ccd_uart *c_uart = (struct ccd_uart *)tty->disc_data;
	int err = 0;

	STE_CONN_INFO("uart_tty_ioctl cmd %d", cmd);
	STE_CONN_DBG("DIR: %d, TYPE: %d, NR: %d, SIZE: %d", _IOC_DIR(cmd), _IOC_TYPE(cmd), _IOC_NR(cmd), _IOC_SIZE(cmd));

	/* Verify the status of the device */
	if (!c_uart) {
		return -EBADF;
	}

	switch (cmd) {
	case HCIUARTSETPROTO:
		/* We don't do anything special here, but we have to show we handle it */
		break;

	case HCIUARTGETPROTO:
		/* We don't do anything special here, but we have to show we handle it */
		break;

	case HCIUARTGETDEVICE:
		/* We don't do anything special here, but we have to show we handle it */
		break;

	default:
		err = n_tty_ioctl_helper(tty, file, cmd, arg);
		break;
	};

	return err;
}

/*
 * We don't provide read/write/poll interface for user space.
 */
static ssize_t uart_tty_read(struct tty_struct *tty, struct file *file,
				unsigned char __user *buf, size_t nr)
{
	STE_CONN_INFO("uart_tty_read");
	return 0;
}

static ssize_t uart_tty_write(struct tty_struct *tty, struct file *file,
				const unsigned char *data, size_t count)
{
	STE_CONN_INFO("uart_tty_write");
	return count;
}

static unsigned int uart_tty_poll(struct tty_struct *tty,
					struct file *filp, poll_table *wait)
{
	return 0;
}

/**
 * uart_check_data_len() - Check number of bytes to receive.
 * @rx_info: UART RX info structure.
 * @len:     Number of bytes left to receive.
 */
static void uart_check_data_len(struct ccd_uart_rx *rx_info, int len)
{
	/* First get number of bytes left in the sk_buffer */
	register int room = skb_tailroom(rx_info->rx_skb);

	if (!len) {
		/* No data left to receive. Transmit to CPD */
		uart_send_skb_to_cpd(rx_info->rx_skb);
	} else if (len > room) {
		STE_CONN_ERR("Data length is too large");
		kfree_skb(rx_info->rx_skb);
	} else {
		/* "Normal" case. Switch to data receiving state and store data length */
		rx_info->rx_state = W4_DATA;
		rx_info->rx_count = len;
		return;
	}

	rx_info->rx_state = W4_PACKET_TYPE;
	rx_info->rx_skb   = NULL;
	rx_info->rx_count = 0;
}

/**
 * uart_receive_skb() - Handles received UART data.
 * @c_uart: CCD UART info structure
 * @data:   Data received
 * @count:  Number of bytes received
 *
 * The uart_receive_skb() function handles received UART data and puts it together
 * to one complete packet.
 *
 * Returns:
 *   Number of bytes not handled, i.e. 0 = no error.
 */
static int uart_receive_skb(struct ccd_uart *c_uart, const uint8_t *data, int count)
{
	struct ccd_uart_rx *rx_info = NULL;
	const uint8_t *r_ptr;
	uint8_t *w_ptr;
	uint8_t len_8;
	uint16_t len_16;
	int len;

	if (c_uart) {
		rx_info = &(c_uart->rx_info);
	} else {
		STE_CONN_ERR("Important structs not allocated!");
		return count;
	}

	r_ptr = data;
	/* Continue while there is data left to handle */
	while (count) {
		/* If we have already received a packet we know how many bytes there are left */
		if (rx_info->rx_count) {
			/* First copy received data into the skb_rx */
			len = min_t(unsigned int, rx_info->rx_count, count);
			memcpy(skb_put(rx_info->rx_skb, len), r_ptr, len);
			/* Update counters from the length and step the data pointer */
			rx_info->rx_count -= len;
			count -= len;
			r_ptr += len;

			if (rx_info->rx_count) {
				/* More data to receive to current packet. Continue */
				continue;
			}

			/* Handle the different states */
			switch (rx_info->rx_state) {
			case W4_DATA:
				/* Whole data packet has been received. Transmit it to CPD */
				uart_send_skb_to_cpd(rx_info->rx_skb);

				rx_info->rx_state = W4_PACKET_TYPE;
				rx_info->rx_skb = NULL;
				continue;

			case W4_EVENT_HDR:
				/* Event op code is not used */
				len_8 = rx_info->rx_skb->data[CCD_HCI_EVT_LEN_POS];
				uart_check_data_len(rx_info, len_8);
				/* Header read. Continue with next bytes */
				continue;

			case W4_ACL_HDR:
				/* Handle is not used */
				len_16 = __le16_to_cpu((uint16_t)((rx_info->rx_skb->data[CCD_HCI_ACL_LEN_POS]) |
								  (rx_info->rx_skb->data[CCD_HCI_ACL_LEN_POS + 1] << 8)));
				uart_check_data_len(rx_info, len_16);
				/* Header read. Continue with next bytes */
				continue;

			case W4_FM_RADIO_HDR:
				/* FM Opcode is not used */
				len_8 = rx_info->rx_skb->data[CCD_FM_RADIO_LEN_POS];
				uart_check_data_len(rx_info, len_8);
				/* Header read. Continue with next bytes */
				continue;

			case W4_GNSS_HDR:
				/* GNSS Opcode is not used */
				len_16 = __le16_to_cpu((uint16_t)((rx_info->rx_skb->data[CCD_GNSS_LEN_POS]) |
								  (rx_info->rx_skb->data[CCD_GNSS_LEN_POS + 1] << 8)));
				uart_check_data_len(rx_info, len_16);
				/* Header read. Continue with next bytes */
				continue;

			default:
				STE_CONN_ERR("Bad state indicating memory overwrite (0x%X)", (uint8_t)(rx_info->rx_state));
				break;
			}
		}

		/* Check which H:4 packet this is and update RX states */
		if (*r_ptr == ccd_info->h4_channels.bt_evt_channel) {
			rx_info->rx_state = W4_EVENT_HDR;
			rx_info->rx_count = CCD_HCI_BT_EVT_HDR_SIZE;
		} else if (*r_ptr == ccd_info->h4_channels.bt_acl_channel) {
			rx_info->rx_state = W4_ACL_HDR;
			rx_info->rx_count = CCD_HCI_BT_ACL_HDR_SIZE;
		} else if (*r_ptr == ccd_info->h4_channels.fm_radio_channel) {
			rx_info->rx_state = W4_FM_RADIO_HDR;
			rx_info->rx_count = CCD_HCI_FM_RADIO_HDR_SIZE;
		} else if (*r_ptr == ccd_info->h4_channels.gnss_channel) {
			rx_info->rx_state = W4_GNSS_HDR;
			rx_info->rx_count = CCD_HCI_GNSS_HDR_SIZE;
		} else {
			STE_CONN_ERR("Unknown HCI packet type 0x%X", (uint8_t)*r_ptr);
			r_ptr++;
			count--;
			continue;
		}

		/* Allocate packet. We do not yet know the size and therefore allocate max size */
		rx_info->rx_skb = alloc_rx_skb(CCD_RX_SKB_MAX_SIZE, GFP_ATOMIC);
		if (!rx_info->rx_skb) {
			STE_CONN_ERR("Can't allocate memory for new packet");
			rx_info->rx_state = W4_PACKET_TYPE;
			rx_info->rx_count = 0;
			return 0;
		}

		/* Write the H:4 header first in the sk_buffer */
		w_ptr = skb_put(rx_info->rx_skb, 1);
		*w_ptr = *r_ptr;

		/* First byte (H4 header) read. Goto next byte */
		r_ptr++;
		count--;
	}

	return count;
}

/* -------------- Generic functions --------------------------- */

/**
 * ccd_work_hw_registered() - Work function called when the interface to HW has been established (e.g. during uart_tty_open).
 * @work: Reference to work data.
 *
 * Notifies CPD that the HW is registered now.
 */
static void ccd_work_hw_registered(struct work_struct *work)
{
	struct ccd_work_struct *current_work = NULL;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct ccd_work_struct, work);

	ste_conn_cpd_hw_registered();

	/* Now the channels should be updated from the controller */
	ste_conn_cpd_get_h4_channel(STE_CONN_DEVICES_BT_CMD,
					&(ccd_info->h4_channels.bt_cmd_channel));
	ste_conn_cpd_get_h4_channel(STE_CONN_DEVICES_BT_ACL,
					&(ccd_info->h4_channels.bt_acl_channel));
	ste_conn_cpd_get_h4_channel(STE_CONN_DEVICES_BT_EVT,
					&(ccd_info->h4_channels.bt_evt_channel));
	ste_conn_cpd_get_h4_channel(STE_CONN_DEVICES_GNSS,
					&(ccd_info->h4_channels.gnss_channel));
	ste_conn_cpd_get_h4_channel(STE_CONN_DEVICES_FM_RADIO,
					&(ccd_info->h4_channels.fm_radio_channel));
	ste_conn_cpd_get_h4_channel(STE_CONN_DEVICES_DEBUG,
					&(ccd_info->h4_channels.debug_channel));
	ste_conn_cpd_get_h4_channel(STE_CONN_DEVICES_STE_TOOLS,
					&(ccd_info->h4_channels.ste_tools_channel));
	ste_conn_cpd_get_h4_channel(STE_CONN_DEVICES_HCI_LOGGER,
					&(ccd_info->h4_channels.hci_logger_channel));
	ste_conn_cpd_get_h4_channel(STE_CONN_DEVICES_US_CTRL,
					&(ccd_info->h4_channels.us_ctrl_channel));
	ste_conn_cpd_get_h4_channel(STE_CONN_DEVICES_CORE,
					&(ccd_info->h4_channels.core_channel));

	kfree(current_work);
}

/**
 * ccd_work_hw_deregistered() - Handle HW deregistered.
 * @work: Reference to work data.
 *
 * Handle work.
 */
static void ccd_work_hw_deregistered(struct work_struct *work)
{
	struct ccd_work_struct *current_work = NULL;
	struct ccd_uart *c_uart = NULL;

	if (!work) {
		STE_CONN_ERR("Wrong pointer (work = 0x%x)", (uint32_t) work);
		return;
	}

	current_work = container_of(work, struct ccd_work_struct, work);
	c_uart = (struct ccd_uart *)current_work->data;

	if (c_uart) {
		/* Tell CPD that HW is disconnected */
		ste_conn_cpd_hw_deregistered();

		/* Purge any stored sk_buffers */
		skb_queue_purge(&ccd_info->tx_queue);
		if (c_uart->rx_info.rx_skb) {
			kfree_skb(c_uart->rx_info.rx_skb);
			c_uart->rx_info.rx_skb = NULL;
		}

		ccd_info->state = CCD_STATE_INITIALIZING;
		ste_conn_transport = CCD_TRANSPORT_UNKNOWN;
		/* Finally free the UART structure */
		kfree(c_uart);
	}
}

/**
 * ccd_create_work_item() - Create work item and add it to the work queue.
 * @wq: work queue struct where the work will be added.
 * @work_func:	Work function.
 * @data: 	Private data for the work.
 *
 * The ccd_create_work_item() function creates work item and
 * add it to the work queue.
 *
 * Returns:
 *   0 if there is no error.
 *   -EBUSY if not possible to queue work.
 *   -ENOMEM if allocation fails.
 */
static int ccd_create_work_item(struct workqueue_struct *wq, work_func_t work_func, void *data)
{
	struct ccd_work_struct *new_work;
	int wq_err;
	int err = 0;

	new_work = kmalloc(sizeof(*new_work), GFP_ATOMIC);

	if (new_work) {
		new_work->data = data;
		INIT_WORK(&new_work->work, work_func);

		wq_err = queue_work(wq, &new_work->work);

		if (!wq_err) {
			STE_CONN_ERR("Failed to queue work_struct because it's already in the queue!");
			kfree(new_work);
			err = -EBUSY;
		}
	} else {
		STE_CONN_ERR("Failed to alloc memory for ccd_work_struct!");
		err = -ENOMEM;
	}

	return err;
}

/**
 * update_timer() - Updates or starts the ccd timer used to detect when there are no current data transmissions.
 */
static void update_timer(void)
{
	if (!use_sleep_timeout) {
		return;
	}

	/* This function indicates data is transmitted.
	 * Therefore see to that the chip is awake
	 */
	if (CHIP_ASLEEP == ccd_info->sleep_state) {
		switch (ste_conn_transport) {
		case CCD_TRANSPORT_UART:
		{
			struct tty_struct *tty;

			if (ccd_info->c_uart && ccd_info->c_uart->tty) {
				tty = ccd_info->c_uart->tty;
			} else {
				return;
			}
			if (tty->ops && tty->ops->break_ctl) {
				tty->ops->break_ctl(tty, TTY_BREAK_OFF);
			}
			break;
		}
		case CCD_TRANSPORT_CHAR_DEV:
			break;
		default:
			STE_CONN_ERR("Unknown ste_conn_transport: 0x%X", ste_conn_transport);
			break;
		};

		ccd_info->sleep_state = CHIP_AWAKE;
	}
	/* If timer is running restart it. If not, start it.
	 * All this is handled by mod_timer()
	 */
	mod_timer(&(ccd_info->timer), jiffies + ccd_timeout_jiffies);
}

/**
 * ccd_timer_expired() - Called when timer expires.
 * @data: Value supplied when starting the timer.
 *
 * The ccd_timer_expired() function is called if there are no ongoing data
 * transmissions it tries to put the chip in sleep mode.
 *
 */
static void ccd_timer_expired(unsigned long data)
{
	if (!use_sleep_timeout) {
		return;
	}

	switch (ste_conn_transport) {
	case CCD_TRANSPORT_UART:
	{
		struct tty_struct *tty;

		if (ccd_info->c_uart && ccd_info->c_uart->tty) {
			tty = ccd_info->c_uart->tty;
		} else {
			return;
		}
		if (tty->ops && tty->ops->break_ctl) {
			tty->ops->break_ctl(tty, TTY_BREAK_ON);
		}
		break;
	}
	case CCD_TRANSPORT_CHAR_DEV:
		break;
	default:
		STE_CONN_ERR("Unknown ste_conn_transport: 0x%X", ste_conn_transport);
		break;
	};

	ccd_info->sleep_state = CHIP_ASLEEP;
}

/**
 * alloc_rx_skb() - Alloc an sk_buff structure for receiving data from controller.
 * @size:     Size in number of octets.
 * @priority: Allocation priority, e.g. GFP_KERNEL.
 *
 * Returns:
 *   Pointer to sk_buff structure.
 */
static struct sk_buff *alloc_rx_skb(unsigned int size, gfp_t priority)
{
	struct sk_buff *skb;

	/* Allocate the SKB and reserve space for the header */
	skb = alloc_skb(size + CCD_RX_SKB_RESERVE, priority);
	if (skb) {
		skb_reserve(skb, CCD_RX_SKB_RESERVE);
	}
	return skb;
}

/**
 * test_char_device_create() - Create a separate char device that will interact directly with userspace test application.
 *
 * Returns:
 *   0 if there is no error.
 *   -ENOMEM if allocation fails.
 *   -EBUSY if device has already been allocated.
 *   Error codes from misc_register.
 */
static int test_char_device_create(void)
{
	int err = 0;

	if (!ccd_info->char_test_dev) {
		ccd_info->char_test_dev =
			kzalloc(sizeof(*(ccd_info->char_test_dev)), GFP_KERNEL);
		if (!ccd_info->char_test_dev) {
			STE_CONN_ERR("Couldn't allocate char_test_dev");
			err = -ENOMEM;
			goto finished;
		}
	} else {
		STE_CONN_ERR("Trying to allocate char_test_dev twice");
		err = -EBUSY;
		goto finished;
	}

	/* Initialize the RX queue */
	skb_queue_head_init(&ccd_info->char_test_dev->rx_queue);

	/* Prepare miscdevice struct before registering the device */
	ccd_info->char_test_dev->test_miscdev.minor = MISC_DYNAMIC_MINOR;
	ccd_info->char_test_dev->test_miscdev.name = STE_CONN_CDEV_NAME;
	ccd_info->char_test_dev->test_miscdev.fops = &test_char_dev_fops;
	ccd_info->char_test_dev->test_miscdev.parent = ccd_info->dev;

	err = misc_register(&ccd_info->char_test_dev->test_miscdev);
	if (err) {
		STE_CONN_ERR("Error %d registering misc dev!", err);
		goto error_handling_w_free;
	}

	goto finished;

error_handling_w_free:
	kfree(ccd_info->char_test_dev);
	ccd_info->char_test_dev = NULL;
finished:
	return err;
}

/**
 * test_char_device_destroy() - Clean up after test_char_device_create().
 */
static void test_char_device_destroy(void)
{
	int err = 0;

	if (ccd_info && ccd_info->char_test_dev && (ste_conn_transport == CCD_TRANSPORT_CHAR_DEV)) {
		err = misc_deregister(&ccd_info->char_test_dev->test_miscdev);
		if (err) {
			STE_CONN_ERR("Error %d deregistering misc dev!", err);
		}
		/*Clean the message queue*/
		skb_queue_purge(&ccd_info->char_test_dev->rx_queue);

		kfree(ccd_info->char_test_dev);
		ccd_info->char_test_dev = NULL;
	}
}

/**
 * test_char_dev_read() - queue and copy buffer to user.
 * @filp:  Pointer to the file struct.
 * @buf:   Received buffer.
 * @count: Count of received data in bytes.
 * @f_pos: Position in buffer.
 *
 * The ste_conn_char_device_read() function queues and copy the received buffer to the user space char device.
 *
 * Returns:
 *   >= 0 is number of bytes read.
 *   -EFAULT if copy_to_user fails.
 */
static ssize_t test_char_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct sk_buff *skb;
	int bytes_to_copy;
	int err;
	struct sk_buff_head *rx_queue = &ccd_info->char_test_dev->rx_queue;

	STE_CONN_INFO("test_char_dev_read");

	if (skb_queue_empty(rx_queue)) {
		wait_event_interruptible(test_char_rx_wait_queue, !(skb_queue_empty(rx_queue)));
	}

	skb = skb_dequeue(rx_queue);

	if (!skb) {
		STE_CONN_INFO("skb queue is empty - return with zero bytes");
		bytes_to_copy = 0;
		goto finished;
	}

	bytes_to_copy = min(count, skb->len);
	err = copy_to_user(buf, skb->data, bytes_to_copy);
	if (err) {
		skb_queue_head(rx_queue, skb);
		return -EFAULT;
	}
	skb_pull(skb, bytes_to_copy);

	if (skb->len > 0) {
		skb_queue_head(rx_queue, skb);
	} else {
		kfree_skb(skb);
	}

finished:
	return bytes_to_copy;
}

/**
 * test_char_dev_write() - copy buffer from user and write to cpd.
 * @filp:  Pointer to the file struct.
 * @buf:   Read buffer.
 * @count: Size of the buffer write.
 * @f_pos: Position in buffer.
 *
 * The ste_conn_char_device_write() function copy buffer from user and write to cpd.
 *
 * Returns:
 *   >= 0 is number of bytes written.
 *   -EFAULT if copy_from_user fails.
 */
static ssize_t test_char_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct sk_buff *skb;

	STE_CONN_INFO("test_char_dev_write count %d", count);

	skb = alloc_rx_skb(count, GFP_ATOMIC);
	if (skb) {
		if (copy_from_user(skb_put(skb, count), buf, count)) {
			kfree_skb(skb);
			return -EFAULT;
		}
		ste_conn_cpd_data_received(skb);
	}
	return count;
}

/**
 * test_char_dev_poll() - handle POLL call to the interface.
 * @filp:  Pointer to the file struct.
 * @wait:  Poll table supplied to caller.
 *
 * The test_char_dev_poll() function handles IOCTL call to the interface.
 *
 * Returns:
 *   Mask of current set POLL values (0 or (POLLIN | POLLRDNORM))
 */
static unsigned int test_char_dev_poll(struct file *filp,
					poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(filp, &test_char_rx_wait_queue, wait);

	if (!(skb_queue_empty(&ccd_info->char_test_dev->rx_queue))) {
		mask |= POLLIN | POLLRDNORM;
	}

	return mask;
}


/**
 * test_char_dev_tx_received() - handle data received from connectivity protocol driver.
 * @rx_queue: pointer to rx queue where the skb shall be put.
 * @skb: buffer with data coming form device.
 *
 * The test_char_dev_tx_received() function handles data received from connectivity protocol driver.
 */
static void test_char_dev_tx_received(struct sk_buff_head *rx_queue, struct sk_buff *skb)
{

	skb_queue_tail(rx_queue, skb);

	wake_up_interruptible(&test_char_rx_wait_queue);
}

/*
  * struct ste_conn_hci_uart_ldisc - UART TTY line discipline.
  *
  * The ste_conn_hci_uart_ldisc structure is used when
  * registering to the UART framework.
  */
static struct tty_ldisc_ops ste_conn_hci_uart_ldisc = {
	.magic        = TTY_LDISC_MAGIC,
	.name         = "n_hci",
	.open         = uart_tty_open,
	.close        = uart_tty_close,
	.read         = uart_tty_read,
	.write        = uart_tty_write,
	.ioctl        = uart_tty_ioctl,
	.poll         = uart_tty_poll,
	.receive_buf  = uart_tty_receive,
	.write_wakeup = uart_tty_wakeup,
	.owner        = THIS_MODULE
};

/**
 * ste_conn_init() - Initialize module.
 *
 * The ste_conn_init() function initialize CCD and CPD,
 * then register to the transport framework.
 *
 * Returns:
 *   0 if success.
 *   -ENOMEM for failed alloc or structure creation.
 *   Error codes generated by ste_conn_devices_init, alloc_chrdev_region,
 *   class_create, device_create, ste_conn_cpd_init, tty_register_ldisc,
 *   ccd_create_work_item.
 */
static int __init ste_conn_init(void)
{
	int err = 0;
	dev_t temp_devt;
	struct timeval time_50ms = {
		.tv_sec = 0,
		.tv_usec = 50 * USEC_PER_MSEC
	};
	struct ste_conn_ccd_driver_data *dev_data;

	STE_CONN_INFO("ste_conn_init");

	err = ste_conn_devices_init();
	if (err) {
		STE_CONN_ERR("Couldn't initialize ste_conn_devices");
		goto finished;
	}

	ccd_info = kzalloc(sizeof(*ccd_info), GFP_KERNEL);
	if (!ccd_info) {
		STE_CONN_ERR("Couldn't allocate ccd_info");
		err = -ENOMEM;
		goto finished;
	}
	ccd_info->state = CCD_STATE_INITIALIZING;
	ccd_info->sleep_state = CHIP_AWAKE;
	skb_queue_head_init(&ccd_info->tx_queue);

	/* Get the H4 channel ID for all channels */
	ccd_info->h4_channels.bt_cmd_channel = HCI_BT_CMD_H4_CHANNEL;
	ccd_info->h4_channels.bt_acl_channel = HCI_BT_ACL_H4_CHANNEL;
	ccd_info->h4_channels.bt_evt_channel = HCI_BT_EVT_H4_CHANNEL;
	ccd_info->h4_channels.gnss_channel = HCI_FM_RADIO_H4_CHANNEL;
	ccd_info->h4_channels.fm_radio_channel = HCI_GNSS_H4_CHANNEL;

	ccd_info->wq = create_singlethread_workqueue(STE_CONN_CCD_WQ_NAME);
	if (!ccd_info->wq) {
		STE_CONN_ERR("Could not create workqueue");
		err = -ENOMEM;
		goto error_handling;
	}

	/* Convert the time to jiffies and setup the timer structure */
	ccd_timeout_jiffies = timeval_to_jiffies(&time_50ms);
	ccd_info->timer.function = ccd_timer_expired;
	ccd_info->timer.expires  = jiffies + ccd_timeout_jiffies;
	ccd_info->timer.data     = 0;

	/* Create major device number */
	err = alloc_chrdev_region (&temp_devt, 0, 33, STE_CONN_DEVICE_NAME);
	if (err) {
		STE_CONN_ERR("Can't get major number (%d)", err);
		goto error_handling_destroy_wq;
	}

	/* Create the device class */
	ccd_info->ccd_class = class_create(THIS_MODULE, STE_CONN_CLASS_NAME);
	if (IS_ERR(ccd_info->ccd_class)) {
		STE_CONN_ERR("Error creating class");
		err = (int)ccd_info->ccd_class;
		goto error_handling_class_create;
	}

	/* Create the main device */
	dev_data = kzalloc(sizeof(*dev_data), GFP_KERNEL);
	if (!dev_data) {
		STE_CONN_ERR("Couldn't allocate dev_data");
		err = -ENOMEM;
		goto error_handling_dev_data_alloc;
	}

	ccd_info->dev = device_create(ccd_info->ccd_class, NULL, temp_devt,
			dev_data, STE_CONN_DEVICE_NAME "%d",
			dev_data->next_free_minor);
	if (IS_ERR(ccd_info->dev)) {
		STE_CONN_ERR("Error creating main device");
		err = (int)ccd_info->dev;
		goto error_handling_dev_create;
	}
	dev_data->next_free_minor++;

	ste_conn_transport = CCD_TRANSPORT_UNKNOWN;

	/* Check if verification mode is enabled to stubb transport interface. */
	if (char_dev_usage & STE_CONN_CHAR_TEST_DEV) {
		/* Create and add test char device. */
		if (test_char_device_create() == 0) {
			ste_conn_transport = CCD_TRANSPORT_CHAR_DEV;
			STE_CONN_INFO("Test char device detected, ccd will be stubbed.");
		}
		char_dev_usage &= ~STE_CONN_CHAR_TEST_DEV;
	}

	/* Initialize the CPD */
	err = ste_conn_cpd_init(char_dev_usage, ccd_info->dev);
	if (err) {
		STE_CONN_ERR("ste_conn_cpd_init failed %d", err);
		goto error_handling;
	}

	if (CCD_TRANSPORT_UNKNOWN == ste_conn_transport) {
		/* Register the tty discipline.
		 * We will see what will be used.
		 */
		err = tty_register_ldisc(N_HCI, &ste_conn_hci_uart_ldisc);
		if (err) {
			STE_CONN_ERR("HCI line discipline registration failed. (0x%X)", err);
			goto error_handling_register;
		}
	}

	/* If we are using a character device as transport we already have the HW */
	if (CCD_TRANSPORT_CHAR_DEV == ste_conn_transport) {

		err = ccd_create_work_item(ccd_info->wq, ccd_work_hw_registered, NULL);
	}

	goto finished;

error_handling_register:
	device_destroy(ccd_info->ccd_class, ccd_info->dev->devt);
	ccd_info->dev = NULL;
error_handling_dev_create:
	kfree(dev_data);
error_handling_dev_data_alloc:
	class_destroy(ccd_info->ccd_class);
error_handling_class_create:
	unregister_chrdev_region(temp_devt, 32);
error_handling_destroy_wq:
	destroy_workqueue(ccd_info->wq);
error_handling:
	kfree(ccd_info);
	ccd_info = NULL;
finished:
	return err;
}

/**
 * ste_conn_exit() - Remove module.
 */
static void __exit ste_conn_exit(void)
{
	int err = 0;
	dev_t temp_devt;

	STE_CONN_INFO("ste_conn_exit");

	/* Release everything allocated in cpd */
	ste_conn_cpd_exit();

	if (CCD_TRANSPORT_CHAR_DEV != ste_conn_transport) {
		/* Release tty registration of line discipline */
		err = tty_unregister_ldisc(N_HCI);
		if (err) {
			STE_CONN_ERR("Can't unregister HCI line discipline (0x%X)", err);
		}
	}

	test_char_device_destroy();
	if (ccd_info) {
		temp_devt = ccd_info->dev->devt;
		device_destroy(ccd_info->ccd_class, temp_devt);
		kfree(dev_get_drvdata(ccd_info->dev));
		class_destroy(ccd_info->ccd_class);
		unregister_chrdev_region(temp_devt, 32);

		destroy_workqueue(ccd_info->wq);

		kfree(ccd_info);
		ccd_info = NULL;
	}

	ste_conn_devices_exit();
}

module_init(ste_conn_init);
module_exit(ste_conn_exit);

MODULE_AUTHOR("Par-Gunnar Hjalmdahl ST-Ericsson");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Linux Bluetooth HCI H:4 Driver for ST-Ericsson Connectivity Controller ver " VERSION);
MODULE_VERSION(VERSION);

module_param(use_sleep_timeout, bool, S_IRUGO);
MODULE_PARM_DESC(use_sleep_timeout, "Use sleep timer for data transmissions: true=1/false=0");

module_param(char_dev_usage, int, S_IRUGO);
MODULE_PARM_DESC(char_dev_usage, "Character devices to enable (bitmask):0x00 = No char devs, \
									0x01 = BT, \
									0x02 = FM radio, \
									0x04 = GNSS, \
									0x08 = Debug, \
									0x10 = STE tools, \
									0x20 = CCD debug, \
									0x40 = HCI Logger, \
									0x80 = US Ctrl, \
									0x100 = STE_CONN test, \
									0x200 = BT Audio, \
									0x400 = FM Audio, \
									0x800 = Core");

module_param(uart_default_baud, int, S_IRUGO);
MODULE_PARM_DESC(uart_default_baud, "Default UART baud rate, e.g. 115200. If not set 115200 will be used.");

module_param(uart_high_baud, int, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(uart_high_baud, "High speed UART baud rate, e.g. 4000000.  If not set 921600 will be used.");

module_param(ste_debug_level, int, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(ste_debug_level, "Debug level. Default 1. Possible values:\n \
				0  = No debug\n \
				1  = Error prints\n \
				10 = General info, e.g. function entries\n \
				20 = Debug info, e.g. steps in a functionality\n \
				25 = Data info, i.e. prints when data is transferred\n \
				30 = Data content, i.e. contents of the transferred data");

module_param_array(ste_conn_bd_address, byte, &bd_addr_count, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(ste_conn_bd_address, "Bluetooth Device address. Default 0x00 0x80 0xDE 0xAD 0xBE 0xEF. \
Enter as comma separated value.");

module_param(ste_conn_default_hci_revision, int, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(ste_conn_default_hci_revision, "Default HCI revision according to Bluetooth Assigned Numbers.");

module_param(ste_conn_default_manufacturer, int, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(ste_conn_default_manufacturer, "Default Manfacturer according to Bluetooth Assigned Numbers.");

module_param(ste_conn_default_sub_version, int, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(ste_conn_default_sub_version, "Default HCI sub-version according to Bluetooth Assigned Numbers.");

