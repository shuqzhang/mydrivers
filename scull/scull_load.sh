#!/bin/sh
chmod +x common_functions.sh
chmod +x scull_load_multi.sh
chmod +x $(pwd)/../misc_progs/*
./scull_load_multi.sh scull scull 4
./scull_load_multi.sh scull scull_p 4
./scull_load_multi.sh scull scull_single 1
./scull_load_multi.sh scull scull_uid 1
./scull_load_multi.sh scull scull_wuid 1
./scull_load_multi.sh scull scull_copy 1
