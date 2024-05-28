
#SUBDIRS =  misc-progs misc-modules \
#           skull scull scullc sculld scullp scullv sbull snull\
#	   short shortprint pci simple usb tty lddbus

SUBDIRS = hello globalmem scullc misc_progs

LINUX_KERNEL_VERSION=5.4.275
HOME=/home/$(shell users)
KERNEL_CODE_DIR = $(HOME)/armlinux/linux-$(LINUX_KERNEL_VERSION)
PROJ_DIR=$(HOME)/mydrivers

export LINUX_KERNEL_VERSION HOME KERNEL_CODE_DIR PROJ_DIR

all: subdirs

subdirs:
	for n in $(SUBDIRS); do $(MAKE) -C $$n || exit 1; done

clean:
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean; done
