/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name          : lsm303dlh_m.c
* Authors            : MH - C&I BU - Application Team
*		     : Carmine Iascone (carmine.iascone@st.com)
*		     : Matteo Dameno (matteo.dameno@st.com)
* Version            : V 0.3
* Date               : 24/02/2010
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
*******************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/uaccess.h>
#include <linux/lsm303dlh.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>

/* lsm303dlh magnetometer registers */
#define IRA_REG_M	0x0A

/* Magnetometer registers */
#define CRA_REG_M	0x00  /* Configuration register A */
#define CRB_REG_M	0x01  /* Configuration register B */
#define MR_REG_M	0x02  /* Mode register */
#define SR_REG_M        0x09  /* Status register */

/* Output register start address*/
#define OUT_X_M		0x03
#define OUT_Y_M		0x05
#define OUT_Z_M		0x07

/* Magnetometer X-Y gain  */
#define XY_GAIN_1_3	1055 /* XY gain at 1.3G */
#define XY_GAIN_1_9	 795 /* XY gain at 1.9G */
#define XY_GAIN_2_5	 635 /* XY gain at 2.5G */
#define XY_GAIN_4_0	 430 /* XY gain at 4.0G */
#define XY_GAIN_4_7	 375 /* XY gain at 4.7G */
#define XY_GAIN_5_6	 320 /* XY gain at 5.6G */
#define XY_GAIN_8_1	 230 /* XY gain at 8.1G */

/* Magnetometer Z gain  */
#define Z_GAIN_1_3	950 /* Z gain at 1.3G */
#define Z_GAIN_1_9	710 /* Z gain at 1.9G */
#define Z_GAIN_2_5	570 /* Z gain at 2.5G */
#define Z_GAIN_4_0	385 /* Z gain at 4.0G */
#define Z_GAIN_4_7	335 /* Z gain at 4.7G */
#define Z_GAIN_5_6	285 /* Z gain at 5.6G */
#define Z_GAIN_8_1	205 /* Z gain at 8.1G */

/* Control A regsiter. */
#define LSM303DLH_M_CRA_DO_BIT 2
#define LSM303DLH_M_CRA_DO_MASK (0x7 <<  LSM303DLH_M_CRA_DO_BIT)
#define LSM303DLH_M_CRA_MS_BIT 0
#define LSM303DLH_M_CRA_MS_MASK (0x3 <<  LSM303DLH_M_CRA_MS_BIT)

/* Control B regsiter. */
#define LSM303DLH_M_CRB_GN_BIT 5
#define LSM303DLH_M_CRB_GN_MASK (0x7 <<  LSM303DLH_M_CRB_GN_BIT)

/* Control Mode regsiter. */
#define LSM303DLH_M_MR_MD_BIT 0
#define LSM303DLH_M_MR_MD_MASK (0x3 <<  LSM303DLH_M_MR_MD_BIT)

/* Control Status regsiter. */
#define LSM303DLH_M_SR_RDY_BIT 0
#define LSM303DLH_M_SR_RDY_MASK (0x1 <<  LSM303DLH_M_SR_RDY_BIT)
#define LSM303DLH_M_SR_LOC_BIT 1
#define LSM303DLH_M_SR_LCO_MASK (0x1 <<  LSM303DLH_M_SR_LOC_BIT)
#define LSM303DLH_M_SR_REN_BIT 2
#define LSM303DLH_M_SR_REN_MASK (0x1 <<  LSM303DLH_M_SR_REN_BIT)

/*
 * LSM303DLH_M magnetometer local data
 */
struct lsm303dlh_m_data{
	struct i2c_client *client;
	struct lsm303dlh_platform_data pdata;
	short gain[3];
	short ms_delay;
	unsigned char mode;
};

static struct lsm303dlh_m_data *file_private;

/* set lsm303dlh magnetometer bandwidth */
int lsm303dlh_m_set_rate(struct lsm303dlh_m_data *ldata, unsigned char bw)
{
	unsigned char data = 0;
	int res;

	switch (bw) {
	case LSM303DLH_M_RATE_00_75:
		ldata->ms_delay = 1334;
		break;
	case LSM303DLH_M_RATE_01_50:
		ldata->ms_delay = 667;
		break;
	case LSM303DLH_M_RATE_03_00:
		ldata->ms_delay = 334;
		break;
	case LSM303DLH_M_RATE_07_50:
		ldata->ms_delay = 134;
		break;
	case LSM303DLH_M_RATE_15_00:
		ldata->ms_delay = 67;
		break;
	case LSM303DLH_M_RATE_30_00:
		ldata->ms_delay = 34;
		break;
	case LSM303DLH_M_RATE_75_00:
		ldata->ms_delay = 14;
		break;
	default:
		return -EINVAL;
	}

	data |= ((bw << LSM303DLH_M_CRA_DO_BIT) & LSM303DLH_M_CRA_DO_MASK);

	res = i2c_smbus_write_byte_data(ldata->client, CRA_REG_M, data);

	/* 100ms delay needed after setting mode. */
	msleep(100);

	return res;
}

/* read selected bandwidth from lsm303dlh_mag */
int lsm303dlh_m_get_bandwidth(struct lsm303dlh_m_data *ldata, unsigned char *bw)
{
	unsigned char data;

	data = i2c_smbus_read_byte_data(ldata->client, CRA_REG_M);
	data &= LSM303DLH_M_CRA_DO_MASK;
	data >>= LSM303DLH_M_CRA_DO_BIT;

	return data;
}

/*  i2c read routine for lsm303dlh magnetometer */
static int lsm303dlh_m_one_axis(struct lsm303dlh_m_data *ldata,
				unsigned char reg_addr,
				u8 negative,
				short *axis_data)
{
	s32 read_data;
	short data;

	/*  No global client pointer? */
	if (ldata->client == NULL)
		return -EINVAL;

	/* MSB is at lower address. */
	read_data = i2c_smbus_read_word_data(ldata->client, reg_addr);
	if (read_data < 0)
		return -EINVAL;

	data = ((read_data & 0xFF) << 8) | ((read_data >> 8) & 0xFF);

	*axis_data = negative ? -data : data;

	return 0;
}


/* X,Y and Z-axis magnetometer data readout
 * param *data pointer to \ref 6 bytes buffer for x,y,z data readout
 * note data will be read by multi-byte protocol into a 6 byte structure
 */
ssize_t lsm303dlh_m_read(struct file *filp, char __user *buf, size_t count,
	loff_t *f_pos)
{
	int res;
	short xyz_data[3];
	struct lsm303dlh_m_data *ldata = filp->private_data;

	if (count < sizeof(xyz_data))
		return -EINVAL;

	while (1) {
		res = i2c_smbus_read_byte_data(ldata->client, SR_REG_M);
		if (res < 0)
			return res;

		if (res & LSM303DLH_M_SR_RDY_MASK)
			break;

		/* Wait for sampling period. */
		if (msleep_interruptible(ldata->ms_delay))
			return -EINTR;
	}

	res = lsm303dlh_m_one_axis(ldata, OUT_X_M, ldata->pdata.negative_x,
		&xyz_data[ldata->pdata.axis_map_x]);
	if (res != 0)
		return -EINVAL;

	res = lsm303dlh_m_one_axis(ldata, OUT_Y_M, ldata->pdata.negative_y,
		&xyz_data[ldata->pdata.axis_map_y]);
	if (res != 0)
		return -EINVAL;

	res = lsm303dlh_m_one_axis(ldata, OUT_Z_M, ldata->pdata.negative_z,
		&xyz_data[ldata->pdata.axis_map_z]);
	if (res != 0)
		return -EINVAL;

	dev_dbg(&ldata->client->dev, "Hx= %d, Hy= %d, Hz= %d\n",
		xyz_data[ldata->pdata.axis_map_x],
		xyz_data[ldata->pdata.axis_map_y],
		xyz_data[ldata->pdata.axis_map_z]);

	if (copy_to_user(buf, xyz_data, sizeof(xyz_data)) != 0) {
		dev_err(&ldata->client->dev, "copy_to error\n");
		res = -EFAULT;
	}

	return sizeof(xyz_data);
}

int lsm303dlh_m_set_range(struct lsm303dlh_m_data *ldata, unsigned char range)
{
	short xy_gain;
	short z_gain;

	switch (range) {
	case LSM303DLH_M_RANGE_1_3G:
		xy_gain = XY_GAIN_1_3;
		z_gain = Z_GAIN_1_3;
		break;
	case LSM303DLH_M_RANGE_1_9G:
		xy_gain = XY_GAIN_1_9;
		z_gain = Z_GAIN_1_9;
		break;
	case LSM303DLH_M_RANGE_2_5G:
		xy_gain = XY_GAIN_2_5;
		z_gain = Z_GAIN_2_5;
		break;
	case LSM303DLH_M_RANGE_4_0G:
		xy_gain = XY_GAIN_4_0;
		z_gain = Z_GAIN_4_0;
		break;
	case LSM303DLH_M_RANGE_4_7G:
		xy_gain = XY_GAIN_4_7;
		z_gain = Z_GAIN_4_7;
		break;
	case LSM303DLH_M_RANGE_5_6G:
		xy_gain = XY_GAIN_5_6;
		z_gain = Z_GAIN_5_6;
		break;
	case LSM303DLH_M_RANGE_8_1G:
		xy_gain = XY_GAIN_8_1;
		z_gain = Z_GAIN_8_1;
		break;
	default:
		return -EINVAL;
	}

	ldata->gain[ldata->pdata.axis_map_x] = xy_gain;
	ldata->gain[ldata->pdata.axis_map_y] = xy_gain;
	ldata->gain[ldata->pdata.axis_map_z] = z_gain;

	range <<= LSM303DLH_M_CRB_GN_BIT;

	return i2c_smbus_write_byte_data(ldata->client, CRB_REG_M, range);

}

int lsm303dlh_m_set_mode(struct lsm303dlh_m_data *ldata, unsigned char mode)
{
	s32 res;
	mode = (mode << LSM303DLH_M_MR_MD_BIT) & LSM303DLH_M_MR_MD_MASK;

	res = i2c_smbus_write_byte_data(ldata->client, MR_REG_M, mode);
	if (res < 0)
		return res;

	ldata->mode = (mode >> LSM303DLH_M_MR_MD_BIT);
	return 0;
}

/*  open command for lsm303dlh_m device file  */
static int lsm303dlh_m_open(struct inode *inode, struct file *filp)
{
	struct lsm303dlh_m_data *ldata = file_private;

	filp->private_data = ldata;

	if (ldata->client == NULL) {
		printk(KERN_ERR"I2C driver not install\n");
		return -EINVAL;
	} else if (filp->f_flags & O_NONBLOCK) {
		dev_err(&ldata->client->dev,
			"Non Blocking operations are not supported\n");
		return -EAGAIN;
	}

	dev_dbg(&ldata->client->dev, "lsm303dlh_m has been opened\n");

	return 0;
}

/*  release command for lsm303dlh_m device file */
static int lsm303dlh_m_close(struct inode *inode, struct file *filp)
{
	struct lsm303dlh_m_data *ldata = filp->private_data;

	dev_dbg(&ldata->client->dev, "lsm303dlh_m has been closed\n");
	return 0;
}


/*  ioctl command for lsm303dlh_m device file */
static int lsm303dlh_m_ioctl(struct inode *inode, struct file *filp,
			       unsigned int cmd, unsigned long arg)
{
	int err = 0;
	unsigned char data[6];
	struct lsm303dlh_m_data *ldata = filp->private_data;

	/* check lsm303dlh_m_client */
	if (ldata->client == NULL) {
		printk(KERN_ERR"I2C driver not install\n");
		return -EFAULT;
	}

	/* cmd mapping */
	switch (cmd) {

	case LSM303DLH_M_SET_RANGE:
		if (copy_from_user(data, (unsigned char *)arg, 1) != 0) {
			dev_err(&ldata->client->dev, "copy_from_user error\n");
			return -EFAULT;
		}
		err = lsm303dlh_m_set_range(ldata, *data);
		return err;

	case LSM303DLH_M_SET_RATE:
		if (copy_from_user(data, (unsigned char *)arg, 1) != 0) {
			dev_err(&ldata->client->dev, "copy_from_user error\n");
			return -EFAULT;
		}
		err = lsm303dlh_m_set_rate(ldata, *data);
		return err;

	case LSM303DLH_M_SET_MODE:
		if (copy_from_user(data, (unsigned char *)arg, 1) != 0) {
			dev_err(&ldata->client->dev, "copy_from_user error\n");
			return -EFAULT;
		}
		err = lsm303dlh_m_set_mode(ldata, *data);
		return err;

	case LSM303DLH_M_GET_GAIN:
		if (copy_to_user((unsigned char *)arg,
			ldata->gain, sizeof(ldata->gain)) != 0) {
			dev_err(&ldata->client->dev, "copy_to_user error\n");
			return -EFAULT;
		}
		return 0;

	case LSM303DLH_M_GET_MODE:
		if (copy_to_user((unsigned char *)arg,
			&ldata->mode, sizeof(ldata->mode)) != 0) {
			dev_err(&ldata->client->dev, "copy_to_user error\n");
			return -EFAULT;
		}
		return 0;

	default:
		return -EINVAL;
	}
}

static const struct file_operations lsm303dlh_m_fops = {
	.owner = THIS_MODULE,
	.llseek	= no_llseek,
	.read = lsm303dlh_m_read,
	.open = lsm303dlh_m_open,
	.release = lsm303dlh_m_close,
	.ioctl = lsm303dlh_m_ioctl,
};

static struct miscdevice lsm303dlh_m_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lsm303dlh_m",
	.fops = &lsm303dlh_m_fops,
};

static int lsm303dlh_m_validate_pdata(struct lsm303dlh_m_data *mdata)
{
	if ((mdata->pdata.axis_map_x > 2) ||
	    (mdata->pdata.axis_map_y > 2) ||
	    (mdata->pdata.axis_map_z > 2) ||
	    (mdata->pdata.axis_map_x == mdata->pdata.axis_map_y) ||
	    (mdata->pdata.axis_map_x == mdata->pdata.axis_map_z) ||
	    (mdata->pdata.axis_map_y == mdata->pdata.axis_map_z)) {
		dev_err(&mdata->client->dev,
			"invalid axis_map value x:%u y:%u z%u\n",
			mdata->pdata.axis_map_x, mdata->pdata.axis_map_y,
			mdata->pdata.axis_map_z);
		return -EINVAL;
	}

	/* Only allow 0 and 1 */
	if ((mdata->pdata.negative_x > 1) ||
	    (mdata->pdata.negative_y > 1) ||
	    (mdata->pdata.negative_z > 1)) {
		dev_err(&mdata->client->dev,
			"invalid negate value x:%u y:%u z:%u\n",
			mdata->pdata.negative_x, mdata->pdata.negative_y,
			mdata->pdata.negative_z);
		return -EINVAL;
	}

	return 0;
}

int lsm303dlh_m_probe(struct i2c_client *client,
			const struct i2c_device_id *devid)
{
	int err = 0;
	int tempvalue;
	struct lsm303dlh_m_data *ldata;

	if (client->dev.platform_data == NULL) {
		dev_err(&client->dev, "platform data is NULL. exiting.\n");
		err = -ENODEV;
		goto exit;
	}

#if 0 /* TEMP */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK))
		goto exit;
#endif

	/*
	 * OK. For now, we presume we have a valid client. We now create the
	 * client structure, even though we cannot fill it completely yet.
	 */

	ldata = kzalloc(sizeof(*ldata), GFP_KERNEL);

	if (ldata == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	file_private = ldata;

	/* Initialize gain by 1 to avoid any divided by 0 errors */
	ldata->gain[0] = 1;
	ldata->gain[1] = 1;
	ldata->gain[2] = 1;

	ldata->mode = LSM303DLH_M_MODE_SLEEP;

	i2c_set_clientdata(client, ldata);
	ldata->client = client;

	memcpy(&ldata->pdata, client->dev.platform_data, sizeof(ldata->pdata));
	err = lsm303dlh_m_validate_pdata(ldata);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto exit_kfree;
	}

	err = i2c_smbus_read_byte(client);
	if (err < 0) {
		dev_err(&client->dev, "i2c_smbus_read_byte error!!\n");
		err = -ENODEV;
		goto exit_kfree;
	} else {
		dev_info(&client->dev, "LSM303DLH_M Device detected!\n");
	}

	/* read chip id */
	tempvalue = i2c_smbus_read_word_data(client, IRA_REG_M);
	if ((tempvalue & 0x00FF) == 0x0048) {
		dev_info(&client->dev, "I2C driver registered!\n");
	} else {
		ldata->client = NULL;
		err = -EINVAL;
		goto exit_kfree;
	}

	if (misc_register(&lsm303dlh_m_misc_device)) {
		dev_err(&client->dev, "misc_register failed\n");
		err = -EINVAL;
		goto error;
	}

	return 0;

error:
	dev_err(&client->dev, "%s: Driver Initialization failed\n", __FILE__);
exit_kfree:
	kfree(ldata);
exit:
	return err;
}

static int lsm303dlh_m_remove(struct i2c_client *client)
{
	struct lsm303dlh_m_data *data = i2c_get_clientdata(client);

	dev_info(&client->dev, "LSM303DLH_M driver removing\n");

	misc_deregister(&lsm303dlh_m_misc_device);

	kfree(data);

	return 0;
}

#ifdef CONFIG_PM
static int lsm303dlh_m_suspend(struct i2c_client *client, pm_message_t state)
{
	s32 reg_data;
	struct lsm303dlh_m_data *ldata = i2c_get_clientdata(client);

	dev_info(&client->dev, "lsm303dlh_m_suspend\n");

	reg_data = i2c_smbus_read_byte_data(ldata->client, CRA_REG_M);
	if (reg_data < 0)
		return reg_data;

	ldata->mode = (reg_data & 0xFF);

	lsm303dlh_m_set_mode(ldata, LSM303DLH_M_MODE_SLEEP);

	return 0;
}

static int lsm303dlh_m_resume(struct i2c_client *client)
{
	struct lsm303dlh_m_data *ldata = i2c_get_clientdata(client);

	dev_info(&client->dev, "lsm303dlh_m_resume\n");

	lsm303dlh_m_set_mode(ldata, ldata->mode);

	return 0;
}
#endif

static const struct i2c_device_id lsm303dlh_m_id[] = {
	{ "lsm303dlh_m", 0 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, lsm303dlh_m_id);

static struct i2c_driver lsm303dlh_m_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = lsm303dlh_m_probe,
	.remove = __devexit_p(lsm303dlh_m_remove),
	.id_table = lsm303dlh_m_id,
	#ifdef CONFIG_PM
	.suspend = lsm303dlh_m_suspend,
	.resume = lsm303dlh_m_resume,
	#endif
	.driver = {
		.owner = THIS_MODULE,
		.name = "lsm303dlh_m",
	},
};

static int __init lsm303dlh_m_init(void)
{
	/* Add i2c driver for lsm303dlh magnetometer */
	return i2c_add_driver(&lsm303dlh_m_driver);
}

static void __exit lsm303dlh_m_exit(void)
{
	i2c_del_driver(&lsm303dlh_m_driver);
	return;
}

module_init(lsm303dlh_m_init);
module_exit(lsm303dlh_m_exit);

MODULE_DESCRIPTION("lsm303dlh magnetometer driver");
MODULE_AUTHOR("STMicroelectronics");
MODULE_LICENSE("GPL");

