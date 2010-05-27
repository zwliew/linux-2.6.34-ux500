/*
 * Copyright (C) ST-Ericsson SA 2010
 * Author: Fredrik Allansson <fredrik.allansson@stericsson.com> for ST-Ericsson
 * License terms: GNU General Public Licence (GPL) version 2.
 */

#include "b2r2_debug.h"
#include <linux/debugfs.h>

int b2r2_log_levels[B2R2_LOG_LEVEL_COUNT];
struct device *b2r2_log_dev;

static struct dentry *root_dir;
static struct dentry *log_lvl_dir;

int b2r2_debug_init(struct device *log_dev)
{
	int i;
	int init_val = 0;

	b2r2_log_dev = log_dev;

#ifdef CONFIG_DYNAMIC_DEBUG
	/*
	 * We want all prints to be enabled by default when using dynamic
	 * debug
	 */
	init_val = 1;
#endif

	for (i = 0; i < B2R2_LOG_LEVEL_COUNT; i++)
		b2r2_log_levels[i] = init_val;

#if !defined(CONFIG_DYNAMIC_DEBUG) && defined(CONFIG_DEBUG_FS)
	/*
	 * If dynamic debug is disabled we need some other way to control the
	 * log prints
	 */
	root_dir = debugfs_create_dir("b2r2_debug", NULL);
	log_lvl_dir = debugfs_create_dir("logs", root_dir);

	/* No need to save the files, they will be removed recursively */
	(void)debugfs_create_bool("warnings", 0644, log_lvl_dir,
				&b2r2_log_levels[B2R2_LOG_LEVEL_WARN]);
	(void)debugfs_create_bool("info", 0644, log_lvl_dir,
				&b2r2_log_levels[B2R2_LOG_LEVEL_INFO]);
	(void)debugfs_create_bool("debug", 0644, log_lvl_dir,
				&b2r2_log_levels[B2R2_LOG_LEVEL_DEBUG]);
	(void)debugfs_create_bool("regdumps", 0644, log_lvl_dir,
				&b2r2_log_levels[B2R2_LOG_LEVEL_REGDUMP]);
#endif

	return 0;
}

void b2r2_debug_exit(void)
{
#if !defined(CONFIG_DYNAMIC_DEBUG) && defined(CONFIG_DEBUG_FS)
	if (root_dir)
		debugfs_remove_recursive(root_dir);
#endif
}
