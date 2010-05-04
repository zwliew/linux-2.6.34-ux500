/*
*   Implementation for Audio Hardware Access
* Overview:
*
* Copyright (C) 2009 ST Ericsson
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#include "ste_audio_io_core.h"
#include "ste_audio_io_hwctrl_common.h"
#include "ste_audio_io_ab8500_reg_defs.h"

audiocodec_context_t *ptr_audio_codec_cnxt;

static int acodec_device_id;
#define AB8500_ED	0
#define AB8500_CUT1	1

void ste_audioio_core_api_init_data(void)
{
	ptr_audio_codec_cnxt = kmalloc(sizeof(audiocodec_context_t),GFP_KERNEL);

	memset(ptr_audio_codec_cnxt, 0, sizeof (*ptr_audio_codec_cnxt));

	init_MUTEX(&(ptr_audio_codec_cnxt->audio_io_sem));

	ste_audioio_init_transducer_cnxt();
}

void ste_audioio_init_transducer_cnxt(void)
{
	transducer_context_t *ptr = NULL;

	/* initializing HEADSET properties*/
	ptr = &ptr_audio_codec_cnxt->transducer[HS_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_headset;
	ptr->pwr_down_func = ste_audio_io_power_down_headset;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = ste_audio_io_set_headset_gain;
	ptr->get_gain_func = ste_audio_io_get_headset_gain;
	ptr->mute_func =  ste_audio_io_mute_headset;
	ptr->unmute_func = ste_audio_io_unmute_headset;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = ste_audio_io_enable_fade_headset;
	ptr->disable_fade_func = ste_audio_io_disable_fade_headset;
	ptr->switch_to_burst_func = ste_audio_io_switch_to_burst_mode_headset;
	ptr->switch_to_normal_func = ste_audio_io_switch_to_normal_mode_headset;

	/* initializing EARPIECE properties*/
	ptr = &ptr_audio_codec_cnxt->transducer[EAR_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_earpiece;
	ptr->pwr_down_func = ste_audio_io_power_down_earpiece;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = ste_audio_io_set_earpiece_gain;
	ptr->get_gain_func = ste_audio_io_get_earpiece_gain;
	ptr->mute_func =  ste_audio_io_mute_earpiece;
	ptr->unmute_func = ste_audio_io_unmute_earpiece;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = ste_audio_io_enable_fade_earpiece;
	ptr->disable_fade_func = ste_audio_io_disable_fade_earpiece;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;


	/* initializing IHF properties*/
	ptr = &ptr_audio_codec_cnxt->transducer[IHF_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_ihf;
	ptr->pwr_down_func = ste_audio_io_power_down_ihf;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = ste_audio_io_set_ihf_gain;
	ptr->get_gain_func = ste_audio_io_get_ihf_gain;
	ptr->mute_func =  ste_audio_io_mute_ihf;
	ptr->unmute_func = ste_audio_io_unmute_ihf;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = ste_audio_io_enable_fade_ihf;
	ptr->disable_fade_func = ste_audio_io_disable_fade_ihf;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;


	/* initializing VIBL properties*/
	ptr = &ptr_audio_codec_cnxt->transducer[VIBL_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_vibl;
	ptr->pwr_down_func = ste_audio_io_power_down_vibl;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = ste_audio_io_set_vibl_gain;
	ptr->get_gain_func = ste_audio_io_get_vibl_gain;
	ptr->mute_func =  ste_audio_io_mute_vibl;
	ptr->unmute_func = ste_audio_io_unmute_vibl;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = ste_audio_io_enable_fade_vibl;
	ptr->disable_fade_func = ste_audio_io_disable_fade_vibl;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;

	/* initializing VIBR properties*/
	ptr = &ptr_audio_codec_cnxt->transducer[VIBR_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_vibr;
	ptr->pwr_down_func = ste_audio_io_power_down_vibr;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = ste_audio_io_set_vibr_gain;
	ptr->get_gain_func = ste_audio_io_get_vibr_gain;
	ptr->mute_func =  ste_audio_io_mute_vibr;
	ptr->unmute_func = ste_audio_io_unmute_vibr;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = ste_audio_io_enable_fade_vibr;
	ptr->disable_fade_func = ste_audio_io_disable_fade_vibr;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;

	/* initializing MIC1A properties*/
	ptr = &ptr_audio_codec_cnxt->transducer[MIC1A_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_mic1a;
	ptr->pwr_down_func = ste_audio_io_power_down_mic1a;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = ste_audio_io_set_mic1a_gain;
	ptr->get_gain_func = ste_audio_io_get_mic1a_gain;
	ptr->mute_func =  ste_audio_io_mute_mic1a;
	ptr->unmute_func = ste_audio_io_unmute_mic1a;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = ste_audio_io_enable_fade_mic1a;
	ptr->disable_fade_func = ste_audio_io_disable_fade_mic1a;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;


	/* initializing MIC1B properties*/
	ptr = &ptr_audio_codec_cnxt->transducer[MIC1B_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_mic1b;
	ptr->pwr_down_func = ste_audio_io_power_down_mic1b;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = ste_audio_io_set_mic1a_gain;
	ptr->get_gain_func = ste_audio_io_get_mic1a_gain;
	ptr->mute_func =  ste_audio_io_mute_mic1a;
	ptr->unmute_func = ste_audio_io_unmute_mic1a;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = ste_audio_io_enable_fade_mic1a;
	ptr->disable_fade_func = ste_audio_io_disable_fade_mic1a;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;

	/* initializing MIC2 properties*/
	ptr = &ptr_audio_codec_cnxt->transducer[MIC2_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_mic2;
	ptr->pwr_down_func = ste_audio_io_power_down_mic2;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = ste_audio_io_set_mic2_gain;
	ptr->get_gain_func = ste_audio_io_get_mic2_gain;
	ptr->mute_func =  ste_audio_io_mute_mic2;
	ptr->unmute_func = ste_audio_io_unmute_mic2;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = ste_audio_io_enable_fade_mic2;
	ptr->disable_fade_func = ste_audio_io_disable_fade_mic2;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;

	/* initializing LIN properties*/
	ptr = &ptr_audio_codec_cnxt->transducer[LIN_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_lin;
	ptr->pwr_down_func = ste_audio_io_power_down_lin;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = ste_audio_io_set_lin_gain;
	ptr->get_gain_func = ste_audio_io_get_lin_gain;
	ptr->mute_func =  ste_audio_io_mute_lin;
	ptr->unmute_func = ste_audio_io_unmute_lin;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = ste_audio_io_enable_fade_lin;
	ptr->disable_fade_func = ste_audio_io_disable_fade_lin;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;

	/* initializing DMIC12 properties*/
	ptr = &ptr_audio_codec_cnxt->transducer[DMIC12_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_dmic12;
	ptr->pwr_down_func = ste_audio_io_power_down_dmic12;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = ste_audio_io_set_dmic12_gain;
	ptr->get_gain_func = ste_audio_io_get_dmic12_gain;
	ptr->mute_func =  ste_audio_io_mute_dmic12;
	ptr->unmute_func = ste_audio_io_unmute_dmic12;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = ste_audio_io_enable_fade_dmic12;
	ptr->disable_fade_func = ste_audio_io_disable_fade_dmic12;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;

	/* initializing DMIC34 properties*/
	ptr = &ptr_audio_codec_cnxt->transducer[DMIC34_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_dmic34;
	ptr->pwr_down_func = ste_audio_io_power_down_dmic34;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = ste_audio_io_set_dmic34_gain;
	ptr->get_gain_func = ste_audio_io_get_dmic34_gain;
	ptr->mute_func =  ste_audio_io_mute_dmic34;
	ptr->unmute_func = ste_audio_io_unmute_dmic34;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = ste_audio_io_enable_fade_dmic34;
	ptr->disable_fade_func = ste_audio_io_disable_fade_dmic34;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;

	/* initializing DMIC56 properties*/
	ptr = &ptr_audio_codec_cnxt->transducer[DMIC56_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_dmic56;
	ptr->pwr_down_func = ste_audio_io_power_down_dmic56;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = ste_audio_io_set_dmic56_gain;
	ptr->get_gain_func = ste_audio_io_get_dmic56_gain;
	ptr->mute_func =  ste_audio_io_mute_dmic56;
	ptr->unmute_func = ste_audio_io_unmute_dmic56;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = ste_audio_io_enable_fade_dmic56;
	ptr->disable_fade_func = ste_audio_io_disable_fade_dmic56;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;
}


void ste_audioio_core_api_free_data(void)
{
	kfree(ptr_audio_codec_cnxt);
}

int return_device_id(void)
{
	__u8 data;

	data = ab8500_read(AB8500_MISC, (0x80 & 0xFF));
	if (((data & 0xF0) == 0x10) || ((data & 0xF0) == 0x11)) {
		/* V1 version */
		return AB8500_CUT1;
	}
	/* V0 version */
	return AB8500_ED;
}

int ste_audioio_core_api_powerup_audiocodec(void)
{
	int error = 0;

	/*aquire mutex*/
	down(&(ptr_audio_codec_cnxt->audio_io_sem));

	ptr_audio_codec_cnxt->audio_codec_powerup++;

	if (1 == ptr_audio_codec_cnxt->audio_codec_powerup) {
		__u8 data, old_data;

		acodec_device_id = return_device_id();

		old_data = HW_REG_READ(AB8500_CTRL3_REG);

		data = 0xFE & old_data;
		HW_REG_WRITE(AB8500_CTRL3_REG, data);	//0x0200

		data = 0x02 | old_data;
		HW_REG_WRITE(AB8500_CTRL3_REG, data);	//0x0200

		old_data = HW_REG_READ(AB8500_SYSULPCLK_CTRL1_REG);

		if(AB8500_CUT1 == acodec_device_id)
			data = 0x18 | old_data;
		else
			data = 0x10 | old_data;

		HW_REG_WRITE(AB8500_SYSULPCLK_CTRL1_REG, data);	//0x020B

		old_data = HW_REG_READ(AB8500_REGU_MISC1_REG);
		data = 0x04 | old_data;
		HW_REG_WRITE(AB8500_REGU_MISC1_REG, data);//0x380

		old_data = HW_REG_READ(AB8500_REGU_VAUDIO_SUPPLY_REG);
		data = 0x5E | old_data;
		HW_REG_WRITE(AB8500_REGU_VAUDIO_SUPPLY_REG, data);//0x0383

		if(AB8500_CUT1 == acodec_device_id){
			old_data = HW_REG_READ(AB8500_GPIO_DIR4_REG);
			data = 0x54 | old_data;
			HW_REG_WRITE(AB8500_GPIO_DIR4_REG, data);//0x1013
		}

	MSG0("Software reset");
	error=HW_REG_WRITE(SOFTWARE_RESET_REG, SW_RESET);
	if (error!=0) {
		printk("ERROR: Software reset error=%d", error);
		return error;
	}

	MSG0("Device Power Up and Analog Parts Power Up");
	error = HW_ACODEC_MODIFY_WRITE(POWER_UP_CONTROL_REG,
				(DEVICE_POWER_UP|ANALOG_PARTS_POWER_UP), 0);
	if(error!=0) {
		printk("ERROR:Device Power Up and Analog Parts Power Up"
							",error=%d", error);
		return error;
	}

	MSG0("Enable Master Generator");
	error=HW_ACODEC_MODIFY_WRITE(IF0_IF1_MASTER_CONF_REG, EN_MASTGEN, 0);
	if(error!=0) {
		printk("ERROR:  Enable Master Generator error=%d", error);
		return error;
	}

	MSG0("Configure IF0: Master Mode FSync0 and FsBitClk0 are set output");
	error=HW_ACODEC_MODIFY_WRITE(TDM_IF_BYPASS_B_FIFO_REG, IF0_MASTER, 0);
	if(error!=0) {
		printk("ERROR: Configure IF0: Master Mode FSync0 and FsBitClk0"
		"are set as output error=%d", error);
		return error;
	}

	/*Configuring IF0*/
	MSG0("Configure IF0: Enable FsBitClk and FSync");


	error=HW_ACODEC_MODIFY_WRITE(IF0_IF1_MASTER_CONF_REG, BITCLK_OSR_N_256,
									0);
	if(error!=0) {
		printk("ERROR: Configure IF0: Enable FsBitClk and FSync"
							"error=%d", error);
		return error;
	}

	MSG0("Configure IF0: TDM Format 20 Bits word length");
	error=HW_REG_WRITE(IF0_CONF_REG, IF_DELAYED| TDM_FORMAT |
								WORD_LENGTH_20);
	if(error!=0) {
		printk("ERROR:  Configure IF0: TDM Format 16 Bits word length"
							" error=%d", error);
		return error;
	}
	}
	/*release mutex*/
	up(&(ptr_audio_codec_cnxt->audio_io_sem));

	return 0;
}

int ste_audioio_core_api_powerdown_audiocodec(void)
{
	/*aquire mutex*/
	down(&(ptr_audio_codec_cnxt->audio_io_sem));

	ptr_audio_codec_cnxt->audio_codec_powerup--;

	if(0 == ptr_audio_codec_cnxt->audio_codec_powerup)
	{
#if 0 ////////////////////////////////////////////////////////////////
		__u8 data, old_data;

		old_data =
			HW_REG_READ(AB8500_CTRL3_REG);

		data = 0xFE & old_data;
		HW_REG_WRITE(AB8500_CTRL3_REG, data);	//0x0200

		data = 0x02 | old_data;
		HW_REG_WRITE(AB8500_CTRL3_REG, data);	//0x0200

		old_data =
		HW_REG_READ(AB8500_SYSULPCLK_CTRL1_REG);
	#ifdef CONFIG_U8500_AB8500_CUT10
		data = 0x18 | old_data;
	#else
		data = 0x10 | old_data;
	#endif
		HW_REG_WRITE(AB8500_SYSULPCLK_CTRL1_REG, data);	//0x020B

		old_data =
			HW_REG_READ(AB8500_REGU_MISC1_REG);
		data = 0x04 | old_data;
		HW_REG_WRITE(AB8500_REGU_MISC1_REG, data);//0x380

		old_data =
			HW_REG_READ(AB8500_REGU_VAUDIO_SUPPLY_REG);
		data = 0x5E | old_data;
		HW_REG_WRITE(AB8500_REGU_VAUDIO_SUPPLY_REG, data);//0x0383

	#ifdef CONFIG_U8500_AB8500_CUT10
		old_data = HW_REG_READ(AB8500_GPIO_DIR4_REG);
		data = 0x54 | old_data;
		HW_REG_WRITE(AB8500_GPIO_DIR4_REG, data);	//0x1013
	#endif
#endif ////////////////////////////////////////////////////////////
	}
	/*release mutex*/
	up(&(ptr_audio_codec_cnxt->audio_io_sem));
	return 0;
}


int ste_audioio_core_api_access_read(audioio_data_t *dev_data)
{
	int reg;
	if(NULL == dev_data)
		return -EFAULT;
	reg = (dev_data->block<<8)|(dev_data->addr&0xff);
	dev_data->data = HW_REG_READ(reg);
	return 0;
}

int ste_audioio_core_api_access_write(audioio_data_t *dev_data)
{
	int retval,reg;
	if(NULL == dev_data)
		return -EFAULT;

	reg = (dev_data->block<<8)|(dev_data->addr&0xff);
	retval = HW_REG_WRITE(reg,dev_data->data);

	return retval;
}

void ste_audioio_core_api_store_data(AUDIOIO_CH_INDEX channel_index,
							int *ptr,int value)
{
	if(channel_index & e_CHANNEL_1)
		ptr[0] = value;

	if(channel_index & e_CHANNEL_2)
		ptr[1] = value;

	if(channel_index & e_CHANNEL_3)
		ptr[2] = value;

	if(channel_index & e_CHANNEL_4)
		ptr[3] = value;
}
/**
 * \brief Get power or mute status on a specific channel
 * \param channel_index Channel-index of the transducer
 * \param ptr Pointer to is_power_up array or is_muted array
 * \return status of control switch
 */
AUDIOIO_COMMON_SWITCH ste_audioio_core_api_get_status(
				AUDIOIO_CH_INDEX channel_index,int *ptr)
{
	if (channel_index & e_CHANNEL_1) {
		if (AUDIOIO_TRUE == ptr[0])
			return AUDIOIO_COMMON_ON;
		else
			return AUDIOIO_COMMON_OFF;
	}

	if (channel_index & e_CHANNEL_2) {
		if(AUDIOIO_TRUE == ptr[1])
			return AUDIOIO_COMMON_ON;
		else
			return AUDIOIO_COMMON_OFF;
	}

	if (channel_index & e_CHANNEL_3) {
		if(AUDIOIO_TRUE == ptr[2])
			return AUDIOIO_COMMON_ON;
		else
			return AUDIOIO_COMMON_OFF;
	}

	if (channel_index & e_CHANNEL_4) {
		if(AUDIOIO_TRUE == ptr[3])
			return AUDIOIO_COMMON_ON;
		else
			return AUDIOIO_COMMON_OFF;
	}
	return 0;
}
/**
 * \brief Control for powering on/off HW components on a specific channel
 * \param pwr_ctrl Pointer to the structure __audioio_pwr_ctrl
 * \return 0 on success otherwise negative error code
 */
int ste_audioio_core_api_power_control_transducer(audioio_pwr_ctrl_t *pwr_ctrl)
{
	int error = 0;
	transducer_context_t *ptr = NULL;
	AUDIOIO_CH_INDEX channel_index;

	channel_index = pwr_ctrl->channel_index;

	ptr = &ptr_audio_codec_cnxt->transducer[pwr_ctrl->channel_type];

	/*aquire mutex*/
	down(&(ptr_audio_codec_cnxt->audio_io_sem));

	if (AUDIOIO_COMMON_ON == pwr_ctrl->ctrl_switch) {
		if (ptr->pwr_up_func) {
			error = ptr->pwr_up_func(pwr_ctrl->channel_index);
			if (0 == error) {
				ste_audioio_core_api_store_data(channel_index,
						ptr->is_power_up, AUDIOIO_TRUE);
			}
		}
	} else {
		if (ptr->pwr_down_func) {
			error = ptr->pwr_down_func(pwr_ctrl->channel_index);
			if (0 == error) {
				ste_audioio_core_api_store_data(channel_index,
					ptr->is_power_up, AUDIOIO_FALSE);
			}
		}
	}

	/*release mutex*/
	up(&(ptr_audio_codec_cnxt->audio_io_sem));

	return error;
}
/**
 * \brief Query power state of HW path on specified channel
 * \param pwr_ctrl Pointer to the structure __audioio_pwr_ctrl
 * \return 0 on success otherwise negative error code
 */

int ste_audioio_core_api_power_status_transducer(audioio_pwr_ctrl_t *pwr_ctrl)
{

	transducer_context_t *ptr = NULL;
	AUDIOIO_CH_INDEX channel_index;

	channel_index = pwr_ctrl->channel_index;

	ptr = &ptr_audio_codec_cnxt->transducer[pwr_ctrl->channel_type];


	/*aquire mutex*/
	down(&(ptr_audio_codec_cnxt->audio_io_sem));


	pwr_ctrl->ctrl_switch = ste_audioio_core_api_get_status(channel_index,
							ptr->is_power_up);

	/*release mutex*/
	up(&(ptr_audio_codec_cnxt->audio_io_sem));

	return 0;

}

int ste_audioio_core_api_loop_control(audioio_loop_ctrl_t *loop_ctrl)
{
	return 0;
}

int ste_audioio_core_api_loop_status(audioio_loop_ctrl_t *loop_ctrl)
{
	return 0;
}

int ste_audioio_core_api_get_transducer_gain_capability(
						audioio_get_gain_t *get_gain)
{
	return 0;
}

int ste_audioio_core_api_gain_capabilities_loop(
						audioio_gain_loop_t *gain_loop)
{
	return 0;
}

int ste_audioio_core_api_supported_loops(
					audioio_support_loop_t *support_loop)
{
	return 0;
}

int ste_audioio_core_api_gain_descriptor_transducer(
				audioio_gain_desc_trnsdr_t *gdesc_trnsdr)
{
	return 0;
}
/**
 * \brief Control for muting a specific channel in HW
 * \param mute_trnsdr Pointer to the structure __audioio_mute_trnsdr
 * \return 0 on success otherwise negative error code
 */
int ste_audioio_core_api_mute_control_transducer(
					audioio_mute_trnsdr_t *mute_trnsdr)
{
	int error = 0;
	transducer_context_t *ptr = NULL;
	AUDIOIO_CH_INDEX channel_index;

	channel_index = mute_trnsdr->channel_index;

	ptr = &ptr_audio_codec_cnxt->transducer[mute_trnsdr->channel_type];

	/*aquire mutex*/
	down(&(ptr_audio_codec_cnxt->audio_io_sem));

	if (AUDIOIO_COMMON_ON == mute_trnsdr->ctrl_switch) {
		if (ptr->mute_func) {
			error = ptr->mute_func(mute_trnsdr->channel_index);
		    if (0 == error) {
				ste_audioio_core_api_store_data(channel_index ,
						ptr->is_muted, AUDIOIO_TRUE);
			}
		}
	} else {
		if (ptr->unmute_func) {
			if (0 == ptr->unmute_func(channel_index, ptr->gain)) {
				ste_audioio_core_api_store_data(channel_index ,
						ptr->is_muted,AUDIOIO_FALSE);
			}
		}
	}

	/*release mutex*/
	up(&(ptr_audio_codec_cnxt->audio_io_sem));

	return error;
}
/**
 * \brief Query state of mute on specified channel
 * \param mute_trnsdr Pointer to the structure __audioio_mute_trnsdr
 * \return 0 on success otherwise negative error code
 */


int ste_audioio_core_api_mute_status_transducer(
					audioio_mute_trnsdr_t *mute_trnsdr)
{
	transducer_context_t *ptr = NULL;
	AUDIOIO_CH_INDEX channel_index;

	channel_index = mute_trnsdr->channel_index;

	ptr = &ptr_audio_codec_cnxt->transducer[mute_trnsdr->channel_type];


	/*aquire mutex*/
	down(&(ptr_audio_codec_cnxt->audio_io_sem));


	mute_trnsdr->ctrl_switch =ste_audioio_core_api_get_status(channel_index,
								ptr->is_muted);
	/*release mutex*/
	up(&(ptr_audio_codec_cnxt->audio_io_sem));

	return 0;
}
/**
 * \brief control the fading on the transducer called on.
 * \param fade_ctrl Pointer to the structure __audioio_fade_ctrl
 * \return 0 on success otherwise negative error code
 */

int ste_audioio_core_api_fading_control(audioio_fade_ctrl_t *fade_ctrl)
{
	int error = 0;
	transducer_context_t *ptr = NULL;

	ptr = &ptr_audio_codec_cnxt->transducer[fade_ctrl->channel_type];

	/*aquire mutex*/
	down(&(ptr_audio_codec_cnxt->audio_io_sem));

	if(AUDIOIO_COMMON_ON == fade_ctrl->ctrl_switch)
		error = ptr->enable_fade_func();

	else
		error = ptr->disable_fade_func();


	/*release mutex*/
	up(&(ptr_audio_codec_cnxt->audio_io_sem));

	return error;
}

int ste_audioio_core_api_burstmode_control(audioio_burst_ctrl_t *burst_ctrl)
{
	int error = 0;
	transducer_context_t *ptr = NULL;
	int32 burst_fifo_interrupt_sample_count;
	int32 burst_fifo_length;
	int32 burst_fifo_switch_frame;
	int32 burst_fifo_sample_number;

	burst_fifo_interrupt_sample_count =
				burst_ctrl->burst_fifo_interrupt_sample_count;
	burst_fifo_length = burst_ctrl->burst_fifo_length;
	burst_fifo_switch_frame = burst_ctrl->burst_fifo_switch_frame;
	burst_fifo_sample_number = burst_ctrl->burst_fifo_sample_number;

	ptr = &ptr_audio_codec_cnxt->transducer[burst_ctrl->channel_type];

	/*aquire mutex*/
	down(&(ptr_audio_codec_cnxt->audio_io_sem));

	if(AUDIOIO_COMMON_ON == burst_ctrl->ctrl_switch) {
		if (ptr->switch_to_burst_func)
			error = ptr->switch_to_burst_func(
			burst_fifo_interrupt_sample_count,burst_fifo_length,
			burst_fifo_switch_frame,burst_fifo_sample_number);
		} else {
			if (ptr->switch_to_normal_func)
				error = ptr->switch_to_normal_func();
		}
	/*release mutex*/
	up(&(ptr_audio_codec_cnxt->audio_io_sem));

	return error;
}
/**
 * \brief Convert channel index to array index
 * \param channel_index Channel Index of transducer
 * \return Array index corresponding to the specified channel index
 */

int convert_channel_index_to_array_index(AUDIOIO_CH_INDEX channel_index)
{
	if (channel_index & e_CHANNEL_1)
		return 0;
	else if (channel_index & e_CHANNEL_2)
		return 1;
	else if (channel_index & e_CHANNEL_3)
		return 2;
	else
		return 3;
}

/**
 * \brief Set individual gain along the HW path of a specified channel
 * \param gctrl_trnsdr Pointer to the structure __audioio_gain_ctrl_trnsdr
 * \return 0 on success otherwise negative error code
 */

int ste_audioio_core_api_gain_control_transducer(
				audioio_gain_ctrl_trnsdr_t *gctrl_trnsdr)
{
	transducer_context_t *ptr = NULL;
	AUDIOIO_CH_INDEX channel_index;
	int ch_array_index;
	uint16 gain_index;
	int32 gain_value;
	uint32 linear;
	int channel_type;
	int error;
	int min_gain, max_gain,gain;

	ptr = &ptr_audio_codec_cnxt->transducer[gctrl_trnsdr->channel_type];
	channel_index = gctrl_trnsdr->channel_index;
	gain_index = gctrl_trnsdr->gain_index;
	gain_value = gctrl_trnsdr->gain_value;
	linear = gctrl_trnsdr->linear;
	channel_type = gctrl_trnsdr->channel_type;

	ch_array_index = convert_channel_index_to_array_index(channel_index);
	if(linear){	/*Gain is in the range 0 to 100*/
		min_gain = gain_descriptor[channel_type][ch_array_index][gain_index].min_gain;
		max_gain = gain_descriptor[channel_type][ch_array_index][gain_index].max_gain;

		gain = ((gain_value * (max_gain - min_gain))/100) + min_gain;
	}else{
		/*Convert to db*/
		gain = gain_value/100;
	}
	gain_value = gain;

#if 1
	if (gain_index >= transducer_no_of_gains[channel_type])
		return -EINVAL;

	if(gain_value < gain_descriptor[channel_type][ch_array_index][gain_index].min_gain)
		return -EINVAL;

	if (gain_value > gain_descriptor[channel_type][ch_array_index][gain_index].max_gain)
		return -EINVAL;

#endif

	/*aquire mutex*/
	down(&(ptr_audio_codec_cnxt->audio_io_sem));

	error = ptr->set_gain_func(channel_index,gain_index,gain_value,linear);
	if (0 == error)
		ste_audioio_core_api_store_data(channel_index ,
							ptr->gain, gain_value);


	/*release mutex*/
	up(&(ptr_audio_codec_cnxt->audio_io_sem));

	return error;
}
/**
 * \brief Get individual gain along the HW path of a specified channel
 * \param gctrl_trnsdr Pointer to the structure __audioio_gain_ctrl_trnsdr
 * \return 0 on success otherwise negative error code
 */


int ste_audioio_core_api_gain_query_transducer(
				audioio_gain_ctrl_trnsdr_t *gctrl_trnsdr)
{
	transducer_context_t *ptr = NULL;
	AUDIOIO_CH_INDEX channel_index;
	uint16 gain_index;
	uint32 linear;
	int left_volume, right_volume;
	int32 max_gain,min_gain;
	int ch_array_index;

	ptr = &ptr_audio_codec_cnxt->transducer[gctrl_trnsdr->channel_type];
	channel_index = gctrl_trnsdr->channel_index;
	gain_index = gctrl_trnsdr->gain_index;
	linear = gctrl_trnsdr->linear;

	ptr->get_gain_func(&left_volume,&right_volume,gain_index);

	ch_array_index = convert_channel_index_to_array_index(channel_index);
	max_gain = gain_descriptor[gctrl_trnsdr->channel_type][ch_array_index][gain_index].max_gain;
	min_gain = gain_descriptor[gctrl_trnsdr->channel_type][ch_array_index][gain_index].min_gain;

	switch(channel_index)
	{
        case e_CHANNEL_1:
            gctrl_trnsdr->gain_value = linear? min_gain+left_volume*(max_gain-min_gain)/100 : left_volume;
            break;
        case e_CHANNEL_2:
            gctrl_trnsdr->gain_value = linear? min_gain+right_volume*(max_gain-min_gain)/100 : right_volume;
            break;
		case e_CHANNEL_3:
			break;
		case e_CHANNEL_4:
			break;
        case e_CHANNEL_ALL:
            if (left_volume == right_volume)
			{
                gctrl_trnsdr->gain_value = linear? min_gain+right_volume*(max_gain-min_gain)/100 : right_volume;
            }
	}

	return 0;
}


