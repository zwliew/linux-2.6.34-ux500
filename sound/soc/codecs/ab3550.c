/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Xie Xiaolei <xie.xiaolei@etericsson.com>,
 *         Ola Lilja ola.o.lilja@stericsson.com,
 *         Roger Nilsson 	roger.xr.nilsson@stericsson.com
 *         for ST-Ericsson.
 *
 * License terms:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/mfd/abx500.h>
#include <linux/bitops.h>
#include <asm/atomic.h>
#include "ab3550.h"


#define I2C_BANK 0

static struct device *ab3550_dev = NULL;

// Registers -----------------------------------------------------------------------------------------------------------------------

struct ab3550_register {
	const char* name;
	u8 address;
};

struct ab3550_register ab3550_registers[] = {
	{"IDX_MIC_BIAS1",				0x31},
	{"IDX_MIC_BIAS2",				0x32},
	{"IDX_MIC_BIAS2_VAD",			0x33},
	{"IDX_MIC1_GAIN",				0x34},
	{"IDX_MIC2_GAIN",				0x35},
	{"IDX_MIC1_INPUT_SELECT",		0x36},
	{"IDX_MIC2_INPUT_SELECT",		0x37},
	{"IDX_MIC1_VMID_SELECT",		0x38},
	{"IDX_MIC2_VMID_SELECT",		0x39},
	{"IDX_MIC2_TO_MIC1",			0x3A},
	{"IDX_ANALOG_LOOP_PGA1",		0x3B},
	{"IDX_ANALOG_LOOP_PGA2",		0x3C},
	{"IDX_APGA_VMID_SELECT",		0x3D},
	{"IDX_EAR",						0x3E},
	{"IDX_AUXO1",					0x3F},
	{"IDX_AUXO2",					0x40},
	{"IDX_AUXO_PWR_MODE",			0x41},
	{"IDX_OFFSET_CANCEL",			0x42},
	{"IDX_SPKR",					0x43},
	{"IDX_LINE1",					0x44},
	{"IDX_LINE2",					0x45},
	{"IDX_APGA1_ADDER",				0x46},
	{"IDX_APGA2_ADDER",				0x47},
	{"IDX_EAR_ADDER",				0x48},
	{"IDX_AUXO1_ADDER",				0x49},
	{"IDX_AUXO2_ADDER",				0x4A},
	{"IDX_SPKR_ADDER",				0x4B},
	{"IDX_LINE1_ADDER",				0x4C},
	{"IDX_LINE2_ADDER",				0x4D},
	{"IDX_EAR_TO_MIC2",				0x4E},
	{"IDX_SPKR_TO_MIC2",			0x4F},
	{"IDX_NEGATIVE_CHARGE_PUMP",	0x50},
	{"IDX_TX1",						0x51},
	{"IDX_TX2",						0x52},
	{"IDX_RX1",						0x53},
	{"IDX_RX2",						0x54},
	{"IDX_RX3",						0x55},
	{"IDX_TX_DIGITAL_PGA1",			0X56},
	{"IDX_TX_DIGITAL_PGA2",			0X57},
	{"IDX_RX1_DIGITAL_PGA",			0x58},
	{"IDX_RX2_DIGITAL_PGA",			0x59},
	{"IDX_RX3_DIGITAL_PGA",			0x5A},
	{"IDX_SIDETONE1_PGA",			0x5B},
	{"IDX_SIDETONE2_PGA",			0x5C},
	{"IDX_CLOCK",					0x5D},
	{"IDX_INTERFACE0",				0x5E},
	{"IDX_INTERFACE1",				0x60},
	{"IDX_INTERFACE0_DATA",			0x5F},
	{"IDX_INTERFACE1_DATA",			0x61},
	{"IDX_INTERFACE_LOOP",			0x62},
	{"IDX_INTERFACE_SWAP",			0x63}
};

#define SET_REG(reg, val) abx500_set_register_interruptible( \
		ab3550_dev, I2C_BANK, (reg), (val)); \
		printk(KERN_DEBUG "REG = 0x%02x, VAL = 0x%02x.\n", (reg), (val))

#define MASK_SET_REG(reg, mask, val) abx500_mask_and_set_register_interruptible( \
		ab3550_dev, I2C_BANK, (reg), (mask), (val)); \
		printk(KERN_DEBUG "REG = 0x%02x, MASK = 0x%02x, VAL = 0x%02x.\n", (reg), (mask), (val))

#define GET_REG(reg, val) abx500_get_register_interruptible( \
		ab3550_dev, I2C_BANK, (reg), (val))

static enum enum_register outamp_indices[] = {
	IDX_LINE1, IDX_LINE2, IDX_SPKR, IDX_EAR, IDX_AUXO1, IDX_AUXO2
};

static enum enum_control outamp_gain_controls[] = {
	IDX_LINE1_Gain, IDX_LINE2_Gain, IDX_SPKR_Gain, IDX_EAR_Gain, IDX_AUXO1_Gain, IDX_AUXO2_Gain
};

static enum enum_register outamp_adder_indices[] = {
	IDX_LINE1_ADDER, IDX_LINE2_ADDER, IDX_SPKR_ADDER, IDX_EAR_ADDER, IDX_AUXO1_ADDER, IDX_AUXO2_ADDER
};

//static const char* outamp_name[] = {
//	"LINE1", "LINE2", "SPKR", "EAR", "AUXO1", "AUXO2"
//};

static const u8 outamp_gain_shift[] = {
	LINEx_GAIN_SHIFT, LINEx_GAIN_SHIFT,
	SPKR_GAIN_SHIFT, EAR_GAIN_SHIFT,
	AUXOx_GAIN_SHIFT, AUXOx_GAIN_SHIFT
};
static const u8 outamp_gain_mask[] = {
	LINEx_GAIN_MASK, LINEx_GAIN_MASK,
	SPKR_GAIN_MASK, EAR_GAIN_MASK,
	AUXOx_GAIN_MASK, AUXOx_GAIN_MASK
};
static const u8 outamp_pwr_shift[] = {
	LINEx_PWR_SHIFT, LINEx_PWR_SHIFT,
	SPKR_PWR_SHIFT, EAR_PWR_SHIFT,
	AUXOx_PWR_SHIFT, AUXOx_PWR_SHIFT
};
static const u8 outamp_pwr_mask[] = {
	LINEx_PWR_MASK, LINEx_PWR_MASK,
	SPKR_PWR_MASK, EAR_PWR_MASK,
	AUXOx_PWR_MASK, AUXOx_PWR_MASK
};

struct codec_dai_private {
	atomic_t substream_count;
} privates[] = {
	{ATOMIC_INIT(0)},
	{ATOMIC_INIT(0)},
};

/* TODO: Add a clock use count.
   Turn off the clock is there is no audio activity in the system.
 */



// Controls -----------------------------------------------------------------------------------------------------------------------

struct ab3550_control {
	const char *name;
	u8 value;
	enum enum_register reg_idx;
	u8 reg_mask;
	u8 reg_shift;
	bool is_enum;
	bool changed;
};

struct ab3550_control ab3550_ctl[] = {
//    name						value	reg_idx					reg_mask				reg_shift			is_enum		changed
	{"RX2 Select",				0,		IDX_RX2,				RX2_IF_SELECT_MASK,		RX2_IF_SELECT_SHIFT,true,		false},
	{"DAC1 Routing",			0x05,	IDX_RX1,				DACx_PWR_MASK,			DACx_PWR_SHIFT,		false,		false}, // DAC1 -> AUXO1, SPKR
	{"DAC2 Routing",			0x06,	IDX_RX2,				DACx_PWR_MASK,			DACx_PWR_SHIFT,		false,		false}, // DAC2 -> AUXO1, SPKR
	{"DAC3 Routing",			0x00,	IDX_RX3,				DACx_PWR_MASK,			DACx_PWR_SHIFT,		false,		false},
	{"MIC1 Input Select",		0x30,	IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"MIC2 Input Select",		0x00,	IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"I2S0 Input Select",		0x11,	IDX_TX1,				0x00,					0x00,				true,		false},
	{"I2S1 Input Select",		0x22,	IDX_TX2,				0x00,					0x00,				true,		false},
	{"APGA1 Source",			0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"APGA2 Source",			0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"APGA1 Destination",		0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"APGA2 Destination",		0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"DAC1 Side Tone",			0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"DAC2 Side Tone",			0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"RX-DPGA1 Gain",			0x3A,	IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"RX-DPGA2 Gain",			0x3A,	IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"RX-DPGA3 Gain",			0x00,	IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"LINE1 Gain",				0x4,	IDX_UNKNOWN,			0x00,					0x00,				false,		false}, // -6 dB
	{"LINE2 Gain",				0x4,	IDX_UNKNOWN,			0x00,					0x00,				false,		false}, // -6 dB
	{"SPKR Gain",				0x4,	IDX_UNKNOWN,			0x00,					0x00,				false,		false}, // -6 dB
	{"EAR Gain",				0x4,	IDX_UNKNOWN,			0x00,					0x00,				false,		false}, // -6 dB
	{"AUXO1 Gain",				0x4,	IDX_UNKNOWN,			0x00,					0x00,				false,		false}, // -6 dB
	{"AUXO2 Gain",				0x4,	IDX_UNKNOWN,			0x00,					0x00,				false,		false}, // -6 dB
	{"MIC1 Gain",				0x2,	IDX_MIC1_GAIN,			MICx_GAIN_MASK,			MICx_GAIN_SHIFT,	false,		false},
	{"MIC2 Gain",				0x2,	IDX_MIC2_GAIN,			MICx_GAIN_MASK,			MICx_GAIN_SHIFT,	false,		false},
	{"TX-DPGA1 Gain",			0x06,	IDX_TX_DIGITAL_PGA1,	TXDPGAx_MASK,			TXDPGAx_SHIFT,		false,		false},
	{"TX-DPGA2 Gain",			0x06,	IDX_TX_DIGITAL_PGA2,	TXDPGAx_MASK,			TXDPGAx_SHIFT,		false,		false},
	{"ST-PGA1 Gain",			0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"ST-PGA2 Gain",			0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"APGA1 Gain",				0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"APGA2 Gain",				0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"DAC1 Power Mode",			0x0,	IDX_UNKNOWN,			0x00,					0x00,				true,		false}, // 100%
	{"DAC2 Power Mode",			0x0,	IDX_UNKNOWN,			0x00,					0x00,				true,		false}, // 100%
	{"DAC3 Power Mode",			0x3,	IDX_UNKNOWN,			0x00,					0x00,				true,		false}, // Do not use
	{"EAR Power Mode",			0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"AUXO Power Mode",			0x4,	IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"LINE1 Inverse",			0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"LINE2 Inverse",			0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"AUXO1 Inverse",			0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"AUXO2 Inverse",			0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"AUXO1 Pulldown",			0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"AUXO2 Pulldown",			0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"VMID1",					0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"VMID2",					0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"MIC1 MBias",				0x01,	IDX_UNKNOWN,			0x00,					0x00,				true,		false}, // MBias 1 On
	{"MIC2 MBias",				0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"MBIAS1 HiZ Option",		0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"MBIAS2 HiZ Option",		0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"MBIAS2 Output Voltage",	0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"MBIAS2 Internal Resistor",0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"MIC1 Input Impedance",	0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"MIC2 Input Impedance",	0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"TX1 HP Filter",			0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"TX2 HP Filter",			0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"LINEIN1 Pre-charge",		0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"LINEIN2 Pre-charge",		0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"MIC1P1 Pre-charge",		0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"MIC1P2 Pre-charge",		0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"MIC1N1 Pre-charge",		0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"MIC1N2 Pre-charge",		0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"MIC2P1 Pre-charge",		0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"MIC2P2 Pre-charge",		0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"MIC2N1 Pre-charge",		0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"MIC2N2 Pre-charge",		0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"ST1 HP Filter",			0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"ST2 HP Filter",			0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"I2S0 Word Length",		0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"I2S1 Word Length",		0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"I2S0 Mode",				0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"I2S1 Mode",				0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"I2S0 Tri-state",			0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"I2S1 Tri-state",			0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"I2S0 Pulldown",			0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"I2S1 Pulldown",			0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"I2S0 Sample Rate",		0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"I2S1 Sample Rate",		0,		IDX_UNKNOWN,			0x00,					0x00,				true,		false},
	{"Interface Loop",			0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"Interface Swap",			0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"Voice Call",				0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false},
	{"Commit",					0,		IDX_UNKNOWN,			0x00,					0x00,				false,		false}
};

/* control No. 0 correpsonds to bit No. 0 in this array.
   If a control value has been changed, but not commited
   to the AB3550. Its corresponding bit is set.
 */
//static unsigned long changed_controls[BIT_WORD(ARRAY_SIZE(ab3550_ctl)) + 1];

static const char *enum_rx2_select[] = {"I2S0", "I2S1"};
static const char *enum_i2s_input_select[] = {"tri-state", "MIC1", "MIC2", "mute"};
static const char *enum_apga1_source[] = {"None", "LINEIN1", "MIC1", "MIC2"};
static const char *enum_apga2_source[] = {"None", "LINEIN2", "MIC1", "MIC2"};
static const char *enum_dac_side_tone[] = {"None", "TX1", "TX2"};
static const char *enum_dac_power_mode[] = {"100%", "75%", "50%"};
static const char *enum_ear_power_mode[] = {"100%", "75%"};
static const char *enum_auxo_power_mode[] = {"100%", "75%", "50%", "25%"};
static const char *enum_mbias[] = {"Off", "On"};
static const char *enum_mbias_hiz_option[] = {"GND", "HiZ"};
static const char *enum_mbias2_output_voltage[] = {"2.0v", "2.2v"};
static const char *enum_mbias2_internal_resistor[] = {"connected", "bypassed"};
static const char *enum_mic_input_impedance[] = {"12.5 kohm", "25 kohm", "50 kohm"};
static const char *enum_hp_filter[] = {"HP3", "HP1", "bypass"};
static const char *enum_i2s_word_length[] = {"16 bits", "24 bits"};
static const char *enum_i2s_mode[] = {"Master Mode", "Slave Mode"};
static const char *enum_i2s_tristate[] = {"Normal", "Tri-state"};
static const char *enum_i2s_pulldown[] = {"disconnected", "connected"};
static const char *enum_i2s_sample_rate[] = {"8 kHz", "16 kHz", "44.1 kHz", "48 kHz"};

static struct soc_enum soc_enum_rx2_select = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_rx2_select), enum_rx2_select); // RX2 Select
static struct soc_enum soc_enum_i2s0_input_select = SOC_ENUM_DOUBLE(0, 0, 4, ARRAY_SIZE(enum_i2s_input_select), enum_i2s_input_select); // I2S0 Input Select
static struct soc_enum soc_enum_i2s1_input_select = SOC_ENUM_DOUBLE(0, 0, 4, ARRAY_SIZE(enum_i2s_input_select), enum_i2s_input_select); // I2S1 Input Select
static struct soc_enum soc_enum_apga1_source = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_apga1_source), enum_apga1_source); // APGA1 Source
static struct soc_enum soc_enum_apga2_source = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_apga2_source), enum_apga2_source); // APGA2 Source
static struct soc_enum soc_enum_dac1_side_tone = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_dac_side_tone), enum_dac_side_tone); // DAC1 Side Tone
static struct soc_enum soc_enum_dac2_side_tone = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_dac_side_tone), enum_dac_side_tone); // DAC2 Side Tone
static struct soc_enum soc_enum_dac1_power_mode = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_dac_power_mode), enum_dac_power_mode); // DAC1 Power Mode
static struct soc_enum soc_enum_dac2_power_mode = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_dac_power_mode), enum_dac_power_mode); // DAC2 Power Mode
static struct soc_enum soc_enum_dac3_power_mode = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_dac_power_mode), enum_dac_power_mode); // DAC3 Power Mode
static struct soc_enum soc_enum_ear_power_mode = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_ear_power_mode), enum_ear_power_mode); // EAR Power Mode
static struct soc_enum soc_enum_auxo_power_mode = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_auxo_power_mode), enum_auxo_power_mode); // AUXO Power Mode
static struct soc_enum soc_enum_mic1_mbias = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mbias), enum_mbias); // MIC Bias Connection
static struct soc_enum soc_enum_mic2_mbias = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mbias), enum_mbias); // MIC Bias Connection
static struct soc_enum soc_enum_mbias1_hiz_option = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mbias_hiz_option), enum_mbias_hiz_option); // MBIAS1 HiZ Option
static struct soc_enum soc_enum_mbias2_hiz_option = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mbias_hiz_option), enum_mbias_hiz_option); // MBIAS1 HiZ Option
static struct soc_enum soc_enum_mbias2_output_voltage = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mbias2_output_voltage), enum_mbias2_output_voltage); // MBIAS2 Output voltage
static struct soc_enum soc_enum_mbias2_internal_resistor = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mbias2_internal_resistor), enum_mbias2_internal_resistor); //
static struct soc_enum soc_enum_mic1_input_impedance = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mic_input_impedance), enum_mic_input_impedance); //
static struct soc_enum soc_enum_mic2_input_impedance = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mic_input_impedance), enum_mic_input_impedance); //
static struct soc_enum soc_enum_tx1_hp_filter = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_hp_filter), enum_hp_filter); //
static struct soc_enum soc_enum_tx2_hp_filter = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_hp_filter), enum_hp_filter); //
static struct soc_enum soc_enum_st1_hp_filter = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_hp_filter), enum_hp_filter); //
static struct soc_enum soc_enum_st2_hp_filter = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_hp_filter), enum_hp_filter); //
static struct soc_enum soc_enum_i2s0_word_length = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_word_length), enum_i2s_word_length); //
static struct soc_enum soc_enum_i2s1_word_length = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_word_length), enum_i2s_word_length); //
static struct soc_enum soc_enum_i2s0_mode = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_mode), enum_i2s_mode); //
static struct soc_enum soc_enum_i2s1_mode = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_mode), enum_i2s_mode); //
static struct soc_enum soc_enum_i2s0_tristate = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_tristate), enum_i2s_tristate); //
static struct soc_enum soc_enum_i2s1_tristate = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_tristate), enum_i2s_tristate); //
static struct soc_enum soc_enum_i2s0_pulldown = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_pulldown), enum_i2s_pulldown); //
static struct soc_enum soc_enum_i2s1_pulldown = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_pulldown), enum_i2s_pulldown); //
static struct soc_enum soc_enum_i2s0_sample_rate = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_sample_rate), enum_i2s_sample_rate); //
static struct soc_enum soc_enum_i2s1_sample_rate = SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_sample_rate), enum_i2s_sample_rate); //

static struct snd_kcontrol_new ab3550_snd_controls[] = {
	/* RX Routing */
	SOC_ENUM(	"RX2 Select",			soc_enum_rx2_select),
	SOC_SINGLE(	"DAC1 Routing",			0, 0, 0x3f, 0),
	SOC_SINGLE(	"DAC2 Routing",			0, 0, 0x3f, 0),
	SOC_SINGLE(	"DAC3 Routing",			0, 0, 0x3f, 0),
	/* TX Routing */
	SOC_SINGLE(	"MIC1 Input Select",	0, 0, 0xff, 0),
	SOC_SINGLE(	"MIC2 Input Select",	0, 0, 0x3f, 0),
	SOC_ENUM(	"I2S0 Input Select",	soc_enum_i2s0_input_select),
	SOC_ENUM(	"I2S1 Input Select",	soc_enum_i2s1_input_select),
	/* Routing of Side Tone and Analop Loop */
	SOC_ENUM(	"APGA1 Source",			soc_enum_apga1_source),
	SOC_ENUM(	"APGA2 Source",			soc_enum_apga2_source),
	SOC_SINGLE(	"APGA1 Destination",	0, 0, 0x3f, 0),
	SOC_SINGLE(	"APGA2 Destination",	0, 0, 0x3f, 0),
	SOC_ENUM(	"DAC1 Side Tone",		soc_enum_dac1_side_tone),
	SOC_ENUM(	"DAC2 Side Tone",		soc_enum_dac2_side_tone),
	/* RX Volume Control */
	SOC_SINGLE(	"RX-DPGA1 Gain",		0, 0, 66, 0),
	SOC_SINGLE(	"RX-DPGA2 Gain",		0, 0, 66, 0),
	SOC_SINGLE(	"RX-DPGA3 Gain",		0, 0, 66, 0),
	SOC_SINGLE(	"LINE1 Gain",			0, 0, 10, 0),
	SOC_SINGLE(	"LINE2 Gain",			0, 0, 10, 0),
	SOC_SINGLE(	"SPKR Gain",			0, 0, 22, 0),
	SOC_SINGLE(	"EAR Gain",				0, 0, 14, 0),
	SOC_SINGLE(	"AUXO1 Gain",			0, 0, 12, 0),
	SOC_SINGLE(	"AUXO2 Gain",			0, 0, 12, 0),
	/* TX Volume Control */
	SOC_SINGLE(	"MIC1 Gain",			0, 0, 10, 0),
	SOC_SINGLE(	"MIC2 Gain",			0, 0, 10, 0),
	SOC_SINGLE(	"TX-DPGA1 Gain",		0, 0, 15, 0),
	SOC_SINGLE(	"TX-DPGA2 Gain",		0, 0, 15, 0),
	/* Volume Control of Side Tone and Analog Loop */
	SOC_SINGLE(	"ST-PGA1 Gain",			0, 0, 10, 0),
	SOC_SINGLE(	"ST-PGA2 Gain",			0, 0, 10, 0),
	SOC_SINGLE(	"APGA1 Gain",			0, 0, 28, 0),
	SOC_SINGLE(	"APGA2 Gain",			0, 0, 28, 0),
	/* RX Properties */
	SOC_ENUM(	"DAC1 Power Mode",		soc_enum_dac1_power_mode),
	SOC_ENUM(	"DAC2 Power Mode",		soc_enum_dac2_power_mode),
	SOC_ENUM(	"DAC3 Power Mode",		soc_enum_dac3_power_mode),
	SOC_ENUM(	"EAR Power Mode",		soc_enum_ear_power_mode),
	SOC_ENUM(	"AUXO Power Mode",		soc_enum_auxo_power_mode),
	SOC_SINGLE(	"LINE1 Inverse",		0, 0, 1, 0),
	SOC_SINGLE(	"LINE2 Inverse",		0, 0, 1, 0),
	SOC_SINGLE(	"AUXO1 Inverse",		0, 0, 1, 0),
	SOC_SINGLE(	"AUXO2 Inverse",		0, 0, 1, 0),
	SOC_SINGLE(	"AUXO1 Pulldown",		0, 0, 1, 0),
	SOC_SINGLE(	"AUXO2 Pulldown",		0, 0, 1, 0),
	/* TX Properties */
	SOC_SINGLE(	"VMID1",				0, 0, 0xff, 0),
	SOC_SINGLE(	"VMID2",				0, 0, 0xff, 0),
	SOC_ENUM(	"MIC1 MBias",			soc_enum_mic1_mbias),
	SOC_ENUM(	"MIC2 MBias",			soc_enum_mic2_mbias),
	SOC_ENUM(	"MBIAS1 HiZ Option",	soc_enum_mbias1_hiz_option),
	SOC_ENUM(	"MBIAS2 HiZ Option",	soc_enum_mbias2_hiz_option),
	SOC_ENUM(	"MBIAS2 Output Voltage",	soc_enum_mbias2_output_voltage),
	SOC_ENUM(	"MBIAS2 Internal Resistor", soc_enum_mbias2_internal_resistor),
	SOC_ENUM(	"MIC1 Input Impedance",		soc_enum_mic1_input_impedance),
	SOC_ENUM(	"MIC2 Input Impedance",		soc_enum_mic2_input_impedance),
	SOC_ENUM(	"TX1 HP Filter",		soc_enum_tx1_hp_filter),
	SOC_ENUM(	"TX2 HP Filter",		soc_enum_tx2_hp_filter),
	SOC_SINGLE(	"LINEIN1 Pre-charge",	0, 0, 100, 0),
	SOC_SINGLE(	"LINEIN2 Pre-charge",	0, 0, 100, 0),
	SOC_SINGLE(	"MIC1P1 Pre-charge",	0, 0, 100, 0),
	SOC_SINGLE(	"MIC1P2 Pre-charge",	0, 0, 100, 0),
	SOC_SINGLE(	"MIC1N1 Pre-charge",	0, 0, 100, 0),
	SOC_SINGLE(	"MIC1N2 Pre-charge",	0, 0, 100, 0),
	SOC_SINGLE(	"MIC2P1 Pre-charge",	0, 0, 100, 0),
	SOC_SINGLE(	"MIC2P2 Pre-charge",	0, 0, 100, 0),
	SOC_SINGLE(	"MIC2N1 Pre-charge",	0, 0, 100, 0),
	SOC_SINGLE(	"MIC2N2 Pre-charge",	0, 0, 100, 0),
	/* Side Tone and Analog Loop Properties */
	SOC_ENUM(	"ST1 HP Filter",		soc_enum_st1_hp_filter),
	SOC_ENUM(	"ST2 HP Filter",		soc_enum_st2_hp_filter),
	/* I2S Interface Properties */
	SOC_ENUM(	"I2S0 Word Length",		soc_enum_i2s0_word_length),
	SOC_ENUM(	"I2S1 Word Length",		soc_enum_i2s1_word_length),
	SOC_ENUM(	"I2S0 Mode",			soc_enum_i2s0_mode),
	SOC_ENUM(	"I2S1 Mode",			soc_enum_i2s1_mode),
	SOC_ENUM(	"I2S0 tri-state",		soc_enum_i2s0_tristate),
	SOC_ENUM(	"I2S1 tri-state",		soc_enum_i2s1_tristate),
	SOC_ENUM(	"I2S0 pulldown",		soc_enum_i2s0_pulldown),
	SOC_ENUM(	"I2S1 pulldown",		soc_enum_i2s1_pulldown),
	SOC_ENUM(	"I2S0 Sample Rate",		soc_enum_i2s0_sample_rate),
	SOC_ENUM(	"I2S1 Sample Rate",		soc_enum_i2s1_sample_rate),
	SOC_SINGLE(	"Interface Loop",		0, 0, 0x0f, 0),
	SOC_SINGLE(	"Interface Swap",		0, 0, 0x1f, 0),
	/* Miscellaneous */
	SOC_SINGLE(	"Voice Call",			0, 0, 1, 0),
	SOC_SINGLE(	"Commit",				0, 0, 1, 0)
};

static const struct snd_soc_dapm_widget ab3550_dapm_widgets[] = {
};

static const struct snd_soc_dapm_route intercon[] = {
};

static unsigned int ab3550_get_control_value(struct snd_soc_codec *codec, unsigned int ctl)
{
	if (ctl >= ARRAY_SIZE(ab3550_ctl))
		return -EINVAL;
	//printk(KERN_DEBUG "%s: returning value of control %s: 0x%02x.\n",
	//      __func__, ab3550_ctl[ctl].name, ab3550_ctl[ctl].value);
	return ab3550_ctl[ctl].value;
}

static unsigned int get_control_index(const char *name)
{
	int i, n;
	for(i = 0, n = ARRAY_SIZE(ab3550_ctl); i < n; i++) {
		if (strcmp(ab3550_ctl[i].name, name) == 0)
			break;
	}
	return i;
}

static unsigned int get_control_value_by_name(const char *name)
{
	int i = get_control_index(name);
	return ab3550_ctl[i].value;
}

static unsigned int get_control_changed_by_name(const char *name)
{
	int i = get_control_index(name);
	return ab3550_ctl[i].changed;
}

static int gain_ramp(u8 reg, u8 target, int shift, u8 mask)
{
//	u8 curr;
	int ret = 0;

//	printk(KERN_DEBUG "%s invoked.\n", __func__);
//	if ((ret = GET_REG(reg, &curr))) {
//		return ret;
//	}
//	if (curr == target)
//		return 0;
//	step = curr < target ? 1 : -1;
//	while (!ret && curr != target) {
/*		At least 15ms are required, so I take 16ms :-) */
//		msleep(1);
//		ret = MASK_SET_REG(reg, mask, (curr += step) << shift);
//	}
	ret = MASK_SET_REG(reg, mask, target << shift);
	return ret;
}

static void ab3550_print_registers(int stream_dir)
{
	int i;
	u8 val;
	for (i = 0; i < ARRAY_SIZE(ab3550_registers); i++) {
		GET_REG(ab3550_registers[i].address, &val);
		printk(KERN_DEBUG "REGISTERS: %s = 0x%02x\n", ab3550_registers[i].name, val);
	}
}

static void ab3550_toggle_output_amp(int idx_amp, u8 val) {
	struct ab3550_register* reg = &(ab3550_registers[(int)outamp_indices[idx_amp]]);
	MASK_SET_REG(reg->address,
				 outamp_pwr_mask[idx_amp],
				 val << outamp_pwr_shift[idx_amp]);

	printk(KERN_DEBUG "DORIANO: %s turned %s.\n", reg->name, val == 0 ? "off" : "on");
}

static void ab3550_toggle_mic_amp(int idx, u8 val) {
	enum enum_register idx_reg_mic_amp[] = {IDX_MIC1_GAIN, IDX_MIC2_GAIN}; // PWR is in located in the gain regs.
	struct ab3550_register* reg = &(ab3550_registers[(int)idx_reg_mic_amp[idx]]);
	MASK_SET_REG(reg->address,
				 MICx_PWR_MASK,
				 val << MICx_PWR_SHIFT);

	printk(KERN_DEBUG "DORIANO: %s turned %s.\n", reg->name, val == 0 ? "off" : "on");
}

static void ab3550_toggle_DAC(int idx_DAC, u8 val)
{
	enum enum_register idx_reg_DAC[] = {IDX_RX1, IDX_RX2, IDX_RX3};
	struct ab3550_register* reg = &(ab3550_registers[(int)idx_reg_DAC[idx_DAC]]);
	MASK_SET_REG(reg->address,
				 DACx_PWR_MASK | RXx_PWR_MASK,
				 (val << RXx_PWR_SHIFT | val << DACx_PWR_SHIFT));

	printk(KERN_DEBUG "DORIANO: DAC%d turned %s.\n", idx_DAC + 1, val == 0 ? "off" : "on");
}

static void	ab3550_toggle_output_adder(int idx_adder, u8 val)
{
	int i;
	struct ab3550_register* reg = &(ab3550_registers[(int)outamp_adder_indices[idx_adder]]);

	MASK_SET_REG(reg->address,
				 DAC1_TO_ADDER_MASK | DAC2_TO_ADDER_MASK | DAC3_TO_ADDER_MASK,
				 val);

	for (i = 0; i < 3; i++)
		printk(KERN_DEBUG "DORIANO: DAC%d -> %s: %s.\n", i + 1, reg->name, (val & 1 << i) ? "on" : "off");
}

static void ab3550_toggle_ADC(int idx_ADC, u8 val)
{
	enum enum_register idx_reg_ADC[] = {IDX_TX1, IDX_TX2};
	struct ab3550_register* reg = &(ab3550_registers[(int)idx_reg_ADC[idx_ADC]]);
	MASK_SET_REG(reg->address,
				 ADCx_PWR_MASK | TXx_PWR_MASK,
				 ( val << ADCx_PWR_SHIFT | val << TXx_PWR_SHIFT));

	printk(KERN_DEBUG "DORIANO: ADC%d turned %s.\n", idx_ADC + 1, val == 0 ? "off" : "on");
}

static void ab3550_set_ADC_filter(int idx_ADC, u8 val)
{
	enum enum_register idx_reg_ADC[] = {IDX_TX1, IDX_TX2};
	struct ab3550_register* reg = &(ab3550_registers[(int)idx_reg_ADC[idx_ADC]]);
	MASK_SET_REG(reg->address,
				 TXx_HP_FILTER_MASK,
				 val << TXx_HP_FILTER_SHIFT);

	printk(KERN_DEBUG "DORIANO: ADC%d filter set to %s.\n", idx_ADC + 1, enum_hp_filter[val]);
}

static int ab3550_add_widgets(struct snd_soc_codec *codec)
{
	snd_soc_dapm_new_controls(codec, ab3550_dapm_widgets,
				  ARRAY_SIZE(ab3550_dapm_widgets));

	snd_soc_dapm_add_routes(codec, intercon, ARRAY_SIZE(intercon));

	snd_soc_dapm_new_widgets(codec);
	return 0;
}



static void ab3550_init_registers_startup(int ifSel)
{
	u8 val;
	u8 iface = (ifSel == 0) ? INTERFACE0 : INTERFACE1;

	// Configure CLOCK register
	val = 1 << CLOCK_REF_SELECT_SHIFT |	// 32kHz RTC
		  1 << CLOCK_ENABLE_SHIFT; // Enable clock
	SET_REG(CLOCK, val);

	// Enable Master Generator I2Sx
	MASK_SET_REG(iface, MASTER_GENx_PWR_MASK | I2Sx_TRISTATE_MASK | I2Sx_PULLDOWN_MASK, 1 << MASTER_GENx_PWR_SHIFT);

	// Configure INTERFACEx_DATA register
	val = 1 << I2Sx_L_DATA_SHIFT |// TX1 to I2Sx_L
		  1 << I2Sx_R_DATA_SHIFT; // TX1 to I2Sx_R
	SET_REG(ifSel == 0 ? INTERFACE0_DATA : INTERFACE1_DATA, val);

	GET_REG(CLOCK, &val);
	printk(KERN_DEBUG "AB: CLOCK = 0x%02x\n", val);
	GET_REG(iface, &val);
	printk(KERN_DEBUG "AB: INTERFACE%d = 0x%02x\n", ifSel, val);
	GET_REG(ifSel == 0 ? INTERFACE0_DATA : INTERFACE1_DATA, &val);
	printk(KERN_DEBUG "AB: INTERFACE%d_DATA = 0x%02x\n", ifSel, val);
}

static void ab3550_init_registers_playback(int ifSel) {

	int i, routing_left, routing_right, idx_RX_left, idx_RX_right;
	u8 val;

	// Get indices to active DACs
	idx_RX_left = idx_RX_right = -1;
	val = get_control_value_by_name("RX2 Select");
	if (ifSel == 0) {
		if (!(val & RX2_IF_SELECT_MASK))
			idx_RX_right = 1;
		idx_RX_left = 0;
	} else {
		if (val & RX2_IF_SELECT_MASK)
			idx_RX_right = 1;
		idx_RX_left = 2;
	}
printk(KERN_DEBUG "DORIANO: idx_RX_left = %d, idx_RX_right = %d\n", idx_RX_left, idx_RX_right);

	// Toggle DACs
	routing_left = ab3550_ctl[IDX_DAC1_Routing].value;
printk(KERN_DEBUG "DORIANO: routing_left = %d\n", routing_left);
	if (routing_left)
		ab3550_toggle_DAC(idx_RX_left, 1);
	routing_right = 0;
	if (idx_RX_right > -1)
	{
		routing_right = ab3550_ctl[IDX_DAC2_Routing].value;
printk(KERN_DEBUG "DORIANO: routing_right = %d\n", routing_right);
		if (routing_right)
			ab3550_toggle_DAC(idx_RX_right, 1);
	}

	// Toggle adders
	for (i = 0; i < ARRAY_SIZE(outamp_indices); i++) {
		val = 0;
		val |= (routing_left & 1 << i) ? 1 << idx_RX_left : 0;
		if (idx_RX_right > -1)
			val |= (routing_right & 1 << i) ? 1 << idx_RX_right : 0;
		ab3550_toggle_output_adder(i, val);
	}

	/* TODO: DC cancellation */

	// Toggle amplifiers
	val = routing_left | routing_right;
	for (i = 0; i < ARRAY_SIZE(outamp_indices); i++) {
printk(KERN_DEBUG "DORIANO: val & 1 << i = %d\n", val & 1 << i);
		if (val & 1 << i)
			ab3550_toggle_output_amp(i, 1);
	}
}

static void ab3550_init_registers_capture(int ifSel)
{
	enum enum_control idx = (ifSel == 0) ? IDX_I2S0_Input_Select : IDX_I2S1_Input_Select;

printk(KERN_DEBUG "DORIANO: ab3550_ctl[idx].value = 0x%02x\n", ab3550_ctl[idx].value);

	// Toggle ADC (RXx) and amplifiers (MICx_GAIN)
	if ((ab3550_ctl[idx].value & 0x0F) < 3) { // value = 3 => mute
		int idx_ADC = (ab3550_ctl[idx].value & 0x0F) - 1; // value = 0xX1 => MIC1, value = 0xX2 => MIC2
		ab3550_toggle_ADC(idx_ADC, 1);
		ab3550_toggle_mic_amp(idx_ADC, 1);
	}
}



// Commit ---------------------------------------------------------------------------------------------------------------

static void printCommit(int idx) {
	printk(KERN_DEBUG "%s: %s committed to 0x%02x.\n", __func__, ab3550_ctl[idx].name, ab3550_ctl[idx].value);
}

static bool commit_if_changed(enum enum_control idx_ctl)
{
	bool changed = false;

	if (ab3550_ctl[idx_ctl].changed) {
		MASK_SET_REG(ab3550_registers[ab3550_ctl[idx_ctl].reg_idx].address,
					 ab3550_ctl[idx_ctl].reg_mask,
					 ab3550_ctl[idx_ctl].value << ab3550_ctl[idx_ctl].reg_shift);
		ab3550_ctl[idx_ctl].changed = false;
		changed = true;

		printCommit(idx_ctl);
	}

	return changed;
}

static int commit_outamps_routing(void)
{
	static const char *control_names[] = {
		"DAC1 Routing", "DAC2 Routing", "DAC3 Routing",
		"APGA1 Destination", "APGA2 Destination"
	};

	static const u8 outamp_0db_gain[] = {
		0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C
	};

	struct ab3550_register* reg;

	printk(KERN_DEBUG "%s: Enter.\n", __func__);

	if ((get_control_changed_by_name("DAC1 Routing")) ||
		(get_control_changed_by_name("DAC2 Routing")) ||
		(get_control_changed_by_name("DAC3 Routing")) ||
		(get_control_changed_by_name("APGA1 Destination")) ||
		(get_control_changed_by_name("APGA2 Destination"))) {

		int i;

		for (i = 0; i < ARRAY_SIZE(outamp_indices); i++) {
			unsigned int connect = 0;
			int j;
			for (j = 0; j < ARRAY_SIZE(control_names); j++) {
				if (get_control_value_by_name(control_names[j])
					& 1 << i) {
					connect |= 1 << j;
				}
			}
			/* Ramp down the amplifier if
			   nothing is to be disconnected to it */
			if (!connect) {
				u8 val = 0;
				reg = &(ab3550_registers[(int)outamp_indices[i]]);
				GET_REG(reg->address, &val);
				if (! (val & 1 << outamp_pwr_shift[i]))
					continue;
				gain_ramp(reg->address, 0,
					  outamp_gain_shift[i],
					  outamp_gain_mask[i]);
				MASK_SET_REG(reg->address,
					     outamp_pwr_mask[i], 0);
				continue;
			}
			reg = &(ab3550_registers[(int)outamp_adder_indices[i]]);
			MASK_SET_REG(reg->address,
				     DAC3_TO_ADDER_MASK |
				     DAC2_TO_ADDER_MASK |
				     DAC3_TO_ADDER_MASK,
				     connect);
			if (connect & 1 << 3) {
				/* Connect APGA1 */
				MASK_SET_REG(APGA1_ADDER,
					     1 << (ARRAY_SIZE(outamp_indices) - i),
					     1 << (ARRAY_SIZE(outamp_indices) - i));
			}
			if (connect & 1 << 4) {
				/* Connect APGA2 */
				MASK_SET_REG(APGA2_ADDER,
					     1 << (ARRAY_SIZE(outamp_indices) - i),
					     1 << (ARRAY_SIZE(outamp_indices) - i));
			}
			reg = &(ab3550_registers[(int)outamp_indices[i]]);
			MASK_SET_REG(reg->address,
				     outamp_pwr_mask[i],
				     1 << outamp_pwr_shift[i]);
			gain_ramp(reg->address, outamp_0db_gain[i],
				  outamp_gain_shift[i],
				  outamp_gain_mask[i]);
		}

		for (i = 0; i < ARRAY_SIZE(control_names); i++)
			ab3550_ctl[get_control_index(control_names[i])].changed = 0;
	}
	return 0;
}

static int commit_rx_routing(void)
{
	int idx;

	printk(KERN_DEBUG "%s: Enter.\n", __func__);

	idx = get_control_index("RX2 Select");
	if (ab3550_ctl[idx].changed) {
		u8 val = ab3550_ctl[idx].value;
		if (val & RXx_PWR_MASK)
			MASK_SET_REG(RX2, RXx_PWR_MASK, 0);
		MASK_SET_REG(RX2, RX2_IF_SELECT_MASK, ab3550_ctl[idx].value);
		if (val & RXx_PWR_MASK)
			MASK_SET_REG(RX2, RXx_PWR_MASK, 1 << RXx_PWR_SHIFT);
		ab3550_ctl[idx].changed = false;

		printCommit(idx);
	}

	commit_outamps_routing();

	return 0;
}

static int commit_rx_gain(void)
{
	int i, idx_ctl;
	//u8 val;
	struct ab3550_register* reg;

	printk(KERN_DEBUG "%s: Enter.\n", __func__);

	for (i = 0; i < ARRAY_SIZE(outamp_gain_controls); i++) {
		idx_ctl = (int)outamp_gain_controls[i];
		reg = &(ab3550_registers[ab3550_ctl[idx_ctl].reg_idx]);
		if (ab3550_ctl[idx_ctl].changed) {
			//val = GET_REG(reg->address, &val);
		//	if (ab3550_ctl[idx_ctl].value & 1 << outamp_pwr_shift[i]) // Ramping is needed if amp is powered up
		//		gain_ramp(outamp_reg[i], ab3550_ctl[idx_ctl].value, outamp_gain_shift[i], outamp_gain_mask[i]);
		//	else // Set directly if amp is not powered up
			MASK_SET_REG(reg->address, outamp_gain_mask[i], ab3550_ctl[idx_ctl].value);
			ab3550_ctl[idx_ctl].changed = false;

			printCommit(idx_ctl);
		}
	}

	idx_ctl = get_control_index("RX-DPGA1 Gain");
	if (ab3550_ctl[idx_ctl].changed) {
		gain_ramp(RX1_DIGITAL_PGA, ab3550_ctl[idx_ctl].value, RXx_PGA_GAIN_SHIFT, RXx_PGA_GAIN_MASK);
		ab3550_ctl[idx_ctl].changed = false;

		printCommit(idx_ctl);

	}

	idx_ctl = get_control_index("RX-DPGA2 Gain");
	if (ab3550_ctl[idx_ctl].changed) {
		gain_ramp(RX2_DIGITAL_PGA, ab3550_ctl[idx_ctl].value, RXx_PGA_GAIN_SHIFT, RXx_PGA_GAIN_MASK);
		ab3550_ctl[idx_ctl].changed = false;

		printCommit(idx_ctl);
	}

	idx_ctl = get_control_index("RX-DPGA3 Gain");
	if (ab3550_ctl[idx_ctl].changed) {
		gain_ramp(RX3_DIGITAL_PGA, ab3550_ctl[idx_ctl].value, RXx_PGA_GAIN_SHIFT, RXx_PGA_GAIN_MASK);
		ab3550_ctl[idx_ctl].changed = false;

		printCommit(idx_ctl);
	}

	return 0;
}

static int commit_tx_routing(void)
{
	int idx;

	printk(KERN_DEBUG "%s: Enter.\n", __func__);

	idx = get_control_index("MIC1 Input Select");
	if (ab3550_ctl[idx].changed) {
		MASK_SET_REG(MIC1_INPUT_SELECT,
		             MICxP1_SEL_MASK | MICxN1_SEL_MASK | MICxP2_SEL_MASK | MICxN2_SEL_MASK | LINEIN_SEL_MASK,
				     ab3550_ctl[idx].value);
		MASK_SET_REG(MIC2_TO_MIC1,
		             MIC2P1_MIC1P_SEL_MASK | MIC2N1_MIC1N_SEL_MASK,
				     ab3550_ctl[idx].value >> 2);
		ab3550_ctl[idx].changed = false;

		printCommit(idx);
	}

	idx = get_control_index("MIC2 Input Select");
	if (ab3550_ctl[idx].changed) {
		MASK_SET_REG(MIC2_INPUT_SELECT,
				     MICxP1_SEL_MASK | MICxN1_SEL_MASK | MICxP2_SEL_MASK | MICxN2_SEL_MASK | LINEIN_SEL_MASK,
				     ab3550_ctl[idx].value);
		ab3550_ctl[idx].changed = false;

		printCommit(idx);
	}

	idx = get_control_index("I2S0 Input Select");
	if (ab3550_ctl[idx].changed) {
		MASK_SET_REG(INTERFACE0_DATA,
					 I2Sx_L_DATA_MASK,
				     ab3550_ctl[idx].value << I2Sx_L_DATA_SHIFT);
		MASK_SET_REG(INTERFACE0_DATA,
					 I2Sx_R_DATA_MASK,
				     ab3550_ctl[idx].value << I2Sx_R_DATA_SHIFT);
		ab3550_ctl[idx].changed = false;

		printCommit(idx);
	}

	idx = get_control_index("I2S1 Input Select");
	if (ab3550_ctl[idx].changed) {
		MASK_SET_REG(INTERFACE1_DATA,
					 I2Sx_L_DATA_MASK,
					 ab3550_ctl[idx].value << I2Sx_L_DATA_SHIFT);
		MASK_SET_REG(INTERFACE1_DATA,
					 I2Sx_R_DATA_MASK,
					 ab3550_ctl[idx].value << I2Sx_R_DATA_SHIFT);
		ab3550_ctl[idx].changed = false;

		printCommit(idx);
	}

	return 0;
}

static int commit_tx_gain(void)
{
	printk(KERN_DEBUG "%s: Enter.\n", __func__);

	commit_if_changed(IDX_MIC1_Gain);
	commit_if_changed(IDX_MIC2_Gain);
	commit_if_changed(IDX_TX_DPGA1_Gain);
	commit_if_changed(IDX_TX_DPGA2_Gain);

	return 0;
}

static int commit_others(void)
{
	int idx;

	printk(KERN_DEBUG "%s: Enter.\n", __func__);

	idx = get_control_index("I2S0 Sample Rate");
	if (ab3550_ctl[idx].changed) {
		// TODO?
		ab3550_ctl[idx].changed = false;
	}

	idx = get_control_index("I2S1 Sample Rate");
	if (ab3550_ctl[idx].changed) {
		// TODO?
		ab3550_ctl[idx].changed = false;
	}

	idx = get_control_index("MIC1 MBias");
	if (ab3550_ctl[idx].changed) {
		MASK_SET_REG(MIC_BIAS1,
					 MBIAS_PWR_MASK,
					 ab3550_ctl[idx].value << MBIAS_PWR_SHIFT);
		ab3550_ctl[idx].changed = false;
	}

	idx = get_control_index("MIC1 MBias");
	if (ab3550_ctl[idx].changed) {
		MASK_SET_REG(MIC_BIAS2,
					 MBIAS_PWR_MASK,
					 ab3550_ctl[idx].value << MBIAS_PWR_SHIFT);
		ab3550_ctl[idx].changed = false;
	}

	idx = get_control_index("TX1 HP Filter");
	if (ab3550_ctl[idx].changed) {
		ab3550_set_ADC_filter(0, ab3550_ctl[idx].value);
		ab3550_ctl[idx].changed = false;
	}

	idx = get_control_index("TX2 HP Filter");
	if (ab3550_ctl[idx].changed) {
		ab3550_set_ADC_filter(1, ab3550_ctl[idx].value);
		ab3550_ctl[idx].changed = false;
	}

	idx = get_control_index("Voice Call");
	if (ab3550_ctl[idx].changed) {
		printk(KERN_DEBUG "%s: commiting 'Voice Call'.\n", __func__);
		if (ab3550_ctl[idx].value) {
			MASK_SET_REG(MIC_BIAS1,
						 MBIAS_PWR_MASK,
					     1 << MBIAS_PWR_SHIFT);
			MASK_SET_REG(MIC_BIAS2,
						 MBIAS_PWR_MASK,
					     1 << MBIAS_PWR_SHIFT);
	//turn_on_interface_tx(INTERFACE1);
			MASK_SET_REG(INTERFACE1,
						 MASTER_GENx_PWR_MASK,
						 1 << MASTER_GENx_PWR_SHIFT);

			printCommit(idx);
		}
		ab3550_ctl[idx].changed = false;
	}

	return 0;
}

void commit_all(void)
{
	commit_rx_routing();
	commit_rx_gain();
	commit_tx_routing();
	commit_tx_gain();
	commit_others();
}

static int ab3550_set_control_value(struct snd_soc_codec *codec,
				    unsigned int ctl, unsigned int value)
{
	if (ctl >= ARRAY_SIZE(ab3550_ctl))
		return -EINVAL;

	if (strcmp(ab3550_ctl[ctl].name, "Commit") == 0) {
		commit_all();

	} else if (ab3550_ctl[ctl].value != value) {
		printk(KERN_DEBUG "%s: Ctl %d changed from 0x%02x to 0x%02x\n", __func__, ctl, ab3550_ctl[ctl].value, value);

		ab3550_ctl[ctl].value = value;
		ab3550_ctl[ctl].changed = true;
	}

	return 0;
}

static int ab3550_add_controls(struct snd_soc_codec *codec)
{
	int err = 0, i, n = ARRAY_SIZE(ab3550_snd_controls);

	printk(KERN_DEBUG "%s: %s called.\n", __FILE__, __func__);
	for (i = 0; i < n; i++) {
		/* Initialize the control indice */
		if (! ab3550_ctl[i].is_enum) {
			((struct soc_mixer_control *)
			 ab3550_snd_controls[i].private_value)->reg = i;
		} else {
			((struct soc_enum *)
			 ab3550_snd_controls[i].private_value)->reg = i;
		}
		printk(KERN_DEBUG "%s: %s.reg = %d\n", __func__,
		       ab3550_ctl[i].name, i);
		err = snd_ctl_add(codec->card,
				  snd_ctl_new1(&ab3550_snd_controls[i], codec));
		if (err < 0) {
			printk(KERN_DEBUG "%s failed to add control No.%d of %d.\n",
			       __func__, i, n);
			return err;
		}
	}
	/* initialize control values */
	printk(KERN_DEBUG "%s: %s to return %d.\n", __FILE__, __func__, err);
	return err;
}



// snd_soc_dai ---------------------------------------------------------------------------------------------------------

static int ab3550_pcm_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai* dai)
{
	struct codec_dai_private *priv = (struct codec_dai_private*)dai->private_data;

	printk(KERN_DEBUG "%s: %s called.\n", __FILE__, __func__);
	/* TODO: consult the clock use count */


	// Configure registers on startup
	if (atomic_read(&priv->substream_count) == 0)
		ab3550_init_registers_startup(dai->id);

	atomic_inc(&priv->substream_count);

	// Configure registers for either playback or capture
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		ab3550_init_registers_playback(dai->id);
	else
		ab3550_init_registers_capture(dai->id);

	ab3550_print_registers(substream->stream);

	return 0;
}

static int ab3550_pcm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *hw_params,
				struct snd_soc_dai* dai)
{

  u8 val;
	const char *reg_name;

  printk(KERN_DEBUG "%s: %s called\n", __FILE__, __func__);

  reg_name = dai->id == 0 ? "I2S0 Sample Rate" : "I2S1 Sample Rate";

  switch(params_rate(hw_params))
  {
    case 8000:
      val=0;
      break;

    case 16000:
      val=1;
      break;

    case 44100:
      val=2;
      break;

    case 48000:
      val=3;
      break;

    default:
      return -EINVAL;
  }

  ab3550_ctl[get_control_index(reg_name)].value = val;

  MASK_SET_REG(dai->id == 0 ? INTERFACE0 : INTERFACE1,
						   I2Sx_SR_MASK,
						   val);

  return 0;
}

static void ab3550_pcm_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai* dai)
{
	int i;
	u8 iface;
	const char *mode_ctl_name;
	printk(KERN_DEBUG "%s: %s called.\n", __FILE__, __func__);
	if (dai->id == 0) {
		iface = INTERFACE0;
		mode_ctl_name = "I2S0 Mode";
	} else {
		iface = INTERFACE1;
		mode_ctl_name = "I2S1 Mode";
	}
	if (get_control_value_by_name(mode_ctl_name) == 0) {
		struct codec_dai_private *priv =
			(struct codec_dai_private *)dai->private_data;
		if (atomic_dec_and_test(&priv->substream_count))
			MASK_SET_REG(iface, MASTER_GENx_PWR_MASK, 0);
	}
	/*TODO: consult the clock use count */
/* 	MASK_SET_REG(CLOCK, CLOCK_ENABLE_MASK, 0); */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		struct ab3550_register* reg;
		/* TODO: Turn off the outamps only used by this I2S */
		for (i = 0; i < ARRAY_SIZE(outamp_indices); i++) {
			reg = &(ab3550_registers[(int)outamp_indices[i]]);
			MASK_SET_REG(reg->address, outamp_pwr_mask[i], 0);
		}
	} else {
		/* TODO: Turn off the inactive components */
	}
}

static int ab3550_set_dai_sysclk(struct snd_soc_dai* dai, int clk_id,
				unsigned int freq, int dir)
{
	/*
  u8 val;
	const char *reg_name;
	printk(KERN_DEBUG "%s: %s called\n", __FILE__, __func__);
	switch(freq) {
	case 8000:
		val = 0;
		break;
	case 16000:
		val = 1;
		break;
	case 44100:
		val = 2;
		break;
	case 48000:
		val = 3;
		break;
	default:
		printk(KERN_ERR "%s: invalid frequency %u.\n",
		       __func__, freq);
		return -EINVAL;
	}

	reg_name = dai->id == 0 ? "I2S0 Sample Rate" : "I2S1 Sample Rate";
	ab3550_ctl[get_control_index(reg_name)].value = val;
	return MASK_SET_REG(dai->id == 0 ? INTERFACE0 : INTERFACE1,
						I2Sx_SR_MASK,
						val << I2Sx_SR_SHIFT);
  */
  return 0;
}

static int ab3550_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
  u8 iface = (codec_dai->id == 0) ? INTERFACE0 : INTERFACE1;
  u8 val=0;
  printk(KERN_DEBUG "%s: %s called\n", __FILE__, __func__);

  switch (fmt & (SND_SOC_DAIFMT_FORMAT_MASK | SND_SOC_DAIFMT_MASTER_MASK)) {

	  case SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS:
	    val |= 1 << I2Sx_MODE_SHIFT;
      break;

    case SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBM_CFM:
      break;

	  default:
		  printk(KERN_WARNING "AB3550_dai: unsupported DAI format 0x%x\n",fmt);
		  return -EINVAL;
	}

  return MASK_SET_REG(iface,I2Sx_MODE_MASK | I2Sx_WORDLENGTH_MASK, val);
}

struct snd_soc_dai ab3550_codec_dai[] = {
	{
		.name = "ab3550_0",
		.id = 0,
		.playback = {
			.stream_name = "ab3550_0",
			.channels_min = 2,
			.channels_max = 2,
			.rates = AB3550_SUPPORTED_RATE,
			.formats = AB3550_SUPPORTED_FMT,
		},
		.capture = {
			.stream_name = "ab3550_0",
			.channels_min = 2,
			.channels_max = 2,
			.rates = AB3550_SUPPORTED_RATE,
			.formats = AB3550_SUPPORTED_FMT,
		},
		.ops = {
			.prepare = ab3550_pcm_prepare,
			.hw_params = ab3550_pcm_hw_params,
			.shutdown = ab3550_pcm_shutdown,
			.set_sysclk = ab3550_set_dai_sysclk,
			.set_fmt = ab3550_set_dai_fmt
		},
		.private_data = &privates[0]
	},
	{
		.name = "ab3550_1",
		.id = 1,
		.playback = {
			.stream_name = "ab3550_1",
			.channels_min = 2,
			.channels_max = 2,
			.rates = AB3550_SUPPORTED_RATE,
			.formats = AB3550_SUPPORTED_FMT,
		},
		.capture = {
			.stream_name = "ab3550_1",
			.channels_min = 2,
			.channels_max = 2,
			.rates = AB3550_SUPPORTED_RATE,
			.formats = AB3550_SUPPORTED_FMT,
		},
		.ops = {
			.prepare = ab3550_pcm_prepare,
			.hw_params = ab3550_pcm_hw_params,
			.shutdown = ab3550_pcm_shutdown,
			.set_sysclk = ab3550_set_dai_sysclk,
			.set_fmt = ab3550_set_dai_fmt
		},
		.private_data = &privates[1]
	}
};
EXPORT_SYMBOL_GPL(ab3550_codec_dai);





// snd_soc_codec_device ---------------------------------------------------------------------------------------------------

static int ab3550_codec_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret;

	printk(KERN_INFO "%s called. pdev = %p.\n", __func__, pdev);
	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;
	codec->name = "AB3550";
	codec->owner = THIS_MODULE;
	codec->dai = ab3550_codec_dai;
	codec->num_dai = 2;
	codec->read = ab3550_get_control_value;
	codec->write = ab3550_set_control_value;
	codec->reg_cache_size = ARRAY_SIZE(ab3550_ctl);
	codec->reg_cache = kmemdup(ab3550_ctl, sizeof(ab3550_ctl), GFP_KERNEL);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	socdev->codec = codec;
	mutex_init(&codec->mutex);
	printk(KERN_DEBUG "%s: %s: snd_soc_new_pcms to be called.\n",
	       __FILE__, __func__);
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "%s: Failed to create a new card "
		       "and new PCMs. error %d\n", __func__, ret);
		goto err1;
	}
	ret = snd_soc_init_card(socdev);

	if (ret < 0) {
		printk(KERN_ERR "%s: failed to register card.  error %d.\n",
		       __func__, ret);
		goto err2;
	}
	/* Add controls */
	if (ab3550_add_controls(socdev->codec) < 0)
		goto err2;
	ab3550_add_widgets(codec);
	return 0;

err2:
	snd_soc_free_pcms(socdev);
err1:
	kfree(codec);
	return ret;
}

static int ab3550_codec_remove(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s : pdev=%p.\n", __func__, pdev);
	return 0;
}

static int ab3550_codec_suspend(struct platform_device *pdev,
				pm_message_t state)
{
	printk(KERN_DEBUG "%s : pdev=%p.\n", __func__, pdev);
	return 0;
}

static int ab3550_codec_resume(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s : pdev=%p.\n", __func__, pdev);
	return 0;
}

struct snd_soc_codec_device soc_codec_dev_ab3550 = {
	.probe =	ab3550_codec_probe,
	.remove =	ab3550_codec_remove,
	.suspend =	ab3550_codec_suspend,
	.resume =	ab3550_codec_resume
};

EXPORT_SYMBOL_GPL(soc_codec_dev_ab3550);

static int ab3550_platform_probe(struct platform_device *pdev)
{
	int ret = 0;
	u8 reg, val;
	unsigned long i;
	printk(KERN_DEBUG "%s invoked with pdev = %p.\n", __func__, pdev);
	ab3550_dev = &pdev->dev;

	/* Initialize the codec registers */
	for (reg = MIC_BIAS1; reg < INTERFACE_SWAP; reg++) {
		SET_REG(reg, 0);
	}
	MASK_SET_REG(CLOCK, CLOCK_REF_SELECT_MASK,
		     1 << CLOCK_REF_SELECT_SHIFT);
	MASK_SET_REG(CLOCK, CLOCK_ENABLE_MASK,
		     1 << CLOCK_ENABLE_SHIFT);
	val = GET_REG(CLOCK, &val);
	printk(KERN_DEBUG "%s: CLOCK = 0x%02x.\n", __func__, val);

	// Init all registers from default control values
	for (i = 0; i < ARRAY_SIZE(ab3550_ctl); i++)
		if (ab3550_ctl[i].value)
			ab3550_ctl[i].changed = true;
	commit_all();

	return ret;
}

static int ab3550_platform_remove(struct platform_device *pdev)
{
	int ret;
	printk(KERN_DEBUG "%s called.\n", __func__);
	MASK_SET_REG(CLOCK, CLOCK_ENABLE_MASK, 0);
	if ((ret = soc_codec_dev_ab3550.remove(NULL)))
		printk(KERN_ERR "%s failed with error code %d.\n",
		       __func__, ret);
	return ret;
}

static int ab3550_platform_suspend(struct platform_device *pdev,
				   pm_message_t state)
{
	int ret;
	printk(KERN_DEBUG "%s called.\n", __func__);
	if ((ret = soc_codec_dev_ab3550.suspend(NULL, state)))
		printk(KERN_ERR "%s failed with error code %d.\n",
		       __func__, ret);
	return ret;
}

static int ab3550_platform_resume(struct platform_device *pdev)
{
	int ret;
	printk(KERN_DEBUG "%s called.\n", __func__);
	if ((ret = soc_codec_dev_ab3550.resume(NULL)))
		printk(KERN_ERR "%s failed with error code %d.\n",
		       __func__, ret);
	return ret;
}

static struct platform_driver ab3550_platform_driver = {
	.driver		= {
		.name		= "ab3550-codec",
		.owner		= THIS_MODULE,
	},
	.probe		= ab3550_platform_probe,
	.remove		= ab3550_platform_remove,
	.suspend	= ab3550_platform_suspend,
	.resume		= ab3550_platform_resume,
};


static int __devinit ab3550_init(void)
{
	int i, ret1;
	int ret2 = 0;

	printk(KERN_DEBUG "%s called.\n", __func__);

	// Register codec platform driver.
	ret1 = platform_driver_register(&ab3550_platform_driver);
	if (ret1 < 0) {
		printk(KERN_DEBUG "%s: Error %d: Failed to register codec platform driver.\n", __func__, ret1);
		ret2 = -1;
	}

	// Register codec-dai.
	printk(KERN_DEBUG "%s: Register codec-dai.\n", __func__);

	for (i = 0; i < ARRAY_SIZE(ab3550_codec_dai); i++) {
		printk(KERN_DEBUG "%s: Register codec-dai %d.\n", __func__, i);
		ret1 = snd_soc_register_dai(&ab3550_codec_dai[i]);
		if (ret1 < 0) {
		  printk(KERN_DEBUG "MOP500_AB3550: Error: Failed to register codec-dai %d.\n", i);
		  ret2 = -1;
		}
	}

	return ret2;
}

static void ab3550_exit(void)
{
	int i;

	printk(KERN_DEBUG "u8500_ab3550_init: Enter.\n");

	// Register codec platform driver.
	printk(KERN_DEBUG "%s: Un-register codec platform driver.\n", __func__);
	platform_driver_unregister(&ab3550_platform_driver);

	for (i = 0; i < ARRAY_SIZE(ab3550_codec_dai); i++) {
		printk(KERN_DEBUG "%s: Un-register codec-dai %d.\n", __func__, i);
    snd_soc_unregister_dai(&ab3550_codec_dai[i]);
	}
}

module_init(ab3550_init);
module_exit(ab3550_exit);

MODULE_DESCRIPTION("AB3550 Codec driver");
MODULE_AUTHOR("Xie Xiaolei, xie.xiaolei@stericsson.com, www.stericsson.com");
MODULE_LICENSE("GPL");
