/*
 * Copyright (C) 2010 ST-Ericsson
 * Copyright (C) 2009 STMicroelectronics
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/device.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <mach/hardware.h>

static struct gpio_altfun_data *altfun_table;
static int altfun_table_size;
static DEFINE_MUTEX(altfun_lock);

/**
 * stm_gpio_set_altfunctable - set the alternate function table
 * @table: pointer to the altfunction table
 * @size: number of elements in the table
 */
int stm_gpio_set_altfunctable(struct gpio_altfun_data *table, int size)
{
	int ret = 0;

	mutex_lock(&altfun_lock);

	if (altfun_table) {
		ret = -EBUSY;
		goto out;
	}

	altfun_table = table;
	altfun_table_size = size;

out:
	mutex_unlock(&altfun_lock);
	return ret;
}

/**
 * stm_gpio_altfuncenable - enables alternate functions for
 * 			    the given set of GPIOs
 * @alt_func: GPIO altfunction to disable
 *
 * Enable the given set of alternate functions.  Error will be returned if any
 * of the pins or invalid or busy.
 */
int stm_gpio_altfuncenable(gpio_alt_function alt_func)
{
	int i, j, found = false;
	int ret = -EINVAL;
	int start, end;

	mutex_lock(&altfun_lock);
	for (i = 0; i < altfun_table_size; i++) {
		if (altfun_table[i].altfun != alt_func)
			continue;

		start = altfun_table[i].start;
		end = altfun_table[i].end;
		if (start > end) {
			j = start;
			start = end;
			end = j;
		}

		found = true;

		if (!gpio_is_valid(start) || !gpio_is_valid(end)) {
			printk(KERN_ERR
			       "%s: GPIO range %d-%d for %s is not valid for\n",
			       __func__, start, end, altfun_table[i].dev_name);
			goto out;
		}

		for (j = start; j <= end; j++) {
			if (gpio_request(j, altfun_table[i].dev_name) < 0) {
				printk(KERN_ERR
				       "%s:Failed to set %s, GPIO %d is busy\n",
				       __func__, altfun_table[i].dev_name, j);
				ret = -EBUSY;
				goto out;
			}

			nmk_gpio_set_mode(j, altfun_table[i].type);
		}
	}

	if (found)
		ret = 0;

out:
	mutex_unlock(&altfun_lock);
	return ret;
}
EXPORT_SYMBOL(stm_gpio_altfuncenable);

/**
 * stm_gpio_altfuncdisable - disable the alternate functions for
 * 			     the given set of GPIOs.
 * @alt_func: GPIO altfunction enum
 *
 * Disable the given set of alternate functions.  Error will be returned if any
 * of the pins or invalid.
 **/
int stm_gpio_altfuncdisable(gpio_alt_function alt_func)
{
	bool found = false;
	int ret = -EINVAL;
	int i, j;
	int start, end;

	mutex_lock(&altfun_lock);
	for (i = 0; i < altfun_table_size; i++) {
		if (altfun_table[i].altfun != alt_func)
			continue;
		start = altfun_table[i].start;
		end = altfun_table[i].end;
		if (start > end) {
			j = start;
			start = end;
			end = j;
		}

		found = true;

		if (!gpio_is_valid(start) || !gpio_is_valid(end)) {
			printk(KERN_ERR
			       "%s: GPIO range %d-%d for %s is out of bound\n",
			       __func__, start, end, altfun_table[i].dev_name);
			goto out;
		}

		for (j = start; j <= end; j++) {
			/* Reset the pin - input, gpio */
			gpio_direction_input(j);
			nmk_gpio_set_mode(j, NMK_GPIO_ALT_GPIO);
			gpio_free(j);
		}
	}

	if (!found)
		printk(KERN_ERR
		       "%s: Didn't find any GPIO alt interface named '%s'\n",
		       __func__, altfun_table[i].dev_name);

	if (found)
		ret = 0;

out:
	mutex_unlock(&altfun_lock);
	return ret;
}
EXPORT_SYMBOL(stm_gpio_altfuncdisable);
