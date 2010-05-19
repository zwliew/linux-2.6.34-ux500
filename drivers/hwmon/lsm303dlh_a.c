/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name          : lsm303dlh_a_char.c
* Authors            : MH - C&I BU - Application Team
*		     : Carmine Iascone (carmine.iascone@st.com)
*		     : Matteo Dameno (matteo.dameno@st.com)
* Version            : V 0.3
* Date               : 24/02/2010
* Description        : LSM303DLH 6D module sensor API
*
********************************************************************************
*
* Author: Mian Yousaf Kaukab <mian.yousaf.kaukab@stericsson.com>
* Copyright 2010 (c) ST-Ericsson AB
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
*
******************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/uaccess.h>
#include <linux/lsm303dlh.h>
#include <linux/miscdevice.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/freezer.h>

/* Temp defines till GPIO extender driver is fixed. */
/* #define A_USE_INT */
/* #define A_USE_GPIO_EXTENDER_INT */

/* lsm303dlh accelerometer registers */
#define WHO_AM_I    0x0F

/* ctrl 1: pm2 pm1 pm0 dr1 dr0 zenable yenable zenable */
#define CTRL_REG1       0x20    /* power control reg */
#define CTRL_REG2       0x21    /* power control reg */
#define CTRL_REG3       0x22    /* power control reg */
#define CTRL_REG4       0x23    /* interrupt control reg */
#define CTRL_REG5       0x24    /* interrupt control reg */
#define AXISDATA_REG    0x28
#define INT1_SRC_A_REG  0x31    /* interrupt 1 source reg */
#define INT2_SRC_A_REG  0x35    /* interrupt 2 source reg */

/* Sensitivity adjustment */
#define SHIFT_ADJ_2G 4 /*    1/16*/
#define SHIFT_ADJ_4G 3 /*    2/16*/
#define SHIFT_ADJ_8G 2 /* ~3.9/16*/

/* Control register 1 */
#define LSM303DLH_A_CR1_PM_BIT 5
#define LSM303DLH_A_CR1_PM_MASK (0x7 << LSM303DLH_A_CR1_PM_BIT)
#define LSM303DLH_A_CR1_DR_BIT 3
#define LSM303DLH_A_CR1_DR_MASK (0x3 << LSM303DLH_A_CR1_DR_BIT)
#define LSM303DLH_A_CR1_EN_BIT 0
#define LSM303DLH_A_CR1_EN_MASK (0x7 << LSM303DLH_A_CR1_EN_BIT)

/* Control register 2 */
#define LSM303DLH_A_CR4_ST_BIT 1
#define LSM303DLH_A_CR4_ST_MASK (0x1 << LSM303DLH_A_CR4_ST_BIT)
#define LSM303DLH_A_CR4_STS_BIT 3
#define LSM303DLH_A_CR4_STS_MASK (0x1 << LSM303DLH_A_CR4_STS_BIT)
#define LSM303DLH_A_CR4_FS_BIT 4
#define LSM303DLH_A_CR4_FS_MASK (0x3 << LSM303DLH_A_CR4_FS_BIT)
#define LSM303DLH_A_CR4_BLE_BIT 6
#define LSM303DLH_A_CR4_BLE_MASK (0x3 << LSM303DLH_A_CR4_BLE_BIT)

/* Control register 3 */
#define LSM303DLH_A_CR3_I1_BIT 0
#define LSM303DLH_A_CR3_I1_MASK (0x3 << LSM303DLH_A_CR3_I1_BIT)
#define LSM303DLH_A_CR3_LIR1_BIT 2
#define LSM303DLH_A_CR3_LIR1_MASK (0x1 << LSM303DLH_A_CR3_LIR1_BIT)
#define LSM303DLH_A_CR3_I2_BIT 3
#define LSM303DLH_A_CR3_I2_MASK (0x3 << LSM303DLH_A_CR3_I2_BIT)
#define LSM303DLH_A_CR3_LIR2_BIT 5
#define LSM303DLH_A_CR3_LIR2_MASK (0x1 << LSM303DLH_A_CR3_LIR2_BIT)
#define LSM303DLH_A_CR3_PPOD_BIT 6
#define LSM303DLH_A_CR3_PPOD_MASK (0x1 << LSM303DLH_A_CR3_PPOD_BIT)
#define LSM303DLH_A_CR3_IHL_BIT 7
#define LSM303DLH_A_CR3_IHL_MASK (0x1 << LSM303DLH_A_CR3_IHL_BIT)

#define LSM303DLH_A_CR3_I_SELF 0x0
#define LSM303DLH_A_CR3_I_OR   0x1
#define LSM303DLH_A_CR3_I_DATA 0x2
#define LSM303DLH_A_CR3_I_BOOT 0x3

#define LSM303DLH_A_CR3_LIR_LATCH 0x1

/*
 * lsm303dlh_a acceleration data
 * brief Structure containing acceleration values for x,y and z-axis in
 * signed short
 */

struct lsm303dlh_a_t {
	short	x; /* < x-axis acceleration data. Range -2048 to 2047. */
	short	y; /* < y-axis acceleration data. Range -2048 to 2047. */
	short	z; /* < z-axis acceleration data. Range -2048 to 2047. */
};

struct lsm303dlh_a_data {
	struct i2c_client *client;
	struct lsm303dlh_platform_data pdata;
	struct input_dev *input_dev;
	struct work_struct work;
	u8 shift_adj;
	u8 ctrl_1;
	u8 mode;
	u8 rate;
	short ms;
	atomic_t user_count;
#ifndef A_USE_INT
	struct task_struct *kthread;
#endif
};

static struct lsm303dlh_a_data *file_private;

/* set lsm303dlh accelerometer bandwidth */
int lsm303dlh_a_set_rate(struct lsm303dlh_a_data *ldata, unsigned char bw)
{
	unsigned char data;

	data = i2c_smbus_read_byte_data(ldata->client, CTRL_REG1);
	data &= ~LSM303DLH_A_CR1_DR_MASK;
	ldata->rate = bw;
	data |= ((bw << LSM303DLH_A_CR1_DR_BIT) & LSM303DLH_A_CR1_DR_MASK);

	return i2c_smbus_write_byte_data(ldata->client, CTRL_REG1, data);
}

/* read selected bandwidth from lsm303dlh_acc */
int lsm303dlh_a_get_bandwidth(struct lsm303dlh_a_data *ldata, unsigned char *bw)
{
	unsigned char data;

	data = i2c_smbus_read_byte_data(ldata->client, CTRL_REG1);
	data &= LSM303DLH_A_CR1_DR_MASK;
	data >>= LSM303DLH_A_CR1_DR_BIT;

	return data;
}

#ifndef A_USE_INT
static int lsm303dlh_a_kthread(void *data);
#endif


int lsm303dlh_a_set_mode(struct lsm303dlh_a_data *ldata, unsigned char mode)
{
	unsigned char data;
	int res;

	data = i2c_smbus_read_byte_data(ldata->client, CTRL_REG1);

	data &= ~LSM303DLH_A_CR1_PM_MASK;
	ldata->mode = mode;
	data |= ((mode << LSM303DLH_A_CR1_PM_BIT) & LSM303DLH_A_CR1_PM_MASK);

	res = i2c_smbus_write_byte_data(ldata->client, CTRL_REG1, data);
	if (res < 0)
		return res;

#ifndef A_USE_INT
	if (mode) {
		if (!ldata->kthread) {
			ldata->kthread = kthread_run(lsm303dlh_a_kthread, ldata,
				"klsm303dlh_a");
			if (IS_ERR(ldata->kthread))
				return PTR_ERR(ldata->kthread);
		}
	} else {
		if (ldata->kthread) {
			kthread_stop(ldata->kthread);
			ldata->kthread = NULL;
		}
	}
#endif

	return 0;
}

int lsm303dlh_a_set_range(struct lsm303dlh_a_data *ldata, unsigned char range)
{
	switch (range) {
	case LSM303DLH_A_RANGE_2G:
		ldata->shift_adj = SHIFT_ADJ_2G;
		break;
	case LSM303DLH_A_RANGE_4G:
		ldata->shift_adj = SHIFT_ADJ_4G;
		break;
	case LSM303DLH_A_RANGE_8G:
		ldata->shift_adj = SHIFT_ADJ_8G;
		break;
	default:
		return -EINVAL;
	}

	range <<= LSM303DLH_A_CR4_FS_BIT;

	return i2c_smbus_write_byte_data(ldata->client, CTRL_REG4, range);
}

/*  i2c read routine for lsm303dlh accelerometer */
static char lsm303dlh_a_i2c_read(struct lsm303dlh_a_data *ldata,
			unsigned char reg_addr,
			unsigned char *data,
			unsigned char len)
{
	int res;
	int i = 0;

	while (i < len) {
		res = i2c_smbus_read_byte_data(ldata->client, reg_addr++);
		if (res >= 0) {
			data[i] = res & 0xff;
			i++;
		} else {
			dev_err(&ldata->client->dev, "i2c read error\n");
			return res;
		}
	}

	return len;
}

/* X,Y and Z-axis acceleration data readout */
int lsm303dlh_a_read_xyz(struct lsm303dlh_a_data *ldata,
				struct lsm303dlh_a_t *data)
{
	int res;
	unsigned char acc_data[6];
	int hw_d[3] = { 0 };

	/* check lsm303dlh_a_client */
	if (ldata->client == NULL) {
		printk(KERN_ERR "I2C driver not install\n");
		return -EFAULT;
	}

	res = lsm303dlh_a_i2c_read(ldata, AXISDATA_REG, &acc_data[0], 6);

	if (res >= 0) {
		hw_d[0] = (short) (((acc_data[1]) << 8) | acc_data[0]);
		hw_d[1] = (short) (((acc_data[3]) << 8) | acc_data[2]);
		hw_d[2] = (short) (((acc_data[5]) << 8) | acc_data[4]);

		hw_d[0] >>= ldata->shift_adj;
		hw_d[1] >>= ldata->shift_adj;
		hw_d[2] >>= ldata->shift_adj;

		data->x = ldata->pdata.negative_x ?
			-hw_d[ldata->pdata.axis_map_x] :
			 hw_d[ldata->pdata.axis_map_x];
		data->y = ldata->pdata.negative_y ?
			-hw_d[ldata->pdata.axis_map_y] :
			 hw_d[ldata->pdata.axis_map_y];
		data->z = ldata->pdata.negative_z ?
			-hw_d[ldata->pdata.axis_map_z] :
			 hw_d[ldata->pdata.axis_map_z];
	}

	return res;
}

/*  open command for lsm303dlh_a device file  */
static int lsm303dlh_a_open(struct inode *inode, struct file *filp)
{
	struct lsm303dlh_a_data *ldata = file_private;

	filp->private_data = ldata;

	if (ldata->client == NULL) {
		printk(KERN_ERR"I2C driver not install\n");
		return -EINVAL;
	}

	atomic_inc(&ldata->user_count);

	dev_dbg(&ldata->client->dev, "lsm303dlh_a has been opened\n");

	return 0;
}

/*  release command for lsm303dlh_a device file */
static int lsm303dlh_a_close(struct inode *inode, struct file *filp)
{
	struct lsm303dlh_a_data *ldata = filp->private_data;
	int res;

	res = atomic_dec_and_test(&ldata->user_count);

#ifndef A_USE_INT
	if ((res) && (ldata->kthread)) {
		kthread_stop(ldata->kthread);
		ldata->kthread = NULL;
	}
#endif

	dev_dbg(&ldata->client->dev, "lsm303dlh_a has been closed\n");

	return 0;
}

/*  ioctl command for lsm303dlh_a device file */
static int lsm303dlh_a_ioctl(struct inode *inode, struct file *filp,
			       unsigned int cmd, unsigned long arg)
{
	int err = 0;
	unsigned char buf[1];
	struct lsm303dlh_a_data *ldata = filp->private_data;

	/* check lsm303dlh_a_client */
	if (ldata->client == NULL) {
		printk(KERN_ERR "I2C driver not install\n");
		return -EFAULT;
	}

	/* cmd mapping */

	switch (cmd) {

	case LSM303DLH_A_SET_RANGE:
		if (copy_from_user(buf, (unsigned char *)arg, 1) != 0) {
			dev_err(&ldata->client->dev, "copy_from_user error\n");
			return -EFAULT;
		}
		err = lsm303dlh_a_set_range(ldata, buf[0]);
		return err;

	case LSM303DLH_A_SET_MODE:
		if (copy_from_user(buf, (unsigned char *)arg, 1) != 0) {
			dev_err(&ldata->client->dev, "copy_from_user error\n");
			return -EFAULT;
		}
		err = lsm303dlh_a_set_mode(ldata, buf[0]);

		return err;

	case LSM303DLH_A_SET_RATE:
		if (copy_from_user(buf, (unsigned char *)arg, 1) != 0) {
			dev_err(&ldata->client->dev, "copy_from_user error\n");
			return -EFAULT;
		}
		err = lsm303dlh_a_set_rate(ldata, buf[0]);
		return err;

	case LSM303DLH_A_GET_MODE:
		if (copy_to_user((unsigned char *)arg,
			&ldata->mode, sizeof(ldata->mode)) != 0) {
			dev_err(&ldata->client->dev, "copy_to_user error\n");
			return -EFAULT;
		}
		return 0;

	default:
		return 0;
	}
}

#ifndef A_USE_INT
static int lsm303dlh_a_calculate_ms(struct lsm303dlh_a_data *ldata)
{
	int ms;
	if (ldata->mode == LSM303DLH_A_MODE_NORMAL) {
		if (ldata->rate == LSM303DLH_A_RATE_50)
			ms = 20;
		else if (ldata->rate == LSM303DLH_A_RATE_100)
			ms = 10;
		else if (ldata->rate == LSM303DLH_A_RATE_400)
			ms = 2;
		else if (ldata->rate == LSM303DLH_A_RATE_1000)
			ms = 1;
		else
			ms = 1000;
	} else {
		if (ldata->mode == LSM303DLH_A_MODE_LP_HALF)
			ms = 2000;
		else if (ldata->mode == LSM303DLH_A_MODE_LP_1)
			ms = 1000;
		else if (ldata->mode == LSM303DLH_A_MODE_LP_2)
			ms = 500;
		else if (ldata->mode == LSM303DLH_A_MODE_LP_5)
			ms = 200;
		else if (ldata->mode == LSM303DLH_A_MODE_LP_10)
			ms = 100;
		else
			ms = 1000;
	}
	return ms;
}
#endif /* A_USE_INT */

#ifdef A_USE_INT
static void lsm303dlh_a_work_func(struct work_struct *work)
#else
static void lsm303dlh_a_work_func(struct lsm303dlh_a_data *ldata)
#endif /* A_USE_INT */
{
	int res;
	struct lsm303dlh_a_t abuf;

#ifdef A_USE_INT
	struct lsm303dlh_a_data *ldata =
		container_of(work, struct lsm303dlh_a_data, work);
#endif

	/* Clear interrupt. */
	res = i2c_smbus_read_byte_data(ldata->client, INT1_SRC_A_REG);

	res = lsm303dlh_a_read_xyz(ldata, &abuf);

	if (res >= 0) {
		input_report_abs(ldata->input_dev, ABS_X, abuf.x);
		input_report_abs(ldata->input_dev, ABS_Y, abuf.y);
		input_report_abs(ldata->input_dev, ABS_Z, abuf.z);

		input_sync(ldata->input_dev);
	}

#ifdef A_USE_INT
	enable_irq(ldata->client->irq);
#endif
}

#ifndef A_USE_INT
/*
 * Kthread polling function
 * @data: unused - here to conform to threadfn prototype
 */
static int lsm303dlh_a_kthread(void *data)
{
	struct lsm303dlh_a_data *ldata = data;

	while (!kthread_should_stop()) {
		lsm303dlh_a_work_func(ldata);
		try_to_freeze();
		msleep_interruptible(lsm303dlh_a_calculate_ms(ldata));
	}

	return 0;
}
#endif /* A_USE_INT */

#ifdef A_USE_INT
#ifdef A_USE_GPIO_EXTENDER_INT
void lsm303dlh_a_interrupt(void *dev_id)
#else
static irqreturn_t lsm303dlh_a_interrupt(int irq, void *dev_id)
#endif /* A_USE_GPIO_EXTENDER_INT */
{
	struct lsm303dlh_a_data *ldata = dev_id;
#ifndef A_USE_GPIO_EXTENDER_INT
	disable_irq(ldata->client->irq);
#endif /* A_USE_GPIO_EXTENDER_INT */
	schedule_work(&ldata->work);

#ifndef A_USE_GPIO_EXTENDER_INT
	return IRQ_HANDLED;
#endif /* A_USE_GPIO_EXTENDER_INT */
}
#endif /* A_USE_GPIO_EXTENDER_INT */

static const struct file_operations lsm303dlh_a_fops = {
	.owner = THIS_MODULE,
	.open = lsm303dlh_a_open,
	.release = lsm303dlh_a_close,
	.ioctl = lsm303dlh_a_ioctl,
};

static struct miscdevice lsm303dlh_a_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lsm303dlh_a",
	.fops = &lsm303dlh_a_fops,
};

static int lsm303dlh_a_validate_pdata(struct lsm303dlh_a_data *adata)
{
	if ((adata->pdata.axis_map_x > 2) ||
	    (adata->pdata.axis_map_y > 2) ||
	    (adata->pdata.axis_map_z > 2) ||
	    (adata->pdata.axis_map_x == adata->pdata.axis_map_y) ||
	    (adata->pdata.axis_map_x == adata->pdata.axis_map_z) ||
	    (adata->pdata.axis_map_y == adata->pdata.axis_map_z)) {
		dev_err(&adata->client->dev,
			"invalid axis_map value x:%u y:%u z%u\n",
			adata->pdata.axis_map_x, adata->pdata.axis_map_y,
			adata->pdata.axis_map_z);
		return -EINVAL;
	}

	/* Only allow 0 and 1 */
	if ((adata->pdata.negative_x > 1) ||
	    (adata->pdata.negative_y > 1) ||
	    (adata->pdata.negative_z > 1)) {
		dev_err(&adata->client->dev,
			"invalid negate value x:%u y:%u z:%u\n",
			adata->pdata.negative_x, adata->pdata.negative_y,
			adata->pdata.negative_z);
		return -EINVAL;
	}

#ifdef A_USE_GPIO_EXTENDER_INT
	if ((!adata->pdata.free_irq) || (!adata->pdata.register_irq))
		return -EINVAL;
#endif /* A_USE_GPIO_EXTENDER_INT */

	return 0;
}

static int lsm303dlh_a_probe(struct i2c_client *client,
			       const struct i2c_device_id *devid)
{
	int err = 0;
	int tempvalue;
	struct lsm303dlh_a_data *ldata;

	if (client->dev.platform_data == NULL) {
		dev_err(&client->dev, "platform data is NULL. exiting.\n");
		err = -ENODEV;
		goto exit;
	}
/* This check does not pass on i2c-stm?? */
#if 0
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit;
	}

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_I2C_BLOCK)) {
		err = -ENODEV;
		goto exit;
	}
#endif

	/*
	 * OK. For now, we presume we have a valid client. We now create the
	 * client structure, even though we cannot fill it completely yet.
	 */
	ldata = kzalloc(sizeof(*ldata), GFP_KERNEL);
	if (ldata == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, ldata);
	ldata->client = client;

	file_private = ldata;

	memcpy(&ldata->pdata, client->dev.platform_data, sizeof(ldata->pdata));

	err = lsm303dlh_a_validate_pdata(ldata);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto exit_kfree;
	}

	if (i2c_smbus_read_byte(client) < 0) {
		dev_err(&client->dev, "i2c_smbus_read_byte error!!\n");
		goto exit_kfree;
	} else {
		dev_info(&client->dev, "lsm303dlh_a Device detected!\n");
	}

	/* read chip id */
	tempvalue = i2c_smbus_read_byte_data(client, WHO_AM_I);
	if ((tempvalue & 0x00FF) == 0x0032) {
		dev_err(&client->dev, "i2c driver registered!\n");
	} else {
		err = -EINVAL;
		ldata->client = NULL;
		goto exit_kfree;
	}

#ifdef A_USE_INT
	INIT_WORK(&ldata->work, lsm303dlh_a_work_func);
#ifdef A_USE_GPIO_EXTENDER_INT
	if (ldata->pdata.register_irq(&client->dev, client,
		lsm303dlh_a_interrupt)) {
#else
	if (request_irq(client->irq, lsm303dlh_a_interrupt,
		IRQF_TRIGGER_HIGH, "lsm303dlh_a", ldata)) {
#endif /* A_USE_GPIO_EXTENDER_INT */
		err = -EBUSY;
		dev_err(&client->dev, "request irq failed\n");
		goto exit_kfree;
	}
#endif /* A_USE_INT */

	/* Set active high, latched interrupt when data is ready. */
	if (i2c_smbus_write_byte_data(client, CTRL_REG3,
		((LSM303DLH_A_CR3_I_DATA << LSM303DLH_A_CR3_I1_BIT) |
		(LSM303DLH_A_CR3_LIR_LATCH << LSM303DLH_A_CR3_LIR1_BIT)))) {
		dev_err(&client->dev, "interrutp configuration failed\n");
		err = -EINVAL;
		goto exit_free_irq;
	}

	if (misc_register(&lsm303dlh_a_misc_device)) {
		dev_err(&client->dev, "misc_register failed\n");
		err = -EINVAL;
		goto exit_free_irq;
	}

	ldata->input_dev = input_allocate_device();
	if (!ldata->input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "Failed to allocate input device\n");
		goto exit_deregister_misc;
	}

	set_bit(EV_ABS, ldata->input_dev->evbit);

	/* x-axis acceleration */
	input_set_abs_params(ldata->input_dev, ABS_X, -2048, 2047, 0, 0);
	/* y-axis acceleration */
	input_set_abs_params(ldata->input_dev, ABS_Y, -2048, 2047, 0, 0);
	/* z-axis acceleration */
	input_set_abs_params(ldata->input_dev, ABS_Z, -2048, 2047, 0, 0);

	ldata->input_dev->name = "compass";

	err = input_register_device(ldata->input_dev);
	if (err) {
		dev_err(&client->dev, "Unable to register input device: %s\n",
			ldata->input_dev->name);
		goto exit_deregister_misc;
	}

	return 0;

exit_deregister_misc:
	misc_deregister(&lsm303dlh_a_misc_device);
exit_free_irq:
#ifdef A_USE_INT
#ifdef A_USE_GPIO_EXTENDER_INT
	ldata->pdata.free_irq(&client->dev);
#else
	free_irq(client->irq, ldata);
#endif /* A_USE_GPIO_EXTENDER_INT */
#endif /* A_USE_INT */

exit_kfree:
	kfree(ldata);
exit:
	dev_info(&client->dev, "%s: Driver Initialization failed\n", __FILE__);
	return err;
}

static int lsm303dlh_a_remove(struct i2c_client *client)
{
	struct lsm303dlh_a_data *ldata = i2c_get_clientdata(client);

	dev_info(&client->dev, "lsm303dlh_a driver removing\n");
#ifdef A_USE_INT
#ifdef A_USE_GPIO_EXTENDER_INT
	ldata->pdata.free_irq(&client->dev);
#else
	free_irq(client->irq, ldata);
#endif /* A_USE_GPIO_EXTENDER_INT */
#endif /* A_USE_INT */

	misc_deregister(&lsm303dlh_a_misc_device);

	input_unregister_device(ldata->input_dev);

	kfree(ldata);

	return 0;
}
#ifdef CONFIG_PM
static int lsm303dlh_a_suspend(struct i2c_client *client, pm_message_t state)
{
	int res;
	struct lsm303dlh_a_data *ldata = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "lsm303dlh_a_suspend\n");

	res = i2c_smbus_read_byte_data(ldata->client, CTRL_REG1);
	if (res < 0)
		return res;

	ldata->ctrl_1 = res & 0xFF;

	/* Put sensor to sleep mode. */
	res = i2c_smbus_write_byte_data(ldata->client, CTRL_REG1, 0);
	if (res)
		return res;

	ldata->mode = 0;

#if defined(A_USE_INT) && !defined(A_USE_GPIO_EXTENDER_INT)
	disable_irq(client->irq);
#endif

	return 0;
}

static int lsm303dlh_a_resume(struct i2c_client *client)
{
	int res;
	struct lsm303dlh_a_data *ldata = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "lsm303dlh_a_resume\n");

	/* Resume sensor state. */
	res = i2c_smbus_write_byte_data(ldata->client, CTRL_REG1,
		ldata->ctrl_1);
	if (res)
		return res;

	ldata->mode = ((ldata->ctrl_1 & LSM303DLH_A_CR1_PM_MASK) >>
		LSM303DLH_A_CR1_PM_BIT);

#if defined(A_USE_INT) && !defined(A_USE_GPIO_EXTENDER_INT)
	enable_irq(client->irq);
#endif

	return 0;
}
#endif

static const struct i2c_device_id lsm303dlh_a_id[] = {
	{ "lsm303dlh_a", 0 },
	{},
};

MODULE_DEVICE_TABLE(i2c, lsm303dlh_a_id);

static struct i2c_driver lsm303dlh_a_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = lsm303dlh_a_probe,
	.remove = __devexit_p(lsm303dlh_a_remove),
	.id_table = lsm303dlh_a_id,
#ifdef CONFIG_PM
	.suspend = lsm303dlh_a_suspend,
	.resume = lsm303dlh_a_resume,
#endif
	.driver = {
	.owner = THIS_MODULE,
	.name = "lsm303dlh_a",
	},
};

static int __init lsm303dlh_a_init(void)
{
	return i2c_add_driver(&lsm303dlh_a_driver);
}

static void __exit lsm303dlh_a_exit(void)
{
	printk(KERN_INFO "lsm303dlh_a exit\n");

	i2c_del_driver(&lsm303dlh_a_driver);
	return;
}

module_init(lsm303dlh_a_init);
module_exit(lsm303dlh_a_exit);

MODULE_DESCRIPTION("lsm303dlh accelerometer driver");
MODULE_AUTHOR("STMicroelectronics");
MODULE_LICENSE("GPL");

