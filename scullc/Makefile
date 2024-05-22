#KERNELDIR ?= /lib/modules/$(shell uname -r)/build
KERNELDIR ?= $(KERNEL_CODE_DIR)
PWD := $(shell pwd)
DEBUG = y
ifeq (${DEBUG}, y)
	DBGFLAGS = -O -g -DSCULLC_DEBUG # "-O" is need to expand to inlines ? 
else
	DBGFLAGS = -O2
endif

CFLAGS += ${DBGFLAGS}
# Kernel modules
obj-m += scullc.o

modules:
#	@echo "$(KERNEL_CODE_DIR)"
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c *.mod .tmp_versions Module.symvers modules.order

.PHONY: modules modules_install clean