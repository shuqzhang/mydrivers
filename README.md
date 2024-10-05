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

NOTE: when execute make command, please add sudo mandatorily, or the building for ARCH=arm64 will proceed abnormally.
executed the make with ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- added, or it may build for x86 and some confuse
prompts like upgrade compile, balabala make you feeling amazing.
First make defconfig and compile all. Then make menuconfig to select your configuration and compile all.


2. Busybox to get a bundle of common apps

pre-requisite: instatll libraries the software depends
sudo apt-get install libncurses5-dev libncursesw5-dev 

Downloaded busybox codes from : https://busybox.net/downloads/

make menuconfig to set options
Settings  ---> 
[*] Build static binary (no shared libs)
[*] Build with debug information  

cross-compile-prefix need to be filled with "aarch64-none-linux-gnu-"


3. Rootfs image creation

qemu-img create rootfs.img 4096m
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

Note: the program hang when startup. Please check if the kernel is build successfully.
Kill the program with command: ps -ef | grep qemu | awk  '{print $2}' | xargs kill -9

5. For user mode programs, some libs is the dependencies. 
5.1 Copy the libs from cross-compile development suites to rootfs.
5.2 Set LD_LIBRARY_PATH (using source /etc/profile to setup enviroment variables)


6. Make a sharing folder with qemu.

dd if=/dev/zero of=share.img bs=4M count=1k
mkfs.ext4 share.img

mkdir share
mount -o loop share.img share


7. How to commit codes to git hub
git remote add origin git@github.com:shuqzhang/mydrivers.git
git push origin main
