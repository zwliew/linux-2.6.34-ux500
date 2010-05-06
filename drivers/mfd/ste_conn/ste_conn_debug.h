/*
 * drivers/mfd/ste_conn/ste_conn_debug.h
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Authors:
 * Par-Gunnar Hjalmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Henrik Possung (henrik.possung@stericsson.com) for ST-Ericsson.
 * Josef Kindberg (josef.kindberg@stericsson.com) for ST-Ericsson.
 * Dariusz Szymszak (dariusz.xd.szymczak@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson. * License terms:  GNU General Public License (GPL), version 2
 *
 * Linux Bluetooth HCI H:4 Driver for ST-Ericsson connectivity controller.
 */

#ifndef _STE_CONN_DEBUG_H_
#define _STE_CONN_DEBUG_H_

#include <linux/kernel.h>
#include <linux/types.h>

#define STE_CONN_DEFAULT_DEBUG_LEVEL 1

/* module_param declared in ste_conn_ccd.c */
extern int ste_debug_level;

#if defined(NDEBUG) || STE_CONN_DEFAULT_DEBUG_LEVEL == 0
	#define STE_CONN_DBG_DATA_CONTENT(fmt, arg...)
	#define STE_CONN_DBG_DATA(fmt, arg...)
	#define STE_CONN_DBG(fmt, arg...)
	#define STE_CONN_INFO(fmt, arg...)
	#define STE_CONN_ERR(fmt, arg...)
	#define STE_CONN_ENTER(fmt, arg...)
	#define STE_CONN_EXIT(fmt, arg...)
#else
	#define STE_CONN_DBG_DATA_CONTENT(fmt, arg...)					\
	if (ste_debug_level >= 30) {							\
		printk(KERN_DEBUG "STE_CONN %s: " fmt "\n" , __func__ , ## arg);	\
	}

	#define STE_CONN_DBG_DATA(fmt, arg...)						\
	if (ste_debug_level >= 25) {							\
		printk(KERN_DEBUG "STE_CONN %s: " fmt "\n" , __func__ , ## arg);	\
	}

	#define STE_CONN_DBG(fmt, arg...)						\
	if (ste_debug_level >= 20) {							\
		printk(KERN_DEBUG "STE_CONN %s: " fmt "\n" , __func__ , ## arg);	\
	}

	#define STE_CONN_INFO(fmt, arg...)				\
	if (ste_debug_level >= 10) {					\
		printk(KERN_INFO "STE_CONN: " fmt "\n" , ## arg);	\
	}

	#define STE_CONN_ERR(fmt, arg...)					\
	if (ste_debug_level >= 1) {						\
		printk(KERN_ERR  "STE_CONN %s: " fmt "\n" , __func__ , ## arg);	\
	}

	#define STE_CONN_ENTER\
	if (ste_debug_level >= 25) {					\
		printk(KERN_DEBUG "STE_CONN_ENTER: %s \n" , __func__);	\
	}

	#define STE_CONN_EXIT\
	if (ste_debug_level >= 25) {					\
		printk(KERN_DEBUG "STE_CONN_EXIT: %s \n" , __func__);	\
	}

#endif /* NDEBUG */

#define STE_CONN_SET_STATE(__ste_conn_name, __ste_conn_var, __ste_conn_new_state)	\
{											\
	STE_CONN_DBG("New %s: 0x%X", __ste_conn_name, (uint32_t)__ste_conn_new_state);	\
	__ste_conn_var = __ste_conn_new_state;						\
}

#endif /* _STE_CONN_DEBUG_H_ */
