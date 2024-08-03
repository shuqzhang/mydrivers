# mydrivers

Notes

1. build a kernel myself but not using commercial release

Using qemu to virtualize kernel running.

Downloaded the qemu for arm platform by commands: sudo apt-get install qemu-system-arm


Using arm64 cross-compile development suites
https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads

Get kernel codes from: https://www.kernel.org/
Current I used the version 5.4.275

Build kernel : make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- Image -j8

Below method can work if the make command cannot find aarch64-none-linux-gnu-gcc.

export ARCH=arm64
export CROSS_COMPILE=aarch64-none-linux-gnu-
make defconfig
make all -j8

2. Busybox to get a bundle of common apps

Downloaded busybox codes from : https://busybox.net/downloads/

make menuconfig to set options
Settings  ---> 
[*] Build static binary (no shared libs)
[*] Build with debug information  


3. Rootfs image creation

qemu-img create rootfs.img 512m
mkfs.ext4 rootfs.img


mount rootfs.img to rootfs:
mkdir rootfs
sudo mount rootfs.img rootfs

Copy built busybox to rootfs and create some directories for mount VFS
sudo cp -rf _install/*  rootfs
sudo mkdir proc sys dev etc etc/init.d
sudo vim etc/init.d/rcS

create rcS (launch scripts) to mount filesystems
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mount -t debugfs none /sys/kernel/debug
...

Add execute previlege for rcS

sudo chmod +x  etc/init.d/rcS
sudo umount rootfs


4. Current using the commends below to run kernel

qemu-system-aarch64 \
    -machine virt,virtualization=true,gic-version=3 \
    -nographic \
    -m size=1024M \
    -drive format=raw,file=rootfs.img \
    -append "root=/dev/vda rw" \
    -kernel Image.gz \
    -smp 2 \
    -hdb share.img \
    -cpu cortex-a53


5. For user mode programs, some libs is the dependencies. 
5.1 Copy the libs from cross-compile development suites to rootfs.
5.2 Set LD_LIBRARY_PATH (using source /etc/profile to setup enviroment variables)


6. Make a sharing folder with qemu.

dd if=/dev/zero of=share.img bs=4M count=1k
mkfs.ext4 share.img

mkdir share
mount -o loop share.img share


