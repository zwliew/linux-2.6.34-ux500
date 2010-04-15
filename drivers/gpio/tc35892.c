/*
 * Overview:
 *  	Driver for tc35892 gpio port expander
 *
 * Copyright (C) 2009 ST-Ericsson SA
 *
 * Author: Hanumath Prasad <hanumath.prasad@stericsson.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <mach/tc35892.h>

static void tc35892_work(struct work_struct *_chip);
/* struct tc35892_chip - tc35892 gpio chip strcture
 * @gpio_chip:	pointer to the gpio_chip library
 * @client:	pointer to the i2c_client structure
 * @lock:	internal sync primitive
 * @gpio_start:	gpio start nunbmer for this chip
 * @handlers:	array of handlers for this chip
 * @data:	callback data
 **/
struct tc35892_chip {
	struct gpio_chip gpio_chip;
	struct i2c_client *client;
	struct mutex lock;
	unsigned gpio_start;
	struct work_struct work;
	void (*handlers[MAX_INT_EXP]) (void *);
	void *data[MAX_INT_EXP];
};

static struct tc35892_chip *the_tc35892;

/**
 * tc35892_write_byte() - Write a single byte
 * @chip:	gpio lib chip strcture
 * @reg:	register offset
 * @data:	data byte to be written
 *
 * This funtion uses smbus byte write API to write a single byte to tc35892
 **/
static int tc35892_write_byte(struct tc35892_chip *chip,
				unsigned char reg, unsigned char data)
{
	int ret;
	ret = i2c_smbus_write_byte_data(chip->client, reg, data);
	if (ret < 0) {
		printk(KERN_ERR "i2c smbus write byte failed\n");
		return ret;
	}
	return 0;
}

/**
 * tc35892_read_byte() - Read a single byte
 * @chip:	gpio lib chip strcture
 * @reg:	register offset
 * @val:	data read
 *
 * This funtion uses smbus byte read API to read a byte from the given offset.
 **/
static int tc35892_read_byte(struct tc35892_chip *chip, unsigned char reg,
			unsigned char *val)
{
	int ret;
	ret = i2c_smbus_read_byte_data(chip->client, reg);
	if (ret < 0) {
		printk(KERN_ERR "error in reading a byte, smbus byte read failed\n");
		return ret;
	}
	*val = ret;
	return 0;
}
/**
 * tc35892_read() - read data from tc35892
 * @chip:	gpio lib chip strcture
 * @reg:        register offset
 * @buf:	read the data in this buffer
 * @nbytes:	number of bytes to read
 *
 * This funtion uses smbus read block API to read multiple bytes from the reg offset.
 **/
static int tc35892_read(struct tc35892_chip *chip, unsigned char reg, unsigned char *buf, unsigned char nbytes)
{
	int ret;
	ret = i2c_smbus_read_i2c_block_data(chip->client, reg, nbytes, buf);
	if (ret < nbytes) {
		printk(KERN_ERR "error in reading a byte, smbus byte read failed\n");
		return ret;
	}
	return 0;
}
/**
 * tc35892_write() - Writes to the tc35892 chip
 * @chip:	gpio lib chip strcture
 * @buf:	buffer to be written
 * @nbytes:	nunmber of bytes to be written
 *
 * This funtion uses smbus block write API's to write n number of bytes to the tc35892
 **/
static t_tc35892_error tc35892_write(struct tc35892_chip *chip,
		unsigned char *buf, unsigned char nbytes)
{
	int ret;
	/* Need to check the return value*/
	ret = i2c_smbus_write_i2c_block_data(chip->client, buf[0], nbytes-1, &buf[1]);
	if (ret < 0) {
		printk(KERN_ERR"i2c smbus write block data failed\n");
		return ret;
	}
	return 0;
}
/**
 * calculate_gpio_offset() - calculate the offset
 * @off:	input offset number
 *
 * There are some registers in TC35892 with 24 bits, each corresponds to
 * to one GPIO. Based on the input gpio number, this funtion just returns
 * the offset number which can be used for the register base address access
 **/
unsigned char calculate_gpio_offset(unsigned off)
{
	if (off < 8)
		return 0;
	else if (off > 7 && off < 16)
		return 0x1;
	else
		return 0x2;
}

/**
 * tc35892_gpio_get_value() - Read a GPIO value from the given offset
 * @gc:		pointer to the gpio_chip strcture
 * @off:	The offset to read from
 *
 * This funtion is called from the gpio library to read a GPIO
 * status. This funtion reads from GPIO DATA Register to find
 * out the status
 **/
static int tc35892_gpio_get_value(struct gpio_chip *gc, unsigned off)
{
	struct tc35892_chip *chip;
	int ret;
	unsigned char reg_val, offset, mask;

	chip = container_of(gc, struct tc35892_chip, gpio_chip);
	if (off >= MAX_TC35892_GPIO)
		/*number of pin exceeded*/
		return TC35892_BAD_PARAMETER;
	mutex_lock(&chip->lock);
	if (off < 8) {
		offset = GPIO_DATA0_Index;
		mask = 1 << off;
	} else if (off < 16) {
		offset = GPIO_DATA1_Index;
		mask = 1 << (off-8);
	} else {
		offset = GPIO_DATA2_Index;
		mask = 1 << (off-16);
	}
	mutex_unlock(&chip->lock);
	ret = tc35892_read_byte(chip,  offset, &reg_val);
	if (ret < 0) {
		printk(KERN_ERR "Error in tc35892_gpio_get_vaule\n");
		return ret;
	}
	return (reg_val & mask) ? 1 : 0;
}
/**
 * tc35892_gpio_set_value() - Set a GPIO value from the given offset
 * @gc:		pointer to the gpio_chip strcture
 * @off:	The write gpio offset
 * @val:	value need to be written
 *
 * This funtion is called from the gpio library to set/unset a GPIO
 * value. This funtion sets the GPIO DATA register to set a bit.
 **/
static void tc35892_gpio_set_value(struct gpio_chip *gc,
				     unsigned off, int val)
{
	struct tc35892_chip *chip;
	unsigned char temp_buf[3], offset, mask;
	chip = container_of(gc, struct tc35892_chip, gpio_chip);

	mutex_lock(&chip->lock);
	offset = calculate_gpio_offset(off);
	mask = 1 << (off % 8);
	/*register selection depending on the pin number*/
	if ((off/8) == 0) {
		temp_buf[0] = GPIO_DATA0_Index;
		if (val == 0) {
			temp_buf[1] = ~(1 << off);
			temp_buf[2] = (1 << off);
		} else if (val == 1) {
			temp_buf[1] = 1 << off;
			temp_buf[2] = 1 << off;
		}
	} else if ((off/8) == 1) {
		temp_buf[0] = GPIO_DATA1_Index;
		if (val == 0) {
			temp_buf[1] = ~(1 << (off-8));
			temp_buf[2] = (1 << (off-8));
		} else if (val == 1) {
			temp_buf[1] = 1 << (off-8);
			temp_buf[2] = 1 << (off-8);
		}
	} else {
		temp_buf[0] = GPIO_DATA2_Index;
		if (val == 0) {
			temp_buf[1] = ~(1 << (off-16));
			temp_buf[2] = 1 << (off-16);
		} else if (val == 1) {
			temp_buf[1] = 1 << (off-16);
			temp_buf[2] = 1 << (off-16);
		}
	}
	tc35892_write(chip, temp_buf, 3);
	mutex_unlock(&chip->lock);
}

/**
 * tc35892_gpio_direction_output() - Set a GPIO direction as output
 * @gc:		pointer to the gpio_chip strcture
 * @off:	The gpio offset
 * @val:	value to be written
 *
 * This funtion is called from the gpio library to set a GPIO
 * direction as output.
 **/
static int tc35892_gpio_direction_output(struct gpio_chip *gc,
					   unsigned off, int val)
{
	struct tc35892_chip *chip;
	unsigned char reg_val, offset;
	unsigned char mask, temp;

	chip = container_of(gc, struct tc35892_chip, gpio_chip);
	if (off >= MAX_TC35892_GPIO)
		/*number of pin exceeded*/
		return TC35892_BAD_PARAMETER;
	offset = calculate_gpio_offset(off);
	mask = 1 << (off % 8);
	reg_val = tc35892_read_byte(chip, (GPIO_DIR0_Index + offset), &temp);
	if (reg_val != TC35892_OK)
		return TC35892_I2C_ERROR;
	temp |= mask;
	reg_val = tc35892_write_byte(chip, (GPIO_DIR0_Index + offset), temp);
	if (reg_val != TC35892_OK)
		return TC35892_I2C_ERROR;
	return 0;
}
/**
 * tc35892_gpio_direction_input() - Set a GPIO direction as input
 * @gc:		pointer to the gpio_chip strcture
 * @off:	The gpio offset
 *
 * This funtion is called from the gpio library to set a GPIO
 * direction as input.
 **/
static int tc35892_gpio_direction_input(struct gpio_chip *gc, unsigned off)
{
	struct tc35892_chip *chip;
	unsigned char reg_val, temp, offset, mask;
	if (off >= MAX_TC35892_GPIO)
		/*number of pin exceeded*/
		return TC35892_BAD_PARAMETER;
	chip = container_of(gc, struct tc35892_chip, gpio_chip);
	offset = calculate_gpio_offset(off);
	mask = 1 << (off % 8);
	reg_val = tc35892_read_byte(chip, (GPIO_DIR0_Index + offset), &temp);
	if (reg_val != TC35892_OK)
		return TC35892_I2C_ERROR;
	temp &= ~mask;
	reg_val = tc35892_write_byte(chip, (GPIO_DIR0_Index + offset), temp);
	if (reg_val != TC35892_OK)
		return TC35892_I2C_ERROR;
	return 0;
}
/**
 * tc35892_setup_gpio() - Set up the gpio lib structure
 * @chip:	pointer to the tc35892_chip strcture
 * @gpios:	Number of GPIO's
 *
 * This funtion set up the  gpio_chip structure to the corresponding
 * callback funtions and other configurations.
 **/
static void tc35892_setup_gpio(struct tc35892_chip *chip, int gpios)
{
	struct gpio_chip *gc;

	gc = &chip->gpio_chip;

	gc->direction_input = tc35892_gpio_direction_input;
	gc->direction_output = tc35892_gpio_direction_output;
	gc->get = tc35892_gpio_get_value;
	gc->set = tc35892_gpio_set_value;

	gc->base = chip->gpio_start;
	gc->ngpio = gpios;
	gc->label = chip->client->name;
	gc->dev = &chip->client->dev;
	gc->owner = THIS_MODULE;
}

/**
 * tc35892_bit_calc() -  gpio bit calculation
 *
 * @pin_index:	pin to be set (0-23)
 * @pin_value:	value to be set (0-1)
 * @reg_byte:	in - current register value
 *
 * This function execute common gpio bit calculation
 * Parameter
 **/
static t_tc35892_error tc35892_bit_calc(unsigned char pin_index, unsigned char pin_value, unsigned char *reg_byte)
{
	unsigned char mask;
	mask = 1 << (pin_index % 8);
	if (pin_value == 0) {
		*reg_byte  &= ~mask;
	} else if (pin_value == 1) {
		*reg_byte |= mask;
	} else {
		return TC35892_BAD_PARAMETER;
	}
	return TC35892_OK;
}
/**
 * tc35892_set_gpio_intr_conf()- configure tc35892 gpio interrupt configuration
 * @pin_index:	pin to be read (0-23)
 * @edge_level_sensitive: interrupt is either edge sensitive(EDGE_SENSITIVE) or level sensitive(LEVEl_SENSITIVE)
 * @edge_level_type: intr can be detected on FALLING/RISING edge or LOW/HIGH level or on BOTH EDGES
 *
 * This function configure tc35892 gpio intr_type for edge/level detection.
 * TC35892_FALLING_EDGE_OR_LOWLEVEL  fallingedge/lowlevel detection
 * TC35892_RISING_EDGE_OR_HIGHLEVEl  risingedge/highlevel detection
 * TC35892_BOTH_EDGE  falling and rising edge detection
 * This function will set the interrupt sensitivity as edge or level sensitive. Depending on that
 * it will configure the interrupt detection either on fallingedge/lowlevel or risingedge/highlevel.
 **/
t_tc35892_error tc35892_set_gpio_intr_conf (int pin_index, unsigned char edge_level_sensitive, unsigned char edge_level_type)
{
	t_tc35892_error retval = TC35892_OK;
	unsigned char offset, tempbyte, off;
	unsigned char val_intr = 0, val_sense = 0;
	unsigned char val_both_edge = 0;

	pin_index = pin_index - EGPIO_PIN_0;
	if (pin_index < 0 || pin_index >= MAX_TC35892_GPIO)
		/*number of pin exceeded*/
		return TC35892_BAD_PARAMETER;
	switch (edge_level_sensitive) {
	case EDGE_SENSITIVE:
		val_sense = 0;
	break;
	case LEVEl_SENSITIVE:
		val_sense = 1;
	break;
	default:
		retval = TC35892_BAD_PARAMETER;
	break;
	}
	switch (edge_level_type) {
	case TC35892_FALLING_EDGE_OR_LOWLEVEL:
		val_intr = 0;
		break;
	case TC35892_RISING_EDGE_OR_HIGHLEVEL:
		val_intr = 1;
	break;
	case TC35892_BOTH_EDGE:
		val_both_edge = 1;
	break;
	default:
		retval = TC35892_BAD_PARAMETER;
	break;
	}
	off = calculate_gpio_offset(pin_index);
	/*configuring the sync register to synchronize the interrupt signal*/
	if (retval == TC35892_OK) {
		offset = GPIO_SYNC0_Index + off;
		retval = tc35892_read_byte(the_tc35892, offset, &tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
		retval = tc35892_bit_calc(pin_index, 1, &tempbyte);
		retval = tc35892_write_byte(the_tc35892, offset, tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
	}
	/*configuring the GPIO interrupt on both edges*/
	if (retval == TC35892_OK && val_both_edge) {
		offset = GPIO_IBE0_Index + off;
		retval = tc35892_read_byte(the_tc35892, offset, &tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
		retval = tc35892_bit_calc(pin_index, val_both_edge, &tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
		retval = tc35892_write_byte(the_tc35892, offset, tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
	}
	/*configure the GPIO on rising edge or falling edge*/
	if (retval == TC35892_OK && !val_both_edge) {
		offset = GPIO_IEV0_Index + off;
		retval = tc35892_read_byte(the_tc35892, offset, &tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
		retval = tc35892_bit_calc(pin_index, val_intr, &tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
		retval = tc35892_write_byte(the_tc35892, offset, tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
	}
	/*configure the interrupt on edgesensitive or levelsensitive*/
	if (retval == TC35892_OK) {
		offset = GPIO_IS0_Index + off;
		retval = tc35892_read_byte(the_tc35892, offset, &tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
		retval = tc35892_bit_calc(pin_index, val_sense, &tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
		retval = tc35892_write_byte(the_tc35892, offset, tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
	}
	/*clearing the interrupt*/
	if (retval == TC35892_OK) {
		offset = GPIO_IC0_Index + off;
		retval = tc35892_read_byte(the_tc35892, offset, &tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
		retval = tc35892_bit_calc(pin_index, 1, &tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
		retval = tc35892_write_byte(the_tc35892, offset, tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
	}
	return retval;
}
EXPORT_SYMBOL(tc35892_set_gpio_intr_conf);

/**
 * tc35892_set_intr_enable() - enable or disable interrupt
 * @pin_index:	gpio number
 * @intr_enable_disable:	interrupt enable or disable
 *
 * This funtion will enable(ENABLE_INTERRUPT) or disable(DISABLE_INTERRUPT) the interrupt
 **/
t_tc35892_error tc35892_set_intr_enable(int pin_index, unsigned char intr_enable_disable)
{
	t_tc35892_error retval = TC35892_OK;
	unsigned char offset, tempbyte;
	unsigned char val_enable, off;

	pin_index = pin_index - EGPIO_PIN_0;
	if (pin_index < 0 || pin_index >= MAX_TC35892_GPIO)
		/*number of pin exceeded*/
		return TC35892_BAD_PARAMETER;
	switch (intr_enable_disable) {
	case ENABLE_INTERRUPT:
		val_enable = 1;
	break;
	case DISABLE_INTERRUPT:
		val_enable = 0;
	break;
	default:
		retval = TC35892_BAD_PARAMETER;
	break;
	}
	off = calculate_gpio_offset(pin_index);
	/* Enabling the interrupt */
	if (retval == TC35892_OK) {
		offset = GPIO_IE0_Index + off;
		retval = tc35892_read_byte(the_tc35892, offset, &tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
		retval = tc35892_bit_calc(pin_index, val_enable, &tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
		retval = tc35892_write_byte(the_tc35892, offset, tempbyte);
		if (retval != TC35892_OK)
			return TC35892_I2C_ERROR;
	}
	return retval;
}
EXPORT_SYMBOL(tc35892_set_intr_enable);
/**
 * tc35892_set_callback() - install a callback handler
 * @irq:	gpio number
 * @handler:	funtion pointer to the callback handler
 * @data:	Arguement to the callback
 *
 * This funtion install the callback handler for the client device
 **/
int tc35892_set_callback(int irq, void *handler, void *data)
{
	irq = irq - EGPIO_PIN_0;
	if (irq < 0 || irq >= MAX_TC35892_GPIO)
		/*number of pin exceeded*/
		return TC35892_BAD_PARAMETER;
	mutex_lock(&the_tc35892->lock);
	the_tc35892->handlers[irq] = handler;
	the_tc35892->data[irq] = data;
	/*if required, you can enable interrupt here
	 */
	mutex_unlock(&the_tc35892->lock);
	return 0;
}
EXPORT_SYMBOL(tc35892_set_callback);

/**
 * tc35892_remove_callback() - remove a callback handler
 * @irq:	gpio number
 *
 * This funtion removes the callback handler for the client device
 **/
int tc35892_remove_callback(int irq)
{
	irq = irq - EGPIO_PIN_0;
	if (irq < 0 || irq >= MAX_TC35892_GPIO)
		/*number of pin exceeded*/
		return TC35892_BAD_PARAMETER;
	mutex_lock(&the_tc35892->lock);
	the_tc35892->handlers[irq] = NULL;
	the_tc35892->data[irq] = NULL;
	/*if required, you can disable interrupt here
	 */
	mutex_unlock(&the_tc35892->lock);
	return 0;
}
EXPORT_SYMBOL(tc35892_remove_callback);

/**
 * tc35892_intr_handler() - interrupt handler for the tc35892
 * @irq:	interrupt number
 * @_chip:	pointer to the tc35892_chip structure
 *
 * This is the interrupt handler for tc35892, since in the interrupt handler
 * there is a need to make a i2c access which is not atomic, we need to schedule
 * a work queue which schedules in the process context
 **/
static irqreturn_t tc35892_intr_handler(int irq, void *_chip)
{
	struct tc35892_chip *chip = _chip;
	schedule_work(&chip->work);

	return IRQ_HANDLED;
}
/**
 * tc35892_work() - bottom half handler for tc35892
 * @_chip:	pointer to the work_struct
 *
 * This is a work queue scheduled from the interrupt handler. This funtion
 * reads from the ISR and finds out the source of interrupt. At the moment
 * the handler handles only GPIO interrupts. Once it figures out the interrupt
 * is from GPIO, a subsequent read is performed on isgpior register to find
 * out the gpio number which triggered the interrupt. This funtion also clears
 * the interrupt after handling.
 **/
static void tc35892_work(struct work_struct *_chip)
{
	struct tc35892_chip *chip =
	    container_of(_chip, struct tc35892_chip, work);
	unsigned char mask;
	int count = 0, i, bit, ret;
	unsigned long mask_intr_status;
	void (*handler) (void *data);

	for (i = 0; i < 3; i++) {
		ret = tc35892_read_byte(chip, (GPIO_MIS0_Index + i), (char *)&mask_intr_status);
		if (ret < 0) {
			printk(KERN_ERR "Error in tc35892_gpio_get_vaule\n");
			return;
		}
		bit = find_first_bit(&mask_intr_status, 8);
		while (bit) {
			handler = the_tc35892->handlers[bit + count];
			if (handler) {
				handler(the_tc35892->data[bit+count]);
				mask = 1 << ((bit + count) % 8);
				tc35892_write_byte(chip, (GPIO_IC0_Index + i), mask);
				tc35892_write_byte(chip, RSTINTCLR_index, 0x01);
				return;
				/*bit = find_next_bit(&mask_intr_status, 8, bit + 1);*/
			} else {
				mask = 1 << ((bit + count) % 8);
				tc35892_write_byte(chip, (GPIO_IC0_Index + i), mask);
				tc35892_write_byte(chip, RSTINTCLR_index, 0x01);
				break;
			}
			/*bit = find_next_bit(&mask_intr_status, 8, bit + 1);*/
		}
		count += 8;
	}
}
/**
 * tc35892_probe() - initialize the tc35892
 * @client:	pointer to the i2c client structure
 * @id:		pointer to the i2c device id table
 *
 * This funtion is called during the kernel boot. This set up the tc35892
 * in a predictable state and attaches the tc35892 to the gpio library structure
 * and set up the i2c client information
 **/
static int tc35892_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct tc35892_platform_data *pdata = client->dev.platform_data;
	struct tc35892_chip *chip;
	unsigned char chip_id, version_id, clk_en;
	int ret, i;

	chip = kzalloc(sizeof(struct tc35892_chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;
	chip->client = client;

	chip->gpio_start = pdata->gpio_base;	/*268 */

	/* setup the gpio chip structure
	 */
	tc35892_setup_gpio(chip, id->driver_data);

	ret = gpiochip_add(&chip->gpio_chip);
	if (ret)
		goto out_failed;

	mutex_init(&chip->lock);

	i2c_set_clientdata(client, chip);

	the_tc35892 = chip;

	INIT_WORK(&chip->work, tc35892_work);

	/*
	 * issue soft reset for all the modules
	 */
	ret = tc35892_write_byte(chip, RSTCTRL_Index, 0x0E);
	if (ret != TC35892_OK) {
		printk(KERN_ERR"couldn't do soft reset the tc35892 modules\n");
		return ret;
	}
	/*configure the EXTRSTN line as the hardware reset*/

	ret = tc35892_write_byte(chip, EXTRSTN_Index, 0x1);
	if (ret != TC35892_OK) {
		printk(KERN_ERR"couldn't configure EXTRSTN line for hard reset \n");
		return ret;
	}
	ret = tc35892_read_byte(chip, MANFACTURE_Code_Index, &chip_id);
	if (ret != TC35892_OK) {
		printk(KERN_ERR"couldn't read the MANFACTURE ID of the GPIO EXPANDER \n");
		return ret;
	}
	ret = tc35892_read_byte(chip, VERSION_ID_Index, &version_id);
	if (ret != TC35892_OK) {
		printk(KERN_ERR"couldn't read the VERSION ID of the GPIO EXPANDER \n");
		return ret;
	}
	ret = tc35892_write_byte(chip, CLKEN_Index, 0x40);
	if (ret != TC35892_OK) {
		printk(KERN_ERR"couldn't configure CLKEN register\n");
		return ret;
	}
	ret = tc35892_read(chip, CLKEN_Index, &clk_en, 1);
	if (ret != TC35892_OK) {
		printk(KERN_ERR"couldn't read CLKEN register\n");
		return ret;
	}
	printk(KERN_INFO
		"tc35892 gpio expander: chip id: %d, version id:%d \n", chip_id,
			version_id);
	ret = tc35892_write_byte(chip, RSTINTCLR_index, 0x01);
	if (ret != TC35892_OK) {
		printk(KERN_ERR"couldn't configure RSTINTCLR register\n");
		return ret;
	}
	for (i = 0; i < MAX_INT_EXP; i++)
		the_tc35892->handlers[i] = NULL;
	ret = request_irq(pdata->irq, tc35892_intr_handler,
			(IRQF_TRIGGER_FALLING), "tc35892", chip);
	if (ret) {
		printk(KERN_ERR
			"unable to request for the irq %d\n", GPIO_TO_IRQ(217));
		gpio_free(pdata->irq);
		return ret;
	}

	return 0;

out_failed:
	kfree(chip);
	return ret;
}

/**
 * tc35892_remove() - freeing all the resources of  tc35892
 * @client:	pointer to the i2c client structure
 *
 * this function will remove the tc35892 by freeing all resources used by the device by using
 *  the current client pointer
 **/
static int __exit tc35892_remove(struct i2c_client *client)
{
	/*TODO- implement the teardown, gpiochip_remove */
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const struct i2c_device_id tc35892_id[] = {
	{"tc35892", 23},
	{}
};

MODULE_DEVICE_TABLE(i2c, tc35892_id);

static struct i2c_driver tc35892_driver = {
	.driver = {
		.name = "tc35892",
		.owner = THIS_MODULE,
		},
	.probe = tc35892_probe,
	.remove = __exit_p(tc35892_remove),
	.id_table = tc35892_id,
};

/**
 *  tc35892_init() - register the tc35892 driver as i2c client driver.
 *  @void: No arguments.
 */
static int __init tc35892_init(void)
{
	return i2c_add_driver(&tc35892_driver);
}

module_init(tc35892_init);
/**
 *  tc35892_exit() - unregister the tc35892 driver.
 *  @void: No arguments.
 */
static void __exit tc35892_exit(void)
{
	i2c_del_driver(&tc35892_driver);
}

module_exit(tc35892_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hanumath Prasad <hanumath.prasad@stericsson.com");
MODULE_DESCRIPTION("TC35892 driver for ST-Ericsson evaluation Platform");
