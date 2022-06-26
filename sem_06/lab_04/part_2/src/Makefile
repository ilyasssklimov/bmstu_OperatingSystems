KVERSION = $(shell uname -r)
KDIR = /lib/modules/$(KVERSION)/build
PWD = $(shell pwd)
obj-m := fortune.o seqfile.o

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD)
	@rm -f *.ko *.symvers *.mod *.o *.mod.c *.order
	