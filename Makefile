
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

SUBDIRS = hello globalmem common_scull scull scullc sculld misc_progs completion misc_modules

LINUX_KERNEL_VERSION=5.15.0
PROJ_DIR=$(HOME)/mydrivers
CC=$(CROSS_COMPILE)gcc
SCULL_COMMON_DIR := $(PROJ_DIR)/common_scull
SCULL_COMMON_SRC := $(SCULL_COMMON_DIR)/main.c
SCULL_COMMON_OBJ := $(SCULL_COMMON_SRC:.c=.o)

export LINUX_KERNEL_VERSION HOME KERNEL_CODE_DIR PROJ_DIR CC
export SCULL_COMMON_DIR SCULL_COMMON_SRC SCULL_COMMON_OBJ

all: subdirs
subdirs:
	for n in $(SUBDIRS); do $(MAKE) -C $$n || exit 1; done
	@echo $(ARCH)

clean:
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean; done
