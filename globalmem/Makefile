#KERNELDIR ?= /lib/modules/$(shell uname -r)/build
KERNELDIR ?= $(KERNEL_CODE_DIR)
PWD := $(shell pwd)

# Kernel modules
obj-m += globalmem.o

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c *.mod .tmp_versions Module.symvers modules.order

.PHONY: modules modules_install clean

