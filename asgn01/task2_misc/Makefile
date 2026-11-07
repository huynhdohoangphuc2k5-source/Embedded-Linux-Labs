KDIR ?= /lib/modules/$(shell uname -r)/build
ARCH ?= $(shell uname -m)
CROSS_COMPILE ?= 

obj-m += counter_driver.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
