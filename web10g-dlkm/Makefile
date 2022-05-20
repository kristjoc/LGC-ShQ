EXTRA_CFLAGS= -g -O3 -Wall -I./ -std=gnu99 -fgnu89-inline

obj-m := test.o
obj-m := tcp_estats_nl.o
tcp_estats_nl-objs := tcp_estats_mib_var.o tcp_estats_nl_kmod.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)
install:
	cp tcp_estats_nl.ko /lib/modules/5.4.0/kernel/net/ipv4/
	depmod -a
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
