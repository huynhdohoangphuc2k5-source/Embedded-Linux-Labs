#!/bin/sh
set -e
MAJOR=245
DEV="/dev/asgn1"

echo "=== 1. Loading Module & Creating Node ==="
insmod asgn1_driver.ko major_num=$MAJOR
mknod $DEV c $MAJOR 0
chmod 666 $DEV

echo "=== 2. Test Echo Mode (Mode 1 Default) ==="
echo "Embedded Linux" > $DEV
cat $DEV

echo "=== 3. Test Kernel Mode (Mode 0) ==="
./ioctl_cli $DEV mode 0
echo "System Test" > $DEV
cat $DEV

echo "=== 4. Test Version and Stats ==="
./ioctl_cli $DEV version
./ioctl_cli $DEV stats

echo "=== 5. Test Reset Buffer ==="
./ioctl_cli $DEV reset
echo "Content after reset: [$(cat $DEV)]"

echo "=== 6. Cleanup ==="
rm -f $DEV
rmmod asgn1_driver
dmesg | tail -n 15
