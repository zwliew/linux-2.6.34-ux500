/*
 * Overview:
 *  	 stmpe2401 gpio port expander register definitions
 *
 * Copyright (C) 2009 ST Ericsson.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License
 */
#ifndef __STMPE2401_H_
#define __STMPE2401_H_

#include <linux/gpio.h>

/*System control registers REG*/
#define CHIP_ID_REG		0x80
#define VERSION_ID_REG		0x81
#define SYSCON_REG		0x02

/*Interrupt registers REG*/
#define ICR_MSB_REG		0x10
#define ICR_LSB_REG		0x11
#define IER_MSB_REG		0x12
#define IER_LSB_REG		0x13
#define ISR_MSB_REG		0x14
#define ISR_LSB_REG		0x15
#define IEGPIOR_MSB_REG	0x16
#define IEGPIOR_CSB_REG	0x17
#define IEGPIOR_LSB_REG	0x18
#define ISGPIOR_MSB_REG	0x19
#define ISGPIOR_CSB_REG	0x1A
#define ISGPIOR_LSB_REG	0x1B

/* GPIO Monitor Pin register REG */
#define GPMR_MSB_REG		0xA2
#define GPMR_CSB_REG		0xA3
#define GPMR_LSB_REG		0xA4

/* GPIO Set Pin State register REG */
#define GPSR_MSB_REG		0x83
#define GPSR_CSB_REG		0x84
#define GPSR_LSB_REG		0x85

/* GPIO Clear Pin State register REG */
#define GPCR_MSB_REG		0x86
#define GPCR_CSB_REG		0x87
#define GPCR_LSB_REG		0x88

/* GPIO Set Pin Direction register */
#define GPDR_MSB_REG		0x89
#define GPDR_CSB_REG		0x8A
#define GPDR_LSB_REG		0x8B

/* GPIO Edge Detect Status register */
#define GPEDR_MSB_REG		0x8C
#define GPEDR_CSB_REG		0x8D
#define GPEDR_LSB_REG		0x8E

/* GPIO Rising Edge register */
#define GPRER_MSB_REG		0x8F
#define GPRER_CSB_REG		0x90
#define GPRER_LSB_REG		0x91

/* GPIO Falling Edge register */
#define GPFER_MSB_REG		0x92
#define GPFER_CSB_REG		0x93
#define GPFER_LSB_REG		0x94

/* GPIO Pull Up register */
#define GPPUR_MSB_REG		0x95
#define GPPUR_CSB_REG		0x96
#define GPPUR_LSB_REG		0x97

/* GPIO Pull Down register */
#define GPPDR_MSB_REG		0x98
#define GPPDR_CSB_REG		0x99
#define GPPDR_LSB_REG		0x9A

/* GPIO Alternate Function register */
#define GPAFR_U_MSB_REG	0x9b
#define GPAFR_U_CSB_REG	0x9c
#define GPAFR_U_LSB_REG	0x9d

#define GPAFR_L_MSB_REG	0x9e
#define GPAFR_L_CSB_REG	0x9f
#define GPAFR_L_LSB_REG	0xA0

#define	MAX_INT		24

struct stmpe2401_platform_data {
	unsigned	gpio_base;
	int irq;
};

int stmpe2401_remove_callback(int irq);
int stmpe2401_set_callback(int irq, void *handler, void *data);

#endif
