/*
 * Copyright (C) 2007-2009 ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2
 * Low-level core for exclusive access to the AB3550 IC on the I2C bus
 * and some basic chip-configuration.
 * Author: Bengt Jönsson <bengt.g.jonsson@stericsson.com>
 * Author: Mattias Nilsson <mattias.i.nilsson@stericsson.com>
 */

#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/mfd/abx.h>

/* */
#define ABX_NAME_STRING "ab3550"
#define ABX_ID_FORMAT_STRING "AB3550 %s"

/* These are the only registers inside AB3550 used in this main file */

/* Chip ID register */
#define ABX_CID_BANK          0
#define ABX_CID_REG           0x20

/* Interrupt event registers */
#define ABX_EVENT_BANK        0
#define ABX_EVENT_REG         0x22

/* Interrupt mask registers */
#define AB3550_IMR3 0x2b

/* Read/write operation values. */
#define ABX_PERM_RD (0x01)
#define ABX_PERM_WR (0x02)

/* Read/write permissions. */
#define ABX_PERM_RO (ABX_PERM_RD)
#define ABX_PERM_RW (ABX_PERM_RD | ABX_PERM_WR)

/**
 * struct abx
 * @access_mutex: lock out concurrent accesses to the ABX registers
 * @dev: pointer to the containing device
 * @i2c_client: I2C client for this chip
 * @testreg_client: secondary client for test registers
 * @chip_name: name of this chip variant
 * @chip_id: 8 bit chip ID for this chip variant
 * @work: an event handling worker
 * @event_subscribers: event subscribers are listed here
 * @events: current events, owned by the interrupt handler and worker
 * @startup_events: a copy of the first reading of the event registers
 * @startup_events_read: whether the first events have been read
 * @devlist: a list of handles for the subdevices
 *
 * This struct is PRIVATE and devices using it should NOT
 * access ANY fields.
 */
struct abx {
	struct mutex access_mutex;
	struct device *dev;
	struct i2c_client *i2c_client[ABX_NUM_BANKS];
	char chip_name[32];
	u8 chip_id;
	struct work_struct work;
	struct blocking_notifier_head event_subscribers;
	u8 events[ABX_NUM_EVENT_REG];
	u8 startup_events[ABX_NUM_EVENT_REG];
	bool startup_events_read;
	struct abx_dev *devlist;
};

/**
 * struct abx_reg_range
 * @first: the first address of the range
 * @last: the last address of the range
 * @perm: access permissions for the range
 */
struct abx_reg_range {
	u8 first;
	u8 last;
	u8 perm;
};

/**
 * struct abx_reg_ranges
 * @count: the number of ranges in the list
 * @range: the list of register ranges
 */
struct abx_reg_ranges {
	u8 count;
	const struct abx_reg_range *range;
};

/**
 * struct abx_devinfo
 * @pdev: a platform device structure for the subdevice
 * @reg_ranges: a list of register access permissions for the subdevice
 */
struct abx_devinfo {
	struct platform_device pdev;
	const struct abx_reg_ranges reg_ranges[ABX_NUM_BANKS];
};


/*
 * Permissible register ranges for reading and writing per device and bank.
 *
 * The ranges must be listed in increasing address order, and no overlaps are
 * allowed. It is assumed that write permission implies read permission
 * (i.e. only RO and RW permissions should be used).  Ranges with write
 * permission must not be split up.
 *
 * TODO:These lists will have to be updated when more is known about Petronella.
 */

#define NO_RANGE {.count = 0, .range = NULL,}

static struct abx_devinfo ab3550_devs[] = {
	{
		.pdev = {
			.name = "ab3550-dac",
			.id = -1,
		},
		.reg_ranges = {
			{
				.count = 1,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x29,
						.last = 0x2d,
						.perm = ABX_PERM_RW,
					},
				}

			},
			{
				.count = 1,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0xb0,
						.last = 0xc3,
						.perm = ABX_PERM_RW,
					},
				}

			},
		},
	},
	{
		.pdev = {
			.name = "ab3550-leds",
			.id = -1,
		},
		.reg_ranges = {
			{
				.count = 1,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x29,
						.last = 0x2d,
						.perm = ABX_PERM_RW,
					},
				}

			},
			{
				.count = 1,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x5a,
						.last = 0xad,
						.perm = ABX_PERM_RW,
					},
				}
			},
		},
	},
	{
		.pdev = {
			.name = "ab3550-power",
			.id = -1,
		},
		.reg_ranges = {
			{
				.count = 2,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x21,
						.last = 0x21,
						.perm = ABX_PERM_RO,
					},
					{
						.first = 0x29,
						.last = 0x2d,
						.perm = ABX_PERM_RW,
					},

				}

			},
			NO_RANGE,
		},
	},
	{
		.pdev = {
			.name = "ab3550-regulators",
			.id = -1,
		},
		.reg_ranges = {

			{
				.count = 2,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x29,
						.last = 0x2d,
						.perm = ABX_PERM_RW,
					},
					{
						.first = 0x69,
						.last = 0xa4,
						.perm = ABX_PERM_RW,
					},
				}
			},
			NO_RANGE,
		},
	},
	{
		.pdev = {
			.name = "ab3550-sim",
			.id = -1,
		},
		.reg_ranges = {
			{
				.count = 2,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x21,
						.last = 0x21,
						.perm = ABX_PERM_RO,
					},
					{
						.first = 0x29,
						.last = 0x2d,
						.perm = ABX_PERM_RW,
					},
				}
			},
			{
				.count = 1,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x14,
						.last = 0x18,
						.perm = ABX_PERM_RW,
					},
				}

			},
		},
	},
	{
		.pdev = {
			.name = "ab3550-uart",
			.id = -1,
		},
		.reg_ranges = {
			NO_RANGE,
			NO_RANGE,
		},
	},
	{
		.pdev = {
			.name = "ab3550-rtc",
			.id = -1,
		},
		.reg_ranges = {
			{
				.count = 2,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x00,
						.last = 0x0c,
						.perm = ABX_PERM_RW,
					},
					{
						.first = 0x29,
						.last = 0x2d,
						.perm = ABX_PERM_RW,
					},
				}
			},
			NO_RANGE,
		},
	},
	{
		.pdev = {
			.name = "ab3550-charger",
			.id = -1,
		},
		.reg_ranges = {
			{
				.count = 3,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x10,
						.last = 0x1c,
						.perm = ABX_PERM_RW,
					},
					{
						.first = 0x21,
						.last = 0x21,
						.perm = ABX_PERM_RO,
					},
					{
						.first = 0x29,
						.last = 0x2d,
						.perm = ABX_PERM_RW,
					},
				}
			},
			NO_RANGE,
		},
	},
	{
		.pdev = {
			.name = "ab3550-adc",
			.id = -1,
		},
		.reg_ranges = {
			{
				.count = 1,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x29,
						.last = 0x2d,
						.perm = ABX_PERM_RW,
					},

				}
			},
			{
				.count = 1,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x20,
						.last = 0x57,
						.perm = ABX_PERM_RW,
					},

				}
			},
		},
	},
	{
		.pdev = {
			.name = "ab3550-fuelgauge",
			.id = -1,
		},
		.reg_ranges = {
			{
				.count = 2,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x21,
						.last = 0x21,
						.perm = ABX_PERM_RO,
					},
					{
						.first = 0x29,
						.last = 0x2d,
						.perm = ABX_PERM_RW,
					},
				}
			},
			{
				.count = 1,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x00,
						.last = 0x0f,
						.perm = ABX_PERM_RW,
					},
				}
			},
		},
	},
	{
		.pdev = {
			.name = "ab3550-vibrator",
			.id = -1,
		},
		.reg_ranges = {
			{
				.count = 1,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x29,
						.last = 0x2d,
						.perm = ABX_PERM_RW,
					},
				}
			},
			{
				.count = 1,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x10,
						.last = 0x13,
						.perm = ABX_PERM_RW,
					},

				}
			},
		},
	},
	{
		/* The codec entry must be the last one as long as the function
		 * get_u300_codec_device (defined below) exists.
		 */
		.pdev = {
			.name = "ab3550-codec",
			.id = -1,
		},
		.reg_ranges = {
			{
				.count = 2,
				.range = (struct abx_reg_range[]) {
					{
						.first = 0x29,
						.last = 0x2d,
						.perm = ABX_PERM_RW,
					},
					{
						.first = 0x31,
						.last = 0x68,
						.perm = ABX_PERM_RW,
					},
				}
			},
			NO_RANGE,
		},
	},
};

u8 abx_get_chip_type(struct abx_dev *abx_dev)
{
	u8 chip = ABUNKNOWN;

	switch (abx_dev->abx->chip_id & 0xf0) {
	case 0xa0:
		chip = AB3000;
		break;
	case 0xc0:
		chip = AB3100;
		break;
	case 0x10:
		chip = AB3550;
		break;
	}
	return chip;
}
EXPORT_SYMBOL(abx_get_chip_type);

/*
 * I2C transactions with error messages.
 */
static int abx_i2c_master_send(struct abx *abx, u8 bank, u8 *data, u8 count)
{
	int err;

	err = i2c_master_send(abx->i2c_client[bank], data, count);
	if (err < 0)
		dev_err(abx->dev, "send error: %d\n", err);
	else
		err = 0;
	return err;
}

static int abx_i2c_master_recv(struct abx *abx, u8 bank, u8 reg, u8 *data)
{
	int err;
	struct i2c_msg msg[2];

	msg[0].addr = abx->i2c_client[bank]->addr;
	msg[0].flags = 0x0;
	msg[0].len = 1;
	msg[0].buf = &reg;

	msg[1].addr = abx->i2c_client[bank]->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = data;

	err = i2c_transfer(abx->i2c_client[bank]->adapter, msg, 2);
	if (err < 0)
		dev_err(abx->dev, "receive error: %d\n", err);
	else
		err = 0;
	return err;
}

/*
 * Functionality for getting/setting mixed signal registers
 */
static int get_register_interruptible(struct abx *abx, u8 bank, u8 reg,
		u8 *value)
{
	int err;

	err = mutex_lock_interruptible(&abx->access_mutex);
	if (err)
		return err;
	err = abx_i2c_master_recv(abx, bank, reg, value);
	mutex_unlock(&abx->access_mutex);
	return err;
}

static int get_register_page_interruptible(struct abx *abx, u8 bank,
		u8 first_reg, u8 *regvals, u8 numregs)
{
	BUG();
	return -EINVAL;
}

static int mask_and_set_register_interruptible(struct abx *abx, u8 bank,
	       u8 reg, u8 bitmask, u8 bitvalues)
{
	int err = 0;

	if (likely(bitmask)) {
		u8 reg_bits[2] = {reg, 0};

		err = mutex_lock_interruptible(&abx->access_mutex);
		if (err)
			return err;

		if (bitmask == 0xFF) /* No need to read in this case */
			reg_bits[1] = bitvalues;
		else {
			u8 bits;

			/* First read out the target register. */
			err = abx_i2c_master_recv(abx, bank, reg, &bits);
			if (err)
				goto unlock_and_return;

			/* Modify the bits. */
			reg_bits[1] = ((~bitmask & bits) |
				       (bitmask & bitvalues));
		}
		/* Write the register */
		err = abx_i2c_master_send(abx, bank, reg_bits, 2);

unlock_and_return:
		mutex_unlock(&abx->access_mutex);
	}
	return err;
}

/*
 * Read/write permission checking functions.
 */
static bool page_write_allowed(const struct abx_reg_ranges *ranges,
	u8 first_reg, u8 last_reg)
{
	u8 i;

	if (last_reg < first_reg)
		return false;
	for (i = 0; i < ranges->count; i++) {
		if (first_reg < ranges->range[i].first)
			break;
		if ((last_reg <= ranges->range[i].last) &&
			(ranges->range[i].perm & ABX_PERM_WR))
			return true;
	}
	return false;
}

static bool reg_write_allowed(const struct abx_reg_ranges *ranges, u8 reg)
{
	return page_write_allowed(ranges, reg, reg);
}

static bool page_read_allowed(const struct abx_reg_ranges *ranges, u8 first_reg,
		u8 last_reg)
{
	u8 i;

	if (last_reg < first_reg)
		return false;
	/* Find the range (if it exists in the list) that includes first_reg. */
	for (i = 0; i < ranges->count; i++) {
		if (first_reg < ranges->range[i].first)
			return false;
		if (first_reg <= ranges->range[i].last)
			break;
	}
	/* Make sure that the entire range up to and including last_reg is
	 * readable. This may span several of the ranges in the list.
	 */
	while ((i < ranges->count) && (ranges->range[i].perm & ABX_PERM_RD)) {
		if (last_reg <= ranges->range[i].last)
			return true;
		if ((++i >= ranges->count) ||
			(ranges->range[i].first !=
				(ranges->range[i - 1].last + 1))) {
			break;
		}
	}
	return false;
}

static bool reg_read_allowed(const struct abx_reg_ranges *ranges, u8 reg)
{
	return page_read_allowed(ranges, reg, reg);
}

/*
 * The exported register access functionality.
 */
int abx_set_register_interruptible(struct abx_dev *abx_dev, u8 bank, u8 reg,
	u8 value)
{
	return abx_mask_and_set_register_interruptible(abx_dev, bank, reg,
						       0xFF, value);
}
EXPORT_SYMBOL(abx_set_register_interruptible);

int abx_get_register_interruptible(struct abx_dev *abx_dev, u8 bank, u8 reg,
	 u8 *value)
{
	if ((ABX_NUM_BANKS <= bank) ||
		!reg_read_allowed(&abx_dev->devinfo->reg_ranges[bank], reg))
		return -EINVAL;

	return get_register_interruptible(abx_dev->abx, bank, reg, value);
}
EXPORT_SYMBOL(abx_get_register_interruptible);

int abx_get_register_page_interruptible(struct abx_dev *abx_dev, u8 bank,
	u8 first_reg, u8 *regvals, u8 numregs)
{
	if ((ABX_NUM_BANKS <= bank) ||
		!page_read_allowed(&abx_dev->devinfo->reg_ranges[bank],
			first_reg, (first_reg + numregs - 1)))
		return -EINVAL;

	return get_register_page_interruptible(abx_dev->abx, bank, first_reg,
		regvals, numregs);
}
EXPORT_SYMBOL(abx_get_register_page_interruptible);

int abx_mask_and_set_register_interruptible(struct abx_dev *abx_dev, u8 bank,
	 u8 reg, u8 bitmask, u8 bitvalues)
{
	if ((ABX_NUM_BANKS <= bank) ||
		!reg_write_allowed(&abx_dev->devinfo->reg_ranges[bank], reg))
		return -EINVAL;

	return mask_and_set_register_interruptible(abx_dev->abx, bank, reg,
		 bitmask, bitvalues);
}
EXPORT_SYMBOL(abx_mask_and_set_register_interruptible);

/*
 * Register a simple callback for handling any AB3550 events.
 */
int abx_event_register(struct abx_dev *abx_dev, struct notifier_block *nb)
{
	return blocking_notifier_chain_register(
			&abx_dev->abx->event_subscribers, nb);
}
EXPORT_SYMBOL(abx_event_register);

/*
 * Remove a previously registered callback.
 */
int abx_event_unregister(struct abx_dev *abx_dev, struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(
			&abx_dev->abx->event_subscribers, nb);
}
EXPORT_SYMBOL(abx_event_unregister);

int abx_event_registers_startup_state_get(struct abx_dev *abx_dev, u8 *event)
{
	if (!abx_dev->abx->startup_events_read)
		return -EAGAIN; /* Try again later */
	memcpy(event, abx_dev->abx->startup_events, ABX_NUM_EVENT_REG);
	return 0;
}
EXPORT_SYMBOL(abx_event_registers_startup_state_get);

/* Interrupt handling worker */
static void abx_work(struct work_struct *work)
{
	struct abx *abx = container_of(work, struct abx, work);
	int err;
	int i;

	err = get_register_page_interruptible(abx, ABX_EVENT_BANK,
		ABX_EVENT_REG, abx->events, ABX_NUM_EVENT_REG);
	if (err)
		goto err_event_wq;

	if (!abx->startup_events_read) {
		memcpy(abx->startup_events, abx->events, ABX_NUM_EVENT_REG);
		abx->startup_events_read = true;
	}

	/*
	 * The notified parties will have to mask out the events
	 * they're interested in and react to them. They will be
	 * notified on all events, then they use the event array
	 * to determine if they're interested.
	 */
	blocking_notifier_call_chain(&abx->event_subscribers, ABX_NUM_EVENT_REG,
		abx->events);

	for (i = 0; i < ABX_NUM_EVENT_REG; i++)
		dev_dbg(abx->dev, "IRQ Event[%d]: 0x%2x\n", i, abx->events[i]);

	/* By now the IRQ should be acked and deasserted so enable it again */
	enable_irq(abx->i2c_client[0]->irq);
	return;

err_event_wq:
	dev_dbg(abx->dev, "error in event workqueue\n");
	/* Enable the IRQ anyway, what choice do we have? */
	enable_irq(abx->i2c_client[0]->irq);
	return;
}

#ifdef CONFIG_DEBUG_FS

static struct abx_reg_ranges debug_ranges[ABX_NUM_BANKS] = {
	{
		.count = 6,
		.range = (struct abx_reg_range[]) {
			{
				.first = 0x00,
				.last = 0x0e,
			},
			{
				.first = 0x10,
				.last = 0x1a,
			},
			{
				.first = 0x1e,
				.last = 0x4f,
			},
			{
				.first = 0x51,
				.last = 0x63,
			},
			{
				.first = 0x65,
				.last = 0xa3,
			},
			{
				.first = 0xa5,
				.last = 0xa9,
			},
		}
	},
	{
		.count = 5,
		.range = (struct abx_reg_range[]) {
			{
				.first = 0x00,
				.last = 0x0e,
			},
			{
				.first = 0x10,
				.last = 0x17,
			},
			{
				.first = 0x1a,
				.last = 0x56,
			},
			{
				.first = 0x5a,
				.last = 0xac,
			},
			{
				.first = 0xb0,
				.last = 0xc2,
			},
		}
	},
};

static int abx_registers_print(struct seq_file *s, void *p)
{
	struct abx *abx = s->private;
	int bank;

	seq_printf(s, ABX_NAME_STRING " register values:\n");

	for (bank = 0; bank < ABX_NUM_BANKS; bank++) {
		unsigned int i;

		seq_printf(s, " bank %d:\n", bank);
		for (i = 0; i < debug_ranges[bank].count; i++) {
			u8 reg;

			for (reg = debug_ranges[bank].range[i].first;
				reg <= debug_ranges[bank].range[i].last;
				reg++) {
				u8 value;

				get_register_interruptible(abx, bank, reg,
					&value);
				seq_printf(s, "  [%d/0x%02X]: 0x%02X\n", bank,
					 reg, value);
			}
		}
	}
	return 0;
}

static int abx_registers_open(struct inode *inode, struct file *file)
{
	return single_open(file, abx_registers_print, inode->i_private);
}

static const struct file_operations abx_registers_fops = {
	.open = abx_registers_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.owner = THIS_MODULE,
};

struct abx_get_set_reg_priv {
	struct abx *abx;
	bool mode;
};

static int abx_get_set_reg_open_file(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t abx_get_set_reg(struct file *file,
			      const char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	struct abx_get_set_reg_priv *priv = file->private_data;
	struct abx *abx = priv->abx;
	char buf[32];
	int buf_size;
	int regp;
	int bankp;
	unsigned long user_reg;
	unsigned long user_bank;
	int err;
	int i = 0;


	/* Get userspace string and assure termination */
	buf_size = min(count, (sizeof(buf)-1));
	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;
	buf[buf_size] = 0;

	/*
	 * The idea is here to parse a string which is either
	 * "b 0xnn" for reading a register with adress 0xnn in
	 * bank b, or "b 0xaa 0xbb" for writing 0xbb to the
	 * register 0xaa in bank b. First move past
	 * whitespace and then begin to parse the bank.
	 */
	while ((i < buf_size) && (buf[i] == ' '))
		i++;
	if (i >= (buf_size - 1))
		goto wrong_input;
	bankp = i;

	/*
	 * Advance pointer to end of string then terminate
	 * the bank string. This is needed to satisfy
	 * the strict_strtoul() function.
	 */
	while ((i < buf_size) && (buf[i] != ' '))
		i++;
	if (i >= buf_size)
		goto wrong_input;
	buf[i++] = '\0';

	err = strict_strtoul(&buf[bankp], 0, &user_bank);
	if (err)
		goto wrong_input;
	if (user_bank >= ABX_NUM_BANKS) {
		dev_err(abx->dev, "debug input error: invalid bank number");
		return -EINVAL;
	}

	while ((i < buf_size) && (buf[i] == ' '))
		i++;
	if (i >= (buf_size - 1))
		goto wrong_input;
	regp = i;

	/*
	 * Advance pointer to end of string then terminate
	 * the register string. This is needed to satisfy
	 * the strict_strtoul() function.
	 */
	while ((i < buf_size) && (buf[i] != ' '))
		i++;
	if ((i >= buf_size) && priv->mode)
		goto wrong_input;
	buf[i] = '\0';

	err = strict_strtoul(&buf[regp], 0, &user_reg);
	if (err)
		goto wrong_input;
	if (user_reg > 0xff) {
		dev_err(abx->dev,
			"debug input error: invalid register address");
		return -EINVAL;
	}

	/* Either we read or we write a register here */
	if (!priv->mode) {
		/* Reading */

		u8 regvalue;

		get_register_interruptible(abx, (u8)user_bank, (u8)user_reg,
					   &regvalue);

		dev_info(abx->dev,
			 "debug read " ABX_NAME_STRING " [%d/0x%02X]: 0x%02X\n",
			 (u8)user_bank, (u8)user_reg, regvalue);
	} else {
		/* Writing */

		int valp;
		unsigned long user_value;
		u8 regvalue;

		/*
		 * We need some value to write to
		 * the register so keep parsing the string
		 * from userspace.
		 */
		i++;
		while ((i < buf_size) && (buf[i] == ' '))
			i++;
		if (i >= (buf_size - 1))
			goto wrong_input;
		valp = i;
		while ((i < buf_size) && (buf[i] != ' '))
			i++;
		buf[i] = '\0';

		err = strict_strtoul(&buf[valp], 0, &user_value);
		if (err)
			goto wrong_input;
		if (user_reg > 0xff) {
			dev_err(abx->dev,
				"debug input error: invalid register value");
			return -EINVAL;
		}

		mask_and_set_register_interruptible(abx, (u8)user_bank,
					   (u8)user_reg, 0xFF, (u8)user_value);
		get_register_interruptible(abx, (u8)user_bank, (u8)user_reg,
					   &regvalue);

		dev_info(abx->dev,
			 "debug write [%d/0x%02X] with 0x%02X, "
			 "after readback: 0x%02X\n",
			 (u8)user_bank, (u8)user_reg, (u8)user_value, regvalue);
	}
	return buf_size;
wrong_input:
	dev_err(abx->dev, "debug input error: should be \"B 0xRR%s\" "
			"(B: bank, RR: register%s)\n",
			(priv->mode ? " 0xVV" : ""),
			(priv->mode ? ", VV: value" : ""));
	return -EINVAL;
}

static const struct file_operations abx_get_set_reg_fops = {
	.open = abx_get_set_reg_open_file,
	.write = abx_get_set_reg,
};

static struct dentry *abx_dir;
static struct dentry *abx_reg_file;
static struct abx_get_set_reg_priv abx_get_priv;
static struct dentry *abx_get_reg_file;
static struct abx_get_set_reg_priv abx_set_priv;
static struct dentry *abx_set_reg_file;

static inline void abx_setup_debugfs(struct abx *abx)
{
	int err;

	abx_dir = debugfs_create_dir(ABX_NAME_STRING, NULL);
	if (!abx_dir)
		goto exit_no_debugfs;

	abx_reg_file = debugfs_create_file("registers",
				S_IRUGO, abx_dir, abx,
				&abx_registers_fops);
	if (!abx_reg_file) {
		err = -ENOMEM;
		goto exit_destroy_dir;
	}

	abx_get_priv.abx = abx;
	abx_get_priv.mode = false;
	abx_get_reg_file = debugfs_create_file("get_reg",
				S_IWUGO, abx_dir, &abx_get_priv,
				&abx_get_set_reg_fops);
	if (!abx_get_reg_file) {
		err = -ENOMEM;
		goto exit_destroy_reg;
	}

	abx_set_priv.abx = abx;
	abx_set_priv.mode = true;
	abx_set_reg_file = debugfs_create_file("set_reg",
				S_IWUGO, abx_dir, &abx_set_priv,
				&abx_get_set_reg_fops);
	if (!abx_set_reg_file) {
		err = -ENOMEM;
		goto exit_destroy_get_reg;
	}
	return;

 exit_destroy_get_reg:
	debugfs_remove(abx_get_reg_file);
 exit_destroy_reg:
	debugfs_remove(abx_reg_file);
 exit_destroy_dir:
	debugfs_remove(abx_dir);
 exit_no_debugfs:
	return;

}
static inline void abx_remove_debugfs(void)
{
	debugfs_remove(abx_set_reg_file);
	debugfs_remove(abx_get_reg_file);
	debugfs_remove(abx_reg_file);
	debugfs_remove(abx_dir);
}
#else
static inline void abx_setup_debugfs(struct abx *abx)
{
}
static inline void abx_remove_debugfs(void)
{
}
#endif

/*
 * Basic set-up, datastructure creation/destruction and I2C interface.
 * This sets up a default config in the AB3550 chip so that it
 * will work as expected.
 *
 * TODO: This should be moved to the machine specific platform data structure.
 */

struct abx_init_setting {
	u8 bank;
	u8 reg;
	u8 setting;
};

static const struct abx_init_setting __initdata
ab3550_init_settings[] = {
	/* TODO: Fill this with more Petronella init values. */
	{
		.bank = 0,
		.reg = AB3550_IMR3,
		.setting = 0x00
	},
};

static int __init abx_setup(struct abx *abx)
{
	int err = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(ab3550_init_settings); i++) {
		err = mask_and_set_register_interruptible(abx,
			ab3550_init_settings[i].bank,
			ab3550_init_settings[i].reg,
			0xFF, ab3550_init_settings[i].setting);
		if (err)
			goto exit_no_setup;
	}

exit_no_setup:
	return err;
}

/* This function should be removed when possible.
 * See arch/arm/mach-u300/i2c.c for details.
 */
struct device *get_u300_codec_device(void)
{
	/* For now, we simply assume that the codec driver is the last entry in
	 * ab3550_devs.
	 */
	return &ab3550_devs[ARRAY_SIZE(ab3550_devs) - 1].pdev.dev;
}
EXPORT_SYMBOL(get_u300_codec_device);

struct ab_family_id {
	u8	id;
	char	*name;
};

static const struct ab_family_id ids[] __initdata = {
	/* AB3550 */
	{
		.id = 0x10,
		.name = "P1A"
	},
	/* Terminator */
	{
		.id = 0x00,
	}
};

static int __init abx_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct abx *abx;
	struct abx_platform_data *abx_plf_data = client->dev.platform_data;
	int err;
	int i;
	int num_i2c_clients = 0;

	abx = kzalloc(sizeof(struct abx), GFP_KERNEL);
	if (!abx) {
		dev_err(&client->dev,
			"could not allocate " ABX_NAME_STRING " device\n");
		return -ENOMEM;
	}
	abx->devlist = kcalloc(ARRAY_SIZE(ab3550_devs),
				sizeof(struct abx_dev), GFP_KERNEL);
	if (!abx->devlist) {
		dev_err(&client->dev,
			"could not allocate " ABX_NAME_STRING " subdevices\n");
		err = -ENOMEM;
		goto free_abx_and_return;
	}

	/* Initialize data structure */
	mutex_init(&abx->access_mutex);
	BLOCKING_INIT_NOTIFIER_HEAD(&abx->event_subscribers);

	abx->i2c_client[num_i2c_clients] = client;
	num_i2c_clients++;
	abx->dev = &client->dev;

	i2c_set_clientdata(client, abx);

	/* Read chip ID register */
	err = get_register_interruptible(abx, ABX_CID_BANK, ABX_CID_REG,
		&abx->chip_id);
	if (err) {
		dev_err(&client->dev, "could not communicate with the analog "
			"baseband chip\n");
		goto exit_no_detect;
	}

	for (i = 0; ids[i].id != 0x0; i++) {
		if (ids[i].id == abx->chip_id) {
			snprintf(&abx->chip_name[0], sizeof(abx->chip_name) - 1,
				ABX_ID_FORMAT_STRING, ids[i].name);
			break;
		}
	}

	if (ids[i].id == 0x0) {
		dev_err(&client->dev, "unknown analog baseband chip id: 0x%x\n",
			abx->chip_id);
		dev_err(&client->dev, "driver not started!\n");
		goto exit_no_detect;
	}

	dev_info(&client->dev, "detected chip: %s\n", &abx->chip_name[0]);

	/* Attach other dummy I2C clients. */
	while (num_i2c_clients < ABX_NUM_BANKS) {
		abx->i2c_client[num_i2c_clients] =
			i2c_new_dummy(client->adapter,
				(client->addr + num_i2c_clients));
		if (!abx->i2c_client[num_i2c_clients]) {
			err = -ENOMEM;
			goto exit_no_dummy_client;
		}
		strlcpy(abx->i2c_client[num_i2c_clients]->name, id->name,
			sizeof(abx->i2c_client[num_i2c_clients]->name));
		num_i2c_clients++;
	}

	err = abx_setup(abx);
	if (err)
		goto exit_no_setup;

	INIT_WORK(&abx->work, abx_work);

	/* Set parent and a pointer back to the container in device data. */
	for (i = 0; i < ARRAY_SIZE(ab3550_devs); i++) {
		int x;
		ab3550_devs[i].pdev.dev.parent = &client->dev;
		ab3550_devs[i].pdev.dev.platform_data = abx_plf_data;
		abx->devlist[i].abx = abx;
		abx->devlist[i].devinfo = &ab3550_devs[i];
		platform_set_drvdata(&ab3550_devs[i].pdev,
			&abx->devlist[i]);
		x = platform_device_register(&ab3550_devs[i].pdev);
		printk(KERN_DEBUG "platform device %s registered %s\n",
		       ab3550_devs[i].pdev.name,
		       x ? "unsuccessfully" : "successfully");
	}

	abx_setup_debugfs(abx);

	return 0;

exit_no_setup:
exit_no_dummy_client:
	/* Unregister the dummy i2c clients. */
	while (--num_i2c_clients)
		i2c_unregister_device(abx->i2c_client[num_i2c_clients]);

exit_no_detect:
	kfree(abx->devlist);
free_abx_and_return:
	kfree(abx);
	return err;
}

static int __exit abx_remove(struct i2c_client *client)
{
	struct abx *abx = i2c_get_clientdata(client);
	int i;
	int num_i2c_clients = ABX_NUM_BANKS;

	/* Unregister subdevices */
	for (i = 0; i < ARRAY_SIZE(ab3550_devs); i++)
		platform_device_unregister(&ab3550_devs[i].pdev);

	abx_remove_debugfs();

	while (num_i2c_clients > 1) {
		num_i2c_clients--;
		i2c_unregister_device(abx->i2c_client[num_i2c_clients]);
	}

	/*
	 * At this point, all subscribers should have unregistered
	 * their notifiers so deactivate IRQ
	 */
	free_irq(client->irq, abx);
	kfree(abx->devlist);
	kfree(abx);
	return 0;
}

static const struct i2c_device_id abx_id[] = {
	{ ABX_NAME_STRING, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, abx_id);

static struct i2c_driver abx_driver = {
	.driver = {
		.name	= ABX_NAME_STRING,
		.owner	= THIS_MODULE,
	},
	.id_table	= abx_id,
	.probe		= abx_probe,
	.remove		= __exit_p(abx_remove),
};

static int __init abx_i2c_init(void)
{
	return i2c_add_driver(&abx_driver);
}

static void __exit abx_i2c_exit(void)
{
	i2c_del_driver(&abx_driver);
}

subsys_initcall(abx_i2c_init);
module_exit(abx_i2c_exit);

MODULE_AUTHOR("Mattias Nilsson <mattias.i.nilsson@stericsson.com>");
MODULE_DESCRIPTION("AB3550 core driver");
MODULE_LICENSE("GPL");
