/*---------------------------------------------------------------------------*/
/* © copyright STEricsson,2009. All rights reserved. For   */
/* information, STEricsson reserves the right to license    */
/* this software concurrently under separate license conditions.             */
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

#ifdef CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT
/* HW V1 */

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

/** Linux include files:charachter driver and memory functions  */
#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/mcde_common.h>
#include <mach/mcde_a0.h>

extern struct mcdefb_info *gpar[];

#define PLATFORM_8500 1


mcde_error mcdesetdsiclk(dsi_link link, mcde_ch_id chid, mcde_dsi_clk_config clk_config)
{
    mcde_error    error = MCDE_OK;

    *gpar[chid]->mcde_clkdsi =
        (
            (*gpar[chid]->mcde_clkdsi &~MCDE_PLLOUT_DIVSEL1_MASK) |
            (((u32) clk_config.pllout_divsel1 << MCDE_PLLOUT_DIVSEL1_SHIFT) & MCDE_PLLOUT_DIVSEL1_MASK)
        );

    *gpar[chid]->mcde_clkdsi =
        (
            (*gpar[chid]->mcde_clkdsi &~MCDE_PLLOUT_DIVSEL0_MASK) |
            (((u32) clk_config.pllout_divsel0 << MCDE_PLLOUT_DIVSEL0_SHIFT) & MCDE_PLLOUT_DIVSEL0_MASK)
        );

    *gpar[chid]->mcde_clkdsi =
        (
            (*gpar[chid]->mcde_clkdsi &~MCDE_PLL4IN_SEL_MASK) |
            (((u32) clk_config.pll4in_sel << MCDE_PLL4IN_SEL_SHIFT) & MCDE_PLL4IN_SEL_MASK)
        );

    *gpar[chid]->mcde_clkdsi =
        (
            (*gpar[chid]->mcde_clkdsi &~MCDE_TXESCDIV_SEL_MASK) |
            (((u32) clk_config.pll4in_sel << MCDE_TXESCDIV_SEL_SHIFT) & MCDE_TXESCDIV_SEL_MASK)
        );

    *gpar[chid]->mcde_clkdsi =
        (
            (*gpar[chid]->mcde_clkdsi &~MCDE_TXESCDIV_MASK) |
            ((u32) clk_config.pll4in_sel & MCDE_TXESCDIV_MASK)
        );

    return(error);
}


mcde_error mcdesetdsicommandword
(
    dsi_link link,
    mcde_ch_id chid,
    mcde_dsi_channel   dsichannel,
    u8              cmdbyte_lsb,
    u8              cmdbyte_msb
)
{
    mcde_error    error = MCDE_OK;
    struct mcde_dsi_reg  *dsi_reg;

    if (MCDE_DSI_CH_CMD2 < (u32) dsichannel)
    {
         return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        dsi_reg = (struct mcde_dsi_reg *) (gpar[chid]->mcde_dsi_channel_reg[dsichannel]);
    }

    /* Importante: Register definition changed! */
    dsi_reg->mcde_dsi_cmdw =
        (
            (dsi_reg->mcde_dsi_cmdw &~MCDE_CMDBYTE_LSB_MASK) |
            ((u32) cmdbyte_lsb & MCDE_CMDBYTE_LSB_MASK)
        );

    dsi_reg->mcde_dsi_cmdw =
        (
            (dsi_reg->mcde_dsi_cmdw &~MCDE_CMDBYTE_MSB_MASK) |
            (((u32) cmdbyte_msb << MCDE_CMDBYTE_MSB_SHIFT) & MCDE_CMDBYTE_MSB_MASK)
        );

     return(error);
}

mcde_error mcdesetdsisyncpulse(dsi_link link, mcde_ch_id chid, mcde_dsi_channel dsichannel, u16 sync_dma, u16 sync_sw)
{
    mcde_error    error = MCDE_OK;
    struct mcde_dsi_reg  *dsi_reg;


    if (MCDE_DSI_CH_CMD2 < (u32) dsichannel)
    {
         return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        dsi_reg = (struct mcde_dsi_reg *) (gpar[chid]->mcde_dsi_channel_reg[dsichannel]);
    }

    dsi_reg->mcde_dsi_sync = ((dsi_reg->mcde_dsi_sync &~MCDE_DSI_DMA_MASK) | ((u32) sync_dma & MCDE_DSI_DMA_MASK));

    dsi_reg->mcde_dsi_sync =
        (
            (dsi_reg->mcde_dsi_sync &~MCDE_DSI_SW_MASK) |
            (((u32) sync_dma << MCDE_DSI_SW_SHIFT) & MCDE_DSI_SW_MASK)
        );


    return(error);
}

mcde_error mcdesetdsiconf(dsi_link link, mcde_ch_id chid, mcde_dsi_channel dsichannel, mcde_dsi_conf dsi_conf)
{
    mcde_error    error = MCDE_OK;
    struct mcde_dsi_reg  *dsi_reg;


    if (MCDE_DSI_CH_CMD2 < (u32) dsichannel)
    {
         return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        dsi_reg = (struct mcde_dsi_reg *) (gpar[chid]->mcde_dsi_channel_reg[dsichannel]);
    }

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_DSI_PACK_MASK) |
            (((u32) dsi_conf.packing << MCDE_DSI_PACK_SHIFT) & MCDE_DSI_PACK_MASK)
        );

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_DSI_DCSVID_MASK) |
            (((u32) dsi_conf.synchro << MCDE_DSI_DCSVID_SHIFT) & MCDE_DSI_DCSVID_MASK)
        );

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_DSI_BYTE_SWAP_MASK) |
            (((u32) dsi_conf.byte_swap << MCDE_DSI_BYTE_SWAP_SHIFT) & MCDE_DSI_BYTE_SWAP_MASK)
        );

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_DSI_BIT_SWAP_MASK) |
            (((u32) dsi_conf.bit_swap << MCDE_DSI_BIT_SWAP_SHIFT) & MCDE_DSI_BIT_SWAP_MASK)
        );

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_DSI_CMD8_MASK) |
            (((u32) dsi_conf.cmd8 << MCDE_DSI_CMD8_SHIFT) & MCDE_DSI_CMD8_MASK)
        );

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_DSI_VID_MODE_MASK) |
            (((u32) dsi_conf.vid_mode << MCDE_DSI_VID_MODE_SHIFT) & MCDE_DSI_VID_MODE_MASK)
        );

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_BLANKING_MASK) |
            ((u32) dsi_conf.blanking & MCDE_BLANKING_MASK)
        );

    dsi_reg->mcde_dsi_frame =
        (
            (dsi_reg->mcde_dsi_frame &~MCDE_DSI_FRAME_MASK) |
            ((u32) dsi_conf.words_per_frame & MCDE_DSI_FRAME_MASK)
        );

    dsi_reg->mcde_dsi_pkt =
        (
            (dsi_reg->mcde_dsi_pkt &~MCDE_DSI_PACKET_MASK) |
            ((u32) dsi_conf.words_per_packet & MCDE_DSI_PACKET_MASK)
        );
    return(error);
}



/****************************************************************************/
/** NAME			:	mcdesetfifoctrl()			    */
/*--------------------------------------------------------------------------*/
/* DESCRIPTION	: 	This routine sets the formatter selection for output FIFOs*/
/*				                                                    		*/
/*                                                                          */
/* PARAMETERS	:                                                           */
/* 		IN  	:mcde_fifo_ctrl : FIFO selection control structure        */
/*     InOut    :None                                                       */
/* 		OUT 	:None                                                       */
/*                                                                          */
/* RETURN		:mcde_error	: MCDE error code						   	*/
/*               MCDE_OK                                                    */
/*               MCDE_INVALID_PARAMETER :if input argument is invalid       */
/*--------------------------------------------------------------------------*/
/* Type              :  PUBLIC                                              */
/* REENTRANCY 	     :	Non Re-entrant                                      */
/* REENTRANCY ISSUES :														*/

/****************************************************************************/
mcde_error mcdesetfifoctrl(dsi_link link, mcde_ch_id chid, struct mcde_fifo_ctrl fifo_ctrl)
{
    mcde_error    error = MCDE_OK;

    /*FIFO A Output Selection*/
    switch (chid)
    {
    case MCDE_CH_A:
	switch (fifo_ctrl.out_fifoa)
	{
        case MCDE_DPI_A:

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIA_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_DPIA_EN_SHIFT) & MCDE_CTRL_DPIA_EN_MASK)
                );
            break;

        case MCDE_DSI_VID0:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIA_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DPIA_EN_SHIFT) & MCDE_CTRL_DPIA_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_FABMUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_FABMUX_SHIFT) & MCDE_CTRL_FABMUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID0_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSIVID0_EN_SHIFT) & MCDE_DSIVID0_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_VID0] = MCDE_CH_A;

            break;

        case MCDE_DSI_VID1:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIA_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DPIA_EN_SHIFT) & MCDE_CTRL_DPIA_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_FABMUX_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_FABMUX_SHIFT) & MCDE_CTRL_FABMUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSIVID0_EN_SHIFT) & MCDE_DSIVID0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID1_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSIVID1_EN_SHIFT) & MCDE_DSIVID1_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_VID1] = MCDE_CH_A;
            break;

        case MCDE_DSI_CMD2:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIA_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DPIA_EN_SHIFT) & MCDE_CTRL_DPIA_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_FABMUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_FABMUX_SHIFT) & MCDE_CTRL_FABMUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSIVID0_EN_SHIFT) & MCDE_DSIVID0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSIVID1_EN_SHIFT) & MCDE_DSIVID1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD2_EN_MASK) |
                    ((u32) MCDE_SET_BIT & MCDE_DSICMD2_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD2] = MCDE_CH_A;
            break;

        default:
            error = MCDE_INVALID_PARAMETER;
	}
	break;
    case MCDE_CH_B:
	/*FIFO B Output Selection*/
	switch (fifo_ctrl.out_fifob)
	{
	case MCDE_DPI_B:
	   gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIB_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_DPIB_EN_SHIFT) & MCDE_CTRL_DPIB_EN_MASK)
                );
            break;

        case MCDE_DSI_VID0:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIB_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DPIB_EN_SHIFT) & MCDE_CTRL_DPIB_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_FABMUX_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_FABMUX_SHIFT) & MCDE_CTRL_FABMUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID0_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSIVID0_EN_SHIFT) & MCDE_DSIVID0_EN_MASK)
                );
            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_VID0] = MCDE_CH_B;
            break;

        case MCDE_DSI_VID1:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIB_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DPIB_EN_SHIFT) & MCDE_CTRL_DPIB_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_FABMUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_FABMUX_SHIFT) & MCDE_CTRL_FABMUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSIVID0_EN_SHIFT) & MCDE_DSIVID0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID1_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSIVID1_EN_SHIFT) & MCDE_DSIVID1_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_VID1] = MCDE_CH_B;
            break;

        case MCDE_DSI_CMD2:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIB_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DPIB_EN_SHIFT) & MCDE_CTRL_DPIB_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_FABMUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_FABMUX_SHIFT) & MCDE_CTRL_FABMUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSIVID0_EN_SHIFT) & MCDE_DSIVID0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSIVID1_EN_SHIFT) & MCDE_DSIVID1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD2_EN_MASK) |
                    ((u32) MCDE_SET_BIT & MCDE_DSICMD2_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD2] = MCDE_CH_B;

            break;

        default:
            error = MCDE_INVALID_PARAMETER;
	}
	break;
    case MCDE_CH_C0:
	/*FIFO 0 Output Selection*/
	switch (fifo_ctrl.out_fifo0)
	{
        case MCDE_DBI_C0:
           gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC0_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_DBIC0_EN_SHIFT) & MCDE_CTRL_DBIC0_EN_MASK)
                );
            break;

        case MCDE_DSI_CMD0:
           gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DBIC0_EN_SHIFT) & MCDE_CTRL_DBIC0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_F01MUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_F01MUX_SHIFT) & MCDE_CTRL_F01MUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD0_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSICMD0_EN_SHIFT) & MCDE_DSICMD0_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD0] = MCDE_CH_C0;

            break;

        case MCDE_DSI_CMD1:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DBIC0_EN_SHIFT) & MCDE_CTRL_DBIC0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_F01MUX_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_F01MUX_SHIFT) & MCDE_CTRL_F01MUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSICMD0_EN_SHIFT) & MCDE_DSICMD0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD1_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSICMD1_EN_SHIFT) & MCDE_DSICMD1_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD1] = MCDE_CH_C0;
            break;

        case MCDE_DSI_VID2:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DBIC0_EN_SHIFT) & MCDE_CTRL_DBIC0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_F01MUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_F01MUX_SHIFT) & MCDE_CTRL_F01MUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSICMD0_EN_SHIFT) & MCDE_DSICMD0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSICMD1_EN_SHIFT) & MCDE_DSICMD1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID2_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSIVID2_EN_SHIFT) & MCDE_DSIVID2_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_VID2] = MCDE_CH_C0;

            break;

        default:
            error = MCDE_INVALID_PARAMETER;
	}
	break;
    case MCDE_CH_C1:
	/*FIFO 1 Output Selection*/
	switch (fifo_ctrl.out_fifo1)
	{
        case MCDE_DBI_C1:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC1_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_DBIC1_EN_SHIFT) & MCDE_CTRL_DBIC1_EN_MASK)
                );
            break;

        case MCDE_DSI_CMD0:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DBIC1_EN_SHIFT) & MCDE_CTRL_DBIC1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_F01MUX_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_F01MUX_SHIFT) & MCDE_CTRL_F01MUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD0_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSICMD0_EN_SHIFT) & MCDE_DSICMD0_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD0] = MCDE_CH_C1;

            break;

        case MCDE_DSI_CMD1:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DBIC1_EN_SHIFT) & MCDE_CTRL_DBIC1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_F01MUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_F01MUX_SHIFT) & MCDE_CTRL_F01MUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSICMD0_EN_SHIFT) & MCDE_DSICMD0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD1_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSICMD1_EN_SHIFT) & MCDE_DSICMD1_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD1] = MCDE_CH_C1;

            break;

        case MCDE_DSI_CMD2:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DBIC1_EN_SHIFT) & MCDE_CTRL_DBIC1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_F01MUX_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_F01MUX_SHIFT) & MCDE_CTRL_F01MUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSICMD0_EN_SHIFT) & MCDE_DSICMD0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSICMD1_EN_SHIFT) & MCDE_DSICMD1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID2_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSIVID2_EN_SHIFT) & MCDE_DSIVID2_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD2] = MCDE_CH_C1;

            break;

        default:
            error = MCDE_INVALID_PARAMETER;
	}
	break;
    default:
	error = MCDE_INVALID_PARAMETER;
    }
    return(error);
}
#ifdef PLATFORM_8500
mcde_error mcdesetoutputconf(dsi_link link, mcde_ch_id chid, mcde_output_conf output_conf)
{
    mcde_error    error = MCDE_OK;

 //  if (chid == CHANNEL_C0)
   //	gpar[chid]->regbase->mcde_conf0 = 0x5e145001; /** has to be removed, Added for testing */

    switch (output_conf)
    {
        case MCDE_CONF_TVA_DPIC0_LCDB:
            gpar[chid]->regbase->mcde_conf0 =
                (
                    (gpar[chid]->regbase->mcde_conf0 &~MCDE_SYNCMUX_MASK) |
                    ((u32) MCDE_TVA_DPIC0_LCDB_MASK & MCDE_SYNCMUX_MASK)
                );
            break;

        case MCDE_CONF_TVB_DPIC1_LCDA:
            gpar[chid]->regbase->mcde_conf0 =
                (
                    (gpar[chid]->regbase->mcde_conf0 &~MCDE_SYNCMUX_MASK) |
                    ((u32) MCDE_TVB_DPIC1_LCDA_MASK & MCDE_SYNCMUX_MASK)
                );
            break;

        case MCDE_CONF_DPIC1_LCDA:
            gpar[chid]->regbase->mcde_conf0 =
                (
                    (gpar[chid]->regbase->mcde_conf0 &~MCDE_SYNCMUX_MASK) |
                    ((u32) MCDE_DPIC1_LCDA_MASK & MCDE_SYNCMUX_MASK)
                );
            break;

        case MCDE_CONF_DPIC0_LCDB:
            gpar[chid]->regbase->mcde_conf0 =
                (
                    (gpar[chid]->regbase->mcde_conf0 &~MCDE_SYNCMUX_MASK) |
                    ((u32) MCDE_DPIC0_LCDB_MASK & MCDE_SYNCMUX_MASK)
                );
            break;

        case MCDE_CONF_LCDA_LCDB:
            gpar[chid]->regbase->mcde_conf0 =
                (
                    (gpar[chid]->regbase->mcde_conf0 &~MCDE_SYNCMUX_MASK) |
                    ((u32) MCDE_LCDA_LCDB_MASK & MCDE_SYNCMUX_MASK)
                );
            break;
        case MCDE_CONF_DSI:
            gpar[chid]->regbase->mcde_conf0 =
                (
                    (gpar[chid]->regbase->mcde_conf0 &~MCDE_SYNCMUX_MASK) |
                    ((u32) MCDE_DSI_MASK & MCDE_SYNCMUX_MASK)
                );
            break;
        default:
            error = MCDE_INVALID_PARAMETER;
    }

    return(error);
}
#endif
/****************************************************************************/
/* NAME			:	mcdesetbufferaddr()								*/
/*--------------------------------------------------------------------------*/
/* DESCRIPTION	: 	This API is used to set the base address of the buffer. */
/*				                                                    		*/
/*                                                                          */
/* PARAMETERS	:                                                           */
/* 		IN  	:mcde_ext_src src_id: External source id number to be     */
/*                                      configured                          */
/*               mcde_buffer_id buffer_id : Buffer id whose address is    */
/*                                              to be configured            */
/*               uint32 address   : Address of the buffer                 */
/*     InOut    :None                                                       */
/* 		OUT 	:None                                                	    */
/*                                                                          */
/* RETURN		:mcde_error	: MCDE error code						   	*/
/*               MCDE_OK                                                    */
/*               MCDE_INVALID_PARAMETER :if input argument is invalid       */
/*--------------------------------------------------------------------------*/
/* Type              :  PUBLIC                                              */
/* REENTRANCY 	     :	Non Re-entrant                                      */
/* REENTRANCY ISSUES :														*/

/****************************************************************************/
mcde_error mcdesetbufferaddr
(
    mcde_ch_id chid,
    mcde_ext_src   src_id,
    mcde_buffer_id buffer_id,
    u32         address
)
{
    mcde_error        error = MCDE_OK;
    struct mcde_ext_src_reg  *ext_src;

    ext_src = (struct mcde_ext_src_reg *) (gpar[chid]->extsrc_regbase[src_id]);

    switch (buffer_id)
    {
        case MCDE_BUFFER_ID_0:
            ext_src->mcde_extsrc_a0 =
                (
                    (ext_src->mcde_extsrc_a0 &~MCDE_EXT_BUFFER_MASK) |
                    ((address << MCDE_EXT_BUFFER_SHIFT) & MCDE_EXT_BUFFER_MASK)
                );

            break;

        case MCDE_BUFFER_ID_1:
            ext_src->mcde_extsrc_a1 =
                (
                    (ext_src->mcde_extsrc_a1 &~MCDE_EXT_BUFFER_MASK) |
                    ((address << MCDE_EXT_BUFFER_SHIFT) & MCDE_EXT_BUFFER_MASK)
                );

            break;

        case MCDE_BUFFER_ID_2:
            ext_src->mcde_extsrc_a2 =
                (
                    (ext_src->mcde_extsrc_a2 &~MCDE_EXT_BUFFER_MASK) |
                    ((address << MCDE_EXT_BUFFER_SHIFT) & MCDE_EXT_BUFFER_MASK)
                );

            break;

        default:
            error = MCDE_INVALID_PARAMETER;
    }

    ext_src->mcde_extsrc_conf =
    (
        (ext_src->mcde_extsrc_conf &~MCDE_EXT_BUFFER_ID_MASK) |
        ((u32) buffer_id & MCDE_EXT_BUFFER_ID_MASK)
     );

    return(error);
}

mcde_error mcdesetextsrcconf(mcde_ch_id chid, mcde_ext_src src_id, struct mcde_ext_conf config)
{
    mcde_error        error = MCDE_OK;
    struct mcde_ext_src_reg  *ext_src;

    ext_src = (struct mcde_ext_src_reg *) (gpar[chid]->extsrc_regbase[src_id]);


    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BEPO_MASK) |
            (((u32) config.ovr_pxlorder << MCDE_EXT_BEPO_SHIFT) & MCDE_EXT_BEPO_MASK)
        );

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BEBO_MASK) |
            (((u32) config.endianity << MCDE_EXT_BEBO_SHIFT) & MCDE_EXT_BEBO_MASK)
        );

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BGR_MASK) |
            (((u32) config.rgb_format << MCDE_EXT_BGR_SHIFT) & MCDE_EXT_BGR_MASK)
        );

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BPP_MASK) |
            (((u32) config.bpp << MCDE_EXT_BPP_SHIFT) & MCDE_EXT_BPP_MASK)
        );

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_PRI_OVR_MASK) |
            (((u32) config.provr_id << MCDE_EXT_PRI_OVR_SHIFT) & MCDE_EXT_PRI_OVR_MASK)
        );

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BUFFER_NUM_MASK) |
            (((u32) config.buf_num << MCDE_EXT_BUFFER_NUM_SHIFT) & MCDE_EXT_BUFFER_NUM_MASK)
        );

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BUFFER_ID_MASK) |
            ((u32) config.buf_id & MCDE_EXT_BUFFER_ID_MASK)
        );

    return(error);
}

mcde_error mcdesetextsrcctrl(mcde_ch_id chid, mcde_ext_src src_id, struct mcde_ext_src_ctrl control)
{
    mcde_error        error = MCDE_OK;
    struct mcde_ext_src_reg  *ext_src;

    ext_src = (struct mcde_ext_src_reg *) (gpar[chid]->extsrc_regbase[src_id]);

    ext_src->mcde_extsrc_cr =
        (
            (ext_src->mcde_extsrc_cr &~MCDE_EXT_FORCEFSDIV_MASK) |
            (((u32) control.fs_div << MCDE_EXT_FORCEFSDIV_SHIFT) & MCDE_EXT_FORCEFSDIV_MASK)
        );

    ext_src->mcde_extsrc_cr =
        (
            (ext_src->mcde_extsrc_cr &~MCDE_EXT_FSDISABLE_MASK) |
            (((u32) control.fs_ctrl << MCDE_EXT_FSDISABLE_SHIFT) & MCDE_EXT_FSDISABLE_MASK)
        );

    ext_src->mcde_extsrc_cr =
        (
            (ext_src->mcde_extsrc_cr &~MCDE_EXT_OVR_CTRL_MASK) |
            (((u32) control.ovr_ctrl << MCDE_EXT_OVR_CTRL_SHIFT) & MCDE_EXT_OVR_CTRL_MASK)
        );

    ext_src->mcde_extsrc_cr =
        (
            (ext_src->mcde_extsrc_cr &~MCDE_EXT_BUF_MODE_MASK) |
            ((u32) control.sel_mode & MCDE_EXT_BUF_MODE_MASK)
        );

      return(error);
}
mcde_error mcdesetbufid(mcde_ch_id chid, mcde_ext_src src_id, mcde_buffer_id buffer_id, mcde_num_buffer_used buffer_num)
{
    mcde_error        error = MCDE_OK;
    struct mcde_ext_src_reg  *ext_src;

    ext_src = (struct mcde_ext_src_reg *) (gpar[chid]->extsrc_regbase[src_id]);

    if ((int) buffer_id > 2)
    {
	return(MCDE_INVALID_PARAMETER);
    }

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BUFFER_ID_MASK) |
            ((u32) buffer_id & MCDE_EXT_BUFFER_ID_MASK)
        );

	ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BUFFER_NUM_MASK) |
            (((u32) buffer_num << MCDE_EXT_BUFFER_NUM_SHIFT) & MCDE_EXT_BUFFER_NUM_MASK)
        );


    return(error);
}
mcde_error mcdesetcolorconvctrl(mcde_ch_id chid, mcde_overlay_id overlay, mcde_col_conv_ctrl  col_ctrl)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);
    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_COLCTRL_MASK) |
            (((u32) col_ctrl << MCDE_OVR_COLCTRL_SHIFT) & MCDE_OVR_COLCTRL_MASK)
        );
    return error;
}
mcde_error mcdesetovrctrl(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_control ovr_cr)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_OVLEN_MASK) |
            ((u32) ovr_cr.ovr_state & MCDE_OVR_OVLEN_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_COLCTRL_MASK) |
            (((u32) ovr_cr.col_ctrl << MCDE_OVR_COLCTRL_SHIFT) & MCDE_OVR_COLCTRL_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_PALCTRL_MASK) |
            (((u32) ovr_cr.pal_control << MCDE_OVR_PALCTRL_SHIFT) & MCDE_OVR_PALCTRL_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_CKEYEN_MASK) |
            (((u32) ovr_cr.color_key << MCDE_OVR_CKEYEN_SHIFT) & MCDE_OVR_CKEYEN_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_STBPRIO_MASK) |
            (((u32) ovr_cr.priority << MCDE_OVR_STBPRIO_SHIFT) & MCDE_OVR_STBPRIO_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_BURSTSZ_MASK) |
            (((u32) ovr_cr.burst_req << MCDE_OVR_BURSTSZ_SHIFT) & MCDE_OVR_BURSTSZ_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_MAXREQ_MASK) |
            (((u32) ovr_cr.outstnd_req << MCDE_OVR_MAXREQ_SHIFT) & MCDE_OVR_MAXREQ_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_ROTBURSTSIZE_MASK) |
            (((u32) ovr_cr.rot_burst_req << MCDE_OVR_ROTBURSTSIZE_SHIFT) & MCDE_OVR_ROTBURSTSIZE_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_ALPHAPMEN_MASK) |
            (((u32) ovr_cr.alpha << MCDE_OVR_ALPHAPMEN_SHIFT) & MCDE_OVR_ALPHAPMEN_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_CLIPEN_MASK) |
            (((u32) ovr_cr.clip << MCDE_OVR_CLIPEN_SHIFT) & MCDE_OVR_CLIPEN_MASK)
        );


    return(error);
}

mcde_error mcdesetovrlayconf(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_config ovr_conf)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_conf =
        (
            (ovr_config->mcde_ovl_conf &~MCDE_OVR_LPF_MASK) |
            (((u32) ovr_conf.line_per_frame << MCDE_OVR_LPF_SHIFT) & MCDE_OVR_LPF_MASK)
        );

    ovr_config->mcde_ovl_conf =
        (
            (ovr_config->mcde_ovl_conf &~MCDE_EXT_SRCID_MASK) |
            (((u32) ovr_conf.src_id << MCDE_EXT_SRCID_SHIFT) & MCDE_EXT_SRCID_MASK)
        );

    ovr_config->mcde_ovl_conf =
        (
            (ovr_config->mcde_ovl_conf &~MCDE_OVR_PPL_MASK) |
            ((u32) ovr_conf.ovr_ppl & MCDE_OVR_PPL_MASK)
        );

    return(error);
}

mcde_error mcdesetovrconf2(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_conf2 ovr_conf2)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);


    ovr_config->mcde_ovl_conf2 =
        (
            (ovr_config->mcde_ovl_conf2 &~MCDE_WATERMARK_MASK) |
            (((u32) ovr_conf2.watermark_level << MCDE_WATERMARK_SHIFT) & MCDE_WATERMARK_MASK)
        );

    ovr_config->mcde_ovl_conf2 =
        (
            (ovr_config->mcde_ovl_conf2 &~MCDE_OVR_OPQ_MASK) |
            (((u32) ovr_conf2.ovr_opaq << MCDE_OVR_OPQ_SHIFT) & MCDE_OVR_OPQ_MASK)
        );

    ovr_config->mcde_ovl_conf2 =
        (
            (ovr_config->mcde_ovl_conf2 &~MCDE_ALPHAVALUE_MASK) |
            (((u32) ovr_conf2.alpha_value << MCDE_ALPHAVALUE_SHIFT) & MCDE_ALPHAVALUE_MASK)
        );

    ovr_config->mcde_ovl_conf2 =
        (
            (ovr_config->mcde_ovl_conf2 &~MCDE_PIXOFF_MASK) |
            (((u32) ovr_conf2.pixoff << MCDE_PIXOFF_SHIFT) & MCDE_PIXOFF_MASK)
        );

    ovr_config->mcde_ovl_conf2 =
        (
            (ovr_config->mcde_ovl_conf2 &~MCDE_OVR_BLEND_MASK) |
            ((u32) ovr_conf2.ovr_blend & MCDE_OVR_BLEND_MASK)
        );

    return(error);
}
mcde_error mcdesetovrljinc(mcde_ch_id chid, mcde_overlay_id overlay, u32 ovr_ljinc)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_ljinc = ((ovr_ljinc << MCDE_LINEINCREMENT_SHIFT) & MCDE_LINEINCREMENT_MASK);

    return(error);
}
#ifdef PLATFORM_8500
mcde_error mcdesettopleftmargincrop(mcde_ch_id chid, mcde_overlay_id overlay, u32 ovr_topmargin, u16 ovr_leftmargin)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);


    ovr_config->mcde_ovl_crop =
        (
            (ovr_config->mcde_ovl_crop &~MCDE_YCLIP_MASK) |
            ((ovr_topmargin << MCDE_YCLIP_SHIFT) & MCDE_YCLIP_MASK)
        );


	ovr_config->mcde_ovl_crop =
			(
				(ovr_config->mcde_ovl_crop &~MCDE_XCLIP_MASK) |
				(((u32) ovr_leftmargin << MCDE_XCLIP_SHIFT) & MCDE_XCLIP_MASK)
			);


    return(error);
}
#endif
mcde_error mcdesetovrcomp(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_comp ovr_comp)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_ZLEVEL_MASK) |
            (((u32) ovr_comp.ovr_zlevel << MCDE_OVR_ZLEVEL_SHIFT) & MCDE_OVR_ZLEVEL_MASK)
        );

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_YPOS_MASK) |
            (((u32) ovr_comp.ovr_ypos << MCDE_OVR_YPOS_SHIFT) & MCDE_OVR_YPOS_MASK)
        );

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_CHID_MASK) |
            (((u32) ovr_comp.ch_id << MCDE_OVR_CHID_SHIFT) & MCDE_OVR_CHID_MASK)
        );

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_XPOS_MASK) |
            ((u32) ovr_comp.ovr_xpos & MCDE_OVR_XPOS_MASK)
        );

    return(error);
}

#ifdef PLATFORM_8500
mcde_error mcdesetovrclip(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_clip ovr_clip)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    /* TODO: Not implemented
    ovr_config->mcde_ovl_bot_rht_clip=
        (
            (ovr_config->mcde_ovl_bot_rht_clip&~MCDE_YBRCOOR_MASK) |
            (((u32) ovr_clip.ybrcoor << MCDE_YBRCOOR_SHIFT) & MCDE_YBRCOOR_MASK)
        );

    ovr_config->mcde_ovl_top_left_clip=
        (
            (ovr_config->mcde_ovl_top_left_clip &~MCDE_YBRCOOR_MASK) |
            (((u32) ovr_clip.ytlcoor << MCDE_YBRCOOR_SHIFT) & MCDE_YBRCOOR_MASK)
        );

    ovr_config->mcde_ovl_bot_rht_clip =
        (
            (ovr_config->mcde_ovl_bot_rht_clip &~MCDE_XBRCOOR_MASK) |
            ((u32) ovr_clip.xbrcoor & MCDE_XBRCOOR_MASK)
        );

    ovr_config->mcde_ovl_top_left_clip =
        (
            (ovr_config->mcde_ovl_top_left_clip &~MCDE_XBRCOOR_MASK) |
            ((u32) ovr_clip.xtlcoor & MCDE_XBRCOOR_MASK)
        );
    */
    return(error);
}
#endif

mcde_error mcdesetovrstate(mcde_ch_id chid, mcde_overlay_id overlay, mcde_overlay_ctrl state)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_cr = ((ovr_config->mcde_ovl_cr &~MCDE_OVR_OVLEN_MASK) | (state & MCDE_OVR_OVLEN_MASK));

    return(error);
}

mcde_error mcdesetovrpplandlpf(mcde_ch_id chid, mcde_overlay_id overlay, u16 ppl, u16 lpf)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_conf =
        (
            (ovr_config->mcde_ovl_conf &~MCDE_OVR_PPL_MASK) |
            ((u32) ppl & MCDE_OVR_PPL_MASK)
        );
	ovr_config->mcde_ovl_conf =
		 (
			 (ovr_config->mcde_ovl_conf &~MCDE_OVR_LPF_MASK) |
			 (((u32) lpf << MCDE_OVR_LPF_SHIFT) & MCDE_OVR_LPF_MASK)
		 );


    return(error);
}

mcde_error mcdeovraassociatechnl(mcde_ch_id chid, mcde_overlay_id overlay, mcde_ch_id ch_id)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_CHID_MASK) |
            (((u32) ch_id << MCDE_OVR_CHID_SHIFT) & MCDE_OVR_CHID_MASK)
        );

    return(error);
}


mcde_error mcdesetovrXYZpos(mcde_ch_id chid, mcde_overlay_id overlay, mcde_ovr_xy xy_pos, u8 z_pos)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_YPOS_MASK) |
            (((u32) xy_pos.ovr_ypos << MCDE_OVR_YPOS_SHIFT) & MCDE_OVR_YPOS_MASK)
        );

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_XPOS_MASK) |
            ((u32) xy_pos.ovr_xpos & MCDE_OVR_XPOS_MASK)
        );

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_ZLEVEL_MASK) |
            (((u32) z_pos << MCDE_OVR_ZLEVEL_SHIFT) & MCDE_OVR_ZLEVEL_MASK)
        );

    return(error);
}

mcde_error mcdeovrassociateextsrc(mcde_ch_id chid, mcde_overlay_id overlay, mcde_ext_src ext_src)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_conf =
        (
            (ovr_config->mcde_ovl_conf &~MCDE_EXT_SRCID_MASK) |
            (((u32) ext_src << MCDE_EXT_SRCID_SHIFT) & MCDE_EXT_SRCID_MASK)
        );

    return(error);
}

mcde_error mcdesetchnlXconf(mcde_ch_id chid, u16 channelnum, struct mcde_chconfig config)
{
    mcde_error        mcde_error = MCDE_OK;
    struct mcde_chnl_conf_reg *ch_syncreg;

    ch_syncreg = (struct mcde_chnl_conf_reg *) (gpar[chid]->ch_regbase1[channelnum]);

    ch_syncreg->mcde_chnl_conf = ((config.lpf & MCDE_CHNLCONF_LPF_MASK) << MCDE_CHNLCONF_LPF_SHIFT) |
                                 ((config.ppl & MCDE_CHNLCONF_PPL_MASK) << MCDE_CHNLCONF_PPL_SHIFT);

    return(mcde_error);
}

mcde_error mcdesetchnlsyncsrc(mcde_ch_id chid, u16 channelnum, struct mcde_chsyncmod sync_mod)
{
    mcde_error        mcde_error = MCDE_OK;
    struct mcde_chnl_conf_reg *ch_syncreg;

    ch_syncreg = (struct mcde_chnl_conf_reg *) (gpar[chid]->ch_regbase1[channelnum]);

    ch_syncreg->mcde_chnl_synchmod = ((sync_mod.out_synch_interface & MCDE_CHNLSYNCHMOD_OUT_SYNCH_SRC_MASK) << MCDE_CHNLSYNCHMOD_OUT_SYNCH_SRC_SHIFT) |
                                     ((sync_mod.ch_synch_src & MCDE_CHNLSYNCHMOD_SRC_SYNCH_MASK) << MCDE_CHNLSYNCHMOD_SRC_SYNCH_SHIFT);

    return(mcde_error);
}

mcde_error mcdesetchnlsyncevent(mcde_ch_id chid, struct mcde_ch_conf ch_config)
{
    mcde_error        mcde_error = MCDE_OK;
#ifdef PLATFORM_8500
    struct mcde_chAB_reg   *ch_x_reg;
#else
    struct mcde_chnl_conf_reg *ch_x_reg;
#endif

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
#ifdef PLATFORM_8500
      ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
#else
      ch_x_reg = (struct mcde_chnl_conf_reg *) (gpar[chid]->ch_regbase1[gpar[chid]->mcde_cur_ovl_bmp]);
#endif

    }

    ch_x_reg->mcde_synchconf = ((ch_x_reg->mcde_synchconf & ~MCDE_SWINTVCNT_MASK) | (((u32)ch_config.swint_vcnt << MCDE_SWINTVCNT_SHIFT) & MCDE_SWINTVCNT_MASK));
    /** set to active video if you want to receive VSYNC interrupts*/
    ch_x_reg->mcde_synchconf = ((ch_x_reg->mcde_synchconf & ~MCDE_SWINTVEVENT_MASK) | (((u32)ch_config.swint_vevent << MCDE_SWINTVEVENT_SHIFT) & MCDE_SWINTVEVENT_MASK));
    ch_x_reg->mcde_synchconf = ((ch_x_reg->mcde_synchconf & ~MCDE_HWREQVCNT_MASK) | (((u32)ch_config.hwreq_vcnt << MCDE_HWREQVCNT_SHIFT) & MCDE_HWREQVCNT_MASK));
    ch_x_reg->mcde_synchconf = ((ch_x_reg->mcde_synchconf & ~MCDE_HWREQVEVENT_MASK) | ((u32)ch_config.hwreq_vevent & MCDE_HWREQVEVENT_MASK));

    return(mcde_error);
}

mcde_error mcdesetswsync(mcde_ch_id chid, u16 channelnum, mcde_sw_trigger sw_trig)
{
    mcde_error        error = MCDE_OK;
    struct mcde_chnl_conf_reg *ch_syncreg;

    ch_syncreg = (struct mcde_chnl_conf_reg *) (gpar[chid]->ch_regbase1[channelnum]);

    ch_syncreg->mcde_chnl_synchsw =
            (ch_syncreg->mcde_chnl_synchsw &~MCDE_CHNLSYNCHSW_SW_TRIG) |
            (sw_trig != 0 ? MCDE_CHNLSYNCHSW_SW_TRIG : 0);

    return(error);
}

mcde_error mcdesetchnlbckgndcol(mcde_ch_id chid, u16 channelnum, struct mcde_ch_bckgrnd_col color)
{
    mcde_error        error = MCDE_OK;
    struct mcde_chnl_conf_reg *ch_syncreg;

    ch_syncreg = (struct mcde_chnl_conf_reg *) (gpar[chid]->ch_regbase1[channelnum]);

    ch_syncreg->mcde_chnl_bckgndcol =
        (
            (ch_syncreg->mcde_chnl_bckgndcol &~MCDE_REDCOLOR_MASK) |
            ((color.red << MCDE_REDCOLOR_SHIFT) & MCDE_REDCOLOR_MASK)
        );

    ch_syncreg->mcde_chnl_bckgndcol =
        (
            (ch_syncreg->mcde_chnl_bckgndcol &~MCDE_GREENCOLOR_MASK) |
            ((color.green << MCDE_GREENCOLOR_SHIFT) & MCDE_GREENCOLOR_MASK)
        );

    ch_syncreg->mcde_chnl_bckgndcol =
        (
            (ch_syncreg->mcde_chnl_bckgndcol &~MCDE_BLUECOLOR_MASK) |
            (color.blue & MCDE_BLUECOLOR_MASK)
        );


    return(error);
}

mcde_error mcdesetchnlsyncprio(mcde_ch_id chid, u16 channelnum, u32 priority)
{
    mcde_error        error = MCDE_OK;
    struct mcde_chnl_conf_reg *ch_syncreg;

    ch_syncreg = (struct mcde_chnl_conf_reg *) (gpar[chid]->ch_regbase1[channelnum]);

    ch_syncreg->mcde_chnl_prio =
        (
            (ch_syncreg->mcde_chnl_prio &~MCDE_CHPRIORITY_MASK) |
            (priority & MCDE_CHPRIORITY_MASK)
        );

    return(error);
}

mcde_error mcdesetoutdevicelpfandppl(mcde_ch_id chid, u16 channelnum, u16 lpf, u16 ppl)
{
    mcde_error        error = MCDE_OK;
    struct mcde_chnl_conf_reg *ch_syncreg;

    ch_syncreg = (struct mcde_chnl_conf_reg *) (gpar[chid]->ch_regbase1[channelnum]);

    ch_syncreg->mcde_chnl_conf = ((lpf & MCDE_CHNLCONF_LPF_MASK) << MCDE_CHNLCONF_LPF_SHIFT) |
                                 ((ppl & MCDE_CHNLCONF_PPL_MASK) << MCDE_CHNLCONF_PPL_SHIFT);

    return(error);
}

mcde_error mcdesetchnlCconf(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_chc_config config)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_RES1_MASK) |
                    (((u32) config.res_pol << MCDE_RES1_SHIFT) & MCDE_RES1_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_RD1_MASK) |
                    (((u32) config.rd_pol << MCDE_RD1_SHIFT) & MCDE_RD1_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_WR1_MASK) |
                    (((u32) config.wr_pol << MCDE_WR1_SHIFT) & MCDE_WR1_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_CD1_MASK) |
                    (((u32) config.cd_pol << MCDE_CD1_SHIFT) & MCDE_CD1_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_CS1_MASK) |
                    (((u32) config.cs_pol << MCDE_CS1_SHIFT) & MCDE_CS1_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_CS1EN_MASK) |
                    (((u32) config.csen << MCDE_CS1EN_SHIFT) & MCDE_CS1EN_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_INBAND1_MASK) |
                    (((u32) config.inband_mode << MCDE_INBAND1_SHIFT) & MCDE_INBAND1_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_BUSSIZE1_MASK) |
                    (((u32) config.bus_size << MCDE_BUSSIZE1_SHIFT) & MCDE_BUSSIZE1_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_SYNCEN1_MASK) |
                    (((u32) config.syncen << MCDE_SYNCEN1_SHIFT) & MCDE_SYNCEN1_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_WMLVL1_MASK) |
                    (((u32) config.fifo_watermark << MCDE_WMLVL1_SHIFT) & MCDE_WMLVL1_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_C1EN_MASK) |
                    (((u32) config.chcen << MCDE_C1EN_SHIFT) & MCDE_C1EN_MASK)
                );

            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_RES2_MASK) |
                    (((u32) config.res_pol << MCDE_RES2_SHIFT) & MCDE_RES2_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_RD2_MASK) |
                    (((u32) config.rd_pol << MCDE_RD2_SHIFT) & MCDE_RD2_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_WR2_MASK) |
                    (((u32) config.wr_pol << MCDE_WR2_SHIFT) & MCDE_WR2_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_CD2_MASK) |
                    (((u32) config.cd_pol << MCDE_CD2_SHIFT) & MCDE_CD2_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_CS2_MASK) |
                    (((u32) config.cs_pol << MCDE_CS2_SHIFT) & MCDE_CS2_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_CS2EN_MASK) |
                    (((u32) config.csen << MCDE_CS2EN_SHIFT) & MCDE_CS2EN_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_INBAND2_MASK) |
                    (((u32) config.inband_mode << MCDE_INBAND2_SHIFT) & MCDE_INBAND2_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_BUSSIZE2_MASK) |
                    (((u32) config.bus_size << MCDE_BUSSIZE2_SHIFT) & MCDE_BUSSIZE2_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_SYNCEN2_MASK) |
                    (((u32) config.syncen << MCDE_SYNCEN2_SHIFT) & MCDE_SYNCEN2_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_WMLVL2_MASK) |
                    (((u32) config.fifo_watermark << MCDE_WMLVL2_SHIFT) & MCDE_WMLVL2_MASK)
                );

            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_C2EN_MASK) |
                    (((u32) config.chcen << MCDE_C2EN_SHIFT) & MCDE_C2EN_MASK)
                );

            break;

        default:
           return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdesetchnlCctrl(mcde_ch_id chid, struct mcde_chc_ctrl control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    ch_c_reg->mcde_crc =
        (
            (ch_c_reg->mcde_crc &~MCDE_SYNCCTRL_MASK) |
            (((u32) control.sync << MCDE_SYNCCTRL_SHIFT) & MCDE_SYNCCTRL_MASK)
        );

    ch_c_reg->mcde_crc =
        (
            (ch_c_reg->mcde_crc &~MCDE_RESEN_MASK) |
            (((u32) control.resen << MCDE_RESEN_SHIFT) & MCDE_RESEN_MASK)
        );

    ch_c_reg->mcde_crc =
        (
            (ch_c_reg->mcde_crc &~MCDE_CLKSEL_MASK) |
            (((u32) control.clksel << MCDE_CLKSEL_SHIFT) & MCDE_CLKSEL_MASK)
        );

    ch_c_reg->mcde_crc =
        (
            (ch_c_reg->mcde_crc &~MCDE_SYNCSEL_MASK) |
            (((u32) control.synsel << MCDE_SYNCSEL_SHIFT) & MCDE_SYNCSEL_MASK)
        );

    return(error);
}

mcde_error mcdesetchnlXpowermode(mcde_ch_id chid, mcde_powen_select power)
{
  mcde_error    error = MCDE_OK;

  if (chid <= MCDE_CH_B)
  { /* Channel A or B */
    struct mcde_chAB_reg *ch_x_reg = (struct mcde_chAB_reg *) gpar[chid]->ch_regbase2[chid];
    ch_x_reg->mcde_cr0 = (ch_x_reg->mcde_cr0 & ~MCDE_CR0_POWEREN) |
                         (power != 0 ? MCDE_CR0_POWEREN : 0);
  } else
  { /* Channel C0 or C1 */
    struct mcde_chC0C1_reg *ch_c_reg = (struct mcde_chC0C1_reg *) gpar[chid]->ch_c_reg;
    ch_c_reg->mcde_crc = (ch_c_reg->mcde_crc & ~MCDE_CRC_POWEREN) |
                         (power != 0 ? MCDE_CRC_POWEREN : 0);
  }

  return(error);
}

mcde_error mcdesetchnlXflowmode(mcde_ch_id chid, mcde_flow_select flow)
{
    mcde_error    error = MCDE_OK;

	if (chid <= MCDE_CH_B)
  { /* Channel A or B */
		struct mcde_chAB_reg *ch_x_reg = (struct mcde_chAB_reg *) gpar[chid]->ch_regbase2[chid];
    ch_x_reg->mcde_cr0 = (ch_x_reg->mcde_cr0 & ~MCDE_CR0_FLOEN) | (flow != 0 ? MCDE_CR0_FLOEN : 0);
	} else
  { /* Channel C0 or C1 */
    struct mcde_chC0C1_reg *ch_c_reg = (struct mcde_chC0C1_reg *) gpar[chid]->ch_c_reg;
    ch_c_reg->mcde_crc = (ch_c_reg->mcde_crc & ~MCDE_CRC_FLOEN) | (flow != 0 ? MCDE_CRC_FLOEN : 0);
	}

    return(error);
}

mcde_error mcdeconfPBCunit(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_pbc_config config)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_pbccrc[0] =
                (
                    (ch_c_reg->mcde_pbccrc[0] &~MCDE_PBCCRC_PDCTRL) |
                    (config.duplex_mode != 0 ? MCDE_PBCCRC_PDCTRL : 0)
                );

            ch_c_reg->mcde_pbccrc[0] =
                (
                    (ch_c_reg->mcde_pbccrc[0] &~(MCDE_PBCCRC_PDM_MASK << MCDE_PBCCRC_PDM_SHIFT)) |
                    ((config.duplex_mode & MCDE_PBCCRC_PDM_MASK) << MCDE_PBCCRC_PDM_SHIFT)
                );

            ch_c_reg->mcde_pbccrc[0] =
                (
                    (ch_c_reg->mcde_pbccrc[0] &~(MCDE_PBCCRC_BSDM_MASK << MCDE_PBCCRC_BSDM_SHIFT)) |
                    ((config.data_segment & MCDE_PBCCRC_BSDM_MASK) << MCDE_PBCCRC_BSDM_SHIFT)
                );

            ch_c_reg->mcde_pbccrc[0] =
                (
                    (ch_c_reg->mcde_pbccrc[0] &~(MCDE_PBCCRC_BSCM_MASK << MCDE_PBCCRC_BSCM_SHIFT)) |
                    ((config.cmd_segment & MCDE_PBCCRC_BSCM_MASK) << MCDE_PBCCRC_BSCM_SHIFT)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_pbccrc[1] =
                (
                    (ch_c_reg->mcde_pbccrc[1] &~MCDE_PBCCRC_PDCTRL) |
                    (config.duplex_mode != 0 ? MCDE_PBCCRC_PDCTRL : 0)
                );

            ch_c_reg->mcde_pbccrc[1] =
                (
                    (ch_c_reg->mcde_pbccrc[1] &~(MCDE_PBCCRC_PDM_MASK << MCDE_PBCCRC_PDM_SHIFT)) |
                    ((config.duplex_mode & MCDE_PBCCRC_PDM_MASK) << MCDE_PBCCRC_PDM_SHIFT)
                );

            ch_c_reg->mcde_pbccrc[1] =
                (
                    (ch_c_reg->mcde_pbccrc[1] &~(MCDE_PBCCRC_BSDM_MASK << MCDE_PBCCRC_BSDM_SHIFT)) |
                    ((config.data_segment & MCDE_PBCCRC_BSDM_MASK) << MCDE_PBCCRC_BSDM_SHIFT)
                );

            ch_c_reg->mcde_pbccrc[1] =
                (
                    (ch_c_reg->mcde_pbccrc[1] &~(MCDE_PBCCRC_BSCM_MASK << MCDE_PBCCRC_BSCM_SHIFT)) |
                    ((config.cmd_segment & MCDE_PBCCRC_BSCM_MASK) << MCDE_PBCCRC_BSCM_SHIFT)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdesetPBCmux(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_pbc_mux mux)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_pbcbmrc0[0] = mux.imux0;

            ch_c_reg->mcde_pbcbmrc0[1] = mux.imux1;

            ch_c_reg->mcde_pbcbmrc0[2] = mux.imux2;

            ch_c_reg->mcde_pbcbmrc0[3] = mux.imux3;

            ch_c_reg->mcde_pbcbmrc0[4] = mux.imux4;
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_pbcbmrc1[0] = mux.imux0;

            ch_c_reg->mcde_pbcbmrc1[1] = mux.imux1;

            ch_c_reg->mcde_pbcbmrc1[2] = mux.imux2;

            ch_c_reg->mcde_pbcbmrc1[3] = mux.imux3;

            ch_c_reg->mcde_pbcbmrc1[4] = mux.imux4;
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdesetchnlCvsyndelay(mcde_ch_id chid, mcde_chc_panel panel_id, u8 delay)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_sctrc = ((ch_c_reg->mcde_sctrc &~MCDE_SYNCDELC0_MASK) | (delay & MCDE_SYNCDELC0_MASK));

            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_sctrc =
                (
                    (ch_c_reg->mcde_sctrc &~MCDE_SYNCDELC1_MASK) |
                    ((delay << MCDE_SYNCDELC1_SHIFT) & MCDE_SYNCDELC1_MASK)
                );

            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}

mcde_error mcdesetchnlCsynctrigdelay(mcde_ch_id chid, u8 delay)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    ch_c_reg->mcde_sctrc =
        (
            (ch_c_reg->mcde_sctrc &~MCDE_TRDELC_MASK) |
            ((delay << MCDE_TRDELC_SHIFT) & MCDE_TRDELC_MASK)
        );


    return(error);
}
mcde_error mcdesetPBCbitctrl(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_pbc_bitctrl bit_control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_pbcbcrc0[0] = bit_control.bit_ctrl0;

            ch_c_reg->mcde_pbcbcrc0[1] = bit_control.bit_ctrl1;
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_pbcbcrc1[0] = bit_control.bit_ctrl0;

            ch_c_reg->mcde_pbcbcrc1[1] = bit_control.bit_ctrl1;
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}

mcde_error mcdesetchnlCsynccapconf(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_sync_conf config)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_vscrc[0] =
                (
                    (ch_c_reg->mcde_vscrc[0] &~MCDE_VSDBL_MASK) |
                    ((config.debounce_length << MCDE_VSDBL_SHIFT) & MCDE_VSDBL_MASK)
                );

            ch_c_reg->mcde_vscrc[0] =
                (
                    (ch_c_reg->mcde_vscrc[0] &~MCDE_VSSEL_MASK) |
                    (((u32) config.sync_sel << MCDE_VSSEL_SHIFT) & MCDE_VSSEL_MASK)
                );

            ch_c_reg->mcde_vscrc[0] =
                (
                    (ch_c_reg->mcde_vscrc[0] &~MCDE_VSPOL_MASK) |
                    (((u32) config.sync_pol << MCDE_VSPOL_SHIFT) & MCDE_VSPOL_MASK)
                );

            ch_c_reg->mcde_vscrc[0] =
                (
                    (ch_c_reg->mcde_vscrc[0] &~MCDE_VSPDIV_MASK) |
                    (((u32) config.clk_div << MCDE_VSPDIV_SHIFT) & MCDE_VSPDIV_MASK)
                );

            ch_c_reg->mcde_vscrc[0] =
                (
                    (ch_c_reg->mcde_vscrc[0] &~MCDE_VSPMAX_MASK) |
                    (((u32) config.vsp_max << MCDE_VSPMAX_SHIFT) & MCDE_VSPMAX_MASK)
                );

            ch_c_reg->mcde_vscrc[0] =
                (
                    (ch_c_reg->mcde_vscrc[0] &~MCDE_VSPMIN_MASK) |
                    ((u32) config.vsp_min & MCDE_VSPMIN_MASK)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_vscrc[1] =
                (
                    (ch_c_reg->mcde_vscrc[1] &~MCDE_VSDBL_MASK) |
                    ((config.debounce_length << MCDE_VSDBL_SHIFT) & MCDE_VSDBL_MASK)
                );

            ch_c_reg->mcde_vscrc[1] =
                (
                    (ch_c_reg->mcde_vscrc[1] &~MCDE_VSSEL_MASK) |
                    (((u32) config.sync_sel << MCDE_VSSEL_SHIFT) & MCDE_VSSEL_MASK)
                );

            ch_c_reg->mcde_vscrc[1] =
                (
                    (ch_c_reg->mcde_vscrc[1] &~MCDE_VSPOL_MASK) |
                    (((u32) config.sync_pol << MCDE_VSPOL_SHIFT) & MCDE_VSPOL_MASK)
                );

            ch_c_reg->mcde_vscrc[1] =
                (
                    (ch_c_reg->mcde_vscrc[1] &~MCDE_VSPDIV_MASK) |
                    (((u32) config.clk_div << MCDE_VSPDIV_SHIFT) & MCDE_VSPDIV_MASK)
                );

            ch_c_reg->mcde_vscrc[1] =
                (
                    (ch_c_reg->mcde_vscrc[1] &~MCDE_VSPMAX_MASK) |
                    (((u32) config.vsp_max << MCDE_VSPMAX_SHIFT) & MCDE_VSPMAX_MASK)
                );

            ch_c_reg->mcde_vscrc[1] =
                (
                    (ch_c_reg->mcde_vscrc[1] &~MCDE_VSPMIN_MASK) |
                    ((u32) config.vsp_min & MCDE_VSPMIN_MASK)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdechnlCclkandsyncsel(mcde_ch_id chid, mcde_clk_sel clk_sel, mcde_synchro_select sync_select)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    ch_c_reg->mcde_crc =
        (
            (ch_c_reg->mcde_crc &~MCDE_CLKSEL_MASK) |
            (((u32) clk_sel << MCDE_CLKSEL_SHIFT) & MCDE_CLKSEL_MASK)
        );

    ch_c_reg->mcde_crc =
        (
            (ch_c_reg->mcde_crc &~MCDE_SYNCSEL_MASK) |
            (((u32) sync_select << MCDE_SYNCSEL_SHIFT) & MCDE_SYNCSEL_MASK)
        );

    return(error);
}

mcde_error mcdesetchnlCmode(mcde_ch_id chid, mcde_chc_panel panel_id, mcde_chc_enable state)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_C1EN_MASK) |
                    ((state << MCDE_C1EN_SHIFT) & MCDE_C1EN_MASK)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_C2EN_MASK) |
                    ((state << MCDE_C2EN_SHIFT) & MCDE_C2EN_MASK)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }


    return(error);
}

mcde_error mcdesetsynchromode(mcde_ch_id chid, mcde_chc_panel panel_id, mcde_synchro_capture sync_enable)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);


    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_SYNCEN1_MASK) |
                    (((u32) sync_enable << MCDE_SYNCEN1_SHIFT) & MCDE_SYNCEN1_MASK)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_crc =
                (
                    (ch_c_reg->mcde_crc &~MCDE_SYNCEN2_MASK) |
                    (((u32) sync_enable << MCDE_SYNCEN2_SHIFT) & MCDE_SYNCEN2_MASK)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}

mcde_error mcdesetchipseltiming(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_cd_timing_activate active)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_cscdtr[0] =
                (
                    (ch_c_reg->mcde_cscdtr[0] &~MCDE_CSCDDEACT_MASK) |
                    ((active.cs_cd_deactivate << MCDE_CSCDDEACT_SHIFT) & MCDE_CSCDDEACT_MASK)
                );

            ch_c_reg->mcde_cscdtr[0] =
                (
                    (ch_c_reg->mcde_cscdtr[0] &~MCDE_CSCDACT_MASK) |
                    (active.cs_cd_activate & MCDE_CSCDACT_MASK)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_cscdtr[1] =
                (
                    (ch_c_reg->mcde_cscdtr[1] &~MCDE_CSCDDEACT_MASK) |
                    ((active.cs_cd_deactivate << MCDE_CSCDDEACT_SHIFT) & MCDE_CSCDDEACT_MASK)
                );

            ch_c_reg->mcde_cscdtr[1] =
                (
                    (ch_c_reg->mcde_cscdtr[1] &~MCDE_CSCDACT_MASK) |
                    (active.cs_cd_activate & MCDE_CSCDACT_MASK)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdesetbusaccessnum(mcde_ch_id chid, mcde_chc_panel panel_id, u8 bcn)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_bcnr[0] = ((ch_c_reg->mcde_bcnr[0] &~MCDE_BCN_MASK) | (bcn & MCDE_BCN_MASK));
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_bcnr[1] = ((ch_c_reg->mcde_bcnr[1] &~MCDE_BCN_MASK) | (bcn & MCDE_BCN_MASK));
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdesetrdwrtiming(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_rw_timing rw_time)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_rdwrtr[0] =
                (
                    (ch_c_reg->mcde_rdwrtr[0] &~MCDE_MOTINT_MASK) |
                    (((u32) rw_time.panel_type << MCDE_MOTINT_SHIFT) & MCDE_MOTINT_MASK)
                );

            ch_c_reg->mcde_rdwrtr[0] =
                (
                    (ch_c_reg->mcde_rdwrtr[0] &~MCDE_RWDEACT_MASK) |
                    ((rw_time.readwrite_deactivate << MCDE_RWDEACT_SHIFT) & MCDE_RWDEACT_MASK)
                );

            ch_c_reg->mcde_rdwrtr[0] =
                (
                    (ch_c_reg->mcde_rdwrtr[0] &~MCDE_RWACT_MASK) |
                    (rw_time.readwrite_activate & MCDE_RWACT_MASK)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_rdwrtr[1] =
                (
                    (ch_c_reg->mcde_rdwrtr[1] &~MCDE_MOTINT_MASK) |
                    (((u32) rw_time.panel_type << MCDE_MOTINT_SHIFT) & MCDE_MOTINT_MASK)
                );

            ch_c_reg->mcde_rdwrtr[1] =
                (
                    (ch_c_reg->mcde_rdwrtr[1] &~MCDE_RWDEACT_MASK) |
                    ((rw_time.readwrite_deactivate << MCDE_RWDEACT_SHIFT) & MCDE_RWDEACT_MASK)
                );

            ch_c_reg->mcde_rdwrtr[1] =
                (
                    (ch_c_reg->mcde_rdwrtr[1] &~MCDE_RWACT_MASK) |
                    (rw_time.readwrite_activate & MCDE_RWACT_MASK)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}

mcde_error mcdesetdataouttiming(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_data_out_timing data_time)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_dotr[0] =
                (
                    (ch_c_reg->mcde_dotr[0] &~MCDE_DODEACT_MASK) |
                    ((data_time.data_out_deactivate << MCDE_DODEACT_SHIFT) & MCDE_DODEACT_MASK)
                );

            ch_c_reg->mcde_dotr[0] =
                (
                    (ch_c_reg->mcde_dotr[0] &~MCDE_DOACT_MASK) |
                    (data_time.data_out_activate & MCDE_DOACT_MASK)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_dotr[1] =
                (
                    (ch_c_reg->mcde_dotr[1] &~MCDE_DODEACT_MASK) |
                    ((data_time.data_out_deactivate << MCDE_DODEACT_SHIFT) & MCDE_DODEACT_MASK)
                );

            ch_c_reg->mcde_dotr[1] =
                (
                    (ch_c_reg->mcde_dotr[1] &~MCDE_DOACT_MASK) |
                    (data_time.data_out_activate & MCDE_DOACT_MASK)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}

mcde_error mcdewrcmd(mcde_ch_id chid, mcde_chc_panel panel_id, u32 command)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_wcmdc[0] = (command & MCDE_DATACOMMANDMASK);

            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_wcmdc[1] = (command & MCDE_DATACOMMANDMASK);

            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}

mcde_error mcdewrdata(mcde_ch_id chid, mcde_chc_panel panel_id, u32 data)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_wdatadc[0] = (data & MCDE_DATACOMMANDMASK);
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_wdatadc[1] = (data & MCDE_DATACOMMANDMASK);
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdewrtxfifo(mcde_ch_id chid, mcde_chc_panel panel_id, mcde_txfifo_request_type type, u32 data)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chC0C1_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chC0C1_reg *) (gpar[chid]->ch_c_reg);
    switch (type)
    {
        case MCDE_TXFIFO_WRITE_DATA:
            ch_c_reg->mcde_wdatadc[panel_id] = (data & MCDE_DATACOMMANDMASK);

            break;

        case MCDE_TXFIFO_WRITE_COMMAND:
            ch_c_reg->mcde_wcmdc[panel_id] = (data & MCDE_DATACOMMANDMASK);

            break;

        default:
            error = MCDE_INVALID_PARAMETER;
    }

    return(error);
}

mcde_error mcdesetcflowXcolorkeyctrl(mcde_ch_id chid, mcde_key_ctrl key_ctrl)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_cr0 =
            (ch_x_reg->mcde_cr0 &~(MCDE_CR0_KEYCTRL_MASK << MCDE_CR0_KEYCTRL_SHIFT)) |
            ((key_ctrl & MCDE_CR0_KEYCTRL_MASK) << MCDE_CR0_KEYCTRL_SHIFT);

    return(error);
}

mcde_error mcdesetblendctrl(mcde_ch_id chid, struct mcde_blend_control blend_ctrl)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }
    /* TODO: Fix me! Strange that alpha_blend and blend trigger the same bit */
    ch_x_reg->mcde_cr0 =
            (ch_x_reg->mcde_cr0 &~MCDE_CR0_BLENDEN) |
            (blend_ctrl.alpha_blend != 0 ? MCDE_CR0_BLENDEN : 0);

    ch_x_reg->mcde_cr0 =
            (ch_x_reg->mcde_cr0 &~MCDE_CR0_BLENDCTRL) |
            (blend_ctrl.blend_ctrl != 0 ? MCDE_CR0_BLENDCTRL : 0);

    ch_x_reg->mcde_cr0 =
            (ch_x_reg->mcde_cr0 &~MCDE_CR0_BLENDEN) |
            (blend_ctrl.blenden != 0 ? MCDE_CR0_BLENDEN : 0);

    return(error);
}

mcde_error mcdesetrotation(mcde_ch_id chid, mcde_rot_dir rot_dir, mcde_roten rot_ctrl)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;


    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }
    if (rot_ctrl == MCDE_ROTATION_ENABLE)
    {
      ch_x_reg->mcde_rot_conf =
        (ch_x_reg->mcde_rot_conf &~MCDE_ROTCONF_ROTDIR) |
        (rot_dir != 0 ? MCDE_ROTCONF_ROTDIR : 0);//(((u32) rot_dir << MCDE_CHX_ROTDIR_SHIFT) & MCDE_CHX_ROTDIR_MASK)
    }
    ch_x_reg->mcde_cr0 =
            (ch_x_reg->mcde_cr0 &~MCDE_CR0_ROTEN) |
            (rot_ctrl != 0 ? MCDE_CR0_ROTEN : 0);//(((u32) rot_ctrl << MCDE_ROTEN_SHIFT) & MCDE_ROTEN_MASK)

    return error;
}
mcde_error mcdesetditheringctrl(mcde_ch_id chid, mcde_dithering_ctrl dithering_control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }
    ch_x_reg->mcde_cr0 =
        (
            (ch_x_reg->mcde_cr0 &~MCDE_CR0_DITHEN) |
            (dithering_control != 0 ? MCDE_CR0_DITHEN : 0)//(((u32) dithering_control << MCDE_DITHEN_SHIFT) & MCDE_DITHEN_MASK)
        );
    return (error);
}
mcde_error mcdesetflowXctrl(mcde_ch_id chid, struct mcde_chx_control0 control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;


    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_cr0 &= ~((MCDE_CR0_ROTBURSTSIZE_MASK << MCDE_CR0_ROTBURSTSIZE_SHIFT) |
                            (MCDE_CR0_ALPHABLEND_MASK << MCDE_CR0_ALPHABLEND_SHIFT) |
                            (MCDE_CR0_GAMEN) | (MCDE_CR0_FLICKFORMAT) | (MCDE_CR0_FLICKMODE_MASK << MCDE_CR0_FLICKMODE_SHIFT) |
                            (MCDE_CR0_BLENDCTRL) | (MCDE_CR0_KEYCTRL_MASK << MCDE_CR0_KEYCTRL_SHIFT) |
                            (MCDE_CR0_ROTEN) | (MCDE_CR0_DITHEN) | (MCDE_CR0_AFLICKEN) | (MCDE_CR0_BLENDEN));

    ch_x_reg->mcde_cr0 |= (control.chx_read_request & MCDE_CR0_ROTBURSTSIZE_MASK) << MCDE_CR0_ROTBURSTSIZE_SHIFT;
            /*(ch_x_reg->mcde_ch_cr0 &~MCDE_CHX_BURSTSIZE_MASK) |
            ((u32) control.chx_read_request << MCDE_CHX_BURSTSIZE_SHIFT) & MCDE_CHX_BURSTSIZE_MASK);*/

    ch_x_reg->mcde_cr0 |= (control.alpha_blend & MCDE_CR0_ALPHABLEND_MASK) << MCDE_CR0_ALPHABLEND_SHIFT;
        /*(
            (ch_x_reg->mcde_ch_cr0 &~MCDE_CHX_ALPHA_MASK) |
            (((u32) control.alpha_blend << MCDE_CHX_ALPHA_SHIFT) & MCDE_CHX_ALPHA_MASK)
        );*/


    ch_x_reg->mcde_rot_conf = (ch_x_reg->mcde_rot_conf & ~MCDE_ROTCONF_ROTDIR) |
                              (control.rot_dir != 0 ? MCDE_ROTCONF_ROTDIR : 0);

    ch_x_reg->mcde_cr0 |= control.gamma_ctrl ? MCDE_CR0_GAMEN : 0;
        /*(
            (ch_x_reg->mcde_ch_cr0 &~MCDE_CHX_GAMAEN_MASK) |
            (((u32) control.gamma_ctrl << MCDE_CHX_GAMAEN_SHIFT) & MCDE_CHX_GAMAEN_MASK)
        );*/

    ch_x_reg->mcde_cr0 |= control.flicker_format ? MCDE_CR0_FLICKFORMAT : 0;
        /*(
            (ch_x_reg->mcde_ch_cr0 &~MCDE_FLICKFORMAT_MASK) |
            (((u32) control.flicker_format << MCDE_FLICKFORMAT_SHIFT) & MCDE_FLICKFORMAT_MASK)
        );*/

    ch_x_reg->mcde_cr0 |= (control.filter_mode & MCDE_CR0_FLICKMODE_MASK) << MCDE_CR0_FLICKMODE_SHIFT;
        /*(
            (ch_x_reg->mcde_ch_cr0 &~MCDE_FLICKMODE_MASK) |
            (((u32) control.filter_mode << MCDE_FLICKMODE_SHIFT) & MCDE_FLICKMODE_MASK)
        );*/

    ch_x_reg->mcde_cr0 |= control.blend ? MCDE_CR0_BLENDCTRL : 0;
        /*(
            (ch_x_reg->mcde_ch_cr0 &~MCDE_BLENDCONTROL_MASK) |
            (((u32) control.blend << MCDE_BLENDCONTROL_SHIFT) & MCDE_BLENDCONTROL_MASK)
        );*/

    ch_x_reg->mcde_cr0 |= (control.key_ctrl & MCDE_CR0_KEYCTRL_MASK) << MCDE_CR0_KEYCTRL_SHIFT;
        /*(
            (ch_x_reg->mcde_ch_cr0 &~MCDE_KEYCTRL_MASK) |
            (((u32) control.key_ctrl << MCDE_KEYCTRL_SHIFT) & MCDE_KEYCTRL_MASK)
        );*/

    ch_x_reg->mcde_cr0 |= control.rot_enable != 0 ? MCDE_CR0_ROTEN : 0;
        /*(
            (ch_x_reg->mcde_ch_cr0 &~MCDE_ROTEN_MASK) |
            (((u32) control.rot_enable << MCDE_ROTEN_SHIFT) & MCDE_ROTEN_MASK)
        );*/

    ch_x_reg->mcde_cr0 |= control.dither_ctrl != 0 ? MCDE_CR0_DITHEN : 0;
        /*(
            (ch_x_reg->mcde_ch_cr0 &~MCDE_DITHEN_MASK) |
            (((u32) control.dither_ctrl << MCDE_DITHEN_SHIFT) & MCDE_DITHEN_MASK)
        );*/

    // TODO: Not implemented yet CEAEN Color Enhancement Algorithm enable
    /*ch_x_reg->mcde_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_CEAEN_MASK) |
            (((u32) control.color_enhance << MCDE_CEAEN_SHIFT) & MCDE_CEAEN_MASK)
        );*/

    ch_x_reg->mcde_cr0 |= control.anti_flicker != 0 ? MCDE_CR0_AFLICKEN : 0;
        /*(
            (ch_x_reg->mcde_ch_cr0 &~MCDE_AFLICKEN_MASK) |
            (((u32) control.anti_flicker << MCDE_AFLICKEN_SHIFT) & MCDE_AFLICKEN_MASK)
        );*/

    ch_x_reg->mcde_cr0 |= control.blend_ctrl != 0 ? MCDE_CR0_BLENDEN : 0;
        /*(
            (ch_x_reg->mcde_ch_cr0 &~MCDE_BLENDEN_MASK) |
            (((u32) control.blend_ctrl << MCDE_BLENDEN_SHIFT) & MCDE_BLENDEN_MASK)
        );*/


    return(error);
}

mcde_error mcdesetpanelctrl(mcde_ch_id chid, struct mcde_chx_control1 control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

	if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_cr1 =
        (
            (ch_x_reg->mcde_cr1 &~MCDE_CLK_MASK) |
            (((u32) control.tv_clk << MCDE_CLK_SHIFT) & MCDE_CLK_MASK)
        );

    ch_x_reg->mcde_cr1 =
        (
            (ch_x_reg->mcde_cr1 &~MCDE_BCD_MASK) |
            (((u32) control.bcd_ctrl << MCDE_BCD_SHIFT) & MCDE_BCD_MASK)
        );

    ch_x_reg->mcde_cr1 =
        (
            (ch_x_reg->mcde_cr1 &~MCDE_OUTBPP_MASK) |
            (((u32) control.out_bpp << MCDE_OUTBPP_SHIFT) & MCDE_OUTBPP_MASK)
        );
/*  Only applicable to older version of chip
    ch_x_reg->mcde_ch_cr1 =
        (
            (ch_x_reg->mcde_ch_cr1 &~MCDE_CLP_MASK) |
            (((u32) control.clk_per_line << MCDE_CLP_SHIFT) & MCDE_CLP_MASK)
        );
*/

    ch_x_reg->mcde_cr1 =
        (
            (ch_x_reg->mcde_cr1 &~MCDE_CDWIN_MASK) |
            (((u32) control.lcd_bus << MCDE_CDWIN_SHIFT) & MCDE_CDWIN_MASK)
        );

    ch_x_reg->mcde_cr1 =
        (
            (ch_x_reg->mcde_cr1 &~MCDE_CLOCKSEL_MASK) |
            (((u32) control.dpi2_clk << MCDE_CLOCKSEL_SHIFT) & MCDE_CLOCKSEL_MASK)
        );

    ch_x_reg->mcde_cr1 = ((ch_x_reg->mcde_cr1 &~MCDE_PCD_MASK) | ((u32) control.pcd & MCDE_PCD_MASK));

    return(error);
}

mcde_error mcdesetcolorkey(mcde_ch_id chid, struct mcde_chx_color_key key, mcde_colorkey_type type)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;


    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

	if (type == MCDE_COLORKEY_NORMAL)
	{
	ch_x_reg->mcde_colkey =
		(
		(ch_x_reg->mcde_colkey &~MCDE_KEYA_MASK) |
		((key.alpha << MCDE_KEYA_SHIFT) & MCDE_KEYA_MASK)
		);

	ch_x_reg->mcde_colkey =
        (
            (ch_x_reg->mcde_colkey &~MCDE_KEYR_MASK) |
            ((key.red << MCDE_KEYR_SHIFT) & MCDE_KEYR_MASK)
        );

	ch_x_reg->mcde_colkey =
        (
            (ch_x_reg->mcde_colkey &~MCDE_KEYG_MASK) |
            ((key.green << MCDE_KEYG_SHIFT) & MCDE_KEYG_MASK)
        );

	ch_x_reg->mcde_colkey = ((ch_x_reg->mcde_colkey &~MCDE_KEYB_MASK) | (key.blue & MCDE_KEYB_MASK));
	}
	else
	if (type == MCDE_COLORKEY_FORCE)
	{
	    ch_x_reg->mcde_fcolkey =
        (
            (ch_x_reg->mcde_fcolkey &~MCDE_KEYA_MASK) |
            ((key.alpha << MCDE_KEYA_SHIFT) & MCDE_KEYA_MASK)
        );

	ch_x_reg->mcde_fcolkey =
        (
            (ch_x_reg->mcde_fcolkey &~MCDE_KEYR_MASK) |
            ((key.red << MCDE_KEYR_SHIFT) & MCDE_KEYR_MASK)
        );

	ch_x_reg->mcde_fcolkey =
        (
            (ch_x_reg->mcde_fcolkey &~MCDE_KEYG_MASK) |
            ((key.green << MCDE_KEYG_SHIFT) & MCDE_KEYG_MASK)
        );

	ch_x_reg->mcde_fcolkey = ((ch_x_reg->mcde_fcolkey &~MCDE_KEYB_MASK) | (key.blue & MCDE_KEYB_MASK));
	}


    return(error);
}
mcde_error mcdesetcolorconvmatrix(mcde_ch_id chid, struct mcde_chx_rgb_conv_coef coef)
{
    mcde_error    error = MCDE_OK;
	struct mcde_chAB_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_rgbconv1 = ((coef.Yr_red & MCDE_RGBCONV1_YR_RED_MASK) << MCDE_RGBCONV1_YR_RED_SHIFT) |
                              ((coef.Yr_green & MCDE_RGBCONV1_YR_GREEN_MASK) << MCDE_RGBCONV1_YR_GREEN_SHIFT);

    ch_x_reg->mcde_rgbconv2 = ((coef.Yr_blue & MCDE_RGBCONV2_YR_BLUE_MASK) << MCDE_RGBCONV2_YR_BLUE_SHIFT) |
                              ((coef.Cr_red & MCDE_RGBCONV2_CR_RED_MASK) << MCDE_RGBCONV2_CR_RED_SHIFT);

    ch_x_reg->mcde_rgbconv3 = ((coef.Cr_green & MCDE_RGBCONV3_CR_GREEN_MASK) << MCDE_RGBCONV3_CR_GREEN_SHIFT) |
                              ((coef.Cr_blue & MCDE_RGBCONV3_CR_BLUE_MASK) << MCDE_RGBCONV3_CR_BLUE_SHIFT);

    ch_x_reg->mcde_rgbconv4 = ((coef.Cb_red & MCDE_RGBCONV4_CB_RED_MASK) << MCDE_RGBCONV4_CB_RED_SHIFT) |
                              ((coef.Cb_green & MCDE_RGBCONV4_CB_GREEN_MASK) << MCDE_RGBCONV4_CB_GREEN_SHIFT);

    ch_x_reg->mcde_rgbconv5 = ((coef.Cb_blue & MCDE_RGBCONV5_CB_BLUE_MASK) << MCDE_RGBCONV5_CB_BLUE_SHIFT) |
                              ((coef.Off_red & MCDE_RGBCONV5_OFF_RED_MASK) << MCDE_RGBCONV5_OFF_RED_SHIFT);

    ch_x_reg->mcde_rgbconv6 = ((coef.Off_green & MCDE_RGBCONV6_OFF_GREEN_MASK) << MCDE_RGBCONV6_OFF_GREEN_SHIFT) |
                              ((coef.Off_blue & MCDE_RGBCONV6_OFF_BLUE_MASK) << MCDE_RGBCONV6_OFF_BLUE_SHIFT);

    return(error);
}
mcde_error mcdesetflickerfiltercoef(mcde_ch_id chid, struct mcde_chx_flickfilter_coef coef)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

	if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }


    ch_x_reg->mcde_ffcoef0 =
        (
            (ch_x_reg->mcde_ffcoef0 &~MCDE_THRESHOLD_MASK) |
            ((coef.threshold_ctrl0 << MCDE_THRESHOLD_SHIFT) & MCDE_THRESHOLD_MASK)
        );

    ch_x_reg->mcde_ffcoef0 =
        (
            (ch_x_reg->mcde_ffcoef0 &~MCDE_COEFFN3_MASK) |
            ((coef.Coeff0_N3 << MCDE_COEFFN3_SHIFT) & MCDE_COEFFN3_MASK)
        );

    ch_x_reg->mcde_ffcoef0 =
        (
            (ch_x_reg->mcde_ffcoef0 &~MCDE_COEFFN2_MASK) |
            ((coef.Coeff0_N2 << MCDE_COEFFN2_SHIFT) & MCDE_COEFFN2_MASK)
        );

    ch_x_reg->mcde_ffcoef0 =
        (
            (ch_x_reg->mcde_ffcoef0 &~MCDE_COEFFN1_MASK) |
            (coef.Coeff0_N1 & MCDE_COEFFN1_MASK)
        );

    ch_x_reg->mcde_ffcoef1 =
        (
            (ch_x_reg->mcde_ffcoef1 &~MCDE_THRESHOLD_MASK) |
            ((coef.threshold_ctrl1 << MCDE_THRESHOLD_SHIFT) & MCDE_THRESHOLD_MASK)
        );

    ch_x_reg->mcde_ffcoef1 =
        (
            (ch_x_reg->mcde_ffcoef1 &~MCDE_COEFFN3_MASK) |
            ((coef.Coeff1_N3 << MCDE_COEFFN3_SHIFT) & MCDE_COEFFN3_MASK)
        );

    ch_x_reg->mcde_ffcoef1 =
        (
            (ch_x_reg->mcde_ffcoef1 &~MCDE_COEFFN2_MASK) |
            ((coef.Coeff1_N2 << MCDE_COEFFN2_SHIFT) & MCDE_COEFFN2_MASK)
        );

    ch_x_reg->mcde_ffcoef1 =
        (
            (ch_x_reg->mcde_ffcoef1 &~MCDE_COEFFN1_MASK) |
            (coef.Coeff1_N1 & MCDE_COEFFN1_MASK)
        );

    ch_x_reg->mcde_ffcoef2 =
        (
            (ch_x_reg->mcde_ffcoef2 &~MCDE_THRESHOLD_MASK) |
            ((coef.threshold_ctrl2 << MCDE_THRESHOLD_SHIFT) & MCDE_THRESHOLD_MASK)
        );

    ch_x_reg->mcde_ffcoef2 =
        (
            (ch_x_reg->mcde_ffcoef2 &~MCDE_COEFFN3_MASK) |
            ((coef.Coeff2_N3 << MCDE_COEFFN3_SHIFT) & MCDE_COEFFN3_MASK)
        );

    ch_x_reg->mcde_ffcoef2 =
        (
            (ch_x_reg->mcde_ffcoef2 &~MCDE_COEFFN2_MASK) |
            ((coef.Coeff2_N2 << MCDE_COEFFN2_SHIFT) & MCDE_COEFFN2_MASK)
        );

    ch_x_reg->mcde_ffcoef2 =
        (
            (ch_x_reg->mcde_ffcoef2 &~MCDE_COEFFN1_MASK) |
            (coef.Coeff2_N1 & MCDE_COEFFN1_MASK)
        );

    return(error);
}

mcde_error mcdesetLCDtiming0ctrl(mcde_ch_id chid, struct mcde_chx_lcd_timing0 control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }


    ch_x_reg->mcde_lcdtim0 =
        (
            (ch_x_reg->mcde_lcdtim0 &~MCDE_REVVAEN_MASK) |
            (((u32) control.rev_va_enable << MCDE_REVVAEN_SHIFT) & MCDE_REVVAEN_MASK)
        );

    ch_x_reg->mcde_lcdtim0 =
        (
            (ch_x_reg->mcde_lcdtim0 &~MCDE_REVTGEN_MASK) |
            (((u32) control.rev_toggle_enable << MCDE_REVTGEN_SHIFT) & MCDE_REVTGEN_MASK)
        );

    ch_x_reg->mcde_lcdtim0 =
        (
            (ch_x_reg->mcde_lcdtim0 &~MCDE_REVLOADSEL_MASK) |
            (((u32) control.rev_sync_sel << MCDE_REVLOADSEL_SHIFT) & MCDE_REVLOADSEL_MASK)
        );

    ch_x_reg->mcde_lcdtim0 =
        (
            (ch_x_reg->mcde_lcdtim0 &~MCDE_REVDEL1_MASK) |
            (((u32) control.rev_delay1 << MCDE_REVDEL1_SHIFT) & MCDE_REVDEL1_MASK)
        );

    ch_x_reg->mcde_lcdtim0 =
        (
            (ch_x_reg->mcde_lcdtim0 &~MCDE_REVDEL0_MASK) |
            (((u32) control.rev_delay0 << MCDE_REVDEL0_SHIFT) & MCDE_REVDEL0_MASK)
        );

    ch_x_reg->mcde_lcdtim0 =
        (
            (ch_x_reg->mcde_lcdtim0 &~MCDE_PSVAEN_MASK) |
            (((u32) control.ps_va_enable << MCDE_PSVAEN_SHIFT) & MCDE_PSVAEN_MASK)
        );

    ch_x_reg->mcde_lcdtim0 =
        (
            (ch_x_reg->mcde_lcdtim0 &~MCDE_PSTGEN_MASK) |
            (((u32) control.ps_toggle_enable << MCDE_PSTGEN_SHIFT) & MCDE_PSTGEN_MASK)
        );

    ch_x_reg->mcde_lcdtim0 =
        (
            (ch_x_reg->mcde_lcdtim0 &~MCDE_PSLOADSEL_MASK) |
            (((u32) control.ps_sync_sel << MCDE_PSLOADSEL_SHIFT) & MCDE_PSLOADSEL_MASK)
        );

    ch_x_reg->mcde_lcdtim0 =
        (
            (ch_x_reg->mcde_lcdtim0 &~MCDE_PSDEL1_MASK) |
            (((u32) control.ps_delay1 << MCDE_PSDEL1_SHIFT) & MCDE_PSDEL1_MASK)
        );

    ch_x_reg->mcde_lcdtim0 =
        (
            (ch_x_reg->mcde_lcdtim0 &~MCDE_PSDEL0_MASK) |
            ((u32) control.ps_delay0 & MCDE_PSDEL0_MASK)
        );

    return(error);
}

mcde_error mcdesetLCDtiming1ctrl(mcde_ch_id chid, struct mcde_chx_lcd_timing1 control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

   if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }


    ch_x_reg->mcde_lcdtim1 =
        (
            (ch_x_reg->mcde_lcdtim1 &~MCDE_IOE_MASK) |
            (((u32) control.io_enable << MCDE_IOE_SHIFT) & MCDE_IOE_MASK)
        );

    ch_x_reg->mcde_lcdtim1 =
        (
            (ch_x_reg->mcde_lcdtim1 &~MCDE_IPC_MASK) |
            (((u32) control.ipc << MCDE_IPC_SHIFT) & MCDE_IPC_MASK)
        );

    ch_x_reg->mcde_lcdtim1 =
        (
            (ch_x_reg->mcde_lcdtim1 &~MCDE_IHS_MASK) |
            (((u32) control.ihs << MCDE_IHS_SHIFT) & MCDE_IHS_MASK)
        );

    ch_x_reg->mcde_lcdtim1 =
        (
            (ch_x_reg->mcde_lcdtim1 &~MCDE_IVS_MASK) |
            (((u32) control.ivs << MCDE_IVS_SHIFT) & MCDE_IVS_MASK)
        );

    ch_x_reg->mcde_lcdtim1 =
        (
            (ch_x_reg->mcde_lcdtim1 &~MCDE_IVP_MASK) |
            (((u32) control.ivp << MCDE_IVP_SHIFT) & MCDE_IPC_MASK)
        );

    ch_x_reg->mcde_lcdtim1 =
        (
            (ch_x_reg->mcde_lcdtim1 &~MCDE_ICLSPL_MASK) |
            (((u32) control.iclspl << MCDE_ICLSPL_SHIFT) & MCDE_ICLSPL_MASK)
        );

    ch_x_reg->mcde_lcdtim1 =
        (
            (ch_x_reg->mcde_lcdtim1 &~MCDE_ICLREV_MASK) |
            (((u32) control.iclrev << MCDE_ICLREV_SHIFT) & MCDE_ICLREV_MASK)
        );

    ch_x_reg->mcde_lcdtim1 =
        (
            (ch_x_reg->mcde_lcdtim1 &~MCDE_ICLSP_MASK) |
            (((u32) control.iclsp << MCDE_ICLSP_SHIFT) & MCDE_ICLSP_MASK)
        );

    ch_x_reg->mcde_lcdtim1 =
        (
            (ch_x_reg->mcde_lcdtim1 &~MCDE_SPLVAEN_MASK) |
            (((u32) control.mcde_spl << MCDE_SPLVAEN_SHIFT) & MCDE_SPLVAEN_MASK)
        );

    ch_x_reg->mcde_lcdtim1 =
        (
            (ch_x_reg->mcde_lcdtim1 &~MCDE_SPLTGEN_MASK) |
            (((u32) control.spltgen << MCDE_SPLTGEN_SHIFT) & MCDE_SPLTGEN_MASK)
        );

    ch_x_reg->mcde_lcdtim1 =
        (
            (ch_x_reg->mcde_lcdtim1 &~MCDE_SPLLOADSEL_MASK) |
            (((u32) control.spl_sync_sel << MCDE_SPLLOADSEL_SHIFT) & MCDE_SPLLOADSEL_MASK)
        );

    ch_x_reg->mcde_lcdtim1 =
        (
            (ch_x_reg->mcde_lcdtim1 &~MCDE_SPLDEL1_MASK) |
            (((u32) control.spl_delay1 << MCDE_SPLDEL1_SHIFT) & MCDE_SPLDEL1_MASK)
        );

    ch_x_reg->mcde_lcdtim1 =
        (
            (ch_x_reg->mcde_lcdtim1 &~MCDE_SPLDEL0_MASK) |
            ((u32) control.spl_delay0 & MCDE_SPLDEL0_MASK)
        );

    return(error);
}
mcde_error mcdesetrotaddr(mcde_ch_id chid, u32 address, mcde_rotate_num rotnum)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

	if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }


    if (0x0 == address)
    {
        return(MCDE_INVALID_PARAMETER);
    }
	if(rotnum == MCDE_ROTATE0)
	    ch_x_reg->mcde_rotadd0 = address;
	else if(rotnum == MCDE_ROTATE1)
		ch_x_reg->mcde_rotadd1 = address;

    return(error);
}

/****************************************************************************/
mcde_error mcdesetstate(mcde_ch_id chid, mcde_state state)
{
    mcde_error    error = MCDE_OK;

#ifdef PLATFORM_8500

    gpar[chid]->regbase->mcde_cr = state > 0 ?
            (gpar[chid]->regbase->mcde_cr | MCDE_CR_MCDEEN) :
            (gpar[chid]->regbase->mcde_cr & ~MCDE_CR_MCDEEN);
#endif
#ifdef PLATFORM_8820
	gpar[chid]->regbase->mcde_cr |= state;
#endif

    return(error);
}

mcde_error mcdesetpalette(mcde_ch_id chid, mcde_palette palette)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;


	if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_pal1 = (palette.alphared & MCDE_PAL1_RED_MASK) << MCDE_PAL1_RED_SHIFT;
    ch_x_reg->mcde_pal0 = ((palette.green & MCDE_PAL0_GREEN_MASK) << MCDE_PAL0_GREEN_SHIFT) |
                          ((palette.blue & MCDE_PAL0_BLUE_MASK) << MCDE_PAL0_BLUE_SHIFT);

    return(error);
}


mcde_error mcdesetditherctrl(mcde_ch_id chid, struct mcde_chx_dither_ctrl control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }
    ch_x_reg->mcde_ditctrl =
        (
            (ch_x_reg->mcde_ditctrl &~MCDE_FOFFY_MASK) |
            ((control.y_offset << MCDE_FOFFY_SHIFT) & MCDE_FOFFY_MASK)
        );

    ch_x_reg->mcde_ditctrl =
        (
            (ch_x_reg->mcde_ditctrl &~MCDE_FOFFX_MASK) |
            ((control.x_offset << MCDE_FOFFX_SHIFT) & MCDE_FOFFX_MASK)
        );

    ch_x_reg->mcde_ditctrl =
        (
            (ch_x_reg->mcde_ditctrl &~MCDE_MASK_BITCTRL_MASK) |
            (((u32) control.masking_ctrl << MCDE_MASK_BITCTRL_SHIFT) & MCDE_MASK_BITCTRL_MASK)
        );

    ch_x_reg->mcde_ditctrl =
        (
            (ch_x_reg->mcde_ditctrl &~MCDE_MODE_MASK) |
            ((control.mode << MCDE_MODE_SHIFT) & MCDE_MODE_MASK)
        );

    ch_x_reg->mcde_ditctrl =
        (
            (ch_x_reg->mcde_ditctrl &~MCDE_COMP_MASK) |
            (((u32) control.comp_dithering << MCDE_COMP_SHIFT) & MCDE_COMP_MASK)
        );

    ch_x_reg->mcde_ditctrl =
        (
            (ch_x_reg->mcde_ditctrl &~MCDE_TEMP_MASK) |
            ((u32) control.temp_dithering & MCDE_TEMP_MASK)
        );
    return(error);
}

mcde_error mcdesetditheroffset(mcde_ch_id chid, struct mcde_chx_dithering_offset offset)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }


    ch_x_reg->mcde_ditctrl =
        (
            (ch_x_reg->mcde_ditctrl &~MCDE_YB_MASK) |
            ((offset.y_offset_rb << MCDE_YB_SHIFT) & MCDE_YB_MASK)
        );

    ch_x_reg->mcde_ditctrl =
        (
            (ch_x_reg->mcde_ditctrl &~MCDE_XB_MASK) |
            ((offset.x_offset_rb << MCDE_XB_SHIFT) & MCDE_XB_MASK)
        );

    ch_x_reg->mcde_ditctrl =
        (
            (ch_x_reg->mcde_ditctrl &~MCDE_YG_MASK) |
            ((offset.y_offset_rg << MCDE_YG_SHIFT) & MCDE_YG_MASK)
        );

    ch_x_reg->mcde_ditctrl = ((ch_x_reg->mcde_ditctrl &~MCDE_XG_MASK) | (offset.x_offset_rg & MCDE_XG_MASK));

    return(error);
}

mcde_error mcdesetgammacoeff(mcde_ch_id chid, struct mcde_chx_gamma gamma)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_gam2 = gamma.red; //(ch_x_reg->mcde_ch_gam &~MCDE_RED_MASK) |
                                     //((gamma.red << MCDE_ARED_SHIFT) & MCDE_RED_MASK);
    ch_x_reg->mcde_gam1 = gamma.green; //(ch_x_reg->mcde_ch_gam &~MCDE_GREEN_MASK) |
                                       //((gamma.green << MCDE_GREEN_SHIFT) & MCDE_GREEN_MASK);
    ch_x_reg->mcde_gam0 = gamma.blue; //(ch_x_reg->mcde_ch_gam &~MCDE_BLUE_MASK) | (gamma.blue & MCDE_BLUE_MASK);

    return(error);
}
mcde_error mcdesetscanmode(mcde_ch_id chid, mcde_scan_mode scan_mode)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }
    ch_x_reg->mcde_tvcr = ((ch_x_reg->mcde_tvcr &~MCDE_INTEREN_MASK) |(((u32) scan_mode << MCDE_INTEREN_SHIFT) & MCDE_INTEREN_MASK));

    return error;
}
mcde_error mcdesetchnlLCDctrlreg(mcde_ch_id chid, struct mcde_chnl_lcd_ctrl_reg lcd_ctrl_reg)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_tvcr = ((ch_x_reg->mcde_tvcr &~MCDE_TV_LINES_MASK) |((lcd_ctrl_reg.num_lines << MCDE_TV_LINES_SHIFT) & MCDE_TV_LINES_MASK));
    ch_x_reg->mcde_tvcr = ((ch_x_reg->mcde_tvcr &~MCDE_TVMODE_MASK) |(((u32) lcd_ctrl_reg.tv_mode << MCDE_TVMODE_SHIFT) & MCDE_TVMODE_MASK));
    ch_x_reg->mcde_tvcr = ((ch_x_reg->mcde_tvcr &~MCDE_IFIELD_MASK) |(((u32) lcd_ctrl_reg.ifield << MCDE_IFIELD_SHIFT) & MCDE_IFIELD_MASK));
    ch_x_reg->mcde_tvcr = ((ch_x_reg->mcde_tvcr &~MCDE_INTEREN_MASK) |(((u32) 0x0 << MCDE_INTEREN_SHIFT) & MCDE_INTEREN_MASK));
    ch_x_reg->mcde_tvcr = ((ch_x_reg->mcde_tvcr &~MCDE_SELMODE_MASK) |((u32) lcd_ctrl_reg.sel_mode & MCDE_SELMODE_MASK));
    ch_x_reg->mcde_tvtim1 = ((ch_x_reg->mcde_tvtim1 & ~MCDE_SWW_MASK) | ((lcd_ctrl_reg.ppl << MCDE_SWW_SHIFT) & MCDE_SWW_MASK));

    return(error);
}
mcde_error mcdesetchnlLCDhorizontaltiming(mcde_ch_id chid, struct mcde_chnl_lcd_horizontal_timing lcd_horizontal_timing)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_tvtim1 = ((ch_x_reg->mcde_tvtim1 & ~MCDE_DHO_MASK) | (lcd_horizontal_timing.hbp & MCDE_DHO_MASK));
    ch_x_reg->mcde_tvlbalw = ((ch_x_reg->mcde_tvlbalw & ~MCDE_ALW_MASK) | ((lcd_horizontal_timing.hfp << MCDE_ALW_SHIFT) & MCDE_ALW_MASK));
    ch_x_reg->mcde_tvlbalw = ((ch_x_reg->mcde_tvlbalw & ~MCDE_LBW_MASK) | (lcd_horizontal_timing.hsw & MCDE_LBW_MASK));

    return(error);
}
mcde_error mcdesetchnlLCDverticaltiming(mcde_ch_id chid, struct mcde_chnl_lcd_vertical_timing lcd_vertical_timing)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chAB_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_chAB_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_tvbl1 = ((ch_x_reg->mcde_tvbl1 & ~MCDE_BSL_MASK) | ((lcd_vertical_timing.vfp << MCDE_BSL_SHIFT) & MCDE_BSL_MASK));
    ch_x_reg->mcde_tvbl1 = ((ch_x_reg->mcde_tvbl1 & ~MCDE_BEL_MASK) | (lcd_vertical_timing.vsw  & MCDE_BEL_MASK));
    ch_x_reg->mcde_tvdvo = ((ch_x_reg->mcde_tvdvo & ~MCDE_DVO1_MASK) | (lcd_vertical_timing.vbp & MCDE_DVO1_MASK));

    return(error);
}
#ifdef PLATFORM_8820

mcde_error mcdesetoutmuxconf(mcde_ch_id chid, mcde_out_bpp outbpp)
{
	mcde_error	  error = MCDE_OK;

	switch (chid)
	{
	case CHANNEL_A:
		if (outbpp == MCDE_BPP_1_TO_8)
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_A_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));

		else if (outbpp == MCDE_BPP_16)
		{
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_A_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX1_MASK) | (((u32)MCDE_CH_A_MID << MCDE_CFG_OUTMUX1_SHIFT)& MCDE_CFG_OUTMUX1_MASK));
		}
		else if (outbpp == MCDE_BPP_24)
		{
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_A_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX1_MASK) | (((u32)MCDE_CH_A_MID << MCDE_CFG_OUTMUX1_SHIFT)& MCDE_CFG_OUTMUX1_MASK));
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX2_MASK) | (((u32)MCDE_CH_A_MSB << MCDE_CFG_OUTMUX2_SHIFT)& MCDE_CFG_OUTMUX2_MASK));
		}
		break;
	case CHANNEL_B:
		if (outbpp == MCDE_BPP_1_TO_8)
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_B_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));

		else if (outbpp == MCDE_BPP_16)
		{
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_B_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX1_MASK) | (((u32)MCDE_CH_B_MID << MCDE_CFG_OUTMUX1_SHIFT)& MCDE_CFG_OUTMUX1_MASK));
		}
		else if (outbpp == MCDE_BPP_24)
		{
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_B_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX2_MASK) | (((u32)MCDE_CH_B_MID << MCDE_CFG_OUTMUX2_SHIFT)& MCDE_CFG_OUTMUX2_MASK));
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX3_MASK) | (((u32)MCDE_CH_B_MSB << MCDE_CFG_OUTMUX3_SHIFT)& MCDE_CFG_OUTMUX3_MASK));
		}
		break;
	default:
		;
	}
	return(error);
}
#else
mcde_error mcdesetoutmuxconf(mcde_ch_id chid, mcde_out_bpp outbpp)
{
	mcde_error	  error = MCDE_OK;

	switch (chid)
	{
	case CHANNEL_A:
		if (outbpp == MCDE_BPP_1_TO_8)
			gpar[chid]->regbase->mcde_conf0 = ((gpar[chid]->regbase->mcde_conf0 & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_A_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));

		else if (outbpp == MCDE_BPP_16)
		{
			gpar[chid]->regbase->mcde_conf0 = ((gpar[chid]->regbase->mcde_conf0 & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_A_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_conf0 = ((gpar[chid]->regbase->mcde_conf0 & ~MCDE_CFG_OUTMUX1_MASK) | (((u32)MCDE_CH_A_MID << MCDE_CFG_OUTMUX1_SHIFT)& MCDE_CFG_OUTMUX1_MASK));
		}
		else if (outbpp == MCDE_BPP_24)
		{
			gpar[chid]->regbase->mcde_conf0 = ((gpar[chid]->regbase->mcde_conf0 & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_A_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_conf0 = ((gpar[chid]->regbase->mcde_conf0 & ~MCDE_CFG_OUTMUX1_MASK) | (((u32)MCDE_CH_A_MID << MCDE_CFG_OUTMUX1_SHIFT)& MCDE_CFG_OUTMUX1_MASK));
			gpar[chid]->regbase->mcde_conf0 = ((gpar[chid]->regbase->mcde_conf0 & ~MCDE_CFG_OUTMUX2_MASK) | (((u32)MCDE_CH_A_MSB << MCDE_CFG_OUTMUX2_SHIFT)& MCDE_CFG_OUTMUX2_MASK));
		}
		break;
	case CHANNEL_B:
		if (outbpp == MCDE_BPP_1_TO_8)
			gpar[chid]->regbase->mcde_conf0 = ((gpar[chid]->regbase->mcde_conf0 & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_B_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));

		else if (outbpp == MCDE_BPP_16)
		{
			gpar[chid]->regbase->mcde_conf0 = ((gpar[chid]->regbase->mcde_conf0 & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_B_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_conf0 = ((gpar[chid]->regbase->mcde_conf0 & ~MCDE_CFG_OUTMUX1_MASK) | (((u32)MCDE_CH_B_MID << MCDE_CFG_OUTMUX1_SHIFT)& MCDE_CFG_OUTMUX1_MASK));
		}
		else if (outbpp == MCDE_BPP_24)
		{
			gpar[chid]->regbase->mcde_conf0 = ((gpar[chid]->regbase->mcde_conf0 & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_B_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_conf0 = ((gpar[chid]->regbase->mcde_conf0 & ~MCDE_CFG_OUTMUX2_MASK) | (((u32)MCDE_CH_B_MID << MCDE_CFG_OUTMUX2_SHIFT)& MCDE_CFG_OUTMUX2_MASK));
			gpar[chid]->regbase->mcde_conf0 = ((gpar[chid]->regbase->mcde_conf0 & ~MCDE_CFG_OUTMUX3_MASK) | (((u32)MCDE_CH_B_MSB << MCDE_CFG_OUTMUX3_SHIFT)& MCDE_CFG_OUTMUX3_MASK));
		}
		break;
	case CHANNEL_C0:
		gpar[chid]->regbase->mcde_conf0 |= 0x5000;//0x5e145000; /** This code needs to be modified */
		break;
	case CHANNEL_C1:
		gpar[chid]->regbase->mcde_conf0 |= 0x5e145000; /** This code needs to be modified */
		break;
	default:
		;
	}
	return(error);
}
#endif
mcde_error mcderesetextsrcovrlay(mcde_ch_id chid)
{
	struct mcde_ext_src_reg  *ext_src;
	struct mcde_ovl_reg  *ovr_config;
	mcde_error retVal = MCDE_OK;

	/** point to the correct ext src register */
	ext_src = (struct mcde_ext_src_reg *) (gpar[chid]->extsrc_regbase[gpar[chid]->mcde_cur_ovl_bmp]);

	/** reset ext src address registers, src conf register and src ctrl register to default value */
	ext_src->mcde_extsrc_a0 = 0x0;
	ext_src->mcde_extsrc_a1 = 0x0;
	ext_src->mcde_extsrc_a2 = 0x0;
	ext_src->mcde_extsrc_conf = 0xA04;
	ext_src->mcde_extsrc_cr = 0x0;

	/** point to the correct overlay register */
	ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[gpar[chid]->mcde_cur_ovl_bmp]);

	/** reset overlay conf and overlay control register to default values...this also disables overlay */
	ovr_config->mcde_ovl_conf = 0x0;
	ovr_config->mcde_ovl_conf2 = 0x00200001;
	ovr_config->mcde_ovl_ljinc = 0x0;
	ovr_config->mcde_ovl_cr = 0x22B00000;

	return retVal;
}

#ifdef _cplusplus
}
#endif /* _cplusplus */

#else	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */
/* HW ED */

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

/** Linux include files:charachter driver and memory functions  */


#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/mcde_common.h>

extern struct mcdefb_info *gpar[];

#define PLATFORM_8500 1

mcde_error mcdesetdsiclk(dsi_link link, mcde_ch_id chid, mcde_dsi_clk_config clk_config)
{
    mcde_error    error = MCDE_OK;

    *gpar[chid]->mcde_clkdsi =
        (
            (*gpar[chid]->mcde_clkdsi &~MCDE_PLLOUT_DIVSEL1_MASK) |
            (((u32) clk_config.pllout_divsel1 << MCDE_PLLOUT_DIVSEL1_SHIFT) & MCDE_PLLOUT_DIVSEL1_MASK)
        );

    *gpar[chid]->mcde_clkdsi =
        (
            (*gpar[chid]->mcde_clkdsi &~MCDE_PLLOUT_DIVSEL0_MASK) |
            (((u32) clk_config.pllout_divsel0 << MCDE_PLLOUT_DIVSEL0_SHIFT) & MCDE_PLLOUT_DIVSEL0_MASK)
        );

    *gpar[chid]->mcde_clkdsi =
        (
            (*gpar[chid]->mcde_clkdsi &~MCDE_PLL4IN_SEL_MASK) |
            (((u32) clk_config.pll4in_sel << MCDE_PLL4IN_SEL_SHIFT) & MCDE_PLL4IN_SEL_MASK)
        );

    *gpar[chid]->mcde_clkdsi =
        (
            (*gpar[chid]->mcde_clkdsi &~MCDE_TXESCDIV_SEL_MASK) |
            (((u32) clk_config.pll4in_sel << MCDE_TXESCDIV_SEL_SHIFT) & MCDE_TXESCDIV_SEL_MASK)
        );

    *gpar[chid]->mcde_clkdsi =
        (
            (*gpar[chid]->mcde_clkdsi &~MCDE_TXESCDIV_MASK) |
            ((u32) clk_config.pll4in_sel & MCDE_TXESCDIV_MASK)
        );

    return(error);
}


mcde_error mcdesetdsicommandword
(
    dsi_link link,
    mcde_ch_id chid,
    mcde_dsi_channel   dsichannel,
    u8              cmdbyte_lsb,
    u8              cmdbyte_msb
)
{
    mcde_error    error = MCDE_OK;
    struct mcde_dsi_reg  *dsi_reg;

    if (MCDE_DSI_CH_CMD2 < (u32) dsichannel)
    {
         return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        dsi_reg = (struct mcde_dsi_reg *) (gpar[chid]->mcde_dsi_channel_reg[dsichannel]);
    }

    dsi_reg->mcde_dsi_cmd =
        (
            (dsi_reg->mcde_dsi_cmd &~MCDE_CMDBYTE_LSB_MASK) |
            ((u32) cmdbyte_lsb & MCDE_CMDBYTE_LSB_MASK)
        );

    dsi_reg->mcde_dsi_cmd =
        (
            (dsi_reg->mcde_dsi_cmd &~MCDE_CMDBYTE_MSB_MASK) |
            (((u32) cmdbyte_msb << MCDE_CMDBYTE_MSB_SHIFT) & MCDE_CMDBYTE_MSB_MASK)
        );

     return(error);
}

mcde_error mcdesetdsisyncpulse(dsi_link link, mcde_ch_id chid, mcde_dsi_channel dsichannel, u16 sync_dma, u16 sync_sw)
{
    mcde_error    error = MCDE_OK;
    struct mcde_dsi_reg  *dsi_reg;


    if (MCDE_DSI_CH_CMD2 < (u32) dsichannel)
    {
         return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        dsi_reg = (struct mcde_dsi_reg *) (gpar[chid]->mcde_dsi_channel_reg[dsichannel]);
    }

    dsi_reg->mcde_dsi_sync = ((dsi_reg->mcde_dsi_sync &~MCDE_DSI_DMA_MASK) | ((u32) sync_dma & MCDE_DSI_DMA_MASK));

    dsi_reg->mcde_dsi_sync =
        (
            (dsi_reg->mcde_dsi_sync &~MCDE_DSI_SW_MASK) |
            (((u32) sync_dma << MCDE_DSI_SW_SHIFT) & MCDE_DSI_SW_MASK)
        );


    return(error);
}

mcde_error mcdesetdsiconf(dsi_link link, mcde_ch_id chid, mcde_dsi_channel dsichannel, mcde_dsi_conf dsi_conf)
{
    mcde_error    error = MCDE_OK;
    struct mcde_dsi_reg  *dsi_reg;


    if (MCDE_DSI_CH_CMD2 < (u32) dsichannel)
    {
         return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        dsi_reg = (struct mcde_dsi_reg *) (gpar[chid]->mcde_dsi_channel_reg[dsichannel]);
    }

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_DSI_PACK_MASK) |
            (((u32) dsi_conf.packing << MCDE_DSI_PACK_SHIFT) & MCDE_DSI_PACK_MASK)
        );

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_DSI_DCSVID_MASK) |
            (((u32) dsi_conf.synchro << MCDE_DSI_DCSVID_SHIFT) & MCDE_DSI_DCSVID_MASK)
        );

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_DSI_BYTE_SWAP_MASK) |
            (((u32) dsi_conf.byte_swap << MCDE_DSI_BYTE_SWAP_SHIFT) & MCDE_DSI_BYTE_SWAP_MASK)
        );

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_DSI_BIT_SWAP_MASK) |
            (((u32) dsi_conf.bit_swap << MCDE_DSI_BIT_SWAP_SHIFT) & MCDE_DSI_BIT_SWAP_MASK)
        );

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_DSI_CMD8_MASK) |
            (((u32) dsi_conf.cmd8 << MCDE_DSI_CMD8_SHIFT) & MCDE_DSI_CMD8_MASK)
        );

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_DSI_VID_MODE_MASK) |
            (((u32) dsi_conf.vid_mode << MCDE_DSI_VID_MODE_SHIFT) & MCDE_DSI_VID_MODE_MASK)
        );

    dsi_reg->mcde_dsi_conf0 =
        (
            (dsi_reg->mcde_dsi_conf0 &~MCDE_BLANKING_MASK) |
            ((u32) dsi_conf.blanking & MCDE_BLANKING_MASK)
        );

    dsi_reg->mcde_dsi_frame =
        (
            (dsi_reg->mcde_dsi_frame &~MCDE_DSI_FRAME_MASK) |
            ((u32) dsi_conf.words_per_frame & MCDE_DSI_FRAME_MASK)
        );

    dsi_reg->mcde_dsi_pkt =
        (
            (dsi_reg->mcde_dsi_pkt &~MCDE_DSI_PACKET_MASK) |
            ((u32) dsi_conf.words_per_packet & MCDE_DSI_PACKET_MASK)
        );
    return(error);
}



/****************************************************************************/
/** NAME			:	mcdesetfifoctrl()			    */
/*--------------------------------------------------------------------------*/
/* DESCRIPTION	: 	This routine sets the formatter selection for output FIFOs*/
/*				                                                    		*/
/*                                                                          */
/* PARAMETERS	:                                                           */
/* 		IN  	:mcde_fifo_ctrl : FIFO selection control structure        */
/*     InOut    :None                                                       */
/* 		OUT 	:None                                                       */
/*                                                                          */
/* RETURN		:mcde_error	: MCDE error code						   	*/
/*               MCDE_OK                                                    */
/*               MCDE_INVALID_PARAMETER :if input argument is invalid       */
/*--------------------------------------------------------------------------*/
/* Type              :  PUBLIC                                              */
/* REENTRANCY 	     :	Non Re-entrant                                      */
/* REENTRANCY ISSUES :														*/

/****************************************************************************/
mcde_error mcdesetfifoctrl(dsi_link link, mcde_ch_id chid, struct mcde_fifo_ctrl fifo_ctrl)
{
    mcde_error    error = MCDE_OK;

    /*FIFO A Output Selection*/
    switch (chid)
    {
    case MCDE_CH_A:
	switch (fifo_ctrl.out_fifoa)
	{
        case MCDE_DPI_A:

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIA_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_DPIA_EN_SHIFT) & MCDE_CTRL_DPIA_EN_MASK)
                );
            break;

        case MCDE_DSI_VID0:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIA_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DPIA_EN_SHIFT) & MCDE_CTRL_DPIA_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_FABMUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_FABMUX_SHIFT) & MCDE_CTRL_FABMUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID0_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSIVID0_EN_SHIFT) & MCDE_DSIVID0_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_VID0] = MCDE_CH_A;

            break;

        case MCDE_DSI_VID1:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIA_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DPIA_EN_SHIFT) & MCDE_CTRL_DPIA_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_FABMUX_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_FABMUX_SHIFT) & MCDE_CTRL_FABMUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSIVID0_EN_SHIFT) & MCDE_DSIVID0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID1_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSIVID1_EN_SHIFT) & MCDE_DSIVID1_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_VID1] = MCDE_CH_A;
            break;

        case MCDE_DSI_CMD2:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIA_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DPIA_EN_SHIFT) & MCDE_CTRL_DPIA_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_FABMUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_FABMUX_SHIFT) & MCDE_CTRL_FABMUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSIVID0_EN_SHIFT) & MCDE_DSIVID0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSIVID1_EN_SHIFT) & MCDE_DSIVID1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD2_EN_MASK) |
                    ((u32) MCDE_SET_BIT & MCDE_DSICMD2_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD2] = MCDE_CH_A;
            break;

        default:
            error = MCDE_INVALID_PARAMETER;
	}
	break;
    case MCDE_CH_B:
	/*FIFO B Output Selection*/
	switch (fifo_ctrl.out_fifob)
	{
	case MCDE_DPI_B:
	   gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIB_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_DPIB_EN_SHIFT) & MCDE_CTRL_DPIB_EN_MASK)
                );
            break;

        case MCDE_DSI_VID0:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIB_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DPIB_EN_SHIFT) & MCDE_CTRL_DPIB_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_FABMUX_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_FABMUX_SHIFT) & MCDE_CTRL_FABMUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID0_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSIVID0_EN_SHIFT) & MCDE_DSIVID0_EN_MASK)
                );
            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_VID0] = MCDE_CH_B;
            break;

        case MCDE_DSI_VID1:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIB_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DPIB_EN_SHIFT) & MCDE_CTRL_DPIB_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_FABMUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_FABMUX_SHIFT) & MCDE_CTRL_FABMUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSIVID0_EN_SHIFT) & MCDE_DSIVID0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID1_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSIVID1_EN_SHIFT) & MCDE_DSIVID1_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_VID1] = MCDE_CH_B;
            break;

        case MCDE_DSI_CMD2:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DPIB_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DPIB_EN_SHIFT) & MCDE_CTRL_DPIB_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_FABMUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_FABMUX_SHIFT) & MCDE_CTRL_FABMUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSIVID0_EN_SHIFT) & MCDE_DSIVID0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSIVID1_EN_SHIFT) & MCDE_DSIVID1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD2_EN_MASK) |
                    ((u32) MCDE_SET_BIT & MCDE_DSICMD2_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD2] = MCDE_CH_B;

            break;

        default:
            error = MCDE_INVALID_PARAMETER;
	}
	break;
    case MCDE_CH_C0:
	/*FIFO 0 Output Selection*/
	switch (fifo_ctrl.out_fifo0)
	{
        case MCDE_DBI_C0:
           gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC0_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_DBIC0_EN_SHIFT) & MCDE_CTRL_DBIC0_EN_MASK)
                );
            break;

        case MCDE_DSI_CMD0:
           gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DBIC0_EN_SHIFT) & MCDE_CTRL_DBIC0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_F01MUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_F01MUX_SHIFT) & MCDE_CTRL_F01MUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD0_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSICMD0_EN_SHIFT) & MCDE_DSICMD0_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD0] = MCDE_CH_C0;

            break;

        case MCDE_DSI_CMD1:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DBIC0_EN_SHIFT) & MCDE_CTRL_DBIC0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_F01MUX_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_F01MUX_SHIFT) & MCDE_CTRL_F01MUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSICMD0_EN_SHIFT) & MCDE_DSICMD0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD1_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSICMD1_EN_SHIFT) & MCDE_DSICMD1_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD1] = MCDE_CH_C0;
            break;

        case MCDE_DSI_VID2:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DBIC0_EN_SHIFT) & MCDE_CTRL_DBIC0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_F01MUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_F01MUX_SHIFT) & MCDE_CTRL_F01MUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSICMD0_EN_SHIFT) & MCDE_DSICMD0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSICMD1_EN_SHIFT) & MCDE_DSICMD1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID2_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSIVID2_EN_SHIFT) & MCDE_DSIVID2_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_VID2] = MCDE_CH_C0;

            break;

        default:
            error = MCDE_INVALID_PARAMETER;
	}
	break;
    case MCDE_CH_C1:
	/*FIFO 1 Output Selection*/
	switch (fifo_ctrl.out_fifo1)
	{
        case MCDE_DBI_C1:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC1_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_DBIC1_EN_SHIFT) & MCDE_CTRL_DBIC1_EN_MASK)
                );
            break;

        case MCDE_DSI_CMD0:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DBIC1_EN_SHIFT) & MCDE_CTRL_DBIC1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_F01MUX_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_F01MUX_SHIFT) & MCDE_CTRL_F01MUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD0_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSICMD0_EN_SHIFT) & MCDE_DSICMD0_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD0] = MCDE_CH_C1;

            break;

        case MCDE_DSI_CMD1:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DBIC1_EN_SHIFT) & MCDE_CTRL_DBIC1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_F01MUX_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_F01MUX_SHIFT) & MCDE_CTRL_F01MUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSICMD0_EN_SHIFT) & MCDE_DSICMD0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD1_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSICMD1_EN_SHIFT) & MCDE_DSICMD1_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD1] = MCDE_CH_C1;

            break;

        case MCDE_DSI_CMD2:
            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_DBIC1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_CTRL_DBIC1_EN_SHIFT) & MCDE_CTRL_DBIC1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_F01MUX_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_CTRL_F01MUX_SHIFT) & MCDE_CTRL_F01MUX_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD0_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSICMD0_EN_SHIFT) & MCDE_DSICMD0_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSICMD1_EN_MASK) |
                    (((u32) MCDE_CLEAR_BIT << MCDE_DSICMD1_EN_SHIFT) & MCDE_DSICMD1_EN_MASK)
                );

            gpar[chid]->regbase->mcde_cr =
                (
                    (gpar[chid]->regbase->mcde_cr &~MCDE_DSIVID2_EN_MASK) |
                    (((u32) MCDE_SET_BIT << MCDE_DSIVID2_EN_SHIFT) & MCDE_DSIVID2_EN_MASK)
                );

            gpar[chid]->dsi_formatter_plugged_channel[MCDE_DSI_CH_CMD2] = MCDE_CH_C1;

            break;

        default:
            error = MCDE_INVALID_PARAMETER;
	}
	break;
    default:
	error = MCDE_INVALID_PARAMETER;
    }
    return(error);
}
#ifdef PLATFORM_8500
mcde_error mcdesetoutputconf(dsi_link link, mcde_ch_id chid, mcde_output_conf output_conf)
{
    mcde_error    error = MCDE_OK;

 //  if (chid == CHANNEL_C0)
   //	gpar[chid]->regbase->mcde_cfg0 = 0x5e145001; /** has to be removed, Added for testing */

    switch (output_conf)
    {
        case MCDE_CONF_TVA_DPIC0_LCDB:
            gpar[chid]->regbase->mcde_cfg0 =
                (
                    (gpar[chid]->regbase->mcde_cfg0 &~MCDE_SYNCMUX_MASK) |
                    ((u32) MCDE_TVA_DPIC0_LCDB_MASK & MCDE_SYNCMUX_MASK)
                );
            break;

        case MCDE_CONF_TVB_DPIC1_LCDA:
            gpar[chid]->regbase->mcde_cfg0 =
                (
                    (gpar[chid]->regbase->mcde_cfg0 &~MCDE_SYNCMUX_MASK) |
                    ((u32) MCDE_TVB_DPIC1_LCDA_MASK & MCDE_SYNCMUX_MASK)
                );
            break;

        case MCDE_CONF_DPIC1_LCDA:
            gpar[chid]->regbase->mcde_cfg0 =
                (
                    (gpar[chid]->regbase->mcde_cfg0 &~MCDE_SYNCMUX_MASK) |
                    ((u32) MCDE_DPIC1_LCDA_MASK & MCDE_SYNCMUX_MASK)
                );
            break;

        case MCDE_CONF_DPIC0_LCDB:
            gpar[chid]->regbase->mcde_cfg0 =
                (
                    (gpar[chid]->regbase->mcde_cfg0 &~MCDE_SYNCMUX_MASK) |
                    ((u32) MCDE_DPIC0_LCDB_MASK & MCDE_SYNCMUX_MASK)
                );
            break;

        case MCDE_CONF_LCDA_LCDB:
            gpar[chid]->regbase->mcde_cfg0 =
                (
                    (gpar[chid]->regbase->mcde_cfg0 &~MCDE_SYNCMUX_MASK) |
                    ((u32) MCDE_LCDA_LCDB_MASK & MCDE_SYNCMUX_MASK)
                );
            break;
        case MCDE_CONF_DSI:
            gpar[chid]->regbase->mcde_cfg0 =
                (
                    (gpar[chid]->regbase->mcde_cfg0 &~MCDE_SYNCMUX_MASK) |
                    ((u32) MCDE_DSI_MASK & MCDE_SYNCMUX_MASK)
                );
            break;
        default:
            error = MCDE_INVALID_PARAMETER;
    }

    return(error);
}
#endif
/****************************************************************************/
/* NAME			:	mcdesetbufferaddr()								*/
/*--------------------------------------------------------------------------*/
/* DESCRIPTION	: 	This API is used to set the base address of the buffer. */
/*				                                                    		*/
/*                                                                          */
/* PARAMETERS	:                                                           */
/* 		IN  	:mcde_ext_src src_id: External source id number to be     */
/*                                      configured                          */
/*               mcde_buffer_id buffer_id : Buffer id whose address is    */
/*                                              to be configured            */
/*               uint32 address   : Address of the buffer                 */
/*     InOut    :None                                                       */
/* 		OUT 	:None                                                	    */
/*                                                                          */
/* RETURN		:mcde_error	: MCDE error code						   	*/
/*               MCDE_OK                                                    */
/*               MCDE_INVALID_PARAMETER :if input argument is invalid       */
/*--------------------------------------------------------------------------*/
/* Type              :  PUBLIC                                              */
/* REENTRANCY 	     :	Non Re-entrant                                      */
/* REENTRANCY ISSUES :														*/

/****************************************************************************/
mcde_error mcdesetbufferaddr
(
    mcde_ch_id chid,
    mcde_ext_src   src_id,
    mcde_buffer_id buffer_id,
    u32         address
)
{
    mcde_error        error = MCDE_OK;
    struct mcde_ext_src_reg  *ext_src;

    ext_src = (struct mcde_ext_src_reg *) (gpar[chid]->extsrc_regbase[src_id]);

    switch (buffer_id)
    {
        case MCDE_BUFFER_ID_0:
            ext_src->mcde_extsrc_a0 =
                (
                    (ext_src->mcde_extsrc_a0 &~MCDE_EXT_BUFFER_MASK) |
                    ((address << MCDE_EXT_BUFFER_SHIFT) & MCDE_EXT_BUFFER_MASK)
                );

            break;

        case MCDE_BUFFER_ID_1:
            ext_src->mcde_extsrc_a1 =
                (
                    (ext_src->mcde_extsrc_a1 &~MCDE_EXT_BUFFER_MASK) |
                    ((address << MCDE_EXT_BUFFER_SHIFT) & MCDE_EXT_BUFFER_MASK)
                );

            break;

        case MCDE_BUFFER_ID_2:
            ext_src->mcde_extsrc_a2 =
                (
                    (ext_src->mcde_extsrc_a2 &~MCDE_EXT_BUFFER_MASK) |
                    ((address << MCDE_EXT_BUFFER_SHIFT) & MCDE_EXT_BUFFER_MASK)
                );

            break;

        default:
            error = MCDE_INVALID_PARAMETER;
    }

    ext_src->mcde_extsrc_conf =
    (
        (ext_src->mcde_extsrc_conf &~MCDE_EXT_BUFFER_ID_MASK) |
        ((u32) buffer_id & MCDE_EXT_BUFFER_ID_MASK)
     );

    return(error);
}

mcde_error mcdesetextsrcconf(mcde_ch_id chid, mcde_ext_src src_id, struct mcde_ext_conf config)
{
    mcde_error        error = MCDE_OK;
    struct mcde_ext_src_reg  *ext_src;

    ext_src = (struct mcde_ext_src_reg *) (gpar[chid]->extsrc_regbase[src_id]);


    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BEPO_MASK) |
            (((u32) config.ovr_pxlorder << MCDE_EXT_BEPO_SHIFT) & MCDE_EXT_BEPO_MASK)
        );

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BEBO_MASK) |
            (((u32) config.endianity << MCDE_EXT_BEBO_SHIFT) & MCDE_EXT_BEBO_MASK)
        );

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BGR_MASK) |
            (((u32) config.rgb_format << MCDE_EXT_BGR_SHIFT) & MCDE_EXT_BGR_MASK)
        );

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BPP_MASK) |
            (((u32) config.bpp << MCDE_EXT_BPP_SHIFT) & MCDE_EXT_BPP_MASK)
        );

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_PRI_OVR_MASK) |
            (((u32) config.provr_id << MCDE_EXT_PRI_OVR_SHIFT) & MCDE_EXT_PRI_OVR_MASK)
        );

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BUFFER_NUM_MASK) |
            (((u32) config.buf_num << MCDE_EXT_BUFFER_NUM_SHIFT) & MCDE_EXT_BUFFER_NUM_MASK)
        );

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BUFFER_ID_MASK) |
            ((u32) config.buf_id & MCDE_EXT_BUFFER_ID_MASK)
        );

    return(error);
}

mcde_error mcdesetextsrcctrl(mcde_ch_id chid, mcde_ext_src src_id, struct mcde_ext_src_ctrl control)
{
    mcde_error        error = MCDE_OK;
    struct mcde_ext_src_reg  *ext_src;

    ext_src = (struct mcde_ext_src_reg *) (gpar[chid]->extsrc_regbase[src_id]);

    ext_src->mcde_extsrc_cr =
        (
            (ext_src->mcde_extsrc_cr &~MCDE_EXT_FORCEFSDIV_MASK) |
            (((u32) control.fs_div << MCDE_EXT_FORCEFSDIV_SHIFT) & MCDE_EXT_FORCEFSDIV_MASK)
        );

    ext_src->mcde_extsrc_cr =
        (
            (ext_src->mcde_extsrc_cr &~MCDE_EXT_FSDISABLE_MASK) |
            (((u32) control.fs_ctrl << MCDE_EXT_FSDISABLE_SHIFT) & MCDE_EXT_FSDISABLE_MASK)
        );

    ext_src->mcde_extsrc_cr =
        (
            (ext_src->mcde_extsrc_cr &~MCDE_EXT_OVR_CTRL_MASK) |
            (((u32) control.ovr_ctrl << MCDE_EXT_OVR_CTRL_SHIFT) & MCDE_EXT_OVR_CTRL_MASK)
        );

    ext_src->mcde_extsrc_cr =
        (
            (ext_src->mcde_extsrc_cr &~MCDE_EXT_BUF_MODE_MASK) |
            ((u32) control.sel_mode & MCDE_EXT_BUF_MODE_MASK)
        );

      return(error);
}
mcde_error mcdesetbufid(mcde_ch_id chid, mcde_ext_src src_id, mcde_buffer_id buffer_id, mcde_num_buffer_used buffer_num)
{
    mcde_error        error = MCDE_OK;
    struct mcde_ext_src_reg  *ext_src;

    ext_src = (struct mcde_ext_src_reg *) (gpar[chid]->extsrc_regbase[src_id]);

    if ((int) buffer_id > 2)
    {
	return(MCDE_INVALID_PARAMETER);
    }

    ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BUFFER_ID_MASK) |
            ((u32) buffer_id & MCDE_EXT_BUFFER_ID_MASK)
        );

	ext_src->mcde_extsrc_conf =
        (
            (ext_src->mcde_extsrc_conf &~MCDE_EXT_BUFFER_NUM_MASK) |
            (((u32) buffer_num << MCDE_EXT_BUFFER_NUM_SHIFT) & MCDE_EXT_BUFFER_NUM_MASK)
        );


    return(error);
}
mcde_error mcdesetcolorconvctrl(mcde_ch_id chid, mcde_overlay_id overlay, mcde_col_conv_ctrl  col_ctrl)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);
    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_COLCTRL_MASK) |
            (((u32) col_ctrl << MCDE_OVR_COLCTRL_SHIFT) & MCDE_OVR_COLCTRL_MASK)
        );
    return error;
}
mcde_error mcdesetovrctrl(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_control ovr_cr)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_OVLEN_MASK) |
            ((u32) ovr_cr.ovr_state & MCDE_OVR_OVLEN_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_COLCTRL_MASK) |
            (((u32) ovr_cr.col_ctrl << MCDE_OVR_COLCTRL_SHIFT) & MCDE_OVR_COLCTRL_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_PALCTRL_MASK) |
            (((u32) ovr_cr.pal_control << MCDE_OVR_PALCTRL_SHIFT) & MCDE_OVR_PALCTRL_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_CKEYEN_MASK) |
            (((u32) ovr_cr.color_key << MCDE_OVR_CKEYEN_SHIFT) & MCDE_OVR_CKEYEN_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_STBPRIO_MASK) |
            (((u32) ovr_cr.priority << MCDE_OVR_STBPRIO_SHIFT) & MCDE_OVR_STBPRIO_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_BURSTSZ_MASK) |
            (((u32) ovr_cr.burst_req << MCDE_OVR_BURSTSZ_SHIFT) & MCDE_OVR_BURSTSZ_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_MAXREQ_MASK) |
            (((u32) ovr_cr.outstnd_req << MCDE_OVR_MAXREQ_SHIFT) & MCDE_OVR_MAXREQ_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_ROTBURSTSIZE_MASK) |
            (((u32) ovr_cr.rot_burst_req << MCDE_OVR_ROTBURSTSIZE_SHIFT) & MCDE_OVR_ROTBURSTSIZE_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_ALPHAPMEN_MASK) |
            (((u32) ovr_cr.alpha << MCDE_OVR_ALPHAPMEN_SHIFT) & MCDE_OVR_ALPHAPMEN_MASK)
        );

    ovr_config->mcde_ovl_cr =
        (
            (ovr_config->mcde_ovl_cr &~MCDE_OVR_CLIPEN_MASK) |
            (((u32) ovr_cr.clip << MCDE_OVR_CLIPEN_SHIFT) & MCDE_OVR_CLIPEN_MASK)
        );


    return(error);
}

mcde_error mcdesetovrlayconf(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_config ovr_conf)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_conf =
        (
            (ovr_config->mcde_ovl_conf &~MCDE_OVR_LPF_MASK) |
            (((u32) ovr_conf.line_per_frame << MCDE_OVR_LPF_SHIFT) & MCDE_OVR_LPF_MASK)
        );

    ovr_config->mcde_ovl_conf =
        (
            (ovr_config->mcde_ovl_conf &~MCDE_EXT_SRCID_MASK) |
            (((u32) ovr_conf.src_id << MCDE_EXT_SRCID_SHIFT) & MCDE_EXT_SRCID_MASK)
        );

    ovr_config->mcde_ovl_conf =
        (
            (ovr_config->mcde_ovl_conf &~MCDE_OVR_PPL_MASK) |
            ((u32) ovr_conf.ovr_ppl & MCDE_OVR_PPL_MASK)
        );

    return(error);
}

mcde_error mcdesetovrconf2(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_conf2 ovr_conf2)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);


    ovr_config->mcde_ovl_conf2 =
        (
            (ovr_config->mcde_ovl_conf2 &~MCDE_WATERMARK_MASK) |
            (((u32) ovr_conf2.watermark_level << MCDE_WATERMARK_SHIFT) & MCDE_WATERMARK_MASK)
        );

    ovr_config->mcde_ovl_conf2 =
        (
            (ovr_config->mcde_ovl_conf2 &~MCDE_OVR_OPQ_MASK) |
            (((u32) ovr_conf2.ovr_opaq << MCDE_OVR_OPQ_SHIFT) & MCDE_OVR_OPQ_MASK)
        );

    ovr_config->mcde_ovl_conf2 =
        (
            (ovr_config->mcde_ovl_conf2 &~MCDE_ALPHAVALUE_MASK) |
            (((u32) ovr_conf2.alpha_value << MCDE_ALPHAVALUE_SHIFT) & MCDE_ALPHAVALUE_MASK)
        );

    ovr_config->mcde_ovl_conf2 =
        (
            (ovr_config->mcde_ovl_conf2 &~MCDE_PIXOFF_MASK) |
            (((u32) ovr_conf2.pixoff << MCDE_PIXOFF_SHIFT) & MCDE_PIXOFF_MASK)
        );

    ovr_config->mcde_ovl_conf2 =
        (
            (ovr_config->mcde_ovl_conf2 &~MCDE_OVR_BLEND_MASK) |
            ((u32) ovr_conf2.ovr_blend & MCDE_OVR_BLEND_MASK)
        );

    return(error);
}
mcde_error mcdesetovrljinc(mcde_ch_id chid, mcde_overlay_id overlay, u32 ovr_ljinc)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_ljinc = ((ovr_ljinc << MCDE_LINEINCREMENT_SHIFT) & MCDE_LINEINCREMENT_MASK);

    return(error);
}
#ifdef PLATFORM_8500
mcde_error mcdesettopleftmargincrop(mcde_ch_id chid, mcde_overlay_id overlay, u32 ovr_topmargin, u16 ovr_leftmargin)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);


    ovr_config->mcde_ovl_crop =
        (
            (ovr_config->mcde_ovl_crop &~MCDE_YCLIP_MASK) |
            ((ovr_topmargin << MCDE_YCLIP_SHIFT) & MCDE_YCLIP_MASK)
        );


	ovr_config->mcde_ovl_crop =
			(
				(ovr_config->mcde_ovl_crop &~MCDE_XCLIP_MASK) |
				(((u32) ovr_leftmargin << MCDE_XCLIP_SHIFT) & MCDE_XCLIP_MASK)
			);


    return(error);
}
#endif
mcde_error mcdesetovrcomp(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_comp ovr_comp)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_ZLEVEL_MASK) |
            (((u32) ovr_comp.ovr_zlevel << MCDE_OVR_ZLEVEL_SHIFT) & MCDE_OVR_ZLEVEL_MASK)
        );

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_YPOS_MASK) |
            (((u32) ovr_comp.ovr_ypos << MCDE_OVR_YPOS_SHIFT) & MCDE_OVR_YPOS_MASK)
        );

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_CHID_MASK) |
            (((u32) ovr_comp.ch_id << MCDE_OVR_CHID_SHIFT) & MCDE_OVR_CHID_MASK)
        );

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_XPOS_MASK) |
            ((u32) ovr_comp.ovr_xpos & MCDE_OVR_XPOS_MASK)
        );

    return(error);
}
#ifdef PLATFORM_8500
mcde_error mcdesetovrclip(mcde_ch_id chid, mcde_overlay_id overlay, struct mcde_ovr_clip ovr_clip)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_bot_rht_clip=
        (
            (ovr_config->mcde_ovl_bot_rht_clip&~MCDE_YBRCOOR_MASK) |
            (((u32) ovr_clip.ybrcoor << MCDE_YBRCOOR_SHIFT) & MCDE_YBRCOOR_MASK)
        );

    ovr_config->mcde_ovl_top_left_clip=
        (
            (ovr_config->mcde_ovl_top_left_clip &~MCDE_YBRCOOR_MASK) |
            (((u32) ovr_clip.ytlcoor << MCDE_YBRCOOR_SHIFT) & MCDE_YBRCOOR_MASK)
        );

    ovr_config->mcde_ovl_bot_rht_clip =
        (
            (ovr_config->mcde_ovl_bot_rht_clip &~MCDE_XBRCOOR_MASK) |
            ((u32) ovr_clip.xbrcoor & MCDE_XBRCOOR_MASK)
        );

    ovr_config->mcde_ovl_top_left_clip =
        (
            (ovr_config->mcde_ovl_top_left_clip &~MCDE_XBRCOOR_MASK) |
            ((u32) ovr_clip.xtlcoor & MCDE_XBRCOOR_MASK)
        );

    return(error);
}
#endif
mcde_error mcdesetovrstate(mcde_ch_id chid, mcde_overlay_id overlay, mcde_overlay_ctrl state)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_cr = ((ovr_config->mcde_ovl_cr &~MCDE_OVR_OVLEN_MASK) | (state & MCDE_OVR_OVLEN_MASK));

    return(error);
}

mcde_error mcdesetovrpplandlpf(mcde_ch_id chid, mcde_overlay_id overlay, u16 ppl, u16 lpf)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_conf =
        (
            (ovr_config->mcde_ovl_conf &~MCDE_OVR_PPL_MASK) |
            ((u32) ppl & MCDE_OVR_PPL_MASK)
        );
	ovr_config->mcde_ovl_conf =
		 (
			 (ovr_config->mcde_ovl_conf &~MCDE_OVR_LPF_MASK) |
			 (((u32) lpf << MCDE_OVR_LPF_SHIFT) & MCDE_OVR_LPF_MASK)
		 );


    return(error);
}

mcde_error mcdeovraassociatechnl(mcde_ch_id chid, mcde_overlay_id overlay, mcde_ch_id ch_id)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_CHID_MASK) |
            (((u32) ch_id << MCDE_OVR_CHID_SHIFT) & MCDE_OVR_CHID_MASK)
        );

    return(error);
}


mcde_error mcdesetovrXYZpos(mcde_ch_id chid, mcde_overlay_id overlay, mcde_ovr_xy xy_pos, u8 z_pos)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_YPOS_MASK) |
            (((u32) xy_pos.ovr_ypos << MCDE_OVR_YPOS_SHIFT) & MCDE_OVR_YPOS_MASK)
        );

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_XPOS_MASK) |
            ((u32) xy_pos.ovr_xpos & MCDE_OVR_XPOS_MASK)
        );

    ovr_config->mcde_ovl_comp =
        (
            (ovr_config->mcde_ovl_comp &~MCDE_OVR_ZLEVEL_MASK) |
            (((u32) z_pos << MCDE_OVR_ZLEVEL_SHIFT) & MCDE_OVR_ZLEVEL_MASK)
        );

    return(error);
}

mcde_error mcdeovrassociateextsrc(mcde_ch_id chid, mcde_overlay_id overlay, mcde_ext_src ext_src)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ovl_reg  *ovr_config;

    ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[overlay]);

    ovr_config->mcde_ovl_conf =
        (
            (ovr_config->mcde_ovl_conf &~MCDE_EXT_SRCID_MASK) |
            (((u32) ext_src << MCDE_EXT_SRCID_SHIFT) & MCDE_EXT_SRCID_MASK)
        );

    return(error);
}

mcde_error mcdesetchnlXconf(mcde_ch_id chid, u16 channelnum, struct mcde_chconfig config)
{
    mcde_error        mcde_error = MCDE_OK;
    struct mcde_ch_synch_reg *ch_syncreg;

    ch_syncreg = (struct mcde_ch_synch_reg *) (gpar[chid]->ch_regbase1[channelnum]);

    ch_syncreg->mcde_ch_conf =
        (
            (ch_syncreg->mcde_ch_conf &~MCDE_CHXLPF_MASK) |
            (((u32) config.lpf << MCDE_CHXLPF_SHIFT) & MCDE_CHXLPF_MASK)
        );

    ch_syncreg->mcde_ch_conf =
        (
            (ch_syncreg->mcde_ch_conf &~MCDE_CHXPPL_MASK) |
            ((u32) config.ppl & MCDE_CHXPPL_MASK)
        );


    return(mcde_error);
}

mcde_error mcdesetchnlsyncsrc(mcde_ch_id chid, u16 channelnum, struct mcde_chsyncmod sync_mod)
{
    mcde_error        mcde_error = MCDE_OK;
    struct mcde_ch_synch_reg *ch_syncreg;

    ch_syncreg = (struct mcde_ch_synch_reg *) (gpar[chid]->ch_regbase1[channelnum]);

    ch_syncreg->mcde_chsyn_mod =
        (
            (ch_syncreg->mcde_chsyn_mod &~MCDE_OUTINTERFACE_MASK) |
            (((u32) sync_mod.out_synch_interface << MCDE_OUTINTERFACE_SHIFT) & MCDE_OUTINTERFACE_MASK)
        );

    ch_syncreg->mcde_chsyn_mod =
        (
            (ch_syncreg->mcde_chsyn_mod &~MCDE_SRCSYNCH_MASK) |
            ((u32) sync_mod.ch_synch_src & MCDE_SRCSYNCH_MASK)
        );


    return(mcde_error);
}
mcde_error mcdesetchnlsyncevent(mcde_ch_id chid, struct mcde_ch_conf ch_config)
{
    mcde_error        mcde_error = MCDE_OK;
#ifdef PLATFORM_8500
    struct mcde_ch_reg   *ch_x_reg;
#else
    struct mcde_ch_synch_reg *ch_x_reg;
#endif

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
#ifdef PLATFORM_8500
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
#else
	ch_x_reg = (struct mcde_ch_synch_reg *) (gpar[chid]->ch_regbase1[gpar[chid]->mcde_cur_ovl_bmp]);
#endif

    }

    ch_x_reg->mcde_chsyn_con = ((ch_x_reg->mcde_chsyn_con & ~MCDE_SWINTVCNT_MASK) | (((u32)ch_config.swint_vcnt << MCDE_SWINTVCNT_SHIFT) & MCDE_SWINTVCNT_MASK));
    /**set to active video if you want to receive VSYNC interrupts*/
    ch_x_reg->mcde_chsyn_con = ((ch_x_reg->mcde_chsyn_con & ~MCDE_SWINTVEVENT_MASK) | (((u32)ch_config.swint_vevent << MCDE_SWINTVEVENT_SHIFT) & MCDE_SWINTVEVENT_MASK));
    ch_x_reg->mcde_chsyn_con = ((ch_x_reg->mcde_chsyn_con & ~MCDE_HWREQVCNT_MASK) | (((u32)ch_config.hwreq_vcnt << MCDE_HWREQVCNT_SHIFT) & MCDE_HWREQVCNT_MASK));
    ch_x_reg->mcde_chsyn_con = ((ch_x_reg->mcde_chsyn_con & ~MCDE_HWREQVEVENT_MASK) | ((u32)ch_config.hwreq_vevent & MCDE_HWREQVEVENT_MASK));

    return(mcde_error);
}
mcde_error mcdesetswsync(mcde_ch_id chid, u16 channelnum, mcde_sw_trigger sw_trig)
{
    mcde_error        error = MCDE_OK;
    struct mcde_ch_synch_reg *ch_syncreg;

    ch_syncreg = (struct mcde_ch_synch_reg *) (gpar[chid]->ch_regbase1[channelnum]);

    ch_syncreg->mcde_chsyn_sw =
        (
            (ch_syncreg->mcde_chsyn_sw &~MCDE_SW_TRIG_MASK) |
            ((u32) sw_trig & MCDE_SW_TRIG_MASK)
        );

    return(error);
}

mcde_error mcdesetchnlbckgndcol(mcde_ch_id chid, u16 channelnum, struct mcde_ch_bckgrnd_col color)
{
    mcde_error        error = MCDE_OK;
    struct mcde_ch_synch_reg *ch_syncreg;

    ch_syncreg = (struct mcde_ch_synch_reg *) (gpar[chid]->ch_regbase1[channelnum]);

    ch_syncreg->mcde_chsyn_bck =
        (
            (ch_syncreg->mcde_chsyn_bck &~MCDE_REDCOLOR_MASK) |
            ((color.red << MCDE_REDCOLOR_SHIFT) & MCDE_REDCOLOR_MASK)
        );

    ch_syncreg->mcde_chsyn_bck =
        (
            (ch_syncreg->mcde_chsyn_bck &~MCDE_GREENCOLOR_MASK) |
            ((color.green << MCDE_GREENCOLOR_SHIFT) & MCDE_GREENCOLOR_MASK)
        );

    ch_syncreg->mcde_chsyn_bck =
        (
            (ch_syncreg->mcde_chsyn_bck &~MCDE_BLUECOLOR_MASK) |
            (color.blue & MCDE_BLUECOLOR_MASK)
        );


    return(error);
}

mcde_error mcdesetchnlsyncprio(mcde_ch_id chid, u16 channelnum, u32 priority)
{
    mcde_error        error = MCDE_OK;
    struct mcde_ch_synch_reg *ch_syncreg;

    ch_syncreg = (struct mcde_ch_synch_reg *) (gpar[chid]->ch_regbase1[channelnum]);

    ch_syncreg->mcde_chsyn_prio =
        (
            (ch_syncreg->mcde_chsyn_prio &~MCDE_CHPRIORITY_MASK) |
            (priority & MCDE_CHPRIORITY_MASK)
        );

    return(error);
}

mcde_error mcdesetoutdevicelpfandppl(mcde_ch_id chid, u16 channelnum, u16 lpf, u16 ppl)
{
    mcde_error        error = MCDE_OK;
    struct mcde_ch_synch_reg *ch_syncreg;

    ch_syncreg = (struct mcde_ch_synch_reg *) (gpar[chid]->ch_regbase1[channelnum]);

    ch_syncreg->mcde_ch_conf =
        (
            (ch_syncreg->mcde_ch_conf &~MCDE_CHXLPF_MASK) |
            ((lpf << MCDE_CHXLPF_SHIFT) & MCDE_CHXLPF_MASK)
        );

    ch_syncreg->mcde_ch_conf = ((ch_syncreg->mcde_ch_conf &~MCDE_CHXPPL_MASK) | (ppl & MCDE_CHXLPF_MASK));


    return(error);
}

mcde_error mcdesetchnlCconf(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_chc_config config)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_RES1_MASK) |
                    (((u32) config.res_pol << MCDE_RES1_SHIFT) & MCDE_RES1_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_RD1_MASK) |
                    (((u32) config.rd_pol << MCDE_RD1_SHIFT) & MCDE_RD1_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_WR1_MASK) |
                    (((u32) config.wr_pol << MCDE_WR1_SHIFT) & MCDE_WR1_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_CD1_MASK) |
                    (((u32) config.cd_pol << MCDE_CD1_SHIFT) & MCDE_CD1_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_CS1_MASK) |
                    (((u32) config.cs_pol << MCDE_CS1_SHIFT) & MCDE_CS1_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_CS1EN_MASK) |
                    (((u32) config.csen << MCDE_CS1EN_SHIFT) & MCDE_CS1EN_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_INBAND1_MASK) |
                    (((u32) config.inband_mode << MCDE_INBAND1_SHIFT) & MCDE_INBAND1_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_BUSSIZE1_MASK) |
                    (((u32) config.bus_size << MCDE_BUSSIZE1_SHIFT) & MCDE_BUSSIZE1_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_SYNCEN1_MASK) |
                    (((u32) config.syncen << MCDE_SYNCEN1_SHIFT) & MCDE_SYNCEN1_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_WMLVL1_MASK) |
                    (((u32) config.fifo_watermark << MCDE_WMLVL1_SHIFT) & MCDE_WMLVL1_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_C1EN_MASK) |
                    (((u32) config.chcen << MCDE_C1EN_SHIFT) & MCDE_C1EN_MASK)
                );

            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_RES2_MASK) |
                    (((u32) config.res_pol << MCDE_RES2_SHIFT) & MCDE_RES2_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_RD2_MASK) |
                    (((u32) config.rd_pol << MCDE_RD2_SHIFT) & MCDE_RD2_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_WR2_MASK) |
                    (((u32) config.wr_pol << MCDE_WR2_SHIFT) & MCDE_WR2_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_CD2_MASK) |
                    (((u32) config.cd_pol << MCDE_CD2_SHIFT) & MCDE_CD2_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_CS2_MASK) |
                    (((u32) config.cs_pol << MCDE_CS2_SHIFT) & MCDE_CS2_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_CS2EN_MASK) |
                    (((u32) config.csen << MCDE_CS2EN_SHIFT) & MCDE_CS2EN_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_INBAND2_MASK) |
                    (((u32) config.inband_mode << MCDE_INBAND2_SHIFT) & MCDE_INBAND2_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_BUSSIZE2_MASK) |
                    (((u32) config.bus_size << MCDE_BUSSIZE2_SHIFT) & MCDE_BUSSIZE2_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_SYNCEN2_MASK) |
                    (((u32) config.syncen << MCDE_SYNCEN2_SHIFT) & MCDE_SYNCEN2_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_WMLVL2_MASK) |
                    (((u32) config.fifo_watermark << MCDE_WMLVL2_SHIFT) & MCDE_WMLVL2_MASK)
                );

            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_C2EN_MASK) |
                    (((u32) config.chcen << MCDE_C2EN_SHIFT) & MCDE_C2EN_MASK)
                );

            break;

        default:
           return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdesetchnlCctrl(mcde_ch_id chid, struct mcde_chc_ctrl control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    ch_c_reg->mcde_chc_crc =
        (
            (ch_c_reg->mcde_chc_crc &~MCDE_SYNCCTRL_MASK) |
            (((u32) control.sync << MCDE_SYNCCTRL_SHIFT) & MCDE_SYNCCTRL_MASK)
        );

    ch_c_reg->mcde_chc_crc =
        (
            (ch_c_reg->mcde_chc_crc &~MCDE_RESEN_MASK) |
            (((u32) control.resen << MCDE_RESEN_SHIFT) & MCDE_RESEN_MASK)
        );

    ch_c_reg->mcde_chc_crc =
        (
            (ch_c_reg->mcde_chc_crc &~MCDE_CLKSEL_MASK) |
            (((u32) control.clksel << MCDE_CLKSEL_SHIFT) & MCDE_CLKSEL_MASK)
        );

    ch_c_reg->mcde_chc_crc =
        (
            (ch_c_reg->mcde_chc_crc &~MCDE_SYNCSEL_MASK) |
            (((u32) control.synsel << MCDE_SYNCSEL_SHIFT) & MCDE_SYNCSEL_MASK)
        );

    return(error);
}
mcde_error mcdesetchnlXpowermode(mcde_ch_id chid, mcde_powen_select power)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;
	struct mcde_ch_reg   *ch_x_reg;

    if (chid <= MCDE_CH_B)
    {
	    ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
		ch_x_reg->mcde_ch_cr0 =
			(
				(ch_x_reg->mcde_ch_cr0 &~MCDE_POWEREN_MASK) |
				((power << MCDE_POWEREN_SHIFT) & MCDE_POWEREN_MASK)
			);

    }else
	{

	  ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

	  ch_c_reg->mcde_chc_crc =
		  (
			  (ch_c_reg->mcde_chc_crc &~MCDE_POWEREN_MASK) |
			  (((u32) power << MCDE_POWEREN_SHIFT) & MCDE_POWEREN_MASK)
		  );

	}


    return(error);
}

mcde_error mcdesetchnlXflowmode(mcde_ch_id chid, mcde_flow_select flow)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;
	struct mcde_ch_reg   *ch_x_reg;

	if (chid <= MCDE_CH_B)
    {
		ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
		ch_x_reg->mcde_ch_cr0 = ((ch_x_reg->mcde_ch_cr0 &~MCDE_FLOEN_MASK) | (flow & MCDE_FLOEN_MASK));

	} else
	{
	    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

	ch_c_reg->mcde_chc_crc = ((ch_c_reg->mcde_chc_crc &~MCDE_FLOEN_MASK) | ((u32) flow & MCDE_FLOEN_MASK));
	}

    return(error);
}

mcde_error mcdeconfPBCunit(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_pbc_config config)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_pbcrc0 =
                (
                    (ch_c_reg->mcde_chc_pbcrc0 &~MCDE_PDCTRL_SHIFT) |
                    (((u32) config.duplex_mode << MCDE_PDCTRL_SHIFT) & MCDE_PDCTRL_SHIFT)
                );

            ch_c_reg->mcde_chc_pbcrc0 =
                (
                    (ch_c_reg->mcde_chc_pbcrc0 &~MCDE_DUPLEXER_MASK) |
                    (((u32) config.duplex_mode << MCDE_DUPLEXER_SHIFT) & MCDE_DUPLEXER_MASK)
                );

            ch_c_reg->mcde_chc_pbcrc0 =
                (
                    (ch_c_reg->mcde_chc_pbcrc0 &~MCDE_BSDM_MASK) |
                    (((u32) config.data_segment << MCDE_BSDM_SHIFT) & MCDE_BSDM_MASK)
                );

            ch_c_reg->mcde_chc_pbcrc0 =
                (
                    (ch_c_reg->mcde_chc_pbcrc0 &~MCDE_BSCM_MASK) |
                    ((u32) config.cmd_segment & MCDE_BSCM_MASK)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_pbcrc1 =
                (
                    (ch_c_reg->mcde_chc_pbcrc1 &~MCDE_PDCTRL_SHIFT) |
                    (((u32) config.duplex_mode << MCDE_PDCTRL_SHIFT) & MCDE_PDCTRL_SHIFT)
                );

            ch_c_reg->mcde_chc_pbcrc1 =
                (
                    (ch_c_reg->mcde_chc_pbcrc1 &~MCDE_DUPLEXER_MASK) |
                    (((u32) config.duplex_mode << MCDE_DUPLEXER_SHIFT) & MCDE_DUPLEXER_MASK)
                );

            ch_c_reg->mcde_chc_pbcrc1 =
                (
                    (ch_c_reg->mcde_chc_pbcrc1 &~MCDE_BSDM_MASK) |
                    (((u32) config.data_segment << MCDE_BSDM_SHIFT) & MCDE_BSDM_MASK)
                );

            ch_c_reg->mcde_chc_pbcrc1 =
                (
                    (ch_c_reg->mcde_chc_pbcrc1 &~MCDE_BSCM_MASK) |
                    ((u32) config.cmd_segment & MCDE_BSCM_MASK)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdesetPBCmux(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_pbc_mux mux)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_pbcbmrc0[0] = mux.imux0;

            ch_c_reg->mcde_chc_pbcbmrc0[1] = mux.imux1;

            ch_c_reg->mcde_chc_pbcbmrc0[2] = mux.imux2;

            ch_c_reg->mcde_chc_pbcbmrc0[3] = mux.imux3;

            ch_c_reg->mcde_chc_pbcbmrc0[4] = mux.imux4;
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_pbcbmrc1[0] = mux.imux0;

            ch_c_reg->mcde_chc_pbcbmrc1[1] = mux.imux1;

            ch_c_reg->mcde_chc_pbcbmrc1[2] = mux.imux2;

            ch_c_reg->mcde_chc_pbcbmrc1[3] = mux.imux3;

            ch_c_reg->mcde_chc_pbcbmrc1[4] = mux.imux4;
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdesetchnlCvsyndelay(mcde_ch_id chid, mcde_chc_panel panel_id, u8 delay)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_sctrc = ((ch_c_reg->mcde_chc_sctrc &~MCDE_SYNCDELC0_MASK) | (delay & MCDE_SYNCDELC0_MASK));

            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_sctrc =
                (
                    (ch_c_reg->mcde_chc_sctrc &~MCDE_SYNCDELC1_MASK) |
                    ((delay << MCDE_SYNCDELC1_SHIFT) & MCDE_SYNCDELC1_MASK)
                );

            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}

mcde_error mcdesetchnlCsynctrigdelay(mcde_ch_id chid, u8 delay)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    ch_c_reg->mcde_chc_sctrc =
        (
            (ch_c_reg->mcde_chc_sctrc &~MCDE_TRDELC_MASK) |
            ((delay << MCDE_TRDELC_SHIFT) & MCDE_TRDELC_MASK)
        );


    return(error);
}
mcde_error mcdesetPBCbitctrl(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_pbc_bitctrl bit_control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_pbcbcrc0[0] = bit_control.bit_ctrl0;

            ch_c_reg->mcde_chc_pbcbcrc0[1] = bit_control.bit_ctrl1;
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_pbcbcrc1[0] = bit_control.bit_ctrl0;

            ch_c_reg->mcde_chc_pbcbcrc1[1] = bit_control.bit_ctrl1;
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}

mcde_error mcdesetchnlCsynccapconf(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_sync_conf config)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_vscrc[0] =
                (
                    (ch_c_reg->mcde_chc_vscrc[0] &~MCDE_VSDBL_MASK) |
                    ((config.debounce_length << MCDE_VSDBL_SHIFT) & MCDE_VSDBL_MASK)
                );

            ch_c_reg->mcde_chc_vscrc[0] =
                (
                    (ch_c_reg->mcde_chc_vscrc[0] &~MCDE_VSSEL_MASK) |
                    (((u32) config.sync_sel << MCDE_VSSEL_SHIFT) & MCDE_VSSEL_MASK)
                );

            ch_c_reg->mcde_chc_vscrc[0] =
                (
                    (ch_c_reg->mcde_chc_vscrc[0] &~MCDE_VSPOL_MASK) |
                    (((u32) config.sync_pol << MCDE_VSPOL_SHIFT) & MCDE_VSPOL_MASK)
                );

            ch_c_reg->mcde_chc_vscrc[0] =
                (
                    (ch_c_reg->mcde_chc_vscrc[0] &~MCDE_VSPDIV_MASK) |
                    (((u32) config.clk_div << MCDE_VSPDIV_SHIFT) & MCDE_VSPDIV_MASK)
                );

            ch_c_reg->mcde_chc_vscrc[0] =
                (
                    (ch_c_reg->mcde_chc_vscrc[0] &~MCDE_VSPMAX_MASK) |
                    (((u32) config.vsp_max << MCDE_VSPMAX_SHIFT) & MCDE_VSPMAX_MASK)
                );

            ch_c_reg->mcde_chc_vscrc[0] =
                (
                    (ch_c_reg->mcde_chc_vscrc[0] &~MCDE_VSPMIN_MASK) |
                    ((u32) config.vsp_min & MCDE_VSPMIN_MASK)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_vscrc[1] =
                (
                    (ch_c_reg->mcde_chc_vscrc[1] &~MCDE_VSDBL_MASK) |
                    ((config.debounce_length << MCDE_VSDBL_SHIFT) & MCDE_VSDBL_MASK)
                );

            ch_c_reg->mcde_chc_vscrc[1] =
                (
                    (ch_c_reg->mcde_chc_vscrc[1] &~MCDE_VSSEL_MASK) |
                    (((u32) config.sync_sel << MCDE_VSSEL_SHIFT) & MCDE_VSSEL_MASK)
                );

            ch_c_reg->mcde_chc_vscrc[1] =
                (
                    (ch_c_reg->mcde_chc_vscrc[1] &~MCDE_VSPOL_MASK) |
                    (((u32) config.sync_pol << MCDE_VSPOL_SHIFT) & MCDE_VSPOL_MASK)
                );

            ch_c_reg->mcde_chc_vscrc[1] =
                (
                    (ch_c_reg->mcde_chc_vscrc[1] &~MCDE_VSPDIV_MASK) |
                    (((u32) config.clk_div << MCDE_VSPDIV_SHIFT) & MCDE_VSPDIV_MASK)
                );

            ch_c_reg->mcde_chc_vscrc[1] =
                (
                    (ch_c_reg->mcde_chc_vscrc[1] &~MCDE_VSPMAX_MASK) |
                    (((u32) config.vsp_max << MCDE_VSPMAX_SHIFT) & MCDE_VSPMAX_MASK)
                );

            ch_c_reg->mcde_chc_vscrc[1] =
                (
                    (ch_c_reg->mcde_chc_vscrc[1] &~MCDE_VSPMIN_MASK) |
                    ((u32) config.vsp_min & MCDE_VSPMIN_MASK)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdechnlCclkandsyncsel(mcde_ch_id chid, mcde_clk_sel clk_sel, mcde_synchro_select sync_select)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    ch_c_reg->mcde_chc_crc =
        (
            (ch_c_reg->mcde_chc_crc &~MCDE_CLKSEL_MASK) |
            (((u32) clk_sel << MCDE_CLKSEL_SHIFT) & MCDE_CLKSEL_MASK)
        );

    ch_c_reg->mcde_chc_crc =
        (
            (ch_c_reg->mcde_chc_crc &~MCDE_SYNCSEL_MASK) |
            (((u32) sync_select << MCDE_SYNCSEL_SHIFT) & MCDE_SYNCSEL_MASK)
        );

    return(error);
}

mcde_error mcdesetchnlCmode(mcde_ch_id chid, mcde_chc_panel panel_id, mcde_chc_enable state)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_C1EN_MASK) |
                    ((state << MCDE_C1EN_SHIFT) & MCDE_C1EN_MASK)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_C2EN_MASK) |
                    ((state << MCDE_C2EN_SHIFT) & MCDE_C2EN_MASK)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }


    return(error);
}

mcde_error mcdesetsynchromode(mcde_ch_id chid, mcde_chc_panel panel_id, mcde_synchro_capture sync_enable)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);


    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_SYNCEN1_MASK) |
                    (((u32) sync_enable << MCDE_SYNCEN1_SHIFT) & MCDE_SYNCEN1_MASK)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_crc =
                (
                    (ch_c_reg->mcde_chc_crc &~MCDE_SYNCEN2_MASK) |
                    (((u32) sync_enable << MCDE_SYNCEN2_SHIFT) & MCDE_SYNCEN2_MASK)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}

mcde_error mcdesetchipseltiming(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_cd_timing_activate active)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_cscdtr[0] =
                (
                    (ch_c_reg->mcde_chc_cscdtr[0] &~MCDE_CSCDDEACT_MASK) |
                    ((active.cs_cd_deactivate << MCDE_CSCDDEACT_SHIFT) & MCDE_CSCDDEACT_MASK)
                );

            ch_c_reg->mcde_chc_cscdtr[0] =
                (
                    (ch_c_reg->mcde_chc_cscdtr[0] &~MCDE_CSCDACT_MASK) |
                    (active.cs_cd_activate & MCDE_CSCDACT_MASK)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_cscdtr[1] =
                (
                    (ch_c_reg->mcde_chc_cscdtr[1] &~MCDE_CSCDDEACT_MASK) |
                    ((active.cs_cd_deactivate << MCDE_CSCDDEACT_SHIFT) & MCDE_CSCDDEACT_MASK)
                );

            ch_c_reg->mcde_chc_cscdtr[1] =
                (
                    (ch_c_reg->mcde_chc_cscdtr[1] &~MCDE_CSCDACT_MASK) |
                    (active.cs_cd_activate & MCDE_CSCDACT_MASK)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdesetbusaccessnum(mcde_ch_id chid, mcde_chc_panel panel_id, u8 bcn)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_bcnr[0] = ((ch_c_reg->mcde_chc_bcnr[0] &~MCDE_BCN_MASK) | (bcn & MCDE_BCN_MASK));
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_bcnr[1] = ((ch_c_reg->mcde_chc_bcnr[1] &~MCDE_BCN_MASK) | (bcn & MCDE_BCN_MASK));
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdesetrdwrtiming(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_rw_timing rw_time)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_rdwrtr[0] =
                (
                    (ch_c_reg->mcde_chc_rdwrtr[0] &~MCDE_MOTINT_MASK) |
                    (((u32) rw_time.panel_type << MCDE_MOTINT_SHIFT) & MCDE_MOTINT_MASK)
                );

            ch_c_reg->mcde_chc_rdwrtr[0] =
                (
                    (ch_c_reg->mcde_chc_rdwrtr[0] &~MCDE_RWDEACT_MASK) |
                    ((rw_time.readwrite_deactivate << MCDE_RWDEACT_SHIFT) & MCDE_RWDEACT_MASK)
                );

            ch_c_reg->mcde_chc_rdwrtr[0] =
                (
                    (ch_c_reg->mcde_chc_rdwrtr[0] &~MCDE_RWACT_MASK) |
                    (rw_time.readwrite_activate & MCDE_RWACT_MASK)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_rdwrtr[1] =
                (
                    (ch_c_reg->mcde_chc_rdwrtr[1] &~MCDE_MOTINT_MASK) |
                    (((u32) rw_time.panel_type << MCDE_MOTINT_SHIFT) & MCDE_MOTINT_MASK)
                );

            ch_c_reg->mcde_chc_rdwrtr[1] =
                (
                    (ch_c_reg->mcde_chc_rdwrtr[1] &~MCDE_RWDEACT_MASK) |
                    ((rw_time.readwrite_deactivate << MCDE_RWDEACT_SHIFT) & MCDE_RWDEACT_MASK)
                );

            ch_c_reg->mcde_chc_rdwrtr[1] =
                (
                    (ch_c_reg->mcde_chc_rdwrtr[1] &~MCDE_RWACT_MASK) |
                    (rw_time.readwrite_activate & MCDE_RWACT_MASK)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}

mcde_error mcdesetdataouttiming(mcde_ch_id chid, mcde_chc_panel panel_id, struct mcde_data_out_timing data_time)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_dotr[0] =
                (
                    (ch_c_reg->mcde_chc_dotr[0] &~MCDE_DODEACT_MASK) |
                    ((data_time.data_out_deactivate << MCDE_DODEACT_SHIFT) & MCDE_DODEACT_MASK)
                );

            ch_c_reg->mcde_chc_dotr[0] =
                (
                    (ch_c_reg->mcde_chc_dotr[0] &~MCDE_DOACT_MASK) |
                    (data_time.data_out_activate & MCDE_DOACT_MASK)
                );
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_dotr[1] =
                (
                    (ch_c_reg->mcde_chc_dotr[1] &~MCDE_DODEACT_MASK) |
                    ((data_time.data_out_deactivate << MCDE_DODEACT_SHIFT) & MCDE_DODEACT_MASK)
                );

            ch_c_reg->mcde_chc_dotr[1] =
                (
                    (ch_c_reg->mcde_chc_dotr[1] &~MCDE_DOACT_MASK) |
                    (data_time.data_out_activate & MCDE_DOACT_MASK)
                );
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}

mcde_error mcdewrcmd(mcde_ch_id chid, mcde_chc_panel panel_id, u32 command)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_wcmd[0] = (command & MCDE_DATACOMMANDMASK);

            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_wcmd[1] = (command & MCDE_DATACOMMANDMASK);

            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdewrdata(mcde_ch_id chid, mcde_chc_panel panel_id, u32 data)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);

    switch (panel_id)
    {
        case MCDE_PANEL_C0:
            ch_c_reg->mcde_chc_wd[0] = (data & MCDE_DATACOMMANDMASK);
            break;

        case MCDE_PANEL_C1:
            ch_c_reg->mcde_chc_wd[1] = (data & MCDE_DATACOMMANDMASK);
            break;

        default:
            return(MCDE_INVALID_PARAMETER);
    }

    return(error);
}
mcde_error mcdewrtxfifo(mcde_ch_id chid, mcde_chc_panel panel_id, mcde_txfifo_request_type type, u32 data)
{
    mcde_error    error = MCDE_OK;
    struct mcde_chc_reg  *ch_c_reg;

    ch_c_reg = (struct mcde_chc_reg *) (gpar[chid]->ch_c_reg);
    switch (type)
    {
        case MCDE_TXFIFO_WRITE_DATA:
            ch_c_reg->mcde_chc_wd[panel_id] = (data & MCDE_DATACOMMANDMASK);

            break;

        case MCDE_TXFIFO_WRITE_COMMAND:
            ch_c_reg->mcde_chc_wcmd[panel_id] = (data & MCDE_DATACOMMANDMASK);

            break;

        default:
            error = MCDE_INVALID_PARAMETER;
    }

    return(error);
}

mcde_error mcdesetcflowXcolorkeyctrl(mcde_ch_id chid, mcde_key_ctrl key_ctrl)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_KEYCTRL_MASK) |
            (((u32) key_ctrl << MCDE_KEYCTRL_SHIFT) & MCDE_KEYCTRL_MASK)
        );
    return(error);
}
mcde_error mcdesetblendctrl(mcde_ch_id chid, struct mcde_blend_control blend_ctrl)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_CHX_ALPHA_MASK) |
            (((u32) blend_ctrl.alpha_blend << MCDE_CHX_ALPHA_SHIFT) & MCDE_CHX_ALPHA_MASK)
        );
    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_BLENDCONTROL_MASK) |
            (((u32) blend_ctrl.blend_ctrl << MCDE_BLENDCONTROL_SHIFT) & MCDE_BLENDCONTROL_MASK)
        );
    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_BLENDEN_MASK) |
            (((u32) blend_ctrl.blenden << MCDE_BLENDEN_SHIFT) & MCDE_BLENDEN_MASK)
        );
    return(error);
}
mcde_error mcdesetrotation(mcde_ch_id chid, mcde_rot_dir rot_dir, mcde_roten rot_ctrl)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;


    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }
    if (rot_ctrl == MCDE_ROTATION_ENABLE)
    {
    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_CHX_ROTDIR_MASK) |
            (((u32) rot_dir << MCDE_CHX_ROTDIR_SHIFT) & MCDE_CHX_ROTDIR_MASK)
        );
    }
    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_ROTEN_MASK) |
            (((u32) rot_ctrl << MCDE_ROTEN_SHIFT) & MCDE_ROTEN_MASK)
        );
    return error;
}
mcde_error mcdesetditheringctrl(mcde_ch_id chid, mcde_dithering_ctrl dithering_control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }
    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_DITHEN_MASK) |
            (((u32) dithering_control << MCDE_DITHEN_SHIFT) & MCDE_DITHEN_MASK)
        );
    return (error);
}
mcde_error mcdesetflowXctrl(mcde_ch_id chid, struct mcde_chx_control0 control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;


    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_CHX_BURSTSIZE_MASK) |
            (((u32) control.chx_read_request << MCDE_CHX_BURSTSIZE_SHIFT) & MCDE_CHX_BURSTSIZE_MASK)
        );

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_CHX_ALPHA_MASK) |
            (((u32) control.alpha_blend << MCDE_CHX_ALPHA_SHIFT) & MCDE_CHX_ALPHA_MASK)
        );

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_CHX_ROTDIR_MASK) |
            (((u32) control.rot_dir << MCDE_CHX_ROTDIR_SHIFT) & MCDE_CHX_ROTDIR_MASK)
        );

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_CHX_GAMAEN_MASK) |
            (((u32) control.gamma_ctrl << MCDE_CHX_GAMAEN_SHIFT) & MCDE_CHX_GAMAEN_MASK)
        );

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_FLICKFORMAT_MASK) |
            (((u32) control.flicker_format << MCDE_FLICKFORMAT_SHIFT) & MCDE_FLICKFORMAT_MASK)
        );

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_FLICKMODE_MASK) |
            (((u32) control.filter_mode << MCDE_FLICKMODE_SHIFT) & MCDE_FLICKMODE_MASK)
        );

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_BLENDCONTROL_MASK) |
            (((u32) control.blend << MCDE_BLENDCONTROL_SHIFT) & MCDE_BLENDCONTROL_MASK)
        );

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_KEYCTRL_MASK) |
            (((u32) control.key_ctrl << MCDE_KEYCTRL_SHIFT) & MCDE_KEYCTRL_MASK)
        );

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_ROTEN_MASK) |
            (((u32) control.rot_enable << MCDE_ROTEN_SHIFT) & MCDE_ROTEN_MASK)
        );

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_DITHEN_MASK) |
            (((u32) control.dither_ctrl << MCDE_DITHEN_SHIFT) & MCDE_DITHEN_MASK)
        );

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_CEAEN_MASK) |
            (((u32) control.color_enhance << MCDE_CEAEN_SHIFT) & MCDE_CEAEN_MASK)
        );

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_AFLICKEN_MASK) |
            (((u32) control.anti_flicker << MCDE_AFLICKEN_SHIFT) & MCDE_AFLICKEN_MASK)
        );

    ch_x_reg->mcde_ch_cr0 =
        (
            (ch_x_reg->mcde_ch_cr0 &~MCDE_BLENDEN_MASK) |
            (((u32) control.blend_ctrl << MCDE_BLENDEN_SHIFT) & MCDE_BLENDEN_MASK)
        );


    return(error);
}

mcde_error mcdesetpanelctrl(mcde_ch_id chid, struct mcde_chx_control1 control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

	if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_ch_cr1 =
        (
            (ch_x_reg->mcde_ch_cr1 &~MCDE_CLK_MASK) |
            (((u32) control.tv_clk << MCDE_CLK_SHIFT) & MCDE_CLK_MASK)
        );

    ch_x_reg->mcde_ch_cr1 =
        (
            (ch_x_reg->mcde_ch_cr1 &~MCDE_BCD_MASK) |
            (((u32) control.bcd_ctrl << MCDE_BCD_SHIFT) & MCDE_BCD_MASK)
        );

    ch_x_reg->mcde_ch_cr1 =
        (
            (ch_x_reg->mcde_ch_cr1 &~MCDE_OUTBPP_MASK) |
            (((u32) control.out_bpp << MCDE_OUTBPP_SHIFT) & MCDE_OUTBPP_MASK)
        );
/*  Only applicable to older version of chip
    ch_x_reg->mcde_ch_cr1 =
        (
            (ch_x_reg->mcde_ch_cr1 &~MCDE_CLP_MASK) |
            (((u32) control.clk_per_line << MCDE_CLP_SHIFT) & MCDE_CLP_MASK)
        );
*/

    ch_x_reg->mcde_ch_cr1 =
        (
            (ch_x_reg->mcde_ch_cr1 &~MCDE_CDWIN_MASK) |
            (((u32) control.lcd_bus << MCDE_CDWIN_SHIFT) & MCDE_CDWIN_MASK)
        );

    ch_x_reg->mcde_ch_cr1 =
        (
            (ch_x_reg->mcde_ch_cr1 &~MCDE_CLOCKSEL_MASK) |
            (((u32) control.dpi2_clk << MCDE_CLOCKSEL_SHIFT) & MCDE_CLOCKSEL_MASK)
        );

    ch_x_reg->mcde_ch_cr1 = ((ch_x_reg->mcde_ch_cr1 &~MCDE_PCD_MASK) | ((u32) control.pcd & MCDE_PCD_MASK));

    return(error);
}

mcde_error mcdesetcolorkey(mcde_ch_id chid, struct mcde_chx_color_key key, mcde_colorkey_type type)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;


    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

	if (type == MCDE_COLORKEY_NORMAL)
	{
	ch_x_reg->mcde_ch_colkey =
		(
		(ch_x_reg->mcde_ch_colkey &~MCDE_KEYA_MASK) |
		((key.alpha << MCDE_KEYA_SHIFT) & MCDE_KEYA_MASK)
		);

	ch_x_reg->mcde_ch_colkey =
        (
            (ch_x_reg->mcde_ch_colkey &~MCDE_KEYR_MASK) |
            ((key.red << MCDE_KEYR_SHIFT) & MCDE_KEYR_MASK)
        );

	ch_x_reg->mcde_ch_colkey =
        (
            (ch_x_reg->mcde_ch_colkey &~MCDE_KEYG_MASK) |
            ((key.green << MCDE_KEYG_SHIFT) & MCDE_KEYG_MASK)
        );

	ch_x_reg->mcde_ch_colkey = ((ch_x_reg->mcde_ch_colkey &~MCDE_KEYB_MASK) | (key.blue & MCDE_KEYB_MASK));
	}
	else
	if (type == MCDE_COLORKEY_FORCE)
	{
	    ch_x_reg->mcde_ch_fcolkey =
        (
            (ch_x_reg->mcde_ch_fcolkey &~MCDE_KEYA_MASK) |
            ((key.alpha << MCDE_KEYA_SHIFT) & MCDE_KEYA_MASK)
        );

	ch_x_reg->mcde_ch_fcolkey =
        (
            (ch_x_reg->mcde_ch_fcolkey &~MCDE_KEYR_MASK) |
            ((key.red << MCDE_KEYR_SHIFT) & MCDE_KEYR_MASK)
        );

	ch_x_reg->mcde_ch_fcolkey =
        (
            (ch_x_reg->mcde_ch_fcolkey &~MCDE_KEYG_MASK) |
            ((key.green << MCDE_KEYG_SHIFT) & MCDE_KEYG_MASK)
        );

	ch_x_reg->mcde_ch_fcolkey = ((ch_x_reg->mcde_ch_fcolkey &~MCDE_KEYB_MASK) | (key.blue & MCDE_KEYB_MASK));
	}


    return(error);
}
mcde_error mcdesetcolorconvmatrix(mcde_ch_id chid, struct mcde_chx_rgb_conv_coef coef)
{
    mcde_error    error = MCDE_OK;
	struct mcde_ch_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_ch_rgbconv1 =
        (
            (ch_x_reg->mcde_ch_rgbconv1 &~MCDE_RGB_MASK1) |
            ((coef.Yr_red << MCDE_RGB_SHIFT) & MCDE_RGB_MASK1)
        );

    ch_x_reg->mcde_ch_rgbconv1 = ((ch_x_reg->mcde_ch_rgbconv1 &~MCDE_RGB_MASK2) | (coef.Yr_green & MCDE_RGB_MASK2));

    ch_x_reg->mcde_ch_rgbconv2 =
        (
            (ch_x_reg->mcde_ch_rgbconv2 &~MCDE_RGB_MASK1) |
            ((coef.Yr_blue << MCDE_RGB_SHIFT) & MCDE_RGB_MASK1)
        );

    ch_x_reg->mcde_ch_rgbconv2 = ((ch_x_reg->mcde_ch_rgbconv2 &~MCDE_RGB_MASK2) | (coef.Cr_red & MCDE_RGB_MASK2));

    ch_x_reg->mcde_ch_rgbconv3 =
        (
            (ch_x_reg->mcde_ch_rgbconv3 &~MCDE_RGB_MASK1) |
            ((coef.Cr_green << MCDE_RGB_SHIFT) & MCDE_RGB_MASK1)
        );

    ch_x_reg->mcde_ch_rgbconv3 = ((ch_x_reg->mcde_ch_rgbconv3 &~MCDE_RGB_MASK2) | (coef.Cr_blue & MCDE_RGB_MASK2));

    ch_x_reg->mcde_ch_rgbconv4 =
        (
            (ch_x_reg->mcde_ch_rgbconv4 &~MCDE_RGB_MASK1) |
            ((coef.Cb_red << MCDE_RGB_SHIFT) & MCDE_RGB_MASK1)
        );

    ch_x_reg->mcde_ch_rgbconv4 = ((ch_x_reg->mcde_ch_rgbconv4 &~MCDE_RGB_MASK2) | (coef.Cb_green & MCDE_RGB_MASK2));

    ch_x_reg->mcde_ch_rgbconv5 =
        (
            (ch_x_reg->mcde_ch_rgbconv5 &~MCDE_RGB_MASK1) |
            ((coef.Cb_blue << MCDE_RGB_SHIFT) & MCDE_RGB_MASK1)
        );

    ch_x_reg->mcde_ch_rgbconv5 = ((ch_x_reg->mcde_ch_rgbconv5 &~MCDE_RGB_MASK2) | (coef.Off_red & MCDE_RGB_MASK2));

    ch_x_reg->mcde_ch_rgbconv6 =
        (
            (ch_x_reg->mcde_ch_rgbconv6 &~MCDE_RGB_MASK1) |
            ((coef.Off_green << MCDE_RGB_SHIFT) & MCDE_RGB_MASK1)
        );

    ch_x_reg->mcde_ch_rgbconv6 = ((ch_x_reg->mcde_ch_rgbconv6 &~MCDE_RGB_MASK2) | (coef.Off_blue & MCDE_RGB_MASK2));

    return(error);
}
mcde_error mcdesetflickerfiltercoef(mcde_ch_id chid, struct mcde_chx_flickfilter_coef coef)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

	if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }


    ch_x_reg->mcde_ch_ffcoef0 =
        (
            (ch_x_reg->mcde_ch_ffcoef0 &~MCDE_THRESHOLD_MASK) |
            ((coef.threshold_ctrl0 << MCDE_THRESHOLD_SHIFT) & MCDE_THRESHOLD_MASK)
        );

    ch_x_reg->mcde_ch_ffcoef0 =
        (
            (ch_x_reg->mcde_ch_ffcoef0 &~MCDE_COEFFN3_MASK) |
            ((coef.Coeff0_N3 << MCDE_COEFFN3_SHIFT) & MCDE_COEFFN3_MASK)
        );

    ch_x_reg->mcde_ch_ffcoef0 =
        (
            (ch_x_reg->mcde_ch_ffcoef0 &~MCDE_COEFFN2_MASK) |
            ((coef.Coeff0_N2 << MCDE_COEFFN2_SHIFT) & MCDE_COEFFN2_MASK)
        );

    ch_x_reg->mcde_ch_ffcoef0 =
        (
            (ch_x_reg->mcde_ch_ffcoef0 &~MCDE_COEFFN1_MASK) |
            (coef.Coeff0_N1 & MCDE_COEFFN1_MASK)
        );

    ch_x_reg->mcde_ch_ffcoef1 =
        (
            (ch_x_reg->mcde_ch_ffcoef1 &~MCDE_THRESHOLD_MASK) |
            ((coef.threshold_ctrl1 << MCDE_THRESHOLD_SHIFT) & MCDE_THRESHOLD_MASK)
        );

    ch_x_reg->mcde_ch_ffcoef1 =
        (
            (ch_x_reg->mcde_ch_ffcoef1 &~MCDE_COEFFN3_MASK) |
            ((coef.Coeff1_N3 << MCDE_COEFFN3_SHIFT) & MCDE_COEFFN3_MASK)
        );

    ch_x_reg->mcde_ch_ffcoef1 =
        (
            (ch_x_reg->mcde_ch_ffcoef1 &~MCDE_COEFFN2_MASK) |
            ((coef.Coeff1_N2 << MCDE_COEFFN2_SHIFT) & MCDE_COEFFN2_MASK)
        );

    ch_x_reg->mcde_ch_ffcoef1 =
        (
            (ch_x_reg->mcde_ch_ffcoef1 &~MCDE_COEFFN1_MASK) |
            (coef.Coeff1_N1 & MCDE_COEFFN1_MASK)
        );

    ch_x_reg->mcde_ch_ffcoef2 =
        (
            (ch_x_reg->mcde_ch_ffcoef2 &~MCDE_THRESHOLD_MASK) |
            ((coef.threshold_ctrl2 << MCDE_THRESHOLD_SHIFT) & MCDE_THRESHOLD_MASK)
        );

    ch_x_reg->mcde_ch_ffcoef2 =
        (
            (ch_x_reg->mcde_ch_ffcoef2 &~MCDE_COEFFN3_MASK) |
            ((coef.Coeff2_N3 << MCDE_COEFFN3_SHIFT) & MCDE_COEFFN3_MASK)
        );

    ch_x_reg->mcde_ch_ffcoef2 =
        (
            (ch_x_reg->mcde_ch_ffcoef2 &~MCDE_COEFFN2_MASK) |
            ((coef.Coeff2_N2 << MCDE_COEFFN2_SHIFT) & MCDE_COEFFN2_MASK)
        );

    ch_x_reg->mcde_ch_ffcoef2 =
        (
            (ch_x_reg->mcde_ch_ffcoef2 &~MCDE_COEFFN1_MASK) |
            (coef.Coeff2_N1 & MCDE_COEFFN1_MASK)
        );

    return(error);
}

mcde_error mcdesetLCDtiming0ctrl(mcde_ch_id chid, struct mcde_chx_lcd_timing0 control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }


    ch_x_reg->mcde_ch_lcdtim0 =
        (
            (ch_x_reg->mcde_ch_lcdtim0 &~MCDE_REVVAEN_MASK) |
            (((u32) control.rev_va_enable << MCDE_REVVAEN_SHIFT) & MCDE_REVVAEN_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim0 =
        (
            (ch_x_reg->mcde_ch_lcdtim0 &~MCDE_REVTGEN_MASK) |
            (((u32) control.rev_toggle_enable << MCDE_REVTGEN_SHIFT) & MCDE_REVTGEN_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim0 =
        (
            (ch_x_reg->mcde_ch_lcdtim0 &~MCDE_REVLOADSEL_MASK) |
            (((u32) control.rev_sync_sel << MCDE_REVLOADSEL_SHIFT) & MCDE_REVLOADSEL_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim0 =
        (
            (ch_x_reg->mcde_ch_lcdtim0 &~MCDE_REVDEL1_MASK) |
            (((u32) control.rev_delay1 << MCDE_REVDEL1_SHIFT) & MCDE_REVDEL1_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim0 =
        (
            (ch_x_reg->mcde_ch_lcdtim0 &~MCDE_REVDEL0_MASK) |
            (((u32) control.rev_delay0 << MCDE_REVDEL0_SHIFT) & MCDE_REVDEL0_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim0 =
        (
            (ch_x_reg->mcde_ch_lcdtim0 &~MCDE_PSVAEN_MASK) |
            (((u32) control.ps_va_enable << MCDE_PSVAEN_SHIFT) & MCDE_PSVAEN_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim0 =
        (
            (ch_x_reg->mcde_ch_lcdtim0 &~MCDE_PSTGEN_MASK) |
            (((u32) control.ps_toggle_enable << MCDE_PSTGEN_SHIFT) & MCDE_PSTGEN_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim0 =
        (
            (ch_x_reg->mcde_ch_lcdtim0 &~MCDE_PSLOADSEL_MASK) |
            (((u32) control.ps_sync_sel << MCDE_PSLOADSEL_SHIFT) & MCDE_PSLOADSEL_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim0 =
        (
            (ch_x_reg->mcde_ch_lcdtim0 &~MCDE_PSDEL1_MASK) |
            (((u32) control.ps_delay1 << MCDE_PSDEL1_SHIFT) & MCDE_PSDEL1_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim0 =
        (
            (ch_x_reg->mcde_ch_lcdtim0 &~MCDE_PSDEL0_MASK) |
            ((u32) control.ps_delay0 & MCDE_PSDEL0_MASK)
        );

    return(error);
}

mcde_error mcdesetLCDtiming1ctrl(mcde_ch_id chid, struct mcde_chx_lcd_timing1 control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

   if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }


    ch_x_reg->mcde_ch_lcdtim1 =
        (
            (ch_x_reg->mcde_ch_lcdtim1 &~MCDE_IOE_MASK) |
            (((u32) control.io_enable << MCDE_IOE_SHIFT) & MCDE_IOE_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim1 =
        (
            (ch_x_reg->mcde_ch_lcdtim1 &~MCDE_IPC_MASK) |
            (((u32) control.ipc << MCDE_IPC_SHIFT) & MCDE_IPC_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim1 =
        (
            (ch_x_reg->mcde_ch_lcdtim1 &~MCDE_IHS_MASK) |
            (((u32) control.ihs << MCDE_IHS_SHIFT) & MCDE_IHS_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim1 =
        (
            (ch_x_reg->mcde_ch_lcdtim1 &~MCDE_IVS_MASK) |
            (((u32) control.ivs << MCDE_IVS_SHIFT) & MCDE_IVS_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim1 =
        (
            (ch_x_reg->mcde_ch_lcdtim1 &~MCDE_IVP_MASK) |
            (((u32) control.ivp << MCDE_IVP_SHIFT) & MCDE_IPC_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim1 =
        (
            (ch_x_reg->mcde_ch_lcdtim1 &~MCDE_ICLSPL_MASK) |
            (((u32) control.iclspl << MCDE_ICLSPL_SHIFT) & MCDE_ICLSPL_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim1 =
        (
            (ch_x_reg->mcde_ch_lcdtim1 &~MCDE_ICLREV_MASK) |
            (((u32) control.iclrev << MCDE_ICLREV_SHIFT) & MCDE_ICLREV_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim1 =
        (
            (ch_x_reg->mcde_ch_lcdtim1 &~MCDE_ICLSP_MASK) |
            (((u32) control.iclsp << MCDE_ICLSP_SHIFT) & MCDE_ICLSP_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim1 =
        (
            (ch_x_reg->mcde_ch_lcdtim1 &~MCDE_SPLVAEN_MASK) |
            (((u32) control.mcde_spl << MCDE_SPLVAEN_SHIFT) & MCDE_SPLVAEN_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim1 =
        (
            (ch_x_reg->mcde_ch_lcdtim1 &~MCDE_SPLTGEN_MASK) |
            (((u32) control.spltgen << MCDE_SPLTGEN_SHIFT) & MCDE_SPLTGEN_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim1 =
        (
            (ch_x_reg->mcde_ch_lcdtim1 &~MCDE_SPLLOADSEL_MASK) |
            (((u32) control.spl_sync_sel << MCDE_SPLLOADSEL_SHIFT) & MCDE_SPLLOADSEL_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim1 =
        (
            (ch_x_reg->mcde_ch_lcdtim1 &~MCDE_SPLDEL1_MASK) |
            (((u32) control.spl_delay1 << MCDE_SPLDEL1_SHIFT) & MCDE_SPLDEL1_MASK)
        );

    ch_x_reg->mcde_ch_lcdtim1 =
        (
            (ch_x_reg->mcde_ch_lcdtim1 &~MCDE_SPLDEL0_MASK) |
            ((u32) control.spl_delay0 & MCDE_SPLDEL0_MASK)
        );

    return(error);
}
mcde_error mcdesetrotaddr(mcde_ch_id chid, u32 address, mcde_rotate_num rotnum)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

	if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }


    if (0x0 == address)
    {
        return(MCDE_INVALID_PARAMETER);
    }
	if(rotnum == MCDE_ROTATE0)
	    ch_x_reg->mcde_rotadd0 = address;
	else if(rotnum == MCDE_ROTATE1)
		ch_x_reg->mcde_rotadd1 = address;

    return(error);
}

/****************************************************************************/
mcde_error mcdesetstate(mcde_ch_id chid, mcde_state state)
{
    mcde_error    error = MCDE_OK;

#ifdef PLATFORM_8500

    gpar[chid]->regbase->mcde_cr =
        (
            (gpar[chid]->regbase->mcde_cr &~MCDE_CTRL_MCDEEN_MASK) |
            (((u32) state << MCDE_CTRL_MCDEEN_SHIFT) & MCDE_CTRL_MCDEEN_MASK)
        );
#endif
#ifdef PLATFORM_8820

	gpar[chid]->regbase->mcde_cr |= state;
#endif

    return(error);
}

mcde_error mcdesetpalette(mcde_ch_id chid, mcde_palette palette)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;


	if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }


    ch_x_reg->mcde_ch_pal =
        (
            (ch_x_reg->mcde_ch_pal &~MCDE_ARED_MASK) |
            ((palette.alphared << MCDE_ARED_SHIFT) & MCDE_ARED_MASK)
        );

    ch_x_reg->mcde_ch_pal =
        (
            (ch_x_reg->mcde_ch_pal &~MCDE_GREEN_MASK) |
            ((palette.green << MCDE_GREEN_SHIFT) & MCDE_GREEN_MASK)
        );

    ch_x_reg->mcde_ch_pal = ((ch_x_reg->mcde_ch_pal &~MCDE_BLUE_MASK) | (palette.blue & MCDE_BLUE_MASK));

    return(error);
}


mcde_error mcdesetditherctrl(mcde_ch_id chid, struct mcde_chx_dither_ctrl control)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }
    ch_x_reg->mcde_ch_ditctrl =
        (
            (ch_x_reg->mcde_ch_ditctrl &~MCDE_FOFFY_MASK) |
            ((control.y_offset << MCDE_FOFFY_SHIFT) & MCDE_FOFFY_MASK)
        );

    ch_x_reg->mcde_ch_ditctrl =
        (
            (ch_x_reg->mcde_ch_ditctrl &~MCDE_FOFFX_MASK) |
            ((control.x_offset << MCDE_FOFFX_SHIFT) & MCDE_FOFFX_MASK)
        );

    ch_x_reg->mcde_ch_ditctrl =
        (
            (ch_x_reg->mcde_ch_ditctrl &~MCDE_MASK_BITCTRL_MASK) |
            (((u32) control.masking_ctrl << MCDE_MASK_BITCTRL_SHIFT) & MCDE_MASK_BITCTRL_MASK)
        );

    ch_x_reg->mcde_ch_ditctrl =
        (
            (ch_x_reg->mcde_ch_ditctrl &~MCDE_MODE_MASK) |
            ((control.mode << MCDE_MODE_SHIFT) & MCDE_MODE_MASK)
        );

    ch_x_reg->mcde_ch_ditctrl =
        (
            (ch_x_reg->mcde_ch_ditctrl &~MCDE_COMP_MASK) |
            (((u32) control.comp_dithering << MCDE_COMP_SHIFT) & MCDE_COMP_MASK)
        );

    ch_x_reg->mcde_ch_ditctrl =
        (
            (ch_x_reg->mcde_ch_ditctrl &~MCDE_TEMP_MASK) |
            ((u32) control.temp_dithering & MCDE_TEMP_MASK)
        );
    return(error);
}

mcde_error mcdesetditheroffset(mcde_ch_id chid, struct mcde_chx_dithering_offset offset)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }


    ch_x_reg->mcde_ch_ditctrl =
        (
            (ch_x_reg->mcde_ch_ditctrl &~MCDE_YB_MASK) |
            ((offset.y_offset_rb << MCDE_YB_SHIFT) & MCDE_YB_MASK)
        );

    ch_x_reg->mcde_ch_ditctrl =
        (
            (ch_x_reg->mcde_ch_ditctrl &~MCDE_XB_MASK) |
            ((offset.x_offset_rb << MCDE_XB_SHIFT) & MCDE_XB_MASK)
        );

    ch_x_reg->mcde_ch_ditctrl =
        (
            (ch_x_reg->mcde_ch_ditctrl &~MCDE_YG_MASK) |
            ((offset.y_offset_rg << MCDE_YG_SHIFT) & MCDE_YG_MASK)
        );

    ch_x_reg->mcde_ch_ditctrl = ((ch_x_reg->mcde_ch_ditctrl &~MCDE_XG_MASK) | (offset.x_offset_rg & MCDE_XG_MASK));

    return(error);
}

mcde_error mcdesetgammacoeff(mcde_ch_id chid, struct mcde_chx_gamma gamma)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_ch_gam =
        (
            (ch_x_reg->mcde_ch_gam &~MCDE_RED_MASK) |
            ((gamma.red << MCDE_ARED_SHIFT) & MCDE_RED_MASK)
        );

    ch_x_reg->mcde_ch_gam =
        (
            (ch_x_reg->mcde_ch_gam &~MCDE_GREEN_MASK) |
            ((gamma.green << MCDE_GREEN_SHIFT) & MCDE_GREEN_MASK)
        );

    ch_x_reg->mcde_ch_gam = ((ch_x_reg->mcde_ch_gam &~MCDE_BLUE_MASK) | (gamma.blue & MCDE_BLUE_MASK));

    return(error);
}
mcde_error mcdesetscanmode(mcde_ch_id chid, mcde_scan_mode scan_mode)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }
    ch_x_reg->mcde_ch_tvcr = ((ch_x_reg->mcde_ch_tvcr &~MCDE_INTEREN_MASK) |(((u32) scan_mode << MCDE_INTEREN_SHIFT) & MCDE_INTEREN_MASK));

    return error;
}
mcde_error mcdesetchnlLCDctrlreg(mcde_ch_id chid, struct mcde_chnl_lcd_ctrl_reg lcd_ctrl_reg)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_ch_tvcr = ((ch_x_reg->mcde_ch_tvcr &~MCDE_TV_LINES_MASK) |((lcd_ctrl_reg.num_lines << MCDE_TV_LINES_SHIFT) & MCDE_TV_LINES_MASK));
    ch_x_reg->mcde_ch_tvcr = ((ch_x_reg->mcde_ch_tvcr &~MCDE_TVMODE_MASK) |(((u32) lcd_ctrl_reg.tv_mode << MCDE_TVMODE_SHIFT) & MCDE_TVMODE_MASK));
    ch_x_reg->mcde_ch_tvcr = ((ch_x_reg->mcde_ch_tvcr &~MCDE_IFIELD_MASK) |(((u32) lcd_ctrl_reg.ifield << MCDE_IFIELD_SHIFT) & MCDE_IFIELD_MASK));
    ch_x_reg->mcde_ch_tvcr = ((ch_x_reg->mcde_ch_tvcr &~MCDE_INTEREN_MASK) |(((u32) 0x0 << MCDE_INTEREN_SHIFT) & MCDE_INTEREN_MASK));
    ch_x_reg->mcde_ch_tvcr = ((ch_x_reg->mcde_ch_tvcr &~MCDE_SELMODE_MASK) |((u32) lcd_ctrl_reg.sel_mode & MCDE_SELMODE_MASK));
    ch_x_reg->mcde_ch_tvtim1 = ((ch_x_reg->mcde_ch_tvtim1 & ~MCDE_SWW_MASK) | ((lcd_ctrl_reg.ppl << MCDE_SWW_SHIFT) & MCDE_SWW_MASK));

    return(error);
}
mcde_error mcdesetchnlLCDhorizontaltiming(mcde_ch_id chid, struct mcde_chnl_lcd_horizontal_timing lcd_horizontal_timing)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_ch_tvtim1 = ((ch_x_reg->mcde_ch_tvtim1 & ~MCDE_DHO_MASK) | (lcd_horizontal_timing.hbp & MCDE_DHO_MASK));
    ch_x_reg->mcde_ch_tvbalw = ((ch_x_reg->mcde_ch_tvbalw & ~MCDE_ALW_MASK) | ((lcd_horizontal_timing.hfp << MCDE_ALW_SHIFT) & MCDE_ALW_MASK));
    ch_x_reg->mcde_ch_tvbalw = ((ch_x_reg->mcde_ch_tvbalw & ~MCDE_LBW_MASK) | (lcd_horizontal_timing.hsw & MCDE_LBW_MASK));

    return(error);
}
mcde_error mcdesetchnlLCDverticaltiming(mcde_ch_id chid, struct mcde_chnl_lcd_vertical_timing lcd_vertical_timing)
{
    mcde_error    error = MCDE_OK;
    struct mcde_ch_reg   *ch_x_reg;

    if (MCDE_CH_B < (u32) chid)
    {
        return(MCDE_INVALID_PARAMETER);
    }
    else
    {
        ch_x_reg = (struct mcde_ch_reg *) (gpar[chid]->ch_regbase2[chid]);
    }

    ch_x_reg->mcde_ch_tvbl1 = ((ch_x_reg->mcde_ch_tvbl1 & ~MCDE_BSL_MASK) | ((lcd_vertical_timing.vfp << MCDE_BSL_SHIFT) & MCDE_BSL_MASK));
    ch_x_reg->mcde_ch_tvbl1 = ((ch_x_reg->mcde_ch_tvbl1 & ~MCDE_BEL_MASK) | (lcd_vertical_timing.vsw  & MCDE_BEL_MASK));
    ch_x_reg->mcde_ch_tvdvo = ((ch_x_reg->mcde_ch_tvdvo & ~MCDE_DVO1_MASK) | (lcd_vertical_timing.vbp & MCDE_DVO1_MASK));

    return(error);
}
#ifdef PLATFORM_8820

mcde_error mcdesetoutmuxconf(mcde_ch_id chid, mcde_out_bpp outbpp)
{
	mcde_error	  error = MCDE_OK;

	switch (chid)
	{
	case CHANNEL_A:
		if (outbpp == MCDE_BPP_1_TO_8)
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_A_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));

		else if (outbpp == MCDE_BPP_16)
		{
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_A_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX1_MASK) | (((u32)MCDE_CH_A_MID << MCDE_CFG_OUTMUX1_SHIFT)& MCDE_CFG_OUTMUX1_MASK));
		}
		else if (outbpp == MCDE_BPP_24)
		{
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_A_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX1_MASK) | (((u32)MCDE_CH_A_MID << MCDE_CFG_OUTMUX1_SHIFT)& MCDE_CFG_OUTMUX1_MASK));
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX2_MASK) | (((u32)MCDE_CH_A_MSB << MCDE_CFG_OUTMUX2_SHIFT)& MCDE_CFG_OUTMUX2_MASK));
		}
		break;
	case CHANNEL_B:
		if (outbpp == MCDE_BPP_1_TO_8)
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_B_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));

		else if (outbpp == MCDE_BPP_16)
		{
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_B_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX1_MASK) | (((u32)MCDE_CH_B_MID << MCDE_CFG_OUTMUX1_SHIFT)& MCDE_CFG_OUTMUX1_MASK));
		}
		else if (outbpp == MCDE_BPP_24)
		{
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_B_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX2_MASK) | (((u32)MCDE_CH_B_MID << MCDE_CFG_OUTMUX2_SHIFT)& MCDE_CFG_OUTMUX2_MASK));
			gpar[chid]->regbase->mcde_cr = ((gpar[chid]->regbase->mcde_cr & ~MCDE_CFG_OUTMUX3_MASK) | (((u32)MCDE_CH_B_MSB << MCDE_CFG_OUTMUX3_SHIFT)& MCDE_CFG_OUTMUX3_MASK));
		}
		break;
	default:
		;
	}
	return(error);
}
#else
mcde_error mcdesetoutmuxconf(mcde_ch_id chid, mcde_out_bpp outbpp)
{
	mcde_error	  error = MCDE_OK;

	switch (chid)
	{
	case CHANNEL_A:
		if (outbpp == MCDE_BPP_1_TO_8)
			gpar[chid]->regbase->mcde_cfg0 = ((gpar[chid]->regbase->mcde_cfg0 & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_A_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));

		else if (outbpp == MCDE_BPP_16)
		{
			gpar[chid]->regbase->mcde_cfg0 = ((gpar[chid]->regbase->mcde_cfg0 & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_A_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_cfg0 = ((gpar[chid]->regbase->mcde_cfg0 & ~MCDE_CFG_OUTMUX1_MASK) | (((u32)MCDE_CH_A_MID << MCDE_CFG_OUTMUX1_SHIFT)& MCDE_CFG_OUTMUX1_MASK));
		}
		else if (outbpp == MCDE_BPP_24)
		{
			gpar[chid]->regbase->mcde_cfg0 = ((gpar[chid]->regbase->mcde_cfg0 & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_A_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_cfg0 = ((gpar[chid]->regbase->mcde_cfg0 & ~MCDE_CFG_OUTMUX1_MASK) | (((u32)MCDE_CH_A_MID << MCDE_CFG_OUTMUX1_SHIFT)& MCDE_CFG_OUTMUX1_MASK));
			gpar[chid]->regbase->mcde_cfg0 = ((gpar[chid]->regbase->mcde_cfg0 & ~MCDE_CFG_OUTMUX2_MASK) | (((u32)MCDE_CH_A_MSB << MCDE_CFG_OUTMUX2_SHIFT)& MCDE_CFG_OUTMUX2_MASK));
		}
		break;
	case CHANNEL_B:
		if (outbpp == MCDE_BPP_1_TO_8)
			gpar[chid]->regbase->mcde_cfg0 = ((gpar[chid]->regbase->mcde_cfg0 & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_B_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));

		else if (outbpp == MCDE_BPP_16)
		{
			gpar[chid]->regbase->mcde_cfg0 = ((gpar[chid]->regbase->mcde_cfg0 & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_B_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_cfg0 = ((gpar[chid]->regbase->mcde_cfg0 & ~MCDE_CFG_OUTMUX1_MASK) | (((u32)MCDE_CH_B_MID << MCDE_CFG_OUTMUX1_SHIFT)& MCDE_CFG_OUTMUX1_MASK));
		}
		else if (outbpp == MCDE_BPP_24)
		{
			gpar[chid]->regbase->mcde_cfg0 = ((gpar[chid]->regbase->mcde_cfg0 & ~MCDE_CFG_OUTMUX0_MASK) | (((u32)MCDE_CH_B_LSB << MCDE_CFG_OUTMUX0_SHIFT)& MCDE_CFG_OUTMUX0_MASK));
			gpar[chid]->regbase->mcde_cfg0 = ((gpar[chid]->regbase->mcde_cfg0 & ~MCDE_CFG_OUTMUX2_MASK) | (((u32)MCDE_CH_B_MID << MCDE_CFG_OUTMUX2_SHIFT)& MCDE_CFG_OUTMUX2_MASK));
			gpar[chid]->regbase->mcde_cfg0 = ((gpar[chid]->regbase->mcde_cfg0 & ~MCDE_CFG_OUTMUX3_MASK) | (((u32)MCDE_CH_B_MSB << MCDE_CFG_OUTMUX3_SHIFT)& MCDE_CFG_OUTMUX3_MASK));
		}
		break;
	case CHANNEL_C0:
		gpar[chid]->regbase->mcde_cfg0 |= 0x5000;//0x5e145000; /** This code needs to be modified */
		break;
	case CHANNEL_C1:
		gpar[chid]->regbase->mcde_cfg0 |= 0x5e145000; /** This code needs to be modified */
		break;
	default:
		;
	}
	return(error);
}
#endif
mcde_error mcderesetextsrcovrlay(mcde_ch_id chid)
{
	struct mcde_ext_src_reg  *ext_src;
	struct mcde_ovl_reg  *ovr_config;
	mcde_error retVal = MCDE_OK;

	/** point to the correct ext src register */
	ext_src = (struct mcde_ext_src_reg *) (gpar[chid]->extsrc_regbase[gpar[chid]->mcde_cur_ovl_bmp]);

	/** reset ext src address registers, src conf register and src ctrl register to default value */
	ext_src->mcde_extsrc_a0 = 0x0;
	ext_src->mcde_extsrc_a1 = 0x0;
	ext_src->mcde_extsrc_a2 = 0x0;
	ext_src->mcde_extsrc_conf = 0xA04;
	ext_src->mcde_extsrc_cr = 0x0;

	/** point to the correct overlay register */
	ovr_config = (struct mcde_ovl_reg *) (gpar[chid]->ovl_regbase[gpar[chid]->mcde_cur_ovl_bmp]);

	/** reset overlay conf and overlay control register to default values...this also disables overlay */
	ovr_config->mcde_ovl_conf = 0x0;
	ovr_config->mcde_ovl_conf2 = 0x00200001;
	ovr_config->mcde_ovl_ljinc = 0x0;
	ovr_config->mcde_ovl_cr = 0x22B00000;

	return retVal;
}

#ifdef _cplusplus
}
#endif /* _cplusplus */

#endif	/* CONFIG_MCDE_ENABLE_FEATURE_HW_V1_SUPPORT */
