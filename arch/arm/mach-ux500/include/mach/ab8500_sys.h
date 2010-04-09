/*
 * ab8500_gpadc.c - AB8500 GPADC Driver
 *
 * Copyright (C) 2010 ST-Ericsson SA
 * Licensed under GPLv2.
 */

#ifndef _AB8500_BM_SYS_H
#define _AB8500_BM_SYS_H

int ab8500_bm_sys_init(void);
void ab8500_bm_sys_deinit(void);
void ab8500_bm_sys_get_gg_capacity(int *value);
void ab8500_bm_sys_get_gg_samples(int *value);

#endif /* _AB8500_BM_SYS_H */
