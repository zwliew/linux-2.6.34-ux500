/*
 * drivers/spi/stm_ssp.c
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <linux/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>

#include <mach/hardware.h>
#include <linux/gpio.h>
#include <mach/dma.h>
#include <asm/dma.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/spi/spi-stm.h>
#include <linux/spi/stm_ssp.h>
#include <mach/debug.h>

/***************************************************************************/
#define SPI_DRIVER_VERSION "2.3.0"

#define STM_SSP_NAME		"DRIVER SSP"


#define DRIVER_DEBUG	  0
#define DRIVER_DEBUG_PFX  STM_SSP_NAME	        /* msg hdr for this module */
#define DRIVER_DBG 	  KERN_ERR	        /* message level */
/***************************************************************************/

#define FALSE			(0)
#define TRUE			(1)
#define SPI_WR_BITS(reg, val, mask, sb) \
		((reg) =   (((reg) & ~(mask)) | (((val)<<(sb)) & (mask))))


/*
 *Calculate Frequency Parameters
 * */
int calculate_effective_freq(int freq, t_ssp_clock_params *clk_freq)
{
	u32 cpsdvsr = 2;
	u32 scr = 0;
	u32 max_tclk;
	u32 min_tclk;
	u32 eff_freq = 0;
	int freq_found = FALSE;

	max_tclk = (STM_SSP_CLOCK_FREQ / (MIN_CPSDVR * (1 + MIN_SCR)));
	min_tclk = (STM_SSP_CLOCK_FREQ / (MAX_CPSDVR * (1 + MAX_SCR)));

	if ((freq > max_tclk) || (freq < min_tclk)) {
		stm_dbg(DBG_ST.spi, "Out of Range Frequency");
		return -EINVAL;
	}

	while (cpsdvsr <= MAX_CPSDVR && !freq_found) {
		while (scr <= MAX_SCR && !freq_found) {
			eff_freq = STM_SSP_CLOCK_FREQ / (cpsdvsr * (1 + scr));

			if (eff_freq > freq)
				scr += 1;
			else {
				freq_found = TRUE;
				if (eff_freq != freq) {
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
		eff_freq = STM_SSP_CLOCK_FREQ / (cpsdvsr * (1 + scr));
		stm_dbg(DBG_ST.spi, "::SSP Effective Freq is %d\n", eff_freq);
		clk_freq->cpsdvsr = (u8) (cpsdvsr & 0xFF);
		clk_freq->scr = (u8) (scr & 0xFF);
		stm_dbg(DBG_ST.spi, ":::: SSP cpsdvsr = %d, scr = %d\n",
				clk_freq->cpsdvsr, clk_freq->scr);
	}
	return 0;
}


/**
 * ssp_controller_cmd - To execute controller specific commands
 * @drv_data: SSP driver private data structure
 * @cmd: Command which is to be executed on the controller
 *
 *
 */
static int ssp_controller_cmd(struct driver_data *drv_data, int cmd)
{
	int retval = 0;

	struct ssp_regs *ctr_regs = NULL;

	switch (cmd) {
	case DISABLE_CONTROLLER:
		{
			stm_dbg2(DBG_ST.ssp, ":::: DISABLE_CONTROLLER\n");
			writel((readl(SSP_CR1(drv_data->regs)) &
				(~SSP_CR1_MASK_SSE)), SSP_CR1(drv_data->regs));
			break;
		}
	case ENABLE_CONTROLLER:
		{
			stm_dbg2(DBG_ST.ssp, ":::: ENABLE_CONTROLLER\n");
			writel((readl(SSP_CR1(drv_data->regs)) |
				SSP_CR1_MASK_SSE), SSP_CR1(drv_data->regs));
			break;
		}
	case DISABLE_DMA:
		{
			stm_dbg2(DBG_ST.ssp, ":::: DISABLE_DMA\n");
			/*As DEFAULT_SSP_REG_DMACR has DMA disabled */
			writel(DEFAULT_SSP_REG_DMACR,
			       SSP_DMACR(drv_data->regs));
			break;
		}
	case ENABLE_DMA:
		{
			ctr_regs = drv_data->cur_chip->ctr_regs;
			stm_dbg2(DBG_ST.ssp, ":::: ENABLE_CONTROLLER\n");
			writel(ctr_regs->dmacr, SSP_DMACR(drv_data->regs));
			break;
		}
	case DISABLE_ALL_INTERRUPT:
		{
			stm_dbg2(DBG_ST.ssp, ":::: DISABLE_ALL_INTERRUPT\n");
			writel(DISABLE_ALL_SSP_INTERRUPTS,
			       SSP_IMSC(drv_data->regs));
			break;
		}
	case ENABLE_ALL_INTERRUPT:
		{
			stm_dbg2(DBG_ST.ssp, ":::: ENABLE_ALL_INTERRUPT\n");
			writel(ENABLE_ALL_SSP_INTERRUPTS,
			       SSP_IMSC(drv_data->regs));
			break;
		}
	case CLEAR_ALL_INTERRUPT:
		{
			writel(CLEAR_ALL_SSP_INTERRUPTS,
			       SSP_ICR(drv_data->regs));
			break;
		}
	case FLUSH_FIFO:
		{
			unsigned long limit = loops_per_jiffy << 1;
			stm_dbg2(DBG_ST.ssp, "::: DATA FIFO is flushed\n");
			do {
				while (readl(SSP_SR(drv_data->regs)) &
				       SSP_SR_MASK_RNE)
					readl(SSP_DR(drv_data->regs));
			} while ((readl(SSP_SR(drv_data->regs)) &
				  SSP_SR_MASK_BSY) && limit--);
			retval = limit;
			break;
		}
	case RESTORE_STATE:
		{
			ctr_regs = drv_data->cur_chip->ctr_regs;
			stm_dbg2(DBG_ST.ssp, ":::: RESTORE_STATE\n");
			writel(ctr_regs->cr0, SSP_CR0(drv_data->regs));
			writel(ctr_regs->cr1, SSP_CR1(drv_data->regs));
			writel(ctr_regs->dmacr,
			       SSP_DMACR(drv_data->regs));
			writel(ctr_regs->cpsr, SSP_CPSR(drv_data->regs));
			writel(DISABLE_ALL_SSP_INTERRUPTS,
			       SSP_IMSC(drv_data->regs));
			writel(CLEAR_ALL_SSP_INTERRUPTS,
			       SSP_ICR(drv_data->regs));
			break;
		}
	case LOAD_DEFAULT_CONFIG:
		{
			stm_dbg2(DBG_ST.ssp, ":::: LOAD_DEFAULT_CONFIG\n");
			writel(DEFAULT_SSP_REG_CR0, SSP_CR0(drv_data->regs));
			writel(DEFAULT_SSP_REG_CR1, SSP_CR1(drv_data->regs));
			writel(DEFAULT_SSP_REG_DMACR,
			       SSP_DMACR(drv_data->regs));
			writel(DEFAULT_SSP_REG_CPSR, SSP_CPSR(drv_data->regs));
			writel(DISABLE_ALL_SSP_INTERRUPTS,
			       SSP_IMSC(drv_data->regs));
			writel(CLEAR_ALL_SSP_INTERRUPTS,
			       SSP_ICR(drv_data->regs));
			break;
		}
	default:
		{
			stm_dbg2(DBG_ST.ssp, ":::: unknown command\n");
			retval = -1;
			break;
		}
	}
	return retval;
}

/**
 * ssp_null_writer - To Write Dummy Data in Data register
 * @drv_data: spi driver private data structure
 *
 * This function is set as a write function for transfer which have
 * Tx transfer buffer as NULL. It simply writes '0' in the Data
 * register
 */
static void ssp_null_writer(struct driver_data *drv_data)
{
	while ((readl(SSP_SR(drv_data->regs)) & SSP_SR_MASK_TNF)
	       && (drv_data->tx < drv_data->tx_end)) {
		/*Write '0' Data to Data Register */
		writel(0x0, SSP_DR(drv_data->regs));
		drv_data->tx += (drv_data->cur_chip->n_bytes);
	}
}

/**
 * ssp_null_reader - To read data from Data register and discard it
 * @drv_data: spi driver private data structure
 *
 * This function is set as a reader function for transfer which have
 * Rx Transfer buffer as null. Read Data is rejected
 *
 */
static void ssp_null_reader(struct driver_data *drv_data)
{
	while ((readl(SSP_SR(drv_data->regs)) & SSP_SR_MASK_RNE)
	       && (drv_data->rx < drv_data->rx_end)) {
		readl(SSP_DR(drv_data->regs));
		drv_data->rx += (drv_data->cur_chip->n_bytes);
	}
}
/**
 * ssp_u8_writer - Write FIFO data in Data register as a 8 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function writes data in Tx FIFO till it is not full
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary write ptr tx in drv_data which maintains
 * current write position in transfer buffer
 */
static void ssp_u8_writer(struct driver_data *drv_data)
{
	/*While Transmit Fifo is not Full
	 * (bit SSP_SR_MASK_TNF == 0 in status Reg)
	 * or our data to transmit is finished */
	while ((readl(SSP_SR(drv_data->regs)) & SSP_SR_MASK_TNF)
	       && (drv_data->tx < drv_data->tx_end)) {
		/*Write Data to Data Register */
		writel(*(u8 *) (drv_data->tx), SSP_DR(drv_data->regs));
		drv_data->tx += (drv_data->cur_chip->n_bytes);
	}
}

/**
 * ssp_u8_reader - Read FIFO data in Data register as a 8 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function reads data in Rx FIFO till it is not empty
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary Read ptr rx in drv_data which maintains
 * current read position in transfer buffer
 */
static void ssp_u8_reader(struct driver_data *drv_data)
{
	/*While Receive Fifo is not Empty
	 * (bit SSP_SR_MASK_RNE == 0 in status Reg)
	 * or We have received Data we wanted to receive */
	while ((readl(SSP_SR(drv_data->regs)) & SSP_SR_MASK_RNE)
	       && (drv_data->rx < drv_data->rx_end)) {
		*(u8 *) (drv_data->rx) = readl(SSP_DR(drv_data->regs));
		drv_data->rx += (drv_data->cur_chip->n_bytes);
	}
}

/**
 * ssp_u16_writer - Write FIFO data in Data register as a 16 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function writes data in Tx FIFO till it is not full
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary write ptr tx in drv_data which maintains
 * current write position in transfer buffer
 */
static void ssp_u16_writer(struct driver_data *drv_data)
{
	/*While Transmit Fifo is not Full
	 * (bit SSP_SR_MASK_TNF == 0 in status Reg)
	 * or our data to transmit is finished */
	while ((readl(SSP_SR(drv_data->regs)) & SSP_SR_MASK_TNF)
	       && (drv_data->tx < drv_data->tx_end)) {
		/*Write Data to Data Register */
		writel((u32) (*(u16 *) (drv_data->tx)), SSP_DR(drv_data->regs));
		drv_data->tx += (drv_data->cur_chip->n_bytes);
	}
}

/**
 * ssp_u16_reader - Read FIFO data in Data register as a 16 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function reads data in Rx FIFO till it is not empty
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary Read ptr rx in drv_data which maintains
 * current read position in transfer buffer
 */
static void ssp_u16_reader(struct driver_data *drv_data)
{
	/*While Receive Fifo is not Empty
	 * (bit SSP_SR_MASK_RNE == 0 in status Reg)
	 * or We have received Data we wanted to receive */
	while ((readl(SSP_SR(drv_data->regs)) & SSP_SR_MASK_RNE)
	       && (drv_data->rx < drv_data->rx_end)) {
		*(u16 *) (drv_data->rx) = (u16) readl(SSP_DR(drv_data->regs));
		drv_data->rx += (drv_data->cur_chip->n_bytes);
	}
}

/**
 * ssp_u32_writer - Write FIFO data in Data register as a 32 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function writes data in Tx FIFO till it is not full
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary write ptr tx in drv_data which maintains
 * current write position in transfer buffer
 */
static void ssp_u32_writer(struct driver_data *drv_data)
{
	/*While Transmit Fifo is not Full
	 * (bit SSP_SR_MASK_TNF == 0 in status Reg)
	 * or our data to transmit is finished */
	while ((readl(SSP_SR(drv_data->regs)) & SSP_SR_MASK_TNF)
	       && (drv_data->tx < drv_data->tx_end)) {
		/*Write Data to Data Register */
		writel(*(u32 *) (drv_data->tx), SSP_DR(drv_data->regs));
		drv_data->tx += (drv_data->cur_chip->n_bytes);
	}

}

/**
 * ssp_u32_reader - Read FIFO data in Data register as a 32 Bit Data
 * @drv_data: spi driver private data structure
 *
 * This function reads data in Rx FIFO till it is not empty
 * which is indicated by the status register or our transfer is complete.
 * It also updates the temporary Read ptr rx in drv_data which maintains
 * current read position in transfer buffer
 */
static void ssp_u32_reader(struct driver_data *drv_data)
{
	/*While Receive Fifo is not Empty
	 * (bit SSP_SR_MASK_RNE == 0 in status Reg)
	 * or We have received Data we wanted to receive */
	while ((readl(SSP_SR(drv_data->regs)) & SSP_SR_MASK_RNE)
	       && (drv_data->rx < drv_data->rx_end)) {
		*(u32 *) (drv_data->rx) = readl(SSP_DR(drv_data->regs));
		drv_data->rx += (drv_data->cur_chip->n_bytes);
	}
}
static void ssp_delay(struct driver_data *drv_data)
{
	while (readl(SSP_SR(drv_data->regs)) & SSP_SR_MASK_BSY)
		udelay(1);
}

/**
 * stm_ssp_interrupt_handler - Interrupt handler for spi controller
 *
 *
 * This function handles interrupts generated for an interrupt based transfer.
 * If a receive overrun (ROR) interrupt is there then we disable SSP, flag the
 * current message's state as ERROR_STATE and schedule the tasklet
 * pump_transfers which will do the postprocessing of the current message
 * by calling giveback(). Otherwise it reads data from Rx FIFO till there is
 * no more data, and writes data in Tx FIFO till it is not full. If we complete
 * the transfer we move to the next transfer and schedule the tasklet
 *
 */
static irqreturn_t stm_ssp_interrupt_handler(int irq, void *dev_id)
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
	irq_status = readl(SSP_MIS(drv_data->regs));

	if (irq_status) {
		if (irq_status & SSP_MIS_MASK_RORMIS) {	/*Overrun interrupt */
			/*Bail-out our Data has been corrupted */
			stm_dbg3(DBG_ST.ssp, ":::: Received ROR interrupt\n");
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
			writel(readl(SSP_IMSC(drv_data->regs)) &
			       (~SSP_IMSC_MASK_TXIM), SSP_IMSC(drv_data->regs));
		}
		if (drv_data->rx == drv_data->rx_end) {
			drv_data->execute_cmd(drv_data, DISABLE_ALL_INTERRUPT);
			drv_data->execute_cmd(drv_data, CLEAR_ALL_INTERRUPT);
			stm_dbg3(DBG_ST.ssp, "::Intrpt xfer Completed...\n");
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

int verify_ssp_controller_parameters(struct ssp_controller *chip_info)
{
	if ((chip_info->lbm != LOOPBACK_ENABLED)
			&& (chip_info->lbm != LOOPBACK_DISABLED)) {
		stm_dbg(DBG_ST.ssp, ": LoopbackMode is configured incorrect\n");
		return -1;
	}
	if ((chip_info->iface < SPI_INTERFACE_MOTOROLA_SPI)
			|| (chip_info->iface > SPI_INTERFACE_UNIDIRECTIONAL)) {
		stm_dbg(DBG_ST.ssp, ":: Interface is configured incorrectly\n");
		return -1;
	}
	if ((chip_info->hierarchy != SPI_MASTER)
			&& (chip_info->hierarchy != SPI_SLAVE)) {
		stm_dbg(DBG_ST.ssp, ":: hierarchy is configured incorrectly\n");
		return -1;
	}
	if ((chip_info->clk_freq.cpsdvsr < MIN_CPSDVR)
			|| (chip_info->clk_freq.cpsdvsr > MAX_CPSDVR)) {
		stm_dbg(DBG_ST.ssp, ":::: cpsdvsr is configured incorrectly\n");
		return -1;
	}
	if ((chip_info->endian_rx != SPI_FIFO_MSB)
			&& (chip_info->endian_rx != SPI_FIFO_LSB)) {
		stm_dbg(DBG_ST.ssp, ":: Rx FIFO endianess is incorrect\n");
		return -1;
	}
	if ((chip_info->endian_tx != SPI_FIFO_MSB)
			&& (chip_info->endian_tx != SPI_FIFO_LSB)) {
		stm_dbg(DBG_ST.ssp, ":::: Tx FIFO endianess is incorrect\n");
		return -1;
	}
	if ((chip_info->data_size < SSP_DATA_BITS_4)
			|| (chip_info->data_size > SSP_DATA_BITS_32)) {
		stm_dbg(DBG_ST.ssp, ":::: DATA Size is incorrectly\n");
		return -1;
	}
	if ((chip_info->com_mode != INTERRUPT_TRANSFER)
			&& (chip_info->com_mode != DMA_TRANSFER)
			&& (chip_info->com_mode != POLLING_TRANSFER)) {
		stm_dbg(DBG_ST.ssp, ":::: Communication mode is incorrect\n");
		return -1;
	}

#if 0
	if ((chip_info->dma_xfer_type != SPI_WITH_PERIPH)
			&& (chip_info->dma_xfer_type != SPI_WITH_MEM)) {
		stm_dbg(DBG_ST.ssp, ":::: DMA xfer type is incorrect\n");
		return -1;
	}
#endif

	if ((chip_info->rx_lev_trig < SSP_RX_1_OR_MORE_ELEM)
			|| (chip_info->rx_lev_trig >
				SSP_RX_32_OR_MORE_ELEM)) {
		stm_dbg(DBG_ST.ssp, ":::: Rx FIFO Trigger Level incorrect\n");
		return -1;
	}
	if ((chip_info->tx_lev_trig <
				SSP_TX_1_OR_MORE_EMPTY_LOC)
			|| (chip_info->tx_lev_trig >
				SSP_TX_32_OR_MORE_EMPTY_LOC)) {
		stm_dbg(DBG_ST.ssp, ":::: Tx FIFO Trigger Level incorrect\n");
		return -1;
	}
	if (chip_info->iface == SPI_INTERFACE_MOTOROLA_SPI) {
		if (((chip_info->proto_params).moto.clk_phase !=
					SPI_CLK_ZERO_CYCLE_DELAY)
				&& ((chip_info->proto_params).moto.clk_phase !=
					SPI_CLK_HALF_CYCLE_DELAY)) {
			stm_dbg(DBG_ST.ssp, ":::: Clock Phase is incorrect\n");
			return -1;
		}
		if (((chip_info->proto_params).moto.clk_pol !=
					SPI_CLK_POL_IDLE_LOW)
				&& ((chip_info->proto_params).moto.clk_pol !=
					SPI_CLK_POL_IDLE_HIGH)) {
			stm_dbg(DBG_ST.ssp, ":::: Clk Polarity is incorrect\n");
			return -1;
		}
	}
	if (chip_info->iface == SPI_INTERFACE_NATIONAL_MICROWIRE) {
		if (((chip_info->proto_params).micro.ctrl_len < MWLEN_BITS_4)
				|| ((chip_info->proto_params).micro.ctrl_len
					> MWLEN_BITS_32)) {
			stm_dbg(DBG_ST.ssp, ":::: CTRL LEN is incorrect\n");
			return -1;
		}
		if (((chip_info->proto_params).micro.wait_state !=
					MWIRE_WAIT_ZERO)
				&& ((chip_info->proto_params).micro.wait_state
					!= MWIRE_WAIT_ONE)) {
			stm_dbg(DBG_ST.ssp, ":::: Wait State is incorrect\n");
			return -1;
		}
		if (((chip_info->proto_params).micro.duplex !=
					MICROWIRE_CHANNEL_FULL_DUPLEX)
				&& ((chip_info->proto_params).micro.duplex !=
					MICROWIRE_CHANNEL_HALF_DUPLEX)) {
			stm_dbg(DBG_ST.ssp, ":::: DUPLEX is incorrect\n");
			return -1;
		}
	}
	if (chip_info->cs_control == NULL) {
		stm_dbg(DBG_ST.ssp, "::::Chip Select Func is NULL \n");
		chip_info->cs_control = null_cs_control;
	}
	return 0;
}

static struct ssp_controller *allocate_default_chip_cfg(struct spi_device *spi)
{
	struct ssp_controller *chip_info;

	/* spi_board_info.controller_data not is supplied */
	chip_info = kzalloc(sizeof(struct ssp_controller), GFP_KERNEL);
	if (!chip_info) {
		stm_error("setup - cannot allocate controller data");
		return NULL;
	}

	stm_dbg(DBG_ST.ssp, ":::: Allocated Memory for controller data\n");

	chip_info->lbm = LOOPBACK_DISABLED;
	chip_info->com_mode = POLLING_TRANSFER; /*DEFAULT MODE*/
	chip_info->iface = SPI_INTERFACE_MOTOROLA_SPI;
	chip_info->hierarchy = SPI_MASTER;
	chip_info->slave_tx_disable = DO_NOT_DRIVE_TX;
	chip_info->endian_tx = SPI_FIFO_LSB;
	chip_info->endian_rx = SPI_FIFO_LSB;
	chip_info->data_size = SSP_DATA_BITS_12;

	if (spi->max_speed_hz != 0) {
		chip_info->freq = spi->max_speed_hz;
		chip_info->clk_freq.cpsdvsr = 0;
		chip_info->clk_freq.scr = 0;
	} else {
		chip_info->freq = 0;
		chip_info->clk_freq.cpsdvsr = STM_SSP_DEFAULT_PRESCALE;
		chip_info->clk_freq.scr = STM_SSP_DEFAULT_CLKRATE;
	}
	chip_info->rx_lev_trig = SSP_RX_1_OR_MORE_ELEM;
	chip_info->tx_lev_trig = SSP_TX_1_OR_MORE_EMPTY_LOC;
	(chip_info->proto_params).moto.clk_phase = SPI_CLK_HALF_CYCLE_DELAY;
	(chip_info->proto_params).moto.clk_pol = SPI_CLK_POL_IDLE_LOW;
	chip_info->cs_control = null_cs_control;
	return chip_info;
}

static int configure_dma_request(struct chip_data *curr_cfg,
		struct ssp_controller *chip_info, struct driver_data *drv_data)
{
	int status = 0;
	curr_cfg->dma_info = kzalloc(sizeof(struct spi_dma_info), GFP_KERNEL);
	if (!curr_cfg->dma_info) {
		stm_dbg(DBG_ST.ssp, "memory alloc failed for chip_data\n");
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


/**
 * stm_ssp_setup - setup function registered to SPI master framework
 * @spi: spi device which is requesting setup
 *
 * This function is registered to the SPI framework for this SPI master
 * controller. If it is the first time when setup is called by this device
 * , this function will initialize the runtime state for this chip and save
 * the same in the device structure. Else it will update the runtime info
 * with the updated chip info.
 */
static int stm_ssp_setup(struct spi_device *spi)
{
	struct ssp_controller *chip_info;
	struct chip_data *curr_cfg;
	int status = 0;
	struct driver_data *drv_data = spi_master_get_devdata(spi->master);
	struct ssp_regs *regs;

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
		curr_cfg->ctr_regs =
				kzalloc(sizeof(struct ssp_regs), GFP_KERNEL);
		if (curr_cfg->ctr_regs == NULL) {
			stm_error("setup - cannot allocate mem for regs");
			goto err_first_setup;
		}
		stm_dbg(DBG_ST.ssp, ":::: chip Id = %d\n", curr_cfg->chip_id);

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

	if ((0 == chip_info->clk_freq.cpsdvsr) &&
			(0 == chip_info->clk_freq.scr)) {
		status = calculate_effective_freq(chip_info->freq,
				&(chip_info->clk_freq));
		if (status < 0)
			goto err_config_params;
	} else {
		if ((chip_info->clk_freq.cpsdvsr % 2) != 0)
			chip_info->clk_freq.cpsdvsr =
			    chip_info->clk_freq.cpsdvsr - 1;
	}
	status = verify_ssp_controller_parameters(chip_info);
	if (status) {
		stm_error("setup - controller data is incorrect");
		goto err_config_params;
	}
	/* Now set controller state based on controller data */
	curr_cfg->xfer_type = chip_info->com_mode;
	curr_cfg->cs_control = chip_info->cs_control;
	curr_cfg->delay = ssp_delay;
	curr_cfg->null_read = ssp_null_reader;
	curr_cfg->null_write = ssp_null_writer;

	if (chip_info->data_size <= 8) {
		stm_dbg(DBG_ST.ssp, ":::: Less than 8 bits per word....\n");
		curr_cfg->n_bytes = 1;
		curr_cfg->read = ssp_u8_reader;
		curr_cfg->write = ssp_u8_writer;
	} else if (chip_info->data_size <= 16) {
		stm_dbg(DBG_ST.ssp, ":::: Less than 16 bits per word....\n");
		curr_cfg->n_bytes = 2;
		curr_cfg->read = ssp_u16_reader;
		curr_cfg->write = ssp_u16_writer;
	} else {
		stm_dbg(DBG_ST.ssp, ":::: Less than 32 bits per word....\n");
		curr_cfg->n_bytes = 4;
		curr_cfg->read = ssp_u32_reader;
		curr_cfg->write = ssp_u32_writer;
	}

	/*Now Initialize all register settings reqd. for this chip */
	regs = (struct ssp_regs *)(curr_cfg->ctr_regs);
	regs->cr0 = 0;
	regs->cr1 = 0;
	regs->dmacr = 0;
	regs->cpsr = 0;

	if ((chip_info->com_mode == DMA_TRANSFER)
	    && ((drv_data->master_info)->enable_dma)) {
		if (configure_dma_request(curr_cfg, chip_info, drv_data))
			goto err_dma_request;
		curr_cfg->enable_dma = 1;
		SPI_WR_BITS(regs->dmacr,
			SSP_DMA_ENABLED, SSP_DMACR_MASK_RXDMAE, 0);
		SPI_WR_BITS(regs->dmacr,
			SSP_DMA_ENABLED, SSP_DMACR_MASK_TXDMAE, 1);
	} else {
		curr_cfg->enable_dma = 0;
		SPI_WR_BITS(regs->dmacr,
			SSP_DMA_DISABLED, SSP_DMACR_MASK_RXDMAE, 0);
		SPI_WR_BITS(regs->dmacr,
			SSP_DMA_DISABLED, SSP_DMACR_MASK_TXDMAE, 1);
	}

	regs->cpsr = chip_info->clk_freq.cpsdvsr;
	SPI_WR_BITS(regs->cr0, chip_info->data_size, SSP_CR0_MASK_DSS, 0);
	SPI_WR_BITS(regs->cr0,
			(chip_info->proto_params).micro.duplex,
			SSP_CR0_MASK_HALFDUP, 5);
	SPI_WR_BITS(regs->cr0,
			(chip_info->proto_params).moto.clk_pol,
			SSP_CR0_MASK_SPO, 6);
	SPI_WR_BITS(regs->cr0,
			(chip_info->proto_params).moto.clk_phase,
			SSP_CR0_MASK_SPH, 7);
	SPI_WR_BITS(regs->cr0, chip_info->clk_freq.scr, SSP_CR0_MASK_SCR, 8);
	SPI_WR_BITS(regs->cr0,
			(chip_info->proto_params).micro.ctrl_len,
			SSP_CR0_MASK_CSS, 16);
	SPI_WR_BITS(regs->cr0, chip_info->iface, SSP_CR0_MASK_FRF, 21);
	SPI_WR_BITS(regs->cr1, chip_info->lbm, SSP_CR1_MASK_LBM, 0);
	SPI_WR_BITS(regs->cr1, SSP_DISABLED, SSP_CR1_MASK_SSE, 1);
	SPI_WR_BITS(regs->cr1, chip_info->hierarchy, SSP_CR1_MASK_MS, 2);
	SPI_WR_BITS(regs->cr1,
			chip_info->slave_tx_disable, SSP_CR1_MASK_SOD, 3);
	SPI_WR_BITS(regs->cr1, chip_info->endian_rx, SSP_CR1_MASK_RENDN, 4);
	SPI_WR_BITS(regs->cr1, chip_info->endian_tx, SSP_CR1_MASK_TENDN, 5);
	SPI_WR_BITS(regs->cr1,
			(chip_info->proto_params).micro.wait_state,
			SSP_CR1_MASK_MWAIT, 6);
	SPI_WR_BITS(regs->cr1,
			chip_info->rx_lev_trig, SSP_CR1_MASK_RXIFLSEL, 7);
	SPI_WR_BITS(regs->cr1,
			chip_info->tx_lev_trig, SSP_CR1_MASK_TXIFLSEL, 10);

	/* Save controller_state */
	spi_set_ctldata(spi, curr_cfg);
	return status;

err_dma_request:
err_config_params:
err_first_setup:
	kfree(curr_cfg->ctr_regs);
	kfree(curr_cfg->dma_info);
	kfree(curr_cfg);
	return status;
}

static int stm_ssp_probe(struct amba_device *adev, struct amba_id *id)
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

	drv_data->clk = clk_get(&adev->dev, NULL);
	if (IS_ERR(drv_data->clk)) {
		stm_error("probe - cannot get clock\n");
		status = PTR_ERR(drv_data->clk);
		goto free_master;
	}

	drv_data->dma_ongoing = 0;

	/*Fetch the Resources, using platform data */
	res = &(adev->res);
	if (res == NULL) {
		stm_error("probe - MEM resources not defined\n");
		status = -ENODEV;
		goto free_clock;
	}
	/*Get Hold of Device Register Area... */
	drv_data->regs = ioremap(res->start, (res->end - res->start));
	if (drv_data->regs == NULL) {
		status = -ENODEV;
		goto free_clock;
	}
	irq = adev->irq[0];
	if (irq <= 0) {
		status = -ENODEV;
		goto err_no_iores;
	}

	stm_dbg(DBG_ST.ssp, ":::: SSP Controller  %d\n", platform_info->id);
	drv_data->execute_cmd = ssp_controller_cmd;
	drv_data->execute_cmd(drv_data, LOAD_DEFAULT_CONFIG);

	master->setup = stm_ssp_setup;

	/*Required Info for an SPI controller */
	/*Bus Number Which has been Assigned to this SPI cntlr on this board */
	master->bus_num = (u16) platform_info->id;
	master->num_chipselect = platform_info->num_chipselect;
	master->cleanup = (void *)stm_spi_cleanup;
	master->transfer = stm_spi_transfer;

	stm_dbg(DBG_ST.ssp, ":::: BUSNO: %d\n", master->bus_num);

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
	stm_dbg(DBG_ST.ssp, "probe succeded\n");
	stm_dbg(DBG_ST.ssp, " Bus No = %d, IRQ Line = %d, Virtual Addr: %x\n",
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
		request_irq(drv_data->adev->irq[0], stm_ssp_interrupt_handler,
			0, drv_data->master_info->device_name, drv_data);
	if (status < 0) {
		stm_error("probe - cannot get IRQ (%d)\n",
				status);
		goto err_irq;
	}

	status = spi_register_master(master);
	if (status != 0) {
		stm_error("probe - problem registering spi master\n");
		goto err_spi_register;
	}

	return 0;

err_spi_register:
	free_irq(drv_data->adev->irq[0], drv_data);
err_irq:
	stm_gpio_altfuncdisable(drv_data->master_info->gpio_alt_func);
err_init_queue:
err_start_queue:
	destroy_queue(drv_data);
err_no_iores:
	iounmap(drv_data->regs);
free_clock:
	clk_put(drv_data->clk);
free_master:
	spi_master_put(master);
err_no_mem:
err_no_pdata:
	return status;
}

static int stm_ssp_remove(struct amba_device *adev)
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

	clk_put(drv_data->clk);

	/* Disconnect from the SPI framework */
	spi_unregister_master(drv_data->master);

	/* Prevent double remove */
	platform_set_drvdata(adev, NULL);
	stm_dbg(DBG_ST.ssp, "remove succeded\n");
	return 0;
}

#ifdef CONFIG_PM
/**
 * stm_ssp_suspend - SSP suspend function registered with PM framework.
 * @dev: Reference to amba device structure of the device
 * @state: power mgmt state.
 *
 * This function is invoked when the system is going into sleep, called
 * by the power management framework of the linux kernel.
 *
 */
int stm_ssp_suspend(struct amba_device *adev, pm_message_t state)
{
	struct driver_data *drv_data = platform_get_drvdata(adev);
	int status = 0;

/*
	if (device_for_each_child(&pdev->dev, &state, suspend_devices) != 0) {
		dev_warn(&pdev->dev, "suspend aborted\n");
		return -1;
	}
*/
	status = stop_queue(drv_data);
	if (status != 0) {
		dev_warn(&adev->dev, "suspend cannot stop queue\n");
		return status;
	}

	stm_dbg(DBG_ST.ssp, "suspended\n");
	return 0;
}

/**
 * stm_ssp_resume - SSP Resume function registered with PM framework.
 * @dev: Reference to amba device structure of the device
 *
 * This function is invoked when the system is coming out of sleep, called
 * by the power management framework of the linux kernel.
 *
 */
int stm_ssp_resume(struct amba_device *pdev)
{
	struct driver_data *drv_data = platform_get_drvdata(pdev);
	int status = 0;

	/* Start the queue running */
	status = start_queue(drv_data);
	if (status != 0)
		stm_error("problem starting queue (%d)\n", status);
	else
		stm_dbg(DBG_ST.ssp, "resumed\n");

	return status;
}

#else
#define stm_ssp_suspend NULL
#define stm_ssp_resume NULL
#endif /* CONFIG_PM */

static struct amba_id ssp_ids[] = {
	{
	 .id = SSP_PER_ID,
	 .mask = SSP_PER_MASK,
	 },
	{0, 0},
};

static struct amba_driver spi_driver = {

	.drv = {
		.owner = THIS_MODULE,
		.name = "SSP",
	},
	.id_table = ssp_ids,
	.probe = stm_ssp_probe,
	.remove = stm_ssp_remove,
	.suspend = stm_ssp_suspend,
	.resume = stm_ssp_resume,
};

static int __init stm_ssp_init(void)
{
	int retval = 0;
	printk(KERN_INFO "\nLoading SSP Controller Driver Version "
		SPI_DRIVER_VERSION "\n");
	retval = amba_driver_register(&spi_driver);
	if (retval)
		printk(KERN_INFO "SSP Registration error\n");
	return retval;
}
#ifdef CONFIG_STM_SSP_MODULE
module_init(stm_ssp_init);
#else
//subsys_initcall_sync(stm_ssp_init);
subsys_initcall(stm_ssp_init);
#endif
static void __exit stm_ssp_exit(void)
{
	printk(KERN_INFO "\nExiting SSP Controller Driver Version "
		SPI_DRIVER_VERSION "\n");
	amba_driver_unregister(&spi_driver);
	return;
}

module_exit(stm_ssp_exit);

MODULE_AUTHOR("Sachin Verma <sachin.verma@st.com>");
MODULE_DESCRIPTION("STM SPI Controller Driver");
MODULE_LICENSE("GPL");
