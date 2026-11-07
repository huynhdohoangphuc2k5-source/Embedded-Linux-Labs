KDIR ?= /lib/modules/$(shell uname -r)/build
ARCH ?= $(shell uname -m)
CROSS_COMPILE ?= 

obj-m += asgn1_driver.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	$(CROSS_COMPILE)gcc -Wall -Wextra -o ioctl_cli ioctl_cli.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f ioctl_cli
