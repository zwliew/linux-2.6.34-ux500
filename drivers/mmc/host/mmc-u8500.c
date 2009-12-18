/*
 * Overview:
 *  	 SD/EMMC driver for u8500 platform
 *
 * Copyright (C) 2009 ST Ericsson.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*-------------------------------------------------------------------------
 * SDI controller configuration
 * Kernel entry point for sdmmc/emmc chip
 *-------------------------------------------------------------------------
 * <VERSION>v1.0.0
 *-------------------------------------------------------------------------
 */

/** @file  mmc-u8500.c
  * @brief This file contains the sdmmc/emmc chip initialization with default as DMA
  * mode.Polling/Interrupt mode support could be enabled in the kernel menuconfig.
  * Read and write function definitions to configure the read and write functionality
  * was implemented.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/mmc/host.h>
#ifdef CONFIG_U8500_SDIO
#include <linux/mmc/sdio.h>
#endif
#include <linux/amba/bus.h>
#include <linux/clk.h>
#include <linux/autoconf.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/completion.h>


#include <asm/cacheflush.h>
#include <asm/div64.h>
#include <asm/io.h>
#include <mach/dma.h>
#include <asm/scatterlist.h>
#include <asm/sizes.h>
#include <mach/hardware.h>
#include <asm/mach/mmc.h>
#include <mach/mmc.h>
#include <mach/debug.h>
#include <mach/dma_40-8500.h>
#include "mmc-u8500.h"

/*
 * Global variables
 */

#define DRIVER_NAME         "DRIVER MMC"

#define CONFIG_STM_MMC_DEBUG
#define DRIVER_DEBUG      CONFIG_STM_MMC_DEBUG
#define DRIVER_DEBUG_PFX  DRIVER_NAME
#define DRIVER_DBG        KERN_ERR
extern struct driver_debug_st  DBG_ST;
#define DMA_SCATERGATHER

#if defined CONFIG_U8500_MMC_POLL
const int mmc_mode = MCI_POLLINGMODE;
#elif defined CONFIG_U8500_MMC_INTR
const int mmc_mode = MCI_INTERRUPTMODE;
#elif defined CONFIG_U8500_MMC_DMA
const int mmc_mode = MCI_DMAMODE;
#endif

#if defined CONFIG_U8500_SDIO_POLL
int sdio_mode = MCI_POLLINGMODE;
#elif defined CONFIG_U8500_SDIO_INTR
int sdio_mode = MCI_INTERRUPTMODE;
#elif defined CONFIG_U8500_SDIO_DMA
int sdio_mode = MCI_DMAMODE;
#endif

#if !defined CONFIG_U8500_MMC_INTR
static unsigned int fmax = CLK_MAX;
#else
static unsigned int fmax = CLK_MAX / 8;
#endif

#if defined CONFIG_U8500_MMC_DMA
/*
 *  A macro that holds the spin_lock
 */
#define MMC_LOCK(lock, flag)	spin_lock(&lock)
/*
 *  A macro that releases the spin_lock
 */
#define MMC_UNLOCK(lock, flag)	spin_unlock(&lock)
#else
/*
 *  A macro that holds the spin_lock with irqsave
 */
#define MMC_LOCK(lock, flag)	spin_lock_irqsave(&lock, flag)
/*
 *  A macro that releases the spin_lock with irqrestore
 */
#define MMC_UNLOCK(lock, flag)	spin_unlock_irqrestore(&lock, flag)
#endif


static void u8500_mmci_start_command(struct u8500_mmci_host *host,
						struct mmc_command *cmd);

static void u8500_mmci_data_irq(struct u8500_mmci_host *host,
						u32 hoststatus);

static void u8500_mmci_cmd_irq(struct u8500_mmci_host *host,
						u32 hoststatus);

/**
 *   u8500_mmci_init_sg() - initializing the host scatterlist
 *   @host:	pointer to the u8500_mmci_host structure
 *   @data:	pointer to the mmc_data structure
 *
 *   this function assigns the scattelist of the mmc_data structure
 *   to the host scatterlist.
 */
static inline void
u8500_mmci_init_sg(struct u8500_mmci_host *host, struct mmc_data *data)
{
	/*
	 * Ideally, we want the higher levels to pass us a scatter list
	 */
	host->sg_len = data->sg_len;
	host->sg_ptr = data->sg;
	host->sg_off = 0;
}

/**
 *   u8500_mmc_clearirqsrc() - Clearing the interrupts of the host
 *   @base:	pointer to the base address of the host
 *   @irqsrc:	irqsrc number.
 *
 *   this function clears the corresponding interrupt of the source of the
 *   SD controller by writing to interrupt clear register
 */
static inline void u8500_mmc_clearirqsrc(void *base, u32 irqsrc)
{
	u32 nmdk_reg;

	nmdk_reg = readl(base + MMCICLEAR);
	nmdk_reg |= irqsrc;
	writel(nmdk_reg, (base + MMCICLEAR));
}

/**
 *   u8500_mmc_disableirqsrc() - Disabling the interrupts of the host
 *   @base:	pointer to the base address of the host
 *   @irqsrc: irqsrc number.
 *
 *   this function disables the corresponding interrupt source
 */
static inline void u8500_mmc_disableirqsrc(void *base, u32 irqsrc)
{
	u32 nmdk_reg;
	nmdk_reg = readl(base + MMCIMASK0);
	nmdk_reg &= ~irqsrc;
	writel(nmdk_reg, (base + MMCIMASK0));
}

/**
 *   u8500_mmc_enableirqsrc() - Enabeling the interrupts of the host
 *   @base:	pointer to the base address of the host
 *   @irqsrc:	irqsrc number
 *
 *   this function enables the corresponding interrupt source
 */
static inline void u8500_mmc_enableirqsrc(void *base, u32 irqsrc)
{
	u32 nmdk_reg;
	nmdk_reg = readl(base + MMCIMASK0);
	nmdk_reg |= irqsrc;
	writel(nmdk_reg, (base + MMCIMASK0));
}

/**
 *   u8500_mmci_request_end() - Informs the stack regarding the completion of the request
 *   @host:	pointer to the u8500_mmci_host structure
 *   @mrq:	pointer to the mmc_request structure
 *
 *   this function will be called after servicing the request sent from the stack.And this
 *   function will inform the stack that the request is done
 */
static void
u8500_mmci_request_end(struct u8500_mmci_host *host,
			 struct mmc_request *mrq)
{
#ifndef CONFIG_U8500_MMC_DMA
	unsigned long flag_lock = 0;
#endif
	MMC_LOCK(host->lock, flag_lock);
	host->mrq = NULL;
	host->cmd = NULL;
	if (mrq->data)
		mrq->data->bytes_xfered = host->data_xfered;
	/*
	 * Need to drop the host lock here; mmc_request_done may call
	 * back into the driver...
	 */
	MMC_UNLOCK(host->lock, flag_lock);
	mmc_request_done(host->mmc, mrq);
}

/**
 *   u8500_mmci_stop_data() - clears the data control registers
 *   @host:	pointer to the u8500_mmci_host structure
 *
 *   this function clears the data control register after the end of
 *   data transfer or when an error occured in the data transfer
 */
static void u8500_mmci_stop_data(struct u8500_mmci_host *host)
{
#ifndef CONFIG_U8500_MMC_DMA
	unsigned long flag_lock = 0;
#endif
	MMC_LOCK(host->lock, flag_lock);
	writel(0, host->base + MMCIDATACTRL);
	writel(0, host->base + MMCIMASK1);
	host->data = NULL;
	MMC_UNLOCK(host->lock, flag_lock);
}

/**
 *   u8500_mmc_send_cmd() - configures the cmd and arg control registers
 *   @base:	pointer to the base address of the host
 *   @cmd:	cmd number to be sent
 *   @arg:	arg value need to be paased along with command
 *   @flags:	flags need to be set for the cmd
 *
 *   this function will send the cmd to the card by enabling the
 *   command path state machine and writing the cmd number and arg to the
 *   corresponding command and arguement registers
 */
static void u8500_mmc_send_cmd(void *base, u32 cmd, u32 arg, u32 flags, int devicemode)
{
	u32 c, irqmask;
	/* stm_dbg2(DBG_ST.mmc,"Sending CMD%d, arg=%08x\n", cmd, arg);*/
	/* Clear any previous command */
	if ((readl(base + MMCICOMMAND) & MCI_CPSM_ENABLE)) {
		writel(0, (base + MMCICOMMAND));
		udelay(1);
	}
	c = cmd | MCI_CPSM_ENABLE;
	irqmask = MCI_CMDCRCFAIL | MCI_CMDTIMEOUT;
	if (flags & MMC_RSP_PRESENT) {
		if (flags & MMC_RSP_136)
			c |= MCI_CPSM_LONGRSP;
		c |= MCI_CPSM_RESPONSE;
		irqmask |= MCI_CMDRESPEND;
	} else {
		irqmask |= MCI_CMDSENT;
	}
	writel(arg, (base + MMCIARGUMENT));
	writel(c, (base + MMCICOMMAND));
	//udelay(10);
	if (devicemode != MCI_POLLINGMODE)
		u8500_mmc_enableirqsrc(base, irqmask);
#if 0//def CONFIG_U8500_SDIO
        if((cmd==SD_IO_RW_EXTENDED) || (cmd==SD_IO_RW_DIRECT))
                u8500_mmc_enableirqsrc(base, MCI_SDIOIT);
#endif
}

/**
 *   process_command_end() - read the response for the corresponding cmd sent
 *   @host:	pointer to the u8500_mmci_host
 *   @hoststatus:	status register value
 *
 *   this function will read the response of the corresponding command by reading
 *   the response registers
 */
static void process_command_end(struct u8500_mmci_host *host, u32 hoststatus)
{
	struct mmc_command *cmd;
	void __iomem *base;

	base = host->base;
	cmd = host->cmd;

	u8500_mmc_disableirqsrc(base, MCI_CMD_IRQ);
	u8500_mmc_clearirqsrc(base, MCI_CMD_IRQ);

	cmd->resp[0] = readl(base + MMCIRESPONSE0);
	cmd->resp[1] = readl(base + MMCIRESPONSE1);
	cmd->resp[2] = readl(base + MMCIRESPONSE2);
	cmd->resp[3] = readl(base + MMCIRESPONSE3);

	if ((hoststatus & MCI_CMDTIMEOUT)) {
		/*printk(KERN_ERR "Command Timeout: %d\n", cmd->opcode);*/
		cmd->error = -ETIMEDOUT;
	} else if ((hoststatus & MCI_CMDCRCFAIL) && (cmd->flags & MMC_RSP_CRC)) {
		printk(KERN_ERR "Command CRC Fail\n");
		cmd->error = -EILSEQ;
	} else if ((cmd->flags & MMC_RSP_OPCODE) &&
		readl(base + MMCIRESPCMD) != cmd->opcode) {
		stm_error("Command failed opcode check\n");
		cmd->error = -EILSEQ;
	} else {
		cmd->error = MMC_ERR_NONE;
	}
	/* stm_dbg2(DBG_ST.mmc,"CMD%d error=%d response:0x%08x 0x%08x 0x%08x 0x%08x\n",
		cmd->opcode, cmd->error, cmd->resp[0], cmd->resp[1], cmd->resp[2],
		cmd->resp[3]);
	*/
}

/**
 *   wait_for_command_end() - waiting for the command completion
 *   @host:	pointer to the u8500_mmci_host
 *
 *   this function will wait for whether the command completion has happened or
 *   any error generated by reading the status register
 */
static void wait_for_command_end(struct u8500_mmci_host *host)
{
	u32 hoststatus, statusmask;
	struct mmc_command *cmd;
	void __iomem *base;

	base = host->base;
	cmd = host->cmd;

	statusmask = MCI_CMDTIMEOUT | MCI_CMDCRCFAIL;
	if ((cmd->flags & MMC_RSP_PRESENT)) {
		statusmask |= MCI_CMDRESPEND;
	} else {
		statusmask |= MCI_CMDSENT;
	}
	do {
		hoststatus = readl(base + MMCISTATUS) & statusmask;
	} while (!hoststatus);

	u8500_mmci_cmd_irq(host, hoststatus);
}

/**
 *   check_for_data_err() - checks the status register value for the error generated
 *   @hoststatus:	value of the host status register
 *
 *   this function will read the status register and will return the error if any
 *   generated during the operation
 */
static int check_for_data_err(u32 hoststatus)
{
	int error = MMC_ERR_NONE;
	if (hoststatus &
		(MCI_DATACRCFAIL | MCI_DATATIMEOUT | MCI_TXUNDERRUN | MCI_RXOVERRUN
			| MCI_STBITERR)) {
		if ((hoststatus & MCI_DATATIMEOUT)) {
			stm_error("MMC: data timeout \n");
			error = -ETIMEDOUT;
		} else if (hoststatus & MCI_DATACRCFAIL) {
			error = -EILSEQ;
			stm_error("MMC: data CRC fail \n");
		} else if (hoststatus & MCI_RXOVERRUN) {
			error = -EIO;
			stm_error("MMC: Rx overrun \n");
		} else if (hoststatus & MCI_TXUNDERRUN) {
			stm_error("MMC: Tx underrun \n");
			error = -EIO;
		} else if (hoststatus & MCI_STBITERR) {
			stm_error("MMC: stbiterr \n");
			error = -EILSEQ;
		}
	}
	return error;
}
/**
 *   u8500_mmc_dma_transfer_done() - called after the DMA tarnsfer completion
 *   @host:	pointer to the u8500_mmci_host
 *
 *   this  function will update the stack with the data read and about the request
 *   completion
 */
static void u8500_mmc_dma_transfer_done(struct u8500_mmci_host *host)
{
	struct mmc_data *data;
	struct mmc_host *mmc;
	void __iomem *base;
	base = host->base;
	data = host->data;
	mmc = host->mmc;
	host->dma_done = NULL;
	if (data == NULL) {
		stm_error("what? data is null, go back  \n");
		u8500_mmci_request_end(host, host->mrq);
		return;
	}
#ifndef DMA_SCATERGATHER
	spin_lock(&host->lock);
	if (!data->error) {
			if ((data->flags & MMC_DATA_READ)) {
				u32 *buffer_local, *buffer;
				u8500_mmci_init_sg(host, data);
				buffer_local = host->dma_buffer;
				while (host->sg_len--) {
					buffer = (u32 *) (page_address(sg_page(host->sg_ptr)) + host->sg_ptr->offset);
					memcpy(buffer, buffer_local, host->sg_ptr->length);
					buffer_local += host->sg_ptr->length / 4;
					host->sg_ptr++;
					host->sg_off = 0;
				}
			}
	}
	spin_unlock(&host->lock);
#endif
	if (data->error) {
		if (data->flags & MMC_DATA_READ)
			stm_disable_dma(host->dmach_mmc2mem);
		else
			stm_disable_dma(host->dmach_mem2mmc);
#ifdef DMA_SCATERGATHER
		dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len,
		 ((data->flags & MMC_DATA_READ)	?
		 DMA_FROM_DEVICE : DMA_TO_DEVICE));
#endif
		u8500_mmc_clearirqsrc(base, MMCCLRSTATICFLAGS);
		u8500_mmci_stop_data(host);
		goto out;
	}
	spin_lock(&host->lock);
	host->data_xfered += host->size;
	spin_unlock(&host->lock);
	u8500_mmc_clearirqsrc(base, MMCCLRSTATICFLAGS);
	u8500_mmci_stop_data(host);
#ifdef DMA_SCATERGATHER
	dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len,
		 ((data->flags & MMC_DATA_READ)	?
		 DMA_FROM_DEVICE : DMA_TO_DEVICE));
#endif
	if (!data->stop)
		u8500_mmci_request_end(host, data->mrq);
	else
		u8500_mmci_start_command(host, data->stop);
out:
	if (data->error != MMC_ERR_NONE) {
		if (!data->stop)
			u8500_mmci_request_end(host, data->mrq);
		else
			u8500_mmci_start_command(host, data->stop);
	}
}
/**
 *   u8500_mmc_dmaclbk() - callback handler for DMA
 *   @data:	pointer to the host
 *   @event:	event value for checking the completion status
 *
 *   this function is the callback which will be called upon the completion of any DMA
 *   request regarding read/write.
 */
void u8500_mmc_dmaclbk(void *data, enum dma_event event)
{
	struct u8500_mmci_host *host = data;
	if (host->dma_done == NULL) {
		/* Removing this print */
		/* stm_info("MMC: host->dma_done is found tobe null ! \n"); */
		return;
	}
	u8500_mmc_dma_transfer_done(host);
	return;
}
/**
 *   u8500_mmc_set_dma() - initiates the DMA transfer
 *   @host:	pointer to the u8500_mmci_host register
 *   @mmc_id:	mmc_id for selecting sdmmc or emmc
 *
 *   configures the DMA channel and selects the burst size depending on the
 *   data size and enables the dma transfer for the corresponding device
 *   either sdmmc or emmc
 */
static void u8500_mmc_set_dma(struct u8500_mmci_host *host)
{
	struct mmc_data *data;
	struct stm_dma_pipe_info pipe_info1, pipe_info2;
#ifndef DMA_SCATERGATHER
	u32 *buffer, *ptr, *buffer1;
	unsigned long flag_lock = 0;

#endif
	data = host->data;
	memset(&pipe_info1, 0, sizeof(pipe_info1));
	memset(&pipe_info2, 0, sizeof(pipe_info2));

#ifdef DMA_SCATERGATHER
/*#warning " scatter gather is compiled "*/
	dma_map_sg(mmc_dev(host->mmc), data->sg,
			data->sg_len, ((data->flags & MMC_DATA_READ)
			? DMA_FROM_DEVICE : DMA_TO_DEVICE));
#else
	spin_lock_irqsave(&host->lock, flag_lock);
	if (data->flags & MMC_DATA_READ) {
		buffer = (u32 *)host->dma;
	} else {
		ptr = (u32 *) host->dma_buffer;
		buffer1 = (u32 *) (page_address(sg_page(host->sg_ptr)) +
				host->sg_ptr->offset) + host->sg_off;
		memcpy(ptr, buffer1, host->size);
		buffer = (u32 *)host->dma;
	}
	spin_unlock_irqrestore(&host->lock, flag_lock);
#endif
	pipe_info2.channel_type = (STANDARD_CHANNEL|CHANNEL_IN_LOGICAL_MODE|
					LCHAN_SRC_LOG_DEST_LOG|NO_TIM_FOR_LINK|
						NON_SECURE_CHANNEL|LOW_PRIORITY_CHANNEL);
	pipe_info2.dir = MEM_TO_PERIPH;
	pipe_info2.src_dev_type = DMA_DEV_SRC_MEMORY;
	pipe_info2.dst_dev_type = host->dma_fifo_dev_type_tx;
	pipe_info2.src_info.endianess = DMA_LITTLE_ENDIAN;
	pipe_info2.src_info.data_width = DMA_WORD_WIDTH;
	pipe_info2.src_info.buffer_type = SINGLE_BUFFERED;
	pipe_info2.dst_info.endianess = DMA_LITTLE_ENDIAN;
	pipe_info2.dst_info.data_width = DMA_WORD_WIDTH;
	pipe_info2.dst_info.buffer_type = SINGLE_BUFFERED;
#if 0 //def CONFIG_U8500_SDIO
        if(host->cmd->opcode == SD_IO_RW_EXTENDED) {
                pipe_info2.src_info.data_width = DMA_BYTE_WIDTH;
                pipe_info2.dst_info.data_width = DMA_BYTE_WIDTH;
		printk("\n 1--- DMA byte width");
        }
#endif
	pipe_info1.channel_type = (STANDARD_CHANNEL|CHANNEL_IN_LOGICAL_MODE|
					LCHAN_SRC_LOG_DEST_LOG|NO_TIM_FOR_LINK|
						NON_SECURE_CHANNEL|LOW_PRIORITY_CHANNEL);
	pipe_info1.dir = PERIPH_TO_MEM;
	pipe_info1.src_dev_type =  host->dma_fifo_dev_type_rx;
	pipe_info1.dst_dev_type = DMA_DEV_DEST_MEMORY;
	pipe_info1.src_info.endianess = DMA_LITTLE_ENDIAN;
	pipe_info1.src_info.data_width = DMA_WORD_WIDTH;
	pipe_info1.src_info.buffer_type = SINGLE_BUFFERED;
	pipe_info1.dst_info.endianess = DMA_LITTLE_ENDIAN;
	pipe_info1.dst_info.data_width = DMA_WORD_WIDTH;
	pipe_info1.dst_info.buffer_type = SINGLE_BUFFERED;
#if 0 //def CONFIG_U8500_SDIO
	if(host->cmd->opcode == SD_IO_RW_EXTENDED) {
		pipe_info1.src_info.data_width = DMA_BYTE_WIDTH;
		pipe_info1.dst_info.data_width = DMA_BYTE_WIDTH;
		printk("\n 2--- DMA byte width");
	}
#endif
#ifdef CONFIG_U8500_SDIO
	if((32>=host->size) || ((host->cmd->opcode != SD_IO_RW_EXTENDED) && (8 >= host->size))){
#else
	if( 8 >= host->size ){
#endif
		/* set smaller burst size*/
		pipe_info1.src_info.burst_size = DMA_BURST_SIZE_1;
		pipe_info2.src_info.burst_size = DMA_BURST_SIZE_1;
		pipe_info1.dst_info.burst_size = DMA_BURST_SIZE_1;
		pipe_info2.dst_info.burst_size = DMA_BURST_SIZE_1;
		data->flags & MMC_DATA_READ ? stm_configure_dma_channel(host->dmach_mmc2mem, &pipe_info1) :
				stm_configure_dma_channel(host->dmach_mem2mmc, &pipe_info2);
	} else {
		/* set bigger burst size*/
		pipe_info1.src_info.burst_size = DMA_BURST_SIZE_4;
		pipe_info2.src_info.burst_size = DMA_BURST_SIZE_4;
		pipe_info1.dst_info.burst_size = DMA_BURST_SIZE_4;
		pipe_info2.dst_info.burst_size = DMA_BURST_SIZE_4;
		data->flags & MMC_DATA_READ ? stm_configure_dma_channel(host->dmach_mmc2mem, &pipe_info1) :
				stm_configure_dma_channel(host->dmach_mem2mmc, &pipe_info2);
	}
	if (data->flags & MMC_DATA_READ) {
			stm_set_dma_count(host->dmach_mmc2mem, host->size);
			#ifdef DMA_SCATERGATHER
				/*set_dma_sg(host->dmach_mmc2mem, data->sg, data->sg_len);*/
				stm_set_dma_addr(host->dmach_mmc2mem,
						(void *)host->dma_fifo_addr,
							(unsigned int *) sg_phys(data->sg));
			#else
			stm_set_dma_addr(host->dmach_mmc2mem,
					(void *)host->dma_fifo_addr,
						/*(physical_address *)*/buffer);
			#endif
			stm_enable_dma(host->dmach_mmc2mem);
	} else {
			stm_set_dma_count(host->dmach_mem2mmc, host->size);
			#ifdef DMA_SCATERGATHER
				/*set_dma_sg(host->dmach_mem2mmc, data->sg, data->sg_len);*/
				stm_set_dma_addr(host->dmach_mem2mmc, (unsigned int *) sg_phys(data->sg),
						(void *)host->dma_fifo_addr);
			#else
			stm_set_dma_addr(host->dmach_mem2mmc,/*(physical_address *)*/ buffer,
						(void *)host->dma_fifo_addr);
			#endif
			stm_enable_dma(host->dmach_mem2mmc);
	}
}
/**
 *   u8500_mmci_read() - data read function in polling mode
 *   @host:	pointer to the u8500_mmci_host structure
 *   @buffer:	pointer to the host buffer
 *   @remain:	size of the remaining data to be transferred
 *   @hoststatus:  value of the host status register
 *
 *   the function will read the data from the fifo and copies to the host buffer
 *   in case of polling mode and returns the data transfered
 */
static int u8500_mmci_read(struct u8500_mmci_host *host, u32 * buffer,
			     unsigned int remain, u32 hoststatus)
{
	void __iomem *base;
	int count, max_count;
	unsigned int data_xfered = 0;
	base = host->base;
	u8* char_buf;
	u16* word_buf;
	int temp;
	u32 flag = MCI_DATA_IRQ;
#ifdef CONFIG_U8500_SDIO
	if(host->cmd && host->cmd->opcode == SD_IO_RW_EXTENDED)
		flag = MCI_DATA_ERR; /*required for SDIO CMD53*/
#endif
	while ((remain > 0) && !(hoststatus & flag)) {
		if ((hoststatus & MCI_RXFIFOHALFFULL) && (remain >= 32))
			max_count = MCI_FIFOHALFSIZE;
		else if ((hoststatus & MCI_RXDATAAVLBL) && (remain < 32))
			max_count = 1;
		else
			max_count = 0;
		for (count = 0; count < max_count; count++) {
			//*buffer = readl(base + MMCIFIFO);
			//buffer++;
			if(remain==2) { /*half word aligned, read 16bit half words*/
				word_buf = (u16*)buffer;
                                *word_buf = readw((u16*)(base + MMCIFIFO));
                                word_buf++;
				buffer = (u32*)word_buf;
                                data_xfered += 2;
                                remain =0;
			}else if(remain<4) /*not word aligned, read byte by byte*/
				{
				char_buf = (u8*)buffer;
					for(temp=0; temp<remain; temp++) {
					*char_buf = readb((u8*)(base + MMCIFIFO));
					char_buf++;
				}
				data_xfered += remain;
				buffer = (u32*)char_buf;
				remain = 0;
				}
			else {
			*buffer = readl(base + MMCIFIFO);
			buffer++;
			data_xfered += 4;
			remain -= 4;
		}
		}
		hoststatus = readl(base + MMCISTATUS);
	}

	if (remain) {
		/* stm_dbg2(DBG_ST.mmc,"While READ, Remain is %d, hoststatus is %x, data_xfered is %d\n",
			remain, hoststatus, data_xfered);
		*/
	}
	return data_xfered;
}
/**
 *   u8500_mmci_write() - data write function in polling mode
 *   @host:  pointer to the u8500_mmci_host structure
 *   @buffer:  pointer to the host buffer
 *   @remain:  size of the remaining data to be transfered
 *   @hoststatus: value of the host status register
 *
 *   the function will write the data from the host buffer to the fifo
 *   in case of polling mode and returns the data transfered
 */
static int u8500_mmci_write(struct u8500_mmci_host *host, u32 * buffer,
			      unsigned int remain, u32 hoststatus)
{
	void __iomem *base;
	int count, max_count, temp;
	unsigned int data_xfered = 0;
	u16* buf16;
	u8* buf8;

	base = host->base;
	u32 flag = MCI_DATA_IRQ;
#ifdef CONFIG_U8500_SDIO
	if(host->cmd && host->cmd->opcode == SD_IO_RW_EXTENDED)
		flag = MCI_DATA_ERR; /*required for SDIO CMD53*/
#endif

	while ((remain > 0) && !(hoststatus & flag)) {
		if ((hoststatus & MCI_TXFIFOEMPTY) && (remain >= 64))
			max_count = MCI_FIFOSIZE;
		else if ((hoststatus & MCI_TXFIFOHALFEMPTY) && (remain >= 32))
			max_count = MCI_FIFOHALFSIZE;
		else if (remain < 32)
			max_count = 1;
		else
			max_count = 0;
		for (count = 0; count < max_count; count++) {
                        if(remain==2) { /*half word aligned, read 16bit half words*/
                                buf16 = (u16*)buffer;
				writew(*buf16, (u16*)(base + MMCIFIFO));
                                buf16++;
                                buffer = (u32*)buf16;
                                data_xfered += 2;
                                remain =0;
                        }else if(remain<4) /*not word aligned, read byte by byte*/
                                {
                                buf8 = (u8*)buffer;
                                for(temp=0; temp<remain; temp++) {
					writew(*buf8, (u8*)(base + MMCIFIFO));
                                        buf8++;
                                }
                                data_xfered += remain;
                                buffer = (u32*)buf8;
                                remain = 0;
                                }
			else{
			writel(*buffer, (base + MMCIFIFO));
			buffer++;
			data_xfered += 4;
			remain -= 4;
		}
		}

		hoststatus = readl(base + MMCISTATUS);
	}
	if (remain) {
		printk(KERN_INFO "While  WRITE, Remain is %d, hoststatus is %x, data_xfered is %d\n",
		     remain, hoststatus, data_xfered);
	}
	return data_xfered;
}
/**
 *   u8500_mmci_read_write() - the function calls corresponding read/write function
 *   @host:  pointer to the u8500_mmci_host structure
 *
 *   this function will call the corresponding read or write function of the polling mode depending
 *   on the flag set in the status register
 */
static void u8500_mmci_read_write(struct u8500_mmci_host *host)
{
	void __iomem *base;
	u32 *buffer;
	u32 hoststatus;
	struct mmc_data *data;
	unsigned int remain, len;
	unsigned long flags;

	int devicemode = mmc_mode;
#ifdef CONFIG_U8500_SDIO
	/*check sdio_mode variable for SDIO card*/
	struct mmc_host *mmc;
	mmc = host->mmc;
	if(host->is_sdio == 1)
		devicemode = sdio_mode;
#endif

	base = host->base;
	data = host->data;

	hoststatus = readl(base + MMCISTATUS);
	u32 temp_flag = MCI_TXFIFOHALFEMPTY | MCI_RXDATAAVLBL;
#ifdef CONFIG_U8500_SDIO
	if(host->cmd && host->cmd->opcode == SD_IO_RW_EXTENDED)
		temp_flag |=  MCI_DATAEND | MCI_DATABLOCKEND;
#endif
	while (host->data_xfered < host->size &&
	       (hoststatus & temp_flag)) {
		/*Do not add any printk or DEBUG at this location */
		if (data->flags & MMC_DATA_READ) {
			buffer = host->buffer;
			remain = host->size;
		} else {
			buffer =
				(u32 *) (page_address(sg_page(host->sg_ptr)) +
						host->sg_ptr->offset) + host->sg_off;
			remain = host->sg_ptr->length - host->sg_off;
		}
		if (devicemode == MCI_POLLINGMODE)
			local_irq_save(flags);
		len = 0;
		if (hoststatus & MCI_RXACTIVE) {
			len =
				u8500_mmci_read(host, buffer, remain, hoststatus);
		} else if (hoststatus & MCI_TXACTIVE) {
			len =
				u8500_mmci_write(host, buffer, remain,
						hoststatus);
		}
		if (devicemode == MCI_POLLINGMODE)
			local_irq_restore(flags);
		spin_lock(&host->lock);
		host->sg_off += len;
		host->data_xfered += len;
		remain -= len;
		if (remain) {
			printk(KERN_INFO "Remain is %d, hoststatus is %x, host->size is %d, host->data_xfered is %d\n",
				remain, readl(base + MMCISTATUS), host->size,
					host->data_xfered);
			spin_unlock(&host->lock);
			break;
		}
		if ((--host->sg_len) && (data->flags & MMC_DATA_WRITE)) {
			host->sg_ptr++;
			host->sg_off = 0;
		} else {
			spin_unlock(&host->lock);
			break;
		}
		spin_unlock(&host->lock);

		hoststatus = readl(base + MMCISTATUS);
	}

	if (devicemode == MCI_INTERRUPTMODE) {
		/* If we're nearing the end of the read, switch to
		 * "any data available" mode */
		if ((hoststatus & MCI_RXACTIVE) &&
			(host->size - host->data_xfered) < MCI_FIFOSIZE) {
			u8500_mmc_enableirqsrc(base, MCI_RXDATAAVLBL);
		}
		/*
		 * If we run out of data, disable the data IRQs; this
		 * prevents a race where the FIFO becomes empty before
		 * the chip itself has disabled the data path, and
		 * stops us racing with our data end IRQ
		 */
		if (host->size == host->data_xfered) {
			u8500_mmc_disableirqsrc(base, MCI_XFER_IRQ_MASK);
		}
	} else if (devicemode == MCI_POLLINGMODE) {
#if 0 //def CONFIG_U8500_SDIO
	if(host->is_sdio== 1)
		u8500_mmc_enableirqsrc(base, MCI_SDIOIT);
#endif
		u8500_mmci_cmd_irq(host, hoststatus);
		u8500_mmci_data_irq(host, hoststatus);
	}
}

/**
 *   convert_from_bytes_to_power_of_two() - converts the bytes to power of two
 *   @x:  value to be converted
 *
 *   this function converts the bytes to power of two
 */
static int convert_from_bytes_to_power_of_two(unsigned int x)
{
	int y = 0;
	y = (x & 0xAAAA) ? 1 : 0;
	y |= ((x & 0xCCCC) ? 1 : 0)<<1;
	y |= ((x & 0xF0F0) ? 1 : 0)<<2;
	y |= ((x & 0xFF00) ? 1 : 0)<<3;

	return y;
}

/**
 *   start_data_xfer() - configures the data control registers
 *   @host:	pointer to the u8500_mmci_host
 *
 *   this function configures the data control registers for enabling the data path,
 *   and direction of data transfer and configures the data length register with
 *   the data size to be transfered and configures the data timer register
 */
static void start_data_xfer(struct u8500_mmci_host *host)
{
	void __iomem *base;
	struct mmc_data *data;
	unsigned int size;
	u32 nmdk_reg, clk_reg;
	unsigned int temp;
	int devicemode = mmc_mode;
#ifdef CONFIG_U8500_SDIO
	/*check sdio_mode variable for SDIO card*/
	struct mmc_host *mmc;
	mmc = host->mmc;
	if(host->is_sdio == 1)
		devicemode = sdio_mode;
#endif
	data = host->data;
	base = host->base;
	size = host->size;

	BUG_ON(!data);
	BUG_ON(!(data->flags & (MMC_DATA_READ | MMC_DATA_WRITE)));
	BUG_ON((data->flags & MMC_DATA_READ) && (data->flags & MMC_DATA_WRITE));

#ifdef CONFIG_U8500_SDIO
	if(host->cmd && (host->cmd->opcode == SD_IO_RW_EXTENDED)){
		clk_reg = readl(host->base + MMCICLOCK);
		if(host->size<8){
			clk_reg &= ~(MCI_HWFC_EN);
		}
		else{
                        clk_reg |= (MCI_HWFC_EN);
		}
		writel(clk_reg, host->base + MMCICLOCK);
		udelay(20);/*give some delay to setup*/
		//printk("\n CMD53 size = %d, clock = %x", host->size, clk_reg);
	}
#endif

	writel(0x0FFFFFFF, (base + MMCIDATATIMER));

	writel(size, (base + MMCIDATALENGTH));

	temp = convert_from_bytes_to_power_of_two(data->blksz);
	nmdk_reg = MCI_DPSM_ENABLE | ((temp & 0xF) << 4);
	if (devicemode == MCI_DMAMODE) {
		/* Enable the DMA mode of the MMC Host Controller */
		nmdk_reg |= MCI_DPSM_DMAENABLE;
		nmdk_reg |= MCI_DPSM_DMAreqctl;
	}
	if ((data->flags & MMC_DATA_READ)) {
		nmdk_reg |= MCI_DPSM_DIRECTION;
	}

	if (devicemode == MCI_DMAMODE)
		u8500_mmc_enableirqsrc(base, MCI_DATA_ERR);
	else if (devicemode == MCI_INTERRUPTMODE) {
		u8500_mmc_enableirqsrc(base, (MCI_DATA_IRQ | MCI_XFER_IRQ));
		if (host->size < MCI_FIFOSIZE) {
			u8500_mmc_enableirqsrc(base, MCI_RXDATAAVLBL);
		}
	}
#if 0 //def CONFIG_U8500_SDIO
        if(host->cmd->opcode==SD_IO_RW_EXTENDED){
                /*check if transfer is in block mode or byte mode*/
                if(host->cmd->arg & 0x08000000) /*block mode*/{
                        nmdk_reg &= ~(MCI_DPSM_MODE);
			nmdk_reg &= ~(MCI_SDIO_ENABLE);
			//printk("--- BLOCK mode");
                }
                else{ /*byte mode*/
			 nmdk_reg &=(~0xF0);/*blk size = 1*/
		        /* sdio specific operations*/
			nmdk_reg |= MCI_SDIO_ENABLE;
			nmdk_reg |= MCI_DPSM_MODE;
			//printk("--- BYTE mode");
                }
                //printk("BLKSIZE = %d, SZ = %d, power of 2= %d", data->blksz,host->size, temp);
		//printk("\n DATA_CTRL = %x, CLK = %x", nmdk_reg, readl(base + MMCICLOCK));
        }
#endif

	writel(nmdk_reg, (base + MMCIDATACTRL));
}

static void u8500_mmci_xfer_irq(struct u8500_mmci_host *host,
				u32 hoststatus)
{
	int devicemode = mmc_mode;
	hoststatus &= MCI_XFER_IRQ_MASK;
	if (!hoststatus)
		return;
#ifdef CONFIG_U8500_SDIO
	/*check sdio_mode variable for SDIO card*/
	struct mmc_host *mmc;
	mmc = host->mmc;
	if(strcmp(mmc_hostname(mmc),"mmc1") == 0)
		devicemode = sdio_mode;
#endif
	BUG_ON(devicemode != MCI_INTERRUPTMODE);
	u8500_mmci_read_write(host);
}
/**
 * u8500_mmci_cmd_irq() - called to service the command status interrupt
 * @host:  pointer to the u8500_mmci_host
 * @hoststatus:  value of the hoststatus register
 *
 * this function processes the command request end in the interrupt mode and will
 * inform the stack about the request end
 */
static void u8500_mmci_cmd_irq(struct u8500_mmci_host *host, u32 hoststatus)
{
	void __iomem *base;
	struct mmc_command *cmd;
	struct mmc_data *data;
	u32 cmdstatusmask;
	int devicemode = mmc_mode;
#ifdef CONFIG_U8500_SDIO
	/*check sdio_mode variable for SDIO card*/
	struct mmc_host *mmc;
	mmc = host->mmc;
	if(host->is_sdio == 1)
		devicemode = sdio_mode;
#endif
	cmd = host->cmd;
	data = host->data;
	base = host->base;
	if (cmd) {
		cmdstatusmask = MCI_CMDTIMEOUT | MCI_CMDCRCFAIL;
		if ((cmd->flags & MMC_RSP_PRESENT))
			cmdstatusmask |= MCI_CMDRESPEND;
		else
			cmdstatusmask |= MCI_CMDSENT;
	} else
		cmdstatusmask = 0;
	hoststatus &= cmdstatusmask;
	if (!hoststatus)
		return;
	process_command_end(host, hoststatus);

	if (!cmd->data || cmd->error != MMC_ERR_NONE) {
		if (cmd->error != MMC_ERR_NONE && data) {
			/*printk("The COMMCND NUMBER IS : %d  status register value is : %d\n",
						cmd->opcode, hoststatus);*/
			if (devicemode == MCI_POLLINGMODE) {
				u8500_mmci_stop_data(host);
				if (!data->stop)
					u8500_mmci_request_end(host, data->mrq);
				else
					u8500_mmci_start_command(host, data->stop);
			} else {
				data->error = cmd->error;
				u8500_mmc_dma_transfer_done(host);
				return;
			}
		} else {
			u8500_mmci_request_end(host, cmd->mrq);
		}
	} else if (!(cmd->data->flags & MMC_DATA_READ)) {
		start_data_xfer(host);
	}

	u8500_mmc_clearirqsrc(base, hoststatus);
}
/**
 * u8500_mmci_data_irq() - called to service the data transfer status interrupt
 * @host:  pointer to the u8500_mmci_host
 * @hoststatus:  value of the hoststatus register
 *
 * this function processes the command with data request end in the interrupt mode and will
 * inform the stack about the request end
 */
static void u8500_mmci_data_irq(struct u8500_mmci_host *host,
					u32 hoststatus)
{
	struct mmc_data *data;
	struct mmc_command *cmd;
	struct mmc_host *mmc;
	int devicemode = mmc_mode;
	hoststatus &= MCI_DATA_IRQ;

	if (!hoststatus) {
		return;
	}
	cmd = host->cmd;
	data = host->data;
	mmc = host->mmc;

#ifdef CONFIG_U8500_SDIO
	/*check sdio_mode variable for SDIO card*/
	if(host->is_sdio == 1)
		devicemode = sdio_mode;
#endif
	if (!data) {
		goto clear_data_irq;
	}
	data->error = check_for_data_err(hoststatus);

	if (data->error != MMC_ERR_NONE) {
	    stm_error("In %d Cmd, data_irq, data->error is %d, data_xfered is %d, hoststatus = %x\n",
		     cmd->opcode, data->error, host->data_xfered, hoststatus);
	}
	if (devicemode == MCI_DMAMODE && data->error != MMC_ERR_NONE) {
		u8500_mmc_dma_transfer_done(host);
		return;
	}
	if (!data->error) {
		if ((data->flags & MMC_DATA_READ)
			&& (devicemode != MCI_DMAMODE)) {
			u32 *buffer_local, *buffer;
			u8500_mmci_init_sg(host, data);
			buffer_local = host->buffer;
			while (host->sg_len--) {
				buffer =
					(u32 *) (page_address(sg_page(host->sg_ptr)) + host->sg_ptr->offset);
						memcpy(buffer, buffer_local,
				host->sg_ptr->length);
				buffer_local += host->sg_ptr->length / 4;
				flush_dcache_page(sg_page(host->sg_ptr));
				host->sg_ptr++;
				host->sg_off = 0;
			}
		}
	}

	u8500_mmci_stop_data(host);

	if (!data->stop)
		u8500_mmci_request_end(host, data->mrq);
	else
		u8500_mmci_start_command(host, data->stop);
clear_data_irq:
	u8500_mmc_clearirqsrc(host->base, hoststatus);
}

/**
 * u8500_mmci_irq() - interrupt handler registred for sd controller interrupt
 * @irq:  irq interrupt source number
 * @dev_id:  dev_id pointer to the current device id
 *
 * this function is the interrupt handler registered for sdmmc interrupt and this
 * will be invoked on any interrupt from mmc
 */
static irqreturn_t u8500_mmci_irq(int irq, void *dev_id)
{
	struct u8500_mmci_host *host = dev_id;
	void __iomem *base;
	u32 hoststatus;

	base = host->base;

	hoststatus = readl(base + MMCISTATUS);
	hoststatus &= readl(base + MMCIMASK0);
#ifdef CONFIG_U8500_SDIO
	if(host->is_sdio==1)
                printk("\n SDIO IRQ =%x", hoststatus);
#endif

	while (hoststatus) {
		if ((hoststatus & MCI_XFER_IRQ_MASK))
			u8500_mmci_xfer_irq(host, hoststatus);
		if ((hoststatus & MCI_CMD_IRQ))
			u8500_mmci_cmd_irq(host, hoststatus);
		if ((hoststatus & MCI_DATA_IRQ))
			u8500_mmci_data_irq(host, hoststatus);
		if(hoststatus & MCI_SDIOIT){
			u8500_mmc_disableirqsrc(base, MCI_SDIOIT);
			u8500_mmc_clearirqsrc(base, MCI_SDIOIT);
			 printk("\n START SDIO IRQ handler");
			 printk("\nfunc[0] = %x, func[1] = %x",
			host->mmc->card->sdio_func[0], host->mmc->card->sdio_func[1]);
			if(host->mmc->card && host->mmc->card->sdio_func[1] && host->mmc->card->sdio_func[1]->irq_handler){
				struct sdio_func *func = host->mmc->card->sdio_func[1];
				host->mmc->card->sdio_func[1]->irq_handler(func);
				printk("\n END SDIO IRQ handler");
				//mmc_signal_sdio_irq(host->mmc);
				}
		}
		u8500_mmc_clearirqsrc(base, hoststatus);
		hoststatus = readl(base + MMCISTATUS);
		hoststatus &= readl(base + MMCIMASK0);
	}

	return IRQ_HANDLED;
}

/**
 * u8500_mmci_start_data() - Processes the command with data request recieved from MMC framework
 * @host:  Pointer to the host structure for MMC
 * @data:  Pointer to the mmc data structure
 * @cmd:  Pointer to the mmc comand structure
 * @mmc_id:  device id to find sdmmc or emmc
 */
static void u8500_mmci_start_data(struct u8500_mmci_host *host,
					struct mmc_data *data,
						struct mmc_command *cmd)
{
	DECLARE_COMPLETION_ONSTACK(complete);
	void __iomem *base;
	u32 nmdk_reg, temp_reg;
	u32 polling_freq_clk_div = 0x0;
	unsigned long flag_lock = 0;
	int devicemode = mmc_mode;

#ifdef CONFIG_U8500_SDIO
	/*check mmc_id variable for SDIO card*/
	if(host->is_sdio==1)
		devicemode = sdio_mode;
#endif

	spin_lock_irqsave(&host->lock, flag_lock);
	host->data = data;
	host->cmd = cmd;
	host->size = data->blocks * data->blksz;
	host->data_xfered = 0;
	base = host->base;

	spin_unlock_irqrestore(&host->lock, flag_lock);

	u8500_mmci_init_sg(host, data);

#ifdef CONFIG_U8500_SDIO
	if(host->is_sdio==1) /*for SDIO*/
		polling_freq_clk_div = 0x4;
		/* HACK: value of clk div =4 is the lowest ,
		 * i.e, max clk 8.33 MHz is working with WLAN.*/
/*
	if(host->cmd->opcode == SD_IO_RW_EXTENDED){
		if(host->size>=32){
			sdio_mode = MCI_DMAMODE;
			polling_freq_clk_div = 0x02;
			}
		else{
			sdio_mode = MCI_POLLINGMODE;
			polling_freq_clk_div = 0x02;
		}
		devicemode = sdio_mode;
	}
*/
#endif
	cmd->error = MMC_ERR_NONE;
	data->error = MMC_ERR_NONE;

	if (devicemode == MCI_POLLINGMODE) {
		nmdk_reg = readl(base + MMCICLOCK);
		nmdk_reg = (nmdk_reg & ~(0xFF)) | ((u8) polling_freq_clk_div & 0xFF);
		writel(0x03, host->base + MMCIPOWER);
		writel(nmdk_reg, (base + MMCICLOCK));
	}
	if (devicemode == MCI_DMAMODE)
		u8500_mmc_set_dma(host);
	/* For a read we need to write to the data ctrl register first, then
	 * send the command. For writes, the order is reversed.
	 */
	if ((data->flags & MMC_DATA_READ))
		start_data_xfer(host);
	if (devicemode == MCI_DMAMODE)
		host->dma_done = &complete;
	u8500_mmc_send_cmd(base, cmd->opcode, cmd->arg, cmd->flags, devicemode);
	if (devicemode == MCI_POLLINGMODE) {
		if (!(data->flags & MMC_DATA_READ))
			wait_for_command_end(host);
		while (data->error == MMC_ERR_NONE && host->data) {
			u8500_mmci_read_write(host);
			schedule();
		}
	}
	return ;
}
/**
 * u8500_mmci_start_command() - Processes the command request recieved from MMC framework
 * @host:  Pointer to the host structure for MMC
 * @cmd:  Pointer to the mmc command structure
 */
static void
u8500_mmci_start_command(struct u8500_mmci_host *host,
			   struct mmc_command *cmd)
{
	void __iomem *base = host->base;
	int devicemode = mmc_mode;

#ifdef CONFIG_U8500_SDIO
	/*check sdio_mode variable for SDIO card*/
	struct mmc_host *mmc;
	mmc = host->mmc;
	if(host->is_sdio == 1)
		devicemode = sdio_mode;
#endif
	host->cmd = cmd;
	u8500_mmc_send_cmd(base, cmd->opcode, cmd->arg, cmd->flags, devicemode);
	if (devicemode == MCI_POLLINGMODE) {
		wait_for_command_end(host);
	}
	return;
}

/**
 * u8500_mmci_request() - Processes the request recieved from MMC framework
 * @mmc:  Pointer to the host structure for MMC
 * @mrq:  Pointer to the mmc request structure
 */
static void u8500_mmci_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct u8500_mmci_host *host = mmc_priv(mmc);
	WARN_ON(host->mrq != NULL);
	host->mrq = mrq;
	if (mrq->data && mrq->data->flags & (MMC_DATA_READ | MMC_DATA_WRITE))
		u8500_mmci_start_data(host, mrq->data, mrq->cmd);
	else
		u8500_mmci_start_command(host, mrq->cmd);
}

/**
 *  u8500_mmci_set_ios() - Perform configuration related settings for MMC host
 *  @mmc: the pointer to the host structure for MMC
 *  @ios: pointer to the configuration settings to be done
 */
static void u8500_mmci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct u8500_mmci_host *host = mmc_priv(mmc);
	u32 clk = 0;
	int devicemode = mmc_mode;

#ifdef CONFIG_U8500_SDIO
	/*check sdio_mode variable for SDIO card*/
	if(host->is_sdio == 1)
		devicemode = sdio_mode;
#endif
	/* Make sure we aren't changing the control registers too soon after
	 * writing data. */
	udelay(10);
	/* Set the bus and power modes */
	switch (ios->power_mode) {
	case MMC_POWER_OFF:
		if (!host->level_shifter) {
			writel(0x40, host->base + MMCIPOWER);
		} else {
			writel(0xBC, host->base + MMCIPOWER);
		}
	break;
	case MMC_POWER_UP:
		if (!host->level_shifter) {
			writel(0x42, host->base + MMCIPOWER);
		} else {
			writel(0xBE, host->base + MMCIPOWER);
		}
	break;
	case MMC_POWER_ON:
		if (!host->level_shifter)
			writel(0x43, host->base + MMCIPOWER);
		else
			writel(0xBF, host->base + MMCIPOWER);
	break;
	}
	if (ios->clock) {
		if (ios->clock >= host->mclk)
			clk |= (MCI_CLK_BYPASS | MCI_NEG_EDGE);
		else {
			u32 div = host->mclk / ios->clock;

			if (div > 2) {
				clk = div - 2;
				if (clk > 255)
					clk = 255;
			} else
				clk = 0;

			host->cclk = host->mclk / (clk + 2);
			if (host->cclk > ios->clock) {
				clk += 1;
				host->cclk = host->mclk / (clk + 2);
			}
		}
		clk |= (MCI_HWFC_EN | MCI_CLK_ENABLE);
		writel(clk, host->base + MMCICLOCK);
	}

	if (!host->level_shifter) {
		if (ios->bus_mode == MMC_BUSMODE_PUSHPULL)
			writel(0x03, host->base + MMCIPOWER);
	}
	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_1:
	break;
	case MMC_BUS_WIDTH_4:
		clk = readl(host->base + MMCICLOCK);
		clk |= MCI_BUS_WIDTH_4;
		writel(clk, host->base + MMCICLOCK);
	break;
	case MMC_BUS_WIDTH_8:
		clk = readl(host->base + MMCICLOCK);
		clk |= MCI_BUS_WIDTH_8;
		writel(clk, host->base + MMCICLOCK);
	break;
	default:
		stm_info("u8500_mmci_setios(): whats going on ??\n");
	break;
	}
}
static struct mmc_host_ops u8500_mmci_ops = {
	.request = u8500_mmci_request,
	.set_ios = u8500_mmci_set_ios,
};

/**
 *  u8500_mmci_check_status() - checks the status of the card on every interval
 *  @void: No arguments
 *
 *  this function will check the status of the card on every interval
 *  of the time configured and inform the status to the mmc stack
 */
static void u8500_mmci_check_status(void *data)
{
	struct u8500_mmci_host *host = (struct u8500_mmci_host *) data;
	char status;

	status = host->card_detect_intr_value();
	status ^= host->oldstat;
	if (status)
		mmc_detect_change(host->mmc, 0);
	host->oldstat = status;
	return;
}

/**
 *  u8500_mmci_probe() - intilaizing the mmc device
 *  @dev:	mmc device pointer
 *  @id: bus_id
 *
 *  this function will use the current mmc device pointer, initialize and configure
 *  the device, allocate the memory for the device,request the memory region with the
 *  device name and scan the device
 */
static int u8500_mmci_probe(struct amba_device *dev, void *id)
{
	struct u8500_mmci_host *host;
	struct mmc_host *mmc;
	int ret;
	int devicemode = mmc_mode;
	struct mmc_board *board = dev->dev.platform_data;
	struct stm_dma_pipe_info pipe_info1, pipe_info2;
	stm_info("MMC: operating mode is set to %d [where 1=dma,2=poll,3=intr]\n", devicemode);
	if (!board) {
		stm_error("mmc: Platform data not set\n");
		return -EINVAL;
	}
#ifdef CONFIG_U8500_SDIO
	if(strcmp(dev->dev.bus_id,"SDIO") == 0){
		devicemode = sdio_mode;
	}
	else {
		devicemode = mmc_mode;
	}
#endif
	ret = board->init(dev);
	if (ret) {
		stm_error("\n Error in configuring MMC");
		goto out;
	}
	ret = amba_request_regions(dev, DRIVER_NAME);
	if (ret) {
		stm_error("\n Error in amba_request_region");
		goto configure_mmc;
	}
	mmc = mmc_alloc_host(sizeof(struct u8500_mmci_host), &dev->dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto rel_regions;
	}
	host = mmc_priv(mmc);
	host->oldstat = -1;
	host->mclk = CLK_MAX;
	host->mmc = mmc;
	host->dma_fifo_addr = board->dma_fifo_addr;
	host->dma_fifo_dev_type_rx = board->dma_fifo_dev_type_rx;
	host->dma_fifo_dev_type_tx = board->dma_fifo_dev_type_tx;
	if (board->level_shifter)
		host->level_shifter = board->level_shifter;
	if (devicemode == MCI_POLLINGMODE) {
		host->buffer = vmalloc(MAX_DATA);
		if (!host->buffer) {
			ret = -ENOMEM;
			goto free_mmc;
		}
	}
	host->base = ioremap(dev->res.start, SZ_4K);
	if (!host->base) {
		ret = -ENOMEM;
		goto host_free;
	}
	mmc->ops = &u8500_mmci_ops;
	mmc->f_max = min(host->mclk, fmax);
	mmc->f_min = CLK_MAX / (CLK_DIV + 2);
	if (devicemode == MCI_DMAMODE)
		mmc->caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA |
					MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED;
#ifdef CONFIG_U8500_SDIO
	if(strcmp(dev->dev.bus_id,"SDIO") == 0){
		host->is_sdio = 1;
		mmc->caps = 0; //MMC_CAP_4_BIT_DATA;
	}
	else
		host->is_sdio = 0;

#endif
	/*
	 * We can do SGIO
	 */
	if (devicemode == MCI_DMAMODE) {
		/* Can't do scatter/gather DMA */
		mmc->max_hw_segs = 1;
		mmc->max_phys_segs = 1;
	} else {
		mmc->max_hw_segs = NR_SG;
		mmc->max_phys_segs = NR_SG;
	}
	mmc->ocr_avail = OCR_AVAIL;
	/* XXX No errors in Tx/Rx but writes garbage data with
		mmc->max_req_size = 32768; */
#ifdef DMA_SCATERGATHER
	mmc->max_req_size = 65535;
#else
	mmc->max_req_size = 4096;
#endif
	mmc->max_seg_size = mmc->max_req_size;
#if 0 //def CONFIG_U8500_SDIO
	if(strcmp(dev->dev.bus_id,"SDIO") == 0){
		mmc->max_blk_size = 128;
		mmc->max_req_size = 128;
		mmc->max_seg_size = mmc->max_req_size;
	}
	else
#endif
	mmc->max_blk_size = 4096;
	mmc->max_blk_count = mmc->max_req_size;
	spin_lock_init(&host->lock);
	host->clk = clk_get(NULL, dev->dev.bus_id);
	if (!host->clk) {
		ret = PTR_ERR(host->clk);
		goto unmap;
	}
	clk_enable(host->clk);
	writel(0, host->base + MMCIMASK0);
	writel(0xfff, host->base + MMCICLEAR);
	if (devicemode != MCI_POLLINGMODE) {
		ret =
			request_irq(dev->irq[0], u8500_mmci_irq, IRQF_DISABLED/*SA_INTERRUPT*/,
				DRIVER_NAME, host);
		if (ret)
			goto put_clk;
	}
	amba_set_drvdata(dev, mmc);
	if (board->card_detect) {
		board->card_detect(u8500_mmci_check_status, (void *)host);
		host->card_detect_intr_value = board->card_detect_intr_value;
	}
	ret = mmc_add_host(mmc);
	if (ret) {
		if (devicemode != MCI_POLLINGMODE)
			goto irq0_free;
		else
			goto unmap;
	}
	stm_info("%s: MMCI rev %x cfg %02x at 0x%08x irq %d\n",
			mmc_hostname(mmc), amba_rev(dev), amba_config(dev),
				dev->res.start, dev->irq[0]);
	if (devicemode == MCI_DMAMODE) {
		memset(&pipe_info1, 0, sizeof(struct stm_dma_pipe_info));
		memset(&pipe_info2, 0, sizeof(struct stm_dma_pipe_info));
#ifndef DMA_SCATERGATHER
		host->dma_buffer = (u32 *)dma_alloc_coherent(NULL, 4096, &host->dma, (GFP_KERNEL | GFP_DMA));
#endif
		pipe_info1.channel_type = (STANDARD_CHANNEL|CHANNEL_IN_LOGICAL_MODE|
						LCHAN_SRC_LOG_DEST_LOG|NO_TIM_FOR_LINK|
							NON_SECURE_CHANNEL|LOW_PRIORITY_CHANNEL);

		/* Request dmapipe for mmc to mem operation */
		pipe_info1.dir = PERIPH_TO_MEM;
		pipe_info1.src_dev_type = host->dma_fifo_dev_type_rx;
		pipe_info1.dst_dev_type = DMA_DEV_DEST_MEMORY;

		pipe_info1.src_info.endianess = DMA_LITTLE_ENDIAN;
		pipe_info1.src_info.data_width = DMA_WORD_WIDTH;
		pipe_info1.src_info.burst_size = DMA_BURST_SIZE_1;
		pipe_info1.src_info.buffer_type = SINGLE_BUFFERED;

		pipe_info1.dst_info.endianess = DMA_LITTLE_ENDIAN;
		pipe_info1.dst_info.data_width = DMA_WORD_WIDTH;
		pipe_info1.dst_info.burst_size = DMA_BURST_SIZE_1;
		pipe_info1.dst_info.buffer_type = SINGLE_BUFFERED;

		if (stm_request_dma(&host->dmach_mmc2mem, &pipe_info1)) {
			stm_error("RX pipe request failed \n");
			goto remove_host;
		}
		if (stm_configure_dma_channel(host->dmach_mmc2mem, &pipe_info1))
			goto mmc2mem_dmareq_failed;
		if (stm_set_callback_handler(host->dmach_mmc2mem, &u8500_mmc_dmaclbk, (void *)host))
			goto mmc2mem_dmareq_failed;

		/* Request dmapipe for mem to mmc operation */

		pipe_info2.channel_type = (STANDARD_CHANNEL|CHANNEL_IN_LOGICAL_MODE|
						LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK |
							NON_SECURE_CHANNEL|LOW_PRIORITY_CHANNEL);

		pipe_info2.dir = MEM_TO_PERIPH;
		pipe_info2.src_dev_type = DMA_DEV_SRC_MEMORY;
		pipe_info2.dst_dev_type = host->dma_fifo_dev_type_tx;
		pipe_info2.src_info.endianess = DMA_LITTLE_ENDIAN;
		pipe_info2.src_info.data_width = DMA_WORD_WIDTH;
		pipe_info2.src_info.burst_size = DMA_BURST_SIZE_1;
		pipe_info2.src_info.buffer_type = SINGLE_BUFFERED;

		pipe_info2.dst_info.endianess = DMA_LITTLE_ENDIAN;
		pipe_info2.dst_info.data_width = DMA_WORD_WIDTH;
		pipe_info2.dst_info.burst_size = DMA_BURST_SIZE_1;
		pipe_info2.dst_info.buffer_type = SINGLE_BUFFERED;

		if (stm_request_dma(&host->dmach_mem2mmc, &pipe_info2)) {
			stm_error("TX pipe request failed \n");
			goto remove_host;
		}
		if (stm_configure_dma_channel(host->dmach_mem2mmc, &pipe_info2))
			goto mem2mmc_dmareq_failed;
		if (stm_set_callback_handler(host->dmach_mem2mmc, &u8500_mmc_dmaclbk, (void *)host))
			goto mem2mmc_dmareq_failed;
	}

	return 0;

mem2mmc_dmareq_failed:
	stm_free_dma(host->dmach_mem2mmc);
mmc2mem_dmareq_failed:
	stm_free_dma(host->dmach_mmc2mem);
remove_host:
	mmc_remove_host(mmc);
irq0_free:
	free_irq(dev->irq[0], host);
put_clk:
	clk_disable(host->clk);
	clk_put(host->clk);
unmap:
	iounmap(host->base);
host_free:
	vfree(host->buffer);
free_mmc:
	mmc_free_host(mmc);
rel_regions:
	amba_release_regions(dev);
configure_mmc:
	board->exit(dev);
out:
	return ret;
}
/**
 *  u8500_mmci_remove() - Remove the mmc by freeing all resources
 *  @dev:	current mmc device pointer
 *
 *  this function will remove the mmc by freeing all resources used by the device by using
 *  the current mmc device pointer
 */
static int u8500_mmci_remove(struct amba_device *dev)
{
	struct mmc_board *board = dev->dev.platform_data;
	struct mmc_host *mmc = amba_get_drvdata(dev);

	stm_info("MMC module removed \n");
	if (!board)
		return -EINVAL;
	amba_set_drvdata(dev, NULL);
	if (mmc) {
		struct u8500_mmci_host *host = mmc_priv(mmc);
		stm_free_dma(host->dmach_mmc2mem);
		stm_free_dma(host->dmach_mem2mmc);
		host->dmach_mmc2mem = -1;
		host->dmach_mem2mmc = -1;
		mmc_remove_host(mmc);
		writel(0, host->base + MMCIMASK0);
		writel(0, host->base + MMCICOMMAND);
		writel(0, host->base + MMCIDATACTRL);
		free_irq(dev->irq[0], host);
		iounmap(host->base);
		vfree(host->buffer);
		mmc_free_host(mmc);
		amba_release_regions(dev);
		board->exit(dev);
		clk_disable(host->clk);
		clk_put(host->clk);
	}
	return 0;
}
#ifdef CONFIG_PM
/**
 *  u8500_mmci_suspend() - Use the current mmc device pointer and called the mmc suspend function
 *  @dev: current mmc device pointer
 *  @state: state suspend state
 */
static int u8500_mmci_suspend(struct amba_device *dev, pm_message_t state)
{
	struct mmc_host *mmc = amba_get_drvdata(dev);
	int ret = 0;
	if (strcmp(dev->dev.bus_id, "EMMC") != 0) {
		if (mmc) {
			struct u8500_mmci_host *host = mmc_priv(mmc);
			ret = mmc_suspend_host(mmc, state);
			if (ret == 0)
				writel(0, host->base + MMCIMASK0);
			clk_disable(host->clk);
		}
	}
	return ret;
}
/**
 *  u8500_mmci_resume() - Use the current mmc device pointer and called the mmc resume function
 *  @dev:  mmc device pointer
 */
static int u8500_mmci_resume(struct amba_device *dev)
{
	struct mmc_host *mmc = amba_get_drvdata(dev);
	int ret = 0;
	if (strcmp(dev->dev.bus_id, "EMMC") != 0) {
		if (mmc) {
			struct u8500_mmci_host *host = mmc_priv(mmc);
			clk_enable(host->clk);
			writel(MCI_IRQENABLE, host->base + MMCIMASK0);
			ret = mmc_resume_host(mmc);
		}
	}
	return ret;
}
#else
#define u8500_mmci_suspend	NULL
#define u8500_mmci_resume	NULL
#endif


static struct amba_id u8500_mmci_ids[] = {
	{
	.id = SDI_PER_ID,
	.mask = SDI_PER_MASK,
	},
	{0, 0, 0},
};

static struct amba_driver u8500_mmci_driver = {
	.drv = {
		.name = DRIVER_NAME,
		},
	.probe = u8500_mmci_probe,
	.remove = u8500_mmci_remove,
	.suspend = u8500_mmci_suspend,
	.resume = u8500_mmci_resume,
	.id_table = u8500_mmci_ids,
};
/**
 *  u8500_mmci_init() - register the mmc driver as amba driver
 *  @void: No arguments
 */
static int __init u8500_mmci_init(void)
{
	return amba_driver_register(&u8500_mmci_driver);
}
/**
 *  u8500_mmci_exit() - unregister the amba mmc driver
 *  @void: No arguments
 */
static void __exit u8500_mmci_exit(void)
{
	amba_driver_unregister(&u8500_mmci_driver);
}
/**
 *  module_init -  register the amba mmc driver to the kernel
 *  @u8500_mmci_init:  mmc init function call
 */
module_init(u8500_mmci_init);
/**
 *  module_exit() -  unregister the amba mmc driver to the kernel
 *  @u8500_mmci_exit:  mmc exit function call
 */
module_exit(u8500_mmci_exit);

MODULE_AUTHOR("Hanumath Prasad(hanumath.prasad@stericcson.com)");
MODULE_DESCRIPTION("ARM PrimeCell PL180 Multimedia Card Interface driver");
MODULE_LICENSE("GPL");
