/*
 * drivers/spi/spi-stm.c
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
#include <linux/init.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <linux/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <linux/delay.h>

#include <mach/hardware.h>
#include <linux/gpio.h>
#include <mach/debug.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <asm/dma.h>
#include <mach/dma.h>
#include <linux/spi/spi-stm.h>

/***************************************************************************/

#define STM_SPI_NAME		"DRIVER SPI"

#define DRIVER_DEBUG    	0
#define DRIVER_DEBUG_PFX        STM_SPI_NAME
#define DRIVER_DBG 	        KERN_ERR


/*	Queue State	*/
#define QUEUE_RUNNING                   (0)
#define QUEUE_STOPPED                   (1)

/**
 * process_ssp_dma_info - Processes the DMA info provided by client drivers
 * @chip_info: chip info provided by client device
 * @chip: Runtime state maintained by the spi controller for each spi device
 *
 * This function processes and stores DMA config provided by client driver
 * into the runtime state maintained by the spi controller driver
 */
int process_spi_dma_info(struct nmdkspi_dma *dma_config,
			 struct chip_data *chip, void *data)
{
	struct driver_data *drv_data = (struct driver_data *)data;

	struct stm_dma_pipe_info *rx_pipe = &(chip->dma_info->rx_dma_info);
	struct stm_dma_pipe_info *tx_pipe = &(chip->dma_info->tx_dma_info);


	struct stm_spi_dma_pipe_config *tx_cfg = NULL;
	struct stm_spi_dma_pipe_config *rx_cfg = NULL;


	if (!chip->dma_info) {
		stm_dbg(DBG_ST.spi, "No DMA info supplied\n");
		return -1;
	}

	tx_cfg = &(dma_config->tx_cfg);
	rx_cfg = &(dma_config->rx_cfg);

	/*Configuring Rx Pipe::*/

	rx_pipe->dir = rx_cfg->dma_dir;

	if (rx_pipe->dir == PERIPH_TO_MEM) {
		rx_pipe->channel_type =
			(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE |
			 LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK |
			 NON_SECURE_CHANNEL | LOW_PRIORITY_CHANNEL);
		rx_pipe->dst_addr = NULL;

	} else if (rx_pipe->dir == PERIPH_TO_PERIPH) {
		rx_pipe->channel_type =
			(STANDARD_CHANNEL | CHANNEL_IN_PHYSICAL_MODE
			 | PCHAN_BASIC_MODE | NO_TIM_FOR_LINK
			 | NON_SECURE_CHANNEL | LOW_PRIORITY_CHANNEL);
		rx_pipe->dst_addr = (void *)rx_cfg->client_fifo_addr;

	} else {
		stm_dbg(DBG_ST.spi, "Incorrect Rx pipe Info\n");
		return -1;
	}

	rx_pipe->src_dev_type = drv_data->master_info->rx_fifo_dev_type;
	rx_pipe->dst_dev_type = rx_cfg->cli_dev_type.dst_dev_type;

	rx_pipe->src_info.endianess = rx_cfg->src_info.endianess;
	rx_pipe->src_info.data_width = rx_cfg->src_info.data_width;
	rx_pipe->src_info.burst_size = rx_cfg->src_info.burst_size;
	rx_pipe->src_info.buffer_type = rx_cfg->src_info.buffer_type;

	rx_pipe->dst_info.endianess = rx_cfg->dst_info.endianess;
	rx_pipe->dst_info.data_width = rx_cfg->dst_info.data_width;
	rx_pipe->dst_info.burst_size = rx_cfg->dst_info.burst_size;
	rx_pipe->dst_info.buffer_type = rx_cfg->dst_info.buffer_type;

	rx_pipe->src_addr = (void *)drv_data->master_info->rx_fifo_addr;

	/*Configuring Tx Pipe::*/

	tx_pipe->dir = tx_cfg->dma_dir;

	if (tx_pipe->dir == MEM_TO_PERIPH) {
		tx_pipe->channel_type =
			(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE |
			 LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK |
			 NON_SECURE_CHANNEL | LOW_PRIORITY_CHANNEL);
		tx_pipe->src_addr = NULL;

	} else if (tx_pipe->dir == PERIPH_TO_PERIPH) {
		tx_pipe->channel_type =
			(STANDARD_CHANNEL | CHANNEL_IN_PHYSICAL_MODE
			 | PCHAN_BASIC_MODE | NO_TIM_FOR_LINK
			 | NON_SECURE_CHANNEL | LOW_PRIORITY_CHANNEL);
		tx_pipe->src_addr = (void *)tx_cfg->client_fifo_addr;
	} else {
		stm_dbg(DBG_ST.spi, "Incorrect Tx pipe Info\n");
		return -1;
	}

	tx_pipe->dst_dev_type = drv_data->master_info->tx_fifo_dev_type;
	tx_pipe->src_dev_type = tx_cfg->cli_dev_type.dst_dev_type;

	tx_pipe->src_info.endianess = tx_cfg->src_info.endianess;
	tx_pipe->src_info.data_width = tx_cfg->src_info.data_width;
	tx_pipe->src_info.burst_size = tx_cfg->src_info.burst_size;
	tx_pipe->src_info.buffer_type = tx_cfg->src_info.buffer_type;

	tx_pipe->dst_info.endianess = tx_cfg->dst_info.endianess;
	tx_pipe->dst_info.data_width = tx_cfg->dst_info.data_width;
	tx_pipe->dst_info.burst_size = tx_cfg->dst_info.burst_size;
	tx_pipe->dst_info.buffer_type = tx_cfg->dst_info.buffer_type;

	tx_pipe->dst_addr = (void *)drv_data->master_info->tx_fifo_addr;

	return 0;
}
EXPORT_SYMBOL(process_spi_dma_info);
/***************************************************************************/

/**
 * null_cs_control - Dummy chip select function
 * @command: select/delect the chip
 *
 * If no chip select function is provided by client this is used as dummy
 * chip select
 */
void null_cs_control(u32 command)
{
	stm_dbg(DBG_ST.spi, "::::Dummy chip select control\n");
}
EXPORT_SYMBOL(null_cs_control);


/**
 * giveback - current spi_message is over, schedule next spi_message
 * @message: current SPI message
 * @drv_data: spi driver private data structure
 *
 * current spi_message is over, schedule next spi_message and call
 * callback of this msg.
 */
void giveback(struct spi_message *message, struct driver_data *drv_data)
{
	struct spi_transfer *last_transfer;
	unsigned long flags;
	struct spi_message *msg;
	void (*curr_cs_control) (u32 command);

	spin_lock_irqsave(&drv_data->lock, flags);
	msg = drv_data->cur_msg;

	curr_cs_control = drv_data->cur_chip->cs_control;
	drv_data->cur_msg = NULL;
	drv_data->cur_transfer = NULL;
	drv_data->cur_chip = NULL;
#ifdef CONFIG_SPI_WORKQUEUE
	queue_work(drv_data->workqueue, &drv_data->spi_work);
#else
	schedule_work(&drv_data->spi_work);
#endif
	spin_unlock_irqrestore(&drv_data->lock, flags);

	last_transfer = list_entry(msg->transfers.prev,
			struct spi_transfer, transfer_list);
	if (!last_transfer->cs_change)
		curr_cs_control(SPI_CHIP_DESELECT);
	msg->state = NULL;
	if (msg->complete)
		msg->complete(msg->context);
	drv_data->execute_cmd(drv_data, DISABLE_CONTROLLER);
	clk_disable(drv_data->clk);
}
EXPORT_SYMBOL(giveback);

/**
 * next_transfer - Move to the Next transfer in the current spi message
 * @drv_data: spi driver private data structure
 *
 * This function moves though the linked list of spi transfers in the
 * current spi message and returns with the state of current spi
 * message i.e whether its last transfer is done(DONE_STATE) or
 * Next transfer is ready(RUNNING_STATE)
 */
void *next_transfer(struct driver_data *drv_data)
{
	struct spi_message *msg = drv_data->cur_msg;
	struct spi_transfer *trans = drv_data->cur_transfer;
	/* Move to next transfer */
	if (trans->transfer_list.next != &msg->transfers) {
		drv_data->cur_transfer =
			list_entry(trans->transfer_list.next,
					struct spi_transfer, transfer_list);
		return RUNNING_STATE;
	}
	return DONE_STATE;
}
EXPORT_SYMBOL(next_transfer);

/**
 * pump_transfers - Tasklet function which schedules next interrupt xfer
 * @data: spi driver private data structure
 *
 */
static void pump_transfers(unsigned long data)
{
	struct driver_data *drv_data = (struct driver_data *)data;
	struct spi_message *message = NULL;
	struct spi_transfer *transfer = NULL;
	struct spi_transfer *previous = NULL;

	message = drv_data->cur_msg;
	/* Handle for abort */
	if (message->state == ERROR_STATE) {
		message->status = -EIO;
		giveback(message, drv_data);
		return;
	}
	/* Handle end of message */
	if (message->state == DONE_STATE) {
		message->status = 0;
		giveback(message, drv_data);
		return;
	}
	transfer = drv_data->cur_transfer;
	/* Delay if requested at end of transfer */
	if (message->state == RUNNING_STATE) {
		previous =
			list_entry(transfer->transfer_list.prev,
					struct spi_transfer, transfer_list);
		if (previous->delay_usecs)
			udelay(previous->delay_usecs);
		if (previous->cs_change)
			drv_data->cur_chip->cs_control(SPI_CHIP_SELECT);
	} else {
		/* START_STATE */
		message->state = RUNNING_STATE;
	}
	drv_data->tx = (void *)transfer->tx_buf;
	drv_data->tx_end = drv_data->tx + drv_data->cur_transfer->len;
	drv_data->rx = (void *)transfer->rx_buf;
	drv_data->rx_end = drv_data->rx + drv_data->cur_transfer->len;

	drv_data->write = drv_data->tx ?
		drv_data->cur_chip->write : drv_data->cur_chip->null_write;
	drv_data->read = drv_data->rx ?
		drv_data->cur_chip->read : drv_data->cur_chip->null_read;

	drv_data->execute_cmd(drv_data, FLUSH_FIFO);
	drv_data->execute_cmd(drv_data, ENABLE_ALL_INTERRUPT);
}


static int setup_rx_dma(void *data)
{
	int status = 0;
	struct driver_data *drv_data = (struct driver_data *)data;
	status = stm_configure_dma_channel(drv_data->rx_dmach ,
			&(drv_data->cur_chip->dma_info->rx_dma_info));

	if (status) {
		stm_dbg(DBG_ST.spi, "dma cfg of Rx channel failed\n");
		return status;
	}
	if (drv_data->cur_chip->dma_info->rx_dma_info.dir == PERIPH_TO_PERIPH) {
		stm_set_dma_addr(drv_data->rx_dmach,
		(void *)(drv_data->cur_chip->dma_info->rx_dma_info.src_addr),
		(void *)(drv_data->cur_chip->dma_info->rx_dma_info.dst_addr));

	} else {
		if (drv_data->cur_msg->is_dma_mapped == 0) {
			drv_data->cur_transfer->rx_dma =
				dma_map_single(&(drv_data->adev->dev),
					(void *)drv_data->cur_transfer->rx_buf,
					drv_data->cur_transfer->len,
					DMA_FROM_DEVICE);
		}

		stm_set_dma_addr(drv_data->rx_dmach,
				(void *)(drv_data->master_info->rx_fifo_addr),
				(void *)(drv_data->cur_transfer->rx_dma));
	}
	stm_set_dma_count(drv_data->rx_dmach ,
			drv_data->cur_transfer->len);
	return status;
}

static int setup_tx_dma(void *data)
{
	int status = 0;
	struct driver_data *drv_data = (struct driver_data *)data;


	status = stm_configure_dma_channel(drv_data->tx_dmach ,
			&(drv_data->cur_chip->dma_info->tx_dma_info));
	if (status) {
		stm_dbg(DBG_ST.spi, "dma cfg of tx channel failed\n");
		return status;
	}

	if (drv_data->cur_chip->dma_info->tx_dma_info.dir == PERIPH_TO_PERIPH) {
		stm_set_dma_addr(drv_data->tx_dmach,
		(void *)(drv_data->cur_chip->dma_info->tx_dma_info.src_addr),
		(void *)(drv_data->cur_chip->dma_info->tx_dma_info.dst_addr));

	} else {
		if (drv_data->cur_msg->is_dma_mapped == 0) {
			void *tx = (void *)drv_data->cur_transfer->tx_buf;
			drv_data->cur_transfer->tx_dma =
				dma_map_single(&(drv_data->adev->dev),
						tx,
						drv_data->cur_transfer->len,
						DMA_TO_DEVICE);
		}
		stm_set_dma_addr(drv_data->tx_dmach,
				(void *)(drv_data->cur_transfer->tx_dma),
				(void *)(drv_data->master_info->tx_fifo_addr));
	}
	stm_set_dma_count(drv_data->tx_dmach ,
			drv_data->cur_transfer->len);
	return status;
}


void stm_spi_tasklet(unsigned long param)
{
	struct driver_data *drv_data = (struct driver_data *)param;
	struct spi_message *msg = drv_data->cur_msg;
	struct spi_transfer *previous = NULL;
	/*DMA complete. schedule next xfer */
	/*DISABLE DMA, and flush FIFO of SPI Controller */
	drv_data->execute_cmd(drv_data, DISABLE_DMA);
	drv_data->execute_cmd(drv_data, FLUSH_FIFO);
	msg->actual_length += drv_data->cur_transfer->len;
	if (drv_data->cur_transfer->cs_change)
		drv_data->cur_chip->cs_control(SPI_CHIP_DESELECT);
	if (msg->is_dma_mapped == 0) {
		if (drv_data->cur_transfer->tx_buf)
			dma_unmap_single(&(drv_data->adev->dev),
					drv_data->cur_transfer->tx_dma,
					drv_data->cur_transfer->len,
					DMA_TO_DEVICE);
		if (drv_data->cur_transfer->rx_buf)
			dma_unmap_single(&(drv_data->adev->dev),
					drv_data->cur_transfer->rx_dma,
					drv_data->cur_transfer->len,
					DMA_FROM_DEVICE);
	}


	msg->state = next_transfer(drv_data);
	if (msg->state == ERROR_STATE)
		goto handle_dma_error;
	else if (msg->state == DONE_STATE) {
		drv_data->execute_cmd(drv_data, DISABLE_CONTROLLER);
		msg->status = 0;
		if (drv_data->rx_dmach != -1) {
			stm_free_dma(drv_data->rx_dmach);
			drv_data->rx_dmach = -1;
		}
		if (drv_data->tx_dmach != -1) {
			stm_free_dma(drv_data->tx_dmach);
			drv_data->tx_dmach = -1;
		}
		giveback(msg, drv_data);
		return ;
	}
	/* Delay if requested at end of transfer */
	else if (msg->state == RUNNING_STATE) {
		previous =
			list_entry(drv_data->cur_transfer->transfer_list.
					prev, struct spi_transfer,
					transfer_list);
		if (previous->delay_usecs)
			udelay(previous->delay_usecs);
		if (previous->cs_change)
			drv_data->cur_chip->cs_control(SPI_CHIP_SELECT);
	} else
		goto handle_dma_error;

	stm_dbg(DBG_ST.spi, "Inside stm_spi_tasklet()\n");
	if (drv_data->cur_transfer->tx_buf) {
		atomic_inc(&drv_data->dma_cnt);
		setup_tx_dma(drv_data);
		if (stm_enable_dma(drv_data->tx_dmach)) {
			stm_dbg(DBG_ST.spi,
				"Tx stm_enable_dma failed inside tasklet\n");
			goto handle_dma_error;
		}
	}
	if (drv_data->cur_transfer->rx_buf) {
		atomic_inc(&drv_data->dma_cnt);
		setup_rx_dma(drv_data);
		if (stm_enable_dma(drv_data->rx_dmach)) {
			stm_dbg(DBG_ST.spi,
				"Rx stm_enable_dma failed inside tasklet\n");
			goto handle_dma_error;
		}
	}
	/*Enable DMA for this chip */
	drv_data->execute_cmd(drv_data, ENABLE_DMA);
	return ;

handle_dma_error:
	atomic_set(&drv_data->dma_cnt, 0);
	drv_data->execute_cmd(drv_data, DISABLE_DMA);
	msg->status = -EIO;
	if (msg->is_dma_mapped == 0) {
		if (drv_data->cur_transfer->tx_buf)
			dma_unmap_single(&(drv_data->adev->dev),
					drv_data->cur_transfer->tx_dma,
					drv_data->cur_transfer->len,
					DMA_TO_DEVICE);
		if (drv_data->cur_transfer->rx_buf)
			dma_unmap_single(&(drv_data->adev->dev),
					drv_data->cur_transfer->rx_dma,
					drv_data->cur_transfer->len,
					DMA_FROM_DEVICE);
	}
	if (drv_data->rx_dmach != -1) {
		stm_free_dma(drv_data->rx_dmach);
		drv_data->rx_dmach = -1;
	}
	if (drv_data->tx_dmach != -1) {
		stm_free_dma(drv_data->tx_dmach);
		drv_data->tx_dmach = -1;
	}
	giveback(msg, drv_data);
	return ;
}
EXPORT_SYMBOL(stm_spi_tasklet);

/**
 * spi_dma_callback_handler - This function is invoked when dma xfer is complete
 * @param: context data which is drivers private data
 * @event: Status of current DMA transfer
 *
 * This function checks if DMA transfer is complete for current transfer
 * It fills the Rx Tx buffer pointers again and launch dma for next
 * transfer from this callback handler itself
 *
 */
irqreturn_t spi_dma_callback_handler(void *param, int event)
{
	struct driver_data *drv_data = (struct driver_data *)param;
	int flag = 0;
	stm_dbg(DBG_ST.spi, "spi_dma_callback_handler() called\n");

	smp_mb();
	if (atomic_dec_and_test(&drv_data->dma_cnt))
		flag = 1;
	smp_mb();
	if (flag == 1)
		tasklet_schedule(&drv_data->spi_dma_tasklet);
	return IRQ_HANDLED;
}
EXPORT_SYMBOL(spi_dma_callback_handler);

/**
 * do_dma_transfer - It handles xfers of the current message if it is DMA xfer
 * @data: spi driver's private data structure
 *
 *
 *
 */
static void do_dma_transfer(void *data)
{
	struct driver_data *drv_data = (struct driver_data *)data;

	atomic_set(&drv_data->dma_cnt, 0);
	drv_data->cur_chip->cs_control(SPI_CHIP_SELECT);

	if (drv_data->cur_transfer->rx_buf) {
		atomic_inc(&drv_data->dma_cnt);
		if (stm_request_dma(&(drv_data->rx_dmach),
				&(drv_data->cur_chip->dma_info->rx_dma_info))) {
			stm_dbg(DBG_ST.ssp, "Request DMA failed\n");
			goto err_dma_transfer;
		}
		setup_rx_dma(data);
		stm_set_callback_handler(drv_data->rx_dmach,
				&spi_dma_callback_handler,
				(void *)drv_data);
	}
	if (drv_data->cur_transfer->tx_buf) {
		atomic_inc(&drv_data->dma_cnt);
		if (stm_request_dma(&(drv_data->tx_dmach),
				&(drv_data->cur_chip->dma_info->tx_dma_info))) {
			stm_dbg(DBG_ST.ssp, "Request DMA failed\n");
			goto err_dma_transfer;
		}
		setup_tx_dma(data);
		stm_set_callback_handler(drv_data->tx_dmach,
				&spi_dma_callback_handler,
				(void *)drv_data);
	}

	if (stm_enable_dma(drv_data->rx_dmach))
		goto err_dma_transfer;
	if (stm_enable_dma(drv_data->tx_dmach))
		goto err_dma_transfer;

	/*Enable SPI Controller */
	drv_data->execute_cmd(drv_data, ENABLE_CONTROLLER);
	return;

err_dma_transfer:
	if (drv_data->cur_msg->is_dma_mapped == 0) {
		if (drv_data->cur_transfer->tx_buf)
			dma_unmap_single(&(drv_data->adev->dev),
					drv_data->cur_transfer->tx_dma,
					drv_data->cur_transfer->len,
					DMA_TO_DEVICE);
		if (drv_data->cur_transfer->rx_buf)
			dma_unmap_single(&(drv_data->adev->dev),
					drv_data->cur_transfer->rx_dma,
					drv_data->cur_transfer->len,
					DMA_FROM_DEVICE);
	}
	if (drv_data->rx_dmach != -1) {
		stm_free_dma(drv_data->rx_dmach);
		drv_data->rx_dmach = -1;
	}
	if (drv_data->tx_dmach != -1) {
		stm_free_dma(drv_data->tx_dmach);
		drv_data->tx_dmach = -1;
	}
	drv_data->cur_msg->state = ERROR_STATE;
	drv_data->cur_msg->status = -EIO;
	giveback(drv_data->cur_msg, drv_data);
	return;
}

static void do_interrupt_transfer(void *data)
{
	struct driver_data *drv_data = (struct driver_data *)data;

	drv_data->tx = (void *)drv_data->cur_transfer->tx_buf;
	drv_data->tx_end = drv_data->tx + drv_data->cur_transfer->len;
	drv_data->rx = (void *)drv_data->cur_transfer->rx_buf;
	drv_data->rx_end = drv_data->rx + drv_data->cur_transfer->len;

	drv_data->write = drv_data->tx ?
		drv_data->cur_chip->write : drv_data->cur_chip->null_write;
	drv_data->read = drv_data->rx ?
		drv_data->cur_chip->read : drv_data->cur_chip->null_read;

	drv_data->cur_chip->cs_control(SPI_CHIP_SELECT);
	drv_data->execute_cmd(drv_data, ENABLE_ALL_INTERRUPT);
	drv_data->execute_cmd(drv_data, ENABLE_CONTROLLER);
}

static void do_polling_transfer(void *data)
{
	struct driver_data *drv_data = (struct driver_data *)data;
	struct spi_message *message = NULL;
	struct spi_transfer *transfer = NULL;
	struct spi_transfer *previous = NULL;
	struct chip_data *chip;
	unsigned long limit = 0;
	u32 timer_expire = 0;

	chip = drv_data->cur_chip;
	message = drv_data->cur_msg;

	while (message->state != DONE_STATE) {
		/* Handle for abort */
		if (message->state == ERROR_STATE)
			break;
		transfer = drv_data->cur_transfer;

		/* Delay if requested at end of transfer */
		if (message->state == RUNNING_STATE) {
			previous =
				list_entry(transfer->transfer_list.prev,
					struct spi_transfer, transfer_list);
			if (previous->delay_usecs)
				udelay(previous->delay_usecs);
			if (previous->cs_change)
				drv_data->cur_chip->cs_control(SPI_CHIP_SELECT);
		} else {
			/* START_STATE */
			message->state = RUNNING_STATE;
			drv_data->cur_chip->cs_control(SPI_CHIP_SELECT);
		}

		/*Configuration Changing Per Transfer */
		drv_data->tx = (void *)transfer->tx_buf;
		drv_data->tx_end = drv_data->tx + drv_data->cur_transfer->len;
		drv_data->rx = (void *)transfer->rx_buf;
		drv_data->rx_end = drv_data->rx + drv_data->cur_transfer->len;

		drv_data->write = drv_data->tx ?
			drv_data->cur_chip->write :
			drv_data->cur_chip->null_write;
		drv_data->read =  drv_data->rx ?
			drv_data->cur_chip->read :
			drv_data->cur_chip->null_read;
		drv_data->delay = drv_data->cur_chip->delay;

		drv_data->execute_cmd(drv_data, FLUSH_FIFO);
		drv_data->execute_cmd(drv_data, ENABLE_CONTROLLER);

		timer_expire = drv_data->cur_transfer->len / 1024;
		if (!timer_expire)
			timer_expire = 5000;
		else
			timer_expire =
				(drv_data->cur_transfer->len / 1024) * 5000;
		drv_data->spi_notify_timer.expires =
			jiffies + msecs_to_jiffies(timer_expire);
		add_timer(&drv_data->spi_notify_timer);

		stm_dbg(DBG_ST.spi, ":::: POLLING TRANSFER ONGOING ... \n");
		while (drv_data->tx < drv_data->tx_end) {
			drv_data->execute_cmd(drv_data, DISABLE_CONTROLLER);
			drv_data->read(drv_data);
			drv_data->write(drv_data);
			drv_data->execute_cmd(drv_data, ENABLE_CONTROLLER);
			if (drv_data->delay)
				drv_data->delay(drv_data);
			if (drv_data->spi_io_error == 1)
				break;

		}

		del_timer(&drv_data->spi_notify_timer);
		if (drv_data->spi_io_error == 1)
			goto out;

		limit = loops_per_jiffy << 1;

		while ((drv_data->rx < drv_data->rx_end) && (limit--))
			drv_data->read(drv_data);

		/* Update total byte transfered */
		message->actual_length += drv_data->cur_transfer->len;
		if (drv_data->cur_transfer->cs_change)
			drv_data->cur_chip->cs_control(SPI_CHIP_DESELECT);
		drv_data->execute_cmd(drv_data, DISABLE_CONTROLLER);

		/* Move to next transfer */
		message->state = next_transfer(drv_data);
	}

out:
	/* Handle end of message */
	if (message->state == DONE_STATE)
		message->status = 0;
	else
		message->status = -EIO;

	giveback(message, drv_data);
	drv_data->spi_io_error = 0; /* Reset state for further transfers */

	return;
}
/**
 * pump_messages - Workqueue function which processes spi message queue
 * @data: pointer to private data of spi driver
 *
 * This function checks if there is any spi message in the queue that
 * needs processing and delegate control to appropriate function
 * do_polling_transfer()/do_interrupt_transfer()/do_dma_transfer()
 * based on the kind of the transfer
 *
 */
static void pump_messages(struct work_struct *work)
{
	struct driver_data *drv_data = container_of(work,
			struct driver_data, spi_work);
	unsigned long flags;

	/* Lock queue and check for queue work */
	spin_lock_irqsave(&drv_data->lock, flags);
	if (list_empty(&drv_data->queue) || drv_data->run == QUEUE_STOPPED) {
		stm_dbg(DBG_ST.spi, ":::: work_queue: Queue Empty\n");
		drv_data->busy = 0;
		spin_unlock_irqrestore(&drv_data->lock, flags);
		return;
	}
	/* Make sure we are not already running a message */
	if (drv_data->cur_msg) {
		spin_unlock_irqrestore(&drv_data->lock, flags);
		return;
	}

	clk_enable(drv_data->clk);

	/* Extract head of queue */
	drv_data->cur_msg =
		list_entry(drv_data->queue.next, struct spi_message, queue);

	list_del_init(&drv_data->cur_msg->queue);
	drv_data->busy = 1;
	spin_unlock_irqrestore(&drv_data->lock, flags);

	/* Initial message state */
	drv_data->cur_msg->state = START_STATE;
	drv_data->cur_transfer = list_entry(drv_data->cur_msg->transfers.next,
			struct spi_transfer, transfer_list);

	/* Setup the SPI using the per chip configuration */
	drv_data->cur_chip = spi_get_ctldata(drv_data->cur_msg->spi);
	drv_data->execute_cmd(drv_data, RESTORE_STATE);
	drv_data->execute_cmd(drv_data, FLUSH_FIFO);

	if (drv_data->cur_chip->xfer_type == POLLING_TRANSFER)
		do_polling_transfer(drv_data);
	else if (drv_data->cur_chip->xfer_type == INTERRUPT_TRANSFER)
		do_interrupt_transfer(drv_data);
	else /* DMA_TRANSFER*/
		do_dma_transfer(drv_data);
}

/**
 * spi_notify - Handles Polling hang issue over spi bus.
 * @data: main driver data
 * Context: Process.
 *
 * This is  used to handle error condition in transfer and receive function used
 * in polling mode.
 * Sometimes due to passing wrong protocol desc , polling transfer may hang.
 * To prevent this, timer is added.
 *
 * Returns void.
 */
static void spi_notify(unsigned long data)
{
	struct driver_data *dspi = (struct driver_data *)data;
	dspi->spi_io_error = 1;
	printk(KERN_ERR
		"Polling is taking time, maybe device not responding\n");
	del_timer(&dspi->spi_notify_timer);
}

int init_queue(struct driver_data *drv_data)
{
	INIT_LIST_HEAD(&drv_data->queue);
	spin_lock_init(&drv_data->lock);

	drv_data->run = QUEUE_STOPPED;
	drv_data->busy = 0;

	tasklet_init(&drv_data->pump_transfers, pump_transfers,
			(unsigned long)drv_data);
	INIT_WORK(&drv_data->spi_work, pump_messages);
#ifdef CONFIG_SPI_WORKQUEUE
	drv_data->workqueue = create_singlethread_workqueue(
		dev_name(&drv_data->master->dev));


	if (drv_data->workqueue == NULL)
		return -EBUSY;
#endif
	init_timer(&drv_data->spi_notify_timer);
	drv_data->spi_notify_timer.expires = jiffies + msecs_to_jiffies(1000);
	drv_data->spi_notify_timer.function = spi_notify;
	drv_data->spi_notify_timer.data = (unsigned long)drv_data;

	return 0;
}
EXPORT_SYMBOL(init_queue);

int start_queue(struct driver_data *drv_data)
{
	unsigned long flags;
	spin_lock_irqsave(&drv_data->lock, flags);
	if (drv_data->run == QUEUE_RUNNING || drv_data->busy) {
		spin_unlock_irqrestore(&drv_data->lock, flags);
		return -EBUSY;
	}
	drv_data->run = QUEUE_RUNNING;
	drv_data->cur_msg = NULL;
	drv_data->cur_transfer = NULL;
	drv_data->cur_chip = NULL;
	spin_unlock_irqrestore(&drv_data->lock, flags);
	/*queue_work(drv_data->workqueue, &drv_data->pump_messages);*/
	/*schedule_work(&drv_data->spi_work);*/
	return 0;
}
EXPORT_SYMBOL(start_queue);

int stop_queue(struct driver_data *drv_data)
{
	unsigned long flags;
	unsigned limit = 500;
	int status = 0;

	spin_lock_irqsave(&drv_data->lock, flags);

	/* This is a bit lame, but is optimized for the common execution path.
	 * A wait_queue on the drv_data->busy could be used, but then the common
	 * execution path (pump_messages) would be required to call wake_up or
	 * friends on every SPI message. Do this instead */
	drv_data->run = QUEUE_STOPPED;
	while (!list_empty(&drv_data->queue) && drv_data->busy && limit--) {
		spin_unlock_irqrestore(&drv_data->lock, flags);
		msleep(10);
		spin_lock_irqsave(&drv_data->lock, flags);
	}
	if (!list_empty(&drv_data->queue) || drv_data->busy)
		status = -EBUSY;

	spin_unlock_irqrestore(&drv_data->lock, flags);

	return status;
}
EXPORT_SYMBOL(stop_queue);

int destroy_queue(struct driver_data *drv_data)
{
	int status;
	status = stop_queue(drv_data);
	if (status != 0)
		return status;
#ifdef CONFIG_SPI_WORKQUEUE
	destroy_workqueue(drv_data->workqueue);
#endif
	del_timer_sync(&drv_data->spi_notify_timer);
	return 0;
}
EXPORT_SYMBOL(destroy_queue);

/**
 * stm_spi_transfer - transfer function registered to SPI master framework
 * @spi: spi device which is requesting transfer
 * @msg: spi message which is to handled is queued to driver queue
 *
 * This function is registered to the SPI framework for this SPI master
 * controller. It will queue the spi_message in the queue of driver if
 * the queue is not stopped and return.
 */
int stm_spi_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct driver_data *drv_data = spi_master_get_devdata(spi->master);
	unsigned long flags;

	spin_lock_irqsave(&drv_data->lock, flags);

	if (drv_data->run == QUEUE_STOPPED) {
		spin_unlock_irqrestore(&drv_data->lock, flags);
		return -ESHUTDOWN;
	}
	stm_dbg(DBG_ST.spi, ":::: Regular request (No infinite DMA ongoing)\n");

	msg->actual_length = 0;
	msg->status = -EINPROGRESS;
	msg->state = START_STATE;

	list_add_tail(&msg->queue, &drv_data->queue);
	if (drv_data->run == QUEUE_RUNNING && !drv_data->busy)
#ifdef CONFIG_SPI_WORKQUEUE
		queue_work(drv_data->workqueue, &drv_data->spi_work);
#else
	schedule_work(&drv_data->spi_work);
#endif

	spin_unlock_irqrestore(&drv_data->lock, flags);
	return 0;
}
EXPORT_SYMBOL(stm_spi_transfer);


/**
 * stm_spi_cleanup - cleanup function registered to SPI master framework
 * @spi: spi device which is requesting cleanup
 *
 * This function is registered to the SPI framework for this SPI master
 * controller. It will free the runtime state of chip.
 */
void stm_spi_cleanup(struct spi_device *spi)
{
	struct chip_data *chip = spi_get_ctldata((struct spi_device *)spi);
	struct driver_data *drv_data = spi_master_get_devdata(spi->master);
	struct spi_master *master;
	master = drv_data->master;

	if (chip) {
		kfree(chip->dma_info);
		kfree(chip->ctr_regs);
		kfree(chip);
		spi_set_ctldata(spi, NULL);
	}
}
EXPORT_SYMBOL(stm_spi_cleanup);

MODULE_AUTHOR("Sachin Verma <sachin.verma@st.com>");
MODULE_DESCRIPTION("SPI driver");
MODULE_LICENSE("GPL");
