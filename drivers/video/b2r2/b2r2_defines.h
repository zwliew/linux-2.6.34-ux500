/*---------------------------------------------------------------------------*/
/* © copyright STEricsson,2009. All rights reserved. For   */
/* information, STEricsson reserves the right to license    */
/* this software concurrently under separate license conditions.             */
/*                                                                           */
/* This program is free software; you can redistribute it and/or modify it   */
/* under the terms of the GNU Lesser General Public License as published     */
/* by the Free Software Foundation; either version 2.1 of the License,       */
/* or (at your option)any later version.                                     */
/*                                                                           */
/* This program is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See                  */
/* the GNU Lesser General Public License for more details.                   */
/*                                                                           */
/* You should have received a copy of the GNU Lesser General Public License  */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.      */
/*---------------------------------------------------------------------------*/



/**

MODULE      :  B2R2 Device Driver

FILE        :  b2r2_defines.h

DESCRIPTION :  Defines various defines used in B2R2 device driver.

*/

#ifndef __B2R2_DEFINES_H
#define __B2R2_DEFINES_H

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

/** Use known types */

typedef u32 t_uint32;
typedef u16 t_uint16;
typedef u8  t_uint8;
typedef u64 t_uint64;


typedef enum b2r2_context
{

B2R2_CONTEXT_CQ1=0x1,
B2R2_CONTEXT_CQ2=0x2,
B2R2_CONTEXT_AQ1=0x4,
B2R2_CONTEXT_AQ2=0x8,
B2R2_CONTEXT_AQ3=0xc,
B2R2_CONTEXT_AQ4=0x10,
B2R2_CONTEXT_ERROR=0x20

}b2r2_context;

#define	B2R2BLT_CTLGLOBAL_soft_reset	(0x80000000)

#define	B2R2BLT_STA1BDISP_IDLE		    (0x1)

#define	B2R2BLT_ITSMask			(0x8ffff0ff)

#define STN8820 1

#ifdef STN8820

/**

    B2R2 Driver pheripheral clock registers
    enable,disable and status register

    From STn8820_RM_V2.8 pdf(page 598)

    B2R2_CLK_ENABLE_REGISTER  at (0x903B_F000)+0x54 SRC_PCKEN2
    B2R2_CLK_DISABLE_REGISTER at (0x903B_F000)+0x58 SRC_PCKDIS2
    B2R2_CLK_STATUS_REGISTER at (0x903B_F000)+0x60 SRC_PCKSR2

    value to be read and write is 3,5 and 6 bit (page 615)

    binary ~ 001101000 hex ~ 0x68

*/

// #define B2R2_CLK_ENABLE_REGISTER (0x903BF054) // 8820 code


//#define B2R2_CLK_DISABLE_REGISTER (0x903BF058) //8820

//#define B2R2_CLK_STATUS_REGISTER (0x903BF060) //8820

#define B2R2_CLK_FLAG (0x125)

#endif


#ifdef STN8500

#endif


/** Device Driver Minor Number */

#define B2R2_DRIVER_MINOR_NUMBER (238)

/** Maximum number of B2R2 interrupts */

#define B2R2_MAX_INTERRUPTS (6)

#define B2R2_NUMBER_OF_CONTEXTS (7)

#define B2R2_INTERRUPT_SOURCES (4)

/** B2R2 Driver timeout value */

#define B2R2_DRIVER_TIMEOUT_VALUE (1500)

/** B2R2 Driver Interrupt Context */

#define B2R2_CQ1_INTERRUPT_CONTEXT   case 0x1:case 0x2:case 0x3:case 0x4:\
                                     case 0x5:case 0x6:case 0x7:case 0x8:\
                                     case 0x9:case 0xA:case 0xB:case 0xC:\
                                     case 0xD:case 0xE:case 0xF

#define B2R2_CQ2_INTERRUPT_CONTEXT   case 0x10:case 0x20:case 0x30:case 0x40:\
                                     case 0x50:case 0x60:case 0x70:case 0x80:\
                                     case 0x90:case 0xA0:case 0xB0:case 0xC0:\
                                     case 0xD0:case 0xE0:case 0xF0

#define B2R2_AQ1_INTERRUPT_CONTEXT   case 0x1000:case 0x2000:case 0x3000:\
                                     case 0x4000:case 0x5000:case 0x6000:\
                                     case 0x7000:case 0x8000:case 0x9000:\
                                     case 0xA000:case 0xB000:case 0xC000:\
                                     case 0xD000:case 0xE000:case 0xF000

#define B2R2_AQ2_INTERRUPT_CONTEXT   case 0x10000:case 0x20000:case 0x30000:\
                                     case 0x40000:case 0x50000:case 0x60000:\
                                     case 0x70000:case 0x80000:case 0x90000:\
                                     case 0xA0000:case 0xB0000:case 0xC0000:\
                                     case 0xD0000:case 0xE0000:case 0xF0000

#define B2R2_AQ3_INTERRUPT_CONTEXT   case 0x100000:case 0x200000:case 0x300000:\
                                     case 0x400000:case 0x500000:case 0x600000:\
                                     case 0x700000:case 0x800000:case 0x900000:\
                                     case 0xA00000:case 0xB00000:case 0xC00000:\
                                     case 0xD00000:case 0xE00000:case 0xF00000

#define B2R2_AQ4_INTERRUPT_CONTEXT   case 0x1000000:case 0x2000000:case 0x3000000:\
                                     case 0x4000000:case 0x5000000:case 0x6000000:\
                                     case 0x7000000:case 0x8000000:case 0x9000000:\
                                     case 0xA000000:case 0xB000000:case 0xC000000:\
                                     case 0xD000000:case 0xE000000:case 0xF000000

#define B2R2_ERROR_INTERRUPT_CONTEXT case 0x80000000


/** B2R2 Debug messages control */

//#define B2R2_DEBUG_ENABLE 1

#ifdef B2R2_DEBUG_ENABLE

#define dbgprintk(format, args...) \
	do { \
			printk("B2R2:"); \
			printk("%s:",__FUNCTION__); \
			printk(format, ##args); \
	} while(0)

#else

#define dbgprintk(format, args...) \

#endif

/** To be removed once proper solution available */
#define B2R2_RESET_WORKAROUND 1


/** Use top-half only to process interrupt */

#define B2R2_ONLY_TOPHALF (1)

#define B2R2_CONTINUE_ON_ERROR_JOB (1)

#define B2R2_TASK_SUSPEND_TIMEOUT_VALUE (100*(HZ / 1000))


/**  Define assert statement if not defined */

//#define B2R2_ASSERT_CHECK (1)

#ifdef B2R2_ASSERT_CHECK

#define b2r2_assert(a) \
                 if(a) printk("error : assert failing at %s function  %d line ",__FUNCTION__,__LINE__);\

#else

#define b2r2_assert(a) \

#endif


#ifdef _cplusplus
}
#endif /* _cplusplus */

#endif /* !defined(__B2R2_DEFINES_H) */
