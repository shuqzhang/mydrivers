How to cross-compile a open source project?

1. With the example util-linux, we can learn the knowledge.

Configure the build options,

export ARCH=arm64
export CROSS_COMPILE=aarch64-none-linux-gnu-
export CC=${CROSS_COMPILE}gcc
export LD=${CROSS_COMPILE}ld
export STRIP=${CROSS_COMPILE}strip
export OBJCOPY=${CROSS_COMPILE}objcopy
export OBJDUMP=${CROSS_COMPILE}objdump

Configure util-linux,

./configure --host=aarch64-none-linux-gnu --prefix=/usr --disable-makeinstall-chown --without-python --without-ncurses

Compile and install

make -j$(nproc)
make install


Notice: 

During build project, there is error like, aarch64-none-linux-gnu-ar : commands not found, which means the compile tools is not found. The reason is that when execute make, we need using sudo to get permission of root, but sudo will reset environment PATH variable, with a default PATH which does not contain the path of cross-compile folder. One tricky method is using alias sudo="sudo env PATH=$PATH" set in ~/.bashrc. The error will not occur after doing that.


