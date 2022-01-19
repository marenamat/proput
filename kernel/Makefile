obj-m += proput.o
proput-objs := ufp.o main.o device.o
VERSION := $(shell uname -r)

all:
	make -C /lib/modules/$(VERSION)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(VERSION)/build M=$(PWD) clean
