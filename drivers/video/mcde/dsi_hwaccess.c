/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License terms:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

/** Linux include files:charachter driver and memory functions  */


#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/mcde_common.h>

extern struct mcdefb_info *gpar[];
dsi_error dsisetlinkstate(dsi_link link, dsi_link_state linkState, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;

      if (link > DSI_LINK2)
    {
        return(DSI_INVALID_PARAMETER);
    }

	gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl & ~DSI_MCTL_LINKEN_MASK) |((u32) linkState & DSI_MCTL_LINKEN_MASK));
    gpar[chid]->dsi_lnk_context.dsi_link_state = linkState;

    return(dsi_error);
}

dsi_error dsisetInterface1mode(dsi_link link, dsi_interface_mode mode, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl &~DSI_MCTL_INTERFACE1_MODE_MASK) |(((u32) mode << DSI_MCTL_INTERFACE1_MODE_SHIFT) & DSI_MCTL_INTERFACE1_MODE_MASK) );

    gpar[chid]->dsi_lnk_context.dsi_if_mode = mode;

    return(dsi_error);

}

dsi_error dsisetVSG(dsi_link link, dsi_vsg_ctrl state, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;

	gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl &~DSI_MCTL_VID_EN_MASK) |(((u32) state << DSI_MCTL_VID_EN_SHIFT ) & DSI_MCTL_VID_EN_MASK) );

	return(dsi_error);
}

dsi_error dsisetTVG(dsi_link link, dsi_tvg_ctrl state, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;

	if ((gpar[chid]->dsi_lnk_context.dsi_if_mode == 1) || (gpar[chid]->dsi_lnk_context.dsi_if1_state == 1))
	{
	return(DSI_REQUEST_NOT_APPLICABLE);
    }
	gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl &~DSI_MCTL_TVG_SEL_MASK) |(((u32) state << DSI_MCTL_TVG_SEL_SHIFT ) & DSI_MCTL_TVG_SEL_MASK) );

    return(dsi_error);
}
dsi_error dsisetTBG(dsi_link link, dsi_tbg_ctrl state, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;

	gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl &~DSI_MCTL_TBG_SEL_MASK) |(((u32) state << DSI_MCTL_TBG_SEL_SHIFT ) & DSI_MCTL_TBG_SEL_MASK) );

    return(dsi_error);
}

dsi_error dsireadset(dsi_link link, dsi_rd_ctrl state, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;


	gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl &~DSI_MCTL_READEN_MASK) |(((u32) state << DSI_MCTL_READEN_SHIFT ) & DSI_MCTL_READEN_MASK) );


    return(dsi_error);
}

dsi_error dsisetBTAmode(dsi_link link, dsi_bta_mode mode, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl &~DSI_MCTL_BTAEN_MASK) |(((u32) mode << DSI_MCTL_BTAEN_SHIFT ) & DSI_MCTL_BTAEN_MASK) );

    return(dsi_error);
}
dsi_error dsisetTE(dsi_link link, dsi_te_en tearing, dsi_te_ctrl state, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;


    switch(tearing.te_sel)
    {
        case DSI_REG_TE:
			gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl &~DSI_REG_TE_MASK) |(((u32) state << DSI_REG_TE_SHIFT ) & DSI_REG_TE_MASK) );
			break;

        case DSI_IF_TE:
            if(tearing.interface == DSI_INTERFACE_1)
                gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl &~DSI_IF1_TE_MASK) |(((u32) state << DSI_IF1_TE_SHIFT ) & DSI_IF1_TE_MASK) );
            else if(tearing.interface == DSI_INTERFACE_2)
                gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl &~DSI_IF2_TE_MASK) |(((u32) state << DSI_IF2_TE_SHIFT ) & DSI_IF2_TE_MASK) );
            else
                dsi_error = DSI_INVALID_PARAMETER ;
            break;

        default:
            dsi_error = DSI_INVALID_PARAMETER ;
    }


    return(dsi_error);
}

dsi_error dsisetdispECCGenmode(dsi_link link, dsi_ecc_gen_mode mode, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl &~DSI_MCTL_DISPECCGEN_MASK) |(((u32) mode << DSI_MCTL_DISPECCGEN_SHIFT ) & DSI_MCTL_DISPECCGEN_MASK) );

    return(dsi_error);
}

dsi_error dsisetdispCHKSUMGenmode(dsi_link link, dsi_checksum_gen_mode mode, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl &~DSI_MCTL_DISPCHECKSUMGEN_MASK) |(((u32) mode << DSI_MCTL_DISPCHECKSUMGEN_SHIFT ) & DSI_MCTL_DISPCHECKSUMGEN_MASK) );

    return(dsi_error);
}

dsi_error dsisetdispEOTGenmode(dsi_link link, dsi_eot_gen_mode mode, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl &~DSI_MCTL_DISPEOTGEN_MASK) |(((u32) mode << DSI_MCTL_DISPEOTGEN_SHIFT ) & DSI_MCTL_DISPEOTGEN_MASK) );

    return(dsi_error);
}

dsi_error dsisetdispHOSTEOTGenmode(dsi_link link, dsi_host_eot_gen_mode mode, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_data_ctl &~DSI_MCTL_HOSTEOTGEN_MASK) |(((u32) mode << DSI_MCTL_HOSTEOTGEN_SHIFT ) & DSI_MCTL_HOSTEOTGEN_MASK) );

    return(dsi_error);
}

dsi_error dsisetPLLcontrol(dsi_link link, mcde_ch_id chid, dsi_pll_ctl pll_ctl)
{
     dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->mctl_pll_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_pll_ctl &~DSI_PLL_OUT_SEL_MASK) |(((u32) pll_ctl.pll_out_sel << DSI_PLL_OUT_SEL_SHIFT ) & DSI_PLL_OUT_SEL_MASK) );

    gpar[chid]->dsi_lnk_registers[link]->mctl_pll_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_pll_ctl &~DSI_PLL_DIV_MASK) |(((u32) pll_ctl.division_ratio << DSI_PLL_DIV_SHIFT ) & DSI_PLL_DIV_MASK) );

    gpar[chid]->dsi_lnk_registers[link]->mctl_pll_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_pll_ctl &~DSI_PLL_IN_SEL_MASK) |(((u32) pll_ctl.pll_in_sel << DSI_PLL_IN_SEL_SHIFT ) & DSI_PLL_IN_SEL_MASK) );

    gpar[chid]->dsi_lnk_registers[link]->mctl_pll_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_pll_ctl &~DSI_PLL_MASTER_MASK) |(((u32) pll_ctl.pll_master << DSI_PLL_MASTER_SHIFT ) & DSI_PLL_MASTER_MASK) );

    gpar[chid]->dsi_lnk_registers[link]->mctl_pll_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_pll_ctl &~DSI_PLL_MULT_MASK) |((u32) pll_ctl.multiplier & DSI_PLL_MASTER_MASK) );

    return(dsi_error);
}
dsi_error dsisetInterface(dsi_link link, mcde_ch_id chid, dsi_if_state state, dsi_interface interface)
{
     dsi_error    dsi_error = DSI_OK;

    switch(interface)
    {
    case DSI_INTERFACE_1:
        gpar[chid]->dsi_lnk_registers[link]->mctl_main_en = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_en &~DSI_IF1_EN_MASK) |(((u32) state << DSI_IF1_EN_SHIFT)& DSI_IF1_EN_MASK) );
        break;

    case DSI_INTERFACE_2:
        gpar[chid]->dsi_lnk_registers[link]->mctl_main_en = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_en &~DSI_IF2_EN_MASK) |(((u32) state << DSI_IF2_EN_SHIFT)& DSI_IF2_EN_MASK) );
        break;

    default:
        dsi_error = DSI_INVALID_PARAMETER;
    }

    return(dsi_error);
}
dsi_error dsisetInterfaceInLpm(dsi_link link, mcde_ch_id chid, dsi_interface_mode_type modType, dsi_interface interface)
{
     dsi_error    dsi_error = DSI_OK;
     dsi_if_state ifState;

    if ( modType == DSI_CLK_LANE_LPM)
	ifState = DSI_IF_ENABLE;
    else
	ifState = DSI_IF_DISABLE;

    switch(interface)
    {
    case DSI_INTERFACE_1:
        gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl = ((gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl &~DSI_IF1_LPM_EN_MASK) |(((u32) ifState << DSI_IF1_LPM_EN_MASK_SHIFT)& DSI_IF1_LPM_EN_MASK) );
        break;

    case DSI_INTERFACE_2:
        gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl = ((gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl &~DSI_IF2_LPM_EN_MASK) |(((u32) ifState << DSI_IF2_LPM_EN_MASK_SHIFT)& DSI_IF2_LPM_EN_MASK) );
        break;

    default:
        dsi_error = DSI_INVALID_PARAMETER;
    }

    return(dsi_error);
}
dsi_error dsisetstallsignal(dsi_link link, mcde_ch_id chid, dsi_stall_signal_state state, dsi_interface_mode mode)
{
    dsi_error    dsi_error = DSI_OK;

    if(gpar[chid]->dsi_lnk_context.dsi_int_mode != DSI_INT_MODE_ENABLE)
        return(DSI_REQUEST_NOT_APPLICABLE);
    switch(mode)
    {
    case DSI_VIDEO_MODE:
        gpar[chid]->dsi_lnk_registers[link]->int_vid_gnt = ((gpar[chid]->dsi_lnk_registers[link]->int_vid_gnt &~DSI_IF_STALL_MASK) |((u32) state & DSI_IF_STALL_MASK) );

        break;

    case DSI_COMMAND_MODE:
        gpar[chid]->dsi_lnk_registers[link]->int_cmd_gnt = ((gpar[chid]->dsi_lnk_registers[link]->int_cmd_gnt &~DSI_IF_STALL_MASK) |((u32) state & DSI_IF_STALL_MASK) );

        break;

    default:
        dsi_error = DSI_INVALID_PARAMETER;

    }

    return(dsi_error);


}
dsi_error dsisetdirectcmdsettings(dsi_link link, mcde_ch_id chid, dsi_cmd_main_setting cmd_settings)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings &~DSI_CMD_NAT_MASK) |((u32) cmd_settings.cmd_nature & DSI_CMD_NAT_MASK) );

    gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings &~DSI_CMD_LONGNOTSHORT_MASK) |(((u32) cmd_settings.packet_type << DSI_CMD_LONGNOTSHORT_SHIFT)& DSI_CMD_LONGNOTSHORT_MASK) );

    gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings &~DSI_CMD_HEAD_MASK) |(((u32) cmd_settings.cmd_header << DSI_CMD_HEAD_SHIFT)& DSI_CMD_HEAD_MASK) );

    gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings &~DSI_CMD_ID_MASK) |(((u32) cmd_settings.cmd_id << DSI_CMD_ID_SHIFT)& DSI_CMD_ID_MASK) );

    gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings &~DSI_CMD_SIZE_MASK) |(((u32) cmd_settings.cmd_size << DSI_CMD_SIZE_SHIFT)& DSI_CMD_SIZE_MASK) );

    gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings &~DSI_CMD_LP_EN_MASK) |(((u32) cmd_settings.cmd_lp_enable << DSI_CMD_LP_EN_SHIFT)& DSI_CMD_LP_EN_MASK) );

    gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings &~DSI_TRIGGER_VAL_MASK) |(((u32) cmd_settings.cmd_trigger_val << DSI_TRIGGER_VAL_SHIFT)& DSI_TRIGGER_VAL_MASK) );

    return(dsi_error);
}

dsi_error dsisetTEtimeout(dsi_link link, mcde_ch_id chid, u32 te_timeout)
{
    dsi_error    dsi_error = DSI_OK;
    u8           te_lowerbits ;
    dsi_te_timeout te_upperbits;

    if(te_timeout/256 == 1)
    {
         te_lowerbits = te_timeout % 256;
         te_upperbits = DSI_TE_256;
    }
    else if(te_timeout/512 == 1)
    {
        te_lowerbits = te_timeout % 512;
        te_upperbits = DSI_TE_512;
    }
    else if (te_timeout/1024 == 1)
    {
        te_lowerbits = te_timeout % 1024;
        te_upperbits = DSI_TE_1024;
    }
    else
    {
        te_lowerbits = te_timeout % 2048;
        te_upperbits = DSI_TE_2048;
    }

     gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl = ((gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl &~DSI_TE_LOWERBIT_MASK) |(((u32) te_lowerbits << DSI_TE_LOWERBIT_SHIFT)& DSI_TE_LOWERBIT_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl = ((gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl &~DSI_TE_UPPERBIT_MASK) |(((u32) te_upperbits << DSI_TE_UPPERBIT_SHIFT)& DSI_TE_UPPERBIT_MASK) );

     return(dsi_error);
}

dsi_error dsisetPLLmode(dsi_link link, mcde_ch_id chid, dsi_pll_mode mode)
{
    dsi_error    dsi_error = DSI_OK;


    gpar[chid]->dsi_lnk_registers[link]->mctl_main_en = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_en &~DSI_PLL_START_MASK) |((u32) mode & DSI_PLL_START_MASK) );

    return(dsi_error);
}
dsi_error dsisetlanestate(dsi_link link, mcde_ch_id chid, dsi_lane_state mode, dsi_lane lane)
{
    dsi_error    dsi_error = DSI_OK;

    if ( (lane > DSI_DATA_LANE2))
    {
        return(DSI_INVALID_PARAMETER);
    }

    switch(lane)
    {
    case DSI_CLK_LANE:
        gpar[chid]->dsi_lnk_registers[link]->mctl_main_en = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_en &~DSI_CKLANE_EN_MASK) |(((u32) mode << DSI_CKLANE_EN_SHIFT)& DSI_CKLANE_EN_MASK) );
        break;

    case DSI_DATA_LANE1:
        gpar[chid]->dsi_lnk_registers[link]->mctl_main_en = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_en &~DSI_DAT1_EN_MASK) |(((u32) mode << DSI_DAT1_EN_SHIFT)& DSI_DAT1_EN_MASK) );
        break;

    case DSI_DATA_LANE2:
	 gpar[chid]->dsi_lnk_registers[link]->mctl_main_phy_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_phy_ctl &~DSI_LANE2_EN_MASK) |((u32) mode & DSI_LANE2_EN_MASK) );
         gpar[chid]->dsi_lnk_registers[link]->mctl_main_en = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_en &~DSI_DAT2_EN_MASK) |(((u32) mode << DSI_DAT2_EN_SHIFT)& DSI_DAT2_EN_MASK) );
        break;

    default:
        dsi_error = DSI_INVALID_PARAMETER;
    }

    return(dsi_error);

}

dsi_error dsisetlanetoULPM(dsi_link link, mcde_ch_id chid, dsi_lane_mode mode, dsi_lane lane)
{
    dsi_error    dsi_error = DSI_OK;

    if ( (lane > DSI_DATA_LANE2))
    {
        return(DSI_INVALID_PARAMETER);
    }

    switch(lane)
    {
    case DSI_CLK_LANE:
	/** enable the ULPM mode for clock lane*/
	gpar[chid]->dsi_lnk_registers[link]->mctl_main_phy_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_phy_ctl &~DSI_CLK_ULPM_EN_MASK) |(((u32) mode << DSI_CLK_ULPM_EN_SHIFT ) & DSI_CLK_ULPM_EN_MASK) );
	/** start the clock lane in ULPM mode*/
	gpar[chid]->dsi_lnk_registers[link]->mctl_main_en = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_en &~DSI_CLK_ULPM_MASK) |(((u32) mode << DSI_CLK_ULPM_SHIFT)& DSI_CLK_ULPM_MASK) );
        break;

    case DSI_DATA_LANE1:
	/** enable the ULPM mode for data1 lane*/
	gpar[chid]->dsi_lnk_registers[link]->mctl_main_phy_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_phy_ctl &~DSI_DAT1_ULPM_EN_MASK) |(((u32) mode << DSI_DAT1_ULPM_EN_SHIFT ) & DSI_DAT1_ULPM_EN_MASK) );
	/** start the data1 lane in ULPM mode*/
	gpar[chid]->dsi_lnk_registers[link]->mctl_main_en = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_en &~DSI_DAT1_ULPM_MASK) |(((u32) mode << DSI_DAT1_ULPM_SHIFT)& DSI_DAT1_ULPM_MASK) );
        break;

    case DSI_DATA_LANE2:
	/** enable the ULPM mode for data2 lane*/
	gpar[chid]->dsi_lnk_registers[link]->mctl_main_phy_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_phy_ctl &~DSI_DAT2_ULPM_EN_MASK) |(((u32) mode << DSI_DAT2_ULPM_EN_SHIFT ) & DSI_DAT2_ULPM_EN_MASK) );
	/** start the data2 lane in ULPM mode*/
	gpar[chid]->dsi_lnk_registers[link]->mctl_main_en = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_en &~DSI_DAT2_ULPM_MASK) |(((u32) mode << DSI_DAT2_ULPM_SHIFT)& DSI_DAT2_ULPM_MASK) );
	break;

    default:
        dsi_error = DSI_INVALID_PARAMETER;
    }
    return dsi_error;
}

dsi_error dsisetCLKHSsendingmode(dsi_link link, dsi_clk_continious_hs_mode mode, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->mctl_main_phy_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_phy_ctl &~DSI_CLK_CONTINUOUS_MASK) |(((u32) mode << DSI_CLK_CONTINUOUS_SHIFT ) & DSI_CLK_CONTINUOUS_MASK) );

    return(dsi_error);
}

dsi_error dsisetwaitbursttime(dsi_link link, u8 delay, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->mctl_main_phy_ctl = ((gpar[chid]->dsi_lnk_registers[link]->mctl_main_phy_ctl &~DSI_WAIT_BURST_MASK) |(((u32) delay << DSI_WAIT_BURST_SHIFT ) & DSI_WAIT_BURST_MASK) );

    return(dsi_error);
}

dsi_error dsigetlinkstatus(dsi_link link, mcde_ch_id chid, u8 *p_status)
{
    dsi_error    dsi_error = DSI_OK;

    *p_status = (u8)(gpar[chid]->dsi_lnk_registers[link]->mctl_main_sts & DSI_MAIN_STS_MASK);

    return(dsi_error);
}


dsi_error dsisetvideomodectrl(dsi_link link, mcde_ch_id chid, dsi_vid_main_ctl vid_ctl)
{
     dsi_error    dsi_error = DSI_OK;

	 gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl = ((gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl &~DSI_STOP_MODE_MASK) |(((u32) vid_ctl.vid_stop_mode << DSI_STOP_MODE_SHIFT)& DSI_STOP_MODE_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl = ((gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl &~DSI_VID_ID_MASK) |(((u32) vid_ctl.vid_id << DSI_VID_ID_SHIFT)& DSI_VID_ID_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl = ((gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl &~DSI_HEADER_MASK) |(((u32) vid_ctl.header << DSI_HEADER_SHIFT)& DSI_HEADER_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl = ((gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl &~DSI_PIXEL_MODE_MASK) |(((u32) vid_ctl.vid_pixel_mode << DSI_PIXEL_MODE_SHIFT)& DSI_PIXEL_MODE_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl = ((gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl &~DSI_BURST_MODE_MASK) |(((u32) vid_ctl.vid_burst_mode << DSI_BURST_MODE_SHIFT)& DSI_VID_ID_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl = ((gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl &~DSI_SYNC_PULSE_ACTIVE_MASK) |(((u32) vid_ctl.sync_pulse_active << DSI_SYNC_PULSE_ACTIVE_SHIFT)& DSI_VID_ID_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl = ((gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl &~DSI_SYNC_PULSE_HORIZONTAL_MASK) |(((u32) vid_ctl.sync_pulse_horizontal << DSI_SYNC_PULSE_HORIZONTAL_SHIFT)& DSI_SYNC_PULSE_HORIZONTAL_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl = ((gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl &~DSI_BLKLINE_MASK) |(((u32) vid_ctl.blkline_mode << DSI_BLKLINE_SHIFT)& DSI_BLKLINE_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl = ((gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl &~DSI_BLKEOL_MASK) |(((u32) vid_ctl.blkeol_mode << DSI_BLKEOL_SHIFT)& DSI_BLKEOL_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl = ((gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl &~DSI_RECOVERY_MODE_MASK) |(((u32) vid_ctl.recovery_mode << DSI_RECOVERY_MODE_SHIFT)& DSI_RECOVERY_MODE_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl = ((gpar[chid]->dsi_lnk_registers[link]->vid_main_ctl &~DSI_START_MODE_MASK) |((u32) vid_ctl.vid_start_mode & DSI_START_MODE_MASK) );

     return(dsi_error);
}

dsi_error dsisetvideoXYsize(dsi_link link, mcde_ch_id chid, dsi_img_horizontal_size vid_hsize, dsi_img_vertical_size vid_vsize)
{
    dsi_error    dsi_error = DSI_OK;

	// X size settings
	gpar[chid]->dsi_lnk_registers[link]->vid_hsize1 = ((gpar[chid]->dsi_lnk_registers[link]->vid_hsize1 &~DSI_HBP_LENGTH_MASK) |(((u32) vid_hsize.hbp_length << DSI_HBP_LENGTH_SHIFT)& DSI_HBP_LENGTH_MASK) );
	gpar[chid]->dsi_lnk_registers[link]->vid_hsize1 = ((gpar[chid]->dsi_lnk_registers[link]->vid_hsize1 &~DSI_HFP_LENGTH_MASK) |(((u32) vid_hsize.hfp_length << DSI_HFP_LENGTH_SHIFT)& DSI_HFP_LENGTH_MASK) );
	gpar[chid]->dsi_lnk_registers[link]->vid_hsize1 = ((gpar[chid]->dsi_lnk_registers[link]->vid_hsize1 &~DSI_HSA_LENGTH_MASK) |((u32) vid_hsize.hsa_length & DSI_HSA_LENGTH_MASK) );
	gpar[chid]->dsi_lnk_registers[link]->vid_hsize2 = ((gpar[chid]->dsi_lnk_registers[link]->vid_hsize2 &~DSI_RGB_SIZE_MASK) |((u32) vid_hsize.rgb_size & DSI_RGB_SIZE_MASK) );

	//Y size settings
	gpar[chid]->dsi_lnk_registers[link]->vid_vsize = ((gpar[chid]->dsi_lnk_registers[link]->vid_vsize &~DSI_VACT_LENGTH_MASK) |(((u32) vid_vsize.vact_length << DSI_VACT_LENGTH_SHIFT)& DSI_VACT_LENGTH_MASK) );
    gpar[chid]->dsi_lnk_registers[link]->vid_vsize = ((gpar[chid]->dsi_lnk_registers[link]->vid_vsize &~DSI_VFP_LENGTH_MASK) |(((u32) vid_vsize.vfp_length << DSI_VFP_LENGTH_SHIFT)& DSI_VFP_LENGTH_MASK) );
    gpar[chid]->dsi_lnk_registers[link]->vid_vsize = ((gpar[chid]->dsi_lnk_registers[link]->vid_vsize &~DSI_VBP_LENGTH_MASK) |(((u32) vid_vsize.vbp_length << DSI_VBP_LENGTH_SHIFT)& DSI_VBP_LENGTH_MASK) );
    gpar[chid]->dsi_lnk_registers[link]->vid_vsize = ((gpar[chid]->dsi_lnk_registers[link]->vid_vsize &~DSI_VSA_LENGTH_MASK) |((u32) vid_vsize.vact_length & DSI_VSA_LENGTH_MASK) );

	return(dsi_error);
}


dsi_error dsisetvideopos(dsi_link link, mcde_ch_id chid, dsi_img_position vid_pos)
{
     dsi_error    dsi_error = DSI_OK;


     gpar[chid]->dsi_lnk_registers[link]->vid_vpos = ((gpar[chid]->dsi_lnk_registers[link]->vid_vpos &~DSI_LINE_POS_MASK) |(	(u32) vid_pos.line_pos & DSI_LINE_POS_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->vid_vpos = ((gpar[chid]->dsi_lnk_registers[link]->vid_vpos &~DSI_LINE_VAL_MASK) |(((u32) vid_pos.line_val << DSI_LINE_VAL_SHIFT)& DSI_LINE_VAL_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->vid_hpos = ((gpar[chid]->dsi_lnk_registers[link]->vid_hpos &~DSI_HORI_POS_MASK) |((u32) vid_pos.horizontal_pos & DSI_HORI_POS_MASK) );

     gpar[chid]->dsi_lnk_registers[link]->vid_hpos = ((gpar[chid]->dsi_lnk_registers[link]->vid_hpos &~DSI_HORI_VAL_MASK) |(((u32) vid_pos.horizontal_val << DSI_HORI_VAL_SHIFT)& DSI_HORI_VAL_MASK) );

     return(dsi_error);
}

dsi_error dsigetvideomodestatus(dsi_link link, mcde_ch_id chid, u16 *p_vid_mode_sts)
{
     dsi_error    dsi_error = DSI_OK;

     *p_vid_mode_sts = (u16)(gpar[chid]->dsi_lnk_registers[link]->vid_mode_sts & DSI_VID_MODE_STS_MASK);

     return(dsi_error);
}

dsi_error dsisetblankingctrl(dsi_link link, mcde_ch_id chid, dsi_vid_blanking blksize)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->vid_blksize1 = ((gpar[chid]->dsi_lnk_registers[link]->vid_blksize1 &~DSI_BLKEOL_PCK_MASK) |(((u32) blksize.blkeol_pck << DSI_BLKEOL_PCK_SHIFT)& DSI_BLKEOL_PCK_MASK) );
    gpar[chid]->dsi_lnk_registers[link]->vid_blksize1 = ((gpar[chid]->dsi_lnk_registers[link]->vid_blksize1 &~DSI_BLKLINE_EVENT_MASK) |((u32) blksize.blkline_event_pck & DSI_BLKLINE_EVENT_MASK) );
    gpar[chid]->dsi_lnk_registers[link]->vid_blksize2 = ((gpar[chid]->dsi_lnk_registers[link]->vid_blksize2 &~DSI_BLKLINE_PULSE_PCK_MASK) |((u32) blksize.blkline_pulse_pck & DSI_BLKLINE_PULSE_PCK_MASK) );
    gpar[chid]->dsi_lnk_registers[link]->vid_pck_time = ((gpar[chid]->dsi_lnk_registers[link]->vid_pck_time &~DSI_VERT_BLANK_DURATION_MASK) |(((u32) blksize.vert_balnking_duration << DSI_VERT_BLANK_DURATION_SHIFT)& DSI_VERT_BLANK_DURATION_MASK) );
    gpar[chid]->dsi_lnk_registers[link]->vid_pck_time = ((gpar[chid]->dsi_lnk_registers[link]->vid_pck_time &~DSI_BLKEOL_DURATION_MASK) |((u32) blksize.blkeol_duration & DSI_BLKEOL_DURATION_MASK) );

    return(dsi_error);
}


dsi_error dsisetviderrorcolor(dsi_link link, mcde_ch_id chid, dsi_vid_err_color err_color)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->vid_err_color = ((gpar[chid]->dsi_lnk_registers[link]->vid_err_color &~DSI_COL_GREEN_MASK) |(((u32) err_color.col_green << DSI_COL_GREEN_SHIFT)& DSI_COL_GREEN_MASK) );
    gpar[chid]->dsi_lnk_registers[link]->vid_err_color = ((gpar[chid]->dsi_lnk_registers[link]->vid_err_color &~DSI_COL_BLUE_MASK) |(((u32) err_color.col_blue << DSI_COL_BLUE_SHIFT)& DSI_COL_BLUE_MASK) );
    gpar[chid]->dsi_lnk_registers[link]->vid_err_color = ((gpar[chid]->dsi_lnk_registers[link]->vid_err_color &~DSI_PAD_VAL_MASK) |(((u32) err_color.pad_val << DSI_PAD_VAL_SHIFT)& DSI_PAD_VAL_MASK) );
    gpar[chid]->dsi_lnk_registers[link]->vid_err_color = ((gpar[chid]->dsi_lnk_registers[link]->vid_err_color &~DSI_COL_RED_MASK) |((u32)err_color.col_red & DSI_COL_RED_MASK) );

    return(dsi_error);
}

dsi_error dsisetVCActrl(dsi_link link, mcde_ch_id chid, dsi_vca_setting vca_setting)
{
     dsi_error    dsi_error = DSI_OK;

     gpar[chid]->dsi_lnk_registers[link]->vid_vca_setting1 = ((gpar[chid]->dsi_lnk_registers[link]->vid_vca_setting1 &~DSI_BURST_LP_MASK) |(((u32) vca_setting.burst_lp << DSI_BURST_LP_SHIFT)& DSI_BURST_LP_MASK) );
     gpar[chid]->dsi_lnk_registers[link]->vid_vca_setting2 = ((gpar[chid]->dsi_lnk_registers[link]->vid_vca_setting1 &~DSI_MAX_BURST_LIMIT_MASK) |((u32) vca_setting.max_burst_limit & DSI_MAX_BURST_LIMIT_MASK) );
     gpar[chid]->dsi_lnk_registers[link]->vid_vca_setting1 = ((gpar[chid]->dsi_lnk_registers[link]->vid_vca_setting2 &~DSI_MAX_LINE_LIMIT_MASK) |(((u32) vca_setting.max_line_limit << DSI_MAX_LINE_LIMIT_SHIFT)& DSI_MAX_LINE_LIMIT_MASK) );
     gpar[chid]->dsi_lnk_registers[link]->vid_vca_setting2 = ((gpar[chid]->dsi_lnk_registers[link]->vid_vca_setting2 &~DSI_EXACT_BURST_LIMIT_MASK) |((u32) vca_setting.exact_burst_limit & DSI_EXACT_BURST_LIMIT_MASK) );

     return(dsi_error);

}
dsi_error dsisetTVGctrl(dsi_link link, mcde_ch_id chid, dsi_tvg_control tvg_control)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->tvg_ctl = ((gpar[chid]->dsi_lnk_registers[link]->tvg_ctl &~DSI_TVG_STRIPE_MASK) |(((u32) tvg_control.tvg_stripe_size << DSI_TVG_STRIPE_SHIFT)& DSI_TVG_STRIPE_MASK) );

    gpar[chid]->dsi_lnk_registers[link]->tvg_ctl = ((gpar[chid]->dsi_lnk_registers[link]->tvg_ctl &~DSI_TVG_MODE_MASK) |(((u32) tvg_control.tvg_mode << DSI_TVG_MODE_SHIFT)& DSI_TVG_MODE_MASK) );

    gpar[chid]->dsi_lnk_registers[link]->tvg_ctl = ((gpar[chid]->dsi_lnk_registers[link]->tvg_ctl &~DSI_TVG_STOPMODE_MASK) |(((u32) tvg_control.stop_mode << DSI_TVG_STOPMODE_SHIFT)& DSI_TVG_STOPMODE_MASK) );

    return(dsi_error);
}

dsi_error dsiTVGsetstate(dsi_link link, mcde_ch_id chid, dsi_tvg_ctrl_state state)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->tvg_ctl = ((gpar[chid]->dsi_lnk_registers[link]->tvg_ctl &~DSI_TVG_RUN_MASK) |((u32) state & DSI_TVG_RUN_MASK) );

    return(dsi_error);
}



dsi_error dsisetTVGimagesize(dsi_link link, mcde_ch_id chid, dsi_tvg_img_size img_size)
{
    dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->tvg_img_size = ((gpar[chid]->dsi_lnk_registers[link]->tvg_img_size &~DSI_TVG_NBLINE_MASK) |(((u32) img_size.tvg_nbline << DSI_TVG_NBLINE_SHIFT)& DSI_TVG_NBLINE_MASK) );

    gpar[chid]->dsi_lnk_registers[link]->tvg_img_size = ((gpar[chid]->dsi_lnk_registers[link]->tvg_img_size &~DSI_TVG_LINE_SIZE_MASK) |((u32) img_size.tvg_line_size & DSI_TVG_LINE_SIZE_MASK) );

    return(dsi_error);
}

dsi_error dsisetTVGcolor(dsi_link link, mcde_ch_id chid, dsi_color_type color_type,dsi_frame_color color)
{
    dsi_error    dsi_error = DSI_OK;

    if(color_type == DSI_TVG_COLOR1)
    {
        gpar[chid]->dsi_lnk_registers[link]->tvg_color1 = ((gpar[chid]->dsi_lnk_registers[link]->tvg_color1 &~DSI_COL_RED_MASK) |((u32) color.col_red & DSI_COL_RED_MASK) );

        gpar[chid]->dsi_lnk_registers[link]->tvg_color1 = ((gpar[chid]->dsi_lnk_registers[link]->tvg_color1 &~DSI_COL_GREEN_MASK) |(((u32) color.col_green << DSI_COL_GREEN_SHIFT)& DSI_COL_GREEN_MASK) );

        gpar[chid]->dsi_lnk_registers[link]->tvg_color1 = ((gpar[chid]->dsi_lnk_registers[link]->tvg_color1 &~DSI_COL_BLUE_MASK) |(((u32) color.col_blue << DSI_COL_BLUE_SHIFT)& DSI_COL_BLUE_MASK) );
    }
    else if (color_type == DSI_TVG_COLOR2)
    {
        gpar[chid]->dsi_lnk_registers[link]->tvg_color2 = ((gpar[chid]->dsi_lnk_registers[link]->tvg_color2 &~DSI_COL_RED_MASK) |((u32) color.col_red & DSI_COL_RED_MASK) );

        gpar[chid]->dsi_lnk_registers[link]->tvg_color2 = ((gpar[chid]->dsi_lnk_registers[link]->tvg_color2 &~DSI_COL_GREEN_MASK) |(((u32) color.col_green << DSI_COL_GREEN_SHIFT)& DSI_COL_GREEN_MASK) );

        gpar[chid]->dsi_lnk_registers[link]->tvg_color2 = ((gpar[chid]->dsi_lnk_registers[link]->tvg_color2 &~DSI_COL_BLUE_MASK) |(((u32) color.col_blue << DSI_COL_BLUE_SHIFT)& DSI_COL_BLUE_MASK) );

    }
    else
        dsi_error = DSI_INVALID_PARAMETER;

    return(dsi_error);
}

dsi_error dsisetARBctrl( dsi_link link, mcde_ch_id chid, dsi_arb_ctl arb_ctl)
{
     dsi_error    dsi_error = DSI_OK;

     gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl = ((gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl &~DSI_ARB_MODE_MASK) |(((u32) arb_ctl.arb_mode << DSI_ARB_MODE_SHIFT)& DSI_ARB_MODE_MASK) );

     if(arb_ctl.arb_mode == DSI_ARB_MODE_FIXED)
         gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl = ((gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl &~DSI_ARB_PRI_MASK) |(((u32) arb_ctl.arb_mode << DSI_ARB_PRI_SHIFT)& DSI_ARB_PRI_MASK) );

     return(dsi_error);

}
dsi_error dsisetpaddingval(dsi_link link, mcde_ch_id chid, u8 padding)
{
     dsi_error    dsi_error = DSI_OK;

     gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl = ((gpar[chid]->dsi_lnk_registers[link]->cmd_mode_ctl &~DSI_FIL_VAL_MASK) |(((u32) padding << DSI_FIL_VAL_SHIFT)& DSI_FIL_VAL_MASK) );

     return(dsi_error);

}
dsi_error dsisetDPHYtimeout(dsi_link link, mcde_ch_id chid, dsi_dphy_timeout timeout)
{
     dsi_error    dsi_error = DSI_OK;

    gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_timeout = ((gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_timeout &~DSI_HSTX_TO_MASK) |(((u32) timeout.hs_tx_timeout << DSI_HSTX_TO_SHIFT ) & DSI_HSTX_TO_MASK) );
    gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_timeout = ((gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_timeout &~DSI_LPRX_TO_MASK) |(((u32) timeout.lp_rx_timeout << DSI_LPRX_TO_SHIFT ) & DSI_LPRX_TO_MASK) );
    gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_timeout = ((gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_timeout &~DSI_CLK_DIV_MASK) |((u32) timeout.clk_div & DSI_CLK_DIV_MASK) );

    return(dsi_error);
}
dsi_error dsisetlaneULPwaittime(dsi_link link, mcde_ch_id chid, dsi_lane lane, u16 timeout)
{
    dsi_error    dsi_error = DSI_OK;

    if ( (lane > DSI_DATA_LANE2) || (link > DSI_LINK2) )
    {
        return(DSI_INVALID_PARAMETER);
    }

    switch(lane)
    {
    case DSI_CLK_LANE:
	gpar[chid]->dsi_lnk_registers[link]->mctl_ulpout_time = ((gpar[chid]->dsi_lnk_registers[link]->mctl_ulpout_time &~DSI_CLK_ULPOUT_MASK) |((u32) timeout & DSI_DATA_ULPOUT_MASK) );
	break;

    case DSI_DATA_LANE1:
    case DSI_DATA_LANE2:
	gpar[chid]->dsi_lnk_registers[link]->mctl_ulpout_time = ((gpar[chid]->dsi_lnk_registers[link]->mctl_ulpout_time &~DSI_DATA_ULPOUT_MASK) |(((u32) timeout << DSI_DATA_ULPOUT_SHIFT ) & DSI_DATA_ULPOUT_MASK) );
	break;

    default:
        dsi_error = DSI_INVALID_PARAMETER;
    }
    return dsi_error;
}
dsi_error dsiset_hs_clock(dsi_link link, struct dsi_dphy_static_conf dphyStaticConf, mcde_ch_id chid)
{
    dsi_error    dsi_error = DSI_OK;

    if (link > DSI_LINK2)
    {
        return(DSI_INVALID_PARAMETER);
    }
	gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static = ((gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static & ~DSIMCTL_DPHY_STATIC_HS_INVERT_CLK) |((u32) dphyStaticConf.clocklanehsinvermode << DSIMCTL_DPHY_STATIC_HS_INVERT_CLK_SHIFT));
	gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static = ((gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static & ~DSIMCTL_DPHY_STATIC_SWAP_PINS_CLK) |((u32) dphyStaticConf.clocklaneswappinmode << DSIMCTL_DPHY_STATIC_SWAP_PINS_CLK_SHIFT));
	gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static = ((gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static & ~DSIMCTL_DPHY_STATIC_HS_INVERT_DAT1) |((u32) dphyStaticConf.datalane1hsinvermode << DSIMCTL_DPHY_STATIC_HS_INVERT_DAT1_SHIFT));
	gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static = ((gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static & ~DSIMCTL_DPHY_STATIC_SWAP_PINS_DAT1) |((u32) dphyStaticConf.datalane1swappinmode << DSIMCTL_DPHY_STATIC_SWAP_PINS_DAT1_SHIFT));
	gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static = ((gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static & ~DSIMCTL_DPHY_STATIC_HS_INVERT_DAT2) |((u32) dphyStaticConf.datalane2hsinvermode << DSIMCTL_DPHY_STATIC_HS_INVERT_DAT2_SHIFT));
	gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static = ((gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static & ~DSIMCTL_DPHY_STATIC_SWAP_PINS_DAT2) |((u32) dphyStaticConf.datalane2swappinmode << DSIMCTL_DPHY_STATIC_SWAP_PINS_DAT2_SHIFT));
	gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static = ((gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_static & ~DSIMCTL_DPHY_STATIC_UI_X4) |((u32) dphyStaticConf.ui_x4 << DSIMCTL_DPHY_STATIC_UI_X4_SHIFT));

    return(dsi_error);
}
u32 dsiclearallstatus(mcde_ch_id chid, dsi_link link)
{
    dsi_error    dsi_error = DSI_OK;

    if (link > DSI_LINK2)
    {
        return(DSI_INVALID_PARAMETER);
    }
    gpar[chid]->dsi_lnk_registers[link]->mctl_main_sts_clr = DSI_SET_ALL_BIT;
    gpar[chid]->dsi_lnk_registers[link]->cmd_mode_sts_clr = DSI_SET_ALL_BIT;
    gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_clr = DSI_SET_ALL_BIT;
    gpar[chid]->dsi_lnk_registers[link]->direct_cmd_rd_sts_clr = DSI_SET_ALL_BIT;
    gpar[chid]->dsi_lnk_registers[link]->vid_mode_sts_clr = DSI_SET_ALL_BIT;
    gpar[chid]->dsi_lnk_registers[link]->tg_sts_clr = DSI_SET_ALL_BIT;
    gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_err_clr = DSI_SET_ALL_BIT;
    return(dsi_error);
}
u32 dsienableirqsrc(mcde_ch_id chid, dsi_link link, dsi_irq_type irq_type,u32 irq_src)
{
    dsi_error    dsi_error = DSI_OK;

    if (link > DSI_LINK2)
    {
        return(DSI_INVALID_PARAMETER);
    }
    switch(irq_type)
    {
    case DSI_IRQ_TYPE_MCTL_MAIN :
       gpar[chid]->dsi_lnk_registers[link]->mctl_main_sts_ctl  |= (u8)irq_src;
       break;

    case DSI_IRQ_TYPE_CMD_MODE :
       gpar[chid]->dsi_lnk_registers[link]->cmd_mode_sts_ctl   |= (u8)irq_src;
       break;

    case DSI_IRQ_TYPE_DIRECT_CMD_MODE :
       gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_ctl |= (u16)irq_src;
       break;

    case DSI_IRQ_TYPE_DIRECT_CMD_RD_MODE :
       gpar[chid]->dsi_lnk_registers[link]->direct_cmd_rd_sts_ctl |= (u16)irq_src;
       break;

    case DSI_IRQ_TYPE_VID_MODE :
       gpar[chid]->dsi_lnk_registers[link]->vid_mode_sts_ctl |= (u16)irq_src;
       break;

     case DSI_IRQ_TYPE_TG :
       gpar[chid]->dsi_lnk_registers[link]->tg_sts_ctl |= (u8)irq_src;
       break;

     case DSI_IRQ_TYPE_DPHY_ERROR :
       gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_err_ctl |= (u16)irq_src;
       break;

     case DSI_IRQ_TYPE_DPHY_CLK_TRIM_RD :
       gpar[chid]->dsi_lnk_registers[link]->dphy_clk_trim_rd_ctl |=(u8)irq_src;
       break;

    default:
	dsi_error = DSI_INVALID_PARAMETER;
        break;

    }
    return(dsi_error);
}
u32 dsidisableirqsrc(mcde_ch_id chid, dsi_link link, dsi_irq_type irq_type,u32 irq_src)
{
    dsi_error    dsi_error = DSI_OK;

    if (link > DSI_LINK2)
    {
        return(DSI_INVALID_PARAMETER);
    }
    switch(irq_type)
    {
    case DSI_IRQ_TYPE_MCTL_MAIN :
       gpar[chid]->dsi_lnk_registers[link]->mctl_main_sts_ctl  |= (u8)~irq_src;
       break;

    case DSI_IRQ_TYPE_CMD_MODE :
       gpar[chid]->dsi_lnk_registers[link]->cmd_mode_sts_ctl   |= (u8)~irq_src;
       break;

    case DSI_IRQ_TYPE_DIRECT_CMD_MODE :
       gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_ctl |= (u16)~irq_src;
       break;

    case DSI_IRQ_TYPE_DIRECT_CMD_RD_MODE :
       gpar[chid]->dsi_lnk_registers[link]->direct_cmd_rd_sts_ctl |= (u16)~irq_src;
       break;

    case DSI_IRQ_TYPE_VID_MODE :
       gpar[chid]->dsi_lnk_registers[link]->vid_mode_sts_ctl |= (u16)~irq_src;
       break;

     case DSI_IRQ_TYPE_TG :
       gpar[chid]->dsi_lnk_registers[link]->tg_sts_ctl |= (u8)~irq_src;
       break;

     case DSI_IRQ_TYPE_DPHY_ERROR :
       gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_err_ctl |= (u16)~irq_src;
       break;

     case DSI_IRQ_TYPE_DPHY_CLK_TRIM_RD :
       gpar[chid]->dsi_lnk_registers[link]->dphy_clk_trim_rd_ctl |=(u8)~irq_src;
       break;

    default:
        dsi_error = DSI_INVALID_PARAMETER;
        break;

    }
    return(dsi_error);

}
u32 dsiclearirqsrc(mcde_ch_id chid, dsi_link link, dsi_irq_type irq_type, u32 irq_src)
{
    dsi_error    dsi_error = DSI_OK;

    if (link > DSI_LINK2)
    {
        return(DSI_INVALID_PARAMETER);
    }
    switch(irq_type)
    {
    case DSI_IRQ_TYPE_MCTL_MAIN :
       gpar[chid]->dsi_lnk_registers[link]->mctl_main_sts_flag  |= (u8)irq_src;
       break;

    case DSI_IRQ_TYPE_CMD_MODE :
       gpar[chid]->dsi_lnk_registers[link]->cmd_mode_sts_flag   |= (u8)irq_src;
       break;

    case DSI_IRQ_TYPE_DIRECT_CMD_MODE :
       gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_flag |= (u16)irq_src;
       break;

    case DSI_IRQ_TYPE_DIRECT_CMD_RD_MODE :
       gpar[chid]->dsi_lnk_registers[link]->direct_cmd_rd_sts_flag |= (u16)irq_src;
       break;

    case DSI_IRQ_TYPE_VID_MODE :
       gpar[chid]->dsi_lnk_registers[link]->vid_mode_sts_flag |= (u16)irq_src;
       break;

     case DSI_IRQ_TYPE_TG :
       gpar[chid]->dsi_lnk_registers[link]->tg_sts_flag |= (u8)irq_src;
       break;

     case DSI_IRQ_TYPE_DPHY_ERROR :
       gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_err_flag |= (u16)irq_src;
       break;

     case DSI_IRQ_TYPE_DPHY_CLK_TRIM_RD :
       gpar[chid]->dsi_lnk_registers[link]->dphy_clk_trim_rd_flag |=(u8)irq_src;
       break;

    default:
        dsi_error = DSI_INVALID_PARAMETER;
        break;

    }
    return(dsi_error);
}
u32 dsigetirqsrc(mcde_ch_id chid, dsi_link link, dsi_irq_type irq_type)
{
    u32 irq_src = DSI_NO_INTERRUPT;

    switch(irq_type)
    {
    case DSI_IRQ_TYPE_MCTL_MAIN :
       irq_src = (u32)gpar[chid]->dsi_lnk_registers[link]->mctl_main_sts_flag ;
       break;

    case DSI_IRQ_TYPE_CMD_MODE :
       irq_src = (u32)gpar[chid]->dsi_lnk_registers[link]->cmd_mode_sts_flag;
       break;

    case DSI_IRQ_TYPE_DIRECT_CMD_MODE :
       irq_src = (u32)gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_flag;
       break;

    case DSI_IRQ_TYPE_DIRECT_CMD_RD_MODE :
       irq_src = (u32)gpar[chid]->dsi_lnk_registers[link]->direct_cmd_rd_sts_flag;
       break;

    case DSI_IRQ_TYPE_VID_MODE :
       irq_src = (u32)gpar[chid]->dsi_lnk_registers[link]->vid_mode_sts_flag;
       break;

     case DSI_IRQ_TYPE_TG :
       irq_src = (u32)gpar[chid]->dsi_lnk_registers[link]->tg_sts_flag;
       break;

     case DSI_IRQ_TYPE_DPHY_ERROR :
       irq_src = (u32)gpar[chid]->dsi_lnk_registers[link]->mctl_dphy_err_flag;
       break;

     case DSI_IRQ_TYPE_DPHY_CLK_TRIM_RD :
       irq_src = (u32)gpar[chid]->dsi_lnk_registers[link]->dphy_clk_trim_rd_flag;
       break;

    default:
        break;

    }
    return(irq_src);

}

u32 dsiconfdphy(mcde_pll_ref_clk pll_sel, mcde_ch_id chid, dsi_link link)
{
    mcde_dsi_clk_config clk_config;
    dsi_pll_ctl         pll_ctl;
    u8              link_sts;
    u32            time_out= 1000;

    clk_config.pllout_divsel2 = MCDE_PLL_OUT_1;
    clk_config.pllout_divsel1 = MCDE_PLL_OUT_1;
    clk_config.pllout_divsel0 = MCDE_PLL_OUT_1;
    clk_config.txescdiv_sel = MCDE_DSI_MCDECLK;
    clk_config.txescdiv     = 0x8;

    pll_ctl.division_ratio  = 0x0;
    pll_ctl.pll_master = 0x0;
    pll_ctl.pll_in_sel = 0x0;
    pll_ctl.pll_out_sel = 0x1;

    switch(pll_sel)
    {
    case MCDE_CLK27:
        clk_config.pll4in_sel = MCDE_CLK27;
        pll_ctl.multiplier = 0x53;
        break;

    case MCDE_TV1CLK:
        clk_config.pll4in_sel = MCDE_TV1CLK;
        pll_ctl.division_ratio = 0xD3;
        break;

    case MCDE_TV2CLK:
        clk_config.pll4in_sel = MCDE_TV2CLK;
        pll_ctl.division_ratio = 0xD3;
        break;

    case MCDE_HDMICLK:
        clk_config.pll4in_sel = MCDE_HDMICLK;
        pll_ctl.division_ratio = 0x9E;
        break;

     case MCDE_MXTALI:
        clk_config.pll4in_sel = MCDE_CLK27;
        pll_ctl.division_ratio = 0xBA;
        break;

    default:
        return(1);
    }

	mcdesetdsiclk(link, chid, clk_config);

      /*MCTL_MAIN_DATA_CTL*/
	dsisetlinkstate(link, DSI_ENABLE, chid);

        /*Main enable-start enable*/

	dsisetlanestate(link, chid, DSI_PLL_START, DSI_CLK_LANE);
	dsisetlanestate(link, chid, DSI_PLL_START, DSI_DATA_LANE1);

	dsisetPLLcontrol(link, chid, pll_ctl);
	dsisetPLLmode(link, chid, DSI_PLL_START);


    while(time_out > 0)
        time_out--;

    /*Wait for PLL Ready*/
	dsigetlinkstatus(link, chid, &link_sts);

    if(!(link_sts & 0x6))
        return(1);/*PLL programming failed*/

    return(0);

}

dsi_error dsigetdirectcommandstatus(dsi_link link,mcde_ch_id chid, u32 * p_status)
{
    dsi_error    dsi_error = DSI_OK;

    *p_status = (u32)(gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts & 0xFFFFFFFF);

    return(dsi_error);
}
u32 dsisenddirectcommand(dsi_interface_mode_type mode_type, u32 cmd_head,u32 cmd_size,u32 cmd1,u32 cmd2,u32 cmd3,u32 cmd4, dsi_link link, mcde_ch_id chid)
{
    u32 err=0;
    switch(cmd_head)
    {
        /********************************************************************/
        case 0x39:
            /** Direct command main  register */
            if (mode_type == DSI_CLK_LANE_HSM)
		gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings = 0x00003908 | cmd_size << 16;
            else /** LPM */
		gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings = 0x00203908 | cmd_size << 16;

            break;
         /********************************************************************/
         case 0x15:
            /** Direct command main  register */
            if (mode_type == DSI_CLK_LANE_HSM)
		gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings =  0x00001500 | cmd_size << 16;
	    else /** LPM */
		gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings =  0x00201500 | cmd_size << 16;
            break;

         /********************************************************************/
         case 0x05:
            /** Direct command main  register */
            if (mode_type == DSI_CLK_LANE_HSM)
		gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings =  0x00000500 | cmd_size << 16;
	    else /** LPM */
		gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings =  0x00200500 | cmd_size << 16;
	    break;

         case 0x09:
            /** Direct command main  register */
            if (mode_type == DSI_CLK_LANE_HSM)
		gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings =  0x00000908 | cmd_size << 16;
	    else /** LPM */
		gpar[chid]->dsi_lnk_registers[link]->direct_cmd_main_settings =  0x00200908 | cmd_size << 16;
	    break;

         /********************************************************************/
         default:
            printk("Command ID not supported\n");
            err=1;
			return err;

    }

	/** Data registers */
	if (cmd1 != 0)
	{
		gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = cmd1;
	}
	if (cmd2 != 0)
	{
		gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat1 = cmd2;
	}
	if (cmd3 != 0)
	{
		gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat2 = cmd3;

	}
	if (cmd4 != 0)
	{
		gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat3= cmd4;

	}

	/** Send command */
	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_send = 0x00000001;
	 return err;
}
EXPORT_SYMBOL(dsisenddirectcommand);
/**
 * dsiLPdcslongwrite - 	This Api is used to send the data(Max 16 bytes) with dcs long packet command using dsi interface in low power mode.
 *
 */
u32 dsiLPdcslongwrite(u32 VC_ID, u32 NoOfParam,  u32 Param0,  u32 Param1,u32 Param2, u32 Param3,
									u32 Param4,u32 Param5,u32 Param6,u32 Param7,u32 Param8, u32 Param9,u32 Param10,u32 Param11,
									u32 Param12,u32 Param13,u32 Param14, u32 Param15, mcde_ch_id chid, dsi_link link)
{
	int uTimeout = 0xFFFFFFFF;
	u32 retVal =0;
    dsi_cmd_main_setting cmd_settings;
	u32              command_sts;

	cmd_settings.cmd_header = 0x39;
	cmd_settings.cmd_lp_enable = DSI_ENABLE;
    cmd_settings.cmd_size = NoOfParam;
	cmd_settings.cmd_id     = VC_ID;
    cmd_settings.packet_type = 0x1; /** long packet */
    cmd_settings.cmd_nature = 0x0; /** command nature is write */
    cmd_settings.cmd_trigger_val = 0x0;

	dsisetdirectcmdsettings(link, chid, cmd_settings);


	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT0) |
																(Param0 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT0));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT1) |
																(Param1 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT1));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT2) |
																(Param2 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT2));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT3) |
																(Param3 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT3));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat1= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT1_WRDAT4) |
																(Param4 << Shift_DSIDIRECT_CMD_WRDAT1_WRDAT4));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat1= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT1_WRDAT5) |
																(Param5 << Shift_DSIDIRECT_CMD_WRDAT1_WRDAT5));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat1= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT1_WRDAT6) |
																(Param6 << Shift_DSIDIRECT_CMD_WRDAT1_WRDAT6));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat1= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT1_WRDAT7) |
																(Param7<< Shift_DSIDIRECT_CMD_WRDAT1_WRDAT7));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat2= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT2_WRDAT8) |
																(Param8 << Shift_DSIDIRECT_CMD_WRDAT2_WRDAT8));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat2= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT2_WRDAT9) |
																(Param9 << Shift_DSIDIRECT_CMD_WRDAT2_WRDAT9));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat2= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT2_WRDAT10) |
																(Param10 << Shift_DSIDIRECT_CMD_WRDAT2_WRDAT10));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat2= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT2_WRDAT11) |
																(Param11 << Shift_DSIDIRECT_CMD_WRDAT2_WRDAT11));


	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat3= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT3_WRDAT12) |
																(Param12 << Shift_DSIDIRECT_CMD_WRDAT3_WRDAT12));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat3= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT3_WRDAT13) |
																(Param13 << Shift_DSIDIRECT_CMD_WRDAT3_WRDAT13));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat3= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT3_WRDAT14) |
																(Param14 << Shift_DSIDIRECT_CMD_WRDAT3_WRDAT14));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat3= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT3_WRDAT15) |
																(Param15 << Shift_DSIDIRECT_CMD_WRDAT3_WRDAT15));

	 /** Send command */
	 gpar[chid]->dsi_lnk_registers[link]->direct_cmd_send = 0x00000001;


	uTimeout = 0xFFF;

	while(uTimeout > 0)
		  uTimeout--;



        /** Wait for PLL Ready */
	dsigetdirectcommandstatus(link, chid, &command_sts);

	if(!(command_sts & 0x2))
	{
		dbgprintk(MCDE_ERROR_INFO, "ERROR in sending DSI_LP_DCS_Long_Write\n");
		return(1);/** PLL programming failed */
	}

	return retVal;
}
EXPORT_SYMBOL(dsiLPdcslongwrite);
/**
 * dsiLPdcsshortwrite1parm - 	This Api is used to send dcs short packet(1 byte max) command using dsi interface in low power mode.
 *
 */
u32 dsiLPdcsshortwrite1parm(u32 VC_ID, u32 Param0,u32 Param1, mcde_ch_id chid, dsi_link link)
{
	u32 retVal =0;

	u32 uTimeout = 0xFFFFFFFF;

    dsi_cmd_main_setting cmd_settings;
	u32              command_sts;

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_clr = 0x2; /** write completed flag */


	cmd_settings.cmd_header = 0x15;
	cmd_settings.cmd_lp_enable = DSI_ENABLE;
    cmd_settings.cmd_size = 0x2;
	cmd_settings.cmd_id     = VC_ID;
    cmd_settings.packet_type = 0x0; /** short packet */
    cmd_settings.cmd_nature = 0x0; /** command nature is write */
    cmd_settings.cmd_trigger_val = 0x0;

	dsisetdirectcmdsettings(link, chid, cmd_settings);



	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT0) |
																(Param0 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT0));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT1) |
																(Param1 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT1));


	/* Send command */
	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_send = 0x00000001;

	while(uTimeout > 0)
		  uTimeout--;



    /*Wait for PLL Ready*/
	dsigetdirectcommandstatus(link, chid, &command_sts);

	if(!(command_sts & 0x2))
	{
		dbgprintk(MCDE_ERROR_INFO, "ERROR in sending DSI_LP_DCS_Long_Write\n");
		return(1);/** PLL programming failed*/
	}
	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_clr = 0x2; /** write completed flag */

	return retVal;
}
EXPORT_SYMBOL(dsiLPdcsshortwrite1parm);
/**
 * dsiLPdcsshortwritenoparam - 	This Api is used to send dcs short packet command using dsi interface with out any data in low power mode.
 *
 */
u32 dsiLPdcsshortwritenoparam(u32 VC_ID, u32 Param0, mcde_ch_id chid, dsi_link link)
{

	u32 retVal =0;
	u32 uTimeout = 0xFFFFFFFF;
    dsi_cmd_main_setting cmd_settings;
	u32              command_sts;

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_clr = 0x2; /** write completed flag */


	cmd_settings.cmd_header = 0x05;
	cmd_settings.cmd_lp_enable = DSI_ENABLE;
    cmd_settings.cmd_size = 0x1;
	cmd_settings.cmd_id     = VC_ID;
    cmd_settings.packet_type = 0x0; /** short packet */
    cmd_settings.cmd_nature = 0x0; /** command nature is write */
    cmd_settings.cmd_trigger_val = 0x0;

	dsisetdirectcmdsettings(link, chid, cmd_settings);

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT0) |
																(Param0 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT0));

	/** Send command */
	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_send = 0x00000001;


	while(uTimeout > 0)
		  uTimeout--;



        /** Wait for PLL Ready */
	dsigetdirectcommandstatus(link, chid, &command_sts);

	if(!(command_sts & 0x2))
	{
		dbgprintk(MCDE_ERROR_INFO, "ERROR in sending DSI_LP_DCS_Long_Write\n");
		return(1);/** PLL programming failed */
	}
	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_clr = 0x2; /** write completed flag	*/

	return retVal;
}
EXPORT_SYMBOL(dsiLPdcsshortwritenoparam);
/**
 * dsiHSdcslongwrite - 	This Api is used to send the data(Max 16 bytes) with dcs long packet command using dsi interface in high speed mode.
 *
 */
u32 dsiHSdcslongwrite(u32 VC_ID, u32 NoOfParam,  u32 Param0,  u32 Param1,u32 Param2, u32 Param3,
									u32 Param4,u32 Param5,u32 Param6,u32 Param7,u32 Param8, u32 Param9,u32 Param10,u32 Param11,
									u32 Param12,u32 Param13,u32 Param14, u32 Param15, mcde_ch_id chid, dsi_link link)
{
	int uTimeout = 0xFFFFFFFF;
	u32 retVal =0;
	dsi_cmd_main_setting cmd_settings;
	u32			   command_sts;

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_clr = 0x2; /** write completed flag */

	cmd_settings.cmd_header = 0x39;
	cmd_settings.cmd_lp_enable = DSI_DISABLE;
	cmd_settings.cmd_size = NoOfParam;
	cmd_settings.cmd_id	  = VC_ID;
	cmd_settings.packet_type = 0x1; /** long packet */
	cmd_settings.cmd_nature = 0x0; /** command nature is write */
	cmd_settings.cmd_trigger_val = 0x0;

	dsisetdirectcmdsettings(link, chid, cmd_settings);


	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT0) |
														  (Param0 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT0));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT1) |
														  (Param1 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT1));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT2) |
														  (Param2 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT2));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT3) |
														  (Param3 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT3));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat1= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT1_WRDAT4) |
														  (Param4 << Shift_DSIDIRECT_CMD_WRDAT1_WRDAT4));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat1= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT1_WRDAT5) |
														  (Param5 << Shift_DSIDIRECT_CMD_WRDAT1_WRDAT5));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat1= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT1_WRDAT6) |
														  (Param6 << Shift_DSIDIRECT_CMD_WRDAT1_WRDAT6));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat1= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT1_WRDAT7) |
														  (Param7<< Shift_DSIDIRECT_CMD_WRDAT1_WRDAT7));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat2= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT2_WRDAT8) |
														  (Param8 << Shift_DSIDIRECT_CMD_WRDAT2_WRDAT8));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat2= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT2_WRDAT9) |
														  (Param9 << Shift_DSIDIRECT_CMD_WRDAT2_WRDAT9));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat2= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT2_WRDAT10) |
														  (Param10 << Shift_DSIDIRECT_CMD_WRDAT2_WRDAT10));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat2= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT2_WRDAT11) |
														  (Param11 << Shift_DSIDIRECT_CMD_WRDAT2_WRDAT11));


	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat3= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT3_WRDAT12) |
														  (Param12 << Shift_DSIDIRECT_CMD_WRDAT3_WRDAT12));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat3= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT3_WRDAT13) |
														  (Param13 << Shift_DSIDIRECT_CMD_WRDAT3_WRDAT13));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat3= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT3_WRDAT14) |
														  (Param14 << Shift_DSIDIRECT_CMD_WRDAT3_WRDAT14));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat3= ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT3_WRDAT15) |
														  (Param15 << Shift_DSIDIRECT_CMD_WRDAT3_WRDAT15));

	/** Send command */
	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_send = 0x00000001;


	uTimeout = 0xFFF;

	while(uTimeout > 0)
		uTimeout--;

	/** Wait for PLL Ready */
	dsigetdirectcommandstatus(link, chid, &command_sts);

	if(!(command_sts & 0x2))
	{
	  dbgprintk(MCDE_ERROR_INFO, "ERROR in sending DSI_HS_DCS_Long_Write\n");
	  return(1);/** PLL programming failed */
	}

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_clr = 0x2; /** write completed flag */
	return retVal;
}
EXPORT_SYMBOL(dsiHSdcslongwrite);
/**
 * dsiHSdcsshortwrite1parm - 	This Api is used to send dcs short packet(1 byte max) command using dsi interface in high speed mode.
 *
 */
u32 dsiHSdcsshortwrite1parm(u32 VC_ID, u32 Param0,u32 Param1, mcde_ch_id chid, dsi_link link)
{
	u32 retVal =0;

	u32 uTimeout = 0xFFFFFFFF;

    dsi_cmd_main_setting cmd_settings;
	u32              command_sts;

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_clr = 0x2; /** write completed flag */


	cmd_settings.cmd_header = 0x15;
	cmd_settings.cmd_lp_enable = DSI_DISABLE;
    cmd_settings.cmd_size = 0x2;
	cmd_settings.cmd_id     = VC_ID;
    cmd_settings.packet_type = 0x0; /** short packet */
    cmd_settings.cmd_nature = 0x0; /** command nature is write */
    cmd_settings.cmd_trigger_val = 0x0;

	dsisetdirectcmdsettings(link, chid, cmd_settings);



	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT0) |
																(Param0 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT0));

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT1) |
																(Param1 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT1));


	/* Send command */
	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_send = 0x00000001;

	while(uTimeout > 0)
		  uTimeout--;



    /*Wait for PLL Ready*/
	dsigetdirectcommandstatus(link, chid, &command_sts);

	if(!(command_sts & 0x2))
	{
		dbgprintk(MCDE_ERROR_INFO, "ERROR in sending DSI_LP_DCS_Long_Write\n");
		return(1);/** PLL programming failed*/
	}
	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_clr = 0x2; /** write completed flag */

	return retVal;
}
EXPORT_SYMBOL(dsiHSdcsshortwrite1parm);
/**
 * dsiHSdcsshortwritenoparam - 	This Api is used to send dcs short packet command using dsi interface with out any data in high speed mode.
 *
 */
u32 dsiHSdcsshortwritenoparam(u32 VC_ID, u32 Param0, mcde_ch_id chid, dsi_link link)
{

	u32 retVal =0;
	u32 uTimeout = 0xFFFFFFFF;
    dsi_cmd_main_setting cmd_settings;
	u32              command_sts;

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_clr = 0x2; /** write completed flag */


	cmd_settings.cmd_header = 0x05;
	cmd_settings.cmd_lp_enable = DSI_DISABLE;
    cmd_settings.cmd_size = 0x1;
	cmd_settings.cmd_id     = VC_ID;
    cmd_settings.packet_type = 0x0; /** short packet */
    cmd_settings.cmd_nature = 0x0; /** command nature is write */
    cmd_settings.cmd_trigger_val = 0x0;

	dsisetdirectcmdsettings(link, chid, cmd_settings);

	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_wrdat0 & ~DSIDIRECT_CMD_WRDAT0_WRDAT0) |
																(Param0 << Shift_DSIDIRECT_CMD_WRDAT0_WRDAT0));

	/** Send command */
	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_send = 0x00000001;


	while(uTimeout > 0)
		  uTimeout--;



        /** Wait for PLL Ready */
	dsigetdirectcommandstatus(link, chid, &command_sts);

	if(!(command_sts & 0x2))
	{
		dbgprintk(MCDE_ERROR_INFO, "ERROR in sending DSI_LP_DCS_Long_Write\n");
		return(1);/** PLL programming failed */
	}
	gpar[chid]->dsi_lnk_registers[link]->direct_cmd_sts_clr = 0x2; /** write completed flag	*/

	return retVal;
}
EXPORT_SYMBOL(dsiHSdcsshortwritenoparam);
/**
 * dsireaddata - 	This Api is used to read the data from dsi interface command fifo(max of 4 bytes).
 *
 */
u32 dsireaddata(u8* byte0, u8* byte1, u8* byte2, u8* byte3, mcde_ch_id chid, dsi_link link)
{
	u32 retVal =0;

	*byte0 = (gpar[chid]->dsi_lnk_registers[link]->direct_cmd_rddat & DSIDIRECT_CMD_RDAT0);
	*byte1 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_rddat & DSIDIRECT_CMD_RDAT1)) >> Shift_DSIDIRECT_CMD_RDAT1;
	*byte2 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_rddat & DSIDIRECT_CMD_RDAT2)) >> Shift_DSIDIRECT_CMD_RDAT2;
	*byte3 = ((gpar[chid]->dsi_lnk_registers[link]->direct_cmd_rddat & DSIDIRECT_CMD_RDAT3)) >> Shift_DSIDIRECT_CMD_RDAT3;

	return retVal;
}
EXPORT_SYMBOL(dsireaddata);

#ifdef _cplusplus
}
#endif /* _cplusplus */







