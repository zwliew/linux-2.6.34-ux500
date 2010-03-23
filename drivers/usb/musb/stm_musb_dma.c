/* Copyright (C) 2009 ST Ericsson
 * Copyright (C) 2009 STMicroelectronics
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include "musb_core.h"
#include <mach/dma.h>
#include "ste_config.h"

#define MUSB_HSDMA_CHANNELS	 16


void musb_rx_dma_controller_handler(void *private_data, int error);
void musb_tx_dma_controller_handler(void *private_data, int error);
struct musb_dma_controller;

struct musb_dma_channel {
	struct dma_channel              channel;
	struct musb_dma_controller      *controller;
	struct  stm_dma_pipe_info   *info;
	struct musb_hw_ep       *hw_ep;
	u32                             start_addr;
	u32                             len;
	u32                             is_pipe_allocated;
	u16                             max_packet_sz;
	u8                              idx;
	unsigned int                            id;
	unsigned int cur_len;
	u8                              epnum;
	u8                              last_xfer;
	u8                              transmit;
};


struct musb_dma_controller {
	struct dma_controller		controller;
	struct musb_dma_channel		channel[MUSB_HSDMA_CHANNELS];
	void				*private_data;
	void __iomem			*base;
	u8				channel_count;
	u8				used_channels;
	u8				irq;
};

/**
 * dma_controller_start() - creates the logical channels pool and registers callbacks
 * @c:	pointer to DMA Controller
 *
 * This function requests the logical channels from the DMA driver and creates
 * logical channels based on event lines and also registers the callbacks which
 * are invoked after data transfer in the transmit or receive direction.
*/

static int dma_controller_start(struct dma_controller *c)
{
	struct musb_dma_controller *controller = container_of(c,
			struct musb_dma_controller, controller);
	struct musb_dma_channel *musb_channel = NULL;
	struct stm_dma_pipe_info *info;
	u8 bit;
	int retval = 0;
	struct dma_channel *channel = NULL;
	/*bit 0 for receive and bit 1 for transmit*/
#ifndef CONFIG_USB_U8500_DMA
	for (bit = 0; bit < 2; bit++) {
#else
	for (bit = 0; bit < (U8500_DMA_END_POINTS*2); bit++) {
#endif
		musb_channel = &(controller->channel[bit]);
		info = (struct stm_dma_pipe_info *)kzalloc(sizeof(struct stm_dma_pipe_info), GFP_KERNEL);
		if (!info)
			ERR("could not allocate dma info structure\n");
		musb_channel->info = info;
		musb_channel->controller = controller;
#ifdef CONFIG_USB_U8500_DMA
		info->channel_type = (STANDARD_CHANNEL|CHANNEL_IN_LOGICAL_MODE|
					LCHAN_SRC_LOG_DEST_LOG|NO_TIM_FOR_LINK|
					NON_SECURE_CHANNEL|HIGH_PRIORITY_CHANNEL);
#else
		info->channel_type = (STANDARD_CHANNEL | CHANNEL_IN_PHYSICAL_MODE | NON_SECURE_CHANNEL | HIGH_PRIORITY_CHANNEL | PCHAN_BASIC_MODE);
#endif

		info->flow_cntlr = DMA_IS_FLOW_CNTLR;
#if 1
#ifndef CONFIG_USB_U8500_DMA
		if (bit)
			{
#else
		if ((bit >= TX_CHANNEL_1) && (bit <= TX_CHANNEL_7)) {
#endif
			info->dir = MEM_TO_PERIPH;
			info->src_dev_type = DMA_DEV_SRC_MEMORY;
#ifdef CONFIG_NOMADIK_NDK15
			info->dst_dev_type = DMA_DEV_USB_OTG_0_TX;
#endif

#ifdef CONFIG_NOMADIK_NDK20
			info->dst_dev_type = DMA_DEV_USB_OTG_OEP_2_10;
#endif

#ifdef CONFIG_USB_U8500_DMA
			switch (bit) {

			case TX_CHANNEL_1:
				info->dst_dev_type = DMA_DEV_USB_OTG_OEP_1_9;
				break;
			case TX_CHANNEL_2:
				info->dst_dev_type = DMA_DEV_USB_OTG_OEP_2_10;
				break;
			case TX_CHANNEL_3:
				info->dst_dev_type = DMA_DEV_USB_OTG_OEP_3_11 ;
				break;
			case TX_CHANNEL_4:
				info->dst_dev_type = DMA_DEV_USB_OTG_OEP_4_12;
				break;
			case TX_CHANNEL_5:
				info->dst_dev_type = DMA_DEV_USB_OTG_OEP_5_13;
				break;
			case TX_CHANNEL_6:
				info->dst_dev_type = DMA_DEV_USB_OTG_OEP_6_14;
				break;
			case TX_CHANNEL_7:
				info->dst_dev_type = DMA_DEV_USB_OTG_OEP_7_15;
				break;

			}

#endif

		} else {
			info->dir = PERIPH_TO_MEM;
#ifdef CONFIG_NOMADIK_NDK15
			info->src_dev_type = DMA_DEV_USB_OTG_1_RX;
#endif
#ifdef CONFIG_NOMADIK_NDK20
			info->src_dev_type = DMA_DEV_USB_OTG_IEP_1_9;
#endif

#ifdef CONFIG_USB_U8500_DMA
			switch (bit) {

			case RX_CHANNEL_1:
				info->src_dev_type = DMA_DEV_USB_OTG_IEP_1_9;
				break;
			case RX_CHANNEL_2:
				info->src_dev_type = DMA_DEV_USB_OTG_IEP_2_10;
				break;
			case RX_CHANNEL_3:
				info->src_dev_type = DMA_DEV_USB_OTG_IEP_3_11;
				break;
			case RX_CHANNEL_4:
				info->src_dev_type = DMA_DEV_USB_OTG_IEP_4_12;
				break;
			case RX_CHANNEL_5:
				info->src_dev_type = DMA_DEV_USB_OTG_IEP_5_13;
				break;
			case RX_CHANNEL_6:
				info->src_dev_type = DMA_DEV_USB_OTG_IEP_6_14;
				break;
			case RX_CHANNEL_7:
				info->src_dev_type =  DMA_DEV_USB_OTG_IEP_7_15;
				break;
			default:
				break;
			}
#endif
			info->dst_dev_type = DMA_DEV_DEST_MEMORY;
		}
		info->src_info.endianess = DMA_LITTLE_ENDIAN;
		info->src_info.buffer_type = SINGLE_BUFFERED;
		info->src_info.data_width = DMA_WORD_WIDTH;
		info->src_info.burst_size = DMA_BURST_SIZE_16;

		info->dst_info.endianess = DMA_LITTLE_ENDIAN;
		info->dst_info.buffer_type = SINGLE_BUFFERED;
		info->dst_info.data_width = DMA_WORD_WIDTH ;
		info->dst_info.burst_size = DMA_BURST_SIZE_16;
#endif
		musb_channel->is_pipe_allocated = 1;
		channel = &(musb_channel->channel);
		channel->private_data = musb_channel;
		channel->status = MUSB_DMA_STATUS_FREE;
		channel->max_len = 0x10000;
		/* Tx => mode 1; Rx => mode 0 */
		channel->desired_mode = bit;
		channel->actual_len = 0;
		retval = stm_request_dma(&musb_channel->id , info);
		if (retval < 0) {
			ERR("dma pipe can't be allocated\n");
		}
#ifndef CONFIG_USB_U8500_DMA
		/* Tx => mode 1; Rx => mode 0 */
		if (bit) {
#else
		if ((bit >= TX_CHANNEL_1) && (bit <= TX_CHANNEL_7)) {
#endif
			stm_set_callback_handler(musb_channel->id, &musb_tx_dma_controller_handler, (void *)channel);
			DBG(2, "channel allocted for TX, id %d\n", musb_channel->id);
		} else{
			stm_set_callback_handler(musb_channel->id, &musb_rx_dma_controller_handler, (void *)channel);
			DBG(2, "channel allocted for RX, id %d\n", musb_channel->id);
		}

	}
	return 0;
}

static void dma_channel_release(struct dma_channel *channel);

/**
 * dma_controller_stop() - releases all the channels and frees the DMA pipes
 * @c: pointer to DMA controller
 *
 * This function frees all of the logical channels and frees the DMA pipes
*/

static int dma_controller_stop(struct dma_controller *c)
{
	struct musb_dma_controller *controller = container_of(c,
			struct musb_dma_controller, controller);
	struct musb *musb = controller->private_data;
	struct musb_dma_channel *musb_channel;
	struct dma_channel *channel;
	u8 bit;
#ifndef CONFIG_USB_U8500_DMA
	for (bit = 0; bit < 2; bit++) {
#else
	for (bit = 0; bit < (U8500_DMA_END_POINTS*2); bit++) {
#endif
		channel = &controller->channel[bit].channel;
		musb_channel = channel->private_data;
		dma_channel_release(channel);
		if (musb_channel->info)
		{
			stm_free_dma(musb_channel->id);
			kfree(musb_channel->info);
			musb_channel->info = NULL;
		}
	}

	return 0;
}

/**
 * dma_controller_allocate() - allocates the DMA channels
 * @c: pointer to DMA controller
 * @hw_ep: pointer to endpoint
 * @transmit: transmit or receive direction
 *
 * This function allocates the DMA channel and initializes
 * the channel
*/

static struct dma_channel *dma_channel_allocate(struct dma_controller *c,
				struct musb_hw_ep *hw_ep, u8 transmit)
{
	struct musb_dma_controller *controller = container_of(c,
			struct musb_dma_controller, controller);
	struct musb_dma_channel *musb_channel = NULL;
	struct dma_channel *channel = NULL;
	u8 bit;

#ifndef CONFIG_USB_U8500_DMA

	 /*bit 0 for receive and bit 1 for transmit*/
	for (bit = 0; bit < 2; bit++) {
		if (!(controller->used_channels & (1 << bit))) {

			if ((transmit && !bit))
			{
				continue;
			}
			if ((!transmit && bit))
			{
				break;
			}
#else
			if (hw_ep->epnum > 0 && hw_ep->epnum <= U8500_DMA_END_POINTS) {
				if (transmit)
					bit = hw_ep->epnum - 1;
				else
					bit = hw_ep->epnum + RX_END_POINT_OFFSET;
			} else
				return NULL;
#endif
			controller->used_channels |= (1 << bit);
			musb_channel = &(controller->channel[bit]);
			musb_channel->idx = bit;
			musb_channel->epnum = hw_ep->epnum;
			musb_channel->hw_ep = hw_ep;
			musb_channel->transmit = transmit;
			musb_channel->is_pipe_allocated = 1;
			channel = &(musb_channel->channel);
#ifndef CONFIG_USB_U8500_DMA
			break;
		}
	}
#endif
	return channel;
}

/**
 * dma_channel_release() - releases the DMA channel
 * @channel:	channel to be released
 *
 * This function releases the DMA channel
 *
*/

static void dma_channel_release(struct dma_channel *channel)
{
	struct musb_dma_channel *musb_channel = channel->private_data;
	channel->actual_len = 0;
	musb_channel->start_addr = 0;
	musb_channel->len = 0;

	DBG(2, "enter\n");
	musb_channel->controller->used_channels &=
		~(1 << musb_channel->idx);

	channel->status = MUSB_DMA_STATUS_FREE;
	if (musb_channel->is_pipe_allocated)
		musb_channel->is_pipe_allocated = 0;
	DBG(2, "exit\n");
}
#ifdef CONFIG_NOMADIK_NDK20
extern void musb_memcpy(void *dest, void *source, unsigned int len);
#endif

/**
 * configure_channel() - configures the source, destination addresses and
 *                       starts the transfer
 * @channel:    pointer to DMA channel
 * @packet_sz:  packet size
 * @mode: Dma mode
 * @dma_addr: DMA source address for transmit direction
 *            or DMA destination address for receive direction
 * @len: length
 * This function configures the source and destination addresses for DMA
 * operation and initiates the DMA transfer
*/

static void configure_channel(struct dma_channel *channel,
				u16 packet_sz, u8 mode,
				dma_addr_t dma_addr, u32 len)
{
	struct musb_dma_channel *musb_channel = channel->private_data;
	struct musb_hw_ep       *hw_ep = musb_channel->hw_ep;
	struct musb *musb = hw_ep->musb;
	void __iomem *mbase = musb->mregs;
	u32 dma_count;
	struct stm_dma_pipe_info *info;

#ifndef CONFIG_USB_U8500_DMA
	struct musb_qh          *qh;
	struct urb              *urb;
#endif
	unsigned int usb_fifo_addr = (unsigned int)(MUSB_FIFO_OFFSET(hw_ep->epnum) + mbase);

#ifndef CONFIG_USB_U8500_DMA
	if (musb_channel->transmit)
		qh = hw_ep->out_qh;
	else
		qh = hw_ep->in_qh;
	urb = next_urb(qh);
#endif
	info = musb_channel->info;
#ifdef CONFIG_NOMADIK_NDK15

	/* For High speed devices*/
	if (urb->dev->speed == USB_SPEED_HIGH)
	{
		info->src_info.burst_size = DMA_BURST_SIZE_128;
		info->dst_info.burst_size = DMA_BURST_SIZE_128;
	} else
	{
		info->src_info.burst_size = DMA_BURST_SIZE_16;
		info->dst_info.burst_size = DMA_BURST_SIZE_16;
	}
	stm_configure_dma_channel(musb_channel->id, info);
#endif

	dma_count = len - (len % packet_sz);
	musb_channel->cur_len = dma_count;
	usb_fifo_addr = U8500_USBOTG_BASE + ((unsigned int)usb_fifo_addr & 0xFFFF);

#ifndef CONFIG_USB_U8500_DMA
	if (!(urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP)) {
		if (musb_channel->transmit)
		{
			dma_addr = musb->tx_dma_phy;

#ifdef CONFIG_NOMADIK_NDK15
			memcpy((void *)musb->tx_dma_log, urb->transfer_buffer, dma_count);
#endif
#ifdef CONFIG_NOMADIK_NDK20
			musb_memcpy((void *)musb->tx_dma_log, urb->transfer_buffer, dma_count);
#endif
		}
		else
		{
			dma_addr = musb->rx_dma_phy;
		}
	}
#endif

	if (musb_channel->transmit)
		stm_set_dma_addr(musb_channel->id, (void *)(dma_addr), (void *)(usb_fifo_addr));
	else
		stm_set_dma_addr(musb_channel->id, (void *)(usb_fifo_addr), (void *)(dma_addr));

	stm_set_dma_count(musb_channel->id , dma_count);

#ifdef CONFIG_NOMADIK_NDK15
	if (musb_channel->transmit)
	{
		uint32_t dma_sel = 0;

/* enabling EP2 tx on usb dma channel 0 */
		dma_sel = musb_readw(musb->mregs, MUSB_O_HDRC_DMASEL);
		dma_sel &= ~MUSB_TX_DMA_MASK;
		dma_sel |= MUSB_EP_TX_DMASEL_VAL(2);
		DBG(2, "dma_sel val for RX = %x\n", dma_sel);
		musb_writew(musb->mregs, MUSB_O_HDRC_DMASEL, dma_sel);
		DBG(2, "DMASEL reg = %x\n", musb_readw(musb->mregs, MUSB_O_HDRC_DMASEL));
	}
	else
	{
		uint32_t dma_sel = 0;

		/* enabling EP1 rx on usb dma channel 1 */
		dma_sel = musb_readl(musb->mregs, MUSB_O_HDRC_DMASEL);
		dma_sel &= ~MUSB_RX_DMA_MASK;
		dma_sel |= MUSB_EP_RX_DMASEL_VAL(1);
		DBG(2, "dma_sel val for RX = %x\n", dma_sel);
		musb_writel(musb->mregs, MUSB_O_HDRC_DMASEL, dma_sel);
		DBG(2, "DMASEL reg = %x\n", musb_readl(musb->mregs, MUSB_O_HDRC_DMASEL));
	}
#endif

	//channel->last_xfer = 0;
	stm_enable_dma(musb_channel->id);
}

/**
 * dma_channel_program() - Configures the channel and initiates transfer
 * @channel:	pointer to DMA channel
 * @packet_sz:	packet size
 * @mode: mode
 * @dma_addr: physical address of memory
 * @len: length
 *
 * This function configures the channel and initiates the DMA transfer
*/

static int dma_channel_program(struct dma_channel *channel,
				u16 packet_sz, u8 mode,
				dma_addr_t dma_addr, u32 len)
{
	struct musb_dma_channel *musb_channel = channel->private_data;
	BUG_ON(channel->status == MUSB_DMA_STATUS_UNKNOWN ||
		channel->status == MUSB_DMA_STATUS_BUSY);
	if (len < packet_sz)
		return false;
	if (!musb_channel->transmit && len < packet_sz)
	{
		return false;
	}
	channel->actual_len = 0;
	musb_channel->start_addr = dma_addr;
	musb_channel->len = len;
	musb_channel->max_packet_sz = packet_sz;
	channel->status = MUSB_DMA_STATUS_BUSY;


	if ((mode == 1) && (len >= packet_sz))
		configure_channel(channel, packet_sz, 1, dma_addr, len);
	else
		configure_channel(channel, packet_sz, 0, dma_addr, len);
	return true;
}

/**
 * dma_channel_abort() - aborts the DMA transfer
 * @channel:	pointer to DMA channel.
 *
 * This function aborts the DMA transfer.
*/

static int dma_channel_abort(struct dma_channel *channel)
{
	struct musb_dma_channel *musb_channel = channel->private_data;
	void __iomem *mbase = musb_channel->controller->base;
	u16 csr;
	if (channel->status == MUSB_DMA_STATUS_BUSY) {
		if (musb_channel->transmit) {

			csr = musb_readw(mbase,
				MUSB_EP_OFFSET(musb_channel->epnum,
						MUSB_TXCSR));
			csr &= ~(MUSB_TXCSR_AUTOSET |
				 MUSB_TXCSR_DMAENAB |
				 MUSB_TXCSR_DMAMODE);
			musb_writew(mbase,
				MUSB_EP_OFFSET(musb_channel->epnum, MUSB_TXCSR),
				csr);
		} else {
			csr = musb_readw(mbase,
				MUSB_EP_OFFSET(musb_channel->epnum,
						MUSB_RXCSR));
			csr &= ~(MUSB_RXCSR_AUTOCLEAR |
				 MUSB_RXCSR_DMAENAB |
				 MUSB_RXCSR_DMAMODE);
			musb_writew(mbase,
				MUSB_EP_OFFSET(musb_channel->epnum, MUSB_RXCSR),
				csr);
		}

		if (musb_channel->is_pipe_allocated)
		{
			stm_disable_dma(musb_channel->id);
			channel->status = MUSB_DMA_STATUS_FREE;
		}
	}
	return 0;
}

/**
 * musb_rx_dma_controller_handler() - callback invoked when the data is received in the receive direction
 * @private_data: DMA channel
 * @error: error code
 *
 * This callback is invoked when the DMA transfer is completed in the receive direction.
*/

void musb_rx_dma_controller_handler(void *private_data, int error)
{
	struct dma_channel      *channel = (struct dma_channel *)private_data;
	struct musb_dma_channel *musb_channel = channel->private_data;
	struct musb_hw_ep       *hw_ep = musb_channel->hw_ep;
	struct musb *musb = hw_ep->musb;
	void __iomem *mbase = musb->mregs;
	unsigned long flags, pio;
	unsigned int rxcsr;
#ifndef CONFIG_USB_U8500_DMA
	struct musb_qh          *qh = hw_ep->in_qh;
	struct urb              *urb;
#endif
	spin_lock_irqsave(&musb->lock, flags);
#ifndef CONFIG_USB_U8500_DMA
	urb = next_urb(qh);
#endif
	musb_ep_select(mbase, hw_ep->epnum);
	channel->actual_len = musb_channel->cur_len;
	pio = musb_channel->len - channel->actual_len;
#ifndef CONFIG_USB_U8500_DMA
	if (!(urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP))
	{
#ifdef CONFIG_NOMADIK_NDK15
			memcpy(urb->transfer_buffer, (void *)musb->rx_dma_log, channel->actual_len);
#endif
#ifdef CONFIG_NOMADIK_NDK20
			musb_memcpy(urb->transfer_buffer, (void *)musb->rx_dma_log, channel->actual_len);
#endif
	}
#endif
	if (!pio)
	{
		channel->status = MUSB_DMA_STATUS_FREE;
		musb_dma_completion(musb, musb_channel->epnum,
			musb_channel->transmit);
	}
	/*else
	{
		channel->last_xfer = 1;
	}*/
	spin_unlock_irqrestore(&musb->lock, flags);
}

/**
 * musb_tx_dma_controller_handler() - callback invoked on the transmit direction DMA data transfer
 * @private_data:	pointer to DMA channel.
 * @error:	error code.
 *
 * This callback is invoked when the DMA tranfer is completed in the transmit direction
*/

void musb_tx_dma_controller_handler(void *private_data, int error)
{
	struct dma_channel      *channel = (struct dma_channel *)private_data;
	struct musb_dma_channel *musb_channel = channel->private_data;
	struct musb_hw_ep       *hw_ep = musb_channel->hw_ep;
	struct musb *musb = hw_ep->musb;
	void __iomem *mbase = musb->mregs;
	unsigned long flags, pio;
	unsigned int txcsr;
#ifndef CONFIG_USB_U8500_DMA
	struct musb_qh          *qh = hw_ep->out_qh;
	struct urb              *urb;
#endif
	spin_lock_irqsave(&musb->lock, flags);
	musb_ep_select(mbase, hw_ep->epnum);
	channel->actual_len = musb_channel->cur_len;
	pio = musb_channel->len - channel->actual_len;

	if (!pio)
	{
		channel->status = MUSB_DMA_STATUS_FREE;
		musb_dma_completion(musb, musb_channel->epnum,
			musb_channel->transmit);
	}

	if (pio)
	{
		channel->status = MUSB_DMA_STATUS_FREE;
#ifndef CONFIG_USB_U8500_DMA
		urb = next_urb(qh);
		qh->offset += channel->actual_len;
		buf = urb->transfer_buffer + qh->offset;
		musb_write_fifo(hw_ep, pio, buf);
		qh->segsize = pio;
		musb_writew(hw_ep->regs, MUSB_TXCSR, MUSB_TXCSR_TXPKTRDY);
		//channel->last_xfer = 1;
#endif
	}

	spin_unlock_irqrestore(&musb->lock, flags);
}

/**
 * dma_controller_destroy() - deallocates the DMA controller
 * @c:	pointer to dma controller.
 *
 * This function deallocates the DMA controller.
*/

void dma_controller_destroy(struct dma_controller *c)
{
	struct musb_dma_controller *controller = container_of(c,
			struct musb_dma_controller, controller);
	struct musb_dma_channel *musb_channel = NULL;
	u8 bit;
	if (!controller)
		return;
	for (bit = 0; bit < MUSB_HSDMA_CHANNELS; bit++) {
		musb_channel = &(controller->channel[bit]);
			}
	if (controller->irq)
		free_irq(controller->irq, c);

	kfree(controller);
}

/**
 * dma_controller_create() - creates the dma controller and initializes callbacks
 *
 * @musb:	pointer to mentor core driver data instance|
 * @base:	base address of musb registers.
 *
 * This function creates the DMA controller and initializes the callbacks
 * that are invoked from the Mentor IP core.
*/

struct dma_controller *__init
dma_controller_create(struct musb *musb, void __iomem *base)
{
	struct musb_dma_controller *controller;

	controller = kzalloc(sizeof(*controller), GFP_KERNEL);
	if (!controller)
		return NULL;

	controller->channel_count = MUSB_HSDMA_CHANNELS;
	controller->private_data = musb;
	controller->base = base;

	controller->controller.start = dma_controller_start;
	controller->controller.stop = dma_controller_stop;
	controller->controller.channel_alloc = dma_channel_allocate;
	controller->controller.channel_release = dma_channel_release;
	controller->controller.channel_program = dma_channel_program;
	controller->controller.channel_abort = dma_channel_abort;

	return &controller->controller;
}
