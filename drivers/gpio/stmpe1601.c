/*
 * Overview:
 *  	Driver for stmpe1601 on U8500 Platforms
 *	heavily based on stmpe2401 driver
 *
 * Copyright (C) 2009 ST-Ericsson SA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

#include <mach/stmpe1601.h>

#define KPD_INT_MASK 		0x2
#define KPD_OVERRUN_INT_MASK	0x4

static void stmpe1601_work(struct work_struct *_chip);

/* struct stmpe1601_chip - stmpe1601 gpio chip strcture
 * @gpio_chip:	pointer to the gpio_chip library
 * @client:	pointer to the i2c_client structure
 * @lock:	internal sync primitive
 * @gpio_start:	gpio start nunbmer for this chip
 * @handlers:	array of handlers for this chip
 * @data:	callback data
 **/
struct stmpe1601_chip {
	struct gpio_chip gpio_chip;
	struct i2c_client *client;
	struct mutex lock;
	unsigned gpio_start;
	struct work_struct work;
	void (*handlers[STMPE1601_MAX_INT+1]) (void *);
	void *data[STMPE1601_MAX_INT+1];
	t_stmpe1601_device_config kpconfig;
};

static struct stmpe1601_chip *the_stmpe1601;

/**
 * stmpe1601_write_byte() - Write a single byte
 * @chip:	gpio lib chip strcture
 * @reg:	register offset
 * @data:	data byte to be written
 *
 * This funtion uses smbus byte write API to write a single byte to stmpe1601
 **/
static int stmpe1601_write_byte(struct stmpe1601_chip *chip,
				unsigned char reg, unsigned char data)
{
	int ret;
	ret = i2c_smbus_write_byte_data(chip->client, reg, data);
	if (ret < 0) {
		printk(KERN_ERR "i2c smbus write byte failed for STMPE1601 client\n");
		return ret;
	}
	/*
	printk("\nstmpe1601:write data = %x reg = %x", data, reg);
	*/
	return 0;
}

/**
 * stmpe1601_read_byte() - Read a single byte
 * @chip:	gpio lib chip strcture
 * @reg:	register offset
 * @val:	data read
 *
 * This funtion uses smbus byte read API to read a byte from the given offset.
 **/
static int stmpe1601_read_byte(struct stmpe1601_chip *chip, unsigned char reg,
			       unsigned char *val)
{
	int ret;
	ret = i2c_smbus_read_byte_data(chip->client, reg);
	if (ret < 0) {
		printk(KERN_ERR "error in reading a byte, smbus byte read failed for STMPE1601 client\n");
		return ret;
	}
	/*
	printk("\nstmpe1601:read data =%x reg =%x\n" ,ret,reg);
	*/
	*val = ret;
	return 0;
}

/**
 * stmpe1601_read() - read data from stmpe
 * @chip:	gpio lib chip strcture
 * @reg:        register offset
 * @buf:	read the data in this buffer
 * @nbytes:	number of bytes to read
 *
 * This funtion uses smbus read block API to read multiple bytes
 * from the reg offset.
 **/
static int stmpe1601_read(struct stmpe1601_chip *chip, unsigned char reg,
			unsigned char *buf, unsigned char nbytes)
{
	int ret = 0;
	ret = i2c_smbus_read_i2c_block_data(chip->client, reg, nbytes, buf);
	if (ret < 0)
		return ret;
	if (ret < nbytes)
		return -EIO;
	/*
	 * printk("stmpe1601_read: reg=%x,no. of data = %d, data =%x\n",
	 * reg,nbytes, *buf);
	*/
	return 0;
}

/**
 * calculate_offset() - calculate the offset
 * @index:	input offset number
 *
 * There are some registers in STMPE with 16 bits, each corresponds to
 * to one GPIO. Based on the input gpio number, this funtion just returns
 * the offset number which can be used for the register base address access
 **/
static unsigned char calculate_offset(unsigned index)
{
	if (index < 8)
		return 1;
	else
		return 0;
}

/**
 * stmpe1601_gpio_get_value() - Read a GPIO value from the given offset
 * @gc:		pointer to the gpio_chip strcture
 * @off:	The offset to read from
 *
 * This funtion is called from the gpio library to read a GPIO
 * status. This funtion reads from GPMR (Monitor Register) to find
 * out the status
 **/
static int stmpe1601_gpio_get_value(struct gpio_chip *gc, unsigned off)
{
	struct stmpe1601_chip *chip;
	int ret;
	unsigned char reg_val, offset, mask;

	chip = container_of(gc, struct stmpe1601_chip, gpio_chip);

	offset = calculate_offset(off);
	mask = 1 << (off % 8);

	ret = stmpe1601_read_byte(chip, (GPMR_Lsb_Index + offset), &reg_val);
	if (ret < 0) {
		printk(KERN_ERR "Error in stmpe1601_gpio_get_vaule\n");
		return ret;
	}
	return (reg_val & mask) ? 1 : 0;
}


/**
 * stmpe1601_gpio_set_value() - Set a GPIO value from the given offset
 * @gc:		pointer to the gpio_chip strcture
 * @off:	The write gpio offset
 * @val:	data to be set, 0 or 1, represents high or low state of pin
 *
 * This funtion is called from the gpio library to set/unset a GPIO
 * value. This funtion sets the GPSR (Set Register) to set a bit.
 **/
static void stmpe1601_gpio_set_value(struct gpio_chip *gc,
				     unsigned off, int val)
{
	struct stmpe1601_chip *chip;
	unsigned char temp, offset, mask, reg_val;

	chip = container_of(gc, struct stmpe1601_chip, gpio_chip);

	mutex_lock(&chip->lock);

	offset = calculate_offset(off);
	mask = 1 << (off % 8);

	if (val) {
		reg_val =
		    stmpe1601_read_byte(chip, (GPSR_Msb_Index + offset), &temp);
		temp |= mask;
		stmpe1601_write_byte(chip, (GPSR_Msb_Index + offset), temp);
	} else {
		reg_val =
		    stmpe1601_read_byte(chip, (GPCR_Msb_Index + offset), &temp);
		temp &= ~mask;
		stmpe1601_write_byte(chip, (GPCR_Msb_Index + offset), temp);
	}

	mutex_unlock(&chip->lock);
}


/**
 * stmpe1601_gpio_direction_output() - Set a GPIO direction as output
 * @gc:		pointer to the gpio_chip strcture
 * @off:	The gpio offset
 * @val:	value to be written
 *
 * This funtion is called from the gpio library to set a GPIO
 * direction as output.
 **/
static int stmpe1601_gpio_direction_output(struct gpio_chip *gc,
					   unsigned off, int val)
{
	struct stmpe1601_chip *chip;
	unsigned char reg_val, offset;
	unsigned char mask, temp;

	chip = container_of(gc, struct stmpe1601_chip, gpio_chip);

	offset = calculate_offset(off);
	mask = 1 << (off % 8);

	reg_val = stmpe1601_read_byte(chip, (GPDR_Msb_Index + offset), &temp);
	temp |= mask;
	stmpe1601_write_byte(chip, (GPDR_Msb_Index + offset), temp);

	if (val) {
		reg_val =
		    stmpe1601_read_byte(chip, (GPSR_Msb_Index + offset), &temp);
		temp |= mask;
		stmpe1601_write_byte(chip, (GPSR_Msb_Index + offset), temp);
	} else {
		/*never reached code! */
		reg_val =
		    stmpe1601_read_byte(chip, (GPCR_Msb_Index + offset), &temp);
		temp &= ~mask;
		stmpe1601_write_byte(chip, (GPCR_Msb_Index + offset), temp);
	}

	return 0;
}

/**
 * stmpe1601_gpio_direction_input() - Set a GPIO direction as input
 * @gc:		pointer to the gpio_chip strcture
 * @off:	The gpio offset
 *
 * This funtion is called from the gpio library to set a GPIO
 * direction as input.
 **/
static int stmpe1601_gpio_direction_input(struct gpio_chip *gc, unsigned off)
{
	struct stmpe1601_chip *chip;
	unsigned char reg_val, temp, offset, mask;

	chip = container_of(gc, struct stmpe1601_chip, gpio_chip);

	offset = calculate_offset(off);
	mask = 1 << (off % 8);
	reg_val = stmpe1601_read_byte(chip, (GPDR_Msb_Index + offset), &temp);
	temp &= ~mask;

	stmpe1601_write_byte(chip, (GPDR_Msb_Index + offset), temp);
	return 0;
}

/**
 * stmpe1601_setup_gpio() - Set up the gpio lib structure
 * @chip:	pointer to the stmpe1601_chip strcture
 * @gpios:	Number of GPIO's
 *
 * This funtion set up the  gpio_chip structure to the corresponding
 * callback funtions and other configurations.
 **/
static void stmpe1601_setup_gpio(struct stmpe1601_chip *chip, int gpios)
{
	struct gpio_chip *gc;

	gc = &chip->gpio_chip;

	gc->direction_input = stmpe1601_gpio_direction_input;
	gc->direction_output = stmpe1601_gpio_direction_output;
	gc->get = stmpe1601_gpio_get_value;
	gc->set = stmpe1601_gpio_set_value;

	gc->base = chip->gpio_start;
	gc->ngpio = gpios;
	gc->label = chip->client->name;
	gc->dev = &chip->client->dev;
	gc->owner = THIS_MODULE;
}


/**
 * stmpe1601_set_callback() - install a callback handler
 * @irq:	gpio number
 * @handler:	funtion pointer to the callback handler
 * @data:	data pointer to be passed to the specific handler
 *
 * This funtion install the callback handler for the client device
 **/
int stmpe1601_set_callback(int irq, void *handler, void *data)
{
	if ((irq > STMPE1601_MAX_INT) || (irq < 0))
		return -EINVAL;
	mutex_lock(&the_stmpe1601->lock);

	the_stmpe1601->handlers[irq] = handler;
	the_stmpe1601->data[irq] = data;
#if 0
	if (irq == KEYPAD_INT) {
		/**
		 * enable keypad interrupt mask
		 */
		stmpe1601_write_byte(the_stmpe1601, IER_Lsb_Index,
				     (KPD_INT_MASK | KPD_OVERRUN_INT_MASK));

	}
#endif
	/*if required, you can enable interrupt here
	 */
	mutex_unlock(&the_stmpe1601->lock);
	return 0;
}
EXPORT_SYMBOL(stmpe1601_set_callback);


/**
 * stmpe1601_remove_callback() - remove a callback handler
 * @irq:	gpio number
 *
 * This funtion removes the callback handler for the client device
 **/
int stmpe1601_remove_callback(int irq)
{
	if ((irq > STMPE1601_MAX_INT) || (irq < 0))
		return -EINVAL;
	mutex_lock(&the_stmpe1601->lock);
	the_stmpe1601->handlers[irq] = NULL;
	the_stmpe1601->data[irq] = NULL;
	if (irq == KEYPAD_INT) {
		/* if required, you can disable interrupt here */
		stmpe1601_write_byte(the_stmpe1601, IER_Lsb_Index, 0x0);
	}
	mutex_unlock(&the_stmpe1601->lock);
	return 0;

}
EXPORT_SYMBOL(stmpe1601_remove_callback);


/**
 * stmpe1601_intr_handler() - interrupt handler for the stmpe1601
 * @irq:	interrupt number
 * @_chip:	pointer to the stmpe1601_chip structure
 *
 * This is the interrupt handler for stmpe1601, since in the interrupt handler
 * there is a need to make a i2c access which is not atomic, we need to schedule
 * a work queue which schedules in the process context
 **/
static irqreturn_t stmpe1601_intr_handler(int irq, void *_chip)
{
	struct stmpe1601_chip *chip = _chip;

	/* chip = container_of(gc,struct stmpe1601_chip, gpio_chip); */
	/* printk("Interrupt from STMPE1601 @ GPIO %d\n", irq); */
	schedule_work(&chip->work);

	return IRQ_HANDLED;
}


/**
 * stmpe1601_work() - bottom half handler for stmpe1601
 * @_chip:	pointer to the work_struct
 *
 * This is a work queue scheduled from the interrupt handler. This funtion
 * reads from the ISR and finds out the source of interrupt. At the moment
 * the handler handles only GPIO interrupts & keypad interrupt. Once it
 * figures out the interrupt is from GPIO/Keypad, a subsequent read is performed
 * on isgpior register to find out the gpio number which triggered the
 * interrupt. This funtion also clears the interrupt after handling.
 **/
static void stmpe1601_work(struct work_struct *_chip)
{
	struct stmpe1601_chip *chip =
	    container_of(_chip, struct stmpe1601_chip, work);
	unsigned char isr;

	/*read the ISR LSB*/
	stmpe1601_read_byte(chip, ISR_Lsb_Index, &isr);
	if (isr & KPD_OVERRUN_INT_MASK) {
		/* clear the Keypad overrun interrupt by writing back to
		 * ISR register and print error msg
		 */
		stmpe1601_write_byte(chip, ISR_Msb_Index, KPD_OVERRUN_INT_MASK);
		printk(KERN_ERR "\n ERR: Keypad overrurn");
	}
	if (isr & KPD_INT_MASK) {
		/* clear the Keypad interrupt by writing back to
		 * ISR register and call the handler
		 */
		stmpe1601_write_byte(chip, ISR_Msb_Index, KPD_INT_MASK);
		/*
		printk("\n Work queue: Keypad interrupt");
		*/
		if (the_stmpe1601->handlers[KEYPAD_INT])
			(the_stmpe1601->handlers[KEYPAD_INT])(the_stmpe1601->\
							      data[KEYPAD_INT]);
	}
	if (!(isr & KPD_OVERRUN_INT_MASK) && !(isr & KPD_INT_MASK)) {
		printk(KERN_ERR "\n STMPE1601 : stray interrupt ISR reg = %x",
			isr);
		/* clear all iterrupts in that case
		*/
		stmpe1601_write_byte(chip, ISR_Msb_Index, 0x1);
		stmpe1601_write_byte(chip, ISR_Lsb_Index, 0xFF);
	}

}

/**
 * stmpe1601_probe() - initialize the stmpe1601
 * @client:	pointer to the i2c client structure
 * @id:		pointer to the i2c device id table
 *
 * This funtion is called during the kernel boot. This set up the stmpe1601
 * in a predictable state and attaches the stmpe to the gpio library
 * structure and set up the i2c client information
 **/
static int stmpe1601_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct stmpe1601_platform_data *pdata = client->dev.platform_data;
	struct stmpe1601_chip *chip;
	int ret;
	unsigned char buf[5];

	chip = kzalloc(sizeof(struct stmpe1601_chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;
	chip->client = client;

	chip->gpio_start = pdata->gpio_base;	/* 268 + 24 */

	/* setup the gpio chip structure
	 */
	stmpe1601_setup_gpio(chip, id->driver_data);

	ret = gpiochip_add(&chip->gpio_chip);
	if (ret)
		goto out_failed;

	mutex_init(&chip->lock);

	i2c_set_clientdata(client, chip);

	the_stmpe1601 = chip;

	INIT_WORK(&chip->work, stmpe1601_work);

	/* issue soft reset to the device
	 */
	stmpe1601_write_byte(chip, SYSCON_Index, 0x80);

	/* read the chip id and version id and exit in case of any error
	 */
	ret =  stmpe1601_read_info();
	if (ret)
		goto out_failed;

#if defined(CONFIG_MACH_U8500_MOP)
	/* setup the default keypad configuration for this platform
	 * This may vary depending on the platform chosen
	 */

	/* Enable only keypad controllder of this chip.
	 * The other supported blocks from this chip are GPIO controller
	 * rotator module, PWM module, and other power management
	 * capability modules which are not used
	 */
	stmpe1601_write_byte(chip, SYSCON_Index, 0x0B);

	/* set all the gpios as alternate function 0,
	 * Alt func 1 is required for keypad, to be configured by kpd driver
	 */
	stmpe1601_write_byte(chip, GPAFR_U_Msb_Index, 0x00);
	stmpe1601_write_byte(chip, GPAFR_U_Lsb_Index, 0x00);
	stmpe1601_write_byte(chip, GPAFR_L_Msb_Index, 0x00);
	stmpe1601_write_byte(chip, GPAFR_L_Lsb_Index, 0x00);

	/* set up the interrupt controller which allows
	 * hosts to trigger the interrupt upon event
	 */

	/* Disable the rising and falling edge
	 * configurations for GPIO interrupts
	 */
	stmpe1601_write_byte(chip, GPRER_Msb_Index, 0x0);
	stmpe1601_write_byte(chip, GPRER_Lsb_Index, 0x0);


	/* Enable only the keypad to trigger the interrupt, disable the
	 * keypad interrupt mask, to be enabled later by keypad driver.
	 * 0x6 to enable keypad interrupt and overrun interrupt
	 */
	stmpe1601_write_byte(chip, IER_Lsb_Index, 0x0
			     /*(KPD_INT_MASK | KPD_OVERRUN_INT_MASK)*/);

	/* Empty FIFO by reading all data bytes
	*/
	ret = stmpe1601_read(the_stmpe1601, KPC_DATA_BYTE0_Index, buf, 5);
	ret = stmpe1601_read(the_stmpe1601, KPC_DATA_BYTE0_Index, buf, 5);

	ret =
	    request_irq(pdata->irq, stmpe1601_intr_handler,
			IRQF_TRIGGER_FALLING, "stmpe1601", chip);
	if (ret) {
		printk(KERN_ERR
			"unable to request for the irq %d\n",
			GPIO_TO_IRQ(STMPE16010_INTR)); /* 218 */
		gpio_free(pdata->irq);
		return ret;
	}

	/**
	 * Need to enable internal pull up of GPIO PIN 218, required for
	 * STMPE1601 INT
	 */

	/**
	*  This does not work as the GPIO pin is already configured as IRQ,
	*  return busy gpio_set_value(STMPE16010_INTR, GPIO_HIGH);

	gpio_set_value(STMPE16010_INTR, GPIO_PULLUP_EN);
	gpio_set_value(STMPE16010_INTR, GPIO_HIGH);
	*/

	#if 1
	/* HACK ... to be removed later - FIXME
	 */
	volatile unsigned int *gpio_base_address;
	gpio_base_address = (unsigned int *)ioremap(0x8011E000,
						    0x8011EFFC-0x8011E000 + 1);
	/* Now set bit 26 of the GPIO_DAT register to enable pull up for GPIO218
	 */
	*gpio_base_address = (*gpio_base_address) | 0x4000000;
	iounmap(gpio_base_address);
	gpio_base_address = NULL;
	#endif

	/**
	 * configure the Interrupt controller to trigger
	 * interrupt on falling edge & enable the global interrupt
	 * mask which allows host to trigger the interrupt.
	 */
	stmpe1601_write_byte(chip, ICR_Lsb_Index, 0x3);
	/* Falling edge sensitive irq */
	/**
	* clear all interrupts
	*/
	stmpe1601_write_byte(chip, ISR_Msb_Index, 0x1);
	stmpe1601_write_byte(chip, ISR_Lsb_Index, 0xFF);

#endif
	return 0;

out_failed:
	kfree(chip);
	the_stmpe1601 = NULL;
	return ret;
}

/**
 * stmpe1601_keypad_init - initialises Keypad matrix row and columns
 * @kpconfig:    keypad configuration for a platform
 *
 * This function configures keypad control registers of stmpe1601
 * The keypad driver should call this function to configure keypad matrix
 **/
int stmpe1601_keypad_init(t_stmpe1601_key_config  kpconfig)
{
	int retval = 0;
	unsigned char i, reg_val;
	unsigned int temp_row, temp_col;

	/*
	* settings verification max 8 columns & max 8
	* rows, max debounce time & cycle count
	*/
	if ((kpconfig.columns > 0x00FF) || (kpconfig.rows > 0x00FF)
	|| (kpconfig.ncycles > 15) || (kpconfig.debounce > 127)) {
		return -1;
		/*
		* TO DO return proper error code
		*/
	}

	/*
	* setting GPIO alternate function
	* columns 0-7 are connected to gpio 0-7
	* rows 0-7 are connected to gpio 8-15
	*/
	temp_row = 0x0; temp_col = 0x0;
	for (i = 0; i < 8; i++) {
		if ((kpconfig.columns & (1 << i)) != 0)
			temp_col |= (0x01 << (i*2));
		if ((kpconfig.rows & (1 << i)) != 0)
			temp_row |= (0x01 << (i*2));
	}
	mutex_lock(&the_stmpe1601->lock);
	stmpe1601_write_byte(the_stmpe1601, GPAFR_L_Lsb_Index,
			     temp_col & 0xFF);
	stmpe1601_write_byte(the_stmpe1601, GPAFR_L_Msb_Index,
			     (temp_col >> 8) & 0xFF);

	stmpe1601_write_byte(the_stmpe1601, GPAFR_U_Lsb_Index,
			     temp_row & 0xFF);
	stmpe1601_write_byte(the_stmpe1601, GPAFR_U_Msb_Index,
			     (temp_row >> 8) & 0xFF);
	/*
	* configure scan on/off
	*/
	stmpe1601_read_byte(the_stmpe1601, KPC_CTRL_Lsb_Index, &reg_val);
	if (kpconfig.scan == 1)
		reg_val |= 0x1;
	else
		reg_val &= ~0x1;

	stmpe1601_write_byte(the_stmpe1601, KPC_CTRL_Lsb_Index, reg_val);

	/*
	* configure individual row/column scan
	*/
	stmpe1601_write_byte(the_stmpe1601, KPC_COL_Index,
			     (kpconfig.columns & 0xFF)); /* columns reg */
	stmpe1601_write_byte(the_stmpe1601, KPC_ROW_Lsb_Index,
			     (kpconfig.rows & 0xFF)); /* row reg */

	/**
	* configure ctrl reg msb, scan count
	*/
	stmpe1601_read_byte(the_stmpe1601, KPC_CTRL_Msb_Index, &reg_val);
	reg_val &= 0x0F;
	reg_val |= (kpconfig.ncycles << 4) & 0xF0;
	retval |= stmpe1601_write_byte(the_stmpe1601, KPC_CTRL_Msb_Index,
				       reg_val);
	/**
	 * configure ctrl reg lsb, debounce time
	 */
	stmpe1601_write_byte(the_stmpe1601, KPC_CTRL_Lsb_Index,
			     ((kpconfig.debounce << 1) & 0xFE));
	mutex_unlock(&the_stmpe1601->lock);

	return 0;
}
EXPORT_SYMBOL(stmpe1601_keypad_init);

/**
 * stmpe1601_keypad_scan - start/stop keypad scannig
 * @status:    flag for enable/disable STMPE1601_SCAN_ON or STMPE1601_SCAN_OFF
 *
 **/
int stmpe1601_keypad_scan(unsigned char status)
{
	int retval = 0;
	unsigned char tempByte;

	/*
	 * configure scan on/off
	 */
	stmpe1601_read_byte(the_stmpe1601, KPC_CTRL_Lsb_Index, &tempByte);

	switch (status)	{
	case STMPE1601_SCAN_ON:
		tempByte |=  1;
		break;
	case STMPE1601_SCAN_OFF:
		tempByte &= ~1;
		break;
	default:
		/**
		 * TO DO return proper err code
		 */
		retval = -1;
		break;
	};

	if (retval == 0)
		stmpe1601_write_byte(the_stmpe1601, KPC_CTRL_Lsb_Index,
				     tempByte);
	return retval;
}
EXPORT_SYMBOL(stmpe1601_keypad_scan);

/**
 * stmpe1601_keypressed - This function read keypad data registers
 * @keys: o/p parameter, returns keys pressed.
 *
 * This function can be used in both polling or interrupt usage.
 */
int stmpe1601_keypressed(t_stmpe1601_key_status *keys)
{
	int retval = 0;
	static unsigned char tempBuffer[5];
	int i = 0;
	/** check for NULL argument
	 */
	if (!keys)
		return -EINVAL;

	keys->button_pressed = 0;
	keys->button_released = 0;

	retval = stmpe1601_read(the_stmpe1601, KPC_DATA_BYTE0_Index,
				tempBuffer, 5);
	for (i = 0; i < 2; i++) {
		if ((tempBuffer[i] & STMPE1601_MASK_NO_KEY) !=
						    STMPE1601_MASK_NO_KEY) {
			if ((tempBuffer[i] & 0x80) == 0) {
				keys->button[keys->button_pressed] =
				    tempBuffer[i] & 0x7F;
				keys->button_pressed++;
				/*
				 printk("\n button[%d]=[%d, %d] --- DOWN ",
				 keys->button_pressed, (tempBuffer[i] >> 3)\
				 & 0x7, tempBuffer[i] & 0x7 );
				 */
			} else {
				keys->released[keys->button_released] =
				    tempBuffer[i] & 0x7F;
				keys->button_released++;
				/*
				printk("\n released[%d]=[%d, %d] --- UP ",
				keys->button_released, (tempBuffer[i]>>3)& 0x7,
				tempBuffer[i] & 0x7 );
				*/
			}

		}
	}

	/*
	if(keys->button_pressed || keys->button_released)
		printk("\nKey pressed = %d, released = %d\n",
		keys->button_pressed, keys->button_released);

	for( i = 0; i < 5; i++)
	    printk("Key[%d]=%x  ",i, tempBuffer[i]);
	printk("\n\n");
	*/

	/**
	 * TO DO : Read and return Special keys & dedicated keys
	 */

	return retval;
}
EXPORT_SYMBOL(stmpe1601_keypressed);

/**
 * stmpe1601_read_info() - read the chip information
 * This function read stmpe1601 chip and version ID
 * and returns error if chip id or version id is not correct.
 * This function can be called to check if UIB is connected or not.
 */
int stmpe1601_read_info()
{
	unsigned char chip_id, version_id;
	/* read the chip id and version id */
	if (!the_stmpe1601) /* STMPE1601 probe failed */
		return -EIO;
	stmpe1601_read_byte(the_stmpe1601, CHIP_ID_Index, &chip_id);
	stmpe1601_read_byte(the_stmpe1601, VERSION_ID_Index, &version_id);
	if ((chip_id != 0x2) || (version_id != 0x12)) {
		printk(KERN_ERR
			"stmpe1601: Incorrect chip id:%x version id:%x\n",
			chip_id, version_id);
		return -EIO;
	} else {
		printk(KERN_INFO
		       "stmpe1601 gpio expander: chip id: %x, version id:%x\n",
			chip_id, version_id);
	}
	return 0;
}
EXPORT_SYMBOL(stmpe1601_read_info);

/**
* stmpe1601_irqen() - enables corresponding interrupt mask
* @irq:		interrupt no.
**/
int stmpe1601_irqen(int irq)
{
	int ret = 0;
	unsigned char buf[5];
	if (irq == KEYPAD_INT) {
		/* printk(">> stmpe1601_irqen"); */
		mutex_lock(&the_stmpe1601->lock);
		/**
		 * enable keypad interrupt mask
		 */
		/**
		* Empty FIFO by reading all data bytes
		*/
		ret = stmpe1601_read(the_stmpe1601, KPC_DATA_BYTE0_Index,
				     buf, 5);
		stmpe1601_write_byte(the_stmpe1601, ISR_Lsb_Index,
				     (KPD_INT_MASK | KPD_OVERRUN_INT_MASK));
		stmpe1601_write_byte(the_stmpe1601, IER_Lsb_Index,
				     (KPD_INT_MASK | KPD_OVERRUN_INT_MASK));
		stmpe1601_write_byte(the_stmpe1601, ICR_Lsb_Index, 0x3);
		/* Falling edge sensitive irq
		 */
		mutex_unlock(&the_stmpe1601->lock);

	}
	return 0;
}
EXPORT_SYMBOL(stmpe1601_irqen);

/**
* stmpe1601_irqdis() - disables corresponding interrupt mask
* @irq:         interrupt no.
**/
int stmpe1601_irqdis(int irq)
{
	unsigned char intr = 0;
	if (irq == KEYPAD_INT) {
		/* printk(">> stmpe1601_irdis"); */
		mutex_lock(&the_stmpe1601->lock);
		/**
		 * disable keypad interrupt mask
		 */
		stmpe1601_read_byte(the_stmpe1601, IER_Lsb_Index, &intr);
		intr &= ~(KPD_INT_MASK | KPD_OVERRUN_INT_MASK);
		stmpe1601_write_byte(the_stmpe1601, IER_Lsb_Index, intr);
		mutex_unlock(&the_stmpe1601->lock);
	}
	return 0;
}
EXPORT_SYMBOL(stmpe1601_irqdis);

static int __exit stmpe1601_remove(struct i2c_client *client)
{
	struct stmpe1601_platform_data *pdata = client->dev.platform_data;
	/* TODO- implement the teardown, gpiochip_remove
	 */
	gpio_free(pdata->irq);
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const struct i2c_device_id stmpe1601_id[] = {
	{ "stmpe1601", 16 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, stmpe1601_id);

static struct i2c_driver stmpe1601_driver = {
	.driver = {
		.name = "stmpe1601",
		.owner = THIS_MODULE,
	},
	.probe 		= stmpe1601_probe,
	.remove 	= __exit_p(stmpe1601_remove),
	.id_table	= stmpe1601_id,
};

static int __init stmpe1601_init(void)
{
	return i2c_add_driver(&stmpe1601_driver);
}

module_init(stmpe1601_init);
static void __exit stmpe1601_exit(void)
{
	i2c_del_driver(&stmpe1601_driver);
}

module_exit(stmpe1601_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jayeeta banerjee");
MODULE_DESCRIPTION("stmpe1601 driver for Nomadik Platform");

