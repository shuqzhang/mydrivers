
#SUBDIRS =  misc-progs misc-modules \
#           skull scull scullc sculld scullp scullv sbull snull\
#	   short shortprint pci simple usb tty lddbus
# 
WORKSPACE:=armlinux
HOME=/home/$(shell users)
KERNEL_CODE_DIR := $(HOME)/linux
ifeq (${x86}, y)
	WORKSPACE:=x86linux
	ARCH=x86
	CROSS_COMPILE=
	KERNEL_CODE_DIR := /usr/src/linux-headers-5.15.0-124-generic
endif

SUBDIRS = hello globalmem scull misc_progs completion

LINUX_KERNEL_VERSION=5.15.0
PROJ_DIR=$(HOME)/mydrivers
CC=$(CROSS_COMPILE)gcc

export LINUX_KERNEL_VERSION HOME KERNEL_CODE_DIR PROJ_DIR CC

all: subdirs
subdirs:
	for n in $(SUBDIRS); do $(MAKE) -C $$n || exit 1; done
	@echo $(ARCH)

clean:
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean; done
