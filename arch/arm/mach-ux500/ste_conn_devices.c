/*
 * arch/arm/mach-ux500/ste_conn_devices.c
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Authors:
 * Par-Gunnar Hjalmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Henrik Possung (henrik.possung@stericsson.com) for ST-Ericsson.
 * Josef Kindberg (josef.kindberg@stericsson.com) for ST-Ericsson.
 * Dariusz Szymszak (dariusz.xd.szymczak@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson.
 * License terms:  GNU General Public License (GPL), version 2
 *
 * Board specific device support for the Linux Bluetooth HCI H:4 Driver
 * for ST-Ericsson connectivity controller.
 */

#include <linux/string.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <asm-generic/errno-base.h>
#include <mach/ste_conn_devices.h>
#include <linux/module.h>
#include <linux/gpio.h>

/** BT_ENABLE_GPIO - GPIO to enable/disable the BT module.
 */
#define BT_ENABLE_GPIO			(GPIO(170))

/** GBF_ENA_RESET_GPIO - GPIO to enable/disable the controller.
 */
#define GBF_ENA_RESET_GPIO		(GPIO(171))

/** GBF_ENA_RESET_NAME - Name of GPIO for enabling/disabling.
 */
#define GBF_ENA_RESET_NAME		"gbf_ena_reset"

/** GBF_ENA_RESET_NAME - Name of GPIO for enabling/disabling.
 */
#define BT_ENABLE_NAME			"bt_enable"

#define STE_CONN_ERR(fmt, arg...)	printk(KERN_ERR "STE_CONN %s: " fmt "\n" , __func__ , ## arg)

/* Bluetooth Opcode Group Field */
#define HCI_BT_OGF_LINK_CTRL		0x01
#define HCI_BT_OGF_LINK_POLICY		0x02
#define HCI_BT_OGF_CTRL_BB		0x03
#define HCI_BT_OGF_LINK_INFO		0x04
#define HCI_BT_OGF_LINK_STATUS		0x05
#define HCI_BT_OGF_LINK_TESTING		0x06
#define HCI_BT_OGF_VS			0x3F

#define MAKE_FIRST_BYTE_IN_CMD(__ocf)		((uint8_t)(__ocf & 0x00FF))
#define MAKE_SECOND_BYTE_IN_CMD(__ogf, __ocf)	((uint8_t)(__ogf << 2) | ((__ocf >> 8) & 0x0003))

/* Bluetooth Opcode Command Field */
#define HCI_BT_OCF_RESET		0x0003
#define HCI_BT_OCF_VS_POWER_SWITCH_OFF	0x0140
#define HCI_BT_OCF_VS_SYSTEM_RESET	0x0312
#define HCI_BT_OCF_VS_BT_ENABLE		0x0310

/* Bluetooth HCI command lengths */
#define HCI_BT_LEN_RESET		0x03
#define HCI_BT_LEN_VS_POWER_SWITCH_OFF	0x09
#define HCI_BT_LEN_VS_SYSTEM_RESET	0x03
#define HCI_BT_LEN_VS_BT_ENABLE		0x04

#define H4_HEADER_LENGTH		0x01

#define LOWEST_STLC2690_HCI_REV		0x0600
#define LOWEST_CG2900_HCI_REV		0x0700

static uint8_t	ste_conn_hci_version;
static uint8_t	ste_conn_lmp_version;
static uint8_t	ste_conn_lmp_subversion;
static uint16_t	ste_conn_hci_revision;
static uint16_t	ste_conn_manufacturer;

void ste_conn_devices_enable_chip(void)
{
	gpio_set_value(GBF_ENA_RESET_GPIO, GPIO_HIGH);
}
EXPORT_SYMBOL(ste_conn_devices_enable_chip);

void ste_conn_devices_disable_chip(void)
{
	gpio_set_value(GBF_ENA_RESET_GPIO, GPIO_LOW);
}
EXPORT_SYMBOL(ste_conn_devices_disable_chip);

void ste_conn_devices_set_hci_revision(uint8_t hci_version,
				       uint16_t hci_revision,
				       uint8_t lmp_version,
				       uint8_t lmp_subversion,
				       uint16_t manufacturer)
{
	ste_conn_hci_version	= hci_version;
	ste_conn_hci_revision	= hci_revision;
	ste_conn_lmp_version	= lmp_version;
	ste_conn_lmp_subversion	= lmp_subversion;
	ste_conn_manufacturer	= manufacturer;
}
EXPORT_SYMBOL(ste_conn_devices_set_hci_revision);

struct sk_buff *ste_conn_devices_get_power_switch_off_cmd(uint8_t *op_lsb, uint8_t *op_msb)
{
	struct sk_buff *skb = NULL;
	uint8_t *data = NULL;

	skb = alloc_skb(HCI_BT_LEN_VS_POWER_SWITCH_OFF + H4_HEADER_LENGTH, GFP_KERNEL);
	if (skb) {
		skb_reserve(skb, H4_HEADER_LENGTH);
		data = skb_put(skb, HCI_BT_LEN_VS_POWER_SWITCH_OFF);
		data[0] = MAKE_FIRST_BYTE_IN_CMD(HCI_BT_OCF_VS_POWER_SWITCH_OFF);
		data[1] = MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, HCI_BT_OCF_VS_POWER_SWITCH_OFF);
		data[2] = 0x06;
		/* Enter system specific GPIO settings here:
		 * Section data[3-5] is GPIO pull-up selection
		 * Section data[6-8] is GPIO pull-down selection
		 * Each section is a bitfield where
		 * - byte 0 bit 0 is GPIO 0
		 * - byte 0 bit 1 is GPIO 1
		 * - up to
		 * - byte 2 bit 4 which is GPIO 20
		 * where each bit means:
		 * - 0: No pull-up / no pull-down
		 * - 1: Pull-up / pull-down
		 * All GPIOs are set as input.
		 */
		data[3] = 0x00; /* GPIO 0-7 pull-up */
		data[4] = 0x00; /* GPIO 8-15 pull-up */
		data[5] = 0x00; /* GPIO 16-20 pull-up */
		data[6] = 0x00; /* GPIO 0-7 pull-down */
		data[7] = 0x00; /* GPIO 8-15 pull-down */
		data[8] = 0x00; /* GPIO 16-20 pull-down */
	} else {
		STE_CONN_ERR("Could not allocate skb");
	}

	if (skb) {
		if (op_lsb) {
			*op_lsb = data[0];
		}
		if (op_msb) {
			*op_msb = data[1];
		}
	}

	return skb;
}
EXPORT_SYMBOL(ste_conn_devices_get_power_switch_off_cmd);

struct sk_buff *ste_conn_devices_get_bt_enable_cmd(uint8_t *op_lsb, uint8_t *op_msb, bool bt_enable)
{
	struct sk_buff *skb = NULL;
	uint8_t *data = NULL;

	if (LOWEST_CG2900_HCI_REV <= ste_conn_hci_revision) {
		/* CG2900 used */
		skb = alloc_skb(HCI_BT_LEN_VS_BT_ENABLE + H4_HEADER_LENGTH, GFP_KERNEL);
		if (skb) {
			skb_reserve(skb, H4_HEADER_LENGTH);
			data = skb_put(skb, HCI_BT_LEN_VS_BT_ENABLE);
			data[0] = MAKE_FIRST_BYTE_IN_CMD(HCI_BT_OCF_VS_BT_ENABLE);
			data[1] = MAKE_SECOND_BYTE_IN_CMD(HCI_BT_OGF_VS, HCI_BT_OCF_VS_BT_ENABLE);
			data[2] = 0x01;
			if (bt_enable) {
				data[3] = 0x01;
			} else {
				data[3] = 0x00;
			}
		} else {
			STE_CONN_ERR("Could not allocate skb");
		}
	} else {
		/* No command to send. Leave as NULL */
	}

	if (skb) {
		if (op_lsb) {
			*op_lsb = data[0];
		}
		if (op_msb) {
			*op_msb = data[1];
		}
	}

	return skb;
}
EXPORT_SYMBOL(ste_conn_devices_get_bt_enable_cmd);

int ste_conn_devices_init(void)
{
	int err = 0;

	err = gpio_request(GBF_ENA_RESET_GPIO, GBF_ENA_RESET_NAME);
	if (err < 0) {
		STE_CONN_ERR("gpio_request failed with err: %d", err);
		goto finished;
	}

	err = gpio_direction_output(GBF_ENA_RESET_GPIO, GPIO_HIGH);
	if (err < 0) {
		STE_CONN_ERR("gpio_direction_output failed with err: %d", err);
		goto error_handling;
	}

	err = gpio_request(BT_ENABLE_GPIO, BT_ENABLE_NAME);
	if (err < 0) {
		STE_CONN_ERR("gpio_request failed with err: %d", err);
		goto finished;
	}

	err = gpio_direction_output(BT_ENABLE_GPIO, GPIO_HIGH);
	if (err < 0) {
		STE_CONN_ERR("gpio_direction_output failed with err: %d", err);
		goto error_handling;
	}

	goto finished;

error_handling:
	gpio_free(GBF_ENA_RESET_GPIO);

finished:
	ste_conn_devices_disable_chip();
	return err;
}
EXPORT_SYMBOL(ste_conn_devices_init);

void ste_conn_devices_exit(void)
{
	ste_conn_devices_disable_chip();

	gpio_free(GBF_ENA_RESET_GPIO);
}
EXPORT_SYMBOL(ste_conn_devices_exit);
