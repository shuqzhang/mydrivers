1. Build zlib

change configure file, with CHOST=aarch64-none-linux-gnu written in top of scripts

2. Build openssl
(cd /home/shuqzhan/armlinux/prefix/ && mkdir openssh openssl)
sudo ./Configure linux-aarch64 --prefix=/home/shuqzhan/armlinux/prefix/openssh --openssldir=/home/shuqzhan/armlinux/prefix/openssl --cross-compile-prefix=aarch64-none-linux-gnu-

3. Build openssh
sudo ./autoheader
sudo ./autoconf
sudo ./configure --host=aarch64-none-linux-gnu --prefix=/home/shuqzhan/armlinux/prefix/openssh --with-zlib=/home/shuqzhan/armlinux/zlib-1.3.1 --with-ssl-dir=/home/shuqzhan/armlinux/openssl/

4. Deploy openssh to target
4.1 Having ignored the "make install" error in step 3, which caused login errors. But the reason is unknown. After sshd scripts finished in target, I tried to login into the sshd server in target, but always failed. From client, the terminal show the session closed by server (192.168.2.129). The client can show the public key from server but not show login with password. From the debug infor from server, the session auths with 'none' method but not with 'password'. Fortunately, I made big mistake that remove the ssl library and tried to install the ssl manually by building the source code and installing the ssh tools. My analysis was that the client ssh tools is not match with server. But that mistake helps me. Why? I almost damaged the ubuntu system after i using 'sudo apt-get remove ssl'. When i ignored the prompt that remove ssl will cause lots of apps not work, for example, google-chrome, vscode, finally system almost not work. This taught me that the privilege of root cannot be abused. I had to establish system from baseland. During which procedure i found that openssh had not been success installed. And this failure helped me successfully deployed openssh in target.

4.2 The failure was that the install from Makefile of openssh was not success executed but ignored by me before. The second time i did not ignore them but check why they failed.
The reason is the Makefile install step default install ssh tools in host machine, but tools are build with cross-compile chains for target machine. The ssh tools built were not identified by host system. So the solution is temperarily ignore the install error, but done them in target mananually.
After that, i found ssh login procedure works.

4.3 When i using root to login ssh server, the server failed for password. Solution is following https://www.cnblogs.com/sunzebo/articles/9609457.html , to configure root permitted.

4.4 When i successfully login, there is always "PTY allocation request failed on channel 0" error. But it does not impact the session working. So just ignore it.


