/**
 * LAB-02: Character Device Driver với procfs và sysfs
 * Target: ARMv7-A (QEMU vexpress-a9) / Linux Kernel 5.15 LTS
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Lab");
MODULE_DESCRIPTION("LAB-02: Character Device Driver with procfs and sysfs");
MODULE_VERSION("1.0");

#define DEVICE_NAME     "lab2"
#define CLASS_NAME      "lab2_class"
#define BUFFER_SIZE     1024
#define DRIVER_MAJOR    240    /* Major number tĩnh (240-254 dùng cho local/thử nghiệm) */

/* =========================================================================
 * Biến toàn cục & Bộ đệm dữ liệu (Kernel Space)
 * ========================================================================= */
static int    major_number = DRIVER_MAJOR;
static char   device_buffer[BUFFER_SIZE];
static int    buffer_len = 0;
static int    open_count = 0;

static struct class  *lab2_class  = NULL;
static struct device *lab2_device = NULL;
static struct proc_dir_entry *proc_entry = NULL;

/**
 * GIẢI THÍCH VỀ MUTEX:
 * Driver chạy trong môi trường đa nhiệm preemptive đa nhân (SMP). Khi nhiều 
 * tiến trình userspace cùng truy cập /dev/lab2, các biến dùng chung như 
 * device_buffer, buffer_len, open_count sẽ xảy ra race condition (xung đột dữ liệu).
 * DEFINE_MUTEX khởi tạo một mutex ở trạng thái unlocked tại compile-time, 
 * dùng để đảm bảo tại một thời điểm chỉ có 1 thread thực thi đoạn critical section.
 */
static DEFINE_MUTEX(lab2_mutex);

/* =========================================================================
 * Triển khai các phương thức File Operations (VFS)
 * ========================================================================= */

/**
 * lab2_open: Được gọi khi userspace thực thi open("/dev/lab2", ...)
 */
static int lab2_open(struct inode *inode, struct file *file) {
    /* Bảo vệ biến đếm open_count khỏi race condition khi nhiều process mở cùng lúc */
    mutex_lock(&lab2_mutex);
    open_count++;
    mutex_unlock(&lab2_mutex);

    pr_info("lab2_driver: device opened (open_count=%d)\n", open_count);
    return 0;
}

/**
 * lab2_release: Được gọi khi userspace gọi close(fd) hoặc tiến trình kết thúc
 */
static int lab2_release(struct inode *inode, struct file *file) {
    pr_info("lab2_driver: device closed\n");
    return 0;
}

/**
 * lab2_read: Được gọi khi userspace thực thi read(fd, buf, len) hoặc cat /dev/lab2
 * 
 * GIẢI THÍCH VỀ copy_to_user():
 * Con trỏ buf thuộc không gian địa chỉ ảo của userspace process (User Space Virtual Address).
 * Kernel KHÔNG THỂ dùng memcpy() trực tiếp vì:
 * 1. Địa chỉ user có thể không hợp lệ (NULL, con trỏ rác, vùng nhớ kernel bị cấm truy cập).
 * 2. Trang bộ nhớ user có thể bị swap hoặc chưa được map (Page Fault).
 * copy_to_user() kiểm tra quyền truy cập hợp lệ, xử lý fault an toàn và trả về số byte
 * chưa thể copy (0 nếu thành công).
 */
static ssize_t lab2_read(struct file *file, char __user *buf,
                         size_t len, loff_t *offset) {
    int bytes_read = 0;

    /* Nếu offset vượt quá độ dài dữ liệu hiện có -> Báo EOF (End Of File) */
    if (*offset >= buffer_len)
        return 0;

    /* Cắt giảm độ dài nếu userspace yêu cầu đọc nhiều hơn lượng dữ liệu còn lại */
    if (len > buffer_len - *offset)
        len = buffer_len - *offset;

    /* Sao chép an toàn từ kernel buffer sang user buffer */
    if (copy_to_user(buf, device_buffer + *offset, len)) {
        pr_err("lab2_driver: copy_to_user failed\n");
        return -EFAULT;
    }

    *offset += len;
    bytes_read = len;

    pr_info("lab2_driver: sent %d bytes to user (offset=%lld)\n", bytes_read, *offset);
    return bytes_read;
}

/**
 * lab2_write: Được gọi khi userspace thực thi write(fd, buf, len) hoặc echo "..." > /dev/lab2
 * 
 * GIẢI THÍCH VỀ copy_from_user():
 * Tương tự copy_to_user(), copy_from_user() giúp đọc an toàn từ user buffer sang kernel buffer,
 * ngăn ngừa nguy cơ bảo mật khi userspace truyền vào địa chỉ trỏ tới vùng nhớ kernel.
 */
static ssize_t lab2_write(struct file *file, const char __user *buf,
                          size_t len, loff_t *offset) {
    /* Đảm bảo dữ liệu không vượt quá kích thước buffer cho phép (-1 byte để chừa chỗ cho '\0') */
    if (len > BUFFER_SIZE - 1) {
        pr_warn("lab2_driver: write too large, truncating to %d bytes\n", BUFFER_SIZE - 1);
        len = BUFFER_SIZE - 1;
    }

    mutex_lock(&lab2_mutex);

    if (copy_from_user(device_buffer, buf, len)) {
        mutex_unlock(&lab2_mutex);
        pr_err("lab2_driver: copy_from_user failed\n");
        return -EFAULT;
    }

    buffer_len = len;
    device_buffer[buffer_len] = '\0'; /* Kết thúc chuỗi an toàn */

    mutex_unlock(&lab2_mutex);

    pr_info("lab2_driver: received %zu bytes: [%s]\n", len, device_buffer);
    return len;
}

/* Ánh xạ các hàm xử lý vào bảng con trỏ file_operations */
static struct file_operations lab2_fops = {
    .owner   = THIS_MODULE,
    .open    = lab2_open,
    .release = lab2_release,
    .read    = lab2_read,
    .write   = lab2_write,
};

/* =========================================================================
 * Triển khai giao diện Procfs (/proc/lab2_info)
 * ========================================================================= */

/**
 * lab2_proc_show: Hàm callback xuất nội dung định dạng khi đọc file proc.
 * Sử dụng seq_file API (thay vì sprintf truyền thống) giúp kernel tự động 
 * cấp phát và quản lý bộ đệm lớn, không lo tràn buffer.
 */
static int lab2_proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "LAB-02 Driver Statistics\n");
    seq_printf(m, "======================\n");
    seq_printf(m, "Driver name   : %s\n", DEVICE_NAME);
    seq_printf(m, "Major number  : %d\n", major_number);
    seq_printf(m, "Buffer size   : %d bytes\n", BUFFER_SIZE);
    seq_printf(m, "Data length   : %d bytes\n", buffer_len);
    seq_printf(m, "Open count    : %d\n", open_count);
    if (buffer_len > 0)
        seq_printf(m, "Last data     : [%s]\n", device_buffer);
    return 0;
}

/**
 * lab2_proc_open: Gắn callback show vào cơ chế single_open của seq_file
 */
static int lab2_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, lab2_proc_show, NULL);
}

/* Cấu hình proc_ops tương thích từ Linux Kernel 5.6+ */
static const struct proc_ops lab2_proc_fops = {
    .proc_open    = lab2_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* =========================================================================
 * Triển khai giao diện Sysfs (/sys/class/lab2_class/lab2/...)
 * ========================================================================= */

/**
 * Macro DEVICE_ATTR_RO(name) sẽ tự động tạo struct device_attribute dev_attr_name
 * trỏ đến hàm callback name_show() với quyền đọc chỉ định (0444).
 */
static ssize_t buffer_len_show(struct device *dev,
                               struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", buffer_len);
}

static ssize_t open_count_show(struct device *dev,
                               struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", open_count);
}

static ssize_t last_data_show(struct device *dev,
                              struct device_attribute *attr, char *buf) {
    if (buffer_len > 0)
        return sprintf(buf, "%s\n", device_buffer);
    return sprintf(buf, "(empty)\n");
}

static DEVICE_ATTR_RO(buffer_len);
static DEVICE_ATTR_RO(open_count);
static DEVICE_ATTR_RO(last_data);

/* Gom các attributes vào một nhóm kobject để đăng ký tự động */
static struct attribute *lab2_attrs[] = {
    &dev_attr_buffer_len.attr,
    &dev_attr_open_count.attr,
    &dev_attr_last_data.attr,
    NULL,
};

static struct attribute_group lab2_attr_group = {
    .attrs = lab2_attrs,
};

/* =========================================================================
 * Khởi tạo (Module Init) & Gỡ bỏ (Module Exit)
 * ========================================================================= */

static int __init lab2_init(void) {
    int ret;
    pr_info("lab2_driver: initializing module\n");

    /* 1. Đăng ký character device với major number cố định */
    ret = register_chrdev(major_number, DEVICE_NAME, &lab2_fops);
    if (ret < 0) {
        pr_err("lab2_driver: register_chrdev failed: %d\n", ret);
        return ret;
    }

    /* 2. Tạo device class trong sysfs (/sys/class/lab2_class) */
    lab2_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(lab2_class)) {
        pr_err("lab2_driver: failed to create class\n");
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_class);
    }

    /* 3. Tạo device node metadata (/sys/class/lab2_class/lab2) và uevent cho udev/mknod */
    lab2_device = device_create(lab2_class, NULL,
                                MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(lab2_device)) {
        pr_err("lab2_driver: failed to create device\n");
        class_destroy(lab2_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_device);
    }

    /* 4. Đăng ký sysfs attribute group */
    ret = sysfs_create_group(&lab2_device->kobj, &lab2_attr_group);
    if (ret)
        pr_warn("lab2_driver: sysfs_create_group failed: %d\n", ret);

    /* 5. Tạo entry /proc/lab2_info (read-only: 0444) */
    proc_entry = proc_create("lab2_info", 0444, NULL, &lab2_proc_fops);
    if (!proc_entry)
        pr_warn("lab2_driver: failed to create /proc/lab2_info\n");
    else
        pr_info("lab2_driver: /proc/lab2_info created successfully\n");

    pr_info("lab2_driver: loaded successfully, major=%d\n", major_number);
    return 0;
}

static void __exit lab2_exit(void) {
    /* Dọn dẹp theo thứ tự ngược lại lúc init để tránh null-pointer dereference */
    if (proc_entry)
        proc_remove(proc_entry);

    if (lab2_device) {
        sysfs_remove_group(&lab2_device->kobj, &lab2_attr_group);
        device_destroy(lab2_class, MKDEV(major_number, 0));
    }

    if (lab2_class)
        class_destroy(lab2_class);

    unregister_chrdev(major_number, DEVICE_NAME);
    pr_info("lab2_driver: module unloaded successfully\n");
}

module_init(lab2_init);
module_exit(lab2_exit);
