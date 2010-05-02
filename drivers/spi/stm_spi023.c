/*
 * drivers/spi/stm_spi023.c
 *
 * Copyright (C) 2008 STMicroelectronics Pvt. Ltd.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#include <linux/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>

#include <mach/hardware.h>
#include <linux/gpio.h>
#include <mach/debug.h>
#include <asm/dma.h>
#include <mach/dma.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-stm.h>
#include <linux/spi/stm_spi023.h>

/***************************************************************************/
#define SPI_DRIVER_VERSION "0.1.0"

#define STM_SPI023_NAME		"STM_SPI023"

#define DRIVER_DEBUG    0
#define DRIVER_DEBUG_PFX  STM_SPI023_NAME
#define DRIVER_DBG      KERN_ERR              /* message level */

#define SPI_WBITS(reg, val, mask, sb) \
		((reg) =   (((reg) & ~(mask)) | (((val)<<(sb)) & (mask))))
/***************************************************************************/

#define FALSE 				(0)
#define TRUE 				(1)

int spi023_eff_freq(int freq, t_spi_clock_params *clk_freq)
{
	/*Lets calculate the frequency parameters */
	u32 cpsdvsr = 2;
	u32 scr = 0;
	int freq_found = FALSE;
	u32 max_tclk;
	u32 min_tclk;
	/* cpsdvscr = 2 & scr 0 */
	max_tclk = (STM_SPICTLR_CLOCK_FREQ / (MIN_CPSDVR * (1 + MIN_SCR)));
	/* cpsdvsr = 254 & scr = 255 */
	min_tclk = (STM_SPICTLR_CLOCK_FREQ / (MAX_CPSDVR * (1 + MAX_SCR)));

	if ((freq <= max_tclk) && (freq >= min_tclk)) {
		while (cpsdvsr <= MAX_CPSDVR && !freq_found) {
			while (scr <= MAX_SCR && !freq_found) {
				if ((STM_SPICTLR_CLOCK_FREQ /
						(cpsdvsr * (1 + scr))) > freq)
					scr += 1;
				else {
					/* This bool is made TRUE when
					 * effective frequency >= target freq
					 * is found */
					freq_found = TRUE;
					if ((STM_SPICTLR_CLOCK_FREQ /
						(cpsdvsr * (1 + scr)))
							!= freq) {
						if (scr == MIN_SCR) {
							cpsdvsr -= 2;
							scr = MAX_SCR;
						} else
							scr -= 1;
					}
				}
			}
			if (!freq_found) {
				cpsdvsr += 2;
				scr = MIN_SCR;
			}
		}
		if (cpsdvsr != 0) {
			stm_dbg(DBG_ST.spi, ":::: SSP Effec Freq is 0x%x\n",
					(STM_SPICTLR_CLOCK_FREQ /
					 (cpsdvsr * (1 + scr))));
			clk_freq->cpsdvsr = (u8) (cpsdvsr & 0xFF);
			clk_freq->scr = (u8) (scr & 0xFF);
			stm_dbg(DBG_ST.spi, ":::: SSP cpsdvsr = %d, scr = %d\n",
					clk_freq->cpsdvsr, clk_freq->scr);
		}
	} else {
		/*User is asking for out of range Freq. */
		stm_dbg(DBG_ST.spi, ":::: setup - ctlr data incorrect:"
				"Out of Range Frequency");
		return -EINVAL;
	}
	return 0;
}


/**
 * spi023_controller_cmd - To execute controller specific commands
 * @drv_data: SPI driver private data structure
 * @cmd: Command which is to be executed on the controller
 *
 *
 */
static int spi023_controller_cmd(struct driver_data *drv_data, int cmd)
{
	int retval = 0;
	struct spi023_regs *ctr_regs = NULL;

	switch (cmd) {
	case DISABLE_CONTROLLER:
		{
			stm_dbg2(DBG_ST.spi, ":::: DISABLE_CONTROLLER\n");
			writel((readl(SPI_CR1(drv_data->regs)) &
						(~SPI_CR1_MASK_SSE)),
						SPI_CR1(drv_data->regs));
			break;
		}
	case ENABLE_CONTROLLER:
		{
			stm_dbg2(DBG_ST.spi, ":::: ENABLE_CONTROLLER\n");
			writel((readl(SPI_CR1(drv_data->regs)) |
						SPI_CR1_MASK_SSE),
						SPI_CR1(drv_data->regs));
			break;
		}
	case DISABLE_DMA:
		{
			stm_dbg2(DBG_ST.spi, ":::: DISABLE_DMA\n");
			/*As DEFAULT_SPI_REG_DMACR has DMA disabled */
			writel(DEFAULT_SPI_REG_DMACR,
					SPI_DMACR(drv_data->regs));
				break;
		}
	case ENABLE_DMA:
		{
			ctr_regs = drv_data->cur_chip->ctr_regs;
			stm_dbg2(DBG_ST.spi, ":::: ENABLE_CONTROLLER\n");
			writel(ctr_regs->dmacr, SPI_DMACR(drv_data->regs));
			break;
		}
	case DISABLE_ALL_INTERRUPT:
		{
			stm_dbg2(DBG_ST.spi, ":::: DISABLE_ALL_INTERRUPT\n");
			writel(DISABLE_ALL_SPI_INTERRUPTS,
					SPI_IMSC(drv_data->regs));
			break;
		}
	case ENABLE_ALL_INTERRUPT:
		{
			stm_dbg2(DBG_ST.spi, ":::: ENABLE_ALL_INTERRUPT\n");
			writel(ENABLE_ALL_SPI_INTERRUPTS,
					SPI_IMSC(drv_data->regs));
				break;
		}
	case CLEAR_ALL_INTERRUPT:
		{
			writel(CLEAR_ALL_SPI_INTERRUPTS,
					SPI_ICR(drv_data->regs));
			break;
		}
	case FLUSH_FIFO:
		{
			unsigned long limit = loops_per_jiffy << 1;
			stm_dbg2(DBG_ST.spi, "::: DATA FIFO is flushed\n");
			do {
				while (readl(SPI_SR(drv_data->regs)) &
						SPI_SR_MASK_RNE)
						readl(SPI_DR(drv_data->regs));
			} while ((readl(SPI_SR(drv_data->regs)) &
						SPI_SR_MASK_BSY) && limit--);
			retval = limit;
			break;
		}
	case RESTORE_STATE:
		{
			ctr_regs = drv_data->cur_chip->ctr_regs;
			stm_dbg2(DBG_ST.spi, ":::: RESTORE_STATE\n");
			writel(ctr_regs->cr0, SPI_CR0(drv_data->regs));
			writel(ctr_regs->cr1, SPI_CR1(drv_data->regs));
			writel(ctr_regs->dmacr,
					SPI_DMACR(drv_data->regs));
			writel(ctr_regs->cpsr, SPI_CPSR(drv_data->regs));
			writel(DISABLE_ALL_SPI_INTERRUPTS,
					SPI_IMSC(drv_data->regs));
			writel(CLEAR_ALL_SPI_INTERRUPTS,
					SPI_ICR(drv_data->regs));
			break;
		}
	case LOAD_DEFAULT_CONFIG:
		{
			stm_dbg2(DBG_ST.spi, ":::: LOAD_DEFAULT_CONFIG\n");
			writel(DEFAULT_SPI_REG_CR0, SPI_CR0(drv_data->regs));
			writel(DEFAULT_SPI_REG_CR1, SPI_CR1(drv_data->regs));
			writel(DEFAULT_SPI_REG_DMACR,
					SPI_DMACR(drv_data->regs));
			writel(DEFAULT_SPI_REG_CPSR, SPI_CPSR(drv_data->regs));
			writel(DISABLE_ALL_SPI_INTERRUPTS,
					SPI_IMSC(drv_data->regs));
			writel(CLEAR_ALL_SPI_INTERRUPTS,
					SPI_ICR(drv_data->regs));
			break;
		}
	default:
		{
			stm_dbg2(DBG_ST.spi, ":::: unknown command\n");
			retval = -1;
			break;
		}
	}
	return retval;
}

/**
 * spi023_u8_writer - Write FIFO data in Data register as a 8 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function writes data in Tx FIFO till it is not full
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary write ptr tx in drv_data which maintains
 * current write position in transfer buffer
 */
static void spi023_u8_writer(struct driver_data *drv_data)
{
	/*While Transmit Fifo is not Full
	 * (bit SPI_SR_MASK_TNF == 0 in status Reg)
	 * or our data to transmit is finished */
	while ((readl(SPI_SR(drv_data->regs)) & SPI_SR_MASK_TNF)
			&& (drv_data->tx < drv_data->tx_end)) {
		/*Write Data to Data Register */
		writel(*(u8 *) (drv_data->tx), SPI_DR(drv_data->regs));
		drv_data->tx += (drv_data->cur_chip->n_bytes);
	}
}

/**
 * spi023_u8_reader - Read FIFO data in Data register as a 8 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function reads data in Rx FIFO till it is not empty
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary Read ptr rx in drv_data which maintains
 * current read position in transfer buffer
 */
static void spi023_u8_reader(struct driver_data *drv_data)
{
	/*While Receive Fifo is not Empty
	 * (bit SPI_SR_MASK_RNE == 0 in status Reg)
	 * or We have received Data we wanted to receive */
	while ((readl(SPI_SR(drv_data->regs)) & SPI_SR_MASK_RNE)
			&& (drv_data->rx < drv_data->rx_end)) {
		*(u8 *) (drv_data->rx) = readl(SPI_DR(drv_data->regs));
		drv_data->rx += (drv_data->cur_chip->n_bytes);
	}
}

/**
 * spi023_u16_writer - Write FIFO data in Data register as a 16 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function writes data in Tx FIFO till it is not full
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary write ptr tx in drv_data which maintains
 * current write position in transfer buffer
 */
static void spi023_u16_writer(struct driver_data *drv_data)
{
	/*While Transmit Fifo is not Full
	 * (bit SPI_SR_MASK_TNF == 0 in status Reg)
	 * or our data to transmit is finished */
	while ((readl(SPI_SR(drv_data->regs)) & SPI_SR_MASK_TNF)
			&& (drv_data->tx < drv_data->tx_end)) {
		/*Write Data to Data Register */
		writel((u32) (*(u16 *) (drv_data->tx)), SPI_DR(drv_data->regs));
		drv_data->tx += (drv_data->cur_chip->n_bytes);
	}
}

/**
 * spi023_u16_reader - Read FIFO data in Data register as a 16 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function reads data in Rx FIFO till it is not empty
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary Read ptr rx in drv_data which maintains
 * current read position in transfer buffer
 */
static void spi023_u16_reader(struct driver_data *drv_data)
{
	/*While Receive Fifo is not Empty
	 * (bit SPI_SR_MASK_RNE == 0 in status Reg)
	 * or We have received Data we wanted to receive */
	while ((readl(SPI_SR(drv_data->regs)) & SPI_SR_MASK_RNE)
			&& (drv_data->rx < drv_data->rx_end)) {
		*(u16 *) (drv_data->rx) = (u16) readl(SPI_DR(drv_data->regs));
		drv_data->rx += (drv_data->cur_chip->n_bytes);
	}
}

/**
 * spi023_u32_writer - Write FIFO data in Data register as a 32 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function writes data in Tx FIFO till it is not full
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary write ptr tx in drv_data which maintains
 * current write position in transfer buffer
 */
static void spi023_u32_writer(struct driver_data *drv_data)
{
	/*While Transmit Fifo is not Full
	 * (bit SPI_SR_MASK_TNF == 0 in status Reg)
	 * or our data to transmit is finished */
	while ((readl(SPI_SR(drv_data->regs)) & SPI_SR_MASK_TNF)
			&& (drv_data->tx < drv_data->tx_end)) {
		/*Write Data to Data Register */
		writel(*(u32 *) (drv_data->tx), SPI_DR(drv_data->regs));
		drv_data->tx += (drv_data->cur_chip->n_bytes);
	}

}

/**
 * spi023_u32_reader - Read FIFO data in Data register as a 32 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function reads data in Rx FIFO till it is not empty
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary Read ptr rx in drv_data which maintains
 * current read position in transfer buffer
 */
static void spi023_u32_reader(struct driver_data *drv_data)
{
	/*While Receive Fifo is not Empty
	 * (bit SPI_SR_MASK_RNE == 0 in status Reg)
	 * or We have received Data we wanted to receive */
	while ((readl(SPI_SR(drv_data->regs)) & SPI_SR_MASK_RNE)
			&& (drv_data->rx < drv_data->rx_end)) {
		*(u32 *) (drv_data->rx) = readl(SPI_DR(drv_data->regs));
		drv_data->rx += (drv_data->cur_chip->n_bytes);
	}
}
static void spi023_delay(struct driver_data *drv_data)
{
	while (readl(SPI_SR(drv_data->regs)) & SPI_SR_MASK_BSY)
		udelay(1);
}

/**
 * spi023_interrupt_handler - Interrupt handler for spi controller
 *
 *
 * This function handles interrupts generated for an interrupt based transfer.
 * If a receive overrun (ROR) interrupt is there then we disable SPI, flag the
 * current msg's state as ERROR_STATE and schedule the tasklet pump_transfers
 * which will do the postprocessing of the current msg by calling giveback().
 * Otherwise it reads data from Rx FIFO till there is no more data, and
 * writes data in Tx FIFO till it is not full.
 * If we complete the transfer we move to the next transfer and
 * schedule the tasklet
 *
 */
static irqreturn_t spi023_interrupt_handler(int irq, void *dev_id)
{
	struct driver_data *drv_data = (struct driver_data *)dev_id;
	struct spi_message *msg = drv_data->cur_msg;
	u32 irq_status = 0;
	u32 flag = 0;
	if (!msg) {
		dev_err(&drv_data->adev->dev,
				"bad message state in interrupt handler");
		/* Never fail */
		return IRQ_HANDLED;
	}
	/*Read the Interrupt Status Register */
	irq_status = readl(SPI_MIS(drv_data->regs));

	if (irq_status) {
		if (irq_status & SPI_MIS_MASK_RORMIS) {	/*Overrun interrupt */
			/*Bail-out our Data has been corrupted */
			stm_dbg3(DBG_ST.spi, ":::: Received ROR interrupt\n");
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
			writel(readl(SPI_IMSC(drv_data->regs)) &
					(~SPI_IMSC_MASK_TXIM),
					SPI_IMSC(drv_data->regs));
		}
		if (drv_data->rx == drv_data->rx_end) {
			drv_data->execute_cmd(drv_data, DISABLE_ALL_INTERRUPT);
			drv_data->execute_cmd(drv_data, CLEAR_ALL_INTERRUPT);
			stm_dbg3(DBG_ST.spi, ": Intpt transfer Completed...\n");
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

int verify_spi_controller_parameters(struct spi023_controller *chip_info)
{
	if ((chip_info->lbm != LOOPBACK_ENABLED)
			&& (chip_info->lbm != LOOPBACK_DISABLED)) {
		stm_dbg(DBG_ST.spi, ": Loopback Mode configured incorrectly\n");
		return -1;
	}
	if ((chip_info->iface < SPI_INTERFACE_MOTOROLA_SPI)
			|| (chip_info->iface > SPI_INTERFACE_UNIDIRECTIONAL)) {
		stm_dbg(DBG_ST.spi, ": Interface is configured incorrectly\n");
		return -1;
	}
	if ((chip_info->hierarchy != SPI_MASTER)
			&& (chip_info->hierarchy != SPI_SLAVE)) {
		stm_dbg(DBG_ST.spi, ": hierarchy is configured incorrectly\n");
		return -1;
	}
	if ((chip_info->clk_freq.cpsdvsr < MIN_CPSDVR)
			|| (chip_info->clk_freq.cpsdvsr > MAX_CPSDVR)) {
		stm_dbg(DBG_ST.spi, ":::: cpsdvsr is configured incorrectly\n");
		return -1;
	}
	if ((chip_info->endian_rx != SPI_FIFO_MSB)
			&& (chip_info->endian_rx != SPI_FIFO_LSB)) {
		stm_dbg(DBG_ST.spi, ":::: Rx FIFO endianess cfg incorrect\n");
		return -1;
	}
	if ((chip_info->endian_tx != SPI_FIFO_MSB)
			&& (chip_info->endian_tx != SPI_FIFO_LSB)) {
		stm_dbg(DBG_ST.spi, ":::: Tx FIFO endianess cfg incorrect\n");
		return -1;
	}
	if ((chip_info->data_size < SPI_DATA_BITS_4)
			|| (chip_info->data_size > SPI_DATA_BITS_32)) {
		stm_dbg(DBG_ST.spi, ": DATA Size is configured incorrectly\n");
		return -1;
	}
	if ((chip_info->com_mode != INTERRUPT_TRANSFER)
			&& (chip_info->com_mode != DMA_TRANSFER)
			&& (chip_info->com_mode != POLLING_TRANSFER)) {
		stm_dbg(DBG_ST.spi, ": Communication mode incorrect\n");
		return -1;
	}

	if ((chip_info->dma_xfer_type != SPI_WITH_PERIPH)
			&& (chip_info->dma_xfer_type != SPI_WITH_MEM)) {
		stm_dbg(DBG_ST.spi, ":::: DMA xfer type incorrect\n");
		return -1;
	}

	if ((chip_info->rx_lev_trig < SPI_RX_1_OR_MORE_ELEM)
			|| (chip_info->rx_lev_trig >
				SPI_RX_16_OR_MORE_ELEM)) {
		stm_dbg
			(DBG_ST.spi, ":::: Rx FIFO Trigger Level incorrect\n");
		return -1;
	}
	if ((chip_info->tx_lev_trig <
				SPI_TX_1_OR_MORE_EMPTY_LOC)
			|| (chip_info->tx_lev_trig >
				SPI_TX_16_OR_MORE_EMPTY_LOC)) {
		stm_dbg
			(DBG_ST.spi, ":::: Tx FIFO Trigger Level incorrect\n");
		return -1;
	}
	if (chip_info->iface == SPI_INTERFACE_MOTOROLA_SPI) {
		if (((chip_info->proto_params).moto.clk_phase !=
					SPI_CLK_ZERO_CYCLE_DELAY)
				&& ((chip_info->proto_params).moto.clk_phase !=
					SPI_CLK_HALF_CYCLE_DELAY)) {
			stm_dbg
				(DBG_ST.spi, ":::: Clock Phase incorrect\n");
			return -1;
		}
		if (((chip_info->proto_params).moto.clk_pol !=
					SPI_CLK_POL_IDLE_LOW)
				&& ((chip_info->proto_params).moto.clk_pol !=
					SPI_CLK_POL_IDLE_HIGH)) {
			stm_dbg
				(DBG_ST.spi, ":::: Clock Polarity incorrect\n");
			return -1;
		}
	}
	if (chip_info->cs_control == NULL) {
		stm_dbg(DBG_ST.spi, "::::Chip Select Fn NULL for this chip\n");
		chip_info->cs_control = null_cs_control;
	}
	return 0;
}

static int configure_dma_request(struct chip_data *curr_cfg,
		struct spi023_controller *chip_info,
		struct driver_data *drv_data)
{
	int status = 0;
	curr_cfg->dma_info = kzalloc(sizeof(struct spi_dma_info), GFP_KERNEL);
	if (!curr_cfg->dma_info) {
		stm_dbg(DBG_ST.spi, "memory alloc failed for chip_data\n");
		return -ENOMEM;
	}

	status = process_spi_dma_info(
			(struct nmdkspi_dma *)&(chip_info->dma_config),
			curr_cfg, (void *)drv_data);
	if (status < 0) {
		kfree(curr_cfg->dma_info);
		return status;
	}
	return 0;

}

static struct spi023_controller *allocate_default_chip_cfg(
		struct spi_device *spi)
{
	struct spi023_controller *chip_info;

	/* spi_board_info.controller_data not is supplied */
	chip_info = kzalloc(sizeof(struct spi023_controller), GFP_KERNEL);
	if (!chip_info) {
		stm_error("setup - cannot allocate controller data");
		return NULL;
	}

	stm_dbg(DBG_ST.spi, ":::: Allocated Memory for controller data\n");

	chip_info->lbm = LOOPBACK_DISABLED;
	chip_info->com_mode = POLLING_TRANSFER; /*DEFAULT MODE*/
	chip_info->iface = SPI_INTERFACE_MOTOROLA_SPI;
	chip_info->hierarchy = SPI_MASTER;
	chip_info->slave_tx_disable = DO_NOT_DRIVE_TX;
	chip_info->endian_tx = SPI_FIFO_LSB;
	chip_info->endian_rx = SPI_FIFO_LSB;
	chip_info->data_size = SPI_DATA_BITS_12;

	if (spi->max_speed_hz != 0) {
		chip_info->freq = spi->max_speed_hz;
		chip_info->clk_freq.cpsdvsr = 0;
		chip_info->clk_freq.scr = 0;
	} else {
		chip_info->freq = 0;
		chip_info->clk_freq.cpsdvsr = STM_SPI_DEFAULT_PRESCALE;
		chip_info->clk_freq.scr = STM_SPI_DEFAULT_CLKRATE;
	}
	chip_info->rx_lev_trig = SPI_RX_1_OR_MORE_ELEM;
	chip_info->tx_lev_trig = SPI_TX_1_OR_MORE_EMPTY_LOC;
	(chip_info->proto_params).moto.clk_phase = SPI_CLK_HALF_CYCLE_DELAY;
	(chip_info->proto_params).moto.clk_pol = SPI_CLK_POL_IDLE_LOW;
	chip_info->cs_control = null_cs_control;
	return chip_info;
}


/**
 * stm_spi023_setup - setup function registered to SPI master framework
 * @spi: spi device which is requesting setup
 *
 * This function is registered to the SPI framework for this SPI master
 * controller. If it is the first time when setup is called by this device
 * , this function will initialize the runtime state for this chip and save
 * the same in the device structure. Else it will update the runtime info
 * with the updated chip info.
 */
static int stm_spi023_setup(struct spi_device *spi)
{
	struct spi023_controller *chip_info;
	struct chip_data *curr_cfg;
	int status = 0;
	struct driver_data *drv_data = spi_master_get_devdata(spi->master);
	struct spi023_regs *regs;

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
		curr_cfg->ctr_regs = kzalloc(sizeof(struct spi023_regs),
				GFP_KERNEL);
		if (curr_cfg->ctr_regs == NULL) {
			stm_error("setup - cannot allocate mem for regs");
			goto err_first_setup;
		}
		stm_dbg(DBG_ST.spi, ":::: chip Id = %d\n", curr_cfg->chip_id);

		if (chip_info == NULL) {
			chip_info = allocate_default_chip_cfg(spi);
			if (!chip_info) {
				stm_error("setup - cant allocate cntlr data");
				status = -ENOMEM;
				goto err_first_setup;
			}
			spi->controller_data = chip_info;
		}
	}

	if ((0 == chip_info->clk_freq.cpsdvsr)
			&& (0 == chip_info->clk_freq.scr)) {
		status = spi023_eff_freq(chip_info->freq, &chip_info->clk_freq);
		if (status < 0)
			goto err_config_params;
	} else {
		if ((chip_info->clk_freq.cpsdvsr % 2) != 0)
			chip_info->clk_freq.cpsdvsr =
				chip_info->clk_freq.cpsdvsr - 1;
	}
	status = verify_spi_controller_parameters(chip_info);
	if (status) {
		dev_err(&spi->dev, "setup - controller data is incorrect");
		goto err_config_params;
	}
	/* Now set controller state based on controller data */
	curr_cfg->xfer_type = chip_info->com_mode;
	curr_cfg->cs_control = chip_info->cs_control;

	if (chip_info->data_size <= 8) {
		stm_dbg(DBG_ST.spi, ":::: Less than 8 bits per word....\n");
		curr_cfg->n_bytes = 1;
		curr_cfg->read = spi023_u8_reader;
		curr_cfg->write = spi023_u8_writer;
	} else if (chip_info->data_size <= 16) {
		stm_dbg(DBG_ST.spi, ":::: Less than 16 bits per word....\n");
		curr_cfg->n_bytes = 2;
		curr_cfg->read = spi023_u16_reader;
		curr_cfg->write = spi023_u16_writer;
	} else {
		stm_dbg(DBG_ST.spi, ":::: Less than 32 bits per word....\n");
		curr_cfg->n_bytes = 4;
		curr_cfg->read = spi023_u32_reader;
		curr_cfg->write = spi023_u32_writer;
	}

	curr_cfg->delay = spi023_delay;
	/*Now Initialize all register settings reqd. for this chip */
	regs = (struct spi023_regs *)(curr_cfg->ctr_regs);
	regs->cr0 = 0;
	regs->cr1 = 0;
	regs->dmacr = 0;
	regs->cpsr = 0;

	if ((chip_info->com_mode == DMA_TRANSFER)
			&& ((drv_data->master_info)->enable_dma)) {
		if (configure_dma_request(curr_cfg, chip_info, drv_data))
			goto err_dma_request;
		curr_cfg->enable_dma = 1;
		SPI_WBITS(regs->dmacr,
				SPI_DMA_ENABLED, SPI_DMACR_MASK_RXDMAE, 0);
		SPI_WBITS(regs->dmacr,
				SPI_DMA_ENABLED, SPI_DMACR_MASK_TXDMAE, 1);
	} else {
		curr_cfg->enable_dma = 0;
		SPI_WBITS(regs->dmacr,
				SPI_DMA_DISABLED, SPI_DMACR_MASK_RXDMAE, 0);
		SPI_WBITS(regs->dmacr,
				SPI_DMA_DISABLED, SPI_DMACR_MASK_TXDMAE, 1);
	}


	regs->cpsr = chip_info->clk_freq.cpsdvsr;

	SPI_WBITS(regs->cr0, chip_info->data_size, SPI_CR0_MASK_DSS, 0);
	SPI_WBITS(regs->cr0,
			(chip_info->proto_params).moto.clk_pol,
			SPI_CR0_MASK_SPO, 6);
	SPI_WBITS(regs->cr0,
			(chip_info->proto_params).moto.clk_phase,
			SPI_CR0_MASK_SPH, 7);
	SPI_WBITS(regs->cr0, chip_info->clk_freq.scr, SPI_CR0_MASK_SCR, 8);
	SPI_WBITS(regs->cr1, chip_info->lbm, SPI_CR1_MASK_LBM, 0);
	SPI_WBITS(regs->cr1, SPI_DISABLED, SPI_CR1_MASK_SSE, 1);
	SPI_WBITS(regs->cr1, chip_info->hierarchy, SPI_CR1_MASK_MS, 2);
	SPI_WBITS(regs->cr1, chip_info->slave_tx_disable, SPI_CR1_MASK_SOD, 3);
	SPI_WBITS(regs->cr1, chip_info->endian_rx, SPI_CR1_MASK_RENDN, 4);
	SPI_WBITS(regs->cr1, chip_info->endian_tx, SPI_CR1_MASK_TENDN, 5);
	SPI_WBITS(regs->cr1,
			(chip_info->proto_params).micro.wait_state,
			SPI_CR1_MASK_MWAIT, 6);
	SPI_WBITS(regs->cr1, chip_info->rx_lev_trig, SPI_CR1_MASK_RXIFLSEL, 7);
	SPI_WBITS(regs->cr1, chip_info->tx_lev_trig, SPI_CR1_MASK_TXIFLSEL, 10);

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

static int stm_spi023_probe(struct amba_device *adev, struct amba_id *id)
{
	struct device *dev = &adev->dev;
	struct nmdk_spi_master_cntlr *platform_info;
	struct spi_master *master;
	struct driver_data *drv_data = NULL;	/*Data for this driver */
	struct resource *res;
	int irq, status = 0;


	platform_info = (struct nmdk_spi_master_cntlr *)(dev->platform_data);
	if (platform_info == NULL) {
		dev_err(&adev->dev, "probe - no platform data supplied\n");
		status = -ENODEV;
		goto err_no_pdata;
	}
	/* Allocate master with space for drv_data */
	master = spi_alloc_master(dev, sizeof(struct driver_data));
	if (master == NULL) {
		dev_err(&adev->dev, "probe - cannot alloc spi_master\n");
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
		goto err_no_clk;
	}

	/*Fetch the Resources, using platform data */
	res = &(adev->res);
	if (res == NULL) {
		dev_err(&adev->dev, "probe - MEM resources not defined\n");
		status = -ENODEV;
		goto err_no_iores;
	}
	/*Get Hold of Device Register Area... */
	drv_data->regs = ioremap(res->start, (res->end - res->start));
	if (drv_data->regs == NULL) {
		status = -ENODEV;
		goto err_no_iores;
	}
	irq = adev->irq[0];
	if (irq <= 0) {
		status = -ENODEV;
		goto err_no_irq;
	}
	stm_dbg(DBG_ST.spi, ":::: SPI Controller  %d\n", platform_info->id);
	drv_data->execute_cmd = spi023_controller_cmd;
	drv_data->execute_cmd(drv_data, LOAD_DEFAULT_CONFIG);

	master->setup = stm_spi023_setup;

	/*Required Info for an SPI controller */
	/*Bus No Which has been Assigned to this SPI controller on this board */
	master->bus_num = (u16) platform_info->id;
	master->num_chipselect = platform_info->num_chipselect;
	master->cleanup = (void *)stm_spi_cleanup;
	master->transfer = stm_spi_transfer;

	stm_dbg(DBG_ST.spi, ":::: BUSNO: %d\n", master->bus_num);
	status = spi_register_master(master);
	if (status != 0) {
		dev_err(&adev->dev, "probe - problem registering spi master\n");
		goto err_spi_register;
	}

	/* Initialize and start queue */
	status = init_queue(drv_data);
	if (status != 0) {
		dev_err(&adev->dev, "probe - problem initializing queue\n");
		goto err_init_queue;
	}
	status = start_queue(drv_data);
	if (status != 0) {
		dev_err(&adev->dev, "probe - problem starting queue\n");
		goto err_start_queue;
	}

	/*Initialize tasklet for DMA transfer */
	tasklet_init(&drv_data->spi_dma_tasklet, stm_spi_tasklet,
			(unsigned long)drv_data);

	/* Register with the SPI framework */
	platform_set_drvdata(adev, drv_data);
	dev_dbg(dev, "probe succeded\n");
	stm_dbg(DBG_ST.spi, " Bus Num = %d, IRQ Line = %d, Virtual Addr: %x\n",
			master->bus_num, irq, (u32) (drv_data->regs));

	status =
		stm_gpio_altfuncenable(drv_data->master_info->gpio_alt_func);
	if (status < 0) {
		dev_err(&drv_data->adev->dev,
				"probe - unable to set GPIO Altfunc, %d\n",
				drv_data->master_info->gpio_alt_func);
		status = -ENODEV;
		goto err_init_queue;
	}
	status =
		request_irq(drv_data->adev->irq[0], spi023_interrupt_handler,
				0, drv_data->master_info->device_name,
				drv_data);
	if (status < 0) {
		dev_err(&drv_data->adev->dev, "probe - cannot get IRQ (%d)\n",
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
err_no_irq:
	iounmap(drv_data->regs);
err_no_iores:
	clk_put(drv_data->clk);
err_no_clk:
	spi_master_put(master);
err_no_mem:
err_no_pdata:
	return status;
}

static int stm_spi023_remove(struct amba_device *adev)
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
		dev_err(&adev->dev, "queue remove failed (%d)\n", status);
		return status;
	}
	drv_data->execute_cmd(drv_data, LOAD_DEFAULT_CONFIG);

	/* Release map resources */
	iounmap(drv_data->regs);
	tasklet_disable(&drv_data->pump_transfers);
	tasklet_kill(&drv_data->spi_dma_tasklet);

	stm_gpio_altfuncdisable(platform_info->gpio_alt_func);
	free_irq(drv_data->adev->irq[0], drv_data);

	clk_put(drv_data->clk);

	/* Disconnect from the SPI framework */
	spi_unregister_master(drv_data->master);
	spi_master_put(drv_data->master);

	/* Prevent double remove */
	platform_set_drvdata(adev, NULL);
	dev_dbg(&adev->dev, "remove succeded\n");
	return 0;
}

#ifdef CONFIG_PM
/*
   static int suspend_devices(struct device *dev, void *pm_message)
   {
   pm_message_t *state = pm_message;

   if (dev->power.power_state.event != state->event) {
   dev_warn(dev, "pm state does not match request\n");
   return -1;
   }
   return 0;
   }
   */
int stm_spi023_suspend(struct amba_device *adev, pm_message_t state)
{
	struct driver_data *drv_data = platform_get_drvdata(adev);
	int status = 0;
	/*
	   if (device_for_each_child(&pdev->dev, &state,
		suspend_devices) != 0) {
	   dev_warn(&pdev->dev, "suspend aborted\n");
	   return -1;
	   }
	   */
	status = stop_queue(drv_data);
	if (status != 0) {
		dev_warn(&adev->dev, "suspend cannot stop queue\n");
		return status;
	}

	dev_dbg(&adev->dev, "suspended\n");
	return 0;
}

int stm_spi023_resume(struct amba_device *pdev)
{
	struct driver_data *drv_data = platform_get_drvdata(pdev);
	int status = 0;

	/* Start the queue running */
	status = start_queue(drv_data);
	if (status != 0)
		dev_err(&pdev->dev, "problem starting queue (%d)\n", status);
	else
		dev_dbg(&pdev->dev, "resumed\n");

	return status;
}

#else
#define stm_spi023_suspend NULL
#define stm_spi023_resume NULL
#endif /* CONFIG_PM */

static struct amba_id spi023_ids[] = {
	{
		.id = SPI_PER_ID,
		.mask = SPI_PER_MASK,
	},
	{0, 0},
};

static struct amba_driver spi023_driver = {

	.drv = {
		.name = "SPI",
	},
	.id_table = spi023_ids,
	.probe = stm_spi023_probe,
	.remove = stm_spi023_remove,
	.suspend = stm_spi023_suspend,
	.resume = stm_spi023_resume,
};

static int __init stm_spi_init(void)
{
	int retval = 0;
	printk(KERN_INFO "\nLoading SPI Controller Driver Version "
		SPI_DRIVER_VERSION "\n");
	retval = amba_driver_register(&spi023_driver);
	if (retval)
		printk(KERN_INFO "SPI Registration error\n");
	return retval;
}
#ifdef CONFIG_STM_SPI023_MODULE
module_init(stm_spi_init);
#else
subsys_initcall_sync(stm_spi_init);
#endif
static void __exit stm_spi_exit(void)
{
	printk(KERN_INFO "\nExiting SPI023 Controller Driver Version "
		SPI_DRIVER_VERSION "\n");
	amba_driver_unregister(&spi023_driver);
	return;
}

module_exit(stm_spi_exit);

MODULE_AUTHOR("Sachin Verma <sachin.verma@st.com>");
MODULE_DESCRIPTION("STM SPI023 Controller Driver");
MODULE_LICENSE("GPL");
