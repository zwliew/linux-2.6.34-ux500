#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/list.h>
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#include <linux/fb.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>

EXPORT_SYMBOL(fget_light);
EXPORT_SYMBOL(fput_light);
EXPORT_SYMBOL(flush_cache_range);
EXPORT_SYMBOL(outer_cache);
#ifdef CONFIG_ANDROID_PMEM
EXPORT_SYMBOL(get_pmem_file);
EXPORT_SYMBOL(put_pmem_file);
EXPORT_SYMBOL(flush_pmem_file);
#endif
