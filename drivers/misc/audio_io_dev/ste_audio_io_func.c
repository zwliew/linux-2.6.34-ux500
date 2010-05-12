/*
* Overview:
*   Implementation of IOCTLs for AB8500
*
* Copyright (C) 2009 ST Ericsson
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/


#include "ste_audio_io_func.h"
#include <mach/ab8500.h>
#include "ste_audio_io_ab8500_reg_defs.h"
#include "ste_audio_io_hwctrl_common.h"

/**
 * \brief Modify the specified register
 * \param reg Register
 * \param mask_set Bit to be set
 * \param mask_clear Bit to be cleared
 * \return 0 on success otherwise negative error code
 */

unsigned int ab8500_acodec_modify_write(unsigned int reg,u8 mask_set,
								u8 mask_clear)
{
	u8 value8,retval=0;
	value8 = HW_REG_READ(reg);
	/*clear the specified bit*/
	value8 &= ~mask_clear;
	/*set the asked bit*/
	value8 |= mask_set;
	retval = HW_REG_WRITE(reg,value8);
	return retval;
}

/**
 * \brief Power up headset on a specific channel
 * \param channel_index Channel-index of headset
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_power_up_headset(AUDIOIO_CH_INDEX channel_index)
{
	int error=0;
	unsigned char initialVal_DA=0;

	/*Check if HS PowerUp request is mono or Stereo channel*/
	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2 ))) {
		printk("HS should have mono or stereo channels");
		return -EINVAL;
	}

	/*Enable DA1 for HSL*/
	if (channel_index & e_CHANNEL_1) {
		initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);

		if (EN_DA1 & initialVal_DA)
			return 0;

		MSG0("Data sent to DA1 from Slot 08");
		error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA1_REG,
							SLOT08_FOR_DA_PATH,0);
		if (0 != error) {
			printk("ERROR: Data sent to DA1 from Slot 08 %d",error);
			return error;
		}

		MSG0("DA_IN1 path mixed with sidetone FIR");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1,
								DA1_TO_HSL, 0);
		if (0!= error) {
			printk("ERROR: DA_IN1 path mixed with "
						"sidetone FIR %d", error);
			return error;
		}

		MSG0("Power Up HSL mono channel");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,
								EN_DA1,0);
		if (0 != error) {
			printk("ERROR: Power up HSL %d", error);
			return error;
		}

		/*Power Up HSL DAC driver*/
		MSG0("Power Up HSL DAC driver");
		error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG,
							POWER_UP_HSL_DAC,0);
		if (error != 0) {
			printk("ERROR: Power Up HSL DAC driver %d", error);
			return error;
		}

		/*Power Up HSL Driver*/
		MSG0("Power Up HSL Driver");
		error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG,
								EN_HSL_MASK,0);
		if (error != 0) {
			printk("ERROR: Power Up HSL Driver %d", error);
			return error;
		}

		/*Power up HSL DAC and digital path*/
		MSG0("Power up HSL DAC and digital path");
		error =HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG,
								EN_HSL_MASK,0);
		if (error != 0) {
			printk("ERROR: Power up HSL DAC and digital path %d",
									error);
			return error;
		}

		/*TEST START*/
		/*Disable short detection. Pull Down output to ground,
		 Use local oscillator, Gain change without zero cross control*/
		MSG0("Disable short detection. Pull Down output to ground,"
					"Use local oscillator,"
				" Gain change without zero cross control");
		error = HW_ACODEC_MODIFY_WRITE(SHORT_CIRCUIT_DISABLE_REG,
			HS_SHORT_DIS|HS_PULL_DOWN_EN|HS_OSC_EN|HS_ZCD_DIS, 0);
		if (error != 0) {
		printk("ERROR: Disable short detection.Pull Down output to "
		"ground,Use local oscillator, Gain change without zero cross "
							"control %d",error);
			return error;
		}
		/*TEST END*/
		/*Enable Negative Charge Pump*/

		MSG0("Enable Negative Charge Pump");
		error =HW_ACODEC_MODIFY_WRITE(NCP_ENABLE_HS_AUTOSTART_REG,
								EN_NEG_CP,0);
		if (error != 0) {
			printk("ERROR: Enable Negative Charge Pump %d", error);
			return error;
		}
	}

	/*Enable DA2 for HSR*/
	if (channel_index & e_CHANNEL_2) {
		initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
		if (EN_DA2 & initialVal_DA)
			return 0;

		MSG0("Data sent to DA2 from Slot 09");
		error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA2_REG,
							SLOT09_FOR_DA_PATH, 0);
		if (error != 0) {
			printk("ERROR: Data sent to DA2 from Slot 09 %d",
									error);
			return error;
		}

		MSG0("DA_IN2 path mixed with sidetone FIR");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1, DA2_TO_HSR,
									0);
		if (error != 0) {
			printk("ERROR: DA_IN2 path mixed with sidetone FIR %d",
									error);
			return error;
		}

		MSG0("Power Up HSR mono channel");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,
								EN_DA2,0);
		if (error != 0) {
			printk("ERROR: Power up HSR %d", error);
			return error;
		}

					/*Power Up HSR DAC driver*/
		MSG0("Power Up HSR DAC driver");
		error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG,
							POWER_UP_HSR_DAC, 0);
		if (error != 0) {
			printk("ERROR: Power Up HSR DAC driver %d", error);
			return error;
		}

				/*Power Up HSR Driver*/
		MSG0("Power Up HSR Driver");
		error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG,
								EN_HSR_MASK,0);
		if (error != 0) {
			printk("ERROR: Power Up HSR Driver %d", error);
			return error;
		}

			/*Power up HSR DAC and digital path*/
		MSG0("Power up HSR DAC and digital path");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG,
								EN_HSR_MASK,0);
		if (error != 0) {
			printk("ERROR: Power up HSR DAC and digital path %d",
									error);
			return error;
		}

	/*TEST START .havent cleared the bits in power down.Disable short
	detection. Pull Down output to ground, Use local oscillator,
	Gain change without zero cross control*/

		MSG0("Disable short detection. Pull Down output to ground,\
		Use local oscillator");
		error = HW_ACODEC_MODIFY_WRITE(SHORT_CIRCUIT_DISABLE_REG,
			HS_SHORT_DIS|HS_PULL_DOWN_EN|HS_OSC_EN|HS_ZCD_DIS, 0);
		if (error != 0) {
			printk("ERROR: Disable short detection.Pull Down output"
			"to ground, Use local oscillator,Gain change without "
			"zero cross control %d",error);
			return error;
		}
				/*TEST END*/

			/*Enable Negative Charge Pump*/
		MSG0("Enable Negative Charge Pump");
		error = HW_ACODEC_MODIFY_WRITE(NCP_ENABLE_HS_AUTOSTART_REG,
								EN_NEG_CP,0);
		if (error != 0) {
			printk("ERROR: Enable Negative Charge Pump %d", error);
			return error;
		}
	}
	dump_acodec_registers(__FUNCTION__);
	return error;
}

/**
 * \brief Power down headset on a specific channel
 * \param channel_index Channel-index of headset
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_power_down_headset(AUDIOIO_CH_INDEX channel_index)
{
	int error=0;
	unsigned char initialVal_DA=0;

		/*Check if HS Power Down request is mono or Stereo channel*/
	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2 ))) {
		printk("HS should have mono or stereo channels");
		return -EINVAL;
	}

		/*Enable DA1 for HSL*/
	if (channel_index & e_CHANNEL_1) {
		initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
		if (!(initialVal_DA & EN_DA1))
			return 0;

		/*Power Down HSL DAC driver*/
		MSG0("Power Down HSL DAC driver");
		error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG, 0,
							POWER_UP_HSL_DAC);
		if (error != 0) {
			printk("ERROR: Power Down HSL DAC driver %d", error);
			return error;
		}

		/*Power Down HSL Driver*/
		MSG0("Power Down HSL Driver");
		error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG, 0,
								EN_HSL_MASK);
		if (error != 0) {
			printk("ERROR: Power Down HSL Driver %d", error);
			return error;
		}

		/*Power Down HSL DAC and digital path*/
		MSG0("Power Down HSL DAC and digital path");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG,0,
								EN_HSL_MASK);
		if (error != 0) {
			printk("ERROR: Power Down HSL DAC and digital path %d",
									error);
			return error;
		}

		MSG0("Disable DA1");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,0,
									EN_DA1);
		if (error != 0) {
			printk("ERROR: Disable DA1 %d", error);
			return error;
		}

		MSG0("Clear DA_IN1 path mixed with sidetone FIR");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1,0,DA1_TO_HSL);
		if (error != 0) {
			printk("ERROR: Clear DA_IN1 path mixed with sidetone "
								"FIR %d",error);
			return error;
		}

		MSG0("Data sent to DA1 cleared from Slot 08");
		error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA1_REG,0,
							SLOT08_FOR_DA_PATH);
		if (error != 0) {
			printk("ERROR: Data sent to DA1 cleared from Slot 08 "
								"%d",error);
			return error;
		}

			/*Disable Negative Charge Pump*/
		MSG0("Disable Negative Charge Pump");
		error = HW_ACODEC_MODIFY_WRITE(NCP_ENABLE_HS_AUTOSTART_REG,0,
								EN_NEG_CP);
		if (error != 0) {
			printk("ERROR: Disable Negative Charge Pump %d", error);
			return error;
		}
	}
	/*Enable DA2 for HSR*/

	if (channel_index & e_CHANNEL_2) {
		initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
		if (!(initialVal_DA & EN_DA2))
			return 0;

		/*Power Down HSR DAC driver*/
		MSG0("Power Down HSR DAC driver");
		error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG, 0,
							POWER_UP_HSR_DAC);
		if (error != 0) {
			printk("ERROR: Power Down HSR DAC driver %d", error);
			return error;
		}

			/*Power Down HSR Driver*/
		MSG0("Power Down HSR Driver");
		error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG,0,
								EN_HSR_MASK);
		if (error != 0) {
			printk("ERROR: Power Down HSR Driver %d", error);
			return error;
		}

			/*Power Down HSR DAC and digital path*/
		MSG0("Power Down HSR DAC and digital path");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG,0,
								EN_HSR_MASK);
		if (error != 0) {
			printk("ERROR: Power Down HSR DAC and digital path %d",
									error);
			return error;
		}

		MSG0("Disable DA2");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,
								0,EN_DA2);
		if (error != 0) {
			printk("ERROR: Disable DA2 %d", error);
			return error;
		}

		MSG0("Clear DA_IN2 path mixed with sidetone FIR");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1, 0,
								DA2_TO_HSR);
		if (error != 0) {
			printk("ERROR: Clear DA_IN2 path mixed with sidetone "
								"FIR %d",error);
			return error;
		}

		MSG0("Data sent to DA2 cleared from Slot 09");
		error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA2_REG, 0,
							SLOT09_FOR_DA_PATH);
		if (error != 0) {
			printk("ERROR: Data sent to DA2 cleared from Slot 09"
								" %d", error);
			return error;
		}

		/*Disable Negative Charge Pump*/
		MSG0("Disable Negative Charge Pump");
		error = HW_ACODEC_MODIFY_WRITE(NCP_ENABLE_HS_AUTOSTART_REG,
								0,EN_NEG_CP);
		if (error != 0) {
			printk("ERROR: Disable Negative Charge Pump %d", error);
			return error;
		}
	}
	dump_acodec_registers(__FUNCTION__);
	return error;
}

/**
 * \brief Mute headset on a specific channel
 * \param channel_index Headeset channel-index
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_mute_headset(AUDIOIO_CH_INDEX channel_index)
{
	int error=0;
		/*Check if HS Mute request is mono or Stereo channel*/
	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2 ))) {
		printk("HS should have mono or stereo channels");
		return -EINVAL;
	}

	if (channel_index & e_CHANNEL_1) {
		/*Mute HSL*/
	    MSG0("Mute HSL");
		error = HW_ACODEC_MODIFY_WRITE(MUTE_HS_EAR_REG, EN_HSL_MASK, 0);
		if (error != 0) {
			printk("ERROR: Mute HSL %d", error);
			return error;
		}
	}

	if (channel_index & e_CHANNEL_2) {
		/*Mute HSR*/
		MSG0("Mute HSR");
		error = HW_ACODEC_MODIFY_WRITE(MUTE_HS_EAR_REG, EN_HSR_MASK, 0);
		if (error != 0) {
			printk("ERROR: Mute HSR %d", error);
			return error;
		}
	}

	dump_acodec_registers(__FUNCTION__);
	return error;
}

/**
 * \brief Unmute headset on a specific channel
 * \param channel_index Headeset channel-index
 * \param gain Gain index of headset
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_unmute_headset(AUDIOIO_CH_INDEX channel_index, int *gain)
{
	int error = 0;

	/*Check if HS UnMute request is mono or Stereo channel*/
	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2 ))) {
		printk("HS should have mono or stereo channels");
		return -EINVAL;
	}

	if (channel_index & e_CHANNEL_1) {
		/*UnMute HSL*/
		MSG0("UnMute HSL");
		error = HW_ACODEC_MODIFY_WRITE(MUTE_HS_EAR_REG, 0, EN_HSL_MASK);
		if (error != 0) {
			printk("ERROR: UnMute HSL %d", error);
			return error;
		}
	}

	if (channel_index & e_CHANNEL_2) {
		/*UnMute HSR*/
		MSG0("UnMute HSR");
		error = HW_ACODEC_MODIFY_WRITE(MUTE_HS_EAR_REG, 0, EN_HSR_MASK);
		if (error != 0) {
			printk("ERROR: UnMute HSR %d", error);
			return error;
		}
	}
	dump_acodec_registers(__FUNCTION__);
	return error;
}

/**
 * \brief Enables fading of headset on a specific channel
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_enable_fade_headset(void)
{
	int error = 0;

	MSG0("Enable fading for HS");
	error = HW_ACODEC_MODIFY_WRITE(SHORT_CIRCUIT_DISABLE_REG,0, DIS_HS_FAD);
	if (error != 0) {
		printk("ERROR: Enable fading for HS %d", error);
		return error;
	}

	MSG0("Enable fading for HSL");
	error = HW_ACODEC_MODIFY_WRITE(DA1_DIGITAL_GAIN_REG, 0, DIS_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for HSL %d", error);
		return error;
	}

	error = HW_ACODEC_MODIFY_WRITE(HSL_EAR_DIGITAL_GAIN_REG, 0,
							DIS_DIG_GAIN_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for Digital Gain of HSL "
								"%d", error);
		return error;
	}

	MSG0("Enable fading for HSR");
	error = HW_ACODEC_MODIFY_WRITE(DA2_DIGITAL_GAIN_REG, 0, DIS_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for HSR %d", error);
		return error;
	}

	error = HW_ACODEC_MODIFY_WRITE(HSR_DIGITAL_GAIN_REG,0,
							DIS_DIG_GAIN_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for Digital Gain of HSR %d",error);
		return error;
	}

	return error;
}
/**
 * \brief Disables fading of headset on a specific channel
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_disable_fade_headset(void)
{
	int error = 0;
	MSG0("Disable fading for HS");
	error = HW_ACODEC_MODIFY_WRITE(SHORT_CIRCUIT_DISABLE_REG, DIS_HS_FAD,0);
	if (error != 0) {
		printk("ERROR: Disable fading for HS %d", error);
		return error;
	}

	MSG0("Disable fading for HSL");
	error = HW_ACODEC_MODIFY_WRITE(DA1_DIGITAL_GAIN_REG, DIS_FADING, 0);
	if  (error!= 0) {
		printk("ERROR: Disable fading for HSL %d", error);
		return error;
	}

	error = HW_ACODEC_MODIFY_WRITE(HSL_EAR_DIGITAL_GAIN_REG,
							DIS_DIG_GAIN_FADING,0);
	if (error != 0) {
		printk("ERROR: Disable fading for Digital Gain of HSL "
								"%d", error);
		return error;
	}

	MSG0("Disable fading for HSR");
	error = HW_ACODEC_MODIFY_WRITE(DA2_DIGITAL_GAIN_REG, DIS_FADING, 0);
	if (error != 0) {
		printk("ERROR: Disable fading for HSR %d", error);
		return error;
	}

	error = HW_ACODEC_MODIFY_WRITE(HSR_DIGITAL_GAIN_REG,
							DIS_DIG_GAIN_FADING,0);
	if (error != 0) {
		printk("ERROR: Disable fading for Digital Gain of HSR "
								"%d", error);
		return error;
	}
	return error;
}
/**
 * \brief Power up earpiece
 * \param channel_index Channel-index
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_power_up_earpiece(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	unsigned char initialVal_DA=0;

	/*Check if Earpiece PowerUp request is mono channel*/
	if (!(channel_index & e_CHANNEL_1)) {
		printk("Earpiece should have mono channel");
		return -EINVAL;
	}

	initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);

	/*Check if Earpiece is already powered up or DA1 being used by HS*/
	if (EN_DA1 & initialVal_DA)
		return 0;

	MSG0("DA_IN1 path mixed with sidetone FIR");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1,
							DA1_TO_HSL, 0);
	if (error != 0) {
		printk(KERN_ERR "DA_IN1 path mixed with sidetone FIR %d", error);
		return error;
	}

	/*Enable DA1*/
	MSG0("Data sent to DA1 from Slot 08");
	error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA1_REG,
							SLOT08_FOR_DA_PATH, 0);
	if (error != 0) {
		printk("ERROR: Data sent to DA1 from Slot 08 %d", error);
		return error;
	}

	MSG0("Enable DA1");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,EN_DA1,0);
	if (error != 0) {
		printk("ERROR: Enable DA1 %d", error);
		return error;
	}

	/*Power Up EAR class-AB driver*/
	MSG0("Power Up EAR class-AB driver");
	error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG, EN_EAR_MASK,0);
	if (error != 0) {
		printk("ERROR: Power Up EAR class-AB driver %d", error);
		return error;
	}

	/*Power up EAR DAC and digital path*/
	MSG0("Power up EAR DAC and digital path");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG,EN_EAR_MASK,0);
	if (error != 0) {
		printk("ERROR: Power up EAR DAC and digital path %d", error);
		return error;
	}
	dump_acodec_registers(__FUNCTION__);
	return error;
}
/**
 * \brief Power down earpiece
 * \param channel_index Channel-index
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_power_down_earpiece(AUDIOIO_CH_INDEX channel_index)
{
   int error = 0;
   unsigned char initialVal_DA=0;

	/*Check if Earpiece PowerDown request is mono channel*/
	if (!(channel_index & e_CHANNEL_1)) {
		printk("Earpiece should have mono channel");
		return -EINVAL;
	}

	/*Check if Earpiece is already powered down or DA1 being used by HS*/
	initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
	if (!(initialVal_DA & EN_DA1))
		return 0;

	/*Power Down EAR class-AB driver*/
	MSG0("Power Down EAR class-AB driver");
	error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG,0, EN_EAR_MASK);
	if (error != 0) {
		printk("ERROR: Power Down EAR class-AB driver %d", error);
		return error;
	}

		/*Power Down EAR DAC and digital path*/
	MSG0("Power Down EAR DAC and digital path");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG,0,EN_EAR_MASK);
	if (error != 0) {
		printk("ERROR: Power Down EAR DAC and digital path %d", error);
		return error;
	}

	MSG0("Clear DA_IN1 path mixed with sidetone FIR");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1,0,DA1_TO_HSL);
	if (error != 0) {
		printk(KERN_ERR "Clear DA_IN1 path mixed with sidetone FIR %d",error);
		return error;
	}

			/*Disable DA1*/
	MSG0("Disable DA1");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,0,EN_DA1);
	if (error != 0) {
		printk("ERROR: Disable DA1 %d", error);
		return error;
	}

	MSG0("Data sent to DA1 cleared from Slot 08");
	error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA1_REG, 0,
							SLOT08_FOR_DA_PATH);
	if (error != 0) {
		printk("ERROR: Data sent to DA1 cleared from Slot 08 %d",error);
		return error;
	}
	dump_acodec_registers(__FUNCTION__);
	return error;
}
/**
 * \brief Mute earpiece
 * \param channel_index Channel-index
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_mute_earpiece(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;

	/*Check if Earpiece Mute request is mono channel*/
	if (!(channel_index & e_CHANNEL_1)) {
		printk("Earpiece should have mono channel");
		return -EINVAL;
	}

	/*Mute Earpiece*/
	MSG0("Mute Earpiece");
	error = HW_ACODEC_MODIFY_WRITE(MUTE_HS_EAR_REG, EN_EAR_MASK, 0);
	if (error != 0) {
		printk("ERROR: Mute Earpiece %d", error);
		return error;
	}
	dump_acodec_registers(__FUNCTION__);
	return error;
}
/**
 * \brief Unmute earpiece
 * \param channel_index Channel-index
 * \param gain Gain index of earpiece
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_unmute_earpiece(AUDIOIO_CH_INDEX channel_index, int *gain)
{
	int error = 0;

	/*Check if Earpiece UnMute request is mono channel*/
	if (!(channel_index & e_CHANNEL_1)) {
		printk("Earpiece should have mono channel");
		return -EINVAL;
	}

	/*UnMute Earpiece*/
	MSG0("UnMute Earpiece");
	error = HW_ACODEC_MODIFY_WRITE(MUTE_HS_EAR_REG, 0, EN_EAR_MASK);
	if (error != 0) {
		printk("ERROR: UnMute Earpiece %d", error);
		return error;
	}
	dump_acodec_registers(__FUNCTION__);
	return error;
}
/**
 * \brief Enables fading of earpiece
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_enable_fade_earpiece(void)
{
	int error = 0;

	MSG0("Enable fading for Ear");
	error = HW_ACODEC_MODIFY_WRITE(DA1_DIGITAL_GAIN_REG, 0, DIS_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for Ear %d", error);
		return error;
	}

	error = HW_ACODEC_MODIFY_WRITE(HSL_EAR_DIGITAL_GAIN_REG, 0,
							DIS_DIG_GAIN_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for Digital Gain of Ear %d",error);
		return error;
	}

	return error;
}
/**
 * \brief Disables fading of earpiece
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_disable_fade_earpiece(void)
{
	int error = 0;
	MSG0("Enable fading for Ear");
	error = HW_ACODEC_MODIFY_WRITE(DA1_DIGITAL_GAIN_REG, DIS_FADING, 0);
	if (error != 0) {
		printk("ERROR: Disable fading for Ear %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Power up IHF on a specific channel
 * \param channel_index Channel-index
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_power_up_ihf(AUDIOIO_CH_INDEX channel_index)
{
	int error=0;
	unsigned char initialVal_DA=0;

	/*Check if IHF PowerUp request is mono or Stereo channel*/
	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2 ))) {
		printk("IHF should have mono or stereo channels");
		return -EINVAL;
	}

	if (channel_index & e_CHANNEL_1) {
		initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
		if (EN_DA3 & initialVal_DA)
			return 0;

		/*Enable DA3 for IHFL*/
		MSG0("Data sent to DA3 from Slot 10");
		error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA3_REG,
							SLOT10_FOR_DA_PATH, 0);
		if (error != 0) {
			printk("ERROR: Data sent to DA3 from Slot 10 %d",error);
			return error;
		}

		MSG0("Power Up IHFL mono channel");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,
								EN_DA3,0);
		if (error != 0) {
			printk("ERROR: Power up IHFL %d", error);
			return error;
		}

		/*Power Up HFL Class-D Driver*/
		MSG0("Power Up HFL Class-D Driver");
		error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG,
								EN_HFL_MASK,0);
		if (error != 0) {
			printk("ERROR: Power Up HFL Class-D Driver %d", error);
			return error;
		}

		/*Power up HFL Class D driver and digital path*/
		MSG0("Power up HFL Class D driver and digital path");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG,
								EN_HFL_MASK,0);
		if (error != 0) {
			printk("ERROR:Power up HFL Class D driver & digital"
							" path %d",error);
			return error;
		}
	}

	/*Enable DA4 for IHFR*/
	if (channel_index & e_CHANNEL_2) {
		initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
		if (EN_DA4 & initialVal_DA)
			return 0;

		MSG0("Data sent to DA4 from Slot 11");
		error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA4_REG,
							SLOT11_FOR_DA_PATH, 0);
		if (error != 0) {
			printk("ERROR: Data sent to DA4 from Slot 11 %d",error);
			return error;
		}

		MSG0("Enable DA4");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,
								EN_DA4,0);
		if (error != 0) {
			printk("ERROR: Enable DA4 %d", error);
			return error;
		}

		/*Power Up HFR Class-D Driver*/
		MSG0("Power Up HFR Class-D Driver");
		error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG,
								EN_HFR_MASK,0);
		if (error != 0) {
			printk("ERROR: Power Up HFR Class-D Driver %d", error);
			return error;
		}

		/*Power up HFR Class D driver and digital path*/
		MSG0("Power up HFR Class D driver and digital path");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG,
								EN_HFR_MASK,0);
		if (error != 0) {
			printk("ERROR: Power up HFR Class D driver and "
						"digital path %d", error);
			return error;
		}
	}
	dump_acodec_registers(__FUNCTION__);
	return error;
}
/**
 * \brief Power down IHF on a specific channel
 * \param channel_index Channel-index
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_power_down_ihf(AUDIOIO_CH_INDEX channel_index)
{
	int error=0;
	unsigned char initialVal_DA=0;

		/*Check if IHF Power Down request is mono or Stereo channel*/
	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2 ))) {
		printk("IHF should have mono or stereo channels");
		return -EINVAL;
	}

	if (channel_index & e_CHANNEL_1) {
		initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
		if (!(initialVal_DA & EN_DA3))
			return 0;

		/*Power Down HFL Class-D Driver*/
		MSG0("Power Down HFL Class-D Driver");
		error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG,0,
								EN_HFL_MASK);
		if (error != 0) {
			printk("ERROR: Power Down HFL Class-D Driver %d",error);
			return error;
		}

		/*Power Down HFL Class D driver and digital path*/
		MSG0("Power Down HFL Class D driver and digital path");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG,0,
								EN_HFL_MASK);
		if (error != 0) {
			printk("ERROR:Power Dwn HFL Class D driver & digital"
							" path%d",error);
			return error;
		}

		/*Disable DA3 for IHFL*/
		MSG0("Disable DA3");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,
								0,EN_DA3);
		if (error != 0) {
			printk("ERROR: Disable DA3 %d", error);
			return error;
		}

		MSG0("Data sent to DA3 cleared from Slot 10");
		error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA3_REG, 0,
							SLOT10_FOR_DA_PATH);
		if (error != 0) {
			printk("ERROR: Data sent to DA3 cleared from Slot 10%d",
									error);
			return error;
		}
	}

	if (channel_index & e_CHANNEL_2) {
		initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);

		/*Check if IHF is already powered Down*/
		if (!(initialVal_DA & EN_DA4))
			return 0;

		/*Power Down HFR Class-D Driver*/
		MSG0("Power Down HFR Class-D Driver");
		error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG, 0,
								EN_HFR_MASK);
		if (error != 0) {
			printk("ERROR: Power Down HFR Class-D Driver %d",error);
			return error;
		}

		/*Power Down HFR Class D driver and digital path*/
		MSG0("Power Down HFR Class D driver and digital path");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG,0,
								EN_HFR_MASK);
		if (error != 0) {
			printk("ERROR:Power Dwn HFR Class D driver & digital"
							" path%d",error);
			return error;
		}

			/*Disable DA4 for IHFR*/
		MSG0("Disable DA4");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,
								0,EN_DA4);
		if (error != 0) {
			printk("ERROR: Disable DA4 %d", error);
			return error;
		}

		MSG0("Data sent to DA4 cleared from Slot 11");
		error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA4_REG, 0,
							SLOT11_FOR_DA_PATH);
		if (error != 0) {
			printk("ERROR: Data sent to DA4 cleared from Slot 11%d",
									error);
			return error;
		}
	}
	dump_acodec_registers(__FUNCTION__);
	return error;
}
/**
 * \brief Mute IHF on a specific channel
 * \param channel_index Channel-index
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_mute_ihf(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;

	if ((channel_index & (e_CHANNEL_1 | e_CHANNEL_2 ))) {
		error = ste_audio_io_set_ihf_gain(channel_index,0,-63, 0);
		if (error != 0) {
			printk("ERROR: Mute ihf %d", error);
			return error;
		}
	}
	dump_acodec_registers(__FUNCTION__);
	return error;
}
/**
 * \brief Unmute IHF on a specific channel
 * \param channel_index Channel-index
 * \param gain Gain index of IHF
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_unmute_ihf(AUDIOIO_CH_INDEX channel_index, int *gain)
{
	int error = 0;

	if (channel_index & e_CHANNEL_1) {
		error = ste_audio_io_set_ihf_gain(channel_index,0,gain[0],0);
		if (error != 0) {
				printk("ERROR: UnMute ihf %d", error);
				return error;
			}
		}

	if (channel_index & e_CHANNEL_2) {
		error = ste_audio_io_set_ihf_gain(channel_index,0,gain[1],0);
		if (error != 0) {
			printk("ERROR: UnMute ihf %d", error);
			return error;
		}
	}
	dump_acodec_registers(__FUNCTION__);
	return error;
}
/**
 * \brief Enable fading of IHF
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_enable_fade_ihf(void)
{
	int error = 0;

	MSG0("Enable fading for HFL");
	error = HW_ACODEC_MODIFY_WRITE(DA3_DIGITAL_GAIN_REG, 0, DIS_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for HFL %d", error);
		return error;
	}

	MSG0("Enable fading for HFR");
	error = HW_ACODEC_MODIFY_WRITE(DA4_DIGITAL_GAIN_REG, 0, DIS_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for HFR %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Disable fading of IHF
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_disable_fade_ihf(void)
{
	int error = 0;

	MSG0("Disable fading for HFL");
	error = HW_ACODEC_MODIFY_WRITE(DA3_DIGITAL_GAIN_REG, DIS_FADING, 0);
	if (error != 0) {
		printk("ERROR: Disable fading for HFL %d", error);
		return error;
	}

	MSG0("Disable fading for HFR");
	error = HW_ACODEC_MODIFY_WRITE(DA4_DIGITAL_GAIN_REG, DIS_FADING, 0);
	if (error != 0) {
		printk("ERROR: Disable fading for HFR %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Power up VIBL
 * \param channel_index Channel-index of VIBL
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_power_up_vibl(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	unsigned char initialVal_DA=0;

	/*Check if VibL PowerUp request is mono channel*/
	if (!(channel_index & e_CHANNEL_1)) {
		printk("VibL should have mono channel");
		return -EINVAL;
	}

	initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);

	/*Check if VibL is already powered up*/
	if (initialVal_DA & EN_DA5)
		return 0;

	/*Enable DA5 for VibL*/
	MSG0("Data sent to DA5 from Slot 12");
	error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA5_REG,
							SLOT12_FOR_DA_PATH, 0);
	if (error != 0) {
		printk("ERROR: Data sent to DA5 from Slot 12 %d", error);
		return error;
	}

	MSG0("Enable DA5 for VibL");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,EN_DA5,0);
	if (error != 0) {
		printk("ERROR: Enable DA5 for VibL %d", error);
		return error;
	}

		/*Power Up VibL Class-D Driver*/
	MSG0("Power Up VibL Class-D Driver");
	error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG,EN_VIBL_MASK,0);
	if (error != 0) {
		printk("ERROR: Power Up VibL Class-D Driver %d", error);
		return error;
	}

	/*Power up VibL Class D driver and digital path*/
	MSG0("Power up VibL Class D driver and digital path");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG, EN_VIBL_MASK,
									0);
	if (error != 0) {
		printk("ERROR:Power up VibL Class D driver and digital path %d",
									error);
		return error;
	}
	return error;
}
/**
 * \brief Power down VIBL
 * \param channel_index Channel-index of VIBL
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_power_down_vibl(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	unsigned char initialVal_DA=0;

	/*Check if VibL Power Down request is mono channel*/
	if (!(channel_index & e_CHANNEL_1)) {
		printk("VibL should have mono channel");
		return -EINVAL;
	}

	initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);

	/*Check if VibL is already powered down*/
	if (!(initialVal_DA & EN_DA5))
		return 0;


	/*Power Down VibL Class-D Driver*/
	MSG0("Power Down VibL Class-D Driver");
	error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG,0,EN_VIBL_MASK);
	if (error != 0)	{
		printk("ERROR: Power Down VibL Class-D Driver %d", error);
		return error;
	}

	/*Power Down VibL Class D driver and digital path*/
	MSG0("Power Down VibL Class D driver and digital path");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG, 0,
								EN_VIBL_MASK);
	if (error != 0) {
		printk("ERROR:Power Down VibL Class D driver & digital path %d",
									error);
		return error;
	}

	/*Disable DA5 for VibL*/
	MSG0("Disable DA5 for VibL");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,0,EN_DA5);
	if (error != 0) {
		printk("ERROR: Disable DA5 for VibL %d", error);
		return error;
	}

	MSG0("Data sent to DA5 cleared from Slot 12");
	error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA5_REG, 0,
							SLOT12_FOR_DA_PATH);
	if (error != 0) {
		printk("ERROR: Data sent to DA5 cleared from Slot 12 %d",error);
		return error;
	}
	return error;
}
/**
 * \brief Enable fading of VIBL
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_enable_fade_vibl(void)
{
	int error = 0;

	MSG0("Enable fading for VibL");
	error = HW_ACODEC_MODIFY_WRITE(DA5_DIGITAL_GAIN_REG, 0, DIS_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for VibL %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Disable fading of VIBL
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_disable_fade_vibl(void)
{
	int error = 0;

	MSG0("Disable fading for VibL");
	error = HW_ACODEC_MODIFY_WRITE(DA5_DIGITAL_GAIN_REG, DIS_FADING, 0);
	if (error != 0) {
		printk("ERROR: Disable fading for VibL %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Power up VIBR
 * \param channel_index Channel-index of VIBR
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_power_up_vibr(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	unsigned char initialVal_DA=0;

	/*Check if VibR PowerUp request is mono channel*/
	if (!(channel_index & e_CHANNEL_1)) {
		printk("VibR should have mono channel");
		return -EINVAL;
	}

	initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);

	/*Check if VibR is already powered up*/
	if (initialVal_DA & EN_DA6)
		return 0;

	/*Enable DA6 for VibR*/
	MSG0("Data sent to DA6 from Slot 13");
	error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA6_REG,
							SLOT13_FOR_DA_PATH, 0);
	if (error != 0) {
		printk("ERROR: Data sent to DA5 from Slot 13 %d", error);
		return error;
	}

	MSG0("Enable DA6 for VibR");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,EN_DA6,0);
	if (error != 0) {
		printk("ERROR: Enable DA6 for VibR %d", error);
		return error;
	}

	/*Power Up VibR Class-D Driver*/
	MSG0("Power Up VibR Class-D Driver");
	error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG,EN_VIBR_MASK,0);
	if (error != 0) {
		printk("ERROR: Power Up VibR Class-D Driver %d", error);
		return error;
	}

		/*Power up VibR Class D driver and digital path*/
	MSG0("Power up VibR Class D driver and digital path");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG,
								EN_VIBR_MASK,0);
	if (error != 0) {
		printk("ERROR: Power up VibR Class D driver & digital path %d",
									error);
		return error;
	}
	return error;
}
/**
 * \brief Power down VIBR
 * \param channel_index Channel-index of VIBR
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_power_down_vibr(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	unsigned char initialVal_DA=0;

	/*Check if VibR PowerDown request is mono channel*/
	if (!(channel_index & e_CHANNEL_1)) {
		printk("VibR should have mono channel");
		return -EINVAL;
	}

	initialVal_DA = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);

	/*Check if VibR is already powered down*/
	if (!(initialVal_DA & EN_DA6))
		return 0;


	/*Power Down VibR Class-D Driver*/
	MSG0("Power Down VibR Class-D Driver");
	error = HW_ACODEC_MODIFY_WRITE(ANALOG_OUTPUT_ENABLE_REG,0,EN_VIBR_MASK);
	if (error != 0)	{
		printk("ERROR: Power Down VibR Class-D Driver %d", error);
		return error;
	}

	/*Power Down VibR Class D driver and digital path*/
	MSG0("Power Down VibR Class D driver and digital path");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_OUTPUT_ENABLE_REG, 0,
								EN_VIBR_MASK);
	if (error != 0) {
		printk("ERROR:Power Down VibR Class D driver & digital path %d",
									error);
		return error;
	}

	/*Disable DA6 for VibR*/
	MSG0("Disable DA6 for VibR");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_DA_CHANNELS_ENABLE_REG,0,EN_DA6);
	if (error != 0) {
		printk("ERROR: Disable DA6 for VibR %d", error);
		return error;
	}

	MSG0("Data sent to DA6 cleared from Slot 13");
	error = HW_ACODEC_MODIFY_WRITE(SLOT_SELECTION_TO_DA6_REG, 0,
							SLOT13_FOR_DA_PATH);
	if (error != 0) {
		printk("ERROR: Data sent to DA5 cleared from Slot 13 %d",error);
		return error;
	}
	return error;
}
/**
 * \brief Enable fading of VIBR
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_enable_fade_vibr(void)
{
	int error = 0;

	MSG0("Enable fading for VibR");
	error = HW_ACODEC_MODIFY_WRITE(DA6_DIGITAL_GAIN_REG, 0, DIS_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for VibR %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Disable fading of VIBR
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_disable_fade_vibr(void)
{
	int error = 0;

	MSG0("Disable fading for VibR");
	error = HW_ACODEC_MODIFY_WRITE(DA6_DIGITAL_GAIN_REG, DIS_FADING, 0);
	if (error != 0) {
		printk("ERROR: Disable fading for VibR %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Power up MIC1A
 * \param channel_index Channel-index of MIC1A
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_power_up_mic1a(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	unsigned char initialVal_AD=0;

	/*Check if Mic1 PowerUp request is mono channel*/

	if (!(channel_index & e_CHANNEL_1)) {
		printk("Mic1 should have mono channel");
		return -EINVAL;
	}

	initialVal_AD = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
	/*Check if Mic1 is already powered up or used by DMIC3*/
	if (EN_AD3 & initialVal_AD)
		return 0;

	MSG0("Slot 02 outputs data from AD_OUT3");

	error = HW_REG_WRITE(AD_ALLOCATION_TO_SLOT0_1_REG, DATA_FROM_AD_OUT3);
	if (error != 0)	{
		printk("ERROR: Slot 02 outputs data from AD_OUT3 %d", error);
		return error;
	}

	MSG0("Enable AD3");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,EN_AD3,0);
	if (error != 0) {
		printk("ERROR: Enable AD3 for Mic1 %d", error);
		return error;
	}

	MSG0("Select ADC1 for AD_OUT3");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1, 0,
							SEL_DMIC3_FOR_AD_OUT3);
	if (error != 0) {
		printk("ERROR: Select ADC1 for AD_OUT3 %d", error);
		return error;
	}

	/*Select MIC1A*/
	MSG0("Select MIC1A");
	error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG, 0,
							SEL_MIC1B_CLR_MIC1A);
	if (error != 0) {
		printk("ERROR: Select MIC1A %d", error);
		return error;
	}

	MSG0("Power Up Mic1");
	error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG, EN_MIC1, 0);
	if (error != 0) {
		printk("ERROR: Power up Mic1 %d", error);
		return error;
	}

	//Power Up ADC1
	MSG0("Power Up ADC1");
	error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG, POWER_UP_ADC1, 0);
	if (error != 0) {
		printk("ERROR: Power Up ADC1 %d", error);
		return error;
	}

return error;
}
/**
 * \brief Power down MIC1A
 * \param channel_index Channel-index of MIC1A
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_power_down_mic1a(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	unsigned char initialVal_AD=0;

	/*Check if Mic1 PowerDown request is mono channel*/

	if (!(channel_index & e_CHANNEL_1)) {
		printk("Mic1 should have mono channel");
		return -EINVAL;
	}

	initialVal_AD = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
	/*Check if Mic1 is already powered down or used by DMIC3*/
	if (!(initialVal_AD & EN_AD3))
		return 0;

	MSG0("Power Down Mic1");
	error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG, 0, EN_MIC1);
	if (error != 0) {
		printk("ERROR: Power Down Mic1 %d", error);
		return error;
	}

		/*Power Down ADC1*/
	MSG0("Power Down ADC1");
	error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG, 0, POWER_UP_ADC1);
	if (error != 0) {
		printk("ERROR: Power Down ADC1 %d", error);
		return error;
	}

	MSG0("Disable AD3");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,0,EN_AD3);
	if (error != 0) {
		printk("ERROR: Disable AD3 for Mic1 %d", error);
		return error;
	}

	MSG0("Slot 02 outputs data cleared from AD_OUT3");
	error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT2_3_REG, 0,
							DATA_FROM_AD_OUT3);
	if (error != 0) {
		printk("ERROR: Slot 02 outputs data cleared from AD_OUT3 %d",
									error);
		return error;
	}
	return error;
}
/**
 * \brief Mute MIC1A
 * \param channel_index Channel-index of MIC1A
 * \return 0 on success otherwise negative error code
 */


int ste_audio_io_mute_mic1a(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	if (!(channel_index & e_CHANNEL_1)) {
		printk("Mic1 should have mono channel");
		return -EINVAL;
	}

	/*Mute Mic1*/
	MSG0("Mute Mic1");
	error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG, MUT_MIC1, 0);
	if (error != 0) {
		printk("ERROR: Mute Mic1 %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Unmute MIC1A
 * \param channel_index Channel-index of MIC1A
 * \param gain Gain index of MIC1A
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_unmute_mic1a(AUDIOIO_CH_INDEX channel_index, int *gain)
{
	int error = 0;
	if (!(channel_index & e_CHANNEL_1)) {
		printk("Mic1 should have mono channel");
		return -EINVAL;
	}
	/*UnMute Mic1*/
	MSG0("UnMute Mic1");
	error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG, 0, MUT_MIC1);
	if (error != 0) {
		printk("ERROR: UnMute Mic1 %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Enable fading of MIC1A
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_enable_fade_mic1a(void)
{
	int error = 0;

	MSG0("Enable fading for Mic1");
	error = HW_ACODEC_MODIFY_WRITE(AD3_DIGITAL_GAIN_REG, 0, DIS_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for Mic1 %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Disable fading of MIC1A
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_disable_fade_mic1a(void)
{
	int error = 0;

	MSG0("Disable fading for Mic1");
	error = HW_ACODEC_MODIFY_WRITE(AD3_DIGITAL_GAIN_REG, DIS_FADING, 0);
	if (error != 0) {
		printk("ERROR: Disable fading for Mic1 %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Power up MIC1B
 * \param channel_index Channel-index of MIC1B
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_power_up_mic1b(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	unsigned char initialVal_AD=0;

	if (!(channel_index & e_CHANNEL_1)) {
		printk("Mic1 should have mono channel");
		return -EINVAL;
	}

	initialVal_AD = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
	/*Check if Mic1 is already powered up or used by DMIC3*/
	if (EN_AD3 & initialVal_AD)
		return 0;

	MSG0("Slot 02 outputs data from AD_OUT3");
	error = HW_REG_WRITE(AD_ALLOCATION_TO_SLOT0_1_REG, DATA_FROM_AD_OUT3);
	if (error != 0) {
		printk("ERROR: Slot 02 outputs data from AD_OUT3 %d", error);
		return error;
	}

	MSG0("Enable AD3");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,EN_AD3,0);
	if (error != 0) {
		printk("ERROR: Enable AD3 for Mic1 %d", error);
		return error;
	}

	MSG0("Select ADC1 for AD_OUT3");

	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1,0,
							SEL_DMIC3_FOR_AD_OUT3);
	if (error != 0) {
		printk("ERROR: Select ADC1 for AD_OUT3 %d", error);
		return error;
	}

	/*Select MIC1B*/
	MSG0("Select MIC1B");
	error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG, SEL_MIC1B_CLR_MIC1A,
									0);
	if (error != 0) {
		printk("ERROR: Select MIC1B %d", error);
		return error;
	}

	MSG0("Power Up Mic1");
	error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG, EN_MIC1, 0);
	if (error != 0) {
		printk("ERROR: Power up Mic1 %d", error);
		return error;
	}

	/*Power Up ADC1*/
	MSG0("Power Up ADC1");
	error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG, POWER_UP_ADC1, 0);
	if (error != 0) {
		printk("ERROR: Power Up ADC1 %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Power down MIC1B
 * \param channel_index Channel-index of MIC1B
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_power_down_mic1b(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	unsigned char initialVal_AD=0;

	/*Check if Mic1 PowerDown request is mono channel*/

	if (!(channel_index & e_CHANNEL_1)) {
		printk("Mic1 should have mono channel");
		return -EINVAL;
	}

	initialVal_AD = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
	/*Check if Mic1 is already powered down or used by DMIC3*/
	if (!(initialVal_AD & EN_AD3))
		return 0;


	MSG0("Power Down Mic1");
	error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG, 0, EN_MIC1);
	if (error != 0) {
		printk("ERROR: Power Down Mic1 %d", error);
		return error;
	}

	/*Power Down ADC1*/
	MSG0("Power Down ADC1");
	error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG, 0, POWER_UP_ADC1);
	if (error != 0) {
		printk("ERROR: Power Down ADC1 %d", error);
		return error;
	}
	MSG0("Disable AD3");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,0,EN_AD3);
	if (error != 0) {
		printk("ERROR: Disable AD3 for Mic1 %d", error);
		return error;
	}

	MSG0("Slot 02 outputs data cleared from AD_OUT3");
	error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT2_3_REG, 0,
							DATA_FROM_AD_OUT3);
	if (error != 0) {
		printk("ERROR: Slot 02 outputs data cleared from AD_OUT3 %d",
									error);
		return error;
	}

	return error;
}
/**
 * \brief Power up MIC2
 * \param channel_index Channel-index of MIC2
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_power_up_mic2(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	unsigned char initialVal_AD=0;

	/*Check if Mic2 PowerUp request is mono channel*/
	if (!(channel_index & e_CHANNEL_1)) {
		printk("Mic2 should have mono channel");
		return -EINVAL;
	}

	initialVal_AD = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);

	/*Check if Mic2 is already powered up or used by LINR or DMIC2*/
	if (EN_AD2 & initialVal_AD)
		return 0;

	MSG0("Slot 01 outputs data from AD_OUT2");

	error = HW_REG_WRITE(AD_ALLOCATION_TO_SLOT0_1_REG,DATA_FROM_AD_OUT2);
	if (error != 0) {
		printk("ERROR: Slot 01 outputs data from AD_OUT2 %d", error);
		return error;
	}

	MSG0("Enable AD2");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG, EN_AD2,
									0);
	if (error != 0) {
		printk("ERROR: Enable AD2 for Mic2 %d", error);
		return error;
	}

	MSG0("Select ADC2 for AD_OUT2");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1, 0,
							SEL_DMIC2_FOR_AD_OUT2);
	if (error != 0) {
		printk("ERROR: Select ADC2 for AD_OUT2 %d", error);
		return error;
	}

		/*Select MIC2*/
	MSG0("Select MIC2");
	error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG, 0,
							SEL_LINR_CLR_MIC2);
	if (error != 0) {
		printk("ERROR: Select MIC2 %d", error);
		return error;
	}

	MSG0("Power Up Mic2");
	error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG, EN_MIC2, 0);
	if (error != 0) {
		printk("ERROR: Power up Mic2 %d", error);
		return error;
	}

			//Dont know whether to include or not
			//Power Up ADC1
	MSG0("Power Up ADC2");
	error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG, POWER_UP_ADC2,0);
	if (error != 0) {
		printk("ERROR: Power Up ADC2 %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Power down MIC2
 * \param channel_index Channel-index of MIC2
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_power_down_mic2(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	unsigned char initialVal_AD=0;

	/*Check if Mic2 PowerDown request is mono channel*/
	if (!(channel_index & e_CHANNEL_1)) {
		printk("Mic2 should have mono channel");
		return -EINVAL;
	}

	initialVal_AD = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);

	/*Check if Mic2 is already powered down or used by LINR or DMIC2*/
	if (!(initialVal_AD & EN_AD2))
		return 0;

	MSG0("Power Down Mic2");
	error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG, 0, EN_MIC2);
	if (error != 0) {
		printk("ERROR: Power Down Mic2 %d", error);
		return error;
	}

	MSG0("Disable AD2");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,0,EN_AD2);
	if (error != 0) {
		printk("ERROR: Disable AD2 for Mic2 %d", error);
		return error;
	}

	MSG0("Slot 01 outputs data cleared from AD_OUT2");
	error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT0_1_REG, 0,
							(DATA_FROM_AD_OUT2<<4));
	if (error != 0) {
		printk("ERROR: Slot 01 outputs data cleared from AD_OUT2 %d",
									error);
		return error;
	}
	return error;
}
/**
 * \brief Mute MIC2
 * \param channel_index Channel-index of MIC2
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_mute_mic2(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	if (!(channel_index & e_CHANNEL_1)) {
		printk("Mic2 should have mono channel");
		return -EINVAL;
	}

	/*Mute Mic2*/
	MSG0("Mute Mic2");
	error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG, MUT_MIC2, 0);
	if (error != 0) {
		printk("ERROR: Mute Mic2 %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Unmute MIC2
 * \param channel_index Channel-index of MIC2
 * \param gain Gain index of MIC2
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_unmute_mic2(AUDIOIO_CH_INDEX channel_index, int *gain)
{
	int error = 0;
	if (!(channel_index & e_CHANNEL_1)) {
		printk("Mic2 should have mono channel");
		return -EINVAL;
	}
/*UnMute Mic2*/
	MSG0("UnMute Mic2");
	error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG, 0, MUT_MIC2);
	if (error != 0) {
		printk("ERROR: UnMute Mic2 %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Enable fading of MIC2
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_enable_fade_mic2(void)
{
	int error = 0;

	MSG0("Enable fading for Mic2");
	error = HW_ACODEC_MODIFY_WRITE(AD2_DIGITAL_GAIN_REG, 0, DIS_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for Mic2 %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Disable fading of MIC2
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_disable_fade_mic2(void)
{
	int error = 0;

	MSG0("Disable fading for Mic2");
	error = HW_ACODEC_MODIFY_WRITE(AD2_DIGITAL_GAIN_REG, DIS_FADING, 0);
	if (error != 0) {
		printk("ERROR: Disable fading for Mic2 %d", error);
		return error;
	}

	return error;
}
/**
 * \brief Power up LinIn
 * \param channel_index Channel-index of LinIn
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_power_up_lin(AUDIOIO_CH_INDEX channel_index)
{
	int error=0;
	unsigned char initialVal_AD=0;

	/*Check if LinIn PowerUp request is mono or Stereo channel*/
	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2 ))) {
		printk("LinIn should have mono or stereo channels");
		return -EINVAL;
	}

	/*Enable AD1 for LinInL*/
	if (channel_index & e_CHANNEL_1) {
		initialVal_AD = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
		if (initialVal_AD & EN_AD1)
			return 0;

		MSG0("Slot 00 outputs data from AD_OUT1");
		error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT0_1_REG,
							DATA_FROM_AD_OUT1, 0);
		if (error != 0) {
			printk("ERROR: Slot 00 outputs data from AD_OUT1 %d",
									error);
			return error;
		}

		MSG0("Enable AD1 for LinInL");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,
								EN_AD1, 0);
		if (error != 0) {
			printk("ERROR: Enable AD1 for LinInL %d", error);
			return error;
		}

		MSG0("Select ADC3 for AD_OUT1");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1, 0,
							SEL_DMIC1_FOR_AD_OUT1);
		if (error != 0) {
			printk("ERROR: Select ADC3 for AD_OUT1 %d", error);
			return error;
		}

		MSG0("Power Up LinInL");
		error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG,EN_LIN_IN_L,
									0);
		if (error != 0) {
			printk("ERROR: Power up LinInL %d", error);
			return error;
		}

		/*Power Up ADC3*/
		MSG0("Power Up ADC3");
		error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG,POWER_UP_ADC3,
									0);
		if (error != 0) {
			printk("ERROR: Power Up ADC3 %d", error);
			return error;
		}
	}
	/*Enable AD2 for LinInR*/

	if (channel_index & e_CHANNEL_2) {
		initialVal_AD = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
		if (EN_AD2 & initialVal_AD)
			return 0;

		MSG0("Slot 01 outputs data from AD_OUT2");
		error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT0_1_REG,
						(DATA_FROM_AD_OUT2<<4), 0);
		if (error != 0) {
			printk("ERROR: Slot 01 outputs data from AD_OUT2 %d",
									error);
			return error;
		}

		MSG0("Power Up LinInR mono channel");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,
								EN_AD2,0);
		if (error != 0) {
			printk("ERROR: Enable AD2 LinInR %d", error);
			return error;
		}

		MSG0("Select ADC2 for AD_OUT2");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1, 0,
							SEL_DMIC2_FOR_AD_OUT2);
		if (error != 0) {
			printk("ERROR: Select ADC2 for AD_OUT2 %d", error);
			return error;
		}

		/*Select LinInR*/
		MSG0("Select LinInR");
		error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG,
							SEL_LINR_CLR_MIC2,0);
		if (error != 0) {
			printk("ERROR: Select LinInR %d", error);
			return error;
		}

		MSG0("Power Up LinInR");
		error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG,EN_LIN_IN_R,
									0);
		if (error != 0) {
			printk("ERROR: Power up LinInR %d", error);
			return error;
		}

		/*Power Up ADC2*/
		MSG0("Power Up ADC2");
		error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG,POWER_UP_ADC2,
									0);
		if (error != 0) {
			printk("ERROR: Power Up ADC2 %d", error);
			return error;
		}
	}
	return error;
}
/**
 * \brief Power down LinIn
 * \param channel_index Channel-index of LinIn
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_power_down_lin(AUDIOIO_CH_INDEX channel_index)
{
	int error=0;
	unsigned char initialVal_AD=0;

		/*Check if LinIn PowerDown request is mono or Stereo channel*/
	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2 ))) {
		printk("LinIn should have mono or stereo channels");
		return -EINVAL;
	}

	/*Enable AD1 for LinInL*/
	if (channel_index & e_CHANNEL_1) {
		initialVal_AD = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
		if (!(initialVal_AD & EN_AD1))
			return 0;

		MSG0("Power Down LinInL");
		error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG, 0,
								EN_LIN_IN_L);
		if (error != 0) {
			printk("ERROR: Power Down LinInL %d", error);
			return error;
		}

			/*Power Down ADC3*/
		MSG0("Power Down ADC3");
		error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG, 0,
								POWER_UP_ADC3);
		if (error != 0) {
			printk("ERROR: Power Down ADC3 %d", error);
			return error;
		}

		MSG0("Disable AD1 for LinInL");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,0,
									EN_AD1);
		if (error != 0) {
			printk("ERROR: Disable AD1 for LinInL %d", error);
			return error;
		}

		MSG0("Slot 00 outputs data cleared from AD_OUT1");
		error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT0_1_REG, 0,
							DATA_FROM_AD_OUT1);
		if (error != 0) {
			printk("ERROR:Slot 00 outputs data cleared from"
							" AD_OUT1 %d",error);
			return error;
		}
	}

	/*Enable AD2 for LinInR*/
	if (channel_index & e_CHANNEL_2) {
		initialVal_AD = HW_REG_READ(DIGITAL_DA_CHANNELS_ENABLE_REG);
		if (!(initialVal_AD & EN_AD2))
			return 0;

		MSG0("Power Down LinInR");
		error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG, 0,
								EN_LIN_IN_R);
		if (error != 0) {
			printk("ERROR: Power Down LinInR %d", error);
			return error;
		}

		/*Power Down ADC2*/
		MSG0("Power Down ADC2");
		error = HW_ACODEC_MODIFY_WRITE(ADC_DAC_ENABLE_REG, 0,
								POWER_UP_ADC2);
		if (error != 0) {
			printk("ERROR: Power Down ADC2 %d", error);
			return error;
		}

		MSG0("Disable AD2 LinInR");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,0,
									EN_AD2);
		if (error != 0) {
			printk("ERROR: Disable AD2 LinInR %d", error);
			return error;
		}

		MSG0("Slot 01 outputs data cleared from AD_OUT2");
		error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT0_1_REG, 0,
							(DATA_FROM_AD_OUT2<<4));
		if (error != 0) {
			printk("ERROR: Slot 01 outputs data cleared from "
							"AD_OUT2 %d",error);
			return error;
		}
	}
	return error;
}
/**
 * \brief Mute LinIn
 * \param channel_index Channel-index of LinIn
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_mute_lin(AUDIOIO_CH_INDEX channel_index)
{
	int error=0;

	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2 ))) {
		printk("LinIn should have mono or stereo channels");
		return -EINVAL;
	}

	if (channel_index & e_CHANNEL_1) {
		/*Mute LinInL*/
		MSG0("Mute LinInL");
		error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG,
							MUT_LIN_IN_L, 0);
		if (error != 0) {
			printk("ERROR: Mute LinInL %d", error);
			return error;
		}
	}

	if (channel_index & e_CHANNEL_2) {
		/*Mute LinInR*/
		MSG0("Mute LinInR");
		error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG,
								MUT_LIN_IN_R,0);
		if (error != 0) {
			printk("ERROR: Mute LinInR %d", error);
			return error;
		}
	}
	return error;
}
/**
 * \brief Unmute LinIn
 * \param channel_index Channel-index of LinIn
 * \param gain Gain index of LinIn
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_unmute_lin(AUDIOIO_CH_INDEX channel_index, int *gain)
{
	int error=0;

	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2 ))) {
		printk("LinIn should have mono or stereo channels");
		return -EINVAL;
	}

	if (channel_index & e_CHANNEL_1) {
		/*UnMute LinInL*/
		MSG0("UnMute LinInL");
		error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG,0,
								MUT_LIN_IN_L);
		if (error != 0) {
			printk("ERROR: UnMute LinInL %d", error);
			return error;
		}
	}

	if (channel_index & e_CHANNEL_2) {
		/*UnMute LinInR*/
		MSG0("UnMute LinInR");
		error = HW_ACODEC_MODIFY_WRITE(LINE_IN_MIC_CONF_REG,0,
								MUT_LIN_IN_R);
		if (error != 0) {
			printk("ERROR: UnMute LinInR %d", error);
			return error;
		}
	}
	return error;
}
/**
 * \brief Enables fading of LinIn
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_enable_fade_lin(void)
{
	int error = 0;

	MSG0("Enable fading for LinInL");
	error = HW_ACODEC_MODIFY_WRITE(AD1_DIGITAL_GAIN_REG, 0, DIS_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for LinInL %d", error);
		return error;
	}

	MSG0("Enable fading for LinInR");
	error = HW_ACODEC_MODIFY_WRITE(AD2_DIGITAL_GAIN_REG, 0, DIS_FADING);
	if (error != 0) {
		printk("ERROR: Enable fading for LinInR %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Disables fading of LinIn
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_disable_fade_lin(void)
{
	int error = 0;

	MSG0("Disable fading for LinInL");
	error = HW_ACODEC_MODIFY_WRITE(AD1_DIGITAL_GAIN_REG, DIS_FADING, 0);
	if (error != 0) {
		printk("ERROR: Disable fading for LinInL %d", error);
		return error;
	}

	MSG0("Disable fading for LinInR");
	error = HW_ACODEC_MODIFY_WRITE(AD2_DIGITAL_GAIN_REG, DIS_FADING, 0);
	if (error != 0) {
		printk("ERROR: Disable fading for LinInR %d", error);
		return error;
	}
	return error;
}
/**
 * \brief Power Up DMIC12 LinIn
 * \param channel_index Channel-index of DMIC12
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_power_up_dmic12(AUDIOIO_CH_INDEX channel_index)
{
	int error=0;
	unsigned char initialVal_AD=0;

	/*Check if DMic12 request is mono or Stereo*/
	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2 ))) {
		printk("DMic12 does not support more than 2 channels");
		return -EINVAL;
	}

	/*Setting Direction for GPIO pins on AB8500*/
	error = HW_REG_WRITE(0x1013, 0x04);
	if (error != 0) {
		printk("Setting Direction for GPIO pins on AB8500 %d", error);
		return error;
	}

	/*Enable AD1 for DMIC1*/
	if (channel_index & e_CHANNEL_1) {
		/*Check if DMIC1 is already powered up or used by LinInL*/
		initialVal_AD = HW_REG_READ(DIGITAL_AD_CHANNELS_ENABLE_REG);
		if (initialVal_AD & EN_AD1)
			return 0;

		MSG0("Slot 00 outputs data from AD_OUT1");
		error = HW_REG_WRITE(AD_ALLOCATION_TO_SLOT0_1_REG,
							DATA_FROM_AD_OUT1);
		if (error != 0) {
			printk("ERROR: Slot 00 outputs data from AD_OUT1 %d",
									error);
			return error;
		}

		MSG0("Enable AD1 for DMIC1 channel");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,
								EN_AD1,0);
		if (error != 0) {
			printk("ERROR: Enable AD1 for DMIC1 %d", error);
			return error;
		}

		MSG0("Select DMIC1 for AD_OUT1");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1,
						SEL_DMIC1_FOR_AD_OUT1, 0);
		if (error != 0) {
			printk("ERROR: Select DMIC1 for AD_OUT1 %d", error);
			return error;
		}

		MSG0("Enable DMIC1");
		error = HW_ACODEC_MODIFY_WRITE(DMIC_ENABLE_REG, EN_DMIC1, 0);
		if (error != 0) {
			printk("ERROR: Enable DMIC1 %d", error);
			return error;
		}
	}
	/*Enable AD2 for DMIC2*/

	if (channel_index & e_CHANNEL_2) {
	/*Check if DMIC2 is already powered up or used by Mic2 or LinInR*/
		initialVal_AD = HW_REG_READ(DIGITAL_AD_CHANNELS_ENABLE_REG);
		if (initialVal_AD & EN_AD2)
			return 0;

		MSG0("Slot 01 outputs data from AD_OUT2");
		error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT0_1_REG,
						(DATA_FROM_AD_OUT2<<4), 0);
		if (error != 0) {
			printk("ERROR: Slot 01 outputs data from AD_OUT2 %d",
									error);
			return error;
		}

		MSG0("Enable AD2 for DMIC2 channel");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,
								EN_AD2,0);
		if (error != 0) {
			printk("ERROR: Enable AD2 for DMIC2 %d", error);
			return error;
		}

		MSG0("Select DMIC2 for AD_OUT2");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1,
						SEL_DMIC2_FOR_AD_OUT2, 0);
		if (error != 0) {
			printk("ERROR: Select DMIC2 for AD_OUT2 %d", error);
			return error;
		}

		MSG0("Enable DMIC2");
		error = HW_ACODEC_MODIFY_WRITE(DMIC_ENABLE_REG, EN_DMIC2, 0);
		if (error != 0) {
			printk("ERROR: Enable DMIC2 %d", error);
			return error;
		}
	}

	return error;
}
/**
 * \brief Power down DMIC12 LinIn
 * \param channel_index Channel-index of DMIC12
 * \return 0 on success otherwise negative error code
 */


int ste_audio_io_power_down_dmic12(AUDIOIO_CH_INDEX channel_index)
{
	int error=0;
	unsigned char initialVal_AD=0;

	/*Check if DMic12 request is mono or Stereo or multi channel*/
	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2))) {
		printk("DMic12 does not support more than 4 channels");
		return -EINVAL;
	}

	/*Setting Direction for GPIO pins on AB8500*/
	error = HW_ACODEC_MODIFY_WRITE(0x1013, 0x0, 0x04);
	if (error != 0) {
		printk("Clearing Direction for GPIO pins on AB8500 %d", error);
		return error;
	}
	/*Enable AD1 for DMIC1*/
	if (channel_index & e_CHANNEL_1) {
		/*Check if DMIC1 is already powered Down or used by LinInL*/
		initialVal_AD = HW_REG_READ(DIGITAL_AD_CHANNELS_ENABLE_REG);
		if (!(initialVal_AD & EN_AD1))
			return 0;

		MSG0("Enable DMIC1");
		error = HW_ACODEC_MODIFY_WRITE(DMIC_ENABLE_REG, 0, EN_DMIC1);
		if (error != 0) {
			printk("ERROR: Enable DMIC1 %d", error);
			return error;
		}

		MSG0("Disable AD1 for DMIC1 channel");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,0,
									EN_AD1);
		if (error != 0) {
			printk("ERROR: Disable AD1 for DMIC1 %d", error);
			return error;
		}

		MSG0("Slot 00 outputs data cleared from AD_OUT1");
		error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT0_1_REG, 0,
							DATA_FROM_AD_OUT1);
		if (error != 0) {
			printk("ERROR: Slot 00 outputs data cleared from"
							" AD_OUT1 %d",error);
			return error;
		}
	}

	/*Enable AD2 for DMIC2*/
	if (channel_index & e_CHANNEL_2) {
	/*Check if DMIC2 is already powered Down or used by Mic2 or LinInR*/
		initialVal_AD = HW_REG_READ(DIGITAL_AD_CHANNELS_ENABLE_REG);
		if (!(initialVal_AD & EN_AD2))
			return 0;

		MSG0("Enable DMIC2");
		error = HW_ACODEC_MODIFY_WRITE(DMIC_ENABLE_REG, 0, EN_DMIC2);
		if (error != 0) {
			printk("ERROR: Enable DMIC2 %d", error);
			return error;
		}

		MSG0("Disable AD2 for DMIC2 channel");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,0,
									EN_AD2);
		if (error != 0) {
			printk("ERROR: Disable AD2 for DMIC2 %d", error);
			return error;
		}

		MSG0("Slot 01 outputs data cleared from AD_OUT2");
		error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT0_1_REG, 0,
							(DATA_FROM_AD_OUT2<<4));
		if (error != 0) {
			printk("ERROR: Slot 01 outputs data cleared from"
							"AD_OUT2 %d",error);
			return error;
		}
	}
	return error;
}
/**
 * \brief  Get headset gain
 * \param left_volume
 * \param right_volume
 * \return 0 on success otherwise negative error code
 */


int ste_audio_io_get_headset_gain(int *left_volume, int *right_volume, uint16 gain_index)
{
	int i = 0;



	if (gain_index == 0) {

		*left_volume = 0 - HW_REG_READ(DA1_DIGITAL_GAIN_REG);

		/*printk("\nleft_volume_0=%d",*left_volume);*/

		*right_volume = 0 - HW_REG_READ(DA2_DIGITAL_GAIN_REG);


		/*printk("\nright_volume_0=%d",*right_volume);*/
	}

	if (gain_index == 1) {
		*left_volume = 8 - HW_REG_READ(HSL_EAR_DIGITAL_GAIN_REG);
		/*printk("\nleft_volume_1=%d",*left_volume);*/
		*right_volume = 8 - HW_REG_READ(HSR_DIGITAL_GAIN_REG);
		/*printk("\nright_volume_1=%d",*right_volume);*/
	}

	if (gain_index == 2) {
		i = (HW_REG_READ(ANALOG_HS_GAIN_REG)>>4);
		*left_volume = hs_analog_gain_table[i];
		/*printk("\nleft_volume_2=%d",*left_volume);*/
		i = (HW_REG_READ(ANALOG_HS_GAIN_REG)&0x0F);
		*right_volume = hs_analog_gain_table[i];
		/*printk("\n right_volume_2=%d",*right_volume);*/
	}
	return 0;
}
/**
 * \brief  Get earpiece gain
 * \param left_volume
 * \param right_volume
 * \return 0 on success otherwise negative error code
 */


int ste_audio_io_get_earpiece_gain(int *left_volume, int *right_volume, uint16 gain_index)
{
	if (0 == gain_index)
        *left_volume =0 - HW_REG_READ(DA1_DIGITAL_GAIN_REG);
	if (1 == gain_index)
	*left_volume =8 - HW_REG_READ(HSL_EAR_DIGITAL_GAIN_REG);
	return 0;
}
/**
 * \brief  Get ihf gain
 * \param left_volume
 * \param right_volume
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_get_ihf_gain(int *left_volume, int *right_volume, uint16 gain_index)
{



	*left_volume = 0-HW_REG_READ(DA3_DIGITAL_GAIN_REG);


	*right_volume = 0-HW_REG_READ(DA4_DIGITAL_GAIN_REG);


	return 0;
}
/**
 * \brief  Get vibl gain
 * \param left_volume
 * \param right_volume
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_get_vibl_gain(int *left_volume, int *right_volume, uint16 gain_index)
{


	*left_volume = 0-HW_REG_READ(DA5_DIGITAL_GAIN_REG);

	return 0;
}
/**
 * \brief  Get vibr gain
 * \param left_volume
 * \param right_volume
 * \return 0 on success otherwise negative error code
 */


int ste_audio_io_get_vibr_gain(int *left_volume, int *right_volume, uint16 gain_index)
{

	*right_volume = 0-HW_REG_READ(DA6_DIGITAL_GAIN_REG);
	return 0;
}
/**
 * \brief  Get MIC1A & MIC2A gain
 * \param left_volume
 * \param right_volume
 * \return 0 on success otherwise negative error code
 */


int ste_audio_io_get_mic1a_gain(int *left_volume,int *right_volume, uint16 gain_index)
{
	if (gain_index == 0)
	*left_volume = 31-HW_REG_READ(AD3_DIGITAL_GAIN_REG);
	if (gain_index == 1)
	*left_volume = HW_REG_READ(ANALOG_MIC1_GAIN_REG);

	return 0;
}
/**
 * \brief  Get MIC2 gain
 * \param left_volume
 * \param right_volume
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_get_mic2_gain(int *left_volume, int *right_volume, uint16 gain_index)
{
	if (gain_index == 0)
	*left_volume =31- HW_REG_READ(AD2_DIGITAL_GAIN_REG);
	if (gain_index == 1)
	*left_volume = HW_REG_READ(ANALOG_MIC2_GAIN_REG);

	return 0;
}
/**
 * \brief  Get Lin IN gain
 * \param left_volume
 * \param right_volume
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_get_lin_gain(int *left_volume, int *right_volume, uint16 gain_index)
{
	if (gain_index == 0) {
		*left_volume = 31-HW_REG_READ(AD1_DIGITAL_GAIN_REG);
		*right_volume = 31-HW_REG_READ(AD2_DIGITAL_GAIN_REG);
	}

	if (gain_index == 0) {
		*left_volume = 2*((HW_REG_READ(ANALOG_HS_GAIN_REG)>>4) -5) ;
		*right_volume = 2*(HW_REG_READ(ANALOG_LINE_IN_GAIN_REG)-5);
	}

	return 0;
}
/**
 * \brief  Get DMIC12 gain
 * \param left_volume
 * \param right_volume
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_get_dmic12_gain(int *left_volume, int *right_volume, uint16 gain_index)
{


	*left_volume = HW_REG_READ(AD1_DIGITAL_GAIN_REG);

	*right_volume = HW_REG_READ(AD2_DIGITAL_GAIN_REG);

	return 0;
}
/**
 * \brief  Get DMIC34 gain
 * \param left_volume
 * \param right_volume
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_get_dmic34_gain(int *left_volume, int *right_volume, uint16 gain_index)
{


	*left_volume = HW_REG_READ(AD3_DIGITAL_GAIN_REG);


	*right_volume = HW_REG_READ(AD4_DIGITAL_GAIN_REG);

	return 0;
}
/**
 * \brief  Get DMIC56 gain
 * \param left_volume
 * \param right_volume
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_get_dmic56_gain(int *left_volume, int *right_volume, uint16 gain_index)
{


	*left_volume = HW_REG_READ(AD5_DIGITAL_GAIN_REG);

	*right_volume = HW_REG_READ(AD6_DIGITAL_GAIN_REG);
	return 0;
}
/**
 * \brief Set gain of headset along a specified channel
 * \param channel_index Channel-index of headset
 * \param gain_index Gain index of headset
 * \param gain_value Gain value of headset
 * \param linear
 * \return 0 on success otherwise negative error code
 */


int ste_audio_io_set_headset_gain(AUDIOIO_CH_INDEX channel_index,
			uint16 gain_index,int32 gain_value, uint32 linear)
{
	int error=0;
	unsigned char initial_val=0;
	int i = 0;

	if (channel_index & e_CHANNEL_1) {
		if (gain_index == 0) {
			int gain=0;
			gain = 0 - gain_value;

			initial_val = HW_REG_READ(DA1_DIGITAL_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(DA1_DIGITAL_GAIN_REG,((initial_val
			& (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain HSL gain index=%d %d",
							gain_index, error);
				return error;
			}
		}

		if (gain_index == 1) {
			int gain=0;
			gain = 8 - gain_value;

			initial_val = HW_REG_READ(HSL_EAR_DIGITAL_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(HSL_EAR_DIGITAL_GAIN_REG,
			((initial_val & (~HS_DIGITAL_GAIN_MASK)) | (gain &
							HS_DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain HSL gain index=%d %d",
							gain_index, error);
				return error;
			}
		}

		if (gain_index == 2) {
			/*Set Analog gain*/
			int gain=-1;

			if (gain_value%2) {
				gain_value -= 1;
				printk("Odd Gain received.Fixing it to 2dB step"
						"gain_value=%ld",gain_value);
			}
			/*Fix for 4dB step gains. Select one lower value*/
			if (gain_value == -22)
				gain_value = -24;

			if (gain_value == -26)
				gain_value = -28;

			if (gain_value == -30)
				gain_value = -32;

			for(i=0;i<16;i++) {
				if (hs_analog_gain_table[i] == gain_value) {
						gain=i<<4;
						break;
					}
			}
			if (gain==-1)
				return -1;

			initial_val = HW_REG_READ(ANALOG_HS_GAIN_REG);

			/*Write gain*/
			error = HW_REG_WRITE(ANALOG_HS_GAIN_REG, ((initial_val &
			(~L_ANALOG_GAIN_MASK)) | (gain & L_ANALOG_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain HSL gain index=%d %d",
							gain_index, error);
				return error;
			}
		}
	}

	/*for HSR*/
	if (channel_index & e_CHANNEL_2) {
		/*Set Gain HSR*/
		MSG0("Set Gain HSR");
		if (gain_index == 0) {
			int gain=0;
			gain = 0 - gain_value;

			initial_val = HW_REG_READ(DA2_DIGITAL_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(DA2_DIGITAL_GAIN_REG, ((initial_val
			& (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain HSR gain index=%d %d",
							gain_index,error);
				return error;
			}
		}

		if (gain_index == 1) {
			int gain=0;
			gain = 8 - gain_value;

			initial_val = HW_REG_READ(HSR_DIGITAL_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(HSR_DIGITAL_GAIN_REG, ((initial_val
			& (~HS_DIGITAL_GAIN_MASK)) | (gain &
							HS_DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain HSR gain index=%d %d",
							gain_index,error);
				return error;
			}

		}

		if (gain_index == 2) {
			/*Set Analog gain*/
			int gain=-1;

			if (gain_value%2) {
				gain_value -= 1;
				printk("Odd Gain received.Fixing it to 2dB step"
						" gain_value=%ld", gain_value);
			}
			/*Fix for 4dB step gains. Select one lower value*/
			if (gain_value == -22)
				gain_value = -24;

			if (gain_value == -26)
				gain_value = -28;

			if (gain_value == -30)
				gain_value = -32;

			for(i=0;i<16;i++) {
				if (hs_analog_gain_table[i] == gain_value){
						gain=i;
						break;
					}
			}
			if (gain==-1)
				return -1;

			initial_val = HW_REG_READ(ANALOG_HS_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(ANALOG_HS_GAIN_REG, ((initial_val &
			(~R_ANALOG_GAIN_MASK)) | (gain & R_ANALOG_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain HSR gain index=%d %d",
							gain_index,error);
				return error;
			}
		}
	}
	dump_acodec_registers(__FUNCTION__);
	return error;
}
/**
 * \brief Set gain of earpiece
 * \param channel_index Channel-index of earpiece
 * \param gain_index Gain index of earpiece
 * \param gain_value Gain value of earpiece
 * \param linear
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_set_earpiece_gain(AUDIOIO_CH_INDEX channel_index,
			uint16 gain_index,int32 gain_value, uint32 linear)
{
	int error=0;
	unsigned char initial_val=0;
	if (channel_index & e_CHANNEL_1) {
		if (0 == gain_index) {
			int gain=0;
			gain = 0 - gain_value;

			initial_val = HW_REG_READ(DA1_DIGITAL_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(DA1_DIGITAL_GAIN_REG, ((initial_val
			& (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK)));
			if (0 != error) {
				printk("ERROR:Set Gain Ear gain index=%d %d",
							gain_index,error);
				return error;
			}
		}

		if (gain_index == 1) {
			int gain=0;
			gain = 8 - gain_value;

			initial_val= HW_REG_READ(HSL_EAR_DIGITAL_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(HSL_EAR_DIGITAL_GAIN_REG,
			((initial_val & (~HS_DIGITAL_GAIN_MASK)) | (gain &
							HS_DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain Ear gain index=%d %d",
							gain_index,error);
				return error;
			}
		}
	}
	dump_acodec_registers(__FUNCTION__);
	return error;
}
/**
 * \brief Set gain of vibl
 * \param channel_index Channel-index of vibl
 * \param gain_index Gain index of vibl
 * \param gain_value Gain value of vibl
 * \param linear
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_set_vibl_gain(AUDIOIO_CH_INDEX channel_index,uint16 gain_index,
						int32 gain_value, uint32 linear)
{

	int error=0;
	unsigned char initial_val=0;

	if (channel_index & e_CHANNEL_1) {
		/*Set Gain VibL*/
		MSG0("Set Gain VibL");
		if (gain_index == 0) {
			int gain=0;
			gain = 0 - gain_value;

			initial_val = HW_REG_READ(DA5_DIGITAL_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(DA5_DIGITAL_GAIN_REG, ((initial_val
			& (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain VibL gain index=%d %d",
							gain_index,error);
				return error;
			}
		}
	}
	return error;
}
/**
 * \brief Set gain of vibr
 * \param channel_index Channel-index of vibr
 * \param gain_index Gain index of vibr
 * \param gain_value Gain value of vibr
 * \param linear
 * \return 0 on success otherwise negative error code
 */
int ste_audio_io_set_vibr_gain(AUDIOIO_CH_INDEX channel_index,uint16 gain_index,
					int32 gain_value, uint32 linear)
{

	int error=0;
	unsigned char initial_val=0;

	if (channel_index & e_CHANNEL_1) {
		/*Set Gain VibR*/
		MSG0("Set Gain VibR");
		if (gain_index == 0) {
			int gain=0;
			gain = 0 - gain_value;

			initial_val = HW_REG_READ(DA6_DIGITAL_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(DA6_DIGITAL_GAIN_REG, ((initial_val
			& (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain VibR gain index=%d %d",
							gain_index,error);
				return error;
			}
		}
	}
	return error;
}
/**
 * \brief Set gain of ihf along a specified channel
 * \param channel_index Channel-index of ihf
 * \param gain_index Gain index of ihf
 * \param gain_value Gain value of ihf
 * \param linear
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_set_ihf_gain(AUDIOIO_CH_INDEX channel_index,uint16 gain_index,
						int32 gain_value, uint32 linear)
{
	int error=0;
	unsigned char initial_val=0;

	if (channel_index & e_CHANNEL_1) {
		/*Set Gain IHFL*/
		MSG0("Set Gain IHFL");
		if (gain_index == 0) {
			int gain=0;
			gain = 0 - gain_value;

			initial_val = HW_REG_READ(DA3_DIGITAL_GAIN_REG);
			error = HW_REG_WRITE(DA3_DIGITAL_GAIN_REG, ((initial_val
			& (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR: Set Gain IHFL gain index=%d %d",
							gain_index, error);
				return error;
			}

		}
	}
	if (channel_index & e_CHANNEL_2) {
		/*Set Gain IHFR*/
		MSG0("Set Gain IHFR");
		if (gain_index == 0) {
			int gain=0;
			gain = 0 - gain_value;

			initial_val = HW_REG_READ(DA4_DIGITAL_GAIN_REG);
			error = HW_REG_WRITE(DA4_DIGITAL_GAIN_REG, ((initial_val
			&  (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR: Set Gain IHFR gain index=%d %d",
							gain_index, error);
				return error;
			}
		}
	}

	return error;
}
/**
 * \brief Set gain of MIC1A & MIC1B
 * \param channel_index Channel-index of MIC1
 * \param gain_index Gain index of MIC1
 * \param gain_value Gain value of MIC1
 * \param linear
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_set_mic1a_gain(AUDIOIO_CH_INDEX channel_index,
			uint16 gain_index, int32 gain_value, uint32 linear)
{
	int error=0;
	unsigned char initial_val=0;

	if (channel_index & e_CHANNEL_1) {
		/*Set Gain Mic1*/
		MSG0("Set Gain Mic1");
		if (gain_index == 0) {
			int gain=0;
			gain = 31 - gain_value;

			initial_val = HW_REG_READ(AD3_DIGITAL_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(AD3_DIGITAL_GAIN_REG, ((initial_val
			&  (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain Mic1 gain index=%d %d",
							gain_index,error);
				return error;
			}

		}

		if (gain_index == 1) {
			/*Set Analog gain*/

			initial_val = HW_REG_READ(ANALOG_MIC1_GAIN_REG);

			/*Write gain*/
			error = HW_REG_WRITE(ANALOG_MIC1_GAIN_REG, ((initial_val
			& (~MIC_ANALOG_GAIN_MASK)) | (gain_value &
							MIC_ANALOG_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain Mic1 gain index=%d %d",
							gain_index,error);
				return error;
			}
		}
	}
	return error;
}
/**
 * \brief Set gain of MIC2
 * \param channel_index Channel-index of MIC2
 * \param gain_index Gain index of MIC2
 * \param gain_value Gain value of MIC2
 * \param linear
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_set_mic2_gain(AUDIOIO_CH_INDEX channel_index,uint16 gain_index,
						int32 gain_value, uint32 linear)
{
	int error=0;
	unsigned char initial_val=0;

	if (channel_index & e_CHANNEL_1) {
		/*Set Gain Mic2*/
		MSG0("Set Gain Mic2");
		if (gain_index == 0) {
			int gain=0;
			gain = 31 - gain_value;

		    initial_val = HW_REG_READ(AD2_DIGITAL_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(AD2_DIGITAL_GAIN_REG, ((initial_val
			& (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain Mic2 gain index=%d %d",
							gain_index,error);
				return error;
			}

		}

		if (gain_index == 1) {
			/*Set Analog gain*/
			initial_val = HW_REG_READ(ANALOG_MIC2_GAIN_REG);

			/*Write gain*/
			error = HW_REG_WRITE(ANALOG_MIC2_GAIN_REG, ((initial_val
			& (~MIC_ANALOG_GAIN_MASK)) | (gain_value &
							MIC_ANALOG_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR: Set Gain Mic2 gain index=%d %d",
							gain_index, error);
				return error;
			}
		}
	}
	return error;
}
/**
 * \brief Set gain of Lin IN along a specified channel
 * \param channel_index Channel-index of Lin In
 * \param gain_index Gain index of Lin In
 * \param gain_value Gain value of Lin In
 * \param linear
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_set_lin_gain(AUDIOIO_CH_INDEX channel_index,uint16 gain_index,
						int32 gain_value, uint32 linear)
{
	int error=0;
	unsigned char initial_val=0;

	if (channel_index & e_CHANNEL_1) {
		MSG0("Set Gain LinInL");
		if (gain_index == 0) {
			int gain=0;
			gain = 31 - gain_value;

			initial_val = HW_REG_READ(AD1_DIGITAL_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(AD1_DIGITAL_GAIN_REG, ((initial_val
			& (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain LinInL gain index=%d %d",
							gain_index, error);
				return error;
			}

		}

		if (gain_index == 1) {
			int gain=0;
			/*Converting -10 to 20 range into 0 - 15
				& shifting it left by 4 bits*/
			gain = ((gain_value/2) + 5)<<4;

			initial_val = HW_REG_READ(ANALOG_LINE_IN_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(ANALOG_LINE_IN_GAIN_REG,
			((initial_val & (~L_ANALOG_GAIN_MASK)) | (gain &
							L_ANALOG_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain LinInL gain index=%d %d",
							gain_index, error);
				return error;
			}
		}
	}

	if (channel_index & e_CHANNEL_2) {
		/*Set Gain LinInR*/
		MSG0("Set Gain LinInR");
		if (gain_index == 0) {
			int gain=0;
			gain = 31 - gain_value;

			initial_val = HW_REG_READ(AD2_DIGITAL_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(AD2_DIGITAL_GAIN_REG, ((initial_val
			& (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR: Set Gain LinInR gain index=%d%d",
							gain_index, error);
				return error;
			}
		}
		if (gain_index == 1) {
			int gain=0;
			/*Converting -10 to 20 range into 0 - 15*/
			gain = ((gain_value/2) + 5);

			initial_val = HW_REG_READ(ANALOG_LINE_IN_GAIN_REG);
			/*Write gain*/
			error = HW_REG_WRITE(ANALOG_LINE_IN_GAIN_REG,
			((initial_val & (~R_ANALOG_GAIN_MASK)) | (gain &
							R_ANALOG_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR:Set Gain LinInR gain index=%d %d",
							gain_index, error);
				return error;
			}
		}
	}

	return error;
}
/**
 * \brief Set gain of DMIC12 along a specified channel
 * \param channel_index Channel-index of DMIC12
 * \param gain_index Gain index of DMIC12
 * \param gain_value Gain value of DMIC12
 * \param linear
 * \return 0 on success otherwise negative error code
 */

int ste_audio_io_set_dmic12_gain(AUDIOIO_CH_INDEX channel_index,
			uint16 gain_index,int32 gain_value, uint32 linear)
{
	int error=0;
	unsigned char initial_val=0;

	if (channel_index & e_CHANNEL_1) {
		/*Set Gain DMIC1*/
		MSG0("Set Gain DMIC1");
		if (gain_index == 0) {
			int gain=0;
			gain = 31 - gain_value;

			initial_val = HW_REG_READ(AD1_DIGITAL_GAIN_REG);
			error = HW_REG_WRITE(AD1_DIGITAL_GAIN_REG, ((initial_val
			& (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK )));
			if (error != 0) {
				printk("ERROR: Set Gain DMic1 gain index=%d %d",
							gain_index, error);
				return error;
			 }
		 }
	 }
	if (channel_index & e_CHANNEL_2) {
	/*Set Gain DMIC2*/
		if (gain_index == 0) {
			int gain=0;
			gain = 31 - gain_value;

			initial_val = HW_REG_READ(AD2_DIGITAL_GAIN_REG);
			error = HW_REG_WRITE(AD2_DIGITAL_GAIN_REG, ((initial_val
			& (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK)));
			if (error != 0) {
				printk("ERROR: Set Gain DMic2 gain index=%d %d",
							gain_index, error);
				return error;
			 }
		  }
	  }
	return error;
}

int ste_audio_io_switch_to_burst_mode_headset(
	int32 burst_fifo_interrupt_sample_count,int32 burst_fifo_length,
		int32 burst_fifo_switch_frame,int32 burst_fifo_sample_number)
{
	int error = 0;

	error = HW_REG_WRITE(BURST_FIFO_INT_CONTROL_REG,
					burst_fifo_interrupt_sample_count);
	if(0 != error) {
				return error;
			 }
	error = HW_REG_WRITE(BURST_FIFO_LENGTH_REG,burst_fifo_length);
	if(0 != error) {
		return error;
		 }
	error = HW_REG_WRITE(BURST_FIFO_SWITCH_FRAME_REG,
						burst_fifo_switch_frame);
	if(0 != error) {
		return error;
	 }

	error = HW_ACODEC_MODIFY_WRITE(BURST_FIFO_INT_CONTROL_REG,0,BFIFO_MASK);
	if(0 != error) {
				return error;
			}

	error = HW_REG_WRITE(TDM_IF_BYPASS_B_FIFO_REG,EN_IF0_BFIFO);
	if(0 != error) {
return error;
}


	/*burst_fifo_sample_number = HW_REG_READ(BURST_FIFO_SAMPLES_REG);*/


	return error;
}
int ste_audio_io_switch_to_normal_mode_headset(void)
{
	int error = 0;

	error = HW_REG_WRITE(TDM_IF_BYPASS_B_FIFO_REG,DIS_IF0_BFIFO);
	if(0 != error) {
		return error;
	}

	return error;
}


int ste_audio_io_mute_vibl(AUDIOIO_CH_INDEX channel_index)
{
	return 0;
}

int ste_audio_io_unmute_vibl(AUDIOIO_CH_INDEX channel_index, int *gain)
{
	return 0;
}

int ste_audio_io_mute_vibr(AUDIOIO_CH_INDEX channel_index)
{
	return 0;
}
int ste_audio_io_unmute_vibr(AUDIOIO_CH_INDEX channel_index, int *gain)
{
	return 0;
}

int ste_audio_io_mute_dmic12(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	if ((channel_index & (e_CHANNEL_1 | e_CHANNEL_2))) {
	error = ste_audio_io_set_dmic12_gain(channel_index,0,-32, 0);
		if (error != 0) {
			printk("ERROR: Mute dmic12 %d", error);
			return error;
		}
	}

	return error;

}

int ste_audio_io_unmute_dmic12(AUDIOIO_CH_INDEX channel_index, int *gain)
{
	int error = 0;
	if ((channel_index & (e_CHANNEL_1 | e_CHANNEL_2))) {
	error = ste_audio_io_set_dmic12_gain(channel_index,0,gain[0], 0);
		if (error != 0) {
			printk("ERROR: UnMute dmic12 %d", error);
			return error;
		}
	}
	return error;
}
int ste_audio_io_enable_fade_dmic12(void)
{
	return 0;
}

int ste_audio_io_disable_fade_dmic12(void)
{
	return 0;
}

int ste_audio_io_power_up_dmic34(AUDIOIO_CH_INDEX channel_index)
{
	int error=0;
	unsigned char initialVal_AD=0;

	/*Check if DMic34 request is mono or Stereo*/
	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2 ))) {
		printk("DMic34 does not support more than 2 channels");
		return -EINVAL;
	}

	/*Setting Direction for GPIO pins on AB8500*/
	error = HW_REG_WRITE(0x1013, 0x10);
	if (error != 0) {
		printk("Setting Direction for GPIO pins on AB8500 %d", error);
		return error;
	}

	if (channel_index & e_CHANNEL_1) {
	/*Check if DMIC3 is already powered up or used by Mic1A or Mic1B*/
	initialVal_AD = HW_REG_READ(DIGITAL_AD_CHANNELS_ENABLE_REG);

	if (initialVal_AD & (EN_AD3)) {
		return 0;
	}

	MSG0("Slot 02 outputs data from AD_OUT3");
	error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT2_3_REG,
							DATA_FROM_AD_OUT3, 0);
	if(error != 0) {
		printk("ERROR: Slot 02 outputs data from AD_OUT3 %d", error);
		return error;
	}

	MSG0("Enable AD3 for DMIC3 channel");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG, EN_AD3,
									0);
	if (error != 0) {
		printk("ERROR: Enable AD3 for DMIC3 %d", error);
		return error;
	}

	MSG0("Select DMIC3 for AD_OUT3");
	error = HW_ACODEC_MODIFY_WRITE(DIGITAL_MUXES_REG1,SEL_DMIC3_FOR_AD_OUT3,
									0);
	if (error != 0) {
		printk("ERROR: Select DMIC3 for AD_OUT3 %d", error);
		return error;
	}

	MSG0("Enable DMIC3");
	error = HW_ACODEC_MODIFY_WRITE(DMIC_ENABLE_REG, EN_DMIC3, 0);
	if (error != 0) {
		printk("ERROR: Enable DMIC3 %d", error);
		return error;
	}
}

	/*Enable AD4 for DMIC4*/
	if(channel_index & e_CHANNEL_2) {
		/*Check if DMIC4 is already powered up*/
		if(initialVal_AD & (EN_AD4)) {
	return 0;
}

		MSG0("Slot 03 outputs data from AD_OUT4");
		error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT2_3_REG,
						(DATA_FROM_AD_OUT4<<4), 0);
		if (error != 0){
			printk("ERROR: Slot 03 outputs data from AD_OUT4 %d",
									error);
			return error;
		}

		MSG0("Enable AD4 for DMIC4 channel");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,
								EN_AD4, 0);
		if (error != 0){
			printk("ERROR: Enable AD4 for DMIC4 %d", error);
			return error;
		}

		MSG0("Enable DMIC4");
		error = HW_ACODEC_MODIFY_WRITE(DMIC_ENABLE_REG, EN_DMIC4, 0);
		if (error != 0) {
			printk("ERROR: Enable DMIC4 %d", error);
			return error;
		}
	}
	return error;
 }

int ste_audio_io_power_down_dmic34(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	unsigned char initialVal_AD=0;


	if (!(channel_index & (e_CHANNEL_1 | e_CHANNEL_2))) {
		printk("DMic34 does not support more than 2 channels");
		return -EINVAL;
	}

	/*Setting Direction for GPIO pins on AB8500*/
	error = HW_ACODEC_MODIFY_WRITE(0x1013, 0x0, 0x10);
	if (error != 0) {
		printk("Clearing Direction for GPIO pins on AB8500 %d", error);
		return error;
	}

	/*Enable AD1 for DMIC1*/
	if (channel_index & e_CHANNEL_1) {
	/*Check if DMIC3 is already powered Down or used by Mic1A or Mic1B*/
		initialVal_AD = HW_REG_READ(DIGITAL_AD_CHANNELS_ENABLE_REG);
		if (!(initialVal_AD & EN_AD3))
	return 0;

		error = HW_ACODEC_MODIFY_WRITE(DMIC_ENABLE_REG, 0, EN_DMIC3);
		if(error != 0) {
			printk("ERROR: Enable DMIC3 %d", error);
			return error;
		}

		MSG0("Disable AD3 for DMIC3 channel");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,0,
									EN_AD3);
		if(error != 0) {
			printk("ERROR: Disable AD3 for DMIC3 %d", error);
			return error;
		}

		MSG0("Slot 02 outputs data cleared from AD_OUT3");
		error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT2_3_REG, 0,
							DATA_FROM_AD_OUT3);
		if(error != 0) {
			printk("ERROR:Slot 02 outputs data cleared from AD_OUT3"
								" %d", error);
			return error;
		}
	}

	/*Enable AD4 for DMIC4*/
	if (channel_index & e_CHANNEL_2) {
		/*Check if DMIC4 is already powered Down*/
		initialVal_AD = HW_REG_READ(DIGITAL_AD_CHANNELS_ENABLE_REG);
		if (!(initialVal_AD & EN_AD4))
			return 0;

		MSG0("Enable DMIC4");
		error = HW_ACODEC_MODIFY_WRITE(DMIC_ENABLE_REG, 0, EN_DMIC4);
		if (error != 0){
			printk("ERROR: Enable DMIC4 %d", error);
			return error;
		}

		MSG0("Disable AD4 for DMIC4 channel");
		error = HW_ACODEC_MODIFY_WRITE(DIGITAL_AD_CHANNELS_ENABLE_REG,0,
									EN_AD4);
		if (error != 0){
			printk("ERROR: Disable AD4 for DMIC4 %d", error);
			return error;
		}

		MSG0("Slot 03 outputs data cleared from AD_OUT3");
		error = HW_ACODEC_MODIFY_WRITE(AD_ALLOCATION_TO_SLOT2_3_REG, 0,
							(DATA_FROM_AD_OUT4<<4));
		if (error != 0){
			printk("ERROR:Slot 03 outputs data cleared from AD_OUT4"
								" %d", error);
			return error;
		}
	}
	return error;
}
int ste_audio_io_set_dmic34_gain(AUDIOIO_CH_INDEX channel_index,
			uint16 gain_index, int32 gain_value, uint32 linear)
{
	int error=0;
	unsigned char initial_val=0;

	if (channel_index & e_CHANNEL_1) {
		/*Set Gain DMIC3*/
		MSG0("Set Gain DMIC3");
		if (gain_index == 0) {
			int gain=0;
			gain = 31 - gain_value;

			initial_val = HW_REG_READ(AD3_DIGITAL_GAIN_REG);
			error = HW_REG_WRITE(AD3_DIGITAL_GAIN_REG, ((initial_val
			& (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK )));
			if (error != 0) {
				printk("ERROR: Set Gain DMic3 gain index=%d %d",
							gain_index, error);
				return error;
			 }
		 }
	 }

	if (channel_index & e_CHANNEL_2) {
		/*Set Gain DMIC4*/
		MSG0("Set Gain DMIC4");
		if (gain_index == 0) {
			int gain=0;
			gain = 31 - gain_value;

			initial_val = HW_REG_READ(AD4_DIGITAL_GAIN_REG);
			error = HW_REG_WRITE(AD4_DIGITAL_GAIN_REG, ((initial_val
			& (~DIGITAL_GAIN_MASK)) | (gain & DIGITAL_GAIN_MASK )));

			if (error != 0) {
				printk("ERROR: Set Gain DMic4 gain index=%d %d",
							gain_index, error);
				return error;
			}
		 }
	 }

	return error;
}
int ste_audio_io_mute_dmic34(AUDIOIO_CH_INDEX channel_index)
{
	int error = 0;
	if ((channel_index & (e_CHANNEL_1 | e_CHANNEL_2))) {
	error = ste_audio_io_set_dmic34_gain(channel_index,0,-32, 0);
		if (error != 0) {
			printk("ERROR: Mute dmic34 %d", error);
			return error;
		}
	}
	return error;
}
int ste_audio_io_unmute_dmic34(AUDIOIO_CH_INDEX channel_index, int *gain)
{
	int error = 0;
	if ((channel_index & (e_CHANNEL_1 | e_CHANNEL_2))) {
	error = ste_audio_io_set_dmic34_gain(channel_index,0,gain[0], 0);
		if (error != 0) {
			printk("ERROR: UnMute dmic34 %d", error);
			return error;
		}
	}
	return error;
}
int ste_audio_io_enable_fade_dmic34(void)
{
	return 0;
}

int ste_audio_io_disable_fade_dmic34(void)
{
	return 0;
}

int ste_audio_io_power_up_dmic56(AUDIOIO_CH_INDEX channel_index)
{
	return 0;
}
int ste_audio_io_power_down_dmic56(AUDIOIO_CH_INDEX channel_index)
{
	return 0;
}
int ste_audio_io_set_dmic56_gain(AUDIOIO_CH_INDEX channel_index,
			uint16 gain_index,int32 gain_value, uint32 linear)
{
	return 0;
}
int ste_audio_io_mute_dmic56(AUDIOIO_CH_INDEX channel_index)
{
	return 0;
}
int ste_audio_io_unmute_dmic56(AUDIOIO_CH_INDEX channel_index, int *gain)
{
	return 0;
}
int ste_audio_io_enable_fade_dmic56(void)
{
	return 0;
}

int ste_audio_io_disable_fade_dmic56(void)
{
	return 0;
}

int dump_acodec_registers(const char *str)
{
#ifdef DEBUG_AUDIOIO
	u8 i;
	printk("\n func: %s\n",str);
	for (i = 0; i <= 0x6D; i++)
		printk("block=0x0D, adr=%x = %x\n", i,
			ab8500_read(AB8500_AUDIO, i));
#endif
	str = str; /*keep compiler happy*/
	return 0;
}

void debug_print(char *str)
{
	str = str; /*keep compiler happy*/
#ifdef DEBUG_AUDIOIO
	printk("\n%s",str);
#endif
}






