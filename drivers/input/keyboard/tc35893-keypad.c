/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License, version 2
 * Author: Jayeeta Banerjee <jayeeta.banerjee@stericsson.com> for ST-Ericsson
 */


/* Keypad driver Version */
#define TC_KEYPAD_VER_MAJOR		1
#define TC_KEYPAD_VER_MINOR		0
#define TC_KEYPAD_VER_RELEASE		0

#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/bitops.h>
#include <linux/i2c.h>
#include <mach/tc35893-keypad.h>

/* Maximum supported keypad matrix row/columns size */
#define TC_MAX_KPROW               8
#define TC_MAX_KPCOL               12

/* keypad related Constants */
#define TC_KEYPAD_RELEASE_PERIOD   11	/*110 Msec, repeate key scan time */
#define TC_KEYPAD_SCAN_PERIOD      4	/*40Msec for new keypress */
#define TC_MAX_DEBOUNCE_SETTLE     0xFF
#define DEDICATED_KEY_VAL	   0xFF
#define KPINTR_LKBIT		   0	/*bit used for interrupt locking */

/* Keyboard Configuration Registers Index */
#define TC_KBDSETTLE_REG	0x01
#define TC_KBDBOUNCE		0x02
#define	TC_KBDSIZE		0x03
#define TC_KBCFG_LSB		0x04
#define TC_KBCFG_MSB		0x05
#define TC_KBDRIS		0x06
#define TC_KBDMIS		0x07
#define TC_KBDIC		0x08
#define TC_KBDMSK		0x09
#define TC_KBDCODE0		0x0B
#define TC_KBDCODE1		0x0C
#define TC_KBDCODE2		0x0D
#define TC_KBDCODE3		0x0E
#define TC_EVTCODE_FIFO		0x10

/* System registers Index */
#define TC_MANUFACTURE_CODE	0x80
#define TC_VERSION_ID		0x81
#define TC_I2CSA		0x82
#define TC_IOCFG		0xA7

/* clock control registers */
#define TC_CLKMODE		0x88
#define TC_CLKCFG		0x89
#define TC_CLKEN		0x8A

/* Reset Control registers */
#define TC_RSTCTRL		0x82
#define TC_RSTINTCLR		0x84

/* special function register and drive config registers */
#define TC_KBDMFS		0x8F
#define TC_IRQST		0x91
#define TC_DRIVE0_LSB		0xA0
#define TC_DRIVE0_MSB		0xA1
#define TC_DRIVE1_LSB		0xA2
#define TC_DRIVE1_MSB		0xA3
#define TC_DRIVE2_LSB		0xA4
#define TC_DRIVE2_MSB		0xA5
#define TC_DRIVE3		0xA6

/* Pull up/down  configuration registers */
#define TC_IOCFG		0xA7
#define TC_IOPULLCFG0_LSB	0xAA
#define TC_IOPULLCFG0_MSB	0xAB
#define TC_IOPULLCFG1_LSB	0xAC
#define TC_IOPULLCFG1_MSB	0xAD
#define TC_IOPULLCFG2_LSB	0xAE

/* Pull up/down masks */
#define TC_NO_PULL_MASK		0x0
#define TC_PULL_DOWN_MASK	0x1
#define TC_PULL_UP_MASK		0x2
#define TC_PULLUP_ALL_MASK	0xAA
#define TC_IO_PULL_VAL(index, mask)	((mask)<<((index)%4)*2))

/* Bit masks for IOCFG register */
#define IOCFG_BALLCFG		0x01
#define IOCFG_IG		0x08

#define KP_EVCODE_COL_MASK	0x0F
#define KP_EVCODE_ROW_MASK	0x70
#define KP_RELEASE_EVT_MASK	0x80

#define KP_ROW_SHIFT		4

#define KP_NO_VALID_KEY_MASK	0x7F


#define TC_MAN_CODE_VAL		0x03
#define TC_SW_VERSION		0x80

/* bit masks for RESTCTRL register */
#define TC_KBDRST		0x2
#define TC_IRQRST		0x10
#define TC_RESET_ALL		0x1B
/* KBDMFS register bit mask */
#define TC_KBDMFS_EN		0x1

/* CLKEN register bitmask */
#define KPD_CLK_EN		0x1

/* RSTINTCLR register bit mask */
#define IRQ_CLEAR		0x1

/* bit masks for keyboard interrupts*/
#define TC_EVT_LOSS_INT		0x8
#define TC_EVT_INT		0x4
#define TC_KBD_LOSS_INT		0x2
#define TC_KBD_INT		0x1

/* bit masks for keyboard interrupt clear*/
#define TC_EVT_INT_CLR		0x2
#define TC_KBD_INT_CLR		0x1

/* Macro for key definition */
#define TC_KEY(col, row)	((col) + ((row) << KP_ROW_SHIFT))

#define EVT_BUF_SIZE		8

struct tc35893_info {
	unsigned char chip_id;
	unsigned char version_id;
};

/**
* struct t_tc35893_key_status - Data structure for key status during last scan.
* @nb_events: count of events received due to key press/release
* @event_buf: To save all key changed key event code, used to save
* data from event FIFO
* @
*/
struct t_tc35893_key_status {
	unsigned char nb_events;
	unsigned char event_buf[EVT_BUF_SIZE];
};

/**
 * struct tc_keypad - keypad data structure used internally by keypad driver
 * @mode:		0 for interrupt mode, 1 for polling mode
 * @lockbits: 		used for synchronisation in ISR
 * @inp_dev:		pointer to input device object
 * @kscan_work:		work queue
 * @board: 		keypad platform device
 * @client:		i2c client data structure
 * @lock:		internal sync primitive
 * @keyp_cnt:		keeps count of total number of keys pressed at
 *			any point of time.
 */
struct tc_keypad {
	int mode;
	unsigned long lockbits;
	struct input_dev *inp_dev;
	struct delayed_work kscan_work;
	struct tc35893_platform_data *board;
	struct i2c_client *client;
	struct mutex lock;
	int keyp_cnt;
};


/**
 * tc35893_write_byte() - Write a single byte
 * @client:	i2c client pointer
 * @reg:	register offset
 * @data:	data byte to be written
 *
 * This funtion uses smbus byte write API to write a single byte to tc35893
 **/
static int tc35893_write_byte(struct i2c_client *client,
				unsigned char reg, unsigned char data)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, reg, data);
	if (ret < 0)
		dev_err(&client->dev, "Error in writing reg %x: error = %d\n",
			reg, ret);
	return ret;
}

/**
 * tc35893_read_byte() - Read a single byte
 * @client:	i2c client data pointer
 * @reg:	register offset
 * @val:	data read
 *
 * This funtion uses smbus byte read API to read a byte from the given offset.
 **/
static int tc35893_read_byte(struct i2c_client *client, unsigned char reg,
			unsigned char *val)
{
	int ret;
	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		dev_err(&client->dev, "Error in reading reg %x: error = %d\n",
			reg, ret);
	*val = ret;
	return ret;
}

/**
 * tc35893_read_info(struct tc_keypad *kp) - read the chip information
 * @kp:	keypad data structure pointer
 *
 * This function read tc35893 chip and version ID
 * and returns error if chip id or version id is not correct.
 * This function can be called to check if UIB is connected or not.
 */
static int tc35893_read_info(struct tc_keypad *kp)
{
	unsigned char manufacture_code, version_id;
	int ret;

	/* read the chip id and version id */
	if ((!kp) || (!kp->client))
		return -EIO;
	/* TC35893 probe failed */
	ret = tc35893_read_byte(kp->client, TC_MANUFACTURE_CODE,
			&manufacture_code);
	if (ret < 0)
		return ret;

	ret = tc35893_read_byte(kp->client, TC_VERSION_ID,
				&version_id);
	if (ret < 0)
		return ret;

	if ((manufacture_code != TC_MAN_CODE_VAL) ||
		(version_id != TC_SW_VERSION)) {
		dev_err(&kp->client->dev,
			"Incorrect manufacture code:%x version id:%x\n"
			, manufacture_code, version_id);
		return -EIO;
	} else {
		dev_info(&kp->client->dev,
			"manufacture code: %x, version id:%x\n",
			manufacture_code, version_id);
	}
	return 0;
}


/**
 * tc35893_kp_key_irqdis- disables keypad interrupt
 *
 * @kp:	pointer to keypad data structure
 *
 * disables keypad interrupt
 */
static int tc35893_kp_key_irqdis(struct tc_keypad *kp)
{
	/* Disable KEYPAD interrupt of tc35893 */
	int err;
	unsigned char reg_val;

	mutex_lock(&kp->lock);
	err = tc35893_read_byte(kp->client, TC_KBDMSK, &reg_val);
	if (err < 0)
		goto error_out;
	reg_val &= (TC_EVT_LOSS_INT | TC_EVT_INT);
	err = tc35893_write_byte(kp->client, TC_KBDMSK, reg_val);
error_out:
	mutex_unlock(&kp->lock);
	return err;
}

/**
 * tc35893_kp_key_irqen- enables keypad interrupt
 *
 * @kp:	pointer to keypad data structure
 *
 * enables keypad interrupt
 */
static int tc35893_kp_key_irqen(struct tc_keypad *kp)
{
	/* Enable KEYPAD interrupt of tc35893*/
	int err;
	unsigned char reg_val;

	mutex_lock(&kp->lock);
	err = tc35893_read_byte(kp->client, TC_KBDMSK, &reg_val);
	if (err < 0)
		goto error_out;
	reg_val |= (TC_EVT_LOSS_INT | TC_EVT_INT);
	err = tc35893_write_byte(kp->client, TC_KBDMSK, reg_val);
error_out:
	mutex_unlock(&kp->lock);
	return err;
}


/**
 * tc35893_kp_key_irqclear- clears all keypad interrupts
 *
 * @kp:	pointer to keypad data structure
 *
 * clears keypad interrupt
 */
static int tc35893_kp_key_irqclear(struct tc_keypad *kp)
{
	/* Clear KEYPAD interrupt of tc35893 */
	int err;

	mutex_lock(&kp->lock);
	err = tc35893_write_byte(kp->client, TC_KBDIC,
		(TC_EVT_INT_CLR | TC_KBD_INT_CLR));
	mutex_unlock(&kp->lock);
	return err;
}

/**
 * tc35893_kp_init_key_hardware -  keypad hardware initialization
 * @kp: keypad configuration for this platform
 *
 * Initializes the keypad hardware specific parameters.
 * This function will be called by tc35893_keypad_init function during init
 * Initialize keypad interrupt handler for interrupt mode operation if enabled
 * Initialize Keyscan matrix
 *
 */
static int tc35893_kp_init_key_hardware(struct tc_keypad *kp)
{
	int err;
	unsigned char reg_val;

	if (!kp || !kp->client || !kp->board)
		return -EINVAL;
	dev_dbg(&kp->client->dev, "tc35893_kp_init_key_hardware\n");

	/*
	* Check if tc35893 is initialised properly, otherwise exit from here
	*/
	err = tc35893_read_info(kp);
	if (err < 0) {
		dev_err(&kp->client->dev,
		"Error in keypad init, keypad controller not initialised.\n");
		return err;
	}

	/*
	* Checking configurations against max possible
	* row, column and debounce value;
	*/
	if ((kp->board->kcol > TC_MAX_KPCOL) || (kp->board->krow > TC_MAX_KPROW)
	|| (kp->board->debounce_period > TC_MAX_DEBOUNCE_SETTLE) ||
	(kp->board->settle_time > TC_MAX_DEBOUNCE_SETTLE)) {
		return -EINVAL;
	}

	mutex_lock(&kp->lock);

	/* configure KBDSIZE 4 LSbits for cols and 4 MSbits for rows */
	reg_val = ((kp->board->krow & 0xF) << KP_ROW_SHIFT) |
			(kp->board->kcol & 0xF);
	err = tc35893_write_byte(kp->client, TC_KBDSIZE, reg_val);
	if (err < 0)
		goto error_out;


	/* configure dedicated key config, no dedicated key selected */
	err = tc35893_write_byte(kp->client, TC_KBCFG_LSB, DEDICATED_KEY_VAL);
	if (err < 0)
		goto error_out;
	err = tc35893_write_byte(kp->client, TC_KBCFG_MSB, DEDICATED_KEY_VAL);
	if (err < 0)
		goto error_out;

	/* Configure settle time */
	err = tc35893_write_byte(kp->client, TC_KBDSETTLE_REG,
				kp->board->settle_time);
	if (err < 0)
		goto error_out;

	/* Configure debounce time */
	err = tc35893_write_byte(kp->client, TC_KBDBOUNCE,
				kp->board->debounce_period);
	if (err < 0)
		goto error_out;


	/* Start of initialise keypad GPIOs */
	err = tc35893_read_byte(kp->client, TC_IOCFG, &reg_val);
	if (err < 0)
		goto error_out;
	reg_val |= IOCFG_IG;
	err = tc35893_write_byte(kp->client, TC_IOCFG, reg_val);
	if (err < 0)
		goto error_out;


	/* Configure pull-up resistors for all row GPIOs */
	err = tc35893_write_byte(kp->client, TC_IOPULLCFG0_LSB,
				TC_PULLUP_ALL_MASK);
	if (err < 0)
		goto error_out;
	err = tc35893_write_byte(kp->client, TC_IOPULLCFG0_MSB,
				TC_PULLUP_ALL_MASK);
	if (err < 0)
		goto error_out;

	/* Configure pull-up resistors for all column GPIOs */
	err = tc35893_write_byte(kp->client, TC_IOPULLCFG1_LSB,
				TC_PULLUP_ALL_MASK);
	if (err < 0)
		goto error_out;

	err = tc35893_write_byte(kp->client, TC_IOPULLCFG1_MSB,
				TC_PULLUP_ALL_MASK);
	if (err < 0)
		goto error_out;

	err = tc35893_write_byte(kp->client, TC_IOPULLCFG2_LSB,
				TC_PULLUP_ALL_MASK);

error_out:
	mutex_unlock(&kp->lock);
	return err;
}

/**
 * tc35893_keypressed - This function read keypad data registers
 * @keys: output parameter, returns keys pressed.
 * @kp:	keypad data structure pointer
 *
 * This function can be used in both polling or interrupt mode.
 */
static int tc35893_keypressed(struct t_tc35893_key_status *keys,
			struct tc_keypad *kp)
{
	int retval;

	if (!keys || !kp || !kp->client)
		return -EINVAL;

	keys->nb_events = 0;

	do {
		retval = tc35893_read_byte(kp->client, TC_EVTCODE_FIFO,
					&keys->event_buf[keys->nb_events]);
		if (retval < 0)
			return retval;
		dev_dbg(&kp->client->dev, "Key[%d] = %x  \n", keys->nb_events,
				keys->event_buf[keys->nb_events]);
		keys->nb_events++;

	} while (((keys->event_buf[keys->nb_events-1] & KP_NO_VALID_KEY_MASK) !=
		KP_NO_VALID_KEY_MASK) && (keys->nb_events < EVT_BUF_SIZE));


	if (keys->nb_events < EVT_BUF_SIZE)
		keys->nb_events--;
		/* discard the last read with no valid key code */

	return retval;
}


/**
 * tc35893_kp_results_autoscan :  This function gets scanned key code
 * @kp: keypad data pointer
 *
 * This function can be used in both polling or interrupt mode.
 */
static int tc35893_kp_results_autoscan(struct tc_keypad *kp)
{
	int err;
	struct t_tc35893_key_status keys;
	u8 kcode, i, row_index, col_index;

	/* read key data from tc35893 */
	err = tc35893_keypressed(&keys, kp);
	if (err < 0) {
		dev_err(&kp->client->dev, "Error in keycode scanning\n");
		return err;
	}

	if (!keys.nb_events)
		return kp->keyp_cnt;

	/* Report any new event to input subsystem */
	for (i = 0; i < keys.nb_events; i++) {
		col_index = (keys.event_buf[i] & KP_EVCODE_COL_MASK);
		row_index = (keys.event_buf[i] & KP_EVCODE_ROW_MASK)
				>> KP_ROW_SHIFT;
		kcode = kp->board->kcode_tbl
			[(row_index * TC_KPD_COLUMNS) + col_index];

		if (kcode == KEY_RESERVED) {
			dev_err(&kp->client->dev,
				"Error in key detection. Key not present\n");
			continue;
		}

		if (keys.event_buf[i] & KP_RELEASE_EVT_MASK) {
			input_report_key(kp->inp_dev, kcode, 0);
			dev_dbg(&kp->client->dev,
				"key (%d, %d)-->UP, kcode = %x\n",
				row_index, col_index, kcode);
			kp->keyp_cnt--;
		} else {
			input_report_key(kp->inp_dev, kcode, 1);
			dev_dbg(&kp->client->dev,
			"key (%d, %d)-->DOWN, kcode = %x\n",
			row_index, col_index, kcode);
			kp->keyp_cnt++;
		}
	}

	return kp->keyp_cnt;
}


/**
 * tc35893_kp_wq_kscan - work queue for keypad scanning
 * @work:	pointer to keypad data
 *
 * Executes at each scan tick, execute the key press/release function,
 * Generates key press/release event message for input subsystem for valid key
 * events, enables keypad interrupts (for int mode)
 */
static void tc35893_kp_wq_kscan(struct work_struct *work)
{
	int key_cnt;
	struct tc_keypad *kp;

	kp = container_of((struct delayed_work *)work,
			  struct tc_keypad, kscan_work);
	if (!kp->board->op_mode)
		tc35893_kp_key_irqdis(kp);
	/* Get the key scan codes */
	key_cnt = tc35893_kp_results_autoscan(kp);

	if (kp->board->op_mode) {
		if (0 == key_cnt) {
			/*if no key is pressed and polling mode */
			schedule_delayed_work(&kp->kscan_work,
					      TC_KEYPAD_SCAN_PERIOD);
		} else {
			/*if key is pressed and hold condition */
			dev_dbg(&kp->client->dev,
				"Work queue: pressed and hold\n");
			schedule_delayed_work(&kp->kscan_work,
					      TC_KEYPAD_RELEASE_PERIOD);
		}
	} else {
		/* if interrupt mode then just enable iterrupt and return */
		clear_bit(KPINTR_LKBIT, &kp->lockbits);
		tc35893_kp_key_irqclear(kp);
		tc35893_kp_key_irqen(kp);
	}
}

/**
 * tc35893_kp_init_keypad - keypad parameter initialization
 * @kp:		pointer to keypad data
 *
 * Initializes Keybits to enable keyevents
 * Initializes Initial keypress status to default
 * Calls the keypad platform specific init function.
 */
static int tc35893_kp_init_keypad(struct tc_keypad *kp)
{
	int row, column, err;
	u8 *p_kcode = kp->board->kcode_tbl;

	err = tc35893_kp_init_key_hardware(kp);
	if (err < 0)
		return err;

	for (row = 0; row < TC_KPD_ROWS; row++) {
		for (column = 0; column < TC_KPD_ROWS; column++) {
			/*set keybits for the keycodes in use */
			set_bit(*p_kcode, kp->inp_dev->keybit);
			p_kcode++;
		}
	}
	return err;
}

#ifdef CONFIG_PM
/**
 * tc35893_kp_suspend - suspend keypad
 * @client:	i2c client pointer
 * @state:	power down level
 */
static int tc35893_kp_suspend(struct i2c_client *client, pm_message_t state)
{
	struct tc35893_platform_data *board;
	struct tc_keypad *kp;

	if (!client)
		return -EINVAL;

	board = client->dev.platform_data;
	kp = i2c_get_clientdata(client);
	if (!board || !kp)
		return -EINVAL;

	if (board->op_mode) {
		dev_dbg(&client->dev, "Enabling interrupt \n");
		tc35893_kp_key_irqen(kp);
	}

	if (!device_may_wakeup(&client->dev)) {
		dev_dbg(&client->dev, "Disabling interrupt\n");
		tc35893_kp_key_irqdis(kp);
	}
	return 0;
}

/**
 * tc35893_kp_resume - resumes keypad
 * @client:       i2c client data
 */

static int tc35893_kp_resume(struct i2c_client *client)
{
	struct tc35893_platform_data *board;
	struct tc_keypad *kp;

	if (!client)
		return -EINVAL;

	board = client->dev.platform_data;
	kp = i2c_get_clientdata(client);
	if (!board || !kp)
		return -EINVAL;

	if (board->op_mode) {
		dev_dbg(&client->dev, "Disabling interrupt\n");
		tc35893_kp_key_irqdis(kp);
	}

	if (!device_may_wakeup(&client->dev)) {
		dev_dbg(&client->dev, "Enabling interrupt\n");
		tc35893_kp_key_irqen(kp);
	}
	return 0;
}

#else
#define tc35893_kp_suspend NULL
#define tc35893_kp_resume NULL
#endif				/* CONFIG_PM */


/* tc35893 keypad controller functions */

/**
 * tc35893_kp_irh - keypad interrupt handler
 * @irq: interrupt no.
 * @dev : keypad data structure pointer
 * checks for valid interrupt, disables interrupt to avoid any nested interrupt
 * starts work queue for further key processing with debouncing logic
 */
static irqreturn_t tc35893_kp_irh(int irq, void *dev)
{
	struct tc_keypad *kp = (struct tc_keypad *)dev;

	/*
	 * prefer BUG_ON instead of just returning IRQ_NONE
	 * otherwise spurious interrupt will keep on coming.
	 */
	BUG_ON(!kp || !kp->board || !kp->client);

	dev_dbg(&kp->client->dev, "Kp interrupt\n");

	if (irq != kp->board->irq)
		return IRQ_NONE;

	if (!(test_bit(KPINTR_LKBIT, &kp->lockbits))) {
		set_bit(KPINTR_LKBIT, &kp->lockbits);

		/* tc35893_kp_key_irqdis(kp); */
		schedule_delayed_work(&kp->kscan_work, 0);
	}
	return IRQ_HANDLED;
}



/**
 * tc35893_kp_probe - keypad module probe function
 * @client:	i2c client data pointer
 * @id:		i2c device id pointer
 *
 * Allocates data memory, registers the module with input subsystem,
 * initializes keypad default condition, initializes keypad interrupt handler
 * for interrupt mode operation, initializes keypad work queues functions for
 * polling mode operation
 */
static int __init tc35893_kp_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct tc_keypad *kp;
	int err;
	unsigned char manufacture_code, version_id, reg_val;
	struct tc35893_platform_data *keypad_board;

	if (!client || !id)
		return -EINVAL;

	keypad_board = client->dev.platform_data;
	if (!keypad_board) {
		dev_err(&client->dev, "keypad platform data not defined\n");
		err = -EINVAL;
		goto err_kzalloc;
	}

	/* Allocate keypad data structure */
	kp = kzalloc(sizeof(struct tc_keypad), GFP_KERNEL);
	if (!kp) {
		err = -ENOMEM;
		goto err_kzalloc;
	}
	kp->client = client;
	if (!keypad_board->irq) {
		dev_err(&client->dev, "keypad irq not defined\n");
		err = -EINVAL;
		goto err_board;
	}
	kp->board = keypad_board;

	/* Allocate and initialise input device */
	kp->inp_dev = input_allocate_device();
	if (!kp->inp_dev) {
		dev_err(&client->dev,
			"Could not allocate memory for the device\n");
		err = -ENOMEM;
		goto err_board;
	}

	kp->inp_dev->evbit[0] = BIT(EV_KEY) | BIT(EV_REP);
	kp->inp_dev->name = client->name;
	kp->inp_dev->phys = "stkpd/input0";

	kp->inp_dev->id.product = TC_KEYPAD_VER_MAJOR;
	kp->inp_dev->id.version = TC_KEYPAD_VER_MINOR * 0x0ff +
					TC_KEYPAD_VER_RELEASE;

	mutex_init(&kp->lock);

	clear_bit(KPINTR_LKBIT, &kp->lockbits);
	i2c_set_clientdata(client, kp);


	/* Reset and initialise TC keypad controller */
	/* issue soft reset for all the modules */
	err = tc35893_write_byte(client, TC_RSTCTRL, TC_RESET_ALL);
	if (err < 0)
		return err;

	err = tc35893_read_byte(client, TC_RSTCTRL, &reg_val);
	if (err < 0)
		return err;

	reg_val &= ~(TC_KBDRST | TC_IRQRST);
	/* take keyboard and IRQ out of reset*/
	err = tc35893_write_byte(client, TC_RSTCTRL, reg_val);
	if (err < 0)
		return err;

	/*configure the KBDMFS*/
	err = tc35893_write_byte(client, TC_KBDMFS, TC_KBDMFS_EN);
	if (err < 0)
		return err;

	err = tc35893_read_byte(client, TC_MANUFACTURE_CODE, &manufacture_code);
	if (err < 0)
		return err;
	err = tc35893_read_byte(client, TC_VERSION_ID, &version_id);
	if (err < 0)
		return err;

	/* Now configure clock */
	err = tc35893_write_byte(client, TC_CLKEN, KPD_CLK_EN);
	if (err < 0)
		return err;

	err = tc35893_read_byte(client, TC_CLKEN, &reg_val);
	if (err < 0)
		return err;

	dev_info(&client->dev, "chip id: %x, version id:%x \n",
		manufacture_code, version_id);

	err = tc35893_write_byte(client, TC_RSTINTCLR, IRQ_CLEAR);
	if (err < 0)
		return err;

	/* End of --- Reset and initialise TC keypad controller */

	err = tc35893_kp_init_keypad(kp);
	if (err < 0)
		goto err_init_kpd;

	if (input_register_device(kp->inp_dev) < 0) {
		dev_err(&client->dev, "Could not register input device\n");
		err = -1;
		goto err_init_kpd;
	} else {
		dev_dbg(&client->dev,
			"Registered keypad module with input subsystem\n");
	}

	/* Initialize keypad workques  */
	INIT_DELAYED_WORK(&kp->kscan_work, tc35893_kp_wq_kscan);

	/* Enable wakeup from keypad */
	device_init_wakeup(&client->dev, kp->board->enable_wakeup);
	device_set_wakeup_capable(&client->dev, kp->board->enable_wakeup);

	/* Initialize keypad interrupt handler  */
	if (!kp->board->op_mode) {	/* true if interrupt mode operation */

		err = request_irq(kp->board->irq, tc35893_kp_irh,
				  kp->board->irqtype, kp->inp_dev->name, kp);
		if (err < 0) {
			dev_err(&client->dev,
				"Could not allocate irq %d,error %d\n",
				   kp->board->irq, err);
			goto err_inp_reg;
		}
		tc35893_kp_key_irqen(kp);

	} else {
		/* Schedule workqueue for polling mode operaion. */
		schedule_delayed_work(&kp->kscan_work, TC_KEYPAD_SCAN_PERIOD);
		dev_info(&client->dev, "Keypad polling started\n");
	}

	dev_info(&client->dev, "Module initialized Ver(%d.%d.%d)\n",
		 TC_KEYPAD_VER_MAJOR, TC_KEYPAD_VER_MINOR,
		 TC_KEYPAD_VER_RELEASE);
	return 0;

err_inp_reg:
	/* unregistering device */
	input_unregister_device(kp->inp_dev);
err_init_kpd:
	input_free_device(kp->inp_dev);
err_board:
	kfree(kp);
err_kzalloc:
	return err;
}

/**
 * tc35893_kp_remove - keypad module remove function
 *
 * @client:	i2c client pointer
 *
 * Disables Keypad interrupt if any, frees allocated keypad interrupt if any,
 * cancels keypad work queues if any, deallocate used GPIO pin, unregisters the
 * module, frees used memory
 */
static int __exit tc35893_kp_remove(struct i2c_client *client)
{
	struct tc35893_platform_data *board;
	struct tc_keypad *kp;

	if (!client)
		return -EINVAL;

	board = client->dev.platform_data;
	kp = i2c_get_clientdata(client);
	if (!board || !kp)
		return -EINVAL;

	/* disable all interrupts */
	tc35893_kp_key_irqdis(kp);

	/* cancel and flush keypad work queues if any  */
	cancel_delayed_work(&kp->kscan_work);
	cancel_delayed_work_sync(&kp->kscan_work);
	/* block until work struct's callback terminates */
	input_unregister_device(kp->inp_dev);
	free_irq(kp->board->irq, kp);
	i2c_set_clientdata(client, NULL);
	input_free_device(kp->inp_dev);
	kfree(kp);
	return 0;
}

static const struct i2c_device_id tc35893_id[] = {
	{ "tc35893-kp", 23 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tc35893_id);


static struct i2c_driver tc35893kpd_driver = {
	.driver = {
		   .name = "tc35893-kp",
		   .owner = THIS_MODULE,
		   },
	.probe = tc35893_kp_probe,
	.remove = __exit_p(tc35893_kp_remove),
	.suspend = tc35893_kp_suspend,
	.resume = tc35893_kp_resume,
	.id_table = tc35893_id,
};

static int __init tc35893_kp_init(void)
{
	return i2c_add_driver(&tc35893kpd_driver);
}

static void __exit tc35893_kp_exit(void)
{
	return i2c_del_driver(&tc35893kpd_driver);
}

module_init(tc35893_kp_init);
module_exit(tc35893_kp_exit);

MODULE_AUTHOR("jayeeta banerjee");
MODULE_DESCRIPTION("tc35893 keyboard driver");
MODULE_LICENSE("GPL");

