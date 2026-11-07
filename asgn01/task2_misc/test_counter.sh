#!/bin/sh
set -e

DEV="/dev/counter"
PROC="/proc/counter_info"
MODULE="counter_driver.ko"

echo "=========================================="
echo "    BẮT ĐẦU TEST TASK 2: COUNTER DRIVER   "
echo "=========================================="

# 1. Nạp module
echo "[+] Step 1: Loading counter module..."
insmod $MODULE

# Chờ udev/devtmpfs tạo node tự động
sleep 1

# Kiểm tra device node & proc
if [ ! -c "$DEV" ]; then
    # Trường hợp rootfs tối giản trên QEMU chưa cấu hình mdev/udev tự động tạo node:
    echo "[!] Node $DEV chưa tự động tạo, đọc minor từ sysfs và mknod thủ công..."
    MINOR=$(cat /sys/class/misc/counter/dev | cut -d: -f2)
    mknod $DEV c 10 $MINOR
    chmod 666 $DEV
fi

ls -l $DEV
echo "\n--- /proc/counter_info ban đầu (Thời điểm 1) ---"
cat $PROC

# 2. Test auto-increment on read
echo "\n[+] Step 2: Testing auto-increment on read..."
echo "Read 1: $(cat $DEV)"
echo "Read 2: $(cat $DEV)"
echo "Read 3: $(cat $DEV)"

# 3. Test ghi giá trị 100
echo "\n[+] Step 3: Setting counter to 100..."
echo "100" > $DEV
echo "Read after set 100: $(cat $DEV)"
echo "Next read: $(cat $DEV)"

echo "\n--- /proc/counter_info sau khi ghi 100 (Thời điểm 2) ---"
cat $PROC

# 4. Test Reset về 0
echo "\n[+] Step 4: Resetting counter to 0..."
echo "0" > $DEV
echo "Read after reset: $(cat $DEV)"

# 5. Test xử lý input không hợp lệ (kỳ vọng trả về lỗi)
echo "\n[+] Step 5: Testing invalid inputs..."
set +e
echo "-5" > $DEV 2>/dev/null
if [ $? -ne 0 ]; then
    echo "[OK] Rejected negative number (-5)"
fi

echo "abc" > $DEV 2>/dev/null
if [ $? -ne 0 ]; then
    echo "[OK] Rejected non-numeric string ('abc')"
fi
set -e

echo "\n--- /proc/counter_info trước khi gỡ module (Thời điểm 3) ---"
cat $PROC

# 6. Gỡ module và dọn dẹp
echo "\n[+] Step 6: Cleaning up..."
rmmod counter_driver

echo "\n[+] Kernel logs (dmesg):"
dmesg | tail -n 15

echo "=========================================="
echo "           HOÀN THÀNH TEST TASK 2         "
echo "=========================================="
