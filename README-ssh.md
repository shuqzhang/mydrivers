1. Build zlib

change configure file, with CHOST=aarch64-none-linux-gnu written in top of scripts

2. Build openssl
(cd /home/shuqzhan/armlinux/prefix/ && mkdir openssh openssl)
sudo ./Configure linux-aarch64 --prefix=/home/shuqzhan/armlinux/prefix/openssh --openssldir=/home/shuqzhan/armlinux/prefix/openssl --cross-compile-prefix=aarch64-none-linux-gnu-

3. Build openssh
sudo ./autoheader
sudo ./autoconf
sudo ./configure --host=aarch64-none-linux-gnu --prefix=/home/shuqzhan/armlinux/prefix/openssh --with-zlib=/home/shuqzhan/armlinux/zlib-1.3.1 --with-ssl-dir=/home/shuqzhan/armlinux/openssl/

