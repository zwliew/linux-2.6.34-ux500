/*
 * arch/arm/mach-stn8500/include/mach/i2c-stm.h
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

#ifndef STM_I2C_HEADER
#define STM_I2C_HEADER
#include <linux/i2c.h>

/*
 * I2C Controller register offsets
 */
#define I2C_CR(r)	(r + 0x000)
#define I2C_SCR(r)	(r + 0x004)
#define I2C_HSMCR(r)	(r + 0x008)
#define I2C_MCR(r)	(r + 0x00C)
#define I2C_TFR(r)	(r + 0x010)
#define I2C_SR(r)	(r + 0x014)
#define I2C_RFR(r)	(r + 0x018)
#define I2C_TFTR(r)	(r + 0x01C)
#define I2C_RFTR(r)	(r + 0x020)
#define I2C_DMAR(r)	(r + 0x024)
#define I2C_BRCR(r)	(r + 0x028)
#define I2C_IMSCR(r)	(r + 0x02C)
#define I2C_RISR(r)	(r + 0x030)
#define I2C_MISR(r)	(r + 0x034)
#define I2C_ICR(r)	(r + 0x038)
#define I2C_THD_FST_STD(r)	(r + 0x050)
#define I2C_THU_FST_STD(r)	(r + 0x058)


/*
 *  I2C :: Controller Register
 */
#define I2C_CR_PE_POS		0
#define I2C_CR_OM_POS		1
#define I2C_CR_SAM_POS		3
#define I2C_CR_SM_POS		4
#define I2C_CR_SGCM_POS		6
#define I2C_CR_FTX_POS		7
#define I2C_CR_FRX_POS		8
#define I2C_CR_DMA_TX_EN_POS 	9
#define I2C_CR_DMA_RX_EN_POS	10
#define I2C_CR_DMA_SLE_POS	11
#define I2C_CR_LM_POS		12
#define I2C_CR_FON_POS		13
#define I2C_CR_FS_POS		15

#define I2C_CR_PE          ((u32)(0x1UL << I2C_CR_PE_POS))
#define I2C_CR_OM          ((u32)(0x3UL << I2C_CR_OM_POS))
#define I2C_CR_SAM         ((u32)(0x1UL << I2C_CR_SAM_POS))
#define I2C_CR_SM          ((u32)(0x3UL << I2C_CR_SM_POS))
#define I2C_CR_SGCM        ((u32)(0x1UL << I2C_CR_SGCM_POS))
#define I2C_CR_FTX         ((u32)(0x1UL << I2C_CR_FTX_POS))
#define I2C_CR_FRX         ((u32)(0x1UL << I2C_CR_FRX_POS))
#define I2C_CR_DMA_TX_EN   ((u32)(0x1UL << I2C_CR_DMA_TX_EN_POS))
#define I2C_CR_DMA_RX_EN   ((u32)(0x1UL << I2C_CR_DMA_RX_EN_POS))
#define I2C_CR_DMA_SLE     ((u32)(0x1UL << I2C_CR_DMA_SLE_POS))
#define I2C_CR_LM          ((u32)(0x1UL << I2C_CR_LM_POS))
#define I2C_CR_FON         ((u32)(0x3UL << I2C_CR_FON_POS))
#define I2C_CR_FS		((u32)(0x3UL << I2C_CR_FS_POS))

/*
 * I2C :: Slave Controller Register
 */
#define I2C_SCR_SLAVE_ADDR7BIT	((u32)(0x7FUL << 0))
#define I2C_SCR_ESA10		((u32)(0x7UL << 7))
#define I2C_SCR_DATA_SETUP_TIME ((u32)(0xFFFFUL << 16))

/*
 * I2C :: Master Controller Register
 */
#define I2C_MCR_OP_POS  	0
#define I2C_MCR_A7_POS		1
#define I2C_MCR_EA10_POS	8
#define I2C_MCR_SB_POS		11
#define I2C_MCR_AM_POS		12
#define I2C_MCR_STOP_POS 	14
#define I2C_MCR_LENGTH_POS	15

#define I2C_MCR_OP      ((u32)(0x1UL << I2C_MCR_OP_POS))
#define I2C_MCR_A7      ((u32)(0x7FUL << I2C_MCR_A7_POS))
#define I2C_MCR_EA10    ((u32)(0x7UL << I2C_MCR_EA10_POS))
#define I2C_MCR_SB      ((u32)(0x1UL << I2C_MCR_SB_POS))
#define I2C_MCR_AM      ((u32)(0x3UL << I2C_MCR_AM_POS))
#define I2C_MCR_STOP    ((u32)(0x1UL << I2C_MCR_STOP_POS))
#define I2C_MCR_LENGTH  ((u32)(0x7FFUL << I2C_MCR_LENGTH_POS))

/*
 *Transmit FIFO Register
 */
#define I2C_TFR_TDATA	((u32)(0xFFUL << 0))

/*
 *Status Register
 */
#define I2C_SR_OP       ((u32)(0x3UL << 0))
#define I2C_SR_STATUS   ((u32)(0x3UL << 2))
#define I2C_SR_CAUSE    ((u32)(0x7UL << 4))
#define I2C_SR_TYPE     ((u32)(0x3UL << 7))
#define I2C_SR_LENGTH   ((u32)(0x7FFUL << 9))

/*
 *Receive FIFO Register
 */
#define I2C_RFR_RDATA	((u32)(0xFFUL << 0))

/*
 *Transmit FIFO Threshhold register
 */
#define I2C_TFTR_THRESHOLD_TX	((u32)(0x3FFUL << 0))

/*
 *Receive FIFO Threshhold register
 */
#define I2C_RFTR_THRESHOLD_RX	((u32)(0x3FFUL << 0))

/*
 *BaudRate Control Register
 */
#define I2C_BRCR_BRCNT2  0x0000FFFF
#define I2C_BRCR_BRCNT1  0xFFFF0000

/*
 *Interrupt bits in I2C_IMSCR, I2C_ICR, I2C_RIS
 */
#define I2C_IT_TXFE 	((u32)(0x1UL << 0))
#define I2C_IT_TXFNE	((u32)(0x1UL << 1))
#define I2C_IT_TXFF	((u32)(0x1UL << 2))
#define I2C_IT_TXFOVR	((u32)(0x1UL << 3))
#define I2C_IT_RXFE	((u32)(0x1UL << 4))
#define I2C_IT_RXFNF	((u32)(0x1UL << 5))
#define I2C_IT_RXFF	((u32)(0x1UL << 6))
#define I2C_IT_RFSR	((u32)(0x1UL << 16))
#define I2C_IT_RFSE	((u32)(0x1UL << 17))
#define I2C_IT_WTSR	((u32)(0x1UL << 18))
#define I2C_IT_MTD	((u32)(0x1UL << 19))
#define I2C_IT_STD	((u32)(0x1UL << 20))
#define I2C_IT_MAL	((u32)(0x1UL << 24))
#define I2C_IT_BERR	((u32)(0x1UL << 25))
#define I2C_IT_MTDWS	((u32)(0x1UL << 28))


#define I2C_IRQ_SRC_ALL		0x131F007F

/**************************************************************/
#define I2C_TEST_BIT(reg_name, val)          (readl(reg_name) & (val))
#define NUM_I2C_ADAPTERS        4


#define MAX_I2C_FIFO_THRESHOLD 15


typedef enum {
	I2C_NO_OPERATION = 0xFF,
	I2C_WRITE = 0x00,
	I2C_READ = 0x01
} i2c_operation_t;


typedef enum {
	I2C_DIGITAL_FILTERS_OFF,
	I2C_DIGITAL_FILTERS_1_CLK_SPIKES,
	I2C_DIGITAL_FILTERS_2_CLK_SPIKES,
	I2C_DIGITAL_FILTERS_4_CLK_SPIKES
} i2c_digital_filter_t;

typedef enum {
	I2C_DISABLED,
	I2C_ENABLED
} i2c_control_t;

typedef enum {
	I2C_FREQ_MODE_STANDARD,	/* Standard mode.   */
	I2C_FREQ_MODE_FAST,	/* Fast mode.       */
	I2C_FREQ_MODE_HIGH_SPEED
} i2c_freq_mode_t;

typedef enum {
	I2C_BUS_SLAVE_MODE = 0,	/* Slave Mode               */
	I2C_BUS_MASTER_MODE,	/* Master Mode              */
	I2C_BUS_MASTER_SLAVE_MODE	/* Dual Configuration Mode  */
} i2c_bus_control_mode_t;

typedef enum {
	I2C_NO_GENERAL_CALL_HANDLING,
	I2C_SOFTWARE_GENERAL_CALL_HANDLING,
	I2C_HARDWARE_GENERAL_CALL_HANDLING
} i2c_general_call_handling_t;

typedef enum {
	I2C_TRANSFER_MODE_POLLING,
	I2C_TRANSFER_MODE_INTERRUPT,
	I2C_TRANSFER_MODE_DMA
} i2c_transfer_mode_t;

typedef enum {
	I2C_7_BIT_ADDRESS = 0x1,
	I2C_10_BIT_ADDRESS = 0x2
} i2c_addr_t;

typedef enum {
	I2C_NO_INDEX,		/* Current transfer is non-indexed      */
	I2C_BYTE_INDEX,		/* Current transfer uses 8-bit index    */
	I2C_HALF_WORD_LITTLE_ENDIAN,	/* Current transfer uses 16-bit index
					   in little endian mode */
	I2C_HALF_WORD_BIG_ENDIAN,	/* Current transfer uses 16-bit index
					   in big endian mode */
	I2C_INVALID_INDEX = -1
} i2c_index_format_t;

typedef enum{
	I2C_NOP,
	I2C_ON_GOING,
	I2C_OK,
	I2C_ABORT
} i2c_status_t;

typedef enum {
	I2C_NACK_ADDR,
	I2C_NACK_DATA,
	I2C_ACK_MCODE,
	I2C_ARB_LOST,
	I2C_BERR_START,
	I2C_BERR_STOP,
	I2C_OVFL
} i2c_error_t;

/**
 * struct i2c_platform_data - Platform Data structure for I2C controller
 * @gpio_alt_func: Gpio Alternate function used by this controller
 * @name: Name of this I2C controller
 * @own_addr: Own address of this I2C controller on the i2c bus
 * @mode: Freq mode in which this controller will operate(std, fast, highspeed)
 * @input_freq: Input frequency of the I2C controller
 * @clk_freq: Frequenct at which SCL line is driven (e.g. 100 KHz)
 * @slave_addressing_mode: seven or 10 bit addressing
 * @digital_filter_control: Digital Filters to be applied
 * @dma_sync_logic_control: Enabled/Disaled
 * @start_byte_procedure: Enabled/Disabled
 * @slave_data_setup_time: Data setup time for slave
 * @bus_control_mode: Whether slave or master
 * @i2c_loopback_mode: If true controller works in Loopback mode
 * @xfer_mode: Polling/Interrupt/DMA mode
 * @high_speed_master_code: Code for high speed master mode
 * @i2c_tx_int_threshold: Interrupt Tx threshold
 * @i2c_rx_int_threshold: Interrupt Rx Threshold
 *
 **/
struct i2c_platform_data {
	gpio_alt_function gpio_alt_func;
	char name[48];
	u32 own_addr;
	i2c_freq_mode_t mode;
	u32 input_freq;
	u32 clk_freq;
	i2c_addr_t slave_addressing_mode;
	i2c_digital_filter_t digital_filter_control;
	i2c_control_t dma_sync_logic_control;
	i2c_control_t start_byte_procedure;
	u16 slave_data_setup_time;
	i2c_bus_control_mode_t bus_control_mode;
	i2c_control_t i2c_loopback_mode;
	i2c_transfer_mode_t xfer_mode;
	u8 high_speed_master_code;
	u8 i2c_tx_int_threshold;
	u8 i2c_rx_int_threshold;
};


/**
 * struct i2c_controller_config - Configuration stored by controller
 * @own_addr: Controllers own address on the I2C bus
 * @clk_freq: Frequency at which SCL line is Driven
 * @digital_filter_control: Digital filter control to be applied
 * @dma_sync_logic_control: Enabled/Disabled
 * @start_byte_procedure: Enabled/Disabled
 * @slave_data_setup_time: Slave set up time
 * @slave_addressing_mode: seven or 10 bit addressing
 * @high_speed_master_code: Code for high speed master mode
 * @input_freq: Input frequency of the I2C controller
 * @mode: Standard/fast/Highspeed mode
 * @bus_control_mode: Master/Slave/Dual
 * @i2c_loopback_mode: Loopback mode
 * @general_call_mode_handling:
 * @xfer_mode: Polling/Interrupt/DMA mode
 * @i2c_transmit_interrupt_threshold: Tx interrupt threshold
 * @i2c_receive_interrupt_threshold: Rx interrupt threshold
 * @transmit_burst_length:
 * @receive_burst_length:
 * @burst_length:
 *
 **/
struct i2c_controller_config {
	u16 own_addr;
	u32 clk_freq;
	i2c_digital_filter_t digital_filter_control;
	i2c_control_t dma_sync_logic_control;
	i2c_control_t start_byte_procedure;
	u16 slave_data_setup_time;
	i2c_addr_t slave_addressing_mode;
	u8 high_speed_master_code;
	u32 input_freq;
	i2c_freq_mode_t mode;
	i2c_bus_control_mode_t bus_control_mode;
	i2c_control_t i2c_loopback_mode;
	i2c_general_call_handling_t general_call_mode_handling;
	i2c_transfer_mode_t xfer_mode;
	u8 i2c_transmit_interrupt_threshold;
	u8 i2c_receive_interrupt_threshold;
	u8 transmit_burst_length;
	u8 receive_burst_length;
	u16 burst_length;
};


/**
 * struct client_data - Holds Data specific to a client we are dealing with
 * @slave_address: Address of the Slave I2C Chip
 * @register_index: Index at which we wish to read data on this I2C slave chip
 * @index_format: 8bit/16bit(big/Little endian) index
 * @count_data: The number of bytes to be transferred.
 * @databuffer: Pointer to the data buffer. Used in Multi operation.
 * @transfer_data: Number of bytes transferred till now
 * @operation: Read/Write
 *
 *
 **/
struct client_data {
	u16 slave_address;
	u16 register_index;
	i2c_index_format_t index_format;
	u32 count_data;
	u8 *databuffer;
	u32 transfer_data;
	i2c_operation_t operation;
};

/**
 * struct i2c_driver_data - Private data structure for the I2C Controller driver
 * @id: Bus Id of this I2C controller
 * @adap: reference to the I2C adapter structure
 * @irq: IRQ line for this Controller
 * @regs: I/O regs Memory Area of this controller
 * @event_wq:
 * @config: Controller configuration
 * @cli_data: Data of the client transfer we are about to perform
 * @stop: Whether we need a stop condition or not
 * @xfer_complete: used for timeout in interrupt xfer.
 *
 *
 **/
struct i2c_driver_data {
	struct i2c_adapter adap;
	int irq;
	void __iomem *regs;
	struct clk *clk;
	struct i2c_controller_config cfg;
	struct client_data cli_data;
	int stop;
	int result;
	struct completion xfer_complete;
};

//#define STD_F_IN_HZ    48000000
#define STD_F_IN_HZ    24000000
#define STD_SPEED_IN_HZ 100000

#define I2C_FIFO_FLUSH_COUNTER 5000000
#define I2C_STATUS_UPDATE_COUNTER 5000000

#define I2C_SET_BIT(reg_name, mask) (writel(readl(reg_name) | mask, (reg_name)))
#define I2C_CLR_BIT(reg_name, mask) (writel(readl(reg_name) & ~(mask), \
								(reg_name)))

#define I2C_ALGO_STM  0x15000000
#define I2C_HW_STM    0x01
#define I2C_DRIVERID_STM 0xF000
#define WRITE_FIELD(reg_name, mask, shift, value) \
	(reg_name = ((reg_name & ~mask) | (value << shift)))

#define GEN_MASK(val, mask, sb)  ((u32)((((u32)val)<<(sb)) & (mask)))

#define STM_SET_BITS(reg, mask)			((reg) |=  (mask))
#define STM_CLEAR_BITS(reg, mask)		((reg) &= ~(mask))

#define I2C_WRITE 0
#define I2C_READ 1

#define I2C_MCR_LENGTH_STOP_OP    0x3FFC001
#define I2C_WRITE_FIELD(reg_name, mask, shift, value) \
	(writel((readl(reg_name) & ~mask) | (value << shift), \
		reg_name))

#define DEFAULT_BRCNT_REG (GEN_MASK((__u32)(STD_F_IN_HZ/(STD_SPEED_IN_HZ*2)), \
			I2C_BRCR_BRCNT2, 0)| \
		GEN_MASK(0, I2C_BRCR_BRCNT1, 16))

#endif
