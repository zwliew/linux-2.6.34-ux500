/*
 *  Copyright (C) 2008 ST Microelectronics
 *  Copyright (C) 2009 ST-Ericsson.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef ASM_ARCH_IRQS_H
#define ASM_ARCH_IRQS_H

#include <mach/hardware.h>

#define IRQ_LOCALTIMER                  29
#define IRQ_LOCALWDOG                   30


#define IRQ_SPI_START			32

/* Interrupt numbers generic for shared peripheral */
#define IRQ_MTU0		(IRQ_SPI_START + 4)
#define IRQ_SPI2		(IRQ_SPI_START + 6)
#define IRQ_SPI0		(IRQ_SPI_START + 8)
#define IRQ_UART0		(IRQ_SPI_START + 11)
#define IRQ_I2C3		(IRQ_SPI_START + 12)
#define IRQ_SSP0		(IRQ_SPI_START + 14)
#define IRQ_MTU1		(IRQ_SPI_START + 17)
#define IRQ_RTC_RTT		(IRQ_SPI_START + 18)
#define IRQ_UART1		(IRQ_SPI_START + 19)
#define IRQ_I2C0		(IRQ_SPI_START + 21)
#define IRQ_I2C1		(IRQ_SPI_START + 22)
#define IRQ_USBOTG		(IRQ_SPI_START + 23)
#define IRQ_DMA			(IRQ_SPI_START + 25)
#define IRQ_UART2		(IRQ_SPI_START + 26)
#define IRQ_HSIR_EXCEP		(IRQ_SPI_START + 29)
#define IRQ_MSP0		(IRQ_SPI_START + 31)
#define IRQ_HSIR_CH0_OVRRUN	(IRQ_SPI_START + 32)
#define IRQ_HSIR_CH1_OVRRUN	(IRQ_SPI_START + 33)
#define IRQ_HSIR_CH2_OVRRUN	(IRQ_SPI_START + 34)
#define IRQ_HSIR_CH3_OVRRUN	(IRQ_SPI_START + 35)
#define STW4500_IRQ		(IRQ_SPI_START + 40)
#define IRQ_SDMMC2		(IRQ_SPI_START + 41)
#define IRQ_SIA_IT0		(IRQ_SPI_START + 42)
#define IRQ_SIA_IT1		(IRQ_SPI_START + 43)
#define IRQ_SVA_IT0		(IRQ_SPI_START + 44)
#define IRQ_SVA_IT1		(IRQ_SPI_START + 45)
#define IRQ_PRCM_ACK_MBOX	(IRQ_SPI_START + 47)
#define IRQ_DISP		(IRQ_SPI_START + 48)
#define IRQ_SPI3		(IRQ_SPI_START + 49)
#define IRQ_SDMMC1		(IRQ_SPI_START + 50)
#define IRQ_I2C4		(IRQ_SPI_START + 51)
#define IRQ_SSP1		(IRQ_SPI_START + 52)
#define IRQ_I2C2		(IRQ_SPI_START + 55)
#define IRQ_SDMMC3		(IRQ_SPI_START + 59)
#define IRQ_SDMMC0		(IRQ_SPI_START + 60)
#define IRQ_HWSEM		(IRQ_SPI_START + 61)
#define IRQ_MSP1		(IRQ_SPI_START + 62)
#define IRQ_SPI1		(IRQ_SPI_START + 96)
#define IRQ_MSP2		(IRQ_SPI_START + 98)
#define IRQ_SDMMC4		(IRQ_SPI_START + 99)
#define IRQ_SDMMC5		(IRQ_SPI_START + 100)
#define IRQ_HSIRD0		(IRQ_SPI_START + 104)
#define IRQ_HSIRD1		(IRQ_SPI_START + 105)
#define IRQ_HSITD0		(IRQ_SPI_START + 106)
#define IRQ_HSITD1		(IRQ_SPI_START + 107)
#define IRQ_GPIO0		(IRQ_SPI_START + 119)
#define IRQ_GPIO1		(IRQ_SPI_START + 120)
#define IRQ_GPIO2		(IRQ_SPI_START + 121)
#define IRQ_GPIO3		(IRQ_SPI_START + 122)
#define IRQ_GPIO4		(IRQ_SPI_START + 123)
#define IRQ_GPIO5		(IRQ_SPI_START + 124)
#define IRQ_GPIO6		(IRQ_SPI_START + 125)
#define IRQ_GPIO7		(IRQ_SPI_START + 126)
#define IRQ_GPIO8		(IRQ_SPI_START + 127)
#define IRQ_B2R2  		(IRQ_SPI_START + 56)

#define IRQ_CA_WAKE_REQ_ED			(IRQ_SPI_START + 71)
#define IRQ_AC_READ_NOTIFICATION_0_ED		(IRQ_SPI_START + 66)
#define IRQ_AC_READ_NOTIFICATION_1_ED		(IRQ_SPI_START + 64)
#define IRQ_CA_MSG_PEND_NOTIFICATION_0_ED	(IRQ_SPI_START + 67)
#define IRQ_CA_MSG_PEND_NOTIFICATION_1_ED	(IRQ_SPI_START + 65)

#define IRQ_CA_WAKE_REQ_V1			(IRQ_SPI_START + 83)
#define IRQ_AC_READ_NOTIFICATION_0_V1		(IRQ_SPI_START + 78)
#define IRQ_AC_READ_NOTIFICATION_1_V1		(IRQ_SPI_START + 76)
#define IRQ_CA_MSG_PEND_NOTIFICATION_0_V1	(IRQ_SPI_START + 79)
#define IRQ_CA_MSG_PEND_NOTIFICATION_1_V1	(IRQ_SPI_START + 77)

#define GPIO_TOTAL_PINS	267

#define MAX_CHIP_IRQ	161
#define MAX_GPIO_IRQ	(MAX_CHIP_IRQ  + GPIO_TOTAL_PINS)

#ifndef NR_IRQS
#define NR_IRQS         (MAX_GPIO_IRQ + 1)
#endif

/*  Macros to get irqno for GPIO pin and vice-versa*/
#define IRQNO_GPIO(x)		(MAX_CHIP_IRQ  + x)
#define GPIO_PIN_FOR_IRQ(x)	(x - MAX_CHIP_IRQ)

/*  Macros to get irqno for DMA channels and vice-versa*/
#define IRQNO_FOR_DMACH(x)	(MAX_GPIO_IRQ + x)
#define DMACH_FOR_IRQNO(x)	(x - MAX_GPIO_IRQ)


#endif /*ASM_ARCH_IRQS_H*/

