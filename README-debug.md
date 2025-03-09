1. KGDB debug driver codes step by step

step1. start qemuarm_gdb, qemuarm will enter into wait mode.

step2. aarch64-none-linux-gnu-gdb vmlinux (Same kernel as qemu started)

step3. after enter into gdb, type 'target remote :1234' to connect with qemu.

step4. type 'c' to make qemu continue, enter into system.

step5. load drivers in qemu linux system.

step6. get info from /sys/module/XXXX/sections/.text

step7. load driver(xxxx.ko) symbols, ctrl+C

step8. type like this, 'add-symbol-file mydrivers/xxxx.ko -s .text ffffabcd12345678', ane enter

step9. set break point, like this 'break scull_read', and type 'c' enter

step10. execute a read device in qemu, like 'cat /dev/scull', gdb will get the break point.