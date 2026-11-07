#define MAX_ALLOWED_WRITE 128

static ssize_t lab2_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    if (len > MAX_ALLOWED_WRITE) {          //them cai nay 
        pr_warn("lab2_driver: write truncated to %d bytes\n", MAX_ALLOWED_WRITE);
        len = MAX_ALLOWED_WRITE;
    }

    mutex_lock(&lab2_mutex);
    if (copy_from_user(device_buffer, buf, len)) {
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }
    buffer_len = len;
    device_buffer[buffer_len] = '\0';
    mutex_unlock(&lab2_mutex);
    return len;
}












static ssize_t lab2_write(struct file *file, const char __user *buf,
                          size_t len, loff_t *offset) {
    if (len > 128) {                                          //them cai nay 
        pr_warn("lab2_driver: write too large, truncating to 128 bytes\n");
        len = 128;
    }

    mutex_lock(&lab2_mutex);
    if (copy_from_user(device_buffer, buf, len)) {
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }
    buffer_len = len;
    device_buffer[buffer_len] = '\0';
    mutex_unlock(&lab2_mutex);

    pr_info("lab2_driver: received %zu bytes: [%s]\n", len, device_buffer);
    return len;
}
