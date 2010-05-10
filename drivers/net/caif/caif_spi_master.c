/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif				/* KERN_VERSION_2_6_27 */
#include <linux/workqueue.h>
#include <linux/completion.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/sched.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif				/* CONFIG_DEBUG_FS */

/* CAIF header files. */
/*#include <net/caif/generic/cfcnfg.h>*/
#include <net/caif/caif_spi.h>

#ifdef CONFIG_UML
static inline void  __BUG_ON(unsigned long condition, int line)
{
	if (condition)
		printk(KERN_ERR "BUG_ON: file: %s, line: %d.\n", __FILE__, line);
	else
		return;
}
#undef BUG_ON
#define BUG_ON(x) __BUG_ON((unsigned long)(x), __LINE__)
#endif	/* CONFIG_UML */

extern void cfspi_dbg_state(struct cfspi *cfspi, int state);
extern int cfspi_xmitfrm(struct cfspi *cfspi, uint8_t *buf, size_t len);
extern int cfspi_xmitlen(struct cfspi *cfspi);
extern int cfspi_rxfrm(struct cfspi *cfspi, uint8_t *buf, size_t len);
extern int cfspi_spi_remove(struct platform_device *pdev);
extern int cfspi_spi_probe(struct platform_device *pdev);


int spi_frm_align = 2;
int spi_up_head_align = 3;
int spi_up_tail_align = 1;
int spi_down_head_align = 1;
int spi_down_tail_align;


void cfspi_xfer(struct work_struct *work)
{
	struct cfspi *cfspi = NULL;
	uint8_t *ptr = NULL;
	bool eot_sent = true;
	bool send_read_cmd = true;
	unsigned long flags;

	cfspi = container_of(work, struct cfspi, work);

 master_loop:

	cfspi_dbg_state(cfspi, CFSPI_STATE_WAITING);

	/* Wait for slave talk or transmit event. */
	wait_event_interruptible(cfspi->wait,
				 test_bit(SPI_XFER, &cfspi->state));

	cfspi_dbg_state(cfspi, CFSPI_STATE_AWAKE);

	/* Check whether this is the first burst in the transfer. */
	if (eot_sent) {
		/* Make sure that the minimum toggle time is respected. */
		udelay(MIN_TRANSITION_TIME_USEC);

		/* Signal that we are ready to receive data. */
		cfspi->dev->sig_xfer(true, cfspi->dev);

		/* Clear state. */
		eot_sent = false;
		send_read_cmd = true;
	}

	/* Check whether we have a committed frame. */
	if (cfspi->tx_cpck_len) {
		int len;

		cfspi_dbg_state(cfspi, CFSPI_STATE_FETCH_PKT);

		/* Fetch commited SPI frames. */
		ptr = (uint8_t *) cfspi->xfer.va_tx;
		len = cfspi_xmitfrm(cfspi, ptr, cfspi->tx_cpck_len);
		BUG_ON(len != cfspi->tx_cpck_len);
	}

	cfspi_dbg_state(cfspi, CFSPI_STATE_GET_NEXT);

	/* Get length of next frame to commit. */
	cfspi->tx_npck_len = cfspi_xmitlen(cfspi);

	if (cfspi->tx_npck_len > SPI_DMA_BUF_LEN) {
		printk(KERN_WARNING
		       "MSPI: TX len error: %d.\n", cfspi->tx_npck_len);
	}

	/* Add SPI command for the master device. The SPI comand is added to
	 * the end of the frame.
	 */
	ptr = (uint8_t *) cfspi->xfer.va_tx + cfspi->tx_cpck_len;
	if (cfspi->tx_npck_len) {
		*ptr++ = (SPI_CMD_WR & 0x0FF);
		*ptr++ = ((SPI_CMD_WR & 0xFF00) >> 8);
		/* As long as we are writing, we have to send a RD before EOT.
		 */
		send_read_cmd = true;
	} else if (!cfspi->rx_npck_len) {
		if (send_read_cmd == true) {
			*ptr++ = (SPI_CMD_RD & 0x0FF);
			*ptr++ = ((SPI_CMD_RD & 0xFF00) >> 8);
			send_read_cmd = false;
		} else {
			*ptr++ = (SPI_CMD_EOT & 0x0FF);
			*ptr++ = ((SPI_CMD_EOT & 0xFF00) >> 8);
			eot_sent = true;
		}
	} else {
		*ptr++ = (SPI_CMD_RD & 0x0FF);
		*ptr++ = ((SPI_CMD_RD & 0xFF00) >> 8);
		send_read_cmd = false;
	}

	/* Add length to TX DMA buffer. */
	*ptr++ = (cfspi->tx_npck_len & 0x00FF);
	*ptr++ = ((cfspi->tx_npck_len & 0xFF00) >> 8);

	/* Calculate length of DMAs. */
	cfspi->xfer.tx_dma_len = cfspi->tx_cpck_len + SPI_CMD_SZ;
	cfspi->xfer.rx_dma_len = cfspi->rx_cpck_len + SPI_IND_SZ;

	/* Add SPI TX frame alignment padding if necessary. */
	if (cfspi->tx_cpck_len && (cfspi->xfer.tx_dma_len % spi_frm_align)) {
		cfspi->xfer.tx_dma_len += spi_frm_align -
		    (cfspi->xfer.tx_dma_len % spi_frm_align);
	}

	/* Add SPI RX frame alignment padding if necessary. */
	if (cfspi->rx_cpck_len && (cfspi->xfer.rx_dma_len % spi_frm_align)) {
		cfspi->xfer.rx_dma_len += spi_frm_align -
		    (cfspi->xfer.rx_dma_len % spi_frm_align);
	}

	/* Wait for slave to synchronize. */
	wait_event_interruptible(cfspi->wait,
				 test_bit(SPI_SS_ON, &cfspi->state));
	clear_bit(SPI_SS_ON, &cfspi->state);

	cfspi_dbg_state(cfspi, CFSPI_STATE_INIT_XFER);

	/* Start transfer. */
	if (cfspi->dev->init_xfer(&cfspi->xfer, cfspi->dev))
		printk(KERN_ERR "MSPI: init_xfer failed.\n");

	cfspi_dbg_state(cfspi, CFSPI_STATE_WAIT_XFER_DONE);

	/* Wait for transfer completion. */
	wait_for_completion(&cfspi->comp);

	cfspi_dbg_state(cfspi, CFSPI_STATE_XFER_DONE);

	/* Wait for slave to synchronize. */
	wait_event_interruptible(cfspi->wait,
				 test_bit(SPI_SS_OFF, &cfspi->state));
	clear_bit(SPI_SS_OFF, &cfspi->state);

	/* Check the SPI indication and length. */
	ptr = (uint8_t *) cfspi->xfer.va_rx;
	cfspi->cmd = *ptr++;
	cfspi->cmd |= (((*ptr++) << 8) & 0xFF00);
	cfspi->rx_npck_len = *ptr++;
	cfspi->rx_npck_len |= (((*ptr++) << 8) & 0xFF00);

	BUG_ON(cfspi->cmd != SPI_CMD_IND);

	BUG_ON(cfspi->rx_cpck_len > SPI_DMA_BUF_LEN);

	/* Check whether we received a CAIF packet. */
	if (cfspi->rx_cpck_len) {
		int len;

		cfspi_dbg_state(cfspi, CFSPI_STATE_DELIVER_PKT);

		/* Parse SPI frame. */
		ptr = ((uint8_t *)cfspi->xfer.va_rx) + SPI_IND_SZ;
		len = cfspi_rxfrm(cfspi, ptr, cfspi->rx_cpck_len);
		BUG_ON(len != cfspi->rx_cpck_len);
	}


	/* Reiterate, if necessary. */
	if (eot_sent == true) {
		/* Reset state. */
		cfspi->tx_cpck_len = 0;
		cfspi->rx_cpck_len = 0;

		/* Make sure that the minimum toggle time is respected. */
		if (SPI_XFER_TIME_USEC
		    (cfspi->xfer.tx_dma_len,
		     cfspi->dev->clk_mhz) < MIN_TRANSITION_TIME_USEC) {
			udelay(MIN_TRANSITION_TIME_USEC -
			       SPI_XFER_TIME_USEC(cfspi->xfer.tx_dma_len,
						  cfspi->dev->clk_mhz));
		}

		/* De-assert transfer signal. */
		cfspi->dev->sig_xfer(false, cfspi->dev);

		/* Check whether we need to clear the xfer bit.
		 * Spin lock is needed for packet insertion.
		 * Test and clear of different bits
		 * are not supported. */
		spin_lock_irqsave(&cfspi->lock, flags);
		if ((!cfspi_xmitlen(cfspi))
		    && (!test_bit(SPI_SS_ON, &cfspi->state))) {
			clear_bit(SPI_XFER, &cfspi->state);
		}
		spin_unlock_irqrestore(&cfspi->lock, flags);
	} else {
		/* Update state. */
		cfspi->tx_cpck_len = cfspi->tx_npck_len;
		cfspi->rx_cpck_len = cfspi->rx_npck_len;
	}

	goto master_loop;
}

struct platform_driver cfspi_spi_driver = {
	.probe = cfspi_spi_probe,
	.remove = cfspi_spi_remove,
	.driver = {
		   .name = "cfspi_mspi",
		   .owner = THIS_MODULE,
		   },
};
