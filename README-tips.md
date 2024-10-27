1. Following error when make minimal rootfs.
shuqzhan@qemu:/ # setlevel 8
-/bin/sh: setlevel: not found
Solution: copy the libs from cross-compile chain suite to rootfs.

2. downloaded the kernel codes: the last commit is, 850925a8133c73c4a2453c360b2c3beb3bab67c9
Note it for recover. I will go the commit with same version as 5.15.124(the kernel version of this ubuntu )

