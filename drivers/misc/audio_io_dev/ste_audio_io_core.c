/*
 * Copyright (C) ST-Ericsson SA 2010
 * Author: Deepak KARDA/ deepak.karda@stericsson.com for ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <mach/bit_mask.h>
#include <mach/ste_audio_io_vibrator.h>
#include "ste_audio_io_core.h"
#include "ste_audio_io_hwctrl_common.h"
#include "ste_audio_io_ab8500_reg_defs.h"

static struct audiocodec_context_t *ptr_audio_codec_cnxt;

static struct clk *clk_ptr_msp1;
#define AB8500_ED	0
#define AB8500_CUT1	1

int ste_audio_io_core_api_init_data(struct platform_device *pdev)
{
	ptr_audio_codec_cnxt = kmalloc(sizeof(struct audiocodec_context_t),
						GFP_KERNEL);
	if (!ptr_audio_codec_cnxt)
		return -ENOMEM;

	memset(ptr_audio_codec_cnxt, 0, sizeof(*ptr_audio_codec_cnxt));
	ptr_audio_codec_cnxt->dev = &pdev->dev;
	mutex_init(&(ptr_audio_codec_cnxt->audio_io_mutex));
	ste_audio_io_init_transducer_cnxt();
	return 0;
}

void ste_audio_io_init_transducer_cnxt(void)
{
	struct transducer_context_t *ptr = NULL;

	/* initializing HEADSET properties */
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

	/* initializing EARPIECE properties */
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


	/* initializing IHF properties */
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


	/* initializing VIBL properties */
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

	/* initializing VIBR properties */
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

	/* initializing MIC1A properties */
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


	/* initializing MIC1B properties */
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

	/* initializing MIC2 properties */
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

	/* initializing LIN properties */
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

	/* initializing DMIC12 properties */
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

	/* initializing DMIC34 properties */
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

	/* initializing DMIC56 properties */
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


	/* initializing FMRx properties */
	ptr = &ptr_audio_codec_cnxt->transducer[FMRX_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_fmrx;
	ptr->pwr_down_func = ste_audio_io_power_down_fmrx;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = NULL;
	ptr->get_gain_func = NULL;
	ptr->mute_func =  NULL;
	ptr->unmute_func = NULL;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = NULL;
	ptr->disable_fade_func = NULL;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;

	/* initializing FMTx properties */
	ptr = &ptr_audio_codec_cnxt->transducer[FMTX_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_fmtx;
	ptr->pwr_down_func = ste_audio_io_power_down_fmtx;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = NULL;
	ptr->get_gain_func = NULL;
	ptr->mute_func =  NULL;
	ptr->unmute_func = NULL;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = NULL;
	ptr->disable_fade_func = NULL;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;

	/* initializing BLUETOOTH properties */
	ptr = &ptr_audio_codec_cnxt->transducer[BLUETOOTH_CH];
	ptr->pwr_up_func = ste_audio_io_power_up_bluetooth;
	ptr->pwr_down_func = ste_audio_io_power_down_bluetooth;
	ptr->pwr_state_func = NULL;
	ptr->set_gain_func = NULL;
	ptr->get_gain_func = NULL;
	ptr->mute_func =  NULL;
	ptr->unmute_func = NULL;
	ptr->mute_state_func = NULL;
	ptr->enable_fade_func = NULL;
	ptr->disable_fade_func = NULL;
	ptr->switch_to_burst_func = NULL;
	ptr->switch_to_normal_func = NULL;
}


void ste_audio_io_core_api_free_data(void)
{
	kfree(ptr_audio_codec_cnxt);
}

static int return_device_id(void)
{
	__u8 data;

	data = ab8500_read(AB8500_MISC, (AB8500_REV_REG & MASK_ALL8));
	if (((data & MASK_QUARTET1) == AB8500_VER_1_0) ||
				((data & MASK_QUARTET1) == AB8500_VER_1_1)) {
		/* V1 version */
		return AB8500_CUT1;
	}
	/* V0 version */
	return AB8500_ED;
}

int ste_audio_io_core_api_powerup_audiocodec(int power_client)
{
	int error = 0;

	/* aquire mutex */
	mutex_lock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	/*
	 * If there is no power client registered, power up
	 * common audio blocks for audio and vibrator
	 */
	if (!ptr_audio_codec_cnxt->power_client) {
		int acodec_device_id;
		__u8 data, old_data;

		acodec_device_id = return_device_id();

		old_data = HW_REG_READ(AB8500_CTRL3_REG);

		/* Enable 32 Khz clock signal on Clk32KOut2 ball */
		data = (~CLK_32K_OUT2_DISABLE) & old_data;
		HW_REG_WRITE(AB8500_CTRL3_REG, data);

		data = INACTIVE_RESET_AUDIO | old_data;
		HW_REG_WRITE(AB8500_CTRL3_REG, data);

		old_data = HW_REG_READ(AB8500_SYSULPCLK_CTRL1_REG);

		if (AB8500_CUT1 == acodec_device_id)
			data = (ENABLE_AUDIO_CLK_TO_AUDIO_BLK |
				 AB8500_REQ_SYS_CLK | old_data);
		else
			data = ENABLE_AUDIO_CLK_TO_AUDIO_BLK | old_data;

		HW_REG_WRITE(AB8500_SYSULPCLK_CTRL1_REG, data);

		old_data = HW_REG_READ(AB8500_REGU_MISC1_REG);
		data = ENABLE_VINTCORE12_SUPPLY | old_data;
		HW_REG_WRITE(AB8500_REGU_MISC1_REG, data);

		old_data = HW_REG_READ(AB8500_REGU_VAUDIO_SUPPLY_REG);
		data = (VAMIC2_ENABLE | VAMIC1_ENABLE | VDMIC_ENABLE |
			VAUDIO_ENABLE) | old_data;
		HW_REG_WRITE(AB8500_REGU_VAUDIO_SUPPLY_REG, data);

		if (AB8500_CUT1 == acodec_device_id) {
			old_data = HW_REG_READ(AB8500_GPIO_DIR4_REG);
			data = (GPIO27_DIR_OUTPUT | GPIO29_DIR_OUTPUT |
				GPIO31_DIR_OUTPUT) | old_data;
			HW_REG_WRITE(AB8500_GPIO_DIR4_REG, data);
		}

		error = HW_REG_WRITE(SOFTWARE_RESET_REG, SW_RESET);
		if (error != 0) {
			dev_err(ptr_audio_codec_cnxt->dev,
				"Software reset error=%d", error);
			goto err_cleanup;
		}

		error = HW_ACODEC_MODIFY_WRITE(POWER_UP_CONTROL_REG,
				(DEVICE_POWER_UP|ANALOG_PARTS_POWER_UP), 0);
		if (error != 0) {
			dev_err(ptr_audio_codec_cnxt->dev,
				"Device Power Up, error=%d", error);
			goto err_cleanup;
		}
	}
	/* Save information that given client already powered up audio block */
	ptr_audio_codec_cnxt->power_client |= power_client;

	/* If audio block requested power up, turn on additional audio blocks */
	if (power_client == STE_AUDIOIO_POWER_AUDIO) {
		ptr_audio_codec_cnxt->audio_codec_powerup++;

		clk_ptr_msp1 = clk_get_sys("msp1", NULL);
		if (!IS_ERR(clk_ptr_msp1)) {
			error = clk_enable(clk_ptr_msp1);
			if (error)
				goto err_cleanup;
		} else {
			error = -EFAULT;
			goto err_cleanup;
		}

		error = stm_gpio_altfuncenable(GPIO_ALT_MSP_1);
		if (error)
			goto err_cleanup;

		error = HW_ACODEC_MODIFY_WRITE(IF0_IF1_MASTER_CONF_REG,
						EN_MASTGEN, 0);
		if (error != 0) {
			dev_err(ptr_audio_codec_cnxt->dev,
				"Enable Master Generator, error=%d", error);
			goto err_cleanup;
		}

		error = HW_ACODEC_MODIFY_WRITE(TDM_IF_BYPASS_B_FIFO_REG,
						IF0_MASTER, 0);
		if (error != 0) {
			dev_err(ptr_audio_codec_cnxt->dev,
				"IF0: Master Mode, error=%d", error);
			goto err_cleanup;
		}

		/* Configuring IF0 */

		error = HW_ACODEC_MODIFY_WRITE(IF0_IF1_MASTER_CONF_REG,
						BITCLK_OSR_N_256, 0);
		if (error != 0) {
			dev_err(ptr_audio_codec_cnxt->dev,
				"IF0: Enable FsBitClk & FSync error=%d", error);
			goto err_cleanup;
		}

		error = HW_REG_WRITE(IF0_CONF_REG, IF_DELAYED | TDM_FORMAT |
								WORD_LENGTH_20);
		if (error != 0) {
			dev_err(ptr_audio_codec_cnxt->dev,
				"IF0: TDM Format 16 Bits word length, error=%d",
				error);
			goto err_cleanup;
		}
	}
err_cleanup:
	/* release mutex */
	mutex_unlock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	return error;
}

int ste_audio_io_core_api_powerdown_audiocodec(int power_client)
{
	int error = 0;
	/* aquire mutex */
	mutex_lock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	/* Update power client status */
	if (power_client == STE_AUDIOIO_POWER_AUDIO) {
		ptr_audio_codec_cnxt->audio_codec_powerup--;
		if (!ptr_audio_codec_cnxt->audio_codec_powerup) {
			ptr_audio_codec_cnxt->power_client &= ~power_client;
			clk_disable(clk_ptr_msp1);
			clk_put(clk_ptr_msp1);
			error = stm_gpio_altfuncdisable(GPIO_ALT_MSP_1);
			if (error)
				goto err_cleanup;
		}
	} else
		ptr_audio_codec_cnxt->power_client &= ~power_client;

	/* If no power client registered, power down audio block */
	if (!ptr_audio_codec_cnxt->power_client) {
		error = HW_ACODEC_MODIFY_WRITE(POWER_UP_CONTROL_REG,
				0, (DEVICE_POWER_UP|ANALOG_PARTS_POWER_UP));
		if (error != 0) {
			dev_err(ptr_audio_codec_cnxt->dev,
		"Device Power Down and Analog Parts Power Down error = % d ",
							error);
			goto err_cleanup;
		}
	}

err_cleanup:
	/* release mutex */
	mutex_unlock(&(ptr_audio_codec_cnxt->audio_io_mutex));
	return error;
}
/**
 * @brief Read from AB8500 device
 * @dev_data Pointer to the structure __audioio_data
 * @return 0
 */

int ste_audio_io_core_api_access_read(struct audioio_data_t *dev_data)
{
	int reg;
	if (NULL == dev_data)
		return -EFAULT;
	reg = (dev_data->block<<SHIFT_BYTE1)|(dev_data->addr&MASK_ALL8);
	dev_data->data = HW_REG_READ(reg);
	return 0;
}
/**
 * @brief Write on AB8500 device
 * @dev_data Pointer to the structure __audioio_data
 * @return 0 on success otherwise negative error code
 */
int ste_audio_io_core_api_access_write(struct audioio_data_t *dev_data)
{
	int retval, reg;
	if (NULL == dev_data)
		return -EFAULT;

	reg = (dev_data->block<<SHIFT_BYTE1)|(dev_data->addr&MASK_ALL8);
	retval = HW_REG_WRITE(reg, dev_data->data);

	return retval;
}
/**
 * @brief Store the power and mute status of transducer
 * @channel_index Channel-index of transducer
 * @ptr Array storing the status
 * @value status being stored
 * @return 0 on success otherwise negative error code
 */

void ste_audio_io_core_api_store_data(enum AUDIOIO_CH_INDEX channel_index,
							int *ptr, int value)
{
	if (channel_index & e_CHANNEL_1)
		ptr[0] = value;

	if (channel_index & e_CHANNEL_2)
		ptr[1] = value;

	if (channel_index & e_CHANNEL_3)
		ptr[2] = value;

	if (channel_index & e_CHANNEL_4)
		ptr[3] = value;
}
/**
 * @brief Get power or mute status on a specific channel
 * @channel_index Channel-index of the transducer
 * @ptr Pointer to is_power_up array or is_muted array
 * @return status of control switch
 */
enum AUDIOIO_COMMON_SWITCH ste_audio_io_core_api_get_status(
				enum AUDIOIO_CH_INDEX channel_index, int *ptr)
{
	if (channel_index & e_CHANNEL_1) {
		if (AUDIOIO_TRUE == ptr[0])
			return AUDIOIO_COMMON_ON;
		else
			return AUDIOIO_COMMON_OFF;
	}

	if (channel_index & e_CHANNEL_2) {
		if (AUDIOIO_TRUE == ptr[1])
			return AUDIOIO_COMMON_ON;
		else
			return AUDIOIO_COMMON_OFF;
	}

	if (channel_index & e_CHANNEL_3) {
		if (AUDIOIO_TRUE == ptr[2])
			return AUDIOIO_COMMON_ON;
		else
			return AUDIOIO_COMMON_OFF;
	}

	if (channel_index & e_CHANNEL_4) {
		if (AUDIOIO_TRUE == ptr[3])
			return AUDIOIO_COMMON_ON;
		else
			return AUDIOIO_COMMON_OFF;
	}
	return 0;
}

int ste_audio_io_core_api_acodec_power_control(struct audioio_acodec_pwr_ctrl_t
							*audio_acodec_pwr_ctrl)
{
	int error = 0;
	if (audio_acodec_pwr_ctrl->ctrl_switch == AUDIOIO_COMMON_ON)
		error = ste_audio_io_core_api_powerup_audiocodec(
				STE_AUDIOIO_POWER_AUDIO);
	else
		error = ste_audio_io_core_api_powerdown_audiocodec(
				STE_AUDIOIO_POWER_AUDIO);

	return error;
}
/**
 * @brief Control for powering on/off HW components on a specific channel
 * @pwr_ctrl Pointer to the structure __audioio_pwr_ctrl
 * @return 0 on success otherwise negative error code
 */
int ste_audio_io_core_api_power_control_transducer(
					struct audioio_pwr_ctrl_t *pwr_ctrl)
{
	int error = 0;
	struct transducer_context_t *ptr = NULL;
	enum AUDIOIO_CH_INDEX channel_index;

	channel_index = pwr_ctrl->channel_index;

	ptr = &ptr_audio_codec_cnxt->transducer[pwr_ctrl->channel_type];

	/* aquire mutex */
	mutex_lock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	if (AUDIOIO_COMMON_ON == pwr_ctrl->ctrl_switch) {
		if (ptr->pwr_up_func) {
			error = ptr->pwr_up_func(pwr_ctrl->channel_index,
						ptr_audio_codec_cnxt->dev);
			if (0 == error) {
				ste_audio_io_core_api_store_data(channel_index,
						ptr->is_power_up, AUDIOIO_TRUE);
			}
		}
	} else {
		if (ptr->pwr_down_func) {
			error = ptr->pwr_down_func(pwr_ctrl->channel_index,
						ptr_audio_codec_cnxt->dev);
			if (0 == error) {
				ste_audio_io_core_api_store_data(channel_index,
					ptr->is_power_up, AUDIOIO_FALSE);
			}
		}
	}

	/* release mutex */
	mutex_unlock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	return error;
}
/**
 * @brief Query power state of HW path on specified channel
 * @pwr_ctrl Pointer to the structure __audioio_pwr_ctrl
 * @return 0 on success otherwise negative error code
 */

int ste_audio_io_core_api_power_status_transducer(
					struct audioio_pwr_ctrl_t *pwr_ctrl)
{

	struct transducer_context_t *ptr = NULL;
	enum AUDIOIO_CH_INDEX channel_index;

	channel_index = pwr_ctrl->channel_index;

	ptr = &ptr_audio_codec_cnxt->transducer[pwr_ctrl->channel_type];


	/* aquire mutex */
	mutex_lock(&(ptr_audio_codec_cnxt->audio_io_mutex));


	pwr_ctrl->ctrl_switch = ste_audio_io_core_api_get_status(channel_index,
							ptr->is_power_up);

	/* release mutex */
	mutex_unlock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	return 0;

}

int ste_audio_io_core_api_loop_control(struct audioio_loop_ctrl_t *loop_ctrl)
{
	return 0;
}

int ste_audio_io_core_api_loop_status(struct audioio_loop_ctrl_t *loop_ctrl)
{
	return 0;
}

int ste_audio_io_core_api_get_transducer_gain_capability(
					struct audioio_get_gain_t *get_gain)
{
	return 0;
}

int ste_audio_io_core_api_gain_capabilities_loop(
					struct audioio_gain_loop_t *gain_loop)
{
	return 0;
}

int ste_audio_io_core_api_supported_loops(
				struct audioio_support_loop_t *support_loop)
{
	return 0;
}

int ste_audio_io_core_api_gain_descriptor_transducer(
				struct audioio_gain_desc_trnsdr_t *gdesc_trnsdr)
{
	return 0;
}
/**
 * @brief Control for muting a specific channel in HW
 * @mute_trnsdr Pointer to the structure __audioio_mute_trnsdr
 * @return 0 on success otherwise negative error code
 */
int ste_audio_io_core_api_mute_control_transducer(
				struct audioio_mute_trnsdr_t *mute_trnsdr)
{
	int error = 0;
	struct transducer_context_t *ptr = NULL;
	enum AUDIOIO_CH_INDEX channel_index;

	channel_index = mute_trnsdr->channel_index;

	ptr = &ptr_audio_codec_cnxt->transducer[mute_trnsdr->channel_type];

	/* aquire mutex */
	mutex_lock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	if (AUDIOIO_COMMON_ON == mute_trnsdr->ctrl_switch) {
		if (ptr->mute_func) {
			error = ptr->mute_func(mute_trnsdr->channel_index,
						ptr_audio_codec_cnxt->dev);
		    if (0 == error) {
				ste_audio_io_core_api_store_data(channel_index ,
						ptr->is_muted, AUDIOIO_TRUE);
			}
		}
	} else {
		if (ptr->unmute_func) {
			if (0 == ptr->unmute_func(channel_index, ptr->gain,
						ptr_audio_codec_cnxt->dev)) {
				ste_audio_io_core_api_store_data(channel_index,
						ptr->is_muted, AUDIOIO_FALSE);
			}
		}
	}

	/* release mutex */
	mutex_unlock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	return error;
}
/**
 * @brief Query state of mute on specified channel
 * @mute_trnsdr Pointer to the structure __audioio_mute_trnsdr
 * @return 0 on success otherwise negative error code
 */
int ste_audio_io_core_api_mute_status_transducer(
				struct audioio_mute_trnsdr_t *mute_trnsdr)
{
	struct transducer_context_t *ptr = NULL;
	enum AUDIOIO_CH_INDEX channel_index;

	channel_index = mute_trnsdr->channel_index;

	ptr = &ptr_audio_codec_cnxt->transducer[mute_trnsdr->channel_type];

	/* aquire mutex */
	mutex_lock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	mute_trnsdr->ctrl_switch = ste_audio_io_core_api_get_status(
				channel_index, ptr->is_muted);
	/* release mutex */
	mutex_unlock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	return 0;
}
/**
 * @brief control the fading on the transducer called on.
 * @fade_ctrl Pointer to the structure __audioio_fade_ctrl
 * @return 0 on success otherwise negative error code
 */

int ste_audio_io_core_api_fading_control(struct audioio_fade_ctrl_t *fade_ctrl)
{
	int error = 0;
	struct transducer_context_t *ptr = NULL;

	ptr = &ptr_audio_codec_cnxt->transducer[fade_ctrl->channel_type];

	/* aquire mutex */
	mutex_lock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	if (AUDIOIO_COMMON_ON == fade_ctrl->ctrl_switch)
		error = ptr->enable_fade_func(ptr_audio_codec_cnxt->dev);

	else
		error = ptr->disable_fade_func(ptr_audio_codec_cnxt->dev);


	/* release mutex */
	mutex_unlock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	return error;
}
/**
 * @brief control the low power mode of headset.
 * @burst_ctrl Pointer to the structure __audioio_burst_ctrl
 * @return 0 on success otherwise negative error code
 */

int ste_audio_io_core_api_burstmode_control(
					struct audioio_burst_ctrl_t *burst_ctrl)
{
	int error = 0;
	struct transducer_context_t *ptr = NULL;
	int burst_fifo_switch_frame;

	burst_fifo_switch_frame = burst_ctrl->burst_fifo_switch_frame;


	ptr = &ptr_audio_codec_cnxt->transducer[burst_ctrl->channel_type];

	/* aquire mutex */
	mutex_lock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	if (AUDIOIO_COMMON_ON == burst_ctrl->ctrl_switch) {
		if (ptr->switch_to_burst_func)
			error = ptr->switch_to_burst_func(
					burst_fifo_switch_frame,
						ptr_audio_codec_cnxt->dev);
		} else {
			if (ptr->switch_to_normal_func)
				error = ptr->switch_to_normal_func(
						ptr_audio_codec_cnxt->dev);
		}
	/* release mutex */
	mutex_unlock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	return error;
}
/**
 * @brief Convert channel index to array index
 * @channel_index Channel Index of transducer
 * @return Array index corresponding to the specified channel index
 */

int convert_channel_index_to_array_index(enum AUDIOIO_CH_INDEX channel_index)
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
 * @brief Set individual gain along the HW path of a specified channel
 * @gctrl_trnsdr Pointer to the structure __audioio_gain_ctrl_trnsdr
 * @return 0 on success otherwise negative error code
 */

int ste_audio_io_core_api_gain_control_transducer(
				struct audioio_gain_ctrl_trnsdr_t *gctrl_trnsdr)
{
	struct transducer_context_t *ptr = NULL;
	enum AUDIOIO_CH_INDEX channel_index;
	int ch_array_index;
	u16 gain_index;
	int gain_value;
	u32 linear;
	int channel_type;
	int error;
	int min_gain, max_gain, gain;

	ptr = &ptr_audio_codec_cnxt->transducer[gctrl_trnsdr->channel_type];
	channel_index = gctrl_trnsdr->channel_index;
	gain_index = gctrl_trnsdr->gain_index;
	gain_value = gctrl_trnsdr->gain_value;
	linear = gctrl_trnsdr->linear;
	channel_type = gctrl_trnsdr->channel_type;

	ch_array_index = convert_channel_index_to_array_index(channel_index);
	if (linear) {	/* Gain is in the range 0 to 100 */
		min_gain = gain_descriptor[channel_type]\
				[ch_array_index][gain_index].min_gain;
		max_gain = gain_descriptor[channel_type]\
				[ch_array_index][gain_index].max_gain;

		gain = ((gain_value * (max_gain - min_gain))/100) + min_gain;
	} else {
		/* Convert to db */
		gain = gain_value/100;
	}
	gain_value = gain;

#if 1
	if (gain_index >= transducer_no_of_gains[channel_type])
		return -EINVAL;

	if (gain_value < gain_descriptor[channel_type]\
				[ch_array_index][gain_index].min_gain)
		return -EINVAL;

	if (gain_value > gain_descriptor[channel_type]\
				[ch_array_index][gain_index].max_gain)
		return -EINVAL;

#endif

	/* aquire mutex */
	mutex_lock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	error = ptr->set_gain_func(channel_index,
					gain_index, gain_value, linear,
					ptr_audio_codec_cnxt->dev);
	if (0 == error)
		ste_audio_io_core_api_store_data(channel_index ,
							ptr->gain, gain_value);


	/* release mutex */
	mutex_unlock(&(ptr_audio_codec_cnxt->audio_io_mutex));

	return error;
}
/**
 * @brief Get individual gain along the HW path of a specified channel
 * @gctrl_trnsdr Pointer to the structure __audioio_gain_ctrl_trnsdr
 * @return 0 on success otherwise negative error code
 */


int ste_audio_io_core_api_gain_query_transducer(
				struct audioio_gain_ctrl_trnsdr_t *gctrl_trnsdr)
{
	struct transducer_context_t *ptr = NULL;
	enum AUDIOIO_CH_INDEX channel_index;
	u16 gain_index;
	u32 linear;
	int left_volume, right_volume;
	int max_gain, min_gain;
	int ch_array_index;

	ptr = &ptr_audio_codec_cnxt->transducer[gctrl_trnsdr->channel_type];
	channel_index = gctrl_trnsdr->channel_index;
	gain_index = gctrl_trnsdr->gain_index;
	linear = gctrl_trnsdr->linear;

	ptr->get_gain_func(&left_volume, &right_volume, gain_index,
						ptr_audio_codec_cnxt->dev);

	ch_array_index = convert_channel_index_to_array_index(channel_index);
	max_gain = gain_descriptor[gctrl_trnsdr->channel_type]\
				[ch_array_index][gain_index].max_gain;
	min_gain = gain_descriptor[gctrl_trnsdr->channel_type]\
				[ch_array_index][gain_index].min_gain;

	switch (channel_index) {
	case e_CHANNEL_1:
		gctrl_trnsdr->gain_value = linear ? \
		min_gain+left_volume*(max_gain-min_gain)/100 : left_volume;
		break;
	case e_CHANNEL_2:
		gctrl_trnsdr->gain_value = linear ? \
		min_gain+right_volume*(max_gain-min_gain)/100 : right_volume;
		break;
	case e_CHANNEL_3:
		break;
	case e_CHANNEL_4:
		break;
	case e_CHANNEL_ALL:
		if (left_volume == right_volume) {
			if (linear)
				gctrl_trnsdr->gain_value =
				min_gain+right_volume*(max_gain-min_gain)/100;
			else
				gctrl_trnsdr->gain_value  = right_volume;
			}
	}

	return 0;
}


int ste_audio_io_core_api_fsbitclk_control(
			struct audioio_fsbitclk_ctrl_t *fsbitclk_ctrl)
{
	int error = 0;

	if (AUDIOIO_COMMON_ON == fsbitclk_ctrl->ctrl_switch)
		error = HW_ACODEC_MODIFY_WRITE(IF0_IF1_MASTER_CONF_REG,
							EN_FSYNC_BITCLK, 0);
	else
		error = HW_ACODEC_MODIFY_WRITE(IF0_IF1_MASTER_CONF_REG, 0,
							EN_FSYNC_BITCLK);


	return error;
}
int ste_audio_io_core_api_pseudoburst_control(
			struct audioio_pseudoburst_ctrl_t *pseudoburst_ctrl)
{
	int error = 0;

	return error;
}
int ste_audio_io_core_debug(int x)
{
	debug_audioio(x);

return 0;
}

/**
 * ste_audioio_vibrator_alloc()
 * @client:		Client id which allocates vibrator
 * @mask:		Mask against which vibrator usage is checked
 *
 * This function allocates vibrator.
 * Mask is added here as audioio driver controls left and right vibrator
 * separately (can work independently). In case when audioio has allocated
 * one of its channels (left or right) it should be still able to allocate
 * the other channel.
 *
 * Returns:
 *	0 - Success
 *	-EBUSY - other client already registered
 **/
int ste_audioio_vibrator_alloc(int client, int mask)
{
	int error = 0;

	mutex_lock(&ptr_audio_codec_cnxt->audio_io_mutex);

	/* Check if other client is already using vibrator */
	if (ptr_audio_codec_cnxt->vibra_client & ~mask)
		error = -EBUSY;
	else
		ptr_audio_codec_cnxt->vibra_client |= client;

	mutex_unlock(&ptr_audio_codec_cnxt->audio_io_mutex);
	return error;
}

/**
 * ste_audioio_vibrator_release()
 * @client:	     Client id which releases vibrator
 *
 * This function releases vibrator
 **/
void ste_audioio_vibrator_release(int client)
{
	mutex_lock(&ptr_audio_codec_cnxt->audio_io_mutex);
	ptr_audio_codec_cnxt->vibra_client &= ~client;
	mutex_unlock(&ptr_audio_codec_cnxt->audio_io_mutex);
}

/**
 * ste_audioio_vibrator_pwm_control()
 * @client:		Client id which will use vibrator
 * @left_speed:		Left vibrator speed
 * @right_speed:	Right vibrator speed
 *
 * This function controls vibrator using PWM source
 *
 * Returns:
 *      0 - success
 *	-EBUSY - Vibrator already used
 **/
int ste_audioio_vibrator_pwm_control(int client, unsigned char left_speed,
					unsigned char right_speed)
{
	int error = 0;

	/* Try to allocate vibrator for given client */
	error = ste_audioio_vibrator_alloc(client, client);
	if (error)
		return error;

	/* Duty cycle supported by vibrator's PWM is 0-100 */
	if (left_speed > STE_AUDIOIO_VIBRATOR_MAX_SPEED)
		left_speed = STE_AUDIOIO_VIBRATOR_MAX_SPEED;

	if (right_speed > STE_AUDIOIO_VIBRATOR_MAX_SPEED)
		right_speed = STE_AUDIOIO_VIBRATOR_MAX_SPEED;

	if (left_speed || right_speed) {
		/* Power up audio block for vibrator */
		error = ste_audio_io_core_api_powerup_audiocodec(
						STE_AUDIOIO_POWER_VIBRA);
		if (error) {
			dev_err(ptr_audio_codec_cnxt->dev,
				"Audio power up failed %d", error);
			return error;
		}

		error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG,
					(EN_VIBL_MASK|EN_VIBR_MASK), 0);
		if (error) {
			dev_err(ptr_audio_codec_cnxt->dev,
				"Powerup Vibrator Class-D driver %d",
				error);
			return error;
		}

		error = HW_REG_WRITE(VIB_DRIVER_CONF_REG, MASK_ALL8);
		if (error) {
			dev_err(ptr_audio_codec_cnxt->dev,
				"Enable Vibrator PWM generator %d",
				error);
			return error;
		}
	}

	error = HW_REG_WRITE(PWM_VIBNL_CONF_REG, 0);
	if (error) {
		dev_err(ptr_audio_codec_cnxt->dev,
			"Write Left Vibrator negative PWM %d", error);
		goto err_cleanup;
	}

	error = HW_REG_WRITE(PWM_VIBPL_CONF_REG, left_speed);
	if (error) {
		dev_err(ptr_audio_codec_cnxt->dev,
			"Write Left Vibrator positive PWM %d", error);
		goto err_cleanup;
	}

	error = HW_REG_WRITE(PWM_VIBNR_CONF_REG, 0);
	if (error) {
		dev_err(ptr_audio_codec_cnxt->dev,
			"Write Right Vibrator negative PWM %d", error);
		goto err_cleanup;
	}

	error = HW_REG_WRITE(PWM_VIBPR_CONF_REG, right_speed);
	if (error) {
		dev_err(ptr_audio_codec_cnxt->dev,
			"Write Right Vibrator positive PWM %d", error);
		goto err_cleanup;
	}

	if (!left_speed && !right_speed) {
		error = HW_REG_WRITE(VIB_DRIVER_CONF_REG, 0);
		if (error) {
			dev_err(ptr_audio_codec_cnxt->dev,
				"Disable PWM Vibrator generator %d",
				error);
			goto err_cleanup;
		}

		error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG,
					0, (EN_VIBL_MASK|EN_VIBR_MASK));
		if (error) {
			dev_err(ptr_audio_codec_cnxt->dev,
				"Power down Vibrator Class-D driver %d",
				error);
			goto err_cleanup;
		}

		/* Power down audio block */
		error = ste_audio_io_core_api_powerdown_audiocodec(
				STE_AUDIOIO_POWER_VIBRA);
		if (error) {
			dev_err(ptr_audio_codec_cnxt->dev,
				"Audio power down failed %d", error);
			goto err_cleanup;
		}
	}

err_cleanup:
	/* Release client */
	if (!left_speed && !right_speed)
		ste_audioio_vibrator_release(client);
	return error;
}
EXPORT_SYMBOL(ste_audioio_vibrator_pwm_control);
