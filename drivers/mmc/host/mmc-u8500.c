/*
 * Overview:
 *  	 SD/EMMC driver for u8500 platform
 *
 * Copyright (C) 2009 ST-Ericsson SA
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
 * @brief This file contains the sdmmc/emmc chip initialization with default
 * as DMA mode.Polling/Interrupt mode support could be enabled in the kernel
 * menuconfig. Read and write function definitions to configure the read and
 * write functionality was implemented.
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
#include <linux/mmc/card.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/amba/bus.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/completion.h>
#include <linux/regulator/consumer.h>

#include <asm/cacheflush.h>
#include <asm/div64.h>
#include <asm/io.h>
#include <mach/dma.h>
#include <asm/scatterlist.h>
#include <asm/sizes.h>
#include <mach/hardware.h>
#include <mach/mmc.h>
#include <asm/bitops.h>
#include <mach/mmc.h>
#include <mach/debug.h>
#include <mach/dma_40-8500.h>
#include "mmc-u8500.h"

/* Workaround for SDIO Multibyte mode, not suuported in H/W */
#ifdef SDIO_MULTIBYTE_WORKAROUND
#undef SDIO_MULTIBYTE_WORKAROUND
/* Not fully validated, to be enabled from menuconfig later */
#endif

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
const int sdio_mode = MCI_POLLINGMODE;
#elif defined CONFIG_U8500_SDIO_INTR
const int sdio_mode = MCI_INTERRUPTMODE;
#elif defined CONFIG_U8500_SDIO_DMA
const int sdio_mode = MCI_DMAMODE;
#else
const int sdio_mode = 0xFFFF;
#endif

#if !defined CONFIG_U8500_MMC_INTR
static unsigned int fmax = MMC_HOST_CLK_MAX;
#else
static unsigned int fmax = MMC_HOST_CLK_MAX / 8;
#endif

#if !defined CONFIG_U8500_MMC_DMA
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
/*
 * Pointer to save SDIO host data structure.
 * Required for sysfs inplementation for card detection
 */
static struct u8500_mmci_host *sdio_host_ptr;
#define CARD_DETECT_DELAY_MSEC (10)

/* For SDIO host DMA mode, min data transfer size in bytes */
#define MIN_DATA_SIZE_DMA 	32
/* DMA element size in bytes */
#define DMA_ELEMENT_SZ		4
/**
 * u8500_sdio_detect_card() - Initiates card scan for sdio host
 *
 *  this function will scan for insertion/removal of sdio card.
 *  This is required to initiate card rescan from sdio client device driver.
 */
void u8500_sdio_detect_card(void)
{
	struct u8500_mmci_host *host = sdio_host_ptr;
	if (sdio_host_ptr && host->mmc)
		mmc_detect_change(host->mmc,
				  msecs_to_jiffies(CARD_DETECT_DELAY_MSEC));

	return;
}
EXPORT_SYMBOL(u8500_sdio_detect_card);

static ssize_t sdio_detect_card_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned int scan_val;

	if (sscanf(buf, "%u", &scan_val) != 1)
		return -EINVAL;
	/*
	 * unsigned integer is valid entry.
	 * mmc_detect_change is called when scan_val == 1.
	 */
	if (scan_val != 1)
		return -EINVAL;

	/* call card scan */
	u8500_sdio_detect_card();

	return strnlen(buf, count);
}

static DEVICE_ATTR(detect_card, S_IWUSR, NULL, sdio_detect_card_store);


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
	irqsrc &= ~(MCI_SDIOIT);
	writel(irqsrc, (base + MMCICLEAR));
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
	u32 temp_reg;
	irqsrc &= ~(MCI_SDIOIT);
	temp_reg = readl(base + MMCIMASK0);
	temp_reg &= ~irqsrc;
	writel(temp_reg, (base + MMCIMASK0));
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
	u32 temp_reg;
	irqsrc &= ~(MCI_SDIOIT);
	temp_reg = readl(base + MMCIMASK0);
	temp_reg |= irqsrc;
	writel(temp_reg, (base + MMCIMASK0));
}

/**
 *   u8500_mmci_request_end() - Informs the stack regarding the completion of the request
 *   @host:	pointer to the u8500_mmci_host structure
 *   @mrq:	pointer to the mmc_request structure
 *
 *   this function will be called after servicing the request sent from the
 *   stack.And this function will inform the stack that the request is done
 */
static void
u8500_mmci_request_end(struct u8500_mmci_host *host,
			 struct mmc_request *mrq)
{
	unsigned long flag_lock = 0;
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
	unsigned long flag_lock = 0;
	u32 temp = 0;
	MMC_LOCK(host->lock, flag_lock);
	temp = readl(host->base + MMCIDATACTRL) & ~(MCI_DPSM_ENABLE);
	writel(temp, host->base + MMCIDATACTRL);
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
 *   @devicemode: devicemode for the data transfer
 *
 *   this function will send the cmd to the card by enabling the
 *   command path state machine and writing the cmd number and arg to the
 *   corresponding command and arguement registers
 */
static void u8500_mmc_send_cmd(void *base, u32 cmd, u32 arg, u32 flags,
			       int devicemode)
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

	if (devicemode != MCI_POLLINGMODE)
		u8500_mmc_enableirqsrc(base, irqmask);
}

/**
 *   process_command_end() - read the response for the corresponding cmd sent
 *   @host:	pointer to the u8500_mmci_host
 *   @hoststatus:	status register value
 *
 *   this function will read the response of the corresponding command by
 *   reading the response registers
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
		/* printk(KERN_ERR "Command Timeout: %d\n", cmd->opcode); */
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
 *   this function will wait for whether the command completion has
 *   happened or any error generated by reading the status register
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
 *   check_for_data_err() - checks the status register value for the error
 *   @hoststatus:	value of the host status register
 *
 *   this function will read the status register and will return the
 *   error if any generated during the operation
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
 *   this  function will update the stack with the data read and about
 *   the request completion
 */
static void u8500_mmc_dma_transfer_done(struct u8500_mmci_host *host)
{
	struct mmc_data *data;
	struct mmc_host *mmc;
	void __iomem *base;
	u32 temp_reg;
	int retry;
	base = host->base;
	data = host->data;
	mmc = host->mmc;
	host->dma_done = NULL;
	if (data == NULL) {
		stm_error("what? data is null, go back  \n");
		u8500_mmci_request_end(host, host->mrq);
		return;
	}
	/*
	 * Need to wait for end of transfer, i.e.,
	 * DATAEND bit to be set in SDI STATUS register
	 */
	temp_reg = readl(base + MMCISTATUS);
	retry = 200*100; /* Max wait for 100 usec */
	while (!(temp_reg & MCI_DATAEND) && (retry > 0)) {
		ndelay(5);
		retry--;
		temp_reg = readl(host->base + MMCISTATUS);
	}

#ifndef DMA_SCATERGATHER
	spin_lock(&host->lock);
	if (!data->error) {
			if ((data->flags & MMC_DATA_READ)) {
				u32 *buffer_local, *buffer;
				u8500_mmci_init_sg(host, data);
				buffer_local = host->dma_buffer;
				while (host->sg_len--) {
					buffer = (u32 *) (page_address(sg_page(host->sg_ptr)) +
							  host->sg_ptr->offset);
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
	return;
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
/* #warning " scatter gather is compiled " */
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
	if ((16 >= host->size) || ((host->cmd->opcode != SD_IO_RW_EXTENDED) && (8 >= host->size))) {
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
				/* set_dma_sg(host->dmach_mmc2mem, data->sg, data->sg_len); */
				stm_set_dma_addr(host->dmach_mmc2mem,
						(void *)host->dma_fifo_addr,
							(unsigned int *) sg_phys(data->sg));
			#else
			stm_set_dma_addr(host->dmach_mmc2mem,
					(void *)host->dma_fifo_addr,
						/* (physical_address *) */buffer);
			#endif
			stm_enable_dma(host->dmach_mmc2mem);
	} else {
			stm_set_dma_count(host->dmach_mem2mmc, host->size);
			#ifdef DMA_SCATERGATHER
				/* set_dma_sg(host->dmach_mem2mmc, data->sg, data->sg_len); */
				stm_set_dma_addr(host->dmach_mem2mmc, (unsigned int *) sg_phys(data->sg),
						(void *)host->dma_fifo_addr);
			#else
			stm_set_dma_addr(host->dmach_mem2mmc,/* (physical_address *) */ buffer,
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
	u8 *char_buf;
	u16 *word_buf;
	unsigned int temp;
	u32 flag = MCI_DATA_IRQ;

	base = host->base;
	if (host->cmd && host->cmd->opcode == SD_IO_RW_EXTENDED)
		flag = MCI_DATA_ERR; /* required for SDIO CMD53 */

	while ((remain > 0) && !(hoststatus & flag)) {
		if ((hoststatus & MCI_RXFIFOHALFFULL) && (remain >= 32))
			max_count = MCI_FIFOHALFSIZE;
		else if ((hoststatus & MCI_RXDATAAVLBL) && (remain < 32))
			max_count = 1;
		else
			max_count = 0;
		for (count = 0; count < max_count; count++) {
			if (remain == 2) { /* half word aligned, read 16bit half words */
				word_buf = (u16 *)buffer;
				*word_buf = readw((u16 *)(base + MMCIFIFO));
				word_buf++;
				buffer = (u32 *)word_buf;
				data_xfered += 2;
				remain = 0;
			} else if (remain < 4) {/* not word aligned, read byte by byte */
				char_buf = (u8 *)buffer;
				for (temp = 0; temp < remain; temp++) {
					*char_buf = readb((u8 *)(base + MMCIFIFO));
					char_buf++;
				}
				data_xfered += remain;
				buffer = (u32 *)char_buf;
				remain = 0;
			} else {
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
		 *	    remain, hoststatus, data_xfered);
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
	int count, max_count;
	unsigned int data_xfered = 0, temp;
	u16 *buf16;
	u8 *buf8;
	u32 flag = MCI_DATA_IRQ;

	base = host->base;
	if (host->cmd && host->cmd->opcode == SD_IO_RW_EXTENDED)
		flag = MCI_DATA_ERR; /* required for SDIO CMD53 */

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
			if (remain == 2) { /* half word aligned, read 16bit half words */
				buf16 = (u16 *)buffer;
				writew(*buf16, (u16 *)(base + MMCIFIFO));
				buf16++;
				buffer = (u32 *)buf16;
				data_xfered += 2;
				remain = 0;
			} else if (remain < 4) /* not word aligned, read byte by byte */ {
				buf8 = (u8 *)buffer;
				for (temp = 0; temp < remain; temp++) {
					writew(*buf8, (u8 *)(base + MMCIFIFO));
					buf8++;
				}
				data_xfered += remain;
				buffer = (u32 *)buf8;
				remain = 0;
			} else {
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

	u32 temp_flag = MCI_TXFIFOHALFEMPTY | MCI_RXDATAAVLBL;
	base = host->base;
	data = host->data;

	hoststatus = readl(base + MMCISTATUS);
	if (host->cmd && host->cmd->opcode == SD_IO_RW_EXTENDED)
		temp_flag |=  MCI_DATAEND | MCI_DATABLOCKEND; /* Required for SDIO CMD53 */

	while (host->data_xfered < host->size &&
	       (hoststatus & temp_flag)) {
		/* Do not add any printk or DEBUG at this location */
		if (data->flags & MMC_DATA_READ) {
			buffer = host->buffer;
			remain = host->size;
		} else {
			buffer =
				(u32 *) (page_address(sg_page(host->sg_ptr)) +
						host->sg_ptr->offset) + host->sg_off;
			remain = host->sg_ptr->length - host->sg_off;
		}
		if (host->devicemode == MCI_POLLINGMODE)
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
		if (host->devicemode == MCI_POLLINGMODE)
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

	if (host->devicemode == MCI_INTERRUPTMODE) {
		/** If we're nearing the end of the read, switch to
		 * "any data available" mode
		 */
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
	} else if (host->devicemode == MCI_POLLINGMODE) {

		u8500_mmci_cmd_irq(host, hoststatus);
		u8500_mmci_data_irq(host, hoststatus);
	}
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
	u32 temp_reg;
	u32 clk_reg;
	unsigned int temp;
	data = host->data;
	base = host->base;
	size = host->size;

	BUG_ON(!data);
	BUG_ON(!(data->flags & (MMC_DATA_READ | MMC_DATA_WRITE)));
	BUG_ON((data->flags & MMC_DATA_READ) && (data->flags & MMC_DATA_WRITE));
	BUG_ON(!host->cmd);

	/**
	 * Required for SDIo CMD53, for transfer sizes <8, H/W flow
	 * control should be disabled
	 */
	if (host->cmd->opcode == SD_IO_RW_EXTENDED) {
		clk_reg = readl(host->base + MMCICLOCK);
		if (host->size < 8)
		    clk_reg &= ~(MCI_HWFC_EN);
		else
		    clk_reg |= (MCI_HWFC_EN);
		writel(clk_reg, host->base + MMCICLOCK);
	}

	writel(0x0FFFFFFF, (base + MMCIDATATIMER));

	/**
	 * Work around for SDIO multibyte mode, required for H/W
	 * limitation, as byte mode is not supported in host controller
	 */
#ifdef SDIO_MULTIBYTE_WORKAROUND
	if (host->cmd && (host->cmd->opcode == SD_IO_RW_EXTENDED) && !(host->cmd->arg & 0x08000000)) {
		temp = __fls(host->aligned_blksz);
		writel(host->aligned_size, (base + MMCIDATALENGTH));
	} else {
		writel(size, (base + MMCIDATALENGTH));
		temp = __fls(data->blksz);
		/* Get the last set bit position i.e power of two */
	}
#else
	writel(size, (base + MMCIDATALENGTH));
	temp = __fls(data->blksz); /* Get the last set bit position, i.e., power of two */
#endif /* End of SDIO_MULTIBYTE_WORKAROUND */

	temp_reg = MCI_DPSM_ENABLE | ((temp & 0xF) << 4);

	if (host->devicemode == MCI_DMAMODE) {
		/* Enable the DMA mode of the MMC Host Controller */
		temp_reg |= MCI_DPSM_DMAENABLE;
		temp_reg |= MCI_DPSM_DMAreqctl;
	}
	if ((data->flags & MMC_DATA_READ)) {
		temp_reg |= MCI_DPSM_DIRECTION;
	}

	if (host->devicemode == MCI_DMAMODE)
		u8500_mmc_enableirqsrc(base, MCI_DATA_ERR);
	else if (host->devicemode == MCI_INTERRUPTMODE) {
		u8500_mmc_enableirqsrc(base, (MCI_DATA_IRQ | MCI_XFER_IRQ));
		if (host->size < MCI_FIFOSIZE) {
			u8500_mmc_enableirqsrc(base, MCI_RXDATAAVLBL);
		}
	}
	/* Required for SDIO CMD53 and SDIO interrupt */
	if (host->cmd->opcode == SD_IO_RW_EXTENDED)
		temp_reg |= MCI_SDIO_ENABLE;

	writel(temp_reg, (base + MMCIDATACTRL));

}
static void u8500_mmci_xfer_irq(struct u8500_mmci_host *host,
				u32 hoststatus)
{
	hoststatus &= MCI_XFER_IRQ_MASK;
	if (!hoststatus)
		return;
	BUG_ON(host->devicemode != MCI_INTERRUPTMODE);
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
			/* printk("The COMMCND NUMBER IS : %d  status register value is : %d\n",
						cmd->opcode, hoststatus); */
			if (host->devicemode == MCI_POLLINGMODE) {
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
	hoststatus &= ~(MCI_SDIOIT);
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
	hoststatus &= MCI_DATA_IRQ;

	if (!hoststatus) {
		return;
	}
	cmd = host->cmd;
	data = host->data;
	mmc = host->mmc;

	if (!data) {
		goto clear_data_irq;
	}
	data->error = check_for_data_err(hoststatus);

	if (data->error != MMC_ERR_NONE) {
	    stm_error("In %d Cmd, data_irq, data->error is %d, data_xfered is %d, hoststatus = %x\n",
		     cmd->opcode, data->error, host->data_xfered, hoststatus);
	}
	if (host->devicemode == MCI_DMAMODE && data->error != MMC_ERR_NONE) {
		u8500_mmc_dma_transfer_done(host);
		return;
	}
	if (!data->error) {
		if ((data->flags & MMC_DATA_READ)
			&& (host->devicemode != MCI_DMAMODE)) {
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
	hoststatus &= ~(MCI_SDIOIT);
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

	while (hoststatus) {
		if ((hoststatus & MCI_XFER_IRQ_MASK))
			u8500_mmci_xfer_irq(host, hoststatus);
		if ((hoststatus & MCI_CMD_IRQ))
			u8500_mmci_cmd_irq(host, hoststatus);
		if ((hoststatus & MCI_DATA_IRQ))
			u8500_mmci_data_irq(host, hoststatus);
		if (hoststatus & MCI_SDIOIT) {
			/* Explicitly clear SDIOIT */
			writel(MCI_SDIOIT, (base + MMCICLEAR));
			 /* printk("\n START SDIO IRQ handler"); */
			if (host->mmc->card && host->mmc->card->sdio_func[0] &&
			    host->mmc->card->sdio_func[0]->irq_handler) {
				struct sdio_func *func = host->mmc->card->sdio_func[0];
				host->mmc->card->sdio_func[0]->irq_handler(func);
				/* printk("\n END SDIO IRQ handler"); */
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
 */
static void u8500_mmci_start_data(struct u8500_mmci_host *host,
					struct mmc_data *data,
						struct mmc_command *cmd)
{
	DECLARE_COMPLETION_ONSTACK(complete);
	void __iomem *base;
	u32 temp_reg;
	u32 polling_freq_clk_div = 0x0;
	unsigned long flag_lock = 0;
	unsigned int mem_addr;

	spin_lock_irqsave(&host->lock, flag_lock);
	host->data = data;
	host->cmd = cmd;
	host->size = data->blocks * data->blksz;
	host->data_xfered = 0;
	base = host->base;

	spin_unlock_irqrestore(&host->lock, flag_lock);

	u8500_mmci_init_sg(host, data);

	/**
	 * Required for SDIO DMA mode, dynamic switching between polling
	 * and DMA mode, depending on transfer size, and address alignment.
	 * For transfer size < 32 and for element size unaligned address
	 * SDIO host is configured in polling mode.
	 */
	if ((host->cmd->opcode == SD_IO_RW_EXTENDED) &&
		(sdio_mode == MCI_DMAMODE)) {
		mem_addr = (unsigned int)(page_address(sg_page(host->sg_ptr)) +
					  host->sg_ptr->offset) + host->sg_off;

		if ((mem_addr % DMA_ELEMENT_SZ) ||
			(host->size < MIN_DATA_SIZE_DMA))
			host->devicemode = MCI_POLLINGMODE;
		else
			host->devicemode = MCI_DMAMODE;

	}
#ifdef SDIO_MULTIBYTE_WORKAROUND
	/**
	 * Work around for multibyte mode, required for SDIO CMD 53
	 * size alignment required as host controller H/W does not
	 * support byte mode
	 */
       if (host->cmd && (host->cmd->opcode == SD_IO_RW_EXTENDED) && !(host->cmd->arg & 0x08000000)) {
		host->aligned_blksz = (1 << __fls(data->blksz));
		if (host->aligned_blksz < data->blksz) /* If the no. is not power of two, get next power of two */
			host->aligned_blksz = host->aligned_blksz << 1;
		if (host->aligned_blksz > 4096) /* Error handling, not sure if this will ever occur */
			host->aligned_blksz = data->blksz;
		host->cmd->arg &= ~(0x1FF); /* reset block size in command args to zero */
		if (host->aligned_blksz < 512)
			host->cmd->arg |= host->aligned_blksz;
		host->aligned_size = data->blocks * host->aligned_blksz;
		/* else blksz in arg will be zero. */
#if 0
		else { /* 512 MULTI BYET not working , so always send 512 as block mode */
			host->cmd->arg |= data->blocks;
			host->cmd->arg |= 0x08000000;
		}
#endif
	}
#endif /* End of SDIO_MULTIBYTE_WORKAROUND */
	cmd->error = MMC_ERR_NONE;
	data->error = MMC_ERR_NONE;

	if (host->devicemode == MCI_DMAMODE)
		u8500_mmc_set_dma(host);

	/**
	 * For a read we need to write to the data ctrl register first, then
	 * send the command. For writes, the order is reversed.
	 */
	if ((data->flags & MMC_DATA_READ))
		start_data_xfer(host);
	if (host->devicemode == MCI_DMAMODE)
		host->dma_done = &complete;
	u8500_mmc_send_cmd(base, cmd->opcode, cmd->arg, cmd->flags, host->devicemode);

	/**
	 * Required for SDIO interrupt from card , always enable SDIOIT ,
	 * even if MMC_CAP_SDIO_IRQ is not set
	 */
	if ((host->is_sdio == 1) /* && (host->mmc->caps & MMC_CAP_SDIO_IRQ) */) {
		/* Explicitly enable SDIOIT */
		if (host->sdio_setirq && !(host->sdio_irqstatus)) {
			/* If nterrupt is not already enabled then enable SDIOIT MASK */
			writel(MCI_SDIOIT, (base + MMCICLEAR));
			writel((readl(base + MMCIMASK0) | MCI_SDIOIT), (base + MMCIMASK0));
			host->sdio_irqstatus = 1;
		} else if (!(host->sdio_setirq) && (host->sdio_irqstatus)) {
			/* If SDIO is not already disabled then disable SDIOIT */
			u32 temp = readl(base + MMCIMASK0);
			temp &= ~(MCI_SDIOIT);
			writel(temp, (base + MMCIMASK0));
			host->sdio_irqstatus = 0;
		}
	}

	if (host->devicemode == MCI_POLLINGMODE) {
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

	host->cmd = cmd;
	u8500_mmc_send_cmd(base, cmd->opcode, cmd->arg, cmd->flags, host->devicemode);
	if (host->devicemode == MCI_POLLINGMODE) {
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
	if (host->mrq != NULL) {
		mrq->cmd->error = -EIO;
		if (mrq->data)
			mrq->data->bytes_xfered = 0;
		return;
	}
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

	/**
	 * Make sure we aren't changing the control registers too soon after
	 * writing data.
	 */
	udelay(10);
	/* Set the bus and power modes */
	switch (ios->power_mode) {
	case MMC_POWER_OFF:
		if (!host->level_shifter) {
			writel(0x40, host->base + MMCIPOWER);
		} else {
			writel(0xBC, host->base + MMCIPOWER);
		}

		if (host->board->set_power)
			host->board->set_power(mmc_dev(mmc), 0);
#ifdef CONFIG_REGULATOR
		/* we disable the supplies for the SD */
		if (host->level_shifter) {
			if (regulator_is_enabled(host->regulator))
				regulator_disable(host->regulator);
		}
#endif
	break;
	case MMC_POWER_UP:
		if (!host->level_shifter) {
			writel(0x42, host->base + MMCIPOWER);
		} else {
			writel(0xBE, host->base + MMCIPOWER);
		}

		if (host->board->set_power)
			host->board->set_power(mmc_dev(mmc), 1);
		if ((host->is_sdio == 1) && (!host->level_shifter))
			writel(0xBE, host->base + MMCIPOWER);
#ifdef CONFIG_REGULATOR
		if (host->level_shifter) {
			if (!regulator_is_enabled(host->regulator))
				regulator_enable(host->regulator);
		}
#endif
	break;
	case MMC_POWER_ON:
		if (!host->level_shifter)
			writel(0x43, host->base + MMCIPOWER);
		else
			writel(0xBF, host->base + MMCIPOWER);
		if ((host->is_sdio == 1) && (!host->level_shifter))
			writel(0x3, host->base + MMCIPOWER);
	break;
	}
	if (ios->clock) {
		if (ios->clock >= (host->mclk / 2))
			clk = 0;
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
		if ((host->devicemode == MCI_DMAMODE) && !host->is_sdio)
			clk |= MCI_CLK_PWRSAVE;
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

static void sdio_enableirq(struct mmc_host *mmc, int enable)
{
	struct u8500_mmci_host *host = mmc_priv(mmc);
	stm_info("\n sdio_enableirq= %d", enable);
	if (host && host->is_sdio)
		host->sdio_setirq = enable;
}

static void sdio_clearirq(struct mmc_host *mmc)
{
	struct u8500_mmci_host *host = mmc_priv(mmc);
	if (host)
		writel(MCI_SDIOIT, (host->base + MMCICLEAR));
}

static struct mmc_host_ops u8500_mmci_ops = {
	.request = u8500_mmci_request,
	.set_ios = u8500_mmci_set_ios,
	.enable_sdio_irq = sdio_enableirq,
};

/**
 *  u8500_mmci_check_status() - checks the status of the card on every interval
 *  @data: pointer to the current host
 *
 *  this function will check the status of the card on every interval
 *  of the time configured and inform the status to the mmc stack
 */
static void u8500_mmci_check_status(void *data)
{
	struct u8500_mmci_host *host = (struct u8500_mmci_host *) data;
	mdelay(50);
	mmc_detect_change(host->mmc, 30);
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
static int u8500_mmci_probe(struct amba_device *dev, struct amba_id *id)
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
	if (board->is_sdio)
	    devicemode = sdio_mode;
	else
	    devicemode = mmc_mode;

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
	host->mclk = MMC_HOST_CLK_MAX;
	host->mmc = mmc;
	host->board = board;
	host->dma_fifo_addr = board->dma_fifo_addr;
	host->dma_fifo_dev_type_rx = board->dma_fifo_dev_type_rx;
	host->dma_fifo_dev_type_tx = board->dma_fifo_dev_type_tx;
	if (board->level_shifter)
		host->level_shifter = board->level_shifter;
	/**
	 * For SDIO , as DMA to polling switching happens dynamically , always need to allocate buffer
	 */
	if (board->is_sdio || (devicemode == MCI_POLLINGMODE)) {
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
	mmc->f_min = MMC_HOST_CLK_MAX / (MMC_CLK_DIV + 2);
	if (devicemode == MCI_DMAMODE)
		mmc->caps = host->caps;

	host->is_sdio = board->is_sdio;
	if (host->is_sdio) {
		if (host->caps & MMC_CAP_SDIO_IRQ)
		    host->sdio_setirq = 0;
		else
		    host->sdio_setirq = 1;
		    /* Make IRQ enable even when SDIO_IRQ is not supported in capability */

		host->sdio_irqstatus = 0;
		mmc->caps = host->caps;
	}

	host->devicemode = devicemode;
	/*
	 * We can do SGIO
	 */
	if (devicemode == MCI_DMAMODE) {
		/* Can't do scatter/gather DMA */
		mmc->max_hw_segs = 1;
		mmc->max_phys_segs = 1;
	} else {
		mmc->max_hw_segs = 1;
		mmc->max_phys_segs = 1;
	}
	mmc->ocr_avail = OCR_AVAIL;
	/**
	 * XXX No errors in Tx/Rx but writes garbage data with mmc->max_req_size = 32768;
	 */
#ifdef DMA_SCATERGATHER
	mmc->max_req_size = 65535;
#else
	mmc->max_req_size = 4096;
#endif
	mmc->max_seg_size = mmc->max_req_size;

	mmc->max_blk_size = 4096;
	if (devicemode == MCI_DMAMODE)
		mmc->max_blk_count = mmc->max_req_size;
	else
		mmc->max_blk_count = 64;
	spin_lock_init(&host->lock);

#ifdef CONFIG_REGULATOR
	/* attach regulator for on board emmc */
	if (board->supply) {
		host->regulator = regulator_get(&dev->dev, board->supply);
		if (IS_ERR(host->regulator)) {
			ret = PTR_ERR(host->regulator);
			goto host_free;
		}
		regulator_set_voltage(host->regulator, board->supply_voltage,
						board->supply_voltage);
		regulator_enable(host->regulator);
	}
#endif
	host->clk = clk_get(&dev->dev, NULL);
	if (IS_ERR(host->clk)) {
		ret = PTR_ERR(host->clk);
#ifdef CONFIG_REGULATOR
		goto put_regulator;
#else
		goto unmap;
#endif
	}

	clk_enable(host->clk);
	writel(0, host->base + MMCIMASK0);
	writel(0xfff, host->base + MMCICLEAR);
	if ((devicemode != MCI_POLLINGMODE) || (host->is_sdio == 1)) {
	    /* interrupt should always be enabled for I/O cards */
		ret = request_irq(dev->irq[0], u8500_mmci_irq,
			    IRQF_DISABLED/* SA_INTERRUPT */, DRIVER_NAME, host);
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
	/*
	 * SDIO host structure is saved into global pointer
	 * Used for sysfs implementation for dynamic detection of I/O cards
	 * Also create sysfs file for detect_card
	 */
	if (host->is_sdio) {
		ret = device_create_file(&dev->dev, &dev_attr_detect_card);
		if (ret < 0) {
			stm_error("\nCould not create sysfs file"
				  "for SDIO detect_card");
			goto mem2mmc_dmareq_failed;
		}
		sdio_host_ptr = host;
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
#ifdef CONFIG_REGULATOR
put_regulator:
	if (host->regulator) {
		if (regulator_is_enabled(host->regulator))
			regulator_disable(host->regulator);
		regulator_put(host->regulator);
	}
#endif
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

		/*
		 * global pointer for storing SDIO host structure is cleared
		 * Used for sysfs implementation for dynamic detection of
		 * I/O cards. Also remove sysfs file for detect_card
		 */
		if (host->is_sdio) {
			sdio_host_ptr = NULL;
			device_remove_file(&dev->dev, &dev_attr_detect_card);
		}
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
#ifdef CONFIG_REGULATOR
		if (host->regulator) {
			if (regulator_is_enabled(host->regulator))
				regulator_disable(host->regulator);
			regulator_put(host->regulator);
		}
#endif
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
	if (mmc) {
		struct u8500_mmci_host *host = mmc_priv(mmc);
		if (host->level_shifter) {
			ret = mmc_suspend_host(mmc, state);
			if (ret == 0)
				writel(0, host->base + MMCIMASK0);
		} else {
			host->reg_context[0] = readl(host->base + MMCICLOCK);
			host->reg_context[1] = readl(host->base + MMCIPOWER);
			writel(0, host->base + MMCIMASK0);
		}
		clk_disable(host->clk);
#ifdef CONFIG_REGULATOR
		if (host->board->supply) {
			if (!host->level_shifter) {
				if (regulator_is_enabled(host->regulator))
					regulator_disable(host->regulator);
			}
		}
#endif
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
	if (mmc) {
		struct u8500_mmci_host *host = mmc_priv(mmc);
#ifdef CONFIG_REGULATOR
		if (host->board->supply) {
			if (!host->level_shifter) {
				if (!regulator_is_enabled(host->regulator))
					regulator_enable(host->regulator);
			}
		}
#endif
		clk_enable(host->clk);
		if (host->level_shifter) {
			writel(MCI_IRQENABLE, host->base + MMCIMASK0);
			ret = mmc_resume_host(mmc);
		} else {
			writel(host->reg_context[1], host->base + MMCIPOWER);
			writel(host->reg_context[0], host->base + MMCICLOCK);
			writel(MCI_IRQENABLE, host->base + MMCIMASK0);
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
