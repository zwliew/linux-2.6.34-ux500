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

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/mm_types.h>
#include <linux/hwmem.h>

/*
 * TODO:
 * Make sure ids can double as mmap offsets. Check how the kernel handles
 * offsets and make sure our ids fullfill all the requirements.
 *
 * Count pin unpin at this level to ensure applications can't interfer
 * with each other.
 */

struct hwmem_file {
	struct mutex lock;
	struct idr idr; /* id -> struct hwmem_alloc*, ref counted */
	struct hwmem_alloc *fd_alloc; /* Ref counted */
};

static int create_id(struct hwmem_file *hwfile, struct hwmem_alloc *alloc)
{
	int id, ret;

	while (true) {
		if (idr_pre_get(&hwfile->idr, GFP_KERNEL) == 0) {
			return -ENOMEM;
		}

		ret = idr_get_new_above(&hwfile->idr, alloc, 1, &id);
		if (ret == 0)
			break;
		else if (ret != -EAGAIN)
			return -ENOMEM;
	}

	return (id << PAGE_SHIFT); /* TODO: Probably not OK but works for now. */
}

static void remove_id(struct hwmem_file *hwfile, int id)
{
	idr_remove(&hwfile->idr, id >> PAGE_SHIFT);
}

static struct hwmem_alloc *resolve_id(struct hwmem_file *hwfile, int id)
{
	struct hwmem_alloc *alloc;

	alloc = id ? idr_find(&hwfile->idr, id >> PAGE_SHIFT) : hwfile->fd_alloc;
	if (alloc == NULL)
		alloc = ERR_PTR(-EINVAL);

	return alloc;
}

static int alloc(struct hwmem_file *hwfile, struct hwmem_alloc_request *req)
{
	int ret;
	struct hwmem_alloc *alloc;

	alloc = hwmem_alloc(req->size, req->flags, req->default_access,
								req->mem_type);
	if (IS_ERR(alloc))
		return PTR_ERR(alloc);

	ret = create_id(hwfile, alloc);
	if (ret < 0)
		hwmem_release(alloc);

	return ret;
}

static int alloc_fd(struct hwmem_file *hwfile, struct hwmem_alloc_request *req)
{
	struct hwmem_alloc *alloc;

	if (hwfile->fd_alloc)
		return -EBUSY;

	alloc = hwmem_alloc(req->size, req->flags, req->default_access,
								req->mem_type);
	if (IS_ERR(alloc))
		return PTR_ERR(alloc);

	hwfile->fd_alloc = alloc;

	return 0;
}

static int release(struct hwmem_file *hwfile, s32 id)
{
	struct hwmem_alloc *alloc;

	alloc = resolve_id(hwfile, id);
	if (IS_ERR(alloc))
		return PTR_ERR(alloc);

	remove_id(hwfile, id);
	hwmem_release(alloc);

	return 0;
}

static int hwmem_ioctl_set_domain(struct hwmem_file *hwfile,
					struct hwmem_set_domain_request *req)
{
	struct hwmem_alloc *alloc;

	alloc = resolve_id(hwfile, req->id);
	if (IS_ERR(alloc))
		return PTR_ERR(alloc);

	return hwmem_set_domain(alloc, req->access, req->domain, &req->region);
}

static int pin(struct hwmem_file *hwfile, struct hwmem_pin_request *req)
{
	struct hwmem_alloc *alloc;

	alloc = resolve_id(hwfile, req->id);
	if (IS_ERR(alloc))
		return PTR_ERR(alloc);

	return hwmem_pin(alloc, &req->phys_addr, req->scattered_addrs);
}

static int unpin(struct hwmem_file *hwfile, s32 id)
{
	struct hwmem_alloc *alloc;

	alloc = resolve_id(hwfile, id);
	if (IS_ERR(alloc))
		return PTR_ERR(alloc);

	hwmem_unpin(alloc);

	return 0;
}

static int set_access(struct hwmem_file *hwfile,
					struct hwmem_set_access_request *req)
{
	struct hwmem_alloc *alloc;

	alloc = resolve_id(hwfile, req->id);
	if (IS_ERR(alloc))
		return PTR_ERR(alloc);

	return hwmem_set_access(alloc, req->access, req->pid);
}

static int get_info(struct hwmem_file *hwfile,
					struct hwmem_get_info_request *req)
{
	struct hwmem_alloc *alloc;

	alloc = resolve_id(hwfile, req->id);
	if (IS_ERR(alloc))
		return PTR_ERR(alloc);

	hwmem_get_info(alloc, &req->size, &req->mem_type, &req->access);

	return 0;
}

static int export(struct hwmem_file *hwfile, s32 id)
{
	struct hwmem_alloc *alloc;

	alloc = resolve_id(hwfile, id);
	if (IS_ERR(alloc))
		return PTR_ERR(alloc);

	/*
	 * TODO: The user could be about to send the buffer to a driver but
	 * there is a chance the current thread group don't have import rights
	 * if it gained access to the buffer via a inter-process fd transfer
	 * (fork, Android binder), if this is the case the driver will not be
	 * able to resolve the buffer name. To avoid this situation we give the
	 * current thread group import rights. This will not breach the
	 * security as the process already has access to the buffer (otherwise
	 * it would not be able to get here). This is not a problem right now
	 * as access control is not yet implemented.
	 */

	return hwmem_get_name(alloc);
}

static int import(struct hwmem_file *hwfile, struct hwmem_import_request *req)
{
	int ret;
	struct hwmem_alloc *alloc;

	alloc = hwmem_resolve_by_name(req->name, &req->size, &req->mem_type,
								&req->access);
	if (IS_ERR(alloc))
		return PTR_ERR(alloc);

	ret = create_id(hwfile, alloc);
	if (ret < 0)
		hwmem_release(alloc);

	return ret;
}

static int import_fd(struct hwmem_file *hwfile, struct hwmem_import_request *req)
{
	struct hwmem_alloc *alloc;

	if (hwfile->fd_alloc)
		return -EBUSY;

	alloc = hwmem_resolve_by_name(req->name, &req->size, &req->mem_type,
								&req->access);
	if (IS_ERR(alloc))
		return PTR_ERR(alloc);

	hwfile->fd_alloc = alloc;

	return 0;
}

static int hwmem_open(struct inode *inode, struct file *file)
{
	struct hwmem_file *hwfile;

	hwfile = kzalloc(sizeof(struct hwmem_file), GFP_KERNEL);
	if (hwfile == NULL)
		return -ENOMEM;

	idr_init(&hwfile->idr);
	mutex_init(&hwfile->lock);
	file->private_data = hwfile;

	return 0;
}

static int hwmem_ioctl_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct hwmem_file *hwfile = (struct hwmem_file*)file->private_data;
	struct hwmem_alloc *alloc;

	alloc = resolve_id(hwfile, vma->vm_pgoff << PAGE_SHIFT);
	if (IS_ERR(alloc))
		return PTR_ERR(alloc);

	return hwmem_mmap(alloc, vma);
}

static int hwmem_release_idr_for_each_wrapper(int id, void* ptr, void* data)
{
	hwmem_release((struct hwmem_alloc*)ptr);

	return 0;
}

int hwmem_release_fop(struct inode *inode, struct file *file)
{
	struct hwmem_file *hwfile = (struct hwmem_file*)file->private_data;

	idr_for_each(&hwfile->idr, hwmem_release_idr_for_each_wrapper, NULL);
	idr_destroy(&hwfile->idr);

	if (hwfile->fd_alloc)
		hwmem_release(hwfile->fd_alloc);

	mutex_destroy(&hwfile->lock);

	kfree(hwfile);

	return 0;
}

long hwmem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -ENOSYS;
	struct hwmem_file *hwfile = (struct hwmem_file *)file->private_data;

	mutex_lock(&hwfile->lock);

	switch (cmd) {
	case HWMEM_ALLOC_IOC:
		{
			struct hwmem_alloc_request req;
			if (copy_from_user(&req, (void __user *)arg,
					sizeof(struct hwmem_alloc_request)))
				ret = -EFAULT;
			else
				ret = alloc(hwfile, &req);
		}
		break;
	case HWMEM_ALLOC_FD_IOC:
		{
			struct hwmem_alloc_request req;
			if (copy_from_user(&req, (void __user *)arg,
					sizeof(struct hwmem_alloc_request)))
				ret = -EFAULT;
			else
				ret = alloc_fd(hwfile, &req);
		}
		break;
	case HWMEM_RELEASE_IOC:
		ret = release(hwfile, (s32)arg);
		break;
	case HWMEM_SET_DOMAIN_IOC:
		{
			struct hwmem_set_domain_request req;
			if (copy_from_user(&req, (void __user *)arg,
				sizeof(struct hwmem_set_domain_request)))
				ret = -EFAULT;
			else
				ret = hwmem_ioctl_set_domain(hwfile, &req);
		}
		break;
	case HWMEM_PIN_IOC:
		{
			struct hwmem_pin_request req;
			/*
			 * TODO: Validate and copy scattered_addrs. Not a
			 * problem right now as it's never used.
			 */
			if (copy_from_user(&req, (void __user *)arg,
				sizeof(struct hwmem_pin_request)))
				ret = -EFAULT;
			else
				ret = pin(hwfile, &req);
			if (ret == 0 && copy_to_user((void __user *)arg, &req,
					sizeof(struct hwmem_pin_request)))
				ret = -EFAULT;
		}
		break;
	case HWMEM_UNPIN_IOC:
		ret = unpin(hwfile, (s32)arg);
		break;
	case HWMEM_SET_ACCESS_IOC:
		{
			struct hwmem_set_access_request req;
			if (copy_from_user(&req, (void __user *)arg,
				sizeof(struct hwmem_set_access_request)))
				ret = -EFAULT;
			else
				ret = set_access(hwfile, &req);
		}
		break;
	case HWMEM_GET_INFO_IOC:
		{
			struct hwmem_get_info_request req;
			if (copy_from_user(&req, (void __user *)arg,
				sizeof(struct hwmem_get_info_request)))
				ret = -EFAULT;
			else
				ret = get_info(hwfile, &req);
			if (ret == 0 && copy_to_user((void __user *)arg, &req,
					sizeof(struct hwmem_get_info_request)))
				ret = -EFAULT;
		}
		break;
	case HWMEM_EXPORT_IOC:
		ret = export(hwfile, (s32)arg);
		break;
	case HWMEM_IMPORT_IOC:
		{
			struct hwmem_import_request req;
			if (copy_from_user(&req, (void __user *)arg,
					sizeof(struct hwmem_import_request)))
				ret = -EFAULT;
			else
				ret = import(hwfile, &req);
			if (ret >= 0 && copy_to_user((void __user *)arg, &req,
					sizeof(struct hwmem_import_request)))
				ret = -EFAULT;
		}
		break;
	case HWMEM_IMPORT_FD_IOC:
		{
			struct hwmem_import_request req;
			if (copy_from_user(&req, (void __user *)arg,
					sizeof(struct hwmem_import_request)))
				ret = -EFAULT;
			else
				ret = import_fd(hwfile, &req);
			if (ret == 0 && copy_to_user((void __user *)arg, &req,
					sizeof(struct hwmem_import_request)))
				ret = -EFAULT;
		}
		break;
	}

	mutex_unlock(&hwfile->lock);

	return ret;
}

static struct file_operations hwmem_fops = {
	.open = hwmem_open,
	.mmap = hwmem_ioctl_mmap,
	.unlocked_ioctl = hwmem_ioctl,
	.release = hwmem_release_fop,
};

static struct miscdevice hwmem_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "hwmem",
	.fops = &hwmem_fops,
};

int __init hwmem_ioctl_init(void)
{
	return misc_register(&hwmem_device);
}

void __exit hwmem_ioctl_exit(void)
{
	misc_deregister(&hwmem_device);
}
