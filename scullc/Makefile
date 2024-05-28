#KERNELDIR ?= /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)
DEBUG := y
DBGFLAGS :=
ifeq (${DEBUG}, y)
	DBGFLAGS := -O -g -DSCULLC_DEBUG # "-O" is need to expand to inlines ? 
else
	DBGFLAGS := -O2
endif

EXTRA_CFLAGS += ${DBGFLAGS} -I$(PROJ_DIR)
ifneq ($(KERNELRELEASE),)
# call from kernel build system

# Kernel modules
obj-m += scullc.o

else

KERNELDIR ?= $(KERNEL_CODE_DIR)
PWD       := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif


modules:
#	@echo "$(KERNEL_CODE_DIR)"
#	@echo "$(PROJ_DIR)"
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c *.mod .tmp_versions Module.symvers modules.order

.PHONY: modules modules_install clean