# Embedded Linux Labs

Bài thực hành và bài tập môn **Lập trình nhúng trên Linux**.

- **Sinh viên:** Huỳnh Đỗ Hoàng Phúc
- **MSSV:** SS196635

## Cấu trúc thư mục

```
Embedded-Linux-Labs/
├── lab01/      # Build hệ thống Linux nhúng (U-Boot, BusyBox, initramfs)
│   └── bao_cao/
├── lab02/      # Character device driver
│   ├── driver/
│   └── test_apps/
├── lab03/      # Debug & profiling ứng dụng user-space
│   ├── apps/
│   └── logs/
├── asgn01/     # Assignment 01: ioctl driver & misc device
│   ├── task1_ioctl/
│   └── task2_misc/
└── asgn02/     # Assignment 02: sensor driver + ứng dụng đa luồng
    ├── driver/
    └── app/
```

## Nội dung

### Lab 01 – Xây dựng hệ thống Linux nhúng
Build U-Boot, BusyBox và tạo initramfs. Chi tiết các bước và kết quả trong `lab01/bao_cao/`.

### Lab 02 – Character Device Driver
- `driver/lab2_driver.c`: kernel module character device.
- `test_apps/`: các chương trình kiểm thử (`read_count`, `write_count`, `buffer_size`, `clear`, `gioi_han_data_ghi`).

### Lab 03 – Debug & Profiling
- `apps/`: `buggy_app.c`, `leaky_app.c` / `leaky_app_fixed.c` (rò rỉ bộ nhớ và bản đã sửa), `perf_target.c`.
- `logs/`: kết quả phân tích bằng **gdb**, **strace**, **valgrind**, **perf** và biểu đồ latency.

### Assignment 01
- `task1_ioctl/`: driver hỗ trợ **ioctl** (`asgn1_driver.c`) và công cụ dòng lệnh `ioctl_cli.c`.
- `task2_misc/`: **misc device** đếm (`counter_driver.c`).
- Mỗi task có script kiểm thử `.sh`.

### Assignment 02
- `driver/sms_sensor_driver.c`: kernel module cho cảm biến.
- `app/`: ứng dụng đa luồng gồm sensor, processor, alert, logger thread và circular buffer.

## Build & chạy

### Kernel module
```bash
cd <thư_mục_driver>
make
sudo insmod <tên_module>.ko
dmesg | tail
sudo rmmod <tên_module>
```

### Ứng dụng user-space
```bash
cd asgn02/app
make
./sms_app
```

### Script kiểm thử Assignment 01
```bash
cd asgn01/task1_ioctl && sudo ./test_asgn1.sh
cd asgn01/task2_misc  && sudo ./test_counter.sh
```

## Yêu cầu
- Linux (đã thử trên Ubuntu)
- `build-essential`, `linux-headers-$(uname -r)`
- Lab 03: `gdb`, `strace`, `valgrind`, `linux-tools` (perf)
