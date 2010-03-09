/*
 * drivers/i2c/busses/i2c-stm.c
 *
 * Copyright (C) 2009 STMicroelectronics Pvt. Ltd.
 *
 * Author: Sachin Verma <sachin.verma@st.com>
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
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/i2c-id.h>
#include <mach/hardware.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <mach/i2c-stm.h>
#include <mach/debug.h>

#define I2C_NAME          "STM I2C DRIVER"
/* enables/disables debug msgs */
#define DRIVER_DEBUG      0
#define DRIVER_DEBUG_PFX  I2C_NAME
#define DRIVER_DBG        KERN_ERR

#define I2C_TIMEOUT (msecs_to_jiffies(500))

static unsigned int stm_i2c_functionality(struct i2c_adapter *adap);
static int stm_i2c_xfer(struct i2c_adapter *i2c_adap,
		struct i2c_msg msgs[], int num_msgs);

static struct i2c_algorithm stm_i2c_algo = {
master_xfer: stm_i2c_xfer,
smbus_xfer : NULL,
functionality : stm_i2c_functionality
};

static unsigned int stm_i2c_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C
			| I2C_FUNC_SMBUS_BYTE_DATA
			| I2C_FUNC_SMBUS_WORD_DATA
			| I2C_FUNC_SMBUS_I2C_BLOCK;
}

/**
 * create_irq_mask - To generate irq mask for a i2c controller
 * @bus_num: I2c controller for which to creat mask
 * @flags: desired mask for interrupts
 *
 */
static inline u32 create_irq_mask(int bus_num, u32 flags)
{
	u32 irq_mask = 0;
	/*First clear bits 29 & 30*/
	flags = flags & (0x1FFFFFFF);
	switch (bus_num) {
	case 0:
		irq_mask = flags;
		break;
	case 1:
		irq_mask = ((0x1UL << 29) | flags);
		break;
	case 2:
		irq_mask = (((0x1UL << 30)) | flags);
		break;
	case 3:
		irq_mask = ((0x1UL << 29) | (0x1UL << 30) | flags);
		break;
	case 4:
		irq_mask = ((0x1UL << 29) | (0x1UL << 30) | (0x1UL << 31)
				| flags);
		break;
	default:
		stm_error("Illegal I2C bus\n");
		break;
	}
	return irq_mask;
}


/**
 * flush_i2c_fifo - This function flushes the I2C FIFO
 * @priv: private data of I2C Driver
 *
 * This function flushes the I2C Tx and Rx FIFOs. It returns
 * 0 on successfull flushing of FIFO
 */
static inline int flush_i2c_fifo(struct i2c_driver_data *priv)
{
	int counter = I2C_FIFO_FLUSH_COUNTER;
	writel((I2C_CR_FTX | I2C_CR_FRX), I2C_CR(priv->regs));
	while (counter) {
		if (!(readl(I2C_CR(priv->regs)) & (I2C_CR_FTX | I2C_CR_FRX)))
			return 0;
		counter--;
	}
	return -1;
}

/**
 * disable_all_interrupts - Disable all interrupts of this I2c Bus
 * @priv: private data of I2C Driver
 *
 */
static inline void disable_all_interrupts(struct i2c_driver_data *priv)
{
	u32 mask;
	mask = create_irq_mask(priv->adap.nr, 0x0);
	writel(mask , I2C_IMSCR(priv->regs));
}

/**
 * clear_all_interrupts - Clear all interrupts of I2C Controller
 * @priv: private data of I2C Driver
 *
 */
static inline void clear_all_interrupts(struct i2c_driver_data *priv)
{
	u32 mask;
	mask = create_irq_mask(priv->adap.nr, I2C_IRQ_SRC_ALL);
	writel(mask , I2C_ICR(priv->regs));
}

/**
 * enable_controller - Enable I2C Controller
 * @priv: private data of I2C Driver
 *
 */
static inline void enable_controller(struct i2c_driver_data *priv)
{
	I2C_SET_BIT(I2C_CR(priv->regs) , I2C_CR_PE);
}

/**
 * disable_controller - Disable I2C Controller
 * @priv: private data of I2C Driver
 *
 */
static inline void disable_controller(struct i2c_driver_data *priv)
{
	I2C_CLR_BIT(I2C_CR(priv->regs), I2C_CR_PE);
}

/**
 * i2c_abort - Abort called in case of I2C transaction failing
 * @priv: private data of I2C Driver
 *
 * This function flushes FIFO, clears all interrupts and
 * disables the controller.
 */
static void i2c_abort(struct i2c_driver_data *priv)
{
	flush_i2c_fifo(priv);
	disable_all_interrupts(priv);
	clear_all_interrupts(priv);
	disable_controller(priv);
	if (priv->clk)
		clk_disable(priv->clk);
	mdelay(1);
	priv->cli_data.operation = I2C_NO_OPERATION;
}

/**
 * reset_i2c_controller - To reset the I2C controller to a sane state
 * @priv: private data of I2C Driver
 *
 */
static int reset_i2c_controller(struct i2c_driver_data *priv)
{
	if (flush_i2c_fifo(priv))
		return -1;
	disable_all_interrupts(priv);
	clear_all_interrupts(priv);
	disable_controller(priv);
	mdelay(1);
	return 0;
}

static inline void print_abort_reason(i2c_error_t cause)
{
	switch (cause) {
	case I2C_NACK_ADDR:
		stm_error("No Ack received after Slave Address xmission\n");
		break;
	case I2C_NACK_DATA:
		stm_error("Valid for MASTER_WRITE: No Ack received"
				"during data phase\n");
		break;
	case I2C_ACK_MCODE:
		stm_error("Master recv ack after xmission of master code"
				"in hs mode\n");
		break;
	case I2C_ARB_LOST:
		stm_error("Master Lost arbitration\n");
		break;
	case I2C_BERR_START:
		stm_error("Slave restarts\n");
		break;
	case I2C_BERR_STOP:
		stm_error("Slave reset\n");
		break;
	case I2C_OVFL:
		stm_error("Overflow\n");
		break;
	default:
		stm_error("Unknown error type\n");
	}
}

static inline i2c_error_t get_abort_cause(struct i2c_driver_data *priv)
{
	return (i2c_error_t)((readl(I2C_SR(priv->regs)) >> 4) & 0x7);
}

static inline i2c_status_t get_cntlr_status(struct i2c_driver_data *priv)
{
	return (i2c_status_t)((readl(I2C_SR(priv->regs)) >> 2) & 0x3);
}

static inline u32 get_i2c_cntlr_reg_cfg(struct i2c_driver_data *priv)
{
	u32 cr = 0;
	/*STD mode*/
	cr |= GEN_MASK(I2C_FREQ_MODE_STANDARD , I2C_CR_SM, I2C_CR_SM_POS);
	/*Loopback Disabled*/
	cr |= GEN_MASK(I2C_DISABLED , I2C_CR_LM, I2C_CR_LM_POS);
	cr |= GEN_MASK(0, I2C_CR_SGCM, I2C_CR_SGCM_POS);
	cr |= GEN_MASK(0, I2C_CR_DMA_TX_EN, I2C_CR_DMA_TX_EN_POS);
	cr |= GEN_MASK(0, I2C_CR_DMA_RX_EN, I2C_CR_DMA_RX_EN_POS);
#if 0
	cr |= GEN_MASK(0, I2C_DMA_BURST_TX, I2C_DMA_BURST_TX_POS);
	cr |= GEN_MASK(0, I2C_DMA_BURST_RX, I2C_DMA_BURST_RX_POS);
#endif
	/*Master mode*/
	cr |= GEN_MASK(1, I2C_CR_OM, I2C_CR_OM_POS);
	cr |= GEN_MASK(0, I2C_CR_DMA_SLE, I2C_CR_DMA_SLE_POS);
	cr |= GEN_MASK(0, I2C_CR_FON, I2C_CR_FON_POS);
	cr |= GEN_MASK(1, I2C_CR_PE, I2C_CR_PE_POS);
	if (!cpu_is_u8500ed())
		cr |= GEN_MASK(0, I2C_CR_FS, I2C_CR_FS_POS);
	/* modified for touch screen client */
	if (priv->adap.nr == 3)
		cr |= 0x10;
	return cr;
}

static inline u32 get_master_cntlr_reg_cfg(struct i2c_driver_data *priv)
{
	u32 mcr = 0;
	mcr |= GEN_MASK(I2C_7_BIT_ADDRESS , I2C_MCR_AM, I2C_MCR_AM_POS);
	mcr |= GEN_MASK(priv->cli_data.slave_address , I2C_MCR_A7 ,
						I2C_MCR_A7_POS);
	/*Start Byte procedure not applied*/
	mcr |= GEN_MASK(0, I2C_MCR_SB, I2C_MCR_SB_POS);
	if (priv->cli_data.operation == I2C_WRITE)
		mcr |= GEN_MASK(I2C_WRITE , I2C_MCR_OP, I2C_MCR_OP_POS);
	else
		mcr |= GEN_MASK(I2C_READ , I2C_MCR_OP, I2C_MCR_OP_POS);

	if (priv->stop)
		mcr |= GEN_MASK(1 , I2C_MCR_STOP, I2C_MCR_STOP_POS);
	else
		mcr &= ~(GEN_MASK(1 , I2C_MCR_STOP, I2C_MCR_STOP_POS));
	mcr |= GEN_MASK(priv->cli_data.count_data, I2C_MCR_LENGTH,
						I2C_MCR_LENGTH_POS);
	return mcr;
}


static inline void setup_i2c_controller(struct i2c_driver_data *priv)
{
	u32 slsu = 0;
	u32 brcr1 = 0;
	u32 brcr2 = 0;

	writel(0x0, I2C_CR(priv->regs));
	writel(0x0, I2C_HSMCR(priv->regs));
	writel(0x0, I2C_TFTR(priv->regs));
	writel(0x0, I2C_RFTR(priv->regs));
	writel(0x0, I2C_DMAR(priv->regs));

	slsu = GEN_MASK(priv->cfg.slave_data_setup_time ,
			I2C_SCR_DATA_SETUP_TIME, 16);
	writel(slsu , I2C_SCR(priv->regs));

	brcr1 =  GEN_MASK(0, I2C_BRCR_BRCNT1, 16);
	/* modified for touch screen client */
	if (priv->adap.nr != 3)
		brcr2 = GEN_MASK((u32)(clk_get_rate(priv->clk)/(priv->cfg.clk_freq*2)),
			I2C_BRCR_BRCNT2, 0);
	else
		brcr2 = 0x20;
	writel((brcr1 | brcr2), I2C_BRCR(priv->regs));
	writel(priv->cfg.i2c_transmit_interrupt_threshold,
			I2C_TFTR(priv->regs));
	writel(priv->cfg.i2c_receive_interrupt_threshold,
			I2C_RFTR(priv->regs));
	/* modified for touch screen client */
	if (priv->adap.nr == 3) {
		writel(0x000F0060, I2C_THD_FST_STD(priv->regs));
		writel(0x000F0071, I2C_THU_FST_STD(priv->regs));
	}
}

void read_index_from_i2c_msg(struct i2c_driver_data *priv, struct i2c_msg *msg)
{
	switch (msg->len) {
	case 0:
		priv->cli_data.index_format = I2C_NO_INDEX;
		priv->cli_data.register_index = 0;
		break;
	case 1:
		priv->cli_data.index_format = I2C_BYTE_INDEX;
		memcpy(&(priv->cli_data.register_index), msg->buf, 1);
		break;
	case 2:
		priv->cli_data.index_format = I2C_HALF_WORD_LITTLE_ENDIAN;
		memcpy(&(priv->cli_data.register_index), msg->buf, 2);
		break;
	case 3:
		priv->cli_data.index_format = I2C_HALF_WORD_BIG_ENDIAN;
		memcpy(&(priv->cli_data.register_index), msg->buf, 2);
		break;
	default:
		priv->cli_data.index_format = I2C_INVALID_INDEX;
	}
}

/**
 * wait_for_data - Wait till some data arrives in Rx FIFO
 * @priv: private data of I2C Driver
 * @counter: Timeout counter for this operation
 *
 * This function checks if there is some data in Rx FIFO.
 * It returns 0 if there is data, -1 otherwise
 */
static inline int wait_for_data(struct i2c_driver_data *priv, int counter)
{
	while (readl(I2C_RISR(priv->regs)) & I2C_IT_RXFE) {
		counter--;
		if (!counter)
			return -1;
	}
	return 0;
}

static inline int read_fifo(struct i2c_driver_data *priv)
{
	return readb(I2C_RFR(priv->regs));
}


static inline int is_master_transaction_done(struct i2c_driver_data *priv,
								int counter)
{
	int bit_flag = 0;
	if (priv->stop)
		bit_flag = I2C_IT_MTD;
	else
		bit_flag = I2C_IT_MTDWS;

	while (counter) {
		if (readl(I2C_RISR(priv->regs)) & bit_flag)
			return 0;
		counter--;
	}
	return -1;
}

static inline void ack_master_transaction(struct i2c_driver_data *priv)
{
	if (priv->stop)
		I2C_SET_BIT(I2C_ICR(priv->regs), I2C_IT_MTD);
	else
		I2C_SET_BIT(I2C_ICR(priv->regs), I2C_IT_MTDWS);
}

/**
 * i2c_master_receive_data_polling - Receive Data in polling mode (as Master)
 * @priv: private data of I2C Driver
 *
 *
 * This function is called when i2c is running in master mode.
 * It will read data from read fifo register of i2c controller in polling
 * mode. It will wait till master transaction done interrupt is set with a
 * timeout. If MTD bit is not set, it will abort and return error.
 *
 */
static int i2c_master_receive_data_polling(struct i2c_driver_data *priv)
{
	volatile u8 *p_data;
	int ret = 0;
	int status = 0;

	p_data = priv->cli_data.databuffer;
	/* Master Receiver */
	while (priv->cli_data.count_data != 0) {
		ret = wait_for_data(priv, I2C_STATUS_UPDATE_COUNTER);
		if (ret) {
			stm_error("Receive FIFO is empty.......\n");
			return -EIO;
		}
		*p_data = read_fifo(priv);
		p_data++;

		priv->cli_data.transfer_data++;
		priv->cli_data.count_data--;
	}
	status = is_master_transaction_done(priv, I2C_STATUS_UPDATE_COUNTER);
	if (status) {
		priv->result = -1;
		i2c_abort(priv);
		return status;
	}
	ack_master_transaction(priv);
	priv->cli_data.operation = I2C_NO_OPERATION;
	return status;
}

static inline int wait_for_space_in_fifo(struct i2c_driver_data *priv,
								int counter)
{
	while (readl(I2C_RISR(priv->regs)) & I2C_IT_TXFF) {
		counter--;
		if (!counter)
			return -1;
	}
	return 0;
}

static inline void write_fifo(struct i2c_driver_data *priv, u8 data)
{
	writeb(data, I2C_TFR(priv->regs));
}

/**
 * transmit_data_polling - Write data in master mode (polling xfer)
 *
 * @priv: private data of I2C Driver
 *
 * This function writes data to transmit fifo of the I2C controller.
 * The controller is configured in master mode. We keep on writing
 * till we have data.
 *
 */
static inline int transmit_data_polling(struct i2c_driver_data *priv)
{
	u8 *p_data;
	int ret = 0;
	int status = 0;
	p_data = priv->cli_data.databuffer;

	while (priv->cli_data.count_data != 0) {
		ret = wait_for_space_in_fifo(priv, I2C_STATUS_UPDATE_COUNTER);
		if (ret) {
			stm_error("Transmit Fifo not getting empty\n");
			return -1;
		}
		write_fifo(priv, *p_data);
		priv->cli_data.transfer_data++;
		priv->cli_data.count_data--;
		p_data++;
	}
	status = is_master_transaction_done(priv, I2C_STATUS_UPDATE_COUNTER);
	if (status) {
		priv->result = -1;
		i2c_abort(priv);
		return status;
	}
	ack_master_transaction(priv);
	priv->cli_data.operation = I2C_NO_OPERATION;
	return 0;
}

/**
 * read_i2c_master_mode - Read from I2C client device when cntlr is in M mode
 *
 * @priv: private data of I2C Driver
 *
 * This function reads from i2c client device when controller is in
 * master mode. Based on the transfer mode (polling or Interrupt) it will
 * read data.
 * For Interrupt mode there is a completion timeout. If there is no transfer
 * before timeout error is returned.
 *
 */
static inline int read_i2c_master_mode(struct i2c_driver_data *priv)
{
	u32 status = 0;
	u32 mcr = 0;
	u32 cr = 0;
	cr = get_i2c_cntlr_reg_cfg(priv);
	mcr = get_master_cntlr_reg_cfg(priv);
	writel(mcr, I2C_MCR(priv->regs));
	writel(cr, I2C_CR(priv->regs));
	enable_controller(priv);

	switch (priv->cfg.xfer_mode) {
	case I2C_TRANSFER_MODE_POLLING:
	{
		status = i2c_master_receive_data_polling(priv);
		break;
	}
	case I2C_TRANSFER_MODE_INTERRUPT:
	{
		u32 irq_mask = 0;
		int timeout = 0;
		irq_mask = (I2C_IT_RXFNF
				| I2C_IT_RXFF
				| I2C_IT_MAL
				| I2C_IT_BERR);
		if (priv->stop)
			irq_mask |= I2C_IT_MTD;
		else
			irq_mask |= I2C_IT_MTDWS;

		irq_mask = create_irq_mask(priv->adap.nr, irq_mask);
		irq_mask = (I2C_IRQ_SRC_ALL & irq_mask);
init_completion(&priv->xfer_complete);
		writel(readl(I2C_IMSCR(priv->regs)) | irq_mask,
							I2C_IMSCR(priv->regs));

		timeout = wait_for_completion_interruptible_timeout(
				&priv->xfer_complete, I2C_TIMEOUT);
		if (timeout == 0) {
			priv->result = -1;
			i2c_abort(priv);
			status = -EIO;
		}
		break;
	}
	case I2C_TRANSFER_MODE_DMA:
	{
		stm_error("DMA Mode not supported....\n");
		status = -EINVAL;
		break;
	}
	default:
	{
		stm_error("xfer mode not supported\n");
		status = -EINVAL;
		break;
	}
	}
	return status;
}

/**
 * read_i2c_slave_mode - Read from i2c client when this Cntlr is in slave mode
 *
 * @priv: private data of I2C Driver
 *
 * Slave mode is not supported.
 */
static int read_i2c_slave_mode(struct i2c_driver_data *priv)
{
	stm_error("Slave Mode not implemented Yet!! \n");
	return -EIO;
}


/**
 * read_i2c - Function to read from client i2c device
 *
 * @priv: private data of I2C Driver
 *
 * This function is called by main transfer function registered with
 * I2c Framework.
 *
 */
static inline int read_i2c(struct i2c_driver_data *priv)
{
	int status = 0;
	if (I2C_BUS_MASTER_MODE == priv->cfg.bus_control_mode)
		status = read_i2c_master_mode(priv);
	else
		status = read_i2c_slave_mode(priv);
	return status;
}

/**
 * write_i2c_slave_mode - Write data when controller is in slave mode
 * @priv: private data of I2C Driver
 *
 * Slave mode is not implemented.
 *
 */
static int write_i2c_slave_mode(struct i2c_driver_data *priv)
{
	return -EIO;
}

/**
 * write_i2c_master_mode - Write data to I2C client.
 *
 * @priv: private data of I2C Driver
 *
 * This function writes data to I2C client based on the xfer mode.
 * Currently data write is supported only in polling and interrupt
 * mode.
 *
 */
static inline int write_i2c_master_mode(struct i2c_driver_data *priv)
{
	u32 status = 0;
	u32 mcr = 0;
	u32 cr = 0;

	cr = get_i2c_cntlr_reg_cfg(priv);
	mcr = get_master_cntlr_reg_cfg(priv);

	writel(mcr, I2C_MCR(priv->regs));
	writel(cr, I2C_CR(priv->regs));

	enable_controller(priv);

	switch (priv->cfg.xfer_mode) {
	case I2C_TRANSFER_MODE_POLLING:
	{
		status = transmit_data_polling(priv);
		break;
	}
	case I2C_TRANSFER_MODE_INTERRUPT:
	{
		u32 irq_mask = 0;
		int timeout = 0;
		irq_mask = (I2C_IT_TXFNE
				| I2C_IT_TXFOVR
				| I2C_IT_MAL
				| I2C_IT_BERR);
		if (priv->stop)
			irq_mask |= I2C_IT_MTD;
		else
			irq_mask |= I2C_IT_MTDWS;

		irq_mask = create_irq_mask(priv->adap.nr, irq_mask);
		irq_mask = (I2C_IRQ_SRC_ALL & irq_mask);
init_completion(&priv->xfer_complete);
		writel(readl(I2C_IMSCR(priv->regs)) | irq_mask,
						I2C_IMSCR(priv->regs));

		timeout = wait_for_completion_interruptible_timeout(
				&priv->xfer_complete, I2C_TIMEOUT);
		if (timeout == 0) {
			priv->result = -1;
			i2c_abort(priv);
			status = -EIO;
		}
		break;
	}
	case I2C_TRANSFER_MODE_DMA:
	{
		stm_error("DMA mode not supported...\n");
		status = -EINVAL;
	}
	default:
	{
		stm_error("xfer mode not supported\n");
		status = -EINVAL;
		break;
	}
	}
	return status;
}

/**
 * write_i2c - Write data to the I2C client.
 * @priv: private data of I2C Driver
 *
 * This function is called by the main transfer function registered
 * with I2C framework.
 */
static inline int write_i2c(struct i2c_driver_data *priv)
{
	/*FIXME: please update the status and send*/
	int status = 0;
	if (I2C_BUS_MASTER_MODE == priv->cfg.bus_control_mode)
		status = write_i2c_master_mode(priv);
	else
		status = write_i2c_slave_mode(priv);
	return status;
}

/**
 * stm_i2c_xfer - I2C transfer function used by kernel framework
 * @i2c_adapter - Adapter pointer to the controller
 * @msgs[] - Pointer to data to be written.
 * @num_msgs - Number of messages to be executed
 *
 * This is the function called by the generic kernel i2c_transfer()
 * or i2c_smbus...() API calls. Note that this code is protected by the
 * semaphore set in the kernel i2c_transfer() function.
 *
 * NOTE:
 *  READ TRANSFER : We impose a restriction of the first message to be the
 *  		    index message for any read transaction.
 *                  msg[0].len should be sent across as type i2c_index_format_t
 *                  - a no index is coded as '0',
 *                  - 2byte big endian index is coded as '3'
 *                  !!! msg[0].buf holds the actual index.
 *                  This is compatible with generic messages of smbus emulator
 *                  that send a one byte index.
 *                  eg. a I2C transation to read 2 bytes from index 0
 *                          idx = 0;
 *                          msg[0].addr = client->addr;
 *                          msg[0].flags = 0x0;
 *                          msg[0].len = 1;
 *                          msg[0].buf = &idx;
 *
 *                          msg[1].addr = client->addr;
 *                          msg[1].flags = I2C_M_RD;
 *                          msg[1].len = 2;
 *                          msg[1].buf = rd_buff
 *                          i2c_transfer(adap, msg, 2);
 *
 * WRITE TRANSFER : The I2C standard interface interprets all data as payload.
 * 		    If you want to emulate an SMBUS write transaction put the
 * 		    index as first byte(or first and second) in the payload.
 *                  eg. a I2C transation to write 2 bytes from index 1
 *                          wr_buff[0] = 0x1;
 *                          wr_buff[1] = 0x23;
 *                          wr_buff[2] = 0x46;
 *
 *                          msg[0].addr = client->addr;
 *                          msg[0].flags = 0x0;
 *                          msg[0].len = 3;
 *                          msg[0].buf = wr_buff;
 *                          i2c_transfer(adap, msg, 1);
 *
 * To read or write a block of data (multiple bytes) using SMBUS emulation
 * please use the i2c_smbus_read_i2c_block_data()
 * or i2c_smbus_write_i2c_block_data() API
 *
 * i2c_master_recv() API is not supported as single read message is not
 * accepted by our interface (it has to be preceded by an index message)
 *
 **/
static int stm_i2c_xfer(struct i2c_adapter *i2c_adap,
		struct i2c_msg msgs[], int num_msgs)
{
	int status = 0;
	int i = 0;
	i2c_error_t cause;
	struct i2c_driver_data *priv = i2c_adap->algo_data;

	if (priv->clk)
		clk_enable(priv->clk);

	if (reset_i2c_controller(priv))
		return -1;
	setup_i2c_controller(priv);

	for (i = 0; i < num_msgs; i++) {
		if (unlikely(msgs[i].flags & I2C_M_TEN)) {
			stm_error("10 bit addressing not supported\n");
			return -EINVAL;
		}
		priv->cli_data.slave_address = msgs[i].addr;
		priv->cli_data.databuffer = msgs[i].buf;
		priv->cli_data.count_data = msgs[i].len;
		priv->stop = (i < (num_msgs - 1)) ? 0 : 1;
		priv->result = 0;

		if (msgs[i].flags & I2C_M_RD) {
			priv->cli_data.operation = I2C_READ;
			status = read_i2c(priv);
		} else {
			if ((i == 0) && (num_msgs > 1) && (msgs[1].flags & I2C_M_RD))
				read_index_from_i2c_msg(priv, &msgs[0]);
			priv->cli_data.operation = I2C_WRITE;
			status = write_i2c(priv);
		}
		if (status || (priv->result)) {
			cause = get_abort_cause(priv);
			stm_error("Err during I2C message xfer: %d\n", cause);
			print_abort_reason(cause);
			return status;
		}
		mdelay(1);
	}
	if (priv->clk)
		clk_disable(priv->clk);
	return status;
}

int disable_i2c_irq_line(void __iomem *regs, int bus_num, u32 irq_line)
{
	irq_line = create_irq_mask(bus_num, irq_line);
	writel(readl(I2C_IMSCR(regs)) & ~(I2C_IRQ_SRC_ALL & irq_line),
			I2C_IMSCR(regs));
	return 0;
}

int process_interrupt(struct i2c_driver_data *priv)
{
	u32 txthreshold, rxthreshold;
	u32 count;
	int flip = 0;

	volatile u32 misr = 0;
	u32 int_num = 0;
	txthreshold = readl(I2C_TFTR(priv->regs));
	rxthreshold = readl(I2C_RFTR(priv->regs));

	misr = readl(I2C_MISR(priv->regs));

	flip = ~misr;
	int_num = find_first_zero_bit(&flip, 32);

	switch ((1UL << int_num)) {
	case I2C_IT_TXFNE:
	{
		if (I2C_READ == priv->cli_data.operation) {
			/*In Read Op why do we care for writing?*/
			disable_i2c_irq_line(priv->regs, priv->adap.nr,
					I2C_IT_TXFNE);
		} else {
			for (count = (MAX_I2C_FIFO_THRESHOLD - txthreshold - 2);
					(count > 0) &&
					(0 != priv->cli_data.count_data);
					count--) {
				writel(*priv->cli_data.databuffer,
						I2C_TFR(priv->regs));
				priv->cli_data.databuffer++;
				priv->cli_data.count_data--;
				priv->cli_data.transfer_data++;
			}

			if (0 == priv->cli_data.count_data)
				disable_i2c_irq_line(priv->regs, priv->adap.nr,
							I2C_IT_TXFNE);
		}
	}
	break;
	case I2C_IT_RXFNF:
		for (count = rxthreshold; count > 0; count--) {
			*priv->cli_data.databuffer = readl(I2C_RFR(priv->regs));
			priv->cli_data.databuffer++;
		}
		priv->cli_data.count_data -= rxthreshold;
		priv->cli_data.transfer_data += rxthreshold;
		break;
	case I2C_IT_RXFF:
		for (count = MAX_I2C_FIFO_THRESHOLD; count > 0; count--) {
			*priv->cli_data.databuffer = readl(I2C_RFR(priv->regs));
			priv->cli_data.databuffer++;
		}
		priv->cli_data.count_data -= MAX_I2C_FIFO_THRESHOLD;
		priv->cli_data.transfer_data += MAX_I2C_FIFO_THRESHOLD;
		break;
	case I2C_IT_MTD:
	case I2C_IT_MTDWS:
		if (I2C_READ == priv->cli_data.operation) {
			while (!I2C_TEST_BIT(I2C_RISR(priv->regs),
						I2C_IT_RXFE)) {
				if (0 == priv->cli_data.count_data)
					break;
				*priv->cli_data.databuffer =
					readl(I2C_RFR(priv->regs));
				priv->cli_data.databuffer++;
				priv->cli_data.count_data--;
				priv->cli_data.transfer_data++;
			}
		}

		I2C_SET_BIT(I2C_ICR(priv->regs), I2C_IT_MTD);
		I2C_SET_BIT(I2C_ICR(priv->regs), I2C_IT_MTDWS);
		disable_i2c_irq_line(priv->regs, priv->adap.nr,
				(I2C_IT_TXFNE | I2C_IT_TXFE | I2C_IT_TXFF
					| I2C_IT_TXFOVR | I2C_IT_RXFNF
					| I2C_IT_RXFF | I2C_IT_RXFE));

		if (priv->cli_data.count_data) {
			priv->result = -1;
			i2c_abort(priv);
			stm_error("%d bytes still remain to be xfered\n",
					priv->cli_data.count_data);
		}
		complete(&priv->xfer_complete);
		break;
	case I2C_IT_MAL:
		priv->result = -1;
		i2c_abort(priv);
		I2C_SET_BIT(I2C_ICR(priv->regs), I2C_IT_MAL);
		complete(&priv->xfer_complete);
		break;
	case I2C_IT_BERR:
		priv->result = -1;
		if (get_cntlr_status(priv) == I2C_ABORT)
			i2c_abort(priv);
		I2C_SET_BIT(I2C_ICR(priv->regs), I2C_IT_BERR);
		complete(&priv->xfer_complete);
		break;
	case I2C_IT_TXFOVR:
		priv->result = -1;
		i2c_abort(priv);
		stm_error("Tx Fifo Over run\n");
		complete(&priv->xfer_complete);
		break;
	case I2C_IT_TXFE:
	case I2C_IT_TXFF:
	case I2C_IT_RXFE:
	case I2C_IT_RFSR:
	case I2C_IT_RFSE:
	case I2C_IT_WTSR:
	case I2C_IT_STD:
		stm_error("Unhandled Interrupts\n");
		break;
	default:
		stm_error("Spurious Interrupt........\n");
		break;
	}

	return 0;
}

static irqreturn_t stm_i2c_irq_handler(int irq, void *arg)
{
	struct i2c_driver_data *drv_data = arg;
	process_interrupt(drv_data);
	return IRQ_HANDLED;
}

static int stm_i2c_probe(struct platform_device *pdev)
{
	int retval = -EINVAL;
	struct resource *res = NULL;
	struct i2c_platform_data *pdata = NULL;
	struct i2c_driver_data *drv_data;
	struct i2c_adapter *p_adap;

	drv_data = kzalloc(sizeof(struct i2c_driver_data), GFP_KERNEL);
	if (!drv_data) {
		dev_err(&pdev->dev, "Cannot allocate memory\n");
		return -ENOMEM;
	}

	/*Fetch the Resources, using platform data */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (NULL == res) {
		stm_error("probe - MEM resources not defined\n");
		retval = -ENODEV;
		goto kfree;
	}

	drv_data->regs = ioremap(res->start, res->end - res->start + 1);
	drv_data->irq = platform_get_irq(pdev, 0);

	if (drv_data->irq < 0) {
		stm_error("probe - IRQ resource not defined\n");
		retval = -ENODEV;
		goto iounmap;
	}

	drv_data->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(drv_data->clk)) {
		retval = PTR_ERR(drv_data->clk);
		goto iounmap;
	}

	p_adap = &drv_data->adap;
	p_adap->dev.parent = &pdev->dev;
	p_adap->id = I2C_ALGO_STM | I2C_HW_STM;
	p_adap->nr = pdev->id;
	p_adap->algo = &stm_i2c_algo;
	p_adap->algo_data = drv_data;
	p_adap->class = I2C_CLASS_HWMON | I2C_CLASS_SPD;

	pdata = pdev->dev.platform_data;
	if (pdata == NULL) {
		stm_error("I2C probe : - no platform data supplied\n");
		retval = -ENODEV;
		goto clk_disable;
	}

	strlcpy(p_adap->name, pdata->name, sizeof(p_adap->name));

	/*Copy Controller config from Platform data*/
	drv_data->cfg.own_addr = pdata->own_addr;
	drv_data->cfg.mode = pdata->mode;
	drv_data->cfg.clk_freq = pdata->clk_freq;
	drv_data->cfg.slave_addressing_mode = pdata->slave_addressing_mode;
	drv_data->cfg.digital_filter_control = pdata->digital_filter_control;
	drv_data->cfg.dma_sync_logic_control = pdata->dma_sync_logic_control;
	drv_data->cfg.start_byte_procedure = pdata->start_byte_procedure;
	drv_data->cfg.slave_data_setup_time = pdata->slave_data_setup_time;
	drv_data->cfg.bus_control_mode = pdata->bus_control_mode;
	drv_data->cfg.i2c_loopback_mode = pdata->i2c_loopback_mode;
	drv_data->cfg.xfer_mode = pdata->xfer_mode;
	drv_data->cfg.high_speed_master_code = pdata->high_speed_master_code;
	drv_data->cfg.i2c_transmit_interrupt_threshold =
						pdata->i2c_tx_int_threshold;
	drv_data->cfg.i2c_receive_interrupt_threshold =
						pdata->i2c_rx_int_threshold;

	retval = stm_gpio_altfuncenable(pdata->gpio_alt_func);
	if (retval) {
		stm_error("GPIO En Alt Func(%d) failed with return = %d\n",
						pdev->id, retval);
		goto clk_disable;
	}

	retval = request_irq(drv_data->irq, stm_i2c_irq_handler, 0,
						p_adap->name, drv_data);
	if (retval < 0) {
		stm_error("i2c[%d] can't get requested irq %d\n",
						p_adap->nr, drv_data->irq);

		i2c_del_adapter(p_adap);
		goto altfuncdisable;
	}

	reset_i2c_controller(drv_data);

	platform_set_drvdata(pdev, drv_data);

	retval = i2c_add_numbered_adapter(p_adap);
	if (retval) {
		stm_error("STM I2C[%d] Error: failed to add adapter\n",
				pdev->id);
		goto altfuncdisable;
	}

	printk(KERN_INFO "I2C master cntlr [%s] intialized with irq %d, GPIO pin %d\n",
			pdata->name, drv_data->irq,
			pdata->gpio_alt_func);
	return 0;

altfuncdisable:
	stm_gpio_altfuncdisable(pdata->gpio_alt_func);
clk_disable:
	clk_put(drv_data->clk);
iounmap:
	iounmap(drv_data->regs);
kfree:
	kfree(drv_data);
	return retval;
}

static int stm_i2c_remove(struct platform_device *pdev)
{
	int retval;
	struct i2c_platform_data *pdata = NULL;
	struct i2c_driver_data *drv_data = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, NULL);
	pdata = pdev->dev.platform_data;
	i2c_del_adapter(&(drv_data->adap));
	free_irq(drv_data->irq, drv_data);
	clk_put(drv_data->clk);
	iounmap(drv_data->regs);
	kfree(drv_data);
	retval = stm_gpio_altfuncdisable(pdata->gpio_alt_func);
	if (retval) {
		stm_error("GPIO Disable Alt Function(%d) failed with  %d\n",
							pdev->id, retval);
		return retval;
	}
	return 0;
}
#ifdef CONFIG_PM
/**
 * stm_i2c_suspend - I2C suspend function registered with PM framework.
 * @pdev: Reference to platform device structure of the device
 * @state: power mgmt state.
 *
 * This function is invoked when the system is going into sleep, called
 * by the power management framework of the linux kernel.
 * Nothing is required as controller is configured with every transfer.
 * It is assumed that no active tranfer is in progress at this time.
 * Client driver should make sure of this.
 *
 */

int stm_i2c_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}
/**
 * stm_i2c_resume - I2C Resume function registered with PM framework.
 * @pdev: Reference to platform device structure of the device
 *
 * This function is invoked when the system is coming out of sleep, called
 * by the power management framework of the linux kernel.
 * Nothing is required.
 *
 */

int stm_i2c_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define stm_i2c_suspend NULL
#define stm_i2c_resume NULL
#endif
static struct platform_driver stm_i2c_driver = {
	.probe = stm_i2c_probe,
	.remove = stm_i2c_remove,
	.suspend = stm_i2c_suspend,
	.resume = stm_i2c_resume,
	.driver = {
		.owner = THIS_MODULE,
		.name = "STM-I2C",
	},
};

static int __init stm_i2c_init(void)
{
	return platform_driver_register(&stm_i2c_driver);
}

static void __exit stm_i2c_exit(void)
{
	platform_driver_unregister(&stm_i2c_driver);
	return;
}

subsys_initcall(stm_i2c_init);
module_exit(stm_i2c_exit);

MODULE_AUTHOR("Sachin Verma <sachin.verma@st.com>");
MODULE_DESCRIPTION("STM I2C driver ");
MODULE_LICENSE("GPL");
