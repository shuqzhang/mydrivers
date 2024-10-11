1. Following error when make minimal rootfs.
shuqzhan@qemu:/ # setlevel 8
-/bin/sh: setlevel: not found
Solution: copy the libs from cross-compile chain suite to rootfs.