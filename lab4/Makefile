obj-m := lab4.o

KDIR := /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

all: run test clean
	sudo dmesg | tail -n 4
run:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	sudo insmod lab4.ko

test:
	cat /proc/tsulab

clean:
	sudo rmmod lab4
	$(MAKE) -C $(KDIR) M=$(PWD) clean

