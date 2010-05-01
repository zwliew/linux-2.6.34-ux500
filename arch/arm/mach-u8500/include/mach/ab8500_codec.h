/*****************************************************************************/
/**
*  © ST-Ericsson, 2009 - All rights reserved
* Reproduction and Communication of this document is strictly prohibited
* unless specifically authorized in writing by ST-Ericsson
*
* \brief   Public header file for AB8500 Codec
* \author  ST-Ericsson
*/
/*****************************************************************************/

#ifndef _AB8500_CODEC_H_
#define _AB8500_CODEC_H_

/*---------------------------------------------------------------------
 * Includes
 *--------------------------------------------------------------------*/
#include "hcl_defs.h"
#include "debug.h"

/*---------------------------------------------------------------------
 * Define
 *--------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
typedef enum
{
    AB8500_CODEC_OK,
    AB8500_CODEC_ERROR,
    AB8500_CODEC_UNSUPPORTED_FEATURE,
    AB8500_CODEC_INVALID_PARAMETER,
    AB8500_CODEC_CONFIG_NOT_COHERENT,
    AB8500_CODEC_TRANSACTION_FAILED
} t_ab8500_codec_error;


typedef enum
{
    AB8500_CODEC_DIRECTION_IN,
    AB8500_CODEC_DIRECTION_OUT,
    AB8500_CODEC_DIRECTION_INOUT
} t_ab8500_codec_direction;

typedef enum
{
    AB8500_CODEC_MASTER_MODE_DISABLE,
    AB8500_CODEC_MASTER_MODE_ENABLE
}t_ab8500_codec_master_mode;

typedef enum
{
    AB8500_CODEC_AUDIO_INTERFACE_0,
    AB8500_CODEC_AUDIO_INTERFACE_1
}t_ab8500_codec_audio_interface;


typedef enum
{
    AB8500_CODEC_MODE_HIFI,
    AB8500_CODEC_MODE_VOICE,
    AB8500_CODEC_MODE_MANUAL_SETTING
} t_ab8500_codec_mode;

typedef enum
{
    AB8500_CODEC_DEST_HEADSET,
    AB8500_CODEC_DEST_EARPIECE,
    AB8500_CODEC_DEST_HANDSFREE,
    AB8500_CODEC_DEST_VIBRATOR_L,
    AB8500_CODEC_DEST_VIBRATOR_R,
    AB8500_CODEC_DEST_ALL
} t_ab8500_codec_dest;

typedef enum
{
    AB8500_CODEC_SRC_LINEIN,
    AB8500_CODEC_SRC_MICROPHONE_1A,
    AB8500_CODEC_SRC_MICROPHONE_1B,
    AB8500_CODEC_SRC_MICROPHONE_2,
    AB8500_CODEC_SRC_D_MICROPHONE_1,
    AB8500_CODEC_SRC_D_MICROPHONE_2,
    AB8500_CODEC_SRC_D_MICROPHONE_3,
    AB8500_CODEC_SRC_D_MICROPHONE_4,
    AB8500_CODEC_SRC_D_MICROPHONE_5,
    AB8500_CODEC_SRC_D_MICROPHONE_6,
    AB8500_CODEC_SRC_ALL
} t_ab8500_codec_src;

typedef enum
{
    AB8500_CODEC_SLOT0,
    AB8500_CODEC_SLOT1,
    AB8500_CODEC_SLOT2,
    AB8500_CODEC_SLOT3,
    AB8500_CODEC_SLOT4,
    AB8500_CODEC_SLOT5,
    AB8500_CODEC_SLOT6,
    AB8500_CODEC_SLOT7,
    AB8500_CODEC_SLOT8,
    AB8500_CODEC_SLOT9,
    AB8500_CODEC_SLOT10,
    AB8500_CODEC_SLOT11,
    AB8500_CODEC_SLOT12,
    AB8500_CODEC_SLOT13,
    AB8500_CODEC_SLOT14,
    AB8500_CODEC_SLOT15,
    AB8500_CODEC_SLOT16,
    AB8500_CODEC_SLOT17,
    AB8500_CODEC_SLOT18,
    AB8500_CODEC_SLOT19,
    AB8500_CODEC_SLOT20,
    AB8500_CODEC_SLOT21,
    AB8500_CODEC_SLOT22,
    AB8500_CODEC_SLOT23,
    AB8500_CODEC_SLOT24,
    AB8500_CODEC_SLOT25,
    AB8500_CODEC_SLOT26,
    AB8500_CODEC_SLOT27,
    AB8500_CODEC_SLOT28,
    AB8500_CODEC_SLOT29,
    AB8500_CODEC_SLOT30,
    AB8500_CODEC_SLOT31,
	AB8500_CODEC_SLOT_UNDEFINED
} t_ab8500_codec_slot;



typedef enum
{
    AB8500_CODEC_DA_CHANNEL_NUMBER_1,
    AB8500_CODEC_DA_CHANNEL_NUMBER_2,
    AB8500_CODEC_DA_CHANNEL_NUMBER_3,
    AB8500_CODEC_DA_CHANNEL_NUMBER_4,
    AB8500_CODEC_DA_CHANNEL_NUMBER_5,
    AB8500_CODEC_DA_CHANNEL_NUMBER_6,
	AB8500_CODEC_DA_CHANNEL_NUMBER_UNDEFINED
}t_ab8500_codec_da_channel_number;


typedef enum
{
    AB8500_CODEC_CR31_TO_CR46_SLOT_OUTPUTS_DATA_FROM_AD_OUT1,
    AB8500_CODEC_CR31_TO_CR46_SLOT_OUTPUTS_DATA_FROM_AD_OUT2,
    AB8500_CODEC_CR31_TO_CR46_SLOT_OUTPUTS_DATA_FROM_AD_OUT3,
    AB8500_CODEC_CR31_TO_CR46_SLOT_OUTPUTS_DATA_FROM_AD_OUT4,
    AB8500_CODEC_CR31_TO_CR46_SLOT_OUTPUTS_DATA_FROM_AD_OUT5,
    AB8500_CODEC_CR31_TO_CR46_SLOT_OUTPUTS_DATA_FROM_AD_OUT6,
    AB8500_CODEC_CR31_TO_CR46_SLOT_OUTPUTS_DATA_FROM_AD_OUT7,
    AB8500_CODEC_CR31_TO_CR46_SLOT_OUTPUTS_DATA_FROM_AD_OUT8,
    AB8500_CODEC_CR31_TO_CR46_SLOT_OUTPUTS_ZEROS,
    AB8500_CODEC_CR31_TO_CR46_SLOT_IS_TRISTATE = 15,
	AB8500_CODEC_CR31_TO_CR46_SLOT_OUTPUTS_DATA_FROM_AD_UNDEFINED
} t_ab8500_codec_cr31_to_cr46_ad_data_allocation;


typedef enum
{
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT00,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT01,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT02,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT03,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT04,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT05,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT06,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT07,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT08,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT09,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT10,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT11,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT12,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT13,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT14,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT15,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT16,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT17,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT18,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT19,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT20,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT21,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT22,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT23,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT24,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT25,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT26,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT27,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT28,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT29,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT30,
    AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT31,
	AB8500_CODEC_CR51_TO_CR56_SLTODA_SLOT_UNDEFINED
} t_ab8500_codec_cr51_to_cr56_sltoda;

typedef enum
{
    AB8500_CODEC_SRC_STATE_DISABLE,
    AB8500_CODEC_SRC_STATE_ENABLE
}t_ab8500_codec_src_state;

typedef enum
{
    AB8500_CODEC_DEST_STATE_DISABLE,
    AB8500_CODEC_DEST_STATE_ENABLE
}t_ab8500_codec_dest_state;

/* CR104 - 5:0 */
typedef t_uint8 t_ab8500_codec_cr104_bfifoint;


/* CR105 - 7:0 */
typedef t_uint8 t_ab8500_codec_cr105_bfifotx;

/* CR106 - 6:4 */
typedef enum
{
    AB8500_CODEC_CR106_BFIFOFSEXT_NO_EXTRA_CLK,
    AB8500_CODEC_CR106_BFIFOFSEXT_1SLOT_EXTRA_CLK,
    AB8500_CODEC_CR106_BFIFOFSEXT_2SLOT_EXTRA_CLK,
    AB8500_CODEC_CR106_BFIFOFSEXT_3SLOT_EXTRA_CLK,
    AB8500_CODEC_CR106_BFIFOFSEXT_4SLOT_EXTRA_CLK,
    AB8500_CODEC_CR106_BFIFOFSEXT_5SLOT_EXTRA_CLK,
    AB8500_CODEC_CR106_BFIFOFSEXT_6SLOT_EXTRA_CLK
} t_ab8500_codec_cr106_bfifofsext;

/* CR106 - 2 */
typedef enum
{
    AB8500_CODEC_CR106_BFIFOMSK_AD_DATA0_UNMASKED,
    AB8500_CODEC_CR106_BFIFOMSK_AD_DATA0_MASKED
} t_ab8500_codec_cr106_bfifomsk;

/* CR106 - 1 */
typedef enum
{
    AB8500_CODEC_CR106_BFIFOMSTR_SLAVE_MODE,
    AB8500_CODEC_CR106_BFIFOMSTR_MASTER_MODE
} t_ab8500_codec_cr106_bfifomstr;

/* CR106 - 0 */
typedef enum
{
    AB8500_CODEC_CR106_BFIFOSTRT_STOPPED,
    AB8500_CODEC_CR106_BFIFOSTRT_RUNNING
} t_ab8500_codec_cr106_bfifostrt;


/* CR107 - 7:0 */
typedef t_uint8 t_ab8500_codec_cr107_bfifosampnr;


/* CR108 - 7:0 */
typedef t_uint8 t_ab8500_codec_cr108_bfifowakeup;


typedef struct
{
    t_ab8500_codec_cr104_bfifoint                    cr104_bfifoint;
	t_ab8500_codec_cr105_bfifotx                     cr105_bfifotx;
	t_ab8500_codec_cr106_bfifofsext                  cr106_bfifofsext;
	t_ab8500_codec_cr106_bfifomsk                    cr106_bfifomsk;
	t_ab8500_codec_cr106_bfifomstr                   cr106_bfifomstr;
	t_ab8500_codec_cr106_bfifostrt                   cr106_bfifostrt;
	t_ab8500_codec_cr107_bfifosampnr                 cr107_bfifosampnr;
	t_ab8500_codec_cr108_bfifowakeup                 cr108_bfifowakeup;
} t_ab8500_codec_burst_fifo_config;

/************************************************************/
/*---------------------------------------------------------------------
 * Exported APIs
 *--------------------------------------------------------------------*/
/* Initialization */
t_ab8500_codec_error   AB8500_CODEC_Init(IN t_uint8 slave_address_of_codec);
t_ab8500_codec_error   AB8500_CODEC_Reset(void);

/* Audio Codec basic configuration */
t_ab8500_codec_error   AB8500_CODEC_SetModeAndDirection(IN t_ab8500_codec_direction ab8500_codec_direction, IN t_ab8500_codec_mode ab8500_codec_mode_in, IN t_ab8500_codec_mode ab8500_codec_mode_out);
t_ab8500_codec_error   AB8500_CODEC_SelectInput(IN t_ab8500_codec_src ab8500_codec_src);
t_ab8500_codec_error   AB8500_CODEC_SelectOutput(IN t_ab8500_codec_dest ab8500_codec_dest);

/* Burst FIFO configuration */
t_ab8500_codec_error AB8500_CODEC_ConfigureBurstFifo(IN t_ab8500_codec_burst_fifo_config const *const p_burst_fifo_config);
t_ab8500_codec_error AB8500_CODEC_EnableBurstFifo(void);
t_ab8500_codec_error AB8500_CODEC_DisableBurstFifo(void);

/* Audio Codec Master mode configuration */
t_ab8500_codec_error AB8500_CODEC_SetMasterMode(IN t_ab8500_codec_master_mode mode);

/* APIs to be implemented by user */
t_ab8500_codec_error AB8500_CODEC_Write(IN t_uint8 register_offset, IN t_uint8 count, IN t_uint8 *p_data);
t_ab8500_codec_error AB8500_CODEC_Read(IN t_uint8 register_offset, IN t_uint8 count, IN t_uint8 *p_dummy_data, IN t_uint8 *p_data);

/* Volume Management */
t_ab8500_codec_error   AB8500_CODEC_SetSrcVolume(IN t_ab8500_codec_src src_device, IN t_uint8 in_left_volume, IN t_uint8 in_right_volume);
t_ab8500_codec_error   AB8500_CODEC_SetDestVolume(IN t_ab8500_codec_dest dest_device, IN t_uint8 out_left_volume, IN t_uint8 out_right_volume);

/* Power management */
t_ab8500_codec_error   AB8500_CODEC_PowerDown(void);
t_ab8500_codec_error   AB8500_CODEC_PowerUp(void);

/* Interface Management */
t_ab8500_codec_error   AB8500_CODEC_SelectInterface(IN t_ab8500_codec_audio_interface audio_interface);
t_ab8500_codec_error 	AB8500_CODEC_GetInterface(OUT t_ab8500_codec_audio_interface *p_audio_interface);

/* Slot Allocation */
t_ab8500_codec_error AB8500_CODEC_ADSlotAllocation(IN t_ab8500_codec_slot ad_slot, IN t_ab8500_codec_cr31_to_cr46_ad_data_allocation value);
t_ab8500_codec_error AB8500_CODEC_DASlotAllocation(IN t_ab8500_codec_da_channel_number channel_number, IN t_ab8500_codec_cr51_to_cr56_sltoda slot);

/* Loopback Management */
t_ab8500_codec_error AB8500_CODEC_SetAnalogLoopback(IN t_uint8 out_left_volume, IN t_uint8 out_right_volume);
t_ab8500_codec_error AB8500_CODEC_RemoveAnalogLoopback(void);

/* Bypass Management */
t_ab8500_codec_error AB8500_CODEC_EnableBypassMode(void);
t_ab8500_codec_error AB8500_CODEC_DisableBypassMode(void);

/* Power Control Management */
t_ab8500_codec_error AB8500_CODEC_SrcPowerControl(IN t_ab8500_codec_src src_device, t_ab8500_codec_src_state state);
t_ab8500_codec_error AB8500_CODEC_DestPowerControl(IN t_ab8500_codec_dest dest_device, t_ab8500_codec_dest_state state);

/* Version Management */
t_ab8500_codec_error   AB8500_CODEC_GetVersion(OUT t_version *p_version);

#if 0
/* Debug management */
t_ab8500_codec_error   AB8500_CODEC_SetDbgLevel(IN t_dbg_level dbg_level);
t_ab8500_codec_error   AB8500_CODEC_GetDbgLevel(OUT t_dbg_level *p_dbg_level);
#endif

/*
** following is added by $kardad$
*/

/* duplicate copy of enum from msp.h */
/* for MSPConfiguration.in_clock_freq parameter to select msp clock freq */
typedef enum {
    CODEC_MSP_INPUT_FREQ_1MHZ = 1024,
    CODEC_MSP_INPUT_FREQ_2MHZ = 2048,
    CODEC_MSP_INPUT_FREQ_3MHZ = 3072,
    CODEC_MSP_INPUT_FREQ_4MHZ = 4096,
    CODEC_MSP_INPUT_FREQ_5MHZ = 5760,
    CODEC_MSP_INPUT_FREQ_6MHZ = 6144,
    CODEC_MSP_INPUT_FREQ_8MHZ = 8192,
    CODEC_MSP_INPUT_FREQ_11MHZ = 11264,
    CODEC_MSP_INPUT_FREQ_12MHZ = 12288,
    CODEC_MSP_INPUT_FREQ_16MHZ = 16384,
    CODEC_MSP_INPUT_FREQ_22MHZ = 22579,
    CODEC_MSP_INPUT_FREQ_24MHZ = 24576,
    CODEC_MSP_INPUT_FREQ_48MHZ = 49152
} codec_msp_in_clock_freq_type;

/* msp clock source internal/external for srg_clock_sel */
typedef enum {
    CODEC_MSP_APB_CLOCK = 0,
    CODEC_MSP_SCK_CLOCK = 2,
    CODEC_MSP_SCK_SYNC_CLOCK = 3
} codec_msp_srg_clock_sel_type;

/* Sample rate supported by Codec */

typedef enum {
	CODEC_FREQUENCY_DONT_CHANGE = -100,
	CODEC_SAMPLING_FREQ_RESET = -1,
	CODEC_SAMPLING_FREQ_MINLIMIT = 7,
	CODEC_SAMPLING_FREQ_8KHZ = 8,	/*default */
	CODEC_SAMPLING_FREQ_11KHZ = 11,
	CODEC_SAMPLING_FREQ_12KHZ = 12,
	CODEC_SAMPLING_FREQ_16KHZ = 16,
	CODEC_SAMPLING_FREQ_22KHZ = 22,
	CODEC_SAMPLING_FREQ_24KHZ = 24,
	CODEC_SAMPLING_FREQ_32KHZ = 32,
	CODEC_SAMPLING_FREQ_44KHZ = 44,
	CODEC_SAMPLING_FREQ_48KHZ = 48,
	CODEC_SAMPLING_FREQ_64KHZ = 64,	/*the frequencies below this line are not supported in stw5094A */
	CODEC_SAMPLING_FREQ_88KHZ = 88,
	CODEC_SAMPLING_FREQ_96KHZ = 96,
	CODEC_SAMPLING_FREQ_128KHZ = 128,
	CODEC_SAMPLING_FREQ_176KHZ = 176,
	CODEC_SAMPLING_FREQ_192KHZ = 192,
	CODEC_SAMPLING_FREQ_MAXLIMIT = 193
} t_codec_sample_frequency;

#define RESET       -1
#define DEFAULT     -100
/***********************************************************/
/*
** following stuff is added to compile code without debug print support $kardad$
*/

#define DBGEXIT(cr)
#define DBGEXIT0(cr)
#define DBGEXIT1(cr,ch,p1)
#define DBGEXIT2(cr,ch,p1,p2)
#define DBGEXIT3(cr,ch,p1,p2,p3)
#define DBGEXIT4(cr,ch,p1,p2,p3,p4)
#define DBGEXIT5(cr,ch,p1,p2,p3,p4,p5)
#define DBGEXIT6(cr,ch,p1,p2,p3,p4,p5,p6)

#define DBGENTER()
#define DBGENTER0()
#define DBGENTER1(ch,p1)
#define DBGENTER2(ch,p1,p2)
#define DBGENTER3(ch,p1,p2,p3)
#define DBGENTER4(ch,p1,p2,p3,p4)
#define DBGENTER5(ch,p1,p2,p3,p4,p5)
#define DBGENTER6(ch,p1,p2,p3,p4,p5,p6)

#define DBGPRINT(dbg_level,dbg_string)
#define DBGPRINTHEX(dbg_level,dbg_string,uint32)
#define DBGPRINTDEC(dbg_level,dbg_string,uint32)
/***********************************************************/

/*---------------------------------------------------------------------
 * PRIVATE APIs
 *--------------------------------------------------------------------*/
PRIVATE t_ab8500_codec_error ab8500_codec_ADSlotAllocationSwitch1(IN t_ab8500_codec_slot ad_slot, IN t_ab8500_codec_cr31_to_cr46_ad_data_allocation value);
PRIVATE t_ab8500_codec_error ab8500_codec_ADSlotAllocationSwitch2(IN t_ab8500_codec_slot ad_slot, IN t_ab8500_codec_cr31_to_cr46_ad_data_allocation value);
PRIVATE t_ab8500_codec_error ab8500_codec_ADSlotAllocationSwitch3(IN t_ab8500_codec_slot ad_slot, IN t_ab8500_codec_cr31_to_cr46_ad_data_allocation value);
PRIVATE t_ab8500_codec_error ab8500_codec_ADSlotAllocationSwitch4(IN t_ab8500_codec_slot ad_slot, IN t_ab8500_codec_cr31_to_cr46_ad_data_allocation value);
PRIVATE t_ab8500_codec_error ab8500_codec_SrcPowerControlSwitch1(IN t_ab8500_codec_src src_device, t_ab8500_codec_src_state state);
PRIVATE t_ab8500_codec_error ab8500_codec_SrcPowerControlSwitch2(IN t_ab8500_codec_src src_device, t_ab8500_codec_src_state state);
PRIVATE t_ab8500_codec_error ab8500_codec_SetModeAndDirectionUpdateCR(void);
PRIVATE t_ab8500_codec_error ab8500_codec_SetSrcVolumeUpdateCR(void);
PRIVATE t_ab8500_codec_error ab8500_codec_SetDestVolumeUpdateCR(void);
PRIVATE t_ab8500_codec_error ab8500_codec_ProgramDirectionIN(void);
PRIVATE t_ab8500_codec_error ab8500_codec_ProgramDirectionOUT(void);
PRIVATE t_ab8500_codec_error ab8500_codec_DestPowerControlUpdateCR(void);

#ifdef __cplusplus
}   /* allow C++ to use these headers*/
#endif /* __cplusplus*/
#endif /* _AB8500_CODEC_H_*/

/* End of file ab8500_codec.h*/

