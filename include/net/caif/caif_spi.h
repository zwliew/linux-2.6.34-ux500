/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Daniel Martensson / Daniel.Martensson@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CAIF_SPI_H_
#define CAIF_SPI_H_

#define SPI_CMD_WR			0x00
#define SPI_CMD_RD			0x01
#define SPI_CMD_EOT			0x02
#define SPI_CMD_IND			0x04

#define SPI_DMA_BUF_LEN			8192

#define WL_SZ				2	/* 16 bits. */
#define SPI_CMD_SZ			4	/* 32 bits. */
#define SPI_IND_SZ			4	/* 32 bits. */

#define SPI_XFER			0
#define SPI_SS_ON			1
#define SPI_SS_OFF			2

/* Minimum time between different levels is 50 microseconds. */
#define MIN_TRANSITION_TIME_USEC	50

/* Defines for calculating duration of SPI transfers for a particular
 * number of bytes.
 */
#define SPI_MASTER_CLK_MHZ		13
#define SPI_XFER_TIME_USEC(bytes, clk) (((bytes) * 8) / clk)

#define SPI_TX_DEBUG			1
#define SPI_RX_DEBUG			2
#define SPI_LOG_DEBUG			4

/* Structure describing a SPI transfer. */
struct cfspi_xfer {
	uint16 tx_dma_len;
	uint16 rx_dma_len;
	void *va_tx;
	dma_addr_t pa_tx;
	void *va_rx;
	dma_addr_t pa_rx;
};

/* Structure implemented by the SPI interface. */
struct cfspi_ifc {
	void (*ss_cb) (bool assert, struct cfspi_ifc *ifc);
	void (*xfer_done_cb) (struct cfspi_ifc *ifc);
	void *priv;
};

/* Structure implemented by SPI clients. */
struct cfspi_dev {
	int (*init_xfer) (struct cfspi_xfer *xfer, struct cfspi_dev *dev);
	void (*sig_xfer) (bool xfer, struct cfspi_dev *dev);
	struct cfspi_ifc *ifc;
	char *name;
	uint32 clk_mhz;
	void *priv;
};

#ifdef CONFIG_DEBUG_FS
/* Enumeration describing the CAIF SPI state. */
enum cfspi_state {
	CFSPI_STATE_WAITING = 0,
	CFSPI_STATE_AWAKE,
	CFSPI_STATE_FETCH_PKT,
	CFSPI_STATE_GET_NEXT,
	CFSPI_STATE_INIT_XFER,
	CFSPI_STATE_WAIT_ACTIVE,
	CFSPI_STATE_SIG_ACTIVE,
	CFSPI_STATE_WAIT_XFER_DONE,
	CFSPI_STATE_XFER_DONE,
	CFSPI_STATE_WAIT_INACTIVE,
	CFSPI_STATE_SIG_INACTIVE,
	CFSPI_STATE_DELIVER_PKT,
	CFSPI_STATE_MAX,
};
#endif				/* CONFIG_DEBUG_FS */

/* Structure implemented by SPI physical interfaces. */
struct cfspi_phy {
	uint16 cmd;
	uint16 tx_cpck_len;
	uint16 tx_npck_len;
	uint16 rx_cpck_len;
	uint16 rx_npck_len;
	struct layer layer;
	struct cfspi_ifc ifc;
	struct cfspi_xfer xfer;
	struct cfspi_dev *dev;
	unsigned long state;
	struct work_struct work;
	struct workqueue_struct *wq;
	struct list_head list;
	struct completion comp;
	wait_queue_head_t wait;
	spinlock_t lock;
#ifdef CONFIG_DEBUG_FS
	enum cfspi_state dbg_state;
	uint16 pcmd;
	uint16 tx_ppck_len;
	uint16 rx_ppck_len;
	struct dentry *dbgfs_dir;
	struct dentry *dbgfs_state;
	struct dentry *dbgfs_frame;
#endif				/* CONFIG_DEBUG_FS */
};

#endif				/* CAIF_SPI_H_ */
