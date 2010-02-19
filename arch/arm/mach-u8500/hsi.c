/*
 * Copyright (C) 2010 ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/io.h>

#include <linux/hsi.h>
#include <linux/hsi-legacy.h>

#include <mach/hsi-stm.h>
#include <mach/devices.h>
#include <mach/hardware.h>

static struct hsi_plat_data hsit_platform_data = {
	.dev_type = 0x0 /** transmitter */ ,
	.mode = 0x2 /** frame mode */ ,
	.divisor = 0x12 /** half HSIT freq */ ,
	.parity = 0x0 /** no parity */ ,
	.channels = 0x4 /** 4 channels */ ,
	.flushbits = 0x0 /** none */ ,
	.priority = 0x3 /** ch0,ch1 high while ch2,ch3 low */ ,
	.burstlen = 0x0 /** infinite b2b */ ,
	.preamble = 0x0 /** none */ ,
	.dataswap = 0x0 /** no swap */ ,
	.framelen = 0x1f /** 32 bits all channels */ ,
	.ch_base_span = {[0] = {.base = 0x0, .span = 0x3},
			 [1] = {.base = 0x4, .span = 0x3},
			 [2] = {.base = 0x8, .span = 0x7},
			 [3] = {.base = 0x10, .span = 0x7}
			 },
#ifdef CONFIG_STN8500_HSI_LEGACY
	.currmode = CONFIG_STN8500_HSI_TRANSFER_MODE,
#else
	.currmode = 1,
#endif
	.hsi_dma_info = {
			 [0] = {
				.reserve_channel = 0,
				.dir = MEM_TO_PERIPH,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.dst_dev_type = DMA_DEV_SLIM0_CH0_TX_HSI_TX_CH0,
				.src_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 [1] = {
				.reserve_channel = 0,
				.dir = MEM_TO_PERIPH,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.dst_dev_type = DMA_DEV_SLIM0_CH1_TX_HSI_TX_CH1,
				.src_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 [2] = {
				.reserve_channel = 0,
				.dir = MEM_TO_PERIPH,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.dst_dev_type = DMA_DEV_SLIM0_CH2_TX_HSI_TX_CH2,
				.src_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 [3] = {
				.reserve_channel = 0,
				.dir = MEM_TO_PERIPH,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.dst_dev_type = DMA_DEV_SLIM0_CH3_TX_HSI_TX_CH3,
				.src_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 },
	.watermark = 0x2 /** 1 free entries for all channels */ ,
	.gpio_alt_func = GPIO_ALT_HSIT,
};

static struct hsi_plat_data hsir_platform_data = {
	.dev_type = 0x1 /** receiver */ ,
	.mode = 0x3 /** pipelined */ ,
	.threshold = 0x22 /** 35 zero bits for break */ ,
	.parity = 0x0 /** no parity */ ,
	.detector = 0x0 /** oversampling */ ,
	.channels = 0x4 /** 4 channels */ ,
	.realtime = 0x0 /** disabled,no overwrite,all channels */ ,
	.framelen = 0x1f /** 32 bits all channels */ ,
	.preamble = 0x0 /** max timeout cycles */ ,
	.ch_base_span = {[0] = {.base = 0x0, .span = 0x3},
			 [1] = {.base = 0x4, .span = 0x3},
			 [2] = {.base = 0x8, .span = 0x7},
			 [3] = {.base = 0x10, .span = 0x7}
			 },
#ifdef CONFIG_STN8500_HSI_LEGACY
	.currmode = CONFIG_STN8500_HSI_TRANSFER_MODE,
#else
	.currmode = 1,
#endif
	.timeout = 0x0 /** immediate updation of channel buffer */ ,
	.hsi_dma_info = {
			 [0] = {
				.reserve_channel = 0,
				.dir = PERIPH_TO_MEM,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.src_dev_type = DMA_DEV_SLIM0_CH0_RX_HSI_RX_CH0,
				.dst_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 [1] = {
				.reserve_channel = 0,
				.dir = PERIPH_TO_MEM,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.src_dev_type = DMA_DEV_SLIM0_CH1_RX_HSI_RX_CH1,
				.dst_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 [2] = {
				.reserve_channel = 0,
				.dir = PERIPH_TO_MEM,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.src_dev_type = DMA_DEV_SLIM0_CH2_RX_HSI_RX_CH2,
				.dst_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 [3] = {
				.reserve_channel = 0,
				.dir = PERIPH_TO_MEM,
				.flow_cntlr = DMA_IS_FLOW_CNTLR,
				.channel_type =
				(STANDARD_CHANNEL | CHANNEL_IN_LOGICAL_MODE
				 | LCHAN_SRC_LOG_DEST_LOG | NO_TIM_FOR_LINK
				 | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL),
				.src_dev_type = DMA_DEV_SLIM0_CH3_RX_HSI_RX_CH3,
				.dst_dev_type = DMA_DEV_DEST_MEMORY,
				.src_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     },
				.dst_info = {
					     .endianess = DMA_LITTLE_ENDIAN,
					     .data_width = DMA_WORD_WIDTH,
					     .burst_size = DMA_BURST_SIZE_4,
					     .buffer_type = SINGLE_BUFFERED,
					     }
				},
			 },
	.watermark = 0x2 /** 1 'occupated' entry for all channels */ ,
	.gpio_alt_func = GPIO_ALT_HSIR,
};

static struct resource u8500_hsit_resource[] = {
	[0] = {
		.start = U8500_HSIT_BASE,
		.end = U8500_HSIT_BASE + (SZ_4K - 1),
		.name = "hsit_iomem_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_HSITD0,
		.end = IRQ_HSITD1,
		.name = "hsit_irq",
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource u8500_hsir_resource[] = {
	[0] = {
		.start = U8500_HSIR_BASE,
		.end = U8500_HSIR_BASE + (SZ_4K - 1),
		.name = "hsir_iomem_base",
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_HSIRD0,
		.end = IRQ_HSIRD1,
		.name = "hsir_irq",
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_HSIR_EXCEP,
		.end = IRQ_HSIR_EXCEP,
		.name = "hsir_irq_excep",
		.flags = IORESOURCE_IRQ,
	},
	[3] = {
		.start = IRQ_HSIR_CH0_OVRRUN,
		.end = IRQ_HSIR_CH0_OVRRUN,
		.name = "hsir_irq_ch0_overrun",
		.flags = IORESOURCE_IRQ,
	},
	[4] = {
		.start = IRQ_HSIR_CH1_OVRRUN,
		.end = IRQ_HSIR_CH1_OVRRUN,
		.name = "hsir_irq_ch1_overrun",
		.flags = IORESOURCE_IRQ,
	},
	[5] = {
		.start = IRQ_HSIR_CH2_OVRRUN,
		.end = IRQ_HSIR_CH2_OVRRUN,
		.name = "hsir_irq_ch2_overrun",
		.flags = IORESOURCE_IRQ,
	},
	[6] = {
		.start = IRQ_HSIR_CH3_OVRRUN,
		.end = IRQ_HSIR_CH3_OVRRUN,
		.name = "hsir_irq_ch3_overrun",
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device u8500_hsit_device = {
	.name = "stm-hsi",
	.id = 0,
	.dev = {
		.platform_data = &hsit_platform_data,
		},
	.num_resources = ARRAY_SIZE(u8500_hsit_resource),
	.resource = u8500_hsit_resource,
};

struct platform_device u8500_hsir_device = {
	.name = "stm-hsi",
	.id = 1,
	.dev = {
		.platform_data = &hsir_platform_data,
		},
	.num_resources = ARRAY_SIZE(u8500_hsir_resource),
	.resource = u8500_hsir_resource,
};
