#!/bin/sh
chmod +x common_functions.sh
chmod +x scull_load_multi.sh
./scull_load_multi.sh scull scull 4
./scull_load_multi.sh scull scull_p 4
./scull_load_multi.sh scull scull_single 1
