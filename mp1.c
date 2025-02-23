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
// #include <stdlib.h>
// #include <stdio.h>
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <stddef.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#define FOLDER_NAME "mp1"
#define FILE_NAME "status"
// !!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!
// Please put your name and email here
MODULE_AUTHOR("Vihaan Rao <vihaanr2@illinois.edu>");
MODULE_LICENSE("GPL");

struct proc_dir_entry *proc_dir = NULL;

static DEFINE_MUTEX(lock);
LIST_HEAD(my_proc_list);

typedef struct proc_t {
	int pid;
	struct list_head list;
	unsigned long u_time;

} proc_t;

// temp proc printf callback
static int myproc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "Hello from procfs directory!\n");
	return 0;
}

// temp proc open callback
// static int open_callback(struct inode *inode, struct file *file)
// {
// 	return single_open(file, myproc_show, NULL);
// }

static ssize_t proc_read_callb(struct file *file, char __user *user_buff,
			       size_t count, loff_t *offset)
{
	struct list_head *ptr;
	int len = 0;
	int to_write = 0;
	// char kbuffer[4096];
	char *kbuffer = kmalloc(4096, GFP_KERNEL);
	if (!kbuffer) {
		return -ENOMEM;
	}
	struct proc_t *proc_entry;

	mutex_lock(&lock);
	list_for_each (ptr, &my_proc_list) {
		proc_entry = list_entry(ptr, struct proc_t, list);
		to_write = snprintf(kbuffer + len, 4096 - len, "PID: %d\n",
				    proc_entry->pid);
		if (len + to_write >=
		    4096) { // don't write if exceeds buff size
			break;
		}
		len += to_write;
	}
	mutex_unlock(&lock);

	if (*offset >= len) {
		kfree(kbuffer);
		return 0;
	}

	if (count > (len - *offset)) {
		count = len - *offset;
	}

	if (copy_to_user(user_buff, kbuffer + *offset, count)) {
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

	/* alloc mem for proc_t struct to hold process entry */
	struct proc_t *proc_entry = kmalloc(sizeof(proc_t), GFP_KERNEL);
	if (proc_entry == NULL) {
		printk("unable to allocate memory for proc_entry struct");
		return -ENOMEM;
	}

	/* allocate memory for temp buffer to hold the pid */
	char *kbuffer = kmalloc(len + 1, GFP_KERNEL); // +1 for \n byte
	if (kbuffer == NULL) {
		printk("unable to allocate memory for kernel buffer");
		kfree(proc_entry);
		return -ENOMEM;
	}

	/* copy pid data from usr space */
	not_copied_bytes = copy_from_user(kbuffer, user_buff, len);
	if (not_copied_bytes > 0) {
		printk("copy_from_user failed to cpy %d bytes",
		       not_copied_bytes);
		kfree(kbuffer);
		kfree(proc_entry);
		return -EFAULT;
	}

	kbuffer[len] = '\0';

	/* parse string in kbuff to int */
	if (kstrtoint(kbuffer, 10, &pid) != 0) {
		printk("error in decimal to string conversion for pid");
		kfree(kbuffer);
		kfree(proc_entry);
		return -EINVAL;
	}

	kfree(kbuffer); // done w/ temp kbuffer

	proc_entry->pid = pid;
	proc_entry->u_time = 0;
	mutex_lock(&lock);
	list_add_tail(&proc_entry->list, &my_proc_list);
	mutex_unlock(&lock);

	// snprintf("DEBUG: The value of the PID is: %d", pid);
	printk("proc_write_callb: successfully added PID %d\n", pid);
	return len;
}
// procfs file ops for entry
static const struct proc_ops fops = {
	// .proc_open = open_callback,
	.proc_read = proc_read_callb, // TODO: implement
	.proc_write = proc_write_callb
};

static int make_proc_entry(void)
{
	struct proc_dir_entry *entry = NULL;
	proc_dir = proc_mkdir(FOLDER_NAME, NULL);
	if (proc_dir == NULL) {
		printk("unable to create directory 'mp1'");
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

static int __init test_module_init(void)
{
	make_proc_entry();
	pr_warn("Hello, world\n");

	return 0;
}

module_init(test_module_init);

static void __exit test_module_exit(void)
{
	remove_proc_entry(FILE_NAME, proc_dir); // remove 'status' file
	remove_proc_entry(FOLDER_NAME, NULL); // remove 'mp1' dir
	pr_warn("Goodbye\n");
}

module_exit(test_module_exit);
