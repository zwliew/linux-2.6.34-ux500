/*
 * Copyright ST-Ericsson 2009.
 *
 * Author: Srinidhi Kasagar <srinidhi.kasagar@stericsson.com>
 * Licensed under GPLv2.
 */

/*
 * TODO: The driver doesn't follow the std. f/w and thus doesn't
 * populate the client devices. The driver needs migration to the below
 * mainelined version.
 * http://git.kernel.org/?p=linux/kernel/git/sameo/mfd-2.6.git;
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include <mach/dma.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-stm.h>
#include <linux/device.h>
#include <linux/spi/stm_ssp.h>
#include <mach/ab8500.h>

/*
 * static data definitions
 */
static void ab8500_work(struct work_struct *work);
static struct ab8500 *ab8500;
static struct platform_driver ab8500_driver;

static unsigned int s_regs[] = {
	AB8500_IT_SOURCE1_REG,
	AB8500_IT_SOURCE2_REG,
	AB8500_IT_SOURCE3_REG,
	AB8500_IT_SOURCE4_REG,
	AB8500_IT_SOURCE5_REG,
	AB8500_IT_SOURCE6_REG,
	AB8500_IT_SOURCE7_REG,
	AB8500_IT_SOURCE8_REG,
	AB8500_IT_SOURCE19_REG,
	AB8500_IT_SOURCE20_REG,
	AB8500_IT_SOURCE21_REG,
	AB8500_IT_SOURCE22_REG,
	AB8500_IT_SOURCE23_REG,
	AB8500_IT_SOURCE24_REG
};

static unsigned int l_regs[] = {
	AB8500_IT_LATCH1_REG,
	AB8500_IT_LATCH2_REG,
	AB8500_IT_LATCH3_REG,
	AB8500_IT_LATCH4_REG,
	AB8500_IT_LATCH5_REG,
	AB8500_IT_LATCH6_REG,
	AB8500_IT_LATCH7_REG,
	AB8500_IT_LATCH8_REG,
	AB8500_IT_LATCH9_REG,
	AB8500_IT_LATCH10_REG,
	AB8500_IT_LATCH19_REG,
	AB8500_IT_LATCH20_REG,
	AB8500_IT_LATCH21_REG,
	AB8500_IT_LATCH22_REG,
	AB8500_IT_LATCH23_REG,
	AB8500_IT_LATCH24_REG
};

static unsigned int m_regs[] = {
	AB8500_IT_MASK1_REG,
	AB8500_IT_MASK2_REG,
	AB8500_IT_MASK3_REG,
	AB8500_IT_MASK4_REG,
	AB8500_IT_MASK5_REG,
	AB8500_IT_MASK6_REG,
	AB8500_IT_MASK7_REG,
	AB8500_IT_MASK8_REG,
	AB8500_IT_MASK9_REG,
	AB8500_IT_MASK10_REG,
	AB8500_IT_MASK11_REG,
	AB8500_IT_MASK12_REG,
	AB8500_IT_MASK13_REG,
	AB8500_IT_MASK14_REG,
	AB8500_IT_MASK15_REG,
	AB8500_IT_MASK16_REG,
	AB8500_IT_MASK17_REG,
	AB8500_IT_MASK18_REG,
	AB8500_IT_MASK19_REG,
	AB8500_IT_MASK20_REG,
	AB8500_IT_MASK21_REG,
	AB8500_IT_MASK22_REG,
	AB8500_IT_MASK23_REG,
	AB8500_IT_MASK24_REG
};

/*
 * work queue for interrupt processing
 */
static struct work_struct ab8500_handler;
static struct workqueue_struct *ab8500_wq;

/**
 *  ab8500_spi_cs_control() - callback function for ssp
 *  @command:	flag decides chip select/deselect operation
 *  @return void
 */
static void ab8500_spi_cs_control(u32 command)
{
	if (!ab8500->board)
		return;

	if (SPI_CHIP_SELECT != command) {
		if (!ab8500->board->cs_dis)
			ab8500->board->cs_dis();

	} else {
		if (!ab8500->board->cs_en)
			ab8500->board->cs_en();

	}
}

/**
 * is_intno_invalid() - to check the validity of client interrupt number
 * @int_no:	client interrupt number whose validity needs to be checked
 *
 * This funtion returns	-1 if int_no is invalid and 0 if valid
 */
int is_intid_invalid(int int_no)
{
	if ((int_no < 0) || (int_no > (AB8500_MAX_INT)))
		return -1;
	else
		return 0;
}

/**
 * ab8500_int_mask() -  mask the interrupt
 * @int_no:	interrupt number to be masked
 *
 * This funtion calculate the bit position to be masked based on int_no
 * and write back the value to the register. This shouldn't be called from
 * interrupt context.
 */
void ab8500_int_mask(int int_no)
{
	/*TODO - Not required? */
}
EXPORT_SYMBOL(ab8500_int_mask);

/**
 * ab8500_int_unmask() -  unmask the interrupt
 * @int_no:	interrupt number to be unmasked
 *
 * This funtion calculate the bit position to be unmasked based on int_no
 * and write back the value to the register. Thios shouldn't be called from
 * the interrupt context.
 */
void ab8500_int_unmask(int int_no)
{
	/*TODO- Not required? */
}
EXPORT_SYMBOL(ab8500_int_unmask);

/**
 * ab8500_get_version() -  get version of STw8500
 * Returns: The version of STw8500
 *
 * This funtion can be used to get the version/revision of AB8500
 */
int ab8500_get_version(void)
{
	return ab8500->revision;
}
EXPORT_SYMBOL(ab8500_get_version);

/**
 * ab8500_set_callback_handler() - callback handler for AB8500
 * @int_no:	Interrupt number
 * @callback_handler:	callback handler to be installed.
 * @data:	any data to be passed to the callback handler
 *
 * This funtion sets the client's callback handler in the bit map
 * array.
 */
int ab8500_set_callback_handler(int int_no, void *callback_handler,
				 void *data)
{
	unsigned long flags;
	struct client_callbacks *client_callback;

	/* TODO: Need to remove this once the clients start
	 * registering to the ab8500 core driver
	 */
	if (!ab8500)
		return -ENODEV;

	if (is_intid_invalid(int_no)) {
		printk(KERN_ERR "invalid interrupt id\n");
		return -1;
	}

	client_callback = kzalloc(sizeof(struct client_callbacks), GFP_KERNEL);
	if (!client_callback) {
		printk(KERN_INFO "\nUnable to allocate client_callbacks\n");
		kfree(client_callback);
		return -1;
	}

	spin_lock_irqsave(&ab8500->ab8500_cfg_lock, flags);
	client_callback->callback = callback_handler;
	client_callback->data = data;
	list_add_tail(&(client_callback->client_list),
			&(ab8500->c_callback[int_no]).client_list);
	spin_unlock_irqrestore(&ab8500->ab8500_cfg_lock, flags);

	return 0;
}
EXPORT_SYMBOL(ab8500_set_callback_handler);

/**
 * ab8500_remove_callback_handler() - To remove the call back handler
 * @int_no:	interrupt number
 *
 * This funtion removes the client's callback handler in the bit map
 * array. This funtion return 0 on success or -1 on failure
 */
int ab8500_remove_callback_handler(int int_no, void *callback_handler)
{
	unsigned long flags;
	struct list_head *cur_list_ptr = NULL;
	struct list_head *q = NULL;
	struct client_callbacks *cur_ptr = NULL;

	/* TODO: Need to remove this once the clients start
	 * registering to the ab8500 core driver
	 */
	if (!ab8500)
		return -ENODEV;

	if (is_intid_invalid(int_no)) {
		printk(KERN_ERR "invalid interrupt id\n");
		return -1;
	}
	spin_lock_irqsave(&ab8500->ab8500_cfg_lock, flags);
	list_for_each_safe(cur_list_ptr, q,
			&(ab8500->c_callback[int_no]).client_list) {
		cur_ptr = list_entry(cur_list_ptr, \
				struct client_callbacks, \
				client_list);
		if (cur_ptr->callback == callback_handler) {
			list_del(cur_list_ptr);
			kfree(cur_ptr);
		}
	}
	spin_unlock_irqrestore(&ab8500->ab8500_cfg_lock, flags);
	return 0;
}
EXPORT_SYMBOL(ab8500_remove_callback_handler);

/**
 * ab8500_set_signal_handler - To set signal handler
 * @int_no:	interrupt number
 * @sig_no:	signal number
 *
 * This function sets the signal number to be sent to a process
 * for a particular interrupt
 */
int ab8500_set_signal_handler(int int_no, int sig_no)
{
	unsigned long flags;

	/* TODO: Need to remove this once the clients start
	 * registering to the ab8500 core driver
	 */
	if (!ab8500)
		return -ENODEV;

	if (is_intid_invalid(int_no)) {
		printk(KERN_ERR "invalid interrupt id\n");
		return -1;
	}

	spin_lock_irqsave(&ab8500->ab8500_cfgsig_lock, flags);
	(ab8500->c_signals[int_no]).pid = get_task_pid(current, PIDTYPE_PID);
	(ab8500->c_signals[int_no]).signal = sig_no;
	spin_unlock_irqrestore(&ab8500->ab8500_cfgsig_lock, flags);
	return 0;
}
EXPORT_SYMBOL(ab8500_set_signal_handler);


int ab8500_remove_signal_handler(int int_no)
{
	unsigned long flags;

	/* TODO: Need to remove this once the clients start
	 * registering to the ab8500 core driver
	 */
	if (!ab8500)
		return -ENODEV;

	if (is_intid_invalid(int_no)) {
		printk(KERN_ERR "invalid interrupt id\n");
		return -1;
	}

	spin_lock_irqsave(&ab8500->ab8500_cfgsig_lock, flags);
	(ab8500->c_signals[int_no]).pid = NULL;
	(ab8500->c_signals[int_no]).signal = 0;
	spin_unlock_irqrestore(&ab8500->ab8500_cfgsig_lock, flags);
	return 0;

}
EXPORT_SYMBOL(ab8500_remove_signal_handler);

/**
 * ab8500_interrupt_handler() - Interrupt handler for AB8500
 * @irq:	irq number from AB8500 to U8500
 * @dev_id:	device id
 *
 * we cannot use SPI in interrupt context, so we just schedule work.
 */
static irqreturn_t ab8500_interrupt_handler(int irq, void *dev_id)
{
	/* schedule a thread to handle interrupts */
	queue_work(ab8500_wq, &ab8500_handler);
	disable_irq_nosync(ab8500->irq);
	return IRQ_HANDLED;
}

/**
 * ab8500_work()  - work queue bottom hanlf handler for ab8500
 * @work:	pointer to the work queue.
 *
 * The interrupt handler reads the latch register to find the interrupt source
 * and calculates the bit positions and calls the corresponding
 * callback handler of client device. The handler also takes care of
 * of clearing the interrupt source by reading the latch register.
 */
static void ab8500_work(struct work_struct *work)
{
	int bit, i, count = 0;
	unsigned long intl;
	struct client_callbacks *cur_list_ptr = NULL;

	/* read the latch interrupt registers */
	for (i = 0; i < ARRAY_SIZE(l_regs); i++) {
		intl = ab8500_read(AB8500_INTERRUPT, l_regs[i]);
		if (!intl)	{
			count += 8;
			continue;
		}
		/* check if any bit is set */
		bit = find_first_bit(&intl, 8);

		while (bit < 8) {
			list_for_each_entry(cur_list_ptr,
					&(ab8500->c_callback[bit + count]).
					client_list, client_list)
				cur_list_ptr->callback(
						cur_list_ptr->data);

			if ((ab8500->c_signals[bit + count]).signal)
				kill_pid((ab8500->c_signals[bit + count]).
					  pid, (ab8500->c_signals[bit + count]).
						signal, 1);
			bit = find_next_bit(&intl, 8, bit + 1);
		}
		count = count + 8;
	}
	enable_irq(ab8500->irq);
	return;
}

/**
 * ab8500_ssp_init() - SSP initialization function
 * @ab8500:	pointer to the internal data structure
 *
 * This funtion configures and Initializes ssp controller and the spi
 * protocol paramters. AB8500 is currently configured to work in SPI
 * mode and the controller used on this platform is
 * SSP (serial synchronous port)
 */
int ab8500_ssp_init(struct ab8500 *ab8500)
{
	struct ssp_controller ssp_ab8500_spi_config = {
		.lbm = LOOPBACK_DISABLED,
		.com_mode = INTERRUPT_TRANSFER,
		.iface = SPI_INTERFACE_MOTOROLA_SPI,
		.hierarchy = SPI_MASTER,
		.endian_rx = SPI_FIFO_MSB,
		.endian_tx = SPI_FIFO_MSB,
		.data_size = SSP_DATA_BITS_24,
		.slave_tx_disable = 0,
		.rx_lev_trig = SSP_RX_1_OR_MORE_ELEM,
		.tx_lev_trig = SSP_TX_1_OR_MORE_EMPTY_LOC,
		.clk_freq =	{
				/* 12 MHz prescale */
				.cpsdvsr = 2,
				.scr = 1,
				},
		.proto_params = {
				 .moto = {
				/* FIXME */
					  .clk_phase = 1,
					  .clk_pol = 1,
					  },
				 },
		.dma_config = NULL,
		.cs_control = ab8500_spi_cs_control,
		.freq = 0
	};

	/* fetch the master */
	ab8500->ab8500_master =
	    /* spi_busnum_to_master(ab8500->board->ssp_controller); */
	    spi_busnum_to_master((u16) SSP_0_CONTROLLER);

	if (NULL == ab8500->ab8500_master) {
		printk(KERN_ERR "AB8500:SSP init error - SSP0 bus not found");
		return -1;
	}
	/* SPI hookup */
	ab8500->ab8500_xfer =
	    kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
	ab8500->ab8500_msg =
	    kzalloc(sizeof(struct spi_message), GFP_KERNEL);
	ab8500->ab8500_board_info =
	    kzalloc(sizeof(struct spi_board_info), GFP_KERNEL);
	if (!ab8500->ab8500_xfer || !ab8500->ab8500_msg
	    || !ab8500->ab8500_board_info) {
		kfree(ab8500->ab8500_xfer);
		kfree(ab8500->ab8500_msg);
		kfree(ab8500->ab8500_board_info);
		printk(KERN_ERR "kzalloc failed in %s", __func__);
		return -1;
	}

	(ab8500->ab8500_board_info)->controller_data =
	    &ssp_ab8500_spi_config;
	(ab8500->ab8500_board_info)->bus_num = SSP_0_CONTROLLER;
	/* ab8500->board->ssp_controller; */
	(ab8500->ab8500_board_info)->chip_select = 0;

	/*
	 * The modalias name MUST match the device_driver name
	 * for the bus glue code to match and subsequently bind them
	 */
	strcpy((ab8500->ab8500_board_info)->modalias, "ab8500");
	ab8500->ab8500_spi =
	    spi_new_device(ab8500->ab8500_master,
			   ab8500->ab8500_board_info);

	/* Set up the linked list head */
	INIT_LIST_HEAD(&(ab8500->ab8500_msg)->transfers);

	/* initialise the mutex? FIXME */
	init_MUTEX(&(ab8500->ab8500_sem));

	/* initialize the spin lock */
	spin_lock_init(&ab8500->ab8500_cfg_lock);

	return 0;
}

/**
 * ab8500_write() - AB8500 write function.
 * @block:	Bank number
 * @adr :	register address in the block
 * @data:	data to be written
 *
 * This funtion writes to any AB8500 registers using SPI protocol &
 * before it writes it packs the data in the below 24 bit frame format
 *
 *	 *|------------------------------------|
 *	 *| 23|22...18|17.......10|9|8|7......0|
 *	 *|      bank       adr          data  |
 *	 * ------------------------------------
 *
 * This function shouldn't be called from interrupt context since it can sleep.
 * This funtion return 0 on success, -1 on error
 */
int ab8500_write(u8 block, u32 adr, u8 data)
{
	u32 spi_data;
	int error;

	if ((block > AB8500_OTP_EMUL) || (block < AB8500_SYS_CTRL1_BLOCK)) {
		printk("Invalid bank address\n");
		return -1;
	}

	/* TODO: Need to remove this once the clients start
	 * registering to the ab8500 core driver
	 */
	if (!ab8500)
		return -ENODEV;

#if defined(CONFIG_AB8500_ACCESS_CONFIG1) || \
	defined(CONFIG_AB8500_ACCESS_CONFIG2)
	if (!u8500_is_earlydrop()) {
		down(&(ab8500->ab8500_sem));
		error = prcmu_i2c_write(block, adr, data);
		up(&(ab8500->ab8500_sem));
		return error;
	}
#else
#ifdef CONFIG_AB8500_ACCESS_CONFIG5
	if (!u8500_is_earlydrop()) {
		if (block == AB8500_REGU_CTRL2) {
			down(&(ab8500->ab8500_sem));
			error = prcmu_i2c_write(block, adr, data);
			up(&(ab8500->ab8500_sem));
			return error;
		}
	}
#endif
#endif

	/**
	 *   AB8500 SPI 24 bits frame format
	 *
	 *|------------------------------------|
	 *| 23|22...18|17.......10|9|8|7......0|
	 *|      bank       adr          data  |
	 * ------------------------------------
	 */
	spi_data = block << 18 | adr << 10 | data;

	down(&(ab8500->ab8500_sem));

	ab8500->ssp_wrbuf[0] = spi_data;
	ab8500->ssp_rdbuf[0] = 0;

	INIT_LIST_HEAD(&(ab8500->ab8500_msg)->transfers);
	(ab8500->ab8500_xfer)->tx_buf =
	    ab8500->ssp_wrbuf;
	(ab8500->ab8500_xfer)->rx_buf = NULL;

	(ab8500->ab8500_xfer)->len = sizeof(u32);

	spi_message_add_tail(ab8500->ab8500_xfer,
			     ab8500->ab8500_msg);
	error =
	    spi_sync(ab8500->ab8500_spi,
		     ab8500->ab8500_msg);

	up(&(ab8500->ab8500_sem));
	return error;
}
EXPORT_SYMBOL(ab8500_write);

/**
 * ab8500_read() - AB8500 read function.
 * @block:	Bank number
 * @adr:	register address in the block
 *
 * This funtion reads from any AB8500 registers using SPI protocol. This should
 * not be called from the interrupt context.
 */
int ab8500_read(u8 block, u32 adr)
{
	u32 data = 0;
	int retval;

	if ((block > AB8500_OTP_EMUL) || (block < AB8500_SYS_CTRL1_BLOCK)) {
		printk(KERN_ERR "Invalid bank address\n");
		return -1;
	}

	/* TODO: Need to remove this once the clients start
	 * registering to the ab8500 core driver
	 */
	if (!ab8500)
		return -ENODEV;

#if defined(CONFIG_AB8500_ACCESS_CONFIG1) || \
	defined(CONFIG_AB8500_ACCESS_CONFIG2)
	if (!u8500_is_earlydrop()) {
		down(&(ab8500->ab8500_sem));
		retval = prcmu_i2c_read(block, adr);
		up(&(ab8500->ab8500_sem));
		return retval;
	}
#else
#ifdef CONFIG_AB8500_ACCESS_CONFIG5
	if (!u8500_is_earlydrop()) {
		if (block == AB8500_REGU_CTRL2) {
			down(&(ab8500->ab8500_sem));
			retval = prcmu_i2c_read(block, adr);
			up(&(ab8500->ab8500_sem));
			return retval;
		}
	}
#endif
#endif

	data = 1 << 23 | block << 18 | adr << 10;

	down(&(ab8500->ab8500_sem));
	ab8500->ssp_wrbuf[0] = data;
	ab8500->ssp_rdbuf[0] = 0;

	INIT_LIST_HEAD(&(ab8500->ab8500_msg)->transfers);
	(ab8500->ab8500_xfer)->tx_buf =
	    ab8500->ssp_wrbuf;
	(ab8500->ab8500_xfer)->rx_buf =
	    ab8500->ssp_rdbuf;
	(ab8500->ab8500_xfer)->len = sizeof(u32);

	spi_message_add_tail(ab8500->ab8500_xfer,
			     ab8500->ab8500_msg);
	spi_sync(ab8500->ab8500_spi,
		 ab8500->ab8500_msg);
	retval = ab8500->ssp_rdbuf[0] & 0xff;
	up(&(ab8500->ab8500_sem));

	/*printk("read value from ssp = %08x,%08x,%08x,%08x\n",
	   ab8500->ssp_rdbuf[0],ab8500->ssp_rdbuf[1],
	   ab8500->ssp_rdbuf[2], ab8500->ssp_rdbuf[3]);
	 */
	return retval;
}
EXPORT_SYMBOL(ab8500_read);

/**
 * ab8500_ssp_close() - unregister the device
 * @ab8500:	AB8500 context data structure
 *
 * This funtion unregisters the AB8500 as spi device and
 * frees the memory allocated.
 */
static void ab8500_ssp_close(struct ab8500 *ab8500)
{
	/* unregister the spi device */
	spi_unregister_device(ab8500->ab8500_spi);

	/* free the data structures allocated during probe */
	kfree(ab8500->ab8500_xfer);
	kfree(ab8500->ab8500_msg);
	kfree(ab8500->ab8500_board_info);
}

/**
 * ab8500_probe() - probe the device
 * @pdev:	pointer to the platform device structure
 *
 * This funtion is called after the driver is registered to platform
 * device framework. It does allocate the memory for the internal
 * data structure and request for the irq and initializes the
 * SSP controller and the locks.
 */
static int __init ab8500_probe(struct platform_device *pdev)
{
	int result = 0, status = 0, i;
	struct resource *res = NULL;

	ab8500 =
	    kzalloc(sizeof(struct ab8500), GFP_KERNEL);
	if (!ab8500) {
		result = -ENOMEM;
		dev_err(&pdev->dev, "Error : no mem\n");
		goto err_zalloc;
	}
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev, "IORESOURCE_IRQ unavailable\n");
		return -ENOENT;
	}
	/* create a thread for work */
	ab8500_wq = create_singlethread_workqueue("ab8500_wq");
	if (ab8500_wq == NULL) {
		dev_err(&pdev->dev, "failed to create work queue\n");
		return -ENOMEM;
	}

	INIT_WORK(&ab8500_handler, ab8500_work);
	ab8500->irq = res->start;

	if (ab8500_ssp_init(ab8500)) {
		result = -EINTR;
		dev_err(&pdev->dev, "error in interface init\n");
		goto err_interface;
	}

	/* Set up the linked list head for each client callbacks */
	for (i = 0; i < 184; i++)
		INIT_LIST_HEAD(&(ab8500->c_callback[i]).client_list);

	/* read the revision register */
	ab8500->revision = ab8500_read(AB8500_MISC, AB8500_REV_REG);

	if (ab8500->revision == 0x0 || ab8500->revision == 0x10
			|| ab8500->revision == 0x11)
		dev_info(&pdev->dev, "Detected chip: %s, revision = %x\n",
			ab8500_driver.driver.name, ab8500->revision);
	else	{
		dev_err(&pdev->dev, "unknown chip: 0x%x\n", ab8500->revision);
		result = -EINTR;
		goto err_interface;
	}
	/*
	 * read the latch registers to clear the any pending interrupts,
	 * and also  mask all interrupts during boot.
	 * The client drivers should unmask the
	 * corresponding interrupt
	 */
	for (i = 0; i <= 4; i++) {
		ab8500_read(AB8500_INTERRUPT, l_regs[i]);
		ab8500_write(AB8500_INTERRUPT, m_regs[i], 0xff);
	}
	if (ab8500->revision == 0x10 || ab8500->revision == 0x11)	{
		for (i = 5; i <= 9; i++)	{
			ab8500_read(AB8500_INTERRUPT, l_regs[i]);
			ab8500_write(AB8500_INTERRUPT, m_regs[i], 0xff);
		}
	}
	for (i = 10; i <= 12; i++)	{
		ab8500_read(AB8500_INTERRUPT, l_regs[i]);
		ab8500_write(AB8500_INTERRUPT, m_regs[i], 0xff);
	}
	if (ab8500->revision == 0x10 || ab8500->revision == 0x11)	{
		for (i = 13; i <= 17; i++)
			ab8500_write(AB8500_INTERRUPT, m_regs[i], 0xff);
	}

	/* mask all common ED/1.0 bits */
	for (i = 18; i <= 20; i++)
		ab8500_write(AB8500_INTERRUPT, m_regs[i], 0xff);

	status =
	    request_irq(ab8500->irq, ab8500_interrupt_handler, 0,
			"ab8500", NULL);
	if (status) {
		dev_err(&pdev->dev, "unable to request the irq %d\n",
				res->start);
		goto driver_cleanup;
	}
	dev_info(&pdev->dev, "companion chip initialized (in SPI mode)\n");
	return result;

err_interface:
	kfree(ab8500);
	ab8500 = NULL;
err_zalloc:
	return result;
driver_cleanup:
	return status;
}

/**
 * ab8500_remove() - Removes the device
 * @pdev:	pointer to the platform_device structure
 *
 * This function is called when this mnodule is removed using rmmod
 * It frees the irq and closes the SSP controller device
 */
static int ab8500_remove(struct platform_device *pdev)
{
	struct resource *res = NULL;
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	kfree(ab8500);
	ab8500_ssp_close(ab8500);
	/* delete the work queue */
	destroy_workqueue(ab8500_wq);
	free_irq(res->start, 0);
	return 0;
}

/*
 * struct ab8500_driver: AB8500 platform structure
 * @probe:	The probe funtion to be called
 * @remove:	The remove funtion to be called
 * @driver:	The driver data
 */
static struct platform_driver ab8500_driver = {

	.probe = ab8500_probe,
	.remove = ab8500_remove,
	.driver = {
		   .name = "ab8500",
		   },
};

/**
 * ab8500_init() - init the device
 *
 * This funtion registers the Stw4500 driver as platform device.
 **/
static int __devinit ab8500_init(void)
{
	return platform_driver_register(&ab8500_driver);
}

/**
 * ab8500_exit() - unregisters the device
 *
 * This funtion de-registers the AB8500 driver as platform device.
 **/
static void __exit ab8500_exit(void)
{
	platform_driver_unregister(&ab8500_driver);
}

subsys_initcall_sync(ab8500_init);

MODULE_DESCRIPTION("AB8500 PMU driver");
MODULE_AUTHOR("Srinidhi <srinidhi.kasagar@stericsson.com>");
MODULE_LICENSE("GPL");
