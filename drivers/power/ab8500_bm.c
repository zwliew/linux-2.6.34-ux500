/*
 * ab8500_bm.c - AB8500 Battery Management Driver
 *
 * Copyright (C) 2010 ST-Ericsson
 * Licensed under GPLv2.
 *
 * Author: Arun R Murthy <arun.murthy@stericsson.com>
 * TODO: Read the type of battery and accordingly set the battery
 * thershold voltage. USB type and set the charger input current.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <mach/ab8500.h>
#include <mach/ab8500_gpadc.h>

/* Interrupt */
#define MAIN_EXT_CH_NOT_OK	0
#define BATT_OVV		8
#define MAIN_CH_UNPLUG_DET	10
#define MAIN_CH_PLUG_DET	11
#define VBUS_DET_F		14
#define VBUS_DET_R		15
#define BAT_CTRL_INDB		20
#define CH_WD_EXP		21
#define VBUS_OVV		22
#define LOW_BAT_F		28
#define LOW_BAT_R		29
#define GP_SW_ADC_CONV_END	39
#define BTEMP_LOW		72
#define BTEMP_HIGH		75
#define USB_CHARGER_NOT_OKR	81
#define USB_CHG_DET_DONE	94
#define USB_CH_TH_PROT_R	97
#define MAIN_CH_TH_PROT_R	99
#define USB_CHARGER_NOT_OKF	103

/* Charger constants */
#define NO_PW_CONN	0
#define AC_PW_CONN	1
#define USB_PW_CONN	2

/* TODO: For time being since there is not sync between the USB connectivity
 * and the battery driver both cant be used simultaneously. Hence when
 * enabling this flag make sure to disable USB gadget serial driver.
 * #define CONFIG_USB_CHARGER
 */

static DEFINE_MUTEX(ab8500_bm_lock);

/* Structure - Device Information
 * struct ab8500_bm_device_info - device information
 * @dev:				Pointer to the structure device
 * @voltage_uV:				Battery voltage in uV
 * @bk_voltage_uV:			Backup battery voltage
 * @current_uA:				Battery current in uA
 * @temp_C:				Battery temperature
 * @charger_resoc:			Charger resource(AC or USB)
 * @charge_status:			Charger status(charging, discharging)
 * @bk_battery_charge_status:		Backup battery charger status
 * @bat:		Structure that holds the battery properties
 * @bk_bat:		Structure that holds the backup battery properties
 * @ac:			Structure that holds the ac/mains properties
 * @usb:		Structure that holds the usb properties
 * @ab8500_bm_monitor_work:		Work to monitor the main battery
 * @ab8500_bm_watchdog_work:		Work to re-kick the watchdog
 * @ab8500_bm_ac_en_monitor_work:	Work to enable ac chaging
 * @ab8500_bm_ac_dis_monitor_work:	Work to disable ac charging
 * @ab8500_bm_usb_en_monitor_work:	Work to enable usb charging
 * @ab8500_bm_usb_dis_monitor_work:	Work to disable usb charging
 * @ab8500_bm_wq:			Pointer to work queue-battery mponitor
 * @ab8500_bm_wd_kick_wq:		Pointer to work queue-rekick watchdog
 *
 */
struct ab8500_bm_device_info {
	struct device *dev;
	int voltage_uV;
	int bk_voltage_uV;
	int current_uA;
	int temp_C;
	int charger_resoc;
	int charge_status;
	int bk_battery_charge_status;
	struct power_supply bat;
	struct power_supply bk_bat;
	struct power_supply ac;
	struct power_supply usb;
	struct delayed_work ab8500_bm_monitor_work;
	struct delayed_work ab8500_bm_watchdog_work;
	struct work_struct ab8500_bm_ac_en_monitor_work;
	struct work_struct ab8500_bm_ac_dis_monitor_work;
	struct work_struct ab8500_bm_usb_en_monitor_work;
	struct work_struct ab8500_bm_usb_dis_monitor_work;
	struct workqueue_struct *ab8500_bm_wq;
	struct workqueue_struct *ab8500_bm_irq;
	struct workqueue_struct *ab8500_bm_wd_kick_wq;
};

static int ab8500_bm_ac_en(struct ab8500_bm_device_info *, int);
static int ab8500_bm_usb_en(struct ab8500_bm_device_info *, int);
static int ab8500_bm_charger_presence(struct ab8500_bm_device_info *);
static int ab8500_bm_status(void);
static void ab8500_bm_battery_update_status(struct ab8500_bm_device_info *);

/* Work queue function for enabling AC charging */
static void ab8500_bm_ac_en_work(struct work_struct *work)
{
	struct ab8500_bm_device_info *di = container_of(work,
							struct
							ab8500_bm_device_info,
							ab8500_bm_ac_en_monitor_work);
	int ret = 0, val = 0;

	/* In case both MAINS and USB are plugged in priority goes to AC
	 * charging hence disable USB charging enable AC charging
	 */
	val = ab8500_bm_charger_presence(di);
	if (val == (USB_PW_CONN + AC_PW_CONN)) {
		ret = ab8500_bm_usb_en(di, false);
		if (ret < 0)
			dev_vdbg(di->dev, "failed to disable USB charging\n");
		dev_dbg
		    (di->dev,
		     "USB present, AC plug detected, hence disabling USB charging and enabling AC charging\n");
	}
	ret = ab8500_bm_ac_en(di, true);
	if (ret < 0)
		dev_vdbg(di->dev, "failed to enable AC charging\n");
}

/* Work queue function for disabling AC charging */
static void ab8500_bm_ac_dis_work(struct work_struct *work)
{
	struct ab8500_bm_device_info *di = container_of(work,
							struct
							ab8500_bm_device_info,
							ab8500_bm_ac_dis_monitor_work);
	int ret = 0, val = 0;
	val = ab8500_bm_charger_presence(di);
	if (val == USB_PW_CONN) {
		dev_dbg(di->dev,
			"disabling AC charging, and enabling USB charging\n");
		ret = ab8500_bm_usb_en(di, true);
		if (ret < 0)
			dev_vdbg(di->dev, "failed to enable USB charging\n");
	}
	ret = ab8500_bm_ac_en(di, false);
	if (ret < 0)
		dev_vdbg(di->dev, "failed to disable AC charging\n");
	ab8500_bm_battery_update_status(di);
}

/* Work queue function for enabling USB charging */
static void ab8500_bm_usb_en_work(struct work_struct *work)
{
	struct ab8500_bm_device_info *di = container_of(work,
							struct
							ab8500_bm_device_info,
							ab8500_bm_usb_en_monitor_work);
	int ret = 0, val = 0;

	/* In case both MAINS and USB are plugged in priority goes to AC
	 * charging hence dont enable USB charging let AC charging continue
	 */
	val = ab8500_bm_charger_presence(di);
	if (val == (AC_PW_CONN + USB_PW_CONN))
		dev_dbg
		    (di->dev,
		     "AC charger present, hence not enabling USB charging\n");
	else {
		ab8500_bm_usb_en(di, true);
		if (ret < 0)
			dev_vdbg(di->dev, "failed to enable USB charging\n");
	}
}

/* Work queue function for disabling USB charging */
static void ab8500_bm_usb_dis_work(struct work_struct *work)
{
	struct ab8500_bm_device_info *di = container_of(work,
							struct
							ab8500_bm_device_info,
							ab8500_bm_usb_dis_monitor_work);
	int ret = 0;
	ab8500_bm_usb_en(di, false);
	if (ret < 0)
		dev_vdbg(di->dev, "failed to disable USB charging\n");
	ab8500_bm_battery_update_status(di);
}

/* callback handlers  - main charger not OK */
static void ab8500_bm_mainextchnotok_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	/* on occurance of this int charging not done */
	dev_info(di->dev, "Main charger NOT OK! Charging cannot proceed\n");
}

/* callback handlers  - battery overvoltage detected */
static void ab8500_bm_battovv_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	int val = 0;
	/* on occurance of this int charging is stopped by hardware */
	dev_info
	    (di->dev,
	     "Battery voltage is above threshold voltage, charging stopped....!\n");
	/* For protection sw is also disabling charging */
	val = ab8500_bm_charger_presence(di);
	if ((val == (AC_PW_CONN + USB_PW_CONN)) || (val == AC_PW_CONN)) {
		dev_dbg(di->dev, "Disabling AC charging\n");
		queue_work(di->ab8500_bm_irq,
			   &di->ab8500_bm_ac_dis_monitor_work);
	} else if (val == USB_PW_CONN) {
		dev_dbg(di->dev, "Disabling USB charging\n");
		queue_work(di->ab8500_bm_irq,
			   &di->ab8500_bm_usb_dis_monitor_work);
	}
	ab8500_bm_battery_update_status(di);
}

/* callback handlers  - main charge unplug detected */
static void ab8500_bm_mainchunplugdet_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	power_supply_changed(&di->bat);
	dev_dbg(di->dev, "main charger unplug detected....!\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_ac_dis_monitor_work);
}

/* callback handlers  - main charge plug detected */
static void ab8500_bm_mainchplugdet_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	power_supply_changed(&di->bat);
	dev_dbg(di->dev, "main charger plug detected....!\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_ac_en_monitor_work);
}

#if defined(CONFIG_USB_CHARGER)
/* callback handlers  - rising edge on vbus detected */
static void ab8500_bm_vbusdetr_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	power_supply_changed(&di->bat);
	dev_dbg(di->dev, "vbus rising edge detected....!\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_usb_en_monitor_work);
}

/* callback handlers  - falling edge on vbus detected */
static void ab8500_bm_vbusdetf_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	power_supply_changed(&di->bat);
	dev_dbg(di->dev, "vbus falling edge detected....!\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_usb_dis_monitor_work);
}
#endif

/* callback handlers  - battery removal detected */
static void ab8500_bm_batctrlindb_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	int val = 0;

	dev_info(di->dev, "Battery removal detected, charging stopped...!\n");
	/* For protection sw is also disabling charging */
	val = ab8500_bm_charger_presence(di);
	if ((val == (AC_PW_CONN + USB_PW_CONN)) || (val == AC_PW_CONN)) {
		dev_dbg(di->dev, "Disabling AC charging\n");
		queue_work(di->ab8500_bm_irq,
			   &di->ab8500_bm_ac_dis_monitor_work);
	} else if (val == USB_PW_CONN) {
		dev_dbg(di->dev, "Disabling USB charging\n");
		queue_work(di->ab8500_bm_irq,
			   &di->ab8500_bm_usb_dis_monitor_work);
	}
}

/* callback handlers  - watchdog charger expiration detected */
static void ab8500_bm_chwdexp_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	dev_info
	    (di->dev,
	     "Watchdog charger expiration detected, charging stopped...!\n");
}

#if defined(CONFIG_USB_CHARGER)
/* callback handlers  - vbus overvoltage detected */
static void ab8500_bm_vbusovv_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;

	dev_info
	    (di->dev, "VBUS overvoltage detected, USB charging stopped...!\n");
	/* For protection sw is also disabling charging */
	dev_dbg(di->dev, "Disabling USB charging\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_usb_dis_monitor_work);
}
#endif

/* callback handlers - bat voltage goes below LowBat detected */
static void ab8500_bm_lowbatf_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	dev_dbg(di->dev, "Battery voltage goes below LowBat voltage\n");
}

/* callback handlers - bat voltage goes above LowBat detected */
static void ab8500_bm_lowbatr_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	dev_dbg(di->dev, "Battery voltage goes above LowBat voltage\n");
}

/* callback handlers  - battery temp lower than 10c */
static void ab8500_bm_btemplow_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	int val = 0;

	dev_info(di->dev, "Battery temperature lower than -10deg c\n");
	/* For protection sw is also disabling charging */
	val = ab8500_bm_charger_presence(di);
	if ((val == (AC_PW_CONN + USB_PW_CONN)) || (val == AC_PW_CONN)) {
		dev_dbg(di->dev, "Disabling AC charging\n");
		queue_work(di->ab8500_bm_irq,
			   &di->ab8500_bm_ac_dis_monitor_work);
	} else if (val == USB_PW_CONN) {
		dev_dbg(di->dev, "Disabling USB charging\n");
		queue_work(di->ab8500_bm_irq,
			   &di->ab8500_bm_usb_dis_monitor_work);
	}
}

/* callback handlers  - battery temp higher than max temp */
static void ab8500_bm_btemphigh_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	int val = 0;

	dev_info(di->dev, "Battery temperature is higher than MAX temp\n");
	/* For protection sw is also disabling charging */
	val = ab8500_bm_charger_presence(di);
	if ((val == (AC_PW_CONN + USB_PW_CONN)) || (val == AC_PW_CONN)) {
		dev_dbg(di->dev, "Disabling AC charging\n");
		queue_work(di->ab8500_bm_irq,
			   &di->ab8500_bm_ac_dis_monitor_work);
	} else if (val == USB_PW_CONN) {
		dev_dbg(di->dev, "Disabling USB charging\n");
		queue_work(di->ab8500_bm_irq,
			   &di->ab8500_bm_usb_dis_monitor_work);
	}
}

#if defined(CONFIG_USB_CHARGER)
/* callback handlers  - not allowed usb charger detected */
static void ab8500_bm_usbchargernotokr_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	dev_info
	    (di->dev,
	     "Not allowed USB charger detected, charging cannot proceed...!\n");
}

/* callback handlers  - usb charger detected */
static void ab8500_bm_usbchgdetdone_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	power_supply_changed(&di->bat);
	dev_dbg(di->dev, "usb charger detected...!\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_usb_en_monitor_work);
}
#endif

/*
 * callback handlers  - Die temp is above usb charger
 * thermal protection threshold
 */
static void ab8500_bm_usbchthprotr_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	int val = 0;

	dev_dbg
	    (di->dev,
	     "Die temp above USB charger thermal protection threshold, charging stopped...!\n");
	/* For protection sw is also disabling charging */
	val = ab8500_bm_charger_presence(di);
	if ((val == (AC_PW_CONN + USB_PW_CONN)) || (val == AC_PW_CONN)) {
		dev_dbg(di->dev, "Disabling AC charging\n");
		queue_work(di->ab8500_bm_irq,
			   &di->ab8500_bm_ac_dis_monitor_work);
	} else if (val == USB_PW_CONN) {
		dev_dbg(di->dev, "Disabling USB charging\n");
		queue_work(di->ab8500_bm_irq,
			   &di->ab8500_bm_usb_dis_monitor_work);
	}
}

/*
 * callback handlers  - Die temp is above main charger
 * thermal protection threshold
 */
static void ab8500_bm_mainchthprotr_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	int val = 0;

	dev_dbg
	    (di->dev,
	     "Die temp above Main charger thermal protection threshold, charging stopped...!\n");
	/* For protection sw is also disabling charging */
	val = ab8500_bm_charger_presence(di);
	if ((val == (AC_PW_CONN + USB_PW_CONN)) || (val == AC_PW_CONN)) {
		dev_dbg(di->dev, "Disabling AC charging\n");
		queue_work(di->ab8500_bm_irq,
			   &di->ab8500_bm_ac_dis_monitor_work);
	} else if (val == USB_PW_CONN) {
		dev_dbg(di->dev, "Disabling USB charging\n");
		queue_work(di->ab8500_bm_irq,
			   &di->ab8500_bm_usb_dis_monitor_work);
	}
}

#if defined(CONFIG_USB_CHARGER)
/* callback handlers  - usb charger unplug detected */
static void ab8500_bm_usbchargernotokf_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	power_supply_changed(&di->bat);
	dev_dbg(di->dev, "usb charger unplug detected...!\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_usb_dis_monitor_work);
}
#endif
/*
 * Registers/Unregisters irq_no and callback handler functions
 * with the AB8500 core driver
 */
static int ab8500_bm_register_handler(int set, void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	int ret = -1;

	/* Register irq_no to callback handler function */
	if (set) {
		/* set callback handlers  - main charger not OK */
		ret = ab8500_set_callback_handler(MAIN_EXT_CH_NOT_OK,
						  ab8500_bm_mainextchnotok_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-main charger not ok\n");
		/* set callback handlers  - battery overvoltage detected */
		ret = ab8500_set_callback_handler(BATT_OVV,
						  ab8500_bm_battovv_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-battery over voltage detected\n");
		/* set callback handlers  - main charge unplug detected */
		ret = ab8500_set_callback_handler(MAIN_CH_UNPLUG_DET,
						  ab8500_bm_mainchunplugdet_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-main charge unplug detected\n");
		/* set callback handlers  - main charge plug detected */
		ret = ab8500_set_callback_handler(MAIN_CH_PLUG_DET,
						  ab8500_bm_mainchplugdet_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-main charge plug detected\n");
#if defined(CONFIG_USB_CHARGER)
		/* set callback handlers  - rising edge on vbus detected */
		ret = ab8500_set_callback_handler(VBUS_DET_R,
						  ab8500_bm_vbusdetr_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-rising edge on vbus detected\n");
		/* set callback handlers  - falling edge on vbus detected */
		ret = ab8500_set_callback_handler(VBUS_DET_F,
						  ab8500_bm_vbusdetf_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-falling edge on vbus detected\n");
#endif
		/* set callback handlers  - battery removal detected */
		ret = ab8500_set_callback_handler(BAT_CTRL_INDB,
						  ab8500_bm_batctrlindb_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-battery removal detected\n");
		/* set callback handlers  - watchdog charger expiration
		 * detected
		 */
		ret = ab8500_set_callback_handler(CH_WD_EXP,
						  ab8500_bm_chwdexp_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-watchdog expiration detected\n");
#if defined(CONFIG_USB_CHARGER)
		/* set callback handlers  - vbus overvoltage detected */
		ret = ab8500_set_callback_handler(VBUS_OVV,
						  ab8500_bm_vbusovv_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-vbus overvoltage detected\n");
#endif
		/* set callback handlers  - bat voltage goes below LowBat
		 * detected
		 */
		ret = ab8500_set_callback_handler(LOW_BAT_F,
						  ab8500_bm_lowbatf_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-bat voltage goes below LowBat detected\n");
		/* set callback handlers  - bat voltage goes above LowBat
		 * detected
		 */
		ret = ab8500_set_callback_handler(LOW_BAT_R,
						  ab8500_bm_lowbatr_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-bat voltage goes above LowBat detected\n");
		/* set callback handlers  - battery temp lower than 10c */
		ret = ab8500_set_callback_handler(BTEMP_LOW,
						  ab8500_bm_btemplow_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-battery temp below -10 deg\n");
		/* set callback handlers - battery temp higher than max temp */
		ret = ab8500_set_callback_handler(BTEMP_HIGH,
						  ab8500_bm_btemphigh_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-battery temp greater than max temp\n");
#if defined(CONFIG_USB_CHARGER)
		/* set callback handlers  - not allowed usb charger detected */
		ret = ab8500_set_callback_handler(USB_CHARGER_NOT_OKR,
						  ab8500_bm_usbchargernotokr_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-not allowed usb charger detected\n");
		/* set callback handlers  - usb charger detected */
		ret = ab8500_set_callback_handler(USB_CHG_DET_DONE,
						  ab8500_bm_usbchgdetdone_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-usb charger detected\n");
#endif
		/* set callback handlers  - Die temp is above usb charger
		 * thermal protection threshold
		 */
		ret = ab8500_set_callback_handler(USB_CH_TH_PROT_R,
						  ab8500_bm_usbchthprotr_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-die temp above usb charger thermal protection threshold\n");
		/* set callback handlers  - Die temp is above main charger
		 * thermal protection threshold
		 */
		ret = ab8500_set_callback_handler(MAIN_CH_TH_PROT_R,
						  ab8500_bm_mainchthprotr_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-dir temp above main charger thermal protection threshold\n");
#if defined(CONFIG_USB_CHARGER)
		/* set callback handlers  - usb charger unplug detected */
		ret = ab8500_set_callback_handler(USB_CHARGER_NOT_OKF,
						  ab8500_bm_usbchargernotokf_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-usb charger unplug detected\n");
#endif
	}
	/* Unregister irq_no and callback handler */
	else {
		/* remove callback handlers  - main charger not OK */
		ret = ab8500_remove_callback_handler(MAIN_EXT_CH_NOT_OK);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-main charger not ok\n");
		/* remove callback handlers  - battery overvoltage detected */
		ret = ab8500_remove_callback_handler(BATT_OVV);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-battery over voltage detected\n");
		/* remove callback handlers  - main charge unplug detected */
		ret = ab8500_remove_callback_handler(MAIN_CH_UNPLUG_DET);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-main charge unplug detected\n");
		/* remove callback handlers  - main charge plug detected */
		ret = ab8500_remove_callback_handler(MAIN_CH_PLUG_DET);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-main charge plug detected\n");
#if defined(CONFIG_USB_CHARGER)
		/* remove callback handlers  - rising edge on vbus detected */
		ret = ab8500_remove_callback_handler(VBUS_DET_R);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-rising edge on vbus detected\n");
		/* remove callback handlers  - falling edge on vbus detected */
		ret = ab8500_remove_callback_handler(VBUS_DET_F);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-falling edge on vbus detected\n");
#endif
		/* remove callback handlers  - battery removal detected */
		ret = ab8500_remove_callback_handler(BAT_CTRL_INDB);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-battery removal detected\n");
		/* remove callback handlers  - watchdog charger expiration
		 * detected
		 */
		ret = ab8500_remove_callback_handler(CH_WD_EXP);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-watchdog expiration detected\n");
#if defined(CONFIG_USB_CHARGER)
		/* remove callback handlers  - vbus overvoltage detected */
		ret = ab8500_remove_callback_handler(VBUS_OVV);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-vbus overvoltage detected\n");
#endif
		/* remove callback handlers  - bat voltage goes below LowBat
		 * detected
		 */
		ret = ab8500_remove_callback_handler(LOW_BAT_F);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-bat voltage goes below LowBat detected\n");
		/* remove callback handlers  - bat voltage goes above LowBat
		 * detected
		 */
		ret = ab8500_remove_callback_handler(LOW_BAT_R);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-bat voltage goes above LowBat detected\n");
		/* remove callback handlers  - battery temp lower than 10c */
		ret = ab8500_remove_callback_handler(BTEMP_LOW);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-battery temp below -10 deg\n");
		/* remove callback handlers  - battery temp higher than
		 * max temp
		 */
		ret = ab8500_remove_callback_handler(BTEMP_HIGH);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-battery temp greater than max temp\n");
#if defined(CONFIG_USB_CHARGER)
		/* remove callback handlers  - not allowed usb charger
		 * detected
		 */
		ret = ab8500_remove_callback_handler(USB_CHARGER_NOT_OKR);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-not allowed usb charger detected\n");
		/* remove callback handlers  - usb charger detected */
		ret = ab8500_remove_callback_handler(USB_CHG_DET_DONE);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-usb charger detected\n");
#endif
		/* remove callback handlers  - Die temp is above usb charger
		 * thermal protection threshold
		 */
		ret = ab8500_remove_callback_handler(USB_CH_TH_PROT_R);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-die temp above usb charger thermal protection threshold\n");
		/* remove callback handlers  - Die temp is above main charger
		 * thermal protection threshold
		 */
		ret = ab8500_remove_callback_handler(MAIN_CH_TH_PROT_R);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-dir temp above main charger thermal protection threshold\n");
#if defined(CONFIG_USB_CHARGER)
		/* remove callback handlers  - usb charger unplug detected */
		ret = ab8500_remove_callback_handler(USB_CHARGER_NOT_OKF);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-usb charger unplug detected\n");
#endif
	}

	return ret;
}

static int ab8500_bm_ac_voltage(struct ab8500_bm_device_info *di)
{
	int data;

	data = ab8500_gpadc_conversion(MAIN_CHARGER_V);
	/* conversion from RAW val to voltage
	 * 20250 * X / 1023
	 */
	data = (20250 * data) / 1023;
	return data;
}

static int ab8500_bm_usb_voltage(struct ab8500_bm_device_info *di)
{
	int data;

	data = ab8500_gpadc_conversion(VBUS_V);
	/* conversion from RAW data to voltage
	 * 20250 * X / 1023
	 */
	data = (20250 * data) / 1023;
	return data;
}

static int ab8500_bm_voltage(struct ab8500_bm_device_info *di)
{
	int data;

	data = ab8500_gpadc_conversion(MAIN_BAT_V);
	/* conversion from RAW value to voltage
	 * 2300 + 2500*X / 1023
	 */
	data = 2300 + ((2500 * data) / 1023);
	return data;
}

static int ab8500_bm_temperature(struct ab8500_bm_device_info *di)
{
	int data, i = 0;
	int tmp;
	struct temperature {
		int raw;
		int uv;
		int gain_op;
		int gain_ip;
	};
	const struct temperature temp[11] = {
		{20, 370, 100, 45},
		{42, 347, 100, 100},
		{67, 334, 100, 200},
		{87, 327, 100, 300},
		{126, 317, 100, 400},
		{190, 306, 100, 625},
		{257, 298, 100, 875},
		{358, 298, 100, 1175},
		{491, 280, 100, 1535},
		{636, 272, 100, 1850},
		{1027, 252, 100, 2075},
	};
	/* Refer Table-5 in
	 * MontBlanc Energy Management System Software Specification v1 41 stripped down.pdf */

	/* Raw Value    Kelvin(Unit Value)      Gain
	 * 1027         252                     100/2075
	 * 636          272                     100/1850
	 * 491          280                     100/1535
	 * 358          298                     100/1175
	 * 257          298                     100/875
	 * 190          306                     100/625
	 * 126          317                     100/400
	 * 87           327                     100/300
	 * 67           334                     100/200
	 * 42           347                     100/100
	 * 20           370                     100/45
	 */

	data = ab8500_gpadc_conversion(BTEMP_BALL);

	/*
	 * Conversion function for each sub-range
	 * UV = UV higher + (RAW higher - RAW measured)*Gain
	 */
	while (data > temp[i].raw)
		i++;
	tmp = temp[i].uv +
	    ((((temp[i].raw -
		data) * temp[i].gain_op * 10000) / temp[i].gain_ip) / 10000);
	/* convert kelvin to deg cel.
	 * Tc = Tk - 273.15
	 */
	tmp = tmp - 273;
	return tmp;
}

static int ab8500_bm_current(struct ab8500_bm_device_info *di)
{
	/* TODO: Batt current conversion is not available
	 * from GPADC, gas guage to be used for this
	 */
	return 1;
}

static void ab8500_bm_battery_read_status(struct ab8500_bm_device_info *di)
{
	int val, status, ret = 0;

	di->temp_C = ab8500_bm_temperature(di);
	di->voltage_uV = ab8500_bm_voltage(di);
	di->current_uA = ab8500_bm_current(di);

	/* Li-ion batteries gets charged upto 4.2v, hence 4.2v should be one of
	 * the criteria for termination charging. Another factor to terminate the
	 * the charging is the CV method, on occurance of Constant Voltage charging
	 * the termination current has to be checked. For this the integrated
	 * current has to be in the range of 50-200mA
	 */
	if (di->charge_status == POWER_SUPPLY_STATUS_CHARGING) {
		if (di->voltage_uV > 4175 && di->voltage_uV < 4200) {
			/* Check if charging is in constant voltage */
			val =
			    ab8500_read(AB8500_CHARGER, AB8500_CH_STATUS1_REG);
			if ((val | 0x04) == val) {
				/* Check for termination current */
				/* TODO: Need to verify this from for valid
				 * units from the gas guage
				 */
				if (di->current_uA > 5000
				    && di->current_uA < 20000) {
					/* TODO: Revisit need to use
					 * ab8500_bm_status()
					 */
					status = ab8500_bm_charger_presence(di);
					if ((status ==
					    (AC_PW_CONN + USB_PW_CONN))
					    || (status == AC_PW_CONN))
						ret =
						    ab8500_bm_ac_en(di, false);
					else
						ret =
						    ab8500_bm_usb_en(di, false);
					di->charge_status =
					    POWER_SUPPLY_STATUS_FULL;
				}
				if (ret < 0)
					dev_dbg(di->dev,
						"Battery reached threshold, failed to stop charging\n");
			}
		}
	}
}

static void ab8500_bm_battery_update_status(struct ab8500_bm_device_info *di)
{
	int val = 0;

	ab8500_bm_battery_read_status(di);
	di->charge_status = POWER_SUPPLY_STATUS_UNKNOWN;
	/* Battery voltage > BattOVV or charging stopped by Btemp indicator */
	if (power_supply_am_i_supplied(&di->bat)) {
		val = ab8500_read(AB8500_CHARGER, AB8500_CH_STAT_REG);
		if ((val | 0x08) == val)
			di->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else
			di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
	} else
		di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;
}

static void ab8500_bm_battery_work(struct work_struct *work)
{
	struct ab8500_bm_device_info *di = container_of(work,
							struct
							ab8500_bm_device_info,
							ab8500_bm_monitor_work.work);

	ab8500_bm_battery_update_status(di);
	queue_delayed_work(di->ab8500_bm_wq, &di->ab8500_bm_monitor_work, 100);
}

static int ab8500_bm_bk_battery_voltage(struct ab8500_bm_device_info *di)
{
	int data;

	data = ab8500_gpadc_conversion(BK_BAT_V);
	/* conversion from RAW value to voltage
	 * 2300 + 2500*X / 1023
	 */
	data = 2300 + ((2500 * data) / 1023);
	return data;
}

static void ab8500_bm_watchdog_kick_work(struct work_struct *work)
{
	struct ab8500_bm_device_info *di = container_of(work,
							struct
							ab8500_bm_device_info,
							ab8500_bm_watchdog_work.work);
	/* Kickoff main watchdog */
	ab8500_write(AB8500_CHARGER, AB8500_CHARG_WD_CTRL, 0x01);
	queue_delayed_work(di->ab8500_bm_wd_kick_wq,
			   &di->ab8500_bm_watchdog_work, 300);
}

/*
 * Power ON/OFF charging LED indication
 */
static int ab8500_bm_led_en(struct ab8500_bm_device_info *di, int on)
{
	int ret;

	if (on) {
		/* Power ON charging LED indicator, set LED current to 5mA */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_LED_INDICATOR_PWM_CTRL,
				 0x05);
		if (ret) {
			dev_vdbg(di->dev,
				 "ab8500_bm_led_en: power ON LED failed\n");
			return ret;
		}
		/* LED indicator PWM duty cycle */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_LED_INDICATOR_PWM_DUTY,
				 0xBF);
		if (ret) {
			dev_vdbg(di->dev,
				 "ab8500_bm_led_en: set LED PWM duty cycle failed\n");
			return ret;
		}
	} else {
		/* Power off charging LED indicator */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_LED_INDICATOR_PWM_CTRL,
				 0x00);
		if (ret) {
			dev_vdbg(di->dev,
				 "ab8500_bm_led_en: power-off LED failed\n");
			return ret;
		}
	}

	return ret;
}

/*
 * ab8500_bm_charger_presence() - check for the presence of charger
 * @di:	pointer to the ab8500 bm device information structure
 *
 * Returns an integer value, that means,
 * NO_PW_CONN  no power supply is connected
 * AC_PW_CONN  if the AC power supply is connected
 * USB_PW_CONN  if the USB power supply is connected
 * AC_PW_CONN + USB_PW_CONN if USB and AC power supplies are both connected
 *
 */
static int ab8500_bm_charger_presence(struct ab8500_bm_device_info *di)
{
	int ret = NO_PW_CONN, val_ac = 0, val_usb = 0;
	mutex_lock(&ab8500_bm_lock);
	/* ITSource2 bit3 MainChPlugDet */
	val_ac = ab8500_read(AB8500_INTERRUPT, AB8500_IT_SOURCE2_REG);
	/* ITSource21 bit 6 UsbChgDetDone */
	val_usb = ab8500_read(AB8500_INTERRUPT, AB8500_IT_SOURCE21_REG);

	if ((val_ac | 0x08) == val_ac)
		ret = AC_PW_CONN;
	if (((val_usb | 0x40) == val_usb) || ((val_ac | 0x80) == val_ac)) {
		if (ret == AC_PW_CONN)
			ret = AC_PW_CONN + USB_PW_CONN;
		else
			ret = USB_PW_CONN;
	}
	mutex_unlock(&ab8500_bm_lock);
	dev_vdbg(di->dev, "Return value of charger presence = %d\n", ret);
	return ret;
}

/*
 Enable/Disable AC charging
 Returns 0 on success and -1 on error
 */
static int ab8500_bm_ac_en(struct ab8500_bm_device_info *di, int enable)
{
	int ret = 0;
	if (enable) {
		/* Check if AC is connected */
		ret = ab8500_bm_charger_presence(di);
		if (ret < 0) {
			dev_vdbg(di->dev, "AC charger presence check failed\n");
			return ret;
		}
		if ((ret != AC_PW_CONN) && (ret != (AC_PW_CONN + USB_PW_CONN))) {
			dev_vdbg(di->dev, "AC charger not connected\n");
			return -ENXIO;
		}
		ret = 0;
		/* BattOVV threshold = 4.75v */
		ret = ab8500_write(AB8500_CHARGER, AB8500_BATT_OVV, 0x03);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}

		/* Enable AC charging */

		/* ChVoltLevel = 4.1v */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_CH_VOLT_LVL_REG, 0x18);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		/* MainChInputCurr = 900mA */
		ret = ab8500_write(AB8500_CHARGER, AB8500_MCH_IPT_CURLVL_REG,
				   0x80);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		/* ChOutputCurentLevel = 0.9A */
		ret = ab8500_write(AB8500_CHARGER, AB8500_CH_OPT_CRNTLVL_REG,
				   0x08);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		/* Enable Main Charger */
		ret = ab8500_write(AB8500_CHARGER, AB8500_MCH_CTRL1, 0x01);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		/* Enable main watchdog */
		ret = ab8500_write(AB8500_SYS_CTRL2_BLOCK,
				   AB8500_MAIN_WDOG_CTRL_REG, 0x01);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		ret = ab8500_write(AB8500_SYS_CTRL2_BLOCK,
				   AB8500_MAIN_WDOG_CTRL_REG, 0x03);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		/* Disable main watchdog */
		ret = ab8500_write(AB8500_SYS_CTRL2_BLOCK,
				   AB8500_MAIN_WDOG_CTRL_REG, 0x00);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		/* Kickoff main watchdog */
		ret = ab8500_write(AB8500_CHARGER, AB8500_CHARG_WD_CTRL, 0x01);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}

		/*If success power on charging LED indication */
		if (ret == 0) {
			ret = ab8500_bm_led_en(di, true);
			if (ret < 0)
				dev_vdbg(di->dev, "failed to enable LED\n");
		}
		/* Schedule delayed work to re-kick watchdog */
		queue_delayed_work(di->ab8500_bm_wd_kick_wq,
				   &di->ab8500_bm_watchdog_work, 300);
	} else {
		/* Disable AC charging */
		ret = ab8500_write(AB8500_CHARGER, AB8500_MCH_CTRL1, 0x00);
		/* If success power off charging LED indication */
		if (ret == 0) {
			/* Check if USB charger is presenc if so then dont
			 * turn off the LED, USB charging is taking place
			 */
			ret = ab8500_bm_charger_presence(di);
			if (ret != USB_PW_CONN) {
				ret = ab8500_bm_led_en(di, false);
				if (ret < 0)
					dev_vdbg(di->dev,
						 "failed to disable LED\n");
			}
		}
		/* Schedule delayed work to re-kick watchdog */
		cancel_delayed_work(&di->ab8500_bm_watchdog_work);
	}
	return ret;
}

/*
 Enable/Disable USB charging
 Returns 0 on success and -1 on error
 */
static int ab8500_bm_usb_en(struct ab8500_bm_device_info *di, int enable)
{
	int ret = 0;

	if (enable) {
		/* Check if USB is connected */
		ret = ab8500_bm_charger_presence(di);
		if (ret < 0) {
			dev_vdbg(di->dev,
				 "USB charger presence check failed\n");
			return ret;
		}
		if ((ret != USB_PW_CONN) && (ret != (AC_PW_CONN + USB_PW_CONN))) {
			dev_vdbg(di->dev, "USB charger not connected\n");
			return -ENXIO;
		}
		/* BattOVV threshold = 4.75v */
		ret = ab8500_write(AB8500_CHARGER, AB8500_BATT_OVV, 0x03);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}

		/* Enable USB charging */

		/* ChVoltLevel = 4.1v */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_CH_VOLT_LVL_REG, 0x18);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		/* USBChInputCurr = 500mA */
		ret = ab8500_write(AB8500_CHARGER, AB8500_USBCH_IPT_CRNTLVL_REG,
				   0x60);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		/* ChOutputCurentLevel = 0.9A */
		ret = ab8500_write(AB8500_CHARGER, AB8500_CH_OPT_CRNTLVL_REG,
				   0x08);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		/* Enable USB Charger */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_USBCH_CTRL1_REG, 0x01);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		/* Enable main watchdog */
		ret = ab8500_write(AB8500_SYS_CTRL2_BLOCK,
				   AB8500_MAIN_WDOG_CTRL_REG, 0x01);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		ret = ab8500_write(AB8500_SYS_CTRL2_BLOCK,
				   AB8500_MAIN_WDOG_CTRL_REG, 0x03);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		/* Disable main watchdog */
		ret = ab8500_write(AB8500_SYS_CTRL2_BLOCK,
				   AB8500_MAIN_WDOG_CTRL_REG, 0x00);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		/* Kickoff main watchdog */
		ret = ab8500_write(AB8500_CHARGER, AB8500_CHARG_WD_CTRL, 0x01);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		/* If success power on charging LED indication */
		if (ret == 0) {
			ret = ab8500_bm_led_en(di, true);
			if (ret < 0)
				dev_vdbg(di->dev, "failed to enable LED\n");
		}
		/* Schedule delayed work to re-kick watchdog */
		queue_delayed_work(di->ab8500_bm_wd_kick_wq,
				   &di->ab8500_bm_watchdog_work, 300);
	} else {
		/* Disable USB charging */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_USBCH_CTRL1_REG, 0x00);
		/* If sucussful power off charging LED indication */
		if (ret == 0) {
			/* Check if AC charger is present if so then dont
			 * turn off the LED, AC charging is taking place
			 */
			ret = ab8500_bm_charger_presence(di);
			if (ret != AC_PW_CONN) {
				ret = ab8500_bm_led_en(di, false);
				if (ret < 0)
					dev_vdbg(di->dev,
						 "failed to disable LED\n");
			}
		}
		/* Schedule delayed work to re-kick watchdog */
		cancel_delayed_work(&di->ab8500_bm_watchdog_work);
	}
	return ret;
}

/*
 Enable/Disable hardware level and presence detection
 */
static int ab8500_bm_hw_presence_en(struct ab8500_bm_device_info *di,
				    int enable)
{
	int val = 0, ret = 0;

	/* OTP: Enable Watchdog */
	val = ab8500_read(AB8500_OTP_EMUL, 0x150E);
	ret = ab8500_write(AB8500_OTP_EMUL, 0x150E,
			   (enable ? (val | 0x01) : (val & 0xFE)));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}
	ret = ab8500_write(AB8500_OTP_EMUL, 0x1504, (enable ? (0xC0) : (0x00)));

	/* Not OK main charger detection */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK1_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK1_REG,
			   (enable ? (val & 0xFE) : (val | 0x01)));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}

	/* Main charge plug/unplug and battery OVV detection
	 * VBUS rising/falling edge detect
	 */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK2_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK2_REG,
			   (enable ? (val & 0x30) : (val | 0xDE)));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}

	/* Battery removal, Watchdog expiration, vbus overvoltage */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK3_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK3_REG,
			   (enable ? (val & 0x8F) : (val | 0x70)));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}

	/* Battert temp below/above low/high threshold */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK19_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK19_REG,
			   (enable ? (val & 0xF6) : (val | 0x09)));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}

	/* Not OK USB detected */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK20_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK20_REG,
			   (enable ? (val & 0xFD) : (val | 0x02)));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}

	/* USB charger detection */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK21_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK21_REG,
			   (enable ? (val & 0xBF) : (val | 0x40)));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}

	/* Die temp above USB/AC charger thermal protection,
	 * USB charger unplug detect
	 * TODO: Die temp has been masked.
	 */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK22_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK22_REG,
			   (enable ? (val & 0x7F) : (val | 0x8A)));
	if (ret)
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
	return ret;
}

/*
 Returns the type of charge AC/USB
 This is called by the battery property "POWER_SUPPLY_PROP_ONLINE"
 */
static int ab8500_bm_status(void)
{
	int val = 0, ret = 0;

	/* Check for AC charging */
	val = ab8500_read(AB8500_CHARGER, AB8500_CH_STATUS1_REG);
	if ((val & 0x02) == 0x02)
		ret = AC_PW_CONN;
	/* Check for USB charging */
	/* TODO: USB Charger is on bit 3 on Reg USBChStatus1 is not
	 * working - Revisit
	 */
	val = ab8500_read(AB8500_CHARGER, AB8500_CH_USBCH_STAT1_REG);
	if ((val & 0x03) == 0x03)
		ret = USB_PW_CONN;
	return ret;
}

#define to_ab8500_bm_device_info(x) container_of((x), \
	struct ab8500_bm_device_info, bat);

static int ab8500_bm_get_battery_property(struct power_supply *psy,
					  enum power_supply_property psp,
					  union power_supply_propval *val)
{
	struct ab8500_bm_device_info *di;
	int status = 0;

	di = to_ab8500_bm_device_info(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		ab8500_bm_battery_update_status(di);
		val->intval = di->charge_status;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		/* Battery presence AB8500 register set doesnt provide
		 * any register to check the presence of battery by
		 * default its value is '1'
		 */
		val->intval = 0x01;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		status = ab8500_bm_status();
		if (status == AC_PW_CONN)
			val->intval = POWER_SUPPLY_TYPE_MAINS;
		else if (status == USB_PW_CONN)
			val->intval = POWER_SUPPLY_TYPE_USB;
		else
			val->intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		status = ab8500_read(AB8500_CHARGER, AB8500_BATT_OVV);
		if ((status | 0x01) == status)
			val->intval = 4750000;
		else
			val->intval = 3700000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		/* Voltage is in terms of uV */
		val->intval = (di->voltage_uV * 1000);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		/* TODO: Update this from gas guage driver */
		val->intval = di->current_uA;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		/* TODO: Need to update this from gauguage driver */
		val->intval = 10000;
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		/* TODO: Need to calculate the present energu and update
		 * the same in uWh from gasguage driver
		 */
		val->intval = 10000;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		/* TODO:
		 * need to get the correct percentage value per the
		 * battery characteristics. Approx values for now.
		 */
		if (di->voltage_uV < 2894)
			val->intval = 5;
		else if (di->voltage_uV < 3451 && di->voltage_uV > 2894)
			val->intval = 20;
		else if (di->voltage_uV < 3702 && di->voltage_uV > 3451)
			val->intval = 40;
		else if (di->voltage_uV < 3902 && di->voltage_uV > 3702)
			val->intval = 50;
		else if (di->voltage_uV < 3949 && di->voltage_uV > 3902)
			val->intval = 75;
		else if (di->voltage_uV > 3949)
			val->intval = 90;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = di->temp_C;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

#define to_ab8500_bm_bk_battery_device_info(x) container_of((x), \
	struct ab8500_bm_device_info, bk_bat);

static int ab8500_bm_bk_battery_get_property(struct power_supply *psy,
					     enum power_supply_property psp,
					     union power_supply_propval *val)
{
	struct ab8500_bm_device_info *di;

	di = to_ab8500_bm_bk_battery_device_info(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = di->bk_battery_charge_status;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		/* Voltage is measured in terms of uV */
		val->intval = (ab8500_bm_bk_battery_voltage(di) * 1000);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

#define to_ab8500_bm_ac_device_info(x) container_of((x), \
	struct ab8500_bm_device_info, ac);

static int ab8500_bm_ac_get_property(struct power_supply *psy,
				     enum power_supply_property psp,
				     union power_supply_propval *val)
{
	struct ab8500_bm_device_info *di;
	int status = 0;

	di = to_ab8500_bm_ac_device_info(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		status = ab8500_bm_charger_presence(di);
		if ((status == (AC_PW_CONN | USB_PW_CONN)) || (status == AC_PW_CONN))
			val->intval = 0x01;
		else
			val->intval = 0x00;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		status = ab8500_bm_status();
		if (status == AC_PW_CONN)
			val->intval = 0x01;
		else
			val->intval = 0x00;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		/* Voltage is measured in terms of uV */
		val->intval = (ab8500_bm_ac_voltage(di) * 1000);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

#define to_ab8500_bm_usb_device_info(x) container_of((x), \
	struct ab8500_bm_device_info, usb);

static int ab8500_bm_usb_get_property(struct power_supply *psy,
				      enum power_supply_property psp,
				      union power_supply_propval *val)
{
	struct ab8500_bm_device_info *di;
	int status = 0;

	di = to_ab8500_bm_usb_device_info(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		status = ab8500_bm_charger_presence(di);
		if (status == USB_PW_CONN)
			val->intval = 0x01;
		else
			val->intval = 0x00;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		status = ab8500_bm_status();
		if (status == USB_PW_CONN)
			val->intval = 0x01;
		else
			val->intval = 0x00;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		/* Voltage is measured in terms of uV */
		val->intval = (ab8500_bm_usb_voltage(di) * 1000);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

#if defined(CONFIG_PM)
static int ab8500_bm_resume(struct platform_device *pdev)
{
	struct ab8500_bm_device_info *di = platform_get_drvdata(pdev);

	queue_delayed_work(di->ab8500_bm_wq, &di->ab8500_bm_monitor_work, 0);
	return 0;
}

static int ab8500_bm_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct ab8500_bm_device_info *di = platform_get_drvdata(pdev);

	di->charge_status = POWER_SUPPLY_STATUS_UNKNOWN;
	cancel_delayed_work(&di->ab8500_bm_monitor_work);
	return 0;
}
#else
#define ab8500_bm_suspend	NULL
#define ab8500_bm_resume	NULL
#endif

static int __devexit ab8500_bm_remove(struct platform_device *pdev)
{
	struct ab8500_bm_device_info *di = platform_get_drvdata(pdev);
	int ret;

	ret = ab8500_bm_ac_en(di, false);
	if (ret)
		dev_err(&pdev->dev, "failed to disable AC charging\n");
	ret = ab8500_bm_usb_en(di, false);
	if (ret)
		dev_err(&pdev->dev, "failed to disable USB charging\n");
	ret = ab8500_bm_hw_presence_en(di, false);
	if (ret)
		dev_err(&pdev->dev,
			"failed to disable h/w presence(i.e mask int)\n");

	/* Unregister callback handler */
	ret = ab8500_bm_register_handler(false, NULL);
	if (ret)
		dev_err(&pdev->dev, "failed to unregister callback handlers\n");

	/* Delete the work queue */
	destroy_workqueue(di->ab8500_bm_wq);
	destroy_workqueue(di->ab8500_bm_wd_kick_wq);
	destroy_workqueue(di->ab8500_bm_irq);

	flush_scheduled_work();
	power_supply_unregister(&di->ac);
	power_supply_unregister(&di->usb);
	power_supply_unregister(&di->bat);
	platform_set_drvdata(pdev, NULL);
	kfree(di);

	return 0;
}

static void ab8500_bm_external_power_changed(struct power_supply *psy)
{
	struct ab8500_bm_device_info *di = to_ab8500_bm_device_info(psy);

	cancel_delayed_work(&di->ab8500_bm_monitor_work);
	queue_delayed_work(di->ab8500_bm_wq, &di->ab8500_bm_monitor_work, 0);
}

static enum power_supply_property ab8500_bm_bk_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static enum power_supply_property ab8500_bm_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_ENERGY_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property ab8500_bm_ac_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static enum power_supply_property ab8500_bm_usb_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static char *ab8500_bm_supplied_to[] = {
	"ab8500_bm_battery",
};

static int __devinit ab8500_bm_probe(struct platform_device *pdev)
{
	/* TODO: This might be required for storing the temp calibration data
	   struct ab8500_bm_platform_data *pdata = pdev->dev.platform_data;
	 */
	struct ab8500_bm_device_info *di;
	int ret = 0;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;
	/* Main Battery */
	di->dev = &pdev->dev;
	di->bat.name = "ab8500_bm_battery";
	di->bat.supplied_to = ab8500_bm_supplied_to;
	di->bat.num_supplicants = ARRAY_SIZE(ab8500_bm_supplied_to);
	di->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties = ab8500_bm_battery_props;
	di->bat.num_properties = ARRAY_SIZE(ab8500_bm_battery_props);
	di->bat.get_property = ab8500_bm_get_battery_property;
	di->bat.external_power_changed = ab8500_bm_external_power_changed;
	di->charge_status = POWER_SUPPLY_STATUS_UNKNOWN;
	di->bk_battery_charge_status = POWER_SUPPLY_STATUS_UNKNOWN;

	/* Backup Battery */
	di->bk_bat.name = "ab8500_backup_battery";
	di->bk_bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bk_bat.properties = ab8500_bm_bk_battery_props;
	di->bk_bat.num_properties = ARRAY_SIZE(ab8500_bm_bk_battery_props);
	di->bk_bat.get_property = ab8500_bm_bk_battery_get_property;
	di->bk_bat.external_power_changed = NULL;

	/* AC supply */
	di->ac.name = "ab8500_ac";
	di->ac.type = POWER_SUPPLY_TYPE_MAINS;
	di->ac.properties = ab8500_bm_ac_props;
	di->ac.num_properties = ARRAY_SIZE(ab8500_bm_ac_props);
	di->ac.get_property = ab8500_bm_ac_get_property;
	di->ac.external_power_changed = NULL;

	/* USB supply */
	di->usb.name = "ab8500_usb";
	di->usb.type = POWER_SUPPLY_TYPE_USB;
	di->usb.properties = ab8500_bm_usb_props;
	di->usb.num_properties = ARRAY_SIZE(ab8500_bm_usb_props);
	di->usb.get_property = ab8500_bm_usb_get_property;
	di->usb.external_power_changed = NULL;

	/* Create a work queue for monitoring the battery */
	di->ab8500_bm_wq = create_singlethread_workqueue("ab8500_bm_wq");
	if (di->ab8500_bm_wq == NULL) {
		dev_err(&pdev->dev, "failed to create work queue\n");
		goto free_wq;
	}

	/* Create a work queue for en/dis AC/USB from irq handler */
	di->ab8500_bm_irq = create_singlethread_workqueue("ab8500_bm_irq");
	if (di->ab8500_bm_irq == NULL) {
		dev_err(&pdev->dev, "failed to create work queue\n");
		goto free_wq;
	}

	/* Create a work queue for rekicking the watchdog */
	di->ab8500_bm_wd_kick_wq =
	    create_singlethread_workqueue("ab8500_bm_wd_kick_wq");
	if (di->ab8500_bm_wd_kick_wq == NULL) {
		dev_err(&pdev->dev, "failed to create work queue\n");
		goto free_wq;
	}

	/* Work Queue to re-kick the charging watchdog */
	INIT_DELAYED_WORK_DEFERRABLE(&di->ab8500_bm_watchdog_work,
				     ab8500_bm_watchdog_kick_work);

	/* Monitor Main Battery */
	INIT_DELAYED_WORK_DEFERRABLE(&di->ab8500_bm_monitor_work,
				     ab8500_bm_battery_work);

	/* Init work queue for enable/desable AC/USB charge */
	INIT_WORK(&di->ab8500_bm_ac_en_monitor_work, ab8500_bm_ac_en_work);
	INIT_WORK(&di->ab8500_bm_ac_dis_monitor_work, ab8500_bm_ac_dis_work);
	INIT_WORK(&di->ab8500_bm_usb_en_monitor_work, ab8500_bm_usb_en_work);
	INIT_WORK(&di->ab8500_bm_usb_dis_monitor_work, ab8500_bm_usb_dis_work);

	/* Enable AC charging */
	ret = ab8500_bm_ac_en(di, true);
	if (ret)
		dev_err(&pdev->dev, "failed to enable AC charging\n");
#if defined(CONFIG_USB_CHARGER)
	/* Enable USB charging */
	ret = ab8500_bm_usb_en(di, true);
	if (ret)
		dev_err(&pdev->dev, "failed to enable USB charging\n");
#endif
	platform_set_drvdata(pdev, di);

	/* Register callback handlers */
	ret = ab8500_bm_register_handler(true, di);
	if (ret) {
		dev_err(&pdev->dev, "Registering callback handler failed\n");
		goto irq_fail;
	}

	/* Register main battery */
	ret = power_supply_register(&pdev->dev, &di->bat);
	if (ret) {
		dev_err(&pdev->dev, "failed to register main battery\n");
		goto batt_fail;
	}

	/* Monitor Main Battery */
	queue_delayed_work(di->ab8500_bm_wq, &di->ab8500_bm_monitor_work, 0);
	/* Register AC charger class */
	ret = power_supply_register(&pdev->dev, &di->ac);
	if (ret) {
		dev_err(&pdev->dev, "failed to register backup battery\n");
		goto ac_fail;
	}

	/* Register USB charger class */
	ret = power_supply_register(&pdev->dev, &di->usb);
	if (ret) {
		dev_err(&pdev->dev, "failed to register backup battery\n");
		goto usb_fail;
	}

	/*
	 * Hardware level detection
	 * Battery/AC/USB presence detection
	 */
	ret = ab8500_bm_hw_presence_en(di, true);
	if (ret) {
		dev_err(&pdev->dev,
			"failed to enable h/w presence(i.e mask int)\n");
		goto intr_fail;
	}
	return 0;
intr_fail:
	ret = ab8500_bm_hw_presence_en(di, false);
	if (ret)
		dev_err(&pdev->dev,
			"failed to enable h/w presence(i.e mask int)\n");
ac_fail:
	power_supply_unregister(&di->ac);
usb_fail:
	power_supply_unregister(&di->usb);
batt_fail:
	power_supply_unregister(&di->bat);
irq_fail:
	ret = ab8500_bm_register_handler(false, NULL);
	if (ret)
		dev_err(&pdev->dev, "failed to unregister callback handlers\n");
	ret = ab8500_bm_ac_en(di, false);
	if (ret)
		dev_err(&pdev->dev, "failed to disable AC charging\n");
	ret = ab8500_bm_usb_en(di, false);
	if (ret)
		dev_err(&pdev->dev, "failed to disable USB charging\n");
free_wq:
	/* Delete the work queue */
	destroy_workqueue(di->ab8500_bm_wq);
	destroy_workqueue(di->ab8500_bm_wd_kick_wq);
	destroy_workqueue(di->ab8500_bm_irq);
	kfree(di);

	return ret;
}

static struct platform_driver ab8500_bm_driver = {
	.probe = ab8500_bm_probe,
	.remove = __exit_p(ab8500_bm_remove),
	.suspend = ab8500_bm_suspend,
	.resume = ab8500_bm_resume,
	.driver = {
		   .name = "ab8500_bm",
		   },
};

static int __init ab8500_bm_init(void)
{
	return platform_driver_register(&ab8500_bm_driver);
}

static void __exit ab8500_bm_exit(void)
{
	platform_driver_unregister(&ab8500_bm_driver);
}

module_init(ab8500_bm_init);
module_exit(ab8500_bm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("STEricsson, Arun R Murthy");
MODULE_ALIAS("Platform:ab8500-bm");
MODULE_DESCRIPTION("AB8500 battery management driver");
