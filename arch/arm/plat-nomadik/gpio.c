/*
 * Generic GPIO driver for logic cells found in the Nomadik SoC
 *
 * Copyright (C) 2008,2009 STMicroelectronics
 * Copyright (C) 2009 Alessandro Rubini <rubini@unipv.it>
 *   Rewritten based on work by Prafulla WADASKAR <prafulla.wadaskar@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <mach/hardware.h>
#include <mach/gpio.h>

/*
 * The GPIO module in the Nomadik family of Systems-on-Chip is an
 * AMBA device, managing 32 pins and alternate functions.  The logic block
 * is currently only used in the Nomadik.
 *
 * Symbols in this file are called "nmk_gpio" for "nomadik gpio"
 */

struct nmk_gpio_chip {
	struct gpio_chip chip;
	void __iomem *addr;
	struct clk *clk;
	unsigned int parent_irq;
	spinlock_t lock;
	/* Keep track of configured edges */
	u32 edge_rising;
	u32 edge_falling;
	u32 backup[10];
};

/**
 * nmk_gpio_set_pull() - enable/disable pull up/down on a gpio
 * @gpio: pin number
 * @gpio_pull: one of NMK_GPIO_PULL_DOWN, NMK_GPIO_PULL_UP,
 * 	       and NMK_GPIO_PULL_NONE
 *
 * Enables/disables pull up/down on a specified pin.  This only takes effect if
 * the pin is configured as an input (either explicitly or by the alternate
 * function).
 *
 * NOTE: If enabling the pull up/down, the caller must ensure that the GPIO is
 * configured as an input.  Otherwise, due to the way the controller registers
 * work, this function will change the value output on the pin.
 */
int nmk_gpio_set_pull(int gpio, int gpio_pull)
{
	struct nmk_gpio_chip *nmk_chip;
	unsigned long flags;
	u32 pdis, bit;

	nmk_chip = get_irq_chip_data(NOMADIK_GPIO_TO_IRQ(gpio));
	if (!nmk_chip)
		return -EINVAL;

	bit = 1 << (gpio - nmk_chip->chip.base);

	spin_lock_irqsave(&nmk_chip->lock, flags);

	pdis = readl(nmk_chip->addr + NMK_GPIO_PDIS);
	if (gpio_pull == NMK_GPIO_PULL_NONE)
		pdis |= bit;
	else
		pdis &= ~bit;
	writel(pdis, nmk_chip->addr + NMK_GPIO_PDIS);

	if (gpio_pull == NMK_GPIO_PULL_UP)
		writel(bit, nmk_chip->addr + NMK_GPIO_DATS);
	else if (gpio_pull == NMK_GPIO_PULL_DOWN)
		writel(bit, nmk_chip->addr + NMK_GPIO_DATC);

	spin_unlock_irqrestore(&nmk_chip->lock, flags);

	return 0;
}
EXPORT_SYMBOL(nmk_gpio_set_pull);

/**
 * nmk_gpio_set_mode() - set the mux mode of a gpio pin
 * @gpio: pin number
 * @gpio_mode: one of NMK_GPIO_ALT_GPIO, NMK_GPIO_ALT_A,
 * 	       NMK_GPIO_ALT_B, and NMK_GPIO_ALT_C
 *
 * Sets the mode of the specified pin to one of the alternate functions or
 * plain GPIO.
 */
int nmk_gpio_set_mode(int gpio, int gpio_mode)
{
	struct nmk_gpio_chip *nmk_chip;
	unsigned long flags;
	u32 afunc, bfunc, bit;

	nmk_chip = get_irq_chip_data(NOMADIK_GPIO_TO_IRQ(gpio));
	if (!nmk_chip)
		return -EINVAL;

	bit = 1 << (gpio - nmk_chip->chip.base);

	spin_lock_irqsave(&nmk_chip->lock, flags);
	afunc = readl(nmk_chip->addr + NMK_GPIO_AFSLA) & ~bit;
	bfunc = readl(nmk_chip->addr + NMK_GPIO_AFSLB) & ~bit;
	if (gpio_mode & NMK_GPIO_ALT_A)
		afunc |= bit;
	if (gpio_mode & NMK_GPIO_ALT_B)
		bfunc |= bit;
	writel(afunc, nmk_chip->addr + NMK_GPIO_AFSLA);
	writel(bfunc, nmk_chip->addr + NMK_GPIO_AFSLB);
	spin_unlock_irqrestore(&nmk_chip->lock, flags);

	return 0;
}
EXPORT_SYMBOL(nmk_gpio_set_mode);

/**
 * nmk_gpio_get_mode() - get the mux mode of a gpio pin
 * @gpio: pin number
 *
 * Returns the mode of the specified pin.  One of NMK_GPIO_ALT_GPIO,
 * NMK_GPIO_ALT_A, NMK_GPIO_ALT_B, and NMK_GPIO_ALT_C.
 */
int nmk_gpio_get_mode(int gpio)
{
	struct nmk_gpio_chip *nmk_chip;
	u32 afunc, bfunc, bit;

	nmk_chip = get_irq_chip_data(NOMADIK_GPIO_TO_IRQ(gpio));
	if (!nmk_chip)
		return -EINVAL;

	bit = 1 << (gpio - nmk_chip->chip.base);

	afunc = readl(nmk_chip->addr + NMK_GPIO_AFSLA) & bit;
	bfunc = readl(nmk_chip->addr + NMK_GPIO_AFSLB) & bit;

	return (afunc ? NMK_GPIO_ALT_A : 0) | (bfunc ? NMK_GPIO_ALT_B : 0);
}
EXPORT_SYMBOL(nmk_gpio_get_mode);


/* IRQ functions */
static inline int nmk_gpio_get_bitmask(int gpio)
{
	return 1 << (gpio % 32);
}

static unsigned int nmk_gpio_irq_startup(unsigned int irq)
{
	int gpio = NOMADIK_IRQ_TO_GPIO(irq);
	int status = 0;

	status = gpio_request(gpio, "IRQ");
	if (status < 0) {
		printk(KERN_ERR "GPIO-request failed\n");
		return status;
	}

	gpio_direction_input(gpio);
	nmk_gpio_set_mode(gpio, NMK_GPIO_ALT_GPIO);

	/* go on as default_startup() */
	get_irq_chip(irq)->enable(irq);

	return 0;
}

static void nmk_gpio_irq_shutdown(unsigned int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);
	int gpio = NOMADIK_IRQ_TO_GPIO(irq);

	/* default_shutdown() */
	desc->chip->mask(irq);
	desc->status |= IRQ_MASKED;

	gpio_free(gpio);
}

static void nmk_gpio_irq_ack(unsigned int irq)
{
	int gpio;
	struct nmk_gpio_chip *nmk_chip;

	gpio = NOMADIK_IRQ_TO_GPIO(irq);
	nmk_chip = get_irq_chip_data(irq);
	if (!nmk_chip)
		return;
	writel(nmk_gpio_get_bitmask(gpio), nmk_chip->addr + NMK_GPIO_IC);
}

static int nmk_gpio_irq_modify(unsigned int irq, unsigned int rising,
			       unsigned int falling, bool enable)
{
	int gpio;
	struct nmk_gpio_chip *nmk_chip;
	unsigned long flags;
	u32 bitmask, reg;

	gpio = NOMADIK_IRQ_TO_GPIO(irq);
	nmk_chip = get_irq_chip_data(irq);
	bitmask = nmk_gpio_get_bitmask(gpio);
	if (!nmk_chip)
		return -EINVAL;

	/* we must individually clear the two edges */
	spin_lock_irqsave(&nmk_chip->lock, flags);
	if (nmk_chip->edge_rising & bitmask) {
		reg = readl(nmk_chip->addr + rising);
		if (enable)
			reg |= bitmask;
		else
			reg &= ~bitmask;
		writel(reg, nmk_chip->addr + rising);
	}
	if (nmk_chip->edge_falling & bitmask) {
		reg = readl(nmk_chip->addr + falling);
		if (enable)
			reg |= bitmask;
		else
			reg &= ~bitmask;
		writel(reg, nmk_chip->addr + falling);
	}
	spin_unlock_irqrestore(&nmk_chip->lock, flags);

	return 0;
}

static void nmk_gpio_irq_mask(unsigned int irq)
{
	nmk_gpio_irq_modify(irq, NMK_GPIO_RIMSC, NMK_GPIO_FIMSC, false);
}

static void nmk_gpio_irq_unmask(unsigned int irq)
{
	nmk_gpio_irq_modify(irq, NMK_GPIO_RIMSC, NMK_GPIO_FIMSC, true);
}

static int nmk_gpio_irq_set_wake(unsigned int irq, unsigned int on)
{
	return nmk_gpio_irq_modify(irq, NMK_GPIO_RWIMSC, NMK_GPIO_FWIMSC, on);
}

static int nmk_gpio_irq_set_type(unsigned int irq, unsigned int type)
{
	int gpio;
	struct nmk_gpio_chip *nmk_chip;
	struct irq_desc *desc;
	unsigned long flags;
	u32 bitmask;

	gpio = NOMADIK_IRQ_TO_GPIO(irq);
	nmk_chip = get_irq_chip_data(irq);
	bitmask = nmk_gpio_get_bitmask(gpio);
	if (!nmk_chip)
		return -EINVAL;

	if (type & IRQ_TYPE_LEVEL_HIGH)
		return -EINVAL;
	if (type & IRQ_TYPE_LEVEL_LOW)
		return -EINVAL;

	spin_lock_irqsave(&nmk_chip->lock, flags);

	nmk_chip->edge_rising &= ~bitmask;
	if (type & IRQ_TYPE_EDGE_RISING)
		nmk_chip->edge_rising |= bitmask;

	nmk_chip->edge_falling &= ~bitmask;
	if (type & IRQ_TYPE_EDGE_FALLING)
		nmk_chip->edge_falling |= bitmask;

	spin_unlock_irqrestore(&nmk_chip->lock, flags);

	desc = irq_to_desc(irq);
	if (!(desc->status & IRQ_DISABLED))
		nmk_gpio_irq_unmask(irq);

	return 0;
}

static struct irq_chip nmk_gpio_irq_chip = {
	.name		= "Nomadik-GPIO",
	.startup	= nmk_gpio_irq_startup,
	.shutdown	= nmk_gpio_irq_shutdown,
	.ack		= nmk_gpio_irq_ack,
	.mask		= nmk_gpio_irq_mask,
	.unmask		= nmk_gpio_irq_unmask,
	.set_type	= nmk_gpio_irq_set_type,
	.set_wake	= nmk_gpio_irq_set_wake,
};

static void nmk_gpio_irq_handler(unsigned int irq, struct irq_desc *desc)
{
	struct nmk_gpio_chip *nmk_chip;
	struct irq_chip *host_chip = get_irq_chip(irq);
	unsigned int gpio_irq;
	u32 pending;
	unsigned int first_irq;

	if (host_chip->mask_ack)
		host_chip->mask_ack(irq);
	else {
		host_chip->mask(irq);
		if (host_chip->ack)
			host_chip->ack(irq);
	}

	nmk_chip = get_irq_data(irq);
	first_irq = NOMADIK_GPIO_TO_IRQ(nmk_chip->chip.base);
	while ( (pending = readl(nmk_chip->addr + NMK_GPIO_IS)) ) {
		gpio_irq = first_irq + __ffs(pending);
		generic_handle_irq(gpio_irq);
	}

	host_chip->unmask(irq);
}

static int nmk_gpio_init_irq(struct nmk_gpio_chip *nmk_chip)
{
	unsigned int first_irq;
	int i;

	first_irq = NOMADIK_GPIO_TO_IRQ(nmk_chip->chip.base);
	for (i = first_irq; i < first_irq + nmk_chip->chip.ngpio; i++) {
		set_irq_chip(i, &nmk_gpio_irq_chip);
		set_irq_handler(i, handle_edge_irq);
		set_irq_flags(i, IRQF_VALID);
		set_irq_chip_data(i, nmk_chip);
		set_irq_type(i, IRQ_TYPE_EDGE_FALLING);
	}
	set_irq_chained_handler(nmk_chip->parent_irq, nmk_gpio_irq_handler);
	set_irq_data(nmk_chip->parent_irq, nmk_chip);
	return 0;
}

/* I/O Functions */
static int nmk_gpio_make_input(struct gpio_chip *chip, unsigned offset)
{
	struct nmk_gpio_chip *nmk_chip =
		container_of(chip, struct nmk_gpio_chip, chip);

	writel(1 << offset, nmk_chip->addr + NMK_GPIO_DIRC);
	return 0;
}

static int nmk_gpio_make_output(struct gpio_chip *chip, unsigned offset,
				int val)
{
	struct nmk_gpio_chip *nmk_chip =
		container_of(chip, struct nmk_gpio_chip, chip);

	writel(1 << offset, nmk_chip->addr + NMK_GPIO_DIRS);
	return 0;
}

static int nmk_gpio_get_input(struct gpio_chip *chip, unsigned offset)
{
	struct nmk_gpio_chip *nmk_chip =
		container_of(chip, struct nmk_gpio_chip, chip);
	u32 bit = 1 << offset;

	return (readl(nmk_chip->addr + NMK_GPIO_DAT) & bit) != 0;
}

static void nmk_gpio_set_output(struct gpio_chip *chip, unsigned offset,
				int val)
{
	struct nmk_gpio_chip *nmk_chip =
		container_of(chip, struct nmk_gpio_chip, chip);
	u32 bit = 1 << offset;

	if (val)
		writel(bit, nmk_chip->addr + NMK_GPIO_DATS);
	else
		writel(bit, nmk_chip->addr + NMK_GPIO_DATC);
}

#ifdef CONFIG_DEBUG_FS

#include <linux/seq_file.h>

static void nmk_gpio_dbg_show(struct seq_file *s, struct gpio_chip *chip)
{
	unsigned		i;
	unsigned		gpio = chip->base;
	int			is_out;
	struct nmk_gpio_chip *nmk_chip =
		container_of(chip, struct nmk_gpio_chip, chip);
	const char *modes[] = {
		[NMK_GPIO_ALT_GPIO]	= "gpio",
		[NMK_GPIO_ALT_A]	= "altA",
		[NMK_GPIO_ALT_B]	= "altB",
		[NMK_GPIO_ALT_C]	= "altC",
	};

	for (i = 0; i < chip->ngpio; i++, gpio++) {
		const char *label = gpiochip_is_requested(chip, i);
		bool pull;
		u32 bit = 1 << i;

		if (!label)
			continue;

		is_out = readl(nmk_chip->addr + NMK_GPIO_DIR) & bit;
		pull = !(readl(nmk_chip->addr + NMK_GPIO_PDIS) & bit);
		seq_printf(s, " gpio-%-3d (%-20.20s) %s %s %s %s",
			gpio, label,
			is_out ? "out" : "in ",
			chip->get
				? (chip->get(chip, i) ? "hi" : "lo")
				: "?  ",
			((nmk_gpio_get_mode(gpio)) < 0)	?
			"unknown" : modes[nmk_gpio_get_mode(gpio)],
			pull ? "pull" : "none");

		if (!is_out) {
			int		irq = gpio_to_irq(gpio);
			struct irq_desc	*desc = irq_to_desc(irq);

			/* This races with request_irq(), set_irq_type(),
			 * and set_irq_wake() ... but those are "rare".
			 *
			 * More significantly, trigger type flags aren't
			 * currently maintained by genirq.
			 */
			if (irq >= 0 && desc->action) {
				char *trigger;

				switch (desc->status & IRQ_TYPE_SENSE_MASK) {
				case IRQ_TYPE_NONE:
					trigger = "(default)";
					break;
				case IRQ_TYPE_EDGE_FALLING:
					trigger = "edge-falling";
					break;
				case IRQ_TYPE_EDGE_RISING:
					trigger = "edge-rising";
					break;
				case IRQ_TYPE_EDGE_BOTH:
					trigger = "edge-both";
					break;
				case IRQ_TYPE_LEVEL_HIGH:
					trigger = "level-high";
					break;
				case IRQ_TYPE_LEVEL_LOW:
					trigger = "level-low";
					break;
				default:
					trigger = "?trigger?";
					break;
				}

				seq_printf(s, " irq-%d %s%s",
					irq, trigger,
					(desc->status & IRQ_WAKEUP)
						? " wakeup" : "");
			}
		}

		seq_printf(s, "\n");
	}
}

#else
#define nmk_gpio_dbg_show	NULL
#endif

/* This structure is replicated for each GPIO block allocated at probe time */
static struct gpio_chip nmk_gpio_template = {
	.direction_input	= nmk_gpio_make_input,
	.get			= nmk_gpio_get_input,
	.direction_output	= nmk_gpio_make_output,
	.set			= nmk_gpio_set_output,
	.dbg_show		= nmk_gpio_dbg_show,
	.can_sleep		= 0,
};

static int __init nmk_gpio_probe(struct platform_device *dev)
{
	struct nmk_gpio_platform_data *pdata = dev->dev.platform_data;
	struct nmk_gpio_chip *nmk_chip;
	struct gpio_chip *chip;
	struct resource *res;
	int irq;
	int ret;
	struct clk *clk;

	if (!pdata)
		return -ENODEV;

	res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENOENT;
		goto out;
	}

	irq = platform_get_irq(dev, 0);
	if (irq < 0) {
		ret = irq;
		goto out;
	}

	if (request_mem_region(res->start, resource_size(res),
			       dev_name(&dev->dev)) == NULL) {
		ret = -EBUSY;
		goto out;
	}

	clk = clk_get(&dev->dev, NULL);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		goto out_release;
	}

	clk_enable(clk);

	nmk_chip = kzalloc(sizeof(*nmk_chip), GFP_KERNEL);
	if (!nmk_chip) {
		ret = -ENOMEM;
		goto out_clk;
	}
	/*
	 * The virt address in nmk_chip->addr is in the nomadik register space,
	 * so we can simply convert the resource address, without remapping
	 */
	nmk_chip->clk = clk;
	nmk_chip->addr = io_p2v(res->start);
	nmk_chip->chip = nmk_gpio_template;
	nmk_chip->parent_irq = irq;
	spin_lock_init(&nmk_chip->lock);

	chip = &nmk_chip->chip;
	chip->base = pdata->first_gpio;
	chip->ngpio = pdata->num_gpio;
	chip->label = pdata->name;
	chip->dev = &dev->dev;
	chip->owner = THIS_MODULE;

	ret = gpiochip_add(&nmk_chip->chip);
	if (ret)
		goto out_free;

	platform_set_drvdata(dev, nmk_chip);

	nmk_gpio_init_irq(nmk_chip);

	dev_info(&dev->dev, "Bits %i-%i at address %p\n",
		 nmk_chip->chip.base, nmk_chip->chip.base+31, nmk_chip->addr);
	return 0;

out_free:
	kfree(nmk_chip);
out_clk:
	clk_disable(clk);
	clk_put(clk);
out_release:
	release_mem_region(res->start, resource_size(res));
out:
	dev_err(&dev->dev, "Failure %i for GPIO %i-%i\n", ret,
		  pdata->first_gpio, pdata->first_gpio+31);
	return ret;
}

static int __exit nmk_gpio_remove(struct platform_device *dev)
{
	struct nmk_gpio_chip *nmk_chip;
	struct resource *res;

	res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!res)
		printk(KERN_ERR "IORESOURCE_MEM unavailable\n");

	nmk_chip = platform_get_drvdata(dev);
	gpiochip_remove(&nmk_chip->chip);
	clk_disable(nmk_chip->clk);
	clk_put(nmk_chip->clk);
	kfree(nmk_chip);
	if (res)
		release_mem_region(res->start, resource_size(res));

	return 0;
}

#ifdef CONFIG_PM
static int nmk_gpio_pm(struct platform_device *dev, bool suspend)
{
	struct nmk_gpio_chip *nmk_chip = platform_get_drvdata(dev);
	int i;
	static const unsigned int regs[] = {
		NMK_GPIO_DAT,
		NMK_GPIO_PDIS,
		NMK_GPIO_DIR,
		NMK_GPIO_AFSLA,
		NMK_GPIO_AFSLB,
		NMK_GPIO_SLPC,
		NMK_GPIO_RIMSC,
		NMK_GPIO_FIMSC,
		NMK_GPIO_RWIMSC,
		NMK_GPIO_FWIMSC,
	};

	BUILD_BUG_ON(ARRAY_SIZE(nmk_chip->backup) != ARRAY_SIZE(regs));

	/* XXX: is this sufficient? what about pull-up/down configuration? */

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		if (suspend)
			nmk_chip->backup[i] = readl(nmk_chip->addr + regs[i]);
		else
			writel(nmk_chip->backup[i], nmk_chip->addr + regs[i]);
	}

	return 0;
}

static int nmk_gpio_suspend(struct platform_device *dev, pm_message_t state)
{
	return nmk_gpio_pm(dev, true);
}

static int nmk_gpio_resume(struct platform_device *dev)
{
	return nmk_gpio_pm(dev, false);
}
#else
#define nmk_gpio_suspend	NULL
#define nmk_gpio_resume		NULL
#endif

static struct platform_driver nmk_gpio_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "gpio",
		},
	.probe = nmk_gpio_probe,
	.remove = __exit_p(nmk_gpio_remove),
	.suspend = nmk_gpio_suspend,
	.resume = nmk_gpio_resume,
};

static int __init nmk_gpio_init(void)
{
	return platform_driver_register(&nmk_gpio_driver);
}

arch_initcall(nmk_gpio_init);

MODULE_AUTHOR("Prafulla WADASKAR and Alessandro Rubini");
MODULE_DESCRIPTION("Nomadik GPIO Driver");
MODULE_LICENSE("GPL");


