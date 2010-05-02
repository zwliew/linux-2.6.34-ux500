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

#ifndef STM_SSP_SPI_HEADER
#define STM_SSP_SPI_HEADER

#define DRIVE_TX			(0)
#define DO_NOT_DRIVE_TX			(1)


#define	SSP_FIFOSIZE  (32)
#define SSP_FIFOWIDTH (32)
#define SSP_PERIPHID0 (0x22)
#define SSP_PERIPHID1 (0x00)
#define SSP_PERIPHID2 (0x08)
#define SSP_PERIPHID3 (0x01)
#define SSP_PCELLID0  (0x0D)
#define SSP_PCELLID1  (0xF0)
#define SSP_PCELLID2  (0x05)
#define SSP_PCELLID3  (0xB1)

/*#######################################################################
  Macros to access SSP Registers with their offsets
#########################################################################
*/
#define SSP_CR0(r)	(r + 0x000)
#define SSP_CR1(r)	(r + 0x004)
#define SSP_DR(r)	(r + 0x008)
#define SSP_SR(r)	(r + 0x00C)
#define SSP_CPSR(r)	(r + 0x010)
#define SSP_IMSC(r)	(r + 0x014)
#define SSP_RIS(r)	(r + 0x018)
#define SSP_MIS(r)	(r + 0x01C)
#define SSP_ICR(r)	(r + 0x020)
#define SSP_DMACR(r)	(r + 0x024)
#define SSP_ITCR(r)	(r + 0x080)
#define SSP_ITIP(r)	(r + 0x084)
#define SSP_ITOP(r)	(r + 0x088)
#define SSP_TDR(r)	(r + 0x08C)

#define SSP_PID0(r)	(r + 0xFE0)
#define SSP_PID1(r)	(r + 0xFE4)
#define SSP_PID2(r)	(r + 0xFE8)
#define SSP_PID3(r)	(r + 0xFEC)

#define SSP_CID0(r)	(r + 0xFF0)
#define SSP_CID1(r)	(r + 0xFF4)
#define SSP_CID2(r)	(r + 0xFF8)
#define SSP_CID3(r)	(r + 0xFFC)

/*#######################################################################
  SSP Control Register 0  - SSP_CR0
#########################################################################
*/
#define SSP_CR0_MASK_DSS		((u32)(0x1FUL << 0))
#define SSP_CR0_MASK_HALFDUP		((u32)(0x1UL << 5))
#define SSP_CR0_MASK_SPO		((u32)(0x1UL << 6))
#define SSP_CR0_MASK_SPH		((u32)(0x1UL << 7))
#define SSP_CR0_MASK_SCR		((u32)(0xFFUL << 8))
#define SSP_CR0_MASK_CSS		((u32)(0x1FUL << 16))
#define SSP_CR0_MASK_FRF		((u32)(0x3UL << 21))

/*#######################################################################
  SSP Control Register 0  - SSP_CR1
#########################################################################
*/
#define SSP_CR1_MASK_LBM		((u32)(0x1UL << 0))
#define SSP_CR1_MASK_SSE		((u32)(0x1UL << 1))
#define SSP_CR1_MASK_MS			((u32)(0x1UL << 2))
#define SSP_CR1_MASK_SOD		((u32)(0x1UL << 3))
#define SSP_CR1_MASK_RENDN		((u32)(0x1UL << 4))
#define SSP_CR1_MASK_TENDN		((u32)(0x1UL << 5))
#define SSP_CR1_MASK_MWAIT		((u32)(0x1UL << 6))
#define SSP_CR1_MASK_RXIFLSEL		((u32)(0x7UL << 7))
#define SSP_CR1_MASK_TXIFLSEL		((u32)(0x7UL << 10))

/*#######################################################################
  SSP Data Register  - ssp_dr
#########################################################################
*/

#define SSP_DR_MASK_DATA		0xFFFFFFFF

/*#######################################################################
  SSP Status Register  - ssp_sr
#########################################################################
*/

#define SSP_SR_MASK_TFE		((u32)(0x1UL << 0))	/* Tx FIFO empty */
#define SSP_SR_MASK_TNF		((u32)(0x1UL << 1))	/* Tx FIFO not full */
#define SSP_SR_MASK_RNE		((u32)(0x1UL << 2))	/* Rx FIFO not empty */
#define SSP_SR_MASK_RFF 	((u32)(0x1UL << 3))	/* Rx FIFO full */
#define SSP_SR_MASK_BSY		((u32)(0x1UL << 4))	/* Busy Flag */

/*#######################################################################
  SSP Clock Prescale Register  - ssp_cpsr
#########################################################################
*/
#define SSP_CPSR_MASK_CPSDVSR	 	((u32)(0xFFUL << 0))	/*(0xFF << 0)*/

/*#######################################################################
  SSP Interrupt Mask Set/Clear Register  - ssp_imsc
#########################################################################
*/
#define SSP_IMSC_MASK_RORIM		((u32)(0x1UL << 0))
#define SSP_IMSC_MASK_RTIM		((u32)(0x1UL << 1))
#define SSP_IMSC_MASK_RXIM		((u32)(0x1UL << 2))
#define SSP_IMSC_MASK_TXIM		((u32)(0x1UL << 3))

/*#######################################################################
  SSP Raw Interrupt Status Register  -  ssp_ris
#########################################################################
*/
#define SSP_RIS_MASK_RORRIS		((u32)(0x1UL << 0))
#define SSP_RIS_MASK_RTRIS		((u32)(0x1UL << 1))
#define SSP_RIS_MASK_RXRIS		((u32)(0x1UL << 2))
#define SSP_RIS_MASK_TXRIS		((u32)(0x1UL << 3))

/*#######################################################################
  SSP Masked Interrupt Status Register  -  ssp_mis
#########################################################################
*/

#define SSP_MIS_MASK_RORMIS		((u32)(0x1UL << 0))
#define SSP_MIS_MASK_RTMIS		((u32)(0x1UL << 1))
#define SSP_MIS_MASK_RXMIS		((u32)(0x1UL << 2))
#define SSP_MIS_MASK_TXMIS		((u32)(0x1UL << 3))

/*#######################################################################
  SSP Interrupt Clear Register - ssp_icr
#########################################################################
*/
#define SSP_ICR_MASK_RORIC		((u32)(0x1UL << 0))
#define SSP_ICR_MASK_RTIC		((u32)(0x1UL << 1))

/*#######################################################################
  SSP DMA Control Register  - ssp_dmacr
#########################################################################
*/
#define SSP_DMACR_MASK_RXDMAE		((u32)(0x1UL << 0))
#define SSP_DMACR_MASK_TXDMAE		((u32)(0x1UL << 1))

/*#######################################################################
  SSP Integration Test control Register  - ssp_itcr
#########################################################################
*/
#define SSP_ITCR_MASK_ITEN		((u32)(0x1UL << 0))
#define SSP_ITCR_MASK_TESTFIFO		((u32)(0x1UL << 1))

/*#######################################################################
  SSP Integration Test Input Register  - ssp_itip
#########################################################################
*/
#define ITIP_MASK_SSPRXD		 ((u32)(0x1UL << 0))
#define ITIP_MASK_SSPFSSIN		 ((u32)(0x1UL << 1))
#define ITIP_MASK_SSPCLKIN		 ((u32)(0x1UL << 2))
#define ITIP_MASK_RXDMAC		 ((u32)(0x1UL << 3))
#define ITIP_MASK_TXDMAC		 ((u32)(0x1UL << 4))
#define ITIP_MASK_SSPTXDIN		 ((u32)(0x1UL << 5))

/*#######################################################################
  SSP Integration Test output Register  - ssp_itop
#########################################################################
*/
#define ITOP_MASK_SSPTXD		 ((u32)(0x1UL << 0))
#define ITOP_MASK_SSPFSSOUT		 ((u32)(0x1UL << 1))
#define ITOP_MASK_SSPCLKOUT		 ((u32)(0x1UL << 2))
#define ITOP_MASK_SSPOEn		 ((u32)(0x1UL << 3))
#define ITOP_MASK_SSPCTLOEn		 ((u32)(0x1UL << 4))
#define ITOP_MASK_RORINTR		 ((u32)(0x1UL << 5))
#define ITOP_MASK_RTINTR		 ((u32)(0x1UL << 6))
#define ITOP_MASK_RXINTR		 ((u32)(0x1UL << 7))
#define ITOP_MASK_TXINTR		 ((u32)(0x1UL << 8))
#define ITOP_MASK_INTR			 ((u32)(0x1UL << 9))
#define ITOP_MASK_RXDMABREQ		 ((u32)(0x1UL << 10))
#define ITOP_MASK_RXDMASREQ		 ((u32)(0x1UL << 11))
#define ITOP_MASK_TXDMABREQ		 ((u32)(0x1UL << 12))
#define ITOP_MASK_TXDMASREQ		 ((u32)(0x1UL << 13))

/*#######################################################################
  SSP Test Data Register  - ssp_tdr
#########################################################################
*/
#define TDR_MASK_TESTDATA 		(0xFFFFFFFF)

/*#######################################################################
  SSP State - Whether Enabled or Disabled
#########################################################################
*/
#define SSP_DISABLED 			(0)
#define SSP_ENABLED 			(1)

/*#######################################################################
  SSP DMA State - Whether DMA Enabled or Disabled
#########################################################################
*/
#define SSP_DMA_DISABLED 			(0)
#define SSP_DMA_ENABLED 			(1)

/*#######################################################################
  SSP Clock Defaults
#########################################################################
*/

#define STM_SSP_DEFAULT_CLKRATE 0x2
#define STM_SSP_DEFAULT_PRESCALE 0x40

/*#######################################################################
  SSP Clock Parameter ranges
#########################################################################
*/
#define MIN_CPSDVR 0x02
#define MAX_CPSDVR 0xFE
#define MIN_SCR 0x00
#define MAX_SCR 0xFF
/*#define STM_SSP_CLOCK_FREQ 24000000*/
#define STM_SSP_CLOCK_FREQ 48000000

/*#######################################################################
  SSP Interrupt related Macros
#########################################################################
*/
#define DEFAULT_SSP_REG_IMSC  0x0UL
#define DISABLE_ALL_SSP_INTERRUPTS DEFAULT_SSP_REG_IMSC
#define ENABLE_ALL_SSP_INTERRUPTS (~DEFAULT_SSP_REG_IMSC)

#define CLEAR_ALL_SSP_INTERRUPTS  0x3

/*#######################################################################
  Default SSP Register Values
#########################################################################
*/
#define DEFAULT_SSP_REG_CR0	( \
	GEN_MASK_BITS(SSP_DATA_BITS_12, SSP_CR0_MASK_DSS, 0)	|	\
	GEN_MASK_BITS(MICROWIRE_CHANNEL_FULL_DUPLEX, SSP_CR0_MASK_HALFDUP, 5)|\
	GEN_MASK_BITS(SPI_CLK_POL_IDLE_LOW, SSP_CR0_MASK_SPO, 6) | \
	GEN_MASK_BITS(SPI_CLK_HALF_CYCLE_DELAY, SSP_CR0_MASK_SPH, 7)	|\
	GEN_MASK_BITS(STM_SSP_DEFAULT_CLKRATE, SSP_CR0_MASK_SCR, 8)	|\
	GEN_MASK_BITS(MWLEN_BITS_8, SSP_CR0_MASK_CSS, 16)	|\
	GEN_MASK_BITS(SPI_INTERFACE_MOTOROLA_SPI, SSP_CR0_MASK_FRF, 21)  \
		)

#define DEFAULT_SSP_REG_CR1	( \
	GEN_MASK_BITS(LOOPBACK_DISABLED, SSP_CR1_MASK_LBM, 0) | \
	GEN_MASK_BITS(SSP_DISABLED, SSP_CR1_MASK_SSE, 1) | \
	GEN_MASK_BITS(SPI_MASTER, SSP_CR1_MASK_MS, 2) | \
	GEN_MASK_BITS(DO_NOT_DRIVE_TX, SSP_CR1_MASK_SOD, 3) | \
	GEN_MASK_BITS(SPI_FIFO_MSB, SSP_CR1_MASK_RENDN, 4) | \
	GEN_MASK_BITS(SPI_FIFO_MSB, SSP_CR1_MASK_TENDN, 5) | \
	GEN_MASK_BITS(MWIRE_WAIT_ZERO, SSP_CR1_MASK_MWAIT, 6) |\
	GEN_MASK_BITS(SSP_RX_1_OR_MORE_ELEM, SSP_CR1_MASK_RXIFLSEL, 7) | \
	GEN_MASK_BITS(SSP_TX_1_OR_MORE_EMPTY_LOC, SSP_CR1_MASK_TXIFLSEL, 10)  \
		)

#define DEFAULT_SSP_REG_CPSR  (\
	GEN_MASK_BITS(STM_SSP_DEFAULT_PRESCALE, SSP_CPSR_MASK_CPSDVSR, 0) \
		)

#define DEFAULT_SSP_REG_DMACR  (\
	GEN_MASK_BITS(SSP_DMA_DISABLED, SSP_DMACR_MASK_RXDMAE, 0) | \
	GEN_MASK_BITS(SSP_DMA_DISABLED, SSP_DMACR_MASK_TXDMAE, 1) \
		)

#ifdef __KERNEL__
struct ssp_regs{
	u32 cr0;
	u32 cr1;
	u32 dmacr;
	u32 cpsr;
};


/**
 * Number of bits in one data element
 */
typedef enum {
	SSP_DATA_BITS_4 = 0x03, SSP_DATA_BITS_5, SSP_DATA_BITS_6,
	SSP_DATA_BITS_7, SSP_DATA_BITS_8, SSP_DATA_BITS_9,
	SSP_DATA_BITS_10, SSP_DATA_BITS_11, SSP_DATA_BITS_12,
	SSP_DATA_BITS_13, SSP_DATA_BITS_14, SSP_DATA_BITS_15,
	SSP_DATA_BITS_16, SSP_DATA_BITS_17, SSP_DATA_BITS_18,
	SSP_DATA_BITS_19, SSP_DATA_BITS_20, SSP_DATA_BITS_21,
	SSP_DATA_BITS_22, SSP_DATA_BITS_23, SSP_DATA_BITS_24,
	SSP_DATA_BITS_25, SSP_DATA_BITS_26, SSP_DATA_BITS_27,
	SSP_DATA_BITS_28, SSP_DATA_BITS_29, SSP_DATA_BITS_30,
	SSP_DATA_BITS_31, SSP_DATA_BITS_32
} t_ssp_data_size;

/**
 * Rx FIFO watermark level which triggers IT: Interrupt fires when _N_ or more
 * elements in RX FIFO.
 */
typedef enum {
	SSP_RX_1_OR_MORE_ELEM,
	SSP_RX_4_OR_MORE_ELEM,
	SSP_RX_8_OR_MORE_ELEM,
	SSP_RX_16_OR_MORE_ELEM,
	SSP_RX_32_OR_MORE_ELEM
} t_ssp_rx_level_trig;

/**
 * Tx FIFO watermark level which triggers (IT Interrupt fires
 * when _N_ or more empty locations in TX FIFO)
 */
typedef enum {
	SSP_TX_1_OR_MORE_EMPTY_LOC,
	SSP_TX_4_OR_MORE_EMPTY_LOC,
	SSP_TX_8_OR_MORE_EMPTY_LOC,
	SSP_TX_16_OR_MORE_EMPTY_LOC,
	SSP_TX_32_OR_MORE_EMPTY_LOC
} t_ssp_tx_level_trig;

/**
 * Clock parameters, to set SSP clock at a desired freq
 */
typedef struct {
	u8 cpsdvsr;		/* value from 2 to 254 (even only!) */
	u8 scr;		/* value from 0 to 255 */
} t_ssp_clock_params;

struct ssp_controller {
	t_spi_loopback lbm;
	t_spi_interface iface;
	t_spi_hierarchy hierarchy;
	t_spi_fifo_endian endian_rx;
	t_spi_fifo_endian endian_tx;
	t_spi_mode com_mode;
	t_ssp_data_size data_size;
	t_ssp_rx_level_trig rx_lev_trig;
	t_ssp_tx_level_trig tx_lev_trig;
	int slave_tx_disable;
	t_ssp_clock_params clk_freq;

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
#endif
