/*
 * This file contains the OS and HW dependencies for CAIF.
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef CFGLUE_H_
#define CFGLUE_H_

#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/hardirq.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/crc-ccitt.h>

/* Unsigned 8 bit */
typedef __u8 uint8;

/* Unsigned 16 bit */
typedef __u16 uint16;

/* Unsigned 32 bit */
typedef __u32 uint32;

/*
 *  Handling endiannes:
 *  CAIF uses little-endian byte order.
 */

/* CAIF Endian handling Net to Host of 16 bits unsigned */
#define cfglu_le16_to_cpu(v)  le16_to_cpu(v)

/* CAIF Endian handling Host to Net of 16 bits unsigned */
#define cfglu_cpu_to_le16(v)  cpu_to_le16(v)

/* CAIF Endian handling Host to Net of 32 bits unsigned */
#define cfglu_le32_to_cpu(v)  le32_to_cpu(v)

/* CAIF Endian handling Net to Host of 32 bits unsigned */
#define cfglu_cpu_to_le32(v)  cpu_to_le32(v)


/* Critical Section support, one thread only between startsync
 * and endsync
 */
#define cfglu_lock_t spinlock_t
#define cfglu_init_lock(sync) spin_lock_init(&(sync))
#define cfglu_lock(sync) spin_lock(&(sync))
#define cfglu_unlock(sync) spin_unlock(&(sync))
#define cfglu_deinit_lock(sync)

/* Atomic counting */
#define cfglu_atomic_t atomic_t
#define cfglu_atomic_read(a) atomic_read(&a)
#define cfglu_atomic_set(a, val) atomic_set(&a, val)
#define cfglu_atomic_inc(a) atomic_inc(&a)
#define cfglu_atomic_dec(a) atomic_dec(&a)

/* HEAP */
static inline void *cfglu_alloc(size_t size)
{
	if (in_interrupt())
		return kmalloc(size, GFP_ATOMIC);
	else
		return kmalloc(size, GFP_KERNEL);
}

#define cfglu_free(ptr) kfree(ptr)
#define cfglu_container_of(p, t, m) container_of(p, t, m)

/* Checksum */
static inline uint16 fcs16(uint16 fcs, uint8 *cp, uint16 len)
{
	return crc_ccitt(fcs, cp, len);
}


#ifndef caif_assert
#define caif_assert(assert)\
do if (!(assert)) { \
	pr_err("caif:Assert detected:'%s'\n", #assert); \
	WARN_ON(!(assert));\
} while (0)
#endif

/*FIXME: Comment error codes*/
enum cfglu_errno {
	CFGLU_EOK = 0,
	CFGLU_EPKT = -EPROTO,
	CFGLU_EADDRINUSE = -EADDRINUSE,
	CFGLU_EIO = -EIO,
	CFGLU_EFCS = -EILSEQ,
	CFGLU_EBADPARAM = -EINVAL,
	CFGLU_EINVAL = -EINVAL,
	CFGLU_ENODEV = -ENODEV,
	CFGLU_ENOTCONN = -ENOTCONN,
	CFGLU_EPROTO = -EPROTO,
	CFGLU_EOVERFLOW = -EOVERFLOW,
	CFGLU_ENOMEM = -ENOMEM,
	CFGLU_ERETRY = -EAGAIN,
	CFGLU_ENOSPC = -ENOSPC,
	CFGLU_ENXIO = -ENXIO
};

#endif /* CFGLUE_H_ */
