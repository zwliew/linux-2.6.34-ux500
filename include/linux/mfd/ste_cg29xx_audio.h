/*
 * include/linux/mfd/ste_cg29xx_audio.h
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Authors:
 * Par-Gunnar Hjalmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson. * License terms:  GNU General Public License (GPL), version 2
 *
 * Linux Bluetooth Audio Driver for ST-Ericsson controller.
 */

#ifndef _STE_CG29XX_AUDIO_H_
#define _STE_CG29XX_AUDIO_H_

#include <linux/types.h>

/*
 * Digital Audio Interface configuration types
 */

/** STE_CG29XX_AUDIO_A2DP_MAX_AVDTP_HEADER_LENGTH - Max length of a AVDTP header for an A2DP packet.
 */
#define STE_CG29XX_AUDIO_A2DP_MAX_AVDTP_HEADER_LENGTH 25

/**
 * enum ste_cg29xx_dai_direction - Contains the DAI port directions alternatives.
 *  @STE_CG29XX_DAI_DIRECTION_PORT_B_RX_PORT_A_TX: Port B as Rx and port A as Tx.
 *  @STE_CG29XX_DAI_DIRECTION_PORT_B_TX_PORT_A_RX: Port B as Tx and port A as Rx.
 */
enum ste_cg29xx_dai_direction {
  STE_CG29XX_DAI_DIRECTION_PORT_B_RX_PORT_A_TX = 0x00,
  STE_CG29XX_DAI_DIRECTION_PORT_B_TX_PORT_A_RX = 0x01
};

/**
 * enum ste_cg29xx_dai_mode - Contains the DAI mode alternatives.
 *  @STE_CG29XX_DAI_MODE_SLAVE: Slave.
 *  @STE_CG29XX_DAI_MODE_MASTER: Master.
 */
enum ste_cg29xx_dai_mode {
  STE_CG29XX_DAI_MODE_SLAVE = 0x00,
  STE_CG29XX_DAI_MODE_MASTER = 0x01
};

/**
 * enum ste_cg29xx_dai_voice_stream_ratio - Contains the alternatives for the voice stream ratio between the Audio stream sample rate and the Voice stream sample rate.
 *  @STE_CG29XX_DAI_VOICE_STREAM_RATIO_FM16_VOICE16:	FM 16kHz, Voice 16kHz.
 *  @STE_CG29XX_DAI_VOICE_STREAM_RATIO_FM16_VOICE8:	FM 16kHz, Voice 8kHz.
 *  @STE_CG29XX_DAI_VOICE_STREAM_RATIO_FM48_VOICE16:	FM 48kHz, Voice 16Khz.
 *  @STE_CG29XX_DAI_VOICE_STREAM_RATIO_FM48_VOICE8:	FM 48kHz, Voice 8kHz.
 */
enum ste_cg29xx_dai_voice_stream_ratio {
	STE_CG29XX_DAI_VOICE_STREAM_RATIO_FM16_VOICE16 = 0x01,
	STE_CG29XX_DAI_VOICE_STREAM_RATIO_FM16_VOICE8 = 0x02,
	STE_CG29XX_DAI_VOICE_STREAM_RATIO_FM48_VOICE16 = 0x03,
	STE_CG29XX_DAI_VOICE_STREAM_RATIO_FM48_VOICE8 = 0x06
};

/**
 * enum ste_cg29xx_dai_frame_sync_duration - Frame sync duration alternatives.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_8: 8 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_16: 16 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_24: 24 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_32: 32 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_48: 48 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_50: 50 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_64: 64 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_75: 75 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_96: 96 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_125: 125 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_128: 128 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_150: 150 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_192: 192 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_250: 250 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_256: 256 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_300: 300 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_384: 384 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_500: 500 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_512: 512 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_600: 600 frames sync duration.
 *  @STE_CG29XX_DAI_FRAME_SYNC_DURATION_768: 768 frames sync duration.
 *
 *  This parameter sets the PCM frame sync duration. It is calculated as the
 *  ratio between the bit clock and the frame rate. For example, if the bit
 *  clock is 512 kHz and the stream sample rate is 8 kHz, the PCM frame sync
 *  duration is 512 / 8 = 64.
 */
enum ste_cg29xx_dai_frame_sync_duration {
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_8 = 8,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_16 = 16,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_24 = 24,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_32 = 32,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_48 = 48,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_50 = 50,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_64 = 64,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_75 = 75,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_96 = 96,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_125 = 125,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_128 = 128,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_150 = 150,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_192 = 192,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_250 = 250,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_256 = 256,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_300 = 300,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_384 = 384,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_500 = 500,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_512 = 512,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_600 = 600,
  STE_CG29XX_DAI_FRAME_SYNC_DURATION_768 = 768
};

/**
 * enum ste_cg29xx_dai_bit_clk - Bit Clock alternatives.
 *  @STE_CG29XX_DAI_BIT_CLK_128: 128 Kbits clock.
 *  @STE_CG29XX_DAI_BIT_CLK_256: 256 Kbits clock.
 *  @STE_CG29XX_DAI_BIT_CLK_512: 512 Kbits clock.
 *  @STE_CG29XX_DAI_BIT_CLK_768: 768 Kbits clock.
 *  @STE_CG29XX_DAI_BIT_CLK_1024: 1024 Kbits clock.
 *  @STE_CG29XX_DAI_BIT_CLK_1411_76: 1411.76 Kbits clock.
 *  @STE_CG29XX_DAI_BIT_CLK_1536: 1536 Kbits clock.
 *  @STE_CG29XX_DAI_BIT_CLK_2000: 2000 Kbits clock.
 *  @STE_CG29XX_DAI_BIT_CLK_2048: 2048 Kbits clock.
 *  @STE_CG29XX_DAI_BIT_CLK_2400: 2400 Kbits clock.
 *  @STE_CG29XX_DAI_BIT_CLK_2823_52: 2823.52 Kbits clock.
 *  @STE_CG29XX_DAI_BIT_CLK_3072: 3072 Kbits clock.
 *
 *  This parameter sets the bit clock speed. This is the clocking of the actual
 *  data. A usual parameter for eSCO voice is 512 kHz.
 */
enum ste_cg29xx_dai_bit_clk {
  STE_CG29XX_DAI_BIT_CLK_128 = 0x00,
  STE_CG29XX_DAI_BIT_CLK_256 = 0x01,
  STE_CG29XX_DAI_BIT_CLK_512 = 0x02,
  STE_CG29XX_DAI_BIT_CLK_768 = 0x03,
  STE_CG29XX_DAI_BIT_CLK_1024 = 0x04,
  STE_CG29XX_DAI_BIT_CLK_1411_76 = 0x05,
  STE_CG29XX_DAI_BIT_CLK_1536 = 0x06,
  STE_CG29XX_DAI_BIT_CLK_2000 = 0x07,
  STE_CG29XX_DAI_BIT_CLK_2048 = 0x08,
  STE_CG29XX_DAI_BIT_CLK_2400 = 0x09,
  STE_CG29XX_DAI_BIT_CLK_2823_52 = 0x0A,
  STE_CG29XX_DAI_BIT_CLK_3072 = 0x0B
};

/**
 * enum ste_cg29xx_dai_sample_rate - Sample rates alternatives.
 *  @STE_CG29XX_DAI_SAMPLE_RATE_8:	8 kHz sample rate.
 *  @STE_CG29XX_DAI_SAMPLE_RATE_16:	16 kHz sample rate.
 *  @STE_CG29XX_DAI_SAMPLE_RATE_44_1:	44.1 kHz sample rate.
 *  @STE_CG29XX_DAI_SAMPLE_RATE_48:	48 kHz sample rate.
 */
enum ste_cg29xx_dai_sample_rate {
  STE_CG29XX_DAI_SAMPLE_RATE_8 = 0x00,
  STE_CG29XX_DAI_SAMPLE_RATE_16 = 0x01,
  STE_CG29XX_DAI_SAMPLE_RATE_44_1 = 0x02,
  STE_CG29XX_DAI_SAMPLE_RATE_48 = 0x04
};

/**
 * enum ste_cg29xx_dai_port_protocol - Port protocol alternatives.
 *  @STE_CG29XX_DAI_PORT_PROTOCOL_PCM: Protocol PCM.
 *  @STE_CG29XX_DAI_PORT_PROTOCOL_I2S: Protocol I2S.
 */
enum ste_cg29xx_dai_port_protocol {
  STE_CG29XX_DAI_PORT_PROTOCOL_PCM = 0x00,
  STE_CG29XX_DAI_PORT_PROTOCOL_I2S = 0x01
};

/**
 * struct ste_cg29xx_dai_port_conf_i2s_pcm - Port configuration structure.
 *  @mode: Operational mode of the port configured.
 *  @slot_0_dir: Direction of slot 0.
 *  @slot_1_dir: Direction of slot 1.
 *  @slot_2_dir: Direction of slot 2.
 *  @slot_3_dir: Direction of slot 3.
 *  @sco_slots_used: True if SCO slots are used.
 *  @a2dp_slots_used: True if A2DP slots are used.
 *  @fm_right_slot_used: True if FM right slot is used.
 *  @fm_left_slot_used:  True if FM left slot is used.
 *  @ring_tone_slots_used: True is ring tone slots are used.
 *  @slot_0_start: Slot 0 start (relative to the PCM frame sync).
 *  @slot_1_start: Slot 1 start (relative to the PCM frame sync)
 *  @slot_2_start: Slot 2 start (relative to the PCM frame sync)
 *  @slot_3_start: Slot 3 start (relative to the PCM frame sync)
 *  @ratio: Voice stream ratio between the Audio stream sample rate and the Voice stream sample rate.
 *  @protocol: Protocol used on port.
 *  @duration: Frame sync duration.
 *  @clk: Bit clock.
 *  @sample_rate: Sample rate.
 */
struct ste_cg29xx_dai_port_conf_i2s_pcm {
  enum ste_cg29xx_dai_mode mode;
  enum ste_cg29xx_dai_direction slot_0_dir;
  enum ste_cg29xx_dai_direction slot_1_dir;
  enum ste_cg29xx_dai_direction slot_2_dir;
  enum ste_cg29xx_dai_direction slot_3_dir;
  bool sco_slots_used;
  bool a2dp_slots_used;
  bool fm_right_slot_used;
  bool fm_left_slot_used;
  bool ring_tone_slots_used;
  unsigned char slot_0_start;
  unsigned char slot_1_start;
  unsigned char slot_2_start;
  unsigned char slot_3_start;
  enum ste_cg29xx_dai_voice_stream_ratio ratio;
  enum ste_cg29xx_dai_port_protocol protocol;
  enum ste_cg29xx_dai_frame_sync_duration duration;
  enum ste_cg29xx_dai_bit_clk clk;
  enum ste_cg29xx_dai_sample_rate sample_rate;
};

/**
 * enum ste_cg29xx_dai_half_period_duration - Contains the half period duration alternatives.
 *  @STE_CG29XX_DAI_HALF_PERIOD_DURATION_8: 8 Bits.
 *  @STE_CG29XX_DAI_HALF_PERIOD_DURATION_16: 16 Bits.
 *  @STE_CG29XX_DAI_HALF_PERIOD_DURATION_24: 24 Bits.
 *  @STE_CG29XX_DAI_HALF_PERIOD_DURATION_25: 25 Bits.
 *  @STE_CG29XX_DAI_HALF_PERIOD_DURATION_32: 32 Bits.
 *  @STE_CG29XX_DAI_HALF_PERIOD_DURATION_48: 48 Bits.
 *  @STE_CG29XX_DAI_HALF_PERIOD_DURATION_64: 64 Bits.
 *  @STE_CG29XX_DAI_HALF_PERIOD_DURATION_75: 75 Bits.
 *  @STE_CG29XX_DAI_HALF_PERIOD_DURATION_96: 96 Bits.
 *  @STE_CG29XX_DAI_HALF_PERIOD_DURATION_128: 128 Bits.
 *  @STE_CG29XX_DAI_HALF_PERIOD_DURATION_150: 150 Bits.
 *  @STE_CG29XX_DAI_HALF_PERIOD_DURATION_192: 192 Bits.
 *
 *  This parameter sets the number of bits contained in each I2S half period,
 *  i.e. each channel slot. A usual value is 16 bits.
 */
enum ste_cg29xx_dai_half_period_duration {
  STE_CG29XX_DAI_HALF_PERIOD_DURATION_8 = 0x00,
  STE_CG29XX_DAI_HALF_PERIOD_DURATION_16 = 0x01,
  STE_CG29XX_DAI_HALF_PERIOD_DURATION_24 = 0x02,
  STE_CG29XX_DAI_HALF_PERIOD_DURATION_25 = 0x03,
  STE_CG29XX_DAI_HALF_PERIOD_DURATION_32 = 0x04,
  STE_CG29XX_DAI_HALF_PERIOD_DURATION_48 = 0x05,
  STE_CG29XX_DAI_HALF_PERIOD_DURATION_64 = 0x06,
  STE_CG29XX_DAI_HALF_PERIOD_DURATION_75 = 0x07,
  STE_CG29XX_DAI_HALF_PERIOD_DURATION_96 = 0x08,
  STE_CG29XX_DAI_HALF_PERIOD_DURATION_128 = 0x09,
  STE_CG29XX_DAI_HALF_PERIOD_DURATION_150 = 0x0A,
  STE_CG29XX_DAI_HALF_PERIOD_DURATION_192 = 0x0B
};

/**
 * enum ste_cg29xx_dai_channel_selection - The channel selection alternatives.
 *  @STE_CG29XX_DAI_CHANNEL_SELECTION_RIGHT: Right channel used.
 *  @STE_CG29XX_DAI_CHANNEL_SELECTION_LEFT: Left channel used.
 *  @STE_CG29XX_DAI_CHANNEL_SELECTION_BOTH: Both channels used.
 */
enum ste_cg29xx_dai_channel_selection {
  STE_CG29XX_DAI_CHANNEL_SELECTION_RIGHT = 0x00,
  STE_CG29XX_DAI_CHANNEL_SELECTION_LEFT = 0x01,
  STE_CG29XX_DAI_CHANNEL_SELECTION_BOTH = 0x02
};

/**
 * enum ste_cg29xx_dai_word_width_selection - Word width alternatives.
 *  @STE_CG29XX_DAI_WORD_WIDTH_SELECTION_16: 16 bits words.
 *  @STE_CG29XX_DAI_WORD_WIDTH_SELECTION_32: 32 bits words.
 */
enum ste_cg29xx_dai_word_width_selection {
  STE_CG29XX_DAI_WORD_WIDTH_SELECTION_16 = 0x00,
  STE_CG29XX_DAI_WORD_WIDTH_SELECTION_32 = 0x01
};

/**
 * struct ste_cg29xx_dai_port_conf_i2s - Port configuration struct for I2S.
 *  @mode: Operational mode of the port.
 *  @half_period: Half period duration.
 *  @channel_selection: Channel selection.
 *  @sample_rate: Sample rate.
 *  @word_width: Word width.
 */
struct ste_cg29xx_dai_port_conf_i2s {
  enum ste_cg29xx_dai_mode mode;
  enum ste_cg29xx_dai_half_period_duration half_period;
  enum ste_cg29xx_dai_channel_selection channel_selection;
  enum ste_cg29xx_dai_sample_rate sample_rate;
  enum ste_cg29xx_dai_word_width_selection word_width;
};

/**
 * union ste_cg29xx_dai_port_conf - DAI port configuration union.
 *  @i2s: The configuration struct for a port supporting only I2S.
 *  @i2s_pcm: The configuration struct for a port supporting both PCM and I2S.
 */
union ste_cg29xx_dai_port_conf {
  struct ste_cg29xx_dai_port_conf_i2s i2s;
  struct ste_cg29xx_dai_port_conf_i2s_pcm i2s_pcm;
};

/**
 * enum ste_cg29xx_dai_ext_port_id - DAI external port id alternatives.
 *  @STE_CG29XX_DAI_PORT_0_I2S: Port id is 0 and it supports only I2S.
 *  @STE_CG29XX_DAI_PORT_1_I2S_PCM: Port id is 1 and it supports both I2S and PCM.
 */
enum ste_cg29xx_dai_ext_port_id {
  STE_CG29XX_DAI_PORT_0_I2S,
  STE_CG29XX_DAI_PORT_1_I2S_PCM
};

/**
 * enum ste_cg29xx_audio_endpoint_id - Audio endpoint id alternatives.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_0_I2S: Internal audio endpoint of the external I2S interface.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_1_I2S_PCM: Internal audio endpoint of the external I2S/PCM interface.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_SLIMBUS_VOICE: Internal audio endpoint of the external Slimbus voice interface. (Currently not supported)
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_SLIMBUS_AUDIO: Internal audio endpoint of the external Slimbus audio interface. (Currently not supported)
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_BT_SCO_INOUT: Bluetooth SCO bidirectional.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_BT_A2DP_SRC: Bluetooth A2DP source.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_BT_A2DP_SNK: Bluetooth A2DP sink.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_FM_RX: FM receive.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_FM_TX: FM transmit.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_ANALOG_OUT: Analog out.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_AUDIO_IN: DSP audio in.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_AUDIO_OUT: DSP audio out.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_VOICE_IN: DSP voice in.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_VOICE_OUT: DSP voice out.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_TONE_IN: DSP tone in.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_BURST_BUFFER_IN: Burst buffer in.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_BURST_BUFFER_OUT: Burst buffer out.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_MUSIC_DECODER: Music decoder.
 *  @STE_CG29XX_AUDIO_ENDPOINT_ID_HCI_AUDIO_IN: HCI audio in.
 */
enum ste_cg29xx_audio_endpoint_id {
  STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_0_I2S,
  STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_1_I2S_PCM,
  STE_CG29XX_AUDIO_ENDPOINT_ID_SLIMBUS_VOICE,
  STE_CG29XX_AUDIO_ENDPOINT_ID_SLIMBUS_AUDIO,
  STE_CG29XX_AUDIO_ENDPOINT_ID_BT_SCO_INOUT,
  STE_CG29XX_AUDIO_ENDPOINT_ID_BT_A2DP_SRC,
  STE_CG29XX_AUDIO_ENDPOINT_ID_BT_A2DP_SNK,
  STE_CG29XX_AUDIO_ENDPOINT_ID_FM_RX,
  STE_CG29XX_AUDIO_ENDPOINT_ID_FM_TX,
  STE_CG29XX_AUDIO_ENDPOINT_ID_ANALOG_OUT,
  STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_AUDIO_IN,
  STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_AUDIO_OUT,
  STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_VOICE_IN,
  STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_VOICE_OUT,
  STE_CG29XX_AUDIO_ENDPOINT_ID_DSP_TONE_IN,
  STE_CG29XX_AUDIO_ENDPOINT_ID_BURST_BUFFER_IN,
  STE_CG29XX_AUDIO_ENDPOINT_ID_BURST_BUFFER_OUT,
  STE_CG29XX_AUDIO_ENDPOINT_ID_MUSIC_DECODER,
  STE_CG29XX_AUDIO_ENDPOINT_ID_HCI_AUDIO_IN
};

/**
 * struct ste_cg29xx_dai_config - Configuration struct for Digital Audio Interface.
 *  @port: The port id to configure. Acts as a discriminator for @conf parameter which is a union.
 *  @conf: The configuration union that contains the parameters for the port.
 */
struct ste_cg29xx_dai_config {
  enum ste_cg29xx_dai_ext_port_id port;
  union ste_cg29xx_dai_port_conf conf;
};



/*
 * Endpoint configuration types
 */

/**
 * enum ste_cg29xx_audio_endpoint_configuration_sample_rate - Audio endpoint configuration sample rate alternatives.
 *  @STE_CG29XX_AUDIO_ENDPOINT_CONFIGURATION_SAMPLE_RATE_8_KHZ: 8 kHz sample rate.
 *  @STE_CG29XX_AUDIO_ENDPOINT_CONFIGURATION_SAMPLE_RATE_16_KHZ: 16 kHz sample rate.
 *  @STE_CG29XX_AUDIO_ENDPOINT_CONFIGURATION_SAMPLE_RATE_44_1_KHZ: 44.1 kHz sample rate.
 *  @STE_CG29XX_AUDIO_ENDPOINT_CONFIGURATION_SAMPLE_RATE_48_KHZ: 48 kHz sample rate.
 */
enum ste_cg29xx_audio_endpoint_configuration_sample_rate {
  STE_CG29XX_AUDIO_ENDPOINT_CONFIGURATION_SAMPLE_RATE_8_KHZ = 0x01,
  STE_CG29XX_AUDIO_ENDPOINT_CONFIGURATION_SAMPLE_RATE_16_KHZ = 0x02,
  STE_CG29XX_AUDIO_ENDPOINT_CONFIGURATION_SAMPLE_RATE_44_1_KHZ = 0x04,
  STE_CG29XX_AUDIO_ENDPOINT_CONFIGURATION_SAMPLE_RATE_48_KHZ = 0x05
};


/**
 * struct ste_cg29xx_audio_endpoint_configuration_a2dp_src - Contains the A2DP source audio endpoint configurations.
 *  @sample_rate: Sample rate.
 *  @channel_count: Number of channels.
 */
struct ste_cg29xx_audio_endpoint_configuration_a2dp_src {
  enum ste_cg29xx_audio_endpoint_configuration_sample_rate sample_rate;
  unsigned int channel_count;
};

/**
 * struct ste_cg29xx_audio_endpoint_configuration_fm - Contains the configuration parameters for an FM endpoint.
 *  @sample_rate: The sample rate alternatives for the FM audio endpoints.
 */
struct ste_cg29xx_audio_endpoint_configuration_fm {
  enum ste_cg29xx_audio_endpoint_configuration_sample_rate sample_rate;
};


/**
 * struct ste_cg29xx_audio_endpoint_configuration_sco_in_out - SCO audio endpoint configuration structure.
 *  @sample_rate: Sample rate, valid values are
 *  * STE_CG29XX_AUDIO_ENDPOINT_CONFIGURATION_SAMPLE_RATE_8_KHZ
 *  * STE_CG29XX_AUDIO_ENDPOINT_CONFIGURATION_SAMPLE_RATE_16_KHZ
 */
struct ste_cg29xx_audio_endpoint_configuration_sco_in_out {
  enum ste_cg29xx_audio_endpoint_configuration_sample_rate sample_rate;
};

/**
 * union ste_cg29xx_audio_endpoint_configuration - Contains the alternatives for different audio endpoint configurations.
 *  @sco: SCO audio endpoint configuration structure.
 *  @a2dp_src: A2DP source audio endpoint configuration structure.
 *  @fm: FM audio endpoint configuration structure.
 */
union ste_cg29xx_audio_endpoint_configuration_union {
  struct ste_cg29xx_audio_endpoint_configuration_sco_in_out sco;
  struct ste_cg29xx_audio_endpoint_configuration_a2dp_src a2dp_src;
  struct ste_cg29xx_audio_endpoint_configuration_fm fm;
};

/**
 * struct ste_cg29xx_audio_endpoint_configuration - Contains the audio endpoint configuration.
 *  @endpoint_id: Identifies the audio endpoint. Works as a discriminator for the config union.
 *  @config: Union holding the configuration parameters for the endpoint.
 */
struct ste_cg29xx_audio_endpoint_configuration {
  enum ste_cg29xx_audio_endpoint_id endpoint_id;
  union ste_cg29xx_audio_endpoint_configuration_union config;
};



/*
 * ste cg29xx audio control driver interfaces methods
 */

/**
 * ste_cg29xx_audio_open() - Opens a session to the ST-Ericsson CG29XX Audio control interface.
 *  @session: [out] Address where to store the session identifier. Allocated by caller, may not be NULL.
 *
 * Opens a session to the ST-Ericsson CG29XX Audio control interface.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL upon bad input parameter.
 *   -ENOMEM upon allocation failure.
 *   -EMFILE if no more user session could be opened.
 *   -EIO upon failure to register to STE_CONN.
 */
int ste_cg29xx_audio_open(unsigned int *session);

/**
 * ste_cg29xx_audio_close() - Closes an opened session to the ST-Ericsson CG29XX audio control interface.
 *  @session: Pointer to session identifier to close. Will be null after this call.
 *
 * Closes an opened session to the ST-Ericsson CG29XX audio control interface.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL upon bad input parameter.
 *   -EIO if driver has not been opened.
 *   -EACCES if session has not opened.
 */
int ste_cg29xx_audio_close(unsigned int *session);

/**
 * ste_cg29xx_audio_set_dai_configuration() -  Sets the Digital Audio Interface configuration.
 *  @session: Session identifier this call is related to.
 *  @config: [in] Pointer to the configuration to set.  Allocated by caller, may not be NULL.
 *
 * Sets the Digital Audio Interface configuration. The DAI is the external interface between
 * the combo chip and the platform. For example the PCM or I2S interface.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL upon bad input parameter.
 *   -EIO if driver has not been opened.
 *   -ENOMEM upon allocation failure.
 *   -EACCES if trying to set unsupported configuration.
 *   Errors from @receive_bt_cmd_complete.
 */
int ste_cg29xx_audio_set_dai_configuration(unsigned int session,
					struct ste_cg29xx_dai_config *config);

/**
 * ste_cg29xx_audio_get_dai_configuration() - Gets the current Digital Audio Interface configuration.
 *  @session: Session identifier this call is related to.
 *  @config: [out] Pointer to the configuration to get.  Allocated by caller, may not be NULL.
 *
 * Gets the current Digital Audio Interface configuration. Currently this method can only be called after
 * some one has called ste_cg29xx_audio_set_dai_configuration(), there is today no way of getting the static
 * settings file parameters from this method.
 * Note that the @port parameter within @config must be set when calling this
 * function so that the ST-Ericsson CG29XX Audio driver will know which configuration to
 * return.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL upon bad input parameter.
 *   -EIO if driver has not been opened or configuration has not been set.
 */
int ste_cg29xx_audio_get_dai_configuration(unsigned int session,
					struct ste_cg29xx_dai_config *config);

/**
 * ste_cg29xx_audio_configure_endpoint() - Configures one endpoint in the combo chip's audio system.
 *  @session: Session identifier this call is related to.
 *  @configuration: [in] Pointer to the endpoint's configuration structure.
 *
 * Configures one endpoint in the combo chip's audio system.
 *
 * Supported values are:
 *   * STE_CG29XX_AUDIO_ENDPOINT_ID_BT_SCO_INOUT
 *   * STE_CG29XX_AUDIO_ENDPOINT_ID_BT_A2DP_SRC
 *   * STE_CG29XX_AUDIO_ENDPOINT_ID_FM_RX
 *   * STE_CG29XX_AUDIO_ENDPOINT_ID_FM_TX
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL upon bad input parameter.
 *   -EIO if driver has not been opened.
 *   -EACCES if supplied ste_cg29xx_dai_config struct contains not supported endpoint_id.
 */
int ste_cg29xx_audio_configure_endpoint(unsigned int session,
					struct ste_cg29xx_audio_endpoint_configuration *configuration);

/**
 * ste_cg29xx_audio_connect_and_start_stream() - Connects two endpoints and starts the audio stream.
 *  @session: Session identifier this call is related to.
 *  @endpoint_1: One of the endpoints, no relation to direction or role.
 *  @endpoint_2: The other endpoint, no relation to direction or role.
 *  @stream_handle: [out] Pointer where to store the stream handle. Allocated by caller, may not be NULL.
 *
 * Connects two endpoints and starts the audio stream.
 * Note that the endpoints need to be configured before the stream is started; DAI endpoints,
 * such as STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_0_I2S, are configured through
 * @ste_cg29xx_audio_set_dai_configuration() while other endpoints are configured through
 * @ste_cg29xx_audio_configure_endpoint().
 *
 * Supported endpoint_id values are:
 *   * STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_0_I2S
 *   * STE_CG29XX_AUDIO_ENDPOINT_ID_PORT_1_I2S_PCM
 *   * STE_CG29XX_AUDIO_ENDPOINT_ID_BT_SCO_INOUT
 *   * STE_CG29XX_AUDIO_ENDPOINT_ID_FM_RX
 *   * STE_CG29XX_AUDIO_ENDPOINT_ID_FM_TX
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL upon bad input parameter or unsupported configuration.
 *   -EIO if driver has not been opened.
 *   Errors from @conn_start_i2s_to_fm_rx, @conn_start_i2s_to_fm_tx, and @conn_start_pcm_to_sco.
 */
int ste_cg29xx_audio_connect_and_start_stream(unsigned int session,
					enum ste_cg29xx_audio_endpoint_id endpoint_1,
					enum ste_cg29xx_audio_endpoint_id endpoint_2,
					unsigned int *stream_handle);

/**
 * ste_cg29xx_audio_stop_stream() - Stops a stream and disconnects the endpoints.
 *  @session: Session identifier this call is related to.
 *  @stream_handle: Handle to the stream to stop.
 *
 * Stops a stream and disconnects the endpoints.
 *
 * Returns:
 *   0 if there is no error.
 *   -EINVAL upon bad input parameter.
 *   -EIO if driver has not been opened.
 */
int ste_cg29xx_audio_stop_stream(unsigned int session,
				unsigned int stream_handle);

#endif /* _STE_CG29XX_AUDIO_H_ */
