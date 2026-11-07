static int write_count = 0;

/* Trong lab2_write() - tăng sau khi copy_from_user thành công */
static ssize_t lab2_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    if (len > BUFFER_SIZE - 1)
        len = BUFFER_SIZE - 1;

    mutex_lock(&lab2_mutex);
    if (copy_from_user(device_buffer, buf, len)) {
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }
    buffer_len = len;
    device_buffer[buffer_len] = '\0';
    write_count++; /* Tăng sau khi ghi thành công */
    mutex_unlock(&lab2_mutex);
    return len;
}

/* Trong lab2_proc_show() */
static int lab2_proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "LAB-02 Driver Statistics\n======================\n");
    seq_printf(m, "Driver name   : %s\n", DEVICE_NAME);
    seq_printf(m, "Open count    : %d\n", open_count);
    seq_printf(m, "Write count   : %d\n", write_count); /* Thêm dòng này */
    seq_printf(m, "Data length   : %d bytes\n", buffer_len);
    if (buffer_len > 0)
        seq_printf(m, "Last data     : [%s]\n", device_buffer);
    return 0;
}








cat /sys/class/lab2_class/lab2/read_count   # Mong đợi: 0
cat /dev/lab2
cat /sys/class/lab2_class/lab2/read_count   # Tăng lên ít nhất 1 hoặc 2 (tùy cat EOF)
