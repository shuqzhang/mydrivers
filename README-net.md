1. How to conifigure tap network for qemu vm connected with host ? 

step 1. Configure tap ifname for qemu in host machine

sudo ip tuntap add dev tap0 mode tap
sudo ip link set dev tap0 up
sudo ip address add dev tap0 192.168.2.128/24
ifconfig

step 2. qemu start with net tap parameters
example, ./qemu-system-arm -M vexpress-a9 -m 512M -kernel zImage -append "rdinit=/linuxrc console=ttyAMA0 loglevel=8" -dtb vexpress-v2p-ca9.dtb -nographic -net nic -net tap,ifname=tap0,script=no,downscript=no

or reference to scripts qemuarm_tap under root directory of this repo

step 3. configure ip address for eth0 of  qemu vm linux 

ip addr add 192.168.2.129/24 dev eth0
ip link set eth0 up



2. Checking why ssh connection failed. ssh server close port 22.

With command below to check the process who bind port num 22.
sudo netstat -tulpn | grep :22 

3. While compile libbsd library, the configure scripts always prompt "configure: error: cannot find required MD5 functions in libc or libmd". Further checked the config.log, I found the arm cross-compiler tools cannot find libmd with logged "aarch64-none-linux-gnu/bin/ld: cannot find -lmd: No such file or directory. 
From the log, I know that the ld program cannot find the md library. Having tried many many tests, like, 1) add the libmd folder path into LD_LIBRARY_PATH, 2) add the libmd path folder into file under /etc/ld.so.conf.d. 3) make install libmd into default path. But no methods can fix it.
Forget one thing, before that, I tried make the libbsd with no cross-compile option, the result is OK. That help me ensure that the codes i downloaded is OK. So the issue comes from the cross-compile support of codes.

The final solution is add LDFLAGS=-L/home/shuqzhan/armlinux/prefix/lib to the comfigure options, like, "sudo ./configure --host=aarch64-none-linux-gnu LDFLAGS=-L/home/shuqzhan/armlinux/prefix/lib", making the configure passed. Before which i checked the configure.ac file that LDFLAGS is empty value before intialization.

Another issue is, when make the libbsd programe, it prompts as below.
In file included from md5.c:27:
../include/bsd/md5.h:29:15: fatal error: md5.h: No such file or directory
   29 | #include_next <md5.h>
      |               ^~~~~~~


Fixed this issue with command "sudo ./configure --host=aarch64-none-linux-gnu --prefix=/home/shuqzhan/armlinux/prefix LDFLAGS=-L/home/shuqzhan/armlinux/prefix/lib CFLAGS=-I/home/shuqzhan/armlinux/prefix/include"
