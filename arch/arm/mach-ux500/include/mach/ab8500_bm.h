/*
 * Copyright ST-Ericsson 2009.
 *
 * Author: Arun Murthy <arun.murthy@stericsson.com>
 * Licensed under GPLv2.
 */

#ifndef _AB8500_BM_H
#define _AB8500_BM_H

struct ab8500_bm_platform_data {
	int name;
	int termination_vol;	/* voltage in mV */
	int op_cur_lvl;		/* o/p current level in mA */
	int ip_vol_lvl;		/* i/p current level in mV */
};

/* Main charge i/p current */
#define MAIN_CH_IP_CUR_0P9A		0x80
#define MAIN_CH_IP_CUR_1P1A		0xA0
/* ChVoltLevel */
#define CH_VOL_LVL_3P5			0x00
#define CH_VOL_LVL_4P05			0x16
#define CH_VOL_LVL_4P1			0x1B
#define CH_VOL_LVL_4P2			0x25
#define CH_VOL_LVL_4P6			0x4D

/* ChOutputCurrentLevel */
#define CH_OP_CUR_LVL_0P1		0x00
#define CH_OP_CUR_LVL_0P9		0x08
#define CH_OP_CUR_LVL_1P4		0x0D
#define CH_OP_CUR_LVL_1P5		0x0E
#define CH_OP_CUR_LVL_1P6		0x0F

/* current is mA */
#define USB_0P1A			100
#define USB_0P2A			200
#define USB_0P3A			300
#define USB_0P4A			400
#define USB_0P5A			500

/* UsbChCurrLevel */
#define USB_CH_IP_CUR_LVL_0P05		0x00
#define USB_CH_IP_CUR_LVL_0P09		0x10
#define USB_CH_IP_CUR_LVL_0P19		0x20
#define USB_CH_IP_CUR_LVL_0P29		0x30
#define USB_CH_IP_CUR_LVL_0P38		0x40
#define USB_CH_IP_CUR_LVL_0P45		0x50
#define USB_CH_IP_CUR_LVL_0P5		0x60
#define USB_CH_IP_CUR_LVL_1P5		0xF0

#define LOW_BAT_3P1V			0x20
#define LOW_BAT_2P3V			0x00
#define LOW_BAT_RESET			0x01

/* constants */
#define AB8500_BM_CHARGING		0x01
#define AB8500_BM_NOT_CHARGING		0x00
#define AB8500_BM_AC_PRESENT		0x01
#define AB8500_BM_AC_NOT_PRESENT	0x00
#define AB8500_BM_USB_PRESENT		0x01
#define AB8500_BM_USB_NOT_PRESENT	0x00

/* Battery OVV */
#define AB8500_BM_BATTOVV_4P75		4750000
#define AB8500_BM_BATTOVV_3P7		3700000
#define MIN_TERMINATION_CUR		5
#define MAX_TERMINATION_CUR		200

/* Check the BattOvv Threshold */
#define BATT_OVV_TH			0x01
#define MAIN_CH_ON			0x02
#define USB_CH_ON			0x04
#define BATT_OVV_ENA			0x02
#define BATT_OVV_TH_3P7			0x00
#define BATT_OVV_TH_4P75		0x01
#define USB_CH_ENA			0x01
#define USB_CH_DIS			0x00
#define MAIN_WDOG_ENA			0x01
#define MAIN_WDOG_KICK			0x02
#define MAIN_WDOG_DIS			0x00
#define CHARG_WD_KICK			0x01
#define MAIN_CH_ENA			0x01
#define MAIN_CH_DIS			0x00
#define MAIN_CH_PLUG_DET		0x08
#define USB_CHG_DET_DONE		0x40
#define LED_INDICATOR_PWM_ENA		0x01
#define LED_INDICATOR_PWM_DIS		0x00
#define LED_IND_CUR_5MA			0x04
#define CHARGING_STOP_BY_BTEMP		0x08
#define LED_INDICATOR_PWM_DUTY_252_256	0xBF
#define MAIN_CH_CV_ON			0x04
#define USB_CH_CV_ON			0x08
#define OTP_ENABLE_WD			0x01
#define OTP_DISABLE_WD			0xFE
#define MASK_MAIN_EXT_CH_NOT_OK		0x01
#define UNMASK_MAIN_EXT_CH_NOT_OK	0xFE
#define MASK_VBUS_DET_R			0x80
#define MASK_VBUS_DET_F			0x40
#define MASK_MAIN_CH_PLUG_DET		0x08
#define MASK_MAIN_CH_UNPLUG_DET		0x04
#define MASK_BATT_OVV			0x01
#define UNMASK_VBUS_DET_R		0x7F
#define UNMASK_VBUS_DET_F		0xBF
#define UNMASK_MAIN_CH_PLUG_DET		0xF7
#define UNMASK_MAIN_CH_UNPLUG_DET	0xFB
#define UNMASK_BATT_OVV			0xFE
#define MASK_VBUS_OVV			0x40
#define MASK_CH_WD_EXP			0x20
#define MASK_BAT_CTRL_INDB		0x10
#define UNMASK_VBUS_OVV			0xBF
#define UNMASK_CH_WD_EXP		0xDF
#define UNMASK_BAT_CTRL_INDB		0xEF
#define MASK_BTEMP_HIGH			0x08
#define MASK_BTEMP_LOW			0x01
#define UNMASK_BTEMP_HIGH		0xF7
#define UNMASK_BTEMP_LOW		0xFE
#define MASK_USB_CHARGER_NOT_OK_R	0x02
#define UNMASK_USB_CHARGER_NOT_OK_R	0xFD
#define MASK_USB_CHG_DET_DONE		0x40
#define UNMASK_USB_CHG_DET_DONE		0xBF
#define MASK_USB_CHARGER_NOT_OK_F	0x80
#define UNMASK_USB_CHARGER_NOT_OK_F	0x7F

/* Battery resistance */
#define NOKIA_BL_5F			0x04
/* static constants */
#define STANDARD_HOST			0x04

/* UsbLineStatus register - usb types */
enum {
	USB_STAT_NOT_CONFIGURED,
	USB_STAT_STD_HOST_NC,
	USB_STAT_STD_HOST_C_NS,
	USB_STAT_STD_HOST_C_S,
	USB_STAT_HOST_CHG_NM,
	USB_STAT_HOST_CHG_HS,
	USB_STAT_HOST_CHG_HS_CHIRP,
	USB_STAT_DEDICATED_CHG,
	USB_STAT_ACA_RID_A,
	USB_STAT_ACA_RID_B,
	USB_STAT_ACA_RID_C_NM,
	USB_STAT_ACA_RID_C_HS,
	USB_STAT_ACA_RID_C_HS_CHIRP,
	USB_STAT_HM_IDGND,
	USB_STAT_RESERVED,
	USB_STAT_NOT_VALID_LINK,
};
#endif /* _AB8500_BM_H */
