How to build the rootfs for arm platform?

./mountshare.sh to get rootfs with mounted.

1. Copy busybox to rootfs folder (please refer to README.md)

2. Copy basic libraries from cross-compile package to rootfs folder

sudo cp ~/armlinux/arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc ~/armlinux/rootfs/. -rf

sudo cp ~/armlinux/arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-linux-gnu/lib64/ ~/armlinux/rootfs/. -rf

sudo cp ~/armlinux/arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/lib64/ ~/armlinux/rootfs/. -rf

(cd rootfs && sudo mkdir util-linux)
sudo cp ~/armlinux/util-linux-2.35-rc1/.libs ~/armlinux/rootfs/util-linux/. -rf


3. Add lib folders to PATH in linux arm system.

 cd rootfs/etc/
sudo touch profile
sudo vi profile
Edit profile and added following line to the file, then save
export LD_LIBRARY_PATH=/lib64:/libc/lib:/libc/lib64:/util-linux:$LD_LIBRARY_PATH
export PATH=/libc/sbin:/libc/usr/bin:libc/usr/sbin:/util-linux:$PATH


sudo vi rootfs/etc/init.d/rcS
Edit rcS and append following line to the file, then save
source /etc/profile


4. for lsblk executive, the file has info below
 file lsblk
lsblk: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-aarch64.so.1, for GNU/Linux 3.7.0, with debug_info, not stripped

, which means it depends on /lib/ld-linux-aarch64.so.1, so we need make link file under /lib and linked with /libc/lib/ld-linux-aarch64.so.1. With commands below to finish it.

ln /libc/lib/ld-linux-aarch64.so.1 ld-linux-aarch64.so.1


5. To build lsblk executive, we need cross-compile util-linux codes. please refers the file README-cross-compile.md.


