1. Following error when make minimal rootfs.
shuqzhan@qemu:/ # setlevel 8
-/bin/sh: setlevel: not found
Solution: copy the libs from cross-compile chain suite to rootfs.

2. downloaded the kernel codes: the last commit is, 850925a8133c73c4a2453c360b2c3beb3bab67c9
Note it for recover. I will go the commit with same version as 5.15.124(the kernel version of this ubuntu )

3. To get kgdb debug mode in virtual box ubuntu:
 added the commands below in .bashrc
 echo 1 > /proc/sys/kernel/sysrq
echo ttyS1 > /sys/module/kgdboc/parameters/kgdboc
After that, the ubuntu targe enter into a pending state, waiting for gdb debug


