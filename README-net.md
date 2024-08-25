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

