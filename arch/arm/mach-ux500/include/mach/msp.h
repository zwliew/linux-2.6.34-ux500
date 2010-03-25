/*
 * Copyright (c) 2009 STMicroelectronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef _STM_MSP_HEADER
#define _STM_MSP_HEADER
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/i2s/i2s.h>
#include <asm/dma.h>
#include <mach/dma.h>
#include <linux/irqreturn.h>
/* Generic config struct. Use the actual values defined below for global
 * control register
 */
typedef enum {
	MSP_MULTICHANNEL_DISABLED = 0,
	MSP_MULTICHANNEL_ENABLED = 1
} t_msp_multichannel_enable;

typedef enum {
	MSP_STATE_IDLE = 0,
	MSP_STATE_CONFIGURED = 1,
	MSP_STATE_RUN = 2,
} t_msp_state;

typedef enum {
	MSP_COMPARISON_DISABLED = 0,
	MSP_COMPARISON_NONEQUAL_ENABLED = 2,
	MSP_COMPARISON_EQUAL_ENABLED = 3
} t_msp_rx_comparison_enable_mode;

#define RMCEN_BIT   0
#define RMCSF_BIT   1
#define RCMPM_BIT   3
#define TMCEN_BIT   5
#define TNCSF_BIT   6

typedef struct {
	t_msp_multichannel_enable rx_multichannel_enable;
	t_msp_multichannel_enable tx_multichannel_enable;
	t_msp_rx_comparison_enable_mode rx_comparison_enable_mode;
	u8 padding;
	u32 comparison_value;
	u32 comparison_mask;
	u32 rx_channel_0_enable;
	u32 rx_channel_1_enable;
	u32 rx_channel_2_enable;
	u32 rx_channel_3_enable;
	u32 tx_channel_0_enable;
	u32 tx_channel_1_enable;
	u32 tx_channel_2_enable;
	u32 tx_channel_3_enable;
} t_msp_multichannel_config;

enum msp_data_size {
	MSP_DATA_SIZE_DEFAULT = -1,
	MSP_DATA_SIZE_8BIT,
	MSP_DATA_SIZE_10BIT,
	MSP_DATA_SIZE_12BIT,
	MSP_DATA_SIZE_14BIT,
	MSP_DATA_SIZE_16BIT,
	MSP_DATA_SIZE_20BIT,
	MSP_DATA_SIZE_24BIT,
	MSP_DATA_SIZE_32BIT,
};

/**
 * struct msp_protocol_desc- MSP Protocol desc structure per MSP.
 * @rx_phase_mode: rx_phase_mode whether single or dual.
 * @tx_phase_mode: tx_phase_mode whether single or dual.
 * @rx_phase2_start_mode: rx_phase2_start_mode whether imediate or after
 * some delay.
 * @tx_phase2_start_mode: tx_phase2_start_mode whether imediate or after
 * some delay.
 * @rx_bit_transfer_format: MSP or LSB.
 * @tx_bit_transfer_format: MSP or LSB.
 * @rx_frame_length_1: Frame1 length 1,2,3..
 * @rx_frame_length_2: Frame2 length 1,2,3..
 * @tx_frame_length_1: Frame1 length 1,2,3..
 * @tx_frame_length_2: Frame2 length 1,2,3..
 * @rx_element_length_1: Element1 length 1,2,...
 * @rx_element_length_2: Element2 length 1,2,...
 * @tx_element_length_1: Element1 length 1,2,...
 * @tx_element_length_2: Element2 length 1,2,...
 * @rx_data_delay: Delay in clk cycle after frame sync
 * @tx_data_delay: Delay in clk cycle after frame sync
 * @rx_clock_pol: Rxpol whether rising or falling.It indicates pol of bit clock.
 * @tx_clock_pol: Txpol whether rising or falling.It indicates pol of bit clock.
 * @rx_frame_sync_pol: Frame sync pol whether rising or Falling.
 * @tx_frame_sync_pol: Frame sync pol whether rising or Falling.
 * @rx_half_word_swap: Word swap half word, full word.
 * @tx_half_word_swap: Word swap half word, full word.
 * @compression_mode: Compression mode whether Alaw or Ulaw or disabled.
 * @expansion_mode: Compression mode whether Alaw or Ulaw or disabled.
 * @spi_clk_mode: Spi clock mode to be enabled or not.
 * @spi_burst_mode: Spi burst mode to be enabled or not.
 * @frame_sync_ignore: Frame sync to be ignored or not. Ignore in case of Audio
 * codec acting as Master.
 * @frame_period: Frame period (clk cycles) after which new frame sync occurs.
 * @frame_width: Frame width (clk cycles) after which frame sycn changes state.
 * @total_clocks_for_one_frame: No. of clk cycles per frame.
 *
 * Main Msp protocol descriptor data structure to be used to store various info
 * in transmit or recevie configuration registers of an MSP.
 */

struct msp_protocol_desc {
	u32 rx_phase_mode;
	u32 tx_phase_mode;
	u32 rx_phase2_start_mode;
	u32 tx_phase2_start_mode;
	u32 rx_bit_transfer_format;
	u32 tx_bit_transfer_format;
	u32 rx_frame_length_1;
	u32 rx_frame_length_2;
	u32 tx_frame_length_1;
	u32 tx_frame_length_2;
	u32 rx_element_length_1;
	u32 rx_element_length_2;
	u32 tx_element_length_1;
	u32 tx_element_length_2;
	u32 rx_data_delay;
	u32 tx_data_delay;
	u32 rx_clock_pol;
	u32 tx_clock_pol;
	u32 rx_frame_sync_pol;
	u32 tx_frame_sync_pol;
	u32 rx_half_word_swap;
	u32 tx_half_word_swap;
	u32 compression_mode;
	u32 expansion_mode;
	u32 spi_clk_mode;
	u32 spi_burst_mode;
	u32 frame_sync_ignore;
	u32 frame_period;
	u32 frame_width;
	u32 total_clocks_for_one_frame;
};

/**
 * struct trans_data - MSP transfer data structure used during xfer.
 * @message: i2s message.
 * @msp: msp structure.
 * @tx_handler: callback handler for transmit path.
 * @rx_handler: callback handler for receive path.
 * @tx_callback_data: callback data for transmit.
 * @rx_callback_data: callback data for receive.
 *
 */
struct trans_data {
	struct i2s_message message;
	struct msp_struct *msp;
	 irqreturn_t(*tx_handler) (void *data, int irq);
	 irqreturn_t(*rx_handler) (void *data, int irq);
	void *tx_callback_data;
	void *rx_callback_data;
};

/**
 * struct msp_generic_config- MSP configuration structure used by i2s client.
 * @input_clock_freq: Input clock frequency default is 48MHz.
 * @rx_clock_sel: Receive clock selection (Provided by Sample Gen or external
 * source).
 * @tx_clock_sel: Transmit clock selection (Provided by Sample Gen or external.
 * source).
 * @srg_clock_sel: APB clock or clock dervied from Slave (Audio codec).
 * @rx_frame_sync_pol: Receive frame sync polarity.
 * @tx_frame_sync_pol: Transmit frame sync polarity.
 * @rx_frame_sync_sel: Rx frame sync signal is provided by which source.
 * External source or by frame generator logic.
 * @tx_frame_sync_sel: Tx frame sync signal is provided by which source.
 * External source or by frame generator logic.
 * @rx_fifo_config: Receive fifo enable or not.
 * @tx_fifo_config: Transmit fifo enable or not.
 * @spi_clk_mode: In case of SPI protocol spi modes: Normal, Zero delay or
 * half cycle delay.
 * @spi_burst_mode: Spi burst mode is enabled or not.
 * @loopback_enable: Loopback mode.
 * @tx_data_enable: Transmit extra delay enable.
 * @default_protocol_desc: Flag to indicate client defined protocol desc or
 * statically defined in msp.h.
 * @protocol_desc: Protocol desc structure filled by i2s client driver.
 * In case client defined default_prtocol_desc as 0.
 * @multichannel_configured: multichannel configuration structure.
 * @multichannel_config: multichannel is enabled or not.
 * @direction: Transmit, Receive or Both.
 * @work_mode: Dma, Polling or Interrupt.
 * @protocol: I2S, PCM, etc.
 * @frame_freq: Sampling freq at which data is sampled.
 * @frame_size: size of element.
 * @data_size: data size which defines the format in which data is written on
 * transmit or receive fifo. Only three modes 8,16,32 are supported.
 * @def_elem_len: Flag to indicate whether default element length is to be used
 * or should be changed acc to data size defined by user at run time.
 * @handler: callback handler in case of interrupt or dma.
 * @tx_callback_data: Callback data for transmit.
 * @rx_callback_data: Callback data for receive.
 *
 * Main Msp configuration data structure used by i2s client driver to fill
 * various info like data size, frequency etc.
 */
struct msp_generic_config {
	unsigned int input_clock_freq;
	unsigned int rx_clock_sel;
	unsigned int tx_clock_sel;
	unsigned int srg_clock_sel;
	unsigned int rx_frame_sync_pol;
	unsigned int tx_frame_sync_pol;
	unsigned int rx_frame_sync_sel;
	unsigned int tx_frame_sync_sel;
	unsigned int rx_fifo_config;
	unsigned int tx_fifo_config;
	unsigned int spi_clk_mode;
	unsigned int spi_burst_mode;
	unsigned int loopback_enable;
	unsigned int tx_data_enable;
	unsigned int default_protocol_desc;
	struct msp_protocol_desc protocol_desc;
	int multichannel_configured;
	t_msp_multichannel_config multichannel_config;
	unsigned int direction;
	unsigned int work_mode;
	unsigned int protocol;
	unsigned int frame_freq;
	unsigned int frame_size;
	enum msp_data_size data_size;
	unsigned int def_elem_len;
	 irqreturn_t(*handler) (void *data, int err_or_data);
	void *tx_callback_data;
	void *rx_callback_data;

};

/*** Protocols ***/
enum {
	MSP_I2S_PROTOCOL,
	MSP_PCM_PROTOCOL,
	MSP_PCM_COMPAND_PROTOCOL,
	MSP_AC97_PROTOCOL,
	MSP_MASTER_SPI_PROTOCOL,
	MSP_SLAVE_SPI_PROTOCOL,
	MSP_INVALID_PROTOCOL
};

/*** Sample Frequencies ***/
/* These are no longer required, frequencies in Hz can be used directly */
enum {
	MSP_SAMPLE_FREQ_NOT_SUPPORTED = -1,
	MSP_SAMPLE_FREQ_8KHZ = 8000,
	MSP_SAMPLE_FREQ_12KHZ = 12000,
	MSP_SAMPLE_FREQ_16KHZ = 16000,
	MSP_SAMPLE_FREQ_24KHZ = 24000,
	MSP_SAMPLE_FREQ_32KHZ = 32000,
	MSP_SAMPLE_FREQ_44KHZ = 44000,
	MSP_SAMPLE_FREQ_48KHZ = 48000,
	MSP_SAMPLE_FREQ_64KHZ = 64000,
	MSP_SAMPLE_FREQ_88KHZ = 88000,
	MSP_SAMPLE_FREQ_96KHZ = 96000,
	MSP_SAMPLE_FREQ_22KHZ = 22000,
	MSP_SAMPLE_FREQ_11KHZ = 11000
};

/*** Input Frequencies ***/
/* These are no longer required, frequencies in Hz can be used directly */
typedef enum {
	MSP_INPUT_FREQ_1MHZ = 1000,
	MSP_INPUT_FREQ_2MHZ = 2000,
	MSP_INPUT_FREQ_3MHZ = 3000,
	MSP_INPUT_FREQ_4MHZ = 4000,
	MSP_INPUT_FREQ_5MHZ = 5000,
	MSP_INPUT_FREQ_6MHZ = 6000,
	MSP_INPUT_FREQ_8MHZ = 8000,
	MSP_INPUT_FREQ_11MHZ = 11000,
	MSP_INPUT_FREQ_12MHZ = 12000,
	MSP_INPUT_FREQ_16MHZ = 16000,
	MSP_INPUT_FREQ_22MHZ = 22000,
	MSP_INPUT_FREQ_24MHZ = 24000,
	MSP_INPUT_FREQ_48MHZ = 48000
} t_msp_in_clock_freq;

#define MSP_INPUT_FREQ_APB 48000000

/*** Stereo mode. Used for APB data accesses as 16 bits accesses (mono),
 *   32 bits accesses (stereo).
 ***/
enum {
	MSP_MONO,
	MSP_STEREO
};

/* Direction (Transmit/Receive mode) */
enum {
	MSP_TRANSMIT_MODE,
	MSP_RECEIVE_MODE,
	MSP_BOTH_T_R_MODE
};

/* Dma mode should be used for large transfers,
 * polling mode should be used for transfers of a few bytes
 */
enum {
	MSP_DMA_MODE,
	MSP_POLLING_MODE,
	MSP_INTERRUPT_MODE
};

/* User client for the MSP */
typedef enum {
	MSP_NO_USER = 0,
	MSP_USER_SPI,
	MSP_USER_ALSA,
	MSP_USER_SAA,
} t_msp_user;

/*Flag structure for MSPx*/
typedef struct {
	struct semaphore lock;
	t_msp_user user;
} msp_flag;

/* User client for the MSP */
typedef enum {
	MSP_NO_MODE = 0,
	MSP_MODE_SPI,
	MSP_MODE_NON_SPI,
} t_msp_mode;

/* Transmit and receive configuration register */
#define MSP_BIG_ENDIAN           0x00000000
#define MSP_LITTLE_ENDIAN        0x00001000
#define MSP_UNEXPECTED_FS_ABORT  0x00000000
#define MSP_UNEXPECTED_FS_IGNORE 0x00008000
#define MSP_NON_MODE_BIT_MASK    0x00009000

/* Global configuration register
--------------------------------*/
#define RX_ENABLE             0x00000001
#define RX_FIFO_ENABLE        0x00000002
#define RX_SYNC_SRG           0x00000010
#define RX_CLK_POL_RISING     0x00000020
#define RX_CLK_SEL_SRG        0x00000040
#define TX_ENABLE             0x00000100
#define TX_FIFO_ENABLE        0x00000200
#define TX_SYNC_SRG_PROG      0x00001800
#define TX_SYNC_SRG_AUTO      0x00001000
#define TX_CLK_POL_RISING     0x00002000
#define TX_CLK_SEL_SRG        0x00004000
#define TX_EXTRA_DELAY_ENABLE 0x00008000
#define SRG_ENABLE            0x00010000
#define FRAME_GEN_ENABLE      0x00100000
#define SRG_CLK_SEL_APB       0x00000000
#define RX_FIFO_SYNC_HI       0x00000000
#define TX_FIFO_SYNC_HI       0x00000000
#define SPI_CLK_MODE_NORMAL   0x00000000

/* SPI Clock Modes enumertion
 * SPI clock modes of MSP provides compatibility with
 * the SPI protocol.MSP supports 2 SPI transfer formats.
 * MSP_ZERO_DELAY_SPI_MODE:MSP transmits data over Tx/Rx
 * Lines immediately after MSPTCK/MSPRCK rising/falling edge.
 * MSP_HALF_CYCLE_DELY_SPI_MODE:MSP transmits data  one-half cycle
 * ahead of the rising/falling edge of the MSPTCK
 */
enum {
	MSP_NON_SPI_PROTOCOL = 0,
	MSP_ZERO_DELAY_SPI_MODE = 2,
	MSP_HALF_CYCLE_DELY_SPI_MODE = 3
};

#define MSP_FRAME_SIZE_AUTO -1


#define MSP_DR 		0x00
#define MSP_GCR		0x04
#define MSP_TCF		0x08
#define MSP_RCF		0x0c
#define MSP_SRG		0x10
#define MSP_FLR		0x14
#define MSP_DMACR	0x18

#define MSP_IMSC	0x20
#define MSP_RIS		0x24
#define MSP_MIS 	0x28
#define MSP_ICR		0x2c
#define MSP_MCR		0x30
#define MSP_RCV 	0x34
#define MSP_RCM 	0x38

#define MSP_TCE0	0x40
#define MSP_TCE1	0x44
#define MSP_TCE2 	0x48
#define MSP_TCE3 	0x4c

#define MSP_RCE0	0x60
#define MSP_RCE1	0x64
#define MSP_RCE2	0x68
#define MSP_RCE3 	0x6c

#define MSP_ITCR	0x80
#define MSP_ITIP	0x84
#define MSP_ITOP	0x88
#define MSP_TSTDR	0x8c

#define MSP_PID0	0xfe0
#define MSP_PID1	0xfe4
#define MSP_PID2 	0xfe8
#define MSP_PID3	0xfec

#define MSP_CID0	0xff0
#define MSP_CID1 	0xff4
#define MSP_CID2	0xff8
#define MSP_CID3 	0xffc

/* Single or dual phase mode */
enum {
	MSP_SINGLE_PHASE,
	MSP_DUAL_PHASE
};

/* Frame length
------------------*/
enum {
	MSP_FRAME_LENGTH_1 = 0,
	MSP_FRAME_LENGTH_2 = 1,
	MSP_FRAME_LENGTH_4 = 3,
	MSP_FRAME_LENGTH_8 = 7,
	MSP_FRAME_LENGTH_12 = 11,
	MSP_FRAME_LENGTH_16 = 15,
	MSP_FRAME_LENGTH_20 = 19,
	MSP_FRAME_LENGTH_32 = 31,
	MSP_FRAME_LENGTH_48 = 47,
	MSP_FRAME_LENGTH_64 = 63
};

/* Element length */
enum {
	MSP_ELEM_LENGTH_8 = 0,
	MSP_ELEM_LENGTH_10 = 1,
	MSP_ELEM_LENGTH_12 = 2,
	MSP_ELEM_LENGTH_14 = 3,
	MSP_ELEM_LENGTH_16 = 4,
	MSP_ELEM_LENGTH_20 = 5,
	MSP_ELEM_LENGTH_24 = 6,
	MSP_ELEM_LENGTH_32 = 7
};
enum {
	MSP_DATA_TRANSFER_WIDTH_BYTE,
	MSP_DATA_TRANSFER_WIDTH_HALFWORD,
	MSP_DATA_TRANSFER_WIDTH_WORD
};
enum {
	MSP_FRAME_SYNC_UNIGNORE = 0,
	MSP_FRAME_SYNC_IGNORE = 1,

};
enum {
	MSP_PHASE2_START_MODE_IMEDIATE,
	MSP_PHASE2_START_MODE_FRAME_SYNC
};
enum {
	MSP_BTF_MS_BIT_FIRST = 0,
	MSP_BTF_LS_BIT_FIRST = 1
};

enum {
	MSP_FRAME_SYNC_POL_ACTIVE_HIGH = 0,
	MSP_FRAME_SYNC_POL_ACTIVE_LOW = 1
};
/* Data delay (in bit clock cycles)
---------------------------------------*/
enum {
	MSP_DELAY_0 = 0,
	MSP_DELAY_1 = 1,
	MSP_DELAY_2 = 2,
	MSP_DELAY_3 = 3
};

/* Configurations of clocks (transmit, receive or sample rate generator)
-------------------------------------------------------------------------*/
enum {
	MSP_FALLING_EDGE = 0,
	MSP_RISING_EDGE = 1,
};

enum {
	MSP_HWS_NO_SWAP = 0,
	MSP_HWS_BYTE_SWAP_IN_WORD = 1,
	MSP_HWS_BYTE_SWAP_IN_EACH_HALF_WORD = 2,
	MSP_HWS_HALF_WORD_SWAP_IN_WORD = 3
};
enum {
	MSP_COMPRESS_MODE_LINEAR = 0,
	MSP_COMPRESS_MODE_MU_LAW = 2,
	MSP_COMPRESS_MODE_A_LAW = 3
};
enum {
	MSP_SPI_CLOCK_MODE_NON_SPI = 0,
	MSP_SPI_CLOCK_MODE_ZERO_DELAY = 2,
	MSP_SPI_CLOCK_MODE_HALF_CYCLE_DELAY = 3
};
enum {
	MSP_SPI_BURST_MODE_DISABLE = 0,
	MSP_SPI_BURST_MODE_ENABLE = 1
};
enum {
	MSP_EXPAND_MODE_LINEAR = 0,
	MSP_EXPAND_MODE_LINEAR_SIGNED = 1,
	MSP_EXPAND_MODE_MU_LAW = 2,
	MSP_EXPAND_MODE_A_LAW = 3
};
/* Protocol dependant parameters list */
#define RX_ENABLE_MASK         0x00000001
#define RX_FIFO_ENABLE_MASK    0x00000002
#define RX_FRAME_SYNC_MASK     0x00000004
#define DIRECT_COMPANDING_MASK 0x00000008
#define RX_SYNC_SEL_MASK       0x00000010
#define RX_CLK_POL_MASK        0x00000020
#define RX_CLK_SEL_MASK        0x00000040
#define LOOPBACK_MASK          0x00000080
#define TX_ENABLE_MASK         0x00000100
#define TX_FIFO_ENABLE_MASK    0x00000200
#define TX_FRAME_SYNC_MASK     0x00000400
#define TX_MSP_TDR_TSR         0x00000800
#define TX_SYNC_SEL_MASK       0x00001800
#define TX_CLK_POL_MASK        0x00002000
#define TX_CLK_SEL_MASK        0x00004000
#define TX_EXTRA_DELAY_MASK    0x00008000
#define SRG_ENABLE_MASK        0x00010000
#define SRG_CLK_POL_MASK       0x00020000
#define SRG_CLK_SEL_MASK       0x000C0000
#define FRAME_GEN_EN_MASK      0x00100000
#define SPI_CLK_MODE_MASK      0x00600000
#define SPI_BURST_MODE_MASK    0x00800000

#define RXEN_BIT		0
#define RFFEN_BIT		1
#define RFSPOL_BIT		2
#define DCM_BIT		3
#define RFSSEL_BIT		4
#define RCKPOL_BIT		5
#define RCKSEL_BIT		6
#define LBM_BIT		7
#define TXEN_BIT		8
#define TFFEN_BIT		9
#define TFSPOL_BIT		10
#define TFSSEL_BIT		11
#define TCKPOL_BIT		13
#define TCKSEL_BIT		14
#define TXDDL_BIT		15
#define SGEN_BIT		16
#define SCKPOL_BIT		17
#define SCKSEL_BIT		18
#define FGEN_BIT		20
#define SPICKM_BIT		21
#define TBSWAP_BIT      28

#define msp_rx_clkpol_bit(n)     ((n & 1) << RCKPOL_BIT)
#define msp_tx_clkpol_bit(n)     ((n & 1) << TCKPOL_BIT)
#define msp_spi_clk_mode_bits(n) ((n & 3) << SPICKM_BIT)

/* Use this to clear the clock mode bits to non-spi */
#define MSP_NON_SPI_CLK_MASK 0x00600000

#define P1ELEN_BIT		0
#define P1FLEN_BIT		3
#define DTYP_BIT		10
#define ENDN_BIT		12
#define DDLY_BIT		13
#define FSIG_BIT		15
#define P2ELEN_BIT		16
#define P2FLEN_BIT		19
#define P2SM_BIT		26
#define P2EN_BIT		27
#define FRAME_SYNC_BIT		15

#define msp_p1_elem_len_bits(n) (n & 0x00000007)
#define msp_p2_elem_len_bits(n)  (((n) << P2ELEN_BIT) & 0x00070000)
#define msp_p1_frame_len_bits(n) (((n) << P1FLEN_BIT) & 0x00000378)
#define msp_p2_frame_len_bits(n) (((n) << P2FLEN_BIT) & 0x03780000)
#define msp_data_delay_bits(n)   (((n) << DDLY_BIT) & 0x00003000)
#define msp_data_type_bits(n)    (((n) << DTYP_BIT) & 0x00000600)
#define msp_p2_start_mode_bit(n) ((n << P2SM_BIT) & 0x04000000)
#define msp_p2_enable_bit(n)     ((n << P2EN_BIT) & 0x08000000)
#define msp_set_endiannes_bit(n) ((n << ENDN_BIT) & 0x00001000)
#define msp_frame_sync_pol(n)	((n << TFSPOL_BIT) & 0x00000400)
#define msp_data_word_swap(n)	((n << TBSWAP_BIT) & 0x30000000)
#define msp_set_companding_mode(n) ((n << DTYP_BIT) & 0x00000c00)
#define msp_set_frame_sync_ignore(n) ((n << FRAME_SYNC_BIT) & 0x00008000)

/* Flag register
--------------------*/
#define RX_BUSY       0x00000001
#define RX_FIFO_EMPTY 0x00000002
#define RX_FIFO_FULL  0x00000004
#define TX_BUSY       0x00000008
#define TX_FIFO_EMPTY 0x00000010
#define TX_FIFO_FULL  0x00000020

#define RBUSY_BIT		0
#define RFE_BIT			1
#define RFU_BIT			2
#define TBUSY_BIT		3
#define TFE_BIT			4
#define TFU_BIT			5

/* Multichannel control register
---------------------------------*/
#define RMCEN_BIT		0
#define RMCSF_BIT		1
#define RCMPM_BIT		3
#define TMCEN_BIT		5
#define TNCSF_BIT		6

/* Sample rate generator register
------------------------------------*/
#define SCKDIV_BIT		0
#define FRWID_BIT		10
#define FRPER_BIT		16

#define SCK_DIV_MASK 0x0000003FF
#define frame_width_bits(n) (((n) << FRWID_BIT)  & 0x0000FC00)
#define frame_period_bits(n) (((n) << FRPER_BIT) & 0x1FFF0000)

/* DMA controller register
---------------------------*/
#define RX_DMA_ENABLE 0x00000001
#define TX_DMA_ENABLE 0x00000002

#define RDMAE_BIT		0
#define TDMAE_BIT		1

/*Interrupt Register
-----------------------------------------*/
#define RECEIVE_SERVICE_INT         0x00000001
#define RECEIVE_OVERRUN_ERROR_INT   0x00000002
#define RECEIVE_FRAME_SYNC_ERR_INT  0x00000004
#define RECEIVE_FRAME_SYNC_INT      0x00000008
#define TRANSMIT_SERVICE_INT        0x00000010
#define TRANSMIT_UNDERRUN_ERR_INT   0x00000020
#define TRANSMIT_FRAME_SYNC_ERR_INT 0x00000040
#define TRANSMIT_FRAME_SYNC_INT     0x00000080
#define ALL_INT                     0x000000ff

/* Protocol configuration values
* I2S: Single phase, 16 bits, 2 words per frame
-----------------------------------------------*/
#define I2S_PROTOCOL_DESC				\
{							\
	MSP_SINGLE_PHASE,				\
	MSP_SINGLE_PHASE,				\
	MSP_PHASE2_START_MODE_IMEDIATE, 		\
	MSP_PHASE2_START_MODE_IMEDIATE, 		\
	MSP_BTF_MS_BIT_FIRST,				\
	MSP_BTF_MS_BIT_FIRST,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_ELEM_LENGTH_32,				\
	MSP_ELEM_LENGTH_32,				\
	MSP_ELEM_LENGTH_32,				\
	MSP_ELEM_LENGTH_32,				\
	MSP_DELAY_1,					\
	MSP_DELAY_1,					\
	MSP_RISING_EDGE,				\
	MSP_RISING_EDGE,				\
	MSP_FRAME_SYNC_POL_ACTIVE_HIGH,			\
	MSP_FRAME_SYNC_POL_ACTIVE_HIGH,			\
	MSP_HWS_NO_SWAP,				\
	MSP_HWS_NO_SWAP,				\
	MSP_COMPRESS_MODE_LINEAR,			\
	MSP_EXPAND_MODE_LINEAR,				\
	MSP_SPI_CLOCK_MODE_NON_SPI,			\
	MSP_SPI_BURST_MODE_DISABLE,			\
	MSP_FRAME_SYNC_IGNORE,			\
	31,						\
	15,						\
	32,						\
}
#define PCM_PROTOCOL_DESC				\
{							\
	MSP_DUAL_PHASE,				\
	MSP_DUAL_PHASE,				\
	MSP_PHASE2_START_MODE_FRAME_SYNC, 		\
	MSP_PHASE2_START_MODE_FRAME_SYNC, 		\
	MSP_BTF_MS_BIT_FIRST,				\
	MSP_BTF_MS_BIT_FIRST,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_ELEM_LENGTH_16,				\
	MSP_ELEM_LENGTH_16,				\
	MSP_ELEM_LENGTH_16,				\
	MSP_ELEM_LENGTH_16,				\
	MSP_DELAY_0,					\
	MSP_DELAY_0,					\
	MSP_FALLING_EDGE,				\
	MSP_FALLING_EDGE,				\
	MSP_FRAME_SYNC_POL_ACTIVE_HIGH,			\
	MSP_FRAME_SYNC_POL_ACTIVE_HIGH,			\
	MSP_HWS_NO_SWAP,				\
	MSP_HWS_NO_SWAP,				\
	MSP_COMPRESS_MODE_LINEAR,			\
	MSP_EXPAND_MODE_LINEAR,				\
	MSP_SPI_CLOCK_MODE_NON_SPI,			\
	MSP_SPI_BURST_MODE_DISABLE,			\
	MSP_FRAME_SYNC_IGNORE,			\
	255,						\
	0,						\
	256,						\
}
/* Companded PCM: Single phase, 8 bits, 1 word per frame
--------------------------------------------------------*/
#define PCM_COMPAND_PROTOCOL_DESC				\
{							\
	MSP_SINGLE_PHASE,				\
	MSP_SINGLE_PHASE,				\
	MSP_PHASE2_START_MODE_FRAME_SYNC, 		\
	MSP_PHASE2_START_MODE_FRAME_SYNC, 		\
	MSP_BTF_MS_BIT_FIRST,				\
	MSP_BTF_MS_BIT_FIRST,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_ELEM_LENGTH_8,				\
	MSP_ELEM_LENGTH_8,				\
	MSP_ELEM_LENGTH_8,				\
	MSP_ELEM_LENGTH_8,				\
	MSP_DELAY_0,					\
	MSP_DELAY_0,					\
	MSP_FALLING_EDGE,				\
	MSP_RISING_EDGE,				\
	MSP_FRAME_SYNC_POL_ACTIVE_HIGH,			\
	MSP_FRAME_SYNC_POL_ACTIVE_HIGH,			\
	MSP_HWS_NO_SWAP,				\
	MSP_HWS_NO_SWAP,				\
	MSP_COMPRESS_MODE_LINEAR,			\
	MSP_EXPAND_MODE_LINEAR,				\
	MSP_SPI_CLOCK_MODE_NON_SPI,			\
	MSP_SPI_BURST_MODE_DISABLE,			\
	MSP_FRAME_SYNC_IGNORE,			\
	255,						\
	0,						\
	256,						\
}

/* AC97: Double phase, 1 element of 16 bits during first phase,
* 12 elements of 20 bits in second phase.
--------------------------------------------------------------*/
#define AC97_PROTOCOL_DESC				\
{							\
	MSP_DUAL_PHASE,				\
	MSP_DUAL_PHASE,				\
	MSP_PHASE2_START_MODE_FRAME_SYNC, 		\
	MSP_PHASE2_START_MODE_FRAME_SYNC, 		\
	MSP_BTF_MS_BIT_FIRST,				\
	MSP_BTF_MS_BIT_FIRST,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_12,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_12,				\
	MSP_ELEM_LENGTH_16,				\
	MSP_ELEM_LENGTH_20,				\
	MSP_ELEM_LENGTH_16,				\
	MSP_ELEM_LENGTH_20,				\
	MSP_DELAY_1,					\
	MSP_DELAY_1,					\
	MSP_FALLING_EDGE,				\
	MSP_RISING_EDGE,				\
	MSP_FRAME_SYNC_POL_ACTIVE_HIGH,			\
	MSP_FRAME_SYNC_POL_ACTIVE_HIGH,			\
	MSP_HWS_NO_SWAP,				\
	MSP_HWS_NO_SWAP,				\
	MSP_COMPRESS_MODE_LINEAR,			\
	MSP_EXPAND_MODE_LINEAR,				\
	MSP_SPI_CLOCK_MODE_NON_SPI,			\
	MSP_SPI_BURST_MODE_DISABLE,			\
	MSP_FRAME_SYNC_IGNORE,			\
	255,						\
	0,						\
	256,						\
}

#define SPI_MASTER_PROTOCOL_DESC				\
{							\
	MSP_SINGLE_PHASE,				\
	MSP_SINGLE_PHASE,				\
	MSP_PHASE2_START_MODE_FRAME_SYNC, 		\
	MSP_PHASE2_START_MODE_FRAME_SYNC, 		\
	MSP_BTF_MS_BIT_FIRST,				\
	MSP_BTF_MS_BIT_FIRST,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_ELEM_LENGTH_8,				\
	MSP_ELEM_LENGTH_8,				\
	MSP_ELEM_LENGTH_8,				\
	MSP_ELEM_LENGTH_8,				\
	MSP_DELAY_1,					\
	MSP_DELAY_1,					\
	MSP_RISING_EDGE,				\
	MSP_FALLING_EDGE,				\
	MSP_FRAME_SYNC_POL_ACTIVE_HIGH,			\
	MSP_FRAME_SYNC_POL_ACTIVE_HIGH,			\
	MSP_HWS_NO_SWAP,				\
	MSP_HWS_NO_SWAP,				\
	MSP_COMPRESS_MODE_LINEAR,			\
	MSP_EXPAND_MODE_LINEAR,				\
	MSP_SPI_CLOCK_MODE_NON_SPI,			\
	MSP_SPI_BURST_MODE_DISABLE,			\
	MSP_FRAME_SYNC_IGNORE,			\
	255,						\
	0,						\
	256,						\
}

#define SPI_SLAVE_PROTOCOL_DESC				\
{							\
	MSP_SINGLE_PHASE,				\
	MSP_SINGLE_PHASE,				\
	MSP_PHASE2_START_MODE_FRAME_SYNC, 		\
	MSP_PHASE2_START_MODE_FRAME_SYNC, 		\
	MSP_BTF_MS_BIT_FIRST,				\
	MSP_BTF_MS_BIT_FIRST,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_FRAME_LENGTH_1,				\
	MSP_ELEM_LENGTH_8,				\
	MSP_ELEM_LENGTH_8,				\
	MSP_ELEM_LENGTH_8,				\
	MSP_ELEM_LENGTH_8,				\
	MSP_DELAY_1,					\
	MSP_DELAY_1,					\
	MSP_RISING_EDGE,				\
	MSP_FALLING_EDGE,				\
	MSP_FRAME_SYNC_POL_ACTIVE_HIGH,			\
	MSP_FRAME_SYNC_POL_ACTIVE_HIGH,			\
	MSP_HWS_NO_SWAP,				\
	MSP_HWS_NO_SWAP,				\
	MSP_COMPRESS_MODE_LINEAR,			\
	MSP_EXPAND_MODE_LINEAR,				\
	MSP_SPI_CLOCK_MODE_NON_SPI,			\
	MSP_SPI_BURST_MODE_DISABLE,			\
	MSP_FRAME_SYNC_IGNORE,			\
	255,						\
	0,						\
	256,						\
}

#define MSP_FRAME_PERIOD_IN_MONO_MODE 256
#define MSP_FRAME_PERIOD_IN_STEREO_MODE 32
#define MSP_FRAME_WIDTH_IN_STEREO_MODE 16

/* No of registers to backup during
 * suspend resume */
#define MAX_MSP_BACKUP_REGS 36

typedef enum {
	MSP_0_I2S_CONTROLLER = 1,
	MSP_1_I2S_CONTROLLER,
	MSP_2_I2S_CONTROLLER,
} t_i2s_controller;

/**
 * struct msp_struct - Main msp controller data structure per MSP.
 * @work_mode: Mode i.e dma, polling or interrupt.
 * @id: Controller id like MSP1 or MSP2 etc.
 * @msp_io_error: To indicate error while transferring.
 * @registers: MSP's register base address.
 * @actual_data_size: Data size in which data needs to send or receive.
 * @irq:  MSP's irq number.
 * @i2s_cont: MSP's Controller's structure pointer created per MSP.
 * @gpio_alt_func: Gpio Alt function id of particular msp.
 * @lock: semaphore lock acquired while configuring msp.
 * @pipe_params_rx: Receive dma pipe structure.
 * @pipe_params_tx: Transmit dma pipe structure.
 * @msp_tx_dma_addr: Transmit Base Address of msp for DMA.
 * @msp_rx_dma_addr: Receive Base Address of msp for DMA.
 * @msp_base_addr: Physical Base Address of msp.
 * @tx_pipeid: Txpipeid for DMA.
 * @rx_pipeid: Rxpipeid for DMA.
 * @msp_state: Current state of msp.
 * @read: Function pointer for read, u8_msp_read,u16_msp_read,u32_msp_read.
 * @write: Function pointer for write, u8_msp_write,u16_msp_write,u32_msp_write.
 * @transfer: Function pointer for type of transfer i.e dma,polling or interrupt
 * @xfer_data: MSP's transfer data structure. Contains info about current xfer.
 * @plat_init: MSP's initialization function.
 * @plat_exit: MSP's Exit function.
 * @notify_timer: Timer used in Polling mode to prevent hang.
 * @polling_flag: Flag used in error handling while polling.
 * @def_elem_len: Flag indicates whether default elem len to be used in
 * protocol_desc or not.
 * @vape_opp_constraint: 1 if constraint is applied to have vape at 100OPP; 0 otherwise
 *
 * Main Msp private data structure to be used to store various info of a
 * particular MSP.Longer description
 */
struct msp_struct {
	int work_mode;
	t_i2s_controller id;
	int msp_io_error;
	void __iomem *registers;
	enum msp_data_size actual_data_size;
	int irq;
	struct i2s_controller *i2s_cont;
	u32 gpio_alt_func;
	struct semaphore lock;
	struct stm_dma_pipe_info pipe_params_rx;
	struct stm_dma_pipe_info pipe_params_tx;
	u32 msp_tx_dma_addr;
	u32 msp_rx_dma_addr;
	u32 msp_base_addr;
	dmach_t tx_pipeid;
	dmach_t rx_pipeid;
	t_msp_state msp_state;
	void (*read) (struct trans_data *xfer_data);
	void (*write) (struct trans_data *xfer_data);
	int (*transfer) (struct msp_struct *msp, struct i2s_message *message);
	struct trans_data xfer_data;
	int (*plat_init) (u32);
	int (*plat_exit) (u32);
	struct timer_list notify_timer;
	int polling_flag;
	int def_elem_len;
	struct clk *clk;
	unsigned int direction;
	int users;
	int loopback_enable;
	u32 backup_regs[MAX_MSP_BACKUP_REGS];
	int vape_opp_constraint;
};

/**
 * struct msp_i2s_platform_datat - Main msp controller platform data structure.
 * @id: Controller id like MSP1 or MSP2 etc.
 * @gpio_alt_func: Gpio Alt function id of particular msp.
 * @msp_tx_dma_addr: Transmit Base Address of msp for DMA.
 * @msp_rx_dma_addr: Receive Base Address of msp for DMA.
 * @msp_base_addr: Physical Base Address of msp.
 * msp_i2s_init@: MSP's initialization function.
 * @msp_i2s_exit: MSP's Exit function.
 * @backup_regs: used for backup registers during suspend resume.
 *
 * Platform data structure passed by devices.c file.
 */
struct msp_i2s_platform_data {
	t_i2s_controller id;
	u32 gpio_alt_func;
	u32 msp_tx_dma_addr;
	u32 msp_rx_dma_addr;
	u32 msp_base_addr;
	int (*msp_i2s_init) (u32);
	int (*msp_i2s_exit) (u32);
};

#endif
