
#SUBDIRS =  misc-progs misc-modules \
#           skull scull scullc sculld scullp scullv sbull snull\
#	   short shortprint pci simple usb tty lddbus
# 
WORKSPACE:=armlinux
ifeq (${X86}, y)
	WORKSPACE:=x86linux
	ARCH=x86
	CROSS_COMPILE=
endif

SUBDIRS = hello globalmem scull misc_progs

LINUX_KERNEL_VERSION=5.4.284
HOME=/home/$(shell users)
KERNEL_CODE_DIR = $(HOME)/$(WORKSPACE)/linux-$(LINUX_KERNEL_VERSION)
PROJ_DIR=$(HOME)/mydrivers
CC=$(CROSS_COMPILE)gcc

export LINUX_KERNEL_VERSION HOME KERNEL_CODE_DIR PROJ_DIR CC

all: subdirs
subdirs:
	for n in $(SUBDIRS); do $(MAKE) -C $$n || exit 1; done
	@echo $(ARCH)

clean:
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean; done
