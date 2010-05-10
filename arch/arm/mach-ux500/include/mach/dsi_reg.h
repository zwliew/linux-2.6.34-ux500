/*---------------------------------------------------------------------------*/
/* Copyright (C) ST Ericsson 2009 					     */
/*                                                                           */
/* This program is free software; you can redistribute it and/or modify it   */
/* under the terms of the GNU Lesser General Public License as published     */
/* by the Free Software Foundation; either version 2.1 of the License,       */
/* or (at your option)any later version.                                     */
/*                                                                           */
/* This program is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See                  */
/* the GNU Lesser General Public License for more details.                   */
/*                                                                           */
/* You should have received a copy of the GNU Lesser General Public License  */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.      */
/*---------------------------------------------------------------------------*/

#ifndef _DSIREG_H_
#define _DSIREG_H_

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

#include <linux/types.h>

#define DSI_SET_BIT                     0x1
#define DSI_CLEAR_BIT                   0x0
#define DSI_SET_ALL_BIT                 0xFFFFFFFF
#define DSI_CLEAR_ALL_BIT               0x0
#define DSI_MCTL_INTMODE_MASK           MASK_BIT0
#define DSI_MCTL_LINKEN_MASK            MASK_BIT0
#define DSI_MCTL_INTERFACE1_MODE_MASK   MASK_BIT1
#define DSI_MCTL_VID_EN_MASK            MASK_BIT2
#define DSI_MCTL_TVG_SEL_MASK           MASK_BIT3
#define DSI_MCTL_TBG_SEL_MASK           MASK_BIT4
#define DSI_MCTL_READEN_MASK            MASK_BIT8
#define DSI_MCTL_BTAEN_MASK             MASK_BIT9
#define DSI_MCTL_DISPECCGEN_MASK        MASK_BIT10
#define DSI_MCTL_DISPCHECKSUMGEN_MASK   MASK_BIT11
#define DSI_MCTL_HOSTEOTGEN_MASK        MASK_BIT12
#define DSI_MCTL_DISPEOTGEN_MASK        MASK_BIT13
#define DSI_PLL_MASTER_MASK             MASK_BIT16
#define DSI_PLL_OUT_SEL_MASK            MASK_BIT11
#define DSI_PLL_IN_SEL_MASK             MASK_BIT10
#define DSI_PLL_DIV_MASK                (MASK_BIT7 | MASK_BIT8 | MASK_BIT9)
#define DSI_PLL_MULT_MASK               (MASK_BYTE0 & 0x7F)
#define DSI_REG_TE_MASK                 MASK_BIT7
#define DSI_IF1_TE_MASK                 MASK_BIT5
#define DSI_IF2_TE_MASK                 MASK_BIT6
#define DSI_LANE2_EN_MASK               MASK_BIT0
#define DSI_FORCE_STOP_MODE_MASK        MASK_BIT1
#define DSI_CLK_CONTINUOUS_MASK         MASK_BIT2
#define DSI_CLK_ULPM_EN_MASK            MASK_BIT3
#define DSI_DAT1_ULPM_EN_MASK           MASK_BIT4
#define DSI_DAT2_ULPM_EN_MASK           MASK_BIT5
#define DSI_WAIT_BURST_MASK             (MASK_BIT6 | MASK_BIT7 | MASK_BIT8 | MASK_BIT9)
#define DSI_CLKLANESTS_MASK             (MASK_BIT0 | MASK_BIT1)
#define DSI_DATALANE1STS_MASK           (MASK_BIT2 | MASK_BIT3 | MASK_BIT4)
#define DSI_DATALANE2STS_MASK           (MASK_BIT5 | MASK_BIT6)
#define DSI_CLK_DIV_MASK                MASK_QUARTET0
#define DSI_HSTX_TO_MASK                (MASK_QUARTET1 | MASK_BYTE1 | MASK_BIT16 | MASK_BIT17)
#define DSI_LPRX_TO_MASK                (MASK_BYTE3 | MASK_QUARTET5 | MASK_BIT18 | MASK_BIT18)
#define DSI_CLK_ULPOUT_MASK             (MASK_BYTE0 | MASK_BIT8)
#define DSI_DATA_ULPOUT_MASK            (MASK_QUARTET3 | MASK_BIT9 | MASK_BIT10 | MASK_BIT11 | MASK_BIT16 | MASK_BIT17)
#define DSI_PLL_START_MASK              MASK_BIT0
#define DSI_CKLANE_EN_MASK              MASK_BIT3
#define DSI_DAT1_EN_MASK                MASK_BIT4
#define DSI_DAT2_EN_MASK                MASK_BIT5
#define DSI_CLK_ULPM_MASK               MASK_BIT6
#define DSI_DAT1_ULPM_MASK              MASK_BIT7
#define DSI_DAT2_ULPM_MASK              MASK_BIT8
#define DSI_IF1_EN_MASK                 MASK_BIT9
#define DSI_IF2_EN_MASK                 MASK_BIT10
#define DSI_MAIN_STS_MASK               MASK_BYTE0
#define DSI_DPHY_ERROR_MASK             MASK_HALFWORD0
#define DSI_IF_DATA_MASK                MASK_HALFWORD0
#define DSI_IF_VALID_MASK               MASK_BIT16
#define DSI_IF_START_MASK               MASK_BIT17
#define DSI_IF_FRAME_SYNC_MASK          MASK_BIT18
#define DSI_IF_STALL_MASK               MASK_BIT0
#define DSI_INT_VAL_MASK                MASK_BIT0
#define DSI_DIRECT_CMD_RD_STS_MASK      (MASK_BYTE0 | MASK_BIT8)
#define DSI_CMD_MODE_STS_MASK           (MASK_QUARTET0 | MASK_BIT4 | MASK_BIT5)
#define DSI_RD_ID_MASK                  (MASK_BIT16 | MASK_BIT17 )
#define DSI_RD_DCSNOTGENERIC_MASK       MASK_BIT18
#define DSI_CMD_NAT_MASK                (MASK_BIT0 | MASK_BIT1 | MASK_BIT2)
#define DSI_CMD_LONGNOTSHORT_MASK       MASK_BIT3
#define DSI_CMD_HEAD_MASK               (MASK_QUARTET2 | MASK_BIT12 | MASK_BIT13)
#define DSI_CMD_ID_MASK                 (MASK_BIT14 | MASK_BIT15)
#define DSI_CMD_SIZE_MASK               (MASK_QUARTET4 | MASK_BIT20)
#define DSI_CMD_LP_EN_MASK              (MASK_BIT21)
#define DSI_TRIGGER_VAL_MASK            MASK_QUARTET6
#define DSI_TE_LOWERBIT_MASK            MASK_BYTE2
#define DSI_TE_UPPERBIT_MASK            (MASK_BIT24 | MASK_BIT25)
#define DSI_FIL_VAL_MASK                MASK_BYTE1
#define DSI_ARB_MODE_MASK               MASK_BIT6
#define DSI_ARB_PRI_MASK                MASK_BIT7
#define DSI_START_MODE_MASK             (MASK_BIT0 | MASK_BIT1 )
#define DSI_STOP_MODE_MASK              (MASK_BIT2 | MASK_BIT3)
#define DSI_VID_ID_MASK                 (MASK_BIT4 | MASK_BIT5)
#define DSI_HEADER_MASK                 (MASK_BIT6 | MASK_BIT7 | MASK_BIT8 | MASK_BIT9 | MASK_BIT10 | MASK_BIT11)
#define DSI_PIXEL_MODE_MASK             (MASK_BIT12 | MASK_BIT13)
#define DSI_BURST_MODE_MASK             (MASK_BIT14)
#define DSI_SYNC_PULSE_ACTIVE_MASK      (MASK_BIT15)
#define DSI_SYNC_PULSE_HORIZONTAL_MASK  (MASK_BIT16)
#define DSI_BLKLINE_MASK                (MASK_BIT17 | MASK_BIT18)
#define DSI_BLKEOL_MASK                 (MASK_BIT19 | MASK_BIT20)
#define DSI_RECOVERY_MODE_MASK          (MASK_BIT21 | MASK_BIT22)
#define DSI_VSA_LENGTH_MASK             MASK_QUARTET0
#define DSI_VBP_LENGTH_MASK             MASK_QUARTET1
#define DSI_VFP_LENGTH_MASK             MASK_BYTE1
#define DSI_VACT_LENGTH_MASK            (MASK_BYTE2 | MASK_QUARTET6)
#define DSI_HSA_LENGTH_MASK             MASK_BYTE0
#define DSI_HBP_LENGTH_MASK             MASK_BYTE1
#define DSI_HFP_LENGTH_MASK             (MASK_BYTE2 | MASK_BIT24 | MASK_BIT25 | MASK_BIT26)
#define DSI_RGB_SIZE_MASK               (MASK_BYTE0 | MASK_QUARTET2 | MASK_BIT12)
#define DSI_LINE_POS_MASK               (MASK_BIT0 | MASK_BIT1)
#define DSI_LINE_VAL_MASK               (MASK_BIT2 | MASK_BIT3 | MASK_QUARTET1 | MASK_QUARTET2 | MASK_BIT12)
#define DSI_HORI_POS_MASK               (MASK_BIT0 | MASK_BIT1 |MASK_BIT2)
#define DSI_HORI_VAL_MASK               (MASK_BYTE1 | MASK_QUARTET1 | MASK_BIT3)
#define DSI_VID_MODE_STS_MASK           (MASK_BYTE0 | MASK_BIT8 | MASK_BIT9)
#define DSI_BURST_LP_MASK               MASK_BIT16
#define DSI_MAX_BURST_LIMIT_MASK        MASK_HALFWORD0
#define DSI_MAX_LINE_LIMIT_MASK         MASK_HALFWORD1
#define DSI_EXACT_BURST_LIMIT_MASK      MASK_HALFWORD0
#define DSI_BLKLINE_EVENT_MASK          (MASK_BYTE0 | MASK_QUARTET2 | MASK_BIT12)
#define DSI_BLKEOL_PCK_MASK             (MASK_BYTE2 | MASK_BIT15 | MASK_BIT14 | MASK_BIT13 | MASK_BIT24 | MASK_BIT25)
#define DSI_BLKLINE_PULSE_PCK_MASK      (MASK_BYTE0 | MASK_QUARTET2 | MASK_BIT12)
#define DSI_BLKEOL_DURATION_MASK        (MASK_BYTE0 | MASK_QUARTET2 | MASK_BIT12)
#define DSI_VERT_BLANK_DURATION_MASK    (MASK_BYTE2 | MASK_BIT15 | MASK_BIT14 | MASK_BIT13 | MASK_BIT24 | MASK_BIT25)
#define DSI_COL_RED_MASK                MASK_BYTE0
#define DSI_COL_GREEN_MASK              MASK_BYTE1
#define DSI_COL_BLUE_MASK               MASK_BYTE2
#define DSI_PAD_VAL_MASK                MASK_BYTE3
#define DSI_TVG_STRIPE_MASK             (MASK_BIT5 | MASK_BIT6 | MASK_BIT7)
#define DSI_TVG_MODE_MASK               (MASK_BIT3 | MASK_BIT4 )
#define DSI_TVG_STOPMODE_MASK           (MASK_BIT1 | MASK_BIT2 )
#define DSI_TVG_RUN_MASK                MASK_BIT0
#define DSI_TVG_NBLINE_MASK             (MASK_BYTE2 | MASK_BIT24 | MASK_BIT25 | MASK_BIT26)
#define DSI_TVG_LINE_SIZE_MASK          (MASK_BYTE0 | MASK_QUARTET2 | MASK_BIT12)
#define DSI_CMD_MODE_STATUS_MASK           (MASK_QUARTET0 | MASK_BIT4 | MASK_BIT5 )
#define DSI_DIRECT_CMD_STS_MASK         (MASK_BYTE0 | MASK_BIT8 | MASK_BIT9 | MASK_BIT10 )
#define DSI_DIRECT_CMD_RD_STATUS_MASK      (MASK_BYTE0 | MASK_BIT8 )
#define DSI_VID_MODE_STATUS_MASK           (MASK_BYTE0 | MASK_BIT8 | MASK_BIT9 )
#define DSI_TG_STS_MASK                 (MASK_BIT0 | MASK_BIT1)
#define DSI_CLK_TRIM_RD_MASK            MASK_BIT0
#define DSI_IF1_LPM_EN_MASK		MASK_BIT4
#define DSI_IF2_LPM_EN_MASK		MASK_BIT5
#define	DSIMCTL_DPHY_STATIC_HS_INVERT_DAT2	MASK_BIT5
#define	DSIMCTL_DPHY_STATIC_SWAP_PINS_DAT2	MASK_BIT4
#define	DSIMCTL_DPHY_STATIC_HS_INVERT_DAT1	MASK_BIT3
#define	DSIMCTL_DPHY_STATIC_SWAP_PINS_DAT1	MASK_BIT2
#define	DSIMCTL_DPHY_STATIC_HS_INVERT_CLK	MASK_BIT1
#define	DSIMCTL_DPHY_STATIC_SWAP_PINS_CLK	MASK_BIT0
#define	DSIMCTL_DPHY_STATIC_UI_X4	(MASK_BIT6 | MASK_BIT7 | MASK_QUARTET2)

#define DSI_MCTL_INTERFACE1_MODE_SHIFT  1
#define DSI_MCTL_VID_EN_SHIF            2
#define DSI_MCTL_TVG_SEL_SHIFT          3
#define DSI_MCTL_TBG_SEL_SHIFT          4
#define DSI_MCTL_READEN_SHIFT           8
#define DSI_MCTL_BTAEN_SHIFT            9
#define DSI_MCTL_DISPECCGEN_SHIFT       10
#define DSI_MCTL_DISPCHECKSUMGEN_SHIFT  11
#define DSI_MCTL_HOSTEOTGEN_SHIFT       12
#define DSI_MCTL_DISPEOTGEN_SHIFT       13
#define DSI_PLL_MASTER_SHIFT            16
#define DSI_PLL_OUT_SEL_SHIFT           11
#define DSI_PLL_IN_SEL_SHIFT            10
#define DSI_MCTL_VID_EN_SHIFT			2
#define DSI_PLL_DIV_SHIFT               7
#define DSI_REG_TE_SHIFT                7
#define DSI_IF1_TE_SHIFT                5
#define DSI_IF2_TE_SHIFT                6
#define DSI_FORCE_STOP_MODE_SHIFT       1
#define DSI_CLK_CONTINUOUS_SHIFT        2
#define DSI_CLK_ULPM_EN_SHIFT           3
#define DSI_DAT1_ULPM_EN_SHIFT          4
#define DSI_DAT2_ULPM_EN_SHIFT          5
#define DSI_WAIT_BURST_SHIFT            6
#define DSI_DATALANE1STS_SHIFT          2
#define DSI_DATALANE2STS_SHIFT          5
#define DSI_HSTX_TO_SHIFT               4
#define DSI_LPRX_TO_SHIFT               18
#define DSI_DATA_ULPOUT_SHIFT           9
#define DSI_CKLANE_EN_SHIFT             3
#define DSI_DAT1_EN_SHIFT               4
#define DSI_DAT2_EN_SHIFT               5
#define DSI_CLK_ULPM_SHIFT              6
#define DSI_DAT1_ULPM_SHIFT             7
#define DSI_DAT2_ULPM_SHIFT             8
#define DSI_IF1_EN_SHIFT                9
#define DSI_IF2_EN_SHIFT                10
#define DSI_IF_VALID_SHIFT              16
#define DSI_IF_START_SHIFT              17
#define DSI_IF_FRAME_SYNC_SHIFT         18
#define DSI_RD_ID_SHIFT                 16
#define DSI_RD_DCSNOTGENERIC_SHIFT      18
#define DSI_CMD_LONGNOTSHORT_SHIFT      3
#define DSI_CMD_HEAD_SHIFT              8
#define DSI_CMD_ID_SHIFT                14
#define DSI_CMD_SIZE_SHIFT              16
#define DSI_CMD_LP_EN_SHIFT             21
#define DSI_TRIGGER_VAL_SHIFT           24
#define DSI_TE_LOWERBIT_SHIFT           16
#define DSI_TE_UPPERBIT_SHIFT           24
#define DSI_FIL_VAL_SHIFT               8
#define DSI_ARB_MODE_SHIFT              6
#define DSI_ARB_PRI_SHIFT               7
#define DSI_STOP_MODE_SHIFT             2
#define DSI_VID_ID_SHIFT                4
#define DSI_HEADER_SHIFT                6
#define DSI_PIXEL_MODE_SHIFT            12
#define DSI_BURST_MODE_SHIFT            14
#define DSI_SYNC_PULSE_ACTIVE_SHIFT     15
#define DSI_SYNC_PULSE_HORIZONTAL_SHIFT 16
#define DSI_BLKLINE_SHIFT               17
#define DSI_BLKEOL_SHIFT                19
#define DSI_RECOVERY_MODE_SHIFT         21
#define DSI_VBP_LENGTH_SHIFT            4
#define DSI_VFP_LENGTH_SHIFT            8
#define DSI_VACT_LENGTH_SHIFT           16
#define DSI_HBP_LENGTH_SHIFT            8
#define DSI_HFP_LENGTH_SHIFT            16
#define DSI_LINE_VAL_SHIFT              2
#define DSI_HORI_VAL_SHIFT              3
#define DSI_BURST_LP_SHIFT              16
#define DSI_MAX_LINE_LIMIT_SHIFT        16
#define DSI_BLKEOL_PCK_SHIFT            13
#define DSI_VERT_BLANK_DURATION_SHIFT   13
#define DSI_COL_GREEN_SHIFT             8
#define DSI_COL_BLUE_SHIFT              16
#define DSI_PAD_VAL_SHIFT               24
#define DSI_TVG_STRIPE_SHIFT            1
#define DSI_TVG_MODE_SHIFT              3
#define DSI_TVG_STOPMODE_SHIFT          5
#define DSI_TVG_NBLINE_SHIFT            16
#define	DSIMCTL_DPHY_STATIC_SWAP_PINS_CLK_SHIFT 	0
#define	DSIMCTL_DPHY_STATIC_HS_INVERT_CLK_SHIFT		1
#define	DSIMCTL_DPHY_STATIC_SWAP_PINS_DAT1_SHIFT	2
#define	DSIMCTL_DPHY_STATIC_HS_INVERT_DAT1_SHIFT	3
#define	DSIMCTL_DPHY_STATIC_SWAP_PINS_DAT2_SHIFT	4
#define	DSIMCTL_DPHY_STATIC_HS_INVERT_DAT2_SHIFT	5
#define DSIMCTL_DPHY_STATIC_UI_X4_SHIFT			6
#define DSI_IF1_LPM_EN_MASK_SHIFT		4
#define DSI_IF2_LPM_EN_MASK_SHIFT		5

#define	DSI_MCTL_MAIN_STS_VRS_UNTERM_PCK	0x80
#define	DSI_MCTL_MAIN_STS_VRS_UNTERM_PCK_SHIFT	7

#define	DSI_MCTL_MAIN_STS_CRS_UNTERM_PCK	0x40
#define	DSI_MCTL_MAIN_STS_CRS_UNTERM_PCK_SHIFT	6

#define	DSI_MCTL_MAIN_STS_LPRX_TO_ERR		0x20
#define	DSI_MCTL_MAIN_STS_LPRX_TO_ERR_SHIFT	5

#define	DSI_MCTL_MAIN_STS_HSTX_TO_ERR		0x10
#define	DSI_MCTL_MAIN_STS_HSTX_TO_ERR_SHIFT	4

#define	DSI_MCTL_MAIN_STS_DAT2_READY		0x8
#define	DSI_MCTL_MAIN_STS_DAT2_READY_SHIFT	3

#define	DSI_MCTL_MAIN_STS_DAT1_READY		0x4
#define	DSI_MCTL_MAIN_STS_DAT1_READY_SHIFT	2

#define	DSI_MCTL_MAIN_STS_CLKLANE_READY		0x2
#define	DSI_MCTL_MAIN_STS_CLKLANE_READY_SHIFT	1

#define	DSI_MCTL_MAIN_STS_PLL_LOCK		0x1
#define	DSI_MCTL_MAIN_STS_PLL_LOCK_SHIFT	0
/** Test mode conf */

//**********************************************************************************************************************
/** - DIRECT_CMD_WRDAT0 */
//**********************************************************************************************************************

#define	DSIDIRECT_CMD_WRDAT0_WRDAT3	(0xFF000000)
#define	Shift_DSIDIRECT_CMD_WRDAT0_WRDAT3	(24)
#define	DSIDIRECT_CMD_WRDAT0_WRDAT2	(0xFF0000)
#define	Shift_DSIDIRECT_CMD_WRDAT0_WRDAT2	(16)
#define	DSIDIRECT_CMD_WRDAT0_WRDAT1	(0xFF00)
#define	Shift_DSIDIRECT_CMD_WRDAT0_WRDAT1	(8)
#define	DSIDIRECT_CMD_WRDAT0_WRDAT0	(0xFF)
#define	Shift_DSIDIRECT_CMD_WRDAT0_WRDAT0	(0)

//**********************************************************************************************************************
/** - DIRECT_CMD_WRDAT1 */
//**********************************************************************************************************************

#define	DSIDIRECT_CMD_WRDAT1_WRDAT7	(0xFF000000)
#define	Shift_DSIDIRECT_CMD_WRDAT1_WRDAT7	(24)
#define	DSIDIRECT_CMD_WRDAT1_WRDAT6	(0xFF0000)
#define	Shift_DSIDIRECT_CMD_WRDAT1_WRDAT6	(16)
#define	DSIDIRECT_CMD_WRDAT1_WRDAT5	(0xFF00)
#define	Shift_DSIDIRECT_CMD_WRDAT1_WRDAT5	(8)
#define	DSIDIRECT_CMD_WRDAT1_WRDAT4	(0xFF)
#define	Shift_DSIDIRECT_CMD_WRDAT1_WRDAT4	(0)


//**********************************************************************************************************************
/** - DIRECT_CMD_WRDAT2 */
//**********************************************************************************************************************

#define	DSIDIRECT_CMD_WRDAT2_WRDAT11	(0xFF000000)
#define	Shift_DSIDIRECT_CMD_WRDAT2_WRDAT11	(24)
#define	DSIDIRECT_CMD_WRDAT2_WRDAT10	(0xFF0000)
#define	Shift_DSIDIRECT_CMD_WRDAT2_WRDAT10	(16)
#define	DSIDIRECT_CMD_WRDAT2_WRDAT9	(0xFF00)
#define	Shift_DSIDIRECT_CMD_WRDAT2_WRDAT9	(8)
#define	DSIDIRECT_CMD_WRDAT2_WRDAT8	(0xFF)
#define	Shift_DSIDIRECT_CMD_WRDAT2_WRDAT8	(0)

//**********************************************************************************************************************
/** - DIRECT_CMD_WRDAT3 */
//**********************************************************************************************************************

#define	DSIDIRECT_CMD_WRDAT3_WRDAT15	(0xFF000000)
#define	Shift_DSIDIRECT_CMD_WRDAT3_WRDAT15	(24)
#define	DSIDIRECT_CMD_WRDAT3_WRDAT14	(0xFF0000)
#define	Shift_DSIDIRECT_CMD_WRDAT3_WRDAT14	(16)
#define	DSIDIRECT_CMD_WRDAT3_WRDAT13	(0xFF00)
#define	Shift_DSIDIRECT_CMD_WRDAT3_WRDAT13	(8)
#define	DSIDIRECT_CMD_WRDAT3_WRDAT12	(0xFF)
#define	Shift_DSIDIRECT_CMD_WRDAT3_WRDAT12	(0)
//**********************************************************************************************************************
/** - DIRECT_CMD_READ */
//**********************************************************************************************************************

#define	DSIDIRECT_CMD_RDAT3	(0xFF000000)
#define	Shift_DSIDIRECT_CMD_RDAT3	(24)
#define	DSIDIRECT_CMD_RDAT2	(0xFF0000)
#define	Shift_DSIDIRECT_CMD_RDAT2	(16)
#define	DSIDIRECT_CMD_RDAT1	(0xFF00)
#define	Shift_DSIDIRECT_CMD_RDAT1	(8)
#define	DSIDIRECT_CMD_RDAT0	(0xFF)
#define	Shift_DSIDIRECT_CMD_RDAT0	(0)

/** TPO COMMAND HEADER */
#define  TPO_CMD_NONE 0x00
#define  TPO_CMD_SWRESET 0x01 /** SWRESET: Software Reset (01h) */
#define  TPO_CMD_SLPOUT 0x11  /**  SLPOUT: Sleep Out (11h) */
#define  TPO_CMD_NORON 0x13   /**  NORON: Normal Display Mode On (13h) */
#define  TPO_CMD_INVOFF 0x20  /**  INVOFF: Display Inversion Off (20h) */
#define  TPO_CMD_INVON 0x21   /**  INVOFF: Display Inversion Off (20h) */
#define  TPO_CMD_GAMMA_SET 0x26 /** Gamma set//reset GC0G2.2 */

#define  TPO_CMD_DISPOFF 0x28 /** DISPON: Display On (29h) */
#define  TPO_CMD_DISPON 0x29  /** DISPON: Display On (29h) */
#define  TPO_CMD_CASET  0x2A  /** CASET :Columen address select */
#define  TPO_CMD_RASET  0x2B  /** RASET :Row address select */
#define  TPO_CMD_RAMWR  0x2C  /** RAMWR ram write */

#define  TPO_CMD_MADCTR 0x36  /** MADCTR: Memory Data Access Control (36h) */
#define  TPO_CMD_IDMOFF 0x38  /** IDMON: Idle Mode On (39h) */
#define  TPO_CMD_IDMON 0x39   /** IDMON: Idle Mode On (39h) */
#define  TPO_CMD_COLMOD 0x3A  /** COLMOD: Interface Pixel Format (3Ah). */
#define  TPO_CMD_RAMWR_CONTINUE 0x3C /** Ram write continue */
#define  TPO_CMD_IFMODE 0xB0  /** IFMODE: Set Display Interface Mode (B0h) */
#define  TPO_CMD_DISSET6 0xB7 /** Display Function Setting 6 (B7h) */
#define  TPO_CMD_LPTS_FUNCTION_SET3 0xBC  /** LPTS_FUNCTION_SET3  (0xBC) */
#define  TPO_CMD_DSLPOUT 0xCA  /** deep sleepout 0xca */


#define  TPO_CMD_GAMCTRP1 0xE0    /** GAMCTRP1: Set Positive Gamma Correction Characteristics (E0h) */
#define  TPO_CMD_GAMCTRN1 0xE1    /** GAMCTRN1: Set Negative Gamma Correction Characteristics (E1h) */
#define  TPO_CMD_GAMCTRP2 0xE2    /** GAMCTRP2: Gamma (‘+’polarity) Correction Characteristics Setting (E2h) */
#define  TPO_CMD_GAMCTRN2 0xE3    /** GAMCTRN2: Gamma (‘-’polarity) Correction Characteristics Setting (E3h) */
#define  TPO_CMD_GAMCTRP3 0xE4    /** GAMCTRP3: Gamma (‘+’polarity) Correction Characteristics Setting (E4h) */
#define  TPO_CMD_GAMCTRN3 0xE5    /** GAMCTRN3: Gamma (‘-’polarity) Correction Characteristics Setting (E5h) */
#define  TPO_CMD_GAM_R_SEL 0xEA   /** GAMMA SELECTION */



#define VC_ID0			0
#define VC_ID1			1




struct dsi_link_registers
{
    /** Main control registers */
    volatile u32 mctl_integration_mode;
    volatile u32 mctl_main_data_ctl;
    volatile u32 mctl_main_phy_ctl;
    volatile u32 mctl_pll_ctl;
    volatile u32 mctl_lane_sts;
    volatile u32 mctl_dphy_timeout;
    volatile u32 mctl_ulpout_time;
    volatile u32 mctl_dphy_static;
    volatile u32 mctl_main_en;
    volatile u32 mctl_main_sts;
    volatile u32 mctl_dphy_err;

    volatile u32 reserved1;
    /**
     integration mode registers */
    volatile u32 int_vid_rddata;
    volatile u32 int_vid_gnt;
    volatile u32 int_cmd_rddata;
    volatile u32 int_cmd_gnt;
    volatile u32 int_interrupt_ctl;
    volatile u32 reserved2[3];
    /**
     Command mode registers */
    volatile u32 cmd_mode_ctl;
    volatile u32 cmd_mode_sts;
    volatile u32 reserved3[2];
    /**
      Direct Command registers */
    volatile u32 direct_cmd_send;
    volatile u32 direct_cmd_main_settings;
    volatile u32 direct_cmd_sts;
    volatile u32 direct_cmd_rd_init;
    volatile u32 direct_cmd_wrdat0;
    volatile u32 direct_cmd_wrdat1;
    volatile u32 direct_cmd_wrdat2;
    volatile u32 direct_cmd_wrdat3;
    volatile u32 direct_cmd_rddat;
    volatile u32 direct_cmd_rd_property;
    volatile u32 direct_cmd_rd_sts;
    volatile u32 reserved4;
    /**
      Video mode registers */
    volatile u32 vid_main_ctl;
    volatile u32 vid_vsize;
    volatile u32 vid_hsize1;
    volatile u32 vid_hsize2;
    volatile u32 vid_blksize1;
    volatile u32 vid_blksize2;
    volatile u32 vid_pck_time;
    volatile u32 vid_dphy_time;
    volatile u32 vid_err_color;
    volatile u32 vid_vpos;
    volatile u32 vid_hpos;
    volatile u32 vid_mode_sts;
    volatile u32 vid_vca_setting1;
    volatile u32 vid_vca_setting2;
    /**
      Test Video Mode regsiter */
    volatile u32 tvg_ctl;
    volatile u32 tvg_img_size;
    volatile u32 tvg_color1;
    volatile u32 tvg_color2;
    volatile u32 tvg_sts;
    volatile u32 reserved5;
    /**
      Test Byte generator register */
    volatile u32 tbg_ctl;
    volatile u32 tbg_setting;
    volatile u32 tbg_sts;
    volatile u32 reserved6;
    /**
      Interrupt Enable and Edge detection register */
    volatile u32 mctl_main_sts_ctl;
    volatile u32 cmd_mode_sts_ctl;
    volatile u32 direct_cmd_sts_ctl;
    volatile u32 direct_cmd_rd_sts_ctl;
    volatile u32 vid_mode_sts_ctl;
    volatile u32 tg_sts_ctl;
    volatile u32 mctl_dphy_err_ctl;
    volatile u32 dphy_clk_trim_rd_ctl;
    /**
      Error/Interrupt Clear Register */
    volatile u32 mctl_main_sts_clr;
    volatile u32 cmd_mode_sts_clr;
    volatile u32 direct_cmd_sts_clr;
    volatile u32 direct_cmd_rd_sts_clr;
    volatile u32 vid_mode_sts_clr;
    volatile u32 tg_sts_clr;
    volatile u32 mctl_dphy_err_clr;
    volatile u32 dphy_clk_trim_rd_clr;
    /**
      Flag registers */
    volatile u32 mctl_main_sts_flag;
    volatile u32 cmd_mode_sts_flag;
    volatile u32 direct_cmd_sts_flag;
    volatile u32 direct_cmd_rd_sts_flag;
    volatile u32 vid_mode_sts_flag;
    volatile u32 tg_sts_flag;
    volatile u32 mctl_dphy_err_flag;
    volatile u32 dphy_clk_trim_rd_flag;
    volatile u32 dhy_lanes_trim;
};



#ifdef _cplusplus
}
#endif /* _cplusplus */

#endif /* !defined(_DSI_H_) */


