// SPDX-License-Identifier: GPL-2.0-only
/*
 * This module emits "Hello, world" on printk when loaded.
 *
 * It is designed to be used for basic evaluation of the module loading
 * subsystem (for example when validating module signing/verification). It
 * lacks any extra dependencies, and will not normally be loaded by the
 * system unless explicitly requested by name.
 */

#include <asm-generic/errno-base.h>
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <stddef.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/seq_file.h>
#include <linux/workqueue.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include "mp1_given.h"
#define TIMEOUT 5000
#define FOLDER_NAME "mp1"
#define FILE_NAME "status"
// !!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!
// Please put your name and email here
MODULE_AUTHOR("Vihaan Rao <vihaanr2@illinois.edu>");
MODULE_LICENSE("GPL");

struct proc_dir_entry *proc_dir = NULL;

static DEFINE_MUTEX(lock);

LIST_HEAD(my_proc_list);

static struct workqueue_struct *mp1_wq; // Workqueue
static struct work_struct mp1_work; // Work item
struct timer_list my_timer;

/* struct to define our process entry */
typedef struct proc_t {
	int pid;
	struct list_head list;
	unsigned long u_time;

} proc_t;

/* read from /proc/mp1/status */
static ssize_t proc_read_callb(struct file *file, char __user *user_buff,
			       size_t count, loff_t *offset)
{
	struct proc_t *proc_entry;
	struct list_head *ptr;

	int len = 0;
	int to_write = 0;
	// char kbuffer[4096];
	char *kbuffer = kmalloc(4096, GFP_KERNEL);
	if (!kbuffer) {
		return -ENOMEM;
	}

	mutex_lock(&lock);
	list_for_each (ptr, &my_proc_list) {
		proc_entry = list_entry(ptr, struct proc_t, list);
		to_write = snprintf(kbuffer + len, 4096 - len, "%d: %zu\n",
				    proc_entry->pid, proc_entry->u_time);
		if (len + to_write >=
		    4096) { // don't write if exceeds buff size
			printk(KERN_ERR "error: input exceeds buffer size.");
			break;
		}
		len += to_write;
	}
	mutex_unlock(&lock);

	if (*offset >= len) { // reached EOF
		kfree(kbuffer);
		return 0;
	}

	if (count > (len - *offset)) {
		count = len - *offset;
	}

	if (copy_to_user(user_buff, kbuffer + *offset, count)) {
		printk(KERN_ERR
		       "error: unable to copy kernel buffer to user");
		kfree(kbuffer);
		return -EFAULT;
	}

	*offset += count;
	return count;
}
static ssize_t proc_write_callb(struct file *file, const char __user *user_buff,
				size_t len, loff_t *offset)
{
	int pid = 0;
	int not_copied_bytes = 0;
	struct proc_t *proc_entry;

	char *kbuffer = kmalloc(len + 1, GFP_KERNEL); // +1 for \n byte
	if (!kbuffer) {
		printk(KERN_ERR
		       "unable to allocate memory for kernel buffer");
		return -ENOMEM;
	}

	proc_entry = kmalloc(sizeof(proc_t), GFP_KERNEL);
	if (!proc_entry) {
		printk(KERN_ERR
		       "unable to allocate memory for proc_entry struct");
		return -ENOMEM;
	}

	/* copy pid data from usr space */
	not_copied_bytes = copy_from_user(kbuffer, user_buff, len);
	if (not_copied_bytes > 0) {
		printk(KERN_ERR
		       "error: copy_from_user failed to cpy %d bytes",
		       not_copied_bytes);
		kfree(kbuffer);
		kfree(proc_entry);
		return -EFAULT;
	}

	kbuffer[len] = '\0';

	/* parse string in kbuff to int */
	if (kstrtoint(kbuffer, 10, &pid) != 0) {
		printk(KERN_ERR
		       "error in decimal to string conversion for pid");
		kfree(kbuffer);
		kfree(proc_entry);
		return -EINVAL;
	}

	kfree(kbuffer);

	proc_entry->pid = pid;
	proc_entry->u_time = 0;
	mutex_lock(&lock);
	list_add_tail(&proc_entry->list, &my_proc_list);
	mutex_unlock(&lock);

	return len;
}

/* procfs file ops for entry */
static const struct proc_ops fops = {
	// .proc_open = open_callback,
	.proc_read = proc_read_callb,
	.proc_write = proc_write_callb
};

/* create 'mp1' dir and 'status' file */
static int make_proc_entry(void)
{
	struct proc_dir_entry *entry = NULL;
	proc_dir = proc_mkdir(FOLDER_NAME, NULL);
	if (proc_dir == NULL) {
		printk(KERN_ERR "unable to create directory 'mp1'");
		return -ENOMEM;
	}

	entry = proc_create("status", 0666, proc_dir, &fops);
	if (entry == NULL) {
		printk("unable to create entry 'status'");
		remove_proc_entry(FOLDER_NAME, NULL);
		return -ENOMEM;
	}

	printk("successfully created /proc/%s/%s", FOLDER_NAME, FILE_NAME);
	return 0;
}

/*  on timer expiration jump here to:
 * 1. schedule workqueue task 
 * 2. reset timer 
 * */
static void timer_callb(struct timer_list *timer)
{
	queue_work(mp1_wq, &mp1_work); // sched workqueue task
	mod_timer(&my_timer,
		  jiffies + msecs_to_jiffies(TIMEOUT)); // restart timer
}

/* upon timer expiration queue the work_handler to the workqueue.*/
static void work_handler(struct work_struct *work)
{
	// printk(KERN_INFO "DEBUG: timer expired. work func() exec'd\n");
	struct list_head *curr, *next;
	struct proc_t *entry;
	unsigned long cpu_time;

	/* iterate through the kernel LL to get_cpu_use and update each proc_t entry */
	mutex_lock(&lock);
	list_for_each_safe (curr, next, &my_proc_list) {
		entry = list_entry(curr, struct proc_t, list);
		if (get_cpu_use(entry->pid, &cpu_time) == 0) {
			entry->u_time = cpu_time;
		} else {
			list_del(curr);
			kfree(entry);
		}
	}
	mutex_unlock(&lock);
}

static int __init test_module_init(void)
{
	make_proc_entry();

	mp1_wq = alloc_workqueue("mp1_wq", WQ_UNBOUND, 0);

	INIT_WORK(&mp1_work, work_handler);

	/* setup to call timer-callback */
	timer_setup(&my_timer, timer_callb, 0);

	/* setup interval to 5 secs (5000ms) */
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(TIMEOUT));

	return 0;
}

module_init(test_module_init);

/* clean up on rmmod */
static void __exit test_module_exit(void)
{
	struct list_head *p, *n;
	struct proc_t *entry;

	remove_proc_entry(FILE_NAME, proc_dir); // remove 'status' file
	remove_proc_entry(FOLDER_NAME, NULL); // remove 'mp1' dir

	del_timer_sync(&my_timer);

	flush_workqueue(mp1_wq);
	destroy_workqueue(mp1_wq);

	/* delete LL */
	mutex_lock(&lock);
	list_for_each_safe (p, n, &my_proc_list) {
		entry = list_entry(p, struct proc_t, list);
		list_del(p);
		kfree(entry);
	}
	mutex_unlock(&lock);
}

module_exit(test_module_exit);
