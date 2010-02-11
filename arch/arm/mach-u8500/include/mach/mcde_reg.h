/*---------------------------------------------------------------------------*/
/* Copyrighti (C) STEricsson 2009. 				  	     */
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

#ifndef _MCDEREG_H_
#define _MCDEREG_H_

#include <linux/types.h>
#include <mach/bit_mask.h>

/*******************************************************************
   MCDE Control Register Fields
********************************************************************/
#define  MCDE_DISABLE 0x00000000
#define  MCDE_ENABLE  0x80000000
#define MCDE_SET_BIT  0x1
#define MCDE_CLEAR_BIT 0x0
#define MCDE_CTRL_MCDEEN_MASK   MASK_BIT31
#define MCDE_CTRL_MCDEEN_SHIFT  31
#define MCDE_CTRL_FABMUX_MASK   MASK_BIT17
#define MCDE_CTRL_FABMUX_SHIFT  17
#define MCDE_CTRL_F01MUX_MASK   MASK_BIT16
#define MCDE_CTRL_F01MUX_SHIFT  16
#define MCDE_CTRL_IFIFICTRL_MASK   MASK_BIT15
#define MCDE_CTRL_IFIFICTRL_SHIFT   15
#define MCDE_CTRL_DPIA_EN_MASK   MASK_BIT9
#define MCDE_CTRL_DPIA_EN_SHIFT   9
#define MCDE_CTRL_DPIB_EN_MASK   MASK_BIT8
#define MCDE_CTRL_DPIB_EN_SHIFT   8
#define MCDE_CTRL_DBIC0_EN_MASK   MASK_BIT7
#define MCDE_CTRL_DBIC0_EN_SHIFT   7
#define MCDE_CTRL_DBIC1_EN_MASK   MASK_BIT6
#define MCDE_CTRL_DBIC1_EN_SHIFT   6
#define MCDE_DSIVID0_EN_MASK   MASK_BIT5
#define MCDE_DSIVID1_EN_MASK   MASK_BIT4
#define MCDE_DSIVID2_EN_MASK   MASK_BIT3
#define MCDE_DSICMD0_EN_MASK   MASK_BIT2
#define MCDE_DSICMD1_EN_MASK   MASK_BIT1
#define MCDE_DSICMD2_EN_MASK   MASK_BIT0
#define MCDE_DSIVID0_EN_SHIFT    5
#define MCDE_DSIVID1_EN_SHIFT    4
#define MCDE_DSIVID2_EN_SHIFT    3
#define MCDE_DSICMD0_EN_SHIFT    2
#define MCDE_DSICMD1_EN_SHIFT    1
#define MCDE_SYNCMUX_MASK          0xFF
#define MCDE_TVA_DPIC0_LCDB_MASK   0x06
#define MCDE_TVB_DPIC1_LCDA_MASK   0xD4
#define MCDE_DPIC1_LCDA_MASK       0xF8
#define MCDE_DPIC0_LCDB_MASK       0x07
#define MCDE_LCDA_LCDB_MASK        0x00
#define MCDE_DSI_MASK		   0x01
#define MCDE_OVR_ALPHAPMEN_SHIFT 6
#define MCDE_OVR_CLIPEN_SHIFT    7

#define MCDE_CFG_OUTMUX4_MASK  (MASK_BIT28 | MASK_BIT29 | MASK_BIT30)
#define MCDE_CFG_OUTMUX3_MASK  (MASK_BIT25 | MASK_BIT26 | MASK_BIT27)
#define MCDE_CFG_OUTMUX2_MASK  (MASK_BIT22 | MASK_BIT23 | MASK_BIT24)
#define MCDE_CFG_OUTMUX1_MASK  (MASK_BIT19 | MASK_BIT20 | MASK_BIT21)
#define MCDE_CFG_OUTMUX0_MASK  (MASK_BIT16 | MASK_BIT17 | MASK_BIT18)
#define MCDE_CFG_IFIFOCTRLWTRMRKLVL_MASK (MASK_BIT12 | MASK_BIT13 | MASK_BIT14)
#define MCDE_CFG_FSYNCTRLB_MASK MASK_BIT11
#define MCDE_CFG_FSYNCTRLA_MASK MASK_BIT10
#define MCDE_CFG_SWAP_B_C1_MASK     MASK_BIT9
#define MCDE_CFG_SWAP_A_C0_MASK     MASK_BIT8
#define MCDE_CFG_SYNCMUX7_MASK  MASK_BIT7
#define MCDE_CFG_SYNCMUX6_MASK  MASK_BIT6
#define MCDE_CFG_SYNCMUX5_MASK  MASK_BIT5
#define MCDE_CFG_SYNCMUX4_MASK  MASK_BIT4
#define MCDE_CFG_SYNCMUX3_MASK  MASK_BIT3
#define MCDE_CFG_SYNCMUX2_MASK  MASK_BIT2
#define MCDE_CFG_SYNCMUX1_MASK  MASK_BIT1
#define MCDE_CFG_SYNCMUX0_MASK  MASK_BIT0

#define MCDE_CFG_OUTMUX4_SHIFT  28
#define MCDE_CFG_OUTMUX3_SHIFT  25
#define MCDE_CFG_OUTMUX2_SHIFT  22
#define MCDE_CFG_OUTMUX1_SHIFT  19
#define MCDE_CFG_OUTMUX0_SHIFT  16
#define MCDE_CFG_IFIFOCTRLWTRMRKLVL_SHIFT 12
#define MCDE_CFG_FSYNCTRLB_SHIFT 11
#define MCDE_CFG_FSYNCTRLA_SHIFT 10
#define MCDE_CFG_SWAP_B_C1_SHIFT  9
#define MCDE_CFG_SWAP_A_C0_SHIFT  8
#define MCDE_CFG_SYNCMUX7_SHIFT 7
#define MCDE_CFG_SYNCMUX6_SHIFT 6
#define MCDE_CFG_SYNCMUX5_SHIFT 5
#define MCDE_CFG_SYNCMUX4_SHIFT 4
#define MCDE_CFG_SYNCMUX3_SHIFT 3
#define MCDE_CFG_SYNCMUX2_SHIFT 2
#define MCDE_CFG_SYNCMUX1_SHIFT 1
#define MCDE_CFG_SYNCMUX0_SHIFT 0


/*******************************************************************
   MCDE External Source Register Fields
********************************************************************/
#define MCDE_EXT_BUFFER_MASK      /*(MASK_HALFWORD1 | MASK_BYTE1 | MASK_QUARTET1 | MASK_BIT3)*/0xFFFFFFFF
#define MCDE_EXT_PRI_OVR_MASK     MASK_QUARTET1
#define MCDE_EXT_BUFFER_NUM_MASK  (MASK_BIT2 | MASK_BIT3)
#define MCDE_EXT_BUFFER_ID_MASK   (MASK_BIT0 | MASK_BIT1)
#define MCDE_EXT_FORCEFSDIV_MASK   MASK_BIT4
#define MCDE_EXT_FSDISABLE_MASK    MASK_BIT3
#define MCDE_EXT_OVR_CTRL_MASK     MASK_BIT2
#define MCDE_EXT_BUF_MODE_MASK     (MASK_BIT0 | MASK_BIT1)
#define MCDE_EXT_BEPO_MASK        MASK_BIT14
#define MCDE_EXT_BEBO_MASK        MASK_BIT13
#define MCDE_EXT_BGR_MASK         MASK_BIT12
#define MCDE_EXT_BPP_MASK         (MASK_BIT11 | MASK_BIT10 | MASK_BIT9 | MASK_BIT8)


#define MCDE_EXT_BUFFER_SHIFT    0
#define MCDE_EXT_PRI_OVR_SHIFT   SHIFT_QUARTET1
#define MCDE_EXT_BUFFER_NUM_SHIFT 2
#define MCDE_EXT_OVR_CTRL_SHIFT   2
#define MCDE_EXT_FORCEFSDIV_SHIFT 4
#define MCDE_EXT_FSDISABLE_SHIFT  3
#define MCDE_EXT_BEPO_SHIFT       14
#define MCDE_EXT_BEBO_SHIFT       13
#define MCDE_EXT_BGR_SHIFT        12
#define MCDE_EXT_BPP_SHIFT        8
/*******************************************************************
   MCDE Overlay Register Fields
********************************************************************/
#define MCDE_OVR_OVLEN_MASK        MASK_BIT0
#define MCDE_OVR_COLCTRL_MASK     (MASK_BIT1 | MASK_BIT2)
#define MCDE_OVR_PALCTRL_MASK     (MASK_BIT3 | MASK_BIT4)
#define MCDE_OVR_CKEYEN_MASK      (MASK_BIT5)
#define MCDE_OVR_STBPRIO_MASK     MASK_QUARTET4
#define MCDE_OVR_BURSTSZ_MASK     MASK_QUARTET5
#define MCDE_OVR_MAXREQ_MASK      MASK_QUARTET6
#define MCDE_OVR_ROTBURSTSIZE_MASK MASK_QUARTET7
#define MCDE_OVR_BLEND_MASK       MASK_BIT0
#define MCDE_OVR_OPQ_MASK         MASK_BIT9
#define MCDE_OVR_INTERMDE_MASK    MASK_BIT29
#define MCDE_OVR_INTERON_MASK     MASK_BIT28
#define MCDE_OVR_ALPHAPMEN_MASK   MASK_BIT6
#define MCDE_OVR_CLIPEN_MASK      MASK_BIT7
#define MCDE_OVR_LPF_MASK        (MASK_BYTE2 | MASK_BIT24 |MASK_BIT25 | MASK_BIT26)
#define MCDE_OVR_PPL_MASK         (MASK_BYTE0 | MASK_BIT8 |MASK_BIT9 |MASK_BIT10)
#define MCDE_ALPHAVALUE_MASK      (MASK_BYTE0 << 1)
/**#define MCDE_EXT_SRCID_MASK       (MASK_BIT9 | MASK_BIT8 | MASK_BIT7 | MASK_BIT6)*/
#define MCDE_EXT_SRCID_MASK       (MASK_BIT14 | MASK_BIT13 | MASK_BIT12 | MASK_BIT11)
#define MCDE_PIXOFF_MASK          (MASK_BIT10 | MASK_BIT11 |MASK_QUARTET3)
#define MCDE_OVR_ZLEVEL_MASK      (MASK_BIT30 | MASK_BIT27 | MASK_BIT28 | MASK_BIT29)
#define MCDE_OVR_YPOS_MASK        (MASK_BYTE2 | MASK_BIT24 | MASK_BIT25 | MASK_BIT26)
#define MCDE_OVR_CHID_MASK        (MASK_BIT14 | MASK_BIT13 | MASK_BIT12 | MASK_BIT11)
#define MCDE_OVR_XPOS_MASK        (MASK_BYTE0 | MASK_BIT8 |MASK_BIT9 |MASK_BIT10)
#define MCDE_OVR_READ_MASK        MASK_BIT9
#define MCDE_OVR_FETCH_MASK       MASK_BIT8
#define MCDE_OVR_BLOCKED_MASK     MASK_BIT10
//#define MCDE_OVR_READ_MASK        MASK_BIT1
//#define MCDE_OVR_FETCH_MASK       MASK_BIT0
#define MCDE_WATERMARK_MASK      (MASK_BYTE2 | MASK_QUARTET6 | MASK_BIT28)
#define MCDE_LINEINCREMENT_MASK   0xFFFFFFFF
#define MCDE_YCLIP_MASK           0xFFFFFFFF
#define MCDE_XCLIP_MASK           0xFFFFFFFF
#define MCDE_XBRCOOR_MASK         0x7FF
#define MCDE_YBRCOOR_MASK         0x07FF0000

#define MCDE_OVR_COLCTRL_SHIFT   1
#define MCDE_OVR_PALCTRL_SHIFT   3
#define MCDE_OVR_CKEYEN_SHIFT    5
#define MCDE_OVR_STBPRIO_SHIFT   SHIFT_QUARTET4
#define MCDE_OVR_BURSTSZ_SHIFT   SHIFT_QUARTET5
#define MCDE_OVR_MAXREQ_SHIFT     SHIFT_QUARTET6
#define MCDE_OVR_ROTBURSTSIZE_SHIFT SHIFT_QUARTET7
#define MCDE_OVR_OPQ_SHIFT        9
#define MCDE_OVR_INTERMDE_SHIFT   29
#define MCDE_OVR_INTERON_SHIFT    28
#define MCDE_OVR_LPF_SHIFT        SHIFT_HALFWORD1
#define MCDE_ALPHAVALUE_SHIFT     1
//#define MCDE_EXT_SRCID_SHIFT      6
#define MCDE_EXT_SRCID_SHIFT      11
#define MCDE_OVR_ZLEVEL_SHIFT     27
#define MCDE_OVR_YPOS_SHIFT       SHIFT_HALFWORD1
#define MCDE_OVR_CHID_SHIFT       11
#define MCDE_OVR_READ_SHIFT       1
#define MCDE_WATERMARK_SHIFT      SHIFT_HALFWORD1
#define MCDE_PIXOFF_SHIFT         10
#define MCDE_LINEINCREMENT_SHIFT  0
#define MCDE_YCLIP_SHIFT          0
#define MCDE_XCLIP_SHIFT          0
#define MCDE_YBRCOOR_SHIFT        16

/*******************************************************************
   MCDE Channel Configuration Register Fields
********************************************************************/
#define MCDE_INITDELAY_MASK      MASK_HALFWORD1
#define MCDE_PPDELAY_MASK        MASK_HALFWORD0
#define MCDE_SWINTVCNT_MASK      (MASK_BYTE3 | MASK_QUARTET5 | MASK_BIT18 |MASK_BIT19)
#define MCDE_SWINTVEVENT_MASK    (MASK_BIT16 | MASK_BIT17)
#define MCDE_HWREQVCNT_MASK      (MASK_BYTE1 | MASK_QUARTET1 | MASK_BIT3 | MASK_BIT2)
#define MCDE_HWREQVEVENT_MASK    (MASK_BIT0 | MASK_BIT1)
#define MCDE_OUTINTERFACE_MASK   (MASK_BIT4 | MASK_BIT3 |MASK_BIT2)
#define MCDE_SRCSYNCH_MASK       (MASK_BIT0 | MASK_BIT1)
#define MCDE_SW_TRIG_MASK        MASK_BIT0
#define MCDE_REDCOLOR_MASK       MASK_BYTE2
#define MCDE_GREENCOLOR_MASK     MASK_BYTE1
#define MCDE_BLUECOLOR_MASK      MASK_BYTE0
#define MCDE_CHPRIORITY_MASK     MASK_QUARTET0
#define MCDE_CHXLPF_MASK         (0x03FF0000)
#define MCDE_CHXPPL_MASK         (MASK_BYTE0 | MASK_BIT8 |MASK_BIT9 |MASK_BIT10)
#define MCDE_CHX_ABORT_MASK       MASK_BIT1
#define MCDE_CHX_READ_MASK        MASK_BIT0

#define MCDE_INITDELAY_SHIFT     SHIFT_HALFWORD1
#define MCDE_SWINTVCNT_SHIFT     18
#define MCDE_SWINTVEVENT_SHIFT   SHIFT_HALFWORD1
#define MCDE_HWREQVCNT_SHIFT     2
#define MCDE_OUTINTERFACE_SHIFT  2
#define MCDE_REDCOLOR_SHIFT      SHIFT_HALFWORD1
#define MCDE_GREENCOLOR_SHIFT    SHIFT_BYTE1
#define MCDE_CHPRIORITY_SHIFT    SHIFT_QUARTET7
#define MCDE_CHXLPF_SHIFT        16
#define MCDE_CHX_ABORT_SHIFT     1

/*******************************************************************
   MCDE Channel A/B Register Fields
********************************************************************/
#define MCDE_CHX_BURSTSIZE_MASK   (MASK_QUARTET6 & 0x07000000)
#define MCDE_CHX_ALPHA_MASK       (MASK_BYTE2)
#define MCDE_CHX_ROTDIR_MASK      (MASK_BIT15)
#define MCDE_CHX_GAMAEN_MASK      (MASK_BIT14)
#define MCDE_FLICKFORMAT_MASK     (MASK_BIT13)
#define MCDE_FLICKMODE_MASK       (MASK_BIT11 | MASK_BIT12)
#define MCDE_BLENDCONTROL_MASK    (MASK_BIT10)
#define MCDE_KEYCTRL_MASK         (MASK_BIT7|MASK_BIT8|MASK_BIT9)
#define MCDE_ROTEN_MASK           (MASK_BIT6)
#define MCDE_DITHEN_MASK          (MASK_BIT5)
#define MCDE_CEAEN_MASK           (MASK_BIT4)
#define MCDE_AFLICKEN_MASK        (MASK_BIT3)
#define MCDE_BLENDEN_MASK         (MASK_BIT2)
#define MCDE_CLK_MASK             (MASK_BIT30)
#define MCDE_BCD_MASK             (MASK_BIT29)
#define MCDE_OUTBPP_MASK          (MASK_BIT25 |MASK_BIT26 | MASK_BIT27 | MASK_BIT28)
#define MCDE_CDWIN_MASK           (MASK_BIT13 | MASK_BIT14 | MASK_BIT15)
#define MCDE_CLOCKSEL_MASK        (MASK_BIT12 | MASK_BIT11 | MASK_BIT10)
#define MCDE_PCD_MASK             (MASK_BYTE0 | MASK_BIT8 | MASK_BIT9)
#define MCDE_KEYA_MASK            (MASK_BYTE3)
#define MCDE_KEYR_MASK            (MASK_BYTE2)
#define MCDE_KEYG_MASK            (MASK_BYTE1)
#define MCDE_KEYB_MASK            (MASK_BYTE0)
#define MCDE_RGB_MASK1            (MASK_BYTE2 | MASK_BIT24 | MASK_BIT25)
#define MCDE_RGB_MASK2            (MASK_BYTE0 | MASK_BIT8 | MASK_BIT9)
#define MCDE_THRESHOLD_MASK       (MASK_QUARTET6)
#define MCDE_COEFFN3_MASK         (MASK_BYTE2)
#define MCDE_COEFFN2_MASK         (MASK_BYTE1)
#define MCDE_COEFFN1_MASK         (MASK_BYTE0)
#define MCDE_TV_LINES_MASK        (MASK_BYTE2 | MASK_BIT24 | MASK_BIT25 | MASK_BIT26)
#define MCDE_TVMODE_MASK          (MASK_BIT3 | MASK_BIT4)
#define MCDE_IFIELD_MASK          (MASK_BIT2)
#define MCDE_INTEREN_MASK         (MASK_BIT1)
#define MCDE_SELMODE_MASK         (MASK_BIT0)
#define MCDE_BSL_MASK             (MASK_BYTE2 | MASK_BIT24 | MASK_BIT25 | MASK_BIT26)
#define MCDE_BEL_MASK             (MASK_BYTE0 | MASK_BIT8 | MASK_BIT9 | MASK_BIT10)
#define MCDE_FSL2_MASK            (MASK_BYTE2 | MASK_BIT24 | MASK_BIT25 | MASK_BIT26)
#define MCDE_FSL1_MASK            (MASK_BYTE0 | MASK_BIT8 | MASK_BIT9 | MASK_BIT10)
#define MCDE_DVO2_MASK            (MASK_BYTE2 | MASK_BIT24 | MASK_BIT25 | MASK_BIT26)
#define MCDE_DVO1_MASK            (MASK_BYTE0 | MASK_BIT8 | MASK_BIT9 | MASK_BIT10)
#define MCDE_SWH2_MASK            (MASK_BYTE2 | MASK_BIT24 | MASK_BIT25 | MASK_BIT26)
#define MCDE_SWH1_MASK            (MASK_BYTE0 | MASK_BIT8 | MASK_BIT9 | MASK_BIT10)
#define MCDE_SWW_MASK             (MASK_BYTE2 | MASK_BIT24 | MASK_BIT25 | MASK_BIT26)
#define MCDE_DHO_MASK             (MASK_BYTE0 | MASK_BIT8 | MASK_BIT9 | MASK_BIT10)
#define MCDE_ALW_MASK             (MASK_BYTE2 | MASK_BIT24 | MASK_BIT25 | MASK_BIT26)
#define MCDE_LBW_MASK             (MASK_BYTE0 | MASK_BIT8 | MASK_BIT9 | MASK_BIT10)
#define MCDE_TVBCR_MASK            MASK_BYTE2
#define MCDE_TVBCB_MASK            MASK_BYTE1
#define MCDE_TVBLU_MASK            MASK_BYTE0
#define MCDE_REVVAEN_MASK          MASK_BIT31
#define MCDE_REVTGEN_MASK          MASK_BIT30
#define MCDE_REVLOADSEL_MASK       (MASK_BIT28 | MASK_BIT29)
#define MCDE_REVDEL1_MASK          (MASK_QUARTET6)
#define MCDE_REVDEL0_MASK          (MASK_BYTE2)
#define MCDE_PSVAEN_MASK           (MASK_BIT15)
#define MCDE_PSTGEN_MASK           (MASK_BIT14)
#define MCDE_PSLOADSEL_MASK        (MASK_BIT12 | MASK_BIT13)
#define MCDE_PSDEL1_MASK           (MASK_QUARTET2)
#define MCDE_PSDEL0_MASK           (MASK_BYTE0)
#define MCDE_IOE_MASK              (MASK_BIT23)
#define MCDE_IPC_MASK              (MASK_BIT22)
#define MCDE_IHS_MASK              (MASK_BIT21)
#define MCDE_IVS_MASK              (MASK_BIT20)
#define MCDE_IVP_MASK              (MASK_BIT19)
#define MCDE_ICLSPL_MASK           (MASK_BIT18)
#define MCDE_ICLREV_MASK           (MASK_BIT17)
#define MCDE_ICLSP_MASK            (MASK_BIT16)
#define MCDE_SPLVAEN_MASK          (MASK_BIT15)
#define MCDE_SPLTGEN_MASK          (MASK_BIT14)
#define MCDE_SPLLOADSEL_MASK       (MASK_BIT12 | MASK_BIT13)
#define MCDE_SPLDEL1_MASK          (MASK_QUARTET2)
#define MCDE_SPLDEL0_MASK          (MASK_BYTE0)
#define MCDE_FOFFY_MASK            (MASK_BIT14 | MASK_BIT13 | MASK_BIT12 | MASK_BIT11 | MASK_BIT10)
#define MCDE_FOFFX_MASK            (MASK_BIT5 | MASK_BIT6 | MASK_BIT7 | MASK_BIT8 | MASK_BIT9)
#define MCDE_MASK_BITCTRL_MASK     (MASK_BIT4)
#define MCDE_MODE_MASK             (MASK_BIT2 | MASK_BIT3)
#define MCDE_COMP_MASK             (MASK_BIT1)
#define MCDE_TEMP_MASK             (MASK_BIT0)
#define MCDE_YB_MASK               (MASK_BIT28 | MASK_BIT27 | MASK_BIT26 | MASK_BIT25 | MASK_BIT24)
#define MCDE_XB_MASK               (MASK_BIT20 | MASK_BIT19 | MASK_BIT18 | MASK_BIT17 | MASK_BIT16)
#define MCDE_YG_MASK               (MASK_BIT12 | MASK_BIT11 | MASK_BIT10 | MASK_BIT9 | MASK_BIT8 | MASK_BIT7 | MASK_BIT6 | MASK_BIT5)
#define MCDE_XG_MASK               (MASK_BIT0 | MASK_BIT1 | MASK_BIT2 | MASK_BIT3 | MASK_BIT4)
#define MCDE_ARED_MASK             (MASK_BYTE2 | MASK_BIT24 | MASK_BIT25)
#define MCDE_GREEN_MASK            (MASK_BYTE1)
#define MCDE_BLUE_MASK             (MASK_BYTE0)
#define MCDE_RED_MASK              (MASK_BYTE2)

#define MCDE_CHX_BURSTSIZE_SHIFT  SHIFT_QUARTET6
#define MCDE_CHX_ALPHA_SHIFT      SHIFT_HALFWORD1
#define MCDE_CHX_ROTDIR_SHIFT     15
#define MCDE_CHX_GAMAEN_SHIFT     14
#define MCDE_FLICKFORMAT_SHIFT    13
#define MCDE_FLICKMODE_SHIFT      11
#define MCDE_BLENDCONTROL_SHIFT   10
#define MCDE_KEYCTRL_SHIFT        7
#define MCDE_ROTEN_SHIFT          6
#define MCDE_DITHEN_SHIFT         5
#define MCDE_CEAEN_SHIFT          4
#define MCDE_AFLICKEN_SHIFT       3
#define MCDE_BLENDEN_SHIFT        2
#define MCDE_CLK_SHIFT            30
#define MCDE_BCD_SHIFT            29
#define MCDE_OUTBPP_SHIFT         25
#define MCDE_CDWIN_SHIFT          13
#define MCDE_CLOCKSEL_SHIFT       10
#define MCDE_KEYA_SHIFT           SHIFT_BYTE3
#define MCDE_KEYR_SHIFT           SHIFT_BYTE2
#define MCDE_KEYG_SHIFT           SHIFT_BYTE1
#define MCDE_RGB_SHIFT            SHIFT_HALFWORD1
#define MCDE_THRESHOLD_SHIFT      SHIFT_QUARTET6
#define MCDE_COEFFN3_SHIFT        SHIFT_BYTE2
#define MCDE_COEFFN2_SHIFT        SHIFT_BYTE1
#define MCDE_COEFFN1_SHIFT        (SHIFT_BYTE0)
#define MCDE_TV_LINES_SHIFT       SHIFT_HALFWORD1
#define MCDE_TVMODE_SHIFT         3
#define MCDE_IFIELD_SHIFT         2
#define MCDE_INTEREN_SHIFT        1
#define MCDE_BSL_SHIFT            16
#define MCDE_FSL2_SHIFT           16
#define MCDE_DVO2_SHIFT           16
#define MCDE_SWW_SHIFT            16
#define MCDE_ALW_SHIFT            16
#define MCDE_TVBCR_SHIFT          SHIFT_BYTE2
#define MCDE_TVBCB_SHIFT          SHIFT_BYTE1
#define MCDE_REVVAEN_SHIFT        31
#define MCDE_REVTGEN_SHIFT        30
#define MCDE_REVLOADSEL_SHIFT     28
#define MCDE_REVDEL1_SHIFT        SHIFT_QUARTET6
#define MCDE_REVDEL0_SHIFT        SHIFT_HALFWORD1
#define MCDE_PSVAEN_SHIFT         15
#define MCDE_PSTGEN_SHIFT         14
#define MCDE_PSLOADSEL_SHIFT      12
#define MCDE_PSDEL1_SHIFT         8
#define MCDE_IOE_SHIFT            23
#define MCDE_IPC_SHIFT            22
#define MCDE_IHS_SHIFT            21
#define MCDE_IVS_SHIFT            20
#define MCDE_IVP_SHIFT            19
#define MCDE_ICLSPL_SHIFT         18
#define MCDE_ICLREV_SHIFT         17
#define MCDE_ICLSP_SHIFT          16
#define MCDE_SPLVAEN_SHIFT        15
#define MCDE_SPLTGEN_SHIFT        14
#define MCDE_SPLLOADSEL_SHIFT     12
#define MCDE_SPLDEL1_SHIFT        8
#define MCDE_FOFFY_SHIFT          10
#define MCDE_FOFFX_SHIFT          5
#define MCDE_MASK_BITCTRL_SHIFT   4
#define MCDE_MODE_SHIFT           2
#define MCDE_COMP_SHIFT           1
#define MCDE_YB_SHIFT             24
#define MCDE_XB_SHIFT             SHIFT_HALFWORD1
#define MCDE_YG_SHIFT             5
#define MCDE_ARED_SHIFT           SHIFT_HALFWORD1
#define MCDE_GREEN_SHIFT          SHIFT_BYTE1
/*******************************************************************
   MCDE Channel C Register Fields
********************************************************************/
#define MCDE_SYNCCTRL_MASK    (MASK_BIT30 | MASK_BIT29)
#define MCDE_RESEN_MASK       (MASK_BIT18)
#define MCDE_CLKSEL_MASK      (MASK_BIT14 | MASK_BIT13)
#define MCDE_SYNCSEL_MASK     MASK_BIT6
#define MCDE_RES2_MASK        MASK_BIT28
#define MCDE_RES1_MASK        MASK_BIT27
#define MCDE_RD2_MASK         MASK_BIT26
#define MCDE_RD1_MASK         MASK_BIT25
#define MCDE_WR2_MASK         MASK_BIT24
#define MCDE_WR1_MASK         MASK_BIT23
#define MCDE_CD2_MASK         MASK_BIT22
#define MCDE_CD1_MASK         MASK_BIT21
#define MCDE_CS2_MASK         MASK_BIT20
#define MCDE_CS1_MASK         MASK_BIT19
#define MCDE_CS2EN_MASK       MASK_BIT17
#define MCDE_CS1EN_MASK       MASK_BIT16
#define MCDE_INBAND2_MASK     MASK_BIT12
#define MCDE_INBAND1_MASK     MASK_BIT11
#define MCDE_BUSSIZE2_MASK    MASK_BIT10
#define MCDE_BUSSIZE1_MASK    MASK_BIT9
#define MCDE_SYNCEN2_MASK     MASK_BIT8
#define MCDE_SYNCEN1_MASK     MASK_BIT7
#define MCDE_WMLVL2_MASK      MASK_BIT5
#define MCDE_WMLVL1_MASK      MASK_BIT4
#define MCDE_C2EN_MASK        MASK_BIT3
#define MCDE_C1EN_MASK        MASK_BIT2
#define MCDE_POWEREN_MASK     MASK_BIT1
#define MCDE_FLOEN_MASK       MASK_BIT0
#define MCDE_PDCTRL_MASK      (MASK_BIT10 |MASK_BIT11 | MASK_BIT12)
#define MCDE_DUPLEXER_MASK    (MASK_BIT7 |MASK_BIT8 | MASK_BIT9)
#define MCDE_BSDM_MASK        (MASK_BIT6 | MASK_BIT5 | MASK_BIT4)
#define MCDE_BSCM_MASK        (MASK_BIT2 | MASK_BIT1 | MASK_BIT0)
#define MCDE_VSDBL_MASK       (MASK_BIT29 | MASK_BIT30 | MASK_BIT31)
#define MCDE_VSSEL_MASK       MASK_BIT28
#define MCDE_VSPOL_MASK       MASK_BIT27
#define MCDE_VSPDIV_MASK      (MASK_BIT24 | MASK_BIT25 | MASK_BIT26)
#define MCDE_VSPMAX_MASK      (MASK_BYTE2 | MASK_QUARTET3)
#define MCDE_VSPMIN_MASK      (MASK_BYTE0 | MASK_QUARTET2)
#define MCDE_TRDELC_MASK      (MASK_BYTE2 | MASK_QUARTET6)
#define MCDE_SYNCDELC1_MASK   (MASK_BYTE1)
#define MCDE_SYNCDELC0_MASK   (MASK_BYTE0)
#define MCDE_VSTAC1_MASK       MASK_BIT1
#define MCDE_VSTAC0_MASK       MASK_BIT0
#define MCDE_BCN_MASK          MASK_BYTE0
#define MCDE_CSCDDEACT_MASK    MASK_BYTE1
#define MCDE_CSCDACT_MASK      MASK_BYTE0
#define MCDE_MOTINT_MASK       MASK_BIT16
#define MCDE_RWDEACT_MASK      MASK_BYTE1
#define MCDE_RWACT_MASK        MASK_BYTE0
#define MCDE_DODEACT_MASK      MASK_BYTE1
#define MCDE_DOACT_MASK        MASK_BYTE0
#define MCDE_READDATA_MASK     MASK_HALFWORD0
#define MCDE_DATACOMMANDMASK   0x01FFFFFF

#define MCDE_SYNCCTRL_SHIFT   29
#define MCDE_RESEN_SHIFT      18
#define MCDE_CLKSEL_SHIFT     13
#define MCDE_SYNCSEL_SHIFT    6
#define MCDE_RES2_SHIFT       28
#define MCDE_RES1_SHIFT       27
#define MCDE_RD2_SHIFT        26
#define MCDE_RD1_SHIFT        25
#define MCDE_WR2_SHIFT        24
#define MCDE_WR1_SHIFT        23
#define MCDE_CD2_SHIFT        22
#define MCDE_CD1_SHIFT        21
#define MCDE_CS2_SHIFT        20
#define MCDE_CS1_SHIFT        19
#define MCDE_CS2EN_SHIFT      17
#define MCDE_CS1EN_SHIFT      16
#define MCDE_INBAND2_SHIFT    12
#define MCDE_INBAND1_SHIFT    11
#define MCDE_BUSSIZE2_SHIFT   10
#define MCDE_BUSSIZE1_SHIFT    9
#define MCDE_SYNCEN2_SHIFT    8
#define MCDE_SYNCEN1_SHIFT    7
#define MCDE_WMLVL2_SHIFT     5
#define MCDE_WMLVL1_SHIFT     4
#define MCDE_C2EN_SHIFT       3
#define MCDE_C1EN_SHIFT       2
#define MCDE_POWEREN_SHIFT    1
#define MCDE_PDCTRL_SHIFT     12
#define MCDE_DUPLEXER_SHIFT   7
#define MCDE_BSDM_SHIFT       4
#define MCDE_VSDBL_SHIFT      29
#define MCDE_VSSEL_SHIFT      28
#define MCDE_VSPOL_SHIFT      27
#define MCDE_VSPDIV_SHIFT     24
#define MCDE_VSPMAX_SHIFT     12
#define MCDE_TRDELC_SHIFT     SHIFT_HALFWORD1
#define MCDE_SYNCDELC1_SHIFT  SHIFT_BYTE1
#define MCDE_VSTAC1_SHIFT     1
#define MCDE_CSCDDEACT_SHIFT  SHIFT_BYTE1
#define MCDE_MOTINT_SHIFT     SHIFT_HALFWORD1
#define MCDE_RWDEACT_SHIFT    8
#define MCDE_DODEACT_SHIFT    SHIFT_BYTE1

/*******************************************************************
   MCDE DSI Register Fields
********************************************************************/
#define MCDE_PLLOUT_DIVSEL1_MASK        (MASK_BIT22 | MASK_BIT23)
#define MCDE_PLLOUT_DIVSEL0_MASK        (MASK_BIT20 | MASK_BIT21)
#define MCDE_PLL4IN_SEL_MASK            (MASK_BIT16 | MASK_BIT17)
#define MCDE_TXESCDIV_SEL_MASK          MASK_BIT8
#define MCDE_TXESCDIV_MASK              0xFF
#define MCDE_CMDBYTE_LSB_MASK           0xFF
#define MCDE_CMDBYTE_MSB_MASK           0xFF00
#define MCDE_DSI_SW_MASK                0xFFF0000
#define MCDE_DSI_DMA_MASK               0xFFF
#define MCDE_DSI_PACK_MASK              (MASK_BIT20 | MASK_BIT21 | MASK_BIT22)
#define MCDE_DSI_DCSVID_MASK            MASK_BIT18
#define MCDE_DSI_BYTE_SWAP_MASK         MASK_BIT17
#define MCDE_DSI_BIT_SWAP_MASK          MASK_BIT16
#define MCDE_DSI_CMD8_MASK              MASK_BIT13
#define MCDE_DSI_VID_MODE_MASK          MASK_BIT12
#define MCDE_BLANKING_MASK              MASK_QUARTET0
#define MCDE_DSI_FRAME_MASK             (MASK_HALFWORD0 | MASK_BYTE2)
#define MCDE_DSI_PACKET_MASK            MASK_HALFWORD0

#define MCDE_PLLOUT_DIVSEL1_SHIFT       22
#define MCDE_PLLOUT_DIVSEL0_SHIFT       20
#define MCDE_PLL4IN_SEL_SHIFT           16
#define MCDE_TXESCDIV_SEL_SHIFT         8
#define MCDE_CMDBYTE_MSB_SHIFT          8
#define MCDE_DSI_SW_SHIFT               16
#define MCDE_DSI_PACK_SHIFT             20
#define MCDE_DSI_DCSVID_SHIFT           18
#define MCDE_DSI_BYTE_SWAP_SHIFT        17
#define MCDE_DSI_BIT_SWAP_SHIFT         16
#define MCDE_DSI_CMD8_SHIFT             13
#define MCDE_DSI_VID_MODE_SHIFT         12

/*******************************************************************
   Register Structure
********************************************************************/

#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT

struct mcde_ext_src_reg
{
    volatile u32 mcde_extsrc_a0;
    volatile u32 mcde_extsrc_a1;
    volatile u32 mcde_extsrc_a2;
    volatile u32 mcde_extsrc_conf;
    volatile u32 mcde_extsrc_cr;
    volatile u32 mcde_unused1[3];
};

struct mcde_ovl_reg
{
    volatile u32 mcde_ovl_cr;
    volatile u32 mcde_ovl_conf;
    volatile u32 mcde_ovl_conf2;
    volatile u32 mcde_ovl_ljinc;
    volatile u32 mcde_ovl_crop;
    volatile u32 mcde_ovl_comp;
    volatile u32 mcde_unused1[2];
};

struct mcde_chnl_conf_reg
{
  volatile u32 mcde_chnl_conf;
  volatile u32 mcde_chnl_stat;
  volatile u32 mcde_chnl_synchmod;
  volatile u32 mcde_chnl_synchsw;
  volatile u32 mcde_chnl_bckgndcol;
  volatile u32 mcde_chnl_prio;
  volatile u32 mcde_unused[2];
};

struct mcde_chAB_reg
{
    volatile u32 mcde_cr0;
    volatile u32 mcde_cr1;
    volatile u32 mcde_colkey;
    volatile u32 mcde_fcolkey;
    volatile u32 mcde_rgbconv1;
    volatile u32 mcde_rgbconv2;
    volatile u32 mcde_rgbconv3;
    volatile u32 mcde_rgbconv4;
    volatile u32 mcde_rgbconv5;
    volatile u32 mcde_rgbconv6;
    volatile u32 mcde_ffcoef0;
    volatile u32 mcde_ffcoef1;
    volatile u32 mcde_ffcoef2;
    volatile u32 mcde_unused1[1];
    volatile u32 mcde_tvcr;
    volatile u32 mcde_tvbl1;
    volatile u32 mcde_tvisl;
    volatile u32 mcde_tvdvo;
    volatile u32 mcde_unused2[1];
    volatile u32 mcde_tvtim1;
    volatile u32 mcde_tvlbalw;
    volatile u32 mcde_tvbl2;
    volatile u32 mcde_tvblu;
    volatile u32 mcde_lcdtim0;
    volatile u32 mcde_lcdtim1;
    volatile u32 mcde_ditctrl;
    volatile u32 mcde_ditoff;
    volatile u32 mcde_pal0;
    volatile u32 mcde_pal1;
    volatile u32 mcde_rotadd0;
    volatile u32 mcde_rotadd1;
    volatile u32 mcde_rot_conf;
    volatile u32 mcde_synchconf;
    volatile u32 mcde_unused3[1];
    volatile u32 mcde_gam0;
    volatile u32 mcde_gam1;
    volatile u32 mcde_gam2;
    volatile u32 mcde_oledconv1;
    volatile u32 mcde_oledconv2;
    volatile u32 mcde_oledconv3;
    volatile u32 mcde_oledconv4;
    volatile u32 mcde_oledconv5;
    volatile u32 mcde_oledconv6;
    volatile u32 mcde_unused4[85];
};

struct mcde_chC0C1_reg
{
    volatile u32 mcde_crc;
    volatile u32 mcde_pbccrc[2];
    volatile u32 mcde_pbcbmrc0[5];
    volatile u32 mcde_pbcbmrc1[5];
    volatile u32 mcde_pbcbcrc0[2];
    volatile u32 mcde_unused1[3];
    volatile u32 mcde_pbcbcrc1[2];
    volatile u32 mcde_unused2[3];
    volatile u32 mcde_vscrc[2];
    volatile u32 mcde_sctrc;
    volatile u32 mcde_scsrc;
    volatile u32 mcde_bcnr[2];
    volatile u32 mcde_cscdtr[2];
    volatile u32 mcde_rdwrtr[2];
    volatile u32 mcde_dotr[2];
    volatile u32 mcde_wcmdc[2];
    volatile u32 mcde_wdatadc[2];
    volatile u32 mcde_rdatadc[2];
    volatile u32 mcde_statc;
    volatile u32 mcde_ctrlc[2];
};

struct mcde_dsi_reg
{
    volatile u32 mcde_dsi_conf0;
    volatile u32 mcde_dsi_frame;
    volatile u32 mcde_dsi_pkt;
    volatile u32 mcde_dsi_sync;
    volatile u32 mcde_dsi_cmdw;
    volatile u32 mcde_dsi_delay0;
    volatile u32 mcde_dsi_delay1;
    volatile u32 mcde_unused1[1];
};

struct mcde_top_reg
{
    volatile u32 mcde_cr;
    volatile u32 mcde_conf0;
    volatile u32 mcde_ssp;
    volatile u32 mcde_reserved1[61];
    volatile u32 mcde_ais;
    volatile u32 mcde_imscpp;
    volatile u32 mcde_imscovl;
    volatile u32 mcde_imscchnl;
    volatile u32 mcde_imscerr;
    volatile u32 mcde_rispp;
    volatile u32 mcde_risovl;
    volatile u32 mcde_rischnl;
    volatile u32 mcde_riserr;
    volatile u32 mcde_mispp;
    volatile u32 mcde_misovl;
    volatile u32 mcde_mischnl;
    volatile u32 mcde_miserr;
    volatile u32 mcde_sispp;
    volatile u32 mcde_sisovl;
    volatile u32 mcde_sischnl;
    volatile u32 mcde_siserr;
    volatile u32 mcde_reserved2[46];
    volatile u32 mcde_pid;
};

#else	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

struct mcde_ext_src_reg
{
    volatile u32 mcde_extsrc_a0;
    volatile u32 mcde_extsrc_a1;
    volatile u32 mcde_extsrc_a2;
    volatile u32 mcde_extsrc_conf;
    volatile u32 mcde_extsrc_cr;
    volatile u32 mcde_unused1[3];
};

struct mcde_ovl_reg
{
    volatile u32 mcde_ovl_cr;
    volatile u32 mcde_ovl_conf;
    volatile u32 mcde_ovl_conf2;
    volatile u32 mcde_ovl_ljinc;
    volatile u32 mcde_ovl_crop;
    volatile u32 mcde_ovl_top_left_clip;
    volatile u32 mcde_ovl_bot_rht_clip;
    volatile u32 mcde_ovl_comp;
};

struct mcde_ch_synch_reg
{
    volatile u32 mcde_ch_conf;
    volatile u32 mcde_ch_stat;
    volatile u32 mcde_chsyn_mod;
    volatile u32 mcde_chsyn_sw;
    volatile u32 mcde_chsyn_bck;
    volatile u32 mcde_chsyn_prio;
    volatile u32 mcde_unused3[2];
};

struct mcde_ch_reg
{
    volatile u32 mcde_ch_cr0;
    volatile u32 mcde_ch_cr1;
    volatile u32 mcde_ch_colkey;
    volatile u32 mcde_ch_fcolkey;
    volatile u32 mcde_ch_rgbconv1;
    volatile u32 mcde_ch_rgbconv2;
    volatile u32 mcde_ch_rgbconv3;
    volatile u32 mcde_ch_rgbconv4;
    volatile u32 mcde_ch_rgbconv5;
    volatile u32 mcde_ch_rgbconv6;
    volatile u32 mcde_ch_ffcoef0;
    volatile u32 mcde_ch_ffcoef1;
    volatile u32 mcde_ch_ffcoef2;
    volatile u32 unused;
    volatile u32 mcde_ch_tvcr;
    volatile u32 mcde_ch_tvbl1;
    volatile u32 mcde_ch_tvisl;
    volatile u32 mcde_ch_tvdvo;
    volatile u32 mcde_ch_tvswh;
    volatile u32 mcde_ch_tvtim1;
    volatile u32 mcde_ch_tvbalw;
    volatile u32 mcde_ch_tvbl2;
    volatile u32 mcde_ch_tvblu;
    volatile u32 mcde_ch_lcdtim0;
    volatile u32 mcde_ch_lcdtim1;
    volatile u32 mcde_ch_ditctrl;
    volatile u32 mcde_ch_ditoff;
    volatile u32 mcde_ch_pal;
    volatile u32 mcde_ch_gam;
    volatile u32 mcde_rotadd0;
    volatile u32 mcde_rotadd1;
    volatile u32 mcde_chsyn_con;
    volatile u32 mcde_unused7[96];
};

struct mcde_dsi_reg
{
    volatile u32 mcde_dsi_conf0;
    volatile u32 mcde_dsi_frame;
    volatile u32 mcde_dsi_pkt;
    volatile u32 mcde_dsi_sync;
    volatile u32 mcde_dsi_cmd;
    volatile u32 mcde_reserved2[3];
};

struct mcde_chc_reg
{
    volatile u32 mcde_chc_crc;
    volatile u32 mcde_chc_pbcrc0;
    volatile u32 mcde_chc_pbcrc1;
    volatile u32 mcde_chc_pbcbmrc0[5];
    volatile u32 mcde_chc_pbcbmrc1[5];
    volatile u32 mcde_chc_pbcbcrc0[2];
    volatile u32 mcde_unused5[3];
    volatile u32 mcde_chc_pbcbcrc1[2];
    volatile u32 mcde_unused6[3];
    volatile u32 mcde_chc_vscrc[2];
    volatile u32 mcde_chc_sctrc;
    volatile u32 mcde_chc_scsr;
    volatile u32 mcde_chc_bcnr[2];
    volatile u32 mcde_chc_cscdtr[2];
    volatile u32 mcde_chc_rdwrtr[2];
    volatile u32 mcde_chc_dotr[2];
    volatile u32 mcde_chc_wcmd[2];
    volatile u32 mcde_chc_wd[2];
    volatile u32 mcde_chc_rdata[2];
};

struct mcde_register_base
{
    volatile u32 mcde_cr;
    volatile u32 mcde_cfg0;
    volatile u32 mcde_reserved1[62];
    volatile u32 mcde_ais;
    volatile u32 mcde_imsc;
    volatile u32 mcde_ris;
    volatile u32 mcde_mis;
    volatile u32 mcde_sis;
    volatile u32 mcde_ssi;
    volatile u32 mcde_reserved2[57];
    volatile u32 mcde_pid;
};

#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */

#endif
