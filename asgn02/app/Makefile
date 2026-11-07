CC_ARM = arm-linux-gnueabihf-gcc
CC_HOST = gcc
CFLAGS = -Wall -Wextra -pthread -std=gnu99 -O2
LDFLAGS = -lrt

all: sms_app sms_app_x86 ioctl_cli

sms_app: sms_app.c
	$(CC_ARM) $(CFLAGS) -o $@ $^ $(LDFLAGS)

sms_app_x86: sms_app.c
	$(CC_HOST) $(CFLAGS) -o $@ $^ $(LDFLAGS)

ioctl_cli: ioctl_cli.c
	$(CC_ARM) $(CFLAGS) -o $@ $^

clean:
	rm -f sms_app sms_app_x86 ioctl_cli *.o
