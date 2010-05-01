/*
 * Copyright 2009 ST-Ericsson.
 * Copyright 2009 STMicroelectronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */
#ifndef __INC_ASM_ARCH_DMA_H
#define __INC_ASM_ARCH_DMA_H

#define MAX_DMA_CHANNELS 32
#ifndef __ASSEMBLY__
#include <asm/scatterlist.h>

typedef unsigned int dmach_t;
typedef unsigned int dmamode_t;

enum dma_flow_controller {
	DMA_IS_FLOW_CNTLR,
	PERIPH_IS_FLOW_CNTLR
};
enum {
	DMA_FALSE,
	DMA_TRUE
};

enum dma_xfer_dir {
	MEM_TO_MEM,
	MEM_TO_PERIPH,
	PERIPH_TO_MEM,
	PERIPH_TO_PERIPH
};

enum dma_endianess {
	DMA_LITTLE_ENDIAN,
	DMA_BIG_ENDIAN
};
enum dma_event {
	XFER_COMPLETE,
	XFER_ERROR
};
typedef void (*dma_callback_t)(void *data, enum dma_event event);

#include <mach/dma_40-8500.h>

/**
 * struct stm_dma_pipe_info - Structure to be filled by client drivers.
 *
 * @reserve_channel: Whether you want to reserve the channel
 * @dir:	MEM 2 MEM, PERIPH 2 MEM , MEM 2 PERIPH, PERIPH 2 PERIPH
 * @flow_cntlr: who is flow controller (Device or DMA)
 * @phys_chan_id: physical channel ID on which this channel will execute
 * @data:	data of callback handler
 * @callback:	callback handler registered by client
 * @channel_type: std/ext, basic/log/operational, priority, security
 * @src_dev_type: Src device type
 * @dst_dev_type: Dest device type
 * @src_addr: Source address
 * @dst_addr: Dest address
 * @src_info: Parameters for Source half channel
 * @dst_info: Parameters for Dest half channel
 *
 *
 * This structure has to be filled by the client drivers, before requesting
 * a DMA pipe. This information is used by the Driver to allocate or
 * reserve an appropriate DMA channel for this client.
 *
 */
struct stm_dma_pipe_info {
	unsigned int reserve_channel;
	enum dma_xfer_dir dir;
	enum dma_flow_controller flow_cntlr;
	enum dma_chan_id phys_chan_id;
	void *data;
	dma_callback_t callback;
	unsigned int channel_type;
	enum dma_src_dev_type src_dev_type;
	enum dma_dest_dev_type dst_dev_type;
	void *src_addr;
	void *dst_addr;
	struct dma_half_channel_info  src_info;
	struct dma_half_channel_info  dst_info;
};

extern int stm_configure_dma_channel(int channel,
		struct stm_dma_pipe_info *info);
extern int stm_request_dma(int  *channel, struct stm_dma_pipe_info *info);
extern void stm_free_dma(int channel);
extern int stm_set_callback_handler(int channel, void *callback_handler,
		void *data);
extern int stm_enable_dma(int channel);
extern void stm_disable_dma(int channel);
extern int stm_pause_dma(int channel);
extern void stm_unpause_dma(int channel);
extern void stm_set_dma_addr(int channel, void *src_addr , void *dst_addr);
extern void stm_set_dma_count(int channel, int count);
extern void stm_set_dma_sg(int channel, struct scatterlist *sg,
		int nr_sg, int type);

extern int stm_dma_residue(int channel);

#endif 	/*__ASSEMBLY__*/
#endif	/* __INC_ASM_ARCH_DMA_H */
/* End of file - dma.h */

