/*****************************************************************************
 *  copyright STMicroelectronics, 2007.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef STM_SPI023_HEADER
#define STM_SPI023_HEADER

#define DRIVE_TX			(0)
#define DO_NOT_DRIVE_TX			(1)


#define	SPI_FIFOSIZE  (32)
#define SPI_FIFOWIDTH (32)
#define SPI_PERIPHID0 (0x23)
#define SPI_PERIPHID1 (0x00)
#define SPI_PERIPHID2 (0x08)
#define SPI_PERIPHID3 (0x01)
#define SPI_PCELLID0  (0x0D)
#define SPI_PCELLID1  (0xF0)
#define SPI_PCELLID2  (0x05)
#define SPI_PCELLID3  (0xB1)

/*#######################################################################
  Macros to access SSP Registers with their offsets
#########################################################################
*/
#define SPI_CR0(r)	(r + 0x000)
#define SPI_CR1(r)	(r + 0x004)
#define SPI_DR(r)	(r + 0x008)
#define SPI_SR(r)	(r + 0x00C)
#define SPI_CPSR(r)	(r + 0x010)
#define SPI_IMSC(r)	(r + 0x014)
#define SPI_RIS(r)	(r + 0x018)
#define SPI_MIS(r)	(r + 0x01C)
#define SPI_ICR(r)	(r + 0x020)
#define SPI_DMACR(r)	(r + 0x024)


#define SPI_PID0(r)	(r + 0xFE0)
#define SPI_PID1(r)	(r + 0xFE4)
#define SPI_PID2(r)	(r + 0xFE8)
#define SPI_PID3(r)	(r + 0xFEC)

#define SPI_CID0(r)	(r + 0xFF0)
#define SPI_CID1(r)	(r + 0xFF4)
#define SPI_CID2(r)	(r + 0xFF8)
#define SPI_CID3(r)	(r + 0xFFC)

/*#######################################################################
  SSP Control Register 0  - SPI_CR0
#########################################################################
*/
#define SPI_CR0_MASK_DSS		((u32)(0x1FUL << 0))
#define SPI_CR0_MASK_SPO		((u32)(0x1UL << 6))
#define SPI_CR0_MASK_SPH		((u32)(0x1UL << 7))
#define SPI_CR0_MASK_SCR		((u32)(0xFFUL << 8))

/*#######################################################################
  SSP Control Register 0  - SPI_CR1
#########################################################################
*/
#define SPI_CR1_MASK_LBM		((u32)(0x1UL << 0))
#define SPI_CR1_MASK_SSE		((u32)(0x1UL << 1))
#define SPI_CR1_MASK_MS			((u32)(0x1UL << 2))
#define SPI_CR1_MASK_SOD		((u32)(0x1UL << 3))
#define SPI_CR1_MASK_RENDN		((u32)(0x1UL << 4))
#define SPI_CR1_MASK_TENDN		((u32)(0x1UL << 5))
#define SPI_CR1_MASK_MWAIT		((u32)(0x1UL << 6))
#define SPI_CR1_MASK_RXIFLSEL		((u32)(0x7UL << 7))
#define SPI_CR1_MASK_TXIFLSEL		((u32)(0x7UL << 10))

/*#######################################################################
  SSP Data Register  - SPI_dr
#########################################################################
*/

#define SPI_DR_MASK_DATA		0xFFFFFFFF

/*#######################################################################
  SSP Status Register  - SPI_sr
#########################################################################
*/

#define SPI_SR_MASK_TFE			((u32)(0x1UL << 0))
#define SPI_SR_MASK_TNF			((u32)(0x1UL << 1))
#define SPI_SR_MASK_RNE			((u32)(0x1UL << 2))
#define SPI_SR_MASK_RFF 		((u32)(0x1UL << 3))
#define SPI_SR_MASK_BSY			((u32)(0x1UL << 4))

/*#######################################################################
  SSP Clock Prescale Register  - SPI_cpsr
#########################################################################
*/
#define SPI_CPSR_MASK_CPSDVSR	 	((u32)(0xFFUL << 0))	/*(0xFF << 0)*/

/*#######################################################################
  SSP Interrupt Mask Set/Clear Register  - SPI_imsc
#########################################################################
*/
#define SPI_IMSC_MASK_RORIM		((u32)(0x1UL << 0))
#define SPI_IMSC_MASK_RTIM		((u32)(0x1UL << 1))
#define SPI_IMSC_MASK_RXIM		((u32)(0x1UL << 2))
#define SPI_IMSC_MASK_TXIM		((u32)(0x1UL << 3))

/*#######################################################################
  SSP Raw Interrupt Status Register  -  SPI_ris
#########################################################################
*/
#define SPI_RIS_MASK_RORRIS		((u32)(0x1UL << 0))
#define SPI_RIS_MASK_RTRIS		((u32)(0x1UL << 1))
#define SPI_RIS_MASK_RXRIS		((u32)(0x1UL << 2))
#define SPI_RIS_MASK_TXRIS		((u32)(0x1UL << 3))

/*#######################################################################
  SSP Masked Interrupt Status Register  -  SPI_mis
#########################################################################
*/

#define SPI_MIS_MASK_RORMIS		((u32)(0x1UL << 0))
#define SPI_MIS_MASK_RTMIS		((u32)(0x1UL << 1))
#define SPI_MIS_MASK_RXMIS		((u32)(0x1UL << 2))
#define SPI_MIS_MASK_TXMIS		((u32)(0x1UL << 3))

/*#######################################################################
  SSP Interrupt Clear Register - SPI_icr
#########################################################################
*/
#define SPI_ICR_MASK_RORIC		((u32)(0x1UL << 0))
#define SPI_ICR_MASK_RTIC		((u32)(0x1UL << 1))

/*#######################################################################
  SSP DMA Control Register  - SPI_dmacr
#########################################################################
*/
#define SPI_DMACR_MASK_RXDMAE		((u32)(0x1UL << 0))
#define SPI_DMACR_MASK_TXDMAE		((u32)(0x1UL << 1))

/*#######################################################################
  SSP State - Whether Enabled or Disabled
#########################################################################
*/
#define SPI_DISABLED 			(0)
#define SPI_ENABLED 			(1)

/*#######################################################################
  SSP DMA State - Whether DMA Enabled or Disabled
#########################################################################
*/
#define SPI_DMA_DISABLED 			(0)
#define SPI_DMA_ENABLED 			(1)

/*#######################################################################
  SSP Clock Defaults
#########################################################################
*/

#define STM_SPI_DEFAULT_CLKRATE 0x2
#define STM_SPI_DEFAULT_PRESCALE 0x40

/*#######################################################################
  SSP Clock Parameter ranges
#########################################################################
*/
#define MIN_CPSDVR 0x02
#define MAX_CPSDVR 0xFE
#define MIN_SCR 0x00
#define MAX_SCR 0xFF
/*#define STM_SPI_CLOCK_FREQ 24000000*/
#define STM_SPI_CLOCK_FREQ 48000000

/*#######################################################################
  SSP Interrupt related Macros
#########################################################################
*/
#define DEFAULT_SPI_REG_IMSC  0x0UL
#define DISABLE_ALL_SPI_INTERRUPTS DEFAULT_SPI_REG_IMSC
#define ENABLE_ALL_SPI_INTERRUPTS (~DEFAULT_SPI_REG_IMSC)

#define CLEAR_ALL_SPI_INTERRUPTS  0x3

/*#######################################################################
    Default SSP Register Values
#########################################################################
 */
#define DEFAULT_SPI_REG_CR0	( \
	GEN_MASK_BITS(SPI_DATA_BITS_12, SPI_CR0_MASK_DSS, 0)	|	\
	GEN_MASK_BITS(SPI_CLK_POL_IDLE_LOW, SPI_CR0_MASK_SPO, 6) | \
	GEN_MASK_BITS(SPI_CLK_HALF_CYCLE_DELAY, SPI_CR0_MASK_SPH, 7)	|\
	GEN_MASK_BITS(STM_SPI_DEFAULT_CLKRATE, SPI_CR0_MASK_SCR, 8)	\
	)

#define DEFAULT_SPI_REG_CR1	( \
	GEN_MASK_BITS(LOOPBACK_DISABLED, SPI_CR1_MASK_LBM, 0) | \
	GEN_MASK_BITS(SPI_DISABLED, SPI_CR1_MASK_SSE, 1) | \
	GEN_MASK_BITS(SPI_MASTER, SPI_CR1_MASK_MS, 2) | \
	GEN_MASK_BITS(DO_NOT_DRIVE_TX, SPI_CR1_MASK_SOD, 3) | \
	GEN_MASK_BITS(SPI_FIFO_MSB, SPI_CR1_MASK_RENDN, 4) | \
	GEN_MASK_BITS(SPI_FIFO_MSB, SPI_CR1_MASK_TENDN, 5) | \
	GEN_MASK_BITS(SPI_RX_1_OR_MORE_ELEM, SPI_CR1_MASK_RXIFLSEL, 7) | \
	GEN_MASK_BITS(SPI_TX_1_OR_MORE_EMPTY_LOC, SPI_CR1_MASK_TXIFLSEL, 10)  \
	)

#define DEFAULT_SPI_REG_CPSR  (\
	GEN_MASK_BITS(STM_SPI_DEFAULT_PRESCALE, SPI_CPSR_MASK_CPSDVSR, 0) \
	)

#define DEFAULT_SPI_REG_DMACR  (\
		GEN_MASK_BITS(SPI_DMA_DISABLED, SPI_DMACR_MASK_RXDMAE, 0) | \
		GEN_MASK_BITS(SPI_DMA_DISABLED, SPI_DMACR_MASK_TXDMAE, 1) \
	)

#define STM_SPICTLR_CLOCK_FREQ 48000000

struct spi023_regs{
	u32 cr0;
	u32 cr1;
	u32 dmacr;
	u32 cpsr;
};


/**
 *  * Number of bits in one data element
 *   */
typedef enum {
	SPI_DATA_BITS_4 = 0x03, SPI_DATA_BITS_5, SPI_DATA_BITS_6,
	SPI_DATA_BITS_7, SPI_DATA_BITS_8, SPI_DATA_BITS_9,
	SPI_DATA_BITS_10, SPI_DATA_BITS_11, SPI_DATA_BITS_12,
	SPI_DATA_BITS_13, SPI_DATA_BITS_14, SPI_DATA_BITS_15,
	SPI_DATA_BITS_16, SPI_DATA_BITS_17, SPI_DATA_BITS_18,
	SPI_DATA_BITS_19, SPI_DATA_BITS_20, SPI_DATA_BITS_21,
	SPI_DATA_BITS_22, SPI_DATA_BITS_23, SPI_DATA_BITS_24,
	SPI_DATA_BITS_25, SPI_DATA_BITS_26, SPI_DATA_BITS_27,
	SPI_DATA_BITS_28, SPI_DATA_BITS_29, SPI_DATA_BITS_30,
	SPI_DATA_BITS_31, SPI_DATA_BITS_32
} t_spi_data_size;

/**
 * Rx FIFO watermark level which triggers IT: Interrupt fires when _N_ or more
 * elements in RX FIFO.
 * */
typedef enum {
	SPI_RX_1_OR_MORE_ELEM,
	SPI_RX_4_OR_MORE_ELEM,
	SPI_RX_8_OR_MORE_ELEM,
	SPI_RX_16_OR_MORE_ELEM
} t_spi_rx_level_trig;

/**
 * Transmit FIFO watermark level which triggers (IT Interrupt fires
 * when _N_ or more empty locations in TX FIFO)
 * */
typedef enum {
	SPI_TX_1_OR_MORE_EMPTY_LOC,
	SPI_TX_4_OR_MORE_EMPTY_LOC,
	SPI_TX_8_OR_MORE_EMPTY_LOC,
	SPI_TX_16_OR_MORE_EMPTY_LOC
} t_spi_tx_level_trig;
/**
 *  * Clock parameters, to set SSP clock at a desired freq
 *   */
typedef struct {
	u8 cpsdvsr;             /* value from 2 to 254 (even only!) */
	u8 scr;         /* value from 0 to 255 */
} t_spi_clock_params;


struct spi023_controller {
	t_spi_loopback lbm;
	t_spi_interface iface;
	t_spi_hierarchy hierarchy;
	t_spi_fifo_endian endian_rx;
	t_spi_fifo_endian endian_tx;
	t_spi_mode com_mode;
	t_spi_data_size data_size;
	t_spi_rx_level_trig rx_lev_trig;
	t_spi_tx_level_trig tx_lev_trig;
	int slave_tx_disable;
	t_spi_clock_params clk_freq;
	union {
		struct motorola_spi_proto_params moto;
		struct microwire_proto_params micro;
	} proto_params;

	u32 freq;
	void (*cs_control) (u32 control);
	t_dma_xfer_type dma_xfer_type;
	struct nmdkspi_dma *dma_config;
};
#endif
