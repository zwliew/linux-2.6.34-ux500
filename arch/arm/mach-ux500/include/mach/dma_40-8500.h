/*----------------------------------------------------------------------------*/
/*   copyright STMicroelectronics, 2008.                                      */
/*                                                                            */
/* This program is free software; you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by       */
/* the Free Software Foundation; either version 2.1 of the License, or 	      */
/* (at your option) any later version.                                        */
/*                                                                            */
/* This program is distributed in the hope that it will be useful,	      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of 	      */
/*  MERCHANTABILITY or FITNESS 						      */
/* FOR A PARTICULAR PURPOSE. See the GNU General Public License for           */
/* more details.							      */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.       */
/*----------------------------------------------------------------------------*/

#ifndef __INC_ARCH_ARM_SOC_DMA_H
#define __INC_ARCH_ARM_SOC_DMA_H
#include <linux/spinlock.h>

/******************************************************************************/
#define MEM_WRITE_BITS(reg, val, mask, sb)  \
		((reg) =   (((reg) & ~(mask)) | (((val)<<(sb)) & (mask))))
#define MEM_READ_BITS(reg, mask, sb)    \
		(((reg) & (mask)) >> (sb))

#define REG_WR_BITS_1(reg, val, mask, sb) \
		iowrite32(((val)<<(sb) | (~mask)), reg)
#define REG_WR_BITS(reg, val, mask, sb) \
    iowrite32(((ioread32(reg) & ~(mask)) | (((val)<<(sb)) & (mask))), reg)
#define REG_RD_BITS(reg, mask, sb)    (((ioread32(reg)) & (mask)) >> (sb))
/******************************************************************************/
#define FULL32_MASK 0xFFFFFFFF
#define NO_SHIFT	0
#define MAX_PHYSICAL_CHANNELS 32
#define MAX_AVAIL_PHY_CHANNELS 8
#define MAX_LOGICAL_CHANNELS 128
#define NUM_CHANNELS (MAX_LOGICAL_CHANNELS + MAX_PHYSICAL_CHANNELS)
/******************************************************************************/

#define PHYSICAL_RESOURCE_TYPE_POS(i)  (2*(i / 2))
#define PHYSICAL_RESOURCE_TYPE_MASK(i) (0x3UL << PHYSICAL_RESOURCE_TYPE_POS(i))

#define PHYSICAL_RESOURCE_CHANNEL_MODE_POS(i)  (2*(i / 2))
#define PHYSICAL_RESOURCE_CHANNEL_MODE_MASK(i) \
		(0x3UL << PHYSICAL_RESOURCE_CHANNEL_MODE_POS(i))


#define PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_POS(i)  (2*(i / 2))
#define PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_MASK(i) \
		(0x3UL << PHYSICAL_RESOURCE_CHANNEL_MODE_OPTION_POS(i))

#define PHYSICAL_RESOURCE_SECURE_MODE_POS(i)  (2*(i / 2))
#define PHYSICAL_RESOURCE_SECURE_MODE_MASK(i) \
		(0x3UL << PHYSICAL_RESOURCE_SECURE_MODE_POS(i))

#define ACT_PHY_RES_POS(i)  (2*(i / 2))
#define ACT_PHY_RES_MASK(i) (0x3UL << ACT_PHY_RES_POS(i))

#define ACTIVATE_RESOURCE_MODE_POS(i)  (2*(i / 2))
#define ACTIVATE_RESOURCE_MODE_MASK(i) (0x3UL << ACTIVATE_RESOURCE_MODE_POS(i))

/*****************************************************************************
 Standard basic Channel configuration macros - start
******************************************************************************/

/* Standard channel parameters - basic mode */
/* (Source and Destination config regs have */
/* similar bit descriptions and hence same mask) */
/*---------------------------------------------------------------------------*/
#define SREG_CFG_PHY_MST_POS 15
#define SREG_CFG_PHY_TIM_POS 14
#define SREG_CFG_PHY_EIM_POS 13
#define SREG_CFG_PHY_PEN_POS 12
#define SREG_CFG_PHY_PSIZE_POS 10
#define SREG_CFG_PHY_ESIZE_POS 8
#define SREG_CFG_PHY_PRI_POS 7
#define SREG_CFG_PHY_LBE_POS 6
#define SREG_CFG_PHY_TM_POS 4
#define SREG_CFG_PHY_EVTL_POS 0

#define SREG_CFG_PHY_MST_MASK	(0x1UL << SREG_CFG_PHY_MST_POS)
#define SREG_CFG_PHY_TIM_MASK	(0x1UL << SREG_CFG_PHY_TIM_POS)
#define SREG_CFG_PHY_EIM_MASK	(0x1UL << SREG_CFG_PHY_EIM_POS)
#define SREG_CFG_PHY_PEN_MASK	(0x1UL << SREG_CFG_PHY_PEN_POS)
#define SREG_CFG_PHY_PSIZE_MASK	(0x3UL << SREG_CFG_PHY_PSIZE_POS)
#define SREG_CFG_PHY_ESIZE_MASK	(0x3UL << SREG_CFG_PHY_ESIZE_POS)
#define SREG_CFG_PHY_PRI_MASK	(0x1UL << SREG_CFG_PHY_PRI_POS)
#define SREG_CFG_PHY_LBE_MASK	(0x1UL << SREG_CFG_PHY_LBE_POS)
#define SREG_CFG_PHY_TM_MASK	(0x3UL << SREG_CFG_PHY_TM_POS)
#define SREG_CFG_PHY_EVTL_MASK	(0xFUL << SREG_CFG_PHY_EVTL_POS)

/* Standard channel parameters - basic mode (element register) */
/*---------------------------------------------------------------------------*/
#define SREG_ELEM_PHY_ECNT_POS 16
#define SREG_ELEM_PHY_EIDX_POS 0

#define SREG_ELEM_PHY_ECNT_MASK	(0xFFFFUL << SREG_ELEM_PHY_ECNT_POS)
#define SREG_ELEM_PHY_EIDX_MASK	(0xFFFFUL << SREG_ELEM_PHY_EIDX_POS)

/* Standard channel parameters - basic mode (Pointer register) */
/*---------------------------------------------------------------------------*/
#define SREG_PTR_PHYS_PTR_MASK (0xFFFFFFFFUL)

/* Standard channel parameters - basic mode (Link register) */
/*---------------------------------------------------------------------------*/
#define SREG_LNK_PHY_TCP_POS 0
#define SREG_LNK_PHY_LMP_POS 1
#define SREG_LNK_PHY_PRE_POS 2
/* Source  destination link address. Contains the
 * 29-bit byte word aligned address of the reload area.
 */
#define SREG_LNK_PHYS_LNK_MASK (0xFFFFFFF8UL)
#define SREG_LNK_PHYS_TCP_MASK (0x1UL << SREG_LNK_PHY_TCP_POS)
#define SREG_LNK_PHYS_LMP_MASK (0x1UL << SREG_LNK_PHY_LMP_POS)
#define SREG_LNK_PHYS_PRE_MASK (0x1UL << SREG_LNK_PHY_PRE_POS)
/******************************************************************************/
/* Standard basic Channel configuration macros end */
/******************************************************************************/

/****************************************************************************/
/* Standard basic Channel LOGICAL mode - start */
/****************************************************************************/
/*Configuration register */
/*--------------------------------------------------------------------------*/

#define SREG_CFG_LOG_MST_POS 15
#define SREG_CFG_LOG_TIM_POS 14
#define SREG_CFG_LOG_EIM_POS 13
#define SREG_CFG_LOG_INCR_POS 12
#define SREG_CFG_LOG_PSIZE_POS 10
#define SREG_CFG_LOG_ESIZE_POS 8
#define SREG_CFG_LOG_PRI_POS 7
#define SREG_CFG_LOG_LBE_POS 6
#define SREG_CFG_LOG_GIM_POS 5
#define SREG_CFG_LOG_MFU_POS 4

#define SREG_CFG_LOG_MST_MASK	(0x1UL << SREG_CFG_LOG_MST_POS)
#define SREG_CFG_LOG_TIM_MASK	(0x1UL << SREG_CFG_LOG_TIM_POS)
#define SREG_CFG_LOG_EIM_MASK	(0x1UL << SREG_CFG_LOG_EIM_POS)
#define SREG_CFG_LOG_INCR_MASK	(0x1UL << SREG_CFG_LOG_INCR_POS)
#define SREG_CFG_LOG_PSIZE_MASK	(0x3UL << SREG_CFG_LOG_PSIZE_POS)
#define SREG_CFG_LOG_ESIZE_MASK	(0x3UL << SREG_CFG_LOG_ESIZE_POS)
#define SREG_CFG_LOG_PRI_MASK	(0x1UL << SREG_CFG_LOG_PRI_POS)
#define SREG_CFG_LOG_LBE_MASK	(0x1UL << SREG_CFG_LOG_LBE_POS)
#define SREG_CFG_LOG_GIM_MASK	(0x1UL << SREG_CFG_LOG_GIM_POS)
#define SREG_CFG_LOG_MFU_MASK	(0x1UL << SREG_CFG_LOG_MFU_POS)

/*Element register*/

#define SREG_ELEM_LOG_ECNT_POS 16
#define SREG_ELEM_LOG_LIDX_POS 8
#define SREG_ELEM_LOG_LOS_POS 1
#define SREG_ELEM_LOG_TCP_POS 0

#define SREG_ELEM_LOG_ECNT_MASK	(0xFFFFUL << SREG_ELEM_LOG_ECNT_POS)
#define SREG_ELEM_LOG_LIDX_MASK	(0xFFUL << SREG_ELEM_LOG_LIDX_POS)
#define SREG_ELEM_LOG_LOS_MASK	(0x7FUL << SREG_ELEM_LOG_LOS_POS)
#define SREG_ELEM_LOG_TCP_MASK	(0x1UL << SREG_ELEM_LOG_TCP_POS)

/*Pointer register */
#define SREG_PTR_LOG_PTR_MASK (0xFFFFFFFFUL)

/*Link registeri */
#define DEACTIVATE_EVENTLINE 0x0
#define ACTIVATE_EVENTLINE 0x1
#define EVENTLINE_POS(i)  (2*i)
#define EVENTLINE_MASK(i) (0x3UL << EVENTLINE_POS(i))

/* Standard basic Channel LOGICAL params in memory*/
#define MEM_LCSP0_ECNT_POS 16
#define MEM_LCSP0_SPTR_POS 0

#define MEM_LCSP0_ECNT_MASK	(0xFFFFUL << MEM_LCSP0_ECNT_POS)
#define MEM_LCSP0_SPTR_MASK	(0xFFFFUL << MEM_LCSP0_SPTR_POS)

#define MEM_LCSP1_SPTR_POS 16

#define MEM_LCSP1_SCFG_MST_POS 15
#define MEM_LCSP1_SCFG_TIM_POS 14
#define MEM_LCSP1_SCFG_EIM_POS 13
#define MEM_LCSP1_SCFG_INCR_POS 12
#define MEM_LCSP1_SCFG_PSIZE_POS 10
#define MEM_LCSP1_SCFG_ESIZE_POS 8

#define MEM_LCSP1_SLOS_POS 1
#define MEM_LCSP1_STCP_POS 0

#define MEM_LCSP1_SPTR_MASK (0xFFFFUL << MEM_LCSP1_SPTR_POS)

#define MEM_LCSP1_SCFG_MST_MASK (0x1UL << MEM_LCSP1_SCFG_MST_POS)
#define MEM_LCSP1_SCFG_TIM_MASK (0x1UL << MEM_LCSP1_SCFG_TIM_POS)
#define MEM_LCSP1_SCFG_EIM_MASK (0x1UL << MEM_LCSP1_SCFG_EIM_POS)
#define MEM_LCSP1_SCFG_INCR_MASK (0x1UL << MEM_LCSP1_SCFG_INCR_POS)
#define MEM_LCSP1_SCFG_PSIZE_MASK (0x3UL << MEM_LCSP1_SCFG_PSIZE_POS)
#define MEM_LCSP1_SCFG_ESIZE_MASK (0x3UL << MEM_LCSP1_SCFG_ESIZE_POS)

#define MEM_LCSP1_SLOS_MASK (0x7FUL << MEM_LCSP1_SLOS_POS)
#define MEM_LCSP1_STCP_MASK (0x1UL << MEM_LCSP1_STCP_POS)

#define MEM_LCSP2_ECNT_POS 16
#define MEM_LCSP2_DPTR_POS 0

#define MEM_LCSP2_ECNT_MASK	(0xFFFFUL << MEM_LCSP2_ECNT_POS)
#define MEM_LCSP2_DPTR_MASK	(0xFFFFUL << MEM_LCSP2_DPTR_POS)

#define MEM_LCSP3_DPTR_POS 16

#define MEM_LCSP3_DCFG_MST_POS 15
#define MEM_LCSP3_DCFG_TIM_POS 14
#define MEM_LCSP3_DCFG_EIM_POS 13
#define MEM_LCSP3_DCFG_INCR_POS 12
#define MEM_LCSP3_DCFG_PSIZE_POS 10
#define MEM_LCSP3_DCFG_ESIZE_POS 8

#define MEM_LCSP3_DLOS_POS 1
#define MEM_LCSP3_DTCP_POS 0

#define MEM_LCSP3_DPTR_MASK (0xFFFFUL << MEM_LCSP3_DPTR_POS)

#define MEM_LCSP3_DCFG_MST_MASK (0x1UL << MEM_LCSP3_DCFG_MST_POS)
#define MEM_LCSP3_DCFG_TIM_MASK (0x1UL << MEM_LCSP3_DCFG_TIM_POS)
#define MEM_LCSP3_DCFG_EIM_MASK (0x1UL << MEM_LCSP3_DCFG_EIM_POS)
#define MEM_LCSP3_DCFG_INCR_MASK (0x1UL << MEM_LCSP3_DCFG_INCR_POS)
#define MEM_LCSP3_DCFG_PSIZE_MASK (0x3UL << MEM_LCSP3_DCFG_PSIZE_POS)
#define MEM_LCSP3_DCFG_ESIZE_MASK (0x3UL << MEM_LCSP3_DCFG_ESIZE_POS)

#define MEM_LCSP3_DLOS_MASK (0x7FUL << MEM_LCSP3_DLOS_POS)
#define MEM_LCSP3_DTCP_MASK (0x1UL << MEM_LCSP3_DTCP_POS)
#define DMA_INFINITE_XFER (0x80000000)
#define CONFIG_USB_U8500_EVENT_LINES
/******************************************************************************/

/* Logical Standard Channel Parameters */

struct std_log_memory_param {
	u32 dmac_lcsp0;
	u32 dmac_lcsp1;
	u32 dmac_lcsp2;
	u32 dmac_lcsp3;
};

struct std_src_log_memory_param {
	u32 dmac_lcsp0;
	u32 dmac_lcsp1;
};

struct std_dest_log_memory_param {
	u32 dmac_lcsp2;
	u32 dmac_lcsp3;
};

enum channel_command {
	STOP_CHANNEL = 0x1,
	RUN_CHANNEL = 0x2,
	SUSPEND_REQ = 0x2,
	SUSPENDED = 0x3
};

/* Standard Channel parameter register offsets */
#define CHAN_REG_SSCFG 0x00
#define CHAN_REG_SSELT 0x04
#define CHAN_REG_SSPTR 0x08
#define CHAN_REG_SSLNK 0x0C
#define CHAN_REG_SDCFG 0x10
#define CHAN_REG_SDELT 0x14
#define CHAN_REG_SDPTR 0x18
#define CHAN_REG_SDLNK 0x1C

/* DMA Register Offsets */
#define DREG_GCC 0x000
#define DREG_PRTYP 0x004
#define DREG_PRSME 0x008
#define DREG_PRSMO 0x00C
#define DREG_PRMSE 0x010
#define DREG_PRMSO 0x014
#define DREG_PRMOE 0x018
#define DREG_PRMOO 0x01C
#define DREG_LCPA 0x020
#define DREG_LCLA 0x024
#define DREG_SLCPA 0x028
#define DREG_SLCLA 0x02C
#define DREG_SSEG(j) (0x030 + j*4)
#define DREG_SCEG(j) (0x040 + j*4)
#define DREG_ACTIVE 0x050
#define DREG_ACTIVO 0x054
#define DREG_FSEB1 0x058
#define DREG_FSEB2 0x05C
#define DREG_PCMIS 0x060
#define DREG_PCICR 0x064
#define DREG_PCTIS 0x068
#define DREG_PCEIS 0x06C
#define DREG_SPCMIS 0x070
#define DREG_SPCICR 0x074
#define DREG_SPCTIS 0x078
#define DREG_SPCEIS 0x07C
#define DREG_LCMIS(j) (0x080 + j*4)
#define DREG_LCICR(j) (0x090 + j*4)
#define DREG_LCTIS(j) (0x0A0 + j*4)
#define DREG_LCEIS(j) (0x0B0 + j*4)
#define DREG_SLCMIS(j) (0x0C0 + j*4)
#define DREG_SLCICR(j) (0x0D0 + j*4)
#define DREG_SLCTIS(j) (0x0E0 + j*4)
#define DREG_SLCEIS(j) (0x0F0 + j*4)
#define DREG_STFU 0xFC8
#define DREG_ICFG 0xFCC
#define DREG_MPLUG(j) (0xFD0 + j*4)
#define DREG_PERIPHID(j) (0xFE0 + j*4)
#define DREG_CELLID(j) (0xFF0 + j*4)

/*
 * LLI related structures
*/

struct dma_lli_info {
	u32 reg_cfg;
	u32 reg_elt;
	u32 reg_ptr;
	u32 reg_lnk;
};

struct dma_logical_src_lli_info {
	u32 dmac_lcsp0;
	u32 dmac_lcsp1;
};

struct dma_logical_dest_lli_info {
	u32 dmac_lcsp2;
	u32 dmac_lcsp3;
};

/*****************************************************************************/
enum dma_toggle_endianess {
	DO_NOT_CHANGE_ENDIANESS,
	CHANGE_ENDIANESS
};

enum dma_master_id {
	DMA_MASTER_0,
	DMA_MASTER_1
};

/******************************************************************/
/*Description of bitfields of channel_type variable in info structure*/

#define INFO_CH_TYPE_POS 0
#define STANDARD_CHANNEL (0x1 << INFO_CH_TYPE_POS)
#define EXTENDED_CHANNEL (0x2 << INFO_CH_TYPE_POS)

#define INFO_PRIO_TYPE_POS 2
#define HIGH_PRIORITY_CHANNEL (0x1 << INFO_PRIO_TYPE_POS)
#define LOW_PRIORITY_CHANNEL (0x2 << INFO_PRIO_TYPE_POS)

#define INFO_SEC_TYPE_POS 4
#define SECURE_CHANNEL (0x1 << INFO_SEC_TYPE_POS)
#define NON_SECURE_CHANNEL (0x2 << INFO_SEC_TYPE_POS)

#define INFO_CH_MODE_TYPE_POS 6
#define CHANNEL_IN_PHYSICAL_MODE (0x1 << INFO_CH_MODE_TYPE_POS)
#define CHANNEL_IN_LOGICAL_MODE (0x2 << INFO_CH_MODE_TYPE_POS)
#define CHANNEL_IN_OPERATION_MODE (0x3 << INFO_CH_MODE_TYPE_POS)

#define INFO_CH_MODE_OPTION_POS 8
#define PCHAN_BASIC_MODE (0x1 << INFO_CH_MODE_OPTION_POS)
#define PCHAN_MODULO_MODE (0x2 << INFO_CH_MODE_OPTION_POS)
#define PCHAN_DOUBLE_DEST_MODE (0x3 << INFO_CH_MODE_OPTION_POS)
#define LCHAN_SRC_PHY_DEST_LOG (0x1 << INFO_CH_MODE_OPTION_POS)
#define LCHAN_SRC_LOG_DEST_PHS (0x2 << INFO_CH_MODE_OPTION_POS)
#define LCHAN_SRC_LOG_DEST_LOG (0x3 << INFO_CH_MODE_OPTION_POS)

#define INFO_LINK_TYPE_POS 9
#define LINK_PRE (0x0 << INFO_LINK_TYPE_POS)
#define LINK_POST (0x1 << INFO_LINK_TYPE_POS)

#define INFO_TIM_POS 10
#define NO_TIM_FOR_LINK (0x0 << INFO_TIM_POS)
#define TIM_FOR_LINK (0x1 << INFO_TIM_POS)

/******************************************************************/

enum dma_phys_res_type {
	DMA_STANDARD = 0x1,
	DMA_EXTENDED = 0x2
};

enum dma_chan_priority {
	DMA_LOW_PRIORITY = 0x1,
	DMA_HIGH_PRIORITY = 0x2
};

enum dma_chan_security {
	DMA_SECURE_CHAN = 0x1,
	DMA_NONSECURE_CHAN = 0x2
};

enum dma_channel_mode_option {
	BASIC_MODE = 0x1,
	MODULO_MODE = 0x2,
	DOUBLE_DESTINATION_MODE = 0x3,

	SRC_PHY_DEST_LOG = 0x1,
	SRC_LOG_DEST_PHS = 0x2,
	SRC_LOG_DEST_LOG = 0x3
};

enum dma_channel_mode {
	DMA_CHAN_IN_PHYS_MODE = 0x1,
	DMA_CHAN_IN_LOG_MODE = 0x2,
	DMA_CHAN_IN_OPERATION_MODE = 0x3
};

enum dma_event_group {
	DMA_EVENT_GROUP_0,
	DMA_EVENT_GROUP_1,
	DMA_EVENT_GROUP_2,
	DMA_EVENT_GROUP_3,
	DMA_NO_EVENT_GROUP
};

enum dma_half_chan {
	DMA_SRC_HALF_CHANNEL,
	DMA_DEST_HALF_CHANNEL
};

enum dma_addr_inc {
	DMA_ADR_NOINC,
	DMA_ADR_INC
};

enum dma_command {
	DMA_STOP,
	DMA_RUN,
	DMA_SUSPEND_REQ,
	DMA_SUSPENDED
};

enum dma_chan_status {
	DMA_ONGOING_EXCHANGE,
	DMA_SUSPENDED_EXCHANGE,
	DMA_HALTED_EXCHANGE,
	DMA_STATUS_UNKNOWN = -1
};

enum dma_chan_id {
	DMA_CHAN_0,
	DMA_CHAN_1,
	DMA_CHAN_2,
	DMA_CHAN_3,
	DMA_CHAN_4,
	DMA_CHAN_5,
	DMA_CHAN_6,
	DMA_CHAN_7,
	DMA_CHAN_NOT_ALLOCATED = -1
};

enum dma_src_dev_type {
	DMA_DEV_SPI0_RX = 0,
	DMA_DEV_SD_MMC0_RX,
	DMA_DEV_SD_MMC1_RX,
	DMA_DEV_SD_MMC2_RX,
	DMA_DEV_I2C1_RX,
	DMA_DEV_I2C3_RX,
	DMA_DEV_I2C2_RX,
	DMA_DEV_SSP0_RX = 8,
	DMA_DEV_SSP1_RX,
	DMA_DEV_MCDE_RX,
	DMA_DEV_UART2_RX,
	DMA_DEV_UART1_RX,
	DMA_DEV_UART0_RX,
	DMA_DEV_MSP2_RX,
	DMA_DEV_I2C0_RX,	/*15*/
#ifndef CONFIG_USB_U8500_EVENT_LINES
        DMA_DEV_USB_OTG_IEP_8 ,
        DMA_DEV_USB_OTG_IEP_1_9 ,
        DMA_DEV_USB_OTG_IEP_2_10 ,
        DMA_DEV_USB_OTG_IEP_3_11 ,
#else
        DMA_DEV_USB_OTG_IEP_7_15 ,
        DMA_DEV_USB_OTG_IEP_6_14 ,
        DMA_DEV_USB_OTG_IEP_5_13 ,
        DMA_DEV_USB_OTG_IEP_4_12 ,
#endif
	DMA_DEV_SLIM0_CH0_RX_HSI_RX_CH0,
	DMA_DEV_SLIM0_CH1_RX_HSI_RX_CH1,
	DMA_DEV_SLIM0_CH2_RX_HSI_RX_CH2,
	DMA_DEV_SLIM0_CH3_RX_HSI_RX_CH3,
	DMA_DEV_SRC_SXA0_RX_TX,
	DMA_DEV_SRC_SXA1_RX_TX,
	DMA_DEV_SRC_SXA2_RX_TX,
	DMA_DEV_SRC_SXA3_RX_TX,
	DMA_DEV_SD_MM2_RX,
	DMA_DEV_SD_MM0_RX,
	DMA_DEV_MSP1_RX,
	DMA_SLIM0_CH0_RX,
	DMA_DEV_MSP0_RX = DMA_SLIM0_CH0_RX,
	DMA_DEV_SD_MM1_RX,
	DMA_DEV_SPI2_RX,
	DMA_DEV_I2C3_RX2,
	DMA_DEV_SPI1_RX,
#ifndef CONFIG_USB_U8500_EVENT_LINES
        DMA_DEV_USB_OTG_IEP_4_12 ,
        DMA_DEV_USB_OTG_IEP_5_13 ,
        DMA_DEV_USB_OTG_IEP_6_14 ,
        DMA_DEV_USB_OTG_IEP_7_15 ,
#else
        DMA_DEV_USB_OTG_IEP_3_11 ,
        DMA_DEV_USB_OTG_IEP_2_10 ,
        DMA_DEV_USB_OTG_IEP_1_9 ,
        DMA_DEV_USB_OTG_IEP_8 ,
#endif
	DMA_DEV_SPI3_RX,
	DMA_DEV_SD_MM3_RX,
	DMA_DEV_SD_MM4_RX,
	DMA_DEV_SD_MM5_RX,
	DMA_DEV_SRC_SXA4_RX_TX,
	DMA_DEV_SRC_SXA5_RX_TX,
	DMA_DEV_SRC_SXA6_RX_TX,
	DMA_DEV_SRC_SXA7_RX_TX,
	DMA_DEV_CAC1_RX,
	DMA_DEV_MSHC_RX = 51,
	DMA_DEV_SLIM1_CH0_RX_HSI_RX_CH4,
	DMA_DEV_SLIM1_CH1_RX_HSI_RX_CH5,
	DMA_DEV_SLIM1_CH2_RX_HSI_RX_CH6,
	DMA_DEV_SLIM1_CH3_RX_HSI_RX_CH7,
	DMA_DEV_CAC0_RX = 61,
	DMA_DEV_SRC_MEMORY = 64,
};

enum dma_dest_dev_type {
	DMA_DEV_SPI0_TX = 0,
	DMA_DEV_SD_MMC0_TX,
	DMA_DEV_SD_MMC1_TX,
	DMA_DEV_SD_MMC2_TX,
	DMA_DEV_I2C1_TX,
	DMA_DEV_I2C3_TX,
	DMA_DEV_I2C2_TX,
	DMA_DEV_SSP0_TX = 8,
	DMA_DEV_SSP1_TX,
	DMA_DEV_UART2_TX = 11,
	DMA_DEV_UART1_TX,
	DMA_DEV_UART0_TX,
	DMA_DEV_MSP2_TX,
	DMA_DEV_I2C0_TX,
#ifndef CONFIG_USB_U8500_EVENT_LINES
        DMA_DEV_USB_OTG_OEP_8 ,
        DMA_DEV_USB_OTG_OEP_1_9 ,
        DMA_DEV_USB_OTG_OEP_2_10 ,
        DMA_DEV_USB_OTG_OEP_3_11 ,
#else
        DMA_DEV_USB_OTG_OEP_7_15 ,
        DMA_DEV_USB_OTG_OEP_6_14 ,
        DMA_DEV_USB_OTG_OEP_5_13 ,
        DMA_DEV_USB_OTG_OEP_4_12 ,
#endif
	DMA_DEV_SLIM0_CH0_TX_HSI_TX_CH0,
	DMA_DEV_SLIM0_CH1_TX_HSI_TX_CH1,
	DMA_DEV_SLIM0_CH2_TX_HSI_TX_CH2,
	DMA_DEV_SLIM0_CH3_TX_HSI_TX_CH3,
	DMA_DEV_DST_SXA0_RX_TX,
	DMA_DEV_DST_SXA1_RX_TX,
	DMA_DEV_DST_SXA2_RX_TX,
	DMA_DEV_DST_SXA3_RX_TX,
	DMA_DEV_SD_MM2_TX,
	DMA_DEV_SD_MM0_TX,
	DMA_DEV_MSP1_TX,
	DMA_SLIM0_CH0_TX,
	DMA_DEV_MSP0_TX = DMA_SLIM0_CH0_TX,
	DMA_DEV_SD_MM1_TX,
	DMA_DEV_SPI2_TX,
	DMA_DEV_I2C3_TX2,
	DMA_DEV_SPI1_TX,
#ifndef CONFIG_USB_U8500_EVENT_LINES
        DMA_DEV_USB_OTG_OEP_4_12 ,
        DMA_DEV_USB_OTG_OEP_5_13 ,
        DMA_DEV_USB_OTG_OEP_6_14 ,
        DMA_DEV_USB_OTG_OEP_7_15 ,
#else
        DMA_DEV_USB_OTG_OEP_3_11 ,
        DMA_DEV_USB_OTG_OEP_2_10 ,
        DMA_DEV_USB_OTG_OEP_1_9 ,
        DMA_DEV_USB_OTG_OEP_8 ,
#endif
	DMA_DEV_SPI3_TX,
	DMA_DEV_SD_MM3_TX,
	DMA_DEV_SD_MM4_TX,
	DMA_DEV_SD_MM5_TX,
	DMA_DEV_DST_SXA4_RX_TX,
	DMA_DEV_DST_SXA5_RX_TX,
	DMA_DEV_DST_SXA6_RX_TX,
	DMA_DEV_DST_SXA7_RX_TX,
	DMA_DEV_CAC1_TX,
	DMA_DEV_CAC1_TX_HAC1_TX,
	DMA_DEV_HAC1_TX,
	DMA_DEV_MSHC_TX,
	DMA_DEV_SLIM1_CH0_TX_HSI_TX_CH4,
	DMA_DEV_SLIM1_CH1_TX_HSI_TX_CH5,
	DMA_DEV_SLIM1_CH2_TX_HSI_TX_CH6,
	DMA_DEV_SLIM1_CH3_TX_HSI_TX_CH7,
	DMA_DEV_CAC0_TX = 61,
	DMA_DEV_CAC0_TX_HAC0_TX,
	DMA_DEV_HAC0_TX,
	DMA_DEV_DEST_MEMORY = 64,
};

enum dma_half_chan_sync {
	DMA_NO_SYNC,
	DMA_PACKET_SYNC,
	DMA_FRAME_SYNC,
	DMA_BLOCK_SYNC
};

enum half_channel_type {
	PHYSICAL_HALF_CHANNEL = 0x1,
	LOGICAL_HALF_CHANNEL,
};

enum periph_data_width {
	DMA_BYTE_WIDTH,
	DMA_HALFWORD_WIDTH,
	DMA_WORD_WIDTH,
	DMA_DOUBLEWORD_WIDTH
};

enum dma_burst_size {
	DMA_BURST_SIZE_1,
	DMA_BURST_SIZE_4,
	DMA_BURST_SIZE_8,
	DMA_BURST_SIZE_16,
	DMA_NO_BURST
};

enum dma_buffer_type {
	SINGLE_BUFFERED,
	DOUBLE_BUFFERED
};

enum dma_link_type {
	POSTLINK,
	PRELINK
};

struct dma_half_channel_info {
	enum half_channel_type half_chan_type;
	enum dma_endianess endianess;
	enum periph_data_width data_width;
	enum dma_burst_size burst_size;
	enum dma_buffer_type buffer_type;
	enum dma_event_group event_group;
	enum dma_addr_inc addr_inc;
	u32 event_line;
};

/**
 * struct dma_channel_info - Data structure to
 * manage state of a DMA channel.
 *
 * @device_id: Name of the device
 * @pipe_id: Pipe Id allocated to the client driver.
 * @channel_id: Channel Id allocated for the client,
 *	used internally by DMA Driver(when interrupt comes)
 *	(there are 128 Logical + 32 Physical). Max is 160 channels
 * @active:	Is the channel active at this point?
 *		1 - active, 0- inactive
 * @invalid:	Has configuration been updated since we last updated
 *		the registers? invalid = 1 (need to update)
 * @phys_chan_id:physical channel ID on which this channel will execute
 * @src_addr:	Src address for single DMA
 * @dst_addr:	Dest address for single DMA
 * @xfer_len:	Length of transfer  expressed in Bytes
 * @current_sg:	Pointer to current SG element being used for xfer(only active
 *		if TIM_MASK is set)
 * @lli_interrupt: 1 if interrupts generated for each LLI
 * @sgcount_src:   Number of SG items in source sg list
 * @lli_block_id_src: Block id for LLI pool used for source half channel
 * @sg_src:	Head Pointer to SG list for source half channel
 * @sgcount_dest:  Number of SG items in Dest sg list
 * @lli_block_id_dest: Block id for LLI pool used for Dest half channel
 * @sg_dest:	Head Pointer to SG list for Destination half channel
 * @sg_block_id_src:  Block id for SG pool used for SRC half channel
 * @sg_block_id_dest: Block id for SG pool used for DEST half channel
 * @link_type:	Whether prelink or postlink
 * @reserve_channel: whether channel is reserved.
 * @dir:	MEM 2 MEM, PERIPH 2 MEM , MEM 2 PERIPH, PERIPH 2 PERIPH
 * @flow_cntlr: who is the flow controller DMA or Peripheral
 * @data:	Pointer to data of callback function
 * @callback:	Pointer to callback function
 * @pr_type:	Physical resource type : standard or extended
 * @chan_mode:	Mode of Physical resource
 *		For std channel: Physical,Logical,Operation
 * @mode_option: Further options of mode selected above
 * @priority:	Priority for this channel
 * @security:	security for this channel
 * @bytes_xfred: Number of Bytes xfered till now
 * @ch_status:
 * @src_dev_type: Device type of Source
 * @dst_dev_type: Device type of Dest
 * @src_info:	Parameters describing source half channel
 * @dst_info:	Parameters describing dest half channel
 * This is a private data structure of DMA driver used to maintain
 * state information of a particular channel
 */
struct dma_channel_info {
	const char *device_id;
	u32 src_cfg;
	u32 dst_cfg;
	u32 dmac_lcsp3;
	u32 dmac_lcsp1;
	int pipe_id;
	int channel_id;
	int active;
	int invalid;
	enum dma_chan_id phys_chan_id;
	void *src_addr;
	void *dst_addr;
	u32 xfer_len;
	u32 src_xfer_elem;
	u32 dst_xfer_elem;
	struct scatterlist *current_sg;
	/*For Scatter Gather DMA */
	int lli_interrupt;
	int sgcount_src;
	int lli_block_id_src;
	struct scatterlist *sg_src;
	int sgcount_dest;
	int lli_block_id_dest;
	struct scatterlist *sg_dest;
	int sg_block_id_src;
	int sg_block_id_dest;

	enum dma_link_type link_type;
	unsigned int reserve_channel;
	enum dma_xfer_dir dir;
	enum dma_flow_controller flow_cntlr;

	void *data;
	dma_callback_t callback;

	enum dma_phys_res_type pr_type;
	enum dma_channel_mode chan_mode;
	enum dma_channel_mode_option mode_option;
	enum dma_chan_priority priority;
	enum dma_chan_security security;

	int bytes_xfred;
	enum dma_chan_status ch_status;
	enum dma_src_dev_type src_dev_type;
	enum dma_dest_dev_type dst_dev_type;
	struct dma_half_channel_info src_info;
	struct dma_half_channel_info dst_info;
	spinlock_t cfg_lock;
};

enum res_status {
	RESOURCE_FREE,
	RESOURCE_PHYSICAL,
	RESOURCE_LOGICAL
};

struct phys_res_status {
	int count;
	enum res_status status;
};

struct phy_res_info {
	enum res_status status;
	u32 count;
	u32 dirty;
};

#define NUM_LLI_PER_REQUEST 40
#define NUM_SG_PER_REQUEST 40
#define NUM_LOGICAL_CHANNEL_PER_PHY_RESOURCE 16
#define NUM_LLI_PER_LOG_CHANNEL 8
#define SIXTY_FOUR_KB (64 * 1024)

#define MAX_NUM_OF_ELEM_IN_A_XFER (64*1024)
/*Number of Fixed size LLI Blocks available for Physical channels */
#define NUM_PCHAN_LLI_BLOCKS 32
/*Number of Fixed size SG blocks */
#define NUM_SG_BLOCKS 32
/*Maximum Iterations taken before giving up suspending a channel */
#define MAX_ITERATIONS 500

extern struct driver_debug_st DBG_ST;

#endif
