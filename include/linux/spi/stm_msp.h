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

#ifndef NOMADIC_MSP_SPI_HEADER
#define NOMADIC_MSP_SPI_HEADER

# define DEFAULT_MSP_CLK 48000000
# define MAX_SCKDIV 1023

#define MSP_FIFO_DEPTH 8

/*#######################################################################
  MSP Controller Register Offsets
#########################################################################
*/
#define MSP_DR(r)	(r + 0x000)
#define MSP_GCR(r)	(r + 0x004)
#define MSP_TCF(r)	(r + 0x008)
#define MSP_RCF(r)	(r + 0x00C)
#define MSP_SRG(r)	(r + 0x010)
#define MSP_FLR(r)	(r + 0x014)
#define MSP_DMACR(r)	(r + 0x018)
#define MSP_IMSC(r)	(r + 0x020)
#define MSP_RIS(r)	(r + 0x024)
#define MSP_MIS(r)	(r + 0x028)
#define MSP_ICR(r)	(r + 0x02C)
#define MSP_MCR(r)	(r + 0x030)
#define MSP_RCV(r)	(r + 0x034)
#define MSP_RCM(r)	(r + 0x038)
#define MSP_TCE0(r)	(r + 0x040)
#define MSP_TCE1(r)	(r + 0x044)
#define MSP_TCE2(r)	(r + 0x048)
#define MSP_TCE3(r)	(r + 0x04C)
#define MSP_RCE0(r)	(r + 0x060)
#define MSP_RCE1(r)	(r + 0x064)
#define MSP_RCE2(r)	(r + 0x068)
#define MSP_RCE3(r)	(r + 0x06C)
#define MSP_PID0(r)	(r + 0xFE0)
#define MSP_PID1(r)	(r + 0xFE4)
#define MSP_PID2(r)	(r + 0xFE8)
#define MSP_PID3(r)	(r + 0xFEC)

/*#######################################################################
  MSP Global Configuration Register - msp_gcr
#########################################################################
*/
#define MSP_GCR_MASK_RXEN		((u32)(0x1UL << 0))
#define MSP_GCR_MASK_RFFEN		((u32)(0x1UL << 1))
#define MSP_GCR_MASK_RFSPOL		((u32)(0x1UL << 2))
#define MSP_GCR_MASK_DCM		((u32)(0x1UL << 3))
#define MSP_GCR_MASK_RFSSEL		((u32)(0x1UL << 4))
#define MSP_GCR_MASK_RCKPOL		((u32)(0x1UL << 5))
#define MSP_GCR_MASK_RCKSEL		((u32)(0x1UL << 6))
#define MSP_GCR_MASK_LBM		((u32)(0x1UL << 7))
#define MSP_GCR_MASK_TXEN		((u32)(0x1UL << 8))
#define MSP_GCR_MASK_TFFEN		((u32)(0x1UL << 9))
#define MSP_GCR_MASK_TFSPOL		((u32)(0x1UL << 10))
#define MSP_GCR_MASK_TFSSEL		((u32)(0x3UL << 11))
#define MSP_GCR_MASK_TCKPOL		((u32)(0x1UL << 13))
#define MSP_GCR_MASK_TCKSEL		((u32)(0x1UL << 14))
#define MSP_GCR_MASK_TXDDL		((u32)(0x1UL << 15))
#define MSP_GCR_MASK_SGEN		((u32)(0x1UL << 16))
#define MSP_GCR_MASK_SCKPOL		((u32)(0x1UL << 17))
#define MSP_GCR_MASK_SCKSEL		((u32)(0x3UL << 18))
#define MSP_GCR_MASK_FGEN		((u32)(0x1UL << 20))
#define MSP_GCR_MASK_SPICKM		((u32)(0x3UL << 21))
#define MSP_GCR_MASK_SPIBME		((u32)(0x1UL << 23))

/*#######################################################################
  MSP Transmit Configuration Register - msp_tcf
#########################################################################
*/
#define MSP_TCF_MASK_TP1ELEN		((u32)(0x7UL << 0))
#define MSP_TCF_MASK_TP1FLEN		((u32)(0x7FUL << 3))
#define MSP_TCF_MASK_TDTYP		((u32)(0x3UL << 10))
#define MSP_TCF_MASK_TENDN		((u32)(0x1UL << 12))
#define MSP_TCF_MASK_TDDLY		((u32)(0x3UL << 13))
#define MSP_TCF_MASK_TFSIG		((u32)(0x1UL << 15))
#define MSP_TCF_MASK_TP2ELEN		((u32)(0x7UL << 16))
#define MSP_TCF_MASK_TP2FLEN		((u32)(0x7FUL << 19))
#define MSP_TCF_MASK_TP2SM		((u32)(0x1UL << 26))
#define MSP_TCF_MASK_TP2EN		((u32)(0x1UL << 27))
#define MSP_TCF_MASK_TBSWAP		((u32)(0x3UL << 28))

/*#######################################################################
  MSP Receive Configuration Register - msp_rcf
#########################################################################
*/
#define MSP_RCF_MASK_RP1ELEN		((u32)(0x7UL << 0))
#define MSP_RCF_MASK_RP1FLEN		((u32)(0x7FUL << 3))
#define MSP_RCF_MASK_RDTYP		((u32)(0x3UL << 10))
#define MSP_RCF_MASK_RENDN		((u32)(0x1UL << 12))
#define MSP_RCF_MASK_RDDLY		((u32)(0x3UL << 13))
#define MSP_RCF_MASK_RFSIG		((u32)(0x1UL << 15))
#define MSP_RCF_MASK_RP2ELEN		((u32)(0x7UL << 16))
#define MSP_RCF_MASK_RP2FLEN		((u32)(0x7FUL << 19))
#define MSP_RCF_MASK_RP2SM		((u32)(0x1UL << 26))
#define MSP_RCF_MASK_RP2EN		((u32)(0x1UL << 27))
#define MSP_RCF_MASK_RBSWAP		((u32)(0x3UL << 28))

/*#######################################################################
  MSP Sample Rate Generator Register - msp_srg
#########################################################################
*/
#define MSP_SRG_MASK_SCKDIV		((u32)(0x3FFUL << 0))
#define MSP_SRG_MASK_FRWID		((u32)(0x3FUL << 10))
#define MSP_SRG_MASK_FRPER		((u32)(0x1FFFUL << 16))

/*#######################################################################
  MSP Flag Register - msp_flr
#########################################################################
*/
#define MSP_FLR_MASK_RBUSY		((u32)(0x1UL << 0))
#define MSP_FLR_MASK_RFE		((u32)(0x1UL << 1))
#define MSP_FLR_MASK_RFU		((u32)(0x1UL << 2))
#define MSP_FLR_MASK_TBUSY		((u32)(0x1UL << 3))
#define MSP_FLR_MASK_TFE		((u32)(0x1UL << 4))
#define MSP_FLR_MASK_TFU		((u32)(0x1UL << 5))

/*#######################################################################
  MSP DMA Control Register - msp_dmacr
#########################################################################
*/
#define MSP_DMACR_MASK_RDMAE		((u32)(0x1UL << 0))
#define MSP_DMACR_MASK_TDMAE		((u32)(0x1UL << 1))

/*#######################################################################
  MSP Interrupt Mask Set/Clear Register - msp_imsc
#########################################################################
*/
#define MSP_IMSC_MASK_RXIM		((u32)(0x1UL << 0))
#define MSP_IMSC_MASK_ROEIM		((u32)(0x1UL << 1))
#define MSP_IMSC_MASK_RSEIM		((u32)(0x1UL << 2))
#define MSP_IMSC_MASK_RFSIM		((u32)(0x1UL << 3))
#define MSP_IMSC_MASK_TXIM		((u32)(0x1UL << 4))
#define MSP_IMSC_MASK_TUEIM		((u32)(0x1UL << 5))
#define MSP_IMSC_MASK_TSEIM		((u32)(0x1UL << 6))
#define MSP_IMSC_MASK_TFSIM		((u32)(0x1UL << 7))
#define MSP_IMSC_MASK_RFOIM		((u32)(0x1UL << 8))
#define MSP_IMSC_MASK_TFOIM		((u32)(0x1UL << 9))

/*#######################################################################
  MSP Raw Interrupt status Register - msp_ris
#########################################################################
*/
#define MSP_RIS_MASK_RXRIS		((u32)(0x1UL << 0))
#define MSP_RIS_MASK_ROERIS		((u32)(0x1UL << 1))
#define MSP_RIS_MASK_RSERIS		((u32)(0x1UL << 2))
#define MSP_RIS_MASK_RFSRIS		((u32)(0x1UL << 3))
#define MSP_RIS_MASK_TXRIS		((u32)(0x1UL << 4))
#define MSP_RIS_MASK_TUERIS		((u32)(0x1UL << 5))
#define MSP_RIS_MASK_TSERIS		((u32)(0x1UL << 6))
#define MSP_RIS_MASK_TFSRIS		((u32)(0x1UL << 7))
#define MSP_RIS_MASK_RFORIS		((u32)(0x1UL << 8))
#define MSP_RIS_MASK_TFORIS		((u32)(0x1UL << 9))

/*#######################################################################
  MSP Masked Interrupt status Register - msp_mis
#########################################################################
*/
#define MSP_MIS_MASK_RXMIS		((u32)(0x1UL << 0))
#define MSP_MIS_MASK_ROEMIS		((u32)(0x1UL << 1))
#define MSP_MIS_MASK_RSEMIS		((u32)(0x1UL << 2))
#define MSP_MIS_MASK_RFSMIS		((u32)(0x1UL << 3))
#define MSP_MIS_MASK_TXMIS		((u32)(0x1UL << 4))
#define MSP_MIS_MASK_TUEMIS		((u32)(0x1UL << 5))
#define MSP_MIS_MASK_TSEMIS		((u32)(0x1UL << 6))
#define MSP_MIS_MASK_TFSMIS		((u32)(0x1UL << 7))
#define MSP_MIS_MASK_RFOMIS		((u32)(0x1UL << 8))
#define MSP_MIS_MASK_TFOMIS		((u32)(0x1UL << 9))

/*#######################################################################
  MSP Interrupt Clear Register - msp_icr
#########################################################################
*/
#define MSP_ICR_MASK_ROEIC		((u32)(0x1UL << 1))
#define MSP_ICR_MASK_RSEIC		((u32)(0x1UL << 2))
#define MSP_ICR_MASK_RFSIC		((u32)(0x1UL << 3))
#define MSP_ICR_MASK_TUEIC		((u32)(0x1UL << 5))
#define MSP_ICR_MASK_TSEIC		((u32)(0x1UL << 6))
#define MSP_ICR_MASK_TFSIC		((u32)(0x1UL << 7))

/*#######################################################################
  MSP Receiver/Transmitter states (Enabled or disabled)
#########################################################################
*/
#define MSP_RECEIVER_DISABLED 0
#define MSP_RECEIVER_ENABLED 1
#define MSP_TRANSMITTER_DISABLED 0
#define MSP_TRANSMITTER_ENABLED 1
/*#######################################################################
  MSP Receiver/Transmitter FIFO constants
#########################################################################
*/
#define MSP_LOOPBACK_DISABLED 0
#define MSP_LOOPBACK_ENABLED 1

#define MSP_TX_FIFO_DISABLED 0
#define MSP_TX_FIFO_ENABLED 1
#define MSP_TX_ENDIANESS_MSB	0
#define MSP_TX_ENDIANESS_LSB	1

#define MSP_RX_FIFO_DISABLED 0
#define MSP_RX_FIFO_ENABLED 1
#define MSP_RX_ENDIANESS_MSB	0
#define MSP_RX_ENDIANESS_LSB	1


/*#######################################################################
  MSP Controller State constants
#########################################################################
*/
#define MSP_IS_SPI_SLAVE 0
#define MSP_IS_SPI_MASTER 1

#define SPI_BURST_MODE_DISABLE 0
#define SPI_BURST_MODE_ENABLE 1

/*#######################################################################
  MSP Phase and Polarity constants
#########################################################################
*/
#define MSP_SPI_PHASE_ZERO_CYCLE_DELAY 0x2
#define MSP_SPI_PHASE_HALF_CYCLE_DELAY 0x3

#define MSP_TX_CLOCK_POL_LOW 0
#define MSP_TX_CLOCK_POL_HIGH 1

/*#######################################################################
  MSP SRG and Frame related constants
#########################################################################
*/
#define MSP_FRAME_GEN_DISABLE 0
#define MSP_FRAME_GEN_ENABLE 1

#define MSP_SAMPLE_RATE_GEN_DISABLE 0
#define MSP_SAMPLE_RATE_GEN_ENABLE 1


#define MSP_CLOCK_INTERNAL 0x0 /*48 MHz*/
#define MSP_CLOCK_EXTERNAL 0x2	/*SRG is derived from MSPSCK*/
/*SRG is derived from MSPSCK pin but is resynchronized on MSPRFS
 * (Receive Frame Sync signal)*/
#define MSP_CLOCK_EXTERNAL_RESYNC 0x3


#define MSP_TRANSMIT_DATA_WITHOUT_DELAY 0
#define MSP_TRANSMIT_DATA_WITH_DELAY 1


/*INT: means frame sync signal provided by frame generator logic in the MSP
EXT: means frame sync signal provided by external pin MSPTFS
*/
#define MSP_TX_FRAME_SYNC_EXT 0x0
#define MSP_TX_FRAME_SYNC_INT 0x2
#define MSP_TX_FRAME_SYNC_INT_CFG 0x3


#define MSP_TX_FRAME_SYNC_POL_HIGH 0
#define MSP_TX_FRAME_SYNC_POL_LOW 1


#define MSP_HANDLE_RX_FRAME_SYNC_PULSE 0
#define MSP_IGNORE_RX_FRAME_SYNC_PULSE 1

#define MSP_RX_NO_DATA_DELAY 0x0
#define MSP_RX_1BIT_DATA_DELAY 0x1
#define MSP_RX_2BIT_DATA_DELAY 0x2
#define MSP_RX_3BIT_DATA_DELAY 0x3

#define MSP_HANDLE_TX_FRAME_SYNC_PULSE 0
#define MSP_IGNORE_TX_FRAME_SYNC_PULSE 1

#define MSP_TX_NO_DATA_DELAY 0x0
#define MSP_TX_1BIT_DATA_DELAY 0x1
#define MSP_TX_2BIT_DATA_DELAY 0x2
#define MSP_TX_3BIT_DATA_DELAY 0x3


/*#######################################################################
  MSP Interrupt related Macros
#########################################################################
*/
#define DISABLE_ALL_MSP_INTERRUPTS  0x0
#define ENABLE_ALL_MSP_INTERRUPTS  0x333
#define CLEAR_ALL_MSP_INTERRUPTS  0xEE

/*#######################################################################
  Default MSP Register Values
#########################################################################
*/

#define DEFAULT_MSP_REG_DMACR 0x00000000

#define DEFAULT_MSP_REG_SRG 0x1FFF0000

#define DEFAULT_MSP_REG_GCR	( \
	GEN_MASK_BITS(MSP_RECEIVER_DISABLED, MSP_GCR_MASK_RXEN, 0)	|\
	GEN_MASK_BITS(MSP_RX_FIFO_ENABLED, MSP_GCR_MASK_RFFEN, 1)	|\
	GEN_MASK_BITS(MSP_LOOPBACK_DISABLED, MSP_GCR_MASK_LBM, 7)	|\
	GEN_MASK_BITS(MSP_TRANSMITTER_DISABLED, MSP_GCR_MASK_TXEN, 8)	|\
	GEN_MASK_BITS(MSP_TX_FIFO_ENABLED, MSP_GCR_MASK_TFFEN, 9)	|\
	GEN_MASK_BITS(MSP_TX_FRAME_SYNC_POL_LOW, MSP_GCR_MASK_TFSPOL, 10)|\
	GEN_MASK_BITS(MSP_TX_FRAME_SYNC_INT, MSP_GCR_MASK_TFSSEL, 11)	|\
	GEN_MASK_BITS(MSP_TX_CLOCK_POL_HIGH, MSP_GCR_MASK_TCKPOL, 13)	|\
	GEN_MASK_BITS(MSP_IS_SPI_MASTER, MSP_GCR_MASK_TCKSEL, 14)	|\
	GEN_MASK_BITS(MSP_TRANSMIT_DATA_WITHOUT_DELAY, MSP_GCR_MASK_TXDDL, 15)|\
	GEN_MASK_BITS(MSP_SAMPLE_RATE_GEN_ENABLE, MSP_GCR_MASK_SGEN, 16)|\
	GEN_MASK_BITS(MSP_CLOCK_INTERNAL, MSP_GCR_MASK_SCKSEL, 18)	|\
	GEN_MASK_BITS(MSP_FRAME_GEN_ENABLE, MSP_GCR_MASK_FGEN, 20)	|\
	GEN_MASK_BITS(MSP_SPI_PHASE_ZERO_CYCLE_DELAY, MSP_GCR_MASK_SPICKM, 21)|\
	GEN_MASK_BITS(SPI_BURST_MODE_DISABLE, MSP_GCR_MASK_SPIBME, 23)\
	)

#define DEFAULT_MSP_REG_RCF	( \
	GEN_MASK_BITS(MSP_DATA_BITS_32, MSP_RCF_MASK_RP1ELEN, 0) | \
	GEN_MASK_BITS(MSP_IGNORE_RX_FRAME_SYNC_PULSE, MSP_RCF_MASK_RFSIG, 15) |\
	GEN_MASK_BITS(MSP_RX_1BIT_DATA_DELAY, MSP_RCF_MASK_RDDLY, 13) | \
	GEN_MASK_BITS(MSP_RX_ENDIANESS_LSB, MSP_RCF_MASK_RENDN, 12)  \
	)

#define DEFAULT_MSP_REG_TCF	( \
	GEN_MASK_BITS(MSP_DATA_BITS_32, MSP_TCF_MASK_TP1ELEN, 0) | \
	GEN_MASK_BITS(MSP_IGNORE_TX_FRAME_SYNC_PULSE, MSP_TCF_MASK_TFSIG, 15) |\
	GEN_MASK_BITS(MSP_TX_1BIT_DATA_DELAY, MSP_TCF_MASK_TDDLY, 13) | \
	GEN_MASK_BITS(MSP_TX_ENDIANESS_LSB, MSP_TCF_MASK_TENDN, 12)  \
	)


typedef enum {
	MSP_DATA_BITS_8 = 0x00,
	MSP_DATA_BITS_10,
	MSP_DATA_BITS_12,
	MSP_DATA_BITS_14,
	MSP_DATA_BITS_16,
	MSP_DATA_BITS_20,
	MSP_DATA_BITS_24,
	MSP_DATA_BITS_32,
} t_msp_data_size;

typedef enum {
	MSP_INTERNAL_CLK = 0x0,
	MSP_EXTERNAL_CLK,
} t_msp_clk_src;

typedef struct {
	t_msp_clk_src clk_src;
	/* value from 0 to 1023 */
	u16 sckdiv;
	/*Used only when MSPSCK clocks the sample rate generator (SCKSEL = 1Xb):
	 * 0b: The rising edge of MSPSCK clocks the sample rate generator
	 * 1b: The falling edge of MSPSCK clocks the sample rate generator */
	int sckpol;
} t_msp_clock_params;

struct msp_controller {
	t_spi_loopback lbm;
	t_spi_interface iface;
	t_spi_hierarchy hierarchy;
	t_spi_fifo_endian endian_rx;
	t_spi_fifo_endian endian_tx;
	t_spi_mode com_mode;
	t_msp_data_size data_size;
	t_msp_clock_params clk_freq;
	int spi_burst_mode_enable;
	union {
		struct motorola_spi_proto_params moto;
		struct microwire_proto_params micro;
	} proto_params;

	u32 freq;
	void (*cs_control) (u32 control);
	t_dma_xfer_type dma_xfer_type;
	struct nmdkspi_dma *dma_config;
};

struct msp_regs{
	u32 gcr;
	u32 tcf;
	u32 rcf;
	u32 srg;
	u32 dmacr;
};

#endif
