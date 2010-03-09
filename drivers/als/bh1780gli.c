/*
 *  bh1780gli.c - Linux kernel modules for ambient light sensor
 *
 * Copyright (C) ST-Ericsson 2010
 * License terms: GNU General Public License (GPL) version 2
 * Author: Stefan Nilsson <stefan.xk.nilsson@stericsson.com>
 * Author: Martin Persson <martin.persson@stericsson.com>
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/als_sys.h>

#define BH1780GLI_DRV_NAME	"bh1780gli"

/* I2C Registers */
#define BH1780GLI_REG_COMMAND   0x80
#define BH1780GLI_REG_CONTROL   0x0
#define BH1780GLI_REG_PARTID    0xA
#define BH1780GLI_REG_MANID     0xB
#define BH1780GLI_REG_DATALOW   0xC
#define BH1780GLI_REG_DATAHIGH  0xD

/* I2C commands */
#define BH1780GLI_CMD_POWER_UP   0x3
#define BH1780GLI_CMD_POWER_DOWN 0x0

/* Register values */
#define BH1780GLI_MANID  0x1
#define BH1780GLI_PARTID 0x81

#define BH1780GLI_ACC_TIME 150

struct bh1780gli_data {
	struct device *classdev;
	struct i2c_client *client;
	struct mutex lock;
};

static int bh1780gli_write_data(struct i2c_client *client, int reg,
				int data)
{
	int ret = i2c_smbus_write_byte_data(client, reg, data);
	if (ret < 0) {
		dev_err(&client->dev,
			 "i2c_smbus_write_byte_data failed! Args: Chip = 0x%.8X, Reg = 0x%X, Data = 0x%X\n",
			 (unsigned int) client, reg, data);
	}
	return ret;
}

static int bh1780gli_read_data(struct i2c_client *client, unsigned char reg)
{
	int ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0) {
		dev_err(&client->dev,
			 "i2c_smbus_read_byte_data failed! Args: Chip = 0x%.8X, Reg = 0x%X\n",
			 (unsigned int) client, reg);
	}
	return ret;
}

static ssize_t bh1780gli_show_illuminance(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int data_low, data_high, val;
	ssize_t ret;

	struct i2c_client *client = to_i2c_client(dev->parent);
	struct bh1780gli_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->lock);

	/* Power up device */
	val = bh1780gli_write_data(client, BH1780GLI_REG_COMMAND,
				BH1780GLI_CMD_POWER_UP);
	if (val < 0)
		goto error_exit;

	/* Wait for data accumulation */
	msleep(BH1780GLI_ACC_TIME);

	data_low = bh1780gli_read_data(client, BH1780GLI_REG_COMMAND |
				BH1780GLI_REG_DATALOW);
	if (data_low < 0) {
		val = data_low;
		goto exit;
	}

	data_high = bh1780gli_read_data(client,	BH1780GLI_REG_COMMAND |
					BH1780GLI_REG_DATAHIGH);
	if (data_high < 0) {
		val = data_high;
		goto exit;
	}

	/* Calculate lux value */
	val = (data_high << 8) | data_low;

exit:
	/* Power down device */
	(void) bh1780gli_write_data(client, BH1780GLI_REG_COMMAND,
				BH1780GLI_CMD_POWER_DOWN);
error_exit:
	ret = snprintf(buf, 8, "%u\n", val);
	mutex_unlock(&data->lock);

	return ret;
}

static DEVICE_ATTR(illuminance, S_IRUGO, bh1780gli_show_illuminance, NULL);

static struct attribute *bh1780gli_attributes[] = {
	&dev_attr_illuminance.attr,
	NULL
};

static const struct attribute_group bh1780gli_attr_group = {
	.attrs = bh1780gli_attributes,
};

static int bh1780gli_init_client(struct i2c_client *client)
{
	int ret;

	/* Power up device */
	ret = bh1780gli_write_data(client, BH1780GLI_REG_COMMAND,
				BH1780GLI_CMD_POWER_UP);
	if (ret < 0)
		goto error_exit;

	/* Check that the manufacturer id matches the expected */
	ret = bh1780gli_read_data(client,
				BH1780GLI_REG_COMMAND |
				BH1780GLI_REG_MANID);
	if (ret < 0)
		goto exit;

	/* Check that the part number matches the expected */
	ret = bh1780gli_read_data(client,
				BH1780GLI_REG_COMMAND |
				BH1780GLI_REG_PARTID);
	if (ret != BH1780GLI_PARTID) {
		dev_err(&client->dev,
			"Incorrect part number: 0x%X != 0x%x\n", ret,
			BH1780GLI_PARTID);
		ret = -ENODEV;
	}

exit:
	/* Power down device */
	(void) bh1780gli_write_data(client, BH1780GLI_REG_COMMAND,
				BH1780GLI_CMD_POWER_DOWN);
error_exit:
	return ret;
}

static int __devinit bh1780gli_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct bh1780gli_data *data;
	int err;

	data = kzalloc(sizeof(struct bh1780gli_data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev, "kzalloc failed!\n");
		err = -ENOMEM;
		goto exit;
	}
	data->client = client;
	i2c_set_clientdata(client, data);

	mutex_init(&data->lock);

	/* Initialize the BH1780GLI chip */
	err = bh1780gli_init_client(client);
	if (err < 0) {
		dev_err(&client->dev, "init_client failed!\n");
		goto exit;
	}

	data->classdev = als_device_register(&client->dev);
	if (IS_ERR(data->classdev)) {
		dev_err(&client->dev, "als_device_register failed!\n");
		err = PTR_ERR(data->classdev);
		goto exit;
	}

	err = sysfs_create_group(&data->classdev->kobj, &bh1780gli_attr_group);
	if (err) {
		dev_err(&client->dev, "sysfs_create_group failed!\n");
		goto exit_unreg;
	}

	return 0;

exit_unreg:
	als_device_unregister(data->classdev);
exit:
	kfree(data);
	return err;
}

static int __devexit bh1780gli_remove(struct i2c_client *client)
{
	struct bh1780gli_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&data->classdev->kobj, &bh1780gli_attr_group);
	als_device_unregister(data->classdev);
	kfree(i2c_get_clientdata(client));

	return 0;
}

static const struct i2c_device_id bh1780gli_id[] = {
	{"bh1780gli", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, bh1780gli_id);

static struct i2c_driver bh1780gli_driver = {
	.driver = {
		.name = BH1780GLI_DRV_NAME,
		.owner = THIS_MODULE,
	},
	.probe = bh1780gli_probe,
	.remove = __devexit_p(bh1780gli_remove),
	.id_table = bh1780gli_id,
};

static int __init bh1780gli_init(void)
{
	return i2c_add_driver(&bh1780gli_driver);
}

static void __exit bh1780gli_exit(void)
{
	i2c_del_driver(&bh1780gli_driver);
}

MODULE_DESCRIPTION("BH1780GLI ambient light sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

late_initcall(bh1780gli_init);
module_exit(bh1780gli_exit);
