/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Xie Xiaolei <xie.xiaolei@etericsson.com>
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
#include "ab3550.h"

#include "../u8500/u8500_pcm.h"
#include "../u8500/u8500_msp_dai.h"

#define I2C_BANK 0

static struct device *ab3550_dev = NULL;

#define SET_REG(reg, val) abx500_set_register_interruptible( \
		ab3550_dev, I2C_BANK, (reg), (val))

#define MASK_SET_REG(reg, mask, val) \
	abx500_mask_and_set_register_interruptible( \
		ab3550_dev, I2C_BANK, (reg), (mask), (val))

#define GET_REG(reg, val) abx500_get_register_interruptible( \
		ab3550_dev, I2C_BANK, (reg), (val))

#define AB3550_SUPPORTED_RATE (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | \
			       SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000)

#define AB3550_SUPPORTED_FMT (SNDRV_PCM_FMTBIT_S16_LE | \
			      SNDRV_PCM_FMTBIT_S24_LE)

static const u8 outamp_reg[] = {
	LINE1, LINE2, SPKR, EAR, AUXO1, AUXO2
};
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

static int ab3550_pcm_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai* dai);
static int ab3550_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *hw_params, struct snd_soc_dai* dai);
static void ab3550_pcm_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai* dai);

static int ab3550_set_dai_sysclk(struct snd_soc_dai *codec_dai, int clk_id,
				 unsigned int freq, int dir);
static int ab3550_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt);

struct snd_soc_dai ab3550_codec_dai[] = {
	{
		.name = "ab3550_0",
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
	},
	{
		.name = "ab3550_1",
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
	}
};
EXPORT_SYMBOL_GPL(ab3550_codec_dai);

struct ab3550_control {
	const char *name;
	u8 value;
	int is_enum;
};

/* When AB3550 starts up, these registers are initialized to 0 */
struct ab3550_control ab3550_ctl[] = {
	{"RX2 Select", 0, 1},
	{"DAC1 Routing", 0, 0},
	{"DAC2 Routing", 0, 0},
	{"DAC3 Routing", 0, 0},
	{"MIC1 Input Select", 0, 0},
	{"MIC2 Input Select", 0, 0},
	{"I2S0 TX", 0, 1},
	{"I2S1 TX", 0, 1},
	{"APGA1 Source", 0, 1},
	{"APGA2 Source", 0, 1},
	{"APGA1 Destination", 0, 0},
	{"APGA2 Destination", 0, 0},
	{"DAC1 Side Tone", 0, 1},
	{"DAC2 Side Tone", 0, 1},
	{"RX-DPGA1 Gain", 0x3D, 0},
	{"RX-DPGA2 Gain", 0x3D, 0},
	{"RX-DPGA3 Gain", 0x3D, 0},
	{"LINE1 Gain", 0x0A, 0},
	{"LINE2 Gain", 0x0A, 0},
	{"SPKR Gain", 0x0A, 0},
	{"EAR Gain", 0x0A, 0},
	{"AUXO1 Gain", 0x0A, 0},
	{"AUXO2 Gain", 0x0A, 0},
	{"MIC1 Gain", 0, 0},
	{"MIC2 Gain", 0, 0},
	{"TX-DPGA1 Gain", 0, 0},
	{"TX-DPGA2 Gain", 0, 0},
	{"ST-PGA1 Gain", 0, 0},
	{"ST-PGA2 Gain", 0, 0},
	{"APGA1 Gain", 0, 0},
	{"APGA2 Gain", 0, 0},
	{"DAC1 Power Mode", 0, 1},
	{"DAC2 Power Mode", 0, 1},
	{"DAC3 Power Mode", 0, 1},
	{"EAR Power Mode", 0, 1},
	{"AUXO Power Mode", 0, 1},
	{"LINE1 Inverse", 0, 0},
	{"LINE2 Inverse", 0, 0},
	{"AUXO1 Inverse", 0, 0},
	{"AUXO2 Inverse", 0, 0},
	{"AUXO1 Pulldown", 0, 0},
	{"AUXO2 Pulldown", 0, 0},
	{"VMID1", 0, 0},
	{"VMID2", 0, 0},
	{"MIC1-1 MBias", 0, 1},
	{"MIC1-2 MBias", 0, 1},
	{"MIC2-1 MBias", 0, 1},
	{"MIC2-2 MBias", 0, 1},
	{"MBIAS1 HiZ Option", 0, 1},
	{"MBIAS2 HiZ Option", 0, 1},
	{"MBIAS2 Output Voltage", 0, 1},
	{"MBIAS2 Internal Resistor", 0, 1},
	{"MIC1 Input Impedance", 0, 1},
	{"MIC2 Input Impedance", 0, 1},
	{"TX1 HP Filter", 0, 1},
	{"TX2 HP Filter", 0, 1},
	{"LINEIN1 Pre-charge", 0, 0},
	{"LINEIN2 Pre-charge", 0, 0},
	{"MIC1P1 Pre-charge", 0, 0},
	{"MIC1P2 Pre-charge", 0, 0},
	{"MIC1N1 Pre-charge", 0, 0},
	{"MIC1N2 Pre-charge", 0, 0},
	{"MIC2P1 Pre-charge", 0, 0},
	{"MIC2P2 Pre-charge", 0, 0},
	{"MIC2N1 Pre-charge", 0, 0},
	{"MIC2N2 Pre-charge", 0, 0},
	{"ST1 HP Filter", 0, 1},
	{"ST2 HP Filter", 0, 1},
	{"I2S0 Word Length", 0, 1},
	{"I2S1 Word Length", 0, 1},
	{"I2S0 Mode", 0, 1},
	{"I2S1 Mode", 0, 1},
	{"I2S0 Tri-state", 0, 1},
	{"I2S1 Tri-state", 0, 1},
	{"I2S0 Pulldown", 0, 1},
	{"I2S1 Pulldown", 0, 1},
	{"I2S0 Sample Rate", 0, 1},
	{"I2S1 Sample Rate", 0, 1},
	{"Interface Loop", 0, 0},
	{"Interface Swap", 0, 0},
	{"Commit", 0, 0},
};

/* control No. 0 correpsonds to bit No. 0 in this array.
   If a control value has been changed, but not commited
   to the AB3550. Its corresponding bit is set.
 */
static unsigned long control_changed[] = {
	0, 0, 0, 0
};

static const char *enum_rx2_select[] = {"I2S0", "I2S1"};
static const char *enum_i2s_tx[] =
{"TX1", "TX2", "tri-state", "mute"};

static const char *enum_apga1_source[] =
{"None", "LINEIN1", "MIC1", "MIC2"};

static const char *enum_apga2_source[] =
{"None", "LINEIN2", "MIC1", "MIC2"};

static const char *enum_dac1_side_tone[] =
{"None", "TX1", "TX2"};

static const char *enum_dac1_power_mode[] =
{"100%", "75%", "50%"};

static const char *enum_ear_power_mode[] =
{"100%", "75%"};

static const char *enum_auxo_power_mode[] =
{"100%", "75%", "50%", "25%"};

static const char *enum_mic_bias_connection[] =
{"MIC Bias 1", "MIC Bias 2"};

static const char *enum_mbias1_hiz_option[] =
{"GND", "HiZ"};

static const char *enum_mbias2_output_voltage[] =
{"2.0v", "2.2v"};

static const char *enum_mbias2_internal_resistor[] =
{"connected", "bypassed"};

static const char *enum_mic1_input_impedance[] =
{"12.5 kohm", "25 kohm", "50 kohm"};

static const char *enum_tx1_hp_filter[] =
{"HP3", "HP1", "bypass"};

static const char *enum_i2s_word_length[] =
{"16 bits", "24 bits"};

static const char *enum_i2s_mode[] =
{"Master Mode", "Slave Mode"};

static const char *enum_i2s_tristate[] =
{"Normal", "Tri-state"};

static const char *enum_i2s_pulldown[] =
{"disconnected", "connected"};

static const char *enum_i2s_sample_rate[] =
{"8 kHz", "16 kHz", "44.1 kHz", "48 kHz"};


static struct soc_enum ab3550_enum[] = {
	/* RX2 Select */
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_rx2_select), enum_rx2_select),
	/* I2S0 TX */
	SOC_ENUM_DOUBLE(0, 0, 4, ARRAY_SIZE(enum_i2s_tx), enum_i2s_tx),
	/* I2S1 TX */
	SOC_ENUM_DOUBLE(0, 0, 4, ARRAY_SIZE(enum_i2s_tx), enum_i2s_tx),
	/* APGA1 Source */
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_apga1_source),
			enum_apga1_source),
	/* APGA2 Source */
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_apga2_source),
			enum_apga2_source),
	/* DAC1 Side Tone */
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_dac1_side_tone),
			enum_dac1_side_tone),
	/* DAC2 Side Tone */
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_dac1_side_tone),
			enum_dac1_side_tone),
	/* DAC1 Power Mode */
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_dac1_power_mode),
			enum_dac1_power_mode),
	/* DAC2 Power Mode */
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_dac1_power_mode),
			enum_dac1_power_mode),
	/* DAC3 Power Mode */
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_dac1_power_mode),
			enum_dac1_power_mode),
	/* EAR Power Mode */
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_ear_power_mode),
			enum_ear_power_mode),
	/* AUXO Power Mode */
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_auxo_power_mode),
			enum_auxo_power_mode),
	/* MIC Bias Connection */
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mic_bias_connection),
			enum_mic_bias_connection),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mic_bias_connection),
			enum_mic_bias_connection),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mic_bias_connection),
			enum_mic_bias_connection),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mic_bias_connection),
			enum_mic_bias_connection),
	/* MBIAS1 HiZ Option */
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mbias1_hiz_option),
			enum_mbias1_hiz_option),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mbias1_hiz_option),
			enum_mbias1_hiz_option),
	/* MBIAS2 Output voltage */
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mbias2_output_voltage),
			enum_mbias2_output_voltage),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mbias2_internal_resistor),
			enum_mbias2_internal_resistor),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mic1_input_impedance),
			enum_mic1_input_impedance),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_mic1_input_impedance),
			enum_mic1_input_impedance),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_tx1_hp_filter),
			enum_tx1_hp_filter),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_tx1_hp_filter),
			enum_tx1_hp_filter),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_tx1_hp_filter),
			enum_tx1_hp_filter),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_tx1_hp_filter),
			enum_tx1_hp_filter),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_word_length),
			enum_i2s_word_length),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_word_length),
			enum_i2s_word_length),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_mode),
			enum_i2s_mode),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_mode),
			enum_i2s_mode),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_tristate),
			enum_i2s_tristate),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_tristate),
			enum_i2s_tristate),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_pulldown),
			enum_i2s_pulldown),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_pulldown),
			enum_i2s_pulldown),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_sample_rate),
			enum_i2s_sample_rate),
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(enum_i2s_sample_rate),
			enum_i2s_sample_rate),

};

static struct snd_kcontrol_new ab3550_snd_controls[] = {
	/* RX Routing */
	SOC_ENUM("RX2 Select", ab3550_enum[0]),
	SOC_SINGLE("DAC1 Routing", 0, 0, 0x3f, 0),
	SOC_SINGLE("DAC2 Routing", 0, 0, 0x3f, 0),
	SOC_SINGLE("DAC3 Routing", 0, 0, 0x3f, 0),
	/* TX Routing */
	SOC_SINGLE("MIC1 Input Select", 0, 0, 0xff, 0),
	SOC_SINGLE("MIC2 Input Select", 0, 0, 0x3f, 0),
	SOC_ENUM("I2S0 TX", ab3550_enum[1]),
	SOC_ENUM("I2S1 TX", ab3550_enum[2]),
	/* Routing of Side Tone and Analop Loop */
	SOC_ENUM("APGA1 Source", ab3550_enum[3]),
	SOC_ENUM("APGA2 Source", ab3550_enum[4]),
	SOC_SINGLE("APGA1 Destination", 0, 0, 0x3f, 0),
	SOC_SINGLE("APGA2 Destination", 0, 0, 0x3f, 0),
	SOC_ENUM("DAC1 Side Tone", ab3550_enum[5]),
	SOC_ENUM("DAC2 Side Tone", ab3550_enum[5]),
	/* RX Volume Control */
	SOC_SINGLE("RX-DPGA1 Gain", 0, 0, 66, 0),
	SOC_SINGLE("RX-DPGA2 Gain", 0, 0, 66, 0),
	SOC_SINGLE("RX-DPGA3 Gain", 0, 0, 66, 0),
	SOC_SINGLE("LINE1 Gain", 0, 0, 10, 0),
	SOC_SINGLE("LINE2 Gain", 0, 0, 10, 0),
	SOC_SINGLE("SPKR Gain", 0, 0, 22, 0),
	SOC_SINGLE("EAR Gain", 0, 0, 14, 0),
	SOC_SINGLE("AUXO1 Gain", 0, 0, 12, 0),
	SOC_SINGLE("AUXO2 Gain", 0, 0, 12, 0),
	/* TX Volume Control */
	SOC_SINGLE("MIC1 Gain", 0, 0, 10, 0),
	SOC_SINGLE("MIC2 Gain", 0, 0, 10, 0),
	SOC_SINGLE("TX-DPGA1 Gain", 0, 0, 15, 0),
	SOC_SINGLE("TX-DPGA2 Gain", 0, 0, 15, 0),
	/* Volume Control of Side Tone and Analog Loop */
	SOC_SINGLE("ST-PGA1 Gain", 0, 0, 10, 0),
	SOC_SINGLE("ST-PGA2 Gain", 0, 0, 10, 0),
	SOC_SINGLE("APGA1 Gain", 0, 0, 28, 0),
	SOC_SINGLE("APGA2 Gain", 0, 0, 28, 0),
	/* RX Properties */
	SOC_ENUM("DAC1 Power Mode", ab3550_enum[6]),
	SOC_ENUM("DAC2 Power Mode", ab3550_enum[7]),
	SOC_ENUM("DAC3 Power Mode", ab3550_enum[8]),
	SOC_ENUM("EAR Power Mode", ab3550_enum[9]),
	SOC_ENUM("AUXO Power Mode", ab3550_enum[10]),
	SOC_SINGLE("LINE1 Inverse", 0, 0, 1, 0),
	SOC_SINGLE("LINE2 Inverse", 0, 0, 1, 0),
	SOC_SINGLE("AUXO1 Inverse", 0, 0, 1, 0),
	SOC_SINGLE("AUXO2 Inverse", 0, 0, 1, 0),
	SOC_SINGLE("AUXO1 Pulldown", 0, 0, 1, 0),
	SOC_SINGLE("AUXO2 Pulldown", 0, 0, 1, 0),
	/* TX Properties */
	SOC_SINGLE("VMID1", 0, 0, 0xff, 0),
	SOC_SINGLE("VMID2", 0, 0, 0xff, 0),
	SOC_ENUM("MIC1-1 MBias", ab3550_enum[11]),
	SOC_ENUM("MIC1-2 MBias", ab3550_enum[12]),
	SOC_ENUM("MIC2-1 MBias", ab3550_enum[13]),
	SOC_ENUM("MIC2-2 MBias", ab3550_enum[14]),
	SOC_ENUM("MBIAS1 HiZ Option", ab3550_enum[15]),
	SOC_ENUM("MBIAS2 HiZ Option", ab3550_enum[16]),
	SOC_ENUM("MBIAS2 Output Voltage", ab3550_enum[17]),
	SOC_ENUM("MBIAS2 Internal Resistor", ab3550_enum[18]),
	SOC_ENUM("MIC1 Input Impedance", ab3550_enum[19]),
	SOC_ENUM("MIC2 Input Impedance", ab3550_enum[20]),
	SOC_ENUM("TX1 HP Filter", ab3550_enum[21]),
	SOC_ENUM("TX2 HP Filter", ab3550_enum[22]),
	SOC_SINGLE("LINEIN1 Pre-charge", 0, 0, 100, 0),
	SOC_SINGLE("LINEIN2 Pre-charge", 0, 0, 100, 0),
	SOC_SINGLE("MIC1P1 Pre-charge", 0, 0, 100, 0),
	SOC_SINGLE("MIC1P2 Pre-charge", 0, 0, 100, 0),
	SOC_SINGLE("MIC1N1 Pre-charge", 0, 0, 100, 0),
	SOC_SINGLE("MIC1N2 Pre-charge", 0, 0, 100, 0),
	SOC_SINGLE("MIC2P1 Pre-charge", 0, 0, 100, 0),
	SOC_SINGLE("MIC2P2 Pre-charge", 0, 0, 100, 0),
	SOC_SINGLE("MIC2N1 Pre-charge", 0, 0, 100, 0),
	SOC_SINGLE("MIC2N2 Pre-charge", 0, 0, 100, 0),
	/* Side Tone and Analog Loop Properties */
	SOC_ENUM("ST1 HP Filter", ab3550_enum[23]),
	SOC_ENUM("ST2 HP Filter", ab3550_enum[24]),
	/* I2S Interface Properties */
	SOC_ENUM("I2S0 Word Length", ab3550_enum[25]),
	SOC_ENUM("I2S1 Word Length", ab3550_enum[26]),
	SOC_ENUM("I2S0 Mode", ab3550_enum[27]),
	SOC_ENUM("I2S1 Mode", ab3550_enum[28]),
	SOC_ENUM("I2S0 tri-state", ab3550_enum[29]),
	SOC_ENUM("I2S1 tri-state", ab3550_enum[30]),
	SOC_ENUM("I2S0 pulldown", ab3550_enum[31]),
	SOC_ENUM("I2S1 pulldown", ab3550_enum[32]),
	SOC_ENUM("I2S0 Sample Rate", ab3550_enum[33]),
	SOC_ENUM("I2S1 Sample Rate", ab3550_enum[34]),
	SOC_SINGLE("Interface Loop", 0, 0, 0x0f, 0),
	SOC_SINGLE("Interface Swap", 0, 0, 0x1f, 0),
	/* Miscellaneous */
	SOC_SINGLE("Commit", 0, 0, 1, 0)
};

static const struct snd_soc_dapm_widget ab3550_dapm_widgets[] = {
};

static const struct snd_soc_dapm_route intercon[] = {
};

static unsigned int ab3550_get_control_value(struct snd_soc_codec *codec,
					     unsigned int ctl)
{
	if (ctl >= ARRAY_SIZE(ab3550_ctl))
		return -EINVAL;
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
	int i, n;
	for(i = 0, n = ARRAY_SIZE(ab3550_ctl); i < n; i++) {
		if (strcmp(ab3550_ctl[i].name, name) == 0)
			break;
	}
	return ab3550_ctl[i].value;
}

static int gain_ramp(u8 reg, u8 target, int shift, u8 mask)
{
	u8 curr, step;
	int ret = 0;

//	printk(KERN_DEBUG "%s invoked.\n", __func__);
	if ((ret = GET_REG(reg, &curr))) {
		return ret;
	}
	if (curr == target)
		return 0;
//	step = curr < target ? 1 : -1;
//	while (!ret && curr != target) {
/*		At least 15ms are required, so I take 16ms :-) */
//		msleep(1);
//		ret = MASK_SET_REG(reg, mask, (curr += step) << shift);
//	}
	ret = MASK_SET_REG(reg, mask, target << shift);
	return ret;
}

static int ab3550_set_dai_sysclk(struct snd_soc_dai *codec_dai, int clk_id,
				unsigned int freq, int dir)
{
	printk(KERN_DEBUG "%s: %s called\n", __FILE__, __func__);
	return 0;
}

static int ab3550_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	printk(KERN_DEBUG "%s: %s called.\n", __FILE__, __func__);
	return 0;
}

static int ab3550_pcm_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai* dai)
{
	u8 val;
	printk(KERN_DEBUG "%s: %s called.\n", __FILE__, __func__);
	MASK_SET_REG(CLOCK, CLOCK_ENABLE_MASK, 1 << CLOCK_ENABLE_SHIFT);
	/* Turn on the master generator */
	MASK_SET_REG(INTERFACE0, MASTER_GENx_PWR_MASK,
		     1 << MASTER_GENx_PWR_SHIFT);
	GET_REG(CLOCK, &val);
	printk(KERN_DEBUG "%s: CLOCK = %02x.\n", __func__, val);
	GET_REG(INTERFACE0, &val);
	printk(KERN_DEBUG "%s: INTERFACE0 = %02x.\n", __func__, val);
	GET_REG(RX1, &val);
	printk(KERN_DEBUG "%s: RX1 = %02x.\n", __func__, val);
	GET_REG(RX2, &val);
	printk(KERN_DEBUG "%s: RX2 = %02x.\n", __func__, val);
	GET_REG(SPKR_ADDER, &val);
	printk(KERN_DEBUG "%s: SPKR_ADDER = %02x.\n", __func__, val);
	GET_REG(AUXO1_ADDER, &val);
	printk(KERN_DEBUG "%s: AUXO1_ADDER = %02x.\n", __func__, val);
	GET_REG(AUXO2_ADDER, &val);
	printk(KERN_DEBUG "%s: AUXO2_ADDER = %02x.\n", __func__, val);
	GET_REG(SPKR, &val);
	printk(KERN_DEBUG "%s: SPKR = %02x.\n", __func__, val);
	GET_REG(AUXO1, &val);
	printk(KERN_DEBUG "%s: AUXO1 = %02x.\n", __func__, val);
	GET_REG(AUXO2, &val);
	printk(KERN_DEBUG "%s: AUXO2 = %02x.\n", __func__, val);
	GET_REG(RX1_DIGITAL_PGA, &val);
	printk(KERN_DEBUG "%s: RX1_DIGITAL_PGA = %02x.\n", __func__, val);
	GET_REG(RX2_DIGITAL_PGA, &val);
	printk(KERN_DEBUG "%s: RX2_DIGITAL_PGA = %02x.\n", __func__, val);
	return 0;
}

static void ab3550_pcm_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai* dai)
{
	int i;
	printk(KERN_DEBUG "%s: %s called.\n", __FILE__, __func__);
	MASK_SET_REG(INTERFACE0, MASTER_GENx_PWR_MASK, 0);
	MASK_SET_REG(CLOCK, CLOCK_ENABLE_MASK, 0);
	for (i = 0; i < ARRAY_SIZE(outamp_reg); i++) {
		MASK_SET_REG(outamp_reg[i], outamp_pwr_mask[i], 0);
	}
}

static int set_up_speaker_playback(void)
{
	printk(KERN_DEBUG "%s: %s called.\n", __FILE__, __func__);
	MASK_SET_REG(CLOCK, CLOCK_REF_SELECT_MASK,
		     1 << CLOCK_REF_SELECT_SHIFT);

	/* Sample rate 48 kHz */
	SET_REG(INTERFACE0, 0x03);
	printk(KERN_DEBUG "%s: interface 0 is setup", __func__);

	MASK_SET_REG(RX2, RX2_IF_SELECT_MASK, 0);
	SET_REG(SPKR_ADDER, 1 << DAC1_TO_ADDER_SHIFT |
		1 << DAC2_TO_ADDER_SHIFT);
/* 	SET_REG(AUXO1_ADDER, 1 << DAC1_TO_ADDER_SHIFT); */
/* 	SET_REG(AUXO2_ADDER, 1 << DAC2_TO_ADDER_SHIFT); */
	printk(KERN_DEBUG "%s: Routes to the speaker and the AUXO are setup.\n",
	       __func__);

	/* Mute the outamps first */
	MASK_SET_REG(RX1_DIGITAL_PGA, RXx_PGA_GAIN_MASK, 0);
	MASK_SET_REG(RX2_DIGITAL_PGA, RXx_PGA_GAIN_MASK, 0);
/* 	MASK_SET_REG(AUXO1, AUXOx_GAIN_MASK, 0); */
/* 	MASK_SET_REG(AUXO2, AUXOx_GAIN_MASK, 0); */
	MASK_SET_REG(SPKR, SPKR_GAIN_MASK, 0);

	MASK_SET_REG(RX1, RX1_PWR_MASK | DAC1_PWR_MASK | DAC1_PWR_MODE_MASK,
		1 << RX1_PWR_SHIFT | 1 << DAC1_PWR_SHIFT |
		1 << DAC1_PWR_MODE_SHIFT);
	MASK_SET_REG(RX2, RX2_PWR_MASK | DAC2_PWR_MASK | DAC2_PWR_MODE_MASK,
		     1 << RX2_PWR_SHIFT | 1 << DAC2_PWR_SHIFT |
		     1 << DAC2_PWR_MODE_SHIFT);
	printk(KERN_DEBUG "%s: DAC1, DAC2 and RX2 are now powered up.\n",
	       __func__);

	MASK_SET_REG(SPKR, SPKR_PWR_MASK, 1 << SPKR_PWR_SHIFT);
/* 	MASK_SET_REG(AUXO1, AUXOx_PWR_MASK, 1 << AUXOx_PWR_SHIFT); */
/* 	MASK_SET_REG(AUXO2, AUXOx_PWR_MASK, 1 << AUXOx_PWR_SHIFT); */
	printk(KERN_DEBUG "%s: The speaker and the AUXO are now powered up.\n",
	       __func__);

	gain_ramp(RX1_DIGITAL_PGA, get_control_value_by_name("RX-DPGA1 Gain"),
		  RXx_PGA_GAIN_SHIFT, RXx_PGA_GAIN_MASK);

	gain_ramp(RX2_DIGITAL_PGA, get_control_value_by_name("RX-DPGA2 Gain"),
		  RXx_PGA_GAIN_SHIFT, RXx_PGA_GAIN_MASK);
	gain_ramp(SPKR, get_control_value_by_name("SPKR Gain"),
		  SPKR_GAIN_SHIFT, SPKR_GAIN_MASK);
/* 	gain_ramp(AUXO1, get_control_value_by_name("AUXO1 Gain"), */
/* 		  AUXOx_GAIN_SHIFT, AUXOx_GAIN_MASK); */
/* 	gain_ramp(AUXO2, get_control_value_by_name("AUXO1 Gain"), */
/* 		  AUXOx_GAIN_SHIFT, AUXOx_GAIN_MASK); */
	printk(KERN_DEBUG "%s: return 0.\n", __func__);
	return 0;
}

static int ab3550_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *hw_params, struct snd_soc_dai* dai)
{
	printk(KERN_DEBUG "%s: %s called.\n", __FILE__, __func__);
	set_up_speaker_playback();
	return 0;
}

static int ab3550_add_widgets(struct snd_soc_codec *codec)
{
	snd_soc_dapm_new_controls(codec, ab3550_dapm_widgets,
				  ARRAY_SIZE(ab3550_dapm_widgets));

	snd_soc_dapm_add_routes(codec, intercon, ARRAY_SIZE(intercon));

	snd_soc_dapm_new_widgets(codec);
	return 0;
}

static int commit_outamps_routing(void)
{
	unsigned long idx;
	int adjust = 0;
	static const char *control_names[] = {
		"DAC1 Routing", "DAC2 Routing", "DAC3 Routing",
		"APGA1 Destination", "APGA2 Destination"
	};
	static const u8 outamp_adder_reg[] = {
		LINE1_ADDER, LINE2_ADDER, SPKR_ADDER,
		EAR_ADDER, AUXO1_ADDER, AUXO2_ADDER
	};
	static const u8 outamp_0db_gain[] = {
		0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C
	};

	for_each_bit(idx, control_changed, ARRAY_SIZE(ab3550_ctl)) {
		if (strcmp(ab3550_ctl[idx].name, "DAC1 Routing") == 0 ||
		    strcmp(ab3550_ctl[idx].name, "DAC2 Routing") == 0 ||
		    strcmp(ab3550_ctl[idx].name, "DAC3 Routing") == 0 ||
		    strcmp(ab3550_ctl[idx].name, "APGA1 Destination") == 0 ||
		    strcmp(ab3550_ctl[idx].name, "APGA2 Destination") == 0
			) {
			adjust = 1;
		}
	}
	if (adjust) {
		int i;
		for (i = 0; i < ARRAY_SIZE(outamp_reg); i++) {
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
				GET_REG(outamp_reg[i], &val);
				if (! (val & 1 << outamp_pwr_shift[i]))
					continue;
				gain_ramp(outamp_reg[i], 0,
					  outamp_gain_shift[i],
					  outamp_gain_mask[i]);
				MASK_SET_REG(outamp_reg[i],
					     outamp_pwr_mask[i], 0);
				continue;
			}
			MASK_SET_REG(outamp_adder_reg[i],
				     DAC3_TO_ADDER_MASK |
				     DAC2_TO_ADDER_MASK |
				     DAC3_TO_ADDER_MASK,
				     connect);
			if (connect & 1 << 3) {
				/* Connect APGA1 */
				MASK_SET_REG(APGA1_ADDER,
					     1 << (ARRAY_SIZE(outamp_reg) - i),
					     1 << (ARRAY_SIZE(outamp_reg) - i));
			}
			if (connect & 1 << 4) {
				/* Connect APGA2 */
				MASK_SET_REG(APGA2_ADDER,
					     1 << (ARRAY_SIZE(outamp_reg) - i),
					     1 << (ARRAY_SIZE(outamp_reg) - i));
			}
			MASK_SET_REG(outamp_reg[i],
				     outamp_pwr_mask[i],
				     1 << outamp_pwr_shift[i]);
			gain_ramp(outamp_reg[i], outamp_0db_gain[i],
				  outamp_gain_shift[i],
				  outamp_gain_mask[i]);
		}
	}
	return 0;
}

static int commit_gain(void)
{
	unsigned long idx;
	for_each_bit(idx, control_changed, ARRAY_SIZE(ab3550_ctl)) {
		if (strcmp(ab3550_ctl[idx].name, "LINE1 Gain") == 0 ||
		    strcmp(ab3550_ctl[idx].name, "LINE2 Gain") == 0 ||
		    strcmp(ab3550_ctl[idx].name, "SPKR Gain") == 0 ||
		    strcmp(ab3550_ctl[idx].name, "EAR Gain") == 0 ||
		    strcmp(ab3550_ctl[idx].name, "AUXO1 Gain") == 0 ||
		    strcmp(ab3550_ctl[idx].name, "AUXO2 Gain") == 0
			) {
			/* Ramping is needed if powered up */
			int i = idx - get_control_index("LINE1 Gain");
			u8 val = GET_REG(outamp_reg[i], &val);
			if (val & 1 << outamp_pwr_shift[i]) {
				gain_ramp(outamp_reg[i], ab3550_ctl[idx].value,
					  outamp_gain_shift[i],
					  outamp_gain_mask[i]);
				continue;
			}
			/* Othwise set directly */
			MASK_SET_REG(outamp_reg[i], outamp_gain_mask[i],
				     ab3550_ctl[idx].value);
		} else if (strcmp(ab3550_ctl[idx].name, "RX-DPGA1 Gain") == 0) {
			gain_ramp(RX1_DIGITAL_PGA,
				  get_control_value_by_name("RX-DPGA1 Gain"),
				  RXx_PGA_GAIN_SHIFT, RXx_PGA_GAIN_MASK);

		} else if (strcmp(ab3550_ctl[idx].name, "RX-DPGA2 Gain") == 0) {
			gain_ramp(RX2_DIGITAL_PGA,
				  get_control_value_by_name("RX-DPGA2 Gain"),
				  RXx_PGA_GAIN_SHIFT, RXx_PGA_GAIN_MASK);

		} else if (strcmp(ab3550_ctl[idx].name, "RX-DPGA3 Gain") == 0) {
			gain_ramp(RX3_DIGITAL_PGA,
				  get_control_value_by_name("RX-DPGA3 Gain"),
				  RXx_PGA_GAIN_SHIFT, RXx_PGA_GAIN_MASK);

		}
	}
	return 0;
}

static int ab3550_set_control_value(struct snd_soc_codec *codec,
				    unsigned int ctl, unsigned int value)
{
	int ret = 0;

	if (ctl >= ARRAY_SIZE(ab3550_ctl))
		return -EINVAL;
	if (ab3550_ctl[ctl].value == value)
		return 0;
	ab3550_ctl[ctl].value = value;
	if (strcmp(ab3550_ctl[ctl].name, "Commit") != 0)
		return 0;
	/* Commit the changes */
	commit_outamps_routing();
	commit_gain();
	memset(control_changed, 0, sizeof(control_changed));


	return ret;
}

static int ab3550_add_controls(struct snd_soc_codec *codec)
{
	int err = 0, i, n = ARRAY_SIZE(ab3550_snd_controls);

	printk(KERN_DEBUG "%s: %s called.\n", __FILE__, __func__);
	for (i = 0; i < n; i++) {
		/* Initialize the control indice */
		if (! ab3550_ctl[i].is_enum) {
			ab3550_snd_controls[i].private_value |= i;
		} else {
			struct soc_enum *p = (struct soc_enum *)
				ab3550_snd_controls[i].private_value;
			p->reg = i;
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
	/* TODO: add codec dai */
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

/* struct snd_soc_dai ab3550_codec_dai = { */
/* 	.name = "AB3550", */
/* 	.playback = { */
/* 		.stream_name = "Playback", */
/* 		.channels_min = 1, */
/* 		.channels_max = 2, */
/* 		.rates = AB3550_RATES, */
/* 		.formats = AB3550_FORMATS,}, */
/* 	.capture = { */
/* 		.stream_name = "Capture", */
/* 		.channels_min = 1, */
/* 		.channels_max = 2, */
/* 		.rates = AB3550_RATES, */
/* 		.formats = AB3550_FORMATS,}, */
/* 	.ops = { */
/* 		.prepare = ab3550_pcm_prepare, */
/* 		.hw_params = ab3550_hw_params, */
/* 		.shutdown = ab3550_shutdown, */
/* 	}, */
/* 	.dai_ops = { */
/* 		.digital_mute = ab3550_mute, */
/* 		.set_sysclk = ab3550_set_dai_sysclk, */
/* 		.set_fmt = ab3550_set_dai_fmt, */
/* 	} */
/* }; */
/* EXPORT_SYMBOL_GPL(ab3550_codec_dai); */
static int ab3550_platform_probe(struct platform_device *pdev)
{
	int ret = 0;

	printk(KERN_DEBUG "%s invoked with pdev = %p.\n", __func__, pdev);
	ab3550_dev = &pdev->dev;
	return ret;
}

static int ab3550_platform_remove(struct platform_device *pdev)
{
	int ret;
	printk(KERN_DEBUG "%s called.\n", __func__);
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
	int ret;
	printk(KERN_DEBUG "%s called.\n", __func__);
	ret= platform_driver_register(&ab3550_platform_driver);
	printk(KERN_DEBUG "%s finished with error code %d.\n",
	       __func__, ret);
	return ret;
}

static void ab3550_exit(void)
{
	printk(KERN_DEBUG "%s called.\n", __func__);
	platform_driver_unregister(&ab3550_platform_driver);
}

module_init(ab3550_init);
module_exit(ab3550_exit);

MODULE_DESCRIPTION("AB3550 Codec driver");
MODULE_AUTHOR("Xie Xiaolei, xie.xiaolei@stericsson.com, www.stericsson.com");
MODULE_LICENSE("GPL");
