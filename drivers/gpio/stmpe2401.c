/*
 * Overview:
 *  	Driver for stmpe2401 gpio port expander
 *
 * Copyright (C) 2009 ST Ericsson.
 *
 * Author: Srinidhi Kasagar <srinidhi.kasagar@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License 2 as published by
 * the Free Software Foundation;
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

#include <mach/stmpe2401.h>

static void stmpe2401_work(struct work_struct *_chip);

/* struct stmpe2401_chip - stmpe2401 gpio chip strcture
 * @gpio_chip:	pointer to the gpio_chip library
 * @client:	pointer to the i2c_client structure
 * @lock:	internal sync primitive
 * @gpio_start:	gpio start nunbmer for this chip
 * @handlers:	array of handlers for this chip
 * @data:	callback data
 **/
struct stmpe2401_chip {
	struct gpio_chip gpio_chip;
	struct i2c_client *client;
	struct mutex lock;
	unsigned gpio_start;
	struct work_struct work;
	void (*handlers[MAX_INT]) (void *);
	void *data[MAX_INT];
};

static struct stmpe2401_chip *the_stmpe2401;

/**
 * stmpe2401_write_byte() - Write a single byte
 * @chip:	gpio lib chip strcture
 * @reg:	register offset
 * @data:	data byte to be written
 *
 * This funtion uses smbus byte write API to write a single byte to stmpe2401
 **/
static int stmpe2401_write_byte(struct stmpe2401_chip *chip,
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
 * stmpe2401_read_byte() - Read a single byte
 * @chip:	gpio lib chip strcture
 * @reg:	register offset
 * @val:	data read
 *
 * This funtion uses smbus byte read API to read a byte from the given offset.
 **/
static int stmpe2401_read_byte(struct stmpe2401_chip *chip, unsigned char reg,
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
 * calculate_offset() - calculate the offset
 * @off:	input offset number
 *
 * There are some registers in STMPE with 23 bits, each corresponds to
 * to one GPIO. Based on the input gpio number, this funtion just returns
 * the offset number which can be used for the register base address access
 **/
unsigned char calculate_offset(unsigned off)
{
	if (off < 8)
		return 0x2;
	else if (off > 7 && off < 16)
		return 0x1;
	else
		return 0;
}

/**
 * stmpe2401_gpio_get_value() - Read a GPIO value from the given offset
 * @gc:		pointer to the gpio_chip strcture
 * @off:	The offset to read from
 *
 * This funtion is called from the gpio library to read a GPIO
 * status. This funtion reads from GPMR (Monitor Register) to find
 * out the status
 **/
static int stmpe2401_gpio_get_value(struct gpio_chip *gc, unsigned off)
{
	struct stmpe2401_chip *chip;
	int ret;
	unsigned char reg_val, offset, mask;

	chip = container_of(gc, struct stmpe2401_chip, gpio_chip);

	offset = calculate_offset(off);
	mask = 1 << (off % 8);

	ret = stmpe2401_read_byte(chip, (GPMR_MSB_REG + offset), &reg_val);

	if (ret < 0) {
		printk(KERN_ERR "Error in stmpe2401_gpio_get_vaule\n");
		return ret;
	}
	return (reg_val & mask) ? 1 : 0;
}

/**
 * stmpe2401_gpio_set_value() - Set a GPIO value from the given offset
 * @gc:		pointer to the gpio_chip strcture
 * @off:	The write gpio offset
 *
 * This funtion is called from the gpio library to set/unset a GPIO
 * value. This funtion sets the GPSR (Set Register) to set a bit.
 **/
static void stmpe2401_gpio_set_value(struct gpio_chip *gc,
				     unsigned off, int val)
{
	struct stmpe2401_chip *chip;
	unsigned char temp, offset, mask, reg_val;

	chip = container_of(gc, struct stmpe2401_chip, gpio_chip);

	mutex_lock(&chip->lock);

	offset = calculate_offset(off);
	mask = 1 << (off % 8);

	if (val) {
		reg_val =
		    stmpe2401_read_byte(chip, (GPSR_MSB_REG + offset), &temp);
		temp |= mask;
		stmpe2401_write_byte(chip, (GPSR_MSB_REG + offset), temp);
	} else {
		reg_val =
		    stmpe2401_read_byte(chip, (GPCR_MSB_REG + offset), &temp);
		temp &= ~mask;
		stmpe2401_write_byte(chip, (GPCR_MSB_REG + offset), temp);
	}

	mutex_unlock(&chip->lock);
}

/**
 * stmpe2401_gpio_direction_output() - Set a GPIO direction as output
 * @gc:		pointer to the gpio_chip strcture
 * @off:	The gpio offset
 * @val:	value to be written
 *
 * This funtion is called from the gpio library to set a GPIO
 * direction as output.
 **/
static int stmpe2401_gpio_direction_output(struct gpio_chip *gc,
					   unsigned off, int val)
{
	struct stmpe2401_chip *chip;
	unsigned char reg_val, offset;
	unsigned char mask, temp;

	chip = container_of(gc, struct stmpe2401_chip, gpio_chip);

	offset = calculate_offset(off);
	mask = 1 << (off % 8);

	reg_val = stmpe2401_read_byte(chip, (GPDR_MSB_REG + offset), &temp);
	temp |= mask;
	stmpe2401_write_byte(chip, (GPDR_MSB_REG + offset), temp);

	if (val) {
		reg_val =
		    stmpe2401_read_byte(chip, (GPSR_MSB_REG + offset), &temp);
		temp |= mask;
	} else {
		/*never reached code! */
		reg_val =
		    stmpe2401_read_byte(chip, (GPCR_MSB_REG + offset), &temp);
		temp &= ~mask;
	}
	stmpe2401_write_byte(chip, (GPSR_MSB_REG + offset), temp);

	return 0;
}

/**
 * stmpe2401_gpio_direction_input() - Set a GPIO direction as input
 * @gc:		pointer to the gpio_chip strcture
 * @off:	The gpio offset
 *
 * This funtion is called from the gpio library to set a GPIO
 * direction as input.
 **/
static int stmpe2401_gpio_direction_input(struct gpio_chip *gc, unsigned off)
{
	struct stmpe2401_chip *chip;
	unsigned char reg_val, temp, offset, mask;

	chip = container_of(gc, struct stmpe2401_chip, gpio_chip);

	offset = calculate_offset(off);
	mask = 1 << (off % 8);
	reg_val = stmpe2401_read_byte(chip, (GPDR_MSB_REG + offset), &temp);
	temp &= ~mask;

	stmpe2401_write_byte(chip, (GPDR_MSB_REG + offset), temp);
	return 0;
}

/**
 * stmpe2401_setup_gpio() - Set up the gpio lib structure
 * @chip:	pointer to the stmpe2401_chip strcture
 * @gpios:	Number of GPIO's
 *
 * This funtion set up the  gpio_chip structure to the corresponding
 * callback funtions and other configurations.
 **/
static void stmpe2401_setup_gpio(struct stmpe2401_chip *chip, int gpios)
{
	struct gpio_chip *gc;

	gc = &chip->gpio_chip;

	gc->direction_input = stmpe2401_gpio_direction_input;
	gc->direction_output = stmpe2401_gpio_direction_output;
	gc->get = stmpe2401_gpio_get_value;
	gc->set = stmpe2401_gpio_set_value;

	gc->base = chip->gpio_start;
	gc->ngpio = gpios;
	gc->label = chip->client->name;
	gc->dev = &chip->client->dev;
	gc->owner = THIS_MODULE;
}

/*static int stmpe2401_add_irq_work(int irq,
		void (*handler)(struct stmpe2401_chip *))*/
/**
 * stmpe2401_set_callback() - install a callback handler
 * @irq:	gpio number
 * @handler:	funtion pointer to the callback handler
 *
 * This funtion install the callback handler for the client device
 **/
int stmpe2401_set_callback(int irq, void *handler, void *data)
{
	mutex_lock(&the_stmpe2401->lock);
	irq -= the_stmpe2401->gpio_start;
	the_stmpe2401->handlers[irq] = handler;
	the_stmpe2401->data[irq] = data;

	/*if required, you can enable interrupt here
	 */
	mutex_unlock(&the_stmpe2401->lock);
	return 0;
}
EXPORT_SYMBOL(stmpe2401_set_callback);

/**
 * stmpe2401_remove_callback() - remove a callback handler
 * @irq:	gpio number
 *
 * This funtion removes the callback handler for the client device
 **/
int stmpe2401_remove_callback(int irq)
{
	mutex_lock(&the_stmpe2401->lock);
	irq -= the_stmpe2401->gpio_start;
	the_stmpe2401->handlers[irq] = NULL;
	the_stmpe2401->data[irq] = 0;
	/*if required, you can disable interrupt here
	 */
	mutex_unlock(&the_stmpe2401->lock);
	return 0;

}
EXPORT_SYMBOL(stmpe2401_remove_callback);

/**
 * stmpe2401_intr_handler() - interrupt handler for the stmpe2401
 * @irq:	interrupt number
 * @_chip:	pointer to the stmpe2401_chip structure
 *
 * This is the interrupt handler for stmpe2401, since in the interrupt handler
 * there is a need to make a i2c access which is not atomic, we need to schedule
 * a work queue which schedules in the process context
 **/
static irqreturn_t stmpe2401_intr_handler(int irq, void *_chip)
{
	struct stmpe2401_chip *chip = _chip;

	/*chip = container_of(gc,struct stmpe2401_chip, gpio_chip); */
	schedule_work(&chip->work);

	return IRQ_HANDLED;
}

/**
 * stmpe2401_work() - bottom half handler for stmpe2401
 * @_chip:	pointer to the work_struct
 *
 * This is a work queue scheduled from the interrupt handler. This funtion
 * reads from the ISR and finds out the source of interrupt. At the moment
 * the handler handles only GPIO interrupts. Once it figures out the interrupt
 * is from GPIO, a subsequent read is performed on isgpior register to find
 * out the gpio number which triggered the interrupt. This funtion also clears
 * the interrupt after handling.
 **/
static void stmpe2401_work(struct work_struct *_chip)
{
	struct stmpe2401_chip *chip =
	    container_of(_chip, struct stmpe2401_chip, work);
	unsigned char isr, mask = 0, isgpior_index = ISGPIOR_MSB_REG;
	unsigned char gpedr_index = GPEDR_MSB_REG, gpedr;
	unsigned char val;
	int count = 16, i, bit;
	unsigned long isgpior;
	void (*handler) (void *data);

	/*read the ISR */
	stmpe2401_read_byte(chip, ISR_MSB_REG, &isr);
	if (isr & 0x1) {
		/* clear the GPIO interrupt by writing back to
		 * ISR register
		 */
		stmpe2401_write_byte(chip, ISR_MSB_REG, 0x1);
		/* now do a subsequent read of ISGPIOR to obtain the
		 * interrupt status of all 23 GPIO's to indentify which
		 * GPIO triggered the interrupt
		 */
		for (i = 0; i < 3; i++)	{
			stmpe2401_read_byte(chip, (isgpior_index + i),
					(u8 *)&isgpior);
			bit = find_first_bit(&isgpior, 8);
			while (bit < 8) {
				handler = the_stmpe2401->handlers[bit + count];
				if (handler)
					handler(the_stmpe2401->
						data[bit + count]);
				else
					mask = 1 << (bit + count) % 8;

				stmpe2401_write_byte(chip, (isgpior_index + i),
					(isgpior | mask));
				/* clear the GPEDR */
				stmpe2401_read_byte(chip, (gpedr_index + i),
						&gpedr);
				stmpe2401_write_byte(chip, (gpedr_index + i),
						(gpedr | mask));

				stmpe2401_read_byte(chip, IEGPIOR_LSB_REG,
						&val);
				bit = find_next_bit(&isgpior, 8, bit + 1);
			}
			count >>= 1;
			if (count == 4)
				count = 0;

		}
	} else {
		printk(KERN_NOTICE "We do not yet support this type of interrupt\n");
	}
}

/**
 * stmpe2401_probe() - initialize the stmpe2401
 * @client:	pointer to the i2c client structure
 * @id:		pointer to the i2c device id table
 *
 * This funtion is called during the kernel boot. This set up the stmpe2401
 * in a predictable state and attaches the stmpe to the gpio library structure
 * and set up the i2c client information
 **/
static int stmpe2401_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct stmpe2401_platform_data *pdata = client->dev.platform_data;
	struct stmpe2401_chip *chip;
	unsigned char chip_id, version_id;
	int ret;

	chip = kzalloc(sizeof(struct stmpe2401_chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;
	chip->client = client;

	chip->gpio_start = pdata->gpio_base;	/*268 */

	/* setup the gpio chip structure
	 */
	stmpe2401_setup_gpio(chip, id->driver_data);

	ret = gpiochip_add(&chip->gpio_chip);
	if (ret)
		goto out_failed;

	mutex_init(&chip->lock);

	i2c_set_clientdata(client, chip);

	the_stmpe2401 = chip;

	INIT_WORK(&chip->work, stmpe2401_work);

	/*issue soft reset to the device */
	stmpe2401_write_byte(chip, SYSCON_REG, 0x80);

	/* FIXME : pass this info from platform data */
#if defined(CONFIG_MACH_U8500_MOP)
	/* setup the default configuration of gpios for
	 * this platform. This may vary depending on the platform
	 * chosen. Should it be here? - FIXME
	 * set the gpio's 3,8,9,10,11,12,13,14,16,17,21 as input
	 * and the test as output.
	 */
	stmpe2401_write_byte(chip, GPDR_MSB_REG, 0xDC);
	stmpe2401_write_byte(chip, GPDR_CSB_REG, 0x80);
	stmpe2401_write_byte(chip, GPDR_LSB_REG, 0xF7);

	/* Enable only GPIO controller of this chip.
	 * The other supported blocks from this chip are keypad controller,
	 * rotator module, PWM module, and other power management
	 * capability modules which are bot used
	 */
	stmpe2401_write_byte(chip, SYSCON_REG, 0x08);

	/* set up the interrupt controller which allows
	 * hosts to trigger the interrupt upon event
	 */

	/* Enable only the GPIO to trigger the interrupt
	 */
	stmpe2401_write_byte(chip, IER_MSB_REG, 0x1);

	/* Enable the Interrupt sources for this platform.
	 * The interrupt sources are fixed in this platform,
	 * and for the other platforms, this may vary
	 */
	/* This enables the SD/MMC card detect interrupt, and
	 * NFC interrupt on this platform
	 */
	stmpe2401_write_byte(chip, IEGPIOR_MSB_REG, 0x23);
	/* This enables accelerometer, magnetometer and
	 * navi interrupts (i.e gpio 8,9,10,11,12,13,14)
	 */
	stmpe2401_write_byte(chip, IEGPIOR_CSB_REG, 0x7F);
	/* This enables touch panel interrupt on this
	 * platform. i.e gpio 3
	 */
	stmpe2401_write_byte(chip, IEGPIOR_LSB_REG, 0x08);

	/* configure the rising and falling edge
	 * configurations for this platform
	 */
	stmpe2401_write_byte(chip, GPRER_MSB_REG, 0x0);
	stmpe2401_write_byte(chip, GPRER_CSB_REG, 0x0);
	stmpe2401_write_byte(chip, GPRER_LSB_REG, 0x0);
	/* set the gpio pins 16,17,21 as falling edge
	 * interrupts
	 */
	stmpe2401_write_byte(chip, GPFER_MSB_REG, 0x23);
	/* set the gpio pins 8,9,10,11,12,13,14 as falling edge */
	stmpe2401_write_byte(chip, GPFER_CSB_REG, 0x7F);
	/* set the gpio pin 3 as falling edge */
	stmpe2401_write_byte(chip, GPFER_LSB_REG, 0x08);
	/* disable the internal pull up resistor for the gpios 16,17,21,
	 * and also for 8,9,10,11,12,13,14
	 */
	stmpe2401_write_byte(chip, GPPUR_MSB_REG, 0x23);
	stmpe2401_write_byte(chip, GPPUR_CSB_REG, 0x7F);
	/* set the gpio pin 3 as Pull up */
	stmpe2401_write_byte(chip, GPPUR_LSB_REG, 0x08);
	/* disable the internal pull down resistors for the
	 * above set of gpios
	 */
	stmpe2401_write_byte(chip, GPPDR_MSB_REG, 0x0);
	stmpe2401_write_byte(chip, GPPDR_CSB_REG, 0x0);
	stmpe2401_write_byte(chip, GPPDR_LSB_REG, 0x0);
	/* set all the gpios as primary funtion; alternate
	 * funtions are not used on this platform.
	 */
	stmpe2401_write_byte(chip, GPAFR_U_MSB_REG, 0x0);
	stmpe2401_write_byte(chip, GPAFR_U_CSB_REG, 0x0);
	stmpe2401_write_byte(chip, GPAFR_U_LSB_REG, 0x0);
	stmpe2401_write_byte(chip, GPAFR_L_MSB_REG, 0x0);
	stmpe2401_write_byte(chip, GPAFR_L_CSB_REG, 0x0);
	stmpe2401_write_byte(chip, GPAFR_L_LSB_REG, 0x0);

	/* configure the Interrupt controller to trigger
	 * interrupt on falling edge & enable the global interrupt
	 * mask which allows host to trigger the interrupt.
	 */
	stmpe2401_write_byte(chip, ICR_LSB_REG, 0x03);

	ret =
	    request_irq(pdata->irq, stmpe2401_intr_handler,
			IRQF_TRIGGER_FALLING, "stmpe2401", chip);
	if (ret) {
		printk(KERN_ERR
			"unable to request for the irq %d\n", GPIO_TO_IRQ(217));
		gpio_free(pdata->irq);
		return ret;
	}
#endif
	/*read the chip id and version id */
	stmpe2401_read_byte(chip, CHIP_ID_REG, &chip_id);
	stmpe2401_read_byte(chip, VERSION_ID_REG, &version_id);
	printk(KERN_INFO
	       "stmpe2401 gpio expander: chip id: %d, version id:%d\n", chip_id,
	       version_id);

	return 0;
out_failed:
	kfree(chip);
	return ret;
}

static int __exit stmpe2401_remove(struct i2c_client *client)
{
	/*TODO- implement the teardown, gpiochip_remove */
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const struct i2c_device_id stmpe2401_id[] = {
	{"stmpe2401", 23},
	{}
};

MODULE_DEVICE_TABLE(i2c, stmpe2401_id);

static struct i2c_driver stmpe2401_driver = {
	.driver = {
		   .name = "stmpe2401",
		   .owner = THIS_MODULE,
		   },
	.probe = stmpe2401_probe,
	.remove = __exit_p(stmpe2401_remove),
	.id_table = stmpe2401_id,
};

static int __init stmpe2401_init(void)
{
	return i2c_add_driver(&stmpe2401_driver);
}

module_init(stmpe2401_init);
static void __exit stmpe2401_exit(void)
{
	i2c_del_driver(&stmpe2401_driver);
}

module_exit(stmpe2401_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Srinidhi Kasagar <srinidhi.kasagar@stericsson.com");
MODULE_DESCRIPTION("STMPE2401 driver for ST Ericsson evaluation Platform");
