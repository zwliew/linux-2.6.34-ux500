/*
 * Copyright ST-Ericsson 2009.
 *
 * Author: Srinidhi Kasagar <srinidhi.kasagar@stericsson.com>
 * Licensed under GPLv2.
 */
#ifndef _AB8500_H
#define _AB8500_H

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <mach/hardware.h>
#include <asm/dma.h>
#include <mach/dma.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-stm.h>
/*
 * AB8500 bank addresses
 */
#define AB8500_SYS_CTRL1_BLOCK	0x1
#define AB8500_SYS_CTRL2_BLOCK	0x2
#define AB8500_REGU_CTRL1	0x3
#define AB8500_REGU_CTRL2	0x4
#define AB8500_USB		0x5
#define AB8500_TVOUT		0x6
#define AB8500_DBI		0x7
#define AB8500_ECI_AV_ACC	0x8
#define AB8500_RESERVED	0x9
#define STw4550_GPADC		0xA
#define AB8500_CHARGER		0xB
#define AB8500_GAS_GAUGE	0xC
#define AB8500_AUDIO		0xD
#define AB8500_INTERRUPT	0xE
#define AB8500_RTC		0xF
#define AB8500_MISC		0x10
#define AB8500_DEBUG		0x12
#define AB8500_PROD_TEST	0x13
#define AB8500_OTP_EMUL	0x15

/*
 * System control 1 register offsets.
 * Bank = 0x01
 */
#define AB8500_TURNON_STAT_REG		0x0100
#define AB8500_RESET_STAT_REG		0x0101
#define AB8500_PONKEY1_PRESS_STAT_REG	0x0102

#define AB8500_FSM_STAT1_REG		0x0140
#define AB8500_FSM_STAT2_REG		0x0141
#define AB8500_SYSCLK_REQ_STAT_REG	0x0142
#define AB8500_USB_STAT1_REG		0x0143
#define AB8500_USB_STAT2_REG		0x0144
#define AB8500_STATUS_SPARE1_REG	0x0145
#define AB8500_STATUS_SPARE2_REG	0x0146

#define AB8500_CTRL1_REG		0x0180
#define AB8500_CTRL2_REG		0x0181

/*
 * System control 2 register offsets.
 * bank = 0x02
 */
#define AB8500_CTRL3_REG		0x0200
#define AB8500_MAIN_WDOG_CTRL_REG	0x0201
#define AB8500_MAIN_WDOG_TIMER_REG	0x0202
#define AB8500_LOW_BAT_REG		0x0203
#define AB8500_BATT_OK_REG		0x0204
#define AB8500_SYSCLK_TIMER_REG	0x0205
#define AB8500_SMPSCLK_CTRL_REG	0x0206
#define AB8500_SMPSCLK_SEL1_REG	0x0207
#define AB8500_SMPSCLK_SEL2_REG	0x0208
#define AB8500_SMPSCLK_SEL3_REG	0x0209
#define AB8500_SYSULPCLK_CONF_REG	0x020A
#define AB8500_SYSULPCLK_CTRL1_REG	0x020B
#define AB8500_SYSCLK_CTRL_REG		0x020C
#define AB8500_SYSCLK_REQ1_VALID_REG	0x020D
#define AB8500_SYSCLK_REQ_VALID_REG	0x020E
#define AB8500_SYSCTRL_SPARE_REG	0x020F
#define AB8500_PAD_CONF_REG		0x0210

/*
 * Regu control1 register offsets (SPI)
 * Bank = 0x03
 */
#define AB8500_REGU_SERIAL_CTRL1_REG	0x0300
#define AB8500_REGU_SERIAL_CTRL2_REG	0x0301
#define AB8500_REGU_SERIAL_CTRL3_REG	0x0302
#define AB8500_REGU_REQ_CTRL1_REG	0x0303
#define AB8500_REGU_REQ_CTRL2_REG	0x0304
#define AB8500_REGU_REQ_CTRL3_REG	0x0305
#define AB8500_REGU_REQ_CTRL4_REG	0x0306
#define AB8500_REGU_SYSCLK_REQ1HP_VALID1_REG	0x0307
#define AB8500_REGU_SYSCLK_REQ1HP_VALID2_REG	0x0308
#define AB8500_REGU_HWHPREQ1_VALID1_REG	0x0309
#define AB8500_REGU_HWHPREQ1_VALID2_REG	0x030A
#define AB8500_REGU_HWHPREQ2_VALID1_REG	0x030B
#define AB8500_REGU_HWHPREQ2_VALID2_REG	0x030C
#define AB8500_REGU_SWHPREQ_VALID1_REG	0x030D
#define AB8500_REGU_SWHPREQ_VALID2_REG	0x030E

#define AB8500_REGU_SYSCLK_REQ1_VALID_REG	0x030F /* only for ED*/
#define AB8500_REGU_SYSCLK_REQ2_VALID_REG	0x0310	/*only for ED*/

#define AB8500_REGU_MISC1_REG		0x0380
#define AB8500_REGU_OTGSUPPLY_CTRL_REG	0x0381
#define AB8500_REGU_VUSB_CTRL_REG	0x0382 /* see reg manaul*/
#define AB8500_REGU_VAUDIO_SUPPLY_REG	0x0383
#define AB8500_REGU_CTRL1_SPARE_REG	0x0384

 /*
 * Regu control2 register offsets (SPI/APE I2C)
 * Bank = 0x04
 */
#define AB8500_REGU_ARM_REGU1_REG	0x0400
#define AB8500_REGU_ARM_REGU2_REG	0x0401
#define AB8500_REGU_VAPE_REGU_REG	0x0402
#define AB8500_REGU_VSMPS1_REGU_REG	0x0403
#define AB8500_REGU_VSMPS2_REGU_REG	0x0404
#define AB8500_REGU_VSMPS3_REGU_REG	0x0405
#define AB8500_REGU_VPLLVANA_REGU_REG	0x0406
#define AB8500_REGU_VREF_DDR_REG	0x0407
#define AB8500_REGU_EXTSUPPLY_REGU_REG	0x0408
#define AB8500_REGU_VAUX12_REGU_REG	0x0409
#define AB8500_REGU_VRF1VAUX3_REGU_REG	0x040A
#define AB8500_REGU_VARM_SEL1_REG	0x040B
#define AB8500_REGU_VARM_SEL2_REG	0x040C
#define AB8500_REGU_VARM_SEL3_REG	0x040D
#define AB8500_REGU_VAPE_SEL1_REG	0x040E
#define AB8500_REGU_VAPE_SEL2_REG	0x040F
#define AB8500_REGU_VAPE_SEL3_REG	0x0410
#define AB8500_REGU_VBB_SEL2_REG	0x0412
#define AB8500_REGU_VSMPS1_SEL1_REG	0x0413
#define AB8500_REGU_VSMPS1_SEL2_REG	0x0414
#define AB8500_REGU_VSMPS1_SEL3_REG	0x0415
#define AB8500_REGU_VSMPS2_SEL1_REG	0x0417
#define AB8500_REGU_VSMPS2_SEL2_REG	0x0418
#define AB8500_REGU_VSMPS2_SEL3_REG	0x0419
#define AB8500_REGU_VSMPS3_SEL1_REG	0x041B
#define AB8500_REGU_VSMPS3_SEL2_REG	0x041C
#define AB8500_REGU_VSMPS3_SEL3_REG	0x041D
#define AB8500_REGU_VAUX1_SEL_REG	0x041F
#define AB8500_REGU_VAUX2_SEL_REG	0x0420
#define AB8500_REGU_VRF1VAUX3_SEL_REG	0x0421
#define AB8500_REGU_CTRL2_SPARE_REG	0x0422

/*
 * Regu control2 Vmod register offsets
 */
#define AB8500_REGU_VMOD_REGU_REG	0x0440
#define AB8500_REGU_VMOD_SEL1_REG	0x0441
#define AB8500_REGU_VMOD_SEL2_REG	0x0442
#define AB8500_REGU_CTRL_DISCH_REG	0x0443
#define AB8500_REGU_CTRL_DISCH2_REG	0x0444

/*
 * Sim control register offsets
 * Bank:0x4
 */
#define AB8500_SIM_REG1_SGR1L_REG		0x0480
#define AB8500_SIM_REG1_SGR1U_REG		0x0481
#define AB8500_SIM_REG2_SCR1L_REG		0x0482
#define AB8500_SIM_REG2_SCR1U_REG		0x0483
#define AB8500_SIM_REG3_SCTRLRL_REG		0x0484
#define AB8500_SIM_REG3_SCTRLRU_REG		0x0485
#define AB8500_SIM_ISOUICCINT_SRC_REG		0x0486
#define AB8500_SIM_ISOUICCINT_LATCH_REG	0x0487
#define AB8500_SIM_ISOUICCINT_MASK_REG		0x0488
#define AB8500_SIM_REG4_USBUICC_REG		0x0489
#define AB8500_SIM_SDELAYSEL_REG		0x048A
#define AB8500_SIM_USBUICC_CTRL		0x048B	/* bit 3 only for ED */

/*
 * USB/ULPI register offsets
 * Bank : 0x5
 */
#define AB8500_USB_LINE_STAT_REG	0x0580
#define AB8500_USB_LINE_CTRL1_REG	0x0581
#define AB8500_USB_LINE_CTRL2_REG	0x0582
#define AB8500_USB_LINE_CTRL3_REG	0x0583
#define AB8500_USB_LINE_CTRL4_REG	0x0584
#define AB8500_USB_LINE_CTRL5_REG	0x0585
#define AB8500_USB_OTG_CTRL_REG	0x0587
#define AB8500_USB_OTG_STAT_REG	0x0588
#define AB8500_USB_OTG_STAT_REG	0x0588
#define AB8500_USB_CTRL_SPARE_REG	0x0589
#define AB8500_USB_PHY_CTRL_REG	0x058A	/*only in Cut1.0*/

/*
 * TVOUT / CTRL register offsets
 * Bank : 0x06
 */
#define AB8500_TVOUT_CTRL_REG	0x0680
/*
 * DBI register offsets
 * Bank : 0x07
 */
#define AB8500_DBI_REG1_REG	0x0700
#define AB8500_DBI_REG2_REG	0x0701
/*
 * ECI regsiter offsets
 * Bank : 0x08
 */
#define AB8500_ECI_CTRL_REG		0x0800
#define AB8500_ECI_HOOKLEVEL_REG	0x0801
#define AB8500_ECI_DATAOUT_REG		0x0802
#define AB8500_ECI_DATAIN_REG		0x0803
/*
 * AV Connector register offsets
 * Bank : 0x08
 */
#define AB8500_AV_CONN_REG	0x0840
/*
 * Accessory detection register offsets
 * Bank : 0x08
 */
#define AB8500_ACC_DET_DB1_REG	0x0880
#define AB8500_ACC_DET_DB2_REG	0x0881
/*
 * GPADC register offsets
 * Bank : 0x0A
 */
#define AB8500_GPADC_CTRL1_REG		0x0A00
#define AB8500_GPADC_CTRL2_REG		0x0A01
#define AB8500_GPADC_CTRL3_REG		0x0A02
#define AB8500_GPADC_AUTO_TIMER_REG	0x0A03
#define AB8500_GPADC_STAT_REG		0x0A04
#define AB8500_GPADC_MANDATAL_REG	0x0A05
#define AB8500_GPADC_MANDATAH_REG	0x0A06
#define AB8500_GPADC_AUTODATAL_REG	0x0A07
#define AB8500_GPADC_AUTODATAH_REG	0x0A08
#define AB8500_GPADC_MUX_CTRL_REG	0x0A09
/*
 * Charger / status register offfsets
 * Bank : 0x0B
 */
#define AB8500_CH_STATUS1_REG		0x0B00
#define AB8500_CH_STATUS2_REG		0x0B01
#define AB8500_CH_USBCH_STAT1_REG	0x0B02
#define AB8500_CH_USBCH_STAT2_REG	0x0B03
#define AB8500_CH_FSM_STAT_REG		0x0B04
#define AB8500_CH_STAT_REG		0x0B05
/*
 * Charger / control register offfsets
 * Bank : 0x0B
 */
#define AB8500_CH_VOLT_LVL_REG		0x0B40
#define AB8500_CH_VOLT_LVL_MAX_REG	0x0B41	/*Only in Cut1.0*/
#define AB8500_CH_OPT_CRNTLVL_REG	0x0B42	/*Only in Cut1.0*/
#define AB8500_CH_OPT_CRNTLVL_MAX_REG	0x0B43	/*Only in Cut1.0*/
#define AB8500_CH_WD_TIMER_REG		0x0B44	/*Only in Cut1.0*/
#define AB8500_CH_WD_CTRL_REG		0x0B45	/*Only in Cut1.0*/
/*
 * Charger / main control register offfsets
 * Bank : 0x0B
 */
#define AB8500_MCH_CTRL1		0x0B80
#define AB8500_MCH_CTRL2		0x0B81
#define AB8500_MCH_IPT_CURLVL_REG	0x0B82
#define AB8500_CH_WD_REG		0x0B83
/*
 * Charger / USB control register offsets
 * Bank : 0x0B
 */
#define AB8500_USBCH_CTRL1_REG		0x0BC0
#define AB8500_USBCH_CTRL2_REG		0x0BC1
#define AB8500_USBCH_IPT_CRNTLVL_REG	0x0BC2
/*
 * Gas Gauge register offsets
 * Bank : 0x0C
 */
#define AB8500_GASG_CC_CTRL_REG	0x0C00
#define AB8500_GASG_CC_ACCU1_REG	0x0C01
#define AB8500_GASG_CC_ACCU2_REG	0x0C02
#define AB8500_GASG_CC_ACCU3_REG	0x0C03
#define AB8500_GASG_CC_ACCU4_REG	0x0C04
#define AB8500_GASG_CC_SMPL_CNTRL_REG	0x0C05
#define AB8500_GASG_CC_SMPL_CNTRH_REG	0x0C06
#define AB8500_GASG_CC_SMPL_CNVL_REG	0x0C07
#define AB8500_GASG_CC_SMPL_CNVH_REG	0x0C08
#define AB8500_GASG_CC_CNTR_AVGOFF_REG	0x0C09
#define AB8500_GASG_CC_OFFSET_REG	0x0C0A
/*
 * Audio
 * Bank : 0x0D
 * Not a part of this file. Should be part of Audio codec driver
 */

/*
 * Interrupt register offsets
 * Bank : 0x0E
 */
#define AB8500_IT_SOURCE1_REG	0x0E00
#define AB8500_IT_SOURCE2_REG	0x0E01
#define AB8500_IT_SOURCE3_REG	0x0E02
#define AB8500_IT_SOURCE4_REG	0x0E03
#define AB8500_IT_SOURCE5_REG	0x0E04

/* available only in 1.0 */
#define AB8500_IT_SOURCE7_REG	0x0E06
#define AB8500_IT_SOURCE8_REG	0x0E07
#define AB8500_IT_SOURCE19_REG	0x0E12

#define AB8500_IT_SOURCE20_REG	0x0E13
#define AB8500_IT_SOURCE21_REG	0x0E14
#define AB8500_IT_SOURCE22_REG	0x0E15

/*
 * latch registers
 */
#define AB8500_IT_LATCH1_REG	0x0E20
#define AB8500_IT_LATCH2_REG	0x0E21
#define AB8500_IT_LATCH3_REG	0x0E22
#define AB8500_IT_LATCH4_REG	0x0E23
#define AB8500_IT_LATCH5_REG	0x0E24

/* available only in 1.0 */
#define AB8500_IT_LATCH7_REG	0x0E26
#define AB8500_IT_LATCH8_REG	0x0E27
#define AB8500_IT_LATCH9_REG	0x0E28
#define AB8500_IT_LATCH10_REG	0x0E29
#define AB8500_IT_LATCH19_REG	0x0E32

#define AB8500_IT_LATCH20_REG	0x0E33
#define AB8500_IT_LATCH21_REG	0x0E34
#define AB8500_IT_LATCH22_REG	0x0E35

/*
 * mask registers
 */

#define AB8500_IT_MASK1_REG	0x0E40
#define AB8500_IT_MASK2_REG	0x0E41
#define AB8500_IT_MASK3_REG	0x0E42
#define AB8500_IT_MASK4_REG	0x0E43
#define AB8500_IT_MASK5_REG	0x0E44


/* available only in 1.0 */
#define AB8500_IT_MASK7_REG	0x0E46
#define AB8500_IT_MASK8_REG	0x0E47
#define AB8500_IT_MASK9_REG	0x0E48
#define AB8500_IT_MASK10_REG	0x0E49
#define AB8500_IT_MASK11_REG	0x0E4A
#define AB8500_IT_MASK12_REG	0x0E4B
#define AB8500_IT_MASK13_REG	0x0E4C
#define AB8500_IT_MASK14_REG	0x0E4D
#define AB8500_IT_MASK15_REG	0x0E4E
#define AB8500_IT_MASK16_REG	0x0E4F
#define AB8500_IT_MASK17_REG	0x0E50
#define AB8500_IT_MASK18_REG	0x0E51
#define AB8500_IT_MASK19_REG	0x0E52

#define AB8500_IT_MASK20_REG	0x0E53
#define AB8500_IT_MASK21_REG	0x0E54
#define AB8500_IT_MASK22_REG	0x0E55

/*
 * RTC bank register offsets
 * Bank : 0xF
 */
#define AB8500_RTC_SWITCHOFF_STAT_REG	0x0F00
#define AB8500_RTC_CC_CONF_REG		0x0F01
#define AB8500_RTC_READ_REQ_REG	0x0F02
#define AB8500_RTC_WATCH_TSECMID_REG	0x0F03
#define AB8500_RTC_WATCH_TSECHI_REG	0x0F04
#define AB8500_RTC_WATCH_TMIN_LOW_REG	0x0F05
#define AB8500_RTC_WATCH_TMIN_MID_REG	0x0F06
#define AB8500_RTC_WATCH_TMIN_HI_REG	0x0F07
#define AB8500_RTC_ALRM_MIN_LOW_REG	0x0F08
#define AB8500_RTC_ALRM_MIN_MID_REG	0x0F09
#define AB8500_RTC_ALRM_MIN_HI_REG	0x0F0A
#define AB8500_RTC_STAT_REG		0x0F0B
#define AB8500_RTC_BKUP_CHG_REG	0x0F0C
#define AB8500_RTC_FORCE_BKUP_REG	0x0F0D
#define AB8500_RTC_CALIB_REG		0x0F0E
#define AB8500_RTC_SWITCH_STAT_REG	0x0F0F

/*
 * Misc block GPIO register offsets - Not for ED
 * Bank : 0x10
 */
/* available only in 1.0 */
#define AB8500_GPIO_SEL1_REG	0x01000
#define AB8500_GPIO_SEL2_REG	0x01001
#define AB8500_GPIO_SEL3_REG	0x01002
#define AB8500_GPIO_SEL4_REG	0x01003
#define AB8500_GPIO_SEL5_REG	0x01004
#define AB8500_GPIO_SEL6_REG	0x01005
#define AB8500_GPIO_DIR1_REG	0x01010
#define AB8500_GPIO_DIR2_REG	0x01011
#define AB8500_GPIO_DIR3_REG	0x01012
#define AB8500_GPIO_DIR4_REG	0x01013
#define AB8500_GPIO_DIR5_REG	0x01014
#define AB8500_GPIO_DIR6_REG	0x01015

#define AB8500_GPIO_OUT1_REG	0x01020
#define AB8500_GPIO_OUT2_REG	0x01021
#define AB8500_GPIO_OUT3_REG	0x01022
#define AB8500_GPIO_OUT4_REG	0x01023
#define AB8500_GPIO_OUT5_REG	0x01024
#define AB8500_GPIO_OUT6_REG	0x01025

#define AB8500_GPIO_PUD1_REG	0x01030
#define AB8500_GPIO_PUD2_REG	0x01031
#define AB8500_GPIO_PUD3_REG	0x01032
#define AB8500_GPIO_PUD4_REG	0x01033
#define AB8500_GPIO_PUD5_REG	0x01034
#define AB8500_GPIO_PUD6_REG	0x01035

#define AB8500_GPIO_IN1_REG	0x01040
#define AB8500_GPIO_IN2_REG	0x01041
#define AB8500_GPIO_IN3_REG	0x01042
#define AB8500_GPIO_IN4_REG	0x01043
#define AB8500_GPIO_IN5_REG	0x01044
#define AB8500_GPIO_IN6_REG	0x01045
#define AB8500_GPIO_ALT_FUNC	0x01050

/*
 * PWM Out generators
 * Bank: 0x10
 */
#define AB8500_PWM_OUT_CTRL1_REG	0x1060
#define AB8500_PWM_OUT_CTRL2_REG	0x1061
#define AB8500_PWM_OUT_CTRL3_REG	0x1062
#define AB8500_PWM_OUT_CTRL4_REG	0x1063
#define AB8500_PWM_OUT_CTRL5_REG	0x1064
#define AB8500_PWM_OUT_CTRL6_REG	0x1065
#define AB8500_PWM_OUT_CTRL7_REG	0x1066

#define AB8500_I2C_PAD_CTRL_REG	0x1067
#define AB8500_REV_REG			0x1080

/*
 * Misc, Debug Test Configuration register
 * Bank : 0x11
 */
#define AB8500_DEBUG_TESTMODE_REG	0x01100

/* only in 1.0 */
#define AB8500_I2C_TRIG1_ADR_REG	0x1101
#define AB8500_I2C_TRIG1_ID_REG	0x1102
#define AB8500_I2C_TRIG2_ADR_REG	0x1103
#define AB8500_I2C_TRIG3_ID_REG	0x1104
#define AB8500_I2C_NOACCESS_SUP_REG	0x1105

/* Offsets in TurnOnstatus register
 */

#define AB8500_MAX_INT		88
#define AB8500_MAX_FUTURE_USE	105

#define AB8500_MAX_INT_SOURCE	11
#define AB8500_MAX_INT_LATCH	13
#define AB8500_MAX_INT_MASK	21

/**
 * struct ab8500_device - Stw4500 device structure
 * @cs_en:	pointer chip select enable
 * @cs_dis:	pointer to chip select disable
 *
 * Stw4500 Internal device structure
 */
struct ab8500_device	{
	void (*cs_en) (void);
	void (*cs_dis) (void);
	u16 ssp_controller;
};

/*struct t_ab8500_context;*/

/**
 * struct client_callbacks - Client callbacks
 * @callback:	callback handler
 * @data:	private data for the handler
 *
 * Stw4500 maintains a internal data structure for the registered
 * callback
 */
struct client_callbacks	{
	void	(*callback)(void *data);
	void	*data;
};

/**
 * struct ab8500 - AB8500 Internal data structure
 * @ab8500_master:	Pointer to the spi_master
 * @ab8500_board_info:	Pointer to the board information structure
 * @ab8500_spi:	Pointer to the spi_device strcture
 * @spi_transfer:	Pointer SPI data transfer structure spi_transfer
 * @spi_message:	SPI message pointer of type spi_message
 * @ab8500_device:	AB8500 internal data structure
 * @ssp_wrbuf:		SSP write data buffer of size 4 bytes
 * @ssp_rdbuf:		SSP read data buffer of size 4 bytes
 * @work:		work queue scheduled in the interrupt handler
 * @c_callback:		Array of client's callback handler
 * @ab8500_cfg_lock:	synchronization primitive to protect the data
 * @ab8500_sem:	synchronization primitive used in the non-interrupt context
 * @irq:		interrupt number of ab8500
 * @revision:		revision number of ab8500 silicon
 *
 * Supports only SPI interface. The device can also be accessed
 * through I2C in the successive version of U8500.
 */
struct ab8500	{
	struct spi_master	*ab8500_master;
	struct spi_board_info	*ab8500_board_info;
	struct spi_device	*ab8500_spi;
	struct spi_transfer	*ab8500_xfer;
	struct spi_message	*ab8500_msg;
	struct ab8500_device	*board;
	u32 ssp_wrbuf[4];
	u32 ssp_rdbuf[4];
	struct client_callbacks	c_callback[184];
	struct work_struct	work;
	spinlock_t		ab8500_cfg_lock;
	struct semaphore	ab8500_sem;
	unsigned char		irq;
	unsigned char		revision;
};

int ab8500_get_version(void);
int ab8500_write(u8 block, u32 adr, u8 data);
int ab8500_read(u8 block, u32 adr);
int ab8500_set_callback_handler(int int_no, void *callback_handler, void *data);
int ab8500_remove_callback_handler(int int_no);
void ab8500_int_mask(int int_no);
void ab8500_int_unmask(int int_no);

#endif /* AB8500_H_ */
