
/*
 * Copyright (C) 2009-2010 ST-Ericsson AB
 * License terms: GNU General Public License (GPL) version 2
 * Simple userspace interface for
 * Proximity Sensor Osram SFH 7741 and HAL switch Samsung HED54XXU11
 * Author: Jonas Aaberg <jonas.aberg@stericsson.com>
 *
 * This driver is only there for making Android happy. It is not ment
 * for mainline.
 */


#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/regulator/consumer.h>

#include <mach/sensors1p.h>
#ifndef CONFIG_REGULATOR
#include <mach/ab8500.h>
#endif

struct sensor {
	struct regulator *regulator;
	int pin;
	int startup_time;
	int active;
	u64 when_enabled;
};

struct sensors1p {
	struct sensor hal;
	struct sensor proximity;
};

#ifndef CONFIG_REGULATOR
static void force_power_on(void)
{
	/* Force VAUX1 to be active */
	int val;
	/* High power */
	val = ab8500_read(AB8500_REGU_CTRL2,
			  AB8500_REGU_VAUX12_REGU_REG);
	val |= 0x1;
	ab8500_write(AB8500_REGU_CTRL2,
		     AB8500_REGU_VAUX12_REGU_REG, val);

	/* 2.5V*/
	val = ab8500_read(AB8500_REGU_CTRL2,
			  AB8500_REGU_VAUX1_SEL_REG);
	val |= 0x8;
	ab8500_write(AB8500_REGU_CTRL2,
		     AB8500_REGU_VAUX1_SEL_REG, val);
}

#endif


static int sensors1p_power_write(struct device *dev,
				 struct sensor *s, const char *buf)
{
	int val;

	if (sscanf(buf, "%d", &val) != 1)
		return -EINVAL;

	if (val != 0 && val != 1)
		return -EINVAL;

	if (val != s->active) {
		if (val) {
			regulator_enable(s->regulator);
			s->when_enabled = get_jiffies_64() +
				msecs_to_jiffies(s->startup_time);
#ifndef CONFIG_REGULATOR
			force_power_on();
#endif
		} else
			regulator_disable(s->regulator);
	}
	s->active = val;

	return strnlen(buf, PAGE_SIZE);

}

static ssize_t sensors1p_sysfs_hal_active_set(struct device *dev,
					      struct device_attribute *attr,
					      const char *buf, size_t count)
{


	struct sensors1p *s = platform_get_drvdata(container_of(dev,
								struct platform_device,
								dev));
	return sensors1p_power_write(dev, &s->hal, buf);

}

static ssize_t sensors1p_sysfs_proximity_active_set(struct device *dev,
						    struct device_attribute *attr,
						    const char *buf,
						    size_t count)
{


	struct sensors1p *s = platform_get_drvdata(container_of(dev,
								struct platform_device,
								dev));
	return sensors1p_power_write(dev, &s->proximity, buf);

}


static ssize_t sensors1p_sysfs_hal_active_get(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	struct sensors1p *s = platform_get_drvdata(container_of(dev,
								struct platform_device,
								dev));
	return sprintf(buf, "%d", s->hal.active);
}

static ssize_t sensors1p_sysfs_proximity_active_get(struct device *dev,
						    struct device_attribute *attr,
						    char *buf)
{
	struct sensors1p *s = platform_get_drvdata(container_of(dev,
								struct platform_device,
								dev));
	return sprintf(buf, "%d", s->proximity.active);
}



static int sensors1p_read(struct device *dev, struct sensor *s, char *buf)
{
	int ret;

	if (!s->active)
		return -EINVAL;

	/* Only wait if read() is called before the sensor is up and running
	 * Since jiffies wraps, always sleep maximum time.
	 */
	if (time_before64(get_jiffies_64(), s->when_enabled))
		mdelay(s->startup_time);

	/* For some odd reason, setting direction in the probe function fails */
	ret = gpio_direction_input(s->pin);

	if (ret)
		dev_err(dev, "Failed to set GPIO pin %d to input.\n", s->pin);
	else
		ret = gpio_get_value(s->pin);

	return sprintf(buf, "%d", ret);
}

static ssize_t sensors1p_sysfs_hal_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct sensors1p *s = platform_get_drvdata(container_of(dev,
								struct platform_device,
								dev));
	return sensors1p_read(dev, &s->hal, buf);
}

static ssize_t sensors1p_sysfs_proximity_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	struct sensors1p *s = platform_get_drvdata(container_of(dev,
								struct platform_device,
								dev));
	return sensors1p_read(dev, &s->proximity, buf);
}


static DEVICE_ATTR(proximity_activate, 0666,
		   sensors1p_sysfs_proximity_active_get,
		   sensors1p_sysfs_proximity_active_set);
static DEVICE_ATTR(hal_activate, 0666,
		   sensors1p_sysfs_hal_active_get,
		   sensors1p_sysfs_hal_active_set);
static DEVICE_ATTR(proximity, 0444, sensors1p_sysfs_proximity_show, NULL);
static DEVICE_ATTR(hal, 0444, sensors1p_sysfs_hal_show, NULL);

static struct attribute *sensors1p_attrs[] = {
	&dev_attr_proximity_activate.attr,
	&dev_attr_hal_activate.attr,
	&dev_attr_proximity.attr,
	&dev_attr_hal.attr,
	NULL,
};

static struct attribute_group sensors1p_attr_group = {
	.name = NULL,
	.attrs = sensors1p_attrs,
};

static int __init sensors1p_probe(struct platform_device *pdev)
{
	int err = -EINVAL;
	struct sensors1p_config *c;
	struct sensors1p *s = NULL;

	if (!pdev)
		goto out;

	c = pdev->dev.platform_data;

	if (c == NULL) {
		dev_err(&pdev->dev, "Error: Missconfigured.\n");
		goto out;
	}

	s = kzalloc(sizeof(struct sensors1p), GFP_KERNEL);

	if (s == NULL) {
		dev_err(&pdev->dev,
			"Could not allocate struct memory!\n");
		err = -ENOMEM;
		goto out;
	}

	s->hal.pin = c->hal.pin;
	s->proximity.pin = c->proximity.pin;

	s->hal.startup_time = s->hal.startup_time;
	s->proximity.startup_time = s->proximity.startup_time;


	s->hal.regulator = regulator_get(&pdev->dev, c->hal.regulator);

	if (s->hal.regulator == NULL) {
		dev_err(&pdev->dev, "regulator_get(\"%s\") failed.\n",
			c->hal.regulator);
		goto out;
	}
	s->proximity.regulator = regulator_get(&pdev->dev,
					       c->proximity.regulator);

	if (s->proximity.regulator == NULL) {
		dev_err(&pdev->dev, "regulator_get(\"%s\") failed.\n",
			c->proximity.regulator);
		goto out;
	}

#ifndef CONFIG_REGULATOR
	force_power_on();
#endif

	err = sysfs_create_group(&pdev->dev.kobj, &sensors1p_attr_group);

	if (err) {
		dev_err(&pdev->dev, "Failed to create sysfs entries.\n");
		goto out;
	}

	platform_set_drvdata(pdev, s);

	return 0;
out:
	if (s) {
		if (s->proximity.regulator)
			regulator_put(s->proximity.regulator);

		if (s->hal.regulator)
			regulator_put(s->hal.regulator);
	}
	kfree(s);
	return err;
}

static int __exit sensors1p_remove(struct platform_device *pdev)
{
	struct sensors1p *s = platform_get_drvdata(pdev);

	sysfs_remove_group(&pdev->dev.kobj, &sensors1p_attr_group);
	regulator_put(s->hal.regulator);
	regulator_put(s->proximity.regulator);
	kfree(s);
	return 0;
}

static struct platform_driver sensors1p_driver = {
	.probe = sensors1p_probe,
	.remove = __exit_p(sensors1p_remove),
	.driver = {
		.name = "sensors1p",
		.owner = THIS_MODULE,
	},
};

static int __init sensors1p_init(void)
{
	return platform_driver_register(&sensors1p_driver);
}

static void __exit sensors1p_exit(void)
{
	platform_driver_unregister(&sensors1p_driver);
}

late_initcall(sensors1p_init);
module_exit(sensors1p_exit);

MODULE_AUTHOR("Jonas Aaberg <jonas.aberg@stericsson.com>");
MODULE_DESCRIPTION("One pin gpio sensors driver (Proximity+HAL)");
MODULE_LICENSE("GPLv2");
