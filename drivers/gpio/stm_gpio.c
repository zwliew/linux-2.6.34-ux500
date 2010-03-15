/*
 * Copyright (C) 2009 STMicroelectronics
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include <linux/amba/bus.h>
#include <mach/hardware.h>

/*
 * GPIO register map (offsets)
 */
#define GPIO_DAT		0x00	/* Data Register                     */
#define GPIO_DATS		0x04	/* Data Set Register                 */
#define GPIO_DATC		0x08	/* Data Clear Register               */
#define GPIO_PDIS		0x0C	/* Pull Disable Register             */
#define GPIO_DIR		0x10	/* Data Direction Register           */
#define GPIO_DIRS		0x14	/* Data Direction Set Register       */
#define GPIO_DIRC		0x18	/* Data Direction Clear Registe      */
#define GPIO_SLPM		0x1C	/* Sleep Mode Register               */
#define GPIO_AFSA		0x20	/* Alt Function A Select Register    */
#define GPIO_AFSB		0x24	/* Alt Function B Select Register    */
#define GPIO_RIMSC		0x40	/* Rising Edge Interrupt Set/Clear   */
#define GPIO_FIMSC		0x44	/* Falling Edge Interrupt Set/Clear  */
#define GPIO_IS			0x48	/* Interrupt Status Register         */
#define GPIO_IC			0x4C	/* Interrupt Clear Register          */
#define GPIO_RWIMSC 		0x50	/* Rising-edge Wakeup IMSC register  */
#define GPIO_FWIMSC     	0x54	/* Falling-edge Wakeup IMSC register */

/*
 * GPIO driver context
 */
struct stm_gpio {
	void __iomem *base;
	u16 irq;
	spinlock_t gpio_lock;
	struct gpio_chip chip;
	int blocks_per_irq;
	u32 saved_dat;
	u32 saved_pdis;
	u32 saved_dir;
	u32 saved_afsa;
	u32 saved_afsb;
	u32 saved_slpm;
	u32 saved_rimsc;
	u32 saved_fimsc;
	u32 saved_rwimsc;
	u32 saved_fwimsc;
/* Some Amba device can have multible gpio blocks */
	struct list_head gpio_list;
};

/*
 * This lock class tells lockdep that GPIO irqs are in a different
 * category than their parents, so it won't report false recursion
 */
static struct lock_class_key gpio_lock_class;

/*
 * The table of alternate interfaces must be common to all instances of
 * this driver, since a single interface can span multiple GPIO blocks
 */
static struct gpio_altfun_data *altfun_table;
static int altfun_table_size;

/*
 * When managing alternate interfaces this global lock must be used instead
 * of single gpio_s locks, since each interface can span multiple GPIO blocks
 */
static DEFINE_SPINLOCK(altfun_lock);

/*
 * Private functions
 */

static inline u32 gpio_get_regs_and_mask(unsigned int gpio,
					 void __iomem **regs)
{
	struct stm_gpio *gpio_s = get_irq_chip_data(GPIO_TO_IRQ(gpio));
	int offset = gpio - gpio_s->chip.base;

	BUG_ON(offset < 0 || offset > gpio_s->chip.ngpio);

	*regs = gpio_s->base;

	return BIT_MASK(offset);
}

/**
 * gpio_input - Configure GPIO in Input mode
 * @chip: GPIO chip
 * @offset: GPIO number
 *
 * It will configure the GPIO in input mode.
 * Return value <0 in case of error.
 * Return value =0 in case of success.
 **/
static int gpio_input(struct gpio_chip *chip, unsigned int offset)
{
	struct stm_gpio *gpio_s = container_of(chip, struct stm_gpio, chip);
	void __iomem *regs = gpio_s->base;
	unsigned long flags;
	u32 mask = BIT_MASK(offset);

	/* refuse to operate if the GPIO is allocated for an IRQ */
	if (irq_has_action(GPIO_TO_IRQ(gpio_s->chip.base + offset)))
		return -EBUSY;

	spin_lock_irqsave(&gpio_s->gpio_lock, flags);

	/* pull-up/down off, input, software mode, interrupts masked */
	writel(readl(regs + GPIO_PDIS) | mask, regs + GPIO_PDIS);
	writel(readl(regs + GPIO_AFSA) & ~mask, regs + GPIO_AFSA);
	writel(readl(regs + GPIO_AFSB) & ~mask, regs + GPIO_AFSB);
	writel(readl(regs + GPIO_RIMSC) & ~mask, regs + GPIO_RIMSC);
	writel(readl(regs + GPIO_FIMSC) & ~mask, regs + GPIO_FIMSC);
	writel(mask, regs + GPIO_DIRC);

	spin_unlock_irqrestore(&gpio_s->gpio_lock, flags);

	return 0;
}

/**
 * gpio_get - Returns status of GPIO pin
 * @chip: GPIO chip
 * @offset: GPIO number
 *
 * It will return the GPIO status (high/low).
 * GPIO must be configured in input mode to use this API.
 * Return value <0 in case of error.
 * Return value =0 in case of success.
 **/
static int gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct stm_gpio *gpio_s = container_of(chip, struct stm_gpio, chip);
	void __iomem *regs = gpio_s->base;
	u32 mask = BIT_MASK(offset), dat;

	dat = readl(regs + GPIO_DAT);

	return (dat & mask) ? GPIO_HIGH : GPIO_LOW;
}

/**
 * gpio_output - configure the GPIO in output mode.
 * @chip: GPIO chip
 * @offset: GPIO number
 * @value: Value to assign high/low
 *
 * It will configure the GPIO in output mode.
 * return error if pin is acquired as a interrupt pin
 **/
static int gpio_output(struct gpio_chip *chip, unsigned int offset, int value)
{
	struct stm_gpio *gpio_s = container_of(chip, struct stm_gpio, chip);
	void __iomem *regs = gpio_s->base;
	unsigned long flags;
	u32 mask = BIT_MASK(offset);

	/* refuse to operate if the GPIO is allocated for an IRQ */
	if (irq_has_action(GPIO_TO_IRQ(gpio_s->chip.base + offset)))
		return -EBUSY;

	spin_lock_irqsave(&gpio_s->gpio_lock, flags);

	/* pull-up/down off, output, software mode, interrupts masked */
	writel(readl(regs + GPIO_PDIS) | mask, regs + GPIO_PDIS);
	writel(readl(regs + GPIO_AFSA) & ~mask, regs + GPIO_AFSA);
	writel(readl(regs + GPIO_AFSB) & ~mask, regs + GPIO_AFSB);
	writel(readl(regs + GPIO_RIMSC) & ~mask, regs + GPIO_RIMSC);
	writel(readl(regs + GPIO_FIMSC) & ~mask, regs + GPIO_FIMSC);
	writel(mask, regs + GPIO_DIRS);

	if (value)
		writel(mask, regs + GPIO_DATS);
	else
		writel(mask, regs + GPIO_DATC);

	spin_unlock_irqrestore(&gpio_s->gpio_lock, flags);

	return 0;
}

/**
 * gpio_set - set a value for a GPIO pin
 * @chip: GPIO chip
 * @offset: GPIO number
 * @value: Value to be set for GPIO
 *
 * It will set given value to GPIO pin.
 * GPIO must be configured in output mode if GPIO_HIGH/GPIO_LOW
 * values to use.
 * for the other values gpio_request is sufficient.
 **/
static void gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct stm_gpio *gpio_s = container_of(chip, struct stm_gpio, chip);
	void __iomem *regs = gpio_s->base;
	unsigned long flags;
	u32 mask = BIT_MASK(offset);

	/* refuse to operate if the GPIO is allocated for an IRQ */
	if (irq_has_action(GPIO_TO_IRQ(gpio_s->chip.base + offset))) {
		dev_err(gpio_s->chip.dev,
			"Unable to set GPIO %d, busy as IRQ %d\n",
			gpio_s->chip.base + offset,
			GPIO_TO_IRQ(gpio_s->chip.base + offset));
		return;
	}

	spin_lock_irqsave(&gpio_s->gpio_lock, flags);

	switch (value) {
	case GPIO_LOW:
		writel(mask, regs + GPIO_DATC);
		break;
	case GPIO_HIGH:
		writel(mask, regs + GPIO_DATS);
		break;
	case GPIO_ALTF_A:
		/* pull-up/down off, alternate function A, interrupts masked */
		writel(readl(regs + GPIO_RIMSC) & ~mask, regs + GPIO_RIMSC);
		writel(readl(regs + GPIO_FIMSC) & ~mask, regs + GPIO_FIMSC);
		writel(readl(regs + GPIO_AFSA) | mask, regs + GPIO_AFSA);
		writel(readl(regs + GPIO_AFSB) & ~mask, regs + GPIO_AFSB);
		break;
	case GPIO_ALTF_B:
		/* pull-up/down off, alternate function B, interrupts masked */
		writel(readl(regs + GPIO_RIMSC) & ~mask, regs + GPIO_RIMSC);
		writel(readl(regs + GPIO_FIMSC) & ~mask, regs + GPIO_FIMSC);
		writel(readl(regs + GPIO_AFSA) & ~mask, regs + GPIO_AFSA);
		writel(readl(regs + GPIO_AFSB) | mask, regs + GPIO_AFSB);
		break;
	case GPIO_ALTF_C:
		/* pull-up/down off, alternate function C, interrupts masked */
		writel(readl(regs + GPIO_RIMSC) & ~mask, regs + GPIO_RIMSC);
		writel(readl(regs + GPIO_FIMSC) & ~mask, regs + GPIO_FIMSC);
		writel(readl(regs + GPIO_AFSA) | mask, regs + GPIO_AFSA);
		writel(readl(regs + GPIO_AFSB) | mask, regs + GPIO_AFSB);
		break;
	case GPIO_RESET:
		/* pull-up/down on, input, software mode, interrupts masked */
		writel(readl(regs + GPIO_PDIS) & ~mask, regs + GPIO_PDIS);
		writel(readl(regs + GPIO_RIMSC) & ~mask, regs + GPIO_RIMSC);
		writel(readl(regs + GPIO_FIMSC) & ~mask, regs + GPIO_FIMSC);
		writel(readl(regs + GPIO_AFSA) & ~mask, regs + GPIO_AFSA);
		writel(readl(regs + GPIO_AFSB) & ~mask, regs + GPIO_AFSB);
		writel(mask, regs + GPIO_DIRC);
		break;
	case GPIO_PULLUP_DIS:
		writel(readl(regs + GPIO_RIMSC) & ~mask, regs + GPIO_RIMSC);
		writel(readl(regs + GPIO_FIMSC) & ~mask, regs + GPIO_FIMSC);
		writel(readl(regs + GPIO_AFSA) & ~mask, regs + GPIO_AFSA);
		writel(readl(regs + GPIO_AFSB) & ~mask, regs + GPIO_AFSB);
		writel(readl(regs + GPIO_PDIS) & ~mask, regs + GPIO_PDIS);
		break;
	case GPIO_PULLUP_EN:
		writel(readl(regs + GPIO_RIMSC) & ~mask, regs + GPIO_RIMSC);
		writel(readl(regs + GPIO_FIMSC) & ~mask, regs + GPIO_FIMSC);
		writel(readl(regs + GPIO_AFSA) & ~mask, regs + GPIO_AFSA);
		writel(readl(regs + GPIO_AFSB) & ~mask, regs + GPIO_AFSB);
		writel(readl(regs + GPIO_PDIS) | mask, regs + GPIO_PDIS);
		break;
	default:
		printk(KERN_ERR "GPIO: Invalid set value for GPIO %d\n",
		       offset);
	}

	spin_unlock_irqrestore(&gpio_s->gpio_lock, flags);
}

/**
 * stm_gpio_altfuncenable - It enables the alternate functions
 * for the given set of GPIOs.
 * @alt_func: GPIO altfunction enum to disable
 * This function will enable the altfuntions specified for the enum alt_func
 * return error if pin is not available. but it will not free the GPIO
 * pins acquired before the errors, as its a serious error.
 *
 **/

int stm_gpio_altfuncenable(gpio_alt_function alt_func)
{
	unsigned long flags;
	int i, j, found = false;
	int start, end;

	spin_lock_irqsave(&altfun_lock, flags);
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
			spin_unlock_irqrestore(&altfun_lock, flags);
			return -EINVAL;
		}

		for (j = start; j <= end; j++) {
			/* refuse to operate if the GPIO is
			 * allocated for an IRQ */
			if (irq_has_action(GPIO_TO_IRQ(j))) {
				printk(KERN_ERR
				       "%s: Failed to set %s, GPIO"
				       "%d is busy as IRQ %d\n",
				       __func__, altfun_table[i].dev_name, j,
				       GPIO_TO_IRQ(j));
				spin_unlock_irqrestore(&altfun_lock, flags);
				return -EBUSY;
			}
			if (gpio_request(j, altfun_table[i].dev_name) < 0) {
				printk(KERN_ERR
				       "%s:Failed to set %s, GPIO %d is busy\n",
				       __func__, altfun_table[i].dev_name, j);
				spin_unlock_irqrestore(&altfun_lock, flags) ;
				return -EBUSY;
			}
			gpio_set_value(j, altfun_table[i].type);
		}
	}
	spin_unlock_irqrestore(&altfun_lock, flags);
	return 0;
}
EXPORT_SYMBOL(stm_gpio_altfuncenable);

/**
 * stm_gpio_altfuncdisable - It disables the alternate
 * functions for the given set of GPIOs.
 * @alt_func: GPIO altfunction enum
 * This function will disable the altfuntions specified for the enum alt_func.
 * Return value <0 in case of error.
 * Return value =0 in case of success.
 *
 **/
int stm_gpio_altfuncdisable(gpio_alt_function alt_func)
{
	int i, j, found = 0;
	int start, end;

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

		found = 1;

		if (!gpio_is_valid(start) || !gpio_is_valid(end)) {
			printk(KERN_ERR
			       "%s: GPIO range %d-%d for %s is out of bound\n",
			       __func__, start, end, altfun_table[i].dev_name);
			return -EINVAL;
		}

		for (j = start; j <= end; j++) {
			gpio_set_value(j, GPIO_RESET);
			gpio_free(j);
		}
	}

	if (!found)
		printk(KERN_ERR
		       "%s: Didn't find any GPIO alt interface named '%s'\n",
		       __func__, altfun_table[i].dev_name);

	return 0;
}
EXPORT_SYMBOL(stm_gpio_altfuncdisable);

/*
 * Interrupt handling functions
 */

/**
 * gpio_irq_startup - irq chip startup function for GPIO controller
 * @irq: irq number
 *
 * This function request the coresponding GPIO pin for irq
 * and configures it into interrupt mode.
 * If the pin is already acquired it frees the pin, throws
 * a warning and claims the pin in interrupt mode
 * We do not return error because error is not checked in
 * the caller function.
 *
 * So gpio_request is not required if GPIO pin needs to be used in
 * Interrupt mode as this function calls gpio_request internally.
 *
 **/
static unsigned int gpio_irq_startup(unsigned int irq)
{
	const char *owner_name;
	struct stm_gpio *gpio_s = get_irq_chip_data(irq);
	int offset = IRQ_TO_GPIO(irq) - gpio_s->chip.base;
	int status = 0;

	void __iomem *regs;
	u32 mask = gpio_get_regs_and_mask(IRQ_TO_GPIO(irq), &regs);

	BUG_ON(offset < 0 || offset > gpio_s->chip.ngpio);

	/* check if the IRQ line is already allocated as GPIO */
	owner_name = gpiochip_is_requested(&gpio_s->chip, offset);
	if (owner_name) {
		/* no way to be fair, warn and take over the GPIO */
		dev_err(gpio_s->chip.dev,
			"IRQ %d was already allocated as GPIO %d\n", irq,
			IRQ_TO_GPIO(irq));
		gpio_set_value(IRQ_TO_GPIO(irq), GPIO_RESET);
		gpio_free(IRQ_TO_GPIO(irq));
	}

	/* allocate GPIO line used for this IRQ */
	status = gpio_request(IRQ_TO_GPIO(irq), "IRQ");
	if (status < 0) {
		printk(KERN_ERR "failed to get the requested gpio\n");
		return status;
	}

	/* pull-up/down off, input, software mode, interrupts masked */
	writel(readl(regs + GPIO_AFSA) & ~mask, regs + GPIO_AFSA);
	writel(readl(regs + GPIO_AFSB) & ~mask, regs + GPIO_AFSB);
	writel(readl(regs + GPIO_RIMSC) & ~mask, regs + GPIO_RIMSC);
	writel(readl(regs + GPIO_FIMSC) & ~mask, regs + GPIO_FIMSC);
	writel(mask, regs + GPIO_DIRC);

	/* go on as default_startup() */
	get_irq_chip(irq)->enable(irq);
	return 0;
}

/**
 * gpio_irq_shutdown - irq chip shutdown function for GPIO controller
 * @irq: irq number
 *
 * This function frees the coresponding GPIO pin for irq
 * and configures it into reset statee.
 *
 * So gpio_free is not required after free_irq in
 * Interrupt mode as this function calls gpio_free internally.
 *
 **/
static void gpio_irq_shutdown(unsigned int irq)
{
	struct irq_chip *chip = get_irq_chip(irq);

	/* start as default_shutdown() */
	chip->mask(irq);
	irq_desc[irq].status |= IRQ_MASKED;

	/* then free the gpio */
	gpio_set_value(IRQ_TO_GPIO(irq), GPIO_RESET);
	gpio_free(IRQ_TO_GPIO(irq));
}

/**
 * gpio_irq_ack - irq chip ack function for GPIO controller
 * @irq: irq number
 *
 * This function clear the interrupt for the GPIO pin.
 *
 **/

static void gpio_irq_ack(unsigned int irq)
{
	void __iomem *regs;
	u32 mask = gpio_get_regs_and_mask(IRQ_TO_GPIO(irq), &regs);

	writel(mask, regs + GPIO_IC);
}

/**
 * gpio_irq_mask - irq chip mask function for GPIO controller
 * @irq: irq number
 *
 * This function masks the interrupt for the GPIO pin.
 *
 **/
static void gpio_irq_mask(unsigned int irq)
{
	void __iomem *regs;
	u32 mask = gpio_get_regs_and_mask(IRQ_TO_GPIO(irq), &regs);

	writel(readl(regs + GPIO_RIMSC) & ~mask, regs + GPIO_RIMSC);
	writel(readl(regs + GPIO_FIMSC) & ~mask, regs + GPIO_FIMSC);
}

/**
 * gpio_irq_unmask - irq chip unmask function for GPIO controller
 * @irq: irq number
 *
 * This function unmasks the interrupt for the GPIO pin.
 *
 **/
static void gpio_irq_unmask(unsigned int irq)
{
	void __iomem *regs;
	u32 mask = gpio_get_regs_and_mask(IRQ_TO_GPIO(irq), &regs);

	switch ((u32) get_irq_data(irq) & IRQ_TYPE_SENSE_MASK) {
	case IRQ_TYPE_NONE:
	case IRQ_TYPE_EDGE_RISING:
		writel(readl(regs + GPIO_RIMSC) | mask, regs + GPIO_RIMSC);
		writel(readl(regs + GPIO_FIMSC) & ~mask, regs + GPIO_FIMSC);
		break;
	case IRQ_TYPE_EDGE_FALLING:
		writel(readl(regs + GPIO_RIMSC) & ~mask, regs + GPIO_RIMSC);
		writel(readl(regs + GPIO_FIMSC) | mask, regs + GPIO_FIMSC);
		break;
	case IRQ_TYPE_EDGE_BOTH:
		writel(readl(regs + GPIO_RIMSC) | mask, regs + GPIO_RIMSC);
		writel(readl(regs + GPIO_FIMSC) | mask, regs + GPIO_FIMSC);
		break;
	}
}

/**
 * gpio_irq_set_type - irq chip set_type function for GPIO controller
 * @irq: irq number
 *
 * This function sets the type of the interrupt for the GPIO pin.
 *
 **/
static int gpio_irq_set_type(unsigned int irq, unsigned int type)
{
	struct irq_desc *desc = irq_desc + irq;

	/* filter input for valid types */
	if (type & ~IRQ_TYPE_SENSE_MASK)
		return -EINVAL;

	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
	case IRQ_TYPE_EDGE_FALLING:
	case IRQ_TYPE_EDGE_BOTH:
		/*
		 * sensing type selection and interrupt enabling is done writing
		 * into the same register bits,hence save the type now and delay
		 * register programming to interrupt unmask time
		 */
		irq_desc[irq].handle_irq = handle_edge_irq;
		irq_desc[irq].handler_data = (void *)type;
		break;
	case IRQ_TYPE_NONE:
	case IRQ_TYPE_LEVEL_HIGH:
	case IRQ_TYPE_LEVEL_LOW:
		irq_desc[irq].handle_irq = handle_level_irq;
		irq_desc[irq].handler_data = (void *)type;
		break;
		/* level sensing is not supported by our GPIO hardware */
		return -EINVAL;
	}

	/*
	 * if the interrupt is already enabled, don't delay and reprogram
	 * registers immediately for the new sensing type
	 */
	if (!(desc->status & IRQ_DISABLED))
		gpio_irq_unmask(irq);

	return 0;
}

/**
 * gpio_intrwake - Interrupt wakeup function for GPIO controller
 * @irq: irq number
 *
 * This function wakesup the system from sleep mode.
 *
 **/
static int gpio_intrwake(unsigned int irq, unsigned int flag)
{
	int ret = 0;
	void __iomem *regs;
	u32 mask = gpio_get_regs_and_mask(IRQ_TO_GPIO(irq), &regs);
	unsigned int type = (unsigned int)get_irq_data(irq);
	if (!flag) {
		writel(readl(regs + GPIO_RWIMSC) & ~mask, regs + GPIO_RWIMSC);
		writel(readl(regs + GPIO_FWIMSC) & ~mask, regs + GPIO_FWIMSC);
	} else {
		switch (type & IRQF_TRIGGER_MASK) {
		case IRQF_TRIGGER_RISING:
			writel(readl(regs + GPIO_RWIMSC) | mask,
			       regs + GPIO_RWIMSC);
			writel(readl(regs + GPIO_FWIMSC) &
			       ~mask, regs + GPIO_FWIMSC);
			break;
		case IRQF_TRIGGER_FALLING:
			writel(readl(regs + GPIO_RWIMSC) &
			       ~mask, regs + GPIO_RWIMSC);
			writel(readl(regs + GPIO_FWIMSC) | mask,
			       regs + GPIO_FWIMSC);
			break;
		case (IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING):
			writel(readl(regs + GPIO_RWIMSC) | mask,
			       regs + GPIO_RWIMSC);
			writel(readl(regs + GPIO_FWIMSC) | mask,
			       regs + GPIO_FWIMSC);
			break;
		case IRQF_TRIGGER_LOW:
		case IRQF_TRIGGER_HIGH:
			printk("wakeup mode %d not supported", (int)type);
			ret = -EINVAL;
			break;
		default:	/* used to disable wakeup */
			break;
		}
	}
	return ret;
}

/*
 * GPIO irq chip structure
 */
static struct irq_chip gpio_irq_chip = {
	.name = "GPIO",
	.startup = gpio_irq_startup,
	.shutdown = gpio_irq_shutdown,
	.ack = gpio_irq_ack,
	.mask = gpio_irq_mask,
	.unmask = gpio_irq_unmask,
	.set_type = gpio_irq_set_type,
	.set_wake = gpio_intrwake,
};

/**
 * gpio_irq_handler - GPIO irq handler
 * @irq: base irq
 * desc: irq descriptor for irq
 *
 * This is a GPIO block irq handler.
 * It identifies the actual gpio pin within the block
 * and calls the corresponding interrupt handler.
 **/
static void gpio_irq_handler(u32 irq, struct irq_desc *desc)
{
	struct stm_gpio *gpio_s = get_irq_data(irq);
	void __iomem *regs;
	unsigned int gpio_irq;
	u32 pending, i;
	unsigned int first_irq;
	int gpio_blocks = gpio_s->blocks_per_irq;
	BUG_ON(gpio_s->irq != irq);
	if (desc->chip->mask_ack)
		desc->chip->mask_ack(irq);
	else {
		desc->chip->mask(irq);
		desc->chip->ack(irq);
	}

	/* redirect to first pin of GPIO gpio_s which sent this IRQ */

	for (i = 0; i < gpio_blocks; i++) {
		first_irq = GPIO_TO_IRQ(gpio_s->chip.base);
		regs = gpio_s->base;
		while ((pending = readl(regs + GPIO_IS))) {
			gpio_irq = first_irq + __ffs(pending);
			generic_handle_irq(gpio_irq);
		}
		gpio_s =
		    list_first_entry(&gpio_s->gpio_list, struct stm_gpio,
				     gpio_list);
	}
	if (!(desc->status & IRQ_DISABLED) && desc->chip->unmask)
		desc->chip->unmask(irq);
}

/**
 * gpio_probe - GPIO probe function
 * @dev: Amba device structure
 * @id: void pointer to id
 *
 * This function allocates and initialises various data structures.
 * It alse registers gpio_chip structure with GPIOLIB framework.
 *
 **/
static int gpio_probe(struct amba_device *dev, struct amba_id *id)
{
	struct gpio_platform_data *plat = dev->dev.platform_data;
	struct gpio_block_data *gpio_block_data;
	int i, irq_start, irq_end, ret;
	int gpio_block;
	struct stm_gpio *gpio_s = NULL;
	struct list_head *gpio_list_head;
	void __iomem *base;
	int prev_irq = 0;
	int err;

	/* must have platform data */
	if (!plat) {
		dev_err(&dev->dev, "missing platform data for %p\n", id);
		return -EINVAL;
	}

	gpio_block_data = plat->gpio_data;

	/* sanity check on configuration parameters */

	ret = amba_request_regions(dev, NULL);
	if (ret < 0)
		return -EBUSY;

	base = ioremap(dev->res.start, dev->res.end - dev->res.start + 1);
	if (!base) {
		err = -ENODEV;
		goto free_region;
	}

	plat->clk = clk_get(&dev->dev, NULL);
	if (IS_ERR(plat->clk)) {
		dev_err(&dev->dev, "unable to get clock");
		err = PTR_ERR(plat->clk);
		goto iounmap;
	}

	clk_enable(plat->clk);

	gpio_list_head = kzalloc(sizeof(struct list_head), GFP_KERNEL);
	if (!gpio_list_head) {
		err = -ENOMEM;
		goto free_clk;
	}

	INIT_LIST_HEAD(gpio_list_head);

	amba_set_drvdata(dev, gpio_list_head);

	for (gpio_block = 0; gpio_block < plat->gpio_block_size; gpio_block++) {
		gpio_s = kzalloc(sizeof(struct stm_gpio), GFP_KERNEL);
		if (gpio_s == NULL) {
			dev_err(&dev->dev, "%s, not enough memory\n", __func__);
			amba_release_regions(dev);
			return -ENOMEM;
		}
		list_add_tail(&gpio_s->gpio_list, gpio_list_head);
		/* set up global alternate functions table */
		if (altfun_table == NULL) {
			altfun_table = plat->altfun_table;
		} else if (altfun_table != plat->altfun_table) {
			dev_warn(&dev->dev,
				 "table must be common to all GPIO blocks\n");
		}

		if (altfun_table_size == 0) {
			altfun_table_size = plat->altfun_table_size;
		} else if (altfun_table_size != plat->altfun_table_size) {
			dev_warn(&dev->dev,
				 "table_size must be common"
				 " to all GPIO blocks\n");
		}

		iounmap(gpio_s->base);
		/* initialize stm_gpio */
		gpio_s->base = base + gpio_block_data[gpio_block].base_offset;
		gpio_s->irq = gpio_block_data[gpio_block].irq;

		spin_lock_init(&gpio_s->gpio_lock);

		gpio_s->chip.label = dev->dev.bus_id;
		gpio_s->chip.dev = &dev->dev;
		gpio_s->chip.owner = THIS_MODULE;
		gpio_s->chip.direction_input = gpio_input;
		gpio_s->chip.get = gpio_get;
		gpio_s->chip.direction_output = gpio_output;
		gpio_s->chip.set = gpio_set;
		gpio_s->chip.base = gpio_block_data[gpio_block].block_base;
		gpio_s->chip.ngpio = gpio_block_data[gpio_block].block_size;
		gpio_s->blocks_per_irq =
		    gpio_block_data[gpio_block].blocks_per_irq;

		BUG_ON(gpiochip_add(&gpio_s->chip) < 0);

		irq_start = GPIO_TO_IRQ(gpio_s->chip.base);
		irq_end = irq_start + gpio_s->chip.ngpio;

		for (i = irq_start; i < irq_end; i++) {
			lockdep_set_class(&irq_desc[i].lock, &gpio_lock_class);
			BUG_ON(set_irq_chip(i, &gpio_irq_chip) < 0);
			BUG_ON(set_irq_data(i, NULL) < 0);
			BUG_ON(set_irq_chip_data(i, gpio_s) < 0);
			set_irq_handler(i, handle_level_irq);
			set_irq_flags(i, IRQF_VALID);
		}
		if (prev_irq != gpio_s->irq) {
			set_irq_data(gpio_s->irq, gpio_s);
			set_irq_chained_handler(gpio_s->irq, gpio_irq_handler);
		}

		prev_irq = gpio_s->irq;
	}

	printk(KERN_INFO "started at 0x%p, IRQ %d\n", gpio_s->base,
	       gpio_s->irq);
	return 0;

free_clk:
	clk_disable(plat->clk);
	clk_put(plat->clk);
iounmap:
	iounmap(base);
free_region:
	amba_release_regions(dev);
	return err;
}

/**
 * gpio_remove - GPIO remove function
 * @dev: Amba device structure
 *
 * This function deallocates the various data structures.
 * It alse remove gpio_chip structure from GPIOLIB framework.
 *
 **/
static int gpio_remove(struct amba_device *dev)
{
	struct gpio_platform_data *plat = dev->dev.platform_data;
	int i, irq_start, irq_end;
	struct stm_gpio *gpio_s;
	struct list_head *gpio_list_head, *gpio_list_node;
	void __iomem *reg_base;
	gpio_list_head = amba_get_drvdata(dev);
	gpio_s = list_first_entry(gpio_list_head, struct stm_gpio, gpio_list);
	reg_base = gpio_s->base;
	amba_release_regions(dev);
	list_for_each(gpio_list_node, gpio_list_head) {
		gpio_s =
		    container_of(gpio_list_node, struct stm_gpio, gpio_list);
		set_irq_data(gpio_s->irq, NULL);
		set_irq_chip_data(gpio_s->irq, NULL);
		set_irq_chained_handler(gpio_s->irq, NULL);

		irq_start = GPIO_TO_IRQ(gpio_s->chip.base);
		irq_end = irq_start + gpio_s->chip.ngpio;

		for (i = irq_start; i < irq_end; i++) {
			set_irq_chip(i, NULL);
			set_irq_data(i, NULL);
			set_irq_chip_data(i, NULL);
			set_irq_handler(i, NULL);
			set_irq_flags(i, 0);
		}

		if (gpiochip_remove(&gpio_s->chip) < 0) {
			dev_err(&dev->dev,
				"failed to remove %s, some GPIO still busy\n",
				gpio_s->chip.label);
			return -EBUSY;
		}
		kfree(gpio_s);
	}
	iounmap(reg_base);
	clk_disable(plat->clk);
	clk_put(plat->clk);
	kfree(gpio_list_head);
	printk(KERN_INFO "module removed\n");
	return 0;
}

#ifdef CONFIG_PM
/**
 * gpio_suspend - GPIO suspend function
 * @dev: Amba device structure
 * @state: Power state
 *
 * This function saves the state of GPIO controller registers in
 * to the memory.
 *
 **/

static int gpio_suspend(struct amba_device *dev, pm_message_t state)
{
	struct list_head *gpio_list_head, *gpio_list_node;
	gpio_list_head = amba_get_drvdata(dev);
	list_for_each(gpio_list_node, gpio_list_head) {
		struct stm_gpio *gpio_s =
		    container_of(gpio_list_node, struct stm_gpio, gpio_list);
		void __iomem *regs = gpio_s->base;

		gpio_s->saved_dat = readl(regs + GPIO_DAT);
		gpio_s->saved_pdis = readl(regs + GPIO_PDIS);
		gpio_s->saved_dir = readl(regs + GPIO_DIR);
		gpio_s->saved_afsa = readl(regs + GPIO_AFSA);
		gpio_s->saved_afsb = readl(regs + GPIO_AFSB);
		gpio_s->saved_slpm = readl(regs + GPIO_SLPM);
		gpio_s->saved_rimsc = readl(regs + GPIO_RIMSC);
		gpio_s->saved_fimsc = readl(regs + GPIO_FIMSC);
		gpio_s->saved_rwimsc = readl(regs + GPIO_RWIMSC);
		gpio_s->saved_fwimsc = readl(regs + GPIO_FWIMSC);
	}
	return 0;
}

/**
 * gpio_resume - GPIO resume function
 * @dev: Amba device structure
 *
 * This function retore the state of GPIO controller registers from
 * memory.
 *
 **/
static int gpio_resume(struct amba_device *dev)
{
	struct list_head *gpio_list_head, *gpio_list_node;
	gpio_list_head = amba_get_drvdata(dev);
	list_for_each(gpio_list_node, gpio_list_head) {
		struct stm_gpio *gpio_s =
		    container_of(gpio_list_node, struct stm_gpio, gpio_list);
		void __iomem *regs = gpio_s->base;
		writel(gpio_s->saved_dat, regs + GPIO_DAT);
		writel(gpio_s->saved_pdis, regs + GPIO_PDIS);
		writel(gpio_s->saved_dir, regs + GPIO_DIR);
		writel(gpio_s->saved_afsa, regs + GPIO_AFSA);
		writel(gpio_s->saved_afsb, regs + GPIO_AFSB);
		writel(gpio_s->saved_slpm, regs + GPIO_SLPM);
		writel(gpio_s->saved_rimsc, regs + GPIO_RIMSC);
		writel(gpio_s->saved_fimsc, regs + GPIO_FIMSC);
		writel(gpio_s->saved_rwimsc, regs + GPIO_RWIMSC);
		writel(gpio_s->saved_fwimsc, regs + GPIO_FWIMSC);
	}
	return 0;
}
#else
#define gpio_suspend	NULL
#define gpio_resume		NULL
#endif

static struct amba_id gpio_ids[] = {
	{
	 .id = GPIO_PER_ID,
	 .mask = GPIO_PER_MASK,
	 },
	{0, 0},
};

static struct amba_driver gpio_driver = {
	.drv = {
		.name = "GPIO-PL060",
		.owner = THIS_MODULE,
		},
	.probe = gpio_probe,
	.remove = gpio_remove,
	.suspend = gpio_suspend,
	.resume = gpio_resume,
	.id_table = gpio_ids,
};

static int __init gpio_init(void)
{
	return amba_driver_register(&gpio_driver);
}

static void __exit gpio_exit(void)
{
	amba_driver_unregister(&gpio_driver);
}

arch_initcall(gpio_init);
module_exit(gpio_exit);

MODULE_AUTHOR("STMicroelectronics");
MODULE_DESCRIPTION("STM GPIO driver");
MODULE_LICENSE("GPL");
