static ssize_t buffer_size_show(struct device *dev, struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", BUFFER_SIZE);
}
static DEVICE_ATTR_RO(buffer_size);







/* Thêm vào lab2_attrs[] */
static struct attribute *lab2_attrs[] = {
    &dev_attr_buffer_len.attr,
    &dev_attr_open_count.attr,
    &dev_attr_last_data.attr,
    &dev_attr_buffer_size.attr, /* Thêm dòng này */
    NULL,
};









cat /sys/class/lab2_class/lab2/buffer_size   # Mong đợi: 1024



