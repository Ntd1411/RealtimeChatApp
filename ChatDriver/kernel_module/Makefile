obj-m += chat_crypto.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	insmod chat_crypto.ko

uninstall:
	rmmod chat_crypto

.PHONY: all clean install uninstall
