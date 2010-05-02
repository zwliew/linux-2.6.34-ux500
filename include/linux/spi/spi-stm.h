/*
 * include/linux/spi/spi-stm.h
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

#ifndef SPI_STM_HEADER
#define SPI_STM_HEADER

/*PUBLIC INTERFACE : To be used by client drivers */
/**
 * Protocols supported across different SPI controllers
 */
typedef enum {
	SPI_INTERFACE_MOTOROLA_SPI,	/* Motorola Interface */
	SPI_INTERFACE_TI_SYNC_SERIAL,	/* Texas Instrument Synch Serial iface*/
	SPI_INTERFACE_NATIONAL_MICROWIRE,/* National Semiconductor uwire iface*/
	SPI_INTERFACE_UNIDIRECTIONAL	/* Unidirectional interface */
} t_spi_interface;



/*Motorola SPI protocol specific definitions*/
typedef enum {
	SPI_CLK_ZERO_CYCLE_DELAY = 0x0,	/* Receive data on rising edge. */
	SPI_CLK_HALF_CYCLE_DELAY	/* Receive data on falling edge. */
} t_spi_clk_phase;

/* SPI Clock Polarity */
typedef enum {
	SPI_CLK_POL_IDLE_LOW,	/* Low inactive level */
	SPI_CLK_POL_IDLE_HIGH	/* High inactive level */
} t_spi_clk_pol;

struct motorola_spi_proto_params {
	t_spi_clk_phase clk_phase;
	t_spi_clk_pol clk_pol;

};


/* MICROWIRE protocol*/
/**
 * Microwire Conrol Lengths Command size in microwire format
 */
typedef enum {
	MWLEN_BITS_4 = 0x03, MWLEN_BITS_5, MWLEN_BITS_6,
	MWLEN_BITS_7, MWLEN_BITS_8, MWLEN_BITS_9,
	MWLEN_BITS_10, MWLEN_BITS_11, MWLEN_BITS_12,
	MWLEN_BITS_13, MWLEN_BITS_14, MWLEN_BITS_15,
	MWLEN_BITS_16, MWLEN_BITS_17, MWLEN_BITS_18,
	MWLEN_BITS_19, MWLEN_BITS_20, MWLEN_BITS_21,
	MWLEN_BITS_22, MWLEN_BITS_23, MWLEN_BITS_24,
	MWLEN_BITS_25, MWLEN_BITS_26, MWLEN_BITS_27,
	MWLEN_BITS_28, MWLEN_BITS_29, MWLEN_BITS_30,
	MWLEN_BITS_31, MWLEN_BITS_32
} t_microwire_ctrl_len;

/**
 * Microwire Wait State
 */
typedef enum {
	MWIRE_WAIT_ZERO,/* No wait state inserted after last command bit */
	MWIRE_WAIT_ONE	/* One wait state inserted after last command bit */
} t_microwire_wait_state;

/**
 * Microwire : whether Full/Half Duplex
 */
typedef enum {
	/* SSPTXD becomes bi-directional, SSPRXD not used */
	MICROWIRE_CHANNEL_FULL_DUPLEX,
	/* SSPTXD is an output, SSPRXD is an input. */
	MICROWIRE_CHANNEL_HALF_DUPLEX
} t_mw_duplex;


struct microwire_proto_params {
	t_microwire_ctrl_len ctrl_len;
	t_microwire_wait_state wait_state;
	t_mw_duplex duplex;
};

/*Common configuration for different SPI controllers*/
typedef enum {
	LOOPBACK_DISABLED,
	LOOPBACK_ENABLED
} t_spi_loopback;

typedef enum {
	SPI_MASTER,
	SPI_SLAVE
} t_spi_hierarchy;

/*Endianess of FIFO Data */
typedef enum {
	SPI_FIFO_MSB,
	SPI_FIFO_LSB
} t_spi_fifo_endian;

/**
 * SPI mode of operation (Communication modes)
 */
typedef enum {
	INTERRUPT_TRANSFER,
	POLLING_TRANSFER,
	DMA_TRANSFER
} t_spi_mode;

/**
 * CHIP select/deselect commands
 */
typedef enum {
	SPI_CHIP_SELECT,
	SPI_CHIP_DESELECT
} t_spi_chip_select;

/**
 * Type of DMA xfer (between SSP fifo & MEM, or SSP fifo & some device)
 */
typedef enum {
	SPI_WITH_MEM,
	SPI_WITH_PERIPH
} t_dma_xfer_type;

struct stm_spi_dma_half_channel_info {
	enum dma_endianess endianess;
	enum periph_data_width data_width;
	enum dma_burst_size burst_size;
	enum dma_buffer_type buffer_type;
};

struct stm_spi_dma_pipe_config {
	union{
		enum dma_src_dev_type src_dev_type;
		enum dma_dest_dev_type dst_dev_type;
	} cli_dev_type;
	u32 client_fifo_addr;
	enum dma_xfer_dir dma_dir;
	struct stm_spi_dma_half_channel_info src_info;
	struct stm_spi_dma_half_channel_info dst_info;
};

/**
 * nmdkspi_dma - DMA configuration for SPI and communicating device
 * @rx_cfg: DMA configuration of Rx Pipe
 * @tx_cfg: DMA configuration of Tx Pipe
 *
 */
struct nmdkspi_dma {
	struct stm_spi_dma_pipe_config rx_cfg;
	struct stm_spi_dma_pipe_config tx_cfg;
};

/*#########################################################################*/

/* Private Interface : Not meant for client drivers*/


#define SPI_REG_WRITE_BITS(reg, val, mask, sb) \
		((reg) =   (((reg) & ~(mask)) | (((val)<<(sb)) & (mask))))
#define GEN_MASK_BITS(val, mask, sb)  ((u32)((((u32)val)<<(sb)) & (mask)))

/*#######################################################################
	Message State
#########################################################################
 */
#define START_STATE                     ((void *)0)
#define RUNNING_STATE                   ((void *)1)
#define DONE_STATE                      ((void *)2)
#define ERROR_STATE                     ((void *)-1)

/*CONTROLLER COMMANDS*/
typedef enum {
	DISABLE_CONTROLLER  = 0,
	ENABLE_CONTROLLER ,
	DISABLE_ALL_INTERRUPT ,
	ENABLE_ALL_INTERRUPT ,
	ENABLE_DMA ,
	DISABLE_DMA ,
	FLUSH_FIFO ,
	RESTORE_STATE ,
	LOAD_DEFAULT_CONFIG ,
	CLEAR_ALL_INTERRUPT,
} cntlr_commands;



/*
 * Private Driver Data structure used by spi controllers
 * */

struct driver_data {
	struct amba_device *adev;
	struct spi_master *master;
	struct nmdk_spi_master_cntlr *master_info;
	void __iomem *regs;
	struct clk *clk;
#ifdef CONFIG_SPI_WORKQUEUE
	struct workqueue_struct *workqueue;
#endif
	struct work_struct spi_work;
	spinlock_t lock;
	struct list_head queue;

	int busy;
	int run;

	int dma_ongoing;
	dmach_t rx_dmach;
	dmach_t tx_dmach;

	struct tasklet_struct pump_transfers;
	struct tasklet_struct spi_dma_tasklet;
	struct timer_list spi_notify_timer;
	int spi_io_error;
	struct spi_message *cur_msg;
	struct spi_transfer *cur_transfer;
	struct chip_data *cur_chip;
	void *tx;
	void *tx_end;
	void *rx;
	void *rx_end;
	void (*write) (struct driver_data *drv_data);
	void (*read) (struct driver_data *drv_data);
	void (*delay) (struct driver_data *drv_data);
	int (*execute_cmd) (struct driver_data *drv_data, int cmd);

	atomic_t dma_cnt;
};

/***************************************************************************/

struct spi_dma_info{
	struct stm_dma_pipe_info rx_dma_info;
	struct stm_dma_pipe_info tx_dma_info;
};

/**
 * struct chip_data - To maintain runtime state of SPICntlr for each client chip
 * @ctr_regs: void pointer which is assigned a struct having regs of the cntlr.
 * @chip_id: Chip Id assigned to this client to identify it.
 * @n_bytes: how many bytes(power of 2) reqd for a given data width of client
 * @enable_dma: Whether to enable DMA or not
 * @dma_info: Structure holding DMA configuration for the client.
 * @write: function to be used to write when doing xfer for this chip
 * @null_write: function to be used for dummy write for receiving data.
 * @read: function to be used to read when doing xfer for this chip
 * @null_read: function to be used to for dummy read while writting data.
 * @cs_control: chip select callback provided by chip
 * @xfer_type: polling/interrupt/dma
 *
 * Runtime state of the SPI controller, maintained per chip,
 * This would be set according to the current message that would be served
 */
struct chip_data {
	void *ctr_regs;
	u32 chip_id;
	u8 n_bytes;
	u8 enable_dma;
	struct spi_dma_info *dma_info;
	void (*write) (struct driver_data *drv_data);
	void (*null_write) (struct driver_data *drv_data);
	void (*read) (struct driver_data *drv_data);
	void (*null_read) (struct driver_data *drv_data);
	void (*delay) (struct driver_data *drv_data);
	void (*cs_control) (u32 command);
	int xfer_type;
};

/**
 * struct nmdk_spi_master_cntlr - device.platform_data for SPI cntlr devices.
 * @num_chipselect: chipselects are used to distinguish individual
 *      SPI slaves, and are numbered from zero to num_chipselects - 1.
 *      each slave has a chipselect signal, but it's common that not
 *      every chipselect is connected to a slave.
 * @enable_dma: if true enables DMA driven transfers.
 */
struct nmdk_spi_master_cntlr {
	u8 num_chipselect;
	u8 enable_dma:1;
	u32 id;
	u32 base_addr;
	u32 rx_fifo_addr;
	u32 tx_fifo_addr;
	enum dma_src_dev_type rx_fifo_dev_type;
	enum dma_dest_dev_type tx_fifo_dev_type;
	gpio_alt_function gpio_alt_func;
	char *device_name;
};

/*
 * Functions declaration
 **/
extern void *next_transfer(struct driver_data *drv_data);
extern void giveback(struct spi_message *message, struct driver_data *drv_data);
extern void null_cs_control(u32 command);
extern int stm_spi_transfer(struct spi_device *spi, struct spi_message *msg);
extern void stm_spi_cleanup(struct spi_device *spi);
extern int init_queue(struct driver_data *drv_data);
extern int start_queue(struct driver_data *drv_data);
extern int stop_queue(struct driver_data *drv_data);
extern int destroy_queue(struct driver_data *drv_data);
extern irqreturn_t spi_dma_callback_handler(void *param, int irq);
extern void stm_spi_tasklet(unsigned long param);
extern int process_spi_dma_info(struct nmdkspi_dma *dma_config,
		struct chip_data *chip, void *data);

#endif
