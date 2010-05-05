/*
 * Copyright (C) ST-Ericsson AB 2009
 *
 * Author:Paul Wannback <paul.wannback@stericsson.com> for ST-Ericsson
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/list.h>
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#include <linux/fb.h>
#include <linux/uaccess.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif
#include <asm/cacheflush.h>
#include <linux/smp.h>

#include <linux/dma-mapping.h>
#include <asm/dma-mapping.h>

#include <linux/sched.h>

#include "b2r2_internal.h"
#include "b2r2_node_split.h"
#include "b2r2_generic.h"
#include "b2r2_mem_alloc.h"
#include "b2r2_profiler_socket.h"
#include "b2r2_timing.h"

#define B2R2_HEAP_SIZE 4 * PAGE_SIZE
static u32 b2r2_heap_size = 31 * PAGE_SIZE;

//#define B2R2_GENERIC_BLT
//#define B2R2_OPT_BLT
#define B2R2_GEN_OPT_MIX

/*
 * TODO:
 * Implementation of query cap
 * Support for user space virtual pointer to physically consecutive memory
 * Support for user space virtual pointer to physically scattered memory
 * Callback reads lagging behind in blt_api_stress app
 * Store smaller items in the report list instead of the whole request
 * Support read of many report records at once.
 */


#ifndef CONFIG_FB
/* Dummy definition of extern variables */
struct fb_info *registered_fb[FB_MAX];
int num_registered_fb = 0;
#endif

/**
 * b2r2_blt_dev - Our device, /dev/b2r2_blt
 */
static struct miscdevice *b2r2_blt_dev;

/* Statistics */

/**
 * stat_lock - Spin lock protecting the statistics
 */
static spinlock_t   stat_lock;
/**
 * stat_n_jobs_added - Number of jobs added to b2r2_core
 */
static unsigned long stat_n_jobs_added;
/**
 * stat_n_jobs_released - Number of jobs released (job_release called)
 */
static unsigned long stat_n_jobs_released;
/**
 * stat_n_jobs_in_report_list - Number of jobs currently in the report list
 */
static unsigned long stat_n_jobs_in_report_list;
/**
 * stat_n_in_blt - Number of client threads currently exec inside b2r2_blt()
 */
static unsigned long stat_n_in_blt;
/**
 * stat_n_in_blt_synch - Nunmber of client threads currently waiting for synch
 */
static unsigned long stat_n_in_blt_synch;
/**
 * stat_n_in_blt_add - Number of client threads currenlty adding in b2r2_blt
 */
static unsigned long stat_n_in_blt_add;
/**
 * stat_n_in_blt_wait - Number of client threads currently waiting in b2r2_blt
 */
static unsigned long stat_n_in_blt_wait;
/**
 * stat_n_in_sync_0 - Number of client threads currently in b2r2_blt_sync
 *                    waiting for all client jobs to finish
 */
static unsigned long stat_n_in_synch_0;
/**
 * stat_n_in_sync_job - Number of client threads currently in b2r2_blt_sync
 *                      waiting specific job to finish
 */
static unsigned long stat_n_in_synch_job;
/**
 * stat_n_in_query_cap - Number of clients currently in query cap
 */
static unsigned long stat_n_in_query_cap;
/**
 * stat_n_in_open - Number of clients currently in b2r2_blt_open
 */
static unsigned long stat_n_in_open;
/**
 * stat_n_in_release - Number of clients currently in b2r2_blt_release
 */
static unsigned long stat_n_in_release;

/* TODO: Remove when a better way to sense coherent memory has been found. */
static bool flush_mio_pmem;

/* Debug file system support */
#ifdef CONFIG_DEBUG_FS
/**
 * debugfs_latest_request - Copy of the latest request issued
 */
struct b2r2_blt_request debugfs_latest_request;
/**
 * debugfs_latest_first_node - Copy of the first B2R2 node of
 *                             latest request issued
 */
struct b2r2_node debugfs_latest_first_node;
/**
 * debugfs_root_dir - The debugfs root directory, i.e. /debugfs/b2r2
 */
static struct dentry *debugfs_root_dir;

static int sprintf_req(struct b2r2_blt_request *request, char *buf, int size);
#endif

/* Local functions */
static void inc_stat(unsigned long *stat);
static void dec_stat(unsigned long *stat);
static int b2r2_blt(struct b2r2_blt_instance *instance,
		    struct b2r2_blt_request *request);
static int b2r2_generic_blt(struct b2r2_blt_instance *instance,
		    struct b2r2_blt_request *request);
static int b2r2_blt_synch(struct b2r2_blt_instance *instance,
			  int request_id);
static int b2r2_blt_query_cap(struct b2r2_blt_instance *instance,
			      struct b2r2_blt_query_cap *query_cap);

static void job_callback(struct b2r2_core_job *job);
static void job_release(struct b2r2_core_job *job);
static int job_acquire_resources(struct b2r2_core_job *job, bool atomic);
static void job_release_resources(struct b2r2_core_job *job, bool atomic);

static void job_callback_gen(struct b2r2_core_job *job);
static void job_release_gen(struct b2r2_core_job *job);
static int job_acquire_resources_gen(struct b2r2_core_job *job, bool atomic);
static void job_release_resources_gen(struct b2r2_core_job *job, bool atomic);
static void tile_job_callback_gen(struct b2r2_core_job *job);
static void tile_job_release_gen(struct b2r2_core_job *job);
static int resolve_buf(struct b2r2_blt_buf *buf,
		       struct b2r2_resolved_buf *resolved);
static void unresolve_buf(struct b2r2_blt_buf *buf,
			  struct b2r2_resolved_buf *resolved);
static void sync_buf(struct b2r2_blt_buf *buf,
		      struct b2r2_resolved_buf *resolved,
		      bool is_dst);
static bool is_report_list_empty(struct b2r2_blt_instance *instance);
static bool is_synching(struct b2r2_blt_instance *instance);

/**
 * struct sync_args - Data for clean/flush
 *
 * @start: Virtual start address
 * @end: Virtual end address
 */
struct sync_args {
	unsigned long start;
	unsigned long end;
};

/**
 * flush_l1_cache_range_curr_cpu() - Cleans and invalidates L1 cache on the current CPU
 *
 * @arg: Pointer to sync_args structure
 */
static inline void flush_l1_cache_range_curr_cpu(void *arg)
{
	struct sync_args *sa = (struct sync_args *)arg;

	dmac_flush_range((void *)sa->start, (void *)sa->end);
}

#ifdef CONFIG_SMP
/**
 * inv_l1_cache_range_all_cpus() - Cleans and invalidates L1 cache on all CPU:s
 *
 * @sa: Pointer to sync_args structure
 */
static void flush_l1_cache_range_all_cpus(struct sync_args *sa)
{
	on_each_cpu(flush_l1_cache_range_curr_cpu, sa, 1);
}
#endif

/**
 * clean_l1_cache_range_curr_cpu() - Cleans L1 cache on current CPU
 *
 * Ensures that data is written out from the CPU:s L1 cache,
 * it will still be in the cache.
 *
 * @arg: Pointer to sync_args structure
 */
static inline void clean_l1_cache_range_curr_cpu(void *arg)
{
	struct sync_args *sa = (struct sync_args *)arg;

	dmac_map_area((void *)sa->start,
		      (void *)sa->end - (void *)sa->start,
		      DMA_FROM_DEVICE);
}

#ifdef CONFIG_SMP
/**
 * clean_l1_cache_range_all_cpus() - Cleans L1 cache on all CPU:s
 *
 * Ensures that data is written out from all CPU:s L1 cache,
 * it will still be in the cache.
 *
 * @sa: Pointer to sync_args structure
 */
static void clean_l1_cache_range_all_cpus(struct sync_args *sa)
{
	on_each_cpu(clean_l1_cache_range_curr_cpu, sa, 1);
}
#endif

/**
 * b2r2_blt_open - Implements file open on the b2r2_blt device
 *
 * @inode: File system inode
 * @filp: File pointer
 *
 * A B2R2 BLT instance is created and stored in the file structure.
 */
static int b2r2_blt_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct b2r2_blt_instance *instance;

	dev_dbg(b2r2_blt_device(), "%s\n", __func__);

	inc_stat(&stat_n_in_open);

	/* Allocate and initialize the instance */
	instance = (struct b2r2_blt_instance *)
		kmalloc(sizeof(*instance), GFP_KERNEL);
	if (!instance) {
		dev_err(b2r2_blt_device(), "%s: Failed to alloc\n", __func__);
		goto instance_alloc_failed;
	}
	memset(instance, 0, sizeof(*instance));
	INIT_LIST_HEAD(&instance->report_list);
	spin_lock_init(&instance->lock);
	init_waitqueue_head(&instance->report_list_waitq);
	init_waitqueue_head(&instance->synch_done_waitq);

	/* Remember the instance so that we can retrieve it in
	   other functions */
	filp->private_data = instance;
	goto out;

instance_alloc_failed:
out:
	dec_stat(&stat_n_in_open);

	return ret;
}

/**
 * b2r2_blt_release - Implements last close on an instance of
 *                    the b2r2_blt device
 *
 * @inode: File system inode
 * @filp: File pointer
 *
 * All active jobs are finished or cancelled and allocated data
 * is released.
 */
static int b2r2_blt_release(struct inode *inode, struct file *filp)
{
	int ret;
	struct b2r2_blt_instance *instance;

	dev_dbg(b2r2_blt_device(), "%s\n", __func__);

	inc_stat(&stat_n_in_release);

	instance = (struct b2r2_blt_instance *) filp->private_data;

	/* Finish all outstanding requests */
	ret = b2r2_blt_synch(instance, 0);
	if (ret < 0)
		dev_warn(b2r2_blt_device(),
			 "%s: b2r2_blt_sync failed with %d\n", __func__, ret);

	/* Now cancel any remaining outstanding request */
	if (instance->no_of_active_requests) {
		struct b2r2_core_job *job;

		dev_warn(b2r2_blt_device(), "%s: %d active requests\n",
			 __func__, instance->no_of_active_requests);

		/* Find and cancel all jobs belonging to us */
		job = b2r2_core_job_find_first_with_tag((int) instance);
		while (job) {
			b2r2_core_job_cancel(job);
			/* This release matches addref in
			   b2r2_core_job_find... */
			b2r2_core_job_release(job, __func__);
			job = b2r2_core_job_find_first_with_tag((int) instance);
		}

		dev_warn(b2r2_blt_device(),
			 "%s: %d active requests after cancel\n",
			 __func__, instance->no_of_active_requests);
	}

	/* Release jobs in report list */
	spin_lock(&instance->lock);
	while (!list_empty(&instance->report_list)) {
		struct b2r2_blt_request *request = list_first_entry(
			&instance->report_list,
			struct b2r2_blt_request,
			list);
		list_del_init(&request->list);
		spin_unlock(&instance->lock);
		/* This release matches the addref when the job was put into
		   the report list */
		b2r2_core_job_release(&request->job, __func__);
		spin_lock(&instance->lock);
	}
	spin_unlock(&instance->lock);

	/* Release our instance */
	kfree(instance);

	dec_stat(&stat_n_in_release);

	return 0;
}

/**
 * b2r2_blt_ioctl - This routine implements b2r2_blt ioctl interface
 *
 * @inode: inode pointer.
 * @file: file pointer.
 * @cmd	:ioctl command.
 * @arg: input argument for ioctl.
 *
 * Returns 0 if OK else negative error code
 */
static int b2r2_blt_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct b2r2_blt_instance *instance;

	/** Process actual ioctl */

	dev_dbg(b2r2_blt_device(), "%s\n", __func__);

	/* Get the instance from the file structure */
	instance = (struct b2r2_blt_instance *) file->private_data;

	switch (cmd) {
	case B2R2_BLT_IOC: {
		/* This is the "blit" command */

		/* arg is user pointer to struct b2r2_blt_request */
		struct b2r2_blt_request *request =
			kmalloc(sizeof(*request), GFP_KERNEL);
		if (!request) {
			dev_err(b2r2_blt_device(), "%s: Failed to alloc mem\n",
				__func__);
			return -ENOMEM;
		}

		/* Initialize the structure */
		memset(request, 0, sizeof(*request));
		INIT_LIST_HEAD(&request->list);
		request->instance = instance;

		/* The user request is a sub structure of the
		   kernel request structure. */

		/* Get the user data */
		if (copy_from_user(&request->user_req, (void *)arg,
				  sizeof(request->user_req))) {
			dev_err(b2r2_blt_device(),
				"%s: copy_from_user failed\n",
				__func__);
			kfree(request);
			return -EFAULT;
		}

		request->profile = is_profiler_registered_approx();

		/* Perform the blit */
#ifdef B2R2_GENERIC_BLT
		ret = b2r2_generic_blt(instance, request);
#elif defined(B2R2_OPT_BLT)
		ret = b2r2_blt(instance, request);
#elif defined(B2R2_GEN_OPT_MIX)
		ret = b2r2_blt(instance, request);
		if (ret < 0) {
			struct b2r2_blt_request *request_gen;
			printk(KERN_INFO "\nb2r2_blt=%d Going generic.\n", ret);
			request_gen = kmalloc(sizeof(*request_gen), GFP_KERNEL);
			if (!request_gen) {
				dev_err(b2r2_blt_device(),
					"%s: Failed to alloc mem for request_gen\n", __func__);
				return -ENOMEM;
			}

			/* Initialize the structure */
			memset(request_gen, 0, sizeof(*request_gen));
			INIT_LIST_HEAD(&request_gen->list);
			request_gen->instance = instance;

			/* The user request is a sub structure of the
			kernel request structure. */

			/* Get the user data */
			if (copy_from_user(&request_gen->user_req, (void *)arg,
				sizeof(request_gen->user_req))) {
					dev_err(b2r2_blt_device(),
						"%s: copy_from_user failed\n",
						__func__);
					kfree(request_gen);
					return -EFAULT;
			}

			request_gen->profile = is_profiler_registered_approx();

			ret = b2r2_generic_blt(instance, request_gen);
			printk(KERN_INFO "\nb2r2_generic_blt=%d Generic done.\n", ret);
		}
#endif
		break;
	}

	case B2R2_BLT_SYNCH_IOC:
		/* This is the "synch" command */

		/* arg is request_id */
		ret = b2r2_blt_synch(instance, (int) arg);
		break;

	case B2R2_BLT_QUERY_CAP_IOC:
	{
		/* This is the "query capabilities" command */

		/* Arg is struct b2r2_blt_query_cap */
		struct b2r2_blt_query_cap query_cap;

		/* Get the user data */
		if (copy_from_user(&query_cap, (void *)arg,
				  sizeof(query_cap))) {
			dev_err(b2r2_blt_device(),
				"%s: copy_from_user failed\n",
				__func__);
			return -EFAULT;
		}

		/* Fill in our capabilities */
		ret = b2r2_blt_query_cap(instance, &query_cap);

		/* Return data to user */
		if (copy_to_user((void *)arg, &query_cap,
				sizeof(query_cap))) {
			dev_err(b2r2_blt_device(), "%s: copy_to_user failed\n",
				__func__);
			return -EFAULT;
		}
		break;
	}

	default:
		/* Unknown command */
		dev_err(b2r2_blt_device(),
			"%s: Unknown cmd %d\n", __func__, cmd);
		ret = -EINVAL;
	    break;

	}

	return ret;
}

/**
 * b2r2_blt_poll - Support for user-space poll, select & epoll.
 *                 Used for user-space callback
 *
 * @filp: File to poll on
 * @wait: Poll table to wait on
 *
 * This function checks if there are anything to read
 */
static unsigned b2r2_blt_poll(struct file *filp, poll_table *wait)
{
	struct b2r2_blt_instance *instance;
	unsigned int mask = 0;

	dev_dbg(b2r2_blt_device(), "%s\n", __func__);

	/* Get the instance from the file structure */
	instance = (struct b2r2_blt_instance *) filp->private_data;

	poll_wait(filp, &instance->report_list_waitq, wait);
	spin_lock(&instance->lock);
	if (!list_empty(&instance->report_list))
		mask |= POLLIN | POLLRDNORM;
	spin_unlock(&instance->lock);

	return mask;
}

/**
 * b2r2_blt_read - Read report data, user for user-space callback
 *
 * @filp: File pointer
 * @buf: User space buffer
 * @count: Number of bytes to read
 * @f_pos: File position
 *
 * Returns number of bytes read or negative error code
 */
static ssize_t b2r2_blt_read(struct file *filp, char __user *buf, size_t count,
			     loff_t *f_pos)
{
	int ret = 0;
	struct b2r2_blt_instance *instance;
	struct b2r2_blt_request *request;
	struct b2r2_blt_report report;

	dev_dbg(b2r2_blt_device(), "%s\n", __func__);

	/* Get the instance from the file structure */
	instance = (struct b2r2_blt_instance *) filp->private_data;

	/* We return only complete report records, one at a time.
	   Might be more efficient to support read of many. */
	count = (count / sizeof(struct b2r2_blt_report)) *
		sizeof(struct b2r2_blt_report);
	if (count > sizeof(struct b2r2_blt_report))
		count = sizeof(struct b2r2_blt_report);
	if (count == 0)
		return count;

	/* Loop and wait here until we have anything to return or
	   until interrupted */
	spin_lock(&instance->lock);
	while (list_empty(&instance->report_list)) {
		spin_unlock(&instance->lock);

		/* Return if non blocking read */
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;

		dev_dbg(b2r2_blt_device(), "%s - Going to sleep\n", __func__);
		if (wait_event_interruptible(
			    instance->report_list_waitq,
			    !is_report_list_empty(instance)))
			/* signal: tell the fs layer to handle it */
			return -ERESTARTSYS;

		/* Otherwise loop, but first reaquire the lock */
		spin_lock(&instance->lock);
	}

	/* Ok, we have something to return */

	/* Return */
	request = NULL;
	if (!list_empty(&instance->report_list))
		request = list_first_entry(
			&instance->report_list, struct b2r2_blt_request, list);

	if (request) {
		/* Remove from list to avoid reading twice */
		list_del_init(&request->list);

		report.request_id = request->request_id;
		report.report1 = request->user_req.report1;
		report.report2 = request->user_req.report2;
		report.usec_elapsed = 0; /* TBD */

		spin_unlock(&instance->lock);
		if (copy_to_user(buf,
				 &report,
				 sizeof(report)))
			ret = -EFAULT;
		spin_lock(&instance->lock);

		if (ret) {
			/* copy to user failed, re-insert into list */
			list_add(&request->list,
				 &request->instance->report_list);
			request = NULL;
		}
	}
	spin_unlock(&instance->lock);

	if (request)
		/* Release matching the addref when the job was put into
		   the report list */
		b2r2_core_job_release(&request->job, __func__);

	return count;
}

/**
 * b2r2_blt_fops - File operations for b2r2_blt
 */
static const struct file_operations b2r2_blt_fops = {
  .owner =    THIS_MODULE,
  .open =     b2r2_blt_open,
  .release =  b2r2_blt_release,
  .ioctl =    b2r2_blt_ioctl,
  .poll  =    b2r2_blt_poll,
  .read  =    b2r2_blt_read,
};

/**
 * b2r2_blt_misc_dev - Misc device config for b2r2_blt
 */
static struct miscdevice b2r2_blt_misc_dev = {
	MISC_DYNAMIC_MINOR,
	"b2r2_blt",
	&b2r2_blt_fops
};

/**
 * b2r2_blt - Implementation of the B2R2 blit request
 *
 * @instance: The B2R2 BLT instance
 * @request; The request to perform
 */
static int b2r2_blt(struct b2r2_blt_instance *instance,
		    struct b2r2_blt_request *request)
{
	int ret = 0;
	int request_id = 0;
	struct b2r2_node *last_node = request->first_node;
	int node_count;

	u32 thread_runtime_at_start;

	if (request->profile) {
		request->start_time_nsec = b2r2_get_curr_nsec();
		thread_runtime_at_start = (u32)task_sched_runtime(current);
	}

	dev_dbg(b2r2_blt_device(), "%s\n", __func__);

	inc_stat(&stat_n_in_blt);

	/* Debug prints of incoming request */
	dev_dbg(b2r2_blt_device(),
		"src.fmt=%d src.buf={%d,%d,%d} "
		"src.w,h={%d,%d} src.rect={%d,%d,%d,%d}\n",
		request->user_req.src_img.fmt,
		request->user_req.src_img.buf.type,
		request->user_req.src_img.buf.fd,
		request->user_req.src_img.buf.offset,
		request->user_req.src_img.width,
		request->user_req.src_img.height,
		request->user_req.src_rect.x,
		request->user_req.src_rect.y,
		request->user_req.src_rect.width,
		request->user_req.src_rect.height);
	dev_dbg(b2r2_blt_device(),
		"dst.fmt=%d dst.buf={%d,%d,%d} "
		"dst.w,h={%d,%d} dst.rect={%d,%d,%d,%d}\n",
		request->user_req.dst_img.fmt,
		request->user_req.dst_img.buf.type,
		request->user_req.dst_img.buf.fd,
		request->user_req.dst_img.buf.offset,
		request->user_req.dst_img.width,
		request->user_req.dst_img.height,
		request->user_req.dst_rect.x,
		request->user_req.dst_rect.y,
		request->user_req.dst_rect.width,
		request->user_req.dst_rect.height);

	/* Check structure size (sanity check) */
	if (request->user_req.size != sizeof(request->user_req)) {
		dev_err(b2r2_blt_device(),
			"%s: Wrong request size %d, should be %d\n",
			__func__, request->user_req.size,
			sizeof(request->user_req));
		ret = -EINVAL;
		goto wrong_size;
	}

	inc_stat(&stat_n_in_blt_synch);

	/* Wait here if synch is ongoing */
	ret = wait_event_interruptible(instance->synch_done_waitq,
				       !is_synching(instance));
	if (ret) {
		dev_warn(b2r2_blt_device(),
			"%s: Sync wait interrupted, %d\n",
			__func__, ret);
		ret = -EAGAIN;
		dec_stat(&stat_n_in_blt_synch);
		goto synch_interrupted;
	}

	dec_stat(&stat_n_in_blt_synch);

	/* Resolve the buffers */

	/* Source buffer */
	ret = resolve_buf(&request->user_req.src_img.buf,
			  &request->src_resolved);
	if (ret < 0) {
		dev_warn(b2r2_blt_device(),
			"%s: Resolve src buf failed, %d\n",
			__func__, ret);
		ret = -EAGAIN;
		goto resolve_src_buf_failed;
	}

	/* Source mask buffer */
	ret = resolve_buf(&request->user_req.src_mask.buf,
			  &request->src_mask_resolved);
	if (ret < 0) {
		dev_warn(b2r2_blt_device(),
			"%s: Resolve src mask buf failed, %d\n",
			__func__, ret);
		ret = -EAGAIN;
		goto resolve_src_mask_buf_failed;
	}

	/* Destination buffer */
	ret = resolve_buf(&request->user_req.dst_img.buf,
			  &request->dst_resolved);
	if (ret < 0) {
		dev_warn(b2r2_blt_device(),
			"%s: Resolve dst buf failed, %d\n",
			__func__, ret);
		ret = -EAGAIN;
		goto resolve_dst_buf_failed;
	}

	/* Debug prints of resolved buffers */
	dev_dbg(b2r2_blt_device(), "src.rbuf={%X,%p,%d} {%p,%X,%X,%d}\n",
		 request->src_resolved.physical_address,
		 request->src_resolved.virtual_address,
		 request->src_resolved.is_pmem,
		 request->src_resolved.filep,
		 request->src_resolved.file_physical_start,
		 request->src_resolved.file_virtual_start,
		 request->src_resolved.file_len);

	dev_dbg(b2r2_blt_device(), "dst.rbuf={%X,%p,%d} {%p,%X,%X,%d}\n",
		 request->dst_resolved.physical_address,
		 request->dst_resolved.virtual_address,
		 request->dst_resolved.is_pmem,
		 request->dst_resolved.filep,
		 request->dst_resolved.file_physical_start,
		 request->dst_resolved.file_virtual_start,
		 request->dst_resolved.file_len);

	/* Calculate the number of nodes (and resources) needed for this job */
	ret = b2r2_node_split_analyze(request, b2r2_heap_size,
			&node_count, &request->bufs, &request->buf_count,
			&request->node_split_job);
	if (ret == -ENOSYS) {
		/* There was no optimized path for this request */
		dev_dbg(b2r2_blt_device(),
			"%s: No optimized path for request\n", __func__);
		goto no_optimized_path;

	} else if (ret < 0) {
		dev_err(b2r2_blt_device(),
			"%s: Failed to analyze request, ret = %d\n",
			__func__, ret);
#ifdef CONFIG_DEBUG_FS
		{
			/*Failed, dump job to dmesg */
			char *Buf = kmalloc(sizeof(char) * 4096, GFP_KERNEL);

			dev_info(b2r2_blt_device(),
				 "%s: Analyze failed for:\n", __func__);
			if (Buf != NULL) {
				sprintf_req(request, Buf, sizeof(char) * 4096);
				dev_info(b2r2_blt_device(), "%s", Buf);
				kfree(Buf);
			} else {
				dev_info(b2r2_blt_device(), "Unable to print the request. "
					"Message buffer allocation failed.\n");
			}
		}
#endif
		goto generate_nodes_failed;
	}

	/* Allocate the nodes needed */
#ifdef B2R2_USE_NODE_GEN
	request->first_node = b2r2_blt_alloc_nodes(node_count);
	if (request->first_node == NULL) {
		dev_err(b2r2_blt_device(),
			"%s: Failed to allocate nodes, ret = %d\n",
			__func__, ret);
		goto generate_nodes_failed;
	}
#else
	ret = b2r2_node_alloc(node_count, &(request->first_node));
	if (ret < 0 || request->first_node == NULL) {
		dev_err(b2r2_blt_device(),
			"%s: Failed to allocate nodes, ret = %d\n",
			__func__, ret);
		goto generate_nodes_failed;
	}
#endif

	/* Build the B2R2 node list */
	ret = b2r2_node_split_configure(&request->node_split_job,
			request->first_node);

	if (ret < 0) {
		dev_err(b2r2_blt_device(),
			"%s: Failed to perform node split, ret = %d\n",
			__func__, ret);
		goto generate_nodes_failed;
	}

	/* Exit here if dry run */
	if (request->user_req.flags & B2R2_BLT_FLAG_DRY_RUN)
		goto exit_dry_run;

	/* Configure the request */
	last_node = request->first_node;
	while (last_node && last_node->next)
		last_node = last_node->next;

	request->job.tag = (int) instance;
	request->job.prio = request->user_req.prio;
	request->job.first_node_address =
		request->first_node->physical_address;
	request->job.last_node_address =
		last_node->physical_address;
	request->job.callback = job_callback;
	request->job.release = job_release;
	request->job.acquire_resources = job_acquire_resources;
	request->job.release_resources = job_release_resources;

	/* Synchronize memory occupied by the buffers*/

	/* Source buffer */
	if ((request->user_req.flags & B2R2_BLT_FLAG_SRC_NO_CACHE_FLUSH) == 0)
		sync_buf(&request->user_req.src_img.buf,
			  &request->src_resolved, false);

	/* Source mask buffer */
	if ((request->user_req.flags & B2R2_BLT_FLAG_SRC_MASK_NO_CACHE_FLUSH) == 0)
		sync_buf(&request->user_req.src_mask.buf,
			  &request->src_mask_resolved, false);

	/* Destination buffer */
	if ((request->user_req.flags & B2R2_BLT_FLAG_DST_NO_CACHE_FLUSH) == 0)
		sync_buf(&request->user_req.dst_img.buf,
			  &request->dst_resolved, true);

#ifdef CONFIG_DEBUG_FS
	/* Remember latest request and first node for debugfs */
	debugfs_latest_request = *request;
	debugfs_latest_first_node = *request->first_node;
#endif

	/* Submit the job */
	dev_dbg(b2r2_blt_device(), "%s: Submitting job\n", __func__);

	inc_stat(&stat_n_in_blt_add);

	if (request->profile)
		request->nsec_active_in_cpu =
			(s32)((u32)task_sched_runtime(current) - thread_runtime_at_start);

	spin_lock(&instance->lock);

	/* Add the job to b2r2_core */
	request_id = b2r2_core_job_add(&request->job);
	request->request_id = request_id;

	dec_stat(&stat_n_in_blt_add);

	if (request_id < 0) {
		dev_warn(b2r2_blt_device(), "%s: Failed to add job, ret = %d\n",
			__func__, request_id);
		ret = request_id;
		spin_unlock(&instance->lock);
		goto job_add_failed;
	}

	inc_stat(&stat_n_jobs_added);

	instance->no_of_active_requests++;
	spin_unlock(&instance->lock);

	/* Wait for the job to be done if synchronous */
	if ((request->user_req.flags & B2R2_BLT_FLAG_ASYNCH) == 0) {
		dev_dbg(b2r2_blt_device(), "%s: Synchronous, waiting\n",
			 __func__);

		inc_stat(&stat_n_in_blt_wait);

		ret = b2r2_core_job_wait(&request->job);

		dec_stat(&stat_n_in_blt_wait);

		if (ret < 0 && ret != -ENOENT)
			dev_warn(b2r2_blt_device(),
				 "%s: Failed to wait job, ret = %d\n",
				__func__, ret);
		else
			dev_dbg(b2r2_blt_device(),
				"%s: Synchronous wait done\n", __func__);
		ret = 0;
	}

	/* Unresolve the buffers */
	unresolve_buf(&request->user_req.src_img.buf,
		      &request->src_resolved);
	unresolve_buf(&request->user_req.src_mask.buf,
		      &request->src_mask_resolved);
	unresolve_buf(&request->user_req.dst_img.buf,
		      &request->dst_resolved);

	/* Release matching the addref in b2r2_core_job_add,
	   the request must not be accessed after this call */
	b2r2_core_job_release(&request->job, __func__);

	dec_stat(&stat_n_in_blt);

	return ret >= 0 ? request_id : ret;

job_add_failed:
exit_dry_run:
no_optimized_path:
generate_nodes_failed:
	unresolve_buf(&request->user_req.dst_img.buf,
		      &request->dst_resolved);
resolve_dst_buf_failed:
	unresolve_buf(&request->user_req.src_mask.buf,
		      &request->src_mask_resolved);
resolve_src_mask_buf_failed:
	unresolve_buf(&request->user_req.src_img.buf,
		      &request->src_resolved);
resolve_src_buf_failed:
synch_interrupted:
wrong_size:
	job_release(&request->job);
	dec_stat(&stat_n_jobs_released);
	if ((request->user_req.flags & B2R2_BLT_FLAG_DRY_RUN) == 0 || ret)
		dev_warn(b2r2_blt_device(),
			 "%s returns with error %d\n", __func__, ret);

	dec_stat(&stat_n_in_blt);

	return ret;
}

/**
 * Called when job for one tile is done or cancelled
 * in the generic path.
 *
 * @job: The job
 */
static void tile_job_callback_gen(struct b2r2_core_job *job)
{
	if (b2r2_blt_device())
		dev_dbg(b2r2_blt_device(), "%s\n", __func__);

	/* Local addref / release within this func */
	b2r2_core_job_addref(job, __func__);

#ifdef CONFIG_DEBUG_FS
	/* Notify if a tile job is cancelled */
	if (job->job_state == B2R2_CORE_JOB_CANCELED) {
		dev_info(b2r2_blt_device(), "%s: Tile job cancelled:\n", __func__);
	}
#endif

	/* Local addref / release within this func */
	b2r2_core_job_release(job, __func__);
}

/**
 * Called when job is done or cancelled.
 * Used for the last tile in the generic path
 * to notify waiting clients.
 *
 * @job: The job
 */
static void job_callback_gen(struct b2r2_core_job *job)
{
	struct b2r2_blt_request *request =
		container_of(job, struct b2r2_blt_request, job);

	if (b2r2_blt_device())
		dev_dbg(b2r2_blt_device(), "%s\n", __func__);

	/* Local addref / release within this func */
	b2r2_core_job_addref(job, __func__);

	/* Move to report list if the job shall be reported */
	/* FIXME: Use a smaller struct? */
	spin_lock(&request->instance->lock);

	if (request->user_req.flags & B2R2_BLT_FLAG_REPORT_WHEN_DONE) {
		/* Move job to report list */
		list_add_tail(&request->list,
			      &request->instance->report_list);
		inc_stat(&stat_n_jobs_in_report_list);

		/* Wake up poll */
		wake_up_interruptible(
			&request->instance->report_list_waitq);

		/* Add a reference because we put the
		   job in the report list */
		b2r2_core_job_addref(job, __func__);
	}

	/* Decrease number of active requests and wake up
	   synching threads if active requests reaches zero */
	BUG_ON(request->instance->no_of_active_requests == 0);
	request->instance->no_of_active_requests--;
	if (request->instance->synching &&
	    request->instance->no_of_active_requests == 0) {
		request->instance->synching = false;
		/* Wake up all syncing */

		wake_up_interruptible_all(
			&request->instance->synch_done_waitq);
	}
	spin_unlock(&request->instance->lock);

#ifdef CONFIG_DEBUG_FS
	/* Dump job if cancelled */
	if (job->job_state == B2R2_CORE_JOB_CANCELED) {
		char *Buf = kmalloc(sizeof(char) * 4096, GFP_KERNEL);

		dev_info(b2r2_blt_device(), "%s: Job cancelled:\n", __func__);
		if (Buf != NULL) {
			sprintf_req(request, Buf, sizeof(char) * 4096);
			dev_info(b2r2_blt_device(), "%s", Buf);
			kfree(Buf);
		} else {
			dev_info(b2r2_blt_device(), "Unable to print the request. "
					"Message buffer allocation failed.\n");
		}
	}
#endif

	/* Local addref / release within this func */
	b2r2_core_job_release(job, __func__);
}

/**
 * Called when tile job should be released (free memory etc.)
 * Should be used only for tile jobs. Tile jobs should only be used
 * by b2r2_core, thus making ref_count trigger their release.
 *
 * @job: The job
 */

static void tile_job_release_gen(struct b2r2_core_job *job)
{
	inc_stat(&stat_n_jobs_released);

	dev_dbg(b2r2_blt_device(), "%s, first_node_address=0x%.8x, ref_count=%d\n",
		 __func__, job->first_node_address, job->ref_count);

	/* Release memory for the job */
	kfree(job);
}

/**
 * Called when job should be released (free memory etc.)
 *
 * @job: The job
 */

static void job_release_gen(struct b2r2_core_job *job)
{
	struct b2r2_blt_request *request =
		container_of(job, struct b2r2_blt_request, job);

	inc_stat(&stat_n_jobs_released);

	dev_dbg(b2r2_blt_device(), "%s, first_node=%p, ref_count=%d\n",
		 __func__, request->first_node, request->job.ref_count);

	/* Free nodes */
#ifdef B2R2_USE_NODE_GEN
	if (request->first_node)
		b2r2_blt_free_nodes(request->first_node);
#else
	if (request->first_node)
		b2r2_node_free(request->first_node);
#endif

	/* Release memory for the request */
	kfree(request);
}

static int job_acquire_resources_gen(struct b2r2_core_job *job, bool atomic)
{
	/* Nothing so far. Temporary buffers are pre-allocated */
	return 0;
}
static void job_release_resources_gen(struct b2r2_core_job *job, bool atomic)
{
	/* Nothing so far. Temporary buffers are pre-allocated */
}

/**
 * b2r2_generic_blt - Generic implementation of the B2R2 blit request
 *
 * @instance: The B2R2 BLT instance
 * @request; The request to perform
 */
static int b2r2_generic_blt(struct b2r2_blt_instance *instance,
		    struct b2r2_blt_request *request)
{
	int ret = 0;
	int request_id = 0;
	struct b2r2_node *last_node = request->first_node;
	int node_count;
	s32 tmp_buf_width = 0;
	s32 tmp_buf_height = 0;
	u32 tmp_buf_count = 0;
	s32 x;
	s32 y;
	s32 dst_rect_width = request->user_req.dst_rect.width;
	s32 dst_rect_height = request->user_req.dst_rect.height;
	/* Descriptors for the temporary buffers */
	struct b2r2_work_buf work_bufs[4];
	struct b2r2_blt_rect dst_rect_tile;
	int i;

	u32 thread_runtime_at_start;
	s32 nsec_active_in_b2r2 = 0;

	if (request->profile) {
		request->start_time_nsec = b2r2_get_curr_nsec();
		thread_runtime_at_start = (u32)task_sched_runtime(current);
	}

	memset(work_bufs, 0, sizeof(work_bufs));

	dev_dbg(b2r2_blt_device(), "%s\n", __func__);

	inc_stat(&stat_n_in_blt);

	/* Debug prints of incoming request */
	dev_dbg(b2r2_blt_device(),
		"src.fmt=%d flags=0x%.8x src.buf={%d,%d,0x%.8x}\n"
		"src.w,h={%d,%d} src.rect={%d,%d,%d,%d}\n",
		request->user_req.src_img.fmt,
		request->user_req.flags,
		request->user_req.src_img.buf.type,
		request->user_req.src_img.buf.fd,
		request->user_req.src_img.buf.offset,
		request->user_req.src_img.width,
		request->user_req.src_img.height,
		request->user_req.src_rect.x,
		request->user_req.src_rect.y,
		request->user_req.src_rect.width,
		request->user_req.src_rect.height);
	dev_dbg(b2r2_blt_device(),
		"dst.fmt=%d dst.buf={%d,%d,0x%.8x}\n"
		"dst.w,h={%d,%d} dst.rect={%d,%d,%d,%d}\n"
		"dst_clip_rect={%d,%d,%d,%d}\n",
		request->user_req.dst_img.fmt,
		request->user_req.dst_img.buf.type,
		request->user_req.dst_img.buf.fd,
		request->user_req.dst_img.buf.offset,
		request->user_req.dst_img.width,
		request->user_req.dst_img.height,
		request->user_req.dst_rect.x,
		request->user_req.dst_rect.y,
		request->user_req.dst_rect.width,
		request->user_req.dst_rect.height,
		request->user_req.dst_clip_rect.x,
		request->user_req.dst_clip_rect.y,
		request->user_req.dst_clip_rect.width,
		request->user_req.dst_clip_rect.height);

	/* Check structure size (sanity check) */
	if (request->user_req.size != sizeof(request->user_req)) {
		dev_err(b2r2_blt_device(),
			"%s: Wrong request size %d, should be %d\n",
			__func__, request->user_req.size,
			sizeof(request->user_req));
		ret = -EINVAL;
		goto wrong_size;
	}

	inc_stat(&stat_n_in_blt_synch);

	/* Wait here if synch is ongoing */
	ret = wait_event_interruptible(instance->synch_done_waitq,
				       !is_synching(instance));
	if (ret) {
		dev_warn(b2r2_blt_device(),
			"%s: Sync wait interrupted, %d\n",
			__func__, ret);
		ret = -EAGAIN;
		dec_stat(&stat_n_in_blt_synch);
		goto synch_interrupted;
	}

	dec_stat(&stat_n_in_blt_synch);

	/* Resolve the buffers */

	/* Source buffer */
	ret = resolve_buf(&request->user_req.src_img.buf,
			  &request->src_resolved);
	if (ret < 0) {
		dev_warn(b2r2_blt_device(),
			"%s: Resolve src buf failed, %d\n",
			__func__, ret);
		ret = -EAGAIN;
		goto resolve_src_buf_failed;
	}

	/* Source mask buffer */
	ret = resolve_buf(&request->user_req.src_mask.buf,
			  &request->src_mask_resolved);
	if (ret < 0) {
		dev_warn(b2r2_blt_device(),
			"%s: Resolve src mask buf failed, %d\n",
			__func__, ret);
		ret = -EAGAIN;
		goto resolve_src_mask_buf_failed;
	}

	/* Destination buffer */
	ret = resolve_buf(&request->user_req.dst_img.buf,
			  &request->dst_resolved);
	if (ret < 0) {
		dev_warn(b2r2_blt_device(),
			"%s: Resolve dst buf failed, %d\n",
			__func__, ret);
		ret = -EAGAIN;
		goto resolve_dst_buf_failed;
	}

	/* Debug prints of resolved buffers */
	dev_dbg(b2r2_blt_device(), "src.rbuf={%X,%p,%d} {%p,%X,%X,%d}\n",
		 request->src_resolved.physical_address,
		 request->src_resolved.virtual_address,
		 request->src_resolved.is_pmem,
		 request->src_resolved.filep,
		 request->src_resolved.file_physical_start,
		 request->src_resolved.file_virtual_start,
		 request->src_resolved.file_len);

	dev_dbg(b2r2_blt_device(), "dst.rbuf={%X,%p,%d} {%p,%X,%X,%d}\n",
		 request->dst_resolved.physical_address,
		 request->dst_resolved.virtual_address,
		 request->dst_resolved.is_pmem,
		 request->dst_resolved.filep,
		 request->dst_resolved.file_physical_start,
		 request->dst_resolved.file_virtual_start,
		 request->dst_resolved.file_len);

	/* Calculate the number of nodes (and resources) needed for this job */
	ret = b2r2_generic_analyze(request, &tmp_buf_width,
			&tmp_buf_height, &tmp_buf_count, &node_count);
	if (ret < 0) {
		dev_err(b2r2_blt_device(),
			"%s: Failed to analyze request, ret = %d\n",
			__func__, ret);
#ifdef CONFIG_DEBUG_FS
		{
			/*Failed, dump job to dmesg */
			char *Buf = kmalloc(sizeof(char) * 4096, GFP_KERNEL);

			dev_info(b2r2_blt_device(),
				 "%s: Analyze failed for:\n", __func__);
			if (Buf != NULL) {
				sprintf_req(request, Buf, sizeof(char) * 4096);
				dev_info(b2r2_blt_device(), "%s", Buf);
				kfree(Buf);
			} else {
				dev_info(b2r2_blt_device(), "Unable to print the request. "
					"Message buffer allocation failed.\n");
			}
		}
#endif
		goto generate_nodes_failed;
	}

	/* Allocate the nodes needed */
#ifdef B2R2_USE_NODE_GEN
	request->first_node = b2r2_blt_alloc_nodes(node_count);
	if (request->first_node == NULL) {
		dev_err(b2r2_blt_device(),
			"%s: Failed to allocate nodes, ret = %d\n",
			__func__, ret);
		goto generate_nodes_failed;
	}
#else
	ret = b2r2_node_alloc(node_count, &(request->first_node));
	if (ret < 0 || request->first_node == NULL) {
		dev_err(b2r2_blt_device(),
			"%s: Failed to allocate nodes, ret = %d\n",
			__func__, ret);
		goto generate_nodes_failed;
	}
#endif

	/* Allocate the temporary buffers */
	for (i = 0; i < tmp_buf_count; i++) {
		void *virt;
		work_bufs[i].size = tmp_buf_width * tmp_buf_height * 4;

		/*printk(KERN_INFO "b2r2::%s: allocating %d bytes\n", __func__,
				work_bufs[i].size);*/

		virt = dma_alloc_coherent(b2r2_blt_device(),
				work_bufs[i].size,
				&(work_bufs[i].phys_addr),
				GFP_DMA | GFP_KERNEL);
		if (virt == NULL) {
			ret = -ENOMEM;
			goto alloc_work_bufs_failed;
		}

		work_bufs[i].virt_addr = virt;
		memset(work_bufs[i].virt_addr, 0xff, work_bufs[i].size);

		/*printk(KERN_INFO "b2r2::%s: phys=0x%.8x, virt=%p\n", __func__,
				work_bufs[i].phys_addr,
				work_bufs[i].virt_addr);*/
	}
	ret = b2r2_generic_configure(request,
			request->first_node, &work_bufs[0], tmp_buf_count);

	if (ret < 0) {
		dev_err(b2r2_blt_device(),
			"%s: Failed to perform generic configure, ret = %d\n",
			__func__, ret);
		goto generic_conf_failed;
	}

	/* Exit here if dry run */
	if (request->user_req.flags & B2R2_BLT_FLAG_DRY_RUN)
		goto exit_dry_run;

	/* Configure the request and make sure
	 * that its job is run only for the LAST tile.
	 * This is when the request is complete
	 * and waiting clients should be notified.
	 */
	last_node = request->first_node;
	while (last_node && last_node->next)
		last_node = last_node->next;

	request->job.tag = (int) instance;
	request->job.prio = request->user_req.prio;
	request->job.first_node_address =
		request->first_node->physical_address;
	request->job.last_node_address =
		last_node->physical_address;
	request->job.callback = job_callback_gen;
	request->job.release = job_release_gen;
	/* Work buffers and nodes are pre-allocated */
	request->job.acquire_resources = job_acquire_resources_gen;
	request->job.release_resources = job_release_resources_gen;

	/* Flush the L1/L2 cache for the buffers*/

	/* Source buffer */
	if ((request->user_req.flags & B2R2_BLT_FLAG_SRC_NO_CACHE_FLUSH) == 0)
		sync_buf(&request->user_req.src_img.buf,
			  &request->src_resolved, false);

	/* Source mask buffer */
	if ((request->user_req.flags & B2R2_BLT_FLAG_SRC_MASK_NO_CACHE_FLUSH)
	    == 0)
		sync_buf(&request->user_req.src_mask.buf,
			  &request->src_mask_resolved, false);

	/* Destination buffer */
	if ((request->user_req.flags & B2R2_BLT_FLAG_DST_NO_CACHE_FLUSH) == 0)
		sync_buf(&request->user_req.dst_img.buf,
			  &request->dst_resolved, true);

#ifdef CONFIG_DEBUG_FS
	/* Remember latest request and first node for debugfs */
	debugfs_latest_request = *request;
	debugfs_latest_first_node = *request->first_node;
#endif

	/* Same nodes are reused for all the jobs needed to complete the blit.
	 * Nodes are NOT released together with associated job,
	 * as is the case with optimized b2r2_blt() path.
	 */
	spin_lock(&instance->lock);
	instance->no_of_active_requests++;
	spin_unlock(&instance->lock);
	/* Process all but the last row.
	 * dst_rect_height - tmp_buf_height being <=0 is allright.
	 * The loop will not be entered since y == 0
	 */
	for (y = 0; y < dst_rect_height - tmp_buf_height; y += tmp_buf_height) {
		/* Tile in the destination rectangle being processed */
		struct b2r2_blt_rect dst_rect_tile;
		dst_rect_tile.y = y;
		dst_rect_tile.width = tmp_buf_width;
		dst_rect_tile.height = tmp_buf_height;

		for (x = 0; x < dst_rect_width; x += tmp_buf_width) {
			/* Tile jobs are freed by the supplied release function
			 * when ref_count on a tile_job reaches zero.
			 */
			struct b2r2_core_job *tile_job = kmalloc(sizeof(*tile_job), GFP_KERNEL);
			if (tile_job == NULL) {
				/* Skip this tile. Do not abort, just hope for better luck
				 * with rest of the tiles. Memory might become available.
				 */
				dev_dbg(b2r2_blt_device(), "%s: Failed to alloc job. "
					"Skipping tile at (x, y)=(%d, %d)\n", __func__, x, y);
				continue;
			}
			tile_job->tag = request->job.tag;
			tile_job->prio = request->job.prio;
			tile_job->first_node_address = request->job.first_node_address;
			tile_job->last_node_address = request->job.last_node_address;
			tile_job->callback = tile_job_callback_gen;
			tile_job->release = tile_job_release_gen;
			/* Work buffers and nodes are pre-allocated */
			tile_job->acquire_resources = job_acquire_resources_gen;
			tile_job->release_resources = job_release_resources_gen;

			dst_rect_tile.x = x;
			if (x + tmp_buf_width <= dst_rect_width) {
				/* Whole tile can be written. */
				dst_rect_tile.width = tmp_buf_width;
			} else {
				/* Only a part of the tile can be written */
				dst_rect_tile.width = dst_rect_width - x;
			}
			/* Where applicable, calculate area in src buffer that is needed
			 * to generate the specified part of destination rectangle.
			 */
			b2r2_generic_set_areas(request, request->first_node, &dst_rect_tile);
			/* Submit the job */
			dev_dbg(b2r2_blt_device(), "%s: Submitting job\n", __func__);

			inc_stat(&stat_n_in_blt_add);

			spin_lock(&instance->lock);

			request_id = b2r2_core_job_add(tile_job);

			dec_stat(&stat_n_in_blt_add);

			if (request_id < 0) {
				dev_warn(b2r2_blt_device(), "%s: Failed to add tile job, ret = %d\n",
					__func__, request_id);
				ret = request_id;
				spin_unlock(&instance->lock);
				goto job_add_failed;
			}

			inc_stat(&stat_n_jobs_added);

			spin_unlock(&instance->lock);

			/* Wait for the job to be done */
			dev_dbg(b2r2_blt_device(), "%s: Synchronous, waiting\n",
				 __func__);

			inc_stat(&stat_n_in_blt_wait);

			ret = b2r2_core_job_wait(tile_job);

			dec_stat(&stat_n_in_blt_wait);

			if (ret < 0 && ret != -ENOENT)
				dev_warn(b2r2_blt_device(),
					 "%s: Failed to wait job, ret = %d\n",
					__func__, ret);
			else {
				dev_dbg(b2r2_blt_device(),
					"%s: Synchronous wait done\n", __func__);

				nsec_active_in_b2r2 += tile_job->nsec_active_in_hw;
			}
			/* Release matching the addref in b2r2_core_job_add */
			b2r2_core_job_release(tile_job, __func__);
		}
	}
	for (x = 0; x < dst_rect_width; x += tmp_buf_width) {
		struct b2r2_core_job *tile_job = NULL;
		if (x + tmp_buf_width < dst_rect_width) {
			/* Tile jobs are freed by the supplied release function
			 * when ref_count on a tile_job reaches zero.
			 * Do NOT allocate a tile_job for the last tile.
			 * Send the job from the request. This way clients
			 * will be notified when the whole blit is complete
			 * and not just part of it.
			 */
			tile_job = kmalloc(sizeof(*tile_job), GFP_KERNEL);
			if (tile_job == NULL) {
				/**/
				dev_dbg(b2r2_blt_device(), "%s: Failed to alloc job. "
					"Skipping tile at (x, y)=(%d, %d)\n", __func__, x, y);
				continue;
			}
			tile_job->tag = request->job.tag;
			tile_job->prio = request->job.prio;
			tile_job->first_node_address = request->job.first_node_address;
			tile_job->last_node_address = request->job.last_node_address;
			tile_job->callback = tile_job_callback_gen;
			tile_job->release = tile_job_release_gen;
			tile_job->acquire_resources = job_acquire_resources_gen;
			tile_job->release_resources = job_release_resources_gen;
		}

		dst_rect_tile.x = x;
		if (x + tmp_buf_width <= dst_rect_width) {
			/* Whole tile can be written. */
			dst_rect_tile.width = tmp_buf_width;
		} else {
			/* Only a part of the tile can be written */
			dst_rect_tile.width = dst_rect_width - x;
		}
		/* y is now the last row */
		dst_rect_tile.y = y;
		dst_rect_tile.height = dst_rect_height - y;
		b2r2_generic_set_areas(request, request->first_node, &dst_rect_tile);

		dev_dbg(b2r2_blt_device(), "%s: Submitting job\n", __func__);
		inc_stat(&stat_n_in_blt_add);

		//printk("%s last row (x,y)=(%d, %d)\n", __func__, x, y);

		spin_lock(&instance->lock);
		if (x + tmp_buf_width < dst_rect_width) {
			request_id = b2r2_core_job_add(tile_job);
		} else {
			/* Last tile. Send the job-struct from the request.
			 * Clients will be notified once it completes.
			 */
			request_id = b2r2_core_job_add(&request->job);
		}

		dec_stat(&stat_n_in_blt_add);

		if (request_id < 0) {
			dev_warn(b2r2_blt_device(), "%s: Failed to add tile job, ret = %d\n",
				__func__, request_id);
			ret = request_id;
			spin_unlock(&instance->lock);
			goto job_add_failed;
		}

		inc_stat(&stat_n_jobs_added);
		spin_unlock(&instance->lock);

		dev_dbg(b2r2_blt_device(), "%s: Synchronous, waiting\n",
			__func__);

		inc_stat(&stat_n_in_blt_wait);
		if (x + tmp_buf_width < dst_rect_width) {
			ret = b2r2_core_job_wait(tile_job);
		} else {
			/* Last tile. Wait for the job-struct from the request. */
			ret = b2r2_core_job_wait(&request->job);
		}
		dec_stat(&stat_n_in_blt_wait);

		if (ret < 0 && ret != -ENOENT)
			dev_warn(b2r2_blt_device(),
				"%s: Failed to wait job, ret = %d\n",
				__func__, ret);
		else {
			dev_dbg(b2r2_blt_device(),
				"%s: Synchronous wait done\n", __func__);

			if (x + tmp_buf_width < dst_rect_width)
				nsec_active_in_b2r2 += tile_job->nsec_active_in_hw;
			else
				nsec_active_in_b2r2 += request->job.nsec_active_in_hw;
		}

		/* Release matching the addref in b2r2_core_job_add.
		 * Make sure that the correct job-struct is released
		 * when the last tile is processed.
		 */
		if (x + tmp_buf_width < dst_rect_width) {
			b2r2_core_job_release(tile_job, __func__);
		} else {
			b2r2_core_job_release(&request->job, __func__);
		}
	}


	ret = 0;


	/* Unresolve the buffers */
	unresolve_buf(&request->user_req.src_img.buf,
		      &request->src_resolved);
	unresolve_buf(&request->user_req.src_mask.buf,
		      &request->src_mask_resolved);
	unresolve_buf(&request->user_req.dst_img.buf,
		      &request->dst_resolved);

	dec_stat(&stat_n_in_blt);

	for (i = 0; i < tmp_buf_count; i++) {
		/*printk(KERN_INFO "b2r2::%s: freeing %d bytes\n",
				__func__, work_bufs[i].size);*/
		dma_free_coherent(b2r2_blt_device(),
			  work_bufs[i].size,
			  work_bufs[i].virt_addr,
			  work_bufs[i].phys_addr);
		memset(&(work_bufs[i]), 0, sizeof(work_bufs[i]));
	}

	if (request->profile) {
		request->nsec_active_in_cpu =
			(s32)((u32)task_sched_runtime(current) - thread_runtime_at_start);
		request->total_time_nsec =
			(s32)(b2r2_get_curr_nsec() - request->start_time_nsec);
		request->job.nsec_active_in_hw = nsec_active_in_b2r2;

		b2r2_call_profiler_blt_done(request);
	}

	return ret >= 0 ? request_id : ret;

job_add_failed:
exit_dry_run:
generic_conf_failed:
alloc_work_bufs_failed:
	for (i = 0; i < 4; i++) {
		if (work_bufs[i].virt_addr != 0)
		{
			/*printk(KERN_INFO "b2r2::%s: freeing %d bytes\n",
					__func__, work_bufs[i].size);*/
			dma_free_coherent(b2r2_blt_device(),
				  work_bufs[i].size,
				  work_bufs[i].virt_addr,
				  work_bufs[i].phys_addr);
			memset(&(work_bufs[i]), 0, sizeof(work_bufs[i]));
		}
	}

generate_nodes_failed:
	unresolve_buf(&request->user_req.dst_img.buf,
		      &request->dst_resolved);
resolve_dst_buf_failed:
	unresolve_buf(&request->user_req.src_mask.buf,
		      &request->src_mask_resolved);
resolve_src_mask_buf_failed:
	unresolve_buf(&request->user_req.src_img.buf,
		      &request->src_resolved);
resolve_src_buf_failed:
synch_interrupted:
wrong_size:
	job_release_gen(&request->job);
	dec_stat(&stat_n_jobs_released);
	if ((request->user_req.flags & B2R2_BLT_FLAG_DRY_RUN) == 0 || ret)
		dev_warn(b2r2_blt_device(),
			 "%s returns with error %d\n", __func__, ret);

	dec_stat(&stat_n_in_blt);

	//printk(KERN_INFO "b2r2:%s error ret=%d", __func__, ret);
	return ret;
}

/**
 * b2r2_blt_synch - Implements wait for all or a specified job
 *
 * @instance: The B2R2 BLT instance
 * @request_id: If 0, wait for all requests on this instance to finish.
 *              Else wait for request with given request id to finish.
 */
static int b2r2_blt_synch(struct b2r2_blt_instance *instance,
			  int request_id)
{
	int ret = 0;
	dev_dbg(b2r2_blt_device(), "%s, request_id=%d\n", __func__, request_id);

	if (request_id == 0) {
		/* Wait for all requests */
		inc_stat(&stat_n_in_synch_0);

		/* Enter state "synching" if we have any active request */
		spin_lock(&instance->lock);
		if (instance->no_of_active_requests)
			instance->synching = true;
		spin_unlock(&instance->lock);

		/* Wait until no longer in state synching */
		ret = wait_event_interruptible(instance->synch_done_waitq,
					       !is_synching(instance));
		dec_stat(&stat_n_in_synch_0);
	} else {
		struct b2r2_core_job *job;

		inc_stat(&stat_n_in_synch_job);

		/* Wait for specific job */
		job = b2r2_core_job_find(request_id);
		if (job) {
			/* Wait on find job */
			ret = b2r2_core_job_wait(job);
			/* Release matching the addref in b2r2_core_job_find */
			b2r2_core_job_release(job, __func__);
		}

		/* If job not found we assume that is has been run */

		dec_stat(&stat_n_in_synch_job);
	}

	dev_dbg(b2r2_blt_device(),
		"%s, request_id=%d, returns %d\n", __func__, request_id, ret);

	return ret;
}

/**
 * Query B2R2 capabilities
 *
 * @instance: The B2R2 BLT instance
 * @query_cap: The structure receiving the capabilities
 */
static int b2r2_blt_query_cap(struct b2r2_blt_instance *instance,
		   struct b2r2_blt_query_cap *query_cap)
{
	/* FIXME: Not implemented yet */
	return -ENOSYS;
}

/**
 * Called when job is done or cancelled
 *
 * @job: The job
 */
static void job_callback(struct b2r2_core_job *job)
{
	struct b2r2_blt_request *request =
		container_of(job, struct b2r2_blt_request, job);

	if (b2r2_blt_device())
		dev_dbg(b2r2_blt_device(), "%s\n", __func__);

	/* Local addref / release within this func */
	b2r2_core_job_addref(job, __func__);

	/* Move to report list if the job shall be reported */
	/* FIXME: Use a smaller struct? */
	spin_lock(&request->instance->lock);
	if (request->user_req.flags & B2R2_BLT_FLAG_REPORT_WHEN_DONE) {
		/* Move job to report list */
		list_add_tail(&request->list,
			      &request->instance->report_list);
		inc_stat(&stat_n_jobs_in_report_list);

		/* Wake up poll */
		wake_up_interruptible(
			&request->instance->report_list_waitq);

		/* Add a reference because we put the
		   job in the report list */
		b2r2_core_job_addref(job, __func__);
	}

	/* Decrease number of active requests and wake up
	   synching threads if active requests reaches zero */
	BUG_ON(request->instance->no_of_active_requests == 0);
	request->instance->no_of_active_requests--;
	if (request->instance->synching &&
	    request->instance->no_of_active_requests == 0) {
		request->instance->synching = false;
		/* Wake up all syncing */

		wake_up_interruptible_all(
			&request->instance->synch_done_waitq);
	}
	spin_unlock(&request->instance->lock);

#ifdef CONFIG_DEBUG_FS
	/* Dump job if cancelled */
	if (job->job_state == B2R2_CORE_JOB_CANCELED) {
		char *Buf = kmalloc(sizeof(char) * 4096, GFP_KERNEL);

		dev_info(b2r2_blt_device(), "%s: Job cancelled:\n", __func__);
		if (Buf != NULL) {
			sprintf_req(request, Buf, sizeof(char) * 4096);
			dev_info(b2r2_blt_device(), "%s", Buf);
			kfree(Buf);
		} else {
			dev_info(b2r2_blt_device(), "Unable to print the request. "
					"Message buffer allocation failed.\n");
		}
	}
#endif

	if (request->profile) {
		request->total_time_nsec =
			(s32)(b2r2_get_curr_nsec() - request->start_time_nsec);
		b2r2_call_profiler_blt_done(request);
	}

	/* Local addref / release within this func */
	b2r2_core_job_release(job, __func__);
}

/**
 * Called when job should be released (free memory etc.)
 *
 * @job: The job
 */
static void job_release(struct b2r2_core_job *job)
{
	struct b2r2_blt_request *request =
		container_of(job, struct b2r2_blt_request, job);

	inc_stat(&stat_n_jobs_released);

	dev_dbg(b2r2_blt_device(), "%s, first_node=%p, ref_count=%d\n",
		 __func__, request->first_node, request->job.ref_count);

	b2r2_node_split_cancel(&request->node_split_job);

	/* Free nodes */
#ifdef B2R2_USE_NODE_GEN
	if (request->first_node)
		b2r2_blt_free_nodes(request->first_node);
#else
	if (request->first_node)
		b2r2_node_free(request->first_node);
#endif

	/* Release memory for the request */
	kfree(request);
}

/**
 * Tells the job to try to allocate the resources needed to execute the job.
 * Called just before execution of a job.
 *
 * @job: The job
 * @atomic: true if called from atomic (i.e. interrupt) context. If function
 *          can't allocate in atomic context it should return error, it
 *          will then be called later from non-atomic context.
 */
static int job_acquire_resources(struct b2r2_core_job *job, bool atomic)
{
	struct b2r2_blt_request *request =
		container_of(job, struct b2r2_blt_request, job);
	int ret;
	int i;

	dev_dbg(b2r2_blt_device(), "%s\n", __func__);

	/* We do not support atomic allocations (I think...) */
	if (atomic)
		return -EAGAIN;

	for (i = 0; i < request->buf_count; i++) {
		void *virt;

		dev_dbg(b2r2_blt_device(), "b2r2::%s: allocating %d bytes\n",
				__func__, request->bufs[i].size);

		virt = dma_alloc_coherent(b2r2_blt_device(),
				request->bufs[i].size,
				&request->bufs[i].phys_addr,
				GFP_DMA);
		if (virt == NULL) {
			ret = -ENOMEM;
			goto error;
		}

		request->bufs[i].virt_addr = virt;

		dev_dbg(b2r2_blt_device(), "b2r2::%s: phys=%p, virt=%p\n",
			__func__, (void *)request->bufs[i].phys_addr,
			request->bufs[i].virt_addr);

		ret = b2r2_node_split_assign_buffers(&request->node_split_job,
					request->first_node, request->bufs,
					request->buf_count);
		if (ret < 0)
			goto error;
	}

	return 0;

error:
	return ret;
}

/**
 * Tells the job to free the resources needed to execute the job.
 * Called after execution of a job.
 *
 * @job: The job
 * @atomic: true if called from atomic (i.e. interrupt) context. If function
 *          can't allocate in atomic context it should return error, it
 *          will then be called later from non-atomic context.
 */
static void job_release_resources(struct b2r2_core_job *job, bool atomic)
{
	struct b2r2_blt_request *request =
		container_of(job, struct b2r2_blt_request, job);

	dev_dbg(b2r2_blt_device(), "%s\n", __func__);

	/* Early release of nodes
	   FIXME: If nodes are to be reused we don't want to release here */
	if (!atomic && request->first_node) {
		int i;

#ifdef B2R2_USE_NODE_GEN
		b2r2_blt_free_nodes(request->first_node);
#else
		b2r2_node_free(request->first_node);
#endif
		request->first_node = NULL;

		/* Free any temporary buffers */
		for (i = 0; i < request->buf_count; i++) {

			dev_dbg(b2r2_blt_device(), "b2r2::%s: freeing %d bytes\n",
				__func__, request->bufs[i].size);
			dma_free_coherent(b2r2_blt_device(),
				  request->bufs[i].size,
				  request->bufs[i].virt_addr,
				  request->bufs[i].phys_addr);
			memset(&request->bufs[i], 0, sizeof(request->bufs[i]));
		}
		request->buf_count = 0;
	}
}

/**
 * unresolve_buf() - Must be called after resolve_buf
 *
 * @buf: The buffer specification as supplied from user space
 * @resolved: Gathered information about the buffer
 *
 * Returns 0 if OK else negative error code
 */
static void unresolve_buf(struct b2r2_blt_buf *buf,
			 struct b2r2_resolved_buf *resolved)
{
#ifdef CONFIG_ANDROID_PMEM
	if (resolved->is_pmem && resolved->filep)
		put_pmem_file(resolved->filep);
#endif

}

/**
 * resolve_buf() - Returns the physical & virtual addresses of a B2R2 blt buffer
 *
 * @buf: The buffer specification as supplied from user space
 * @resolved: Gathered information about the buffer
 *
 * Returns 0 if OK else negative error code
 */
static int resolve_buf(struct b2r2_blt_buf *buf,
		       struct b2r2_resolved_buf *resolved)
{
	int ret = 0;

	memset(resolved, 0, sizeof(*resolved));

	switch (buf->type) {
	case B2R2_BLT_PTR_NONE:
		break;

	case B2R2_BLT_PTR_PHYSICAL:
		resolved->physical_address = buf->offset;
		resolved->file_len = buf->len;
		break;

		/* FD + OFFSET type */
	case B2R2_BLT_PTR_FD_OFFSET: {
		/* TODO: Do we need to check if the process is allowed to read/write
		(depending on if it's dst or src) to the file? */
		struct file *file;
		int put_needed;
		int i;

#ifdef CONFIG_ANDROID_PMEM
		if (!get_pmem_file(
			    buf->fd,
			    (unsigned long *) &resolved->file_physical_start,
			    (unsigned long *) &resolved->file_virtual_start,
			    (unsigned long *) &resolved->file_len,
			    &resolved->filep)) {
			resolved->physical_address =
				resolved->file_physical_start +
				buf->offset;
			resolved->virtual_address = (void *)
				(resolved->file_virtual_start +
				 buf->offset);
			resolved->is_pmem = true;
		} else
#endif
		{

			file = fget_light(buf->fd, &put_needed);
			if (file == NULL)
				return -EINVAL;

			if (MAJOR(file->f_dentry->d_inode->i_rdev) == FB_MAJOR) {
				/* This is a frame buffer device, find fb_info
				   (OK to do it like this, no locking???) */

				for (i = 0; i < num_registered_fb; i++) {
					struct fb_info *info = registered_fb[i];

					if (info &&
					    info->dev && MINOR(info->dev->devt) ==
					    MINOR(file->f_dentry->d_inode->i_rdev)) {
						resolved->file_physical_start =
							info->fix.smem_start;
						resolved->file_virtual_start =
							(u32)info->screen_base;
						resolved->file_len =
							info->fix.smem_len;

						resolved->physical_address =
							resolved->file_physical_start +
							buf->offset;
						resolved->virtual_address =
							(void *)(resolved->file_virtual_start +
							buf->offset);

						break;
					}
				}
				if (i == num_registered_fb)
					ret = -EINVAL;
			} else
				ret = -EINVAL;
			fput_light(file, put_needed);
		}

		/* Check bounds */
		if (ret >= 0 && buf->offset + buf->len > resolved->file_len) {
			ret = -ESPIPE;
			unresolve_buf(buf, resolved);
		}

		break;
	}

	default:
		dev_err(b2r2_blt_device(),
			"%s: Failed to resolve buf type %d\n",
			__func__, buf->type);

		ret = -EINVAL;
		break;

	}

	return ret;
}

/**
 * is_mio_pmem - Checks if a buffer belongs to mio pmem
 *
 * @resolved_buf: Gathered info about the buffer
 *
 * Returns true if the buffer belongs to mio pmem, false otherwise
 */
static bool is_mio_pmem(struct b2r2_resolved_buf *buf)
{
	struct inode *inode;

	if (!buf->is_pmem || NULL == buf->filep)
		return false;

	inode = buf->filep->f_dentry->d_inode;
	/* It's a bit risky identifying mio pmem on the minor number given that pmem
	device's minor numbers depends on the order in which they where created. It
	seems to work however so no problem at this time. This type of information
	should be gotten from the pmem driver directly but that is not possible with
	the current pmem driver. */
	if (MISC_MAJOR == imajor(inode) && 1 == iminor(inode))
		return true;
	else
		return false;
}

/**
 * sync_buf - Synchronizes the memory occupied by an image buffer.
 *
 * @buf: User buffer specification
 * @resolved_buf: Gathered info (physical address etc.) about buffer
 * @is_dst: true if the buffer is a destination buffer, false if the buffer is a source buffer.
 */
static void sync_buf(struct b2r2_blt_buf *buf,
		      struct b2r2_resolved_buf *resolved,
		      bool is_dst)
{
	struct sync_args sa;
	u32 start_phys, end_phys;

	if (B2R2_BLT_PTR_NONE == buf->type)
		return;

	sa.start = (unsigned long)resolved->virtual_address;
	sa.end = (unsigned long)resolved->virtual_address + buf->len;
	start_phys = resolved->physical_address;
	end_phys = resolved->physical_address + buf->len;

	/* TODO: Very ugly. We should find out whether the memory is coherent in some generic way
	but cache handling will be rewritten soon so there is no use spending time on it. In the
	new design this will probably not be a problem. */
	/* Frame buffer and mio pmem memory is coherent, at least now. */
	if (!resolved->is_pmem || (!flush_mio_pmem && is_mio_pmem(resolved)))
	{
		/* Drain the write buffers as they are not always part of the coherent concept. */
		wmb();

		return;
	}

	/* The virtual address to a pmem buffer is retrieved from ioremap, not sure if it's
	ok to use such an address as a kernel virtual address. When doing it at a higher
	level such as dma_map_single it triggers an error but at lower levels such as
	dmac_clean_range it seems to work, hence the low level stuff. */

	if (is_dst) {
		/* According to ARM's docs you must clean before invalidating (ie flush) to
		avoid loosing data. */

		/* Flush L1 cache */
#ifdef CONFIG_SMP
		flush_l1_cache_range_all_cpus(&sa);
#else
		flush_l1_cache_range_curr_cpu(&sa);
#endif

		/* Flush L2 cache */
		outer_flush_range(start_phys, end_phys);
	} else {
		/* Clean L1 cache */
#ifdef CONFIG_SMP
		clean_l1_cache_range_all_cpus(&sa);
#else
		clean_l1_cache_range_curr_cpu(&sa);
#endif

		/* Clean L2 cache */
		outer_clean_range(start_phys, end_phys);
	}
}

/**
 * is_report_list_empty() - Spin lock protected check of report list
 *
 * @instance: The B2R2 BLT instance
 */
static bool is_report_list_empty(struct b2r2_blt_instance *instance)
{
	bool is_empty;

	spin_lock(&instance->lock);
	is_empty = list_empty(&instance->report_list);
	spin_unlock(&instance->lock);

	return is_empty;
}

/**
 * is_synching() - Spin lock protected check if synching
 *
 * @instance: The B2R2 BLT instance
 */
static bool is_synching(struct b2r2_blt_instance *instance)
{
	bool is_synching;

	spin_lock(&instance->lock);
	is_synching = instance->synching;
	spin_unlock(&instance->lock);

	return is_synching;
}

/**
 * b2r2_blt_devide() - Returns the B2R2 blt device for logging
 */
struct device *b2r2_blt_device(void)
{
	return b2r2_blt_dev ? b2r2_blt_dev->this_device : NULL;
}

/**
 * inc_stat() - Spin lock protected increment of statistics variable
 *
 * @stat: Pointer to statistics variable that should be incremented
 */
static void inc_stat(unsigned long *stat)
{
	spin_lock(&stat_lock);
	(*stat)++;
	spin_unlock(&stat_lock);
}

/**
 * inc_stat() - Spin lock protected decrement of statistics variable
 *
 * @stat: Pointer to statistics variable that should be decremented
 */
static void dec_stat(unsigned long *stat)
{
	spin_lock(&stat_lock);
	(*stat)--;
	spin_unlock(&stat_lock);
}


#ifdef CONFIG_DEBUG_FS
/**
 * sprintf_req() - Builds a string representing the request, for debug
 *
 * @request:Request that should be encoded into a string
 * @buf: Receiving buffer
 * @size: Size of receiving buffer
 *
 * Returns number of characters in string, excluding null terminator
 */
static int sprintf_req(struct b2r2_blt_request *request, char *buf, int size)
{
	size_t dev_size = 0;

	dev_size += sprintf(buf + dev_size,
			    "instance: %p\n\n",
			    request->instance);

	dev_size += sprintf(buf + dev_size,
			    "size: %d bytes\n",
			    request->user_req.size);
	dev_size += sprintf(buf + dev_size,
			    "flags: %8lX\n",
			    (unsigned long) request->user_req.flags);
	dev_size += sprintf(buf + dev_size,
			    "transform: %3lX\n",
			    (unsigned long) request->user_req.transform);
	dev_size += sprintf(buf + dev_size,
			    "prio: %d\n",
			    request->user_req.transform);
	dev_size += sprintf(buf + dev_size,
			    "src_img.fmt: %d\n",
			    request->user_req.src_img.fmt);
	dev_size += sprintf(buf + dev_size,
			    "src_img.buf: {type=%d,fd=%d,offset=%d,len=%d}\n",
			    request->user_req.src_img.buf.type,
			    request->user_req.src_img.buf.fd,
			    request->user_req.src_img.buf.offset,
			    request->user_req.src_img.buf.len);
	dev_size += sprintf(buf + dev_size,
			    "src_img.{width=%d,height=%d,pitch=%d}\n",
			    request->user_req.src_img.width,
			    request->user_req.src_img.height,
			    request->user_req.src_img.pitch);
	dev_size += sprintf(buf + dev_size,
			    "src_mask.fmt: %d\n",
			    request->user_req.src_mask.fmt);
	dev_size += sprintf(buf + dev_size,
			    "src_mask.buf: {type=%d,fd=%d,offset=%d,len=%d}\n",
			    request->user_req.src_mask.buf.type,
			    request->user_req.src_mask.buf.fd,
			    request->user_req.src_mask.buf.offset,
			    request->user_req.src_mask.buf.len);
	dev_size += sprintf(buf + dev_size,
			    "src_mask.{width=%d,height=%d,pitch=%d}\n",
			    request->user_req.src_mask.width,
			    request->user_req.src_mask.height,
			    request->user_req.src_mask.pitch);
	dev_size += sprintf(buf + dev_size,
			    "src_rect.{x=%d,y=%d,width=%d,height=%d}\n",
			    request->user_req.src_rect.x,
			    request->user_req.src_rect.y,
			    request->user_req.src_rect.width,
			    request->user_req.src_rect.height);
	dev_size += sprintf(buf + dev_size,
			    "src_color=%08lX\n",
			    (unsigned long) request->user_req.src_color);

	dev_size += sprintf(buf + dev_size,
			    "dst_img.fmt: %d\n",
			    request->user_req.dst_img.fmt);
	dev_size += sprintf(buf + dev_size,
			    "dst_img.buf: {type=%d,fd=%d,offset=%d,len=%d}\n",
			    request->user_req.dst_img.buf.type,
			    request->user_req.dst_img.buf.fd,
			    request->user_req.dst_img.buf.offset,
			    request->user_req.dst_img.buf.len);
	dev_size += sprintf(buf + dev_size,
			    "dst_img.{width=%d,height=%d,pitch=%d}\n",
			    request->user_req.dst_img.width,
			    request->user_req.dst_img.height,
			    request->user_req.dst_img.pitch);
	dev_size += sprintf(buf + dev_size,
			    "dst_rect.{x=%d,y=%d,width=%d,height=%d}\n",
			    request->user_req.dst_rect.x,
			    request->user_req.dst_rect.y,
			    request->user_req.dst_rect.width,
			    request->user_req.dst_rect.height);
	dev_size += sprintf(buf + dev_size,
			    "dst_clip_rect.{x=%d,y=%d,width=%d,height=%d}\n",
			    request->user_req.dst_clip_rect.x,
			    request->user_req.dst_clip_rect.y,
			    request->user_req.dst_clip_rect.width,
			    request->user_req.dst_clip_rect.height);
	dev_size += sprintf(buf + dev_size,
			    "dst_color=%08lX\n",
			    (unsigned long) request->user_req.dst_color);
	dev_size += sprintf(buf + dev_size,
			    "global_alpha=%d\n",
			    (int) request->user_req.global_alpha);
	dev_size += sprintf(buf + dev_size,
			    "report1=%08lX\n",
			    (unsigned long) request->user_req.report1);
	dev_size += sprintf(buf + dev_size,
			    "report2=%08lX\n",
			    (unsigned long) request->user_req.report2);

	dev_size += sprintf(buf + dev_size,
			    "request_id: %d\n",
			    request->request_id);

	dev_size += sprintf(buf + dev_size,
			    "src_resolved.physical: %lX\n",
			    (unsigned long) request->src_resolved.
			    physical_address);
	dev_size += sprintf(buf + dev_size,
			    "src_resolved.virtual: %p\n",
			    request->src_resolved.virtual_address);
	dev_size += sprintf(buf + dev_size,
			    "src_resolved.filep: %p\n",
			    request->src_resolved.filep);
	dev_size += sprintf(buf + dev_size,
			    "src_resolved.filep_physical_start: %lX\n",
			    (unsigned long) request->src_resolved.
			    file_physical_start);
	dev_size += sprintf(buf + dev_size,
			    "src_resolved.filep_virtual_start: %p\n",
			    (void *) request->src_resolved.file_virtual_start);
	dev_size += sprintf(buf + dev_size,
			    "src_resolved.file_len: %d\n",
			    request->src_resolved.file_len);

	dev_size += sprintf(buf + dev_size,
			    "src_mask_resolved.physical: %lX\n",
			    (unsigned long) request->src_mask_resolved.
			    physical_address);
	dev_size += sprintf(buf + dev_size,
			    "src_mask_resolved.virtual: %p\n",
			    request->src_mask_resolved.virtual_address);
	dev_size += sprintf(buf + dev_size,
			    "src_mask_resolved.filep: %p\n",
			    request->src_mask_resolved.filep);
	dev_size += sprintf(buf + dev_size,
			    "src_mask_resolved.filep_physical_start: %lX\n",
			    (unsigned long) request->src_mask_resolved.
			    file_physical_start);
	dev_size += sprintf(buf + dev_size,
			    "src_mask_resolved.filep_virtual_start: %p\n",
			    (void *) request->src_mask_resolved.
			    file_virtual_start);
	dev_size += sprintf(buf + dev_size,
			    "src_mask_resolved.file_len: %d\n",
			    request->src_mask_resolved.file_len);

	dev_size += sprintf(buf + dev_size,
			    "dst_resolved.physical: %lX\n",
			    (unsigned long) request->dst_resolved.
			    physical_address);
	dev_size += sprintf(buf + dev_size,
			    "dst_resolved.virtual: %p\n",
			    request->dst_resolved.virtual_address);
	dev_size += sprintf(buf + dev_size,
			    "dst_resolved.filep: %p\n",
			    request->dst_resolved.filep);
	dev_size += sprintf(buf + dev_size,
			    "dst_resolved.filep_physical_start: %lX\n",
			    (unsigned long) request->dst_resolved.
			    file_physical_start);
	dev_size += sprintf(buf + dev_size,
			    "dst_resolved.filep_virtual_start: %p\n",
			    (void *) request->dst_resolved.file_virtual_start);
	dev_size += sprintf(buf + dev_size,
			    "dst_resolved.file_len: %d\n",
			    request->dst_resolved.file_len);

	return dev_size;
}

/**
 * debugfs_b2r2_blt_request_read() - Implements debugfs read for B2R2 register
 *
 * @filp: File pointer
 * @buf: User space buffer
 * @count: Number of bytes to read
 * @f_pos: File position
 *
 * Returns number of bytes read or negative error code
 */
static int debugfs_b2r2_blt_request_read(struct file *filp, char __user *buf,
			       size_t count, loff_t *f_pos)
{
	size_t dev_size = 0;
	int ret = 0;
	char *Buf = kmalloc(sizeof(char) * 4096, GFP_KERNEL);

	if (Buf == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	dev_size = sprintf_req(&debugfs_latest_request, Buf, sizeof(char) * 4096);

	/* No more to read if offset != 0 */
	if (*f_pos > dev_size)
		goto out;

	if (*f_pos + count > dev_size)
		count = dev_size - *f_pos;

	if (copy_to_user(buf, Buf, count))
		ret = -EINVAL;
	*f_pos += count;
	ret = count;

out:
	if (Buf != NULL)
		kfree(Buf);
	return ret;
}

/**
 * debugfs_b2r2_blt_request_fops - File operations for B2R2 request debugfs
 */
static const struct file_operations debugfs_b2r2_blt_request_fops = {
	.owner = THIS_MODULE,
	.read  = debugfs_b2r2_blt_request_read,
};

/**
 * struct debugfs_reg - Represents a B2R2 node "register"
 *
 * @name: Register name
 * @offset: Offset within the node
 */
struct debugfs_reg {
	const char  name[30];
	u32   offset;
};

/**
 * debugfs_node_regs - Array with all the registers in a B2R2 node, for debug
 */
static const struct debugfs_reg debugfs_node_regs[] = {
	{"GROUP0.B2R2_NIP", offsetof(struct b2r2_link_list, GROUP0.B2R2_NIP)},
	{"GROUP0.B2R2_CIC", offsetof(struct b2r2_link_list, GROUP0.B2R2_CIC)},
	{"GROUP0.B2R2_INS", offsetof(struct b2r2_link_list, GROUP0.B2R2_INS)},
	{"GROUP0.B2R2_ACK", offsetof(struct b2r2_link_list, GROUP0.B2R2_ACK)},

	{"GROUP1.B2R2_TBA", offsetof(struct b2r2_link_list, GROUP1.B2R2_TBA)},
	{"GROUP1.B2R2_TTY", offsetof(struct b2r2_link_list, GROUP1.B2R2_TTY)},
	{"GROUP1.B2R2_TXY", offsetof(struct b2r2_link_list, GROUP1.B2R2_TXY)},
	{"GROUP1.B2R2_TSZ", offsetof(struct b2r2_link_list, GROUP1.B2R2_TSZ)},

	{"GROUP2.B2R2_S1CF", offsetof(struct b2r2_link_list, GROUP2.B2R2_S1CF)},
	{"GROUP2.B2R2_S2CF", offsetof(struct b2r2_link_list, GROUP2.B2R2_S2CF)},

	{"GROUP3.B2R2_SBA", offsetof(struct b2r2_link_list, GROUP3.B2R2_SBA)},
	{"GROUP3.B2R2_STY", offsetof(struct b2r2_link_list, GROUP3.B2R2_STY)},
	{"GROUP3.B2R2_SXY", offsetof(struct b2r2_link_list, GROUP3.B2R2_SXY)},
	{"GROUP3.B2R2_SSZ", offsetof(struct b2r2_link_list, GROUP3.B2R2_SSZ)},

	{"GROUP4.B2R2_SBA", offsetof(struct b2r2_link_list, GROUP4.B2R2_SBA)},
	{"GROUP4.B2R2_STY", offsetof(struct b2r2_link_list, GROUP4.B2R2_STY)},
	{"GROUP4.B2R2_SXY", offsetof(struct b2r2_link_list, GROUP4.B2R2_SXY)},
	{"GROUP4.B2R2_SSZ", offsetof(struct b2r2_link_list, GROUP4.B2R2_SSZ)},

	{"GROUP5.B2R2_SBA", offsetof(struct b2r2_link_list, GROUP5.B2R2_SBA)},
	{"GROUP5.B2R2_STY", offsetof(struct b2r2_link_list, GROUP5.B2R2_STY)},
	{"GROUP5.B2R2_SXY", offsetof(struct b2r2_link_list, GROUP5.B2R2_SXY)},
	{"GROUP5.B2R2_SSZ", offsetof(struct b2r2_link_list, GROUP5.B2R2_SSZ)},

	{"GROUP6.B2R2_CWO", offsetof(struct b2r2_link_list, GROUP6.B2R2_CWO)},
	{"GROUP6.B2R2_CWS", offsetof(struct b2r2_link_list, GROUP6.B2R2_CWS)},

	{"GROUP7.B2R2_CCO", offsetof(struct b2r2_link_list, GROUP7.B2R2_CCO)},
	{"GROUP7.B2R2_CML", offsetof(struct b2r2_link_list, GROUP7.B2R2_CML)},

	{"GROUP8.B2R2_FCTL", offsetof(struct b2r2_link_list, GROUP8.B2R2_FCTL)},
	{"GROUP8.B2R2_PMK", offsetof(struct b2r2_link_list, GROUP8.B2R2_PMK)},

	{"GROUP9.B2R2_RSF", offsetof(struct b2r2_link_list, GROUP9.B2R2_RSF)},
	{"GROUP9.B2R2_RZI", offsetof(struct b2r2_link_list, GROUP9.B2R2_RZI)},
	{"GROUP9.B2R2_HFP", offsetof(struct b2r2_link_list, GROUP9.B2R2_HFP)},
	{"GROUP9.B2R2_VFP", offsetof(struct b2r2_link_list, GROUP9.B2R2_VFP)},

	{"GROUP10.B2R2_RSF", offsetof(struct b2r2_link_list, GROUP10.B2R2_RSF)},
	{"GROUP10.B2R2_RZI", offsetof(struct b2r2_link_list, GROUP10.B2R2_RZI)},
	{"GROUP10.B2R2_HFP", offsetof(struct b2r2_link_list, GROUP10.B2R2_HFP)},
	{"GROUP10.B2R2_VFP", offsetof(struct b2r2_link_list, GROUP10.B2R2_VFP)},

	{"GROUP11.B2R2_Coeff0", offsetof(struct b2r2_link_list,
					 GROUP11.B2R2_Coeff0)},
	{"GROUP11.B2R2_Coeff1", offsetof(struct b2r2_link_list,
					 GROUP11.B2R2_Coeff1)},
	{"GROUP11.B2R2_Coeff2", offsetof(struct b2r2_link_list,
					 GROUP11.B2R2_Coeff2)},
	{"GROUP11.B2R2_Coeff3", offsetof(struct b2r2_link_list,
					 GROUP11.B2R2_Coeff3)},

	{"GROUP12.B2R2_KEY1", offsetof(struct b2r2_link_list,
				       GROUP12.B2R2_KEY1)},
	{"GROUP12.B2R2_KEY2", offsetof(struct b2r2_link_list,
				       GROUP12.B2R2_KEY2)},

	{"GROUP13.B2R2_XYL", offsetof(struct b2r2_link_list, GROUP13.B2R2_XYL)},
	{"GROUP13.B2R2_XYP", offsetof(struct b2r2_link_list, GROUP13.B2R2_XYP)},

	{"GROUP14.B2R2_SAR", offsetof(struct b2r2_link_list, GROUP14.B2R2_SAR)},
	{"GROUP14.B2R2_USR", offsetof(struct b2r2_link_list, GROUP14.B2R2_USR)},

	{"GROUP15.B2R2_VMX0", offsetof(struct b2r2_link_list,
				       GROUP15.B2R2_VMX0)},
	{"GROUP15.B2R2_VMX1", offsetof(struct b2r2_link_list,
				       GROUP15.B2R2_VMX1)},
	{"GROUP15.B2R2_VMX2", offsetof(struct b2r2_link_list,
				       GROUP15.B2R2_VMX2)},
	{"GROUP15.B2R2_VMX3", offsetof(struct b2r2_link_list,
				       GROUP15.B2R2_VMX3)},

	{"GROUP16.B2R2_VMX0", offsetof(struct b2r2_link_list,
				       GROUP16.B2R2_VMX0)},
	{"GROUP16.B2R2_VMX1", offsetof(struct b2r2_link_list,
				       GROUP16.B2R2_VMX1)},
	{"GROUP16.B2R2_VMX2", offsetof(struct b2r2_link_list,
				       GROUP16.B2R2_VMX2)},
	{"GROUP16.B2R2_VMX3", offsetof(struct b2r2_link_list,
				       GROUP16.B2R2_VMX3)},
};

/**
 * sprintf_node() - Encodes a B2R2 node into a string
 *
 * @node: The node to convert to a string
 * @buf: Receiving buffer
 * @size: Size of receiving buffer
 */
static int sprintf_node(struct b2r2_link_list *node, char *buf, int size)
{
	size_t dev_size = 0;
	int i;

	/* Handle null node */
	if (!node) {
		dev_size = sprintf(buf + dev_size,
				   "{null}\n");
		return dev_size;
	}

	/* Loop over all registers */
	for (i = 0; i < ARRAY_SIZE(debugfs_node_regs); i++) {
		unsigned long value =
			*((unsigned long *) (((u8 *) node) +
					     debugfs_node_regs[i].offset));
		dev_size += sprintf(buf + dev_size, "%s: %08lX\n",
				    debugfs_node_regs[i].name,
				    value);
	}

	return dev_size;
}

/**
 * debugfs_b2r2_blt_first_node_read() - Implements debugfs read for
 *                                      B2R2 BLT request first node
 *
 * @filp: File pointer
 * @buf: User space buffer
 * @count: Number of bytes to read
 * @f_pos: File position
 *
 * Returns number of bytes read or negative error code
 */
static int debugfs_b2r2_blt_first_node_read(struct file *filp, char __user *buf,
			       size_t count, loff_t *f_pos)
{
	size_t dev_size = 0;
	int ret = 0;
	char *Buf = kmalloc(sizeof(char) * 4096, GFP_KERNEL);

	if (Buf == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	dev_size = sprintf_node(&debugfs_latest_first_node.node,
				Buf, sizeof(char) * 4096);

	/* No more to read if offset != 0 */
	if (*f_pos > dev_size)
		goto out;

	if (*f_pos + count > dev_size)
		count = dev_size - *f_pos;

	if (copy_to_user(buf, Buf, count))
		ret = -EINVAL;
	*f_pos += count;
	ret = count;

out:
	if (Buf != NULL)
		kfree(Buf);
	return ret;
}

/**
 * debugfs_b2r2_blt_first_node_fops() - File operations for first node
 */
static const struct file_operations debugfs_b2r2_blt_first_node_fops = {
	.owner = THIS_MODULE,
	.read  = debugfs_b2r2_blt_first_node_read,
};


/**
 * debugfs_b2r2_blt_stat_read() - Implements debugfs read for B2R2 BLT
 *                                statistics
 *
 * @filp: File pointer
 * @buf: User space buffer
 * @count: Number of bytes to read
 * @f_pos: File position
 *
 * Returns number of bytes read or negative error code
 */
static int debugfs_b2r2_blt_stat_read(struct file *filp, char __user *buf,
				  size_t count, loff_t *f_pos)
{
	size_t dev_size = 0;
	int ret = 0;
	char *Buf = kmalloc(sizeof(char) * 4096, GFP_KERNEL);

	if (Buf == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	spin_lock(&stat_lock);
	dev_size += sprintf(Buf + dev_size, "Added jobs: %lu\n",
			    stat_n_jobs_added);
	dev_size += sprintf(Buf + dev_size, "Released jobs: %lu\n",
			    stat_n_jobs_released);
	dev_size += sprintf(Buf + dev_size, "Jobs in report list: %lu\n",
			    stat_n_jobs_in_report_list);
	dev_size += sprintf(Buf + dev_size, "Clients in open: %lu\n",
			    stat_n_in_open);
	dev_size += sprintf(Buf + dev_size, "Clients in release: %lu\n",
			    stat_n_in_release);
	dev_size += sprintf(Buf + dev_size, "Clients in blt: %lu\n",
			    stat_n_in_blt);
	dev_size += sprintf(Buf + dev_size, "              synch: %lu\n",
			    stat_n_in_blt_synch);
	dev_size += sprintf(Buf + dev_size, "              add: %lu\n",
			    stat_n_in_blt_add);
	dev_size += sprintf(Buf + dev_size, "              wait: %lu\n",
			    stat_n_in_blt_wait);
	dev_size += sprintf(Buf + dev_size, "Clients in synch 0: %lu\n",
			    stat_n_in_synch_0);
	dev_size += sprintf(Buf + dev_size, "Clients in synch job: %lu\n",
			    stat_n_in_synch_job);
	dev_size += sprintf(Buf + dev_size, "Clients in query_cap: %lu\n",
			    stat_n_in_query_cap);
	spin_unlock(&stat_lock);

	/* No more to read if offset != 0 */
	if (*f_pos > dev_size)
		goto out;

	if (*f_pos + count > dev_size)
		count = dev_size - *f_pos;

	if (copy_to_user(buf, Buf, count))
		ret = -EINVAL;
	*f_pos += count;
	ret = count;

out:
	if (Buf != NULL)
		kfree(Buf);
	return ret;
}

/**
 * debugfs_b2r2_blt_stat_fops() - File operations for B2R2 BLT
 *                                statistics debugfs
 */
static const struct file_operations debugfs_b2r2_blt_stat_fops = {
	.owner = THIS_MODULE,
	.read  = debugfs_b2r2_blt_stat_read,
};
#endif

/**
 * b2r2_blt_module_init() - Module init function
 *
 * Returns 0 if OK else negative error code
 */
int b2r2_blt_module_init(void)
{
	int ret;

	spin_lock_init(&stat_lock);

	/* Initialize node splitter */
	ret = b2r2_node_split_init();
	if (ret) {
		printk(KERN_WARNING "%s: node split init fails\n",
			__func__);
		goto b2r2_node_split_init_fail;
	}

	/** Register b2r2 driver */
	ret = misc_register(&b2r2_blt_misc_dev);
	if (ret) {
		printk(KERN_WARNING "%s: registering misc device fails\n",
			__func__);
		goto b2r2_misc_register_fail;
	}

	b2r2_blt_misc_dev.this_device->coherent_dma_mask = 0xFFFFFFFF;
	b2r2_blt_dev = &b2r2_blt_misc_dev;
	dev_dbg(b2r2_blt_device(), "%s\n", __func__);

	/* Initialize memory allocator */
	/* FIXME: Should be before misc_register,
	   someone might issue requests on /dev/b2r2_blt */
	ret = b2r2_mem_init(b2r2_blt_device(), B2R2_HEAP_SIZE,
			    4, sizeof(struct b2r2_node));
	if (ret) {
		printk(KERN_WARNING "%s: initializing B2R2 memhandler fails\n",
			__func__);
		goto b2r2_mem_init_fail;
	}

#ifdef CONFIG_DEBUG_FS
	/* Register debug fs */
	if (!debugfs_root_dir) {
		debugfs_root_dir = debugfs_create_dir("b2r2_blt", NULL);
		debugfs_create_file("latest_request",
				    0666, debugfs_root_dir,
				    0,
				    &debugfs_b2r2_blt_request_fops);
		debugfs_create_file("first_node",
				    0666, debugfs_root_dir,
				    0,
				    &debugfs_b2r2_blt_first_node_fops);
		debugfs_create_file("stat",
				    0666, debugfs_root_dir,
				    0,
				    &debugfs_b2r2_blt_stat_fops);
		debugfs_create_bool("flush_mio_pmem",
				    0666, debugfs_root_dir,
				    (u32 *) &flush_mio_pmem);

		debugfs_create_u32("b2r2_heap_size",
				    0666, debugfs_root_dir,
				    &b2r2_heap_size);
	}
#endif
	goto out;

b2r2_misc_register_fail:
b2r2_mem_init_fail:
	b2r2_node_split_exit();
b2r2_node_split_init_fail:
out:
	return ret;
}

/**
 * b2r2_module_exit() - Module exit function
 */
void b2r2_blt_module_exit(void)
{
#ifdef CONFIG_DEBUG_FS
	if (debugfs_root_dir) {
		debugfs_remove_recursive(debugfs_root_dir);
		debugfs_root_dir = NULL;
	}
#endif
	if (b2r2_blt_dev) {
		dev_dbg(b2r2_blt_device(), "%s\n", __func__);
		b2r2_mem_exit();
		b2r2_blt_dev = NULL;
		misc_deregister(&b2r2_blt_misc_dev);
	}

	b2r2_node_split_exit();
}

/* module_init(b2r2_blt_module_init); */
/* module_exit(b2r2_blt_module_exit); */

MODULE_AUTHOR("Paul Wannback <paul.wannback@stericsson.com>");
MODULE_DESCRIPTION("ST-Ericsson B2R2 Blitter module");
MODULE_LICENSE("GPL");
