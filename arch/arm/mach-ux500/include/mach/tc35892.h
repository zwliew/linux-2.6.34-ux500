/*
 * Overview:
 *  	 tc35892 gpio port expander register definitions
 *
 * Copyright (C) 2009 ST Ericsson.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __TC35892_H_
#define __Tc35892_H_

#include <linux/gpio.h>

/*System registers Index*/
#define MANFACTURE_Code_Index		0x80
#define VERSION_ID_Index		0x81
#define IOCFG_Index			0xA7

/*clock control registers*/
#define CLKMODE_Index		0x88
#define CLKCFG_Index		0x89
#define CLKEN_Index		0x8A

/*Reset Control registers*/
#define RSTCTRL_Index		0x82
#define EXTRSTN_Index		0x83
#define RSTINTCLR_index		0x84
#define GPIO_OFFSET

/* Interrupt registers Index*/
#define GPIO_IS0_Index		0xC9
#define GPIO_IS1_Index		0xCA
#define GPIO_IS2_Index		0xCB
#define GPIO_IBE0_Index		0xCC
#define GPIO_IBE1_Index		0xCD
#define GPIO_IBE2_Index		0xCE
#define GPIO_IEV0_Index		0xCF
#define GPIO_IEV1_Index		0xD0
#define GPIO_IEV2_Index		0xD1
#define GPIO_IE0_Index		0xD2
#define GPIO_IE1_Index		0xD3
#define GPIO_IE2_Index		0xD4
#define GPIO_RIS0_Index		0xD6
#define GPIO_RIS1_Index		0xD7
#define GPIO_RIS2_Index		0xD8
#define GPIO_MIS0_Index		0xD9
#define GPIO_MIS1_Index		0xDA
#define GPIO_MIS2_Index		0xDB
#define GPIO_IC0_Index		0xDC
#define GPIO_IC1_Index		0xDD
#define GPIO_IC2_Index		0xDE

/*GPIO's defines*/
/*GPIO data register Index*/
#define GPIO_DATA0_Index	0xC0
#define GPIO_MASK0_Index	0xc1
#define GPIO_DATA1_Index	0xC2
#define GPIO_MASK1_Index	0xc3
#define GPIO_DATA2_Index	0xC4
#define GPIO_MASK2_Index	0xC5

/* GPIO direction register Index*/
#define GPIO_DIR0_Index		0xC6
#define GPIO_DIR1_Index		0xC7
#define GPIO_DIR2_Index		0xC8

/* GPIO Sync registers*/
#define GPIO_SYNC0_Index	0xE6
#define GPIO_SYNC1_Index	0xE7
#define GPIO_SYNC2_Index	0xE8

/*GPIO Wakeup registers*/
#define GPIO_WAKE0_Index	0xE9
#define GPIO_WAKE1_Index	0xEA
#define GPIO_WAKE2_Index	0xEB

/*GPIO OpenDrain registers*/
#define GPIO_ODM0_Index		0xE0
#define GPIO_ODE0_Index		0xE1
#define GPIO_ODM1_Index		0xE2
#define GPIO_ODE1_Index		0xE3
#define GPIO_ODM2_Index		0xE4
#define GPIO_ODE2_Index		0xE5

/*PULL UP REGISTERS*/
#define IOPC0_Index	0xAA
#define IOPC1_Index	0xAC
#define IOPC2_Index	0xAE
#define MAX_TC35892_GPIO	24
#define MAX_INT_EXP		24
#define HIGH 1
#define LOW 0
#define EDGE_SENSITIVE		0
#define LEVEl_SENSITIVE		1
#define DISABLE_INTERRUPT	0
#define ENABLE_INTERRUPT	1
#define TC35892_FALLING_EDGE_OR_LOWLEVEL	1
#define TC35892_RISING_EDGE_OR_HIGHLEVEL	2
#define TC35892_BOTH_EDGE	3

typedef enum {
	EGPIO_PIN_0 = U8500_NR_GPIO,
	EGPIO_PIN_1,
	EGPIO_PIN_2,
	EGPIO_PIN_3,
	EGPIO_PIN_4,
	EGPIO_PIN_5,
	EGPIO_PIN_6,
	EGPIO_PIN_7,
	EGPIO_PIN_8,
	EGPIO_PIN_9,
	EGPIO_PIN_10,
	EGPIO_PIN_11,
	EGPIO_PIN_12,
	EGPIO_PIN_13,
	EGPIO_PIN_14,
	EGPIO_PIN_15,
	EGPIO_PIN_16,
	EGPIO_PIN_17,
	EGPIO_PIN_18,
	EGPIO_PIN_19,
	EGPIO_PIN_20,
	EGPIO_PIN_21,
	EGPIO_PIN_22,
	EGPIO_PIN_23
} egpio_pin;
typedef enum {
	TC35892_OK = 0,
	TC35892_BAD_PARAMETER = -2,
	TC35892_FEAT_NOT_SUPPORTED = -3,
	TC35892_INTERNAL_ERROR = -4,
	TC35892_TIMEOUT_ERROR = -5,
	TC35892_INITIALIZATION_ERROR = -6,
	TC35892_I2C_ERROR = -7,
	TC35892_ERROR = -8
} t_tc35892_error;
/**
 * struct tc35892_platform_data -  tc35892 platform dependent  structure
 * @gpio_base:	starting number of the gpio pin
 * @irq:	irq number of the port expander
 *
 * tc35892 platfoem dependent structure
 **/
struct tc35892_platform_data {
	unsigned	gpio_base;
	int irq;
};
int tc35892_remove_callback(int irq);
int tc35892_set_callback(int irq, void *handler, void *data);
t_tc35892_error tc35892_set_intr_enable (int pin_index, unsigned char intr_enable_disable);
t_tc35892_error tc35892_set_gpio_intr_conf (int pin_index, unsigned char edge_level_sensitive, unsigned char edge_level_type);
#endif
