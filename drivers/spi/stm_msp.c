/*
 * drivers/spi/stm_msp.c
 *
 * Copyright (C) 2006 STMicroelectronics Pvt. Ltd.
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
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/delay.h>

#include <mach/hardware.h>
#include <linux/io.h>
#include <asm/irq.h>

#include <mach/hardware.h>
#include <mach/debug.h>
#include <mach/irqs.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <mach/dma.h>
#include <asm/dma.h>
#include <linux/spi/spi-stm.h>
#include <linux/spi/stm_msp.h>


#define STM_MSP_NAME		"STM_MSP"
#define MSP_NAME                "msp"
#define DRIVER_DEBUG_PFX "MSP"
#define DRIVER_DEBUG 0
#define DRIVER_DBG "MSP"
#define NMDK_DBG 	KERN_ERR	/* message level */

#define MSP_WBITS(reg, val, mask, sb) \
	((reg) =   (((reg) & ~(mask)) | (((val)<<(sb)) & (mask))))


/**
 * msp_controller_cmd - To execute controller specific commands for MSP
 * @drv_data: SPI driver private data structure
 * @cmd: Command which is to be executed on the controller
 *
 *
 */
static int msp_controller_cmd(struct driver_data *drv_data, int cmd)
{
	int retval = 0;
	struct msp_regs *msp_regs = NULL;

	switch (cmd) {
	case DISABLE_CONTROLLER:
		{
			stm_dbg2(DBG_ST.msp, ":::: DISABLE_CONTROLLER\n");
			writel((readl(MSP_GCR(drv_data->regs)) &
				(~(MSP_GCR_MASK_TXEN | MSP_GCR_MASK_RXEN))),
			       MSP_GCR(drv_data->regs));
			break;
		}
	case ENABLE_CONTROLLER:
		{
			stm_dbg2(DBG_ST.msp, ":::: ENABLE_CONTROLLER\n");
			writel((readl(MSP_GCR(drv_data->regs)) |
				(MSP_GCR_MASK_TXEN | MSP_GCR_MASK_RXEN)),
			       MSP_GCR(drv_data->regs));
			break;
		}
	case DISABLE_DMA:
		{
			stm_dbg2(DBG_ST.msp, ":::: DISABLE_DMA\n");
			writel(DEFAULT_MSP_REG_DMACR,
			       MSP_DMACR(drv_data->regs));
			break;
		}
	case ENABLE_DMA:
		{
			msp_regs =
				(struct msp_regs *)drv_data->cur_chip->ctr_regs;
			stm_dbg2(DBG_ST.msp, ":::: ENABLE_DMA\n");
			writel(msp_regs->dmacr, MSP_DMACR(drv_data->regs));
			break;
		}
	case DISABLE_ALL_INTERRUPT:
		{
			stm_dbg2(DBG_ST.msp, ":::: DISABLE_ALL_INTERRUPT\n");
			writel(DISABLE_ALL_MSP_INTERRUPTS,
			       MSP_IMSC(drv_data->regs));
			break;
		}
	case ENABLE_ALL_INTERRUPT:
		{
			stm_dbg2(DBG_ST.msp, ":::: ENABLE_ALL_INTERRUPT\n");
			writel(ENABLE_ALL_MSP_INTERRUPTS,
			       MSP_IMSC(drv_data->regs));
			break;
		}
	case CLEAR_ALL_INTERRUPT:
		{
			stm_dbg2(DBG_ST.msp, ":::: CLEAR_ALL_INTERRUPT\n");
			writel(CLEAR_ALL_MSP_INTERRUPTS,
			       MSP_ICR(drv_data->regs));
			break;
		}
	case FLUSH_FIFO:
		{
			unsigned long limit = loops_per_jiffy << 1;
			stm_dbg2(DBG_ST.msp, ":::: DATA FIFO is flushed\n");
			do {
				while (!(readl(MSP_FLR(drv_data->regs))
					& MSP_FLR_MASK_RFE)) {
						readl(MSP_DR(drv_data->regs));
				}
			} while ((readl(MSP_FLR(drv_data->regs)) &
				  (MSP_FLR_MASK_TBUSY | MSP_FLR_MASK_RBUSY))
				 && limit--);
			retval = limit;
			break;
		}
	case RESTORE_STATE:
		{
			msp_regs =
				(struct msp_regs *)drv_data->cur_chip->ctr_regs;
			stm_dbg2(DBG_ST.msp, ":::: RESTORE_STATE\n");
			writel(msp_regs->gcr, MSP_GCR(drv_data->regs));
			writel(msp_regs->tcf, MSP_TCF(drv_data->regs));
			writel(msp_regs->rcf, MSP_RCF(drv_data->regs));
			writel(msp_regs->srg, MSP_SRG(drv_data->regs));
			writel(msp_regs->dmacr,
			       MSP_DMACR(drv_data->regs));
			writel(DISABLE_ALL_MSP_INTERRUPTS,
			       MSP_IMSC(drv_data->regs));
			writel(CLEAR_ALL_MSP_INTERRUPTS,
			       MSP_ICR(drv_data->regs));
			break;
		}
	case LOAD_DEFAULT_CONFIG:
		{
			stm_dbg2(DBG_ST.msp, ":::: LOAD_DEFAULT_CONFIG\n");
			writel(DEFAULT_MSP_REG_GCR, MSP_GCR(drv_data->regs));
			writel(DEFAULT_MSP_REG_TCF, MSP_TCF(drv_data->regs));
			writel(DEFAULT_MSP_REG_RCF, MSP_RCF(drv_data->regs));
			writel(DEFAULT_MSP_REG_SRG, MSP_SRG(drv_data->regs));
			writel(DEFAULT_MSP_REG_DMACR,
			       MSP_DMACR(drv_data->regs));
			writel(DISABLE_ALL_MSP_INTERRUPTS,
			       MSP_IMSC(drv_data->regs));
			writel(CLEAR_ALL_MSP_INTERRUPTS,
			       MSP_ICR(drv_data->regs));
			break;
		}
	default:
		{
			stm_dbg2(DBG_ST.msp, ":::: unknown command\n");
			retval = -1;
			break;
		}
	}
	return retval;
}

/**
 * msp_null_writer - To Write Dummy Data in Data register
 * @drv_data: spi driver private data structure
 *
 * This function is set as a write function for transfer which have
 * Tx transfer buffer as NULL. It simply writes '0' in the Data
 * register
 */
static void msp_null_writer(struct driver_data *drv_data)
{
	u32 cur_write = 0;
	u32 status;
	while (1) {
		status = readl(MSP_FLR(drv_data->regs));
		if ((status & MSP_FLR_MASK_TFU) ||
			(drv_data->tx >= drv_data->tx_end))
				return;
		writel(0x0, MSP_DR(drv_data->regs));
		drv_data->tx += (drv_data->cur_chip->n_bytes);
		cur_write++;
		if (cur_write == 8)
			return;
	}
}

/**
 * msp_null_reader - To read data from Data register and discard it
 * @drv_data: spi driver private data structure
 *
 * This function is set as a reader function for transfer which have
 * Rx Transfer buffer as null. Read Data is rejected
 *
 */
static void msp_null_reader(struct driver_data *drv_data)
{
	u32 status;
	while (1) {
		status = readl(MSP_FLR(drv_data->regs));
		if ((status & MSP_FLR_MASK_RFE) ||
			(drv_data->rx >= drv_data->rx_end))
			return;
		readl(MSP_DR(drv_data->regs));
		drv_data->rx += (drv_data->cur_chip->n_bytes);
	}
}


/**
 * msp_u8_writer - Write FIFO data in Data register as a 8 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function writes data in Tx FIFO till it is not full
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary write ptr tx in drv_data which maintains
 * current write position in transfer buffer. we do not write data more than
 * FIFO depth
 */
void msp_u8_writer(struct driver_data *drv_data)
{
	u32 cur_write = 0;
	u32 status;
	while (1) {
		status = readl(MSP_FLR(drv_data->regs));
		if ((status & MSP_FLR_MASK_TFU)
		    || (drv_data->tx >= drv_data->tx_end))
			return;
		writel((u32) (*(u8 *) (drv_data->tx)), MSP_DR(drv_data->regs));
		drv_data->tx += (drv_data->cur_chip->n_bytes);
		cur_write++;
		if (cur_write == MSP_FIFO_DEPTH)
			return;
	}
}

/**
 * msp_u8_reader - Read FIFO data in Data register as a 8 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function reads data in Rx FIFO till it is not empty
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary Read ptr rx in drv_data which maintains
 * current read position in transfer buffer
 */
void msp_u8_reader(struct driver_data *drv_data)
{
	u32 status;
	while (1) {
		status = readl(MSP_FLR(drv_data->regs));
		if ((status & MSP_FLR_MASK_RFE)
		    || (drv_data->rx >= drv_data->rx_end))
			return;
		*(u8 *) (drv_data->rx) = (u8) readl(MSP_DR(drv_data->regs));
		drv_data->rx += (drv_data->cur_chip->n_bytes);
	}
}

/**
 * msp_u16_writer - Write FIFO data in Data register as a 16 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function writes data in Tx FIFO till it is not full
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary write ptr tx in drv_data which maintains
 * current write position in transfer buffer. we do not write data more than
 * FIFO depth
 */
void msp_u16_writer(struct driver_data *drv_data)
{
	u32 cur_write = 0;
	u32 status;
	while (1) {
		status = readl(MSP_FLR(drv_data->regs));

		if ((status & MSP_FLR_MASK_TFU)
		    || (drv_data->tx >= drv_data->tx_end))
			return;
		writel((u32) (*(u16 *) (drv_data->tx)), MSP_DR(drv_data->regs));
		drv_data->tx += (drv_data->cur_chip->n_bytes);
		cur_write++;
		if (cur_write == MSP_FIFO_DEPTH)
			return;
	}
}

/**
 * msp_u16_reader - Read FIFO data in Data register as a 16 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function reads data in Rx FIFO till it is not empty
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary Read ptr rx in drv_data which maintains
 * current read position in transfer buffer
 */
void msp_u16_reader(struct driver_data *drv_data)
{
	u32 status;
	while (1) {
		status = readl(MSP_FLR(drv_data->regs));
		if ((status & MSP_FLR_MASK_RFE)
		    || (drv_data->rx >= drv_data->rx_end))
			return;
		*(u16 *) (drv_data->rx) = (u16) readl(MSP_DR(drv_data->regs));
		drv_data->rx += (drv_data->cur_chip->n_bytes);
	}
}

/**
 * msp_u32_writer - Write FIFO data in Data register as a 32 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function writes data in Tx FIFO till it is not full
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary write ptr tx in drv_data which maintains
 * current write position in transfer buffer. we do not write data more than
 * FIFO depth
 */
void msp_u32_writer(struct driver_data *drv_data)
{
	u32 cur_write = 0;
	u32 status;
	while (1) {
		status = readl(MSP_FLR(drv_data->regs));

		if ((status & MSP_FLR_MASK_TFU)
		    || (drv_data->tx >= drv_data->tx_end))
			return;
		/*Write Data to Data Register */
		writel(*(u32 *) (drv_data->tx), MSP_DR(drv_data->regs));
		drv_data->tx += (drv_data->cur_chip->n_bytes);
		cur_write++;
		if (cur_write == MSP_FIFO_DEPTH)
			return;
	}
}

/**
 * msp_u32_reader - Read FIFO data in Data register as a 32 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function reads data in Rx FIFO till it is not empty
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary Read ptr rx in drv_data which maintains
 * current read position in transfer buffer
 */
void msp_u32_reader(struct driver_data *drv_data)
{
	u32 status;
	while (1) {
		status = readl(MSP_FLR(drv_data->regs));
		if ((status & MSP_FLR_MASK_RFE)
		    || (drv_data->rx >= drv_data->rx_end))
			return;
		*(u32 *) (drv_data->rx) = readl(MSP_DR(drv_data->regs));
		drv_data->rx += (drv_data->cur_chip->n_bytes);
	}
}

static irqreturn_t stm_msp_interrupt_handler(int irq, void *dev_id)
{
	struct driver_data *drv_data = (struct driver_data *)dev_id;
	struct spi_message *msg = drv_data->cur_msg;
	u32 irq_status = 0;
	u32 flag = 0;
	if (!msg) {
		stm_error("bad message state in interrupt handler");
		/* Never fail */
		return IRQ_HANDLED;
	}
	/*Read the Interrupt Status Register */
	irq_status = readl(MSP_MIS(drv_data->regs));

	if (irq_status) {
		if (irq_status & MSP_MIS_MASK_ROEMIS) {	/*Overrun interrupt */
			/*Bail-out our Data has been corrupted */
			stm_dbg3(DBG_ST.msp, ":::: Received ROR interrupt\n");
			drv_data->execute_cmd(drv_data, DISABLE_ALL_INTERRUPT);
			drv_data->execute_cmd(drv_data, CLEAR_ALL_INTERRUPT);
			drv_data->execute_cmd(drv_data, DISABLE_CONTROLLER);
			msg->state = ERROR_STATE;
			tasklet_schedule(&drv_data->pump_transfers);
			return IRQ_HANDLED;
		}

		drv_data->read(drv_data);
		drv_data->write(drv_data);

		if ((drv_data->tx == drv_data->tx_end) && (flag == 0)) {
			flag = 1;
			/*Disable Transmit interrupt */
			writel(readl(MSP_IMSC(drv_data->regs)) &
			       (~MSP_IMSC_MASK_TXIM) & (~MSP_IMSC_MASK_TFOIM),
			       (drv_data->regs + 0x14));
		}
		/*Clearing any Xmit underrun error. overrun already handled */
		drv_data->execute_cmd(drv_data, CLEAR_ALL_INTERRUPT);

		if (drv_data->rx == drv_data->rx_end) {
			drv_data->execute_cmd(drv_data, DISABLE_ALL_INTERRUPT);
			drv_data->execute_cmd(drv_data, CLEAR_ALL_INTERRUPT);
			stm_dbg3(DBG_ST.msp,
				 ":::: Interrupt transfer Completed...\n");
			/* Update total bytes transfered */
			msg->actual_length += drv_data->cur_transfer->len;
			if (drv_data->cur_transfer->cs_change)
				drv_data->cur_chip->
				    cs_control(SPI_CHIP_DESELECT);
			/* Move to next transfer */
			msg->state = next_transfer(drv_data);
			tasklet_schedule(&drv_data->pump_transfers);
			return IRQ_HANDLED;
		}
	}
	return IRQ_HANDLED;
}

static int verify_msp_controller_parameters(struct msp_controller
					    *chip_info)
{
	/*FIXME: check clock params */
	if ((chip_info->lbm != LOOPBACK_ENABLED)
	    && (chip_info->lbm != LOOPBACK_DISABLED)) {
		stm_dbg(DBG_ST.msp,
			":::: Loopback Mode is configured incorrectly\n");
		return -1;
	}
	if (chip_info->iface != SPI_INTERFACE_MOTOROLA_SPI) {
		stm_dbg(DBG_ST.msp,
			":::: Interface is configured incorrectly."
			"This controller supports only MOTOROLA SPI\n");
		return -1;
	}
	if ((chip_info->hierarchy != SPI_MASTER)
	    && (chip_info->hierarchy != SPI_SLAVE)) {
		stm_dbg(DBG_ST.msp,
			":::: hierarchy is configured incorrectly\n");
		return -1;
	}
	if ((chip_info->endian_rx != SPI_FIFO_MSB)
	    && (chip_info->endian_rx != SPI_FIFO_LSB)) {
		stm_dbg(DBG_ST.msp,
			":::: Rx FIFO endianess is configured incorrectly\n");
		return -1;
	}
	if ((chip_info->endian_tx != SPI_FIFO_MSB)
	    && (chip_info->endian_tx != SPI_FIFO_LSB)) {
		stm_dbg(DBG_ST.msp,
			":::: Tx FIFO endianess is configured incorrectly\n");
		return -1;
	}
	if ((chip_info->data_size < MSP_DATA_BITS_8)
	    || (chip_info->data_size > MSP_DATA_BITS_32)) {
		stm_dbg(DBG_ST.msp,
			":::: MSP DATA Size is configured incorrectly\n");
		return -1;
	}
	if ((chip_info->com_mode != INTERRUPT_TRANSFER)
	    && (chip_info->com_mode != DMA_TRANSFER)
	    && (chip_info->com_mode != POLLING_TRANSFER)) {
		stm_dbg(DBG_ST.msp,
			":::: Communication mode is configured incorrectly\n");
		return -1;
	}
	if (chip_info->iface == SPI_INTERFACE_MOTOROLA_SPI) {
		if (((chip_info->proto_params).moto.clk_phase !=
		     SPI_CLK_ZERO_CYCLE_DELAY)
		    && ((chip_info->proto_params).moto.clk_phase !=
			SPI_CLK_HALF_CYCLE_DELAY)) {
			stm_dbg(DBG_ST.msp,
				":::: Clock Phase is configured incorrectly\n");
			return -1;
		}
		if (((chip_info->proto_params).moto.clk_pol !=
		     SPI_CLK_POL_IDLE_LOW)
		    && ((chip_info->proto_params).moto.clk_pol !=
			SPI_CLK_POL_IDLE_HIGH)) {
			stm_dbg(DBG_ST.msp,
				":::: Clk Polarity configured incorrectly\n");
			return -1;
		}
	}
	if (chip_info->cs_control == NULL) {
		stm_dbg(DBG_ST.msp,
			"::::Chip Select Function is NULL for this chip\n");
		chip_info->cs_control = null_cs_control;
	}
	return 0;
}

static int configure_dma_request(struct chip_data *curr_cfg,
		struct msp_controller *chip_info, struct driver_data *drv_data)
{
	int status = 0;
	curr_cfg->dma_info = kzalloc(sizeof(struct spi_dma_info), GFP_KERNEL);
	if (!curr_cfg->dma_info) {
		stm_dbg(DBG_ST.msp, "memory alloc failed for chip_data\n");
		return -ENOMEM;
	}
	status = process_spi_dma_info(
			(struct nmdkspi_dma *)chip_info->dma_config,
			curr_cfg, (void *)drv_data);
	if (status < 0) {
		kfree(curr_cfg->dma_info);
		return status;
	}
	return 0;
}


static struct msp_controller *allocate_default_msp_chip_cfg(
					struct spi_device *spi)
{
	struct msp_controller *chip_info;

	/* spi_board_info.controller_data not is supplied */
	chip_info = kzalloc(sizeof(struct msp_controller), GFP_KERNEL);
	if (!chip_info) {
		stm_error("setup - cannot allocate controller data");
		return NULL;
	}
	stm_dbg(DBG_ST.msp, ":::: Allocated Memory for controller data\n");

	chip_info->lbm = LOOPBACK_DISABLED;
	chip_info->com_mode = POLLING_TRANSFER;
	chip_info->iface = SPI_INTERFACE_MOTOROLA_SPI;
	chip_info->hierarchy = SPI_MASTER;
	chip_info->endian_tx = SPI_FIFO_LSB;
	chip_info->endian_rx = SPI_FIFO_LSB;
	chip_info->data_size = MSP_DATA_BITS_32;

	if (spi->max_speed_hz != 0)
		chip_info->freq = spi->max_speed_hz;
	else
		chip_info->freq = 48000;

	(chip_info->proto_params).moto.clk_phase = SPI_CLK_HALF_CYCLE_DELAY;
	(chip_info->proto_params).moto.clk_pol = SPI_CLK_POL_IDLE_LOW;
	chip_info->cs_control = null_cs_control;
	return chip_info;
}

static void msp_delay(struct driver_data *drv_data)
{
	udelay(15);
	while (readl(MSP_FLR(drv_data->regs))
		& (MSP_FLR_MASK_RBUSY | MSP_FLR_MASK_TBUSY))
			udelay(1);
}


/**
 * stm_msp_setup - setup function registered to SPI master framework
 * @spi: spi device which is requesting setup
 *
 * This function is registered to the SPI framework for this SPI master
 * controller. If it is the first time when setup is called by this device
 * , this function will initialize the runtime state for this chip and save
 * the same in the device structure. Else it will update the runtime info
 * with the updated chip info.
 */

static int stm_msp_setup(struct spi_device *spi)
{
	struct msp_controller *chip_info;
	struct chip_data *curr_cfg;
	struct spi_master *master;
	int status = 0;
	u16 sckdiv = 0;
	s16 bus_num = 0;
	struct driver_data *drv_data = spi_master_get_devdata(spi->master);
	struct msp_regs *msp_regs;
	master = drv_data->master;
	bus_num = master->bus_num - 1;


	/* Get controller data */
	chip_info = spi->controller_data;
	/* Get controller_state */
	curr_cfg = spi_get_ctldata(spi);

	if (curr_cfg == NULL) {
		curr_cfg = kzalloc(sizeof(struct chip_data), GFP_KERNEL);
		if (!curr_cfg) {
			stm_error("setup - cannot allocate controller state");
			return -ENOMEM;
		}
		curr_cfg->chip_id = spi->chip_select;
		curr_cfg->ctr_regs = kzalloc(sizeof(struct msp_regs),
						GFP_KERNEL);
		if (curr_cfg->ctr_regs == NULL) {
			stm_error("setup - cannot allocate mem for regs");
			goto err_first_setup;
		}
		stm_dbg(DBG_ST.msp, ":::: chip Id = %d\n", curr_cfg->chip_id);

		if (chip_info == NULL) {
			chip_info = allocate_default_msp_chip_cfg(spi);
			if (!chip_info) {
				stm_error("setup - cant allocate cntlr data");
				status = -ENOMEM;
				goto err_first_setup;
			}
			spi->controller_data = chip_info;
		}
	}



	if (chip_info->freq == 0) {
		/*Calculate Specific Freq. */
		if ((MSP_INTERNAL_CLK ==
		     chip_info->clk_freq.clk_src)
		    || (MSP_EXTERNAL_CLK ==
			chip_info->clk_freq.clk_src)) {
			sckdiv = chip_info->clk_freq.sckdiv;

		} else {
			status = -1;
			stm_error("setup - controller clock data is incorrect");
			goto err_config_params;
		}
	} else {
		/*Calculate Effective Freq. */
		sckdiv = ((DEFAULT_MSP_CLK) / (chip_info->freq)) - 1;
		if (sckdiv > MAX_SCKDIV) {
			stm_dbg(DBG_ST.msp,
				"SPI: Cannot set frequency less than 48Khz,"
				"setting lowest(48 Khz)\n");
			sckdiv = MAX_SCKDIV;
		}
	}

	status = verify_msp_controller_parameters(chip_info);
	if (status) {
		stm_error("setup - controller data is incorrect");
		goto err_config_params;
	}

	/* Now set controller state based on controller data */
	curr_cfg->xfer_type = chip_info->com_mode;
	curr_cfg->cs_control = chip_info->cs_control;
	curr_cfg->delay = msp_delay;
	curr_cfg->null_write = msp_null_writer;
	curr_cfg->null_read = msp_null_reader;

	/*FIXME: write all 8 & 16 bit functions */
	if (chip_info->data_size <= MSP_DATA_BITS_8) {
		stm_dbg(DBG_ST.msp, ":::: Less than 8 bits per word....\n");
		curr_cfg->n_bytes = 1;
		curr_cfg->read = msp_u8_reader;
		curr_cfg->write = msp_u8_writer;
	} else if (chip_info->data_size <= MSP_DATA_BITS_16) {
		stm_dbg(DBG_ST.msp, ":::: Less than 16 bits per word....\n");
		curr_cfg->n_bytes = 2;
		curr_cfg->read = msp_u16_reader;
		curr_cfg->write = msp_u16_writer;
	} else {
		stm_dbg(DBG_ST.msp, ":::: Less than 32 bits per word....\n");
		curr_cfg->n_bytes = 4;
		curr_cfg->read = msp_u32_reader;
		curr_cfg->write = msp_u32_writer;
	}

	/*Now Initialize all register settings reqd. for this chip */

	msp_regs = (struct msp_regs *)(curr_cfg->ctr_regs);
	msp_regs->gcr = 0x0;
	msp_regs->tcf = 0x0;
	msp_regs->rcf = 0x0;
	msp_regs->srg = 0x0;
	msp_regs->dmacr = 0x0;

	if ((chip_info->com_mode == DMA_TRANSFER)
	    && ((drv_data->master_info)->enable_dma)) {
		if (configure_dma_request(curr_cfg, chip_info, drv_data))
			goto err_dma_request;
		curr_cfg->enable_dma = 1;
		MSP_WBITS(msp_regs->dmacr, 0x1, MSP_DMACR_MASK_RDMAE, 0);
		MSP_WBITS(msp_regs->dmacr, 0x1, MSP_DMACR_MASK_TDMAE, 1);
	} else {
		curr_cfg->enable_dma = 0;
		stm_dbg(DBG_ST.msp,
			":::: DMA mode NOT set in controller state\n");
		MSP_WBITS(msp_regs->dmacr, 0x0, MSP_DMACR_MASK_RDMAE, 0);
		MSP_WBITS(msp_regs->dmacr, 0x0, MSP_DMACR_MASK_TDMAE, 1);
	}

	/****	GCR Reg Config	*****/

	MSP_WBITS(msp_regs->gcr,
			MSP_RECEIVER_DISABLED, MSP_GCR_MASK_RXEN, 0);
	MSP_WBITS(msp_regs->gcr,
			MSP_RX_FIFO_ENABLED, MSP_GCR_MASK_RFFEN, 1);
	MSP_WBITS(msp_regs->gcr,
			MSP_TRANSMITTER_DISABLED, MSP_GCR_MASK_TXEN, 8);
	MSP_WBITS(msp_regs->gcr,
			MSP_TX_FIFO_ENABLED, MSP_GCR_MASK_TFFEN, 9);
	MSP_WBITS(msp_regs->gcr,
			MSP_TX_FRAME_SYNC_POL_LOW, MSP_GCR_MASK_TFSPOL, 10);
	MSP_WBITS(msp_regs->gcr,
			MSP_TX_FRAME_SYNC_INT, MSP_GCR_MASK_TFSSEL, 11);
	MSP_WBITS(msp_regs->gcr,
			MSP_TRANSMIT_DATA_WITH_DELAY, MSP_GCR_MASK_TXDDL, 15);
	MSP_WBITS(msp_regs->gcr,
			MSP_SAMPLE_RATE_GEN_ENABLE, MSP_GCR_MASK_SGEN, 16);
	MSP_WBITS(msp_regs->gcr,
			MSP_CLOCK_INTERNAL, MSP_GCR_MASK_SCKSEL, 18);
	MSP_WBITS(msp_regs->gcr,
			MSP_FRAME_GEN_ENABLE, MSP_GCR_MASK_FGEN, 20);
	MSP_WBITS(msp_regs->gcr,
			SPI_BURST_MODE_DISABLE, MSP_GCR_MASK_SPIBME, 23);

	if (chip_info->lbm == LOOPBACK_ENABLED)
		MSP_WBITS(msp_regs->gcr,
				MSP_LOOPBACK_ENABLED, MSP_GCR_MASK_LBM, 7);
	else
		MSP_WBITS(msp_regs->gcr,
				MSP_LOOPBACK_DISABLED, MSP_GCR_MASK_LBM, 7);

	if (chip_info->hierarchy == SPI_MASTER)
		MSP_WBITS(msp_regs->gcr,
				MSP_IS_SPI_MASTER, MSP_GCR_MASK_TCKSEL, 14);
	else
		MSP_WBITS(msp_regs->gcr,
				MSP_IS_SPI_SLAVE, MSP_GCR_MASK_TCKSEL, 14);

	if (chip_info->proto_params.moto.clk_phase == SPI_CLK_ZERO_CYCLE_DELAY)
		MSP_WBITS(msp_regs->gcr,
				MSP_SPI_PHASE_ZERO_CYCLE_DELAY,
				MSP_GCR_MASK_SPICKM, 21);
	else
		MSP_WBITS(msp_regs->gcr,
				MSP_SPI_PHASE_HALF_CYCLE_DELAY,
				MSP_GCR_MASK_SPICKM, 21);

	if (chip_info->proto_params.moto.clk_pol == SPI_CLK_POL_IDLE_HIGH)
		MSP_WBITS(msp_regs->gcr,
				MSP_TX_CLOCK_POL_HIGH, MSP_GCR_MASK_TCKPOL, 13);
	else
		MSP_WBITS(msp_regs->gcr,
				MSP_TX_CLOCK_POL_LOW, MSP_GCR_MASK_TCKPOL, 13);

	/****	RCF Reg Config	*****/
	MSP_WBITS(msp_regs->rcf,
			MSP_IGNORE_RX_FRAME_SYNC_PULSE, MSP_RCF_MASK_RFSIG, 15);
	MSP_WBITS(msp_regs->rcf,
			MSP_RX_1BIT_DATA_DELAY, MSP_RCF_MASK_RDDLY, 13);
	if (chip_info->endian_rx == SPI_FIFO_LSB)
		MSP_WBITS(msp_regs->rcf,
				MSP_RX_ENDIANESS_LSB, MSP_RCF_MASK_RENDN, 12);
	else
		MSP_WBITS(msp_regs->rcf,
				MSP_RX_ENDIANESS_MSB, MSP_RCF_MASK_RENDN, 12);

	MSP_WBITS(msp_regs->rcf, chip_info->data_size, MSP_RCF_MASK_RP1ELEN, 0);

	/****	TCF Reg Config	*****/
	MSP_WBITS(msp_regs->tcf,
			MSP_IGNORE_TX_FRAME_SYNC_PULSE, MSP_TCF_MASK_TFSIG, 15);
	MSP_WBITS(msp_regs->tcf,
			MSP_TX_1BIT_DATA_DELAY, MSP_TCF_MASK_TDDLY, 13);
	if (chip_info->endian_rx == SPI_FIFO_LSB)
		MSP_WBITS(msp_regs->tcf,
				MSP_TX_ENDIANESS_LSB, MSP_TCF_MASK_TENDN, 12);
	else
		MSP_WBITS(msp_regs->tcf,
				MSP_TX_ENDIANESS_MSB, MSP_TCF_MASK_TENDN, 12);
	MSP_WBITS(msp_regs->tcf, chip_info->data_size, MSP_TCF_MASK_TP1ELEN, 0);

	/****	SRG Reg Config	*****/
	MSP_WBITS(msp_regs->srg, sckdiv, MSP_SRG_MASK_SCKDIV, 0);

	/* Save controller_state */
	spi_set_ctldata(spi, curr_cfg);
	return status;

err_dma_request:
err_config_params:
err_first_setup:
	kfree(curr_cfg->dma_info);
	kfree(curr_cfg);
	return status;
}

static int stm_msp_probe(struct amba_device *adev, struct amba_id *id)
{
	struct device *dev = &adev->dev;
	struct nmdk_spi_master_cntlr *platform_info;
	struct spi_master *master;
	struct driver_data *drv_data = NULL;	/*Data for this driver */
	struct resource *res;
	int irq, status = 0;

	platform_info = (struct nmdk_spi_master_cntlr *)(dev->platform_data);
	if (platform_info == NULL) {
		stm_error("probe - no platform data supplied\n");
		status = -ENODEV;
		goto err_no_pdata;
	}
	/* Allocate master with space for drv_data */
	master = spi_alloc_master(dev, sizeof(struct driver_data));
	if (master == NULL) {
		stm_error("probe - cannot alloc spi_master\n");
		status = -ENOMEM;
		goto err_no_mem;
	}

	drv_data = spi_master_get_devdata(master);
	drv_data->master = master;
	drv_data->master_info = platform_info;
	drv_data->adev = adev;

	drv_data->dma_ongoing = 0;

	drv_data->clk = clk_get(&adev->dev, NULL);
	if (IS_ERR(drv_data->clk)) {
		stm_error("probe - cannot find clock\n");
		status = PTR_ERR(drv_data->clk);
		goto free_master;
	}

	/*Fetch the Resources, using platform data */
	res = &(adev->res);
	if (res == NULL) {
		stm_error("probe - MEM resources not defined\n");
		status = -ENODEV;
		goto disable_clk;
	}
	/*Get Hold of Device Register Area... */
	drv_data->regs = ioremap(res->start, (res->end - res->start));
	if (drv_data->regs == NULL) {
		status = -ENODEV;
		goto disable_clk;
	}
	irq = adev->irq[0];
	if (irq <= 0) {
		status = -ENODEV;
		goto err_no_iores;
	}

	stm_dbg(DBG_ST.msp, ":::: mSP Controller  %d\n", platform_info->id);
	drv_data->execute_cmd = msp_controller_cmd;
	drv_data->execute_cmd(drv_data, LOAD_DEFAULT_CONFIG);

	/*Required Info for an SPI controller */
	/*Bus Number Which Assigned to this SPI controller on this board */
	master->bus_num = (u16) platform_info->id;
	master->num_chipselect = platform_info->num_chipselect;
	master->setup = stm_msp_setup;
	master->cleanup = (void *)stm_spi_cleanup;
	master->transfer = stm_spi_transfer;

	stm_dbg(DBG_ST.msp, ":::: BUSNO: %d\n", master->bus_num);
	status = spi_register_master(master);
	if (status != 0) {
		stm_error("probe - problem registering spi master\n");
		goto err_spi_register;
	}

	/* Initialize and start queue */
	status = init_queue(drv_data);
	if (status != 0) {
		stm_error("probe - problem initializing queue\n");
		goto err_init_queue;
	}
	status = start_queue(drv_data);
	if (status != 0) {
		stm_error("probe - problem starting queue\n");
		goto err_start_queue;
	}

	/*Initialize tasklet for DMA transfer */
	tasklet_init(&drv_data->spi_dma_tasklet, stm_spi_tasklet,
			(unsigned long)drv_data);

	/* Register with the SPI framework */
	platform_set_drvdata(adev, drv_data);
	stm_dbg(DBG_ST.msp, "probe succeded\n");
	stm_dbg(DBG_ST.msp, "Bus No = %d, IRQ Line = %d, Virtual Addr: %x\n",
			master->bus_num, irq, (u32) (drv_data->regs));

	status =
		stm_gpio_altfuncenable(drv_data->master_info->gpio_alt_func);
	if (status < 0) {
		stm_error("probe - unable to set GPIO Altfunc, %d\n",
				drv_data->master_info->gpio_alt_func);
		status = -ENODEV;
		goto err_init_queue;
	}
	status =
		request_irq(drv_data->adev->irq[0],
				stm_msp_interrupt_handler,
				0, drv_data->master_info->device_name,
				drv_data);
	if (status < 0) {
		stm_error("probe - cannot get IRQ (%d)\n",
				status);
		goto err_irq;
	}

	return 0;

err_irq:
	stm_gpio_altfuncdisable(drv_data->master_info->gpio_alt_func);
err_init_queue:
err_start_queue:
err_spi_register:
	destroy_queue(drv_data);
err_no_iores:
	iounmap(drv_data->regs);
disable_clk:
	clk_put(drv_data->clk);
free_master:
	spi_master_put(master);
err_no_mem:
err_no_pdata:
	return status;
}

static int msp_remove(struct amba_device *adev)
{
	struct driver_data *drv_data = platform_get_drvdata(adev);
	struct device *dev = &adev->dev;
	struct nmdk_spi_master_cntlr *platform_info;
	int status = 0;
	if (!drv_data)
		return 0;

	platform_info = dev->platform_data;

	/* Remove the queue */
	status = destroy_queue(drv_data);
	if (status != 0) {
		stm_error("queue remove failed (%d)\n", status);
		return status;
	}
	drv_data->execute_cmd(drv_data, LOAD_DEFAULT_CONFIG);

	/* Release map resources */
	iounmap(drv_data->regs);
	tasklet_disable(&drv_data->pump_transfers);
	tasklet_kill(&drv_data->spi_dma_tasklet);

	stm_gpio_altfuncdisable(platform_info->gpio_alt_func);
	free_irq(drv_data->adev->irq[0], drv_data);

	/* Disconnect from the SPI framework */
	spi_unregister_master(drv_data->master);

	clk_put(drv_data->clk);

	/* Prevent double remove */
	platform_set_drvdata(adev, NULL);
	stm_dbg(DBG_ST.msp, "remove succeded\n");
	return status;
}

static struct amba_id msp_ids[] = {
	{
	 .id = MSP_PER_ID,
	 .mask = MSP_PER_MASK,
	 },
	{0, 0},
};
#ifdef CONFIG_PM
/**
 * stm_msp_suspend - MSP suspend function registered with PM framework.
 * @dev: Reference to amba device structure of the device
 * @state: power mgmt state.
 *
 * This function is invoked when the system is going into sleep, called
 * by the power management framework of the linux kernel.
 *
 */
int stm_msp_suspend(struct amba_device *adev, pm_message_t state)
{
	struct driver_data *drv_data = platform_get_drvdata(adev);
	int status = 0;

	status = stop_queue(drv_data);
	if (status != 0) {
		dev_warn(&adev->dev, "suspend cannot stop queue\n");
		return status;
	}

	stm_dbg(DBG_ST.msp, "suspended\n");
	return 0;
}

/**
 * stm_msp_resume - MSP Resume function registered with PM framework.
 * @dev: Reference to amba device structure of the device
 *
 * This function is invoked when the system is coming out of sleep, called
 * by the power management framework of the linux kernel.
 *
 */
int stm_msp_resume(struct amba_device *pdev)
{
	struct driver_data *drv_data = platform_get_drvdata(pdev);
	int status = 0;

	/* Start the queue running */
	status = start_queue(drv_data);
	if (status != 0)
		stm_error("problem starting queue (%d)\n", status);
	else
		stm_dbg(DBG_ST.msp, "resumed\n");

	return status;
}

#else
#define stm_msp_suspend NULL
#define stm_msp_resume NULL
#endif /* CONFIG_PM */

static struct amba_driver msp_driver = {
	.drv = {
		.name = "MSP",
		},
	.id_table = msp_ids,
	.probe = stm_msp_probe,
	.remove = msp_remove,
	.suspend = stm_msp_suspend,
	.resume = stm_msp_resume,
};

static int __init stm_msp_mod_init(void)
{
	return amba_driver_register(&msp_driver);
}

static void __exit stm_msp_exit(void)
{
	amba_driver_unregister(&msp_driver);
	return;
}

module_init(stm_msp_mod_init);
module_exit(stm_msp_exit);

MODULE_AUTHOR("Sachin Verma <sachin.verma@st.com>");
MODULE_DESCRIPTION("STM MSP (SPI protocol) Driver");
MODULE_LICENSE("GPL");
