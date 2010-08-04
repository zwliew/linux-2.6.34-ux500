/*---------------------------------------------------------------------------*/
/* Copyright (C) ST-Ericsson 2009. 					    */
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

#ifndef _BITMASK_H_
#define _BITMASK_H_

/*-----------------------------------------------------------------------------
 * Bit mask definition
 *---------------------------------------------------------------------------*/
#define TRUE	      0x1
#define FALSE         0x0
#define MASK_NULL8    0x00
#define MASK_NULL16   0x0000
#define MASK_NULL32   0x00000000
#define MASK_ALL8     0xFF
#define MASK_ALL16    0xFFFF
#define MASK_ALL32    0xFFFFFFFF

#define MASK_BIT0     (1UL<<0)
#define MASK_BIT1     (1UL<<1)
#define MASK_BIT2     (1UL<<2)
#define MASK_BIT3     (1UL<<3)
#define MASK_BIT4     (1UL<<4)
#define MASK_BIT5     (1UL<<5)
#define MASK_BIT6     (1UL<<6)
#define MASK_BIT7     (1UL<<7)
#define MASK_BIT8     (1UL<<8)
#define MASK_BIT9     (1UL<<9)
#define MASK_BIT10    (1UL<<10)
#define MASK_BIT11    (1UL<<11)
#define MASK_BIT12    (1UL<<12)
#define MASK_BIT13    (1UL<<13)
#define MASK_BIT14    (1UL<<14)
#define MASK_BIT15    (1UL<<15)
#define MASK_BIT16    (1UL<<16)
#define MASK_BIT17    (1UL<<17)
#define MASK_BIT18    (1UL<<18)
#define MASK_BIT19    (1UL<<19)
#define MASK_BIT20    (1UL<<20)
#define MASK_BIT21    (1UL<<21)
#define MASK_BIT22    (1UL<<22)
#define MASK_BIT23    (1UL<<23)
#define MASK_BIT24    (1UL<<24)
#define MASK_BIT25    (1UL<<25)
#define MASK_BIT26    (1UL<<26)
#define MASK_BIT27    (1UL<<27)
#define MASK_BIT28    (1UL<<28)
#define MASK_BIT29    (1UL<<29)
#define MASK_BIT30    (1UL<<30)
#define MASK_BIT31    (1UL<<31)

/*-----------------------------------------------------------------------------
 * quartet shift definition
 *---------------------------------------------------------------------------*/
#define MASK_QUARTET    (0xFUL)
#define SHIFT_QUARTET0  0
#define SHIFT_QUARTET1  4
#define SHIFT_QUARTET2  8
#define SHIFT_QUARTET3  12
#define SHIFT_QUARTET4  16
#define SHIFT_QUARTET5  20
#define SHIFT_QUARTET6  24
#define SHIFT_QUARTET7  28
#define MASK_QUARTET0   (MASK_QUARTET << SHIFT_QUARTET0)
#define MASK_QUARTET1   (MASK_QUARTET << SHIFT_QUARTET1)
#define MASK_QUARTET2   (MASK_QUARTET << SHIFT_QUARTET2)
#define MASK_QUARTET3   (MASK_QUARTET << SHIFT_QUARTET3)
#define MASK_QUARTET4   (MASK_QUARTET << SHIFT_QUARTET4)
#define MASK_QUARTET5   (MASK_QUARTET << SHIFT_QUARTET5)
#define MASK_QUARTET6   (MASK_QUARTET << SHIFT_QUARTET6)
#define MASK_QUARTET7   (MASK_QUARTET << SHIFT_QUARTET7)

/*-----------------------------------------------------------------------------
 * Byte shift definition
 *---------------------------------------------------------------------------*/
#define MASK_BYTE      (0xFFUL)
#define SHIFT_BYTE0	0
#define SHIFT_BYTE1	8
#define SHIFT_BYTE2	16
#define SHIFT_BYTE3	24
#define MASK_BYTE0      (MASK_BYTE << SHIFT_BYTE0)
#define MASK_BYTE1      (MASK_BYTE << SHIFT_BYTE1)
#define MASK_BYTE2      (MASK_BYTE << SHIFT_BYTE2)
#define MASK_BYTE3      (MASK_BYTE << SHIFT_BYTE3)

/*-----------------------------------------------------------------------------
 * Halfword shift definition
 *---------------------------------------------------------------------------*/
#define MASK_HALFWORD       (0xFFFFUL)
#define SHIFT_HALFWORD0	    0
#define SHIFT_HALFWORD1	    16
#define MASK_HALFWORD0      (MASK_HALFWORD << SHIFT_HALFWORD0)
#define MASK_HALFWORD1      (MASK_HALFWORD << SHIFT_HALFWORD1)

#endif

