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


2. Build drivers

When i use sudo make to build drivers, there is error below,

make[2]: Entering directory '/home/shuqzhan/armlinux/linux-5.4.284'
  CC [M]  /home/shuqzhan/mydrivers/hello/hello.o
In file included from ./include/linux/compiler.h:249,
                 from ./include/linux/init.h:5,
                 from /home/shuqzhan/mydrivers/hello/hello.c:4:
./include/uapi/linux/types.h:5:10: fatal error: asm/types.h: No such file or directory
    5 | #include <asm/types.h>

Finally i used the command below, all building passed.
sudo make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu-


3. cross-compile tmux
 a) No rule for compile forkpty-linux.o, reason: no forkpty-linux.c. solution checkout the 3.4 verion codes to make again
 b) Following compile issue:
 /home/shuqzhan/armlinux/arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-linux-gnu/bin/../lib/gcc/aarch64-none-linux-gnu/13.2.1/../../../../aarch64-none-linux-gnu/bin/ld: input.o: in function `input_reply_clipboard':
input.c:(.text+0x3170): undefined reference to `b64_ntop'
reason: b64_ntop function is not built, with readelf -a compat/base64.o | grep b64, got __b64_ntop, but not b64_ntop.
solution: add #include "compat.h" in base64.c file. then remake.
 c) Following compile issue:
 compat/base64.c:134:1: error: conflicting types for ‘b64_ntop’; have ‘int(const u_char *, size_t,  char *, size_t)’ {aka ‘int(const unsigned char *, long unsigned int,  char *, long unsigned int)’}
  134 | b64_ntop(u_char const *src,
      | ^~~~~~~~
In file included from compat/base64.c:58:
./compat.h:378:18: note: previous declaration of ‘b64_ntop’ with type ‘int(const char *, size_t,  char *, size_t)’ {aka ‘int(const char *, long unsigned int,  char *, long unsigned int)’}
  378 | int              b64_ntop(const char *, size_t, char *, size_t);

reason: source file base64.c using u_char, which is not match with header file function using char.
solution: change them into same type. 
