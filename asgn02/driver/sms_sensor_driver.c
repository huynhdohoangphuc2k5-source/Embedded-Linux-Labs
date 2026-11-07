#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include "sms_sensor.h"

#define DRIVER_NAME "sms_sensor"
#define PROC_NAME   "sms_stats"

static int major_num = 246;
module_param(major_num, int, 0644);

static DEFINE_MUTEX(sensor_mutex);
static unsigned long read_count = 0;
static int last_temp_x10 = 250;
static int last_humid_x10 = 500;
static ktime_t driver_start_time;
static unsigned int min_interval_ms = 100;
static ktime_t last_read_time;

static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset)
{
    char kbuf[64];
    int str_len;
    u32 rand_val;
    s64 now_ms;

    if (*offset > 0)
        return 0;

    mutex_lock(&sensor_mutex);

    now_ms = ktime_to_ms(ktime_get());
    if (ktime_to_ms(ktime_sub(ktime_get(), last_read_time)) < min_interval_ms) {
        mutex_unlock(&sensor_mutex);
        return -EAGAIN;
    }
    last_read_time = ktime_get();

    rand_val = get_random_u32();
    last_temp_x10 = 150 + (rand_val % 301);

    rand_val = get_random_u32();
    last_humid_x10 = 300 + (rand_val % 601);

    read_count++;
    str_len = snprintf(kbuf, sizeof(kbuf), "%lld,%d.%d,%d.%d\n",
                       now_ms,
                       last_temp_x10 / 10, last_temp_x10 % 10,
                       last_humid_x10 / 10, last_humid_x10 % 10);

    if (copy_to_user(buffer, kbuf, str_len)) {
        mutex_unlock(&sensor_mutex);
        return -EFAULT;
    }

    *offset += str_len;
    mutex_unlock(&sensor_mutex);
    return str_len;
}

static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    unsigned int val;
    if (cmd == SMS_SET_INTERVAL) {
        if (copy_from_user(&val, (unsigned int __user *)arg, sizeof(val)))
            return -EFAULT;
        mutex_lock(&sensor_mutex);
        min_interval_ms = val;
        mutex_unlock(&sensor_mutex);
        pr_info("sms_sensor: interval set to %u ms\n", val);
        return 0;
    }
    return -ENOTTY;
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .read           = dev_read,
    .unlocked_ioctl = dev_ioctl,
};

static int stats_proc_show(struct seq_file *m, void *v)
{
    s64 uptime_sec = ktime_divns(ktime_sub(ktime_get(), driver_start_time), 1000000000);
    seq_printf(m, "read_count: %lu\n", read_count);
    seq_printf(m, "last_temperature: %d.%d\n", last_temp_x10 / 10, last_temp_x10 % 10);
    seq_printf(m, "last_humidity: %d.%d\n", last_humid_x10 / 10, last_humid_x10 % 10);
    seq_printf(m, "driver_uptime: %lld\n", uptime_sec);
    return 0;
}

static int stats_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, stats_proc_show, NULL);
}

static const struct proc_ops proc_fops = {
    .proc_open    = stats_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init sms_driver_init(void)
{
    int ret = register_chrdev(major_num, DRIVER_NAME, &fops);
    if (ret < 0) return ret;

    driver_start_time = ktime_get();
    last_read_time = ktime_set(0, 0);

    if (!proc_create(PROC_NAME, 0444, NULL, &proc_fops)) {
        unregister_chrdev(major_num, DRIVER_NAME);
        return -ENOMEM;
    }
    pr_info("sms_sensor: registered with major %d\n", major_num);
    return 0;
}

static void __exit sms_driver_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    unregister_chrdev(major_num, DRIVER_NAME);
    pr_info("sms_sensor: unregistered\n");
}

module_init(sms_driver_init);
module_exit(sms_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huynh Do Hoang Phuc - SS196635");
