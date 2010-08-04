/*
 * ab8500-dev.c - simple userspace interface to ab8500
 *
 * Copyright (C) 2009 ST-Ericsson SA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

/*
 * struct ab8500dev_data - AB8500 /dev structure
 * @block:	bank address
 * @addr:	register address
 * @data:	data to be read/write to
 *
 * This supports access to AB8500 chip using normal userspace I/O calls.
 */
struct ab8500dev_data  {
	unsigned char	block;
	unsigned char	addr;
	unsigned char	data;
};

#define AB8500_IOC_MAGIC       'S'
#define AB8500_GET_REGISTER    _IOWR(AB8500_IOC_MAGIC, 1, struct ab8500dev_data)
#define AB8500_SET_REGISTER    _IOW(AB8500_IOC_MAGIC, 2, struct ab8500dev_data)
