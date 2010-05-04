/*
* Overview:
*   AB8500 register definitions
*
* Copyright (C) 2009 ST Ericsson
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/


#ifndef _AUDIOIO_REG_DEFS_H_
#define _AUDIOIO_REG_DEFS_H_


/*---------------------------------------------------------------------
 * STW4500 specific Defines
 *--------------------------------------------------------------------*/

 // Registers
#define POWER_UP_CONTROL_REG 			0x0D00	//General power up
#define SOFTWARE_RESET_REG 			0x0D01	//Software Reset
#define DIGITAL_AD_CHANNELS_ENABLE_REG 		0x0D02	//Digital AD Channels Enable
#define DIGITAL_DA_CHANNELS_ENABLE_REG	 	0x0D03	//Digital DA Channels Enable
#define LOW_POWER_HS_EAR_CONF_REG		0x0D04	//Low Power and HS/EAR Configuration
#define LINE_IN_MIC_CONF_REG			0x0D05	//Line-in Mic Configuration
#define DMIC_ENABLE_REG				0x0D06	//DMic Enable
#define ADC_DAC_ENABLE_REG			0x0D07	//ADC/DAC Enable
#define ANALOG_OUTPUT_ENABLE_REG		0x0D08	//Analog Output Enable
#define DIGITAL_OUTPUT_ENABLE_REG		0x0D09	//Digital Output Enable
#define MUTE_HS_EAR_REG				0x0D0A	//Mute Enable
#define SHORT_CIRCUIT_DISABLE_REG		0x0D0B	//Short Circuit Disable HS/EAR and HS configuration
#define NCP_ENABLE_HS_AUTOSTART_REG		0x0D0C	//NCP enable, HS autostart
#define ENVELOPE_THRESHOLD_REG			0x0D0D	//Envelope Thresholds
#define ENVELOPE_DECAY_TIME_REG			0x0D0E	//Envelope Decay Time
#define VIB_DRIVER_CONF_REG			0x0D0F	//Class-D Vib Configuration
#define PWM_VIBNL_CONF_REG			0x0D10	//PWM VIBNL Configuration
#define PWM_VIBPL_CONF_REG			0x0D11	//PWM VIBPL Configuration
#define PWM_VIBNR_CONF_REG			0x0D12	//PWM VIBNR Configuration
#define PWM_VIBPR_CONF_REG			0x0D13	//PWM VIBPR Configuration
#define ANALOG_MIC1_GAIN_REG			0x0D14	//Microphone 1 Analog Gain
#define ANALOG_MIC2_GAIN_REG			0x0D15	//Microphone 2 Analog Gain
#define ANALOG_HS_GAIN_REG			0x0D16	//HS Analog Gain
#define ANALOG_LINE_IN_GAIN_REG			0x0D17	//Line-In Analog Gain
#define LINE_IN_TO_HSL_GAIN_REG			0x0D18	//Line-in to HSL Gain
#define LINE_IN_TO_HSR_GAIN_REG			0x0D19	//Line-in to HSR Gain
#define AD_FILTER_CONF_REG			0x0D1A	//AD Channel Filters Configuration
#define IF0_IF1_MASTER_CONF_REG			0x0D1B	//Audio Interfaces 0/1 Master Configuration
#define IF0_CONF_REG				0x0D1C	//Audio Interface 0 Configuration
#define TDM_IF_BYPASS_B_FIFO_REG		0x0D1D	//TDM Audio Interface Bypass control and B-FIFO enable
#define IF1_CONF_REG				0x0D1E	//Audio Interface 1 Configuration
#define AD_ALLOCATION_TO_SLOT0_1_REG		0x0D1F	//AD Data allocation in Slot 0 and 1
#define AD_ALLOCATION_TO_SLOT2_3_REG		0x0D20	//AD Data allocation in Slot 2 and 3
#define AD_ALLOCATION_TO_SLOT4_5_REG		0x0D21	//AD Data allocation in Slot 4 and 5
#define AD_ALLOCATION_TO_SLOT6_7_REG		0x0D22	//AD Data allocation in Slot 6 and 7
#define AD_ALLOCATION_TO_SLOT8_9_REG		0x0D23	//AD Data allocation in Slot 8 and 9
#define AD_ALLOCATION_TO_SLOT10_11_REG		0x0D24	//AD Data allocation in Slot 10 and 11
#define AD_ALLOCATION_TO_SLOT12_13_REG		0x0D25	//AD Data allocation in Slot 12 and 13
#define AD_ALLOCATION_TO_SLOT14_15_REG		0x0D26	//AD Data allocation in Slot 14 and 15
#define AD_ALLOCATION_TO_SLOT16_17_REG		0x0D27	//AD Data allocation in Slot 16 and 17
#define AD_ALLOCATION_TO_SLOT18_19_REG		0x0D28	//AD Data allocation in Slot 18 and 19
#define AD_ALLOCATION_TO_SLOT20_21_REG		0x0D29	//AD Data allocation in Slot 20 and 21
#define AD_ALLOCATION_TO_SLOT22_23_REG		0x0D2A	//AD Data allocation in Slot 22 and 23
#define AD_ALLOCATION_TO_SLOT24_25_REG		0x0D2B	//AD Data allocation in Slot 24 and 25
#define AD_ALLOCATION_TO_SLOT26_27_REG		0x0D2C	//AD Data allocation in Slot 26 and 27
#define AD_ALLOCATION_TO_SLOT28_29_REG		0x0D2D	//AD Data allocation in Slot 28 and 29
#define AD_ALLOCATION_TO_SLOT30_31_REG		0x0D2E	//AD Data allocation in Slot 30 and 31
#define AD_SLOT_0_TO_7_TRISTATE_REG		0x0D2F	//AD slot 0/7 tristate
#define AD_SLOT_8_TO_15_TRISTATE_REG		0x0D30	//AD slot 8/15 tristate
#define AD_SLOT_16_TO_23_TRISTATE_REG		0x0D31	//AD slot 16/23 tristate
#define AD_SLOT_24_TO_31_TRISTATE_REG		0x0D32	//AD slot 24/31 tristate
#define SLOT_SELECTION_TO_DA1_REG		0x0D33	//Slots selection for DA path 1
#define SLOT_SELECTION_TO_DA2_REG		0x0D34	//Slots selection for DA path 2
#define SLOT_SELECTION_TO_DA3_REG		0x0D35	//Slots selection for DA path 3
#define SLOT_SELECTION_TO_DA4_REG		0x0D36	//Slots selection for DA path 4
#define SLOT_SELECTION_TO_DA5_REG		0x0D37	//Slots selection for DA path 5
#define SLOT_SELECTION_TO_DA6_REG		0x0D38	//Slots selection for DA path 6
#define SLOT_SELECTION_TO_DA7_REG		0x0D39	//Slots selection for DA path 7
#define SLOT_SELECTION_TO_DA8_REG		0x0D3A	//Slots selection for DA path 8
#define CLASS_D_EMI_PARALLEL_CONF_REG		0x0D3B	//Class-D EMI and Parallel Configuration control
#define CLASS_D_PATH_CONTROL_REG		0x0D3C	//Class-D path control
#define CLASS_D_DITHER_CONTROL_REG		0x0D3D	//Class-D dither control
#define DMIC_DECIMATOR_FILTER_REG		0x0D3E	//DMIC decimator filter
#define DIGITAL_MUXES_REG1			0x0D3F	//Digital Multiplexers
#define DIGITAL_MUXES_REG2			0x0D40	//Digital Multiplexers
#define AD1_DIGITAL_GAIN_REG			0x0D41	//AD1 Digital Gain
#define AD2_DIGITAL_GAIN_REG			0x0D42	//AD2 Digital Gain
#define AD3_DIGITAL_GAIN_REG			0x0D43	//AD3 Digital Gain
#define AD4_DIGITAL_GAIN_REG			0x0D44	//AD4 Digital Gain
#define AD5_DIGITAL_GAIN_REG			0x0D45	//AD5 Digital Gain
#define AD6_DIGITAL_GAIN_REG			0x0D46	//AD6 Digital Gain
#define DA1_DIGITAL_GAIN_REG			0x0D47	//DA1 Digital Gain
#define DA2_DIGITAL_GAIN_REG			0x0D48	//DA2 Digital Gain
#define DA3_DIGITAL_GAIN_REG			0x0D49	//DA3 Digital Gain
#define DA4_DIGITAL_GAIN_REG			0x0D4A	//DA4 Digital Gain
#define DA5_DIGITAL_GAIN_REG			0x0D4B	//DA5 Digital Gain
#define DA6_DIGITAL_GAIN_REG			0x0D4C	//DA6 Digital Gain
#define AD1_TO_HFL_DIGITAL_GAIN_REG		0x0D4D	//AD1 loopback to HFL digital gain
#define AD2_TO_HFR_DIGITAL_GAIN_REG		0x0D4E	//AD2 loopback to HFR digital gain
#define HSL_EAR_DIGITAL_GAIN_REG		0x0D4F	//HSL/EAR digital gain
#define HSR_DIGITAL_GAIN_REG			0x0D50	//HSR digital gain
#define SIDETONE_FIR1_GAIN_REG			0x0D51	//Side tone FIR1 gain
#define SIDETONE_FIR2_GAIN_REG			0x0D52	//Side tone FIR2 gain
#define ANC_FILTER_CONTROL_REG			0x0D53	//ANC filter control
#define ANC_WARPED_GAIN_REG			0x0D54	//ANC Warped Delay Line Shift
#define ANC_FIR_OUTPUT_GAIN_REG			0x0D55	//ANC FIR output Shift
#define ANC_IIR_OUTPUT_GAIN_REG			0x0D56	//ANC IIR output Shift
#define ANC_FIR_COEFF_MSB_REG			0x0D57	//ANC FIR coefficients msb
#define ANC_FIR_COEFF_LSB_REG			0x0D58	//ANC FIR coefficients lsb
#define ANC_IIR_COEFF_MSB_REG			0x0D59	//ANC IIR coefficients msb
#define ANC_IIR_COEFF_LSB_REG			0x0D5A	//ANC IIR coefficients lsb
#define ANC_WARP_DELAY_MSB_REG			0x0D5B	//ANC Warp delay msb
#define ANC_WARP_DELAY_LSB_REG			0x0D5C	//ANC Warp delay lsb
#define ANC_FIR_PEAK_MSB_REG			0x0D5D	//ANC FIR peak register MSB
#define ANC_FIR_PEAK_LSB_REG			0x0D5E	//ANC FIR peak register LSB
#define ANC_IIR_PEAK_MSB_REG			0x0D5F	//ANC IIR peak register MSB
#define ANC_IIR_PEAK_LSB_REG			0x0D60	//ANC IIR peak register LSB
#define SIDETONE_FIR_ADDR_REG			0x0D61	//Side-tone FIR address
#define SIDETONE_FIR_COEFF_MSB_REG		0x0D62	//Side tone FIR coefficient MSB
#define SIDETONE_FIR_COEFF_LSB_REG		0x0D63	//Side tone FIR coefficient LSB
#define FILTERS_CONTROL_REG			0x0D64	//Filters control
#define IRQ_MASK_LSB_REG			0x0D65	//IRQ mask lsb
#define IRQ_STATUS_LSB_REG			0x0D66	//IRQ status lsb
#define IRQ_MASK_MSB_REG			0x0D67	//IRQ mask msb
#define IRQ_STATUS_MSB_REG			0x0D68	//IRQ status msb
#define BURST_FIFO_INT_CONTROL_REG		0x0D69	//Burst FIFO INT control
#define BURST_FIFO_LENGTH_REG			0x0D6A	//Burst FIFO length
#define BURST_FIFO_CONTROL_REG			0x0D6B	//Burst FIFO control
#define BURST_FIFO_SWITCH_FRAME_REG		0x0D6C	//Burst FIFO switch frame
#define BURST_FIFO_WAKE_UP_DELAY_REG		0x0D6D	//Burst FIFO wake up delay
#define BURST_FIFO_SAMPLES_REG			0x0D6E	//Burst FIFO samples number
#define REVISION_REG				0x0D6F	//Revision

//POWER_UP_CONTROL_REG Masks
#define DEVICE_POWER_UP		  		0x80
#define ANALOG_PARTS_POWER_UP 			0x08

//SOFTWARE_RESET_REG Masks
#define SW_RESET				0x80

//DIGITAL_AD_CHANNELS_ENABLE_REG Masks
#define EN_AD1					0x80
#define EN_AD2					0x80
#define EN_AD3					0x20
#define EN_AD4					0x20
#define EN_AD5					0x08
#define EN_AD6					0x04

//DIGITAL_DA_CHANNELS_ENABLE_REG Masks
#define EN_DA1					0x80
#define EN_DA2					0x40
#define EN_DA3					0x20
#define EN_DA4					0x10
#define EN_DA5					0x08
#define EN_DA6					0x04

//LOW_POWER_HS_EAR_CONF_REG Masks
#define LOW_POWER_HS				0x80
#define HS_DAC_DRIVER_LP			0x40
#define HS_DAC_LP				0x20
#define EAR_DAC_LP				0x10

//LINE_IN_MIC_CONF_REG Masks
#define EN_MIC1					0x80
#define EN_MIC2					0x40
#define EN_LIN_IN_L				0x20
#define EN_LIN_IN_R				0x10
#define MUT_MIC1				0x08
#define MUT_MIC2				0x04
#define MUT_LIN_IN_L				0x02
#define MUT_LIN_IN_R				0x01

//DMIC_ENABLE_REG Masks
#define EN_DMIC1				0x80
#define EN_DMIC2				0x40
#define EN_DMIC3				0x20
#define EN_DMIC4				0x10
#define EN_DMIC5				0x08
#define EN_DMIC6				0x04

//ADC_DAC_ENABLE_REG Masks
#define SEL_MIC1B_CLR_MIC1A			0x80
#define SEL_LINR_CLR_MIC2			0x40
#define POWER_UP_HSL_DAC			0x20
#define POWER_UP_HSR_DAC			0x10
#define POWER_UP_ADC1				0x04
#define POWER_UP_ADC3				0x02
#define POWER_UP_ADC2				0x01

//ANALOG_OUTPUT_ENABLE_REG and DIGITAL_OUTPUT_ENABLE_REG and MUTE_HS_EAR_REG Masks
#define EN_EAR_MASK				0x40
#define EN_HSL_MASK				0x20
#define EN_HSR_MASK				0x10
#define EN_HFL_MASK				0x08
#define EN_HFR_MASK				0x04
#define EN_VIBL_MASK				0x02
#define EN_VIBR_MASK				0x01

//SHORT_CIRCUIT_DISABLE_REG Masks
#define HS_SHORT_DIS				0x20
#define HS_PULL_DOWN_EN				0x10	//Not in High Impedance
#define HS_OSC_EN				0x04
#define DIS_HS_FAD				0x02
#define HS_ZCD_DIS				0x01

//NCP_ENABLE_HS_AUTOSTART_REG Masks
#define EN_NEG_CP				0x80

//ANALOG_MIC1_GAIN_REG and ANALOG_MIC1_GAIN_REG Masks
#define MIC_ANALOG_GAIN_MASK			0x1F

//ANALOG_HS_GAIN_REG and ANALOG_LINE_IN_GAIN_REG Masks
#define L_ANALOG_GAIN_MASK			0xF0
#define R_ANALOG_GAIN_MASK			0x0F

//IF0_IF1_MASTER_CONF_REG Masks
#define EN_MASTGEN				0x80
#define BITCLK_OSR_N_64				0x02
#define BITCLK_OSR_N_128			0x04
#define BITCLK_OSR_N_256			0x06
#define EN_FSYNC_BITCLK				0x01

//IF0_CONF_REG and IF1_CONF_REG Masks
#define FSYNC_FALLING_EDGE			0x40
#define BITCLK_FALLING_EDGE			0x20
#define IF_DELAYED				0x10
#define I2S_LEFT_ALIGNED_FORMAT			0x08
#define TDM_FORMAT				0x04
#define WORD_LENGTH_32				0x03
#define WORD_LENGTH_24				0x02
#define WORD_LENGTH_20				0x01
#define WORD_LENGTH_16				0x00

//TDM_IF_BYPASS_B_FIFO_REG Masks
#define IF0_MASTER				0x02

//AD_ALLOCATION_TO_SLOT0_1_REG and AD_ALLOCATION_TO_SLOT2_3_REG and AD_ALLOCATION_TO_SLOT4_5_REG and AD_ALLOCATION_TO_SLOT6_7_REG Masks
#define DATA_FROM_AD_OUT1			0x00
#define DATA_FROM_AD_OUT2			0x01
#define DATA_FROM_AD_OUT3			0x02
#define DATA_FROM_AD_OUT4			0x03
#define DATA_FROM_AD_OUT5			0x04
#define DATA_FROM_AD_OUT6			0x05
#define DATA_FROM_AD_OUT7			0x06
#define DATA_FROM_AD_OUT8			0x07
#define TRISTATE				0x0C

//SLOT_SELECTION_TO_DA1_REG and SLOT_SELECTION_TO_DA2_REG and SLOT_SELECTION_TO_DA3_REG and SLOT_SELECTION_TO_DA4_REG Masks
//SLOT_SELECTION_TO_DA5_REG and SLOT_SELECTION_TO_DA6_REG  Masks
#define SLOT08_FOR_DA_PATH			0x08
#define SLOT09_FOR_DA_PATH			0x09
#define SLOT10_FOR_DA_PATH			0x0A
#define SLOT11_FOR_DA_PATH			0x0B
#define SLOT12_FOR_DA_PATH			0x0C
#define SLOT13_FOR_DA_PATH			0x0D
#define SLOT14_FOR_DA_PATH			0x0E
#define SLOT15_FOR_DA_PATH			0x0F

//DIGITAL_MUXES_REG1 Masks
#define DA1_TO_HSL				0x80
#define DA2_TO_HSR				0x40
#define SEL_DMIC1_FOR_AD_OUT1			0x20
#define SEL_DMIC2_FOR_AD_OUT2			0x10
#define SEL_DMIC3_FOR_AD_OUT3			0x08
//#define SEL_DMIC5_FOR_AD_OUT5			0x04
//#define SEL_DMIC6_FOR_AD_OUT6			0x02
//#define SEL_DMIC1_FOR_AD_OUT1			0x01

//AD1_DIGITAL_GAIN_REG and AD2_DIGITAL_GAIN_REG and AD3_DIGITAL_GAIN_REG Masks
//AD4_DIGITAL_GAIN_REG and AD5_DIGITAL_GAIN_REG and AD6_DIGITAL_GAIN_REG Masks
//DA1_DIGITAL_GAIN_REG and DA2_DIGITAL_GAIN_REG and DA3_DIGITAL_GAIN_REG Masks
//DA4_DIGITAL_GAIN_REG and DA5_DIGITAL_GAIN_REG and DA6_DIGITAL_GAIN_REG Masks
#define DIS_FADING				0x40
#define DIGITAL_GAIN_MASK			0x3F

//HSL_EAR_DIGITAL_GAIN_REG and HSR_DIGITAL_GAIN_REG Masks
#define FADE_SPEED_MASK				0xC0
#define DIS_DIG_GAIN_FADING			0x10
#define HS_DIGITAL_GAIN_MASK			0x0F

//ENABLE_BURST_MODE Masks
#define EN_IF0_BFIFO				0x03
#define DIS_IF0_BFIFO				0x02
#define BFIFO_MASK				0x80

#endif
