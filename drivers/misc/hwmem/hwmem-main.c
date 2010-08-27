/*
 * Copyright (C) ST-Ericsson AB 2010
 *
 * Hardware memory driver, hwmem
 *
 * Author: Marcus Lorentzon <marcus.xm.lorentzon@stericsson.com>
 * for ST-Ericsson.
 *
 * License terms: GNU General Public License (GPL), version 2.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/idr.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/pid.h>
#include <linux/list.h>
#include <linux/hwmem.h>

/*
 * TODO:
 * Investigate startup and shutdown, what requirements are there and do we
 * fulfill them?
 */

struct alloc_threadg_info {
	struct list_head list;
	struct pid *threadg_pid; /* Ref counted */
};

struct hwmem_alloc {
	struct list_head list;
	atomic_t ref_cnt;
	enum hwmem_alloc_flags flags;
	u32 start;
	u32 size;
	u32 name;
	struct list_head threadg_info_list;
};

static struct platform_device *hwdev;

static u32 hwmem_start = 0;
static u32 hwmem_size  = 0;

static LIST_HEAD(alloc_list);
static DEFINE_IDR(global_idr);
static DEFINE_MUTEX(lock);

static void vm_open(struct vm_area_struct *vma);
static void vm_close(struct vm_area_struct *vma);
static struct vm_operations_struct vm_ops = {
	.open = vm_open,
	.close = vm_close,
};

/* Helpers */

static void destroy_alloc_threadg_info(
				struct alloc_threadg_info *alloc_threadg_info)
{
	kfree(alloc_threadg_info);
}

static void clean_alloc_threadg_info_list(struct hwmem_alloc *alloc)
{
	while (list_empty(&alloc->threadg_info_list) == 0) {
		struct alloc_threadg_info *i = list_first_entry(
						&alloc->threadg_info_list,
					struct alloc_threadg_info, list);

		list_del(&i->list);

		destroy_alloc_threadg_info(i);
	}
}

static void clean_alloc(struct hwmem_alloc *alloc)
{
	if (alloc->name) {
		idr_remove(&global_idr, alloc->name);
		alloc->name = 0;
	}

	clean_alloc_threadg_info_list(alloc);
}

static void destroy_alloc(struct hwmem_alloc *alloc)
{
	clean_alloc(alloc);

	kfree(alloc);
}

static void __hwmem_release(struct hwmem_alloc *alloc)
{
	struct hwmem_alloc *other;

	clean_alloc(alloc);

	other = list_entry(alloc->list.prev, struct hwmem_alloc, list);
	if (alloc->list.prev != &alloc_list && atomic_read(&other->ref_cnt) == 0) {
		other->size += alloc->size;
		list_del(&alloc->list);
		destroy_alloc(alloc);
		alloc = other;
	}
	other = list_entry(alloc->list.next, struct hwmem_alloc, list);
	if (alloc->list.next != &alloc_list && atomic_read(&other->ref_cnt) == 0) {
		alloc->size += other->size;
		list_del(&other->list);
		destroy_alloc(other);
	}
}

static struct hwmem_alloc *find_free_alloc_bestfit(u32 size)
{
	u32 best_diff = ~0;
	struct hwmem_alloc *alloc = NULL, *i;

	list_for_each_entry(i, &alloc_list, list)
	{
		u32 diff = i->size - size;
		if (atomic_read(&i->ref_cnt) > 0 || i->size < size)
			continue;
		if (diff < best_diff) {
			alloc = i;
			best_diff = diff;
		}
	}

	return alloc != NULL ? alloc : ERR_PTR(-ENOMEM);
}

static struct hwmem_alloc *split_allocation(struct hwmem_alloc *alloc,
							u32 new_alloc_size)
{
	struct hwmem_alloc *new_alloc;

	new_alloc = kzalloc(sizeof(struct hwmem_alloc), GFP_KERNEL);
	if (new_alloc == NULL)
		return ERR_PTR(-ENOMEM);

	atomic_inc(&new_alloc->ref_cnt);
	INIT_LIST_HEAD(&new_alloc->threadg_info_list);
	new_alloc->start = alloc->start;
	new_alloc->size = new_alloc_size;
	alloc->size -= new_alloc_size;
	alloc->start += new_alloc_size;

	list_add_tail(&new_alloc->list, &alloc->list);

	return new_alloc;
}

static int init_alloc_list(void)
{
	struct hwmem_alloc *first_alloc;

	first_alloc = kzalloc(sizeof(struct hwmem_alloc), GFP_KERNEL);
	if (first_alloc == NULL)
		return -ENOMEM;

	first_alloc->start = hwmem_start;
	first_alloc->size = hwmem_size;
	INIT_LIST_HEAD(&first_alloc->threadg_info_list);

	list_add_tail(&first_alloc->list, &alloc_list);

	return 0;
}

static void clean_alloc_list(void)
{
	while (list_empty(&alloc_list) == 0) {
		struct hwmem_alloc *i = list_first_entry(&alloc_list,
						struct hwmem_alloc, list);

		list_del(&i->list);

		destroy_alloc(i);
	}
}

/* HWMEM API */

struct hwmem_alloc *hwmem_alloc(u32 size, enum hwmem_alloc_flags flags,
		enum hwmem_access def_access, enum hwmem_mem_type mem_type)
{
	struct hwmem_alloc *alloc;

	mutex_lock(&lock);

	size = PAGE_ALIGN(size);

	alloc = find_free_alloc_bestfit(size);
	if (IS_ERR(alloc)) {
		dev_info(&hwdev->dev, "Allocation failed, no free slot\n");
		goto no_slot;
	}

	if (size < alloc->size) {
		alloc = split_allocation(alloc, size);
		if (IS_ERR(alloc))
			goto split_alloc_failed;
	}
	else
		atomic_inc(&alloc->ref_cnt);

	alloc->flags = flags;

	goto out;

split_alloc_failed:
no_slot:
out:
	mutex_unlock(&lock);

	return alloc;
}
EXPORT_SYMBOL(hwmem_alloc);

void hwmem_release(struct hwmem_alloc *alloc)
{
	mutex_lock(&lock);

	if (atomic_dec_and_test(&alloc->ref_cnt))
		__hwmem_release(alloc);

	mutex_unlock(&lock);
}
EXPORT_SYMBOL(hwmem_release);

int hwmem_set_domain(struct hwmem_alloc *alloc, enum hwmem_access access,
		enum hwmem_domain domain, struct hwmem_region *region)
{
	mutex_lock(&lock);

	if (domain == HWMEM_DOMAIN_SYNC)
		/*
		 * TODO: Here we want to drain write buffers and wait for
		 * completion but there is no linux function that does that.
		 * On ARMv7 wmb() is implemented in a way that fullfills our
		 * requirements so this should be fine for now.
		 */
		wmb();

	mutex_unlock(&lock);

	return 0;
}
EXPORT_SYMBOL(hwmem_set_domain);

int hwmem_pin(struct hwmem_alloc *alloc, uint32_t *phys_addr,
					uint32_t *scattered_phys_addrs)
{
	mutex_lock(&lock);

	*phys_addr = alloc->start;

	mutex_unlock(&lock);

	return 0;
}
EXPORT_SYMBOL(hwmem_pin);

void hwmem_unpin(struct hwmem_alloc *alloc)
{
}
EXPORT_SYMBOL(hwmem_unpin);

static void vm_open(struct vm_area_struct *vma)
{
	atomic_inc(&((struct hwmem_alloc *)vma->vm_private_data)->ref_cnt);
}

static void vm_close(struct vm_area_struct *vma)
{
	hwmem_release((struct hwmem_alloc *)vma->vm_private_data);
}

int hwmem_mmap(struct hwmem_alloc *alloc, struct vm_area_struct *vma)
{
	int ret = 0;
	unsigned long vma_size = vma->vm_end - vma->vm_start;

	mutex_lock(&lock);

	if (vma_size > (unsigned long)alloc->size) {
		ret = -EINVAL;
		goto illegal_size;
	}

	vma->vm_flags |= VM_RESERVED | VM_IO | VM_DONTEXPAND;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	vma->vm_private_data = (void *)alloc;
	atomic_inc(&alloc->ref_cnt);
	vma->vm_ops = &vm_ops;

	ret = remap_pfn_range(vma, vma->vm_start, alloc->start >> PAGE_SHIFT,
		min(vma_size, (unsigned long)alloc->size), vma->vm_page_prot);
	if (ret < 0)
		goto map_failed;

	goto out;

map_failed:
	atomic_dec(&alloc->ref_cnt);
illegal_size:
out:
	mutex_unlock(&lock);

	return ret;
}
EXPORT_SYMBOL(hwmem_mmap);

int hwmem_set_access(struct hwmem_alloc *alloc, enum hwmem_access access,
								pid_t pid)
{
	return 0;
}
EXPORT_SYMBOL(hwmem_set_access);

void hwmem_get_info(struct hwmem_alloc *alloc, uint32_t *size,
	enum hwmem_mem_type *mem_type, enum hwmem_access *access)
{
	mutex_lock(&lock);

	*size = alloc->size;
	*mem_type = HWMEM_MEM_CONTIGUOUS_SYS;
	*access = HWMEM_ACCESS_READ | HWMEM_ACCESS_WRITE | HWMEM_ACCESS_IMPORT;

	mutex_unlock(&lock);
}
EXPORT_SYMBOL(hwmem_get_info);

int hwmem_get_name(struct hwmem_alloc *alloc)
{
	int ret = 0, name;

	mutex_lock(&lock);

	if (alloc->name != 0) {
		ret = alloc->name;
		goto out;
	}

	while (true) {
		if (idr_pre_get(&global_idr, GFP_KERNEL) == 0) {
			ret = -ENOMEM;
			goto pre_get_id_failed;
		}

		ret = idr_get_new_above(&global_idr, alloc, 1, &name);
		if (ret == 0)
			break;
		else if (ret != -EAGAIN)
			goto get_id_failed;
	}

	alloc->name = name;

	ret = name;
	goto out;

get_id_failed:
pre_get_id_failed:

out:
	mutex_unlock(&lock);

	return ret;
}
EXPORT_SYMBOL(hwmem_get_name);

struct hwmem_alloc *hwmem_resolve_by_name(s32 name, u32 *size,
		enum hwmem_mem_type *mem_type, enum hwmem_access *access)
{
	struct hwmem_alloc *alloc;

	mutex_lock(&lock);

	alloc = idr_find(&global_idr, name);
	if (alloc == NULL) {
		alloc = ERR_PTR(-EINVAL);
		goto find_failed;
	}
	atomic_inc(&alloc->ref_cnt);
	*size = alloc->size;
	*mem_type = HWMEM_MEM_CONTIGUOUS_SYS;
	*access = HWMEM_ACCESS_READ | HWMEM_ACCESS_WRITE | HWMEM_ACCESS_IMPORT;

	goto out;

find_failed:

out:
	mutex_unlock(&lock);

	return alloc;
}
EXPORT_SYMBOL(hwmem_resolve_by_name);

/* Module */

extern int hwmem_ioctl_init(void);
extern void hwmem_ioctl_exit(void);

static int __devinit hwmem_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct hwmem_platform_data *platform_data = pdev->dev.platform_data;

	if (hwdev || platform_data->size == 0) {
		dev_info(&pdev->dev, "hwdev || platform_data->size == 0\n");
		return -EINVAL;
	}

	hwdev = pdev;
	hwmem_start = platform_data->start;
	hwmem_size = platform_data->size;

	ret = init_alloc_list();
	if (ret < 0)
		goto init_alloc_list_failed;

	ret = hwmem_ioctl_init();
	if (ret)
		goto ioctl_init_failed;

	dev_info(&pdev->dev, "Hwmem probed, device contains %#x bytes\n", hwmem_size);

	goto out;

ioctl_init_failed:
	clean_alloc_list();
init_alloc_list_failed:
	hwdev = NULL;
out:
	return ret;
}

static int __devexit hwmem_remove(struct platform_device *pdev)
{
	/*
	 * TODO: This should never happen but if it does we risk crashing the system.
	 * After this call I assume pdev ie hwdev is invalid and must not be used but
	 * we will nonetheless use it when printing in the log.
	 */
	printk(KERN_ERR "Hwmem device removed. Hwmem driver can not yet handle this.\n Any usage of hwmem beyond this point may cause the system to crash.\n");

	return 0;
}

static struct platform_driver hwmem_driver = {
	.probe	= hwmem_probe,
	.remove = hwmem_remove,
	.driver = {
		.name	= "hwmem",
	},
};

static int __init hwmem_init(void)
{
	return platform_driver_register(&hwmem_driver);
}
subsys_initcall(hwmem_init);

static void __exit hwmem_exit(void)
{
	hwmem_ioctl_exit();

	platform_driver_unregister(&hwmem_driver);

	/*
	 * TODO: Release allocated resources! Not a big issue right now as
	 * this code is always built into the kernel and thus this function
	 * is never called.
	 */
}
module_exit(hwmem_exit);

MODULE_AUTHOR("Marcus Lorentzon <marcus.xm.lorentzon@stericsson.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hardware memory driver");

