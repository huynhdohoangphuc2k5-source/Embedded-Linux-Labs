#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/version.h>

#define DRIVER_NAME "counter_driver"
#define DEV_NAME    "counter"
#define PROC_NAME   "counter_info"

static atomic_t counter_val = ATOMIC_INIT(0);
static atomic_t read_ops    = ATOMIC_INIT(0);
static atomic_t write_ops   = ATOMIC_INIT(0);

static ssize_t counter_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char kbuf[32];
    int len;
    int current_val;

    if (*ppos > 0)
        return 0;

    current_val = atomic_read(&counter_val);
    atomic_inc(&counter_val);
    atomic_inc(&read_ops);

    len = snprintf(kbuf, sizeof(kbuf), "%d\n", current_val);

    if (copy_to_user(buf, kbuf, len))
        return -EFAULT;

    *ppos += len;
    return len;
}

static ssize_t counter_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char kbuf[32];
    int new_val;
    int ret;
    size_t copy_len;

    if (count == 0)
        return 0;

    copy_len = (count < sizeof(kbuf) - 1) ? count : (sizeof(kbuf) - 1);
    if (copy_from_user(kbuf, buf, copy_len))
        return -EFAULT;
    kbuf[copy_len] = '\0';

    ret = kstrtoint(strim(kbuf), 10, &new_val);
    if (ret < 0 || new_val < 0) {
        pr_warn("%s: Invalid value written\n", DRIVER_NAME);
        return -EINVAL;
    }

    atomic_set(&counter_val, new_val);
    atomic_inc(&write_ops);

    pr_info("%s: Counter set to %d\n", DRIVER_NAME, new_val);
    return count;
}

static const struct file_operations counter_fops = {
    .owner   = THIS_MODULE,
    .read    = counter_read,
    .write   = counter_write,
};

static struct miscdevice counter_misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEV_NAME,
    .fops  = &counter_fops,
    .mode  = 0666,
};

static int counter_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "=== Counter Driver Statistics ===\n");
    seq_printf(m, "Current Counter Value   : %d\n", atomic_read(&counter_val));
    seq_printf(m, "Total Read Count (auto++) : %d\n", atomic_read(&read_ops));
    seq_printf(m, "Total Write/Reset Count : %d\n", atomic_read(&write_ops));
    return 0;
}

static int counter_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, counter_proc_show, NULL);
}

static const struct proc_ops counter_proc_ops = {
    .proc_open    = counter_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init counter_init(void)
{
    int ret;

    ret = misc_register(&counter_misc_dev);
    if (ret) {
        pr_err("%s: Failed to register misc device\n", DRIVER_NAME);
        return ret;
    }

    if (!proc_create(PROC_NAME, 0444, NULL, &counter_proc_ops)) {
        misc_deregister(&counter_misc_dev);
        pr_err("%s: Failed to create /proc/%s\n", DRIVER_NAME, PROC_NAME);
        return -ENOMEM;
    }

    pr_info("%s: Initialized successfully: /dev/%s, /proc/%s\n", DRIVER_NAME, DEV_NAME, PROC_NAME);
    return 0;
}

static void __exit counter_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    misc_deregister(&counter_misc_dev);
    pr_info("%s: Cleanup successfully\n", DRIVER_NAME);
}

module_init(counter_init);
module_exit(counter_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huynh Do Hoang Phuc - SS196635");
MODULE_DESCRIPTION("Assignment 01 - Task 2 Misc Device Counter Driver");
