FILES = setlevel hello

KERNELDIR ?= $(KERNEL_CODE_DIR)
INCLUDEDIR = $(KERNELDIR)/include
CFLAGS = -I$(INCLUDEDIR)

all:
	for n in $(FILES); do $(CROSS_COMPILE)gcc $(CFLAGS) -o $$n $$n.c || exit 1; done

clean:
	rm -f $(FILES) *~ core *.o *.ko
