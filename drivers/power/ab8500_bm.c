/*
 * ab8500_bm.c - AB8500 Battery Management Driver
 *
 * Copyright (C) 2010 ST-Ericsson SA
 * Licensed under GPLv2.
 *
 * Author: Arun R Murthy <arun.murthy@stericsson.com>
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
#include <linux/kobject.h>
#include <mach/ab8500.h>
#include <mach/ab8500_bm.h>
#include <mach/ab8500_gpadc.h>
#include <mach/ab8500_sys.h>

#include <drivers/usb/musb/stm_musb.h>

/* Interrupt */
#define MAIN_EXT_CH_NOT_OK	0
#define BATT_OVV		8
#define MAIN_CH_UNPLUG_DET	10
#define MAIN_CHARGE_PLUG_DET	11
#define VBUS_DET_F		14
#define VBUS_DET_R		15
#define BAT_CTRL_INDB		20
#define CH_WD_EXP		21
#define VBUS_OVV		22
#define NCONV_ACCU  		24
#define LOW_BAT_F		28
#define LOW_BAT_R		29
#define GP_SW_ADC_CONV_END	39
#define BTEMP_LOW		72
#define BTEMP_HIGH		75
#define USB_CHARGER_NOT_OKR	81
#define USB_CHARGE_DET_DONE	94
#define USB_CH_TH_PROT_R	97
#define MAIN_CH_TH_PROT_R	99
#define USB_CHARGER_NOT_OKF	103

/* Charger constants */
#define NO_PW_CONN	0
#define AC_PW_CONN	1
#define USB_PW_CONN	2

#define to_ab8500_bm_usb_device_info(x) container_of((x), \
		struct ab8500_bm_device_info, usb);
#define to_ab8500_bm_ac_device_info(x) container_of((x), \
		struct ab8500_bm_device_info, ac);
#define to_ab8500_bm_bk_battery_device_info(x) container_of((x), \
		struct ab8500_bm_device_info, bk_bat);
#define to_ab8500_bm_device_info(x) container_of((x), \
		struct ab8500_bm_device_info, bat);

static DEFINE_MUTEX(ab8500_bm_lock);
static DEFINE_MUTEX(ab8500_cc_lock);

/* Structure - Device Information
 * struct ab8500_bm_device_info - device information
 * @dev:				Pointer to the structure device
 * @voltage_uV:				Battery voltage in uV
 * @temp_C:				Battery temperature
 * @inst_current:			Battery instantenous current
 * @avg_current:			Battery average current
 * @capacity:				Battery capacity
 * @charge_status:			Charger status(charging, discharging)
 * @bk_battery_charge_status:		Backup battery charger status
 * @usb_ip_cur_lvl:			USB charger i/p current level
 * @usb_mA				current that can be drawn, obtained
 * 					from usb stack.
 * @usb_state				USB state obtained from the usb stack
 * @maintenance_chg:			flag for enabling/disabling
 * 					maintenance charging
 * @usb_changed				flag to indicate that usb stack has
 * 					some updates on the usb state
 * @usb_plug_unplug			flag to indicate the event as usb
 *					plug ot unplug
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
 * @ab8500_bm_avg_cur_monitor_work:	Work to monitor the average current
 * @ab8500_bm_usb_state_changed_monitor_work
 * 					Work to enable/disable usb charging
 * 					based on the info from usb stack
 * @ab8500_bm_wq:			Pointer to work queue-battery mponitor
 * @ab8500_bm_irq:			Pointer to the work queue-
 * 					enable/disable chargin from irq
 * 					handler
 * @ab8500_bm_wd_kick_wq:		Pointer to work queue-rekick watchdog
 * @ab8500_bm_kobject			structure of type kobject
 * @ab8500_bm_spin_lock			spin lock to lock the usb changed flag
 */
struct ab8500_bm_device_info {
	struct device *dev;
	int voltage_uV;
	int temp_C;
	int inst_current;
	int avg_current;
	int capacity;
	int charge_status;
	int bk_battery_charge_status;
	int usb_ip_cur_lvl;
	int usb_mA;
	int usb_state;
	bool maintenance_chg;
	bool usb_changed;
	bool usb_plug_unplug;
	struct ab8500_bm_platform_data *pdata;
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
	struct work_struct ab8500_bm_avg_cur_monitor_work;
	struct work_struct ab8500_bm_usb_state_changed_monitor_work;
	struct workqueue_struct *ab8500_bm_wq;
	struct workqueue_struct *ab8500_bm_irq;
	struct workqueue_struct *ab8500_bm_wd_kick_wq;
	struct kobject ab8500_bm_kobject;
	spinlock_t ab8500_bm_spin_lock;
	struct completion ab8500_bm_usb_completed;
};

/* structure - battery capacity based on voltage
 * @vol_mV:	voltage in mV
 * @capacity	capacity corresponding to the voltage
 */
struct ab8500_bm_capacity {
	int vol_mV;
	int capacity;
};

/* voltage based capacity */
static struct ab8500_bm_capacity ab8500_bm_cap[] = {
	{2894, 5},
	{3451, 20},
	{3701, 40},
	{3902, 50},
	{3949, 75},
	{4150, 90},
	{4200, 100},
};

static char *ab8500_bm_supplied_to[] = {
	"ab8500_bm_battery",
};

static enum power_supply_property ab8500_bm_bk_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static enum power_supply_property ab8500_bm_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TECHNOLOGY,
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

/* Battery specific information */
struct ab8500_bm_device_info *di;

static int ab8500_bm_ac_en(struct ab8500_bm_device_info *, int);
static int ab8500_bm_usb_en(struct ab8500_bm_device_info *, int);
static int ab8500_bm_charger_presence(struct ab8500_bm_device_info *);
static int ab8500_bm_status(void);
static void ab8500_bm_battery_update_status(struct ab8500_bm_device_info *);

/* TODO: Need to remove this from here as this has to goto ab8500_sysfs.c */

static ssize_t ab8500_bm_sysfs_charger(struct kobject *kobj,
		struct attribute *attr, const char *buf, size_t length)
{
	struct ab8500_bm_device_info *di = container_of(kobj,
							struct
							ab8500_bm_device_info,
							ab8500_bm_kobject);
	int ac_usb, ret = 0;

	ac_usb = simple_strtol(buf, NULL, 10);
	switch (ac_usb) {
	case 0:
		/* Disable charging */
		ret = ab8500_bm_ac_en(di, false);
		ret = ab8500_bm_usb_en(di, false);
		if (ret < 0)
			dev_dbg(di->dev, "failed to disable charging\n");
		break;
	case 1:
		/* Enable AC Charging */
		ret = ab8500_bm_ac_en(di, true);
		if (ret < 0)
			dev_dbg(di->dev, "failed to enable AC charging\n");
		break;
	case 2:
		/* Enable USB charging */
		ret = ab8500_bm_usb_en(di, true);
		if (ret < 0)
			dev_dbg(di->dev, "failed to enable USB charging\n");
		break;
	default:
		dev_info(di->dev, "Wrong input\n"
				"Enter 0. Disable AC/USB Charging\n"
				"1. Enable AC charging\n"
				"2. Enable USB Charging\n");
	};
	return strlen(buf);
}

static struct attribute ab8500_bm_en_charger = \
				{
				.name = "charger",
				.mode = S_IWUGO,
				};

static struct attribute *ab8500_bm_chg[] = {
	&ab8500_bm_en_charger,
	NULL
};

static struct sysfs_ops ab8500_bm_sysfs_ops = {
	.store = ab8500_bm_sysfs_charger,
};

static struct kobj_type ab8500_bm_ktype = {
	.sysfs_ops = &ab8500_bm_sysfs_ops,
	.default_attrs = ab8500_bm_chg,
};

static void ab8500_bm_sysfs_exit(struct ab8500_bm_device_info *di)
{
	kobject_del(&di->ab8500_bm_kobject);
}

static int ab8500_bm_sysfs_init(struct ab8500_bm_device_info *di)
{
	int ret = 0;

	ret = kobject_init_and_add(&di->ab8500_bm_kobject, &ab8500_bm_ktype,
						NULL, "ab8500_charger");
	if (ret < 0) {
		dev_err(di->dev, "failed to create sysfs entry\n");
	}
	return ret;
}

/* END: Need to remove this from here as this has to goto ab8500_sysfs.c */

/**
 * ab8500_bm_get_usb_cur() - get usb current
 * @di:		pointer to the ab8500_bm_device_info structre
 *
 * The usb stack provides the i/p current that can be drawn from the standard
 * usb host. This will be in mA. This function converts current in mA to a
 * value that can be written to the register.
 **/
static void ab8500_bm_get_usb_cur(struct ab8500_bm_device_info *di)
{
	switch (di->usb_mA) {
	case USB_0P1A:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P09;
		break;
	case USB_0P2A:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P19;
		break;
	case USB_0P3A:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P29;
		break;
	case USB_0P4A:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P38;
		break;
	case USB_0P5A:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P5;
		break;
	default:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P09;
		break;
	};
}

/**
 * ab8500_bm_usb_state_changed() - get notified with the usb states
 * @bm_usb_state:       USB status(reset, config, high speed)
 * @mA:                 current in mA
 *
 * USB driver will call this API to notify the change in the USB state
 * to the battery driver. Based on this information battery driver will
 * take measurements to enable/disable charging.
 *
 **/
void ab8500_bm_usb_state_changed(u8 bm_usb_state, u16 mA)
{
	/* USB unplug event, disable charging */
	if (!di->usb_plug_unplug) {
		queue_work(di->ab8500_bm_irq,
				&di->ab8500_bm_usb_dis_monitor_work);
		return;
	}

	spin_lock(&di->ab8500_bm_spin_lock);
		di->usb_changed = true;
	spin_unlock(&di->ab8500_bm_spin_lock);

	di->usb_state = bm_usb_state;
	di->usb_mA = mA;

	queue_work(di->ab8500_bm_wd_kick_wq,
			&di->ab8500_bm_usb_state_changed_monitor_work);
	return;
}
EXPORT_SYMBOL(ab8500_bm_usb_state_changed);

/**
 * ab8500_bm_usb_state_changed_work() - work to get notified with usb state
 * @work:               Pointer to the work_struct structure
 *
 * USB driver will call this API to notify the change in the USB state
 * to the battery driver. Based on this informatio battery driver will
 * take measurements to enable/disable charging.
 *
 **/
void ab8500_bm_usb_state_changed_work(struct work_struct *work)
{
	int val;

	/* USB unplug event, disable charging */
	if (!di->usb_plug_unplug) {
		queue_work(di->ab8500_bm_irq,
				&di->ab8500_bm_usb_dis_monitor_work);
		return;
	}

	/* Before reading the UsbLineStatus register, ITSource 21
	 * register should be read
	 */
	do {
		val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_SOURCE21_REG);
		val = ab8500_read(AB8500_USB, AB8500_USB_LINE_STAT_REG);
		/* get the USB type */
		val = (val >> 3) & 0x0F;
		/* If the detected device is not s standard host then return
		 * as it has nothing to do with Host chargeror dedicated host
		 * or ACA.
		 */
	} while (!val);
	if (val > STANDARD_HOST || val < 0x01) {
		dev_info(di->dev, "Is not a standard host device\n");
		return;
	}
	spin_lock(&di->ab8500_bm_spin_lock);
		di->usb_changed = false;
	spin_unlock(&di->ab8500_bm_spin_lock);
	/* wait for some time until you get updates from the usb stack
	 * and negotiations are completed
	 */
	msleep(250);
	if (di->usb_changed)
		return;

	if (!di->usb_mA) {
		queue_work(di->ab8500_bm_irq,
				&di->ab8500_bm_usb_dis_monitor_work);
		return;
	}

	switch (di->usb_state) {
	case AB8500_BM_USB_STATE_RESET_HS:
		/* if cur > 0 enable charging else disable */
		dev_dbg(di->dev, "USB configured in HS mode\n");
		if (di->usb_mA > 0)
			queue_work(di->ab8500_bm_irq,
				&di->ab8500_bm_usb_dis_monitor_work);
		else
		queue_work(di->ab8500_bm_irq,
				&di->ab8500_bm_usb_dis_monitor_work);
		break;
	case AB8500_BM_USB_STATE_RESET_FS:
		/* if cur > 0 enable charging else disable */
		dev_dbg(di->dev, "USB configured in LS/FS mode\n");
		if (di->usb_mA > 0)
			queue_work(di->ab8500_bm_irq,
				&di->ab8500_bm_usb_dis_monitor_work);
		else
		queue_work(di->ab8500_bm_irq,
				&di->ab8500_bm_usb_dis_monitor_work);
		break;
	case AB8500_BM_USB_STATE_CONFIGURED:
		/* USB is configured, enable charging with the charging
		* input current obtained from USB driver
		*/
		dev_dbg(di->dev, "USB configured for standard host\n");
		complete(&di->ab8500_bm_usb_completed);
		break;
	case AB8500_BM_USB_STATE_SUSPEND:
		/* USB in suspend state */
		ab8500_bm_ulpi_set_char_suspend_mode
				(AB8500_BM_USB_STATE_SUSPEND);
		queue_work(di->ab8500_bm_irq,
				&di->ab8500_bm_usb_dis_monitor_work);
		dev_dbg(di->dev, "USB entering suspend mode\n");
		break;
	case AB8500_BM_USB_STATE_RESUME:
		/* USB in resume state */
		ab8500_bm_ulpi_set_char_speed_mode
				(AB8500_BM_USB_STATE_RESUME);
		/* when suspend->resume there should be delay
		 * of 1sec for enabling charging
		 */
		msleep(1000);
		dev_dbg(di->dev, "USB entering resume mode\n");
		queue_work(di->ab8500_bm_irq,
				&di->ab8500_bm_usb_en_monitor_work);
		break;
	case AB8500_BM_USB_STATE_MAX:
		/* USB in max state */
		dev_dbg(di->dev, "USB max state\n");
		break;
	default:
		break;
	};
	return;
}

/**
 * ab8500_bm_detect_usb_type() - get the type of usb connected
 * @di:		pointer to the ab8500_bm_device_info structure
 *
 * TODO: Need to remove this function as this data is
 * suppose to come from the USB driver.
 *
 * Detect the type of USB plugged in and based on the
 * type set the mac current and voltage
 **/

static int ab8500_bm_detect_usb_type(struct ab8500_bm_device_info *di)
{
	int val, cnt;

	/* On getting the VBUS rising edge detect interrupt there
	 * is a 250ms delay after which the register UsbLineStatus
	 * if filled with valid data
	 */
	for (cnt = 0; cnt < 10; cnt++) {
		msleep(250);
		val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_SOURCE21_REG);
		val = ab8500_read(AB8500_USB, AB8500_USB_LINE_STAT_REG);
		/* Until the IT source register is read the UsbLineStatus
		 * register is not updated, hence doing the same
		 * Revisit this:
		 */

		/* get the USB type */
		val = (val >> 3) & 0x0F;
	}
	if (!val && cnt == 10) {
		dev_err(di->dev, "unable to detect the usb type\n");
		return -1;
	}

	/* TODO: verify the i/p current levev for the types of USB */
	switch (val) {
	case USB_STAT_NOT_CONFIGURED:
		dev_dbg(di->dev, "USB Type - Not configured\n");
		break;
	case USB_STAT_STD_HOST_NC:
		wait_for_completion(&di->ab8500_bm_usb_completed);
		ab8500_bm_get_usb_cur(di);
		dev_dbg(di->dev, "USB Type - Standard Host, Not Charging\n");
		break;
	case USB_STAT_STD_HOST_C_NS:
		/* This is never being called */
		dev_info(di->dev, "This is never called, if called bug,,!!\n");
		wait_for_completion(&di->ab8500_bm_usb_completed);
		ab8500_bm_get_usb_cur(di);
		dev_dbg(di->dev, "USB Type - Standard Host, charging, not suspended\n");
		break;
	case USB_STAT_STD_HOST_C_S:
		/* This is never being called */
		dev_info(di->dev, "This is never called, if called bug,,!!\n");
		wait_for_completion(&di->ab8500_bm_usb_completed);
		ab8500_bm_get_usb_cur(di);
		dev_dbg(di->dev, "USB Type - Standard Host, charging, suspended\n");
		break;
	case USB_STAT_HOST_CHG_NM:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P29;
		dev_dbg(di->dev, "USB Type - Host charger, noraml mode\n");
		break;
	case USB_STAT_HOST_CHG_HS:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P29;
		dev_dbg(di->dev, "USB Type - Host charger, HS mode\n");
		break;
	case USB_STAT_HOST_CHG_HS_CHIRP:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P29;
		dev_dbg(di->dev, "USB Type - Host charger, HS Chirp mode\n");
		break;
	case USB_STAT_DEDICATED_CHG:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_1P5;
		dev_dbg(di->dev, "USB Type - Dedicated USB Charger\n");
		printk(KERN_ALERT "USB Type - Dedicated USB Charger\n");
		break;
	case USB_STAT_ACA_RID_A:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P09;
		dev_dbg(di->dev, "USB Type - ACA RID_A configuration\n");
		break;
	case USB_STAT_ACA_RID_B:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P09;
		dev_dbg(di->dev, "USB Type - ACA RiD_B configuration\n");
		break;
	case USB_STAT_ACA_RID_C_NM:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P09;
		dev_dbg(di->dev, "USB Type - ACA RID_C configuration, normal mode\n");
		break;
	case USB_STAT_ACA_RID_C_HS:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P09;
		dev_dbg(di->dev, "USB Type - ACA RID_C configuration, HS mode\n");
		break;
	case USB_STAT_ACA_RID_C_HS_CHIRP:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P09;
		dev_dbg(di->dev, "USB Type - ACA RID_C configuration, HS Chirp mode\n");
		break;
	case USB_STAT_HM_IDGND:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P5;
		dev_dbg(di->dev, "USB Type - Host Mode(IDGND)\n");
		break;
	case USB_STAT_RESERVED:
		dev_dbg(di->dev, "USB Type - Reserved\n");
		break;
	case USB_STAT_NOT_VALID_LINK:
		dev_dbg(di->dev, "USB Type - USB link not valid\n");
		break;
	default:
		di->usb_ip_cur_lvl = USB_CH_IP_CUR_LVL_0P5;
		break;
	};
	return 0;
}

/**
 * ab8500_bm_cc_enable() - enable coulomb counter
 * @di:		pointer to the ab8500_bm_device_info structure
 * @flag:	flag to enable/disable
 *
 * Enable/Disable coulomb counter.
 * On failure returns -ve value.
 **/
static int ab8500_bm_cc_enable(struct ab8500_bm_device_info *di, int flag)
{
	int ret = 0;
	if (flag) {
		/* Enable CC */
		ret = ab8500_write(AB8500_RTC, AB8500_RTC_CC_CONF_REG, 0x03);
		if (ret)
			goto cc_err;
		ret = ab8500_write(AB8500_GAS_GAUGE, AB8500_GASG_CC_NCOV_ACCU,
								 0x14);
		if (ret)
			goto cc_err;
	} else {
		ret = ab8500_write(AB8500_RTC, AB8500_RTC_CC_CONF_REG, 0x01);
		if (ret)
			goto cc_err;
	}
	dev_dbg(di->dev, "coulomb counter enabled\n");
	return ret;
cc_err:
	dev_dbg(di->dev, "Enabling coulomb counter failed\n");
	return ret;

}

/**
 * ab8500_bm_inst_current() - battery instantenous current
 * @di:		pointer to the ab8500_bm_device_info structure
 *
 * Returns the battery instantenous current else -ve on failure.
 * This is obtained from the coulomb bounter.
 **/
static int ab8500_bm_inst_current(struct ab8500_bm_device_info *di)
{
	int ret = 0;
	s8 low, high;
	static int val;

	mutex_lock(&ab8500_cc_lock);
	/* Reset counter and Read request */
	ret = ab8500_write(AB8500_GAS_GAUGE, AB8500_GASG_CC_CTRL_REG, 0x03);
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_inst_current:: write failed\n");
		mutex_unlock(&ab8500_cc_lock);
		return ret;
	}

	/* Since there is no interrupt for this, just wait for 250ms
	 * 250ms is one sample convertion time with 32.768 Khz RTC clock
	 */
	msleep (250);
	/* Read CC Sample conversion value Low and high */
	low = ab8500_read(AB8500_GAS_GAUGE, AB8500_GASG_CC_SMPL_CNVL_REG);
	high = ab8500_read(AB8500_GAS_GAUGE, AB8500_GASG_CC_SMPL_CNVH_REG);

	dev_vdbg(di->dev, "instantaneous LSB %0x\n", low);
	dev_vdbg(di->dev, "instantaneous MSB %0x\n", high);

	/*  negative value for Discharging
	 *  convert 2's compliment into decimal */
	if (high & 0x10) {
		val = (low | high << 8);
		val ^= 0x1;
		val = ~val;
		/* Extract 12 bits */
		val &= 0xfff;
	} else
		val = (low | (high << 8));

	/* Convert to unit value in mA
	 * Full scale input voltage is
	 * 66.660mV => LSB = 66.660mV/(4096*10) = 1.627mA
	 * resister value is 10 ohms
	 */

	val = ((val * 6666)  / 4096);

	dev_vdbg(di->dev, "inst current = %d\n", val);

	mutex_unlock(&ab8500_cc_lock);
	return val;
}

/**
 * ab8500_bm_avg_cur_work() - average battery current
 * @work:	pointer to the work_struct structure
 *
 * Updated the average battery current obtained from the
 * coulomb counter.
 **/
static void ab8500_bm_avg_cur_work(struct work_struct *work)
{

	int val, ret, num_samples;
	s8 low, med, high;
	static int old_average;

	struct ab8500_bm_device_info *di = container_of(work,
					struct
					ab8500_bm_device_info,
					ab8500_bm_avg_cur_monitor_work);

	mutex_lock(&ab8500_cc_lock);
	ret = ab8500_write(AB8500_GAS_GAUGE, AB8500_GASG_CC_NCOV_ACCU_CTRL, 0x01);
	if (ret) {
		mutex_unlock(&ab8500_cc_lock);
		goto exit;
	}

	low = ab8500_read(AB8500_GAS_GAUGE, AB8500_GASG_CC_NCOV_ACCU_LOW);
	med = ab8500_read(AB8500_GAS_GAUGE, AB8500_GASG_CC_NCOV_ACCU_MED);
	high = ab8500_read(AB8500_GAS_GAUGE, AB8500_GASG_CC_NCOV_ACCU_HIGH);

	/* Check for sign bit in case of negative value, 2's compliment */
	if (high & 0x10) {
		val = (low | (med << 8) | (high << 16));
		val ^= 0x1;
		val = ~val;
		/* Extract 20 bits */
		val &= 0xfffff;
	} else {
		val = (low | (med << 8) | (high << 16));
	}

	/* Get the number of samples from sysfs */
	ab8500_bm_sys_get_gg_samples(&num_samples);
	/* Average Current in mA = (intgrated value * 1627uA) /Number Of samples * 1000)
	 */

	val = ((val * 1627) / (num_samples * 1000));

	/* Filter out zero averge current, single zero may be because of read error */
	if (val == 0) {
		val = old_average;
		old_average = 0;
	} else {
		old_average = val;
	}

	mutex_unlock(&ab8500_cc_lock);

	di->avg_current = val;

exit:
	dev_vdbg(di->dev, "Failed to write Read request to gas gauge controller\n");
	/* If  i2c write fails,
	 *  update it with previous average value  */
	di->avg_current = old_average;
}

/**
 * ab8500_bm_ac_en_work() - work to enable ac charging
 * @work:	pointer to the work_struct structure
 *
 * Work queue function for enabling AC charging
 **/
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

/**
 * ab8500_bm_ac_dis_work() - work to disable ac charging
 * @work:	pointer to the work_struct structure
 *
 * Work queue function for disabling AC charging
 **/

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

/**
 * ab8500_bm_usb_en_work() - work to enable usb charging
 * @work:	pointer to the work_struct structure
 *
 * Work queue function for enabling USB charging
 **/
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
		ret = ab8500_bm_usb_en(di, true);
		if (ret < 0)
			dev_vdbg(di->dev, "failed to enable USB charging\n");
	}
}

/**
 * ab8500_bm_usb_dis_work() - work to disable usb charging
 * @work:	pointer to the work_struct structure
 *
 * Work queue function for disabling USB charging
 **/
static void ab8500_bm_usb_dis_work(struct work_struct *work)
{
	struct ab8500_bm_device_info *di = container_of(work,
							struct
							ab8500_bm_device_info,
							ab8500_bm_usb_dis_monitor_work);
	int ret = 0;
	ret = ab8500_bm_usb_en(di, false);
	if (ret < 0)
		dev_vdbg(di->dev, "failed to disable USB charging\n");
	ab8500_bm_battery_update_status(di);
}

/**
 * ab8500_bm_cc_convend_handler() - work to get battery avg current.
 * @work:	pointer to the work_struct structure
 *
 * Work queue function for disabling USB charging
 **/
static void ab8500_bm_cc_convend_handler (void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	queue_work(di->ab8500_bm_wq, &di->ab8500_bm_avg_cur_monitor_work);
}

/**
 * ab8500_bm_mainextchnotok_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - main charger not OK
 **/
static void ab8500_bm_mainextchnotok_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	/* on occurance of this int charging not done */
	dev_info(di->dev, "Main charger NOT OK! Charging cannot proceed\n");
}

/**
 * ab8500_bm_battovv_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - battery overvoltage detected
 **/
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

/**
 * ab8500_bm_mainchunplugdet_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - main charge unplug detected
 **/
static void ab8500_bm_mainchunplugdet_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	power_supply_changed(&di->bat);
	dev_dbg(di->dev, "main charger unplug detected....!\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_ac_dis_monitor_work);
}

/**
 * ab8500_bm_mainchplugdet_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - main charge plug detected
 **/
static void ab8500_bm_mainchplugdet_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	power_supply_changed(&di->bat);
	dev_dbg(di->dev, "main charger plug detected....!\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_ac_en_monitor_work);
}

/**
 * ab8500_bm_vbusdetr_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - rising edge on vbus detected
 **/
static void ab8500_bm_vbusdetr_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	di->usb_plug_unplug = true;
	power_supply_changed(&di->bat);
	dev_dbg(di->dev, "vbus rising edge detected....!\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_usb_en_monitor_work);
}

/**
 * ab8500_bm_vbusdetf_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - falling edge on vbus detected
 **/
static void ab8500_bm_vbusdetf_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	di->usb_plug_unplug = false;
	power_supply_changed(&di->bat);
	dev_dbg(di->dev, "vbus falling edge detected....!\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_usb_dis_monitor_work);
}

/**
 * ab8500_bm_batctrlindb_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - battery removal detected
 **/
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

/**
 * ab8500_bm_chwdexp_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - watchdog charger expiration detected
 **/
static void ab8500_bm_chwdexp_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	dev_info
	    (di->dev,
	     "Watchdog charger expiration detected, charging stopped...!\n");
}

/**
 * ab8500_bm_vbusovv_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - vbus overvoltage detected
 **/
static void ab8500_bm_vbusovv_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;

	dev_info
	    (di->dev, "VBUS overvoltage detected, USB charging stopped...!\n");
	/* For protection sw is also disabling charging */
	dev_dbg(di->dev, "Disabling USB charging\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_usb_dis_monitor_work);
}

/**
 * ab8500_bm_lowbatf_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers - bat voltage goes below LowBat detected
 **/
static void ab8500_bm_lowbatf_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	dev_dbg(di->dev, "Battery voltage goes below LowBat voltage\n");
}

/**
 * ab8500_bm_lowbatr_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers - bat voltage goes above LowBat detected
 **/
static void ab8500_bm_lowbatr_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	dev_dbg(di->dev, "Battery voltage goes above LowBat voltage\n");
}

/**
 * ab8500_bm_btemplow_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - battery temp lower than 10c
 **/
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

/**
 * ab8500_bm_btemphigh_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - battery temp higher than max temp
 **/
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

/**
 * ab8500_bm_usbchgdetdone_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - usb charger detected
 **/
static void ab8500_bm_usbchgdetdone_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	power_supply_changed(&di->bat);
	dev_dbg(di->dev, "usb charger detected...!\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_usb_en_monitor_work);
}

/**
 * ab8500_bm_usbchthprotr_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - Die temp is above usb charger
 * thermal protection threshold
 **/
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

/**
 * ab8500_bm_mainchthprotr_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - Die temp is above main charger
 * thermal protection threshold
 **/
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

/**
 * ab8500_bm_usbchargernotokf_handler()
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * callback handlers  - usb charger unplug detected
 **/
static void ab8500_bm_usbchargernotokf_handler(void *_di)
{
	struct ab8500_bm_device_info *di = _di;
	power_supply_changed(&di->bat);
	dev_dbg(di->dev, "usb charger unplug detected...!\n");
	queue_work(di->ab8500_bm_irq, &di->ab8500_bm_usb_dis_monitor_work);
}

/**
 * ab8500_bm_register_handler() - register/unregister callback handler
 * @set:	flag to register/unregister the callback handler
 * @_di:	void pointer that has to address of ab8500_bm_device_info
 *
 * Registers/Unregisters irq_no and callback handler functions
 * with the AB8500 core driver
 **/
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
		ret = ab8500_set_callback_handler(MAIN_CHARGE_PLUG_DET,
						  ab8500_bm_mainchplugdet_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-main charge plug detected\n");
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
		/* set callback handlers  - vbus overvoltage detected */
		ret = ab8500_set_callback_handler(VBUS_OVV,
						  ab8500_bm_vbusovv_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-vbus overvoltage detected\n");
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
		/* set callback handlers  - usb charger detected */
		ret = ab8500_set_callback_handler(USB_CHARGE_DET_DONE,
						  ab8500_bm_usbchgdetdone_handler,
						  di);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-usb charger detected\n");
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

		/* set callback handlers  - usb charger unplug detected */
		ret = ab8500_set_callback_handler(USB_CHARGER_NOT_OKF,
						  ab8500_bm_usbchargernotokf_handler,
						  di);

		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to set callback handler-usb charger unplug detected\n");

		/* set callback handlers  - coulomb counter conversion end */
		ret = ab8500_set_callback_handler (NCONV_ACCU, ab8500_bm_cc_convend_handler, di);
		if (ret < 0)
			 dev_vdbg(di->dev, "failed to set coulomb counter conversion end handler\n");
	}
	/* Unregister irq_no and callback handler */
	else {
		/* remove callback handlers  - main charger not OK */
		ret = ab8500_remove_callback_handler(MAIN_EXT_CH_NOT_OK,
				ab8500_bm_mainextchnotok_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-main charger not ok\n");
		/* remove callback handlers  - battery overvoltage detected */
		ret = ab8500_remove_callback_handler(BATT_OVV,
				ab8500_bm_battovv_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-battery over voltage detected\n");
		/* remove callback handlers  - main charge unplug detected */
		ret = ab8500_remove_callback_handler(MAIN_CH_UNPLUG_DET,
				ab8500_bm_mainchunplugdet_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-main charge unplug detected\n");
		/* remove callback handlers  - main charge plug detected */
		ret = ab8500_remove_callback_handler(MAIN_CHARGE_PLUG_DET,
				ab8500_bm_mainchplugdet_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-main charge plug detected\n");

		/* remove callback handlers  - rising edge on vbus detected */
		ret = ab8500_remove_callback_handler(VBUS_DET_R,
				ab8500_bm_vbusdetr_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-rising edge on vbus detected\n");
		/* remove callback handlers  - falling edge on vbus detected */
		ret = ab8500_remove_callback_handler(VBUS_DET_F,
				ab8500_bm_vbusdetf_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-falling edge on vbus detected\n");

		/* remove callback handlers  - battery removal detected */
		ret = ab8500_remove_callback_handler(BAT_CTRL_INDB,
				ab8500_bm_batctrlindb_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-battery removal detected\n");
		/* remove callback handlers  - watchdog charger expiration
		 * detected
		 */
		ret = ab8500_remove_callback_handler(CH_WD_EXP,
				ab8500_bm_chwdexp_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-watchdog expiration detected\n");

		/* remove callback handlers  - vbus overvoltage detected */
		ret = ab8500_remove_callback_handler(VBUS_OVV,
				ab8500_bm_vbusovv_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-vbus overvoltage detected\n");
		/* remove callback handlers  - bat voltage goes below LowBat
		 * detected
		 */
		ret = ab8500_remove_callback_handler(LOW_BAT_F,
				ab8500_bm_lowbatf_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-bat voltage goes below LowBat detected\n");
		/* remove callback handlers  - bat voltage goes above LowBat
		 * detected
		 */
		ret = ab8500_remove_callback_handler(LOW_BAT_R,
				ab8500_bm_lowbatr_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-bat voltage goes above LowBat detected\n");
		/* remove callback handlers  - battery temp lower than 10c */
		ret = ab8500_remove_callback_handler(BTEMP_LOW,
				ab8500_bm_btemplow_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-battery temp below -10 deg\n");
		/* remove callback handlers  - battery temp higher than
		 * max temp
		 */
		ret = ab8500_remove_callback_handler(BTEMP_HIGH,
				ab8500_bm_btemphigh_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-battery temp greater than max temp\n");

		/* remove callback handlers  - usb charger detected */
		ret = ab8500_remove_callback_handler(USB_CHARGE_DET_DONE,
				ab8500_bm_usbchgdetdone_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-usb charger detected\n");

		/* remove callback handlers  - Die temp is above usb charger
		 * thermal protection threshold
		 */
		ret = ab8500_remove_callback_handler(USB_CH_TH_PROT_R,
				ab8500_bm_usbchthprotr_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-die temp above usb charger thermal protection threshold\n");
		/* remove callback handlers  - Die temp is above main charger
		 * thermal protection threshold
		 */
		ret = ab8500_remove_callback_handler(MAIN_CH_TH_PROT_R,
				ab8500_bm_mainchthprotr_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				 "failed to remove callback handler-dir temp above main charger thermal protection threshold\n");

		/* remove callback handlers  - usb charger unplug detected */
		ret = ab8500_remove_callback_handler(USB_CHARGER_NOT_OKF,
				ab8500_bm_usbchargernotokf_handler);
		if (ret < 0)
			dev_vdbg(di->dev,
				"failed to remove callback handler-usb charger unplug detected\n");

		/* remove callback handlers  - coulomb counter conversion end */
		ret = ab8500_remove_callback_handler(NCONV_ACCU, ab8500_bm_cc_convend_handler);
		if (ret < 0)
			dev_vdbg(di->dev, "failed to remove coulomb counter conversion handler\n");
	}

	return ret;
}

/**
 * ab8500_bm_ac_voltage() - get ac charger voltage
 * @di:		pointer to the ab8500_bm_device_info structure
 *
 * This function returns the ac charger voltage.
 **/
static int ab8500_bm_ac_voltage(struct ab8500_bm_device_info *di)
{
	int data;

	data = ab8500_gpadc_conversion(MAIN_CHARGER_V);
	if (!data) {
		dev_err(di->dev, "gpadc conversion failed\n");
		return data;
	}

	/* conversion from RAW val to voltage
	 * 20250 * X / 1023
	 */
	data = (20250 * data) / 1023;
	return data;
}

/**
 * ab8500_bm_usb_voltage() - get vbus voltage
 * @di:		pointer to the ab8500_bm_device_info structure
 *
 * This fucntion returns the vbus voltage.
 **/
static int ab8500_bm_usb_voltage(struct ab8500_bm_device_info *di)
{
	int data;

	data = ab8500_gpadc_conversion(VBUS_V);
	if (!data) {
		dev_err(di->dev, "gpadc conversion failed\n");
		return data;
	}

	/* conversion from RAW data to voltage
	 * 20250 * X / 1023
	 */
	data = (20250 * data) / 1023;
	return data;
}

/**
 * ab8500_bm_voltage() - get battery voltage
 * @di:		pointer to the ab8500_bm_device_info structure
 *
 * This fucntion returns the battery voltage obtained from the
 * GPADC.
 **/
static int ab8500_bm_voltage(struct ab8500_bm_device_info *di)
{
	int data;

	data = ab8500_gpadc_conversion(MAIN_BAT_V);
	if (!data) {
		dev_err(di->dev, "gpadc conversion failed\n");
		return data;
	}

	/* conversion from RAW value to voltage
	 * 2300 + 2500*X / 1023
	 */
	data = 2300 + ((2500 * data) / 1023);
	return data;
}

/**
 * ab8500_bm_temperature() - get battery temperature
 * @di:		pointer to the ab8500_bm_device_info structure
 *
 * This fucntion returns the battery temperature obtained from the
 * GPADC.
 **/
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
	if (!data) {
		dev_err(di->dev, "gpadc conversion failed\n");
		return data;
	}

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

/**
 * ab8500_bm_battery_read_status() - get the battery parameters
 * @di:		pointer to the ab8500_bm_device_info structure
 *
 * This fucntion is being called from ab8500_bm_battery_update_status
 * to get the battery voltage, current and temperature. Ahead to this
 * it also checks for the condition to terminate charging if chargign
 * is in progress.
 **/
static void ab8500_bm_battery_read_status(struct ab8500_bm_device_info *di)
{
	int val, val_usb, status, ret = 0;

	di->temp_C = ab8500_bm_temperature(di);
	di->voltage_uV = ab8500_bm_voltage(di);

	ab8500_bm_sys_get_gg_capacity(&di->capacity);
	di->inst_current = ab8500_bm_inst_current(di);

	/* Li-ion batteries gets charged upto 4.2v, hence 4.2v should be one of
	 * the criteria for termination charging. Another factor to terminate the
	 * the charging is the CV method, on occurance of Constant Voltage charging
	 * the termination current has to be checked. For this the integrated
	 * current has to be in the range of 50-200mA
	 */
	if (di->charge_status == POWER_SUPPLY_STATUS_CHARGING) {
		if (di->voltage_uV > (di->pdata->termination_vol - 25)
				&& di->voltage_uV < di->pdata->termination_vol
				&& !di->maintenance_chg) {
			/* Check if charging is in constant voltage */
			val =
			    ab8500_read(AB8500_CHARGER, AB8500_CH_STATUS1_REG);
			val_usb =
			    ab8500_read(AB8500_CHARGER, AB8500_CH_USBCH_STAT1_REG);
			dev_dbg(di->dev, "termination voltage reached\n");
			if (((val | MAIN_CH_CV_ON) == val) || ((val_usb | USB_CH_CV_ON) == val_usb)) {
				/* Check for termination current */
				dev_dbg(di->dev, "in constant voltage\n");
				if (di->inst_current > MIN_TERMINATION_CUR
				    && di->inst_current < MAX_TERMINATION_CUR) {
					/* TODO: Revisit need to use
					 * ab8500_bm_status()
					 */
					dev_dbg(di->dev, "current reduced and in range >5<200mA\n");
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
					di->maintenance_chg = true;
					dev_dbg(di->dev, "Battery full charging stopped\n");
				}
				if (ret < 0)
					dev_dbg(di->dev,
						"Battery reached threshold, failed to stop charging\n");
			}
		}
	}
}

/**
 * ab8500_bm_battery_update_status() - update battery status
 * @di:		pointer to the ab8500_bm_device_info structure
 *
 * This function is being called by the ab8500_bm_battery_work to update the
 * battery status i.e charging/discharging/full/unknown.
 * It also takes care of maintenance charging.
 **/
static void ab8500_bm_battery_update_status(struct ab8500_bm_device_info *di)
{
	int val = 0, status = 0, ret;

	di->charge_status = POWER_SUPPLY_STATUS_UNKNOWN;
	/* Battery voltage > BattOVV or charging stopped by Btemp indicator */
	if (power_supply_am_i_supplied(&di->bat)) {
		val = ab8500_read(AB8500_CHARGER, AB8500_CH_STAT_REG);
		if ((val | CHARGING_STOP_BY_BTEMP) == val)
			di->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else
			di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
	} else
		di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;
	ab8500_bm_battery_read_status(di);

	/* Maintenance charging: On charging the battery fully the charging
	 * is terminated. If the presence of AC/USB is still present check
	 * for the battery voltage until it drops to 4.1v and if so enable
	 * charging here. Termination of charging is taken care by the
	 * function ab8500_bm_battery_read_status()
	 */
	status = ab8500_bm_charger_presence(di);
	if ((di->voltage_uV < (di->pdata->termination_vol - 200)) && status &&
			di->maintenance_chg) {
		dev_dbg(di->dev, "welcome to maintenance charging\n");
		if ((status == (AC_PW_CONN + USB_PW_CONN))
				|| (status == AC_PW_CONN))
			ret = ab8500_bm_ac_en(di, true);
		else
			ret = ab8500_bm_usb_en(di, true);
		if (ret < 0)
			dev_dbg(di->dev, "Maintenance charging: Enabling charging failed\n");
		di->maintenance_chg = false;
	}
}

/**
 * ab8500_bm_battery_work() - battery monitoring work
 * @work:	pointer to the work_struct structure
 *
 * This function is called on probe of the driver and thereafter at
 * cnnstant interval to time to keep monitoring the battery.
 **/
static void ab8500_bm_battery_work(struct work_struct *work)
{
	struct ab8500_bm_device_info *di = container_of(work,
							struct
							ab8500_bm_device_info,
							ab8500_bm_monitor_work.work);

	ab8500_bm_battery_update_status(di);
	queue_delayed_work(di->ab8500_bm_wq, &di->ab8500_bm_monitor_work, 100);
}

/**
 * ab8500_bm_bk_battery_voltage() - backup battery voltage
 * @di:		pointer to the ab8500_bm_device_info structure
 *
 * This fucntion is called from the ab8500_bm_bk_battery_get_property function
 * to get the backup battery voltage. It uses GPADC to get the voltage
 * converted from analog to digital.
 **/
static int ab8500_bm_bk_battery_voltage(struct ab8500_bm_device_info *di)
{
	int data;

	data = ab8500_gpadc_conversion(BK_BAT_V);
	if (!data) {
		dev_err(di->dev, "gpadc conversion failed\n");
		return data;
	}

	/* conversion from RAW value to voltage
	 * 2300 + 2500*X / 1023
	 */
	data = 2300 + ((2500 * data) / 1023);
	return data;
}

/**
 * ab8500_bm_watchdog_kick_work() - Re-kick the watchdog
 * @work:	pointer to the work_struct struccture
 *
 * This function is called on enabling the ac/usb charging to
 * re-kick the watchdog. Thereafter as delayed work in the workqueue
 * at contant intervals to keep re-kicking the watchdog until charging
 * is stopped.
 **/
static void ab8500_bm_watchdog_kick_work(struct work_struct *work)
{
	struct ab8500_bm_device_info *di = container_of(work,
							struct
							ab8500_bm_device_info,
							ab8500_bm_watchdog_work.work);
	/* Kickoff main watchdog */
	ab8500_write(AB8500_CHARGER, AB8500_CHARG_WD_CTRL, CHARG_WD_KICK);
	queue_delayed_work(di->ab8500_bm_wd_kick_wq,
			   &di->ab8500_bm_watchdog_work, 300);
}

/**
 * ab8500_bm_led_en() - turn on/off chargign led
 * @di:		pointer to the ab8500_bm_device_info structure
 * @on:		flag to turn on/off the chargign led
 *
 * Return 0 on success and -1 on error
 *
 * Power ON/OFF charging LED indication
 **/
static int ab8500_bm_led_en(struct ab8500_bm_device_info *di, int on)
{
	int ret;

	if (on) {
		/* Power ON charging LED indicator, set LED current to 5mA */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_LED_INDICATOR_PWM_CTRL,
				 (LED_IND_CUR_5MA | LED_INDICATOR_PWM_ENA));
		if (ret) {
			dev_vdbg(di->dev,
				 "ab8500_bm_led_en: power ON LED failed\n");
			return ret;
		}
		/* LED indicator PWM duty cycle 252/256 */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_LED_INDICATOR_PWM_DUTY,
				 LED_INDICATOR_PWM_DUTY_252_256);
		if (ret) {
			dev_vdbg(di->dev,
				 "ab8500_bm_led_en: set LED PWM duty cycle failed\n");
			return ret;
		}
	} else {
		/* Power off charging LED indicator */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_LED_INDICATOR_PWM_CTRL,
				 LED_INDICATOR_PWM_DIS);
		if (ret) {
			dev_vdbg(di->dev,
				 "ab8500_bm_led_en: power-off LED failed\n");
			return ret;
		}
	}

	return ret;
}

/**
 * ab8500_bm_charger_presence() - check for the presence of charger
 * @di:		pointer to the ab8500 bm device information structure
 *
 * Returns an integer value, that means,
 * NO_PW_CONN  no power supply is connected
 * AC_PW_CONN  if the AC power supply is connected
 * USB_PW_CONN  if the USB power supply is connected
 * AC_PW_CONN + USB_PW_CONN if USB and AC power supplies are both connected
 *
 **/
static int ab8500_bm_charger_presence(struct ab8500_bm_device_info *di)
{
	int ret = NO_PW_CONN, val_ac = 0, val_usb = 0;
	mutex_lock(&ab8500_bm_lock);
	/* ITSource2 bit3 MainChPlugDet */
	val_ac = ab8500_read(AB8500_INTERRUPT, AB8500_IT_SOURCE2_REG);
	/* ITSource21 bit 6 UsbChgDetDone */
	val_usb = ab8500_read(AB8500_INTERRUPT, AB8500_IT_SOURCE21_REG);

	if ((val_ac | MAIN_CH_PLUG_DET) == val_ac)
		ret = AC_PW_CONN;
	if (((val_usb | USB_CHG_DET_DONE) == val_usb) || ((val_ac | 0x80) == val_ac)) {
		if (ret == AC_PW_CONN)
			ret = AC_PW_CONN + USB_PW_CONN;
		else
			ret = USB_PW_CONN;
	}
	mutex_unlock(&ab8500_bm_lock);
	dev_dbg(di->dev, "Return value of charger presence = %d\n", ret);
	return ret;
}

/**
 * ab8500_bm_ac_en() - enable ac charging
 * @di:		pointer to the ab8500_bm_device_info structure
 * @enable:	enable/disable flag
 *
 * Returns 0 on success and -1 on error
 *
 * Enable/Disable AC/Mains charging and turns on/off the charging led
 * respectively.
 **/
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
		/* BattOVV threshold can take values 3.7 or 4.75
		 * if set to 3.7 charging can be done upto 3.5v
		 * whereas Li-Ion charges of capacity 3.7v charges
		 * upto 4.2v hence
		 * BattOVV threshold = 4.75v
		 */

		ret = ab8500_write(AB8500_CHARGER, AB8500_BATT_OVV,
				(BATT_OVV_ENA | BATT_OVV_TH_4P75));
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}

		/* Enable AC charging */

		/* ChVoltLevel */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_CH_VOLT_LVL_REG,
				   di->pdata->ip_vol_lvl);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		/* MainChInputCurr = 1.1A */
		ret = ab8500_write(AB8500_CHARGER, AB8500_MCH_IPT_CURLVL_REG,
				   MAIN_CH_IP_CUR_1P1A);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		/* ChOutputCurentLevel */
		ret = ab8500_write(AB8500_CHARGER, AB8500_CH_OPT_CRNTLVL_REG,
			       di->pdata->op_cur_lvl);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		/* Enable Main Charger */
		ret = ab8500_write(AB8500_CHARGER, AB8500_MCH_CTRL1, MAIN_CH_ENA);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		/* Enable main watchdog */
		ret = ab8500_write(AB8500_SYS_CTRL2_BLOCK,
				   AB8500_MAIN_WDOG_CTRL_REG, MAIN_WDOG_ENA);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		ret = ab8500_write(AB8500_SYS_CTRL2_BLOCK,
				   AB8500_MAIN_WDOG_CTRL_REG,
				   (MAIN_WDOG_ENA | MAIN_WDOG_KICK));
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		/* Disable main watchdog */
		ret = ab8500_write(AB8500_SYS_CTRL2_BLOCK,
				   AB8500_MAIN_WDOG_CTRL_REG, MAIN_WDOG_DIS);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_ac_en(): write failed\n");
			return ret;
		}
		/* Kickoff main watchdog */
		ret = ab8500_write(AB8500_CHARGER, AB8500_CHARG_WD_CTRL, CHARG_WD_KICK);
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
		dev_dbg(di->dev, "Enabled AC charging\n");
		/* Schedule delayed work to re-kick watchdog */
		queue_delayed_work(di->ab8500_bm_wd_kick_wq,
				   &di->ab8500_bm_watchdog_work, 300);
	} else {
		/* Disable AC charging */
		ret = ab8500_write(AB8500_CHARGER, AB8500_MCH_CTRL1, MAIN_CH_DIS);
		/* If success power off charging LED indication */
		if (ret == 0) {
			/* Check if USB charger is presenc if so then dont
			 * turn off the LED, USB charging is taking place
			 */
			ret = ab8500_bm_charger_presence(di);
			if ((ret != USB_PW_CONN) &&
					(ret != (AC_PW_CONN + USB_PW_CONN))) {
				/* Schedule delayed work to re-kick watchdog */
				cancel_delayed_work
					(&di->ab8500_bm_watchdog_work);
				ret = ab8500_bm_led_en(di, false);
				if (ret < 0)
					dev_vdbg(di->dev,
						 "failed to disable LED\n");
			}
		}
		dev_dbg(di->dev, "Disabled AC charging\n");
	}
	return ret;
}

/**
 * ab8500_bm_usb_en() - enable usb charging
 * @di:		pointer to the ab8500_bm_device_info structure
 * @enable:	enable/disable flag
 *
 * Returns 0 on success and -1 on error
 *
 * Enable/Disable USB charging and turns on/off the charging led respectively.
 **/
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
			dev_err(di->dev, "USB charger not connected\n");
			return -ENXIO;
		}
		ret = ab8500_bm_detect_usb_type(di);
		if (ret < 0) {
			dev_err(di->dev, "Unable to detect usb type\n");
			return ret;
		}
		/* BattOVV threshold = 4.75v */
		ret = ab8500_write(AB8500_CHARGER, AB8500_BATT_OVV, 0x03);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}

		/* Enable USB charging */

		/* ChVoltLevel */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_CH_VOLT_LVL_REG,
				   di->pdata->ip_vol_lvl);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		/* USBChInputCurr */
		ret = ab8500_write(AB8500_CHARGER, AB8500_USBCH_IPT_CRNTLVL_REG,
				   di->usb_ip_cur_lvl);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		/* ChOutputCurentLevel */
		ret = ab8500_write(AB8500_CHARGER, AB8500_CH_OPT_CRNTLVL_REG,
				   di->pdata->op_cur_lvl);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		/* Enable USB Charger */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_USBCH_CTRL1_REG, USB_CH_ENA);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		/* Enable main watchdog */
		ret = ab8500_write(AB8500_SYS_CTRL2_BLOCK,
				   AB8500_MAIN_WDOG_CTRL_REG, MAIN_WDOG_ENA);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		ret = ab8500_write(AB8500_SYS_CTRL2_BLOCK,
				   AB8500_MAIN_WDOG_CTRL_REG, (MAIN_WDOG_ENA | MAIN_WDOG_KICK));
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		/* Disable main watchdog */
		ret = ab8500_write(AB8500_SYS_CTRL2_BLOCK,
				   AB8500_MAIN_WDOG_CTRL_REG, MAIN_WDOG_DIS);
		if (ret) {
			dev_vdbg(di->dev, "ab8500_bm_usb_en(): write failed\n");
			return ret;
		}
		/* Kickoff main watchdog */
		ret = ab8500_write(AB8500_CHARGER, AB8500_CHARG_WD_CTRL, CHARG_WD_KICK);
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
		dev_dbg(di->dev, "Enabled USB charging\n");
		/* Schedule delayed work to re-kick watchdog */
		queue_delayed_work(di->ab8500_bm_wd_kick_wq,
				   &di->ab8500_bm_watchdog_work, 300);
	} else {
		/* Disable USB charging */
		ret =
		    ab8500_write(AB8500_CHARGER, AB8500_USBCH_CTRL1_REG, USB_CH_DIS);
		/* If sucussful power off charging LED indication */
		if (ret == 0) {
			/* Check if AC charger is present if so then dont
			 * turn off the LED, AC charging is taking place
			 */
			ret = ab8500_bm_charger_presence(di);
			if ((ret != AC_PW_CONN) &&
					(ret != (AC_PW_CONN + USB_PW_CONN))) {
				/* Schedule delayed work to re-kick watchdog */
				cancel_delayed_work
					(&di->ab8500_bm_watchdog_work);
				ret = ab8500_bm_led_en(di, false);
				if (ret < 0)
					dev_vdbg(di->dev,
						 "failed to disable LED\n");
			}
		}
		dev_dbg(di->dev, "Disabled USB charging\n");
	}
	return ret;
}

/**
 * ab8500_bm_hw_presence_en() - enable battery interrupts
 * @di:		pointer to the ab8500_bm_device_info structure
 * @enable:	flag to enable/disable interrupts
 *
 * This fucntion enables/disable the various battery interrupts.
 * Also OTP values are set.
 **/
static int ab8500_bm_hw_presence_en(struct ab8500_bm_device_info *di,
				    int enable)
{
	int val = 0, ret = 0;

	/* Set BATOVV to 4.75v */
	ret = ab8500_write(AB8500_CHARGER, AB8500_BATT_OVV, BATT_OVV_TH_4P75);

	/* Low Battery Voltage = 3.1v */
	val = ab8500_read(AB8500_SYS_CTRL2_BLOCK, AB8500_LOW_BAT_REG);
	ret = ab8500_write(AB8500_SYS_CTRL2_BLOCK, AB8500_LOW_BAT_REG,
			(enable ? (val | LOW_BAT_3P1V) : (val & LOW_BAT_RESET)));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}

	/* OTP: Enable Watchdog */
	val = ab8500_read(AB8500_OTP_EMUL, 0x150E);
	ret = ab8500_write(AB8500_OTP_EMUL, 0x150E,
			   (enable ? (val | OTP_ENABLE_WD) : (val & OTP_DISABLE_WD)));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}
	ret = ab8500_write(AB8500_OTP_EMUL, 0x1504, (enable ? (0xC0) : (0x00)));

	/* Not OK main charger detection */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK1_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK1_REG,
			   (enable ? (val & UNMASK_MAIN_EXT_CH_NOT_OK)
			    : (val | MASK_MAIN_EXT_CH_NOT_OK)));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}

	/* Main charge plug/unplug and battery OVV detection
	 * VBUS rising/falling edge detect
	 */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK2_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK2_REG,
			   (enable ? (val & (UNMASK_VBUS_DET_R & UNMASK_VBUS_DET_F &
			    UNMASK_MAIN_CH_PLUG_DET & UNMASK_MAIN_CH_UNPLUG_DET
			    & UNMASK_BATT_OVV)) : (val | (MASK_VBUS_DET_R
			    | MASK_VBUS_DET_F | MASK_MAIN_CH_PLUG_DET
			    | MASK_MAIN_CH_UNPLUG_DET | MASK_BATT_OVV))));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}

	/* Battery removal, Watchdog expiration, vbus overvoltage */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK3_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK3_REG,
			(enable ? (val & (UNMASK_VBUS_OVV & UNMASK_CH_WD_EXP
				& UNMASK_BAT_CTRL_INDB)) : (val
				| (MASK_VBUS_OVV | MASK_CH_WD_EXP
				| MASK_BAT_CTRL_INDB))));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}

	/* Battert temp below/above low/high threshold */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK19_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK19_REG,
			(enable ? (val & (UNMASK_BTEMP_HIGH &
			UNMASK_BTEMP_LOW)) : (val | (MASK_BTEMP_HIGH
			| MASK_BTEMP_LOW))));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}

	/* Not OK USB detected */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK20_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK20_REG,
			(enable ? (val & UNMASK_USB_CHARGER_NOT_OK_R)
				: (val | MASK_USB_CHARGER_NOT_OK_R)));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}

	/* USB charger detection */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK21_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK21_REG,
			(enable ? (val & UNMASK_USB_CHG_DET_DONE)
				: (val | MASK_USB_CHG_DET_DONE)));
	if (ret) {
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
		return ret;
	}

	/* Not OK USB charger fallinf edge detect */
	val = ab8500_read(AB8500_INTERRUPT, AB8500_IT_MASK22_REG);
	ret = ab8500_write(AB8500_INTERRUPT, AB8500_IT_MASK22_REG,
			(enable ? (val & UNMASK_USB_CHARGER_NOT_OK_F)
				: (val | MASK_USB_CHARGER_NOT_OK_F)));
	if (ret)
		dev_vdbg(di->dev, "ab8500_bm_hw_presence_en(): write failed\n");
	return ret;
}

/**
 * ab8500_bm_status() - battery status
 *
 * Returns the type of charge AC/USB
 * This is called by the battery property "POWER_SUPPLY_PROP_ONLINE"
 **/
static int ab8500_bm_status(void)
{
	int val = 0, ret = 0;

	/* Check for AC charging */
	val = ab8500_read(AB8500_CHARGER, AB8500_CH_STATUS1_REG);
	if ((val | MAIN_CH_ON) == val)
		ret = AC_PW_CONN;
	/* Check for USB charging */
	/* TODO: USB Charger is on bit 3 on Reg USBChStatus1 is not
	 * working - Revisit
	 */
	val = ab8500_read(AB8500_CHARGER, AB8500_CH_USBCH_STAT1_REG);
	/* if ((val | USB_CH_ON) == val) */
	if ((val | 0x03) == val)
		ret = USB_PW_CONN;
	return ret;
}

/**
 * ab8500_bm_get_bat_resis(): get battery resistance
 * @di:		pointer to the ab8500_bm_device_info structure
 *
 * This function returns the battery resistance that is
 * read from the GPADC.
 **/
static int ab8500_bm_get_bat_resis(struct ab8500_bm_device_info *di)
{
	int data, rbs, vbs;

	data = ab8500_gpadc_conversion(BAT_CTRL);
	vbs = (1350 * data) / 1023;
	rbs = (80 * vbs) / (1800 - vbs);
	dev_dbg(di->dev, "Battery Resistance = %d\n", rbs);
	return rbs;
}

/**
 * ab8500_bm_get_battery_property() - get the battery properties
 * @psy:	pointer to the power_supply structure
 * @psp:	pointer to the power_supply_property structure
 * @val:	pointer to the power_supply_propval union
 *
 * This function is called when an application tries to get the battery
 * properties by reading the sysfs interface.
 * Battery properties include
 * status:	charging/discharging/full/unknown
 * present:	presence of battery
 * online:	who is charging the battery mains/usb
 * technology:	battery type
 * voltage_max:	maximum voltage the battery can withstand
 * voltage_now:	present battery voltage
 * current_now:	instantaneous current of the battery
 * current_avg:	average current of the battery
 * energy_now: 	battery energy
 * capacity:	capacity of the battery in percentage
 * temp:	battery temperature in deg cel
 **/
static int ab8500_bm_get_battery_property(struct power_supply *psy,
					  enum power_supply_property psp,
					  union power_supply_propval *val)
{
	struct ab8500_bm_device_info *di;
	int status = 0, cnt;

	di = to_ab8500_bm_device_info(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		ab8500_bm_battery_update_status(di);
		val->intval = di->charge_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		if (ab8500_bm_get_bat_resis(di) <= NOKIA_BL_5F)
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		else
			val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		/* TODO: Battery presence can be obtained from the
		 * battery sense of GPADC. This reading is not
		 * consistant.
		 * This should always be 1 fro android to work.
		 * Hence by default havin it to be 0x01;
		 */
		/*if (ab8500_bm_get_bat_resis(di) <= NOKIA_BL_5F)
		 *	val->intval = 0x01;
		 * else
		 *	val->intval = 0x00;
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
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		if (ab8500_bm_get_bat_resis(di) <= NOKIA_BL_5F)
			val->intval = di->pdata->name;
		else
			val->intval = POWER_SUPPLY_TECHNOLOGY_UNKNOWN;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		status = ab8500_read(AB8500_CHARGER, AB8500_BATT_OVV);
		if ((status | BATT_OVV_TH) == status)
			val->intval = AB8500_BM_BATTOVV_4P75;
		else
			val->intval = AB8500_BM_BATTOVV_3P7;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		/* Voltage is in terms of uV */
		val->intval = (di->voltage_uV * 1000);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = (di->inst_current * 1000);
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = (di->avg_current * 1000);
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		/* TODO: Need to calculate the present energy and update
		 * the same in uWh from gasguage driver
		 */
		val->intval = 10000;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		/* Get the battery capacity from the gas guage
		 * if available else use the voltage based
		 * capacity
		 */
		ab8500_bm_sys_get_gg_capacity(&di->capacity);
		if (di->capacity > 0) {
			val->intval = di->capacity;
			dev_dbg(di->dev, "capacity based on gas gauge\n");
			break;
		}
		if (di->voltage_uV > 4200) {
			val->intval = 100;
			break;
		}
		for (cnt = 0; cnt < ARRAY_SIZE(ab8500_bm_cap); cnt++) {
			if (di->voltage_uV <= ab8500_bm_cap[cnt].vol_mV) {
				val->intval = ab8500_bm_cap[cnt].capacity;
				dev_dbg(di->dev, "voltage based capacity\n");
				break;
			}
		};
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = (di->temp_C * 10);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/**
 * ab8500_bm_bk_battery_get_property()  - get the backup battery properties
 * @psy:	pointer to the power_supply structure
 * @psp:	pointer to the power_supply_property structure
 * @val:	pointer to the power_supply_propval union
 *
 * This fucntion is called when the an application tries to read the backup
 * battery properties through the sysfs interface.
 * The properties include status and voltage.
 * status:	charging/discharging/unknown/full
 * voltage:	backup battery voltage
 **/
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

/**
 * ab8500_bm_ac_get_property()  - get the ac/mains properties
 * @psy:	pointer to the power_supply structure
 * @psp:	pointer to the power_supply_property structure
 * @val:	pointer to the power_supply_propval union
 *
 * This function gets called when an application tries to get the ac/mains
 * properties by reading the sysfs files.
 * AC/Mains properties are online, present and voltage.
 * online:	ac/mains charging is in progress or not
 * present:	presence of the ac/mains
 * voltage:	AC/Mains voltage
 **/
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
			val->intval = AB8500_BM_CHARGING;
		else
			val->intval = AB8500_BM_NOT_CHARGING;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		status = ab8500_bm_status();
		if (status == AC_PW_CONN)
			val->intval = AB8500_BM_AC_PRESENT;
		else
			val->intval = AB8500_BM_AC_NOT_PRESENT;
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

/**
 * ab8500_bm_usb_get_property() - get the usb properties
 * @psy:        pointer to the power_supply structure
 * @psp:        pointer to the power_supply_property structure
 * @val:        pointer to the power_supply_propval union
 *
 * This function gets called when an application tries to get the usb
 * properties by reading the sysfs files.
 * USB properties are online, present and voltage.
 * online:	usb charging is in progress or not
 * present:	presence of the usb
 * voltage:	usb vbus voltage
 **/
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
			val->intval = AB8500_BM_CHARGING;
		else
			val->intval = AB8500_BM_NOT_CHARGING;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		status = ab8500_bm_status();
		if (status == USB_PW_CONN)
			val->intval = AB8500_BM_USB_PRESENT;
		else
			val->intval = AB8500_BM_USB_NOT_PRESENT;
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
/**
 * ab8500_bm_resume() - resume the ab8500 battery driver
 * @pdev:	pointer to the platform_device structure
 *
 * This function gets called when the system resumes after suspend.
 * It starts the battery monitor work.
 **/
static int ab8500_bm_resume(struct platform_device *pdev)
{
	struct ab8500_bm_device_info *di = platform_get_drvdata(pdev);

	queue_delayed_work(di->ab8500_bm_wq, &di->ab8500_bm_monitor_work, 0);
	return 0;
}

/**
 * ab8500_bm_susupend() - suspend the ab8500 battery driver
 * @pdev:	pointer to the platform_device structure
 *
 * This fucntion is called whe the system goes to suspend state.
 * It cancels the scheduled battery monitor work.
 **/
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

/**
 * ab8500_bm_remove() - remove the ab8500 battery device
 * @pdev:	pointer to the paltform_device structure
 *
 * This fucntion is called when the driver gets unregisterd with the
 * platform device. It frees the allocated memory, destroys the workqueue,
 * stops the work, disable ac/usb charging, disable battery interrupts,
 * un-register callback handlers and unregister the ac, usb and battery
 * with the power supply core driver.
 **/
static int __devexit ab8500_bm_remove(struct platform_device *pdev)
{
	struct ab8500_bm_device_info *di = platform_get_drvdata(pdev);
	int ret;

	/* Disable coulomb counter */
	ret = ab8500_bm_cc_enable(di, false);
	if (ret)
		dev_err(&pdev->dev, "failed to disable coulomb counter\n");

	/* Disable AC charging */
	ret = ab8500_bm_ac_en(di, false);
	if (ret)
		dev_err(&pdev->dev, "failed to disable AC charging\n");

	/* Disable USB charging */
	ret = ab8500_bm_usb_en(di, false);
	if (ret)
		dev_err(&pdev->dev, "failed to disable USB charging\n");

	/* Disable interrupts */
	ret = ab8500_bm_hw_presence_en(di, false);
	if (ret)
		dev_err(&pdev->dev,
			"failed to disable h/w presence(i.e mask int)\n");

	/* Unregister callback handler */
	ret = ab8500_bm_register_handler(false, NULL);
	if (ret)
		dev_err(&pdev->dev, "failed to unregister callback handlers\n");

	/* sysfs interface to enable/disbale charging from user space */
	ab8500_bm_sysfs_exit(di);

	/* de-initialize the sysfs for gas gauge */
	ab8500_bm_sys_deinit();

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

/**
 * ab8500_bm_external_power_changed() - external_power_changed
 * @psy:	pointer to the structre power_supply
 *
 * This function is pointing to the function pointer external_power_changed
 * of the structure power_supply.
 * This function gets executed when there is a chage in the external power
 * supply to tbe battery. It cancels the pending monitor work and starts
 * the monitor work so as to include the changed power supply parameters.
 **/
static void ab8500_bm_external_power_changed(struct power_supply *psy)
{
	struct ab8500_bm_device_info *di = to_ab8500_bm_device_info(psy);

	cancel_delayed_work(&di->ab8500_bm_monitor_work);
	queue_delayed_work(di->ab8500_bm_wq, &di->ab8500_bm_monitor_work, 0);
}

/**
 * ab8500_bm_probe() - probe of the ab8500 battery device
 * @pdev:	pointer to the platform device structure
 *
 * This function is called after the driver is registered to the platform
 * device framework. It allocates memory for the internal data structure,
 * creates single threaded workqueue, enabled battery related interrupts
 * and register callback handlers for the associated interrut with the
 * ab8500 core driver,initializes the delayed and normal work, enables
 * charging in safe mode and registers the battery, ac and usb to the
 * power supply core driver.
 **/
static int __devinit ab8500_bm_probe(struct platform_device *pdev)
{
	/* TODO: This might be required for storing the temp calibration data
	   struct ab8500_bm_platform_data *pdata = pdev->dev.platform_data;
	 */
	int ret = 0;
#if 0
	struct ab8500_bm_device_info *di;
#endif

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;
	/* get the paltform data i.e the battery information */
	di->pdata = pdev->dev.platform_data;

	di->maintenance_chg = false;
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

	/* Backup Battery */
	di->bk_bat.name = "ab8500_backup_battery";
	di->bk_bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bk_bat.properties = ab8500_bm_bk_battery_props;
	di->bk_bat.num_properties = ARRAY_SIZE(ab8500_bm_bk_battery_props);
	di->bk_bat.get_property = ab8500_bm_bk_battery_get_property;
	di->bk_bat.external_power_changed = NULL;
	di->bk_battery_charge_status = POWER_SUPPLY_STATUS_UNKNOWN;

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
		goto free_device_info;
	}

	/* Create a work queue for en/dis AC/USB from irq handler */
	di->ab8500_bm_irq = create_singlethread_workqueue("ab8500_bm_irq");
	if (di->ab8500_bm_irq == NULL) {
		dev_err(&pdev->dev, "failed to create work queue\n");
		goto free_bm_wq;
	}

	/* Create a work queue for rekicking the watchdog */
	di->ab8500_bm_wd_kick_wq =
	    create_singlethread_workqueue("ab8500_bm_wd_kick_wq");
	if (di->ab8500_bm_wd_kick_wq == NULL) {
		dev_err(&pdev->dev, "failed to create work queue\n");
		goto free_bm_irq;
	}

	/* Work Queue to re-kick the charging watchdog */
	INIT_DELAYED_WORK_DEFERRABLE(&di->ab8500_bm_watchdog_work,
				     ab8500_bm_watchdog_kick_work);

	/* Monitor Main Battery */
	INIT_DELAYED_WORK_DEFERRABLE(&di->ab8500_bm_monitor_work,
				     ab8500_bm_battery_work);

	/* Init work for enable/desable AC/USB charge */
	INIT_WORK(&di->ab8500_bm_ac_en_monitor_work, ab8500_bm_ac_en_work);
	INIT_WORK(&di->ab8500_bm_ac_dis_monitor_work, ab8500_bm_ac_dis_work);
	INIT_WORK(&di->ab8500_bm_usb_en_monitor_work, ab8500_bm_usb_en_work);
	INIT_WORK(&di->ab8500_bm_usb_dis_monitor_work, ab8500_bm_usb_dis_work);

	/* Init work for getting the battery average current */
	INIT_WORK(&di->ab8500_bm_avg_cur_monitor_work, ab8500_bm_avg_cur_work);

	/* Init work to handle usb state changed event from USB stack */
	INIT_WORK(&di->ab8500_bm_usb_state_changed_monitor_work,
			ab8500_bm_usb_state_changed_work);

	/* Init completion to get notified when USB negotiations are
	 * done for a standard host
	 */
	init_completion(&di->ab8500_bm_usb_completed);

	/* Enable coulomb counter */
	ret = ab8500_bm_cc_enable(di, true);
	if (ret)
		dev_err(&pdev->dev, "failed to enable coulomb counter\n");

	/* Enable AC charging */
	ret = ab8500_bm_ac_en(di, true);
	if (ret)
		dev_err(&pdev->dev, "failed to enable AC charging\n");

	/* Enable USB charging */
	ret = ab8500_bm_usb_en(di, true);
	if (ret)
		dev_err(&pdev->dev, "failed to enable USB charging\n");
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
	/* Initialize Gas gauge sysfs interface */
	ret = ab8500_bm_sys_init();
	if (ret)
		dev_err(&pdev->dev, "failed to initialize sysfs for gas gauge\n");

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
	/* sysfs interface to enable/disbale charging from user space */
	ret = ab8500_bm_sysfs_init(di);
	if (ret)
		dev_info(di->dev, "failed to create sysfs entry\n");
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
	/* Delete the work queue */
	destroy_workqueue(di->ab8500_bm_wd_kick_wq);
free_bm_irq:
	destroy_workqueue(di->ab8500_bm_irq);
free_bm_wq:
	destroy_workqueue(di->ab8500_bm_wq);
free_device_info:
	kfree(di);

	return ret;
}

/*
 * struct an8500_bm_driver: AB8500 battery platform structure
 * @probe:	The probe function to be called
 * @remove:	The remove function to be called
 * @suspend:	The suspend function to be called
 * @resume:	The resume function to be called
 * @driver:	The driver data
 */
static struct platform_driver ab8500_bm_driver = {
	.probe = ab8500_bm_probe,
	.remove = __exit_p(ab8500_bm_remove),
	.suspend = ab8500_bm_suspend,
	.resume = ab8500_bm_resume,
	.driver = {
		   .name = "ab8500_bm",
		   },
};

/**
 * ab8500_bm_init() - register the device
 *
 * This function registers the AB8500 battery driver as a platform device.
 **/
static int __init ab8500_bm_init(void)
{
	return platform_driver_register(&ab8500_bm_driver);
}

/**
 * ab8500_bm_exit() - unregister the device
 *
 * This function de-registers the AB8500 battery driver as a platform device.
 **/
static void __exit ab8500_bm_exit(void)
{
	platform_driver_unregister(&ab8500_bm_driver);
}

module_init(ab8500_bm_init);
module_exit(ab8500_bm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ST-Ericsson, Arun R Murthy");
MODULE_ALIAS("Platform:ab8500-bm");
MODULE_DESCRIPTION("AB8500 battery management driver");
