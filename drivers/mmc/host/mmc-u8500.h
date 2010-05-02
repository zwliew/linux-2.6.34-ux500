/*
 * Copyright (C) 2008 STMicroelectronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MMC_NOMADIK
#define __MMC_NOMADIK

#define MMC_ERR_NONE    0
#define MMC_ERR_TIMEOUT 1
#define MMC_ERR_BADCRC  2
#define MMC_ERR_FIFO    3
#define MMC_ERR_FAILED  4
#define MMC_ERR_INVALID 5

#define MMC_VDD_18_19   0x00000040  /* VDD voltage 1.8 - 1.9 */
#define MMC_VDD_17_18   0x00000020  /* VDD voltage 1.7 - 1.8 */
#define MMC_VDD_32_33   0x00100000  /* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34   0x00200000  /* VDD voltage 3.3 ~ 3.4 */

#endif

