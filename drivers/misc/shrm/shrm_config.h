/*----------------------------------------------------------------------------*/
/* Copyright (C) ST Ericsson, 2009.                                           */
/*                                                                            */
/* This program is free software; you can redistribute it and/or modify it    */
/* under the terms of the GNU General Public License as published by the Free */
/* Software Foundation; either version 2.1 of the License, or (at your option */
/* any later version.                                                         */
/*                                                                            */
/* This program is distributed in the hope that it will be useful, but WITHOUT*/
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or      */
/* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for   */
/*more details.   							      */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.       */
/*----------------------------------------------------------------------------*/


#ifndef __SHRM_CONFIG_H
#define __SHRM_CONFIG_H


/*
Note: modem need to define IPC as a non-cacheable area.
In Cortex R4 MPU requires that base address of NC area is aligned on a
region-sized boundary.On modem side, only 1 NC area can be defined, hence
the whole IPC area must be defined as NC (at least).

*/

/* cache line size = 32bytes*/
#define SHM_CACHE_LINE 32
#define SHM_PTR_SIZE 4

/* FIFO 0 address configuration */
/* ---------------------------- */
/* 128KB */
#define SHM_FIFO_0_SIZE (128*1024)


/* == APE addresses == */
#define SHM_IPC_BASE_AMCU 0x06F80000
/* offset pointers */
#define SHM_ACFIFO_0_WRITE_AMCU SHM_IPC_BASE_AMCU
#define SHM_ACFIFO_0_READ_AMCU (SHM_ACFIFO_0_WRITE_AMCU + SHM_PTR_SIZE)
#define SHM_CAFIFO_0_WRITE_AMCU (SHM_ACFIFO_0_WRITE_AMCU + SHM_CACHE_LINE)
#define SHM_CAFIFO_0_READ_AMCU (SHM_CAFIFO_0_WRITE_AMCU + SHM_PTR_SIZE)
/* FIFO start */
#define SHM_ACFIFO_0_START_AMCU (SHM_CAFIFO_0_WRITE_AMCU + SHM_CACHE_LINE)
#define SHM_CAFIFO_0_START_AMCU (SHM_ACFIFO_0_START_AMCU + SHM_FIFO_0_SIZE)


/* == CMT addresses ==*/
#define SHM_IPC_BASE_CMCU (SHM_IPC_BASE_AMCU+0x08000000)
/* offset pointers */
#define SHM_ACFIFO_0_WRITE_CMCU SHM_IPC_BASE_CMCU
#define SHM_ACFIFO_0_READ_CMCU (SHM_ACFIFO_0_WRITE_CMCU + SHM_PTR_SIZE)
#define SHM_CAFIFO_0_WRITE_CMCU (SHM_ACFIFO_0_WRITE_CMCU + SHM_CACHE_LINE)
#define SHM_CAFIFO_0_READ_CMCU (SHM_CAFIFO_0_WRITE_CMCU + SHM_PTR_SIZE)
/* FIFO*/
#define SHM_ACFIFO_0_START_CMCU (SHM_CAFIFO_0_WRITE_CMCU + SHM_CACHE_LINE)
#define SHM_CAFIFO_0_START_CMCU (SHM_ACFIFO_0_START_CMCU + SHM_FIFO_0_SIZE)


/* ADSP addresses*/
#define SHM_ACFIFO_0_START_ADSP 0x0
#define SHM_CAFIFO_0_START_ADSP 0x0
#define SHM_ACFIFO_0_WRITE_ADSP 0x0
#define SHM_ACFIFO_0_READ_ADSP 0x0
#define SHM_CAFIFO_0_WRITE_ADSP 0x0
#define SHM_CAFIFO_0_READ_ADSP 0x0

/* FIFO 1 address configuration */
/* ---------------------------- */


/* FIFO 1 - 4K  */
#define SHM_FIFO_1_SIZE (4*1024)


/* == APE addresses == */
#define SHM_ACFIFO_1_WRITE_AMCU (SHM_CAFIFO_0_START_AMCU + SHM_FIFO_0_SIZE)
#define SHM_ACFIFO_1_READ_AMCU (SHM_ACFIFO_1_WRITE_AMCU + SHM_PTR_SIZE)
#define SHM_CAFIFO_1_WRITE_AMCU (SHM_ACFIFO_1_WRITE_AMCU + SHM_CACHE_LINE)
#define SHM_CAFIFO_1_READ_AMCU (SHM_CAFIFO_1_WRITE_AMCU + SHM_PTR_SIZE)
/* FIFO*/
#define SHM_ACFIFO_1_START_AMCU (SHM_CAFIFO_1_WRITE_AMCU + SHM_CACHE_LINE)
#define SHM_CAFIFO_1_START_AMCU (SHM_ACFIFO_1_START_AMCU + SHM_FIFO_1_SIZE)


/* == CMT addresses ==*/
#define SHM_ACFIFO_1_WRITE_CMCU (SHM_CAFIFO_0_START_CMCU + SHM_FIFO_0_SIZE)
#define SHM_ACFIFO_1_READ_CMCU (SHM_ACFIFO_1_WRITE_CMCU + SHM_PTR_SIZE)
#define SHM_CAFIFO_1_WRITE_CMCU (SHM_ACFIFO_1_WRITE_CMCU + SHM_CACHE_LINE)
#define SHM_CAFIFO_1_READ_CMCU (SHM_CAFIFO_1_WRITE_CMCU + SHM_PTR_SIZE)
/* FIFO1 start */
#define SHM_ACFIFO_1_START_CMCU (SHM_CAFIFO_1_WRITE_CMCU + SHM_CACHE_LINE)
#define SHM_CAFIFO_1_START_CMCU (SHM_ACFIFO_1_START_CMCU + SHM_FIFO_1_SIZE)



/* ADSP addresses*/
#define SHM_ACFIFO_1_START_ADSP 0x0
#define SHM_CAFIFO_1_START_ADSP 0x0
#define SHM_ACFIFO_1_WRITE_ADSP 0x0
#define SHM_ACFIFO_1_READ_ADSP 0x0
#define SHM_CAFIFO_1_WRITE_ADSP 0x0
#define SHM_CAFIFO_1_READ_ADSP 0x0


#define U8500_SHM_FIFO_APE_COMMON_BASE  (SHM_ACFIFO_0_START_AMCU)
#define U8500_SHM_FIFO_CMT_COMMON_BASE  (SHM_CAFIFO_0_START_AMCU)
#define U8500_SHM_FIFO_APE_AUDIO_BASE   (SHM_ACFIFO_1_START_AMCU)
#define U8500_SHM_FIFO_CMT_AUDIO_BASE   (SHM_CAFIFO_1_START_AMCU)



#endif /* __SHRM_CONFIG_H */


