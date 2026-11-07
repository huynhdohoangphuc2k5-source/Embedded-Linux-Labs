#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include "asgn1_ioctl.h"

#define DRIVER_NAME "asgn1_driver"
#define BUF_SIZE 1024
#define VERSION_STR "asgn1_driver v1.0"

static int major_num = 245;
module_param(major_num, int, 0644);
MODULE_PARM_DESC(major_num, "Major device number (241-254)");

static char device_buffer[BUF_SIZE];
static int buffer_len = 0;
static int open_count = 0;
static int write_count = 0;
static int read_count = 0;
static int last_write_size = 0;
static int current_mode = 1;

static DEFINE_MUTEX(asgn1_mutex);

static int dev_open(struct inode *inodep, struct file *filep)
{
    mutex_lock(&asgn1_mutex);
    open_count++;
    mutex_unlock(&asgn1_mutex);
    pr_info("%s: Device opened (open_count=%d)\n", DRIVER_NAME, open_count);
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
    pr_info("%s: Device closed\n", DRIVER_NAME);
    return 0;
}

static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset)
{
    ssize_t bytes_to_read;

    mutex_lock(&asgn1_mutex);
    if (*offset >= buffer_len) {
        mutex_unlock(&asgn1_mutex);
        return 0;
    }

    bytes_to_read = buffer_len - *offset;
    if (len < bytes_to_read)
        bytes_to_read = len;

    if (copy_to_user(buffer, device_buffer + *offset, bytes_to_read)) {
        mutex_unlock(&asgn1_mutex);
        return -EFAULT;
    }

    *offset += bytes_to_read;
    read_count++;
    mutex_unlock(&asgn1_mutex);

    return bytes_to_read;
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset)
{
    char temp_buf[BUF_SIZE];
    size_t copy_len;

    if (len >= BUF_SIZE)
        copy_len = BUF_SIZE - 1;
    else
        copy_len = len;

    if (copy_from_user(temp_buf, buffer, copy_len))
        return -EFAULT;
    temp_buf[copy_len] = '\0';

    mutex_lock(&asgn1_mutex);
    if (current_mode == 0) {
        snprintf(device_buffer, BUF_SIZE, "[KERNEL] %s", temp_buf);
    } else {
        strncpy(device_buffer, temp_buf, BUF_SIZE);
        device_buffer[BUF_SIZE - 1] = '\0';
    }

    buffer_len = strlen(device_buffer);
    last_write_size = (int)copy_len;
    write_count++;
    mutex_unlock(&asgn1_mutex);

    return copy_len;
}

static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    mutex_lock(&asgn1_mutex);

    switch (cmd) {
    case ASGN1_RESET_BUFFER:
        memset(device_buffer, 0, BUF_SIZE);
        buffer_len = 0;
        write_count = 0;
        read_count = 0;
        last_write_size = 0;
        pr_info("%s: IOCTL ASGN1_RESET_BUFFER executed\n", DRIVER_NAME);
        break;

    case ASGN1_GET_STATS: {
        struct asgn1_stats stats;
        stats.open_count = open_count;
        stats.write_count = write_count;
        stats.read_count = read_count;
        stats.buffer_len = buffer_len;
        stats.last_write_size = last_write_size;

        if (copy_to_user((struct asgn1_stats __user *)arg, &stats, sizeof(stats))) {
            mutex_unlock(&asgn1_mutex);
            return -EFAULT;
        }
        pr_info("%s: IOCTL ASGN1_GET_STATS executed\n", DRIVER_NAME);
        break;
    }

    case ASGN1_SET_MODE: {
        int new_mode;
        if (copy_from_user(&new_mode, (int __user *)arg, sizeof(int))) {
            mutex_unlock(&asgn1_mutex);
            return -EFAULT;
        }
        if (new_mode != 0 && new_mode != 1) {
            mutex_unlock(&asgn1_mutex);
            return -EINVAL;
        }
        current_mode = new_mode;
        pr_info("%s: IOCTL ASGN1_SET_MODE set to %d\n", DRIVER_NAME, current_mode);
        break;
    }

    case ASGN1_GET_VERSION: {
        char ver[64];
        strncpy(ver, VERSION_STR, sizeof(ver));
        ver[sizeof(ver) - 1] = '\0';

        if (copy_to_user((char __user *)arg, ver, sizeof(ver))) {
            mutex_unlock(&asgn1_mutex);
            return -EFAULT;
        }
        pr_info("%s: IOCTL ASGN1_GET_VERSION executed\n", DRIVER_NAME);
        break;
    }

    default:
        mutex_unlock(&asgn1_mutex);
        return -ENOTTY;
    }

    mutex_unlock(&asgn1_mutex);
    return 0;
}

static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = dev_open,
    .release        = dev_release,
    .read           = dev_read,
    .write          = dev_write,
    .unlocked_ioctl = dev_ioctl,
};

static int __init asgn1_init(void)
{
    int ret = register_chrdev(major_num, DRIVER_NAME, &fops);
    if (ret < 0) {
        pr_err("%s: Failed to register chrdev with major %d\n", DRIVER_NAME, major_num);
        return ret;
    }
    pr_info("%s: Initialized with major %d\n", DRIVER_NAME, major_num);
    return 0;
}

static void __exit asgn1_exit(void)
{
    unregister_chrdev(major_num, DRIVER_NAME);
    pr_info("%s: Unregistered\n", DRIVER_NAME);
}

module_init(asgn1_init);
module_exit(asgn1_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huynh Do Hoang Phuc");
MODULE_DESCRIPTION("Assignment 01 - ioctl Driver");
