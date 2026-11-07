mutex_lock(&lab2_mutex);

    /* Sao chep an toan tu User Space -> Kernel Space */
    if (copy_from_user(device_buffer, buf, len)) {
        mutex_unlock(&lab2_mutex);
        pr_err("lab2_driver: copy_from_user failed\n");
        return -EFAULT;
    }

    device_buffer[len] = '\0';

    /* Kiem tra chuoi clear */
    if (strncmp(device_buffer, "clear\n", 6) == 0 || strcmp(device_buffer, "clear") == 0) {
        device_buffer[0] = '\0';
        buffer_len = 0;
        pr_info("lab2_driver: buffer cleared by command\n");
    } else {
        buffer_len = len;
        device_buffer[buffer_len] = '\0';
        write_count++;
    }

    mutex_unlock(&lab2_mutex); /* <--- ĐẶT Ở ĐÂY (NGOÀI CẢ IF VÀ ELSE) */

    pr_info("lab2_driver: received %zu bytes: [%s]\n", len, device_buffer);
    return len;
}










echo "test data" > /dev/lab2
cat /dev/lab2
echo "clear" > /dev/lab2
cat /dev/lab2









static ssize_t lab2_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    if (len > BUFFER_SIZE - 1)
        len = BUFFER_SIZE - 1;

    mutex_lock(&lab2_mutex);
    if (copy_from_user(device_buffer, buf, len)) {
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }
    device_buffer[len] = '\0';

    /* Kiểm tra chuỗi clear */
    if (strncmp(device_buffer, "clear\n", 6) == 0 || strcmp(device_buffer, "clear") == 0) {
        device_buffer[0] = '\0';
        buffer_len = 0;
        pr_info("lab2_driver: buffer cleared by command\n");
    } else {
        buffer_len = len;
    }
    mutex_unlock(&lab2_mutex);
    return len;
}


