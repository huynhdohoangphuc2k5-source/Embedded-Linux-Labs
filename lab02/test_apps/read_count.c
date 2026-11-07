/* 1. Khai báo biến toàn cục */
static int read_count = 0;

/* 2. Trong hàm lab2_read() */
static ssize_t lab2_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    int bytes_read = 0;
    
    mutex_lock(&lab2_mutex);
    read_count++;
    mutex_unlock(&lab2_mutex);

    if (*offset >= buffer_len)
        return 0;
    if (len > buffer_len - *offset)
        len = buffer_len - *offset;
    if (copy_to_user(buf, device_buffer + *offset, len))
        return -EFAULT;

    *offset += len;
    bytes_read = len;
    return bytes_read;
}

/* 3. Thêm sysfs show callback & attribute */
static ssize_t read_count_show(struct device *dev, struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", read_count);
}
static DEVICE_ATTR_RO(read_count);

/* 4. Thêm vào lab2_attrs[] */
static struct attribute *lab2_attrs[] = {
    &dev_attr_buffer_len.attr,
    &dev_attr_open_count.attr,
    &dev_attr_last_data.attr,
    &dev_attr_read_count.attr, /* Thêm dòng này */
    NULL,
};


                     Bước 1: Mở file code để dán trên Ubuntu Host

cd ~/embedded_lab2/driver
nano lab2_driver.c

                     Bước 2: Build lại module trên Ubuntu Host

make clean && make

                    Bước 3: Copy file .ko vào rootfs và đóng gói initramfs

# 1. Chép module mới vào rootfs
cp lab2_driver.ko ~/embedded_lab2/rootfs/initramfs/lib/modules/5.15.0/

# 2. Đóng gói lại initramfs
cd ~/embedded_lab2/rootfs/initramfs
find . | cpio -H newc -o | gzip > ~/embedded_lab2/output/initramfs_lab2.cpio.gz

                    Bước 4: Khởi động QEMU

qemu-system-arm \
  -M vexpress-a9 \
  -cpu cortex-a9 \
  -m 512M \
  -smp 2 \
  -nographic \
  -kernel ~/embedded_lab1/output/zImage \
  -dtb ~/embedded_lab1/output/vexpress-v2p-ca9.dtb \
  -initrd ~/embedded_lab2/output/initramfs_lab2.cpio.gz \
  -append 'console=ttyAMA0,115200 rdinit=/sbin/init mem=512M'

                   Bước 5: Dán các lệnh kiểm tra (Tại màn hình QEMU)

cat /sys/class/lab2_class/lab2/read_count
cat /dev/lab2
cat /sys/class/lab2_class/lab2/read_count
